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

// MODE1 bit 7 (RESTART) is write-1-to-clear and is SET by the chip
// when SLEEP is written while PWM channels are running; after wake,
// all outputs stay OFF until software writes a 1 back to it. The
// chip is powered continuously, so a previous process routinely
// leaves channels running and the next init used to trip this on
// alternating launches (ESC panic, bench-confirmed 2026-08-14 --
// see moos-ivp-blueboat docs/rc_controllers.md section 8.3). Rules:
// stop the channels before any SLEEP write, never blindly write a
// stale bit 7 back in a read-modify-write, and after any wake give
// the oscillator 500 us then clear a pending RESTART explicitly.
#define PCA9685_MODE1_RESTART 0x80

static const float EXT_CLOCK_HZ = 24576000.0f;
static bool s_pca_ok = false;
static float s_freq_hz = 50.0f;  // Track current frequency for us conversion

std::string pca9685_init(int i2c_fd, GpioChip* gpio, int oe_pin) {
    s_pca_ok = false;

    std::string err = i2c_set_slave(i2c_fd, PCA9685_ADDR);
    if (!err.empty()) return "pca9685_init: " + err;

    err = gpio_request_output(gpio, oe_pin, 1, "pca9685-oe");
    if (!err.empty()) return "pca9685_init: OE pin: " + err;

    // Stop all PWM channels BEFORE writing SLEEP, so the sleep below
    // can never catch a running counter and set RESTART-pending (the
    // alternating-launch trap; see note at PCA9685_MODE1_RESTART).
    // OE is already high here, so nothing downstream sees a glitch.
    err = i2c_write_reg(i2c_fd, PCA9685_ALL_LED_OFF_H, 0x10);
    if (!err.empty()) return "pca9685_init: all-off: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, 0x10);
    if (!err.empty()) return "pca9685_init: sleep: " + err;
    usleep(1000);

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, 0x50);
    if (!err.empty()) return "pca9685_init: ext clock: " + err;
    usleep(1000);

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, 0x40);
    if (!err.empty()) return "pca9685_init: wake: " + err;
    usleep(1000);

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, 0x60);
    if (!err.empty()) return "pca9685_init: auto-increment: " + err;

    // Clear any RESTART still pending from a previous process (e.g.
    // one that slept the chip and died before waking it). Write-1-to-
    // clear; all channels are off, so nothing restarts. The >=500us
    // post-wake oscillator delay is covered by the usleep above.
    uint8_t mode1 = 0;
    err = i2c_read_reg(i2c_fd, PCA9685_MODE1, mode1);
    if (!err.empty()) return "pca9685_init: read mode1: " + err;
    if (mode1 & PCA9685_MODE1_RESTART) {
        err = i2c_write_reg(i2c_fd, PCA9685_MODE1, mode1);
        if (!err.empty()) return "pca9685_init: clear restart: " + err;
    }

    s_pca_ok = true;
    return "";
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

    // Never write a stale RESTART bit back: bit 7 is write-1-to-clear
    // with order-dependent side effects (see PCA9685_MODE1_RESTART).
    mode1 &= (uint8_t)~PCA9685_MODE1_RESTART;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, (mode1 & 0xEF) | 0x10);
    if (!err.empty()) return "pca9685_set_frequency: sleep: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_PRE_SCALE, prescale);
    if (!err.empty()) return "pca9685_set_frequency: write prescale: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, mode1);
    if (!err.empty()) return "pca9685_set_frequency: restore: " + err;
    usleep(500);

    // If the sleep above caught running channels, RESTART is now
    // pending and the outputs are held off; write bit 7 = 1 (the
    // 500us oscillator delay has elapsed) so they resume with their
    // pre-sleep values. A frequency change on a running chip is then
    // glitch-free instead of silently killing the outputs.
    uint8_t m2 = 0;
    err = i2c_read_reg(i2c_fd, PCA9685_MODE1, m2);
    if (!err.empty()) return "pca9685_set_frequency: read restart: " + err;
    if (m2 & PCA9685_MODE1_RESTART) {
        err = i2c_write_reg(i2c_fd, PCA9685_MODE1, m2);
        if (!err.empty()) return "pca9685_set_frequency: restart: " + err;
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
