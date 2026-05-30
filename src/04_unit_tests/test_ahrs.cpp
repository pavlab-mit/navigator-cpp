#include "nav_bindings.h"
#include <cstdio>
#include <cmath>
#include <unistd.h>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    // Configure AHRS — no mag correction for now
    nav.ahrs_set_mag_calib(0.0, 0.0, 0.0);
    nav.ahrs_set_gains(2.2, 2.65, 10.0, 1.25);
    nav.ahrs_reset(true, true);

    // Run 100 updates at ~150Hz to let it converge
    NavAxisData accel, gyro;
    NavAttitudeData att;
    double dt = 1.0 / 150.0;

    for (int i = 0; i < 100; i++) {
        std::string err = nav.read_accel(accel);
        if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
        err = nav.read_gyro(gyro);
        if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

        nav.ahrs_update(dt, gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);
        usleep((useconds_t)(dt * 1e6));
    }

    nav.ahrs_get_attitude(att);

    double roll_deg  = att.roll  * 180.0 / M_PI;
    double pitch_deg = att.pitch * 180.0 / M_PI;
    double yaw_deg   = att.yaw   * 180.0 / M_PI;

    printf("Roll:  %+.2f deg\n", roll_deg);
    printf("Pitch: %+.2f deg\n", pitch_deg);
    printf("Yaw:   %+.2f deg\n", yaw_deg);
    printf("Quat:  w=%.4f x=%.4f y=%.4f z=%.4f\n", att.qw, att.qx, att.qy, att.qz);

    // Just verify the filter produced valid numbers (not NaN/inf)
    if (std::isnan(roll_deg) || std::isnan(pitch_deg) || std::isnan(yaw_deg)) {
        fprintf(stderr, "FAIL: attitude contains NaN\n");
        return 1;
    }

    double bx, by, bz;
    nav.ahrs_get_gyro_bias(bx, by, bz);
    printf("Gyro bias: %.5f %.5f %.5f rad/s\n", bx, by, bz);

    printf("PASS\n");
    return 0;
}
