/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2025-02-08 16:59:18
 * @License: GPL 3.0
 */
#pragma once

#include "../bus/bus_guide.h"

namespace Cpp_Bus_Driver
{
    class IIC_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_IIC_Guide> _bus;

    private:
        int16_t _address;

    public:
        IIC_Guide(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address)
            : _bus(bus), _address(address)
        {
        }

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool iic_init_list(const uint8_t *list, size_t length);
    };

    class IIS_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_IIS_Guide> _bus;

        int16_t _address;

        int32_t _rst;

    public:
        IIS_Guide(std::shared_ptr<Bus_IIS_Guide> bus, int16_t address)
            : _bus(bus), _address(address)
        {
        }

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };

    class SPI_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_SPI_Guide> _bus;

        int32_t _cs;

    public:
        SPI_Guide(std::shared_ptr<Bus_SPI_Guide> bus, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _bus(bus), _cs(cs)
        {
        }

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };

}