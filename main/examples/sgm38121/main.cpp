/*
 * @Description: sgm38121
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:52:48
 * @LastEditTime: 2026-03-06 17:25:51
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Sgm38121_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Sgm38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(Sgm38121_Iic_Bus, SGM38121_IIC_ADDRESS);

void Iic_Scan(void)
{
    std::vector<uint8_t> address;
    if (Sgm38121_Iic_Bus->scan_7bit_address(&address) == true)
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

    vTaskDelay(pdMS_TO_TICKS(100));

    Sgm38121->begin();

    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);

    // Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1000);
    // Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_2, 1000);

    while (1)
    {
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
        vTaskDelay(pdMS_TO_TICKS(1000));
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
        // vTaskDelay(pdMS_TO_TICKS(1000));
        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        // Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        // vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
