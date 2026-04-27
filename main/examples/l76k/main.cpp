/*
 * @Description: l76k
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:32:01
 * @LastEditTime: 2026-04-25 16:43:35
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto l76k = driver.chip().l76k.get();

  while (1) {
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t buffer_lenght = 0;

    if (l76k->GetInfoData(buffer, &buffer_lenght)) {
      // 打印RMC的相关信息
      printf("---begin---\n%s \n---end---\n", buffer.get());

      printf("------------RMC------------\n");

      cpp_bus_driver::L76k::Rmc rmc;

      if (l76k->ParseRmcInfo(buffer.get(), buffer_lenght, rmc)) {
        printf("Location status: %s\n", (rmc.location_status).c_str());

        if (rmc.data.update_flag) {
          printf("Utc data: %d/%d/%d\n", rmc.data.year + 2000, rmc.data.month,
                 rmc.data.day);
          rmc.data.update_flag = false;
        }
        if (rmc.utc.update_flag) {
          printf("Utc time: %d:%d:%.03f\n", rmc.utc.hour, rmc.utc.minute,
                 rmc.utc.second);
          printf("China time: %d:%d:%.03f\n", (rmc.utc.hour + 8 + 24) % 24,
                 rmc.utc.minute, rmc.utc.second);
          rmc.utc.update_flag = false;
        }

        if (rmc.location.lat.update_flag &&
            rmc.location.lat.direction_update_flag) {
          printf(
              "Location lat degrees: %d \nLocation lat minutes: %.10lf "
              "\nLocation lat degrees_minutes: %.10lf \nLocation lat "
              "direction: %s\n",
              rmc.location.lat.degrees, rmc.location.lat.minutes,
              rmc.location.lat.degrees_minutes,
              (rmc.location.lat.direction).c_str());
          rmc.location.lat.update_flag = false;
          rmc.location.lat.direction_update_flag = false;
        }
        if (rmc.location.lon.update_flag &&
            rmc.location.lon.direction_update_flag) {
          printf(
              "Location lon degrees: %d \nLocation lon minutes: %.10lf "
              "\nLocation lon degrees_minutes: %.10lf \nLocation lon "
              "direction: %s\n",
              rmc.location.lon.degrees, rmc.location.lon.minutes,
              rmc.location.lon.degrees_minutes,
              (rmc.location.lon.direction).c_str());
          rmc.location.lon.update_flag = false;
          rmc.location.lon.direction_update_flag = false;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
