/*
 * @Description: screen_lvgl_touch_draw
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:35:38
 * @LastEditTime: 2026-04-25 16:53:33
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "lvgl.h"

#define LVGL_TICK_PERIOD_MS 1

size_t g_cycle_time = 0;

std::vector<uint16_t> g_lvgl_draw_x_data;
std::vector<uint16_t> g_lvgl_draw_y_data;

// LVGL library is not thread-safe, this example will call LVGL APIs from
// different tasks, so use a mutex to protect it
static _lock_t g_lvgl_api_lock;

// 定义一个计时器，用于检测 5 秒无操作
time_t g_last_touch_time = 0;

// 定义一个标志，用于判断是否需要清除画布
bool g_need_clear_lock_flag = false;

// 定义一个画布对象
static lv_obj_t* g_canvas;
static lv_layer_t g_layer;

lv_point_t g_point;

#if defined CONFIG_SCREEN_TYPE_HI8561
cpp_bus_driver::Hi8561* g_screen = nullptr;
cpp_bus_driver::Hi8561Touch* g_touch = nullptr;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
cpp_bus_driver::Rm69a10* g_screen = nullptr;
cpp_bus_driver::gt9895* g_touch = nullptr;
#endif

void LvglUiTask(void* arg) {
  printf("LvglUiTask start\n");

  while (1) {
    // _lock_acquire(&g_lvgl_api_lock);
    // time_till_next_ms = lv_timer_handler();
    // _lock_release(&g_lvgl_api_lock);

    // 获取当前时间
    time_t current_time = time(NULL);

    // 如果距离上次触摸时间超过 5 秒，则清除画布
    if ((current_time - g_last_touch_time > 5) &&
        (g_need_clear_lock_flag == true)) {
      lv_canvas_fill_bg(g_canvas, lv_color_hex3(0xccc), LV_OPA_COVER);
      g_last_touch_time = current_time;

      g_lvgl_draw_x_data.clear();
      g_lvgl_draw_y_data.clear();

      g_need_clear_lock_flag = false;
    }

    lv_timer_handler();

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void MyTouchpadRead(lv_indev_t* indev, lv_indev_data_t* data) {
#if defined CONFIG_SCREEN_TYPE_HI8561
  cpp_bus_driver::Hi8561Touch::TouchPoint tp;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  cpp_bus_driver::Gt9895::TouchPoint tp;
#endif

  if (g_touch->GetSingleTouchPoint(tp) == true) {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = tp.info[0].x;
    data->point.y = tp.info[0].y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// 绘图回调函数
void DrawPoint(lv_event_t* e) {
  // lv_obj_t *obj = lv_event_get_target(e);
  lv_event_code_t code = lv_event_get_code(e);

  // printf("code: %d\n", code);

  switch (code) {
    case LV_EVENT_PRESSING: {
      lv_indev_t* indev = lv_indev_get_act();
      lv_indev_get_point(indev, &g_point);

      // printf("touch x: %ld y: %ld\n", g_point.x, g_point.y);

      // 在画布上绘制点
      // lv_canvas_set_px(g_canvas, g_point.x, g_point.y,
      // lv_palette_main(LV_PALETTE_RED), LV_OPA_COVER);

      g_lvgl_draw_x_data.push_back(g_point.x);
      g_lvgl_draw_y_data.push_back(g_point.y);

      if ((g_lvgl_draw_x_data.size() >= 2) &&
          (g_lvgl_draw_y_data.size() >= 2)) {
        lv_draw_line_dsc_t dsc;
        lv_draw_line_dsc_init(&dsc);
        dsc.color = lv_palette_main(LV_PALETTE_RED);
        dsc.width = 4;
        dsc.round_end = 1;
        dsc.round_start = 1;
        dsc.p1.x = g_lvgl_draw_x_data[0];
        dsc.p1.y = g_lvgl_draw_y_data[0];
        dsc.p2.x = g_lvgl_draw_x_data[1];
        dsc.p2.y = g_lvgl_draw_y_data[1];
        lv_draw_line(&g_layer, &dsc);

        lv_canvas_finish_layer(g_canvas, &g_layer);

        g_lvgl_draw_x_data.erase(g_lvgl_draw_x_data.begin());
        g_lvgl_draw_y_data.erase(g_lvgl_draw_y_data.begin());
      }

      // 获取当前时间
      time_t current_time = time(NULL);
      // 更新上次触摸时间
      g_last_touch_time = current_time;
      g_need_clear_lock_flag = true;
    } break;
    case LV_EVENT_RELEASED:
      //  printf("777\n");
      g_lvgl_draw_x_data.clear();
      g_lvgl_draw_y_data.clear();
      break;

    default:
      break;
  }
}

void lv_example_canvas_7(void) {
  void* draw_buf = NULL;
  size_t draw_buffer_sz = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
  draw_buf = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);

  /*Create a g_canvas and initialize its palette*/
  g_canvas = lv_canvas_create(lv_screen_active());
  // lv_canvas_set_draw_buf(g_canvas, (lv_draw_buf_t *)draw_buf);
  lv_canvas_set_buffer(g_canvas, draw_buf, SCREEN_WIDTH, SCREEN_HEIGHT,
                       [](uint8_t format) -> lv_color_format_t {
                         switch (format) {
                           case 16:
                             return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
                           case 24:
                             return lv_color_format_t::LV_COLOR_FORMAT_RGB888;
                           default:
                             return lv_color_format_t::LV_COLOR_FORMAT_RGB565;
                         }
                       }(SCREEN_BITS_PER_PIXEL));
  lv_canvas_fill_bg(g_canvas, lv_color_hex3(0xCCC), LV_OPA_COVER);
  lv_obj_center(g_canvas);

  lv_canvas_init_layer(g_canvas, &g_layer);

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
  lv_draw_line(&g_layer, &dsc);

  lv_canvas_finish_layer(g_canvas, &g_layer);

  // 注册触摸事件回调
  lv_obj_add_event_cb(lv_screen_active(), DrawPoint, LV_EVENT_ALL, NULL);
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
}

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

#if defined CONFIG_SCREEN_TYPE_HI8561
  g_screen = driver.chip().hi8561.get();
  g_touch = driver.chip().hi8561_touch.get();
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  g_screen = driver.chip().rm69a10.get();
  g_touch = driver.chip().gt9895.get();
#endif

  InitLvgl();
  xTaskCreate(LvglUiTask, "LvglUiTask", 4 * 1024, NULL, 2, NULL);

  lv_example_canvas_7();

#if defined CONFIG_SCREEN_TYPE_HI8561
  driver.chip().hi8561_backlight->StartGradientTime(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  for (uint8_t i = 0; i < 255; i += 5) {
    screen->SetBrightness(i);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
#endif

  while (1) {
    if (esp_log_timestamp() > g_cycle_time) {
#if defined CONFIG_SCREEN_TYPE_HI8561
      cpp_bus_driver::Hi8561Touch::TouchPoint tp;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
      cpp_bus_driver::Gt9895::TouchPoint tp;
#endif

      if (g_touch->GetMultipleTouchPoint(tp) == true) {
        printf("Touch finger: %d edge touch flag: %d\n", tp.finger_count,
               tp.edge_touch_flag);

#if defined CONFIG_SCREEN_TYPE_HI8561
        for (uint8_t i = 0; i < tp.info.size(); i++) {
          printf("Touch num:[%d] x: %d y: %d p: %d\n", i + 1, tp.info[i].x,
                 tp.info[i].y, tp.info[i].pressure_value);
        }

#elif defined CONFIG_SCREEN_TYPE_RM69A10
        for (uint8_t i = 0; i < tp.info.size(); i++) {
          printf("Touch num:[%d] id:[%d] x: %d y: %d p: %d\n", i + 1,
                 tp.info[i].finger_id, tp.info[i].x, tp.info[i].y,
                 tp.info[i].pressure_value);
        }
#endif

        g_cycle_time = esp_log_timestamp() + 1000;
      }

      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}
