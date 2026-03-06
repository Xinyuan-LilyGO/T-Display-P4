/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-06 17:59:09
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

volatile bool Interrupt_Flag = false;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    Xl9535->create_gpio_interrupt(XL9535_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                  [](void *arg) -> IRAM_ATTR void
                                  {
                                      //   if ((uint32_t)arg == XL9535_INT)
                                      //   {
                                      Interrupt_Flag = true;
                                      //   }
                                  });

    Xl9535->begin();
    Xl9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO3, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    Xl9535->clear_irq_flag();

    while (1)
    {
        if (Interrupt_Flag == true)
        {
            printf("XL9535 Interrupt_Flag triggered\n");

            Xl9535->clear_irq_flag();
            Interrupt_Flag = false;
        }

        Xl9535->pin_write(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        printf("XL9535 IO3: %d\n", Xl9535->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO3));
        vTaskDelay(pdMS_TO_TICKS(1000));
        Xl9535->pin_write(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        printf("XL9535 IO3: %d\n", Xl9535->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO3));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
