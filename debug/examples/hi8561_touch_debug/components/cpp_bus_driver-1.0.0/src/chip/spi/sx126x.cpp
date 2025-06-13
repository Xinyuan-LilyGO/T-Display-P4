/*
 * @Description:
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-02-07 17:18:49
 * @License: GPL 3.0
 */
#include "Sx126x.h"

namespace Cpp_Bus_Driver
{
    // const uint16_t Sx126x::Init_List[] =
    //     {
    //         static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_R16_D8),
    //         static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
    //         static_cast<uint16_t>(Reg::RW_TX_CLAMP_CONFIG),
    //         0B00000000,
    // };

    bool Sx126x::begin(int32_t frequency)
    {
        if (_busy != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            pin_mode(_busy, Pin_Mode::INPUT, Pin_Status::PULLUP);
        }

        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (SPI_Guide::begin(frequency) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        uint8_t buffer = iic_device_id();
        if (buffer != SX126X_DEVICE_DEFAULT_READ_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sx126x id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sx126x id: %#X\n", buffer);
        }

        set_tx_clamp(true);

        // if (iic_init_list(Init_List, sizeof(Init_List)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list fail\n");
        //     return false;
        // }

        return true;
    }

    uint8_t Sx126x::iic_device_id(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RO_DEVICE_ID), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer[1];
    }

    bool Sx126x::check_busy(void)
    {
        if (_busy != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _watchdog_busy = 0;
            while (1)
            {
                delay_ms(1);
                if (pin_read(_busy) == 0)
                {
                    break;
                }
                _watchdog_busy++;
                if (_watchdog_busy > SX126X_DEVICE_DEFAULT_BUSY_WAIT_TIMEOUT_MS)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "watchdog busy timeout\n");
                    return false;
                }
            }
        }
        else if (_busy_wait_callback != nullptr)
        {
            _watchdog_busy = 0;
            while (1)
            {
                delay_ms(1);
                if (_busy_wait_callback() == 0)
                {
                    break;
                }
                _watchdog_busy++;
                if (_watchdog_busy > SX126X_DEVICE_DEFAULT_BUSY_WAIT_TIMEOUT_MS)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "watchdog busy timeout\n");
                    return false;
                }
            }
        }

        return true;
    }

    bool Sx126x::set_standby(Stdby_Config config)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_STANDBY), static_cast<uint8_t>(config)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_dio3_as_tcxo_ctrl(Dio3_Tcxo_Voltage voltage, uint32_t time_out_us)
    {
        time_out_us = static_cast<float>(time_out_us) / 15.625;
        uint8_t buffer[4] =
            {
                static_cast<uint8_t>(voltage),
                static_cast<uint8_t>(time_out_us >> 16),
                static_cast<uint8_t>(time_out_us >> 8),
                static_cast<uint8_t>(time_out_us),
            };
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_DIO3_AS_TCXO_CTRL), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_tx_clamp(bool enable)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_TX_CLAMP_CONFIG),
                       buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable == true)
        {
            buffer[1] |= 0B00011110;
        }
        else
        {
            buffer[1] = (buffer[1] & 0B11100001) | 0B00010000;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_TX_CLAMP_CONFIG), buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_buffer_base_address(uint8_t tx_base_address, uint8_t rx_base_address)
    {
        uint8_t buffer[2] = {tx_base_address, rx_base_address};
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_BUFFER_BASE_ADDRESS), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_packet_type(Packet_Type type)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_PACKET_TYPE), static_cast<uint8_t>(type)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_rx_tx_fallback_mode(Fallback_Mode mode)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_RX_TX_FALLBACK_MODE), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_cad_params(Cad_Symbol_Num num, uint8_t cad_det_peak, uint8_t cad_det_min, Cad_Exit_Mode exit_mode, uint32_t time_out_us)
    {
        time_out_us = static_cast<float>(time_out_us) / 15.625;
        uint8_t buffer[7] =
            {
                static_cast<uint8_t>(num),
                cad_det_peak,
                cad_det_min,
                static_cast<uint8_t>(exit_mode),
                static_cast<uint8_t>(time_out_us >> 16),
                static_cast<uint8_t>(time_out_us >> 8),
                static_cast<uint8_t>(time_out_us),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_CAD_PARAMS), buffer, 7) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::clear_irq_status(uint16_t clear_irq_param)
    {
        uint8_t buffer[2] =
            {
                static_cast<uint8_t>(clear_irq_param >> 8),
                static_cast<uint8_t>(clear_irq_param),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_CLEAR_IRQ_STATUS), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_dio_irq_params(uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask, uint16_t dio3_mask)
    {
        uint8_t buffer[8] =
            {
                static_cast<uint8_t>(irq_mask >> 8),
                static_cast<uint8_t>(irq_mask),
                static_cast<uint8_t>(dio1_mask >> 8),
                static_cast<uint8_t>(dio1_mask),
                static_cast<uint8_t>(dio2_mask >> 8),
                static_cast<uint8_t>(dio2_mask),
                static_cast<uint8_t>(dio3_mask >> 8),
                static_cast<uint8_t>(dio3_mask),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_DIO_IRQ_PARAMS), buffer, 8) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::calibrate(uint8_t calib_param)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_CALIBRATE), calib_param) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint8_t Sx126x::get_status(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sx126x::check_status(void)
    {
        uint8_t assert = get_status();
        if ((assert & 0B00001110) == 0x03)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_status fail: cmd timeout\n");
            return false;
        }
        else if ((assert & 0B00001110) == 0x04)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_status fail: cmd invali\n");
            return false;
        }
        else if ((assert & 0B00001110) == 0x05)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_status fail: cmd failed\n");
            return false;
        }
        else if ((assert == 0x00) || (assert == 0xFF))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_status fail: chip not found\n");
            return false;
        }

        return true;
    }

    uint8_t Sx126x::get_packet_type(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_PACKET_TYPE), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer[1];
    }

    bool Sx126x::set_regulator_mode(Regulator_Mode mode)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_REGULATOR_MODE), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_current_limit(float current)
    {
        if (current < 0.0)
        {
            current = 0.0;
        }
        else if (current > 140.0)
        {
            current = 140.0;
        }

        current /= 2.5;
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_OCP_CONFIGURATION), current) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint8_t Sx126x::get_current_limit(void)
    {
        uint8_t buffer[2] = {0};
        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_OCP_CONFIGURATION), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return static_cast<float>(buffer[1]) * 2.5;
    }

    bool Sx126x::set_dio2_as_rf_switch_ctrl(Dio2_Mode mode)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_DIO2_AS_RF_SWITCH_CTRL), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_pa_config(uint8_t pa_duty_cycle, uint8_t hp_max)
    {
        uint8_t buffer[4] =
            {
                pa_duty_cycle,
                hp_max,
                static_cast<uint8_t>(_chip_type),
                0x01, // 这一位固定为0x01
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_PA_CONFIG), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_tx_params(uint8_t power, Ramp_Time ramp_time)
    {
        uint8_t buffer[2] =
            {
                power,
                static_cast<uint8_t>(ramp_time),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_TX_PARAMS), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_lora_sync_word(uint16_t sync_word)
    {
        uint8_t buffer = sync_word >> 8;

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_LORA_SYNC_WORD_MSB), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        buffer = sync_word;

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_LORA_SYNC_WORD_LSB), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint16_t Sx126x::get_lora_sync_word(void)
    {
        uint8_t buffer[4] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_LORA_SYNC_WORD_MSB), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_LORA_SYNC_WORD_LSB), &buffer[2], 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[3];
    }

    bool Sx126x::set_lora_inverted_iq(Invert_Iq iq)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_IQ_POLARITY_SETUP), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (iq == Invert_Iq::STANDARD_IQ_SETUP)
        {
            buffer[1] |= 0B00000100;
        }
        else
        {
            buffer[1] &= 0B11111011;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_IQ_POLARITY_SETUP), buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_lora_modulation_params(Sf sf, Bw bw, Cr cr, Ldro ldro)
    {
        uint8_t buffer[4] = {static_cast<uint8_t>(sf), static_cast<uint8_t>(bw), static_cast<uint8_t>(cr), static_cast<uint8_t>(ldro)};

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_MODULATION_PARAMS), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_lora_packet_params(uint16_t preamble_length, Header_Type header_type, uint8_t payload_length, Crc_Type crc_type, Invert_Iq iq)
    {
        uint8_t buffer[6] =
            {
                static_cast<uint8_t>(preamble_length >> 8),
                static_cast<uint8_t>(preamble_length),
                static_cast<uint8_t>(header_type),
                payload_length,
                static_cast<uint8_t>(crc_type),
                static_cast<uint8_t>(iq),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_PACKET_PARAMS), buffer, 6) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_output_power(int8_t power)
    {
        if (power < (-9))
        {
            power = -9;
        }
        else if (power > 22)
        {
            power = 22;
        }

        uint8_t buffer[2] = {0};

        // 读取OCP配置
        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_OCP_CONFIGURATION), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (set_pa_config(0x04, 0x07) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_pa_config fail\n");
            return false;
        }

        if (set_tx_params(power, Ramp_Time::RAMP_200_US) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_tx_params fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_OCP_CONFIGURATION), buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::calibrate_image(Img_Cal_Freq freq)
    {
        uint8_t buffer[2] = {0};

        switch (freq)
        {
        case Img_Cal_Freq::FREQ_430_440_MHZ:
            buffer[0] = 0x6B;
            buffer[1] = 0x6F;
            break;
        case Img_Cal_Freq::FREQ_470_510_MHZ:
            buffer[0] = 0x75;
            buffer[1] = 0x81;
            break;
        case Img_Cal_Freq::FREQ_779_787_MHZ:
            buffer[0] = 0xC1;
            buffer[1] = 0xC5;
            break;
        case Img_Cal_Freq::FREQ_863_870_MHZ:
            buffer[0] = 0xD7;
            buffer[1] = 0xDB;
            break;
        case Img_Cal_Freq::FREQ_902_928_MHZ:
            buffer[0] = 0xE1;
            buffer[1] = 0xE9;
            break;

        default:
            break;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_CALIBRATE_IMAGE), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_rf_frequency(float freq)
    {
        uint32_t freq_buffer = (freq * (static_cast<uint32_t>(1) << 25)) / 32.0;

        uint8_t buffer[4] =
            {
                static_cast<uint8_t>(freq_buffer >> 24),
                static_cast<uint8_t>(freq_buffer >> 16),
                static_cast<uint8_t>(freq_buffer >> 8),
                static_cast<uint8_t>(freq_buffer),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_RF_FREQUENCY), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_frequency(float freq)
    {
        if (freq < 150.0)
        {
            freq = 150.0;
        }
        else if (freq > 960.0)
        {
            freq = 960.0;
        }

        if ((freq >= 902) && (freq <= 928))
        {
            calibrate_image(Img_Cal_Freq::FREQ_902_928_MHZ);
        }
        else if ((freq >= 863) && (freq <= 870))
        {
            calibrate_image(Img_Cal_Freq::FREQ_863_870_MHZ);
        }
        else if ((freq >= 779) && (freq <= 787))
        {
            calibrate_image(Img_Cal_Freq::FREQ_779_787_MHZ);
        }
        else if ((freq >= 470) && (freq <= 510))
        {
            calibrate_image(Img_Cal_Freq::FREQ_470_510_MHZ);
        }
        else if ((freq >= 430) && (freq <= 440))
        {
            calibrate_image(Img_Cal_Freq::FREQ_430_440_MHZ);
        }

        // 设置射频频率模式的频率
        if (set_rf_frequency(freq) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rf_frequency fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::config_lora_params(float frequency, Bw bw, float current_limit, int8_t power, Sf sf, Cr cr, Crc_Type crc_type,
                                    uint16_t preamble_length, uint16_t sync_word, Regulator_Mode regulator_mode)
    {
        // 启用13MHz晶振模式
        if (set_standby(Stdby_Config::STDBY_RC) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_standby fail\n");
            return false;
        }

        // TCXO的供电电压不能超过供电电压减去200 mV （VDDop > VTCXO + 200 mV）
        if (set_dio3_as_tcxo_ctrl(Dio3_Tcxo_Voltage::OUTPUT_1600_MV, 5000) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_dio3_as_tcxo_ctrl fail\n");
            return false;
        }

        // 设置包类型
        if (set_packet_type(Packet_Type::LORA) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_packet_type fail\n");
            return false;
        }

        if (set_rx_tx_fallback_mode(Fallback_Mode::STDBY_RC) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_packet_type fail\n");
            return false;
        }

        if (set_cad_params(Cad_Symbol_Num::ON_8_SYMB, static_cast<uint8_t>(sf) + 13, 10, Cad_Exit_Mode::ONLY, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cad_params fail\n");
            return false;
        }

        // 清除所有中断
        if (clear_irq_status() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "clear_irq_status fail\n");
            return false;
        }

        // 设置中断标志
        if (set_dio_irq_params(0, 0, 0, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_dio_irq_params fail\n");
            return false;
        }

        // 校准
        if (calibrate(0b01111111) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "calibrate fail\n");
            return false;
        }

        // delay_ms(5);
        // check_busy();

        // 检查状态
        if (check_status() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "check_status fail\n");
            return false;
        }

        if (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / static_cast<float>(bw) >= 16.0)
        {
            if (set_lora_modulation_params(sf, bw, cr, Ldro::LDRO_ON) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_modulation_params fail\n");
                return false;
            }
        }
        else
        {
            if (set_lora_modulation_params(sf, bw, cr, Ldro::LDRO_OFF) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_modulation_params fail\n");
                return false;
            }
        }

        // 设置同步字
        if (set_lora_sync_word(sync_word) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_sync_word fail\n");
            return false;
        }

        if (set_lora_inverted_iq(Invert_Iq::STANDARD_IQ_SETUP) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_inverted_iq fail\n");
            return false;
        }

        // 设置包类型
        if (set_lora_packet_params(preamble_length, Header_Type::VARIABLE_LENGTH_PACKET, 255, crc_type, Invert_Iq::STANDARD_IQ_SETUP) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_packet_params fail\n");
            return false;
        }

        // 设置电源调节器模式
        if (set_regulator_mode(regulator_mode) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_regulator_mode fail\n");
            return false;
        }

        // 设置电流限制
        if (set_current_limit(current_limit) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_current_limit fail\n");
            return false;
        }

        // 设置DIO2的模式功能为控制RF开关
        if (set_dio2_as_rf_switch_ctrl(Dio2_Mode::RF_SWITCH) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_dio2_as_rf_switch_ctrl fail\n");
            return false;
        }

        // 设置频率
        if (set_frequency(frequency) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_frequency fail\n");
            return false;
        }

        // 设置功率
        if (set_output_power(power) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_output_power fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_rx(uint32_t time_out_us)
    {
        if (time_out_us != 0xFFFFFF)
        {
            time_out_us = static_cast<float>(time_out_us) / 15.625;
        }

        uint8_t buffer[3] =
            {
                static_cast<uint8_t>(time_out_us >> 16),
                static_cast<uint8_t>(time_out_us >> 8),
                static_cast<uint8_t>(time_out_us),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_RX), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::start_lora_receive(uint32_t time_out_us, uint16_t preamble_length, Crc_Type crc_type)
    {
        // 设置接收中断标志
        // 默认设置 irq_mask：RX_DONE, TIMEOUT, CRC_ERR 和 HEADER_ERR
        // 默认设置 dio1_mask：RX_DONE（包接收完成后中断）
        if (set_dio_irq_params(0B0000001001100010, 0B0000000000000010, 0, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_dio_irq_params fail\n");
            return false;
        }

        // 设置基地址
        if (set_buffer_base_address(0, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_buffer_base_address fail\n");
            return false;
        }

        // 清除全部中断
        if (clear_irq_status() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "clear_irq_status fail\n");
            return false;
        }

        // 设置包类型
        if (set_lora_packet_params(preamble_length, Header_Type::VARIABLE_LENGTH_PACKET, 255, crc_type, Invert_Iq::STANDARD_IQ_SETUP) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_packet_params fail\n");
            return false;
        }

        check_busy();

        // 设置为接收模式
        if (set_rx(time_out_us) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rx fail\n");
            return false;
        }

        return true;
    }

    uint16_t Sx126x::get_irq_status(void)
    {
        uint8_t buffer[3] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_IRQ_STATUS), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[2];
    }

    uint8_t Sx126x::get_rx_buffer_length(void)
    {
        uint8_t buffer[3] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_RX_BUFFER_STATUS), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer[1];
    }

    bool Sx126x::read_buffer(uint8_t *data, uint8_t length, uint8_t offset)
    {
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_READ_BUFFER), offset, data, length) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return true;
    }

    uint8_t Sx126x::receive_data(uint8_t *data, uint8_t length)
    {
        // 检查状态
        if (check_status() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "check_status fail\n");
            return false;
        }

        // 检查RX中断超时
        uint16_t assert = get_irq_status();
        if ((assert & 0B0000001000000000) == 1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "RX timeout\n");
            return false;
        }

        // 检查Crc错误
        if ((assert & 0B0000000001000000))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "Crc error\n");
            // return false;
        }

        // 检查Header错误
        if ((assert & 0B0000000000100000))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "Header error\n");
            // return false;
        }

        uint8_t length_buffer = get_rx_buffer_length();
        if (length_buffer == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_buffer_length is empty\n");
        }
        if ((length == 0) || (length > length_buffer))
        {
            read_buffer(data, length_buffer);
        }
        else if (length < length_buffer)
        {
            read_buffer(data, length);
        }

        // 复原基地址
        if (set_buffer_base_address(0, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_buffer_base_address fail\n");
            return false;
        }

        // 清除全部中断
        if (clear_irq_status() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "clear_irq_status fail\n");
            return false;
        }

        return length_buffer;
    }
}
