#include <gpiod.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <time.h>

#define CHIP_PATH        "/dev/gpiochip0"   // check met gpiodetect
#define PWM_PIN          17
#define DIR_PIN          27
#define ENABLE_PIN       22
#define SWITCH_MINUS_PIN 25
#define SWITCH_PLUS_PIN  24

#define PWM_FREQ_HZ      1000
#define STEP_DELAY_US    500000   // 0.5s
#define DEBOUNCE_MS      200

static struct gpiod_chip *chip;
static struct gpiod_line_request *pwm_req, *dir_req, *en_req;
static struct gpiod_line_request *sw_minus_req, *sw_plus_req;

static atomic_int running = 1;
static atomic_int direction = 1;          // 1 = plus, -1 = minus
static atomic_int pwm_duty_percent = 0;   // 0-100

static pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

void handle_sigint(int sig) { (void)sig; atomic_store(&running, 0); }

/* ---------- Helpers ---------- */

static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

struct gpiod_line_request *request_output_line(struct gpiod_chip *chip,
                                                 unsigned int offset,
                                                 int initial_value,
                                                 const char *consumer) {
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings,
        initial_value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, consumer);

    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    return req;
}

/* Input met pull-down + rising-edge detectie (equivalent van GPIO.PUD_DOWN + GPIO.RISING) */
struct gpiod_line_request *request_input_rising(struct gpiod_chip *chip,
                                                  unsigned int offset,
                                                  const char *consumer) {
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_RISING);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, consumer);

    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    return req;
}

void set_line(struct gpiod_line_request *req, unsigned int offset, int value) {
    gpiod_line_request_set_value(req, offset,
        value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
}

/* ---------- Functionele equivalenten van de Python-functies ---------- */

void enable_driver()  { set_line(en_req, ENABLE_PIN, 1); }
void disable_driver() { set_line(en_req, ENABLE_PIN, 0); }

void apply_direction(void) {
    int dir = atomic_load(&direction);
    pthread_mutex_lock(&print_lock);
    if (dir == 1) {
        set_line(dir_req, DIR_PIN, 1);
        printf("Direction: +\n");
    } else {
        set_line(dir_req, DIR_PIN, 0);
        printf("Direction: -\n");
    }
    fflush(stdout);
    pthread_mutex_unlock(&print_lock);
}

void set_speed(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    atomic_store(&pwm_duty_percent, percent);
    pthread_mutex_lock(&print_lock);
    printf("PWM: %d%%\n", percent);
    fflush(stdout);
    pthread_mutex_unlock(&print_lock);
}

/* ---------- PWM-thread (vervangt GPIO.PWM) ---------- */

void *pwm_thread_func(void *arg) {
    (void)arg;
    const long period_us = 1000000L / PWM_FREQ_HZ;

    while (atomic_load(&running)) {
        int duty = atomic_load(&pwm_duty_percent);
        long on_us  = period_us * duty / 100;
        long off_us = period_us - on_us;

        if (on_us > 0) {
            set_line(pwm_req, PWM_PIN, 1);
            usleep(on_us);
        }
        if (off_us > 0) {
            set_line(pwm_req, PWM_PIN, 0);
            usleep(off_us);
        }
    }
    set_line(pwm_req, PWM_PIN, 0);
    return NULL;
}

/* ---------- Schakelaar-thread (vervangt GPIO.add_event_detect) ---------- */

void *switch_thread_func(void *arg) {
    (void)arg;

    int fd_minus = gpiod_line_request_get_fd(sw_minus_req);
    int fd_plus  = gpiod_line_request_get_fd(sw_plus_req);

    struct gpiod_edge_event_buffer *buf = gpiod_edge_event_buffer_new(4);

    long last_minus_ms = 0;
    long last_plus_ms   = 0;

    struct pollfd pfds[2];
    pfds[0].fd = fd_minus; pfds[0].events = POLLIN;
    pfds[1].fd = fd_plus;  pfds[1].events = POLLIN;

    while (atomic_load(&running)) {
        int ret = poll(pfds, 2, 200); // 200ms timeout zodat running gecheckt blijft worden
        if (ret <= 0) continue;

        if (pfds[0].revents & POLLIN) {
            int n = gpiod_line_request_read_edge_events(sw_minus_req, buf, 4);
            if (n > 0) {
                long t = now_ms();
                if (t - last_minus_ms >= DEBOUNCE_MS) {
                    last_minus_ms = t;
                    atomic_store(&direction, -1);
                    apply_direction();
                }
            }
        }

        if (pfds[1].revents & POLLIN) {
            int n = gpiod_line_request_read_edge_events(sw_plus_req, buf, 4);
            if (n > 0) {
                long t = now_ms();
                if (t - last_plus_ms >= DEBOUNCE_MS) {
                    last_plus_ms = t;
                    atomic_store(&direction, 1);
                    apply_direction();
                }
            }
        }
    }

    gpiod_edge_event_buffer_free(buf);
    return NULL;
}

/* ---------- Main ---------- */

int main(void) {
    signal(SIGINT, handle_sigint);

    chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) { perror("open chip"); return 1; }

    pwm_req = request_output_line(chip, PWM_PIN,    0, "xdc-pwm");
    dir_req = request_output_line(chip, DIR_PIN,    0, "xdc-dir");
    en_req  = request_output_line(chip, ENABLE_PIN, 0, "xdc-enable");

    sw_minus_req = request_input_rising(chip, SWITCH_MINUS_PIN, "xdc-sw-minus");
    sw_plus_req  = request_input_rising(chip, SWITCH_PLUS_PIN,  "xdc-sw-plus");

    if (!pwm_req || !dir_req || !en_req || !sw_minus_req || !sw_plus_req) {
        fprintf(stderr, "Kon een of meer GPIO-lines niet aanvragen\n");
        return 1;
    }

    enable_driver();
    apply_direction();
    set_speed(0);

    pthread_t pwm_thread, switch_thread;
    pthread_create(&pwm_thread, NULL, pwm_thread_func, NULL);
    pthread_create(&switch_thread, NULL, switch_thread_func, NULL);

    int speed = 0;
    int speed_direction = 1;

    while (atomic_load(&running)) {
        set_speed(speed);
        usleep(STEP_DELAY_US);

        if (speed >= 100) {
            speed = 100;
            speed_direction = -1;
        } else if (speed <= 0) {
            speed = 0;
            speed_direction = 1;
        }

        speed += speed_direction;
    }

    printf("Stopped by user\n");

    set_speed(0);
    atomic_store(&running, 0);   // zorgt dat pwm/switch-threads ook stoppen

    pthread_join(pwm_thread, NULL);
    pthread_join(switch_thread, NULL);

    disable_driver();

    gpiod_line_request_release(pwm_req);
    gpiod_line_request_release(dir_req);
    gpiod_line_request_release(en_req);
    gpiod_line_request_release(sw_minus_req);
    gpiod_line_request_release(sw_plus_req);
    gpiod_chip_close(chip);

    return 0;
}