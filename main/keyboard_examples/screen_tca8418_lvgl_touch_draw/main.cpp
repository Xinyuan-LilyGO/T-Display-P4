/*
 * @Description: screen_tca8418_lvgl_touch_draw
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:35:38
 * @LastEditTime: 2026-04-27 15:20:11
 * @License: GPL 3.0
 */
#include "lilygo_device_driver.h"
#include "lvgl.h"
#include "lvgl_keyboard_config.h"

#include <algorithm>
#include <cassert>
#include <ctime>
#include <iterator>
#include <vector>

#include "esp_heap_caps.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LVGL_TICK_PERIOD_MS 1

namespace board = lilygo_device_driver::t_display_p4;
namespace keyboard = board::keyboard_expansion::device::tca8418;
using DeviceDriver = lilygo_device_driver::TDisplayP4Driver;
static_assert(keyboard::kMap.size() == std::size(Tca8418_Map_Lvgl));

std::vector<uint16_t> g_lvgl_draw_x_data;
std::vector<uint16_t> g_lvgl_draw_y_data;

time_t g_last_touch_time = 0;
bool g_need_clear_lock_flag = false;

static lv_obj_t* g_canvas;
static lv_layer_t g_layer;
static lv_obj_t* g_keyboard_label;
static uint8_t* g_rotation_buffer = nullptr;

lv_point_t g_point;

cpp_bus_driver::Tca8418* g_tca8418 = nullptr;

lv_color_format_t ScreenColorFormat() {
  return DeviceDriver::GetInstance().screen_info().bits_per_pixel == 24
             ? LV_COLOR_FORMAT_RGB888
             : LV_COLOR_FORMAT_RGB565;
}

void ShowKeyLabel(const keyboard::KeyMapping& mapping) {
  if (mapping.key == keyboard::KeyCode::kCharacter) {
    const char label[] = {mapping.character, '\0'};
    lv_label_set_text(g_keyboard_label, label);
    return;
  }
  if (mapping.key >= keyboard::KeyCode::kF1 &&
      mapping.key <= keyboard::KeyCode::kF11) {
    lv_label_set_text_fmt(g_keyboard_label, "F%u",
        static_cast<unsigned int>(mapping.key) -
            static_cast<unsigned int>(keyboard::KeyCode::kF1) + 1);
    return;
  }
  switch (mapping.key) {
    case keyboard::KeyCode::kEscape:
      lv_label_set_text(g_keyboard_label, "Esc");
      break;
    case keyboard::KeyCode::kBackspace:
      lv_label_set_text(g_keyboard_label, "Backspace");
      break;
    case keyboard::KeyCode::kEnter:
      lv_label_set_text(g_keyboard_label, "Enter");
      break;
    case keyboard::KeyCode::kTab:
      lv_label_set_text(g_keyboard_label, "Tab");
      break;
    case keyboard::KeyCode::kUp:
      lv_label_set_text(g_keyboard_label, "Up");
      break;
    case keyboard::KeyCode::kDown:
      lv_label_set_text(g_keyboard_label, "Down");
      break;
    case keyboard::KeyCode::kLeft:
      lv_label_set_text(g_keyboard_label, "Left");
      break;
    case keyboard::KeyCode::kRight:
      lv_label_set_text(g_keyboard_label, "Right");
      break;
    case keyboard::KeyCode::kCapsLock:
      lv_label_set_text(g_keyboard_label, "Caps");
      break;
    case keyboard::KeyCode::kShift:
      lv_label_set_text(g_keyboard_label, "Shift");
      break;
    case keyboard::KeyCode::kControl:
      lv_label_set_text(g_keyboard_label, "Ctrl");
      break;
    case keyboard::KeyCode::kAlt:
      lv_label_set_text(g_keyboard_label, "Alt");
      break;
    case keyboard::KeyCode::kMeta:
      lv_label_set_text(g_keyboard_label, "Win");
      break;
    case keyboard::KeyCode::kFunction:
      lv_label_set_text(g_keyboard_label, "Fn");
      break;
    case keyboard::KeyCode::kRecord:
      lv_label_set_text(g_keyboard_label, "Record");
      break;
    default:
      lv_label_set_text(g_keyboard_label, "Unknown");
      break;
  }
}

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

void MyTouchpadRead(lv_indev_t*, lv_indev_data_t* data) {
  auto& driver = DeviceDriver::GetInstance();
  cpp_bus_driver::TouchFrame frame;
  cpp_bus_driver::TouchReadStatus status =
      cpp_bus_driver::TouchReadStatus::kNoData;
  if (driver.screen_type() == board::device::ScreenType::kHi8561) {
    status = driver.chip().hi8561_touch->ReadPrimaryTouch(&frame);
  } else if (driver.screen_type() == board::device::ScreenType::kRm69a10) {
    status = driver.chip().gt9895->ReadPrimaryTouch(&frame);
  }

  if (status == cpp_bus_driver::TouchReadStatus::kSuccess &&
      frame.contact_count != 0) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = frame.contacts[0].x;
    data->point.y = frame.contacts[0].y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void MyKeyboardRead(lv_indev_t*, lv_indev_data_t* data) {
  static uint32_t last_key = 0;
  static uint8_t last_key_number = 0;
  static bool pressed_state_flag = false;
  static bool caps_lock_flag = false;
  static cpp_bus_driver::Tca8418::TouchPoint events;
  static size_t event_index = 0;

  // Deliver every FIFO event separately so a quick press/release reaches LVGL.
  if (event_index >= events.info.size()) {
    events.info.clear();
    event_index = 0;
    const uint8_t irq_flags = g_tca8418->GetIrqFlag();
    cpp_bus_driver::Tca8418::IrqStatus irq_status;
    if (g_tca8418->ParseIrqStatus(irq_flags, irq_status)) {
      if (irq_status.fifo_overflow_flag) {
        printf("Keyboard FIFO overflow\n");
        pressed_state_flag = false;
      }
      if (irq_status.gpio_interrupt_flag) {
        uint32_t gpio_flags = 0;
        g_tca8418->GetClearGpioIrqFlag(&gpio_flags);
      }
      g_tca8418->ClearIrqFlag(irq_flags);
      // Poll the FIFO as well as IRQ status to retain events around IRQ clear.
      g_tca8418->GetMultipleTouchPoint(events);
    } else {
      pressed_state_flag = false;
    }
  }

  if (event_index < events.info.size()) {
    const auto& event = events.info[event_index++];
    const bool key_valid =
        event.event_type == cpp_bus_driver::Tca8418::EventType::kKeypad &&
        event.num > 0 && event.num <= keyboard::kMap.size() &&
        event.num <= std::size(Tca8418_Map_Lvgl);
    if (key_valid) {
      const size_t key_index = event.num - 1;
      const auto& mapping = keyboard::kMap[key_index];
      ShowKeyLabel(mapping);

      if (event.press_flag) {
        if (mapping.key == keyboard::KeyCode::kCapsLock) {
          caps_lock_flag = !caps_lock_flag;
          auto& driver = DeviceDriver::GetInstance();
          for (auto led : {DeviceDriver::KeyboardExpansionLed::kLed1,
                   DeviceDriver::KeyboardExpansionLed::kLed2,
                   DeviceDriver::KeyboardExpansionLed::kLed3}) {
            driver.SetKeyboardExpansionLed(led, caps_lock_flag);
          }
        } else {
          last_key_number = event.num;
          last_key = mapping.key == keyboard::KeyCode::kCharacter
                         ? static_cast<uint32_t>(mapping.character)
                         : Tca8418_Map_Lvgl[key_index];
          if (caps_lock_flag && last_key >= 'a' && last_key <= 'z') {
            last_key = last_key - 'a' + 'A';
          }
          pressed_state_flag = true;
        }
      } else if (event.num == last_key_number) {
        pressed_state_flag = false;
      }
    }
  }

  data->continue_reading = event_index < events.info.size();
  data->key = last_key;
  if (pressed_state_flag == false) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;
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
  const int width = lv_display_get_horizontal_resolution(nullptr);
  const int height = lv_display_get_vertical_resolution(nullptr);
  const auto color_format = ScreenColorFormat();
  const size_t draw_buffer_sz =
      static_cast<size_t>(lv_draw_buf_width_to_stride(width, color_format)) *
      height;
  void* draw_buf = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
  assert(draw_buf != nullptr);

  g_canvas = lv_canvas_create(lv_screen_active());
  lv_canvas_set_buffer(g_canvas, draw_buf, width, height, color_format);
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

  const auto& screen_info = DeviceDriver::GetInstance().screen_info();
  lv_display_t* display =
      lv_display_create(screen_info.width, screen_info.height);
  assert(display != nullptr);
  lv_display_set_color_format(display, ScreenColorFormat());

  printf("Allocate separate lvgl draw buffers\n");
  const size_t draw_buffer_sz = std::max(
      static_cast<size_t>(lv_draw_buf_width_to_stride(
          screen_info.width, ScreenColorFormat())) * screen_info.height,
      static_cast<size_t>(lv_draw_buf_width_to_stride(
          screen_info.height, ScreenColorFormat())) * screen_info.width);
  void* buf1 = heap_caps_malloc(
      draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
  assert(buf1);
  if (board::device::screen::kRotationDirection != 0) {
    // Keep the rotated pixels alive until the asynchronous panel flush ends.
    g_rotation_buffer = static_cast<uint8_t*>(heap_caps_malloc(
        draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA));
    assert(g_rotation_buffer != nullptr);
  }

  lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_flush_cb(display, [](lv_display_t* disp, const lv_area_t* area,
                                      uint8_t* px_map) {
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    lv_area_t rotated_area;

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
      lv_draw_sw_rotate(px_map, g_rotation_buffer, src_w, src_h, src_stride,
                        dest_stride, rotation, color_format);

      area = &rotated_area;
      px_map = g_rotation_buffer;
    }

    auto& driver = DeviceDriver::GetInstance();
    bool sent = false;
    switch (driver.screen_type()) {
      case board::device::ScreenType::kHi8561:
        sent = driver.chip().hi8561->SendColorStreamCoordinate(
            area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
        break;
      case board::device::ScreenType::kRm69a10:
        sent = driver.chip().rm69a10->SendColorStreamCoordinate(
            area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
        break;
      default:
        break;
    }
    if (!sent) {
      lv_display_flush_ready(disp);
    }
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
      }(board::device::screen::kRotationDirection));
}

void InitLvglKeyboard(void) {
  lv_obj_t* text_area = lv_textarea_create(g_canvas);
  lv_obj_set_width(text_area, 300);
  lv_obj_set_height(text_area, 150);
  lv_obj_align(text_area, LV_ALIGN_BOTTOM_LEFT, 24, -50);
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

  auto& driver = DeviceDriver::GetInstance();
  if (!driver.InitMinimal() || !driver.InitScreen() || !driver.InitTouch() ||
      !driver.InitScreenBacklight()) {
    printf("Screen or touch initialization failed\n");
    return;
  }
  if (!driver.InitKeyboardExpansion()) {
    printf("Keyboard expansion initialization completed with errors\n");
  }
  if (!driver.IsTca8418Ready() || !driver.IsXl9555Ready() ||
      !driver.IsSy7200aReady()) {
    printf("Keyboard or keyboard backlight initialization failed\n");
    return;
  }

  g_tca8418 = driver.chip().tca8418.get();
  driver.chip().sy7200a->FadeTo(
      {.value = 1, .scale = 2}, 1000, cpp_bus_driver::Pwm::FadeMode::kNoWait);

  InitLvgl();
  InitLvglCanvas();
  InitLvglKeyboard();
  lv_refr_now(lv_display_get_default());
  if (driver.screen_type() == board::device::ScreenType::kHi8561) {
    driver.chip().pt4103->FadeTo({.value = 1, .scale = 1}, 500,
        cpp_bus_driver::Pwm::FadeMode::kNoWait);
  } else if (driver.screen_type() == board::device::ScreenType::kRm69a10) {
    for (uint16_t brightness = 0; brightness <= 255; brightness += 5) {
      driver.chip().rm69a10->SetBrightness(static_cast<uint8_t>(brightness));
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  if (xTaskCreate(LvglUiTask, "LvglUiTask", 16 * 1024, NULL, 2, NULL) !=
      pdPASS) {
    printf("LVGL task creation failed\n");
  }
}
