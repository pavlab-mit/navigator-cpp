#pragma once

#include <string>
#include "nav_bindings.h"

std::string bmp280_init(int i2c_fd);
std::string bmp280_configure(int i2c_fd, const BARO_Config& cfg);
std::string bmp280_read(int i2c_fd, float& pressure_kpa, float& temperature_c);
