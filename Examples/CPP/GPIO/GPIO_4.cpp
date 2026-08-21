#include <gpiod.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <array>

constexpr unsigned int A = 17;
constexpr unsigned int B = 27;
constexpr unsigned int ENABLE = 22;
constexpr unsigned int INDEX = 23;

const std::array<std::pair<bool, bool>, 4> quad_states = {{
    {false, false},
    {true,  false},
    {true,  true},
    {false, true}
}};

int quad_index = 0;

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void sleep_us(int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

void set(gpiod::line_request& req, unsigned int pin, bool value) {
    req.set_value(pin, value ? gpiod::line::value::ACTIVE
                             : gpiod::line::value::INACTIVE);
}

void set_quad(gpiod::line_request& req) {
    set(req, A, quad_states[quad_index].first);
    set(req, B, quad_states[quad_index].second);
}

void enable(gpiod::line_request& req) {
    set(req, ENABLE, true);
}

void disable(gpiod::line_request& req) {
    set(req, ENABLE, false);
}

void index_search(gpiod::line_request& req) {
    set(req, INDEX, false);
    sleep_ms(1000);

    set(req, INDEX, true);
    sleep_ms(1000);

    set(req, INDEX, false);
}

void one_mm_forward(gpiod::line_request& req, int delay_us = 1000) {
    quad_index = (quad_index + 1) % 4;
    set_quad(req);
    sleep_us(delay_us);
}

void one_mm_backward(gpiod::line_request& req, int delay_us = 1000) {
    quad_index = (quad_index - 1 + 4) % 4;
    set_quad(req);
    sleep_us(delay_us);
}

void forward(gpiod::line_request& req, int mm) {
    for (int i = 0; i < mm; i++) {
        one_mm_forward(req);
        sleep_ms(500);
    }
}

void backward(gpiod::line_request& req, int mm) {
    for (int i = 0; i < mm; i++) {
        one_mm_backward(req);
        sleep_ms(50);
    }
}

int main() {
    try {
        auto request = gpiod::chip("/dev/gpiochip4")
            .prepare_request()
            .set_consumer("xeryon-quadrature")
            .add_line_settings(
                {A, B, ENABLE, INDEX},
                gpiod::line_settings()
                    .set_direction(gpiod::line::direction::OUTPUT)
                    .set_output_value(gpiod::line::value::INACTIVE)
            )
            .do_request();

        // Initial state zoals in Python
        set_quad(request);
        disable(request);
        set(request, INDEX, false);

        enable(request);

        index_search(request);
        sleep_ms(2000);

        while (true) {
            forward(request, 10);
            backward(request, 20);
            forward(request, 10);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fout: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}