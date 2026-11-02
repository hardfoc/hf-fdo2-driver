/**
 * @file test_driver.cpp
 * @brief Driver behaviour over the UART model: commands on the wire, decoding, errors, split
 * exchange.
 */
#include "mock_fdo2.hpp"
#include "test_harness.hpp"

#include "fdo2.hpp"

#include <cmath>
#include <cstring>

using Driver = fdo2::Driver<MockFdo2>;

namespace {
bool near(double a, double b) {
  return std::fabs(a - b) < 1e-9;
}
} // namespace

TEST_CASE("ReadVersion sends #VERS CR and decodes D N R S") {
  MockFdo2 uart;
  Driver dev(uart);
  const auto v = dev.ReadVersion();
  REQUIRE(v.ok());
  REQUIRE(uart.commands.size() == 1U);
  CHECK(uart.commands[0] == "#VERS");
  CHECK_EQ(v.value.device_id, 8);
  CHECK_EQ(v.value.num_channels, 1);
  CHECK_EQ(v.value.firmware_revision, 357);
  CHECK_EQ(v.value.sensor_types, 15);
  CHECK(near(fdo2::FirmwareRevisionToDouble(v.value.firmware_revision), 3.57));
}

TEST_CASE("Every transaction flushes stale input before it writes") {
  MockFdo2 uart;
  Driver dev(uart);
  uart.inject("#MRAW 1 2 0 3 4 5 6 7\r"); // a stale line, e.g. from broadcast mode
  const auto m = dev.MeasureMoxy();
  REQUIRE(m.ok());
  CHECK_EQ(uart.flushes, 1);
  CHECK_EQ(m.value.o_raw, 203456);
}

TEST_CASE("ReadUniqueId accepts the full unsigned 64-bit range") {
  MockFdo2 uart;
  Driver dev(uart);
  const auto id = dev.ReadUniqueId();
  REQUIRE(id.ok());
  CHECK(id.value == 18446744073709551615ULL);
}

TEST_CASE("MeasureMoxy scales O to hPa and T to degrees C") {
  MockFdo2 uart;
  Driver dev(uart);
  const auto m = dev.MeasureMoxy();
  REQUIRE(m.ok());
  CHECK(uart.commands[0] == "#MOXY");
  CHECK(near(m.value.p_o2_hpa, 203.456));
  CHECK(near(m.value.temp_c, 17.892));
  CHECK_EQ(m.value.status_u32, 0U);
  CHECK(fdo2::MoxyStatusOkForOxygen(m.value.status_u32));
}

TEST_CASE("MeasureMraw decodes all eight fields") {
  MockFdo2 uart;
  Driver dev(uart);
  const auto r = dev.MeasureMraw();
  REQUIRE(r.ok());
  CHECK(uart.commands[0] == "#MRAW");
  CHECK(near(r.value.temp_c, -1.965));
  CHECK_EQ(r.value.status_u32, 1U); // amplification reduced: still valid
  CHECK(fdo2::MoxyStatusOkForOxygen(r.value.status_u32));
  CHECK(near(r.value.dphi_deg, 24.385));
  CHECK(near(r.value.signal_intensity_mv, 124.072));
  CHECK(near(r.value.ambient_light_mv, 12.792));
  CHECK(near(r.value.pressure_mbar, 999.734));
  CHECK(near(r.value.rh_in_housing_pct, 40.365));
}

TEST_CASE("Fatal status bits make the reading invalid for oxygen") {
  MockFdo2 uart;
  uart.moxy = "#MOXY 203456 17892 2"; // bit 1: signal intensity too low
  Driver dev(uart);
  const auto m = dev.MeasureMoxy();
  REQUIRE(m.ok()); // the transaction itself succeeded
  CHECK(!fdo2::MoxyStatusOkForOxygen(m.value.status_u32));
  CHECK((m.value.status_u32 & fdo2::moxy_status::kFatalSignalTooLow) != 0U);
}

TEST_CASE("#ERRO replies become DeviceError with the code in LastDeviceErrorCode") {
  MockFdo2 uart;
  uart.override_reply["#MOXY"] = "#ERRO -22";
  Driver dev(uart);
  const auto m = dev.MeasureMoxy();
  CHECK(!m.ok());
  CHECK(m.error == fdo2::DriverError::DeviceError);
  CHECK_EQ(dev.LastDeviceErrorCode(), -22);
  REQUIRE(dev.ReadVersion().ok()); // the next call clears it
  CHECK_EQ(dev.LastDeviceErrorCode(), 0);
}

TEST_CASE("A silent module times out") {
  MockFdo2 uart;
  uart.respond = false;
  Driver dev(uart);
  const auto v = dev.ReadVersion();
  CHECK(v.error == fdo2::DriverError::Timeout);
}

TEST_CASE("A reply with the wrong header or field count is a ProtocolError") {
  MockFdo2 uart;
  uart.moxy = "#MOXY 203456 17892"; // S missing
  uart.override_reply["#VERS"] = "#IDNR 42";
  Driver dev(uart);
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::ProtocolError);
  CHECK(dev.ReadVersion().error == fdo2::DriverError::ProtocolError);
}

// Data sheet §4.2: with #CRCE 1 every reply ends in ": C" before the CR, C being the CRC-16/MODBUS
// (CRC-16-IBM) of every byte before the ':' as a decimal string. The data sheet gives no CRC
// examples, so the expected values below are the CRC-16/MODBUS of the data sheet's own example
// replies, computed independently (reflected polynomial 0xA001, initial value 0xFFFF).
TEST_CASE("Crc16Modbus matches the CRC-16/MODBUS check value and the example replies") {
  CHECK_EQ(fdo2::detail::Crc16Modbus("123456789", 9), 0x4B37U);
  const auto crc = [](const char* s) { return fdo2::detail::Crc16Modbus(s, std::strlen(s)); };
  CHECK_EQ(crc("#VERS 8 1 357 15"), 17525U);
  CHECK_EQ(crc("#MOXY 203456 17892 0"), 43291U);
  CHECK_EQ(crc("#MRAW 203456 -1965 1 24385 124072 12792 999734 40365"), 49266U);
  CHECK_EQ(crc("#ERRO -26"), 51302U);
}

TEST_CASE("CRC: a reply ending in ': C' with the right checksum is verified and parsed") {
  MockFdo2 uart;
  uart.crc_suffix = ": 43291"; // CRC of "#MOXY 203456 17892 0"
  Driver dev(uart);
  const auto m = dev.MeasureMoxy();
  REQUIRE(m.ok());
  CHECK_EQ(m.value.o_raw, 203456);
  CHECK_EQ(m.value.t_raw, 17892);
  CHECK_EQ(m.value.status_u32, 0U);

  uart.crc_suffix = ": 49266"; // CRC of the #MRAW example
  const auto r = dev.MeasureMraw();
  REQUIRE(r.ok());
  CHECK(near(r.value.rh_in_housing_pct, 40.365));

  uart.crc_suffix = ": 17525"; // CRC of "#VERS 8 1 357 15"
  REQUIRE(dev.ReadVersion().ok());
}

TEST_CASE("CRC: a wrong checksum is CrcMismatch, a malformed suffix a ProtocolError") {
  MockFdo2 uart;
  Driver dev(uart);
  uart.crc_suffix = ": 43292"; // one off
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::CrcMismatch);
  uart.moxy = "#MOXY 203457 17892 0"; // one corrupted digit, original checksum
  uart.crc_suffix = ": 43291";
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::CrcMismatch);
  uart.moxy = "#MOXY 203456 17892 0";
  uart.crc_suffix = ": 65536"; // not a 16-bit value
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::ProtocolError);
  uart.crc_suffix = ": 4329x";
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::ProtocolError);
  uart.crc_suffix = ":";
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::ProtocolError);
}

TEST_CASE("CRC: #ERRO replies carry the checksum too") {
  MockFdo2 uart;
  uart.override_reply["#MOXY"] = "#ERRO -26";
  uart.crc_suffix = ": 51302";
  Driver dev(uart);
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::DeviceError);
  CHECK_EQ(dev.LastDeviceErrorCode(), -26);
}

TEST_CASE("CRC: SetCrcRequired rejects a reply without the suffix") {
  MockFdo2 uart;
  Driver dev(uart);
  CHECK(!dev.IsCrcRequired());
  REQUIRE(dev.MeasureMoxy().ok()); // no suffix, not required
  dev.SetCrcRequired(true);
  CHECK(dev.IsCrcRequired());
  CHECK(dev.MeasureMoxy().error == fdo2::DriverError::CrcMismatch);
  uart.crc_suffix = ": 43291";
  CHECK(dev.MeasureMoxy().ok());
}

TEST_CASE("CRC: the split exchange verifies the checksum") {
  MockFdo2 uart;
  uart.respond = false;
  Driver dev(uart);
  REQUIRE(dev.SendMeasure(Driver::Measure::Moxy) == fdo2::DriverError::None);
  char line[96];
  uart.inject("#MOXY 203456 17892 0: 43291\r");
  REQUIRE(dev.TryReadReplyLine(line, sizeof(line)) == fdo2::DriverError::None);
  CHECK(std::strcmp(line, "#MOXY 203456 17892 0") == 0);
  uart.inject("#MOXY 203456 17892 0: 1\r");
  CHECK(dev.TryReadReplyLine(line, sizeof(line)) == fdo2::DriverError::CrcMismatch);
}

TEST_CASE("FlashLogo expects the #LOGO echo") {
  MockFdo2 uart;
  Driver dev(uart);
  CHECK(dev.FlashLogo().ok());
  CHECK(uart.commands[0] == "#LOGO");
}

TEST_CASE("Split exchange: SendMeasure, TryReadReplyLine, ParseMoxyLine") {
  MockFdo2 uart;
  uart.respond = false; // nothing back yet
  Driver dev(uart);
  CHECK(dev.SendMeasure(Driver::Measure::Moxy) == fdo2::DriverError::None);
  CHECK(uart.commands[0] == "#MOXY");
  char line[96];
  CHECK(dev.TryReadReplyLine(line, sizeof(line)) == fdo2::DriverError::Timeout);
  uart.inject("#MOXY 100000 25000 0\r"); // the reply arrives later
  REQUIRE(dev.TryReadReplyLine(line, sizeof(line)) == fdo2::DriverError::None);
  CHECK(std::strcmp(line, "#MOXY 100000 25000 0") == 0);
  int32_t device_error = 0;
  const auto m = Driver::ParseMoxyLine(line, &device_error);
  REQUIRE(m.ok());
  CHECK(near(m.value.p_o2_hpa, 100.0));
  CHECK(near(m.value.temp_c, 25.0));
}

TEST_CASE("VolumePercentO2 follows the data sheet formula and rejects bad pressure") {
  CHECK(near(fdo2::VolumePercentO2(212.73, 1013.0), 21.0));
  CHECK(near(fdo2::VolumePercentO2(200.0, 0.0), 0.0));
  CHECK(near(fdo2::VolumePercentO2(200.0, -5.0), 0.0));
}

TEST_CASE("Default timeouts and the setters") {
  MockFdo2 uart;
  Driver dev(uart);
  CHECK_EQ(dev.GetLineTimeoutMs(), 400U);
  CHECK_EQ(dev.GetMeasureTimeoutMs(), 250U);
  dev.SetMeasureTimeoutMs(400);
  dev.SetLineTimeoutMs(600);
  CHECK_EQ(dev.GetMeasureTimeoutMs(), 400U);
  CHECK_EQ(dev.GetLineTimeoutMs(), 600U);
}
