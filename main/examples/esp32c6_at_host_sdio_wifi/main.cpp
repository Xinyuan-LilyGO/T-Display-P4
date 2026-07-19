/*
 * @Description: esp32c6_at_host_sdio_wifi
 * @Author: LILYGO_L
 * @Date: 2025-06-13 10:40:19
 * @LastEditTime: 2026-04-25 16:43:05
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

size_t g_cycle_time = 0;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto esp32c6_at_sdio_bus = std::make_shared<cpp_bus_driver::HardwareSdio>(
      board::gpio::esp32c6::kSdioClk, board::gpio::esp32c6::kSdioCmd,
      board::gpio::esp32c6::kSdioD0, board::gpio::esp32c6::kSdioD1,
      board::gpio::esp32c6::kSdioD2, board::gpio::esp32c6::kSdioD3,
      -1, -1, -1, -1,
      cpp_bus_driver::HardwareSdio::SdioPort::kSlot1);

  auto esp32c6_at = std::make_unique<cpp_bus_driver::EspAt>(
      esp32c6_at_sdio_bus, [](bool value) -> void {
        // ESP32C6复位
        lilygo_device_driver::TDisplayP4Driver::GetInstance()
            .chip()
            .xl9535->GpioWrite(board::gpio::xl9535::kEsp32c6En,
                static_cast<uint8_t>(value));
      });

  esp32c6_at->Init();

  uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);

  uart_flush_input(UART_NUM_0);

  // 开启falsh保存
  if (esp32c6_at->SetFlashSave(true)) {
    printf("SetFlashSave success\n");
  } else {
    printf("SetFlashSave fail\n");
  }

  if (esp32c6_at->SetWifiMode(cpp_bus_driver::EspAt::WifiMode::kStation)) {
    printf("SetWifiMode success\n");
  } else {
    printf("SetWifiMode fail\n");
  }

  std::vector<uint8_t> buffer_wifi_scan;
  if (esp32c6_at->WifiScan(buffer_wifi_scan)) {
    printf("WifiScan: \n[%s]\n", buffer_wifi_scan.data());
  } else {
    printf("WifiScan fail\n");
  }

  std::string ssid = "LilyGo-AABB";
  std::string password = "xinyuandianzi";
  if (esp32c6_at->SetWifiConnect(ssid, password)) {
    printf(
        "SetWifiConnect success\nConnected to wifi ssid: [%s],password: "
        "[%s]\n",
        ssid.c_str(), password.c_str());
  } else {
    printf("SetWifiConnect fail\n");
  }

  cpp_bus_driver::EspAt::RealTime rt;
  if (esp32c6_at->GetRealTime(rt)) {
    printf("GetRealTime success\n");
    printf(
        "Week: [%s] day: [%d] Month: [%d] Year: [%d] Time: [%02d:%02d:%02d] "
        "Time zone: [%s] China time: [%02d:%02d:%02d]\n",
        rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute,
        rt.second, rt.time_zone.c_str(), (rt.hour + 8 + 24) % 24, rt.minute,
        rt.second);
  } else {
    printf("GetRealTime fail\n");
  }

  while (1) {
    if (esp_log_timestamp() > g_cycle_time) {
      if (esp32c6_at->GetRealTime(rt)) {
        printf("GetRealTime success\n");
        printf(
            "Week: [%s] Day: [%d] Month: [%d] Year: [%d] Time: "
            "[%02d:%02d:%02d] Time zone: [%s] China time: [%02d:%02d:%02d]\n",
            rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute,
            rt.second, rt.time_zone.c_str(), (rt.hour + 8 + 24) % 24, rt.minute,
            rt.second);
      } else {
        printf("GetRealTime fail\n");
      }

      g_cycle_time = esp_log_timestamp() + 1000;
    }

    if (!esp32c6_at->GetConnectStatus()) {
      printf("Esp32c6 at lost connection,attempting to reconnect\n");

      esp32c6_at->Reconnect();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
