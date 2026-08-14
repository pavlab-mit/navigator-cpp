// test_pca9685_restart -- reproduce / verify the alternating-launch
// ESC panic (moos-ivp-blueboat docs/rc_controllers.md section 8.3).
//
// Hypothesis under test: pca9685_init() writes SLEEP to a chip whose
// PWM channels the PREVIOUS process left running. Per the PCA9685
// datasheet that sets the RESTART-pending condition (MODE1 bit 7),
// and after wake all PWM outputs stay OFF until software writes a 1
// to that bit -- which no MODE1 write in pca9685.cpp ever does. The
// result is an enabled OE line with no pulses: ESCs panic-beep, the
// neutral-hold "arm" does nothing. The NEXT launch inherits a
// channels-off chip, sets no RESTART condition, and works. Hence
// fine / panic / fine alternation across mission launches.
//
// This program mimics one iBBNavigatorInterface launch exactly:
//   init -> pwm_set_frequency(100) -> neutral on ch14/ch16 ->
//   pwm_enable(true) -> hold -> exit WITHOUT pwm_enable(false)
// (the mission default disarm_on_exit=false: OE is released by the
// Navigator destructor, but the chip's channels keep running -- the
// state that arms the trap for the next run).
//
// Run it back to back and read the verdicts:
//   run 1: expect HEALTHY
//   run 2: expect BUG REPRODUCED (RESTART pending, outputs dead)
//   run 3: expect HEALTHY again
//
// While it holds (5 s), optionally confirm the pin physically, no
// ESC needed: multimeter DC on the ch14 signal pin reads ~0.5 V
// (1500 us @ 100 Hz = 15% duty of 3.3 V) when alive, flat 0 V on a
// bug run.
//
// MODE1 is read via a private fd on the PWM I2C bus (Pi 4: i2c-4,
// Pi 5: i2c-3) so the check is independent of the library's own
// state. Do NOT run this next to a live mission app (single-owner
// hardware).

#include "nav_bindings.h"
#include <cstdio>
#include <string>
#include <unistd.h>

#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

static int openPwmBus() {
  // Same bus selection as nav.cpp: Pi 5 -> i2c-3, else i2c-4.
  const char* candidates[] = {"/dev/i2c-4", "/dev/i2c-3"};
  for (const char* path : candidates) {
    int fd = open(path, O_RDWR);
    if (fd < 0) continue;
    if (ioctl(fd, I2C_SLAVE, 0x40) < 0) { close(fd); continue; }
    uint8_t reg = 0x00;
    if (write(fd, &reg, 1) == 1) {
      uint8_t v;
      if (read(fd, &v, 1) == 1) {
        printf("PWM bus: %s (PCA9685 at 0x40)\n", path);
        return fd;
      }
    }
    close(fd);
  }
  return -1;
}

static bool g_pending_seen = false;  // any bit-7 sighting this run

static int readMode1(int fd) {
  uint8_t reg = 0x00;
  if (write(fd, &reg, 1) != 1) return -1;
  uint8_t v;
  if (read(fd, &v, 1) != 1) return -1;
  if (v & 0x80) g_pending_seen = true;
  return v;
}

static void printMode1(int fd, const char* when) {
  int v = readMode1(fd);
  if (v < 0) { printf("  MODE1 %-28s read failed\n", when); return; }
  printf("  MODE1 %-28s 0x%02X%s\n", when, v,
         (v & 0x80) ? "  << RESTART PENDING (bit 7)" : "");
}

int main() {
  int i2c = openPwmBus();
  if (i2c < 0) {
    fprintf(stderr, "FAIL: cannot open PWM I2C bus / reach 0x40\n");
    return 1;
  }
  printMode1(i2c, "inherited from last run:");

  // --- the armIfNeeded() sequence, step by step -------------------
  Navigator nav;
  nav.init();
  if (!nav.is_initialized()) { fprintf(stderr, "FAIL: init\n"); return 1; }
  printMode1(i2c, "after pca9685_init:");

  std::string err = nav.pwm_set_frequency(100.0f);
  if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
  printMode1(i2c, "after set_frequency(100):");

  // Neutral before enable, ch14/ch16 (0-based 13/15), like the app.
  nav.pwm_set_pulse_us(13, 1500.0f);
  nav.pwm_set_pulse_us(15, 1500.0f);

  err = nav.pwm_enable(true);
  if (!err.empty()) { fprintf(stderr, "FAIL: %s\n", err.c_str()); return 1; }
  printMode1(i2c, "after pwm_enable(true):");

  // The 2026-08-14 bench trace showed the pending flag can appear
  // after init OR inside set_frequency (parity-dependent) and then
  // be cleared by the driver's MODE1 read-modify-write or by the LED
  // writes above -- WITHOUT that proving the channels restarted. A
  // clear flag here is NOT proof of life; only a set flag is proof
  // of death.
  int m = readMode1(i2c);
  bool pended = g_pending_seen;
  if (m >= 0 && (m & 0x80))
    printf("\nVERDICT: DEAD (definitive) -- OE is low but PWM channels\n"
           "are held OFF by the pending-RESTART condition. An ESC on\n"
           "ch14 would be panic-beeping. Pin ch14: expect 0 V.\n\n");
  else if (pended)
    printf("\nVERDICT: SUSPECT -- RESTART pended during this run's\n"
           "sequence and was cleared by the driver's MODE1/LED writes.\n"
           "The datasheet allows flag-clear WITHOUT channel restart, so\n"
           "registers cannot tell dead from alive here. MEASURE THE\n"
           "PIN: ch14 signal ~0.5 V DC = alive, flat 0 V = dead.\n\n");
  else
    printf("\nVERDICT: healthy by registers (no RESTART event seen).\n"
           "Confirm once with the pin: ch14 ~0.5 V DC average.\n\n");

  printf("Holding 15 s for pin measurement (ch14 signal vs GND)...\n");
  for (int i = 15; i > 0; i -= 5) { printf("  %d...\n", i); sleep(5); }

  // --- mission-style exit: neutral, OE released by the destructor,
  // --- channels left running. This arms the trap for the next run.
  nav.pwm_set_pulse_us(13, 1500.0f);
  nav.pwm_set_pulse_us(15, 1500.0f);
  printMode1(i2c, "at exit (chip keeps running):");
  close(i2c);
  printf("Exiting WITHOUT disabling PWM (mission disarm_on_exit=false).\n"
         "Run me again to see what the next launch inherits.\n");
  return 0;
}

#else  // !__linux__

int main() {
  fprintf(stderr, "test_pca9685_restart: Linux/Navigator hardware only\n");
  return 1;
}

#endif
