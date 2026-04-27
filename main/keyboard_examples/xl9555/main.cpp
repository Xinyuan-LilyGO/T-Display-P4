/*
 * @Description: xl9555
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-27 00:00:00
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto xl9555 = driver.chip().xl9555.get();

  xl9555->SetPinMode(cpp_bus_driver::Xl95x5::Pin::kIo7,
                     cpp_bus_driver::Xl95x5::Mode::kInput);
  xl9555->ClearIrqFlag();

  while (1) {
    xl9555->PinWrite(XL9555_LED_1, cpp_bus_driver::Xl95x5::Value::kHigh);
    xl9555->PinWrite(XL9555_LED_2, cpp_bus_driver::Xl95x5::Value::kHigh);
    xl9555->PinWrite(XL9555_LED_3, cpp_bus_driver::Xl95x5::Value::kHigh);
    printf("Xl9555 io7: %d\n",
           xl9555->PinRead(cpp_bus_driver::Xl95x5::Pin::kIo7));
    vTaskDelay(pdMS_TO_TICKS(1000));

    xl9555->PinWrite(XL9555_LED_1, cpp_bus_driver::Xl95x5::Value::kLow);
    xl9555->PinWrite(XL9555_LED_2, cpp_bus_driver::Xl95x5::Value::kLow);
    xl9555->PinWrite(XL9555_LED_3, cpp_bus_driver::Xl95x5::Value::kLow);
    printf("Xl9555 io7: %d\n",
           xl9555->PinRead(cpp_bus_driver::Xl95x5::Pin::kIo7));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
