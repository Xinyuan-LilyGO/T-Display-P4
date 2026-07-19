/*
 * @Description: radiolib_cc1101_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-27 09:11:16
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

const uint8_t g_send_package[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g',
};

volatile bool g_interrupt_flag = false;

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto cc1101 = driver.chip().cc1101;
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->SetGpioMode(board::gpio::button::kEsp32p4Boot,
      cpp_bus_driver::Tool::GpioMode::kInput);

  esp32p4->InitGpioInterrupt(
      board::keyboard::gpio::t_mix_rf::cc1101::kInt,
      cpp_bus_driver::Tool::InterruptMode::kRising,
      [](void* arg) -> void { g_interrupt_flag = true; });

  int16_t status = cc1101->begin(868.0);
  if (status == RADIOLIB_ERR_NONE) {
    printf("Cc1101 init success\n");
  } else {
    printf("Cc1101 init failed (error code: %d)\n", status);
  }

  status = cc1101->setSyncWord(0xAA, 0xBB);
  if (status != RADIOLIB_ERR_NONE) {
    printf("setSyncWord failed (error code: %d)\n", status);
  }

  status = cc1101->startReceive();
  if (status != RADIOLIB_ERR_NONE) {
    printf("startReceive failed (error code: %d)\n", status);
  }

  g_interrupt_flag = false;

  while (1) {
    if (esp32p4->GpioRead(board::gpio::button::kEsp32p4Boot) == 0) {
      vTaskDelay(pdMS_TO_TICKS(300));

      printf("Cc1101 send package\n");

      status = cc1101->transmit(g_send_package, 10);
      if (status != RADIOLIB_ERR_NONE) {
        printf("transmit failed (error code: %d)\n", status);
      }

      status = cc1101->startReceive();
      if (status != RADIOLIB_ERR_NONE) {
        printf("startReceive failed (error code: %d)\n", status);
      }

      g_interrupt_flag = false;
    }

    if (g_interrupt_flag) {
      uint8_t receive_package[255] = {0};
      if (cc1101->readData(receive_package, 9) == RADIOLIB_ERR_NONE) {
        for (uint8_t i = 0; i < 9; i++) {
          printf("Get t_mixrf_cc1101 data[%d]: %d\n", i, receive_package[i]);
        }
      }

      status = cc1101->startReceive();
      if (status != RADIOLIB_ERR_NONE) {
        printf("startReceive failed (error code: %d)\n", status);
      }

      g_interrupt_flag = false;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
