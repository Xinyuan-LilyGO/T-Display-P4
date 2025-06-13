/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2025-01-03 13:45:34
 * @License: GPL 3.0
 */
#pragma once

#include "../bus/bus_guide.h"

namespace Cpp_Bus_Driver
{
    class IIC_Guide : protected Tool
    {
    protected:
        enum class Init_List_Cmd
        {
            DELAY_MS = 0,
            WRITE_DATA,
            WRITE_C8_D8,
        };

        std::shared_ptr<Bus_IIC_Guide> _bus;

    private:
        int16_t _address;

    protected:
        int32_t _rst;

    public:
        IIC_Guide(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _bus(bus), _address(address), _rst(rst)
        {
        }

        virtual bool begin(int64_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool iic_init_list(const uint8_t *list, size_t length);
    };

    class IIS_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_IIS_Guide> _bus;

        int16_t _address;

        int32_t _rst;

    public:
        IIS_Guide(std::shared_ptr<Bus_IIS_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _bus(bus), _address(address), _rst(rst)
        {
        }

        virtual bool begin(int64_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };

}