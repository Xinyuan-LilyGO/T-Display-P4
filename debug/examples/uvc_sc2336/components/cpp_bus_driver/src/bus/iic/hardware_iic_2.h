/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-04-09 09:36:55
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Iic_2 : public Bus_Iic_Guide
    {
    public:
        int32_t _sda, _scl;
        i2c_port_t _port;
        int16_t _address = DEFAULT_CPP_BUS_DRIVER_VALUE;
        int32_t _freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE;

        Hardware_Iic_2(int32_t sda, int32_t scl, i2c_port_t port = I2C_NUM_0)
            : _sda(sda), _scl(scl), _port(port)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE, int16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool read(uint8_t *data, size_t length) override;
        bool write(const uint8_t *data, size_t length) override;
        bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) override;

        bool probe(const uint16_t address) override;

        i2c_cmd_handle_t cmd_link_create(void) override;
        bool start_transmit(i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en = true) override;
        bool read(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, i2c_ack_type_t ack = I2C_MASTER_LAST_NACK) override;
        bool write(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en) override;
        bool write(i2c_cmd_handle_t cmd_handle, const uint8_t *data, size_t data_len, bool ack_en = true) override;
        bool stop_transmit(i2c_cmd_handle_t cmd_handle) override;
    };
}
