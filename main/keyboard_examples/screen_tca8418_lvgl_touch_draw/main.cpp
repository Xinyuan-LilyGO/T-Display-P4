/*
 * @Description: screen_lvgl_touch_draw
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:35:38
 * @LastEditTime: 2026-03-26 15:02:12
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "lvgl_keyboard_config.h"
#include "lvgl.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

#define LVGL_TICK_PERIOD_MS 1

size_t Cycle_Time = 0;

esp_lcd_panel_handle_t Screen_Mipi_Dpi_Panel = NULL;

std::vector<uint16_t> Lvgl_Draw_X_Data;
std::vector<uint16_t> Lvgl_Draw_Y_Data;

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

// 定义一个计时器，用于检测 5 秒无操作
time_t last_touch_time = 0;

// 定义一个标志，用于判断是否需要清除画布
bool need_clear_lock_flag = false;

// 定义一个画布对象
static lv_obj_t *canvas;
static lv_layer_t layer;

lv_obj_t *Keyboard_Label;

lv_point_t point;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(XL9555_SDA, XL9555_SCL);
auto Tca8418_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Software_Iic>(TCA8418_SDA, TCA8418_SCL);

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

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);
auto Tca8418 = std::make_unique<Cpp_Bus_Driver::Tca8418>(Tca8418_Iic_Bus, TCA8418_IIC_ADDRESS);

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

#if defined CONFIG_SCREEN_TYPE_HI8561
auto Hi8561_Iic_Touch_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(HI8561_TOUCH_SDA, HI8561_TOUCH_SCL, I2C_NUM_0);

auto Hi8561_Touch = std::make_unique<Cpp_Bus_Driver::Hi8561_Touch>(Hi8561_Iic_Touch_Bus, HI8561_TOUCH_IIC_ADDRESS);

auto Screen = std::make_unique<Cpp_Bus_Driver::Hi8561>(Screen_Mipi_Bus);

#elif defined CONFIG_SCREEN_TYPE_RM69A10

auto Gt9895_Touch_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(GT9895_SDA, GT9895_SCL, I2C_NUM_0);

auto Gt9895_Touch = std::make_unique<Cpp_Bus_Driver::Gt9895>(Gt9895_Touch_Iic_Bus, Gt9895_Touch_IIC_ADDRESS, -1, GT9895_TOUCH_X_SCALE_FACTOR, GT9895_TOUCH_Y_SCALE_FACTOR);

auto Screen = std::make_unique<Cpp_Bus_Driver::Rm69a10>(Screen_Mipi_Bus);

#else
#error "no macro definition is set"
#endif

volatile bool Interrupt_Flag = false;

void lvgl_port_task(void *arg)
{
    printf("lvgl_ui_task start\n");

    while (1)
    {
        // _lock_acquire(&lvgl_api_lock);
        // time_till_next_ms = lv_timer_handler();
        // _lock_release(&lvgl_api_lock);

        // 获取当前时间
        time_t current_time = time(NULL);

        // 如果距离上次触摸时间超过 5 秒，则清除画布
        if ((current_time - last_touch_time > 5) && (need_clear_lock_flag == true))
        {
            lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_COVER);
            last_touch_time = current_time;

            Lvgl_Draw_X_Data.clear();
            Lvgl_Draw_Y_Data.clear();

            need_clear_lock_flag = false;
        }

        lv_timer_handler();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    // if (XL9535->pin_read(XL9535_TOUCH_INT) == 0)
    // {
#if defined CONFIG_SCREEN_TYPE_HI8561
    Cpp_Bus_Driver::Hi8561_Touch::Touch_Point tp;

    if (Hi8561_Touch->get_single_touch_point(tp) == true)
    {
        // printf("touch finger: %d edge touch flag: %d\nx: %d y: %d p: %d\n",
        //        tp.finger_count, tp.edge_touch_flag, tp.info[0].x, tp.info[0].y, tp.info[0].pressure_value);

        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = tp.info[0].x;
        data->point.y = tp.info[0].y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

#elif defined CONFIG_SCREEN_TYPE_RM69A10

    Cpp_Bus_Driver::Gt9895::Touch_Point tp;

    if (Gt9895->get_single_touch_point(tp) == true)
    {
        printf("touch finger: %d edge touch flag: %d\n id: %d x: %d y: %d p: %d\n",
               tp.finger_count, tp.edge_touch_flag, tp.info[0].finger_id, tp.info[0].x, tp.info[0].y, tp.info[0].pressure_value);

        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = tp.info[0].x;
        data->point.y = tp.info[0].y;
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

void my_keyboard_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint32_t last_key = 0; // 静态变量记录上一次按键
    static bool pressed_state_flag = false;
    static bool caps_lock_flag = false;

    if (Interrupt_Flag == true)
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
                    printf("touch finger: %d\n", tp.finger_count);

                    for (uint8_t i = 0; i < tp.info.size(); i++)
                    {
                        switch (tp.info[i].event_type)
                        {
                        case Cpp_Bus_Driver::Tca8418::Event_Type::KEYPAD:
                        {
                            Cpp_Bus_Driver::Tca8418::Touch_Position tp_2;
                            if (Tca8418->parse_touch_num(tp.info[i].num, tp_2) == true)
                            {
                                printf("keypad event\n");
                                printf("   touch num:[%d] num: %d x: %d y: %d press_flag: %d\n", i + 1, tp.info[i].num, tp_2.x, tp_2.y, tp.info[i].press_flag);
                                if (tp.info[i].num <= (sizeof(Tca8418_Map) / sizeof(std::string)))
                                {
                                    printf("   touch string: %s\n", Tca8418_Map[tp.info[i].num - 1].c_str());

                                    lv_label_set_text(Keyboard_Label, Tca8418_Map[tp.info[i].num - 1].c_str());
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
                                    pressed_state_flag = false;
                                }
                            }

                            break;
                        }
                        case Cpp_Bus_Driver::Tca8418::Event_Type::GPIO:
                            printf("gpio event\n");
                            printf("   touch num:[%d] num: %d press_flag: %d\n", i + 1, tp.info[i].num, tp.info[i].press_flag);
                            break;

                        default:
                            break;
                        }
                    }
                }

                Tca8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);
            }
        }

        Interrupt_Flag = false;
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

// 绘图回调函数
void draw_point(lv_event_t *e)
{
    // lv_obj_t *obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    // printf("code: %d\n", code);

    switch (code)
    {
    case LV_EVENT_PRESSING:
    {
        lv_indev_t *indev = lv_indev_get_act();
        lv_indev_get_point(indev, &point);

        // printf("touch x: %ld y: %ld\n", point.x, point.y);

        // 在画布上绘制点
        // lv_canvas_set_px(canvas, point.x, point.y, lv_palette_main(LV_PALETTE_RED), LV_OPA_COVER);

        Lvgl_Draw_X_Data.push_back(point.x);
        Lvgl_Draw_Y_Data.push_back(point.y);

        if ((Lvgl_Draw_X_Data.size() >= 2) && (Lvgl_Draw_Y_Data.size() >= 2))
        {
            lv_draw_line_dsc_t dsc;
            lv_draw_line_dsc_init(&dsc);
            dsc.color = lv_palette_main(LV_PALETTE_RED);
            dsc.width = 4;
            dsc.round_end = 1;
            dsc.round_start = 1;
            dsc.p1.x = Lvgl_Draw_X_Data[0];
            dsc.p1.y = Lvgl_Draw_Y_Data[0];
            dsc.p2.x = Lvgl_Draw_X_Data[1];
            dsc.p2.y = Lvgl_Draw_Y_Data[1];
            lv_draw_line(&layer, &dsc);

            lv_canvas_finish_layer(canvas, &layer);

            Lvgl_Draw_X_Data.erase(Lvgl_Draw_X_Data.begin());
            Lvgl_Draw_Y_Data.erase(Lvgl_Draw_Y_Data.begin());
        }

        // 获取当前时间
        time_t current_time = time(NULL);
        // 更新上次触摸时间
        last_touch_time = current_time;
        need_clear_lock_flag = true;
    }
    break;
    case LV_EVENT_RELEASED:
        //  printf("777\n");
        Lvgl_Draw_X_Data.clear();
        Lvgl_Draw_Y_Data.clear();
        break;

    default:
        break;
    }
}

void lv_example_canvas_7(void)
{
    void *draw_buf = NULL;
    size_t draw_buffer_sz = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
    draw_buf = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);

    /*Create a canvas and initialize its palette*/
    canvas = lv_canvas_create(lv_screen_active());
    // lv_canvas_set_draw_buf(canvas, (lv_draw_buf_t *)draw_buf);
    lv_canvas_set_buffer(canvas, draw_buf, SCREEN_WIDTH, SCREEN_HEIGHT, [](uint8_t format) -> lv_color_format_t
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

    lv_canvas_fill_bg(canvas, lv_color_hex3(0xCCC), LV_OPA_COVER);
    lv_obj_center(canvas);

    lv_canvas_init_layer(canvas, &layer);

    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = lv_palette_main(LV_PALETTE_RED);
    dsc.width = 4;
    dsc.round_end = 1;
    dsc.round_start = 1;
    dsc.p1.x = 15;
    dsc.p1.y = 15;
    dsc.p2.x = 35;
    dsc.p2.y = 10;
    lv_draw_line(&layer, &dsc);

    lv_canvas_finish_layer(canvas, &layer);

    // 注册触摸事件回调
    lv_obj_add_event_cb(lv_screen_active(), draw_point, LV_EVENT_ALL, NULL);
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
                                lv_area_t rotated_area;
                                if(rotation != LV_DISPLAY_ROTATION_0) 
                                {
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
                                    auto rotated_buf = std::make_unique<uint8_t[]>(SCREEN_WIDTH * SCREEN_HEIGHT* (SCREEN_BITS_PER_PIXEL/8));
                                    lv_draw_sw_rotate(px_map, rotated_buf.get(), src_w, src_h, src_stride, dest_stride, rotation, cf);
                                    /*Use the rotated area and rotated buffer from now on*/
                                    area = &rotated_area;
                                    px_map = rotated_buf.get();
                                }

#if defined CONFIG_SCREEN_TYPE_HI8561
                                auto screen = (Cpp_Bus_Driver::Hi8561 *)lv_display_get_user_data(disp);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
                                auto screen = (Cpp_Bus_Driver::Rm69a10 *)lv_display_get_user_data(disp);
#else
#error "no macro definition is set"
#endif

                                int offsetx1 = area->x1;
                                int offsetx2 = area->x2;
                                int offsety1 = area->y1;
                                int offsety2 = area->y2;
                                // pass the draw buffer to the driver
                                screen->send_color_stream_coordinate(offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map); });

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

    lv_indev_t *indev_2 = lv_indev_create();
    lv_indev_set_type(indev_2, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_2, my_keyboard_read);

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
}

void Lvgl_Keyboard_Init(void)
{
    // 创建一个半透明文本框
    lv_obj_t *ta = lv_textarea_create(canvas);
    lv_obj_set_width(ta, 300);
    lv_obj_set_height(ta, 150);
    lv_obj_align(ta, LV_ALIGN_BOTTOM_MID, -370, -50);
    // 设置背景为50%透明
    lv_obj_set_style_bg_opa(ta, LV_OPA_50, 0);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_24, 0);
    // 设置文本框不可触摸
    lv_obj_remove_flag(ta, LV_OBJ_FLAG_CLICKABLE);

    // 查找类型为KEYPAD的输入设备
    lv_indev_t *kb_indev = nullptr;
    lv_indev_t *indev_iter = lv_indev_get_next(NULL);
    while (indev_iter)
    {
        if (lv_indev_get_type(indev_iter) == LV_INDEV_TYPE_KEYPAD)
        {
            kb_indev = indev_iter;
            break;
        }
        indev_iter = lv_indev_get_next(indev_iter);
    }
    if (kb_indev != nullptr)
    {
        lv_group_t *group = lv_group_create();
        lv_group_add_obj(group, ta);
        lv_indev_set_group(kb_indev, group);
    }

    // 创建一个标签并居中显示
    Keyboard_Label = lv_label_create(canvas);
    lv_obj_set_style_text_color(Keyboard_Label, lv_color_black(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(Keyboard_Label, &lv_font_montserrat_48, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_label_set_text(Keyboard_Label, "null");
    lv_obj_align(Keyboard_Label, LV_ALIGN_CENTER, 0, 0);
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
    int16_t assert = Kode_Bq25896::bq25896_init(Bq25896_Iic_Bus, Bq25896_Handle);
    if (assert != ESP_OK)
    {
        printf("bq25896 init fail (error code: %#X)\n", assert);
    }
    else
    {
        printf("bq25896 init success\n");

        Kode_Bq25896::bq25896_set_input_current_limit(Bq25896_Handle, Kode_Bq25896::bq25896_ilim_t ::BQ25896_ILIM_2000MA);
        // 禁用看门狗后不能读取看门狗寄存器状态，否者看门狗禁用会失效
        Kode_Bq25896::bq25896_set_watchdog_timer(Bq25896_Handle, Kode_Bq25896::bq25896_watchdog_t::BQ25896_WATCHDOG_DISABLE);
        // Kode_Bq25896::bq25896_set_adc_conversion(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
        // Kode_Bq25896::bq25896_set_adc_conversion_rate(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);
        Kode_Bq25896::bq25896_set_charge_current(Bq25896_Handle, Kode_Bq25896::bq25896_ichg_t::BQ25896_ICHG_512MA);
        // Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);
    }

    Xl9555_Iic_Bus->set_bus_handle(Bq25896_Iic_Bus->get_bus_handle());
#endif

    Xl9535->begin();

    Xl9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(3, 2500);
    Lilygo_Device_Driver::Init_Ldo_Channel_Power(4, 3300);

    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(5));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(120));

    Screen->begin(SCREEN_MIPI_DSI_DPI_CLK_MHZ, SCREEN_LANE_BIT_RATE_MBPS);

#if defined CONFIG_SCREEN_TYPE_HI8561
    Esp32p4->create_pwm(HI8561_SCREEN_BL, ledc_timer_t::LEDC_TIMER_0, ledc_channel_t::LEDC_CHANNEL_0, 2000);

    Hi8561_Iic_Touch_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    Hi8561_Touch->begin();

#elif defined CONFIG_SCREEN_TYPE_RM69A10

    Gt9895_Touch_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    Gt9895_Touch->begin();

#else
#error "no macro definition is set"
#endif

    Xl9555->begin();
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
                                       Interrupt_Flag = true;
                                   });

    Tca8418->begin();
    Tca8418->set_keypad_scan_window(0, 0, TCA8418_KEYPAD_SCAN_WIDTH, TCA8418_KEYPAD_SCAN_HEIGHT);
    Tca8418->set_irq_pin_mode(Cpp_Bus_Driver::Tca8418::Irq_Mask::KEY_EVENTS);
    Tca8418->clear_irq_flag(Cpp_Bus_Driver::Tca8418::Irq_Flag::KEY_EVENTS);

    Tca8418->create_pwm(KEYBOARD_BL, ledc_timer_t::LEDC_TIMER_1, ledc_channel_t::LEDC_CHANNEL_1, 1000000,
                        0, ledc_mode_t::LEDC_LOW_SPEED_MODE, ledc_timer_bit_t ::LEDC_TIMER_5_BIT);
    Tca8418->start_pwm_gradient_time(30, 1000);

    Lvgl_Init();
    lv_example_canvas_7();
    Lvgl_Keyboard_Init();
    xTaskCreate(lvgl_port_task, "lvgl_port_task", 4 * 1024, NULL, 2, NULL);

    // 等待lvgl刷新完成
    while (lv_display_flush_is_last(lv_display_get_default()) == false)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

#if defined CONFIG_SCREEN_TYPE_HI8561
    Hi8561_Touch->start_pwm_gradient_time(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        Screen->set_brightness(i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "no macro definition is set"
#endif

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
