/*
 * @Description: st25r3916
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-23 15:56:38
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "st25r3916_driver.h"

auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

size_t Cycle_Time = 0;

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

    St25r3916_Init();

    while (1)
    {
        St25r3916_Loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
