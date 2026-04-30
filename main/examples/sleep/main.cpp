/*
 * @Description: deep_sleep
 * @Author: LILYGO_L
 * @Date: 2025-05-12 14:08:31
 * @LastEditTime: 2026-04-30 15:37:17
 * @License: GPL 3.0
 */
#include "app_video.h"
#include "driver/ppa.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_hosted.h"
#include "esp_netif.h"
#include "esp_private/esp_cache_private.h"
#include "esp_private/wifi.h"
#include "esp_sleep.h"
#include "esp_video_init.h"
#include "esp_wifi.h"
#include "esp_wifi_remote.h"
#include "ethernet_init.h"
#include "lilygo_device_driver_library.h"

constexpr char kWifiSsid[] = "xinyuandianzi";
constexpr char kWifiPassword[] = "AA15994823428";

#define EXAMPLE_SLEEP_MODE_DEEP_SLEEP 0
#define EXAMPLE_SLEEP_MODE_LIGHT_SLEEP 1
#define EXAMPLE_SLEEP_MODE EXAMPLE_SLEEP_MODE_DEEP_SLEEP

uint8_t g_eth_port_count = 0;
esp_eth_handle_t* g_eth_handles = nullptr;
esp_netif_t* g_eth_netif = nullptr;

ppa_client_handle_t g_ppa_srm_handle = nullptr;
size_t g_data_cache_line_size = 0;
int32_t g_video_cam_fd0 = -1;
int32_t g_fps_count = 0;
int64_t g_start_time = 0;
bool g_wifi_initialized = false;
bool g_ethernet_initialized = false;
bool g_app_video_initialized = false;
bool g_app_video_stream_started = false;

void EthernetEventHandler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
  uint8_t mac_addr[6] = {0};
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t*)event_data;

  switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
      esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
      printf("Ethernet link up\n");
      printf("Ethernet hw addr %02x:%02x:%02x:%02x:%02x:%02x\n", mac_addr[0],
          mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      printf("Ethernet link down\n");
      break;
    case ETHERNET_EVENT_START:
      printf("Ethernet started\n");
      break;
    case ETHERNET_EVENT_STOP:
      printf("Ethernet stopped\n");
      break;
    default:
      break;
  }
}

void GotIpEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
    void* event_data) {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  const esp_netif_ip_info_t* ip_info = &event->ip_info;

  printf("Ethernet get ip address\n");
  printf("~~~~~~~~~~~\n");
  printf("Eth ip: %d.%d.%d.%d\n", IP2STR(&ip_info->ip));
  printf("Eth mask: %d.%d.%d.%d\n", IP2STR(&ip_info->netmask));
  printf("Eth gw: %d.%d.%d.%d\n", IP2STR(&ip_info->gw));
  printf("~~~~~~~~~~~\n");
}

void WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id,
    void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    printf("Wifi started\n");
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    printf("Wifi disconnected, retrying\n");
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    printf("Wifi got ip: " IPSTR "\n", IP2STR(&event->ip_info.ip));
  }
}

void WifiCleanup() {
  if (!g_wifi_initialized) {
    return;
  }

  esp_wifi_disconnect();
  vTaskDelay(pdMS_TO_TICKS(200));

  wifi_event_sta_disconnected_t disconnect_event = {};
  disconnect_event.reason = WIFI_REASON_ASSOC_LEAVE;
  esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_event,
      sizeof(wifi_event_sta_disconnected_t), portMAX_DELAY);
  vTaskDelay(pdMS_TO_TICKS(100));

  esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_STOP, nullptr, 0, portMAX_DELAY);
  vTaskDelay(pdMS_TO_TICKS(500));

  esp_wifi_internal_reg_rxcb(WIFI_IF_STA, nullptr);
  esp_wifi_internal_reg_rxcb(WIFI_IF_AP, nullptr);

  esp_wifi_stop();
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_wifi_deinit();
  g_wifi_initialized = false;
}

void InitEthernet() {
  char if_key_str[10];
  char if_desc_str[10];
  esp_netif_config_t cfg;
  esp_netif_inherent_config_t eth_netif_cfg;

  ethernet_init_all(&g_eth_handles, &g_eth_port_count);
  ESP_ERROR_CHECK(esp_event_handler_register(
      ETH_EVENT, ESP_EVENT_ANY_ID, &EthernetEventHandler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(
      IP_EVENT, IP_EVENT_ETH_GOT_IP, &GotIpEventHandler, nullptr));

  if (g_eth_port_count == 1) {
    eth_netif_cfg = *(ESP_NETIF_BASE_DEFAULT_ETH);
  } else {
    eth_netif_cfg =
        (esp_netif_inherent_config_t)ESP_NETIF_INHERENT_DEFAULT_ETH();
  }

  cfg = (esp_netif_config_t){
      .base = &eth_netif_cfg,
      .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH,
  };

  for (int i = 0; i < g_eth_port_count; i++) {
    sprintf(if_key_str, "ETH_%d", i);
    sprintf(if_desc_str, "eth%d", i);
    eth_netif_cfg.if_key = if_key_str;
    eth_netif_cfg.if_desc = if_desc_str;
    eth_netif_cfg.route_prio -= i * 5;
    g_eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(esp_netif_attach(
        g_eth_netif, esp_eth_new_netif_glue(g_eth_handles[i])));
    ESP_ERROR_CHECK(esp_eth_start(g_eth_handles[i]));
  }

  g_ethernet_initialized = g_eth_handles != nullptr && g_eth_port_count > 0;
}

void EthernetCleanup() {
  if (!g_ethernet_initialized || g_eth_handles == nullptr ||
      g_eth_port_count == 0) {
    return;
  }

  printf("Stop and deinitialize ethernet network\n");

  esp_eth_handle_t eth_handle = g_eth_handles[0];
  uint32_t reg_value = 0;
  esp_eth_phy_reg_rw_data_t read_data = {
      .reg_addr = 0,
      .reg_value_p = &reg_value,
  };

  esp_err_t ret = esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &read_data);
  if (ret != ESP_OK) {
    printf("esp_eth_ioctl failed (error code: %s)\n", esp_err_to_name(ret));
  }

  printf("Register 0: %#lX\n", reg_value);
  reg_value |= (1UL << 11);

  esp_eth_phy_reg_rw_data_t write_data = {
      .reg_addr = 0,
      .reg_value_p = &reg_value,
  };

  ret = esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &write_data);
  if (ret != ESP_OK) {
    printf("esp_eth_ioctl failed (error code: %s)\n", esp_err_to_name(ret));
  }

  for (int i = 0; i < g_eth_port_count; i++) {
    esp_eth_stop(g_eth_handles[i]);
  }

  if (g_eth_netif != nullptr) {
    esp_netif_destroy(g_eth_netif);
    g_eth_netif = nullptr;
  }

  ethernet_deinit_all(g_eth_handles);
  g_eth_handles = nullptr;
  g_eth_port_count = 0;
  g_ethernet_initialized = false;
}

bool InitWifiConnect() {
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  g_wifi_initialized = true;

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiEventHandler, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiEventHandler, nullptr, nullptr));

  wifi_config_t wifi_config = {};
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), kWifiSsid,
      sizeof(wifi_config.sta.ssid) - 1);
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), kWifiPassword,
      sizeof(wifi_config.sta.password) - 1);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  printf("Connecting to wifi: %s\n", kWifiSsid);
  ESP_ERROR_CHECK(esp_wifi_connect());

  for (int i = 0; i < 10; i++) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK && ap_info.ssid[0] != 0) {
      printf("Wifi connected\n");
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  return false;
}

void CameraVideoFrameOperation(uint8_t* camera_buf, uint8_t camera_buf_index,
    uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len) {
  g_fps_count++;
  if (g_fps_count == 50) {
    int64_t end_time = esp_timer_get_time();
    printf("Fps: %f\n", 1000000.0 / ((end_time - g_start_time) / 50.0));
    g_start_time = end_time;
    g_fps_count = 0;

    printf("Camera buffer hes: %lu, ves: %lu, len: %d KB\n", camera_buf_hes,
        camera_buf_ves, camera_buf_len / 1024);
  }
}

void AppVideoCleanup() {
  if (!g_app_video_initialized && g_video_cam_fd0 < 0) {
    return;
  }

  if (g_app_video_stream_started) {
    esp_err_t ret = app_video_stream_task_stop(g_video_cam_fd0);
    if (ret != ESP_OK) {
      printf("app_video_stream_task_stop failed (error code: %#X)\n", ret);
    }
    g_app_video_stream_started = false;
  }

  if (g_video_cam_fd0 >= 0) {
    esp_err_t ret = app_video_close(g_video_cam_fd0);
    if (ret != ESP_OK) {
      printf("app_video_close failed (error code: %#X)\n", ret);
    }
  }

  g_video_cam_fd0 = -1;
  g_app_video_initialized = false;
}

bool InitAppVideo() {
  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();

  ppa_client_config_t ppa_srm_config = {
      .oper_type = PPA_OPERATION_SRM,
  };

  esp_err_t ret = ppa_register_client(&ppa_srm_config, &g_ppa_srm_handle);
  if (ret != ESP_OK) {
    printf("ppa_register_client failed (error code: %#X)\n", ret);
    return false;
  }

  ret = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &g_data_cache_line_size);
  if (ret != ESP_OK) {
    printf("esp_cache_get_alignment failed (error code: %#X)\n", ret);
    return false;
  }

  esp_video_init_csi_config_t csi_config = {
      .sccb_config =
          {
              .init_sccb = false,
              .i2c_handle = driver.bus().sgm38121_i2c_bus->bus_handle(),
              .freq = static_cast<uint32_t>(100000),
          },
      .reset_pin = gpio_num_t::GPIO_NUM_NC,
      .pwdn_pin = gpio_num_t::GPIO_NUM_NC,
      .dont_init_ldo = true,
  };

  esp_video_init_config_t cam_config = {
      .csi = &csi_config,
  };

  ret = esp_video_init(&cam_config);
  if (ret != ESP_OK) {
    printf("esp_video_init failed (error code: %#X)\n", ret);
    return false;
  }

#if (defined CONFIG_CAMERA_TYPE_SC2336) || (defined CONFIG_CAMERA_TYPE_OV2710)
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
  g_video_cam_fd0 =
      app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, APP_VIDEO_FMT_RGB565);
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
  g_video_cam_fd0 =
      app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, APP_VIDEO_FMT_RGB888);
#else
#error "no macro definition is set"
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
  g_video_cam_fd0 =
      app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, APP_VIDEO_FMT_RGB565);
#else
#error "no macro definition is set"
#endif

  if (g_video_cam_fd0 < 0) {
    printf("app_video_open failed (video_cam_fd0: %ld)\n", g_video_cam_fd0);
    return false;
  }
  g_app_video_initialized = true;

  ret = app_video_set_bufs(g_video_cam_fd0, CAMERA_BUFFER_COUNT, nullptr);
  if (ret != ESP_OK) {
    printf("app_video_set_bufs failed (error code: %#X)\n", ret);
    AppVideoCleanup();
    return false;
  }

  ret = app_video_register_frame_operation_cb(CameraVideoFrameOperation);
  if (ret != ESP_OK) {
    printf("app_video_register_frame_operation_cb failed (error code: %#X)\n",
        ret);
    AppVideoCleanup();
    return false;
  }

  ret = app_video_stream_task_start(g_video_cam_fd0, 0);
  if (ret != ESP_OK) {
    printf("app_video_stream_task_start failed (error code: %#X)\n", ret);
    AppVideoCleanup();
    return false;
  }

  g_start_time = esp_timer_get_time();
  g_app_video_stream_started = true;
  return true;
}

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  InitEthernet();

  if (!InitAppVideo()) {
    printf("InitAppVideo failed\n");
  }

  if (!InitWifiConnect()) {
    printf("InitWifiConnect failed\n");
  }

  vTaskDelay(pdMS_TO_TICKS(1000));

  printf("Sleep start\n");

  AppVideoCleanup();
  WifiCleanup();

  esp_err_t ret = esp_hosted_deinit();
  if (ret != ESP_OK) {
    printf("esp_hosted_deinit failed (error code: %s)\n", esp_err_to_name(ret));
  }

  EthernetCleanup();

#if EXAMPLE_SLEEP_MODE == EXAMPLE_SLEEP_MODE_DEEP_SLEEP
  if (!driver.SetSleep(
          lilygo_device_driver::TDisplayP4Driver::SleepLevel::kPowerOff,
          true)) {
    printf("SetSleep failed\n");
  }

  printf("Entering deep sleep\n");
  uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);
  esp_deep_sleep_start();
#elif EXAMPLE_SLEEP_MODE == EXAMPLE_SLEEP_MODE_LIGHT_SLEEP
  if (!driver.SetSleep(
          lilygo_device_driver::TDisplayP4Driver::SleepLevel::kPowerOff,
          true)) {
    printf("SetSleep failed\n");
  }

  gpio_config_t boot_config = {
      .pin_bit_mask = 1ULL << ESP32P4_BOOT,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ret = gpio_config(&boot_config);
  if (ret != ESP_OK) {
    printf("gpio_config failed (error code: %s)\n", esp_err_to_name(ret));
  }

  ret = gpio_wakeup_enable(
      static_cast<gpio_num_t>(ESP32P4_BOOT), GPIO_INTR_LOW_LEVEL);
  if (ret != ESP_OK) {
    printf(
        "gpio_wakeup_enable failed (error code: %s)\n", esp_err_to_name(ret));
  }

  ret = esp_sleep_enable_gpio_wakeup();
  if (ret != ESP_OK) {
    printf("esp_sleep_enable_gpio_wakeup failed (error code: %s)\n",
        esp_err_to_name(ret));
  }

  printf("Entering light sleep\n");
  uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);
  ret = esp_light_sleep_start();
  if (ret != ESP_OK) {
    printf("esp_light_sleep_start failed (error code: %s)\n",
        esp_err_to_name(ret));
  }

  printf("Wake up from light sleep, cause: %d\n", esp_sleep_get_wakeup_cause());

  ret = gpio_wakeup_disable(static_cast<gpio_num_t>(ESP32P4_BOOT));
  if (ret != ESP_OK) {
    printf(
        "gpio_wakeup_disable failed (error code: %s)\n", esp_err_to_name(ret));
  }

  if (!driver.SetSleep(
          lilygo_device_driver::TDisplayP4Driver::SleepLevel::kPowerOff,
          false)) {
    printf("SetSleep failed\n");
  }
#else
#error "no macro definition is set"
#endif

  while (1) {
    printf("Wake up\n");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
