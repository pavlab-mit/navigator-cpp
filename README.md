# navigator-cpp

Pure C++ hardware interface library for the [BlueRobotics Navigator](https://bluerobotics.com/store/comm-control-power/control/navigator/) flight controller.

Replaces the Rust `navigator-rs` / `navigator-lib` stack with direct Linux I2C, SPI, and GPIO access. Handles Navigator V1 (BMP280) and V2 (BMP390) boards, and Raspberry Pi 4 and Pi 5, transparently at runtime.

## Sensors

| Chip | Interface | Function |
|------|-----------|----------|
| BMP280 (V1) / BMP390 (V2) | I2C | Barometer + temperature |
| ICM20689 | SPI | 6-axis IMU (accel + gyro) |
| AK09915 | I2C | Magnetometer |
| MMC5983MA | I2C | Magnetometer |
| ADS1115 | I2C | 4-channel 16-bit ADC |
| PCA9685 | I2C | 16-channel PWM |
| SK6812 | SPI | NeoPixel RGB LED strip |
| GPIO | GPIO | 3 user LEDs, leak detector |

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Test (on vehicle with Navigator connected)

```bash
./scripts/run_tests.sh
```

## License

MIT. Vendored Bosch BMP3 SensorAPI is BSD-3-Clause.
