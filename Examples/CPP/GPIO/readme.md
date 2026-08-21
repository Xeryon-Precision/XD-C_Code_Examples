# Raspberry Pi GPIO Motor Test Scripts (C++)

This repository contains a series of C++ test programs (`GPIO_2.cpp`, `GPIO_3.cpp`, `GPIO_4.cpp`, `GPIO_8.cpp`, `GPIO_9.cpp`) used to explore different ways of driving a motor / linear stage from a Raspberry Pi over GPIO, using the modern [`libgpiod`](https://libgpiod.readthedocs.io/) C++ bindings (`gpiod.hpp`). Each program tries a different control scheme (pulse/direction, quadrature, PWM with limit switches, etc.).

> [!NOTE]
> These programs control real hardware. Double check your wiring against the pin tables below before running anything, and keep a hand near the power switch the first time you test a new program.

## Requirements

- Raspberry Pi (with a `libgpiod` v2-compatible kernel/character device, e.g. `/dev/gpiochip4`)
- A C++17-capable compiler (e.g. `g++`)
- `libgpiod` v2 with C++ bindings installed

```bash
sudo apt install libgpiod-dev
```

## Building

Each `.cpp` file is a standalone program. Compile with:

```bash
g++ -std=c++17 GPIO_X.cpp -o GPIO_X -lgpiod -lgpiodcxx
```

Replace `GPIO_X` with the script you want to build (`GPIO_2`, `GPIO_3`, `GPIO_4`, `GPIO_8`, or `GPIO_9`).

> [!IMPORTANT]
> These programs open `/dev/gpiochip4`. Confirm this is the correct gpiochip for your Raspberry Pi model — run `gpiodetect` to list available chips, and update the path in the source if yours differs.

## XD-C Controller Setup (required before running any program)

Every program in this repo is paired with an **XD-C motor controller**, which needs to be configured over USB before it will respond correctly to GPIO input. The number in each program's filename corresponds to the GPIO mode you need to set on the XD-C — e.g. `GPIO_2.cpp` requires `GPIO=2`, `GPIO_3.cpp` requires `GPIO=3`, `GPIO_9.cpp` requires `GPIO=9`, and so on.

1. Wire the Raspberry Pi to the XD-C using pins **19, 23, 25, 27, and 30**.
2. Connect the XD-C to a computer via USB-C and power it on.
3. Open a serial terminal and connect to the XD-C's COM port at **115200 baud**.
4. *(Optional)* Send `INFO=0` to disable verbose status updates.
5. Set the GPIO mode to match the program you're running, e.g. send `GPIO=3` for `GPIO_3.cpp`.
   - To verify it was set, send `GPIO=?`.
6. Set the steps-per-unit value: send `STPS=<value>`, where `<value>` is however many encoder counts you want per unit of travel (default is `1`). For example, `STPS=3200`.
   - To verify it was set, send `STPS=?`.
7. Save the configuration to the XD-C with `SAVE`.

> [!WARNING]
> If you skip the `SAVE` step, all settings (`GPIO`, `STPS`, etc.) reset the next time the XD-C loses power. You'll need to redo steps 5–7 every time you switch to a different program, since each one expects a different `GPIO` mode.

Once the XD-C is configured, update the pin constants in the C++ source to match your wiring, then rebuild.

## General Usage

1. Complete the XD-C setup above for the program you want to run.
2. Wire your motor driver / controller to the Raspberry Pi according to the pin table for that program (see below).
3. Build the program (see [Building](#building)).
4. Run it:

   ```bash
   ./GPIO_X
   ```
5. Stop the program at any time with `Ctrl+C`. Errors during setup or the main loop are caught and printed (`Fout: ...`) before exiting with a non-zero status.

> [!IMPORTANT]
> All programs use **BCM pin numbering** via the gpiochip line offsets. Make sure your wiring matches BCM numbers, not physical board positions.

## Program Overview

| Program | Control scheme | Pins used | Notes |
|---------|----------------|-----------|-------|
| `GPIO_2.cpp` | Pulse / Direction / Enable | PULSE 17, DIRECTION 27, ENABLE 22, INDEX 23 | Basic step-pulse driver, single direction pin |
| `GPIO_3.cpp` | Pulse / Forward / Backward | PULSE 17, FORWARD 27, BACKWARD 22, INDEX 23 | Separate forward/backward pins instead of one direction pin |
| `GPIO_4.cpp` | Quadrature (A/B) | A 17, B 27, ENABLE 22, INDEX 23 | Cycles through a 4-state quadrature sequence instead of sending step pulses |
| `GPIO_8.cpp` | PWM + Direction + limit switches | PWM 17, DIR 27, ENABLE 22, SWITCH_MINUS 25, SWITCH_PLUS 24 | Variable speed via software PWM duty cycle; direction changed by polling hardware limit switches |
| `GPIO_9.cpp` | PWM + Forward/Backward + limit switches | PWM 17, FORWARD 27, BACKWARD 22, SWITCH_MINUS 25, SWITCH_PLUS 24 | Same as `GPIO_8.cpp`, but uses separate forward/backward pins instead of a direction + enable pin |

## Program Details

### `GPIO_2.cpp` — Pulse / Direction / Enable

Drives a stepper-style motor by pulsing the `PULSE` line while `DIRECTION` sets the travel direction. `ENABLE` turns the driver on/off, and `INDEX` triggers a homing pulse at the start.

**Behavior:** Enables the driver, runs an index search, then loops forward 10 steps → backward 20 steps → forward 10 steps indefinitely.

### `GPIO_3.cpp` — Pulse / Forward / Backward

Similar to `GPIO_2.cpp`, but uses two separate pins (`FORWARD` and `BACKWARD`) to select travel direction instead of one direction pin.

**Behavior:** Same forward/backward/forward test loop as `GPIO_2.cpp`, minus the enable pin (no `ENABLE` control in this version).

### `GPIO_4.cpp` — Quadrature (A/B) Stepping

Instead of pulsing a single step line, this program drives two lines (`A`, `B`) through a 4-state quadrature sequence `(0,0) → (1,0) → (1,1) → (0,1)` to move the motor one increment at a time — forward advances the sequence, backward reverses it.

**Behavior:** Enables the driver, runs an index search, then loops forward 10 mm → backward 20 mm → forward 10 mm indefinitely.

### `GPIO_8.cpp` — PWM Speed Control + Limit Switches

Generates a software PWM signal (1 kHz) on `PWM_PIN` and controls direction with a single `DIR_PIN`. Two limit switch inputs (`SWITCH_PIN_MINUS`, `SWITCH_PIN_PLUS`) are polled in the main loop — a rising edge on either one flips the motor's direction.

**Behavior:** Ramps speed up from 0% to 100% and back down in a continuous triangle-wave pattern, while direction is controlled by whichever limit switch was last triggered.

### `GPIO_9.cpp` — PWM Speed Control + Limit Switches (Forward/Backward pins)

Functionally identical to `GPIO_8.cpp`, but uses separate `FORWARD_PIN` / `BACKWARD_PIN` outputs instead of a single direction + enable pin combination.

**Behavior:** Same speed ramp and limit-switch-triggered direction change as `GPIO_8.cpp`.

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
- If a program is killed forcefully (e.g. `kill -9`), GPIO lines may be left in an unexpected state since the `gpiod::line_request` destructor won't run. Power-cycle the driver or re-run a program that resets the lines you used if this happens.
- For `GPIO_8.cpp` and `GPIO_9.cpp`, make sure the limit switches are physically installed and wired correctly before running — without them, the speed ramp will run without any direction change.