/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:42:57
 * @LastEditTime: 2025-06-16 18:12:17
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define ES8311_DEVICE_DEFAULT_ADDRESS_1 0x18
#define ES8311_DEVICE_DEFAULT_ADDRESS_2 0x19
#define ES8311_DEVICE_DEFAULT_ADDRESS_2 0x19

    class Es8311 : public Iic_Guide, Iis_Guide
    {
    private:
        static constexpr uint16_t ES8311_DEVICE_ID = 0x8311;
        static constexpr uint8_t ES8311_INIT_SUCCESS_ID = 0x80;
        static constexpr uint8_t ES8311_CMD_TIMEOUT_COUNT = 100;

        enum class Cmd
        {
            RO_DEVICE_ID_START = 0xFD, // 连续读取两个返回芯片ID 0x8311

            RW_RESET_SERIAL_PORT_MODE_CONTROL = 0x00,
            RW_CLOCK_MANAGER_1,
            RW_CLOCK_MANAGER_2,
            RW_CLOCK_MANAGER_3,
            RW_CLOCK_MANAGER_4,
            RW_CLOCK_MANAGER_5,
            RW_CLOCK_MANAGER_6,
            RW_CLOCK_MANAGER_7,
            RW_CLOCK_MANAGER_8,
            RW_SDP_IN_FORMAT,
            RW_SDP_OUT_FORMAT,

            RW_POWER_UP_POWER_DOWN_CONTORL = 0x0D,
            RW_PGA_ADC_MODULATOR_POWER_CONTROL,
            RW_LOW_POWER_CONTROL,

            RW_DAC_POWER_CONTROL = 0x12,
            RW_OUTPUT_TO_HP_DRIVE_CONTROL,

            RW_ADC_DMIC_PGA_GAIN = 0x14,

            RW_ADC_GAIN_SCALE_UP = 0x16,
            RW_ADC_VOLUME,
            RW_ADC_ALC,

            RW_ADC_EQUALIZER_BYPASS = 0x1C,

            RW_DAC_VOLUME = 0x32,

            RW_DAC_RAMPRATE_EQBYPASS = 0x37,

            RW_ADC_DAC_CONTROL = 0x44,

        };

        // 时钟系数结构体
        struct Clock_Coeff
        {
            uint32_t mclk_multiple; // mclk倍速
            uint32_t sample_rate;   // 采样率
            uint8_t pre_div;        // 前分频器，范围从1到8
            uint8_t pre_multi;      // 前倍频器选择，0: 1x, 1: 2x, 2: 4x, 3: 8x
            uint8_t adc_div;        // adcclk分频器
            uint8_t dac_div;        // dacclk分频器
            uint8_t fs_mode;        // 双速或单速，=0, 单速, =1, 双速
            uint8_t lrck_h;         // adclrck分频器和daclrck分频器高字节
            uint8_t lrck_l;         // adclrck分频器和daclrck分频器低字节
            uint8_t bclk_div;       // sclk分频器
            uint8_t adc_osr;        // adc过采样率
            uint8_t dac_osr;        // dac过采样率
        };

        // 时钟分配系数列表
        static constexpr const Clock_Coeff _clock_coeff_list[] =
            {
                // 按照如下顺序查找系数并填充Clock_Coeff结构体
                // uint32_t mclk_multiple;     // mclk倍数
                // uint32_t sample_rate;     // 采样率
                // uint8_t pre_div;   // 前分频器，范围从1到8
                // uint8_t pre_multi; // 前倍频器选择，0: 1x, 1: 2x, 2: 4x, 3: 8x
                // uint8_t adc_div;   // adcclk分频器
                // uint8_t dac_div;   // dacclk分频器
                // uint8_t fs_mode;   // 双速或单速，=0, 单速, =1, 双速
                // uint8_t lrck_h;    // adclrck分频器和daclrck分频器高字节
                // uint8_t lrck_l;    // adclrck分频器和daclrck分频器低字节
                // uint8_t bclk_div;  // sclk分频器
                // uint8_t adc_osr;   // adc过采样率
                // uint8_t dac_osr;   // dac过采样率

                // 8khz
                {2304, 8000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x10},
                {2048, 8000, 0x08, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {1536, 8000, 0x06, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {1024, 8000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {768, 8000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {512, 8000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {384, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {192, 8000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 11.025khz
                {1024, 11025, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {512, 11025, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 11025, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 12khz
                {1024, 12000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {512, 12000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 12000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 16khz
                {1152, 16000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
                {1024, 16000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {768, 16000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {512, 16000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {384, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 16000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {192, 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {96, 16000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 22.05khz
                {512, 22050, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 22050, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {32, 22050, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 24khz
                {768, 24000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {512, 24000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 24000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 32khz
                {576, 32000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
                {512, 32000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {384, 32000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 32000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {192, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {96, 32000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {48, 32000, 0x03, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
                {32, 32000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 44.1khz
                {256, 44100, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {32, 44100, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 48khz
                {384, 48000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {256, 48000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {32, 48000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

                // 64khz
                {288, 64000, 0x03, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
                {256, 64000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {192, 64000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {96, 64000, 0x01, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
                {64, 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {48, 64000, 0x01, 0x03, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
                {32, 64000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {24, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
                {16, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

                // 88.2khz
                {128, 88200, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {32, 88200, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {16, 88200, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

                // 96khz
                {192, 96000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {128, 96000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {64, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {32, 96000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
                {16, 96000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
            };

        int32_t _rst;

    public:
        enum class Clock_Source
        {
            ADC_DAC_MCLK = 0,
            ADC_DAC_BCLK,
        };

        enum class Mic_Type
        {
            ANALOG_MIC = 0, // 模拟麦克风
            DIGITAL_MIC,    // 数字麦克风
        };

        enum class Mic_Input
        {
            NO_INPUT = 0,
            MIC1P_1N,
            MIC2P_2N,
            MIC1P_1N_MIC2P_2N,
        };

        enum class Vmid
        {
            POWER_DOWN = 0,
            START_UP_VMID_NORMAL_SPEED_CHARGE,
            NORMAL_VMID_OPERATION,
            START_UP_VMID_FAST_SPEED_CHARGE,
        };

        enum class Sdp
        {
            DAC = 0,
            ADC,
        };

        enum class Bits_Per_Sample
        {
            DATA_24BIT = 0,
            DATA_20BIT,
            DATA_18BIT,
            DATA_16BIT,
            DATA_32BIT,
        };

        enum class Adc_Offset_Freeze
        {
            // 冻结偏置（Offset Freeze）功能用于在ADC开始工作时捕获输入信号的初始直流偏置，并将其固定在整个采样过程中
            // 这意味着ADC会忽略输入信号中的任何缓慢变化的直流分量，只对交流成分进行量化
            FREEZE_OFFSET = 0,

            // 动态高通滤波器（Dynamic High-Pass Filter，HPF）是一种能够根据信号的变化自动调整其截止频率的滤波器
            // 在这种模式下，ADC会在信号稳定后应用一个高通滤波器，以去除低频噪声和直流偏置
            DYNAMIC_HPF,
        };

        enum class Adc_Gain
        {
            GAIN_0DB = 0,
            GAIN_6DB,
            GAIN_12DB,
            GAIN_18DB,
            GAIN_24DB,
            GAIN_30DB,
            GAIN_36DB,
            GAIN_42DB,
        };

        enum class Adc_Pga_Gain
        {
            GAIN_0DB = 0,
            GAIN_3DB,
            GAIN_6DB,
            GAIN_9DB,
            GAIN_12DB,
            GAIN_15DB,
            GAIN_18DB,
            GAIN_21DB,
            GAIN_24DB,
            GAIN_27DB,
            GAIN_30DB,
        };

        enum class Serial_Port_Mode
        {
            SLAVE,
            MASTER,
        };

        struct Power_Status
        {
            //[true]：启动，[false]：关闭，控制设置
            struct
            {
                bool analog_circuits = false;               // 模拟电路
                bool analog_bias_circuits = false;          // 模拟偏置电路
                bool analog_adc_bias_circuits = false;      // 模拟ADC偏置电路
                bool analog_adc_reference_circuits = false; // 模拟ADC参考电路
                bool analog_dac_reference_circuit = false;  // 模拟DAC参考电路
                bool internal_reference_circuits = false;   // 内部参考电路
            } contorl;

            Vmid vmid = Vmid::POWER_DOWN;
        };

        // 低功耗状态
        //[true]：启动低功耗，[false]：关闭低功耗，控制设置
        struct Low_Power_Status
        {
            bool dac = false;
            bool pga = false;
            bool pga_output = false;
            bool adc = false;
            bool adc_reference = false;
            bool dac_reference = false;
            bool flash = false; // 闪存
            bool int1 = false;  // 中断
        };

        Clock_Coeff _clock_coeff;

        Es8311(std::shared_ptr<Bus_Iic_Guide> iic_bus, std::shared_ptr<Bus_Iis_Guide> iis_bus, int16_t iic_address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(iic_bus, iic_address), Iis_Guide(iis_bus), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width) override;

        uint16_t get_device_id(void);

        /**
         * @brief 软件复位 ，在编解码器准备好待机或睡眠时，将所有复位位设置为“1”，并将CSM_ON清除为“0”，以最大限度地降低功耗
         * @param enalbe [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-11 17:42:32
         */
        bool software_reset(bool enable);

        /**
         * @brief 设置主时钟源
         * @param clock 使用Clock_Source::配置
         * @return
         * @Date 2025-03-12 09:12:58
         */
        bool set_master_clock_source(Clock_Source clock);

        /**
         * @brief 设置时钟启动
         * @param clock 使用Clock_Source::配置，时钟源
         * @param enalbe [true]：启动，[false]：关闭
         * @param invert [true]：反转，[false]：不反转
         * @return
         * @Date 2025-03-12 09:14:23
         */
        bool set_clock(Clock_Source clock, bool enalbe, bool invert = false);

        /**
         * @brief 设置DAC的音量
         * @param volume 值范围0~255，增益范围 -95.5dB 到 +32dB，步进0.5dB，0dB为191
         * @return
         * @Date 2025-03-12 10:16:50
         */
        bool set_dac_volume(uint8_t volume);

        /**
         * @brief 设置ADC的音量，如果开启了自动音量控制（调用函数set_adc_auto_volume_control()）那么此设置音量函数无效
         * @param volume 值范围0~255，增益范围 -95.5dB 到 +32dB，步进0.5dB，0dB为191
         * @return
         * @Date 2025-03-12 10:16:50
         */
        bool set_adc_volume(uint8_t volume);

        /**
         * @brief 设置ADC自动控制音量
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-12 10:26:52
         */
        bool set_adc_auto_volume_control(bool enable);

        /**
         * @brief 设置MIC1P引脚的麦克风使用类型
         * @param type 使用Mic_Type::配置
         * @return
         * @Date 2025-03-12 10:32:55
         */
        bool set_mic(Mic_Type type, Mic_Input input);

        /**
         * @brief 设置电源状态
         * @param status 使用 Power_Status 结构体配置
         * @return
         * @Date 2025-03-12 10:52:49
         */
        bool set_power_status(Power_Status status);

        /**
         * @brief 设置低功耗电压状态，在正常模式下，如果设置了低功率控制，功耗将显著降低，音频性能，例如THD+N和信噪比，将略有下降
         * @param status 使用 Low_Power_Status 结构体配置
         * @return
         * @Date 2025-03-12 11:02:38
         */
        bool set_low_power_status(Low_Power_Status status);

        /**
         * @brief 搜索时钟系数
         * @param mclk_multiple mclk倍速
         * @param sample_rate_hz 采样率
         * @param *library 需要查找的库，使用Clock_Coeff类型的库写入
         * @param library_length 查找的库的长度
         * @param search_index 搜索引索
         * @return
         * @Date 2025-03-13 10:56:10
         */
        bool search_clock_coeff(uint16_t mclk_multiple, uint32_t sample_rate_hz,
                                const Clock_Coeff *library, size_t library_length, size_t *search_index = nullptr);

        /**
         * @brief 通过所输入的mclk_multiple和sample_rate_hz参数查找_clock_coeff_list列表里的最佳系数值来设置时钟系数
         * @param mclk_multiple mclk倍速
         * @param sample_rate_hz 采样率
         * @return
         * @Date 2025-03-12 16:58:34
         */
        bool set_clock_coeff(uint16_t mclk_multiple, uint32_t sample_rate_hz);

        /**
         * @brief 设置SDP字节长度
         * @param dsp 使用Sdp::配置
         * @param length 使用Bits_Per_Sample::配置
         * @return
         * @Date 2025-03-13 11:42:42
         */
        bool set_sdp_data_bit_length(Sdp sdp, Bits_Per_Sample length);

        /**
         * @brief 设置PGA电源
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-13 11:47:36
         */
        bool set_pga_power(bool enable);

        /**
         * @brief 设置ADC电源
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-13 11:47:36
         */
        bool set_adc_power(bool enable);

        /**
         * @brief 设置DAC电源
         * @param enable
         * @return
         * @Date 2025-03-13 11:58:38
         */
        bool set_dac_power(bool enable);

        /**
         * @brief 设置输出到HP驱动器
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-13 12:04:13
         */
        bool set_output_to_hp_drive(bool enable);

        /**
         * @brief 设置ADC处理信号中的直流偏置和高频噪声的模式
         * @param offset_freeze 使用Adc_Offset_Freeze::配置
         * @return
         * @Date 2025-03-13 12:12:17
         */
        bool set_adc_offset_freeze(Adc_Offset_Freeze offset_freeze);

        /**
         * @brief 设置ADC的HPF第二系数
         * @param coeff 值范围0~31
         * @return
         * @Date 2025-03-13 13:33:08
         */
        bool set_adc_hpf_stage2_coeff(uint8_t coeff);

        /**
         * @brief 设置DAC的均衡器（EQ）
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-13 13:38:17
         */
        bool set_dac_equalizer(bool enable);

        /**
         * @brief 读取数据
         * @param *data 数据指针
         * @param byte 字节长度
         * @return size_t 实际读取到的字节数
         * @Date 2025-03-13 14:02:47
         */
        size_t read_data(void *data, size_t byte);

        /**
         * @brief 写入数据
         * @param *data 数据指针
         * @param byte 字节长度
         * @return size_t 实际写入的字节数
         * @Date 2025-03-13 14:02:47
         */
        size_t write_data(const void *data, size_t byte);

        /**
         * @brief ADC增益
         * @param gain 使用Adc_Gain::配置
         * @return
         * @Date 2025-03-13 16:53:47
         */
        bool set_adc_gain(Adc_Gain gain);

        /**
         * @brief 设置ADC数据全部输出到DAC上
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-13 16:58:44
         */
        bool set_adc_data_to_dac(bool enable);

        /**
         * @brief ADC的PGA增益
         * @param gain 使用Adc_Pga_Gain::配置
         * @return
         * @Date 2025-03-14 11:03:35
         */
        bool set_adc_pga_gain(Adc_Pga_Gain gain);

        /**
         * @brief 设置串行模式
         * @param mode 使用Serial_Port_Mode::配置
         * @return
         * @Date 2025-03-29 16:13:18
         */
        bool set_serial_port_mode(Serial_Port_Mode mode);
    };
}