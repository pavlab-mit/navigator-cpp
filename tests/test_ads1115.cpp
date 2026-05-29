#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    NavADCData adc;
    std::string err = nav.read_adc_all(adc);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

    for (int i = 0; i < 4; i++) {
        printf("ADC Ch%d: %.4f V\n", i, adc.channel[i]);
        if (adc.channel[i] < -0.5f || adc.channel[i] > 5.0f) {
            fprintf(stderr, "FAIL: channel %d range\n", i);
            return 1;
        }
    }
    printf("PASS\n");
    return 0;
}
