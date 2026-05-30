// LEDs and NeoPixel: toggle user LEDs, rainbow on NeoPixel.

#include "nav_bindings.h"
#include <cstdio>
#include <cmath>
#include <unistd.h>

int main() {
    Navigator nav;
    nav.init();

    // Blink each user LED
    const char* names[] = {"Red", "Green", "Blue"};
    for (int i = 0; i < 3; i++) {
        printf("%s ON\n", names[i]);
        nav.led_set(i, true);
        usleep(300000);
        nav.led_set(i, false);
    }

    // Rainbow on NeoPixel
    for (int i = 0; i < 200; i++) {
        float t = (float)i / 200.0f;
        uint8_t rgb[1][3] = {{
            (uint8_t)((sin(t * 2.0 * M_PI)          * 0.5 + 0.5) * 255.0),
            (uint8_t)((sin((t + 0.33) * 2.0 * M_PI) * 0.5 + 0.5) * 255.0),
            (uint8_t)((sin((t + 0.67) * 2.0 * M_PI) * 0.5 + 0.5) * 255.0)
        }};
        nav.neopixel_set(rgb, 1);
        usleep(10000);
    }
    nav.neopixel_clear();

    nav.shutdown();
    return 0;
}
