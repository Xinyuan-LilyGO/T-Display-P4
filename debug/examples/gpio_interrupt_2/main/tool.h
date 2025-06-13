/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 17:58:03
 * @LastEditTime: 2024-12-23 14:04:41
 * @License: GPL 3.0
 */

#pragma once

#include "driver/gpio.h"

namespace Cpp_Bus_Driver
{
    class Tool
    {
    public:
        enum class Log_Level
        {
            INFO, // 普通信息

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

        void assert_log(Log_Level level, const char *func_name, size_t line_number, const char *format, ...);

        void create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*Interrupt_Function)(void *));

        void delete_gpio_interrupt(uint32_t pin);
    };
}