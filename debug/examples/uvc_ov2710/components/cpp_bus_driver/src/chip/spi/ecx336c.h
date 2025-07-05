/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-06-20 13:45:50
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
    class Ecx336c : public Spi_Guide
    {
    private:
        enum class Cmd
        {

        };

        static constexpr uint8_t _init_list[] =
            {
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x00, 0x0E,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x01, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x02, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x03, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x04, 0x3F,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x05, 0xC0,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x06, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x07, 0x40,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x08, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x09, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x0A, 0x10,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x0B, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x0C, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x0D, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x0E, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x0F, 0x56,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x10, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x11, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x12, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x13, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x14, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x15, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x16, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x17, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x18, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x19, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x1A, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x1B, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x1C, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x1D, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x1E, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x1F, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x20, 0x01,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x21, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x22, 0x40,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x23, 0x40,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x24, 0x40,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x25, 0x80,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x26, 0x40,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x27, 0x40,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x28, 0x40,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x29, 0x0B,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x2A, 0xBE,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x2B, 0x3C,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x2C, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x2D, 0x7A,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x2E, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x2F, 0xFA,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x30, 0x26,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x31, 0x01,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x32, 0xB6,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x33, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x34, 0x03,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x35, 0x5A,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x36, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x37, 0x76,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x38, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x39, 0xFE,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x3A, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x3B, 0x0D,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x3C, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x3D, 0x1B,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x3E, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x3F, 0x1C,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x40, 0x01,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x41, 0xF3,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x42, 0x01,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x43, 0xF4,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x44, 0x80,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x45, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x46, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x47, 0x41,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x48, 0x08,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x49, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x4A, 0xFC,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x4B, 0x08,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x4C, 0x16,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x4D, 0x08,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x4E, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x4F, 0x4E,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x50, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x51, 0xC2,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x52, 0x01,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x53, 0x2D,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x54, 0x01,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x55, 0x2B,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x56, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x57, 0x2B,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x58, 0x23,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x59, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x5A, 0x25,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x5B, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x5C, 0x25,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x5D, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x5E, 0x1D,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x5F, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x60, 0x23,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x61, 0x02,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x62, 0x1D,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x63, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x64, 0x1A,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x65, 0x03,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x66, 0x0A,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x67, 0xF0,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x68, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x69, 0xF0,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x6A, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x6B, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x6C, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x6D, 0xF0,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x6E, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x6F, 0x60,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x70, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x71, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x72, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x73, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x74, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x75, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x76, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x77, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x78, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x79, 0x68,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x7A, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x7B, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x7C, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x7D, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x7E, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x7F, 0x00,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), 0x00, 0x0F};

        int32_t _rst;

    public:
        Ecx336c(std::shared_ptr<Bus_Spi_Guide> bus, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE,
                 int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Spi_Guide(bus, cs), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);
    };
}