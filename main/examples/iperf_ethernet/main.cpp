/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-01-26 14:42:15
 * @LastEditTime: 2026-03-26 15:00:46
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_console.h"
#include "esp_log.h"
#include "ethernet_init.h"
#include "iperf_cmd.h"
#include "esp_eth_phy_802_3.h"
#include <stdatomic.h>

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
#include "kode_bq25896.h"
#endif

typedef enum
{
    ESP_ETH_FSM_STOP,
    ESP_ETH_FSM_START
} esp_eth_fsm_t;

typedef struct
{
    esp_eth_mediator_t mediator;
    esp_eth_phy_t *phy;
    esp_eth_mac_t *mac;
    esp_timer_handle_t check_link_timer;
    uint32_t check_link_period_ms;
    bool auto_nego_en;
    eth_speed_t speed;
    eth_duplex_t duplex;
    std::atomic<eth_link_t> link;
    atomic_int ref_count;
    void *priv;
    std::atomic<esp_eth_fsm_t> fsm;
#if CONFIG_ETH_TRANSMIT_MUTEX
    SemaphoreHandle_t transmit_mutex;
#endif // CONFIG_ETH_TRANSMIT_MUTEX
    esp_err_t (*stack_input)(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t length, void *priv);
    esp_err_t (*stack_input_info)(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t length, void *priv, void *info);
    esp_err_t (*on_lowlevel_init_done)(esp_eth_handle_t eth_handle);
    esp_err_t (*on_lowlevel_deinit_done)(esp_eth_handle_t eth_handle);
    esp_err_t (*customized_read_phy_reg)(esp_eth_handle_t eth_handle, uint32_t phy_addr, uint32_t phy_reg, uint32_t *reg_value);
    esp_err_t (*customized_write_phy_reg)(esp_eth_handle_t eth_handle, uint32_t phy_addr, uint32_t phy_reg, uint32_t reg_value);
} esp_eth_driver_t;

#define CONFIG_EXAMPLE_ACT_AS_DHCP_SERVER 0

static const char *TAG = "iperf_example";

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

#if CONFIG_EXAMPLE_ACT_AS_DHCP_SERVER
static void start_dhcp_server_after_connection(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_netif_t *eth_netif = esp_netif_next_unsafe(NULL);
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    while (eth_netif != NULL)
    {
        esp_eth_handle_t eth_handle_for_current_netif = esp_netif_get_io_driver(eth_netif);
        if (memcmp(&eth_handle, &eth_handle_for_current_netif, sizeof(esp_eth_handle_t)) == 0)
        {
            esp_netif_dhcpc_stop(eth_netif);
            esp_netif_dhcps_start(eth_netif);
        }
        eth_netif = esp_netif_next_unsafe(eth_netif);
    }
}
#endif

extern "C" void app_main(void)
{
    printf("Ciallo\n");

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
    int16_t assert = Kode_Bq25896::bq25896_init(Bq25896_Iic_Bus, Bq25896_Handle);
    if (assert != ESP_OK)
    {
        printf("bq25896 init fail (error code: %#X)\n", assert);
    }
    else
    {
        printf("bq25896 init success\n");

        Kode_Bq25896::bq25896_set_input_current_limit(Bq25896_Handle, Kode_Bq25896::bq25896_ilim_t ::BQ25896_ILIM_2000MA);
        // 禁用看门狗后不能读取看门狗寄存器状态，否者看门狗禁用会失效
        Kode_Bq25896::bq25896_set_watchdog_timer(Bq25896_Handle, Kode_Bq25896::bq25896_watchdog_t::BQ25896_WATCHDOG_DISABLE);
        // Kode_Bq25896::bq25896_set_adc_conversion(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_state_t::BQ25896_ADC_CONV_START);
        // Kode_Bq25896::bq25896_set_adc_conversion_rate(Bq25896_Handle, Kode_Bq25896::bq25896_adc_conv_rate_t ::BQ25896_ADC_CONV_RATE_CONTINUOUS);
        Kode_Bq25896::bq25896_set_charge_current(Bq25896_Handle, Kode_Bq25896::bq25896_ichg_t::BQ25896_ICHG_512MA);
        // Kode_Bq25896::bq25896_set_otg(Bq25896_Handle, Kode_Bq25896::bq25896_otg_state_t::BQ25896_OTG_ENABLE);
    }

    Xl9535_Iic_Bus->set_bus_handle(Bq25896_Iic_Bus->get_bus_handle());
#endif

    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_mode(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t eth_port_cnt = 0;
    char if_key_str[10];
    char if_desc_str[10];
    esp_eth_handle_t *eth_handles;
    esp_netif_config_t cfg;
    esp_netif_inherent_config_t eth_netif_cfg;
    esp_netif_init();
    esp_event_loop_create_default();
    ethernet_init_all(&eth_handles, &eth_port_cnt);

#if CONFIG_EXAMPLE_ACT_AS_DHCP_SERVER
    esp_netif_ip_info_t *ip_infos;

    ip_infos = (esp_netif_ip_info_t *)calloc(eth_port_cnt, sizeof(esp_netif_ip_info_t));

    eth_netif_cfg = (esp_netif_inherent_config_t){
        .flags = ESP_NETIF_DHCP_SERVER,
        .get_ip_event = IP_EVENT_ETH_GOT_IP,
        .lost_ip_event = 0,
        .route_prio = 50};
    cfg = (esp_netif_config_t){
        .base = &eth_netif_cfg,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};

    for (uint8_t i = 0; i < eth_port_cnt; i++)
    {
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
        esp_netif_t *eth_netif = esp_netif_new(&cfg);
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
    }
    esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_CONNECTED, start_dhcp_server_after_connection, NULL);
    ESP_LOGI(TAG, "--------");
    for (uint8_t i = 0; i < eth_port_cnt; i++)
    {
        esp_eth_start(eth_handles[i]);
        ESP_LOGI(TAG, "Network Interface %d: " IPSTR, i, IP2STR(&ip_infos[i].ip));
    }
    ESP_LOGI(TAG, "--------");
#else
    if (eth_port_cnt == 1)
    {
        // Use default config when using one interface
        eth_netif_cfg = *(ESP_NETIF_BASE_DEFAULT_ETH);
    }
    else
    {
        // Set config to support multiple interfaces
        eth_netif_cfg = (esp_netif_inherent_config_t)ESP_NETIF_INHERENT_DEFAULT_ETH();
    }
    cfg = (esp_netif_config_t){
        .base = &eth_netif_cfg,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
    for (int i = 0; i < eth_port_cnt; i++)
    {
        sprintf(if_key_str, "ETH_%d", i);
        sprintf(if_desc_str, "eth%d", i);
        eth_netif_cfg.if_key = if_key_str;
        eth_netif_cfg.if_desc = if_desc_str;
        eth_netif_cfg.route_prio -= i * 5;
        esp_netif_t *eth_netif = esp_netif_new(&cfg);
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
        esp_eth_start(eth_handles[i]);
    }
#endif
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_console_new_repl_uart(&uart_config, &repl_config, &repl);
    app_register_iperf_commands();

    printf("\n ========================================================\n");
    printf(" |                    Ethernet iperf                    |\n");
    printf(" |                                                      |\n");
    printf(" | Type 'help' to display a list of available commands. |\n");
    printf(" |                                                      |\n");
    printf(" ========================================================\n");

    esp_console_start_repl(repl);

    // Esp32p4->pin_mode(ETHERNET_MDC, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
    // Esp32p4->pin_mode(ETHERNET_MDIO, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

    // 睡眠测试
    // esp_eth_driver_t *eth_driver = (esp_eth_driver_t *)eth_handles[0];
    // phy_802_3_t *phy_802_3 = esp_eth_phy_into_phy_802_3(eth_driver->phy);

    // uint32_t buffer = 0;
    // esp_eth_phy_802_3_read_mmd_register(phy_802_3, 0x1F, 0x0080, &buffer);
    // printf("buffer: %#lX\n", buffer);
    // buffer |= 0B1100000000000000;
    // esp_eth_phy_802_3_write_mmd_register(phy_802_3, 0x1F, 0x0080, buffer);
}
