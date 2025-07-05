/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:26:23
 * @LastEditTime: 2025-06-18 14:48:47
 * @License: GPL 3.0
 */
#include "hardware_uart.h"

namespace Cpp_Bus_Driver
{
    bool Hardware_Uart::begin(int32_t baud_rate)
    {
        if (baud_rate == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            baud_rate = DEFAULT_CPP_BUS_DRIVER_UART_BAUD_RATE;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring _port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring _tx: %d\n", _tx);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring _rx: %d\n", _rx);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring baud_rate: %d bps\n", baud_rate);

        const uart_config_t uart_config =
            {
                .baud_rate = baud_rate,
                .data_bits = UART_DATA_8_BITS,
                .parity = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .rx_flow_ctrl_thresh = 122,
                .source_clk = UART_SCLK_DEFAULT,
                .flags =
                    {
                        .allow_pd = 1,
                        .backup_before_sleep = 1,
                    },
            };

        // We won't use a buffer for sending data.
        esp_err_t assert = uart_driver_install(static_cast<uart_port_t>(_port), UART_RX_MAX_SIZE, 0, 0, NULL, 0);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_driver_install fail (error code: %#X)\n", assert);
            return false;
        }

        assert = uart_param_config(static_cast<uart_port_t>(_port), &uart_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_param_config fail (error code: %#X)\n", assert);
            return false;
        }

        assert = uart_set_pin(static_cast<uart_port_t>(_port), _tx, _rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_set_pin fail (error code: %#X)\n", assert);
            return false;
        }

        _baud_rate = baud_rate;

        return true;
    }

    int32_t Hardware_Uart::read(void *data, uint32_t length)
    {
        int32_t buffer_size = uart_read_bytes(static_cast<uart_port_t>(_port), data, length, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_UART_WAIT_TIMEOUT_MS));
        if (buffer_size == (-1))
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_read_bytes fail(uart_read_bytes == (-1))\n");
            return false;
        }

        return buffer_size;
    }

    int32_t Hardware_Uart::write(const void *data, size_t length)
    {
        int32_t buffer_size = uart_write_bytes(static_cast<uart_port_t>(_port), data, length);
        if (buffer_size == (-1))
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_write_bytes fail(uart_write_bytes == (-1))\n");
            return false;
        }

        return buffer_size;
    }

    size_t Hardware_Uart::get_rx_buffer_length(void)
    {
        size_t buffer = 0;

        esp_err_t assert = uart_get_buffered_data_len(static_cast<uart_port_t>(_port), &buffer);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_get_buffered_data_len fail (error code: %#X)\n", assert);
            return false;
        }

        return buffer;
    }

    bool Hardware_Uart::clear_rx_buffer_data(void)
    {
        esp_err_t assert = uart_flush_input(static_cast<uart_port_t>(_port));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_flush_input fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Uart::set_baud_rate(uint32_t baud_rate)
    {
        esp_err_t assert = uart_set_baudrate(static_cast<uart_port_t>(_port), baud_rate);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_set_baudrate fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    uint32_t Hardware_Uart::get_baud_rate(void)
    {
        uint32_t buffer = 0;

        esp_err_t assert = uart_get_baudrate(static_cast<uart_port_t>(_port), &buffer);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "uart_get_baudrate fail (error code: %#X)\n", assert);
            return -1;
        }

        return buffer;
    }
}