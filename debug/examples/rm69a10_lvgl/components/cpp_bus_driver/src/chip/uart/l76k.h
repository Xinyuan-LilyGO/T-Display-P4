/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-07-05 12:01:44
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
    class L76k : public Uart_Guide
    {
    private:
        static constexpr uint8_t GET_INFORMATION_TIMEOUT_COUNT = 3; // 获取信息超时计数

        static constexpr uint16_t L76K_UART_RX_MAX_SIZE = 1024 * 2; // 最大获取的数据量

        enum class Cmd
        {
            RO_DEVICE_ID = 0x00,

        };

        std::function<bool(bool)> _wake_up_callback = nullptr;

        int32_t _wake_up = DEFAULT_CPP_BUS_DRIVER_VALUE;
        int32_t _rst;

    public:
        enum class Update_Freq // 更新频率（定位频率）
        {
            FREQ_1HZ,
            FREQ_2HZ,
            FREQ_5HZ,
        };

        enum class Baud_Rate // 波特率
        {
            BR_4800_BPS = 0,
            BR_9600_BPS,
            BR_19200_BPS,
            BR_38400_BPS,
            BR_57600_BPS,
            BR_115200_BPS,
        };

        enum class Restart_Mode // 重启模式
        {
            HOT_START = 0,
            WARM_START,
            COLD_START,
            COLD_START_FACTORY_RESET,
        };

        enum class Gnss_Constellation // GNSS 星系
        {
            GPS = 1, // 美国全球定位系统
            BEIDOU,  // 中国的全球卫星导航系统
            GPS_BEIDOU,
            GLONASS, // 俄罗斯的全球卫星导航系统
            GPS_GLONASS,
            BEIDOU_GLONASS,
            GPS_BEIDOU_GLONASS,
        };

        struct Rmc
        {
            struct
            {
                uint8_t hour = -1;   // 小时
                uint8_t minute = -1; // 分钟
                float second = -1;   // 秒

                bool update_flag = false;
            } utc;

            // 定位系统状态。
            // A = 数据有效
            // V = 无效
            // D = 差分
            std::string location_status;

            struct
            {
                struct
                {
                    uint8_t degrees = -1;        // 度
                    double minutes = -1;         // 分
                    double degrees_minutes = -1; // 带小数的度分转度

                    // 纬度方向
                    // N = 北
                    // S = 南
                    std::string direction;

                    bool update_flag = false;           // 度分更新标志
                    bool direction_update_flag = false; // 方向更新标志
                } lat;

                struct
                {
                    uint8_t degrees = -1;
                    double minutes = -1;
                    double degrees_minutes = -1;

                    // 经度方向
                    // E = 东
                    // W = 西
                    std::string direction;

                    bool update_flag = false;
                    bool direction_update_flag = false;
                } lon;

            } location;

            struct
            {
                uint8_t day = -1;   // 日
                uint8_t month = -1; // 月
                uint8_t year = -1;  // 年

                bool update_flag = false;
            } data;
        };

        struct Gga
        {
            struct
            {
                uint8_t hour = -1;   // 小时
                uint8_t minute = -1; // 分钟
                float second = -1;   // 秒

                bool update_flag = false;
            } utc;

            struct
            {
                struct
                {
                    uint8_t degrees = -1;       // 度
                    float minutes = -1;         // 分
                    float degrees_minutes = -1; // 带小数的度分转度

                    // 纬度方向
                    // N = 北
                    // S = 南
                    std::string direction;

                    bool update_flag = false;           // 度分更新标志
                    bool direction_update_flag = false; // 方向更新标志
                } lat;

                struct
                {
                    uint8_t degrees = -1;
                    float minutes = -1;
                    float degrees_minutes = -1;

                    // 经度方向
                    // E = 东
                    // W = 西
                    std::string direction;

                    bool update_flag = false;
                    bool direction_update_flag = false;
                } lon;

            } location;

            // GPS 定位模式/状态指示
            // 0 = 定位不可用或无效
            // 1 = GPS SPS 模式，定位有效
            // 2 = 差分 GPS、SPS 模式或 SBAS定位有效
            // 6 = 估算（航位推算）模式
            uint8_t gps_mode_status = -1;
            uint8_t online_satellite_count = -1; // 在线的卫星数量

            float hdop = -1;
        };

        uint16_t _update_freq = 1000; // 默认更新频率为 1000ms（1Hz）

        L76k(const std::shared_ptr<Bus_Uart_Guide> bus, const int32_t wake_up, const int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Uart_Guide(bus), _wake_up(wake_up), _rst(rst)
        {
        }

        L76k(const std::shared_ptr<Bus_Uart_Guide> bus, const std::function<bool(bool)> &wake_up_callback,
             const int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Uart_Guide(bus), _wake_up_callback(wake_up_callback), _rst(rst)
        {
        }

        bool begin(int32_t baud_rate = 9600) override;

        /**
         * @brief 启动睡眠
         * @param enable [true]：进入睡眠，[false]：退出睡眠
         * @return
         * @Date 2025-02-14 16:18:00
         */
        bool sleep(bool enable);

        bool get_device_id(size_t *search_index = nullptr);

        /**
         * @brief 直接读取数据
         * @param *data 读取数据的指针
         * @param length 要读取数据的长度
         * @return 接收的数据长度，如果接收错误或者接收长度为0都返回0
         * @Date 2025-02-13 18:04:11
         */
        uint32_t read_data(uint8_t *data, uint32_t length = 0);

        /**
         * @brief 获取接收缓存数据的长度
         * @return
         * @Date 2025-02-13 18:22:46
         */
        size_t get_rx_buffer_length(void);

        /**
         * @brief 清除接收缓存中的所有数据
         * @return
         * @Date 2025-02-13 18:22:46
         */
        bool clear_rx_buffer_data(void);

        /**
         * @brief 获取信息数据
         * @param &data 获取数据的指针
         * @param *length 获取长度的指针
         * @param max_length 获取数据的最大长度
         * @param timeout_count 超时计数
         * @return
         * @Date 2025-03-24 10:03:31
         */
        bool get_info_data(std::shared_ptr<uint8_t[]> &data, uint32_t *length, uint32_t max_length = L76K_UART_RX_MAX_SIZE, uint8_t timeout_count = GET_INFORMATION_TIMEOUT_COUNT);

        /**
         * @brief 解析rmc信息
         * @param data 要解析的数据
         * @param length 要解析的数据长度
         * @param &rmc 返回的解析结构体
         * @return
         * @Date 2025-02-18 11:54:34
         */
        bool parse_rmc_info(std::shared_ptr<uint8_t[]> data, size_t length, Rmc &rmc);

        /**
         * @brief 解析gga信息
         * @param data 要解析的数据
         * @param length 要解析的数据长度
         * @param &gga 返回的解析结构体
         * @return
         * @Date 2025-02-18 11:54:34
         */
        bool parse_gga_info(std::shared_ptr<uint8_t[]> data, size_t length, Gga &gga);

        /**
         * @brief 设置定位频率
         * @param freq 使用 Positioning_Freq::配置，频率设定
         * @return
         * @Date 2025-02-14 18:18:57
         */
        bool set_update_frequency(Update_Freq freq);

        /**
         * @brief 设置模块和系统的波特率
         * @param baud_rate 使用 Baud_Rate::配置，波特率设定
         * @return
         * @Date 2025-02-17 13:45:17
         */
        bool set_baud_rate(Baud_Rate baud_rate);

        /**
         * @brief 获取系统的波特率
         * @return 波特率数据
         * @Date 2025-02-17 13:45:58
         */
        uint32_t get_baud_rate(void);

        /**
         * @brief 设置重启模式
         * @param mode 使用 Restart_Mode::配置
         * @return
         * @Date 2025-02-17 14:27:01
         */
        bool set_restart_mode(Restart_Mode mode);

        /**
         * @brief 设置GNSS的星系
         * @param constellation 使用 Gnss_Constellation::配置
         * @return
         * @Date 2025-02-17 14:45:15
         */
        bool set_gnss_constellation(Gnss_Constellation constellation);
    };
}