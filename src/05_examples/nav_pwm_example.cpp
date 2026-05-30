// PWM output: set frequency and sweep a servo using microseconds.

#include "nav_bindings.h"
#include <cstdio>
#include <unistd.h>

int main() {
    Navigator nav;
    nav.init();

    nav.pwm_set_frequency(50.0f);  // 50 Hz for servos
    nav.pwm_enable(true);

    // Sweep channel 0 from 1000us to 2000us (full servo range)
    for (int us = 1000; us <= 2000; us += 50) {
        nav.pwm_set_pulse_us(0, (float)us);
        printf("Ch0: %d us\n", us);
        usleep(100000);
    }

    // Center
    nav.pwm_set_pulse_us(0, 1500.0f);
    usleep(500000);

    nav.pwm_enable(false);
    nav.shutdown();
    return 0;
}
