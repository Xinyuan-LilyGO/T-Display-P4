/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-06-16 17:51:28
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define BQ27220YZFR_DEVICE_DEFAULT_ADDRESS 0x55

    class Bq27220xxxx : public Iic_Guide
    {
    private:
        static constexpr uint16_t BQ27220YZFR_DEVICE_ID = 0x0220;

        enum class Cmd
        {
            // 读写寄存器命令
            // WO_WRITE_REGISTER = 0xAA,
            // WO_READ_REGISTER,
            // RAM寄存器地址命令
            WR_RAM_REGISTER = 0x3E,

            RW_CONTROL_STATUS_START = 0x00,

            RW_AT_RATE_START = 0x02,
            RO_AT_RATE_TIME_TO_EMPTY_START = 0x04,
            RO_TEMPERATURE_START = 0x06,
            RO_VOLTAGE_START = 0x08,
            RO_BATTERY_STATUS_START = 0x0A,
            RO_CURRENT_START = 0x0C,
            RO_REMAINING_CAPACITY_START = 0x10,
            RO_FULL_CHARGE_CAPACITY_START = 0x12,
            RO_TIME_TO_EMPTY_START = 0x16,
            RO_TIME_TO_FULL_START = 0x18,
            RO_STANDBY_CURRENT_START = 0x1A,
            RO_STANDBY_TIME_TO_EMPTY_START = 0x1C,
            RO_MAX_LOAD_CURRENT_START = 0x1E,
            RO_MAX_LOAD_TIME_TO_EMPTY_START = 0x20,
            RO_RAW_COULOMB_COUNT_START = 0x22,
            RO_AVERAGE_POWER_START = 0x24,
            RO_INTERNAL_TEMPERATURE_START = 0x28,
            RO_CYCLE_COUNT_START = 0x2A,
            RO_STATUS_OF_CHARGE_START = 0x2C,
            RO_STATUS_OF_HEALTH_START = 0x2E,
            RO_CHARGING_VOLTAGE_START = 0x30,
            RO_CHARGING_CURRENT_START = 0x32,
            RO_OPERATION_STATUS_START = 0x3A,
            RO_DESIGN_CAPACITY_START = 0x3C,
            RW_MAC_DATA_START = 0x40,
            RO_MAC_DATA_SUM_START = 0x60,
            RO_MAC_DATA_LEN_START,
        };

        // 访问寄存器需要通过前置读写命令来访问
        // 采用小端先发的规则发送（0x0001 先发0x01后发0x00）
        // 凡是写寄存器都需要延时10ms
        enum class Control_Status_Reg
        {
            RO_DEVICE_ID = 0x0001,
            WO_ENTER_CFG_UPDATE = 0x0090,
            WO_EXIT_CFG_UPDATE_REINIT,
            WO_EXIT_CFG_UPDATE,
        };

        // 凡是写寄存器都需要延时10ms
        enum class Configuration_Reg
        {
            RW_OPERATION_CONFIG_A = 0x9206,
            RW_SLEEP_CURRENT = 0x9217,
        };

        // 凡是写寄存器都需要延时10ms
        enum class Gas_Gauging_Reg
        {
            WO_DESIGN_CAPACITY = 0x929F,
        };

        // static constexpr uint8_t Init_List[];

        int32_t _rst;

    public:
        enum class Temperature_Mode
        {
            INTERNAL,
            EXTERNAL_NTC,
        };
        struct Battery_Status
        {
            struct
            {
                bool fd = false;       // 检测到完全放电，该标志根据选择的 SOC Flag Config B 选项进行设置和清除
                bool ocvcomp = false;  // OCV 测量更新已完成，设置时为真
                bool ocvfail = false;  // 指示 OCV 读取因电流而失败的状态位，该位只能在接收到 OCV_CMD() 后在电池存在的情况下进行设置，设置时为真
                bool sleep = false;    // 设置时器件在 SLEEP 模式下运行，该位将在 SLEEP 模式下的 AD 测量期间暂时清除
                bool otc = false;      //  检测到充电条件下的过热，如果 Operation Config B [INT_OT] 位 = 1，则 SOC_INT 引脚会在 [OTC] 位被设置时切换一次
                bool otd = false;      // 检测到放电条件下的过热，设置时为真，如果 Operation Config B [INT_OT] 位 = 1，则 SOC_INT 引脚会在 [OTD] 位被设置时切换一次
                bool fc = false;       // 检测到充满电，该标志根据选择的 SOC Flag Config A 和 SOC Flag Config B 选项进行设置和清除
                bool chginh = false;   // 充电禁止：如果设置，则表示不应开始充电，因为 Temperature() 超出范围 [Charge Inhibit Temp Low, Charge Inhibit Temp High]，设置时为真
                bool tca = false;      // 终止充电警报，该标志根据选择的 SOC Flag Config A 选项进行设置和清除
                bool ocvgd = false;    // 进行了良好的 OCV 测量，设置时为真
                bool auth_gd = false;  // 检测插入的电池，设置时为真
                bool battpres = false; // 检测到电池存在，设置时为真
                bool tda = false;      // 终止放电警报，该标志根据选择的 SOC Flag Config A 选项进行设置和清除
                bool sysdwn = false;   // 指示系统应关闭的系统关闭位，设置时为真，如果设置，SOC_INT 引脚会切换一次
                bool dsg = false;      // 设置时，器件处于 DISCHARGE 模式；清除时，器件处于 CHARGING 或 RELAXATION 模式
            } flag;
        };

        struct Operation_Status
        {
            uint8_t sec = 0; // 定义当前安全访问
            struct
            {
                bool cfg_update = false; // 电量监测计处于 CONFIG UPDATE 模式，电量监测暂停
                bool btp_int = false;    // 指示已超过 BTP 阈值的标志
                bool smth = false;       // 指示 RemainingCapacity() 累积当前正在由平滑处理引擎进行调节
                bool init_comp = false;  // 指示电量监测计初始化是否完成。该位只能在电池存在时被设置。设置时为真
                bool vdq = false;        // 指示当前的放电周期是符合还是不符合 FCC 更新的要求。会设置对 FCC 更新有效的放电周期
                bool edv2 = false;       // 指示测量的电池电压是高于还是低于 EDV2 阈值。设置时表示低于
                bool calmd = false;      // 使用 0x2D 命令进行切换，以启用/禁用 CALIBRATION 模式
            } flag;
        };

    public:
        Bq27220xxxx(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

    private:
        /**
         * @brief 进入配置更新模式
         * @return
         * @Date 2025-02-26 15:33:17
         */
        bool enter_cgf_update(void);
        /**
         * @brief 退出配置更新模式
         * @return
         * @Date 2025-02-26 15:33:17
         */
        bool exit_cfg_update(void);

    public:
        uint16_t get_device_id(void);

        /**
         * @brief 获取设置的电池容量，该值在电池的插拔过程中恢复初始状态
         * @return
         * @Date 2025-02-26 10:39:26
         */
        uint16_t get_design_capacity(void);

        /**
         * @brief 获取电池包电压，以 mV 为单位，范围为 0 至 6000mV
         * @return
         * @Date 2025-02-26 16:04:44
         */
        uint16_t get_voltage(void);

        /**
         * @brief 获取流过感测电阻的瞬时电流，它每秒更新一次，单位为 mA
         * @return
         * @Date 2025-02-26 16:05:21
         */
        int16_t get_current(void);

        /**
         * @brief 获取电池剩余容量，当 CEDV Smoothing Config [SMEN] 被设置时，这将是平滑处理引擎的结果，否则，会返回未过滤的剩余容量，单位为 mAh
         * @return
         * @Date 2025-02-26 16:06:35
         */
        uint16_t get_remaining_capacity(void);

        /**
         * @brief 获取充满电的电池的补偿容量，单位为 mAh，FullChargeCapacity() 按照 CEDV 算法的规定定期更新（需要电池充放电才更新）
         * @return
         * @Date 2025-02-26 16:07:12
         */
        uint16_t get_full_charge_capacity(void);

        /**
         * @brief 获取放电速率，默认值是 0，AtRate 的值会被 AtRateTimeToEmpty() 函数使用，用于计算剩余运行时间，
         * 如果 AtRate 设置为 0，AtRateTimeToEmpty() 将返回 65,535，表示无法预测剩余时间（或者剩余时间非常长）
         * 只能在 NORMAL 模式下使用
         * @return 放电或者充电电流值，负号代表放电
         * @Date 2025-02-26 10:53:31
         */
        int16_t get_at_rate(void);

        /**
         * @brief 设置放电速率，默认值是 0，AtRate 的值会被 AtRateTimeToEmpty() 函数使用，用于计算剩余运行时间，
         * 如果 AtRate 设置为 0，AtRateTimeToEmpty() 将返回 65,535，表示无法预测剩余时间（或者剩余时间非常长）
         * 只能在 NORMAL 模式下使用
         * @param rate 放电或者充电电流值，负号代表放电
         * @return
         * @Date 2025-02-26 10:55:46
         */
        bool set_at_rate(int16_t rate);

        /**
         * @brief 在set_at_rate()函数设置的放电电流速度的条件下，预测的剩余电池还能工作的时间（以分钟为单位），在系统设置 AtRate() 值后，
         * AtRateTimeToEmpty() 会在 1 秒内更新，此外，燃料计（fuel gauge）会每秒自动更新 AtRateTimeToEmpty()，基于当前的 AtRate() 值和电池状态，
         *  只能在 NORMAL 模式下使用，值 65535 表示 AtRate() = 0
         * @return 值的范围是 0 到 65534 分钟
         * @Date 2025-02-26 10:56:42
         */
        uint16_t get_at_rate_time_to_empty(void);

        /**
         * @brief 获取温度，可以获取芯片内部温度或者外部NTC温度，由Operation Config中的[WRTEMP]和[TEMPS]值决定，默认读取芯片内部温度
         * @return 电量监测计测量的温度的浮点数，以°K为单位
         * @Date 2025-02-26 11:35:54
         */
        float get_temperature_kelvin(void);

        /**
         * @brief 获取温度，可以获取芯片内部温度或者外部NTC温度，由Operation Config中的[WRTEMP]和[TEMPS]值决定，默认读取芯片内部温度
         * @return 电量监测计测量的温度的浮点数，以°C为单位
         * @Date 2025-02-26 11:35:54
         */
        float get_temperature_celsius(void);

        /**
         * @brief 设置获取温度的模式，从内部获取或外部NTC获取
         * @param mode 使用 Temperature_Mode::配置
         * @return
         * @Date 2025-02-26 15:35:10
         */
        bool set_temperature_mode(Temperature_Mode mode);

        /**
         * @brief 获取电池状态
         * @param &status 使用 Battery_Status::获取
         * @return
         * @Date 2025-02-26 16:03:21
         */
        bool get_battery_status(Battery_Status &status);

        /**
         * @brief 获取操作状态
         * @param &status 使用 Operation_Status::获取
         * @return
         * @Date 2025-02-26 16:03:49
         */
        bool get_operation_status(Operation_Status &status);

        /**
         * @brief 设置电池容量
         * @param capacity 电池容量
         * @return
         * @Date 2025-02-26 10:39:54
         */
        bool set_design_capacity(uint16_t capacity);

        /**
         * @brief 获取当前放电率下预测的剩余电池还能工作的时间（以分钟为单位），值 65535 表示电池未在放电
         * @return 值的范围是 0 到 65534 分钟
         * @Date 2025-02-26 16:22:42
         */
        uint16_t get_time_to_empty(void);

        /**
         * @brief 获取当前电池充电的所需时间，根据 AverageCurrent() 预测电池达到充满电状态的剩余时间（以分钟为单位），
         * 该计算考虑了基于固定 AverageCurrent() 电荷累积速率的线性 TTF 计算的收尾电流时间扩展，值 65535 表示电池未在充电
         * @return 值的范围是 0 到 65534 分钟
         * @Date 2025-02-26 16:22:42
         */
        uint16_t get_time_to_full(void);

        /**
         * @brief 获取通过检测电阻的待机电流，StandbyCurrent() 是自适应测量值，最初它会报告在 Initial Standby 中编程的待机电流，
         * 在待机模式下经过几秒钟后会报告测得的待机电流，当测量的电流高于 Deadband 且小于或等于 2 × Initial Standby 时，寄存器值每秒更新一次，
         * 符合这些标准的第一个值和最后一个值不会包含在内，因为它们可能不是稳定的值。为了接近 1 分钟的时间常数，
         * 每个新的 StandbyCurrent() 值通过取最后一个待机电流的大约 93% 的权重和当前测量的平均电流的大约 7% 来计算
         * @return
         * @Date 2025-02-26 16:44:07
         */
        int16_t get_standby_current(void);

        /**
         * @brief 获取待机放电率下预测的剩余电池还能工作的时间（以分钟为单位），该计算使用 NominalAvailableCapacity() (NAC)（未经补偿的剩余容量）
         * 值 65535 表示电池未在放电
         * @return
         * @Date 2025-02-26 16:48:14
         */
        uint16_t get_standby_time_to_empty(void);

        /**
         * @brief 获取最大负载情况下的电流，以 mA 为单位，MaxLoadCurrent() 是自适应测量值，最初报告为在 Initial Max Load Current 中编程的最大负载电流，
         * 如果测量的电流始终大于 Initial Max Load Current，则只要在前一次放电至 SOC 低于 50% 后电池充满电，Max Load Current () 就会减小至前一个值和 Initial Max Load Current 的平均值，
         * 这可以防止报告的值保持在异常高的水平
         * @return
         * @Date 2025-02-26 16:56:09
         */
        int16_t get_max_load_current(void);

        /**
         * @brief 获取最大负载电流放电率下预测的剩余电池还能工作的时间（以分钟为单位），值 65535 表示电池未在放电
         * @return
         * @Date 2025-02-26 16:59:30
         */
        uint16_t get_max_load_time_to_empty(void);

        /**
         * @brief 获取在充电/放电期间从电池中转移的库仑量，计数器在放电期间递增，在充电期间递减，充电期间，当 FC 位被设置（表示充满电）时，计数器清零，
         * IGNORE_SD 位提供忽略自放电的功能，IGNORE_SD = 0（默认值）（常规放电或自放电期间库仑计递增），IGNORE_SD = 1（库仑计仅在真正放电时才递增）
         * @return
         * @Date 2025-02-26 17:08:21
         */
        uint16_t get_raw_coulomb_count(void);

        /**
         * @brief 该值表示电池充电和放电期间的平均功率，放电期间值为负，充电期间值为正，值 0 表示电池未在放电，报告该值时采用的单位为 mW
         * @return
         * @Date 2025-02-26 17:12:11
         */
        int16_t get_average_power(void);

        /**
         * @brief 获取芯片温度，获取电量监测计测量的内部温度传感器的浮点数值
         * @return 电量监测计测量的温度的浮点数，以°K为单位
         * @return
         * @Date 2025-02-26 15:56:06
         */
        float get_chip_temperature_kelvin(void);

        /**
         * @brief 获取芯片温度，获取电量监测计测量的内部温度传感器的浮点数值
         * @return 电量监测计测量的温度的浮点数，以°C为单位
         * @return
         * @Date 2025-02-26 15:56:06
         */
        float get_chip_temperature_celsius(void);

        /**
         * @brief 获取活动电池已经历的周期数，其范围为 0 至 65535，当累积放电 ≥ 周期阈值时，会产生一个周期，
         * 周期阈值的计算方法为 Cycle Count Percentage 乘以 FullChargeCapacity()（当CEDV Gauging Configuration [CCT] = 1 时）或 DesignCapacity()（当 [CCT] = 0 时）
         * @return
         * @Date 2025-02-26 17:18:35
         */
        uint16_t get_cycle_count(void);

        /**
         * @brief 获取预测的剩余电池电量百分比，实际为RemainingCapacity() 占 FullChargeCapacity() 的百分比，范围为 0 至 100%，
         * StatusOfCharge() = RemainingCapacity() ÷ FullChargeCapacity()，四舍五入至最接近的整数百分点
         * @return
         * @Date 2025-02-26 17:23:27
         */
        uint16_t get_status_of_charge(void);

        /**
         * @brief 获取预测的电池健康百分比，实际为FullChargeCapacity() 占 DesignCapacity() 的百分比，范围为 0 至 100%，
         * StatusOfHealth() = FullChargeCapacity() ÷ DesignCapacity()，四舍五入至最接近的整数百分点
         * @return
         * @Date 2025-02-26 17:23:42
         */
        uint16_t get_status_of_health(void);

        /**
         * @brief 获取电池充电电压，值 65535 表示电池请求电池充电器提供最大电压
         * @return
         * @Date 2025-02-26 17:34:03
         */
        uint16_t get_charging_voltage(void);

        /**
         * @brief 获取电池充电电流，值 65535 表示电池请求电池充电器提供最大电流
         * @return
         * @Date 2025-02-26 17:34:52
         */
        uint16_t get_charging_current(void);

        /**
         * @brief 设置睡眠电流阈值，单位mA默认值为10mA，当设置sleep使能并且电流低于设置的这个阈值时自动进入睡眠模式
         * @param threshold （0 ~ 100mA）睡眠电流阈值
         * @return
         * @Date 2025-02-26 18:08:31
         */
        bool set_sleep_current_threshold(uint16_t threshold);
    };
}