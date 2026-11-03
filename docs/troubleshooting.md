# Troubleshooting

Symptoms you may see while bringing up an FDO2-G2, their causes and the fix for each.

Start with the `DriverError` of the failing call (`fdo2::ToString(result.error)`): `Timeout` points at wiring,
baud rate or power; `SerialError` at the host UART (the port's `write()` failed); `ProtocolError` and `CrcMismatch` at what arrives on RX; `DeviceError` at the command, with
the module's code in `LastDeviceErrorCode()`.

## No reply: every call returns `Timeout`

| Cause | Check | Fix |
| --- | --- | --- |
| TX and RX not crossed | MCU TX must go to pin 3 (RXD), MCU RX to pin 2 (TXD) | Swap the two lines, or the pins in your port |
| Wrong baud rate | The module runs at 19200 after every power-up | Set 19200, 8N1, no flow control |
| Module still booting | First command less than 1 s after power-up | Wait `kFdo2G2PowerUpSettleMs` (1100 ms) |
| No supply or no common ground | VCC at pin 4 between 3.3 V and 5.0 V, GND shared with the MCU | Fix the harness; a 40 mA peak must not pull VCC below 3.3 V |
| Port `read()` never returns a line | A logic analyser shows the reply, but the call times out | The port must return at the CR ([Porting](porting.md#uart-port)) |
| Only `FlashLogo()` times out | The LED flashes, but the `#LOGO` echo comes about 1 s later | Keep `SetSlowCommandTimeoutMs()` above 1000 ms (default 12000 ms) |

## Replies that do not parse: `ProtocolError`

| Cause | Check | Fix |
| --- | --- | --- |
| Broadcast mode left on | `#MRAW` lines arrive without a command | Send `#BCST 0` once from a terminal; it is stored in flash |
| Port returns partial lines | Errors come and go, worse at low timeouts; a line without its CR is rejected | Return one whole line, CR included, per `read()` |
| Noise or a 5 V signal on RXD | Garbled characters on a logic analyser | Level-shift the MCU's TX; shorten or shield the cable |

The datasheet's advice for a garbled reply is to send the command again.

## Checksum errors: `CrcMismatch`

The module has the CRC on (`#CRCE 1`) and a reply's checksum did not match what arrived, or
`SetCrcRequired(true)` is set and a reply came without one. A wrong checksum means the reply was corrupted on the
line: send the command again, and look for noise or a level problem on RXD if it happens often. If every reply
fails while the module's CRC is off, clear `SetCrcRequired()` ([UART protocol](uart-protocol.md#optional-crc)).

## `DeviceError` after `#ERRO`

Read `LastDeviceErrorCode()` right after the failing call. Codes -21, -22 and -23 are transmission errors:
repeat the command. Code -26 means the module did not recognise the command. Any code not in the
[error table](uart-protocol.md#error-codes) is potentially fatal: the datasheet says to replace the sensor and
contact PyroScience.

## Status is not 0 or 1

The call succeeded but `MoxyStatusOkForOxygen()` is `false`: do not use the oxygen or temperature reading.

| Bit set | Likely cause | Fix |
| --- | --- | --- |
| 2 or 4 (signal or ambient light too high) | Sunlight or a lamp on the sensing membrane | Shade the sensor; check `ambient_light_mv` with `#MRAW` |
| 1 or 3 (signal too low) | Membrane damaged or contaminated, condensation | Inspect the membrane; keep the dew point out of it |
| 5 (temperature sensor) | Sensor fault | Replace the sensor |
| 7 (humidity above 90 %RH, warning) | Moisture reaching the housing through the back side | Keep the back side dry and non-condensing |
| 9 or 10 (pressure or humidity sensor) | Housing sensor fault; pO<sub>2</sub> still valid | `MoxyStatusOkForOxygen()` rejects these too; decide in your code whether to accept pO<sub>2</sub> |

## The reading is plausible but off

| Symptom | Cause | Fix |
| --- | --- | --- |
| Drifts during the first minutes | Warm-up: 3 min to full accuracy | Discard readings for 3 min after power-up |
| Volume percent wrong while pO<sub>2</sub> is right | The membrane and the back of the housing are at different pressures | Compute % O<sub>2</sub> from a pressure measured at the membrane |
| Extra noise at one or two temperatures between 8 °C and 15 °C | Firmware 3.55 erratum | Firmware 3.57 fixes it; `#SETM` trades noise for drift ([Hardware](hardware.md#errata)) |
| Readings creep after exposure to solvents, NO<sub>2</sub>, Cl<sub>2</sub> or heat above 60 °C | Cross-sensitivity or ageing | See section 3 of the [datasheet](datasheet/Datasheet_FDO2-G2.pdf); recalibrate or replace |

## Calls take longer than the measurement

Each `MeasureMoxy()` takes the full timeout (250 ms by default) instead of about 150 ms: the port's `read()` waits
for `max` bytes or for the timeout instead of returning at the CR. Use the line-based port on
[Porting](porting.md#uart-port), or the ESP32 example port, which returns at the CR.

## Still stuck

Open an [issue](https://github.com/hardfoc/hf-fdo2-driver/issues) with a capture of the UART traffic, the
`#VERS` reply (firmware revision) and the driver version from `fdo2::GetDriverVersion()`.
