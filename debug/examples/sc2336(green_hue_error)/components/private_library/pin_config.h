/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-06 10:32:28
 * @LastEditTime: 2025-03-18 16:15:48
 */
#pragma once

// IIC
#define IIC_1_SDA 7
#define IIC_1_SCL 8
#define IIC_2_SDA 20
#define IIC_2_SCL 21

// BOOT
#define ESP32P4_BOOT 35

// XL9535
#define XL9535_IIC_ADDRESS 0x20
#define XL9535_SDA IIC_1_SDA
#define XL9535_SCL IIC_1_SCL
#define XL9535_INT 24
// XL9535引脚功能
#define XL9535_VIBRATION_MOTOR_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(0)
#define XL9535_BATTERY_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(1)
#define XL9535_SCREEN_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(2)
#define XL9535_TOUCH_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(3)
#define XL9535_TOUCH_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(4)
#define XL9535_ETHERNET_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(5)
#define XL9535_3_3_V_POWER_EN static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(6)
#define XL9535_EXTERNAL_SENSOR_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(7)
#define XL9535_GPS_PPS static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(10)
#define XL9535_GPS_WAKE_UP static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(11)
#define XL9535_RTC_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(12)
#define XL9535_ESP32C6_WAKE_UP static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(13)
#define XL9535_ESP32C6_EN static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(14)
#define XL9535_LORA_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(16)
#define XL9535_LORA_DIO1 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(17)

// ES8311 IIC
#define ES8311_IIC_ADDRESS 0x18
#define ES8311_SDA IIC_2_SDA
#define ES8311_SCL IIC_2_SCL
// ES8311 IIS
#define ES8311_ADC_DATA 11
#define ES8311_DAC_DATA 10
#define ES8311_BCLK 12
#define ES8311_MCLK 13
#define ES8311_WS_LRCK 9

// AW86224
#define AW86224_IIC_ADDRESS 0x58
#define AW86224_SDA IIC_2_SDA
#define AW86224_SCL IIC_2_SCL

// SGM38121
#define SGM38121_IIC_ADDRESS 0x28
#define SGM38121_SDA IIC_2_SDA
#define SGM38121_SCL IIC_2_SCL

// Camera
#define CAMERA_SDA IIC_2_SDA
#define CAMERA_SCL IIC_2_SCL

// PCF8563
#define PCF8563_IIC_ADDRESS 0x51
#define PCF8563_SDA IIC_1_SDA
#define PCF8563_SCL IIC_1_SCL

// BQ27220
#define BQ27220_IIC_ADDRESS 0x55
#define BQ27220_SDA IIC_1_SDA
#define BQ27220_SCL IIC_1_SCL

// SPI
#define SPI_1_SCLK 2
#define SPI_1_MOSI 3
#define SPI_1_MISO 4

// SX1262
#define LORA_CS 5
#define LORA_DIO1 -1
#define LORA_DIO3 -1
#define LORA_RST -1
#define LORA_BUSY 6
#define LORA_SCLK 2
#define LORA_MOSI 3
#define LORA_MISO 4

// L76K
#define GPS_TX 22
#define GPS_RX 23

// ICM20948
#define ICM20948_IIC_ADDRESS 0x68
#define ICM20948_SDA IIC_2_SDA
#define ICM20948_SCL IIC_2_SCL

// HI8561 SCREEN
#define HI8561_SCREEN_BL 51

// HI8561 TOUCH
#define HI8561_TOUCH_IIC_ADDRESS 0x68
#define HI8561_TOUCH_SDA IIC_1_SDA
#define HI8561_TOUCH_SCL IIC_1_SCL
