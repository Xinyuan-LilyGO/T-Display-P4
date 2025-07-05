/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-06-12 10:49:15
 * @License: GPL 3.0
 */
#include "sgm38121.h"

namespace Cpp_Bus_Driver
{
    bool Sgm38121::begin(int32_t freq_hz)
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
        if (buffer != SGM38121_DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm38121 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm38121 id: %#X\n", buffer);
        }

        return true;
    }

    uint8_t Sgm38121::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sgm38121::set_output_voltage(Channel channel, uint16_t voltage)
    {
        uint8_t buffer = 0;

        switch (channel)
        {
        case Channel::DVDD_1:
            if (voltage < 528)
            {
                voltage = 528;
            }
            else if (voltage > 1504)
            {
                voltage = 1504;
            }
            buffer = (voltage - 504) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_DVDD1_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Channel::DVDD_2:
            if (voltage < 528)
            {
                voltage = 528;
            }
            else if (voltage > 1504)
            {
                voltage = 1504;
            }
            buffer = (voltage - 504) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_DVDD2_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Channel::AVDD_1:
            if (voltage < 1504)
            {
                voltage = 1504;
            }
            else if (voltage > 3424)
            {
                voltage = 3424;
            }
            buffer = (voltage - 1348) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_AVDD1_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Channel::AVDD_2:
            if (voltage < 1504)
            {
                voltage = 1504;
            }
            else if (voltage > 3424)
            {
                voltage = 3424;
            }
            buffer = (voltage - 1348) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_AVDD2_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
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

    bool Sgm38121::set_channel_status(Channel channel, Status status)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_ENABLE_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        switch (channel)
        {
        case Channel::DVDD_1:
            switch (status)
            {
            case Status::ON:
                buffer |= 0B00000001;
                break;
            case Status::OFF:
                buffer &= 0B11111110;
                break;

            default:
                break;
            }
            break;
        case Channel::DVDD_2:
            switch (status)
            {
            case Status::ON:
                buffer |= 0B00000010;
                break;
            case Status::OFF:
                buffer &= 0B11111101;
                break;

            default:
                break;
            }
            break;
        case Channel::AVDD_1:
            switch (status)
            {
            case Status::ON:
                buffer |= 0B00000100;
                break;
            case Status::OFF:
                buffer &= 0B11111011;
                break;

            default:
                break;
            }
            break;
        case Channel::AVDD_2:
            switch (status)
            {
            case Status::ON:
                buffer |= 0B00001000;
                break;
            case Status::OFF:
                buffer &= 0B11110111;
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_ENABLE_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }
}
