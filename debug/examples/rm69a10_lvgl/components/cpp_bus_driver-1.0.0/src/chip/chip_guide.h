/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2024-12-27 10:35:29
 * @License: GPL 3.0
 */
#pragma once

#include "../bus/bus_guide.h"

namespace Cpp_Bus_Driver
{
    class IIC_Guide : protected Tool
    {

    protected:
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
        virtual int16_t iic_device_id(void) = 0;

        // virtual int32_t IIC_Device_Reset(void);
        // // 写设备状态开关的虚函数
        // virtual bool IIC_Write_Device_State(uint32_t device, uint8_t state);
        // // 写设备值的虚函数
        // virtual bool IIC_Write_Device_Value(uint32_t device, int64_t value);
        // // 写连续数据的虚函数
        // virtual bool IIC_Write_Device_Data(uint32_t device, const uint8_t *data, size_t length);
        // // 读取状态信息虚函数
        // virtual String IIC_Read_Device_State(uint32_t information);
        // // 读取值信息虚函数
        // virtual double IIC_Read_Device_Value(uint32_t information);
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