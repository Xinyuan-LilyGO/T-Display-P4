/*
 * @Description: icm20948
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:59:51
 * @LastEditTime: 2026-04-25 16:43:14
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto icm20948 = driver.chip().icm20948.get();

  while (1) {
    icm20948->readSensor();

    xyzFloat gValue;
    icm20948->getGValues(&gValue);
    xyzFloat angle;
    icm20948->getAngles(&angle);
    float pitch = icm20948->getPitch();
    float roll = icm20948->getRoll();

    // 获取磁力计的 x, y 值以计算航向角（Yaw）
    xyzFloat magValues;
    icm20948->getMagValues(&magValues);
    float yaw = atan2(magValues.y, magValues.x) * (180.0 / M_PI);  // 计算航向角

    printf("gValue (x,y,z): %f , %f , %f\n", gValue.x, gValue.y, gValue.z);
    printf("angle (x,y,z): %f , %f , %f\n", angle.x, angle.y, angle.z);
    printf("magValues (x,y,z): %f , %f , %f\n", magValues.x, magValues.y,
           magValues.z);

    printf("Euler angles (pitch,Roll,Yaw): %f , %f , %f\n", pitch, roll, yaw);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
