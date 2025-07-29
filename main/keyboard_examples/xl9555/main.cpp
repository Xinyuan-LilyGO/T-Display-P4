/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-07-29 17:56:08
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

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto XL9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus, XL9555_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

volatile bool interrupt_flag = false;

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9555->begin();
    XL9555->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO3, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    XL9555->clear_irq_flag();

    while (1)
    {
        if (interrupt_flag == true)
        {
            printf("XL9555 interrupt_flag triggered\n");

            XL9555->clear_irq_flag();
            interrupt_flag = false;
        }

        XL9555->pin_write(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        printf("XL9555 IO3: %d\n", XL9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO3));
        vTaskDelay(pdMS_TO_TICKS(1000));
        XL9555->pin_write(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        printf("XL9555 IO3: %d\n", XL9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO3));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
