/*
 * @Description: aw86224
 * @Author: LILYGO_L
 * @Date: 2024-12-25 10:33:25
 * @LastEditTime: 2026-04-25 16:40:27
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& aw86224 = driver.chip().aw86224;

  // 等待F0校准
  while (1) {
    uint32_t f0_value = aw86224->GetF0Detection();
    printf("Aw86224 GetF0Detection: %ld\n", f0_value);

    if (aw86224->SetF0Calibrate(f0_value)) {
      break;
    }
  }

  // aw86224->SetWaveformDataSampleRate(
  //     cpp_bus_driver::Aw862xx::SampleRate::kRate12Khz);

  // RAM播放
  aw86224->InitRamMode(
      cpp_bus_driver::Aw862xx::kHapticWaveformRam12k0809_170,
      sizeof(cpp_bus_driver::Aw862xx::kHapticWaveformRam12k0809_170));

  while (1) {
    printf("Aw86224 GetInputVoltage: %.06f v\n", aw86224->GetInputVoltage());

    // RTP播放
    // cpp_bus_driver::Aw862xx::SystemStatus ss;
    // if (aw86224->GetSystemStatus(ss)) {
    //   if (!ss.rtp_fifo_full) {
    //     aw86224->RunRtpPlaybackWaveform(
    //         cpp_bus_driver::kHapticWaveformTest,
    //         sizeof(cpp_bus_driver::kHapticWaveformTest));
    //     printf("Aw86224 RunRtpPlaybackWaveform \n");

    //     vTaskDelay(pdMS_TO_TICKS(100));
    //   }
    // }
    // vTaskDelay(pdMS_TO_TICKS(10));

    // RAM播放
    aw86224->RunRamPlaybackWaveform(1, 15, 255);
    vTaskDelay(pdMS_TO_TICKS(1000));
    aw86224->StopRamPlaybackWaveform();
    vTaskDelay(pdMS_TO_TICKS(1000));

    aw86224->RunRamPlaybackWaveform(2, 14, 255);
    vTaskDelay(pdMS_TO_TICKS(30));
    aw86224->RunRamPlaybackWaveform(2, 14, 255);
    vTaskDelay(pdMS_TO_TICKS(30));

    vTaskDelay(pdMS_TO_TICKS(1000));

    aw86224->RunRamPlaybackWaveform(1, 14, 255);
    vTaskDelay(pdMS_TO_TICKS(1000));

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
