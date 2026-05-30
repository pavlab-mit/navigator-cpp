#include "led.h"
#include "gpio.h"

#include <unistd.h>

static const int LED_PINS[3] = {11, 24, 25};
static bool s_led_ok = false;

std::string led_init(GpioChip* gpio) {
    s_led_ok = false;
    for (int i = 0; i < 3; i++) {
        std::string err = gpio_request_output(gpio, LED_PINS[i], 1, "nav-led");
        if (!err.empty()) return "led_init: pin " + std::to_string(LED_PINS[i]) + ": " + err;
        usleep(30000);
    }
    s_led_ok = true;
    return "";
}

std::string led_set(GpioChip* gpio, int led, bool on) {
    if (!s_led_ok) return "led_set: not initialized";
    if (led < 0 || led > 2) return "led_set: invalid led " + std::to_string(led);
    return gpio_set(gpio, LED_PINS[led], on ? 0 : 1);
}

std::string led_get(GpioChip* gpio, int led, bool& on) {
    on = false;
    if (!s_led_ok) return "led_get: not initialized";
    if (led < 0 || led > 2) return "led_get: invalid led " + std::to_string(led);
    int val = 0;
    std::string err = gpio_get(gpio, LED_PINS[led], val);
    if (!err.empty()) return "led_get: " + err;
    on = (val == 0);
    return "";
}
