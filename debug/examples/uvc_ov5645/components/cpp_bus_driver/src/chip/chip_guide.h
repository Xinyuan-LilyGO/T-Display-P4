/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2025-06-27 16:13:41
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

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool init_list(const uint8_t *list, size_t length);
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

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool init_list(const uint8_t *list, size_t length);
    };

    class Qspi_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Qspi_Guide> _bus;

        int32_t _cs;

    public:
        Qspi_Guide(std::shared_ptr<Bus_Qspi_Guide> bus, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _bus(bus), _cs(cs)
        {
        }

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool init_list(const uint32_t *list, size_t length);
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

    class Iis_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Iis_Guide> _bus;

    public:
        Iis_Guide(std::shared_ptr<Bus_Iis_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width);
    };

    class Sdio_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Sdio_Guide> _bus;

    public:
        Sdio_Guide(std::shared_ptr<Bus_Sdio_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };
}