/*
 * @Description: PCF85063.cpp
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2025-01-10 17:06:55
 * @License: GPL 3.0
 */
#include "BQ27220xxxx.h"

namespace Cpp_Bus_Driver
{
    // const uint8_t BQ27220xxxx::Init_List[] =
    //     {
    //         static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8),
    //         static_cast<uint8_t>(Reg::RW_CLKOUT_CONTROL),
    //         0B00000000,
    // };

    bool BQ27220xxxx::begin(int64_t frequency)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (IIC_Guide::begin(frequency) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        // if (iic_init_list(Init_List, sizeof(Init_List)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list fail\n");
        //     return false;
        // }

        return true;
    }

    int32_t BQ27220xxxx::iic_device_id(void)
    {
        uint16_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Reg_Std::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }
}
