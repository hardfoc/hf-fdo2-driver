---
layout: default
title: "Documentation"
description: "Complete documentation for the HardFOC PyroScience FDO2-G2 UART driver"
nav_order: 2
parent: "HardFOC FDO2-G2 Driver"
permalink: /docs/
has_children: true
---

# HF-FDO2 driver documentation

This site mirrors the [`docs/`](https://github.com/hardfoc/hf-fdo2-driver/tree/main/docs) folder in the repository and documents the **PyroScience FDO2-G2** optical oxygen sensor UART interface (**data sheet v5, §4**) as implemented by this header-only driver.

> **Browse on GitHub:** [repository home](https://github.com/hardfoc/hf-fdo2-driver) · [Issues](https://github.com/hardfoc/hf-fdo2-driver/issues)

## Documentation structure

### Getting started

1. **[Installation](installation.md)** — Toolchain, CMake, submodule layout
2. **[Quick start](quickstart.md)** — Minimal CRTP UART adapter and first `#MOXY` read
3. **[UART protocol](uart_protocol.md)** — Commands, scaling, CRC suffix, `#ERRO` codes

### Hardware and integration

4. **[Hardware setup](hardware_setup.md)** — Levels, baud, boot delay, connector notes
5. **[CMake integration](cmake_integration.md)** — `hf_fdo2_build_settings.cmake`, `hf::fdo2`

### Reference and examples

6. **[API reference](api_reference.md)** — `Driver<UartT>`, types, results, status bits
7. **[Examples](examples.md)** — ESP32-S3 `build_app.sh` workflow
8. **[Troubleshooting](troubleshooting.md)** — Baud mismatch, timeouts, status interpretation

### Manufacturer

9. **[Datasheet and links](datasheet/README.md)** — Bundled PDF and PyroScience references

---

## Recommended reading order

1. [Installation](installation.md)
2. [Hardware setup](hardware_setup.md)
3. [Quick start](quickstart.md)
4. [UART protocol](uart_protocol.md)
5. [Examples](examples.md)

---

## Need help?

- **Build or flash:** [Examples](examples.md) and [Troubleshooting](troubleshooting.md)
- **Protocol details:** [UART protocol](uart_protocol.md)
- **API:** [API reference](api_reference.md) and headers under [`inc/`](https://github.com/hardfoc/hf-fdo2-driver/tree/main/inc)

**Next:** [Installation →](installation.md)
