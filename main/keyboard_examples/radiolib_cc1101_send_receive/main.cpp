/*
 * @Description: radiolib_cc1101_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-23 15:01:11
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "radiolib_bridge_driver.h"

enum class Cc1101_Rf_Switch
{
    RF_SWITCH_315MHZ,
    RF_SWITCH_434MHZ,
    RF_SWITCH_868_915MHZ,
};

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

auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto Cc1101_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_CC1101_MOSI, T_MIXRF_CC1101_SCLK, T_MIXRF_CC1101_MISO, SPI2_HOST, 0);

auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);

RadioLibHal *Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(Cc1101_Spi_Bus, 10000000, T_MIXRF_CC1101_CS);
CC1101 Cc1101 = new Module(Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC),
                           static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), T_MIXRF_CC1101_BUSY);

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

void Cc1101_Rf_Switch_Control(Cc1101_Rf_Switch rf_switch)
{
    switch (rf_switch)
    {
    case Cc1101_Rf_Switch::RF_SWITCH_315MHZ:
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        break;
    case Cc1101_Rf_Switch::RF_SWITCH_434MHZ:
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        break;
    case Cc1101_Rf_Switch::RF_SWITCH_868_915MHZ:
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        break;

    default:
        break;
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    Xl9555->begin();
    Xl9555->pin_mode(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_write(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(10));

    Esp32p4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);

    Esp32p4->pin_mode(T_MIXRF_CC1101_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    Esp32p4->pin_mode(T_MIXRF_NRF24L01_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    Esp32p4->pin_mode(T_MIXRF_ST25R3916_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    Esp32p4->pin_write(T_MIXRF_CC1101_CS, 1);
    Esp32p4->pin_write(T_MIXRF_NRF24L01_CS, 1);
    Esp32p4->pin_write(T_MIXRF_ST25R3916_CS, 1);

    Xl9555->pin_mode(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Esp32p4->pin_mode(T_MIXRF_CC1101_BUSY, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

    Esp32p4->create_gpio_interrupt(T_MIXRF_CC1101_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::RISING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Interrupt_Flag = true;
                                   });

    Cc1101_Rf_Switch_Control(Cc1101_Rf_Switch::RF_SWITCH_868_915MHZ);

    int16_t status = Cc1101.begin(868.0);
    // int16_t status = Cc1101.beginFSK4(868.0);
    if (status == RADIOLIB_ERR_NONE)
    {
        printf("cc1101 init success\n");
    }
    else
    {
        printf("cc1101 init fail (error code: %d)\n", status);
    }

    status = Cc1101.setSyncWord(0xAA, 0xBB);
    if (status != RADIOLIB_ERR_NONE)
    {
        printf("setSyncWord fail (error code: %d)\n", status);
    }

    Cc1101.startReceive();

    Interrupt_Flag = false;

    while (1)
    {
        if (Esp32p4->pin_read(ESP32P4_BOOT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));

            printf("T_MIXRF_CC1101 send package\n");

            status = Cc1101.transmit(Send_Package, 10);
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("transmit fail (error code: %d)\n", status);
            }

            status = Cc1101.startReceive();
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("startReceive fail (error code: %d)\n", status);
            }

            Interrupt_Flag = false;
        }

        if (Interrupt_Flag == true) // 接收完成中断
        {
            uint8_t receive_package[255] = {0};
            if (Cc1101.readData(receive_package, 9) == RADIOLIB_ERR_NONE)
            {
                for (uint8_t i = 0; i < 9; i++)
                {
                    printf("get T_MIXRF_CC1101 data[%d]: %d\n", i, receive_package[i]);
                }
            }

            status = Cc1101.startReceive();
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("startReceive fail (error code: %d)\n", status);
            }

            Interrupt_Flag = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
