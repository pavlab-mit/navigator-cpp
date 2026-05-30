#pragma once

#include "nav_bindings.h"
#include <string>

std::string ads1115_init(int i2c_fd);
std::string ads1115_configure(int i2c_fd, const ADS1115_Config& cfg);
std::string ads1115_read(int i2c_fd, int channel, float& volts);
