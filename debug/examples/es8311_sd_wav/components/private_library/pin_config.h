/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-06 10:32:28
 * @LastEditTime: 2025-05-06 10:01:16
 */
#pragma once

// IIC
#define IIC_1_SDA 7
#define IIC_1_SCL 8
#define IIC_2_SDA 20
#define IIC_2_SCL 21

// BOOT
#define ESP32P4_BOOT 35

// XL9535
#define XL9535_IIC_ADDRESS 0x20
#define XL9535_SDA IIC_1_SDA
#define XL9535_SCL IIC_1_SCL
// #define XL9535_INT 24
#define XL9535_INT 5
// XL9535引脚功能
#define XL9535_VIBRATION_MOTOR_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(0)
#define XL9535_BATTERY_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(1)
#define XL9535_SCREEN_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(2)
#define XL9535_TOUCH_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(3)
#define XL9535_TOUCH_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(4)
#define XL9535_ETHERNET_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(5)
#define XL9535_3_3_V_POWER_EN static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(6)
#define XL9535_EXTERNAL_SENSOR_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(7)
#define XL9535_GPS_PPS static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(10)
#define XL9535_GPS_WAKE_UP static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(11)
#define XL9535_RTC_INT static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(12)
#define XL9535_ESP32C6_WAKE_UP static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(13)
#define XL9535_ESP32C6_EN static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(14)
#define XL9535_LORA_RST static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(16)
#define XL9535_LORA_DIO1 static_cast<Cpp_Bus_Driver::Xl95x5::Pin>(17)

// ES8311 IIC
#define ES8311_IIC_ADDRESS 0x18
#define ES8311_SDA IIC_2_SDA
#define ES8311_SCL IIC_2_SCL
// ES8311 IIS
#define ES8311_ADC_DATA 11
#define ES8311_DAC_DATA 10
#define ES8311_BCLK 12
#define ES8311_MCLK 13
#define ES8311_WS_LRCK 9

// AW86224
#define AW86224_IIC_ADDRESS 0x58
#define AW86224_SDA IIC_2_SDA
#define AW86224_SCL IIC_2_SCL

// SGM38121
#define SGM38121_IIC_ADDRESS 0x28
#define SGM38121_SDA IIC_2_SDA
#define SGM38121_SCL IIC_2_SCL

// PCF8563
#define PCF8563_IIC_ADDRESS 0x51
#define PCF8563_SDA IIC_1_SDA
#define PCF8563_SCL IIC_1_SCL

// BQ27220
#define BQ27220_IIC_ADDRESS 0x55
#define BQ27220_SDA IIC_1_SDA
#define BQ27220_SCL IIC_1_SCL

// SPI
#define SPI_1_SCLK 2
#define SPI_1_MOSI 3
#define SPI_1_MISO 4

// SX1262
// #define LORA_CS 5
#define LORA_CS 24
#define LORA_DIO1 -1
#define LORA_DIO3 -1
#define LORA_RST -1
#define LORA_BUSY 6
#define LORA_SCLK 2
#define LORA_MOSI 3
#define LORA_MISO 4

// L76K
#define GPS_TX 22
#define GPS_RX 23

// ICM20948
#define ICM20948_IIC_ADDRESS 0x68
#define ICM20948_SDA IIC_2_SDA
#define ICM20948_SCL IIC_2_SCL

// HI8561 SCREEN
#define HI8561_SCREEN_BITS_PER_PIXEL_RGB565 16
#define HI8561_SCREEN_BITS_PER_PIXEL_RGB888 24
#define HI8561_SCREEN_MIPI_DSI_DPI_CLK_MHZ 60
// #define HI8561_SCREEN_MIPI_DSI_DPI_CLK_MHZ 45
#define HI8561_SCREEN_WIDTH 540
#define HI8561_SCREEN_HEIGHT 1168
#define HI8561_SCREEN_MIPI_DSI_HSYNC 28
#define HI8561_SCREEN_MIPI_DSI_HBP 26
#define HI8561_SCREEN_MIPI_DSI_HFP 20
#define HI8561_SCREEN_MIPI_DSI_VSYNC 2
#define HI8561_SCREEN_MIPI_DSI_VBP 22
#define HI8561_SCREEN_MIPI_DSI_VFP 200
#define HI8561_SCREEN_DATA_LANE_NUM 2
#define HI8561_SCREEN_LANE_BIT_RATE_MBPS 1000
#define HI8561_SCREEN_BL 51

// HI8561 TOUCH
#define HI8561_TOUCH_IIC_ADDRESS 0x68
#define HI8561_TOUCH_SDA IIC_1_SDA
#define HI8561_TOUCH_SCL IIC_1_SCL

// Camera
// SC2336
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW10_1280x720_30fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW10_1280x720_50fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW10_1280x720_60fps"
// #define SC2336_FORMAT "MIPI_1lane_24Minput_RAW10_1920x1080_25fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW10_1920x1080_25fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW10_1920x1080_30fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW10_800x800_30fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW10_640x480_50fps"
#define SC2336_FORMAT "MIPI_2lane_24Minput_RAW8_1920x1080_30fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW8_1280x720_30fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW8_800x800_30fps"
// #define SC2336_FORMAT "MIPI_2lane_24Minput_RAW8_1024x600_30fps"
// #define SC2336_FORMAT "DVP_8bit_24Minput_RAW10_1280x720_30fps"

#define SC2336_DATA_LANE_NUM 2
#define SC2336_BITS_PER_PIXEL_RGB565 16
#define SC2336_BITS_PER_PIXEL_RGB888 24
#define SC2336_ISP_INPUT_DATA_COLOR_TYPE ISP_COLOR_RAW8

#define SC2336_WIDTH 1920
#define SC2336_HEIGHT 1080

// #define SC2336_WIDTH 640
// #define SC2336_HEIGHT 480

// #define SC2336_WIDTH 1280
// #define SC2336_HEIGHT 720

// #define SC2336_WIDTH 800
// #define SC2336_HEIGHT 800

#define SC2336_SDA IIC_2_SDA
#define SC2336_SCL IIC_2_SCL
#define SC2336_ADDRESS 0x30
#define SC2336_PID 0xCB3A
#define SC2336_LANE_BIT_RATE_MBPS 1000
#define SC2336_MIPI_DSI_DPI_CLK_MHZ 60
#define SC2336_CSI_LANE_BIT_RATE_MBPS 200 // line_rate = pclk * 4
#define SC2336_SCCB_FREQ 100000
#define SC2336_CSI_INPUT_DATA_COLOR_TYPE CAM_CTLR_COLOR_RGB888
#define SC2336_CSI_OUTPUT_DATA_COLOR_TYPE CAM_CTLR_COLOR_RGB888
#define SC2336_ISP_OUTPUT_DATA_COLOR_TYPE ISP_COLOR_RGB888

// SDIO
#define SDIO_1_CLK 43
#define SDIO_1_CMD 44
#define SDIO_1_D0 39
#define SDIO_1_D1 40
#define SDIO_1_D2 41
#define SDIO_1_D3 42

#define SDIO_2_CLK 18
#define SDIO_2_CMD 19
#define SDIO_2_D0 14
#define SDIO_2_D1 15
#define SDIO_2_D2 16
#define SDIO_2_D3 17

// SD
#define SD_BASE_PATH "/sdcard"
// SDMMC
#define SD_SDIO_CLK SDIO_1_CLK
#define SD_SDIO_CMD SDIO_1_CMD
#define SD_SDIO_D0 SDIO_1_D0
#define SD_SDIO_D1 SDIO_1_D1
#define SD_SDIO_D2 SDIO_1_D2
#define SD_SDIO_D3 SDIO_1_D3

// SDSPI
#define SD_SCLK SDIO_1_CLK
#define SD_MOSI SDIO_1_CMD
#define SD_MISO SDIO_1_D0
#define SD_CS SDIO_1_D3


// ESP32C6 SDIO
#define ESP32C6_SDIO_CLK SDIO_2_CLK
#define ESP32C6_SDIO_CMD SDIO_2_CMD
#define ESP32C6_SDIO_D0 SDIO_2_D0
#define ESP32C6_SDIO_D1 SDIO_2_D1
#define ESP32C6_SDIO_D2 SDIO_2_D2
#define ESP32C6_SDIO_D3 SDIO_2_D3

// SCREEN
#define SCREEN_WIDTH HI8561_SCREEN_WIDTH
#define SCREEN_HEIGHT HI8561_SCREEN_HEIGHT