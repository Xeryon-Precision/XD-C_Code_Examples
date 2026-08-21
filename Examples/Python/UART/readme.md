# XD-C UART Control Script

This script controls an **XD-C motor controller** from a Raspberry Pi over **UART serial** (`/dev/ttyAMA0`). Unlike a GPIO-based setup, no GPIO pins are toggled at all — the Pi sends the XD-C plain-text commands and reads back its responses.

## Requirements

- Raspberry Pi (any model with a hardware UART)
- Python 3
- [`pyserial`](https://pypi.org/project/pyserial/)

```bash
pip install pyserial
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
> Make sure the Pi's built-in serial console is disabled, otherwise it will conflict with `/dev/ttyAMA0` and this script won't be able to open the port.

## Running the Script

```bash
python3 UART.py
```

Stop at any time with `Ctrl+C`. The script closes the serial port with `ser.close()` at the end of its run.

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

1. Sends `INDX=1` and polls `STAT=?` until bit 8 (index found) is set, or times out after 10 seconds.
2. Runs a full-stroke scan test: scans in one direction, then the other.
3. Sets a higher scan speed (`SSPD=1000`), starts another scan, then stops it after a fixed delay.
4. Sets an even higher scan speed (`SSPD=10000`).
5. Runs a few absolute position moves (`DPOS`) to test positioning: `0 → 10000 → -10000 → 0`.
6. Runs 10 relative step moves of 3200 steps each, with a short pause between each.
7. Resets the controller with `RSET=1` and closes the serial connection.

## Safety Notes

- Always run this script with the motor mechanically free to move — it starts moving the stage immediately once the index is found.
- If the script is killed forcefully (e.g. `kill -9`), the serial port may not close cleanly. Re-run or manually release `/dev/ttyAMA0` if you get a "port busy" error on the next run.