# UART protocol

The FDO2-G2's ASCII command protocol as the driver uses it: framing, commands, reply fields, status bits, error
codes and the optional CRC.

The source is section 4 of the [FDO2-G2 datasheet](datasheet/Datasheet_FDO2-G2.pdf) (v5, 03/2026). Other
PyroScience instruments use a different command set; this page and the driver apply to the FDO2-G2 only.

## Framing

- A command is a header (`#` and capital letters, such as `#MOXY`), optional values separated by spaces, and a CR
  (`0x0D`) or CR LF.
- A reply that worked echoes the header, appends the requested values and ends with a single CR.
- A reply that failed is `#ERRO E` with a negative code `E`.
- Numbers are decimal ASCII in the range −2147483648 to 2147483647. The only exception is the `#IDNR` value, an
  unsigned 64-bit number of up to 20 digits.

The driver sends the command, waits for one line, ignores a LF if one appears, and splits the line at spaces.

## Commands the driver sends

| Command | Reply | Driver call |
| --- | --- | --- |
| `#VERS` | `#VERS D N R S` | `ReadVersion()` |
| `#IDNR` | `#IDNR N` | `ReadUniqueId()` |
| `#MOXY` | `#MOXY O T S` | `MeasureMoxy()`, or `SendMeasure(Measure::Moxy)` |
| `#MRAW` | `#MRAW O T S D I A P H` | `MeasureMraw()`, or `SendMeasure(Measure::Mraw)` |
| `#LOGO` | `#LOGO` | `FlashLogo()`: the status LED flashes 4 times, about 1 s |

`#VERS` reports D = 8 for every FDO2-G2, N = 1 oxygen channel, the firmware revision as R = 357 for 3.57, and S,
a bit mask of the sensors present (bit 0 oxygen, bit 1 temperature, bit 2 pressure, bit 3 humidity; 15 means all).
The unique id from `#IDNR` is not the serial number printed on the device.

## Reply fields

Every field is a signed 32-bit integer, except the status `S`, which is unsigned. The driver keeps the raw value
and adds the scaled one.

| Field | Raw unit | Scaled member | Typical range |
| --- | --- | --- | --- |
| `O` oxygen partial pressure | 10<sup>−3</sup> hPa | `p_o2_hpa` | 0 to 210 hPa in air |
| `T` sensor temperature | m°C | `temp_c` | 0 °C to 50 °C |
| `S` status | bits | `status_u32` | 0 or 1 in normal operation |
| `D` phase shift dphi | m° | `dphi_deg` | 5° to 60° |
| `I` signal intensity | µV | `signal_intensity_mv` | 100 mV to 500 mV |
| `A` ambient light | µV | `ambient_light_mv` | 0 to about 100 mV |
| `P` pressure at the back of the housing | µbar | `pressure_mbar` | 900 mbar to 1100 mbar |
| `H` humidity inside the housing | m%RH | `rh_in_housing_pct` | 10 %RH to 90 %RH |

`O`, `T` and `S` come with both `#MOXY` and `#MRAW`; the rest only with `#MRAW`. Every scaled member is the raw
value times 10<sup>−3</sup>.

Two rules from the datasheet for the `#MRAW` values:

- **Signal and ambient light.** Keep `I + A` under about 2000 mV. Above it the detector can saturate; shade the
  sensor. Compare signal intensities only at similar oxygen levels: about 200 mV to 400 mV is typical at
  200 hPa and room temperature.
- **Volume percent.** When the gas at the membrane is at the same pressure as the back of the housing,
  % O<sub>2</sub> = 100 x pO<sub>2</sub> / P. `fdo2::VolumePercentO2(p_o2_hpa, pressure_mbar)` computes it
  (1 mbar = 1 hPa) and returns 0 for a pressure of zero or below.

## Status bits

<div class="hf-registers" markdown>

| Bit | Constant in `fdo2::moxy_status` | Meaning |
| --- | --- | --- |
| `0` | `kWarnAutoAmpReduced` | Warning: amplification reduced to avoid saturation; the reading is valid |
| `1` | `kFatalSignalTooLow` | Fatal: oxygen signal below 20 mV |
| `2` | `kFatalSignalTooHigh` | Fatal: oxygen signal or ambient light too high |
| `3` | `kFatalRefTooLow` | Fatal: reference signal below 20 mV |
| `4` | `kFatalRefTooHigh` | Fatal: reference signal or ambient light above 2400 mV |
| `5` | `kFatalTempSensor` | Fatal: temperature sensor failure |
| `7` | `kWarnHumidityHigh` | Warning: humidity in the housing above 90 %RH |
| `9` | `kErrPressureSensor` | Error: housing pressure sensor failure; pO<sub>2</sub> not affected |
| `10` | `kErrHumiditySensor` | Error: housing humidity sensor failure; pO<sub>2</sub> not affected |

</div>

Bits 6 and 8 are reserved. The datasheet's rule: under normal operation `S` is 0 or 1; any other value means `O`
and `T` are or can be wrong, and the user must check `S` on every measurement. `fdo2::MoxyStatusOkForOxygen(S)`
is that rule.

## Error codes

A command that failed returns `#ERRO E`. The driver turns it into `DriverError::DeviceError` and keeps `E` in
`LastDeviceErrorCode()`.

| E | Type | Meaning and what to do |
| --- | --- | --- |
| `-1` | General | A non-specific error |
| `-21` | UART parse | The command did not parse; repeat it |
| `-22` | UART receive | The command was not received correctly (not ready, bad terminator); repeat it |
| `-23` | UART header | The header has characters other than A to Z; repeat it |
| `-26` | UART request | The header matches no supported command |
| `-60` | Invalid calibration | A calibration was out of range and was discarded |

Any other code is potentially fatal: the datasheet says to replace the sensor and contact PyroScience.

## Optional CRC

`#CRCE 1` makes the module end every reply with a CRC-16 (the MODBUS polynomial, CRC-16-IBM) as decimal ASCII
after a colon, computed over every character before the colon. The setting is stored in flash and survives a
power cycle. The driver removes a trailing `" :<digits>"` before it parses a reply, but it does not check the
CRC, and it never sends `#CRCE`. Without the CRC, the datasheet's first-level check is the echo of the command
header, which the driver does compare.

## Commands the driver does not send

| Command | What it does | Why the driver leaves it out |
| --- | --- | --- |
| `#CALO`, `#CAHI P` | Two-point calibration at 0 % and at `P`; discards the factory calibration | Writes flash; up to 10 s; needs a calibration gas |
| `#SETM M` | Flash duration mode 0 (lowest drift) to 4 (lowest noise); default 2 | Writes flash |
| `#BCST T` | Broadcast `#MRAW` replies every `T` ms (100 to 10000) | Writes flash; interleaves with command replies |
| `#CRCE K` | CRC on or off | Writes flash |
| `#BAUD R` | Baud rate until the next power cycle | Not implemented |
| `#RDUM`, `#WRUM` | Read and write 64 words of user memory | Not implemented; `#WRUM` writes flash |

The module tolerates about 20,000 flash cycles, and an interrupted flash write can corrupt it. Send these commands
from a service tool with a stable supply, not from production firmware. If a module arrives with broadcast mode
on, send `#BCST 0` once: its broadcasts otherwise interrupt the driver's replies.

## Next

[API guide](api-guide.md) uses these commands from C++; [Troubleshooting](troubleshooting.md) maps errors and
status bits to causes.
