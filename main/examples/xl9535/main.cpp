/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-25 16:54:24
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

volatile bool g_interrupt_flag = false;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto xl9535 = driver.chip().xl9535.get();

  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->InitGpioInterrupt(
      XL9535_INT, cpp_bus_driver::Tool::InterruptMode::kFalling,
      [](void* arg) -> void { g_interrupt_flag = true; });

  xl9535->SetPinMode(cpp_bus_driver::Xl95x5::Pin::kIo2,
                     cpp_bus_driver::Xl95x5::Mode::kOutput);
  xl9535->SetPinMode(cpp_bus_driver::Xl95x5::Pin::kIo3,
                     cpp_bus_driver::Xl95x5::Mode::kInput);

  xl9535->ClearIrqFlag();

  while (1) {
    if (g_interrupt_flag == true) {
      printf("Xl9535 g_interrupt_flag triggered\n");

      xl9535->ClearIrqFlag();
      g_interrupt_flag = false;
    }

    xl9535->PinWrite(cpp_bus_driver::Xl95x5::Pin::kIo2,
                     cpp_bus_driver::Xl95x5::Value::kHigh);
    printf("Xl9535 io3: %d\n",
           xl9535->PinRead(cpp_bus_driver::Xl95x5::Pin::kIo3));
    vTaskDelay(pdMS_TO_TICKS(1000));
    xl9535->PinWrite(cpp_bus_driver::Xl95x5::Pin::kIo2,
                     cpp_bus_driver::Xl95x5::Value::kLow);
    printf("Xl9535 io3: %d\n",
           xl9535->PinRead(cpp_bus_driver::Xl95x5::Pin::kIo3));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
