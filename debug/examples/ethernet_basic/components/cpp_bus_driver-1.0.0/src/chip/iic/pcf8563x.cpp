/*
 * @Description: PCF85063.cpp
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2025-01-03 15:28:18
 * @License: GPL 3.0
 */
#include "pcf8563x.h"

namespace Cpp_Bus_Driver
{
    const uint8_t PCF8563x::Init_List[] =
        {
            static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8),
            static_cast<uint8_t>(Reg::RW_CLKOUT_CONTROL),
            0B00000000,
    };

    bool PCF8563x::begin(int64_t frequency)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (IIC_Guide::begin(frequency) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        if (iic_init_list(Init_List, sizeof(Init_List)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list fail\n");
            return false;
        }

        return true;
    }

    int16_t PCF8563x::iic_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool PCF8563x::set_clock_frequency_output(Clock_Frequency frequency)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_CLKOUT_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        switch (frequency)
        {
        case Clock_Frequency::OUTPUT_OFF:
            buffer &= 0B01111111;
            break;
        case Clock_Frequency::OUTPUT_1:
            buffer |= 0B10000011;
            break;
        case Clock_Frequency::OUTPUT_32:
            buffer = (buffer & 0B01111100) | 0B10000010;
            break;
        case Clock_Frequency::OUTPUT_1024:
            buffer = (buffer & 0B01111100) | 0B10000001;
            break;
        case Clock_Frequency::OUTPUT_32768:
            buffer = (buffer & 0B01111100) | 0B10000000;
            break;

        default:
            break;
        }
        if (_bus->write(static_cast<uint8_t>(Reg::RW_CLKOUT_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool PCF8563x::clock_run(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_CONTROL_STATUS_1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0B11011111;
        if (_bus->write(static_cast<uint8_t>(Reg::RW_CONTROL_STATUS_1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        return true;
    }

    bool PCF8563x::clock_stop(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_CONTROL_STATUS_1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer |= 0B00100000;
        if (_bus->write(static_cast<uint8_t>(Reg::RW_CONTROL_STATUS_1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        return true;
    }

    int8_t PCF8563x::get_second(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_VL_SECONDS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B01110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    int8_t PCF8563x::get_minute(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_MINUTES), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B01110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    int8_t PCF8563x::get_hour(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_HOURS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B00110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    int8_t PCF8563x::get_day(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_DAYS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B00110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    int8_t PCF8563x::get_month(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_CENTURY_MONTHS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B00010000) >> 4) * 10) + (buffer & 0B00001111);
    }

    int8_t PCF8563x::get_year(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg::RW_YEARS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B11110000) >> 4) * 10) + (buffer & 0B00001111);
    }

}
