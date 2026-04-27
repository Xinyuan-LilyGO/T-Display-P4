/*
 * @Description: screen_lvgl
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:31:49
 * @LastEditTime: 2026-04-25 16:51:53
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "lvgl.h"
// #include <cmath>

#define LVGL_TICK_PERIOD_MS 1

// LVGL library is not thread-safe, this example will call LVGL APIs from
// different tasks, so use a mutex to protect it
static _lock_t g_lvgl_api_lock;

#if defined CONFIG_SCREEN_TYPE_HI8561
cpp_bus_driver::Hi8561* g_screen = nullptr;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
cpp_bus_driver::Rm69a10* g_screen = nullptr;
#endif

extern "C" void example_lvgl_demo_ui(lv_display_t* disp);

void MyTouchpadRead(lv_indev_t* indev, lv_indev_data_t* data) {
  data->state = LV_INDEV_STATE_REL;
}

void LvglUiTask(void* arg) {
  printf("LvglUiTask start\n");
  uint32_t time_till_next_ms = 0;

  while (1) {
    _lock_acquire(&g_lvgl_api_lock);
    time_till_next_ms = lv_timer_handler();
    _lock_release(&g_lvgl_api_lock);

    // in case of task watch dog timeout, set the minimal delay to 10ms
    if (time_till_next_ms < 10) {
      time_till_next_ms = 10;
    }
    usleep(1000 * time_till_next_ms);

    // lv_timer_handler();
    // vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void InitLvgl(void) {
  printf("InitLvgl\n");

  lv_init();

  // create a lvgl display
  lv_display_t* display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  // associate the mipi panel handle to the display
  lv_display_set_user_data(display, g_screen);
  // set color depth
  lv_display_set_color_format(display, [](uint8_t format) -> lv_color_format_t {
    switch (format) {
      case 16:
        return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
      case 24:
        return lv_color_format_t::LV_COLOR_FORMAT_RGB888;
      default:
        return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
    }
  }(SCREEN_BITS_PER_PIXEL));
  // create draw buffer
  printf("Allocate separate lvgl draw buffers\n");
  size_t draw_buffer_sz = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
  void* buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
  assert(buf1);
  // void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
  // assert(buf2);
  // initialize LVGL draw buffers
  lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  // set the callback which can copy the rendered image to an area of the
  // display
  lv_display_set_flush_cb(
      display, [](lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {

#if defined CONFIG_SCREEN_TYPE_HI8561
        auto screen = (cpp_bus_driver::Hi8561*)lv_display_get_user_data(disp);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
                                auto screen = (cpp_bus_driver::Rm69a10 *)lv_display_get_user_data(disp);
#endif

        int offsetx1 = area->x1;
        int offsetx2 = area->x2;
        int offsety1 = area->y1;
        int offsety2 = area->y2;
        // pass the draw buffer to the driver
        screen->SendColorStreamCoordinate(offsetx1, offsety1, offsetx2 + 1,
                                          offsety2 + 1, px_map);
      });

  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(
      indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
  lv_indev_set_read_cb(indev, MyTouchpadRead);

  printf(
      "Register dpi panel event callback for lvgl flush ready notification\n");
  esp_lcd_dpi_panel_event_callbacks_t cbs = {
      .on_color_trans_done = [](esp_lcd_panel_handle_t panel,
                                esp_lcd_dpi_panel_event_data_t* edata,
                                void* user_ctx) -> bool {
        lv_display_t* disp = (lv_display_t*)user_ctx;
        lv_display_flush_ready(disp);
        return false;
      },
      .on_refresh_done = [](esp_lcd_panel_handle_t panel,
                            esp_lcd_dpi_panel_event_data_t* edata,
                            void* user_ctx) -> bool {
        // static int io_level = 0;
        // // please note, the real refresh rate should be 2*frequency of this
        // GPIO toggling gpio_set_level(EXAMPLE_PIN_NUM_REFRESH_MONITOR,
        // io_level); io_level = !io_level;
        return false;
      },
  };

  ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(
      lilygo_device_driver::TDisplayP4Driver::GetInstance()
          .bus()
          .screen_mipi_bus->device_handle(),
      &cbs, display));

  printf("Use esp_timer as lvgl tick timer\n");
  const esp_timer_create_args_t lvgl_tick_timer_args = {
      .callback = [](void* arg) { lv_tick_inc(LVGL_TICK_PERIOD_MS); },
      .name = "lvgl_tick"};
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(
      esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

  _lock_acquire(&g_lvgl_api_lock);
  example_lvgl_demo_ui(display);
  _lock_release(&g_lvgl_api_lock);
}

void ColorGridBuffer(void* buffer, int buffer_width, int buffer_height,
                     int pixel_format, int color_segments) {
  if (!buffer || buffer_width <= 0 || buffer_height <= 0) {
    printf("ColorGridBuffer failed\n");
    return;
  }

  // 设置默认值
  if (color_segments <= 0) {
    color_segments = 8;
  }

  // 确保颜色分段数不超过屏幕高度
  if (color_segments > buffer_height) {
    color_segments = buffer_height;
  }

  // 计算垂直方向每个颜色块的高度
  int block_height = buffer_height / color_segments;
  if (block_height < 1) block_height = 1;

  // 计算垂直方向需要多少个块
  int blocks_y = (buffer_height + block_height - 1) / block_height;
  int blocks_x = 1;  // 垂直排列，水平方向只有1个块

  // 计算缓冲区跨距（stride）
  int buffer_stride = 0;
  if (pixel_format == 2)  // RGB565
  {
    buffer_stride = buffer_width * 2;
  } else if (pixel_format == 3)  // RGB888
  {
    buffer_stride = buffer_width * 3;
  } else {
    printf("Unsupported pixel format: %d\n", pixel_format);
    return;
  }

  // HSV到RGB的转换函数
  auto hsv_to_rgb = [](float h, float s,
                       float v) -> std::tuple<uint8_t, uint8_t, uint8_t> {
    h = fmod(h, 360.0f);
    if (h < 0) h += 360.0f;

    float c = v * s;
    float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r = 0, g = 0, b = 0;

    if (h < 60) {
      r = c;
      g = x;
      b = 0;
    } else if (h < 120) {
      r = x;
      g = c;
      b = 0;
    } else if (h < 180) {
      r = 0;
      g = c;
      b = x;
    } else if (h < 240) {
      r = 0;
      g = x;
      b = c;
    } else if (h < 300) {
      r = x;
      g = 0;
      b = c;
    } else {
      r = c;
      g = 0;
      b = x;
    }

    return std::make_tuple(static_cast<uint8_t>((r + m) * 255),
                           static_cast<uint8_t>((g + m) * 255),
                           static_cast<uint8_t>((b + m) * 255));
  };

  if (pixel_format == 2) {
    // RGB565模式
    uint16_t* p = (uint16_t*)buffer;

    // 使用智能指针分配颜色查找表
    auto color_lut = std::make_unique<uint16_t[]>(blocks_y);

    // 生成颜色查找表 - 垂直方向颜色渐变
    // 色相从0到360度均匀分布
    // 饱和度S=100%，亮度V=100%
    for (int by = 0; by < blocks_y; by++) {
      // 计算色相值：从0到360度均匀分布
      // 最顶部的块色相值为0，最底部的块色相值为360
      float hue = 0.0f;
      if (blocks_y > 1) {
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
    for (int y = 0; y < buffer_height; y++) {
      int block_y = y / block_height;
      if (block_y >= blocks_y) block_y = blocks_y - 1;

      uint16_t* row_ptr = (uint16_t*)((uint8_t*)buffer + y * buffer_stride);

      // 获取当前行的颜色值
      uint16_t color = color_lut[block_y];

      // 填充一整行相同的颜色
      for (int x = 0; x < buffer_width; x++) {
        row_ptr[x] = color;
      }
    }
  } else if (pixel_format == 3) {
    // RGB888模式
    uint8_t* p = (uint8_t*)buffer;

    // 使用智能指针分配颜色查找表
    auto color_lut_r = std::make_unique<uint8_t[]>(blocks_y);
    auto color_lut_g = std::make_unique<uint8_t[]>(blocks_y);
    auto color_lut_b = std::make_unique<uint8_t[]>(blocks_y);

    // 生成颜色查找表 - 垂直方向颜色渐变
    // 色相从0到360度均匀分布
    // 饱和度S=100%，亮度V=100%
    for (int by = 0; by < blocks_y; by++) {
      // 计算色相值：从0到360度均匀分布
      // 最顶部的块色相值为0，最底部的块色相值为360
      float hue = 0.0f;
      if (blocks_y > 1) {
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
    for (int block_y_idx = 0; block_y_idx < blocks_y; block_y_idx++) {
      int start_y = block_y_idx * block_height;
      int end_y = (block_y_idx + 1) * block_height;
      if (end_y > buffer_height) end_y = buffer_height;

      // 获取当前块的颜色
      uint8_t r = color_lut_r[block_y_idx];
      uint8_t g = color_lut_g[block_y_idx];
      uint8_t b = color_lut_b[block_y_idx];

      // 填充整个块（多个水平行）
      for (int y = start_y; y < end_y; y++) {
        uint8_t* row_ptr = p + y * buffer_stride;

        // 填充一整行相同的颜色
        for (int x = 0; x < buffer_width; x++) {
          *row_ptr++ = r;
          *row_ptr++ = g;
          *row_ptr++ = b;
        }
      }
    }
  }
}

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

#if defined CONFIG_SCREEN_TYPE_HI8561
  g_screen = driver.chip().hi8561.get();
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  g_screen = driver.chip().rm69a10.get();
#endif

  size_t screen_size = SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BITS_PER_PIXEL / 8;
  void* color_buf =
      heap_caps_aligned_calloc(16, 1, screen_size, MALLOC_CAP_SPIRAM);
  if (color_buf != nullptr) {
    ColorGridBuffer(color_buf, SCREEN_WIDTH, SCREEN_HEIGHT,
                    SCREEN_BITS_PER_PIXEL / 8, SCREEN_HEIGHT);

    if (!g_screen->SendColorStreamCoordinate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                             color_buf)) {
      printf("SendColorStreamCoordinate failed\n");
    }

    heap_caps_free(color_buf);
  } else {
    printf("heap_caps_aligned_calloc failed\n");
  }

#if defined CONFIG_SCREEN_TYPE_HI8561
  driver.chip().hi8561_backlight->StartGradientTime(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  for (uint8_t i = 0; i < 255; i += 5) {
    screen->SetBrightness(i);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
#endif

  vTaskDelay(pdMS_TO_TICKS(5000));

  InitLvgl();
  xTaskCreate(LvglUiTask, "LvglUiTask", 100 * 1024, NULL, 1, NULL);

  vTaskDelay(pdMS_TO_TICKS(1000));

#if defined CONFIG_SCREEN_TYPE_HI8561
  g_screen->SetMirror(cpp_bus_driver::Hi8561::MirrorMode::kHorizontal);
  vTaskDelay(pdMS_TO_TICKS(1000));
  g_screen->SetMirror(cpp_bus_driver::Hi8561::MirrorMode::kVertical);
  vTaskDelay(pdMS_TO_TICKS(1000));
  g_screen->SetMirror(cpp_bus_driver::Hi8561::MirrorMode::kHorizontalVertical);
  vTaskDelay(pdMS_TO_TICKS(1000));
  g_screen->SetMirror(cpp_bus_driver::Hi8561::MirrorMode::kOff);
  vTaskDelay(pdMS_TO_TICKS(1000));

  g_screen->SetColorOrder(cpp_bus_driver::Hi8561::ColorOrder::kBgr);
  vTaskDelay(pdMS_TO_TICKS(1000));
  g_screen->SetColorOrder(cpp_bus_driver::Hi8561::ColorOrder::kRgb);
  vTaskDelay(pdMS_TO_TICKS(1000));
#endif

  g_screen->SetInversion(true);
  vTaskDelay(pdMS_TO_TICKS(1000));
  g_screen->SetInversion(false);
  vTaskDelay(pdMS_TO_TICKS(1000));

  g_screen->SetScreenOff(true);
  g_screen->SetSleep(true);
  vTaskDelay(pdMS_TO_TICKS(1000));
  g_screen->SetSleep(false);
  g_screen->SetScreenOff(false);
}
