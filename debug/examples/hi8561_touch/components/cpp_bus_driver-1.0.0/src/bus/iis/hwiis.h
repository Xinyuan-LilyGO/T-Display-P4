/*
 * @Description(CN):
 *      这是一个使用硬件IIS的IIS底层驱动文件
 *
 * @Description(EN):
 *      This is a bottom-layer IIS driver file using hardware IIS.
 *
 * @version: V1.0.0
 * @Author: LILYGO_L
 * @Date: 2023-12-20 15:45:17
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2025-01-18 17:17:40
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class HWIIS : public Bus_IIS_Guide
    {
    private:
        int32_t _bclk, _lrck, _data;
        int8_t _port;

    public:
        HWIIS(int8_t data, int8_t lrck, int8_t bclk = DEFAULT_CPP_BUS_DRIVER_VALUE,
              int8_t port = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _data(data), _lrck(lrck), _bclk(bclk), _port(port)
        {
        }

        // bool begin(i2s_mode_t iis_mode, ad_iis_data_mode_t device_status, i2s_channel_fmt_t channel_mode,
        //            int8_t bits_per_sample = DRIVEBUS_DEFAULT_VALUE, int32_t sample_rate = DRIVEBUS_DEFAULT_VALUE) override;

        // size_t Read(void *data, size_t bytes) override;
        // size_t Write(const void *data, size_t bytes) override;

        // bool end() override;
    };
}