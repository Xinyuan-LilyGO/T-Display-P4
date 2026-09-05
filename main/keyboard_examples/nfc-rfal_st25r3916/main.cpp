/*
 * @Description: nfc-rfal_st25r3916
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-07-15
 * @License: GPL 3.0
 */
#include "lilygo_device_driver.h"
#include "st25r3916_driver.h"

extern "C" void app_main(void) {
  printf("ST25R3916 NFC/NDEF example\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!driver.InitMinimal()) {
    printf("Base device initialization failed\n");
    return;
  }
  if (!driver.InitKeyboardExpansion()) {
    printf("Some keyboard expansion devices failed to initialize\n");
  }
  if (!driver.IsXl9555Ready() || !driver.IsSt25r3916Ready()) {
    printf("Keyboard expansion NFC reader is unavailable\n");
    return;
  }

  // The Arduino RFAL demo takes over the NFC device and its interrupt.
  if (!driver.DeinitSt25r3916()) {
    printf("Unable to release the board NFC driver\n");
    return;
  }
  if (!St25r3916_Init(driver.bus().st25r3916_spi_bus)) {
    return;
  }

  while (true) {
    St25r3916_Loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
