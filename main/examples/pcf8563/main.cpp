/*
 * @Description: pcf8563
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:45:08
 * @LastEditTime: 2026-03-06 09:35:23
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Pcf8563_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(PCF8563_SDA, PCF8563_SCL, I2C_NUM_0);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Pcf8563 = std::make_unique<Cpp_Bus_Driver::Pcf8563x>(Pcf8563_Iic_Bus, PCF8563_IIC_ADDRESS);

volatile bool Interrupt_Flag = false;

void Iic_Scan(void)
{
    std::vector<uint8_t> address;
    if (Xl9535_Iic_Bus->scan_7bit_address(&address) == true)
    {
        for (size_t i = 0; i < address.size(); i++)
        {
            printf("discovered iic devices[%u]: %#X\n", i, address[i]);
        }
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Xl9535->create_gpio_interrupt(XL9535_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                  [](void *arg) IRAM_ATTR
                                  {
                                      Interrupt_Flag = true;
                                  });

    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    Xl9535->pin_mode(XL9535_RTC_INT, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    Pcf8563_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());
    Pcf8563->begin();

    // Pcf8563->set_clock_frequency_output(Cpp_Bus_Driver::PCF8563x::Clock_Frequency::OUTPUT_OFF);

    Pcf8563->set_clock(false);
    Pcf8563->stop_timer();
    Pcf8563->stop_scheduled_alarm();

    Cpp_Bus_Driver::Pcf8563x::Time t =
        {
            .second = 55,
            .minute = 59,
            .hour = 23,
            .day = 31,
            .week = Cpp_Bus_Driver::Pcf8563x::Week::SUNDAY,
            .month = 12,
            .year = 99,
        };

    Cpp_Bus_Driver::Pcf8563x::Time_Alarm ta =
        {
            .minute =
                {
                    .value = 0,
                    .alarm_flag = true,
                },
            .hour =
                {
                    .value = 0,
                    .alarm_flag = true,
                },
            .day =
                {
                    .value = 1,
                    .alarm_flag = true,
                },
            .week =
                {
                    .value = Cpp_Bus_Driver::Pcf8563x::Week::SUNDAY,
                    .alarm_flag = false,
                },
        };

    Pcf8563->set_time(t);
    // 定时10秒产生定时器中断
    Pcf8563->run_timer(10, Cpp_Bus_Driver::Pcf8563x::Timer_Freq::CLOCK_1HZ);
    Pcf8563->run_scheduled_alarm(ta);
    Pcf8563->set_clock(true);

    Xl9535->clear_irq_flag();

    while (1)
    {
        // Iic_Scan();
        printf("pcf8563 ID: %#X\n", Pcf8563->get_device_id());

        if (Pcf8563->check_clock_integrity_flag() == true) // 检查时钟完整
        {
            if (Pcf8563->get_time(t) == true)
            {
                printf("pcf8563 year:[%d] month:[%d] day:[%d] time:[%d:%d:%d] week:[%d]\n", t.year, t.month, t.day,
                       t.hour, t.minute, t.second, static_cast<uint8_t>(t.week));
            }
        }
        else
        {
            printf("pcf8563 integrity of the clock information is not guaranteed\n");

            Pcf8563->clear_clock_integrity_flag();
        }

        if (Interrupt_Flag == true)
        {
            if (Xl9535->pin_read(XL9535_RTC_INT) == 0)
            {
                if (Pcf8563->check_timer_flag() == true)
                {
                    printf("pcf8563 timer_flag triggered\n");
                    Pcf8563->clear_timer_flag();
                }

                if (Pcf8563->check_scheduled_alarm_flag() == true)
                {
                    printf("pcf8563 scheduled_alarm_flag triggered\n");
                    Pcf8563->clear_scheduled_alarm_flag();
                }
            }

            Xl9535->clear_irq_flag();
            Interrupt_Flag = false;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
