/*
 * @Description: radiolib_nrf24l01_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-27 09:16:00
 * @License: GPL 3.0
 */
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lilygo_device_driver.h"
#include "radiolib_cpp_bus_driver.h"

namespace board = lilygo_device_driver::t_display_p4;
namespace keyboard = board::keyboard_expansion;

extern "C" void app_main(void) {
  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!driver.InitMinimal()) {
    printf("Board initialization failed\n");
    return;
  }
  if (!driver.InitKeyboardExpansion()) {
    printf("Some keyboard expansion peripherals could not be initialized\n");
  }
  if (!driver.IsNrf24l01Ready() ||
      driver.bus().nrf24l01_spi_bus == nullptr) {
    printf("Keyboard expansion nRF24L01 initialization failed\n");
    return;
  }

  // Release the native SPI device before RadioLib takes ownership, retaining
  // the SPI host shared with the other keyboard expansion peripherals.
  if (!driver.chip().nrf24l01->Deinit(false)) {
    printf("nRF24L01 native driver release failed\n");
    return;
  }
  cpp_bus_driver::PlatformHal platform_hal;
  if (!platform_hal.SetGpioMode(board::gpio::button::kEsp32p4Boot,
          cpp_bus_driver::PlatformHal::GpioMode::kInput,
          cpp_bus_driver::PlatformHal::GpioStatus::kPullup)) {
    printf("BOOT button initialization failed\n");
    return;
  }

  RadiolibCppBusDriverHal radiolib_hal(driver.bus().nrf24l01_spi_bus,
      10000000, keyboard::gpio::t_mix_rf::nrf24l01::kCs);
  Module module(&radiolib_hal, static_cast<uint32_t>(RADIOLIB_NC),
      keyboard::gpio::t_mix_rf::nrf24l01::kInt,
      keyboard::gpio::t_mix_rf::nrf24l01::kCe);
  nRF24 nrf24l01(&module);
  int16_t result = nrf24l01.begin(2400, 1000, 0);
  if (result != RADIOLIB_ERR_NONE) {
    printf("nRF24L01 initialization failed (error: %d)\n", result);
    return;
  }

  const uint8_t address[] = {0x01, 0x23, 0x45, 0x67, 0x89};
  result = nrf24l01.setTransmitPipe(address);
  if (result != RADIOLIB_ERR_NONE) {
    printf("nRF24L01 transmit pipe configuration failed (error: %d)\n", result);
    return;
  }
  result = nrf24l01.setReceivePipe(0, address);
  if (result != RADIOLIB_ERR_NONE) {
    printf("nRF24L01 receive pipe configuration failed (error: %d)\n", result);
    return;
  }
  result = nrf24l01.startReceive();
  if (result != RADIOLIB_ERR_NONE) {
    printf("nRF24L01 receive start failed (error: %d)\n", result);
    return;
  }

  const uint8_t send_package[] = {
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      'A', 'B', 'C', 'D', 'E', 'F',
  };
  bool button_was_pressed = false;
  while (true) {
    if (platform_hal.GpioRead(keyboard::gpio::t_mix_rf::nrf24l01::kInt) == 0) {
      uint8_t receive_package[32] = {};
      const size_t length = nrf24l01.getPacketLength();
      if (length > 0 && length <= sizeof(receive_package)) {
        result = nrf24l01.readData(receive_package, length);
        if (result == RADIOLIB_ERR_NONE) {
          printf("nRF24L01 received %zu bytes\n", length);
          for (size_t i = 0; i < length; ++i) {
            printf("nRF24L01 data[%zu]: %u\n", i,
                static_cast<unsigned int>(receive_package[i]));
          }
        } else {
          printf("nRF24L01 receive failed (error: %d)\n", result);
        }
      } else {
        printf("nRF24L01 invalid packet length: %zu\n", length);
      }
      result = nrf24l01.startReceive();
      if (result != RADIOLIB_ERR_NONE) {
        printf("nRF24L01 receive restart failed (error: %d)\n", result);
        return;
      }
    }

    const bool button_pressed =
        platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0;
    if (button_pressed && !button_was_pressed) {
      vTaskDelay(pdMS_TO_TICKS(30));
      if (platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0) {
        result = nrf24l01.transmit(send_package, sizeof(send_package));
        printf("nRF24L01 transmit result: %d\n", result);
        result = nrf24l01.startReceive();
        if (result != RADIOLIB_ERR_NONE) {
          printf("nRF24L01 receive restart failed (error: %d)\n", result);
          return;
        }
      }
    }
    button_was_pressed = button_pressed;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
