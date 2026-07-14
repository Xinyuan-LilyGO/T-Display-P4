/*
 * @Description: sx1262_gfsk_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:54:47
 * @LastEditTime: 2026-04-30 10:31:24
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
  driver.Init();

  auto& sx1262 = driver.chip().sx1262;
  auto& xl9535 = driver.chip().xl9535;
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->SetGpioMode(board::gpio::button::kEsp32p4Boot,
      cpp_bus_driver::Tool::GpioMode::kInput,
      cpp_bus_driver::Tool::GpioStatus::kPullup);

  Sx126x::GfskConfig gfsk_config;
  gfsk_config.frequency_mhz = 850.0;
  gfsk_config.bit_rate_kbps = 200.0;
  gfsk_config.bandwidth = Sx126x::GfskBw::kBw467000Hz;
  gfsk_config.current_limit = 140.0f;
  gfsk_config.power = 22;
  gfsk_config.frequency_deviation_khz = 10.0;
  gfsk_config.sync_word = {};
  gfsk_config.sync_word_length = 0;
  gfsk_config.pulse_shape = Sx126x::PulseShape::kGaussianBt1;
  gfsk_config.crc_type = Sx126x::GfskCrcType::kCrc2ByteInv;
  gfsk_config.crc_initial = 0x1D0F;
  gfsk_config.crc_polynomial = 0x1021;
  if (!sx1262->Configure(gfsk_config) || !sx1262->ClearBuffer() ||
      !sx1262->StartReceive()) {
    printf("Sx1262 GFSK config failed\n");
    return;
  }

  printf("Sx1262 start gfsk transmit\n");

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
          if (timeout_count > 500) {
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
        const auto packet_metrics = Sx126x::ParseGfskPacketMetrics(
            receive_status.gfsk_packet_status_raw);

        printf("Sx1262 receive rssi_average: %.01f rssi_sync: %.01f\n",
            packet_metrics.gfsk.rssi_average, packet_metrics.gfsk.rssi_sync);

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
