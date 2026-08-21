# Examples — XD-C Motor Controller

This folder contains example code and projects for controlling an **XD-C motor controller**, organized by language/platform:

| Folder | Language | Description |
|--------|----------|-------------|
| [`Python/`](Python/readme.md) | Python | USB (library), UART, and GPIO examples for controlling the XD-C, including the `Xeryon.py` library and Raspberry Pi scripts. |
| [`CPP/`](CPP/readme.md) | C++ | USB, UART, and GPIO examples, including Raspberry Pi programs using POSIX `termios` and the `libgpiod` v2 C++ bindings. |
| [`C/`](C/readme.md) | C | UART and GPIO examples for Raspberry Pi, using POSIX `termios` and the `libgpiod` v2 C API. |
| [`LabVIEW/`](LabVIEW/readme.md) | LabVIEW | A full LabVIEW project (axis driver, sequencer, preferences) for controlling one or more XD-C axes from a LabVIEW application. |

> [!NOTE]
> All folders control the same XD-C controller, but use different languages and, in some cases, different physical interfaces (USB, UART, GPIO). See the readme inside each folder for setup, wiring, and usage details before running anything.

## Which one do I need?

- Using Python on a PC or Raspberry Pi → see [`Python/readme.md`](Python/readme.md).
- Using C++ on a PC or Raspberry Pi → see [`CPP/readme.md`](CPP/readme.md).
- Using C on a Raspberry Pi → see [`C/readme.md`](C/readme.md).
- Using LabVIEW → see [`LabVIEW/readme.md`](LabVIEW/readme.md).

> [!IMPORTANT]
> Within `Python/` and `CPP/`, examples are further split by interface — USB, UART, or GPIO. Make sure you pick the folder matching how your XD-C is physically wired to your computer or Raspberry Pi.