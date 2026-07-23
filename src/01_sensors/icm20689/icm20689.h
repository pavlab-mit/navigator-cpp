#pragma once

#include "nav_bindings.h"
#include "spi.h"
#include <string>

std::string icm20689_init(SpiDevice& spi_fd);
std::string icm20689_configure(SpiDevice& spi_fd, const ICM_Config& cfg);
std::string icm20689_read_accel(SpiDevice& spi_fd, float& ax, float& ay, float& az);
std::string icm20689_read_gyro(SpiDevice& spi_fd, float& gx, float& gy, float& gz);
