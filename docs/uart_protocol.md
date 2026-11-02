# FDO2-G2 UART protocol

This driver follows **PyroScience FDO2-G2**, document version **v5 (03/2026)**,
**§4 Communication Interface**.

## Electrical and framing

- **3.0 V UART levels** (absolute maximum **3.3 V** on IO).
- **8N1**, no handshake.
- **Default baud after power-up: 19200** (1200 … 115200 programmable via `#BAUD`; not wrapped in this driver yet).
- Allow **~1 s** after power-up before commands.
- Host commands end with **carriage return** (`0x0D`), or **CR+LF** (`0x0D 0x0A`).
- Device responses end with **CR** only.
- If **CRC** is enabled (`#CRCE 1`), each response ends with ` : <decimal CRC16>`
before CR. The driver **strips** that suffix before tokenizing.

## Implemented commands

| Command | Response shape | Driver API |
|---------|----------------|------------|
| `#VERS` | `#VERS D N R S` | `ReadVersion()` |
| `#IDNR` | `#IDNR N` (64-bit decimal) | `ReadUniqueId()` |
| `#MOXY` | `#MOXY O T S` | `MeasureMoxy()` |
| `#MRAW` | `#MRAW O T S D I A P H` | `MeasureMraw()` |
| `#LOGO` | `#LOGO` | `FlashLogo()` |

## Numeric decoding

- **O** — pO₂ in **10⁻³ hPa** → `p_o2_hpa = O × 10⁻³`.
- **T** — temperature in **m°C** → `temp_c = T × 10⁻³`.
- **S** — **unsigned 32-bit** status. Normal operation: **S = 0 or 1** only.
- **#MRAW** extras: **D** in millidegrees; **I**, **A** in **µV** → mV with `×10⁻³`;
  **P** in **µbar** → mbar with `×10⁻³` (1 mbar = 1000 µbar); **H** in **m%RH** → `%RH` with `×10⁻³`.

Constants for individual status bits live in `fdo2::moxy_status` in `fdo2_types.hpp`.

## Volume % O₂

When pressure at the sensing membrane matches the **vent / back-side** pressure
represented by **P** in `#MRAW`:

`%O₂ = 100 × pO₂ [hPa] / p_air [hPa]` (mbar ≈ hPa for this ratio per data sheet §2).

Use `VolumePercentO2(p_o2_hpa, pressure_mbar)`.

## Errors

Responses beginning with `#ERRO` followed by signed **E** (Table 1, §4.4). When
`DriverResult::error == DeviceError`, read `Driver::LastDeviceErrorCode()`.

## Out of scope in this driver

Flash-writing commands (`#CALO`, `#CAHI`, `#CRCE`, `#SETM`, `#BCST`, `#BAUD`,
`#WRUM`, …) are **not** exposed: they consume **flash endurance** (~20k cycles
total) and require stable power during programming (data sheet warning).

**Next:** [Hardware setup →](hardware_setup.md)
