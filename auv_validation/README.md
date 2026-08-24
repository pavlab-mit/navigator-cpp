# Navigator AUV repeat-run validation

These tests isolate `navigator_cpp` from MOOS and exercise the powered
PCA9685 across repeated process launches. Run them with the propeller removed
or thruster power disconnected. Use an oscilloscope or logic analyzer on the
selected PWM channel and GPIO 26 (OE) when possible.

## Copy and build

Copy the complete `navigator-cpp` tree to the AUV so the validation binary is
linked to the library under test, then run:

```sh
./build.sh --release
cd auv_validation
chmod +x build_validation.sh preflight.sh run_repeated.sh
./build_validation.sh
./preflight.sh | tee preflight.log
```

Use `sudo` for the validation commands if the deployed account does not have
permission to open the GPIO and I2C character devices.

Confirm `preflight.log` shows no existing `pAntler`, `pAftCtrl`, Navigator test,
or `pwm_cycle` process. Confirm `ldd` resolves `libnavigator_cpp.so` from the
copied tree rather than `/usr/local/lib`.

The complete standard sequence can be run interactively with:

```sh
chmod +x run_suite.sh
./run_suite.sh
```

## Clean repeated launches

```sh
./run_repeated.sh 30 clean
```

Every cycle must report successful frequency, pulse, enable, disable, and
shutdown operations. While enabled, the default waveform is 100 Hz with a
1500 us high pulse on channel 15. `ALL_LED_OFF_H` must not have bit 4 set in
the `configured_disabled` or `enabled` snapshots.

## Abrupt process exits

This uses `_exit(0)` to skip C++ destructors and library shutdown while still
letting Linux close GPIO handles. It reproduces the persistent-controller
state inherited after a process dies:

```sh
./run_repeated.sh 30 abrupt
./run_repeated.sh 5 clean
```

All abrupt cycles and the following clean recovery cycles must start and emit
the expected waveform. Observe OE after each abrupt exit. It must return high;
if it does not, confirm the Navigator board's physical OE pull-up before any
in-water test.

## Competing-owner check

In terminal 1:

```sh
./build/pwm_cycle --hold-ms=30000
```

While it is holding, run the same command in terminal 2. The second process
must fail clearly with `Device or resource busy`. It must not mention a sysfs
fallback and must not claim PWM was enabled.

## Evidence to retain

Keep `preflight.log`, all files under `logs/`, and scope captures showing OE
and PWM for the first, second, and final iterations of both repeated tests.
