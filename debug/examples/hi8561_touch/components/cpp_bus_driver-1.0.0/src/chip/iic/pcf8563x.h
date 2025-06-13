/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-01-22 14:34:51
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

#define PCF8563X_DEVICE_DEFAULT_ADDRESS 0x51

namespace Cpp_Bus_Driver
{
    class Pcf8563x : public IIC_Guide
    {

    private:
        int32_t _rst;

    protected:
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

        static const uint8_t Init_List[];

    public:
        enum class Clock_Frequency
        {
            OUTPUT_OFF = 0,
            OUTPUT_1,
            OUTPUT_32,
            OUTPUT_1024,
            OUTPUT_32768,
        };

        Pcf8563x(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : IIC_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t iic_device_id(void);
        bool set_clock_frequency_output(Clock_Frequency frequency);
        bool clock_run(void);
        bool clock_stop(void);
        uint8_t get_second(void);
        uint8_t get_minute(void);
        uint8_t get_hour(void);
        uint8_t get_day(void);
        uint8_t get_month(void);
        uint8_t get_year(void);
    };
}