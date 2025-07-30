/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-06-16 17:53:00
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
        static constexpr uint32_t MEMORY_ADDRESS_ERAM = 0x20011000;
        static constexpr uint8_t MAX_DSRAM_NUM = 25;

        static constexpr uint32_t DSRAM_SECTION_INFO_START_ADDRESS = MEMORY_ADDRESS_ERAM + 4;
        // 乘8bytes 是因为一共有两个数据，uint32_t数据（uint32_t地址（4 bytes）和uint32_t长度（4 bytes））
        static constexpr uint32_t ESRAM_NUM_START_ADDRESS = DSRAM_SECTION_INFO_START_ADDRESS + MAX_DSRAM_NUM * 8;
        static constexpr uint32_t ESRAM_SECTION_INFO_START_ADDRESS = ESRAM_NUM_START_ADDRESS + 4;
        static constexpr uint16_t MEMORY_ERAM_SIZE = 4 * 1024;

        static constexpr uint8_t MAX_TOUCH_FINGER_COUNT = 10;

        static constexpr uint8_t MAX_TOUCH_DATA_SIZE = 78;

        static constexpr uint8_t TOUCH_POINT_ADDRESS_OFFSET = 3;
        static constexpr uint8_t SINGLE_TOUCH_POINT_DATA_SIZE = 5;

        // enum class Cmd
        // {
        //     // 0x首地址
        //     RO_DEVICE_ID = 0x3C,
        // };

        // static constexpr uint8_t Init_List[];

        int32_t _rst;
        uint32_t _touch_info_start_address;

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
            bool edge_touch_flag = false; // 边缘触摸标志

            std::vector<struct Touch_Info> info;
        };

        Hi8561_Touch(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

    private:
        bool init_address_info(void);

    public:
        /**
         * @brief 获取触摸总数
         * @return
         * @Date 2025-03-28 09:51:25
         */
        uint8_t get_finger_count(void);

        /**
         * @brief 获取单指触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @param finger_num 要获取的触摸点
         * @return [true]：获取的触摸点和finger_num相同 [false]：获取错误或者获取的触摸点和finger_num不相同
         * @Date 2025-03-28 09:49:03
         */
        bool get_single_touch_point(Touch_Point &tp, uint8_t finger_num = 1);

        /**
         * @brief 获取多个触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
         * @Date 2025-03-28 09:52:56
         */
        bool get_multiple_touch_point(Touch_Point &tp);

        /**
         * @brief 获取边缘检测
         * @return  [true]：屏幕边缘检测触发 [false]：屏幕边缘检测未触发
         * @Date 2025-03-28 09:56:59
         */
        bool get_edge_touch(void);
    };
}