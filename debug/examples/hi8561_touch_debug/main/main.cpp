#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::HWIIC>(IIC_1_SDA, IIC_1_SCL, I2C_NUM_0);

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

    IIC_Bus->begin(-1, 0x68);

    // uint8_t buffer[] = {0xF3, 0x20, 0x01, 0x10, 0xCC, 0x03};
    // uint8_t buffer_2[6] = {0};
    // // [2025-02-08 10:45:06.120] data[0]: 0X6
    // // [2025-02-08 10:45:06.120] data[1]: 0
    // // [2025-02-08 10:45:06.120] data[2]: 0
    // // [2025-02-08 10:45:06.120] data[3]: 0
    // // [2025-02-08 10:45:06.120] data[4]: 0
    // // [2025-02-08 10:45:06.120] data[5]: 0X10

    uint8_t buffer[] = {0xF3, 0x20, 0x01, 0x10, 0xD0, 0x03};
    uint8_t buffer_2[48] = {0};
    // [2025-02-08 11:13:38.519] buffer_2 data[0]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[1]: 0X10
    // [2025-02-08 11:13:38.519] buffer_2 data[2]: 0X1
    // [2025-02-08 11:13:38.519] buffer_2 data[3]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[4]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[5]: 0X1
    // [2025-02-08 11:13:38.519] buffer_2 data[6]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[7]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[8]: 0X20 //Touch address
    // [2025-02-08 11:13:38.519] buffer_2 data[9]: 0X11 //Touch address
    // [2025-02-08 11:13:38.519] buffer_2 data[10]: 0X1 //Touch address
    // [2025-02-08 11:13:38.519] buffer_2 data[11]: 0X20 //Touch address
    // [2025-02-08 11:13:38.519] buffer_2 data[12]: 0X3C //Touch length
    // [2025-02-08 11:13:38.519] buffer_2 data[13]: 0 //Touch length
    // [2025-02-08 11:13:38.519] buffer_2 data[14]: 0 //Touch length
    // [2025-02-08 11:13:38.519] buffer_2 data[15]: 0 //Touch length
    // [2025-02-08 11:13:38.519] buffer_2 data[16]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[17]: 0X12
    // [2025-02-08 11:13:38.519] buffer_2 data[18]: 0X1
    // [2025-02-08 11:13:38.519] buffer_2 data[19]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[20]: 0X80
    // [2025-02-08 11:13:38.519] buffer_2 data[21]: 0X4
    // [2025-02-08 11:13:38.519] buffer_2 data[22]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[23]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[24]: 0XA0
    // [2025-02-08 11:13:38.519] buffer_2 data[25]: 0X16
    // [2025-02-08 11:13:38.519] buffer_2 data[26]: 0X1
    // [2025-02-08 11:13:38.519] buffer_2 data[27]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[28]: 0X18
    // [2025-02-08 11:13:38.519] buffer_2 data[29]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[30]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[31]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[32]: 0XB8
    // [2025-02-08 11:13:38.519] buffer_2 data[33]: 0X16
    // [2025-02-08 11:13:38.519] buffer_2 data[34]: 0X1
    // [2025-02-08 11:13:38.519] buffer_2 data[35]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[36]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[37]: 0X1
    // [2025-02-08 11:13:38.519] buffer_2 data[38]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[39]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[40]: 0XB8
    // [2025-02-08 11:13:38.519] buffer_2 data[41]: 0X17
    // [2025-02-08 11:13:38.519] buffer_2 data[42]: 0X1
    // [2025-02-08 11:13:38.519] buffer_2 data[43]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[44]: 0X20
    // [2025-02-08 11:13:38.519] buffer_2 data[45]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[46]: 0
    // [2025-02-08 11:13:38.519] buffer_2 data[47]: 0

    IIC_Bus->write_read(buffer, 6, buffer_2, 48);

    uint8_t buffer_3[] = {0xF3, 0X20, 0X01, 0X11, 0X20, 0x03};
    uint8_t buffer_4[78] = {0};

    while (1)
    {
        IIC_Scan();

        // for (size_t i = 0; i < 48; i++)
        // {
        //     printf("buffer_2 data[%d]: %#X\n", i, buffer_2[i]);
        // }

        IIC_Bus->write_read(buffer_3, 6, buffer_4, 78);

        // [2025-02-08 18:07:42.535] buffer_4 data[0]: 0X1 //触摸手指数
        // [2025-02-08 18:07:42.535] buffer_4 data[1]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[2]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[3]: 0XFF //触摸手指号为1的X坐标高位
        // [2025-02-08 18:07:42.535] buffer_4 data[4]: 0XFF //触摸手指号为1的X坐标低位
        // [2025-02-08 18:07:42.535] buffer_4 data[5]: 0XFF //触摸手指号为1的Y坐标高位
        // [2025-02-08 18:07:42.535] buffer_4 data[6]: 0XFF //触摸手指号为1的Y坐标低位
        // [2025-02-08 18:07:42.535] buffer_4 data[7]: 0 //触摸手指号为1的压力
        // [2025-02-08 18:07:42.535] buffer_4 data[8]: 0XFF //触摸手指号为2的X坐标高位
        // [2025-02-08 18:07:42.535] buffer_4 data[9]: 0XFF //触摸手指号为2的X坐标低位
        // [2025-02-08 18:07:42.535] buffer_4 data[10]: 0XFF //触摸手指号为2的Y坐标高位
        // [2025-02-08 18:07:42.535] buffer_4 data[11]: 0XFF //触摸手指号为2的Y坐标低位
        // [2025-02-08 18:07:42.535] buffer_4 data[12]: 0 //触摸手指号为2的压力
        // [2025-02-08 18:07:42.535] buffer_4 data[13]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[14]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[15]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[16]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[17]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[18]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[19]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[20]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[21]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[22]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[23]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[24]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[25]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[26]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[27]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[28]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[29]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[30]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[31]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[32]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[33]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[34]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[35]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[36]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[37]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[38]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[39]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[40]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[41]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[42]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[43]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[44]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[45]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[46]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[47]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[48]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[49]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[50]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[51]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[52]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[53]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[54]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[55]: 0X1 //屏幕边缘触摸
        // [2025-02-08 18:07:42.535] buffer_4 data[56]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[57]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[58]: 0X27
        // [2025-02-08 18:07:42.535] buffer_4 data[59]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[60]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[61]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[62]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[63]: 0
        // [2025-02-08 18:07:42.535] buffer_4 data[64]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[65]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[66]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[67]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[68]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[69]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[70]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[71]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[72]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[73]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[74]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[75]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[76]: 0XFF
        // [2025-02-08 18:07:42.535] buffer_4 data[77]: 0XFF

        for (size_t i = 0; i < 78; i++)
        {
            printf("buffer_4 data[%d]: %#X\n", i, buffer_4[i]);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
