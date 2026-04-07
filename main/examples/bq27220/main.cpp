/*
 * @Description: bq27220
 * @Author: LILYGO_L
 * @Date: 2025-01-04 15:06:05
 * @LastEditTime: 2026-03-26 14:59:03
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Bq27220_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ27220_SDA, BQ27220_SCL, I2C_NUM_0);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Bq27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(Bq27220_Iic_Bus, BQ27220_IIC_ADDRESS);

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

    vTaskDelay(pdMS_TO_TICKS(1000));

    Bq27220_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    Bq27220->begin();

    printf("BQ27220 ID: %#X\n", Bq27220->get_device_id());

    // 设置的电池容量会在没有电池插入的时候自动还原为默认值
    Bq27220->set_design_capacity(400);
    Bq27220->set_temperature_mode(Cpp_Bus_Driver::Bq27220xxxx::Temperature_Mode::EXTERNAL_NTC);
    Bq27220->set_sleep_current_threshold(5);

    while (1)
    {
        // printf("BQ27220 ID: %#X\n", Bq27220->get_device_id());

        // Iic_Scan();
        printf("////////////////////////////////////////////////////\n");
        printf("--------------------------------------------------------------------------\n");
        printf("BQ27220 ID: %#X\n", Bq27220->get_device_id());
        printf("--------------------------------------------------------------------------\n");
        printf("design capacity: %dmah\n", Bq27220->get_design_capacity());
        printf("remaining capacity: %dmah\n", Bq27220->get_remaining_capacity());
        // 放电后才更新full_charge_capacity
        printf("full charge capacity: %dmah\n", Bq27220->get_full_charge_capacity());
        printf("raw coulomb count: %dc\n", Bq27220->get_raw_coulomb_count());
        printf("cycle count: %d\n", Bq27220->get_cycle_count());
        printf("battery level: %d%%\n", Bq27220->get_status_of_health());
        printf("battery health: %d%%\n", Bq27220->get_status_of_health());
        printf("--------------------------------------------------------------------------\n");
        printf("voltage: %dmv\n", Bq27220->get_voltage());
        int16_t current = Bq27220->get_current();
        printf("charging voltage: %dmv\n", Bq27220->get_charging_voltage());
        printf("current: %dma\n", current);
        printf("charging current: %dma\n", Bq27220->get_charging_current());
        printf("standby current: %dma\n", Bq27220->get_standby_current());
        printf("max load current current: %dma\n", Bq27220->get_max_load_current());
        printf("average power: %dmw\n", Bq27220->get_average_power());
        printf("--------------------------------------------------------------------------\n");
        printf("chip temperature: %.03f^C\n", Bq27220->get_chip_temperature_celsius());
        printf("ntc temperature: %.03f^C\n", Bq27220->get_temperature_celsius());
        printf("--------------------------------------------------------------------------\n");
        Bq27220->set_at_rate(current);
        printf("at rate: %dma\n", Bq27220->get_at_rate());
        printf("at rate battery time to empty: %dmin\n", Bq27220->get_at_rate_time_to_empty());
        printf("battery time to empty: %dmin\n", Bq27220->get_time_to_empty());
        printf("battery time to full charge: %dmin\n", Bq27220->get_time_to_full());
        printf("battery standby time to empty: %dmin\n", Bq27220->get_standby_time_to_empty());
        printf("battery max load time to empty: %dmin\n", Bq27220->get_max_load_time_to_empty());
        printf("--------------------------------------------------------------------------\n");

        // Cpp_Bus_Driver::Bq27220xxxx::Operation_Status os;
        // Bq27220->get_operation_status(os);

        Cpp_Bus_Driver::Bq27220xxxx::Battery_Status bs;
        if (Bq27220->get_battery_status(bs) == true)
        {
            printf("fully discharged flag: %d\n", bs.flag.fd);
            printf("sleep flag: %d\n", bs.flag.sleep);
            printf("charging overheat flag: %d\n", bs.flag.otc);
            printf("discharging overheat flag: %d\n", bs.flag.otd);
            printf("fully discharged flag: %d\n", bs.flag.fc);
            printf("charging prohibited flag: %d\n", bs.flag.chginh);
            printf("terminate charging alarm flag: %d\n", bs.flag.tca);
            printf("terminate discharging alarm flag: %d\n", bs.flag.tda);
            printf("battery insertion detection flag: %d\n", bs.flag.auth_gd);
            printf("battery present flag: %d\n", bs.flag.battpres);
            printf("discharge flag: %d\n", bs.flag.dsg);
        }
        printf("--------------------------------------------------------------------------\n");
        printf("////////////////////////////////////////////////////\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
