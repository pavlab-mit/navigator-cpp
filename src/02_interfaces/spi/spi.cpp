#include "spi.h"

#ifdef __linux__

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>
#include <cerrno>

std::string spi_open(const char* dev_path, uint32_t speed_hz, uint8_t mode, int& fd_out) {
    fd_out = -1;
    if (!dev_path) return "spi_open: null dev_path";
    int fd = open(dev_path, O_RDWR);
    if (fd < 0)
        return std::string("spi_open: failed to open ") + dev_path + ": " + strerror(errno);

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        close(fd); return std::string("spi_open: set mode failed: ") + strerror(errno);
    }
    uint8_t bits = 8;
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        close(fd); return std::string("spi_open: set bits failed: ") + strerror(errno);
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0) {
        close(fd); return std::string("spi_open: set speed failed: ") + strerror(errno);
    }

    fd_out = fd;
    return "";
}

void spi_close(int& fd) {
    if (fd >= 0) { close(fd); fd = -1; }
}

std::string spi_transfer(int fd, const uint8_t* tx, uint8_t* rx, int len) {
    if (fd < 0) return "spi_transfer: invalid fd";
    struct spi_ioc_transfer xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.tx_buf = (unsigned long)tx;
    xfer.rx_buf = (unsigned long)rx;
    xfer.len = len;
    xfer.speed_hz = 0;  // Use default from open
    xfer.bits_per_word = 8;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
        return std::string("spi_transfer: ioctl failed: ") + strerror(errno);
    return "";
}

std::string spi_write(int fd, const uint8_t* tx, int len) {
    return spi_transfer(fd, tx, nullptr, len);
}

std::string spi_read_reg(int fd, uint8_t reg, uint8_t* data, int len) {
    if (!data) return "spi_read_reg: null data pointer";
    if (len < 0 || len > 126) return "spi_read_reg: len out of range";
    uint8_t tx[128];
    uint8_t rx[128];
    memset(tx, 0, len + 1);
    memset(rx, 0, len + 1);
    tx[0] = reg | 0x80;
    std::string err = spi_transfer(fd, tx, rx, len + 1);
    if (!err.empty()) return err;
    memcpy(data, rx + 1, len);
    return "";
}

std::string spi_write_reg(int fd, uint8_t reg, uint8_t value) {
    uint8_t tx[2] = {(uint8_t)(reg & 0x7F), value};
    return spi_write(fd, tx, 2);
}

#else
std::string spi_open(const char*, uint32_t, uint8_t, int& fd_out) { fd_out = -1; return "spi: not supported on this platform"; }
void spi_close(int& fd) { fd = -1; }
std::string spi_transfer(int, const uint8_t*, uint8_t*, int) { return "spi: not supported on this platform"; }
std::string spi_write(int, const uint8_t*, int) { return "spi: not supported on this platform"; }
std::string spi_read_reg(int, uint8_t, uint8_t*, int) { return "spi: not supported on this platform"; }
std::string spi_write_reg(int, uint8_t, uint8_t) { return "spi: not supported on this platform"; }
#endif
