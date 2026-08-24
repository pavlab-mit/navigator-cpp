#include "i2c.h"

#ifdef __linux__

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <mutex>
#include <unordered_map>

// I2C_SLAVE changes shared state on an open file description. Navigator uses
// one sensor-bus fd from multiple threads, so address selection followed by a
// separate read/write can be retargeted by another thread. Keep the selected
// address per thread and put it directly in atomic I2C_RDWR messages instead.
struct SlaveSelection {
    uint8_t address;
    uint64_t generation;
};

static std::mutex s_fd_mutex;
static uint64_t s_next_generation = 1;
static std::unordered_map<int, uint64_t> s_fd_generation;
static thread_local std::unordered_map<int, SlaveSelection> s_slave_by_fd;

static std::string hex_byte(uint8_t value) {
    char out[5];
    snprintf(out, sizeof(out), "0x%02X", value);
    return out;
}

static std::string selected_slave(int fd, uint8_t& addr) {
    auto it = s_slave_by_fd.find(fd);
    if (it == s_slave_by_fd.end())
        return "I2C slave not selected in this thread";
    std::lock_guard<std::mutex> lock(s_fd_mutex);
    auto live = s_fd_generation.find(fd);
    if (live == s_fd_generation.end() || live->second != it->second.generation)
        return "I2C slave selection is stale";
    addr = it->second.address;
    return "";
}

static std::string transfer(int fd, struct i2c_msg* messages, uint32_t count,
                            const char* operation) {
    struct i2c_rdwr_ioctl_data request = {messages, count};
    if (ioctl(fd, I2C_RDWR, &request) < 0)
        return std::string(operation) + ": " + strerror(errno);
    return "";
}

std::string i2c_open(const char* bus_path, int& fd_out) {
    fd_out = -1;
    if (!bus_path) return "i2c_open: null bus_path";
    int fd = open(bus_path, O_RDWR);
    if (fd < 0)
        return std::string("i2c_open: failed to open ") + bus_path + ": " + strerror(errno);
    {
        std::lock_guard<std::mutex> lock(s_fd_mutex);
        s_fd_generation[fd] = s_next_generation++;
    }
    fd_out = fd;
    return "";
}

void i2c_close(int& fd) {
    if (fd >= 0) {
        s_slave_by_fd.erase(fd);
        {
            std::lock_guard<std::mutex> lock(s_fd_mutex);
            s_fd_generation.erase(fd);
        }
        close(fd);
        fd = -1;
    }
}

std::string i2c_set_slave(int fd, uint8_t addr) {
    if (fd < 0) return "i2c_set_slave: invalid fd";
    std::lock_guard<std::mutex> lock(s_fd_mutex);
    auto it = s_fd_generation.find(fd);
    if (it == s_fd_generation.end()) return "i2c_set_slave: unknown fd";
    s_slave_by_fd[fd] = {addr, it->second};
    return "";
}

std::string i2c_write_reg(int fd, uint8_t reg, uint8_t value) {
    if (fd < 0) return "i2c_write_reg: invalid fd";
    uint8_t addr = 0;
    std::string err = selected_slave(fd, addr);
    if (!err.empty()) return "i2c_write_reg: " + err;
    uint8_t buf[2] = {reg, value};
    struct i2c_msg message = {addr, 0, sizeof(buf), buf};
    err = transfer(fd, &message, 1, "write failed");
    if (!err.empty()) return "i2c_write_reg: reg " + hex_byte(reg) + ": " + err;
    return err;
}

std::string i2c_write_reg_buf(int fd, uint8_t reg, const uint8_t* data, int len) {
    if (fd < 0) return "i2c_write_reg_buf: invalid fd";
    if (!data) return "i2c_write_reg_buf: null data pointer";
    if (len < 0 || len > 254) return "i2c_write_reg_buf: len out of range";
    uint8_t buf[256];
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    uint8_t addr = 0;
    std::string err = selected_slave(fd, addr);
    if (!err.empty()) return "i2c_write_reg_buf: " + err;
    struct i2c_msg message = {addr, 0, static_cast<uint16_t>(len + 1), buf};
    err = transfer(fd, &message, 1, "write failed");
    if (!err.empty()) return "i2c_write_reg_buf: reg " + hex_byte(reg) + ": " + err;
    return err;
}

std::string i2c_read_reg(int fd, uint8_t reg, uint8_t& value) {
    value = 0;
    return i2c_read_reg_buf(fd, reg, &value, 1);
}

std::string i2c_read_reg_buf(int fd, uint8_t reg, uint8_t* data, int len) {
    if (fd < 0) return "i2c_read_reg_buf: invalid fd";
    if (!data) return "i2c_read_reg_buf: null data pointer";
    if (len < 0 || len > 65535) return "i2c_read_reg_buf: len out of range";
    memset(data, 0, len);  // Zero-init before read
    uint8_t addr = 0;
    std::string err = selected_slave(fd, addr);
    if (!err.empty()) return "i2c_read_reg_buf: " + err;
    struct i2c_msg messages[2] = {
        {addr, 0, 1, &reg},
        {addr, I2C_M_RD, static_cast<uint16_t>(len), data}
    };
    err = transfer(fd, messages, 2, "combined read failed");
    if (!err.empty()) return "i2c_read_reg_buf: reg " + hex_byte(reg) + ": " + err;
    return err;
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
