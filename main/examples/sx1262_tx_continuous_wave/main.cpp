/*
 * @Description: sx1262_tx_continuous_wave
 * @Author: LILYGO_L
 * @Date: 2025-03-28 17:41:28
 * @LastEditTime: 2026-07-15 16:00:00
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  using Sx126x = cpp_bus_driver::Sx126x;

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

  Sx126x::LoraConfig lora_config;
  lora_config.frequency_mhz = 868.0;
  lora_config.bandwidth = Sx126x::LoraBw::kBw125000Hz;
  lora_config.current_limit = 140.0f;
  lora_config.power = 22;
  if (!sx1262->Configure(lora_config) || !sx1262->SetTxContinuousWave()) {
    printf("Sx1262 continuous wave config failed\n");
    return;
  }

  Sx126x::DeviceId device_id;
  if (sx1262->GetDeviceId(device_id)) {
    printf("Sx1262 id: %.*s\n", static_cast<int>(device_id.bytes.size()),
        reinterpret_cast<const char*>(device_id.bytes.data()));
  }
  printf("Sx1262 start send continuous wave\n");

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
