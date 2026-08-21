#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define CHIP_PATH  "/dev/gpiochip0"   // check met gpiodetect
#define A_PIN      17
#define B_PIN      27
#define ENABLE_PIN 22
#define INDEX_PIN  23

static struct gpiod_chip *chip;
static struct gpiod_line_request *a_req, *b_req, *en_req, *index_req;
static volatile int running = 1;

/* Quadrature-states: {A, B} */
static const int quad_states[4][2] = {
    {0, 0},
    {1, 0},
    {1, 1},
    {0, 1},
};

static int quad_index = 0;

void handle_sigint(int sig) { (void)sig; running = 0; }

/* Helper: vraag één GPIO-lijn op als output, met gegeven startwaarde */
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

void set_line(struct gpiod_line_request *req, unsigned int offset, int value) {
    gpiod_line_request_set_value(req, offset,
        value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
}

void enable_driver()  { set_line(en_req, ENABLE_PIN, 1); }
void disable_driver() { set_line(en_req, ENABLE_PIN, 0); }

void do_index() {
    set_line(index_req, INDEX_PIN, 0);
    sleep(1);
    set_line(index_req, INDEX_PIN, 1);
    sleep(1);
    set_line(index_req, INDEX_PIN, 0);
}

/* Python: quad_index = (quad_index - 1) % 4  -> Python's % is altijd niet-negatief.
   In C moeten we dat zelf afdwingen. */
static inline int mod4(int x) {
    int r = x % 4;
    return (r < 0) ? r + 4 : r;
}

void apply_quad_state(void) {
    set_line(a_req, A_PIN, quad_states[quad_index][0]);
    set_line(b_req, B_PIN, quad_states[quad_index][1]);
}

void one_mm_forward(int delay_us) {
    quad_index = mod4(quad_index + 1);
    apply_quad_state();
    usleep(delay_us);
}

void one_mm_backward(int delay_us) {
    quad_index = mod4(quad_index - 1);
    apply_quad_state();
    usleep(delay_us);
}

void forward(int mm) {
    for (int i = 0; i < mm && running; i++) {
        one_mm_forward(1000);   // 1 ms, zoals default delay=0.001 in Python
        usleep(500000);         // 0.5s
    }
}

void backward(int mm) {
    for (int i = 0; i < mm && running; i++) {
        one_mm_backward(1000);
        usleep(50000);          // 0.05s
    }
}

int main(void) {
    signal(SIGINT, handle_sigint);

    chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) { perror("open chip"); return 1; }

    /* Startwaarden: A/B op quad_states[0], ENABLE en INDEX op 0 */
    a_req     = request_output_line(chip, A_PIN,      quad_states[quad_index][0], "xdc-a");
    b_req     = request_output_line(chip, B_PIN,      quad_states[quad_index][1], "xdc-b");
    en_req    = request_output_line(chip, ENABLE_PIN, 0,                          "xdc-enable");
    index_req = request_output_line(chip, INDEX_PIN,  0,                          "xdc-index");

    if (!a_req || !b_req || !en_req || !index_req) {
        fprintf(stderr, "Kon een of meer GPIO-lines niet aanvragen\n");
        return 1;
    }

    enable_driver();
    do_index();
    sleep(2);

    while (running) {
        forward(10);
        backward(20);
        forward(10);
    }

    disable_driver();
    gpiod_line_request_release(a_req);
    gpiod_line_request_release(b_req);
    gpiod_line_request_release(en_req);
    gpiod_line_request_release(index_req);
    gpiod_chip_close(chip);

    return 0;
}