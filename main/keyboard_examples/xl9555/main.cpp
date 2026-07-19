/*
 * @Description: xl9555
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-30 10:31:53
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& xl9555 = driver.chip().xl9555;

  xl9555->SetGpioMode(
      cpp_bus_driver::Xl95x5::Pin::kIo7, cpp_bus_driver::Xl95x5::Mode::kInput);
  xl9555->ClearIrqFlag();

  while (1) {
    xl9555->GpioWrite(board::keyboard::gpio::xl9555::kLed1, 1);
    xl9555->GpioWrite(board::keyboard::gpio::xl9555::kLed2, 1);
    xl9555->GpioWrite(board::keyboard::gpio::xl9555::kLed3, 1);
    printf("Xl9555 io7: %d\n",
        xl9555->GpioRead(cpp_bus_driver::Xl95x5::Pin::kIo7));
    vTaskDelay(pdMS_TO_TICKS(1000));

    xl9555->GpioWrite(board::keyboard::gpio::xl9555::kLed1, 0);
    xl9555->GpioWrite(board::keyboard::gpio::xl9555::kLed2, 0);
    xl9555->GpioWrite(board::keyboard::gpio::xl9555::kLed3, 0);
    printf("Xl9555 io7: %d\n",
        xl9555->GpioRead(cpp_bus_driver::Xl95x5::Pin::kIo7));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
