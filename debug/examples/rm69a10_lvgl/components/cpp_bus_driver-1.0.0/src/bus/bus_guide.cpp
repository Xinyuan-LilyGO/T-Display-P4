/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2024-12-26 11:04:05
 * @License: GPL 3.0
 */

#include "bus_guide.h"

namespace Cpp_Bus_Driver
{
    bool Bus_IIC_Guide::read(uint8_t write_c8, uint8_t *read_d8)
    {
        if (write_read(&write_c8, 1, read_d8, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return true;
    }

    bool Bus_IIC_Guide::write(const uint8_t write_c8, const uint8_t write_d8)
    {
        const uint8_t buffer[2] = {write_c8, write_d8};
        if (write(buffer, 2) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_IIC_Guide::write(const uint8_t write_c8, const uint8_t *data, size_t length)
    {
        if (write(write_c8, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (write(data, length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_IIC_Guide::scan_7bit_address(std::vector<uint8_t> *address)
    {
        std::vector<uint8_t> address_buffer; // 地址存储器

        for (uint8_t i = 1; i < 128; i++)
        {
            if (probe(i) == true)
            {
                address_buffer.push_back(i);
            }
        }

        if (address_buffer.empty() == true)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "address_buffer is empty\n");
            return false;
        }

        address->assign(address_buffer.begin(), address_buffer.end());
        return true;
    }

    // {

    //     const uint8_t buffer[2] = {write_c8, write_d8};
    //     if (write(buffer, 2) == false)
    //     {
    //         assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return true;
    // }
}
