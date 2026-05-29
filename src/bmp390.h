#pragma once

#include <string>

std::string bmp390_init(int i2c_fd);
std::string bmp390_read(int i2c_fd, float& pressure_kpa, float& temperature_c);
