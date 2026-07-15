/*
 * @Description: STSW-ST25RFAL002 ST25R3916 NFC 发现示例
 * @Author: LILYGO_L
 * @Date: 2026-07-15
 * @License: GPL 3.0
 */
#include <cstdio>
#include <memory>

#include "lilygo_device_driver_library.h"
#include "stsw_st25rfal002_cpp_bus_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;
namespace stsw = stsw_st25rfal002_cpp_bus_driver;

namespace {

const char* DeviceTypeName(rfalNfcDevType type) {
  switch (type) {
    case RFAL_NFC_LISTEN_TYPE_NFCA:
      return "NFC-A";
    case RFAL_NFC_LISTEN_TYPE_NFCB:
      return "NFC-B";
    case RFAL_NFC_LISTEN_TYPE_NFCF:
      return "NFC-F";
    case RFAL_NFC_LISTEN_TYPE_NFCV:
      return "NFC-V";
    case RFAL_NFC_LISTEN_TYPE_ST25TB:
      return "ST25TB";
    case RFAL_NFC_LISTEN_TYPE_AP2P:
      return "NFC-DEP listener";
    case RFAL_NFC_POLL_TYPE_NFCA:
      return "NFC-A poller";
    case RFAL_NFC_POLL_TYPE_NFCB:
      return "NFC-B poller";
    case RFAL_NFC_POLL_TYPE_NFCF:
      return "NFC-F poller";
    case RFAL_NFC_POLL_TYPE_NFCV:
      return "NFC-V poller";
    case RFAL_NFC_POLL_TYPE_AP2P:
      return "NFC-DEP poller";
    default:
      return "unknown";
  }
}

void PrintDevice(const rfalNfcDevice& device) {
  std::printf("NFC device found: %s, interface: %u, NFCID:",
      DeviceTypeName(device.type),
      static_cast<unsigned int>(device.rfInterface));
  if (device.nfcid != nullptr) {
    for (uint8_t index = 0; index < device.nfcidLen; ++index) {
      std::printf(" %02X", device.nfcid[index]);
    }
  }
  std::printf("\n");
}

rfalNfcDiscoverParam CreateDiscoveryParameters() {
  rfalNfcDiscoverParam parameters = {};
  parameters.compMode = RFAL_COMPLIANCE_MODE_NFC;
  parameters.techs2Find = RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_B |
                          RFAL_NFC_POLL_TECH_F | RFAL_NFC_POLL_TECH_V |
                          RFAL_NFC_POLL_TECH_ST25TB;
  parameters.totalDuration = 1000;
  parameters.devLimit = 1;
  parameters.maxBR = RFAL_BR_848;
  parameters.nfcfBR = RFAL_BR_212;
  parameters.ap2pBR = RFAL_BR_424;
  parameters.notifyCb = nullptr;
  parameters.wakeupEnabled = false;
  parameters.wakeupConfigDefault = true;
  return parameters;
}

}  // namespace

extern "C" void app_main(void) {
  auto& board_driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!board_driver.Init()) {
    std::printf("T-Display-P4 initialization failed\n");
    return;
  }

  // 使用 SPI 前必须确认模块 I2C_EN 已稳定接地。
  // 复用开发板 SPI 总线，并为 ST25R3916 创建 Mode 1 和独立片选设备。
  auto spi_bus = std::make_shared<cpp_bus_driver::HardwareSpi>(
      board_driver.bus().sx1262_spi_bus, 1);
  stsw::St25r3916 nfc(spi_bus, board::keyboard::gpio::t_mix_rf::st25r3916::kInt,
      board::keyboard::gpio::t_mix_rf::st25r3916::kCs);

  ReturnCode result = nfc.Init();
  if (result != RFAL_ERR_NONE) {
    std::printf("ST25R3916 initialization failed: %u, platform: %u\n",
        static_cast<unsigned int>(result),
        static_cast<unsigned int>(nfc.platform_error()));
    return;
  }

  rfalNfcDiscoverParam discovery = CreateDiscoveryParameters();
  result = rfalNfcDiscover(&discovery);
  if (result != RFAL_ERR_NONE) {
    std::printf(
        "NFC discovery start failed: %u\n", static_cast<unsigned int>(result));
    return;
  }

  std::printf("ST25R3916 NFC discovery started\n");
  bool device_reported = false;
  while (true) {
    nfc.NfcWorker();
    const stsw::PlatformError platform_error = nfc.platform_error();
    if (platform_error != stsw::PlatformError::kNone) {
      std::printf("ST25R3916 platform failure: %u\n",
          static_cast<unsigned int>(platform_error));
      break;
    }

    const rfalNfcState state = rfalNfcGetState();
    if (rfalNfcIsDevActivated(state)) {
      if (!device_reported) {
        rfalNfcDevice* device = nullptr;
        result = rfalNfcGetActiveDevice(&device);
        if (result == RFAL_ERR_NONE && device != nullptr) {
          PrintDevice(*device);
        } else {
          std::printf("Failed to get active NFC device: %u\n",
              static_cast<unsigned int>(result));
        }

        device_reported = true;
        vTaskDelay(pdMS_TO_TICKS(500));
      }

      result = rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_DISCOVERY);
      if (result != RFAL_ERR_NONE) {
        std::printf("NFC discovery restart failed: %u\n",
            static_cast<unsigned int>(result));
        vTaskDelay(pdMS_TO_TICKS(100));
      }
    } else if (rfalNfcIsInDiscovery(state)) {
      device_reported = false;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
