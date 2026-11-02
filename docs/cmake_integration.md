# CMake integration

## Standalone consumer

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_project LANGUAGES CXX)

add_subdirectory(third_party/hf-fdo2-driver)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE hf::fdo2)
target_compile_features(my_app PRIVATE cxx_std_17)
```

`hf::fdo2` is an `INTERFACE` target. Include paths cover:

- `inc/` — public headers
- `${CMAKE_CURRENT_BINARY_DIR}/hf_fdo2_generated/` — generated `fdo2_version.h`

## Variables of interest (`cmake/hf_fdo2_build_settings.cmake`)

| Variable | Role |
|----------|------|
| `HF_FDO2_TARGET_NAME` | CMake target name (`hf_fdo2`) |
| `HF_FDO2_PUBLIC_INCLUDE_DIRS` | Include directories for IDF `COMPONENT` wrappers |
| `HF_FDO2_IDF_REQUIRES` | ESP-IDF component dependencies (`driver`) |

## Package config

The root `CMakeLists.txt` also emits `hf_fdo2Config.cmake` / version files for
install-tree consumers (`CMakePackageConfigHelpers`).

**Next:** [API reference →](api_reference.md)
