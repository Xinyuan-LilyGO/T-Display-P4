/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 10:22:46
 * @LastEditTime: 2025-02-12 10:59:06
 * @License: GPL 3.0
 */
#include "tool.h"

namespace Cpp_Bus_Driver
{
    void Tool::assert_log(Log_Level level, const char *file_name, size_t line_number, const char *format, ...)
    {
#if defined CPP_BUS_LOG_LEVEL_BUS || defined CPP_BUS_LOG_LEVEL_CHIP || defined CPP_BUS_LOG_LEVEL_INFO

        va_list args;
        va_start(args, format);
        char buffer[256];
        switch (level)
        {
        case Log_Level::INFO:
#if defined CPP_BUS_LOG_LEVEL_INFO
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log info]->[%s][%u line]: %s", file_name, line_number, format);
#endif
            break;
        case Log_Level::BUS:
#if defined CPP_BUS_LOG_LEVEL_BUS
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log bus]->[%s][%u line]: %s", file_name, line_number, format);
#endif
            break;
        case Log_Level::CHIP:
#if defined CPP_BUS_LOG_LEVEL_CHIP
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log chip]->[%s][%u line]: %s", file_name, line_number, format);
#endif
            break;

        default:
            break;
        }

        vprintf(buffer, args);
        va_end(args);

#endif
    }

    bool Tool::pin_mode(uint32_t pin, Pin_Mode mode, Pin_Status status)
    {
        gpio_config_t config;
        config.pin_bit_mask = BIT64(pin);
        switch (mode)
        {
        case Pin_Mode::INPUT:
            config.mode = GPIO_MODE_INPUT;
            break;
        case Pin_Mode::OUTPUT:
            config.mode = GPIO_MODE_OUTPUT;
            break;
        case Pin_Mode::OUTPUT_OD:
            config.mode = GPIO_MODE_OUTPUT_OD;
            break;
        case Pin_Mode::INPUT_OUTPUT_OD:
            config.mode = GPIO_MODE_INPUT_OUTPUT_OD;
            break;
        case Pin_Mode::INPUT_OUTPUT:
            config.mode = GPIO_MODE_INPUT_OUTPUT;
            break;

        default:
            break;
        }
        switch (status)
        {
        case Pin_Status::DISABLE:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case Pin_Status::PULLUP:
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case Pin_Status::PULLDOWN:
            config.pull_up_en = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;

        default:
            break;
        }
        config.intr_type = GPIO_INTR_DISABLE;
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
        config.hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE;
#endif

        esp_err_t assert = gpio_config(&config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_config fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Tool::pin_write(uint32_t pin, bool value)
    {
        esp_err_t assert = gpio_set_level(static_cast<gpio_num_t>(pin), value);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_config fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Tool::pin_read(uint32_t pin)
    {
        return gpio_get_level(static_cast<gpio_num_t>(pin));
    }

    bool Tool::create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*interrupt)(void *))
    {
        gpio_config_t config;
        config.pin_bit_mask = BIT64(pin);
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

        esp_err_t assert = gpio_config(&config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_config fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_install_isr_service(0);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_install_isr_service fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_isr_handler_add(static_cast<gpio_num_t>(pin), interrupt, (void *)pin);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_isr_handler_add fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_intr_enable(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_intr_enable fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Tool::delete_gpio_interrupt(uint32_t pin)
    {
        esp_err_t assert = gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_set_intr_type fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_isr_handler_remove fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_intr_disable(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_intr_disable fail (error code: %#X)\n", assert);
            return false;
        }

        assert = gpio_reset_pin(static_cast<gpio_num_t>(pin));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "gpio_reset_pin fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    void Tool::delay_ms(uint32_t value)
    {
        vTaskDelay(pdMS_TO_TICKS(value));
    }

    void Tool::delay_us(uint32_t value)
    {
        usleep(value);
    }
}