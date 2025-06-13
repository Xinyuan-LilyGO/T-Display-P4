/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2024-12-27 11:27:37
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

        // if (iic_initialization() == false)
        // {
        //     log_e("->IIC_Initialization() fail");
        //     return false;
        // }

        // if (iic_initialization() == false)
        // {
        //     cpp_bus_driver_assert(CPP_Bus_Driver_Log_Level::BUS, __FILE__, __LINE__, "iic_initialization fail");
        //     return false;
        // }

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