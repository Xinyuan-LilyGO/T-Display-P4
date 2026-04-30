/*
 * @Description: bq27220
 * @Author: LILYGO_L
 * @Date: 2025-01-04 15:06:05
 * @LastEditTime: 2026-04-25 16:41:02
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& bq27220 = driver.chip().bq27220;

  while (1) {
    printf("////////////////////////////////////////////////////\n");
    printf(
        "----------------------------------------------------------------------"
        "----\n");
    printf("Bq27220 id: %#X\n", bq27220->GetDeviceId());
    printf(
        "----------------------------------------------------------------------"
        "----\n");
    printf("Design capacity: %dmah\n", bq27220->GetDesignCapacity());
    printf("Remaining capacity: %dmah\n", bq27220->GetRemainingCapacity());
    // 放电后才更新full_charge_capacity
    printf("Full charge capacity: %dmah\n", bq27220->GetFullChargeCapacity());
    printf("Raw coulomb count: %dc\n", bq27220->GetRawCoulombCount());
    printf("Cycle count: %d\n", bq27220->GetCycleCount());
    printf("Battery level: %d%%\n", bq27220->GetStatusOfHealth());
    printf("Battery health: %d%%\n", bq27220->GetStatusOfHealth());
    printf(
        "----------------------------------------------------------------------"
        "----\n");
    printf("Voltage: %dmv\n", bq27220->GetVoltage());
    int16_t current = bq27220->GetCurrent();
    printf("Charging voltage: %dmv\n", bq27220->GetChargingVoltage());
    printf("Current: %dma\n", current);
    printf("Charging current: %dma\n", bq27220->GetChargingCurrent());
    printf("Standby current: %dma\n", bq27220->GetStandbyCurrent());
    printf("Max load current: %dma\n", bq27220->GetMaxLoadCurrent());
    printf("Average power: %dmw\n", bq27220->GetAveragePower());
    printf(
        "----------------------------------------------------------------------"
        "----\n");
    printf("Chip temperature: %.03f^C\n", bq27220->GetChipTemperatureCelsius());
    printf("NTC temperature: %.03f^C\n", bq27220->GetTemperatureCelsius());
    printf(
        "----------------------------------------------------------------------"
        "----\n");
    bq27220->SetAtRate(current);
    printf("At rate: %dma\n", bq27220->GetAtRate());
    printf("At rate battery time to empty: %dmin\n",
           bq27220->GetAtRateTimeToEmpty());
    printf("Battery time to empty: %dmin\n", bq27220->GetTimeToEmpty());
    printf("Battery time to full charge: %dmin\n", bq27220->GetTimeToFull());
    printf("Battery standby time to empty: %dmin\n",
           bq27220->GetStandbyTimeToEmpty());
    printf("Battery max load time to empty: %dmin\n",
           bq27220->GetMaxLoadTimeToEmpty());
    printf(
        "----------------------------------------------------------------------"
        "----\n");

    // cpp_bus_driver::Bq27220xxxx::OperationStatus os;
    // bq27220->GetOperationStatus(os);

    cpp_bus_driver::Bq27220xxxx::BatteryStatus bs;
    if (bq27220->GetBatteryStatus(bs)) {
      printf("Fully discharged flag: %d\n", bs.flag.fd);
      printf("Sleep flag: %d\n", bs.flag.sleep);
      printf("Charging overheat flag: %d\n", bs.flag.otc);
      printf("Discharging overheat flag: %d\n", bs.flag.otd);
      printf("Fully charged flag: %d\n", bs.flag.fc);
      printf("Charging prohibited flag: %d\n", bs.flag.chginh);
      printf("Terminate charging alarm flag: %d\n", bs.flag.tca);
      printf("Terminate discharging alarm flag: %d\n", bs.flag.tda);
      printf("Battery insertion detection flag: %d\n", bs.flag.auth_gd);
      printf("Battery present flag: %d\n", bs.flag.battpres);
      printf("Discharge flag: %d\n", bs.flag.dsg);
    }
    printf(
        "----------------------------------------------------------------------"
        "----\n");
    printf("////////////////////////////////////////////////////\n");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
