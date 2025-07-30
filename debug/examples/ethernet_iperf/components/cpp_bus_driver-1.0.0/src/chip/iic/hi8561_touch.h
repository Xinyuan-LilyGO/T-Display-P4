/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-03-12 15:50:57
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define HI8561_TOUCH_DEVICE_DEFAULT_ADDRESS 0x68

    class Hi8561_Touch : public Iic_Guide
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

        // enum class Cmd
        // {
        //     // 0x首地址
        //     RO_DEVICE_ID = 0x3C,
        // };

        // static const uint8_t Init_List[];

        int32_t _rst;
        uint32_t _touch_info_start_address;

        bool init_address_info(void);

    public:
        struct Touch_Point
        {
            uint8_t finger_count = -1; // 触摸手指总数
            uint16_t x = -1;           // x 坐标
            uint16_t y = -1;           // y 坐标
            uint8_t p = -1;            // 触摸压力值
            bool edge_touch_flag = 0;  // 边缘触摸标志
        };

        Hi8561_Touch(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        // uint8_t device_id(void);

        uint8_t get_finger_count(void);

        uint8_t get_single_touch_point(Touch_Point &tp, uint8_t finger_num);

        uint8_t get_single_touch_point(Touch_Point &tp);

        bool get_edge_touch(void);
    };
}