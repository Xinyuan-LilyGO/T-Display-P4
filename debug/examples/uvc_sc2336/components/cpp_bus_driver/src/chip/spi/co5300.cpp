/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-07-04 13:13:38
 * @License: GPL 3.0
 */
#include "co5300.h"

namespace Cpp_Bus_Driver
{
    bool Co5300::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            pin_write(_rst, 1);
            delay_ms(10);
            pin_write(_rst, 0);
            delay_ms(10);
            pin_write(_rst, 1);
            delay_ms(10);
        }

        if (Qspi_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        if (init_list(_init_list, sizeof(_init_list) / sizeof(uint32_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::set_render_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end)
    {
        x_start += _x_offset;
        x_end += _x_offset;
        y_start += _y_offset;
        y_end += _y_offset;

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_COLUMN_ADDRESS_SET) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_COLUMN_ADDRESS_SET) >> 8),
                static_cast<uint8_t>(Reg::WO_COLUMN_ADDRESS_SET),

                static_cast<uint8_t>(x_start >> 8),
                static_cast<uint8_t>(x_start),
                static_cast<uint8_t>(x_end >> 8),
                static_cast<uint8_t>(x_end),

                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_PAGE_ADDRESS_SET) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_PAGE_ADDRESS_SET) >> 8),
                static_cast<uint8_t>(Reg::WO_PAGE_ADDRESS_SET),

                static_cast<uint8_t>(y_start >> 8),
                static_cast<uint8_t>(y_start),
                static_cast<uint8_t>(y_end >> 8),
                static_cast<uint8_t>(y_end),

                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_WRITE_START) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_WRITE_START) >> 8),
                static_cast<uint8_t>(Reg::WO_MEMORY_WRITE_START),
            };

        if (_bus->write(buffer, 20) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::send_color_stream(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
    {
        // 有效性检查
        if (w == 0 || h == 0 || data == nullptr || x >= _width || y >= _height || (_width - x) < w || (_height - y) < h)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid parameters: x=%d, y=%d, w=%d, h=%d, data=%p", x, y, w, h, data);
            return false;
        }

        // 硬件通常期望的是 [x_start, x_end] 和 [y_start, y_end] 的闭区间，即 x_end 和 y_end 是最后一个像素的坐标
        // 例如：
        // 如果 x=10, w=5，那么像素列是 10, 11, 12, 13, 14，所以 x_end 应该是 14（即 x + w - 1）
        // 如果不 -1，x_end 会是 15，可能超出实际范围或导致多写一个像素
        if (set_render_window(x, x + w - 1, y, y + h - 1) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
            return false;
        }

        if (set_write_stream_mode(Write_Stream_Mode::CONTINUOUS_WRITE_4LANES) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_write_stream_mode fail\n");
            return false;
        }

        if (_bus->write(data, w * h * (static_cast<uint8_t>(_color_format) / 8), SPI_TRANS_MODE_QIO) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_stream fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::set_write_stream_mode(Write_Stream_Mode mode)
    {
        uint8_t buffer[4] = {0};

        switch (mode)
        {
        case Write_Stream_Mode::WRITE_1LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_1LANES_CMD);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_START_WRITE);
            break;
        case Write_Stream_Mode::WRITE_4LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_START_WRITE);
            break;
        case Write_Stream_Mode::CONTINUOUS_WRITE_1LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_1LANES_CMD);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE);
            break;
        case Write_Stream_Mode::CONTINUOUS_WRITE_4LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE);
            break;

        default:
            break;
        }

        if (_bus->write(buffer, 4, static_cast<uint32_t>(NULL), true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
