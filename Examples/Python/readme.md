# Python Examples — XD-C Motor Controller

This folder contains Python examples for controlling an **XD-C motor controller**, split into three approaches:

| Folder | Approach | Description |
|--------|----------|-------------|
| [`USB/`](USB/readme.md) | USB (library) | Uses the `Xeryon.py` library over a USB/COM port. The recommended starting point — handles scanning, positioning, stepping, logging, and status bits through a simple Python API. |
| [`UART/`](UART/readme.md) | UART (Raspberry Pi) | Controls the XD-C directly from a Raspberry Pi over UART serial (`/dev/ttyAMA0`), sending plain-text commands with `pyserial`. No GPIO pins involved. |
| [`GPIO/`](GPIO/readme.md) | GPIO (Raspberry Pi) | Drives the XD-C by toggling Raspberry Pi GPIO pins directly (pulse/direction, quadrature, PWM with limit switches), using `RPi.GPIO`. |

> [!NOTE]
> All three approaches control the same XD-C controller, but require different hardware setups and wiring. See the README in each folder for full setup and usage details before running anything.

## Which one do I need?

- Running on a computer connected to the XD-C over USB → see [`USB/readme.md`](USB/readme.md).
- Running on a Raspberry Pi wired to the XD-C's UART (serial) pins → see [`UART/readme.md`](UART/readme.md).
- Running on a Raspberry Pi wired to the XD-C's GPIO input pins → see [`GPIO/readme.md`](GPIO/readme.md).

> [!IMPORTANT]
> If you're not sure how your XD-C is wired to your computer or Raspberry Pi, check your physical connections first — sending commands over the wrong interface won't work even if the code runs without errors.