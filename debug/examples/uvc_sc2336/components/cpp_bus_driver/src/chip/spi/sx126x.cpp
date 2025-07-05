/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-06-27 10:07:10
 * @License: GPL 3.0
 */
#include "Sx126x.h"

namespace Cpp_Bus_Driver
{
    bool Sx126x::begin(int32_t freq_hz)
    {
        if (_busy != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            pin_mode(_busy, Pin_Mode::INPUT, Pin_Status::DISABLE);
        }

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

        if (Spi_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        auto buffer = get_device_id();
        if ((buffer == 0x00) || (buffer == 0xFF))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sx126x id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sx126x id: %#X\n", buffer);
        }

        if (fix_tx_clamp(true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "fix_tx_clamp fail\n");
        }

        // if (iic_init_list(Init_List, sizeof(Init_List)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list fail\n");
        //     return false;
        // }

        // 启用13MHz晶振模式
        if (set_standby(Stdby_Config::STDBY_RC) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_standby fail\n");
        }

        // TCXO的供电电压不能超过供电电压减去200 mV （VDDop > VTCXO + 200 mV）
        if (set_dio3_as_tcxo_ctrl(Dio3_Tcxo_Voltage::OUTPUT_1600_MV, 5000) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_dio3_as_tcxo_ctrl fail\n");
        }

        // 设置电源调节器模式
        if (set_regulator_mode(Regulator_Mode::LDO_AND_DCDC) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_regulator_mode fail\n");
        }

        // 设置DIO2的模式功能为控制RF开关
        if (set_dio2_as_rf_switch_ctrl(Dio2_Mode::RF_SWITCH) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_dio2_as_rf_switch_ctrl fail\n");
        }

        return true;
    }

    uint8_t Sx126x::get_device_id(void)
    {
        return get_status();
    }

    bool Sx126x::check_busy(void)
    {
        if (_busy != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            uint16_t timeout_count = 0;
            while (1)
            {
                delay_us(1);
                if (pin_read(_busy) == 0)
                {
                    break;
                }
                timeout_count++;
                if (timeout_count > SX126X_BUSY_PIN_TIMEOUT_COUNT)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_busy timeout\n");
                    return false;
                }
            }
        }
        else if (_busy_wait_callback != nullptr)
        {
            uint16_t timeout_count = 0;
            while (1)
            {
                delay_us(1);
                if (_busy_wait_callback() == 0)
                {
                    break;
                }
                timeout_count++;
                if (timeout_count > SX126X_BUSY_FUNCTION_TIMEOUT_COUNT)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_busy_wait_callback timeout\n");
                    return false;
                }
            }
        }

        return true;
    }

    uint8_t Sx126x::get_status(void)
    {
        uint8_t buffer = 0;

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    Sx126x::Cmd_Status Sx126x::assert_cmd_status(uint8_t assert_status)
    {
        if ((assert_status == 0x00) || (assert_status == static_cast<uint8_t>(-1)))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read error\n");
            return Cmd_Status::FALSE;
        }

        const uint8_t buffer = (assert_status & 0B00001110) >> 1;

        switch (buffer)
        {
        case static_cast<uint8_t>(Cmd_Status::RFU):
            break;
        case static_cast<uint8_t>(Cmd_Status::DATA_IS_AVAILABLE_TO_HOST):
            break;
        case static_cast<uint8_t>(Cmd_Status::CMD_TIMEOUT):
            break;
        case static_cast<uint8_t>(Cmd_Status::CMD_PROCESSING_ERROR):
            break;
        case static_cast<uint8_t>(Cmd_Status::FAIL_TO_EXECUTE_CMD):
            break;
        case static_cast<uint8_t>(Cmd_Status::CMD_TX_DONE):
            break;

        default:
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "unknown command (error code: %#X)\n", buffer);
            return Cmd_Status::FALSE;
        }

        return static_cast<Cmd_Status>(buffer);
    }

    Sx126x::Chip_Mode_Status Sx126x::assert_chip_mode_status(uint8_t assert_status)
    {
        if ((assert_status == 0x00) || (assert_status == static_cast<uint8_t>(-1)))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read error\n");
            return Chip_Mode_Status::FALSE;
        }

        const uint8_t buffer = (assert_status & 0B01110000) >> 4;

        switch (buffer)
        {
        case static_cast<uint8_t>(Chip_Mode_Status::STBY_RC):
            break;
        case static_cast<uint8_t>(Chip_Mode_Status::STBY_XOSC):
            break;
        case static_cast<uint8_t>(Chip_Mode_Status::FS):
            break;
        case static_cast<uint8_t>(Chip_Mode_Status::RX):
            break;
        case static_cast<uint8_t>(Chip_Mode_Status::TX):
            break;

        default:
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "unknown command (error code: %#X)\n", buffer);
            return Chip_Mode_Status::FALSE;
        }

        return static_cast<Chip_Mode_Status>(buffer);
    }

    bool Sx126x::assert_irq_status(uint16_t irq_flag, Irq_Status &status)
    {
        if (irq_flag == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read error\n");
            return false;
        }

        status.all_flag.tx_done = irq_flag & 0B0000000000000001;
        status.all_flag.rx_done = (irq_flag & 0B0000000000000010) > 1;
        status.all_flag.preamble_detected = (irq_flag & 0B0000000000000100) > 2;
        status.gfsk_flag.sync_word_valid = (irq_flag & 0B0000000000001000) > 3;
        status.lora_reg_flag.header_valid = (irq_flag & 0B0000000000010000) > 4;
        status.lora_reg_flag.header_error = (irq_flag & 0B0000000000100000) > 5;
        status.all_flag.crc_error = (irq_flag & 0B0000000001000000) > 6;
        status.lora_reg_flag.cad_done = (irq_flag & 0B0000000010000000) > 7;
        status.lora_reg_flag.cad_detected = (irq_flag & 0B0000000100000000) > 8;
        status.all_flag.tx_rx_timeout = (irq_flag & 0B0000001000000000) > 9;
        status.lrfhss_flag.pa_ramped_up_hop = (irq_flag & 0B0100000000000000) > 14;

        return true;
    }

    bool Sx126x::set_standby(Stdby_Config config)
    {
        check_busy();
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
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(voltage),
                static_cast<uint8_t>(time_out_us >> 16),
                static_cast<uint8_t>(time_out_us >> 8),
                static_cast<uint8_t>(time_out_us),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_DIO3_AS_TCXO_CTRL), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::fix_tx_clamp(bool enable)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_TX_CLAMP_CONFIG), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        // if (assert_cmd_status(buffer[0]) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
        //     return false;
        // }

        if (enable == true)
        {
            buffer[1] |= 0B00011110;
        }
        else
        {
            buffer[1] = (buffer[1] & 0B11100001) | 0B00010000;
        }

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_TX_CLAMP_CONFIG), buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_buffer_base_address(uint8_t tx_base_address, uint8_t rx_base_address)
    {
        uint8_t buffer[] = {tx_base_address, rx_base_address};

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_BUFFER_BASE_ADDRESS), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_packet_type(Packet_Type type)
    {
        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_PACKET_TYPE), static_cast<uint8_t>(type)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.packet_type = type;

        return true;
    }

    bool Sx126x::set_rx_tx_fallback_mode(Fallback_Mode mode)
    {
        check_busy();
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
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(num),
                cad_det_peak,
                cad_det_min,
                static_cast<uint8_t>(exit_mode),
                static_cast<uint8_t>(time_out_us >> 16),
                static_cast<uint8_t>(time_out_us >> 8),
                static_cast<uint8_t>(time_out_us),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_CAD_PARAMS), buffer, 7) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::clear_irq_flag(Irq_Flag flag)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(static_cast<uint16_t>(flag) >> 8),
                static_cast<uint8_t>(flag),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_CLEAR_IRQ_STATUS), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_dio_irq_params(uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask, uint16_t dio3_mask)
    {
        uint8_t buffer[] =
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

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_DIO_IRQ_PARAMS), buffer, 8) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::calibrate(uint8_t calib_param)
    {
        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_CALIBRATE), calib_param) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    Sx126x::Packet_Type Sx126x::get_packet_type(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_PACKET_TYPE), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return Packet_Type::FALSE;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return Packet_Type::FALSE;
        }

        switch (buffer[1])
        {
        case static_cast<uint8_t>(Packet_Type::GFSK):
            break;
        case static_cast<uint8_t>(Packet_Type::LORA):
            break;
        case static_cast<uint8_t>(Packet_Type::LR_FHSS):
            break;

        default:
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "unknown packet type (error code: %#X)\n", buffer[1]);
            return Packet_Type::FALSE;
        }

        return static_cast<Packet_Type>(buffer[1]);
    }

    bool Sx126x::set_regulator_mode(Regulator_Mode mode)
    {
        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_REGULATOR_MODE), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.regulator_mode = mode;

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

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_OCP_CONFIGURATION), current) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.current_limit = current;

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

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return -1;
        }

        return static_cast<float>(buffer[1]) * 2.5;
    }

    bool Sx126x::set_dio2_as_rf_switch_ctrl(Dio2_Mode mode)
    {
        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_DIO2_AS_RF_SWITCH_CTRL), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_pa_config(uint8_t pa_duty_cycle, uint8_t hp_max)
    {
        uint8_t buffer[] =
            {
                pa_duty_cycle,
                hp_max,
                static_cast<uint8_t>(_chip_type),
                0x01, // 这一位固定为0x01
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_PA_CONFIG), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_tx_params(uint8_t power, Ramp_Time ramp_time)
    {
        uint8_t buffer[] = {power, static_cast<uint8_t>(ramp_time)};

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_TX_PARAMS), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_lora_sync_word(uint16_t sync_word)
    {
        uint8_t buffer[2] = {static_cast<uint8_t>(sync_word >> 8), static_cast<uint8_t>(sync_word)};

        for (uint8_t i = 0; i < 2; i++)
        {
            check_busy();
            if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_LORA_SYNC_WORD_START) + i, buffer[i]) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        _param.lora.sync_word = sync_word;

        return true;
    }

    uint16_t Sx126x::get_lora_sync_word(void)
    {
        uint8_t buffer[4] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_LORA_SYNC_WORD_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return -1;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(static_cast<uint16_t>(Reg::RW_LORA_SYNC_WORD_START) + 1), &buffer[2], 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        buffer_cs = assert_cmd_status(buffer[2]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[2]);
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[3];
    }

    bool Sx126x::fix_lora_inverted_iq(Invert_Iq iq)
    {
        uint8_t buffer[2] = {0};

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_IQ_POLARITY_SETUP), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
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

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_IQ_POLARITY_SETUP), buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_lora_modulation_params(Sf sf, Lora_Bw bw, Cr cr, Ldro ldro)
    {
        uint8_t buffer[] = {static_cast<uint8_t>(sf), static_cast<uint8_t>(bw), static_cast<uint8_t>(cr), static_cast<uint8_t>(ldro)};

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_MODULATION_PARAMS), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.lora.spreading_factor = sf;
        _param.lora.band_width = bw;
        _param.lora.cr = cr;
        _param.lora.low_data_rate_optimize = ldro;

        return true;
    }

    bool Sx126x::set_lora_packet_params(uint16_t preamble_length, Lora_Header_Type header_type, uint8_t payload_length, Lora_Crc_Type crc_type, Invert_Iq iq)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(preamble_length >> 8),
                static_cast<uint8_t>(preamble_length),
                static_cast<uint8_t>(header_type),
                payload_length,
                static_cast<uint8_t>(crc_type),
                static_cast<uint8_t>(iq),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_PACKET_PARAMS), buffer, 6) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.lora.preamble_length = preamble_length;
        _param.lora.header_type = header_type;
        _param.lora.payload_length = payload_length;
        _param.lora.crc_type = crc_type;
        _param.lora.invert_iq = iq;

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
        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_OCP_CONFIGURATION), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
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

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_OCP_CONFIGURATION), buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.power = power;

        return true;
    }

    bool Sx126x::calibrate_image(Img_Cal_Freq freq_mhz)
    {
        uint8_t buffer[2] = {0};

        switch (freq_mhz)
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

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_CALIBRATE_IMAGE), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_rf_frequency(float freq_mhz)
    {
        uint32_t buffer_freq = (freq_mhz * (static_cast<uint32_t>(1) << 25)) / 32.0;

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(buffer_freq >> 24),
                static_cast<uint8_t>(buffer_freq >> 16),
                static_cast<uint8_t>(buffer_freq >> 8),
                static_cast<uint8_t>(buffer_freq),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_RF_FREQUENCY), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_frequency(float freq_mhz)
    {
        if (freq_mhz < 150.0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "freq=%f out of range\n", freq_mhz);
            freq_mhz = 150.0;
        }
        else if (freq_mhz > 960.0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "freq=%f out of range\n", freq_mhz);
            freq_mhz = 960.0;
        }

        if ((freq_mhz >= 902) && (freq_mhz <= 928))
        {
            calibrate_image(Img_Cal_Freq::FREQ_902_928_MHZ);
        }
        else if ((freq_mhz >= 863) && (freq_mhz <= 870))
        {
            calibrate_image(Img_Cal_Freq::FREQ_863_870_MHZ);
        }
        else if ((freq_mhz >= 779) && (freq_mhz <= 787))
        {
            calibrate_image(Img_Cal_Freq::FREQ_779_787_MHZ);
        }
        else if ((freq_mhz >= 470) && (freq_mhz <= 510))
        {
            calibrate_image(Img_Cal_Freq::FREQ_470_510_MHZ);
        }
        else if ((freq_mhz >= 430) && (freq_mhz <= 440))
        {
            calibrate_image(Img_Cal_Freq::FREQ_430_440_MHZ);
        }

        // 设置射频频率模式的频率
        if (set_rf_frequency(freq_mhz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rf_frequency fail\n");
            return false;
        }
        _param.freq_mhz = freq_mhz;

        return true;
    }

    bool Sx126x::config_lora_params(float freq_mhz, Lora_Bw bw, float current_limit, int8_t power, Sf sf, Cr cr, Lora_Crc_Type crc_type,
                                    uint16_t preamble_length, uint16_t sync_word)
    {
        // 启用13MHz晶振模式
        if (set_standby(Stdby_Config::STDBY_RC) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_standby fail\n");
            return false;
        }

        // 设置包类型
        if (set_packet_type(Packet_Type::LORA) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_packet_type fail\n");
            return false;
        }

        if (set_cad_params(Cad_Symbol_Num::ON_8_SYMB, static_cast<uint8_t>(sf) + 13, 10, Cad_Exit_Mode::ONLY, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cad_params fail\n");
            return false;
        }

        // 校准
        if (calibrate(0b01111111) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "calibrate fail\n");
            return false;
        }

        delay_ms(5);
        check_busy();

        // 检查 calibrate 命令结果
        Cmd_Status buffer_cs = assert_cmd_status(get_status());
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", static_cast<uint8_t>(buffer_cs));
            return false;
        }

        // 自动设置低速率优化（大于或等于16.384 ms时建议启用）
        // SF11 with BW125 kHz: time = (2^11 / 125000Hz)*1000 = 16.384 ms (建议启用 LDRO)
        // SF12 with BW125 kHz: time = (2^12 / 125000Hz)*1000 = 32.768 ms (建议启用 LDRO)
        // SF12 with BW250 kHz: time = (2^12 / 250000Hz)*1000 = 16.384 ms (建议启用 LDRO)

        float buffer_time = 0;
        switch (bw)
        {
        case Lora_Bw::BW_7810HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 7810.0) * 1000.0;
            break;
        case Lora_Bw::BW_15630HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 15630.0) * 1000.0;
            break;
        case Lora_Bw::BW_31250HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 31250.0) * 1000.0;
            break;
        case Lora_Bw::BW_62500HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 62500.0) * 1000.0;
            break;
        case Lora_Bw::BW_125000HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 125000.0) * 1000.0;
            break;
        case Lora_Bw::BW_250000HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 250000.0) * 1000.0;
            break;
        case Lora_Bw::BW_500000HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 500000.0) * 1000.0;
            break;
        case Lora_Bw::BW_10420HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 10420.0) * 1000.0;
            break;
        case Lora_Bw::BW_20830HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 20830.0) * 1000.0;
            break;
        case Lora_Bw::BW_41670HZ:
            buffer_time = (static_cast<float>((static_cast<uint32_t>(1) << static_cast<uint8_t>(sf))) / 41670.0) * 1000.0;
            break;

        default:
            break;
        }
        if (buffer_time >= 16.384)
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

        if (fix_lora_inverted_iq(Invert_Iq::STANDARD_IQ_SETUP) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "fix_lora_inverted_iq fail\n");
            return false;
        }

        // 设置包的参数
        if (set_lora_packet_params(preamble_length, Lora_Header_Type::VARIABLE_LENGTH_PACKET, SX126X_MAX_TRANSMIT_BUFFER_SIZE, crc_type, Invert_Iq::STANDARD_IQ_SETUP) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_packet_params fail\n");
            return false;
        }

        // 设置电流限制
        if (set_current_limit(current_limit) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_current_limit fail\n");
            return false;
        }

        // 设置频率
        if (set_frequency(freq_mhz) == false)
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

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(time_out_us >> 16),
                static_cast<uint8_t>(time_out_us >> 8),
                static_cast<uint8_t>(time_out_us),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_RX), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::start_lora_transmit(Chip_Mode chip_mode, uint32_t time_out_us,
                                     Fallback_Mode fallback_mode, uint16_t preamble_length)
    {
        // 从RX或TX模式退出返回的模式设定
        if (set_rx_tx_fallback_mode(fallback_mode) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_packet_type fail\n");
            return false;
        }

        // 设置包类型
        if (set_lora_packet_params(preamble_length, _param.lora.header_type, _param.lora.payload_length, _param.lora.crc_type,
                                   _param.lora.invert_iq) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_packet_params fail\n");
            return false;
        }

        switch (chip_mode)
        {
        case Chip_Mode::RX:
            // 设置为接收模式
            if (set_rx(time_out_us) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rx fail\n");
                return false;
            }
            break;
        case Chip_Mode::TX:
            // 设置为发送模式
            // if (set_tx(time_out_us) == false)
            // {
            //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rx fail\n");
            //     return false;
            // }
            break;

        default:
            break;
        }

        return true;
    }

    uint16_t Sx126x::get_irq_flag(void)
    {
        uint8_t buffer[3] = {0};

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_IRQ_STATUS), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[2];
    }

    uint8_t Sx126x::get_rx_buffer_length(void)
    {
        uint8_t buffer[3] = {0};

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_RX_BUFFER_STATUS), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return false;
        }

        return buffer[1];
    }

    bool Sx126x::read_buffer(uint8_t *data, uint8_t length, uint8_t offset)
    {
        // 设置基地址
        if (set_buffer_base_address(0, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_buffer_base_address fail\n");
            return false;
        }

        uint8_t buffer[length + 1] = {0};

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_READ_BUFFER), offset, buffer, length + 1) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return false;
        }

        memcpy(data, &buffer[1], length);

        return true;
    }

    uint8_t Sx126x::receive_data(uint8_t *data, uint8_t length)
    {
        _assert = 0;

        // // 检查中断
        // Irq_Status buffer_is;
        // if (assert_irq_flag(get_irq_flag(), buffer_is) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_irq_flag fail\n");
        //     _assert = 1;
        //     return false;
        // }
        // else
        // {
        //     if (buffer_is.all_flag.tx_rx_timeout == true)
        //     {
        //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive timeout\n");
        //         _assert = 2;
        //         return false;
        //     }
        //     if (buffer_is.all_flag.crc_error == true)
        //     {
        //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive crc error\n");
        //         _assert = 3;
        //         return false;
        //     }

        //     if (_param.packet_type == Packet_Type::LORA)
        //     {
        //         if (buffer_is.lora_reg_flag.header_error == true)
        //         {
        //             assert_log(Log_Level::CHIP, __FILE__, __LINE__, "lora receive header error\n");
        //             _assert = 4;
        //             return false;
        //         }
        //     }
        // }

        uint8_t buffer_length = get_rx_buffer_length();
        if (buffer_length == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_buffer_length is empty\n");
            _assert = 1;
            return false;
        }

        if ((length == 0) || (length >= buffer_length))
        {
            if (read_buffer(data, buffer_length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read_buffer fail\n");
                _assert = 2;
                return false;
            }
        }
        else if (length < buffer_length)
        {
            if (read_buffer(data, length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read_buffer fail\n");
                _assert = 2;
                return false;
            }
        }

        return buffer_length;
    }

    bool Sx126x::get_lora_packet_metrics(Packet_Metrics &metrics)
    {
        uint8_t buffer[4] = {0};

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_PACKET_STATUS), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return false;
        }

        metrics.lora.rssi_average = -1.0 * static_cast<float>(buffer[1]) / 2.0;
        metrics.lora.snr = static_cast<float>(buffer[2]) / 4.0;
        metrics.lora.rssi_instantaneous = -1.0 * static_cast<float>(buffer[3]) / 2.0;

        return true;
    }

    bool Sx126x::fix_bw_500khz_sensitivity(bool enable)
    {
        uint8_t buffer[2] = {0};

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::WO_READ_REGISTER), static_cast<uint16_t>(Reg::RW_TX_MODULATION), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return false;
        }

        if (enable == true)
        {
            buffer[1] &= 0xFB;
        }
        else
        {
            buffer[1] |= 0x04;
        }

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER), static_cast<uint16_t>(Reg::RW_TX_MODULATION), buffer[1]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_tx(uint32_t time_out_us)
    {
        if (time_out_us != 0xFFFFFF)
        {
            time_out_us = static_cast<float>(time_out_us) / 15.625;
        }

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(time_out_us >> 16),
                static_cast<uint8_t>(time_out_us >> 8),
                static_cast<uint8_t>(time_out_us),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_TX), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::write_buffer(uint8_t *data, uint8_t length, uint8_t offset)
    {
        // 设置基地址
        if (set_buffer_base_address(0, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_buffer_base_address fail\n");
            return false;
        }

        // 没有校验
        // check_busy();
        // if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_BUFFER), offset, data, length) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        // 有校验
        uint8_t buffer[2 + length] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_BUFFER),
                offset,
            };

        uint8_t assert[2 + length] = {0};

        memcpy(&buffer[2], data, length);

        check_busy();
        if (_bus->write_read(buffer, assert, 2 + length) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        for (uint16_t i = 1; i < (2 + length); i++)
        {
            Cmd_Status buffer_cs = assert_cmd_status(assert[i]);
            if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
            {
                if (i == 1)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "offset data write fail (error code: %#X)\n", assert[i]);
                    return false;
                }
                else
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "data[%d] write fail (error code: %#X)\n", i - 2, assert[i]);
                    return false;
                }
            }
        }

        return true;
    }

    bool Sx126x::send_data(uint8_t *data, uint8_t length, uint32_t time_out_us)
    {
        switch (_param.packet_type)
        {
        case Packet_Type::GFSK:
            if (_param.gfsk.payload_length != length)
            {
                // 重新设置长度
                if (set_gfsk_packet_params(_param.gfsk.preamble_length, _param.gfsk.preamble_detector, length * 8, _param.gfsk.address_comparison,
                                           _param.gfsk.header_type, _param.gfsk.payload_length, _param.gfsk.crc.type, _param.gfsk.whitening) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_packet_params fail\n");
                    return false;
                }
                _param.gfsk.payload_length = length;
            }
            break;
        case Packet_Type::LORA:
            if (_param.lora.payload_length != length)
            {
                // 重新设置长度
                if (set_lora_packet_params(_param.lora.preamble_length, _param.lora.header_type, length, _param.lora.crc_type,
                                           _param.lora.invert_iq) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_packet_params fail\n");
                    return false;
                }
                _param.lora.payload_length = length;
            }
            break;

        default:
            break;
        }

        if (write_buffer(data, length, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_buffer fail\n");
            return false;
        }

        if (_param.packet_type == Packet_Type::LORA)
        {
            if (_param.lora.band_width == Lora_Bw::BW_500000HZ)
            {
                fix_bw_500khz_sensitivity(true);
            }
            else
            {
                fix_bw_500khz_sensitivity(false);
            }
        }

        if (set_tx(time_out_us) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_tx fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_lora_crc_packet_params(Lora_Crc_Type crc_type)
    {
        // 设置CRC
        if (set_lora_packet_params(_param.lora.preamble_length, _param.lora.header_type, _param.lora.payload_length, crc_type,
                                   _param.lora.invert_iq) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lora_packet_params fail\n");
            return false;
        }
        _param.lora.crc_type = crc_type;

        return true;
    }

    bool Sx126x::set_gfsk_modulation_params(float br, Pulse_Shape ps, Gfsk_Bw bw, float freq_deviation)
    {
        if (br < 0.6)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "br=%u out of range\n", br);
            br = 0.6;
        }
        else if (br > 300.0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "br=%u out of range\n", br);
            br = 300.0;
        }

        if (freq_deviation < 0.6)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "freq_deviation=%u out of range\n", freq_deviation);
            freq_deviation = 0.6;
        }
        else if (freq_deviation > 200.0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "freq_deviation=%u out of range\n", freq_deviation);
            freq_deviation = 200.0;
        }

        // 计算原始比特率值
        uint32_t buffer_br = (32.0 * 1000000.0 * 32.0) / (br * 1000.0);

        // 计算原始频率偏差值
        uint32_t buffer_freq_deviation = ((freq_deviation * 1000.0) * static_cast<float>(static_cast<uint32_t>(1) << 25)) / (32.0 * 1000000.0);

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(buffer_br >> 16),
                static_cast<uint8_t>(buffer_br >> 8),
                static_cast<uint8_t>(buffer_br),
                static_cast<uint8_t>(ps),
                static_cast<uint8_t>(bw),
                static_cast<uint8_t>(buffer_freq_deviation >> 16),
                static_cast<uint8_t>(buffer_freq_deviation >> 8),
                static_cast<uint8_t>(buffer_freq_deviation),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_MODULATION_PARAMS), buffer, 8) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.gfsk.bit_rate = br;
        _param.gfsk.pulse_shape = ps;
        _param.gfsk.band_width = bw;
        _param.gfsk.freq_deviation = freq_deviation;

        return true;
    }

    bool Sx126x::set_gfsk_sync_word(uint8_t *sync_word, uint8_t length)
    {
        for (uint8_t i = 0; i < length; i++)
        {
            check_busy();
            if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                            static_cast<uint16_t>(Reg::RW_SYNC_WORD_PROGRAMMING_START) + i, sync_word[i]) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

    bool Sx126x::set_gfsk_packet_params(uint16_t preamble_length, Preamble_Detector preamble_detector_length, uint8_t sync_word_length,
                                        Addr_Comp addr_comp, Gfsk_Header_Type header_type, uint8_t payload_length, Gfsk_Crc_Type crc_type, Whitening whitening)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(preamble_length >> 8),
                static_cast<uint8_t>(preamble_length),
                static_cast<uint8_t>(preamble_detector_length),
                sync_word_length,
                static_cast<uint8_t>(addr_comp),
                static_cast<uint8_t>(header_type),
                payload_length,
                static_cast<uint8_t>(crc_type),
                static_cast<uint8_t>(whitening),
            };

        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_PACKET_PARAMS), buffer, 9) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        _param.gfsk.preamble_length = preamble_length;
        _param.gfsk.address_comparison = addr_comp;
        _param.gfsk.header_type = header_type;
        _param.gfsk.payload_length = payload_length;
        _param.gfsk.crc.type = crc_type;
        _param.gfsk.whitening = whitening;

        return true;
    }

    bool Sx126x::set_gfsk_crc(uint16_t initial, uint16_t polynomial)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(initial >> 8),
                static_cast<uint8_t>(initial),
                static_cast<uint8_t>(polynomial >> 8),
                static_cast<uint8_t>(polynomial),
            };

        for (uint8_t i = 0; i < 4; i++)
        {
            check_busy();
            if (_bus->write(static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                            static_cast<uint16_t>(Reg::RW_CRC_VALUE_PROGRAMMING_START) + i, buffer[i]) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        _param.gfsk.crc.initial = initial;
        _param.gfsk.crc.polynomial = polynomial;

        return true;
    }

    bool Sx126x::config_gfsk_params(float freq_mhz, uint32_t br, Gfsk_Bw bw, float current_limit, int8_t power, float freq_deviation,
                                    uint8_t *sync_word, uint8_t sync_word_length, Pulse_Shape ps, Sf sf, Gfsk_Crc_Type crc_type,
                                    uint16_t crc_initial, uint16_t crc_polynomial, uint16_t preamble_length)
    {
        // 启用13MHz晶振模式
        if (set_standby(Stdby_Config::STDBY_RC) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_standby fail\n");
            return false;
        }

        // 设置包类型
        if (set_packet_type(Packet_Type::GFSK) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_packet_type fail\n");
            return false;
        }

        if (set_cad_params(Cad_Symbol_Num::ON_8_SYMB, static_cast<uint8_t>(sf) + 13, 10, Cad_Exit_Mode::ONLY, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cad_params fail\n");
            return false;
        }
        _param.gfsk.spreading_factor = sf;

        // 校准
        if (calibrate(0b01111111) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "calibrate fail\n");
            return false;
        }

        delay_ms(5);
        check_busy();

        // 检查 calibrate 命令结果
        Cmd_Status buffer_cs = assert_cmd_status(get_status());
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", static_cast<uint8_t>(buffer_cs));
            return false;
        }

        if (set_gfsk_modulation_params(br, ps, bw, freq_deviation) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_modulation_params fail\n");
            return false;
        }

        if ((sync_word == nullptr) || (sync_word_length == 0))
        {
            uint8_t buffer_sync_word[] = {0x01, 0x23, 0x45, 0x67,
                                          0x89, 0xAB, 0xCD, 0xEF};
            sync_word = buffer_sync_word;
            sync_word_length = 8;
        }
        // 设置同步字（还需要同时设置set_gfsk_packet_params）
        if (set_gfsk_sync_word(sync_word, sync_word_length) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_sync_word fail\n");
            return false;
        }

        uint16_t buffer_preamble_detector = std::min(static_cast<uint16_t>(sync_word_length * 8), preamble_length);
        if (buffer_preamble_detector >= 32)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_32BIT;
        }
        else if (buffer_preamble_detector >= 24)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_24BIT;
        }
        else if (buffer_preamble_detector >= 16)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_16BIT;
        }
        else if (buffer_preamble_detector > 0)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_8BIT;
        }
        else
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_OFF;
        }

        // 设置包的参数
        if (set_gfsk_packet_params(preamble_length, _param.gfsk.preamble_detector, sync_word_length * 8, Addr_Comp::FILTERING_DISABLE,
                                   Gfsk_Header_Type::VARIABLE_PACKET, SX126X_MAX_TRANSMIT_BUFFER_SIZE, crc_type, Whitening::NO_ENCODING) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_packet_params fail\n");
            return false;
        }
        _param.gfsk.sync_word.data = sync_word;
        _param.gfsk.sync_word.length = sync_word_length;

        // 设置CRC（还需要同时设置set_gfsk_packet_params）
        if (set_gfsk_crc(crc_initial, crc_polynomial) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_crc fail\n");
            return false;
        }

        // 设置电流限制
        if (set_current_limit(current_limit) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_current_limit fail\n");
            return false;
        }

        // 设置频率
        if (set_frequency(freq_mhz) == false)
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

    bool Sx126x::start_gfsk_transmit(Chip_Mode chip_mode, uint32_t time_out_us, Fallback_Mode fallback_mode, uint16_t preamble_length)
    {
        // 从RX或TX模式退出返回的模式设定
        if (set_rx_tx_fallback_mode(fallback_mode) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_packet_type fail\n");
            return false;
        }

        uint16_t buffer_preamble_detector = std::min(static_cast<uint16_t>(_param.gfsk.sync_word.length * 8), preamble_length);
        if (buffer_preamble_detector >= 32)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_32BIT;
        }
        else if (buffer_preamble_detector >= 24)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_24BIT;
        }
        else if (buffer_preamble_detector >= 16)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_16BIT;
        }
        else if (buffer_preamble_detector > 0)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_8BIT;
        }
        else
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_OFF;
        }

        // 设置包的参数
        if (set_gfsk_packet_params(preamble_length, _param.gfsk.preamble_detector, _param.gfsk.sync_word.length * 8, _param.gfsk.address_comparison,
                                   _param.gfsk.header_type, _param.gfsk.payload_length, _param.gfsk.crc.type, _param.gfsk.whitening) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_packet_params fail\n");
            return false;
        }

        switch (chip_mode)
        {
        case Chip_Mode::RX:
            // 设置为接收模式
            if (set_rx(time_out_us) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rx fail\n");
                return false;
            }
            break;
        case Chip_Mode::TX:
            // 设置为发送模式
            // if (set_tx(time_out_us) == false)
            // {
            //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rx fail\n");
            //     return false;
            // }
            break;

        default:
            break;
        }

        return true;
    }

    uint32_t Sx126x::get_gfsk_packet_status(void)
    {
        uint8_t buffer[4] = {0};

        check_busy();
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GET_PACKET_STATUS), buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        Cmd_Status buffer_cs = assert_cmd_status(buffer[0]);
        if ((buffer_cs != Cmd_Status::RFU) && (buffer_cs != Cmd_Status::CMD_TX_DONE) && (buffer_cs != Cmd_Status::DATA_IS_AVAILABLE_TO_HOST))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_cmd_status fail (error code: %#X)\n", buffer[0]);
            return -1;
        }

        return (static_cast<uint32_t>(buffer[1]) << 16) | (static_cast<uint32_t>(buffer[2]) << 8) | static_cast<uint32_t>(buffer[3]);
    }

    bool Sx126x::assert_gfsk_packet_status(uint32_t assert_status, Gfsk_Packet_Status &status)
    {
        if (assert_status == -1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read error\n");
            return false;
        }

        assert_status >>= 16;

        status.packet_send_done_flag = assert_status & 0B00000001;
        status.packet_receive_done_flag = (assert_status & 0B00000010) >> 1;
        status.abort_error_flag = (assert_status & 0B00000100) >> 2;
        status.length_error_flag = (assert_status & 0B00001000) >> 3;
        status.crc_error_flag = (assert_status & 0B00010000) >> 4;
        status.address_error_flag = (assert_status & 0B00100000) >> 5;
        status.sync_word_flag = (assert_status & 0B01000000) >> 6;
        status.preamble_error_flag = (assert_status & 0B10000000) >> 7;

        return true;
    }

    bool Sx126x::assert_gfsk_packet_metrics(uint32_t assert_metrics, Packet_Metrics &metrics)
    {
        if (assert_metrics == -1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        const uint8_t buffer[2] =
            {
                static_cast<uint8_t>((assert_metrics & 0B00000000000000001111111111111111) >> 8),
                static_cast<uint8_t>(assert_metrics),
            };

        metrics.gfsk.rssi_sync = -1.0 * static_cast<float>(buffer[0]) / 2.0;
        metrics.gfsk.rssi_average = -1.0 * static_cast<float>(buffer[1]) / 2.0;

        return true;
    }

    bool Sx126x::set_gfsk_sync_word_packet_params(uint8_t *sync_word, uint8_t sync_word_length)
    {
        if ((sync_word == nullptr) || (sync_word_length == 0))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "sync_word error\n");
            return false;
        }
        // 设置同步字（还需要同时设置set_gfsk_packet_params）
        if (set_gfsk_sync_word(sync_word, sync_word_length) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_sync_word fail\n");
            return false;
        }

        uint16_t buffer_preamble_detector = std::min(static_cast<uint16_t>(sync_word_length * 8), _param.gfsk.preamble_length);
        if (buffer_preamble_detector >= 32)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_32BIT;
        }
        else if (buffer_preamble_detector >= 24)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_24BIT;
        }
        else if (buffer_preamble_detector >= 16)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_16BIT;
        }
        else if (buffer_preamble_detector > 0)
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_8BIT;
        }
        else
        {
            _param.gfsk.preamble_detector = Preamble_Detector::LENGTH_OFF;
        }

        // 设置包的参数
        if (set_gfsk_packet_params(_param.gfsk.preamble_length, _param.gfsk.preamble_detector, sync_word_length * 8, _param.gfsk.address_comparison,
                                   _param.gfsk.header_type, _param.gfsk.payload_length, _param.gfsk.crc.type, _param.gfsk.whitening) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_packet_params fail\n");
            return false;
        }
        _param.gfsk.sync_word.data = sync_word;
        _param.gfsk.sync_word.length = sync_word_length;

        return true;
    }

    bool Sx126x::set_gfsk_crc_packet_params(Gfsk_Crc_Type crc_type, uint16_t crc_initial, uint16_t crc_polynomial)
    {
        // 设置包的参数
        if (set_gfsk_packet_params(_param.gfsk.preamble_length, _param.gfsk.preamble_detector, _param.gfsk.sync_word.length * 8, _param.gfsk.address_comparison,
                                   _param.gfsk.header_type, _param.gfsk.payload_length, crc_type, _param.gfsk.whitening) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_packet_params fail\n");
            return false;
        }

        // 设置CRC（还需要同时设置set_gfsk_packet_params）
        if (set_gfsk_crc(crc_initial, crc_polynomial) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_gfsk_crc fail\n");
            return false;
        }

        _param.gfsk.crc.type = crc_type;
        _param.gfsk.crc.initial = crc_initial;
        _param.gfsk.crc.polynomial = crc_polynomial;

        return true;
    }

    bool Sx126x::set_irq_pin_mode(Irq_Flag dio1_mode, Irq_Flag dio2_mode, Irq_Flag dio3_mode)
    {
        // 设置接收中断标志
        // 默认设置 irq_mask：RX_DONE, TIMEOUT, CRC_ERR 和 HEADER_ERR
        // 默认设置 diox_mask：RX_DONE（包接收完成后中断）
        // 设置接收中断标志
        // 默认设置 irq_mask：TX_DONE, TIMEOUT
        // 默认设置 diox_mask：TX_DONE（包发送完成后中断）
        if (set_dio_irq_params(0B0000001001100011, static_cast<uint16_t>(dio1_mode), static_cast<uint16_t>(dio2_mode), static_cast<uint16_t>(dio3_mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_dio_irq_params fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::clear_buffer(void)
    {
        std::shared_ptr<uint8_t[]> buffer = std::make_shared<uint8_t[]>(SX126X_MAX_TRANSMIT_BUFFER_SIZE);

        memset(buffer.get(), 0, SX126X_MAX_TRANSMIT_BUFFER_SIZE);

        if (write_buffer(buffer.get(), SX126X_MAX_TRANSMIT_BUFFER_SIZE, 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_buffer fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_tx_continuous_wave(void)
    {
        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_TX_CONTINUOUS_WAVE)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sx126x::set_sleep(Sleep_Mode mode)
    {
        check_busy();
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SET_SLEEP), static_cast<uint8_t>(mode)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 等待Sx126x进入睡眠
        delay_ms(10);

        return true;
    }

}
