/*
 * @Description: hi8561_camera
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:45:00
 * @LastEditTime: 2025-06-13 12:01:55
 * @License: GPL 3.0
 */
#include "esp_video_init.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_cache_private.h"
#include "esp_timer.h"
#include "driver/ppa.h"
#include "esp_ldo_regulator.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "hi8561_driver.h"
#include "app_video.h"

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

ppa_client_handle_t ppa_srm_handle = NULL;
size_t data_cache_line_size = 0;
void *lcd_buffer[CONFIG_EXAMPLE_CAM_BUF_COUNT];

int32_t fps_count;
int64_t start_time;

esp_lcd_panel_handle_t screen_mipi_dpi_panel = NULL;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto SGM38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(IIC_Bus_1, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

void bsp_enable_dsi_phy_power(void)
{
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config =
        {
            .chan_id = 3,
            .voltage_mv = 2500,
        };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    printf("mipi dsi phy powered on\n");
}

bool Mipi_Dsi_Init(uint8_t num_data_lanes, uint32_t lane_bit_rate_mbps, uint32_t dpi_clock_freq_mhz, lcd_color_format_t color_format, uint8_t num_fbs, uint32_t width, uint32_t height,
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
        .in_color_format = color_format,
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

    return true;
}

bool Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    if (Mipi_Dsi_Init(HI8561_SCREEN_DATA_LANE_NUM, HI8561_SCREEN_LANE_BIT_RATE_MBPS, HI8561_SCREEN_MIPI_DSI_DPI_CLK_MHZ, LCD_COLOR_FMT_RGB565,
                      0, HI8561_SCREEN_WIDTH, HI8561_SCREEN_HEIGHT, HI8561_SCREEN_MIPI_DSI_HSYNC, HI8561_SCREEN_MIPI_DSI_HBP,
                      HI8561_SCREEN_MIPI_DSI_HFP, HI8561_SCREEN_MIPI_DSI_VSYNC, HI8561_SCREEN_MIPI_DSI_VBP, HI8561_SCREEN_MIPI_DSI_VFP,
                      HI8561_SCREEN_BITS_PER_PIXEL_RGB565, mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    return true;
}

void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves,
                                  size_t camera_buf_len, void *user_data)
{
    fps_count++;
    if (fps_count == 50)
    {
        int64_t end_time = esp_timer_get_time();
        printf("fps: %f\n", 1000000.0 / ((end_time - start_time) / 50.0));
        start_time = end_time;
        fps_count = 0;

        printf("camera_buf_hes: %lu, camera_buf_ves: %lu, camera_buf_len: %d KB\n", camera_buf_hes, camera_buf_ves, camera_buf_len / 1024);

        for (size_t i = 0; i < 10; i++)
        {
            printf("camera_buf: 0x%02x\n", camera_buf[i]);
        }
    }

    ppa_srm_oper_config_t srm_config =
        {
            .in =
                {
                    .buffer = camera_buf,
                    .pic_w = camera_buf_hes,
                    .pic_h = camera_buf_ves,
                    .block_w = camera_buf_hes,
                    .block_h = camera_buf_ves,
                    .block_offset_x = (camera_buf_hes > HI8561_SCREEN_WIDTH) ? (camera_buf_hes - HI8561_SCREEN_WIDTH) / 2 : 0,
                    .block_offset_y = (camera_buf_ves > HI8561_SCREEN_HEIGHT) ? (camera_buf_ves - HI8561_SCREEN_HEIGHT) / 2 : 0,
                    .srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,
                },

            .out =
                {
                    .buffer = lcd_buffer[camera_buf_index],
                    .buffer_size = ALIGN_UP(HI8561_SCREEN_WIDTH * HI8561_SCREEN_HEIGHT * (APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? 2 : 3), data_cache_line_size),
                    .pic_w = HI8561_SCREEN_WIDTH,
                    .pic_h = HI8561_SCREEN_HEIGHT,
                    .block_offset_x = 0,
                    .block_offset_y = 0,
                    .srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,
                },

            .rotation_angle = PPA_SRM_ROTATION_ANGLE_180,
            .scale_x = 1,
            .scale_y = 1,
            .mirror_x = false,
            .mirror_y = false,
            .rgb_swap = false,
            .byte_swap = false,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };

    if (camera_buf_hes > HI8561_SCREEN_WIDTH || camera_buf_ves > HI8561_SCREEN_HEIGHT)
    {
        // The resolution of the camera does not match the LCD resolution. Image processing can be done using PPA, but there will be some frame rate loss

        srm_config.in.block_w = (camera_buf_hes > HI8561_SCREEN_WIDTH) ? HI8561_SCREEN_WIDTH : camera_buf_hes;
        srm_config.in.block_h = (camera_buf_ves > HI8561_SCREEN_HEIGHT) ? HI8561_SCREEN_HEIGHT : camera_buf_ves;

        esp_err_t assert = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
        if (assert != ESP_OK)
        {
            printf("ppa_do_scale_rotate_mirror fail (error code: %#X)\n", assert);
        }

        assert = esp_lcd_panel_draw_bitmap(screen_mipi_dpi_panel, 0, (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 + 120,
                                           srm_config.in.block_w, srm_config.in.block_h + (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 - 120, lcd_buffer[camera_buf_index]);
        if (assert != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
        }
    }
    else
    {
        esp_err_t assert = esp_lcd_panel_draw_bitmap(screen_mipi_dpi_panel, 0, 0, camera_buf_hes, camera_buf_ves, camera_buf);
        if (assert != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
        }
    }
}

bool App_Video_Init()
{
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

    if (Mipi_Dsi_Init(CAMERA_DATA_LANE_NUM, CAMERA_LANE_BIT_RATE_MBPS, CAMERA_MIPI_DSI_DPI_CLK_MHZ, LCD_COLOR_FMT_RGB565,
                      CONFIG_EXAMPLE_CAM_BUF_COUNT, CAMERA_WIDTH, CAMERA_HEIGHT, 0, 0, 0, 0, 0, 0, CAMERA_BITS_PER_PIXEL_RGB565, &mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    ppa_client_config_t ppa_srm_config =
        {
            .oper_type = PPA_OPERATION_SRM,
        };
    esp_err_t assert = ppa_register_client(&ppa_srm_config, &ppa_srm_handle);
    if (assert != ESP_OK)
    {
        printf("ppa_register_client fail (error code: %#X)\n", assert);
        return false;
    }
    assert = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &data_cache_line_size);
    if (assert != ESP_OK)
    {
        printf("esp_cache_get_alignment fail (error code: %#X)\n", assert);
        return false;
    }

    assert = app_video_main(IIC_Bus_1->_iic_bus_handle);
    if (assert != ESP_OK)
    {
        printf("video_init fail (error code: %#X)\n", assert);
        return false;
    }

    int32_t video_cam_fd0 = app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }

#if CONFIG_EXAMPLE_CAM_BUF_COUNT == 2
    assert = esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 2, &lcd_buffer[0], &lcd_buffer[1]);
#else
    assert = esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 3, &lcd_buffer[0], &lcd_buffer[1], &lcd_buffer[2]);
#endif
    if (assert != ESP_OK)
    {
        printf("esp_lcd_dpi_panel_get_frame_buffer fail (error code: %#X)\n", assert);
        return false;
    }

    // #if CONFIG_EXAMPLE_USE_MEMORY_MAPPING
    //     ESP_LOGI(TAG, "Using map buffer");
    //     // When setting the camera video buffer, it can be written as NULL to automatically allocate the buffer using mapping
    //     assert = app_video_set_bufs(app_video_set_bufs(video_cam_fd0, EXAMPLE_CAM_BUF_NUM, NULL));
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #elif CONFIG_CAMERA_CAMERA_MIPI_RAW8_1280X720_30FPS
    //     printf("using user defined buffer\n");
    //     assert = app_video_set_bufs(video_cam_fd0, CONFIG_EXAMPLE_CAM_BUF_COUNT, (const void **)lcd_buffer);
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #else
    //     void *camera_buf[EXAMPLE_CAM_BUF_NUM];
    //     for (int i = 0; i < EXAMPLE_CAM_BUF_NUM; i++)
    //     {
    //         camera_buf[i] = heap_caps_aligned_calloc(data_cache_line_size, 1, app_video_get_buf_size(), MALLOC_CAP_SPIRAM);
    //     }
    //     assert = app_video_set_bufs(video_cam_fd0, EXAMPLE_CAM_BUF_NUM, (const void **)camera_buf);
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #endif

    assert = app_video_set_bufs(video_cam_fd0, CONFIG_EXAMPLE_CAM_BUF_COUNT, (const void **)lcd_buffer);
    if (assert != ESP_OK)
    {
        printf("app_video_set_bufs fail (error code: %#X)\n", assert);
        return false;
    }

    assert = app_video_register_frame_operation_cb(camera_video_frame_operation);
    if (assert != ESP_OK)
    {

        printf("app_video_register_frame_operation_cb fail (error code: %#X)\n", assert);
        return false;
    }

    assert = app_video_stream_task_start(video_cam_fd0, 0, NULL);
    if (assert != ESP_OK)
    {

        printf("app_video_stream_task_start fail (error code: %#X)\n", assert);
        return false;
    }

    // Get the initial time for frame rate statistics
    start_time = esp_timer_get_time();

    return true;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    SGM38121->begin();
#if defined CONFIG_CAMERA_TYPE_SC2336
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV2710
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3100);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

    ESP32P4->create_pwm(HI8561_SCREEN_BL, ledc_channel_t::LEDC_CHANNEL_0, 2000);

    vTaskDelay(pdMS_TO_TICKS(100));

    bsp_enable_dsi_phy_power();

    if (App_Video_Init() == false)
    {
        printf("App_Video_Init fail\n");
    }

    Screen_Init(&screen_mipi_dpi_panel);

    esp_err_t assert = esp_lcd_panel_reset(screen_mipi_dpi_panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_reset fail (error code: %#X)\n", assert);
    }
    assert = esp_lcd_panel_init(screen_mipi_dpi_panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_init fail (error code: %#X)\n", assert);
    }

    ESP32P4->start_pwm_gradient_time(100, 500);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
