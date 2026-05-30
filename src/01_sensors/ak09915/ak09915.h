#pragma once

#include "nav_bindings.h"
#include <string>

std::string ak09915_init(int i2c_fd);
std::string ak09915_configure(int i2c_fd, const AK09915_Config& cfg);
std::string ak09915_read(int i2c_fd, float& mx, float& my, float& mz);
