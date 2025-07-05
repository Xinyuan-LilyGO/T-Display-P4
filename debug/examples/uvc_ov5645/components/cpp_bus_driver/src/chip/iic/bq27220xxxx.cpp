/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-06-16 18:01:42
 * @License: GPL 3.0
 */
#include "Bq27220xxxx.h"

namespace Cpp_Bus_Driver
{
    // constexpr uint8_t Bq27220xxxx::Init_List[] =
    //     {
    //         static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8),
    //         static_cast<uint8_t>(Cmd::RW_CLKOUT_CONTROL),
    //         0B00000000,
    // };

    bool Bq27220xxxx::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        uint16_t buffer = get_device_id();
        if (buffer != static_cast<uint16_t>(BQ27220YZFR_DEVICE_ID))
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get bq27220xxxx id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get bq27220xxxx id: %#X\n", buffer);
        }

        // if (iic_init_list(Init_List, sizeof(Init_List)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list fail\n");
        //     return false;
        // }

        return true;
    }
    
    bool Bq27220xxxx::enter_cgf_update(void)
    {
        // 发送 ENTER_CGF_UPDATE 子命令 (0x0090)
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_START), static_cast<uint16_t>(Control_Status_Reg::WO_ENTER_CFG_UPDATE), Endian::LITTLE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        // 通过轮询 OperationStatus() 寄存器直到位 2 被设置来确认 CFGUPDATE 模式，可能最多需要 1 秒
        uint8_t timeout_count = 0;
        while (1)
        {
            Operation_Status os;
            if (get_operation_status(os) == true)
            {
                if (os.flag.cfg_update == true)
                {
                    break;
                }
            }

            timeout_count++;
            if (timeout_count > 200)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "enter config update mode timeout\n");
                return false;
            }
            delay_ms(5);
        }

        return true;
    }

    bool Bq27220xxxx::exit_cfg_update(void)
    {
        // 通过发送 EXIT_CFG_UPDATE_REINIT (0x0091) 或 EXIT_CFG_UPDATE (0x0092) 命令退出 CFGUPDATE 模式
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_START), static_cast<uint16_t>(Control_Status_Reg::WO_EXIT_CFG_UPDATE), Endian::LITTLE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        // 通过轮询 OperationStatus() 寄存器直到位 2 被清除来确认 CFGUPDATE 模式
        uint8_t timeout_count = 0;
        while (1)
        {
            Operation_Status os;
            if (get_operation_status(os) == true)
            {
                if (os.flag.cfg_update == false)
                {
                    break;
                }
            }

            timeout_count++;
            if (timeout_count > 200)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "exit config update mode timeout\n");
                return false;
            }
            delay_ms(5);
        }

        return true;
    }

    uint16_t Bq27220xxxx::get_device_id(void)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_START), static_cast<uint16_t>(Control_Status_Reg::RO_DEVICE_ID), Endian::LITTLE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return -1;
        }
        delay_ms(20); // 必须有延时

        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        // uint8_t buffer[2] = {static_cast<uint8_t>(Control_Status_Reg::RO_DEVICE_ID), static_cast<uint8_t>(static_cast<uint16_t>(Control_Status_Reg::RO_DEVICE_ID) >> 8)};

        // i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
        // if (_bus->start_transmit(cmd_handle, i2c_rw_t ::I2C_MASTER_WRITE, false) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
        //     return false;
        // }
        // if (_bus->write(cmd_handle, static_cast<uint8_t>(Cmd::RW_CONTROL_STATUS_START)) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(cmd_handle, buffer, 2) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->stop_transmit(cmd_handle) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "stop_transmit fail\n");
        //     return false;
        // }

        // cmd_handle = i2c_cmd_link_create();
        // if (_bus->start_transmit(cmd_handle, i2c_rw_t ::I2C_MASTER_WRITE, false) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
        //     return false;
        // }
        // if (_bus->write(cmd_handle, static_cast<uint8_t>(Cmd::RW_MAC_DATA_START)) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        // if (_bus->start_transmit(cmd_handle, i2c_rw_t ::I2C_MASTER_READ) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
        //     return false;
        // }
        // if (_bus->read(cmd_handle, buffer, 2) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->stop_transmit(cmd_handle) == false)
        // {
        //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "stop_transmit fail\n");
        //     return false;
        // }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_design_capacity(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DESIGN_CAPACITY_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_voltage(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_VOLTAGE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    int16_t Bq27220xxxx::get_current(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_CURRENT_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_remaining_capacity(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_REMAINING_CAPACITY_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_full_charge_capacity(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_FULL_CHARGE_CAPACITY_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
    }

    int16_t Bq27220xxxx::get_at_rate(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_AT_RATE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
    }

    bool Bq27220xxxx::set_at_rate(int16_t rate)
    {
        // 小端发送
        const uint8_t buffer[2] =
            {
                static_cast<uint8_t>(rate),
                static_cast<uint8_t>(rate >> 8),
            };

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_AT_RATE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint16_t Bq27220xxxx::get_at_rate_time_to_empty(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_AT_RATE_TIME_TO_EMPTY_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    float Bq27220xxxx::get_temperature_kelvin(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TEMPERATURE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1;
    }

    float Bq27220xxxx::get_temperature_celsius(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TEMPERATURE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1 - 273.15;
    }

    bool Bq27220xxxx::set_temperature_mode(Temperature_Mode mode)
    {
        if (enter_cgf_update() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "enter_cgf_update fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WR_RAM_REGISTER), static_cast<uint16_t>(Configuration_Reg::RW_OPERATION_CONFIG_A), Endian::LITTLE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        // for (uint8_t i = 0; i < 2; i++)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer[%d]: %#X\n", i, buffer[i]);
        // }

        switch (mode)
        {
        case Temperature_Mode::INTERNAL:
            buffer[0] &= 0B01111110;
            break;
        case Temperature_Mode::EXTERNAL_NTC:
            buffer[0] = (buffer[0] | 0B10000000) & 0B11111110;
            break;

        default:
            break;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        uint8_t buffer_new_chksum = 0xFF - (0xFF & (static_cast<uint8_t>(static_cast<uint16_t>(Configuration_Reg::RW_OPERATION_CONFIG_A) >> 8) +
                                                    static_cast<uint8_t>(Configuration_Reg::RW_OPERATION_CONFIG_A) + buffer[0] + buffer[1]));

        uint8_t buffer_data_len = 6;

        // 写入新校验和
        if (_bus->write(static_cast<uint8_t>(Cmd::RO_MAC_DATA_SUM_START), buffer_new_chksum) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 写入块长度，当整个块的正确校验和以及长度被写入时，数据实际上被传输到 RAM 中
        if (_bus->write(static_cast<uint8_t>(Cmd::RO_MAC_DATA_LEN_START), buffer_data_len) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        if (exit_cfg_update() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "enter_cgf_update fail\n");
            return false;
        }

        return true;
    }

    bool Bq27220xxxx::get_battery_status(Battery_Status &status)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_BATTERY_STATUS_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        status.flag.fd = (buffer[1] & 0B10000000) >> 7;
        status.flag.ocvcomp = (buffer[1] & 0B01000000) >> 6;
        status.flag.ocvfail = (buffer[1] & 0B00100000) >> 5;
        status.flag.sleep = (buffer[1] & 0B00010000) >> 4;
        status.flag.otc = (buffer[1] & 0B00001000) >> 3;
        status.flag.otd = (buffer[1] & 0B00000100) >> 2;
        status.flag.fc = (buffer[1] & 0B00000010) >> 1;
        status.flag.chginh = buffer[1] & 0B00000001;
        status.flag.tca = (buffer[0] & 0B01000000) >> 6;
        status.flag.ocvgd = (buffer[0] & 0B00100000) >> 5;
        status.flag.auth_gd = (buffer[0] & 0B00010000) >> 4;
        status.flag.battpres = (buffer[0] & 0B00001000) >> 3;
        status.flag.tda = (buffer[0] & 0B00000100) >> 2;
        status.flag.sysdwn = (buffer[0] & 0B00000010) >> 1;
        status.flag.dsg = buffer[0] & 0B00000001;

        // for (uint8_t i = 0; i < 2; i++)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "battery_status[%d]: %#X\n", i, buffer[i]);
        // }

        return true;
    }

    bool Bq27220xxxx::get_operation_status(Operation_Status &status)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_OPERATION_STATUS_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        status.sec = (buffer[0] & 0B00000110) >> 1;

        status.flag.cfg_update = (buffer[1] & 0B00000100) >> 2;
        status.flag.btp_int = (buffer[0] & 0B10000000) >> 7;
        status.flag.smth = (buffer[0] & 0B01000000) >> 6;
        status.flag.init_comp = (buffer[0] & 0B00100000) >> 5;
        status.flag.vdq = (buffer[0] & 0B00010000) >> 4;
        status.flag.edv2 = (buffer[0] & 0B00001000) >> 3;
        status.flag.calmd = buffer[0] & 0B00000001;

        // for (uint8_t i = 0; i < 2; i++)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "operation_status[%d]: %#X\n", i, buffer[i]);
        // }

        return true;
    }

    bool Bq27220xxxx::set_design_capacity(uint16_t capacity)
    {
        // uint8_t buffer[3] = {0x00, 0x14, 0x04};

        // // 发送密钥使其芯片处于 UNSEAL 模式
        // if (_bus->write(buffer, 3) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // buffer[0] = 0x00;
        // buffer[1] = 0x72;
        // buffer[2] = 0x36;
        // if (_bus->write(buffer, 3) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x00), static_cast<uint8_t>(0x14)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x01), static_cast<uint8_t>(0x04)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x00), static_cast<uint8_t>(0x72)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x01), static_cast<uint8_t>(0x36)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        // // 发送命令进入 FULL ACCESS 模式以访问数据存储器
        // buffer[0] = 0x00;
        // buffer[1] = 0xFF;
        // buffer[2] = 0xFF;
        // if (_bus->write(buffer, 3) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(buffer, 3) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x00), static_cast<uint8_t>(0xFF)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x01), static_cast<uint8_t>(0xFF)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x00), static_cast<uint8_t>(0xFF)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }
        // if (_bus->write(static_cast<uint8_t>(0x01), static_cast<uint8_t>(0xFF)) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        // delay_ms(10);

        if (enter_cgf_update() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "enter_cgf_update fail\n");
            return false;
        }

        // 将 0x929F 写入 0x3E 以访问 Design Capacity
        if (_bus->write(static_cast<uint8_t>(Cmd::WR_RAM_REGISTER), static_cast<uint16_t>(Gas_Gauging_Reg::WO_DESIGN_CAPACITY), Endian::LITTLE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        // // 使用 MACDataSum() 命令 (0x60) 读取 1 字节校验和
        // uint8_t buffer_old_chksum = 0;
        // if (_bus->read(static_cast<uint8_t>(Cmd::RO_MAC_DATA_SUM_START), &buffer_old_chksum) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }

        // // 使用 MACDataLen() 命令 (0x61) 读取 1 字节块长度
        // uint8_t buffer_data_len = 0;
        // if (_bus->read(static_cast<uint8_t>(Cmd::RO_MAC_DATA_LEN_START), &buffer_data_len) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }

        // // 从 0x40 开始读取两个 Design Capacity 字节
        // uint8_t buffer_old_dc_msb = 0;
        // uint8_t buffer_old_dc_lsb = 0;
        // if (_bus->read(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START), &buffer_old_dc_msb) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // if (_bus->read(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START) + 1, &buffer_old_dc_lsb) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }

        // 从 0x40 开始读取写入两个 Design Capacity 字节，也就是 capacity 值
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START), capacity) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 计算新校验和，校验和为 (255 – x)，其中 x 是逐字节的 BlockData() 8 位总和（0x40 至 0x5F）
        // 计算新校验和的一种快速方法是使用新旧数据总和字节的数据替换方法，请参阅所示方法的代码
        // uint8_t buffer_new_chksum = 255 - ((((255 - buffer_old_chksum - buffer_old_dc_msb - buffer_old_dc_lsb) % 256) +
        //                                     static_cast<uint8_t>(capacity >> 8) + static_cast<uint8_t>(capacity)) %
        //                                    256)-23;
        uint8_t buffer_new_chksum = 0xFF - (0xFF & (static_cast<uint8_t>(static_cast<uint16_t>(Gas_Gauging_Reg::WO_DESIGN_CAPACITY) >> 8) +
                                                    static_cast<uint8_t>(Gas_Gauging_Reg::WO_DESIGN_CAPACITY) + static_cast<uint8_t>(capacity >> 8) + static_cast<uint8_t>(capacity)));

        uint8_t buffer_data_len = 6;
        // buffer_new_chksum=0xB0;

        // 写入新校验和
        if (_bus->write(static_cast<uint8_t>(Cmd::RO_MAC_DATA_SUM_START), buffer_new_chksum) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 写入块长度，当整个块的正确校验和以及长度被写入时，数据实际上被传输到 RAM 中
        if (_bus->write(static_cast<uint8_t>(Cmd::RO_MAC_DATA_LEN_START), buffer_data_len) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_old_chksum: %#X\n", buffer_old_chksum);
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_data_len: %#X\n", buffer_data_len);
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_old_dc_msb: %#X\n", buffer_old_dc_msb);
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_old_dc_lsb: %#X\n", buffer_old_dc_lsb);
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_new_chksum: %#X\n", buffer_new_chksum);

        if (exit_cfg_update() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "enter_cgf_update fail\n");
            return false;
        }

        // 如果器件之前处于 SEALED 状态，则通过发送 Control (0x0030) 子命令来返回至 SEALED 模式
        // buffer[0] = 0x00;
        // buffer[1] = 0x30;
        // buffer[2] = 0x00;
        // if (_bus->write(buffer, 3) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
        //     return false;
        // }

        return true;
    }

    uint16_t Bq27220xxxx::get_time_to_empty(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TIME_TO_EMPTY_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_time_to_full(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_TIME_TO_FULL_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    int16_t Bq27220xxxx::get_standby_current(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_STANDBY_CURRENT_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_standby_time_to_empty(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_STANDBY_TIME_TO_EMPTY_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    int16_t Bq27220xxxx::get_max_load_current(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_MAX_LOAD_CURRENT_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_max_load_time_to_empty(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_MAX_LOAD_TIME_TO_EMPTY_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_raw_coulomb_count(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_RAW_COULOMB_COUNT_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    int16_t Bq27220xxxx::get_average_power(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_AVERAGE_POWER_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<int16_t>(buffer[1]) << 8) | buffer[0];
    }

    float Bq27220xxxx::get_chip_temperature_kelvin(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_INTERNAL_TEMPERATURE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1;
    }

    float Bq27220xxxx::get_chip_temperature_celsius(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_INTERNAL_TEMPERATURE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return ((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]) * 0.1 - 273.15;
    }

    uint16_t Bq27220xxxx::get_cycle_count(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_CYCLE_COUNT_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_status_of_charge(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_STATUS_OF_CHARGE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_status_of_health(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_STATUS_OF_HEALTH_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_charging_voltage(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_CHARGING_VOLTAGE_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    uint16_t Bq27220xxxx::get_charging_current(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_CHARGING_CURRENT_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (static_cast<uint16_t>(buffer[1]) << 8) | buffer[0];
    }

    bool Bq27220xxxx::set_sleep_current_threshold(uint16_t threshold)
    {
        if (threshold > 100)
        {
            threshold = 100;
        }

        if (enter_cgf_update() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "enter_cgf_update fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WR_RAM_REGISTER), static_cast<uint16_t>(Configuration_Reg::RW_SLEEP_CURRENT), Endian::LITTLE) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        // uint8_t buffer[2] = {0};

        // if (_bus->read(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START), buffer, 2) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }

        // for (uint8_t i = 0; i < 2; i++)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer[%d]: %#X\n", i, buffer[i]);
        // }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MAC_DATA_START), threshold) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        uint8_t buffer_new_chksum = 0xFF - (0xFF & (static_cast<uint8_t>(static_cast<uint16_t>(Configuration_Reg::RW_SLEEP_CURRENT) >> 8) +
                                                    static_cast<uint8_t>(Configuration_Reg::RW_SLEEP_CURRENT) + static_cast<uint8_t>(threshold >> 8) +
                                                    static_cast<uint8_t>(threshold)));

        uint8_t buffer_data_len = 6;

        // 写入新校验和
        if (_bus->write(static_cast<uint8_t>(Cmd::RO_MAC_DATA_SUM_START), buffer_new_chksum) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 写入块长度，当整个块的正确校验和以及长度被写入时，数据实际上被传输到 RAM 中
        if (_bus->write(static_cast<uint8_t>(Cmd::RO_MAC_DATA_LEN_START), buffer_data_len) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        delay_ms(10); // 必须有延时

        if (exit_cfg_update() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "enter_cgf_update fail\n");
            return false;
        }

        return true;
    }

}
