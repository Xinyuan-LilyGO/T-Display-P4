/*
 * @Description: t_display_p4_keyboard_config
 * @Author: LILYGO_L
 * @Date: 2024-12-06 10:32:28
 * @LastEditTime: 2026-01-14 13:36:31
 */
#pragma once
#include "t_display_p4_config.h"
#include <string>
#include "lvgl.h"

////////////////////////////////////////////////// gpio config //////////////////////////////////////////////////

#define IIC_3_SDA EXT_1X4P_2_IO_46
#define IIC_3_SCL EXT_1X4P_2_IO_45

// XL9555
#define XL9555_SDA IIC_3_SDA
#define XL9555_SCL IIC_3_SCL
// XL9555引脚功能
#define XL9555_T_MIXRF_EN Cpp_Bus_Driver::Xl95x5::Pin::IO0
#define XL9555_T_MIXRF_CC1101_RF_SWITCH_0 Cpp_Bus_Driver::Xl95x5::Pin::IO1
#define XL9555_T_MIXRF_CC1101_RF_SWITCH_1 Cpp_Bus_Driver::Xl95x5::Pin::IO2
#define XL9555_LED_1 Cpp_Bus_Driver::Xl95x5::Pin::IO3
#define XL9555_LED_2 Cpp_Bus_Driver::Xl95x5::Pin::IO4
#define XL9555_LED_3 Cpp_Bus_Driver::Xl95x5::Pin::IO5
#define XL9555_TCA8418_RST Cpp_Bus_Driver::Xl95x5::Pin::IO6
#define XL9555_T_MIXRF_LR1121_INT Cpp_Bus_Driver::Xl95x5::Pin::IO7
#define XL9555_T_MIXRF_LR1121_RST Cpp_Bus_Driver::Xl95x5::Pin::IO10
#define XL9555_T_MIXRF_LR1121_CS Cpp_Bus_Driver::Xl95x5::Pin::IO11
#define XL9555_T_MIXRF_LR1121_BUSY Cpp_Bus_Driver::Xl95x5::Pin::IO12

// SY7200A
#define SY7200A_EN_PWM EXT_1X4P_1_IO_47

// TCA8418
#define TCA8418_SDA IIC_3_SDA
#define TCA8418_SCL IIC_3_SCL
#define TCA8418_INT EXT_1X4P_1_IO_48

// Keyboard
#define KEYBOARD_BL SY7200A_EN_PWM

// T-MixRF
#define T_MIXRF_CC1101_CS EXT_2X8P_IO_36
#define T_MIXRF_CC1101_SCLK EXT_2X8P_SPI_SCLK
#define T_MIXRF_CC1101_MOSI EXT_2X8P_SPI_MOSI
#define T_MIXRF_CC1101_MISO EXT_2X8P_SPI_MISO
#define T_MIXRF_CC1101_GDO0 EXT_2X8P_IO_25
#define T_MIXRF_CC1101_GDO2 EXT_2X8P_IO_33
#define T_MIXRF_CC1101_INT T_MIXRF_CC1101_GDO0
#define T_MIXRF_CC1101_BUSY T_MIXRF_CC1101_GDO2

#define T_MIXRF_NRF24L01_CS EXT_2X8P_IO_54
#define T_MIXRF_NRF24L01_SCLK EXT_2X8P_SPI_SCLK
#define T_MIXRF_NRF24L01_MOSI EXT_2X8P_SPI_MOSI
#define T_MIXRF_NRF24L01_MISO EXT_2X8P_SPI_MISO
#define T_MIXRF_NRF24L01_CE EXT_2X8P_IO_53
#define T_MIXRF_NRF24L01_INT EXT_2X8P_IO_32

#define T_MIXRF_ST25R3916_CS EXT_2X8P_IO_27
#define T_MIXRF_ST25R3916_SCLK EXT_2X8P_SPI_SCLK
#define T_MIXRF_ST25R3916_MOSI EXT_2X8P_SPI_MOSI
#define T_MIXRF_ST25R3916_MISO EXT_2X8P_SPI_MISO
#define T_MIXRF_ST25R3916_INT EXT_2X8P_IO_26

#define T_MIXRF_LR1121_SCLK EXT_2X8P_SPI_SCLK
#define T_MIXRF_LR1121_MOSI EXT_2X8P_SPI_MOSI
#define T_MIXRF_LR1121_MISO EXT_2X8P_SPI_MISO

// BQ25896
#define BQ25896_SDA IIC_3_SDA
#define BQ25896_SCL IIC_3_SCL

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
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
        "Caps", "a", "s", "d", "f", "g", "h", "j", "k", "l",
        "Alt", "z", "x", "c", "v", "b", "n", "m", "Ctrl", "Up",
        "Fn", "Win", "Shift", "Tab", "Space", "Space", "Space", "Fn", "Left", "Down",
        "F11", "9", "Del", "Enter", "Record", "Enter", "0", "Right"};

// TCA8418 Lvgl键盘按键映射
constexpr const uint32_t Tca8418_Map_Lvgl[] =
    {
        // F1-F10  使用ASCII控制字符范围
        0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,

        LV_KEY_ESC, LV_KEY_ESC,
        '1', '2', '3', '4', '5', '6', '7', '8',
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
        0x8B, // Caps Lock  自定义值
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
        0x8C, // Alt  自定义值
        'z', 'x', 'c', 'v', 'b', 'n', 'm',
        0x8D, // Ctrl  自定义值
        LV_KEY_UP,
        0x8E,        // Fn  自定义值
        0x8F,        // Win  自定义值
        0x90,        // Shift  自定义值
        LV_KEY_NEXT, // Tab
        ' ',         // Space
        ' ',         // Space
        ' ',         // Space
        0x8E,        // Fn 自定义值
        LV_KEY_LEFT,
        LV_KEY_DOWN,

        0x91, // F11  自定义值
        '9',
        LV_KEY_BACKSPACE,
        LV_KEY_ENTER,
        0x92, // Record 自定义值
        LV_KEY_ENTER,
        '0',
        LV_KEY_RIGHT};

constexpr const uint32_t Tca8418_Map_Lvgl_Shift[] =
    {
        // F1-F10  使用ASCII控制字符范围
        0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,

        0x8B, 0x8C,
        '!', '@', '#', '$', '%', '^', '&', '*',
        '\'', '_', '-', '+', '=', '\\', '|', ';', ':', '"',
        0x8D, // Caps Lock  自定义值
        '~', '[', ']', '{', '}', '\'', '`', '/', '?',
        0x8E, // Alt  自定义值
        0x8F, 0x90, 0x91, 0x92, '.', '<', '>',
        0x93, // Ctrl  自定义值
        0x94,
        0x95, // Fn  自定义值
        0x96, // Win  自定义值
        0x97, // Shift  自定义值
        0x98, // Tab
        0x99, // Space
        0x9A, // Space
        0x9B, // Space
        0x9C, // Fn 自定义值
        0x9D,
        0x9E,

        0x9F, // F11  自定义值
        '(',
        0xA0,
        0xA1,
        0xA2, // Record 自定义值
        0xA3,
        ')',
        0xA4};

// BQ25896
#define BQ25896_IIC_ADDRESS 0x6B

////////////////////////////////////////////////// other define config //////////////////////////////////////////////////