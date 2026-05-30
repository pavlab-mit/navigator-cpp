#include "nav_bindings.h"
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <csignal>
#include <atomic>
#include <sys/statvfs.h>

static std::atomic<bool> s_running{true};

static void signal_handler(int) {
    s_running = false;
}

static double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// ─── Host health ───────────────────────────────────────────────

#define MAX_CORES 8

struct HostHealth {
    float cpu_temp_c     = -1.0f;
    float core_volt      = -1.0f;
    float cpu_mhz        = -1.0f;
    int   num_cores      = 0;
    float core_usage[MAX_CORES] = {};
    float mem_used_mb    = -1.0f;
    float mem_total_mb   = -1.0f;
    float disk_used_gb   = -1.0f;
    float disk_total_gb  = -1.0f;
    float uptime_sec     = -1.0f;
    bool  under_voltage  = false;
    bool  freq_capped    = false;
    bool  throttled      = false;
};

static float read_sysfs_float(const char* path, float scale) {
    FILE* f = fopen(path, "r");
    if (!f) return -1.0f;
    long val = 0;
    if (fscanf(f, "%ld", &val) != 1) val = 0;
    fclose(f);
    return val / scale;
}

static float read_vcgencmd_float(const char* cmd, const char* fmt) {
    FILE* f = popen(cmd, "r");
    if (!f) return -1.0f;
    char buf[64] = {};
    if (!fgets(buf, sizeof(buf), f)) { pclose(f); return -1.0f; }
    pclose(f);
    float v = -1.0f;
    sscanf(buf, fmt, &v);
    return v;
}

static float read_vcgencmd_clock(const char* id) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "vcgencmd measure_clock %s 2>/dev/null", id);
    FILE* f = popen(cmd, "r");
    if (!f) return -1.0f;
    char buf[64] = {};
    if (!fgets(buf, sizeof(buf), f)) { pclose(f); return -1.0f; }
    pclose(f);
    char* eq = strchr(buf, '=');
    if (!eq) return -1.0f;
    return atoll(eq + 1) / 1e6f;
}

static long s_prev_idle[MAX_CORES] = {};
static long s_prev_total[MAX_CORES] = {};

static int compute_per_core_usage(float usage_out[]) {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return 0;
    char line[256];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    int core = 0;
    while (fgets(line, sizeof(line), f) && core < MAX_CORES) {
        if (strncmp(line, "cpu", 3) != 0) break;
        long user, nice, system, idle, iowait, irq, softirq, steal;
        if (sscanf(line + 3, "%*d %ld %ld %ld %ld %ld %ld %ld %ld",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 4)
            break;
        long total = user + nice + system + idle + iowait + irq + softirq + steal;
        long idle_all = idle + iowait;
        usage_out[core] = -1.0f;
        if (s_prev_total[core] > 0) {
            long dt = total - s_prev_total[core];
            long di = idle_all - s_prev_idle[core];
            if (dt > 0) usage_out[core] = 100.0f * (1.0f - (float)di / (float)dt);
        }
        s_prev_idle[core] = idle_all;
        s_prev_total[core] = total;
        core++;
    }
    fclose(f);
    return core;
}

static HostHealth read_host_health() {
    HostHealth h;
    h.cpu_temp_c  = read_sysfs_float("/sys/class/thermal/thermal_zone0/temp", 1000.0f);
    h.core_volt   = read_vcgencmd_float("vcgencmd measure_volts core 2>/dev/null", "volt=%fV");
    h.cpu_mhz     = read_vcgencmd_clock("arm");
    h.num_cores   = compute_per_core_usage(h.core_usage);
    h.uptime_sec  = read_sysfs_float("/proc/uptime", 1.0f);

    FILE* f = fopen("/proc/meminfo", "r");
    if (f) {
        float total = 0, avail = 0;
        char line[128];
        while (fgets(line, sizeof(line), f)) {
            long val;
            if (sscanf(line, "MemTotal: %ld kB", &val) == 1) total = val / 1024.0f;
            if (sscanf(line, "MemAvailable: %ld kB", &val) == 1) avail = val / 1024.0f;
        }
        fclose(f);
        h.mem_total_mb = total;
        h.mem_used_mb = total - avail;
    }

    struct statvfs st;
    if (statvfs("/", &st) == 0) {
        double total = (double)st.f_blocks * (double)st.f_frsize;
        double avail = (double)st.f_bavail * (double)st.f_frsize;
        h.disk_total_gb = (float)(total / 1e9);
        h.disk_used_gb  = (float)((total - avail) / 1e9);
    }

    f = popen("vcgencmd get_throttled 2>/dev/null", "r");
    if (f) {
        char buf[64] = {};
        if (fgets(buf, sizeof(buf), f)) {
            unsigned int flags = 0;
            sscanf(buf, "throttled=0x%x", &flags);
            h.under_voltage = (flags & 0x1) != 0;
            h.freq_capped   = (flags & 0x2) != 0;
            h.throttled     = (flags & 0x4) != 0;
        }
        pclose(f);
    }
    return h;
}

static void format_uptime(float sec, char* buf, int len) {
    if (sec < 0) { snprintf(buf, len, "--"); return; }
    int s = (int)sec;
    int d = s / 86400; s %= 86400;
    int h = s / 3600;  s %= 3600;
    int m = s / 60;    s %= 60;
    if (d > 0)
        snprintf(buf, len, "%dd %02d:%02d:%02d", d, h, m, s);
    else
        snprintf(buf, len, "%02d:%02d:%02d", h, m, s);
}

static void render_bar(char* buf, int width, float pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int filled = (int)(pct / 100.0f * width);
    buf[0] = '[';
    for (int i = 0; i < width; i++)
        buf[1 + i] = (i < filled) ? '#' : ' ';
    buf[1 + width] = ']';
    buf[2 + width] = '\0';
}

// ─── PSU conversion (BlueRobotics PSM) ─────────────────────────
// Current offset/gain varies with battery count.
// Defaults estimated from PSM docs + bench calibration.

struct PSUCoeffs {
    int cells;
    float current_offset;  // V at zero current
    float current_scale;   // A per V
    float voltage_scale;   // V per V (resistor divider)
};

static const PSUCoeffs PSU_TABLE[] = {
    {1, 0.330f, 37.88f, 11.13f},
    {2, 0.330f, 37.88f, 11.13f},
    {3, 0.325f, 37.88f, 11.13f},
    {4, 0.324f, 37.88f, 11.13f},
    {5, 0.322f, 37.88f, 11.13f},
    {6, 0.320f, 37.88f, 11.13f},
    {7, 0.318f, 37.88f, 11.13f},
    {8, 0.316f, 37.88f, 11.13f},
};
static const int PSU_TABLE_SIZE = sizeof(PSU_TABLE) / sizeof(PSU_TABLE[0]);

// ─── Main ──────────────────────────────────────────────────────

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Navigator nav;
    std::string warnings = nav.init();

    if (!warnings.empty()) {
        fprintf(stderr, "Init warnings:\n%s\n", warnings.c_str());
    }
    if (!nav.is_initialized()) {
        fprintf(stderr, "Navigator failed to initialize\n");
        return 1;
    }

    // Gyro bias calibration
    const int cal_seconds = 3;
    const int cal_hz = 150;
    double bias_x = 0.0, bias_y = 0.0, bias_z = 0.0;
    int good_samples = 0;

    for (int s = cal_seconds; s > 0; s--) {
        printf("\rHold still... %d ", s);
        fflush(stdout);
        for (int i = 0; i < cal_hz; i++) {
            NavAxisData gyro;
            if (nav.read_gyro(gyro).empty()) {
                bias_x += gyro.x;
                bias_y += gyro.y;
                bias_z += gyro.z;
                good_samples++;
            }
            usleep(1000000 / cal_hz);
        }
    }

    if (good_samples > 0) {
        bias_x /= good_samples;
        bias_y /= good_samples;
        bias_z /= good_samples;
    }

    // Prime CPU usage
    float dummy[MAX_CORES];
    compute_per_core_usage(dummy);
    usleep(100000);

    // AHRS setup
    nav.ahrs_set_mag_calib(0.0, 0.0, 0.0);
    nav.ahrs_set_gains(2.2, 2.65, 10.0, 1.25);
    nav.ahrs_reset(true, true);
    nav.ahrs_set_gyro_bias(bias_x, bias_y, bias_z);

    // Dashboard
    printf("\033[2J");
    const double display_dt = 0.2;
    const double sensor_dt = 1.0 / 150.0;
    double last_display = 0.0;
    double rainbow_phase = 0.0;

    while (s_running) {
        double t0 = now_sec();

        NavAxisData accel, gyro, mag_ak, mag_mmc;
        NavBaroData baro;
        NavADCData adc;
        NavAttitudeData att;
        bool leak = false;

        std::string e_acc  = nav.read_accel(accel);
        std::string e_gyr  = nav.read_gyro(gyro);
        std::string e_mak  = nav.read_mag_ak09915(mag_ak);
        std::string e_mmc  = nav.read_mag_mmc5983(mag_mmc);
        std::string e_bar  = nav.read_baro(baro);
        std::string e_adc  = nav.read_adc_all(adc);
        std::string e_leak = nav.read_leak(leak);

        double gx = gyro.x - bias_x;
        double gy = gyro.y - bias_y;
        double gz = gyro.z - bias_z;
        nav.ahrs_update(sensor_dt, gx, gy, gz, accel.x, accel.y, accel.z);
        nav.ahrs_get_attitude(att);

        rainbow_phase += sensor_dt * 0.5;
        if (rainbow_phase > 1.0) rainbow_phase -= 1.0;
        uint8_t rgb[1][3] = {{
            (uint8_t)((sin(rainbow_phase * 2.0 * M_PI)          * 0.5 + 0.5) * 255.0),
            (uint8_t)((sin((rainbow_phase + 0.33) * 2.0 * M_PI) * 0.5 + 0.5) * 255.0),
            (uint8_t)((sin((rainbow_phase + 0.67) * 2.0 * M_PI) * 0.5 + 0.5) * 255.0)
        }};
        nav.neopixel_set(rgb, 1);

        double now = now_sec();
        if (now - last_display < display_dt) {
            double remaining = sensor_dt - (now_sec() - t0);
            if (remaining > 0) usleep((useconds_t)(remaining * 1e6));
            continue;
        }
        last_display = now;

        HostHealth hh = read_host_health();

        double roll_d  = att.roll  * 180.0 / M_PI;
        double pitch_d = att.pitch * 180.0 / M_PI;
        double yaw_d   = att.yaw   * 180.0 / M_PI;

        char uptime_str[32];
        format_uptime(hh.uptime_sec, uptime_str, sizeof(uptime_str));

        // PSU from ADC
        float adc_v = 0, adc_i = 0;
        bool psu_valid = false;
        if (e_adc.empty()) {
            adc_v = adc.channel[3];
            adc_i = adc.channel[2];
            psu_valid = (adc_v > 0.1f);  // Something connected
        }
        float psu_voltage = adc_v * PSU_TABLE[3].voltage_scale;  // Default 4S scaling for voltage display

        // Bars
        char mem_bar[22], disk_bar[22];
        float mem_pct  = (hh.mem_total_mb  > 0) ? 100.0f * hh.mem_used_mb  / hh.mem_total_mb  : 0;
        float disk_pct = (hh.disk_total_gb > 0) ? 100.0f * hh.disk_used_gb / hh.disk_total_gb : 0;
        render_bar(mem_bar,  16, mem_pct);
        render_bar(disk_bar, 16, disk_pct);

        printf("\033[H");

        printf("navigator-cpp                            Nav V%d  Pi %d\n",
               nav.detected_version() == NAV_V1 ? 1 : 2,
               nav.detected_pi() == PI_4 ? 4 : 5);
        printf("─────────────────────────────────────────────────────\n");

        printf("  Roll  %+8.2f    Pitch %+8.2f    Yaw  %+8.2f  deg\n", roll_d, pitch_d, yaw_d);
        printf("─────────────────────────────────────────────────────\n");

        if (e_acc.empty())
            printf("  Accel  %+8.3f  %+8.3f  %+8.3f        m/s2\n", accel.x, accel.y, accel.z);
        else
            printf("  Accel  --\n");

        if (e_gyr.empty())
            printf("  Gyro   %+8.4f  %+8.4f  %+8.4f       rad/s\n", gyro.x, gyro.y, gyro.z);
        else
            printf("  Gyro   --\n");

        printf("  Bias   %+8.5f  %+8.5f  %+8.5f       rad/s\n", bias_x, bias_y, bias_z);
        printf("─────────────────────────────────────────────────────\n");

        if (e_mak.empty())
            printf("  AK09915  %+8.2f  %+8.2f  %+8.2f        uT\n", mag_ak.x, mag_ak.y, mag_ak.z);
        else
            printf("  AK09915  --\n");

        if (e_mmc.empty())
            printf("  MMC5983  %+8.2f  %+8.2f  %+8.2f        uT\n", mag_mmc.x, mag_mmc.y, mag_mmc.z);
        else
            printf("  MMC5983  --\n");

        printf("─────────────────────────────────────────────────────\n");

        if (e_bar.empty())
            printf("  Baro   %7.2f kPa   %5.1f C          Pi %5.1f C\n",
                   baro.pressure_kpa, baro.temperature_c, hh.cpu_temp_c);
        else
            printf("  Baro   --                              Pi %5.1f C\n", hh.cpu_temp_c);

        printf("─────────────────────────────────────────────────────\n");

        // PSU with per-cell-count current estimates
        if (psu_valid) {
            printf("  PSU    VBat: %5.2f V\n", psu_voltage);
            printf("  Current estimates by cell count:\n");
            for (int i = 0; i < PSU_TABLE_SIZE; i += 4) {
                printf("  ");
                for (int j = i; j < i + 4 && j < PSU_TABLE_SIZE; j++) {
                    float current = (adc_i - PSU_TABLE[j].current_offset) * PSU_TABLE[j].current_scale;
                    if (current < 0.0f) current = 0.0f;
                    printf(" %dS:%6.2fA", PSU_TABLE[j].cells, current);
                }
                printf("\n");
            }
        } else {
            printf("  PSU    --  (no PSM connected)\n");
        }

        if (e_leak.empty())
            printf("  Leak   %s\n", leak ? "** DETECTED **" : "none");
        else
            printf("  Leak   --\n");

        printf("─────────────────────────────────────────────────────\n");

        // Per-core CPU
        for (int c = 0; c < hh.num_cores && c < MAX_CORES; c += 2) {
            char bar0[14], bar1[14];
            float p0 = (hh.core_usage[c] >= 0) ? hh.core_usage[c] : 0.0f;
            render_bar(bar0, 10, p0);
            if (c + 1 < hh.num_cores) {
                float p1 = (hh.core_usage[c+1] >= 0) ? hh.core_usage[c+1] : 0.0f;
                render_bar(bar1, 10, p1);
                printf("  CPU%d %s %3.0f%%    CPU%d %s %3.0f%%\n",
                       c, bar0, p0, c+1, bar1, p1);
            } else {
                printf("  CPU%d %s %3.0f%%\n", c, bar0, p0);
            }
        }
        printf("  %4.0f MHz   %5.3fV\n", hh.cpu_mhz, hh.core_volt);
        printf("  RAM  %s  %4.0f / %4.0f MB\n", mem_bar, hh.mem_used_mb, hh.mem_total_mb);
        printf("  Disk %s  %4.1f / %4.1f GB\n", disk_bar, hh.disk_used_gb, hh.disk_total_gb);

        if (hh.under_voltage || hh.freq_capped || hh.throttled)
            printf("  Up %s  !! %s%s%s\n", uptime_str,
                   hh.under_voltage ? "UNDER-VOLT " : "",
                   hh.freq_capped   ? "FREQ-CAP " : "",
                   hh.throttled     ? "THROTTLED" : "");
        else
            printf("  Up %s\n", uptime_str);

        printf("─────────────────────────────────────────────────────\n");
        printf("  Ctrl+C to exit\n");

        double remaining = sensor_dt - (now_sec() - t0);
        if (remaining > 0) usleep((useconds_t)(remaining * 1e6));
    }

    nav.neopixel_clear();
    printf("\n");
    nav.shutdown();
    return 0;
}
