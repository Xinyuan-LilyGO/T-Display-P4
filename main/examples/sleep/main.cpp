/*
 * @Description: deep_sleep
 * @Author: LILYGO_L
 * @Date: 2025-05-12 14:08:31
 * @LastEditTime: 2026-04-02 17:36:36
 * @License: GPL 3.0
 */
#include "cpp_bus_driver_library.h"
#include "lilygo_device_driver_library.h"

#include "Icm20948_WE.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "ethernet_init.h"
#include "esp_private/wifi.h"

#include "app_video.h"
#include "driver/ppa.h"
#include "esp_private/esp_cache_private.h"
#include "esp_video_init.h"

#include "esp_hosted.h"
#include "esp_wifi_remote.h"

#include "esp_sleep.h"
#include "light_sleep_example.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 44100

// #define USE_SCREEN

#define WIFI_SSID "xinyuandianzi"
#define WIFI_PASSWORD "AA15994823428"

uint8_t eth_port_cnt = 0;
esp_eth_handle_t *eth_handles;
esp_netif_t *eth_netif;

ppa_client_handle_t ppa_srm_handle = NULL;
size_t data_cache_line_size = 0;
int32_t video_cam_fd0;

int32_t fps_count;
int64_t start_time;

// IIC 1
auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Bq27220_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ27220_SDA, BQ27220_SCL, I2C_NUM_0);
auto Pcf8563_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(PCF8563_SDA, PCF8563_SCL, I2C_NUM_0);

// IIC 2
auto Sgm38121_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);
auto Aw86224_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);
auto Es8311_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);

// IIS
auto Es8311_Iis_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK,
                                                                     i2s_port_t::I2S_NUM_0, Cpp_Bus_Driver::Hardware_Iis::Data_Mode::INPUT_OUTPUT, Cpp_Bus_Driver::Hardware_Iis::Iis_Mode::STD,
                                                                     i2s_clock_src_t::I2S_CLK_SRC_APLL);

// UART
auto L76k_Uart_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Uart>(GPS_RX, GPS_TX, UART_NUM_1);

// SPI
auto Sx1262_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI3_HOST, 0);

auto Screen_Mipi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Mipi>(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_MIPI_DSI_HSYNC, SCREEN_MIPI_DSI_HBP, SCREEN_MIPI_DSI_HFP,
                                                                       SCREEN_MIPI_DSI_VSYNC, SCREEN_MIPI_DSI_VBP, SCREEN_MIPI_DSI_VFP, SCREEN_DATA_LANE_NUM,
                                                                       [](uint8_t format) -> Cpp_Bus_Driver::Hardware_Mipi::Color_Format
                                                                       {
                                                                    switch (format)
                                                                    {
                                                                    case 16:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB565;
                                                                    case 24:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB888;
                                                                    default:
                                                                        return Cpp_Bus_Driver::Hardware_Mipi::Color_Format::RGB565;
                                                                    } }(SCREEN_BITS_PER_PIXEL));

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

// IIC 1
auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Bq27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(Bq27220_Iic_Bus, BQ27220_IIC_ADDRESS);
auto Pcf8563 = std::make_unique<Cpp_Bus_Driver::Pcf8563x>(Pcf8563_Iic_Bus, PCF8563_IIC_ADDRESS);

// IIC 2
auto Sgm38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(Sgm38121_Iic_Bus, SGM38121_IIC_ADDRESS);
auto Aw86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(Aw86224_Iic_Bus, AW86224_IIC_ADDRESS);
auto Es8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(Es8311_Iic_Bus, Es8311_Iis_Bus, ES8311_IIC_ADDRESS);
auto Icm20948 = std::make_unique<ICM20948_WE>(&Wire1, ICM20948_IIC_ADDRESS);

// UART
auto L76k = std::make_unique<Cpp_Bus_Driver::L76k>(L76k_Uart_Bus, [](bool Value) -> IRAM_ATTR bool
                                                   { return Xl9535->pin_write(XL9535_GPS_WAKE_UP, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(Value)); });

// SPI
auto Sx1262 = std::make_unique<Cpp_Bus_Driver::Sx126x>(Sx1262_Spi_Bus, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262, SX1262_BUSY,
                                                       SX1262_CS);

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

#if defined CONFIG_SCREEN_TYPE_HI8561
auto Hi8561_Iic_Touch_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(HI8561_TOUCH_SDA, HI8561_TOUCH_SCL, I2C_NUM_0);

auto Hi8561_Touch = std::make_unique<Cpp_Bus_Driver::Hi8561_Touch>(Hi8561_Iic_Touch_Bus, HI8561_TOUCH_IIC_ADDRESS);

auto Screen = std::make_unique<Cpp_Bus_Driver::Hi8561>(Screen_Mipi_Bus);

#elif defined CONFIG_SCREEN_TYPE_RM69A10

auto Gt9895_Touch_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(GT9895_SDA, GT9895_SCL, I2C_NUM_0);

auto Gt9895 = std::make_unique<Cpp_Bus_Driver::Gt9895>(Gt9895_Touch_Iic_Bus, GT9895_IIC_ADDRESS, -1, GT9895_X_SCALE_FACTOR, GT9895_Y_SCALE_FACTOR);

auto Screen = std::make_unique<Cpp_Bus_Driver::Rm69a10>(Screen_Mipi_Bus);
#else
#error "no macro definition is set"
#endif

/** Event handler for Ethernet events */
void eth_event_handler(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        printf("ethernet link up\n");
        printf("ethernet hw addr %02x:%02x:%02x:%02x:%02x:%02x\n",
               mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        printf("ethernet link down\n");
        break;
    case ETHERNET_EVENT_START:
        printf("ethernet started\n");
        break;
    case ETHERNET_EVENT_STOP:
        printf("ethernet stopped\n");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    printf("ethernet get ip address\n");
    printf("~~~~~~~~~~~\n");
    printf("eth ip: %d.%d.%d.%d\n", IP2STR(&ip_info->ip));
    printf("eth mask: %d.%d.%d.%d\n", IP2STR(&ip_info->netmask));
    printf("eth gw: %d.%d.%d.%d\n", IP2STR(&ip_info->gw));
    printf("~~~~~~~~~~~\n");
}

void wifi_cleanup(void)
{
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Post disconnect/stop events to clean up netif state */
    wifi_event_sta_disconnected_t disconnect_event = {0};
    disconnect_event.reason = WIFI_REASON_ASSOC_LEAVE;
    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                   &disconnect_event, sizeof(wifi_event_sta_disconnected_t), portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_STOP, NULL, 0, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));

    esp_wifi_internal_reg_rxcb(WIFI_IF_STA, NULL);
    esp_wifi_internal_reg_rxcb(WIFI_IF_AP, NULL);

    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_wifi_deinit();
}

void Device_Sleep_Status(bool status)
{
    if (status == true)
    {
        printf("device sleep start\n");

        Icm20948->sleep(true);

        Cpp_Bus_Driver::Es8311::Power_Status ps =
            {
                .contorl =
                    {
                        .analog_circuits = false,               // 关闭模拟电路
                        .analog_bias_circuits = false,          // 关闭模拟偏置电路
                        .analog_adc_bias_circuits = false,      // 关闭模拟ADC偏置电路
                        .analog_adc_reference_circuits = false, // 关闭模拟ADC参考电路
                        .analog_dac_reference_circuit = false,  // 关闭模拟DAC参考电路
                        .internal_reference_circuits = false,   // 关闭内部参考电路
                    },
                .vmid = Cpp_Bus_Driver::Es8311::Vmid::POWER_DOWN,
            };
        Es8311->set_power_status(ps);
        Es8311->set_pga_power(false);
        Es8311->set_adc_power(false);
        Es8311->set_dac_power(false);

#if defined CONFIG_CAMERA_TYPE_SC2336
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
#elif (defined CONFIG_CAMERA_TYPE_OV2710) || (defined CONFIG_CAMERA_TYPE_OV5645)
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
#else
#error "no macro definition is set"
#endif

        wifi_cleanup();

        ESP_ERROR_CHECK(esp_hosted_deinit());

        printf("stop and deinitialize Ethernet network...\n");
        esp_eth_handle_t eth_handle = eth_handles[0];
        uint32_t reg_value = 0;
        esp_eth_phy_reg_rw_data_t read_data =
            {
                .reg_addr = 0, // Register 0
                .reg_value_p = &reg_value,
            };

        esp_err_t err = esp_eth_ioctl(eth_handle, ETH_CMD_READ_PHY_REG, &read_data);
        if (err != ESP_OK)
        {
            printf("esp_eth_ioctl fail (error code: %s)\n", esp_err_to_name(err));
        }

        printf("register 0: %#lX\n", reg_value);

        reg_value |= (1UL << 11);

        esp_eth_phy_reg_rw_data_t write_data =
            {
                .reg_addr = 0,
                .reg_value_p = &reg_value,
            };

        err = esp_eth_ioctl(eth_handle, ETH_CMD_WRITE_PHY_REG, &write_data);
        if (err != ESP_OK)
        {
            printf("esp_eth_ioctl fail (error code: %s)\n", esp_err_to_name(err));
        }

        esp_eth_stop(eth_handles[0]);
        // esp_eth_del_netif_glue(eth_netif_glues[0]);
        esp_netif_destroy(eth_netif);
        ethernet_deinit_all(eth_handles);

        Sx1262->set_sleep();

#if defined USE_SCREEN
        printf("esp_lcd_panel_disp_off\n");

        Screen->set_screen_off(true);
        Screen->set_sleep(true);

#if defined CONFIG_SCREEN_TYPE_HI8561

#elif defined CONFIG_SCREEN_TYPE_RM69A10
        Xl9535->pin_mode(XL9535_TOUCH_INT, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
        Xl9535->pin_write(XL9535_TOUCH_INT, Cpp_Bus_Driver::Xl95x5::Value::LOW);

        Gt9895->set_sleep();
#else
#error "no macro definition is set"
#endif
#endif

        Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

        Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    }
    else
    {
        printf("device sleep close\n");
    }
}

void ES8311_Init(void)
{
    Es8311->begin(MCLK_MULTIPLE, SAMPLE_RATE, i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT);

    if (Es8311->begin(50000) == true)
    {
        printf("es8311 initialization success\n");
    }
    else
    {
        printf("es8311 initialization fail\n");
    }

    Es8311->set_master_clock_source(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK);
    Es8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK, true);
    Es8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_BCLK, true);

    Es8311->set_clock_coeff(MCLK_MULTIPLE, SAMPLE_RATE);

    Es8311->set_serial_port_mode(Cpp_Bus_Driver::Es8311::Serial_Port_Mode::SLAVE);

    Es8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::ADC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
    Es8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::DAC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
    Cpp_Bus_Driver::Es8311::Power_Status ps =
        {
            .contorl =
                {
                    .analog_circuits = true,               // 开启模拟电路
                    .analog_bias_circuits = true,          // 开启模拟偏置电路
                    .analog_adc_bias_circuits = true,      // 开启模拟ADC偏置电路
                    .analog_adc_reference_circuits = true, // 开启模拟ADC参考电路
                    .analog_dac_reference_circuit = true,  // 开启模拟DAC参考电路
                    .internal_reference_circuits = false,  // 关闭内部参考电路
                },
            .vmid = Cpp_Bus_Driver::Es8311::Vmid::START_UP_VMID_NORMAL_SPEED_CHARGE,
        };
    Es8311->set_power_status(ps);
    Es8311->set_pga_power(true);
    Es8311->set_adc_power(true);
    Es8311->set_dac_power(true);
    Es8311->set_output_to_hp_drive(true);
    Es8311->set_adc_offset_freeze(Cpp_Bus_Driver::Es8311::Adc_Offset_Freeze::DYNAMIC_HPF);
    Es8311->set_adc_hpf_stage2_coeff(10);
    Es8311->set_dac_equalizer(false);

    Es8311->set_mic(Cpp_Bus_Driver::Es8311::Mic_Type::ANALOG_MIC, Cpp_Bus_Driver::Es8311::Mic_Input::MIC1P_1N);
    Es8311->set_adc_auto_volume_control(false);
    Es8311->set_adc_gain(Cpp_Bus_Driver::Es8311::Adc_Gain::GAIN_18DB);
    Es8311->set_adc_pga_gain(Cpp_Bus_Driver::Es8311::Adc_Pga_Gain::GAIN_30DB);

    Es8311->set_adc_volume(191);
    Es8311->set_dac_volume(210);

    // 将ADC的数据自动输出到DAC上
    // Es8311->set_adc_data_to_dac(true);
}

bool ICM20948_Init(void)
{
    Wire1.begin(ICM20948_SDA, ICM20948_SCL);
    if (Icm20948->init() == false)
    {
        printf("ICM20948 AG initialization failed\n");
        return false;
    }

    if (Icm20948->initMagnetometer() == false)
    {
        printf("ICM20948 M initialization failed\n");
        return false;
    }

    printf("Position your ICM20948 flat and don't move it - calibrating...\n");
    Icm20948->autoOffsets();
    printf("Done!\n");

    Icm20948->setAccRange(ICM20948_ACC_RANGE_2G);
    Icm20948->setAccDLPF(ICM20948_DLPF_6);
    Icm20948->setMagOpMode(AK09916_CONT_MODE_20HZ);

    return true;
}

void Ethernet_Init(void)
{
    char if_key_str[10];
    char if_desc_str[10];
    esp_netif_config_t cfg;
    esp_netif_inherent_config_t eth_netif_cfg;

    ethernet_init_all(&eth_handles, &eth_port_cnt);

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

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
        eth_netif = esp_netif_new(&cfg);
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
        esp_eth_start(eth_handles[i]);
    }
}

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        printf("wifi started\n");

        // esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("wifi disconnected / connect failed, retrying...\n");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ip_str[64];
        sprintf(ip_str, "wifi ip:" IPSTR ":", IP2STR(&event->ip_info.ip));
        printf("got ip: %s\n", ip_str);
    }
}

bool Wifi_Init_Connect(void)
{
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    // {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config =
        {
            .sta =
                {
                    .ssid = WIFI_SSID,
                    .password = WIFI_PASSWORD,
                },
        };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("connecting to wifi: %s\n", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Wait for connection
    for (int i = 0; i < 10; i++)
    {
        wifi_ap_record_t ap_info;
        // Check connection status
        if ((esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) && (ap_info.ssid[0] != 0))
        {
            printf("wifi connected\n");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    return false;
}

void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len)
{
    fps_count++;
    if (fps_count == 50)
    {
        int64_t end_time = esp_timer_get_time();
        printf("fps: %f\n", 1000000.0 / ((end_time - start_time) / 50.0));
        start_time = end_time;
        fps_count = 0;

        printf("camera_buf_hes: %lu, camera_buf_ves: %lu, camera_buf_len: %d KB\n", camera_buf_hes, camera_buf_ves, camera_buf_len / 1024);
    }
}

bool App_Video_Init(void)
{
    ppa_client_config_t ppa_srm_config =
        {
            .oper_type = PPA_OPERATION_SRM,
        };
    esp_err_t assert = ppa_register_client(&ppa_srm_config, &ppa_srm_handle);
    if (assert != ESP_OK)
    {
        printf("ppa_register_client fail (error code: %#X)\n", assert);
        return false;
    }
    assert = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &data_cache_line_size);
    if (assert != ESP_OK)
    {
        printf("esp_cache_get_alignment fail (error code: %#X)\n", assert);
        return false;
    }

    esp_video_init_csi_config_t csi_config =
        {
            .sccb_config = {
                .init_sccb = false,
                .i2c_handle = Sgm38121_Iic_Bus->get_bus_handle(),
                .freq = static_cast<uint32_t>(100000),
            },
            .reset_pin = gpio_num_t ::GPIO_NUM_NC,
            .pwdn_pin = gpio_num_t ::GPIO_NUM_NC,

            .dont_init_ldo = true,
        };

    esp_video_init_config_t cam_config =
        {
            .csi = &csi_config,
        };

    assert = esp_video_init(&cam_config);
    if (assert != ESP_OK)
    {
        printf("esp_video_init fail (error code: %#X)\n", assert);
        return false;
    }

#if (defined CONFIG_CAMERA_TYPE_SC2336) || (defined CONFIG_CAMERA_TYPE_OV2710)
#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB565);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB888);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#else
#error "no macro definition is set"
#endif
#elif defined CONFIG_CAMERA_TYPE_OV5645
    video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, video_fmt_t::APP_VIDEO_FMT_RGB565);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }
#else
#error "no macro definition is set"
#endif

    assert = app_video_set_bufs(video_cam_fd0, CAMERA_BUFFER_COUNT, NULL);
    if (assert != ESP_OK)
    {
        printf("app_video_set_bufs fail (error code: %#X)\n", assert);
        return false;
    }

    assert = app_video_register_frame_operation_cb(camera_video_frame_operation);
    if (assert != ESP_OK)
    {

        printf("app_video_register_frame_operation_cb fail (error code: %#X)\n", assert);
        return false;
    }

    assert = app_video_stream_task_start(video_cam_fd0, 0);
    if (assert != ESP_OK)
    {

        printf("app_video_stream_task_start fail (error code: %#X)\n", assert);
        return false;
    }

    // app_video_stream_task_stop(video_cam_fd0);

    // // Get the initial time for frame rate statistics
    // start_time = esp_timer_get_time();

    return true;
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

#if defined USE_SCREEN
    Xl9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
#endif
    Xl9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_EXTERNAL_SENSOR_INT, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

#if defined USE_SCREEN
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
#endif
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    Esp32p4->pin_mode(ETHERNET_MDC, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
    Esp32p4->pin_mode(ETHERNET_MDIO, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));

    if (Sgm38121->begin() == false)
    {
        printf("sgm38121 init fail\n");
    }

#if defined CONFIG_CAMERA_TYPE_SC2336
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV2710
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1700);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3000);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_TYPE_OV5645
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    Sgm38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    Sgm38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "no macro definition is set"
#endif

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(3, 2500);

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(4, 3300);

#if defined USE_SCREEN
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
#endif
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
#if defined USE_SCREEN
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
#endif
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
#if defined USE_SCREEN
    Xl9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
#endif
    Xl9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    Ethernet_Init();

#if defined USE_SCREEN
#if defined CONFIG_SCREEN_TYPE_HI8561
    // 这个必须放在以太网后面
    Screen->create_pwm(HI8561_SCREEN_BL, ledc_timer_t::LEDC_TIMER_0, ledc_channel_t::LEDC_CHANNEL_0, 2000);

#elif defined CONFIG_SCREEN_TYPE_RM69A10
#else
#error "no macro definition is set"
#endif
#endif

    if (App_Video_Init() == false)
    {
        printf("App_Video_Init fail\n");
    }

#if defined USE_SCREEN

    Screen->begin(SCREEN_MIPI_DSI_DPI_CLK_MHZ, SCREEN_LANE_BIT_RATE_MBPS);

#if defined CONFIG_SCREEN_TYPE_HI8561
    Hi8561_Iic_Touch_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    Hi8561_Touch->begin();

#elif defined CONFIG_SCREEN_TYPE_RM69A10

    Gt9895_Touch_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());

    Gt9895->begin();

#else
#error "no macro definition is set"
#endif

#endif
    if (Wifi_Init_Connect() == false)
    {
        printf("Wifi_Init_Connect fail\n");
    }

    if (Lilygo_Device_Driver::Sdmmc_Init(SD_BASE_PATH) == false)
    {
        printf("Sdmmc_Init fail\n");
    }

    Pcf8563_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());
    Pcf8563->begin();

    Bq27220_Iic_Bus->set_bus_handle(Xl9535_Iic_Bus->get_bus_handle());
    Bq27220->begin();

    // 设置的电池容量会在没有电池插入的时候自动还原为默认值
    Bq27220->set_design_capacity(1000);
    Bq27220->set_temperature_mode(Cpp_Bus_Driver::Bq27220xxxx::Temperature_Mode::EXTERNAL_NTC);
    Bq27220->set_sleep_current_threshold(50);

    Aw86224_Iic_Bus->set_bus_handle(Sgm38121_Iic_Bus->get_bus_handle());
    Aw86224->begin(500000);
    // printf("AW86224 input voltage: %.06f V\n", Aw86224->get_input_voltage());

    // RAM播放
    Aw86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    Es8311_Iic_Bus->set_bus_handle(Sgm38121_Iic_Bus->get_bus_handle());
    ES8311_Init();

    Wire1._bus->set_bus_handle(Sgm38121_Iic_Bus->get_bus_handle());
    ICM20948_Init();

    L76k->begin();
    printf("get_baud_rate:%ld\n", L76k->get_baud_rate());
    L76k->set_baud_rate(Cpp_Bus_Driver::L76k::Baud_Rate::BR_115200_BPS);
    printf("set_baud_rate:%ld\n", L76k->get_baud_rate());
    L76k->set_update_frequency(Cpp_Bus_Driver::L76k::Update_Freq::FREQ_5HZ);
    L76k->clear_rx_buffer_data();

    Xl9535->pin_mode(XL9535_SX1262_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    // LORA复位
    Xl9535->pin_mode(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    Xl9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    Sx1262->begin(10000000);

#if defined USE_SCREEN

#if defined CONFIG_SCREEN_PIXEL_FORMAT_RGB565
    // 设置整个屏幕为白色
    size_t screen_size = SCREEN_WIDTH * SCREEN_HEIGHT * 2; // RGB565: 2 bytes per pixel
    size_t data_cache_line_size = 16;                      // 通常16或32，具体可查芯片手册
    void *color_buf = heap_caps_aligned_calloc(data_cache_line_size, 1, screen_size, MALLOC_CAP_SPIRAM);
    if (color_buf)
    {
        uint16_t *p = (uint16_t *)color_buf;
        for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i)
        {
            p[i] = 0xFFFF; // RGB565白色
        }
        if (Screen->send_color_stream_coordinate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color_buf) == false)
        {
            printf("send_color_stream_coordinate fail\n");
        }
        heap_caps_free(color_buf);
    }
#elif defined CONFIG_SCREEN_PIXEL_FORMAT_RGB888

    // 设置整个屏幕为白色
    size_t screen_size = SCREEN_WIDTH * SCREEN_HEIGHT * 3; // RGB888: 3 bytes per pixel
    size_t data_cache_line_size = 16;                      // 通常16或32，具体可查芯片手册
    void *color_buf = heap_caps_aligned_calloc(data_cache_line_size, 1, screen_size, MALLOC_CAP_SPIRAM);
    if (color_buf)
    {
        uint8_t *p = (uint8_t *)color_buf;
        for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i)
        {
            p[i * 3 + 0] = 0xFF; // R
            p[i * 3 + 1] = 0xFF; // G
            p[i * 3 + 2] = 0xFF; // B
        }
        if (Screen->send_color_stream_coordinate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color_buf) == false)
        {
            printf("send_color_stream_coordinate fail\n");
        }
        heap_caps_free(color_buf);
    }

#else
#error "no macro definition is set"
#endif

#if defined CONFIG_SCREEN_TYPE_HI8561
    Esp32p4->start_pwm_gradient_time(100, 500);
#elif defined CONFIG_SCREEN_TYPE_RM69A10
    for (uint8_t i = 0; i < 255; i += 5)
    {
        Screen->set_brightness(i);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#else
#error "no macro definition is set"
#endif

#endif

    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Enable wakeup from light sleep by gpio */
    // example_register_gpio_wakeup();
    //     /* Enable wakeup from light sleep by timer */
    // example_register_timer_wakeup();
    //     /* Enable wakeup from light sleep by uart */
    //     example_register_uart_wakeup();
    // #if TOUCH_LSLEEP_SUPPORTED
    //     /* Enable wakeup from light sleep by touch element */
    //     example_register_touch_wakeup();
    // #endif

    while (1)
    {
        Device_Sleep_Status(true);

        printf("entering sleep\n");
        /* To make sure the complete line is printed before entering sleep mode,
         * need to wait until UART TX FIFO is empty:
         */
        uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);

        /* Get timestamp before entering sleep */
        int64_t t_before_us = esp_timer_get_time();

        esp_deep_sleep_start();

        /* Get timestamp after waking up from sleep */
        int64_t t_after_us = esp_timer_get_time();

        /* Determine wake up reason */
        const char *wakeup_reason;
        switch (esp_sleep_get_wakeup_cause())
        {
        case ESP_SLEEP_WAKEUP_TIMER:
            wakeup_reason = "timer";
            break;
        case ESP_SLEEP_WAKEUP_GPIO:
            wakeup_reason = "pin";
            break;
        case ESP_SLEEP_WAKEUP_UART:
            wakeup_reason = "uart";
            /* Hang-up for a while to switch and execute the uart task
             * Otherwise the chip may fall sleep again before running uart task */
            vTaskDelay(1);
            break;
#if TOUCH_LSLEEP_SUPPORTED
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            wakeup_reason = "touch";
            break;
#endif
        default:
            wakeup_reason = "other";
            break;
        }
#if CONFIG_NEWLIB_NANO_FORMAT
        /* printf in newlib-nano does not support %ll format, causing example test fail */
        printf("Returned from light sleep, reason: %s, t=%d ms, slept for %d ms\n",
               wakeup_reason, (int)(t_after_us / 1000), (int)((t_after_us - t_before_us) / 1000));
#else
        printf("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
               wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
#endif
        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO)
        {
            /* Waiting for the gpio inactive, or the chip will continuously trigger wakeup*/
            example_wait_gpio_inactive();
        }
    }
}
