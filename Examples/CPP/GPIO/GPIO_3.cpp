#include <gpiod.hpp>
#include <chrono>
#include <thread>
#include <iostream>

constexpr unsigned int PULSE    = 17;
constexpr unsigned int FORWARD  = 27;
constexpr unsigned int BACKWARD = 22;
constexpr unsigned int INDEX    = 23;

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void set(gpiod::line_request& req, unsigned int pin, bool value) {
    req.set_value(pin, value ? gpiod::line::value::ACTIVE
                             : gpiod::line::value::INACTIVE);
}

void pulse(gpiod::line_request& req, int width_us = 1000) {
    set(req, PULSE, true);
    std::this_thread::sleep_for(std::chrono::microseconds(width_us));
    set(req, PULSE, false);
}

void forward(gpiod::line_request& req, int steps) {
    set(req, FORWARD, true);
    set(req, BACKWARD, false);

    for (int i = 0; i < steps; i++) {
        pulse(req);
        sleep_ms(10);
    }
}

void backward(gpiod::line_request& req, int steps) {
    set(req, FORWARD, false);
    set(req, BACKWARD, true);

    for (int i = 0; i < steps; i++) {
        pulse(req);
    }
}

void index_search(gpiod::line_request& req) {
    set(req, INDEX, false);
    sleep_ms(1000);

    set(req, INDEX, true);
    sleep_ms(1000);

    set(req, INDEX, false);
}

int main() {
    try {
        auto request = gpiod::chip("/dev/gpiochip4")
            .prepare_request()
            .set_consumer("xeryon-gpio")
            .add_line_settings(
                {PULSE, FORWARD, BACKWARD, INDEX},
                gpiod::line_settings()
                    .set_direction(gpiod::line::direction::OUTPUT)
                    .set_output_value(gpiod::line::value::INACTIVE)
            )
            .do_request();

        index_search(request);
        sleep_ms(2000);

        while (true) {
            for (int i = 0; i < 10; i++) {
                forward(request, 1);
                sleep_ms(500);
            }

            for (int i = 0; i < 20; i++) {
                backward(request, 1);
                sleep_ms(50);
            }

            for (int i = 0; i < 10; i++) {
                forward(request, 1);
                sleep_ms(500);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fout: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}