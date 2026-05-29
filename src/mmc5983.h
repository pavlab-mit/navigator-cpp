#pragma once

#include <string>

std::string mmc5983_init(int i2c_fd);
std::string mmc5983_read(int i2c_fd, float& mx, float& my, float& mz);
