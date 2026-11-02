# Testing and CI

How the driver is checked: host unit tests against a model of the sensor, compiled documentation, ESP32 builds
and static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a model of the FDO2-G2's command interface: it parses each command line,
answers with the datasheet's own example values, and can stay silent, answer `#ERRO`, append a CRC suffix or
deliver a reply later. Every command the driver sends is logged, so tests check what went on the wire.

```bash
cmake -S . -B build-host -DHF_FDO2_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_driver.cpp`](../tests/test_driver.cpp) | Commands, scaling of every field, status, `#ERRO` codes, timeouts, CRC verification, the split exchange |
| [`tests/mock_fdo2.hpp`](../tests/mock_fdo2.hpp) | The sensor model and command log |
| [`tests/test_harness.hpp`](../tests/test_harness.hpp) | A dependency-free test registry and `CHECK` macros |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_uart.hpp), which the host build
compiles (target `hf_fdo2_doc_snippets`) with the driver instantiated over the documented port. A change to the API
that breaks a page breaks the build first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every app in `examples/esp32/app_config.yml`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware test: `fdo2_minimal_example` prints the identification and a
reading every second over the serial monitor. In air at sea level, expect about 210 hPa and a status of 0 or 1.
