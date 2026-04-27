/*
 * @Description: screen_camera
 * @Author: LILYGO_L
 * @Date: 2025-06-13 11:45:00
 * @LastEditTime: 2026-04-25 16:45:21
 * @License: GPL 3.0
 */
#include "app_video.h"
#include "driver/ppa.h"
#include "esp_private/esp_cache_private.h"
#include "esp_video_init.h"
#include "lilygo_device_driver_library.h"

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

ppa_client_handle_t g_ppa_srm_handle = NULL;
size_t g_data_cache_line_size = 0;
int32_t g_video_cam_fd0;

int64_t g_start_time;

bool g_camera_init_flag = false;

#if defined CONFIG_SCREEN_TYPE_HI8561
cpp_bus_driver::Hi8561* g_screen = nullptr;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
cpp_bus_driver::Rm69a10* g_screen = nullptr;
#endif

void CameraVideoFrameOperation(uint8_t* camera_buf, uint8_t camera_buf_index,
                               uint32_t camera_buf_hes, uint32_t camera_buf_ves,
                               size_t camera_buf_len) {
  static int32_t fps_count = 0;
  fps_count++;
  if (fps_count == 50) {
    int64_t end_time = esp_timer_get_time();
    printf("fps: %f\n", 1000000.0 / ((end_time - g_start_time) / 50.0));
    g_start_time = end_time;
    fps_count = 0;

    printf("camera_buf_hes: %lu, camera_buf_ves: %lu, camera_buf_len: %d KB\n",
           camera_buf_hes, camera_buf_ves, camera_buf_len / 1024);
  }

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
#if defined CONFIG_SCREEN_TYPE_HI8561
  auto screen_status = driver.status().hi8561.init_flag;
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  auto screen_status = driver.status().rm69a10.init_flag;
#endif

  if (!screen_status) {
    return;
  }

  uint32_t input_img_block_width = (camera_buf_hes - SCREEN_WIDTH) / 2;
  uint32_t input_img_block_height = 0;
  uint32_t input_img_width = SCREEN_WIDTH;
  uint32_t input_img_height = camera_buf_ves;

  uint32_t output_img_width = input_img_width;
  uint32_t output_img_height = input_img_height;

  size_t output_buffer_size =
      output_img_width * output_img_height * (SCREEN_BITS_PER_PIXEL / 8);

  auto output_buffer =
      std::unique_ptr<uint8_t[], std::function<void(uint8_t*)>>(
          (uint8_t*)heap_caps_aligned_calloc(
              g_data_cache_line_size, 1, output_buffer_size, MALLOC_CAP_SPIRAM),
          [](uint8_t* p) { heap_caps_free(p); });
  if (output_buffer == NULL) {
    printf("heap_caps_malloc failed\n");
    return;
  }

  ppa_srm_oper_config_t srm_config = {
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
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
              .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#endif
          },

      .out =
          {
              .buffer = output_buffer.get(),
              .buffer_size =
                  ALIGN_UP(output_buffer_size, g_data_cache_line_size),
              .pic_w = output_img_width,
              .pic_h = output_img_height,
              .block_offset_x = 0,
              .block_offset_y = 0,
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
              .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB565,
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
              .srm_cm = ppa_srm_color_mode_t::PPA_SRM_COLOR_MODE_RGB888,
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
#endif
#elif if SCREEN_ROTATION_DIRECTION == 90
      .mirror_y = false,
#endif
      .rgb_swap = false,
      .byte_swap = false,
      .mode = PPA_TRANS_MODE_BLOCKING,
  };

  esp_err_t result = ppa_do_scale_rotate_mirror(g_ppa_srm_handle, &srm_config);
  if (result != ESP_OK) {
    printf("ppa_do_scale_rotate_mirror failed (error code: %#X)\n", result);
    return;
  }

  if (!g_screen->SendColorStreamCoordinate(
          0, (SCREEN_HEIGHT - output_img_height) / 2, output_img_width,
          output_img_height + ((SCREEN_HEIGHT - output_img_height) / 2),
          output_buffer.get())) {
    printf("SendColorStreamCoordinate failed\n");
    return;
  }
}

bool AppVideoInit() {
  ppa_client_config_t ppa_srm_config = {
      .oper_type = PPA_OPERATION_SRM,
  };
  esp_err_t result = ppa_register_client(&ppa_srm_config, &g_ppa_srm_handle);
  if (result != ESP_OK) {
    printf("ppa_register_client failed (error code: %#X)\n", result);
    return false;
  }
  result = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &g_data_cache_line_size);
  if (result != ESP_OK) {
    printf("esp_cache_get_alignment failed (error code: %#X)\n", result);
    return false;
  }

  esp_video_init_csi_config_t csi_config = {
      .sccb_config =
          {
              .init_sccb = false,
              .i2c_handle =
                  lilygo_device_driver::TDisplayP4Driver::GetInstance()
                      .bus()
                      .sgm38121_i2c_bus->bus_handle(),
              .freq = static_cast<uint32_t>(100000),
          },
      .reset_pin = gpio_num_t ::GPIO_NUM_NC,
      .pwdn_pin = gpio_num_t ::GPIO_NUM_NC,

      .dont_init_ldo = true,
  };

  esp_video_init_config_t cam_config = {
      .csi = &csi_config,
  };

  result = esp_video_init(&cam_config);
  if (result != ESP_OK) {
    printf("esp_video_init failed (error code: %#X)\n", result);
    return false;
  }

#if (defined CONFIG_CAMERA_TYPE_SC2336) || (defined CONFIG_CAMERA_TYPE_OV2710)
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
  g_video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME,
                                   video_fmt_t::APP_VIDEO_FMT_RGB565);
  if (g_video_cam_fd0 < 0) {
    printf("video cam open failed (g_video_cam_fd0: %ld)\n", g_video_cam_fd0);
    return false;
  }
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
  g_video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME,
                                   video_fmt_t::APP_VIDEO_FMT_RGB888);
  if (g_video_cam_fd0 < 0) {
    printf("video cam open failed (g_video_cam_fd0: %ld)\n", g_video_cam_fd0);
    return false;
  }
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
  g_video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME,
                                   video_fmt_t::APP_VIDEO_FMT_RGB565);
  if (g_video_cam_fd0 < 0) {
    printf("video cam open failed (g_video_cam_fd0: %ld)\n", g_video_cam_fd0);
    return false;
  }
#endif

  result = app_video_set_bufs(g_video_cam_fd0, CAMERA_BUFFER_COUNT, NULL);
  if (result != ESP_OK) {
    printf("app_video_set_bufs failed (error code: %#X)\n", result);
    return false;
  }

  result = app_video_register_frame_operation_cb(CameraVideoFrameOperation);
  if (result != ESP_OK) {
    printf("app_video_register_frame_operation_cb failed (error code: %#X)\n",
           result);
    return false;
  }

  result = app_video_stream_task_start(g_video_cam_fd0, 0);
  if (result != ESP_OK) {
    printf("app_video_stream_task_start failed (error code: %#X)\n", result);
    return false;
  }

  app_video_stream_task_stop(g_video_cam_fd0);

  return true;
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

  if (!AppVideoInit()) {
    printf("AppVideoInit failed\n");
    g_camera_init_flag = false;
  } else {
    printf("AppVideoInit success\n");
    g_camera_init_flag = true;
  }

  // 设置整个屏幕为白色
  size_t screen_size =
      SCREEN_WIDTH * SCREEN_HEIGHT * 2;  // RGB565: 2 bytes per pixel
  void* white_buf =
      heap_caps_aligned_calloc(16, 1, screen_size, MALLOC_CAP_SPIRAM);
  if (white_buf) {
    uint16_t* p = (uint16_t*)white_buf;
    for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
      p[i] = 0xFFFF;  // RGB565白色
    }

    if (!g_screen->SendColorStreamCoordinate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                             white_buf)) {
      printf("SendColorStreamCoordinate white failed\n");
    }
    heap_caps_free(white_buf);
  }

#if defined CONFIG_SCREEN_TYPE_HI8561
  driver.chip().hi8561_backlight->StartGradientTime(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
  for (uint8_t i = 0; i < 255; i += 5) {
    g_screen->SetBrightness(i);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
#endif

  if (g_camera_init_flag) {
    esp_err_t result =
        app_video_set_bufs(g_video_cam_fd0, CAMERA_BUFFER_COUNT, NULL);
    if (result != ESP_OK) {
      printf("app_video_set_bufs failed (error code: %#X)\n", result);
    }

    result = app_video_stream_task_start(g_video_cam_fd0, 0);
    if (result != ESP_OK) {
      printf("app_video_stream_task_start failed (error code: %#X)\n", result);
    } else {
      // Get the initial time for frame rate statistics
      g_start_time = esp_timer_get_time();
    }
  }

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
