#include "gpio.h"

#ifdef __linux__

#include <gpiod.h>
#include <map>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <unistd.h>

#ifdef NAV_LIBGPIOD_V2

#include <dirent.h>

struct GpioLine {
    struct gpiod_line_request* request = nullptr;
    int pin = -1;
};

struct GpioChip {
    struct gpiod_chip* chip;
    std::map<int, GpioLine> lines;
};

static struct gpiod_line_request* request_line(struct gpiod_chip* chip, int pin, bool output,
                                               int initial_value, const char* label) {
    struct gpiod_line_settings* settings = gpiod_line_settings_new();
    struct gpiod_line_config* line_cfg = gpiod_line_config_new();
    struct gpiod_request_config* req_cfg = gpiod_request_config_new();
    struct gpiod_line_request* request = nullptr;

    if (settings && line_cfg && req_cfg) {
        gpiod_line_settings_set_direction(
            settings, output ? GPIOD_LINE_DIRECTION_OUTPUT : GPIOD_LINE_DIRECTION_INPUT);
        if (output)
            gpiod_line_settings_set_output_value(
                settings, initial_value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
        unsigned int offset = static_cast<unsigned int>(pin);
        if (gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings) == 0) {
            gpiod_request_config_set_consumer(req_cfg, label ? label : "navigator-cpp");
            request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
        }
    }

    const int saved_errno = errno;
    if (req_cfg) gpiod_request_config_free(req_cfg);
    if (line_cfg) gpiod_line_config_free(line_cfg);
    if (settings) gpiod_line_settings_free(settings);
    errno = saved_errno;
    return request;
}

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

std::string gpio_open_by_label(const char* chip_label, GpioChip*& chip_out) {
    chip_out = nullptr;
    if (!chip_label) return "gpio_open_by_label: null chip_label";

    DIR* dir = opendir("/dev");
    if (!dir)
        return std::string("gpio_open_by_label: cannot scan /dev: ") + strerror(errno);

    struct gpiod_chip* found = nullptr;
    while (struct dirent* ent = readdir(dir)) {
        if (std::strncmp(ent->d_name, "gpiochip", 8) != 0) continue;
        const std::string path = std::string("/dev/") + ent->d_name;
        if (!gpiod_is_gpiochip_device(path.c_str())) continue;
        struct gpiod_chip* chip = gpiod_chip_open(path.c_str());
        if (!chip) continue;
        struct gpiod_chip_info* info = gpiod_chip_get_info(chip);
        const bool match = info && std::strcmp(gpiod_chip_info_get_label(info), chip_label) == 0;
        if (info) gpiod_chip_info_free(info);
        if (match) {
            found = chip;
            break;
        }
        gpiod_chip_close(chip);
    }
    closedir(dir);

    if (!found)
        return std::string("gpio_open_by_label: failed to open ") + chip_label
               + ": no gpiochip with that label";
    chip_out = new GpioChip();
    chip_out->chip = found;
    return "";
}

void gpio_close(GpioChip*& gc) {
    if (!gc) return;
    for (auto& [pin, gl] : gc->lines) {
        if (gl.request) gpiod_line_request_release(gl.request);
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

    struct gpiod_line_request* request = request_line(gc->chip, pin, true, initial_value, label);
    if (request) {
        GpioLine gl;
        gl.request = request;
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

    struct gpiod_line_request* request = request_line(gc->chip, pin, false, 0, label);
    if (request) {
        GpioLine gl;
        gl.request = request;
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

    if (gpiod_line_request_set_value(it->second.request, static_cast<unsigned int>(pin),
                                     value ? GPIOD_LINE_VALUE_ACTIVE
                                           : GPIOD_LINE_VALUE_INACTIVE) < 0)
        return std::string("gpio_set: set_value failed for pin ") + std::to_string(pin)
               + ": " + strerror(errno);
    return "";
}

std::string gpio_get(GpioChip* gc, int pin, int& value) {
    if (!gc) return "gpio_get: null gpio chip";
    auto it = gc->lines.find(pin);
    if (it == gc->lines.end())
        return std::string("gpio_get: pin ") + std::to_string(pin) + " not requested";

    enum gpiod_line_value v =
        gpiod_line_request_get_value(it->second.request, static_cast<unsigned int>(pin));
    if (v == GPIOD_LINE_VALUE_ERROR)
        return std::string("gpio_get: get_value failed for pin ") + std::to_string(pin)
               + ": " + strerror(errno);
    value = (v == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
    return "";
}

void gpio_release(GpioChip* gc, int pin) {
    if (!gc) return;
    auto it = gc->lines.find(pin);
    if (it != gc->lines.end()) {
        if (it->second.request) gpiod_line_request_release(it->second.request);
        gc->lines.erase(it);
    }
}

#else

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

std::string gpio_open_by_label(const char* chip_label, GpioChip*& chip_out) {
    chip_out = nullptr;
    if (!chip_label) return "gpio_open_by_label: null chip_label";
    struct gpiod_chip* chip = gpiod_chip_open_by_label(chip_label);
    if (!chip)
        return std::string("gpio_open_by_label: failed to open ") + chip_label
               + ": " + strerror(errno);
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

#endif

#else
struct GpioChip {};
std::string gpio_open(const char*, GpioChip*&) { return "gpio: not supported on this platform"; }
std::string gpio_open_by_label(const char*, GpioChip*&) { return "gpio: not supported on this platform"; }
void gpio_close(GpioChip*& p) { p = nullptr; }
std::string gpio_request_output(GpioChip*, int, int, const char*) { return "gpio: not supported on this platform"; }
std::string gpio_request_input(GpioChip*, int, const char*) { return "gpio: not supported on this platform"; }
std::string gpio_set(GpioChip*, int, int) { return "gpio: not supported on this platform"; }
std::string gpio_get(GpioChip*, int, int&) { return "gpio: not supported on this platform"; }
void gpio_release(GpioChip*, int) {}
#endif
