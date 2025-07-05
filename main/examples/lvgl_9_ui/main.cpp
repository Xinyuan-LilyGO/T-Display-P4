/*
 * @Description: lvgl_9_ui
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:34:16
 * @LastEditTime: 2025-07-04 18:20:36
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
#include "hi8561_driver.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "lvgl_ui.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "esp_vfs_fat.h"
#include "c2_b16_s44100.h"
#include "ICM20948_WE.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "ethernet_init.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "app_video.h"
#include "driver/ppa.h"
#include "esp_private/esp_cache_private.h"
#include <fstream>

#define SD_FILE_PATH_MUSIC "/sdcard/t_display_p4_lvgl_9_ui_resource/music/Eagles - Hotel California (Live on MTV, 1994).wav"

#define LVGL_TICK_PERIOD_MS 1

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

typedef struct
{
    uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + PREPEND_LENGTH + 1]; // Data buffer
    size_t buf_len;                                                  // Number of bytes received
    uint8_t itf;                                                     // Index of CDC device interface
} app_message_t;

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

Ethernet_Info Eth_Info;

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
TaskHandle_t Lora_Task_Handle = NULL;
TaskHandle_t Iis_Transmission_Data_Stream_Task = NULL;

uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

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
void *lcd_buffer[CONFIG_EXAMPLE_CAM_BUF_COUNT];
int32_t fps_count;
int64_t start_time;
int32_t video_cam_fd0;
bool Camera_Init_Status = false;

bool Lora_Send_falg = false;
uint8_t Lora_Send_Package[255] = {0};

bool Device_Lora_Task_Stop_Flag = false;

QueueHandle_t app_queue;

esp_lcd_panel_handle_t Screen_Mipi_Dpi_Panel = NULL;

// IIC 1
auto XL9535_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto HI8561_T_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(HI8561_TOUCH_SDA, HI8561_TOUCH_SCL, I2C_NUM_0);
auto BQ27220_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ27220_SDA, BQ27220_SCL, I2C_NUM_0);
auto PCF8563_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(PCF8563_SDA, PCF8563_SCL, I2C_NUM_0);

// IIC 2
auto SGM38121_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);
auto AW86224_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);
auto ES8311_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);
auto ICM20948_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ICM20948_SDA, ICM20948_SCL, I2C_NUM_1);

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
auto SX1262_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(LORA_MOSI, LORA_SCLK, LORA_MISO, SPI3_HOST, 0);

// IIC 1
auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(XL9535_IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto HI8561_T = std::make_unique<Cpp_Bus_Driver::Hi8561_Touch>(HI8561_T_IIC_Bus, HI8561_TOUCH_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto BQ27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(BQ27220_IIC_Bus, BQ27220_IIC_ADDRESS);
auto PCF8563 = std::make_unique<Cpp_Bus_Driver::Pcf8563x>(PCF8563_IIC_Bus, PCF8563_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

// IIC 2
auto SGM38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(SGM38121_IIC_Bus, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto AW86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(AW86224_IIC_Bus, AW86224_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ES8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(ES8311_IIC_Bus, ES8311_IIS_Bus, ES8311_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ICM20948 = std::make_unique<ICM20948_WE>(ICM20948_IIC_Bus, ICM20948_IIC_ADDRESS);

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
auto SX1262 = std::make_unique<Cpp_Bus_Driver::Sx126x>(SX1262_SPI_Bus, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262, LORA_BUSY,
                                                       LORA_CS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

auto System_Ui = std::make_unique<Lvgl_Ui::System>(SCREEN_WIDTH, SCREEN_HEIGHT);

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

//         SX1262->set_sleep();

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
//         ESP32P4->start_pwm_gradient_time(0, 500);

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

    System_Ui->set_wifi_connect_status(true);

    // _lock_acquire(&lvgl_api_lock);
    // System_Ui->status_bar_wifi_connect_status_update();
    // _lock_release(&lvgl_api_lock);
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
            Music_Play_End_Flag = false;

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

            if (Iis_Transmission_Data_Stream.size() > 1024 * 8)
            {
                // 存储数据
                // memcpy(data_buffer.get(), Iis_Transmission_Data_Stream.data(), 1024 * 100);
                // size_t bytes_read = ES8311->write_data(data_buffer.get(), 1024 * 100); // 这一行需要根据你的 I2S 驱动实现来修改
                // if (bytes_read > 0)
                // {
                //     // 删除已经存储的数据
                //     Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.begin(), Iis_Transmission_Data_Stream.begin() + bytes_read);
                // }

                size_t bytes_read = ES8311->write_data(Iis_Transmission_Data_Stream.data() + Iis_Read_Data_Size_Index, 1024 * 8); // 这一行需要根据你的 I2S 驱动实现来修改
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

    Music_File.close();

    printf("music play finish\n");

    System_Ui->_registry.win.music.play_flag = false;

    _lock_acquire(&lvgl_api_lock);
    System_Ui->set_win_music_play_imagebutton_status(System_Ui->_registry.win.music.play_flag);
    System_Ui->set_win_music_current_total_time(0, duration);
    _lock_release(&lvgl_api_lock);

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
                int16_t microphone_data[10] = {0};
                ES8311->read_data(microphone_data, 10 * sizeof(int16_t));

                // 提取10个数据中最大的数据
                int16_t max_microphone_data = *std::max_element(microphone_data, microphone_data + 10);
                uint8_t max_microphone_data_percentage = (static_cast<float>(max_microphone_data) / static_cast<float>(32767)) * 100;

                // 将麦克风数据格式化为字符串
                std::string microphone_data_str = "microphone data: " + std::to_string(max_microphone_data);

                _lock_acquire(&lvgl_api_lock);
                // 更新麦克风圆盘
                // 使用动画
                lv_anim_t anim;
                lv_anim_init(&anim);
                lv_anim_set_var(&anim, System_Ui->_registry.win.cit.microphone_test.needle_line);
                lv_anim_set_values(&anim, System_Ui->_registry.win.cit.microphone_test.data.value_percentage, max_microphone_data_percentage);
                lv_anim_set_time(&anim, 100); // Animation duration in milliseconds
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
                xyzFloat gValue = ICM20948->getGValues();
                xyzFloat angle = ICM20948->getAngles();
                float pitch = ICM20948->getPitch();
                float roll = ICM20948->getRoll();

                // 获取磁力计的 x, y 值以计算航向角（Yaw）
                xyzFloat magValues = ICM20948->getMagValues();
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
                std::string battery_health_data_str = "battery health data:\n";
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
                battery_health_data_str += "charging voltage: " + std::to_string(BQ27220->get_charging_voltage()) + " mv\n";
                battery_health_data_str += "charging current: " + std::to_string(BQ27220->get_charging_current()) + " ma\n";
                battery_health_data_str += "standby current: " + std::to_string(BQ27220->get_standby_current()) + " ma\n";
                battery_health_data_str += "max load current: " + std::to_string(BQ27220->get_max_load_current()) + " ma\n";
                battery_health_data_str += "average power: " + std::to_string(BQ27220->get_average_power()) + " mw\n\n";

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

                _lock_acquire(&lvgl_api_lock);
                // 更新数据的标签
                lv_label_set_text(System_Ui->_registry.win.cit.battery_health_test.data_label, battery_health_data_str.c_str());
                lv_obj_align(System_Ui->_registry.win.cit.battery_health_test.data_label, LV_ALIGN_TOP_MID, 0, 10);
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
                std::shared_ptr<uint8_t[]> buffer;
                uint32_t buffer_length = 0;

                if (L76K->get_info_data(buffer, &buffer_length) == true)
                {
                    // 打印RMC的相关信息
                    Cpp_Bus_Driver::L76k::Rmc rmc;

                    if (L76K->parse_rmc_info(buffer, buffer_length, rmc) == true)
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

void device_lora_task(void *arg)
{
    printf("device_lora_task start\n");

    size_t cycle_time = 0;
    size_t auto_send_cycle_time = 0;

    while (1)
    {
        // if (esp_log_timestamp() > cycle_time)
        // {
        //     printf("SX1262 ID: %#X\n", SX1262->get_device_id());

        //     printf("SX1262 get current limit: %d\n", SX1262->get_current_limit());

        //     switch (SX1262->get_packet_type())
        //     {
        //     case Cpp_Bus_Driver::Sx126x::Packet_Type::GFSK:
        //         printf("SX1262 packet type: GFSK\n");
        //         break;
        //     case Cpp_Bus_Driver::Sx126x::Packet_Type::LORA:
        //         printf("SX1262 packet type: LORA\n");
        //         break;
        //     case Cpp_Bus_Driver::Sx126x::Packet_Type::LR_FHSS:
        //         printf("SX1262 packet type: LR_FHSS\n");
        //         break;

        //     default:
        //         break;
        //     }

        //     switch (SX1262->assert_chip_mode_status(SX1262->get_status()))
        //     {
        //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::STBY_RC:
        //         printf("SX1262 chip mode status: STBY_RC\n");
        //         break;
        //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::STBY_XOSC:
        //         printf("SX1262 chip mode status: STBY_XOSC\n");
        //         break;
        //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::FS:
        //         printf("SX1262 chip mode status: FS\n");
        //         break;
        //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::RX:
        //         printf("SX1262 chip mode status: RX\n");
        //         break;
        //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::TX:
        //         printf("SX1262 chip mode status: TX\n");
        //         break;

        //     default:
        //         break;
        //     }

        //     cycle_time = esp_log_timestamp() + 1000;
        // }

        if (System_Ui->_device_lora.auto_send.flag == true)
        {
            if (Lora_Send_falg == false)
            {
                if (esp_log_timestamp() > auto_send_cycle_time)
                {
                    memset(Lora_Send_Package, '\0', sizeof(Lora_Send_Package));

                    // 检查长度是否越界
                    if (System_Ui->_device_lora.auto_send.text.size() <= 255)
                    {
                        memcpy(Lora_Send_Package, System_Ui->_device_lora.auto_send.text.data(), System_Ui->_device_lora.auto_send.text.size());
                    }
                    else
                    {
                        // 处理错误：数据过长
                        memcpy(Lora_Send_Package, System_Ui->_device_lora.auto_send.text.data(), 254);
                        Lora_Send_Package[254] = '\0';

                        printf("lora send out of bounds(data > Lora_Send_Package)\n");
                    }

                    char buffer_time[15];
                    snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                    Lvgl_Ui::System::Win_Lora_Chat_Message wlcm =
                        {
                            .direction = Lvgl_Ui::System::Chat_Message_Direction::SEND,
                            .time = buffer_time,
                            .data = System_Ui->_device_lora.auto_send.text,
                        };
                    System_Ui->_registry.win.lora.chat_message_data.push_back(wlcm);

                    if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::LORA)
                    {
                        // 更新聊天容器
                        _lock_acquire(&lvgl_api_lock);
                        System_Ui->win_lora_chat_message_data_update(System_Ui->_registry.win.lora.chat_message_data);
                        _lock_release(&lvgl_api_lock);
                    }

                    Lora_Send_falg = true;

                    auto_send_cycle_time = esp_log_timestamp() + System_Ui->_device_lora.auto_send.interval;
                }
            }
        }

        if (Lora_Send_falg == true)
        {
            // 设置发送模式，发送完成后进入快速切换模式（FS模式）
            SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::TX, 0, Cpp_Bus_Driver::Sx126x::Fallback_Mode::FS);
            SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::TX_DONE);
            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::TX_DONE);

            if (SX1262->send_data(Lora_Send_Package, strlen(reinterpret_cast<const char *>(Lora_Send_Package))) == true)
            {
                uint16_t timeout_count = 0;
                printf("SX1262 send start\n");
                printf("SX1262 send data size: %d\n", strlen(reinterpret_cast<const char *>(Lora_Send_Package)));

                while (1) // 等待发送完成
                {
                    if (XL9535->pin_read(XL9535_LORA_DIO1) == 1) // 发送完成中断
                    {
                        // 方法1（速度比方法2快）
                        //  获取芯片模式状态
                        //  先前设置发送成功后进入FS模式，所以这里进入FS模式即判断成功发送
                        if (SX1262->assert_chip_mode_status(SX1262->get_status()) == Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::FS)
                        {
                            printf("SX1262 send success\n");
                            break;
                        }

                        // //方法2（速度比方法1慢）
                        // // 检查中断
                        // Cpp_Bus_Driver::Sx126x::Iqr_Status is;
                        // if (SX1262->assert_iqr_status(SX1262->get_irq_status(), is) == false)
                        // {
                        //     printf("assert_Iqr_status fail\n");
                        // }
                        // else
                        // {
                        //     if (is.all_flag.tx_done == true) // 发送完成
                        //     {
                        //         printf("SX1262 send success\n");
                        //         break;
                        //     }
                        // }
                    }

                    timeout_count++;
                    if (timeout_count > 1000) // 超时
                    {
                        printf("SX1262 send timeout\n");
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
            else
            {
                printf("SX1262 send fail\n");
            }

            // vTaskDelay(pdMS_TO_TICKS(1000));

            // 还原接收模式
            SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
            SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);
            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);

            Lora_Send_falg = false;
        }

        if (XL9535->pin_read(XL9535_LORA_DIO1) == 1) // 接收完成中断
        {
            // 检查中断
            Cpp_Bus_Driver::Sx126x::Irq_Status is;
            if (SX1262->assert_irq_status(SX1262->get_irq_flag(), is) == false)
            {
                printf("assert_iqr_status fail\n");
            }
            else
            {
                if (is.all_flag.tx_rx_timeout == true)
                {
                    printf("receive timeout\n");
                    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::TIMEOUT);
                }
                else if (is.all_flag.crc_error == true)
                {
                    printf("receive crc error\n");
                    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::CRC_ERROR);
                }
                else if (is.lora_reg_flag.header_error == true)
                {
                    printf("receive header error\n");
                    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::HEADER_ERROR);
                }
                else
                {
                    uint8_t receive_package[255] = {0};
                    uint8_t length_buffer = SX1262->receive_data(receive_package);
                    if (length_buffer == 0)
                    {
                        printf("SX1262 receive fail (error assert: %d)\n", SX1262->_assert);
                    }
                    else
                    {
                        Cpp_Bus_Driver::Sx126x::Packet_Metrics pm;
                        if (SX1262->get_lora_packet_metrics(pm) == true)
                        {
                            printf("SX1262 receive rssi_average: %.01f rssi_instantaneous: %.01f snr: %.01f\n", pm.lora.rssi_average, pm.lora.rssi_instantaneous, pm.lora.snr);
                        }

                        for (uint8_t i = 0; i < length_buffer; i++)
                        {
                            printf("get SX1262 data[%d]: %d\n", i, receive_package[i]);
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

                        char buffer_rssi_snr[30];
                        snprintf(buffer_rssi_snr, sizeof(buffer_rssi_snr), "rssi[%.01f] snr[%.01f]", pm.lora.rssi_average, pm.lora.snr);

                        Lvgl_Ui::System::Win_Lora_Chat_Message wlcm =
                            {
                                .direction = Lvgl_Ui::System::Chat_Message_Direction::RECEIVE,
                                .time = buffer_time,
                                .data = message_str,
                                .rssi_snr = buffer_rssi_snr,
                            };
                        System_Ui->_registry.win.lora.chat_message_data.push_back(wlcm);

                        if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::LORA)
                        {
                            // 更新聊天容器
                            _lock_acquire(&lvgl_api_lock);
                            System_Ui->win_lora_chat_message_data_update(System_Ui->_registry.win.lora.chat_message_data);
                            _lock_release(&lvgl_api_lock);
                        }
                    }
                }
            }

            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);
        }

        // 如果有触发停止标志就等待lora一次发送或接收过程完成后再停止
        // 这样做为了防止spi意外终止导致的iic的0x107错误
        // 多任务处理spi和iic不能同时工作，spi工作的时候有概率会导致iic死机
        if (Device_Lora_Task_Stop_Flag == true)
        {
            vTaskSuspend(Lora_Task_Handle);
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
                    Iis_Transmission_Data_Stream.resize(current_buf_size + 1024 * 10);

                    Music_File.read(Iis_Transmission_Data_Stream.data() + current_buf_size, 1024 * 10);
                    std::streamsize bytes_read = Music_File.gcount(); // 获取实际读取的字节数
                    // 如果实际读取的字节数小于预期，则从末尾扣除多余的空间
                    if (bytes_read < 1024 * 10)
                    {
                        Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.end() - (1024 * 10 - bytes_read), Iis_Transmission_Data_Stream.end());
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
            if (Iis_Read_Data_Size_Index > 1024 * 100)
            {
                // 删除已经存储的数据
                Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.begin(), Iis_Transmission_Data_Stream.begin() + 1024 * 100);
                Iis_Read_Data_Size_Index -= 1024 * 100;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void bsp_enable_dsi_phy_power(void)
{
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    printf("mipi dsi phy powered on\n");
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

        System_Ui->_touch_point = tp;

        if (tp.edge_touch_flag == true)
        {
            tp.edge_touch_flag = false;
            System_Ui->_edge_touch_flag = true;

            edge_touch_scheduled_shutdown_time = esp_log_timestamp() + 100;
            edge_touch_scheduled_shutdown_lock = true;
        }

        tp.info.clear();
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    // }
}

bool Mipi_Dsi_Init(uint8_t num_data_lanes, uint32_t lane_bit_rate_mbps, uint32_t dpi_clock_freq_mhz, lcd_color_rgb_pixel_format_t color_rgb_pixel_format, uint8_t num_fbs, uint32_t width, uint32_t height,
                   uint32_t mipi_dsi_hsync, uint32_t mipi_dsi_hbp, uint32_t mipi_dsi_hfp, uint32_t mipi_dsi_vsync, uint32_t mipi_dsi_vbp, uint32_t mipi_dsi_vfp,
                   uint32_t bits_per_pixel, esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    esp_lcd_panel_io_handle_t mipi_dbi_io;

    auto cpp_assert = std::make_unique<Cpp_Bus_Driver::Tool>();

    // create MIPI DSI bus first, it will initialize the DSI PHY as well
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = num_data_lanes,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = lane_bit_rate_mbps,
    };

    esp_err_t assert = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_dsi_bus fail (error code: %#X)\n", assert);
        return false;
    }

    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t dbi_io_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // according to the LCD spec
        .lcd_param_bits = 8, // according to the LCD spec
    };
    assert = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_io_config, &mipi_dbi_io);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_panel_io_dbi fail (error code: %#X)\n", assert);
        return false;
    }

    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = dpi_clock_freq_mhz,
        .pixel_format = color_rgb_pixel_format,
        .num_fbs = num_fbs,
        .video_timing = {
            .h_size = width,
            .v_size = height,
            .hsync_pulse_width = mipi_dsi_hsync,
            .hsync_back_porch = mipi_dsi_hbp,
            .hsync_front_porch = mipi_dsi_hfp,
            .vsync_pulse_width = mipi_dsi_vsync,
            .vsync_back_porch = mipi_dsi_vbp,
            .vsync_front_porch = mipi_dsi_vfp,
        },
        .flags = {
            .use_dma2d = true, // use DMA2D to copy draw buffer into frame buffer
        }};

    hi8561_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = bits_per_pixel,
        .vendor_config = &vendor_config,
    };
    assert = esp_lcd_new_panel_hi8561(mipi_dbi_io, &dev_config, mipi_dpi_panel);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_panel_hi8561 fail (error code: %#X)\n", assert);
        return false;
    }

    return true;
}

bool Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    if (Mipi_Dsi_Init(HI8561_SCREEN_DATA_LANE_NUM, HI8561_SCREEN_LANE_BIT_RATE_MBPS, HI8561_SCREEN_MIPI_DSI_DPI_CLK_MHZ, LCD_COLOR_PIXEL_FORMAT_RGB565,
                      0, HI8561_SCREEN_WIDTH, HI8561_SCREEN_HEIGHT, HI8561_SCREEN_MIPI_DSI_HSYNC, HI8561_SCREEN_MIPI_DSI_HBP,
                      HI8561_SCREEN_MIPI_DSI_HFP, HI8561_SCREEN_MIPI_DSI_VSYNC, HI8561_SCREEN_MIPI_DSI_VBP, HI8561_SCREEN_MIPI_DSI_VFP,
                      HI8561_SCREEN_BITS_PER_PIXEL_RGB565, mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    return true;
}

// bool Usb_Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
// {
//     usb_display_vendor_config_t vendor_config_usb = DEFAULT_USB_DISPLAY_VENDOR_CONFIG(HI8561_SCREEN_WIDTH, HI8561_SCREEN_HEIGHT,
//                                                                                       HI8561_SCREEN_BITS_PER_PIXEL_RGB565, mipi_dpi_panel);

//     if (esp_lcd_new_panel_usb_display(&vendor_config_usb, &mipi_dpi_panel) != ESP_OK)
//     {
//         printf("esp_lcd_new_panel_usb_display fail\n");
//         return false;
//     }

//     return true;
// }

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
        if (Camera_Init_Status == true)
        {
            esp_err_t assert;
            if (status == true)
            {
                assert = app_video_stream_task_restart(video_cam_fd0);
                if (assert != ESP_OK)
                {
                    printf("app_video_stream_task_restart fail (error code: %#X)\n", assert);
                }
                else
                {
                    // Get the initial time for frame rate statistics
                    start_time = esp_timer_get_time();
                }
            }
            else
            {
                assert = app_video_stream_task_stop(video_cam_fd0);
                if (assert != ESP_OK)
                {
                    printf("app_video_stream_task_stop fail (error code: %#X)\n", assert);
                }
            }
        }
    };

    System_Ui->_win_lora_config_lora_params_callback = [](Lvgl_Ui::System::Device_Lora device_lora) -> bool
    {
        if (SX1262->config_lora_params(device_lora.params.freq, device_lora.params.bw, device_lora.params.current_limit,
                                       device_lora.params.power, device_lora.params.sf, device_lora.params.cr, device_lora.params.crc_type,
                                       device_lora.params.preamble_length, device_lora.params.sync_word) == false)
        {
            printf("config_lora_params fail\n");
            return false;
        }
        SX1262->clear_buffer();
        SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
        SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);
        SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);

        printf("lora config_lora_params finish start lora transmit\n");
        return true;
    };

    System_Ui->_win_lora_send_data_callback = [](std::string data)
    {
        memset(Lora_Send_Package, '\0', sizeof(Lora_Send_Package));

        // 检查长度是否越界
        if (data.size() <= 255)
        {
            memcpy(Lora_Send_Package, data.data(), data.size());
        }
        else
        {
            // 处理错误：数据过长
            memcpy(Lora_Send_Package, data.data(), 254);
            Lora_Send_Package[254] = '\0';

            printf("lora send out of bounds(data > Lora_Send_Package)\n");
        }

        Lora_Send_falg = true;
    };

    System_Ui->_win_lora_status_callback = [](bool status)
    {
        if (status == true)
        {
            Device_Lora_Task_Stop_Flag = false;
            vTaskResume(Lora_Task_Handle);
        }
        else
        {
            Device_Lora_Task_Stop_Flag = true;
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
}

void Lvgl_Init(void)
{
    printf("initialize lvgl\n");

    lv_init();

    // create a lvgl display
    lv_display_t *display = lv_display_create(HI8561_SCREEN_WIDTH, HI8561_SCREEN_HEIGHT);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, Screen_Mipi_Dpi_Panel);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    // create draw buffer
    printf("allocate separate lvgl draw buffers\n");
    size_t draw_buffer_sz = HI8561_SCREEN_WIDTH * HI8561_SCREEN_HEIGHT * sizeof(lv_color_t);
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(buf1);
    void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // set the callback which can copy the rendered image to an area of the display

    lv_display_set_flush_cb(display, [](lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
                            {
                                esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
                                int offsetx1 = area->x1;
                                int offsetx2 = area->x2;
                                int offsety1 = area->y1;
                                int offsety2 = area->y2;
                                // pass the draw buffer to the driver
                                esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map); });

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

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
    }
    else
    {
        printf("es8311 initialization fail\n");
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
    if (ICM20948->begin() == false)
    {
        printf("ICM20948 AG initialization failed\n");
        return false;
    }

    if (ICM20948->initMagnetometer() == false)
    {
        printf("ICM20948 M initialization failed\n");
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
    }
    else
    {
        printf("set_wifi_connect fail\n");
    }

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

        System_Ui->set_wifi_connect_status(false);

        // _lock_acquire(&lvgl_api_lock);
        // System_Ui->status_bar_wifi_connect_status_update();
        // _lock_release(&lvgl_api_lock);
    }
}

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

void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves,
                                  size_t camera_buf_len, void *user_data)
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

    ppa_srm_oper_config_t srm_config =
        {
            .in =
                {
                    .buffer = camera_buf,
                    .pic_w = camera_buf_hes,
                    .pic_h = camera_buf_ves,
                    .block_w = camera_buf_hes,
                    .block_h = camera_buf_ves,
                    .block_offset_x = (camera_buf_hes > HI8561_SCREEN_WIDTH) ? (camera_buf_hes - HI8561_SCREEN_WIDTH) / 2 : 0,
                    .block_offset_y = (camera_buf_ves > HI8561_SCREEN_HEIGHT) ? (camera_buf_ves - HI8561_SCREEN_HEIGHT) / 2 : 0,
                    .srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,
                },

            .out =
                {
                    .buffer = lcd_buffer[camera_buf_index],
                    .buffer_size = ALIGN_UP(HI8561_SCREEN_WIDTH * HI8561_SCREEN_HEIGHT * (APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? 2 : 3), data_cache_line_size),
                    .pic_w = HI8561_SCREEN_WIDTH,
                    .pic_h = HI8561_SCREEN_HEIGHT,
                    .block_offset_x = 0,
                    .block_offset_y = 0,
                    .srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,
                },

            .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
            .scale_x = 1,
            .scale_y = 1,
            .mirror_x = false,
            .mirror_y = true,
            .rgb_swap = false,
            .byte_swap = false,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };

    if (camera_buf_hes > HI8561_SCREEN_WIDTH || camera_buf_ves > HI8561_SCREEN_HEIGHT)
    {
        // The resolution of the camera does not match the LCD resolution. Image processing can be done using PPA, but there will be some frame rate loss

        srm_config.in.block_w = (camera_buf_hes > HI8561_SCREEN_WIDTH) ? HI8561_SCREEN_WIDTH : camera_buf_hes;
        srm_config.in.block_h = (camera_buf_ves > HI8561_SCREEN_HEIGHT) ? HI8561_SCREEN_HEIGHT : camera_buf_ves;

        esp_err_t assert = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
        if (assert != ESP_OK)
        {
            printf("ppa_do_scale_rotate_mirror fail (error code: %#X)\n", assert);
        }

        if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CAMERA)
        {
#if defined CONFIG_CAMERA_TYPE_SC2336 || defined CONFIG_CAMERA_TYPE_OV2710
            assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 + 120,
                                               srm_config.in.block_w, srm_config.in.block_h + (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 - 120, lcd_buffer[camera_buf_index]);
#elif defined CONFIG_CAMERA_TYPE_OV5645
            assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 + 150,
                                               srm_config.in.block_w, srm_config.in.block_h + (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 - 150, lcd_buffer[camera_buf_index]);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif
            if (assert != ESP_OK)
            {
                printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
            }

            // _lock_acquire(&lvgl_api_lock);
            // lv_canvas_set_buffer(System_Ui->_registry.win.camera.canvas, lcd_buffer[camera_buf_index],
            //                      srm_config.in.block_w, srm_config.in.block_h + (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2,
            //                      LCD_COLOR_PIXEL_FORMAT_RGB565);
            // _lock_release(&lvgl_api_lock);
        }
    }
    else
    {
        // esp_err_t assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, 0, camera_buf_hes, camera_buf_ves, camera_buf);
        // if (assert != ESP_OK)
        // {
        //     printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
        // }
    }
}

bool App_Video_Init()
{
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

    if (Mipi_Dsi_Init(CAMERA_DATA_LANE_NUM, CAMERA_LANE_BIT_RATE_MBPS, CAMERA_MIPI_DSI_DPI_CLK_MHZ, LCD_COLOR_PIXEL_FORMAT_RGB565,
                      CONFIG_EXAMPLE_CAM_BUF_COUNT, CAMERA_WIDTH, CAMERA_HEIGHT, 0, 0, 0, 0, 0, 0, CAMERA_BITS_PER_PIXEL_RGB565, &mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

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

    assert = app_video_main(SGM38121_IIC_Bus->_iic_bus_handle);
    if (assert != ESP_OK)
    {
        printf("video_init fail (error code: %#X)\n", assert);
        return false;
    }

    video_cam_fd0 = app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }

#if CONFIG_EXAMPLE_CAM_BUF_COUNT == 2
    assert = esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 2, &lcd_buffer[0], &lcd_buffer[1]);
#else
    assert = esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 3, &lcd_buffer[0], &lcd_buffer[1], &lcd_buffer[2]);
#endif
    if (assert != ESP_OK)
    {
        printf("esp_lcd_dpi_panel_get_frame_buffer fail (error code: %#X)\n", assert);
        return false;
    }

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

    assert = app_video_set_bufs(video_cam_fd0, CONFIG_EXAMPLE_CAM_BUF_COUNT, (const void **)lcd_buffer);
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

    assert = app_video_stream_task_start(video_cam_fd0, 0, NULL);
    if (assert != ESP_OK)
    {
        printf("app_video_stream_task_start fail (error code: %#X)\n", assert);
        return false;
    }

    app_video_stream_task_stop(video_cam_fd0);

    // // Get the initial time for frame rate statistics
    // start_time = esp_timer_get_time();

    return true;
}

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

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Hardware_Usb_Cdc_Init();

    XL9535->begin(500000);
    XL9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    // 开关3.3v电压时候必须先将GPS断电
    XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    // 开关3.3v电压时候必须先将ESP32C6断电
    XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));

    XL9535->pin_mode(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Ethernet_Init();

    // 这个必须放在以太网后面
    ESP32P4->create_pwm(HI8561_SCREEN_BL, ledc_channel_t::LEDC_CHANNEL_0, 2000);

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    SGM38121->begin();
#if defined CONFIG_CAMERA_TYPE_SC2336
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV2710
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3100);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV5645
    XL9535->pin_mode(XL9535_CAMERA_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_CAMERA_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH); // 打开摄像头
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 2800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

    // bsp_init_refresh_monitor_io();

    bsp_enable_dsi_phy_power();

    vTaskDelay(pdMS_TO_TICKS(100));

    if (App_Video_Init() == false)
    {
        printf("App_Video_Init fail\n");
        Camera_Init_Status = false;
    }
    else
    {
        Camera_Init_Status = true;
    }

    Screen_Init(&Screen_Mipi_Dpi_Panel);
    // Usb_Screen_Init(&Screen_Mipi_Dpi_Panel);

    esp_err_t assert = esp_lcd_panel_reset(Screen_Mipi_Dpi_Panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_reset fail (error code: %#X)\n", assert);
    }
    assert = esp_lcd_panel_init(Screen_Mipi_Dpi_Panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_init fail (error code: %#X)\n", assert);
    }

    XL9535->pin_mode(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    HI8561_T_IIC_Bus->_iic_bus_handle = XL9535_IIC_Bus->_iic_bus_handle;
    HI8561_T->begin();

    // SDMMC_HOST_SLOT_1必须要先于SDMMC_HOST_SLOT_0初始化
    ESP32C6_AT->begin();

    XL9535->pin_mode(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
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

    Lvgl_Init();
    Lvgl_Startup();
    xTaskCreate(lvgl_ui_task, "lvgl_ui_task", 100 * 1024, NULL, 1, NULL);

    // Start PWM backlight after LVGL refresh is complete
    ESP32P4->start_pwm_gradient_time(100, 500);

    PCF8563_IIC_Bus->_iic_bus_handle = XL9535_IIC_Bus->_iic_bus_handle;
    PCF8563->begin();

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(20);
    _lock_release(&lvgl_api_lock);

    // ESP32C6复位模式
    // XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Esp32c6_At_Init();

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(40);
    _lock_release(&lvgl_api_lock);

    BQ27220_IIC_Bus->_iic_bus_handle = XL9535_IIC_Bus->_iic_bus_handle;
    BQ27220->begin();

    // 设置的电池容量会在没有电池插入的时候自动还原为默认值
    BQ27220->set_design_capacity(2000);
    BQ27220->set_temperature_mode(Cpp_Bus_Driver::Bq27220xxxx::Temperature_Mode::EXTERNAL_NTC);
    BQ27220->set_sleep_current_threshold(50);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(50);
    _lock_release(&lvgl_api_lock);

    AW86224_IIC_Bus->_iic_bus_handle = SGM38121_IIC_Bus->_iic_bus_handle;
    AW86224->begin(500000);
    // printf("AW86224 input voltage: %.06f V\n", AW86224->get_input_voltage());

    // RAM播放
    AW86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(60);
    _lock_release(&lvgl_api_lock);

    ES8311_IIC_Bus->_iic_bus_handle = SGM38121_IIC_Bus->_iic_bus_handle;
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

    ICM20948_IIC_Bus->_iic_bus_handle = SGM38121_IIC_Bus->_iic_bus_handle;
    ICM20948_Init();

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(80);
    _lock_release(&lvgl_api_lock);

    // XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    L76K->begin();
    printf("get_baud_rate:%ld\n", L76K->get_baud_rate());
    L76K->set_baud_rate(Cpp_Bus_Driver::L76k::Baud_Rate::BR_115200_BPS);
    printf("set_baud_rate:%ld\n", L76K->get_baud_rate());
    L76K->set_update_frequency(Cpp_Bus_Driver::L76k::Update_Freq::FREQ_5HZ);
    L76K->clear_rx_buffer_data();
    L76K->sleep(true);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(90);
    _lock_release(&lvgl_api_lock);

    XL9535->pin_mode(XL9535_LORA_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    // LORA复位
    XL9535->pin_mode(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    XL9535->pin_mode(XL9535_LORA_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    if (SX1262->begin(10000000) == false)
    {
        System_Ui->_device_lora.init_flag = false;
        printf("SX1262 begin fail\n");
    }

    if (SX1262->config_lora_params(System_Ui->_device_lora.params.freq, System_Ui->_device_lora.params.bw, System_Ui->_device_lora.params.current_limit,
                                   System_Ui->_device_lora.params.power, System_Ui->_device_lora.params.sf, System_Ui->_device_lora.params.cr,
                                   System_Ui->_device_lora.params.crc_type, System_Ui->_device_lora.params.preamble_length, System_Ui->_device_lora.params.sync_word) == false)
    {
        printf("config_lora_params fail\n");
    }
    SX1262->clear_buffer();
    SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);
    SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(100);
    _lock_release(&lvgl_api_lock);

    _lock_acquire(&lvgl_api_lock);
    System_Ui->begin();
    _lock_release(&lvgl_api_lock);

    xTaskCreate(hardware_usb_cdc_task, "hardware_usb_cdc_task", 4 * 1024, NULL, 3, NULL);
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
    xTaskCreate(device_lora_task, "device_lora_task", 4 * 1024, NULL, 3, &Lora_Task_Handle);
    xTaskCreate(iis_transmission_data_stream_task, "iis_transmission_data_stream_task", 4 * 1024, NULL, 4, &Iis_Transmission_Data_Stream_Task);

    // 等待lvgl刷新完成
    while (lv_display_flush_is_last(lv_display_get_default()) == false)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    System_Ui->set_vibration();

    // while (1)
    // {
    //     if (esp_log_timestamp() > Cycle_Time)
    //     {
    //         // Cpp_Bus_Driver::Hi8561_Touch::Touch_Point tp;

    //         // if (HI8561_T->get_multiple_touch_point(tp) == true)
    //         // {
    //         //     printf("touch finger: %d edge touch flag: %d\n", tp.finger_count, tp.edge_touch_flag);

    //         //     for (uint8_t i = 0; i < tp.info.size(); i++)
    //         //     {
    //         //         printf("touch num [%d] x: %d y: %d p: %d\n", i + 1, tp.info[i].x, tp.info[i].y, tp.info[i].pressure_value);
    //         //     }
    //         // }

    //         // lv_indev_data_t data;
    //         // if (lv_indev_get_read_cb(lv_indev_get_default()) != NULL)
    //         // {
    //         //     lv_indev_get_read_cb(lv_indev_get_default())(lv_indev_get_default(), &data);
    //         //     if (data.state == LV_INDEV_STATE_PR)
    //         //     {
    //         //         printf("touch x: %ld, y: %ld\n", data.point.x, data.point.y);
    //         //     }
    //         // }

    //         Cycle_Time = esp_log_timestamp() + 1000;
    //     }

    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }
}
