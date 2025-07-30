/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-06-23 18:02:57
 * @License: GPL 3.0
 */
#include "ecx336c.h"

namespace Cpp_Bus_Driver
{
    bool Ecx336c::begin(int32_t freq_hz)
    {
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

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        // auto buffer = get_device_id();
        // if ((buffer == 0x00) || (buffer == 0xFF))
        // {
        //     assert_log(Log_Level::INFO, __FILE__, __LINE__, "get  ecx336c id fail (error id: %#X)\n", buffer);
        //     return false;
        // }
        // else
        // {
        //     assert_log(Log_Level::INFO, __FILE__, __LINE__, "get  ecx336c id: %#X\n", buffer);
        // }

        return true;
    }

    uint8_t Ecx336c::get_device_id(void)
    {
        return false;
    }

}
