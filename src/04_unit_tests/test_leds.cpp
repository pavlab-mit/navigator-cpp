#include "nav_bindings.h"
#include <cstdio>
#include <unistd.h>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    const char* names[3] = {"Red", "Green", "Blue"};
    for (int i = 0; i < 3; i++) {
        printf("LED %d (%s) ON\n", i, names[i]);
        std::string err = nav.led_set(i, true);
        if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
        usleep(500000);
        nav.led_set(i, false);
        usleep(500000);
    }
    printf("PASS\n");
    return 0;
}
