/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-01-16 15:54:44
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Spi : public Bus_Spi_Guide
    {
    private:
        int32_t _mosi, _sclk, _miso, _cs, _frequency;
        int8_t _port;
        uint8_t _mode;

        spi_device_handle_t _spi_device;

    public:
        Hardware_Spi(int32_t mosi, int32_t sclk, int32_t miso = DEFAULT_CPP_BUS_DRIVER_VALUE,
                     int8_t port = DEFAULT_CPP_BUS_DRIVER_VALUE, int8_t mode = 0)
            : _mosi(mosi), _sclk(sclk), _miso(miso), _port(port), _mode(mode)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool write(const void *data, size_t byte) override;
        bool read(void *data, size_t byte) override;
        bool write_read(const void *write_data, void *read_data, size_t data_byte) override;
    };
}
