/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-01-22 14:34:13
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

#define BQ27220YZFR_DEVICE_DEFAULT_ADDRESS 0x55

namespace Cpp_Bus_Driver
{
    class Bq27220xxxx : public IIC_Guide
    {
    private:
        int32_t _rst;

    protected:
        enum class Cmd
        {
            // 0x首地址
            RO_DEVICE_ID = 0x3C,

            RW_CONTROL_CONTROL_STATUS = 0x00,
            RW_AT_RATE = 0x02,
            RO_AT_RATE_TIME_TO_EMPTY = 0x04,
            RW_TEMPERATURE = 0x06,
            RO_VOLTAGE = 0x08,
            RO_BATTERY_STATUS = 0x0A,
            RO_CURRENT = 0x0C,
        };

        enum class Cmd_Control
        {
            RO_CONTROL_STATUS = 0x0000,
            RO_DEVICE_NUMBER,
            RO_FW_VERSION,
            RO_BOARD_OFFSET = 0x0009,
            RO_CC_OFFSET,
            RO_CC_OFFSET_SAVE,
            RO_OCV_CMD,
            RO_BAT_INSERT,
            RO_BAT_REMOVE,
            RO_SET_SNOOZE = 0x0013,
            RO_CLEAR_SNOOZE,
            RO_SET_PROFILE_1,
            RO_SET_PROFILE_2,
            RO_SET_PROFILE_3,
            RO_SET_PROFILE_4,
            RO_SET_PROFILE_5,
            RO_SET_PROFILE_6,
            WO_CAL_TOGGLE = 0x002D,
            WO_SEALED = 0x0030,
            WO_RESET = 0x0041,
            WO_EXIT_CAL = 0x0080,
            WO_ENTER_CAL,
            RO_ENTER_CFG_UPDATE = 0x0090,
            RO_EXIT_CFG_UPDATE_REINIT,
            RO_EXIT_CFG_UPDATE,
        };

        // static const uint8_t Init_List[];

    public:
        Bq27220xxxx(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : IIC_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint16_t iic_device_id(void);
    };
}