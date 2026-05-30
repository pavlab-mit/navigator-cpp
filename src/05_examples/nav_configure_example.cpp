// Sensor configuration: change ranges, rates, and filters after init.

#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    nav.init();

    // IMU: wider range for dynamic environments
    ICM_Config imu;
    imu.accel_range = ICM_ACCEL_8G;
    imu.gyro_range  = ICM_GYRO_500DPS;
    imu.gyro_dlpf   = ICM_DLPF_41HZ;
    nav.configure_imu(imu);

    // Barometer: high precision, hardware filtered
    BARO_Config baro;
    baro.press_os = BARO_OS_16X;
    baro.iir      = BARO_IIR_COEFF_7;
    baro.odr      = BARO_ODR_25HZ;
    nav.configure_baro(baro);

    // Magnetometer: lower rate
    AK09915_Config ak;
    ak.mode = AK_CONT_50HZ;
    nav.configure_ak09915(ak);

    // ADC: tighter gain for better resolution
    ADS1115_Config adc;
    adc.gain = ADC_GAIN_2048MV;
    nav.configure_adc(adc);

    // Read with new settings
    NavAxisData accel;
    NavBaroData baro_data;
    nav.read_accel(accel);
    nav.read_baro(baro_data);

    printf("Accel (8g range): %+.3f %+.3f %+.3f m/s2\n", accel.x, accel.y, accel.z);
    printf("Baro (16x OS):    %.3f kPa  %.2f C\n", baro_data.pressure_kpa, baro_data.temperature_c);

    nav.shutdown();
    return 0;
}
