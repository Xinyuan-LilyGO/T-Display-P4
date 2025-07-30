/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-07-30 17:27:24
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_keyboard_config.h"
#include "t_display_p4_driver.h"
#include "cpp_bus_driver_library.h"

auto XL9555_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto TCA8418_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(TCA8418_SDA, TCA8418_SCL, I2C_NUM_0);

auto XL9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(XL9555_IIC_Bus, XL9555_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto TCA8418 = std::make_unique<Cpp_Bus_Driver::Tca8418>(TCA8418_IIC_Bus, TCA8418_IIC_ADDRESS, TCA8418_SCAN_WIDTH, TCA8418_SCAN_HEIGHT,
                                                         DEFAULT_CPP_BUS_DRIVER_VALUE);

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

volatile bool Interrupt_Flag = false;

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Init_Ldo_Channel_Power(4, 3300);

    XL9555->begin();
    XL9555->pin_mode(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9555->pin_mode(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    TCA8418->create_gpio_interrupt(TCA8418_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Interrupt_Flag = true;
                                   });

    TCA8418_IIC_Bus->set_bus_handle(XL9555_IIC_Bus->get_bus_handle());

    TCA8418->begin();

    ESP32P4->create_pwm(KEYBOARD_BL, ledc_channel_t::LEDC_CHANNEL_0, 20000);

    ESP32P4->start_pwm_gradient_time(100, 1000);

    while (1)
    {
        if (Interrupt_Flag == true)
        {
            Cpp_Bus_Driver::Tca8418::Irq_Status is;

            if (TCA8418->parse_irq_status(TCA8418->get_irq_flag(), is) == true)
            {
                printf("parse_irq_status fail\n");
            }
            else
            {
            }

            if (is.key_events_flag)
            {
                Cpp_Bus_Driver::Touch_Point touch_point;
                TCA8418->get_touch_point(&touch_point);

                printf("Touch Point: finger_count=%d\n", touch_point.finger_count);
                for (const auto &info : touch_point.info)
                {
                    printf("  Finger: x=%d, y=%d, press_status_flag=%d\n", info.x, info.y, info.press_status_flag);
                }
            }

            if (is.gpio_interrupt_flag)
            {
                printf("GPIO Interrupt Triggered!\n");
            }

            TCA8418->clear_irq_flag();

            Interrupt_Flag = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
