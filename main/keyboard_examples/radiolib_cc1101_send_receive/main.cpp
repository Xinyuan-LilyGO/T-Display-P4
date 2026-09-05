/*
 * @Description: radiolib_cc1101_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-27 09:11:16
 * @License: GPL 3.0
 */
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lilygo_device_driver.h"
#include "radiolib_cpp_bus_driver.h"

namespace board = lilygo_device_driver::t_display_p4;
namespace keyboard = board::keyboard_expansion;

// The 64-byte RX FIFO also holds the length byte and two status bytes.
constexpr uint8_t kMaxPacketLength = 61;

extern "C" void app_main(void) {
  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!driver.InitMinimal()) {
    printf("Board initialization failed\n");
    return;
  }
  if (!driver.InitKeyboardExpansion()) {
    printf("Some keyboard expansion peripherals could not be initialized\n");
  }
  if (!driver.IsCc1101Ready() || !driver.IsXl9555Ready() ||
      driver.bus().cc1101_spi_bus == nullptr) {
    printf("Keyboard expansion CC1101 initialization failed\n");
    return;
  }
  if (!driver.SetCc1101RfSwitch(
          lilygo_device_driver::TDisplayP4Driver::Cc1101RfSwitch::k868_915Mhz)) {
    printf("CC1101 RF switch configuration failed\n");
    return;
  }

  // Leave the radio awake and release the native SPI device before RadioLib
  // reopens it with hardware CS. The shared SPI host remains available.
  if (!driver.chip().cc1101->Wakeup() ||
      !driver.chip().cc1101->DeinitLocalResources(false)) {
    printf("CC1101 native driver release failed\n");
    return;
  }
  cpp_bus_driver::PlatformHal platform_hal;
  if (!platform_hal.SetGpioMode(board::gpio::button::kEsp32p4Boot,
          cpp_bus_driver::PlatformHal::GpioMode::kInput,
          cpp_bus_driver::PlatformHal::GpioStatus::kPullup) ||
      !platform_hal.SetGpioMode(keyboard::gpio::t_mix_rf::cc1101::kGdo2,
          cpp_bus_driver::PlatformHal::GpioMode::kInput)) {
    printf("CC1101 GPIO initialization failed\n");
    return;
  }

  RadiolibCppBusDriverHal radiolib_hal(driver.bus().cc1101_spi_bus,
      keyboard::device::cc1101::kSpiFrequencyHz,
      keyboard::gpio::t_mix_rf::cc1101::kCs);
  Module module(&radiolib_hal, static_cast<uint32_t>(RADIOLIB_NC),
      keyboard::gpio::t_mix_rf::cc1101::kGdo0,
      static_cast<uint32_t>(RADIOLIB_NC),
      keyboard::gpio::t_mix_rf::cc1101::kGdo2);
  CC1101 cc1101(&module);
  int16_t result = cc1101.begin(868.0);
  if (result != RADIOLIB_ERR_NONE) {
    printf("CC1101 initialization failed (error: %d)\n", result);
    return;
  }
  result = cc1101.setSyncWord(0xAA, 0xBB);
  if (result != RADIOLIB_ERR_NONE) {
    printf("CC1101 sync word configuration failed (error: %d)\n", result);
    return;
  }
  result = cc1101.variablePacketLengthMode(kMaxPacketLength);
  if (result != RADIOLIB_ERR_NONE) {
    printf("CC1101 packet length configuration failed (error: %d)\n", result);
    return;
  }
  result = cc1101.startReceive();
  if (result != RADIOLIB_ERR_NONE) {
    printf("CC1101 receive start failed (error: %d)\n", result);
    return;
  }

  const uint8_t send_package[] = {'a', 'b', 'c', 'd', 'e',
      'f', 'g', 'h', 'i', 'j'};
  bool button_was_pressed = false;
  while (true) {
    if (platform_hal.GpioRead(keyboard::gpio::t_mix_rf::cc1101::kGdo0) == 1) {
      uint8_t receive_package[kMaxPacketLength] = {};
      const size_t length = cc1101.getPacketLength();
      if (length > 0 && length <= sizeof(receive_package)) {
        result = cc1101.readData(receive_package, length);
        if (result == RADIOLIB_ERR_NONE) {
          printf("CC1101 received %zu bytes, RSSI: %.2f dBm\n",
              length, cc1101.getRSSI());
          for (size_t i = 0; i < length; ++i) {
            printf("CC1101 data[%zu]: %u\n", i,
                static_cast<unsigned int>(receive_package[i]));
          }
        } else {
          printf("CC1101 receive failed (error: %d)\n", result);
        }
      } else {
        printf("CC1101 invalid packet length: %zu\n", length);
      }
      result = cc1101.startReceive();
      if (result != RADIOLIB_ERR_NONE) {
        printf("CC1101 receive restart failed (error: %d)\n", result);
        return;
      }
    }

    const bool button_pressed =
        platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0;
    if (button_pressed && !button_was_pressed) {
      vTaskDelay(pdMS_TO_TICKS(30));
      if (platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0) {
        result = cc1101.transmit(send_package, sizeof(send_package));
        printf("CC1101 transmit result: %d\n", result);
        result = cc1101.startReceive();
        if (result != RADIOLIB_ERR_NONE) {
          printf("CC1101 receive restart failed (error: %d)\n", result);
          return;
        }
      }
    }
    button_was_pressed = button_pressed;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
