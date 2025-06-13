/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-01-22 14:35:58
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

#define XL9535_DEVICE_DEFAULT_ADDRESS 0x20

namespace Cpp_Bus_Driver
{
    class Xl95x5 : public IIC_Guide
    {
    private:
        enum class Cmd
        {
            RO_DEVICE_ID = 0x04,

            RO_INPUT_PORT_0 = 0x00,
            RO_INPUT_PORT_1,
            RW_OUTPUT_PORT_0,
            RW_OUTPUT_PORT_1,
            RW_POLARITY_INVERSION_PORT_0,
            RW_POLARITY_INVERSION_PORT_1,
            RW_CONFIGURATION_PORT_0,
            RW_CONFIGURATION_PORT_1,
        };

        int32_t _rst;

    public:
        enum class Pin
        {
            IO_PORT0 = 0,
            IO_PORT1,

            IO0 = 0,
            IO1,
            IO2,
            IO3,
            IO4,
            IO5,
            IO6,
            IO7,
            IO10 = 10,
            IO11,
            IO12,
            IO13,
            IO14,
            IO15,
            IO16,
            IO17,
        };

        enum class Mode
        {
            OUTPUT,
            INPUT,
        };

        enum class Value
        {
            HIGH,
            LOW,
        };

        Xl95x5(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : IIC_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t iic_device_id(void);
        bool pin_mode(Pin pin, Mode mode);
        bool digital_write(Pin pin, Value value);
        uint8_t digital_read(Pin pin);
    };
}