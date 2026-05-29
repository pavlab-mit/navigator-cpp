#pragma once

#include <cstdint>
#include <string>

// All functions return "" on success, or an error message on failure.

std::string i2c_open(const char* bus_path, int& fd_out);
void i2c_close(int& fd);
std::string i2c_set_slave(int fd, uint8_t addr);
std::string i2c_write_reg(int fd, uint8_t reg, uint8_t value);
std::string i2c_write_reg_buf(int fd, uint8_t reg, const uint8_t* data, int len);
std::string i2c_read_reg(int fd, uint8_t reg, uint8_t& value);
std::string i2c_read_reg_buf(int fd, uint8_t reg, uint8_t* data, int len);
