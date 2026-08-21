# Raspberry Pi GPIO Motor Test Scripts (C)

This repository contains a series of C test programs (`GPIO_2.c`, `GPIO_3.c`, `GPIO_4.c`, `GPIO_8.c`, `GPIO_9.c`) used to explore different ways of driving a motor from a Raspberry Pi over GPIO, using the [`libgpiod`](https://libgpiod.readthedocs.io/) v2 C API (`gpiod.h`). Each program tries a different control scheme (pulse/direction, quadrature, PWM with limit switches, etc.).

> [!NOTE]
> These programs control real hardware. Double check your wiring against the pin tables below before running anything, and keep a hand near the power switch the first time you test a new program.

## Requirements

- Raspberry Pi (with a `libgpiod` v2-compatible kernel/character device)
- A C11-capable compiler (e.g. `gcc`)
- `libgpiod` v2 (C API)
- `pthread` (used by `GPIO_8.c` and `GPIO_9.c` for the PWM and switch-polling threads)

```bash
sudo apt install libgpiod-dev
```

## Building

Each `.c` file is a standalone program.

For `GPIO_2.c`, `GPIO_3.c`, and `GPIO_4.c`:

```bash
gcc -std=c11 GPIO_X.c -o GPIO_X -lgpiod
```

For `GPIO_8.c` and `GPIO_9.c` (these use threads and atomics, so link `pthread` too):

```bash
gcc -std=c11 GPIO_X.c -o GPIO_X -lgpiod -lpthread
```

Replace `GPIO_X` with the program you want to build.

> [!IMPORTANT]
> These programs open `/dev/gpiochip0`. Confirm this is the correct gpiochip for your Raspberry Pi model by running `gpiodetect`, and update the `CHIP_PATH` define in the source if yours differs.

## XD-C Controller Setup (required before running any program)

Every program in this repo is paired with an **XD-C motor controller**, which needs to be configured over USB before it will respond correctly to GPIO input. The number in each program's filename corresponds to the GPIO mode you need to set on the XD-C — e.g. `GPIO_2.c` requires `GPIO=2`, `GPIO_3.c` requires `GPIO=3`, `GPIO_9.c` requires `GPIO=9`, and so on.

1. Wire the Raspberry Pi to the XD-C using pins **19, 23, 25, 27, and 30**.
2. Connect the XD-C to a computer via USB-C and power it on.
3. Open a serial terminal and connect to the XD-C's COM port at **115200 baud**.
4. *(Optional)* Send `INFO=0` to disable verbose status updates.
5. Set the GPIO mode to match the program you're running, e.g. send `GPIO=3` for `GPIO_3.c`.
   - To verify it was set, send `GPIO=?`.
6. Set the steps-per-unit value: send `STPS=<value>`, where `<value>` is however many encoder counts you want per unit of travel (default is `1`). For example, `STPS=3200`.
   - To verify it was set, send `STPS=?`.
7. Save the configuration to the XD-C with `SAVE`.

> [!WARNING]
> If you skip the `SAVE` step, all settings (`GPIO`, `STPS`, etc.) reset the next time the XD-C loses power. You'll need to redo steps 5–7 every time you switch to a different program, since each one expects a different `GPIO` mode.

Once the XD-C is configured, update the pin `#define`s in the C source to match your wiring, then rebuild.

## General Usage

1. Complete the XD-C setup above for the program you want to run.
2. Wire your motor driver / controller to the Raspberry Pi according to the pin table for that program (see below).
3. Build the program (see [Building](#building)).
4. Run it:

   ```bash
   ./GPIO_X
   ```
5. Stop the program with `Ctrl+C`. All programs install a `SIGINT` handler that flips a `running` flag, so the main loop exits and GPIO lines are released cleanly before the process ends.

> [!IMPORTANT]
> All programs use **BCM pin numbering** via the gpiochip line offsets. Make sure your wiring matches BCM numbers, not physical board positions.

## Program Overview

| Program | Control scheme | Pins used | Notes |
|---------|----------------|-----------|-------|
| `GPIO_2.c` | Pulse / Direction / Enable | PULSE 17, DIR 27, ENABLE 22, INDEX 23 | Basic step-pulse driver, single direction pin |
| `GPIO_3.c` | Pulse / Forward / Backward | PULSE 17, FORWARD 27, BACKWARD 22, INDEX 23 | Separate forward/backward pins instead of one direction pin |
| `GPIO_4.c` | Quadrature (A/B) | A 17, B 27, ENABLE 22, INDEX 23 | Cycles through a 4-state quadrature sequence instead of sending step pulses |
| `GPIO_8.c` | PWM + Direction + limit switches | PWM 17, DIR 27, ENABLE 22, SWITCH_MINUS 25, SWITCH_PLUS 24 | Software PWM generated on a dedicated thread; limit switches read via a second thread using edge-event polling |
| `GPIO_9.c` | PWM + Forward/Backward + limit switches | PWM 17, FORWARD 27, BACKWARD 22, SWITCH_MINUS 25, SWITCH_PLUS 24 | Same threaded design as `GPIO_8.c`, but uses separate forward/backward pins instead of a direction + enable pin |

## Program Details

### `GPIO_2.c` — Pulse / Direction / Enable

Drives a stepper-style motor by pulsing the `PULSE` line while `DIR_PIN` sets the travel direction. `EN_PIN` turns the driver on/off, and `INDEX_PIN` triggers a homing pulse at the start.

**Behavior:** Enables the driver, runs an index search, then loops forward 10 steps → backward 20 steps → forward 10 steps until interrupted.

### `GPIO_3.c` — Pulse / Forward / Backward

Similar to `GPIO_2.c`, but uses two separate pins (`FORWARD_PIN` and `BACKWARD_PIN`) to select travel direction instead of one direction pin. No `ENABLE` control in this version.

**Behavior:** Same forward/backward/forward test loop as `GPIO_2.c`.

### `GPIO_4.c` — Quadrature (A/B) Stepping

Instead of pulsing a single step line, this program drives two lines (`A_PIN`, `B_PIN`) through a 4-state quadrature sequence `(0,0) → (1,0) → (1,1) → (0,1)` to move the motor one increment at a time — forward advances the sequence, backward reverses it (with a custom `mod4()` helper, since C's `%` can return negative results unlike Python's).

**Behavior:** Enables the driver, runs an index search, then loops forward 10 mm → backward 20 mm → forward 10 mm until interrupted.

### `GPIO_8.c` — PWM Speed Control + Limit Switches

Spins up two background threads:
- A **PWM thread** that toggles `PWM_PIN` on and off to generate a ~1 kHz software PWM signal at the current duty cycle.
- A **switch thread** that polls the file descriptors of the two limit-switch lines (`SWITCH_MINUS_PIN`, `SWITCH_PLUS_PIN`) for rising-edge events, with a 200 ms debounce, and flips `DIR_PIN` accordingly.

The main thread just ramps the shared `pwm_duty_percent` value up and down between 0–100%.

**Behavior:** Speed ramps up from 0% to 100% and back down in a continuous triangle-wave pattern, while direction is controlled by whichever limit switch was last triggered.

### `GPIO_9.c` — PWM Speed Control + Limit Switches (Forward/Backward pins)

Functionally identical to `GPIO_8.c`, but uses separate `FORWARD_PIN` / `BACKWARD_PIN` outputs instead of a single direction + enable pin combination.

**Behavior:** Same speed ramp and limit-switch-triggered direction change as `GPIO_8.c`.

## Pin Reference (BCM numbering)

| Signal | GPIO_2 | GPIO_3 | GPIO_4 | GPIO_8 | GPIO_9 |
|--------|--------|--------|--------|--------|--------|
| Pulse / PWM | 17 | 17 | — | 17 | 17 |
| Direction | 27 | — | — | 27 | — |
| Forward | — | 27 | — | — | 27 |
| Backward | — | 22 | — | — | 22 |
| Enable | 22 | — | 22 | 22 | — |
| Index | 23 | 23 | 23 | — | — |
| Quadrature A | — | — | 17 | — | — |
| Quadrature B | — | — | 27 | — | — |
| Limit switch (–) | — | — | — | 25 | 25 |
| Limit switch (+) | — | — | — | 24 | 24 |

> [!WARNING]
> Several programs reuse the same GPIO numbers for different purposes (e.g. pin 17 is `PULSE` in some programs and part of a quadrature pair in others). Only wire up **one** program's circuit at a time to avoid pin conflicts.

## Safety Notes

- Always run programs with the motor mechanically free to move — the test loops move the stage automatically as soon as the program starts.
- All programs handle `Ctrl+C` (`SIGINT`) gracefully and release GPIO lines before exiting. A forced kill (e.g. `kill -9`) skips this cleanup, so GPIO lines may be left in whatever state they were last set to.
- `GPIO_8.c` and `GPIO_9.c` join their background threads (`pthread_join`) before releasing GPIO lines — make sure to let the program exit normally via `Ctrl+C` rather than killing it, so the PWM and switch threads shut down cleanly.
- For `GPIO_8.c` and `GPIO_9.c`, make sure the limit switches are physically installed and wired correctly before running — without them, the speed ramp will run without any direction change.