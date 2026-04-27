/*
 * @Description: bq25896
 * @Author: LILYGO_L
 * @Date: 2026-03-06 11:26:23
 * @LastEditTime: 2026-04-25 16:40:45
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto bq25896_handle = driver.chip().bq25896_handle;

  kode_bq25896::bq25896_set_adc_conversion(
      bq25896_handle,
      kode_bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
  kode_bq25896::bq25896_set_adc_conversion_rate(
      bq25896_handle,
      kode_bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);

  kode_bq25896::bq25896_set_otg(
      bq25896_handle, kode_bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);

  while (1) {
    printf("\n////////////////////////////////////////////////////\n");
    printf("Bq25896 status report\n");

    // 设备信息
    printf("Device information: \n");

    uint8_t part_number = 0;
    kode_bq25896::bq25896_dev_rev_t dev_rev;
    uint8_t ts_profile = 0;
    kode_bq25896::bq25896_get_part_number(bq25896_handle, &part_number);
    kode_bq25896::bq25896_get_device_revision(bq25896_handle, &dev_rev);
    kode_bq25896::bq25896_get_ts_profile(bq25896_handle, &ts_profile);

    printf("Part number: %#X (0=bq25896)\n", part_number);
    printf("Device revision: %d\n", dev_rev);
    printf("Temperature profile: %s\n",
           ts_profile == 1 ? "Jeita (default)" : "Unknown");
    printf("\n");

    // 充电状态
    printf("Charging status: \n");

    kode_bq25896::bq25896_vbus_stat_t vbus_stat;
    kode_bq25896::bq25896_chrg_stat_t chrg_stat;
    kode_bq25896::bq25896_pg_stat_t pg_stat;
    kode_bq25896::bq25896_vsys_stat_t vsys_stat;

    kode_bq25896::bq25896_get_vbus_status(bq25896_handle, &vbus_stat);
    kode_bq25896::bq25896_get_charging_status(bq25896_handle, &chrg_stat);
    kode_bq25896::bq25896_get_pg_status(bq25896_handle, &pg_stat);
    kode_bq25896::bq25896_get_vsys_status(bq25896_handle, &vsys_stat);

    const char* vbus_stat_str;
    switch (vbus_stat) {
      case kode_bq25896::BQ25896_VBUS_STAT_NO_INPUT:
        vbus_stat_str = "No input";
        break;
      case kode_bq25896::BQ25896_VBUS_STAT_USB_HOST:
        vbus_stat_str = "Usb host sdp";
        break;
      case kode_bq25896::BQ25896_VBUS_STAT_ADAPTER:
        vbus_stat_str = "Adapter (3.25a)";
        break;
      case kode_bq25896::BQ25896_VBUS_STAT_OTG:
        vbus_stat_str = "Otg";
        break;
      default:
        vbus_stat_str = "Unknown";
        break;
    }

    const char* chrg_stat_str;
    switch (chrg_stat) {
      case kode_bq25896::BQ25896_CHRG_STAT_NOT_CHARGING:
        chrg_stat_str = "Not charging";
        break;
      case kode_bq25896::BQ25896_CHRG_STAT_PRE_CHARGE:
        chrg_stat_str = "Pre-charge";
        break;
      case kode_bq25896::BQ25896_CHRG_STAT_FAST_CHARGING:
        chrg_stat_str = "Fast charging";
        break;
      case kode_bq25896::BQ25896_CHRG_STAT_TERM_DONE:
        chrg_stat_str = "Charge termination done";
        break;
      default:
        chrg_stat_str = "Unknown";
        break;
    }

    printf("Vbus status: %s\n", vbus_stat_str);
    printf("Charging status: %s\n", chrg_stat_str);
    printf("Power good: %s\n",
           pg_stat == kode_bq25896::BQ25896_PG_STAT_GOOD ? "Good" : "Not good");
    printf("Vsys status: %s\n",
           vsys_stat == kode_bq25896::BQ25896_VSYS_STAT_IN_REG
               ? "In vsysmin regulation"
               : "Not in regulation");
    printf("\n");

    // 故障状态
    printf("Fault status: \n");

    // kode_bq25896::bq25896_watchdog_fault_t wd_fault;
    kode_bq25896::bq25896_boost_fault_t boost_fault;
    kode_bq25896::bq25896_chrg_fault_t chrg_fault;
    kode_bq25896::bq25896_bat_fault_t bat_fault;
    kode_bq25896::bq25896_ntc_fault_t ntc_fault;
    kode_bq25896::bq25896_therm_stat_t therm_stat;

    // kode_bq25896::bq25896_get_watchdog_fault(bq25896_handle, &wd_fault);
    kode_bq25896::bq25896_get_boost_fault(bq25896_handle, &boost_fault);
    kode_bq25896::bq25896_get_charge_fault(bq25896_handle, &chrg_fault);
    kode_bq25896::bq25896_get_battery_fault(bq25896_handle, &bat_fault);
    kode_bq25896::bq25896_get_ntc_fault(bq25896_handle, &ntc_fault);
    kode_bq25896::bq25896_get_thermal_regulation_status(bq25896_handle,
                                                        &therm_stat);

    const char* chrg_fault_str;
    switch (chrg_fault) {
      case kode_bq25896::BQ25896_CHRG_FAULT_NORMAL:
        chrg_fault_str = "Normal";
        break;
      case kode_bq25896::BQ25896_CHRG_FAULT_INPUT_FAULT:
        chrg_fault_str = "Input fault";
        break;
      case kode_bq25896::BQ25896_CHRG_FAULT_THERMAL:
        chrg_fault_str = "Thermal shutdown";
        break;
      case kode_bq25896::BQ25896_CHRG_FAULT_TIMER_EXPIRED:
        chrg_fault_str = "Safety timer expired";
        break;
      default:
        chrg_fault_str = "Unknown";
        break;
    }

    const char* ntc_fault_str;
    switch (ntc_fault) {
      case kode_bq25896::BQ25896_NTC_FAULT_NORMAL:
        ntc_fault_str = "Normal";
        break;
      case kode_bq25896::BQ25896_NTC_FAULT_TS_WARM:
        ntc_fault_str = "Ts warm";
        break;
      case kode_bq25896::BQ25896_NTC_FAULT_TS_COOL:
        ntc_fault_str = "Ts cool";
        break;
      case kode_bq25896::BQ25896_NTC_FAULT_TS_COLD:
        ntc_fault_str = "Ts cold";
        break;
      case kode_bq25896::BQ25896_NTC_FAULT_TS_HOT:
        ntc_fault_str = "Ts hot";
        break;
      default:
        ntc_fault_str = "Unknown";
        break;
    }

    // printf("Watchdog fault: %s\n", wd_fault ==
    // kode_bq25896::BQ25896_WD_FAULT_NoRMAL ? "Normal" : "Timer expired");
    printf("Boost fault: %s\n",
           boost_fault == kode_bq25896::BQ25896_BOOST_FAULT_NORMAL
               ? "Normal"
               : "Vbus overloaded/ovp");
    printf("Charge fault: %s\n", chrg_fault_str);
    printf("Battery fault: %s\n",
           bat_fault == kode_bq25896::BQ25896_BAT_FAULT_NORMAL
               ? "Normal"
               : "Battery overvoltage");
    printf("Ntc fault: %s\n", ntc_fault_str);
    printf("Thermal status: %s\n",
           therm_stat == kode_bq25896::BQ25896_THERM_STAT_NORMAL
               ? "Normal"
               : "In thermal regulation");
    printf("\n");

    // 电压测量
    printf("Voltage measurements: \n");

    uint16_t bat_voltage = 0;
    uint16_t sys_voltage = 0;
    uint16_t vbus_voltage = 0;

    kode_bq25896::bq25896_get_battery_voltage(bq25896_handle, &bat_voltage);
    kode_bq25896::bq25896_get_system_voltage(bq25896_handle, &sys_voltage);
    kode_bq25896::bq25896_get_vbus_voltage(bq25896_handle, &vbus_voltage);

    printf("Battery voltage: %dmv\n", bat_voltage);
    printf("System voltage: %dmv\n", sys_voltage);
    printf("Vbus voltage: %dmv\n", vbus_voltage);
    printf("\n");

    // 电流测量
    printf("Current measurements: \n");

    uint16_t charge_current = 0;
    uint16_t ico_current_limit = 0;

    kode_bq25896::bq25896_get_charge_current(bq25896_handle, &charge_current);
    kode_bq25896::bq25896_get_ico_current_limit(bq25896_handle,
                                                &ico_current_limit);

    printf("Charge current: %dma\n", charge_current);
    printf("Ico current limit: %dma\n", ico_current_limit);
    printf("\n");

    // DPM状态
    printf("Dpm status: \n");

    kode_bq25896::bq25896_vdpm_stat_t vdpm_stat;
    kode_bq25896::bq25896_idpm_stat_t idpm_stat;

    kode_bq25896::bq25896_get_vdpm_status(bq25896_handle, &vdpm_stat);
    kode_bq25896::bq25896_get_idpm_status(bq25896_handle, &idpm_stat);

    printf("VINDPM Status: %s\n", vdpm_stat == kode_bq25896::BQ25896_VDPM_ACTIVE
                                      ? "Active"
                                      : "Not active");
    printf("IINDPM Status: %s\n", idpm_stat == kode_bq25896::BQ25896_IDPM_ACTIVE
                                      ? "Active"
                                      : "Not active");
    printf("\n");

    // ICO状态
    printf("Ico status: \n");

    kode_bq25896::bq25896_ico_status_t ico_status;
    kode_bq25896::bq25896_get_ico_status(bq25896_handle, &ico_status);

    printf("ICO Status: %s\n", ico_status == kode_bq25896::BQ25896_ICO_COMPLETE
                                   ? "Complete (maximum current detected)"
                                   : "In progress");
    printf("\n");

    // 温度传感器
    printf("Temperature sensor: \n");

    float ts_percentage = 0.0;
    kode_bq25896::bq25896_get_ts_voltage_percentage(bq25896_handle,
                                                    &ts_percentage);

    printf("TS voltage: %.3f%% of regn\n", ts_percentage);
    printf("\n");

    // VBUS状态详细信息
    printf("Vbus details: \n");

    kode_bq25896::bq25896_vbus_gd_t vbus_gd;
    kode_bq25896::bq25896_get_vbus_good_status(bq25896_handle, &vbus_gd);

    printf("Vbus good: %s\n", vbus_gd == kode_bq25896::BQ25896_VBUS_ATTACHED
                                  ? "Attached"
                                  : "Not attached");
    printf("\n");

    // 计算功率
    printf("Power calculations: \n");

    if (charge_current > 0 && bat_voltage > 0) {
      uint32_t power_mw =
          ((uint32_t)bat_voltage * (uint32_t)charge_current) / 1000;
      printf("Charging power: %ldmw\n", power_mw);
    } else {
      printf("Charging power: 0mw (not charging)\n");
    }
    printf("\n");

    // 状态总结
    printf("Status summary: \n");

    bool is_charging =
        (chrg_stat == kode_bq25896::BQ25896_CHRG_STAT_PRE_CHARGE ||
         chrg_stat == kode_bq25896::BQ25896_CHRG_STAT_FAST_CHARGING);
    bool is_charge_done =
        (chrg_stat == kode_bq25896::BQ25896_CHRG_STAT_TERM_DONE);
    bool vbus_present = (vbus_stat != kode_bq25896::BQ25896_VBUS_STAT_NO_INPUT);
    bool power_good = (pg_stat == kode_bq25896::BQ25896_PG_STAT_GOOD);

    printf("Charging: %s\n", is_charging ? "Yes" : "No");
    printf("Charge complete: %s\n", is_charge_done ? "Yes" : "No");
    printf("Vbus present: %s\n", vbus_present ? "Yes" : "No");
    printf("Power good: %s\n", power_good ? "Yes" : "No");
    printf("Battery low: %s\n", bat_voltage < 3000 ? "Yes" : "No");
    printf(
        "Thermal issue: %s\n",
        therm_stat == kode_bq25896::BQ25896_THERM_STAT_NORMAL ? "No" : "Yes");
    printf("\n");

    printf("////////////////////////////////////////////////////\n\n");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}