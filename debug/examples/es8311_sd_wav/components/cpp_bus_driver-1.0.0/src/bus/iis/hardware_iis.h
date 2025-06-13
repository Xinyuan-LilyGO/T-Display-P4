/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2025-03-12 18:04:26
 * @License: GPL 3.0
 */

#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Iis : public Bus_Iis_Guide
    {
    private:
        int32_t _data;
        int32_t _data_in, _data_out;
        int32_t _ws_lrck, _bclk, _mclk;
        i2s_port_t _port;

        uint16_t _mclk_multiple = -1;
        uint32_t _sample_rate_hz = -1;
        uint8_t _data_bit_width = -1;

        i2s_chan_handle_t _chan_tx_handle = nullptr;
        i2s_chan_handle_t _chan_rx_handle = nullptr;

    public:
        enum class Data_Mode
        {
            INPUT = 0, // 输入模式
            OUTPUT,    // 输出模式

            INPUT_OUTPUT, // 输入输出共有
        };

        Data_Mode _data_mode = Data_Mode::INPUT_OUTPUT;

        // 配置输入或者输出设备
        Hardware_Iis(Data_Mode mode, int32_t data, int32_t ws_lrck, int32_t bclk, int32_t mclk, i2s_port_t port = I2S_NUM_0)
            : _data(data), _ws_lrck(ws_lrck), _bclk(bclk), _mclk(mclk), _port(port)
        {
        }

        // 配置输入和输出设备
        Hardware_Iis(int32_t data_in, int32_t data_out, int32_t ws_lrck, int32_t bclk, int32_t mclk, i2s_port_t port = I2S_NUM_0)
            : _data_in(data_in), _data_out(data_out), _ws_lrck(ws_lrck), _bclk(bclk), _mclk(mclk), _port(port)
        {
        }

        bool begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width) override;

        size_t read(void *data, size_t byte) override;
        size_t write(const void *data, size_t byte) override;

        // bool end() override;
    };
}