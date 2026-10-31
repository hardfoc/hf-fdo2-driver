---
layout: default
title: "Examples"
nav_order: 7
parent: "Documentation"
permalink: /docs/examples/
---

# Examples

## ESP32-S3 (`examples/esp32`)

1. Initialize the tooling submodule:

   ```bash
   cd examples/esp32
   git submodule update --init --recursive
   ```

2. Build (ESP-IDF `release/v5.5`, target `esp32s3` from `app_config.yml`):

   ```bash
   ./scripts/build_app.sh fdo2_minimal_example Debug
   ./scripts/build_app.sh fdo2_sensor_demo Debug
   ```

3. Flash and monitor:

   ```bash
   ./scripts/flash_app.sh flash_monitor fdo2_minimal_example Debug
   ./scripts/flash_app.sh flash_monitor fdo2_sensor_demo Debug
   ```

Both apps use **`UART_NUM_1`** with default pins **TX = GPIO47**, **RX = GPIO21**
(see `main/include/hf_fdo2_esp_uart.hpp` and the `using Uart = …` alias in each
`.cpp`). **`fdo2_minimal_example`** prints `#VERS`, `#IDNR`, and 1 Hz `#MRAW`.
**`fdo2_sensor_demo`** runs **200 ms** `#MOXY` plus **3 s** `#MRAW` for bench O₂
testing.

See [`examples/esp32/README.md`](https://github.com/hardfoc/hf-fdo2-driver/blob/main/examples/esp32/README.md) for matrix and override notes.

**Next:** [Troubleshooting →](troubleshooting.md)
