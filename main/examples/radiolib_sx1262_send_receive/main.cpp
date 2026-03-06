/*
 * @Description: radiolib_sx1262_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-06 11:33:24
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "radiolib_bridge_driver.h"

uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto Sx1262_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST, 0);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

RadioLibHal *Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(Sx1262_Spi_Bus, 10000000, SX1262_CS);
SX1262 Sx1262 = new Module(Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC),
                           static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), SX1262_BUSY);

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(10));

    Esp32p4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);

    Esp32p4->pin_mode(SX1262_BUSY, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status ::PULLDOWN);
    Xl9535->pin_mode(XL9535_SX1262_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    // 默认使用RF1天线
    Xl9535->pin_mode(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    // LORA复位
    Xl9535->pin_mode(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    int16_t status = Sx1262.begin(920.0, 125.0, 12, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 8);
    // int16_t status = Sx1262.beginFSK(850.0, 200.0, 10, 467.0, 22, 16);
    if (status == RADIOLIB_ERR_NONE)
    {
        printf("sx1262 init success\n");
    }
    else
    {
        printf("sx1262 init fail (error code: %d)\n", status);
    }

    status = Sx1262.setCurrentLimit(140);
    if (status != RADIOLIB_ERR_NONE)
    {
        printf("setCurrentLimit fail (error code: %d)\n", status);
    }

    Sx1262.startReceive();

    while (1)
    {
        if (Esp32p4->pin_read(ESP32P4_BOOT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));

            printf("SX1262 send package\n");

            status = Sx1262.transmit(Send_Package, 9);
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("transmit fail (error code: %d)\n", status);
            }

            status = Sx1262.startReceive();
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("startReceive fail (error code: %d)\n", status);
            }
        }

        if (Xl9535->pin_read(XL9535_SX1262_DIO1) == 1) // 接收完成中断
        {
            uint8_t receive_package[255] = {0};
            if (Sx1262.readData(receive_package, 9) == RADIOLIB_ERR_NONE)
            {
                printf("SX1262 rssi: %.2f dBm, snr: %.2f dB\n", Sx1262.getRSSI(), Sx1262.getSNR());

                for (uint8_t i = 0; i < 9; i++)
                {
                    printf("get SX1262 data[%d]: %d\n", i, receive_package[i]);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
