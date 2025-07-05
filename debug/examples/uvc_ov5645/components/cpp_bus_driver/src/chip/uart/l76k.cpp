/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-07-05 13:47:26
 * @License: GPL 3.0
 */
#include "l76k.h"

namespace Cpp_Bus_Driver
{
    bool L76k::begin(int32_t baud_rate)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
        }

        if (_wake_up != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            if (pin_mode(_wake_up, Pin_Mode::OUTPUT, Pin_Status::PULLUP) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "pin_mode fail\n");
            }
        }

        if (sleep(false) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "sleep fail\n");
        }

        if (Uart_Guide::begin(baud_rate) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        size_t buffer_index = 0;
        if (get_device_id(&buffer_index) == false)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get l76k id fail\n");
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get l76k id success,index: %d\n", buffer_index);
        }

        // if (_bus->BufferOperation(_address, PCF85063_Initialization_BufferOperations,
        //                           sizeof(PCF85063_Initialization_BufferOperations)) == false)
        // {
        //     return false;
        // }

        return true;
    }

    bool L76k::sleep(bool enable)
    {
        if (_wake_up != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            if (pin_write(_wake_up, !enable) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "pin_write fail\n");
                return false;
            }
        }
        else if (_wake_up_callback != nullptr)
        {
            if (_wake_up_callback(!enable) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_wake_up_callback fail\n");
                return false;
            }
        }

        return true;
    }

    bool L76k::get_device_id(size_t *search_index)
    {
        std::shared_ptr<uint8_t[]> buffer;
        uint32_t buffer_lenght = 0;

        if (get_info_data(buffer, &buffer_lenght) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_info_data fail\n");
            return false;
        }

        const char *buffer_cmd = "\r\n$G";
        if (search(buffer.get(), buffer_lenght, buffer_cmd, std::strlen(buffer_cmd), search_index) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
            return false;
        }

        return true;
    }

    uint32_t L76k::read_data(uint8_t *data, uint32_t length)
    {
        uint32_t length_buffer = _bus->get_rx_buffer_length();
        if (length_buffer == 0)
        {
            // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_buffer_length is empty\n");
            return false;
        }

        if ((length == 0) || (length >= length_buffer))
        {
            if (_bus->read(data, length_buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }
        else if (length < length_buffer)
        {
            if (_bus->read(data, length) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }

        return length_buffer;
    }

    size_t L76k::get_rx_buffer_length(void)
    {
        return _bus->get_rx_buffer_length();
    }

    bool L76k::clear_rx_buffer_data(void)
    {
        return _bus->clear_rx_buffer_data();
    }

    bool L76k::get_info_data(std::shared_ptr<uint8_t[]> &data, uint32_t *length, uint32_t max_length, uint8_t timeout_count)
    {
        uint8_t buffer_timeout_count = 0;

        while (1)
        {
            delay_ms(_update_freq);

            int32_t buffer_lenght = get_rx_buffer_length();
            if (buffer_lenght > max_length)
            {
                buffer_lenght = max_length;
            }

            if (buffer_lenght > 0)
            {
                data = std::make_shared<uint8_t[]>(buffer_lenght);
                if (data == nullptr)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "data std::make_shared fail\n");
                    data = nullptr;
                    *length = 0;
                    return false;
                }

                buffer_lenght = _bus->read(data.get(), buffer_lenght);
                if (buffer_lenght == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                    data = nullptr;
                    *length = 0;
                    return false;
                }

                assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "get_info_data lenght: %d\n", buffer_lenght);
                *length = buffer_lenght;
                break;
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_count) // 超时
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read timeout\n");
                data = nullptr;
                *length = 0;
                return false;
            }
        }

        return true;
    }

    bool L76k::parse_rmc_info(std::shared_ptr<uint8_t[]> data, size_t length, Rmc &rmc)
    {
        assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "parse_rmc_info(length: %d): \n---begin---\n%s\n---end---\n", length, data.get());

        size_t buffer_index = 0;
        const char *buffer_cmd = "$GNRMC";
        bool buffer_exit_flag = false;
        size_t buffer_used_size = 0;

        // 循环搜索数据里面所有的命令
        while (1)
        {
            if (buffer_exit_flag == true)
            {
                break;
            }

            if (search(data.get() + buffer_used_size, length - buffer_used_size, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == false)
            {
                // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                buffer_exit_flag = true;
            }
            else
            {
                buffer_used_size += buffer_index;
                assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "$GNRMC index: %d\n", buffer_used_size);

                buffer_used_size += std::strlen(buffer_cmd);

                uint8_t buffer_field_count = 0; // 记录当前的字段号
                buffer_exit_flag = true;
                for (auto i = buffer_used_size; i < length; i++)
                {
                    if ((data[i] == '\r') && (data[i + 1] == '\n')) // 停止符
                    {
                        break;
                    }
                    else if (data[i] == ',')
                    {
                        buffer_field_count++;
                        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_field_count++\n");
                    }
                    else
                    {
                        switch (buffer_field_count)
                        {
                        case 1: //<UTC>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 10)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<UTC> length error((length = %d) != 10)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = false;
                                break;
                            }

                            char buffer_hour[] = {data[i], data[i + 1], '\0'};
                            char buffer_minute[] = {data[i + 2], data[i + 3], '\0'};
                            // data[i + 6]为小数点
                            char buffer_second[] =
                                {
                                    data[i + 4],
                                    data[i + 5],
                                    data[i + 6],
                                    data[i + 7],
                                    data[i + 8],
                                    data[i + 9],
                                    '\0',
                                };

                            rmc.utc.hour = std::stoi(buffer_hour);
                            rmc.utc.minute = std::stoi(buffer_minute);
                            rmc.utc.second = std::stof(buffer_second, nullptr);
                            rmc.utc.update_flag = true;

                            i += (10 - 1);
                            break;
                        }
                        case 2: //<Status>
                            rmc.location_status = data[i];
                            break;
                        case 3: //<Lat>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 10)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<Lat> length error((length = %d) != 10)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = false;
                                break;
                            }

                            char buffer_lat_degrees[] = {data[i], data[i + 1], '\0'};
                            char buffer_lat_minutes[] =
                                {
                                    data[i + 2],
                                    data[i + 3],
                                    data[i + 4],
                                    data[i + 5],
                                    data[i + 6],
                                    data[i + 7],
                                    data[i + 8],
                                    data[i + 9],
                                    '\0',
                                };

                            rmc.location.lat.degrees = std::stoi(buffer_lat_degrees);
                            rmc.location.lat.minutes = std::stof(buffer_lat_minutes, nullptr);
                            rmc.location.lat.degrees_minutes = static_cast<double>(rmc.location.lat.degrees) + (rmc.location.lat.minutes / 60.0);

                            rmc.location.lat.update_flag = true;

                            i += (10 - 1);
                            break;
                        }
                        case 4: //<N/S>
                            rmc.location.lat.direction = data[i];

                            rmc.location.lat.direction_update_flag = true;
                            break;
                        case 5: //<Lon>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 11)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<Lon> length error((length = %d) != 11)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = false;
                                break;
                            }

                            char buffer_lon_degrees[] = {data[i], data[i + 1], data[i + 2], '\0'};
                            char buffer_lon_minutes[] =
                                {
                                    data[i + 3],
                                    data[i + 4],
                                    data[i + 5],
                                    data[i + 6],
                                    data[i + 7],
                                    data[i + 8],
                                    data[i + 9],
                                    data[i + 10],
                                    '\0',
                                };

                            rmc.location.lon.degrees = std::stoi(buffer_lon_degrees);
                            rmc.location.lon.minutes = std::stof(buffer_lon_minutes, nullptr);
                            rmc.location.lon.degrees_minutes = static_cast<double>(rmc.location.lon.degrees) + (rmc.location.lon.minutes / 60.0);

                            rmc.location.lon.update_flag = true;

                            i += (11 - 1);
                            break;
                        }
                        case 6: //<E/W>
                            rmc.location.lon.direction = data[i];

                            rmc.location.lon.direction_update_flag = true;
                            break;
                        case 7:
                            break;
                        case 8:
                            break;
                        case 9: //<Date>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 6)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<Date> length error((length = %d) != 6)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = false;
                                break;
                            }

                            char buffer_day[] = {data[i], data[i + 1], '\0'};
                            char buffer_month[] = {data[i + 2], data[i + 3], '\0'};
                            char buffer_year[] = {data[i + 4], data[i + 5], '\0'};

                            rmc.data.day = std::stoi(buffer_day);
                            rmc.data.month = std::stoi(buffer_month);
                            rmc.data.year = std::stoi(buffer_year);
                            rmc.data.update_flag = true;

                            i += (6 - 1);
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }

                assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "parse_rmc_info finish(field count: %d)\n", buffer_field_count);
            }
        }

        return true;
    }

    bool L76k::parse_gga_info(std::shared_ptr<uint8_t[]> data, size_t length, Gga &gga)
    {
        assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "parse_gga_info(length: %d): \n---begin---\n%s\n---end---\n", length, data.get());

        size_t buffer_index = 0;
        const char *buffer_cmd = "$GNGGA";
        bool buffer_exit_flag = false;
        size_t buffer_used_size = 0;

        // 循环搜索数据里面所有的命令
        while (1)
        {
            if (search(data.get() + buffer_used_size, length - buffer_used_size, buffer_cmd, std::strlen(buffer_cmd), &buffer_index) == false)
            {
                // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                break;
            }
            else
            {
                buffer_used_size += buffer_index;
                assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "$GNGGA index: %d\n", buffer_used_size);

                buffer_used_size += std::strlen(buffer_cmd);

                uint8_t buffer_field_count = 0; // 记录当前的字段号
                for (auto i = buffer_used_size; i < length; i++)
                {
                    if ((data[i] == '\r') && (data[i + 1] == '\n')) // 停止符
                    {
                        break;
                    }
                    else if (data[i] == ',')
                    {
                        buffer_field_count++;
                        // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_field_count++\n");
                    }
                    else
                    {
                        switch (buffer_field_count)
                        {
                        case 1: //<UTC>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 10)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<UTC> length error((length = %d) != 10)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = true;
                                break;
                            }

                            char buffer_hour[] = {data[i], data[i + 1], '\0'};
                            char buffer_minute[] = {data[i + 2], data[i + 3], '\0'};
                            // data[i + 6]为小数点
                            char buffer_second[] =
                                {
                                    data[i + 4],
                                    data[i + 5],
                                    data[i + 6],
                                    data[i + 7],
                                    data[i + 8],
                                    data[i + 9],
                                    '\0',
                                };

                            gga.utc.hour = std::stoi(buffer_hour);
                            gga.utc.minute = std::stoi(buffer_minute);
                            gga.utc.second = std::stof(buffer_second, nullptr);
                            gga.utc.update_flag = true;

                            i += (10 - 1);
                            break;
                        }
                        case 2: //<Lat>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 10)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<Lat> length error((length = %d) != 10)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = true;
                                break;
                            }

                            char buffer_lat_degrees[] = {data[i], data[i + 1], '\0'};
                            char buffer_lat_minutes[] =
                                {
                                    data[i + 2],
                                    data[i + 3],
                                    data[i + 4],
                                    data[i + 5],
                                    data[i + 6],
                                    data[i + 7],
                                    data[i + 8],
                                    data[i + 9],
                                    '\0',
                                };

                            gga.location.lat.degrees = std::stoi(buffer_lat_degrees);
                            gga.location.lat.minutes = std::stof(buffer_lat_minutes, nullptr);
                            gga.location.lat.degrees_minutes = static_cast<double>(gga.location.lat.degrees) + (gga.location.lat.minutes / 60.0);

                            gga.location.lat.update_flag = true;

                            i += (10 - 1);
                            break;
                        }
                        case 3: //<N/S>
                            gga.location.lat.direction = data[i];

                            gga.location.lat.direction_update_flag = true;
                            break;
                        case 4: //<Lon>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 11)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<Lon> length error((length = %d) != 11)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = true;
                                break;
                            }

                            char buffer_lon_degrees[] = {data[i], data[i + 1], data[i + 2], '\0'};
                            char buffer_lon_minutes[] =
                                {
                                    data[i + 3],
                                    data[i + 4],
                                    data[i + 5],
                                    data[i + 6],
                                    data[i + 7],
                                    data[i + 8],
                                    data[i + 9],
                                    data[i + 10],
                                    '\0',
                                };

                            gga.location.lon.degrees = std::stoi(buffer_lon_degrees);
                            gga.location.lon.minutes = std::stof(buffer_lon_minutes, nullptr);
                            gga.location.lon.degrees_minutes = static_cast<double>(gga.location.lon.degrees) + (gga.location.lon.minutes / 60.0);

                            gga.location.lon.update_flag = true;

                            i += (11 - 1);
                            break;
                        }
                        case 5: //<E/W>
                            gga.location.lon.direction = data[i];

                            gga.location.lon.direction_update_flag = true;
                            break;
                        case 6: //<Quality>
                        {
                            char buffer_gps_mode_status[] = {data[i], '\0'};

                            gga.gps_mode_status = std::stoi(buffer_gps_mode_status);
                            break;
                        }
                        case 7: //<NumSatUsed>
                        {
                            // 确保数据长度正确
                            size_t buffer_index_2 = 0;
                            const char *buffer_cmd_2 = ",";
                            if (search(data.get() + i, length - i, buffer_cmd_2, std::strlen(buffer_cmd_2), &buffer_index_2) == false)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }
                            if (buffer_index_2 != 2)
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "<NumSatUsed> length error((length = %d) != 2)\n", buffer_index_2);
                                i += (buffer_index_2 - 1);
                                buffer_exit_flag = true;
                                break;
                            }

                            char buffer_online_satellite_count[] = {data[i], data[i + 1], '\0'};

                            gga.online_satellite_count = std::stoi(buffer_online_satellite_count);

                            i += (2 - 1);
                            break;
                        }
                        case 8: // <HDOP>
                        {
                            size_t buffer_index_3 = 0;

                            if (search(data.get() + i, length - i, ",", 1, &buffer_index_3) == false) // 搜索下一个
                            {
                                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
                                break;
                            }

                            char buffer_1[buffer_index_3 + 1] = {0};

                            memcpy(buffer_1, data.get() + i, buffer_index_3);

                            buffer_1[buffer_index_3] = '\0';

                            gga.hdop = std::stof(buffer_1, nullptr); // 转为float

                            i += (buffer_index_3 - 1);
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }

                assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "buffer_field_count: %d\n", buffer_field_count);
            }
        }

        return true;
    }

    bool L76k::set_update_frequency(Update_Freq freq)
    {
        const char *buffer = nullptr;

        switch (freq)
        {
        case Update_Freq::FREQ_1HZ:
            buffer = "$PCAS02,1000*2E\r\n";
            _update_freq = 1000;
            break;
        case Update_Freq::FREQ_2HZ:
            buffer = "$PCAS02,500*1A\r\n";
            _update_freq = 500;
            break;
        case Update_Freq::FREQ_5HZ:
            buffer = "$PCAS02,200*1D\r\n";
            _update_freq = 200;
            break;
        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool L76k::set_baud_rate(Baud_Rate baud_rate)
    {
        const char *buffer = nullptr;

        switch (baud_rate)
        {
        case Baud_Rate::BR_4800_BPS:
            buffer = "$PCAS01,0*1C\r\n";
            break;
        case Baud_Rate::BR_9600_BPS:
            buffer = "$PCAS01,1*1D\r\n";
            break;
        case Baud_Rate::BR_19200_BPS:
            buffer = "$PCAS01,2*1E\r\n";
            break;
        case Baud_Rate::BR_38400_BPS:
            buffer = "$PCAS01,3*1F\r\n";
            break;
        case Baud_Rate::BR_57600_BPS:
            buffer = "$PCAS01,4*18\r\n";
            break;
        case Baud_Rate::BR_115200_BPS:
            buffer = "$PCAS01,5*19\r\n";
            break;

        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        // 只有设置波特率时候需要延时
        //  因为没有忙总线所以这里写入数据需要在模块未发送数据空闲的时候写，所以要延时，延时时间为更新频率的一半
        delay_ms(_update_freq / 2);
        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        switch (baud_rate)
        {
        case Baud_Rate::BR_4800_BPS:
            if (_bus->set_baud_rate(4800) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_9600_BPS:
            if (_bus->set_baud_rate(9600) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_19200_BPS:
            if (_bus->set_baud_rate(19200) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_38400_BPS:
            if (_bus->set_baud_rate(38400) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_57600_BPS:
            if (_bus->set_baud_rate(57600) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_115200_BPS:
            if (_bus->set_baud_rate(115200) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;

        default:
            break;
        }

        return true;
    }

    uint32_t L76k::get_baud_rate(void)
    {
        return _bus->get_baud_rate();
    }

    bool L76k::set_restart_mode(Restart_Mode mode)
    {
        const char *buffer = nullptr;

        switch (mode)
        {
        case Restart_Mode::HOT_START:
            buffer = "$PCAS10,0*1C\r\n";
            break;
        case Restart_Mode::WARM_START:
            buffer = "$PCAS10,1*1D\r\n";
            break;
        case Restart_Mode::COLD_START:
            buffer = "$PCAS10,2*1E\r\n";
            break;
        case Restart_Mode::COLD_START_FACTORY_RESET:
            buffer = "$PCAS10,3*1F\r\n";
            break;
        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool L76k::set_gnss_constellation(Gnss_Constellation constellation)
    {
        const char *buffer = nullptr;

        switch (constellation)
        {
        case Gnss_Constellation::GPS:
            buffer = "$PCAS04,1*18\r\n";
            break;
        case Gnss_Constellation::BEIDOU:
            buffer = "$PCAS04,2*1B\r\n";
            break;
        case Gnss_Constellation::GPS_BEIDOU:
            buffer = "$PCAS04,3*1A\r\n";
            break;
        case Gnss_Constellation::GLONASS:
            buffer = "$PCAS04,4*1D\r\n";
            break;
        case Gnss_Constellation::GPS_GLONASS:
            buffer = "$PCAS04,5*1C\r\n";
            break;
        case Gnss_Constellation::BEIDOU_GLONASS:
            buffer = "$PCAS04,6*1F\r\n";
            break;
        case Gnss_Constellation::GPS_BEIDOU_GLONASS:
            buffer = "$PCAS04,7*1E\r\n";
            break;
        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
