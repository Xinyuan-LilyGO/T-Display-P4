/*
 * @Description: CPP_HWIIC.cpp
 * @version: V1.0.1
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:46:16
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2025-02-08 09:18:33
 * @License: GPL 3.0
 */
#include "hwspi.h"

namespace Cpp_Bus_Driver
{
    bool HWSPI::begin(int32_t frequency, int32_t cs)
    {
        _frequency = (frequency == DEFAULT_CPP_BUS_DRIVER_VALUE) ? DEFAULT_CPP_BUS_DRIVER_SPI_FREQUENCY : frequency;
        _cs = cs;

        if ((_port < SPI2_HOST) || (_port > SPI_HOST_MAX)) // SPI1_HOST已被系统占用
        {
            _port = SPI2_HOST;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring mosi_io_num: %d\n", _mosi);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring sclk_io_num: %d\n", _sclk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring miso_io_num: %d\n", _miso);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring cs_io_num: %d\n", _cs);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring spi_port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring spi_mode: %d\n", _mode);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring clock_speed_hz: %d hz\n", _frequency);

        spi_bus_config_t bus_config =
            {
                .mosi_io_num = _mosi,
                .miso_io_num = _miso,
                .sclk_io_num = _sclk,
                .quadwp_io_num = -1, // WP引脚不设置，这个引脚配置Quad SPI的时候才有用
                .quadhd_io_num = -1, // HD引脚不设置，这个引脚配置Quad SPI的时候才有用
                .data4_io_num = -1,
                .data5_io_num = -1,
                .data6_io_num = -1,
                .data7_io_num = -1,
                .data_io_default_level = 0,
                .max_transfer_sz = 0,
                .flags = SPICOMMON_BUSFLAG_MASTER,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = static_cast<uint32_t>(NULL),
            };

        esp_err_t assert = spi_bus_initialize(static_cast<spi_host_device_t>(_port), &bus_config, SPI_DMA_CH_AUTO);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_bus_initialize fail (error code: %#X)\n", assert);
            return false;
        }

        spi_device_interface_config_t device_config = {
            .command_bits = 0,
            .address_bits = 0,
            .dummy_bits = 0, // 无虚拟位
            .mode = _mode,
            .clock_source = SPI_CLK_SRC_DEFAULT, // 默认时钟源
            .duty_cycle_pos = 128,               // 50% 占空比
            .cs_ena_pretrans = 1,                // 在数据传输开始之前，片选信号（CS）应该提前多少个SPI位周期被激活
            .cs_ena_posttrans = 1,               // 在数据传输结束后，片选信号（CS）应该保持激活状态多少个SPI位周期
            .clock_speed_hz = _frequency,
            .input_delay_ns = 0, // 无输入延迟
            .spics_io_num = _cs,
            .flags = static_cast<uint32_t>(NULL),
            // .flags = SPI_DEVICE_TXBIT_LSBFIRST, // 启用LSB优先发送
            .queue_size = 1,
            .pre_cb = NULL,  // 无传输前回调
            .post_cb = NULL, // 无传输后回调
        };
        assert = spi_bus_add_device(static_cast<spi_host_device_t>(_port), &device_config, &_spi_device);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_bus_add_device fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool HWSPI::write(const void *data, size_t byte)
    {
        spi_transaction_t buffer =
            {
                .flags = static_cast<uint32_t>(NULL),
                .cmd = 0,
                .addr = 0,
                .length = byte * 8,
                .rxlength = 0,
                .user = (void *)0,
                .tx_buffer = data,
                .rx_buffer = NULL,
            };

        esp_err_t assert = spi_device_polling_transmit(_spi_device, &buffer);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_device_polling_transmit fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool HWSPI::read(void *data, size_t byte)
    {
        spi_transaction_t buffer =
            {
                .flags = static_cast<uint32_t>(NULL),
                .cmd = 0,
                .addr = 0,
                .length = byte * 8,
                .rxlength = 0,
                .user = (void *)0,
                .tx_buffer = NULL,
                .rx_buffer = data,
            };

        esp_err_t assert = spi_device_polling_transmit(_spi_device, &buffer);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_device_polling_transmit fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool HWSPI::write_read(const void *write_data, void *read_data, size_t data_byte)
    {
        spi_transaction_t buffer =
            {
                .flags = static_cast<uint32_t>(NULL),
                .cmd = 0,
                .addr = 0,
                .length = data_byte * 8,
                .rxlength = 0,
                .user = (void *)0,
                .tx_buffer = write_data,
                .rx_buffer = read_data,
            };

        esp_err_t assert = spi_device_polling_transmit(_spi_device, &buffer);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_device_polling_transmit fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

}