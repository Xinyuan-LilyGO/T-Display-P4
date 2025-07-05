/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2025-06-16 18:12:04
 * @License: GPL 3.0
 */
#include "es8311.h"

namespace Cpp_Bus_Driver
{
    bool Es8311::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        if (software_reset(true) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "software_reset fail\n");
            return false;
        }
        Iic_Guide::delay_ms(20);
        if (software_reset(false) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "software_reset fail\n");
            return false;
        }

        uint16_t buffer = get_device_id();
        if (buffer != ES8311_DEVICE_ID)
        {
            Iic_Guide::assert_log(Log_Level::INFO, __FILE__, __LINE__, "get es8311 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            Iic_Guide::assert_log(Log_Level::INFO, __FILE__, __LINE__, "get es8311 id: %#X\n", buffer);
        }

        return true;
    }

    bool Es8311::begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width)
    {
        if (Iis_Guide::begin(mclk_multiple, sample_rate_hz, data_bit_width) == false)
        {
            Iis_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }
        return true;
    }

    uint16_t Es8311::get_device_id(void)
    {
        uint8_t buffer[2] = {0};

        for (uint8_t i = 0; i < 2; i++)
        {
            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID_START) + i, &buffer[i]) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
        }

        return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
    }

    bool Es8311::software_reset(bool enable)
    {
        uint8_t buffer = 0;

        // 启动复位
        if (enable == true)
        {
            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_RESET_SERIAL_PORT_MODE_CONTROL), &buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            buffer = (buffer & 0B01100000) | 0B00011111;

            if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_RESET_SERIAL_PORT_MODE_CONTROL), buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else // 关闭复位
        {
            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_RESET_SERIAL_PORT_MODE_CONTROL), &buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            buffer = (buffer & 0B01100000) | 0B10000000;
            if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_RESET_SERIAL_PORT_MODE_CONTROL), buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

    bool Es8311::set_master_clock_source(Clock_Source clock)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_1), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(clock) << 7);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_1), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_clock(Clock_Source clock, bool enalbe, bool invert)
    {
        uint8_t buffer = 0;

        switch (clock)
        {
        case Clock_Source::ADC_DAC_MCLK:
            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_1), &buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(invert) << 6);
            buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(enalbe) << 5);
            buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enalbe) << 3); // 控制内部ADC时钟的开启或关闭的控制位
            buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(enalbe) << 2); // 控制内部DAC时钟的开启或关闭的控制位
            buffer = (buffer & 0B11111101) | (static_cast<uint8_t>(enalbe) << 1); // 未知位，必须置1才能正常工作
            buffer = (buffer & 0B11111110) | (static_cast<uint8_t>(enalbe));      // 未知位，必须置1才能正常工作
            if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_1), buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Clock_Source::ADC_DAC_BCLK:
        {
            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_1), &buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            buffer = (buffer & 0B11101111) | (static_cast<uint8_t>(enalbe) << 4);
            buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enalbe) << 3); // 控制内部ADC时钟的开启或关闭的控制位
            buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(enalbe) << 2); // 控制内部DAC时钟的开启或关闭的控制位
            buffer = (buffer & 0B11111101) | (static_cast<uint8_t>(enalbe) << 1); // 未知位，必须置1才能正常工作
            buffer = (buffer & 0B11111110) | (static_cast<uint8_t>(enalbe));      // 未知位，必须置1才能正常工作
            if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_1), buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_6), &buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(invert) << 5);
            if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_6), buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        }
        default:
            break;
        }

        return true;
    }

    bool Es8311::set_dac_volume(uint8_t volume)
    {
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_DAC_VOLUME), volume) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_adc_volume(uint8_t volume)
    {
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_VOLUME), volume) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_adc_auto_volume_control(bool enable)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_ADC_ALC), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_ALC), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_mic(Mic_Type type, Mic_Input input)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_ADC_DMIC_PGA_GAIN), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(type) << 6);
        buffer = (buffer & 0B11001111) | (static_cast<uint8_t>(input) << 4);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_DMIC_PGA_GAIN), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_power_status(Power_Status status)
    {
        uint8_t buffer =
            static_cast<uint8_t>(!status.contorl.analog_circuits) << 7 |
            static_cast<uint8_t>(!status.contorl.analog_bias_circuits) << 6 |
            static_cast<uint8_t>(!status.contorl.analog_adc_bias_circuits) << 5 |
            static_cast<uint8_t>(!status.contorl.analog_adc_reference_circuits) << 4 |
            static_cast<uint8_t>(!status.contorl.analog_dac_reference_circuit) << 3 |
            static_cast<uint8_t>(status.contorl.internal_reference_circuits) << 2 |
            static_cast<uint8_t>(status.vmid);

        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_POWER_UP_POWER_DOWN_CONTORL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_low_power_status(Low_Power_Status status)
    {
        uint8_t buffer =
            static_cast<uint8_t>(status.dac) << 7 |
            static_cast<uint8_t>(status.pga) << 6 |
            static_cast<uint8_t>(status.pga_output) << 5 |
            static_cast<uint8_t>(status.adc) << 4 |
            static_cast<uint8_t>(status.adc_reference) << 3 |
            static_cast<uint8_t>(status.dac_reference) << 2 |
            static_cast<uint8_t>(status.flash) << 1 |
            static_cast<uint8_t>(status.int1);

        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_LOW_POWER_CONTROL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::search_clock_coeff(uint16_t mclk_multiple, uint32_t sample_rate_hz,
                                    const Clock_Coeff *library, size_t library_length, size_t *search_index)
    {
        for (size_t i = 0; i < library_length; i++)
        {
            if ((library[i].mclk_multiple == mclk_multiple) && (library[i].sample_rate == sample_rate_hz))
            {
                if (search_index != nullptr)
                {
                    *search_index = i;
                }
                return true;
            }
        }

        return false;
    }

    bool Es8311::set_clock_coeff(uint16_t mclk_multiple, uint32_t sample_rate_hz)
    {
        size_t buffer_index = 0;

        // 搜索
        if (search_clock_coeff(mclk_multiple, sample_rate_hz,
                               _clock_coeff_list, sizeof(_clock_coeff_list) / sizeof(Clock_Coeff), &buffer_index) == false)
        {
            Iis_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search _clock_coeff_list fail\n");
            return false;
        }

        // Iis_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_index: %d\n", buffer_index);

        const Clock_Coeff *buffer_clock_coeff = &_clock_coeff_list[buffer_index];

        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_2), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0x07;
        buffer |= (buffer_clock_coeff->pre_div - 1) << 5;
        buffer |= buffer_clock_coeff->pre_multi << 3;
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_2), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        buffer = (buffer_clock_coeff->fs_mode << 6) | buffer_clock_coeff->adc_osr;
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_3), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_4), buffer_clock_coeff->dac_osr) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        buffer = ((buffer_clock_coeff->adc_div - 1) << 4) | (buffer_clock_coeff->dac_div - 1);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_5), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_6), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0xE0;
        if (buffer_clock_coeff->bclk_div < 19)
        {
            buffer |= (buffer_clock_coeff->bclk_div - 1) << 0;
        }
        else
        {
            buffer |= (buffer_clock_coeff->bclk_div) << 0;
        }
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_6), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_7), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0xC0;
        buffer |= buffer_clock_coeff->lrck_h << 0;
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_7), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_CLOCK_MANAGER_8), buffer_clock_coeff->lrck_l) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_sdp_data_bit_length(Sdp dsp, Bits_Per_Sample length)
    {
        uint8_t buffer = 0;

        switch (dsp)
        {
        case Sdp::DAC:
            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_SDP_IN_FORMAT), &buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            buffer = (buffer & 0B11100011) | (static_cast<uint8_t>(length) << 2);
            if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_SDP_IN_FORMAT), buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Sdp::ADC:
            if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_SDP_OUT_FORMAT), &buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
            buffer = (buffer & 0B11100011) | (static_cast<uint8_t>(length) << 2);
            if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_SDP_OUT_FORMAT), buffer) == false)
            {
                Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;

        default:
            break;
        }

        return true;
    }

    bool Es8311::set_pga_power(bool enable)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_PGA_ADC_MODULATOR_POWER_CONTROL), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(!enable) << 6);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_PGA_ADC_MODULATOR_POWER_CONTROL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_adc_power(bool enable)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_PGA_ADC_MODULATOR_POWER_CONTROL), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(!enable) << 5);
        buffer = (buffer & 0B11101111) | (static_cast<uint8_t>(!enable) << 4);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_PGA_ADC_MODULATOR_POWER_CONTROL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_dac_power(bool enable)
    {
        uint8_t buffer = 0;

        if (enable == true)
        {
            buffer = 0x00;
        }
        else
        {
            buffer = 0x02; // 默认值
        }
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_DAC_POWER_CONTROL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_output_to_hp_drive(bool enable)
    {
        uint8_t buffer = 0;

        if (enable == true)
        {
            buffer = 0x10;
        }
        else
        {
            buffer = 0x40; // 默认值
        }
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_OUTPUT_TO_HP_DRIVE_CONTROL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_adc_offset_freeze(Adc_Offset_Freeze offset_freeze)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_ADC_EQUALIZER_BYPASS), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11011111) | (static_cast<uint8_t>(offset_freeze) << 5);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_EQUALIZER_BYPASS), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_adc_hpf_stage2_coeff(uint8_t coeff)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_ADC_EQUALIZER_BYPASS), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11100000) | coeff;
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_EQUALIZER_BYPASS), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_dac_equalizer(bool enable)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_DAC_RAMPRATE_EQBYPASS), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(!enable) << 3);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_DAC_RAMPRATE_EQBYPASS), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    size_t Es8311::read_data(void *data, size_t byte)
    {
        size_t buffer = Iis_Guide::_bus->read(data, byte);

        if (buffer == 0)
        {
            Iis_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return buffer;
    }

    size_t Es8311::write_data(const void *data, size_t byte)
    {
        size_t buffer = Iis_Guide::_bus->write(data, byte);

        if (buffer == 0)
        {
            Iis_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return buffer;
    }

    bool Es8311::set_adc_gain(Adc_Gain gain)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_ADC_GAIN_SCALE_UP), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11111000) | static_cast<uint8_t>(gain);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_GAIN_SCALE_UP), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_adc_data_to_dac(bool enable)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_ADC_DAC_CONTROL), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_DAC_CONTROL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_adc_pga_gain(Adc_Pga_Gain gain)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_ADC_DMIC_PGA_GAIN), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11110000) | static_cast<uint8_t>(gain);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_ADC_DMIC_PGA_GAIN), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Es8311::set_serial_port_mode(Serial_Port_Mode mode)
    {
        uint8_t buffer = 0;

        if (Iic_Guide::_bus->read(static_cast<uint8_t>(Cmd::RW_RESET_SERIAL_PORT_MODE_CONTROL), &buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B10111011) | (static_cast<uint8_t>(mode) << 6) | (!static_cast<uint8_t>(mode) << 2);
        if (Iic_Guide::_bus->write(static_cast<uint8_t>(Cmd::RW_RESET_SERIAL_PORT_MODE_CONTROL), buffer) == false)
        {
            Iic_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }
}
