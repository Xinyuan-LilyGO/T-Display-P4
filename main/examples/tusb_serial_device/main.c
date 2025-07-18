/*
 * @Description: tusb_serial_device
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:03:32
 * @LastEditTime: 2025-06-13 14:03:48
 * @License: GPL 3.0
 */
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "sdkconfig.h"

#define PREPEND_STRING "esp32p4 hardware usb cdc receive: "
#define PREPEND_LENGTH 34

const char *TAG = "example";
uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

/**
 * @brief Application Queue
 */
QueueHandle_t app_queue;
typedef struct
{
    uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + PREPEND_LENGTH + 1]; // Data buffer
    size_t buf_len;                                                  // Number of bytes received
    uint8_t itf;                                                     // Index of CDC device interface
} app_message_t;

/**
 * @brief CDC device RX callback
 *
 * CDC device signals, that new data were received
 *
 * @param[in] itf   CDC device index
 * @param[in] event CDC event type
 */
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(itf, rx_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK)
    {
        app_message_t tx_msg = {
            .buf_len = rx_size + PREPEND_LENGTH,
            .itf = itf,
        };

        memcpy(tx_msg.buf, PREPEND_STRING, PREPEND_LENGTH);
        memcpy(tx_msg.buf + PREPEND_LENGTH, rx_buf, rx_size);
        xQueueSend(app_queue, &tx_msg, 0);
    }
    else
    {
        ESP_LOGE(TAG, "Read Error");
    }
}

/**
 * @brief CDC device line change callback
 *
 * CDC device signals, that the DTR, RTS states changed
 *
 * @param[in] itf   CDC device index
 * @param[in] event CDC event type
 */
void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}

void Hardware_Usb_Cdc_Init(void)
{
    // Create FreeRTOS primitives
    app_queue = xQueueCreate(5, sizeof(app_message_t));
    assert(app_queue);

    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = NULL,
        .hs_configuration_descriptor = NULL,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = NULL,
#endif // TUD_OPT_HIGH_SPEED
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL};

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    /* the second way to register a callback */
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
        TINYUSB_CDC_ACM_0,
        CDC_EVENT_LINE_STATE_CHANGED,
        &tinyusb_cdc_line_state_changed_callback));

#if (CONFIG_TINYUSB_CDC_COUNT > 1)
    acm_cfg.cdc_port = TINYUSB_CDC_ACM_1;
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
        TINYUSB_CDC_ACM_1,
        CDC_EVENT_LINE_STATE_CHANGED,
        &tinyusb_cdc_line_state_changed_callback));
#endif

    ESP_LOGI(TAG, "USB initialization DONE");
}

void hardware_usb_cdc_task(void *arg)
{
    printf("hardware_usb_cdc_task start\n");

    while (1)
    {
        app_message_t msg;
        if (xQueueReceive(app_queue, &msg, portMAX_DELAY))
        {
            if (msg.buf_len)
            {
                /* Print received data*/
                ESP_LOGI(TAG, "Data from channel %d:", msg.itf);
                ESP_LOG_BUFFER_HEXDUMP(TAG, msg.buf, msg.buf_len, ESP_LOG_INFO);

                /* write back */
                tinyusb_cdcacm_write_queue(msg.itf, msg.buf, msg.buf_len);
                esp_err_t err = tinyusb_cdcacm_write_flush(msg.itf, 0);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "CDC ACM write flush error: %s", esp_err_to_name(err));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    printf("Ciallo\n");

    Hardware_Usb_Cdc_Init();

    xTaskCreate(hardware_usb_cdc_task, "hardware_usb_cdc_task", 4 * 1024, NULL, 3, NULL);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
