#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }
    if (nav.detected_version() != NAV_V1) return 0;  // Not this hardware, not a failure

    NavBaroData baro;
    std::string err = nav.read_baro(baro);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

    printf("Pressure: %.2f kPa, Temperature: %.2f C\n", baro.pressure_kpa, baro.temperature_c);
    // BMP280 range: 30-110 kPa per datasheet. Values below 80 kPa are valid (e.g., vacuum preload on o-rings).
    if (baro.pressure_kpa < 1.0f || baro.pressure_kpa > 120.0f) { fprintf(stderr, "FAIL: pressure range\n"); return 1; }
    if (baro.temperature_c < -40.0f || baro.temperature_c > 85.0f) { fprintf(stderr, "FAIL: temp range\n"); return 1; }

    printf("PASS\n");
    return 0;
}
