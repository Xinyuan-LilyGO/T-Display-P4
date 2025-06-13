/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2025-02-24 13:48:06
 * @License: GPL 3.0
 */
#pragma once

#include "../bus/bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Iic_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Iic_Guide> _bus;

    private:
        int16_t _address;

    public:
        Iic_Guide(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address)
            : _bus(bus), _address(address)
        {
        }

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool iic_init_list(const uint8_t *list, size_t length);
    };

    class Iis_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Iis_Guide> _bus;

        int16_t _address;

        int32_t _rst;

    public:
        Iis_Guide(std::shared_ptr<Bus_Iis_Guide> bus, int16_t address)
            : _bus(bus), _address(address)
        {
        }

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };

    class Spi_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Spi_Guide> _bus;

        int32_t _cs;

    public:
        Spi_Guide(std::shared_ptr<Bus_Spi_Guide> bus, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _bus(bus), _cs(cs)
        {
        }

        virtual bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };

    class Uart_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Uart_Guide> _bus;

    public:
        Uart_Guide(std::shared_ptr<Bus_Uart_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(int32_t baud_rate = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };

}