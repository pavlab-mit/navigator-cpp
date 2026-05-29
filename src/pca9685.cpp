#include "pca9685.h"
#include "i2c.h"
#include "gpio.h"

#include <unistd.h>
#include <cmath>

#define PCA9685_ADDR      0x40
#define PCA9685_MODE1     0x00
#define PCA9685_MODE2     0x01
#define PCA9685_LED0_ON_L 0x06
#define PCA9685_PRE_SCALE 0xFE

static const float EXT_CLOCK_HZ = 24576000.0f;
static bool s_pca_ok = false;

std::string pca9685_init(int i2c_fd, GpioChip* gpio, int oe_pin) {
    s_pca_ok = false;

    std::string err = i2c_set_slave(i2c_fd, PCA9685_ADDR);
    if (!err.empty()) return "pca9685_init: " + err;

    err = gpio_request_output(gpio, oe_pin, 1, "pca9685-oe");
    if (!err.empty()) return "pca9685_init: OE pin: " + err;

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

    s_pca_ok = true;
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

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, (mode1 & 0xEF) | 0x10);
    if (!err.empty()) return "pca9685_set_frequency: sleep: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_PRE_SCALE, prescale);
    if (!err.empty()) return "pca9685_set_frequency: write prescale: " + err;

    err = i2c_write_reg(i2c_fd, PCA9685_MODE1, mode1);
    if (!err.empty()) return "pca9685_set_frequency: restore: " + err;
    usleep(500);

    return "";
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
