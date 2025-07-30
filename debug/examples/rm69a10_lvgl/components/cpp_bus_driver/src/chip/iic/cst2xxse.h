/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-07 11:50:08
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define CST2xxSE_DEVICE_DEFAULT_ADDRESS 0x5A

    class Cst2xxse : public Iic_Guide
    {
    private:
        static constexpr uint8_t CST2xxSE_DEVICE_ID = 0xAB;
        static constexpr uint8_t MAX_TOUCH_FINGER_COUNT = 6;
        static constexpr uint8_t SINGLE_TOUCH_POINT_DATA_SIZE = 5;

        enum class Cmd
        {
            RO_DEVICE_ID = 0x06, // 读取后返回0xAB
            RO_TOUCH_POINT_INFO_START = 0x00,
            RO_GET_FINGER_COUNT = 0x05,
        };

    private:
        int32_t _rst;

    public:
        struct Touch_Info
        {
            uint16_t x = -1;             // x 坐标
            uint16_t y = -1;             // y 坐标
            uint8_t pressure_value = -1; // 触摸压力值
        };

        struct Touch_Point
        {
            uint8_t finger_count = -1;    // 触摸手指总数
            bool home_touch_flag = false; // home按键触摸标志

            std::vector<struct Touch_Info> info;
        };

        Cst2xxse(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 获取触摸总数
         * @return
         * @Date 2025-04-23 11:42:45
         */
        uint8_t get_finger_count(void);

        /**
         * @brief 获取单指触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @param finger_num 要获取的触摸点
         * @return [true]：获取的触摸点和finger_num相同 [false]：获取错误或者获取的触摸点和finger_num不相同
         * @Date 2025-04-23 11:53:37
         */
        bool get_single_touch_point(Touch_Point &tp, uint8_t finger_num = 1);

        /**
         * @brief 获取多个触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
         * @Date 2025-04-23 15:05:49
         */
        bool get_multiple_touch_point(Touch_Point &tp);

        /**
         * @brief 获取home按键检测
         * @return  [true]：屏幕home按键检测触发 [false]：屏幕home按键检测未触发
         * @Date 2025-04-23 15:33:06
         */
        bool get_home_touch(void);
    };
}