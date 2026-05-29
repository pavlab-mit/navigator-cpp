#include "mmc5983.h"
#include "i2c.h"

#include <unistd.h>

#define MMC5983_ADDR           0x30
#define MMC5983_REG_XOUT0      0x00
#define MMC5983_REG_STATUS     0x08
#define MMC5983_REG_CTRL0      0x09
#define MMC5983_REG_CTRL1      0x0A
#define MMC5983_REG_CTRL2      0x0B
#define MMC5983_REG_PRODUCT_ID 0x2F
#define MMC5983_PRODUCT_ID     0x30

static const float SCALE = 800.0f / 131072.0f;
static bool s_mmc_ok = false;

std::string mmc5983_init(int i2c_fd) {
    s_mmc_ok = false;

    std::string err = i2c_set_slave(i2c_fd, MMC5983_ADDR);
    if (!err.empty()) return "mmc5983_init: " + err;

    uint8_t id = 0;
    err = i2c_read_reg(i2c_fd, MMC5983_REG_PRODUCT_ID, id);
    if (!err.empty()) return "mmc5983_init: read product id: " + err;
    if (id != MMC5983_PRODUCT_ID)
        return "mmc5983_init: unexpected product id 0x" + std::to_string(id) + " (expected 0x30)";

    err = i2c_write_reg(i2c_fd, MMC5983_REG_CTRL1, 0x80);
    if (!err.empty()) return "mmc5983_init: software reset: " + err;
    usleep(20000);

    err = i2c_set_slave(i2c_fd, MMC5983_ADDR);
    if (!err.empty()) return "mmc5983_init: re-select after reset: " + err;

    err = i2c_write_reg(i2c_fd, MMC5983_REG_CTRL0, 0x08);
    if (!err.empty()) return "mmc5983_init: SET command: " + err;
    usleep(1000);

    err = i2c_write_reg(i2c_fd, MMC5983_REG_CTRL2, 0x08 | 0x05);
    if (!err.empty()) return "mmc5983_init: continuous mode: " + err;

    s_mmc_ok = true;
    return "";
}

std::string mmc5983_read(int i2c_fd, float& mx, float& my, float& mz) {
    mx = my = mz = 0.0f;
    if (!s_mmc_ok) return "mmc5983_read: sensor not initialized";

    std::string err = i2c_set_slave(i2c_fd, MMC5983_ADDR);
    if (!err.empty()) return "mmc5983_read: " + err;

    uint8_t status = 0;
    for (int i = 0; i < 10; i++) {
        err = i2c_read_reg(i2c_fd, MMC5983_REG_STATUS, status);
        if (!err.empty()) return "mmc5983_read: read status: " + err;
        if (status & 0x01) break;
        usleep(1000);
    }
    if (!(status & 0x01)) return "mmc5983_read: measurement not ready after polling";

    uint8_t buf[7];
    err = i2c_read_reg_buf(i2c_fd, MMC5983_REG_XOUT0, buf, 7);
    if (!err.empty()) return "mmc5983_read: read data: " + err;

    uint32_t raw_x = ((uint32_t)buf[0] << 10) | ((uint32_t)buf[1] << 2) | ((buf[6] >> 6) & 0x03);
    uint32_t raw_y = ((uint32_t)buf[2] << 10) | ((uint32_t)buf[3] << 2) | ((buf[6] >> 4) & 0x03);
    uint32_t raw_z = ((uint32_t)buf[4] << 10) | ((uint32_t)buf[5] << 2) | ((buf[6] >> 2) & 0x03);

    mx = ((float)raw_x - 131072.0f) * SCALE;
    my = ((float)raw_y - 131072.0f) * SCALE;
    mz = ((float)raw_z - 131072.0f) * SCALE;
    return "";
}
