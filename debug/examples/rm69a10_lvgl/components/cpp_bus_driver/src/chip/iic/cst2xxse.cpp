/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:51
 * @LastEditTime: 2025-07-07 11:59:20
 * @License: GPL 3.0
 */
#include "cst2xxse.h"

namespace Cpp_Bus_Driver
{
    bool Cst2xxse::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            pin_write(_rst, 1);
            delay_ms(10);
            pin_write(_rst, 0);
            delay_ms(10);
            pin_write(_rst, 1);
            delay_ms(30);
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        uint8_t buffer = get_device_id();
        if (buffer != static_cast<uint8_t>(CST2xxSE_DEVICE_ID))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get cst2xxse id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get cst2xxse id: %#X\n", buffer);
        }

        return true;
    }

    uint8_t Cst2xxse::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    uint8_t Cst2xxse::get_finger_count(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_FINGER_COUNT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer & 0B00001111;
    }

    bool Cst2xxse::get_single_touch_point(Touch_Point &tp, uint8_t finger_num)
    {
        if ((finger_num == 0) || (finger_num > MAX_TOUCH_FINGER_COUNT))
        {
            return false;
        }

        uint8_t buffer[SINGLE_TOUCH_POINT_DATA_SIZE] = {0};

        if (finger_num == 1)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_TOUCH_POINT_INFO_START) + ((finger_num - 1) * SINGLE_TOUCH_POINT_DATA_SIZE), buffer, SINGLE_TOUCH_POINT_DATA_SIZE) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }
        else
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_TOUCH_POINT_INFO_START) + ((finger_num - 1) * SINGLE_TOUCH_POINT_DATA_SIZE) + 2, buffer, SINGLE_TOUCH_POINT_DATA_SIZE) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }

        tp.finger_count = finger_num;

        Touch_Info buffer_ti;
        buffer_ti.x = (static_cast<uint16_t>(buffer[1]) << 4) | ((buffer[3] & 0B11110000) >> 4);
        buffer_ti.y = (static_cast<uint16_t>(buffer[2]) << 4) | (buffer[3] & 0B00001111);
        buffer_ti.pressure_value = buffer[4];

        tp.info.push_back(buffer_ti);

        tp.home_touch_flag = false;

        return true;
    }

    bool Cst2xxse::get_multiple_touch_point(Touch_Point &tp)
    {
        const uint8_t buffer_touch_point_size = MAX_TOUCH_FINGER_COUNT * SINGLE_TOUCH_POINT_DATA_SIZE + 2;
        uint8_t buffer[buffer_touch_point_size] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TOUCH_POINT_INFO_START), buffer, buffer_touch_point_size) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        // 如果手指数为0
        if ((buffer[5] & 0B00001111) == 0)
        {
            return false;
        }
        tp.finger_count = buffer[5] & 0B00001111;

        for (uint8_t i = 1; i <= tp.finger_count; i++)
        {
            uint8_t buffer_touch_point_offset;
            if (i == 1)
            {
                buffer_touch_point_offset = i * SINGLE_TOUCH_POINT_DATA_SIZE - SINGLE_TOUCH_POINT_DATA_SIZE;
            }
            else
            {
                buffer_touch_point_offset = i * SINGLE_TOUCH_POINT_DATA_SIZE - SINGLE_TOUCH_POINT_DATA_SIZE + 2;
            }

            Touch_Info buffer_ti;
            buffer_ti.x = (static_cast<uint16_t>(buffer[buffer_touch_point_offset + 1]) << 4) |
                          ((buffer[buffer_touch_point_offset + 3] & 0B11110000) >> 4);
            buffer_ti.y = (static_cast<uint16_t>(buffer[buffer_touch_point_offset + 2]) << 4) |
                          (buffer[buffer_touch_point_offset + 3] & 0B00001111);
            buffer_ti.pressure_value = buffer[buffer_touch_point_offset + 4];

            tp.info.push_back(buffer_ti);
        }

        if ((buffer[5] & 0B10000000) > 0)
        {
            tp.home_touch_flag = true;
        }
        else
        {
            tp.home_touch_flag = false;
        }

        return true;
    }

    bool Cst2xxse::get_home_touch(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_FINGER_COUNT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if ((buffer & 0B10000000) == 0)
        {
            return false;
        }

        return true;
    }

}
