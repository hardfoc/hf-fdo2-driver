# API guide

How to do the common jobs with the driver: identify the module, measure, read raw data, measure without
blocking and handle errors.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Sensor` standing for `fdo2::Driver<MyUart>`. The
[API reference](api-reference.md) lists every member.

## The shape of the API

One `fdo2::Driver<UartT>` object per sensor. Each command is one call that sends the command, waits for the
reply and returns a `DriverResult<T>`.

| Call | Command | Returns |
| --- | --- | --- |
| `ReadVersion()` | `#VERS` | `VersionInfo`: device id, channels, firmware, sensors |
| `ReadUniqueId()` | `#IDNR` | `uint64_t` |
| `MeasureMoxy(timeout_ms)` | `#MOXY` | `MoxyReading`: pO<sub>2</sub>, temperature, status |
| `MeasureMraw(timeout_ms)` | `#MRAW` | `MrawReading`: the above plus dphi, signal, ambient light, pressure, humidity |
| `FlashLogo()` | `#LOGO` | `DriverResult<void>` |
| `SendMeasure()`, `TryReadReplyLine()`, `ParseMoxyLine()`, `ParseMrawLine()`, `ParseVersionLine()` | any of the above | The same exchange in two halves |

A `DriverResult<T>` has `value`, `error` and `ok()`, and converts to `bool`. The driver never throws and never
allocates.

## Identify the module

```cpp
--8<-- "tests/docs/api_tasks.cpp:identify"
```

Call it once after power-up (wait `kFdo2G2PowerUpSettleMs` first). Device id 8 confirms an FDO2-G2; the
firmware revision tells you whether the [noise erratum](hardware.md#errata) applies.

## Measure oxygen

```cpp
--8<-- "tests/docs/api_tasks.cpp:measure"
```

`#MOXY` is the shortest exchange: one measurement, three fields. A successful call means the module answered;
the status check tells you whether the answer is a valid oxygen reading. A status of 1 (amplification reduced) is
still valid.

## Read raw data and volume percent

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

`#MRAW` takes the same measurement and adds the raw optical values and the pressure and humidity inside the
housing. Use it for diagnostics, for volume percent, or at a slower rate next to fast `#MOXY` reads, as the
`fdo2_sensor_demo` example does. [UART protocol](uart-protocol.md#reply-fields) lists every field and its unit.

## Measure without blocking

`MeasureMoxy()` blocks for the whole measurement, up to about 150 ms. A loop that serves other devices can split
the exchange: send the command, keep working, and pick up the reply when the UART has a complete line.

```cpp
--8<-- "tests/docs/api_tasks.cpp:split"
```

`TryReadReplyLine()` calls your port's `read()` with a timeout of 0, so the port must return a whole line or
nothing ([Porting](porting.md#what-you-implement)). You own the timeout: if no reply arrives within about 300 ms,
send the command again.

## Set timeouts

```cpp
--8<-- "tests/docs/api_tasks.cpp:timeouts"
```

| Timeout | Default | Used by |
| --- | --- | --- |
| Measurement | 250 ms | `MeasureMoxy()`, `MeasureMraw()` when called with 0 (the default argument) |
| Line | 400 ms | `ReadVersion()`, `ReadUniqueId()`, `FlashLogo()` |

The defaults fit the factory setting (M = 2, up to about 150 ms per measurement). A module set to M = 3 or 4
takes up to about 300 ms: raise the measurement timeout above that.

## Handle errors and status

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| Signal | Meaning | What to do |
| --- | --- | --- |
| `DriverError::Timeout` | No reply within the timeout | Check wiring, baud rate, supply; wait 1 s after power-up |
| `DriverError::ProtocolError` | A reply that did not parse: wrong header or field count, a value that is not a number in range, no CR at the end | Repeat the command; check the baud rate and broadcast mode |
| `DriverError::CrcMismatch` | With `#CRCE 1`: the reply's checksum is wrong, or missing while `SetCrcRequired(true)` | Repeat the command; see [optional CRC](uart-protocol.md#optional-crc) |
| `DriverError::DeviceError` | `#ERRO E` | `LastDeviceErrorCode()`; [error codes](uart-protocol.md#error-codes) |
| `DriverError::BufferOverflow` | A reply longer than the line buffer | Noise on RX, or broadcast replies running together |
| `MoxyStatusOkForOxygen(S)` is `false` | Status bits other than bit 0 set | Do not use `O` and `T`; see the [status bits](uart-protocol.md#status-bits) |

`LastDeviceErrorCode()` is reset to 0 at the start of every command, so read it right after the call that
failed.

## Identify a sensor on a port

```cpp
--8<-- "tests/docs/api_tasks.cpp:logo"
```

With several modules on several UARTs, `FlashLogo()` shows which connector belongs to which port.

## Next

[Configuration](configuration.md) lists every setting and default; [Troubleshooting](troubleshooting.md) maps
symptoms to causes.
