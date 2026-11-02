# =============================================================================
# hf-fdo2-driver — Root CMakeLists.txt
# =============================================================================
# Hardware-agnostic header-only UART driver for PyroScience FDO2-G2 (data sheet
# v5 §4: #VERS, #MOXY, #MRAW, …).
# Integrate via cmake/hf_fdo2_build_settings.cmake or the ESP-IDF component
# under examples/esp32/components/hf_fdo2.
# =============================================================================
cmake_minimum_required(VERSION 3.16)

project(hf_fdo2
  VERSION 1.0.0
  LANGUAGES CXX
)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_fdo2_build_settings.cmake)

add_library(${HF_FDO2_TARGET_NAME} INTERFACE)
target_include_directories(${HF_FDO2_TARGET_NAME}
    INTERFACE ${HF_FDO2_PUBLIC_INCLUDE_DIRS})
target_compile_features(${HF_FDO2_TARGET_NAME} INTERFACE cxx_std_17)

add_library(hf::fdo2 ALIAS ${HF_FDO2_TARGET_NAME})

option(HF_FDO2_ENABLE_WARNINGS "Enable compiler warnings for consumers" OFF)
if(HF_FDO2_ENABLE_WARNINGS)
  target_compile_options(${HF_FDO2_TARGET_NAME}
      INTERFACE -Wall -Wextra -Wpedantic)
endif()

#===============================================================================
# Optional: host unit tests and the compiled documentation snippets (tests/)
#===============================================================================
option(HF_FDO2_BUILD_TESTS "Build the host unit tests in tests/" OFF)
if(HF_FDO2_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hf_fdo2ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hf_fdo2Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hf_fdo2Config.cmake"
  INSTALL_DESTINATION lib/cmake/hf_fdo2
)
