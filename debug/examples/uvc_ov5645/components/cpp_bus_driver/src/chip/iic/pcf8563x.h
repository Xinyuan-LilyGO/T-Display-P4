/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-06-27 10:50:16
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define PCF8563X_DEVICE_DEFAULT_ADDRESS 0x51

    class Pcf8563x : public Iic_Guide
    {
    private:
        enum class Cmd
        {
            RO_DEVICE_ID = 0x00,

            RW_CONTROL_STATUS_1 = 0x00,
            RW_CONTROL_STATUS_2,
            RW_VL_SECONDS,
            RW_MINUTES,
            RW_HOURS,
            RW_DAYS,
            RW_WEEKDAYS,
            RW_CENTURY_MONTHS,
            RW_YEARS,
            RW_MINUTE_ALARM,
            RW_HOUR_ALARM,
            RW_DAY_ALARM,
            RW_WEEKDAY_ALARM,
            RW_CLKOUT_CONTROL,
            RW_TIMER_CONTROL,
            RW_TIMER,
        };

    public:
        enum class Week
        {
            SUNDAY = 0X00,
            MONDAY,
            TUESDAY,
            WEDNESDAY,
            THURSDAY,
            FRIDAY,
            SATURDAY,
        };

    private:
        static constexpr const uint8_t _init_list[] =
            {
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_CLKOUT_CONTROL), 0B00000000};

        int32_t _rst;

    public:
        enum class Out_Freq
        {
            CLOCK_OFF = 0,
            CLOCK_1HZ,
            CLOCK_32HZ,
            CLOCK_1024HZ,
            CLOCK_32768HZ,
        };

        enum class Timer_Freq
        {
            CLOCK_4096HZ = 0,
            CLOCK_64HZ,
            CLOCK_1HZ,
            CLOCK_1_DIVIDED_BY_60HZ,
        };

        struct Time_Alarm
        {
            struct
            {
                uint8_t value = 0;       // 分钟报警值（0~59）
                bool alarm_flag = false; // 报警启用标志
            } minute;

            struct
            {
                uint8_t value = 0;       // 小时报警值（0~23）
                bool alarm_flag = false; // 报警启用标志
            } hour;
            struct
            {
                uint8_t value = 0;       // 天报警值（1~31）
                bool alarm_flag = false; // 报警启用标志
            } day;

            struct
            {
                Week value = Week::SUNDAY; // 周报警值，使用Week::配置
                bool alarm_flag = false;   // 报警启用标志
            } week;
        };

        struct Time
        {
            uint8_t second = -1;
            uint8_t minute = -1;
            uint8_t hour = -1;
            uint8_t day = -1;
            Week week = Week::SUNDAY;
            uint8_t month = -1;
            uint8_t year = -1;
        };

        Pcf8563x(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);
        /**
         * @brief 设置CLKOUT引脚输出频率
         * @param freq_hz 使用Clock_Frequency::配置
         * @return
         * @Date 2025-02-27 15:46:00
         */
        bool set_clock_frequency_output(Out_Freq freq_hz);

        /**
         * @brief 设置时钟启动
         *  @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-02-27 15:50:50
         */
        bool set_clock(bool enalbe);

        /**
         * @brief 检查时钟数据完整性
         * @return [0]：不保证时钟信息的完整 [1]：保证时钟完整
         * @Date 2025-02-27 15:35:07
         */
        bool check_clock_integrity_flag(void);

        /**
         * @brief 清除时钟数据完整性标志，设置0为时钟完整
         * @return
         * @Date 2025-02-27 15:36:50
         */
        bool clear_clock_integrity_flag(void);

        uint8_t get_second(void);
        uint8_t get_minute(void);
        uint8_t get_hour(void);
        uint8_t get_day(void);
        uint8_t get_week(void);
        uint8_t get_month(void);
        uint8_t get_year(void);
        bool get_time(Time &time);

        bool set_second(uint8_t second);
        bool set_minute(uint8_t minute);
        bool set_hour(uint8_t hour);
        bool set_day(uint8_t day);
        bool set_week(Week week);
        bool set_month(uint8_t month);
        bool set_year(uint8_t year);
        bool set_time(Time time);

        /**
         * @brief 停止定时器
         * @return
         * @Date 2025-02-27 16:02:39
         */
        bool stop_timer(void);

        /**
         * @brief 开启定时器
         * @param n_value 定时器值，与freq搭配使用（定时的值 （单位：秒） = n_value / freq（单位：赫兹））
         * @param freq_hz 使用Timer_Freq::配置，选择的频率越高定时的时间越精准
         * @return
         * @Date 2025-02-27 16:12:48
         */
        bool run_timer(uint8_t n_value, Timer_Freq freq_hz);

        /**
         * @brief 检查定时器标志和中断
         * @return
         * @Date 2025-02-27 16:53:51
         */
        bool check_timer_flag(void);

        /**
         * @brief 清除定时器标志和中断
         * @return
         * @Date 2025-02-27 16:54:04
         */
        bool clear_timer_flag(void);

        /**
         * @brief 停止预定时间报警
         * @return
         * @Date 2025-02-27 17:30:19
         */
        bool stop_scheduled_alarm(void);

        /**
         * @brief 开启预定时间报警
         * @param alarm 使用Time_Alarm::配置
         * @return
         * @Date 2025-02-27 17:31:35
         */
        bool run_scheduled_alarm(Time_Alarm alarm);

        /**
         * @brief 检查预定时间报警标志和中断
         * @return
         * @Date 2025-02-27 17:55:50
         */
        bool check_scheduled_alarm_flag(void);

        /**
         * @brief 清除预定时间报警标志和中断
         * @return
         * @Date 2025-02-27 17:56:56
         */
        bool clear_scheduled_alarm_flag(void);
    };
}