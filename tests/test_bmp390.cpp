#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    std::string err = nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    if (nav.detected_version() != NAV_V2) {
        printf("SKIP: not Navigator V2 (BMP390)\n");
        return 2;
    }

    NavBaroData baro;
    err = nav.read_baro(baro);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

    printf("Pressure: %.2f kPa, Temperature: %.2f C\n", baro.pressure_kpa, baro.temperature_c);
    if (baro.pressure_kpa < 80.0f || baro.pressure_kpa > 120.0f) { fprintf(stderr, "FAIL: pressure range\n"); return 1; }
    if (baro.temperature_c < -40.0f || baro.temperature_c > 85.0f) { fprintf(stderr, "FAIL: temp range\n"); return 1; }

    printf("PASS\n");
    return 0;
}
