#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

struct GpioChip;

// SPI device handle — wraps fd + optional manual chip select GPIO
struct SpiDevice {
    int fd = -1;
    int cs_pin = -1;     // -1 = no manual CS (kernel manages it)
    bool cs_active = false;
    GpioChip* gpio = nullptr;
    std::shared_ptr<std::mutex> bus_mutex;
};

// Open an SPI device. If cs_gpio >= 0, that pin is used as a manual chip select
// via the shared libgpiod chip (requested output, driven high = deselected).
// This allows multiple devices on the same spidev node (e.g., spidev1.0).
std::string spi_open(const char* dev_path, uint32_t speed_hz, uint8_t mode,
                      GpioChip* gpio, int cs_gpio, SpiDevice& dev_out);
void spi_close(SpiDevice& dev);
std::string spi_transfer(SpiDevice& dev, const uint8_t* tx, uint8_t* rx, int len);
std::string spi_write(SpiDevice& dev, const uint8_t* tx, int len);
std::string spi_read_reg(SpiDevice& dev, uint8_t reg, uint8_t* data, int len);
std::string spi_write_reg(SpiDevice& dev, uint8_t reg, uint8_t value);
