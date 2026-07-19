/*
 * @Description: pcf8563
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:45:08
 * @LastEditTime: 2026-07-13 22:31:44
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& xl9535 = driver.chip().xl9535;
  auto& pcf8563 = driver.chip().pcf8563;
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  volatile bool interrupt_flag = false;

  esp32p4->InitGpioInterrupt(
      board::gpio::xl9535::kInt,
      cpp_bus_driver::Tool::InterruptMode::kFalling,
      [](void* arg) {
        auto* flag = static_cast<volatile bool*>(arg);
        *flag = true;
      },
      (void*)&interrupt_flag  // 这里的第四个参数需要确认你的 InitGpioInterrupt
                              // 是否支持
  );

  pcf8563->SetClockFrequencyOutput(
      cpp_bus_driver::Pcf8563x::OutFreq ::kClockOff);

  pcf8563->SetClock(false);
  pcf8563->StopTimer();
  pcf8563->StopScheduledAlarm();

  cpp_bus_driver::Pcf8563x::Time t = {
      .second = 55,
      .minute = 59,
      .hour = 23,
      .day = 31,
      .week = cpp_bus_driver::Pcf8563x::Week::kSunday,
      .month = 12,
      .year = 99,
  };

  cpp_bus_driver::Pcf8563x::TimeAlarm ta = {
      .minute =
          {
              .value = 0,
              .alarm_flag = true,
          },
      .hour =
          {
              .value = 0,
              .alarm_flag = true,
          },
      .day =
          {
              .value = 1,
              .alarm_flag = true,
          },
      .week =
          {
              .value = cpp_bus_driver::Pcf8563x::Week::kSunday,
              .alarm_flag = false,
          },
  };

  pcf8563->SetTime(t);
  // 定时10秒产生定时器中断
  pcf8563->RunTimer(10, cpp_bus_driver::Pcf8563x::TimerFreq::kClock1Hz);
  pcf8563->RunScheduledAlarm(ta);
  pcf8563->SetClock(true);

  xl9535->ClearIrqFlag();

  while (1) {
    // 检查时钟完整
    if (pcf8563->CheckClockIntegrityFlag()) {
      if (pcf8563->GetTime(t)) {
        printf(
            "Pcf8563 year:[%d] month:[%d] day:[%d] time:[%d:%d:%d] week:[%d]\n",
            t.year, t.month, t.day, t.hour, t.minute, t.second,
            static_cast<uint8_t>(t.week));
      }
    } else {
      printf("Pcf8563 integrity of the clock information is not guaranteed\n");

      pcf8563->ClearClockIntegrityFlag();
    }

    if (interrupt_flag) {
      if (xl9535->GpioRead(board::gpio::xl9535::kRtcInt) == 0) {
        if (pcf8563->CheckTimerFlag()) {
          printf("Pcf8563 timer_flag triggered\n");
          pcf8563->ClearTimerFlag();
        }

        if (pcf8563->CheckScheduledAlarmFlag()) {
          printf("Pcf8563 scheduled_alarm_flag triggered\n");
          pcf8563->ClearScheduledAlarmFlag();
        }
      }

      xl9535->ClearIrqFlag();
      interrupt_flag = false;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
