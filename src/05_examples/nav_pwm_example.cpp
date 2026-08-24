// PWM output: set frequency and sweep a servo using microseconds.

#include "nav_bindings.h"
#include <cstdio>
#include <unistd.h>

int main() {
    Navigator nav;
    std::string err = nav.init();
    if (!nav.is_pwm_ready()) {
        std::fprintf(stderr, "PWM unavailable:\n%s", err.c_str());
        return 1;
    }

    err = nav.pwm_set_frequency(50.0f);  // 50 Hz for servos
    if (!err.empty()) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    err = nav.pwm_enable(true);
    if (!err.empty()) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }

    // Sweep channel 0 from 1000us to 2000us (full servo range)
    for (int us = 1000; us <= 2000; us += 50) {
        err = nav.pwm_set_pulse_us(0, (float)us);
        if (!err.empty()) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        printf("Ch0: %d us\n", us);
        usleep(100000);
    }

    // Center
    err = nav.pwm_set_pulse_us(0, 1500.0f);
    if (!err.empty()) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    usleep(500000);

    err = nav.pwm_enable(false);
    if (!err.empty()) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    nav.shutdown();
    return 0;
}
