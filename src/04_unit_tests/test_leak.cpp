#include "nav_bindings.h"
#include <cstdio>

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }

    bool detected;
    std::string err = nav.read_leak(detected);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

    printf("Leak: %s\nPASS\n", detected ? "DETECTED" : "none");
    return 0;
}
