# XD-C UART Control Program (C++)

This program controls an **XD-C motor controller** from a Raspberry Pi over **UART serial** (`/dev/ttyAMA0`), using the POSIX `termios` API directly. No GPIO pins are toggled — the Pi sends the XD-C plain-text commands and reads back its responses.

## Requirements

- Raspberry Pi (any model with a hardware UART)
- A C++11-capable compiler (e.g. `g++`)
- POSIX headers (`termios.h`, `fcntl.h`, `unistd.h`) — Linux only

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

## Building

`UART.cpp` uses `std::thread`, so link `pthread`:

```bash
g++ -std=c++11 UART.cpp -o UART -lpthread
```

## Running the Program

```bash
./UART
```

Stop at any time with `Ctrl+C`. Under normal completion, the `XDC` destructor closes the serial port automatically.

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

## Code Structure

The `XDC` class wraps the serial connection:

- **Constructor** — opens the port (default `/dev/ttyAMA0`) and configures `termios` for 9600 baud, 8N1, no parity, no hardware flow control, and raw (non-canonical) mode with a 1-second read timeout (`VMIN=0`, `VTIME=10`).
- **Destructor** — closes the file descriptor if open.
- `sendData(cmd)` — appends `\r\n` and writes the command, then blocks until it's transmitted (`tcdrain`).
- `recvData(cmd)` — flushes pending input, sends `cmd`, and reads back a response line (stripping `\r`, stopping at `\n`).
- `getStat()` — sends `STAT=?` and parses the numeric value out of the `STAT=` response.
- `waitForBit8(timeout_sec)` — polls `getStat()` until bit 8 (index found) is set or the timeout elapses, then waits 2 more seconds before returning `true`.

## Behavior

`main()` runs a fixed test sequence:

1. Sends `INDX=1` and polls status until bit 8 (index found) is set, or times out after 10 seconds; prints `Index found` or `Timeout`.
2. Runs a full-stroke scan test: scans in one direction (`SCAN=-1`), then the other (`SCAN=1`), with a 2-second pause after each.
3. Sets a higher scan speed (`SSPD=1000`), starts another scan (`SCAN=-1`), then stops it (`STOP=1`) after a fixed delay.
4. Sets an even higher scan speed (`SSPD=10000`).
5. Runs a few absolute position moves (`DPOS`) to test positioning: `0 → 10000 → -10000 → 0`, pausing 2 seconds after each.
6. Runs 10 relative step moves of 3200 steps each (`STEP=3200`), with a 500 ms pause between each.
7. Resets the controller with `RSET=1`.

Any failure during setup (e.g. the port can't be opened or configured) throws a `std::runtime_error`, which `main()` catches and reports via `std::cerr` before returning a non-zero exit code.

## Safety Notes

- Always run this program with the motor mechanically free to move — it starts moving the stage immediately once the index is found.
- If the program is killed forcefully (e.g. `kill -9`), the serial port may not close cleanly. Re-run or manually release `/dev/ttyAMA0` if you get a "port busy" error on the next run.