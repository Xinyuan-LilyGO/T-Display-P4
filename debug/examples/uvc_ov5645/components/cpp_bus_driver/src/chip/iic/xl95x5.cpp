/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2025-05-15 10:00:47
 * @License: GPL 3.0
 */
#include "xl95x5.h"

namespace Cpp_Bus_Driver
{
    bool Xl95x5::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        uint8_t buffer = get_device_id();
        if (buffer == static_cast<uint8_t>(DEFAULT_CPP_BUS_DRIVER_VALUE))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get xl95x5 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get xl95x5 id: %#X\n", buffer);
        }

        return true;
    }

    uint8_t Xl95x5::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Xl95x5::pin_mode(Pin pin, Mode mode)
    {
        uint8_t buffer = 0;

        if (pin == Pin::IO_PORT0)
        {
            if (mode == Mode::OUTPUT)
            {
                buffer = 0B00000000;
            }
            else
            {
                buffer = 0B11111111;
            }
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONFIGURATION_PORT_0), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else if (pin == Pin::IO_PORT1)
        {
            if (mode == Mode::OUTPUT)
            {
                buffer = 0B00000000;
            }
            else
            {
                buffer = 0B11111111;
            }
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONFIGURATION_PORT_1), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else if (static_cast<uint8_t>(pin) > 7)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONFIGURATION_PORT_1), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (mode == Mode::OUTPUT) // 写0输出，写1输入
            {
                buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
            }
            else
            {
                buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
            }
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONFIGURATION_PORT_1), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONFIGURATION_PORT_0), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (mode == Mode::OUTPUT) // 写0输出，写1输入
            {
                buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
            }
            else
            {
                buffer = buffer | (1 << static_cast<uint8_t>(pin));
            }
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONFIGURATION_PORT_0), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

    bool Xl95x5::pin_write(Pin pin, Value value)
    {
        uint8_t buffer = 0;

        if (pin == Pin::IO_PORT0)
        {
            if (value == Value::LOW) // 写0为低电平，写1为高电平
            {
                buffer = 0B00000000;
            }
            else
            {
                buffer = 0B11111111;
            }
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_OUTPUT_PORT_0), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else if (pin == Pin::IO_PORT1)
        {
            if (value == Value::LOW) // 写0为低电平，写1为高电平
            {
                buffer = 0B00000000;
            }
            else
            {
                buffer = 0B11111111;
            }
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_OUTPUT_PORT_1), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        if (static_cast<uint8_t>(pin) > 7)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RW_OUTPUT_PORT_1), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (value == Value::LOW) // 写0为低电平，写1为高电平
            {
                buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
            }
            else
            {
                buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
            }

            if (_bus->write(static_cast<uint8_t>(Cmd::RW_OUTPUT_PORT_1), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RW_OUTPUT_PORT_0), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (value == Value::LOW) // 写0为低电平，写1为高电平
            {
                buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
            }
            else
            {
                buffer = buffer | (1 << static_cast<uint8_t>(pin));
            }
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_OUTPUT_PORT_0), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

    bool Xl95x5::pin_read(Pin pin)
    {
        uint8_t buffer = 0;

        // 写0为低电平，写1为高电平
        if (static_cast<uint8_t>(pin) > 7)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_INPUT_PORT_1), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
            buffer = (buffer >> (static_cast<uint8_t>(pin) - 10)) & 0B00000001;
        }
        else
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_INPUT_PORT_0), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
            buffer = (buffer >> static_cast<uint8_t>(pin)) & 0B00000001;
        }

        return buffer;
    }

    bool Xl95x5::clear_irq_flag(void)
    {
        uint8_t buffer = 0;

        for (uint8_t i = 0; i < 2; i++)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_INPUT_PORT_0) + i, &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }

        return true;
    }
}
