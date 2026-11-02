/**
 * @file fdo2_types.hpp
 * @brief FDO2-G2 UART types, scaling, and status decoding (PyroScience data sheet v5).
 *
 * @details Numeric scaling and command names follow the **FDO2-G2** data sheet
 *          (document version v5, 03/2026), §4 Communication Interface. This is
 *          distinct from the broader “PyroScience Unified Protocol” used on
 *          laboratory / multi-channel instruments (`MEA`, eighteen-word results,
 *          etc.).
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cmath>
#include <cstdint>
#include <string_view>

namespace fdo2 {

/// Default UART speed after power-up per FDO2-G2 data sheet §4.
constexpr uint32_t kFdo2G2DefaultBaud = 19200U;

/// Typical boot delay before the module accepts commands (data sheet: ~1 s).
constexpr uint32_t kFdo2G2PowerUpSettleMs = 1100U;

/// Driver-level error codes (stable for logging / telemetry).
enum class DriverError : uint8_t {
    None = 0,
    InvalidParameter,
    Timeout,
    BufferOverflow,
    ProtocolError,
    DeviceError,
    CrcMismatch,  ///< `#CRCE 1` checksum suffix did not match the reply, or was required and missing.
};

constexpr std::string_view ToString(DriverError e) noexcept {
    switch (e) {
        case DriverError::None:              return "None";
        case DriverError::InvalidParameter: return "InvalidParameter";
        case DriverError::Timeout:          return "Timeout";
        case DriverError::BufferOverflow:   return "BufferOverflow";
        case DriverError::ProtocolError:    return "ProtocolError";
        case DriverError::DeviceError:      return "DeviceError";
        case DriverError::CrcMismatch:      return "CrcMismatch";
    }
    return "?";
}

template <typename T>
struct DriverResult {
    T           value{};
    DriverError error{DriverError::None};

    constexpr bool ok() const noexcept { return error == DriverError::None; }
    constexpr explicit operator bool() const noexcept { return ok(); }

    static constexpr DriverResult success(T v) noexcept { return {v, DriverError::None}; }
    static constexpr DriverResult failure(DriverError e) noexcept { return {T{}, e}; }
};

template <>
struct DriverResult<void> {
    DriverError error{DriverError::None};
    constexpr bool ok() const noexcept { return error == DriverError::None; }
    constexpr explicit operator bool() const noexcept { return ok(); }
    static constexpr DriverResult success() noexcept { return {DriverError::None}; }
    static constexpr DriverResult failure(DriverError e) noexcept { return {e}; }
};

/**
 * @brief `#VERS D N R S` fields (FDO2-G2 data sheet §4.3).
 *
 * Firmware revision **R** is encoded as integer hundreds, e.g. R=328 → 3.28.
 * Expected for FDO2-G2: D=8, N=1, S=15 when all internal sensors are present.
 */
struct VersionInfo {
    int32_t device_id{};          ///< D (8 for FDO2-G2).
    int32_t num_channels{};     ///< N (1 for FDO2-G2).
    int32_t firmware_revision{};///< R (e.g. 328 → 3.28).
    int32_t sensor_types{};     ///< S bit mask: bit0 O2, bit1 temp housing, bit2 pressure, bit3 RH.
};

/// @return Firmware as major.minor (R / 100.0).
constexpr double FirmwareRevisionToDouble(int32_t r) noexcept {
    return static_cast<double>(r) * 0.01;
}

/**
 * @brief Decoded `#MOXY O T S` sample (data sheet §4.3).
 *
 * Raw **O** is signed 32-bit in **10⁻³ hPa** (milli-hPa). **T** is **m°C**
 * (milli-degrees C). **S** is **unsigned 32-bit** status (warning/error bits).
 */
struct MoxyReading {
    int32_t  o_raw{};       ///< O as received (1e-3 hPa).
    int32_t  t_raw{};       ///< T as received (m°C).
    uint32_t status_u32{};  ///< S as received.

    double p_o2_hpa{}; ///< Partial pressure O₂ [hPa] = O × 10⁻³.
    double temp_c{};   ///< Temperature [°C] = T × 10⁻³.
};

/**
 * @brief Decoded `#MRAW O T S D I A P H` (data sheet §4.3).
 *
 * **D** millidegrees phase, **I** and **A** in microvolts (µV) → divide by 1000 for mV,
 * **P** in microbar (µbar) → ×10⁻³ for mbar (1 mbar = 1000 µbar), **H** in milli-%RH (m%RH) → ×10⁻³ for %RH.
 */
struct MrawReading : MoxyReading {
    int32_t d_raw{}; ///< D (m°).
    int32_t i_raw{}; ///< I (µV).
    int32_t a_raw{}; ///< A (µV).
    int32_t p_raw{}; ///< P (µbar at connector / back side).
    int32_t h_raw{}; ///< H (m%RH).

    double dphi_deg{};          ///< D × 10⁻³ degrees.
    double signal_intensity_mv{}; ///< I × 10⁻³ mV.
    double ambient_light_mv{};    ///< A × 10⁻³ mV.
    double pressure_mbar{};       ///< P × 10⁻³ mbar (≈ hPa).
    double rh_in_housing_pct{};   ///< H × 10⁻³ %RH.
};

namespace moxy_status {

constexpr uint32_t kWarnAutoAmpReduced = 1U << 0;
constexpr uint32_t kFatalSignalTooLow  = 1U << 1;
constexpr uint32_t kFatalSignalTooHigh = 1U << 2;
constexpr uint32_t kFatalRefTooLow     = 1U << 3;
constexpr uint32_t kFatalRefTooHigh    = 1U << 4;
constexpr uint32_t kFatalTempSensor    = 1U << 5;
constexpr uint32_t kWarnHumidityHigh   = 1U << 7;
constexpr uint32_t kErrPressureSensor    = 1U << 9;
constexpr uint32_t kErrHumiditySensor    = 1U << 10;

}  // namespace moxy_status

/// Data sheet: normal operation implies S is 0 or 1 only.
constexpr bool MoxyStatusOkForOxygen(uint32_t s) noexcept { return s <= 1U; }

inline MoxyReading DecodeMoxy(int32_t o_raw, int32_t t_raw, uint32_t s) noexcept {
    MoxyReading m{};
    m.o_raw      = o_raw;
    m.t_raw      = t_raw;
    m.status_u32 = s;
    m.p_o2_hpa   = static_cast<double>(o_raw) * 1e-3;
    m.temp_c     = static_cast<double>(t_raw) * 1e-3;
    return m;
}

inline MrawReading DecodeMraw(int32_t o_raw, int32_t t_raw, uint32_t s, int32_t d_raw,
                               int32_t i_raw, int32_t a_raw, int32_t p_raw,
                               int32_t h_raw) noexcept {
    MrawReading r{};
    static_cast<MoxyReading&>(r) = DecodeMoxy(o_raw, t_raw, s);
    r.d_raw                  = d_raw;
    r.i_raw                  = i_raw;
    r.a_raw                  = a_raw;
    r.p_raw                  = p_raw;
    r.h_raw                  = h_raw;
    r.dphi_deg               = static_cast<double>(d_raw) * 1e-3;
    r.signal_intensity_mv    = static_cast<double>(i_raw) * 1e-3;
    r.ambient_light_mv       = static_cast<double>(a_raw) * 1e-3;
    r.pressure_mbar          = static_cast<double>(p_raw) * 1e-3;  /* µbar → mbar */
    r.rh_in_housing_pct      = static_cast<double>(h_raw) * 1e-3;
    return r;
}

/**
 * @brief Volume % O₂ when p_air at the sensing membrane equals back-side pressure **P**.
 *
 * @param p_o2_hpa Partial pressure O₂ [hPa] (from `#MOXY` / `#MRAW` **O** scaling).
 * @param p_air_mbar Ambient / vent-path pressure [mbar] (from `#MRAW` **P** scaling;
 *                   mbar ≈ hPa for this ratio per data sheet §2).
 */
inline double VolumePercentO2(double p_o2_hpa, double p_air_mbar) noexcept {
    if (p_air_mbar <= 0.0 || !std::isfinite(p_air_mbar) || !std::isfinite(p_o2_hpa)) {
        return 0.0;
    }
    return 100.0 * p_o2_hpa / p_air_mbar;
}

}  // namespace fdo2
