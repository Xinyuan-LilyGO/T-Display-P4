/*
 * @Description: esp32c6_at_host_sdio_wifi
 * @Author: LILYGO_L
 * @Date: 2025-06-13 10:40:19
 * @LastEditTime: 2026-04-25 16:43:05
 * @License: GPL 3.0
 */
#include <cstdio>
#include <string>

#include "esp_at_transport.h"
#include "esp_log.h"

namespace {

constexpr char kWifiSsid[] = "LilyGo-AABB";
constexpr char kWifiPassword[] = "xinyuandianzi";

std::string QuoteAtString(const char* value) {
  std::string quoted = "\"";
  for (const char* cursor = value; *cursor != '\0'; ++cursor) {
    if (*cursor == '\\' || *cursor == '"' || *cursor == ',') {
      quoted += '\\';
    }
    quoted += *cursor;
  }
  quoted += '"';
  return quoted;
}

bool SendCommand(cpp_bus_driver::EspAt& transport,
    const std::string& command, uint32_t timeout_ms = 5000) {
  if (!transport.SendPacket(command + "\r\n")) {
    printf("ESP-AT command send failed\n");
    return false;
  }

  const uint32_t started = esp_log_timestamp();
  std::string pending;
  std::string received;
  while (esp_log_timestamp() - started < timeout_ms) {
    if (!transport.IsConnected() ||
        !esp_at_example::ReceiveAvailable(transport, received)) {
      printf("ESP-AT command receive failed\n");
      return false;
    }

    pending += received;
    size_t end = 0;
    while ((end = pending.find('\n')) != std::string::npos) {
      std::string line = pending.substr(0, end);
      pending.erase(0, end + 1);
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (line.empty()) {
        continue;
      }
      if (line == "OK") {
        return true;
      }
      printf("%s\n", line.c_str());
      if (line == "ERROR" || line == "FAIL" ||
          line.rfind("+CME ERROR", 0) == 0 || line.rfind("busy ", 0) == 0) {
        return false;
      }
    }
    if (pending.size() > 4096) {
      printf("ESP-AT response line is too long\n");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  printf("ESP-AT command response timed out\n");
  return false;
}

bool ConfigureWifi(cpp_bus_driver::EspAt& transport) {
  if (!SendCommand(transport, "ATE0") ||
      !SendCommand(transport, "AT+SYSSTORE=1") ||
      !SendCommand(transport, "AT+CWMODE=1") ||
      !SendCommand(transport, "AT+CWLAP", 20000)) {
    return false;
  }
  const std::string join_command =
      "AT+CWJAP=" + QuoteAtString(kWifiSsid) + "," + QuoteAtString(kWifiPassword);
  if (!SendCommand(transport, join_command, 30000)) {
    printf("Wi-Fi connection failed\n");
    return false;
  }
  printf("Connected to Wi-Fi: %s\n", kWifiSsid);
  return SendCommand(transport,
      "AT+CIPSNTPCFG=1,8,\"pool.ntp.org\",\"time.nist.gov\"");
}

}  // namespace

extern "C" void app_main(void) {
  auto esp32c6_at = esp_at_example::CreateTransport();
  if (esp32c6_at == nullptr) {
    return;
  }

  bool configured = ConfigureWifi(*esp32c6_at);
  while (true) {
    if (!configured || !esp32c6_at->IsConnected()) {
      configured = esp_at_example::Reconnect(*esp32c6_at) &&
                   ConfigureWifi(*esp32c6_at);
      if (!configured) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }

    // ESP-AT reports SNTP time in UTC+8, configured above.
    configured = SendCommand(*esp32c6_at, "AT+CIPSNTPTIME?");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
