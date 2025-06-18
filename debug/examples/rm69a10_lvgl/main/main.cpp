/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "rm69a10_driver.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"

static const char *TAG = "example";

#if CONFIG_EXAMPLE_LCD_USE_RM69A10
// Refresh Rate = 80000000/(80+50+150+568)/(40+120+80+1232) = 64Hz
#define EXAMPLE_MIPI_DSI_DPI_CLK_MHZ 80
#define EXAMPLE_MIPI_DSI_LCD_H_RES 568
#define EXAMPLE_MIPI_DSI_LCD_V_RES 1232
#define EXAMPLE_MIPI_DSI_LCD_HSYNC 50
#define EXAMPLE_MIPI_DSI_LCD_HBP 150
#define EXAMPLE_MIPI_DSI_LCD_HFP 50
#define EXAMPLE_MIPI_DSI_LCD_VSYNC 40
#define EXAMPLE_MIPI_DSI_LCD_VBP 120
#define EXAMPLE_MIPI_DSI_LCD_VFP 80
#endif

#define EXAMPLE_MIPI_DSI_LANE_NUM 2             // 2 data lanes
#define EXAMPLE_MIPI_DSI_LANE_BITRATE_MBPS 1000 // 1Gbps

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your Board Design //////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The "VDD_MIPI_DPHY" should be supplied with 2.5V, it can source from the internal LDO regulator or from external LDO chip
#define EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN 3 // LDO_VO3 is connected to VDD_MIPI_DPHY
#define EXAMPLE_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV 2500
#define EXAMPLE_PIN_NUM_BK_LIGHT -1
#define EXAMPLE_PIN_NUM_LCD_RST -1

#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
#define EXAMPLE_PIN_NUM_REFRESH_MONITOR 20 // Monitor the Refresh Rate by toggling the GPIO
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your Application ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define EXAMPLE_LVGL_DRAW_BUF_LINES (EXAMPLE_MIPI_DSI_LCD_V_RES / 10) // number of display lines in each draw buffer
#define EXAMPLE_LVGL_TICK_PERIOD_MS 2
#define EXAMPLE_LVGL_TASK_STACK_SIZE (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY 2

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::HWIIC>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

// auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::HWIIC>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::XL95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

// auto SGM38121 = std::make_unique<Cpp_Bus_Driver::SGM38121>(IIC_Bus_1, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

extern "C" void example_lvgl_demo_ui(lv_display_t *disp);

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1)
    {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);

        // in case of task watch dog timeout, set the minimal delay to 10ms
        if (time_till_next_ms < 10)
        {
            time_till_next_ms = 10;
        }
        usleep(1000 * time_till_next_ms);
    }
}

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
static bool example_monitor_refresh_rate(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    static int io_level = 0;
    // please note, the real refresh rate should be 2*frequency of this GPIO toggling
    gpio_set_level(EXAMPLE_PIN_NUM_REFRESH_MONITOR, io_level);
    io_level = !io_level;
    return false;
}
#endif

static void example_bsp_enable_dsi_phy_power(void)
{
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
#ifdef EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
#endif
}

#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
static void example_bsp_init_refresh_monitor_io(void)
{
    gpio_config_t monitor_io_conf = {
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_REFRESH_MONITOR,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&monitor_io_conf));
}
#endif

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    printf("XL9535 ID: %#X\n", XL9535->iic_device_id());
    XL9535->pin_mode((Cpp_Bus_Driver::XL95x5::Pin)XL9535_IO6_POWER_EN, Cpp_Bus_Driver::XL95x5::Mode::OUTPUT);
    XL9535->digital_write((Cpp_Bus_Driver::XL95x5::Pin)XL9535_IO6_POWER_EN, Cpp_Bus_Driver::XL95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(1000));

    XL9535->pin_mode((Cpp_Bus_Driver::XL95x5::Pin)XL9535_IO2_SCREEN_RST, Cpp_Bus_Driver::XL95x5::Mode::OUTPUT);
    XL9535->digital_write((Cpp_Bus_Driver::XL95x5::Pin)XL9535_IO2_SCREEN_RST, Cpp_Bus_Driver::XL95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->digital_write((Cpp_Bus_Driver::XL95x5::Pin)XL9535_IO2_SCREEN_RST, Cpp_Bus_Driver::XL95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->digital_write((Cpp_Bus_Driver::XL95x5::Pin)XL9535_IO2_SCREEN_RST, Cpp_Bus_Driver::XL95x5::Value::HIGH);

#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
    example_bsp_init_refresh_monitor_io();
#endif

    example_bsp_enable_dsi_phy_power();
    // example_bsp_init_lcd_backlight();
    // example_bsp_set_lcd_backlight(EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);

    // create MIPI DSI bus first, it will initialize the DSI PHY as well
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = EXAMPLE_MIPI_DSI_LANE_NUM,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = EXAMPLE_MIPI_DSI_LANE_BITRATE_MBPS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

    ESP_LOGI(TAG, "Install MIPI DSI LCD control IO");
    esp_lcd_panel_io_handle_t mipi_dbi_io;
    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // according to the LCD spec
        .lcd_param_bits = 8, // according to the LCD spec
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io));

    ESP_LOGI(TAG, "Install MIPI DSI LCD data panel");
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;
    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = EXAMPLE_MIPI_DSI_DPI_CLK_MHZ,
        .in_color_format = LCD_COLOR_FMT_RGB888,
        .video_timing = {
            .h_size = EXAMPLE_MIPI_DSI_LCD_H_RES,
            .v_size = EXAMPLE_MIPI_DSI_LCD_V_RES,
            .hsync_pulse_width = EXAMPLE_MIPI_DSI_LCD_HSYNC,
            .hsync_back_porch = EXAMPLE_MIPI_DSI_LCD_HBP,
            .hsync_front_porch = EXAMPLE_MIPI_DSI_LCD_HFP,
            .vsync_pulse_width = EXAMPLE_MIPI_DSI_LCD_VSYNC,
            .vsync_back_porch = EXAMPLE_MIPI_DSI_LCD_VBP,
            .vsync_front_porch = EXAMPLE_MIPI_DSI_LCD_VFP,
        },
#if CONFIG_EXAMPLE_USE_DMA2D_COPY_FRAME
        .flags = {
            .use_dma2d = true, // use DMA2D to copy draw buffer into frame buffer
        }
#endif
    };

#if CONFIG_EXAMPLE_LCD_USE_RM69A10
    rm69a10_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t lcd_dev_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_rm69a10(mipi_dbi_io, &lcd_dev_config, &mipi_dpi_panel));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(mipi_dpi_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(mipi_dpi_panel));

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // create a lvgl display
    lv_display_t *display = lv_display_create(EXAMPLE_MIPI_DSI_LCD_H_RES, EXAMPLE_MIPI_DSI_LCD_V_RES);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, mipi_dpi_panel);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB888);
    // create draw buffer
    void *buf1 = NULL;
    void *buf2 = NULL;
    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers");
    // Note:
    // Keep the display buffer in **internal** RAM can speed up the UI because LVGL uses it a lot and it should have a fast access time
    // This example allocate the buffer from PSRAM mainly because we want to save the internal RAM
    size_t draw_buffer_sz = EXAMPLE_MIPI_DSI_LCD_H_RES * EXAMPLE_LVGL_DRAW_BUF_LINES * sizeof(lv_color_t);
    buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf1);
    buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, example_lvgl_flush_cb);

    ESP_LOGI(TAG, "Register DPI panel event callback for LVGL flush ready notification");
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
        .on_refresh_done = example_monitor_refresh_rate,
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(mipi_dpi_panel, &cbs, display));

    ESP_LOGI(TAG, "Use esp_timer as LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Display LVGL Meter Widget");
    _lock_acquire(&lvgl_api_lock);
    example_lvgl_demo_ui(display);
    _lock_release(&lvgl_api_lock);
}
