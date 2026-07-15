/*
 * @Description: sx1262_lora_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:57:12
 * @LastEditTime: 2026-07-15 16:00:00
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  using Sx126x = cpp_bus_driver::Sx126x;
  uint8_t receive_package[255] = {0};
  const uint8_t send_package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  size_t cycle_time = 0;

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.CreateDrivers();

  if (!driver.InitXl9535() || !driver.InitPower() ||
      !driver.ConfigXl9535()) {
    printf("Board radio power init failed\n");
    return;
  }

  auto sx1262 = std::make_unique<Sx126x>(driver.bus().sx1262_spi_bus,
      Sx126x::ChipType::kSx1262, board::gpio::sx1262::kBusy,
      board::gpio::sx1262::kCs);
  if (!sx1262->Init(10000000)) {
    printf("Sx1262 init failed\n");
    return;
  }

  auto& xl9535 = driver.chip().xl9535;
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->SetGpioMode(board::gpio::button::kEsp32p4Boot,
      cpp_bus_driver::Tool::GpioMode::kInput,
      cpp_bus_driver::Tool::GpioStatus::kPullup);

  Sx126x::LoraConfig lora_config;
  lora_config.frequency_mhz = 920.0;
  lora_config.bandwidth = Sx126x::LoraBw::kBw125000Hz;
  lora_config.current_limit = 140.0f;
  lora_config.power = 22;
  lora_config.spreading_factor = Sx126x::Sf::kSf9;
  lora_config.coding_rate = Sx126x::Cr::kCr47;
  lora_config.crc_type = Sx126x::LoraCrcType::kOn;
  if (!sx1262->Configure(lora_config) || !sx1262->ClearBuffer() ||
      !sx1262->StartReceive()) {
    printf("Sx1262 LoRa config failed\n");
    return;
  }

  printf("Sx1262 start lora transmit\n");

  while (1) {
    if (esp_log_timestamp() > cycle_time) {
      Sx126x::DeviceId device_id;
      if (sx1262->GetDeviceId(device_id)) {
        printf("Sx1262 id: %.*s\n", static_cast<int>(device_id.bytes.size()),
            reinterpret_cast<const char*>(device_id.bytes.data()));
      }

      float current_limit = 0.0f;
      if (sx1262->GetCurrentLimit(current_limit)) {
        printf("Sx1262 get current limit: %.01f mA\n", current_limit);
      }

      Sx126x::PacketType packet_type = Sx126x::PacketType::kFalse;
      sx1262->GetPacketType(packet_type);
      switch (packet_type) {
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

      uint8_t status = 0;
      sx1262->GetStatus(status);
      switch (sx1262->ParseChipModeStatus(status)) {
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

    if (esp32p4->GpioRead(board::gpio::button::kEsp32p4Boot) == 0) {
      printf("Sx1262 send start\n");
      uint16_t timeout_count = 0;
      if (sx1262->StartTransmit(send_package, sizeof(send_package),
              Sx126x::kTimeoutDisabled, Sx126x::FallbackMode::kFs)) {
        while (1) {
          if (xl9535->GpioRead(board::gpio::xl9535::kSx1262Dio1) == 1) {
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
            sx1262->ClearIrqFlag(Sx126x::IrqMaskFlag::kAll);
            break;
          }
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      } else {
        printf("Sx1262 send failed\n");
      }

      sx1262->StartReceive();
    }

    if (xl9535->GpioRead(board::gpio::xl9535::kSx1262Dio1) == 1) {
      Sx126x::ReceiveStatus receive_status;
      std::memset(receive_package, 0, sizeof(receive_package));
      size_t length_buffer = 0;
      if (!sx1262->ReadReceivedPacket(receive_package,
              sizeof(receive_package), length_buffer, &receive_status)) {
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

        for (size_t i = 0; i < length_buffer; i++) {
          printf("Get sx1262 data[%zu]: %u\n", i,
              static_cast<unsigned int>(receive_package[i]));
        }
      }
      sx1262->StartReceive();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
