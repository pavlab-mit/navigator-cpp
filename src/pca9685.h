#pragma once

#include <cstdint>
#include <string>

struct GpioChip;

std::string pca9685_init(int i2c_fd, GpioChip* gpio, int oe_pin);
std::string pca9685_enable(GpioChip* gpio, int oe_pin, bool enable);
std::string pca9685_set_frequency(int i2c_fd, float freq_hz);
std::string pca9685_set_duty(int i2c_fd, int channel, float duty);
std::string pca9685_set_raw(int i2c_fd, int channel, uint16_t on, uint16_t off);
