/*
 * @Description: icm20948
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:59:51
 * @LastEditTime: 2026-03-06 09:07:15
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "ICM20948_WE.h"

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Icm20948 = std::make_unique<ICM20948_WE>(&Wire1, ICM20948_IIC_ADDRESS);

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    Wire1.begin(ICM20948_SDA, ICM20948_SCL);
    while (Icm20948->init() == false)
    {
        printf("ICM20948 AG initialization failed\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (Icm20948->initMagnetometer() == false)
    {
        printf("ICM20948 M initialization failed\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("ICM20948 initialization successful\n");

    printf("Position your ICM20948 flat and don't move it - calibrating...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    Icm20948->autoOffsets();
    printf("Done!\n");

    Icm20948->setAccRange(ICM20948_ACC_RANGE_2G);
    Icm20948->setAccDLPF(ICM20948_DLPF_6);
    Icm20948->setMagOpMode(AK09916_CONT_MODE_20HZ);

    while (1)
    {
        Icm20948->readSensor();

        xyzFloat gValue;
        Icm20948->getGValues(&gValue);
        xyzFloat angle;
        Icm20948->getAngles(&angle);
        float pitch = Icm20948->getPitch();
        float roll = Icm20948->getRoll();

        // 获取磁力计的 x, y 值以计算航向角（Yaw）
        xyzFloat magValues;
        Icm20948->getMagValues(&magValues);
        float yaw = atan2(magValues.y, magValues.x) * (180.0 / M_PI); // 计算航向角

        printf("gValue (x,y,z): %f , %f , %f\n", gValue.x, gValue.y, gValue.z);
        printf("angle (x,y,z): %f , %f , %f\n", angle.x, angle.y, angle.z);
        printf("magValues (x,y,z): %f , %f , %f\n", magValues.x, magValues.y, magValues.z);

        printf("Euler angles (pitch,Roll,Yaw): %f , %f , %f\n", pitch, roll, yaw);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
