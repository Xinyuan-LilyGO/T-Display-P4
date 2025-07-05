/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 17:58:03
 * @LastEditTime: 2025-07-04 11:23:48
 * @License: GPL 3.0
 */

#pragma once

#include "config.h"

namespace Cpp_Bus_Driver
{
    class Tool
    {

    private:
        struct Pwm
        {
            ledc_channel_t channel = ledc_channel_t::LEDC_CHANNEL_0;
            uint32_t freq_hz = 2000;
            uint32_t duty = 0;
            ledc_mode_t speed_mode = ledc_mode_t::LEDC_LOW_SPEED_MODE;
            ledc_timer_bit_t duty_resolution = ledc_timer_bit_t::LEDC_TIMER_10_BIT;
            ledc_timer_t timer_num = ledc_timer_t::LEDC_TIMER_0;
            ledc_sleep_mode_t sleep_mode = ledc_sleep_mode_t::LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
        };

    protected:
        enum class Init_List_Cmd
        {
            DELAY_MS = 0,
            WRITE_DATA,
            WRITE_C8_D8,
            WRITE_C8_R24,
            WRITE_C8_R24_D8,
        };

        enum class Endian
        {
            BIG,
            LITTLE,
        };

    public:
        enum class Log_Level
        {
            DEBUG, // debug信息
            INFO,  // 普通信息

            BUS,  // 总线错误
            CHIP, // 芯片错误
        };

        enum class Interrupt_Mode
        {
            RISING = 0,
            FALLING,
            CHANGE,
            ONLOW,
            ONHIGH,
        };

        enum class Pin_Mode
        {
            DISABLE = 0,
            INPUT,           // input only
            OUTPUT,          // output only mode
            OUTPUT_OD,       // output only with open-drain mode
            INPUT_OUTPUT_OD, // output and input with open-drain mode
            INPUT_OUTPUT,    // output and input mode
        };

        enum class Pin_Status
        {
            DISABLE,
            PULLUP,
            PULLDOWN,
        };

        Pwm _pwm;

        Tool()
        {
        }

        void assert_log(Log_Level level, const char *file_name, size_t line_number, const char *format, ...);

        /**
         * @brief 搜索函数
         * @param *search_library 需要使用的搜索库
         * @param search_library_length 需要使用的搜索库长度
         * @param *search_sample 需要搜索的样本
         * @param sample_length 需要搜索的样本长度
         * @param *search_index 搜索成功的的样本中第一个搜索成功字符位置的引索
         * @return
         * @Date 2025-03-26 16:41:35
         */
        bool search(const uint8_t *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index = nullptr);
        bool search(const char *search_library, size_t search_library_length, const char *search_sample, size_t sample_length, size_t *search_index = nullptr);

        bool pin_mode(uint32_t pin, Pin_Mode mode, Pin_Status status = Pin_Status::DISABLE);
        bool pin_write(uint32_t pin, bool value);
        bool pin_read(uint32_t pin);

        bool create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*interrupt)(void *));
        bool delete_gpio_interrupt(uint32_t pin);

        void delay_ms(uint32_t value);
        void delay_us(uint32_t value);

        bool create_pwm(int32_t pin, ledc_channel_t channel, uint32_t freq_hz, uint32_t duty = 0, ledc_mode_t speed_mode = ledc_mode_t::LEDC_LOW_SPEED_MODE,
                        ledc_timer_bit_t duty_resolution = ledc_timer_bit_t::LEDC_TIMER_10_BIT, ledc_timer_t timer_num = ledc_timer_t::LEDC_TIMER_0,
                        ledc_sleep_mode_t sleep_mode = ledc_sleep_mode_t::LEDC_SLEEP_MODE_NO_ALIVE_NO_PD);

        /**
         * @brief 设置占空比
         * @param duty 值范围0~100
         * @return
         * @Date 2025-03-19 10:02:18
         */
        bool set_pwm_duty(uint8_t duty);
        bool set_pwm_frequency(uint32_t freq_hz);

        /**
         * @brief 设置渐变占空比
         * @param target_duty 值范围0~100
         * @param time_ms 渐变花费时间
         * @return
         * @Date 2025-03-19 10:03:07
         */
        bool start_pwm_gradient_time(uint8_t target_duty, int32_t time_ms);

        bool stop_pwm(uint32_t idle_level = 0);
    };
}