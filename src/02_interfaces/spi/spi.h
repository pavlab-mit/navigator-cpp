#pragma once

#include <cstdint>
#include <string>

std::string spi_open(const char* dev_path, uint32_t speed_hz, uint8_t mode, int& fd_out);
void spi_close(int& fd);
std::string spi_transfer(int fd, const uint8_t* tx, uint8_t* rx, int len);
std::string spi_write(int fd, const uint8_t* tx, int len);
std::string spi_read_reg(int fd, uint8_t reg, uint8_t* data, int len);
std::string spi_write_reg(int fd, uint8_t reg, uint8_t value);
