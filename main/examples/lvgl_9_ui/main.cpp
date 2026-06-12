/*
 * @Description: lvgl_9_ui
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:34:16
 * @LastEditTime: 2026-06-12 09:08:41
 * @License: GPL 3.0
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "t_display_p4_driver.h"
#include "cpp_bus_driver_library.h"
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4
#include "t_display_p4_config.h"
#elif defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
#include "t_display_p4_keyboard_config.h"
#include "st25r3916_driver.h"
#include "kode_bq25896.h"
#endif
#include "RadioLib.h"
#include "radiolib_bridge_driver.h"
#include "lvgl_ui.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "esp_vfs_fat.h"
#include "New Notification 010_c2_b16_s44100.h"
#include "ICM20948_WE.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "ethernet_init.h"
#include "esp_video_init.h"
#if CONFIG_ENABLE_USB_DISPLAY == true
#include "esp_lcd_usb_display.h"
#else
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#endif
#include "app_video.h"
#include "driver/ppa.h"
#include "esp_private/esp_cache_private.h"
#include <fstream>

#ifndef LR2021_CS
#define LR2021_CS 24
#endif

#ifndef LR2021_BUSY
#define LR2021_BUSY 6
#endif

#ifndef LR2021_SCLK
#define LR2021_SCLK 2
#endif

#ifndef LR2021_MOSI
#define LR2021_MOSI 3
#endif

#ifndef LR2021_MISO
#define LR2021_MISO 4
#endif

#ifndef XL9535_LR2021_RST
#define XL9535_LR2021_RST Cpp_Bus_Driver::Xl95x5::Pin::IO16
#endif

#ifndef XL9535_LR2021_DIO1
#define XL9535_LR2021_DIO1 Cpp_Bus_Driver::Xl95x5::Pin::IO17
#endif

#define SD_FILE_PATH_MUSIC "/sdcard/t_display_p4_lvgl_9_ui_resource/music/Erik Satie-Gymnopedie 1-Chase Coleman (piano).wav"

#define LVGL_TICK_PERIOD_MS 1

#ifndef CAMERA_BUFFER_COUNT
#define CAMERA_BUFFER_COUNT CONFIG_EXAMPLE_CAM_BUF_COUNT
#endif

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define NUM_CHANNEL 2

#define PREPEND_STRING "esp32p4 hardware usb cdc receive: "
#define PREPEND_LENGTH 34

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

enum class Es8311_Mode
{
    TEST = 0,
    PLAY_MUSIC,
};

enum class Imu_Mode
{
    TEST = 0,
};

enum class Battery_Health_Mode
{
    TEST = 0,
};

enum class Gps_Mode
{
    TEST = 0,
};

enum class Ethernet_Mode
{
    TEST = 0,
};

struct Ethernet_Info
{
    bool link_up_flag = false;

    struct
    {
        std::string data;

        bool update_flag = false;
    } status;

    struct
    {
        std::string data;

        bool update_flag = false;
    } connect_ip_status;
};

enum class Rtc_Mode
{
    TEST = 0,
    GET_TIME,
};

enum class At_Mode
{
    TEST = 0,
};

// enum class Sleep_Mode
// {
//     NORMAL_SLEEP_TEST,
//     LIGHT_SLEEP_TEST,
// };

enum class Music_File_Read_Speed_Enum
{
    LOW_SPEED,
    HIGH_SPEED,
};

// WAV 文件头结构体
struct Wav_Header
{
    char riff_header[4];      // "RIFF" 标记，表示这是一个 RIFF 文件
    uint32_t riff_size;       // 整个 RIFF 块的大小，不包括 "RIFF" 标记和 riff_size 本身 (文件大小 - 8)
    char wave_header[4];      // "WAVE" 标记，表示这是一个 WAVE 文件
    char fmt_header[4];       // "fmt " 标记，表示这是格式块
    uint32_t fmt_chunk_size;  // 格式块的大小，通常是 16 (PCM) 或 18/40 (有附加信息)
    uint16_t audio_format;    // 音频格式，1 表示 PCM (未压缩)，其他值表示压缩格式
    uint16_t num_channel;     // 声道数，1 表示单声道，2 表示立体声
    uint32_t sample_rate;     // 采样率，例如 44100 Hz, 48000 Hz
    uint32_t byte_rate;       // 字节率，每秒的字节数 (sample_rate * num_channel * bits_per_sample / 8)
    uint16_t block_align;     // 块对齐，每个采样需要的字节数 (num_channel * bits_per_sample / 8)
    uint16_t bits_per_sample; // 位深度，每个采样的位数，例如 8, 16, 24, 32
    char data_header[4];      // "data" 标记，表示这是数据块
    uint32_t data_size;       // 数据块的大小，即音频数据的字节数
};

struct System_Status
{
    struct
    {
        bool init_flag = false;
    } sgm38121;

    struct
    {
        bool init_flag = false;
    } lr2021;

    struct
    {
        bool init_flag = false;
    } camera;

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    struct
    {
        bool init_flag = false;
    } xl9555;

    struct
    {
        bool init_flag = false;
    } tca8418;

    struct
    {
        bool init_flag = false;
    } st25r3916;

    struct
    {
        bool init_flag = false;
    } cc1101;

    struct
    {
        bool init_flag = false;
    } nrf24l01;

    struct
    {
        bool init_flag = false;
    } bq25896;
#endif

    struct
    {
        bool init_flag = false;
    } pcf8563;

    struct
    {
        bool init_flag = false;
    } bq27220;

    struct
    {
        bool init_flag = false;
    } aw86224;

    struct
    {
        bool init_flag = false;
    } es8311;

    struct
    {
        bool init_flag = false;
    } icm20948;

    struct
    {
        bool init_flag = false;
    } l76k;

    struct
    {
        bool init_flag = false;

        bool wifi_connect_status = false;
    } esp32c6;
};

Ethernet_Info Eth_Info;

System_Status Sys_Status;

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
_lock_t lvgl_api_lock;

lv_obj_t *Lvgl_Startup_Progress_Bar;

size_t Cycle_Time = 0;

TaskHandle_t Vibration_Task_Handle = NULL;
TaskHandle_t Speaker_Task_Handle = NULL;
TaskHandle_t Microphone_Task_Handle = NULL;
TaskHandle_t Imu_Task_Handle = NULL;
TaskHandle_t Gps_Task_Handle = NULL;
TaskHandle_t Ethernet_Task_Handle = NULL;
TaskHandle_t At_Task_Handle = NULL;
TaskHandle_t Sleep_Task_Handle = NULL;
TaskHandle_t Rf_Task_Handle = NULL;
TaskHandle_t Iis_Transmission_Data_Stream_Task = NULL;

uint8_t AW86224_Vibration_Play_Count = 0;

Es8311_Mode ES8311_Speaker_Mode = Es8311_Mode::TEST;
Es8311_Mode ES8311_Microphone_Mode = Es8311_Mode::TEST;

bool Music_Play_End_Flag = false;
bool Set_Music_Current_Time_S_Flag = false;
double Set_Music_Current_Time_S = 0;
std::vector<char> Iis_Transmission_Data_Stream;
size_t Iis_Read_Data_Size_Index = 0;
std::ifstream Music_File;
Music_File_Read_Speed_Enum Music_File_Read_Speed = Music_File_Read_Speed_Enum::HIGH_SPEED;

Imu_Mode ICM20948_Imu_Mode = Imu_Mode::TEST;

Gps_Mode L76k_Gps_Mode = Gps_Mode::TEST;

bool L76k_Gps_Positioning_Flag = false;
size_t L76k_Gps_Positioning_Time = 0;

Ethernet_Mode Ip101gri_Ethernet_Mode = Ethernet_Mode::TEST;

At_Mode Esp32c6_At_Mode = At_Mode::TEST;

// Sleep_Mode Esp32p4_Sleep_Mode = Sleep_Mode::LIGHT_SLEEP_TEST;

ppa_client_handle_t ppa_srm_handle = NULL;
size_t data_cache_line_size = 0;
ppa_client_handle_t ppa_srm_handle_2 = NULL;
size_t data_cache_line_size_2 = 0;
void *lcd_buffer[CONFIG_EXAMPLE_CAM_BUF_COUNT];
int32_t fps_count;
int64_t start_time;
int32_t video_cam_fd0;

bool Rf_Send_Flag = false;
uint8_t Rf_Send_Package[255] = {0};

static constexpr uint32_t kGpioLow = 0;
static constexpr uint32_t kGpioHigh = 1;
static constexpr float kLr2021InvalidRssiThresholdDbm = -200.0f;

static const uint32_t lr2021_rfswitch_dio_pins[] = {
    RADIOLIB_NC,
    RADIOLIB_LR2021_DIO6,
    RADIOLIB_LR2021_DIO7,
    RADIOLIB_NC,
    RADIOLIB_NC,
};

static const Module::RfSwitchMode_t lr2021_rfswitch_table[] = {
      // mode               DIO5   DIO6  DIO7
    {LR2021::MODE_STBY, {kGpioLow, kGpioLow, kGpioLow}},
    {LR2021::MODE_RX, {kGpioLow, kGpioLow, kGpioLow}},
    {LR2021::MODE_TX, {kGpioLow, kGpioLow, kGpioLow}},
    {LR2021::MODE_RX_HF, {kGpioLow, kGpioHigh, kGpioLow}},
    {LR2021::MODE_TX_HF, {kGpioLow, kGpioLow, kGpioHigh}},
    END_OF_MODE_TABLE,
};

bool Device_Rf_Task_Stop_Flag = false;

QueueHandle_t app_queue;

esp_lcd_panel_handle_t Screen_Mipi_Dpi_Panel = NULL;

// IIC 1
auto XL9535_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto BQ27220_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ27220_SDA, BQ27220_SCL, I2C_NUM_0);
auto PCF8563_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(PCF8563_SDA, PCF8563_SCL, I2C_NUM_0);

// IIC 2
auto SGM38121_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);
auto AW86224_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);
auto ES8311_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);

// IIS
auto ES8311_IIS_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK, I2S_NUM_0);

// UART
auto L76K_Uart_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Uart>(GPS_RX, GPS_TX, UART_NUM_1);

// SDIO
auto ESP32C6_AT_SDIO_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Sdio>(ESP32C6_SDIO_CLK, ESP32C6_SDIO_CMD,
                                                                           ESP32C6_SDIO_D0, ESP32C6_SDIO_D1, ESP32C6_SDIO_D2, ESP32C6_SDIO_D3, DEFAULT_CPP_BUS_DRIVER_VALUE,
                                                                           DEFAULT_CPP_BUS_DRIVER_VALUE, DEFAULT_CPP_BUS_DRIVER_VALUE, DEFAULT_CPP_BUS_DRIVER_VALUE,
                                                                           Cpp_Bus_Driver::Hardware_Sdio::Sdio_Port::SLOT_1);

// SPI
auto LR2021_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(LR2021_MOSI, LR2021_SCLK, LR2021_MISO, SPI2_HOST, 0);

// IIC 1
auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(XL9535_IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto BQ27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(BQ27220_IIC_Bus, BQ27220_IIC_ADDRESS);
auto PCF8563 = std::make_unique<Cpp_Bus_Driver::Pcf8563x>(PCF8563_IIC_Bus, PCF8563_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

// IIC 2
auto SGM38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(SGM38121_IIC_Bus, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto AW86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(AW86224_IIC_Bus, AW86224_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ES8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(ES8311_IIC_Bus, ES8311_IIS_Bus, ES8311_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ICM20948 = std::make_unique<ICM20948_WE>(&Wire1, ICM20948_IIC_ADDRESS);

// UART
auto L76K = std::make_unique<Cpp_Bus_Driver::L76k>(L76K_Uart_Bus, [](bool Value) -> IRAM_ATTR bool
                                                   { return XL9535->pin_write(XL9535_GPS_WAKE_UP, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(Value)); }, DEFAULT_CPP_BUS_DRIVER_VALUE);

// SDIO
auto ESP32C6_AT = std::make_unique<Cpp_Bus_Driver::Esp_At>(ESP32C6_AT_SDIO_Bus,
                                                           [](bool value) -> IRAM_ATTR void
                                                           {
                                                               // ESP32C6复位
                                                               XL9535->pin_write(XL9535_ESP32C6_EN, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(value));
                                                           });

// SPI
static RadioLibHal *Lr2021_Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(LR2021_SPI_Bus, 10000000, LR2021_CS);
static LR2021 Lr2021 = new Module(Lr2021_Radiolib_Hal,
                                  static_cast<uint32_t>(RADIOLIB_NC),
                                  static_cast<uint32_t>(RADIOLIB_NC),
                                  static_cast<uint32_t>(RADIOLIB_NC),
                                  LR2021_BUSY);

static void lr2021_reset(void)
{
    XL9535->pin_mode(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
}

#if defined SCREEN_ROTATION_DIRECTION_0
auto System_Ui = std::make_unique<Lvgl_Ui::System>(SCREEN_WIDTH, SCREEN_HEIGHT);
#elif defined SCREEN_ROTATION_DIRECTION_90
auto System_Ui = std::make_unique<Lvgl_Ui::System>(SCREEN_HEIGHT, SCREEN_WIDTH);
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

#if defined CONFIG_SCREEN_TYPE_HI8561
auto HI8561_T_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(HI8561_TOUCH_SDA, HI8561_TOUCH_SCL, I2C_NUM_0);

auto HI8561_T = std::make_unique<Cpp_Bus_Driver::Hi8561_Touch>(HI8561_T_IIC_Bus, HI8561_TOUCH_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

#elif defined CONFIG_SCREEN_TYPE_RM69A10

auto GT9895_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(GT9895_TOUCH_SDA, GT9895_TOUCH_SCL, I2C_NUM_0);

auto GT9895 = std::make_unique<Cpp_Bus_Driver::Gt9895>(GT9895_IIC_Bus, GT9895_IIC_ADDRESS, GT9895_X_SCALE_FACTOR, GT9895_Y_SCALE_FACTOR,
                                                       DEFAULT_CPP_BUS_DRIVER_VALUE);
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD

enum class Nfc_Mode
{
    TEST = 0,
};

enum class Cc1101_Rf_Switch
{
    RF_SWITCH_315MHZ,
    RF_SWITCH_434MHZ,
    RF_SWITCH_868_915MHZ,
};

volatile bool TCA8418_Interrupt_Flag = false;
volatile bool Cc1101_Interrupt_Flag = false;
volatile bool Nrf24l01_Interrupt_Flag = false;

bool Device_Nfc_Task_Stop_Flag = false;

TaskHandle_t Nfc_Task_Handle = NULL;

Nfc_Mode St25r3916_Nfc_Mode = Nfc_Mode::TEST;

auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

//  Software IIC
auto XL9555_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(XL9555_SDA, XL9555_SCL);
auto TCA8418_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(TCA8418_SDA, TCA8418_SCL);
auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(BQ25896_SDA, BQ25896_SCL);

// SPI
auto Cc1101_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_CC1101_MOSI, T_MIXRF_CC1101_SCLK, T_MIXRF_CC1101_MISO, SPI2_HOST, 0);
auto Nrf24l01_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_NRF24L01_MOSI, T_MIXRF_NRF24L01_SCLK, T_MIXRF_NRF24L01_MISO, SPI2_HOST, 0);
RadioLibHal *Cc1101_Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(Cc1101_SPI_Bus, 10000000, T_MIXRF_CC1101_CS);
RadioLibHal *Nrf24l01_Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(Nrf24l01_SPI_Bus, 10000000, T_MIXRF_NRF24L01_CS);

//  Software IIC
auto XL9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(XL9555_IIC_Bus, XL9555_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto TCA8418 = std::make_unique<Cpp_Bus_Driver::Tca8418>(TCA8418_IIC_Bus, TCA8418_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

// SPI
CC1101 Cc1101 = new Module(Cc1101_Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC),
                           static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), T_MIXRF_CC1101_BUSY);
nRF24 Nrf24l01 = new Module(Nrf24l01_Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC),
                            static_cast<uint32_t>(T_MIXRF_NRF24L01_INT), static_cast<uint32_t>(T_MIXRF_NRF24L01_CE), static_cast<uint32_t>(RADIOLIB_NC));

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

#endif

#if CONFIG_ENABLE_USB_DISPLAY == true
#else
typedef struct
{
    uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + PREPEND_LENGTH + 1]; // Data buffer
    size_t buf_len;                                                  // Number of bytes received
    uint8_t itf;                                                     // Index of CDC device interface
} app_message_t;

uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
#endif

// esp_err_t register_gpio_wakeup(void)
// {
//     /* Initialize GPIO */
//     gpio_config_t config = {
//         .pin_bit_mask = BIT64(35),
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
// #if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
//         .hys_ctrl_mode = GPIO_HYS_SOFT_ENABLE, /*!< GPIO hysteresis: hysteresis filter on slope input    */
// #endif
//     };
//     ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "Initialize GPIO%d failed", 35);

//     /* Enable wake up from GPIO */
//     ESP_RETURN_ON_ERROR(gpio_wakeup_enable(gpio_num_t(35), GPIO_WAKEUP_LEVEL == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL),
//                         TAG, "Enable gpio wakeup failed");
//     ESP_RETURN_ON_ERROR(esp_sleep_enable_gpio_wakeup(), TAG, "Configure gpio as wakeup source failed");

//     /* Make sure the GPIO is inactive and it won't trigger wakeup immediately */
//     example_wait_gpio_inactive();
//     ESP_LOGI(TAG, "gpio wakeup source is ready");

//     return ESP_OK;
// }

// void Esp_Enter_Light_Sleep(void)
// {
//     register_gpio_wakeup();

//     printf("Entering light sleep\n");
//     /* To make sure the complete line is printed before entering sleep mode,
//      * need to wait until UART TX FIFO is empty:
//      */
//     uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);

//     /* Get timestamp before entering sleep */
//     int64_t t_before_us = esp_timer_get_time();

//     esp_light_sleep_start();

//     /* Get timestamp after waking up from sleep */
//     int64_t t_after_us = esp_timer_get_time();

//     /* Determine wake up reason */
//     const char *wakeup_reason;
//     switch (esp_sleep_get_wakeup_cause())
//     {
//     case ESP_SLEEP_WAKEUP_TIMER:
//         wakeup_reason = "timer";
//         break;
//     case ESP_SLEEP_WAKEUP_GPIO:
//         wakeup_reason = "pin";
//         break;
//     case ESP_SLEEP_WAKEUP_UART:
//         wakeup_reason = "uart";
//         /* Hang-up for a while to switch and execute the uart task
//          * Otherwise the chip may fall sleep again before running uart task */
//         vTaskDelay(1);
//         break;
// #if TOUCH_LSLEEP_SUPPORTED
//     case ESP_SLEEP_WAKEUP_TOUCHPAD:
//         wakeup_reason = "touch";
//         break;
// #endif
//     default:
//         wakeup_reason = "other";
//         break;
//     }
// #if CONFIG_NEWLIB_NANO_FORMAT
//     /* printf in newlib-nano does not support %ll format, causing example test fail */
//     printf("Returned from light sleep, reason: %s, t=%d ms, slept for %d ms\n",
//            wakeup_reason, (int)(t_after_us / 1000), (int)((t_after_us - t_before_us) / 1000));
// #else
//     printf("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
//            wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
// #endif
// }

// void Device_Sleep_Status(bool status)
// {
//     if (status == true)
//     {
//         printf("device sleep start\n");

//         Lr2021.set_sleep();

//         XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);

//         ICM20948->sleep(true);

//         Cpp_Bus_Driver::Es8311::Power_Status ps =
//             {
//                 .contorl =
//                     {
//                         .analog_circuits = false,               // 关闭模拟电路
//                         .analog_bias_circuits = false,          // 关闭模拟偏置电路
//                         .analog_adc_bias_circuits = false,      // 关闭模拟ADC偏置电路
//                         .analog_adc_reference_circuits = false, // 关闭模拟ADC参考电路
//                         .analog_dac_reference_circuit = false,  // 关闭模拟DAC参考电路
//                         .internal_reference_circuits = false,   // 关闭内部参考电路
//                     },
//                 .vmid = Cpp_Bus_Driver::Es8311::Vmid::POWER_DOWN,
//             };
//         ES8311->set_power_status(ps);
//         ES8311->set_pga_power(false);
//         ES8311->set_adc_power(false);
//         ES8311->set_dac_power(false);

//         ESP32C6_AT->set_sleep(Cpp_Bus_Driver::Esp_At::Sleep_Mode::POWER_DOWN);

//         XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

//         SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
//         SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);

//         XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
//         XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

//         // 背光150ma
//         HI8561_T->start_pwm_gradient_time(0, 500);

//         // Esp_Enter_Light_Sleep();
//     }
//     else
//     {
//         printf("device sleep close\n");
//     }
// }

void Save_Real_Time(Cpp_Bus_Driver::Esp_At::Real_Time time)
{
    // 保存实时时间
    Cpp_Bus_Driver::Pcf8563x::Time t =
        {
            .second = time.second,
            .minute = time.minute,
            .hour = static_cast<uint8_t>((time.hour + 8 + 24) % 24),
            .day = time.day,
            .week = Cpp_Bus_Driver::Pcf8563x::Week::SUNDAY,
            .month = time.month,
            .year = static_cast<uint8_t>(time.year - 2000),
        };

    if (time.week == "Sun")
    {
        t.week = Cpp_Bus_Driver::Pcf8563x::Week::SUNDAY;
    }
    else if (time.week == "Mon")
    {
        t.week = Cpp_Bus_Driver::Pcf8563x::Week::MONDAY;
    }
    else if (time.week == "Tue")
    {
        t.week = Cpp_Bus_Driver::Pcf8563x::Week::TUESDAY;
    }
    else if (time.week == "Wed")
    {
        t.week = Cpp_Bus_Driver::Pcf8563x::Week::WEDNESDAY;
    }
    else if (time.week == "Thu")
    {
        t.week = Cpp_Bus_Driver::Pcf8563x::Week::THURSDAY;
    }
    else if (time.week == "Fri")
    {
        t.week = Cpp_Bus_Driver::Pcf8563x::Week::FRIDAY;
    }
    else if (time.week == "Sat")
    {
        t.week = Cpp_Bus_Driver::Pcf8563x::Week::SATURDAY;
    }

    PCF8563->set_time(t);

    System_Ui->_time.week = time.week;
    System_Ui->_time.year = time.year;
    System_Ui->_time.month = time.month;
    System_Ui->_time.day = time.day;
    System_Ui->_time.hour = static_cast<uint8_t>((time.hour + 8 + 24) % 24);
    System_Ui->_time.minute = time.minute;
    System_Ui->_time.second = time.second;
    System_Ui->_time.time_zone = time.time_zone;
}

bool Play_Wav_File(const char *file_path)
{
    Music_File.open(file_path, std::ios::binary);

    if (Music_File.is_open() == false)
    {
        printf("failed to open wav file: %s\n", file_path);
        return false;
    }

    Wav_Header wav_header;
    if (!Music_File.read(reinterpret_cast<char *>(&wav_header), sizeof(wav_header)))
    {
        printf("failed to read wav header\n");
        Music_File.close();
        return false;
    }

    // 分别检查 WAV 文件头的每个部分
    if (strncmp(wav_header.riff_header, "RIFF", 4) != 0)
    {
        printf("invalid wav file format: riff_header is not 'RIFF'\n");
        // Music_File.close();
        // return false;
    }
    else if (strncmp(wav_header.wave_header, "WAVE", 4) != 0)
    {
        printf("invalid wav file format: wave_header is not 'WAVE'\n");
        // Music_File.close();
        // return false;
    }
    else if (strncmp(wav_header.fmt_header, "fmt ", 4) != 0)
    {
        printf("invalid wav file format: fmt_header is not 'fmt '\n");
        // Music_File.close();
        // return false;
    }
    else if (strncmp(wav_header.data_header, "data", 4) != 0)
    {
        printf("invalid wav file format: data_header is not 'data'\n");
        // Music_File.close();
        // return false;
    }

    printf("sample rate: %ld\n", wav_header.sample_rate);
    printf("channels: %d\n", wav_header.num_channel);
    printf("bits per sample: %d\n", wav_header.bits_per_sample);
    printf("data_size: %ld\n", wav_header.data_size);

    // 检查采样率、通道数和位深度是否与 I2S 配置匹配 (如果使用 I2S)
    if (wav_header.sample_rate != SAMPLE_RATE ||
        wav_header.num_channel != NUM_CHANNEL ||
        wav_header.bits_per_sample != BITS_PER_SAMPLE)
    {
        printf("wav file parameters do not match i2s configuration audio may not play correctly\n");
        Music_File.close();
        return false;
    }

    // 计算播放时间
    double duration = 0.0;
    if (wav_header.sample_rate > 0 && wav_header.num_channel > 0 && wav_header.bits_per_sample > 0)
    {
        duration = static_cast<double>(wav_header.data_size) / (wav_header.sample_rate * wav_header.num_channel * (wav_header.bits_per_sample / 8.0));
    }

    printf("duration: %.2f s\n", duration);

    _lock_acquire(&lvgl_api_lock);
    System_Ui->set_win_music_current_total_time(0, duration);
    _lock_release(&lvgl_api_lock);

    // 读取并播放音频数据
    // std::unique_ptr<char[]> data_buffer = std::make_unique<char[]>(1024 * 8);
    // if (data_buffer == nullptr)
    // {
    //     printf("failed to allocate memory for audio buffer\n");
    //     Music_File.close();
    //     return false;
    // }

    size_t cycle_time = 0;

    Iis_Transmission_Data_Stream.clear();
    Iis_Read_Data_Size_Index = 0;

    vTaskResume(Iis_Transmission_Data_Stream_Task);

    while (Music_File.good())
    {
        if (Music_Play_End_Flag == true)
        {
            break;
        }

        if (ES8311_Speaker_Mode == Es8311_Mode::TEST)
        {
            // 播放音乐测试
            ES8311->write_data(c2_b16_s44100, sizeof(c2_b16_s44100));

            ES8311_Speaker_Mode = Es8311_Mode::PLAY_MUSIC;
        }

        if (Set_Music_Current_Time_S_Flag == true)
        {
            printf("music play set current time: %.2f s\n", Set_Music_Current_Time_S);

            // 计算每帧的字节数
            size_t bytes_per_frame = wav_header.num_channel * (wav_header.bits_per_sample / 8);
            // 确保seek_offset是帧的整数倍
            std::streamoff seek_offset = static_cast<std::streamoff>(
                                             Set_Music_Current_Time_S * wav_header.sample_rate) *
                                         bytes_per_frame;
            Music_File.seekg(sizeof(wav_header) + seek_offset, std::ios::beg);

            Iis_Transmission_Data_Stream.clear();
            Iis_Read_Data_Size_Index = 0;

            Set_Music_Current_Time_S_Flag = false;
        }

        if (System_Ui->_registry.win.music.play_flag == true)
        {
            if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::MUSIC)
            {
                // 每隔1秒更新一次音乐播放时间数据
                if (esp_log_timestamp() > cycle_time)
                {
                    std::streamoff current_pos = Music_File.tellg();
                    double current_time = 0.0;
                    if (current_pos > 0)
                    {
                        // 当前数据在文件中的偏移量，减去头部长度
                        std::streamoff data_offset = current_pos - sizeof(wav_header);
                        current_time = static_cast<double>(data_offset) / (wav_header.sample_rate * wav_header.num_channel * (wav_header.bits_per_sample / 8.0));
                        _lock_acquire(&lvgl_api_lock);
                        System_Ui->set_win_music_current_total_time(current_time, duration);
                        _lock_release(&lvgl_api_lock);
                    }

                    printf("music play current time: %.2f s\n", current_time);

                    cycle_time = esp_log_timestamp() + 1000;
                }
            }

            // Music_File.read(data_buffer.get(), 1024 * 8);
            // std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数

            // if (bytes_read > 0)
            // {
            //     ES8311->write_data(data_buffer.get(), bytes_read); // 这一行需要根据你的 I2S 驱动实现来修改
            // }
            // // else
            // // {
            // //     break; // 结束循环，如果读取的字节数为 0
            // // }

            if (Iis_Transmission_Data_Stream.size() > 1024 * 10)
            {
                // 存储数据
                // memcpy(data_buffer.get(), Iis_Transmission_Data_Stream.data(), 1024 * 100);
                // size_t bytes_read = ES8311->write_data(data_buffer.get(), 1024 * 100); // 这一行需要根据你的 I2S 驱动实现来修改
                // if (bytes_read > 0)
                // {
                //     // 删除已经存储的数据
                //     Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.begin(), Iis_Transmission_Data_Stream.begin() + bytes_read);
                // }

                size_t bytes_read = ES8311->write_data(Iis_Transmission_Data_Stream.data() + Iis_Read_Data_Size_Index, 1024 * 10); // 这一行需要根据你的 I2S 驱动实现来修改
                Iis_Read_Data_Size_Index += bytes_read;
                // if (bytes_read > 0)
                // {
                //     // 删除已经存储的数据
                //     Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.begin(), Iis_Transmission_Data_Stream.begin() + bytes_read);
                // }
            }
            // else
            // {
            //     break; // 结束循环，如果读取的字节数为 0
            // }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskSuspend(Iis_Transmission_Data_Stream_Task);
    Iis_Transmission_Data_Stream.clear();
    Iis_Transmission_Data_Stream.shrink_to_fit(); // 释放内存

    Music_File.close();

    System_Ui->_registry.win.music.play_flag = false;

    if (Music_Play_End_Flag == false)
    {
        printf("music play finish\n");

        _lock_acquire(&lvgl_api_lock);
        System_Ui->set_win_music_play_imagebutton_status(System_Ui->_registry.win.music.play_flag);
        System_Ui->set_win_music_current_total_time(0, duration);
        _lock_release(&lvgl_api_lock);
    }
    else
    {
        printf("music play end\n");
        Music_Play_End_Flag = false;
    }

    return true;
}

void lvgl_ui_task(void *arg)
{
    printf("lvgl_ui_task start\n");
    uint32_t time_till_next_ms = 0;

    while (1)
    {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);

        // in case of task watch dog timeout, set the minimal delay to 10ms
        if (time_till_next_ms < 10)
        {
            time_till_next_ms = 10;
        }
        usleep(1000 * time_till_next_ms);

        // lv_timer_handler();
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_vibration_task(void *arg)
{
    printf("device_vibration_task start\n");
    vTaskSuspend(Vibration_Task_Handle);

    while (1)
    {
        if (AW86224_Vibration_Play_Count == static_cast<uint8_t>(-1)) // 开启F0校验
        {
            uint8_t timeout_count = 0;
            uint32_t f0_value = 0;
            bool f0_detection_result = false;

            // 等待F0校准
            while (1)
            {
                f0_value = AW86224->get_f0_detection();
                printf("AW86224 get f0 detection value: %ld\n", f0_value);

                if (AW86224->set_f0_calibrate(f0_value) == true)
                {
                    f0_detection_result = true;
                    break;
                }
                else
                {
                    // 阈值限定
                    if (f0_value > AW86224->_f0_value)
                    {
                        if ((f0_value - AW86224->_f0_value) <= 1500)
                        {
                            f0_detection_result = true;
                            AW86224->_f0_value = f0_value;
                            break;
                        }
                    }
                    else
                    {
                        if ((AW86224->_f0_value - f0_value) <= 1500)
                        {
                            f0_detection_result = true;
                            AW86224->_f0_value = f0_value;
                            break;
                        }
                    }
                }

                timeout_count++;
                if (timeout_count > 5)
                {
                    printf("AW86224 get f0 detection fail\n");
                    f0_detection_result = false;
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }

            // 将触摸数据格式化为字符串
            std::string vibration_data_str = "vibration data:\n";
            vibration_data_str += "f0 value: " + std::to_string(f0_value) + "\n";

            if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CIT_VIBRATION_TEST)
            {
                _lock_acquire(&lvgl_api_lock);
                if (f0_detection_result == false)
                {
                    vibration_data_str += "result: fail\n";
                    lv_obj_set_style_text_color(System_Ui->_registry.win.cit.vibration_test.data_label, lv_color_hex(0xEE2C2C), LV_PART_MAIN);
                }
                else
                {
                    vibration_data_str += "result: success\n";
                    lv_obj_set_style_text_color(System_Ui->_registry.win.cit.vibration_test.data_label, lv_color_hex(0x008B45), LV_PART_MAIN);
                }
                // 更新数据的标签
                lv_label_set_text(System_Ui->_registry.win.cit.vibration_test.data_label, vibration_data_str.c_str());
                _lock_release(&lvgl_api_lock);
            }

            AW86224_Vibration_Play_Count = 0;
        }
        else if (AW86224_Vibration_Play_Count > 0)
        {
            // 启动振动
            AW86224->run_ram_playback_waveform(1, 15, 255);
            vTaskDelay(pdMS_TO_TICKS(50));
            AW86224->stop_ram_playback_waveform();

            AW86224_Vibration_Play_Count--;
        }
        else
        {
            vTaskSuspend(Vibration_Task_Handle);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_speaker_task(void *arg)
{
    printf("device_speaker_task start\n");
    vTaskSuspend(Speaker_Task_Handle);

    while (1)
    {
        switch (ES8311_Speaker_Mode)
        {
        case Es8311_Mode::TEST:
            // 播放音乐测试
            ES8311->write_data(c2_b16_s44100, sizeof(c2_b16_s44100));
            break;
        case Es8311_Mode::PLAY_MUSIC:
            // 播放音乐

            Play_Wav_File(SD_FILE_PATH_MUSIC);
            break;

        default:
            break;
        }

        vTaskSuspend(Speaker_Task_Handle);
    }
}

void device_microphone_task(void *arg)
{
    printf("device_microphone_task start\n");
    vTaskSuspend(Microphone_Task_Handle);

    size_t cycle_time = 0;

    while (1)
    {

        switch (ES8311_Microphone_Mode)
        {
        case Es8311_Mode::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                // 读取麦克风数据
                int16_t microphone_data[1] = {0};
                ES8311->read_data(microphone_data, 1 * sizeof(int16_t));

                if (microphone_data[0] < 0)
                {
                    continue;
                }

                int16_t max_microphone_data = microphone_data[0];
                int16_t max_microphone_data_2 = microphone_data[0];

                if (max_microphone_data >= 1000)
                {
                    max_microphone_data_2 = 1000;
                }
                uint8_t max_microphone_data_percentage = (static_cast<float>(max_microphone_data_2) / static_cast<float>(1000)) * 100;

                // 将麦克风数据格式化为字符串
                std::string microphone_data_str = "microphone data: " + std::to_string(max_microphone_data);

                _lock_acquire(&lvgl_api_lock);
                // 更新麦克风圆盘
                // 使用动画
                lv_anim_t anim;
                lv_anim_init(&anim);
                lv_anim_set_var(&anim, System_Ui->_registry.win.cit.microphone_test.needle_line);
                lv_anim_set_values(&anim, System_Ui->_registry.win.cit.microphone_test.data.value_percentage, max_microphone_data_percentage);
                lv_anim_set_time(&anim, 300); // Animation duration in milliseconds
                lv_anim_set_exec_cb(&anim, [](void *needle, int32_t value)
                                    { lv_scale_set_line_needle_value(System_Ui->_registry.win.cit.microphone_test.scale_line,
                                                                     (lv_obj_t *)needle, 150, value); });
                lv_anim_start(&anim);
                // 不使用动画
                //  lv_scale_set_line_needle_value(System_Ui->_registry.win.cit.microphone_test.scale_line,
                //                                 System_Ui->_registry.win.cit.microphone_test.needle_line, 150, max_microphone_data_percentage);

                // 更新数据的标签
                lv_label_set_text(System_Ui->_registry.win.cit.microphone_test.data.label, microphone_data_str.c_str());
                _lock_release(&lvgl_api_lock);

                System_Ui->_registry.win.cit.microphone_test.data.value_percentage = max_microphone_data_percentage;

                cycle_time = esp_log_timestamp() + 300;
            }
        }
        break;

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_imu_task(void *arg)
{
    printf("device_imu_task start\n");
    vTaskSuspend(Imu_Task_Handle);

    size_t cycle_time = 0;

    while (1)
    {

        switch (ICM20948_Imu_Mode)
        {
        case Imu_Mode::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                // 读取IMU数据
                ICM20948->readSensor();
                xyzFloat gValue;
                ICM20948->getGValues(&gValue);
                xyzFloat angle;
                ICM20948->getAngles(&angle);
                float pitch = ICM20948->getPitch();
                float roll = ICM20948->getRoll();

                // 获取磁力计的 x, y 值以计算航向角（Yaw）
                xyzFloat magValues;
                ICM20948->getMagValues(&magValues);
                float yaw = atan2(magValues.y, magValues.x) * (180.0 / M_PI); // 计算航向角

                // 将IMU数据格式化为字符串
                std::string imu_data_str = "imu data:\n";
                imu_data_str += "gyroscope:\nx: " + std::to_string(gValue.x) + "\ny: " + std::to_string(gValue.y) + "\nz:  " + std::to_string(gValue.z) + "\n\n";
                imu_data_str += "accelerometer:\nx: " + std::to_string(angle.x) + "\ny: " + std::to_string(angle.y) + "\nz: " + std::to_string(angle.z) + "\n\n";
                imu_data_str += "magnetometer:\nx: " + std::to_string(magValues.x) + "\ny: " + std::to_string(magValues.y) + "\nz: " + std::to_string(magValues.z) + "\n\n";
                imu_data_str += "euler angles:\npitch: " + std::to_string(pitch) + "\nroll: " + std::to_string(roll) + "\nyaw: " + std::to_string(yaw);

                _lock_acquire(&lvgl_api_lock);
                // 更新数据的标签
                lv_label_set_text(System_Ui->_registry.win.cit.imu_test.data_label, imu_data_str.c_str());
                _lock_release(&lvgl_api_lock);

                cycle_time = esp_log_timestamp() + 100;
            }
        }
        break;

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_battery_health_task(void *arg)
{
    printf("device_battery_health_task start\n");

    size_t cycle_time = 0;

    while (1)
    {
        if (esp_log_timestamp() > cycle_time)
        {
            // 读取Battery Health数据

            uint16_t battery_level = BQ27220->get_status_of_charge();

            System_Ui->set_battery_level(battery_level);

            _lock_acquire(&lvgl_api_lock);
            System_Ui->status_bar_battery_level_update();
            _lock_release(&lvgl_api_lock);

            switch (System_Ui->get_current_win())
            {
            case Lvgl_Ui::System::Current_Win::CIT_BATTERY_HEALTH_TEST:
            {
                // 将电池数据格式化为字符串
                std::string battery_health_data_str = "battery health data:\n\n";

                battery_health_data_str += "bq27220 data:\n";
                battery_health_data_str += "device id: " + std::to_string(BQ27220->get_device_id()) + "\n\n";

                battery_health_data_str += "design capacity: " + std::to_string(BQ27220->get_design_capacity()) + " mah\n";
                battery_health_data_str += "remaining capacity: " + std::to_string(BQ27220->get_remaining_capacity()) + " mah\n";
                battery_health_data_str += "full charge capacity: " + std::to_string(BQ27220->get_full_charge_capacity()) + " mah\n\n";

                // battery_health_data_str += "raw coulomb count: " + std::to_string(BQ27220->get_raw_coulomb_count()) + " c\n";
                // battery_health_data_str += "cycle count: " + std::to_string(BQ27220->get_cycle_count()) + "\n\n";

                battery_health_data_str += "battery level: " + std::to_string(battery_level) + "%\n";
                battery_health_data_str += "battery health: " + std::to_string(BQ27220->get_status_of_charge()) + "%\n\n";

                battery_health_data_str += "voltage: " + std::to_string(BQ27220->get_voltage()) + " mv\n";
                battery_health_data_str += "current: " + std::to_string(BQ27220->get_current()) + " ma\n";
                // battery_health_data_str += "charging voltage: " + std::to_string(BQ27220->get_charging_voltage()) + " mv\n";
                // battery_health_data_str += "charging current: " + std::to_string(BQ27220->get_charging_current()) + " ma\n";
                // battery_health_data_str += "standby current: " + std::to_string(BQ27220->get_standby_current()) + " ma\n";
                // battery_health_data_str += "max load current: " + std::to_string(BQ27220->get_max_load_current()) + " ma\n";
                // battery_health_data_str += "average power: " + std::to_string(BQ27220->get_average_power()) + " mw\n\n";

                battery_health_data_str += "chip temperature: " + std::to_string(BQ27220->get_chip_temperature_celsius()) + " °c\n\n";
                // battery_health_data_str += "ntc temperature: " + std::to_string(BQ27220->get_temperature_celsius()) + " °c\n\n";

                // battery_health_data_str += "at rate: " + std::to_string(BQ27220->get_at_rate()) + " ma\n";
                // battery_health_data_str += "at rate battery time to empty: " + std::to_string(BQ27220->get_at_rate_time_to_empty()) + " min\n";
                // battery_health_data_str += "battery time to empty: " + std::to_string(BQ27220->get_time_to_empty()) + " min\n";
                // battery_health_data_str += "battery time to full charge: " + std::to_string(BQ27220->get_time_to_full()) + " min\n";
                // battery_health_data_str += "battery standby time to empty: " + std::to_string(BQ27220->get_standby_time_to_empty()) + " min\n";
                // battery_health_data_str += "battery max load time to empty: " + std::to_string(BQ27220->get_max_load_time_to_empty()) + " min\n\n";

                Cpp_Bus_Driver::Bq27220xxxx::Battery_Status bs;
                if (BQ27220->get_battery_status(bs) == true)
                {
                    // battery_health_data_str += "fully discharged flag: " + std::to_string(bs.flag.fd) + "\n";
                    battery_health_data_str += "sleep flag: " + std::to_string(bs.flag.sleep) + "\n";
                    // battery_health_data_str += "charging overheat flag: " + std::to_string(bs.flag.otc) + "\n";
                    // battery_health_data_str += "discharging overheat flag: " + std::to_string(bs.flag.otd) + "\n";
                    // battery_health_data_str += "fully charged flag: " + std::to_string(bs.flag.fc) + "\n";
                    // battery_health_data_str += "charging prohibited flag: " + std::to_string(bs.flag.chginh) + "\n";
                    // battery_health_data_str += "terminate charging alarm flag: " + std::to_string(bs.flag.tca) + "\n";
                    // battery_health_data_str += "terminate discharging alarm flag: " + std::to_string(bs.flag.tda) + "\n";
                    // battery_health_data_str += "battery insertion detection flag: " + std::to_string(bs.flag.auth_gd) + "\n";
                    // battery_health_data_str += "battery present flag: " + std::to_string(bs.flag.battpres) + "\n";
                    battery_health_data_str += "discharge flag: " + std::to_string(bs.flag.dsg) + "\n";
                }

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD

                battery_health_data_str += "\nbq25896 data:\n";
                uint8_t part_number = 0;
                Kode_Bq25896::bq25896_get_part_number(Bq25896_Handle, &part_number);
                battery_health_data_str += "device id: " + std::to_string(part_number) + "\n\n";

                Kode_Bq25896::bq25896_vbus_stat_t vbus_stat;
                Kode_Bq25896::bq25896_get_vbus_status(Bq25896_Handle, &vbus_stat);
                switch (vbus_stat)
                {
                case Kode_Bq25896::BQ25896_VBUS_STAT_NO_INPUT:
                    battery_health_data_str += "vbus status: no input\n";
                    break;
                case Kode_Bq25896::BQ25896_VBUS_STAT_USB_HOST:
                    battery_health_data_str += "vbus status: usb host sdp\n";
                    break;
                case Kode_Bq25896::BQ25896_VBUS_STAT_ADAPTER:
                    battery_health_data_str += "vbus status: adapter (3.25a)\n";
                    break;
                case Kode_Bq25896::BQ25896_VBUS_STAT_OTG:
                    battery_health_data_str += "vbus status: otg\n";
                    break;
                default:
                    battery_health_data_str += "vbus status: unknown\n";
                    break;
                }

                Kode_Bq25896::bq25896_chrg_stat_t chrg_stat;
                Kode_Bq25896::bq25896_get_charging_status(Bq25896_Handle, &chrg_stat);
                switch (chrg_stat)
                {
                case Kode_Bq25896::BQ25896_CHRG_STAT_NOT_CHARGING:
                    battery_health_data_str += "charging status: not charging\n";
                    break;
                case Kode_Bq25896::BQ25896_CHRG_STAT_PRE_CHARGE:
                    battery_health_data_str += "charging status: pre charge\n";
                    break;
                case Kode_Bq25896::BQ25896_CHRG_STAT_FAST_CHARGING:
                    battery_health_data_str += "charging status: fast charging\n";
                    break;
                case Kode_Bq25896::BQ25896_CHRG_STAT_TERM_DONE:
                    battery_health_data_str += "charging status: done charging\n";
                    break;
                default:
                    battery_health_data_str += "charging status: unknown\n";
                    break;
                }

                battery_health_data_str += "\n";

                uint16_t bat_voltage = 0;
                uint16_t sys_voltage = 0;
                uint16_t vbus_voltage = 0;

                Kode_Bq25896::bq25896_get_battery_voltage(Bq25896_Handle, &bat_voltage);
                Kode_Bq25896::bq25896_get_system_voltage(Bq25896_Handle, &sys_voltage);
                Kode_Bq25896::bq25896_get_vbus_voltage(Bq25896_Handle, &vbus_voltage);

                battery_health_data_str += "battery voltage: " + std::to_string(bat_voltage) + "mv\n";
                battery_health_data_str += "system voltage: " + std::to_string(sys_voltage) + "mv\n";
                battery_health_data_str += "vbus voltage: " + std::to_string(vbus_voltage) + "mv\n\n";

                uint16_t charge_current = 0;
                uint16_t ico_current_limit = 0;

                Kode_Bq25896::bq25896_get_charge_current(Bq25896_Handle, &charge_current);
                Kode_Bq25896::bq25896_get_ico_current_limit(Bq25896_Handle, &ico_current_limit);

                battery_health_data_str += "charge current: " + std::to_string(charge_current) + "ma\n";
                battery_health_data_str += "ico current limit: " + std::to_string(ico_current_limit) + "ma\n";

#endif

                _lock_acquire(&lvgl_api_lock);
                // 更新数据的标签
                lv_label_set_text(System_Ui->_registry.win.cit.battery_health_test.data_label, battery_health_data_str.c_str());
                lv_obj_align(System_Ui->_registry.win.cit.battery_health_test.data_label, LV_ALIGN_TOP_MID, 0, 10);
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
                if ((vbus_stat == Kode_Bq25896::BQ25896_VBUS_STAT_ADAPTER) || (vbus_stat == Kode_Bq25896::BQ25896_VBUS_STAT_USB_HOST))
                {
                    lv_obj_add_flag(System_Ui->_registry.win.cit.battery_health_test.otg_label, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(System_Ui->_registry.win.cit.battery_health_test.otg_switch, LV_OBJ_FLAG_HIDDEN);
                }
                else
                {
                    lv_obj_remove_flag(System_Ui->_registry.win.cit.battery_health_test.otg_label, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(System_Ui->_registry.win.cit.battery_health_test.otg_switch, LV_OBJ_FLAG_HIDDEN);

                    lv_obj_align_to(System_Ui->_registry.win.cit.battery_health_test.otg_label,
                                    System_Ui->_registry.win.cit.battery_health_test.data_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
                    lv_obj_align_to(System_Ui->_registry.win.cit.battery_health_test.otg_switch,
                                    System_Ui->_registry.win.cit.battery_health_test.otg_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
                }
#endif

                _lock_release(&lvgl_api_lock);
            }

            break;

            default:
                break;
            }

            cycle_time = esp_log_timestamp() + 1000;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_gps_task(void *arg)
{
    printf("device_gps_task start\n");
    vTaskSuspend(Gps_Task_Handle);

    size_t cycle_time = 0;

    while (1)
    {
        switch (L76k_Gps_Mode)
        {
        case Gps_Mode::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                // 读取Gps数据
                std::unique_ptr<uint8_t[]> buffer;
                uint32_t buffer_length = 0;

                if (L76K->get_info_data(buffer, &buffer_length) == true)
                {
                    // 打印RMC的相关信息
                    Cpp_Bus_Driver::L76k::Rmc rmc;

                    if (L76K->parse_rmc_info(buffer.get(), buffer_length, rmc) == true)
                    {
                        std::string rmc_data_str = "";
                        if (L76k_Gps_Positioning_Flag == false)
                        {
                            L76k_Gps_Positioning_Time++;

                            rmc_data_str = "getting location time: " + std::to_string(L76k_Gps_Positioning_Time) + " s\n\n";
                        }
                        else
                        {
                            rmc_data_str = "location found time: " + std::to_string(L76k_Gps_Positioning_Time) + " s\n\n";
                        }

                        rmc_data_str += "gps data:\nrmc data:\nlocation status: " + rmc.location_status + "\n\n";

                        if (rmc.data.update_flag == true)
                        {
                            rmc_data_str += "utc data: " + std::to_string(rmc.data.year + 2000) + "/" + std::to_string(rmc.data.month) + "/" + std::to_string(rmc.data.day) + "\n";
                            rmc.data.update_flag = false;
                        }
                        if (rmc.utc.update_flag == true)
                        {
                            rmc_data_str += "utc time: " + std::to_string(rmc.utc.hour) + ":" + std::to_string(rmc.utc.minute) + ":" + std::to_string(static_cast<uint8_t>(rmc.utc.second)) + "\n";
                            rmc_data_str += "china time: " + std::to_string((rmc.utc.hour + 8 + 24) % 24) + ":" + std::to_string(rmc.utc.minute) + ":" + std::to_string(static_cast<uint8_t>(rmc.utc.second)) + "\n";
                            rmc.utc.update_flag = false;
                        }

                        rmc_data_str += "\n";

                        if ((rmc.location.lat.update_flag == true) && (rmc.location.lat.direction_update_flag == true))
                        {
                            L76k_Gps_Positioning_Flag = true;

                            rmc_data_str += "lat degrees: " + std::to_string(rmc.location.lat.degrees) + "\n";
                            rmc_data_str += "lat minutes: " + std::to_string(rmc.location.lat.minutes) + "\n";
                            rmc_data_str += "lat degrees_minutes: " + std::to_string(rmc.location.lat.degrees_minutes) + "\n";
                            rmc_data_str += "lat direction: " + rmc.location.lat.direction + "\n";
                            rmc.location.lat.update_flag = false;
                            rmc.location.lat.direction_update_flag = false;
                        }

                        rmc_data_str += "\n";

                        if ((rmc.location.lon.update_flag == true) && (rmc.location.lon.direction_update_flag == true))
                        {
                            L76k_Gps_Positioning_Flag = true;

                            rmc_data_str += "lon degrees: " + std::to_string(rmc.location.lon.degrees) + "\n";
                            rmc_data_str += "lon minutes: " + std::to_string(rmc.location.lon.minutes) + "\n";
                            rmc_data_str += "lon degrees_minutes: " + std::to_string(rmc.location.lon.degrees_minutes) + "\n";
                            rmc_data_str += "lon direction: " + rmc.location.lon.direction + "\n";
                            rmc.location.lon.update_flag = false;
                            rmc.location.lon.direction_update_flag = false;
                        }

                        // 更新数据的标签
                        _lock_acquire(&lvgl_api_lock);
                        lv_label_set_text(System_Ui->_registry.win.cit.gps_test.data_label, rmc_data_str.c_str());
                        _lock_release(&lvgl_api_lock);
                    }
                    else
                    {
                        std::string rmc_data_str = "gps data:\nread fail";

                        // 更新数据的标签
                        _lock_acquire(&lvgl_api_lock);
                        lv_label_set_text(System_Ui->_registry.win.cit.gps_test.data_label, rmc_data_str.c_str());
                        _lock_release(&lvgl_api_lock);
                    }
                }
                else
                {
                    std::string rmc_data_str = "gps data:\nread null";

                    // 更新数据的标签
                    _lock_acquire(&lvgl_api_lock);
                    lv_label_set_text(System_Ui->_registry.win.cit.gps_test.data_label, rmc_data_str.c_str());
                    _lock_release(&lvgl_api_lock);
                }

                cycle_time = esp_log_timestamp() + 1000;
            }
        }
        break;

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_ethernet_task(void *arg)
{
    printf("device_ethernet_task start\n");
    vTaskSuspend(Ethernet_Task_Handle);

    size_t cycle_time = 0;

    while (1)
    {
        switch (Ip101gri_Ethernet_Mode)
        {
        case Ethernet_Mode::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                // 读取Ethernet数据

                if (Eth_Info.status.update_flag == true)
                {
                    std::string ethernet_data_str = "ethernet data:\n" + Eth_Info.status.data + "\n";

                    _lock_acquire(&lvgl_api_lock);
                    // 更新数据的标签
                    lv_label_set_text(System_Ui->_registry.win.cit.ethernet_test.data_label, ethernet_data_str.c_str());
                    _lock_release(&lvgl_api_lock);

                    Eth_Info.status.update_flag = false;
                }

                if (Eth_Info.connect_ip_status.update_flag == true)
                {
                    if (Eth_Info.link_up_flag == true)
                    {
                        std::string ethernet_data_str = "ethernet data:\n" + Eth_Info.status.data + "\n" + Eth_Info.connect_ip_status.data;

                        _lock_acquire(&lvgl_api_lock);
                        // 更新数据的标签
                        lv_label_set_text(System_Ui->_registry.win.cit.ethernet_test.data_label, ethernet_data_str.c_str());
                        _lock_release(&lvgl_api_lock);
                    }

                    Eth_Info.connect_ip_status.update_flag = false;
                }

                cycle_time = esp_log_timestamp() + 1000;
            }
        }
        break;

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_rtc_task(void *arg)
{
    printf("device_rtc_task start\n");

    size_t cycle_time = 0;

    while (1)
    {
        if (esp_log_timestamp() > cycle_time)
        {
            // 读取rtc数据
            Cpp_Bus_Driver::Pcf8563x::Time t;
            if (PCF8563->get_time(t) == true)
            {
                printf("pcf8563 year:[%d] month:[%d] day:[%d] time:[%d:%d:%d] week:[%d]\n", t.year, t.month, t.day,
                       t.hour, t.minute, t.second, static_cast<uint8_t>(t.week));

                System_Ui->set_time(t);

                _lock_acquire(&lvgl_api_lock);
                System_Ui->status_bar_time_update();
                _lock_release(&lvgl_api_lock);

                switch (System_Ui->get_current_win())
                {
                case Lvgl_Ui::System::Current_Win::CIT_RTC_TEST:
                {
                    std::string rtc_data_str = "rtc data:\n";
                    char buffer[100];
                    snprintf(buffer, sizeof(buffer), "week:[%s]\ndata: [%d/%d/%d]\ntime: [%02d:%02d:%02d]\n",
                             System_Ui->_time.week.c_str(), System_Ui->_time.year, System_Ui->_time.month, System_Ui->_time.day,
                             System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);
                    rtc_data_str += buffer;

                    _lock_acquire(&lvgl_api_lock);
                    lv_label_set_text(System_Ui->_registry.win.cit.rtc_test.data_label, rtc_data_str.c_str());
                    _lock_release(&lvgl_api_lock);
                }
                break;
                case Lvgl_Ui::System::Current_Win::HOME:
                    _lock_acquire(&lvgl_api_lock);
                    System_Ui->win_home_time_update();
                    _lock_release(&lvgl_api_lock);
                    break;

                default:
                    break;
                }
            }
            else
            {
                printf("pcf8563 integrity of the clock information is not guaranteed\n");

                if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CIT_RTC_TEST)
                {
                    std::string rtc_data_str = "rtc data:\npcf8563 integrity of the clock\ninformation is not guaranteed\n";

                    _lock_acquire(&lvgl_api_lock);
                    // 更新数据的标签
                    lv_label_set_text(System_Ui->_registry.win.cit.rtc_test.data_label, rtc_data_str.c_str());
                    _lock_release(&lvgl_api_lock);
                }

                PCF8563->clear_clock_integrity_flag();
            }

            cycle_time = esp_log_timestamp() + 1000;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_at_task(void *arg)
{
    printf("device_at_task start\n");
    vTaskSuspend(At_Task_Handle);

    size_t cycle_time = 0;

    while (1)
    {
        switch (Esp32c6_At_Mode)
        {
        case At_Mode::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                Cpp_Bus_Driver::Esp_At::Real_Time rt;
                if (ESP32C6_AT->get_real_time(rt) == true)
                {
                    printf("get_real_time success\n");
                    printf("week: [%s] day: [%d] month: [%d] year: [%d] time: [%02d:%02d:%02d] time zone: [%s] china time: [%02d:%02d:%02d]\n",
                           rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute, rt.second, rt.time_zone.c_str(),
                           (rt.hour + 8 + 24) % 24, rt.minute, rt.second);

                    // 读取At数据
                    std::string at_data_str = "esp32c6 at time data:\n";
                    char buffer[200];
                    snprintf(buffer, sizeof(buffer),
                             "week: [%s]\ndata: [%d/%d/%d]\nchina time: [%02d:%02d:%02d]\n",
                             rt.week.c_str(), rt.year, rt.month, rt.day,
                             (rt.hour + 8 + 24) % 24, rt.minute, rt.second);
                    at_data_str += buffer;

                    _lock_acquire(&lvgl_api_lock);
                    // 更新数据的标签
                    lv_label_set_text(System_Ui->_registry.win.cit.esp32c6_at_test.data_label, at_data_str.c_str());
                    _lock_release(&lvgl_api_lock);

                    Save_Real_Time(rt);
                    _lock_acquire(&lvgl_api_lock);
                    System_Ui->status_bar_wifi_connect_status_update();
                    _lock_release(&lvgl_api_lock);
                }
                else
                {
                    printf("get_real_time fail\n");

                    std::string at_data_str = "esp32c6 at time data:\nget_real_time fail\n";

                    _lock_acquire(&lvgl_api_lock);
                    // 更新数据的标签
                    lv_label_set_text(System_Ui->_registry.win.cit.esp32c6_at_test.data_label, at_data_str.c_str());
                    _lock_release(&lvgl_api_lock);
                }

                if (ESP32C6_AT->get_connect_status() == false)
                {
                    printf("esp32c6 at lost connection,attempting to reconnect\n");
                    std::string at_data_str = "esp32c6 at lost connection,\nattempting to reconnect";

                    _lock_acquire(&lvgl_api_lock);
                    // 更新数据的标签
                    lv_label_set_text(System_Ui->_registry.win.cit.esp32c6_at_test.data_label, at_data_str.c_str());
                    _lock_release(&lvgl_api_lock);

                    _lock_acquire(&lvgl_api_lock);
                    ESP32C6_AT->reconnect_esp_at();
                    _lock_release(&lvgl_api_lock);
                }

                cycle_time = esp_log_timestamp() + 1000;
            }
        }
        break;

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// void esp32p4_sleep_task(void *arg)
// {
//     printf("esp32p4_sleep_task start\n");
//     vTaskSuspend(Sleep_Task_Handle);

//     while (1)
//     {

//             switch (Esp32p4_Sleep_Mode)
//             {
//             case Sleep_Mode::NORMAL_SLEEP_TEST:
//                 /* code */
//                 break;
//             case Sleep_Mode::LIGHT_SLEEP_TEST:
//                 Device_Sleep_Status(true);

//                 break;

//             default:
//                 break;
//             }

//             vTaskSuspend(Sleep_Task_Handle);

//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }

void device_rf_task(void *arg)
{
    printf("device_rf_task start\n");

    size_t auto_send_cycle_time = 0;

    while (1)
    {
        switch (System_Ui->_rf_chip_type)
        {
        case Lvgl_Ui::System::Rf_Chip_Type::LR2021:
        {
            if (System_Ui->_device_lr2021.auto_send.flag == true)
            {
                if (Rf_Send_Flag == false)
                {
                    if (esp_log_timestamp() > auto_send_cycle_time)
                    {
                        memset(Rf_Send_Package, '\0', sizeof(Rf_Send_Package));

                        // 检查长度是否越界
                        if (System_Ui->_device_lr2021.auto_send.text.size() <= 255)
                        {
                            memcpy(Rf_Send_Package, System_Ui->_device_lr2021.auto_send.text.data(), System_Ui->_device_lr2021.auto_send.text.size());
                        }
                        else
                        {
                            // 处理错误：数据过长
                            memcpy(Rf_Send_Package, System_Ui->_device_lr2021.auto_send.text.data(), 254);
                            Rf_Send_Package[254] = '\0';

                            printf("lr2021 send out of bounds(data > Rf_Send_Package)\n");
                        }

                        char buffer_time[15];
                        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                        Lvgl_Ui::System::Win_Rf_Chat_Message wlcm =
                            {
                                .direction = Lvgl_Ui::System::Chat_Message_Direction::SEND,
                                .time = buffer_time,
                                .data = System_Ui->_device_lr2021.auto_send.text,
                            };
                        System_Ui->_registry.win.rf.chat_message_data.push_back(wlcm);

                        if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::RF)
                        {
                            // 更新聊天容器
                            _lock_acquire(&lvgl_api_lock);
                            System_Ui->win_rf_chat_message_data_update(System_Ui->_registry.win.rf.chat_message_data);
                            _lock_release(&lvgl_api_lock);
                        }

                        Rf_Send_Flag = true;

                        auto_send_cycle_time = esp_log_timestamp() + System_Ui->_device_lr2021.auto_send.interval;
                    }
                }
            }

            if (Rf_Send_Flag == true)
            {
                printf("lr2021 send start\n");
                printf("lr2021 send data size: %d\n", strlen(reinterpret_cast<const char *>(Rf_Send_Package)));
                uint16_t timeout_count = 0;
                int16_t assert = Lr2021.startTransmit(Rf_Send_Package, strlen(reinterpret_cast<const char *>(Rf_Send_Package)));
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("lr2021 startTransmit fail (error code: %d)\n", assert);
                }
                else
                {
                    while (1)
                    {
                        if (XL9535->pin_read(XL9535_LR2021_DIO1) == 1)
                        {
                            assert = Lr2021.finishTransmit();
                            if (assert == RADIOLIB_ERR_NONE)
                            {
                                printf("lr2021 send success\n");
                            }
                            else
                            {
                                printf("lr2021 finishTransmit fail (error code: %d)\n", assert);
                            }
                            break;
                        }

                        timeout_count++;
                        if (timeout_count > 1000)
                        {
                            printf("lr2021 send timeout\n");
                            Lr2021.finishTransmit();
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }

                assert = Lr2021.startReceive();
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("lr2021 startReceive fail (error code: %d)\n", assert);
                }

                Rf_Send_Flag = false;
            }

            if (XL9535->pin_read(XL9535_LR2021_DIO1) == 1)
            {
                uint8_t receive_package[255] = {0};
                size_t length_buffer = Lr2021.getPacketLength();
                if (length_buffer > sizeof(receive_package))
                {
                    length_buffer = sizeof(receive_package);
                }

                int16_t assert = Lr2021.readData(receive_package, length_buffer);
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("lr2021 receive fail (error assert: %d)\n", assert);
                }
                else
                {
                    float buffer_rssi = Lr2021.getRSSI();
                    float buffer_snr = Lr2021.getSNR();
                    bool lr2021_rssi_invalid = buffer_rssi < kLr2021InvalidRssiThresholdDbm;
                    if (lr2021_rssi_invalid)
                    {
                        // printf("lr2021 receive rssi[Invalid(%.01f)] snr: %.01f\n", buffer_rssi, buffer_snr);
                        printf("lr2021 receive rssi[0.0(invalid)] snr: %.01f\n", buffer_snr);
                    }
                    else
                    {
                        printf("lr2021 receive rssi: %.01f snr: %.01f\n", buffer_rssi, buffer_snr);
                    }

                    for (size_t i = 0; i < length_buffer; i++)
                    {
                        printf("get lr2021 data[%d]: %d\n", static_cast<int>(i), receive_package[i]);
                    }

                    char buffer_time[15];
                    snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                    std::vector<uint8_t> buffer_vector(receive_package, receive_package + length_buffer);
                    buffer_vector.erase(std::remove(buffer_vector.begin(), buffer_vector.end(), 0), buffer_vector.end());
                    std::string message_str(buffer_vector.begin(), buffer_vector.end());
                    message_str += '\0';

                    char buffer_data_info[30];
                    if (lr2021_rssi_invalid)
                    {
                        // snprintf(buffer_data_info, sizeof(buffer_data_info), "rssi[Invalid(%.01f)] snr[%.01f]", buffer_rssi, buffer_snr);
                        snprintf(buffer_data_info, sizeof(buffer_data_info), "rssi[0.0(invalid)] snr[%.01f]", buffer_snr);
                    }
                    else
                    {
                        snprintf(buffer_data_info, sizeof(buffer_data_info), "rssi[%.01f] snr[%.01f]", buffer_rssi, buffer_snr);
                    }

                    Lvgl_Ui::System::Win_Rf_Chat_Message wlcm =
                        {
                            .direction = Lvgl_Ui::System::Chat_Message_Direction::RECEIVE,
                            .time = buffer_time,
                            .data = message_str,
                            .data_info = buffer_data_info,
                        };
                    System_Ui->_registry.win.rf.chat_message_data.push_back(wlcm);

                    if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::RF)
                    {
                        _lock_acquire(&lvgl_api_lock);
                        System_Ui->win_rf_chat_message_data_update(System_Ui->_registry.win.rf.chat_message_data);
                        _lock_release(&lvgl_api_lock);
                    }
                }

                // 连续接收模式下，readData() 后 LR2021 会继续保持接收。
                // 这里再次调用 startReceive() 可能会让芯片拒绝 SetRx 命令。
            }
        }
        break;
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
        case Lvgl_Ui::System::Rf_Chip_Type::CC1101:
        {
            if (System_Ui->_device_cc1101.auto_send.flag == true)
            {
                if (Rf_Send_Flag == false)
                {
                    if (esp_log_timestamp() > auto_send_cycle_time)
                    {
                        memset(Rf_Send_Package, '\0', sizeof(Rf_Send_Package));

                        // 检查长度是否越界
                        if (System_Ui->_device_cc1101.auto_send.text.size() <= 255)
                        {
                            memcpy(Rf_Send_Package, System_Ui->_device_cc1101.auto_send.text.data(), System_Ui->_device_cc1101.auto_send.text.size());
                        }
                        else
                        {
                            // 处理错误：数据过长
                            memcpy(Rf_Send_Package, System_Ui->_device_cc1101.auto_send.text.data(), 254);
                            Rf_Send_Package[254] = '\0';

                            printf("cc1101 send out of bounds(data > Rf_Send_Package)\n");
                        }

                        char buffer_time[15];
                        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                        Lvgl_Ui::System::Win_Rf_Chat_Message wlcm =
                            {
                                .direction = Lvgl_Ui::System::Chat_Message_Direction::SEND,
                                .time = buffer_time,
                                .data = System_Ui->_device_cc1101.auto_send.text,
                            };
                        System_Ui->_registry.win.rf.chat_message_data.push_back(wlcm);

                        if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::RF)
                        {
                            // 更新聊天容器
                            _lock_acquire(&lvgl_api_lock);
                            System_Ui->win_rf_chat_message_data_update(System_Ui->_registry.win.rf.chat_message_data);
                            _lock_release(&lvgl_api_lock);
                        }

                        Rf_Send_Flag = true;

                        auto_send_cycle_time = esp_log_timestamp() + System_Ui->_device_cc1101.auto_send.interval;
                    }
                }
            }

            if (Rf_Send_Flag == true)
            {
                printf("cc1101 send start\n");
                printf("cc1101 send data size: %d\n", strlen(reinterpret_cast<const char *>(Rf_Send_Package)));
                int16_t assert = Cc1101.transmit(Rf_Send_Package, strlen(reinterpret_cast<const char *>(Rf_Send_Package)));
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("cc1101 transmit fail (error code: %d)\n", assert);
                }

                assert = Cc1101.startReceive();
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("cc1101 startReceive fail (error code: %d)\n", assert);
                }

                Cc1101_Interrupt_Flag = false;

                Rf_Send_Flag = false;
            }

            if (Cc1101_Interrupt_Flag == true) // 接收完成中断
            {
                uint8_t receive_package[255] = {0};
                uint8_t length_buffer = Cc1101.getPacketLength();
                int16_t assert = Cc1101.readData(receive_package, length_buffer);
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("cc1101 receive fail (error assert: %d)\n", assert);
                }
                else
                {
                    float buffer_rssi = Cc1101.getRSSI();
                    uint8_t buffer_lqi = Cc1101.getLQI();
                    printf("cc1101 receive rssi: %.01f lqi: %d\n", buffer_rssi, buffer_lqi);

                    for (uint8_t i = 0; i < length_buffer; i++)
                    {
                        printf("get cc1101 data[%d]: %d\n", i, receive_package[i]);
                    }

                    char buffer_time[15];
                    snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                    // 创建一个 vector 来存储数据，因为 std::remove 需要可修改的序列
                    std::vector<uint8_t> buffer_vector(receive_package, receive_package + length_buffer);

                    // 使用 std::remove 将 \0 字符移除
                    buffer_vector.erase(std::remove(buffer_vector.begin(), buffer_vector.end(), 0), buffer_vector.end());

                    // 使用 string 的构造函数从 vector 创建 string
                    std::string message_str(buffer_vector.begin(), buffer_vector.end());

                    message_str += '\0';

                    char buffer_data_info[30];
                    snprintf(buffer_data_info, sizeof(buffer_data_info), "rssi[%.01f] lqi[%d]", buffer_rssi, buffer_lqi);

                    Lvgl_Ui::System::Win_Rf_Chat_Message wlcm =
                        {
                            .direction = Lvgl_Ui::System::Chat_Message_Direction::RECEIVE,
                            .time = buffer_time,
                            .data = message_str,
                            .data_info = buffer_data_info,
                        };
                    System_Ui->_registry.win.rf.chat_message_data.push_back(wlcm);

                    if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::RF)
                    {
                        // 更新聊天容器
                        _lock_acquire(&lvgl_api_lock);
                        System_Ui->win_rf_chat_message_data_update(System_Ui->_registry.win.rf.chat_message_data);
                        _lock_release(&lvgl_api_lock);
                    }
                }

                assert = Cc1101.startReceive();
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("cc1101 startReceive fail (error code: %d)\n", assert);
                }

                Cc1101_Interrupt_Flag = false;
            }
        }
        break;
        case Lvgl_Ui::System::Rf_Chip_Type::NRF24L01:
        {
            if (System_Ui->_device_nrf24l01.auto_send.flag == true)
            {
                if (Rf_Send_Flag == false)
                {
                    if (esp_log_timestamp() > auto_send_cycle_time)
                    {
                        memset(Rf_Send_Package, '\0', sizeof(Rf_Send_Package));

                        // 检查长度是否越界
                        if (System_Ui->_device_nrf24l01.auto_send.text.size() <= 255)
                        {
                            memcpy(Rf_Send_Package, System_Ui->_device_nrf24l01.auto_send.text.data(), System_Ui->_device_nrf24l01.auto_send.text.size());
                        }
                        else
                        {
                            // 处理错误：数据过长
                            memcpy(Rf_Send_Package, System_Ui->_device_nrf24l01.auto_send.text.data(), 254);
                            Rf_Send_Package[254] = '\0';

                            printf("nrf24l01 send out of bounds(data > Rf_Send_Package)\n");
                        }

                        char buffer_time[15];
                        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                        Lvgl_Ui::System::Win_Rf_Chat_Message wlcm =
                            {
                                .direction = Lvgl_Ui::System::Chat_Message_Direction::SEND,
                                .time = buffer_time,
                                .data = System_Ui->_device_nrf24l01.auto_send.text,
                            };
                        System_Ui->_registry.win.rf.chat_message_data.push_back(wlcm);

                        if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::RF)
                        {
                            // 更新聊天容器
                            _lock_acquire(&lvgl_api_lock);
                            System_Ui->win_rf_chat_message_data_update(System_Ui->_registry.win.rf.chat_message_data);
                            _lock_release(&lvgl_api_lock);
                        }

                        Rf_Send_Flag = true;

                        auto_send_cycle_time = esp_log_timestamp() + System_Ui->_device_nrf24l01.auto_send.interval;
                    }
                }
            }

            if (Rf_Send_Flag == true)
            {
                printf("nrf24l01 send start\n");
                printf("nrf24l01 send data size: %d\n", strlen(reinterpret_cast<const char *>(Rf_Send_Package)));
                int16_t assert = Nrf24l01.transmit(Rf_Send_Package, strlen(reinterpret_cast<const char *>(Rf_Send_Package)), 0);
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("nrf24l01 transmit fail (error code: %d)\n", assert);
                }

                assert = Nrf24l01.startReceive();
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("nrf24l01 startReceive fail (error code: %d)\n", assert);
                }

                Nrf24l01_Interrupt_Flag = false;

                Rf_Send_Flag = false;
            }

            if (Nrf24l01_Interrupt_Flag == true) // 接收完成中断
            {
                uint8_t receive_package[255] = {0};
                uint8_t length_buffer = Nrf24l01.getPacketLength();
                int16_t assert = Nrf24l01.readData(receive_package, length_buffer);
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("nrf24l01 receive fail (error assert: %d)\n", assert);
                }
                else
                {
                    float buffer_rssi = Nrf24l01.getRSSI();
                    float buffer_lqi = Nrf24l01.getSNR();
                    printf("nrf24l01 receive rssi: %.01f snr: %.01f\n", buffer_rssi, buffer_lqi);

                    for (uint8_t i = 0; i < length_buffer; i++)
                    {
                        printf("get nrf24l01 data[%d]: %d\n", i, receive_package[i]);
                    }

                    char buffer_time[15];
                    snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                    // 创建一个 vector 来存储数据，因为 std::remove 需要可修改的序列
                    std::vector<uint8_t> buffer_vector(receive_package, receive_package + length_buffer);

                    // 使用 std::remove 将 \0 字符移除
                    buffer_vector.erase(std::remove(buffer_vector.begin(), buffer_vector.end(), 0), buffer_vector.end());

                    // 使用 string 的构造函数从 vector 创建 string
                    std::string message_str(buffer_vector.begin(), buffer_vector.end());

                    message_str += '\0';

                    char buffer_data_info[30];
                    snprintf(buffer_data_info, sizeof(buffer_data_info), "rssi[%.01f] snr[%.01f]", buffer_rssi, buffer_lqi);

                    Lvgl_Ui::System::Win_Rf_Chat_Message wlcm =
                        {
                            .direction = Lvgl_Ui::System::Chat_Message_Direction::RECEIVE,
                            .time = buffer_time,
                            .data = message_str,
                            .data_info = buffer_data_info,
                        };
                    System_Ui->_registry.win.rf.chat_message_data.push_back(wlcm);

                    if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::RF)
                    {
                        // 更新聊天容器
                        _lock_acquire(&lvgl_api_lock);
                        System_Ui->win_rf_chat_message_data_update(System_Ui->_registry.win.rf.chat_message_data);
                        _lock_release(&lvgl_api_lock);
                    }
                }

                assert = Nrf24l01.startReceive();
                if (assert != RADIOLIB_ERR_NONE)
                {
                    printf("nrf24l01 startReceive fail (error code: %d)\n", assert);
                }

                Nrf24l01_Interrupt_Flag = false;
            }
        }
        break;
#endif
        default:
            break;
        }

        // 如果有触发停止标志就等待一次发送或接收过程完成后再停止
        // 这样做为了防止spi意外终止导致的iic的0x107错误
        // 多任务处理spi和iic不能同时工作，spi工作的时候有概率会导致iic死机
        if (Device_Rf_Task_Stop_Flag == true)
        {
            vTaskSuspend(Rf_Task_Handle);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void iis_transmission_data_stream_task(void *arg)
{
    printf("iis_transmission_data_stream_task start\n");

    size_t cycle_time = 0;
    size_t cycle_time_2 = 0;

    vTaskSuspend(Iis_Transmission_Data_Stream_Task);

    // 读取音频数据
    // std::unique_ptr<char[]> data_buffer = std::make_unique<char[]>(1024 * 10);
    // if (data_buffer == nullptr)
    // {
    //     printf("failed to allocate memory for audio buffer\n");
    // }

    while (1)
    {
        // 限制读取速度
        if (esp_log_timestamp() > cycle_time)
        {
            if (Music_File.good())
            {
                // 限制流的最大长度
                // if (Iis_Transmission_Data_Stream.size() < 1024 * 1000)
                // {
                //     Music_File.read(data_buffer.get(), 1024 * 10);
                //     std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数

                //     if (bytes_read > 0)
                //     {
                //         const auto current_buf_size = Iis_Transmission_Data_Stream.size();
                //         // 调整容量
                //         Iis_Transmission_Data_Stream.resize(current_buf_size + bytes_read);
                //         // 存储数据
                //         // memcpy拷贝的是字节数据
                //         memcpy(Iis_Transmission_Data_Stream.data() + current_buf_size, data_buffer.get(), bytes_read);
                //     }
                // }

                // 限制流的最大长度
                const auto current_buf_size = Iis_Transmission_Data_Stream.size();
                if (current_buf_size < 1024 * 300)
                {
                    // printf("current_buf_size: %d\n", current_buf_size);

                    // 调整容量
                    Iis_Transmission_Data_Stream.resize(current_buf_size + 1024 * 20);

                    Music_File.read(Iis_Transmission_Data_Stream.data() + current_buf_size, 1024 * 20);
                    std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数
                    // 如果实际读取的字节数小于预期，则从末尾扣除多余的空间
                    if (bytes_read < 1024 * 20)
                    {
                        Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.end() - (1024 * 20 - bytes_read), Iis_Transmission_Data_Stream.end());
                    }
                }

                // const auto current_buf_size = Iis_Transmission_Data_Stream.size();
                // if (current_buf_size >= 1024 * 400)
                // {
                //     Music_File_Read_Speed = Music_File_Read_Speed_Enum::LOW_SPEED;
                // }
                // if (current_buf_size <= 1024 * 200)
                // {
                //     Music_File_Read_Speed = Music_File_Read_Speed_Enum::HIGH_SPEED;
                // }

                // switch (Music_File_Read_Speed)
                // {
                // case Music_File_Read_Speed_Enum::LOW_SPEED:
                // {
                //     if (current_buf_size < 1024 * 600)
                //     {
                //         printf("LOW_SPEED current_buf_size: %d\n", current_buf_size);

                //         // 调整容量
                //         Iis_Transmission_Data_Stream.resize(current_buf_size + 1024 * 5);

                //         Music_File.read(Iis_Transmission_Data_Stream.data() + current_buf_size, 1024 * 5);
                //         std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数
                //         // 如果实际读取的字节数小于预期，则从末尾扣除多余的空间
                //         if (bytes_read < 1024 * 5)
                //         {
                //             Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.end() - (1024 * 5 - bytes_read), Iis_Transmission_Data_Stream.end());
                //         }

                //         // Music_File.read(data_buffer.get(), 1024 * 5);
                //         // std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数
                //         // if (bytes_read > 0)
                //         // {
                //         //     current_buf_size = Iis_Transmission_Data_Stream.size();
                //         //     // 调整容量
                //         //     Iis_Transmission_Data_Stream.resize(current_buf_size + bytes_read);
                //         //     // 存储数据
                //         //     // memcpy拷贝的是字节数据
                //         //     memcpy(Iis_Transmission_Data_Stream.data() + current_buf_size, data_buffer.get(), bytes_read);
                //         // }
                //     }

                //     break;
                // }
                // case Music_File_Read_Speed_Enum::HIGH_SPEED:
                // {
                //     if (current_buf_size < 1024 * 600)
                //     {
                //         printf("HIGH_SPEED current_buf_size: %d\n", current_buf_size);

                //         // 调整容量
                //         Iis_Transmission_Data_Stream.resize(current_buf_size + 1024 * 10);

                //         Music_File.read(Iis_Transmission_Data_Stream.data() + current_buf_size, 1024 * 10);
                //         std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数
                //         // 如果实际读取的字节数小于预期，则从末尾扣除多余的空间
                //         if (bytes_read < 1024 * 10)
                //         {
                //             Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.end() - (1024 * 10 - bytes_read), Iis_Transmission_Data_Stream.end());
                //         }

                //         // Music_File.read(data_buffer.get(), 1024 * 10);
                //         // std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数
                //         // if (bytes_read > 0)
                //         // {
                //         //     current_buf_size = Iis_Transmission_Data_Stream.size();
                //         //     // 调整容量
                //         //     Iis_Transmission_Data_Stream.resize(current_buf_size + bytes_read);
                //         //     // 存储数据
                //         //     // memcpy拷贝的是字节数据
                //         //     memcpy(Iis_Transmission_Data_Stream.data() + current_buf_size, data_buffer.get(), bytes_read);
                //         // }
                //     }
                //     break;
                // }
                // default:
                //     break;
                // }
            }

            cycle_time = esp_log_timestamp() + 30;
        }

        if (Music_File.good())
        {
            if (Iis_Read_Data_Size_Index > 1024 * 250)
            {
                // 删除已经存储的数据
                Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.begin(), Iis_Transmission_Data_Stream.begin() + 1024 * 250);
                Iis_Read_Data_Size_Index -= 1024 * 250;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void bsp_init_refresh_monitor_io(void)
{
    // gpio_config_t monitor_io_conf = {
    //     .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_REFRESH_MONITOR,
    //     .mode = GPIO_MODE_OUTPUT,
    // };
    // ESP_ERROR_CHECK(gpio_config(&monitor_io_conf));
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    static size_t edge_touch_scheduled_shutdown_time = 0;
    static size_t edge_touch_scheduled_shutdown_lock = false;

    if (edge_touch_scheduled_shutdown_lock == true)
    {
        if (esp_log_timestamp() > edge_touch_scheduled_shutdown_time)
        {
            System_Ui->_edge_touch_flag = false;
            edge_touch_scheduled_shutdown_lock = false;
        }
    }

// if (XL9535->pin_read(XL9535_TOUCH_INT) == 0)
// {
#if defined CONFIG_SCREEN_TYPE_HI8561

    Cpp_Bus_Driver::Hi8561_Touch::Touch_Point tp;

    if (HI8561_T->get_multiple_touch_point(tp) == true)
    {
        // printf("finger_count: %d edge_touch_flag: %d\nx: %d y: %d pressure_value: %d\n",
        //        tp.finger_count, tp.edge_touch_flag, tp.info[0].x, tp.info[0].y, tp.info[0].pressure_value);

        // printf("touch finger: %d edge touch flag: %d\n", tp.finger_count, tp.edge_touch_flag);
        // for (uint8_t i = 0; i < tp.info.size(); i++)
        // {
        //     printf("touch num [%d] x: %d y: %d p: %d\n", i + 1, tp.info[i].x, tp.info[i].y, tp.info[i].pressure_value);
        // }

        if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CIT_TOUCH_TEST)
        {
            /*Set the coordinates*/
            data->point.x = tp.info[0].x;
            data->point.y = tp.info[0].y;

            data->state = LV_INDEV_STATE_PR;
        }
        else
        {
            if ((tp.finger_count == 1) && (tp.info[0].x != static_cast<uint16_t>(-1)) && (tp.info[0].y != static_cast<uint16_t>(-1)) && (tp.info[0].pressure_value != 0))
            {
                /*Set the coordinates*/
                data->point.x = tp.info[0].x;
                data->point.y = tp.info[0].y;

                data->state = LV_INDEV_STATE_PR;
            }
            else
            {
                data->state = LV_INDEV_STATE_REL;
            }
        }

        if (tp.edge_touch_flag == true)
        {
            System_Ui->_edge_touch_flag = true;

            edge_touch_scheduled_shutdown_time = esp_log_timestamp() + 100;
            edge_touch_scheduled_shutdown_lock = true;
        }

        System_Ui->_touch_point = tp;

        tp.info.clear();
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

#elif defined CONFIG_SCREEN_TYPE_RM69A10

    Cpp_Bus_Driver::Gt9895::Touch_Point tp;

    if (GT9895->get_multiple_touch_point(tp) == true)
    {
        if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CIT_TOUCH_TEST)
        {
            /*Set the coordinates*/
            data->point.x = tp.info[0].x;
            data->point.y = tp.info[0].y;

            data->state = LV_INDEV_STATE_PR;
        }
        else
        {
            if ((tp.finger_count == 1) && (tp.info[0].x != static_cast<uint16_t>(-1)) && (tp.info[0].y != static_cast<uint16_t>(-1)) && (tp.info[0].pressure_value != 0))
            {
                /*Set the coordinates*/
                data->point.x = tp.info[0].x;
                data->point.y = tp.info[0].y;

                data->state = LV_INDEV_STATE_PR;
            }
            else
            {
                data->state = LV_INDEV_STATE_REL;
            }
        }

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4
        if (tp.edge_touch_flag == true)
        {
            System_Ui->_edge_touch_flag = true;

            edge_touch_scheduled_shutdown_time = esp_log_timestamp() + 100;
            edge_touch_scheduled_shutdown_lock = true;
        }
#elif defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD

        if ((tp.info[0].y < 20) || ((tp.info[0].y > SCREEN_HEIGHT - 20) && (tp.info[0].y <= SCREEN_HEIGHT)))
        {
            tp.edge_touch_flag = true;
            System_Ui->_edge_touch_flag = true;

            edge_touch_scheduled_shutdown_time = esp_log_timestamp() + 200;
            edge_touch_scheduled_shutdown_lock = true;
        }

#else
#error "unknown macro definition, please select the correct macro definition."
#endif

        System_Ui->_touch_point = tp;

        tp.info.clear();
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

#else
#error "unknown macro definition, please select the correct macro definition."
#endif

    // }
}

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
void my_keyboard_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint32_t last_key = 0; // 静态变量记录上一次按键
    static bool pressed_state_flag = false;
    static bool caps_lock_flag = false;
    static bool shift_press_flag = false;

    if (TCA8418_Interrupt_Flag == true)
    {
        Cpp_Bus_Driver::Tca8418::Irq_Status is;

        if (TCA8418->parse_irq_status(TCA8418->get_irq_flag(), is) == false)
        {
            printf("parse_irq_status fail\n");
        }
        else
        {
            if (is.key_events_flag == true)
            {
                Cpp_Bus_Driver::Tca8418::Touch_Point tp;
                if (TCA8418->get_multiple_touch_point(tp) == true)
                {
                    // printf("touch finger: %d\n", tp.finger_count);

                    for (uint8_t i = 0; i < tp.info.size(); i++)
                    {
                        switch (tp.info[i].event_type)
                        {
                        case Cpp_Bus_Driver::Tca8418::Event_Type::KEYPAD:
                        {
                            Cpp_Bus_Driver::Tca8418::Touch_Position tp_2;
                            if (TCA8418->parse_touch_num(tp.info[i].num, tp_2) == true)
                            {
                                // printf("keypad event\n");
                                // printf("   touch num:[%d] num: %d x: %d y: %d press_flag: %d\n", i + 1, tp.info[i].num, tp_2.x, tp_2.y, tp.info[i].press_flag);
                                if (tp.info[i].num <= (sizeof(Tca8418_Map) / sizeof(std::string)))
                                {
                                    // printf("   touch string: %s\n", Tca8418_Map[tp.info[i].num - 1].c_str());

                                    if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CIT_KEYBOARD_TEST)
                                    {
                                        lv_label_set_text(System_Ui->_registry.win.cit.keyboard_test.data_label, Tca8418_Map[tp.info[i].num - 1].c_str());
                                    }
                                }

                                if (tp.info[i].press_flag == 1)
                                {
                                    pressed_state_flag = true;
                                    if (Tca8418_Map[tp.info[i].num - 1] == "Caps")
                                    {
                                        caps_lock_flag = !caps_lock_flag;
                                        if (caps_lock_flag == false)
                                        {
                                            XL9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH); // 关闭LED
                                            XL9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
                                            XL9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
                                        }
                                        else
                                        {
                                            XL9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::LOW); // 开启LED
                                            XL9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
                                            XL9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::LOW);
                                        }
                                    }

                                    if (Tca8418_Map[tp.info[i].num - 1] == "Shift")
                                    {
                                        shift_press_flag = true;
                                    }

                                    if (shift_press_flag == false)
                                    {
                                        last_key = Tca8418_Map_Lvgl[tp.info[i].num - 1]; // 保存最后按下的键
                                        if (caps_lock_flag == true)
                                        {
                                            // 如果是小写字母，转为大写
                                            if (last_key >= 'a' && last_key <= 'z')
                                            {
                                                last_key = last_key - 'a' + 'A';
                                            }
                                        }
                                    }
                                    else
                                    {
                                        last_key = Tca8418_Map_Lvgl_Shift[tp.info[i].num - 1]; // 保存最后按下的键
                                    }
                                }
                                else
                                {
                                    pressed_state_flag = false;

                                    if (Tca8418_Map[tp.info[i].num - 1] == "Shift")
                                    {
                                        shift_press_flag = false;
                                    }
                                }
                            }

                            break;
                        }
                        case Cpp_Bus_Driver::Tca8418::Event_Type::GPIO:
                            // printf("gpio event\n");
                            // printf("   touch num:[%d] num: %d press_flag: %d\n", i + 1, tp.info[i].num, tp.info[i].press_flag);
                            break;

                        default:
                            break;
                        }
                    }
                }

                TCA8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);
            }
        }

        TCA8418_Interrupt_Flag = false;
    }

    if (pressed_state_flag == false)
    {
        data->state = LV_INDEV_STATE_RELEASED; // 释放状态
    }
    else
    {
        data->state = LV_INDEV_STATE_PRESSED; // 按下状态

        data->key = last_key; // 当前按下的键值
    }
}

void device_nfc_task(void *arg)
{
    printf("device_nfc_task start\n");
    vTaskSuspend(Nfc_Task_Handle);

    size_t cycle_time = 0;

    while (1)
    {
        switch (St25r3916_Nfc_Mode)
        {
        case Nfc_Mode::TEST:
        {
            St25r3916_Loop();
        }
        break;

        default:
            break;
        }

        // 如果有触发停止标志就等待一次发送或接收过程完成后再停止
        // 这样做为了防止spi意外终止导致的iic的0x107错误
        // 多任务处理spi和iic不能同时工作，spi工作的时候有概率会导致iic死机
        if (Device_Nfc_Task_Stop_Flag == true)
        {
            vTaskSuspend(Nfc_Task_Handle);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Cc1101_Rf_Switch_Control(Cc1101_Rf_Switch rf_switch)
{
    switch (rf_switch)
    {
    case Cc1101_Rf_Switch::RF_SWITCH_315MHZ:
        XL9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        XL9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        break;
    case Cc1101_Rf_Switch::RF_SWITCH_434MHZ:
        XL9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        XL9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        break;
    case Cc1101_Rf_Switch::RF_SWITCH_868_915MHZ:
        XL9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        XL9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        break;

    default:
        printf("unknown rf switch\n");
        break;
    }
}

bool Set_T_Mixrf_Lr1121_Sleep()
{
    XL9555->pin_mode(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    // XL9555->pin_write(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    // vTaskDelay(pdMS_TO_TICKS(10));
    XL9555->pin_write(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    // vTaskDelay(pdMS_TO_TICKS(10));
    // XL9555->pin_write(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    // vTaskDelay(pdMS_TO_TICKS(10));

    // XL9555->pin_mode(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    // XL9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    // auto lr1121_spi_bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_LR1121_MOSI, T_MIXRF_LR1121_SCLK, T_MIXRF_LR1121_MISO, SPI2_HOST, 0);
    // RadioLibHal *lr1121_radiolib_hal = new Radiolib_Cpp_Bus_Driver_Hal(lr1121_spi_bus, 10000000, -1);
    // LR1121 lr1121 = new Module(lr1121_radiolib_hal, static_cast<uint32_t>(RADIOLIB_NC),
    //                            static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC));

    // XL9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    // int16_t assert = lr1121.begin(434.0, 125.0, 9, 7, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, 10, 8, 3.3);
    // if (assert == RADIOLIB_ERR_NONE)
    // {
    //     printf("lr1121 init success\n");
    // }
    // else
    // {
    //     printf("lr1121 init fail (error code: %d)\n", assert);
    //     XL9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    //     return false;
    // }
    // assert = lr1121.sleep();
    // if (assert != RADIOLIB_ERR_NONE)
    // {
    //     printf("lr1121 sleep fail (error code: %d)\n", assert);
    //     XL9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    //     return false;
    // }

    // XL9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    return true;
}

#endif

bool Sdmmc_Init(const char *base_path)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config =
        {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,
        };

    sdmmc_card_t *card;

    printf("initializing sd card\n");
    printf("using sdmmc peripheral\n");

    sd_pwr_ctrl_ldo_config_t ldo_config =
        {
            .ldo_chan_id = 4,
        };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    int32_t assert = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (assert != ESP_OK)
    {
        printf("failed to create a new on-chip ldo power control driver\n");
        // return false;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;

    host.max_freq_khz = SDMMC_FREQ_52M;

    host.pwr_ctrl_handle = pwr_ctrl_handle;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.clk = static_cast<gpio_num_t>(SD_SDIO_CLK);
    slot_config.cmd = static_cast<gpio_num_t>(SD_SDIO_CMD);
    slot_config.d0 = static_cast<gpio_num_t>(SD_SDIO_D0);
    slot_config.d1 = static_cast<gpio_num_t>(SD_SDIO_D1);
    slot_config.d2 = static_cast<gpio_num_t>(SD_SDIO_D2);
    slot_config.d3 = static_cast<gpio_num_t>(SD_SDIO_D3);

    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    printf("mounting filesystem\n");

    assert = esp_vfs_fat_sdmmc_mount(base_path, &host, &slot_config, &mount_config, &card);
    if (assert != ESP_OK)
    {
        printf("failed to mount filesystem\n");
        return false;
    }

    printf("filesystem mounted\n");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return true;
}

bool Sdspi_Init(const char *base_path)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config =
        {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,
        };

    sdmmc_card_t *card;

    printf("initializing sd card\n");
    printf("using sdspi peripheral\n");

    sd_pwr_ctrl_ldo_config_t ldo_config =
        {
            .ldo_chan_id = 4,
        };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    int32_t assert = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (assert != ESP_OK)
    {
        printf("failed to create a new on-chip ldo power control driver\n");
        return false;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;

    // host.max_freq_khz=SDMMC_FREQ_52M;

    host.pwr_ctrl_handle = pwr_ctrl_handle;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };

    assert = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (assert != ESP_OK)
    {
        printf("failed to initialize bus\n");
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = SPI3_HOST;
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = host.slot;

    printf("mounting filesystem\n");

    assert = esp_vfs_fat_sdspi_mount(base_path, &host, &slot_config, &mount_config, &card);
    if (assert != ESP_OK)
    {
        printf("failed to mount filesystem\n");
        return false;
    }

    printf("filesystem mounted\n");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return true;
}

void System_Ui_Callback_Init(void)
{
    System_Ui->_device_vibration_callback = [](uint8_t vibration_count)
    {
        AW86224_Vibration_Play_Count = vibration_count;
        vTaskResume(Vibration_Task_Handle);
    };

    System_Ui->_win_cit_speaker_test_callback = [](void)
    {
        ES8311_Speaker_Mode = Es8311_Mode::TEST;

        vTaskResume(Speaker_Task_Handle);
    };

    System_Ui->_win_cit_microphone_test_callback = [](bool status)
    {
        if (status == true)
        {
            ES8311_Microphone_Mode = Es8311_Mode::TEST;

            vTaskResume(Microphone_Task_Handle);
        }
        else
        {
            vTaskSuspend(Microphone_Task_Handle);
        }
    };

    System_Ui->_win_cit_adc_to_dac_switch_callback = [](bool status)
    {
        if (status == true)
        {
            // 将ADC的数据自动输出到DAC上
            ES8311->set_adc_data_to_dac(true);
        }
        else
        {
            ES8311->set_adc_data_to_dac(false);
        }
    };

    System_Ui->_win_cit_imu_test_callback = [](bool status)
    {
        if (status == true)
        {
            ICM20948_Imu_Mode = Imu_Mode::TEST;

            vTaskResume(Imu_Task_Handle);
        }
        else
        {
            vTaskSuspend(Imu_Task_Handle);
        }
    };

    System_Ui->_win_cit_gps_test_callback = [](bool status)
    {
        if (status == true)
        {
            L76k_Gps_Mode = Gps_Mode::TEST;
            L76K->clear_rx_buffer_data();

            L76K->sleep(false);
            L76k_Gps_Positioning_Time = 0;
            L76k_Gps_Positioning_Flag = false;

            vTaskResume(Gps_Task_Handle);
        }
        else
        {
            vTaskSuspend(Gps_Task_Handle);

            L76K->sleep(true);
        }
    };

    System_Ui->_win_cit_ethernet_test_callback = [](bool status)
    {
        if (status == true)
        {
            Ip101gri_Ethernet_Mode = Ethernet_Mode::TEST;
            Eth_Info.status.update_flag = true;
            Eth_Info.connect_ip_status.update_flag = true;

            vTaskResume(Ethernet_Task_Handle);
        }
        else
        {
            vTaskSuspend(Ethernet_Task_Handle);
        }
    };

    System_Ui->_win_cit_esp32c6_at_test_callback = [](bool status)
    {
        if (status == true)
        {
            Esp32c6_At_Mode = At_Mode::TEST;

            vTaskResume(At_Task_Handle);
        }
        else
        {
            vTaskSuspend(At_Task_Handle);
        }
    };

    // System_Ui->_device_start_sleep_test_callback = [](Lvgl_Ui::System::Sleep_Mode mode)
    // {
    //     switch (mode)
    //     {
    //     case Lvgl_Ui::System::Sleep_Mode::NORMAL_SLEEP:
    //         Esp32p4_Sleep_Mode = Sleep_Mode::NORMAL_SLEEP_TEST;
    //         break;

    //     case Lvgl_Ui::System::Sleep_Mode::LIGHT_SLEEP:
    //         Esp32p4_Sleep_Mode = Sleep_Mode::LIGHT_SLEEP_TEST;
    //         break;

    //     default:
    //         break;
    //     }

    //     vTaskResume(Sleep_Task_Handle);
    // };

    System_Ui->_win_camera_status_callback = [](bool status)
    {
        if (Sys_Status.camera.init_flag == true)
        {
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4
#elif defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
            vTaskDelay(pdMS_TO_TICKS(1000));
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
            if (status == true)
            {
                esp_err_t assert = app_video_set_bufs(video_cam_fd0, CAMERA_BUFFER_COUNT, NULL);
                if (assert != ESP_OK)
                {
                    printf("app_video_set_bufs fail (error code: %#X)\n", assert);
                    return;
                }

                assert = app_video_stream_task_start(video_cam_fd0, 0);
                if (assert != ESP_OK)
                {
                    printf("app_video_stream_task_start fail (error code: %#X)\n", assert);
                }
                else
                {
                    // Get the initial time for frame rate statistics
                    start_time = esp_timer_get_time();
                }
            }
            else
            {
                esp_err_t assert = app_video_stream_task_stop(video_cam_fd0);
                if (assert != ESP_OK)
                {
                    printf("app_video_stream_task_stop fail (error code: %#X)\n", assert);
                }
            }
        }
    };

    System_Ui->_win_rf_config_lr2021_params_callback = [](Lvgl_Ui::System::Device_Lr2021 device_lr2021) -> bool
    {
        // RF switch control (same as sx1262 style)
        if (device_lr2021.params.rf_switch == 0)
        {
            printf("LR2021 rf switch: RF1\n");
            XL9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        }
        else
        {
            printf("LR2021 rf switch: RF2\n");
            XL9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        }

        const int8_t min_power = (device_lr2021.params.freq >= 1000.0) ? -19 : -9;
        const int8_t max_power = (device_lr2021.params.freq >= 1000.0) ? 12 : 22;
        int8_t output_power = device_lr2021.params.power;
        if (output_power < min_power)
        {
            output_power = min_power;
        }
        else if (output_power > max_power)
        {
            output_power = max_power;
        }

        printf("LR2021 config freq: %.1f MHz, output power: %d dBm\n",
               device_lr2021.params.freq, output_power);

        lr2021_reset();

        int16_t assert = Lr2021.begin(device_lr2021.params.freq,
                                      device_lr2021.params.bandwidth,
                                      device_lr2021.params.sf,
                                      device_lr2021.params.cr,
                                      device_lr2021.params.sync_word,
                                      output_power,
                                      device_lr2021.params.preamble_length,
                                      3.3);
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("LR2021 begin fail (error code: %d)\n", assert);
            return false;
        }

        Lr2021.setRfSwitchTable(lr2021_rfswitch_dio_pins, lr2021_rfswitch_table);

        assert = Lr2021.setCRC(device_lr2021.params.crc_type ? 2 : 0);
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("LR2021 setCRC fail (error code: %d)\n", assert);
            return false;
        }

        assert = Lr2021.startReceive();
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("LR2021 startReceive fail (error code: %d)\n", assert);
            return false;
        }

        printf("LR2021 config finish, start receive\n");
        return true;
    };

    System_Ui->_win_rf_send_data_callback = [](std::string data)
    {
        memset(Rf_Send_Package, '\0', sizeof(Rf_Send_Package));

        // 检查长度是否越界
        if (data.size() <= 255)
        {
            memcpy(Rf_Send_Package, data.data(), data.size());
        }
        else
        {
            // 处理错误：数据过长
            memcpy(Rf_Send_Package, data.data(), 254);
            Rf_Send_Package[254] = '\0';

            printf("lora send out of bounds(data > Rf_Send_Package)\n");
        }

        Rf_Send_Flag = true;
    };

    System_Ui->_win_rf_status_callback = [](bool status)
    {
        if (status == true)
        {
            Device_Rf_Task_Stop_Flag = false;
            vTaskResume(Rf_Task_Handle);
        }
        else
        {
            Device_Rf_Task_Stop_Flag = true;
        }
    };

    System_Ui->_win_music_start_end_callback = [](bool status)
    {
        if (status == true)
        {
            ES8311_Speaker_Mode = Es8311_Mode::PLAY_MUSIC;

            vTaskResume(Speaker_Task_Handle);
        }
        else
        {
            Music_Play_End_Flag = true;
        }
    };

    System_Ui->_set_music_current_time_s_callback = [](double current_time_s)
    {
        Set_Music_Current_Time_S = current_time_s;

        Set_Music_Current_Time_S_Flag = true;
    };

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    System_Ui->_win_cit_nfc_test_callback = [](bool status)
    {
        if (status == true)
        {
            St25r3916_Nfc_Mode = Nfc_Mode::TEST;

            Device_Nfc_Task_Stop_Flag = false;
            vTaskResume(Nfc_Task_Handle);
        }
        else
        {
            Device_Nfc_Task_Stop_Flag = true;
        }
    };

    System_Ui->_win_rf_config_cc1101_params_callback = [](Lvgl_Ui::System::Device_Cc1101 device_cc1101) -> bool
    {
        Cc1101_Rf_Switch_Control(static_cast<Cc1101_Rf_Switch>(device_cc1101.params.rf_switch));

        float buffer_bandwidth = 0;

        switch (device_cc1101.params.bandwidth)
        {
        case Lvgl_Ui::System::Cc1101_Bw::BW_58KHZ:
            buffer_bandwidth = 58.0f; // 58 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_68KHZ:
            buffer_bandwidth = 68.0f; // 68 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_81KHZ:
            buffer_bandwidth = 81.0f; // 81 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_102KHZ:
            buffer_bandwidth = 102.0f; // 102 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_116KHZ:
            buffer_bandwidth = 116.0f; // 116 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_135KHZ:
            buffer_bandwidth = 135.0f; // 135 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_162KHZ:
            buffer_bandwidth = 162.0f; // 162 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_203KHZ:
            buffer_bandwidth = 203.0f; // 203 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_232KHZ:
            buffer_bandwidth = 232.0f; // 232 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_270KHZ:
            buffer_bandwidth = 270.0f; // 270 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_325KHZ:
            buffer_bandwidth = 325.0f; // 325 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_406KHZ:
            buffer_bandwidth = 406.0f; // 406 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_464KHZ:
            buffer_bandwidth = 464.0f; // 464 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_541KHZ:
            buffer_bandwidth = 541.0f; // 541 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_650KHZ:
            buffer_bandwidth = 650.0f; // 650 kHz
            break;
        case Lvgl_Ui::System::Cc1101_Bw::BW_812KHZ:
            buffer_bandwidth = 812.0f; // 812 kHz
            break;
        default:
            break;
        }

        int16_t assert = Cc1101.begin(device_cc1101.params.freq, device_cc1101.params.bit_rate, device_cc1101.params.freq_deviation_khz,
                                      buffer_bandwidth, device_cc1101.params.power, device_cc1101.params.preamble_length);
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("cc1101 begin fail (error code: %d)\n", assert);
            return false;
        }

        assert = Cc1101.setSyncWord(device_cc1101.params.sync_word >> 8, device_cc1101.params.sync_word);
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("cc1101 setSyncWord fail (error code: %d)\n", assert);
            return false;
        }

        assert = Cc1101.startReceive();
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("cc1101 startReceive fail (error code: %d)\n", assert);
        }

        Cc1101_Interrupt_Flag = false;

        printf("config_cc1101_params finish start cc1101 transmit\n");
        return true;
    };

    System_Ui->_win_rf_config_nrf24l01_params_callback = [](Lvgl_Ui::System::Device_Nrf24l01 device_nrf24l01) -> bool
    {
        int16_t assert = Nrf24l01.begin(device_nrf24l01.params.freq, device_nrf24l01.params.bit_rate, device_nrf24l01.params.power,
                                        device_nrf24l01.params.address_width);
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("nrf24l01 begin fail (error code: %d)\n", assert);
            return false;
        }

        ESP32P4->create_gpio_interrupt(T_MIXRF_NRF24L01_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                       [](void *arg) -> IRAM_ATTR void
                                       {
                                           Nrf24l01_Interrupt_Flag = true;
                                       });

        uint8_t address[] = {
            static_cast<uint8_t>(device_nrf24l01.params.address >> 32),
            static_cast<uint8_t>(device_nrf24l01.params.address >> 24),
            static_cast<uint8_t>(device_nrf24l01.params.address >> 16),
            static_cast<uint8_t>(device_nrf24l01.params.address >> 8),
            static_cast<uint8_t>(device_nrf24l01.params.address),
        };
        assert = Nrf24l01.setTransmitPipe(address);
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("nrf24l01 setTransmitPipe fail (error code: %d)\n", assert);
            return false;
        }

        assert = Nrf24l01.startReceive();
        if (assert != RADIOLIB_ERR_NONE)
        {
            printf("nrf24l01 startReceive fail (error code: %d)\n", assert);
        }

        Nrf24l01_Interrupt_Flag = false;

        printf("config_nrf24l01_params finish start nrf24l01 transmit\n");
        return true;
    };

    System_Ui->_win_cit_otg_switch_callback = [](bool status)
    {
        if (status == true)
        {
            Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);
        }
        else
        {
            Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_DISABLE);
        }
    };

#endif
}

void Lvgl_Init(void)
{
    printf("initialize lvgl\n");

    lv_init();

    // create a lvgl display
    lv_display_t *display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, Screen_Mipi_Dpi_Panel);
    // set color depth
    lv_display_set_color_format(display, LVGL_COLOR_FORMAT);
    // create draw buffer
    printf("allocate separate lvgl draw buffers\n");
    size_t draw_buffer_sz = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    assert(buf1);
    // void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    // assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, [](lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
                            {
                                lv_display_rotation_t rotation = lv_display_get_rotation(disp);
                                esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);

                                int32_t offsetx1 = area->x1;
                                int32_t offsetx2 = area->x2;
                                int32_t offsety1 = area->y1;
                                int32_t offsety2 = area->y2;

                                if (rotation != LV_DISPLAY_ROTATION_0)
                                {
#if CONFIG_ENABLE_PPA_SCREEN_ROTATION == true
                                    uint32_t input_img_width = area->x2 - area->x1 + 1;
                                    uint32_t input_img_height = area->y2 - area->y1 + 1;

                                    // 根据旋转角度确定输出尺寸
                                    uint32_t output_img_width = input_img_width;
                                    uint32_t output_img_height = input_img_height;

                                    // 如果是90或270度旋转，宽度和高度需要交换
                                    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
                                    {
                                        output_img_width = input_img_height;
                                        output_img_height = input_img_width;
                                    }

                                    // 计算实际需要的缓冲区大小
                                    size_t output_buffer_size = output_img_width * output_img_height * (SCREEN_BITS_PER_PIXEL / 8);
                                    uint8_t *output_buffer = (uint8_t *)heap_caps_malloc(output_buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
                                    if (output_buffer == NULL)
                                    {
                                        printf("failed to allocate rotated buffer\n");
                                        return;
                                    }

                                    ppa_srm_oper_config_t srm_config =
                                        {
                                            .in =
                                                {
                                                    .buffer = px_map,
                                                    .pic_w = input_img_width,
                                                    .pic_h = input_img_height,
                                                    .block_w = input_img_width,
                                                    .block_h = input_img_height,
                                                    .block_offset_x = 0,
                                                    .block_offset_y = 0,
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
                                                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
                                                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB888,
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
                                                },

                                            .out =
                                                {
                                                    .buffer = output_buffer,
                                                    .buffer_size = ALIGN_UP(output_buffer_size, data_cache_line_size_2),
                                                    .pic_w = output_img_width,
                                                    .pic_h = output_img_height,
                                                    .block_offset_x = 0,
                                                    .block_offset_y = 0,
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
                                                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
                                                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB888,
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
                                                },

                                            .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
                                            .scale_x = 1,
                                            .scale_y = 1,
                                            .mirror_x = false,
                                            .mirror_y = false,
                                            .rgb_swap = false,
                                            .byte_swap = false,
                                            .mode = PPA_TRANS_MODE_BLOCKING,
                                        };

                                    switch (rotation)
                                    {
                                    case LV_DISPLAY_ROTATION_90:
                                        srm_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_90;
                                        break;
                                    case LV_DISPLAY_ROTATION_180:
                                        srm_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_180;
                                        break;
                                    case LV_DISPLAY_ROTATION_270:
                                        srm_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_270;
                                        break;
                                    default:
                                        break;
                                    }

                                    esp_err_t ret = ppa_do_scale_rotate_mirror(ppa_srm_handle_2, &srm_config);
                                    if (ret != ESP_OK)
                                    {
                                        printf("ppa_do_scale_rotate_mirror fail (error code: 0x%X)\n", ret);
                                        heap_caps_free(output_buffer);
                                        return;
                                    }

                                    // 根据旋转角度重新计算坐标
                                    int32_t rotated_offsetx1 = offsetx1;
                                    int32_t rotated_offsety1 = offsety1;
                                    int32_t rotated_offsetx2 = offsetx2;
                                    int32_t rotated_offsety2 = offsety2;

                                    switch (rotation)
                                    {
                                    case LV_DISPLAY_ROTATION_90:
                                        // 90度旋转：x = original_y, y = SCREEN_HEIGHT - original_x - 1
                                        rotated_offsetx1 = offsety1;
                                        rotated_offsety1 = SCREEN_HEIGHT - offsetx2 - 1;
                                        rotated_offsetx2 = offsety2;
                                        rotated_offsety2 = SCREEN_HEIGHT - offsetx1 - 1;
                                        break;
                                    case LV_DISPLAY_ROTATION_180:
                                        // 180度旋转：x = SCREEN_WIDTH - original_x - 1, y = SCREEN_HEIGHT - original_y - 1
                                        rotated_offsetx1 = SCREEN_WIDTH - offsetx2 - 1;
                                        rotated_offsety1 = SCREEN_HEIGHT - offsety2 - 1;
                                        rotated_offsetx2 = SCREEN_WIDTH - offsetx1 - 1;
                                        rotated_offsety2 = SCREEN_HEIGHT - offsety1 - 1;
                                        break;
                                    case LV_DISPLAY_ROTATION_270:
                                        // 270度旋转：x = SCREEN_WIDTH - original_y - 1, y = original_x
                                        rotated_offsetx1 = SCREEN_WIDTH - offsety2 - 1;
                                        rotated_offsety1 = offsetx1;
                                        rotated_offsetx2 = SCREEN_WIDTH - offsety1 - 1;
                                        rotated_offsety2 = offsetx2;
                                        break;
                                    default:
                                        break;
                                    }

                                    // 确保旋转后的坐标在屏幕范围内
                                    rotated_offsetx1 = (rotated_offsetx1 < 0) ? 0 : rotated_offsetx1;
                                    rotated_offsety1 = (rotated_offsety1 < 0) ? 0 : rotated_offsety1;
                                    rotated_offsetx2 = (rotated_offsetx2 >= SCREEN_WIDTH) ? SCREEN_WIDTH - 1 : rotated_offsetx2;
                                    rotated_offsety2 = (rotated_offsety2 >= SCREEN_HEIGHT) ? SCREEN_HEIGHT - 1 : rotated_offsety2;

                                    // 确保 x1 <= x2 且 y1 <= y2
                                    if (rotated_offsetx1 > rotated_offsetx2)
                                    {
                                        int32_t temp = rotated_offsetx1;
                                        rotated_offsetx1 = rotated_offsetx2;
                                        rotated_offsetx2 = temp;
                                    }
                                    if (rotated_offsety1 > rotated_offsety2)
                                    {
                                        int32_t temp = rotated_offsety1;
                                        rotated_offsety1 = rotated_offsety2;
                                        rotated_offsety2 = temp;
                                    }

                                    esp_lcd_panel_draw_bitmap(panel_handle, rotated_offsetx1, rotated_offsety1,
                                                              rotated_offsetx2 + 1, rotated_offsety2 + 1, output_buffer);

                                    heap_caps_free(output_buffer);

#else
                                    lv_area_t rotated_area;
                                    lv_color_format_t cf = lv_display_get_color_format(disp);
                                    /*Calculate the position of the rotated area*/
                                    rotated_area = *area;
                                    lv_display_rotate_area(disp, &rotated_area);
                                    /*Calculate the source stride (bytes in a line) from the width of the area*/
                                    uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
                                    /*Calculate the stride of the destination (rotated) area too*/
                                    uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
                                    /*Have a buffer to store the rotated area and perform the rotation*/

                                    int32_t src_w = lv_area_get_width(area);
                                    int32_t src_h = lv_area_get_height(area);
                                    auto rotated_buf = std::make_unique<uint8_t[]>(SCREEN_WIDTH * SCREEN_HEIGHT * (SCREEN_BITS_PER_PIXEL / 8));
                                    lv_draw_sw_rotate(px_map, rotated_buf.get(), src_w, src_h, src_stride, dest_stride, rotation, cf);
                                    /*Use the rotated area and rotated buffer from now on*/
                                    area = &rotated_area;
                                    px_map = rotated_buf.get();

                                    offsetx1 = area->x1;
                                    offsetx2 = area->x2;
                                    offsety1 = area->y1;
                                    offsety2 = area->y2;

                                    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
#endif
                                }
                                else
                                {
                                    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
                                }

#if CONFIG_ENABLE_USB_DISPLAY == true
                                lv_display_flush_ready(disp);
#endif
                            });

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    lv_indev_t *indev_2 = lv_indev_create();
    lv_indev_set_type(indev_2, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_2, my_keyboard_read);
#endif

#if CONFIG_ENABLE_USB_DISPLAY == true
#else
    printf("register dpi panel event callback for lvgl flush ready notification\n");
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_color_trans_done = [](esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx) -> bool
        {
            lv_display_t *disp = (lv_display_t *)user_ctx;
            lv_display_flush_ready(disp);
            return false; },
        .on_refresh_done = [](esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx) -> bool
        {
            // static int io_level = 0;
            // // please note, the real refresh rate should be 2*frequency of this GPIO toggling
            // gpio_set_level(EXAMPLE_PIN_NUM_REFRESH_MONITOR, io_level);
            // io_level = !io_level;
            return false; },
    };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(Screen_Mipi_Dpi_Panel, &cbs, display));
#endif

    printf("use esp_timer as lvgl tick timer\n");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = [](void *arg)
        {
            lv_tick_inc(LVGL_TICK_PERIOD_MS);
        },
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    lv_display_set_rotation(display, LV_DISPLAY_ROTATION);

    System_Ui_Callback_Init();
}

void Lvgl_Startup(void)
{
    // 创建一个全屏黑色背景
    lv_obj_t *bg = lv_obj_create(NULL);
    lv_obj_set_size(bg, lv_display_get_horizontal_resolution(lv_display_get_default()), lv_display_get_vertical_resolution(lv_display_get_default()));
    lv_obj_set_style_bg_color(bg, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(bg, 0, LV_PART_MAIN);

    // 创建进度条
    Lvgl_Startup_Progress_Bar = lv_bar_create(bg);
    lv_obj_set_size(Lvgl_Startup_Progress_Bar, lv_pct(70), 10); // 宽度为屏幕70%，高度10像素
    lv_bar_set_range(Lvgl_Startup_Progress_Bar, 0, 100);
    lv_bar_set_value(Lvgl_Startup_Progress_Bar, 10, LV_ANIM_OFF); // 进度条初始进度
    lv_obj_set_style_bg_color(Lvgl_Startup_Progress_Bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(Lvgl_Startup_Progress_Bar, lv_color_white(), LV_PART_INDICATOR);

    lv_obj_align(Lvgl_Startup_Progress_Bar, LV_ALIGN_CENTER, 0, 15);

    // 创建白色"LILYGO"标签
    lv_obj_t *logo_label = lv_label_create(bg);
    lv_label_set_text(logo_label, "LILYGO");
    lv_obj_set_style_text_color(logo_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(logo_label, &lv_font_montserrat_48, LV_PART_MAIN); // 可根据需要调整字体
    // logo放在进度条上方，整体居中
    lv_obj_align_to(logo_label, Lvgl_Startup_Progress_Bar, LV_ALIGN_OUT_TOP_MID, 0, -30);

    lv_obj_update_layout(bg);

    lv_screen_load(bg);
}

void Set_Lvgl_Startup_Progress_Bar(uint8_t percentage)
{
    if (Lvgl_Startup_Progress_Bar != nullptr)
    {
        lv_bar_set_value(Lvgl_Startup_Progress_Bar, percentage, LV_ANIM_OFF);
    }
}

void ES8311_Init(void)
{
    ES8311->begin(MCLK_MULTIPLE, SAMPLE_RATE, i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT);

    if (ES8311->begin(50000) == true)
    {
        printf("es8311 initialization success\n");
        Sys_Status.es8311.init_flag = true;
    }
    else
    {
        printf("es8311 initialization fail\n");
        Sys_Status.es8311.init_flag = false;
    }

    ES8311->set_master_clock_source(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK);
    ES8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK, true);
    ES8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_BCLK, true);

    ES8311->set_clock_coeff(MCLK_MULTIPLE, SAMPLE_RATE);

    ES8311->set_serial_port_mode(Cpp_Bus_Driver::Es8311::Serial_Port_Mode::SLAVE);

    ES8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::ADC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
    ES8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::DAC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
    Cpp_Bus_Driver::Es8311::Power_Status ps =
        {
            .contorl =
                {
                    .analog_circuits = true,               // 开启模拟电路
                    .analog_bias_circuits = true,          // 开启模拟偏置电路
                    .analog_adc_bias_circuits = true,      // 开启模拟ADC偏置电路
                    .analog_adc_reference_circuits = true, // 开启模拟ADC参考电路
                    .analog_dac_reference_circuit = true,  // 开启模拟DAC参考电路
                    .internal_reference_circuits = false,  // 关闭内部参考电路
                },
            .vmid = Cpp_Bus_Driver::Es8311::Vmid::START_UP_VMID_NORMAL_SPEED_CHARGE,
        };
    ES8311->set_power_status(ps);
    ES8311->set_pga_power(true);
    ES8311->set_adc_power(true);
    ES8311->set_dac_power(true);
    ES8311->set_output_to_hp_drive(true);
    ES8311->set_adc_offset_freeze(Cpp_Bus_Driver::Es8311::Adc_Offset_Freeze::DYNAMIC_HPF);
    ES8311->set_adc_hpf_stage2_coeff(10);
    ES8311->set_dac_equalizer(false);

    ES8311->set_mic(Cpp_Bus_Driver::Es8311::Mic_Type::ANALOG_MIC, Cpp_Bus_Driver::Es8311::Mic_Input::MIC1P_1N);
    ES8311->set_adc_auto_volume_control(false);
    ES8311->set_adc_gain(Cpp_Bus_Driver::Es8311::Adc_Gain::GAIN_18DB);
    ES8311->set_adc_pga_gain(Cpp_Bus_Driver::Es8311::Adc_Pga_Gain::GAIN_30DB);

    ES8311->set_adc_volume(191);
    ES8311->set_dac_volume(200);

    // 将ADC的数据自动输出到DAC上
    // ES8311->set_adc_data_to_dac(true);
}

bool ICM20948_Init(void)
{
    Wire1.begin(ICM20948_SDA, ICM20948_SCL);
    if (ICM20948->init() == false)
    {
        printf("icm20948 ag init fail\n");
        return false;
    }

    if (ICM20948->initMagnetometer() == false)
    {
        printf("icm20948 m init fail\n");
        return false;
    }

    printf("Position your ICM20948 flat and don't move it - calibrating...\n");
    ICM20948->autoOffsets();
    printf("Done!\n");

    ICM20948->setAccRange(ICM20948_ACC_RANGE_2G);
    ICM20948->setAccDLPF(ICM20948_DLPF_6);
    ICM20948->setMagOpMode(AK09916_CONT_MODE_20HZ);

    return true;
}

/** Event handler for Ethernet events */
void eth_event_handler(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        printf("ethernet link up\n");
        printf("ethernet hw addr %02x:%02x:%02x:%02x:%02x:%02x\n",
               mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

        Eth_Info.status.data = "status: link up\nhw addr: " +
                               std::to_string(mac_addr[0]) + ":" +
                               std::to_string(mac_addr[1]) + ":" +
                               std::to_string(mac_addr[2]) + ":" +
                               std::to_string(mac_addr[3]) + ":" +
                               std::to_string(mac_addr[4]) + ":" +
                               std::to_string(mac_addr[5]) + "\n";
        Eth_Info.status.update_flag = true;
        Eth_Info.link_up_flag = true;
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        printf("ethernet link down\n");

        Eth_Info.status.data = "status: link down\n";
        Eth_Info.status.update_flag = true;
        Eth_Info.link_up_flag = false;
        break;
    case ETHERNET_EVENT_START:
        printf("ethernet started\n");

        Eth_Info.status.data = "status: started\n";
        Eth_Info.status.update_flag = true;
        Eth_Info.link_up_flag = false;
        break;
    case ETHERNET_EVENT_STOP:
        printf("ethernet stopped\n");

        Eth_Info.status.data = "status: stopped\n";
        Eth_Info.status.update_flag = true;
        Eth_Info.link_up_flag = false;
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    printf("ethernet get ip address\n");
    printf("~~~~~~~~~~~\n");
    printf("eth ip: %d.%d.%d.%d\n", IP2STR(&ip_info->ip));
    printf("eth mask: %d.%d.%d.%d\n", IP2STR(&ip_info->netmask));
    printf("eth gw: %d.%d.%d.%d\n", IP2STR(&ip_info->gw));
    printf("~~~~~~~~~~~\n");

    // 定义一个足够大的字符数组来存储格式化后的字符串
    char ip_status_data[256];

    snprintf(ip_status_data, sizeof(ip_status_data),
             "ethernet get ip address\n"
             "eth ip: %d.%d.%d.%d\n"
             "eth mask: %d.%d.%d.%d\n"
             "eth gw: %d.%d.%d.%d\n",
             IP2STR(&ip_info->ip),
             IP2STR(&ip_info->netmask),
             IP2STR(&ip_info->gw));

    Eth_Info.connect_ip_status.data = ip_status_data;

    Eth_Info.connect_ip_status.update_flag = true;
}

void Ethernet_Init(void)
{
    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *eth_netifs[eth_port_cnt];
    esp_eth_netif_glue_handle_t eth_netif_glues[eth_port_cnt];

    // Create instance(s) of esp-netif for Ethernet(s)
    if (eth_port_cnt == 1)
    {
        // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
        // default esp-netif configuration parameters.
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netifs[0] = esp_netif_new(&cfg);
        eth_netif_glues[0] = esp_eth_new_netif_glue(eth_handles[0]);
        // Attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[0], eth_netif_glues[0]));
    }
    else
    {
        // Use ESP_NETIF_INHERENT_DEFAULT_ETH when multiple Ethernet interfaces are used and so you need to modify
        // esp-netif configuration parameters for each interface (name, priority, etc.).
        esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        esp_netif_config_t cfg_spi = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
        char if_key_str[10];
        char if_desc_str[10];
        char num_str[3];
        for (int i = 0; i < eth_port_cnt; i++)
        {
            itoa(i, num_str, 10);
            strcat(strcpy(if_key_str, "ETH_"), num_str);
            strcat(strcpy(if_desc_str, "eth"), num_str);
            esp_netif_config.if_key = if_key_str;
            esp_netif_config.if_desc = if_desc_str;
            esp_netif_config.route_prio -= i * 5;
            eth_netifs[i] = esp_netif_new(&cfg_spi);
            eth_netif_glues[i] = esp_eth_new_netif_glue(eth_handles[0]);
            // Attach Ethernet driver to TCP/IP stack
            ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[i], eth_netif_glues[i]));
        }
    }

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    for (int i = 0; i < eth_port_cnt; i++)
    {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }
}

void Esp32c6_At_Init(void)
{
    // ESP32C6_AT->begin();

    // 开启falsh保存
    if (ESP32C6_AT->set_flash_save(true) == true)
    {
        printf("set_flash_save success\n");
    }
    else
    {
        printf("set_flash_save fail\n");
    }

    if (ESP32C6_AT->set_wifi_mode(Cpp_Bus_Driver::Esp_At::Wifi_Mode::STATION) == true)
    {
        printf("set_wifi_mode success\n");
    }
    else
    {
        printf("set_wifi_mode fail\n");
    }

    std::vector<uint8_t> buffer_wifi_scan;
    if (ESP32C6_AT->wifi_scan(buffer_wifi_scan) == true)
    {
        printf("wifi_scan: \n[%s]\n", buffer_wifi_scan.data());
    }
    else
    {
        printf("wifi_scan fail\n");
    }

    std::string ssid = "xinyuandianzi";
    std::string password = "AA15994823428";
    if (ESP32C6_AT->set_wifi_connect(ssid, password) == true)
    {
        printf("set_wifi_connect success\nconnected to wifi ssid: [%s],password: [%s]\n", ssid.c_str(), password.c_str());
        Sys_Status.esp32c6.wifi_connect_status = true;
    }
    else
    {
        printf("set_wifi_connect fail\n");
        Sys_Status.esp32c6.wifi_connect_status = false;
    }

    System_Ui->set_wifi_connect_status(Sys_Status.esp32c6.wifi_connect_status);

    Cpp_Bus_Driver::Esp_At::Real_Time rt;
    if (ESP32C6_AT->get_real_time(rt) == true)
    {
        printf("get_real_time success\n");
        printf("real_time week: [%s] day: [%d] month: [%d] year: [%d] time: [%d:%d:%d] time zone: [%s] china time: [%d:%d:%d]\n",
               rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute, rt.second, rt.time_zone.c_str(),
               (rt.hour + 8 + 24) % 24, rt.minute, rt.second);

        Save_Real_Time(rt);
    }
    else
    {
        printf("get_real_time fail\n");

        // 保存rtc时间
        Cpp_Bus_Driver::Pcf8563x::Time t;
        if (PCF8563->get_time(t) == true)
        {
            printf("pcf8563 year:[%d] month:[%d] day:[%d] time:[%d:%d:%d] week:[%d]\n", t.year, t.month, t.day,
                   t.hour, t.minute, t.second, static_cast<uint8_t>(t.week));

            std::string week_str;
            switch (t.week)
            {
            case Cpp_Bus_Driver::Pcf8563x::Week::SUNDAY:
                week_str = "Sun";
                break;
            case Cpp_Bus_Driver::Pcf8563x::Week::MONDAY:
                week_str = "Mon";
                break;
            case Cpp_Bus_Driver::Pcf8563x::Week::TUESDAY:
                week_str = "Tue";
                break;
            case Cpp_Bus_Driver::Pcf8563x::Week::WEDNESDAY:
                week_str = "Wed";
                break;
            case Cpp_Bus_Driver::Pcf8563x::Week::THURSDAY:
                week_str = "Thu";
                break;
            case Cpp_Bus_Driver::Pcf8563x::Week::FRIDAY:
                week_str = "Fri";
                break;
            case Cpp_Bus_Driver::Pcf8563x::Week::SATURDAY:
                week_str = "Sat";
                break;

            default:
                break;
            }

            System_Ui->_time.week = week_str;
            System_Ui->_time.year = static_cast<uint16_t>(t.year + 2000);
            System_Ui->_time.month = t.month;
            System_Ui->_time.day = t.day;
            System_Ui->_time.hour = t.hour;
            System_Ui->_time.minute = t.minute;
            System_Ui->_time.second = t.second;
        }
    }
}
#if CONFIG_ENABLE_USB_DISPLAY == true
bool Usb_Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    usb_display_vendor_config_t vendor_config_usb = DEFAULT_USB_DISPLAY_VENDOR_CONFIG(SCREEN_WIDTH, SCREEN_HEIGHT,
                                                                                      SCREEN_BITS_PER_PIXEL, *mipi_dpi_panel);

    if (esp_lcd_new_panel_usb_display(&vendor_config_usb, mipi_dpi_panel) != ESP_OK)
    {
        printf("esp_lcd_new_panel_usb_display fail\n");
        return false;
    }

    return true;
}
#else
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(itf, rx_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK)
    {
        app_message_t tx_msg = {
            .buf_len = rx_size + PREPEND_LENGTH,
            .itf = static_cast<uint8_t>(itf),
        };

        memcpy(tx_msg.buf, PREPEND_STRING, PREPEND_LENGTH);
        memcpy(tx_msg.buf + PREPEND_LENGTH, rx_buf, rx_size);
        xQueueSend(app_queue, &tx_msg, 0);
    }
    else
    {
        printf("tinyusb_cdc_rx_callback read error\n");
    }
}

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;

    printf("line state changed on channel %d: dtr:%d, rts:%d\n", itf, dtr, rts);
}

void Hardware_Usb_Cdc_Init(void)
{
    // Create FreeRTOS primitives
    app_queue = xQueueCreate(5, sizeof(app_message_t));
    assert(app_queue);

    printf("USB initialization\n");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = NULL,
        .hs_configuration_descriptor = NULL,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = NULL,
#endif // TUD_OPT_HIGH_SPEED
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL};

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    /* the second way to register a callback */
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
        TINYUSB_CDC_ACM_0,
        CDC_EVENT_LINE_STATE_CHANGED,
        &tinyusb_cdc_line_state_changed_callback));

#if (CONFIG_TINYUSB_CDC_COUNT > 1)
    acm_cfg.cdc_port = TINYUSB_CDC_ACM_1;
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
        TINYUSB_CDC_ACM_1,
        CDC_EVENT_LINE_STATE_CHANGED,
        &tinyusb_cdc_line_state_changed_callback));
#endif

    printf("USB initialization DONE\n");
}

void hardware_usb_cdc_task(void *arg)
{
    printf("hardware_usb_cdc_task start\n");

    while (1)
    {
        app_message_t msg;
        if (xQueueReceive(app_queue, &msg, portMAX_DELAY))
        {
            if (msg.buf_len)
            {
                /* Print received data*/
                printf("data from channel %d: ", msg.itf);

                for (size_t i = 0; i < msg.buf_len; i++)
                {
                    printf("%c", msg.buf[i]);
                }
                printf("\n");

                /* write back */
                tinyusb_cdcacm_write_queue(msg.itf, msg.buf, msg.buf_len);
                esp_err_t err = tinyusb_cdcacm_write_flush(msg.itf, 0);
                if (err != ESP_OK)
                {
                    printf("CDC ACM write flush error: %s\n", esp_err_to_name(err));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#endif

void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves,
                                  size_t camera_buf_len)
{
    fps_count++;
    if (fps_count == 50)
    {
        int64_t end_time = esp_timer_get_time();
        printf("fps: %f\n", 1000000.0 / ((end_time - start_time) / 50.0));
        start_time = end_time;
        fps_count = 0;

        printf("camera_buf_hes: %lu, camera_buf_ves: %lu, camera_buf_len: %d KB\n", camera_buf_hes, camera_buf_ves, camera_buf_len / 1024);
    }

    uint32_t input_img_block_width = (camera_buf_hes - SCREEN_WIDTH) / 2;
    uint32_t input_img_block_height = 0;
    uint32_t input_img_width = SCREEN_WIDTH;
    uint32_t input_img_height = camera_buf_ves;

    uint32_t output_img_width = input_img_width;
    uint32_t output_img_height = input_img_height;

    size_t output_buffer_size = output_img_width * output_img_height * (SCREEN_BITS_PER_PIXEL / 8);
    uint8_t *output_buffer = (uint8_t *)heap_caps_aligned_calloc(data_cache_line_size, 1, output_buffer_size, MALLOC_CAP_SPIRAM);
    if (output_buffer == NULL)
    {
        printf("heap_caps_malloc fail\n");
        return;
    }

    ppa_srm_oper_config_t srm_config =
        {
            .in =
                {
                    .buffer = camera_buf,
                    .pic_w = camera_buf_hes,
                    .pic_h = camera_buf_ves,
                    .block_w = input_img_width,
                    .block_h = input_img_height,
                    .block_offset_x = input_img_block_width,
                    .block_offset_y = input_img_block_height,
#if (defined CONFIG_CAMERA_TYPE_SC2336) || (defined CONFIG_CAMERA_TYPE_OV2710)
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB888,
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
                },

            .out =
                {
                    .buffer = output_buffer,
                    .buffer_size = ALIGN_UP(output_buffer_size, data_cache_line_size),
                    .pic_w = output_img_width,
                    .pic_h = output_img_height,
                    .block_offset_x = 0,
                    .block_offset_y = 0,
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB888,
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
                },

            .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
            .scale_x = 1,
            .scale_y = 1,
            .mirror_x = false,
#if defined SCREEN_ROTATION_DIRECTION_0
#if defined CONFIG_SCREEN_TYPE_HI8561
            .mirror_y = true,
#elif defined CONFIG_SCREEN_TYPE_RM69A10
            .mirror_y = false,
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
#elif defined SCREEN_ROTATION_DIRECTION_90
            .mirror_y = false,
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
            .rgb_swap = false,
            .byte_swap = false,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };

    esp_err_t assert = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
    if (assert != ESP_OK)
    {
        printf("ppa_do_scale_rotate_mirror fail (error code: %#X)\n", assert);
        heap_caps_free(output_buffer);
        return;
    }

    if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CAMERA)
    {
        assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, (SCREEN_HEIGHT - output_img_height) / 2,
                                           output_img_width, output_img_height + ((SCREEN_HEIGHT - output_img_height) / 2),
                                           output_buffer);
        if (assert != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
            heap_caps_free(output_buffer);
            return;
        }
        // _lock_acquire(&lvgl_api_lock);
        // lv_canvas_set_buffer(System_Ui->_registry.win.camera.canvas, lcd_buffer[camera_buf_index],
        //                      srm_config.in.block_w, srm_config.in.block_h + (SCREEN_HEIGHT - srm_config.in.block_h) / 2,
        //                      LCD_COLOR_PIXEL_FORMAT_RGB565);
        // _lock_release(&lvgl_api_lock);
    }

    heap_caps_free(output_buffer);
}

bool App_Video_Init(void)
{
    ppa_client_config_t ppa_srm_config =
        {
            .oper_type = PPA_OPERATION_SRM,
        };
    esp_err_t assert = ppa_register_client(&ppa_srm_config, &ppa_srm_handle);
    if (assert != ESP_OK)
    {
        printf("ppa_register_client fail (error code: %#X)\n", assert);
        return false;
    }
    assert = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &data_cache_line_size);
    if (assert != ESP_OK)
    {
        printf("esp_cache_get_alignment fail (error code: %#X)\n", assert);
        return false;
    }

    esp_video_init_csi_config_t csi_config =
        {
            .sccb_config = {
                .init_sccb = false,
                .i2c_handle = SGM38121_IIC_Bus->get_bus_handle(),
                .freq = static_cast<uint32_t>(100000),
            },
            .reset_pin = GPIO_NUM_NC,
            .pwdn_pin = GPIO_NUM_NC,
            .dont_init_ldo = true,
        };

    esp_video_init_config_t cam_config =
        {
            .csi = &csi_config,
        };

    assert = esp_video_init(&cam_config);
    if (assert != ESP_OK)
    {
        printf("esp_video_init fail (error code: %#X)\n", assert);
        return false;
    }

#if (defined CONFIG_CAMERA_TYPE_SC2336) || (defined CONFIG_CAMERA_TYPE_OV2710)
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB565);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB888);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB565);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

    // #if CONFIG_EXAMPLE_USE_MEMORY_MAPPING
    //     ESP_LOGI(TAG, "Using map buffer");
    //     // When setting the camera video buffer, it can be written as NULL to automatically allocate the buffer using mapping
    //     assert = app_video_set_bufs(app_video_set_bufs(video_cam_fd0, EXAMPLE_CAM_BUF_NUM, NULL));
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #elif CONFIG_CAMERA_CAMERA_MIPI_RAW8_1280X720_30FPS
    //     printf("using user defined buffer\n");
    //     assert = app_video_set_bufs(video_cam_fd0, CONFIG_EXAMPLE_CAM_BUF_COUNT, (const void **)lcd_buffer);
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #else
    //     void *camera_buf[EXAMPLE_CAM_BUF_NUM];
    //     for (int i = 0; i < EXAMPLE_CAM_BUF_NUM; i++)
    //     {
    //         camera_buf[i] = heap_caps_aligned_calloc(data_cache_line_size, 1, app_video_get_buf_size(), MALLOC_CAP_SPIRAM);
    //     }
    //     assert = app_video_set_bufs(video_cam_fd0, EXAMPLE_CAM_BUF_NUM, (const void **)camera_buf);
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #endif

    assert = app_video_set_bufs(video_cam_fd0, CAMERA_BUFFER_COUNT, NULL);
    if (assert != ESP_OK)
    {
        printf("app_video_set_bufs fail (error code: %#X)\n", assert);
        return false;
    }

    assert = app_video_register_frame_operation_cb(camera_video_frame_operation);
    if (assert != ESP_OK)
    {

        printf("app_video_register_frame_operation_cb fail (error code: %#X)\n", assert);
        return false;
    }

    return true;
}

#if (CONFIG_ENABLE_PPA_SCREEN_ROTATION == true) && (!defined SCREEN_ROTATION_DIRECTION_0)
bool Ppa_Screen_Rotation_Init(void)
{
    ppa_client_config_t ppa_srm_config =
        {
            .oper_type = PPA_OPERATION_SRM,
        };
    esp_err_t assert = ppa_register_client(&ppa_srm_config, &ppa_srm_handle_2);
    if (assert != ESP_OK)
    {
        printf("ppa_register_client fail (error code: %#X)\n", assert);
        return false;
    }
    assert = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &data_cache_line_size_2);
    if (assert != ESP_OK)
    {
        printf("esp_cache_get_alignment fail (error code: %#X)\n", assert);
        return false;
    }

    return true;
}
#endif

bool Play_Wav_File_2(const char *file_path)
{
    std::ifstream file(file_path, std::ios::binary);

    if (file.is_open() == false)
    {
        printf("failed to open wav file: %s\n", file_path);
        return false;
    }

    Wav_Header wav_header;
    if (!file.read(reinterpret_cast<char *>(&wav_header), sizeof(wav_header)))
    {
        printf("failed to read wav header\n");
        file.close();
        return false;
    }

    // 分别检查 WAV 文件头的每个部分
    if (strncmp(wav_header.riff_header, "RIFF", 4) != 0)
    {
        printf("invalid wav file format: riff_header is not 'RIFF'\n");
        // file.close();
        // return false;
    }
    else if (strncmp(wav_header.wave_header, "WAVE", 4) != 0)
    {
        printf("invalid wav file format: wave_header is not 'WAVE'\n");
        // file.close();
        // return false;
    }
    else if (strncmp(wav_header.fmt_header, "fmt ", 4) != 0)
    {
        printf("invalid wav file format: fmt_header is not 'fmt '\n");
        // file.close();
        // return false;
    }
    else if (strncmp(wav_header.data_header, "data", 4) != 0)
    {
        printf("invalid wav file format: data_header is not 'data'\n");
        // file.close();
        // return false;
    }

    printf("sample rate: %ld\n", wav_header.sample_rate);
    printf("channels: %d\n", wav_header.num_channel);
    printf("bits per sample: %d\n", wav_header.bits_per_sample);

    // 检查采样率、通道数和位深度是否与 I2S 配置匹配 (如果使用 I2S)
    if (wav_header.sample_rate != SAMPLE_RATE ||
        wav_header.num_channel != NUM_CHANNEL ||
        wav_header.bits_per_sample != BITS_PER_SAMPLE)
    {
        printf("wav file parameters do not match i2s configuration audio may not play correctly\n");
        file.close();
        return false;
    }

    // 计算播放时间
    double duration = 0.0;
    if (wav_header.sample_rate > 0 && wav_header.num_channel > 0 && wav_header.bits_per_sample > 0)
    {
        duration = static_cast<double>(wav_header.data_size) / (wav_header.sample_rate * wav_header.num_channel * (wav_header.bits_per_sample / 8.0));
    }

    printf("duration: %.2f s\n", duration);

    // 读取并播放音频数据
    std::unique_ptr<char[]> data_buffer = std::make_unique<char[]>(1024);

    if (data_buffer == nullptr)
    {
        printf("failed to allocate memory for audio buffer\n");
        file.close();
        return false;
    }

    while (file.good())
    {
        file.read(data_buffer.get(), 1024);
        std::streamsize bytes_read = file.gcount(); // 获取实际读取的字节数

        if (bytes_read > 0)
        {
            ES8311->write_data(data_buffer.get(), bytes_read); // 这一行需要根据你的 I2S 驱动实现来修改
        }
        // else
        // {
        //     break; // 结束循环，如果读取的字节数为 0
        // }
    }

    file.close();
    return true;
}

void System_Startup_Message_Init(void)
{
    if (Sys_Status.sgm38121.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "sgm38121 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.camera.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "camera init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.esp32c6.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "esp32c6 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.esp32c6.wifi_connect_status == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "system massage", "esp32c6 connect wifi fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD

    if (Sys_Status.xl9555.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "xl9555 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.tca8418.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "tca8418 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.st25r3916.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "st25r3916 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.cc1101.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "cc1101 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.nrf24l01.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "nrf24l01 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.bq25896.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "bq25896 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
#endif

    if (Sys_Status.pcf8563.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "pcf8563 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.bq27220.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "bq27220 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.aw86224.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "aw86224 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.es8311.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "es8311 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.icm20948.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "icm20948 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.l76k.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "l76k init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (Sys_Status.lr2021.init_flag == false)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        _lock_acquire(&lvgl_api_lock);
        System_Ui->create_system_message_box(lv_screen_active(), "device massage", "lr2021 init fail");
        _lock_release(&lvgl_api_lock);

        while (System_Ui->_registry.system_message_box.occupancy_flag == true)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

#if CONFIG_ENABLE_USB_DISPLAY == true
#else
    Hardware_Usb_Cdc_Init();
#endif

    XL9535->begin();

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(200));

    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));

    XL9535->pin_mode(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Ethernet_Init();

#if defined CONFIG_SCREEN_TYPE_HI8561
    // 这个必须放在以太网后面
    HI8561_T->create_pwm(HI8561_SCREEN_BL, ledc_channel_t::LEDC_CHANNEL_0, 2000);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

    if (SGM38121->begin() == false)
    {
        printf("sgm38121 init fail\n");
        Sys_Status.sgm38121.init_flag = false;
    }
    else
    {
        printf("sgm38121 init success\n");
        Sys_Status.sgm38121.init_flag = true;
    }

#if defined CONFIG_CAMERA_TYPE_SC2336
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV2710
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1700);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3000);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV5645
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

    // bsp_init_refresh_monitor_io();

    Init_Ldo_Channel_Power(3, 2500);

    vTaskDelay(pdMS_TO_TICKS(100));

    if (App_Video_Init() == false)
    {
        printf("App_Video_Init fail\n");
        Sys_Status.camera.init_flag = false;
    }
    else
    {
        printf("App_Video_Init success\n");
        Sys_Status.camera.init_flag = true;
    }

#if (CONFIG_ENABLE_PPA_SCREEN_ROTATION == true) && (!defined SCREEN_ROTATION_DIRECTION_0)
    if (Ppa_Screen_Rotation_Init() == false)
    {
        printf("Ppa_Screen_Rotation_init fail\n");
    }
    else
    {
        printf("Ppa_Screen_Rotation_init success\n");
    }
#endif

#if CONFIG_ENABLE_USB_DISPLAY == true
    bool screen_init_ok = Usb_Screen_Init(&Screen_Mipi_Dpi_Panel);
#else
    bool screen_init_ok = Screen_Init(&Screen_Mipi_Dpi_Panel);
#endif
    if (screen_init_ok == false)
    {
        printf("Screen_Init fail\n");
    }

    esp_err_t assert = ESP_FAIL;
    if (Screen_Mipi_Dpi_Panel != NULL)
    {
        assert = esp_lcd_panel_init(Screen_Mipi_Dpi_Panel);
        if (assert != ESP_OK)
        {
            printf("esp_lcd_panel_init fail (error code: %#X)\n", assert);
            screen_init_ok = false;
        }
    }
    else
    {
        printf("esp_lcd_panel_init skip: Screen_Mipi_Dpi_Panel is NULL\n");
        screen_init_ok = false;
    }

#if defined CONFIG_SCREEN_TYPE_HI8561
    HI8561_T_IIC_Bus->set_bus_handle(XL9535_IIC_Bus->get_bus_handle());

    HI8561_T->begin();

#elif defined CONFIG_SCREEN_TYPE_RM69A10

    GT9895_IIC_Bus->set_bus_handle(XL9535_IIC_Bus->get_bus_handle());

    GT9895->begin();

#else
#error "unknown macro definition, please select the correct macro definition."
#endif

    // SDMMC_HOST_SLOT_1必须要先于SDMMC_HOST_SLOT_0初始化

    if (ESP32C6_AT->begin() == false)
    {
        printf("esp32c6 init fail\n");
        Sys_Status.esp32c6.init_flag = false;
    }
    else
    {
        printf("esp32c6 init success\n");
        Sys_Status.esp32c6.init_flag = true;
    }

    XL9535->pin_mode(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));

    if (Sdmmc_Init(SD_BASE_PATH) == false)
    {
        printf("Sdmmc_Init fail\n");
    }

    // if (Sdspi_Init(SD_BASE_PATH) == false)
    // {
    //     printf("Sdspi_Init fail\n");
    // }

    if (screen_init_ok == true)
    {
        Lvgl_Init();
        Lvgl_Startup();
        xTaskCreate(lvgl_ui_task, "lvgl_ui_task", 100 * 1024, NULL, 1, NULL);
    }
    else
    {
        printf("LVGL init skip: screen init failed\n");
    }

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    if (XL9555->begin() == false)
    {
        printf("xl9555 init fail\n");
        Sys_Status.xl9555.init_flag = false;
    }
    else
    {
        printf("xl9555 init success\n");
        Sys_Status.xl9555.init_flag = true;
    }

    XL9555->pin_mode(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH); // 关闭led
    XL9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    XL9555->pin_mode(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    TCA8418->create_gpio_interrupt(TCA8418_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       TCA8418_Interrupt_Flag = true;
                                   });

    if (TCA8418->begin() == false)
    {
        printf("tca8418 init fail\n");
        Sys_Status.tca8418.init_flag = false;
    }
    else
    {
        printf("tca8418 init success\n");
        Sys_Status.tca8418.init_flag = true;
    }
    TCA8418->set_keypad_scan_window(0, 0, TCA8418_KEYPAD_SCAN_WIDTH, TCA8418_KEYPAD_SCAN_HEIGHT);
    TCA8418->set_irq_pin_mode(Cpp_Bus_Driver::Tca8418::Irq_Mask::KEY_EVENTS);
    TCA8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);

    TCA8418->create_pwm(KEYBOARD_BL, ledc_channel_t::LEDC_CHANNEL_1, 1000000,
                        0, ledc_mode_t::LEDC_LOW_SPEED_MODE, ledc_timer_bit_t ::LEDC_TIMER_5_BIT);
    TCA8418->start_pwm_gradient_time(30, 1000);

    XL9555->pin_mode(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_write(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    if (St25r3916_Init() == false)
    {
        printf("st25r3916 init fail\n");
        Sys_Status.st25r3916.init_flag = false;
    }
    else
    {
        printf("st25r3916 init success\n");
        Sys_Status.st25r3916.init_flag = true;
    }

    Set_T_Mixrf_Lr1121_Sleep();

    XL9555->pin_mode(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_mode(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    ESP32P4->pin_mode(T_MIXRF_CC1101_BUSY, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

    ESP32P4->create_gpio_interrupt(T_MIXRF_CC1101_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::RISING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Cc1101_Interrupt_Flag = true;
                                   });

    Cc1101_SPI_Bus->_bus_init_flag = true;
    int16_t assert_2 = Cc1101.begin();
    if (assert_2 == RADIOLIB_ERR_NONE)
    {
        Sys_Status.cc1101.init_flag = true;
        printf("cc1101 init success\n");
    }
    else
    {
        Sys_Status.cc1101.init_flag = false;
        printf("cc1101 init fail (error code: %d)\n", assert_2);
    }

    System_Ui->set_config_rf_params(System_Ui->_device_cc1101);

    Nrf24l01_SPI_Bus->_bus_init_flag = true;
    assert_2 = Nrf24l01.begin();
    if (assert_2 == RADIOLIB_ERR_NONE)
    {
        Sys_Status.nrf24l01.init_flag = true;
        printf("nrf24l01 init success\n");
    }
    else
    {
        Sys_Status.nrf24l01.init_flag = false;
        printf("nrf24l01 init fail (error code: %d)\n", assert_2);
    }

    ESP32P4->create_gpio_interrupt(T_MIXRF_NRF24L01_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Nrf24l01_Interrupt_Flag = true;
                                   });

    System_Ui->set_config_rf_params(System_Ui->_device_nrf24l01);

    assert = Kode_Bq25896::bq25896_init(Bq25896_Iic_Bus, Bq25896_Handle);
    if (assert != ESP_OK)
    {
        Sys_Status.bq25896.init_flag = false;
        printf("bq25896 init fail (error code: %#X)\n", assert);
    }
    else
    {
        Sys_Status.bq25896.init_flag = true;
        printf("bq25896 init success\n");

        // 禁用看门狗后不能读取看门狗寄存器状态，否者看门狗禁用会失效
        Kode_Bq25896::bq25896_set_watchdog_timer(Bq25896_Handle, Kode_Bq25896::bq25896_watchdog_t::BQ25896_WATCHDOG_DISABLE);

        Kode_Bq25896::bq25896_set_adc_conversion(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
        Kode_Bq25896::bq25896_set_adc_conversion_rate(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);

        // Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);
    }

#endif

#if CONFIG_ENABLE_USB_DISPLAY == true
#else
#if defined CONFIG_SCREEN_TYPE_HI8561
    HI8561_T->start_pwm_gradient_time(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        set_rm69a10_brightness(Screen_Mipi_Dpi_Panel, i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
#endif

    PCF8563_IIC_Bus->set_bus_handle(XL9535_IIC_Bus->get_bus_handle());

    if (PCF8563->begin() == false)
    {
        printf("pcf8563 init fail\n");
        Sys_Status.pcf8563.init_flag = false;
    }
    else
    {
        printf("pcf8563 init success\n");
        Sys_Status.pcf8563.init_flag = true;
    }

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(20);
    _lock_release(&lvgl_api_lock);

    // ESP32C6复位模式
    // XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Esp32c6_At_Init();

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(40);
    _lock_release(&lvgl_api_lock);

    BQ27220_IIC_Bus->set_bus_handle(XL9535_IIC_Bus->get_bus_handle());

    if (BQ27220->begin() == false)
    {
        printf("bq27220 init fail\n");
        Sys_Status.bq27220.init_flag = false;
    }
    else
    {
        printf("bq27220 init success\n");
        Sys_Status.bq27220.init_flag = true;
    }

    // 设置的电池容量会在没有电池插入的时候自动还原为默认值
    BQ27220->set_design_capacity(1000);
    BQ27220->set_temperature_mode(Cpp_Bus_Driver::Bq27220xxxx::Temperature_Mode::EXTERNAL_NTC);
    BQ27220->set_sleep_current_threshold(50);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(50);
    _lock_release(&lvgl_api_lock);

    AW86224_IIC_Bus->set_bus_handle(SGM38121_IIC_Bus->get_bus_handle());

    if (AW86224->begin(500000) == false)
    {
        printf("aw86224 init fail\n");
        Sys_Status.aw86224.init_flag = false;
    }
    else
    {
        printf("aw86224 init success\n");
        Sys_Status.aw86224.init_flag = true;
    }
    // printf("AW86224 input voltage: %.06f V\n", AW86224->get_input_voltage());

    // RAM播放
    AW86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(60);
    _lock_release(&lvgl_api_lock);

    ES8311_IIC_Bus->set_bus_handle(SGM38121_IIC_Bus->get_bus_handle());
    ES8311_Init();

    // if (Play_Wav_File_2(SD_FILE_PATH_MUSIC) == false)
    // {
    //     printf("Play_Wav_File fail\n");
    // }
    // else
    // {
    //     printf("Play_Wav_File complete\n");
    // }

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(70);
    _lock_release(&lvgl_api_lock);

    Wire1._bus->set_bus_handle(SGM38121_IIC_Bus->get_bus_handle());
    if (ICM20948_Init() == false)
    {
        printf("icm20948 init fail\n");
        Sys_Status.icm20948.init_flag = false;
    }
    else
    {
        printf("icm20948 init success\n");
        Sys_Status.icm20948.init_flag = true;
    }

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(80);
    _lock_release(&lvgl_api_lock);

    // XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    if (L76K->begin() == false)
    {
        L76K_Uart_Bus->set_baud_rate(115200);

        if (L76K->begin() == false)
        {
            printf("l76k init fail\n");
            Sys_Status.l76k.init_flag = false;
        }
        else
        {
            printf("l76k init success\n");
            Sys_Status.l76k.init_flag = true;
        }
    }
    else
    {
        printf("l76k init success\n");
        Sys_Status.l76k.init_flag = true;

        L76K->set_baud_rate(Cpp_Bus_Driver::L76k::Baud_Rate::BR_115200_BPS);
    }
    printf("get_baud_rate:%ld\n", L76K->get_baud_rate());
    L76K->set_update_frequency(Cpp_Bus_Driver::L76k::Update_Freq::FREQ_5HZ);
    L76K->clear_rx_buffer_data();
    L76K->sleep(true);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(90);
    _lock_release(&lvgl_api_lock);

    // SKY13453 射频开关控制引脚（与 sx1262 共用）
    XL9535->pin_mode(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    XL9535->pin_mode(XL9535_LR2021_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    // LORA复位
    XL9535->pin_mode(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    XL9535->pin_mode(XL9535_LR2021_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    LR2021_SPI_Bus->_bus_init_flag = true;
#endif
    Lr2021.irqDioNum = 11;
    if (System_Ui->set_config_rf_params(System_Ui->_device_lr2021) == false)
    {
        printf("lr2021 init fail\n");
        Sys_Status.lr2021.init_flag = false;
    }
    else
    {
        printf("lr2021 init success\n");
        Sys_Status.lr2021.init_flag = true;
    }

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(100);
    _lock_release(&lvgl_api_lock);

    _lock_acquire(&lvgl_api_lock);
    System_Ui->begin();
    _lock_release(&lvgl_api_lock);

#if CONFIG_ENABLE_USB_DISPLAY == true
#else
    xTaskCreate(hardware_usb_cdc_task, "hardware_usb_cdc_task", 4 * 1024, NULL, 3, NULL);
#endif
    xTaskCreate(device_vibration_task, "device_vibration_task", 4 * 1024, NULL, 2, &Vibration_Task_Handle);
    xTaskCreate(device_speaker_task, "device_speaker_task", 4 * 1024, NULL, 3, &Speaker_Task_Handle);
    xTaskCreate(device_microphone_task, "device_microphone_task", 4 * 1024, NULL, 3, &Microphone_Task_Handle);
    xTaskCreate(device_imu_task, "device_imu_task", 4 * 1024, NULL, 3, &Imu_Task_Handle);
    xTaskCreate(device_battery_health_task, "device_battery_health_task", 8 * 1024, NULL, 3, NULL);
    xTaskCreate(device_gps_task, "device_gps_task", 8 * 1024, NULL, 3, &Gps_Task_Handle);
    xTaskCreate(device_ethernet_task, "device_ethernet_task", 4 * 1024, NULL, 3, &Ethernet_Task_Handle);
    xTaskCreate(device_rtc_task, "device_rtc_task", 4 * 1024, NULL, 3, NULL);
    xTaskCreate(device_at_task, "device_at_task", 4 * 1024, NULL, 3, &At_Task_Handle);
    // xTaskCreate(esp32p4_sleep_task, "esp32p4_sleep_task", 4 * 1024, NULL, 3, &Sleep_Task_Handle);
    xTaskCreate(device_rf_task, "device_rf_task", 4 * 1024, NULL, 3, &Rf_Task_Handle);
    xTaskCreate(iis_transmission_data_stream_task, "iis_transmission_data_stream_task", 4 * 1024, NULL, 4, &Iis_Transmission_Data_Stream_Task);
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    xTaskCreate(device_nfc_task, "device_nfc_task", 8 * 1024, NULL, 3, &Nfc_Task_Handle);
#endif

    // 等待lvgl刷新完成
    while (lv_display_flush_is_last(lv_display_get_default()) == false)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    printf("system ui init finish\n");
    System_Ui->set_vibration();

    System_Startup_Message_Init();

    //     while (1)
    //     {
    //         if (esp_log_timestamp() > Cycle_Time)
    //         {
    // #if defined CONFIG_SCREEN_TYPE_HI8561
    //             Cpp_Bus_Driver::Hi8561_Touch::Touch_Point tp;

    //             if (HI8561_T->get_multiple_touch_point(tp) == true)
    //             {
    //                 printf("touch finger: %d edge touch flag: %d\n", tp.finger_count, tp.edge_touch_flag);

    //                 for (uint8_t i = 0; i < tp.info.size(); i++)
    //                 {
    //                     printf("touch num:[%d] x: %d y: %d p: %d\n", i + 1, tp.info[i].x, tp.info[i].y, tp.info[i].pressure_value);
    //                 }
    //             }
    // #elif defined CONFIG_SCREEN_TYPE_RM69A10
    //             Cpp_Bus_Driver::Gt9895::Touch_Point tp;

    //             if (GT9895->get_multiple_touch_point(tp) == true)
    //             {
    //                 printf("touch finger: %d edge touch flag: %d\n", tp.finger_count, tp.edge_touch_flag);

    //                 for (uint8_t i = 0; i < tp.info.size(); i++)
    //                 {
    //                     printf("touch num:[%d] id:[%d] x: %d y: %d p: %d\n", i + 1, tp.info[i].finger_id, tp.info[i].x, tp.info[i].y, tp.info[i].pressure_value);
    //                 }
    //             }
    // #else
    // #error "unknown macro definition, please select the correct macro definition."
    // #endif

    //             Cycle_Time = esp_log_timestamp() + 1000;
    //         }

    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
}
