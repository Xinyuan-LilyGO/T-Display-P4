/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-06-18 14:46:04
 * @License: GPL 3.0
 */
#include "hardware_iic_2.h"

namespace Cpp_Bus_Driver
{
    bool Hardware_Iic_2::begin(int32_t freq_hz, int16_t address)
    {
        if (freq_hz == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            freq_hz = DEFAULT_CPP_BUS_DRIVER_IIC_FREQ_HZ;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config address: %#X\n", address);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config _port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config _sda: %d\n", _sda);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config _scl: %d\n", _scl);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config freq_hz: %d hz\n", freq_hz);

        if (address == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "address is null\n");
            // return false;
        }

        const i2c_config_t iic_config =
            {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = static_cast<gpio_num_t>(_sda),
                .scl_io_num = static_cast<gpio_num_t>(_scl),
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master =
                    {
                        .clk_speed = static_cast<uint32_t>(freq_hz),
                    },
                .clk_flags = 0,
            };

        esp_err_t assert = i2c_param_config(_port, &iic_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_param_config fail (error code: %#X)\n", assert);
            return false;
        }

        assert = i2c_driver_install(_port, I2C_MODE_MASTER, 0, 0, 0);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_driver_install fail (error code: %#X)\n", assert);
            // return false;
        }

        _freq_hz = freq_hz;
        _address = address;

        return true;
    }

    bool Hardware_Iic_2::read(uint8_t *data, size_t length)
    {
        esp_err_t assert = i2c_master_read_from_device(_port, _address, data, length, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_read_from_device fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }
    bool Hardware_Iic_2::write(const uint8_t *data, size_t length)
    {
        esp_err_t assert = i2c_master_write_to_device(_port, _address, data, length, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_write_to_device fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }
    bool Hardware_Iic_2::write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length)
    {
        esp_err_t assert = i2c_master_write_read_device(_port, _address, write_data, write_length, read_data, read_length, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_transmit_receive fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_2::probe(const uint16_t address)
    {
        uint8_t buffer = 0;
        esp_err_t assert = i2c_master_read_from_device(_port, address, &buffer, 1, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            // assert_log(Log_Level::INFO, __FILE__, __LINE__, "i2c_master_read_from_device fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    i2c_cmd_handle_t Hardware_Iic_2::cmd_link_create(void)
    {
        return i2c_cmd_link_create();
    }

    bool Hardware_Iic_2::start_transmit(i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en)
    {
        esp_err_t assert = i2c_master_start(cmd_handle);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_start fail (error code: %#X)\n", assert);
            i2c_cmd_link_delete(cmd_handle);
            return false;
        }

        assert = i2c_master_write_byte(cmd_handle, _address << 1 | rw, ack_en);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_write_byte fail (error code: %#X)\n", assert);
            i2c_cmd_link_delete(cmd_handle);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_2::write(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en)
    {
        esp_err_t assert = i2c_master_write_byte(cmd_handle, data, ack_en);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_write_byte fail (error code: %#X)\n", assert);
            i2c_cmd_link_delete(cmd_handle);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_2::write(i2c_cmd_handle_t cmd_handle, const uint8_t *data, size_t data_len, bool ack_en)
    {
        esp_err_t assert = i2c_master_write(cmd_handle, data, data_len, ack_en);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_write fail (error code: %#X)\n", assert);
            i2c_cmd_link_delete(cmd_handle);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_2::read(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, i2c_ack_type_t ack)
    {
        esp_err_t assert = i2c_master_read(cmd_handle, data, data_len, ack);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_read fail (error code: %#X)\n", assert);
            i2c_cmd_link_delete(cmd_handle);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_2::stop_transmit(i2c_cmd_handle_t cmd_handle)
    {
        esp_err_t assert = i2c_master_stop(cmd_handle);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "stop_transmit fail (error code: %#X)\n", assert);
            i2c_cmd_link_delete(cmd_handle);
            return false;
        }

        assert = i2c_master_cmd_begin(_port, cmd_handle, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_cmd_begin fail (error code: %#X)\n", assert);
            i2c_cmd_link_delete(cmd_handle);
            return false;
        }

        i2c_cmd_link_delete(cmd_handle);

        return true;
    }

}