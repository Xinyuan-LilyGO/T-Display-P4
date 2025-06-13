/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 10:22:46
 * @LastEditTime: 2024-12-23 14:52:02
 * @License: GPL 3.0
 */
#include "tool.h"

namespace Cpp_Bus_Driver
{
    void Tool::assert_log(Log_Level level, const char *func_name, size_t line_number, const char *format, ...)
    {
#if defined CPP_BUS_LOG_LEVEL_BUS || defined CPP_BUS_LOG_LEVEL_CHIP || defined CPP_BUS_LOG_LEVEL_INFO

        va_list args;
        va_start(args, format);
        char buffer[256];
        switch (level)
        {
        case Log_Level::INFO:
#if defined CPP_BUS_LOG_LEVEL_INFO
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log info]->[%s][%u line]: %s", func_name, line_number, format);
#endif
            break;
        case Log_Level::BUS:
#if defined CPP_BUS_LOG_LEVEL_BUS
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log bus]->[%s][%u line]: %s", func_name, line_number, format);
#endif
            break;
        case Log_Level::CHIP:
#if defined CPP_BUS_LOG_LEVEL_CHIP
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log chip]->[%s][%u line]: %s", func_name, line_number, format);
#endif
            break;

        default:
            break;
        }

        vprintf(buffer, args);
        va_end(args);

#endif
    }

    void Tool::create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*Interrupt_Function)(void *))
    {
        gpio_config_t config;
        config.pin_bit_mask = 1ull << pin;
        config.mode = GPIO_MODE_INPUT;
        switch (mode)
        {
        case Interrupt_Mode::RISING:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            config.intr_type = GPIO_INTR_POSEDGE;
            break;
        case Interrupt_Mode::FALLING:
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_NEGEDGE;
            break;
        case Interrupt_Mode::CHANGE:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_ANYEDGE;
            break;
        case Interrupt_Mode::ONLOW:
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_LOW_LEVEL;
            break;
        case Interrupt_Mode::ONHIGH:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            config.intr_type = GPIO_INTR_HIGH_LEVEL;
            break;

        default:
            break;
        }
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

        gpio_config(&config);

        gpio_install_isr_service(0);

        gpio_isr_handler_add(static_cast<gpio_num_t>(pin), Interrupt_Function, (void *)pin);

        gpio_intr_enable(static_cast<gpio_num_t>(pin));
    }

    void Tool::delete_gpio_interrupt(uint32_t pin)
    {
        gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);

        gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));

        gpio_intr_disable(static_cast<gpio_num_t>(pin));

        gpio_reset_pin(static_cast<gpio_num_t>(pin));
    }
}