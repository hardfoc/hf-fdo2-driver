---
layout: default
title: "Installation"
nav_order: 1
parent: "Documentation"
permalink: /docs/installation/
---

# Installation

The driver is **header-only**. CMake generates `fdo2_version.h` and exposes an
`INTERFACE` target `hf::fdo2` for consumers.

## Requirements

- **C++17** (`std::string_view`, `if constexpr`, `<cmath>` helpers).
- **CMake ≥ 3.16** for the root `CMakeLists.txt`.
- A **byte-level UART** (3.0 V signalling per FDO2-G2 data sheet; host IO must
  respect **3.3 V max** on the module side).
- **ESP-IDF ≥ 5.4** recommended; CI targets `release/v5.5` (see `examples/esp32/app_config.yml`).

## Repository layout

```text
hf-fdo2-driver/
├── inc/                         ← public headers (`fdo2.hpp`, …)
├── cmake/
│   ├── hf_fdo2_build_settings.cmake
│   └── hf_fdo2Config.cmake.in
├── examples/esp32/              ← ESP-IDF example + `hf_fdo2` component wrapper
├── docs/                        ← this documentation (Jekyll + Doxygen input)
├── _config/                     ← Doxygen, Jekyll, clang-format, lychee, …
├── .github/workflows/          ← CI (build, lint, docs)
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Submodule (recommended)

```bash
git submodule add https://github.com/hardfoc/hf-fdo2-driver.git third_party/hf-fdo2-driver
git submodule update --init --recursive third_party/hf-fdo2-driver
```

```cmake
add_subdirectory(third_party/hf-fdo2-driver)
target_link_libraries(my_app PRIVATE hf::fdo2)
```

## ESP-IDF component

See [Examples](examples.md) for the shipped wrapper under
`examples/esp32/components/hf_fdo2/`.

## Doxygen (optional)

From the repository root:

```bash
doxygen _config/Doxyfile
```

HTML output is written under `docs/html/` (ignored by Git).

**Next:** [Quick start →](quickstart.md)
