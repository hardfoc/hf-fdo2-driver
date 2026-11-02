/**
 * @file my_uart.hpp
 * @brief The UART port shown on the documentation's Porting page, compiled by the host build.
 *
 * Every platform call goes through the five `platform_*` functions declared below; a real port replaces them with
 * its SDK (ESP-IDF, STM32 HAL, Zephyr, Linux termios). The class is what the driver needs, nothing more.
 */
#pragma once

#include "fdo2.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
void platform_uart_write(const uint8_t* data, std::size_t len) noexcept;  // 19200 baud, 8N1
bool platform_uart_read_byte(uint8_t& byte) noexcept;                     // non-blocking: false when empty
void platform_uart_flush() noexcept;                                      // drop received bytes
uint32_t platform_millis() noexcept;
void platform_delay_ms(uint32_t ms) noexcept;

// --8<-- [start:port]
class MyUart : public fdo2::UartInterface<MyUart> {
public:
  // Send one command line (the driver adds the CR).
  void write(const uint8_t* data, std::size_t length) noexcept { platform_uart_write(data, length); }

  // Return one complete reply line, CR included, or 0 if none arrived within timeout_ms.
  // timeout_ms == 0 means "only if a whole line is already here".
  std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept {
    const uint32_t start = platform_millis();
    for (;;) {
      uint8_t byte = 0;
      while (platform_uart_read_byte(byte)) {
        if (len_ < sizeof(line_)) {
          line_[len_++] = byte;
        }
        if (byte == '\r') {                          // the module ends every reply with one CR
          const std::size_t n = (len_ < max) ? len_ : max;
          std::memcpy(out, line_, n);
          len_ = 0;
          return n;
        }
      }
      if (platform_millis() - start >= timeout_ms) {
        return 0;                                    // keep a partial line for the next call
      }
      platform_delay_ms(1);
    }
  }

  void flush_rx() noexcept {
    platform_uart_flush();
    len_ = 0;
  }

  // Optional: lets the driver and your code wait through UartInterface::delay_ms().
  void delay_ms_impl(uint32_t ms) noexcept { platform_delay_ms(ms); }

private:
  uint8_t line_[192]{};                              // longest reply: #MRAW, under 120 characters
  std::size_t len_ = 0;
};
// --8<-- [end:port]
