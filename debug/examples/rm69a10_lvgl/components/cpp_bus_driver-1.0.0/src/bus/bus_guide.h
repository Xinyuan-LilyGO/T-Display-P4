/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2024-12-26 11:05:13
 * @License: GPL 3.0
 */

#pragma once

#include "../config.h"

namespace Cpp_Bus_Driver
{
    class Bus_IIC_Guide : protected Tool
    {
    public:
        Bus_IIC_Guide()
        {
        }

        virtual bool begin(int64_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE, int16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE) = 0;
        virtual bool read(uint8_t *data, size_t length) = 0;
        virtual bool write(const uint8_t *data, size_t length) = 0;
        virtual bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) = 0;
        virtual bool probe(const uint16_t address) = 0;

        bool read(uint8_t write_c8, uint8_t *read_d8);
        bool write(const uint8_t write_c8, const uint8_t write_d8);
        bool write(const uint8_t write_c8, const uint8_t *data, size_t length);

        bool scan_7bit_address(std::vector<uint8_t> *address);
    };

    class Bus_IIS_Guide : protected Tool
    {
    public:
        Bus_IIS_Guide()
        {
        }

        // virtual bool begin(int16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE, int64_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) = 0;
        // virtual bool read(uint8_t *data, size_t length) = 0;
        // virtual bool write(const uint8_t *data, size_t length) = 0;
        // virtual bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) = 0;

        // bool read(uint8_t write_c8, uint8_t *read_d8);
        // bool write(const uint8_t write_c8, const uint8_t write_d8);
    };
}