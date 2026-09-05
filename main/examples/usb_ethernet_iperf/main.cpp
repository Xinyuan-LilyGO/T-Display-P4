/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-05-05 18:15:09
 * @LastEditTime: 2026-05-06 15:35:17
 * @License: GPL 3.0
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "iot_eth.h"
#include "iot_eth_netif_glue.h"
#include "iot_usbh_cdc.h"
#include "iot_usbh_ecm.h"
#include "iperf_cmd.h"
#include "lilygo_device_driver.h"
#include "usb/usb_host.h"

static const char* TAG = "rtl8152b_iperf";

static constexpr uint16_t RTL8152B_VID = 0x0BDA;
static constexpr uint16_t RTL8152B_PID = 0x8152;
static constexpr EventBits_t EVENT_GOT_IP_BIT = BIT0;
static constexpr uint8_t RTL8152B_FALLBACK_MAC[6] = {
    0x02, 0x00, 0x00, 0x12, 0x15, 0x2B};

static EventGroupHandle_t s_event_group = nullptr;

static bool usb_host_enum_filter_cb(
    const usb_device_desc_t* dev_desc, uint8_t* bConfigurationValue) {
  if (dev_desc->idVendor == RTL8152B_VID &&
      dev_desc->idProduct == RTL8152B_PID) {
    *bConfigurationValue = 2;
    ESP_LOGI(TAG, "USB device VID:%04X PID:%04X use config %u",
        dev_desc->idVendor, dev_desc->idProduct, *bConfigurationValue);
    return true;
  }

  *bConfigurationValue = 1;
  ESP_LOGI(TAG, "USB device VID:%04X PID:%04X use config %u",
      dev_desc->idVendor, dev_desc->idProduct, *bConfigurationValue);
  return true;
}

static void usb_lib_task(void* arg) {
  const usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .root_port_unpowered = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
      .enum_filter_cb = usb_host_enum_filter_cb,
      .fifo_settings_custom = {},
      .peripheral_map = 0,
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));
  xTaskNotifyGive(static_cast<TaskHandle_t>(arg));

  bool has_clients = true;
  bool has_devices = false;
  while (has_clients) {
    uint32_t event_flags = 0;
    ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));

    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      ESP_LOGI(TAG, "USB host has no clients");
      if (usb_host_device_free_all() == ESP_OK) {
        ESP_LOGI(TAG, "All USB devices are free");
        has_clients = false;
      } else {
        has_devices = true;
      }
    }

    if (has_devices && (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)) {
      ESP_LOGI(TAG, "All USB devices are free");
      has_clients = false;
    }
  }

  ESP_LOGI(TAG, "Uninstall USB Host library");
  vTaskDelay(pdMS_TO_TICKS(100));
  usb_host_uninstall();
  vTaskDelete(nullptr);
}

static void eth_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
  if (event_base == IOT_ETH_EVENT) {
    switch (event_id) {
      case IOT_ETH_EVENT_START:
        ESP_LOGI(TAG, "IOT_ETH_EVENT_START");
        break;
      case IOT_ETH_EVENT_STOP:
        ESP_LOGI(TAG, "IOT_ETH_EVENT_STOP");
        break;
      case IOT_ETH_EVENT_CONNECTED:
        ESP_LOGI(TAG, "RTL8152B link up");
        break;
      case IOT_ETH_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "RTL8152B link down");
        xEventGroupClearBits(s_event_group, EVENT_GOT_IP_BIT);
        break;
      default:
        ESP_LOGI(TAG, "IOT_ETH_EVENT id=%ld", event_id);
        break;
    }
    return;
  }

  if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
    const ip_event_got_ip_t* event =
        static_cast<const ip_event_got_ip_t*>(event_data);
    ESP_LOGI(TAG, "RTL8152B got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "Gateway: " IPSTR ", Netmask: " IPSTR,
        IP2STR(&event->ip_info.gw), IP2STR(&event->ip_info.netmask));
    xEventGroupSetBits(s_event_group, EVENT_GOT_IP_BIT);
  }
}

static void start_iperf_console(void) {
  esp_console_repl_t* repl = nullptr;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  esp_console_dev_uart_config_t uart_config =
      ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

  repl_config.prompt = "rtl8152b>";
  repl_config.max_history_len = 8;
  repl_config.task_priority = 24;

  ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
  ESP_ERROR_CHECK(app_register_iperf_commands());

  printf("\n ========================================================\n");
  printf(" |              RTL8152B USB Ethernet iperf             |\n");
  printf(" |                                                      |\n");
  printf(" | Type 'help' to display a list of available commands. |\n");
  printf(" | Example: iperf -s -i 1 -t 30                         |\n");
  printf(" | Example: iperf -c <server_ip> -i 1 -t 30             |\n");
  printf(" |                                                      |\n");
  printf(" ========================================================\n\n");

  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

static esp_err_t install_rtl8152b_ecm(void) {
  static usb_device_match_id_t dev_match_id[2] = {};
  dev_match_id[0].match_flags = USB_DEVICE_ID_MATCH_VID_PID;
  dev_match_id[0].idVendor = RTL8152B_VID;
  dev_match_id[0].idProduct = RTL8152B_PID;

  iot_usbh_ecm_config_t ecm_cfg = {
      .match_id_list = dev_match_id,
  };

  iot_eth_driver_t* ecm_driver = nullptr;
  esp_err_t ret = iot_eth_new_usb_ecm(&ecm_cfg, &ecm_driver);
  if (ret != ESP_OK) {
    return ret;
  }

  iot_eth_handle_t eth_handle = nullptr;
  iot_eth_config_t eth_cfg = {
      .driver = ecm_driver,
      .stack_input = nullptr,
  };
  ESP_RETURN_ON_ERROR(
      iot_eth_install(&eth_cfg, &eth_handle), TAG, "install iot_eth failed");

  esp_netif_inherent_config_t inherent_cfg = ESP_NETIF_INHERENT_DEFAULT_ETH();
  inherent_cfg.if_key = "USBECM";
  inherent_cfg.if_desc = "rtl8152b";
  inherent_cfg.route_prio = 64;

  esp_netif_config_t netif_cfg = {
      .base = &inherent_cfg,
      .driver = nullptr,
      .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH,
  };

  esp_netif_t* ecm_netif = esp_netif_new(&netif_cfg);
  ESP_RETURN_ON_FALSE(
      ecm_netif != nullptr, ESP_FAIL, TAG, "create USB ECM netif failed");
  esp_netif_set_default_netif(ecm_netif);

  iot_eth_netif_glue_handle_t glue = iot_eth_new_netif_glue(eth_handle);
  ESP_RETURN_ON_FALSE(
      glue != nullptr, ESP_FAIL, TAG, "create iot_eth netif glue failed");

  ESP_RETURN_ON_ERROR(
      esp_netif_attach(ecm_netif, glue), TAG, "attach netif failed");

  uint8_t fallback_mac[6] = {};
  memcpy(fallback_mac, RTL8152B_FALLBACK_MAC, sizeof(fallback_mac));
  ESP_RETURN_ON_ERROR(esp_netif_set_mac(ecm_netif, fallback_mac), TAG,
      "set fallback MAC failed");
  ESP_LOGI(TAG, "Use fallback MAC: %02X:%02X:%02X:%02X:%02X:%02X",
      fallback_mac[0], fallback_mac[1], fallback_mac[2], fallback_mac[3],
      fallback_mac[4], fallback_mac[5]);

  ESP_RETURN_ON_ERROR(iot_eth_start(eth_handle), TAG, "start iot_eth failed");

  ESP_LOGI(TAG, "Waiting for RTL8152B VID:0x%04X PID:0x%04X", RTL8152B_VID,
      RTL8152B_PID);
  return ESP_OK;
}

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  if (!driver.InitMinimal() || !driver.SetUsbHostPowerEnabled(true)) {
    ESP_LOGE(TAG, "USB host power initialization failed");
    return;
  }

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  s_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(s_event_group != nullptr ? ESP_OK : ESP_ERR_NO_MEM);

  ESP_ERROR_CHECK(esp_event_handler_register(
      IOT_ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(
      IP_EVENT, IP_EVENT_ETH_GOT_IP, eth_event_handler, nullptr));

  BaseType_t task_created = xTaskCreatePinnedToCore(usb_lib_task, "usb_lib",
      4096, xTaskGetCurrentTaskHandle(), 5, nullptr, 0);
  ESP_ERROR_CHECK(task_created == pdPASS ? ESP_OK : ESP_FAIL);

  uint32_t notify_value = ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1000));
  ESP_ERROR_CHECK(notify_value > 0 ? ESP_OK : ESP_ERR_TIMEOUT);

  usbh_cdc_driver_config_t cdc_config = {
      .task_stack_size = 4096,
      .task_priority = configMAX_PRIORITIES - 1,
      .task_coreid = 0,
      .skip_init_usb_host_driver = true,
  };
  ESP_ERROR_CHECK(usbh_cdc_driver_install(&cdc_config));
  ESP_ERROR_CHECK(install_rtl8152b_ecm());

  ESP_LOGI(TAG, "Connect RTL8152B Ethernet cable and wait for DHCP");
  start_iperf_console();

  xEventGroupWaitBits(
      s_event_group, EVENT_GOT_IP_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  vTaskSuspend(nullptr);
}
