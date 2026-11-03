/**
 * @file test_esp_uart.cpp
 * @brief The ESP32 example port (`hf_fdo2_esp_uart.hpp`) over a host model of the ESP-IDF UART.
 *
 * Data sheet v5 §4.1: every reply ends with a single CR, so the port can hand the line over as
 * soon as the CR arrives instead of waiting for its whole timeout.
 */
#include "esp_idf_stub.hpp"
#include "test_harness.hpp"

#include "hf_fdo2_esp_uart.hpp"

#include <cstring>

namespace {
using EspUart = hf_fdo2_examples::Fdo2EspIdfUart<UART_NUM_1, 17, 16>;
namespace stub = esp_idf_stub;
} // namespace

TEST_CASE("ESP32 port: a command returns when the reply's CR arrives, not after the timeout") {
  stub::reset();
  EspUart uart;
  fdo2::Driver<EspUart> dev(uart);
  stub::arrive(150, "#MOXY 203456 17892 0\r"); // measurement done after 150 ms
  const auto m = dev.MeasureMoxy();            // 250 ms timeout
  REQUIRE(m.ok());
  CHECK_EQ(m.value.o_raw, 203456);
  CHECK_EQ(stub::state().now, 150U);
  CHECK(stub::state().tx == "#MOXY\r");
}

TEST_CASE("ESP32 port: a silent module takes the whole timeout") {
  stub::reset();
  EspUart uart;
  uint8_t buf[64];
  CHECK_EQ(uart.read(buf, sizeof(buf), 100), 0U);
  CHECK_EQ(stub::state().now, 100U);
}

TEST_CASE("ESP32 port: a partial line is kept across polls with timeout 0") {
  stub::reset();
  EspUart uart;
  stub::arrive(0, "#MOXY 2034");
  stub::arrive(10, "56 17892 0\r");
  uint8_t buf[64];
  CHECK_EQ(uart.read(buf, sizeof(buf), 0), 0U); // no CR yet: nothing, and no wait
  CHECK_EQ(stub::state().now, 0U);
  uart.delay_ms(10);
  const std::size_t n = uart.read(buf, sizeof(buf), 0);
  REQUIRE(n == 21U);
  CHECK(std::memcmp(buf, "#MOXY 203456 17892 0\r", n) == 0);
}

TEST_CASE("ESP32 port: flush_rx drops a partial line") {
  stub::reset();
  EspUart uart;
  stub::arrive(0, "#MRAW 1 2");
  uint8_t buf[64];
  CHECK_EQ(uart.read(buf, sizeof(buf), 0), 0U);
  uart.flush_rx();
  stub::arrive(5, "#LOGO\r");
  const std::size_t n = uart.read(buf, sizeof(buf), 50);
  REQUIRE(n == 6U);
  CHECK(std::memcmp(buf, "#LOGO\r", n) == 0);
}

TEST_CASE("ESP32 port: a uart_write_bytes() error reaches the driver as SerialError") {
  stub::reset();
  stub::state().write_result = -1;
  EspUart uart;
  fdo2::Driver<EspUart> dev(uart);
  const auto m = dev.MeasureMoxy();
  CHECK(m.error == fdo2::DriverError::SerialError);
  CHECK_EQ(stub::state().now, 0U); // no reply awaited
}
