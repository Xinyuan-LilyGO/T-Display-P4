/*
 * @Description: tca8418
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-23 16:03:37
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(XL9555_SDA, XL9555_SCL);

auto Tca8418_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(TCA8418_SDA, TCA8418_SCL);

auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);

auto Tca8418 = std::make_unique<Cpp_Bus_Driver::Tca8418>(Tca8418_Iic_Bus, TCA8418_IIC_ADDRESS);

volatile bool Interrupt_Flag = false;

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(4, 3300);

    Xl9555->begin();
    Xl9555->pin_mode(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::LOW); // 开启led
    Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9555->pin_mode(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    Tca8418->create_gpio_interrupt(TCA8418_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Interrupt_Flag = true;
                                   });

    // Tca8418_Iic_Bus->set_bus_handle(Xl9555_Iic_Bus->get_bus_handle());

    Tca8418->begin();
    Tca8418->set_keypad_scan_window(0, 0, TCA8418_KEYPAD_SCAN_WIDTH, TCA8418_KEYPAD_SCAN_HEIGHT);
    Tca8418->set_irq_pin_mode(Cpp_Bus_Driver::Tca8418::Irq_Mask::KEY_EVENTS);
    Tca8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);

    Tca8418->create_pwm(KEYBOARD_BL, ledc_channel_t::LEDC_CHANNEL_1, 1000000,
                        0, ledc_mode_t::LEDC_LOW_SPEED_MODE, ledc_timer_bit_t ::LEDC_TIMER_5_BIT);
    Tca8418->start_pwm_gradient_time(30, 1000);

    while (1)
    {
        if (Interrupt_Flag == true)
        {
            Cpp_Bus_Driver::Tca8418::Irq_Status is;

            if (Tca8418->parse_irq_status(Tca8418->get_irq_flag(), is) == false)
            {
                printf("parse_irq_status fail\n");
            }
            else
            {
                if (is.key_events_flag == true)
                {
                    Cpp_Bus_Driver::Tca8418::Touch_Point tp;
                    if (Tca8418->get_multiple_touch_point(tp) == true)
                    {
                        printf("touch finger: %d\n", tp.finger_count);

                        for (uint8_t i = 0; i < tp.info.size(); i++)
                        {
                            switch (tp.info[i].event_type)
                            {
                            case Cpp_Bus_Driver::Tca8418::Event_Type::KEYPAD:
                            {
                                Cpp_Bus_Driver::Tca8418::Touch_Position tp_2;
                                if (Tca8418->parse_touch_num(tp.info[i].num, tp_2) == true)
                                {
                                    printf("keypad event\n");
                                    printf("   touch num:[%d] num: %d x: %d y: %d press_flag: %d\n", i + 1, tp.info[i].num, tp_2.x, tp_2.y, tp.info[i].press_flag);
                                    if (tp.info[i].num <= (sizeof(Tca8418_Map) / sizeof(std::string)))
                                    {
                                        printf("   touch string: %s\n", Tca8418_Map[tp.info[i].num - 1].c_str());
                                    }
                                }

                                break;
                            }
                            case Cpp_Bus_Driver::Tca8418::Event_Type::GPIO:
                                printf("gpio event\n");
                                printf("   touch num:[%d] num: %d press_flag: %d\n", i + 1, tp.info[i].num, tp.info[i].press_flag);
                                break;

                            default:
                                break;
                            }
                        }
                    }

                    Tca8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);
                }
            }

            Interrupt_Flag = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
