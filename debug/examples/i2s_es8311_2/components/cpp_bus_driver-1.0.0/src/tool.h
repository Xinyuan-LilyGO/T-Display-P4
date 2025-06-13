/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 17:58:03
 * @LastEditTime: 2025-02-24 15:50:04
 * @License: GPL 3.0
 */

#pragma once

#include "config.h"

namespace Cpp_Bus_Driver
{
    class Tool
    {
    protected:
        enum class Log_Level
        {
            INFO, // 普通信息

            BUS,  // 总线错误
            CHIP, // 芯片错误
        };

        enum class Init_List_Cmd
        {
            DELAY_MS = 0,
            WRITE_DATA,
            WRITE_C8_D8,
            WRITE_C8_R16_D8,
        };

        enum class Endian
        {
            BIG,
            LITTLE,
        };

        void assert_log(Log_Level level, const char *file_name, size_t line_number, const char *format, ...);

    public:
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
            INPUT = 0,       // input only
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

        Tool()
        {
        }

        bool pin_mode(uint32_t pin, Pin_Mode mode,Pin_Status status);
        bool pin_write(uint32_t pin, bool value);
        bool pin_read(uint32_t pin);

        bool create_gpio_interrupt(uint32_t pin, Interrupt_Mode mode, void (*interrupt)(void *));
        bool delete_gpio_interrupt(uint32_t pin);

        void delay_ms(uint32_t value);
        void delay_us(uint32_t value);
    };
}