#pragma once

#include "nav_bindings.h"
#include <string>

std::string bmp390_init(int i2c_fd);
std::string bmp390_configure(int i2c_fd, const BARO_Config& cfg);
std::string bmp390_read(int i2c_fd, float& pressure_kpa, float& temperature_c);
