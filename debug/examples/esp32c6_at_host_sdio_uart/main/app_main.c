/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "nvs_flash.h"

#include "sdio_host_log.h"
#include "sdio_host_transport.h"
#include "sdio_config.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "time.h"

#define WRITE_BUFFER_LEN 2048
#define READ_BUFFER_LEN 4096

uint8_t send_buffer[WRITE_BUFFER_LEN] = "";
uint8_t rcv_buffer[READ_BUFFER_LEN] = "";

static const char TAG[] = "sdio_at_host";

int sdio_debugLevel = 2; // print info log, set to 3 if you want to debug
static SemaphoreHandle_t rdySem;
static QueueHandle_t esp_at_uart_queue = NULL;

#define SDIO_ERROR_CHECK(x)                                                                    \
    do                                                                                         \
    {                                                                                          \
        sdio_err_t __err_rc = (x);                                                             \
        if (__err_rc != SUCCESS)                                                               \
        {                                                                                      \
            SDIO_LOGE(TAG, "file: \"%s\" line %d\nfunc: %s\n error: %d\n", __FILE__, __LINE__, \
                      __FUNCTION__, x);                                                        \
        }                                                                                      \
    } while (0);

static void sdio_recv_task(void *pvParameters)
{
    while (1)
    {
        // esp_err_t ret = sdio_host_wait_int(1000 / portTICK_PERIOD_MS);

        // if (ret != SUCCESS)
        // {
        //     continue;
        // }

        uint32_t intr_raw, intr_st;
        esp_err_t ret = sdio_host_get_intr(&intr_raw, &intr_st);
        SDIO_ERROR_CHECK(ret);

        if (intr_raw == 0)
        {
            // SDIO_LOGW(TAG, "No intr\r\n");
            continue;
        }

        ret = sdio_host_clear_intr(intr_raw);
        SDIO_ERROR_CHECK(ret);
        SDIO_LOGD(TAG, "intr raw: %x, intr_st: %x", (unsigned int)intr_raw, (unsigned int)intr_st);

        const int wait_ms = 50;

        if (intr_raw & HOST_SLC0_RX_NEW_PACKET_INT_ST)
        {
            SDIO_LOGD(TAG, "new packet coming");

            while (1)
            {
                size_t size_read = READ_BUFFER_LEN;
                ret = sdio_host_get_packet(rcv_buffer, READ_BUFFER_LEN, &size_read, wait_ms);

                if (ret == ERR_NOT_FOUND)
                {
                    SDIO_LOGE(TAG, "interrupt but no data can be read");
                    break;
                }
                else if (ret != SUCCESS && ret != ERR_NOT_FINISHED)
                {
                    SDIO_LOGE(TAG, "rx packet error: %08X", ret);
                    break;
                }

                {
                    printf("%s", rcv_buffer);
                    // uart_write_bytes(UART_NUM_0, rcv_buffer, size_read);
                    fflush(stdout);
                }

                memset(rcv_buffer, 0x0, sizeof(rcv_buffer));

                if (ret == SUCCESS)
                {
                    break;
                }
            }
        }
    }

    vTaskDelete(NULL);
}

static void sdio_task(void *pvParameters)
{
    sdio_err_t err;
    int at_uart_len;
    printf("Start SDIO test\r\n");
    // Make sure SDIO slave has been inited
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    SDIO_LOGI(TAG, "host ready, start initializing slave...");
    err = sdio_init();
    assert(err == ESP_OK);

    uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);

    xTaskCreate(sdio_recv_task, "sdioRecvTask", 2048, NULL, 4, NULL);

    uart_flush_input(UART_NUM_0);

    while (1)
    {
        size_t buffer_lenght = 0;
        uart_get_buffered_data_len(UART_NUM_0, &buffer_lenght);
        if (buffer_lenght > 0)
        {
            uint8_t *buffer = (uint8_t *)malloc(buffer_lenght);
            uart_read_bytes(UART_NUM_0, buffer, buffer_lenght, pdMS_TO_TICKS(20));

            uart_write_bytes(UART_NUM_0, "uart receive: ", strlen("uart receive: "));
            uart_write_bytes(UART_NUM_0, buffer, buffer_lenght);

            err = sdio_host_send_packet(buffer, buffer_lenght);
            // Send timeout
            if (err == ERR_TIMEOUT)
            {
                SDIO_LOGW(TAG, "send timeout");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        // memset(send_buffer, '\0', sizeof(send_buffer));
    }
    vTaskDelete(NULL);
}

void app_main()
{
    xTaskCreate(sdio_task, "sdio_task", 3 * 1024, NULL, 5, NULL);
}
