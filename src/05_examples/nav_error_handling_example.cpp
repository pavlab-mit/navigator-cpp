// Error handling: every method returns "" on success or a message on failure.

#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    std::string err;

    // Init collects warnings — partial init is OK
    err = nav.init();
    if (!err.empty())
        printf("Some sensors failed:\n%s\n", err.c_str());

    // Each read is independent — one failure doesn't block others
    NavAxisData accel;
    err = nav.read_accel(accel);
    if (!err.empty())
        printf("Accel error: %s\n", err.c_str());
    else
        printf("Accel OK: %.3f %.3f %.3f\n", accel.x, accel.y, accel.z);

    NavBaroData baro;
    err = nav.read_baro(baro);
    if (!err.empty())
        printf("Baro error: %s\n", err.c_str());
    else
        printf("Baro OK: %.2f kPa\n", baro.pressure_kpa);

    // Configure on an uninitialized sensor returns an error, no crash
    MMC5983_Config mmc;
    err = nav.configure_mmc5983(mmc);
    if (!err.empty())
        printf("MMC config: %s\n", err.c_str());

    nav.shutdown();

    // Reading after shutdown returns an error, no crash
    err = nav.read_accel(accel);
    if (!err.empty())
        printf("After shutdown: %s\n", err.c_str());

    return 0;
}
