/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2025-07-07 11:39:53
 * @License: GPL 3.0
 */
#include "ft3x68.h"

namespace Cpp_Bus_Driver
{
    bool Ft3x68::begin(int32_t freq_hz)
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
        if (buffer != static_cast<uint8_t>(FT3168_DEVICE_ID))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get ft3x68 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get ft3x68 id: %#X\n", buffer);
        }

        return true;
    }

    uint8_t Ft3x68::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    uint8_t Ft3x68::get_finger_count(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TD_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Ft3x68::get_single_touch_point(Touch_Point &tp, uint8_t finger_num)
    {
        if ((finger_num == 0) || (finger_num > MAX_TOUCH_FINGER_COUNT))
        {
            return false;
        }

        uint8_t buffer[SINGLE_TOUCH_POINT_DATA_SIZE] = {0};

        // 地址自动偏移
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_P1_XH) + ((finger_num - 1) * SINGLE_TOUCH_POINT_DATA_SIZE), buffer, SINGLE_TOUCH_POINT_DATA_SIZE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        tp.finger_count = finger_num;

        Touch_Info buffer_ti;
        buffer_ti.x = (static_cast<uint16_t>(buffer[0] & 0B00001111) << 8) | buffer[1];
        buffer_ti.y = (static_cast<uint16_t>(buffer[2] & 0B00001111) << 8) | buffer[3];

        tp.info.push_back(buffer_ti);

        return true;
    }

    bool Ft3x68::get_multiple_touch_point(Touch_Point &tp)
    {
        // +1 把手指数的地址也读出来
        const uint8_t buffer_touch_point_size = MAX_TOUCH_FINGER_COUNT * SINGLE_TOUCH_POINT_DATA_SIZE + 1;
        uint8_t buffer[buffer_touch_point_size] = {0};

        // 地址自动偏移
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TD_STATUS), buffer, buffer_touch_point_size) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        // 如果手指数为0或者大于最大触摸手指数
        if ((buffer[0] == 0) || (buffer[0] > MAX_TOUCH_FINGER_COUNT))
        {
            return false;
        }
        tp.finger_count = buffer[0];

        for (uint8_t i = 0; i < tp.finger_count; i++)
        {
            const uint8_t buffer_touch_point_offset = i * SINGLE_TOUCH_POINT_DATA_SIZE + 1;

            Touch_Info buffer_ti;
            buffer_ti.x = (static_cast<uint16_t>(buffer[buffer_touch_point_offset] & 0B00001111) << 8) | buffer[buffer_touch_point_offset + 1];
            buffer_ti.y = (static_cast<uint16_t>(buffer[buffer_touch_point_offset + 2] & 0B00001111) << 8) | buffer[buffer_touch_point_offset + 3];

            tp.info.push_back(buffer_ti);
        }

        return true;
    }

}
