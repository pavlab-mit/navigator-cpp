#!/bin/bash
set -u

echo "== identity =="
date --iso-8601=seconds
uname -a

echo "== relevant processes =="
pgrep -a pAntler || true
pgrep -a pAftCtrl || true
pgrep -a navigator_test || true
pgrep -a pwm_cycle || true

echo "== GPIO devices =="
ls -l /dev/gpiochip* 2>&1 || true

echo "== GPIO consumers =="
if command -v gpioinfo >/dev/null 2>&1; then
  gpioinfo 2>&1 || true
else
  echo "gpioinfo not installed"
fi

echo "== PWM I2C devices =="
ls -l /dev/i2c-3 /dev/i2c-4 2>&1 || true

echo "== linked navigator library =="
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
if [ -x "$SCRIPT_DIR/build/pwm_cycle" ]; then
  ldd "$SCRIPT_DIR/build/pwm_cycle" | grep navigator || true
else
  echo "pwm_cycle not built"
fi
