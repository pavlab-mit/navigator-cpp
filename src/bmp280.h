#pragma once

#include <string>

std::string bmp280_init(int i2c_fd);
std::string bmp280_read(int i2c_fd, float& pressure_kpa, float& temperature_c);
