/*
 * @Description: t-display-p4 config
 * @Author: LILYGO_L
 * @Date: 2024-12-06 10:32:28
 * @LastEditTime: 2025-07-31 14:50:26
 */
#pragma once
#include "t_display_p4_config.h"
#include <string>

////////////////////////////////////////////////// gpio config //////////////////////////////////////////////////

#define IIC_3_SDA EXT_1X4P_2_IO_46
#define IIC_3_SCL EXT_1X4P_2_IO_45

// XL9555
#define XL9555_SDA IIC_3_SDA
#define XL9555_SCL IIC_3_SCL
// XL9555引脚功能
#define XL9555_LED_1 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(3)
#define XL9555_LED_2 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(4)
#define XL9555_LED_3 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(5)
#define XL9555_TCA8418_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(6)

// SY7200A
#define SY7200A_EN_PWM EXT_1X4P_1_IO_47

// TCA8418
#define TCA8418_SDA IIC_3_SDA
#define TCA8418_SCL IIC_3_SCL
#define TCA8418_INT EXT_1X4P_1_IO_48

// Keyboard
#define KEYBOARD_BL SY7200A_EN_PWM

////////////////////////////////////////////////// gpio config //////////////////////////////////////////////////

////////////////////////////////////////////////// other define config //////////////////////////////////////////////////

// XL9555
#define XL9555_IIC_ADDRESS 0x20

// TCA8418
#define TCA8418_IIC_ADDRESS 0x34
#define TCA8418_KEYPAD_SCAN_WIDTH 10
#define TCA8418_KEYPAD_SCAN_HEIGHT 7
// TCA8418键盘按键映射
constexpr const std::string Tca8418_Map[] =
    {
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
        "Esc", "Esc", "1", "2", "3", "4", "5", "6", "7", "8",
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
        "Caps", "A", "S", "D", "F", "G", "H", "J", "K", "L",
        "Alt", "Z", "X", "C", "V", "B", "N", "M", "Ctrl", "Up",
        "Fn", "Win", "Shift", "Tab", "Space", "Space", "Space", "Fn", "Left", "Down",
        "F11", "9", "Del", "Enter", "Record", "Enter", "0", "Right"};

////////////////////////////////////////////////// other define config //////////////////////////////////////////////////