/**
 * @file api_tasks.cpp
 * @brief The API guide's task snippets, compiled by the host build (never run).
 */
#include "my_uart.hpp"

#include <cinttypes>
#include <cstdio>

using Sensor = fdo2::Driver<MyUart>;

bool identify(Sensor& sensor) {
  // --8<-- [start:identify]
  const auto version = sensor.ReadVersion();           // #VERS D N R S
  if (!version) {
    std::printf("#VERS failed: %s\n", fdo2::ToString(version.error).data());
    return false;
  }
  std::printf("device %" PRId32 ", firmware %.2f, sensors 0x%" PRIX32 "\n", version.value.device_id,
              fdo2::FirmwareRevisionToDouble(version.value.firmware_revision),
              static_cast<uint32_t>(version.value.sensor_types));  // 15: O2, T, P, RH

  const auto id = sensor.ReadUniqueId();               // #IDNR N, unsigned 64-bit
  if (id) {
    std::printf("unique id %" PRIu64 "\n", id.value);
  }
  // --8<-- [end:identify]
  return true;
}

bool measure(Sensor& sensor) {
  // --8<-- [start:measure]
  const auto m = sensor.MeasureMoxy();                 // #MOXY O T S
  if (!m) {
    return false;                                      // Timeout, ProtocolError or DeviceError
  }
  if (!fdo2::MoxyStatusOkForOxygen(m.value.status_u32)) {
    return false;                                      // S > 1: O and T are or may be wrong
  }
  const double po2_hpa = m.value.p_o2_hpa;             // O x 10^-3
  const double temp_c = m.value.temp_c;                // T x 10^-3
  // --8<-- [end:measure]
  (void)po2_hpa;
  (void)temp_c;
  return true;
}

bool measure_raw(Sensor& sensor) {
  // --8<-- [start:raw]
  const auto r = sensor.MeasureMraw();                 // #MRAW O T S D I A P H
  if (!r || !fdo2::MoxyStatusOkForOxygen(r.value.status_u32)) {
    return false;
  }
  // Valid when the gas at the membrane is at the pressure measured at the back of the housing.
  const double percent_o2 = fdo2::VolumePercentO2(r.value.p_o2_hpa, r.value.pressure_mbar);

  // Optics check: keep signal + ambient light under about 2000 mV (shade the sensor otherwise).
  const bool too_bright = r.value.signal_intensity_mv + r.value.ambient_light_mv > 2000.0;
  // --8<-- [end:raw]
  (void)percent_o2;
  (void)too_bright;
  return true;
}

// --8<-- [start:split]
// Start a measurement, do other work, collect the reply when the UART has it.
class OxygenPoller {
public:
  explicit OxygenPoller(Sensor& sensor) : sensor_(sensor) {}

  void start() { pending_ = sensor_.SendMeasure(Sensor::Measure::Moxy) == fdo2::DriverError::None; }

  // Call from your loop; true when a new reading was stored in `out`.
  bool poll(fdo2::MoxyReading& out) {
    char line[96];
    if (!pending_ || sensor_.TryReadReplyLine(line, sizeof(line)) != fdo2::DriverError::None) {
      return false;                                    // no complete line yet; you own the timeout
    }
    pending_ = false;
    int32_t device_error = 0;
    const auto m = Sensor::ParseMoxyLine(line, &device_error);
    if (!m || !fdo2::MoxyStatusOkForOxygen(m.value.status_u32)) {
      return false;
    }
    out = m.value;
    return true;
  }

private:
  Sensor& sensor_;
  bool pending_ = false;
};
// --8<-- [end:split]

void timeouts(Sensor& sensor) {
  // --8<-- [start:timeouts]
  sensor.SetMeasureTimeoutMs(400);   // #MOXY / #MRAW: covers M = 3 or 4 (up to about 300 ms)
  sensor.SetLineTimeoutMs(400);      // #VERS, #IDNR, #LOGO
  const auto m = sensor.MeasureMoxy(1000);             // or per call, in ms
  // --8<-- [end:timeouts]
  (void)m;
}

void errors(Sensor& sensor) {
  // --8<-- [start:errors]
  const auto m = sensor.MeasureMoxy();
  switch (m.error) {
    case fdo2::DriverError::None:
      break;
    case fdo2::DriverError::Timeout:                   // no reply: wiring, baud, power, still booting
    case fdo2::DriverError::ProtocolError:             // reply did not parse: noise, wrong baud, broadcast on
      sensor.MeasureMoxy();                            // the data sheet's advice: send the command again
      break;
    case fdo2::DriverError::DeviceError:               // #ERRO E
      std::printf("#ERRO %" PRId32 "\n", sensor.LastDeviceErrorCode());  // -21, -22, -23: repeat; -26: unknown
      break;
    default:
      std::printf("%s\n", fdo2::ToString(m.error).data());
      break;
  }

  if (m && (m.value.status_u32 & fdo2::moxy_status::kWarnHumidityHigh) != 0U) {
    std::printf("humidity in the housing above 90 %%RH\n");
  }
  // --8<-- [end:errors]
}

bool blink(Sensor& sensor) {
  // --8<-- [start:logo]
  return sensor.FlashLogo().ok();    // status LED flashes 4 times, about 1 s: which sensor is on this port
  // --8<-- [end:logo]
}
