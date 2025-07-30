/*
 * @Description: t-display-p4 config
 * @Author: LILYGO_L
 * @Date: 2024-12-06 10:32:28
 * @LastEditTime: 2025-07-30 09:14:55
 */
#pragma once
#include "t_display_p4_config.h"

////////////////////////////////////////////////// gpio config //////////////////////////////////////////////////

// XL9555
#define XL9555_SDA EXT_1X4P_2_IO_46
#define XL9555_SCL EXT_1X4P_2_IO_45
// XL9555引脚功能
#define XL9555_LED_1 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(3)
#define XL9555_LED_2 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(4)
#define XL9555_LED_3 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(5)

// SY7200A
#define SY7200A_EN_PWM EXT_1X4P_1_IO_47


////////////////////////////////////////////////// gpio config //////////////////////////////////////////////////

////////////////////////////////////////////////// other define config //////////////////////////////////////////////////

// XL9555
#define XL9555_IIC_ADDRESS 0x20


////////////////////////////////////////////////// other define config //////////////////////////////////////////////////