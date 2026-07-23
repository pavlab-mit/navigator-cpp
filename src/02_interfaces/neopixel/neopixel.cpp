#include "neopixel.h"
#include "spi.h"

#include <cstring>
#include <unistd.h>
#include <vector>

#define NEOPIXEL_SPI_DEV  "/dev/spidev0.0"
#define NEOPIXEL_SPI_HZ   6400000
#define NEOPIXEL_SPI_MODE 0

static const uint8_t BIT_HIGH = 0xF0;
static const uint8_t BIT_LOW  = 0xC0;

static SpiDevice s_spi;
static int s_max_leds = 0;

static void encode_byte(uint8_t val, uint8_t* out) {
    for (int bit = 7; bit >= 0; bit--) {
        *out++ = (val & (1 << bit)) ? BIT_HIGH : BIT_LOW;
    }
}

std::string neopixel_init(int max_leds) {
    if (max_leds < 1 || max_leds > 255)
        return "neopixel_init: max_leds out of range (1-255)";
    std::string err = spi_open(NEOPIXEL_SPI_DEV, NEOPIXEL_SPI_HZ, NEOPIXEL_SPI_MODE, -1, s_spi);
    if (!err.empty()) return "neopixel_init: " + err;
    s_max_leds = max_leds;
    return "";
}

void neopixel_shutdown() {
    spi_close(s_spi);
    s_max_leds = 0;
}

std::string neopixel_set_rgb(const uint8_t (*rgb)[3], int count) {
    if (s_spi.fd < 0) return "neopixel_set_rgb: not initialized";
    if (!rgb) return "neopixel_set_rgb: null pointer";
    if (count < 1 || count > s_max_leds)
        return "neopixel_set_rgb: count out of range";

    std::vector<uint8_t> buf(count * 32);
    uint8_t* p = buf.data();
    for (int i = 0; i < count; i++) {
        encode_byte(rgb[i][1], p); p += 8;  // G
        encode_byte(rgb[i][0], p); p += 8;  // R
        encode_byte(rgb[i][2], p); p += 8;  // B
        encode_byte(0,         p); p += 8;  // W
    }

    std::string err = spi_write(s_spi, buf.data(), (int)buf.size());
    if (!err.empty()) return "neopixel_set_rgb: " + err;
    usleep(80);
    return "";
}

std::string neopixel_set_rgbw(const uint8_t (*rgbw)[4], int count) {
    if (s_spi.fd < 0) return "neopixel_set_rgbw: not initialized";
    if (!rgbw) return "neopixel_set_rgbw: null pointer";
    if (count < 1 || count > s_max_leds)
        return "neopixel_set_rgbw: count out of range";

    std::vector<uint8_t> buf(count * 32);
    uint8_t* p = buf.data();
    for (int i = 0; i < count; i++) {
        encode_byte(rgbw[i][1], p); p += 8;
        encode_byte(rgbw[i][0], p); p += 8;
        encode_byte(rgbw[i][2], p); p += 8;
        encode_byte(rgbw[i][3], p); p += 8;
    }

    std::string err = spi_write(s_spi, buf.data(), (int)buf.size());
    if (!err.empty()) return "neopixel_set_rgbw: " + err;
    usleep(80);
    return "";
}

std::string neopixel_clear(int count) {
    if (s_spi.fd < 0) return "neopixel_clear: not initialized";
    if (count < 1 || count > s_max_leds)
        return "neopixel_clear: count out of range";

    std::vector<uint8_t> buf(count * 32, BIT_LOW);
    std::string err = spi_write(s_spi, buf.data(), (int)buf.size());
    if (!err.empty()) return "neopixel_clear: " + err;
    usleep(80);
    return "";
}
