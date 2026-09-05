/*
 * @Description: radiolib_sx1262_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-04-30 10:31:11
 * @License: GPL 3.0
 */
#include <cstdio>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lilygo_device_driver.h"
#include "radiolib_cpp_bus_driver.h"

namespace board = lilygo_device_driver::t_display_p4;

namespace {

// RadioLib polls DIO1 during transmit and drives RESET during begin().
// These signals are connected to the XL9535, not ESP32-P4 GPIOs.
class P4RadioLibHal final : public RadiolibCppBusDriverHal {
 public:
  static constexpr uint32_t kDio1 = UINT32_MAX - 1;
  static constexpr uint32_t kReset = UINT32_MAX - 2;

  P4RadioLibHal(std::shared_ptr<cpp_bus_driver::SpiBusBase> bus,
      cpp_bus_driver::Xl95x5& xl9535)
      : RadiolibCppBusDriverHal(bus, 10000000, board::gpio::radio::kCs),
        xl9535_(xl9535) {}

  void pinMode(uint32_t pin, uint32_t mode) override {
    if (pin != kDio1 && pin != kReset) {
      RadiolibCppBusDriverHal::pinMode(pin, mode);
    }
  }

  void digitalWrite(uint32_t pin, uint32_t value) override {
    if (pin == kReset) {
      if (!xl9535_.GpioWrite(board::gpio::xl9535::kRadioRst, value != 0)) {
        printf("SX1262 reset control failed\n");
      }
    } else if (pin != kDio1) {
      RadiolibCppBusDriverHal::digitalWrite(pin, value);
    }
  }

  uint32_t digitalRead(uint32_t pin) override {
    if (pin == kDio1) {
      return xl9535_.GpioRead(board::gpio::xl9535::kRadioDio1) == 1;
    }
    if (pin == kReset) {
      return xl9535_.GpioRead(board::gpio::xl9535::kRadioRst) == 1;
    }
    return RadiolibCppBusDriverHal::digitalRead(pin);
  }

 private:
  cpp_bus_driver::Xl95x5& xl9535_;
};

}  // namespace

extern "C" void app_main(void) {
  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!driver.InitMinimal()) {
    printf("Board radio power initialization failed\n");
    return;
  }

  auto& xl9535 = *driver.chip().xl9535;
  if (!xl9535.GpioWrite(board::gpio::xl9535::kRadioRst, 0) ||
      !xl9535.SetGpioMode(board::gpio::xl9535::kRadioRst,
          cpp_bus_driver::Xl95x5::Mode::kOutput)) {
    printf("SX1262 reset initialization failed\n");
    return;
  }

  cpp_bus_driver::PlatformHal platform_hal;
  if (!platform_hal.SetGpioMode(board::gpio::button::kEsp32p4Boot,
          cpp_bus_driver::PlatformHal::GpioMode::kInput,
          cpp_bus_driver::PlatformHal::GpioStatus::kPullup)) {
    printf("BOOT button initialization failed\n");
    return;
  }

  P4RadioLibHal radiolib_hal(driver.bus().radio_spi_bus, xl9535);
  Module module(&radiolib_hal, static_cast<uint32_t>(RADIOLIB_NC),
      P4RadioLibHal::kDio1, P4RadioLibHal::kReset, board::gpio::radio::kBusy);
  SX1262 sx1262(&module);
  int16_t result = sx1262.begin(
      920.0, 125.0, 12, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8);
  if (result != RADIOLIB_ERR_NONE) {
    printf("SX1262 initialization failed (error: %d)\n", result);
    return;
  }
  result = sx1262.setCurrentLimit(140);
  if (result != RADIOLIB_ERR_NONE) {
    printf("SX1262 current limit configuration failed (error: %d)\n", result);
    return;
  }
  result = sx1262.startReceive();
  if (result != RADIOLIB_ERR_NONE) {
    printf("SX1262 receive start failed (error: %d)\n", result);
    return;
  }

  const uint8_t send_package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  bool button_was_pressed = false;
  while (true) {
    const bool button_pressed =
        platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0;
    if (button_pressed && !button_was_pressed) {
      vTaskDelay(pdMS_TO_TICKS(30));
      if (platform_hal.GpioRead(board::gpio::button::kEsp32p4Boot) == 0) {
        result = sx1262.transmit(send_package, sizeof(send_package));
        printf("SX1262 transmit result: %d\n", result);
        result = sx1262.startReceive();
        if (result != RADIOLIB_ERR_NONE) {
          printf("SX1262 receive restart failed (error: %d)\n", result);
          return;
        }
      }
    }
    button_was_pressed = button_pressed;

    if (xl9535.GpioRead(board::gpio::xl9535::kRadioDio1) == 1) {
      uint8_t receive_package[255] = {};
      const size_t length = sx1262.getPacketLength();
      if (length > 0 && length <= sizeof(receive_package) &&
          sx1262.readData(receive_package, length) == RADIOLIB_ERR_NONE) {
        printf("SX1262 RSSI: %.2f dBm, SNR: %.2f dB\n",
            sx1262.getRSSI(), sx1262.getSNR());
        for (size_t i = 0; i < length; ++i) {
          printf("SX1262 data[%zu]: %u\n", i,
              static_cast<unsigned int>(receive_package[i]));
        }
      } else {
        printf("SX1262 receive packet failed\n");
      }
      result = sx1262.startReceive();
      if (result != RADIOLIB_ERR_NONE) {
        printf("SX1262 receive restart failed (error: %d)\n", result);
        return;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
