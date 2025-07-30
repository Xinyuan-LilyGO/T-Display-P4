/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-06-24 15:47:48
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define FT3168_DEVICE_DEFAULT_ADDRESS 0x38
#define FT3268_DEVICE_DEFAULT_ADDRESS 0x38

    class Ft3x68 : public Iic_Guide
    {
    private:
        static constexpr uint8_t FT3168_DEVICE_ID = 0x03;

        static constexpr uint8_t MAX_TOUCH_FINGER_COUNT = 2;
        static constexpr uint8_t SINGLE_TOUCH_POINT_DATA_SIZE = 6;

        enum class Cmd
        {
            RO_DEVICE_ID = 0xA0, // 0x00:FT6456 0x04:FT3268 0x01:FT3067 0x05:FT3368 0x02:FT3068 0x03:FT3168

            RO_TD_STATUS = 0x02, // 触摸手指数
            RO_P1_XH,            // 第1点的X坐标高4位
        };

        int32_t _rst;

    public:
        struct Touch_Info
        {
            uint16_t x = -1; // x 坐标
            uint16_t y = -1; // y 坐标
        };

        struct Touch_Point
        {
            uint8_t finger_count = -1; // 触摸手指总数

            std::vector<struct Touch_Info> info;
        };

        Ft3x68(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 获取触摸总数
         * @return
         * @Date 2025-06-24 15:01:52
         */
        uint8_t get_finger_count(void);

        /**
         * @brief 获取单指触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @param finger_num 要获取的触摸点
         * @return [true]：获取的触摸点和finger_num相同 [false]：获取错误或者获取的触摸点和finger_num不相同
         * @return
         * @Date 2025-06-24 15:47:40
         */
        bool get_single_touch_point(Touch_Point &tp, uint8_t finger_num = 1);

        /**
         * @brief 获取多个触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
         * @Date 2025-06-24 15:12:00
         */
        bool get_multiple_touch_point(Touch_Point &tp);
    };
}