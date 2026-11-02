# Examples

Two ESP32-S3 applications that read an FDO2-G2: a minimal bring-up and a dual-rate demo.

They run on an ESP32-S3 with the sensor on UART1 (MCU TX on GPIO47 to sensor RXD, MCU RX on GPIO21 from sensor
TXD, 19200 baud 8N1); the pins are template arguments in each app's source and the port is
[`hf_fdo2_esp_uart.hpp`](../examples/esp32/main/include/hf_fdo2_esp_uart.hpp). CI builds every app below on each
pull request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `fdo2_minimal_example` :material-star:{ title="featured" } | Smallest bring-up: identify the module, then read oxygen, pressure and humidity once a second | [`fdo2_minimal_example.cpp`](../examples/esp32/main/fdo2_minimal_example.cpp) | yes |
| `fdo2_sensor_demo` :material-star:{ title="featured" } | Oxygen every 200 ms and the full raw data every 3 s, with the status check on every reading | [`fdo2_sensor_demo.cpp`](../examples/esp32/main/fdo2_sensor_demo.cpp) | yes |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh fdo2_minimal_example Release
./scripts/flash_app.sh flash_monitor fdo2_minimal_example Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| See the smallest bring-up: boot delay, `#VERS`, `#IDNR`, a 1 Hz `#MRAW` loop | `fdo2_minimal_example.cpp` |
| Read fast `#MOXY` every 200 ms and slow `#MRAW` every 3 s, with the status check | `fdo2_sensor_demo.cpp` |
| Write an ESP-IDF port | `main/include/hf_fdo2_esp_uart.hpp` ([Porting](porting.md#reference-implementation)) |

The [example folder's README](../examples/esp32/README.md) has the wiring and the build commands.
