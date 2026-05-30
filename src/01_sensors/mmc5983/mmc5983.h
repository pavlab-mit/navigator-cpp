#pragma once

#include "nav_bindings.h"
#include <string>

std::string mmc5983_init(int spi_fd);
std::string mmc5983_configure(int spi_fd, const MMC5983_Config& cfg);
std::string mmc5983_read(int spi_fd, float& mx, float& my, float& mz);
