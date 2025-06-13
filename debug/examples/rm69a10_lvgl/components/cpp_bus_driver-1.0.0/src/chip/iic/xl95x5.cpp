/*
 * @Description: PCF85063.cpp
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-12-25 16:44:19
 * @License: GPL 3.0
 */
#include "xl95x5.h"

namespace Cpp_Bus_Driver
{
    bool XL95x5::begin(int64_t frequency)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (IIC_Guide::begin(frequency) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        // if (_bus->BufferOperation(_address, PCF85063_Initialization_BufferOperations,
        //                           sizeof(PCF85063_Initialization_BufferOperations)) == false)
        // {
        //     return false;
        // }

        return true;
    }

    int16_t XL95x5::iic_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool XL95x5::pin_mode(Pin pin, Mode mode)
    {
        uint8_t buffer = 0;

        // 写0输出，写1输入
        if (static_cast<uint8_t>(pin) > 7)
        {
            if (_bus->read(static_cast<uint8_t>(Reg::RW_CONFIGURATION_PORT_1), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (mode == Mode::OUTPUT)
            {
                buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
            }
            else
            {
                buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
            }
            if (_bus->write(static_cast<uint8_t>(Reg::RW_CONFIGURATION_PORT_1), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else
        {
            if (_bus->read(static_cast<uint8_t>(Reg::RW_CONFIGURATION_PORT_0), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (mode == Mode::OUTPUT)
            {
                buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
            }
            else
            {
                buffer = buffer | (1 << static_cast<uint8_t>(pin));
            }
            if (_bus->write(static_cast<uint8_t>(Reg::RW_CONFIGURATION_PORT_0), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

    bool XL95x5::digital_write(Pin pin, Value value)
    {
        uint8_t buffer = 0;

        // 写0为低电平，写1为高电平
        if (static_cast<uint8_t>(pin) > 7)
        {
            if (_bus->read(static_cast<uint8_t>(Reg::RW_OUTPUT_PORT_1), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (value == Value::LOW)
            {
                buffer = buffer & (~(1 << (static_cast<uint8_t>(pin) - 10)));
            }
            else
            {
                buffer = buffer | (1 << (static_cast<uint8_t>(pin) - 10));
            }

            if (_bus->write(static_cast<uint8_t>(Reg::RW_OUTPUT_PORT_1), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else
        {
            if (_bus->read(static_cast<uint8_t>(Reg::RW_OUTPUT_PORT_0), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            if (value == Value::LOW)
            {
                buffer = buffer & (~(1 << static_cast<uint8_t>(pin)));
            }
            else
            {
                buffer = buffer | (1 << static_cast<uint8_t>(pin));
            }
            if (_bus->write(static_cast<uint8_t>(Reg::RW_OUTPUT_PORT_0), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

    int8_t XL95x5::digital_read(Pin pin)
    {
        uint8_t buffer = 0;
        
        // 写0为低电平，写1为高电平
        if (static_cast<uint8_t>(pin) > 7)
        {
            if (_bus->read(static_cast<uint8_t>(Reg::RO_INPUT_PORT_1), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
            buffer = (buffer >> (static_cast<uint8_t>(pin) - 10)) & 0B00000001;
        }
        else
        {
            if (_bus->read(static_cast<uint8_t>(Reg::RO_INPUT_PORT_0), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
            buffer = (buffer >> static_cast<uint8_t>(pin)) & 0B00000001;
        }
        
        return buffer;
    }
}
