# Quickstart

Read oxygen from your MCU: wire four pins, write a small port class, call three driver functions.

## 1. Wire it

| MCU | FDO2-G2 connector | Notes |
| --- | --- | --- |
| UART TX | pin 3, RXD | Sensor input, 3.0 V levels, 3.3 V maximum |
| UART RX | pin 2, TXD | Sensor output, 3.0 V levels |
| GND | pin 1, GND | Common ground |
| 3.3 V to 5.0 V | pin 4, VCC | 40 mA peak during a measurement |

Set the UART to 19200 baud, 8 data bits, no parity, 1 stop bit, no flow control. A 5 V MCU needs a level
shifter on its TX line. The [Hardware](hardware.md) page has the connector, the timing and the electrical limits.

## 2. Add the driver

Add the repository as a submodule and link the `hf::fdo2` CMake target (or add `inc/` to your include path and
build with C++17). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-fdo2-driver.git external/hf-fdo2-driver
```

## 3. Write the port

The port is the only platform code. It writes bytes, returns one complete reply line and flushes the receive
buffer. Replace the five `platform_*` calls with your SDK's.

```cpp title="my_uart.hpp"
--8<-- "tests/docs/my_uart.hpp:port"
```

`read()` must return a whole line ending in CR, or nothing. [Porting](porting.md) explains why and lists the
rules for each method.

## 4. Read oxygen

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. The module answers commands about 1 s after power-up. `kFdo2G2PowerUpSettleMs` is 1100 ms.
2. `ReadVersion()` sends `#VERS` and checks that a module answers. An FDO2-G2 reports device id 8.
3. `MeasureMoxy()` sends `#MOXY`, waits for the reply and scales it. `MoxyStatusOkForOxygen()` accepts the
   status values 0 and 1 only: any other value means the oxygen or temperature reading can be wrong.
4. `p_o2_hpa` is the oxygen partial pressure in hPa and `temp_c` the sensor temperature in °C.

Allow 3 minutes of warm-up for full accuracy. This example is built by the host CI, so it compiles against the
current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `ReadVersion()` returns `Timeout` | TX and RX crossed (MCU TX to sensor RXD), 19200 baud, supply, 1 s boot delay |
| `ProtocolError` on every call | Baud rate, or broadcast mode left on by another tool ([Troubleshooting](troubleshooting.md)) |
| `DeviceError` | Read `LastDeviceErrorCode()`; the codes are in [UART protocol](uart-protocol.md#error-codes) |
| Status not 0 or 1 | Ambient light, condensation on the membrane, a sensor fault ([Troubleshooting](troubleshooting.md#status-is-not-0-or-1)) |

## Next

- [API guide](api-guide.md): raw data, volume percent, non-blocking measurement, errors.
- [Examples](examples.md): two complete ESP32-S3 applications.
- [UART protocol](uart-protocol.md): every command and field the driver uses.
