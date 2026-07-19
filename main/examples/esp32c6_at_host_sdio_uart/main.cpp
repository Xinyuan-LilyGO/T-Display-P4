/*
 * @Description: esp32c6_at_host_sdio_uart
 * @Author: LILYGO_L
 * @Date: 2025-03-14 17:45:03
 * @LastEditTime: 2026-03-26 15:00:26
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto Esp32c6_At_Sdio_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Sdio>(ESP32C6_SDIO_CLK, ESP32C6_SDIO_CMD,
                                                                           ESP32C6_SDIO_D0, ESP32C6_SDIO_D1, ESP32C6_SDIO_D2, ESP32C6_SDIO_D3, -1,
                                                                           -1, -1, -1, Cpp_Bus_Driver::Hardware_Sdio::Sdio_Port::SLOT_1);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

auto Esp32c6_At = std::make_unique<Cpp_Bus_Driver::Esp_At>(Esp32c6_At_Sdio_Bus,
                                                           [](bool value) -> IRAM_ATTR void
                                                           {
                                                               // ESP32C6复位
                                                               Xl9535->pin_write(XL9535_ESP32C6_EN, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(value));
                                                           });

extern "C" void app_main(void)
{
    printf("Ciallo\n");

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
    int16_t assert = Kode_Bq25896::bq25896_init(Bq25896_Iic_Bus, Bq25896_Handle);
    if (assert != ESP_OK)
    {
        printf("bq25896 init fail (error code: %#X)\n", assert);
    }
    else
    {
        printf("bq25896 init success\n");

        Kode_Bq25896::bq25896_set_input_current_limit(Bq25896_Handle, Kode_Bq25896::bq25896_ilim_t ::BQ25896_ILIM_2000MA);
        // 禁用看门狗后不能读取看门狗寄存器状态，否者看门狗禁用会失效
        Kode_Bq25896::bq25896_set_watchdog_timer(Bq25896_Handle, Kode_Bq25896::bq25896_watchdog_t::BQ25896_WATCHDOG_DISABLE);
        // Kode_Bq25896::bq25896_set_adc_conversion(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
        // Kode_Bq25896::bq25896_set_adc_conversion_rate(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);
        Kode_Bq25896::bq25896_set_charge_current(Bq25896_Handle, Kode_Bq25896::bq25896_ichg_t::BQ25896_ICHG_512MA);
        // Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);
    }

    Xl9535_Iic_Bus->set_bus_handle(Bq25896_Iic_Bus->get_bus_handle());
#endif

    uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);

    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    // ESP32C6复位
    Xl9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    vTaskDelay(pdMS_TO_TICKS(1000));

    Esp32c6_At->begin();

    uart_flush_input(UART_NUM_0);

    while (1)
    {
        size_t uart_lenght = 0;
        uart_get_buffered_data_len(UART_NUM_0, &uart_lenght);
        if (uart_lenght > 0)
        {
            // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(uart_lenght + 1);
            uart_read_bytes(UART_NUM_0, buffer.get(), uart_lenght, pdMS_TO_TICKS(20));
            Esp32c6_At->send_packet(buffer.get(), uart_lenght);
            printf("esp32c6_at send lenght: [%d] send: %s\n", uart_lenght, buffer.get());
        }

        uint32_t flag = Esp32c6_At->get_irq_flag();
        if (Esp32c6_At->assert_rx_new_packet_flag(flag) == true)
        {
            // 中断后必须马上进行清除标志
            Esp32c6_At->clear_irq_flag(flag);

            // 方式1（只能读取小容量数据）
            // std::vector<uint8_t> buffer;
            // Esp32c6_At->receive_packet(buffer);
            // // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
            // buffer.push_back('\0');
            // printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n", buffer.size() - 1, buffer.data());

            // 方式2（只能读取小容量数据）
            // size_t buffer_lenght = 0;
            // std::unique_ptr<uint8_t[]> buffer;
            // Esp32c6_At->receive_packet(buffer, &buffer_lenght);
            // // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
            // std::shared_ptr<uint8_t[]> buffer_2 = std::make_unique<uint8_t[]>(buffer_lenght + 1);
            // memcpy(buffer_2.get(), buffer.get(), buffer_lenght);
            // printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n", buffer_lenght, buffer_2.get());

            // 方式3（只能读取小容量数据）
            // size_t buffer_lenght = Esp32c6_At->get_rx_data_length();
            // //为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
            // std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(buffer_lenght + 1);
            // Esp32c6_At->receive_packet(buffer.get(), &buffer_lenght);
            // printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n", buffer_lenght, buffer.get());

            // 方式4（由于espidf库限制只能使用方式4的方式读取大容量数据）
            size_t buffer_length = Esp32c6_At->get_rx_data_length();
            // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
            size_t alloc_size = buffer_length + 1;
            uint8_t *buffer = (uint8_t *)heap_caps_malloc(alloc_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            // 接收数据（注意：receive_packet接口可能需要调整以接受普通指针）
            Esp32c6_At->receive_packet(buffer, &buffer_length);
            // 确保字符串终止
            buffer[buffer_length] = '\0';
            printf("esp32c6_at receive length: [%d] receive: \n[%s]\n", buffer_length, buffer);
            heap_caps_free(buffer);
        }

        if (Esp32c6_At->get_connect_status() == false)
        {
            printf("esp32c6 at lost connection,attempting to reconnect\n");

            Esp32c6_At->reconnect_esp_at();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
