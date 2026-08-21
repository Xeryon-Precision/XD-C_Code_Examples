# XD-C UART Control Program (C)

This program controls an **XD-C motor controller** from a Raspberry Pi over **UART serial** (`/dev/ttyAMA0`), using POSIX `termios` directly — no external serial library required. It's the C equivalent of the Python UART control script: no GPIO pins are toggled at all, communication happens entirely by sending the XD-C plain-text commands and reading back its responses.

## Requirements

- Raspberry Pi (with a hardware UART, e.g. `/dev/ttyAMA0`)
- A C compiler (e.g. `gcc`)
- No external libraries — only the standard C library and POSIX headers (`termios.h`, `fcntl.h`, `unistd.h`)

## Building

```bash
gcc UART.c -o UART
```

## XD-C Setup for UART

1. Connect the XD-C to a computer via USB-C, power it on, and open a serial terminal on its COM port at 115200 baud.
2. Set the UART baud rate the XD-C should use to talk to the Raspberry Pi: send `UART=9600`.
   - To verify it was set, send `UART=?`.
3. Save the configuration: send `SAVE`.

> [!WARNING]
> If you skip the `SAVE` step, the `UART` setting resets the next time the XD-C loses power.

## Raspberry Pi Wiring

Connect the XD-C's UART TX/RX lines to the Raspberry Pi's hardware UART pins (BCM 14 / TXD and BCM 15 / RXD — physical pins 8 and 10), plus a shared GND.

> [!WARNING]
> Make sure the Pi's built-in serial console is disabled, otherwise it will conflict with `/dev/ttyAMA0` and this program won't be able to open the port.

## Running the Program

```bash
./UART
```

The program runs through its full test sequence and exits on its own — there's no interactive loop to interrupt.

> [!NOTE]
> `BAUDRATE` is hardcoded to `B9600` in the source (`#define BAUDRATE B9600`). Make sure this matches the `UART=9600` value you set on the XD-C — if you change one, change the other.

## Serial Port Configuration

The program opens `/dev/ttyAMA0` and configures it directly via `termios` to match the settings used by the original Python version (`pyserial`):

| Setting | Value |
|---------|-------|
| Baud rate | 9600 |
| Data bits | 8 (`CS8`) |
| Parity | None |
| Stop bits | 1 |
| Flow control | None (no hardware or software) |
| Mode | Raw input/output (no line buffering, echo, or signal processing) |
| Read timeout | 1.0 s (`VTIME = 10`) |

## Commands Used

| Command | Meaning |
|---------|---------|
| `INDX=1` | Start homing / index search |
| `STAT=?` | Read status word; bit 8 set = index found |
| `SCAN=-1` / `SCAN=1` | Start a continuous scan in the negative / positive direction |
| `SSPD=n` | Set the scan speed |
| `STOP=1` | Stop the current motion |
| `DPOS=n` | Move to absolute position `n` |
| `STEP=n` | Move `n` steps relative to the current position |
| `RSET=1` | Reset the controller |

> [!NOTE]
> The full command set is documented in the [Xeryon XD-C manual v3.1](https://xeryon.com/download-files/Xeryon%20XD-C%20manual%20v3.1%20-%20JPL.pdf), section 4.2.

## Behavior

1. Opens and configures the serial port, then waits 2 seconds for the connection to stabilize.
2. Sends `INDX=1` and polls `STAT=?` until bit 8 (index found) is set, or times out after 10 seconds.
3. Runs a full-stroke scan test: scans in one direction, then the other.
4. Sets a higher scan speed (`SSPD=1000`), starts another scan, then stops it after a fixed delay.
5. Sets an even higher scan speed (`SSPD=10000`).
6. Runs a few absolute position moves (`DPOS`) to test positioning: `0 → 10000 → -10000 → 0`.
7. Runs 10 relative step moves of 3200 steps each, with a short pause between each.
8. Resets the controller with `RSET=1` and closes the serial connection.

## Safety Notes

- Always run this program with the motor mechanically free to move — it starts moving the stage immediately once the index is found.
- The program does not install a signal handler, so `Ctrl+C` will terminate it immediately without closing the serial port cleanly. If you get a "port busy" error on the next run, make sure no other process is still holding `/dev/ttyAMA0`.