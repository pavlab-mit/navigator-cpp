#include "gpio.h"

#ifdef __linux__

#include <gpiod.h>
#include <map>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <unistd.h>

// ─── GpioChip with dual backend ────────────────────────────────

struct GpioLine {
    struct gpiod_line* gpiod_line = nullptr;
    int pin = -1;
};

struct GpioChip {
    struct gpiod_chip* chip;
    std::map<int, GpioLine> lines;
};

std::string gpio_open(const char* chip_path, GpioChip*& chip_out) {
    chip_out = nullptr;
    if (!chip_path) return "gpio_open: null chip_path";
    struct gpiod_chip* chip = gpiod_chip_open(chip_path);
    if (!chip)
        return std::string("gpio_open: failed to open ") + chip_path + ": " + strerror(errno);
    chip_out = new GpioChip();
    chip_out->chip = chip;
    return "";
}

void gpio_close(GpioChip*& gc) {
    if (!gc) return;
    for (auto& [pin, gl] : gc->lines) {
        if (gl.gpiod_line) gpiod_line_release(gl.gpiod_line);
    }
    gpiod_chip_close(gc->chip);
    delete gc;
    gc = nullptr;
}

std::string gpio_request_output(GpioChip* gc, int pin, int initial_value, const char* label) {
    if (!gc) return "gpio_request_output: null gpio chip";
    if (gc->lines.find(pin) != gc->lines.end())
        return "gpio_request_output: pin " + std::to_string(pin)
               + " is already requested by this Navigator instance";

    // GPIO character-device ownership is authoritative. Falling back to the
    // deprecated sysfs API hides useful errors (especially EBUSY) and cannot
    // take a line away from another character-device consumer.
    struct gpiod_line* line = gpiod_chip_get_line(gc->chip, pin);
    if (!line)
        return "gpio_request_output: get line " + std::to_string(pin)
               + " failed: " + strerror(errno);
    if (gpiod_line_request_output(line, label, initial_value) == 0) {
        GpioLine gl;
        gl.gpiod_line = line;
        gl.pin = pin;
        gc->lines[pin] = gl;
        return "";
    }
    const int request_errno = errno;
    return "gpio_request_output: libgpiod request failed for pin "
           + std::to_string(pin) + " (consumer=" + (label ? label : "navigator-cpp")
           + "): " + strerror(request_errno);
}

std::string gpio_request_input(GpioChip* gc, int pin, const char* label) {
    if (!gc) return "gpio_request_input: null gpio chip";
    if (gc->lines.find(pin) != gc->lines.end())
        return "gpio_request_input: pin " + std::to_string(pin)
               + " is already requested by this Navigator instance";

    struct gpiod_line* line = gpiod_chip_get_line(gc->chip, pin);
    if (!line)
        return "gpio_request_input: get line " + std::to_string(pin)
               + " failed: " + strerror(errno);
    if (gpiod_line_request_input(line, label) == 0) {
        GpioLine gl;
        gl.gpiod_line = line;
        gl.pin = pin;
        gc->lines[pin] = gl;
        return "";
    }
    const int request_errno = errno;
    return "gpio_request_input: libgpiod request failed for pin "
           + std::to_string(pin) + " (consumer=" + (label ? label : "navigator-cpp")
           + "): " + strerror(request_errno);
}

std::string gpio_set(GpioChip* gc, int pin, int value) {
    if (!gc) return "gpio_set: null gpio chip";
    auto it = gc->lines.find(pin);
    if (it == gc->lines.end())
        return std::string("gpio_set: pin ") + std::to_string(pin) + " not requested";

    if (gpiod_line_set_value(it->second.gpiod_line, value) < 0)
        return std::string("gpio_set: set_value failed for pin ") + std::to_string(pin)
               + ": " + strerror(errno);
    return "";
}

std::string gpio_get(GpioChip* gc, int pin, int& value) {
    if (!gc) return "gpio_get: null gpio chip";
    auto it = gc->lines.find(pin);
    if (it == gc->lines.end())
        return std::string("gpio_get: pin ") + std::to_string(pin) + " not requested";

    int v = gpiod_line_get_value(it->second.gpiod_line);
    if (v < 0)
        return std::string("gpio_get: get_value failed for pin ") + std::to_string(pin)
               + ": " + strerror(errno);
    value = v;
    return "";
}

void gpio_release(GpioChip* gc, int pin) {
    if (!gc) return;
    auto it = gc->lines.find(pin);
    if (it != gc->lines.end()) {
        if (it->second.gpiod_line) gpiod_line_release(it->second.gpiod_line);
        gc->lines.erase(it);
    }
}

#else
struct GpioChip {};
std::string gpio_open(const char*, GpioChip*&) { return "gpio: not supported on this platform"; }
void gpio_close(GpioChip*& p) { p = nullptr; }
std::string gpio_request_output(GpioChip*, int, int, const char*) { return "gpio: not supported on this platform"; }
std::string gpio_request_input(GpioChip*, int, const char*) { return "gpio: not supported on this platform"; }
std::string gpio_set(GpioChip*, int, int) { return "gpio: not supported on this platform"; }
std::string gpio_get(GpioChip*, int, int&) { return "gpio: not supported on this platform"; }
void gpio_release(GpioChip*, int) {}
#endif
