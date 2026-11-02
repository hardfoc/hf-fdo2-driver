/**
 * @file mock_fdo2.hpp
 * @brief A model of the FDO2-G2 UART command interface for the host unit tests.
 *
 * The model follows data sheet v5 §4: the host sends `#HEAD` terminated by CR (or CR LF), the
 * module echoes the header with its values and a single CR, or answers `#ERRO E`. Each complete
 * command queues exactly one reply line; `read()` hands back one queued line (CR included) or
 * nothing, which is the transport contract the driver relies on. Replies can be overridden, dropped
 * or given a CRC suffix to exercise the driver's error paths.
 */
#pragma once

#include "fdo2_uart_interface.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>

class MockFdo2 : public fdo2::UartInterface<MockFdo2> {
public:
  // Values the model reports (the data sheet's own examples).
  std::string vers = "#VERS 8 1 357 15";
  std::string idnr = "#IDNR 18446744073709551615";
  std::string moxy = "#MOXY 203456 17892 0";
  std::string mraw = "#MRAW 203456 -1965 1 24385 124072 12792 999734 40365";

  std::map<std::string, std::string> override_reply; ///< header -> reply line (without CR)
  bool respond = true;                               ///< false: the module stays silent
  bool terminate = true;                             ///< false: the reply is sent without its CR
  std::string crc_suffix;                            ///< appended before CR, e.g. ": 43291" (§4.2)

  std::vector<std::string> commands; ///< every command line received, without terminator
  int flushes = 0;
  int delays_ms = 0;

  void write(const uint8_t* data, std::size_t length) noexcept {
    for (std::size_t i = 0; i < length; ++i) {
      const char c = static_cast<char>(data[i]);
      if (c == '\n') {
        continue;
      }
      if (c != '\r') {
        rx_line_ += c;
        continue;
      }
      commands.push_back(rx_line_);
      if (respond) {
        tx_lines_.push_back(replyFor(rx_line_) + crc_suffix + (terminate ? "\r" : ""));
      }
      rx_line_.clear();
    }
  }

  std::size_t read(uint8_t* out, std::size_t max, uint32_t /*timeout_ms*/) noexcept {
    if (tx_lines_.empty()) {
      return 0; // timeout: no complete line
    }
    const std::string line = tx_lines_.front();
    tx_lines_.pop_front();
    const std::size_t n = line.size() < max ? line.size() : max;
    std::memcpy(out, line.data(), n);
    return n;
  }

  void flush_rx() noexcept {
    ++flushes;
    tx_lines_.clear();
  }

  void delay_ms_impl(uint32_t ms) noexcept {
    delays_ms += static_cast<int>(ms);
  }

  /// Put a line in the receive queue without a command, e.g. a stale broadcast.
  void inject(const std::string& line) {
    tx_lines_.push_back(line);
  }

private:
  std::string replyFor(const std::string& cmd) const {
    const auto it = override_reply.find(cmd);
    if (it != override_reply.end()) {
      return it->second;
    }
    if (cmd == "#VERS")
      return vers;
    if (cmd == "#IDNR")
      return idnr;
    if (cmd == "#MOXY")
      return moxy;
    if (cmd == "#MRAW")
      return mraw;
    if (cmd == "#LOGO")
      return "#LOGO";
    return "#ERRO -26"; // header matches no supported request (Table 1)
  }

  std::string rx_line_;
  std::deque<std::string> tx_lines_;
};
