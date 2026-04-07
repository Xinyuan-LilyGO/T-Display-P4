/*
 * @Description: sx1262_lora_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:57:12
 * @LastEditTime: 2026-03-26 15:01:45
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

uint8_t Receive_Package[255] = {0};

uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

size_t Cycle_Time = 0;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto Sx1262_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

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

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
    int16_t assert = Kode_Bq25896::bq25896_init(Bq25896_Iic_Bus, Bq25896_Handle);
    if (assert != ESP_OK)
    {
        printf("bq25896 init fail (error code: %#X)\n", assert);
    }
    else
    {
        printf("bq25896 init success\n");

        Kode_Bq25896::bq25896_set_input_current_limit(Bq25896_Handle, Kode_Bq25896::bq25896_ilim_t ::BQ25896_ILIM_2000MA);
        // 禁用看门狗后不能读取看门狗寄存器状态，否者看门狗禁用会失效
        Kode_Bq25896::bq25896_set_watchdog_timer(Bq25896_Handle, Kode_Bq25896::bq25896_watchdog_t::BQ25896_WATCHDOG_DISABLE);
        // Kode_Bq25896::bq25896_set_adc_conversion(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
        // Kode_Bq25896::bq25896_set_adc_conversion_rate(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);
        Kode_Bq25896::bq25896_set_charge_current(Bq25896_Handle, Kode_Bq25896::bq25896_ichg_t::BQ25896_ICHG_512MA);
        // Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);
    }

    Xl9535_Iic_Bus->set_bus_handle(Bq25896_Iic_Bus->get_bus_handle());
#endif

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
    Sx1262->config_lora_params(920.0, Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_125000HZ, 140, 22);
    Sx1262->clear_buffer();

    Sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE,
                             Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
                             Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE);
    Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);

    printf("SX1262 start lora transmit\n");

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
            Sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::TX, 0, Cpp_Bus_Driver::Sx126x::Fallback_Mode::FS);
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
            Sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
            Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
                                     Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE);
            Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        }

        if (Xl9535->pin_read(XL9535_SX1262_DIO1) == 1) // 接收完成中断
        {
            // 检查中断
            Cpp_Bus_Driver::Sx126x::Irq_Status is;
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
                else if (is.lora_reg_flag.header_error == true)
                {
                    printf("receive header error\n");
                    Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::HEADER_ERROR);
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
                        if (Sx1262->get_lora_packet_metrics(pm) == true)
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

            Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
