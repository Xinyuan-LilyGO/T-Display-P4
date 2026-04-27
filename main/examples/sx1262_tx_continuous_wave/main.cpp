/*
 * @Description: sx1262_tx_continuous_wave
 * @Author: LILYGO_L
 * @Date: 2025-03-28 17:41:28
 * @LastEditTime: 2026-04-25 16:54:16
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto sx1262 = driver.chip().sx1262.get();

  sx1262->ConfigLoraParams(868.0, cpp_bus_driver::Sx126x::LoraBw::kBw125000Hz,
                           140, 22);
  sx1262->SetTxContinuousWave();

  printf("Sx1262 id: %s\n", sx1262->GetDeviceId().c_str());
  printf("Sx1262 start send continuous wave\n");

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
