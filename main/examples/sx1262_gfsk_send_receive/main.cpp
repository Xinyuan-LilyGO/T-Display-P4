/*
 * @Description: sx1262_gfsk_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:54:47
 * @LastEditTime: 2026-04-25 16:53:58
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  uint8_t receive_package[255] = {0};
  const uint8_t send_package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  size_t cycle_time = 0;

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto sx1262 = driver.chip().sx1262.get();
  auto xl9535 = driver.chip().xl9535.get();
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->SetGpioMode(ESP32P4_BOOT, cpp_bus_driver::Tool::GpioMode::kInput,
                      cpp_bus_driver::Tool::GpioStatus::kPullup);

  sx1262->ConfigGfskParams(
      850.0, 200.0, cpp_bus_driver::Sx126x::GfskBw::kBw467000Hz, 140, 22);
  sx1262->ClearBuffer();

  sx1262->StartGfskTransmit(cpp_bus_driver::Sx126x::ChipMode::kRx);
  sx1262->SetIrqGpioMode(cpp_bus_driver::Sx126x::IrqMaskFlag::kRxDone);
  sx1262->ClearIrqFlag(cpp_bus_driver::Sx126x::IrqMaskFlag::kRxDone);

  printf("Sx1262 start gfsk transmit\n");

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
      sx1262->StartGfskTransmit(cpp_bus_driver::Sx126x::ChipMode::kTx, 0,
                                cpp_bus_driver::Sx126x::FallbackMode::kFs);
      sx1262->SetIrqGpioMode(cpp_bus_driver::Sx126x::IrqMaskFlag::kTxDone);
      sx1262->ClearIrqFlag(cpp_bus_driver::Sx126x::IrqMaskFlag::kTxDone);

      printf("Sx1262 send start\n");
      uint16_t timeout_count = 0;
      if (sx1262->SendData(send_package, sizeof(send_package))) {
        while (1) {
          if (xl9535->GpioRead(XL9535_SX1262_DIO1) == 1) {
            cpp_bus_driver::Sx126x::IrqStatus irq_status;
            if (!sx1262->ParseIrqStatus(sx1262->GetIrqFlag(), irq_status)) {
              printf("ParseIrqStatus failed\n");
            } else if (irq_status.all_flag.tx_done) {
              printf("Sx1262 send success\n");
              break;
            }
          }

          timeout_count++;
          if (timeout_count > 500) {
            printf("Sx1262 send timeout\n");
            break;
          }
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      } else {
        printf("Sx1262 send failed\n");
      }

      sx1262->StartGfskTransmit(cpp_bus_driver::Sx126x::ChipMode::kRx);
      sx1262->SetIrqGpioMode(cpp_bus_driver::Sx126x::IrqMaskFlag::kRxDone);
      sx1262->ClearIrqFlag(cpp_bus_driver::Sx126x::IrqMaskFlag::kRxDone);
    }

    if (xl9535->GpioRead(XL9535_SX1262_DIO1) == 1) {
      cpp_bus_driver::Sx126x::IrqStatus irq_status;
      if (!sx1262->ParseIrqStatus(sx1262->GetIrqFlag(), irq_status)) {
        printf("ParseIrqStatus failed\n");
      } else {
        if (irq_status.all_flag.tx_rx_timeout) {
          printf("Receive timeout\n");
          sx1262->ClearIrqFlag(cpp_bus_driver::Sx126x::IrqMaskFlag::kTimeout);
        } else if (irq_status.all_flag.crc_error) {
          printf("Receive crc error\n");
          sx1262->ClearIrqFlag(cpp_bus_driver::Sx126x::IrqMaskFlag::kCrcError);
        } else {
          cpp_bus_driver::Sx126x::GfskPacketStatus gfsk_packet_status;
          uint32_t packet_buffer = sx1262->GetGfskPacketStatus();
          if (!sx1262->ParseGfskPacketStatus(packet_buffer,
                                             gfsk_packet_status)) {
            printf("ParseGfskPacketStatus failed\n");
          } else {
            if (gfsk_packet_status.abort_error_flag) {
              printf("Receive abort_error_flag error\n");
            } else if (gfsk_packet_status.length_error_flag) {
              printf("Receive length_error_flag error\n");
            } else if (gfsk_packet_status.crc_error_flag) {
              printf("Receive crc_error_flag error\n");
            } else if (gfsk_packet_status.address_error_flag) {
              printf("Receive address_error_flag error\n");
            } else if (gfsk_packet_status.sync_word_flag) {
              printf("Receive sync_word_flag error\n");
            } else if (gfsk_packet_status.preamble_error_flag) {
              printf("Receive preamble_error_flag error\n");
            } else {
              std::memset(receive_package, 0, sizeof(receive_package));
              uint8_t length_buffer = sx1262->ReceiveData(receive_package);
              if (length_buffer == 0) {
                printf("Sx1262 receive failed\n");
              } else {
                cpp_bus_driver::Sx126x::PacketMetrics packet_metrics;
                sx1262->ParseGfskPacketMetrics(packet_buffer, packet_metrics);

                printf("Sx1262 receive rssi_average: %.01f rssi_sync: %.01f\n",
                       packet_metrics.gfsk.rssi_average,
                       packet_metrics.gfsk.rssi_sync);

                for (uint8_t i = 0; i < length_buffer; i++) {
                  printf("Get sx1262 data[%d]: %d\n", i, receive_package[i]);
                }
              }
            }
          }
        }
      }

      sx1262->ClearIrqFlag(cpp_bus_driver::Sx126x::IrqMaskFlag::kRxDone);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
