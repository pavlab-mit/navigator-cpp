#pragma once

#include <string>

struct GpioChip;

std::string leak_init(GpioChip* gpio);
std::string leak_read(GpioChip* gpio, bool& detected);
