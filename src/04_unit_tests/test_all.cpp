#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    std::string warnings = nav.init();
    if (!warnings.empty()) {
        fprintf(stderr, "init warnings:\n%s\n", warnings.c_str());
    }
    if (!nav.is_initialized()) {
        fprintf(stderr, "FAIL: navigator not initialized\n");
        return 1;
    }

    int pass = 0, fail = 0;
    std::string err;

    printf("Navigator V%d, Pi %d\n\n",
           nav.detected_version() == NAV_V1 ? 1 : 2,
           nav.detected_pi() == PI_4 ? 4 : 5);

    NavAxisData accel;
    err = nav.read_accel(accel);
    if (err.empty()) { printf("[PASS] Accel:   %+.3f %+.3f %+.3f m/s^2\n", accel.x, accel.y, accel.z); pass++; }
    else { printf("[FAIL] Accel:   %s\n", err.c_str()); fail++; }

    NavAxisData gyro;
    err = nav.read_gyro(gyro);
    if (err.empty()) { printf("[PASS] Gyro:    %+.4f %+.4f %+.4f rad/s\n", gyro.x, gyro.y, gyro.z); pass++; }
    else { printf("[FAIL] Gyro:    %s\n", err.c_str()); fail++; }

    NavAxisData mag_ak;
    err = nav.read_mag_ak09915(mag_ak);
    if (err.empty()) { printf("[PASS] AK09915: %+.2f %+.2f %+.2f uT\n", mag_ak.x, mag_ak.y, mag_ak.z); pass++; }
    else { printf("[FAIL] AK09915: %s\n", err.c_str()); fail++; }

    NavAxisData mag_mmc;
    err = nav.read_mag_mmc5983(mag_mmc);
    if (err.empty()) { printf("[PASS] MMC5983: %+.2f %+.2f %+.2f uT\n", mag_mmc.x, mag_mmc.y, mag_mmc.z); pass++; }
    else { printf("[FAIL] MMC5983: %s\n", err.c_str()); fail++; }

    NavBaroData baro;
    err = nav.read_baro(baro);
    if (err.empty()) { printf("[PASS] Baro:    %.2f kPa, %.2f C\n", baro.pressure_kpa, baro.temperature_c); pass++; }
    else { printf("[FAIL] Baro:    %s\n", err.c_str()); fail++; }

    NavADCData adc;
    err = nav.read_adc_all(adc);
    if (err.empty()) { printf("[PASS] ADC:     %.3f %.3f %.3f %.3f V\n", adc.channel[0], adc.channel[1], adc.channel[2], adc.channel[3]); pass++; }
    else { printf("[FAIL] ADC:     %s\n", err.c_str()); fail++; }

    bool leak;
    err = nav.read_leak(leak);
    if (err.empty()) { printf("[PASS] Leak:    %s\n", leak ? "DETECTED" : "none"); pass++; }
    else { printf("[FAIL] Leak:    %s\n", err.c_str()); fail++; }

    printf("\n%d/%d sensors OK\n", pass, pass + fail);
    return (fail > 0) ? 1 : 0;
}
