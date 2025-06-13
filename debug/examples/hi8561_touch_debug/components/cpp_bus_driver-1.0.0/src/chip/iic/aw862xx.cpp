/*
 * @Description:
 * @Author: LILYGO_L
 * @Date: 2024-12-26 11:13:26
 * @LastEditTime: 2025-01-22 14:37:41
 * @License: GPL 3.0
 */
#include "aw862xx.h"

namespace Cpp_Bus_Driver
{
    bool Aw862xx::begin(int32_t frequency)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (IIC_Guide::begin(frequency) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        uint8_t buffer = iic_device_id();
        if (buffer == static_cast<uint8_t>(DEFAULT_CPP_BUS_DRIVER_VALUE))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get aw862xx id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get aw862xx id: %#X\n", buffer);
        }

        // if (_bus->BufferOperation(_address, PCF85063_Initialization_BufferOperations,
        //                           sizeof(PCF85063_Initialization_BufferOperations)) == false)
        // {
        //     return false;
        // }

        return true;
    }

    uint8_t Aw862xx::iic_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Aw862xx::software_reset(void)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SRST), 0xAA) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        return true;
    }

    float Aw862xx::get_input_voltage(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSCTRL1), &buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        buffer[0] |= 0B00001000;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSCTRL1), buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_DETCFG2), &buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        buffer[0] |= 0B00000010;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_DETCFG2), buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        delay_ms(3);

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSCTRL1), &buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        buffer[0] &= 0B11110111;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSCTRL1), buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_DET_VBAT), &buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_DET_LO), &buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        buffer[1] = (buffer[1] & 0B00110000) >> 4;

        return ((6.1 * (buffer[0] * 4 + buffer[1])) / 1024);
    }

    bool Aw862xx::rtp_playback_waveform(const uint8_t *waveform_data, size_t length)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_PLAYCFG3), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        buffer = (buffer & 0B11111100) | 0B00000001;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_PLAYCFG3), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_PLAYCFG4), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }
        buffer = 0B00000001;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_PLAYCFG4), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        // delay_ms(1);

        buffer = 0;
        uint8_t timeout_count = 0;
        while ((buffer & 0B00001111) != 0B00001000)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_GLBRD5), &buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
            timeout_count++;
            if (timeout_count > 100)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rtp wait timeout (RO_GLBRD5: %#X)\n", buffer);
                return -1;
            }
            delay_ms(1);
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RTPDATA), waveform_data, length) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        return true;
    }
}
