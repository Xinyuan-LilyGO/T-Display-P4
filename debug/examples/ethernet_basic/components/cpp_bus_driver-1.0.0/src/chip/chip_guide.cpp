/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2025-01-04 14:43:12
 * @License: GPL 3.0
 */
#include "chip_guide.h"

namespace Cpp_Bus_Driver
{
    bool IIC_Guide::begin(int64_t frequency)
    {
        if (_bus->begin(frequency, _address) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        return true;
    }

    bool IIC_Guide::iic_init_list(const uint8_t *list, size_t length)
    {
        for (size_t i = 0; i < length; i++)
        {
            switch (list[i])
            {
            case static_cast<uint8_t>(Init_List_Cmd::DELAY_MS):
                i++;
                delay_ms(list[i]);
                break;
            case static_cast<uint8_t>(Init_List_Cmd::WRITE_DATA):
                i++;
                if (_bus->write(&list[i + 1], list[i]) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list write fail\n");
                    return false;
                }
                i = list[i] + 1;
                break;
            case static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8):
                i++;
                if (_bus->write(&list[i], 2) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "iic_init_list write fail\n");
                    return false;
                }
                i += 2;
                break;

            default:
                break;
            }
        }

        return true;
    }

    // int32_t CPP_IIC::IIC_Device_Reset(void)
    // {
    //     log_e("No 'IIC_Control_Device' fictional function has been created.");
    //     return -1;
    // }
    // bool CPP_IIC::IIC_Write_Device_State(uint32_t device, uint8_t state)
    // {
    //     log_e("No 'IIC_Control_Device' fictional function has been created.");
    //     return -1;
    // }
    // bool CPP_IIC::IIC_Write_Device_Value(uint32_t device, int64_t value)
    // {
    //     log_e("No 'IIC_Control_Device' fictional function has been created.");
    //     return -1;
    // }
    // bool CPP_IIC::IIC_Write_Device_Data(uint32_t device, const uint8_t *data, size_t length)
    // {
    //     log_e("No 'IIC_Control_Device' fictional function has been created.");
    //     return -1;
    // }
    // String CPP_IIC::IIC_Read_Device_State(uint32_t information)
    // {
    //     log_e("No 'IIC_Read_Information' fictional function has been created.");
    //     return "No 'IIC_Read_Information' fictional function has been created.";
    // }
    // double CPP_IIC::IIC_Read_Device_Value(uint32_t information)
    // {
    //     log_e("No 'IIC_Read_Information' fictional function has been created.");
    //     return -1;
    // }
}