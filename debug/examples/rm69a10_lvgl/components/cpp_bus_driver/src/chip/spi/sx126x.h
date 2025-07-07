/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-06-20 18:10:25
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
    class Sx126x : public Spi_Guide
    {
    private:
        static constexpr uint16_t SX126X_BUSY_PIN_TIMEOUT_COUNT = 10000;
        static constexpr uint8_t SX126X_BUSY_FUNCTION_TIMEOUT_COUNT = 100;
        static constexpr uint8_t SX126X_MAX_TRANSMIT_BUFFER_SIZE = 255;

    public:
        enum class Chip_Type
        {
            SX1262 = 0x00,
            SX1261,
        };

    private:
        enum class Cmd
        {
            // 用于读写寄存器命令
            WO_WRITE_REGISTER = 0x0D,
            WO_READ_REGISTER = 0x1D,

            WO_CLEAR_IRQ_STATUS = 0x02,
            WO_SET_DIO_IRQ_PARAMS = 0x08,
            WO_WRITE_BUFFER = 0x0E,
            RO_GET_PACKET_TYPE = 0x11,
            RO_GET_IRQ_STATUS,
            RO_GET_RX_BUFFER_STATUS,
            RO_GET_PACKET_STATUS,
            RO_READ_BUFFER = 0x1E,
            WO_SET_STANDBY = 0x80,
            WO_SET_RX = 0x82,
            WO_SET_TX,
            WO_SET_SLEEP,
            WO_SET_RF_FREQUENCY = 0x86,
            WO_SET_CAD_PARAMS = 0x88,
            WO_CALIBRATE,
            WO_SET_PACKET_TYPE,
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
            WO_SET_TX_CONTINUOUS_WAVE = 0xD1,

        };

        // 访问寄存器需要通过前置读写命令来访问
        // 采用大端先发的规则发送（0x0001 先发0x00后发0x01）
        enum class Reg
        {
            RW_CRC_VALUE_PROGRAMMING_START = 0x06BC,
            RW_SYNC_WORD_PROGRAMMING_START = 0x06C0,
            RW_IQ_POLARITY_SETUP = 0x0736,
            RW_LORA_SYNC_WORD_START = 0x0740,
            RW_TX_MODULATION = 0x0889,
            RW_TX_CLAMP_CONFIG = 0x08D8,
            RW_OCP_CONFIGURATION = 0x08E7,
        };

        // static constexpr uint16_t Init_List[];

        Chip_Type _chip_type;

        int32_t _rst;

        int32_t _busy = DEFAULT_CPP_BUS_DRIVER_VALUE;
        bool (*_busy_wait_callback)(void) = nullptr;

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
            FALSE = -1,

            GFSK = 0x00,
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

        enum class Chip_Mode // 芯片所处的模式
        {
            RX, // 接收模式
            TX, // 发送模式
        };

        // Table 13-29: IRQ Registers
        /**
         * Bit | IRQ              | Description                              | Modulation
         * ----|------------------|------------------------------------------|-----------
         * 0   | TxDone           | Packet transmission completed            | All
         * 1   | RxDone           | Packet received                          | All
         * 2   | PreambleDetected | Preamble detected                        | All
         * 3   | SyncWordValid    | Valid sync word detected                 | FSK
         * 4   | HeaderValid      | Valid LoRa header received               | LoRa&reg;
         * 5   | HeaderErr        | LoRa header CRC error                    | LoRa&reg;
         * 6   | CrcErr           | Wrong CRC received                       | All
         * 7   | CadDone          | Channel activity detection finished       | LoRa&reg;
         * 8   | CadDetected      | Channel activity detected                | LoRa&reg;
         * 9   | Timeout          | Rx or Tx timeout                         | All
         * 10-13| -                | RFU                                      | -
         * 14  | LrFhssHop        | Asserted at each hop, in Long Range FHSS, after the PA has ramped-up again | LR-FHSS
         * 15  | -                | RFU                                      | -
         */
        enum class Irq_Flag // 需要清除的中断请求参数，设置1代表清除
        {
            DISABLE = 0,                  // 取消中断
            ALL = 0B0100001111111111,     // 全部中断
            TX_DONE = 0B0000000000000001, // TX完成中断
            RX_DONE = 0B0000000000000010, // RX完成中断
            PREAMBLE_DETECTED = 0B0000000000000100,
            SYNC_WORD_VALID = 0B0000000000001000,
            HEADER_VALID = 0B0000000000010000,
            HEADER_ERROR = 0B0000000000100000,
            CRC_ERROR = 0B0000000001000000,
            CAD_DONE = 0B0000000010000000,
            CAD_DETECTED = 0B0000000100000000,
            TIMEOUT = 0B0000001000000000,
            LRFHSS_HOP = 0B0100000000000000,
        };

        // Table 13-76: Status Bytes Definition
        /**
         * Bit |  Description
         * ----|------------------
         * 0   | Reserved
         * 1-3   |Command status (
         * [0x0: Reserved],
         * [0x1: RFU],
         * [0x2: Data is available to host],
         * [0x3: Command timeout],
         * [0x4: Command processing error],
         * [0x5: Failure to execute command],
         * [0x6: Command TX done])
         * 4-6   | Chip mode (
         * [0x0: Unused],
         * [0x1: RFU],
         * [0x2: STBY_RC],
         * [0x3: STBY_XOSC],
         * [0x4: FS],
         * [0x5: RX],
         * [0x6: TX])
         * 7   | Reserved
         */
        enum class Cmd_Status // 命令状态
        {
            FALSE = -1,

            RFU = 0x01,
            DATA_IS_AVAILABLE_TO_HOST,
            CMD_TIMEOUT,
            CMD_PROCESSING_ERROR,
            FAIL_TO_EXECUTE_CMD,
            CMD_TX_DONE,
        };

        enum class Chip_Mode_Status // 芯片模式状态
        {
            FALSE = -1,

            STBY_RC = 0x02,
            STBY_XOSC,
            FS,
            RX,
            TX,
        };

        enum class Sleep_Mode
        {
            COLD_START = 0B00000000, // 冷启动，关闭RTC唤醒
            WARM_START = 0B00000100, // 热启动，关闭RTC唤醒

            COLD_START_WAKE_UP_ON_RTC_TIMEOUT = 0B00000001, // 冷启动，开启RTC唤醒（RTC唤醒来自RC64k）
            WARM_START_WAKE_UP_ON_RTC_TIMEOUT = 0B00000101, // 热启动，开启RTC唤醒（RTC唤醒来自RC64k）
        };

        struct Irq_Status // 中断状态
        {
            struct // 全局标志
            {
                bool tx_done = false;           // 发送完成标志
                bool rx_done = false;           // 接收完成标志
                bool preamble_detected = false; // 检测到前导字标志
                bool crc_error = false;         // CRC错误标志
                bool tx_rx_timeout = false;     // 发送或接收超时标志
            } all_flag;

            struct // GFSK模式标志
            {
                bool sync_word_valid = false; // 同步字正确性标志
            } gfsk_flag;

            struct // LORA和寄存器模式标志
            {
                bool header_valid = false; // 头字节正确性标志
                bool header_error = false; // 头字节错误标志
                bool cad_done = false;     // cad传输完成标志
                bool cad_detected = false; // cad检测成功标志
            } lora_reg_flag;

            struct // LRFHSS模式标志
            {
                bool pa_ramped_up_hop = false; // 每次跳频后，在功率放大器 (PA) 再次完成升压 (ramped-up) 之后触发标志
            } lrfhss_flag;
        };

        struct Packet_Metrics
        {
            struct
            {
                float rssi_average = 0.0;       // 平均最后一个接收到的数据包的RSSI
                float rssi_instantaneous = 0.0; // 估算LoRa信号（去扩频后）在最后一个接收到的数据包上的RSSI
                float snr = 0.0;                // 最后一个接收到的数据包的SNR估计值
            } lora;

            struct
            {
                float rssi_average = 0.0; // 接收到的数据包有效载荷部分的RSSI平均值，在pkt_done中断请求时锁定
                float rssi_sync = 0.0;    // 在检测到同步地址时锁定的RSSI值
            } gfsk;
        };

        uint8_t _assert = 0;

        Sx126x(std::shared_ptr<Bus_Spi_Guide> bus, Chip_Type chip_type, int32_t busy,
               int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Spi_Guide(bus, cs), _chip_type(chip_type), _rst(rst), _busy(busy)
        {
        }

        Sx126x(std::shared_ptr<Bus_Spi_Guide> bus, Chip_Type chip_type, bool (*busy_wait_callback)(void),
               int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Spi_Guide(bus, cs), _chip_type(chip_type), _rst(rst), _busy_wait_callback(busy_wait_callback)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 检查设备忙
         * @return 如果返回 [true] 代表忙检查成功设备可以接收数据了，如果返回 [false] 代表满检测超过设定的忙等待最大阈值
         * @Date 2025-02-14 16:12:30
         */
        bool check_busy(void);

        /**
         * @brief 返回设备的状态，主机可以直接获取芯片状态
         * @return 状态数据，读取错误返回(-1)
         * @Date 2025-01-18 17:04:10
         */
        uint8_t get_status(void);

        /**
         * @brief 命令断言，详细请参考SX126x手册表格 13-76: Status Bytes Definition
         * @param assert_status 断言状态语句，由get_status()函数获取
         * @return Cmd_Status 由Cmd_Status::配置，命令状态
         * @Date 2025-02-12 14:41:21
         */
        Cmd_Status assert_cmd_status(uint8_t assert_status);

        /**
         * @brief 芯片模式断言，详细请参考SX126x手册表格 13-76: Status Bytes Definition
         * @param assert_status 断言状态语句，由get_status()函数获取
         * @return Chip_Mode_Status 由Chip_Mode_Status::配置，芯片模式状态
         * @Date 2025-02-13 13:52:33
         */
        Chip_Mode_Status assert_chip_mode_status(uint8_t assert_status);

        /**
         * @brief 中断断言，详细请参考SX126x手册表格 13-29: IRQ Registers
         * @param irq_flag 断言状态语句，由get_irq_flag()函数获取
         * @param &status 使用Irq_Status结构体配置，相应位自动置位
         * @return
         * @Date 2025-02-13 13:33:15
         */
        bool assert_irq_status(uint16_t irq_flag, Irq_Status &status);

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
         * @brief 优化功率放大器（PA）的钳位阈值（SX126x手册第15.2节）
         * @param enable [true]：开启修复，[false]：关闭修复
         * @return
         * @Date 2025-01-17 16:58:01
         */
        bool fix_tx_clamp(bool enable);

        /**
         * @brief 此命令为数据缓冲区设置基地址，适用于所有操作模式下的数据包处理操作，包括发送（TX）和接收（RX）模式
         * @param tx_base_address tx 基地址
         * @param rx_base_address rx 基地址
         * @return
         * @Date 2025-01-17 17:08:57
         */
        bool set_buffer_base_address(uint8_t tx_base_address, uint8_t rx_base_address);

        /**
         * @brief 设置传输数据包类型
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
         * @param flag 使用Irq_Flag::配置，需要清除的标志，设置1为清除标志
         * @return
         * @Date 2025-01-18 09:27:58
         */
        bool clear_irq_flag(Irq_Flag flag);

        /**
         * @brief 设置中断请求（IRQ）标志（中断标志参考SX126x手册表格 13-29: IRQ Registers）
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
         * @brief 获取当前使用的数据包类型
         * @return 由Packet_Type::配置，包类型
         * @Date 2025-01-21 16:28:58
         */
        Packet_Type get_packet_type(void);

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

        // LORA带宽
        enum class Lora_Bw
        {
            BW_7810HZ = 0x00,
            BW_15630HZ,
            BW_31250HZ,
            BW_62500HZ,
            BW_125000HZ,
            BW_250000HZ,
            BW_500000HZ,
            BW_10420HZ = 0x08,
            BW_20830HZ,
            BW_41670HZ,
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

        enum class Lora_Header_Type
        {
            VARIABLE_LENGTH_PACKET = 0x00, // 显式标头
            FIXED_LENGTH_PACKET,           // 隐式标头
        };

        enum class Lora_Crc_Type
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
         * @brief 设置LORA的同步字，每个LoRa数据包的开始部分都包含一个同步字，接收机通过匹配这个同步字来确认数据包的有效性，
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
         * @brief 修复LORA模式反转 IQ 配置 （SX126x手册第15.4节）
         * @param iq 使用 Invert_Iq:: 配置
         * @return
         * @Date 2025-01-22 09:58:32
         */
        bool fix_lora_inverted_iq(Invert_Iq iq);

        /**
         * @brief 设置数据包处理块的参数
         * @param preamble_length 前导长度是一个16位的值，表示无线电将发送的LoRa符号数量
         * @param header_type 使用 Lora_Header_Type:: 配置，当字节 header_type 的值为 0x00 时，有效载荷长度、编码率和头CRC将被添加到LoRa头部，并传输给接收器
         * @param payload_length 数据包的有效载荷（即实际传输的数据）的长度，这个参数通常用于指示数据包中有效数据的字节数，
         * 在进行数据传输时，发送端会设置这个长度，而接收端则根据这个长度来解析接收到的数据包
         * @param crc_type 使用 Lora_Crc_Type:: 配置
         * @param iq 使用 Invert_Iq:: 配置
         * @return
         * @Date 2025-01-22 11:44:47
         */
        bool set_lora_packet_params(uint16_t preamble_length, Lora_Header_Type header_type, uint8_t payload_length, Lora_Crc_Type crc_type, Invert_Iq iq);

        /**
         * @brief 配置无线电的调制参数，根据在此函数调用之前选择的数据包类型，这些参数将由芯片以不同的方式解释
         * @param sf 使用 Sf:: 配置，LoRa调制中使用的扩频因子
         * @param bw 使用 Lora_Bw:: 配置，LoRa信号的带宽
         * @param cr 使用 Cr:: 配置，LoRa有效载荷使用前向纠错机制，该机制有多个编码级别
         * @param ldro 使用 Ldro:: 配置，低数据速率优化，当LoRa符号时间等于或大于16.38毫秒时（通常在SF11和BW125以及SF12与BW125和BW250的情况下），
         * 通常会设置此参数
         * @return
         * @Date 2025-01-21 16:47:47
         */
        bool set_lora_modulation_params(Sf sf, Lora_Bw bw, Cr cr, Ldro ldro);

        /**
         * @brief 设置输出功率
         * @param power （-9 ~ 22）有越界校正
         * @return
         * @Date 2025-02-06 17:42:39
         */
        bool set_output_power(int8_t power);

        /**
         * @brief 校准设备在其工作频段内的镜像抑制
         * @param freq_mhz 使用 Img_Cal_Freq:: 配置，需要校准的频率范围
         * @return
         * @Date 2025-02-06 18:00:12
         */
        bool calibrate_image(Img_Cal_Freq freq_mhz);

        /**
         * @brief 设置射频频率模式的频率
         * @param freq_mhz （150 ~ 960）RF的频率设置
         * @return
         * @Date 2025-02-07 09:43:33
         */
        bool set_rf_frequency(float freq_mhz);

        /**
         * @brief 设置传输频率
         * @param freq_mhz （150 ~ 960）频率设置
         * @return
         * @Date 2025-02-07 09:44:36
         */
        bool set_frequency(float freq_mhz);

        /**
         * @brief 配置LORA模式的传输参数
         * @param freq_mhz （150 ~ 960）频率设置
         * @param bw 使用 Lora_Bw:: 配置，带宽设置
         * @param current_limit （0 ~ 140）电流限制
         * @param power （-9 ~ 22）设置功率
         * @param crc_type 使用 Lora_Crc_Type:: 配置，Crc校验
         * @param sf 使用 Sf:: 配置，扩频因子设置
         * @param cr 使用 Cr:: 配置，纠错编码级别
         * @param sync_word 同步字设置，0x3444 为公共网络，0x1424 为专用网络
         * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
         * @return
         * @Date 2025-02-07 09:55:00
         */
        bool config_lora_params(float freq_mhz, Lora_Bw bw, float current_limit, int8_t power, Sf sf = Sf::SF9, Cr cr = Cr::CR_4_7,
                                Lora_Crc_Type crc_type = Lora_Crc_Type::ON, uint16_t preamble_length = 8, uint16_t sync_word = 0x1424);

        /**
         * @brief 设置设备模式为接收模式
         * @param time_out_us 超时时间 = 设置的超时时间 * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）
         * 当设置为 [0x000000] 时，设备将保持在RX模式下，直到接收发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式
         * 当设置为 [0xFFFFFF] 时，设备将一直处于所设置的模式，直到主机发送命令更改操作模式。
         * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包 指示给主机，设备将自动搜索一个新的数据包
         * @return
         * @Date 2025-02-07 11:31:43
         */
        bool set_rx(uint32_t time_out_us);

        /**
         * @brief 开始LORA模式传输
         * @param chip_mode 使用 Chip_Mode:: 配置，芯片的模式
         * @param fallback_mode 从RX或TX模式退出返回的模式设定
         * 当设置为 [0x000000] 时，禁用超时，设备将保持在TX或RX模式下，直到TX或RX发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式
         * 当设置为 [0xFFFFFF] 时，设备将一直处于所设置的模式，直到主机发送命令更改操作模式。
         * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包 指示给主机，设备将自动搜索一个新的数据包
         * @param time_out_us 超时时间 = 设置的超时时间 * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）
         * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
         * @return
         * @Date 2025-02-07 11:44:43
         */
        bool start_lora_transmit(Chip_Mode chip_mode, uint32_t time_out_us = 0xFFFFFF,
                                 Fallback_Mode fallback_mode = Fallback_Mode::STDBY_XOSC, uint16_t preamble_length = 8);

        /**
         * @brief 获取中断请求状态
         * @return 中断状态，读取错误返回(-1)
         * @Date 2025-02-07 13:57:28
         */
        uint16_t get_irq_flag(void);

        /**
         * @brief 获取接收到的数据长度
         * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
         * @Date 2025-02-07 14:39:31
         */
        uint8_t get_rx_buffer_length(void);

        /**
         * @brief 读取数据
         * @param *data 读取数据的指针
         * @param length 要读取数据的长度，最大255
         * @param offset 数据偏移量
         * @return
         * @Date 2025-02-07 14:55:09
         */
        bool read_buffer(uint8_t *data, uint8_t length, uint8_t offset = 0);

        /**
         * @brief 接收数据
         * @param *data 接收数据的指针
         * @param length 接收数据的长度，如果等于0将默认接收对应接收到的数据长度并返回接受到的长度数据，最大255
         * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
         * @Date 2025-02-12 11:17:14
         */
        uint8_t receive_data(uint8_t *data, uint8_t length = 0);

        /**
         * @brief 获取LORA模式的包的指标信息
         * @param &metrics 使用 Packet_Metrics的结构体配置，读出包的相关指标信息
         * @return
         * @Date 2025-02-12 17:02:03
         */
        bool get_lora_packet_metrics(Packet_Metrics &metrics);

        /**
         * @brief 修复LORA发送模式在带宽为500khz下灵敏度衰减问题，在任何传输之前设置（设置一次即可不是每次都要设置）（SX126x手册第15.1节）
         * @param enable [true]：开启修复，[false]：关闭修复，只有在LORA模式并且带宽为500khz才开启
         * @return
         * @Date 2025-02-12 18:20:23
         */
        bool fix_bw_500khz_sensitivity(bool enable);

        /**
         * @brief 设置设备模式为接收模式
         * @param time_out_us 超时时间 = 设置的超时时间 * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）。
         * 当设置为 [0x000000] 时，设备将保持在TX模式下，直到发送发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式。
         * 当设置为 [0xFFFFFF] 时，设备将一直处于TX模式，直到主机发送命令更改操作模式。
         * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包 指示给主机，设备将自动搜索一个新的数据包。
         * @return
         * @Date 2025-02-12 18:27:27
         */
        bool set_tx(uint32_t time_out_us);

        /**
         * @brief 写入数据
         * @param *data 写入数据的指针
         * @param length 要写入数据的长度，最大255
         * @param offset 写入偏移量
         * @return
         * @Date 2025-02-13 09:51:03
         */
        bool write_buffer(uint8_t *data, uint8_t length, uint8_t offset = 0);

        /**
         * @brief 发送数据
         * @param *data 发送数据的指针
         * @param length 发送数据的长度，最大255
         * @param time_out_us 超时时间 = 设置的超时时间 * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）。
         * 当设置为 [0x000000] 时，设备将保持在TX模式下，直到发送发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式。
         * 当设置为 [0xFFFFFF] 时，设备将一直处于TX模式，直到主机发送命令更改操作模式。
         * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包 指示给主机，设备将自动搜索一个新的数据包。
         * @return
         * @Date 2025-02-13 09:08:52
         */
        bool send_data(uint8_t *data, uint8_t length, uint32_t time_out_us = 0);

        /**
         * @brief 设置LORA模式的CRC
         * @param crc_type 使用 Lora_Crc_Type:: 配置，Crc校验
         * @return
         * @Date 2025-02-21 15:36:16
         */
        bool set_lora_crc_packet_params(Lora_Crc_Type crc_type);

        // GFSK 的高斯滤波器的滚降因子
        enum class Pulse_Shape
        {
            NO_FILTER = 0x00,
            GAUSSIAN_BT_0_3 = 0X08,
            GAUSSIAN_BT_0_5,
            GAUSSIAN_BT_0_7,
            GAUSSIAN_BT_1,
        };

        // GFSK带宽
        enum class Gfsk_Bw
        {
            BW_467000HZ = 0x09,
            BW_234300HZ,
            BW_117300HZ,
            BW_58600HZ,
            BW_29300HZ,
            BW_14600HZ,
            BW_7300HZ,
            BW_373600HZ = 0x11,
            BW_187200HZ,
            BW_93800HZ,
            BW_46900HZ,
            BW_23400HZ,
            BW_11700HZ,
            BW_5800HZ,
            BW_312000HZ = 0x19,
            BW_156200HZ,
            BW_78200HZ,
            BW_39000HZ,
            BW_19500HZ,
            BW_9700HZ,
            BW_4800HZ,
        };

        // 检测接收到的信号中的前导码
        enum class Preamble_Detector
        {
            LENGTH_OFF = 0x00,
            LENGTH_8BIT = 0X04,
            LENGTH_16BIT,
            LENGTH_24BIT,
            LENGTH_32BIT,
        };

        // 比较接收到的数据包地址与设备预设的地址（节点地址和广播地址）的机制
        enum class Addr_Comp
        {
            FILTERING_DISABLE = 0X00,
            FILTERING_ACTIVATED_NODE,
            FILTERING_ACTIVATED_NODE_BROADCAST,
        };

        enum class Gfsk_Header_Type
        {
            KNOWN_PACKET = 0x00, // 数据包的长度在双方都是已知的，有效载荷的大小没有被添加到数据包中
            VARIABLE_PACKET,     // 数据包是可变长度的，有效载荷的第一个字节将是数据包的大小
        };

        enum class Gfsk_Crc_Type
        {
            CRC_1_BYTE = 0x00,     // CRC在1字节计算
            CRC_OFF,               // 关闭 CRC
            CRC_2_BYTE,            // CRC在2字节计算
            CRC_1_BYTE_INV = 0x04, // CRC在1字节上计算并反转
            CRC_2_BYTE_INV = 0x06, // CRC在2字节上计算并反转
        };

        enum class Whitening
        {
            NO_ENCODING = 0X00,
            ENABLE,
        };

        // Table 13-80: Status Fields
        /**
         * Bit |  RxStatus FSK
         * ----|-----------------
         * 0   | pkt send
         * 1   | pkt received
         * 2   | abort err
         * 3   | length err
         * 4   | crc err
         * 5   | adrs err
         * 6   | sync err
         * 7   | preamble err
         */
        struct Gfsk_Packet_Status
        {
            bool packet_send_done_flag = false;    // 发送完成标志
            bool packet_receive_done_flag = false; // 接收完成标志
            bool abort_error_flag = false;         // 异常终止错误标志
            bool length_error_flag = false;        // 长度错误标志
            bool crc_error_flag = false;           // CRC错误标志
            bool address_error_flag = false;       // 地址错误标志
            bool sync_word_flag = false;           // 同步字错误标志
            bool preamble_error_flag = false;      // 前导字错误标志
        };

    private:
        struct Param
        {
            Packet_Type packet_type = Packet_Type::GFSK;
            Regulator_Mode regulator_mode = Regulator_Mode::LDO_AND_DCDC;
            float freq_mhz = 868.1;
            float current_limit = 140;
            int8_t power = 10;

            struct
            {
                uint32_t bit_rate = 100.0;
                Gfsk_Bw band_width = Gfsk_Bw::BW_467000HZ;
                float freq_deviation = 10.0;

                struct
                {
                    uint8_t *data = nullptr;
                    uint8_t length = 0;
                } sync_word;

                Preamble_Detector preamble_detector = Preamble_Detector::LENGTH_8BIT;
                Addr_Comp address_comparison = Addr_Comp::FILTERING_DISABLE;
                Gfsk_Header_Type header_type = Gfsk_Header_Type::VARIABLE_PACKET;
                uint8_t payload_length = SX126X_MAX_TRANSMIT_BUFFER_SIZE;
                Pulse_Shape pulse_shape = Pulse_Shape::GAUSSIAN_BT_1;
                Sf spreading_factor = Sf::SF9;

                struct
                {
                    Gfsk_Crc_Type type = Gfsk_Crc_Type::CRC_2_BYTE_INV;
                    uint16_t initial = 0x1D0F;
                    uint16_t polynomial = 0x1021;
                } crc;

                Whitening whitening = Whitening::NO_ENCODING;

                uint16_t preamble_length = 32;
            } gfsk;

            struct
            {
                Sf spreading_factor = Sf::SF9;
                Lora_Bw band_width = Lora_Bw::BW_125000HZ;
                Ldro low_data_rate_optimize = Ldro::LDRO_OFF;
                Cr cr = Cr::CR_4_7;
                uint16_t sync_word = 0x1424;
                uint16_t preamble_length = 8;
                Lora_Header_Type header_type = Lora_Header_Type::VARIABLE_LENGTH_PACKET;
                uint8_t payload_length = SX126X_MAX_TRANSMIT_BUFFER_SIZE;
                Lora_Crc_Type crc_type = Lora_Crc_Type::ON;
                Invert_Iq invert_iq = Invert_Iq::STANDARD_IQ_SETUP;
            } lora;
        };

    public:
        Param _param;

        /**
         * @brief 设置GFSK的同步字（还需要同时设置set_gfsx_packet_params）
         * @param *sync_word 同步字数据，最大8个字节
         * @param length 同步字数据长度
         * @return
         * @Date 2025-02-18 16:24:14
         */
        bool set_gfsk_sync_word(uint8_t *sync_word, uint8_t length);

        /**
         * @brief 设置GFSK的校验参数（还需要同时设置set_gfsx_packet_params）
         * @param initial 参数1，默认为 0x1D0F
         * @param polynomial 参数2，默认为 0x1021
         * @return
         * @Date 2025-02-19 09:33:11
         */
        bool set_gfsk_crc(uint16_t initial, uint16_t polynomial);

        /**
         * @brief 设置数据包处理块的参数
         * @param preamble_length 前导长度是一个16位的值，表示无线电将发送的GFSK符号数量
         * @param preamble_detector_length 使用 Preamble_Detector:: 配置,前导码检测器充当数据包控制器的门控器，当前导码检测器长度不为 0x00 时，
         * 数据包控制器仅在无线电成功接收到一定数量的前导码位后才会激活
         * @param sync_word_bit_length 同步字长度，1个字节等于8位，写入的时候要乘以8
         * @param addr_comp 使用 Addr_Comp:: 配置，用于比较接收到的数据包地址与设备预设的地址（节点地址和广播地址）的机制
         * @param header_type 使用 Gfsk_Header_Type:: 配置
         * @param payload_length 数据包的有效载荷（即实际传输的数据）的长度，这个参数通常用于指示数据包中有效数据的字节数，
         * 在进行数据传输时，发送端会设置这个长度，而接收端则根据这个长度来解析接收到的数据包
         * @param crc_type 使用 Gfsk_Crc_Type:: 配置
         * @param whitening 使用 Whitening:: 配置
         * @return
         * @Date 2025-02-18 17:29:22
         */
        bool set_gfsk_packet_params(uint16_t preamble_length, Preamble_Detector preamble_detector_length, uint8_t sync_word_bit_length,
                                    Addr_Comp addr_comp, Gfsk_Header_Type header_type, uint8_t payload_length, Gfsk_Crc_Type crc_type, Whitening whitening);

        /**
         * @brief 配置无线电的调制参数，根据在此函数调用之前选择的数据包类型，这些参数将由芯片以不同的方式解释
         * @param br （0.6 ~ 300）传输比特率
         * @param ps 使用 Pulse_Shape:: 配置，高斯滤波器的滚降因子
         * @param bw 使用 Gfsk_Bw:: 配置，带宽
         * @param freq_deviation （0.6 ~ 200）频率偏移
         * @return
         * @Date 2025-02-19 09:17:55
         */
        bool set_gfsk_modulation_params(float br, Pulse_Shape ps, Gfsk_Bw bw, float freq_deviation);

        /**
         * @brief 配置GFSK模式的传输参数
         * @param freq_mhz （150 ~ 960）频率设置
         * @param br （0.6 ~ 300）传输比特率
         * @param bw 使用 Gfsk_Bw:: 配置，带宽设置
         * @param current_limit （0 ~ 140）电流限制
         * @param power （-9 ~ 22）设置功率
         * @param freq_deviation （0.6 ~ 200）频率偏移
         * @param *sync_word 设置同步字数据指针
         * @param sync_word_length 设置同步字数据长度
         * @param ps 使用 Pulse_Shape:: 配置，高斯滤波器的滚降因子
         * @param sf 使用 Sf:: 配置，扩频因子设置
         * @param crc_type 使用 Gfsk_Crc_Type:: 配置，Crc校验
         * @param crc_initial CRC参数1，默认为 0x1D0F
         * @param crc_polynomial CRC参数2，默认为 0x1021
         * @param preamble_length 前导长度，表示无线电将发送的GFSK符号数量
         * @return
         * @Date 2025-02-19 09:17:55
         */
        bool config_gfsk_params(float freq_mhz, uint32_t br, Gfsk_Bw bw, float current_limit, int8_t power, float freq_deviation = 10.0,
                                uint8_t *sync_word = nullptr, uint8_t sync_word_length = 0, Pulse_Shape ps = Pulse_Shape::GAUSSIAN_BT_1,
                                Sf sf = Sf::SF9, Gfsk_Crc_Type crc_type = Gfsk_Crc_Type::CRC_2_BYTE_INV, uint16_t crc_initial = 0x1D0F, uint16_t crc_polynomial = 0x1021,
                                uint16_t preamble_length = 32);

        /**
         * @brief 开始GFSK模式传输
         * @param chip_mode 使用 Chip_Mode:: 配置，芯片的模式
         * @param fallback_mode 从RX或TX模式退出返回的模式设定
         * @param time_out_us 超时时间 = 设置的超时时间 * 15.625μs，设置的超时时间最大值为 16777215 （0xFFFFFF）
         * 当设置为 [0x000000] 时，禁用超时，设备将保持在TX或RX模式下，直到TX或RX发生，并且在完成后设备将返回到set_rx_tx_fallback_mode函数设置的模式
         * 当设置为 [0xFFFFFF] 时，设备将一直处于所设置的模式，直到主机发送命令更改操作模式
         * 该设备可以接收到多个数据包。每次收到一个数据包，就会完成一个数据包 指示给主机，设备将自动搜索一个新的数据包
         * @param preamble_length 前导长度，表示无线电将发送的LoRa符号数量
         * @return
         * @Date 2025-02-19 09:17:55
         */
        bool start_gfsk_transmit(Chip_Mode chip_mode, uint32_t time_out_us = 0xFFFFFF,
                                 Fallback_Mode fallback_mode = Fallback_Mode::STDBY_XOSC, uint16_t preamble_length = 32);

        /**
         * @brief 获取GFSK模式包的状态
         * @return uint32_t 值的排序为 [未使用(8bit)|RxStatus(8bit)|RssiSync(8bit)|RssiAvg(8bit)]
         * @Date 2025-02-28 16:34:18
         */
        uint32_t get_gfsk_packet_status(void);

        /**
         * @brief GFSK模式数据接收断言
         * @param assert_status 需要断言的状态数据，使用get_gfsk_packet_status()函数的返回值配置 （[RxStatus(8bit)]数据）
         * @param status 由Gfsk_Packet_Status结构体配置，包状态
         * @return
         * @Date 2025-02-21 15:17:05
         */
        bool assert_gfsk_packet_status(uint32_t assert_status, Gfsk_Packet_Status &status);

        /**
         * @brief 断言GFSK模式的包的指标信息
         * @param assert_metrics 需要断言的指标数据，使用get_gfsk_packet_status()函数的返回值配置（[RssiSync(8bit)|RssiAvg(8bit)]数据）
         * @param &metrics 使用 Packet_Metrics的结构体配置，读出包的相关指标信息
         * @return
         * @Date 2025-02-21 15:17:53
         */
        bool assert_gfsk_packet_metrics(uint32_t assert_metrics, Packet_Metrics &metrics);

        /**
         * @brief 设置GFSK模式的同步字，需要同时设置两个寄存器
         * @param *sync_word 设置同步字数据指针
         * @param sync_word_length 设置同步字数据长度
         * @return
         * @Date 2025-02-21 15:36:16
         */
        bool set_gfsk_sync_word_packet_params(uint8_t *sync_word, uint8_t sync_word_length);

        /**
         * @brief 设置GFSK模式的CRC，需要同时设置两个寄存器
         * @param crc_type 使用 Gfsk_Crc_Type:: 配置，Crc校验
         * @param crc_initial CRC参数1，默认为 0x1D0F
         * @param crc_polynomial CRC参数2，默认为 0x1021
         * @return
         * @Date 2025-02-21 15:36:16
         */
        bool set_gfsk_crc_packet_params(Gfsk_Crc_Type crc_type, uint16_t crc_initial, uint16_t crc_polynomial);

        /**
         * @brief 设置中断引脚的模式
         * @param dio1_mode 使用 Irq_Flag:: 配置，DIO1需要配置的芯片中断模式
         * @param dio2_mode 使用 Irq_Flag:: 配置，DIO2需要配置的芯片中断模式
         * @param dio3_mode 使用 Irq_Flag:: 配置，DIO3需要配置的芯片中断模式
         * @return
         * @Date 2025-03-15 11:48:59
         */
        bool set_irq_pin_mode(Irq_Flag dio1_mode, Irq_Flag dio2_mode = Irq_Flag::DISABLE, Irq_Flag dio3_mode = Irq_Flag::DISABLE);

        /**
         * @brief 清空buffer缓冲区所有数据
         * @return
         * @Date 2025-03-15 15:06:48
         */
        bool clear_buffer(void);

        /**
         * @brief 发送一个连续波（RF 纯音）用于测试TX最大发射功率（信号类型： 单一频率的连续信号，没有调制），
         * 用于检查无线电在特定频率和输出功率下的基本发射能力。可以用来测量发射功率、频率精度等
         * 这是一个测试命令，适用于所有数据包类型，用于在选定的频率和输出功率下生成连续波（RF 信号）
         * 设备会保持在 TX 连续波模式，直到主机发送模式配置命令。虽然此命令在实际应用中没有真正的用例，
         * 但它可以为开发者提供有价值的帮助，以检查和监控无线电在 Tx 模式下的性能
         * @return
         * @Date 2025-03-28 17:53:52
         */
        bool set_tx_continuous_wave(void);

        /**
         * @brief 将设备设置为电流消耗最低的睡眠模式（SLEEP 模式），该命令只能在待机模式（STDBY_RC 或 STDBY_XOSC）下发送，在整个 SLEEP 期间，BUSY（忙碌）线会被拉高，
         * 在 NSS 的上升沿之后，除了备份电源调节器（如果需要）和参数 sleepConfig 中指定的模块外，所有模块都将被关闭，开启睡眠，可以通过 NSS 线的下降沿从主机处理器唤醒设备，
         * 也可以使用set_standby()函数进行唤醒或者使用rtc唤醒
         * @param mode 使用Sleep_Mode::进行配置
         * @return
         * @Date 2025-05-08 17:27:59
         */
        bool set_sleep(Sleep_Mode mode = Sleep_Mode::WARM_START);
    };
}