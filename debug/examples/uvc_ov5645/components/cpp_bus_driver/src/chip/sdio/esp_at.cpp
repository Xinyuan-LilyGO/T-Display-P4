/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2025-06-16 18:11:28
 * @License: GPL 3.0
 */
#include "esp_at.h"

namespace Cpp_Bus_Driver
{
    bool Esp_At::begin(int32_t freq_hz)
    {
        _connect.status = true;
        _connect.error_count = 0;
        _connect.receive_total_length_index = 0;

        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);
            pin_write(_rst, 1);
            delay_ms(50);
            pin_write(_rst, 0);
            delay_ms(50);
            pin_write(_rst, 1);
            delay_ms(1000);
        }
        else if (_rst_callback != nullptr)
        {
            _rst_callback(1);
            delay_ms(50);
            _rst_callback(0);
            delay_ms(50);
            _rst_callback(1);
            delay_ms(1000);
        }

        if (Sdio_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        if (init_esp_at() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_esp_at fail\n");
            return false;
        }

        if (init_connect() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_connect fail\n");
            return false;
        }

        if (get_device_id() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get esp_at id fail\n");
            return false;
        }
        else
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get esp_at id success\n");
        }

        return true;
    }

    bool Esp_At::set_sleep(Sleep_Mode mode, int16_t timeout_ms)
    {
        if (mode == Sleep_Mode::POWER_DOWN)
        {
            if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
            {
                pin_write(_rst, 0);
            }
            else if (_rst_callback != nullptr)
            {
                _rst_callback(0);
            }
            else
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_sleep power_down fail\n");
                return false;
            }

            _connect.status = false;
            return true;
        }

        const char *buffer = nullptr;

        switch (mode)
        {
        case Sleep_Mode::DISABLE_SLEEP:
            buffer = "AT+SLEEP=0\r\n";
            break;
        case Sleep_Mode::MODEM_SLEEP:
            buffer = "AT+SLEEP=1\r\n";
            break;
        case Sleep_Mode::LIGHT_SLEEP:
            buffer = "AT+SLEEP=2\r\n";
            break;
        case Sleep_Mode::MODEM_SLEEP_LISTEN_INTERVAL:
            buffer = "AT+SLEEP=3\r\n";
            break;

        default:
            break;
        }

        send_packet(buffer, strlen(buffer));

        int16_t buffer_timeout_count = 0;

        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        break;
                    }

                    buffer_cmd = "\r\nERROR\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_sleep error\n");
                        return false;
                    }

                    buffer_cmd = "\r\nbusy p...\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_sleep busy\n");
                        buffer_timeout_count = 0;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_ms / 10)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_sleep timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::set_deep_sleep(uint32_t sleep_time_ms, int16_t timeout_ms)
    {
        if (sleep_time_ms > ((1U << 31) - 1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "overflow(sleep_time > ((1U << 31) - 1))\n");
            return false;
        }

        char at_cmd[30];
        snprintf(at_cmd, sizeof(at_cmd), "AT+GSLP=%ld\r\n", sleep_time_ms);
        const char *buffer = at_cmd;

        send_packet(buffer, strlen(buffer));

        int16_t buffer_timeout_count = 0;

        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        break;
                    }

                    buffer_cmd = "\r\nERROR\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_deep_sleep error\n");
                        return false;
                    }

                    buffer_cmd = "\r\nbusy p...\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_deep_sleep busy\n");
                        buffer_timeout_count = 0;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_ms / 10)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_deep_sleep timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::init_esp_at(void)
    {
        // enable function 1
        if (_bus->write(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_FN_ENABLE), 6) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_FN_READY), 6) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // enable interrupts for function 1&2 and master enable
        if (_bus->write(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_INT_ENABLE), 7) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_BLKSIZEL), 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_BLKSIZEH), 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(0, static_cast<uint32_t>(0x110), 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        // Set block size 512 (0x200)
        if (_bus->write(0, static_cast<uint32_t>(0x111), 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(0, static_cast<uint32_t>(0x210), 0) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(0, static_cast<uint32_t>(0x210), 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // [157 line]: read SD_IO_CCCR_FN_ENABLE: 0X6
        // [163 line]: read SD_IO_CCCR_FN_READY: 0
        // [171 line]: read SD_IO_CCCR_INT_ENABLE: 0X7
        // [178 line]: read SD_IO_CCCR_BLKSIZEL: 0
        // [184 line]: read SD_IO_CCCR_BLKSIZEH: 0X2
        // [191 line]: read 0x110: 0
        // [198 line]: read 0x111: 0X2
        // [204 line]: read 0x210: 0X2

        // uint8_t buffer = 0;
        // if (_bus->read(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_FN_ENABLE), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read SD_IO_CCCR_FN_ENABLE: %#X\n", buffer);
        // if (_bus->read(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_FN_READY), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read SD_IO_CCCR_FN_READY: %#X\n", buffer);

        // // get interrupt status
        // if (_bus->read(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_INT_ENABLE), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read SD_IO_CCCR_INT_ENABLE: %#X\n", buffer);

        // if (_bus->read(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_BLKSIZEL), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read SD_IO_CCCR_BLKSIZEL: %#X\n", buffer);
        // if (_bus->read(0, static_cast<uint32_t>(Cmd::SD_IO_CCCR_BLKSIZEH), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read SD_IO_CCCR_BLKSIZEH: %#X\n", buffer);

        // if (_bus->read(0, static_cast<uint32_t>(0x110), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read 0x110: %#X\n", buffer);

        // if (_bus->read(0, static_cast<uint32_t>(0x111), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read 0x111: %#X\n", buffer);
        // if (_bus->read(0, static_cast<uint32_t>(0x210), &buffer) == false)
        // {
        //     assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
        //     return false;
        // }
        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read 0x210: %#X\n", buffer);

        return true;
    }

    bool Esp_At::init_connect(void)
    {
        uint8_t buffer_timeout_count = 0;

        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "\r\nready\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        _connect.status = true;
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_at init connect success\n");
                        break;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > ESP_AT_TRANSMIT_TIMEOUT_COUNT)
            {
                _connect.status = false;
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_at connect timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::get_device_id(void)
    {
        const std::string buffer_cmd = "AT\r\n";

        send_packet(buffer_cmd);

        int8_t buffer_timeout_count = 0;
        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp_at connect success\n");
                        break;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > ESP_AT_TRANSMIT_TIMEOUT_COUNT)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_device_id timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::reconnect_esp_at(void)
    {
        _connect.status = true;
        _connect.error_count = 0;

        if (get_device_id() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "esp32_at get_device_id fail,starting reinitialization\n");
            begin();

            return false;
        }

        return true;
    }

    bool Esp_At::get_connect_status(void)
    {
        return _connect.status;
    }

    void Esp_At::assert_connect_count(int8_t count)
    {
        _connect.error_count += count;
        if (_connect.error_count < 0)
        {
            _connect.error_count = 0;
        }
        else if (_connect.error_count > ESP_AT_CONNECT_ERROR_COUNT)
        {
            _connect.error_count = ESP_AT_CONNECT_ERROR_COUNT + 1;
            _connect.status = false;
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_connect.error_count > ESP_AT_CONNECT_ERROR_COUNT\n");
        }
    }

    uint32_t Esp_At::get_irq_flag(void)
    {
        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint32_t buffer = 0;

        if (_bus->read(1, static_cast<uint32_t>(Cmd::INTERRUPT_RAW), &buffer, sizeof(uint32_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            assert_connect_count(1);
            return -1;
        }

        assert_connect_count(-1);
        return buffer;
    }

    bool Esp_At::clear_irq_flag(uint32_t irq_mask)
    {
        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        if (_bus->write(1, static_cast<uint32_t>(Cmd::INTERRUPT_CLEAR), &irq_mask, sizeof(uint32_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            assert_connect_count(1);
            return false;
        }

        assert_connect_count(-1);
        return true;
    }

    bool Esp_At::assert_rx_new_packet_flag(uint32_t flag)
    {
        if (flag == -1)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "flag error\n");
            return false;
        }

        if (((flag & static_cast<uint32_t>(Irq_Flag::RX_NEW_PACKET)) >> 23) == false)
        {
            // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "RX_NEW_PACKET fail\n");
            return false;
        }

        return true;
    }

    uint32_t Esp_At::get_rx_data_length(void)
    {
        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint32_t buffer = 0;

        if (_bus->read(1, static_cast<uint32_t>(Cmd::PACKET_LENGTH), &buffer, sizeof(uint32_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            assert_connect_count(1);
            return false;
        }

        buffer &= RX_BUFFER_MASK;
        buffer = (buffer + RX_BUFFER_MAX - _connect.receive_total_length_index) % RX_BUFFER_MAX;

        assert_connect_count(-1);
        return buffer;
    }

    bool Esp_At::receive_packet(std::vector<uint8_t> &data, uint32_t max_receive_size)
    {
        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint32_t buffer_lenght = get_rx_data_length();

        if (buffer_lenght == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_data_length fail(get_rx_data_length = 0)\n");
            return false;
        }
        else if (buffer_lenght > max_receive_size)
        {
            _connect.status = false;

            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_data_length fail((get_rx_data_length = %u) > max_receive_size)\n", buffer_lenght);
            return false;
        }

        data.resize(buffer_lenght);

        size_t buffer_block_length = (buffer_lenght / ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE) * ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE;
        if (buffer_block_length != 0)
        {
            // 多字节对齐读取
            if (_bus->read_block(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_lenght, data.data(), buffer_block_length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read_block fail\n");
                assert_connect_count(1);
                return false;
            }
            buffer_lenght -= buffer_block_length;

            _connect.receive_total_length_index += buffer_block_length;
        }

        if (buffer_lenght != 0)
        {
            // 4字节对齐读取
            if (_bus->read(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_lenght, data.data() + buffer_block_length, (buffer_lenght + 3) & (~3)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                assert_connect_count(1);
                return false;
            }

            _connect.receive_total_length_index += buffer_lenght;
        }

        assert_connect_count(-1);
        return true;
    }

    bool Esp_At::receive_packet(uint8_t *data, size_t *byte, uint32_t max_receive_size)
    {
        if (data == nullptr)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "data is nullptr\n");
            return false;
        }

        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint32_t buffer_lenght = get_rx_data_length();

        if (buffer_lenght == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_data_length fail\n");
            return false;
        }
        else if (buffer_lenght > max_receive_size)
        {
            _connect.status = false;

            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_data_length fail((get_rx_data_length = %u) > max_receive_size)\n", buffer_lenght);
            return false;
        }

        *byte = buffer_lenght;

        size_t buffer_block_length = (buffer_lenght / ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE) * ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE;
        if (buffer_block_length != 0)
        {
            // 多字节对齐读取
            if (_bus->read_block(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_lenght, data, buffer_block_length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read_block fail\n");
                *byte = 0;
                assert_connect_count(1);
                return false;
            }
            buffer_lenght -= buffer_block_length;

            _connect.receive_total_length_index += buffer_block_length;
        }

        if (buffer_lenght != 0)
        {
            // 4字节对齐读取
            if (_bus->read(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_lenght, data + buffer_block_length, (buffer_lenght + 3) & (~3)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                *byte = 0;
                assert_connect_count(1);
                return false;
            }

            _connect.receive_total_length_index += buffer_lenght;
        }

        assert_connect_count(-1);
        return true;
    }

    bool Esp_At::receive_packet(std::unique_ptr<uint8_t[]> &data, size_t *byte, uint32_t max_receive_size)
    {
        if (data == nullptr)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "data is nullptr\n");
            return false;
        }

        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint32_t buffer_lenght = get_rx_data_length();

        if (buffer_lenght == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_data_length fail\n");
            return false;
        }
        else if (buffer_lenght > max_receive_size)
        {
            _connect.status = false;

            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_data_length fail((get_rx_data_length = %u) > max_receive_size)\n", buffer_lenght);
            return false;
        }

        *byte = buffer_lenght;

        data = std::make_unique<uint8_t[]>(buffer_lenght);

        size_t buffer_block_length = (buffer_lenght / ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE) * ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE;
        if (buffer_block_length != 0)
        {
            // 多字节对齐读取
            if (_bus->read_block(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_lenght, data.get(), buffer_block_length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read_block fail\n");
                *byte = 0;
                assert_connect_count(1);
                return false;
            }
            buffer_lenght -= buffer_block_length;

            _connect.receive_total_length_index += buffer_block_length;
        }

        if (buffer_lenght != 0)
        {
            // 4字节对齐读取
            if (_bus->read(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_lenght, data.get() + buffer_block_length, (buffer_lenght + 3) & (~3)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                *byte = 0;
                assert_connect_count(1);
                return false;
            }

            _connect.receive_total_length_index += buffer_lenght;
        }

        assert_connect_count(-1);
        return true;
    }

    uint32_t Esp_At::get_tx_block_buffer_length(void)
    {
        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint32_t buffer = 0;

        if (_bus->read(1, static_cast<uint32_t>(Cmd::TOKEN_RDATA), &buffer, sizeof(uint32_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            assert_connect_count(1);
            return false;
        }

        return (buffer >> TX_BUFFER_OFFSET) & TX_BUFFER_MASK;
    }

    bool Esp_At::send_packet(const char *data, size_t byte)
    {
        if (data == nullptr || byte == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid input parameters\n");
            return false;
        }

        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint16_t buffer_timeout_count = 0;

        while (1)
        {
            if (get_tx_block_buffer_length() * ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE >= byte)
            {
                break;
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > 100) // 超时
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_tx_block_buffer_length timeout\n");
                return false;
            }

            delay_ms(10);
        }

        size_t buffer_block_length = (byte / ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE) * ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE;
        if (buffer_block_length != 0)
        {
            // 多字节对齐发送
            if (_bus->write_block(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - byte, data, buffer_block_length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_block fail\n");
                assert_connect_count(1);
                return false;
            }
            byte -= buffer_block_length;
        }

        if (byte != 0)
        {
            // 4字节对齐发送
            if (_bus->write(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - byte, data + buffer_block_length, (byte + 3) & (~3)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                assert_connect_count(1);
                return false;
            }
        }

        assert_connect_count(-1);
        return true;
    }

    bool Esp_At::send_packet(const std::string data)
    {
        if (data.size() == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid input parameters\n");
            return false;
        }

        if (_connect.status == false)
        {
            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "esp_at connect fail\n");
            return false;
        }

        uint16_t buffer_timeout_count = 0;
        size_t buffer_length = data.length();

        while (1)
        {
            if (get_tx_block_buffer_length() * ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE >= buffer_length)
            {
                break;
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > 100) // 超时
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_tx_block_buffer_length timeout\n");
                return false;
            }

            delay_ms(10);
        }

        size_t buffer_block_length = (buffer_length / ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE) * ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE;
        if (buffer_block_length != 0)
        {
            // 多字节对齐发送
            if (_bus->write_block(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_length, data.data(), buffer_block_length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_block fail\n");
                assert_connect_count(1);
                return false;
            }
            buffer_length -= buffer_block_length;
        }

        if (buffer_length != 0)
        {
            // 4字节对齐发送
            if (_bus->write(1, static_cast<uint32_t>(Cmd::SLAVE_CMD53_END_ADDR) - buffer_length, data.data() + buffer_block_length, (buffer_length + 3) & (~3)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                assert_connect_count(1);
                return false;
            }
        }

        assert_connect_count(-1);
        return true;
    }

    bool Esp_At::set_wifi_mode(Wifi_Mode mode, int16_t timeout_ms)
    {
        const char *buffer = nullptr;

        switch (mode)
        {
        case Wifi_Mode::OFF:
            buffer = "AT+CWMODE=0\r\n";
            break;
        case Wifi_Mode::STATION:
            buffer = "AT+CWMODE=1\r\n";
            break;
        case Wifi_Mode::SOFTAP:
            buffer = "AT+CWMODE=2\r\n";
            break;
        case Wifi_Mode::STATION_SOFTAP:
            buffer = "AT+CWMODE=3\r\n";
            break;

        default:
            break;
        }

        send_packet(buffer, strlen(buffer));

        int16_t buffer_timeout_count = 0;

        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        break;
                    }

                    buffer_cmd = "\r\nERROR\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_wifi_mode error\n");
                        return false;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_ms / 10)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_wifi_mode timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::wifi_scan(std::vector<uint8_t> &data, int16_t timeout_ms)
    {
        const char *buffer = "AT+CWLAP\r\n";

        send_packet(buffer, strlen(buffer));

        int16_t buffer_timeout_count = 0;

        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "+CWLAP:";
                    size_t buffer_index = 0;
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
                    {
                        buffer.pop_back(); // 这里移除末尾的'\0'否则每个数据后都会有'\0'
                        data.insert(data.end(), buffer.begin() + buffer_index + strlen(buffer_cmd), buffer.end());
                    }

                    buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        data.push_back('\0'); // 数据末尾加上'\0'使数据有终止符号
                        break;
                    }

                    buffer_cmd = "\r\nERROR\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "wifi_scan error\n");
                        return false;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_ms / 10)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "wifi_scan timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::wait_interrupt(uint32_t timeout_ms)
    {
        if (_bus->wait_interrupt(timeout_ms) == false)
        {
            // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "wait_interrupt fail\n");
            return false;
        }

        return true;
    }

    bool Esp_At::set_flash_save(bool enable, int16_t timeout_ms)
    {
        std::string buffer;
        if (enable == true)
        {
            buffer = "AT+SYSSTORE=1\r\n"; // 存入flash
        }
        else
        {
            buffer = "AT+SYSSTORE=0\r\n"; // 不存入flash
        }

        send_packet(buffer);

        int16_t buffer_timeout_count = 0;

        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        break;
                    }

                    buffer_cmd = "\r\nERROR\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_wifi_connect error\n");
                        return false;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_ms / 10)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_wifi_connect timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::set_wifi_connect(std::string ssid, std::string password, int16_t timeout_ms)
    {
        std::string buffer = "AT+CWJAP=\"" + ssid + "\"" + ",\"";
        if (password != "")
        {
            buffer = buffer + password + "\"\r\n";
        }

        send_packet(buffer);

        int16_t buffer_timeout_count = 0;

        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        break;
                    }

                    buffer_cmd = "\r\nERROR\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_wifi_connect error\n");
                        return false;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_ms / 10)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_wifi_mode timeout\n");
                return false;
            }

            delay_ms(10);
        }

        return true;
    }

    bool Esp_At::get_real_time(Real_Time &time, int16_t timeout_ms)
    {
        const std::string buffer = "AT+HTTPCLIENT=1,0,\"http://httpbin.org/get\",,,1\r\n";

        send_packet(buffer);

        int16_t buffer_timeout_count = 0;
        std::vector<uint8_t> buffer_data;
        while (1)
        {
            uint32_t flag = get_irq_flag();
            if (assert_rx_new_packet_flag(flag) == true)
            {
                // 中断后必须马上进行清除标志
                clear_irq_flag(flag);

                buffer_timeout_count--;
                if (buffer_timeout_count < 0)
                {
                    buffer_timeout_count = 0;
                }

                std::vector<uint8_t> buffer;
                if (receive_packet(buffer) == true)
                {
                    // 获取的字符末尾必须要加'\0'才能进行search否则会触发非法输入
                    buffer.push_back('\0');
                    // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "receive_packet lenght: [%d] receive: \n[%s]\n", buffer.size(), buffer.data());

                    const char *buffer_cmd = ",Date: ";
                    size_t buffer_index = 0;
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
                    {
                        buffer.pop_back(); // 这里移除末尾的'\0'否则每个数据后都会有'\0'
                        buffer_data.insert(buffer_data.end(), buffer.begin() + buffer_index + strlen(buffer_cmd), buffer.end());
                    }

                    buffer_cmd = "\r\nOK\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        buffer_data.push_back('\0'); // 数据末尾加上'\0'使数据有终止符号
                        break;
                    }

                    buffer_cmd = "\r\nERROR\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_real_time error\n");
                        return false;
                    }

                    buffer_cmd = "\r\nbusy p...\r\n";
                    if (search(buffer.data(), buffer.size(), buffer_cmd, std::strlen(buffer_cmd)) == true)
                    {
                        assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_real_time busy\n");
                        buffer_timeout_count = 0;
                    }
                }
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_ms / 10)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_real_time timeout\n");
                return false;
            }

            delay_ms(10);
        }

        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_data: [%s]\n", buffer_data.data());

        const char *buffer_cmd = " ";
        size_t buffer_index = 0;
        size_t buffer_Index_used = 0;
        if (buffer_data.size() == 0)
        {
            // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail(buffer_data.size() == 0)\n");
            return false;
        }
        if (search(buffer_data.data(), buffer_data.size(), buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            //-1去除","字符
            time.week.assign(buffer_data.begin(), buffer_data.begin() + (buffer_index - 1));
        }
        buffer_Index_used = buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

        if (search(buffer_data.data() + buffer_Index_used, buffer_data.size() - buffer_Index_used, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            std::string buffer(buffer_data.begin() + buffer_Index_used, buffer_data.begin() + buffer_Index_used + buffer_index);
            time.day = std::stoi(buffer.c_str());
        }
        buffer_Index_used = buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

        if (search(buffer_data.data() + buffer_Index_used, buffer_data.size() - buffer_Index_used, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            std::string buffer_month(buffer_data.begin() + buffer_Index_used, buffer_data.begin() + buffer_Index_used + buffer_index);
            uint8_t buffer_month_2 = 0;
            for (uint8_t i = 0; i < sizeof(_time_month_list); i++)
            {
                if (search(_time_month_list[i], std::strlen((const char *)_time_month_list[i]), buffer_month.data(), std::strlen(buffer_month.c_str())) == true)
                {
                    buffer_month_2 = i + 1;
                    break;
                }
            }
            time.month = buffer_month_2;
        }
        buffer_Index_used = buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

        if (search(buffer_data.data() + buffer_Index_used, buffer_data.size() - buffer_Index_used, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            std::string buffer(buffer_data.begin() + buffer_Index_used, buffer_data.begin() + buffer_Index_used + buffer_index);
            time.year = std::stoi(buffer.c_str());
        }
        buffer_Index_used = buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

        buffer_cmd = ":";
        if (search(buffer_data.data() + buffer_Index_used, buffer_data.size() - buffer_Index_used, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            std::string buffer(buffer_data.begin() + buffer_Index_used, buffer_data.begin() + buffer_Index_used + buffer_index);
            time.hour = std::stoi(buffer.c_str());
        }
        buffer_Index_used = buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

        if (search(buffer_data.data() + buffer_Index_used, buffer_data.size() - buffer_Index_used, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            std::string buffer(buffer_data.begin() + buffer_Index_used, buffer_data.begin() + buffer_Index_used + buffer_index);
            time.minute = std::stoi(buffer.c_str());
        }
        buffer_Index_used = buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

        buffer_cmd = " ";
        if (search(buffer_data.data() + buffer_Index_used, buffer_data.size() - buffer_Index_used, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            std::string buffer(buffer_data.begin() + buffer_Index_used, buffer_data.begin() + buffer_Index_used + buffer_index);
            time.second = std::stoi(buffer.c_str());
        }
        buffer_Index_used = buffer_Index_used + buffer_index + std::strlen(buffer_cmd);

        // 结束
        buffer_cmd = "\r\n";
        if (search(buffer_data.data() + buffer_Index_used, buffer_data.size() - buffer_Index_used, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == true)
        {
            time.time_zone.assign(buffer_data.begin() + buffer_Index_used, buffer_data.begin() + buffer_Index_used + buffer_index);
        }

        return true;
    }

}
