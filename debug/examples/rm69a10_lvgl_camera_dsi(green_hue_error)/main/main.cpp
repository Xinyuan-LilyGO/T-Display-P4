/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_cache.h"
#include "driver/i2c_master.h"
#include "driver/isp.h"
#include "esp_cam_ctlr_csi.h"
#include "esp_cam_ctlr.h"
#include "example_dsi_init.h"
#include "example_dsi_init_config.h"
#include "example_sensor_init.h"
#include "example_config.h"
#include "esp_timer.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "lvgl.h"

extern "C"
{
#include "rm69a10_driver.h"

#include "esp_sccb_intf.h"
#include "esp_sccb_i2c.h"
#include "esp_cam_sensor.h"
#include "esp_cam_sensor_detect.h"
#include "example_sensor_init_config.h"
}

// #define CAMERA_WIDTH 1280
// #define CAMERA_HEIGHT 720
// #define CAMERA_WIDTH 640
// #define CAMERA_HEIGHT 480
#define CAMERA_WIDTH 1920
#define CAMERA_HEIGHT 1080
// #define CAMERA_WIDTH 800
// #define CAMERA_HEIGHT 800
#define CAMERA_BITS_PER_PIXEL 24 // RGB888

#define SCREEN_WIDTH 568
#define SCREEN_HEIGHT 1232
#define SCREEN_BITS_PER_PIXEL 24 // RGB888

// Refresh Rate = 80000000/(80+50+150+568)/(40+120+80+1232) = 64Hz
// #define MIPI_DSI_LCD_H_RES 568
// #define MIPI_DSI_LCD_V_RES 1232
#define MIPI_DSI_DPI_CLK_MHZ 20
#define MIPI_DSI_LCD_HSYNC 50
#define MIPI_DSI_LCD_HBP 150
#define MIPI_DSI_LCD_HFP 50
#define MIPI_DSI_LCD_VSYNC 40
#define MIPI_DSI_LCD_VBP 120
#define MIPI_DSI_LCD_VFP 80

static const char *TAG = "system";

size_t CycleTime = 0;

esp_cam_ctlr_handle_t cam_handle = NULL;

esp_cam_ctlr_trans_t new_trans;

bool new_trans_Flag = false;

static _lock_t lvgl_api_lock;

esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::HWIIC>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::HWIIC>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::XL95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto SGM38121 = std::make_unique<Cpp_Bus_Driver::SGM38121>(IIC_Bus_1, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

extern "C" void example_lvgl_demo_ui(lv_display_t *disp);

void rotate90(uint8_t *src, uint8_t *dst, int width, int height)
{
    int newWidth = height;
    int newHeight = width;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int srcIndex = (y * width + x) * 3;                     // 源图像中的像素索引
            int dstIndex = (x * newWidth + (newWidth - y - 1)) * 3; // 目标图像中的像素索引

            // 复制RGB值
            dst[dstIndex] = src[srcIndex];         // R
            dst[dstIndex + 1] = src[srcIndex + 1]; // G
            dst[dstIndex + 2] = src[srcIndex + 2]; // B
        }
    }
}

static bool s_camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    esp_cam_ctlr_trans_t new_trans = *(esp_cam_ctlr_trans_t *)user_data;

    trans->buffer = new_trans.buffer;
    trans->buflen = new_trans.buflen;

    new_trans_Flag = true;

    return false;
}

static bool s_camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    return false;
}

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(2);
}

static void example_bsp_enable_dsi_phy_power(void)
{
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;

    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
}

void example_sensor_init_2(int i2c_port, i2c_master_bus_handle_t *out_i2c_bus_handle)
{
    esp_err_t ret = ESP_FAIL;

    // //---------------I2C Init------------------//
    // i2c_master_bus_config_t i2c_bus_conf = {
    //     .clk_source = I2C_CLK_SRC_DEFAULT,
    //     .sda_io_num = EXAMPLE_CAM_SCCB_SDA_IO,
    //     .scl_io_num = EXAMPLE_CAM_SCCB_SCL_IO,
    //     .i2c_port = i2c_port,
    //     .flags.enable_internal_pullup = true,
    // };
    // i2c_master_bus_handle_t i2c_bus_handle = NULL;
    // ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &i2c_bus_handle));

    //---------------SCCB Init------------------//
    esp_sccb_io_handle_t sccb_io_handle = NULL;
    esp_cam_sensor_config_t cam_config = {
        .sccb_handle = sccb_io_handle,
        .reset_pin = -1,
        .pwdn_pin = -1,
        .xclk_pin = -1,
        .sensor_port = ESP_CAM_SENSOR_MIPI_CSI,
    };

    esp_cam_sensor_device_t *cam = NULL;
    for (esp_cam_sensor_detect_fn_t *p = &__esp_cam_sensor_detect_fn_array_start; p < &__esp_cam_sensor_detect_fn_array_end; ++p)
    {
        sccb_i2c_config_t i2c_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = p->sccb_addr,
            .scl_speed_hz = EXAMPLE_CAM_SCCB_FREQ,
        };
        ESP_ERROR_CHECK(sccb_new_i2c_io(IIC_Bus_1->_iic_bus, &i2c_config, &cam_config.sccb_handle));

        cam = (*(p->detect))(&cam_config);
        if (cam)
        {
            if (p->port != ESP_CAM_SENSOR_MIPI_CSI)
            {
                ESP_LOGE(TAG, "detect a camera sensor with mismatched interface");
                return;
            }
            break;
        }
        ESP_ERROR_CHECK(esp_sccb_del_i2c_io(cam_config.sccb_handle));
    }

    if (!cam)
    {
        ESP_LOGE(TAG, "failed to detect camera sensor");
        return;
    }

    esp_cam_sensor_format_array_t cam_fmt_array = {0};
    esp_cam_sensor_query_format(cam, &cam_fmt_array);
    const esp_cam_sensor_format_t *parray = cam_fmt_array.format_array;
    for (int i = 0; i < cam_fmt_array.count; i++)
    {
        ESP_LOGI(TAG, "fmt[%d].name:%s", i, parray[i].name);
    }

    esp_cam_sensor_format_t *cam_cur_fmt = NULL;
    // for (int i = 0; i < cam_fmt_array.count; i++)
    // {
    //     if (!strcmp(parray[i].name, EXAMPLE_CAM_FORMAT))
    //     {
    //         cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[i].name);
    //     }
    // }
    // [2025-03-19 14:52:03.839] I (3921) sc2336: Detected Camera sensor PID=0xcb3a
    // [2025-03-19 14:52:03.839] I (3931) system: fmt[0].name:MIPI_2lane_24Minput_RAW10_1280x720_30fps
    // [2025-03-19 14:52:03.839] I (3931) system: fmt[1].name:MIPI_2lane_24Minput_RAW10_1280x720_50fps
    // [2025-03-19 14:52:03.839] I (3941) system: fmt[2].name:MIPI_2lane_24Minput_RAW10_1280x720_60fps
    // [2025-03-19 14:52:03.839] I (3951) system: fmt[3].name:MIPI_1lane_24Minput_RAW10_1920x1080_25fps
    // [2025-03-19 14:52:03.839] I (3951) system: fmt[4].name:MIPI_2lane_24Minput_RAW10_1920x1080_25fps
    // [2025-03-19 14:52:03.839] I (3961) system: fmt[5].name:MIPI_2lane_24Minput_RAW10_1920x1080_30fps
    // [2025-03-19 14:52:03.839] I (3961) system: fmt[6].name:MIPI_2lane_24Minput_RAW10_800x800_30fps
    // [2025-03-19 14:52:03.839] I (3971) system: fmt[7].name:MIPI_2lane_24Minput_RAW10_640x480_50fps
    // [2025-03-19 14:52:03.839] I (3981) system: fmt[8].name:MIPI_2lane_24Minput_RAW8_1920x1080_30fps
    // [2025-03-19 14:52:03.839] I (3981) system: fmt[9].name:MIPI_2lane_24Minput_RAW8_1280x720_30fps
    // [2025-03-19 14:52:03.839] I (3991) system: fmt[10].name:MIPI_2lane_24Minput_RAW8_800x800_30fps
    // [2025-03-19 14:52:03.839] I (4001) system: fmt[11].name:MIPI_2lane_24Minput_RAW8_1024x600_30fps
    // [2025-03-19 14:52:03.839] I (4001) system: fmt[12].name:DVP_8bit_24Minput_RAW10_1280x720_30fps
    
    // cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[7].name);
    cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[8].name);
    // cam_cur_fmt=(esp_cam_sensor_format_t *)&("MIPI_2lane_24Minput_RAW10_640x480_50fps");

    ret = esp_cam_sensor_set_format(cam, (const esp_cam_sensor_format_t *)cam_cur_fmt);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Format set fail");
    }
    else
    {
        ESP_LOGI(TAG, "Format in use:%s", cam_cur_fmt->name);
    }
    int enable_flag = 1;
    // Set sensor output stream
    ret = esp_cam_sensor_ioctl(cam, ESP_CAM_SENSOR_IOC_S_STREAM, &enable_flag);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Start stream fail");
    }
    ESP_ERROR_CHECK(ret);

    *out_i2c_bus_handle = IIC_Bus_1->_iic_bus;
}

void example_dsi_resource_alloc_2(esp_lcd_dsi_bus_handle_t *mipi_dsi_bus, esp_lcd_panel_io_handle_t *mipi_dbi_io, esp_lcd_panel_handle_t *mipi_dpi_panel, void **frame_buffer)
{
    // create MIPI DSI bus first, it will initialize the DSI PHY as well
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = 2,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = 1000,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, mipi_dsi_bus));

    ESP_LOGI(TAG, "Install MIPI DSI LCD control IO");
    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // according to the LCD spec
        .lcd_param_bits = 8, // according to the LCD spec
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(*mipi_dsi_bus, &dbi_config, mipi_dbi_io));

    ESP_LOGI(TAG, "Install MIPI DSI LCD data panel");
    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = MIPI_DSI_DPI_CLK_MHZ,
        .in_color_format = LCD_COLOR_FMT_RGB888,
        .video_timing = {
            .h_size = CAMERA_WIDTH,
            .v_size = CAMERA_HEIGHT,
            .hsync_pulse_width = MIPI_DSI_LCD_HSYNC,
            .hsync_back_porch = MIPI_DSI_LCD_HBP,
            .hsync_front_porch = MIPI_DSI_LCD_HFP,
            .vsync_pulse_width = MIPI_DSI_LCD_VSYNC,
            .vsync_back_porch = MIPI_DSI_LCD_VBP,
            .vsync_front_porch = MIPI_DSI_LCD_VFP,
        },
        .flags = {
            .use_dma2d = true, // use DMA2D to copy draw buffer into frame buffer
        }};

    rm69a10_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = *mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t lcd_dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 24,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_rm69a10(*mipi_dbi_io, &lcd_dev_config, mipi_dpi_panel));

    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(*mipi_dpi_panel, 1, frame_buffer));
}

esp_err_t Camera_Init()
{
    esp_err_t ret = ESP_FAIL;

    esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
    esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

    void *frame_buffer = NULL;
    size_t frame_buffer_size = 0;

    //---------------DSI Init------------------//
    // example_dsi_resource_alloc(&mipi_dsi_bus, &mipi_dbi_io, &mipi_dpi_panel, &frame_buffer);
    example_dsi_resource_alloc_2(&mipi_dsi_bus, &mipi_dbi_io, &mipi_dpi_panel, &frame_buffer);

    // ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 1, &frame_buffer));

    //---------------Necessary variable config------------------//
    frame_buffer_size = CAMERA_WIDTH * CAMERA_HEIGHT * CAMERA_BITS_PER_PIXEL / 8;

    // frame_buffer = (uint8_t *)malloc(frame_buffer_size);

    ESP_LOGI(TAG, "frame_buffer_size: %zu", frame_buffer_size);
    ESP_LOGI(TAG, "frame_buffer: %p", frame_buffer);

    new_trans.buffer = frame_buffer;
    new_trans.buflen = frame_buffer_size;

    //--------Camera Sensor and SCCB Init-----------//
    i2c_master_bus_handle_t i2c_bus_handle = NULL;
    example_sensor_init_2(I2C_NUM_1, &i2c_bus_handle);

    //---------------CSI Init------------------//
    esp_cam_ctlr_csi_config_t csi_config = {
        .ctlr_id = 0,
        .h_res = CAMERA_WIDTH,
        .v_res = CAMERA_HEIGHT,
        .data_lane_num = 2,
        .lane_bit_rate_mbps = EXAMPLE_MIPI_CSI_LANE_BITRATE_MBPS,
        .input_data_color_type = CAM_CTLR_COLOR_RAW8,
        .output_data_color_type = CAM_CTLR_COLOR_RGB888,
        .queue_items = 1,
        .byte_swap_en = false,
    };
    ret = esp_cam_new_csi_ctlr(&csi_config, &cam_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "csi init fail[%d]", ret);
        return ret;
    }

    esp_cam_ctlr_evt_cbs_t cbs = {
        .on_get_new_trans = s_camera_get_new_vb,
        .on_trans_finished = s_camera_get_finished_trans,
    };
    if (esp_cam_ctlr_register_event_callbacks(cam_handle, &cbs, &new_trans) != ESP_OK)
    {
        ESP_LOGE(TAG, "ops register fail");
        return ret;
    }

    ESP_ERROR_CHECK(esp_cam_ctlr_enable(cam_handle));

    //---------------ISP Init------------------//
    isp_proc_handle_t isp_proc = NULL;
    esp_isp_processor_cfg_t isp_config = {
        .clk_hz = 160 * 1000 * 1000,
        .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
        .input_data_color_type = ISP_COLOR_RAW8,
        .output_data_color_type = ISP_COLOR_RGB888,
        .has_line_start_packet = false,
        .has_line_end_packet = false,
        .h_res = CAMERA_WIDTH,
        .v_res = CAMERA_HEIGHT,
    };
    ESP_ERROR_CHECK(esp_isp_new_processor(&isp_config, &isp_proc));

    // 自动对焦
    //  esp_isp_af_config_t af_config = {
    //      .edge_thresh = 128,
    //  };
    //  isp_af_ctlr_t af_ctrlr = NULL;
    //  ESP_ERROR_CHECK(esp_isp_new_af_controller(isp_proc, &af_config, &af_ctrlr));
    //  ESP_ERROR_CHECK(esp_isp_af_controller_enable(af_ctrlr));

    // // 自动白平衡
    // isp_awb_ctlr_t awb_ctlr = NULL;
    // /* AWB 配置，请参考 API 注释来调整参数 */
    // esp_isp_awb_config_t awb_config = {
    //     .sample_point = ISP_AWB_SAMPLE_POINT_AFTER_CCM,
    //     .window =
    //         {
    //             .top_left =
    //                 {
    //                     .x = CAMERA_WIDTH / 2 - 100,
    //                     .y = CAMERA_HEIGHT / 2 - 100,
    //                 },
    //             .btm_right =
    //                 {
    //                     .x = CAMERA_WIDTH / 2 + 100,
    //                     .y = CAMERA_HEIGHT / 2 + 100,
    //                 },
    //         },
    //     .white_patch =
    //         {
    //             .luminance =
    //                 {
    //                     .min = 0,
    //                     .max = 255 * 3,
    //                 },
    //             .red_green_ratio =
    //                 {
    //                     .min = 0,
    //                     .max = 3.0,
    //                 },
    //             .blue_green_ratio =
    //                 {
    //                     .min = 0,
    //                     .max = 3.0,
    //                 },
    //         }};
    // ESP_ERROR_CHECK(esp_isp_new_awb_controller(isp_proc, &awb_config, &awb_ctlr));
    // ESP_ERROR_CHECK(esp_isp_awb_controller_enable(awb_ctlr));

    // 自动曝光
    esp_isp_ae_config_t ae_config = {
        .sample_point = ISP_AE_SAMPLE_POINT_AFTER_DEMOSAIC,
        .window =
            {
                .top_left =
                    {
                        .x = CAMERA_WIDTH / 2 - 100,
                        .y = CAMERA_HEIGHT / 2 - 100,
                    },
                .btm_right =
                    {
                        .x = CAMERA_WIDTH / 2 + 100,
                        .y = CAMERA_HEIGHT / 2 + 100,
                    },
            },
    };
    isp_ae_ctlr_t ae_ctlr = NULL;
    ESP_ERROR_CHECK(esp_isp_new_ae_controller(isp_proc, &ae_config, &ae_ctlr));
    ESP_ERROR_CHECK(esp_isp_ae_controller_enable(ae_ctlr));

    // 色彩控制器
    esp_isp_color_config_t color_config = {
        .color_contrast = {
            .decimal = 100,
            .integer = 0,
        },
        .color_saturation = {
            .decimal = 90,
            .integer = 0,
        },
        .color_hue = 30,
        .color_brightness = 0,
    };
    ESP_ERROR_CHECK(esp_isp_color_configure(isp_proc, &color_config));
    ESP_ERROR_CHECK(esp_isp_color_enable(isp_proc));

    ESP_ERROR_CHECK(esp_isp_enable(isp_proc));

    // //---------------DPI Reset------------------//
    // example_dpi_panel_reset(mipi_dpi_panel);

    // init to all white
    // memset(frame_buffer, 0xFF, frame_buffer_size);
    // esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

    if (esp_cam_ctlr_start(cam_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Driver start fail");
        return ret;
    }

    // example_dpi_panel_init(mipi_dpi_panel);

    return ret;
}

static void example_bsp_init_refresh_monitor_io(void)
{
    gpio_config_t monitor_io_conf = {
        .pin_bit_mask = 1ULL << 20,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&monitor_io_conf));
}

void Screen_Init(void)
{
    // create MIPI DSI bus first, it will initialize the DSI PHY as well
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = 2,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = 1000,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

    ESP_LOGI(TAG, "Install MIPI DSI LCD control IO");
    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // according to the LCD spec
        .lcd_param_bits = 8, // according to the LCD spec
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io));

    ESP_LOGI(TAG, "Install MIPI DSI LCD data panel");
    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = MIPI_DSI_DPI_CLK_MHZ,
        .in_color_format = LCD_COLOR_FMT_RGB888,
        .video_timing = {
            .h_size = SCREEN_WIDTH,
            .v_size = SCREEN_HEIGHT,
            .hsync_pulse_width = MIPI_DSI_LCD_HSYNC,
            .hsync_back_porch = MIPI_DSI_LCD_HBP,
            .hsync_front_porch = MIPI_DSI_LCD_HFP,
            .vsync_pulse_width = MIPI_DSI_LCD_VSYNC,
            .vsync_back_porch = MIPI_DSI_LCD_VBP,
            .vsync_front_porch = MIPI_DSI_LCD_VFP,
        },
        .flags = {
            .use_dma2d = true, // use DMA2D to copy draw buffer into frame buffer
        }};

    rm69a10_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t lcd_dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 24,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_rm69a10(mipi_dbi_io, &lcd_dev_config, &mipi_dpi_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(mipi_dpi_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(mipi_dpi_panel));
}

static bool example_monitor_refresh_rate(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    static int io_level = 0;
    // please note, the real refresh rate should be 2*frequency of this GPIO toggling
    gpio_set_level((gpio_num_t)(20), io_level);
    io_level = !io_level;
    return false;
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1)
    {
        // esp_cam_ctlr_receive(cam_handle, &new_trans, ESP_CAM_CTLR_MAX_DELAY);

        // esp_lcd_panel_draw_bitmap(mipi_dpi_panel, 0, 0, 568 + 1, 1232 + 1, new_trans.buffer);

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

void Lvgl_Init()
{
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // create a lvgl display
    lv_display_t *display = lv_display_create(568, 1232);
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
    size_t draw_buffer_sz = 568 * 1232 / 10 * sizeof(lv_color_t);
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
        // #if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
        .on_refresh_done = example_monitor_refresh_rate,
        // #endif
    };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(mipi_dpi_panel, &cbs, display));

    ESP_LOGI(TAG, "Use esp_timer as LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2 * 1000));

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(example_lvgl_port_task, "LVGL", (4 * 1024), NULL, 2, NULL);

    // ESP_LOGI(TAG, "Display LVGL Meter Widget");
    // _lock_acquire(&lvgl_api_lock);
    // example_lvgl_demo_ui(display);
    // _lock_release(&lvgl_api_lock);
}

// 函数定义
bool crop_rgb888_image(const uint8_t *src, uint8_t *cropped, uint32_t src_width, uint32_t src_height, uint32_t x, uint32_t y, uint32_t crop_width, uint32_t crop_height)
{
    if (cropped == NULL)
    {
        printf("memory allocation failed\n");
        return false;
    }

    // 遍历裁剪区域
    for (uint32_t i = 0; i < crop_height; i++)
    {
        for (uint32_t j = 0; j < crop_width; j++)
        {
            // 计算原始图像中的像素位置
            uint32_t src_index = ((y + i) * src_width + (x + j)) * 3;
            // 计算裁剪图像中的像素位置
            uint32_t crop_index = (i * crop_width + j) * 3;
            // 复制像素数据
            cropped[crop_index] = src[src_index];         // 红色
            cropped[crop_index + 1] = src[src_index + 1]; // 绿色
            cropped[crop_index + 2] = src[src_index + 2]; // 蓝色
        }
    }

    return true;
}

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

    SGM38121->begin();
    printf("SGM38121 ID: %#X\n", SGM38121->iic_device_id());

    SGM38121->set_output_voltage(Cpp_Bus_Driver::SGM38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::SGM38121::Channel::AVDD_2, 2800);

    SGM38121->set_channel_state(Cpp_Bus_Driver::SGM38121::Channel::AVDD_1, Cpp_Bus_Driver::SGM38121::State::ON);
    SGM38121->set_channel_state(Cpp_Bus_Driver::SGM38121::Channel::AVDD_2, Cpp_Bus_Driver::SGM38121::State::ON);

    vTaskDelay(pdMS_TO_TICKS(1000));

    example_bsp_enable_dsi_phy_power();

    Camera_Init();
    Screen_Init();

    // example_bsp_init_refresh_monitor_io();

    // Lvgl_Init();

    // // 创建一个简单的 LVGL 界面
    // lv_obj_t *scr = lv_scr_act();
    // lv_obj_t *label = lv_label_create(scr);
    // lv_label_set_text(label, "Hello, LVGL with ESP32!");
    // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // // 刷新屏幕
    // lv_task_handler();

    while (1)
    {
        // esp_cam_ctlr_receive(cam_handle, &new_trans, ESP_CAM_CTLR_MAX_DELAY);

        if (new_trans_Flag == true)
        {

            // esp_cam_ctlr_trans_t trans;
            // esp_cam_ctlr_receive(cam_handle, &trans, ESP_CAM_CTLR_MAX_DELAY);

            // // 分配目标图像内存
            uint8_t *dst = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT * CAMERA_BITS_PER_PIXEL / 8);

            // rotate90((uint8_t *)trans.buffer, dst, CAMERA_WIDTH, CAMERA_HEIGHT);

            //         esp_lcd_panel_draw_bitmap(mipi_dpi_panel, 0, 0, CAMERA_HEIGHT, CAMERA_WIDTH, dst);

            // rotate90((uint8_t *)new_trans_2->buffer, dst, CAMERA_WIDTH, CAMERA_HEIGHT);

            // // esp_lcd_panel_draw_bitmap(mipi_dpi_panel, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, crop_rgb888_image(dst, CAMERA_HEIGHT, CAMERA_WIDTH, 10, 10, SCREEN_WIDTH, SCREEN_HEIGHT));

            crop_rgb888_image((uint8_t *)new_trans.buffer, dst, CAMERA_WIDTH, CAMERA_HEIGHT,
                              (CAMERA_WIDTH - SCREEN_WIDTH) / 2, 0, SCREEN_WIDTH, CAMERA_HEIGHT);

            esp_lcd_panel_draw_bitmap(mipi_dpi_panel, 0, 0, SCREEN_WIDTH, CAMERA_HEIGHT, dst);

            // esp_lcd_panel_draw_bitmap(mipi_dpi_panel, 0, 0, CAMERA_HEIGHT, CAMERA_WIDTH, dst);
            // // esp_lcd_panel_draw_bitmap(mipi_dpi_panel, 0, 0, 550, 1232, new_trans_2->buffer);

            free(dst);

            new_trans_Flag = false;
        }

        // if ((esp_timer_get_time() / 1000) > CycleTime)
        // {
        //     if (new_trans.buflen > 0)
        //     {
        //         // 打印接收到的数据
        //         for (size_t i = 0; i < new_trans.buflen; i++)
        //         {
        //             printf("data: %#X\n", ((uint8_t *)new_trans.buffer)[i]);
        //         }
        //     }
        //     CycleTime = esp_timer_get_time() + 1000;
        // }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
