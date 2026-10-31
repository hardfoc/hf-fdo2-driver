---
layout: default
title: "HardFOC FDO2-G2 Driver"
description: "Hardware-agnostic C++17 UART client for PyroScience FDO2-G2 optical oxygen sensors"
nav_order: 1
permalink: /
---

# HF-FDO2 Driver

**Header-only C++17 UART client for the PyroScience FDO2-G2 optical oxygen sensor**
(data sheet **v5**, **§4** UART API): `#VERS`, `#IDNR`, `#MOXY`, `#MRAW`, `#LOGO`,
with engineering-unit decoding, optional **CRC** suffix stripping, and `#ERRO`
codes via `LastDeviceErrorCode()`. Default baud after power-up is **19200** 8N1.

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CI](https://github.com/hardfoc/hf-fdo2-driver/actions/workflows/esp32-examples-build-ci.yml/badge.svg?branch=main)](https://github.com/hardfoc/hf-fdo2-driver/actions/workflows/esp32-examples-build-ci.yml)
[![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://hardfoc.github.io/hf-fdo2-driver/)

## Table of contents

1. [Overview](#overview)
2. [Features](#features)
3. [Quick start](#quick-start)
4. [Documentation](#documentation)
5. [Examples](#examples)
6. [Official references](#official-references)
7. [License](#license)

## Overview

> **[Live documentation (GitHub Pages)](https://hardfoc.github.io/hf-fdo2-driver/)** —
> Installation, UART protocol tables, CMake, API summary, and troubleshooting.

The driver targets **read-only measurement** commands suitable for host firmware:
fast **`#MOXY`** for control loops and richer **`#MRAW`** when you need vent-side
pressure **P**, internal RH **H**, dphi, and intensities. Flash-writing commands
(`#CALO`, `#CAHI`, `#CRCE`, `#SETM`, `#BAUD`, …) are intentionally **not**
implemented here (endurance and power-stability constraints in the data sheet).

## Features

- **CRTP** `fdo2::UartInterface<Derived>` — no virtual calls; you provide `write` /
  `read` / `flush_rx`.
- **`fdo2::Driver<UartT>`** — `ReadVersion`, `ReadUniqueId`, `MeasureMoxy`,
  `MeasureMraw`, `FlashLogo`; timeouts configurable per command class.
- **No heap allocation** in the driver framing path; fits FreeRTOS / bare metal.
- **ESP32-S3** examples (`build_app.sh` matrix): **UART1**, default **TX=GPIO47** /
  **RX=GPIO21**, **19200** 8N1, shared template `Fdo2EspIdfUart` — see
  `examples/esp32/`.

## Quick start

CMake:

```cmake
add_subdirectory(/path/to/hf-fdo2-driver)
target_link_libraries(your_target PRIVATE hf::fdo2)
```

Application:

```cpp
#include "fdo2.hpp"

struct MyUart : fdo2::UartInterface<MyUart> {
  void write(const uint8_t* d, size_t n);
  size_t read(uint8_t* o, size_t m, uint32_t t);
  void flush_rx();
};

MyUart uart;
fdo2::Driver<MyUart> dev(uart);
auto v = dev.ReadVersion();
auto m = dev.MeasureMraw();  // or MeasureMoxy() for smallest frame
```

## Documentation

| Topic | Link |
|--------|------|
| Doc hub | [docs/index.md](docs/index.md) |
| UART protocol | [docs/uart_protocol.md](docs/uart_protocol.md) |
| API / CMake / hardware | [docs/](docs/) |
| Doxygen | `_config/Doxyfile` → run `doxygen _config/Doxyfile` from repo root |

## Examples

- **ESP32 workflow:** [examples/esp32/README.md](examples/esp32/README.md)
- **Build script:** `examples/esp32/scripts/build_app.sh`

## Official references

Use the **FDO2-G2** data sheet and firmware notes for calibration, measurement
modes, broadcast, and flash lifetime. Other PyroScience products may use
different UART command sets.

## License

MIT — see [LICENSE](LICENSE).
