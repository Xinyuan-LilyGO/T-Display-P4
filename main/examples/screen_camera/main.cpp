/*
 * @Description: screen_camera
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:45:00
 * @LastEditTime: 2025-07-28 10:19:53
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
#include "t_display_p4_driver.h"
#include "app_video.h"

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

ppa_client_handle_t ppa_srm_handle = NULL;
size_t data_cache_line_size = 0;
void *lcd_buffer[CONFIG_EXAMPLE_CAM_BUF_COUNT];
int32_t video_cam_fd0;

int32_t fps_count;
int64_t start_time;

esp_lcd_panel_handle_t Screen_Mipi_Dpi_Panel = NULL;

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
            .voltage_mv = 1800,
        };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    printf("mipi dsi phy powered on\n");
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
                    .block_offset_x = (camera_buf_hes > SCREEN_WIDTH) ? (camera_buf_hes - SCREEN_WIDTH) / 2 : 0,
                    .block_offset_y = (camera_buf_ves > SCREEN_HEIGHT) ? (camera_buf_ves - SCREEN_HEIGHT) / 2 : 0,
                    .srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,
                },

            .out =
                {
                    .buffer = lcd_buffer[camera_buf_index],
                    .buffer_size = ALIGN_UP(SCREEN_WIDTH * SCREEN_HEIGHT * (APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? 2 : 3), data_cache_line_size),
                    .pic_w = SCREEN_WIDTH,
                    .pic_h = SCREEN_HEIGHT,
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

    if (camera_buf_hes > SCREEN_WIDTH || camera_buf_ves > SCREEN_HEIGHT)
    {
        // The resolution of the camera does not match the LCD resolution. Image processing can be done using PPA, but there will be some frame rate loss

        srm_config.in.block_w = (camera_buf_hes > SCREEN_WIDTH) ? SCREEN_WIDTH : camera_buf_hes;
        srm_config.in.block_h = (camera_buf_ves > SCREEN_HEIGHT) ? SCREEN_HEIGHT : camera_buf_ves;

        esp_err_t assert = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
        if (assert != ESP_OK)
        {
            printf("ppa_do_scale_rotate_mirror fail (error code: %#X)\n", assert);
        }

#if defined CONFIG_CAMERA_TYPE_SC2336 || defined CONFIG_CAMERA_TYPE_OV2710
        assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, (SCREEN_HEIGHT - srm_config.in.block_h) / 2 + 130,
                                           srm_config.in.block_w, srm_config.in.block_h + (SCREEN_HEIGHT - srm_config.in.block_h) / 2 - 130, lcd_buffer[camera_buf_index]);
#elif defined CONFIG_CAMERA_TYPE_OV5645
        assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, (SCREEN_HEIGHT - srm_config.in.block_h) / 2 + 150,
                                           srm_config.in.block_w, srm_config.in.block_h + (SCREEN_HEIGHT - srm_config.in.block_h) / 2 - 150, lcd_buffer[camera_buf_index]);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

        if (assert != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
        }
    }
    else
    {
        esp_err_t assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, 0, camera_buf_hes, camera_buf_ves, camera_buf);
        if (assert != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
        }
    }
}

bool App_Video_Init()
{
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

    if (Camera_Init(&mipi_dpi_panel) == false)
    {
        printf("Camera_Init fail\n");
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

    video_cam_fd0 = app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT);
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

    app_video_stream_task_stop(video_cam_fd0);

    return true;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    XL9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    // 开关3.3v电压时候必须先将GPS断电
    XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    // 开关3.3v电压时候必须先将ESP32C6断电
    XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));

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
#elif defined CONFIG_CAMERA_TYPE_OV5645
    XL9535->pin_mode(XL9535_CAMERA_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_CAMERA_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH); // 打开摄像头
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 2800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

#if defined CONFIG_SCREEN_TYPE_HI8561
    ESP32P4->create_pwm(HI8561_SCREEN_BL, ledc_channel_t::LEDC_CHANNEL_0, 2000);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

    bsp_enable_dsi_phy_power();

    if (App_Video_Init() == false)
    {
        printf("App_Video_Init fail\n");
    }

    Screen_Init(&Screen_Mipi_Dpi_Panel);

    esp_err_t assert = esp_lcd_panel_reset(Screen_Mipi_Dpi_Panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_reset fail (error code: %#X)\n", assert);
    }
    assert = esp_lcd_panel_init(Screen_Mipi_Dpi_Panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_init fail (error code: %#X)\n", assert);
    }

    // // 设置整个屏幕为白色
    // size_t screen_size = SCREEN_WIDTH * SCREEN_HEIGHT * 2; // RGB565: 2 bytes per pixel
    // size_t data_cache_line_size = 16;                      // 通常16或32，具体可查芯片手册
    // void *white_buf = heap_caps_aligned_calloc(data_cache_line_size, 1, screen_size, MALLOC_CAP_SPIRAM);
    // if (white_buf)
    // {
    //     uint16_t *p = (uint16_t *)white_buf;
    //     for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i)
    //     {
    //         p[i] = 0xFFFF; // RGB565白色
    //     }
    //     esp_err_t assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, white_buf);
    //     if (assert != ESP_OK)
    //     {
    //         printf("esp_lcd_panel_draw_bitmap white fail (error code: %#X)\n", assert);
    //     }
    //     heap_caps_free(white_buf);
    // }

#if defined CONFIG_SCREEN_TYPE_HI8561
    ESP32P4->start_pwm_gradient_time(100, 500);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        set_rm69a10_brightness(Screen_Mipi_Dpi_Panel, i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

    assert = app_video_stream_task_restart(video_cam_fd0);
    if (assert != ESP_OK)
    {
        printf("app_video_stream_task_restart fail (error code: %#X)\n", assert);
    }
    else
    {
        // Get the initial time for frame rate statistics
        start_time = esp_timer_get_time();
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
