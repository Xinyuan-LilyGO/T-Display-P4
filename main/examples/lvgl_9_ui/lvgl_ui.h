/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-11-28 17:07:50
 * @LastEditTime: 2026-03-24 15:34:54
 * @License: GPL 3.0
 */
#pragma once

#include "lvgl.h"
#include "cpp_bus_driver_library.h"
#include "t_display_p4_config.h"

extern const lv_image_dsc_t win_home_app_icon_cit_110x110px_rgb565a8;
extern const lv_image_dsc_t win_home_app_icon_camera_110x110px_rgb565a8;
extern const lv_image_dsc_t win_home_app_icon_setings_110x110px_rgb565a8;
extern const lv_image_dsc_t win_home_app_icon_rf_110x110px_rgb565a8;
extern const lv_image_dsc_t win_home_app_icon_music_110x110px_rgb565a8;

extern const lv_image_dsc_t win_music_play_start_1_140x140px_rgb565a8;
extern const lv_image_dsc_t win_music_play_start_2_140x140px_rgb565a8;
extern const lv_image_dsc_t win_music_play_switch_left_1_95x95px_rgb565a8;
extern const lv_image_dsc_t win_music_play_switch_left_2_95x95px_rgb565a8;
extern const lv_image_dsc_t win_music_play_switch_right_1_95x95px_rgb565a8;
extern const lv_image_dsc_t win_music_play_switch_right_2_95x95px_rgb565a8;
extern const lv_image_dsc_t win_music_play_pause_1_117x117px_rgb565a8;
extern const lv_image_dsc_t win_music_play_pause_2_117x117px_rgb565a8;
extern const lv_image_dsc_t win_music_album_cover_540x540px_rgb565a8;

extern const lv_font_t lvgl_font_lineseedkr_rg_120;
extern const lv_font_t lvgl_font_lineseedkr_th_60;
extern const lv_font_t lvgl_font_misans_bold_27;

namespace Lvgl_Ui
{
    using namespace Cpp_Bus_Driver;
    class System
    {
    private:
#define RESOURCE_ROOT_DIRECTORY "A:" SD_BASE_PATH "/t_display_p4_lvgl_9_ui_resource/"
#define GET_PATH(path) RESOURCE_ROOT_DIRECTORY path
#define GET_WALLPAPER_PATH(path) GET_PATH("wallpaper/") path

#define APP_STYLE_ICON_WIDTH_HEIGHT 110 // 应用图标尺寸 对应图标图片像素的大小

        static constexpr char *DEFAULT_KEYBOARD_ALLOWED_CHARS =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_-+=[]{}|;:',.<>?/\" \\`~ ";

        struct Win_Home_App_Icon
        {
            const std::string name;
            const lv_image_dsc_t *image;
        };

        static const Win_Home_App_Icon _win_home_app_icon_list[];
        static const Win_Home_App_Icon _win_home_app_icon_fixed_list[];

        struct Win_Cit_Test_Item
        {
            std::string name;
            std::string symbol;
            uint32_t color;
        };

        struct Device_Information
        {
            std::string name;
            std::string info;
        };

        static Win_Cit_Test_Item _win_cit_test_item_list[];

        static Device_Information _device_information_list[];

    public:
        uint32_t _width;
        uint32_t _height;

        enum class Current_Win
        {
            UNKNOWN,
            HOME,
            CIT,
            CIT_VIBRATION_TEST,
            CIT_RTC_TEST,
            CIT_BATTERY_HEALTH_TEST,
            CIT_TOUCH_TEST,
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
            CIT_KEYBOARD_TEST,
            CIT_NFC_TEST,
#endif
            CAMERA,
            RF,
            RF_SETINGS,
            MUSIC,
        };

        enum class Sleep_Mode
        {
            NORMAL_SLEEP,
            LIGHT_SLEEP,
        };

        enum class Chat_Message_Direction
        {
            SEND,
            RECEIVE,
        };

        enum class Rf_Chip_Type
        {
            SX1262 = 0,
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
            CC1101,
            NRF24L01,
#endif
        };

        struct Win_Rf_Chat_Message
        {
            Chat_Message_Direction direction;
            std::string time;
            std::string data;
            std::string data_info;
        };

        struct Registry
        {
            lv_obj_t *keyboard;
            lv_group_t *keyboard_group;

            struct
            {
                lv_obj_t *message_box;

                // 占用标志，用于判断该消息框是否被占用
                bool occupancy_flag = false;

                size_t system_message_height = 0;
            } system_message_box;

            struct
            {
                lv_obj_t *root;
                lv_obj_t *time_label;
                lv_obj_t *battery_icon;
                lv_obj_t *wifi_signal_icon;
            } status_bar;

            struct
            {
                struct
                {
                    lv_obj_t *root;

                    struct
                    {
                        lv_obj_t *time_label;
                        lv_obj_t *month_label;
                        lv_obj_t *week_label;

                    } clock;

                } home;

                struct
                {
                    lv_obj_t *root;
                    uint8_t current_test_item_index;

                    lv_obj_t *version_information_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *canvas;
                        lv_layer_t layer;

                        lv_obj_t *touch_data_label;

                        std::vector<uint16_t> draw_x;
                        std::vector<uint16_t> draw_y;

                    } touch_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *start_color_test;
                        uint8_t color_change_count = 0;
                    } screen_color_test;

                    struct
                    {
                        lv_obj_t *root;

                        lv_obj_t *data_label;
                    } vibration_test;

                    lv_obj_t *speaker_test;

                    struct
                    {
                        lv_obj_t *root;

                        struct
                        {
                            int16_t value_percentage;
                            lv_obj_t *label;

                        } data;

                        lv_obj_t *needle_line;
                        lv_obj_t *scale_line;

                        bool adc_to_dac_switch_status = false;
                        lv_obj_t *adc_to_dac_switch;

                    } microphone_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;
                    } imu_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
                        lv_obj_t *otg_label;
                        bool otg_switch_status = false;
                        lv_obj_t *otg_switch;

                        lv_obj_t *hcc_label;
                        bool hcc_switch_status = false;
                        lv_obj_t *hcc_switch;
#endif
                    } battery_health_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;
                    } gps_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;
                    } ethernet_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;
                    } rtc_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;
                    } esp32c6_test;

                    // lv_obj_t *sleep_test;

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;
                    } keyboard_test;

                    struct
                    {
                        lv_obj_t *root;
                        lv_obj_t *data_label;
                    } nfc_test;
#endif

                } cit;

                struct
                {
                    lv_obj_t *root;

                    // lv_obj_t *canvas;
                } camera;

                struct
                {
                    lv_obj_t *root;

                    lv_obj_t *send_box_container;
                    lv_obj_t *chat_message_container;

                    lv_obj_t *chat_textarea;

                    std::string chat_textarea_data = "";

                    std::vector<Win_Rf_Chat_Message> chat_message_data;

                    struct
                    {
                        lv_obj_t *root;

                        struct
                        {
                            lv_obj_t *root;
                            lv_obj_t *root_container;
                            lv_obj_t *btn_container;
                            lv_obj_t *parameter_container;
                        } message_box;

                        struct
                        {
                            struct
                            {
                                lv_obj_t *rf_chip;
                            } dropdown;

                        } rf_chip_type;

                        struct
                        {
                            struct
                            {
                                struct
                                {
                                    lv_obj_t *freq;
                                    lv_obj_t *current_limit;
                                    lv_obj_t *power;
                                    lv_obj_t *preamble_length;
                                    lv_obj_t *sync_word;
                                } textarea;

                                struct
                                {
                                    lv_obj_t *rf_switch;
                                    lv_obj_t *bandwidth;
                                    lv_obj_t *spreading_factor;
                                    lv_obj_t *coding_rate;
                                    lv_obj_t *crc_type;
                                } dropdown;
                            } sx1262;
#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
                            struct
                            {
                                struct
                                {
                                    lv_obj_t *freq;
                                    lv_obj_t *bit_rate;
                                    lv_obj_t *freq_deviation_khz;
                                    lv_obj_t *power;
                                    lv_obj_t *preamble_length;
                                    lv_obj_t *sync_word;
                                } textarea;

                                struct
                                {
                                    lv_obj_t *rf_switch;
                                    lv_obj_t *modulation;
                                    lv_obj_t *bandwidth;
                                } dropdown;
                            } cc1101;

                            struct
                            {
                                struct
                                {
                                    lv_obj_t *freq;
                                    lv_obj_t *bit_rate;
                                    lv_obj_t *power;
                                    lv_obj_t *address_width;
                                    lv_obj_t *address;
                                } textarea;

                            } nrf24l01;
#endif

                        } config_rf_params;

                        struct
                        {
                            lv_obj_t *control_switch;
                            struct
                            {
                                lv_obj_t *auto_send_text;
                                lv_obj_t *auto_send_interval;

                            } textarea;
                        } auto_send;

                    } setings;
                } rf;

                struct
                {
                    lv_obj_t *root;
                    bool play_flag = 0;
                    double current_time_s = 0;
                    double total_time_s = 0;

                    struct
                    {
                        lv_obj_t *play;
                        lv_obj_t *switch_left;
                        lv_obj_t *switch_right;
                    } imagebutton;

                    struct
                    {
                        lv_obj_t *current_time;
                        lv_obj_t *total_time;
                    } label;

                    lv_obj_t *slider;

                } music;

            } win;
        };

        struct App_Style
        {
            struct
            {
                struct
                {
                    uint16_t width;
                    uint16_t height;
                } edge_distance;

                struct
                {
                    uint16_t width;
                    uint16_t height;
                } edge_distance_fixed;

                struct
                {
                    uint16_t width;
                    uint16_t height;
                    uint16_t fixed_width;
                } icon_distance;

            } icon;

            struct
            {
                uint16_t width;
                uint16_t height;
            } label;
        };

        struct Time
        {
            std::string week = "null";
            uint8_t day = 0;   // 日
            uint8_t month = 0; // 月
            uint16_t year = 0; // 年

            uint8_t hour = 0;   // 小时
            uint8_t minute = 0; // 分钟
            uint8_t second = 0; // 秒

            std::string time_zone = "null"; // 时区
        };

        struct Device_Sx1262
        {
            struct
            {
                bool flag = false;
                std::string text = "ciallo";
                uint32_t interval = 1000;
            } auto_send;

            struct
            {
                bool rf_switch = 0; // 射频开关
                double freq = 868.0;
                Sx126x::Lora_Bw bandwidth = Sx126x::Lora_Bw::BW_125000HZ;
                float current_limit = 140.0;
                int8_t power = 22;
                Sx126x::Sf sf = Sx126x::Sf::SF9;
                Sx126x::Cr cr = Sx126x::Cr::CR_4_7;
                Sx126x::Lora_Crc_Type crc_type = Sx126x::Lora_Crc_Type::ON;
                uint16_t preamble_length = 8;
                uint16_t sync_word = 0x1424;
            } params;
        };

        uint16_t _battery_level = 0;

        bool _wifi_connect_status = false;

        std::unique_ptr<lv_color_t[]> _lv_color_win_draw_buf = std::make_unique<lv_color_t[]>(_width * _height);

        Registry _registry;
        App_Style _app_style;

        Time _time;

        Device_Sx1262 _device_sx1262;

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
        // cc1101带宽
        enum class Cc1101_Bw
        {
            BW_58KHZ,
            BW_68KHZ,
            BW_81KHZ,
            BW_102KHZ,
            BW_116KHZ,
            BW_135KHZ,
            BW_162KHZ,
            BW_203KHZ,
            BW_232KHZ,
            BW_270KHZ,
            BW_325KHZ,
            BW_406KHZ,
            BW_464KHZ,
            BW_541KHZ,
            BW_650KHZ,
            BW_812KHZ,
        };

        enum class Cc1101_Modulation
        {
            FSK_2,
            OOK,
        };

        struct Device_Cc1101
        {
            struct
            {
                bool flag = false;
                std::string text = "ciallo";
                uint32_t interval = 1000;
            } auto_send;

            struct
            {
                uint8_t rf_switch = 2; // RF switch
                double freq = 868.0;
                Cc1101_Modulation modulation = Cc1101_Modulation::OOK;
                Cc1101_Bw bandwidth = Cc1101_Bw::BW_58KHZ;
                float bit_rate = 1.2;
                float freq_deviation_khz = 5.2;
                int8_t power = 10;
                uint16_t preamble_length = 16;
                uint16_t sync_word = 0x12AD;
            } params;
        };

        struct Device_Nrf24l01
        {
            struct
            {
                bool flag = false;
                std::string text = "ciallo";
                uint32_t interval = 1000;
            } auto_send;

            struct
            {
                double freq = 2400.0;
                float bit_rate = 1000.0;
                int8_t power = 0;
                uint8_t address_width = 5;
                uint64_t address = 0xAABBCCDDEE;
            } params;
        };

        Device_Cc1101 _device_cc1101;
        Device_Nrf24l01 _device_nrf24l01;
#endif

        Current_Win _current_win = Current_Win::UNKNOWN;

        Rf_Chip_Type _rf_chip_type = Rf_Chip_Type::SX1262;

#if defined CONFIG_SCREEN_TYPE_HI8561
        Hi8561_Touch::Touch_Point _touch_point;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
        Gt9895::Touch_Point _touch_point;
#else
#error "no macro definition is set"
#endif

        bool _edge_touch_flag = false;

        void (*_device_vibration_callback)(uint8_t vibration_count) = nullptr;

        void (*_win_cit_speaker_test_callback)(void) = nullptr;

        void (*_win_cit_microphone_test_callback)(bool status) = nullptr;

        void (*_win_cit_adc_to_dac_switch_callback)(bool status) = nullptr;

        void (*_win_cit_imu_test_callback)(bool status) = nullptr;

        void (*_win_cit_gps_test_callback)(bool status) = nullptr;

        void (*_win_cit_ethernet_test_callback)(bool status) = nullptr;

        void (*_win_cit_esp32c6_test_callback)(bool status) = nullptr;

        // void (*_device_start_sleep_test_callback)(Sleep_Mode mode) = nullptr;

        void (*_win_camera_status_callback)(bool status) = nullptr;

        bool (*_win_rf_config_sx1262_params_callback)(Device_Sx1262 device_sx1262) = nullptr;

        void (*_win_rf_send_data_callback)(std::string data) = nullptr;

        void (*_win_rf_status_callback)(bool status) = nullptr;

        void (*_win_music_start_end_callback)(bool status) = nullptr;

        void (*_set_music_current_time_s_callback)(double current_time_s) = nullptr;

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
        void (*_win_cit_otg_switch_callback)(bool status) = nullptr;

        void (*_win_cit_hcc_switch_callback)(bool status) = nullptr;
#endif

        System(uint32_t width, uint32_t height)
            : _width(width), _height(height)
        {
        }

        void begin();

        Current_Win get_current_win(void);

        void set_time(Pcf8563x::Time time);
        void set_battery_level(uint16_t battery_level);
        void set_wifi_connect_status(bool status);

        void add_event_cb_win_return_to_cit(lv_obj_t *obj);

        void add_win_cit_test_item_pass_fail_button(lv_obj_t *obj);

        void set_vibration(uint8_t vibration_count = 1);
        void set_speaker_test(void);
        void set_microphone_test(bool status);
        void set_adc_to_dac_switch_status(bool status);
        void set_imu_test(bool status);
        void set_gps_test(bool status);
        void set_ethernet_test(bool status);
        void set_esp32c6_test(bool status);
        // void start_sleep_test(Sleep_Mode mode);

        void set_camera_status(bool status);

        void init_win_home(void);

        void status_bar_time_update(void);
        void status_bar_battery_level_update(void);
        void status_bar_wifi_connect_status_update(void);

        void win_home_time_update(void);

        void init_status_bar(lv_obj_t *parent);

        void init_win_cit(void);

        void init_win_cit_version_information_test(void);

        void init_win_cit_touch_test(void);

        void init_win_cit_screen_color_test(void);
        void init_win_cit_screen_color_test_start_color_test(void);

        void init_win_cit_vibration_test(void);

        void init_win_cit_speaker_test(void);

        void init_win_cit_microphone_test(void);

        void init_win_cit_imu_test(void);

        void init_win_cit_battery_health_test(void);

        void init_win_cit_gps_test(void);

        void init_win_cit_ethernet_test(void);

        void init_win_cit_rtc_test(void);

        void init_win_cit_esp32c6_test(void);

        // void init_win_cit_sleep_test(void);

        void init_win_camera(void);

        void init_win_rf(void);
        void win_rf_chat_message_data_update(std::vector<Win_Rf_Chat_Message> wlcm);

        void init_win_rf_setings(void);
        void init_win_rf_setings_keyboard_position_event_cb(lv_obj_t *parent);
        void init_win_rf_setings_rf_chip_type_message_box(void);
        void init_win_rf_setings_config_sx1262_params_message_box(void);
        void init_win_rf_setings_auto_send_message_box(void);
        bool set_config_rf_params(Device_Sx1262 device_sx1262);
        void set_rf_send_data_callback(std::string data);
        void set_rf_status_callback(bool status);

        void init_win_music(void);
        void set_win_music_play_imagebutton_status(bool status);
        void set_win_music_current_total_time(double current_time_s, double total_time_s);
        void set_music_current_time_s(double current_time_s);
        void set_music_start_end(bool status);

        void create_keyboard(lv_obj_t *parent);

        void create_system_message_box(lv_obj_t *parent, std::string message_title, std::string message_data);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
        void set_otg_switch_status(bool status);

        void set_hcc_switch_status(bool status);
#endif

#if defined CONFIG_BOARD_TYPE_T_DISPLAY_P4_KEYBOARD
        void (*_win_cit_nfc_test_callback)(bool status) = nullptr;

        bool (*_win_rf_config_cc1101_params_callback)(Device_Cc1101 device_cc1101) = nullptr;
        bool (*_win_rf_config_nrf24l01_params_callback)(Device_Nrf24l01 device_nrf24l01) = nullptr;

        void set_nfc_test(bool status);

        void init_win_cit_keyboard_test(void);
        bool set_keyboard_group(lv_group_t *group);

        void init_win_cit_nfc_test(void);

        void init_win_rf_setings_config_cc1101_params_message_box(void);
        void init_win_rf_setings_config_nrf24l01_params_message_box(void);

        bool set_config_rf_params(Device_Cc1101 device_cc1101);
        bool set_config_rf_params(Device_Nrf24l01 device_nrf24l01);
#endif
    };

};
