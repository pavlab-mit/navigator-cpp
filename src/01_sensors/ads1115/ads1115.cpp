#include "ads1115.h"
#include "i2c.h"

#include <unistd.h>

#define ADS1115_ADDR           0x48
#define ADS1115_REG_CONVERSION 0x00
#define ADS1115_REG_CONFIG     0x01

static const uint16_t MUX_CHANNEL[4] = {0x4000, 0x5000, 0x6000, 0x7000};

// LSB sizes per gain setting
static const float GAIN_LSB[] = {
    0.0001875f,   // ±6.144V
    0.000125f,    // ±4.096V
    0.0000625f,   // ±2.048V
    0.00003125f,  // ±1.024V
    0.000015625f, // ±0.512V
    0.0000078125f // ±0.256V
};

static bool s_ads_ok = false;
static ADC_Gain s_gain = ADC_GAIN_4096MV;
static ADC_DataRate s_rate = ADC_RATE_860SPS;

static uint16_t build_config(int channel) {
    uint16_t cfg = 0x8000;  // OS: start conversion
    cfg |= MUX_CHANNEL[channel];
    cfg |= ((uint16_t)s_gain << 9);
    cfg |= 0x0100;  // Single-shot
    cfg |= ((uint16_t)s_rate << 5);
    cfg |= 0x0003;  // Comparator defaults
    return cfg;
}

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

std::string ads1115_configure(int i2c_fd, const ADS1115_Config& cfg) {
    if (!s_ads_ok) return "ads1115_configure: sensor not initialized";
    s_gain = cfg.gain;
    s_rate = cfg.data_rate;
    return "";
}

std::string ads1115_read(int i2c_fd, int channel, float& volts) {
    volts = 0.0f;
    if (!s_ads_ok) return "ads1115_read: sensor not initialized";
    if (channel < 0 || channel > 3)
        return "ads1115_read: invalid channel " + std::to_string(channel);

    std::string err = i2c_set_slave(i2c_fd, ADS1115_ADDR);
    if (!err.empty()) return "ads1115_read: " + err;

    uint16_t config = build_config(channel);
    uint8_t cfg_data[2] = {(uint8_t)((config >> 8) & 0xFF), (uint8_t)(config & 0xFF)};
    err = i2c_write_reg_buf(i2c_fd, ADS1115_REG_CONFIG, cfg_data, 2);
    if (!err.empty()) return "ads1115_read: write config: " + err;

    usleep(2000);

    uint8_t buf[2];
    err = i2c_read_reg_buf(i2c_fd, ADS1115_REG_CONVERSION, buf, 2);
    if (!err.empty()) return "ads1115_read: read conversion: " + err;

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    int gain_idx = (int)s_gain;
    if (gain_idx < 0 || gain_idx > 5) gain_idx = 1;
    volts = raw * GAIN_LSB[gain_idx];
    return "";
}
