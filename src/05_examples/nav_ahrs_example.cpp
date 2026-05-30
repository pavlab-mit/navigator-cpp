// Attitude estimation: gyro bias calibration + AHRS loop.

#include "nav_bindings.h"
#include <cstdio>
#include <cmath>
#include <unistd.h>

int main() {
    Navigator nav;
    nav.init();

    // Calibrate gyro bias (hold still)
    double bx = 0, by = 0, bz = 0;
    int n = 0;
    for (int i = 0; i < 450; i++) {
        NavAxisData g;
        if (nav.read_gyro(g).empty()) {
            bx += g.x; by += g.y; bz += g.z; n++;
        }
        usleep(6667);
    }
    if (n > 0) { bx /= n; by /= n; bz /= n; }

    // Setup AHRS (no mag)
    nav.ahrs_set_mag_calib(0, 0, 0);
    nav.ahrs_reset(true, true);
    nav.ahrs_set_gyro_bias(bx, by, bz);

    // Run for 10 seconds
    double dt = 1.0 / 150.0;
    for (int i = 0; i < 1500; i++) {
        NavAxisData accel, gyro;
        nav.read_accel(accel);
        nav.read_gyro(gyro);

        nav.ahrs_update(dt, gyro.x - bx, gyro.y - by, gyro.z - bz,
                         accel.x, accel.y, accel.z);

        if (i % 150 == 0) {
            NavAttitudeData att;
            nav.ahrs_get_attitude(att);
            printf("Roll: %+7.2f  Pitch: %+7.2f  Yaw: %+7.2f\n",
                   att.roll * 180.0 / M_PI,
                   att.pitch * 180.0 / M_PI,
                   att.yaw * 180.0 / M_PI);
        }
        usleep((useconds_t)(dt * 1e6));
    }

    nav.shutdown();
    return 0;
}
