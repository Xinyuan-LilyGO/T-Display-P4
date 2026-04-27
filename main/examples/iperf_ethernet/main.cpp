/*
 * @Description: iperf_ethernet
 * @Author: LILYGO_L
 * @Date: 2026-01-26 14:42:15
 * @LastEditTime: 2026-04-24 14:09:19
 * @License: GPL 3.0
 */
#include <stdatomic.h>

#include "esp_console.h"
#include "esp_eth.h"
#include "esp_eth_phy_802_3.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "ethernet_init.h"
#include "iperf_cmd.h"
#include "lilygo_device_driver_library.h"

#define CONFIG_EXAMPLE_ACT_AS_DHCP_SERVER 0

static const char* TAG = "iperf_example";

#if CONFIG_EXAMPLE_ACT_AS_DHCP_SERVER
static void start_dhcp_server_after_connection(void* arg, esp_event_base_t base,
                                               int32_t id, void* event_data) {
  esp_netif_t* eth_netif = esp_netif_next_unsafe(NULL);
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t*)event_data;
  while (eth_netif != NULL) {
    esp_eth_handle_t eth_handle_for_current_netif =
        esp_netif_get_io_driver(eth_netif);
    if (memcmp(&eth_handle, &eth_handle_for_current_netif,
               sizeof(esp_eth_handle_t)) == 0) {
      esp_netif_dhcpc_stop(eth_netif);
      esp_netif_dhcps_start(eth_netif);
    }
    eth_netif = esp_netif_next_unsafe(eth_netif);
  }
}
#endif

extern "C" void app_main(void) {
  printf("Ciallo\n");

  lilygo_device_driver::TDisplayP4Driver::GetInstance().Init();

  // auto esp32p4 = std::make_unique<cpp_bus_driver::Tool>();

  uint8_t eth_port_cnt = 0;
  char if_key_str[10];
  char if_desc_str[10];
  esp_eth_handle_t* eth_handles;
  esp_netif_config_t cfg;
  esp_netif_inherent_config_t eth_netif_cfg;
  esp_netif_init();
  esp_event_loop_create_default();
  ethernet_init_all(&eth_handles, &eth_port_cnt);

#if CONFIG_EXAMPLE_ACT_AS_DHCP_SERVER
  esp_netif_ip_info_t* ip_infos;

  ip_infos =
      (esp_netif_ip_info_t*)calloc(eth_port_cnt, sizeof(esp_netif_ip_info_t));

  eth_netif_cfg =
      (esp_netif_inherent_config_t){.flags = ESP_NETIF_DHCP_SERVER,
                                    .get_ip_event = IP_EVENT_ETH_GOT_IP,
                                    .lost_ip_event = 0,
                                    .route_prio = 50};
  cfg = (esp_netif_config_t){.base = &eth_netif_cfg,
                             .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};

  for (uint8_t i = 0; i < eth_port_cnt; i++) {
    sprintf(if_key_str, "ETH_S%d", i);
    sprintf(if_desc_str, "eth%d", i);

    esp_netif_ip_info_t ip_info_i = {
        .ip = {.addr = ESP_IP4TOADDR(192, 168, i, 1)},
        .netmask = {.addr = ESP_IP4TOADDR(255, 255, 255, 0)},
        .gw = {.addr = ESP_IP4TOADDR(192, 168, i, 1)}};
    ip_infos[i] = ip_info_i;

    eth_netif_cfg.if_key = if_key_str;
    eth_netif_cfg.if_desc = if_desc_str;
    eth_netif_cfg.route_prio -= i * 5;
    eth_netif_cfg.ip_info = &(ip_infos[i]);
    esp_netif_t* eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(
        esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
  }
  esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_CONNECTED,
                             start_dhcp_server_after_connection, NULL);
  ESP_LOGI(TAG, "--------");
  for (uint8_t i = 0; i < eth_port_cnt; i++) {
    esp_eth_start(eth_handles[i]);
    ESP_LOGI(TAG, "Network Interface %d: " IPSTR, i, IP2STR(&ip_infos[i].ip));
  }
  ESP_LOGI(TAG, "--------");
#else
  if (eth_port_cnt == 1) {
    // Use default config when using one interface
    eth_netif_cfg = *(ESP_NETIF_BASE_DEFAULT_ETH);
  } else {
    // Set config to support multiple interfaces
    eth_netif_cfg =
        (esp_netif_inherent_config_t)ESP_NETIF_INHERENT_DEFAULT_ETH();
  }
  cfg = (esp_netif_config_t){.base = &eth_netif_cfg,
                             .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
  for (int i = 0; i < eth_port_cnt; i++) {
    sprintf(if_key_str, "ETH_%d", i);
    sprintf(if_desc_str, "eth%d", i);
    eth_netif_cfg.if_key = if_key_str;
    eth_netif_cfg.if_desc = if_desc_str;
    eth_netif_cfg.route_prio -= i * 5;
    esp_netif_t* eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(
        esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
    esp_eth_start(eth_handles[i]);
  }
#endif
  esp_console_repl_t* repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  esp_console_dev_uart_config_t uart_config =
      ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  esp_console_new_repl_uart(&uart_config, &repl_config, &repl);
  app_register_iperf_commands();

  printf("\n ========================================================\n");
  printf(" |                    Ethernet iperf                    |\n");
  printf(" |                                                      |\n");
  printf(" | Type 'help' to display a list of available commands. |\n");
  printf(" |                                                      |\n");
  printf(" ========================================================\n");

  esp_console_start_repl(repl);

  // esp32p4->SetPinMode(ETHERNET_MDC, cpp_bus_driver::Tool::PinMode::kInput,
  //                     cpp_bus_driver::Tool::PinStatus::kPulldown);
  // esp32p4->SetPinMode(ETHERNET_MDIO, cpp_bus_driver::Tool::PinMode::kInput,
  //                     cpp_bus_driver::Tool::PinStatus::kPulldown);

  // vTaskDelay(pdMS_TO_TICKS(5000));

  // // 断电测试
  // esp_eth_handle_t eth_handle = eth_handles[0];
  // uint32_t reg_value = 0;
  // esp_eth_phy_reg_rw_data_t read_data = {
  //     .reg_addr = 0,  // Register 0
  //     .reg_value_p = &reg_value,
  // };

  // esp_err_t err = esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG,
  // &read_data); if (err != ESP_OK) {
  //   printf("esp_eth_ioctl fail (error code: %s)\n", esp_err_to_name(err));
  // }

  // printf("Register 0: %#lX\n", reg_value);

  // reg_value |= (1UL << 11);

  // esp_eth_phy_reg_rw_data_t write_data = {
  //     .reg_addr = 0,
  //     .reg_value_p = &reg_value,
  // };

  // err = esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &write_data);
  // if (err != ESP_OK) {
  //   printf("esp_eth_ioctl fail (error code: %s)\n", esp_err_to_name(err));
  // }

  // printf("Ip101g off\n");

  // vTaskDelay(pdMS_TO_TICKS(5000));

  // // 唤醒
  // reg_value = 0;
  // read_data = {
  //     .reg_addr = 0,
  //     .reg_value_p = &reg_value,
  // };
  // esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &read_data);
  // printf("Register 0: %#lX\n", reg_value);

  // reg_value &= ~(1UL << 11);

  // write_data = {
  //     .reg_addr = 0,
  //     .reg_value_p = &reg_value,
  // };
  // esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &write_data);

  // printf("Ip101g on\n");
}
