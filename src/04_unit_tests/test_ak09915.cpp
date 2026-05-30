#include "nav_bindings.h"
#include <cstdio>
#include <cmath>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    NavAxisData mag;
    std::string err = nav.read_mag_ak09915(mag);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

    printf("AK09915: x=%.2f y=%.2f z=%.2f uT\n", mag.x, mag.y, mag.z);
    float magnitude = sqrtf(mag.x*mag.x + mag.y*mag.y + mag.z*mag.z);
    printf("Magnitude: %.2f uT\n", magnitude);
    if (magnitude < 1.0f || magnitude > 200.0f) { fprintf(stderr, "FAIL: magnitude range\n"); return 1; }

    printf("PASS\n");
    return 0;
}
