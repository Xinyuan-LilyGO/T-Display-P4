#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "pin_config.h"
#include "cpp_bus_driver_library.h"

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::HWIIC>(IIC_1_SDA, IIC_1_SCL, I2C_NUM_0);

auto HI8561_T = std::make_unique<Cpp_Bus_Driver::Hi8561_Touch>(IIC_Bus, HI8561_TOUCH_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

volatile bool HI8561_T_Interrupt_Flag = false;

IRAM_ATTR void HI8561_T_Interrupt(void *arg)
{
    HI8561_T_Interrupt_Flag = true;
}

void IIC_Scan(void)
{
    std::vector<uint8_t> address;
    if (IIC_Bus->scan_7bit_address(&address) == true)
    {
        for (size_t i = 0; i < address.size(); i++)
        {
            printf("Discovered IIC devices[%u]: %#x\n", i, address[i]);
        }
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    HI8561_T->create_gpio_interrupt(24, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING, HI8561_T_Interrupt);

    HI8561_T->begin();

    Cpp_Bus_Driver::Hi8561_Touch::Touch_Point tp;

    while (1)
    {
        // IIC_Scan();

        // uint8_t finger_count = HI8561_T->get_finger_count();
        // if (HI8561_T->get_single_touch_point(tp, finger_count) == true)
        // {
        //     printf("Touch finger: %d\nX: %d Y: %d P: %d\n", finger_count, tp.x, tp.y, tp.p);
        // }

        if (HI8561_T_Interrupt_Flag == true)
        {
            if (HI8561_T->get_single_touch_point(tp) == true)
            {
                printf("Touch finger: %d Edge touch flag: %#X\nX: %d Y: %d P: %d\n", tp.finger_count, tp.edge_touch_flag, tp.x, tp.y, tp.p);
            }

            HI8561_T_Interrupt_Flag = false;
        }
        else
        {
            if (HI8561_T->get_edge_touch() == true)
            {
                printf("Edge detection triggered\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));

        // HI8561_T->get_single_touch_point(tp);
        // printf("Touch finger: %d edge_touch: %#X\nX: %d Y: %d P: %d\n",
        //        tp.finger_count, tp.edge_touch_flag,tp.x, tp.y, tp.p);
        // vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
