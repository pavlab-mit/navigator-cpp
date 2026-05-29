#include "ads1115.h"
#include "i2c.h"

#include <unistd.h>

#define ADS1115_ADDR           0x48
#define ADS1115_REG_CONVERSION 0x00
#define ADS1115_REG_CONFIG     0x01

static const uint16_t BASE_CONFIG = 0x8183 | (0x01 << 9) | (0x07 << 5);
static const uint16_t MUX_CHANNEL[4] = {0x4000, 0x5000, 0x6000, 0x7000};
static const float LSB_VOLTS = 0.000125f;
static bool s_ads_ok = false;

std::string ads1115_init(int i2c_fd) {
    s_ads_ok = false;

    std::string err = i2c_set_slave(i2c_fd, ADS1115_ADDR);
    if (!err.empty()) return "ads1115_init: " + err;

    uint8_t buf[2];
    err = i2c_read_reg_buf(i2c_fd, ADS1115_REG_CONFIG, buf, 2);
    if (!err.empty()) return "ads1115_init: read config: " + err;

    s_ads_ok = true;
    return "";
}

std::string ads1115_read(int i2c_fd, int channel, float& volts) {
    volts = 0.0f;
    if (!s_ads_ok) return "ads1115_read: sensor not initialized";
    if (channel < 0 || channel > 3)
        return "ads1115_read: invalid channel " + std::to_string(channel);

    std::string err = i2c_set_slave(i2c_fd, ADS1115_ADDR);
    if (!err.empty()) return "ads1115_read: " + err;

    uint16_t config = BASE_CONFIG | MUX_CHANNEL[channel];
    uint8_t cfg_data[2] = {(uint8_t)((config >> 8) & 0xFF), (uint8_t)(config & 0xFF)};
    err = i2c_write_reg_buf(i2c_fd, ADS1115_REG_CONFIG, cfg_data, 2);
    if (!err.empty()) return "ads1115_read: write config: " + err;

    usleep(2000);

    uint8_t buf[2];
    err = i2c_read_reg_buf(i2c_fd, ADS1115_REG_CONVERSION, buf, 2);
    if (!err.empty()) return "ads1115_read: read conversion: " + err;

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    volts = raw * LSB_VOLTS;
    return "";
}
