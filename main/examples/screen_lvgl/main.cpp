/*
 * @Description: screen_lvgl
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:31:49
 * @LastEditTime: 2026-01-24 17:46:18
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
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "t_display_p4_driver.h"
#if CONFIG_ENABLE_USB_DISPLAY == true
#include "esp_lcd_usb_display.h"
#endif
#include <cmath>

#define LVGL_TICK_PERIOD_MS 1

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Mipi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Mipi>(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_MIPI_DSI_HSYNC, SCREEN_MIPI_DSI_HBP, SCREEN_MIPI_DSI_HFP,
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

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS);

#if CONFIG_ENABLE_USB_DISPLAY == true
#else

#if defined CONFIG_SCREEN_TYPE_HI8561
auto Screen = std::make_unique<Cpp_Bus_Driver::Hi8561>(Mipi_Bus);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
auto Screen = std::make_unique<Cpp_Bus_Driver::Rm69a10>(Mipi_Bus);
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

#endif

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

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
#if defined CONFIG_SCREEN_TYPE_HI8561
                                auto Screen = (Cpp_Bus_Driver::Hi8561 *)lv_display_get_user_data(disp);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
                                auto Screen = (Cpp_Bus_Driver::Rm69a10 *)lv_display_get_user_data(disp);
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

                                esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
                                int offsetx1 = area->x1;
                                int offsetx2 = area->x2;
                                int offsety1 = area->y1;
                                int offsety2 = area->y2;
                                // pass the draw buffer to the driver
                                Screen->send_color_stream_coordinate(offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

#if CONFIG_ENABLE_USB_DISPLAY == true
                                lv_display_flush_ready(disp);
#endif
                            });

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

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
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(Mipi_Bus->get_device_handle(), &cbs, display));
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

    printf("display lvgl meter widget\n");
    _lock_acquire(&lvgl_api_lock);
    example_lvgl_demo_ui(display);
    _lock_release(&lvgl_api_lock);
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
#endif

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
    XL9535->begin();

    XL9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    vTaskDelay(pdMS_TO_TICKS(200));

#if defined CONFIG_SCREEN_TYPE_HI8561
    ESP32P4->create_pwm(HI8561_SCREEN_BL, ledc_channel_t::LEDC_CHANNEL_0, 2000);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#else
#error "unknown macro definition, please select the correct macro definition."
#endif

    Init_Ldo_Channel_Power(3, 1830);

    vTaskDelay(pdMS_TO_TICKS(100));

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(5));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(120));

#if CONFIG_ENABLE_USB_DISPLAY == true
    Usb_Screen_Init(&Screen_Mipi_Dpi_Panel);
#else
    Screen->begin(SCREEN_MIPI_DSI_DPI_CLK_MHZ, SCREEN_LANE_BIT_RATE_MBPS);
#endif

    size_t screen_size = SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BITS_PER_PIXEL / 8;
    size_t data_cache_line_size = 16;
    void *color_buf = heap_caps_aligned_calloc(data_cache_line_size, 1, screen_size, MALLOC_CAP_SPIRAM);
    if (color_buf != nullptr)
    {
        color_grid_buffer(color_buf, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BITS_PER_PIXEL / 8, SCREEN_HEIGHT);

        esp_err_t err = esp_lcd_panel_draw_bitmap(Mipi_Bus->get_device_handle(), 0, 0,
                                                  SCREEN_WIDTH, SCREEN_HEIGHT, color_buf);
        if (err != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", err);
        }

        heap_caps_free(color_buf);
    }
    else
    {
        printf("heap_caps_aligned_calloc fail\n");
    }

#if CONFIG_ENABLE_USB_DISPLAY == true
#else
#if defined CONFIG_SCREEN_TYPE_HI8561
    ESP32P4->start_pwm_gradient_time(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        Screen->set_brightness(i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "unknown macro definition, please select the correct macro definition."
#endif
#endif

    vTaskDelay(pdMS_TO_TICKS(5000));

    Lvgl_Init();
    xTaskCreate(lvgl_ui_task, "lvgl_ui_task", 100 * 1024, NULL, 1, NULL);

#if CONFIG_ENABLE_USB_DISPLAY == true
#else
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

#endif

    // for (uint8_t i = 0; i < 100; i++)
    // {
    //     ESP32P4->set_pwm_duty(i);
    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }
}
