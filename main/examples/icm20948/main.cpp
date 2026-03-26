/*
 * @Description: icm20948
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:59:51
 * @LastEditTime: 2026-03-26 15:00:36
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "ICM20948_WE.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
#include "kode_bq25896.h"
#endif

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Icm20948 = std::make_unique<ICM20948_WE>(&Wire1, ICM20948_IIC_ADDRESS);

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
