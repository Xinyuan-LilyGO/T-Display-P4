/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2025-06-27 15:58:18
 * @License: GPL 3.0
 */
#include "bus_guide.h"

namespace Cpp_Bus_Driver
{
    i2c_cmd_handle_t Bus_Iic_Guide::cmd_link_create(void)
    {
        assert_log(Log_Level::BUS, __FILE__, __LINE__, "cmd_link_create fail\n");
        return nullptr;
    }

    bool Bus_Iic_Guide::start_transmit(i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en)
    {
        assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
        return false;
    }

    bool Bus_Iic_Guide::write(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en)
    {
        assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
        return false;
    }

    bool Bus_Iic_Guide::write(i2c_cmd_handle_t cmd_handle, const uint8_t *data, size_t data_len, bool ack_en)
    {
        assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
        return false;
    }

    bool Bus_Iic_Guide::read(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, i2c_ack_type_t ack)
    {
        assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
        return false;
    }

    bool Bus_Iic_Guide::stop_transmit(i2c_cmd_handle_t cmd_handle)
    {
        assert_log(Log_Level::BUS, __FILE__, __LINE__, "stop_transmit fail\n");
        return false;
    }

    bool Bus_Iic_Guide::read(const uint8_t write_c8, uint8_t *read_data, size_t read_data_length)
    {
        if (write_read(&write_c8, 1, read_data, read_data_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return true;
    }

    bool Bus_Iic_Guide::write(const uint8_t write_c8, const uint8_t write_d8)
    {
        const uint8_t buffer[] = {write_c8, write_d8};
        if (write(buffer, 2) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_Iic_Guide::write(const uint8_t write_c8, const uint16_t write_d16, Endian endian)
    {
        uint8_t buffer[3] = {write_c8};

        switch (endian)
        {
        case Endian::BIG:
            buffer[1] = write_d16 >> 8;
            buffer[2] = write_d16;
            break;
        case Endian::LITTLE:
            buffer[1] = write_d16;
            buffer[2] = write_d16 >> 8;
            break;

        default:
            break;
        }

        if (write(buffer, 3) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    // bool Bus_Iic_Guide::write(const uint16_t write_c16, Endian endian)
    // {
    //     uint8_t buffer[2] = {0};

    //     switch (endian)
    //     {
    //     case Endian::BIG:
    //         buffer[0] = write_c16 >> 8;
    //         buffer[1] = write_c16;
    //         break;
    //     case Endian::LITTLE:
    //         buffer[0] = write_c16;
    //         buffer[1] = write_c16 >> 8;
    //         break;

    //     default:
    //         break;
    //     }

    //     if (write(buffer, 2) == false)
    //     {
    //         assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    bool Bus_Iic_Guide::write(const uint8_t write_c8, const uint8_t *write_data, size_t write_data_length)
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

    bool Bus_Iic_Guide::scan_7bit_address(std::vector<uint8_t> *address)
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

    bool Bus_Spi_Guide::read(const uint8_t write_c8, uint8_t *read_d8)
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

    bool Bus_Spi_Guide::read(const uint8_t write_c8, uint8_t *read_data, size_t read_data_length)
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

    bool Bus_Spi_Guide::write(const uint8_t write_c8)
    {
        if (write(&write_c8, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_Spi_Guide::write(const uint8_t write_c8, const uint8_t write_d8)
    {
        const uint8_t buffer[] = {write_c8, write_d8};

        if (write(buffer, 2) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_Spi_Guide::write(const uint8_t write_c8, const uint8_t *write_data, size_t write_data_length)
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

    bool Bus_Spi_Guide::read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data, size_t read_data_length)
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

    bool Bus_Spi_Guide::read(const uint8_t write_c8_1, const uint8_t write_c8_2, uint8_t *read_data, size_t read_data_length)
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

    bool Bus_Spi_Guide::read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data)
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

    bool Bus_Spi_Guide::write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t *write_data, size_t write_data_length)
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

    bool Bus_Spi_Guide::write(const uint8_t write_c8_1, const uint8_t write_c8_2, const uint8_t *write_data, size_t write_data_length)
    {
        uint8_t buffer[2 + write_data_length] =
            {
                write_c8_1,
                write_c8_2,
            };

        memcpy(&buffer[2], write_data, write_data_length);

        if (write(buffer, 2 + write_data_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Bus_Spi_Guide::write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t write_data)
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

    // bool Bus_Qspi_Guide::write(const uint16_t cmd, const uint64_t addr, const uint8_t write_d8)
    // {
    //     if (write(cmd, addr, &write_d8, 1) == false)
    //     {
    //         assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

}
