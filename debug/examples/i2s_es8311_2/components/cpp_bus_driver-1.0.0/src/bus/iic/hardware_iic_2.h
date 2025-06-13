/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-02-24 13:34:30
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Iic_2 : public Bus_Iic_Guide
    {
    private:
        int32_t _sda, _scl;
        int8_t _port;
        int32_t _frequency;
        int16_t _address;

    public:
        Hardware_Iic_2(int32_t sda, int32_t scl, int8_t port = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _sda(sda), _scl(scl), _port(port)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE, int16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool read(uint8_t *data, size_t length) override;
        bool write(const uint8_t *data, size_t length) override;
        bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) override;

        bool probe(const uint16_t address) override;

        // bool RequestFrom(uint8_t address, size_t length) override;
    };
}
