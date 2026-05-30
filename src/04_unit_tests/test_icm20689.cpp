#include "nav_bindings.h"
#include <cstdio>
#include <cmath>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    NavAxisData accel, gyro;
    std::string err = nav.read_accel(accel);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
    err = nav.read_gyro(gyro);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

    printf("Accel: x=%.3f y=%.3f z=%.3f m/s^2\n", accel.x, accel.y, accel.z);
    printf("Gyro:  x=%.4f y=%.4f z=%.4f rad/s\n", gyro.x, gyro.y, gyro.z);

    float mag = sqrtf(accel.x*accel.x + accel.y*accel.y + accel.z*accel.z);
    printf("Accel magnitude: %.3f m/s^2\n", mag);
    if (mag < 7.0f || mag > 13.0f) { fprintf(stderr, "FAIL: accel magnitude range\n"); return 1; }

    printf("PASS\n");
    return 0;
}
