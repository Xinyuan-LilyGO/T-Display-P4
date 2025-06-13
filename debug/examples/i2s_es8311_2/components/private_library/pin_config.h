/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-06 10:32:28
 * @LastEditTime: 2025-03-11 11:19:34
 */
#pragma once

// IIC
#define IIC_1_SDA 7
#define IIC_1_SCL 8
#define IIC_2_SDA 20
#define IIC_2_SCL 21

// XL9535
#define XL9535_IIC_ADDRESS 0x20
#define XL9535_SDA IIC_1_SDA
#define XL9535_SCL IIC_1_SCL
#define XL9535_INT 24
// XL9535引脚功能
#define XL9535_IO0_VIBRATION_MOTOR_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(0)
#define XL9535_IO1_BATTERY_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(1)
#define XL9535_IO2_SCREEN_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(2)
#define XL9535_IO3_TOUCH_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(3)
#define XL9535_IO4_TOUCH_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(4)
#define XL9535_IO5_ETHERNET_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(5)
#define XL9535_IO6_3_3_V_POWER_EN static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(6)
#define XL9535_IO7_EXTERNAL_SENSOR_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(7)
#define XL9535_IO10_GPS_PPS static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(10)
#define XL9535_IO11_GPS_WAKE_UP static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(11)
#define XL9535_IO12_RTC_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(12)
#define XL9535_IO13_ESP32C6_WAKE_UP static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(13)
#define XL9535_IO14_ESP32C6_EN static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(14)
#define XL9535_IO15_LORA_DIO3 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(15)
#define XL9535_IO16_LORA_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(16)
#define XL9535_IO17_LORA_DIO1 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(17)

// ES8311
#define ES8311_IIC_ADDRESS 0x18
#define ES8311_SDA IIC_2_SDA
#define ES8311_SCL IIC_2_SCL

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
#define LORA_RST -1
#define LORA_BUSY 6
#define LORA_SCLK 2
#define LORA_MOSI 3
#define LORA_MISO 4

// L76K
#define GPS_TX 22
#define GPS_RX 23
