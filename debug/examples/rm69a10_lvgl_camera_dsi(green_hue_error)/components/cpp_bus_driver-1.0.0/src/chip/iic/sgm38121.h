/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2024-12-27 10:14:42
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

#define SGM38121_DEVICE_DEFAULT_ADDRESS 0x28

namespace Cpp_Bus_Driver
{
    static const uint8_t SGM38121_Initialization_Operation[] = {
        // BO_BEGIN_TRANSMISSION,

        // BO_END_TRANSMISSION,
        // BO_DELAY, 100
    };

    class SGM38121 : public IIC_Guide
    {
    private:
        enum class Reg
        {
            RO_DEVICE_ID = 0x00,

            RW_DISCHARGE_RESISTOR_SELECTION = 0X02,
            RW_DVDD1_OUTPUT_VOLTAGE_LEVEL,
            RW_DVDD2_OUTPUT_VOLTAGE_LEVEL,
            RW_AVDD1_OUTPUT_VOLTAGE_LEVEL,
            RW_AVDD2_OUTPUT_VOLTAGE_LEVEL,
            RW_FUNCTION,
            RW_POWER_SEQUENCE_SETTING_1 = 0X0A,
            RW_POWER_SEQUENCE_SETTING_2,
            RW_ENABLE_CONTROL = 0X0E,
            RW_SEQUENCE_CONTROL,
        };

    public:
        enum class Channel
        {
            DVDD_1 = 0,
            DVDD_2,
            AVDD_1,
            AVDD_2,
        };

        enum class State
        {
            ON = 0,
            OFF,
        };

        SGM38121(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : IIC_Guide(bus, address, rst)
        {
        }

        bool begin(int64_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        int16_t iic_device_id(void) override;

        bool set_output_voltage(Channel channel, uint16_t voltage);
        bool set_channel_state(Channel channel, State state);
    };
}