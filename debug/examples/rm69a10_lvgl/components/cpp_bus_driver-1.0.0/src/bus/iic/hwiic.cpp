/*
 * @Description: CPP_HWIIC.cpp
 * @version: V1.0.1
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:46:16
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-12-27 16:21:00
 * @License: GPL 3.0
 */
#include "hwiic.h"

namespace Cpp_Bus_Driver
{
    bool HWIIC::begin(int64_t frequency, int16_t address)
    {
        _frequency = (frequency == DEFAULT_CPP_BUS_DRIVER_VALUE) ? DEFAULT_CPP_BUS_DRIVER_IIC_FREQUENCY : frequency;

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring the address: %#X\n", address);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring i2c_port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring sda_io_num: %d\n", _sda);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring scl_io_num: %d\n", _scl);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "configuring scl_speed_hz: %llu hz\n", _frequency);

#if defined INITIALIZATION_CONFIGURATION_METHOD_1
        i2c_master_bus_config_t bus_config =
            {
                .i2c_port = static_cast<i2c_port_num_t>(_port), // 选择I2C端口号，I2C_NUM_0表示使用I2C0
                .sda_io_num = static_cast<gpio_num_t>(_sda),
                .scl_io_num = static_cast<gpio_num_t>(_scl),
                .clk_source = I2C_CLK_SRC_DEFAULT, // 使用默认的I2C时钟源
                .glitch_ignore_cnt = 7,            // 设置滤波器忽略的毛刺周期为7个I2C模块时钟周期
                .intr_priority = 0,                // 设置中断优先级
                .trans_queue_depth = 0,            // 设置传输队列深度
                .flags =
                    {
                        .enable_internal_pullup = 1, // 启用内部上拉电阻
                        .allow_pd = 0                // 不允许在睡眠模式下备份/恢复I2C寄存器
                    },
            };

        esp_err_t assert = i2c_new_master_bus(&bus_config, &_iic_bus);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_new_master_bus fail (error code%u)\n", assert);
            return false;
        }

        if (address == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "address is null\n");
            return false;
        }
        else
        {
            _address = address;
        }

        i2c_device_config_t device_config =
            {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = static_cast<uint16_t>(_address),
                .scl_speed_hz = static_cast<uint32_t>(_frequency),
                .scl_wait_us = 0,
                .flags =
                    {
                        .disable_ack_check = 1,
                    },
            };

        assert = i2c_master_bus_add_device(_iic_bus, &device_config, &_iic_device);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_bus_add_device fail (error code%u)\n", assert);
            return false;
        }
#elif defined INITIALIZATION_CONFIGURATION_METHOD_2

        if (address == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "address is null\n");
            // return false;
        }
        else
        {
            _address = address;
        }

        i2c_config_t iic_config = {
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
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_param_config fail (error code%u)\n", assert);
            return false;
        }

        assert = i2c_driver_install(static_cast<i2c_port_t>(_port), I2C_MODE_MASTER, 0, 0, 0);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_driver_install fail (error code%u)\n", assert);
            return false;
        }

#endif

        return true;
    }

    bool HWIIC::read(uint8_t *data, size_t length)
    {
#if defined INITIALIZATION_CONFIGURATION_METHOD_1
        size_t assert = i2c_master_receive(_iic_device, data, length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_receive fail (error code%u)\n", assert);
            return false;
        }
#elif defined INITIALIZATION_CONFIGURATION_METHOD_2
        size_t assert = i2c_master_read_from_device(static_cast<i2c_port_t>(_port), _address, data, length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_read_from_device fail (error code%u)\n", assert);
            return false;
        }
#endif
        return true;
    }
    bool HWIIC::write(const uint8_t *data, size_t length)
    {
#if defined INITIALIZATION_CONFIGURATION_METHOD_1
        size_t assert = i2c_master_transmit(_iic_device, data, length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_transmit fail (error code%u)\n", assert);
            return false;
        }
#elif defined INITIALIZATION_CONFIGURATION_METHOD_2
        size_t assert = i2c_master_write_to_device(static_cast<i2c_port_t>(_port), _address, data, length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_write_to_device fail (error code%u)\n", assert);
            return false;
        }
#endif
        return true;
    }
    bool HWIIC::write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length)
    {
#if defined INITIALIZATION_CONFIGURATION_METHOD_1
        size_t assert = i2c_master_transmit_receive(_iic_device, write_data, write_length, read_data, read_length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_transmit_receive fail (error code%u)\n", assert);
            return false;
        }
#elif defined INITIALIZATION_CONFIGURATION_METHOD_2
        size_t assert = i2c_master_write_read_device(static_cast<i2c_port_t>(_port), _address, write_data, write_length, read_data, read_length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_transmit_receive fail (error code%u)\n", assert);
            return false;
        }
#endif
        return true;
    }

    bool HWIIC::probe(const uint16_t address)
    {
#if defined INITIALIZATION_CONFIGURATION_METHOD_1
        size_t assert = i2c_master_probe(_iic_bus, address, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            // assert_log(Log_Level::INFO, __FILE__, __LINE__, "i2c_master_probe fail (error code%u)\n", assert);
            return false;
        }
#elif defined INITIALIZATION_CONFIGURATION_METHOD_2
        uint8_t buffer = 0;
        size_t assert = i2c_master_read_from_device(static_cast<i2c_port_t>(_port), address, &buffer, 1, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            // assert_log(Log_Level::INFO, __FILE__, __LINE__, "i2c_master_read_from_device fail (error code%u)\n", assert);
            return false;
        }
#endif
        return true;
    }

    // void CPP_HWIIC::BeginTransmission(uint8_t address)
    // {
    //     _wire->beginTransmission(address);
    // }

    // bool CPP_HWIIC::EndTransmission(void)
    // {
    //     return !(_wire->endTransmission());
    // }

    // bool CPP_HWIIC::Write(uint8_t d)
    // {
    //     return _wire->write(d);
    // }

    // bool CPP_HWIIC::Write(const uint8_t *data, size_t length)
    // {
    //     return _wire->write(data, length);
    // }

    // uint8_t CPP_HWIIC::Read(void)
    // {
    //     return (uint8_t)_wire->read();
    // }

    // bool CPP_HWIIC::RequestFrom(uint8_t address, size_t length)
    // {
    //     return _wire->requestFrom(address, length);
    // }

}