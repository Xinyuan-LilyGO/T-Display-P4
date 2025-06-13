/*
 * @Description: Arduino_HWIIS.cpp
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-12-20 15:46:16
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-12-25 09:19:59
 * @License: GPL 3.0
 */
#include "hwiis.h"

namespace Cpp_Bus_Driver
{
    bool HWIIS::begin(i2s_mode_t iis_mode, ad_iis_data_mode_t device_state, i2s_channel_fmt_t channel_mode,
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
