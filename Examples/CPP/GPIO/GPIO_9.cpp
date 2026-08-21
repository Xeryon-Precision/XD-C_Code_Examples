#include <gpiod.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>

constexpr unsigned int PWM_PIN = 17;
constexpr unsigned int FORWARD_PIN = 27;
constexpr unsigned int BACKWARD_PIN = 22;

constexpr unsigned int SWITCH_PIN_MINUS = 25;
constexpr unsigned int SWITCH_PIN_PLUS  = 24;

constexpr int PWM_FREQ = 1000;
constexpr int PWM_PERIOD_US = 1000000 / PWM_FREQ;
constexpr int STEP_DELAY_MS = 500;

int direction = 1;
int speed = 0;
int speed_direction = 1;

void sleep_us(int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

void set(gpiod::line_request& req, unsigned int pin, bool value) {
    req.set_value(pin, value ? gpiod::line::value::ACTIVE
                             : gpiod::line::value::INACTIVE);
}

bool get(gpiod::line_request& req, unsigned int pin) {
    return req.get_value(pin) == gpiod::line::value::ACTIVE;
}

void stop_motion(gpiod::line_request& req) {
    set(req, FORWARD_PIN, false);
    set(req, BACKWARD_PIN, false);
}

void apply_direction(gpiod::line_request& req) {
    if (direction == 1) {
        set(req, FORWARD_PIN, true);
        set(req, BACKWARD_PIN, false);
        std::cout << "Direction: +" << std::endl;
    } else {
        set(req, FORWARD_PIN, false);
        set(req, BACKWARD_PIN, true);
        std::cout << "Direction: -" << std::endl;
    }
}

void set_speed(int percent) {
    speed = std::clamp(percent, 0, 100);
    std::cout << "PWM: " << speed << "%" << std::endl;
}

void pwm_cycle(gpiod::line_request& req) {
    int high_time = PWM_PERIOD_US * speed / 100;
    int low_time  = PWM_PERIOD_US - high_time;

    if (high_time > 0) {
        set(req, PWM_PIN, true);
        sleep_us(high_time);
    }

    if (low_time > 0) {
        set(req, PWM_PIN, false);
        sleep_us(low_time);
    }
}

int main() {
    try {
        auto outputs = gpiod::chip("/dev/gpiochip4")
            .prepare_request()
            .set_consumer("xeryon-pwm-output")
            .add_line_settings(
                {PWM_PIN, FORWARD_PIN, BACKWARD_PIN},
                gpiod::line_settings()
                    .set_direction(gpiod::line::direction::OUTPUT)
                    .set_output_value(gpiod::line::value::INACTIVE)
            )
            .do_request();

        auto inputs = gpiod::chip("/dev/gpiochip4")
            .prepare_request()
            .set_consumer("xeryon-switch-input")
            .add_line_settings(
                {SWITCH_PIN_MINUS, SWITCH_PIN_PLUS},
                gpiod::line_settings()
                    .set_direction(gpiod::line::direction::INPUT)
                    .set_bias(gpiod::line::bias::PULL_DOWN)
            )
            .do_request();

        apply_direction(outputs);
        set_speed(speed);

        bool last_minus = false;
        bool last_plus = false;

        auto last_speed_update = std::chrono::steady_clock::now();

        while (true) {
            bool minus_now = get(inputs, SWITCH_PIN_MINUS);
            bool plus_now  = get(inputs, SWITCH_PIN_PLUS);

            if (minus_now && !last_minus) {
                direction = -1;
                apply_direction(outputs);
            }

            if (plus_now && !last_plus) {
                direction = 1;
                apply_direction(outputs);
            }

            last_minus = minus_now;
            last_plus = plus_now;

            pwm_cycle(outputs);

            auto now = std::chrono::steady_clock::now();
            auto elapsed_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_speed_update
                ).count();

            if (elapsed_ms >= STEP_DELAY_MS) {
                set_speed(speed);

                if (speed >= 100) {
                    speed = 100;
                    speed_direction = -1;
                } else if (speed <= 0) {
                    speed = 0;
                    speed_direction = 1;
                }

                speed += speed_direction;
                last_speed_update = now;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fout: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}