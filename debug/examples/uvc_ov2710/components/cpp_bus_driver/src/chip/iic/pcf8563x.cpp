/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:51
 * @LastEditTime: 2025-06-16 18:10:50
 * @License: GPL 3.0
 */
#include "pcf8563x.h"

namespace Cpp_Bus_Driver
{
    bool Pcf8563x::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        uint8_t buffer = get_device_id();
        if (buffer == static_cast<uint8_t>(DEFAULT_CPP_BUS_DRIVER_VALUE))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get pcf8563x id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get pcf8563x id: %#X\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        clear_clock_integrity_flag();

        return true;
    }

    uint8_t Pcf8563x::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Pcf8563x::set_clock_frequency_output(Out_Freq freq_hz)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CLKOUT_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        switch (freq_hz)
        {
        case Out_Freq::CLOCK_OFF:
            buffer &= 0B01111111;
            break;
        case Out_Freq::CLOCK_1HZ:
            buffer |= 0B10000011;
            break;
        case Out_Freq::CLOCK_32HZ:
            buffer = (buffer & 0B01111100) | 0B10000010;
            break;
        case Out_Freq::CLOCK_1024HZ:
            buffer = (buffer & 0B01111100) | 0B10000001;
            break;
        case Out_Freq::CLOCK_32768HZ:
            buffer = (buffer & 0B01111100) | 0B10000000;
            break;

        default:
            break;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CLKOUT_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_clock(bool enalbe)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_1), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        if (enalbe == true)
        {
            buffer &= 0B11011111; // 开启时钟
        }
        else
        {
            buffer |= 0B00100000; // 停止时钟
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_1), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        return true;
    }

    bool Pcf8563x::check_clock_integrity_flag(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_VL_SECONDS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return !((buffer & 0B10000000) >> 7);
    }

    bool Pcf8563x::clear_clock_integrity_flag(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_VL_SECONDS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer &= 0B01111111;

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_VL_SECONDS), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint8_t Pcf8563x::get_second(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_VL_SECONDS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B01110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    uint8_t Pcf8563x::get_minute(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_MINUTES), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B01110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    uint8_t Pcf8563x::get_hour(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_HOURS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B00110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    uint8_t Pcf8563x::get_day(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_DAYS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B00110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    uint8_t Pcf8563x::get_week(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_WEEKDAYS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (buffer & 0B00000111);
    }

    uint8_t Pcf8563x::get_month(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CENTURY_MONTHS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B00010000) >> 4) * 10) + (buffer & 0B00001111);
    }

    uint8_t Pcf8563x::get_year(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_YEARS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (((buffer & 0B11110000) >> 4) * 10) + (buffer & 0B00001111);
    }

    bool Pcf8563x::get_time(Time &time)
    {
        uint8_t buffer[7] = {0};

        for (uint8_t i = 0; i < 7; i++)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RW_VL_SECONDS) + i, &buffer[i]) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }

        time.second = (((buffer[0] & 0B01110000) >> 4) * 10) + (buffer[0] & 0B00001111);
        time.minute = (((buffer[1] & 0B01110000) >> 4) * 10) + (buffer[1] & 0B00001111);
        time.hour = (((buffer[2] & 0B00110000) >> 4) * 10) + (buffer[2] & 0B00001111);
        time.day = (((buffer[3] & 0B00110000) >> 4) * 10) + (buffer[3] & 0B00001111);
        time.week = static_cast<Week>(buffer[4] & 0B00000111);
        time.month = (((buffer[5] & 0B00010000) >> 4) * 10) + (buffer[5] & 0B00001111);
        time.year = (((buffer[6] & 0B11110000) >> 4) * 10) + (buffer[6] & 0B00001111);

        return true;
    }

    bool Pcf8563x::set_second(uint8_t second)
    {
        if (second > 59)
        {
            second = 59;
        }

        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_VL_SECONDS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B10000000) | (((second / 10) << 4) | (second % 10));

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_VL_SECONDS), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_minute(uint8_t minute)
    {
        if (minute > 59)
        {
            minute = 59;
        }

        uint8_t buffer = (((minute / 10) << 4) | (minute % 10));

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MINUTES), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_hour(uint8_t hour)
    {
        if (hour > 23)
        {
            hour = 23;
        }

        uint8_t buffer = (((hour / 10) << 4) | (hour % 10));

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_HOURS), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_day(uint8_t day)
    {
        if (day > 31)
        {
            day = 31;
        }
        else if (day < 1)
        {
            day = 1;
        }

        uint8_t buffer = (((day / 10) << 4) | (day % 10));

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_DAYS), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_week(Week week)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_WEEKDAYS), static_cast<uint8_t>(week)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_month(uint8_t month)
    {
        if (month > 12)
        {
            month = 12;
        }
        else if (month < 1)
        {
            month = 1;
        }

        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CENTURY_MONTHS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B11100000) | (((month / 10) << 4) | (month % 10));

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CENTURY_MONTHS), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_year(uint8_t year)
    {
        if (year > 99)
        {
            year = 99;
        }

        uint8_t buffer = (((year / 10) << 4) | (year % 10));

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_YEARS), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::set_time(Time time)
    {
        if (time.second > 59)
        {
            time.second = 59;
        }
        if (time.minute > 59)
        {
            time.minute = 59;
        }
        if (time.hour > 23)
        {
            time.hour = 23;
        }
        if (time.day > 31)
        {
            time.day = 31;
        }
        else if (time.day < 1)
        {
            time.day = 1;
        }
        if (time.month > 12)
        {
            time.month = 12;
        }
        else if (time.month < 1)
        {
            time.month = 1;
        }
        if (time.year > 99)
        {
            time.year = 99;
        }

        uint8_t buffer[7] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_VL_SECONDS), &buffer[0]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CENTURY_MONTHS), &buffer[5]) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer[0] = (buffer[0] & 0B10000000) | (((time.second / 10) << 4) | (time.second % 10));
        buffer[1] = (((time.minute / 10) << 4) | (time.minute % 10));
        buffer[2] = (((time.hour / 10) << 4) | (time.hour % 10));
        buffer[3] = (((time.day / 10) << 4) | (time.day % 10));
        buffer[4] = static_cast<uint8_t>(time.week);
        buffer[5] = (buffer[5] & 0B11100000) | (((time.month / 10) << 4) | (time.month % 10));
        buffer[6] = (((time.year / 10) << 4) | (time.year % 10));

        for (uint8_t i = 0; i < 7; i++)
        {
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_VL_SECONDS) + i, buffer[i]) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

    bool Pcf8563x::stop_timer(void)
    {
        uint8_t buffer = 0;

        // 关闭定时器
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_TIMER_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 清除定时器TF标志位并关闭定时器TIE外部中断
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0B11111010;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        return true;
    }

    bool Pcf8563x::run_timer(uint8_t n_value, Timer_Freq freq_hz)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_TIMER), n_value) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        uint8_t buffer = 0B10000000 | static_cast<uint8_t>(freq_hz);

        // 开启定时器并设置频率
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_TIMER_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 清除定时器TF标志位并开启定时器TIE外部中断
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11111010) | 0B00000001;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::check_timer_flag(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return (buffer & 0B00000100) >> 2;
    }

    bool Pcf8563x::clear_timer_flag(void)
    {
        uint8_t buffer = 0;
        // 清除定时器TF标志位
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0B11111011;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::stop_scheduled_alarm(void)
    {
        uint8_t buffer = 0B10000000;

        // 关闭报警
        for (uint8_t i = 0; i < 4; i++)
        {
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_MINUTE_ALARM) + i, buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        // 清除报警AF标志位并关闭报警AIE外部中断
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0B11110101;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::run_scheduled_alarm(Time_Alarm alarm)
    {
        uint8_t buffer = 0;

        if (alarm.minute.alarm_flag == true)
        {
            if (alarm.minute.value > 59)
            {
                alarm.minute.value = 59;
            }
            buffer = (((alarm.minute.value / 10) << 4) | (alarm.minute.value % 10));
        }
        else
        {
            buffer = 0B10000000;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MINUTE_ALARM), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (alarm.hour.alarm_flag == true)
        {
            if (alarm.hour.value > 23)
            {
                alarm.hour.value = 23;
            }
            buffer = (((alarm.hour.value / 10) << 4) | (alarm.hour.value % 10));
        }
        else
        {
            buffer = 0B10000000;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_HOUR_ALARM), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (alarm.day.alarm_flag == true)
        {
            if (alarm.day.value > 31)
            {
                alarm.day.value = 31;
            }
            else if (alarm.day.value < 1)
            {
                alarm.day.value = 1;
            }
            buffer = (((alarm.day.value / 10) << 4) | (alarm.day.value % 10));
        }
        else
        {
            buffer = 0B10000000;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_DAY_ALARM), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (alarm.week.alarm_flag == true)
        {
            buffer = static_cast<uint8_t>(alarm.week.value);
        }
        else
        {
            buffer = 0B10000000;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_WEEKDAY_ALARM), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 清除预定时间报警AF标志位并开启预定时间报警AIE外部中断
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer = (buffer & 0B11110101) | 0B00000010;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Pcf8563x::check_scheduled_alarm_flag(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return (buffer & 0B00001000) >> 3;
    }

    bool Pcf8563x::clear_scheduled_alarm_flag(void)
    {
        uint8_t buffer = 0;
        // 清除预定时间报警AF标志位
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        buffer &= 0B11110111;
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_2), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
