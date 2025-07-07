/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-06-18 14:48:25
 * @License: GPL 3.0
 */
#include "hardware_sdio.h"

namespace Cpp_Bus_Driver
{
    bool Hardware_Sdio::begin(int32_t freq_hz)
    {
        if (freq_hz == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            freq_hz = DEFAULT_CPP_BUS_DRIVER_SDIO_FREQ_HZ;
        }
        else if ((freq_hz != SDMMC_FREQ_DEFAULT) || (freq_hz != SDMMC_FREQ_HIGHSPEED) || (freq_hz != SDMMC_FREQ_PROBING) ||
                 (freq_hz != SDMMC_FREQ_52M) || (freq_hz != SDMMC_FREQ_26M))
        {
            freq_hz = DEFAULT_CPP_BUS_DRIVER_SDIO_FREQ_HZ;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config freq_hz: %d\n", freq_hz);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _clk: %d\n", _clk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _cmd: %d\n", _cmd);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d0: %d\n", _d0);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d1: %d\n", _d1);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d2: %d\n", _d2);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d3: %d\n", _d3);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d4: %d\n", _d4);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d5: %d\n", _d5);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d6: %d\n", _d6);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _d7: %d\n", _d7);

        if (_d7 != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _width = 8;
        }
        else if (_d6 != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _width = 7;
        }
        else if (_d5 != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _width = 6;
        }
        else if (_d4 != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _width = 5;
        }
        else if (_d3 != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _width = 4;
        }
        else if (_d2 != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _width = 3;
        }
        else if (_d1 != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _width = 2;
        }
        else
        {
            _width = 1;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_sdio config _width: %d\n", _width);

        sdmmc_slot_config_t sdmmc_slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
        sdmmc_slot_config.clk = static_cast<gpio_num_t>(_clk);
        sdmmc_slot_config.cmd = static_cast<gpio_num_t>(_cmd);
        sdmmc_slot_config.d0 = static_cast<gpio_num_t>(_d0);
        sdmmc_slot_config.d1 = static_cast<gpio_num_t>(_d1);
        sdmmc_slot_config.d2 = static_cast<gpio_num_t>(_d2);
        sdmmc_slot_config.d3 = static_cast<gpio_num_t>(_d3);
        sdmmc_slot_config.d4 = static_cast<gpio_num_t>(_d4);
        sdmmc_slot_config.d5 = static_cast<gpio_num_t>(_d5);
        sdmmc_slot_config.d6 = static_cast<gpio_num_t>(_d6);
        sdmmc_slot_config.d7 = static_cast<gpio_num_t>(_d7);
        sdmmc_slot_config.width = _width;

        esp_err_t assert = sdmmc_host_init();
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_host_init fail (error code: %#X)\n", assert);
            return false;
        }

        assert = sdmmc_host_deinit();
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_host_deinit fail (error code: %#X)\n", assert);
            return false;
        }

        assert = sdmmc_host_init();
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_host_init fail (error code: %#X)\n", assert);
            return false;
        }

        assert = sdmmc_host_init_slot(static_cast<int>(_port), &sdmmc_slot_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_host_init_slot fail (error code: %#X)\n", assert);
            // return false;
        }

        if (_sdio_handle == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "_sdio_handle std::make_unique fail\n");
            return false;
        }

        sdmmc_host_t sdmmc_host = SDMMC_HOST_DEFAULT();
        sdmmc_host.flags |= SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF; // 以任意字节发送（不强制以每4个字节对其发送数据）
        sdmmc_host.slot = static_cast<int>(_port);
        sdmmc_host.max_freq_khz = freq_hz;

        uint8_t timeout_count = 0;
        while (1)
        {
            assert = sdmmc_card_init(&sdmmc_host, _sdio_handle.get());
            if (assert == ESP_OK)
            {
                break;
            }

            timeout_count++;
            if (timeout_count > SDIO_BUS_INIT_TIMEOUT_COUNT)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_card_init fail (error code: %#X)\n", assert);
                return false;
            }
            delay_ms(100);
        }

        assert = sdmmc_io_enable_int(_sdio_handle.get());
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_enable_int fail (error code: %#X)\n", assert);
            // return false;
        }

        // sdmmc_card_print_info(stdout, _sdio_handle.get());

        _freq_hz = freq_hz;

        return true;
    }

    bool Hardware_Sdio::wait_interrupt(uint32_t timeout_ms)
    {
        esp_err_t assert = sdmmc_io_wait_int(_sdio_handle.get(), pdMS_TO_TICKS(timeout_ms));
        if (assert == ESP_ERR_TIMEOUT)
        {
            // assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_wait_int timeout \n");
            return false;
        }
        else if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_wait_int fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Sdio::read(uint32_t function, uint32_t write_c32, void *data, size_t byte)
    {
        esp_err_t assert = sdmmc_io_read_bytes(_sdio_handle.get(), function, write_c32, data, byte);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_read_bytes fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Sdio::read(uint32_t function, uint32_t write_c32, uint8_t *data)
    {
        esp_err_t assert = sdmmc_io_read_byte(_sdio_handle.get(), function, write_c32, data);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_read_byte fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Sdio::read_block(uint32_t function, uint32_t write_c32, void *data, size_t byte)
    {
        esp_err_t assert = sdmmc_io_read_blocks(_sdio_handle.get(), function, write_c32, data, byte);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_read_blocks fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Sdio::write(uint32_t function, uint32_t write_c32, const void *data, size_t byte)
    {
        esp_err_t assert = sdmmc_io_write_bytes(_sdio_handle.get(), function, write_c32, data, byte);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_write_bytes fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Sdio::write(uint32_t function, uint32_t write_c32, uint8_t data, uint8_t *read_d8_verify)
    {
        esp_err_t assert = sdmmc_io_write_byte(_sdio_handle.get(), function, write_c32, data, read_d8_verify);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_write_byte fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Sdio::write_block(uint32_t function, uint32_t write_c32, const void *data, size_t byte)
    {
        esp_err_t assert = sdmmc_io_write_blocks(_sdio_handle.get(), function, write_c32, data, byte);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "sdmmc_io_write_blocks fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }
}