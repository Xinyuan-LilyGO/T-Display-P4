/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-02-24 12:01:04
 * @License: GPL 3.0
 */
#include "hardware_iic_2.h"

namespace Cpp_Bus_Driver
{
    bool Hardware_Iic_2::begin(int32_t frequency, int16_t address)
    {
        _frequency = (frequency == DEFAULT_CPP_BUS_DRIVER_VALUE) ? DEFAULT_CPP_BUS_DRIVER_IIC_FREQUENCY : frequency;

        if ((_port < I2C_NUM_0) || (_port > I2C_NUM_MAX))
        {
            _port = I2C_NUM_0;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring the address: %#X\n", address);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring iic_port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring sda_io_num: %d\n", _sda);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring scl_io_num: %d\n", _scl);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring scl_speed_hz: %d hz\n", _frequency);

        if (address == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "address is null\n");
            // return false;
        }
        else
        {
            _address = address;
        }

        const i2c_config_t iic_config = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = static_cast<gpio_num_t>(_sda),
            .scl_io_num = static_cast<gpio_num_t>(_scl),
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master =
                {
                    .clk_speed = static_cast<uint32_t>(_frequency),
                },
            .clk_flags = 0,
        };
        esp_err_t assert = i2c_param_config(static_cast<i2c_port_t>(_port), &iic_config);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_param_config fail (error code: %#X)\n", assert);
            return false;
        }

        assert = i2c_driver_install(static_cast<i2c_port_t>(_port), I2C_MODE_MASTER, 0, 0, 0);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_driver_install fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_2::read(uint8_t *data, size_t length)
    {
        size_t assert = i2c_master_read_from_device(static_cast<i2c_port_t>(_port), _address, data, length, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_read_from_device fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }
    bool Hardware_Iic_2::write(const uint8_t *data, size_t length)
    {
        size_t assert = i2c_master_write_to_device(static_cast<i2c_port_t>(_port), _address, data, length, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_write_to_device fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }
    bool Hardware_Iic_2::write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length)
    {
        size_t assert = i2c_master_write_read_device(static_cast<i2c_port_t>(_port), _address, write_data, write_length, read_data, read_length, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
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
        size_t assert = i2c_master_read_from_device(static_cast<i2c_port_t>(_port), address, &buffer, 1, pdMS_TO_TICKS(DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS));
        if (assert != ESP_OK)
        {
            // assert_log(Log_Level::INFO, __FILE__, __LINE__, "i2c_master_read_from_device fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    // void CPP_Hardware_Iic_2::BeginTransmission(uint8_t address)
    // {
    //     _wire->beginTransmission(address);
    // }

    // bool CPP_Hardware_Iic_2::EndTransmission(void)
    // {
    //     return !(_wire->endTransmission());
    // }

    // bool CPP_Hardware_Iic_2::Write(uint8_t d)
    // {
    //     return _wire->write(d);
    // }

    // bool CPP_Hardware_Iic_2::Write(const uint8_t *data, size_t length)
    // {
    //     return _wire->write(data, length);
    // }

    // uint8_t CPP_Hardware_Iic_2::Read(void)
    // {
    //     return (uint8_t)_wire->read();
    // }

    // bool CPP_Hardware_Iic_2::RequestFrom(uint8_t address, size_t length)
    // {
    //     return _wire->requestFrom(address, length);
    // }

}