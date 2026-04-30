/*
 * @Description: tca8418
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-30 10:31:48
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

volatile bool g_interrupt_flag = false;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& xl9555 = driver.chip().xl9555;
  auto& tca8418 = driver.chip().tca8418;
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  xl9555->GpioWrite(XL9555_LED_1, cpp_bus_driver::Xl95x5::Value::kLow);
  xl9555->GpioWrite(XL9555_LED_2, cpp_bus_driver::Xl95x5::Value::kLow);
  xl9555->GpioWrite(XL9555_LED_3, cpp_bus_driver::Xl95x5::Value::kLow);

  esp32p4->InitGpioInterrupt(TCA8418_INT,
      cpp_bus_driver::Tool::InterruptMode::kFalling,
      [](void* arg) -> void { g_interrupt_flag = true; });

  driver.chip().tca8418_backlight->StartGradientTime(30, 1000);

  while (1) {
    if (g_interrupt_flag == true) {
      cpp_bus_driver::Tca8418::IrqStatus irq_status;

      if (tca8418->ParseIrqStatus(tca8418->GetIrqFlag(), irq_status) == false) {
        printf("ParseIrqStatus failed\n");
      } else if (irq_status.key_events_flag == true) {
        cpp_bus_driver::Tca8418::TouchPoint tp;

        if (tca8418->GetMultipleTouchPoint(tp) == true) {
          printf("Touch finger: %d\n", tp.finger_count);

          for (uint8_t i = 0; i < tp.info.size(); i++) {
            switch (tp.info[i].event_type) {
              case cpp_bus_driver::Tca8418::EventType::kKeypad: {
                cpp_bus_driver::Tca8418::TouchPosition touch_position;

                if (tca8418->ParseTouchNum(tp.info[i].num, touch_position) ==
                    true) {
                  printf("Keypad event\n");
                  printf(
                      "   Touch num:[%d] num: %d x: %d y: %d press flag: %d\n",
                      i + 1, tp.info[i].num, touch_position.x, touch_position.y,
                      tp.info[i].press_flag);

                  const size_t key_count =
                      sizeof(Tca8418_Map) / sizeof(std::string);
                  if ((tp.info[i].num > 0) && (tp.info[i].num <= key_count)) {
                    printf("   Touch string: %s\n",
                        Tca8418_Map[tp.info[i].num - 1].c_str());
                  }
                }

                break;
              }

              case cpp_bus_driver::Tca8418::EventType::kGpio:
                printf("Gpio event\n");
                printf("   Touch num:[%d] num: %d press flag: %d\n", i + 1,
                    tp.info[i].num, tp.info[i].press_flag);
                break;

              default:
                break;
            }
          }
        }

        tca8418->ClearIrqFlag(cpp_bus_driver::Tca8418::IrqFlag::kKeyEvents);
      }

      g_interrupt_flag = false;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
