/*
 * @Description: screen_lvgl
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:31:49
 * @LastEditTime: 2026-03-26 15:01:20
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"
#include "lvgl.h"
#include <cmath>

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

#define LVGL_TICK_PERIOD_MS 1

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
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

#if defined CONFIG_SCREEN_TYPE_HI8561
auto Screen = std::make_unique<Cpp_Bus_Driver::Hi8561>(Screen_Mipi_Bus);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
auto Screen = std::make_unique<Cpp_Bus_Driver::Rm69a10>(Screen_Mipi_Bus);
#else
#error "no macro definition is set"
#endif

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

extern "C" void example_lvgl_demo_ui(lv_display_t *disp);

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    data->state = LV_INDEV_STATE_REL;
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

    printf("display lvgl meter widget\n");
    _lock_acquire(&lvgl_api_lock);
    example_lvgl_demo_ui(display);
    _lock_release(&lvgl_api_lock);
}

void color_grid_buffer(void *buffer, int buffer_width, int buffer_height, int pixel_format, int color_segments)
{
    if (!buffer || buffer_width <= 0 || buffer_height <= 0)
    {
        printf("color_grid_buffer fail\n");
        return;
    }

    // 设置默认值
    if (color_segments <= 0)
    {
        color_segments = 8;
    }

    // 确保颜色分段数不超过屏幕高度
    if (color_segments > buffer_height)
    {
        color_segments = buffer_height;
    }

    // 计算垂直方向每个颜色块的高度
    int block_height = buffer_height / color_segments;
    if (block_height < 1)
        block_height = 1;

    // 计算垂直方向需要多少个块
    int blocks_y = (buffer_height + block_height - 1) / block_height;
    int blocks_x = 1; // 垂直排列，水平方向只有1个块

    // 计算缓冲区跨距（stride）
    int buffer_stride = 0;
    if (pixel_format == 2) // RGB565
    {
        buffer_stride = buffer_width * 2;
    }
    else if (pixel_format == 3) // RGB888
    {
        buffer_stride = buffer_width * 3;
    }
    else
    {
        printf("unsupported pixel format: %d\n", pixel_format);
        return;
    }

    // HSV到RGB的转换函数
    auto hsv_to_rgb = [](float h, float s, float v) -> std::tuple<uint8_t, uint8_t, uint8_t>
    {
        h = fmod(h, 360.0f);
        if (h < 0)
            h += 360.0f;

        float c = v * s;
        float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;

        float r = 0, g = 0, b = 0;

        if (h < 60)
        {
            r = c;
            g = x;
            b = 0;
        }
        else if (h < 120)
        {
            r = x;
            g = c;
            b = 0;
        }
        else if (h < 180)
        {
            r = 0;
            g = c;
            b = x;
        }
        else if (h < 240)
        {
            r = 0;
            g = x;
            b = c;
        }
        else if (h < 300)
        {
            r = x;
            g = 0;
            b = c;
        }
        else
        {
            r = c;
            g = 0;
            b = x;
        }

        return std::make_tuple(
            static_cast<uint8_t>((r + m) * 255),
            static_cast<uint8_t>((g + m) * 255),
            static_cast<uint8_t>((b + m) * 255));
    };

    if (pixel_format == 2)
    {
        // RGB565模式
        uint16_t *p = (uint16_t *)buffer;

        // 使用智能指针分配颜色查找表
        auto color_lut = std::make_unique<uint16_t[]>(blocks_y);

        // 生成颜色查找表 - 垂直方向颜色渐变
        // 色相从0到360度均匀分布
        // 饱和度S=100%，亮度V=100%
        for (int by = 0; by < blocks_y; by++)
        {
            // 计算色相值：从0到360度均匀分布
            // 最顶部的块色相值为0，最底部的块色相值为360
            float hue = 0.0f;
            if (blocks_y > 1)
            {
                hue = (static_cast<float>(by) / (blocks_y - 1)) * 360.0f;
            }

            // 固定饱和度和亮度为100%
            float saturation = 1.0f;
            float value = 1.0f;

            // 转换为RGB
            auto [r, g, b] = hsv_to_rgb(hue, saturation, value);

            // 转换为RGB565格式
            uint16_t rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            color_lut[by] = rgb565;
        }

        // 填充整个缓冲区
        for (int y = 0; y < buffer_height; y++)
        {
            int block_y = y / block_height;
            if (block_y >= blocks_y)
                block_y = blocks_y - 1;

            uint16_t *row_ptr = (uint16_t *)((uint8_t *)buffer + y * buffer_stride);

            // 获取当前行的颜色值
            uint16_t color = color_lut[block_y];

            // 填充一整行相同的颜色
            for (int x = 0; x < buffer_width; x++)
            {
                row_ptr[x] = color;
            }
        }
    }
    else if (pixel_format == 3)
    {
        // RGB888模式
        uint8_t *p = (uint8_t *)buffer;

        // 使用智能指针分配颜色查找表
        auto color_lut_r = std::make_unique<uint8_t[]>(blocks_y);
        auto color_lut_g = std::make_unique<uint8_t[]>(blocks_y);
        auto color_lut_b = std::make_unique<uint8_t[]>(blocks_y);

        // 生成颜色查找表 - 垂直方向颜色渐变
        // 色相从0到360度均匀分布
        // 饱和度S=100%，亮度V=100%
        for (int by = 0; by < blocks_y; by++)
        {
            // 计算色相值：从0到360度均匀分布
            // 最顶部的块色相值为0，最底部的块色相值为360
            float hue = 0.0f;
            if (blocks_y > 1)
            {
                hue = (static_cast<float>(by) / (blocks_y - 1)) * 360.0f;
            }

            // 固定饱和度和亮度为100%
            float saturation = 1.0f;
            float value = 1.0f;

            // 转换为RGB
            auto [r, g, b] = hsv_to_rgb(hue, saturation, value);

            color_lut_r[by] = r;
            color_lut_g[by] = g;
            color_lut_b[by] = b;
        }

        // 填充整个缓冲区
        for (int block_y_idx = 0; block_y_idx < blocks_y; block_y_idx++)
        {
            int start_y = block_y_idx * block_height;
            int end_y = (block_y_idx + 1) * block_height;
            if (end_y > buffer_height)
                end_y = buffer_height;

            // 获取当前块的颜色
            uint8_t r = color_lut_r[block_y_idx];
            uint8_t g = color_lut_g[block_y_idx];
            uint8_t b = color_lut_b[block_y_idx];

            // 填充整个块（多个水平行）
            for (int y = start_y; y < end_y; y++)
            {
                uint8_t *row_ptr = p + y * buffer_stride;

                // 填充一整行相同的颜色
                for (int x = 0; x < buffer_width; x++)
                {
                    *row_ptr++ = r;
                    *row_ptr++ = g;
                    *row_ptr++ = b;
                }
            }
        }
    }
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

    Xl9535_Iic_Bus->set_bus_handle(Bq25896_Iic_Bus->get_bus_handle());
#endif

    Xl9535->begin();

    Xl9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));

#if defined CONFIG_SCREEN_TYPE_HI8561
    Esp32p4->create_pwm(HI8561_SCREEN_BL, ledc_timer_t::LEDC_TIMER_0, ledc_channel_t::LEDC_CHANNEL_0, 2000);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#else
#error "no macro definition is set"
#endif

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(3, 2500);

    vTaskDelay(pdMS_TO_TICKS(100));

    Xl9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(5));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(120));

    Screen->begin(SCREEN_MIPI_DSI_DPI_CLK_MHZ, SCREEN_LANE_BIT_RATE_MBPS);

    size_t screen_size = SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BITS_PER_PIXEL / 8;
    size_t data_cache_line_size = 16;
    void *color_buf = heap_caps_aligned_calloc(data_cache_line_size, 1, screen_size, MALLOC_CAP_SPIRAM);
    if (color_buf != nullptr)
    {
        color_grid_buffer(color_buf, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BITS_PER_PIXEL / 8, SCREEN_HEIGHT);

        if (Screen->send_color_stream_coordinate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color_buf) == false)
        {
            printf("send_color_stream_coordinate fail\n");
        }

        heap_caps_free(color_buf);
    }
    else
    {
        printf("heap_caps_aligned_calloc fail\n");
    }

#if defined CONFIG_SCREEN_TYPE_HI8561
    Esp32p4->start_pwm_gradient_time(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        Screen->set_brightness(i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "no macro definition is set"
#endif

    vTaskDelay(pdMS_TO_TICKS(5000));

    Lvgl_Init();
    xTaskCreate(lvgl_ui_task, "lvgl_ui_task", 100 * 1024, NULL, 1, NULL);

    vTaskDelay(pdMS_TO_TICKS(1000));

#if defined CONFIG_SCREEN_TYPE_HI8561
    Screen->set_mirror(Cpp_Bus_Driver::Hi8561::Mirror_Mode::HORIZONTAL);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Screen->set_mirror(Cpp_Bus_Driver::Hi8561::Mirror_Mode::VERTICAL);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Screen->set_mirror(Cpp_Bus_Driver::Hi8561::Mirror_Mode::HORIZONTAL_VERTICAL);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Screen->set_mirror(Cpp_Bus_Driver::Hi8561::Mirror_Mode::OFF);
    vTaskDelay(pdMS_TO_TICKS(1000));

    Screen->set_color_order(Cpp_Bus_Driver::Hi8561::Color_Order::BGR);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Screen->set_color_order(Cpp_Bus_Driver::Hi8561::Color_Order::RGB);
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif

    Screen->set_inversion(true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Screen->set_inversion(false);
    vTaskDelay(pdMS_TO_TICKS(1000));

    Screen->set_screen_off(true);
    Screen->set_sleep(true);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Screen->set_sleep(false);
    Screen->set_screen_off(false);

    // for (uint8_t i = 0; i < 100; i++)
    // {
    //     Esp32p4->set_pwm_duty(i);
    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }
}
