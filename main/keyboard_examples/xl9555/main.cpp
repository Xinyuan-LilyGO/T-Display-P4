/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-08-01 14:11:44
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

volatile bool Interrupt_Flag = false;

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto XL9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus, XL9555_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    // XL9555->create_gpio_interrupt(XL9535_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
    //                               [](void *arg) -> IRAM_ATTR void
    //                               {
    //                                   //   if ((uint32_t)arg == XL9535_INT)
    //                                   //   {
    //                                   Interrupt_Flag = true;
    //                                   //   }
    //                               });

    XL9555->begin();
    XL9555->pin_mode(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO7, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    XL9555->clear_irq_flag();

    while (1)
    {
        // if (interrupt_flag == true)
        // {
        //     printf("XL9555 Interrupt_Flag triggered\n");

        //     XL9555->clear_irq_flag();
        //     Interrupt_Flag = false;
        // }

        XL9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        XL9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        XL9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        printf("XL9555 IO3: %d\n", XL9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO7));
        vTaskDelay(pdMS_TO_TICKS(1000));
        XL9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        XL9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        XL9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        printf("XL9555 IO3: %d\n", XL9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO7));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
