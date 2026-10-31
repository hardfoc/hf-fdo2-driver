---
layout: default
title: "API reference"
nav_order: 6
parent: "Documentation"
permalink: /docs/api-reference/
---

# API reference

Authoritative declarations live under [`inc/`](https://github.com/hardfoc/hf-fdo2-driver/tree/main/inc).
Run **Doxygen** (`doxygen _config/Doxyfile`) for HTML cross-links.

## Transport — `fdo2::UartInterface<Derived>`

CRTP base. Implement:

- `void write(const uint8_t* data, size_t length);`
- `size_t read(uint8_t* out, size_t max, uint32_t timeout_ms);`
- `void flush_rx();`
- Optional `void delay_ms_impl(uint32_t ms);`

## Driver — `fdo2::Driver<UartT>`

| Method | Description |
|--------|-------------|
| `ReadVersion()` | `#VERS` → `VersionInfo` (D, N, R, S) |
| `ReadUniqueId()` | `#IDNR` → `uint64_t` |
| `MeasureMoxy()` | `#MOXY` → `MoxyReading` |
| `MeasureMraw()` | `#MRAW` → `MrawReading` |
| `FlashLogo()` | `#LOGO` |
| `SetLineTimeoutMs` / `SetMeasureTimeoutMs` / `SetSlowCommandTimeoutMs` | UART timeouts |
| `LastDeviceErrorCode()` | Parsed **E** after `#ERRO` |

## Types — `fdo2_types.hpp`

- `DriverResult<T>`, `DriverError`, `ToString(DriverError)`
- `VersionInfo`, `FirmwareRevisionToDouble(R)`
- `MoxyReading`, `MrawReading`
- `moxy_status::*` bitmask constants
- `MoxyStatusOkForOxygen(uint32_t s)` — true if **S ≤ 1**
- `VolumePercentO2(p_o2_hpa, p_air_mbar)`
- `kFdo2G2DefaultBaud`, `kFdo2G2PowerUpSettleMs`

## Umbrella include

```cpp
#include "fdo2.hpp"
```

**Next:** [Examples →](examples.md)
