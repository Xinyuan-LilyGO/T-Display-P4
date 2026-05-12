/*
 * @Description: aw86224
 * @Author: LILYGO_L
 * @Date: 2024-12-25 10:33:25
 * @LastEditTime: 2026-05-12 15:02:17
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& aw86224 = driver.chip().aw86224;

  using Aw862xx = cpp_bus_driver::Aw862xx;
  static constexpr uint8_t kGainLevels[] = {
      16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255};
  static constexpr uint8_t kLoopCount = 15;
  static constexpr uint32_t kPlayMs = 220;
  static constexpr uint32_t kStopMs = 180;

  while (1) {
    Aw862xx::RamWaveformSelection selection;
    while (!aw86224->InitRamModeByF0(selection)) {
      printf("Aw86224 auto RAM waveform selection failed\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    const auto& info = selection.info;
    printf(
        "Aw86224 auto selected library: %s, sequences: %u, rated f0: %uHz, "
        "detected f0: %u.%uHz\n",
        info.name, static_cast<unsigned int>(info.waveform_count),
        static_cast<unsigned int>(info.rated_f0_hz),
        static_cast<unsigned int>(selection.detected_f0_0p1_hz / 10),
        static_cast<unsigned int>(selection.detected_f0_0p1_hz % 10));
    printf("Aw86224 gain test levels:");
    for (uint8_t gain : kGainLevels) {
      printf(" %u", static_cast<unsigned int>(gain));
    }
    printf("\n");

    printf("Aw86224 GetInputVoltage: %.06f v\n", aw86224->GetInputVoltage());

    for (uint8_t gain : kGainLevels) {
      printf("Aw86224 gain level: %u\n", static_cast<unsigned int>(gain));

      for (uint8_t sequence = 1; sequence <= info.waveform_count; sequence++) {
        printf("Play %s sequence %u gain %u\n", info.name,
            static_cast<unsigned int>(sequence),
            static_cast<unsigned int>(gain));
        aw86224->PlayRamWaveform(sequence, kLoopCount, gain);
        vTaskDelay(pdMS_TO_TICKS(kPlayMs));
        aw86224->StopRamPlaybackWaveform();
        vTaskDelay(pdMS_TO_TICKS(kStopMs));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}
