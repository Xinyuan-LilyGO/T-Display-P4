/*
 * @Description: afe
 * @Author: LILYGO_L
 * @Date: 2025-07-22 15:02:53
 * @LastEditTime: 2026-04-22 14:42:10
 * @License: GPL 3.0
 */
#include "esp_afe_sr_models.h"
#include "hiesp.h"
#include "hilexin.h"
#include "lilygo_device_driver_library.h"

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 16000

esp_afe_sr_iface_t* g_afe_handle = NULL;

void feed_Task(void* arg) {
  esp_afe_sr_data_t* afe_data = (esp_afe_sr_data_t*)arg;
  int audio_chunksize = g_afe_handle->get_feed_chunksize(afe_data);
  int nch = g_afe_handle->get_feed_channel_num(afe_data);

  auto i2s_buffer =
      std::make_unique<int16_t[]>(audio_chunksize * sizeof(int16_t));

  while (1) {
    lilygo_device_driver::TDisplayP4Driver::GetInstance()
        .chip()
        .es8311->ReadData(i2s_buffer.get(), audio_chunksize * sizeof(uint16_t));

    // lilygo_device_driver::TDisplayP4Driver::GetInstance()
    //     .chip()
    //     .es8311->WriteData(i2s_buffer, audio_chunksize * sizeof(uint16_t));

    // for (uint8_t i = 0; i < 10; i++)
    // {
    //     printf("ReadData: %d\n", i2s_buffer[i]);
    // }

    g_afe_handle->feed(afe_data, i2s_buffer.get());

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  vTaskDelete(NULL);
}

void detect_Task(void* arg) {
  esp_afe_sr_data_t* afe_data = (esp_afe_sr_data_t*)arg;
  int afe_chunksize = g_afe_handle->get_fetch_chunksize(afe_data);
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

  afe_config_t* afe_config =
      afe_config_init("MM", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);

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
  esp_afe_sr_data_t* afe_data = g_afe_handle->create_from_config(afe_config);

  //
  afe_config_free(afe_config);

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
    printf("wake word: %s, size:%d\n", "hiesp", data_size);
  } else if (strstr(model_name, "hilexin") != NULL) {
    data = (unsigned char*)hilexin;
    data_size = sizeof(hilexin);
    printf("wake word: %s, size:%d\n", "hilexin", data_size);
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

  lilygo_device_driver::TDisplayP4Driver::GetInstance().Init();

  // Wakenet_Init();
  Afe_Init();
}
