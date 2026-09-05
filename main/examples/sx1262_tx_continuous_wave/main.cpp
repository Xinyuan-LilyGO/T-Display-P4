/*
 * @Description: sx1262_tx_continuous_wave
 * @Author: LILYGO_L
 * @Date: 2025-03-28 17:41:28
 * @LastEditTime: 2026-07-15 16:00:00
 * @License: GPL 3.0
 */
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lilygo_device_driver.h"

extern "C" void app_main(void) {
  using DeviceDriver = lilygo_device_driver::TDisplayP4Driver;
  auto& driver = DeviceDriver::GetInstance();
  if (!driver.InitMinimal() || !driver.InitSx1262() ||
      !driver.SetSx1262OperatingMode(
          DeviceDriver::Sx1262OperatingMode::kStandby)) {
    printf("SX1262 initialization or wake-up failed\n");
    return;
  }

  auto& sx1262 = *driver.chip().sx1262;
  usp_cpp_bus_driver::Sx126x::LoraConfig config;
  config.frequency_hz = 868000000;
  config.output_power_dbm = 22;
  config.image_calibration_min_mhz = 863;
  config.image_calibration_max_mhz = 870;
  if (!sx1262.Configure(config) ||
      sx1262.Invoke(sx126x_set_tx_cw) != SX126X_STATUS_OK) {
    printf("SX1262 continuous wave configuration failed\n");
    return;
  }

  printf("SX1262 continuous wave started at 868 MHz, 22 dBm\n");
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
