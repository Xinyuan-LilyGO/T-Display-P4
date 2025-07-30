/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-02-10 12:06:00
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

#define HI8561_TOUCH_DEVICE_DEFAULT_ADDRESS 0x68

namespace Cpp_Bus_Driver
{
    class Hi8561_Touch : public IIC_Guide
    {
    private:
#define MEMORY_ADDRESS_ERAM 0x20011000
#define MAX_DSRAM_NUM 25

#define DSRAM_SECTION_INFO_START_ADDRESS (MEMORY_ADDRESS_ERAM + 4)
// 乘8bytes 是因为一共有两个数据，uint32_t数据（uint32_t地址（4 bytes）和uint32_t长度（4 bytes））
#define ESRAM_NUM_START_ADDRESS (DSRAM_SECTION_INFO_START_ADDRESS + MAX_DSRAM_NUM * 8)
#define ESRAM_SECTION_INFO_START_ADDRESS (ESRAM_NUM_START_ADDRESS + 4)
#define MEMORY_ERAM_SIZE (4 * 1024)

#define MAX_TOUCH_FINGER_COUNT 10

#define MAX_TOUCH_DATA_SIZE 78

#define TOUCH_POINT_ADDRESS_OFFSET 3
#define SINGLE_TOUCH_POINT_DATA_SIZE 5

        int32_t _rst;
        uint32_t _touch_info_start_address;

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

        // static const uint8_t Init_List[];

    private:
        bool init_address_info(void);

    public:
        struct Touch_Point
        {
            uint8_t finger_count; // 触摸手指总数
            uint16_t x;           // x 坐标
            uint16_t y;           // y 坐标
            uint8_t p;            // 触摸压力值
            bool edge_touch_flag;   // 边缘触摸标志
        };

        Hi8561_Touch(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : IIC_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t iic_device_id(void);

        uint8_t get_finger_count(void);

        uint8_t get_single_touch_point(Touch_Point &tp, uint8_t finger_num);

        uint8_t get_single_touch_point(Touch_Point &tp);

        bool get_edge_touch(void);
    };
}