/*
 * @Description: screen_tca8418_lvgl_touch_draw
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:35:38
 * @LastEditTime: 2026-04-27 15:20:11
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "lvgl.h"
#include "lvgl_keyboard_config.h"

#define LVGL_TICK_PERIOD_MS 1

std::vector<uint16_t> g_lvgl_draw_x_data;
std::vector<uint16_t> g_lvgl_draw_y_data;

time_t g_last_touch_time = 0;
bool g_need_clear_lock_flag = false;

static lv_obj_t* g_canvas;
static lv_layer_t g_layer;
static lv_obj_t* g_keyboard_label;

lv_point_t g_point;

cpp_bus_driver::Xl95x5* g_xl9555 = nullptr;
cpp_bus_driver::Tca8418* g_tca8418 = nullptr;

#if defined CONFIG_SCREEN_TYPE_HI8561
cpp_bus_driver::Hi8561* g_screen = nullptr;
cpp_bus_driver::Hi8561Touch* g_touch = nullptr;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
cpp_bus_driver::Rm69a10* g_screen = nullptr;
cpp_bus_driver::Gt9895* g_touch = nullptr;
#endif

volatile bool g_interrupt_flag = false;

void LvglUiTask(void* arg) {
  printf("LvglUiTask start\n");

  while (1) {
    time_t current_time = time(NULL);

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

  if ((g_touch->GetSingleTouchPoint(tp) == true) && !tp.info.empty()) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = tp.info[0].x;
    data->point.y = tp.info[0].y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void MyKeyboardRead(lv_indev_t* indev, lv_indev_data_t* data) {
  static uint32_t last_key = 0;
  static bool pressed_state_flag = false;
  static bool caps_lock_flag = false;

  if (g_interrupt_flag == true) {
    cpp_bus_driver::Tca8418::IrqStatus irq_status;

    if (g_tca8418->ParseIrqStatus(g_tca8418->GetIrqFlag(), irq_status) ==
        false) {
      printf("ParseIrqStatus failed\n");
    } else if (irq_status.key_events_flag == true) {
      cpp_bus_driver::Tca8418::TouchPoint tp;

      if (g_tca8418->GetMultipleTouchPoint(tp) == true) {
        printf("Touch finger: %d\n", tp.finger_count);

        for (uint8_t i = 0; i < tp.info.size(); i++) {
          switch (tp.info[i].event_type) {
            case cpp_bus_driver::Tca8418::EventType::kKeypad: {
              cpp_bus_driver::Tca8418::TouchPosition touch_position;

              if (g_tca8418->ParseTouchNum(tp.info[i].num, touch_position) ==
                  true) {
                printf("Keypad event\n");
                printf("   Touch num:[%d] num: %d x: %d y: %d press flag: %d\n",
                       i + 1, tp.info[i].num, touch_position.x,
                       touch_position.y, tp.info[i].press_flag);

                const size_t key_count =
                    sizeof(Tca8418_Map) / sizeof(std::string);
                const size_t lvgl_key_count =
                    sizeof(Tca8418_Map_Lvgl) / sizeof(uint32_t);
                const bool key_valid = (tp.info[i].num > 0) &&
                                       (tp.info[i].num <= key_count) &&
                                       (tp.info[i].num <= lvgl_key_count);
                const size_t key_index = tp.info[i].num - 1;

                if (key_valid) {
                  printf("   Touch string: %s\n",
                         Tca8418_Map[key_index].c_str());
                  lv_label_set_text(g_keyboard_label,
                                    Tca8418_Map[key_index].c_str());
                }

                if (tp.info[i].press_flag == 1) {
                  pressed_state_flag = true;

                  if (key_valid && (Tca8418_Map[key_index] == "Caps")) {
                    caps_lock_flag = !caps_lock_flag;

                    auto led_value = caps_lock_flag
                                         ? cpp_bus_driver::Xl95x5::Value::kLow
                                         : cpp_bus_driver::Xl95x5::Value::kHigh;
                    g_xl9555->PinWrite(XL9555_LED_1, led_value);
                    g_xl9555->PinWrite(XL9555_LED_2, led_value);
                    g_xl9555->PinWrite(XL9555_LED_3, led_value);
                  }

                  if (key_valid) {
                    last_key = Tca8418_Map_Lvgl[key_index];
                    if ((caps_lock_flag == true) && (last_key >= 'a') &&
                        (last_key <= 'z')) {
                      last_key = last_key - 'a' + 'A';
                    }
                  }
                } else {
                  pressed_state_flag = false;
                }
              }
              break;
            }

            case cpp_bus_driver::Tca8418::EventType::kGpio:
              printf("Gpio event\n");
              printf("   Touch num:[%d] num: %d press flag: %d\n", i + 1,
                     tp.info[i].num, tp.info[i].press_flag);
              break;

            default:
              break;
          }
        }
      }

      g_tca8418->ClearIrqFlag(cpp_bus_driver::Tca8418::IrqFlag::kKeyEvents);
    }

    g_interrupt_flag = false;
  }

  if (pressed_state_flag == false) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = last_key;
  }
}

void DrawPoint(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  switch (code) {
    case LV_EVENT_PRESSING: {
      lv_indev_t* indev = lv_indev_get_act();
      lv_indev_get_point(indev, &g_point);

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

      g_last_touch_time = time(NULL);
      g_need_clear_lock_flag = true;
    } break;

    case LV_EVENT_RELEASED:
      g_lvgl_draw_x_data.clear();
      g_lvgl_draw_y_data.clear();
      break;

    default:
      break;
  }
}

void InitLvglCanvas(void) {
  void* draw_buf = NULL;
  size_t draw_buffer_sz = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
  draw_buf = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);

  g_canvas = lv_canvas_create(lv_screen_active());
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

  lv_obj_add_event_cb(lv_screen_active(), DrawPoint, LV_EVENT_ALL, NULL);
}

void InitLvgl(void) {
  printf("InitLvgl\n");

  lv_init();

  lv_display_t* display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_user_data(display, g_screen);
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

  printf("Allocate separate lvgl draw buffers\n");
  size_t draw_buffer_sz = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
  void* buf1 = heap_caps_malloc(
      draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
  assert(buf1);

  lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_flush_cb(display, [](lv_display_t* disp, const lv_area_t* area,
                                      uint8_t* px_map) {
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    lv_area_t rotated_area;
    std::unique_ptr<uint8_t[]> rotated_buf;

    if (rotation != LV_DISPLAY_ROTATION_0) {
      lv_color_format_t color_format = lv_display_get_color_format(disp);
      rotated_area = *area;
      lv_display_rotate_area(disp, &rotated_area);

      uint32_t src_stride =
          lv_draw_buf_width_to_stride(lv_area_get_width(area), color_format);
      uint32_t dest_stride = lv_draw_buf_width_to_stride(
          lv_area_get_width(&rotated_area), color_format);
      int32_t src_w = lv_area_get_width(area);
      int32_t src_h = lv_area_get_height(area);
      rotated_buf = std::make_unique<uint8_t[]>(SCREEN_WIDTH * SCREEN_HEIGHT *
                                                (SCREEN_BITS_PER_PIXEL / 8));
      lv_draw_sw_rotate(px_map, rotated_buf.get(), src_w, src_h, src_stride,
                        dest_stride, rotation, color_format);

      area = &rotated_area;
      px_map = rotated_buf.get();
    }

#if defined CONFIG_SCREEN_TYPE_HI8561
    auto screen = (cpp_bus_driver::Hi8561*)lv_display_get_user_data(disp);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
        auto screen = (cpp_bus_driver::Rm69a10*)lv_display_get_user_data(disp);
#endif

    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    screen->SendColorStreamCoordinate(offsetx1, offsety1, offsetx2 + 1,
                                      offsety2 + 1, px_map);
  });

  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, MyTouchpadRead);

  lv_indev_t* keyboard_indev = lv_indev_create();
  lv_indev_set_type(keyboard_indev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(keyboard_indev, MyKeyboardRead);

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
                            void* user_ctx) -> bool { return false; },
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

  lv_display_set_rotation(
      display, [](uint8_t rotation) -> lv_display_rotation_t {
        switch (rotation) {
          case 0:
            return lv_display_rotation_t::LV_DISPLAY_ROTATION_0;
          case 90:
            return lv_display_rotation_t::LV_DISPLAY_ROTATION_90;
          default:
            return lv_display_rotation_t::LV_DISPLAY_ROTATION_0;
        }
      }(SCREEN_ROTATION_DIRECTION));
}

void InitLvglKeyboard(void) {
  lv_obj_t* text_area = lv_textarea_create(g_canvas);
  lv_obj_set_width(text_area, 300);
  lv_obj_set_height(text_area, 150);
  lv_obj_align(text_area, LV_ALIGN_BOTTOM_MID, -370, -50);
  lv_obj_set_style_bg_opa(text_area, LV_OPA_50, 0);
  lv_obj_set_style_text_font(text_area, &lv_font_montserrat_24, 0);
  lv_obj_remove_flag(text_area, LV_OBJ_FLAG_CLICKABLE);

  lv_indev_t* keyboard_indev = nullptr;
  lv_indev_t* indev_iter = lv_indev_get_next(NULL);
  while (indev_iter) {
    if (lv_indev_get_type(indev_iter) == LV_INDEV_TYPE_KEYPAD) {
      keyboard_indev = indev_iter;
      break;
    }
    indev_iter = lv_indev_get_next(indev_iter);
  }

  if (keyboard_indev != nullptr) {
    lv_group_t* group = lv_group_create();
    lv_group_add_obj(group, text_area);
    lv_indev_set_group(keyboard_indev, group);
  }

  g_keyboard_label = lv_label_create(g_canvas);
  lv_obj_set_style_text_color(g_keyboard_label, lv_color_black(),
                              (lv_style_selector_t)LV_PART_MAIN |
                                  (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(g_keyboard_label, &lv_font_montserrat_48,
                             (lv_style_selector_t)LV_PART_MAIN |
                                 (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_label_set_text(g_keyboard_label, "null");
  lv_obj_align(g_keyboard_label, LV_ALIGN_CENTER, 0, 0);
}

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  g_xl9555 = driver.chip().xl9555.get();
  g_tca8418 = driver.chip().tca8418.get();

#if defined CONFIG_SCREEN_TYPE_HI8561
  g_screen = driver.chip().hi8561.get();
  g_touch = driver.chip().hi8561_touch.get();
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  g_screen = driver.chip().rm69a10.get();
  g_touch = driver.chip().gt9895.get();
#endif

  auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();
  esp32p4->InitGpioInterrupt(
      TCA8418_INT, cpp_bus_driver::Tool::InterruptMode::kFalling,
      [](void* arg) -> void { g_interrupt_flag = true; });

  driver.chip().tca8418_backlight->StartGradientTime(30, 1000);

  InitLvgl();
  InitLvglCanvas();
  InitLvglKeyboard();
  xTaskCreate(LvglUiTask, "LvglUiTask", 4 * 1024, NULL, 2, NULL);

  while (lv_display_flush_is_last(lv_display_get_default()) == false) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

#if defined CONFIG_SCREEN_TYPE_HI8561
  driver.chip().hi8561_backlight->StartGradientTime(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  for (uint8_t i = 0; i < 255; i += 5) {
    g_screen->SetBrightness(i);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
#endif
}
