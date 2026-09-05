#pragma once

#include <cstdio>
#include <memory>
#include <string>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lilygo_device_driver.h"

namespace esp_at_example {

inline std::unique_ptr<cpp_bus_driver::EspAt> CreateTransport() {
  namespace board = lilygo_device_driver::t_display_p4;
  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!driver.InitMinimal() || !driver.SetEsp32c6PowerEnabled(true)) {
    printf("ESP32-C6 power initialization failed\n");
    return nullptr;
  }

  auto bus = std::make_shared<cpp_bus_driver::HardwareSdio>(
      board::gpio::esp32c6::kSdioClk, board::gpio::esp32c6::kSdioCmd,
      board::gpio::esp32c6::kSdioD0, board::gpio::esp32c6::kSdioD1,
      board::gpio::esp32c6::kSdioD2, board::gpio::esp32c6::kSdioD3,
      -1, -1, -1, -1, cpp_bus_driver::HardwareSdio::SdioPort::kSlot1);
  auto transport = std::make_unique<cpp_bus_driver::EspAt>(
      bus, [](bool enabled) {
        if (!lilygo_device_driver::TDisplayP4Driver::GetInstance()
                 .SetEsp32c6PowerEnabled(enabled)) {
          printf("ESP32-C6 reset control failed\n");
        }
      });
  if (!transport->Init()) {
    printf("ESP-AT SDIO initialization failed\n");
    transport->Deinit();
    return nullptr;
  }
  return transport;
}

inline bool Reconnect(cpp_bus_driver::EspAt& transport) {
  transport.Deinit();
  const bool connected = transport.Init();
  printf("ESP-AT reconnect %s\n", connected ? "succeeded" : "failed");
  return connected;
}

inline bool ReceiveAvailable(
    cpp_bus_driver::EspAt& transport, std::string& received) {
  received.clear();
  const uint32_t flags = transport.GetInterruptFlags();
  if (flags == UINT32_MAX ||
      (transport.HasReceivePacketInterrupt(flags) &&
          !transport.ClearInterruptFlags(flags))) {
    return false;
  }

  // Query pending data even after clearing the IRQ: a packet can grow before
  // ReceivePacket() checks the caller's capacity.
  const size_t capacity = transport.GetReceiveDataLength();
  if (capacity == 0) {
    return transport.IsConnected();
  }
  if (capacity > 0x1F800) {
    printf("ESP-AT receive length exceeds the SDIO address range\n");
    return false;
  }

  const size_t allocation_size = (capacity + 3) & ~size_t{3};
  auto* data = static_cast<uint8_t*>(heap_caps_malloc(allocation_size,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
  if (data == nullptr) {
    data = static_cast<uint8_t*>(heap_caps_malloc(allocation_size,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
  }
  if (data == nullptr) {
    printf("ESP-AT receive buffer allocation failed\n");
    return false;
  }
  std::unique_ptr<uint8_t, decltype(&heap_caps_free)> buffer(
      data, &heap_caps_free);
  size_t received_size = capacity;
  if (!transport.ReceivePacket(buffer.get(), &received_size)) {
    // Capacity failures do not consume data. Retry with the new size on the
    // next poll, without treating the required size as valid received bytes.
    return received_size > capacity;
  }
  received.assign(reinterpret_cast<const char*>(buffer.get()), received_size);
  return true;
}

}  // namespace esp_at_example
