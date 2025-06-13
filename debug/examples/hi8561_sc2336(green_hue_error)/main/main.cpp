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
#include "esp_timer.h"
#include "pin_config.h"
#include "cpp_bus_driver_library.h"
#include "hi8561_driver.h"

extern "C"
{
#include "esp_sccb_intf.h"
#include "esp_sccb_i2c.h"
#include "esp_cam_sensor.h"
#include "esp_cam_sensor_detect.h"
}

size_t CycleTime = 0;

volatile bool Cam_Trans_Flag = false;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto SGM38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(IIC_Bus_1, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

void rotate90(uint8_t *src, uint8_t *dst, int width, int height)
{
    int newWidth = height;
    int newHeight = width;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int srcIndex = (y * width + x) * 3;                      // 源图像中的像素索引
            int dstIndex = (x * newWidth + (newHeight - y - 1)) * 3; // 目标图像中的像素索引

            // 复制RGB值
            dst[dstIndex] = src[srcIndex];         // R
            dst[dstIndex + 1] = src[srcIndex + 1]; // G
            dst[dstIndex + 2] = src[srcIndex + 2]; // B
        }
    }
}

void example_bsp_enable_dsi_phy_power(void)
{
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    printf("mipi dsi phy powered on\n");
}

bool SC2336_Sensor_Init(std::shared_ptr<Cpp_Bus_Driver::Hardware_Iic_1> bus)
{
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
        sccb_i2c_config_t i2c_config =
            {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = p->sccb_addr,
                .scl_speed_hz = static_cast<uint32_t>(bus->_freq_hz),
            };
        ESP_ERROR_CHECK(sccb_new_i2c_io(bus->_iic_bus, &i2c_config, &cam_config.sccb_handle));

        cam = (*(p->detect))(&cam_config);
        if (cam)
        {
            if (p->port != ESP_CAM_SENSOR_MIPI_CSI)
            {
                printf("detect a camera sensor with mismatched interface\n");
                return false;
            }
            break;
        }
        ESP_ERROR_CHECK(esp_sccb_del_i2c_io(cam_config.sccb_handle));
    }

    if (!cam)
    {
        printf("failed to detect camera sensor\n");
        return false;
    }

    esp_cam_sensor_format_array_t cam_fmt_array = {0};
    esp_cam_sensor_query_format(cam, &cam_fmt_array);
    const esp_cam_sensor_format_t *parray = cam_fmt_array.format_array;
    for (int i = 0; i < cam_fmt_array.count; i++)
    {
        printf("fmt[%d].name:%s\n", i, parray[i].name);
    }

    // fmt[0].name:MIPI_2lane_24Minput_RAW10_1280x720_30fps
    // fmt[1].name:MIPI_2lane_24Minput_RAW10_1280x720_50fps
    // fmt[2].name:MIPI_2lane_24Minput_RAW10_1280x720_60fps
    // fmt[3].name:MIPI_1lane_24Minput_RAW10_1920x1080_25fps
    // fmt[4].name:MIPI_2lane_24Minput_RAW10_1920x1080_25fps
    // fmt[5].name:MIPI_2lane_24Minput_RAW10_1920x1080_30fps
    // fmt[6].name:MIPI_2lane_24Minput_RAW10_800x800_30fps
    // fmt[7].name:MIPI_2lane_24Minput_RAW10_640x480_50fps
    // fmt[8].name:MIPI_2lane_24Minput_RAW8_1920x1080_30fps
    // fmt[9].name:MIPI_2lane_24Minput_RAW8_1280x720_30fps
    // fmt[10].name:MIPI_2lane_24Minput_RAW8_800x800_30fps
    // fmt[11].name:MIPI_2lane_24Minput_RAW8_1024x600_30fps
    // fmt[12].name:DVP_8bit_24Minput_RAW10_1280x720_30fps
    esp_cam_sensor_format_t *cam_cur_fmt = NULL;
    for (int i = 0; i < cam_fmt_array.count; i++)
    {
        if (!strcmp(parray[i].name, SC2336_FORMAT))
        {
            cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[i].name);
        }
    }

    // cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[7].name);
    // cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[8].name);

    esp_err_t assert = esp_cam_sensor_set_format(cam, (const esp_cam_sensor_format_t *)cam_cur_fmt);
    if (assert != ESP_OK)
    {
        printf("esp_cam_sensor_set_format fail (error code: %#X)\n", assert);
        return false;
    }
    printf("format in use: %s\n", cam_cur_fmt->name);

    int enable_flag = 1;
    // Set sensor output stream
    assert = esp_cam_sensor_ioctl(cam, ESP_CAM_SENSOR_IOC_S_STREAM, &enable_flag);
    if (assert != ESP_OK)
    {
        printf("esp_cam_sensor_ioctl fail (error code: %#X)\n", assert);
        return false;
    }

    return true;
}

bool Mipi_Dsi_Init(uint8_t num_data_lanes, uint32_t lane_bit_rate_mbps, uint32_t dpi_clock_freq_mhz, uint32_t width, uint32_t height,
                   uint32_t mipi_dsi_hsync, uint32_t mipi_dsi_hbp, uint32_t mipi_dsi_hfp, uint32_t mipi_dsi_vsync, uint32_t mipi_dsi_vbp, uint32_t mipi_dsi_vfp,
                   uint32_t data_bit_length, esp_lcd_panel_handle_t *mipi_dpi_panel)
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
        .in_color_format = LCD_COLOR_FMT_RGB888,
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
        .bits_per_pixel = data_bit_length,
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

bool SC2336_Init(esp_cam_ctlr_handle_t cam_ctlr_handle, esp_cam_ctlr_trans_t &cam_ctlr_trans)
{
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

    size_t frame_buffer_size = 0;
    void *frame_buffer = NULL;

    //---------------DSI Init------------------//

    if (Mipi_Dsi_Init(SC2336_DATA_LANE_NUM, SC2336_LANE_BIT_RATE_MBPS, SC2336_MIPI_DSI_DPI_CLK_MHZ,
                      SC2336_WIDTH, SC2336_HEIGHT, 0, 0, 0, 0, 0, 0, SC2336_DATA_BIT_LENGTH, &mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    esp_err_t assert = esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 1, &frame_buffer);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_dpi_panel_get_frame_buffer fail (error code: %#X)\n", assert);
        return false;
    }

    //---------------Necessary variable config------------------//
    frame_buffer_size = SC2336_WIDTH * SC2336_HEIGHT * SC2336_DATA_BIT_LENGTH / 8;

    printf("frame_buffer_size: %zu\n", frame_buffer_size);
    printf("frame_buffer: %p\n", frame_buffer);

    cam_ctlr_trans.buffer = frame_buffer;
    cam_ctlr_trans.buflen = frame_buffer_size;

    //--------Camera Sensor and SCCB Init-----------//

    SC2336_Sensor_Init(IIC_Bus_1);

    //---------------CSI Init------------------//
    esp_cam_ctlr_csi_config_t csi_config = {
        .ctlr_id = 0,
        .h_res = SC2336_WIDTH,
        .v_res = SC2336_HEIGHT,
        .data_lane_num = SC2336_DATA_LANE_NUM,
        .lane_bit_rate_mbps = SC2336_CSI_LANE_BIT_RATE_MBPS,
        .input_data_color_type = SC2336_CSI_INPUT_DATA_COLOR_TYPE,
        .output_data_color_type = SC2336_CSI_OUTPUT_DATA_COLOR_TYPE,
        .queue_items = 1,
        .byte_swap_en = false,
    };
    assert = esp_cam_new_csi_ctlr(&csi_config, &cam_ctlr_handle);
    if (assert != ESP_OK)
    {
        printf("esp_cam_new_csi_ctlr fail (error code: %#X)\n", assert);
        return false;
    }

    esp_cam_ctlr_evt_cbs_t cbs = {
        .on_get_new_trans = [](esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data) -> IRAM_ATTR bool
        {
            esp_cam_ctlr_trans_t new_trans = *(esp_cam_ctlr_trans_t *)user_data;

            trans->buffer = new_trans.buffer;
            trans->buflen = new_trans.buflen;

            Cam_Trans_Flag = true;

            return false;
        },
        .on_trans_finished = [](esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data) -> IRAM_ATTR bool
        {
            return true;
        },
    };
    assert = esp_cam_ctlr_register_event_callbacks(cam_ctlr_handle, &cbs, &cam_ctlr_trans);
    if (assert != ESP_OK)
    {
        printf("esp_cam_ctlr_register_event_callbacks fail (error code: %#X)\n", assert);
        return false;
    }

    assert = esp_cam_ctlr_enable(cam_ctlr_handle);
    if (assert != ESP_OK)
    {
        printf("esp_cam_ctlr_enable fail (error code: %#X)\n", assert);
        return false;
    }

    //---------------ISP Init------------------//
    isp_proc_handle_t isp_proc = NULL;
    esp_isp_processor_cfg_t isp_config = {
        .clk_hz = 160 * 1000 * 1000,
        .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
        .input_data_color_type = SC2336_ISP_INPUT_DATA_COLOR_TYPE,
        .output_data_color_type = SC2336_ISP_OUTPUT_DATA_COLOR_TYPE,
        .has_line_start_packet = false,
        .has_line_end_packet = false,
        .h_res = SC2336_WIDTH,
        .v_res = SC2336_HEIGHT,
    };
    assert = esp_isp_new_processor(&isp_config, &isp_proc);
    if (assert != ESP_OK)
    {
        printf("esp_isp_new_processor fail (error code: %#X)\n", assert);
        return false;
    }

    // 自动对焦
    //  esp_isp_af_config_t af_config = {
    //      .edge_thresh = 128,
    //  };
    //  isp_af_ctlr_t af_ctrlr = NULL;
    // assert = esp_isp_new_af_controller(isp_proc, &af_config, &af_ctrlr);
    // if (assert != ESP_OK)
    // {
    //     printf("esp_isp_new_af_controller fail (error code: %#X)\n", assert);
    //     return false;
    // }
    // assert = esp_isp_af_controller_enable(af_ctrlr);
    // if (assert != ESP_OK)
    // {
    //     printf("esp_isp_af_controller_enable fail (error code: %#X)\n", assert);
    //     return false;
    // }

    // // 自动白平衡
    // isp_awb_ctlr_t awb_ctlr = NULL;
    // /* AWB 配置，请参考 API 注释来调整参数 */
    // esp_isp_awb_config_t awb_config = {
    //     .sample_point = ISP_AWB_SAMPLE_POINT_AFTER_CCM,
    //     .window =
    //         {
    //             .top_left =
    //                 {
    //                     .x = SC2336_WIDTH / 2 - 100,
    //                     .y = SC2336_HEIGHT / 2 - 100,
    //                 },
    //             .btm_right =
    //                 {
    //                     .x = SC2336_WIDTH / 2 + 100,
    //                     .y = SC2336_HEIGHT / 2 + 100,
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
    // assert = esp_isp_new_awb_controller(isp_proc, &awb_config, &awb_ctlr);
    // if (assert != ESP_OK)
    // {
    //     printf("esp_isp_new_awb_controller fail (error code: %#X)\n", assert);
    //     return false;
    // }
    // assert = esp_isp_awb_controller_enable(awb_ctlr);
    // if (assert != ESP_OK)
    // {
    //     printf("esp_isp_awb_controller_enable fail (error code: %#X)\n", assert);
    //     return false;
    // }

    // 自动曝光
    esp_isp_ae_config_t ae_config = {
        .sample_point = ISP_AE_SAMPLE_POINT_AFTER_DEMOSAIC,
        .window =
            {
                .top_left =
                    {
                        .x = SC2336_WIDTH / 2 - 100,
                        .y = SC2336_HEIGHT / 2 - 100,
                    },
                .btm_right =
                    {
                        .x = SC2336_WIDTH / 2 + 100,
                        .y = SC2336_HEIGHT / 2 + 100,
                    },
            },
    };
    isp_ae_ctlr_t ae_ctlr = NULL;
    assert = esp_isp_new_ae_controller(isp_proc, &ae_config, &ae_ctlr);
    if (assert != ESP_OK)
    {
        printf("esp_isp_new_ae_controller fail (error code: %#X)\n", assert);
        return false;
    }
    assert = esp_isp_ae_controller_enable(ae_ctlr);
    if (assert != ESP_OK)
    {
        printf("esp_isp_ae_controller_enable fail (error code: %#X)\n", assert);
        return false;
    }

    // 色彩控制器
    // esp_isp_color_config_t color_config = {
    //     .color_contrast = {
    //         .decimal = 100,
    //         .integer = 0,
    //     },
    //     .color_saturation = {
    //         .decimal = 90,
    //         .integer = 0,
    //     },
    //     .color_hue = 30,
    //     .color_brightness = 0,
    // };
    // assert = esp_isp_color_configure(isp_proc, &color_config);
    // if (assert != ESP_OK)
    // {
    //     printf("esp_isp_color_configure fail (error code: %#X)\n", assert);
    //     return false;
    // }
    // assert = esp_isp_color_enable(isp_proc);
    // if (assert != ESP_OK)
    // {
    //     printf("esp_isp_color_enable fail (error code: %#X)\n", assert);
    //     return false;
    // }

    assert = esp_isp_enable(isp_proc);
    if (assert != ESP_OK)
    {
        printf("esp_isp_enable fail (error code: %#X)\n", assert);
        return false;
    }

    //---------------DPI Reset------------------//
    // 初始化白屏
    memset(frame_buffer, 0xFF, frame_buffer_size);
    assert = esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if (assert != ESP_OK)
    {
        printf("esp_cache_msync fail (error code: %#X)\n", assert);
        return false;
    }

    assert = esp_cam_ctlr_start(cam_ctlr_handle);
    if (assert != ESP_OK)
    {
        printf("esp_cam_ctlr_start fail (error code: %#X)\n", assert);
        return false;
    }

    return true;
}

bool Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    if (Mipi_Dsi_Init(HI8561_SCREEN_DATA_LANE_NUM, HI8561_SCREEN_LANE_BIT_RATE_MBPS, HI8561_SCREEN_MIPI_DSI_DPI_CLK_MHZ,
                      HI8561_SCREEN_WIDTH, HI8561_SCREEN_HEIGHT, HI8561_SCREEN_MIPI_DSI_HSYNC, HI8561_SCREEN_MIPI_DSI_HBP,
                      HI8561_SCREEN_MIPI_DSI_HFP, HI8561_SCREEN_MIPI_DSI_VSYNC, HI8561_SCREEN_MIPI_DSI_VBP, HI8561_SCREEN_MIPI_DSI_VFP,
                      HI8561_SCREEN_DATA_BIT_LENGTH, mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    return true;
}

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
    printf("XL9535 ID: %#X\n", XL9535->get_device_id());
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    SGM38121->begin();
    printf("SGM38121 ID: %#X\n", SGM38121->get_device_id());

    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);

    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);

    ESP32P4->create_pwm(HI8561_SCREEN_BL, ledc_channel_t::LEDC_CHANNEL_0, 2000);

    vTaskDelay(pdMS_TO_TICKS(1000));

    example_bsp_enable_dsi_phy_power();

    esp_cam_ctlr_handle_t cam_ctlr_handle = NULL;
    esp_cam_ctlr_trans_t cam_ctlr_trans;
    SC2336_Init(cam_ctlr_handle, cam_ctlr_trans);

    esp_lcd_panel_handle_t screen_mipi_dpi_panel = NULL;
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
        // esp_cam_ctlr_receive(cam_handle, &new_trans, ESP_CAM_CTLR_MAX_DELAY);

        if (Cam_Trans_Flag == true)
        {
            // 分配目标图像内存
            auto buffer = std::make_unique<uint8_t[]>(cam_ctlr_trans.buflen);

            crop_rgb888_image((uint8_t *)cam_ctlr_trans.buffer, buffer.get(), SC2336_WIDTH, SC2336_HEIGHT,
                              (SC2336_WIDTH - HI8561_SCREEN_WIDTH) / 2, 0, HI8561_SCREEN_WIDTH, SC2336_HEIGHT);

            esp_lcd_panel_draw_bitmap(screen_mipi_dpi_panel, 0, 0, HI8561_SCREEN_WIDTH, SC2336_HEIGHT, buffer.get());
            Cam_Trans_Flag = false;
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
