/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BOOT_KEY 35

bool Interrupt_Trigger = false;

static void IRAM_ATTR exit_gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;

    if (gpio_num == BOOT_KEY)
    {
        Interrupt_Trigger = true;
    }
}

/**
 * @brief       外部中断初始化程序
 * @param       无
 * @retval      无
 */
void exit_init(void)
{
    gpio_config_t config =
        {
            .pin_bit_mask = 1ull << BOOT_KEY,      /* 配置BOOT按键引脚 */
            .mode = GPIO_MODE_INPUT,               /* 选择为输入模式 */
            .pull_up_en = GPIO_PULLUP_ENABLE,      /* 上拉使能 */
            .pull_down_en = GPIO_PULLDOWN_DISABLE, /* 下拉失能 */
            .intr_type = GPIO_INTR_NEGEDGE,        /* 下降沿触发 */
        };

    gpio_config(&config); /* 配置使能 */

    /* 注册中断服务 */
    gpio_install_isr_service(0);
    /*
    入参：
    中断等级（1-7）
    ESP_INTR_FLAG_LEVEL1 ：
    ESP_INTR_FLAG_LEVEL2 ：
    ESP_INTR_FLAG_LEVEL3 ：
    ESP_INTR_FLAG_LEVEL4 ：
    ESP_INTR_FLAG_LEVEL5 ：
    ESP_INTR_FLAG_LEVEL6 ：
    ESP_INTR_FLAG_NMI    ：          //这个是等级7
    ESP_INTR_FLAG_SHARED ：          //中断可以在ISR之间进行共享
    ESP_INTR_FLAG_EDGE   ：          //边沿触发方式，使能GPIO边沿触发中断
    ESP_INTR_FLAG_IRAM   ：          //如果缓存被禁用，ISR可以被调用
    ESP_INTR_FLAG_INTRDISABLED  ：   //返回时禁用此中断
    ESP_INTR_FLAG_LOWMED ：          //使用低电平触发方式。使能GPIO低电平触发中断
    ESP_INTR_FLAG_HIGH   ：          //使用高电平触发方式。使能GPIO高电平触发中断
    ESP_INTR_FLAG_LEVELMASK     ：   //这个不明白是什么意思
    */

    /* 设置GPIO的中断回调函数 */
    gpio_isr_handler_add(BOOT_KEY, exit_gpio_isr_handler, (void *)BOOT_KEY); // 设置GPIO口的中断回调函数

    /* 使能GPIO模块中断信号 */
    gpio_intr_enable(BOOT_KEY); // 使能GPIO模块的中断信号
}

void app_main(void)
{
    printf("Ciallo\n");

    exit_init();

    while (1)
    {
        if (Interrupt_Trigger == true)
        {
            Interrupt_Trigger = false;

            printf("Boot pin interrupt trigger!\n");
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        printf("6\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
