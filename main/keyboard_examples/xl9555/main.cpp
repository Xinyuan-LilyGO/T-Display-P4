/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-23 16:04:57
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

volatile bool Interrupt_Flag = false;

auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    // Xl9555->create_gpio_interrupt(XL9535_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
    //                               [](void *arg) -> IRAM_ATTR void
    //                               {
    //                                   //   if ((uint32_t)arg == XL9535_INT)
    //                                   //   {
    //                                   Interrupt_Flag = true;
    //                                   //   }
    //                               });

    Xl9555->begin();
    Xl9555->pin_mode(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO7, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    Xl9555->clear_irq_flag();

    while (1)
    {
        // if (interrupt_flag == true)
        // {
        //     printf("XL9555 Interrupt_Flag triggered\n");

        //     Xl9555->clear_irq_flag();
        //     Interrupt_Flag = false;
        // }

        Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        printf("XL9555 IO3: %d\n", Xl9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO7));
        vTaskDelay(pdMS_TO_TICKS(1000));
        Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        printf("XL9555 IO3: %d\n", Xl9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO7));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
