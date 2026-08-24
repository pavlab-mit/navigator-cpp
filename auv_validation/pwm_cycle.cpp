#include "nav_bindings.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <thread>
#include <unistd.h>

#ifdef __linux__
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#endif

namespace {

struct Options {
    int channel = 15;
    float frequency_hz = 100.0f;
    float pulse_us = 1500.0f;
    int hold_ms = 1000;
    bool abrupt_exit = false;
};

void usage(const char* argv0) {
    std::fprintf(stderr,
        "Usage: %s [--channel=N] [--frequency=HZ] [--pulse=US] "
        "[--hold-ms=MS] [--abrupt-exit]\n", argv0);
}

bool parse_int(const char* text, int& value) {
    char* end = nullptr;
    errno = 0;
    long parsed = std::strtol(text, &end, 10);
    if (errno || !end || *end != '\0') return false;
    value = static_cast<int>(parsed);
    return true;
}

bool parse_float(const char* text, float& value) {
    char* end = nullptr;
    errno = 0;
    float parsed = std::strtof(text, &end);
    if (errno || !end || *end != '\0') return false;
    value = parsed;
    return true;
}

bool parse_options(int argc, char** argv, Options& out) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--abrupt-exit") out.abrupt_exit = true;
        else if (arg.rfind("--channel=", 0) == 0) {
            if (!parse_int(arg.c_str() + 10, out.channel)) return false;
        } else if (arg.rfind("--frequency=", 0) == 0) {
            if (!parse_float(arg.c_str() + 12, out.frequency_hz)) return false;
        } else if (arg.rfind("--pulse=", 0) == 0) {
            if (!parse_float(arg.c_str() + 8, out.pulse_us)) return false;
        } else if (arg.rfind("--hold-ms=", 0) == 0) {
            if (!parse_int(arg.c_str() + 10, out.hold_ms)) return false;
        } else return false;
    }
    return out.channel >= 0 && out.channel <= 15 && out.hold_ms >= 0;
}

#ifdef __linux__
class RegisterProbe {
public:
    ~RegisterProbe() { if (fd_ >= 0) ::close(fd_); }

    bool open_for(PiVersion pi) {
        const char* path = (pi == PI_5) ? "/dev/i2c-3" : "/dev/i2c-4";
        fd_ = ::open(path, O_RDWR);
        if (fd_ < 0) {
            std::fprintf(stderr, "PROBE open %s failed: %s\n", path, std::strerror(errno));
            return false;
        }
        if (::ioctl(fd_, I2C_SLAVE, 0x40) < 0) {
            std::fprintf(stderr, "PROBE select 0x40 failed: %s\n", std::strerror(errno));
            return false;
        }
        return true;
    }

    int read_reg(unsigned char reg) const {
        if (fd_ < 0 || ::write(fd_, &reg, 1) != 1) return -1;
        unsigned char value = 0;
        if (::read(fd_, &value, 1) != 1) return -1;
        return value;
    }

    void print(const char* stage) const {
        const int mode1 = read_reg(0x00);
        const int mode2 = read_reg(0x01);
        const int all_off_h = read_reg(0xFD);
        const int prescale = read_reg(0xFE);
        if (mode1 < 0 || mode2 < 0 || all_off_h < 0 || prescale < 0) {
            std::fprintf(stderr, "PROBE register read failed stage=%s\n", stage);
            return;
        }
        std::printf("REGISTERS stage=%s MODE1=0x%02X MODE2=0x%02X "
                    "ALL_LED_OFF_H=0x%02X PRE_SCALE=0x%02X\n",
                    stage, mode1, mode2, all_off_h, prescale);
    }

private:
    int fd_ = -1;
};
#endif

bool require_ok(const char* operation, const std::string& error) {
    if (error.empty()) {
        std::printf("PASS %s\n", operation);
        return true;
    }
    std::fprintf(stderr, "FAIL %s: %s\n", operation, error.c_str());
    return false;
}

} // namespace

int main(int argc, char** argv) {
#ifndef __linux__
    std::fprintf(stderr, "FAIL pwm_cycle requires Linux Navigator hardware\n");
    return 2;
#else
    Options options;
    if (!parse_options(argc, argv, options)) {
        usage(argv[0]);
        return 2;
    }

    std::printf("BEGIN pid=%ld channel=%d frequency=%.3f pulse_us=%.3f "
                "hold_ms=%d exit=%s\n",
                static_cast<long>(::getpid()), options.channel,
                options.frequency_hz, options.pulse_us, options.hold_ms,
                options.abrupt_exit ? "abrupt" : "clean");

    Navigator nav;
    const std::string init_result = nav.init();
    if (!init_result.empty())
        std::fprintf(stderr, "INIT_WARNINGS\n%s", init_result.c_str());

    std::string error = nav.pwm_set_frequency(options.frequency_hz);
    if (!require_ok("pwm_set_frequency", error)) return 1;
    error = nav.pwm_set_pulse_us(options.channel, options.pulse_us);
    if (!require_ok("pwm_set_pulse_us", error)) return 1;

    RegisterProbe probe;
    if (probe.open_for(nav.detected_pi())) probe.print("configured_disabled");

    error = nav.pwm_enable(true);
    if (!require_ok("pwm_enable_true", error)) return 1;
    if (probe.read_reg(0x00) >= 0) probe.print("enabled");

    std::printf("MEASURE_NOW expected_frequency_hz=%.3f expected_pulse_us=%.3f\n",
                options.frequency_hz, options.pulse_us);
    std::this_thread::sleep_for(std::chrono::milliseconds(options.hold_ms));

    if (options.abrupt_exit) {
        std::printf("ABRUPT_EXIT skipping Navigator destructor and shutdown\n");
        std::fflush(nullptr);
        ::_exit(0);
    }

    error = nav.pwm_enable(false);
    if (!require_ok("pwm_enable_false", error)) return 1;
    nav.shutdown();
    if (probe.read_reg(0x00) >= 0) probe.print("after_shutdown");
    std::printf("PASS clean_shutdown\nEND\n");
    return 0;
#endif
}
