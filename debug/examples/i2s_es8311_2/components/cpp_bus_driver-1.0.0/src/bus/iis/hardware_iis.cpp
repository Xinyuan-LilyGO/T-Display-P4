/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-02-13 15:52:54
 * @License: GPL 3.0
 */
#include "hardware_iis.h"

namespace Cpp_Bus_Driver
{
    bool Hardware_Iis::begin(i2s_mode_t iis_mode, ad_iis_data_mode_t device_status, i2s_channel_fmt_t channel_mode,
                      int8_t bits_per_sample, int32_t sample_rate) 
    {

        return true;
    }

    // size_t Arduino_HWIIS::Read(void *data, size_t bytes)
    // {
    //     size_t temp_buf;
    //     i2s_read(_iis_num, data, bytes, &temp_buf, portMAX_DELAY);
    //     return temp_buf;
    // }

    // size_t Arduino_HWIIS::Write(const void *data, size_t bytes)
    // {
    //     size_t temp_buf;
    //     i2s_write(_iis_num, data, bytes, &temp_buf, portMAX_DELAY);
    //     return temp_buf;
    // }

    // bool Arduino_HWIIS::end()
    // {
    //     if (i2s_driver_uninstall(_iis_num) != ESP_OK)
    //     {
    //         return false;
    //     }
    //     return true;
    // }
}
