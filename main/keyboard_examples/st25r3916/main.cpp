/*
 * @Description: st25r3916
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-27 15:55:08
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "st25r3916_driver.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  St25r3916_Init(true);

  while (1) {
    St25r3916_Loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
