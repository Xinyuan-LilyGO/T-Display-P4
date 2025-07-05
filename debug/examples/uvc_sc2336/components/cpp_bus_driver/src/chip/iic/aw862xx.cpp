/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-26 11:13:26
 * @LastEditTime: 2025-06-12 10:48:50
 * @License: GPL 3.0
 */
#include "aw862xx.h"

namespace Cpp_Bus_Driver
{
    bool Aw862xx::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        uint8_t buffer = get_device_id();
        if (buffer == static_cast<uint8_t>(DEFAULT_CPP_BUS_DRIVER_VALUE))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get aw862xx id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get aw862xx id: %#X\n", buffer);
        }

        return true;
    }

    uint8_t Aw862xx::get_device_id(void)
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
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_SRST), static_cast<uint8_t>(0xAA)) == false)
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

    bool Aw862xx::set_play_mode(Play_Mode mode)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_PLAYCFG3), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11111100) | static_cast<uint8_t>(mode);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_PLAYCFG3), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_go_flag(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_PLAYCFG4), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11111110) | 0B00000001;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_PLAYCFG4), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    Aw862xx::Global_Status Aw862xx::get_global_status(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GLBRD5), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return Global_Status::FALSE;
        }

        buffer &= 0B00001111;

        switch (buffer)
        {
        case static_cast<uint8_t>(Global_Status::STANDBY):
            break;
        case static_cast<uint8_t>(Global_Status::CONT):
            break;
        case static_cast<uint8_t>(Global_Status::RAM):
            break;
        case static_cast<uint8_t>(Global_Status::RTP):
            break;
        case static_cast<uint8_t>(Global_Status::TRIG):
            break;
        case static_cast<uint8_t>(Global_Status::BRAKE):
            break;

        default:
            // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "unknown command (error code: %#X)\n", buffer);
            return Global_Status::FALSE;
        }

        return static_cast<Global_Status>(buffer);
    }

    bool Aw862xx::run_rtp_playback_waveform(const uint8_t *waveform_data, size_t length)
    {
        if (set_play_mode(Play_Mode::RTP) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_play_mode fail\n");
            return false;
        }

        if (set_go_flag() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_go_flag fail\n");
            return false;
        }

        // delay_us(1000);

        uint8_t timeout_count_buffer = 0;
        while (1)
        {
            if (get_global_status() == Global_Status::RTP)
            {
                break;
            }

            timeout_count_buffer++;
            if (timeout_count_buffer > 100)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rtp mode switch timeout\n");
                return false;
            }
            delay_us(1000);
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RTPDATA), waveform_data, length) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_waveform_data_sample_rate(Sample_Rate rate)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSCTRL2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11111100) | static_cast<uint8_t>(rate);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSCTRL2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_playing_changed_gain_bypass(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSCTRL7), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(enable) << 6);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSCTRL7), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_d2s_gain(D2s_Gain gain)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSCTRL7), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11000000) | (static_cast<uint8_t>(gain));
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSCTRL7), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_lra_osc_frequency(uint8_t freq_hz)
    {
        if (freq_hz > 63)
        {
            freq_hz = 63;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_TRIMCFG3), freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_f0_detection_mode(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTCFG1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enable) << 3);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_track_switch(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTCFG6), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG6), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_auto_brake_stop(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_PLAYCFG3), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(enable) << 2);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_PLAYCFG3), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_cont_drive_1_level(uint8_t level)
    {
        if (level > 127)
        {
            level = 127;
        }
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTCFG6), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B10000000) | level;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG6), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_cont_drive_2_level(uint8_t level)
    {
        if (level > 127)
        {
            level = 127;
        }
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTCFG7), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B10000000) | level;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG7), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_cont_drive_1_times(uint8_t times)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG8), times) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_cont_drive_2_times(uint8_t times)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG9), times) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_cont_track_margin(uint8_t value)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG11), value) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_cont_drive_width(uint8_t value)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTCFG3), value) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint32_t Aw862xx::get_f0_detection(void)
    {
        if (set_lra_osc_frequency(0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lra_osc_frequency fail\n");
            return -1;
        }

        if (set_play_mode(Play_Mode::CONT) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_play_mode fail\n");
            return -1;
        }

        if (set_f0_detection_mode(true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_f0_detection_mode fail\n");
            return -1;
        }

        if (set_track_switch(true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_track_switch fail\n");
            return -1;
        }

        if (set_auto_brake_stop(true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_auto_brake_stop fail\n");
            return -1;
        }

        if (set_cont_drive_1_level(127) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cont_drive_1_level fail\n");
            return -1;
        }

        uint8_t cont_drive_2_level_buffer = ((_f0_value < 1800) ? 1809920 : 1990912) / 1000 * 61000000;

        if (set_cont_drive_1_level(cont_drive_2_level_buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cont_drive_1_level fail\n");
            return -1;
        }

        if (set_cont_drive_1_times(4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cont_drive_1_times fail\n");
            return -1;
        }

        if (set_cont_drive_2_times(6) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cont_drive_2_times fail\n");
            return -1;
        }

        if (set_cont_track_margin(15) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cont_track_margin fail\n");
            return -1;
        }

        int32_t cont_drive_width_buffer = 240000 / _f0_value - 8 - 8 - 15;

        if (cont_drive_width_buffer < 0)
        {
            cont_drive_width_buffer = 0;
        }
        else if (cont_drive_width_buffer > 255)
        {
            cont_drive_width_buffer = 255;
        }

        if (set_cont_drive_width(cont_drive_width_buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_cont_drive_width fail\n");
            return -1;
        }

        if (set_go_flag() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_go_flag fail\n");
            return -1;
        }

        delay_ms(300);

        uint8_t buffer[2] = {0};

        for (uint8_t i = 0; i < 2; i++)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_CONTRD14) + i, &buffer[i]) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return -1;
            }
        }

        if (set_f0_detection_mode(false) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_f0_detection_mode fail\n");
            return -1;
        }

        if (set_auto_brake_stop(false) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_auto_brake_stop fail\n");
            return -1;
        }

        return 384000 * 10 / ((static_cast<uint32_t>(buffer[0]) << 8) | buffer[1]);
    }

    bool Aw862xx::set_f0_calibrate(uint32_t f0_value)
    {
        uint32_t f0_calibrate_value_min_buffer = _f0_value * 93 / 100;
        uint32_t f0_calibrate_value_max_buffer = _f0_value * 107 / 100;

        if (f0_value < f0_calibrate_value_min_buffer || f0_value > f0_calibrate_value_max_buffer)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "f0_value fail\n");
            return false;
        }

        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "f0_value: %d\n", f0_value);
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_f0_value: %d\n", _f0_value);

        int32_t f0_calibrate_step_buffer = 100000 * ((static_cast<int32_t>(f0_value) - static_cast<int32_t>(_f0_value))) /
                                           ((static_cast<int32_t>(_f0_value) * 24));

        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "f0_calibrate_step_buffer_1: %d\n", f0_calibrate_step_buffer);

        if (f0_calibrate_step_buffer >= 0)
        {
            if (f0_calibrate_step_buffer % 10 >= 5)
            {
                f0_calibrate_step_buffer = 32 + (f0_calibrate_step_buffer / 10 + 1);
            }
            else
            {
                f0_calibrate_step_buffer = 32 + f0_calibrate_step_buffer / 10;
            }
        }
        else
        {
            if (f0_calibrate_step_buffer % 10 <= -5)
            {
                f0_calibrate_step_buffer = 32 + (f0_calibrate_step_buffer / 10 - 1);
            }
            else
            {
                f0_calibrate_step_buffer = 32 + f0_calibrate_step_buffer / 10;
            }
        }

        int8_t f0_calibrate_lra_buffer = 0;

        if (f0_calibrate_step_buffer > 31)
        {
            f0_calibrate_lra_buffer = static_cast<int8_t>(f0_calibrate_step_buffer) - 32;
        }
        else
        {
            f0_calibrate_lra_buffer = static_cast<int8_t>(f0_calibrate_step_buffer) + 32;
        }

        if (set_lra_osc_frequency(f0_calibrate_lra_buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_lra_osc_frequency fail\n");
            return false;
        }

        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "f0_calibrate_step_buffer_2: %d\n", f0_calibrate_step_buffer);
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "f0_calibrate_lra_buffer: %d\n", f0_calibrate_lra_buffer);

        _f0_value = f0_value;

        return true;
    }

    bool Aw862xx::get_system_status(System_Status &status)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_SYSST), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        status.uvls_flag = (buffer & 0B00100000) > 5;
        status.rtp_fifo_empty = (buffer & 0B00010000) > 4;
        status.rtp_fifo_full = (buffer & 0B00001000) > 3;
        status.over_current_flag = (buffer & 0B00000100) > 2;
        status.over_temperature_flag = (buffer & 0B00000010) > 1;
        status.playback_flag = buffer & 0B00000001;

        return true;
    }

    bool Aw862xx::set_clock(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSCTRL1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(enable) << 3);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSCTRL1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_rrt_mode_gain(uint8_t gain)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_PLAYCFG2), static_cast<uint8_t>(gain)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::init_ram_mode(const uint8_t *waveform_data, size_t length)
    {
        if (set_clock(true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_clock fail\n");
            return false;
        }

        delay_us(1000);

        uint8_t buffer = 0;
        // 默认使用2KB FIFO，1KB RAM波形储存空间
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_RTPCFG1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11110000) | 0x08;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RTPCFG1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RTPCFG2), static_cast<uint8_t>(0)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RAMADDRH), static_cast<uint8_t>(0x08)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RAMADDRL), static_cast<uint8_t>(0)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 写入RAM波形数据，上电时只需写入一次
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_RAMADATA), waveform_data, length) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (set_clock(false) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_clock fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::run_ram_playback_waveform(uint8_t waveform_sequence_number, uint8_t waveform_playback_count,
                                            uint8_t gain, bool auto_brake, bool gain_bypass)
    {
        if (waveform_playback_count > 15)
        {
            waveform_playback_count = 15;
        }

        // 设置RAM #x(x=waveform_sequence_number) waveform
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_WAVCFG1), static_cast<uint8_t>(waveform_sequence_number)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 设置停止
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_WAVCFG2), static_cast<uint8_t>(0)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        uint8_t buffer = 0;
        // 设置播放次数（15为无限循环）
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_WAVCFG9), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B00001111) | (static_cast<uint8_t>(waveform_playback_count) << 4);
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_WAVCFG9), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (set_play_mode(Play_Mode::RAM) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_play_mode fail\n");
            return false;
        }

        // 自动制动
        if (set_auto_brake_stop(auto_brake) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_auto_brake_stop fail\n");
            return false;
        }

        if (set_playing_changed_gain_bypass(gain_bypass) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_playing_changed_gain_bypass fail\n");
            return false;
        }

        if (set_rrt_mode_gain(gain) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_rrt_mode_gain fail\n");
            return false;
        }

        // 开始播放
        if (set_go_flag() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_go_flag fail\n");
            return false;
        }
        return true;
    }

    bool Aw862xx::set_stop_flag(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_PLAYCFG4), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11111101) | 0B00000010;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_PLAYCFG4), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::set_force_enter_mode(Force_Mode mode, bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSCTRL2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        switch (mode)
        {
        case Force_Mode::ACTIVE:
            buffer = (buffer & 0B01111111) | (static_cast<uint8_t>(enable) << 7);
            break;
        case Force_Mode::STANDBY:
            buffer = (buffer & 0B10111111) | (static_cast<uint8_t>(enable) << 6);
            break;

        default:
            break;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSCTRL2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw862xx::stop_ram_playback_waveform(void)
    {
        // 停止播放
        if (set_stop_flag() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_stop_flag fail\n");
            return false;
        }

        // uint8_t timeout_count_buffer = 0;
        // while (1)
        // {
        //     if (get_global_status() == Global_Status::STANDBY)
        //     {
        //         break;
        //     }

        //     timeout_count_buffer++;
        //     if (timeout_count_buffer > 200)
        //     {
        //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "standby mode switch timeout\n");

        //         if (set_force_enter_mode(Force_Mode::STANDBY, true) == false)
        //         {
        //             assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_force_enter_mode fail\n");
        //             return false;
        //         }

        //         timeout_count_buffer = 0;
        //         while (1)
        //         {
        //             if (get_global_status() == Global_Status::STANDBY)
        //             {
        //                 break;
        //             }

        //             timeout_count_buffer++;
        //             if (timeout_count_buffer > 100)
        //             {
        //                 assert_log(Log_Level::CHIP, __FILE__, __LINE__, "force enter standby mode timeout\n");

        //                 if (set_force_enter_mode(Force_Mode::STANDBY, false) == false)
        //                 {
        //                     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_force_enter_mode fail\n");
        //                     return false;
        //                 }

        //                 return false;
        //             }

        //             delay_us(1000);
        //         }

        //         if (set_force_enter_mode(Force_Mode::STANDBY, false) == false)
        //         {
        //             assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_force_enter_mode fail\n");
        //             return false;
        //         }

        //         break;
        //     }

        //     delay_us(1000);
        // }

        if (set_force_enter_mode(Force_Mode::STANDBY, true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_force_enter_mode fail\n");
            return false;
        }

        uint8_t timeout_count_buffer = 0;
        while (1)
        {
            if (get_global_status() == Global_Status::STANDBY)
            {
                break;
            }

            timeout_count_buffer++;
            if (timeout_count_buffer > 100)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "force enter standby mode timeout\n");

                if (set_force_enter_mode(Force_Mode::STANDBY, false) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_force_enter_mode fail\n");
                    return false;
                }

                return false;
            }

            delay_us(1000);
        }

        if (set_force_enter_mode(Force_Mode::STANDBY, false) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_force_enter_mode fail\n");
            return false;
        }

        return true;
    }

}
