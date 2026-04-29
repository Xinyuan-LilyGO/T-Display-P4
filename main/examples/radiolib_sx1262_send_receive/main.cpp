/*
 * @Description: radiolib_sx1262_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-25 16:44:07
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "radiolib_cpp_bus_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.CreateDrivers();

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
  driver.InitBq25896();
  driver.bus().xl9535_i2c_bus->set_bus_handle(
      driver.bus().bq25896_i2c_bus->bus_handle());
#endif

  driver.InitXl9535();
  driver.InitPower();
  driver.ConfigXl9535();

  auto xl9535 = driver.chip().xl9535.get();

  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  auto sx1262_spi_bus = std::make_shared<cpp_bus_driver::HardwareSpi>(
      SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST, 0);

  RadioLibHal* radiolib_hal =
      new RadiolibCppBusDriverHal(sx1262_spi_bus, 10000000, SX1262_CS);
  SX1262 sx1262 = new Module(radiolib_hal, static_cast<uint32_t>(RADIOLIB_NC),
                             static_cast<uint32_t>(RADIOLIB_NC),
                             static_cast<uint32_t>(RADIOLIB_NC), SX1262_BUSY);

  const uint8_t send_package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

  esp32p4->SetGpioMode(ESP32P4_BOOT, cpp_bus_driver::Tool::GpioMode::kInput);

  esp32p4->SetGpioMode(SX1262_BUSY, cpp_bus_driver::Tool::GpioMode::kInput,
                      cpp_bus_driver::Tool::GpioStatus ::kPulldown);

  int16_t result = sx1262.begin(920.0, 125.0, 12, 7,
                                RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8);
  // int16_t result = sx1262.beginFSK(850.0, 200.0, 10, 467.0, 22, 16);
  if (result == RADIOLIB_ERR_NONE) {
    printf("Sx1262 init success\n");
  } else {
    printf("Sx1262 init failed (error code: %d)\n", result);
  }

  result = sx1262.setCurrentLimit(140);
  if (result != RADIOLIB_ERR_NONE) {
    printf("setCurrentLimit failed (error code: %d)\n", result);
  }

  sx1262.startReceive();

  while (1) {
    if (esp32p4->GpioRead(ESP32P4_BOOT) == 0) {
      vTaskDelay(pdMS_TO_TICKS(300));

      printf("SX1262 send package\n");

      result = sx1262.transmit(send_package, 9);
      if (result != RADIOLIB_ERR_NONE) {
        printf("transmit failed (error code: %d)\n", result);
      }

      result = sx1262.startReceive();
      if (result != RADIOLIB_ERR_NONE) {
        printf("startReceive failed (error code: %d)\n", result);
      }
    }

    if (xl9535->GpioRead(XL9535_SX1262_DIO1) == 1)  // 接收完成中断
    {
      uint8_t receive_package[255] = {0};
      if (sx1262.readData(receive_package, 9) == RADIOLIB_ERR_NONE) {
        printf("Sx1262 rssi: %.2f dBm, snr: %.2f dB\n", sx1262.getRSSI(),
               sx1262.getSNR());

        for (uint8_t i = 0; i < 9; i++) {
          printf("Get sx1262 data[%d]: %d\n", i, receive_package[i]);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
