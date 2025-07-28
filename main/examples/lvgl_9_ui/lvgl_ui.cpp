/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-11-28 17:07:50
 * @LastEditTime: 2025-07-28 14:06:45
 * @License: GPL 3.0
 */
#include "lvgl_ui.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_flash.h"

namespace Lvgl_Ui
{
    const System::Win_Home_App_Icon System::_win_home_app_icon_list[] =
        {
            {"Cit", &win_home_app_icon_cit_110x110px_rgb565a8},
            {"Lora", &win_home_app_icon_lora_110x110px_rgb565a8},
            {"Music", &win_home_app_icon_music_110x110px_rgb565a8},
    };

    const System::Win_Home_App_Icon System::_win_home_app_icon_fixed_list[] =
        {
            {"Camera", &win_home_app_icon_camera_110x110px_rgb565a8},
            {"Setings", &win_home_app_icon_setings_110x110px_rgb565a8},
    };

    System::Win_Cit_Test_Item System::_win_cit_test_item_list[] =
        {
            {"version information test", LV_SYMBOL_WARNING, 0xFFA500},
            {"touch test", LV_SYMBOL_REFRESH, 0x000000},
            {"screen color test", LV_SYMBOL_WARNING, 0xFFA500},
            {"vibration test", LV_SYMBOL_WARNING, 0xFFA500},
            {"speaker test", LV_SYMBOL_WARNING, 0xFFA500},
            {"microphone test", LV_SYMBOL_WARNING, 0xFFA500},
            {"imu test", LV_SYMBOL_WARNING, 0xFFA500},
            {"battery health test", LV_SYMBOL_WARNING, 0xFFA500},
            {"gps test", LV_SYMBOL_WARNING, 0xFFA500},
            {"ethernet test", LV_SYMBOL_WARNING, 0xFFA500},
            {"rtc test", LV_SYMBOL_WARNING, 0xFFA500},
            {"esp32c6 at test", LV_SYMBOL_WARNING, 0xFFA500},
            // {"sleep test", LV_SYMBOL_WARNING, 0xFFA500},
    };

    System::Device_Information System::_device_information_list[] =
        {
            {"chip model: ", ""},
            {"chip efuse mac:\n     ", ""},
            {"chip revision: ", ""},
            {"chip cores: ", ""},
            {"chip flash size: ", ""},
            {"chip flash features: ", ""},
            {"chip free heap size:\n     ", ""},
            {"espidf version:\n     ", ""},
            {"company: ", "lilygo"},
            {"board name: ", "t-display-p4"},
            {"software name: ", "lvgl_9_ui"},

#if defined CONFIG_SCREEN_TYPE_HI8561
            {"screen type: ", "hi8561"},
#elif defined CONFIG_SCREEN_TYPE_RM69A10
            {"screen type: ", "rm69a10"},
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

#if defined CONFIG_LCD_PIXEL_FORMAT_RGB565
            {"screen pixel format: ", "rgb565"},
#elif defined CONFIG_LCD_PIXEL_FORMAT_RGB888
            {"screen pixel format: ", "rgb888"},
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

#if defined CONFIG_CAMERA_TYPE_SC2336
            {"camera type: ", "sc2333"},
#elif defined CONFIG_CAMERA_TYPE_OV2710
            {"camera type: ", "ov2710"},
#elif defined CONFIG_CAMERA_TYPE_OV5645
            {"camera type: ", "ov5645"},
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

            {"firmware build date:\n     ", "202507281406"},
    };

    void System::begin()
    {
        _app_style.icon.edge_distance.height = std::min(_width, _height) / 5;
        _app_style.icon.edge_distance.width = _app_style.icon.edge_distance.height / 5;
        _app_style.icon.icon_distance.width = (_width - (_app_style.icon.edge_distance.width * 2) - (4 * APP_STYLE_ICON_WIDTH_HEIGHT)) / 3;
        _app_style.icon.icon_distance.height = APP_STYLE_ICON_WIDTH_HEIGHT + (APP_STYLE_ICON_WIDTH_HEIGHT / 1.5);
        _app_style.label.width = APP_STYLE_ICON_WIDTH_HEIGHT;
        _app_style.label.height = APP_STYLE_ICON_WIDTH_HEIGHT / 3;
        _app_style.icon.edge_distance_fixed.width = _app_style.icon.edge_distance.width + 40;
        _app_style.icon.edge_distance_fixed.height = 10;
        _app_style.icon.icon_distance.fixed_width = (_width - (_app_style.icon.edge_distance_fixed.width * 2) - (3 * APP_STYLE_ICON_WIDTH_HEIGHT)) / 2;

        _device_information_list[0].info = "esp32p4";

        uint8_t mac[6];
        esp_efuse_mac_get_default(mac);
        char mac_str[18];

        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        _device_information_list[1].info = mac_str;

        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        char revision_str[10];
        snprintf(revision_str, sizeof(revision_str), "v%d.%d", chip_info.revision / 100, chip_info.revision % 100);
        _device_information_list[2].info = revision_str;

        char cores_str[10];
        snprintf(cores_str, sizeof(cores_str), "%d", chip_info.cores);
        _device_information_list[3].info = cores_str;

        uint32_t flash_size;
        ESP_ERROR_CHECK(esp_flash_get_size(NULL, &flash_size));
        char flash_size_str[20];
        snprintf(flash_size_str, sizeof(flash_size_str), "%lu bytes", flash_size);
        _device_information_list[4].info = flash_size_str;

        _device_information_list[5].info = (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external";

        char free_heap_size_str[20];
        snprintf(free_heap_size_str, sizeof(free_heap_size_str), "%lu bytes", esp_get_free_heap_size());
        _device_information_list[6].info = free_heap_size_str;

        _device_information_list[7].info = esp_get_idf_version();

        init_win_home();
        // lv_screen_load(_registry.win.home.root);
        lv_screen_load_anim(_registry.win.home.root, LV_SCR_LOAD_ANIM_FADE_OUT, 500, 0, true);
    }

    System::Current_Win System::get_current_win(void)
    {
        return _current_win;
    }

    void System::set_time(Pcf8563x::Time time)
    {
        std::string week_str;
        switch (time.week)
        {
        case Pcf8563x::Week::SUNDAY:
            week_str = "Sun";
            break;
        case Pcf8563x::Week::MONDAY:
            week_str = "Mon";
            break;
        case Pcf8563x::Week::TUESDAY:
            week_str = "Tue";
            break;
        case Pcf8563x::Week::WEDNESDAY:
            week_str = "Wed";
            break;
        case Pcf8563x::Week::THURSDAY:
            week_str = "Thu";
            break;
        case Pcf8563x::Week::FRIDAY:
            week_str = "Fri";
            break;
        case Pcf8563x::Week::SATURDAY:
            week_str = "Sat";
            break;

        default:
            break;
        }

        _time.week = week_str;
        _time.year = static_cast<uint16_t>(time.year + 2000);
        _time.month = time.month;
        _time.day = time.day;
        _time.hour = time.hour;
        _time.minute = time.minute;
        _time.second = time.second;
    }

    void System::set_battery_level(uint16_t battery_level)
    {
        _battery_level = battery_level;
    }

    void System::set_wifi_connect_status(bool status)
    {
        _wifi_connect_status = status;
    }

    void System::add_event_cb_win_return_to_cit(lv_obj_t *obj)
    {
        lv_obj_add_event_cb(obj, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_GESTURE)
                                {
                                    lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());

                                    // 边缘检测以及左右滑动
                                    if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                    {
                                        self->set_vibration();
                                        self->init_win_cit();
                                        
                                        lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);

                                        self->_edge_touch_flag = false;
                                    }
                                } }, LV_EVENT_ALL, this);
    }

    void System::add_win_cit_test_item_pass_fail_button(lv_obj_t *parent)
    {
        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(parent);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);
    }

    void System::set_vibration(uint8_t vibration_count)
    {
        if (_device_vibration_callback != nullptr)
        {
            _device_vibration_callback(vibration_count);
        }
    }

    void System::set_speaker_test(void)
    {
        if (_win_cit_speaker_test_callback != nullptr)
        {
            _win_cit_speaker_test_callback();
        }
    }

    void System::set_microphone_test(bool status)
    {
        if (_win_cit_microphone_test_callback != nullptr)
        {
            _win_cit_microphone_test_callback(status);
        }
    }

    void System::set_adc_to_dac_switch_status(bool status)
    {
        if (_win_cit_adc_to_dac_switch_callback != nullptr)
        {
            _win_cit_adc_to_dac_switch_callback(status);
        }
    }

    void System::set_imu_test(bool status)
    {
        if (_win_cit_imu_test_callback != nullptr)
        {
            _win_cit_imu_test_callback(status);
        }
    }

    void System::set_gps_test(bool status)
    {
        if (_win_cit_gps_test_callback != nullptr)
        {
            _win_cit_gps_test_callback(status);
        }
    }

    void System::set_ethernet_test(bool status)
    {
        if (_win_cit_ethernet_test_callback != nullptr)
        {
            _win_cit_ethernet_test_callback(status);
        }
    }

    void System::set_esp32c6_at_test(bool status)
    {
        if (_win_cit_esp32c6_at_test_callback != nullptr)
        {
            _win_cit_esp32c6_at_test_callback(status);
        }
    }

    // void System::start_sleep_test(Sleep_Mode mode)
    // {
    //     if (_device_start_sleep_test_callback != nullptr)
    //     {
    //         _device_start_sleep_test_callback(mode);
    //     }
    // }

    void System::set_camera_status(bool status)
    {
        if (_win_camera_status_callback != nullptr)
        {
            _win_camera_status_callback(status);
        }
    }

    void System::init_win_home(void)
    {
        // 主界面
        _registry.win.home.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.home.root, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN);
#if defined CONFIG_SCREEN_TYPE_HI8561
        lv_obj_set_style_bg_image_src(_registry.win.home.root, GET_WALLPAPER_PATH("wallpaper_1_540x1168px.png"), (lv_style_selector_t)LV_PART_MAIN);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
        lv_obj_set_style_bg_image_src(_registry.win.home.root, GET_WALLPAPER_PATH("wallpaper_1_568x1232px.png"), (lv_style_selector_t)LV_PART_MAIN);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif
        lv_obj_set_size(_registry.win.home.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.home.root, LV_SCROLLBAR_MODE_OFF);

        // 页
        lv_obj_t *tileview = lv_tileview_create(_registry.win.home.root);
        lv_obj_set_size(tileview, _width, _height - (APP_STYLE_ICON_WIDTH_HEIGHT + 40));
        lv_obj_set_style_bg_opa(tileview, LV_OPA_0, (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_ACTIVE);

        // 页1
        lv_obj_t *tileview_tile_1 = lv_tileview_add_tile(tileview, 0, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));

        lv_obj_t *image_button[sizeof(_win_home_app_icon_list) / sizeof(Win_Home_App_Icon)];
        for (uint16_t i = 0; i < sizeof(_win_home_app_icon_list) / sizeof(Win_Home_App_Icon); i++)
        {
            // 定义需要过渡的属性：缩放 X 和缩放 Y
            static const lv_style_prop_t tr_prop[] = {LV_STYLE_TRANSFORM_SCALE_X, LV_STYLE_TRANSFORM_SCALE_Y, 0};
            // 创建过渡描述符
            static lv_style_transition_dsc_t tr;
            lv_style_transition_dsc_init(&tr, tr_prop, lv_anim_path_ease_out, 100, 0, NULL);
            // 创建默认样式
            static lv_style_t style_def;
            lv_style_init(&style_def);
            // 设置过渡效果
            lv_style_set_transition(&style_def, &tr);
            // 设置缩放的中心点为对象的中心
            lv_style_set_transform_pivot_x(&style_def, LV_PCT(50)); // X 方向的中心点
            lv_style_set_transform_pivot_y(&style_def, LV_PCT(50)); // Y 方向的中心点
            // 创建按下时的样式
            static lv_style_t style_pr;
            lv_style_init(&style_pr);
            // 设置按下时的缩放比例
            lv_style_set_transform_scale_x(&style_pr, 256 * 0.9);
            lv_style_set_transform_scale_y(&style_pr, 256 * 0.9);
            // 设置缩放的中心点为对象的中心
            lv_style_set_transform_pivot_x(&style_pr, LV_PCT(50)); // X 方向的中心点
            lv_style_set_transform_pivot_y(&style_pr, LV_PCT(50)); // Y 方向的中心点

            // 创建图像按钮
            image_button[i] = lv_imagebutton_create(tileview_tile_1);
            lv_imagebutton_set_src(image_button[i], LV_IMAGEBUTTON_STATE_RELEASED, NULL, _win_home_app_icon_list[i].image, NULL);
            lv_imagebutton_set_src(image_button[i], LV_IMAGEBUTTON_STATE_PRESSED, NULL, _win_home_app_icon_list[i].image, NULL);
            lv_imagebutton_set_src(image_button[i], LV_IMAGEBUTTON_STATE_DISABLED, NULL, _win_home_app_icon_list[i].image, NULL);
            // 应用样式
            lv_obj_add_style(image_button[i], &style_def, (lv_style_selector_t)LV_PART_MAIN);
            lv_obj_add_style(image_button[i], &style_pr, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
            // 设置按钮位置
            lv_obj_align(image_button[i], LV_ALIGN_TOP_LEFT, _app_style.icon.edge_distance.width + (APP_STYLE_ICON_WIDTH_HEIGHT * i) + (_app_style.icon.icon_distance.width * i), _app_style.icon.edge_distance.height + 300);

            lv_obj_t *image_button_label = lv_label_create(tileview_tile_1);
            lv_label_set_text(image_button_label, _win_home_app_icon_list[i].name.c_str());
            lv_obj_set_style_text_align(image_button_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(image_button_label, &lv_font_montserrat_22, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
            lv_obj_set_size(image_button_label, _app_style.label.width, _app_style.label.height);
            lv_obj_set_style_text_color(image_button_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
            lv_obj_align_to(image_button_label, image_button[i], LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        }

        lv_obj_add_event_cb(image_button[0], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->init_win_cit();

                                lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_FADE_OUT, 500, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(image_button[1], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->init_win_lora();

                                lv_screen_load_anim(self->_registry.win.lora.root, LV_SCR_LOAD_ANIM_FADE_OUT, 500, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(image_button[2], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->init_win_music();

                                lv_screen_load(self->_registry.win.music.root);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        // 时钟
        _registry.win.home.clock.time_label = lv_label_create(tileview_tile_1);
        char buffer_time[10];
        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d", _time.hour, _time.minute);
        lv_label_set_text(_registry.win.home.clock.time_label, buffer_time);
        lv_obj_set_style_text_align(_registry.win.home.clock.time_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.home.clock.time_label, &lvgl_font_lineseedkr_rg_120, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.home.clock.time_label, 400, 110);
        lv_obj_set_style_text_color(_registry.win.home.clock.time_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.home.clock.time_label, tileview_tile_1, LV_ALIGN_TOP_LEFT, 10, 90);

        _registry.win.home.clock.month_label = lv_label_create(tileview_tile_1);

        std::string month_str = "null";
        switch (_time.month)
        {
        case 1:
            month_str = "January";
            break;
        case 2:
            month_str = "February";
            break;
        case 3:
            month_str = "March";
            break;
        case 4:
            month_str = "April";
            break;
        case 5:
            month_str = "May";
            break;
        case 6:
            month_str = "June";
            break;
        case 7:
            month_str = "July";
            break;
        case 8:
            month_str = "August";
            break;
        case 9:
            month_str = "September";
            break;
        case 10:
            month_str = "October";
            break;
        case 11:
            month_str = "November";
            break;
        case 12:
            month_str = "December";
            break;

        default:
            break;
        }
        char buffer_month[20];
        snprintf(buffer_month, sizeof(buffer_month), "%s %dth", month_str.c_str(), _time.day);
        lv_label_set_text(_registry.win.home.clock.month_label, buffer_month);
        lv_obj_set_style_text_align(_registry.win.home.clock.month_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.home.clock.month_label, &lvgl_font_lineseedkr_th_60, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.home.clock.month_label, 400, 70);
        lv_obj_set_style_text_color(_registry.win.home.clock.month_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.home.clock.month_label, _registry.win.home.clock.time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 10, 0);

        _registry.win.home.clock.week_label = lv_label_create(tileview_tile_1);
        lv_label_set_text(_registry.win.home.clock.week_label, _time.week.c_str());
        lv_obj_set_style_text_align(_registry.win.home.clock.week_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.home.clock.week_label, &lvgl_font_lineseedkr_th_60, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.home.clock.week_label, 400, 50);
        lv_obj_set_style_text_color(_registry.win.home.clock.week_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.home.clock.week_label, _registry.win.home.clock.month_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        // 页2
        lv_obj_t *tileview_tile_2 = lv_tileview_add_tile(tileview, 1, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));

        // 固定页
        lv_obj_t *tileview_fixed = lv_tileview_create(_registry.win.home.root);
        lv_obj_set_size(tileview_fixed, _width, APP_STYLE_ICON_WIDTH_HEIGHT + 50);
        lv_obj_set_style_bg_color(tileview_fixed, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tileview_fixed, LV_OPA_30, (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_align(tileview_fixed, LV_ALIGN_BOTTOM_MID, 0, 0);

        // 页1
        lv_obj_t *tileview_fixed_tile_1 = lv_tileview_add_tile(tileview_fixed, 0, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));

        lv_obj_t *image_button_fixed[sizeof(_win_home_app_icon_fixed_list) / sizeof(Win_Home_App_Icon)];
        for (uint16_t i = 0; i < sizeof(_win_home_app_icon_fixed_list) / sizeof(Win_Home_App_Icon); i++)
        {
            // 定义需要过渡的属性：缩放 X 和缩放 Y
            static const lv_style_prop_t tr_prop[] = {LV_STYLE_TRANSFORM_SCALE_X, LV_STYLE_TRANSFORM_SCALE_Y, 0};
            // 创建过渡描述符
            static lv_style_transition_dsc_t tr;
            lv_style_transition_dsc_init(&tr, tr_prop, lv_anim_path_ease_out, 100, 0, NULL);
            // 创建默认样式
            static lv_style_t style_def;
            lv_style_init(&style_def);
            // 设置过渡效果
            lv_style_set_transition(&style_def, &tr);
            // 设置缩放的中心点为对象的中心
            lv_style_set_transform_pivot_x(&style_def, LV_PCT(50)); // X 方向的中心点
            lv_style_set_transform_pivot_y(&style_def, LV_PCT(50)); // Y 方向的中心点
            // 创建按下时的样式
            static lv_style_t style_pr;
            lv_style_init(&style_pr);
            // 设置按下时的缩放比例
            lv_style_set_transform_scale_x(&style_pr, 256 * 0.9);
            lv_style_set_transform_scale_y(&style_pr, 256 * 0.9);
            // 设置缩放的中心点为对象的中心
            lv_style_set_transform_pivot_x(&style_pr, LV_PCT(50)); // X 方向的中心点
            lv_style_set_transform_pivot_y(&style_pr, LV_PCT(50)); // Y 方向的中心点

            // 创建图像按钮
            image_button_fixed[i] = lv_imagebutton_create(tileview_fixed_tile_1);
            lv_imagebutton_set_src(image_button_fixed[i], LV_IMAGEBUTTON_STATE_RELEASED, NULL, _win_home_app_icon_fixed_list[i].image, NULL);
            lv_imagebutton_set_src(image_button_fixed[i], LV_IMAGEBUTTON_STATE_PRESSED, NULL, _win_home_app_icon_fixed_list[i].image, NULL);
            lv_imagebutton_set_src(image_button_fixed[i], LV_IMAGEBUTTON_STATE_DISABLED, NULL, _win_home_app_icon_fixed_list[i].image, NULL);
            // 应用样式
            lv_obj_add_style(image_button_fixed[i], &style_def, (lv_style_selector_t)LV_PART_MAIN);
            lv_obj_add_style(image_button_fixed[i], &style_pr, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
            // 设置按钮位置
            lv_obj_align(image_button_fixed[i], LV_ALIGN_TOP_LEFT, _app_style.icon.edge_distance_fixed.width + (APP_STYLE_ICON_WIDTH_HEIGHT * i) + (_app_style.icon.icon_distance.fixed_width * i),
                         _app_style.icon.edge_distance_fixed.height);

            lv_obj_t *image_button_label = lv_label_create(tileview_fixed_tile_1);
            lv_label_set_text(image_button_label, _win_home_app_icon_fixed_list[i].name.c_str());
            lv_obj_set_style_text_align(image_button_label, LV_TEXT_ALIGN_CENTER, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(image_button_label, &lv_font_montserrat_22, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
            lv_obj_set_size(image_button_label, _app_style.label.width, _app_style.label.height);
            lv_obj_set_style_text_color(image_button_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
            lv_obj_align_to(image_button_label, image_button_fixed[i], LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        }

        lv_obj_add_event_cb(image_button_fixed[0], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->init_win_camera();

                                // lv_screen_load_anim(self->_registry.win.camera.root, LV_SCR_LOAD_ANIM_FADE_OUT, 500, 0, true);
                                lv_screen_load(self->_registry.win.camera.root);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.home.root);

        lv_obj_update_layout(_registry.win.home.root);

        _current_win = Current_Win::HOME;
    }

    void System::status_bar_time_update(void)
    {
        char buffer_time[10];
        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d", _time.hour, _time.minute);
        lv_label_set_text(_registry.status_bar.time_label, buffer_time);
    }

    void System::status_bar_battery_level_update(void)
    {
        if (_battery_level == 100)
        {
            lv_label_set_text(_registry.status_bar.battery_icon, LV_SYMBOL_BATTERY_FULL);
        }
        else if ((_battery_level >= 66) && (_battery_level < 100))
        {
            lv_label_set_text(_registry.status_bar.battery_icon, LV_SYMBOL_BATTERY_3);
        }
        else if ((_battery_level >= 33) && (_battery_level < 66))
        {
            lv_label_set_text(_registry.status_bar.battery_icon, LV_SYMBOL_BATTERY_2);
        }
        else if ((_battery_level > 0) && (_battery_level < 33))
        {
            lv_label_set_text(_registry.status_bar.battery_icon, LV_SYMBOL_BATTERY_1);
        }
        else
        {
            lv_label_set_text(_registry.status_bar.battery_icon, LV_SYMBOL_BATTERY_EMPTY);
        }
    }

    void System::status_bar_wifi_connect_status_update(void)
    {
        if (_wifi_connect_status == true)
        {
            // 显示wifi信号强度图标
            lv_obj_remove_flag(_registry.status_bar.wifi_signal_icon, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            // 隐藏wifi信号强度图标
            lv_obj_add_flag(_registry.status_bar.wifi_signal_icon, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void System::win_home_time_update(void)
    {
        char buffer_time[10];
        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d", _time.hour, _time.minute);
        lv_label_set_text(_registry.win.home.clock.time_label, buffer_time);

        std::string month_str = "null";
        switch (_time.month)
        {
        case 1:
            month_str = "January";
            break;
        case 2:
            month_str = "February";
            break;
        case 3:
            month_str = "March";
            break;
        case 4:
            month_str = "April";
            break;
        case 5:
            month_str = "May";
            break;
        case 6:
            month_str = "June";
            break;
        case 7:
            month_str = "July";
            break;
        case 8:
            month_str = "August";
            break;
        case 9:
            month_str = "September";
            break;
        case 10:
            month_str = "October";
            break;
        case 11:
            month_str = "November";
            break;
        case 12:
            month_str = "December";
            break;

        default:
            break;
        }
        char buffer_month[20];
        snprintf(buffer_month, sizeof(buffer_month), "%s %dth", month_str.c_str(), _time.day);
        lv_label_set_text(_registry.win.home.clock.month_label, buffer_month);

        lv_label_set_text(_registry.win.home.clock.week_label, _time.week.c_str());
    }

    void System::init_status_bar(lv_obj_t *parent)
    {
        // 创建一个容器来放置状态栏内容
        _registry.status_bar.root = lv_obj_create(parent);
        lv_obj_set_size(_registry.status_bar.root, LV_HOR_RES, 50);                                                // 设置状态栏的宽度和高度
        lv_obj_set_style_bg_opa(_registry.status_bar.root, LV_OPA_20, (lv_style_selector_t)LV_PART_MAIN);          // 设置背景透明度
        lv_obj_set_style_bg_color(_registry.status_bar.root, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色
        lv_obj_set_style_border_width(_registry.status_bar.root, 0, (lv_style_selector_t)LV_PART_MAIN);            // 移除边框
        lv_obj_align(_registry.status_bar.root, LV_ALIGN_TOP_MID, 0, 0);                                           // 将状态栏对齐到顶部中间
        // lv_obj_set_scrollbar_mode(_registry.status_bar.root, LV_SCROLLBAR_MODE_OFF);
        lv_obj_remove_flag(_registry.status_bar.root, LV_OBJ_FLAG_SCROLLABLE); // 禁止滚动
        lv_obj_remove_flag(_registry.status_bar.root, LV_OBJ_FLAG_CLICKABLE);  // 禁止触摸

        // 创建时间标签
        _registry.status_bar.time_label = lv_label_create(_registry.status_bar.root);
        lv_obj_set_style_text_color(_registry.status_bar.time_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.status_bar.time_label, &lv_font_montserrat_22, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        char buffer_time[10];
        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d", _time.hour, _time.minute);
        lv_label_set_text(_registry.status_bar.time_label, buffer_time);
        lv_obj_align(_registry.status_bar.time_label, LV_ALIGN_LEFT_MID, 0, 0);

        // 创建电池图标
        _registry.status_bar.battery_icon = lv_label_create(_registry.status_bar.root);
        lv_obj_set_style_text_color(_registry.status_bar.battery_icon, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.status_bar.battery_icon, &lv_font_montserrat_22, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        status_bar_battery_level_update();
        lv_obj_align(_registry.status_bar.battery_icon, LV_ALIGN_RIGHT_MID, 0, 0); // 将电池图标对齐到右中间

        // 创建wifi信号强度图标
        _registry.status_bar.wifi_signal_icon = lv_label_create(_registry.status_bar.root);
        lv_obj_set_style_text_color(_registry.status_bar.wifi_signal_icon, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.status_bar.wifi_signal_icon, &lv_font_montserrat_22, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.status_bar.wifi_signal_icon, LV_SYMBOL_WIFI);
        lv_obj_align(_registry.status_bar.wifi_signal_icon, LV_ALIGN_RIGHT_MID, -40, 0); // 将信号强度图标对齐到右中间
        status_bar_wifi_connect_status_update();
    }

    void System::init_win_cit(void)
    {
        // 主界面
        _registry.win.cit.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.root);
        lv_label_set_text(title_label, "CIT");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建列表
        lv_obj_t *list = lv_list_create(_registry.win.cit.root);
        lv_obj_set_size(list, _width, _height - 50 - 80);
        lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_pad_left(list, 20, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(list, 20, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_radius(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);

        lv_obj_t *list_button[sizeof(_win_cit_test_item_list) / sizeof(Win_Cit_Test_Item)];
        for (uint16_t i = 0; i < sizeof(_win_cit_test_item_list) / sizeof(Win_Cit_Test_Item); i++)
        {
            list_button[i] = lv_list_add_button(list, _win_cit_test_item_list[i].symbol.c_str(), _win_cit_test_item_list[i].name.c_str());
            lv_obj_set_style_text_color(list_button[i], lv_color_hex(_win_cit_test_item_list[i].color), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(list_button[i], &lv_font_montserrat_30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        }

        lv_obj_add_event_cb(list_button[0], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 0;

                                self->init_win_cit_version_information_test();

                                lv_screen_load_anim(self->_registry.win.cit.version_information_test, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[1], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 1;

                                self->init_win_cit_touch_test();

                                lv_screen_load_anim(self->_registry.win.cit.touch_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);
        lv_obj_add_event_cb(list_button[2], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 2;

                                self->init_win_cit_screen_color_test();

                                lv_screen_load_anim(self->_registry.win.cit.screen_color_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[3], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 3;

                                self->init_win_cit_vibration_test();

                                lv_screen_load_anim(self->_registry.win.cit.vibration_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[4], [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    switch (code)
                                    {
                                    case LV_EVENT_CLICKED:
                                    self->_registry.win.cit.current_test_item_index = 4;
    
                                    self->init_win_cit_speaker_test();
    
                                    lv_screen_load_anim(self->_registry.win.cit.speaker_test, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                    break;
                                    default:
                                    break;
                                    } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[5], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 5;

                                self->init_win_cit_microphone_test();

                                lv_screen_load_anim(self->_registry.win.cit.microphone_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[6], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 6;

                                self->init_win_cit_imu_test();

                                lv_screen_load_anim(self->_registry.win.cit.imu_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[7], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 7;

                                self->init_win_cit_battery_health_test();

                                lv_screen_load_anim(self->_registry.win.cit.battery_health_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[8], [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    switch (code)
                                    {
                                    case LV_EVENT_CLICKED:
                                    self->_registry.win.cit.current_test_item_index = 8;
    
                                    self->init_win_cit_gps_test();
    
                                    lv_screen_load_anim(self->_registry.win.cit.gps_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                    break;
                                    default:
                                    break;
                                    } }, LV_EVENT_ALL, this);
        lv_obj_add_event_cb(list_button[9], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 9;

                                self->init_win_cit_ethernet_test();

                                lv_screen_load_anim(self->_registry.win.cit.ethernet_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[10], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 10;

                                self->init_win_cit_rtc_test();

                                lv_screen_load_anim(self->_registry.win.cit.rtc_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(list_button[11], [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                self->_registry.win.cit.current_test_item_index = 11;

                                self->init_win_cit_esp32c6_at_test();

                                lv_screen_load_anim(self->_registry.win.cit.esp32c6_at_test.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        // lv_obj_add_event_cb(list_button[12], [](lv_event_t *e)
        //                     {
        //                         System *self = static_cast<System *>(lv_event_get_user_data(e));
        //                         lv_event_code_t code = lv_event_get_code(e);

        //                         switch (code)
        //                         {
        //                         case LV_EVENT_CLICKED:
        //                         self->_registry.win.cit.current_test_item_index = 12;

        //                         self->init_win_cit_sleep_test();

        //                         lv_screen_load_anim(self->_registry.win.cit.sleep_test, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
        //                         break;
        //                         default:
        //                         break;
        //                         } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.root, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_GESTURE)
                                {
                                    lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());

                                    // 边缘检测以及左右滑动
                                    if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                    {
                                        self->set_vibration();
                                        self->init_win_home();
                                        
                                        lv_screen_load_anim(self->_registry.win.home.root, LV_SCR_LOAD_ANIM_FADE_OUT, 100, 0, true);

                                        self->_edge_touch_flag = false;
                                    }
                                } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.root);

        lv_obj_update_layout(_registry.win.cit.root);

        _current_win = Current_Win::CIT;
    }

    void System::init_win_cit_version_information_test(void)
    {
        _registry.win.cit.version_information_test = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.version_information_test, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.version_information_test, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.version_information_test, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.version_information_test);
        lv_label_set_text(title_label, "Version Information Test");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建列表
        lv_obj_t *list = lv_list_create(_registry.win.cit.version_information_test);
        lv_obj_set_size(list, _width, _height - 50 - 80 - 140);
        lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_pad_left(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_radius(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);

        for (uint16_t i = 0; i < sizeof(_device_information_list) / sizeof(Device_Information); i++)
        {
            lv_obj_t *list_button = lv_list_add_button(list, NULL, (_device_information_list[i].name + _device_information_list[i].info).c_str());

            lv_obj_set_style_text_font(list_button, &lv_font_montserrat_28, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        }

        add_win_cit_test_item_pass_fail_button(_registry.win.cit.version_information_test);

        // // 创建Button Matrix按键
        // static const char *btnm_map[] = {"PASS", "FAIL"};
        // _registry.win.cit.judgment_button = lv_buttonmatrix_create(_registry.win.cit.version_information_test);
        // lv_buttonmatrix_set_map(_registry.win.cit.judgment_button, btnm_map);
        // lv_obj_set_size(_registry.win.cit.judgment_button, _width, 140);
        // lv_obj_align(_registry.win.cit.judgment_button, LV_ALIGN_BOTTOM_MID, 0, 0);
        // lv_obj_set_style_text_font(_registry.win.cit.judgment_button, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        // lv_obj_set_style_border_width(_registry.win.cit.judgment_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        // lv_obj_set_style_shadow_width(_registry.win.cit.judgment_button, 0, (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_DEFAULT);

        // lv_obj_add_event_cb(_registry.win.cit.judgment_button, [](lv_event_t *e)
        //                     {
        //                         System *self = static_cast<System *>(lv_event_get_user_data(e));
        //                         lv_event_code_t code = lv_event_get_code(e);
        //                         uint16_t id = lv_buttonmatrix_get_selected_button(self->_registry.win.cit.judgment_button);
        //                         const char *txt = lv_buttonmatrix_get_button_text(self->_registry.win.cit.judgment_button, id);

        //                         switch (code)
        //                         {
        //                         case LV_EVENT_CLICKED:

        //                             if (strcmp(txt, "PASS") == 0)
        //                             {
        //                                 _win_cit_test_item_list[0].symbol = LV_SYMBOL_OK;
        //                                 _win_cit_test_item_list[0].color = 0x008B45;
        //                             }
        //                             else if (strcmp(txt, "FAIL") == 0)
        //                             {
        //                                 _win_cit_test_item_list[0].symbol = LV_SYMBOL_CLOSE;
        //                                 _win_cit_test_item_list[0].color = 0xEE2C2C;
        //                             }

        //                             self->init_win_cit();

        //                             lv_screen_load_anim(self->_registry.win.cit.root,  LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);

        //                         break;
        //                         default:
        //                         break;
        //                         } }, LV_EVENT_ALL, this);

        add_event_cb_win_return_to_cit(_registry.win.cit.version_information_test);

        init_status_bar(_registry.win.cit.version_information_test);

        lv_obj_update_layout(_registry.win.cit.version_information_test);
    }

    void System::init_win_cit_touch_test(void)
    {
        _registry.win.cit.touch_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.touch_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.touch_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.touch_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建画布并初始化调色板
        _registry.win.cit.touch_test.canvas = lv_canvas_create(_registry.win.cit.touch_test.root);
        lv_canvas_set_buffer(_registry.win.cit.touch_test.canvas, _lv_color_win_draw_buf.get(), _width, _height, LV_COLOR_FORMAT_RGB565);
        lv_canvas_fill_bg(_registry.win.cit.touch_test.canvas, lv_color_hex(0xCCCCCC), LV_OPA_COVER);
        lv_obj_center(_registry.win.cit.touch_test.canvas);

        lv_canvas_init_layer(_registry.win.cit.touch_test.canvas, &_registry.win.cit.touch_test.layer);

        _registry.win.cit.touch_test.draw_x.clear();
        _registry.win.cit.touch_test.draw_y.clear();

        lv_obj_add_event_cb(_registry.win.cit.touch_test.root, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_PRESSING:
                                {
                                    lv_point_t point;
                                    lv_indev_get_point(lv_indev_active(), &point);

                                    // printf("touch x: %ld y: %ld\n", point.x, point.y);

                                    // 在画布上绘制点
                                    // lv_canvas_set_px(canvas, point.x, point.y, lv_palette_main(LV_PALETTE_RED), LV_OPA_COVER);

                                    // printf("touch finger: %d edge touch flag: %d\n", self->_touch_point.finger_count, self->_touch_point.edge_touch_flag);
                                    // for (uint8_t i = 0; i < self->_touch_point.info.size(); i++)
                                    // {
                                    //     printf("touch num [%d] x: %d y: %d p: %d\n", i + 1, self->_touch_point.info[i].x, self->_touch_point.info[i].y, self->_touch_point.info[i].pressure_value);
                                    // }

                                    // 将触摸数据格式化为字符串
                                    std::string touch_data = "touch data:\n";
                                    touch_data += "finger count: " + std::to_string(self->_touch_point.finger_count) + "\n";
                                    touch_data += "edge touch flag: " + std::to_string(self->_touch_point.edge_touch_flag) + "\n";

                                    for (uint8_t i = 0; i < self->_touch_point.info.size(); i++)
                                    {
                                        touch_data += "touch [" + std::to_string(i + 1) + "] x: " + std::to_string(self->_touch_point.info[i].x) +
                                                      " y: " + std::to_string(self->_touch_point.info[i].y) +
                                                      " p: " + std::to_string(self->_touch_point.info[i].pressure_value) + "\n";
                                    }
                                    // 更新触摸数据的标签
                                    lv_label_set_text(self->_registry.win.cit.touch_test.touch_data_label, touch_data.c_str());
                                    lv_obj_align(self->_registry.win.cit.touch_test.touch_data_label, LV_ALIGN_CENTER, 0, 0);

                                    self->_registry.win.cit.touch_test.draw_x.push_back(point.x);
                                    self->_registry.win.cit.touch_test.draw_y.push_back(point.y);

                                    if ((self->_registry.win.cit.touch_test.draw_x.size() >= 2) && (self->_registry.win.cit.touch_test.draw_y.size() >= 2))
                                    {
                                        lv_draw_line_dsc_t dsc;
                                        lv_draw_line_dsc_init(&dsc);
                                        dsc.color = lv_palette_main(LV_PALETTE_RED);
                                        dsc.width = 4;
                                        dsc.round_end = 1;
                                        dsc.round_start = 1;
                                        dsc.p1.x = self->_registry.win.cit.touch_test.draw_x[0];
                                        dsc.p1.y = self->_registry.win.cit.touch_test.draw_y[0];
                                        dsc.p2.x = self->_registry.win.cit.touch_test.draw_x[1];
                                        dsc.p2.y = self->_registry.win.cit.touch_test.draw_y[1];
                                        lv_draw_line(&self->_registry.win.cit.touch_test.layer, &dsc);

                                        lv_canvas_finish_layer(self->_registry.win.cit.touch_test.canvas, &self->_registry.win.cit.touch_test.layer);

                                        self->_registry.win.cit.touch_test.draw_x.erase(self->_registry.win.cit.touch_test.draw_x.begin());
                                        self->_registry.win.cit.touch_test.draw_y.erase(self->_registry.win.cit.touch_test.draw_y.begin());
                                    }
                                }
                                    break;

                                case LV_EVENT_RELEASED:
                                    self->_registry.win.cit.touch_test.draw_x.clear();
                                    self->_registry.win.cit.touch_test.draw_y.clear();
                                break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建一个标签用于显示触摸点数据
        _registry.win.cit.touch_test.touch_data_label = lv_label_create(_registry.win.cit.touch_test.root);
        lv_obj_set_style_text_color(_registry.win.cit.touch_test.touch_data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.touch_test.touch_data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.touch_test.touch_data_label, "touch data:");
        lv_obj_align(_registry.win.cit.touch_test.touch_data_label, LV_ALIGN_CENTER, 0, 0);

        add_event_cb_win_return_to_cit(_registry.win.cit.touch_test.root);

        init_status_bar(_registry.win.cit.touch_test.root);

        lv_obj_update_layout(_registry.win.cit.touch_test.root);

        _current_win = Current_Win::CIT_TOUCH_TEST;
    }

    void System::init_win_cit_screen_color_test(void)
    {
        // 主界面
        _registry.win.cit.screen_color_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.screen_color_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.screen_color_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.screen_color_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.screen_color_test.root);
        lv_label_set_text(title_label, "Screen Color");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.screen_color_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建START按键
        lv_obj_t *start_button = lv_button_create(container);
        lv_obj_set_size(start_button, 150, 80);
        lv_obj_align(start_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(start_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(start_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影
        lv_obj_set_style_bg_color(start_button, lv_color_hex(0xFF6A6A), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(start_button, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *start_label = lv_label_create(start_button);
        lv_obj_set_style_text_font(start_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(start_label, "START");
        lv_obj_center(start_label);

        lv_obj_add_event_cb(start_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:

                                    self->_registry.win.cit.screen_color_test.color_change_count = 0;

                                    self->init_win_cit_screen_color_test_start_color_test();

                                    lv_screen_load(self->_registry.win.cit.screen_color_test.start_color_test);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        add_win_cit_test_item_pass_fail_button(_registry.win.cit.screen_color_test.root);

        add_event_cb_win_return_to_cit(_registry.win.cit.screen_color_test.root);

        init_status_bar(_registry.win.cit.screen_color_test.root);

        lv_obj_update_layout(_registry.win.cit.screen_color_test.root);
    }

    void System::init_win_cit_screen_color_test_start_color_test(void)
    {
        // 主界面
        _registry.win.cit.screen_color_test.start_color_test = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.screen_color_test.start_color_test, lv_color_hex(0xFF0000), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.screen_color_test.start_color_test, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.screen_color_test.start_color_test, LV_SCROLLBAR_MODE_OFF);
        const uint32_t color_list[4] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
        lv_obj_add_event_cb(_registry.win.cit.screen_color_test.start_color_test, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                    case LV_EVENT_CLICKED:
                                    self->_registry.win.cit.screen_color_test.color_change_count++;

                                    switch (self->_registry.win.cit.screen_color_test.color_change_count)
                                    {
                                    case 1:
                                        lv_obj_set_style_bg_color(self->_registry.win.cit.screen_color_test.start_color_test,lv_color_hex(0x00FF00),(lv_style_selector_t)LV_PART_MAIN);
                                    break;
                                    case 2:
                                        lv_obj_set_style_bg_color(self->_registry.win.cit.screen_color_test.start_color_test,lv_color_hex(0x00FF00),(lv_style_selector_t)LV_PART_MAIN);
                                    break;
                                    case 3:
                                        lv_obj_set_style_bg_color(self->_registry.win.cit.screen_color_test.start_color_test,lv_color_hex(0x0000FF),(lv_style_selector_t)LV_PART_MAIN);
                                    break;
                                    case 4:
                                        lv_obj_set_style_bg_color(self->_registry.win.cit.screen_color_test.start_color_test,lv_color_hex(0xFFFFFF),(lv_style_selector_t)LV_PART_MAIN);
                                    break;
#if defined CONFIG_SCREEN_TYPE_HI8561
                                    case 5:
                                    lv_obj_set_style_bg_image_src(self->_registry.win.cit.screen_color_test.start_color_test, GET_WALLPAPER_PATH("wallpaper_2_540x1168px.png"), (lv_style_selector_t)LV_PART_MAIN);
                                    break;
                                    case 6:
                                    lv_obj_set_style_bg_image_src(self->_registry.win.cit.screen_color_test.start_color_test, GET_WALLPAPER_PATH("wallpaper_3_540x1168px.png"), (lv_style_selector_t)LV_PART_MAIN);
                                    break;
                                    case 7:
                                    lv_obj_set_style_bg_image_src(self->_registry.win.cit.screen_color_test.start_color_test, GET_WALLPAPER_PATH("wallpaper_4_540x1168px.png"), (lv_style_selector_t)LV_PART_MAIN);
                                    break;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
                                    case 5:
                                    lv_obj_set_style_bg_image_src(self->_registry.win.cit.screen_color_test.start_color_test, GET_WALLPAPER_PATH("wallpaper_2_568x1232px.png"), (lv_style_selector_t)LV_PART_MAIN);
                                    break;
                                    case 6:
                                    lv_obj_set_style_bg_image_src(self->_registry.win.cit.screen_color_test.start_color_test, GET_WALLPAPER_PATH("wallpaper_3_568x1232px.png"), (lv_style_selector_t)LV_PART_MAIN);
                                    break;
                                    case 7:
                                    lv_obj_set_style_bg_image_src(self->_registry.win.cit.screen_color_test.start_color_test, GET_WALLPAPER_PATH("wallpaper_4_568x1232px.png"), (lv_style_selector_t)LV_PART_MAIN);
                                    break;
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

                                    case 8:
                                        self->init_win_cit_screen_color_test();

                                        lv_screen_load_anim(self->_registry.win.cit.screen_color_test.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;

                                    default:
                                        break;
                                    }


                                    break;
                                
                                default:
                                    break;
                                }

                                if (code == LV_EVENT_GESTURE)
                                {
                                lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());

                                // 边缘检测以及左右滑动
                                if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                {
                                    self->set_vibration();
                                    self->init_win_cit_screen_color_test();

                                    lv_screen_load_anim(self->_registry.win.cit.screen_color_test.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);

                                    self->_edge_touch_flag = false;
                                }
                                } }, LV_EVENT_ALL, this);

        lv_obj_update_layout(_registry.win.cit.screen_color_test.start_color_test);
    }

    void System::init_win_cit_vibration_test(void)
    {
        // 主界面
        _registry.win.cit.vibration_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.vibration_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.vibration_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.vibration_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.vibration_test.root);
        lv_label_set_text(title_label, "Vibration");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.vibration_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建一个标签用于显示振动数据
        _registry.win.cit.vibration_test.data_label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.vibration_test.data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.vibration_test.data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.vibration_test.data_label, "vibration data:");
        lv_obj_align(_registry.win.cit.vibration_test.data_label, LV_ALIGN_TOP_MID, 0, 300);

        // 创建START F0按键
        lv_obj_t *start_button = lv_button_create(container);
        lv_obj_set_size(start_button, 200, 80);
        lv_obj_align(start_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(start_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(start_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影
        lv_obj_set_style_bg_color(start_button, lv_color_hex(0xFF6A6A), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(start_button, LV_ALIGN_CENTER, 0, 300);

        lv_obj_t *start_label = lv_label_create(start_button);
        lv_obj_set_style_text_font(start_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(start_label, "START F0");
        lv_obj_center(start_label);

        lv_obj_add_event_cb(start_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:

                                    self->set_vibration(-1);//启动振动F0校验
                                    
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        add_win_cit_test_item_pass_fail_button(_registry.win.cit.vibration_test.root);

        add_event_cb_win_return_to_cit(_registry.win.cit.vibration_test.root);

        init_status_bar(_registry.win.cit.vibration_test.root);

        lv_obj_update_layout(_registry.win.cit.vibration_test.root);

        _current_win = Current_Win::CIT_VIBRATION_TEST;
    }

    void System::init_win_cit_speaker_test(void)
    {
        // 主界面
        _registry.win.cit.speaker_test = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.speaker_test, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.speaker_test, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.speaker_test, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.speaker_test);
        lv_label_set_text(title_label, "Speaker");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.speaker_test);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建START按键
        lv_obj_t *start_button = lv_button_create(container);
        lv_obj_set_size(start_button, 250, 80);
        lv_obj_align(start_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(start_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(start_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影
        lv_obj_set_style_bg_color(start_button, lv_color_hex(0xFF6A6A), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(start_button, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *start_label = lv_label_create(start_button);
        lv_obj_set_style_text_font(start_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(start_label, "START PLAY");
        lv_obj_center(start_label);

        lv_obj_add_event_cb(start_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_speaker_test();

                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        add_win_cit_test_item_pass_fail_button(_registry.win.cit.speaker_test);

        add_event_cb_win_return_to_cit(_registry.win.cit.speaker_test);

        init_status_bar(_registry.win.cit.speaker_test);

        lv_obj_update_layout(_registry.win.cit.speaker_test);
    }

    void System::init_win_cit_microphone_test(void)
    {
        // 主界面
        _registry.win.cit.microphone_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.microphone_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.microphone_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.microphone_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.microphone_test.root);
        lv_label_set_text(title_label, "Microphone");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.microphone_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建音量圆盘
        _registry.win.cit.microphone_test.scale_line = lv_scale_create(container);
        lv_obj_set_size(_registry.win.cit.microphone_test.scale_line, 400, 400);
        lv_scale_set_mode(_registry.win.cit.microphone_test.scale_line, LV_SCALE_MODE_ROUND_INNER);
        lv_obj_set_style_bg_opa(_registry.win.cit.microphone_test.scale_line, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(_registry.win.cit.microphone_test.scale_line, lv_color_white(), 0);
        lv_obj_set_style_radius(_registry.win.cit.microphone_test.scale_line, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_clip_corner(_registry.win.cit.microphone_test.scale_line, true, 0);
        lv_obj_align(_registry.win.cit.microphone_test.scale_line, LV_ALIGN_TOP_MID, 0, 100);

        lv_scale_set_label_show(_registry.win.cit.microphone_test.scale_line, true);
        lv_scale_set_total_tick_count(_registry.win.cit.microphone_test.scale_line, 51);
        lv_scale_set_major_tick_every(_registry.win.cit.microphone_test.scale_line, 5);

        lv_obj_set_style_length(_registry.win.cit.microphone_test.scale_line, 5, LV_PART_ITEMS);
        lv_obj_set_style_length(_registry.win.cit.microphone_test.scale_line, 10, LV_PART_INDICATOR);
        lv_scale_set_range(_registry.win.cit.microphone_test.scale_line, 0, 100);

        lv_scale_set_angle_range(_registry.win.cit.microphone_test.scale_line, 270);
        lv_scale_set_rotation(_registry.win.cit.microphone_test.scale_line, 135);

        _registry.win.cit.microphone_test.needle_line = lv_line_create(_registry.win.cit.microphone_test.scale_line);
        lv_obj_set_style_line_width(_registry.win.cit.microphone_test.needle_line, 3, (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_style_line_rounded(_registry.win.cit.microphone_test.needle_line, true, (lv_style_selector_t)LV_PART_MAIN);

        lv_scale_set_line_needle_value(_registry.win.cit.microphone_test.scale_line, _registry.win.cit.microphone_test.needle_line, 150, 0);

        // 创建一个标签用于显示麦克风数据
        _registry.win.cit.microphone_test.data.label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.microphone_test.data.label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.microphone_test.data.label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.microphone_test.data.label, "microphone data:");
        lv_obj_align(_registry.win.cit.microphone_test.data.label, LV_ALIGN_TOP_MID, 0, 500);

        lv_obj_t *adc_to_dac_label = lv_label_create(container);
        lv_label_set_text(adc_to_dac_label, "adc -> dac");
        lv_obj_set_style_text_font(adc_to_dac_label, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(adc_to_dac_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align_to(adc_to_dac_label, _registry.win.cit.microphone_test.data.label, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);

        _registry.win.cit.microphone_test.adc_to_dac_switch = lv_switch_create(container);
        lv_obj_set_size(_registry.win.cit.microphone_test.adc_to_dac_switch, 90, 50);
        lv_obj_align_to(_registry.win.cit.microphone_test.adc_to_dac_switch, adc_to_dac_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

        if (_registry.win.cit.microphone_test.adc_to_dac_switch_status == true)
        {
            lv_obj_add_state(_registry.win.cit.microphone_test.adc_to_dac_switch, LV_STATE_CHECKED);

            set_adc_to_dac_switch_status(true);
        }
        else
        {
            lv_obj_remove_state(_registry.win.cit.microphone_test.adc_to_dac_switch, LV_STATE_CHECKED);

            set_adc_to_dac_switch_status(false);
        }

        lv_obj_add_event_cb(_registry.win.cit.microphone_test.adc_to_dac_switch, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                self->_registry.win.cit.microphone_test.adc_to_dac_switch_status = lv_obj_has_state(self->_registry.win.cit.microphone_test.adc_to_dac_switch, LV_STATE_CHECKED);

                                if (self->_registry.win.cit.microphone_test.adc_to_dac_switch_status == true)
                                {
                                    self->set_adc_to_dac_switch_status(true);
                                }
                                else
                                {
                                    self->set_adc_to_dac_switch_status(false);
                                } }, LV_EVENT_VALUE_CHANGED, this);

        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(_registry.win.cit.microphone_test.root);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_microphone_test(false);
                                    self->set_adc_to_dac_switch_status(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_microphone_test(false);
                                    self->set_adc_to_dac_switch_status(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.microphone_test.root, [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    if (code == LV_EVENT_GESTURE)
                                    {
                                        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());
    
                                        // 边缘检测以及左右滑动
                                        if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                        {
                                            self->set_microphone_test(false);
                                            self->set_adc_to_dac_switch_status(false);
                                            
                                            self->set_vibration();
                                            self->init_win_cit();
                                            
                                            lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    
                                            self->_edge_touch_flag = false;
                                        }
                                    } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.microphone_test.root);

        lv_obj_update_layout(_registry.win.cit.microphone_test.root);

        set_microphone_test(true);
    }

    void System::init_win_cit_imu_test(void)
    {
        // 主界面
        _registry.win.cit.imu_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.imu_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.imu_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.imu_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.imu_test.root);
        lv_label_set_text(title_label, "Imu");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.imu_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建一个标签用于显示触摸点数据
        _registry.win.cit.imu_test.data_label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.imu_test.data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.imu_test.data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.imu_test.data_label, "imu data:");
        lv_obj_align(_registry.win.cit.imu_test.data_label, LV_ALIGN_CENTER, 0, 0);

        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(_registry.win.cit.imu_test.root);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_imu_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_imu_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.imu_test.root, [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    if (code == LV_EVENT_GESTURE)
                                    {
                                        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());
    
                                        // 边缘检测以及左右滑动
                                        if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                        {
                                            self->set_imu_test(false);
                                            
                                            self->set_vibration();
                                            self->init_win_cit();
                                            
                                            lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    
                                            self->_edge_touch_flag = false;
                                        }
                                    } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.imu_test.root);

        lv_obj_update_layout(_registry.win.cit.imu_test.root);

        set_imu_test(true);
    }

    void System::init_win_cit_battery_health_test(void)
    {
        // 主界面
        _registry.win.cit.battery_health_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.battery_health_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.battery_health_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.battery_health_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.battery_health_test.root);
        lv_label_set_text(title_label, "Battery Health");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.battery_health_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);

        // 创建一个标签用于显示电池健康数据
        _registry.win.cit.battery_health_test.data_label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.battery_health_test.data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.battery_health_test.data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.battery_health_test.data_label, "battery health data:");
        lv_obj_align(_registry.win.cit.battery_health_test.data_label, LV_ALIGN_CENTER, 0, 0);

        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(_registry.win.cit.battery_health_test.root);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.battery_health_test.root, [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    if (code == LV_EVENT_GESTURE)
                                    {
                                        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());
    
                                        // 边缘检测以及左右滑动
                                        if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                        {
                                            self->set_vibration();
                                            self->init_win_cit();
                                            
                                            lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    
                                            self->_edge_touch_flag = false;
                                        }
                                    } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.battery_health_test.root);

        lv_obj_update_layout(_registry.win.cit.battery_health_test.root);

        _current_win = Current_Win::CIT_BATTERY_HEALTH_TEST;
    }

    void System::init_win_cit_gps_test(void)
    {
        // 主界面
        _registry.win.cit.gps_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.gps_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.gps_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.gps_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.gps_test.root);
        lv_label_set_text(title_label, "Gps");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.gps_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);

        // 创建一个标签用于显示电池健康数据
        _registry.win.cit.gps_test.data_label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.gps_test.data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.gps_test.data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.gps_test.data_label, "gps data:");
        lv_obj_align(_registry.win.cit.gps_test.data_label, LV_ALIGN_CENTER, 0, 0);

        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(_registry.win.cit.gps_test.root);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_gps_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_gps_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.gps_test.root, [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    if (code == LV_EVENT_GESTURE)
                                    {
                                        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());
    
                                        // 边缘检测以及左右滑动
                                        if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                        {
                                            self->set_gps_test(false);
                                            
                                            self->set_vibration();
                                            self->init_win_cit();
                                            
                                            lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    
                                            self->_edge_touch_flag = false;
                                        }
                                    } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.gps_test.root);

        lv_obj_update_layout(_registry.win.cit.gps_test.root);

        set_gps_test(true);
    }

    void System::init_win_cit_ethernet_test(void)
    {
        // 主界面
        _registry.win.cit.ethernet_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.ethernet_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.ethernet_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.ethernet_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.ethernet_test.root);
        lv_label_set_text(title_label, "Ethernet");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.ethernet_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);

        // 创建一个标签用于显示电池健康数据
        _registry.win.cit.ethernet_test.data_label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.ethernet_test.data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.ethernet_test.data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.ethernet_test.data_label, "ethernet data:");
        lv_obj_align(_registry.win.cit.ethernet_test.data_label, LV_ALIGN_CENTER, 0, 0);

        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(_registry.win.cit.ethernet_test.root);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_ethernet_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_ethernet_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.ethernet_test.root, [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    if (code == LV_EVENT_GESTURE)
                                    {
                                        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());
    
                                        // 边缘检测以及左右滑动
                                        if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                        {
                                            self->set_ethernet_test(false);
                                            
                                            self->set_vibration();
                                            self->init_win_cit();
                                            
                                            lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    
                                            self->_edge_touch_flag = false;
                                        }
                                    } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.ethernet_test.root);

        lv_obj_update_layout(_registry.win.cit.ethernet_test.root);

        set_ethernet_test(true);
    }

    void System::init_win_cit_rtc_test(void)
    {
        // 主界面
        _registry.win.cit.rtc_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.rtc_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.rtc_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.rtc_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.rtc_test.root);
        lv_label_set_text(title_label, "Rtc");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.rtc_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);

        // 创建一个标签用于显示电池健康数据
        _registry.win.cit.rtc_test.data_label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.rtc_test.data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.rtc_test.data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.rtc_test.data_label, "rtc data:");
        lv_obj_align(_registry.win.cit.rtc_test.data_label, LV_ALIGN_CENTER, 0, 0);

        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(_registry.win.cit.rtc_test.root);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.rtc_test.root, [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    if (code == LV_EVENT_GESTURE)
                                    {
                                        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());
    
                                        // 边缘检测以及左右滑动
                                        if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                        {
                                            
                                            self->set_vibration();
                                            self->init_win_cit();
                                            
                                            lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    
                                            self->_edge_touch_flag = false;
                                        }
                                    } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.rtc_test.root);

        lv_obj_update_layout(_registry.win.cit.rtc_test.root);

        _current_win = Current_Win::CIT_RTC_TEST;
    }

    void System::init_win_cit_esp32c6_at_test(void)
    {
        // 主界面
        _registry.win.cit.esp32c6_at_test.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.cit.esp32c6_at_test.root, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.cit.esp32c6_at_test.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.cit.esp32c6_at_test.root, LV_SCROLLBAR_MODE_OFF);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.cit.esp32c6_at_test.root);
        lv_label_set_text(title_label, "Esp32c6 At");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 100, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

        // 创建容器
        lv_obj_t *container = lv_obj_create(_registry.win.cit.esp32c6_at_test.root);
        lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
        lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);

        // 创建一个标签用于显示电池健康数据
        _registry.win.cit.esp32c6_at_test.data_label = lv_label_create(container);
        lv_obj_set_style_text_color(_registry.win.cit.esp32c6_at_test.data_label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.cit.esp32c6_at_test.data_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(_registry.win.cit.esp32c6_at_test.data_label, "esp32c6 at time data:");
        lv_obj_align(_registry.win.cit.esp32c6_at_test.data_label, LV_ALIGN_CENTER, 0, 0);

        // 创建一个容器来存放两个按键
        lv_obj_t *button_container = lv_obj_create(_registry.win.cit.esp32c6_at_test.root);
        lv_obj_set_size(button_container, _width, 140);
        lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(button_container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
        lv_obj_set_style_radius(button_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

        // 创建PASS按键
        lv_obj_t *pass_button = lv_button_create(button_container);
        lv_obj_set_size(pass_button, 200, 60);
        lv_obj_align(pass_button, LV_ALIGN_LEFT_MID, 10, 0);
        lv_obj_set_style_radius(pass_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(pass_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *pass_label = lv_label_create(pass_button);
        lv_obj_set_style_text_font(pass_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(pass_label, "PASS");
        lv_obj_center(pass_label);

        lv_obj_add_event_cb(pass_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_esp32c6_at_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_OK;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0x008B45;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建FAIL按键
        lv_obj_t *fail_button = lv_button_create(button_container);
        lv_obj_set_size(fail_button, 200, 60);
        lv_obj_align(fail_button, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_radius(fail_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(fail_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影

        lv_obj_t *fail_label = lv_label_create(fail_button);
        lv_obj_set_style_text_font(fail_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_label_set_text(fail_label, "FAIL");
        lv_obj_center(fail_label);

        lv_obj_add_event_cb(fail_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_esp32c6_at_test(false);

                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].symbol = LV_SYMBOL_CLOSE;
                                    _win_cit_test_item_list[self->_registry.win.cit.current_test_item_index].color = 0xEE2C2C;

                                    self->init_win_cit();

                                    lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                    break;
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_add_event_cb(_registry.win.cit.esp32c6_at_test.root, [](lv_event_t *e)
                            {
                                    System *self = static_cast<System *>(lv_event_get_user_data(e));
                                    lv_event_code_t code = lv_event_get_code(e);
    
                                    if (code == LV_EVENT_GESTURE)
                                    {
                                        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());
    
                                        // 边缘检测以及左右滑动
                                        if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                        {
                                            self->set_esp32c6_at_test(false);
                                            
                                            self->set_vibration();
                                            self->init_win_cit();
                                            
                                            lv_screen_load_anim(self->_registry.win.cit.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    
                                            self->_edge_touch_flag = false;
                                        }
                                    } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.cit.esp32c6_at_test.root);

        lv_obj_update_layout(_registry.win.cit.esp32c6_at_test.root);

        set_esp32c6_at_test(true);
    }

    // void System::init_win_cit_sleep_test(void)
    // {
    //     // 主界面
    //     _registry.win.cit.sleep_test = lv_obj_create(NULL);
    //     lv_obj_set_style_bg_color(_registry.win.cit.sleep_test, lv_color_hex(0xFF7F58), (lv_style_selector_t)LV_PART_MAIN);
    //     lv_obj_set_size(_registry.win.cit.sleep_test, _width, _height);
    //     lv_obj_set_scrollbar_mode(_registry.win.cit.sleep_test, LV_SCROLLBAR_MODE_OFF);

    //     // 创建标题
    //     lv_obj_t *title_label = lv_label_create(_registry.win.cit.sleep_test);
    //     lv_label_set_text(title_label, "Sleep");
    //     lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_set_size(title_label, _width - 100, 40);
    //     lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20, 10 + 50);

    //     // 创建容器
    //     lv_obj_t *container = lv_obj_create(_registry.win.cit.sleep_test);
    //     lv_obj_set_size(container, _width, _height - 50 - 80 - 140);
    //     lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50 + 80);
    //     lv_obj_set_style_bg_color(container, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为白色
    //     lv_obj_set_style_radius(container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_set_style_border_width(container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框

    //     // 创建NORMAL_SLEEP按键
    //     lv_obj_t *light_sleep_button = lv_button_create(container);
    //     lv_obj_set_size(light_sleep_button, 250, 80);
    //     lv_obj_align(light_sleep_button, LV_ALIGN_RIGHT_MID, -10, 0);
    //     lv_obj_set_style_radius(light_sleep_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_set_style_shadow_width(light_sleep_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影
    //     lv_obj_set_style_bg_color(light_sleep_button, lv_color_hex(0xFF6A6A), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_align(light_sleep_button, LV_ALIGN_CENTER, 0, -50);

    //     lv_obj_t *light_sleep_label = lv_label_create(light_sleep_button);
    //     lv_obj_set_style_text_font(light_sleep_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_label_set_text(light_sleep_label, "NORMAL SLEEP");
    //     lv_obj_center(light_sleep_label);

    //     lv_obj_add_event_cb(light_sleep_button, [](lv_event_t *e)
    //                         {
    //                             System *self = static_cast<System *>(lv_event_get_user_data(e));
    //                             lv_event_code_t code = lv_event_get_code(e);

    //                             switch (code)
    //                             {
    //                             case LV_EVENT_CLICKED:

    //                                 break;
    //                             default:
    //                                 break;
    //                             } }, LV_EVENT_ALL, this);

    //     // 创建LIGHT_SLEEP按键
    //     lv_obj_t *deep_sleep_button = lv_button_create(container);
    //     lv_obj_set_size(deep_sleep_button, 250, 80);
    //     lv_obj_align(deep_sleep_button, LV_ALIGN_RIGHT_MID, -10, 0);
    //     lv_obj_set_style_radius(deep_sleep_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_set_style_shadow_width(deep_sleep_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 移除阴影
    //     lv_obj_set_style_bg_color(deep_sleep_button, lv_color_hex(0xFF6A6A), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_obj_align(deep_sleep_button, LV_ALIGN_CENTER, 0, 50);

    //     lv_obj_t *deep_sleep_label = lv_label_create(deep_sleep_button);
    //     lv_obj_set_style_text_font(deep_sleep_label, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    //     lv_label_set_text(deep_sleep_label, "LIGHT SLEEP");
    //     lv_obj_center(deep_sleep_label);

    //     lv_obj_add_event_cb(deep_sleep_button, [](lv_event_t *e)
    //                         {
    //                             System *self = static_cast<System *>(lv_event_get_user_data(e));
    //                             lv_event_code_t code = lv_event_get_code(e);

    //                             switch (code)
    //                             {
    //                             case LV_EVENT_CLICKED:
    //                             self->start_sleep_test(Sleep_Mode::LIGHT_SLEEP);

    //                                 break;
    //                             default:
    //                                 break;
    //                             } }, LV_EVENT_ALL, this);

    //     add_win_cit_test_item_pass_fail_button(_registry.win.cit.sleep_test);

    //     add_event_cb_win_return_to_cit(_registry.win.cit.sleep_test);

    //     init_status_bar(_registry.win.cit.sleep_test);

    //     lv_obj_update_layout(_registry.win.cit.sleep_test);
    // }

    void System::init_win_camera(void)
    {
        // 主界面
        _registry.win.camera.root = lv_obj_create(NULL);

        lv_obj_set_style_bg_color(_registry.win.camera.root, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.camera.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.camera.root, LV_SCROLLBAR_MODE_OFF);

        // // 创建画布来显示摄像头数据
        // _registry.win.camera.canvas = lv_canvas_create(_registry.win.camera.root);
        // lv_canvas_set_buffer(_registry.win.camera.canvas, _lv_color_win_draw_buf.get(), _width, _height, LV_COLOR_FORMAT_RGB565);
        // lv_canvas_fill_bg(_registry.win.camera.canvas, lv_color_black(), LV_OPA_COVER);
        // lv_obj_center(_registry.win.camera.canvas);

        lv_obj_add_event_cb(_registry.win.camera.root, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_GESTURE)
                                {
                                    lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());

                                    // 边缘检测以及左右滑动
                                    if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                    {
                                        self->set_camera_status(false);
                                        
                                        self->set_vibration();
                                        self->init_win_home();
                                        
                                        lv_screen_load_anim(self->_registry.win.home.root, LV_SCR_LOAD_ANIM_FADE_OUT, 100, 0, true);

                                        self->_edge_touch_flag = false;
                                    }
                                } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.camera.root);

        lv_obj_update_layout(_registry.win.camera.root);

        set_camera_status(true);

        _current_win = Current_Win::CAMERA;
    }

    void System::init_win_lora(void)
    {
        // 主界面
        _registry.win.lora.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.lora.root, lv_color_hex(0xA69CDB), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.lora.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.lora.root, LV_SCROLLBAR_MODE_OFF);

        // 添加 symbol list 图标
        lv_obj_t *symbol_icon = lv_label_create(_registry.win.lora.root);
        lv_label_set_text(symbol_icon, LV_SYMBOL_LIST);
        lv_obj_set_style_text_color(symbol_icon, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(symbol_icon, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(symbol_icon, LV_ALIGN_TOP_RIGHT, -30, 15 + 50);

        // 设置标签为可点击
        lv_obj_add_flag(symbol_icon, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(symbol_icon, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->set_lora_status_callback(false);
                                    self->init_win_lora_setings();

                                    lv_screen_load_anim(self->_registry.win.lora.setings.root, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, true);
                                break;
                                default:
                                break;
                                } }, LV_EVENT_ALL, this);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.lora.root);
        lv_label_set_text(title_label, "Lora");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 300, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 30, 10 + 50);

        // 创建发送框容器
        _registry.win.lora.send_box_container = lv_obj_create(_registry.win.lora.root);
        lv_obj_set_size(_registry.win.lora.send_box_container, _width, 100);
        lv_obj_align(_registry.win.lora.send_box_container, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(_registry.win.lora.send_box_container, lv_color_hex(0xEEE9E9), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为灰色
        lv_obj_set_style_radius(_registry.win.lora.send_box_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(_registry.win.lora.send_box_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_remove_flag(_registry.win.lora.send_box_container, LV_OBJ_FLAG_SCROLLABLE);                          // 禁止滚动
        lv_obj_remove_flag(_registry.win.lora.send_box_container, LV_OBJ_FLAG_CLICKABLE);                           // 禁止触摸

        _registry.win.lora.chat_textarea = lv_textarea_create(_registry.win.lora.send_box_container);
        lv_textarea_set_one_line(_registry.win.lora.chat_textarea, true);
        lv_textarea_set_password_mode(_registry.win.lora.chat_textarea, false);
        lv_obj_set_style_pad_top(_registry.win.lora.chat_textarea, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.chat_textarea, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.chat_textarea, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.chat_textarea, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        // 设置初始内容为_registry.win.lora.chat_textarea_data
        lv_textarea_set_text(_registry.win.lora.chat_textarea, _registry.win.lora.chat_textarea_data.c_str());
        lv_obj_set_style_text_font(_registry.win.lora.chat_textarea, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.chat_textarea, _width - 20 - 150);
        lv_obj_align(_registry.win.lora.chat_textarea, LV_ALIGN_BOTTOM_LEFT, -20, 20); // 调整位置到底部往上一点点

        lv_obj_t *send_button = lv_button_create(_registry.win.lora.send_box_container);
        lv_obj_set_size(send_button, 120, 55);
        lv_obj_align_to(send_button, _registry.win.lora.chat_textarea, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
        lv_obj_set_style_radius(send_button, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(send_button, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_t *label = lv_label_create(send_button);
        lv_label_set_text(label, "send");
        lv_obj_set_style_text_font(label, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_center(label);

        lv_obj_add_event_cb(send_button, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_CLICKED)
                                {
                                // 获取当前输入框内容
                                std::string text = lv_textarea_get_text(self->_registry.win.lora.chat_textarea);

                                    //如果消息不为空
                                    if (!text.empty())
                                    {
                                        char buffer_time[15];
                                        snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", self->_time.hour , self-> _time.minute , self->_time.second);
        
                                        Win_Lora_Chat_Message wlcm =
                                        {
                                            .direction = Chat_Message_Direction::SEND,
                                            .time = buffer_time,
                                            .data = text,
                                        };
                                        self->_registry.win.lora.chat_message_data.push_back(wlcm);

                                        // 清空输入框
                                        lv_textarea_set_text(self->_registry.win.lora.chat_textarea, "");

                                        // 更新聊天容器
                                        self->win_lora_chat_message_data_update(self->_registry.win.lora.chat_message_data);

                                        self->set_lora_send_data_callback(text);
                                    }
                                } }, LV_EVENT_ALL, this);

        _registry.keyboard = lv_keyboard_create(_registry.win.lora.root);
        lv_obj_set_size(_registry.keyboard, _width, _height / 3.5);
        lv_obj_set_style_radius(_registry.keyboard, 8, (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_DEFAULT);
        // 设置键盘按钮间距更密集
        lv_obj_set_style_pad_row(_registry.keyboard, 8, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(_registry.keyboard, 4, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.keyboard, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部
        lv_obj_add_flag(_registry.keyboard, LV_OBJ_FLAG_HIDDEN);     // 初始隐藏键盘

        lv_keyboard_set_textarea(_registry.keyboard, _registry.win.lora.chat_textarea);

        lv_obj_add_event_cb(_registry.win.lora.chat_textarea, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                lv_obj_t *ta = lv_event_get_target_obj(e);

                                switch (code)
                                {
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, ta);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.chat_message_container, self->_width, self->_height - 100 - 130 - lv_obj_get_height(self->_registry.keyboard));

                                        // 调整容器位置
                                        lv_obj_align_to(self->_registry.win.lora.send_box_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0);
                                        lv_obj_align_to(self->_registry.win.lora.chat_message_container, self->_registry.win.lora.send_box_container, LV_ALIGN_OUT_TOP_MID, 0, 0);
                                    break;
                                case LV_EVENT_DEFOCUSED:
                                    //     lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                    //     // 调整聊天框的大小
                                    //     lv_obj_set_size(self->_registry.win.lora.chat_message_container, self->_width, self->_height - 100 - 130);
                                    //     // 恢复容器位置
                                    //     lv_obj_align(self->_registry.win.lora.send_box_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                    //     lv_obj_align_to(self->_registry.win.lora.chat_message_container, self->_registry.win.lora.send_box_container, LV_ALIGN_OUT_TOP_MID, 0, 0);
                                    break;
                                case LV_EVENT_READY:
                                    {
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        // lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // // 调整聊天框的大小
                                        // lv_obj_set_size(self->_registry.win.lora.chat_message_container, self->_width, self->_height - 100 - 130);
                                        // // 恢复容器位置
                                        // lv_obj_align(self->_registry.win.lora.send_box_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        // lv_obj_align_to(self->_registry.win.lora.chat_message_container, self->_registry.win.lora.send_box_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                        // 获取当前输入框内容
                                        std::string text = lv_textarea_get_text(self->_registry.win.lora.chat_textarea);

                                        //如果消息不为空
                                        if (!text.empty())
                                        {
                                            char buffer_time[15];
                                            snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", self->_time.hour , self-> _time.minute , self->_time.second);
            
                                            Win_Lora_Chat_Message wlcm =
                                            {
                                                .direction = Chat_Message_Direction::SEND,
                                                .time = buffer_time,
                                                .data = text,
                                            };
                                            self->_registry.win.lora.chat_message_data.push_back(wlcm);

                                            // 清空输入框
                                            lv_textarea_set_text(self->_registry.win.lora.chat_textarea, "");

                                            // 更新聊天容器
                                            self->win_lora_chat_message_data_update(self->_registry.win.lora.chat_message_data);
                                        }
                                    }
                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        // 创建聊天容器
        _registry.win.lora.chat_message_container = lv_obj_create(_registry.win.lora.root);
        lv_obj_set_size(_registry.win.lora.chat_message_container, _width, _height - 100 - 130);
        lv_obj_set_style_bg_color(_registry.win.lora.chat_message_container, lv_color_hex(0xEEE9E9), (lv_style_selector_t)LV_PART_MAIN); // 设置背景颜色为灰
        lv_obj_set_style_radius(_registry.win.lora.chat_message_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(_registry.win.lora.chat_message_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_align_to(_registry.win.lora.chat_message_container, _registry.win.lora.send_box_container, LV_ALIGN_OUT_TOP_MID, 0, 0);
        lv_obj_set_scrollbar_mode(_registry.win.lora.chat_message_container, LV_SCROLLBAR_MODE_ACTIVE);

        // 触摸聊天区域时隐藏键盘
        lv_obj_add_event_cb(_registry.win.lora.chat_message_container, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_CLICKED)
                                {
                                lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                // 调整聊天框的大小
                                lv_obj_set_size(self->_registry.win.lora.chat_message_container, self->_width, self->_height - 100 - 130);
                                // 恢复容器位置
                                lv_obj_align(self->_registry.win.lora.send_box_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                lv_obj_align_to(self->_registry.win.lora.chat_message_container, self->_registry.win.lora.send_box_container, LV_ALIGN_OUT_TOP_MID, 0, 0);
                                } }, LV_EVENT_ALL, this);

        win_lora_chat_message_data_update(_registry.win.lora.chat_message_data);

        lv_obj_add_event_cb(_registry.win.lora.root, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_GESTURE)
                                {
                                    lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());

                                    // 边缘检测以及左右滑动
                                    if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                    {   
                                        self->set_lora_status_callback(false);
                                        self->_registry.win.lora.chat_textarea_data = lv_textarea_get_text(self->_registry.win.lora.chat_textarea);

                                        self->set_vibration();
                                        self->init_win_home();
                                        
                                        lv_screen_load_anim(self->_registry.win.home.root, LV_SCR_LOAD_ANIM_FADE_OUT, 100, 0, true);

                                        self->_edge_touch_flag = false;
                                    }
                                } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.lora.root);

        lv_obj_update_layout(_registry.win.lora.root);

        set_lora_status_callback(true);

        _current_win = Current_Win::LORA;
    }

    void System::win_lora_chat_message_data_update(std::vector<Win_Lora_Chat_Message> wlcm)
    {
        // 清空 _registry.win.lora.chat_message_container 的所有子对象
        lv_obj_clean(_registry.win.lora.chat_message_container);

        if (wlcm.size() > 100)
        {
            // 删除最早的数据直到只剩下100条
            wlcm.erase(wlcm.begin(), wlcm.begin() + (wlcm.size() - 100));
        }

        lv_obj_t *previous_message_btn = nullptr;
        for (uint8_t i = 0; i < wlcm.size(); i++)
        {
            // 聊天按钮
            lv_obj_t *message_btn = lv_button_create(_registry.win.lora.chat_message_container);
            lv_obj_set_width(message_btn, LV_SIZE_CONTENT);
            lv_obj_set_height(message_btn, LV_SIZE_CONTENT);
            lv_obj_set_style_pad_left(message_btn, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(message_btn, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(message_btn, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(message_btn, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_width(message_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT); // 去除按钮阴影

            // 时间标签
            lv_obj_t *time_label = lv_label_create(_registry.win.lora.chat_message_container);
            lv_label_set_text(time_label, wlcm[i].time.c_str());
            lv_obj_set_style_text_font(time_label, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(time_label, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);

            // 设置按钮颜色和对齐
            if (wlcm[i].direction == Chat_Message_Direction::SEND)
            {
                lv_obj_set_style_bg_color(message_btn, lv_color_hex(0x4A90E2), LV_PART_MAIN | LV_STATE_DEFAULT); // 蓝色
                if (previous_message_btn != nullptr)
                {
                    lv_obj_update_layout(previous_message_btn); // 确保布局更新
                    lv_obj_align(message_btn, LV_ALIGN_TOP_RIGHT, 0, lv_obj_get_y(previous_message_btn) + lv_obj_get_height(previous_message_btn) + 40);
                }
                else
                {
                    lv_obj_align(message_btn, LV_ALIGN_TOP_RIGHT, 0, 20);
                }

                lv_obj_align_to(time_label, message_btn, LV_ALIGN_OUT_TOP_RIGHT, 0, -5);
            }
            else
            {
                // rssi/snr标签
                lv_obj_t *rssi_snr_label = lv_label_create(_registry.win.lora.chat_message_container);
                lv_label_set_text(rssi_snr_label, wlcm[i].rssi_snr.c_str());
                lv_obj_set_style_text_font(rssi_snr_label, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(rssi_snr_label, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_set_style_bg_color(message_btn, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT); // 白色

                if (previous_message_btn != nullptr)
                {
                    lv_obj_update_layout(previous_message_btn); // 确保布局更新
                    lv_obj_align(message_btn, LV_ALIGN_TOP_LEFT, 0, lv_obj_get_y(previous_message_btn) + lv_obj_get_height(previous_message_btn) + 70);
                }
                else
                {
                    lv_obj_align(message_btn, LV_ALIGN_TOP_LEFT, 0, 50);
                }

                lv_obj_align_to(rssi_snr_label, message_btn, LV_ALIGN_OUT_TOP_LEFT, 0, -5);
                lv_obj_align_to(time_label, rssi_snr_label, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
            }

            // 判断 wlcm[i].data 的长度并插入换行符
            std::string message_text = wlcm[i].data;

            if (wlcm[i].data.size() > 15)
            {
                for (size_t pos = 15; pos < message_text.length(); pos += 16)
                {
                    message_text.insert(pos, "\n");
                }
            }

            // 消息内容
            lv_obj_t *message_label = lv_label_create(message_btn);
            lv_label_set_text(message_label, message_text.c_str());
            lv_obj_set_style_text_font(message_label, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(message_label, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);

            previous_message_btn = message_btn;
        }

        lv_obj_update_layout(_registry.win.lora.chat_message_container); // 确保布局更新
        // 滚动到聊天容器的最底部
        lv_obj_scroll_to_y(_registry.win.lora.chat_message_container,
                           lv_obj_get_scroll_bottom(_registry.win.lora.chat_message_container), LV_ANIM_OFF);
    }

    void System::init_win_lora_setings(void)
    {
        // 主界面
        _registry.win.lora.setings.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.lora.setings.root, lv_color_hex(0xA69CDB), (lv_style_selector_t)LV_PART_MAIN);
        lv_obj_set_size(_registry.win.lora.setings.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.lora.setings.root, LV_SCROLLBAR_MODE_OFF);

        // 添加 symbol 退出图标
        lv_obj_t *symbol_icon = lv_label_create(_registry.win.lora.setings.root);
        lv_label_set_text(symbol_icon, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_color(symbol_icon, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(symbol_icon, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(symbol_icon, LV_ALIGN_TOP_LEFT, 30, 15 + 50);

        // 设置标签为可点击
        lv_obj_add_flag(symbol_icon, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(symbol_icon, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                    self->init_win_lora();

                                    lv_screen_load_anim(self->_registry.win.lora.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
                                break;
                                default:
                                break;
                            } }, LV_EVENT_ALL, this);

        // 创建标题
        lv_obj_t *title_label = lv_label_create(_registry.win.lora.setings.root);
        lv_label_set_text(title_label, "Lora Setings");
        lv_obj_set_style_text_color(title_label, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(title_label, _width - 200, 40);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 20 + 90, 10 + 50);

        // 创建列表
        lv_obj_t *list = lv_list_create(_registry.win.lora.setings.root);
        lv_obj_set_size(list, _width, _height - 130);
        lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_pad_left(list, 20, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(list, 20, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_24, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_radius(list, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);

        lv_obj_t *list_button_config_lora_params = lv_list_add_button(list, NULL, "config lora params");
        lv_obj_set_style_text_font(list_button_config_lora_params, &lv_font_montserrat_30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

        lv_obj_add_event_cb(list_button_config_lora_params, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                {
                                    self->init_win_lora_setings_config_lora_params_message_box();
                                }
                                    break;
                                default:
                                    break;
                            } }, LV_EVENT_ALL, this);

        lv_obj_t *list_button_auto_send = lv_list_add_button(list, NULL, "auto send");
        lv_obj_set_style_text_font(list_button_auto_send, &lv_font_montserrat_30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

        lv_obj_add_event_cb(list_button_auto_send, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                {
                                    self->init_win_lora_setings_auto_send_message_box();
                                }
                                    break;
                                default:
                                    break;
                            } }, LV_EVENT_ALL, this);

        // // 创建开关并添加到列表项中
        // lv_obj_t *sw = lv_switch_create(list_button);
        // lv_obj_set_size(sw, 80, 50);

        lv_obj_add_event_cb(_registry.win.lora.setings.root, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_GESTURE)
                                {
                                    lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());

                                    // 边缘检测以及左右滑动
                                    if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                    {   
                                        self->set_vibration();
                                        self->init_win_lora();
                                        
                                        lv_screen_load_anim(self->_registry.win.lora.root, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);

                                        self->_edge_touch_flag = false;
                                    }
                                } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.lora.setings.root);

        lv_obj_update_layout(_registry.win.lora.setings.root);

        _current_win = Current_Win::LORA_SETINGS;
    }

    void System::init_win_lora_setings_config_lora_params_message_box(void)
    {
        // 创建全屏灰色透明遮罩，禁止触摸
        _registry.win.lora.setings.message_box.root = lv_obj_create(_registry.win.lora.setings.root);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.root, _width, _height);
        lv_obj_set_style_bg_color(_registry.win.lora.setings.message_box.root, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_registry.win.lora.setings.message_box.root, LV_OPA_50, LV_PART_MAIN);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.root, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(_registry.win.lora.setings.message_box.root, 0, LV_PART_MAIN);
        lv_obj_align(_registry.win.lora.setings.message_box.root, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(_registry.win.lora.setings.message_box.root, LV_OBJ_FLAG_CLICKABLE); // 禁止触摸

        _registry.win.lora.setings.message_box.root_container = lv_obj_create(_registry.win.lora.setings.message_box.root);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.root_container, 450, 900);
        lv_obj_set_style_radius(_registry.win.lora.setings.message_box.root_container, 15, LV_PART_MAIN);
        lv_obj_set_style_bg_color(_registry.win.lora.setings.message_box.root_container, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.root_container, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(_registry.win.lora.setings.message_box.root_container, 16, LV_PART_MAIN);
        lv_obj_center(_registry.win.lora.setings.message_box.root_container);

        _registry.win.lora.setings.message_box.btn_container = lv_obj_create(_registry.win.lora.setings.message_box.root_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.btn_container, 450, 110);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_align(_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);

        lv_obj_t *btn_cancel = lv_button_create(_registry.win.lora.setings.message_box.btn_container);
        lv_obj_set_size(btn_cancel, 150, 60);
        lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT, 40, -30);
        lv_obj_set_style_radius(btn_cancel, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn_cancel, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 去除按钮阴影
        lv_obj_t *btn_cancel_label = lv_label_create(btn_cancel);
        lv_label_set_text(btn_cancel_label, "cancel");
        lv_obj_set_style_text_font(btn_cancel_label, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_center(btn_cancel_label);

        // cancel 按钮回调
        lv_obj_add_event_cb(btn_cancel, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_obj_delete(self->_registry.win.lora.setings.message_box.root); }, LV_EVENT_CLICKED, this);

        lv_obj_t *btn_apply = lv_button_create(_registry.win.lora.setings.message_box.btn_container);
        lv_obj_set_size(btn_apply, 150, 60);
        lv_obj_align(btn_apply, LV_ALIGN_BOTTOM_RIGHT, -40, -30);
        lv_obj_set_style_radius(btn_apply, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn_apply, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 去除按钮阴影
        lv_obj_t *btn_apply_label = lv_label_create(btn_apply);
        lv_label_set_text(btn_apply_label, "apply");
        lv_obj_set_style_text_font(btn_apply_label, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_center(btn_apply_label);

        // apply 按钮回调
        lv_obj_add_event_cb(btn_apply, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));

                                Device_Lora dl;

                                const char* freq_text = lv_textarea_get_text(self->_registry.win.lora.setings.config_lora_params.textarea.freq);
                                if (freq_text != nullptr && freq_text[0] != '\0') // 同时检查NULL和空字符串
                                {  
                                    double buffer = std::stod(freq_text, nullptr);  

                                    // 限制范围
                                    if((buffer >= 150.0) && (buffer <= 960.0))
                                    {
                                        dl.params.freq = buffer;
                                    }
                                }

                                uint32_t bandwidth_buffer_index = lv_dropdown_get_selected(self->_registry.win.lora.setings.config_lora_params.dropdown.bandwidth);
                                if(bandwidth_buffer_index > 6)
                                {
                                    bandwidth_buffer_index++;
                                }
                                dl.params.bw = static_cast<Sx126x::Lora_Bw>(bandwidth_buffer_index);
                                // printf("_device_lora.params.bw: %ld\n", static_cast<uint32_t>(dl.params.bw));

                                const char* current_limit_text = lv_textarea_get_text(self->_registry.win.lora.setings.config_lora_params.textarea.current_limit);
                                if (current_limit_text != nullptr && current_limit_text[0] != '\0') // 同时检查NULL和空字符串
                                {  
                                    float buffer = std::stof(current_limit_text, nullptr);  

                                    // 限制范围
                                    if((buffer >= 0) && (buffer <= 140.0))
                                    {
                                        dl.params.current_limit = buffer;
                                    }
                                }

                                const char* power_text = lv_textarea_get_text(self->_registry.win.lora.setings.config_lora_params.textarea.power);
                                if (power_text != nullptr && power_text[0] != '\0') // 同时检查NULL和空字符串
                                {  
                                    int8_t buffer = std::stoi(power_text);  

                                    // 限制范围
                                    if((buffer >= -9) && (buffer <= 22))
                                    {
                                        dl.params.power = buffer;
                                    }
                                }

                                dl.params.sf = static_cast<Sx126x::Sf>(
                                                            lv_dropdown_get_selected(self->_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor) + 5);

                                dl.params.cr = static_cast<Sx126x::Cr>(
                                                            lv_dropdown_get_selected(self->_registry.win.lora.setings.config_lora_params.dropdown.coding_rate) + 1);

                                dl.params.crc_type = static_cast<Sx126x::Lora_Crc_Type>(
                                                            lv_dropdown_get_selected(self->_registry.win.lora.setings.config_lora_params.dropdown.crc_type));

                                const char* preamble_length_text = lv_textarea_get_text(self->_registry.win.lora.setings.config_lora_params.textarea.preamble_length);
                                if (preamble_length_text != nullptr && preamble_length_text[0] != '\0') // 同时检查NULL和空字符串
                                {  
                                    dl.params.preamble_length = std::stoi(preamble_length_text);  
                                }

                                const char* sync_word_text = lv_textarea_get_text(self->_registry.win.lora.setings.config_lora_params.textarea.sync_word);
                                if (sync_word_text != nullptr && sync_word_text[0] != '\0') // 同时检查NULL和空字符串
                                {  
                                    dl.params.sync_word = std::stoi(sync_word_text);  
                                }

                                if(self->set_config_lora_params(dl) == true)
                                {
                                    self->_device_lora.params = dl.params;
                                }

                                lv_obj_delete(self->_registry.win.lora.setings.message_box.root); }, LV_EVENT_CLICKED, this);

        _registry.keyboard = lv_keyboard_create(_registry.win.lora.setings.message_box.root);
        lv_obj_set_size(_registry.keyboard, _width, _height / 3.5);
        lv_obj_set_style_radius(_registry.keyboard, 8, (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_DEFAULT);
        // 设置键盘按钮间距更密集
        lv_obj_set_style_pad_row(_registry.keyboard, 8, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(_registry.keyboard, 4, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.keyboard, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部
        lv_obj_add_flag(_registry.keyboard, LV_OBJ_FLAG_HIDDEN);     // 初始隐藏键盘

        _registry.win.lora.setings.message_box.parameter_container = lv_obj_create(_registry.win.lora.setings.message_box.root_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.parameter_container, 450, 780);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.parameter_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_set_scrollbar_mode(_registry.win.lora.setings.message_box.parameter_container, LV_SCROLLBAR_MODE_ACTIVE);
        lv_obj_align_to(_registry.win.lora.setings.message_box.parameter_container, _registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

        // 触摸lora设置消息框区域时隐藏键盘
        lv_obj_add_event_cb(_registry.win.lora.setings.message_box.parameter_container, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_CLICKED)
                                {
                                    lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                    // 调整聊天框的大小
                                    lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                    lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);
                                    lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                    lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 780);
                                    lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                } }, LV_EVENT_ALL, this);

        lv_obj_t *msgbox_freq_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_freq_text, "freq");
        lv_obj_set_size(msgbox_freq_text, 100, 40);
        lv_obj_set_style_text_font(msgbox_freq_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(msgbox_freq_text, LV_ALIGN_TOP_LEFT, 0, 0);

        _registry.win.lora.setings.config_lora_params.textarea.freq = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.textarea.freq, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.textarea.freq, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.textarea.freq, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.textarea.freq, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.setings.config_lora_params.textarea.freq, 300);
        lv_textarea_set_one_line(_registry.win.lora.setings.config_lora_params.textarea.freq, true);
        char freq_str[15];
        snprintf(freq_str, sizeof(freq_str), "%.6f", _device_lora.params.freq);
        lv_textarea_set_text(_registry.win.lora.setings.config_lora_params.textarea.freq, freq_str);
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.textarea.freq, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.textarea.freq, msgbox_freq_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.textarea.freq, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.freq);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.freq);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;

                                // case LV_EVENT_DEFOCUSED:
                                //         lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                //     break;
                                case LV_EVENT_READY:
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                        lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);

                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_t *freq_unit_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(freq_unit_text, "mhz");
        lv_obj_set_size(freq_unit_text, 70, 40);
        lv_obj_set_style_text_font(freq_unit_text, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(freq_unit_text, _registry.win.lora.setings.config_lora_params.textarea.freq, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);

        lv_obj_t *msgbox_bandwidth_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_bandwidth_text, "bandwidth");
        lv_obj_set_size(msgbox_bandwidth_text, 200, 40);
        lv_obj_set_style_text_font(msgbox_bandwidth_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_bandwidth_text, _registry.win.lora.setings.config_lora_params.textarea.freq, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        // lv_obj_t *textarea_bandwidth = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        // lv_obj_set_width(textarea_bandwidth, 300);
        // lv_textarea_set_one_line(textarea_bandwidth, true);
        // lv_textarea_set_text(textarea_bandwidth, ""); // 设置初始内容为空
        // lv_obj_align_to(textarea_bandwidth, msgbox_bandwidth_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        _registry.win.lora.setings.config_lora_params.dropdown.bandwidth = lv_dropdown_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_dropdown_set_dir(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, LV_DIR_BOTTOM);
        lv_dropdown_set_options(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, "BW_7810\n"
                                                                                                  "BW_15630\n"
                                                                                                  "BW_31250\n"
                                                                                                  "BW_62500\n"
                                                                                                  "BW_125000\n"
                                                                                                  "BW_250000\n"
                                                                                                  "BW_500000\n"
                                                                                                  "BW_10420\n"
                                                                                                  "BW_20830\n"
                                                                                                  "BW_41670");
        uint32_t buffer_index = static_cast<uint32_t>(_device_lora.params.bw);
        if (buffer_index > 6)
        {
            buffer_index--;
        }
        lv_dropdown_set_selected(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, buffer_index);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_min_width(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_min_height(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);  // 输入框字体
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT); // 下拉列表字体
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, msgbox_bandwidth_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.dropdown.bandwidth, [](lv_event_t *e)
                            {
                                lv_obj_t *dropdown = lv_event_get_target_obj(e);
                                lv_event_code_t code = lv_event_get_code(e);
                                
                                if (code == LV_EVENT_CLICKED) 
                                {
                                    // // 强制下拉列表向下打开
                                    // lv_dropdown_set_dir(dropdown, LV_DIR_BOTTOM);
                                    // 获取弹出的下拉列表对象
                                    lv_obj_t *list = lv_dropdown_get_list(dropdown);
                                    // // 设置下拉列表最多显示100高度
                                    // lv_obj_set_height(list, 300);
                                    lv_obj_set_style_bg_color(list, lv_color_hex(0xEEE9E9), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);
                                    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT);
                                } }, LV_EVENT_ALL, NULL);

        lv_obj_t *bandwidth_unit_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(bandwidth_unit_text, "hz");
        lv_obj_set_size(bandwidth_unit_text, 70, 40);
        lv_obj_set_style_text_font(bandwidth_unit_text, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(bandwidth_unit_text, _registry.win.lora.setings.config_lora_params.dropdown.bandwidth, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);

        lv_obj_t *msgbox_current_limit_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_current_limit_text, "current limit");
        lv_obj_set_size(msgbox_current_limit_text, 300, 40);
        lv_obj_set_style_text_font(msgbox_current_limit_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_current_limit_text, _registry.win.lora.setings.config_lora_params.dropdown.bandwidth, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.config_lora_params.textarea.current_limit = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.textarea.current_limit, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.textarea.current_limit, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.textarea.current_limit, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.textarea.current_limit, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.setings.config_lora_params.textarea.current_limit, 300);
        lv_textarea_set_one_line(_registry.win.lora.setings.config_lora_params.textarea.current_limit, true);
        char current_limit_str[10];
        snprintf(current_limit_str, sizeof(current_limit_str), "%.1f", _device_lora.params.current_limit);
        lv_textarea_set_text(_registry.win.lora.setings.config_lora_params.textarea.current_limit, current_limit_str); // 设置初始内容
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.textarea.current_limit, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.textarea.current_limit, msgbox_current_limit_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.textarea.current_limit, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.current_limit);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.current_limit);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;

                                // case LV_EVENT_DEFOCUSED:
                                //         lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                //     break;
                                case LV_EVENT_READY:
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                        lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);

                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_t *current_limit_unit_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(current_limit_unit_text, "ma");
        lv_obj_set_size(current_limit_unit_text, 70, 40);
        lv_obj_set_style_text_font(current_limit_unit_text, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(current_limit_unit_text, _registry.win.lora.setings.config_lora_params.textarea.current_limit, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);

        lv_obj_t *msgbox_power_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_power_text, "power");
        lv_obj_set_size(msgbox_power_text, 100, 40);
        lv_obj_set_style_text_font(msgbox_power_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_power_text, _registry.win.lora.setings.config_lora_params.textarea.current_limit, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.config_lora_params.textarea.power = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.textarea.power, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.textarea.power, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.textarea.power, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.textarea.power, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.setings.config_lora_params.textarea.power, 300);
        lv_textarea_set_one_line(_registry.win.lora.setings.config_lora_params.textarea.power, true);
        char power_str[10];
        snprintf(power_str, sizeof(power_str), "%d", _device_lora.params.power);
        lv_textarea_set_text(_registry.win.lora.setings.config_lora_params.textarea.power, power_str); // 设置初始内容
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.textarea.power, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.textarea.power, msgbox_power_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.textarea.power, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.power);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.power);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;

                                // case LV_EVENT_DEFOCUSED:
                                //         lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                //     break;
                                case LV_EVENT_READY:
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                        lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);

                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_t *power_unit_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(power_unit_text, "dbm");
        lv_obj_set_size(power_unit_text, 70, 40);
        lv_obj_set_style_text_font(power_unit_text, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(power_unit_text, _registry.win.lora.setings.config_lora_params.textarea.power, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);

        lv_obj_t *msgbox_spreading_factor_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_spreading_factor_text, "spreading factor");
        lv_obj_set_size(msgbox_spreading_factor_text, 300, 40);
        lv_obj_set_style_text_font(msgbox_spreading_factor_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_spreading_factor_text, _registry.win.lora.setings.config_lora_params.textarea.power, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.config_lora_params.dropdown.spreading_factor = lv_dropdown_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_dropdown_set_dir(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, LV_DIR_BOTTOM);
        lv_dropdown_set_options(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, "SF5\n"
                                                                                                         "SF6\n"
                                                                                                         "SF7\n"
                                                                                                         "SF8\n"
                                                                                                         "SF9\n"
                                                                                                         "SF10\n"
                                                                                                         "SF11\n"
                                                                                                         "SF12");
        lv_dropdown_set_selected(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, static_cast<uint32_t>(_device_lora.params.sf) - 5);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_min_width(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_min_height(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);  // 输入框字体
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT); // 下拉列表字体
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, msgbox_spreading_factor_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, [](lv_event_t *e)
                            {
                                lv_obj_t *dropdown = lv_event_get_target_obj(e);
                                lv_event_code_t code = lv_event_get_code(e);
                                
                                if (code == LV_EVENT_CLICKED) 
                                {
                                    // // 强制下拉列表向下打开
                                    // lv_dropdown_set_dir(dropdown, LV_DIR_BOTTOM);
                                    // 获取弹出的下拉列表对象
                                    lv_obj_t *list = lv_dropdown_get_list(dropdown);
                                    // // 设置下拉列表最多显示100高度
                                    // lv_obj_set_height(list, 300);

                                    lv_obj_set_style_bg_color(list, lv_color_hex(0xEEE9E9), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);
                                    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT);
                                } }, LV_EVENT_ALL, NULL);

        lv_obj_t *msgbox_coding_rate_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_coding_rate_text, "coding rate");
        lv_obj_set_size(msgbox_coding_rate_text, 300, 40);
        lv_obj_set_style_text_font(msgbox_coding_rate_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_coding_rate_text, _registry.win.lora.setings.config_lora_params.dropdown.spreading_factor, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.config_lora_params.dropdown.coding_rate = lv_dropdown_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_dropdown_set_dir(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, LV_DIR_BOTTOM);
        lv_dropdown_set_options(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, "CR_4_5\n"
                                                                                                    "CR_4_6\n"
                                                                                                    "CR_4_7\n"
                                                                                                    "CR_4_8");
        lv_dropdown_set_selected(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, static_cast<uint32_t>(_device_lora.params.cr) - 1);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_min_width(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_min_height(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);  // 输入框字体
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT); // 下拉列表字体
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, msgbox_coding_rate_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.dropdown.coding_rate, [](lv_event_t *e)
                            {
                                lv_obj_t *dropdown = lv_event_get_target_obj(e);
                                lv_event_code_t code = lv_event_get_code(e);
                                
                                if (code == LV_EVENT_CLICKED) 
                                {
                                    // // 强制下拉列表向下打开
                                    // lv_dropdown_set_dir(dropdown, LV_DIR_BOTTOM);
                                    // 获取弹出的下拉列表对象
                                    lv_obj_t *list = lv_dropdown_get_list(dropdown);
                                    // // 设置下拉列表最多显示100高度
                                    // lv_obj_set_height(list, 300);

                                    lv_obj_set_style_bg_color(list, lv_color_hex(0xEEE9E9), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);
                                    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT);
                                } }, LV_EVENT_ALL, NULL);

        lv_obj_t *msgbox_crc_type_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_crc_type_text, "crc type");
        lv_obj_set_size(msgbox_crc_type_text, 300, 40);
        lv_obj_set_style_text_font(msgbox_crc_type_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_crc_type_text, _registry.win.lora.setings.config_lora_params.dropdown.coding_rate, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.config_lora_params.dropdown.crc_type = lv_dropdown_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_dropdown_set_dir(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, LV_DIR_BOTTOM);
        lv_dropdown_set_options(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, "OFF\n"
                                                                                                 "ON");
        lv_dropdown_set_selected(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, static_cast<uint32_t>(_device_lora.params.crc_type));
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_min_width(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_min_height(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);  // 输入框字体
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT); // 下拉列表字体
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, msgbox_crc_type_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.dropdown.crc_type, [](lv_event_t *e)
                            {
                                lv_obj_t *dropdown = lv_event_get_target_obj(e);
                                lv_event_code_t code = lv_event_get_code(e);
                                
                                if (code == LV_EVENT_CLICKED) 
                                {
                                    // // 强制下拉列表向下打开
                                    // lv_dropdown_set_dir(dropdown, LV_DIR_BOTTOM);
                                    // 获取弹出的下拉列表对象
                                    lv_obj_t *list = lv_dropdown_get_list(dropdown);
                                    // // 设置下拉列表最多显示100高度
                                    // lv_obj_set_height(list, 300);

                                    lv_obj_set_style_bg_color(list, lv_color_hex(0xEEE9E9), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);
                                    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, LV_PART_ITEMS | LV_STATE_DEFAULT);
                                } }, LV_EVENT_ALL, NULL);

        lv_obj_t *msgbox_preamble_length = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_preamble_length, "preamble length");
        lv_obj_set_size(msgbox_preamble_length, 300, 40);
        lv_obj_set_style_text_font(msgbox_preamble_length, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_preamble_length, _registry.win.lora.setings.config_lora_params.dropdown.crc_type, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.config_lora_params.textarea.preamble_length = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, 300);
        lv_textarea_set_one_line(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, true);
        char preamble_length_str[10];
        snprintf(preamble_length_str, sizeof(preamble_length_str), "%d", _device_lora.params.preamble_length);
        lv_textarea_set_text(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, preamble_length_str); // 设置初始内容
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, msgbox_preamble_length, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.textarea.preamble_length, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.preamble_length);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.preamble_length);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;

                                // case LV_EVENT_DEFOCUSED:
                                //         lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                //     break;
                                case LV_EVENT_READY:
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                        lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);

                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_t *msgbox_sync_word = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_sync_word, "sync word");
        lv_obj_set_size(msgbox_sync_word, 300, 40);
        lv_obj_set_style_text_font(msgbox_sync_word, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_sync_word, _registry.win.lora.setings.config_lora_params.textarea.preamble_length, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.config_lora_params.textarea.sync_word = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.config_lora_params.textarea.sync_word, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.config_lora_params.textarea.sync_word, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.config_lora_params.textarea.sync_word, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.config_lora_params.textarea.sync_word, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.setings.config_lora_params.textarea.sync_word, 300);
        lv_textarea_set_one_line(_registry.win.lora.setings.config_lora_params.textarea.sync_word, true);
        char sync_word_str[10];
        snprintf(sync_word_str, sizeof(sync_word_str), "%d", _device_lora.params.sync_word);
        lv_textarea_set_text(_registry.win.lora.setings.config_lora_params.textarea.sync_word, sync_word_str); // 设置初始内容
        lv_obj_set_style_text_font(_registry.win.lora.setings.config_lora_params.textarea.sync_word, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.lora.setings.config_lora_params.textarea.sync_word, msgbox_sync_word, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.config_lora_params.textarea.sync_word, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.sync_word);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.config_lora_params.textarea.sync_word);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;

                                // case LV_EVENT_DEFOCUSED:
                                //         lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                //     break;
                                case LV_EVENT_READY:
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                        lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);

                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);
    }

    void System::init_win_lora_setings_auto_send_message_box(void)
    {
        // 创建全屏灰色透明遮罩，禁止触摸
        _registry.win.lora.setings.message_box.root = lv_obj_create(_registry.win.lora.setings.root);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.root, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.root, _width, _height);
        lv_obj_set_style_bg_color(_registry.win.lora.setings.message_box.root, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_registry.win.lora.setings.message_box.root, LV_OPA_50, LV_PART_MAIN);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.root, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(_registry.win.lora.setings.message_box.root, 0, LV_PART_MAIN);
        lv_obj_align(_registry.win.lora.setings.message_box.root, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(_registry.win.lora.setings.message_box.root, LV_OBJ_FLAG_CLICKABLE); // 禁止触摸

        _registry.win.lora.setings.message_box.root_container = lv_obj_create(_registry.win.lora.setings.message_box.root);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.root_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.root_container, 450, 900);
        lv_obj_set_style_radius(_registry.win.lora.setings.message_box.root_container, 15, LV_PART_MAIN);
        lv_obj_set_style_bg_color(_registry.win.lora.setings.message_box.root_container, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.root_container, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(_registry.win.lora.setings.message_box.root_container, 16, LV_PART_MAIN);
        lv_obj_center(_registry.win.lora.setings.message_box.root_container);

        _registry.win.lora.setings.message_box.btn_container = lv_obj_create(_registry.win.lora.setings.message_box.root_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.btn_container, 450, 110);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.btn_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_align(_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);

        lv_obj_t *btn_cancel = lv_button_create(_registry.win.lora.setings.message_box.btn_container);
        lv_obj_set_size(btn_cancel, 150, 60);
        lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT, 40, -30);
        lv_obj_set_style_radius(btn_cancel, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn_cancel, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 去除按钮阴影
        lv_obj_t *btn_cancel_label = lv_label_create(btn_cancel);
        lv_label_set_text(btn_cancel_label, "cancel");
        lv_obj_set_style_text_font(btn_cancel_label, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_center(btn_cancel_label);

        // cancel 按钮回调
        lv_obj_add_event_cb(btn_cancel, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_obj_delete(self->_registry.win.lora.setings.message_box.root); }, LV_EVENT_CLICKED, this);

        lv_obj_t *btn_apply = lv_button_create(_registry.win.lora.setings.message_box.btn_container);
        lv_obj_set_size(btn_apply, 150, 60);
        lv_obj_align(btn_apply, LV_ALIGN_BOTTOM_RIGHT, -40, -30);
        lv_obj_set_style_radius(btn_apply, 10, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(btn_apply, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT); // 去除按钮阴影
        lv_obj_t *btn_apply_label = lv_label_create(btn_apply);
        lv_label_set_text(btn_apply_label, "apply");
        lv_obj_set_style_text_font(btn_apply_label, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_center(btn_apply_label);

        // apply 按钮回调
        lv_obj_add_event_cb(btn_apply, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));

                                self->_device_lora.auto_send.flag = lv_obj_has_state(self->_registry.win.lora.setings.auto_send.control_switch, LV_STATE_CHECKED);
                                
                                const char* auto_send_text = lv_textarea_get_text(self->_registry.win.lora.setings.auto_send.textarea.auto_send_text);
                                if((auto_send_text != nullptr) && (auto_send_text[0] != '\0'))  // 同时检查NULL和空字符串
                                {
                                    self->_device_lora.auto_send.text = auto_send_text;
                                }

                                const char* auto_send_interval_text = lv_textarea_get_text(self->_registry.win.lora.setings.auto_send.textarea.auto_send_interval);
                                if (auto_send_interval_text != nullptr && auto_send_interval_text[0] != '\0') // 同时检查NULL和空字符串
                                {  
                                    uint32_t buffer = std::stoi(auto_send_interval_text);  

                                    // 限制范围
                                    if((buffer >= 1) && (buffer <= 1000000))
                                    {
                                        self->_device_lora.auto_send.interval = buffer;
                                    }
                                }

                                lv_obj_delete(self->_registry.win.lora.setings.message_box.root); }, LV_EVENT_CLICKED, this);

        _registry.keyboard = lv_keyboard_create(_registry.win.lora.setings.message_box.root);
        lv_obj_set_size(_registry.keyboard, _width, _height / 3.5);
        lv_obj_set_style_radius(_registry.keyboard, 8, (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_DEFAULT);
        // 设置键盘按钮间距更密集
        lv_obj_set_style_pad_row(_registry.keyboard, 8, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(_registry.keyboard, 4, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.keyboard, &lv_font_montserrat_26, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_align(_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部
        lv_obj_add_flag(_registry.keyboard, LV_OBJ_FLAG_HIDDEN);     // 初始隐藏键盘

        _registry.win.lora.setings.message_box.parameter_container = lv_obj_create(_registry.win.lora.setings.message_box.root_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.message_box.parameter_container, 30, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_size(_registry.win.lora.setings.message_box.parameter_container, 450, 780);
        lv_obj_set_style_border_width(_registry.win.lora.setings.message_box.parameter_container, 0, (lv_style_selector_t)LV_PART_MAIN); // 移除边框
        lv_obj_set_scrollbar_mode(_registry.win.lora.setings.message_box.parameter_container, LV_SCROLLBAR_MODE_ACTIVE);
        lv_obj_align_to(_registry.win.lora.setings.message_box.parameter_container, _registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

        // 触摸lora设置消息框区域时隐藏键盘
        lv_obj_add_event_cb(_registry.win.lora.setings.message_box.parameter_container, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_CLICKED)
                                {
                                    lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                    // 调整聊天框的大小
                                    lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                    lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);
                                    lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                    lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 780);
                                    lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                } }, LV_EVENT_ALL, this);

        lv_obj_t *msgbox_freq_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_freq_text, "auto send control");
        lv_obj_set_size(msgbox_freq_text, 280, 30);
        lv_obj_set_style_text_font(msgbox_freq_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(msgbox_freq_text, LV_ALIGN_TOP_LEFT, 0, 0);

        _registry.win.lora.setings.auto_send.control_switch = lv_switch_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_size(_registry.win.lora.setings.auto_send.control_switch, 90, 50);
        lv_obj_align_to(_registry.win.lora.setings.auto_send.control_switch, msgbox_freq_text, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        if (_device_lora.auto_send.flag == true)
        {
            lv_obj_add_state(_registry.win.lora.setings.auto_send.control_switch, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_remove_state(_registry.win.lora.setings.auto_send.control_switch, LV_STATE_CHECKED);
        }

        lv_obj_t *msgbox_auto_send_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_auto_send_text, "auto send text");
        lv_obj_set_size(msgbox_auto_send_text, 300, 40);
        lv_obj_set_style_text_font(msgbox_auto_send_text, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_auto_send_text, msgbox_freq_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);

        _registry.win.lora.setings.auto_send.textarea.auto_send_text = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.auto_send.textarea.auto_send_text, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.auto_send.textarea.auto_send_text, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.auto_send.textarea.auto_send_text, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.auto_send.textarea.auto_send_text, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.setings.auto_send.textarea.auto_send_text, 300);
        lv_textarea_set_one_line(_registry.win.lora.setings.auto_send.textarea.auto_send_text, true);
        lv_textarea_set_text(_registry.win.lora.setings.auto_send.textarea.auto_send_text, _device_lora.auto_send.text.c_str()); // 设置初始内容
        lv_obj_set_style_text_font(_registry.win.lora.setings.auto_send.textarea.auto_send_text, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.lora.setings.auto_send.textarea.auto_send_text, msgbox_auto_send_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.auto_send.textarea.auto_send_text, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.auto_send.textarea.auto_send_text);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.auto_send.textarea.auto_send_text);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;

                                // case LV_EVENT_DEFOCUSED:
                                //         lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                //     break;
                                case LV_EVENT_READY:
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                        lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);

                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_t *msgbox_auto_send_interval = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(msgbox_auto_send_interval, "auto send interval");
        lv_obj_set_size(msgbox_auto_send_interval, 300, 40);
        lv_obj_set_style_text_font(msgbox_auto_send_interval, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(msgbox_auto_send_interval, _registry.win.lora.setings.auto_send.textarea.auto_send_text, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        _registry.win.lora.setings.auto_send.textarea.auto_send_interval = lv_textarea_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_obj_set_style_pad_top(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, 15, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_set_width(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, 300);
        lv_textarea_set_one_line(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, true);
        char auto_send_interval_str[10];
        snprintf(auto_send_interval_str, sizeof(auto_send_interval_str), "%ld", _device_lora.auto_send.interval);
        lv_textarea_set_text(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, auto_send_interval_str); // 设置初始内容
        lv_obj_set_style_text_font(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, msgbox_auto_send_interval, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

        lv_obj_add_event_cb(_registry.win.lora.setings.auto_send.textarea.auto_send_interval, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                switch (code)
                                {
                                case LV_EVENT_CLICKED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.auto_send.textarea.auto_send_interval);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;
                                case LV_EVENT_FOCUSED:
                                        lv_keyboard_set_textarea(self->_registry.keyboard, self->_registry.win.lora.setings.auto_send.textarea.auto_send_interval);
                                        lv_obj_remove_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
                                        lv_obj_align(self->_registry.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0); // 对齐到屏幕底部

                                        // 调整消息框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 800);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.root_container, self->_registry.keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0); 
                                        lv_obj_align(self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_BOTTOM_MID, 0, 0);
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.parameter_container, 450, 680);
                                        lv_obj_align_to(self->_registry.win.lora.setings.message_box.parameter_container, self->_registry.win.lora.setings.message_box.btn_container, LV_ALIGN_OUT_TOP_MID, 0, 0);

                                    break;

                                // case LV_EVENT_DEFOCUSED:
                                //         lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                //     break;
                                case LV_EVENT_READY:
                                        // printf("Ready, current text: %s", lv_textarea_get_text(ta));

                                        lv_obj_add_flag(self->_registry.keyboard, LV_OBJ_FLAG_HIDDEN); // 隐藏键盘

                                        // 调整聊天框的大小
                                        lv_obj_set_size(self->_registry.win.lora.setings.message_box.root_container, 450, 900);
                                        lv_obj_center(self->_registry.win.lora.setings.message_box.root_container);

                                    break;
                                
                                default:
                                    break;
                                } }, LV_EVENT_ALL, this);

        lv_obj_t *auto_send_interval_unit_text = lv_label_create(_registry.win.lora.setings.message_box.parameter_container);
        lv_label_set_text(auto_send_interval_unit_text, "ms");
        lv_obj_set_size(auto_send_interval_unit_text, 70, 40);
        lv_obj_set_style_text_font(auto_send_interval_unit_text, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align_to(auto_send_interval_unit_text, _registry.win.lora.setings.auto_send.textarea.auto_send_interval, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);
    }

    bool System::set_config_lora_params(Device_Lora device_lora)
    {
        if (_win_lora_config_lora_params_callback != nullptr)
        {
            if (_win_lora_config_lora_params_callback(device_lora) == true)
            {
                return true;
            }
        }

        return false;
    }

    void System::set_lora_send_data_callback(std::string data)
    {
        if (_win_lora_send_data_callback != nullptr)
        {
            _win_lora_send_data_callback(data);
        }
    }

    void System::set_lora_status_callback(bool status)
    {
        if (_win_lora_status_callback != nullptr)
        {
            _win_lora_status_callback(status);
        }
    }

    void System::init_win_music(void)
    {
        // 主界面
        _registry.win.music.root = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_registry.win.music.root, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN);
#if defined CONFIG_SCREEN_TYPE_HI8561
        lv_obj_set_style_bg_image_src(_registry.win.music.root, GET_MUSIC_COVER_PATH("Eagles - Hotel California (Live on MTV, 1994)_540x1168px.png"), (lv_style_selector_t)LV_PART_MAIN);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
        lv_obj_set_style_bg_image_src(_registry.win.music.root, GET_MUSIC_COVER_PATH("Eagles - Hotel California (Live on MTV, 1994)_568x1232px.png"), (lv_style_selector_t)LV_PART_MAIN);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

        lv_obj_set_size(_registry.win.music.root, _width, _height);
        lv_obj_set_scrollbar_mode(_registry.win.music.root, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *song_name_btn = lv_button_create(_registry.win.music.root);
        lv_obj_set_size(song_name_btn, 270, 60);
        lv_obj_set_style_radius(song_name_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(song_name_btn, LV_ALIGN_TOP_LEFT, 30, 590);
        lv_obj_set_style_bg_color(song_name_btn, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(song_name_btn, 60, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(song_name_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *song_name_label = lv_label_create(song_name_btn);
        lv_label_set_text(song_name_label, "Hotel California");
        lv_obj_set_style_text_color(song_name_label, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(song_name_label, &lvgl_font_misans_bold_27, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(song_name_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(song_name_label);

        lv_obj_t *artist_btn = lv_button_create(_registry.win.music.root);
        lv_obj_set_size(artist_btn, 130, 50);
        lv_obj_set_style_radius(artist_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(artist_btn, LV_ALIGN_TOP_LEFT, 30, 660);
        lv_obj_set_style_bg_color(artist_btn, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(artist_btn, 60, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(artist_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *artist_label = lv_label_create(artist_btn);
        lv_label_set_text(artist_label, "Eagles");
        lv_obj_set_style_text_color(artist_label, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(artist_label, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(song_name_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(artist_label);

        // 创建播放按键图片按钮
        _registry.win.music.imagebutton.play = lv_imagebutton_create(_registry.win.music.root);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.play, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &win_music_play_start_1_140x140px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.play, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &win_music_play_start_2_140x140px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.play, LV_IMAGEBUTTON_STATE_DISABLED, NULL, &win_music_play_start_1_140x140px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.play, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &win_music_play_pause_1_117x117px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.play, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &win_music_play_pause_2_117x117px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.play, LV_IMAGEBUTTON_STATE_CHECKED_DISABLED, NULL, &win_music_play_pause_1_117x117px_rgb565a8, NULL);
        // 设置按钮大小和位置
        // lv_obj_set_size(_registry.win.music.imagebutton.play, 140, 140);
        // lv_obj_align(_registry.win.music.imagebutton.play, LV_ALIGN_BOTTOM_MID, 0, -120);

        // 添加点击事件切换播放/暂停状态
        lv_obj_add_event_cb(_registry.win.music.imagebutton.play, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_obj_t *btn = lv_event_get_target_obj(e);
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_CLICKED) 
                                {
                                    if (lv_obj_has_state(btn, LV_STATE_CHECKED)) 
                                    {
                                        self->_registry.win.music.play_flag = false;

                                        self->set_win_music_play_imagebutton_status(self->_registry.win.music.play_flag);
                                    }
                                    else // 按键点击 切换为播放模式
                                    {
                                        self->_registry.win.music.play_flag = true;

                                        self->set_win_music_play_imagebutton_status(self->_registry.win.music.play_flag);

                                        self->set_music_start_end(true);
                                    }
                                } }, LV_EVENT_ALL, this);

        // 创建左切换按键图片按钮
        _registry.win.music.imagebutton.switch_left = lv_imagebutton_create(_registry.win.music.root);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.switch_left, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &win_music_play_switch_left_1_95x95px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.switch_left, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &win_music_play_switch_left_2_95x95px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.switch_left, LV_IMAGEBUTTON_STATE_DISABLED, NULL, &win_music_play_switch_left_1_95x95px_rgb565a8, NULL);
        // 设置按钮大小和位置
        lv_obj_set_size(_registry.win.music.imagebutton.switch_left, 95, 95);
        // lv_obj_align_to(_registry.win.music.imagebutton.switch_left, _registry.win.music.imagebutton.play, LV_ALIGN_OUT_LEFT_MID, -10, 0);

        // 创建右切换按键图片按钮
        _registry.win.music.imagebutton.switch_right = lv_imagebutton_create(_registry.win.music.root);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.switch_right, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &win_music_play_switch_right_1_95x95px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.switch_right, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &win_music_play_switch_right_2_95x95px_rgb565a8, NULL);
        lv_imagebutton_set_src(_registry.win.music.imagebutton.switch_right, LV_IMAGEBUTTON_STATE_DISABLED, NULL, &win_music_play_switch_right_1_95x95px_rgb565a8, NULL);
        // 设置按钮大小和位置
        lv_obj_set_size(_registry.win.music.imagebutton.switch_right, 95, 95);
        // lv_obj_align_to(_registry.win.music.imagebutton.switch_right, _registry.win.music.imagebutton.play, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

        // 创建左侧当前播放时间按钮
        lv_obj_t *current_time_btn = lv_button_create(_registry.win.music.root);
        lv_obj_set_size(current_time_btn, 90, 40);
        lv_obj_set_style_bg_color(current_time_btn, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(current_time_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(current_time_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(current_time_btn, LV_ALIGN_TOP_LEFT, 30, 795);

        _registry.win.music.label.current_time = lv_label_create(current_time_btn);
        // lv_label_set_text(_registry.win.music.label.current_time, "00:00");
        lv_obj_set_style_text_color(_registry.win.music.label.current_time, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.music.label.current_time, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(_registry.win.music.label.current_time);

        // 创建右侧总时长按钮
        lv_obj_t *total_time_btn = lv_button_create(_registry.win.music.root);
        lv_obj_set_size(total_time_btn, 90, 40);
        lv_obj_set_style_bg_color(total_time_btn, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(total_time_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(total_time_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(total_time_btn, LV_ALIGN_TOP_RIGHT, -30, 795);

        _registry.win.music.label.total_time = lv_label_create(total_time_btn);
        // lv_label_set_text(_registry.win.music.label.total_time, "04:20");
        lv_obj_set_style_text_color(_registry.win.music.label.total_time, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(_registry.win.music.label.total_time, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(_registry.win.music.label.total_time);

        // 创建音乐滚动条
        static lv_style_t style_indic;
        lv_style_init(&style_indic);
        lv_style_set_bg_color(&style_indic, lv_color_white());
        lv_style_set_bg_grad_color(&style_indic, lv_color_white());
        lv_style_set_bg_grad_dir(&style_indic, LV_GRAD_DIR_HOR);

        static lv_style_t style_indic_pr;
        lv_style_init(&style_indic_pr);
        lv_style_set_shadow_color(&style_indic_pr, lv_color_white());
        lv_style_set_shadow_width(&style_indic_pr, 4);
        lv_style_set_shadow_spread(&style_indic_pr, 3);

        _registry.win.music.slider = lv_slider_create(_registry.win.music.root);
        lv_obj_set_style_bg_color(_registry.win.music.slider, lv_color_black(), LV_PART_MAIN); // 设置进度条背景（未填充部分）为灰色
        // 去除蓝色圆形按钮
        lv_obj_set_style_pad_left(_registry.win.music.slider, 0, LV_PART_KNOB);
        lv_obj_set_style_pad_right(_registry.win.music.slider, 0, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(_registry.win.music.slider, LV_OPA_COVER, LV_PART_KNOB);
        lv_obj_set_style_bg_color(_registry.win.music.slider, lv_color_white(), LV_PART_KNOB);
        lv_obj_set_style_radius(_registry.win.music.slider, 50, LV_PART_KNOB);
        lv_obj_set_style_width(_registry.win.music.slider, 8, LV_PART_KNOB);
        lv_obj_set_style_height(_registry.win.music.slider, 8, LV_PART_KNOB);
        lv_obj_set_size(_registry.win.music.slider, 460, 8);
        lv_obj_add_style(_registry.win.music.slider, &style_indic, LV_PART_INDICATOR);
        lv_obj_add_style(_registry.win.music.slider, &style_indic_pr, LV_PART_INDICATOR | LV_STATE_PRESSED);
        // lv_slider_set_value(_registry.win.music.slider, 0, LV_ANIM_OFF);
        lv_obj_align(_registry.win.music.slider, LV_ALIGN_TOP_LEFT, 40, 860);

        lv_obj_add_event_cb(_registry.win.music.slider, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_RELEASED)
                                {
                                    int32_t  percent = lv_slider_get_value(self->_registry.win.music.slider);
                                    // printf("slider percent: %d\n", percent);

                                    double set_current_time_s = self->_registry.win.music.total_time_s * (static_cast<double>(percent) / 100.0);

                                    self->set_music_current_time_s(set_current_time_s);

                                    self->set_win_music_current_total_time(set_current_time_s, self->_registry.win.music.total_time_s);
                                } }, LV_EVENT_ALL, this);

        // 创建底部黑色长条按钮
        lv_obj_t *bottom_bar_btn = lv_button_create(_registry.win.music.root);
        lv_obj_set_size(bottom_bar_btn, _width - 60, 70);
        lv_obj_align(bottom_bar_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
        lv_obj_set_style_bg_color(bottom_bar_btn, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(bottom_bar_btn, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(bottom_bar_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 长条按钮添加一个标签
        lv_obj_t *bottom_bar_label = lv_label_create(bottom_bar_btn);
        lv_label_set_text(bottom_bar_label, "Music");
        lv_obj_set_style_text_color(bottom_bar_label, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(bottom_bar_label, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(bottom_bar_label);

        set_win_music_play_imagebutton_status(_registry.win.music.play_flag);

        set_win_music_current_total_time(_registry.win.music.current_time_s, _registry.win.music.total_time_s);

        lv_obj_add_event_cb(_registry.win.music.root, [](lv_event_t *e)
                            {
                                System *self = static_cast<System *>(lv_event_get_user_data(e));
                                lv_event_code_t code = lv_event_get_code(e);

                                if (code == LV_EVENT_GESTURE)
                                {
                                    lv_dir_t gesture_dir = lv_indev_get_gesture_dir(lv_indev_active());

                                    // 边缘检测以及左右滑动
                                    if ((gesture_dir == LV_DIR_LEFT || gesture_dir == LV_DIR_RIGHT)&&(self->_edge_touch_flag == true))
                                    {   
                                        self->set_vibration();
                                        self->init_win_home();
                                        
                                        lv_screen_load_anim(self->_registry.win.home.root, LV_SCR_LOAD_ANIM_FADE_OUT, 100, 0, true);

                                        self->_edge_touch_flag = false;
                                    }
                                } }, LV_EVENT_ALL, this);

        init_status_bar(_registry.win.music.root);

        lv_obj_update_layout(_registry.win.music.root);

        set_music_start_end(true);

        _current_win = Current_Win::MUSIC;
    }

    void System::set_win_music_play_imagebutton_status(bool status)
    {
        if (status == false)
        {
            lv_obj_remove_state(_registry.win.music.imagebutton.play, LV_STATE_CHECKED);

            lv_obj_set_size(_registry.win.music.imagebutton.play, 140, 140);
            lv_obj_align(_registry.win.music.imagebutton.play, LV_ALIGN_BOTTOM_MID, 0, -130);
            lv_obj_align_to(_registry.win.music.imagebutton.switch_left, _registry.win.music.imagebutton.play, LV_ALIGN_OUT_LEFT_MID, -10, 0);
            lv_obj_align_to(_registry.win.music.imagebutton.switch_right, _registry.win.music.imagebutton.play, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        }
        else
        {
            lv_obj_add_state(_registry.win.music.imagebutton.play, LV_STATE_CHECKED);

            lv_obj_set_size(_registry.win.music.imagebutton.play, 117, 117);
            lv_obj_align(_registry.win.music.imagebutton.play, LV_ALIGN_BOTTOM_MID, 0, -142);
            lv_obj_align_to(_registry.win.music.imagebutton.switch_left, _registry.win.music.imagebutton.play, LV_ALIGN_OUT_LEFT_MID, -10, 0);
            lv_obj_align_to(_registry.win.music.imagebutton.switch_right, _registry.win.music.imagebutton.play, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        }
    }

    void System::set_win_music_current_total_time(double current_time_s, double total_time_s)
    {
        uint8_t sliding_percentage = 0;

        if (current_time_s == 0)
        {
            sliding_percentage = 0;
            _registry.win.music.current_time_s = 0;
            _registry.win.music.total_time_s = total_time_s;
        }
        else if (current_time_s > total_time_s)
        {
            sliding_percentage = 100;
            _registry.win.music.current_time_s = total_time_s;
            _registry.win.music.total_time_s = total_time_s;
        }
        else
        {
            sliding_percentage = (current_time_s / total_time_s) * 100;
            _registry.win.music.current_time_s = current_time_s;
            _registry.win.music.total_time_s = total_time_s;
        }

        // 格式化 current_time_s 和 total_time_s 格式
        char current_time_s_str[20];
        char total_time_s_str[20];
        snprintf(current_time_s_str, sizeof(current_time_s_str), "%ld:%02ld", static_cast<uint32_t>(_registry.win.music.current_time_s) / 60,
                 static_cast<uint32_t>(_registry.win.music.current_time_s) % 60);
        snprintf(total_time_s_str, sizeof(total_time_s_str), "%ld:%02ld", static_cast<uint32_t>(_registry.win.music.total_time_s) / 60,
                 static_cast<uint32_t>(_registry.win.music.total_time_s) % 60);

        lv_label_set_text(_registry.win.music.label.current_time, current_time_s_str);
        lv_label_set_text(_registry.win.music.label.total_time, total_time_s_str);
        lv_slider_set_value(_registry.win.music.slider, sliding_percentage, LV_ANIM_OFF);
    }

    void System::set_music_start_end(bool status)
    {
        if (_win_music_start_end_callback != nullptr)
        {
            _win_music_start_end_callback(status);
        }
    }

    void System::set_music_current_time_s(double current_time_s)
    {
        if (_set_music_current_time_s_callback != nullptr)
        {
            _set_music_current_time_s_callback(current_time_s);
        }
    }

};
