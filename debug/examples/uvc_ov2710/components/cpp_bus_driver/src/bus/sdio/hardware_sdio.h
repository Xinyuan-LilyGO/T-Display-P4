/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-06-16 17:51:12
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Sdio : public Bus_Sdio_Guide
    {
    private:
        static constexpr uint8_t SDIO_BUS_INIT_TIMEOUT_COUNT = 30;

    public:
        enum class Sdio_Port
        {
            SLOT_0 = 0, // 只能用作于固定GPIO口，专用于 UHS-I 模式
            SLOT_1 = 1, // 可以通过GPIO交换矩阵路由，用于任意 GPIO口
        };

    private:
        uint8_t _width;
        int32_t _clk, _cmd, _d0, _d1, _d2, _d3, _d4, _d5, _d6, _d7;
        int32_t _freq_hz;
        Sdio_Port _port;

    public:
        // enum class Frequency
        // {
        //     FREQ_PROBING = 400,       // SD/MMC 探测速度
        //     FREQ_DEFAULT = 20000,     // SD/MMC 默认速度 20MHz
        //     FREQ_26MHZ = 26000,       //   MMC 26MHz速度
        //     FREQ_HIGHSPEED = 40000,   // SD 高速（受时钟分频器限制）
        //     FREQ_DDR_50MHZ = 50000,   //    MMC 50MHz速度
        //     FREQ_52MHZ = 52000,       // MMC 52MHz速度
        //     FREQ_SDR_100MHZ = 100000, // MMC 100MHz速度
        // };

        std::unique_ptr<sdmmc_card_t> _sdio_handle = std::make_unique<sdmmc_card_t>();

        Hardware_Sdio(int32_t clk, int32_t cmd, int32_t d0, int32_t d1 = DEFAULT_CPP_BUS_DRIVER_VALUE,
                      int32_t d2 = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t d3 = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t d4 = DEFAULT_CPP_BUS_DRIVER_VALUE,
                      int32_t d5 = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t d6 = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t d7 = DEFAULT_CPP_BUS_DRIVER_VALUE,
                      Sdio_Port port = Sdio_Port::SLOT_1)
            : _clk(clk), _cmd(cmd), _d0(d0), _d1(d1), _d2(d2), _d3(d3), _d4(d4), _d5(d5), _d6(d6), _d7(d7), _port(port)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool wait_interrupt(uint32_t timeout_ms) override;

        /**
         * @brief 使用 IO_RW_EXTENDED (CMD53) 的字节模式读多个字节
         * @param function 作用号
         * @param write_c32 读取的命令或地址
         * @param *data 读取的数据
         * @param byte 读取的数据长度，数据长度必须小于512个字节，且每4位内存要对齐
         * @return
         * @Date 2025-03-21 15:08:14
         */
        bool read(uint32_t function, uint32_t write_c32, void *data, size_t byte) override;
        /**
         * @brief 使用 IO_RW_DIRECT (CMD52) 读单个字节
         * @param function 作用号
         * @param write_c32 读取的命令或地址
         * @param *data 读取的数据
         * @return
         * @Date 2025-03-21 15:08:14
         */
        bool read(uint32_t function, uint32_t write_c32, uint8_t *data) override;
        /**
         * @brief 块模式下，使用 IO_RW_EXTENDED (CMD53) 读数据块
         * @param function 作用号
         * @param write_c32 读取的命令或地址
         * @param *data 读取的块数据
         * @param byte 读取的块数据长度，目前支持以最大512个字节每块数据传输
         * @return
         * @Date 2025-03-21 15:09:53
         */
        bool read_block(uint32_t function, uint32_t write_c32, void *data, size_t byte) override;
        /**
         * @brief 使用 IO_RW_EXTENDED (CMD53) 的字节模式写多个字节
         * @param function 作用号
         * @param write_c32 写入的命令或地址
         * @param *data 写入的数据
         * @param byte 写入的数据长度，数据长度必须小于512个字节，且每4位内存要对齐
         * @return
         * @Date 2025-03-21 15:08:14
         */
        bool write(uint32_t function, uint32_t write_c32, const void *data, size_t byte) override;
        /**
         * @brief 使用 IO_RW_DIRECT (CMD52) 写单个字节
         * @param function 作用号
         * @param write_c32 写入的命令或地址
         * @param *data 写入的数据
         * @param *read_d8_verify 用于校验的再次读取的数据，填NULL为禁用
         * @return
         * @Date 2025-03-21 15:08:14
         */
        bool write(uint32_t function, uint32_t write_c32, uint8_t data, uint8_t *read_d8_verify = NULL) override;
        /**
         * @brief 块模式下，使用 IO_RW_EXTENDED (CMD53) 写数据块
         * @param function 作用号
         * @param write_c32 写入的命令或地址
         * @param *data 写入的块数据
         * @param byte 写入的块数据长度，目前支持以最大512个字节每块数据传输
         * @return
         * @Date 2025-03-21 15:09:53
         */
        bool write_block(uint32_t function, uint32_t write_c32, const void *data, size_t byte) override;
    };
}
