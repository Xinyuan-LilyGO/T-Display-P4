/*
 * @Description: sx1262_lora_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:57:12
 * @LastEditTime: 2026-04-30 10:31:31
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  using Sx126x = cpp_bus_driver::Sx126x;
  constexpr uint16_t kRxDoneIrqMask =
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kRxDone) |
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kTimeout) |
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kCrcError) |
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kHeaderError);
  constexpr uint16_t kRxIrqMask =
      kRxDoneIrqMask |
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kPreambleDetected) |
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kHeaderValid);
  constexpr uint16_t kTxIrqMask =
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kTxDone) |
      Sx126x::IrqMask(Sx126x::IrqMaskFlag::kTimeout);

  uint8_t receive_package[255] = {0};
  uint8_t send_package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  size_t cycle_time = 0;

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& sx1262 = driver.chip().sx1262;
  auto& xl9535 = driver.chip().xl9535;
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->SetGpioMode(ESP32P4_BOOT, cpp_bus_driver::Tool::GpioMode::kInput,
      cpp_bus_driver::Tool::GpioStatus::kPullup);

  sx1262->ConfigLoraParams(
      920.0, Sx126x::LoraBw::kBw125000Hz, 140, 22, Sx126x::Sf::kSf9,
      Sx126x::Cr::kCr47, Sx126x::LoraCrcType::kOn);
  sx1262->ClearBuffer();

  sx1262->StartLora(cpp_bus_driver::Sx126x::ChipMode::kRx);
  sx1262->SetIrqGpioMode(
      kRxDoneIrqMask, 0, 0, kRxIrqMask);
  sx1262->ClearIrqFlag(kRxIrqMask);

  printf("Sx1262 start lora transmit\n");

  while (1) {
    if (esp_log_timestamp() > cycle_time) {
      printf("Sx1262 id: %s\n", sx1262->GetDeviceId().c_str());

      printf("Sx1262 get current limit: %d\n", sx1262->GetCurrentLimit());

      switch (sx1262->GetPacketType()) {
        case cpp_bus_driver::Sx126x::PacketType::kGfsk:
          printf("Sx1262 packet type: GFSK\n");
          break;
        case cpp_bus_driver::Sx126x::PacketType::kLora:
          printf("Sx1262 packet type: LORA\n");
          break;
        case cpp_bus_driver::Sx126x::PacketType::kLrFhss:
          printf("Sx1262 packet type: LR_FHSS\n");
          break;

        default:
          break;
      }

      switch (sx1262->ParseChipModeStatus(sx1262->GetStatus())) {
        case cpp_bus_driver::Sx126x::ChipModeStatus::kStbyRc:
          printf("Sx1262 chip mode status: STBY_RC\n");
          break;
        case cpp_bus_driver::Sx126x::ChipModeStatus::kStbyXosc:
          printf("Sx1262 chip mode status: STBY_XOSC\n");
          break;
        case cpp_bus_driver::Sx126x::ChipModeStatus::kFs:
          printf("Sx1262 chip mode status: FS\n");
          break;
        case cpp_bus_driver::Sx126x::ChipModeStatus::kRx:
          printf("Sx1262 chip mode status: RX\n");
          break;
        case cpp_bus_driver::Sx126x::ChipModeStatus::kTx:
          printf("Sx1262 chip mode status: TX\n");
          break;

        default:
          break;
      }

      cycle_time = esp_log_timestamp() + 1000;
    }

    if (esp32p4->GpioRead(ESP32P4_BOOT) == 0) {
      sx1262->SetRxTxFallbackMode(cpp_bus_driver::Sx126x::FallbackMode::kFs);
      sx1262->SetIrqGpioMode(
          kTxIrqMask, 0, 0, kTxIrqMask);
      sx1262->ClearIrqFlag(kTxIrqMask);

      printf("Sx1262 send start\n");
      uint16_t timeout_count = 0;
      if (sx1262->SendData(send_package, sizeof(send_package))) {
        while (1) {
          if (xl9535->GpioRead(XL9535_SX1262_DIO1) == 1) {
            Sx126x::SendStatus send_status;
            if (!sx1262->GetSendStatus(send_status)) {
              printf("Get send status failed\n");
              break;
            } else if (send_status.timeout) {
              printf("Sx1262 send irq timeout\n");
              sx1262->ClearIrqFlag(send_status.irq_flags);
              break;
            } else if (send_status.done) {
              printf("Sx1262 send success\n");
              sx1262->ClearIrqFlag(send_status.irq_flags);
              break;
            }
          }

          timeout_count++;
          if (timeout_count > 1000) {
            printf("Sx1262 send timeout\n");
            sx1262->ClearIrqFlag(kTxIrqMask);
            break;
          }
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      } else {
        printf("Sx1262 send failed\n");
      }

      sx1262->StartLora(cpp_bus_driver::Sx126x::ChipMode::kRx);
      sx1262->SetIrqGpioMode(
          kRxDoneIrqMask, 0, 0, kRxIrqMask);
      sx1262->ClearIrqFlag(kRxIrqMask);
    }

    if (xl9535->GpioRead(XL9535_SX1262_DIO1) == 1) {
      Sx126x::ReceiveStatus receive_status;
      std::memset(receive_package, 0, sizeof(receive_package));
      uint8_t length_buffer =
          sx1262->ReceiveData(receive_package, 0, &receive_status);
      if (length_buffer == 0) {
        printf("Sx1262 receive failed\n");
      } else {
        cpp_bus_driver::Sx126x::PacketMetrics packet_metrics;
        if (sx1262->GetLoraPacketMetrics(packet_metrics)) {
          printf(
              "Sx1262 receive rssi_average: %.01f "
              "rssi_instantaneous: %.01f snr: %.01f\n",
              packet_metrics.lora.rssi_average,
              packet_metrics.lora.rssi_instantaneous,
              packet_metrics.lora.snr);
        }

        for (uint8_t i = 0; i < length_buffer; i++) {
          printf("Get sx1262 data[%d]: %d\n", i, receive_package[i]);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
