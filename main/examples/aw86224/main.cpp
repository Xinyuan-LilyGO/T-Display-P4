/*
 * @Description: aw86224
 * @Author: LILYGO_L
 * @Date: 2024-12-25 10:33:25
 * @LastEditTime: 2026-03-05 16:23:00
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Aw86224_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Aw86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(Aw86224_Iic_Bus, AW86224_IIC_ADDRESS);

void Iic_Scan(void)
{
    std::vector<uint8_t> address;
    if (Aw86224_Iic_Bus->scan_7bit_address(&address) == true)
    {
        for (size_t i = 0; i < address.size(); i++)
        {
            printf("discovered iic devices[%u]: %#X\n", i, address[i]);
        }
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Aw86224->begin(500000);
    // printf("AW86224 input voltage: %.06f V\n", Aw86224->get_input_voltage());

    Iic_Scan();

    // 等待F0校准
    while (1)
    {
        uint32_t f0_value = Aw86224->get_f0_detection();
        printf("AW86224 get f0 detection value: %ld\n", f0_value);

        if (Aw86224->set_f0_calibrate(f0_value) == true)
        {
            break;
        }
    }

    // Aw86224->set_waveform_data_sample_rate(Cpp_Bus_Driver::Aw862xx::Sample_Rate::RATE_12KHZ);

    // RAM播放
    Aw86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    while (1)
    {
        // Iic_Scan();
        // printf("AW86224 input voltage: %.06f V\n", Aw86224->get_input_voltage());

        // // RTP播放
        //  Cpp_Bus_Driver::Aw862xx::System_Status ss;
        //  if (Aw86224->get_system_status(ss) == true)
        //  {
        //      if (ss.rtp_fifo_full == false)
        //      {
        //          Aw86224->run_rtp_playback_waveform(Cpp_Bus_Driver::haptic_waveform_ordinary, sizeof(Cpp_Bus_Driver::haptic_waveform_ordinary));
        //          printf("AW86224 rtp_playback_waveform \n");

        //         vTaskDelay(pdMS_TO_TICKS(100));
        //     }
        // }
        // vTaskDelay(pdMS_TO_TICKS(10));

        // RAM播放
        Aw86224->run_ram_playback_waveform(1, 15, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));
        Aw86224->stop_ram_playback_waveform();
        vTaskDelay(pdMS_TO_TICKS(1000));

        Aw86224->run_ram_playback_waveform(2, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(30));
        Aw86224->run_ram_playback_waveform(2, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(30));

        vTaskDelay(pdMS_TO_TICKS(1000));

        Aw86224->run_ram_playback_waveform(1, 14, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
