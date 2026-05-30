#include "nav_bindings.h"
#include <cstdio>
#include <unistd.h>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    const char* names[3] = {"Red", "Green", "Blue"};
    uint8_t colors[3][3] = {{255,0,0}, {0,255,0}, {0,0,255}};

    for (int i = 0; i < 3; i++) {
        printf("NeoPixel: %s\n", names[i]);
        uint8_t rgb[1][3] = {{colors[i][0], colors[i][1], colors[i][2]}};
        std::string err = nav.neopixel_set(rgb, 1);
        if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
        usleep(300000);
    }

    nav.neopixel_clear();
    printf("PASS\n");
    return 0;
}
