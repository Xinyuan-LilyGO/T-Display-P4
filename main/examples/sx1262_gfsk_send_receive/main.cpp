/*
 * @Description: sx1262_gfsk_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:54:47
 * @LastEditTime: 2026-03-06 17:35:27
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

uint8_t Receive_Package[255] = {0};

uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

size_t Cycle_Time = 0;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto Sx1262_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

// bool SX1262_Busy_Wait_Callback(void)
// {
//     return Xl9535->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO0);
//     // return 1;
// }

auto Sx1262 = std::make_unique<Cpp_Bus_Driver::Sx126x>(Sx1262_Spi_Bus, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262, SX1262_BUSY, SX1262_CS);

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    Xl9535->pin_mode(XL9535_SX1262_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    // LORA复位
    Xl9535->pin_mode(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 默认使用RF1天线
    Xl9535->pin_mode(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    Sx1262->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);

    Sx1262->begin(10000000);
    Sx1262->config_gfsk_params(850.0, 200.0, Cpp_Bus_Driver::Sx126x::Gfsk_Bw::BW_467000HZ, 140, 22);
    Sx1262->clear_buffer();

    Sx1262->start_gfsk_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE,
                             Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
                             Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE);
    Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);

    printf("SX1262 start gfsk transmit\n");

    while (1)
    {
        if (esp_log_timestamp() > Cycle_Time)
        {
            printf("SX1262 ID: %s\n", Sx1262->get_device_id().c_str());

            printf("SX1262 get current limit: %d\n", Sx1262->get_current_limit());

            switch (Sx1262->get_packet_type())
            {
            case Cpp_Bus_Driver::Sx126x::Packet_Type::GFSK:
                printf("SX1262 packet type: GFSK\n");
                break;
            case Cpp_Bus_Driver::Sx126x::Packet_Type::LORA:
                printf("SX1262 packet type: LORA\n");
                break;
            case Cpp_Bus_Driver::Sx126x::Packet_Type::LR_FHSS:
                printf("SX1262 packet type: LR_FHSS\n");
                break;

            default:
                break;
            }

            switch (Sx1262->parse_chip_mode_status(Sx1262->get_status()))
            {
            case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::STBY_RC:
                printf("SX1262 chip mode status: STBY_RC\n");
                break;
            case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::STBY_XOSC:
                printf("SX1262 chip mode status: STBY_XOSC\n");
                break;
            case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::FS:
                printf("SX1262 chip mode status: FS\n");
                break;
            case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::RX:
                printf("SX1262 chip mode status: RX\n");
                break;
            case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::TX:
                printf("SX1262 chip mode status: TX\n");
                break;

            default:
                break;
            }

            Cycle_Time = esp_log_timestamp() + 1000;
        }

        if (Sx1262->pin_read(ESP32P4_BOOT) == 0)
        {
            // 设置发送模式，发送完成后进入快速切换模式（FS模式）
            Sx1262->start_gfsk_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::TX, 0, Cpp_Bus_Driver::Sx126x::Fallback_Mode::FS);
            Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE);
            Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE);

            printf("SX1262 send start\n");
            uint16_t timeout_count = 0;
            if (Sx1262->send_data(Send_Package, sizeof(Send_Package)) == true)
            {
                while (1) // 等待发送完成
                {
                    if (Xl9535->pin_read(XL9535_SX1262_DIO1) == 1) // 发送完成中断
                    {
                        // 检查中断
                        Cpp_Bus_Driver::Sx126x::Irq_Status is;
                        if (Sx1262->parse_irq_status(Sx1262->get_irq_flag(), is) == false)
                        {
                            printf("parse_irq_status fail\n");
                        }
                        else
                        {
                            if (is.all_flag.tx_done == true) // 发送完成
                            {
                                printf("SX1262 send success\n");
                                break;
                            }
                        }
                    }

                    timeout_count++;
                    if (timeout_count > 500) // 超时
                    {
                        printf("SX1262 send timeout\n");
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
            else
            {
                printf("SX1262 send fail\n");
            }

            // vTaskDelay(pdMS_TO_TICKS(1000));

            // 还原接收模式
            Sx1262->start_gfsk_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
            Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE);
            Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        }

        if (Xl9535->pin_read(XL9535_SX1262_DIO1) == 1) // 接收完成中断
        {
            // 检查中断
            Cpp_Bus_Driver::Sx126x::Irq_Status is;
            // 判断中断正确性
            if (Sx1262->parse_irq_status(Sx1262->get_irq_flag(), is) == false)
            {
                printf("parse_irq_status fail\n");
            }
            else
            {
                if (is.all_flag.tx_rx_timeout == true)
                {
                    printf("receive timeout\n");
                    Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TIMEOUT);
                }
                else if (is.all_flag.crc_error == true)
                {
                    printf("receive crc error\n");
                    Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::CRC_ERROR);
                }
                else
                {
                    // 判断传输包正确性
                    Cpp_Bus_Driver::Sx126x::Gfsk_Packet_Status gps;
                    uint32_t packet_buffer = Sx1262->get_gfsk_packet_status();
                    if (Sx1262->parse_gfsk_packet_status(packet_buffer, gps) == false)
                    {
                        printf("parse_gfsk_packet_status fail\n");
                    }
                    else
                    {
                        if (gps.abort_error_flag == true) // 中止错误
                        {
                            printf("receive abort_error_flag error\n");
                        }
                        else if (gps.length_error_flag == true) // 长度错误
                        {
                            printf("receive length_error_flag error\n");
                        }
                        else if (gps.crc_error_flag == true) // CRC校验错误
                        {
                            printf("receive crc_error_flag error\n");
                        }
                        else if (gps.address_error_flag == true) // 地址错误
                        {
                            printf("receive address_error_flag error\n");
                        }
                        else if (gps.sync_word_flag == true) // 同步错误
                        {
                            printf("receive sync_word_flag error\n");
                        }
                        else if (gps.preamble_error_flag == true) // 前导错误
                        {
                            printf("receive preamble_error_flag error\n");
                        }
                        else
                        {
                            memset(Receive_Package, 0, 255);
                            uint8_t length_buffer = Sx1262->receive_data(Receive_Package);
                            if (length_buffer == 0)
                            {
                                printf("SX1262 receive fail (error assert: %d)\n", Sx1262->_assert);
                            }
                            else
                            {
                                Cpp_Bus_Driver::Sx126x::Packet_Metrics pm;
                                Sx1262->parse_gfsk_packet_metrics(packet_buffer, pm);

                                printf("SX1262 receive rssi_average: %.01f rssi_sync: %.01f\n", pm.gfsk.rssi_average, pm.gfsk.rssi_sync);

                                for (uint8_t i = 0; i < length_buffer; i++)
                                {
                                    printf("get SX1262 data[%d]: %d\n", i, Receive_Package[i]);
                                }
                            }
                        }
                    }
                }
            }

            Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
