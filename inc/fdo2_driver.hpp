/**
 * @file fdo2_driver.hpp
 * @brief FDO2-G2 UART command client (PyroScience data sheet v5, §4).
 *
 * @details Implements the **FDO2-G2** command set: `#VERS`, `#IDNR`, `#MOXY`,
 *          `#MRAW`, `#LOGO`. Lines are ASCII, terminated with `\\r` (optional
 *          `\\n` after `\\r` per §4.1). If optional CRC is enabled (`#CRCE 1`),
 *          responses end with `: C` before `\\r` (§4.2), C being the CRC-16/MODBUS
 *          of every byte before the `:` in decimal; the driver verifies it and
 *          rejects a mismatch with `DriverError::CrcMismatch`.
 *
 *          Default UART is **19200** 8N1 after power-up; allow ~1 s settle time
 *          before the first transaction.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include "fdo2_types.hpp"
#include "fdo2_uart_interface.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace fdo2 {
namespace detail {

inline bool AppendCmd(char* buf, std::size_t cap, std::string_view cmd) noexcept {
    if (cmd.size() + 2U > cap) {
        return false;
    }
    std::memcpy(buf, cmd.data(), cmd.size());
    buf[cmd.size()]      = '\r';
    buf[cmd.size() + 1U] = '\0';
    return true;
}

/**
 * @brief CRC-16/MODBUS (CRC-16-IBM, reflected polynomial 0xA001, initial value 0xFFFF, no final XOR).
 * @details Data sheet §4.2: the checksum the module appends with `#CRCE 1`, computed over the reply
 *          from its first character up to, not including, the `:`. Check value: "123456789" -> 0x4B37.
 */
inline uint16_t Crc16Modbus(const char* data, std::size_t len) noexcept {
    uint16_t crc = 0xFFFFU;
    for (std::size_t i = 0; i < len; ++i) {
        crc = static_cast<uint16_t>(crc ^ static_cast<unsigned char>(data[i]));
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 1U) != 0U) {
                crc = static_cast<uint16_t>((crc >> 1U) ^ 0xA001U);
            } else {
                crc = static_cast<uint16_t>(crc >> 1U);
            }
        }
    }
    return crc;
}

/**
 * @brief Verify and remove the optional CRC suffix of a reply line (CR already removed).
 * @details Data sheet §4.2: with `#CRCE 1` every reply ends in `": C"` before the CR, where C is the
 *          CRC-16 as a decimal ASCII string, computed over every byte before the `:` (spaces included).
 *          Replies never contain `:` otherwise, so a `:` marks the suffix. On success the line is cut
 *          at the `:`.
 * @param require  When true, a line without the suffix is rejected (module known to have `#CRCE 1`).
 * @return None (no suffix and not required, or suffix valid); CrcMismatch (checksum differs, or
 *         required and missing); ProtocolError (a `:` not followed by a decimal 0..65535).
 */
inline DriverError VerifyAndStripCrc(char* line, bool require) noexcept {
    char* colon = std::strrchr(line, ':');
    if (colon == nullptr) {
        return require ? DriverError::CrcMismatch : DriverError::None;
    }
    const char* p = colon + 1;
    while (*p == ' ') {
        ++p;
    }
    uint32_t value  = 0;
    std::size_t nd  = 0;
    while (std::isdigit(static_cast<unsigned char>(*p)) != 0) {
        value = value * 10U + static_cast<uint32_t>(*p - '0');
        ++p;
        if (++nd > 5U) {
            return DriverError::ProtocolError;
        }
    }
    if (nd == 0U || *p != '\0' || value > 0xFFFFU) {
        return DriverError::ProtocolError;
    }
    const std::size_t covered = static_cast<std::size_t>(colon - line);
    if (Crc16Modbus(line, covered) != static_cast<uint16_t>(value)) {
        return DriverError::CrcMismatch;
    }
    *colon = '\0';
    return DriverError::None;
}

/// @deprecated Strips a CRC suffix without checking it; use VerifyAndStripCrc().
[[deprecated("use VerifyAndStripCrc(), which checks the checksum")]]
inline void StripOptionalModbusCrcSuffix(char* line) noexcept {
    if (line != nullptr) {
        char* colon = std::strrchr(line, ':');
        if (colon != nullptr) {
            *colon = '\0';
        }
    }
}

template <typename UartT>
inline DriverError ReadAsciiLine(UartT& uart, char* buf, std::size_t cap, uint32_t timeout_ms,
                                 bool require_crc = false) noexcept {
    if (cap < 2) {
        return DriverError::InvalidParameter;
    }
    uint8_t tmp[512];
    const std::size_t max_rx = sizeof(tmp) - 1U;
    const std::size_t n      = uart.read(tmp, max_rx, timeout_ms);
    if (n == 0U) {
        return DriverError::Timeout;
    }
    std::size_t pos = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const char c = static_cast<char>(tmp[i]);
        if (c == '\r') {
            break;
        }
        if (c == '\n') {
            continue;
        }
        if (pos + 1 >= cap) {
            return DriverError::BufferOverflow;
        }
        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return VerifyAndStripCrc(buf, require_crc);
}

inline const char* SkipWs(const char* p) noexcept {
    while (*p != '\0' && std::isspace(static_cast<unsigned char>(*p)) != 0) {
        ++p;
    }
    return p;
}

inline DriverError Tokenize(const char* line, char* store, std::size_t store_cap,
                             const char* tokens[], std::size_t max_tokens,
                             std::size_t* out_count) noexcept {
    *out_count = 0;
    const char* p = SkipWs(line);
    if (*p == '\0') {
        return DriverError::ProtocolError;
    }
    std::size_t bi = 0;
    while (*p != '\0' && *out_count < max_tokens) {
        if (bi + 1U >= store_cap) {
            return DriverError::BufferOverflow;
        }
        const std::size_t start = bi;
        tokens[*out_count]      = store + start;
        while (*p != '\0' && std::isspace(static_cast<unsigned char>(*p)) == 0) {
            if (bi + 1U >= store_cap) {
                return DriverError::BufferOverflow;
            }
            store[bi++] = *p++;
        }
        if (bi + 1U >= store_cap) {
            return DriverError::BufferOverflow;
        }
        store[bi++] = '\0';
        ++(*out_count);
        p = SkipWs(p);
    }
    if (*p != '\0') {
        return DriverError::ProtocolError;
    }
    return DriverError::None;
}

inline int32_t ParseI32(const char* s, bool* ok) noexcept {
    char* end = nullptr;
    const long v = std::strtol(s, &end, 10);
    if (ok != nullptr) {
        *ok = (end != s) && (*end == '\0');
    }
    constexpr long kMin = -2147483648L;
    constexpr long kMax = 2147483647L;
    if (v < kMin || v > kMax) {
        if (ok != nullptr) {
            *ok = false;
        }
        return 0;
    }
    return static_cast<int32_t>(v);
}

inline uint32_t ParseU32(const char* s, bool* ok) noexcept {
    char* end = nullptr;
    const unsigned long v = std::strtoul(s, &end, 10);
    if (ok != nullptr) {
        *ok = (end != s) && (*end == '\0');
    }
    return static_cast<uint32_t>(v);
}

inline uint64_t ParseU64(const char* s, bool* ok) noexcept {
    char* end = nullptr;
    const unsigned long long v = std::strtoull(s, &end, 10);
    if (ok != nullptr) {
        *ok = (end != s) && (*end == '\0');
    }
    return static_cast<uint64_t>(v);
}

inline bool IsErroHeader(const char* tok0) noexcept {
    return std::strncmp(tok0, "#ERRO", 5) == 0;
}

}  // namespace detail

/**
 * @brief FDO2-G2 UART client.
 * @tparam UartT Concrete type inheriting `UartInterface<UartT>`.
 */
template <typename UartT>
class Driver {
public:
    explicit Driver(UartT& uart) noexcept : uart_(uart) {}

    void SetLineTimeoutMs(uint32_t ms) noexcept { line_timeout_ms_ = ms; }
    void SetMeasureTimeoutMs(uint32_t ms) noexcept { measure_timeout_ms_ = ms; }
    void SetSlowCommandTimeoutMs(uint32_t ms) noexcept { slow_timeout_ms_ = ms; }

    /**
     * @brief Require the `#CRCE 1` checksum on every reply (data sheet §4.2).
     * @details A reply that carries the suffix is always verified. With this set, a reply without
     *          it is rejected too (`CrcMismatch`), so a corrupted `:` cannot hide a bad reply. Set it
     *          only for a module whose CRC is enabled; the driver never sends `#CRCE`.
     */
    void SetCrcRequired(bool required) noexcept { crc_required_ = required; }
    bool IsCrcRequired() const noexcept { return crc_required_; }

    uint32_t GetLineTimeoutMs() const noexcept { return line_timeout_ms_; }
    uint32_t GetMeasureTimeoutMs() const noexcept { return measure_timeout_ms_; }
    int32_t  LastDeviceErrorCode() const noexcept { return last_device_error_; }

    DriverResult<VersionInfo> ReadVersion() noexcept {
        last_device_error_ = 0;
        char tx[16];
        if (!detail::AppendCmd(tx, sizeof(tx), "#VERS")) {
            return DriverResult<VersionInfo>::failure(DriverError::InvalidParameter);
        }
        char line[160];
        const auto err = TransactLine(tx, line, sizeof(line));
        if (err != DriverError::None) {
            return DriverResult<VersionInfo>::failure(err);
        }
        return ParseVersionLine(line, &last_device_error_);
    }

    DriverResult<uint64_t> ReadUniqueId() noexcept {
        last_device_error_ = 0;
        char tx[16];
        if (!detail::AppendCmd(tx, sizeof(tx), "#IDNR")) {
            return DriverResult<uint64_t>::failure(DriverError::InvalidParameter);
        }
        char line[96];
        const auto err = TransactLine(tx, line, sizeof(line));
        if (err != DriverError::None) {
            return DriverResult<uint64_t>::failure(err);
        }
        char tstore[128];
        const char* tok[8];
        std::size_t nt = 0;
        if (detail::Tokenize(line, tstore, sizeof(tstore), tok, 8, &nt) != DriverError::None) {
            return DriverResult<uint64_t>::failure(DriverError::ProtocolError);
        }
        if (nt >= 2 && detail::IsErroHeader(tok[0])) {
            return FailErro<uint64_t>(tok, nt);
        }
        if (nt != 2U || std::strncmp(tok[0], "#IDNR", 5) != 0) {
            return DriverResult<uint64_t>::failure(DriverError::ProtocolError);
        }
        bool ok = false;
        const uint64_t id = detail::ParseU64(tok[1], &ok);
        if (!ok) {
            return DriverResult<uint64_t>::failure(DriverError::ProtocolError);
        }
        return DriverResult<uint64_t>::success(id);
    }

    /// Single oxygen + temperature + status round-trip (typically &lt; ~150 ms for M=2).
    DriverResult<MoxyReading> MeasureMoxy(uint32_t timeout_ms = 0U) noexcept {
        last_device_error_ = 0;
        char tx[12];
        if (!detail::AppendCmd(tx, sizeof(tx), "#MOXY")) {
            return DriverResult<MoxyReading>::failure(DriverError::InvalidParameter);
        }
        char line[96];
        const uint32_t tmo = (timeout_ms != 0U) ? timeout_ms : measure_timeout_ms_;
        const auto err       = TransactLine(tx, line, sizeof(line), tmo);
        if (err != DriverError::None) {
            return DriverResult<MoxyReading>::failure(err);
        }
        return ParseMoxyLine(line, &last_device_error_);
    }

    /// Same measurement plus raw optics / vent-path pressure / internal RH.
    DriverResult<MrawReading> MeasureMraw(uint32_t timeout_ms = 0U) noexcept {
        last_device_error_ = 0;
        char tx[12];
        if (!detail::AppendCmd(tx, sizeof(tx), "#MRAW")) {
            return DriverResult<MrawReading>::failure(DriverError::InvalidParameter);
        }
        char line[192];
        const uint32_t tmo = (timeout_ms != 0U) ? timeout_ms : measure_timeout_ms_;
        const auto err       = TransactLine(tx, line, sizeof(line), tmo);
        if (err != DriverError::None) {
            return DriverResult<MrawReading>::failure(err);
        }
        return ParseMrawLine(line, &last_device_error_);
    }

    /**
     * @name Split exchange — send now, parse the reply later
     * @details For a caller that serves other devices while the probe
     *          measures (~150 ms): @ref SendMeasure writes the command and
     *          returns; when the transport reports a complete line,
     *          @ref TryReadReplyLine takes it without waiting and
     *          @ref ParseMoxyLine / @ref ParseMrawLine decode it. The caller
     *          owns the reply timeout. Same wire protocol as @ref MeasureMoxy.
     * @{
     */
    /// Which command @ref SendMeasure starts (`Version` = the `#VERS` identity handshake).
    enum class Measure : uint8_t { Moxy, Mraw, Version };

    /// Drop stale input, then send `#MOXY` / `#MRAW`. Does not wait for the reply.
    DriverError SendMeasure(Measure m) noexcept {
        last_device_error_ = 0;
        char tx[12];
        const char* cmd = (m == Measure::Mraw) ? "#MRAW" : (m == Measure::Version) ? "#VERS" : "#MOXY";
        if (!detail::AppendCmd(tx, sizeof(tx), cmd)) {
            return DriverError::InvalidParameter;
        }
        uart_.flush_rx();
        uart_.write(reinterpret_cast<const uint8_t*>(tx), std::strlen(tx));
        return DriverError::None;
    }

    /**
     * @brief Take one complete reply line if the transport already has it.
     * @return None with @p line filled; Timeout when no complete line is waiting.
     */
    DriverError TryReadReplyLine(char* line, std::size_t line_cap) noexcept {
        return detail::ReadAsciiLine(uart_, line, line_cap, 0U, crc_required_);
    }

    /// Decode a `#VERS` reply line. @p device_error receives the `#ERRO` code (may be null).
    static DriverResult<VersionInfo> ParseVersionLine(const char* line,
                                                      int32_t* device_error) noexcept {
        char tstore[128];
        const char* tok[8];
        std::size_t nt = 0;
        if (detail::Tokenize(line, tstore, sizeof(tstore), tok, 8, &nt) != DriverError::None) {
            return DriverResult<VersionInfo>::failure(DriverError::ProtocolError);
        }
        if (nt >= 2 && detail::IsErroHeader(tok[0])) {
            return FailErroStatic<VersionInfo>(tok, nt, device_error);
        }
        if (nt < 5U || std::strncmp(tok[0], "#VERS", 5) != 0) {
            return DriverResult<VersionInfo>::failure(DriverError::ProtocolError);
        }
        bool ok = true;
        VersionInfo v{};
        v.device_id          = detail::ParseI32(tok[1], &ok);
        v.num_channels       = detail::ParseI32(tok[2], &ok);
        v.firmware_revision  = detail::ParseI32(tok[3], &ok);
        v.sensor_types       = detail::ParseI32(tok[4], &ok);
        if (!ok) {
            return DriverResult<VersionInfo>::failure(DriverError::ProtocolError);
        }
        return DriverResult<VersionInfo>::success(v);
    }

    /// Decode a `#MOXY` reply line. @p device_error receives the `#ERRO` code (may be null).
    static DriverResult<MoxyReading> ParseMoxyLine(const char* line,
                                                   int32_t* device_error) noexcept {
        char tstore[128];
        const char* tok[8];
        std::size_t nt = 0;
        if (detail::Tokenize(line, tstore, sizeof(tstore), tok, 8, &nt) != DriverError::None) {
            return DriverResult<MoxyReading>::failure(DriverError::ProtocolError);
        }
        if (nt >= 2 && detail::IsErroHeader(tok[0])) {
            return FailErroStatic<MoxyReading>(tok, nt, device_error);
        }
        if (nt != 4U || std::strncmp(tok[0], "#MOXY", 5) != 0) {
            return DriverResult<MoxyReading>::failure(DriverError::ProtocolError);
        }
        bool ok = true;
        const int32_t o = detail::ParseI32(tok[1], &ok);
        const int32_t t = detail::ParseI32(tok[2], &ok);
        const uint32_t s = detail::ParseU32(tok[3], &ok);
        if (!ok) {
            return DriverResult<MoxyReading>::failure(DriverError::ProtocolError);
        }
        return DriverResult<MoxyReading>::success(DecodeMoxy(o, t, s));
    }

    /// Decode a `#MRAW` reply line. @p device_error receives the `#ERRO` code (may be null).
    static DriverResult<MrawReading> ParseMrawLine(const char* line,
                                                   int32_t* device_error) noexcept {
        char tstore[256];
        const char* tok[16];
        std::size_t nt = 0;
        if (detail::Tokenize(line, tstore, sizeof(tstore), tok, 16, &nt) != DriverError::None) {
            return DriverResult<MrawReading>::failure(DriverError::ProtocolError);
        }
        if (nt >= 2 && detail::IsErroHeader(tok[0])) {
            return FailErroStatic<MrawReading>(tok, nt, device_error);
        }
        if (nt != 9U || std::strncmp(tok[0], "#MRAW", 5) != 0) {
            return DriverResult<MrawReading>::failure(DriverError::ProtocolError);
        }
        bool ok = true;
        const int32_t o = detail::ParseI32(tok[1], &ok);
        const int32_t t = detail::ParseI32(tok[2], &ok);
        const uint32_t s = detail::ParseU32(tok[3], &ok);
        const int32_t d = detail::ParseI32(tok[4], &ok);
        const int32_t i = detail::ParseI32(tok[5], &ok);
        const int32_t a = detail::ParseI32(tok[6], &ok);
        const int32_t p = detail::ParseI32(tok[7], &ok);
        const int32_t h = detail::ParseI32(tok[8], &ok);
        if (!ok) {
            return DriverResult<MrawReading>::failure(DriverError::ProtocolError);
        }
        return DriverResult<MrawReading>::success(DecodeMraw(o, t, s, d, i, a, p, h));
    }
    /** @} */

    /// Flash the status LED (identification).
    DriverResult<void> FlashLogo() noexcept {
        last_device_error_ = 0;
        return SimpleEchoCommand("#LOGO", 5);
    }

private:
    template <typename T>
    static DriverResult<T> FailErroStatic(const char* tok[], std::size_t nt,
                                          int32_t* device_error) noexcept {
        int32_t code = -1;
        if (nt >= 2) {
            bool ok = false;
            code = detail::ParseI32(tok[1], &ok);
            if (!ok) {
                code = -1;
            }
        }
        if (device_error != nullptr) {
            *device_error = code;
        }
        return DriverResult<T>::failure(DriverError::DeviceError);
    }

    template <typename T>
    DriverResult<T> FailErro(const char* tok[], std::size_t nt) noexcept {
        if (nt >= 2) {
            bool ok = false;
            last_device_error_ = detail::ParseI32(tok[1], &ok);
            if (!ok) {
                last_device_error_ = -1;
            }
        } else {
            last_device_error_ = -1;
        }
        return DriverResult<T>::failure(DriverError::DeviceError);
    }

    DriverResult<void> SimpleEchoCommand(const char* cmd, std::size_t cmd_len) noexcept {
        char tx[16];
        if (cmd_len + 2U > sizeof(tx)) {
            return DriverResult<void>::failure(DriverError::InvalidParameter);
        }
        char line[32];
        std::memcpy(tx, cmd, cmd_len);
        tx[cmd_len]     = '\r';
        tx[cmd_len + 1] = '\0';
        const auto err = TransactLine(tx, line, sizeof(line), line_timeout_ms_);
        if (err != DriverError::None) {
            return DriverResult<void>::failure(err);
        }
        char tstore[48];
        const char* tok[4];
        std::size_t nt = 0;
        if (detail::Tokenize(line, tstore, sizeof(tstore), tok, 4, &nt) != DriverError::None) {
            return DriverResult<void>::failure(DriverError::ProtocolError);
        }
        if (nt >= 2 && detail::IsErroHeader(tok[0])) {
            return FailErro<void>(tok, nt);
        }
        if (nt < 1U || std::strncmp(tok[0], cmd, cmd_len) != 0) {
            return DriverResult<void>::failure(DriverError::ProtocolError);
        }
        return DriverResult<void>::success();
    }

    DriverError TransactLine(const char* tx, char* line, std::size_t line_cap,
                             uint32_t timeout_ms = 0U) noexcept {
        uart_.flush_rx();
        uart_.write(reinterpret_cast<const uint8_t*>(tx), std::strlen(tx));
        const uint32_t tmo = (timeout_ms != 0U) ? timeout_ms : line_timeout_ms_;
        return detail::ReadAsciiLine(uart_, line, line_cap, tmo, crc_required_);
    }

    UartT&   uart_;
    uint32_t line_timeout_ms_{400};
    uint32_t measure_timeout_ms_{250};
    uint32_t slow_timeout_ms_{12000};
    int32_t  last_device_error_{0};
    bool     crc_required_{false};
};

}  // namespace fdo2
