/**
 * @file fdo2_uart_interface.hpp
 * @brief CRTP byte transport for the PyroScience Unified Protocol (PSUP).
 *
 * @details The driver only moves bytes; each platform provides a concrete
 *          adapter inheriting this CRTP base and implementing:
 *
 *            - `bool write(const uint8_t* data, std::size_t length);` (true = every
 *              byte accepted, false = serial error; must return `bool`)
 *            - `std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms);`
 *            - `void flush_rx();`
 *
 *          PSUP lines are ASCII, terminated with carriage return (`\\r`, 0x0D).
 *          Typical UART framing is 8N1 at 115200 or 19200 baud (see the device
 *          datasheet). See `examples/esp32/` in this repository for an ESP-IDF
 *          adapter.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace fdo2 {

/**
 * @brief CRTP base for PSUP serial transport.
 * @tparam Derived Concrete adapter type (e.g. `EspUartAdapter`).
 */
template <typename Derived>
class UartInterface {
public:
    /**
     * @brief Send @p length bytes.
     * @return `true` when every byte was accepted, `false` on a serial error (the driver then
     *         fails the call with `DriverError::SerialError` without waiting for a reply).
     */
    bool write(const uint8_t* data, std::size_t length) noexcept {
        static_assert(
            std::is_same_v<decltype(std::declval<Derived&>().write(data, length)), bool>,
            "fdo2::UartInterface: the port's write(const uint8_t*, std::size_t) must return bool "
            "(true = every byte accepted, false = serial error)");
        return static_cast<Derived*>(this)->write(data, length);
    }

    std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept {
        return static_cast<Derived*>(this)->read(out, max, timeout_ms);
    }

    void flush_rx() noexcept { static_cast<Derived*>(this)->flush_rx(); }

    void delay_ms(uint32_t ms) noexcept {
        if constexpr (HasDelay<Derived>::value) {
            static_cast<Derived*>(this)->delay_ms_impl(ms);
        } else {
            (void)ms;
        }
    }

private:
    /// @cond INTERNAL
    template <typename, typename = void>
    struct HasDelay : std::false_type {};
    template <typename T>
    struct HasDelay<T, std::void_t<decltype(std::declval<T>().delay_ms_impl(0U))>>
        : std::true_type {};
    /// @endcond
};

}  // namespace fdo2
