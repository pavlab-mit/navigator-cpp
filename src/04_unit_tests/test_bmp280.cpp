#include "nav_bindings.h"
#include <cstdio>

#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

static bool read_register(int fd, unsigned char reg, unsigned char& value) {
    return ::write(fd, &reg, 1) == 1 && ::read(fd, &value, 1) == 1;
}
#endif

int main() {
    Navigator nav;
    nav.init();
    if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }
    if (nav.detected_version() != NAV_V1) return 0;  // Not this hardware, not a failure

#ifdef __linux__
    int fd = ::open("/dev/i2c-1", O_RDWR);
    if (fd < 0 || ::ioctl(fd, I2C_SLAVE, 0x76) < 0) {
        fprintf(stderr, "FAIL: BMP280 register probe: %s\n", std::strerror(errno));
        if (fd >= 0) ::close(fd);
        return 1;
    }
    unsigned char ctrl_meas = 0;
    unsigned char config = 0;
    if (!read_register(fd, 0xF4, ctrl_meas) || !read_register(fd, 0xF5, config)) {
        fprintf(stderr, "FAIL: BMP280 configuration readback\n");
        ::close(fd);
        return 1;
    }
    ::close(fd);
    if (ctrl_meas != 0x57 || config != 0x10) {
        fprintf(stderr, "FAIL: BMP280 config ctrl_meas=0x%02X config=0x%02X\n",
                ctrl_meas, config);
        return 1;
    }
#endif

    NavBaroData baro;
    std::string err = nav.read_baro(baro);
    if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }

    printf("Pressure: %.2f kPa, Temperature: %.2f C\n", baro.pressure_kpa, baro.temperature_c);
    // BMP280 range: 30-110 kPa per datasheet. Values below 80 kPa are valid (e.g., vacuum preload on o-rings).
    if (baro.pressure_kpa < 1.0f || baro.pressure_kpa > 120.0f) { fprintf(stderr, "FAIL: pressure range\n"); return 1; }
    if (baro.temperature_c < -40.0f || baro.temperature_c > 85.0f) { fprintf(stderr, "FAIL: temp range\n"); return 1; }

    printf("PASS\n");
    return 0;
}
