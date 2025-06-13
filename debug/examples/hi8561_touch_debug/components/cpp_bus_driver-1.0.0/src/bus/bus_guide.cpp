/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2025-02-07 14:54:00
 * @License: GPL 3.0
 */

#include "bus_guide.h"

namespace Cpp_Bus_Driver
{
    bool Bus_IIC_Guide::read(const uint8_t write_c8, uint8_t *read_data)
    {
        if (write_read(&write_c8, 1, read_data, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return true;
    }

    bool Bus_IIC_Guide::read(const uint8_t write_c8, uint16_t *read_data)
    {
        uint8_t buffer[2] = {0};

        if (write(write_c8, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (read(buffer, 2) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        *read_data = (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];

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

    bool Bus_SPI_Guide::read(const uint8_t write_c8, uint8_t *read_d8)
    {
        const uint8_t buffer_write[2] = {write_c8};
        uint8_t buffer_read[2] = {0};

        if (write_read(buffer_write, buffer_read, 2) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        *read_d8 = buffer_read[1];

        return true;
    }

    bool Bus_SPI_Guide::read(const uint8_t write_c8, uint8_t *read_data, size_t read_data_length)
    {
        const uint8_t buffer_write[1 + read_data_length] = {write_c8};
        uint8_t buffer_read[1 + read_data_length] = {0};

        if (write_read(buffer_write, buffer_read, 1 + read_data_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        memcpy(read_data, &buffer_read[1], read_data_length);

        return true;
    }

    bool Bus_SPI_Guide::write(const uint8_t write_c8, uint8_t write_d8)
    {
        const uint8_t buffer[2] = {write_c8, write_d8};

        if (write(buffer, 2) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_SPI_Guide::write(const uint8_t write_c8, const uint8_t *write_data, size_t write_data_length)
    {
        uint8_t buffer[1 + write_data_length] = {write_c8};

        memcpy(&buffer[1], write_data, write_data_length);

        if (write(buffer, 1 + write_data_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_SPI_Guide::read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data, size_t read_data_length)
    {
        const uint8_t buffer_write[3 + read_data_length] =
            {
                write_c8,
                static_cast<uint8_t>(write_c16 >> 8),
                static_cast<uint8_t>(write_c16),
            };

        uint8_t buffer_read[3 + read_data_length] = {0};

        if (write_read(buffer_write, buffer_read, 3 + read_data_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        memcpy(read_data, &buffer_read[3], read_data_length);

        return true;
    }

    bool Bus_SPI_Guide::read(const uint8_t write_c8_1, const uint8_t write_c8_2, uint8_t *read_data, size_t read_data_length)
    {
        const uint8_t buffer_write[2 + read_data_length] =
            {
                write_c8_1,
                write_c8_2,
            };

        uint8_t buffer_read[2 + read_data_length] = {0};

        if (write_read(buffer_write, buffer_read, 2 + read_data_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        memcpy(read_data, &buffer_read[2], read_data_length);

        return true;
    }

    bool Bus_SPI_Guide::read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data)
    {
        const uint8_t buffer_write[4] =
            {
                write_c8,
                static_cast<uint8_t>(write_c16 >> 8),
                static_cast<uint8_t>(write_c16),
            };

        uint8_t buffer_read[4] = {0};

        if (write_read(buffer_write, buffer_read, 4) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        memcpy(read_data, &buffer_read[3], 1);

        return true;
    }

    bool Bus_SPI_Guide::write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t *write_data, size_t write_data_length)
    {
        uint8_t buffer[3 + write_data_length] =
            {
                write_c8,
                static_cast<uint8_t>(write_c16 >> 8),
                static_cast<uint8_t>(write_c16),
            };

        memcpy(&buffer[3], write_data, write_data_length);

        if (write(buffer, 3 + write_data_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_SPI_Guide::write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t write_data)
    {
        uint8_t buffer[4] =
            {
                write_c8,
                static_cast<uint8_t>(write_c16 >> 8),
                static_cast<uint8_t>(write_c16),
            };

        memcpy(&buffer[3], &write_data, 1);

        if (write(buffer, 4) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
