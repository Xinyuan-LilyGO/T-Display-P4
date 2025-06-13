/*
 * @Description: PCF85063.cpp
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-12-27 10:28:08
 * @License: GPL 3.0
 */
#include "sgm38121.h"

namespace Cpp_Bus_Driver
{
    bool SGM38121::begin(int64_t frequency)
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

    int16_t SGM38121::iic_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool SGM38121::set_output_voltage(Channel channel, uint16_t voltage)
    {
        uint8_t buffer = 0;

        switch (channel)
        {
        case Channel::DVDD_1:
            if ((voltage < 528) && (voltage > 1504))
            {
                assert_log(Log_Level::WARN, __FILE__, __LINE__, "voltage setting out of range\n");
                return false;
            }
            break;
        case Channel::DVDD_2:
            if ((voltage < 528) && (voltage > 1504))
            {
                assert_log(Log_Level::WARN, __FILE__, __LINE__, "voltage setting out of range\n");
                return false;
            }
            break;
        case Channel::AVDD_1:
            if ((voltage < 1504) && (voltage > 3424))
            {
                assert_log(Log_Level::WARN, __FILE__, __LINE__, "voltage setting out of range\n");
                return false;
            }
            buffer = (voltage - 1348) / 8;
            if (_bus->write(static_cast<uint8_t>(Reg::RW_AVDD1_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Channel::AVDD_2:
            if ((voltage < 1504) && (voltage > 3424))
            {
                assert_log(Log_Level::WARN, __FILE__, __LINE__, "voltage setting out of range\n");
                return false;
            }
            buffer = (voltage - 1348) / 8;
            if (_bus->write(static_cast<uint8_t>(Reg::RW_AVDD2_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;

        default:
            break;
        }

        return true;
    }

    bool SGM38121::set_channel_state(Channel channel, State state)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Reg::RW_ENABLE_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        switch (channel)
        {
        case Channel::DVDD_1:
            /* code */
            break;
        case Channel::DVDD_2:
            /* code */
            break;
        case Channel::AVDD_1:
            switch (state)
            {
            case State::ON:
                buffer |= 0B00000100;
                break;
            case State::OFF:
                buffer &= 0B11111011;
                break;

            default:
                break;
            }
            break;
        case Channel::AVDD_2:
            switch (state)
            {
            case State::ON:
                buffer |= 0B00001000;
                break;
            case State::OFF:
                buffer &= 0B11110111;
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
        if (_bus->write(static_cast<uint8_t>(Reg::RW_ENABLE_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }
}
