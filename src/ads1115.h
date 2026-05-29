#pragma once

#include <string>

std::string ads1115_init(int i2c_fd);
std::string ads1115_read(int i2c_fd, int channel, float& volts);
