#include "i2c.h"

#ifdef __linux__

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <cerrno>

std::string i2c_open(const char* bus_path, int& fd_out) {
    fd_out = -1;
    if (!bus_path) return "i2c_open: null bus_path";
    int fd = open(bus_path, O_RDWR);
    if (fd < 0)
        return std::string("i2c_open: failed to open ") + bus_path + ": " + strerror(errno);
    fd_out = fd;
    return "";
}

void i2c_close(int& fd) {
    if (fd >= 0) { close(fd); fd = -1; }
}

std::string i2c_set_slave(int fd, uint8_t addr) {
    if (fd < 0) return "i2c_set_slave: invalid fd";
    if (ioctl(fd, I2C_SLAVE, addr) < 0)
        return std::string("i2c_set_slave: ioctl failed for addr 0x")
               + std::to_string(addr) + ": " + strerror(errno);
    return "";
}

std::string i2c_write_reg(int fd, uint8_t reg, uint8_t value) {
    if (fd < 0) return "i2c_write_reg: invalid fd";
    uint8_t buf[2] = {reg, value};
    ssize_t ret = write(fd, buf, 2);
    if (ret != 2)
        return std::string("i2c_write_reg: write failed for reg 0x")
               + std::to_string(reg) + ": " + strerror(errno);
    return "";
}

std::string i2c_write_reg_buf(int fd, uint8_t reg, const uint8_t* data, int len) {
    if (fd < 0) return "i2c_write_reg_buf: invalid fd";
    if (!data) return "i2c_write_reg_buf: null data pointer";
    if (len < 0 || len > 254) return "i2c_write_reg_buf: len out of range";
    uint8_t buf[256];
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    ssize_t ret = write(fd, buf, len + 1);
    if (ret != len + 1)
        return std::string("i2c_write_reg_buf: write failed for reg 0x")
               + std::to_string(reg) + ": " + strerror(errno);
    return "";
}

std::string i2c_read_reg(int fd, uint8_t reg, uint8_t& value) {
    value = 0;
    return i2c_read_reg_buf(fd, reg, &value, 1);
}

std::string i2c_read_reg_buf(int fd, uint8_t reg, uint8_t* data, int len) {
    if (fd < 0) return "i2c_read_reg_buf: invalid fd";
    if (!data) return "i2c_read_reg_buf: null data pointer";
    memset(data, 0, len);  // Zero-init before read
    ssize_t ret = write(fd, &reg, 1);
    if (ret != 1)
        return std::string("i2c_read_reg_buf: write reg addr failed: ") + strerror(errno);
    ret = read(fd, data, len);
    if (ret != len)
        return std::string("i2c_read_reg_buf: read failed for reg 0x")
               + std::to_string(reg) + ": " + strerror(errno);
    return "";
}

#else
std::string i2c_open(const char*, int& fd_out) { fd_out = -1; return "i2c: not supported on this platform"; }
void i2c_close(int& fd) { fd = -1; }
std::string i2c_set_slave(int, uint8_t) { return "i2c: not supported on this platform"; }
std::string i2c_write_reg(int, uint8_t, uint8_t) { return "i2c: not supported on this platform"; }
std::string i2c_write_reg_buf(int, uint8_t, const uint8_t*, int) { return "i2c: not supported on this platform"; }
std::string i2c_read_reg(int, uint8_t, uint8_t&) { return "i2c: not supported on this platform"; }
std::string i2c_read_reg_buf(int, uint8_t, uint8_t*, int) { return "i2c: not supported on this platform"; }
#endif
