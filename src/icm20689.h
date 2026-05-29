#pragma once

#include <string>

std::string icm20689_init(int spi_fd);
std::string icm20689_read_accel(int spi_fd, float& ax, float& ay, float& az);
std::string icm20689_read_gyro(int spi_fd, float& gx, float& gy, float& gz);
