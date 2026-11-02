/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_uart.hpp"

// --8<-- [start:quickstart]
#include "fdo2.hpp"
#include <cstdio>

int read_oxygen(MyUart& uart) {
  fdo2::Driver<MyUart> sensor(uart);
  uart.delay_ms(fdo2::kFdo2G2PowerUpSettleMs);          // 1. the module needs about 1 s after power-up

  const auto version = sensor.ReadVersion();            // 2. identify: D = 8 for an FDO2-G2
  if (!version || version.value.device_id != 8) {
    return -1;                                          // wiring, baud rate or supply
  }

  const auto m = sensor.MeasureMoxy();                  // 3. one measurement, up to 150 ms
  if (!m || !fdo2::MoxyStatusOkForOxygen(m.value.status_u32)) {
    return -2;                                          // timeout, #ERRO or a fault bit in S
  }
  std::printf("pO2 %.3f hPa at %.3f C\n", m.value.p_o2_hpa, m.value.temp_c);  // 4. use it
  return 0;
}
// --8<-- [end:quickstart]
