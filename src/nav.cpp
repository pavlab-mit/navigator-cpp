#include "nav_bindings.h"

#include "i2c.h"
#include "spi.h"
#include "gpio.h"
#include "bmp390.h"
#include "bmp280.h"
#include "icm20689.h"
#include "ak09915.h"
#include "mmc5983.h"
#include "ads1115.h"
#include "pca9685.h"
#include "leak.h"
#include "led.h"
#include "neopixel.h"

#include <sys/stat.h>

// ─── Impl ───────────────────────────────────────────────────────

struct Navigator::Impl {
    bool initialized = false;

    NavVersion nav_version = NAV_AUTO;
    PiVersion  pi_version  = PI_AUTO;

    // File descriptors (-1 = not open)
    int i2c_sensor_fd = -1;
    int i2c_pwm_fd    = -1;
    int spi_imu_fd    = -1;

    GpioChip* gpio = nullptr;

    // Per-sensor init status
    bool baro_ok  = false;
    bool imu_ok   = false;
    bool ak_ok    = false;
    bool mmc_ok   = false;
    bool adc_ok   = false;
    bool pwm_ok   = false;
    bool leak_ok  = false;
    bool led_ok   = false;
    bool neo_ok   = false;

    static const int OE_PIN = 26;
};

// ─── Lifecycle ──────────────────────────────────────────────────

Navigator::Navigator() : m_impl(new Impl()) {}

Navigator::~Navigator() {
    shutdown();
    delete m_impl;
}

static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

std::string Navigator::init(NavVersion nav, PiVersion pi) {
    if (m_impl->initialized) return "Navigator::init: already initialized";

    // Detect Pi version
    m_impl->pi_version = (pi == PI_AUTO)
        ? (file_exists("/dev/gpiochip4") ? PI_5 : PI_4)
        : pi;

    const char* gpio_chip = (m_impl->pi_version == PI_5) ? "/dev/gpiochip4" : "/dev/gpiochip0";
    const char* pwm_i2c   = (m_impl->pi_version == PI_5) ? "/dev/i2c-3" : "/dev/i2c-4";

    // Open buses — collect warnings, don't abort on individual failures
    std::string warnings;
    std::string err;

    err = i2c_open("/dev/i2c-1", m_impl->i2c_sensor_fd);
    if (!err.empty()) warnings += "  [i2c-1] " + err + "\n";

    err = i2c_open(pwm_i2c, m_impl->i2c_pwm_fd);
    if (!err.empty()) warnings += "  [pwm-i2c] " + err + "\n";

    err = spi_open("/dev/spidev1.2", 10000000, 0, m_impl->spi_imu_fd);
    if (!err.empty()) warnings += "  [spi-imu] " + err + "\n";

    err = gpio_open(gpio_chip, m_impl->gpio);
    if (!err.empty()) warnings += "  [gpio] " + err + "\n";

    // Detect Navigator version
    if (nav == NAV_AUTO && m_impl->i2c_sensor_fd >= 0) {
        // Try BMP390 first
        err = i2c_set_slave(m_impl->i2c_sensor_fd, 0x76);
        if (err.empty()) {
            uint8_t id = 0;
            err = i2c_read_reg(m_impl->i2c_sensor_fd, 0x00, id);
            if (err.empty() && id == 0x60) {
                m_impl->nav_version = NAV_V2;
            } else {
                err = i2c_read_reg(m_impl->i2c_sensor_fd, 0xD0, id);
                m_impl->nav_version = (err.empty() && id == 0x58) ? NAV_V1 : NAV_V1;
            }
        } else {
            m_impl->nav_version = NAV_V1;
        }
    } else {
        m_impl->nav_version = (nav == NAV_AUTO) ? NAV_V1 : nav;
    }

    // Initialize sensors — each independently, failures don't block others
    if (m_impl->i2c_sensor_fd >= 0) {
        if (m_impl->nav_version == NAV_V2) {
            err = bmp390_init(m_impl->i2c_sensor_fd);
            m_impl->baro_ok = err.empty();
            if (!err.empty()) warnings += "  [bmp390] " + err + "\n";
        } else {
            err = bmp280_init(m_impl->i2c_sensor_fd);
            m_impl->baro_ok = err.empty();
            if (!err.empty()) warnings += "  [bmp280] " + err + "\n";
        }

        err = ak09915_init(m_impl->i2c_sensor_fd);
        m_impl->ak_ok = err.empty();
        if (!err.empty()) warnings += "  [ak09915] " + err + "\n";

        err = mmc5983_init(m_impl->i2c_sensor_fd);
        m_impl->mmc_ok = err.empty();
        if (!err.empty()) warnings += "  [mmc5983] " + err + "\n";

        err = ads1115_init(m_impl->i2c_sensor_fd);
        m_impl->adc_ok = err.empty();
        if (!err.empty()) warnings += "  [ads1115] " + err + "\n";
    }

    if (m_impl->spi_imu_fd >= 0) {
        err = icm20689_init(m_impl->spi_imu_fd);
        m_impl->imu_ok = err.empty();
        if (!err.empty()) warnings += "  [icm20689] " + err + "\n";
    }

    if (m_impl->i2c_pwm_fd >= 0 && m_impl->gpio) {
        err = pca9685_init(m_impl->i2c_pwm_fd, m_impl->gpio, Impl::OE_PIN);
        m_impl->pwm_ok = err.empty();
        if (!err.empty()) warnings += "  [pca9685] " + err + "\n";
    }

    if (m_impl->gpio) {
        err = leak_init(m_impl->gpio);
        m_impl->leak_ok = err.empty();
        if (!err.empty()) warnings += "  [leak] " + err + "\n";

        err = led_init(m_impl->gpio);
        m_impl->led_ok = err.empty();
        if (!err.empty()) warnings += "  [led] " + err + "\n";
    }

    err = neopixel_init(255);
    m_impl->neo_ok = err.empty();
    if (!err.empty()) warnings += "  [neopixel] " + err + "\n";

    m_impl->initialized = true;
    return warnings;  // Empty = all good, non-empty = partial init (still usable)
}

void Navigator::shutdown() {
    if (!m_impl->initialized) return;

    neopixel_shutdown();
    gpio_close(m_impl->gpio);
    i2c_close(m_impl->i2c_sensor_fd);
    i2c_close(m_impl->i2c_pwm_fd);
    spi_close(m_impl->spi_imu_fd);

    m_impl->baro_ok = m_impl->imu_ok = m_impl->ak_ok = m_impl->mmc_ok = false;
    m_impl->adc_ok = m_impl->pwm_ok = m_impl->leak_ok = m_impl->led_ok = m_impl->neo_ok = false;
    m_impl->initialized = false;
}

bool Navigator::is_initialized() const { return m_impl->initialized; }
NavVersion Navigator::detected_version() const { return m_impl->nav_version; }
PiVersion Navigator::detected_pi() const { return m_impl->pi_version; }

// ─── Sensor reads ───────────────────────────────────────────────

std::string Navigator::read_accel(NavAxisData& out) {
    out = {};
    if (!m_impl->imu_ok) return "read_accel: IMU (ICM20689) not initialized";
    return icm20689_read_accel(m_impl->spi_imu_fd, out.x, out.y, out.z);
}

std::string Navigator::read_gyro(NavAxisData& out) {
    out = {};
    if (!m_impl->imu_ok) return "read_gyro: IMU (ICM20689) not initialized";
    return icm20689_read_gyro(m_impl->spi_imu_fd, out.x, out.y, out.z);
}

std::string Navigator::read_mag_ak09915(NavAxisData& out) {
    out = {};
    if (!m_impl->ak_ok) return "read_mag_ak09915: AK09915 not initialized";
    return ak09915_read(m_impl->i2c_sensor_fd, out.x, out.y, out.z);
}

std::string Navigator::read_mag_mmc5983(NavAxisData& out) {
    out = {};
    if (!m_impl->mmc_ok) return "read_mag_mmc5983: MMC5983 not initialized";
    return mmc5983_read(m_impl->i2c_sensor_fd, out.x, out.y, out.z);
}

std::string Navigator::read_baro(NavBaroData& out) {
    out = {};
    if (!m_impl->baro_ok) return "read_baro: barometer not initialized";
    if (m_impl->nav_version == NAV_V2)
        return bmp390_read(m_impl->i2c_sensor_fd, out.pressure_kpa, out.temperature_c);
    else
        return bmp280_read(m_impl->i2c_sensor_fd, out.pressure_kpa, out.temperature_c);
}

std::string Navigator::read_adc(int channel, float& volts) {
    volts = 0.0f;
    if (!m_impl->adc_ok) return "read_adc: ADS1115 not initialized";
    return ads1115_read(m_impl->i2c_sensor_fd, channel, volts);
}

std::string Navigator::read_adc_all(NavADCData& out) {
    out = {};
    if (!m_impl->adc_ok) return "read_adc_all: ADS1115 not initialized";
    for (int i = 0; i < 4; i++) {
        std::string err = ads1115_read(m_impl->i2c_sensor_fd, i, out.channel[i]);
        if (!err.empty()) return err;
    }
    return "";
}

std::string Navigator::read_leak(bool& detected) {
    detected = false;
    if (!m_impl->leak_ok) return "read_leak: leak detector not initialized";
    return leak_read(m_impl->gpio, detected);
}

// ─── PWM ────────────────────────────────────────────────────────

std::string Navigator::pwm_enable(bool enable) {
    if (!m_impl->pwm_ok) return "pwm_enable: PCA9685 not initialized";
    return pca9685_enable(m_impl->gpio, Impl::OE_PIN, enable);
}

std::string Navigator::pwm_set_frequency(float freq_hz) {
    if (!m_impl->pwm_ok) return "pwm_set_frequency: PCA9685 not initialized";
    return pca9685_set_frequency(m_impl->i2c_pwm_fd, freq_hz);
}

std::string Navigator::pwm_set_duty(int channel, float duty) {
    if (!m_impl->pwm_ok) return "pwm_set_duty: PCA9685 not initialized";
    return pca9685_set_duty(m_impl->i2c_pwm_fd, channel, duty);
}

std::string Navigator::pwm_set_raw(int channel, uint16_t on, uint16_t off) {
    if (!m_impl->pwm_ok) return "pwm_set_raw: PCA9685 not initialized";
    return pca9685_set_raw(m_impl->i2c_pwm_fd, channel, on, off);
}

// ─── LEDs ───────────────────────────────────────────────────────

std::string Navigator::led_set(int led, bool on) {
    if (!m_impl->led_ok) return "led_set: LEDs not initialized";
    return ::led_set(m_impl->gpio, led, on);
}

std::string Navigator::led_get(int led, bool& on) {
    on = false;
    if (!m_impl->led_ok) return "led_get: LEDs not initialized";
    return ::led_get(m_impl->gpio, led, on);
}

// ─── NeoPixel ───────────────────────────────────────────────────

std::string Navigator::neopixel_set(const uint8_t (*rgb)[3], int count) {
    if (!m_impl->neo_ok) return "neopixel_set: NeoPixel not initialized";
    return neopixel_set_rgb(rgb, count);
}

std::string Navigator::neopixel_set_rgbw(const uint8_t (*rgbw)[4], int count) {
    if (!m_impl->neo_ok) return "neopixel_set_rgbw: NeoPixel not initialized";
    return ::neopixel_set_rgbw(rgbw, count);
}

std::string Navigator::neopixel_clear() {
    if (!m_impl->neo_ok) return "neopixel_clear: NeoPixel not initialized";
    return ::neopixel_clear(1);
}
