#include "mmc5983.h"
#include "spi.h"

#include <unistd.h>
#include <cstring>

#define MMC5983_REG_XOUT0      0x00
#define MMC5983_REG_STATUS     0x08
#define MMC5983_REG_CTRL0      0x09
#define MMC5983_REG_CTRL1      0x0A
#define MMC5983_REG_CTRL2      0x0B
#define MMC5983_REG_PRODUCT_ID 0x2F
#define MMC5983_PRODUCT_ID_VAL 0x30

static const float SCALE = 800.0f / 131072.0f;
static bool s_mmc_ok = false;

std::string mmc5983_init(int spi_fd) {
    s_mmc_ok = false;

    uint8_t id = 0;
    std::string err = spi_read_reg(spi_fd, MMC5983_REG_PRODUCT_ID, &id, 1);
    if (!err.empty()) return "mmc5983_init: read product id: " + err;
    if (id != MMC5983_PRODUCT_ID_VAL)
        return "mmc5983_init: unexpected product id 0x" + std::to_string(id) + " (expected 0x30)";

    err = spi_write_reg(spi_fd, MMC5983_REG_CTRL1, 0x80);
    if (!err.empty()) return "mmc5983_init: software reset: " + err;
    usleep(20000);

    err = spi_write_reg(spi_fd, MMC5983_REG_CTRL0, 0x08);
    if (!err.empty()) return "mmc5983_init: SET command: " + err;
    usleep(1000);

    // Default: 100Hz continuous, BW=100Hz, auto SET/RESET
    uint8_t ctrl2 = 0x08 | MMC_CONT_100HZ;
    err = spi_write_reg(spi_fd, MMC5983_REG_CTRL2, ctrl2);
    if (!err.empty()) return "mmc5983_init: continuous mode: " + err;

    s_mmc_ok = true;
    return "";
}

std::string mmc5983_configure(int spi_fd, const MMC5983_Config& cfg) {
    if (!s_mmc_ok) return "mmc5983_configure: sensor not initialized";

    std::string err;

    // Bandwidth in CTRL1 bits [1:0]
    uint8_t ctrl1 = (uint8_t)cfg.bandwidth;
    err = spi_write_reg(spi_fd, MMC5983_REG_CTRL1, ctrl1);
    if (!err.empty()) return "mmc5983_configure: bandwidth: " + err;

    // CTRL2: continuous mode enable (bit 3) + rate (bits 0-2) + auto SET/RESET (bit 5)
    uint8_t ctrl2 = (uint8_t)cfg.cont_rate;
    if (cfg.cont_rate != MMC_CONT_OFF)
        ctrl2 |= 0x08;  // CMM_EN
    if (cfg.auto_set_reset)
        ctrl2 |= 0x20;  // EN_PRD_SET
    err = spi_write_reg(spi_fd, MMC5983_REG_CTRL2, ctrl2);
    if (!err.empty()) return "mmc5983_configure: ctrl2: " + err;

    return "";
}

std::string mmc5983_read(int spi_fd, float& mx, float& my, float& mz) {
    mx = my = mz = 0.0f;
    if (!s_mmc_ok) return "mmc5983_read: sensor not initialized";

    uint8_t status = 0;
    std::string err;
    for (int i = 0; i < 10; i++) {
        err = spi_read_reg(spi_fd, MMC5983_REG_STATUS, &status, 1);
        if (!err.empty()) return "mmc5983_read: read status: " + err;
        if (status & 0x01) break;
        usleep(1000);
    }
    if (!(status & 0x01)) return "mmc5983_read: measurement not ready after polling";

    uint8_t buf[7];
    memset(buf, 0, sizeof(buf));
    err = spi_read_reg(spi_fd, MMC5983_REG_XOUT0, buf, 7);
    if (!err.empty()) return "mmc5983_read: read data: " + err;

    uint32_t raw_x = ((uint32_t)buf[0] << 10) | ((uint32_t)buf[1] << 2) | ((buf[6] >> 6) & 0x03);
    uint32_t raw_y = ((uint32_t)buf[2] << 10) | ((uint32_t)buf[3] << 2) | ((buf[6] >> 4) & 0x03);
    uint32_t raw_z = ((uint32_t)buf[4] << 10) | ((uint32_t)buf[5] << 2) | ((buf[6] >> 2) & 0x03);

    mx = ((float)raw_x - 131072.0f) * SCALE;
    my = ((float)raw_y - 131072.0f) * SCALE;
    mz = ((float)raw_z - 131072.0f) * SCALE;
    return "";
}
