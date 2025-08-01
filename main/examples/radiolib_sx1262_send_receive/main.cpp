/*
 * @Description: radiolib_sx1262_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-08-01 11:38:33
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "RadioLib.h"
#include "radiolib_bridge_driver.h"

uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto SPI_Bus_2 = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST, 0);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

RadioLibHal *Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(SPI_Bus_2, 10000000, SX1262_CS);
SX1262 Sx1262 = new Module(Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), SX1262_BUSY);

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(10));

    ESP32P4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);

    XL9535->pin_mode(XL9535_SX1262_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    // LORA复位
    XL9535->pin_mode(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    int32_t status = Sx1262.begin(920.0, 125.0, 9, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8);
    status = Sx1262.setCurrentLimit(140);
    if (status == RADIOLIB_ERR_NONE)
    {
        printf("sx1262 init success\n");
    }
    else
    {
        printf("sx1262 init fail (error code: %ld)\n", status);
    }

    Sx1262.startReceive();

    while (1)
    {
        if (ESP32P4->pin_read(ESP32P4_BOOT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));

            printf("SX1262 send package\n");

            Sx1262.transmit(Send_Package, 9);
            Sx1262.startReceive();
        }

        if (XL9535->pin_read(XL9535_SX1262_DIO1) == 1) // 接收完成中断
        {
            uint8_t receive_package[255] = {0};
            if (Sx1262.readData(receive_package, 9) == RADIOLIB_ERR_NONE)
            {
                for (uint8_t i = 0; i < 9; i++)
                {
                    printf("get SX1262 data[%d]: %d\n", i, receive_package[i]);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
