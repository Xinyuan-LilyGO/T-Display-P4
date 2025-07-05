/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2025-06-18 14:47:44
 * @License: GPL 3.0
 */
#include "hardware_iis.h"

namespace Cpp_Bus_Driver
{
    bool Hardware_Iis::begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width)
    {
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _ws_lrck: %d\n", _ws_lrck);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _bclk: %d\n", _bclk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _mclk: %d\n", _mclk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: %d\n", mclk_multiple);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config sample_rate_hz: %d hz\n", sample_rate_hz);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_bit_width: %d\n", data_bit_width);

        i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(_port, I2S_ROLE_MASTER);
        // 自动清除DMA缓冲区中的旧数据
        chan_config.auto_clear = true;

        esp_err_t assert = ESP_FAIL;

        if (_data_mode == Data_Mode::INPUT_OUTPUT)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _data_in: %d\n", _data_in);
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _data_out: %d\n", _data_out);

            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: input_output\n");

            const i2s_std_config_t std_config =
                {
                    .clk_cfg =
                        {
                            .sample_rate_hz = sample_rate_hz,
                            .clk_src = I2S_CLK_SRC_DEFAULT,
#if SOC_I2S_HW_VERSION_2
                            .ext_clk_freq_hz = 0,
#endif
                            .mclk_multiple = mclk_multiple,
                        },
                    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(data_bit_width, I2S_SLOT_MODE_STEREO),
                    .gpio_cfg =
                        {
                            .mclk = static_cast<gpio_num_t>(_mclk),
                            .bclk = static_cast<gpio_num_t>(_bclk),
                            .ws = static_cast<gpio_num_t>(_ws_lrck),
                            .dout = static_cast<gpio_num_t>(_data_out),
                            .din = static_cast<gpio_num_t>(_data_in),
                            .invert_flags =
                                {
                                    .mclk_inv = 0,
                                    .bclk_inv = 0,
                                    .ws_inv = 0,
                                },
                        },
                };

            assert = i2s_new_channel(&chan_config, &_chan_tx_handle, &_chan_rx_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                return false;
            }

            assert = i2s_channel_init_std_mode(_chan_tx_handle, &std_config);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                return false;
            }

            assert = i2s_channel_init_std_mode(_chan_rx_handle, &std_config);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                return false;
            }

            assert = i2s_channel_enable(_chan_tx_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                return false;
            }

            assert = i2s_channel_enable(_chan_rx_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                return false;
            }
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _data: %d\n", _data);

            i2s_std_config_t std_config =
                {
                    .clk_cfg =
                        {
                            .sample_rate_hz = sample_rate_hz,
                            .clk_src = I2S_CLK_SRC_DEFAULT,
#if SOC_I2S_HW_VERSION_2
                            .ext_clk_freq_hz = 0,
#endif
                            .mclk_multiple = mclk_multiple,
                        },
                    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(data_bit_width, I2S_SLOT_MODE_STEREO),
                    .gpio_cfg =
                        {
                            .mclk = static_cast<gpio_num_t>(_mclk),
                            .bclk = static_cast<gpio_num_t>(_bclk),
                            .ws = static_cast<gpio_num_t>(_ws_lrck),
                            .dout = I2S_GPIO_UNUSED,
                            .din = I2S_GPIO_UNUSED,
                            .invert_flags =
                                {
                                    .mclk_inv = 0,
                                    .bclk_inv = 0,
                                    .ws_inv = 0,
                                },
                        },
                };

            switch (_data_mode)
            {
            case Data_Mode::INPUT:
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: input\n");

                std_config.gpio_cfg.din = static_cast<gpio_num_t>(_data);

                assert = i2s_new_channel(&chan_config, NULL, &_chan_rx_handle);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                    return false;
                }

                assert = i2s_channel_init_std_mode(_chan_rx_handle, &std_config);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                    return false;
                }

                break;
            case Data_Mode::OUTPUT:
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: output\n");

                std_config.gpio_cfg.dout = static_cast<gpio_num_t>(_data);

                assert = i2s_new_channel(&chan_config, &_chan_tx_handle, NULL);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                    return false;
                }

                assert = i2s_channel_init_std_mode(_chan_tx_handle, &std_config);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                    return false;
                }

                break;

            default:
                break;
            }
        }

        _mclk_multiple = mclk_multiple;
        _sample_rate_hz = sample_rate_hz;
        _data_bit_width = data_bit_width;

        return true;
    }

    size_t Hardware_Iis::read(void *data, size_t byte)
    {
        if (_chan_rx_handle == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "_chan_rx_handle is nullptr \n");
            return false;
        }

        size_t buffer = 0;
        esp_err_t assert = i2s_channel_read(_chan_rx_handle, data, byte, &buffer, DEFAULT_CPP_BUS_DRIVER_IIS_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_read fail (error code: %#X)\n", assert);
            return false;
        }

        return buffer;
    }

    size_t Hardware_Iis::write(const void *data, size_t byte)
    {
        if (_chan_tx_handle == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "_chan_tx_handle is nullptr \n");
            return false;
        }

        size_t buffer = 0;
        esp_err_t assert = i2s_channel_write(_chan_tx_handle, data, byte, &buffer, DEFAULT_CPP_BUS_DRIVER_IIS_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_write fail (error code: %#X)\n", assert);
            return false;
        }

        return buffer;
    }
}
