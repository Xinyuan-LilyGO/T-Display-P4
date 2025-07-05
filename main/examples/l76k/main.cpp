/*
 * @Description: l76k
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:32:01
 * @LastEditTime: 2025-07-05 11:02:30
 * @License: GPL 3.0
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"

auto Iic_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(IIC_1_SDA, IIC_1_SCL, I2C_NUM_0);

auto Uart_Bus_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Uart>(GPS_RX, GPS_TX, UART_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Iic_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto L76K = std::make_unique<Cpp_Bus_Driver::L76k>(Uart_Bus_1, [](bool Value) -> IRAM_ATTR bool
                                                   { return XL9535->pin_write(XL9535_GPS_WAKE_UP, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(Value)); }, DEFAULT_CPP_BUS_DRIVER_VALUE);

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(100));

    XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH); // 关闭睡眠

    L76K->begin();
    printf("get_baud_rate:%ld\n", L76K->get_baud_rate());

    L76K->set_baud_rate(Cpp_Bus_Driver::L76k::Baud_Rate::BR_115200_BPS);
    printf("set_baud_rate:%ld\n", L76K->get_baud_rate());

    L76K->set_update_frequency(Cpp_Bus_Driver::L76k::Update_Freq::FREQ_5HZ);
    L76K->clear_rx_buffer_data();

    while (1)
    {
        // printf("get_baud_rate:%ld\n", L76K->get_baud_rate());

        // size_t buffer_lenght = L76K->get_rx_buffer_length();
        // if (buffer_lenght > 0)
        // {
        //     auto buffer = std::make_shared<uint8_t []>(buffer_lenght);

        //     if (L76K->read_data(buffer.get()) > 0)
        //     {
        //         printf("---begin---\n%s \n---end---\n", buffer.get());
        //     }
        // }

        // vTaskDelay(pdMS_TO_TICKS(L76K->_update_freq)); // 等待接收

        std::shared_ptr<uint8_t[]> buffer;
        size_t buffer_lenght = 0;

        if (L76K->get_info_data(buffer, &buffer_lenght) == true)
        {
            // 打印RMC的相关信息
            printf("---begin---\n%s \n---end---\n", buffer.get());

            printf("------------RMC------------\n");

            Cpp_Bus_Driver::L76k::Rmc rmc;

            if (L76K->parse_rmc_info(buffer, buffer_lenght, rmc) == true)
            {
                printf("location status: %s\n", (rmc.location_status).c_str());

                if (rmc.data.update_flag == true)
                {
                    printf("utc data year: %d month: %d day: %d\n", rmc.data.year, rmc.data.month, rmc.data.day);
                    rmc.data.update_flag = false;
                }
                if (rmc.utc.update_flag == true)
                {
                    printf("utc hour: %d minute: %d second: %f\n", rmc.utc.hour, rmc.utc.minute, rmc.utc.second);
                    printf("china hour: %d minute: %d second: %f\n", (rmc.utc.hour + 8 + 24) % 24, rmc.utc.minute, rmc.utc.second);
                    rmc.utc.update_flag = false;
                }

                if ((rmc.location.lat.update_flag == true) && (rmc.location.lat.direction_update_flag == true))
                {
                    printf("location lat degrees: %d \nlocation lat minutes: %.10lf \nlocation lat degrees_minutes: %.10lf \nlocation lat direction: %s\n",
                           rmc.location.lat.degrees, rmc.location.lat.minutes, rmc.location.lat.degrees_minutes, (rmc.location.lat.direction).c_str());
                    rmc.location.lat.update_flag = false;
                    rmc.location.lat.direction_update_flag = false;
                }
                if ((rmc.location.lon.update_flag == true) && (rmc.location.lon.direction_update_flag == true))
                {
                    printf("location lon degrees: %d \nlocation lon minutes: %.10lf \nlocation lon degrees_minutes: %.10lf \nlocation lon direction: %s\n",
                           rmc.location.lon.degrees, rmc.location.lon.minutes, rmc.location.lon.degrees_minutes, (rmc.location.lon.direction).c_str());
                    rmc.location.lon.update_flag = false;
                    rmc.location.lon.direction_update_flag = false;
                }
            }

            // printf("\n");

            // 打印GGA的相关信息
            // printf("------------GGA------------\n");

            // Cpp_Bus_Driver::L76k::Gga gga;

            // if (L76K->parse_gga_info(buffer, buffer_lenght, gga) == true)
            // {
            //     if (gga.utc.update_flag == true)
            //     {
            //         printf("utc hour: %d minute: %d second: %f\n", gga.utc.hour, gga.utc.minute, gga.utc.second);
            //         gga.utc.update_flag = false;
            //     }
            //     if ((gga.location.lat.update_flag == true) && (gga.location.lat.direction_update_flag == true))
            //     {
            //         printf("location lat: %f lat_direction: %s\n", gga.location.lat.degrees_minutes, (gga.location.lat.direction).c_str());
            //         gga.location.lat.update_flag = false;
            //         gga.location.lat.direction_update_flag = false;
            //     }
            //     if ((gga.location.lon.update_flag == true) && (gga.location.lon.direction_update_flag == true))
            //     {
            //         printf("location lon: %f lon_direction: %s\n", gga.location.lon.degrees_minutes, (gga.location.lon.direction).c_str());
            //         gga.location.lon.update_flag = false;
            //         gga.location.lon.direction_update_flag = false;
            //     }

            //     printf("gps mode status: %d\n", gga.gps_mode_status);
            //     printf("online satellite count: %d\n", gga.online_satellite_count);
            //     printf("hdop: %f\n", gga.hdop);
            // }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
