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
#include "tool.h"

#define BOOT_KEY 35

bool Interrupt_Trigger = false;

Cpp_Bus_Driver::Tool tool;

// void IRAM_ATTR Interrupt(void *arg)
// {
//     if ((uint32_t)arg == BOOT_KEY)
//     {
//         Interrupt_Trigger = true;
//     }
// }

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    tool.create_gpio_interrupt(BOOT_KEY, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                               [](void *arg) IRAM_ATTR
                               {
                                   if ((uint32_t)arg == BOOT_KEY)
                                   {
                                       Interrupt_Trigger = true;
                                   }
                               });

    tool.delete_gpio_interrupt(BOOT_KEY);

    tool.create_gpio_interrupt(BOOT_KEY, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                               [](void *arg) IRAM_ATTR
                               {
                                   if ((uint32_t)arg == BOOT_KEY)
                                   {
                                       Interrupt_Trigger = true;
                                   }
                               });

    while (1)
    {
        if (Interrupt_Trigger == true)
        {
            Interrupt_Trigger = false;

            printf("Boot pin interrupt trigger!\n");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        printf("6\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
