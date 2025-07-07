/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 10:22:46
 * @LastEditTime: 2025-06-18 14:49:15
 * @License: GPL 3.0
 */
#include "tool.h"

namespace Cpp_Bus_Driver
{
    void Tool::assert_log(Log_Level level, const char *file_name, size_t line_number, const char *format, ...)
    {
#if defined CPP_BUS_LOG_LEVEL_BUS || defined CPP_BUS_LOG_LEVEL_CHIP || defined CPP_BUS_LOG_LEVEL_INFO || defined CPP_BUS_LOG_LEVEL_DEBUG

        switch (level)
        {
        case Log_Level::DEBUG:
        {
#if defined CPP_BUS_LOG_LEVEL_DEBUG
            va_list args;
            va_start(args, format);
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log debug]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer, args);
            va_end(args);
#endif
            break;
        }
        case Log_Level::INFO:
        {
#if defined CPP_BUS_LOG_LEVEL_INFO
            va_list args;
            va_start(args, format);
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log info]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer, args);
            va_end(args);
#endif
            break;
        }
        case Log_Level::BUS:
        {
#if defined CPP_BUS_LOG_LEVEL_BUS
            va_list args;
            va_start(args, format);
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log bus]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer, args);
            va_end(args);
#endif
            break;
        }
        case Log_Level::CHIP:
        {
#if defined CPP_BUS_LOG_LEVEL_CHIP
            va_list args;
            va_start(args, format);
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "[cpp_bus_driver][log chip]->[%s][%u line]: %s", file_name, line_number, format);
            vprintf(buffer, args);
            va_end(args);
#endif
            break;
        }

        default:
            break;
        }

#endif
    }

    bool Tool::search(const uint8_t *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index)
    {
        // 检查参数有效性
        if (search_sample == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_sample == nullptr)\n");
            return false;
        }
        else if (search_library == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library == nullptr\n");
            return false;
        }
        else if (sample_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length == 0)\n");
            return false;
        }
        else if (search_library_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library_length == 0)\n");
            return false;
        }
        else if (sample_length > search_library_length)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length > search_library_length)\n");
            return false;
        }

        auto buffer = std::search(search_library, search_library + search_library_length, search_sample, search_sample + sample_length);
        // 检查是否找到了数据
        if (buffer == (search_library + search_library_length))
        {
            // assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search fail\n");
            return false;
        }

        if (search_index != nullptr)
        {
            *search_index = buffer - search_library;
        }

        return true;
    }

    bool Tool::search(const char *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index)
    {
        // 检查参数有效性
        if (search_sample == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_sample == nullptr)\n");
            return false;
        }
        else if (search_library == nullptr)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library == nullptr\n");
            return false;
        }
        else if (sample_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length == 0)\n");
            return false;
        }
        else if (search_library_length == 0)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(search_library_length == 0)\n");
            return false;
        }
        else if (sample_length > search_library_length)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search invalid(sample_length > search_library_length)\n");
            return false;
        }

        auto buffer = std::search(search_library, search_library + search_library_length, search_sample, search_sample + sample_length);
        // 检查是否找到了数据
        if (buffer == (search_library + search_library_length))
        {
            // assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "search fail\n");
            return false;
        }

        if (search_index != nullptr)
        {
            *search_index = buffer - search_library;
        }

        return true;
    }

    bool Tool::pin_mode(uint32_t pin, Pin_Mode mode, Pin_Status status)
    {
        gpio_config_t config;
        config.pin_bit_mask = BIT64(pin);
        switch (mode)
        {
        case Pin_Mode::DISABLE:
            config.mode = GPIO_MODE_INPUT;
            break;
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
            // 只要 GPIO 引脚保持低电平，就会持续触发中断
            // 需要确保你的中断处理函数能够处理这种情况，或者你的外部信号不会长时间保持低电平
            // 否则系统将崩溃重启
            config.pull_up_en = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            config.intr_type = GPIO_INTR_LOW_LEVEL;
            break;
        case Interrupt_Mode::ONHIGH:
            // 只要 GPIO 引脚保持高电平，就会持续触发中断
            // 需要确保你的中断处理函数能够处理这种情况，或者你的外部信号不会长时间保持高电平
            // 否则系统将崩溃重启
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
        // 默认状态下vTaskDelay在小于10ms延时的时候不精确
        vTaskDelay(pdMS_TO_TICKS(value));
    }

    void Tool::delay_us(uint32_t value)
    {
        usleep(value);
    }

    bool Tool::create_pwm(int32_t pin, ledc_channel_t channel, uint32_t freq_hz, uint32_t duty, ledc_mode_t speed_mode,
                          ledc_timer_bit_t duty_resolution, ledc_timer_t timer_num, ledc_sleep_mode_t sleep_mode)
    {
        const ledc_timer_config_t buffer_ledc_timer_config =
            {
                .speed_mode = speed_mode,
                .duty_resolution = duty_resolution, // LEDC驱动器占空比精度
                .timer_num = timer_num,             // ledc使用的定时器编号，若需要生成多个频率不同的PWM信号，则需要指定不同的定时器
                .freq_hz = freq_hz,                 // PWM频率
                .clk_cfg = LEDC_AUTO_CLK,           // 自动选择定时器的时钟源
                .deconfigure = false,
            };

        esp_err_t assert = ledc_timer_config(&buffer_ledc_timer_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_timer_config fail (error code: %#X)\n", assert);
            return false;
        }

        const ledc_channel_config_t buffer_ledc_channel_config =
            {
                .gpio_num = pin,
                .speed_mode = speed_mode,
                .channel = channel,
                .intr_type = LEDC_INTR_DISABLE,
                .timer_sel = timer_num,
                .duty = duty,
                .hpoint = 0,
                .sleep_mode = sleep_mode,
                .flags =
                    {
                        .output_invert = false,
                    },
            };

        assert = ledc_channel_config(&buffer_ledc_channel_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_channel_config fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.channel = channel;
        _pwm.freq_hz = freq_hz;
        _pwm.duty = duty;
        _pwm.speed_mode = speed_mode;
        _pwm.duty_resolution = duty_resolution;
        _pwm.timer_num = timer_num;
        _pwm.sleep_mode = sleep_mode;

        return true;
    }

    bool Tool::set_pwm_duty(uint8_t duty)
    {
        if (duty > 100)
        {
            duty = 100;
        }

        esp_err_t assert = ledc_set_duty(_pwm.speed_mode, _pwm.channel, (static_cast<float>(duty) / 100.0) * (1 << static_cast<uint8_t>(_pwm.duty_resolution)));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_set_duty fail (error code: %#X)\n", assert);
            return false;
        }

        assert = ledc_update_duty(_pwm.speed_mode, _pwm.channel);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_update_duty fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.duty = duty;

        return true;
    }

    bool Tool::set_pwm_frequency(uint32_t freq_hz)
    {
        esp_err_t assert = ledc_set_freq(_pwm.speed_mode, _pwm.timer_num, freq_hz);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_set_duty fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.freq_hz = freq_hz;

        return true;
    }

    bool Tool::start_pwm_gradient_time(uint8_t target_duty, int32_t time_ms)
    {
        if (target_duty > 100)
        {
            target_duty = 100;
        }

        esp_err_t assert = ledc_fade_func_install(false);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_fade_func_install fail (error code: %#X)\n", assert);
            // return false;
        }

        assert = ledc_set_fade_with_time(_pwm.speed_mode, _pwm.channel,
                                         (static_cast<float>(target_duty) / 100.0) * (1 << static_cast<uint8_t>(_pwm.duty_resolution)), time_ms);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_set_fade fail (error code: %#X)\n", assert);
            return false;
        }

        assert = ledc_fade_start(_pwm.speed_mode, _pwm.channel, ledc_fade_mode_t::LEDC_FADE_WAIT_DONE);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_fade_start fail (error code: %#X)\n", assert);
            return false;
        }

        _pwm.duty = target_duty;

        return true;
    }

    bool Tool::stop_pwm(uint32_t idle_level)
    {
        esp_err_t assert = ledc_stop(_pwm.speed_mode, _pwm.channel, idle_level);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "ledc_stop fail (error code: %#X)\n", assert);
            return false;
        }

        ledc_fade_func_uninstall();

        return true;
    }
}