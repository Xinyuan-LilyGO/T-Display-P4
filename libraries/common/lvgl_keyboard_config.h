/*
 * @Description: t_display_p4_keyboard_config
 * @Author: LILYGO_L
 * @Date: 2024-12-06 10:32:28
 * @LastEditTime: 2026-03-05 14:37:13
 */
#pragma once
#include "lvgl.h"

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

////////////////////////////////////////////////// other define config //////////////////////////////////////////////////