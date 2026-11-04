/**
 * @file test_write_errors.cpp
 * @brief A command the UART did not send is a SerialError, not a Timeout, and the driver does not
 * wait for a reply to it.
 */
#include "mock_fdo2.hpp"
#include "test_harness.hpp"

#include "fdo2.hpp"

#include <type_traits>
#include <utility>

using Driver = fdo2::Driver<MockFdo2>;

// The port contract: write() returns whether every byte was accepted.
static_assert(std::is_same_v<decltype(std::declval<fdo2::UartInterface<MockFdo2>&>().write(
                                 static_cast<const uint8_t*>(nullptr), std::size_t{0})),
                             bool>,
              "UartInterface::write() must return bool");

TEST_CASE("A transaction whose command was not sent fails with SerialError without reading") {
  MockFdo2 uart;
  uart.write_fails = true;
  Driver dev(uart);
  const auto v = dev.ReadVersion();
  CHECK(v.error == fdo2::DriverError::SerialError);
  CHECK_EQ(uart.failed_writes, 1);
  CHECK(uart.read_timeouts.empty()); // no wait for a reply that cannot come
  const auto m = dev.MeasureMoxy();
  CHECK(m.error == fdo2::DriverError::SerialError);
  CHECK(uart.read_timeouts.empty());
}

TEST_CASE("SendMeasure reports a command that was not sent") {
  MockFdo2 uart;
  uart.write_fails = true;
  Driver dev(uart);
  CHECK(dev.SendMeasure(Driver::Measure::Moxy) == fdo2::DriverError::SerialError);
  CHECK(dev.SendMeasure(Driver::Measure::Version) == fdo2::DriverError::SerialError);
}

TEST_CASE("SerialError has a name") {
  CHECK(fdo2::ToString(fdo2::DriverError::SerialError) == "SerialError");
}

TEST_CASE("The link works again once write() succeeds") {
  MockFdo2 uart;
  uart.write_fails = true;
  Driver dev(uart);
  CHECK(dev.ReadVersion().error == fdo2::DriverError::SerialError);
  uart.write_fails = false;
  CHECK(dev.ReadVersion().ok());
}
