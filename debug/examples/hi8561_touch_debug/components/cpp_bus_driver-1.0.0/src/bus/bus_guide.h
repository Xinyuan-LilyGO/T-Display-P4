/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2025-02-07 14:52:41
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

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE, int16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE) = 0;
        virtual bool read(uint8_t *data, size_t length) = 0;
        virtual bool write(const uint8_t *data, size_t length) = 0;
        virtual bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) = 0;
        virtual bool probe(const uint16_t address) = 0;

        bool read(const uint8_t write_c8, uint8_t *read_data);
        bool read(const uint8_t write_c8, uint16_t *read_data);
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

        // virtual bool begin(int16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) = 0;
        // virtual bool read(uint8_t *data, size_t length) = 0;
        // virtual bool write(const uint8_t *data, size_t length) = 0;
        // virtual bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) = 0;

        // bool read(uint8_t write_c8, uint8_t *read_d8);
        // bool write(const uint8_t write_c8, const uint8_t write_d8);
    };

    class Bus_SPI_Guide : protected Tool
    {
    public:
        Bus_SPI_Guide()
        {
        }

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE) = 0;
        virtual bool write(const void *data, size_t byte) = 0;
        virtual bool read(void *data, size_t byte) = 0;
        virtual bool write_read(const void *write_data, void *read_data, size_t data_byte) = 0;

        bool read(const uint8_t write_c8, uint8_t *read_d8);
        bool read(const uint8_t write_c8, uint8_t *read_data, size_t read_data_length);
        bool write(const uint8_t write_c8, uint8_t write_d8);
        bool write(const uint8_t write_c8, const uint8_t *write_data, size_t write_data_length);

        /**
         * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) | 0xAA(等待码，有可能有有可能没有，一般为 0xAA) | read_data(8 bit)(要读出的数据) | read_data(8 bit)(要读出的数据) | ......]
         * @param write_c8 一般为命令位
         * @param write_c16 一般为寄存器地址位
         * @param *read_data 要读出数据的指针
         * @param read_data_length 要读出的数据长度
         * @return
         * @Date 2025-01-17 13:53:33
         */
        bool read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data, size_t read_data_length);

        bool read(const uint8_t write_c8_1, const uint8_t write_c8_2, uint8_t *read_data, size_t read_data_length);
        
        bool read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data);

        /**
         * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) | write_data(8 bit)(要写入的数据) | write_data(8 bit)(要写入的数据) | ......]
         * @param write_c8 一般为命令位
         * @param write_c16 一般为寄存器地址位
         * @param *write_data 要写入数据的指针
         * @param write_data_length 要写入的数据长度
         * @return
         * @Date 2025-01-17 13:48:09
         */
        bool write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t *write_data, size_t write_data_length);

        bool write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t write_data);
    };
}