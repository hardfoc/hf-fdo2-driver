/**
 * @file fdo2.hpp
 * @brief Umbrella header for the HF-FDO2 (FDO2-G2) UART driver.
 *
 * @details Pulls in `fdo2::UartInterface`, typed readings (`MoxyReading`,
 *          `MrawReading`, `VersionInfo`), and `fdo2::Driver<UartT>` implementing
 *          the FDO2-G2 data sheet §4 command set. See `examples/esp32/` for an
 *          ESP-IDF UART adapter (default **19200** baud).
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include "fdo2_uart_interface.hpp"
#include "fdo2_types.hpp"
#include "fdo2_driver.hpp"
#include "fdo2_version.h"

namespace fdo2 {

/** @brief Driver version string (from the generated `fdo2_version.h`). */
inline const char* GetDriverVersion() noexcept {
    return HF_FDO2_VERSION_STRING;
}

}  // namespace fdo2
