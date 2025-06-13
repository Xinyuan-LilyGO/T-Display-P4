/*
 * @Description: icm20948
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:59:51
 * @LastEditTime: 2025-06-13 13:47:15
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "pin_config.h"
#include "cpp_bus_driver_library.h"
#include "ICM20948_WE.h"

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto ICM20948_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ICM20948_SDA, ICM20948_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto ICM20948 = std::make_unique<ICM20948_WE>(ICM20948_IIC_Bus, ICM20948_IIC_ADDRESS);

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    while (ICM20948->begin() == false)
    {
        printf("ICM20948 AG initialization failed\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (ICM20948->initMagnetometer() == false)
    {
        printf("ICM20948 M initialization failed\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("ICM20948 initialization successful\n");

    printf("Position your ICM20948 flat and don't move it - calibrating...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    ICM20948->autoOffsets();
    printf("Done!\n");

    ICM20948->setAccRange(ICM20948_ACC_RANGE_2G);
    ICM20948->setAccDLPF(ICM20948_DLPF_6);
    ICM20948->setMagOpMode(AK09916_CONT_MODE_20HZ);

    while (1)
    {
        ICM20948->readSensor();
        xyzFloat gValue = ICM20948->getGValues();
        xyzFloat angle = ICM20948->getAngles();
        float pitch = ICM20948->getPitch();
        float roll = ICM20948->getRoll();

        // 获取磁力计的 x, y 值以计算航向角（Yaw）
        xyzFloat magValues = ICM20948->getMagValues();
        float yaw = atan2(magValues.y, magValues.x) * (180.0 / M_PI); // 计算航向角

        printf("gValue (x,y,z): %f , %f , %f\n", gValue.x, gValue.y, gValue.z);
        printf("angle (x,y,z): %f , %f , %f\n", angle.x, angle.y, angle.z);
        printf("magValues (x,y,z): %f , %f , %f\n", magValues.x, magValues.y, magValues.z);

        printf("Euler angles (pitch,Roll,Yaw): %f , %f , %f\n", pitch, roll, yaw);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
