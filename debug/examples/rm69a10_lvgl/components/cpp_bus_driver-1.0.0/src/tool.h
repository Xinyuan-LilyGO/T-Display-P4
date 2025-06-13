/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 17:58:03
 * @LastEditTime: 2024-12-27 09:55:49
 * @License: GPL 3.0
 */

#pragma once

#include "config.h"

namespace Cpp_Bus_Driver
{
    class Tool
    {
    public:
        enum class Log_Level
        {
            INFO, // 普通信息
            WARN, // 警告信息

            BUS,  // 总线错误
            CHIP, // 芯片错误
        };

        enum class Interrupt_Mode
        {
            RISING = 0x01,
            FALLING,
            CHANGE,
            ONLOW,
            ONHIGH,
        };

        Tool()
        {
        }

        void assert_log(Log_Level level, const char *file_name, size_t line_number, const char *format, ...);

        bool create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*Interrupt_Function)(void *));

        bool delete_gpio_interrupt(uint32_t pin);

        void delay_ms(uint32_t value);
    };
}