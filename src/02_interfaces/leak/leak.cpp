#include "leak.h"
#include "gpio.h"

#define LEAK_PIN 27

static bool s_leak_ok = false;

std::string leak_init(GpioChip* gpio) {
    s_leak_ok = false;
    std::string err = gpio_request_input(gpio, LEAK_PIN, "nav-leak");
    if (!err.empty()) return "leak_init: " + err;
    s_leak_ok = true;
    return "";
}

std::string leak_read(GpioChip* gpio, bool& detected) {
    detected = false;
    if (!s_leak_ok) return "leak_read: not initialized";
    int val = 0;
    std::string err = gpio_get(gpio, LEAK_PIN, val);
    if (!err.empty()) return "leak_read: " + err;
    detected = (val == 1);
    return "";
}
