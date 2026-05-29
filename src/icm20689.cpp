#include "icm20689.h"
#include "spi.h"

#include <unistd.h>
#include <cmath>

#define ICM20689_WHO_AM_I     0x75
#define ICM20689_PWR_MGMT_1   0x6B
#define ICM20689_GYRO_CONFIG  0x1B
#define ICM20689_ACCEL_CONFIG 0x1C
#define ICM20689_ACCEL_XOUT_H 0x3B
#define ICM20689_GYRO_XOUT_H  0x43

static const float ACCEL_SCALE = 9.80665f / 16384.0f;
static const float GYRO_SCALE  = (float)(M_PI / 180.0) / 131.0f;
static bool s_icm_ok = false;

static int16_t to_int16(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

std::string icm20689_init(int spi_fd) {
    s_icm_ok = false;

    uint8_t who = 0;
    std::string err = spi_read_reg(spi_fd, ICM20689_WHO_AM_I, &who, 1);
    if (!err.empty()) return "icm20689_init: read WHO_AM_I: " + err;
    if (who != 0x98)
        return "icm20689_init: unexpected WHO_AM_I 0x" + std::to_string(who) + " (expected 0x98)";

    err = spi_write_reg(spi_fd, ICM20689_PWR_MGMT_1, 0x80);
    if (!err.empty()) return "icm20689_init: reset: " + err;
    usleep(100000);

    err = spi_write_reg(spi_fd, ICM20689_PWR_MGMT_1, 0x01);
    if (!err.empty()) return "icm20689_init: wake: " + err;
    usleep(10000);

    err = spi_write_reg(spi_fd, ICM20689_GYRO_CONFIG, 0x00);
    if (!err.empty()) return "icm20689_init: gyro config: " + err;

    err = spi_write_reg(spi_fd, ICM20689_ACCEL_CONFIG, 0x00);
    if (!err.empty()) return "icm20689_init: accel config: " + err;

    s_icm_ok = true;
    return "";
}

std::string icm20689_read_accel(int spi_fd, float& ax, float& ay, float& az) {
    ax = ay = az = 0.0f;
    if (!s_icm_ok) return "icm20689_read_accel: sensor not initialized";

    uint8_t buf[6];
    std::string err = spi_read_reg(spi_fd, ICM20689_ACCEL_XOUT_H, buf, 6);
    if (!err.empty()) return "icm20689_read_accel: " + err;

    ax = to_int16(buf[0], buf[1]) * ACCEL_SCALE;
    ay = to_int16(buf[2], buf[3]) * ACCEL_SCALE;
    az = to_int16(buf[4], buf[5]) * ACCEL_SCALE;
    return "";
}

std::string icm20689_read_gyro(int spi_fd, float& gx, float& gy, float& gz) {
    gx = gy = gz = 0.0f;
    if (!s_icm_ok) return "icm20689_read_gyro: sensor not initialized";

    uint8_t buf[6];
    std::string err = spi_read_reg(spi_fd, ICM20689_GYRO_XOUT_H, buf, 6);
    if (!err.empty()) return "icm20689_read_gyro: " + err;

    gx = to_int16(buf[0], buf[1]) * GYRO_SCALE;
    gy = to_int16(buf[2], buf[3]) * GYRO_SCALE;
    gz = to_int16(buf[4], buf[5]) * GYRO_SCALE;
    return "";
}
