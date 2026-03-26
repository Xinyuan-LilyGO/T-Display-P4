/*
 * @Description: sgm38121
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:52:48
 * @LastEditTime: 2026-03-26 15:01:36
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
#include "kode_bq25896.h"
#endif

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Sgm38121_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Sgm38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(Sgm38121_Iic_Bus, SGM38121_IIC_ADDRESS);

void Iic_Scan(void)
{
    std::vector<uint8_t> address;
    if (Sgm38121_Iic_Bus->scan_7bit_address(&address) == true)
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

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
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

    Sgm38121->begin();

    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);

    // Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1000);
    // Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_2, 1000);

    while (1)
    {
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
        vTaskDelay(pdMS_TO_TICKS(1000));
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
        // vTaskDelay(pdMS_TO_TICKS(1000));
        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        // vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
