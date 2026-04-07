/*
 * @Description: aw86224
 * @Author: LILYGO_L
 * @Date: 2024-12-25 10:33:25
 * @LastEditTime: 2026-03-26 14:58:48
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Aw86224_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Aw86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(Aw86224_Iic_Bus, AW86224_IIC_ADDRESS);

void Iic_Scan(void)
{
    std::vector<uint8_t> address;
    if (Aw86224_Iic_Bus->scan_7bit_address(&address) == true)
    {
        for (size_t i = 0; i < address.size(); i++)
        {
            printf("discovered iic devices[%u]: %#X\n", i, address[i]);
        }
    }
}

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

    Aw86224->begin(500000);
    // printf("AW86224 input voltage: %.06f V\n", Aw86224->get_input_voltage());

    Iic_Scan();

    // 等待F0校准
    while (1)
    {
        uint32_t f0_value = Aw86224->get_f0_detection();
        printf("AW86224 get f0 detection value: %ld\n", f0_value);

        if (Aw86224->set_f0_calibrate(f0_value) == true)
        {
            break;
        }
    }

    // Aw86224->set_waveform_data_sample_rate(Cpp_Bus_Driver::Aw862xx::Sample_Rate::RATE_12KHZ);

    // RAM播放
    Aw86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    while (1)
    {
        // Iic_Scan();
        // printf("AW86224 input voltage: %.06f V\n", Aw86224->get_input_voltage());

        // // RTP播放
        //  Cpp_Bus_Driver::Aw862xx::System_Status ss;
        //  if (Aw86224->get_system_status(ss) == true)
        //  {
        //      if (ss.rtp_fifo_full == false)
        //      {
        //          Aw86224->run_rtp_playback_waveform(Cpp_Bus_Driver::haptic_waveform_ordinary, sizeof(Cpp_Bus_Driver::haptic_waveform_ordinary));
        //          printf("AW86224 rtp_playback_waveform \n");

        //         vTaskDelay(pdMS_TO_TICKS(100));
        //     }
        // }
        // vTaskDelay(pdMS_TO_TICKS(10));

        // RAM播放
        Aw86224->run_ram_playback_waveform(1, 15, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));
        Aw86224->stop_ram_playback_waveform();
        vTaskDelay(pdMS_TO_TICKS(1000));

        Aw86224->run_ram_playback_waveform(2, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(30));
        Aw86224->run_ram_playback_waveform(2, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(30));

        vTaskDelay(pdMS_TO_TICKS(1000));

        Aw86224->run_ram_playback_waveform(1, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
