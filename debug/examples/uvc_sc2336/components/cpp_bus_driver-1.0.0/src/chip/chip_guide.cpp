/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2025-04-09 10:35:37
 * @License: GPL 3.0
 */
#include "chip_guide.h"

namespace Cpp_Bus_Driver
{
    bool Iic_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz, _address) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Iic_Guide::init_list(const uint8_t *list, size_t length)
    {
        for (size_t i = 0; i < length; i++)
        {
            switch (list[i])
            {
            case static_cast<uint8_t>(Init_List_Cmd::DELAY_MS):
                i++;
                delay_ms(list[i]);
                break;
            case static_cast<uint8_t>(Init_List_Cmd::WRITE_DATA):
                i++;
                if (_bus->write(&list[i + 1], list[i]) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list write fail\n");
                    return false;
                }
                i = list[i] + 1;
                break;
            case static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8):
                i++;
                if (_bus->write(&list[i], 2) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list write fail\n");
                    return false;
                }
                i += 2;
                break;

            default:
                break;
            }
        }

        return true;
    }

    bool Spi_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz, _cs) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Uart_Guide::begin(int32_t baud_rate)
    {
        if (_bus->begin(baud_rate) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Iis_Guide::begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width)
    {
        if (_bus->begin(mclk_multiple, sample_rate_hz, data_bit_width) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool Sdio_Guide::begin(int32_t freq_hz)
    {
        if (_bus->begin(freq_hz) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }
}