#include "gpio.h"

#ifdef __linux__

#include <gpiod.h>
#include <map>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <unistd.h>

// ─── Sysfs GPIO fallback ───────────────────────────────────────
// Used when libgpiod can't request a pin (e.g., pre-configured by device tree).

static std::string sysfs_export_pin(int pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    if (access(path, F_OK) == 0) return "";

    FILE* f = fopen("/sys/class/gpio/export", "w");
    if (!f) return std::string("sysfs_export: ") + strerror(errno);
    fprintf(f, "%d", pin);
    fclose(f);

    for (int i = 0; i < 50; i++) {
        if (access(path, F_OK) == 0) return "";
        usleep(10000);
    }
    return "sysfs_export: gpio" + std::to_string(pin) + " not created";
}

static std::string sysfs_set_dir(int pin, const char* dir) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    FILE* f = fopen(path, "w");
    if (!f) return std::string("sysfs_set_dir: ") + strerror(errno);
    fprintf(f, "%s", dir);
    fclose(f);
    return "";
}

static std::string sysfs_set_value(int pin, int value) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    FILE* f = fopen(path, "w");
    if (!f) return std::string("sysfs_set_value: ") + strerror(errno);
    fprintf(f, "%d", value);
    fclose(f);
    return "";
}

static int sysfs_get_value(int pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    int val = -1;
    if (fscanf(f, "%d", &val) != 1) val = -1;
    fclose(f);
    return val;
}

// ─── GpioChip with dual backend ────────────────────────────────

struct GpioLine {
    struct gpiod_line* gpiod_line = nullptr;  // null = using sysfs fallback
    int pin = -1;
    bool is_sysfs = false;
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

    // Try libgpiod first
    struct gpiod_line* line = gpiod_chip_get_line(gc->chip, pin);
    if (line && gpiod_line_request_output(line, label, initial_value) == 0) {
        GpioLine gl;
        gl.gpiod_line = line;
        gl.pin = pin;
        gl.is_sysfs = false;
        gc->lines[pin] = gl;
        return "";
    }

    // Fallback to sysfs
    std::string err = sysfs_export_pin(pin);
    if (!err.empty())
        return "gpio_request_output: libgpiod failed and sysfs fallback failed for pin "
               + std::to_string(pin) + ": " + err;

    err = sysfs_set_dir(pin, initial_value ? "high" : "low");
    if (!err.empty())
        return "gpio_request_output: sysfs set direction failed for pin "
               + std::to_string(pin) + ": " + err;

    GpioLine gl;
    gl.gpiod_line = nullptr;
    gl.pin = pin;
    gl.is_sysfs = true;
    gc->lines[pin] = gl;
    return "";
}

std::string gpio_request_input(GpioChip* gc, int pin, const char* label) {
    if (!gc) return "gpio_request_input: null gpio chip";

    // Try libgpiod first
    struct gpiod_line* line = gpiod_chip_get_line(gc->chip, pin);
    if (line && gpiod_line_request_input(line, label) == 0) {
        GpioLine gl;
        gl.gpiod_line = line;
        gl.pin = pin;
        gl.is_sysfs = false;
        gc->lines[pin] = gl;
        return "";
    }

    // Fallback to sysfs
    std::string err = sysfs_export_pin(pin);
    if (!err.empty())
        return "gpio_request_input: libgpiod failed and sysfs fallback failed for pin "
               + std::to_string(pin) + ": " + err;

    err = sysfs_set_dir(pin, "in");
    if (!err.empty())
        return "gpio_request_input: sysfs set direction failed for pin "
               + std::to_string(pin) + ": " + err;

    GpioLine gl;
    gl.gpiod_line = nullptr;
    gl.pin = pin;
    gl.is_sysfs = true;
    gc->lines[pin] = gl;
    return "";
}

std::string gpio_set(GpioChip* gc, int pin, int value) {
    if (!gc) return "gpio_set: null gpio chip";
    auto it = gc->lines.find(pin);
    if (it == gc->lines.end())
        return std::string("gpio_set: pin ") + std::to_string(pin) + " not requested";

    if (it->second.is_sysfs) {
        return sysfs_set_value(pin, value);
    } else {
        if (gpiod_line_set_value(it->second.gpiod_line, value) < 0)
            return std::string("gpio_set: set_value failed for pin ") + std::to_string(pin)
                   + ": " + strerror(errno);
        return "";
    }
}

std::string gpio_get(GpioChip* gc, int pin, int& value) {
    if (!gc) return "gpio_get: null gpio chip";
    auto it = gc->lines.find(pin);
    if (it == gc->lines.end())
        return std::string("gpio_get: pin ") + std::to_string(pin) + " not requested";

    if (it->second.is_sysfs) {
        int v = sysfs_get_value(pin);
        if (v < 0) return "gpio_get: sysfs read failed for pin " + std::to_string(pin);
        value = v;
        return "";
    } else {
        int v = gpiod_line_get_value(it->second.gpiod_line);
        if (v < 0)
            return std::string("gpio_get: get_value failed for pin ") + std::to_string(pin)
                   + ": " + strerror(errno);
        value = v;
        return "";
    }
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
