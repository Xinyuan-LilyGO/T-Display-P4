/*
 * @Description: usp_sx1262_lora_send_receive
 * @Author: LILYGO_L
 * @Date: 2026-07-15 08:55:18
 * @LastEditTime: 2026-07-17 17:21:49
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace {

/**
 * @brief Convert the SX1262 chip mode to readable text.
 * @param mode SX1262 chip mode
 * @return Text describing the chip mode
 */
const char* ChipModeToString(sx126x_chip_modes_t mode) {
  switch (mode) {
    case SX126X_CHIP_MODE_UNUSED:
      return "unused";
    case SX126X_CHIP_MODE_RFU:
      return "reserved for future use";
    case SX126X_CHIP_MODE_STBY_RC:
      return "standby RC";
    case SX126X_CHIP_MODE_STBY_XOSC:
      return "standby XOSC";
    case SX126X_CHIP_MODE_FS:
      return "frequency synthesis";
    case SX126X_CHIP_MODE_RX:
      return "receive";
    case SX126X_CHIP_MODE_TX:
      return "transmit";
    default:
      return "unknown";
  }
}

/**
 * @brief Convert the SX1262 command status to readable text.
 * @param status SX1262 command status
 * @return Text describing the command status
 */
const char* CommandStatusToString(sx126x_cmd_status_t status) {
  switch (status) {
    case SX126X_CMD_STATUS_RESERVED:
      return "reserved";
    case SX126X_CMD_STATUS_RFU:
      return "reserved for future use";
    case SX126X_CMD_STATUS_DATA_AVAILABLE:
      return "data available";
    case SX126X_CMD_STATUS_CMD_TIMEOUT:
      return "command timeout";
    case SX126X_CMD_STATUS_CMD_PROCESS_ERROR:
      return "command processing error";
    case SX126X_CMD_STATUS_CMD_EXEC_FAILURE:
      return "command execution failure";
    case SX126X_CMD_STATUS_CMD_TX_DONE:
      return "transmit done";
    default:
      return "unknown";
  }
}

}  // namespace

extern "C" void app_main(void) {
  printf("Ciallo\n");

  namespace board = lilygo_device_driver::t_display_p4;

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.CreateDrivers();

  if (!driver.InitXl9535() || !driver.InitPower() || !driver.ConfigXl9535() ||
      !driver.InitSx1262()) {
    printf("Board radio power init failed\n");
    return;
  }

  auto& xl9535 = driver.chip().xl9535;
  auto& sx1262 = *driver.chip().sx1262;
  auto tool = std::make_unique<cpp_bus_driver::Tool>();

  if (!tool->SetGpioMode(board::gpio::button::kEsp32p4Boot,
          cpp_bus_driver::Tool::GpioMode::kInput,
          cpp_bus_driver::Tool::GpioStatus::kPullup)) {
    printf("Button init failed\n");
    return;
  }

  usp_cpp_bus_driver::Sx126x::LoraConfig lora_config;
  lora_config.frequency_hz = 920000000;
  lora_config.spreading_factor = SX126X_LORA_SF9;
  lora_config.bandwidth = SX126X_LORA_BW_125;
  lora_config.coding_rate = SX126X_LORA_CR_4_7;
  lora_config.output_power_dbm = 22;
  lora_config.crc_enabled = true;
  if (!sx1262.Configure(lora_config) || !sx1262.StartReceive()) {
    printf("SX1262 LoRa config failed\n");
    return;
  }

  const uint8_t send_package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  uint8_t receive_package[255] = {};
  bool transmitting = false;
  uint32_t status_print_time = 0;

  printf("SX1262 start LoRa receive\n");

  while (true) {
    const bool button_pressed =
        tool->GpioRead(board::gpio::button::kEsp32p4Boot) == 0;
    if (button_pressed && !transmitting) {
      printf("SX1262 send start\n");
      if (sx1262.StartTransmit(send_package, sizeof(send_package))) {
        transmitting = true;
      } else {
        printf("SX1262 send failed\n");
        sx1262.StartReceive();
      }
    }

    if (xl9535->GpioRead(board::gpio::xl9535::kSx1262Dio1) == 1) {
      sx126x_irq_mask_t irq_mask = SX126X_IRQ_NONE;
      if (!sx1262.GetIrqStatus(irq_mask)) {
        printf("SX1262 get IRQ status failed\n");
      } else {
        sx1262.ClearIrqStatus(irq_mask);

        if ((irq_mask & SX126X_IRQ_TX_DONE) != 0) {
          printf("SX1262 send success\n");
          transmitting = false;
          sx1262.StartReceive();
        } else if ((irq_mask & SX126X_IRQ_TIMEOUT) != 0) {
          printf("SX1262 radio timeout\n");
          transmitting = false;
          sx1262.StartReceive();
        } else if ((irq_mask &
                       (SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR)) != 0) {
          printf("SX1262 receive packet error: %#X\n", irq_mask);
          sx1262.StartReceive();
        } else if ((irq_mask & SX126X_IRQ_RX_DONE) != 0) {
          uint8_t received_size = 0;
          usp_cpp_bus_driver::Sx126x::PacketMetrics metrics;
          if (sx1262.ReadPacket(receive_package, sizeof(receive_package),
                  received_size, &metrics)) {
            printf("SX1262 receive RSSI: %d dBm SNR: %d dB\n",
                static_cast<int>(metrics.rssi_dbm),
                static_cast<int>(metrics.snr_db));
            for (uint8_t i = 0; i < received_size; ++i) {
              printf("Get SX1262 data[%u]: %u\n", static_cast<unsigned int>(i),
                  static_cast<unsigned int>(receive_package[i]));
            }
          } else {
            printf("SX1262 receive failed\n");
          }
          sx1262.StartReceive();
        }
      }
    }

    const uint32_t current_time = esp_log_timestamp();
    if (current_time >= status_print_time) {
      sx126x_chip_status_t chip_status = {};
      if (sx1262.GetChipStatus(chip_status)) {
        printf("SX1262 chip mode: %s (%d) command status: %s (%d)\n",
            ChipModeToString(chip_status.chip_mode),
            static_cast<int>(chip_status.chip_mode),
            CommandStatusToString(chip_status.cmd_status),
            static_cast<int>(chip_status.cmd_status));
      }
      status_print_time = current_time + 1000;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
