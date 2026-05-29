#include "ak09915.h"
#include "i2c.h"

#include <unistd.h>

#define AK09915_ADDR         0x0C
#define AK09915_REG_ST1      0x10
#define AK09915_REG_HXL      0x11
#define AK09915_REG_CNTL2    0x31
#define AK09915_REG_CNTL3    0x32
#define AK09915_MODE_CONT200 0x0A

static const float SCALE = 0.15f;
static bool s_ak_ok = false;

static int16_t to_int16_le(uint8_t lo, uint8_t hi) {
    return (int16_t)((hi << 8) | lo);
}

std::string ak09915_init(int i2c_fd) {
    s_ak_ok = false;

    std::string err = i2c_set_slave(i2c_fd, AK09915_ADDR);
    if (!err.empty()) return "ak09915_init: " + err;

    err = i2c_write_reg(i2c_fd, AK09915_REG_CNTL3, 0x01);
    if (!err.empty()) return "ak09915_init: soft reset: " + err;
    usleep(1000);

    err = i2c_write_reg(i2c_fd, AK09915_REG_CNTL2, 0x00);
    if (!err.empty()) return "ak09915_init: power down: " + err;
    usleep(100);

    err = i2c_write_reg(i2c_fd, AK09915_REG_CNTL2, AK09915_MODE_CONT200);
    if (!err.empty()) return "ak09915_init: set mode: " + err;

    s_ak_ok = true;
    return "";
}

std::string ak09915_read(int i2c_fd, float& mx, float& my, float& mz) {
    mx = my = mz = 0.0f;
    if (!s_ak_ok) return "ak09915_read: sensor not initialized";

    std::string err = i2c_set_slave(i2c_fd, AK09915_ADDR);
    if (!err.empty()) return "ak09915_read: " + err;

    uint8_t st1 = 0;
    for (int i = 0; i < 10; i++) {
        err = i2c_read_reg(i2c_fd, AK09915_REG_ST1, st1);
        if (!err.empty()) return "ak09915_read: read ST1: " + err;
        if (st1 & 0x01) break;
        usleep(500);
    }
    if (!(st1 & 0x01)) return "ak09915_read: data not ready after polling";

    uint8_t buf[8];
    err = i2c_read_reg_buf(i2c_fd, AK09915_REG_HXL, buf, 8);
    if (!err.empty()) return "ak09915_read: read data: " + err;

    uint8_t st2 = buf[7];
    if (st2 & 0x08) return "ak09915_read: magnetic sensor overflow";
    if (st2 & 0x04) return "ak09915_read: invalid data flag set";

    mx = to_int16_le(buf[0], buf[1]) * SCALE;
    my = to_int16_le(buf[2], buf[3]) * SCALE;
    mz = to_int16_le(buf[4], buf[5]) * SCALE;
    return "";
}
