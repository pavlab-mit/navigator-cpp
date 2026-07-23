#pragma once

#include "nav_bindings.h"
#include "spi.h"
#include <string>

std::string mmc5983_init(SpiDevice& spi_fd);
std::string mmc5983_configure(SpiDevice& spi_fd, const MMC5983_Config& cfg);
std::string mmc5983_read(SpiDevice& spi_fd, float& mx, float& my, float& mz);
