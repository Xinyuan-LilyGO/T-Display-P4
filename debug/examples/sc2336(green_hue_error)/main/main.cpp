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
#include "pin_config.h"
#include "cpp_bus_driver_library.h"
#include "hi8561_driver.h"

extern "C"
{
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

#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 1168
#define SCREEN_BITS_PER_PIXEL 24 // RGB888

// Refresh Rate = 80000000/(80+50+150+568)/(40+120+80+1232) = 64Hz
// #define MIPI_DSI_LCD_H_RES 540
// #define MIPI_DSI_LCD_V_RES 1168
#define MIPI_DSI_DPI_CLK_MHZ 60
#define MIPI_DSI_LCD_HSYNC 28
#define MIPI_DSI_LCD_HBP 26
#define MIPI_DSI_LCD_HFP 20
#define MIPI_DSI_LCD_VSYNC 2
#define MIPI_DSI_LCD_VBP 22
#define MIPI_DSI_LCD_VFP 200

static const char *TAG = "cam_dsi";

size_t CycleTime = 0;

void *frame_buffer = NULL;
size_t frame_buffer_size = 0;
esp_cam_ctlr_trans_t new_trans;
esp_cam_ctlr_handle_t cam_handle = NULL;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto SGM38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(IIC_Bus_1, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

static bool s_camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    esp_cam_ctlr_trans_t new_trans = *(esp_cam_ctlr_trans_t *)user_data;
    trans->buffer = new_trans.buffer;
    trans->buflen = new_trans.buflen;

    return false;
}

static bool s_camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    return false;
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

    hi8561_vendor_config_t vendor_config = {
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_hi8561(*mipi_dbi_io, &lcd_dev_config, mipi_dpi_panel));

    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(*mipi_dpi_panel, 1, frame_buffer));
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

    // [2025-02-11 11:00:42.781] I (3663) cam_dsi: fmt[0].name:MIPI_2lane_24Minput_RAW10_1280x720_30fps
    // [2025-02-11 11:00:42.781] I (3663) cam_dsi: fmt[1].name:MIPI_2lane_24Minput_RAW10_1280x720_50fps
    // [2025-02-11 11:00:42.781] I (3673) cam_dsi: fmt[2].name:MIPI_2lane_24Minput_RAW10_1280x720_60fps
    // [2025-02-11 11:00:42.781] I (3683) cam_dsi: fmt[3].name:MIPI_1lane_24Minput_RAW10_1920x1080_25fps
    // [2025-02-11 11:00:42.781] I (3683) cam_dsi: fmt[4].name:MIPI_2lane_24Minput_RAW10_1920x1080_25fps
    // [2025-02-11 11:00:42.781] I (3693) cam_dsi: fmt[5].name:MIPI_2lane_24Minput_RAW10_1920x1080_30fps
    // [2025-02-11 11:00:42.781] I (3703) cam_dsi: fmt[6].name:MIPI_2lane_24Minput_RAW10_800x800_30fps
    // [2025-02-11 11:00:42.781] I (3703) cam_dsi: fmt[7].name:MIPI_2lane_24Minput_RAW10_640x480_50fps
    // [2025-02-11 11:00:42.781] I (3713) cam_dsi: fmt[8].name:MIPI_2lane_24Minput_RAW8_1920x1080_30fps
    // [2025-02-11 11:00:42.781] I (3713) cam_dsi: fmt[9].name:MIPI_2lane_24Minput_RAW8_1280x720_30fps
    // [2025-02-11 11:00:42.781] I (3723) cam_dsi: fmt[10].name:MIPI_2lane_24Minput_RAW8_800x800_30fps
    // [2025-02-11 11:00:42.781] I (3733) cam_dsi: fmt[11].name:MIPI_2lane_24Minput_RAW8_1024x600_30fps
    // [2025-02-11 11:00:42.781] I (3733) cam_dsi: fmt[12].name:DVP_8bit_24Minput_RAW10_1280x720_30fps

    cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[7].name);
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

esp_err_t Camera_Init()
{
    esp_err_t ret = ESP_FAIL;
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
    esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

    // mipi ldo
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = CONFIG_EXAMPLE_USED_LDO_CHAN_ID,
        .voltage_mv = CONFIG_EXAMPLE_USED_LDO_VOLTAGE_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));

    /**
     * @background
     * Sensor use RAW8
     * ISP convert to RGB565
     */
    //---------------DSI Init------------------//
    // example_dsi_resource_alloc(&mipi_dsi_bus, &mipi_dbi_io, &mipi_dpi_panel, &frame_buffer);
    example_dsi_resource_alloc_2(&mipi_dsi_bus, &mipi_dbi_io, &mipi_dpi_panel, &frame_buffer);

    //---------------Necessary variable config------------------//
    frame_buffer_size = 640 * 480 * 24 / 8;

    ESP_LOGI(TAG, "CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES: %d, CONFIG_EXAMPLE_MIPI_DSI_DISP_VRES: %d, bits per pixel: %d", CONFIG_EXAMPLE_MIPI_CSI_DISP_HRES, CONFIG_EXAMPLE_MIPI_DSI_DISP_VRES, 8);
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
        .h_res = 640,
        .v_res = 480,
        .data_lane_num = 2,
        .lane_bit_rate_mbps = EXAMPLE_MIPI_CSI_LANE_BITRATE_MBPS,
        .input_data_color_type = CAM_CTLR_COLOR_RAW10,
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
        .clk_hz = 80 * 1000 * 1000,
        .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
        .input_data_color_type = ISP_COLOR_RAW10,
        .output_data_color_type = ISP_COLOR_RGB888,
        .has_line_start_packet = false,
        .has_line_end_packet = false,
        .h_res = 640,
        .v_res = 480,
    };
    ESP_ERROR_CHECK(esp_isp_new_processor(&isp_config, &isp_proc));
    ESP_ERROR_CHECK(esp_isp_enable(isp_proc));

    // //---------------DPI Reset------------------//
    // example_dpi_panel_reset(mipi_dpi_panel);

    // init to all white
    memset(frame_buffer, 0xFF, frame_buffer_size);
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

    if (esp_cam_ctlr_start(cam_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Driver start fail");
        return ret;
    }

    // example_dpi_panel_init(mipi_dpi_panel);

    return ret;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    printf("XL9535 ID: %#X\n", XL9535->get_device_id());
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(1000));

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

    vTaskDelay(pdMS_TO_TICKS(1000));

    Camera_Init();

    while (1)
    {
        ESP_ERROR_CHECK(esp_cam_ctlr_receive(cam_handle, &new_trans, ESP_CAM_CTLR_MAX_DELAY));

        if ((esp_timer_get_time() / 1000) > CycleTime)
        {
            if (new_trans.buflen > 0)
            {
                // 打印接收到的数据
                for (size_t i = 0; i < new_trans.buflen; i++)
                {
                    printf("data: %#X\n", ((uint8_t *)new_trans.buffer)[i]);
                }
            }
            CycleTime = esp_timer_get_time() + 1000;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
