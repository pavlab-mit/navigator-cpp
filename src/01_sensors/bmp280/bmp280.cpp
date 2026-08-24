#include "bmp280.h"
#include "i2c.h"

#include <unistd.h>
#include <cstring>
#include <cstdio>

#define BMP280_ADDR          0x76
#define BMP280_REG_CALIB     0x88
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG    0xF5
#define BMP280_REG_DATA      0xF7

static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t  t_fine;
static bool s_bmp280_ok = false;

static std::string hex_byte(uint8_t value) {
    char out[5];
    snprintf(out, sizeof(out), "0x%02X", value);
    return out;
}

static std::string load_calibration(int fd) {
    uint8_t cal[26];
    std::string err = i2c_read_reg_buf(fd, BMP280_REG_CALIB, cal, 26);
    if (!err.empty()) return "bmp280: load_calibration: " + err;

    dig_T1 = (uint16_t)(cal[1] << 8 | cal[0]);
    dig_T2 = (int16_t)(cal[3] << 8 | cal[2]);
    dig_T3 = (int16_t)(cal[5] << 8 | cal[4]);
    dig_P1 = (uint16_t)(cal[7] << 8 | cal[6]);
    dig_P2 = (int16_t)(cal[9] << 8 | cal[8]);
    dig_P3 = (int16_t)(cal[11] << 8 | cal[10]);
    dig_P4 = (int16_t)(cal[13] << 8 | cal[12]);
    dig_P5 = (int16_t)(cal[15] << 8 | cal[14]);
    dig_P6 = (int16_t)(cal[17] << 8 | cal[16]);
    dig_P7 = (int16_t)(cal[19] << 8 | cal[18]);
    dig_P8 = (int16_t)(cal[21] << 8 | cal[20]);
    dig_P9 = (int16_t)(cal[23] << 8 | cal[22]);
    return "";
}

static float compensate_temperature(int32_t adc_T) {
    double var1 = ((double)adc_T / 16384.0 - (double)dig_T1 / 1024.0) * (double)dig_T2;
    double var2 = (((double)adc_T / 131072.0 - (double)dig_T1 / 8192.0) *
                   ((double)adc_T / 131072.0 - (double)dig_T1 / 8192.0)) * (double)dig_T3;
    t_fine = (int32_t)(var1 + var2);
    return (float)((var1 + var2) / 5120.0);
}

static float compensate_pressure(int32_t adc_P) {
    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)dig_P4) * 65536.0);
    var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)dig_P1);
    if (var1 == 0.0) return 0.0f;
    double p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
    return (float)p;
}

std::string bmp280_init(int i2c_fd) {
    s_bmp280_ok = false;
    std::string err = i2c_set_slave(i2c_fd, BMP280_ADDR);
    if (!err.empty()) return "bmp280_init: " + err;

    uint8_t chip_id = 0;
    err = i2c_read_reg(i2c_fd, 0xD0, chip_id);
    if (!err.empty()) return "bmp280_init: read chip id: " + err;
    if (chip_id != 0x58)
        return "bmp280_init: unexpected chip id " + hex_byte(chip_id) + " (expected 0x58)";

    err = load_calibration(i2c_fd);
    if (!err.empty()) return err;

    s_bmp280_ok = true;
    BARO_Config defaults;
    defaults.press_os = BARO_OS_16X;
    defaults.temp_os = BARO_OS_2X;
    defaults.odr = BARO_ODR_50HZ;
    defaults.iir = BARO_IIR_COEFF_15;
    err = bmp280_configure(i2c_fd, defaults);
    if (!err.empty()) {
        s_bmp280_ok = false;
        return "bmp280_init: defaults: " + err;
    }
    usleep(50000);
    return err;
}

std::string bmp280_configure(int i2c_fd, const BARO_Config& cfg) {
    if (!s_bmp280_ok) return "bmp280_configure: sensor not initialized";
    std::string err = i2c_set_slave(i2c_fd, BMP280_ADDR);
    if (!err.empty()) return "bmp280_configure: " + err;

    auto oversampling = [](BARO_Oversampling value) -> uint8_t {
        uint8_t code = static_cast<uint8_t>(value) + 1;
        return code > 5 ? 5 : code;
    };
    const uint8_t osrs_t = oversampling(cfg.temp_os);
    const uint8_t osrs_p = oversampling(cfg.press_os);
    const uint8_t ctrl_meas = static_cast<uint8_t>((osrs_t << 5) | (osrs_p << 2) | 0x03);

    // BMP280 exposes standby periods rather than a direct ODR. Choose the
    // nearest non-faster supported period for the shared BARO rate enum.
    static const uint8_t standby_by_odr[] = {0, 0, 0, 1, 1, 2, 3, 5};
    uint8_t odr = static_cast<uint8_t>(cfg.odr);
    if (odr > 7) odr = 7;
    uint8_t filter = static_cast<uint8_t>(cfg.iir);
    if (filter > 4) filter = 4;
    const uint8_t config = static_cast<uint8_t>((standby_by_odr[odr] << 5) | (filter << 2));

    err = i2c_write_reg(i2c_fd, BMP280_REG_CONFIG, config);
    if (!err.empty()) return "bmp280_configure: write config: " + err;
    err = i2c_write_reg(i2c_fd, BMP280_REG_CTRL_MEAS, ctrl_meas);
    if (!err.empty()) return "bmp280_configure: write ctrl_meas: " + err;
    return "";
}

std::string bmp280_read(int i2c_fd, float& pressure_kpa, float& temperature_c) {
    pressure_kpa = 0.0f;
    temperature_c = 0.0f;
    if (!s_bmp280_ok) return "bmp280_read: sensor not initialized";

    std::string err = i2c_set_slave(i2c_fd, BMP280_ADDR);
    if (!err.empty()) return "bmp280_read: " + err;

    uint8_t data[6];
    err = i2c_read_reg_buf(i2c_fd, BMP280_REG_DATA, data, 6);
    if (!err.empty()) return "bmp280_read: " + err;

    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);

    temperature_c = compensate_temperature(adc_T);
    float pressure_pa = compensate_pressure(adc_P);
    pressure_kpa = pressure_pa / 1000.0f;
    return "";
}
