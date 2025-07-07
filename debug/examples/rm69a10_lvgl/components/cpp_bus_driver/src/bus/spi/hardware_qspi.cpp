/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-07-04 13:39:39
 * @License: GPL 3.0
 */
#include "hardware_qspi.h"

namespace Cpp_Bus_Driver
{
    bool Hardware_Qspi::begin(int32_t freq_hz, int32_t cs)
    {
        if (freq_hz == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            freq_hz = DEFAULT_CPP_BUS_DRIVER_QSPI_FREQ_HZ;
        }

        if (_flags == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _flags = SPI_DEVICE_HALFDUPLEX;
        }

        if (_cs != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            _cs = cs;
            pin_mode(_cs, Pin_Mode::OUTPUT, Pin_Status ::PULLUP);
            set_cs(1);
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _data0: %d\n", _data0);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _data1: %d\n", _data1);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _data2: %d\n", _data2);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _data3: %d\n", _data3);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _sclk: %d\n", _sclk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config cs: %d\n", _cs);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _mode: %d\n", _mode);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _flags: %d\n", _flags);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config freq_hz: %d hz\n", freq_hz);

        const spi_bus_config_t bus_config =
            {
                .data0_io_num = _data0,
                .data1_io_num = _data1,
                .sclk_io_num = _sclk,
                .data2_io_num = _data2,
                .data3_io_num = _data3,
                .data4_io_num = -1,
                .data5_io_num = -1,
                .data6_io_num = -1,
                .data7_io_num = -1,
                .data_io_default_level = 0,
                .max_transfer_sz = QSPI_MAX_TRANSFER_SIZE,
                .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_QUAD,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = static_cast<uint32_t>(NULL),
            };

        esp_err_t assert = spi_bus_initialize(_port, &bus_config, SPI_DMA_CH_AUTO);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_bus_initialize fail (error code: %#X)\n", assert);
            return false;
        }

        const spi_device_interface_config_t device_config =
            {
                .command_bits = 0,
                .address_bits = 0,
                .dummy_bits = 0, // 无虚拟位
                .mode = _mode,
                .clock_source = SPI_CLK_SRC_DEFAULT, // 默认时钟源
                .duty_cycle_pos = 128,               // 50% 占空比
                .cs_ena_pretrans = 1,                // 在数据传输开始之前，片选信号（CS）应该提前多少个SPI位周期被激活
                .cs_ena_posttrans = 1,               // 在数据传输结束后，片选信号（CS）应该保持激活状态多少个SPI位周期
                .clock_speed_hz = freq_hz,
                .input_delay_ns = 0, // 无输入延迟
                .spics_io_num = -1,
                .flags = _flags, // 标志，可以填入SPI_DEVICE_BIT_LSBFIRST等信息
                .queue_size = 1,
                .pre_cb = NULL,  // 无传输前回调
                .post_cb = NULL, // 无传输后回调
            };
        assert = spi_bus_add_device(_port, &device_config, &_spi_device);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_bus_add_device fail (error code: %#X)\n", assert);
            return false;
        }

        size_t buffer = 0;
        assert = spi_bus_get_max_transaction_len(_port, &buffer);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_bus_get_max_transaction_len fail (error code: %#X)\n", assert);
            _max_transfer_size = QSPI_MAX_TRANSFER_SIZE;
        }
        else
        {
            _max_transfer_size = buffer;
        }
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_qspi config _max_transfer_size: %d\n", _max_transfer_size);

        _freq_hz = freq_hz;

        return true;
    }

    bool Hardware_Qspi::write(const void *data, size_t byte, uint32_t flags, bool cs_keep_active)
    {
        spi_transaction_t buffer =
            {
                .flags = flags,
                .cmd = 0,
                .addr = 0,
                .length = byte * 8,
                .rxlength = 0,
                .user = (void *)0,
                .tx_buffer = data,
                .rx_buffer = NULL,
            };

        if (byte > _max_transfer_size)
        {
            const uint8_t *buffer_data_ptr = static_cast<const uint8_t *>(data);
            size_t buffer_send_count = byte / _max_transfer_size;
            size_t buffer_remaining_size = byte % _max_transfer_size;

            buffer.length = _max_transfer_size * 8;

            set_cs(0);
            for (size_t i = 0; i < buffer_send_count; i++)
            {
                buffer.tx_buffer = buffer_data_ptr;

                esp_err_t assert = spi_device_polling_transmit(_spi_device, &buffer);
                if (assert != ESP_OK)
                {
                    set_cs(1);
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_device_polling_transmit fail (error code: %#X)\n", assert);
                    return false;
                }

                buffer_data_ptr += _max_transfer_size;
            }
            if (buffer_remaining_size > 0)
            {
                buffer.tx_buffer = buffer_data_ptr;
                buffer.length = buffer_remaining_size * 8;

                esp_err_t assert = spi_device_polling_transmit(_spi_device, &buffer);
                if (assert != ESP_OK)
                {
                    set_cs(1);
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_device_polling_transmit fail (error code: %#X)\n", assert);
                    return false;
                }
            }

            if (cs_keep_active == false)
            {
                set_cs(1);
            }
        }
        else
        {
            set_cs(0);
            esp_err_t assert = spi_device_polling_transmit(_spi_device, &buffer);
            if (assert != ESP_OK)
            {
                set_cs(1);
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "spi_device_polling_transmit fail (error code: %#X)\n", assert);
                return false;
            }
            if (cs_keep_active == false)
            {
                set_cs(1);
            }
        }

        return true;
    }

    bool Hardware_Qspi::set_cs(bool value)
    {
        if (pin_write(_cs, value) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }

        return true;
    }

}