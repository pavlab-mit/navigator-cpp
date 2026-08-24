#include "icm20689.h"
#include "spi.h"

#include <unistd.h>
#include <cmath>
#include <cstdio>

#define ICM20689_WHO_AM_I       0x75
#define ICM20689_WHO_AM_I_VAL   0x98
#define ICM20602_WHO_AM_I_VAL   0x12
#define ICM20689_PWR_MGMT_1     0x6B
#define ICM20689_SMPLRT_DIV     0x19
#define ICM20689_CONFIG         0x1A
#define ICM20689_GYRO_CONFIG    0x1B
#define ICM20689_ACCEL_CONFIG   0x1C
#define ICM20689_ACCEL_CONFIG2  0x1D
#define ICM20689_ACCEL_XOUT_H   0x3B
#define ICM20689_GYRO_XOUT_H    0x43

static float s_accel_scale = 9.80665f / 16384.0f;
static float s_gyro_scale  = (float)(M_PI / 180.0) / 131.0f;
static bool s_icm_ok = false;

static std::string hex_byte(uint8_t value) {
    char out[5];
    snprintf(out, sizeof(out), "0x%02X", value);
    return out;
}

static void update_scales(ICM_AccelRange ar, ICM_GyroRange gr) {
    switch (ar) {
        case ICM_ACCEL_2G:  s_accel_scale = 9.80665f / 16384.0f; break;
        case ICM_ACCEL_4G:  s_accel_scale = 9.80665f / 8192.0f;  break;
        case ICM_ACCEL_8G:  s_accel_scale = 9.80665f / 4096.0f;  break;
        case ICM_ACCEL_16G: s_accel_scale = 9.80665f / 2048.0f;  break;
    }
    switch (gr) {
        case ICM_GYRO_250DPS:  s_gyro_scale = (float)(M_PI / 180.0) / 131.0f;  break;
        case ICM_GYRO_500DPS:  s_gyro_scale = (float)(M_PI / 180.0) / 65.5f;   break;
        case ICM_GYRO_1000DPS: s_gyro_scale = (float)(M_PI / 180.0) / 32.8f;   break;
        case ICM_GYRO_2000DPS: s_gyro_scale = (float)(M_PI / 180.0) / 16.4f;   break;
    }
}

static int16_t to_int16(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

std::string icm20689_init(SpiDevice& spi_fd) {
    s_icm_ok = false;

    uint8_t who = 0;
    std::string err = spi_read_reg(spi_fd, ICM20689_WHO_AM_I, &who, 1);
    if (!err.empty()) return "icm20689_init: read WHO_AM_I: " + err;
    if (who != ICM20689_WHO_AM_I_VAL && who != ICM20602_WHO_AM_I_VAL)
        return "icm20689_init: unexpected WHO_AM_I " + hex_byte(who)
               + " (expected 0x98 or 0x12)";

    err = spi_write_reg(spi_fd, ICM20689_PWR_MGMT_1, 0x80);
    if (!err.empty()) return "icm20689_init: reset: " + err;
    usleep(100000);

    err = spi_write_reg(spi_fd, ICM20689_PWR_MGMT_1, 0x01);
    if (!err.empty()) return "icm20689_init: wake: " + err;
    usleep(10000);

    // Apply defaults
    ICM_Config defaults;
    update_scales(defaults.accel_range, defaults.gyro_range);

    err = spi_write_reg(spi_fd, ICM20689_GYRO_CONFIG, defaults.gyro_range);
    if (!err.empty()) return "icm20689_init: gyro config: " + err;

    err = spi_write_reg(spi_fd, ICM20689_ACCEL_CONFIG, defaults.accel_range);
    if (!err.empty()) return "icm20689_init: accel config: " + err;

    s_icm_ok = true;
    return "";
}

std::string icm20689_configure(SpiDevice& spi_fd, const ICM_Config& cfg) {
    if (!s_icm_ok) return "icm20689_configure: sensor not initialized";

    std::string err;

    err = spi_write_reg(spi_fd, ICM20689_ACCEL_CONFIG, cfg.accel_range);
    if (!err.empty()) return "icm20689_configure: accel range: " + err;

    err = spi_write_reg(spi_fd, ICM20689_GYRO_CONFIG, cfg.gyro_range);
    if (!err.empty()) return "icm20689_configure: gyro range: " + err;

    err = spi_write_reg(spi_fd, ICM20689_SMPLRT_DIV, cfg.sample_rate_div);
    if (!err.empty()) return "icm20689_configure: sample rate div: " + err;

    if (cfg.gyro_dlpf != ICM_DLPF_OFF) {
        err = spi_write_reg(spi_fd, ICM20689_CONFIG, cfg.gyro_dlpf);
        if (!err.empty()) return "icm20689_configure: gyro dlpf: " + err;
    }

    if (cfg.accel_dlpf != ICM_DLPF_OFF) {
        err = spi_write_reg(spi_fd, ICM20689_ACCEL_CONFIG2, cfg.accel_dlpf);
        if (!err.empty()) return "icm20689_configure: accel dlpf: " + err;
    }

    update_scales(cfg.accel_range, cfg.gyro_range);
    return "";
}

std::string icm20689_read_accel(SpiDevice& spi_fd, float& ax, float& ay, float& az) {
    ax = ay = az = 0.0f;
    if (!s_icm_ok) return "icm20689_read_accel: sensor not initialized";

    uint8_t buf[6];
    std::string err = spi_read_reg(spi_fd, ICM20689_ACCEL_XOUT_H, buf, 6);
    if (!err.empty()) return "icm20689_read_accel: " + err;

    ax = to_int16(buf[0], buf[1]) * s_accel_scale;
    ay = to_int16(buf[2], buf[3]) * s_accel_scale;
    az = to_int16(buf[4], buf[5]) * s_accel_scale;
    return "";
}

std::string icm20689_read_gyro(SpiDevice& spi_fd, float& gx, float& gy, float& gz) {
    gx = gy = gz = 0.0f;
    if (!s_icm_ok) return "icm20689_read_gyro: sensor not initialized";

    uint8_t buf[6];
    std::string err = spi_read_reg(spi_fd, ICM20689_GYRO_XOUT_H, buf, 6);
    if (!err.empty()) return "icm20689_read_gyro: " + err;

    gx = to_int16(buf[0], buf[1]) * s_gyro_scale;
    gy = to_int16(buf[2], buf[3]) * s_gyro_scale;
    gz = to_int16(buf[4], buf[5]) * s_gyro_scale;
    return "";
}
