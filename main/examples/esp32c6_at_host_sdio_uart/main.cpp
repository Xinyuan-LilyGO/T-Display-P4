/*
 * @Description: esp32c6_at_host_sdio_uart
 * @Author: LILYGO_L
 * @Date: 2025-03-14 17:45:03
 * @LastEditTime: 2026-04-25 16:42:39
 * @License: GPL 3.0
 */
#include <array>
#include <cstdio>
#include <string>

#include "esp_at_transport.h"
#include "driver/uart.h"
#include "esp_check.h"

extern "C" void app_main(void) {
  auto esp32c6_at = esp_at_example::CreateTransport();
  if (esp32c6_at == nullptr) {
    return;
  }

  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048, 0, 0, nullptr, 0));
  ESP_ERROR_CHECK(uart_flush_input(UART_NUM_0));

  std::array<char, 1024> uart_buffer = {};
  std::string received;
  bool connected = true;
  while (true) {
    if (!connected || !esp32c6_at->IsConnected()) {
      connected = esp_at_example::Reconnect(*esp32c6_at);
      if (!connected) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }

    const int bytes_read = uart_read_bytes(UART_NUM_0, uart_buffer.data(),
        uart_buffer.size(), pdMS_TO_TICKS(20));
    if (bytes_read > 0 &&
        !esp32c6_at->SendPacket(uart_buffer.data(), bytes_read)) {
      printf("ESP-AT send failed\n");
      connected = false;
      continue;
    }

    if (!esp_at_example::ReceiveAvailable(*esp32c6_at, received)) {
      printf("ESP-AT receive failed\n");
      connected = false;
      continue;
    }
    if (!received.empty()) {
      fwrite(received.data(), 1, received.size(), stdout);
      fflush(stdout);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
