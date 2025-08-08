/*
 * @Description: radiolib_nrf24l01_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-08-08 18:09:05
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_keyboard_config.h"
#include "cpp_bus_driver_library.h"
#include "RadioLib.h"
#include "radiolib_bridge_driver.h"

// uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

// 定义 const 数组并直接填入字符序列
const uint8_t Send_Package[] =
    {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g'};

volatile bool Interrupt_Flag = false;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto SPI_Bus_2 = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_NRF24L01_MOSI, T_MIXRF_NRF24L01_SCLK, T_MIXRF_NRF24L01_MISO, SPI2_HOST, 0);

auto XL9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9555_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

RadioLibHal *Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(SPI_Bus_2, 10000000, T_MIXRF_NRF24L01_CS);
nRF24 Nrf24l01 = new Module(Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(T_MIXRF_NRF24L01_CE), static_cast<uint32_t>(RADIOLIB_NC));

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9555->begin();
    XL9555->pin_mode(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_write(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(10));

    ESP32P4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);

    ESP32P4->pin_mode(T_MIXRF_CC1101_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    ESP32P4->pin_mode(T_MIXRF_NRF24L01_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    ESP32P4->pin_mode(T_MIXRF_ST25R3916_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    ESP32P4->pin_write(T_MIXRF_CC1101_CS, 1);
    ESP32P4->pin_write(T_MIXRF_NRF24L01_CS, 1);
    ESP32P4->pin_write(T_MIXRF_ST25R3916_CS, 1);

    ESP32P4->create_gpio_interrupt(T_MIXRF_NRF24L01_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Interrupt_Flag = true;
                                   });

    int16_t status = Nrf24l01.begin(2400.0, 1000.0, 0);
    if (status == RADIOLIB_ERR_NONE)
    {
        printf("nrf24l01 init success\n");
    }
    else
    {
        printf("nrf24l01 init fail (error code: %d)\n", status);
    }

    uint8_t addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
    status = Nrf24l01.setTransmitPipe(addr);
    if (status != RADIOLIB_ERR_NONE)
    {
        printf("nrf24l01 setTransmitPipe fail (error code: %d)\n", status);
    }

    Nrf24l01.startReceive();

    while (1)
    {
        if (ESP32P4->pin_read(ESP32P4_BOOT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));

            printf("T_MIXRF_NRF24L01 send package\n");

            Nrf24l01.finishTransmit();

            status = Nrf24l01.transmit(Send_Package, 32, 0);
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("transmit fail (error code: %d)\n", status);
            }

            status = Nrf24l01.startReceive();
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("startReceive fail (error code: %d)\n", status);
            }
        }

        if (Interrupt_Flag == true) // 接收完成中断
        {
            uint8_t receive_package[255] = {0};
            if (Nrf24l01.readData(receive_package, 9) == RADIOLIB_ERR_NONE)
            {
                for (uint8_t i = 0; i < 9; i++)
                {
                    printf("get T_MIXRF_NRF24L01 data[%d]: %d\n", i, receive_package[i]);
                }
            }

            Interrupt_Flag = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
