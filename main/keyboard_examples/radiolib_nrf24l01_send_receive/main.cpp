/*
 * @Description: radiolib_nrf24l01_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-27 09:16:00
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

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

  auto nrf24l01 = driver.chip().nrf24l01;
  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  esp32p4->SetGpioMode(ESP32P4_BOOT, cpp_bus_driver::Tool::GpioMode::kInput);

  int16_t status = nrf24l01->begin(2400.0, 1000.0, 0);
  if (status == RADIOLIB_ERR_NONE) {
    printf("Nrf24l01 init success\n");
  } else {
    printf("Nrf24l01 init failed (error code: %d)\n", status);
  }

  esp32p4->InitGpioInterrupt(
      T_MIXRF_NRF24L01_INT, cpp_bus_driver::Tool::InterruptMode::kFalling,
      [](void* arg) -> void { g_interrupt_flag = true; });

  uint8_t addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
  status = nrf24l01->setTransmitPipe(addr);
  if (status != RADIOLIB_ERR_NONE) {
    printf("setTransmitPipe failed (error code: %d)\n", status);
  }

  status = nrf24l01->startReceive();
  if (status != RADIOLIB_ERR_NONE) {
    printf("startReceive failed (error code: %d)\n", status);
  }

  while (1) {
    if (esp32p4->GpioRead(ESP32P4_BOOT) == 0) {
      vTaskDelay(pdMS_TO_TICKS(300));

      printf("Nrf24l01 send package\n");

      status = nrf24l01->transmit(g_send_package, 32, 0);
      if (status != RADIOLIB_ERR_NONE) {
        printf("transmit failed (error code: %d)\n", status);
      }

      status = nrf24l01->startReceive();
      if (status != RADIOLIB_ERR_NONE) {
        printf("startReceive failed (error code: %d)\n", status);
      }

      g_interrupt_flag = false;
    }

    if (g_interrupt_flag) {
      uint8_t receive_package[255] = {0};
      if (nrf24l01->readData(receive_package, 9) == RADIOLIB_ERR_NONE) {
        for (uint8_t i = 0; i < 9; i++) {
          printf("Get t_mixrf_nrf24l01 data[%d]: %d\n", i, receive_package[i]);
        }
      }

      status = nrf24l01->startReceive();
      if (status != RADIOLIB_ERR_NONE) {
        printf("startReceive failed (error code: %d)\n", status);
      }

      g_interrupt_flag = false;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
