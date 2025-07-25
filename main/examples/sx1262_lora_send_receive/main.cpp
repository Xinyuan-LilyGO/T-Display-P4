/*
 * @Description: sx1262_lora_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:57:12
 * @LastEditTime: 2025-07-17 14:32:28
 * @License: GPL 3.0
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"

uint8_t Receive_Package[255] = {0};

uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

size_t Cycle_Time = 0;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(IIC_1_SDA, IIC_1_SCL, I2C_NUM_0);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto SPI_Bus_2 = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SPI_1_MOSI, SPI_1_SCLK, SPI_1_MISO, SPI2_HOST, 0);

// bool SX1262_Busy_Wait_Callback(void)
// {
//     return XL9535->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO0);
//     // return 1;
// }

auto SX1262 = std::make_unique<Cpp_Bus_Driver::Sx126x>(SPI_Bus_2, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262, LORA_BUSY,
                                                       LORA_CS, DEFAULT_CPP_BUS_DRIVER_VALUE);

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    
    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    XL9535->pin_mode(XL9535_LORA_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    // LORA复位
    XL9535->pin_mode(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    SX1262->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);

    SX1262->begin(10000000);
    SX1262->config_lora_params(920.0, Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_125000HZ, 140, 22);
    SX1262->clear_buffer();

    SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE,
                             Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE,
                             Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE);
    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);

    printf("SX1262 start lora transmit\n");

    while (1)
    {
        if (esp_log_timestamp() > Cycle_Time)
        {
            printf("SX1262 ID: %#X\n", SX1262->get_device_id());

            printf("SX1262 get current limit: %d\n", SX1262->get_current_limit());

            switch (SX1262->get_packet_type())
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

            switch (SX1262->parse_chip_mode_status(SX1262->get_status()))
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

        if (SX1262->pin_read(ESP32P4_BOOT) == 0)
        {
            // 设置发送模式，发送完成后进入快速切换模式（FS模式）
            SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::TX, 0, Cpp_Bus_Driver::Sx126x::Fallback_Mode::FS);
            SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::TX_DONE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE);
            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::TX_DONE);

            if (SX1262->send_data(Send_Package, sizeof(Send_Package)) == true)
            {
                uint16_t timeout_count = 0;
                printf("SX1262 send start\n");

                while (1) // 等待发送完成
                {
                    if (XL9535->pin_read(XL9535_LORA_DIO1) == 1) // 发送完成中断
                    {
                        // 方法1（速度比方法2快）
                        //  获取芯片模式状态
                        //  先前设置发送成功后进入FS模式，所以这里进入FS模式即判断成功发送
                        if (SX1262->parse_chip_mode_status(SX1262->get_status()) == Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::FS)
                        {
                            printf("SX1262 send success\n");
                            break;
                        }

                        // //方法2（速度比方法1慢）
                        // // 检查中断
                        // Cpp_Bus_Driver::Sx126x::Iqr_Status is;
                        // if (SX1262->parse_iqr_status(SX1262->get_irq_status(), is) == false)
                        // {
                        //     printf("parse_Iqr_status fail\n");
                        // }
                        // else
                        // {
                        //     if (is.all_flag.tx_done == true) // 发送完成
                        //     {
                        //         printf("SX1262 send success\n");
                        //         break;
                        //     }
                        // }
                    }

                    timeout_count++;
                    if (timeout_count > 1000) // 超时
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
            SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
            SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE);
            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);
        }

        if (XL9535->pin_read(XL9535_LORA_DIO1) == 1) // 接收完成中断
        {
            // 检查中断
            Cpp_Bus_Driver::Sx126x::Irq_Status is;
            if (SX1262->parse_irq_status(SX1262->get_irq_flag(), is) == false)
            {
                printf("parse_iqr_status fail\n");
            }
            else
            {
                if (is.all_flag.tx_rx_timeout == true)
                {
                    printf("receive timeout\n");
                    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::TIMEOUT);
                }
                else if (is.all_flag.crc_error == true)
                {
                    printf("receive crc error\n");
                    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::CRC_ERROR);
                }
                else if (is.lora_reg_flag.header_error == true)
                {
                    printf("receive header error\n");
                    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::HEADER_ERROR);
                }
                else
                {
                    memset(Receive_Package, 0, 255);
                    uint8_t length_buffer = SX1262->receive_data(Receive_Package);
                    if (length_buffer == 0)
                    {
                        printf("SX1262 receive fail (error assert: %d)\n", SX1262->_assert);
                    }
                    else
                    {
                        Cpp_Bus_Driver::Sx126x::Packet_Metrics pm;
                        if (SX1262->get_lora_packet_metrics(pm) == true)
                        {
                            printf("SX1262 receive rssi_average: %.01f rssi_instantaneous: %.01f snr: %.01f\n", pm.lora.rssi_average, pm.lora.rssi_instantaneous, pm.lora.snr);
                        }

                        for (uint8_t i = 0; i < length_buffer; i++)
                        {
                            printf("get SX1262 data[%d]: %d\n", i, Receive_Package[i]);
                        }
                    }
                }
            }

            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
