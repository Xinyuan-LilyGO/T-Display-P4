/*
 * @Description:
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-03-28 10:18:55
 * @License: GPL 3.0
 */
#include "Hi8561_Touch.h"

namespace Cpp_Bus_Driver
{
    // const uint8_t Hi8561_Touch::Init_List[] =
    //     {
    //         static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8),
    //         static_cast<uint8_t>(Cmd::RW_CLKOUT_CONTROL),
    //         0B00000000,
    // };

    bool Hi8561_Touch::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        if (init_address_info() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_address_info fail\n");
            return false;
        }

        return true;
    }

    bool Hi8561_Touch::init_address_info(void)
    {
        uint8_t buffer[] =
            {
                0xF3,
                static_cast<uint8_t>(ESRAM_SECTION_INFO_START_ADDRESS >> 24),
                static_cast<uint8_t>(ESRAM_SECTION_INFO_START_ADDRESS >> 16),
                static_cast<uint8_t>(ESRAM_SECTION_INFO_START_ADDRESS >> 8),
                static_cast<uint8_t>(ESRAM_SECTION_INFO_START_ADDRESS),
                0x03,
            };

        uint8_t buffer_2[48] = {0};

        if (_bus->write_read(buffer, 6, buffer_2, 48) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        _touch_info_start_address = buffer_2[8] + (buffer_2[8 + 1] << 8) + (buffer_2[8 + 2] << 16) + (buffer_2[8 + 3] << 24);

        if ((_touch_info_start_address < MEMORY_ADDRESS_ERAM) || (_touch_info_start_address >= (MEMORY_ADDRESS_ERAM + MEMORY_ERAM_SIZE)))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_touch_info_start_address get error\n");
            _touch_info_start_address = 0;
            return false;
        }

        return true;
    }

    // uint8_t Hi8561_Touch::device_id(void)
    // {
    //     uint8_t buffer = 0;

    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return -1;
    //     }

    //     return buffer;
    // }

    uint8_t Hi8561_Touch::get_finger_count(void)
    {
        uint8_t buffer[] =
            {
                0xF3,
                static_cast<uint8_t>(_touch_info_start_address >> 24),
                static_cast<uint8_t>(_touch_info_start_address >> 16),
                static_cast<uint8_t>(_touch_info_start_address >> 8),
                static_cast<uint8_t>(_touch_info_start_address),
                0x03,
            };

        uint8_t buffer_2 = 0;

        if (_bus->write_read(buffer, 6, &buffer_2, 1) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return -1;
        }

        return buffer_2;
    }

    bool Hi8561_Touch::get_single_touch_point(Touch_Point &tp, uint8_t finger_num)
    {
        if ((finger_num == 0) || (finger_num > MAX_TOUCH_FINGER_COUNT))
        {
            return false;
        }

        uint8_t buffer[] =
            {
                0xF3,
                static_cast<uint8_t>(_touch_info_start_address >> 24),
                static_cast<uint8_t>(_touch_info_start_address >> 16),
                static_cast<uint8_t>(_touch_info_start_address >> 8),
                static_cast<uint8_t>(_touch_info_start_address),
                0x03,
            };

        const uint8_t touch_point_pointer = TOUCH_POINT_ADDRESS_OFFSET + finger_num * SINGLE_TOUCH_POINT_DATA_SIZE;
        uint8_t buffer_2[touch_point_pointer] = {0};

        if (_bus->write_read(buffer, 6, buffer_2, touch_point_pointer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        // 获取的手指个数不为finger_num
        if (buffer_2[0] != finger_num)
        {
            return false;
        }

        tp.finger_count = buffer_2[0];

        Touch_Num_Info tni;
        tni.x = (static_cast<uint16_t>(buffer_2[touch_point_pointer - SINGLE_TOUCH_POINT_DATA_SIZE]) << 8) |
                buffer_2[touch_point_pointer - SINGLE_TOUCH_POINT_DATA_SIZE + 1];
        tni.y = (static_cast<uint16_t>(buffer_2[touch_point_pointer - SINGLE_TOUCH_POINT_DATA_SIZE + 2]) << 8) |
                buffer_2[touch_point_pointer - SINGLE_TOUCH_POINT_DATA_SIZE + 3];
        tni.p = buffer_2[touch_point_pointer - SINGLE_TOUCH_POINT_DATA_SIZE + 4];

        tp.num_info.push_back(tni);

        if ((tni.x == 65535) && (tni.y == 65535) && (tni.p == 0))
        {
            tp.edge_touch_flag = true;
        }
        else
        {
            tp.edge_touch_flag = false;
        }

        return true;
    }

    bool Hi8561_Touch::get_multiple_touch_point(Touch_Point &tp)
    {
        uint8_t buffer[] =
            {
                0xF3,
                static_cast<uint8_t>(_touch_info_start_address >> 24),
                static_cast<uint8_t>(_touch_info_start_address >> 16),
                static_cast<uint8_t>(_touch_info_start_address >> 8),
                static_cast<uint8_t>(_touch_info_start_address),
                0x03,
            };

        uint8_t touch_point_pointer = MAX_TOUCH_DATA_SIZE;
        uint8_t buffer_2[touch_point_pointer] = {0};

        if (_bus->write_read(buffer, 6, buffer_2, touch_point_pointer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        // 如果手指数为0
        if (buffer_2[0] == 0)
        {
            return false;
        }
        tp.finger_count = buffer_2[0];

        for (uint8_t i = 1; i <= tp.finger_count; i++)
        {
            touch_point_pointer = TOUCH_POINT_ADDRESS_OFFSET + i * SINGLE_TOUCH_POINT_DATA_SIZE - SINGLE_TOUCH_POINT_DATA_SIZE;

            Touch_Num_Info tni;
            tni.x = (static_cast<uint16_t>(buffer_2[touch_point_pointer]) << 8) | buffer_2[touch_point_pointer + 1];
            tni.y = (static_cast<uint16_t>(buffer_2[touch_point_pointer + 2]) << 8) | buffer_2[touch_point_pointer + 3];
            tni.p = buffer_2[touch_point_pointer + 4];

            tp.num_info.push_back(tni);
        }

        if ((tp.num_info[tp.finger_count].x == 65535) && (tp.num_info[tp.finger_count].y == 65535) && (tp.num_info[tp.finger_count].p == 0))
        {
            tp.edge_touch_flag = true;
        }
        else
        {
            tp.edge_touch_flag = false;
        }

        return true;
    }

    bool Hi8561_Touch::get_edge_touch(void)
    {
        uint8_t buffer[] =
            {
                0xF3,
                static_cast<uint8_t>(_touch_info_start_address >> 24),
                static_cast<uint8_t>(_touch_info_start_address >> 16),
                static_cast<uint8_t>(_touch_info_start_address >> 8),
                static_cast<uint8_t>(_touch_info_start_address),
                0x03,
            };

        uint8_t touch_point_pointer = TOUCH_POINT_ADDRESS_OFFSET + MAX_TOUCH_FINGER_COUNT * SINGLE_TOUCH_POINT_DATA_SIZE;
        uint8_t buffer_2[touch_point_pointer] = {0};

        if (_bus->write_read(buffer, 6, buffer_2, touch_point_pointer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        // 如果手指数为0
        if (buffer_2[0] == 0)
        {
            return false;
        }

        touch_point_pointer = TOUCH_POINT_ADDRESS_OFFSET + buffer_2[0] * SINGLE_TOUCH_POINT_DATA_SIZE - SINGLE_TOUCH_POINT_DATA_SIZE;

        if ((static_cast<uint16_t>((static_cast<uint16_t>(buffer_2[touch_point_pointer]) << 8) | buffer_2[touch_point_pointer + 1]) != 65535) ||
            (static_cast<uint16_t>((static_cast<uint16_t>(buffer_2[touch_point_pointer + 2]) << 8) | buffer_2[touch_point_pointer + 3]) != 65535) ||
            (buffer_2[touch_point_pointer + 4] != 0))
        {
            return false;
        }

        return true;
    }

}
