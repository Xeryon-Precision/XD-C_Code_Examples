# C Examples — Raspberry Pi

This folder contains C examples for controlling an **XD-C motor controller** from a Raspberry Pi, split into two approaches:

| Folder | Approach | Description |
|--------|----------|-------------|
| [`GPIO/`](GPIO/readme.md) | GPIO | Drives the XD-C directly by toggling Raspberry Pi GPIO pins (pulse/direction, quadrature, PWM), using the `libgpiod` v2 C API. |
| [`UART/`](UART/readme.md) | UART | Controls the XD-C over serial (`/dev/ttyAMA0`) using plain-text commands, with no GPIO pins involved. |

> [!NOTE]
> Both approaches control the same XD-C controller, but require different wiring and different setup steps. See the README in each folder for full details before running anything.

## Which one do I need?

- If your Raspberry Pi is wired to the XD-C's GPIO input pins → see [`GPIO/readme.md`](GPIO/readme.md).
- If your Raspberry Pi is wired to the XD-C's UART (serial) pins → see [`UART/readme.md`](UART/readme.md).