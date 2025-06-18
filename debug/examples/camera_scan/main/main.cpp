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
#include "hi8561_driver.h"

extern "C"
{
#include "esp_sccb_intf.h"
#include "esp_sccb_i2c.h"
#include "esp_cam_sensor.h"
#include "esp_cam_sensor_detect.h"
#include "example_sensor_init_config.h"
}

static const char *TAG = "cam_dsi";

size_t CycleTime = 0;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto SGM38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(IIC_Bus_1, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    printf("XL9535 ID: %#X\n", XL9535->get_device_id());
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    SGM38121->begin();
    printf("SGM38121 ID: %#X\n", SGM38121->get_device_id());

    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3100);

    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);

    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t ret = ESP_FAIL;

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
        ESP_ERROR_CHECK(sccb_new_i2c_io(IIC_Bus_1->_iic_bus_handle, &i2c_config, &cam_config.sccb_handle));

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

    while (1)
    {
        if (esp_log_timestamp() > CycleTime)
        {
            for (int i = 0; i < cam_fmt_array.count; i++)
            {
                ESP_LOGI(TAG, "fmt[%d].name:%s", i, parray[i].name);
            }

            CycleTime = esp_log_timestamp() + 1000;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
