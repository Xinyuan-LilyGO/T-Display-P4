/*
 * @Description: bq27220
 * @Author: LILYGO_L
 * @Date: 2025-01-04 15:06:05
 * @LastEditTime: 2026-05-12 17:45:18
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace {
constexpr uint16_t kBatteryCapacityMah = 1000;

void PrintSeparator() {
  printf(
      "--------------------------------------------------------------------------"
      "\n");
}

const char* SecurityModeToString(
    cpp_bus_driver::Bq27220::SecurityMode mode) {
  switch (mode) {
    case cpp_bus_driver::Bq27220::SecurityMode::kFullAccess:
      return "full-access";
    case cpp_bus_driver::Bq27220::SecurityMode::kUnsealed:
      return "unsealed";
    case cpp_bus_driver::Bq27220::SecurityMode::kSealed:
      return "sealed";
    case cpp_bus_driver::Bq27220::SecurityMode::kUnknown:
    default:
      return "unknown";
  }
}
}  // namespace

extern "C" void app_main(void) {
  printf("BQ27220 example\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& bq27220 = driver.chip().bq27220;
  if (bq27220 == nullptr) {
    printf("BQ27220 handle is null\n");
    return;
  }

  cpp_bus_driver::Bq27220::CedvProfile battery_profile;
  battery_profile.design_capacity = kBatteryCapacityMah;
  battery_profile.full_charge_capacity = kBatteryCapacityMah;

  cpp_bus_driver::Bq27220::GaugingConfig gauging_config;

  bool config_ok =
      bq27220->ApplyBatteryProfileIfNeeded(battery_profile, gauging_config);
  config_ok &= bq27220->SetTemperatureMode(
      cpp_bus_driver::Bq27220::TemperatureMode::kExternalNtc);
  printf("BQ27220 example config: %s, capacity: %u mAh\n",
      config_ok ? "ok" : "failed", kBatteryCapacityMah);

  while (true) {
    cpp_bus_driver::Bq27220::BatteryStatus battery_status;
    cpp_bus_driver::Bq27220::OperationStatus operation_status;
    const bool battery_status_ok = bq27220->GetBatteryStatus(battery_status);
    const bool operation_status_ok =
        bq27220->GetOperationStatus(operation_status);
    const int16_t current_ma = bq27220->GetCurrent();

    printf("\nBQ27220 snapshot\n");
    PrintSeparator();
    printf("Device ID: 0x%04X\n", bq27220->GetDeviceId());
    printf("Firmware version: 0x%04X\n", bq27220->GetFirmwareVersion());
    printf("Hardware version: 0x%04X\n", bq27220->GetHardwareVersion());

    if (operation_status_ok) {
      printf("Security mode: %s\n",
          SecurityModeToString(operation_status.security));
      printf("Config update: %d\n", operation_status.flag.config_update);
      printf("Init complete: %d\n", operation_status.flag.init_comp);
      printf("EDV2 reached: %d\n", operation_status.flag.edv2);
      printf("Smoothing active: %d\n", operation_status.flag.smth);
    }

    PrintSeparator();
    printf("Design capacity: %u mAh\n", bq27220->GetDesignCapacity());
    printf("Remaining capacity: %u mAh\n",
        bq27220->GetRemainingCapacity());
    printf("Full charge capacity: %u mAh\n",
        bq27220->GetFullChargeCapacity());
    printf("State of charge: %u%%\n", bq27220->GetStatusOfCharge());
    printf("State of health: %u%%\n", bq27220->GetStatusOfHealth());
    printf("Cycle count: %u\n", bq27220->GetCycleCount());
    printf("Raw coulomb count: %d c\n", bq27220->GetRawCoulombCount());

    PrintSeparator();
    printf("Voltage: %u mV\n", bq27220->GetVoltage());
    printf("Current: %d mA\n", current_ma);
    printf("Average current: %d mA\n", bq27220->GetAverageCurrent());
    printf("Average power: %d mW\n", bq27220->GetAveragePower());
    printf("Charging voltage request: %u mV\n",
        bq27220->GetChargingVoltage());
    printf("Charging current request: %u mA\n",
        bq27220->GetChargingCurrent());
    printf("Standby current: %d mA\n", bq27220->GetStandbyCurrent());
    printf("Max load current: %d mA\n", bq27220->GetMaxLoadCurrent());

    PrintSeparator();
    printf("Gauge temperature: %.2f C\n", bq27220->GetTemperatureCelsius());
    printf("Internal temperature: %.2f C\n",
        bq27220->GetChipTemperatureCelsius());

    PrintSeparator();
    bq27220->SetAtRate(current_ma);
    printf("AtRate: %d mA\n", bq27220->GetAtRate());
    printf("AtRate time to empty: %u min\n",
        bq27220->GetAtRateTimeToEmpty());
    printf("Time to empty: %u min\n", bq27220->GetTimeToEmpty());
    printf("Time to full: %u min\n", bq27220->GetTimeToFull());
    printf("Standby time to empty: %u min\n",
        bq27220->GetStandbyTimeToEmpty());
    printf("Max load time to empty: %u min\n",
        bq27220->GetMaxLoadTimeToEmpty());

    if (battery_status_ok) {
      PrintSeparator();
      printf("Battery status raw: 0x%04X\n", battery_status.value);
      printf("Discharging: %d\n", battery_status.flag.dsg);
      printf("Battery present: %d\n", battery_status.flag.battpres);
      printf("Full charged: %d\n", battery_status.flag.fc);
      printf("Full discharged: %d\n", battery_status.flag.fd);
      printf("Charge inhibit: %d\n", battery_status.flag.chginh);
      printf("Charge overtemperature: %d\n", battery_status.flag.otc);
      printf("Discharge overtemperature: %d\n", battery_status.flag.otd);
      printf("Sleep: %d\n", battery_status.flag.sleep);
      printf("Terminate charge alarm: %d\n", battery_status.flag.tca);
      printf("Terminate discharge alarm: %d\n", battery_status.flag.tda);
      printf("System down: %d\n", battery_status.flag.sysdwn);
    }

    PrintSeparator();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
