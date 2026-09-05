/*
 * @Description: sx1262_lora_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:57:12
 * @LastEditTime: 2026-07-15 16:00:00
 * @License: GPL 3.0
 */
#include <array>
#include <cstdio>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lilygo_device_driver.h"

namespace board = lilygo_device_driver::t_display_p4;

extern "C" void app_main(void) {
  using DeviceDriver = lilygo_device_driver::TDisplayP4Driver;
  auto& driver = DeviceDriver::GetInstance();
  if (!driver.InitMinimal() || !driver.InitSx1262() ||
      !driver.SetSx1262OperatingMode(
          DeviceDriver::Sx1262OperatingMode::kStandby)) {
    printf("SX1262 initialization or wake-up failed\n");
    return;
  }

  cpp_bus_driver::PlatformHal platform_hal;
  if (!platform_hal.SetGpioMode(board::gpio::button::kEsp32p4Boot,
          cpp_bus_driver::PlatformHal::GpioMode::kInput,
          cpp_bus_driver::PlatformHal::GpioStatus::kPullup)) {
    printf("BOOT button initialization failed\n");
    return;
  }

  auto& sx1262 = *driver.chip().sx1262;
  auto& xl9535 = *driver.chip().xl9535;
  usp_cpp_bus_driver::Sx126x::LoraConfig config;
  config.frequency_hz = 920000000;
  config.bandwidth = SX126X_LORA_BW_125;
  config.spreading_factor = SX126X_LORA_SF9;
  config.coding_rate = SX126X_LORA_CR_4_7;
  config.output_power_dbm = 22;
  config.crc_enabled = true;
  if (!sx1262.Configure(config) || !sx1262.StartReceive()) {
    printf("SX1262 LoRa configuration failed\n");
    return;
  }

  const std::array<uint8_t, 9> send_package = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::array<uint8_t, 255> receive_package = {};
  bool transmitting = false;
  bool button_was_pressed = false;
  uint32_t transmit_started = 0;
  uint32_t status_print_time = 0;
  printf("SX1262 LoRa receive started\n");

  while (true) {
    const bool button_pressed =
        platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0;
    if (button_pressed && !button_was_pressed && !transmitting) {
      vTaskDelay(pdMS_TO_TICKS(30));
      if (platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0) {
        if (sx1262.StartTransmit(send_package.data(), send_package.size())) {
          transmitting = true;
          transmit_started = esp_log_timestamp();
          printf("SX1262 send started\n");
        } else {
          printf("SX1262 send failed\n");
          if (!sx1262.StartReceive()) {
            return;
          }
        }
      }
    }
    button_was_pressed = button_pressed;

    if (xl9535.GpioRead(board::gpio::xl9535::kRadioDio1) == 1) {
      sx126x_irq_mask_t irq_mask = SX126X_IRQ_NONE;
      if (!sx1262.GetIrqStatus(irq_mask) || !sx1262.ClearIrqStatus(irq_mask)) {
        printf("SX1262 IRQ handling failed\n");
      } else if (irq_mask != SX126X_IRQ_NONE) {
        if ((irq_mask & SX126X_IRQ_TX_DONE) != 0) {
          printf("SX1262 send completed\n");
        } else if ((irq_mask & SX126X_IRQ_TIMEOUT) != 0) {
          printf("SX1262 radio timeout\n");
        } else if ((irq_mask &
                       (SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR)) != 0) {
          printf("SX1262 receive packet error\n");
        } else if ((irq_mask & SX126X_IRQ_RX_DONE) != 0) {
          uint8_t received_size = 0;
          usp_cpp_bus_driver::Sx126x::PacketMetrics metrics;
          if (sx1262.ReadPacket(receive_package.data(), receive_package.size(),
                  received_size, &metrics)) {
            printf("SX1262 receive RSSI: %.2f dBm, SNR: %.2f dB\n",
                static_cast<double>(metrics.rssi_quarter_dbm) / 4.0,
                static_cast<double>(metrics.snr_quarter_db) / 4.0);
            for (size_t i = 0; i < received_size; ++i) {
              printf("SX1262 data[%zu]: %u\n", i,
                  static_cast<unsigned int>(receive_package[i]));
            }
          } else {
            printf("SX1262 receive packet failed\n");
          }
        }
        transmitting = false;
        if (!sx1262.StartReceive()) {
          printf("SX1262 receive restart failed\n");
          return;
        }
      }
    }

    const uint32_t now = esp_log_timestamp();
    if (transmitting && now - transmit_started >= 2000) {
      printf("SX1262 send completion timeout\n");
      transmitting = false;
      if (!sx1262.StartReceive()) {
        return;
      }
    }
    if (now - status_print_time >= 1000) {
      sx126x_chip_status_t status = {};
      if (sx1262.GetChipStatus(status)) {
        printf("SX1262 chip mode: %d, command status: %d\n",
            static_cast<int>(status.chip_mode),
            static_cast<int>(status.cmd_status));
      }
      status_print_time = now;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
