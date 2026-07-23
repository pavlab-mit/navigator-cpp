#include "spi.h"

#ifdef __linux__

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>
#include <cerrno>
#include <cstdio>

// ─── Sysfs GPIO helpers for manual chip select ─────────────────

static std::string sysfs_export(int pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    if (access(path, F_OK) == 0) return "";  // Already exported

    FILE* f = fopen("/sys/class/gpio/export", "w");
    if (!f) return std::string("sysfs_export: failed to open export: ") + strerror(errno);
    fprintf(f, "%d", pin);
    fclose(f);

    // Wait for sysfs to create the node
    for (int i = 0; i < 50; i++) {
        if (access(path, F_OK) == 0) break;
        usleep(10000);
    }
    if (access(path, F_OK) != 0)
        return "sysfs_export: gpio" + std::to_string(pin) + " direction file not created";
    return "";
}

static std::string sysfs_set_direction(int pin, const char* dir) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    FILE* f = fopen(path, "w");
    if (!f) return std::string("sysfs_set_direction: ") + strerror(errno);
    fprintf(f, "%s", dir);
    fclose(f);
    return "";
}

static std::string sysfs_write(int pin, int value) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    FILE* f = fopen(path, "w");
    if (!f) return std::string("sysfs_write: ") + strerror(errno);
    fprintf(f, "%d", value);
    fclose(f);
    return "";
}

static std::string sysfs_unexport(int pin) {
    FILE* f = fopen("/sys/class/gpio/unexport", "w");
    if (!f) return "";  // Best effort
    fprintf(f, "%d", pin);
    fclose(f);
    return "";
}

// ─── CS select/deselect ────────────────────────────────────────

static std::string cs_select(SpiDevice& dev) {
    if (dev.cs_pin < 0) return "";
    std::string err = sysfs_write(dev.cs_pin, 0);  // Active low
    if (!err.empty()) return "cs_select pin " + std::to_string(dev.cs_pin) + ": " + err;
    return "";
}

static std::string cs_deselect(SpiDevice& dev) {
    if (dev.cs_pin < 0) return "";
    std::string err = sysfs_write(dev.cs_pin, 1);  // Deselect
    if (!err.empty()) return "cs_deselect pin " + std::to_string(dev.cs_pin) + ": " + err;
    return "";
}

// ─── Public API ────────────────────────────────────────────────

std::string spi_open(const char* dev_path, uint32_t speed_hz, uint8_t mode,
                      int cs_gpio, SpiDevice& dev_out) {
    dev_out.fd = -1;
    dev_out.cs_pin = -1;
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

    dev_out.fd = fd;

    // Set up manual CS if requested
    if (cs_gpio >= 0) {
        std::string err = sysfs_export(cs_gpio);
        if (!err.empty()) { close(fd); dev_out.fd = -1; return "spi_open: " + err; }

        err = sysfs_set_direction(cs_gpio, "high");  // Output, initially high (deselected)
        if (!err.empty()) { close(fd); dev_out.fd = -1; return "spi_open: " + err; }

        dev_out.cs_pin = cs_gpio;
    }

    return "";
}

void spi_close(SpiDevice& dev) {
    if (dev.cs_pin >= 0) {
        sysfs_write(dev.cs_pin, 1);  // Deselect, but leave exported
        dev.cs_pin = -1;
    }
    if (dev.fd >= 0) { close(dev.fd); dev.fd = -1; }
}

std::string spi_transfer(SpiDevice& dev, const uint8_t* tx, uint8_t* rx, int len) {
    if (dev.fd < 0) return "spi_transfer: invalid fd";

    std::string err = cs_select(dev);
    if (!err.empty()) return "spi_transfer: " + err;

    struct spi_ioc_transfer xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.tx_buf = (unsigned long)tx;
    xfer.rx_buf = (unsigned long)rx;
    xfer.len = len;
    xfer.speed_hz = 0;
    xfer.bits_per_word = 8;

    int ret = ioctl(dev.fd, SPI_IOC_MESSAGE(1), &xfer);

    std::string cs_err = cs_deselect(dev);

    if (ret < 0)
        return std::string("spi_transfer: ioctl failed: ") + strerror(errno);
    if (!cs_err.empty())
        return "spi_transfer: " + cs_err;
    return "";
}

std::string spi_write(SpiDevice& dev, const uint8_t* tx, int len) {
    return spi_transfer(dev, tx, nullptr, len);
}

std::string spi_read_reg(SpiDevice& dev, uint8_t reg, uint8_t* data, int len) {
    if (!data) return "spi_read_reg: null data pointer";
    if (len < 0 || len > 126) return "spi_read_reg: len out of range";
    uint8_t tx[128];
    uint8_t rx[128];
    memset(tx, 0, len + 1);
    memset(rx, 0, len + 1);
    tx[0] = reg | 0x80;
    std::string err = spi_transfer(dev, tx, rx, len + 1);
    if (!err.empty()) return err;
    memcpy(data, rx + 1, len);
    return "";
}

std::string spi_write_reg(SpiDevice& dev, uint8_t reg, uint8_t value) {
    uint8_t tx[2] = {(uint8_t)(reg & 0x7F), value};
    return spi_write(dev, tx, 2);
}

#else
std::string spi_open(const char*, uint32_t, uint8_t, int, SpiDevice& d) { d.fd = -1; d.cs_pin = -1; return "spi: not supported on this platform"; }
void spi_close(SpiDevice& d) { d.fd = -1; d.cs_pin = -1; }
std::string spi_transfer(SpiDevice&, const uint8_t*, uint8_t*, int) { return "spi: not supported on this platform"; }
std::string spi_write(SpiDevice&, const uint8_t*, int) { return "spi: not supported on this platform"; }
std::string spi_read_reg(SpiDevice&, uint8_t, uint8_t*, int) { return "spi: not supported on this platform"; }
std::string spi_write_reg(SpiDevice&, uint8_t, uint8_t) { return "spi: not supported on this platform"; }
#endif
