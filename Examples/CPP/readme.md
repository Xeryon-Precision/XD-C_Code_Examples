# C++ Examples — XD-C Motor Controller

This folder contains C++ examples for controlling an **XD-C motor controller**, split into three approaches:

| Folder | Approach | Description |
|--------|----------|-------------|
| [`USB/`](USB/readme.md) | USB (library) | Cross-platform C++ SDK (CMake-based) for a USB/COM-connected setup, equivalent to the Python `Xeryon.py` API. |
| [`UART/`](UART/readme.md) | UART (Raspberry Pi) | Controls the XD-C directly from a Raspberry Pi over UART serial (`/dev/ttyAMA0`), sending plain-text commands via POSIX `termios`. No GPIO pins involved. |
| [`GPIO/`](GPIO/readme.md) | GPIO (Raspberry Pi) | Drives the XD-C by toggling Raspberry Pi GPIO pins directly (pulse/direction, quadrature, PWM with limit switches), using the `libgpiod` v2 C++ bindings (`gpiod.hpp`). |

> [!NOTE]
> All approaches control the same XD-C controller, but require different hardware setups and wiring. See the readme in each folder for full setup, build, and usage details before running anything.

## Which one do I need?

- Running on a computer connected to the XD-C over USB → see [`USB/readme.md`](USB/readme.md).
- Running on a Raspberry Pi wired to the XD-C's UART (serial) pins → see [`UART/readme.md`](UART/readme.md).
- Running on a Raspberry Pi wired to the XD-C's GPIO input pins → see [`GPIO/readme.md`](GPIO/readme.md).

> [!IMPORTANT]
> If you're not sure how your XD-C is wired to your computer or Raspberry Pi, check your physical connections first — sending commands over the wrong interface won't work even if the code compiles and runs without errors.

## Requirements

All approaches need a C++ compiler (`g++`) on the target machine. Beyond that, requirements differ per folder — see [`USB/readme.md`](USB/readme.md), [`UART/readme.md`](UART/readme.md), and [`GPIO/readme.md`](GPIO/readme.md) for the specific compiler standard, libraries, and build commands each one needs.

> [!TIP]
> If you're looking for the same examples in Python, see [`../Python/readme.md`](../Python/readme.md).