/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-04 13:09:08
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
    class Co5300 : public Qspi_Guide
    {
    public:
        enum class Color_Format
        {
            RGB565 = 16,
            RGB666 = 24,
            RGB888 = 24,
        };

    private:
        enum class Cmd
        {
            // 用于读写寄存器命令
            WO_WRITE_REGISTER = 0x02,
            WO_READ_REGISTER = 0x03,

            // 用于写颜色流命令
            WO_WRITE_COLOR_STREAM_1LANES_CMD = 0x02,
            WO_WRITE_COLOR_STREAM_4LANES_CMD_1 = 0x32,
            WO_WRITE_COLOR_STREAM_4LANES_CMD_2 = 0x12,

        };

        enum class Reg
        {
            // 用于写颜色流命令
            // 从指定的像素位置开始写入图像数据，该位置由之前的 CASET (2Ah)（列地址设置）和 RASET (2Bh)（行地址设置）命令定义
            WO_MEMORY_START_WRITE = 0x002C00,
            // 从上一次写入结束的位置继续写入数据，无需重新指定地址
            WO_MEMORY_CONTINUOUS_WRITE = 0x003C00,

            WO_COLUMN_ADDRESS_SET = 0x002A00,
            WO_PAGE_ADDRESS_SET = 0x002B00,
            WO_MEMORY_WRITE_START = 0x002C00,
        };

        enum class Write_Stream_Mode
        {
            WRITE_1LANES,
            WRITE_4LANES,
            CONTINUOUS_WRITE_1LANES,
            CONTINUOUS_WRITE_4LANES,
        };

        static constexpr uint32_t _init_list[] =
            {
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x001100,

                static_cast<uint8_t>(Init_List_Cmd::DELAY_MS), 120,

                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x00FE00, 0x00, // page switch
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x00C400, 0x80, // spi mode
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x003600, 0x00, // RGB
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x003600, 0x08, // BGR
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x003A00, 0x55, // interface pixel format 16bit/pixel
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x003A00, 0x66, // interface pixel format 18bit/pixel
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x003A00, 0x77, // interface pixel format 24bit/pixel
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x005300, 0x20, // write control display1
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x006300, 0xFF, // write display brightness value in hbm mode

                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x005100, 0x00, // brightness adjustment
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x005800, 0x00, // sunlight readability enhancement off
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x005800, 0x50, // sunlight readability enhancement low
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x005800, 0x60, // sunlight readability enhancement medium
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x005800, 0x70, // sunlight readability enhancement high
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x002900, // display on
                static_cast<uint8_t>(Init_List_Cmd::DELAY_MS), 10,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R24_D8), static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), 0x005100, 0xFF, // brightness adjustment
            };

        int32_t _rst;
        int16_t _width, _height;
        int16_t _x_offset, _y_offset;
        Color_Format _color_format;

    public:
        Co5300(std::shared_ptr<Bus_Qspi_Guide> bus, int16_t width, int16_t height, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE,
               int16_t x_offset = 0, int16_t y_offset = 0, Color_Format color_format = Color_Format::RGB565)
            : Qspi_Guide(bus, cs), _rst(rst), _width(width), _height(height), _x_offset(x_offset), _y_offset(y_offset), _color_format(color_format)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        /**
         * @brief 设置需要渲染的窗口
         * @param x_start x坐标开始点
         * @param x_end x坐标结束点
         * @param y_start y坐标开始点
         * @param y_end x坐标结束点
         * @return
         * @Date 2025-06-30 11:10:12
         */
        bool set_render_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);

        bool set_write_stream_mode(Write_Stream_Mode mode);

        /**
         * @brief 发送颜色流
         * @param x 绘制点x坐标
         * @param y 绘制点y坐标
         * @param w 绘制长度
         * @param h 绘制高度
         * @param *data 颜色数据
         * @return
         * @Date 2025-07-03 10:27:58
         */
        bool send_color_stream(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);
    };
};