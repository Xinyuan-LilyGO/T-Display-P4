/*
 * @Description: screen_camera
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:45:00
 * @LastEditTime: 2026-03-24 17:44:13
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"
#include "app_video.h"
#include "driver/ppa.h"
#include "esp_private/esp_cache_private.h"
#include "esp_video_init.h"

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

ppa_client_handle_t ppa_srm_handle = NULL;
size_t data_cache_line_size = 0;
int32_t video_cam_fd0;

int32_t fps_count;
int64_t start_time;

bool Screen_Init_flag = false;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Sgm38121_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);
auto Screen_Mipi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Mipi>(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_MIPI_DSI_HSYNC, SCREEN_MIPI_DSI_HBP, SCREEN_MIPI_DSI_HFP,
                                                                       SCREEN_MIPI_DSI_VSYNC, SCREEN_MIPI_DSI_VBP, SCREEN_MIPI_DSI_VFP, SCREEN_DATA_LANE_NUM,
                                                                       [](uint8_t format) -> Cpp_Bus_Driver::Hardware_Mipi::Color_Format
                                                                       {
                                                                    switch (format)
                                                                    {
                                                                    case 16:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB565;
                                                                    case 24:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB888;
                                                                    default:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB565;
                                                                    } }(SCREEN_BITS_PER_PIXEL));

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Sgm38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(Sgm38121_Iic_Bus, SGM38121_IIC_ADDRESS);
#if defined CONFIG_SCREEN_TYPE_HI8561
auto Screen = std::make_unique<Cpp_Bus_Driver::Hi8561>(Screen_Mipi_Bus);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
auto Screen = std::make_unique<Cpp_Bus_Driver::Rm69a10>(Screen_Mipi_Bus);
#else
#error "no macro definition is set"
#endif
auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves,
                                  size_t camera_buf_len)
{
    fps_count++;
    if (fps_count == 50)
    {
        int64_t end_time = esp_timer_get_time();
        printf("fps: %f\n", 1000000.0 / ((end_time - start_time) / 50.0));
        start_time = end_time;
        fps_count = 0;

        printf("camera_buf_hes: %lu, camera_buf_ves: %lu, camera_buf_len: %d KB\n", camera_buf_hes, camera_buf_ves, camera_buf_len / 1024);
    }

    if (Screen_Init_flag == false)
    {
        return;
    }

    uint32_t input_img_block_width = (camera_buf_hes - SCREEN_WIDTH) / 2;
    uint32_t input_img_block_height = 0;
    uint32_t input_img_width = SCREEN_WIDTH;
    uint32_t input_img_height = camera_buf_ves;

    uint32_t output_img_width = input_img_width;
    uint32_t output_img_height = input_img_height;

    size_t output_buffer_size = output_img_width * output_img_height * (SCREEN_BITS_PER_PIXEL / 8);

    auto output_buffer = std::unique_ptr<uint8_t[], std::function<void(uint8_t *)>>(
        (uint8_t *)heap_caps_aligned_calloc(data_cache_line_size, 1, output_buffer_size, MALLOC_CAP_SPIRAM),
        [](uint8_t *p)
        { heap_caps_free(p); });
    if (output_buffer == NULL)
    {
        printf("heap_caps_malloc fail\n");
        return;
    }

    ppa_srm_oper_config_t srm_config =
        {
            .in =
                {
                    .buffer = camera_buf,
                    .pic_w = camera_buf_hes,
                    .pic_h = camera_buf_ves,
                    .block_w = input_img_width,
                    .block_h = input_img_height,
                    .block_offset_x = input_img_block_width,
                    .block_offset_y = input_img_block_height,
#if (defined CONFIG_CAMERA_TYPE_SC2336) || (defined CONFIG_CAMERA_TYPE_OV2710)
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB888,
#else
#error "no macro definition is set"
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#else
#error "no macro definition is set"
#endif
                },

            .out =
                {
                    .buffer = output_buffer.get(),
                    .buffer_size = ALIGN_UP(output_buffer_size, data_cache_line_size),
                    .pic_w = output_img_width,
                    .pic_h = output_img_height,
                    .block_offset_x = 0,
                    .block_offset_y = 0,
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB888,
#else
#error "no macro definition is set"
#endif
                },

            .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
            .scale_x = 1,
            .scale_y = 1,
            .mirror_x = false,
#if SCREEN_ROTATION_DIRECTION == 0
#if defined CONFIG_SCREEN_TYPE_HI8561
            .mirror_y = true,
#elif defined CONFIG_SCREEN_TYPE_RM69A10
            .mirror_y = false,
#else
#error "no macro definition is set"
#endif
#elif if SCREEN_ROTATION_DIRECTION == 90
            .mirror_y = false,
#else
#error "no macro definition is set"
#endif
            .rgb_swap = false,
            .byte_swap = false,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };

    esp_err_t assert = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
    if (assert != ESP_OK)
    {
        printf("ppa_do_scale_rotate_mirror fail (error code: %#X)\n", assert);
        return;
    }

    if (Screen->send_color_stream_coordinate(0, (SCREEN_HEIGHT - output_img_height) / 2,
                                             output_img_width, output_img_height + ((SCREEN_HEIGHT - output_img_height) / 2),
                                             output_buffer.get()) == false)
    {
        printf("send_color_stream_coordinate fail\n");
        return;
    }
}

bool App_Video_Init()
{
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

    esp_video_init_csi_config_t csi_config =
        {
            .sccb_config = {
                .init_sccb = false,
                .i2c_handle = Sgm38121_Iic_Bus->get_bus_handle(),
                .freq = static_cast<uint32_t>(100000),
            },
            .reset_pin = gpio_num_t ::GPIO_NUM_NC,
            .pwdn_pin = gpio_num_t ::GPIO_NUM_NC,

            .dont_init_ldo = true,
        };

    esp_video_init_config_t cam_config =
        {
            .csi = &csi_config,
        };

    assert = esp_video_init(&cam_config);
    if (assert != ESP_OK)
    {
        printf("esp_video_init fail (error code: %#X)\n", assert);
        return false;
    }

#if (defined CONFIG_CAMERA_TYPE_SC2336) || (defined CONFIG_CAMERA_TYPE_OV2710)
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB565);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB888);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#else
#error "no macro definition is set"
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB565);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#else
#error "no macro definition is set"
#endif

    assert = app_video_set_bufs(video_cam_fd0, CAMERA_BUFFER_COUNT, NULL);
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

    assert = app_video_stream_task_start(video_cam_fd0, 0);
    if (assert != ESP_OK)
    {

        printf("app_video_stream_task_start fail (error code: %#X)\n", assert);
        return false;
    }

    // app_video_stream_task_stop(video_cam_fd0);

    return true;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    Xl9535->begin();

    Xl9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));

    Sgm38121->begin();
#if defined CONFIG_CAMERA_TYPE_SC2336
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV2710
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1700);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3000);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV5645
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "no macro definition is set"
#endif

#if defined CONFIG_SCREEN_TYPE_HI8561
    Esp32p4->create_pwm(HI8561_SCREEN_BL, ledc_timer_t::LEDC_TIMER_0, ledc_channel_t::LEDC_CHANNEL_0, 2000);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#else
#error "no macro definition is set"
#endif

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(3, 2500);

    vTaskDelay(pdMS_TO_TICKS(100));

    if (App_Video_Init() == false)
    {
        printf("App_Video_Init fail\n");
    }

    Xl9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(5));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(120));

    if (Screen->begin(SCREEN_MIPI_DSI_DPI_CLK_MHZ, SCREEN_LANE_BIT_RATE_MBPS) == false)
    {
        Screen_Init_flag = false;
        printf("Screen->begin fail\n");
    }
    else
    {
        Screen_Init_flag = true;
        printf("Screen->begin success\n");
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

    //     if (Screen->send_color_stream_coordinate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, white_buf) == false)
    //     {
    //         printf("send_color_stream_coordinate white fail\n");
    //     }
    //     heap_caps_free(white_buf);
    // }

#if defined CONFIG_SCREEN_TYPE_HI8561
    Esp32p4->start_pwm_gradient_time(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        Screen->set_brightness(i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "no macro definition is set"
#endif

    // assert = app_video_stream_task_restart(video_cam_fd0);
    // if (assert != ESP_OK)
    // {
    //     printf("app_video_stream_task_restart fail (error code: %#X)\n", assert);
    // }
    // else
    // {
    //     // Get the initial time for frame rate statistics
    //     start_time = esp_timer_get_time();
    // }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
