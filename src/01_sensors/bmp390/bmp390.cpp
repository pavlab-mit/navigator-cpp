#include "bmp390.h"
#include "i2c.h"
#include "bmp3.h"

#include <unistd.h>
#include <cstring>

static int s_i2c_fd = -1;

static BMP3_INTF_RET_TYPE bmp3_i2c_read(uint8_t reg_addr, uint8_t* reg_data,
                                          uint32_t len, void* intf_ptr) {
    (void)intf_ptr;
    std::string err = i2c_set_slave(s_i2c_fd, 0x76);
    if (!err.empty()) return BMP3_E_COMM_FAIL;
    err = i2c_read_reg_buf(s_i2c_fd, reg_addr, reg_data, (int)len);
    return err.empty() ? BMP3_OK : BMP3_E_COMM_FAIL;
}

static BMP3_INTF_RET_TYPE bmp3_i2c_write(uint8_t reg_addr, const uint8_t* reg_data,
                                           uint32_t len, void* intf_ptr) {
    (void)intf_ptr;
    std::string err = i2c_set_slave(s_i2c_fd, 0x76);
    if (!err.empty()) return BMP3_E_COMM_FAIL;
    err = i2c_write_reg_buf(s_i2c_fd, reg_addr, reg_data, (int)len);
    return err.empty() ? BMP3_OK : BMP3_E_COMM_FAIL;
}

static void bmp3_delay_us(uint32_t period, void* intf_ptr) {
    (void)intf_ptr;
    usleep(period);
}

static struct bmp3_dev s_dev;
static struct bmp3_settings s_settings;
static bool s_bmp390_ok = false;

std::string bmp390_init(int i2c_fd) {
    s_bmp390_ok = false;
    s_i2c_fd = i2c_fd;

    std::string err = i2c_set_slave(i2c_fd, 0x76);
    if (!err.empty()) return "bmp390_init: " + err;

    memset(&s_dev, 0, sizeof(s_dev));
    memset(&s_settings, 0, sizeof(s_settings));

    s_dev.intf = BMP3_I2C_INTF;
    s_dev.read = bmp3_i2c_read;
    s_dev.write = bmp3_i2c_write;
    s_dev.delay_us = bmp3_delay_us;
    s_dev.intf_ptr = &s_dev;

    int8_t rc = bmp3_init(&s_dev);
    if (rc != BMP3_OK)
        return "bmp390_init: bmp3_init failed, rc=" + std::to_string(rc);

    // Apply defaults
    BARO_Config defaults;
    return bmp390_configure(i2c_fd, defaults);
}

std::string bmp390_configure(int i2c_fd, const BARO_Config& cfg) {
    s_i2c_fd = i2c_fd;

    uint32_t settings_sel = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN |
                            BMP3_SEL_PRESS_OS | BMP3_SEL_TEMP_OS |
                            BMP3_SEL_ODR;

    s_settings.press_en = BMP3_ENABLE;
    s_settings.temp_en = BMP3_ENABLE;
    s_settings.odr_filter.press_os = (uint8_t)cfg.press_os;
    s_settings.odr_filter.temp_os = (uint8_t)cfg.temp_os;
    s_settings.odr_filter.odr = (uint8_t)cfg.odr;

    if (cfg.iir != BARO_IIR_OFF) {
        settings_sel |= BMP3_SEL_IIR_FILTER;
        s_settings.odr_filter.iir_filter = (uint8_t)cfg.iir;
    }

    int8_t rc = bmp3_set_sensor_settings(settings_sel, &s_settings, &s_dev);
    if (rc != BMP3_OK)
        return "bmp390_configure: bmp3_set_sensor_settings failed, rc=" + std::to_string(rc);

    s_settings.op_mode = BMP3_MODE_NORMAL;
    rc = bmp3_set_op_mode(&s_settings, &s_dev);
    if (rc != BMP3_OK)
        return "bmp390_configure: bmp3_set_op_mode failed, rc=" + std::to_string(rc);

    s_bmp390_ok = true;
    return "";
}

std::string bmp390_read(int i2c_fd, float& pressure_kpa, float& temperature_c) {
    pressure_kpa = 0.0f;
    temperature_c = 0.0f;
    if (!s_bmp390_ok) return "bmp390_read: sensor not initialized";

    s_i2c_fd = i2c_fd;
    std::string err = i2c_set_slave(i2c_fd, 0x76);
    if (!err.empty()) return "bmp390_read: " + err;

    uint8_t reg[BMP3_LEN_P_T_DATA] = {0};
    int8_t rc = bmp3_get_regs(BMP3_REG_DATA, reg, BMP3_LEN_P_T_DATA, &s_dev);
    if (rc != BMP3_OK)
        return "bmp390_read: bmp3_get_regs failed, rc=" + std::to_string(rc);

    double up = (double)((uint32_t)reg[0] | ((uint32_t)reg[1] << 8) | ((uint32_t)reg[2] << 16));
    double ut = (double)((uint32_t)reg[3] | ((uint32_t)reg[4] << 8) | ((uint32_t)reg[5] << 16));

    // Datasheet App. A 8.5/8.6 - same math as bmp3.c compensate_*(), without the
    // 300-1250 hPa clamp. That clamp returns BMP3_W_MIN_PRES, which the old code
    // treated as an error, so a hard vacuum reported 0.00 kPa / 0.00 C instead of
    // an extrapolated value. Outside 30-125 kPa the result is uncalibrated:
    // monotonic and fine for leak-down trends, no accuracy guarantee.
    const struct bmp3_quantized_calib_data* c = &s_dev.calib_data.quantized_calib_data;

    double d1 = ut - c->par_t1;
    double t_lin = (d1 * c->par_t2) + (d1 * d1) * c->par_t3;

    double o1 = c->par_p5 + c->par_p6 * t_lin + c->par_p7 * t_lin * t_lin
                + c->par_p8 * t_lin * t_lin * t_lin;
    double o2 = up * (c->par_p1 + c->par_p2 * t_lin + c->par_p3 * t_lin * t_lin
                + c->par_p4 * t_lin * t_lin * t_lin);
    double o3 = up * up * (c->par_p9 + c->par_p10 * t_lin) + up * up * up * c->par_p11;

    temperature_c = (float)t_lin;
    pressure_kpa = (float)((o1 + o2 + o3) / 1000.0);
    return "";
}
