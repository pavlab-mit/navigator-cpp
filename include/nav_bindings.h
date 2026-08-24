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

struct NavAttitudeData {
    float roll  = 0.0f;    // radians
    float pitch = 0.0f;    // radians
    float yaw   = 0.0f;    // radians
    float qw = 1.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f;
};

// ═══════════════════════════════════════════════════════════════
//  Sensor Configuration Structs
// ═══════════════════════════════════════════════════════════════

// ─── ICM20602 / ICM20689 (Accel + Gyro) ────────────────────────

enum ICM_AccelRange {
    ICM_ACCEL_2G  = 0x00,   // ±2g,  sensitivity 16384 LSB/g
    ICM_ACCEL_4G  = 0x08,   // ±4g,  sensitivity 8192 LSB/g
    ICM_ACCEL_8G  = 0x10,   // ±8g,  sensitivity 4096 LSB/g
    ICM_ACCEL_16G = 0x18    // ±16g, sensitivity 2048 LSB/g
};

enum ICM_GyroRange {
    ICM_GYRO_250DPS  = 0x00,  // ±250°/s,  sensitivity 131 LSB/(°/s)
    ICM_GYRO_500DPS  = 0x08,  // ±500°/s,  sensitivity 65.5 LSB/(°/s)
    ICM_GYRO_1000DPS = 0x10,  // ±1000°/s, sensitivity 32.8 LSB/(°/s)
    ICM_GYRO_2000DPS = 0x18   // ±2000°/s, sensitivity 16.4 LSB/(°/s)
};

enum ICM_DlpfBandwidth {
    ICM_DLPF_OFF    = 0xFF,  // Disabled (default)
    ICM_DLPF_250HZ  = 0x00,
    ICM_DLPF_176HZ  = 0x01,
    ICM_DLPF_92HZ   = 0x02,
    ICM_DLPF_41HZ   = 0x03,
    ICM_DLPF_20HZ   = 0x04,
    ICM_DLPF_10HZ   = 0x05,
    ICM_DLPF_5HZ    = 0x06
};

struct ICM_Config {
    ICM_AccelRange    accel_range = ICM_ACCEL_2G;
    ICM_GyroRange     gyro_range  = ICM_GYRO_250DPS;
    ICM_DlpfBandwidth accel_dlpf  = ICM_DLPF_OFF;
    ICM_DlpfBandwidth gyro_dlpf   = ICM_DLPF_OFF;
    uint8_t           sample_rate_div = 0;  // 0 = 1kHz, N = 1kHz/(1+N)
};

// ─── AK09915 (Magnetometer) ────────────────────────────────────

enum AK_Mode {
    AK_POWER_DOWN  = 0x00,
    AK_SINGLE      = 0x01,
    AK_CONT_10HZ   = 0x02,
    AK_CONT_20HZ   = 0x04,
    AK_CONT_50HZ   = 0x06,
    AK_CONT_100HZ  = 0x08,
    AK_CONT_200HZ  = 0x0A,
    AK_CONT_1HZ    = 0x0C
};

struct AK09915_Config {
    AK_Mode mode = AK_CONT_200HZ;
};

// ─── MMC5983 (Magnetometer, SPI) ───────────────────────────────

enum MMC_Bandwidth {
    MMC_BW_50HZ   = 0x00,
    MMC_BW_100HZ  = 0x01,
    MMC_BW_225HZ  = 0x02,
    MMC_BW_580HZ  = 0x03
};

enum MMC_ContRate {
    MMC_CONT_OFF    = 0x00,
    MMC_CONT_1HZ    = 0x01,
    MMC_CONT_10HZ   = 0x02,
    MMC_CONT_20HZ   = 0x03,
    MMC_CONT_50HZ   = 0x04,
    MMC_CONT_100HZ  = 0x05,
    MMC_CONT_200HZ  = 0x06,
    MMC_CONT_1000HZ = 0x07
};

struct MMC5983_Config {
    MMC_Bandwidth bandwidth = MMC_BW_100HZ;
    MMC_ContRate  cont_rate = MMC_CONT_100HZ;
    bool          auto_set_reset = true;  // Periodic degauss
};

// ─── BMP390 / BMP280 (Barometer) ───────────────────────────────

enum BARO_Oversampling {
    BARO_OS_NONE = 0x00,
    BARO_OS_2X   = 0x01,
    BARO_OS_4X   = 0x02,
    BARO_OS_8X   = 0x03,
    BARO_OS_16X  = 0x04,
    BARO_OS_32X  = 0x05
};

enum BARO_OutputRate {
    BARO_ODR_200HZ = 0x00,
    BARO_ODR_100HZ = 0x01,
    BARO_ODR_50HZ  = 0x02,
    BARO_ODR_25HZ  = 0x03,
    BARO_ODR_12HZ  = 0x04,
    BARO_ODR_6HZ   = 0x05,
    BARO_ODR_3HZ   = 0x06,
    BARO_ODR_1HZ   = 0x07
};

enum BARO_IIRFilter {
    BARO_IIR_OFF     = 0x00,
    BARO_IIR_COEFF_1 = 0x01,
    BARO_IIR_COEFF_3 = 0x02,
    BARO_IIR_COEFF_7 = 0x03,
    BARO_IIR_COEFF_15 = 0x04,
    BARO_IIR_COEFF_31 = 0x05,
    BARO_IIR_COEFF_63 = 0x06,
    BARO_IIR_COEFF_127 = 0x07
};

struct BARO_Config {
    BARO_Oversampling press_os = BARO_OS_4X;
    BARO_Oversampling temp_os  = BARO_OS_NONE;
    BARO_OutputRate   odr      = BARO_ODR_50HZ;
    BARO_IIRFilter    iir      = BARO_IIR_OFF;
};

// ─── ADS1115 (ADC) ─────────────────────────────────────────────

enum ADC_Gain {
    ADC_GAIN_6144MV = 0x00,  // ±6.144V (LSB = 0.1875 mV)
    ADC_GAIN_4096MV = 0x01,  // ±4.096V (LSB = 0.125 mV)  ← default
    ADC_GAIN_2048MV = 0x02,  // ±2.048V (LSB = 0.0625 mV)
    ADC_GAIN_1024MV = 0x03,  // ±1.024V (LSB = 0.03125 mV)
    ADC_GAIN_512MV  = 0x04,  // ±0.512V (LSB = 0.015625 mV)
    ADC_GAIN_256MV  = 0x05   // ±0.256V (LSB = 0.0078125 mV)
};

enum ADC_DataRate {
    ADC_RATE_8SPS   = 0x00,
    ADC_RATE_16SPS  = 0x01,
    ADC_RATE_32SPS  = 0x02,
    ADC_RATE_64SPS  = 0x03,
    ADC_RATE_128SPS = 0x04,
    ADC_RATE_250SPS = 0x05,
    ADC_RATE_475SPS = 0x06,
    ADC_RATE_860SPS = 0x07   // ← default
};

struct ADS1115_Config {
    ADC_Gain     gain      = ADC_GAIN_4096MV;
    ADC_DataRate data_rate = ADC_RATE_860SPS;
};

// ─── PCA9685 (PWM) ─────────────────────────────────────────────

struct PCA9685_Config {
    float frequency_hz = 50.0f;  // 24-1526 Hz
    bool  invert       = false;
    bool  open_drain   = false;
};

std::string navigator_force_pwm_off(std::string* detail = nullptr);

// ═══════════════════════════════════════════════════════════════
//  Navigator Class
// ═══════════════════════════════════════════════════════════════

class Navigator {
public:
    Navigator();
    ~Navigator();

    Navigator(const Navigator&) = delete;
    Navigator& operator=(const Navigator&) = delete;

    // ─── Lifecycle ──────────────────────────────────────────

    std::string init(NavVersion nav = NAV_AUTO, PiVersion pi = PI_AUTO);
    void shutdown();
    bool is_initialized() const;
    bool is_pwm_ready() const;
    NavVersion detected_version() const;
    PiVersion  detected_pi() const;

    // ─── Sensor Configuration ──────────────────────────────
    // Call after init(). Reconfigures the sensor registers.
    // Returns empty string on success.

    std::string configure_imu(const ICM_Config& cfg);
    std::string configure_ak09915(const AK09915_Config& cfg);
    std::string configure_mmc5983(const MMC5983_Config& cfg);
    std::string configure_baro(const BARO_Config& cfg);
    std::string configure_adc(const ADS1115_Config& cfg);
    std::string configure_pwm(const PCA9685_Config& cfg);

    // ─── IMU (ICM20602/ICM20689, SPI) ──────────────────────

    std::string read_accel(NavAxisData& out);
    std::string read_gyro(NavAxisData& out);

    // ─── Magnetometers ─────────────────────────────────────

    std::string read_mag_ak09915(NavAxisData& out);
    std::string read_mag_mmc5983(NavAxisData& out);
    std::string read_mag(NavAxisData& out);  // V1: AK09915, V2: MMC5983 with AK fallback

    // ─── Barometer (BMP280 or BMP390, I2C) ─────────────────

    std::string read_baro(NavBaroData& out);

    // ─── ADC (ADS1115, I2C) ────────────────────────────────

    std::string read_adc(int channel, float& volts);
    std::string read_adc_all(NavADCData& out);

    // ─── Leak Detector (GPIO) ──────────────────────────────

    std::string read_leak(bool& detected);

    // ─── PWM (PCA9685, I2C + GPIO OE pin) ──────────────────

    std::string pwm_enable(bool enable);
    std::string pwm_set_frequency(float freq_hz);
    std::string pwm_set_pulse_us(int channel, float pulse_us);  // e.g., 1500 = center servo
    std::string pwm_set_duty(int channel, float duty);           // 0.0 to 1.0
    std::string pwm_set_raw(int channel, uint16_t on, uint16_t off);

    // ─── User LEDs (GPIO, active low) ──────────────────────

    std::string led_set(int led, bool on);
    std::string led_get(int led, bool& on);

    // ─── NeoPixel (SK6812 via SPI) ─────────────────────────

    std::string neopixel_set(const uint8_t (*rgb)[3], int count);
    std::string neopixel_set_rgbw(const uint8_t (*rgbw)[4], int count);
    std::string neopixel_clear();

    // ─── Attitude Estimation ───────────────────────────────

    void ahrs_set_gains(double Kp, double Ti, double KpQuick, double TiQuick);
    void ahrs_set_mag_calib(double mx, double my, double mz);
    std::string ahrs_update(double dt,
                            double gx, double gy, double gz,
                            double ax, double ay, double az,
                            double mx = 0.0, double my = 0.0, double mz = 0.0);
    std::string ahrs_get_attitude(NavAttitudeData& out);
    void ahrs_get_gyro_bias(double& bx, double& by, double& bz);
    void ahrs_set_gyro_bias(double bx, double by, double bz);
    void ahrs_reset(bool quick_learn = true, bool reset_gyro_bias = true);

private:
    struct Impl;
    Impl* m_impl;
};
