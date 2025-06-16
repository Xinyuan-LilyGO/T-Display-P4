/*
 * @Description: aw86224
 * @Author: LILYGO_L
 * @Date: 2024-12-25 10:33:25
 * @LastEditTime: 2025-06-16 16:21:20
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "pin_config.h"
#include "cpp_bus_driver_library.h"

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto AW86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(IIC_Bus_1, AW86224_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

void IIC_Scan(void)
{
    std::vector<uint8_t> address;
    if (IIC_Bus_1->scan_7bit_address(&address) == true)
    {
        for (size_t i = 0; i < address.size(); i++)
        {
            printf("Discovered IIC devices[%u]: %#X\n", i, address[i]);
        }
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    AW86224->begin(500000);
    // printf("AW86224 input voltage: %.06f V\n", AW86224->get_input_voltage());

    IIC_Scan();

    // 等待F0校准
    while (1)
    {
        uint32_t f0_value = AW86224->get_f0_detection();
        printf("AW86224 get f0 detection value: %ld\n", f0_value);

        if (AW86224->set_f0_calibrate(f0_value) == true)
        {
            break;
        }
    }

    // AW86224->set_waveform_data_sample_rate(Cpp_Bus_Driver::Aw862xx::Sample_Rate::RATE_12KHZ);

    // RAM播放
    AW86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    while (1)
    {
        // IIC_Scan();
        // printf("AW86224 input voltage: %.06f V\n", AW86224->get_input_voltage());

        // // RTP播放
        //  Cpp_Bus_Driver::Aw862xx::System_Status ss;
        //  if (AW86224->get_system_status(ss) == true)
        //  {
        //      if (ss.rtp_fifo_full == false)
        //      {
        //          AW86224->run_rtp_playback_waveform(Cpp_Bus_Driver::haptic_waveform_ordinary, sizeof(Cpp_Bus_Driver::haptic_waveform_ordinary));
        //          printf("AW86224 rtp_playback_waveform \n");

        //         vTaskDelay(pdMS_TO_TICKS(100));
        //     }
        // }
        // vTaskDelay(pdMS_TO_TICKS(10));

        // RAM播放
        AW86224->run_ram_playback_waveform(1, 15, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));
        AW86224->stop_ram_playback_waveform();
        vTaskDelay(pdMS_TO_TICKS(1000));

        AW86224->run_ram_playback_waveform(2, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(30));
        AW86224->run_ram_playback_waveform(2, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(30));

        vTaskDelay(pdMS_TO_TICKS(1000));

        AW86224->run_ram_playback_waveform(1, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
