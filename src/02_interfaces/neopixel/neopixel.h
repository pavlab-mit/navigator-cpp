#pragma once

#include <cstdint>
#include <string>

std::string neopixel_init(int max_leds);
void neopixel_shutdown();
std::string neopixel_set_rgb(const uint8_t (*rgb)[3], int count);
std::string neopixel_set_rgbw(const uint8_t (*rgbw)[4], int count);
std::string neopixel_clear(int count);
