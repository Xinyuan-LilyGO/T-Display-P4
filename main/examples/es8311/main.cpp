/*
 * @Description: es8311
 * @Author: LILYGO_L
 * @Date: 2024-12-23 15:18:58
 * @LastEditTime: 2026-04-30 10:29:42
 * @License: GPL 3.0
 */
#include "New Notification 010_c2_b16_s44100.h"
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& es8311 = driver.chip().es8311;

  // 将ADC的数据自动输出到DAC上
  // es8311->SetAdcDataToDac(true);

  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->SetGpioMode(ESP32P4_BOOT, cpp_bus_driver::Tool::GpioMode::kInput,
      cpp_bus_driver::Tool::GpioStatus::kPullup);

  size_t play_count = 1;

  // 播放音乐测试
  // es8311->WriteI2s(c2_b16_s44100, sizeof(c2_b16_s44100));

  while (1) {
    // ADC和DAC相互回环测试
    // size_t data_lenght = 2048;
    // std::shared_ptr<uint16_t[]> data =
    //     std::make_shared<uint16_t[]>(data_lenght);
    // if (es8311->ReadI2s(data.get(), data_lenght * sizeof(uint16_t)) > 0) {
    //   // for (uint8_t i = 0; i < 10; i++)
    //   // {
    //   //     printf("ReadI2s: %d\n", data[i]);
    //   // }

    //   es8311->WriteI2s(data.get(), data_lenght * sizeof(uint16_t));
    // }

    if (esp32p4->Tool::GpioRead(ESP32P4_BOOT) == 0) {
      uint8_t buffer = 0;
      for (size_t i = 0; i < 256; i++) {
        driver.bus().es8311_i2c_bus->BusI2cGuide::Read(
            static_cast<uint8_t>(i), &buffer);
        printf("Es8311 register[%d]: %#X\n", i, buffer);
      }

      play_count++;
      printf("play_count: %d\n", play_count);

      // 播放音乐测试
      es8311->WriteI2s(c2_b16_s44100, sizeof(c2_b16_s44100));
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
