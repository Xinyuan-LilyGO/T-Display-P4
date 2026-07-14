/*
 * @Description: esp32c6_at_host_sdio_uart
 * @Author: LILYGO_L
 * @Date: 2025-03-14 17:45:03
 * @LastEditTime: 2026-04-25 16:42:39
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

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

  while (1) {
    size_t uart_lenght = 0;
    uart_get_buffered_data_len(UART_NUM_0, &uart_lenght);
    if (uart_lenght > 0) {
      // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
      std::unique_ptr<char[]> buffer =
          std::make_unique<char[]>(uart_lenght + 1);
      uart_read_bytes(UART_NUM_0, buffer.get(), uart_lenght, pdMS_TO_TICKS(20));
      esp32c6_at->SendPacket(buffer.get(), uart_lenght);
      printf("esp32c6_at send lenght: [%d] send: %s\n", uart_lenght,
             buffer.get());
    }

    uint32_t flag = esp32c6_at->GetIrqFlag();
    if (esp32c6_at->ParseRxNewPacketFlag(flag)) {
      // 中断后必须马上进行清除标志
      esp32c6_at->ClearIrqFlag(flag);

      // 方式1（只能读取小容量数据）
      // std::vector<uint8_t> buffer;
      // esp32c6_at->ReceivePacket(buffer);
      // // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
      // buffer.push_back('\0');
      // printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n",
      // buffer.size() - 1, buffer.data());

      // 方式2（只能读取小容量数据）
      // size_t buffer_lenght = 0;
      // std::unique_ptr<uint8_t[]> buffer;
      // esp32c6_at->ReceivePacket(buffer, &buffer_lenght);
      // // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
      // std::shared_ptr<uint8_t[]> buffer_2 =
      // std::make_unique<uint8_t[]>(buffer_lenght + 1); memcpy(buffer_2.get(),
      // buffer.get(), buffer_lenght); printf("esp32c6_at receive lenght: [%d]
      // receive: \n[%s]\n", buffer_lenght, buffer_2.get());

      // 方式3（只能读取小容量数据）
      // size_t buffer_lenght = esp32c6_at->GetRxDataLength();
      // //为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
      // std::unique_ptr<uint8_t[]> buffer =
      // std::make_unique<uint8_t[]>(buffer_lenght + 1);
      // esp32c6_at->ReceivePacket(buffer.get(), &buffer_lenght);
      // printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n",
      // buffer_lenght, buffer.get());

      // 方式4（由于espidf库限制只能使用方式4的方式读取大容量数据）
      size_t buffer_length = esp32c6_at->GetRxDataLength();
      // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
      size_t alloc_size = buffer_length + 1;
      uint8_t* buffer = (uint8_t*)heap_caps_malloc(
          alloc_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
      // 接收数据（注意：ReceivePacket接口可能需要调整以接受普通指针）
      esp32c6_at->ReceivePacket(buffer, &buffer_length);
      // 确保字符串终止
      buffer[buffer_length] = '\0';
      printf("esp32c6_at receive length: [%d] receive: \n[%s]\n", buffer_length,
             buffer);
      heap_caps_free(buffer);
    }

    if (!esp32c6_at->GetConnectStatus()) {
      printf("esp32c6_at lost connection,attempting to reconnect\n");

      esp32c6_at->Reconnect();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
