/*
 * @Description: SX1262
            SX1262 最大SPI传输速度为16MHz
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-02-07 17:09:15
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

// #define SX1262_DEVICE_DEFAULT_ADDRESS 0x55

namespace Cpp_Bus_Driver
{
    class Sx126x : public SPI_Guide
    {
    public:
        enum class Chip_Type
        {
            SX1262 = 0x00,
            SX1261,
        };

    private:
#define SX126X_DEVICE_DEFAULT_BUSY_WAIT_TIMEOUT_MS 100
#define SX126X_DEVICE_DEFAULT_READ_ID 0xC8

#define SX1262_DEVICE_DEFAULT_MISO_WAIT_CODE 0xAA // MISO等待码，读取操作等待时返回该值

        Chip_Type _chip_type;

        int32_t _rst;

        int32_t _busy = DEFAULT_CPP_BUS_DRIVER_VALUE;
        bool (*_busy_wait_callback)(void) = nullptr;

        size_t _watchdog_busy = 0;

    protected:
        enum class Cmd
        {
            // 用于读写寄存器命令
            WO_WRITE_REGISTER = 0x0D,
            WO_READ_REGISTER = 0x1D,

            WO_CLEAR_IRQ_STATUS = 0x02,
            WO_SET_DIO_IRQ_PARAMS = 0x08,
            RO_GET_PACKET_TYPE = 0x11,
            RO_GET_IRQ_STATUS,
            RO_GET_RX_BUFFER_STATUS,
            RO_READ_BUFFER = 0x1E,
            WO_SET_STANDBY = 0x80,
            WO_SET_RX = 0x82,
            WO_SET_RF_FREQUENCY = 0x86,
            WO_SET_CAD_PARAMS = 0x88,
            WO_CALIBRATE,
            WO_SET_PACKET_TYPE = 0x8A,
            WO_SET_MODULATION_PARAMS,
            WO_SET_PACKET_PARAMS,
            WO_SET_TX_PARAMS = 0x8E,
            WO_SET_BUFFER_BASE_ADDRESS,
            WO_SET_RX_TX_FALLBACK_MODE = 0x93,
            WO_SET_PA_CONFIG = 0x95,
            WO_SET_REGULATOR_MODE,
            WO_SET_DIO3_AS_TCXO_CTRL,
            WO_CALIBRATE_IMAGE,
            WO_SET_DIO2_AS_RF_SWITCH_CTRL = 0x9D,
            RO_GET_STATUS = 0xC0,

        };

        // 访问寄存器需要通过前置读写命令来访问
        enum class Reg
        {
            RO_DEVICE_ID = 0x08D8, // 初次读取该寄存器将返回0xC8,初始化以后需要改成0xDE

            RW_IQ_POLARITY_SETUP = 0x0736,
            RW_LORA_SYNC_WORD_MSB = 0x0740,
            RW_LORA_SYNC_WORD_LSB,
            RW_TX_CLAMP_CONFIG = 0x08D8,
            RW_OCP_CONFIGURATION = 0x08E7,
        };

        // static const uint16_t Init_List[];

    public:
        enum class Stdby_Config
        {
            STDBY_RC = 0, // 13 MHz Resistance-Capacitance Oscillator
            STDBY_XOSC,   // XTAL 32MHz
        };

        enum class Dio3_Tcxo_Voltage
        {
            OUTPUT_1600_MV = 0x00,
            OUTPUT_1700_MV,
            OUTPUT_1800_MV,
            OUTPUT_2200_MV,
            OUTPUT_2400_MV,
            OUTPUT_2700_MV,
            OUTPUT_3000_MV,
            OUTPUT_3300_MV,
        };

        enum class Packet_Type
        {
            GFSK_OR_FSK = 0x00,
            LORA = 0x01,
            LR_FHSS = 0x03,
        };

        // 在发送（Tx）或接收（Rx）操作之后，无线电会进入的模式。
        enum class Fallback_Mode
        {
            STDBY_RC = 0x20,
            STDBY_XOSC = 0x30,
            FS = 0x40,
        };

        // 设置在多少个符号（symbol）的时间内执行信道活动检测
        enum class Cad_Symbol_Num
        {
            ON_1_SYMB = 0x00,
            ON_2_SYMB,
            ON_4_SYMB,
            ON_8_SYMB,
            ON_16_SYMB,
        };

        enum class Cad_Exit_Mode
        {
            // 芯片在 LoRa 模式下执行信道活动检测（CAD）操作，一旦操作完成，无论信道上是否有活动，
            // 芯片都会返回到 STDBY_RC （待机模式，使用内部 RC 振荡器）模式
            ONLY = 0x00,

            // 芯片执行信道活动检测（CAD）操作，如果检测到活动，芯片将保持在接收（RX）模式，
            // 直到检测到数据包或计时器达到由 cadTimeout * 15.625μs 定义的超时时间。
            RX,
        };

        enum class Regulator_Mode
        {
            LDO = 0x00,   // 仅使用LDO用于所有模式
            LDO_AND_DCDC, // 在STBY_XOSC、FS、RX和TX模式中使用DC-DC+LDO
        };

        enum class Dio2_Mode
        {
            IRQ = 0x00, // DIO2被用作IRQ
            RF_SWITCH,  // 控制一个射频开关，这种情况：在睡眠模式、待机接收模式、待机外部振荡器模式、频率合成模式和接收模式下，DIO2 = 0；在发射模式下，DIO2 = 1
        };

        // enum class Pa_Device
        // {
        //     SX1262 = 0x00, // 选择SX1262
        //     SX1261,        // 选择SX1261
        // };

        enum class Ramp_Time
        {
            RAMP_10_US = 0x00,
            RAMP_20_US,
            RAMP_40_US,
            RAMP_80_US,
            RAMP_200_US,
            RAMP_800_US,
            RAMP_1700_US,
            RAMP_3400_US,
        };

        enum class Img_Cal_Freq
        {
            FREQ_430_440_MHZ,
            FREQ_470_510_MHZ,
            FREQ_779_787_MHZ,
            FREQ_863_870_MHZ,
            FREQ_902_928_MHZ,
        };

        Sx126x(std::shared_ptr<Bus_SPI_Guide> bus, Chip_Type chip_type, int32_t busy,
               int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : SPI_Guide(bus, cs), _chip_type(chip_type), _rst(rst), _busy(busy)
        {
        }

        Sx126x(std::shared_ptr<Bus_SPI_Guide> bus, Chip_Type chip_type, bool (*busy_wait_callback)(void),
               int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : SPI_Guide(bus, cs), _chip_type(chip_type), _rst(rst), _busy_wait_callback(busy_wait_callback)
        {
        }

        bool begin(int32_t frequency = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t iic_device_id(void);

        bool check_busy(void);

        /**
         * @brief 配置功耗模式，应用程序如果对时间要求严格需要切换到 STDBY_XOSC 模式，使用 STDBY_XOSC 前，
         * 需要在 STDBY_RC 模式配置 set_regulator_mode() 为 LDO_AND_DCDC 模式再切换到 STDBY_XOSC 模式
         * @param config 使用 Stdby_Config:: 配置
         * @return
         * @Date 2025-01-17 16:49:59
         */
        bool set_standby(Stdby_Config config);

        /**
         * @brief 配置由DIO3控制的外部TCXO参考电压
         * @param voltage 使用 Dio3_Tcxo_Voltage:: 配置
         * @param time_out_us （0 ~ 16777215）超时时间，Delay duration = Delay(23:0) *15.625 µs，每一步等于15.625 µs
         * @return
         * @Date 2025-01-22 12:04:01
         */
        bool set_dio3_as_tcxo_ctrl(Dio3_Tcxo_Voltage voltage, uint32_t time_out_us);

        /**
         * @brief 设置优化功率放大器（PA）的钳位阈值（SX1262手册第15.2.2节）
         * @param enable [true]：启动，[false]：关闭
         * @return
         * @Date 2025-01-17 16:58:01
         */
        bool set_tx_clamp(bool enable);

        /**
         * @brief 此命令为数据缓冲区设置基地址，适用于所有操作模式下的数据包处理操作，包括发送（TX）和接收（RX）模式
         * @param tx_base_address tx 基地址
         * @param rx_base_address rx 基地址
         * @return
         * @Date 2025-01-17 17:08:57
         */
        bool set_buffer_base_address(uint8_t tx_base_address, uint8_t rx_base_address);

        /**
         * @brief 设置传输数据包类型 其中 SX1261 第一个模式为GFSK，SX1262第一个模式为FSK
         * @param type 使用 Packet_Type:: 配置
         * @return
         * @Date 2025-01-17 17:29:17
         */
        bool set_packet_type(Packet_Type type);

        /**
         * @brief 芯片在成功发送数据包或成功接收数据包后进入的模式
         * @param mode 使用 Fallback_Mode:: 配置
         * @return
         * @Date 2025-01-17 17:34:32
         */
        bool set_rx_tx_fallback_mode(Fallback_Mode mode);

        /**
         * @brief 设置信道活动检测（CAD）参数
         * @param num 使用 Cad_Symbol_Num:: 配置
         * @param cad_det_peak LoRa 调制解调器在尝试与实际 LoRa 前导码符号进行相关时的灵敏度，设置取决于 LoRa 的扩频因子（Spreading Factor）和带宽（Bandwidth），同时也取决于用于验证检测的符号数量
         * @param cad_det_min LoRa 调制解调器在尝试与实际 LoRa 前导码符号进行相关时的灵敏度，设置取决于 LoRa 的扩频因子（Spreading Factor）和带宽（Bandwidth），同时也取决于用于验证检测的符号数量
         * @param exit_mode 在 CAD（信道活动检测）操作完成后要执行的操作。此参数是可选的
         * @param time_out_us 仅在 cadExitMode = CAD_RX 时执行 CAD 时使用，在这里，cadTimeout 表示设备在成功完成 CAD 后保持在接收模式（Rx）的时间，
         * 接收超时时间（Rx Timeout）= cadTimeout * 15.625μs，最大值为 16777215 （0xFFFFFF）
         * @return
         * @Date 2025-01-17 18:07:39
         */
        bool set_cad_params(Cad_Symbol_Num num, uint8_t cad_det_peak, uint8_t cad_det_min, Cad_Exit_Mode exit_mode, uint32_t time_out_us);

        /**
         * @brief 用于清除IRQ寄存器中的一个IRQ标志,此函数通过将ClearIrqParam中与待清除的IRQ标志位位置相对应的位设置为1，来清除IRQ寄存器中的IRQ标志,
         * 例如，如果ClearIrqParam的第0位被设置为1，那么IRQ寄存器中第0位的IRQ标志将被清除,如果一个DIO（数字输入/输出）被映射到单一的IRQ源，
         * 当IRQ寄存器中对应的位被清除时，该DIO也会被清除,如果DIO被设置为0且与多个IRQ源相关联，那么只有当IRQ寄存器中所有映射到该DIO的位都被清除时，DIO才会被设置为0
         * @param clear_irq_param 需要清除的标志，默认清除全部中断
         * @return
         * @Date 2025-01-18 09:27:58
         */
        bool clear_irq_status(uint16_t clear_irq_param = 0B0100001111111111);

        // Table 13-29: IRQ Registers
        /*
        Bit | IRQ              | Description                              | Modulation
        ----|------------------|------------------------------------------|-----------
        0   | TxDone           | Packet transmission completed            | All
        1   | RxDone           | Packet received                          | All
        2   | PreambleDetected | Preamble detected                        | All
        3   | SyncWordValid    | Valid sync word detected                 | FSK
        4   | HeaderValid      | Valid LoRa header received               | LoRa&reg;
        5   | HeaderErr        | LoRa header CRC error                    | LoRa&reg;
        6   | CrcErr           | Wrong CRC received                       | All
        7   | CadDone          | Channel activity detection finished       | LoRa&reg;
        8   | CadDetected      | Channel activity detected                | LoRa&reg;
        9   | Timeout          | Rx or Tx timeout                         | All
        10-13| -                | RFU                                      | -
        14  | LrFhssHop        | Asserted at each hop, in Long Range FHSS, after the PA has ramped-up again | LR-FHSS
        15  | -                | RFU                                      | -
        */

        /**
         * @brief 设置中断请求（IRQ）标志（中断标志参考SX1262手册表格 13-29: IRQ Registers）
         * @param irq_mask IrqMask用于屏蔽或解除屏蔽可由设备触发的中断请求（IRQ），默认情况下，所有IRQ都被屏蔽（所有位为‘0’），
         * 用户可以通过将相应的掩码设置为‘1’来逐个（或同时多个）启用它们。
         * @param dio1_mask 使用 IRQ_Registers:: 配置，当中断发生时，如果 DIO1Mask 和 IrqMask 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
         * 并且DIO1Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，并同时出现在DIO1上，一个IRQ可以映射到所有DIO，
         * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
         * @param dio2_mask 使用 IRQ_Registers:: 配置，当中断发生时，如果 DIO2Mask 和 IrqMask 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
         * 并且DIO2Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，并同时出现在DIO1上，一个IRQ可以映射到所有DIO，
         * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
         * @param dio3_mask 使用 IRQ_Registers:: 配置，当中断发生时，如果 DIO3Mask 和 IrqMask 中的相应位都被设置为1，则会触发DIO的设置。例如，如果IrqMask的第0位被设置为1，
         * 并且DIO3Mask的第0位也被设置为1，那么IRQ源TxDone的上升沿将被记录在IRQ寄存器中，并同时出现在DIO1上，一个IRQ可以映射到所有DIO，
         * 一个DIO也可以映射到所有IRQ（进行“或”操作），但某些IRQ源仅在特定的操作模式和帧中可用
         * @return
         * @Date 2025-01-18 09:48:15
         */
        bool set_dio_irq_params(uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask, uint16_t dio3_mask);

        /**
         * @brief 在电源启动时，无线电设备会执行RC64k、RC13M、PLL和ADC的校准，然而，从STDBY_RC模式开始，可以随时启动一个或多个模块的校准，
         * 校准功能会启动由calibParam定义的模块的校准，如果所有模块都进行校准，总校准时间为3.5毫秒，校准必须在STDBY_RC模式下启动，
         * 并且在校准过程中BUSY引脚将保持高电平，BUSY引脚的下降沿表示校准过程结束
         * @param calib_param 需要校准的参数设置
         * @return
         * @Date 2025-01-18 13:59:03
         */
        bool calibrate(uint8_t calib_param);

        /**
         * @brief 返回设备的状态，主机可以直接获取芯片状态
         * @return
         * @Date 2025-01-18 17:04:10
         */
        uint8_t get_status(void);

        bool check_status(void);

        /**
         * @brief 获取当前使用的数据包类型
         * @return
         * @Date 2025-01-21 16:28:58
         */
        uint8_t get_packet_type(void);

        /**
         * @brief 默认情况下，只使用LDO（低压差线性稳压器），这在成本敏感的应用中非常有用，因为DC-DC转换器所需的额外元件会增加成本，
         * 仅使用线性稳压器意味着接收或发送电流几乎会加倍，此功能允许指定是使用DC-DC还是LDO来进行电源调节
         * @param mode 使用 Regulator_Mode:: 配置
         * @return
         * @Date 2025-01-22 13:44:23
         */
        bool set_regulator_mode(Regulator_Mode mode);

        /**
         * @brief 设置电流限制
         * @param current （0mA ~ 140mA）步长为2.5mA，有越界校正
         * @return
         * @Date 2025-01-22 14:09:23
         */
        bool set_current_limit(float current);

        /**
         * @brief 获取电流限制
         * @return
         * @Date 2025-01-22 14:45:10
         */
        uint8_t get_current_limit(void);

        /**
         * @brief 配置DIO2的模式功能，IRQ或者控制外部RF开关
         * @param mode 使用 Dio2_Mode:: 配置
         * @return
         * @Date 2025-01-22 14:47:07
         */
        bool set_dio2_as_rf_switch_ctrl(Dio2_Mode mode);

        /**
         * @brief 选择不同设备要使用的功率放大器（PA）及其配置
         * @param pa_duty_cycle 控制着两个功率放大器（SX1261 和 SX1262）的占空比（导通角），最大输出功率、功耗和谐波都会随着 paDutyCycle 的改变而显著变化，
         * 实现功率放大器最佳效率的推荐设置请参考手册13.1.14点，改变 paDutyCycle 会影响谐波中的功率分布，因此应根据给定的匹配网络进行选择和调整
         * @param hp_max 选择 SX1262 中功率放大器的大小，此值对 SX1261 没有影响，通过减小 hpMax 的值可以降低最大输出功率，有效范围在 0x00 到 0x07 之间，
         * 0x07 是 SX1262 实现 +22 dBm 输出功率的最大支持值，将 hpMax 增加到 0x07 以上可能会导致设备过早老化，
         * 或在极端温度下使用时可能损坏设备
         * @return
         * @Date 2025-01-22 16:26:38
         */
        bool set_pa_config(uint8_t pa_duty_cycle, uint8_t hp_max);

        /**
         * @brief 设置TX（发送）输出功率，并通过使用参数 ramp_time 来设置 TX 上升时间，此命令适用于所有选定的协议
         * @param power 输出功率定义为以 dBm 为单位的功率，范围如下：
         * 如果选择低功率 PA，则范围为 -17 (0xEF) 到 +14 (0x0E) dBm，步长为 1 dB，
         * 如果选择高功率 PA，则范围为 -9 (0xF7) 到 +22 (0x16) dBm，步长为 1 dB，
         * 通过命令 set_pa_config 和参数 device 来选择高功率 PA 或低功率 PA，默认情况下，设置为低功率 PA 和 +14 dBm
         * @param ramp_time 使用 Ramp_Time:: 配置
         * @return
         * @Date 2025-01-22 16:46:55
         */
        bool set_tx_params(uint8_t power, Ramp_Time ramp_time);

        // 扩频因子
        enum class Sf
        {
            SF5 = 0x05,
            SF6,
            SF7,
            SF8,
            SF9,
            SF10,
            SF11,
            SF12,
        };

        // Lora带宽
        enum class Bw
        {
            BW_7810Hz = 0x00,
            BW_15630Hz,
            BW_31250Hz,
            BW_62500Hz,
            BW_125000Hz,
            BW_250000Hz,
            BW_500000Hz,
            BW_10420Hz = 0x08,
            BW_20830Hz,
            BW_41670Hz,
        };

        // 纠错编码级别
        enum class Cr
        {
            CR_4_5 = 0x01,
            CR_4_6,
            CR_4_7,
            CR_4_8,
        };

        // 低数据速率优化
        enum class Ldro
        {
            LDRO_OFF = 0x00,
            LDRO_ON,
        };

        enum class Header_Type
        {
            VARIABLE_LENGTH_PACKET = 0x00, // explicit header
            FIXED_LENGTH_PACKET,           // implicit header
        };

        enum class Crc_Type
        {
            OFF = 0x00, // 关闭
            ON,         // 打开
        };

        enum class Invert_Iq
        {
            STANDARD_IQ_SETUP = 0x00, // 使用标准的IQ极性
            INVERTED_IQ_SETUP,        // 使用反转的IQ极性
        };
        /**
         * @brief 设置Lora的同步字，每个LoRa数据包的开始部分都包含一个同步字，接收机通过匹配这个同步字来确认数据包的有效性，
         * 如果接收机发现接收到的数据包中的同步字与预设值一致，则认为这是一个有效的数据包，并继续解码后续的数据
         * @param sync_word 它可以设置为任何值，但有些值无法与SX126X系列等其他LoRa设备互操作，有些值会降低您的接收灵敏度，
         * 除非你有如何检查接收灵敏度和可靠性极限的经验和知识，否则请使用标准值，0x3444 为公共网络，0x1424 为专用网络
         * @return
         * @Date 2025-01-22 09:15:24
         */
        bool set_lora_sync_word(uint16_t sync_word);

        /**
         * @brief 获取当前设置的同步字
         * @return
         * @Date 2025-01-22 09:30:25
         */
        uint16_t get_lora_sync_word(void);

        /**
         * @brief 设置Lora模式反转 IQ 配置 （SX1262手册第15.4.2节）
         * @param iq 使用 Invert_Iq:: 配置
         * @return
         * @Date 2025-01-22 09:58:32
         */
        bool set_lora_inverted_iq(Invert_Iq iq);

        /**
         * @brief 设置数据包处理块的参数
         * @param preamble_length 前导长度是一个16位的值，表示无线电将发送的LoRa符号数量
         * @param header_type 使用 Header_Type:: 配置，当字节 header_type 的值为 0x00 时，有效载荷长度、编码率和头CRC将被添加到LoRa头部，并传输给接收器
         * @param payload_length 数据包的有效载荷（即实际传输的数据）的长度，这个参数通常用于指示数据包中有效数据的字节数，
         * 在进行数据传输时，发送端会设置这个长度，而接收端则根据这个长度来解析接收到的数据包
         * @param crc_type 使用 Crc_Type:: 配置
         * @param iq 使用 Invert_Iq:: 配置
         * @return
         * @Date 2025-01-22 11:44:47
         */
        bool set_lora_packet_params(uint16_t preamble_length, Header_Type header_type, uint8_t payload_length, Crc_Type crc_type, Invert_Iq iq);

        /**
         * @brief 配置无线电的调制参数，根据在此函数调用之前选择的数据包类型，这些参数将由芯片以不同的方式解释
         * @param sf 使用 Sf:: 配置，LoRa调制中使用的扩频因子
         * @param bw 使用 Bw:: 配置，LoRa信号的带宽
         * @param cr 使用 Cr:: 配置，LoRa有效载荷使用前向纠错机制，该机制有多个编码级别
         * @param ldro 使用 Ldro:: 配置，低数据速率优化，当LoRa符号时间等于或大于16.38毫秒时（通常在SF11和BW125以及SF12与BW125和BW250的情况下），
         * 通常会设置此参数
         * @return
         * @Date 2025-01-21 16:47:47
         */
        bool set_lora_modulation_params(Sf sf, Bw bw, Cr cr, Ldro ldro);

        /**
         * @brief 设置输出功率
         * @param power （-9 ~ 22）有越界校正
         * @return
         * @Date 2025-02-06 17:42:39
         */
        bool set_output_power(int8_t power);

        /**
         * @brief 校准设备在其工作频段内的镜像抑制
         * @param freq 使用 Img_Cal_Freq:: 配置，需要校准的频率范围
         * @return
         * @Date 2025-02-06 18:00:12
         */
        bool calibrate_image(Img_Cal_Freq freq);

        /**
         * @brief 设置射频频率模式的频率
         * @param freq （150 ~ 960）RF的频率设置
         * @return
         * @Date 2025-02-07 09:43:33
         */
        bool set_rf_frequency(float freq);

        /**
         * @brief 设置传输频率
         * @param freq （150 ~ 960）频率设置
         * @return
         * @Date 2025-02-07 09:44:36
         */
        bool set_frequency(float freq);

        /**
         * @brief 配置Lora模式的传输参数
         * @param frequency （150 ~ 960）频率设置
         * @param bw 使用 Bw:: 配置，带宽设置
         * @param current_limit （0 ~ 140）电流限制
         * @param power （-9 ~ 22）设置功率
         * @param crc_type 使用 Crc_Type:: 配置，Crc校验
         * @param sf 使用 Sf:: 配置，扩频因子设置
         * @param cr 使用 Cr:: 配置，纠错编码级别
         * @param sync_word 同步字设置，0x3444 为公共网络，0x1424 为专用网络
         * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
         * @param regulator_mode 使用 Regulator_Mode:: 配置，设置电源调节器模式
         * @return
         * @Date 2025-02-07 09:55:00
         */
        bool config_lora_params(float frequency, Bw bw, float current_limit, int8_t power, Sf sf = Sf::SF9, Cr cr = Cr::CR_4_7,
                                Crc_Type crc_type = Crc_Type::ON, uint16_t preamble_length = 8, uint16_t sync_word = 0x1424,
                                Regulator_Mode regulator_mode = Regulator_Mode::LDO_AND_DCDC);

        /**
         * @brief 设置设备模式为接收模式
         * @param time_out_us 超时时间 = 设置的超时时间 * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）。
         * 当设置为 [0x000000] 时，设备将保持在RX模式下，直到接收发生，并且在完成后设备将返回到STBY_RC模式，
         * 当设置为 [0xFFFFFF] 时，设备将一直处于RX模式，直到主机发送命令更改操作模式。
         * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包 指示给主机，设备将自动搜索一个新的数据包。
         * @return
         * @Date 2025-02-07 11:31:43
         */
        bool set_rx(uint32_t time_out_us);

        /**
         * @brief 启动接收
         * @param time_out_us 超时时间 = 设置的超时时间 * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）。
         * 当设置为 [0x000000] 时，设备将保持在RX模式下，直到接收发生，并且在完成后设备将返回到STBY_RC模式，
         * 当设置为 [0xFFFFFF] 时，设备将一直处于RX模式，直到主机发送命令更改操作模式。
         * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包 指示给主机，设备将自动搜索一个新的数据包。
         * @return
         * @Date 2025-02-07 11:44:43
         */
        bool start_lora_receive(uint32_t time_out_us = 0xFFFFFF, uint16_t preamble_length = 8, Crc_Type crc_type = Crc_Type::ON);

        /**
         * @brief 获取中断状态
         * @return
         * @Date 2025-02-07 13:57:28
         */
        uint16_t get_irq_status(void);

        /**
         * @brief 获取接收到的数据长度
         * @return
         * @Date 2025-02-07 14:39:31
         */
        uint8_t get_rx_buffer_length(void);

        /**
         * @brief 读取传输数据
         * @param *data 读取数据的指针
         * @param length 要读取数据的长度，最大255
         * @param offset 数据偏移量
         * @return
         * @Date 2025-02-07 14:55:09
         */
        bool read_buffer(uint8_t *data, uint8_t length, uint8_t offset = 0);

        uint8_t receive_data(uint8_t *data, uint8_t length = 0);
    };
}