# navigator-cpp

Pure C++ hardware interface library for the [BlueRobotics Navigator](https://bluerobotics.com/store/comm-control-power/control/navigator/) flight controller.

Replaces the Rust `navigator-rs` / `navigator-lib` stack with direct Linux I2C, SPI, and GPIO access. Handles Navigator V1 (BMP280) and V2 (BMP390), and Raspberry Pi 4 and Pi 5, transparently at runtime via auto-detection.

## Sensors

| Chip | Interface | Function | Config |
|------|-----------|----------|--------|
| ICM-20602 / ICM-20689 | SPI | 6-axis IMU (accel + gyro) | Range, DLPF, sample rate |
| AK09915 | I2C | 3-axis magnetometer | Continuous mode rate |
| MMC5983MA | SPI | 3-axis magnetometer | Bandwidth, rate, auto SET/RESET |
| BMP280 (V1) / BMP390 (V2) | I2C | Barometer + temperature | Oversampling, ODR, IIR filter |
| ADS1115 | I2C | 4-channel 16-bit ADC | Gain, data rate |
| PCA9685 | I2C | 16-channel PWM (24.576 MHz ext clk) | Frequency, pulse width in us |
| SK6812 | SPI | NeoPixel RGB/RGBW LED strip | — |
| SBUS | UART | RC receiver input (16 channels) | — |
| GPIO | GPIO | 3 user LEDs, leak detector | — |

## Dependencies

**Required on Raspberry Pi (vehicle):**

```bash
sudo apt-get install libgpiod-dev cmake build-essential
```

**On Mac/Linux (development):** Only `cmake` and a C++17 compiler. The library compiles with platform stubs — hardware calls return errors at runtime but everything links cleanly.

## Build

```bash
./build.sh                          # Library + navigator_test + test_rc
./build.sh -r                       # Release build
./build.sh -d                       # Debug build
./build.sh --unit_tests             # Also build sensor unit tests
./build.sh --examples               # Also build usage examples
./build.sh --unit_tests --examples  # Everything
```

Default build outputs:
- `lib/libnavigator_cpp.a` — static library
- `bin/navigator_test` — interactive sensor dashboard with AHRS + host health
- `bin/test_rc` — SBUS RC receiver diagnostic tool

## Install

Install the static library and header to `/usr/local` so other projects can link against it:

```bash
./build.sh -r
sudo ./install.sh
```

This installs:
- `/usr/local/lib/libnavigator_cpp.a`
- `/usr/local/include/nav_bindings.h`

To uninstall:

```bash
sudo ./uninstall.sh
```

## Linking From Another Project

In your top-level `CMakeLists.txt` (e.g., `moos-ivp-blueboat`):

```cmake
find_library(NAVIGATOR_CPP navigator_cpp)
find_path(NAVIGATOR_CPP_INCLUDE nav_bindings.h)

if(NAVIGATOR_CPP AND NAVIGATOR_CPP_INCLUDE)
    message(STATUS "Found navigator-cpp: ${NAVIGATOR_CPP}")
    include_directories(${NAVIGATOR_CPP_INCLUDE})
else()
    message(WARNING "navigator-cpp not found -- Navigator apps will not be built")
endif()
```

Then in any app's `CMakeLists.txt`:

```cmake
add_executable(pSensor_Interface SensorInterface.cpp)
target_link_libraries(pSensor_Interface ${NAVIGATOR_CPP})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(pSensor_Interface gpiod)
endif()
```

## Clean

```bash
./clean.sh
```

## Test

Run all sensor unit tests on a vehicle with the Navigator connected:

```bash
./scripts/run_tests.sh
```

Run the interactive dashboard:

```bash
sudo ./bin/navigator_test
```

The dashboard shows real-time sensor readings, 6-axis attitude estimation, gyro bias calibration, PSU voltage/current, and host health (per-core CPU, RAM, disk, thermals, throttle status).

Test the RC receiver:

```bash
sudo ./bin/test_rc
```

## Usage

### Basic

```cpp
#include "nav_bindings.h"

Navigator nav;
std::string err = nav.init();  // Auto-detects Nav V1/V2 and Pi 4/5
if (!err.empty()) {
    // err contains warnings for sensors that failed to init.
    // Sensors that initialized OK are still usable.
}

NavAxisData accel;
err = nav.read_accel(accel);
if (err.empty()) {
    printf("Accel: %.3f %.3f %.3f m/s^2\n", accel.x, accel.y, accel.z);
}

nav.shutdown();
```

### Sensor Configuration

Each sensor has a config struct with enums for every hardware option. Defaults are applied at `init()`. Call `configure_*()` after init to change settings.

```cpp
ICM_Config imu_cfg;
imu_cfg.accel_range = ICM_ACCEL_8G;
imu_cfg.gyro_range  = ICM_GYRO_1000DPS;
imu_cfg.gyro_dlpf   = ICM_DLPF_92HZ;
nav.configure_imu(imu_cfg);

BARO_Config baro_cfg;
baro_cfg.press_os = BARO_OS_16X;
baro_cfg.iir      = BARO_IIR_COEFF_7;
baro_cfg.odr      = BARO_ODR_25HZ;
nav.configure_baro(baro_cfg);

AK09915_Config ak_cfg;
ak_cfg.mode = AK_CONT_50HZ;
nav.configure_ak09915(ak_cfg);
```

### PWM (Servos / ESCs)

```cpp
nav.pwm_set_frequency(50.0f);     // 50 Hz for servos
nav.pwm_enable(true);
nav.pwm_set_pulse_us(0, 1500.0f); // Channel 0, 1500 us = center
nav.pwm_set_pulse_us(0, 1100.0f); // Full reverse
nav.pwm_set_pulse_us(0, 1900.0f); // Full forward
```

### Attitude Estimation

Built-in attitude filter (vendored AIS-Bonn passive complementary filter with gyro bias estimation):

```cpp
nav.ahrs_set_mag_calib(0, 0, 0);  // No mag, 6-axis only
nav.ahrs_set_gains(2.2, 2.65, 10.0, 1.25);
nav.ahrs_reset(true, true);

// In your loop at 150 Hz:
NavAxisData accel, gyro;
nav.read_accel(accel);
nav.read_gyro(gyro);
nav.ahrs_update(dt, gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);

NavAttitudeData att;
nav.ahrs_get_attitude(att);
// att.roll, att.pitch, att.yaw in radians
// att.qw, att.qx, att.qy, att.qz quaternion
```

### Error Handling

All methods return `std::string` — empty on success, descriptive message on failure. No method will crash even if hardware is absent.

```cpp
std::string err = nav.read_baro(baro);
if (!err.empty()) {
    // err = "read_baro: barometer not initialized"
    // Handle gracefully — other sensors still work
}
```

## Project Structure

```
navigator-cpp/
├── build.sh / clean.sh / install.sh / uninstall.sh
├── CMakeLists.txt
├── include/
│   └── nav_bindings.h              Public API (Navigator class, config structs, enums)
├── src/
│   ├── 00_lib_nav/                 Navigator class implementation
│   ├── 01_sensors/                 Sensor drivers
│   │   ├── ads1115/
│   │   ├── ak09915/
│   │   ├── bmp280/
│   │   ├── bmp390/
│   │   ├── icm20689/
│   │   └── mmc5983/
│   ├── 02_interfaces/              Bus and peripheral drivers
│   │   ├── gpio/
│   │   ├── i2c/
│   │   ├── leak/
│   │   ├── led/
│   │   ├── neopixel/
│   │   ├── pca9685/
│   │   ├── sbus/
│   │   └── spi/
│   ├── 03_vendored/                Third-party libraries
│   │   ├── BMP3_SensorAPI/         Bosch BMP390 C driver (BSD-3-Clause)
│   │   └── attitude_estimator/     AIS-Bonn attitude filter (BSD-3-Clause)
│   ├── 04_unit_tests/              Per-sensor hardware tests (--unit_tests)
│   ├── 05_examples/                Usage examples (--examples)
│   ├── 06_navigator_test/          Interactive sensor dashboard (always built)
│   └── 07_test_rc/                 SBUS RC receiver test (always built)
├── lib/                            Build output: libnavigator_cpp.a
├── bin/                            Build output: binaries
└── scripts/
    └── run_tests.sh                Automated unit test runner
```

## License

MIT. Vendored components retain their original licenses (BSD-3-Clause).
