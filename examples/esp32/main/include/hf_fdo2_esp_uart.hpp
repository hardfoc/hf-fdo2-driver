/**
 * @file hf_fdo2_esp_uart.hpp
 * @brief Reusable ESP-IDF UART transport for `fdo2::Driver` (CRTP, zero virtual calls).
 */
#pragma once

#include "fdo2.hpp"

#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hf_fdo2_examples {

/**
 * @brief Template UART bridge: one concrete type per (port, TX, RX, baud) tuple.
 *
 * Call `Install()` once before constructing `fdo2::Driver<Fdo2EspIdfUart<...>>`.
 * MCU TX → sensor RX, MCU RX ← sensor TX. 8N1, no flow control (FDO2-G2 §4).
 */
template <uart_port_t kPort, int kTxGpio, int kRxGpio, std::uint32_t kBaud = fdo2::kFdo2G2DefaultBaud,
          int kRxBufBytes = 2048, int kTxBufBytes = 0>
class Fdo2EspIdfUart : public fdo2::UartInterface<Fdo2EspIdfUart<kPort, kTxGpio, kRxGpio, kBaud, kRxBufBytes, kTxBufBytes>> {
public:
    static esp_err_t Install() noexcept {
        uart_config_t uart_cfg = {};
        uart_cfg.baud_rate           = static_cast<int>(kBaud);
        uart_cfg.data_bits           = UART_DATA_8_BITS;
        uart_cfg.parity              = UART_PARITY_DISABLE;
        uart_cfg.stop_bits           = UART_STOP_BITS_1;
        uart_cfg.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
        uart_cfg.rx_flow_ctrl_thresh = 0;
        uart_cfg.source_clk          = UART_SCLK_DEFAULT;
        esp_err_t err = uart_driver_install(kPort, kRxBufBytes, kTxBufBytes, 0, nullptr, 0);
        if (err != ESP_OK) {
            return err;
        }
        err = uart_param_config(kPort, &uart_cfg);
        if (err != ESP_OK) {
            return err;
        }
        err = uart_set_pin(kPort, kTxGpio, kRxGpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        return err;
    }

    void write(const std::uint8_t* data, std::size_t len) noexcept {
        if (len > 0) {
            (void)uart_write_bytes(kPort, data, static_cast<int>(len));
        }
    }

    /**
     * @brief Return one complete reply line, CR included, as soon as its CR arrives (FDO2-G2 §4.1).
     * @details Reads byte by byte with the time left of @p timeout_ms, so a reply is handed over
     *          when it is complete instead of after the whole timeout. On timeout it returns 0 and
     *          keeps the partial line for the next call; with @p timeout_ms = 0 it only takes bytes
     *          already received. A line longer than the buffer loses its tail (and its CR), which the
     *          driver rejects.
     */
    std::size_t read(std::uint8_t* out, std::size_t max, std::uint32_t timeout_ms) noexcept {
        const TickType_t start  = xTaskGetTickCount();
        const TickType_t budget = pdMS_TO_TICKS(timeout_ms);
        for (;;) {
            const TickType_t elapsed = xTaskGetTickCount() - start;
            const TickType_t wait    = (elapsed < budget) ? (budget - elapsed) : 0;
            std::uint8_t byte        = 0;
            if (uart_read_bytes(kPort, &byte, 1U, wait) != 1) {
                return 0U;
            }
            if (len_ < sizeof(line_)) {
                line_[len_++] = byte;
            }
            if (byte == '\r') {
                const std::size_t n = (len_ < max) ? len_ : max;
                std::memcpy(out, line_, n);
                len_ = 0;
                return n;
            }
        }
    }

    void flush_rx() noexcept {
        uart_flush_input(kPort);
        len_ = 0;
    }

    void delay_ms_impl(std::uint32_t ms) noexcept { vTaskDelay(pdMS_TO_TICKS(ms)); }

private:
    std::uint8_t line_[256]{};  ///< Longest reply: #MRAW with a CRC suffix, under 120 characters.
    std::size_t  len_ = 0;
};

}  // namespace hf_fdo2_examples
