/*
 * @Description:
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-07-04 18:32:33
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

        size_t index = 0;
        if (get_device_id(&index) == false)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get l76k id fail\n");
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get l76k id success,index: %d\n", index);
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

    bool L76k::search_command(const char *cmd, const uint8_t *data, size_t data_length, size_t *search_index)
    {
        auto buffer = std::search(data, data + data_length, cmd, cmd + std::strlen(cmd));
        // 检查是否找到了字符串
        if (buffer == (data + data_length))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
            return false;
        }

        if (search_index != nullptr)
        {
            *search_index = buffer - data;
        }

        return true;
    }

    bool L76k::get_device_id(size_t *search_index)
    {
        std::shared_ptr<uint8_t[]> buffer;
        size_t buffer_lenght = 0;

        if (get_info_data(buffer, &buffer_lenght) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_info_data fail\n");
            return false;
        }

        if (search_command("\r\n$G", buffer.get(), buffer_lenght, search_index) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search_command fail\n");
            return false;
        }

        return true;
    }

    uint32_t L76k::read_data(uint8_t *data, uint32_t length)
    {
        size_t length_buffer = _bus->get_rx_buffer_length();
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

    bool L76k::get_info_data(std::shared_ptr<uint8_t[]> &data, size_t *length)
    {
        uint8_t timeout_count = 0;

        while (1)
        {
            delay_ms(_update_freq);

            size_t buffer_lenght = get_rx_buffer_length();
            if (buffer_lenght > 0)
            {
                data = std::make_shared<uint8_t[]>(buffer_lenght);

                if (read_data(data.get()) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read_data fail\n");
                    data = nullptr;
                    *length = 0;
                    return false;
                }

                *length = buffer_lenght;
                break;
            }

            timeout_count++;
            if (timeout_count > GET_INFORMATION_TIMEOUT_COUNT) // 超时
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_device_id find timeout\n");
                data = nullptr;
                *length = 0;
                return false;
            }
        }

        return true;
    }

    bool L76k::parse_rmc_info(std::shared_ptr<uint8_t[]> data, size_t length, Rmc &rmc)
    {
        size_t index = 0;
        const char *cmd = "$GNRMC";
        if (search_command(cmd, data.get(), length, &index) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search_command fail\n");
        }
        else
        {
            // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "\n---begin---\n%s \n---end---\n$GNRMC index: %d\n", data.get(), index);

            uint8_t buffer_field_count = 0; // 记录当前的字段号
            for (size_t i = index + std::strlen(cmd); i < length; i++)
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
                        char buffer_hour[2] = {data[i], data[i + 1]};
                        char buffer_minute[2] = {data[i + 2], data[i + 3]};
                        // data[i + 6]为小数点
                        char buffer_second[6] =
                            {
                                data[i + 4],
                                data[i + 5],
                                data[i + 6],
                                data[i + 7],
                                data[i + 8],
                                data[i + 9],
                            };

                        rmc.utc.hour = std::stoi(buffer_hour);
                        rmc.utc.minute = std::stoi(buffer_minute);
                        rmc.utc.second = std::strtof(buffer_second, nullptr);
                        rmc.utc.update_flag = true;

                        i += (10 - 1);
                        break;
                    }
                    case 2: //<Status>
                        rmc.location_status = data[i];
                        break;
                    case 3: //<Lat>
                    {
                        char buffer_lat_degrees[2] = {data[i], data[i + 1]};
                        char buffer_lat_minutes[8] =
                            {
                                data[i + 2],
                                data[i + 3],
                                data[i + 4],
                                data[i + 5],
                                data[i + 6],
                                data[i + 7],
                                data[i + 8],
                                data[i + 9],
                            };

                        rmc.location.lat.degrees = std::stoi(buffer_lat_degrees);
                        rmc.location.lat.minutes = std::strtof(buffer_lat_minutes, nullptr);
                        rmc.location.lat.degrees_minutes = static_cast<float>(rmc.location.lat.degrees) + (rmc.location.lat.minutes / 6.0);

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
                        char buffer_lon_degrees[3] = {data[i], data[i + 1], data[i + 2]};
                        char buffer_lon_minutes[8] =
                            {
                                data[i + 3],
                                data[i + 4],
                                data[i + 5],
                                data[i + 6],
                                data[i + 7],
                                data[i + 8],
                                data[i + 9],
                                data[i + 10],
                            };

                        rmc.location.lon.degrees = std::stoi(buffer_lon_degrees);
                        rmc.location.lon.minutes = std::strtof(buffer_lon_minutes, nullptr);
                        rmc.location.lon.degrees_minutes = static_cast<float>(rmc.location.lon.degrees) + (rmc.location.lon.minutes / 6.0);

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

                        char buffer_day[2] = {data[i], data[i + 1]};
                        char buffer_month[2] = {data[i + 2], data[i + 3]};
                        char buffer_year[2] = {data[i + 4], data[i + 5]};

                        rmc.data.day = std::stoi(buffer_day);
                        rmc.data.month = std::stoi(buffer_month);
                        rmc.data.year = std::stoi(buffer_year);
                        rmc.utc.update_flag = true;

                        i += (6 - 1);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_field_count: %d\n", buffer_field_count);
        }

        return true;
    }

    bool L76k::parse_gga_info(std::shared_ptr<uint8_t[]> data, size_t length, Gga &gga)
    {
        size_t index = 0;
        const char *cmd = "$GNGGA";
        if (search_command(cmd, data.get(), length, &index) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search_command fail\n");
        }
        else
        {
            // assert_log(Log_Level::CHIP, __FILE__, __LINE__, "\n---begin---\n%s \n---end---\n$GNGGA index: %d\n", data.get(), index);

            uint8_t buffer_field_count = 0; // 记录当前的字段号
            for (size_t i = index + std::strlen(cmd); i < length; i++)
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
                        char buffer_hour[2] = {data[i], data[i + 1]};
                        char buffer_minute[2] = {data[i + 2], data[i + 3]};
                        // data[i + 6]为小数点
                        char buffer_second[6] =
                            {
                                data[i + 4],
                                data[i + 5],
                                data[i + 6],
                                data[i + 7],
                                data[i + 8],
                                data[i + 9],
                            };

                        gga.utc.hour = std::stoi(buffer_hour);
                        gga.utc.minute = std::stoi(buffer_minute);
                        gga.utc.second = std::strtof(buffer_second, nullptr);
                        gga.utc.update_flag = true;

                        i += (10 - 1);
                        break;
                    }
                    case 2: //<Lat>
                    {
                        char buffer_lat_degrees[2] = {data[i], data[i + 1]};
                        char buffer_lat_minutes[8] =
                            {
                                data[i + 2],
                                data[i + 3],
                                data[i + 4],
                                data[i + 5],
                                data[i + 6],
                                data[i + 7],
                                data[i + 8],
                                data[i + 9],
                            };

                        gga.location.lat.degrees = std::stoi(buffer_lat_degrees);
                        gga.location.lat.minutes = std::strtof(buffer_lat_minutes, nullptr);
                        gga.location.lat.degrees_minutes = static_cast<float>(gga.location.lat.degrees) + (gga.location.lat.minutes / 6.0);

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
                        char buffer_lon_degrees[3] = {data[i], data[i + 1], data[i + 2]};
                        char buffer_lon_minutes[8] =
                            {
                                data[i + 3],
                                data[i + 4],
                                data[i + 5],
                                data[i + 6],
                                data[i + 7],
                                data[i + 8],
                                data[i + 9],
                                data[i + 10],
                            };

                        gga.location.lon.degrees = std::stoi(buffer_lon_degrees);
                        gga.location.lon.minutes = std::strtof(buffer_lon_minutes, nullptr);
                        gga.location.lon.degrees_minutes = static_cast<float>(gga.location.lon.degrees) + (gga.location.lon.minutes / 6.0);

                        gga.location.lon.update_flag = true;

                        i += (11 - 1);
                        break;
                    }
                    case 5: //<E/W>
                        gga.location.lon.direction = data[i];

                        gga.location.lon.direction_update_flag = true;
                        break;
                    case 6: //<Quality>
                        gga.gps_mode_status = std::stoi(static_cast<char[]>(data[i]));
                        break;
                    case 7: //<NumSatUsed>
                    {
                        char buffer_online_satellite_count[2] = {data[i], data[i + 1]};

                        gga.online_satellite_count = std::stoi(buffer_online_satellite_count);

                        i += (2 - 1);
                        break;
                    }
                    case 8: // <HDOP>
                    {
                        size_t buffer_index = 0;

                        if (search_command(",", data.get() + i, length - i, &buffer_index) == false) // 搜索下一个
                        {
                            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search_command fail\n");
                        }

                        char buffer_1[buffer_index] = {0};

                        memcpy(buffer_1, data.get() + i, buffer_index);

                        gga.hdop = std::strtof(buffer_1, nullptr); // 转为float

                        i += (buffer_index - 1);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "buffer_field_count: %d\n", buffer_field_count);
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
