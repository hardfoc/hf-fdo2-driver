# ESP32 examples — HF-FDO2 (PyroScience FDO2-G2)

ESP-IDF examples for the **FDO2-G2** UART API (**data sheet v5 §4**). Factory
default after power-up is **19200 baud**, 8N1.

## Prerequisites

- ESP-IDF **v5.4+** (CI uses `release/v5.5`)
- Python 3 with PyYAML (for `scripts/generate_matrix.py`)

## One-time setup

From `examples/esp32/`:

```bash
git submodule update --init --recursive
```

This pulls `scripts/` (**hf-espidf-project-tools**): `build_app.sh`, `flash_app.sh`,
and `generate_matrix.py` (same layout as other `hf-*-driver` submodules).

## Default wiring (ESP32-S3)

Examples use **`UART_NUM_1`** so **`UART_NUM_0`** stays available for the USB
serial console on typical devkits:

| MCU signal | GPIO (default) | Sensor / cable |
|------------|------------------|----------------|
| TX         | **47**           | module RX      |
| RX         | **21**           | module TX      |
| GND        | GND              | GND            |

If your harness swaps TX/RX, swap the two GPIO numbers in
`main/include/hf_fdo2_esp_uart.hpp` usage (the `using Uart = Fdo2EspIdfUart<...>`
alias) or change the template parameters in the `.cpp` file.

## Apps (`app_config.yml`)

| App | Role |
|-----|------|
| `fdo2_minimal_example` | Smallest bring-up: `#VERS`, `#IDNR`, 1 Hz `#MRAW` |
| `fdo2_sensor_demo` | Practical test: **200 ms** `#MOXY` + **3 s** `#MRAW` |

List matrix:

```bash
./scripts/build_app.sh list
```

## Build / flash

```bash
cd examples/esp32
./scripts/build_app.sh fdo2_minimal_example Debug
./scripts/flash_app.sh flash_monitor fdo2_minimal_example Debug
```

Sensor demo (recommended for bench O₂ testing):

```bash
./scripts/build_app.sh fdo2_sensor_demo Debug
./scripts/flash_app.sh flash_monitor fdo2_sensor_demo Debug
```

## Transport layer

Shared header: **`main/include/hf_fdo2_esp_uart.hpp`** — template
`hf_fdo2_examples::Fdo2EspIdfUart<PORT, TX_GPIO, RX_GPIO, BAUD>` implementing
`fdo2::UartInterface<>` for `fdo2::Driver` with **zero virtual calls**.

## Protocol reference

Use the **FDO2-G2** data sheet (v5) §4 as the authority. In-repo docs:
[`../../docs/uart-protocol.md`](../../docs/uart-protocol.md).
