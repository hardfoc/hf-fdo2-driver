/**
 * @file esp_idf_stub.hpp
 * @brief The few ESP-IDF UART and FreeRTOS calls the ESP32 example port uses, modelled on the host.
 *
 * Lets the host tests compile `examples/esp32/main/include/hf_fdo2_esp_uart.hpp` and check its
 * timing. One tick is one millisecond. `uart_read_bytes()` behaves like ESP-IDF's: it returns when
 * `length` bytes have been read or `ticks_to_wait` has passed, whichever comes first, and the
 * simulated clock advances by the time it waited.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <utility>

using esp_err_t = int;
using TickType_t = std::uint32_t;
using uart_port_t = int;

constexpr esp_err_t ESP_OK = 0;
constexpr int UART_NUM_1 = 1;
constexpr int UART_PIN_NO_CHANGE = -1;
constexpr int UART_DATA_8_BITS = 3;
constexpr int UART_PARITY_DISABLE = 0;
constexpr int UART_STOP_BITS_1 = 1;
constexpr int UART_HW_FLOWCTRL_DISABLE = 0;
constexpr int UART_SCLK_DEFAULT = 0;

struct uart_config_t {
  int baud_rate;
  int data_bits;
  int parity;
  int stop_bits;
  int flow_ctrl;
  int rx_flow_ctrl_thresh;
  int source_clk;
};

inline TickType_t pdMS_TO_TICKS(std::uint32_t ms) {
  return ms;
}

namespace esp_idf_stub {

struct State {
  TickType_t now = 0;
  std::deque<std::pair<TickType_t, std::uint8_t>> rx; ///< (arrival tick, byte), in order
  std::string tx;
  int write_result = 0; ///< 0: uart_write_bytes() takes every byte; otherwise its return value
};

inline State& state() {
  static State s;
  return s;
}

inline void reset() {
  state() = State{};
}

/// Schedule @p bytes to arrive at tick @p at.
inline void arrive(TickType_t at, const std::string& bytes) {
  for (const char c : bytes) {
    state().rx.emplace_back(at, static_cast<std::uint8_t>(c));
  }
}

} // namespace esp_idf_stub

inline TickType_t xTaskGetTickCount() {
  return esp_idf_stub::state().now;
}

inline void vTaskDelay(TickType_t ticks) {
  esp_idf_stub::state().now += ticks;
}

inline esp_err_t uart_driver_install(uart_port_t, int, int, int, void*, int) {
  return ESP_OK;
}
inline esp_err_t uart_param_config(uart_port_t, const uart_config_t*) {
  return ESP_OK;
}
inline esp_err_t uart_set_pin(uart_port_t, int, int, int, int) {
  return ESP_OK;
}

inline int uart_write_bytes(uart_port_t, const void* src, std::size_t size) {
  if (esp_idf_stub::state().write_result != 0) {
    return esp_idf_stub::state().write_result; // e.g. -1: ESP_FAIL / parameter error
  }
  esp_idf_stub::state().tx.append(static_cast<const char*>(src), size);
  return static_cast<int>(size);
}

inline int uart_read_bytes(uart_port_t, void* buf, std::uint32_t length, TickType_t ticks_to_wait) {
  auto& s = esp_idf_stub::state();
  const TickType_t deadline = s.now + ticks_to_wait;
  auto* out = static_cast<std::uint8_t*>(buf);
  std::uint32_t got = 0;
  while (got < length) {
    if (!s.rx.empty() && s.rx.front().first <= s.now) {
      out[got++] = s.rx.front().second;
      s.rx.pop_front();
    } else if (!s.rx.empty() && s.rx.front().first <= deadline) {
      s.now = s.rx.front().first; // wait for the next byte
    } else {
      s.now = deadline; // nothing more within the wait: the whole timeout passes
      break;
    }
  }
  return static_cast<int>(got);
}

inline esp_err_t uart_flush_input(uart_port_t) {
  auto& s = esp_idf_stub::state();
  while (!s.rx.empty() && s.rx.front().first <= s.now) {
    s.rx.pop_front();
  }
  return ESP_OK;
}
