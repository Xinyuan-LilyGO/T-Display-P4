/*
 * @Description: st25r3916
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-08-09 17:12:58
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
#include "st25r3916_driver.h"

volatile bool Interrupt_Flag = false;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto XL9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9555_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

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

    ESP32P4->create_gpio_interrupt(T_MIXRF_ST25R3916_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::RISING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Interrupt_Flag = true;
                                   });

    St25r3916_Init();

    while (1)
    {
        if (Interrupt_Flag == true)
        {
            printf("Interrupt_Flag trigger\n");

            rfst25r3916.st25r3916Isr();

            Interrupt_Flag = false;
        }

        St25r3916_Loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
