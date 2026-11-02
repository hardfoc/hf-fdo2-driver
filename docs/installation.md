# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++17 library with no dependencies.

## Requirements

- A C++17 compiler. CI builds the tests with current GCC and Clang.
- CMake 3.16 or newer, for the CMake target.
- No RTOS, heap or exceptions: every call is `noexcept` and returns a `DriverResult` or a `DriverError`.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-fdo2-driver.git external/hf-fdo2-driver
git submodule update --init external/hf-fdo2-driver
```

Only `inc/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-fdo2-driver)
target_link_libraries(my_app PRIVATE hf::fdo2)
```

`hf::fdo2` is an `INTERFACE` target (alias of `hf_fdo2`). It carries the include directories, including the
folder with the generated `fdo2_version.h`, and requires C++17 (`cxx_std_17`).

| Option | Default | Effect |
| --- | --- | --- |
| `HF_FDO2_ENABLE_WARNINGS` | `OFF` | `-Wall -Wextra -Wpedantic` on targets that link the driver |
| `HF_FDO2_BUILD_TESTS` | `OFF` | Builds the host unit tests and documentation snippets in `tests/` ([Testing and CI](testing.md)) |

## ESP-IDF

The ESP-IDF component wrapper is [`examples/esp32/components/hf_fdo2/`](../examples/esp32/components/hf_fdo2/CMakeLists.txt).
Copy that folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the
`HF_FDO2_ROOT` cache variable to the driver checkout; the wrapper stops with a clear message when the path is
wrong. Your component then declares `REQUIRES hf_fdo2`. The wrapper itself requires the IDF `driver` component.

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_fdo2_build_settings.cmake`](../cmake/hf_fdo2_build_settings.cmake) | Include directories, version, IDF requirements: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | `INTERFACE` target for host builds and CI |
| ESP-IDF | [`examples/esp32/components/hf_fdo2/CMakeLists.txt`](../examples/esp32/components/hf_fdo2/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `fdo2_version.h` at configure time. Read it at run
time:

```cpp
const char* v = fdo2::GetDriverVersion();   // "1.0.0"
```

## Next

[Quickstart](quickstart.md) wires the sensor and reads oxygen; [Porting](porting.md) writes the UART interface
for your MCU.
