# Hardware setup

## Logic levels

The FDO2-G2 UART is **3.0 V** signalling with **3.3 V absolute maximum** on those
pins (data sheet §2 / §4). Your MCU UART must **not** drive 5 V TTL into the
sensor. Use level shifters or a 3.3 V-native host (for example ESP32-S3).

## UART parameters

| Parameter | Default | Notes |
|-----------|---------|--------|
| Baud | **19200** | Factory default after every power cycle |
| Data | 8 bits | |
| Parity | None | |
| Stop | 1 bit | |
| Flow control | None | |

Higher baud is possible only after a successful `#BAUD` command (not implemented
in this driver build).

## Boot delay

The module needs on the order of **one second** after power-up before it accepts
commands. The ESP32 example waits `kFdo2G2PowerUpSettleMs` (~1100 ms) before `#VERS`.

## Connector

The data sheet references **Molex 560020-0420** for the module harness. Route
TX/RX/GND only after confirming pinout for your cable revision.

## ESP32-S3 shipped examples (default)

The `examples/esp32` apps map **MCU TX → sensor RX** and **MCU RX ← sensor TX**
on **`UART_NUM_1`**: **GPIO47** (TX), **GPIO21** (RX), **19200** 8N1. Change the
template parameters on the `Fdo2EspIdfUart<…>` type alias if your board uses
different pins.

## Fast versus verbose reads

- **`#MOXY`** — smallest response; ideal for **high-rate control loops**.
- **`#MRAW`** — adds pressure, humidity, dphi, and intensities for diagnostics and
  for the documented **%O₂** ratio when **P** tracks vent-side pressure.

**Next:** [CMake integration →](cmake_integration.md)
