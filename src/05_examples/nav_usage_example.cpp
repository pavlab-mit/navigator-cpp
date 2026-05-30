// Basic usage: init, read sensors, shutdown.

#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    std::string err = nav.init();
    if (!err.empty())
        printf("Warnings: %s\n", err.c_str());

    NavAxisData accel, gyro, mag;
    NavBaroData baro;
    bool leak;

    nav.read_accel(accel);
    nav.read_gyro(gyro);
    nav.read_mag_ak09915(mag);
    nav.read_baro(baro);
    nav.read_leak(leak);

    printf("Accel:    %+.3f %+.3f %+.3f m/s2\n", accel.x, accel.y, accel.z);
    printf("Gyro:     %+.4f %+.4f %+.4f rad/s\n", gyro.x, gyro.y, gyro.z);
    printf("Mag:      %+.2f %+.2f %+.2f uT\n", mag.x, mag.y, mag.z);
    printf("Baro:     %.2f kPa  %.1f C\n", baro.pressure_kpa, baro.temperature_c);
    printf("Leak:     %s\n", leak ? "DETECTED" : "none");

    nav.shutdown();
    return 0;
}
