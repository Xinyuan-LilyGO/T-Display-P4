/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2024-12-26 11:05:57
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"
#include "../../resources/haptic_chip_waveform_library.h"

#define AW862xx_DEVICE_DEFAULT_ADDRESS 0x58

namespace Cpp_Bus_Driver
{
    static const uint8_t AW862xx_Initialization_Operation[] = {
        // BO_BEGIN_TRANSMISSION,

        // BO_END_TRANSMISSION,
        // BO_DELAY, 100
    };

    class AW862xx : public IIC_Guide
    {
    protected:
        enum class Reg
        {
            RO_DEVICE_ID = 0x64,

            WO_SRST = 0x00,
            RO_SYSST,
            RC_SYSINT,
            RW_SYSINTM,
            RO_SYSSST2,
            RW_PLAYCFG2 = 0x07,
            RW_PLAYCFG3,
            RW_PLAYCFG4,
            RW_WAVCFG1,
            RW_WAVCFG2,
            RW_WAVCFG3,
            RW_WAVCFG4,
            RW_WAVCFG5,
            RW_WAVCFG6,
            RW_WAVCFG7,
            RW_WAVCFG8,
            RW_WAVCFG9,
            RW_WAVCFG10,
            RW_WAVCFG11,
            RW_WAVCFG12,
            RW_WAVCFG13,
            RW_CONTCFG1 = 0x18,
            RW_CONTCFG2,
            RW_CONTCFG3,
            RW_CONTCFG4,
            RW_CONTCFG5,
            RW_CONTCFG6,
            RW_CONTCFG7,
            RW_CONTCFG8,
            RW_CONTCFG9,
            RW_CONTCFG10,
            RW_CONTCFG11,
            RO_CONTRD14 = 0x25,
            RO_CONTRD15,
            RO_CONTRD16,
            RO_CONTRD17,
            RW_RTPCFG1 = 0x2D,
            RW_RTPCFG2,
            RW_RTPCFG3,
            RW_RTPCFG4,
            RW_RTPCFG5,
            RW_RTPDATA,
            RW_TRGCFG1,
            RW_TRGCFG4 = 0x36,
            RW_TRGCFG7 = 0x39,
            RW_TRGCFG8,
            RW_GLBCFG2 = 0x3C,
            RW_GLBCFG4 = 0x3E,
            RO_GLBRD5,
            RW_RAMADDRH,
            RW_RAMADDRL,
            RW_RAMADATA,
            RW_SYSCTRL1,
            RW_SYSCTRL2,
            RW_SYSCTRL7 = 0x49,
            RW_PWMCFG1 = 0x4C,
            RW_PWMCFG2,
            RW_PWMCFG3,
            RW_PWMCFG4,
            RW_DETCFG1 = 0x51,
            RW_DETCFG2,
            RW_DET_RL,
            RW_DET_VBAT = 0x55,
            RW_DET_LO = 0x57,
            RW_TRIMCFG3 = 0x5A,
            RW_ANACFG8 = 0X77,
        };

    public:
        AW862xx(std::shared_ptr<Bus_IIC_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : IIC_Guide(bus, address, rst)
        {
        }

        bool begin(int64_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        int16_t iic_device_id(void) override;

        bool software_reset(void);
        float get_input_voltage(void);
        bool rtp_playback_waveform(const uint8_t *waveform_data, size_t length);
    };
}