/*
 * @Description: t_display_p4_driver
 * @Author: LILYGO_L
 * @Date: 2025-07-07 14:31:29
 * @LastEditTime: 2025-08-15 15:00:40
 * @License: GPL 3.0
 */
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "cpp_bus_driver_library.h"
#include "t_display_p4_driver.h"
#include "esp_ldo_regulator.h"

bool Mipi_Dsi_Init(uint8_t num_data_lanes, uint32_t lane_bit_rate_mbps, uint32_t dpi_clock_freq_mhz, lcd_color_rgb_pixel_format_t color_rgb_pixel_format, uint8_t num_fbs, uint32_t width, uint32_t height,
                   uint32_t mipi_dsi_hsync, uint32_t mipi_dsi_hbp, uint32_t mipi_dsi_hfp, uint32_t mipi_dsi_vsync, uint32_t mipi_dsi_vbp, uint32_t mipi_dsi_vfp,
                   uint32_t bits_per_pixel, esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    esp_lcd_panel_io_handle_t mipi_dbi_io;

    auto cpp_assert = std::make_unique<Cpp_Bus_Driver::Tool>();

    // create MIPI DSI bus first, it will initialize the DSI PHY as well
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = num_data_lanes,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = lane_bit_rate_mbps,
    };

    esp_err_t assert = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_dsi_bus fail (error code: %#X)\n", assert);
        return false;
    }

    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t dbi_io_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // according to the LCD spec
        .lcd_param_bits = 8, // according to the LCD spec
    };
    assert = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_io_config, &mipi_dbi_io);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_panel_io_dbi fail (error code: %#X)\n", assert);
        return false;
    }

    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = dpi_clock_freq_mhz,
        .pixel_format = color_rgb_pixel_format,
        .num_fbs = num_fbs,
        .video_timing = {
            .h_size = width,
            .v_size = height,
            .hsync_pulse_width = mipi_dsi_hsync,
            .hsync_back_porch = mipi_dsi_hbp,
            .hsync_front_porch = mipi_dsi_hfp,
            .vsync_pulse_width = mipi_dsi_vsync,
            .vsync_back_porch = mipi_dsi_vbp,
            .vsync_front_porch = mipi_dsi_vfp,
        },
        .flags = {
            .use_dma2d = true, // use DMA2D to copy draw buffer into frame buffer
        }};

#if defined CONFIG_SCREEN_TYPE_HI8561
    hi8561_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = bits_per_pixel,
        .vendor_config = &vendor_config,
    };
    assert = esp_lcd_new_panel_hi8561(mipi_dbi_io, &dev_config, mipi_dpi_panel);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_panel_hi8561 fail (error code: %#X)\n", assert);
        return false;
    }
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    rm69a10_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = bits_per_pixel,
        .vendor_config = &vendor_config,
    };
    assert = esp_lcd_new_panel_rm69a10(mipi_dbi_io, &dev_config, mipi_dpi_panel);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_panel_rm69a10 fail (error code: %#X)\n", assert);
        return false;
    }
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

    return true;
}

bool Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    if (Mipi_Dsi_Init(SCREEN_DATA_LANE_NUM, SCREEN_LANE_BIT_RATE_MBPS, SCREEN_MIPI_DSI_DPI_CLK_MHZ, SCREEN_COLOR_RGB_PIXEL_FORMAT,
                      0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_MIPI_DSI_HSYNC, SCREEN_MIPI_DSI_HBP,
                      SCREEN_MIPI_DSI_HFP, SCREEN_MIPI_DSI_VSYNC, SCREEN_MIPI_DSI_VBP, SCREEN_MIPI_DSI_VFP,
                      SCREEN_BITS_PER_PIXEL, mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    return true;
}

bool Camera_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    if (Mipi_Dsi_Init(CAMERA_DATA_LANE_NUM, CAMERA_LANE_BIT_RATE_MBPS, CAMERA_MIPI_DSI_DPI_CLK_MHZ, CAMERA_COLOR_RGB_PIXEL_FORMAT,
                      CONFIG_EXAMPLE_CAM_BUF_COUNT, CAMERA_WIDTH, CAMERA_HEIGHT, 0, 0, 0, 0, 0, 0, CAMERA_BITS_PER_PIXEL, mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    return true;
}

bool Init_Ldo_Channel_Power(uint8_t chan_id, uint32_t voltage_mv)
{
    esp_ldo_channel_handle_t ldo_channel_handle = NULL;
    esp_ldo_channel_config_t ldo_channel_config =
        {
            .chan_id = static_cast<int>(chan_id),
            .voltage_mv = static_cast<int>(voltage_mv),
        };
    if (esp_ldo_acquire_channel(&ldo_channel_config, &ldo_channel_handle) != ESP_OK)
    {
        printf("esp_ldo_acquire_channel %d fail\n", chan_id);
        return false;
    }

    return true;
}
