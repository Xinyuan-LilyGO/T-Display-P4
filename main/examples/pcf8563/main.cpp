/*
 * @Description: pcf8563
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:45:08
 * @LastEditTime: 2025-06-13 13:50:26
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "pin_config.h"
#include "cpp_bus_driver_library.h"

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(IIC_1_SDA, IIC_1_SCL, I2C_NUM_0);
auto IIC_Bus_0_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(IIC_1_SDA, IIC_1_SCL, I2C_NUM_0);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto PCF8563 = std::make_unique<Cpp_Bus_Driver::Pcf8563x>(IIC_Bus_0_1, PCF8563_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

volatile bool interrupt_flag = false;

void IIC_Scan(void)
{
    std::vector<uint8_t> address;
    if (IIC_Bus_0->scan_7bit_address(&address) == true)
    {
        for (size_t i = 0; i < address.size(); i++)
        {
            printf("Discovered IIC devices[%u]: %#X\n", i, address[i]);
        }
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    XL9535->create_gpio_interrupt(XL9535_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                  [](void *arg) IRAM_ATTR
                                  {
                                      interrupt_flag = true;
                                  });

    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    XL9535->pin_mode(XL9535_RTC_INT, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    IIC_Bus_0_1->_iic_bus_handle = IIC_Bus_0->_iic_bus_handle;
    PCF8563->begin();

    // PCF8563->set_clock_frequency_output(Cpp_Bus_Driver::PCF8563x::Clock_Frequency::OUTPUT_OFF);

    PCF8563->set_clock(false);
    PCF8563->stop_timer();
    PCF8563->stop_scheduled_alarm();

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

    PCF8563->set_time(t);
    // 定时10秒产生定时器中断
    PCF8563->run_timer(10, Cpp_Bus_Driver::Pcf8563x::Timer_Freq::CLOCK_1HZ);
    PCF8563->run_scheduled_alarm(ta);
    PCF8563->set_clock(true);

    XL9535->clear_irq_flag();

    while (1)
    {
        // IIC_Scan();
        printf("pcf8563 ID: %#X\n", PCF8563->get_device_id());

        if (PCF8563->check_clock_integrity_flag() == true) // 检查时钟完整
        {
            if (PCF8563->get_time(t) == true)
            {
                printf("pcf8563 year:[%d] month:[%d] day:[%d] time:[%d:%d:%d] week:[%d]\n", t.year, t.month, t.day,
                       t.hour, t.minute, t.second, static_cast<uint8_t>(t.week));
            }
        }
        else
        {
            printf("pcf8563 integrity of the clock information is not guaranteed\n");

            PCF8563->clear_clock_integrity_flag();
        }

        if (interrupt_flag == true)
        {
            if (XL9535->pin_read(XL9535_RTC_INT) == 0)
            {
                if (PCF8563->check_timer_flag() == true)
                {
                    printf("pcf8563 timer_flag triggered\n");
                    PCF8563->clear_timer_flag();
                }

                if (PCF8563->check_scheduled_alarm_flag() == true)
                {
                    printf("pcf8563 scheduled_alarm_flag triggered\n");
                    PCF8563->clear_scheduled_alarm_flag();
                }
            }

            XL9535->clear_irq_flag();
            interrupt_flag = false;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
