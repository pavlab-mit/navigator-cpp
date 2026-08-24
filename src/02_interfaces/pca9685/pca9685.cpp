#include "pca9685.h"
#include "i2c.h"
#include "gpio.h"

#include <unistd.h>
#include <cmath>

#define PCA9685_ADDR      0x40
#define PCA9685_MODE1     0x00
#define PCA9685_MODE2     0x01
#define PCA9685_LED0_ON_L 0x06
#define PCA9685_ALL_LED_OFF_H 0xFD
#define PCA9685_PRE_SCALE 0xFE

#define PCA9685_MODE1_RESTART 0x80
#define PCA9685_MODE1_EXTCLK  0x40
#define PCA9685_MODE1_AI      0x20
#define PCA9685_MODE1_SLEEP   0x10
#define PCA9685_FULL_OFF      0x10
#define PCA9685_MODE2_TOTEM_POLE 0x04

static const float EXT_CLOCK_HZ = 24576000.0f;
static const float DEFAULT_FREQUENCY_HZ = 50.0f;
static bool s_pca_ok = false;
static float s_freq_hz = 50.0f;  // Track current frequency for us conversion

static std::string set_all_channels_full_off(int i2c_fd) {
    // ALL_LED_OFF is the PCA9685's documented orderly-stop mechanism. It
    // prevents a subsequent SLEEP write from arming the RESTART trap.
    return i2c_write_reg(i2c_fd, PCA9685_ALL_LED_OFF_H, PCA9685_FULL_OFF);
}

static std::string clear_all_channels_full_off(int i2c_fd) {
    return i2c_write_reg(i2c_fd, PCA9685_ALL_LED_OFF_H, 0x00);
}

static std::string initialize_channel_registers_off(int i2c_fd) {
    // An orderly all-off invalidates the per-channel PWM state. Initialize
    // every channel explicitly before releasing the global override.
    const uint8_t off[4] = {0x00, 0x00, 0x00, PCA9685_FULL_OFF};
    for (int channel = 0; channel < 16; ++channel) {
        const uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
        std::string err = i2c_write_reg_buf(i2c_fd, reg, off, 4);
        if (!err.empty())
            return "initialize channel " + std::to_string(channel) + ": " + err;
    }
    return "";
}

std::string pca9685_init(int i2c_fd, GpioChip* gpio, int oe_pin) {
    s_pca_ok = false;

    std::string err = i2c_set_slave(i2c_fd, PCA9685_ADDR);
    if (!err.empty()) return "pca9685_init: " + err;

    err = gpio_request_output(gpio, oe_pin, 1, "pca9685-oe");
    if (!err.empty()) return "pca9685_init: OE pin: " + err;
    int oe_value = -1;
    err = gpio_get(gpio, oe_pin, oe_value);
    if (!err.empty() || oe_value != 1)
        return "pca9685_init: OE did not enter safe disabled state"
               + (err.empty() ? std::string() : ": " + err);

    err = set_all_channels_full_off(i2c_fd);
    if (!err.empty()) return "pca9685_init: orderly all-off: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, PCA9685_MODE1_SLEEP);
    if (!err.empty()) return "pca9685_init: sleep: " + err;
    usleep(1000);

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1,
                        PCA9685_MODE1_SLEEP | PCA9685_MODE1_EXTCLK);
    if (!err.empty()) return "pca9685_init: ext clock: " + err;
    usleep(1000);

    const uint8_t default_prescale = static_cast<uint8_t>(
        roundf(EXT_CLOCK_HZ / (4096.0f * DEFAULT_FREQUENCY_HZ)) - 1.0f);
    err = i2c_write_reg(i2c_fd, PCA9685_PRE_SCALE, default_prescale);
    if (!err.empty()) return "pca9685_init: default prescale: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, PCA9685_MODE1_EXTCLK);
    if (!err.empty()) return "pca9685_init: wake: " + err;
    usleep(1000);

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1,
                        PCA9685_MODE1_EXTCLK | PCA9685_MODE1_AI);
    if (!err.empty()) return "pca9685_init: auto-increment: " + err;

    uint8_t mode1 = 0;
    err = i2c_read_reg(i2c_fd, PCA9685_MODE1, mode1);
    if (!err.empty()) return "pca9685_init: read restart state: " + err;
    if (mode1 & PCA9685_MODE1_RESTART) {
        // SLEEP has been clear for more than the datasheet's required 500 us.
        err = i2c_write_reg(i2c_fd, PCA9685_MODE1, mode1);
        if (!err.empty()) return "pca9685_init: restart channels: " + err;
    }

    err = initialize_channel_registers_off(i2c_fd);
    if (!err.empty()) return "pca9685_init: " + err;

    // Establish output polarity/driver/disabled behavior explicitly instead
    // of inheriting register state from a previous process.
    err = i2c_write_reg(i2c_fd, PCA9685_MODE2, PCA9685_MODE2_TOTEM_POLE);
    if (!err.empty()) return "pca9685_init: mode2 defaults: " + err;
    err = clear_all_channels_full_off(i2c_fd);
    if (!err.empty()) return "pca9685_init: clear all-off: " + err;

    s_pca_ok = true;
    s_freq_hz = DEFAULT_FREQUENCY_HZ;
    return "";
}

std::string pca9685_shutdown(int i2c_fd, GpioChip* gpio, int oe_pin) {
    std::string first_error;

    // Physical output disable is the highest-priority shutdown action.
    if (gpio) {
        std::string err = gpio_set(gpio, oe_pin, 1);
        if (!err.empty()) first_error = "pca9685_shutdown: disable OE: " + err;
    }
    if (i2c_fd >= 0) {
        std::string err = i2c_set_slave(i2c_fd, PCA9685_ADDR);
        if (err.empty()) err = set_all_channels_full_off(i2c_fd);
        if (err.empty()) err = initialize_channel_registers_off(i2c_fd);
        for (int channel = 0; channel < 16 && err.empty(); ++channel) {
            uint8_t off_h = 0;
            err = i2c_read_reg(i2c_fd, PCA9685_LED0_ON_L + 4 * channel + 3, off_h);
            if (err.empty() && !(off_h & PCA9685_FULL_OFF))
                err = "channel " + std::to_string(channel) + " full-off did not latch";
        }
        if (first_error.empty() && !err.empty())
            first_error = "pca9685_shutdown: orderly all-off: " + err;
    }

    s_pca_ok = false;
    s_freq_hz = 50.0f;
    return first_error;
}

std::string pca9685_configure(int i2c_fd, const PCA9685_Config& cfg) {
    if (!s_pca_ok) return "pca9685_configure: not initialized";

    std::string err = i2c_set_slave(i2c_fd, PCA9685_ADDR);
    if (!err.empty()) return "pca9685_configure: " + err;

    // Frequency
    err = pca9685_set_frequency(i2c_fd, cfg.frequency_hz);
    if (!err.empty()) return err;

    // MODE2: output mode
    uint8_t mode2 = 0x04;  // Default: totem pole, no invert
    if (cfg.open_drain) mode2 &= ~0x04;
    if (cfg.invert) mode2 |= 0x10;
    err = i2c_write_reg(i2c_fd, PCA9685_MODE2, mode2);
    if (!err.empty()) return "pca9685_configure: mode2: " + err;

    return "";
}

std::string pca9685_enable(GpioChip* gpio, int oe_pin, bool enable) {
    if (!s_pca_ok) return "pca9685_enable: not initialized";
    return gpio_set(gpio, oe_pin, enable ? 0 : 1);
}

std::string pca9685_set_frequency(int i2c_fd, float freq_hz) {
    if (!s_pca_ok) return "pca9685_set_frequency: not initialized";
    if (freq_hz < 24.0f || freq_hz > 1526.0f)
        return "pca9685_set_frequency: freq out of range (24-1526 Hz)";

    std::string err = i2c_set_slave(i2c_fd, PCA9685_ADDR);
    if (!err.empty()) return "pca9685_set_frequency: " + err;

    uint8_t prescale = (uint8_t)(roundf(EXT_CLOCK_HZ / (4096.0f * freq_hz)) - 1);

    uint8_t mode1 = 0;
    err = i2c_read_reg(i2c_fd, PCA9685_MODE1, mode1);
    if (!err.empty()) return "pca9685_set_frequency: read mode1: " + err;

    // Never replay an inherited RESTART bit during the sleep transition.
    mode1 &= (uint8_t)~PCA9685_MODE1_RESTART;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, (mode1 & 0xEF) | 0x10);
    if (!err.empty()) return "pca9685_set_frequency: sleep: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_PRE_SCALE, prescale);
    if (!err.empty()) return "pca9685_set_frequency: write prescale: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, mode1);
    if (!err.empty()) return "pca9685_set_frequency: restore: " + err;
    usleep(500);

    uint8_t restarted_mode1 = 0;
    err = i2c_read_reg(i2c_fd, PCA9685_MODE1, restarted_mode1);
    if (!err.empty()) return "pca9685_set_frequency: read restart state: " + err;
    if (restarted_mode1 & PCA9685_MODE1_RESTART) {
        err = i2c_write_reg(i2c_fd, PCA9685_MODE1, restarted_mode1);
        if (!err.empty()) return "pca9685_set_frequency: restart channels: " + err;
    }
    s_freq_hz = freq_hz;

    return "";
}

std::string pca9685_set_pulse_us(int i2c_fd, int channel, float pulse_us) {
    if (!s_pca_ok) return "pca9685_set_pulse_us: not initialized";
    if (channel < 0 || channel > 15)
        return "pca9685_set_pulse_us: invalid channel " + std::to_string(channel);

    // Convert microseconds to duty cycle
    // Period in us = 1e6 / freq_hz
    // Duty = pulse_us / period_us
    float period_us = 1e6f / s_freq_hz;
    float duty = pulse_us / period_us;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    uint16_t off = (uint16_t)(duty * 4095.0f);
    return pca9685_set_raw(i2c_fd, channel, 0, off);
}

std::string pca9685_set_duty(int i2c_fd, int channel, float duty) {
    if (!s_pca_ok) return "pca9685_set_duty: not initialized";
    if (channel < 0 || channel > 15)
        return "pca9685_set_duty: invalid channel " + std::to_string(channel);
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    uint16_t off = (uint16_t)(duty * 4095.0f);
    return pca9685_set_raw(i2c_fd, channel, 0, off);
}

std::string pca9685_set_raw(int i2c_fd, int channel, uint16_t on, uint16_t off) {
    if (!s_pca_ok) return "pca9685_set_raw: not initialized";
    if (channel < 0 || channel > 15)
        return "pca9685_set_raw: invalid channel " + std::to_string(channel);

    std::string err = i2c_set_slave(i2c_fd, PCA9685_ADDR);
    if (!err.empty()) return "pca9685_set_raw: " + err;

    uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
    uint8_t data[4] = {
        (uint8_t)(on & 0xFF),
        (uint8_t)((on >> 8) & 0x0F),
        (uint8_t)(off & 0xFF),
        (uint8_t)((off >> 8) & 0x0F)
    };
    err = i2c_write_reg_buf(i2c_fd, reg, data, 4);
    if (!err.empty()) return "pca9685_set_raw: write channel: " + err;
    return "";
}
