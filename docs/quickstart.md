# Quick start

You supply a **CRTP** adapter that inherits `fdo2::UartInterface<YourType>` and
implements three calls: `write`, `read`, `flush_rx`. Optional `delay_ms_impl`
helps with boot delays.

## 1. Wire and baud

- Default UART after sensor power-up: **19200**, **8N1**, no handshake (data sheet §4).
- Allow **~1 s** after power-up before the first command (`kFdo2G2PowerUpSettleMs`).

## 2. Minimal adapter (sketch)

```cpp
#include "fdo2.hpp"

struct MyUart : fdo2::UartInterface<MyUart> {
  void write(const uint8_t* d, size_t n);
  size_t read(uint8_t* o, size_t m, uint32_t timeout_ms);
  void flush_rx();
};
```

## 3. First measurements

```cpp
MyUart uart;
fdo2::Driver<MyUart> dev(uart);
dev.SetMeasureTimeoutMs(300);

auto v = dev.ReadVersion();          // #VERS D N R S
auto m = dev.MeasureMoxy();          // fastest: #MOXY O T S
// or: auto r = dev.MeasureMraw();   // adds pressure, RH, dphi, intensities
```

Decode fields are ready in `MoxyReading` / `MrawReading` (`p_o2_hpa`, `temp_c`,
`status_u32`, …). Use `fdo2::MoxyStatusOkForOxygen(s)` to assert **S ≤ 1** as
recommended in the data sheet.

## 4. Volume % O₂

When membrane pressure matches the vent-side pressure from `#MRAW` **P**, use
`VolumePercentO2(p_o2_hpa, pressure_mbar)` (see [UART protocol](uart_protocol.md)).

**Next:** [UART protocol →](uart_protocol.md)
