/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-05-15 17:32:28
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define XL9535_DEVICE_DEFAULT_ADDRESS 0x20

    class Xl95x5 : public Iic_Guide
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

            IO_PORT0,
            IO_PORT1,
        };

        enum class Mode
        {
            OUTPUT,
            INPUT,
        };

        enum class Value
        {
            LOW = 0,
            HIGH,
        };

        Xl95x5(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 设置引脚模式
         * @param pin 使用Pin::配置，引脚号
         * @param mode 使用Mode::配置，模式
         * @return
         * @Date 2025-03-11 10:44:12
         */
        bool pin_mode(Pin pin, Mode mode);

        /**
         * @brief 引脚写数据
         * @param pin 使用Pin::配置，引脚号
         * @param value [0]：低电平，[1]：高电平
         * @return
         * @Date 2025-03-11 10:44:53
         */
        bool pin_write(Pin pin, Value value);

        /**
         * @brief 引脚读数据
         * @param pin 使用Pin::配置，引脚号
         * @return [0]：低电平，[1]：高电平
         * @Date 2025-03-11 10:46:16
         */
        bool pin_read(Pin pin);

        /**
         * @brief 清除中断请求
         * @return
         * @Date 2025-03-11 10:48:11
         */
        bool clear_irq_flag(void);
    };
}