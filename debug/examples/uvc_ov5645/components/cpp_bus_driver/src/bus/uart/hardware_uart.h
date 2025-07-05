/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-06-16 17:51:21
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Uart : public Bus_Uart_Guide
    {
    private:
        static constexpr uint16_t UART_RX_MAX_SIZE = 1024 * 2;

        int32_t _tx, _rx;
        uart_port_t _port;
        int32_t _baud_rate;

    public:
        Hardware_Uart(int32_t tx, int32_t rx, uart_port_t port = uart_port_t::UART_NUM_1)
            : _tx(tx), _rx(rx), _port(port)
        {
        }

        bool begin(int32_t baud_rate = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        int32_t read(void *data, uint32_t length) override;
        int32_t write(const void *data, size_t length) override;

        size_t get_rx_buffer_length(void) override;
        bool clear_rx_buffer_data(void) override;
        bool set_baud_rate(uint32_t baud_rate) override;
        uint32_t get_baud_rate(void) override;
    };
}
