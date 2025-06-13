/*
 * @Description: 
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-01-22 14:34:08
 * @License: GPL 3.0
 */
#include "Bq27220xxxx.h"

namespace Cpp_Bus_Driver
{
    // const uint8_t Bq27220xxxx::Init_List[] =
    //     {
    //         static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8),
    //         static_cast<uint8_t>(Cmd::RW_CLKOUT_CONTROL),
    //         0B00000000,
    // };

    bool Bq27220xxxx::begin(int32_t frequency)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (IIC_Guide::begin(frequency) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        int32_t buffer = iic_device_id();
        if (buffer == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get bq27220xxxx id fail (error id: %d)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get bq27220xxxx id: %ld\n", buffer);
        }

        // if (iic_init_list(Init_List, sizeof(Init_List)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list fail\n");
        //     return false;
        // }

        return true;
    }

    uint16_t Bq27220xxxx::iic_device_id(void)
    {
        uint16_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }
}
