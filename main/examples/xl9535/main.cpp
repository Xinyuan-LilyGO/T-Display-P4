/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-08-01 14:12:01
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

volatile bool Interrupt_Flag = false;

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->create_gpio_interrupt(XL9535_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                  [](void *arg) -> IRAM_ATTR void
                                  {
                                      //   if ((uint32_t)arg == XL9535_INT)
                                      //   {
                                      Interrupt_Flag = true;
                                      //   }
                                  });

    XL9535->begin();
    XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO3, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    XL9535->clear_irq_flag();

    while (1)
    {
        if (interrupt_flag == true)
        {
            printf("XL9535 Interrupt_Flag triggered\n");

            XL9535->clear_irq_flag();
            Interrupt_Flag = false;
        }

        XL9535->pin_write(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        printf("XL9535 IO3: %d\n", XL9535->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO3));
        vTaskDelay(pdMS_TO_TICKS(1000));
        XL9535->pin_write(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        printf("XL9535 IO3: %d\n", XL9535->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO3));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
