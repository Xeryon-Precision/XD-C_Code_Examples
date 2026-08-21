#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define CHIP_PATH   "/dev/gpiochip0"   // check met gpiodetect
#define PULSE_PIN   17
#define FORWARD_PIN 27
#define BACKWARD_PIN 22
#define INDEX_PIN   23

static struct gpiod_chip *chip;
static struct gpiod_line_request *pulse_req, *fwd_req, *bwd_req, *index_req;
static volatile int running = 1;

void handle_sigint(int sig) { (void)sig; running = 0; }

/* Helper: vraag één GPIO-lijn op als output, startwaarde 0 */
struct gpiod_line_request *request_output_line(struct gpiod_chip *chip,
                                                 unsigned int offset,
                                                 const char *consumer) {
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

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

void pulse(int width_us) {
    set_line(pulse_req, PULSE_PIN, 1);
    usleep(width_us);
    set_line(pulse_req, PULSE_PIN, 0);
}

void forward(int steps) {
    set_line(fwd_req, FORWARD_PIN, 1);
    set_line(bwd_req, BACKWARD_PIN, 0);

    for (int i = 0; i < steps; i++) {
        pulse(1000);
        usleep(10000);
    }
}

void backward(int steps) {
    set_line(fwd_req, FORWARD_PIN, 0);
    set_line(bwd_req, BACKWARD_PIN, 1);

    for (int i = 0; i < steps; i++) {
        pulse(1000);
        // Let op: origineel Python had hier geen sleep tussen pulsen
    }
}

void do_index() {
    set_line(index_req, INDEX_PIN, 0);
    sleep(1);
    set_line(index_req, INDEX_PIN, 1);
    sleep(1);
    set_line(index_req, INDEX_PIN, 0);
}

int main(void) {
    signal(SIGINT, handle_sigint);

    chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) { perror("open chip"); return 1; }

    pulse_req = request_output_line(chip, PULSE_PIN,   "xdc-pulse");
    fwd_req   = request_output_line(chip, FORWARD_PIN, "xdc-forward");
    bwd_req   = request_output_line(chip, BACKWARD_PIN,"xdc-backward");
    index_req = request_output_line(chip, INDEX_PIN,   "xdc-index");

    if (!pulse_req || !fwd_req || !bwd_req || !index_req) {
        fprintf(stderr, "Kon een of meer GPIO-lines niet aanvragen\n");
        return 1;
    }

    do_index();
    sleep(2);

    while (running) {
        for (int i = 0; i < 10 && running; i++) { forward(1);  usleep(500000); }
        for (int i = 0; i < 20 && running; i++) { backward(1); usleep(50000);  }
        for (int i = 0; i < 10 && running; i++) { forward(1);  usleep(500000); }
    }

    gpiod_line_request_release(pulse_req);
    gpiod_line_request_release(fwd_req);
    gpiod_line_request_release(bwd_req);
    gpiod_line_request_release(index_req);
    gpiod_chip_close(chip);

    return 0;
}