/*
 * @Description: lvgl_9_ui
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:34:16
 * @LastEditTime: 2026-04-07 18:04:16
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
#include "st25r3916_driver.h"
#include "radiolib_bridge_driver.h"
#include "lvgl_keyboard_config.h"
#endif

#include "lvgl.h"
#include "lvgl_ui.h"

#include "Icm20948_WE.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "ethernet_init.h"

#include "tinyusb.h"
#include "tusb_cdc_acm.h"

#include "app_video.h"
#include "driver/ppa.h"
#include "esp_private/esp_cache_private.h"
#include "esp_video_init.h"

#include "New Notification 010_c2_b16_s44100.h"

#include "esp_hosted.h"
#include "esp_wifi_remote.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include <fstream>

#include "esp_audio_dec.h"
#include "esp_audio_dec_default.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

#define SD_FILE_PATH_MUSIC "/sdcard/t_display_p4_lvgl_9_ui_resource/music/Nocturne, Op.9 No.2 in E-flat major-Aya Higuchi (piano).mp3"

#define LVGL_TICK_PERIOD_MS 1

#define AUDIO_MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BITS_PER_SAMPLE 16
#define AUDIO_NUM_CHANNEL 2

#define PREPEND_STRING "esp32p4 hardware usb cdc receive: "
#define PREPEND_LENGTH 34

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#define WIFI_SSID "xinyuandianzi"
#define WIFI_PASSWORD "AA15994823428"

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

enum class Rtc_Mode
{
    TEST = 0,
    GET_TIME,
};

enum class Esp32c6_Mode_List
{
    TEST = 0,
};

enum class Music_File_Read_Speed_Enum
{
    LOW_SPEED,
    HIGH_SPEED,
};

struct Ethernet_Info
{
    bool link_up_flag = false;

    struct
    {
        std::string data = "status: null\n";

        bool update_flag = false;
    } status;

    struct
    {
        std::string data;

        bool update_flag = false;
    } connect_ip_status;
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
    } sx1262;

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
#endif

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
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
        bool wifi_connect_status = false;
    } esp32c6;
};

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
TaskHandle_t Esp32c6_Task_Handle = NULL;
TaskHandle_t Sleep_Task_Handle = NULL;
TaskHandle_t Rf_Task_Handle = NULL;

uint8_t Aw86224_Vibration_Play_Count = 0;

Es8311_Mode Es8311_Speaker_Mode = Es8311_Mode::TEST;
Es8311_Mode Es8311_Microphone_Mode = Es8311_Mode::TEST;

bool Music_Play_End_Flag = false;
bool Set_Music_Current_Time_S_Flag = false;
double Set_Music_Current_Time_S = 0;
std::vector<uint8_t> Iis_Transmission_Data_Stream;
size_t Iis_Read_Data_Size_Index = 0;
std::ifstream Music_File;
Music_File_Read_Speed_Enum Music_File_Read_Speed = Music_File_Read_Speed_Enum::HIGH_SPEED;

Imu_Mode Icm20948_Imu_Mode = Imu_Mode::TEST;

Gps_Mode L76k_Gps_Mode = Gps_Mode::TEST;

bool L76k_Gps_Positioning_Flag = false;
size_t L76k_Gps_Positioning_Time = 0;

Ethernet_Mode Ip101gri_Ethernet_Mode = Ethernet_Mode::TEST;

Esp32c6_Mode_List Esp32c6_Mode = Esp32c6_Mode_List::TEST;

ppa_client_handle_t ppa_srm_handle = NULL;
size_t data_cache_line_size = 0;
int32_t video_cam_fd0;

ppa_client_handle_t ppa_srm_handle_2 = NULL;
size_t data_cache_line_size_2 = 0;

int32_t fps_count;
int64_t start_time;

bool Rf_Send_Flag = false;
uint8_t Rf_Send_Package[255] = {0};

bool Device_Rf_Task_Stop_Flag = false;

QueueHandle_t app_queue;

size_t Camera_Refresh_Flag = true;

// IIC 1
auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Bq27220_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ27220_SDA, BQ27220_SCL, I2C_NUM_0);
auto Pcf8563_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(PCF8563_SDA, PCF8563_SCL, I2C_NUM_0);

// IIC 2
auto Sgm38121_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);
auto Aw86224_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);
auto Es8311_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);

// IIS
auto Es8311_Iis_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK,
                                                                     i2s_port_t::I2S_NUM_0, Cpp_Bus_Driver::Hardware_Iis::Data_Mode::INPUT_OUTPUT, Cpp_Bus_Driver::Hardware_Iis::Iis_Mode::STD,
                                                                     i2s_clock_src_t::I2S_CLK_SRC_APLL);

// UART
auto L76k_Uart_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Uart>(GPS_RX, GPS_TX, UART_NUM_1);

// SPI
auto Sx1262_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST, 0);

auto Screen_Mipi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Mipi>(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_MIPI_DSI_HSYNC, SCREEN_MIPI_DSI_HBP, SCREEN_MIPI_DSI_HFP,
                                                                       SCREEN_MIPI_DSI_VSYNC, SCREEN_MIPI_DSI_VBP, SCREEN_MIPI_DSI_VFP, SCREEN_DATA_LANE_NUM,
                                                                       [](uint8_t format) -> Cpp_Bus_Driver::Hardware_Mipi::Color_Format
                                                                       {
                                                                    switch (format)
                                                                    {
                                                                    case 16:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB565;
                                                                    case 24:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB888;
                                                                    default:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB565;
                                                                    } }(SCREEN_BITS_PER_PIXEL));

// IIC 1
auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Bq27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(Bq27220_Iic_Bus, BQ27220_IIC_ADDRESS);
auto Pcf8563 = std::make_unique<Cpp_Bus_Driver::Pcf8563x>(Pcf8563_Iic_Bus, PCF8563_IIC_ADDRESS);

// IIC 2
auto Sgm38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(Sgm38121_Iic_Bus, SGM38121_IIC_ADDRESS);
auto Aw86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(Aw86224_Iic_Bus, AW86224_IIC_ADDRESS);
auto Es8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(Es8311_Iic_Bus, Es8311_Iis_Bus, ES8311_IIC_ADDRESS);
auto Icm20948 = std::make_unique<ICM20948_WE>(&Wire1, ICM20948_IIC_ADDRESS);

// UART
auto L76k = std::make_unique<Cpp_Bus_Driver::L76k>(L76k_Uart_Bus, [](bool Value) -> IRAM_ATTR bool
                                                   { return Xl9535->pin_write(XL9535_GPS_WAKE_UP, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(Value)); });

// SPI
auto Sx1262 = std::make_unique<Cpp_Bus_Driver::Sx126x>(Sx1262_Spi_Bus, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262, SX1262_BUSY,
                                                       SX1262_CS);

#if SCREEN_ROTATION_DIRECTION == 0
auto System_Ui = std::make_unique<Lvgl_Ui::System>(SCREEN_WIDTH, SCREEN_HEIGHT);
#elif SCREEN_ROTATION_DIRECTION == 90
auto System_Ui = std::make_unique<Lvgl_Ui::System>(SCREEN_HEIGHT, SCREEN_WIDTH);
#else
#error "no macro definition is set"
#endif

#if defined CONFIG_SCREEN_TYPE_HI8561
auto Hi8561_Iic_Touch_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(HI8561_TOUCH_SDA, HI8561_TOUCH_SCL, I2C_NUM_0);

auto Hi8561_Touch = std::make_unique<Cpp_Bus_Driver::Hi8561_Touch>(Hi8561_Iic_Touch_Bus, HI8561_TOUCH_IIC_ADDRESS);

auto Screen = std::make_unique<Cpp_Bus_Driver::Hi8561>(Screen_Mipi_Bus);

#elif defined CONFIG_SCREEN_TYPE_RM69A10

auto Gt9895_Touch_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(GT9895_SDA, GT9895_SCL, I2C_NUM_0);

auto Gt9895 = std::make_unique<Cpp_Bus_Driver::Gt9895>(Gt9895_Touch_Iic_Bus, GT9895_IIC_ADDRESS, -1, GT9895_X_SCALE_FACTOR, GT9895_Y_SCALE_FACTOR);

auto Screen = std::make_unique<Cpp_Bus_Driver::Rm69a10>(Screen_Mipi_Bus);
#else
#error "no macro definition is set"
#endif

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
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

volatile bool Tca8418_Interrupt_Flag = false;
volatile bool Cc1101_Interrupt_Flag = false;
volatile bool Nrf24l01_Interrupt_Flag = false;

bool Device_Nfc_Task_Stop_Flag = false;

TaskHandle_t Nfc_Task_Handle = NULL;

Nfc_Mode St25r3916_Nfc_Mode = Nfc_Mode::TEST;

//  Software IIC
auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(XL9555_SDA, XL9555_SCL);
auto Tca8418_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(TCA8418_SDA, TCA8418_SCL);

// SPI
auto Cc1101_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_CC1101_MOSI, T_MIXRF_CC1101_SCLK, T_MIXRF_CC1101_MISO, SPI2_HOST, 0);
auto Nrf24l01_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_NRF24L01_MOSI, T_MIXRF_NRF24L01_SCLK, T_MIXRF_NRF24L01_MISO, SPI2_HOST, 0);
RadioLibHal *Cc1101_Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(Cc1101_SPI_Bus, 10000000, T_MIXRF_CC1101_CS);
RadioLibHal *Nrf24l01_Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(Nrf24l01_SPI_Bus, 10000000, T_MIXRF_NRF24L01_CS);

//  Software IIC
auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);
auto Tca8418 = std::make_unique<Cpp_Bus_Driver::Tca8418>(Tca8418_Iic_Bus, TCA8418_IIC_ADDRESS);

// SPI
CC1101 Cc1101 = new Module(Cc1101_Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC),
                           static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), T_MIXRF_CC1101_BUSY);
nRF24 Nrf24l01 = new Module(Nrf24l01_Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC),
                            static_cast<uint32_t>(T_MIXRF_NRF24L01_INT), static_cast<uint32_t>(T_MIXRF_NRF24L01_CE), static_cast<uint32_t>(RADIOLIB_NC));

#endif

typedef struct
{
    uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + PREPEND_LENGTH + 1]; // Data buffer
    size_t buf_len;                                                  // Number of bytes received
    uint8_t itf;                                                     // Index of CDC device interface
} app_message_t;

uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

void Save_Real_Time(Real_Time time)
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

    Pcf8563->set_time(t);

    System_Ui->_time.week = time.week;
    System_Ui->_time.year = time.year;
    System_Ui->_time.month = time.month;
    System_Ui->_time.day = time.day;
    System_Ui->_time.hour = static_cast<uint8_t>((time.hour + 8 + 24) % 24);
    System_Ui->_time.minute = time.minute;
    System_Ui->_time.second = time.second;
    System_Ui->_time.time_zone = time.time_zone;
}

uint32_t synchsafe_to_uint32(const uint8_t *buf)
{
    return ((buf[0] & 0x7F) << 21) | ((buf[1] & 0x7F) << 14) | ((buf[2] & 0x7F) << 7) | (buf[3] & 0x7F);
}

uint32_t bigendian_to_uint32(const uint8_t *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

size_t parse_and_print_id3v2(FILE *f)
{
    uint8_t header[10];
    if (fread(header, 1, 10, f) != 10)
    {
        printf("Failed to read ID3v2 header\n");
        rewind(f);
        return 0;
    }

    if (memcmp(header, "ID3", 3) != 0)
    {
        printf("No ID3v2 tag found\n");
        rewind(f);
        return 0;
    }

    uint8_t major_version = header[3];
    uint8_t revision = header[4];
    uint8_t flags = header[5];
    uint32_t tag_size = synchsafe_to_uint32(header + 6);

    printf("ID3v2.%d.%d tag detected, flags: 0x%02X, size: %lu bytes\n", major_version, revision, flags, tag_size);

    bool has_extended_header = (flags & 0x40) != 0;

    size_t pos = 10;
    uint32_t extended_size = 0;

    if (has_extended_header)
    {
        uint8_t ext_header_size_buf[4];
        if (fread(ext_header_size_buf, 1, 4, f) != 4)
        {
            printf("Failed to read extended header size\n");
            rewind(f);
            return 0;
        }
        if (major_version == 4)
        {
            extended_size = synchsafe_to_uint32(ext_header_size_buf);
        }
        else
        {
            extended_size = bigendian_to_uint32(ext_header_size_buf);
        }
        // 跳过扩展头的剩余部分
        fseek(f, extended_size - 4, SEEK_CUR);
        pos += extended_size;
    }

    // 解析帧，直到达到标签大小或填充区域
    while (pos < 10 + tag_size)
    {
        char frame_id[5] = {0};
        if (fread(frame_id, 1, 4, f) != 4)
            break;
        pos += 4;

        if (frame_id[0] == 0)
            break; // 已到达填充区域

        uint8_t frame_size_buf[4];
        if (fread(frame_size_buf, 1, 4, f) != 4)
            break;
        pos += 4;

        uint32_t frame_size;
        if (major_version == 4)
        {
            frame_size = synchsafe_to_uint32(frame_size_buf);
        }
        else
        {
            frame_size = bigendian_to_uint32(frame_size_buf);
        }

        uint8_t frame_flags[2];
        if (fread(frame_flags, 1, 2, f) != 2)
            break;
        pos += 2;

        auto frame_data = std::make_unique<uint8_t[]>(frame_size);
        if (fread(frame_data.get(), 1, frame_size, f) != frame_size)
            break;
        pos += frame_size;

        // 打印帧信息
        printf("Frame %s (size %lu, flags 0x%02X%02X): ", frame_id, frame_size, frame_flags[0], frame_flags[1]);

        if (frame_size < 1)
        {
            printf("Empty frame\n");
            continue;
        }

        uint8_t encoding = frame_data[0];
        const uint8_t *text_start = frame_data.get() + 1;
        size_t text_len = frame_size - 1;

        // 通过查找空终止符来确定实际字符串长度
        size_t actual_text_len = text_len;
        if (encoding == 0 || encoding == 3)
        { // ISO-8859-1 或 UTF-8：单个空字符
            for (size_t i = 0; i < text_len; ++i)
            {
                if (text_start[i] == 0)
                {
                    actual_text_len = i;
                    break;
                }
            }
        }
        else if (encoding == 1 || encoding == 2)
        { // UTF-16 变体：双空字符
            for (size_t i = 0; i + 1 < text_len; i += 2)
            {
                if (text_start[i] == 0 && text_start[i + 1] == 0)
                {
                    actual_text_len = i;
                    break;
                }
            }
        }

        if (frame_id[0] == 'T' && strcmp(frame_id, "TXXX") != 0)
        { // 标准文本帧（TIT2, TPE1等）
            printf("Text (%s): ", encoding == 0 ? "ISO-8859-1" : encoding == 1 ? "UTF-16 (with BOM)"
                                                             : encoding == 2   ? "UTF-16BE"
                                                             : encoding == 3   ? "UTF-8"
                                                                               : "Unknown");

            if (encoding == 0)
            {
                // ISO-8859-1： 直接作为char*打印
                printf("%.*s\n", (int)actual_text_len, text_start);
            }
            else if (encoding == 3)
            {
                // UTF-8： 在大多数控制台中可以直接打印
                printf("%.*s\n", (int)actual_text_len, text_start);
            }
            else if (encoding == 1 || encoding == 2)
            {
                // UTF-16： 为简化，可以打印十六进制或跳过完整解码（C++ std::wstring_convert较重）
                // 这里只显示前几个字符的十六进制，避免在嵌入式代码中进行复杂转换
                printf("(UTF-16 data, first 20 bytes hex): ");
                for (size_t i = 0; i < std::min(actual_text_len, (size_t)20); ++i)
                {
                    printf("%02X ", text_start[i]);
                }
                printf("...\n");
                // 如果确实需要可读的UTF-16文本，需要UTF-16转UTF-8转换器（例如iconv或手动实现）
                // 但在ESP32上通常只支持0和3编码更简单
            }
            else
            {
                printf("Unsupported encoding %d\n", encoding);
            }
        }
        else if (strcmp(frame_id, "APIC") == 0)
        {
            printf("Attached picture (not printing binary data)\n");
        }
        else
        {
            // 其他帧：显示原始十六进制预览
            printf("Raw data (hex first 16 bytes): ");
            for (size_t i = 0; i < std::min((size_t)16, text_len + 1); ++i)
            { // +1 包含编码字节
                printf("%02X ", frame_data[i]);
            }
            if (frame_size > 16)
                printf("...\n");
            else
                printf("\n");
        }
    }

    // 返回MP3数据的起始偏移量
    return 10 + tag_size;
}

float get_mp3_duration_from_vbr_header(FILE *f, size_t mp3_offset)
{
    fseek(f, mp3_offset, SEEK_SET);

    uint8_t buf[576 + 256]; // 足够涵盖Xing/VBRI 位置
    size_t read_len = fread(buf, 1, sizeof(buf), f);
    if (read_len < 100)
    {
        return -1.0f;
    }

    // 找 "Xing" 或 "Info" (LAME 常用 Info 代替 Xing)
    for (size_t i = 0; i < read_len - 4; ++i)
    {
        if (memcmp(buf + i, "Xing", 4) == 0 || memcmp(buf + i, "Info", 4) == 0)
        {
            size_t pos = i + 4;

            uint32_t flags = (buf[pos] << 24) | (buf[pos + 1] << 16) | (buf[pos + 2] << 8) | buf[pos + 3];
            pos += 4;

            if (flags & 0x00000001) // Frames field present
            {
                uint32_t frames = (buf[pos] << 24) | (buf[pos + 1] << 16) | (buf[pos + 2] << 8) | buf[pos + 3];
                pos += 4;

                // MPEG1 Layer3: 1152 samples/frame, MPEG2: 576
                // 假设 MPEG1 Layer3
                uint32_t samples_per_frame = 1152;

                // 如果能从前面 frame header 拿到 version/layer 更好
                uint32_t total_samples = frames * samples_per_frame;

                return (float)total_samples / AUDIO_SAMPLE_RATE;
            }
        }
        // VBRI (Fraunhofer 格式)
        else if (memcmp(buf + i, "VBRI", 4) == 0)
        {
            return -1.0f;
        }
    }

    return -1.0f; // 没找到 VBR header
}

bool Play_Mp3_File(const char *file_path)
{
    Music_File.open(file_path, std::ios::binary);
    if (!Music_File.is_open())
    {
        printf("failed to open mp3 file: %s\n", file_path);
        return false;
    }

    // ID3 标签解析与时长获取
    FILE *f_c = fopen(file_path, "rb");
    size_t mp3_offset = parse_and_print_id3v2(f_c);
    float duration = get_mp3_duration_from_vbr_header(f_c, mp3_offset);
    fclose(f_c);

    Music_File.seekg(mp3_offset, std::ios::beg);
    printf("mp3 start offset: %zu, duration: %.2f s\n", mp3_offset, duration);
    if (duration == -1.0f)
    {
        printf("duration error\n");
        Music_File.close();
        return false;
    }

    _lock_acquire(&lvgl_api_lock);
    System_Ui->set_win_music_current_total_time(0, duration);
    _lock_release(&lvgl_api_lock);

    esp_audio_dec_register_default();
    esp_audio_dec_cfg_t dec_cfg = {.type = ESP_AUDIO_TYPE_MP3};
    esp_audio_dec_handle_t decoder = NULL;
    if (esp_audio_dec_open(&dec_cfg, &decoder) != ESP_AUDIO_ERR_OK)
    {
        printf("esp_audio_dec_open fail\n");
        Music_File.close();
        return false;
    }

    Iis_Transmission_Data_Stream.clear();
    Iis_Read_Data_Size_Index = 0;
    uint64_t total_pcm_bytes_sent = 0;
    const uint32_t bytes_per_second = AUDIO_SAMPLE_RATE * AUDIO_NUM_CHANNEL * (AUDIO_BITS_PER_SAMPLE / 8);
    size_t cycle_time = 0;
    size_t cycle_time_2 = 0;

    // 解码输出的音频数据缓冲区大小必须比压缩的大
    auto pcm_buf = std::make_unique<uint8_t[]>(50 * 1024);

    Music_Play_End_Flag = false;

    while (Music_File.good())
    {
        if (Music_Play_End_Flag)
        {
            break;
        }

        if (Es8311_Speaker_Mode == Es8311_Mode::TEST)
        {
            // 播放音乐测试
            Es8311->write_data(c2_b16_s44100, sizeof(c2_b16_s44100));

            Es8311_Speaker_Mode = Es8311_Mode::PLAY_MUSIC;
        }

        // 读取频率限制
        if (esp_log_timestamp() > cycle_time_2)
        {
            if (Music_File.good())
            {
                const auto current_buf_size = Iis_Transmission_Data_Stream.size();
                if (current_buf_size < 1024 * 20)
                {
                    size_t read_request = 1024 * 10;
                    Iis_Transmission_Data_Stream.resize(current_buf_size + read_request);
                    Music_File.read((char *)(Iis_Transmission_Data_Stream.data() + current_buf_size), read_request);

                    std::streamsize actual_read = Music_File.gcount();
                    if (actual_read < static_cast<std::streamsize>(read_request))
                    {
                        Iis_Transmission_Data_Stream.erase(
                            Iis_Transmission_Data_Stream.end() - (read_request - actual_read),
                            Iis_Transmission_Data_Stream.end());
                    }
                }
            }
            cycle_time_2 = esp_log_timestamp() + 50;
        }

        // 清理已消费数据
        if (Iis_Read_Data_Size_Index > 1024 * 10)
        {
            Iis_Transmission_Data_Stream.erase(Iis_Transmission_Data_Stream.begin(),
                                               Iis_Transmission_Data_Stream.begin() + 1024 * 10);
            Iis_Read_Data_Size_Index -= 1024 * 10;
        }

        if (Set_Music_Current_Time_S_Flag == true)
        {
            printf("MP3 seek to: %.2f s (duration %.2f)\n", Set_Music_Current_Time_S, duration);

            // 清除文件流的 EOF/Fail 错误标志，否则 seekg 会直接失效导致死锁或没声音
            Music_File.clear();

            // 清空缓冲区
            Iis_Transmission_Data_Stream.clear();
            Iis_Read_Data_Size_Index = 0;

            // 计算跳转进度比例 (0.0 ~ 0.99)
            double safe_progress = Set_Music_Current_Time_S / duration;
            if (safe_progress < 0.0)
                safe_progress = 0.0;
            if (safe_progress > 0.99)
                safe_progress = 0.99; // 留 1% 尾巴防止跳到文件外

            // 获取文件总大小并计算粗略的字节偏移位置
            Music_File.seekg(0, std::ios::end);
            std::streamoff file_end = Music_File.tellg();
            std::streamoff mp3_data_bytes = file_end - static_cast<std::streamoff>(mp3_offset);

            // 粗略位置计算 (基于百分比)
            std::streamoff seek_pos = mp3_offset + static_cast<std::streamoff>(safe_progress * mp3_data_bytes);

            // 防御性钳制 (最小跳过ID3头，最大留出安全余量)
            const std::streamoff TAIL_RESERVE = 4096;
            if (seek_pos < mp3_offset)
                seek_pos = mp3_offset;
            if (seek_pos > file_end - TAIL_RESERVE)
                seek_pos = file_end - TAIL_RESERVE;

            // 执行跳转
            Music_File.clear();
            Music_File.seekg(seek_pos, std::ios::beg);
            printf("Seeking to file pos %lld (progress %.2f%%)\n", (long long)seek_pos, safe_progress * 100.0);

            // 重置解码器（丢弃旧的解码状态）
            esp_audio_dec_close(decoder);
            if (esp_audio_dec_open(&dec_cfg, &decoder) != ESP_AUDIO_ERR_OK)
            {
                printf("Decoder re-open failed after seek\n");
                Music_Play_End_Flag = true;
                break;
            }

            // 重新预载一段数据，供解码器去寻找帧同步头
            const size_t PRELOAD_SZ = 1024 * 40; // 预载 40KB
            Iis_Transmission_Data_Stream.resize(PRELOAD_SZ);
            Music_File.read(reinterpret_cast<char *>(Iis_Transmission_Data_Stream.data()), PRELOAD_SZ);
            std::streamsize actual = Music_File.gcount();

            if (actual > 0)
            {
                Iis_Transmission_Data_Stream.resize(static_cast<size_t>(actual));
            }
            else
            {
                Iis_Transmission_Data_Stream.clear();
            }

            // 强制重置当前时间基准，防止 UI 进度条反弹
            total_pcm_bytes_sent = static_cast<uint64_t>(Set_Music_Current_Time_S * bytes_per_second);

            // 收尾恢复
            Set_Music_Current_Time_S_Flag = false;
            vTaskDelay(pdMS_TO_TICKS(50));
            printf("Seek done\n");
        }

        if (System_Ui->_registry.win.music.play_flag == true)
        {
            // 安全计算 available_mp3_bytes，防止 size_t 下溢出
            size_t available_mp3_bytes = 0;
            if (Iis_Transmission_Data_Stream.size() > Iis_Read_Data_Size_Index)
            {
                available_mp3_bytes = Iis_Transmission_Data_Stream.size() - Iis_Read_Data_Size_Index;
            }

            if (Music_File.eof() && available_mp3_bytes == 0)
            {
                printf("MP3 file EOF reached, naturally ending playback.\n");
                Music_Play_End_Flag = true;
                break;
            }

            if (available_mp3_bytes > 0)
            {
                esp_audio_dec_in_raw_t in_raw = {
                    .buffer = Iis_Transmission_Data_Stream.data() + Iis_Read_Data_Size_Index,
                    .len = available_mp3_bytes};
                esp_audio_dec_out_frame_t out_frame = {
                    .buffer = pcm_buf.get(),
                    .len = 50 * 1024};

                esp_audio_err_t err = esp_audio_dec_process(decoder, &in_raw, &out_frame);

                if (err == ESP_AUDIO_ERR_OK)
                {
                    Iis_Read_Data_Size_Index += in_raw.consumed;
                    if (out_frame.decoded_size > 0)
                    {
                        Es8311->write_data(out_frame.buffer, out_frame.decoded_size);
                        total_pcm_bytes_sent += out_frame.decoded_size;
                    }

                    if (in_raw.consumed == 0)
                    {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
                else if (err == ESP_AUDIO_ERR_FAIL)
                {
                    // ESP解码器报错时，安全地跳过坏数据，防止越界
                    size_t skip_len = (available_mp3_bytes < 64) ? available_mp3_bytes : 64;
                    Iis_Read_Data_Size_Index += skip_len;
                }
                else
                {
                    // 其他错误，同样需安全跳过
                    size_t skip_len = (available_mp3_bytes < 128) ? available_mp3_bytes : 128;
                    Iis_Read_Data_Size_Index += skip_len;
                }
            }

            if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::MUSIC)
            {
                // UI 更新
                if (esp_log_timestamp() > cycle_time)
                {
                    double current_time = static_cast<double>(total_pcm_bytes_sent) / bytes_per_second;
                    _lock_acquire(&lvgl_api_lock);
                    System_Ui->set_win_music_current_total_time(current_time, duration);
                    _lock_release(&lvgl_api_lock);

                    printf("Playing MP3: %.2f / %.2f s\n", current_time, duration);
                    cycle_time = esp_log_timestamp() + 1000;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    esp_audio_dec_close(decoder);
    esp_audio_dec_unregister_default();
    Iis_Transmission_Data_Stream.clear();
    Music_File.close();

    System_Ui->_registry.win.music.play_flag = false;

    if (System_Ui->_current_win == Lvgl_Ui::System::Current_Win::MUSIC)
    {
        _lock_acquire(&lvgl_api_lock);
        System_Ui->set_win_music_play_imagebutton_status(System_Ui->_registry.win.music.play_flag);
        System_Ui->set_win_music_current_total_time(0, duration);
        _lock_release(&lvgl_api_lock);
    }

    printf("music play finish\n");
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

bool Http_Client_Get_Real_Time(Real_Time &rt)
{
    struct HttpTimeCtx
    {
        char date_str[64] = {0};
        bool date_found = false;
    } ctx;

    esp_http_client_config_t config = {};
    config.url = "http://httpbin.org/get";
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 10000;
    config.event_handler =
        [](esp_http_client_event_t *evt) -> esp_err_t
    {
        HttpTimeCtx *ctx_ptr = static_cast<HttpTimeCtx *>(evt->user_data);

        switch (evt->event_id)
        {
        case HTTP_EVENT_ON_HEADER:
            // printf("HEADER: key=[%s], value=[%s]\n", evt->header_key, evt->header_value);

            // 查找
            if (strcmp(evt->header_key, "Date") == 0)
            {
                size_t len = strlen(evt->header_value);
                if (len >= sizeof(ctx_ptr->date_str))
                {
                    len = sizeof(ctx_ptr->date_str) - 1;
                }
                memcpy(ctx_ptr->date_str, evt->header_value, len);
                ctx_ptr->date_str[len] = '\0';
                ctx_ptr->date_found = true;
                // printf("Found Date: %s\n", ctx_ptr->date_str);
            }
            break;

        default:
            break;
        }
        return ESP_OK;
    };

    config.user_data = &ctx;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr)
    {
        printf("esp_http_client_init fail\n");
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);

    bool success = false;

    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        if (status == 200 && ctx.date_found)
        {
            // 解析 Date 字符串
            const char *date_str = ctx.date_str;

            if (date_str && strlen(date_str) >= 20)
            {
                char weekday[4] = {0};
                char mon[4] = {0};
                int day, year, hour, min, sec;
                char zone[16] = {0};

                int n = sscanf(date_str, "%3s, %d %3s %d %d:%d:%d %15s",
                               weekday, &day, mon, &year, &hour, &min, &sec, zone);

                if (n == 8)
                {
                    rt.week = weekday;
                    rt.day = static_cast<uint8_t>(day);
                    rt.year = static_cast<uint16_t>(year);
                    rt.hour = static_cast<uint8_t>(hour);
                    rt.minute = static_cast<uint8_t>(min);
                    rt.second = static_cast<uint8_t>(sec);
                    rt.time_zone = zone;

                    // 月份转换
                    const char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
                    rt.month = 0;
                    for (int i = 0; i < 12; ++i)
                    {
                        if (strncasecmp(mon, months[i], 3) == 0)
                        {
                            rt.month = static_cast<uint8_t>(i + 1);
                            break;
                        }
                    }

                    if (rt.month >= 1 && rt.month <= 12)
                    {
                        success = true;

                        // 输出结果
                        printf("parsed time: %s %02d %02d %04d %02d:%02d:%02d %s\n",
                               rt.week.c_str(), rt.day, rt.month, rt.year,
                               rt.hour, rt.minute, rt.second, rt.time_zone.c_str());
                    }
                }
            }

            if (success == false)
            {
                printf("date parse failed: %s\n", date_str ? date_str : "(null)");
            }
        }
        else
        {
            printf("bad status or no date header: %d (found=%d)\n", status, ctx.date_found);
        }
    }
    else
    {
        printf("http perform failed: %d\n", err);
    }

    esp_http_client_cleanup(client);
    return success;
}

void device_vibration_task(void *arg)
{
    printf("device_vibration_task start\n");
    vTaskSuspend(Vibration_Task_Handle);

    while (1)
    {
        if (Aw86224_Vibration_Play_Count == static_cast<uint8_t>(-1)) // 开启F0校验
        {
            uint8_t timeout_count = 0;
            uint32_t f0_value = 0;
            bool f0_detection_result = false;

            // 等待F0校准
            while (1)
            {
                f0_value = Aw86224->get_f0_detection();
                printf("Aw86224 get f0 detection value: %ld\n", f0_value);

                if (Aw86224->set_f0_calibrate(f0_value) == true)
                {
                    f0_detection_result = true;
                    break;
                }
                else
                {
                    // 阈值限定
                    if (f0_value > Aw86224->_f0_value)
                    {
                        if ((f0_value - Aw86224->_f0_value) <= 1500)
                        {
                            f0_detection_result = true;
                            Aw86224->_f0_value = f0_value;
                            break;
                        }
                    }
                    else
                    {
                        if ((Aw86224->_f0_value - f0_value) <= 1500)
                        {
                            f0_detection_result = true;
                            Aw86224->_f0_value = f0_value;
                            break;
                        }
                    }
                }

                timeout_count++;
                if (timeout_count > 5)
                {
                    printf("Aw86224 get f0 detection fail\n");
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

            Aw86224_Vibration_Play_Count = 0;
        }
        else if (Aw86224_Vibration_Play_Count > 0)
        {
            // 启动振动
            Aw86224->run_ram_playback_waveform(1, 15, 255);
            vTaskDelay(pdMS_TO_TICKS(50));
            Aw86224->stop_ram_playback_waveform();

            Aw86224_Vibration_Play_Count--;
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
        switch (Es8311_Speaker_Mode)
        {
        case Es8311_Mode::TEST:
            // 播放音乐测试
            Es8311->write_data(c2_b16_s44100, sizeof(c2_b16_s44100));
            break;
        case Es8311_Mode::PLAY_MUSIC:
            // 播放音乐

            Play_Mp3_File(SD_FILE_PATH_MUSIC);
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

        switch (Es8311_Microphone_Mode)
        {
        case Es8311_Mode::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                // 读取麦克风数据
                int16_t microphone_data[1] = {0};
                Es8311->read_data(microphone_data, 1 * sizeof(int16_t));

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
        switch (Icm20948_Imu_Mode)
        {
        case Imu_Mode::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                // 读取IMU数据
                Icm20948->readSensor();
                xyzFloat gValue;
                Icm20948->getGValues(&gValue);
                xyzFloat angle;
                Icm20948->getAngles(&angle);
                float pitch = Icm20948->getPitch();
                float roll = Icm20948->getRoll();

                // 获取磁力计的 x, y 值以计算航向角（Yaw）
                xyzFloat magValues;
                Icm20948->getMagValues(&magValues);
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

            uint16_t battery_level = Bq27220->get_status_of_charge();

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

                battery_health_data_str += "Bq27220 data:\n";
                battery_health_data_str += "device id: " + std::to_string(Bq27220->get_device_id()) + "\n\n";

                battery_health_data_str += "design capacity: " + std::to_string(Bq27220->get_design_capacity()) + " mah\n";
                battery_health_data_str += "remaining capacity: " + std::to_string(Bq27220->get_remaining_capacity()) + " mah\n";
                battery_health_data_str += "full charge capacity: " + std::to_string(Bq27220->get_full_charge_capacity()) + " mah\n\n";

                // battery_health_data_str += "raw coulomb count: " + std::to_string(Bq27220->get_raw_coulomb_count()) + " c\n";
                // battery_health_data_str += "cycle count: " + std::to_string(Bq27220->get_cycle_count()) + "\n\n";

                battery_health_data_str += "battery level: " + std::to_string(battery_level) + "%\n";
                battery_health_data_str += "battery health: " + std::to_string(Bq27220->get_status_of_charge()) + "%\n\n";

                battery_health_data_str += "voltage: " + std::to_string(Bq27220->get_voltage()) + " mv\n";
                battery_health_data_str += "current: " + std::to_string(Bq27220->get_current()) + " ma\n";
                // battery_health_data_str += "charging voltage: " + std::to_string(Bq27220->get_charging_voltage()) + " mv\n";
                // battery_health_data_str += "charging current: " + std::to_string(Bq27220->get_charging_current()) + " ma\n";
                // battery_health_data_str += "standby current: " + std::to_string(Bq27220->get_standby_current()) + " ma\n";
                // battery_health_data_str += "max load current: " + std::to_string(Bq27220->get_max_load_current()) + " ma\n";
                // battery_health_data_str += "average power: " + std::to_string(Bq27220->get_average_power()) + " mw\n\n";

                battery_health_data_str += "chip temperature: " + std::to_string(Bq27220->get_chip_temperature_celsius()) + " °c\n\n";
                // battery_health_data_str += "ntc temperature: " + std::to_string(Bq27220->get_temperature_celsius()) + " °c\n\n";

                // battery_health_data_str += "at rate: " + std::to_string(Bq27220->get_at_rate()) + " ma\n";
                // battery_health_data_str += "at rate battery time to empty: " + std::to_string(Bq27220->get_at_rate_time_to_empty()) + " min\n";
                // battery_health_data_str += "battery time to empty: " + std::to_string(Bq27220->get_time_to_empty()) + " min\n";
                // battery_health_data_str += "battery time to full charge: " + std::to_string(Bq27220->get_time_to_full()) + " min\n";
                // battery_health_data_str += "battery standby time to empty: " + std::to_string(Bq27220->get_standby_time_to_empty()) + " min\n";
                // battery_health_data_str += "battery max load time to empty: " + std::to_string(Bq27220->get_max_load_time_to_empty()) + " min\n\n";

                Cpp_Bus_Driver::Bq27220xxxx::Battery_Status bs;
                if (Bq27220->get_battery_status(bs) == true)
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

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0

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

                float ts_percentage = 0.0;
                Kode_Bq25896::bq25896_get_ts_voltage_percentage(Bq25896_Handle, &ts_percentage);

                battery_health_data_str += "ts voltage: " + std::to_string(ts_percentage) + "%% of regn\n";

                Kode_Bq25896::bq25896_ntc_fault_t ntc_fault;

                Kode_Bq25896::bq25896_get_ntc_fault(Bq25896_Handle, &ntc_fault);

                std::string ntc_fault_str;
                switch (ntc_fault)
                {
                case Kode_Bq25896::BQ25896_NTC_FAULT_NORMAL:
                    ntc_fault_str = "normal";
                    break;
                case Kode_Bq25896::BQ25896_NTC_FAULT_TS_WARM:
                    ntc_fault_str = "ts warm";
                    break;
                case Kode_Bq25896::BQ25896_NTC_FAULT_TS_COOL:
                    ntc_fault_str = "ts cool";
                    break;
                case Kode_Bq25896::BQ25896_NTC_FAULT_TS_COLD:
                    ntc_fault_str = "ts cold";
                    break;
                case Kode_Bq25896::BQ25896_NTC_FAULT_TS_HOT:
                    ntc_fault_str = "ts hot";
                    break;
                default:
                    ntc_fault_str = "unknown";
                    break;
                }

                battery_health_data_str += "ntc fault: " + ntc_fault_str + "\n\n";

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
#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
                if ((vbus_stat == Kode_Bq25896::BQ25896_VBUS_STAT_ADAPTER) || (vbus_stat == Kode_Bq25896::BQ25896_VBUS_STAT_USB_HOST))
                {
                    lv_obj_add_flag(System_Ui->_registry.win.cit.battery_health_test.otg_label, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(System_Ui->_registry.win.cit.battery_health_test.otg_switch, LV_OBJ_FLAG_HIDDEN);

                    lv_obj_remove_flag(System_Ui->_registry.win.cit.battery_health_test.hcc_label, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(System_Ui->_registry.win.cit.battery_health_test.hcc_switch, LV_OBJ_FLAG_HIDDEN);

                    lv_obj_align_to(System_Ui->_registry.win.cit.battery_health_test.hcc_label,
                                    System_Ui->_registry.win.cit.battery_health_test.data_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
                    lv_obj_align_to(System_Ui->_registry.win.cit.battery_health_test.hcc_switch,
                                    System_Ui->_registry.win.cit.battery_health_test.hcc_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
                }
                else
                {
                    lv_obj_remove_flag(System_Ui->_registry.win.cit.battery_health_test.otg_label, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(System_Ui->_registry.win.cit.battery_health_test.otg_switch, LV_OBJ_FLAG_HIDDEN);

                    lv_obj_align_to(System_Ui->_registry.win.cit.battery_health_test.otg_label,
                                    System_Ui->_registry.win.cit.battery_health_test.data_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
                    lv_obj_align_to(System_Ui->_registry.win.cit.battery_health_test.otg_switch,
                                    System_Ui->_registry.win.cit.battery_health_test.otg_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

                    lv_obj_add_flag(System_Ui->_registry.win.cit.battery_health_test.hcc_label, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(System_Ui->_registry.win.cit.battery_health_test.hcc_switch, LV_OBJ_FLAG_HIDDEN);
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

                if (L76k->get_info_data(buffer, &buffer_length) == true)
                {
                    // 打印RMC的相关信息
                    Cpp_Bus_Driver::L76k::Rmc rmc;

                    if (L76k->parse_rmc_info(buffer.get(), buffer_length, rmc) == true)
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
            if (Pcf8563->get_time(t) == true)
            {
                printf("Pcf8563 year:[%d] month:[%d] day:[%d] time:[%d:%d:%d] week:[%d]\n", t.year, t.month, t.day,
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
                printf("Pcf8563 integrity of the clock information is not guaranteed\n");

                if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CIT_RTC_TEST)
                {
                    std::string rtc_data_str = "rtc data:\nPcf8563 integrity of the clock\ninformation is not guaranteed\n";

                    _lock_acquire(&lvgl_api_lock);
                    // 更新数据的标签
                    lv_label_set_text(System_Ui->_registry.win.cit.rtc_test.data_label, rtc_data_str.c_str());
                    _lock_release(&lvgl_api_lock);
                }

                Pcf8563->clear_clock_integrity_flag();
            }

            cycle_time = esp_log_timestamp() + 1000;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void device_esp32c6_task(void *arg)
{
    printf("device_esp32c6_task start\n");
    vTaskSuspend(Esp32c6_Task_Handle);

    size_t cycle_time = 0;

    while (1)
    {
        switch (Esp32c6_Mode)
        {
        case Esp32c6_Mode_List::TEST:
        {
            if (esp_log_timestamp() > cycle_time)
            {
                if (Sys_Status.esp32c6.wifi_connect_status == false)
                {
                    printf("esp32c6 wifi connect fail\n");
                    std::string data_str = "esp32c6 wifi connect fail,\nattempting to reconnect";

                    _lock_acquire(&lvgl_api_lock);
                    // 更新数据的标签
                    lv_label_set_text(System_Ui->_registry.win.cit.esp32c6_test.data_label, data_str.c_str());
                    _lock_release(&lvgl_api_lock);
                }
                else
                {
                    Real_Time rt;
                    if (Http_Client_Get_Real_Time(rt) == true)
                    {
                        printf("get_real_time success\n");
                        printf("week: [%s] day: [%d] month: [%d] year: [%d] time: [%02d:%02d:%02d] time zone: [%s] china time: [%02d:%02d:%02d]\n",
                               rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute, rt.second, rt.time_zone.c_str(),
                               (rt.hour + 8 + 24) % 24, rt.minute, rt.second);

                        // 读取At数据
                        std::string data_str = "esp32c6 wifi time data:\n";
                        char buffer[200];
                        snprintf(buffer, sizeof(buffer),
                                 "week: [%s]\ndata: [%d/%d/%d]\nchina time: [%02d:%02d:%02d]\n",
                                 rt.week.c_str(), rt.year, rt.month, rt.day,
                                 (rt.hour + 8 + 24) % 24, rt.minute, rt.second);
                        data_str += buffer;

                        _lock_acquire(&lvgl_api_lock);
                        // 更新数据的标签
                        lv_label_set_text(System_Ui->_registry.win.cit.esp32c6_test.data_label, data_str.c_str());
                        _lock_release(&lvgl_api_lock);

                        Save_Real_Time(rt);
                        _lock_acquire(&lvgl_api_lock);
                        System_Ui->status_bar_wifi_connect_status_update();
                        _lock_release(&lvgl_api_lock);
                    }
                    else
                    {
                        printf("Http_Client_Get_Real_Time fail\n");

                        std::string data_str = "esp32c6 time data:\nHttp_Client_Get_Real_Time fail\n";

                        _lock_acquire(&lvgl_api_lock);
                        // 更新数据的标签
                        lv_label_set_text(System_Ui->_registry.win.cit.esp32c6_test.data_label, data_str.c_str());
                        _lock_release(&lvgl_api_lock);
                    }
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

void device_rf_task(void *arg)
{
    printf("device_rf_task start\n");

    size_t cycle_time = 0;
    size_t auto_send_cycle_time = 0;

    while (1)
    {
        switch (System_Ui->_rf_chip_type)
        {
        case Lvgl_Ui::System::Rf_Chip_Type::SX1262:
        {
            // if (esp_log_timestamp() > cycle_time)
            // {
            //     printf("sx1262 ID: %#X\n", Sx1262->get_device_id());

            //     printf("sx1262 get current limit: %d\n", Sx1262->get_current_limit());

            //     switch (Sx1262->get_packet_type())
            //     {
            //     case Cpp_Bus_Driver::Sx126x::Packet_Type::GFSK:
            //         printf("sx1262 packet type: GFSK\n");
            //         break;
            //     case Cpp_Bus_Driver::Sx126x::Packet_Type::LORA:
            //         printf("sx1262 packet type: LORA\n");
            //         break;
            //     case Cpp_Bus_Driver::Sx126x::Packet_Type::LR_FHSS:
            //         printf("sx1262 packet type: LR_FHSS\n");
            //         break;

            //     default:
            //         break;
            //     }

            //     switch (Sx1262->parse_chip_mode_status(Sx1262->get_status()))
            //     {
            //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::STBY_RC:
            //         printf("sx1262 chip mode status: STBY_RC\n");
            //         break;
            //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::STBY_XOSC:
            //         printf("sx1262 chip mode status: STBY_XOSC\n");
            //         break;
            //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::FS:
            //         printf("sx1262 chip mode status: FS\n");
            //         break;
            //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::RX:
            //         printf("sx1262 chip mode status: RX\n");
            //         break;
            //     case Cpp_Bus_Driver::Sx126x::Chip_Mode_Status::TX:
            //         printf("sx1262 chip mode status: TX\n");
            //         break;

            //     default:
            //         break;
            //     }

            //     cycle_time = esp_log_timestamp() + 1000;
            // }

            if (System_Ui->_device_sx1262.auto_send.flag == true)
            {
                if (Rf_Send_Flag == false)
                {
                    if (esp_log_timestamp() > auto_send_cycle_time)
                    {
                        memset(Rf_Send_Package, '\0', sizeof(Rf_Send_Package));

                        // 检查长度是否越界
                        if (System_Ui->_device_sx1262.auto_send.text.size() <= 255)
                        {
                            memcpy(Rf_Send_Package, System_Ui->_device_sx1262.auto_send.text.data(), System_Ui->_device_sx1262.auto_send.text.size());
                        }
                        else
                        {
                            // 处理错误：数据过长
                            memcpy(Rf_Send_Package, System_Ui->_device_sx1262.auto_send.text.data(), 254);
                            Rf_Send_Package[254] = '\0';

                            printf("sx1262 send out of bounds(data > Rf_Send_Package)\n");
                        }

                        char buffer_time[15];
                        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", System_Ui->_time.hour, System_Ui->_time.minute, System_Ui->_time.second);

                        Lvgl_Ui::System::Win_Rf_Chat_Message wlcm =
                            {
                                .direction = Lvgl_Ui::System::Chat_Message_Direction::SEND,
                                .time = buffer_time,
                                .data = System_Ui->_device_sx1262.auto_send.text,
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

                        auto_send_cycle_time = esp_log_timestamp() + System_Ui->_device_sx1262.auto_send.interval;
                    }
                }
            }

            if (Rf_Send_Flag == true)
            {
                // 设置发送模式，发送完成后进入快速切换模式（FS模式）
                Sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::TX, 0, Cpp_Bus_Driver::Sx126x::Fallback_Mode::FS);
                Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE);
                Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE);

                printf("sx1262 send start\n");
                printf("sx1262 send data size: %d\n", strlen(reinterpret_cast<const char *>(Rf_Send_Package)));
                uint16_t timeout_count = 0;
                if (Sx1262->send_data(Rf_Send_Package, strlen(reinterpret_cast<const char *>(Rf_Send_Package))) == true)
                {
                    while (1) // 等待发送完成
                    {
                        if (Xl9535->pin_read(XL9535_SX1262_DIO1) == 1) // 发送完成中断
                        {
                            // 检查中断
                            Cpp_Bus_Driver::Sx126x::Irq_Status is;
                            if (Sx1262->parse_irq_status(Sx1262->get_irq_flag(), is) == false)
                            {
                                printf("parse_Iqr_status fail\n");
                            }
                            else
                            {
                                if (is.all_flag.tx_done == true) // 发送完成
                                {
                                    printf("sx1262 send success\n");
                                    break;
                                }
                            }
                        }

                        timeout_count++;
                        if (timeout_count > 1000) // 超时
                        {
                            printf("sx1262 send timeout\n");
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
                else
                {
                    printf("sx1262 send fail\n");
                }

                // vTaskDelay(pdMS_TO_TICKS(1000));

                // 还原接收模式
                Sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
                Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
                Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);

                Rf_Send_Flag = false;
            }

            if (Xl9535->pin_read(XL9535_SX1262_DIO1) == 1) // 接收完成中断
            {
                // 检查中断
                Cpp_Bus_Driver::Sx126x::Irq_Status is;
                if (Sx1262->parse_irq_status(Sx1262->get_irq_flag(), is) == false)
                {
                    printf("parse_irq_status fail\n");
                }
                else
                {
                    if (is.all_flag.tx_rx_timeout == true)
                    {
                        printf("receive timeout\n");
                        Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TIMEOUT);
                    }
                    else if (is.all_flag.crc_error == true)
                    {
                        printf("receive crc error\n");
                        Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::CRC_ERROR);
                    }
                    else if (is.lora_reg_flag.header_error == true)
                    {
                        printf("receive header error\n");
                        Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::HEADER_ERROR);
                    }
                    else
                    {
                        uint8_t receive_package[255] = {0};
                        uint8_t length_buffer = Sx1262->receive_data(receive_package);
                        if (length_buffer == 0)
                        {
                            printf("sx1262 receive fail (error assert: %d)\n", Sx1262->_assert);
                        }
                        else
                        {
                            Cpp_Bus_Driver::Sx126x::Packet_Metrics pm;
                            if (Sx1262->get_lora_packet_metrics(pm) == true)
                            {
                                printf("sx1262 receive rssi_average: %.01f rssi_instantaneous: %.01f snr: %.01f\n", pm.lora.rssi_average, pm.lora.rssi_instantaneous, pm.lora.snr);
                            }

                            for (uint8_t i = 0; i < length_buffer; i++)
                            {
                                printf("get sx1262 data[%d]: %d\n", i, receive_package[i]);
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
                            snprintf(buffer_data_info, sizeof(buffer_data_info), "rssi[%.01f] snr[%.01f]", pm.lora.rssi_instantaneous, pm.lora.snr);

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
                    }
                }

                Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
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
                    printf("nrf24l01 receive rssi: none snr: none\n");

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
                    snprintf(buffer_data_info, sizeof(buffer_data_info), "rssi[none] snr[none]");

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

// if (Xl9535->pin_read(Xl9535_TOUCH_INT) == 0)
// {
#if defined CONFIG_SCREEN_TYPE_HI8561

    Cpp_Bus_Driver::Hi8561_Touch::Touch_Point tp;

    if (Hi8561_Touch->get_multiple_touch_point(tp) == true)
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

    if (Gt9895->get_multiple_touch_point(tp) == true)
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
#error "no macro definition is set"
#endif

        System_Ui->_touch_point = tp;

        tp.info.clear();
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

#else
#error "no macro definition is set"
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

    if (Tca8418_Interrupt_Flag == true)
    {
        Cpp_Bus_Driver::Tca8418::Irq_Status is;

        if (Tca8418->parse_irq_status(Tca8418->get_irq_flag(), is) == false)
        {
            printf("parse_irq_status fail\n");
        }
        else
        {
            if (is.key_events_flag == true)
            {
                Cpp_Bus_Driver::Tca8418::Touch_Point tp;
                if (Tca8418->get_multiple_touch_point(tp) == true)
                {
                    // printf("touch finger: %d\n", tp.finger_count);

                    for (uint8_t i = 0; i < tp.info.size(); i++)
                    {
                        switch (tp.info[i].event_type)
                        {
                        case Cpp_Bus_Driver::Tca8418::Event_Type::KEYPAD:
                        {
                            Cpp_Bus_Driver::Tca8418::Touch_Position tp_2;
                            if (Tca8418->parse_touch_num(tp.info[i].num, tp_2) == true)
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
                                            Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH); // 关闭LED
                                            Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
                                            Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
                                        }
                                        else
                                        {
                                            Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::LOW); // 开启LED
                                            Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
                                            Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::LOW);
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

                Tca8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);
            }
        }

        Tca8418_Interrupt_Flag = false;
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
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        break;
    case Cc1101_Rf_Switch::RF_SWITCH_434MHZ:
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        break;
    case Cc1101_Rf_Switch::RF_SWITCH_868_915MHZ:
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        break;

    default:
        printf("unknown rf switch\n");
        break;
    }
}

bool Set_T_Mixrf_Lr1121_Sleep()
{
    Xl9555->pin_mode(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    // Xl9555->pin_write(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    // vTaskDelay(pdMS_TO_TICKS(10));
    Xl9555->pin_write(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    // vTaskDelay(pdMS_TO_TICKS(10));
    // Xl9555->pin_write(XL9555_T_MIXRF_LR1121_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    // vTaskDelay(pdMS_TO_TICKS(10));

    // Xl9555->pin_mode(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    // Xl9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    // auto lr1121_spi_bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_LR1121_MOSI, T_MIXRF_LR1121_SCLK, T_MIXRF_LR1121_MISO, SPI2_HOST, 0);
    // RadioLibHal *lr1121_radiolib_hal = new Radiolib_Cpp_Bus_Driver_Hal(lr1121_spi_bus, 10000000, -1);
    // LR1121 lr1121 = new Module(lr1121_radiolib_hal, static_cast<uint32_t>(RADIOLIB_NC),
    //                            static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC), static_cast<uint32_t>(RADIOLIB_NC));

    // Xl9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    // int16_t assert = lr1121.begin(434.0, 125.0, 9, 7, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, 10, 8, 3.3);
    // if (assert == RADIOLIB_ERR_NONE)
    // {
    //     printf("lr1121 init success\n");
    // }
    // else
    // {
    //     printf("lr1121 init fail (error code: %d)\n", assert);
    //     Xl9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    //     return false;
    // }
    // assert = lr1121.sleep();
    // if (assert != RADIOLIB_ERR_NONE)
    // {
    //     printf("lr1121 sleep fail (error code: %d)\n", assert);
    //     Xl9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    //     return false;
    // }

    // Xl9555->pin_write(XL9555_T_MIXRF_LR1121_CS, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    return true;
}

#endif

void System_Ui_Callback_Init(void)
{
    System_Ui->_device_vibration_callback = [](uint8_t vibration_count)
    {
        Aw86224_Vibration_Play_Count = vibration_count;
        vTaskResume(Vibration_Task_Handle);
    };

    System_Ui->_win_cit_speaker_test_callback = [](void)
    {
        Es8311_Speaker_Mode = Es8311_Mode::TEST;

        vTaskResume(Speaker_Task_Handle);
    };

    System_Ui->_win_cit_microphone_test_callback = [](bool status)
    {
        if (status == true)
        {
            Es8311_Microphone_Mode = Es8311_Mode::TEST;

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
            Es8311->set_adc_data_to_dac(true);
        }
        else
        {
            Es8311->set_adc_data_to_dac(false);
        }
    };

    System_Ui->_win_cit_imu_test_callback = [](bool status)
    {
        if (status == true)
        {
            Icm20948_Imu_Mode = Imu_Mode::TEST;

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
            L76k->clear_rx_buffer_data();

            L76k->sleep(false);
            L76k_Gps_Positioning_Time = 0;
            L76k_Gps_Positioning_Flag = false;

            vTaskResume(Gps_Task_Handle);
        }
        else
        {
            vTaskSuspend(Gps_Task_Handle);

            L76k->sleep(true);
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

    System_Ui->_win_cit_esp32c6_test_callback = [](bool status)
    {
        if (status == true)
        {
            Esp32c6_Mode = Esp32c6_Mode_List::TEST;

            vTaskResume(Esp32c6_Task_Handle);
        }
        else
        {
            vTaskSuspend(Esp32c6_Task_Handle);
        }
    };

    System_Ui->_win_camera_status_callback = [](bool status)
    {
        if (Sys_Status.camera.init_flag == true)
        {
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4
#elif defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
            vTaskDelay(pdMS_TO_TICKS(1000));
#else
#error "no macro definition is set"
#endif
            if (status == true)
            {
                esp_err_t assert = app_video_set_bufs(video_cam_fd0, CAMERA_BUFFER_COUNT, NULL);
                if (assert != ESP_OK)
                {
                    printf("app_video_set_bufs fail (error code: %#X)\n", assert);
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

    System_Ui->_win_rf_config_sx1262_params_callback = [](Lvgl_Ui::System::Device_Sx1262 device_sx1262) -> bool
    {
        if (device_sx1262.params.rf_switch == 0)
        {
            Xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        }
        else
        {
            Xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        }

        if (Sx1262->config_lora_params(device_sx1262.params.freq, device_sx1262.params.bandwidth, device_sx1262.params.current_limit,
                                       device_sx1262.params.power, device_sx1262.params.sf, device_sx1262.params.cr, device_sx1262.params.crc_type,
                                       device_sx1262.params.preamble_length, device_sx1262.params.sync_word) == false)
        {
            printf("config_lora_params fail\n");
            return false;
        }
        Sx1262->clear_buffer();
        Sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
        Sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        Sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);

        printf("config_lora_params finish start sx1262 transmit\n");
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
            Es8311_Speaker_Mode = Es8311_Mode::PLAY_MUSIC;

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

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
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

    System_Ui->_win_cit_hcc_switch_callback = [](bool status)
    {
        if (status == true)
        {
            Kode_Bq25896::bq25896_set_charge_current(Bq25896_Handle, Kode_Bq25896::bq25896_ichg_t::BQ25896_ICHG_2048MA);
        }
        else
        {
            Kode_Bq25896::bq25896_set_charge_current(Bq25896_Handle, Kode_Bq25896::bq25896_ichg_t::BQ25896_ICHG_512MA);
        }
    };
#endif

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

        Esp32p4->create_gpio_interrupt(T_MIXRF_NRF24L01_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
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

#endif
}

void Lvgl_Init(void)
{
    printf("initialize lvgl\n");

    lv_init();

    // create a lvgl display
    lv_display_t *display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, Screen.get());
    // set color depth
    lv_display_set_color_format(display, [](uint8_t format) -> lv_color_format_t
                                {
                                    switch (format)
                                    {
                                    case 16:
                                        return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
                                    case 24:
                                        return lv_color_format_t::LV_COLOR_FORMAT_RGB888;
                                    default:
                                        return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
                                    } }(SCREEN_BITS_PER_PIXEL));
    // create draw buffer
    printf("allocate separate lvgl draw buffers\n");
    size_t draw_buffer_sz = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf1);
    // void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    // assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, [](lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
                            {
                                lv_display_rotation_t rotation = lv_display_get_rotation(disp);

#if defined CONFIG_SCREEN_TYPE_HI8561
                                auto screen = (Cpp_Bus_Driver::Hi8561 *)lv_display_get_user_data(disp);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
                                auto screen = (Cpp_Bus_Driver::Rm69a10 *)lv_display_get_user_data(disp);
#else
#error "no macro definition is set"
#endif

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
                                    auto output_buffer = std::unique_ptr<uint8_t[], std::function<void(uint8_t *)>>(
                                        (uint8_t *)heap_caps_aligned_calloc(data_cache_line_size_2, 1, ALIGN_UP(output_buffer_size, data_cache_line_size_2)
                                        , MALLOC_CAP_SPIRAM),
                                        [](uint8_t *p)
                                        { heap_caps_free(p); });
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
#error "no macro definition is set"
#endif
                                                },

                                            .out =
                                                {
                                                    .buffer = output_buffer.get(),
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
#error "no macro definition is set"
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

                                    screen->send_color_stream_coordinate(rotated_offsetx1, rotated_offsety1,rotated_offsetx2 + 1, rotated_offsety2 + 1, output_buffer.get());

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

                                    screen->send_color_stream_coordinate(offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
#endif
                                }
                                else
                                {
                                    screen->send_color_stream_coordinate(offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
                                } });

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    lv_indev_t *indev_2 = lv_indev_create();
    lv_indev_set_type(indev_2, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_2, my_keyboard_read);
#endif

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
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(Screen_Mipi_Bus->get_device_handle(), &cbs, display));

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

    lv_display_set_rotation(display, [](uint8_t rotation) -> lv_display_rotation_t
                            {
                                switch (rotation)
                                {
                                case 0:
                                    return lv_display_rotation_t::LV_DISPLAY_ROTATION_0;
                                case 90:
                                    return lv_display_rotation_t::LV_DISPLAY_ROTATION_90;
                                default:
                                    return lv_display_rotation_t::LV_DISPLAY_ROTATION_0;
                                } }(SCREEN_ROTATION_DIRECTION));

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

void Es8311_Init(void)
{
    Es8311->begin(AUDIO_MCLK_MULTIPLE, AUDIO_SAMPLE_RATE, i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT);

    if (Es8311->begin(50000) == true)
    {
        printf("Es8311 initialization success\n");
        Sys_Status.es8311.init_flag = true;
    }
    else
    {
        printf("Es8311 initialization fail\n");
        Sys_Status.es8311.init_flag = false;
    }

    Es8311->set_master_clock_source(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK);
    Es8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK, true);
    Es8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_BCLK, true);

    Es8311->set_clock_coeff(AUDIO_MCLK_MULTIPLE, AUDIO_SAMPLE_RATE);

    Es8311->set_serial_port_mode(Cpp_Bus_Driver::Es8311::Serial_Port_Mode::SLAVE);

    Es8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::ADC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
    Es8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::DAC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
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
    Es8311->set_power_status(ps);
    Es8311->set_pga_power(true);
    Es8311->set_adc_power(true);
    Es8311->set_dac_power(true);
    Es8311->set_output_to_hp_drive(true);
    Es8311->set_adc_offset_freeze(Cpp_Bus_Driver::Es8311::Adc_Offset_Freeze::DYNAMIC_HPF);
    Es8311->set_adc_hpf_stage2_coeff(10);
    Es8311->set_dac_equalizer(false);

    Es8311->set_mic(Cpp_Bus_Driver::Es8311::Mic_Type::ANALOG_MIC, Cpp_Bus_Driver::Es8311::Mic_Input::MIC1P_1N);
    Es8311->set_adc_auto_volume_control(false);
    Es8311->set_adc_gain(Cpp_Bus_Driver::Es8311::Adc_Gain::GAIN_18DB);
    Es8311->set_adc_pga_gain(Cpp_Bus_Driver::Es8311::Adc_Pga_Gain::GAIN_30DB);

    Es8311->set_adc_volume(191);
    Es8311->set_dac_volume(191);

    // 将ADC的数据自动输出到DAC上
    // Es8311->set_adc_data_to_dac(true);
}

bool Icm20948_Init(void)
{
    Wire1.begin(ICM20948_SDA, ICM20948_SCL);
    if (Icm20948->init() == false)
    {
        printf("Icm20948 ag init fail\n");
        return false;
    }

    if (Icm20948->initMagnetometer() == false)
    {
        printf("Icm20948 m init fail\n");
        return false;
    }

    printf("Position your Icm20948 flat and don't move it - calibrating...\n");
    Icm20948->autoOffsets();
    printf("Done!\n");

    Icm20948->setAccRange(ICM20948_ACC_RANGE_2G);
    Icm20948->setAccDLPF(ICM20948_DLPF_6);
    Icm20948->setMagOpMode(AK09916_CONT_MODE_20HZ);

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
    uint8_t eth_port_cnt = 0;
    char if_key_str[10];
    char if_desc_str[10];
    esp_eth_handle_t *eth_handles;
    esp_netif_config_t cfg;
    esp_netif_inherent_config_t eth_netif_cfg;

    ethernet_init_all(&eth_handles, &eth_port_cnt);

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    if (eth_port_cnt == 1)
    {
        // Use default config when using one interface
        eth_netif_cfg = *(ESP_NETIF_BASE_DEFAULT_ETH);
    }
    else
    {
        // Set config to support multiple interfaces
        eth_netif_cfg = (esp_netif_inherent_config_t)ESP_NETIF_INHERENT_DEFAULT_ETH();
    }
    cfg = (esp_netif_config_t){
        .base = &eth_netif_cfg,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
    for (int i = 0; i < eth_port_cnt; i++)
    {
        sprintf(if_key_str, "ETH_%d", i);
        sprintf(if_desc_str, "eth%d", i);
        eth_netif_cfg.if_key = if_key_str;
        eth_netif_cfg.if_desc = if_desc_str;
        eth_netif_cfg.route_prio -= i * 5;
        esp_netif_t *eth_netif = esp_netif_new(&cfg);
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
        esp_eth_start(eth_handles[i]);
    }
}

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        printf("wifi started\n");

        // esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("wifi disconnected / connect failed, retrying...\n");
        Sys_Status.esp32c6.wifi_connect_status = false;
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ip_str[64];
        sprintf(ip_str, "wifi ip:" IPSTR ":", IP2STR(&event->ip_info.ip));
        printf("got ip: %s\n", ip_str);

        Sys_Status.esp32c6.wifi_connect_status = true;
    }
}

bool Wifi_Init_Connect(void)
{
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    // {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config =
        {
            .sta =
                {
                    .ssid = WIFI_SSID,
                    .password = WIFI_PASSWORD,
                },
        };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("connecting to wifi: %s\n", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Wait for connection
    for (int i = 0; i < 10; i++)
    {
        wifi_ap_record_t ap_info;
        // Check connection status
        if ((esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) && (ap_info.ssid[0] != 0))
        {
            printf("wifi connected\n");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    return false;
}

void Wifi_Get_Real_Time(void)
{
    System_Ui->set_wifi_connect_status(Sys_Status.esp32c6.wifi_connect_status);

    if (Sys_Status.esp32c6.wifi_connect_status == false)
    {
        return;
    }

    Real_Time rt;
    if (Http_Client_Get_Real_Time(rt) == true)
    {
        printf("Http_Client_Get_Real_Time success\n");
        printf("real_time week: [%s] day: [%d] month: [%d] year: [%d] time: [%d:%d:%d] time zone: [%s] china time: [%d:%d:%d]\n",
               rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute, rt.second, rt.time_zone.c_str(),
               (rt.hour + 8 + 24) % 24, rt.minute, rt.second);

        Save_Real_Time(rt);
    }
    else
    {
        printf("Http_Client_Get_Real_Time fail\n");

        // 保存rtc时间
        Cpp_Bus_Driver::Pcf8563x::Time t;
        if (Pcf8563->get_time(t) == true)
        {
            printf("Pcf8563 year:[%d] month:[%d] day:[%d] time:[%d:%d:%d] week:[%d]\n", t.year, t.month, t.day,
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

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read((tinyusb_cdcacm_itf_t)itf, rx_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
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
                tinyusb_cdcacm_write_queue((tinyusb_cdcacm_itf_t)msg.itf, msg.buf, msg.buf_len);
                esp_err_t err = tinyusb_cdcacm_write_flush((tinyusb_cdcacm_itf_t)msg.itf, 0);
                if (err != ESP_OK)
                {
                    printf("CDC ACM write flush error: %s\n", esp_err_to_name(err));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len)
{
    if (System_Ui->_registry.system_message_box.occupancy_flag == true)
    {
        Camera_Refresh_Flag = false;
        fps_count = 0;
        return;
    }

    fps_count++;
    if (fps_count == 50)
    {
        int64_t end_time = esp_timer_get_time();
        printf("fps: %f\n", 1000000.0 / ((end_time - start_time) / 50.0));
        start_time = end_time;
        fps_count = 0;

        Camera_Refresh_Flag = true;

        printf("camera_buf_hes: %lu, camera_buf_ves: %lu, camera_buf_len: %d KB\n", camera_buf_hes, camera_buf_ves, camera_buf_len / 1024);
    }

    if (Camera_Refresh_Flag == false)
    {
        return;
    }

    uint32_t input_img_block_width = (camera_buf_hes - SCREEN_WIDTH) / 2;
    uint32_t input_img_block_height = 0;
    uint32_t input_img_width = SCREEN_WIDTH;
    uint32_t input_img_height = camera_buf_ves;

    uint32_t output_img_width = input_img_width;
    uint32_t output_img_height = input_img_height;

    size_t output_buffer_size = output_img_width * output_img_height * (SCREEN_BITS_PER_PIXEL / 8);

    auto output_buffer = std::unique_ptr<uint8_t[], std::function<void(uint8_t *)>>(
        (uint8_t *)heap_caps_aligned_calloc(data_cache_line_size, 1, output_buffer_size, MALLOC_CAP_SPIRAM),
        [](uint8_t *p)
        { heap_caps_free(p); });
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
#error "no macro definition is set"
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
                    .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#else
#error "no macro definition is set"
#endif
                },

            .out =
                {
                    .buffer = output_buffer.get(),
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
#error "no macro definition is set"
#endif
                },

            .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
            .scale_x = 1,
            .scale_y = 1,
            .mirror_x = false,
#if SCREEN_ROTATION_DIRECTION == 0
#if defined CONFIG_SCREEN_TYPE_HI8561
            .mirror_y = true,
#elif defined CONFIG_SCREEN_TYPE_RM69A10
            .mirror_y = false,
#else
#error "no macro definition is set"
#endif
#elif SCREEN_ROTATION_DIRECTION == 90
            .mirror_y = false,
#else
#error "no macro definition is set"
#endif
            .rgb_swap = false,
            .byte_swap = false,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };

    esp_err_t assert = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
    if (assert != ESP_OK)
    {
        printf("ppa_do_scale_rotate_mirror fail (error code: %#X)\n", assert);
        return;
    }

    if (System_Ui->get_current_win() == Lvgl_Ui::System::Current_Win::CAMERA)
    {
        if (Screen->send_color_stream_coordinate(0, (SCREEN_HEIGHT - output_img_height) / 2,
                                                 output_img_width, output_img_height + ((SCREEN_HEIGHT - output_img_height) / 2),
                                                 output_buffer.get()) == false)
        {
            printf("send_color_stream_coordinate fail\n");
            return;
        }
        // _lock_acquire(&lvgl_api_lock);
        // lv_canvas_set_buffer(System_Ui->_registry.win.camera.canvas, output_buffer.get(),
        //                      SCREEN_WIDTH, 720,
        //                      [](uint8_t format) -> lv_color_format_t
        //                      {
        //                     switch (format)
        //                     {
        //                     case 16:
        //                         return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
        //                     case 24:
        //                         return lv_color_format_t::LV_COLOR_FORMAT_RGB888;
        //                     default:
        //                         return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
        //                     } }(SCREEN_BITS_PER_PIXEL));
        // _lock_release(&lvgl_api_lock);
    }
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
                .i2c_handle = Sgm38121_Iic_Bus->get_bus_handle(),
                .freq = static_cast<uint32_t>(100000),
            },
            .reset_pin = gpio_num_t ::GPIO_NUM_NC,
            .pwdn_pin = gpio_num_t ::GPIO_NUM_NC,

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
#error "no macro definition is set"
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB565);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#else
#error "no macro definition is set"
#endif

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

    assert = app_video_stream_task_start(video_cam_fd0, 0);
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

#if (CONFIG_ENABLE_PPA_SCREEN_ROTATION == true) && (SCREEN_ROTATION_DIRECTION != 0)
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

static void Lvgl_Show_Device_Message(const char *msg)
{
    vTaskDelay(pdMS_TO_TICKS(500));

    _lock_acquire(&lvgl_api_lock);
    System_Ui->create_system_message_box(lv_screen_active(), "device message", msg);
    _lock_release(&lvgl_api_lock);

    while (System_Ui->_registry.system_message_box.occupancy_flag == true)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void System_Startup_Message_Init(void)
{
    if (Sys_Status.sgm38121.init_flag == false)
    {
        Lvgl_Show_Device_Message("sgm38121 init fail");
    }

    if (Sys_Status.camera.init_flag == false)
    {
        Lvgl_Show_Device_Message("camera init fail");
    }

    if (Sys_Status.esp32c6.wifi_connect_status == false)
    {
        Lvgl_Show_Device_Message("esp32c6 connect wifi fail");
    }

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
    if (Sys_Status.bq25896.init_flag == false)
    {
        Lvgl_Show_Device_Message("bq25896 init fail");
    }
#endif

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD

    if (Sys_Status.xl9555.init_flag == false)
    {
        Lvgl_Show_Device_Message("xl9555 init fail");
    }

    if (Sys_Status.tca8418.init_flag == false)
    {
        Lvgl_Show_Device_Message("tca8418 init fail");
    }

    if (Sys_Status.st25r3916.init_flag == false)
    {
        Lvgl_Show_Device_Message("st25r3916 init fail");
    }

    if (Sys_Status.cc1101.init_flag == false)
    {
        Lvgl_Show_Device_Message("cc1101 init fail");
    }

    if (Sys_Status.nrf24l01.init_flag == false)
    {
        Lvgl_Show_Device_Message("nrf24l01 init fail");
    }

#endif

    if (Sys_Status.pcf8563.init_flag == false)
    {
        Lvgl_Show_Device_Message("pcf8563 init fail");
    }

    if (Sys_Status.bq27220.init_flag == false)
    {
        Lvgl_Show_Device_Message("bq27220 init fail");
    }

    if (Sys_Status.aw86224.init_flag == false)
    {
        Lvgl_Show_Device_Message("aw86224 init fail");
    }

    if (Sys_Status.es8311.init_flag == false)
    {
        Lvgl_Show_Device_Message("es8311 init fail");
    }

    if (Sys_Status.icm20948.init_flag == false)
    {
        Lvgl_Show_Device_Message("icm20948 init fail");
    }

    if (Sys_Status.l76k.init_flag == false)
    {
        Lvgl_Show_Device_Message("l76k init fail");
    }

    if (Sys_Status.sx1262.init_flag == false)
    {
        Lvgl_Show_Device_Message("sx1262 init fail");
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
    int16_t assert = Kode_Bq25896::bq25896_init(Bq25896_Iic_Bus, Bq25896_Handle);
    if (assert != ESP_OK)
    {
        Sys_Status.bq25896.init_flag = false;
        printf("bq25896 init fail (error code: %#X)\n", assert);
    }
    else
    {
        Sys_Status.bq25896.init_flag = true;
        printf("bq25896 init success\n");

        Kode_Bq25896::bq25896_set_input_current_limit(Bq25896_Handle, Kode_Bq25896::bq25896_ilim_t ::BQ25896_ILIM_2000MA);
        // 禁用看门狗后不能读取看门狗寄存器状态，否者看门狗禁用会失效
        Kode_Bq25896::bq25896_set_watchdog_timer(Bq25896_Handle, Kode_Bq25896::bq25896_watchdog_t::BQ25896_WATCHDOG_DISABLE);
        Kode_Bq25896::bq25896_set_adc_conversion(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
        Kode_Bq25896::bq25896_set_adc_conversion_rate(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);
        Kode_Bq25896::bq25896_set_charge_current(Bq25896_Handle, Kode_Bq25896::bq25896_ichg_t::BQ25896_ICHG_512MA);
        // Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);
    }
#endif

    Hardware_Usb_Cdc_Init();
    xTaskCreate(hardware_usb_cdc_task, "hardware_usb_cdc_task", 4 * 1024, NULL, 3, NULL);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
    Xl9535_Iic_Bus->set_bus_handle(Bq25896_Iic_Bus->get_bus_handle());
#endif
    Xl9535->begin();

    Xl9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_EXTERNAL_SENSOR_INT, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    Esp32p4->pin_mode(ETHERNET_MDC, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
    Esp32p4->pin_mode(ETHERNET_MDIO, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));

    if (Sgm38121->begin() == false)
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
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV2710
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1700);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3000);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV5645
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "no macro definition is set"
#endif

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(3, 2500);

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(4, 3300);

    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    Ethernet_Init();

#if defined CONFIG_SCREEN_TYPE_HI8561
    // 这个必须放在以太网后面
    Screen->create_pwm(HI8561_SCREEN_BL, ledc_timer_t::LEDC_TIMER_0, ledc_channel_t::LEDC_CHANNEL_0, 2000);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#else
#error "no macro definition is set"
#endif

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

#if (CONFIG_ENABLE_PPA_SCREEN_ROTATION == true) && (SCREEN_ROTATION_DIRECTION != 0)
    if (Ppa_Screen_Rotation_Init() == false)
    {
        printf("Ppa_Screen_Rotation_init fail\n");
    }
    else
    {
        printf("Ppa_Screen_Rotation_init success\n");
    }
#endif

    Screen->begin(SCREEN_MIPI_DSI_DPI_CLK_MHZ, SCREEN_LANE_BIT_RATE_MBPS);

#if defined CONFIG_SCREEN_TYPE_HI8561
    Hi8561_Iic_Touch_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    Hi8561_Touch->begin();

#elif defined CONFIG_SCREEN_TYPE_RM69A10

    Gt9895_Touch_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    Gt9895->begin();

#else
#error "no macro definition is set"
#endif

    Lvgl_Init();
    Lvgl_Startup();
    xTaskCreate(lvgl_ui_task, "lvgl_ui_task", 100 * 1024, NULL, 1, NULL);

#if defined CONFIG_SCREEN_TYPE_HI8561
    Screen->start_pwm_gradient_time(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        Screen->set_brightness(i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "no macro definition is set"
#endif

    // SDMMC_HOST_SLOT_1必须要先于SDMMC_HOST_SLOT_0初始化

    if (Wifi_Init_Connect() == false)
    {
        printf("Wifi_Init_Connect fail\n");
        Sys_Status.esp32c6.wifi_connect_status = false;
    }
    else
    {
        printf("Wifi_Init_Connect success\n");
        Sys_Status.esp32c6.wifi_connect_status = true;
    }

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(10);
    _lock_release(&lvgl_api_lock);

    if (Lilygo_Device_Driver::Sdmmc_Init(SD_BASE_PATH) == false)
    {
        printf("Sdmmc_Init fail\n");
    }

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    if (Xl9555->begin() == false)
    {
        printf("xl9555 init fail\n");
        Sys_Status.xl9555.init_flag = false;
    }
    else
    {
        printf("xl9555 init success\n");
        Sys_Status.xl9555.init_flag = true;
    }

    Xl9555->pin_mode(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH); // 关闭led
    Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    Xl9555->pin_mode(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9555->pin_write(XL9555_TCA8418_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    Tca8418->create_gpio_interrupt(TCA8418_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Tca8418_Interrupt_Flag = true;
                                   });

    if (Tca8418->begin() == false)
    {
        printf("tca8418 init fail\n");
        Sys_Status.tca8418.init_flag = false;
    }
    else
    {
        printf("tca8418 init success\n");
        Sys_Status.tca8418.init_flag = true;
    }
    Tca8418->set_keypad_scan_window(0, 0, TCA8418_KEYPAD_SCAN_WIDTH, TCA8418_KEYPAD_SCAN_HEIGHT);
    Tca8418->set_irq_pin_mode(Cpp_Bus_Driver::Tca8418::Irq_Mask::KEY_EVENTS);
    Tca8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);

    Tca8418->create_pwm(KEYBOARD_BL, ledc_timer_t::LEDC_TIMER_1, ledc_channel_t::LEDC_CHANNEL_1, 1000000,
                        0, ledc_mode_t::LEDC_LOW_SPEED_MODE, ledc_timer_bit_t ::LEDC_TIMER_5_BIT);
    Tca8418->start_pwm_gradient_time(30, 1000);

    Xl9555->pin_mode(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_write(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

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

    Xl9555->pin_mode(XL9555_T_MIXRF_CC1101_RF_SWITCH_0, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_T_MIXRF_CC1101_RF_SWITCH_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Esp32p4->pin_mode(T_MIXRF_CC1101_BUSY, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

    Esp32p4->create_gpio_interrupt(T_MIXRF_CC1101_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::RISING,
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

    Esp32p4->create_gpio_interrupt(T_MIXRF_NRF24L01_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Nrf24l01_Interrupt_Flag = true;
                                   });

    System_Ui->set_config_rf_params(System_Ui->_device_nrf24l01);
#endif

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(20);
    _lock_release(&lvgl_api_lock);

    Pcf8563_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    if (Pcf8563->begin() == false)
    {
        printf("Pcf8563 init fail\n");
        Sys_Status.pcf8563.init_flag = false;
    }
    else
    {
        printf("Pcf8563 init success\n");
        Sys_Status.pcf8563.init_flag = true;
    }

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(30);
    _lock_release(&lvgl_api_lock);

    Wifi_Get_Real_Time();

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(40);
    _lock_release(&lvgl_api_lock);

    Bq27220_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    if (Bq27220->begin() == false)
    {
        printf("Bq27220 init fail\n");
        Sys_Status.bq27220.init_flag = false;
    }
    else
    {
        printf("Bq27220 init success\n");
        Sys_Status.bq27220.init_flag = true;
    }

    // 设置的电池容量会在没有电池插入的时候自动还原为默认值
    Bq27220->set_design_capacity(1000);
    Bq27220->set_temperature_mode(Cpp_Bus_Driver::Bq27220xxxx::Temperature_Mode::EXTERNAL_NTC);
    Bq27220->set_sleep_current_threshold(50);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(50);
    _lock_release(&lvgl_api_lock);

    Aw86224_Iic_Bus->set_bus_handle(Sgm38121_Iic_Bus->get_bus_handle());

    if (Aw86224->begin(500000) == false)
    {
        printf("Aw86224 init fail\n");
        Sys_Status.aw86224.init_flag = false;
    }
    else
    {
        printf("Aw86224 init success\n");
        Sys_Status.aw86224.init_flag = true;
    }
    // printf("Aw86224 input voltage: %.06f V\n", Aw86224->get_input_voltage());

    // RAM播放
    Aw86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(60);
    _lock_release(&lvgl_api_lock);

    Es8311_Iic_Bus->set_bus_handle(Sgm38121_Iic_Bus->get_bus_handle());
    Es8311_Init();

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(70);
    _lock_release(&lvgl_api_lock);

    Wire1._bus->set_bus_handle(Sgm38121_Iic_Bus->get_bus_handle());
    if (Icm20948_Init() == false)
    {
        printf("Icm20948 init fail\n");
        Sys_Status.icm20948.init_flag = false;
    }
    else
    {
        printf("Icm20948 init success\n");
        Sys_Status.icm20948.init_flag = true;
    }

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(80);
    _lock_release(&lvgl_api_lock);

    if (L76k->begin() == false)
    {
        L76k_Uart_Bus->set_baud_rate(115200);

        if (L76k->begin() == false)
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

        L76k->set_baud_rate(Cpp_Bus_Driver::L76k::Baud_Rate::BR_115200_BPS);
    }
    printf("get_baud_rate:%ld\n", L76k->get_baud_rate());
    L76k->set_update_frequency(Cpp_Bus_Driver::L76k::Update_Freq::FREQ_5HZ);
    L76k->clear_rx_buffer_data();
    L76k->sleep(true);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(90);
    _lock_release(&lvgl_api_lock);

    Xl9535->pin_mode(XL9535_SX1262_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    // LORA复位
    Xl9535->pin_mode(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    Xl9535->pin_mode(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
    Sx1262_Spi_Bus->_bus_init_flag = true;
#endif
    if (Sx1262->begin(10000000) == false)
    {
        printf("sx1262 begin fail\n");
        Sys_Status.sx1262.init_flag = false;
    }
    else
    {
        printf("sx1262 begin success\n");
        Sys_Status.sx1262.init_flag = true;
    }

    System_Ui->set_config_rf_params(System_Ui->_device_sx1262);

    _lock_acquire(&lvgl_api_lock);
    Set_Lvgl_Startup_Progress_Bar(100);
    _lock_release(&lvgl_api_lock);

    _lock_acquire(&lvgl_api_lock);
    System_Ui->begin();
    _lock_release(&lvgl_api_lock);

    xTaskCreate(device_vibration_task, "device_vibration_task", 4 * 1024, NULL, 2, &Vibration_Task_Handle);
    xTaskCreate(device_speaker_task, "device_speaker_task", 4 * 1024, NULL, 3, &Speaker_Task_Handle);
    xTaskCreate(device_microphone_task, "device_microphone_task", 4 * 1024, NULL, 3, &Microphone_Task_Handle);
    xTaskCreate(device_imu_task, "device_imu_task", 4 * 1024, NULL, 3, &Imu_Task_Handle);
    xTaskCreate(device_battery_health_task, "device_battery_health_task", 8 * 1024, NULL, 3, NULL);
    xTaskCreate(device_gps_task, "device_gps_task", 8 * 1024, NULL, 3, &Gps_Task_Handle);
    xTaskCreate(device_ethernet_task, "device_ethernet_task", 4 * 1024, NULL, 3, &Ethernet_Task_Handle);
    xTaskCreate(device_rtc_task, "device_rtc_task", 4 * 1024, NULL, 3, NULL);
    xTaskCreate(device_esp32c6_task, "device_esp32c6_task", 4 * 1024, NULL, 3, &Esp32c6_Task_Handle);
    xTaskCreate(device_rf_task, "device_rf_task", 4 * 1024, NULL, 3, &Rf_Task_Handle);
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

    //             if (Hi8561_Touch->get_multiple_touch_point(tp) == true)
    //             {
    //                 printf("touch finger: %d edge touch flag: %d\n", tp.finger_count, tp.edge_touch_flag);

    //                 for (uint8_t i = 0; i < tp.info.size(); i++)
    //                 {
    //                     printf("touch num:[%d] x: %d y: %d p: %d\n", i + 1, tp.info[i].x, tp.info[i].y, tp.info[i].pressure_value);
    //                 }
    //             }
    // #elif defined CONFIG_SCREEN_TYPE_RM69A10
    //             Cpp_Bus_Driver::Gt9895::Touch_Point tp;

    //             if (Gt9895->get_multiple_touch_point(tp) == true)
    //             {
    //                 printf("touch finger: %d edge touch flag: %d\n", tp.finger_count, tp.edge_touch_flag);

    //                 for (uint8_t i = 0; i < tp.info.size(); i++)
    //                 {
    //                     printf("touch num:[%d] id:[%d] x: %d y: %d p: %d\n", i + 1, tp.info[i].finger_id, tp.info[i].x, tp.info[i].y, tp.info[i].pressure_value);
    //                 }
    //             }
    // #else
    // #error "no macro definition is set"
    // #endif

    //             Cycle_Time = esp_log_timestamp() + 1000;
    //         }

    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
}
