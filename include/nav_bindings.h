#pragma once

#include <cstdint>
#include <string>

// ─── Version Enums ──────────────────────────────────────────────

enum NavVersion { NAV_AUTO = 0, NAV_V1 = 1, NAV_V2 = 2 };
enum PiVersion  { PI_AUTO = 0, PI_4 = 4, PI_5 = 5 };

// ─── Data Structs ───────────────────────────────────────────────

struct NavAxisData {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct NavBaroData {
    float pressure_kpa = 0.0f;
    float temperature_c = 0.0f;
};

struct NavADCData {
    float channel[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

// ─── Navigator Class ────────────────────────────────────────────
//
// Usage:
//   Navigator nav;
//   std::string err = nav.init(NAV_AUTO, PI_AUTO);
//   if (!err.empty()) { handle error }
//
//   NavAxisData accel;
//   err = nav.read_accel(accel);
//   if (!err.empty()) { handle error }
//
// All methods return an empty string on success, or a descriptive
// error message on failure. No method will crash even if the
// hardware is absent or a sensor failed to initialize.

class Navigator {
public:
    Navigator();
    ~Navigator();

    // No copy/move — single hardware owner
    Navigator(const Navigator&) = delete;
    Navigator& operator=(const Navigator&) = delete;

    // ─── Lifecycle ──────────────────────────────────────────

    // Initialize hardware. Detects versions if NAV_AUTO / PI_AUTO.
    // Sensors that fail to init are marked unavailable; subsequent
    // reads on them return an error string but do not crash.
    std::string init(NavVersion nav = NAV_AUTO, PiVersion pi = PI_AUTO);

    // Shutdown and release all resources. Safe to call multiple times.
    void shutdown();

    // True after a successful init(), false after shutdown().
    bool is_initialized() const;

    // Detected versions (valid after init).
    NavVersion detected_version() const;
    PiVersion  detected_pi() const;

    // ─── IMU (ICM20689, SPI) ────────────────────────────────

    std::string read_accel(NavAxisData& out);
    std::string read_gyro(NavAxisData& out);

    // ─── Magnetometers (I2C) ────────────────────────────────

    std::string read_mag_ak09915(NavAxisData& out);
    std::string read_mag_mmc5983(NavAxisData& out);

    // ─── Barometer (BMP280 or BMP390, I2C) ──────────────────

    std::string read_baro(NavBaroData& out);

    // ─── ADC (ADS1115, I2C) ────────────────────────────────

    std::string read_adc(int channel, float& volts);
    std::string read_adc_all(NavADCData& out);

    // ─── Leak Detector (GPIO) ──────────────────────────────

    std::string read_leak(bool& detected);

    // ─── PWM (PCA9685, I2C + GPIO OE pin) ──────────────────

    std::string pwm_enable(bool enable);
    std::string pwm_set_frequency(float freq_hz);
    std::string pwm_set_duty(int channel, float duty);
    std::string pwm_set_raw(int channel, uint16_t on, uint16_t off);

    // ─── User LEDs (GPIO, active low) ──────────────────────

    std::string led_set(int led, bool on);
    std::string led_get(int led, bool& on);

    // ─── NeoPixel (SK6812 via SPI) ─────────────────────────

    std::string neopixel_set(const uint8_t (*rgb)[3], int count);
    std::string neopixel_set_rgbw(const uint8_t (*rgbw)[4], int count);
    std::string neopixel_clear();

private:
    struct Impl;
    Impl* m_impl;
};
