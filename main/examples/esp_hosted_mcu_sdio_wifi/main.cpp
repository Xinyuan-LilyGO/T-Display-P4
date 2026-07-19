/*
 * @Description: esp_hosted_mcu_sdio_wifi
 * @Author: LILYGO_L
 * @Date: 2025-09-30 14:21:42
 * @LastEditTime: 2026-03-26 15:00:17
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "esp_hosted.h"
#include "esp_wifi_remote.h"
#include <unordered_set>

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

extern "C"
{
#include "esp_hosted_transport_config.h"
}

#define WIFI_SSID "LilyGo-AABB"
#define WIFI_PASSWORD "xinyuandianzi"

typedef enum
{
    TRANSPORT_CONFIG_STATUS_UNINITIALIZED = 0,
    TRANSPORT_CONFIG_STATUS_CONFIGURED,
    TRANSPORT_CONFIG_STATUS_ACTIVE,
    TRANSPORT_CONFIG_STATUS_ERROR
} transport_config_status_t;

typedef struct
{
    transport_config_status_t status;
    esp_hosted_transport_err_t last_error;
} transport_config_context_t;

transport_config_context_t g_transport_context =
    {
        .status = TRANSPORT_CONFIG_STATUS_UNINITIALIZED,
        .last_error = ESP_TRANSPORT_OK,
};

bool wifi_scan_success_flag = false;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

esp_err_t validate_transport_configuration(void)
{
    printf("validating transport configuration...\n");

    struct esp_hosted_transport_config *config;
    esp_hosted_transport_err_t result = esp_hosted_transport_get_config(&config);

    if (result != ESP_TRANSPORT_OK || !config)
    {
        printf("configuration validation failed: cannot retrieve config\n");
        return ESP_FAIL;
    }

    // Basic validation checks
    if (config->transport_in_use < 1 || config->transport_in_use > 4)
    {
        printf("invalid transport type: %d\n", config->transport_in_use);
        return ESP_FAIL;
    }

    printf("transport configuration validation passed\n");
    return ESP_OK;
}

void display_transport_configuration(void)
{
    printf("retrieving current transport configuration...\n");

    struct esp_hosted_transport_config *config;
    esp_hosted_transport_err_t result = esp_hosted_transport_get_config(&config);

    if (result != ESP_TRANSPORT_OK || !config)
    {
        printf("failed to retrieve transport configuration: error %d\n", result);
        return;
    }

    printf("current transport configuration:\n");
    printf("  transport type: %d\n", config->transport_in_use);

    switch (config->transport_in_use)
    {
    case 1: // sdio
        printf("  sdio configuration:\n");
        printf("    clock frequency: %lu khz\n", config->u.sdio.clock_freq_khz);
        printf("    bus width: %d bits\n", config->u.sdio.bus_width);
        printf("    queue sizes - tx: %d, rx: %d\n",
               config->u.sdio.tx_queue_size, config->u.sdio.rx_queue_size);
        break;

    case 2: // spi half-duplex
        printf("  spi half-duplex configuration:\n");
        printf("    clock frequency: %lu mhz\n", config->u.spi_hd.clk_mhz);
        printf("    data lines: %d\n", config->u.spi_hd.num_data_lines);
        printf("    checksum: %s\n",
               config->u.spi_hd.checksum_enable ? "enabled" : "disabled");
        break;

    case 3: // spi full-duplex
        printf("  spi full-duplex configuration:\n");
        printf("    clock frequency: %lu mhz\n", config->u.spi.clk_mhz);
        printf("    spi mode: %d\n", config->u.spi.mode);
        printf("    queue sizes - tx: %d, rx: %d\n",
               config->u.spi.tx_queue_size, config->u.spi.rx_queue_size);
        break;

    case 4: // uart
        printf("  uart configuration:\n");
        printf("    baud rate: %lu bps\n", config->u.uart.baud_rate);
        printf("    uart port: %d\n", config->u.uart.port);
        printf("    checksum: %s\n",
               config->u.uart.checksum_enable ? "enabled" : "disabled");
        break;

    default:
        printf("  unknown transport type: %d\n", config->transport_in_use);
        break;
    }
}

esp_err_t initialize_esp_hosted_simple(void)
{
    printf("initializing esp-hosted...\n");

    esp_err_t result = esp_hosted_init();
    if (result == ESP_OK)
    {
        printf("esp-hosted initialized successfully\n");
        g_transport_context.status = TRANSPORT_CONFIG_STATUS_ACTIVE;
    }
    else
    {
        printf("esp-hosted initialization failed: %s\n", esp_err_to_name(result));
        g_transport_context.status = TRANSPORT_CONFIG_STATUS_ERROR;
        printf("check transport configuration and slave device connection\n");
    }

    return result;
}

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        printf("wifi sta start\n");
        // esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        printf("got ip: " IPSTR "\n", IP2STR(&event->ip_info.ip));

        printf("wifi connect success\n");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("wifi connect fail\n");
        // printf("wifi disconnected, trying to reconnect...\n");
        // esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        printf("wifi scan finish\n");
        uint16_t ap_count = 0;
        std::vector<wifi_ap_record_t> ap_info;

        // get scan result count
        if (esp_wifi_scan_get_ap_num(&ap_count) != ESP_OK)
        {
            printf("failed to get ap count\n");
            return;
        }

        // limit maximum count to avoid memory issues
        if (ap_count > 64)
        {
            ap_count = 64;
        }
        ap_info.resize(ap_count);

        // get wifi hotspot info
        if (esp_wifi_scan_get_ap_records(&ap_count, ap_info.data()) != ESP_OK)
        {
            printf("failed to get ap records\n");
            return;
        }

        // clear cache
        ESP_ERROR_CHECK(esp_wifi_clear_ap_list());

        printf("scan %d wifi:\n", ap_count);

        // **sort by rssi signal strength in descending order**
        std::sort(ap_info.begin(), ap_info.end(),
                  [](const wifi_ap_record_t &a, const wifi_ap_record_t &b)
                  {
                      return a.rssi > b.rssi; // higher rssi means stronger signal
                  });

        // **remove duplicate ssid**
        std::unordered_set<std::string> seen_ssids;
        std::vector<wifi_ap_record_t> unique_ap_info;

        for (const auto &info : ap_info)
        {
            std::string ssid_str(reinterpret_cast<const char *>(info.ssid)); // convert to std::string
            if (!ssid_str.empty() && seen_ssids.find(ssid_str) == seen_ssids.end())
            {
                seen_ssids.emplace(ssid_str);
                unique_ap_info.emplace_back(info);
            }
        }

        // **iterate and print wifi hotspot info**

        printf("-------------------------------------------------------------\n");
        printf("| %-32s | %4s | %7s | %17s |\n", "ssid", "rssi", "channel", "mac address");
        printf("-------------------------------------------------------------\n");

        for (const auto &info : unique_ap_info)
        {
            const char *band = (info.primary <= 14) ? "2.4ghz" : "5ghz";
            printf("| %-32s | %4d dbm | %3d (%s) | %02x:%02x:%02x:%02x:%02x:%02x |\n", info.ssid,
                   info.rssi, info.primary, band, info.bssid[0], info.bssid[1], info.bssid[2], info.bssid[3],
                   info.bssid[4], info.bssid[5]);
        }

        printf("-------------------------------------------------------------\n");

        wifi_scan_success_flag = true;
    }
    else
    {
        printf("event %s %ld\n", event_base, event_id);
    }
}

void wifi_init_sta()
{
    // // initialize nvs
    // esp_err_t err = nvs_flash_init();
    // if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    // {
    //     // nvs partition was truncated and needs to be erased
    //     // retry nvs_flash_init
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     err = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    while (1)
    {
        esp_wifi_scan_start(NULL, true);
        vTaskDelay(pdMS_TO_TICKS(1500));

        if (wifi_scan_success_flag == true)
        {
            wifi_scan_success_flag = false;
            printf("wifi scan success\n");
            break;
        }
    }

    wifi_config_t wifi_config =
        {
            .sta =
                {
                    .ssid = WIFI_SSID,
                    .password = WIFI_PASSWORD,
                },
        };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_wifi_connect();
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
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

    // ESP32C6复位
    Xl9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));

    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("esp-hosted transport configuration example\n");
    printf("========================================\n");

    // validate configuration
    if (validate_transport_configuration() != ESP_OK)
    {
        printf("transport configuration validation failed\n");
        return;
    }

    // display current configuration
    display_transport_configuration();

    printf("========================================\n");

    // initialize esp-hosted with configured transport
    if (initialize_esp_hosted_simple() == ESP_OK)
    {
        printf("wifi and bluetooth functionality is now available\n");

        // display final configuration
        display_transport_configuration();
        printf("========================================\n");
    }
    else
    {
        printf("esp-hosted initialization failed\n");
    }

    printf("application running - transport status: %s\n",
           g_transport_context.status == TRANSPORT_CONFIG_STATUS_ACTIVE ? "active" : "error");

    wifi_init_sta();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
