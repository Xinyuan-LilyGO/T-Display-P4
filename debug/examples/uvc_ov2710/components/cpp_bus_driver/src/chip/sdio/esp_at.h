/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-06-16 18:10:23
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
    class Esp_At : public Sdio_Guide
    {
    private:
        static constexpr uint16_t ESP_AT_MAX_TRANSMIT_BLOCK_BUFFER_SIZE = 512;
        static constexpr uint8_t TX_BUFFER_OFFSET = 16; // 发送缓冲区偏移量
        static constexpr uint16_t TX_BUFFER_MASK = 0xFFF;
        static constexpr uint32_t RX_BUFFER_MASK = 0xFFFFF;
        static constexpr uint32_t RX_BUFFER_MAX = 0x100000;
        static constexpr uint8_t ESP_AT_TRANSMIT_TIMEOUT_COUNT = 100;
        static constexpr uint8_t ESP_AT_CONNECT_ERROR_COUNT = 5;
        static constexpr uint8_t ESP_AT_RECEIVE_TIMEOUT_COUNT = 3;

        static constexpr uint16_t ESP_AT_MAX_RECEIVE_SIZE = 2 * 1024;

        enum class Cmd
        {
            SD_IO_CCCR_FN_ENABLE = 0x00000002,
            SD_IO_CCCR_FN_READY,
            SD_IO_CCCR_INT_ENABLE,
            SD_IO_CCCR_BUS_WIDTH = 0x00000007,

            SD_IO_CCCR_BLKSIZEL = 0x00000010,
            SD_IO_CCCR_BLKSIZEH,

            SLCHOST_BASE = 0x3FF55000,

            SLAVE_CMD53_END_ADDR = 0x1F800,

            PACKET_LENGTH = (SLCHOST_BASE + 0x60) & 0x3FF,
            INTERRUPT_CLEAR = (SLCHOST_BASE + 0xD4) & 0x3FF,
            INTERRUPT_RAW = (SLCHOST_BASE + 0x50) & 0x3FF, // 原始中断位
            INTERRUPT_ST = (SLCHOST_BASE + 0x58) & 0x3FF,  // 掩码中断位
            TOKEN_RDATA = (SLCHOST_BASE + 0x44) & 0x3FF,

            CONF = (SLCHOST_BASE + 0x8C) & 0x3FF,
            CONF_OFFSET = 0,

        };

        int32_t _rst = DEFAULT_CPP_BUS_DRIVER_VALUE;
        void (*_rst_callback)(bool value) = nullptr;

        struct Esp_At_Connect
        {
            // 设备连接状态
            bool status = true;
            int8_t error_count = 0;

            // 从从机那里接收到的总长度引索，初始化为0，如果该值和从机内部定义的不对应，将与从机失去连接，需要重新初始化
            uint32_t receive_total_length_index = 0;
        };

        static constexpr const char *_time_month_list[] =
            {
                "Jan",
                "Feb",
                "Mar",
                "Apr",
                "May",
                "Jun",
                "Jul",
                "Aug",
                "Sep",
                "Oct",
                "Nov",
                "Dec",
            };

    public:
        struct Real_Time
        {
            std::string week = "";
            uint8_t day = -1;   // 日
            uint8_t month = -1; // 月
            uint16_t year = -1; // 年

            uint8_t hour = -1;   // 小时
            uint8_t minute = -1; // 分钟
            uint8_t second = -1; // 秒

            std::string time_zone = ""; // 时区
        };

        Esp_At_Connect _connect;

        enum class Irq_Flag
        {
            RX_NEW_PACKET = 1 << 23,
        };

        enum class Wifi_Mode
        {
            OFF = 0,
            STATION,
            SOFTAP,
            STATION_SOFTAP
        };

        enum class Sleep_Mode
        {
            DISABLE_SLEEP = 0,
            MODEM_SLEEP,
            LIGHT_SLEEP,
            MODEM_SLEEP_LISTEN_INTERVAL,
            POWER_DOWN, // 下电模式，将esp-at的使能引脚拉低
        };

        Esp_At(std::shared_ptr<Bus_Sdio_Guide> bus, int32_t rst)
            : Sdio_Guide(bus), _rst(rst)
        {
        }

        Esp_At(std::shared_ptr<Bus_Sdio_Guide> bus, void (*rst_callback)(bool value))
            : Sdio_Guide(bus), _rst_callback(rst_callback)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        /**
         * @brief None
         * @param mode 使用Sleep_Mode::配置，睡眠模式
         * @param timeout_ms 超时时间，单位ms
         * @return
         * @Date 2025-05-29 18:36:08
         */
        bool set_sleep(Sleep_Mode mode, int16_t timeout_ms = 100);

        bool set_deep_sleep(uint32_t sleep_time_ms, int16_t timeout_ms = 100);

        /**
         * @brief esp_at上电初始化
         * @return
         * @Date 2025-05-09 18:03:26
         */
        bool init_esp_at(void);

        /**
         * @brief 初次连接会返回准备完成信号
         * @return
         * @Date 2025-05-09 18:03:36
         */
        bool init_connect(void);

        bool get_device_id(void);

        /**
         * @brief 重新连接esp_at
         * @return
         * @Date 2025-05-09 18:04:09
         */
        bool reconnect_esp_at(void);

        /**
         * @brief 获取连接状态
         * @return
         * @Date 2025-05-09 18:04:22
         */
        bool get_connect_status(void);

        /**
         * @brief 断言连接错误计数
         * @param count 错误计数的数字可以为正或者负
         * @return
         * @Date 2025-05-09 18:04:30
         */
        void assert_connect_count(int8_t count);

        /**
         * @brief 获取中断
         * @return
         * @Date 2025-03-21 17:11:27
         */
        uint32_t get_irq_flag(void);

        /**
         * @brief 清除中断
         * @param irq_mask 要清除的中断请求位
         * @return
         * @Date 2025-03-21 17:11:35
         */
        bool clear_irq_flag(uint32_t irq_mask);

        /**
         * @brief 断言接收到新包标志
         * @param flag 使用函数get_irq_flag()写入
         * @return [true]: 有接收到新的数据包，[false]: 没有接收到新的数据包
         * @Date 2025-03-21 17:21:59
         */
        bool assert_rx_new_packet_flag(uint32_t flag);

        /**
         * @brief 获取接收数据的长度
         * @return
         * @Date 2025-03-21 17:30:57
         */
        uint32_t get_rx_data_length(void);

        /**
         * @brief 接收包
         * @param *data 包的数据容器
         * @param max_receive_size 最大接收尺寸
         * @return
         * @Date 2025-03-21 17:51:33
         */
        bool receive_packet(std::vector<uint8_t> &data, uint32_t max_receive_size = ESP_AT_MAX_RECEIVE_SIZE);

        /**
         * @brief 接收包
         * @param *data 获取的数据指针
         * @param *byte 获取的数据长度
         * @param max_receive_size 最大接收尺寸
         * @return
         * @Date 2025-03-25 14:38:15
         */
        bool receive_packet(uint8_t *data, size_t *byte, uint32_t max_receive_size = ESP_AT_MAX_RECEIVE_SIZE);

        /**
         * @brief 接收包
         * @param &data 获取的数据指针
         * @param *byte 获取的数据长度
         * @param max_receive_size 最大接收尺寸
         * @return
         * @Date 2025-03-25 14:38:15
         */
        bool receive_packet(std::unique_ptr<uint8_t[]> &data, size_t *byte, uint32_t max_receive_size = ESP_AT_MAX_RECEIVE_SIZE);

        /**
         * @brief 获取发送block数据缓冲区的长度
         * @return
         * @Date 2025-03-24 10:38:54
         */
        uint32_t get_tx_block_buffer_length(void);

        /**
         * @brief 发送包
         * @param *data 数据指针
         * @param byte 数据字节长度
         * @return
         * @Date 2025-03-24 10:47:22
         */
        bool send_packet(const char *data, size_t byte);

        /**
         * @brief 发送包
         * @param data 需要发送的数据字符串
         * @return
         * @Date 2025-03-27 09:38:47
         */
        bool send_packet(const std::string data);

        /**
         * @brief 设置wifi模式
         * @param mode 使用Wifi_Mode::配置
         * @param timeout_ms 超时时间，单位ms
         * @return
         * @Date 2025-03-26 14:03:40
         */
        bool set_wifi_mode(Wifi_Mode mode, int16_t timeout_ms = 100);

        /**
         * @brief wifi扫描，使用之前需要调用函数set_wifi_mode()先将wifi模式设置为STATION模式
         * @param &data wifi_scan值的数据指针
         * @param timeout_ms 超时时间，单位ms
         * @return
         * @Date 2025-03-26 16:26:38
         */
        bool wifi_scan(std::vector<uint8_t> &data, int16_t timeout_ms = 5000);

        /**
         * @brief 等待SDIO总线中断（使用前需要线开启SDIO总线中断）
         * @return
         * @Date 2025-03-27 09:00:05
         */
        bool wait_interrupt(uint32_t timeout_ms);

        /**
         * @brief 设置保存到flash中
         * @param enable [true]：开启保存到falsh中 [false]：关闭保存到falsh中
         * @param timeout_ms 超时时间，单位ms
         * @return
         * @Date 2025-03-27 10:16:06
         */
        bool set_flash_save(bool enable, int16_t timeout_ms = 100);

        /**
         * @brief 设置wifi连接
         * @param ssid wifi名字
         * @param password wifi密码
         * @param timeout_ms 超时时间，单位ms
         * @return
         * @Date 2025-03-27 10:17:09
         */
        bool set_wifi_connect(std::string ssid, std::string password = "", int16_t timeout_ms = 5000);

        /**
         * @brief 获取实时时间
         * @param &time 使用Real_Time结构体配置
         * @param timeout_ms 超时时间
         * @return
         * @Date 2025-05-06 16:07:50
         */
        bool get_real_time(Real_Time &time, int16_t timeout_ms = 3000);
    };
}