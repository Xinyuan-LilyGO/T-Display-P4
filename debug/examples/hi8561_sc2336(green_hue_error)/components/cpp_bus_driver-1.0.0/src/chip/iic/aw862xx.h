/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-03-19 09:43:44
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"
#include "../../resources/haptic_chip_waveform_library.h"

namespace Cpp_Bus_Driver
{
#define AW862XX_DEVICE_DEFAULT_ADDRESS 0x58

    class Aw862xx : public Iic_Guide
    {
    private:
        enum class Cmd
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

        int32_t _rst;

    public:
        enum class Play_Mode
        {
            RAM = 0,
            RTP,
            CONT,
            NO_PLAY,
        };

        enum class Global_Status
        {
            FALSE = -1,

            STANDBY = 0,
            CONT = 6,
            RAM,
            RTP,
            TRIG,
            BRAKE = 11,
        };

        enum class Sample_Rate
        {
            RATE_24KHZ = 0,
            RATE_48KHZ,
            RATE_12KHZ,
        };

        enum class D2s_Gain
        {
            GAIN_1 = 0,
            GAIN_2,
            GAIN_4,
            GAIN_5,
            GAIN_8,
            GAIN_10,
            GAIN_20,
            GAIN_40,
        };

        enum class Force_Mode
        {
            ACTIVE = 0, // 激活模式
            STANDBY,    // 待机模式
        };

        struct System_Status
        {
            bool uvls_flag = false;             // VDD电压低于UV电压标志
            bool rtp_fifo_empty = true;         // RTP模式FIFO为空标志
            bool rtp_fifo_full = false;         // RTP模式FIFO为满标志
            bool over_current_flag = false;     // 过流标志
            bool over_temperature_flag = false; // 过温标志
            bool playback_flag = false;         // 回放标志
        };

        uint32_t _f0_value = 1700;

        Aw862xx(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 软件复位
         * @return
         * @Date 2025-03-07 17:31:30
         */
        bool software_reset(void);

        /**
         * @brief 获取输入电压
         * @return
         * @Date 2025-03-07 17:31:39
         */
        float get_input_voltage(void);

        /**
         * @brief 设置GO TRIG的波形播放模式，RAM/RTP/CONT/NO PLAY 模式
         * @param mode 使用Play_Mode::配置
         * @return
         * @Date 2025-02-28 10:37:12
         */
        bool set_play_mode(Play_Mode mode);

        /**
         * @brief RAM/RTP/CONT 模式播放触发位，当设置为 1 时，芯片将播放其中一种播放模式
         * @return
         * @Date 2025-02-28 10:41:41
         */
        bool set_go_flag(void);

        /**
         * @brief 获取全局状态
         * @return
         * @Date 2025-02-28 10:47:13
         */
        Global_Status get_global_status(void);

        /**
         * @brief RTP模式播放waveform库文件
         * @param *waveform_data 波形数据指针
         * @param length 波形输出长度
         * @return
         * @Date 2025-03-07 17:31:52
         */
        bool run_rtp_playback_waveform(const uint8_t *waveform_data, size_t length);

        /**
         * @brief 设置播放waveform库文件的采样率
         * @param rate
         * @return
         * @Date 2025-03-07 17:50:35
         */
        bool set_waveform_data_sample_rate(Sample_Rate rate);

        /**
         * @brief 设置在播放的时候改变增益
         * @param enable [true]：在播放的时候改变增益，[false]：在播放的时候不改变增益
         * @return
         * @Date 2025-03-07 17:57:48
         */
        bool set_playing_changed_gain_bypass(bool enable);

        /**
         * @brief 设置D2S增益
         * @param gain 使用D2s_Gain::配置
         * @return
         * @Date 2025-03-07 18:05:22
         */
        bool set_d2s_gain(D2s_Gain gain);

        /**
         * @brief 调整LRA（线性振动马达）的频率，以适应LRA共振频率的偏差
         * @param freq_hz 值范围：0~63
         * @return
         * @Date 2025-03-10 09:33:31
         */
        bool set_lra_osc_frequency(uint8_t freq_hz);

        /**
         * @brief 设置是否启动f0校验模式
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-10 09:27:41
         */
        bool set_f0_detection_mode(bool enable);

        /**
         * @brief 设置振动马达开关
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-10 09:40:13
         */
        bool set_track_switch(bool enable);

        /**
         * @brief 设置在RTP/RAM/CONT播放模式停止后启用自动制动（当设置为1时）
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-10 09:45:50
         */
        bool set_auto_brake_stop(bool enable);

        /**
         * @brief 用于控制第1个连续驱动器的输出电压级别，它通过设置寄存器的值来调节输出电压
         * @param level 值范围：0~127
         * @return
         * @Date 2025-03-10 09:52:04
         */
        bool set_cont_drive_1_level(uint8_t level);

        /**
         * @brief 用于控制第2个连续驱动器的输出电压级别，它通过设置寄存器的值来调节输出电压
         * @param level 值范围：0~127
         * @return
         * @Date 2025-03-10 09:52:04
         */
        bool set_cont_drive_2_level(uint8_t level);

        /**
         * @brief 设置第1次连续驱动的半循环次数
         * @param times 值范围：0~255
         * @return
         * @Date 2025-03-10 10:12:21
         */
        bool set_cont_drive_1_times(uint8_t times);

        /**
         * @brief 设置第2次连续驱动的半循环次数
         * @param times 值范围：0~255
         * @return
         * @Date 2025-03-10 10:12:21
         */
        bool set_cont_drive_2_times(uint8_t times);

        /**
         * @brief 设置跟踪余量值，跟踪余量值越小，跟踪精度越高
         * @param value 值范围：0~255
         * @return
         * @Date 2025-03-10 10:19:55
         */
        bool set_cont_track_margin(uint8_t value);

        /**
         * @brief 设置制动器的半周期驱动时间（code/48K s），该值必须小于一半F0的循环时间，
         * 建议将DRV_WIDTH配置为（24k/F0）-8-TRACK_MARGIN-BRK_GAIN
         * @param value 值范围：0~255
         * @return
         * @Date 2025-03-10 10:27:30
         */
        bool set_cont_drive_width(uint8_t value);

        /**
         * @brief 获取f0检测的值
         * @return
         * @Date 2025-03-10 11:13:49
         */
        uint32_t get_f0_detection(void);

        /**
         * @brief 输入f0的值开始进行f0校准
         * @param f0_value get_f0_detection()函数获取的f0的值
         * @return
         * @Date 2025-03-10 11:17:27
         */
        bool set_f0_calibrate(uint32_t f0_value);

        /**
         * @brief 获取系统状态
         * @param status 使用System_Status结构体配置
         * @return
         * @Date 2025-03-10 14:01:45
         */
        bool get_system_status(System_Status &status);

        /**
         * @brief 设置数字时钟启动
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-10 14:30:42
         */
        bool set_clock(bool enable);

        /**
         * @brief 设置在RTP、RAM、TRIG模式下的增益
         * @param gain 值范围0~255
         * @return
         * @Date 2025-03-10 15:28:05
         */
        bool set_rrt_mode_gain(uint8_t gain);

        /**
         * @brief 初始化RAM模式
         * @param *waveform_data 波形数据指针
         * @param length 波形输出长度
         * @return
         * @Date 2025-03-10 16:20:23
         */
        bool init_ram_mode(const uint8_t *waveform_data, size_t length);

        /**
         * @brief 设置RAM模式播放waveform数据
         * @param waveform_sequence_number 值范围0~127，波形的序列号（该序列号为波形数据库里的序列号）
         * @param waveform_playback_count 值范围0~15，波形的回放次数，当设置为15的时候为无限循环播放，当设置为无限循环播放的时候需要使用函数xxx进行停止播放
         * @param gain 值范围0~255，配置增益
         * @param auto_brake [true]：启动，[false]：关闭，自动制动
         * @param gain_bypass [true]：在播放的时候改变增益，[false]：在播放的时候不改变增益，设置在播放的时候改变增益
         * @return
         * @Date 2025-03-10 15:35:46
         */
        bool run_ram_playback_waveform(uint8_t waveform_sequence_number, uint8_t waveform_playback_count,
                                       uint8_t gain = 127, bool auto_brake = true, bool gain_bypass = true);

        /**
         * @brief 设置停止标志位，设置该标志将停止当前的振动模式
         * @return
         * @Date 2025-03-10 17:40:58
         */
        bool set_stop_flag(void);

        /**
         * @brief 设置强制进入的模式
         * @param mode 使用Force_Mode::配置
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-03-10 17:46:30
         */
        bool set_force_enter_mode(Force_Mode mode, bool enable);

        /**
         * @brief RAM模式播放停止
         * @return
         * @Date 2025-03-10 17:55:04
         */
        bool stop_ram_playback_waveform(void);
    };
}