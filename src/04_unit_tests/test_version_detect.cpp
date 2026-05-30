#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    std::string err = nav.init(NAV_AUTO, PI_AUTO);
    if (!err.empty()) {
        fprintf(stderr, "init warnings:\n%s", err.c_str());
    }
    if (!nav.is_initialized()) {
        fprintf(stderr, "FAIL: navigator not initialized\n");
        return 1;
    }

    NavVersion nv = nav.detected_version();
    PiVersion  pv = nav.detected_pi();

    printf("Navigator: V%d\n", (nv == NAV_V1) ? 1 : 2);
    printf("Raspberry Pi: %d\n", (pv == PI_4) ? 4 : 5);

    printf("PASS\n");
    return 0;
}
