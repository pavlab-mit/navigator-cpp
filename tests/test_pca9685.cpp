#include "nav_bindings.h"
#include <cstdio>
#include <unistd.h>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    std::string err = nav.pwm_set_frequency(50.0f);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
    printf("PWM frequency set to 50 Hz\n");

    err = nav.pwm_enable(true);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
    printf("PWM enabled\n");

    err = nav.pwm_set_duty(0, 0.5f);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
    printf("Channel 0 at 50%% duty for 1s\n");
    usleep(1000000);

    nav.pwm_set_duty(0, 0.0f);
    nav.pwm_enable(false);
    printf("PWM disabled\nPASS\n");
    return 0;
}
