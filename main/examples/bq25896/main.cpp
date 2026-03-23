/*
 * @Description: bq25896
 * @Author: LILYGO_L
 * @Date: 2026-03-06 11:26:23
 * @LastEditTime: 2026-03-23 17:35:02
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "kode_bq25896.h"

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);

auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    esp_err_t assert = Kode_Bq25896::bq25896_init(Bq25896_Iic_Bus, Bq25896_Handle);
    if (assert != ESP_OK)
    {
        while (1)
        {
            printf("bq25896_init fail (error code: %#X)\n", assert);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    printf("bq25896 init successful\n");

    // 禁用看门狗后不能读取看门狗寄存器状态，否者看门狗禁用会失效
    Kode_Bq25896::bq25896_set_watchdog_timer(Bq25896_Handle, Kode_Bq25896::bq25896_watchdog_t::BQ25896_WATCHDOG_DISABLE);

    Kode_Bq25896::bq25896_set_adc_conversion(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
    Kode_Bq25896::bq25896_set_adc_conversion_rate(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);

    // Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);

    while (1)
    {
        printf("\n////////////////////////////////////////////////////\n");
        printf("BQ25896 Status Report\n");

        // 设备信息
        printf("device information: \n");

        uint8_t part_number = 0;
        Kode_Bq25896::bq25896_dev_rev_t dev_rev;
        uint8_t ts_profile = 0;
        Kode_Bq25896::bq25896_get_part_number(Bq25896_Handle, &part_number);
        Kode_Bq25896::bq25896_get_device_revision(Bq25896_Handle, &dev_rev);
        Kode_Bq25896::bq25896_get_ts_profile(Bq25896_Handle, &ts_profile);

        printf("Part Number: %#X (0=BQ25896)\n", part_number);
        printf("Device Revision: %d\n", dev_rev);
        printf("Temperature Profile: %s\n", ts_profile == 1 ? "JEITA (default)" : "Unknown");
        printf("\n");

        // 充电状态
        printf("charging status: \n");

        Kode_Bq25896::bq25896_vbus_stat_t vbus_stat;
        Kode_Bq25896::bq25896_chrg_stat_t chrg_stat;
        Kode_Bq25896::bq25896_pg_stat_t pg_stat;
        Kode_Bq25896::bq25896_vsys_stat_t vsys_stat;

        Kode_Bq25896::bq25896_get_vbus_status(Bq25896_Handle, &vbus_stat);
        Kode_Bq25896::bq25896_get_charging_status(Bq25896_Handle, &chrg_stat);
        Kode_Bq25896::bq25896_get_pg_status(Bq25896_Handle, &pg_stat);
        Kode_Bq25896::bq25896_get_vsys_status(Bq25896_Handle, &vsys_stat);

        const char *vbus_stat_str;
        switch (vbus_stat)
        {
        case Kode_Bq25896::BQ25896_VBUS_STAT_NO_INPUT:
            vbus_stat_str = "No Input";
            break;
        case Kode_Bq25896::BQ25896_VBUS_STAT_USB_HOST:
            vbus_stat_str = "USB Host SDP";
            break;
        case Kode_Bq25896::BQ25896_VBUS_STAT_ADAPTER:
            vbus_stat_str = "Adapter (3.25A)";
            break;
        case Kode_Bq25896::BQ25896_VBUS_STAT_OTG:
            vbus_stat_str = "OTG";
            break;
        default:
            vbus_stat_str = "Unknown";
            break;
        }

        const char *chrg_stat_str;
        switch (chrg_stat)
        {
        case Kode_Bq25896::BQ25896_CHRG_STAT_NOT_CHARGING:
            chrg_stat_str = "Not Charging";
            break;
        case Kode_Bq25896::BQ25896_CHRG_STAT_PRE_CHARGE:
            chrg_stat_str = "Pre-charge";
            break;
        case Kode_Bq25896::BQ25896_CHRG_STAT_FAST_CHARGING:
            chrg_stat_str = "Fast Charging";
            break;
        case Kode_Bq25896::BQ25896_CHRG_STAT_TERM_DONE:
            chrg_stat_str = "Charge Termination Done";
            break;
        default:
            chrg_stat_str = "Unknown";
            break;
        }

        printf("VBUS Status: %s\n", vbus_stat_str);
        printf("Charging Status: %s\n", chrg_stat_str);
        printf("Power Good: %s\n", pg_stat == Kode_Bq25896::BQ25896_PG_STAT_GOOD ? "Good" : "Not Good");
        printf("VSYS Status: %s\n", vsys_stat == Kode_Bq25896::BQ25896_VSYS_STAT_IN_REG ? "In VSYSMIN regulation" : "Not in regulation");
        printf("\n");

        // 故障状态
        printf("fault status: \n");

        // Kode_Bq25896::bq25896_watchdog_fault_t wd_fault;
        Kode_Bq25896::bq25896_boost_fault_t boost_fault;
        Kode_Bq25896::bq25896_chrg_fault_t chrg_fault;
        Kode_Bq25896::bq25896_bat_fault_t bat_fault;
        Kode_Bq25896::bq25896_ntc_fault_t ntc_fault;
        Kode_Bq25896::bq25896_therm_stat_t therm_stat;

        // Kode_Bq25896::bq25896_get_watchdog_fault(Bq25896_Handle, &wd_fault);
        Kode_Bq25896::bq25896_get_boost_fault(Bq25896_Handle, &boost_fault);
        Kode_Bq25896::bq25896_get_charge_fault(Bq25896_Handle, &chrg_fault);
        Kode_Bq25896::bq25896_get_battery_fault(Bq25896_Handle, &bat_fault);
        Kode_Bq25896::bq25896_get_ntc_fault(Bq25896_Handle, &ntc_fault);
        Kode_Bq25896::bq25896_get_thermal_regulation_status(Bq25896_Handle, &therm_stat);

        const char *chrg_fault_str;
        switch (chrg_fault)
        {
        case Kode_Bq25896::BQ25896_CHRG_FAULT_NORMAL:
            chrg_fault_str = "Normal";
            break;
        case Kode_Bq25896::BQ25896_CHRG_FAULT_INPUT_FAULT:
            chrg_fault_str = "Input fault";
            break;
        case Kode_Bq25896::BQ25896_CHRG_FAULT_THERMAL:
            chrg_fault_str = "Thermal shutdown";
            break;
        case Kode_Bq25896::BQ25896_CHRG_FAULT_TIMER_EXPIRED:
            chrg_fault_str = "Safety Timer Expired";
            break;
        default:
            chrg_fault_str = "Unknown";
            break;
        }

        const char *ntc_fault_str;
        switch (ntc_fault)
        {
        case Kode_Bq25896::BQ25896_NTC_FAULT_NORMAL:
            ntc_fault_str = "Normal";
            break;
        case Kode_Bq25896::BQ25896_NTC_FAULT_TS_WARM:
            ntc_fault_str = "TS Warm";
            break;
        case Kode_Bq25896::BQ25896_NTC_FAULT_TS_COOL:
            ntc_fault_str = "TS Cool";
            break;
        case Kode_Bq25896::BQ25896_NTC_FAULT_TS_COLD:
            ntc_fault_str = "TS Cold";
            break;
        case Kode_Bq25896::BQ25896_NTC_FAULT_TS_HOT:
            ntc_fault_str = "TS Hot";
            break;
        default:
            ntc_fault_str = "Unknown";
            break;
        }

        // printf("Watchdog Fault: %s\n", wd_fault == Kode_Bq25896::BQ25896_WD_FAULT_NORMAL ? "Normal" : "Timer expired");
        printf("Boost Fault: %s\n", boost_fault == Kode_Bq25896::BQ25896_BOOST_FAULT_NORMAL ? "Normal" : "VBUS overloaded/OVP");
        printf("Charge Fault: %s\n", chrg_fault_str);
        printf("Battery Fault: %s\n", bat_fault == Kode_Bq25896::BQ25896_BAT_FAULT_NORMAL ? "Normal" : "Battery Overvoltage");
        printf("NTC Fault: %s\n", ntc_fault_str);
        printf("Thermal Status: %s\n", therm_stat == Kode_Bq25896::BQ25896_THERM_STAT_NORMAL ? "Normal" : "In Thermal Regulation");
        printf("\n");

        // 电压测量
        printf("voltage measurements: \n");

        uint16_t bat_voltage = 0;
        uint16_t sys_voltage = 0;
        uint16_t vbus_voltage = 0;

        Kode_Bq25896::bq25896_get_battery_voltage(Bq25896_Handle, &bat_voltage);
        Kode_Bq25896::bq25896_get_system_voltage(Bq25896_Handle, &sys_voltage);
        Kode_Bq25896::bq25896_get_vbus_voltage(Bq25896_Handle, &vbus_voltage);

        printf("Battery Voltage: %dmV\n", bat_voltage);
        printf("System Voltage: %dmV\n", sys_voltage);
        printf("VBUS Voltage: %dmV\n", vbus_voltage);
        printf("\n");

        // 电流测量
        printf("current measurements: \n");

        uint16_t charge_current = 0;
        uint16_t ico_current_limit = 0;

        Kode_Bq25896::bq25896_get_charge_current(Bq25896_Handle, &charge_current);
        Kode_Bq25896::bq25896_get_ico_current_limit(Bq25896_Handle, &ico_current_limit);

        printf("Charge Current: %dmA\n", charge_current);
        printf("ICO Current Limit: %dmA\n", ico_current_limit);
        printf("\n");

        // DPM状态
        printf("dpm status: \n");

        Kode_Bq25896::bq25896_vdpm_stat_t vdpm_stat;
        Kode_Bq25896::bq25896_idpm_stat_t idpm_stat;

        Kode_Bq25896::bq25896_get_vdpm_status(Bq25896_Handle, &vdpm_stat);
        Kode_Bq25896::bq25896_get_idpm_status(Bq25896_Handle, &idpm_stat);

        printf("VINDPM Status: %s\n", vdpm_stat == Kode_Bq25896::BQ25896_VDPM_ACTIVE ? "Active" : "Not Active");
        printf("IINDPM Status: %s\n", idpm_stat == Kode_Bq25896::BQ25896_IDPM_ACTIVE ? "Active" : "Not Active");
        printf("\n");

        // ICO状态
        printf("ico status: \n");

        Kode_Bq25896::bq25896_ico_status_t ico_status;
        Kode_Bq25896::bq25896_get_ico_status(Bq25896_Handle, &ico_status);

        printf("ICO Status: %s\n",
               ico_status == Kode_Bq25896::BQ25896_ICO_COMPLETE ? "Complete (Maximum Current Detected)" : "In Progress");
        printf("\n");

        // 温度传感器
        printf("temperature sensor: \n");

        float ts_percentage = 0.0;
        Kode_Bq25896::bq25896_get_ts_voltage_percentage(Bq25896_Handle, &ts_percentage);

        printf("TS Voltage: %.3f%% of REGN\n", ts_percentage);
        printf("\n");

        // VBUS状态详细信息
        printf("vbus details: \n");

        Kode_Bq25896::bq25896_vbus_gd_t vbus_gd;
        Kode_Bq25896::bq25896_get_vbus_good_status(Bq25896_Handle, &vbus_gd);

        printf("VBUS Good: %s\n",
               vbus_gd == Kode_Bq25896::BQ25896_VBUS_ATTACHED ? "Attached" : "Not Attached");
        printf("\n");

        // 计算功率
        printf("power calculations: \n");

        if (charge_current > 0 && bat_voltage > 0)
        {
            uint32_t power_mw = ((uint32_t)bat_voltage * (uint32_t)charge_current) / 1000;
            printf("Charging Power: %ldmW\n", power_mw);
        }
        else
        {
            printf("Charging Power: 0mW (Not charging)\n");
        }
        printf("\n");

        // 状态总结
        printf("status summary: \n");

        bool is_charging = (chrg_stat == Kode_Bq25896::BQ25896_CHRG_STAT_PRE_CHARGE ||
                            chrg_stat == Kode_Bq25896::BQ25896_CHRG_STAT_FAST_CHARGING);
        bool is_charge_done = (chrg_stat == Kode_Bq25896::BQ25896_CHRG_STAT_TERM_DONE);
        bool vbus_present = (vbus_stat != Kode_Bq25896::BQ25896_VBUS_STAT_NO_INPUT);
        bool power_good = (pg_stat == Kode_Bq25896::BQ25896_PG_STAT_GOOD);

        printf("Charging: %s\n", is_charging ? "YES" : "NO");
        printf("Charge Complete: %s\n", is_charge_done ? "YES" : "NO");
        printf("VBUS Present: %s\n", vbus_present ? "YES" : "NO");
        printf("Power Good: %s\n", power_good ? "YES" : "NO");
        printf("Battery Low: %s\n", bat_voltage < 3000 ? "YES" : "NO");
        printf("Thermal Issue: %s\n", therm_stat == Kode_Bq25896::BQ25896_THERM_STAT_NORMAL ? "NO" : "YES");
        printf("\n");

        printf("////////////////////////////////////////////////////\n\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}