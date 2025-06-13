/*
 * @Description: esp32c6_at_host_sdio_wifi
 * @Author: LILYGO_L
 * @Date: 2025-06-13 10:40:19
 * @LastEditTime: 2025-06-13 10:56:24
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "pin_config.h"
#include "cpp_bus_driver_library.h"

size_t Cycle_Time = 0;

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto SDIO_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Sdio>(ESP32C6_SDIO_CLK, ESP32C6_SDIO_CMD,
                                                                ESP32C6_SDIO_D0, ESP32C6_SDIO_D1, ESP32C6_SDIO_D2, ESP32C6_SDIO_D3, DEFAULT_CPP_BUS_DRIVER_VALUE,
                                                                DEFAULT_CPP_BUS_DRIVER_VALUE, DEFAULT_CPP_BUS_DRIVER_VALUE, DEFAULT_CPP_BUS_DRIVER_VALUE,
                                                                Cpp_Bus_Driver::Hardware_Sdio::Sdio_Port::SLOT_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto ESP32C6_AT = std::make_unique<Cpp_Bus_Driver::Esp_At>(SDIO_Bus,
                                                           [](bool value) -> IRAM_ATTR void
                                                           {
                                                               // ESP32C6复位
                                                               XL9535->pin_write(XL9535_ESP32C6_EN, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(value));
                                                           });

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);

    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    // ESP32C6复位
    XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP32C6_AT->begin();

    uart_flush_input(UART_NUM_0);

    // 开启falsh保存
    if (ESP32C6_AT->set_flash_save(true) == true)
    {
        printf("set_flash_save success\n");
    }
    else
    {
        printf("set_flash_save fail\n");
    }

    if (ESP32C6_AT->set_wifi_mode(Cpp_Bus_Driver::Esp_At::Wifi_Mode::STATION) == true)
    {
        printf("set_wifi_mode success\n");
    }
    else
    {
        printf("set_wifi_mode fail\n");
    }

    std::vector<uint8_t> buffer_wifi_scan;
    if (ESP32C6_AT->wifi_scan(buffer_wifi_scan) == true)
    {
        printf("wifi_scan: \n[%s]\n", buffer_wifi_scan.data());
    }
    else
    {
        printf("wifi_scan fail\n");
    }

    std::string ssid = "LilyGo-AABB";
    std::string password = "xinyuandianzi";
    if (ESP32C6_AT->set_wifi_connect(ssid, password) == true)
    {
        printf("set_wifi_connect success\nconnected to wifi ssid: [%s],password: [%s]\n", ssid.c_str(), password.c_str());
    }
    else
    {
        printf("set_wifi_connect fail\n");
    }

    Cpp_Bus_Driver::Esp_At::Real_Time rt;
    if (ESP32C6_AT->get_real_time(rt) == true)
    {
        printf("get_real_time success\n");
        printf("week: [%s] day: [%d] month: [%d] year: [%d] time: [%02d:%02d:%02d] time zone: [%s] china time: [%02d:%02d:%02d]\n",
               rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute, rt.second, rt.time_zone.c_str(),
               (rt.hour + 8 + 24) % 24, rt.minute, rt.second);
    }
    else
    {
        printf("get_real_time fail\n");
    }

    while (1)
    {
        // 串口回调
        // size_t uart_lenght = 0;
        // uart_get_buffered_data_len(UART_NUM_0, &uart_lenght);
        // if (uart_lenght > 0)
        // {
        //     // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
        //     std::unique_ptr<char[]> buffer = std::make_unique<char[]>(uart_lenght + 1);
        //     uart_read_bytes(UART_NUM_0, buffer.get(), uart_lenght, pdMS_TO_TICKS(20));
        //     ESP32C6_AT->send_packet(buffer.get(), uart_lenght);
        //     printf("esp32c6_at send lenght: [%d] send: %s\n", uart_lenght, buffer.get());
        // }

        // // if (ESP32C6_AT->wait_interrupt(100) == true)
        // // {
        // uint32_t flag = ESP32C6_AT->get_irq_flag();
        // if (ESP32C6_AT->assert_rx_new_packet_flag(flag) == true)
        // {
        //     // 中断后必须马上进行清除标志
        //     ESP32C6_AT->clear_irq_flag(flag);

        //     // 方式1
        //     std::vector<uint8_t> buffer;
        //     ESP32C6_AT->receive_packet(buffer);
        //     // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
        //     buffer.push_back('\0');
        //     printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n", buffer.size() - 1, buffer.data());

        //     // 方式2
        //     // size_t buffer_lenght = 0;
        //     // std::unique_ptr<uint8_t[]> buffer;
        //     // ESP32C6_AT->receive_packet(buffer, &buffer_lenght);
        //     // // 为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
        //     // std::shared_ptr<uint8_t[]> buffer_2 = std::make_unique<uint8_t[]>(buffer_lenght + 1);
        //     // memcpy(buffer_2.get(), buffer.get(), buffer_lenght);
        //     // printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n", buffer_lenght, buffer_2.get());

        //     // 方式3
        //     // size_t buffer_lenght = ESP32C6_AT->get_rx_data_length();
        //     // //为了去除串口调试出现的乱码，这里+1预留最后一位'\0'
        //     // std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(buffer_lenght + 1);
        //     // ESP32C6_AT->receive_packet(buffer.get(), &buffer_lenght);
        //     // printf("esp32c6_at receive lenght: [%d] receive: \n[%s]\n", buffer_lenght, buffer.get());
        // }
        // // }

        if (esp_log_timestamp() > Cycle_Time)
        {
            if (ESP32C6_AT->get_real_time(rt) == true)
            {
                printf("get_real_time success\n");
                printf("week: [%s] day: [%d] month: [%d] year: [%d] time: [%02d:%02d:%02d] time zone: [%s] china time: [%02d:%02d:%02d]\n",
                       rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute, rt.second, rt.time_zone.c_str(),
                       (rt.hour + 8 + 24) % 24, rt.minute, rt.second);
            }
            else
            {
                printf("get_real_time fail\n");
            }

            Cycle_Time = esp_log_timestamp() + 1000;
        }

        if (ESP32C6_AT->get_connect_status() == false)
        {
            printf("esp32c6 at lost connection,attempting to reconnect\n");

            ESP32C6_AT->reconnect_esp_at();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
