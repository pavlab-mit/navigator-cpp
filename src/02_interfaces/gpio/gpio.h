#pragma once

#include <string>

struct GpioChip;

std::string gpio_open(const char* chip_path, GpioChip*& chip_out);
void gpio_close(GpioChip*& chip);
std::string gpio_request_output(GpioChip* chip, int pin, int initial_value, const char* label);
std::string gpio_request_input(GpioChip* chip, int pin, const char* label);
std::string gpio_set(GpioChip* chip, int pin, int value);
std::string gpio_get(GpioChip* chip, int pin, int& value);
void gpio_release(GpioChip* chip, int pin);
