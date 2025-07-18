#pragma once

#include "t_display_p4_config.h"
#include "hi8561_driver.h"
#include "rm69a10_driver.h"

#if CONFIG_LCD_PIXEL_FORMAT_RGB565
#define SCREEN_BITS_PER_PIXEL 16
#define CAMERA_BITS_PER_PIXEL 16
#define SCREEN_COLOR_RGB_PIXEL_FORMAT LCD_COLOR_PIXEL_FORMAT_RGB565
#define CAMERA_COLOR_RGB_PIXEL_FORMAT LCD_COLOR_PIXEL_FORMAT_RGB565
#define LVGL_COLOR_FORMAT LV_COLOR_FORMAT_RGB565
#elif CONFIG_LCD_PIXEL_FORMAT_RGB888
#define SCREEN_BITS_PER_PIXEL 24
#define SCREEN_COLOR_RGB_PIXEL_FORMAT LCD_COLOR_PIXEL_FORMAT_RGB888
#define CAMERA_BITS_PER_PIXEL 24
#define CAMERA_COLOR_RGB_PIXEL_FORMAT LCD_COLOR_PIXEL_FORMAT_RGB888
#define LVGL_COLOR_FORMAT LV_COLOR_FORMAT_RGB888
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

// SCREEN
#if defined CONFIG_SCREEN_TYPE_HI8561
#define SCREEN_WIDTH HI8561_SCREEN_WIDTH
#define SCREEN_HEIGHT HI8561_SCREEN_HEIGHT
#define SCREEN_MIPI_DSI_DPI_CLK_MHZ HI8561_SCREEN_MIPI_DSI_DPI_CLK_MHZ
#define SCREEN_MIPI_DSI_HSYNC HI8561_SCREEN_MIPI_DSI_HSYNC
#define SCREEN_MIPI_DSI_HBP HI8561_SCREEN_MIPI_DSI_HBP
#define SCREEN_MIPI_DSI_HFP HI8561_SCREEN_MIPI_DSI_HFP
#define SCREEN_MIPI_DSI_VSYNC HI8561_SCREEN_MIPI_DSI_VSYNC
#define SCREEN_MIPI_DSI_VBP HI8561_SCREEN_MIPI_DSI_VBP
#define SCREEN_MIPI_DSI_VFP HI8561_SCREEN_MIPI_DSI_VFP
#define SCREEN_DATA_LANE_NUM HI8561_SCREEN_DATA_LANE_NUM
#define SCREEN_LANE_BIT_RATE_MBPS HI8561_SCREEN_LANE_BIT_RATE_MBPS

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#define SCREEN_WIDTH RM69A10_SCREEN_WIDTH
#define SCREEN_HEIGHT RM69A10_SCREEN_HEIGHT
#define SCREEN_MIPI_DSI_DPI_CLK_MHZ RM69A10_SCREEN_MIPI_DSI_DPI_CLK_MHZ
#define SCREEN_MIPI_DSI_HSYNC RM69A10_SCREEN_MIPI_DSI_HSYNC
#define SCREEN_MIPI_DSI_HBP RM69A10_SCREEN_MIPI_DSI_HBP
#define SCREEN_MIPI_DSI_HFP RM69A10_SCREEN_MIPI_DSI_HFP
#define SCREEN_MIPI_DSI_VSYNC RM69A10_SCREEN_MIPI_DSI_VSYNC
#define SCREEN_MIPI_DSI_VBP RM69A10_SCREEN_MIPI_DSI_VBP
#define SCREEN_MIPI_DSI_VFP RM69A10_SCREEN_MIPI_DSI_VFP
#define SCREEN_DATA_LANE_NUM RM69A10_SCREEN_DATA_LANE_NUM
#define SCREEN_LANE_BIT_RATE_MBPS RM69A10_SCREEN_LANE_BIT_RATE_MBPS

#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

bool Mipi_Dsi_Init(uint8_t num_data_lanes, uint32_t lane_bit_rate_mbps, uint32_t dpi_clock_freq_mhz, lcd_color_rgb_pixel_format_t color_rgb_pixel_format, uint8_t num_fbs, uint32_t width, uint32_t height,
                   uint32_t mipi_dsi_hsync, uint32_t mipi_dsi_hbp, uint32_t mipi_dsi_hfp, uint32_t mipi_dsi_vsync, uint32_t mipi_dsi_vbp, uint32_t mipi_dsi_vfp,
                   uint32_t bits_per_pixel, esp_lcd_panel_handle_t *mipi_dpi_panel);

bool Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel);

bool Camera_Init(esp_lcd_panel_handle_t *mipi_dpi_panel);
