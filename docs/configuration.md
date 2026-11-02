# Configuration

Every setting the driver exposes, its default, and the module settings it leaves alone.

## Constructor

```cpp
fdo2::Driver<MyUart> sensor(uart);
```

| Argument | Type | Meaning |
| --- | --- | --- |
| `uart` | `MyUart&` | Your port; must outlive the driver. The UART must already be open at 19200 baud, 8N1 |

The constructor does not touch the UART. One driver per module; each module needs its own UART, because the
protocol has no addresses.

## Timeouts

| Setting | Call | Default | Applies to |
| --- | --- | --- | --- |
| Measurement timeout | `SetMeasureTimeoutMs(uint32_t)` | 250 ms | `MeasureMoxy()`, `MeasureMraw()` |
| Per-call measurement timeout | `MeasureMoxy(ms)`, `MeasureMraw(ms)` | 0: use the setting above | That call only |
| Line timeout | `SetLineTimeoutMs(uint32_t)` | 400 ms | `ReadVersion()`, `ReadUniqueId()`, `FlashLogo()` |
| Slow-command timeout | `SetSlowCommandTimeoutMs(uint32_t)` | 12000 ms | Reserved: no current command uses it |

Read the first two back with `GetMeasureTimeoutMs()` and `GetLineTimeoutMs()`. The timeout is what the driver
passes to your port's `read()`, so the port decides how precisely it is kept.

## Constants

| Constant | Value | Meaning |
| --- | --- | --- |
| `fdo2::kFdo2G2DefaultBaud` | 19200 | The module's baud rate after every power-up |
| `fdo2::kFdo2G2PowerUpSettleMs` | 1100 | Wait after power-up before the first command (datasheet: about 1 s) |

## Module settings the driver does not change

These live in the module's flash and survive a power cycle. The driver reads with whatever is set; change them
with a service tool, sparingly (about 20,000 flash cycles in the module's life).

| Setting | Command | Factory value | Effect on the driver |
| --- | --- | --- | --- |
| Flash duration mode | `#SETM M` | M = 2: up to about 150 ms per measurement | M = 3 or 4: up to about 300 ms; raise the measurement timeout |
| Broadcast mode | `#BCST T` | Off | On: unsolicited `#MRAW` lines interleave with replies; turn it off |
| CRC | `#CRCE K` | Off | On: the driver strips a `" :<digits>"` suffix ([UART protocol](uart-protocol.md#optional-crc)) |
| Calibration | `#CALO`, `#CAHI` | Factory calibration | None; a user calibration replaces the factory one |
| Baud rate | `#BAUD R` | 19200, after every power-up | Your port must match it |

## Build options

| Option or define | Default | Effect |
| --- | --- | --- |
| `HF_FDO2_ENABLE_WARNINGS` (CMake) | `OFF` | `-Wall -Wextra -Wpedantic` on targets that link the driver |
| `HF_FDO2_BUILD_TESTS` (CMake) | `OFF` | Host unit tests and documentation snippets |
| `HF_FDO2_VERSION_*` (CMake) | `1.0.0` | Set in `cmake/hf_fdo2_build_settings.cmake`, written to `fdo2_version.h` |

## Next

[API guide](api-guide.md) uses these settings in context; [Installation](installation.md) shows the CMake setup.
