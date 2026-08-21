# Raspberry Pi GPIO Motor Test Scripts

This repository contains a series of test scripts (`GPIO_2.py` – `GPIO_9.py`) used to explore different ways of driving a motor / linear stage from a Raspberry Pi over GPIO. Each script tries a different control scheme (pulse/direction, quadrature, PWM with limit switches, etc.). They were written as incremental experiments, so the pin layout and logic differ between them.

> [!NOTE]
> These scripts control real hardware. Double check your wiring against the pin tables below before running anything, and keep a hand near the power switch the first time you test a new script.

## Requirements

- Raspberry Pi (any model with 40-pin GPIO header)
- Python 3
- [`RPi.GPIO`](https://pypi.org/project/RPi.GPIO/) library

```bash
pip install RPi.GPIO
```

## XD-C Controller Setup (required before running any script)

Every script in this repo is paired with an **XD-C motor controller**, which needs to be configured over USB before it will respond correctly to GPIO input. The number in each script's filename corresponds to the GPIO mode you need to set on the XD-C — e.g. `GPIO_2.py` requires `GPIO=2`, `GPIO_3.py` requires `GPIO=3`, `GPIO_9.py` requires `GPIO=9`, and so on.

1. Wire the Raspberry Pi to the XD-C using pins **19, 23, 25, 27, and 30**.
2. Connect the XD-C to a computer via USB-C and power it on.
3. Open a serial terminal and connect to the XD-C's COM port at **115200 baud**.
4. *(Optional)* Send `INFO=0` to disable verbose status updates.
5. Set the GPIO mode to match the script you're running, e.g. send `GPIO=3` for `GPIO_3.py`.
   - To verify it was set, send `GPIO=?`.
6. Set the steps-per-unit value: send `STPS=3200` (default is `1`).
   - To verify it was set, send `STPS=?`.
7. Save the configuration to the XD-C with `SAVE`.

> [!WARNING]
> If you skip the `SAVE` step, all settings (`GPIO`, `STPS`, etc.) reset the next time the XD-C loses power. You'll need to redo steps 5–7 every time you switch to a different script, since each one expects a different `GPIO` mode.

Once the XD-C is configured, update the pin numbers in the Python script to match your wiring.

## General Usage

1. Complete the XD-C setup above for the script you want to run.
2. Wire your motor driver / controller to the Raspberry Pi according to the pin table for that script (see below).
3. Run the script with Python:

   ```bash
   python3 GPIO_X.py
   ```
4. Stop the script at any time with `Ctrl+C`. All scripts clean up GPIO state (and disable outputs) in a `finally` block, so it's safe to interrupt them.

> [!IMPORTANT]
> All scripts use **BCM pin numbering** (`GPIO.setmode(GPIO.BCM)`), not physical board numbering. Make sure your wiring matches BCM numbers, not the physical pin positions.

---

## Script Overview

| Script | Control scheme | Pins used | Notes |
|--------|----------------|-----------|-------|
| `GPIO_2.py` | Pulse / Direction / Enable | PULSE 17, DIRECTION 27, ENABLE 22, INDEX 23 | Basic step-pulse driver, single direction pin |
| `GPIO_3.py` | Pulse / Forward / Backward | PULSE 17, FORWARD 27, BACKWARD 22, INDEX 23 | Separate forward/backward pins instead of one direction pin |
| `GPIO_4.py` | Quadrature (A/B) | A 17, B 27, ENABLE 22, INDEX 23 | Cycles through a 4-state quadrature sequence instead of sending step pulses |
| `GPIO_8.py` | PWM + Direction + limit switches | PWM 17, DIR 27, ENABLE 22, SWITCH_MINUS 25, SWITCH_PLUS 24 | Variable speed via PWM duty cycle; direction changed by hardware limit switches (interrupt-driven) |
| `GPIO_9.py` | PWM + Forward/Backward + limit switches | PWM 17, FORWARD 27, BACKWARD 22, SWITCH_MINUS 25, SWITCH_PLUS 24 | Same as `GPIO_8.py`, but uses separate forward/backward pins instead of a direction + enable pin |

---

## Script Details

### `GPIO_2.py` — Pulse / Direction / Enable

Drives a stepper-style motor by sending pulses on the `PULSE` pin while `DIRECTION` sets the travel direction. `ENABLE` turns the driver on/off, and `INDEX` triggers a homing pulse at the start.

**Behavior:** Enables the driver, sends an index pulse, then loops forward 10 steps → backward 20 steps → forward 10 steps indefinitely.

### `GPIO_3.py` — Pulse / Forward / Backward

Similar to `GPIO_2.py`, but instead of one direction pin it uses two separate pins (`FORWARD` and `BACKWARD`) to select travel direction.

**Behavior:** Same forward/backward/forward test loop as `GPIO_2.py`, minus the enable pin (no `ENABLE` control in this version).

### `GPIO_4.py` — Quadrature (A/B) Stepping

Instead of pulsing a single step pin, this script drives two pins (`A`, `B`) through a 4-state quadrature sequence `(0,0) → (1,0) → (1,1) → (0,1)` to move the motor one increment at a time — forward advances the sequence, backward reverses it.

**Behavior:** Enables the driver, sends an index pulse, then loops forward 10 mm → backward 20 mm → forward 10 mm indefinitely.

### `GPIO_8.py` — PWM Speed Control + Limit Switches

Controls motor speed with a PWM signal (1 kHz) and direction with a single `DIR` pin. Two limit switches (`SWITCH_PIN_MINUS`, `SWITCH_PIN_PLUS`) are wired as interrupts — hitting either switch flips the direction automatically.

**Behavior:** Ramps speed up from 0% to 100% and back down in a continuous triangle-wave pattern, while direction is controlled by whichever limit switch was last triggered.

### `GPIO_9.py` — PWM Speed Control + Limit Switches (Forward/Backward pins)

Functionally identical to `GPIO_8.py`, but uses separate `FORWARD_PIN` / `BACKWARD_PIN` outputs instead of a single direction + enable pin combination.

**Behavior:** Same speed ramp and limit-switch-triggered direction change as `GPIO_8.py`.

---

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
> Several scripts reuse the same GPIO numbers for different purposes (e.g. pin 17 is `PULSE` in some scripts and part of a quadrature pair in others). Only wire up **one** script's circuit at a time to avoid pin conflicts.

## Safety Notes

- Always run scripts with the motor mechanically free to move — the test loops move the stage automatically as soon as the script starts.
- `Ctrl+C` triggers cleanup, but if the script is killed forcefully (e.g. `kill -9`), GPIO pins may be left in an unexpected state. Run `GPIO.cleanup()` manually in a Python shell if needed.