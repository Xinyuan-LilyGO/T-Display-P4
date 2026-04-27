/*
 * @Description: iic_scan
 * @Author: LILYGO_L
 * @Date: 2025-06-13 12:06:14
 * @LastEditTime: 2026-04-25 09:58:15
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

void Iic_Scan(void) {
  std::vector<uint8_t> address;
  if (lilygo_device_driver::TDisplayP4Driver::GetInstance()
          .bus()
          .xl9535_i2c_bus->Scan7bitAddress(&address)) {
    for (size_t i = 0; i < address.size(); i++) {
      printf("discovered iic devices[%u]: %#X\n", i, address[i]);
    }
  }
}

extern "C" void app_main(void) {
  printf("Ciallo\n");

  lilygo_device_driver::TDisplayP4Driver::GetInstance().Init();

  while (1) {
    Iic_Scan();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
