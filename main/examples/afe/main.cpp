/*
 * @Description: afe
 * @Author: LILYGO_L
 * @Date: 2025-07-22 15:02:53
 * @LastEditTime: 2026-04-22 14:42:10
 * @License: GPL 3.0
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "esp_afe_sr_models.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hiesp.h"
#include "hilexin.h"
#include "lilygo_device_driver.h"

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 16000

esp_afe_sr_iface_t* g_afe_handle = NULL;

void feed_Task(void* arg) {
  esp_afe_sr_data_t* afe_data = (esp_afe_sr_data_t*)arg;
  int audio_chunksize = g_afe_handle->get_feed_chunksize(afe_data);
  int nch = g_afe_handle->get_feed_channel_num(afe_data);

  const size_t sample_count = static_cast<size_t>(audio_chunksize) * nch;
  const size_t buffer_size = sample_count * sizeof(int16_t);
  auto i2s_buffer = std::make_unique<int16_t[]>(sample_count);
  auto& es8311 =
      lilygo_device_driver::TDisplayP4Driver::GetInstance().chip().es8311;

  while (1) {
    size_t filled = 0;
    while (filled < buffer_size) {
      const size_t bytes_read = es8311->ReadI2s(
          reinterpret_cast<uint8_t*>(i2s_buffer.get()) + filled,
          buffer_size - filled);
      if (bytes_read == 0) {
        printf("ES8311 audio read failed\n");
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }
      filled += bytes_read;
    }
    g_afe_handle->feed(afe_data, i2s_buffer.get());
  }

  vTaskDelete(NULL);
}

void detect_Task(void* arg) {
  esp_afe_sr_data_t* afe_data = (esp_afe_sr_data_t*)arg;
  printf("------------detect start------------\n");

  // modify wakenet detection threshold
  // g_afe_handle->set_wakenet_threshold(afe_data, 1, 0.6); // set model1's
  // threshold to 0.6 g_afe_handle->set_wakenet_threshold(afe_data, 2, 0.6); //
  // set model2's threshold to 0.6
  g_afe_handle->reset_wakenet_threshold(
      afe_data, 1);  // reset model1's threshold to default
  g_afe_handle->reset_wakenet_threshold(
      afe_data, 2);  // reset model2's threshold to default

  size_t cycle_time = 0;
  while (1) {
    afe_fetch_result_t* res = g_afe_handle->fetch(afe_data);
    if (!res || res->ret_value == ESP_FAIL) {
      printf("fetch error!\n");
      break;
    }

    if (esp_log_timestamp() > cycle_time) {
      // 这里检测是否有语音
      printf("vad state: %d\n", res->vad_state);
      cycle_time = esp_log_timestamp() + 1000;
    }

    if (res->wakeup_state == WAKENET_DETECTED) {
      printf("wakeword detected\n");
      printf("model index:%d, word index:%d\n", res->wakenet_model_index,
             res->wake_word_index);
      printf("-----------LISTENING-----------\n");
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  vTaskDelete(NULL);
}

void Afe_Init() {
  srmodel_list_t* models = esp_srmodel_init("model");
  if (models) {
    for (int i = 0; i < models->num; i++) {
      if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
        printf("wakenet model in flash: %s\n", models->model_name[i]);
      }
    }
  }

  if (models == nullptr) {
    printf("Speech models could not be loaded\n");
    return;
  }

  // ES8311 has one microphone in a two-slot I2S stream.
  afe_config_t* afe_config =
      afe_config_init("MN", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
  if (afe_config == nullptr) {
    printf("AFE configuration failed\n");
    return;
  }

  // print/modify wake word model.
  if (afe_config->wakenet_model_name) {
    printf("wakeword model in AFE config: %s\n",
           afe_config->wakenet_model_name);
  }
  if (afe_config->wakenet_model_name_2) {
    printf("wakeword model in AFE config: %s\n",
           afe_config->wakenet_model_name_2);
  }

  g_afe_handle =
      const_cast<esp_afe_sr_iface_t*>(esp_afe_handle_from_config(afe_config));
  if (g_afe_handle == nullptr) {
    afe_config_free(afe_config);
    printf("AFE interface unavailable\n");
    return;
  }
  esp_afe_sr_data_t* afe_data = g_afe_handle->create_from_config(afe_config);

  //
  afe_config_free(afe_config);

  if (afe_data == nullptr) {
    printf("AFE creation failed\n");
    return;
  }

  xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5,
                          NULL, 0);
  xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5,
                          NULL, 1);
}

void Wakenet_Init() {
  srmodel_list_t* models = esp_srmodel_init("model");
  char* model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "hilexin");
  esp_wn_iface_t* wakenet =
      (esp_wn_iface_t*)esp_wn_handle_from_name(model_name);
  model_iface_data_t* model_data = wakenet->create(model_name, DET_MODE_95);

  int audio_chunksize =
      wakenet->get_samp_chunksize(model_data) * sizeof(int16_t);
  int16_t* buffer = (int16_t*)malloc(audio_chunksize);
  unsigned char* data = NULL;
  size_t data_size = 0;
  if (strstr(model_name, "hiesp") != NULL) {
    data = (unsigned char*)hiesp;
    data_size = sizeof(hiesp);
    printf("wake word: %s, size:%zu\n", "hiesp", data_size);
  } else if (strstr(model_name, "hilexin") != NULL) {
    data = (unsigned char*)hilexin;
    data_size = sizeof(hilexin);
    printf("wake word: %s, size:%zu\n", "hilexin", data_size);
  }

  int chunks = 0;
  while (1) {
    if ((chunks + 1) * audio_chunksize <= data_size) {
      memcpy(buffer, data + chunks * audio_chunksize, audio_chunksize);
    } else {
      chunks = 0;
      // break;
    }

    wakenet_state_t state = wakenet->detect(model_data, buffer);
    if (state == WAKENET_DETECTED) {
      printf("Detected\n");
    }
    chunks++;

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  wakenet->destroy(model_data);
  vTaskDelete(NULL);
}

extern "C" void app_main() {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!driver.InitMinimal() || !driver.InitEs8311() ||
      !driver.SetEs8311OperatingMode(
          lilygo_device_driver::TDisplayP4Driver::Es8311OperatingMode::kCapture)) {
    printf("ES8311 capture initialization failed\n");
    return;
  }

  auto& es8311 = driver.chip().es8311;
  if (!es8311->SetI2sChannelEnable(false)) {
    printf("ES8311 I2S stop failed\n");
    return;
  }
  const bool clock_configured =
      es8311->ReconfigureClock(MCLK_MULTIPLE, SAMPLE_RATE);
  const bool channel_enabled = es8311->SetI2sChannelEnable(true);
  if (!clock_configured || !channel_enabled) {
    printf("ES8311 16 kHz clock configuration failed\n");
    return;
  }

  // Wakenet_Init();
  Afe_Init();
}
