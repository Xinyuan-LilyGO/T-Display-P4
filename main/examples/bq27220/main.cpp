/*
 * @Description: bq27220
 * @Author: LILYGO_L
 * @Date: 2025-01-04 15:06:05
 * @LastEditTime: 2025-06-13 09:20:11
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"

auto XL9535_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto BQ27220_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ27220_SDA, BQ27220_SCL, I2C_NUM_0);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(XL9535_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto BQ27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(BQ27220_Bus, BQ27220_IIC_ADDRESS);

// auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_2>(IIC_1_SDA, IIC_1_SCL, I2C_NUM_0);

// auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
// auto BQ27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(IIC_Bus_0, BQ27220_IIC_ADDRESS);

// void IIC_Scan(void)
// {
//     std::vector<uint8_t> address;
//     if (IIC_Bus_0->scan_7bit_address(&address) == true)
//     {
//         for (size_t i = 0; i < address.size(); i++)
//         {
//             printf("Discovered IIC devices[%u]: %#X\n", i, address[i]);
//         }
//     }
// }

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(1000));

    BQ27220_Bus->_iic_bus_handle = XL9535_Bus->_iic_bus_handle;

    BQ27220->begin();

    printf("BQ27220 ID: %#X\n", BQ27220->get_device_id());

    // 设置的电池容量会在没有电池插入的时候自动还原为默认值
    BQ27220->set_design_capacity(400);
    BQ27220->set_temperature_mode(Cpp_Bus_Driver::Bq27220xxxx::Temperature_Mode::EXTERNAL_NTC);
    BQ27220->set_sleep_current_threshold(5);

    while (1)
    {
        // printf("BQ27220 ID: %#X\n", BQ27220->get_device_id());

        // IIC_Scan();
        printf("////////////////////////////////////////////////////\n");
        printf("--------------------------------------------------------------------------\n");
        printf("BQ27220 ID: %#X\n", BQ27220->get_device_id());
        printf("--------------------------------------------------------------------------\n");
        printf("design capacity: %dmah\n", BQ27220->get_design_capacity());
        printf("remaining capacity: %dmah\n", BQ27220->get_remaining_capacity());
        // 放电后才更新full_charge_capacity
        printf("full charge capacity: %dmah\n", BQ27220->get_full_charge_capacity());
        printf("raw coulomb count: %dc\n", BQ27220->get_raw_coulomb_count());
        printf("cycle count: %d\n", BQ27220->get_cycle_count());
        printf("battery level: %d%%\n", BQ27220->get_status_of_health());
        printf("battery health: %d%%\n", BQ27220->get_status_of_health());
        printf("--------------------------------------------------------------------------\n");
        printf("voltage: %dmv\n", BQ27220->get_voltage());
        int16_t current = BQ27220->get_current();
        printf("charging voltage: %dmv\n", BQ27220->get_charging_voltage());
        printf("current: %dma\n", current);
        printf("charging current: %dma\n", BQ27220->get_charging_current());
        printf("standby current: %dma\n", BQ27220->get_standby_current());
        printf("max load current current: %dma\n", BQ27220->get_max_load_current());
        printf("average power: %dmw\n", BQ27220->get_average_power());
        printf("--------------------------------------------------------------------------\n");
        printf("chip temperature: %.03f^C\n", BQ27220->get_chip_temperature_celsius());
        printf("ntc temperature: %.03f^C\n", BQ27220->get_temperature_celsius());
        printf("--------------------------------------------------------------------------\n");
        BQ27220->set_at_rate(current);
        printf("at rate: %dma\n", BQ27220->get_at_rate());
        printf("at rate battery time to empty: %dmin\n", BQ27220->get_at_rate_time_to_empty());
        printf("battery time to empty: %dmin\n", BQ27220->get_time_to_empty());
        printf("battery time to full charge: %dmin\n", BQ27220->get_time_to_full());
        printf("battery standby time to empty: %dmin\n", BQ27220->get_standby_time_to_empty());
        printf("battery max load time to empty: %dmin\n", BQ27220->get_max_load_time_to_empty());
        printf("--------------------------------------------------------------------------\n");

        // Cpp_Bus_Driver::Bq27220xxxx::Operation_Status os;
        // BQ27220->get_operation_status(os);

        Cpp_Bus_Driver::Bq27220xxxx::Battery_Status bs;
        if (BQ27220->get_battery_status(bs) == true)
        {
            printf("fully discharged flag: %d\n", bs.flag.fd);
            printf("sleep flag: %d\n", bs.flag.sleep);
            printf("charging overheat flag: %d\n", bs.flag.otc);
            printf("discharging overheat flag: %d\n", bs.flag.otd);
            printf("fully discharged flag: %d\n", bs.flag.fc);
            printf("charging prohibited flag: %d\n", bs.flag.chginh);
            printf("terminate charging alarm flag: %d\n", bs.flag.tca);
            printf("terminate discharging alarm flag: %d\n", bs.flag.tda);
            printf("battery insertion detection flag: %d\n", bs.flag.auth_gd);
            printf("battery present flag: %d\n", bs.flag.battpres);
            printf("discharge flag: %d\n", bs.flag.dsg);
        }
        printf("--------------------------------------------------------------------------\n");
        printf("////////////////////////////////////////////////////\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
