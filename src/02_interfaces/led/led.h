#pragma once

#include <string>

struct GpioChip;

std::string led_init(GpioChip* gpio);
std::string led_set(GpioChip* gpio, int led, bool on);
std::string led_get(GpioChip* gpio, int led, bool& on);
