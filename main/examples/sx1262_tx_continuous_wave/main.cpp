/*
 * @Description: sx1262_tx_continuous_wave
 * @Author: LILYGO_L
 * @Date: 2025-03-28 17:41:28
 * @LastEditTime: 2026-03-06 17:42:44
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

uint8_t Receive_Package[255] = {0};

uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

size_t Cycle_Time = 0;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto Sx1262_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

// bool SX1262_Busy_Wait_Callback(void)
// {
//     return Xl9535->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO0);
//     // return 1;
// }

auto Sx1262 = std::make_unique<Cpp_Bus_Driver::Sx126x>(Sx1262_Spi_Bus, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262, SX1262_BUSY, SX1262_CS);

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    Xl9535->pin_mode(XL9535_SX1262_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    // LORA复位
    Xl9535->pin_mode(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 默认使用RF1天线
    Xl9535->pin_mode(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    Sx1262->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);

    Sx1262->begin(10000000);
    Sx1262->config_lora_params(868.0, Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_125000HZ, 140, 22);

    Sx1262->set_tx_continuous_wave();

    printf("sx1262 start send continuous wave\n");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
