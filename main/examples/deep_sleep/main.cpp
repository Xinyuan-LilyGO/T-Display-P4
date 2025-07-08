/*
 * @Description: deep_sleep
 * @Author: LILYGO_L
 * @Date: 2025-05-12 14:08:31
 * @LastEditTime: 2025-07-05 16:58:04
 * @License: GPL 3.0
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "light_sleep_example.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "ICM20948_WE.h"
#include "driver/rtc_io.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "ethernet_init.h"
#include "esp_event.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "hi8561_driver.h"
#include "esp_lcd_panel_io.h"
#include "app_video.h"
#include "driver/ppa.h"
#include "esp_private/esp_cache_private.h"

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 44100

uint8_t eth_port_cnt = 0;
esp_eth_handle_t *eth_handles;
esp_netif_t *eth_netifs[10];
esp_eth_netif_glue_handle_t eth_netif_glues[10];

esp_lcd_panel_handle_t Screen_Mipi_Dpi_Panel = NULL;

ppa_client_handle_t ppa_srm_handle = NULL;
size_t data_cache_line_size = 0;
void *lcd_buffer[CONFIG_EXAMPLE_CAM_BUF_COUNT];
int32_t fps_count;
int64_t start_time;
int32_t video_cam_fd0;

// IIC 1
auto XL9535_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto HI8561_T_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(HI8561_TOUCH_SDA, HI8561_TOUCH_SCL, I2C_NUM_0);
auto BQ27220_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ27220_SDA, BQ27220_SCL, I2C_NUM_0);
auto PCF8563_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(PCF8563_SDA, PCF8563_SCL, I2C_NUM_0);

// IIC 2
auto SGM38121_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(SGM38121_SDA, SGM38121_SCL, I2C_NUM_1);
auto AW86224_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(AW86224_SDA, AW86224_SCL, I2C_NUM_1);
auto ES8311_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);
auto ICM20948_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ICM20948_SDA, ICM20948_SCL, I2C_NUM_1);

// IIS
auto ES8311_IIS_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK, I2S_NUM_0);

// UART
auto L76K_Uart_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Uart>(GPS_RX, GPS_TX, UART_NUM_1);

// SDIO
auto ESP32C6_AT_SDIO_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Sdio>(ESP32C6_SDIO_CLK, ESP32C6_SDIO_CMD,
                                                                           ESP32C6_SDIO_D0, ESP32C6_SDIO_D1, ESP32C6_SDIO_D2, ESP32C6_SDIO_D3, DEFAULT_CPP_BUS_DRIVER_VALUE,
                                                                           DEFAULT_CPP_BUS_DRIVER_VALUE, DEFAULT_CPP_BUS_DRIVER_VALUE, DEFAULT_CPP_BUS_DRIVER_VALUE,
                                                                           Cpp_Bus_Driver::Hardware_Sdio::Sdio_Port::SLOT_1);

// SPI
auto SX1262_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(LORA_MOSI, LORA_SCLK, LORA_MISO, SPI3_HOST, 0);

// IIC 1
auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(XL9535_IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto HI8561_T = std::make_unique<Cpp_Bus_Driver::Hi8561_Touch>(HI8561_T_IIC_Bus, HI8561_TOUCH_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto BQ27220 = std::make_unique<Cpp_Bus_Driver::Bq27220xxxx>(BQ27220_IIC_Bus, BQ27220_IIC_ADDRESS);
auto PCF8563 = std::make_unique<Cpp_Bus_Driver::Pcf8563x>(PCF8563_IIC_Bus, PCF8563_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

// IIC 2
auto SGM38121 = std::make_unique<Cpp_Bus_Driver::Sgm38121>(SGM38121_IIC_Bus, SGM38121_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto AW86224 = std::make_unique<Cpp_Bus_Driver::Aw862xx>(AW86224_IIC_Bus, AW86224_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ES8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(ES8311_IIC_Bus, ES8311_IIS_Bus, ES8311_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
auto ICM20948 = std::make_unique<ICM20948_WE>(ICM20948_IIC_Bus, ICM20948_IIC_ADDRESS);

// UART
auto L76K = std::make_unique<Cpp_Bus_Driver::L76k>(L76K_Uart_Bus, [](bool Value) -> IRAM_ATTR bool
                                                   { return XL9535->pin_write(XL9535_GPS_WAKE_UP, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(Value)); }, DEFAULT_CPP_BUS_DRIVER_VALUE);

// SDIO
auto ESP32C6_AT = std::make_unique<Cpp_Bus_Driver::Esp_At>(ESP32C6_AT_SDIO_Bus,
                                                           [](bool value) -> IRAM_ATTR void
                                                           {
                                                               // ESP32C6复位
                                                               XL9535->pin_write(XL9535_ESP32C6_EN, static_cast<Cpp_Bus_Driver::Xl95x5::Value>(value));
                                                           });

// SPI
auto SX1262 = std::make_unique<Cpp_Bus_Driver::Sx126x>(SX1262_SPI_Bus, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262, LORA_BUSY,
                                                       LORA_CS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

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

void Device_Sleep_Status(bool status)
{
    if (status == true)
    {
        printf("device sleep start\n");

        ICM20948->sleep(true);

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
        ES8311->set_power_status(ps);
        ES8311->set_pga_power(false);
        ES8311->set_adc_power(false);
        ES8311->set_dac_power(false);

        ES8311->software_reset(true);

        if (ESP32C6_AT->set_deep_sleep(100000) == false)
        {
            printf("esp32c6-at failed to enter deep sleep mode\n");
        }
        else
        {
            printf("esp32c6-at successfully entered deep sleep mode\n");
        }

        // if (ESP32C6_AT->set_sleep(Cpp_Bus_Driver::Esp_At::Sleep_Mode::POWER_DOWN) == false)
        // {
        //     printf("esp32c6-at failed to enter sleep mode\n");
        // }
        // else
        // {
        //     printf("esp32c6-at successfully entered sleep mode\n");
        // }

#if defined CONFIG_CAMERA_SC2336
        SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
#elif defined CONFIG_CAMERA_OV2710
        SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::OFF);
        SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::OFF);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

        printf("stop and deinitialize Ethernet network...\n");
        // Stop Ethernet driver state machine and destroy netif
        for (int i = 0; i < eth_port_cnt; i++)
        {
            ESP_ERROR_CHECK(esp_eth_stop(eth_handles[i]));
            ESP_ERROR_CHECK(esp_eth_del_netif_glue(eth_netif_glues[i]));
            esp_netif_destroy(eth_netifs[i]);
        }
        esp_netif_deinit();
        ESP_ERROR_CHECK(example_eth_deinit(eth_handles, eth_port_cnt));
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler));
        ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler));
        ESP_ERROR_CHECK(esp_event_loop_delete_default());

        SX1262->set_sleep();

        // printf("esp_lcd_panel_disp_off\n");
        // esp_lcd_panel_disp_off(Screen_Mipi_Dpi_Panel, true);
        // printf("esp_lcd_panel_disp_sleep\n");
        // esp_lcd_panel_disp_sleep(Screen_Mipi_Dpi_Panel, true);

        // esp_lcd_panel_del(Screen_Mipi_Dpi_Panel);
        // printf("esp_lcd_panel_del\n");

        // XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO_PORT0, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        // XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO_PORT1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

        // XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
        XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        // XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
        // XL9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        XL9535->pin_mode(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
        XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
        XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        XL9535->pin_mode(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
        XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        // XL9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        // XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
        XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

        // XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO0, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        // XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO2, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        // XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO3, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO4, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO5, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO6, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO7, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO10, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        // XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO11, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO12, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO13, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO14, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO15, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        // XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO16, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
        XL9535->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO17, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    }
    else
    {
        printf("device sleep close\n");
    }
}

void sleep_task(void *args)
{
    while (true)
    {
        XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

        Device_Sleep_Status(true);

        for (size_t i = 0; i < gpio_num_t::GPIO_NUM_MAX; i++)
        {
            if ((i == LORA_BUSY) || (i == LORA_CS))
            {
                // ESP32P4->pin_mode(i, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::DISABLE);
            }
            else
            {
                ESP32P4->pin_mode(i, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
            }
        }

        ESP32P4->pin_mode(IIC_1_SDA, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
        ESP32P4->pin_mode(IIC_1_SCL, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

        ESP32P4->pin_mode(IIC_2_SDA, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);
        ESP32P4->pin_mode(IIC_2_SCL, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

        // ESP32P4->pin_mode(SDIO_1_CLK, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        // ESP32P4->pin_mode(SDIO_1_CMD, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        // ESP32P4->pin_mode(SDIO_1_D0, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        // ESP32P4->pin_mode(SDIO_1_D1, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        // ESP32P4->pin_mode(SDIO_1_D2, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        // ESP32P4->pin_mode(SDIO_1_D3, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);

        ESP32P4->pin_mode(SDIO_2_CLK, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        ESP32P4->pin_mode(SDIO_2_CMD, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        ESP32P4->pin_mode(SDIO_2_D0, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        ESP32P4->pin_mode(SDIO_2_D1, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        ESP32P4->pin_mode(SDIO_2_D2, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
        ESP32P4->pin_mode(SDIO_2_D3, Cpp_Bus_Driver::Tool::Pin_Mode::DISABLE, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);

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
    vTaskDelete(NULL);
}

void ES8311_Init(void)
{
    ES8311->begin(MCLK_MULTIPLE, SAMPLE_RATE, i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT);

    if (ES8311->begin(50000) == true)
    {
        printf("es8311 initialization success\n");
    }
    else
    {
        printf("es8311 initialization fail\n");
    }

    ES8311->set_master_clock_source(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK);
    ES8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_MCLK, true);
    ES8311->set_clock(Cpp_Bus_Driver::Es8311::Clock_Source::ADC_DAC_BCLK, true);

    ES8311->set_clock_coeff(MCLK_MULTIPLE, SAMPLE_RATE);

    ES8311->set_serial_port_mode(Cpp_Bus_Driver::Es8311::Serial_Port_Mode::SLAVE);

    ES8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::ADC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
    ES8311->set_sdp_data_bit_length(Cpp_Bus_Driver::Es8311::Sdp::DAC, Cpp_Bus_Driver::Es8311::Bits_Per_Sample::DATA_16BIT);
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
    ES8311->set_power_status(ps);
    ES8311->set_pga_power(true);
    ES8311->set_adc_power(true);
    ES8311->set_dac_power(true);
    ES8311->set_output_to_hp_drive(true);
    ES8311->set_adc_offset_freeze(Cpp_Bus_Driver::Es8311::Adc_Offset_Freeze::DYNAMIC_HPF);
    ES8311->set_adc_hpf_stage2_coeff(10);
    ES8311->set_dac_equalizer(false);

    ES8311->set_mic(Cpp_Bus_Driver::Es8311::Mic_Type::ANALOG_MIC, Cpp_Bus_Driver::Es8311::Mic_Input::MIC1P_1N);
    ES8311->set_adc_auto_volume_control(false);
    ES8311->set_adc_gain(Cpp_Bus_Driver::Es8311::Adc_Gain::GAIN_18DB);
    ES8311->set_adc_pga_gain(Cpp_Bus_Driver::Es8311::Adc_Pga_Gain::GAIN_30DB);

    ES8311->set_adc_volume(191);
    ES8311->set_dac_volume(210);

    // 将ADC的数据自动输出到DAC上
    // ES8311->set_adc_data_to_dac(true);
}

bool ICM20948_Init(void)
{
    if (ICM20948->begin() == false)
    {
        printf("ICM20948 AG initialization failed\n");
        return false;
    }

    if (ICM20948->initMagnetometer() == false)
    {
        printf("ICM20948 M initialization failed\n");
        return false;
    }

    printf("Position your ICM20948 flat and don't move it - calibrating...\n");
    ICM20948->autoOffsets();
    printf("Done!\n");

    ICM20948->setAccRange(ICM20948_ACC_RANGE_2G);
    ICM20948->setAccDLPF(ICM20948_DLPF_6);
    ICM20948->setMagOpMode(AK09916_CONT_MODE_20HZ);

    return true;
}

void IIC_Scan(void)
{
    std::vector<uint8_t> address_1;
    if (XL9535_IIC_Bus->scan_7bit_address(&address_1) == true)
    {
        for (size_t i = 0; i < address_1.size(); i++)
        {
            printf("Discovered IIC 1 devices[%u]: %#x\n", i, address_1[i]);
        }
    }

    std::vector<uint8_t> address_2;
    if (SGM38121_IIC_Bus->scan_7bit_address(&address_2) == true)
    {
        for (size_t i = 0; i < address_2.size(); i++)
        {
            printf("Discovered IIC 2 devices[%u]: %#x\n", i, address_2[i]);
        }
    }
}

void Ethernet_Init(void)
{
    // Initialize Ethernet driver
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create instance(s) of esp-netif for Ethernet(s)
    if (eth_port_cnt == 1)
    {
        // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
        // default esp-netif configuration parameters.
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netifs[0] = esp_netif_new(&cfg);
        eth_netif_glues[0] = esp_eth_new_netif_glue(eth_handles[0]);
        // Attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[0], eth_netif_glues[0]));
    }
    else
    {
        // Use ESP_NETIF_INHERENT_DEFAULT_ETH when multiple Ethernet interfaces are used and so you need to modify
        // esp-netif configuration parameters for each interface (name, priority, etc.).
        esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        esp_netif_config_t cfg_spi = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
        char if_key_str[10];
        char if_desc_str[10];
        char num_str[3];
        for (int i = 0; i < eth_port_cnt; i++)
        {
            itoa(i, num_str, 10);
            strcat(strcpy(if_key_str, "ETH_"), num_str);
            strcat(strcpy(if_desc_str, "eth"), num_str);
            esp_netif_config.if_key = if_key_str;
            esp_netif_config.if_desc = if_desc_str;
            esp_netif_config.route_prio -= i * 5;
            eth_netifs[i] = esp_netif_new(&cfg_spi);
            eth_netif_glues[i] = esp_eth_new_netif_glue(eth_handles[0]);
            // Attach Ethernet driver to TCP/IP stack
            ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[i], eth_netif_glues[i]));
        }
    }

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    for (int i = 0; i < eth_port_cnt; i++)
    {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }
}

void Esp32c6_At_Init(void)
{
    ESP32C6_AT->begin();

    // 开启falsh保存
    if (ESP32C6_AT->set_flash_save(true) == true)
    {
        printf("set_flash_save success\n");
    }
    else
    {
        printf("set_flash_save fail\n");
    }

    if (ESP32C6_AT->set_wifi_mode(Cpp_Bus_Driver::Esp_At::Wifi_Mode::STATION) == true)
    {
        printf("set_wifi_mode success\n");
    }
    else
    {
        printf("set_wifi_mode fail\n");
    }

    std::vector<uint8_t> buffer_wifi_scan;
    if (ESP32C6_AT->wifi_scan(buffer_wifi_scan) == true)
    {
        printf("wifi_scan: \n[%s]\n", buffer_wifi_scan.data());
    }
    else
    {
        printf("wifi_scan fail\n");
    }

    // std::string ssid = "xinyuandianzi";
    // std::string password = "AA15994823428";
    // if (ESP32C6_AT->set_wifi_connect(ssid, password) == true)
    // {
    //     printf("set_wifi_connect success\nconnected to wifi ssid: [%s],password: [%s]\n", ssid.c_str(), password.c_str());
    // }
    // else
    // {
    //     printf("set_wifi_connect fail\n");
    // }

    // Cpp_Bus_Driver::Esp_At::Real_Time rt;
    // if (ESP32C6_AT->get_real_time(rt) == true)
    // {
    //     printf("get_real_time success\n");
    //     printf("real_time week: [%s] day: [%d] month: [%d] year: [%d] time: [%d:%d:%d] time zone: [%s] china time: [%d:%d:%d]\n",
    //            rt.week.c_str(), rt.day, rt.month, rt.year, rt.hour, rt.minute, rt.second, rt.time_zone.c_str(),
    //            (rt.hour + 8 + 24) % 24, rt.minute, rt.second);
    // }
    // else
    // {
    //     printf("get_real_time fail\n");
    // }
}

void bsp_enable_dsi_phy_power(void)
{
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    printf("mipi dsi phy powered on\n");
}

bool Mipi_Dsi_Init(uint8_t num_data_lanes, uint32_t lane_bit_rate_mbps, uint32_t dpi_clock_freq_mhz, lcd_color_rgb_pixel_format_t color_format, uint8_t num_fbs, uint32_t width, uint32_t height,
                   uint32_t mipi_dsi_hsync, uint32_t mipi_dsi_hbp, uint32_t mipi_dsi_hfp, uint32_t mipi_dsi_vsync, uint32_t mipi_dsi_vbp, uint32_t mipi_dsi_vfp,
                   uint32_t bits_per_pixel, esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    esp_lcd_panel_io_handle_t mipi_dbi_io;

    auto cpp_assert = std::make_unique<Cpp_Bus_Driver::Tool>();

    // create MIPI DSI bus first, it will initialize the DSI PHY as well
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = num_data_lanes,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = lane_bit_rate_mbps,
    };

    esp_err_t assert = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_dsi_bus fail (error code: %#X)\n", assert);
        return false;
    }

    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t dbi_io_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,   // according to the LCD spec
        .lcd_param_bits = 8, // according to the LCD spec
    };
    assert = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_io_config, &mipi_dbi_io);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_panel_io_dbi fail (error code: %#X)\n", assert);
        return false;
    }

    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = dpi_clock_freq_mhz,
        .pixel_format = color_format,
        .num_fbs = num_fbs,
        .video_timing = {
            .h_size = width,
            .v_size = height,
            .hsync_pulse_width = mipi_dsi_hsync,
            .hsync_back_porch = mipi_dsi_hbp,
            .hsync_front_porch = mipi_dsi_hfp,
            .vsync_pulse_width = mipi_dsi_vsync,
            .vsync_back_porch = mipi_dsi_vbp,
            .vsync_front_porch = mipi_dsi_vfp,
        },
        .flags = {
            .use_dma2d = true, // use DMA2D to copy draw buffer into frame buffer
        }};

    hi8561_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = bits_per_pixel,
        .vendor_config = &vendor_config,
    };
    assert = esp_lcd_new_panel_hi8561(mipi_dbi_io, &dev_config, mipi_dpi_panel);
    if (assert != ESP_OK)
    {
        cpp_assert->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "esp_lcd_new_panel_hi8561 fail (error code: %#X)\n", assert);
        return false;
    }

    return true;
}

bool Screen_Init(esp_lcd_panel_handle_t *mipi_dpi_panel)
{
    if (Mipi_Dsi_Init(HI8561_SCREEN_DATA_LANE_NUM, HI8561_SCREEN_LANE_BIT_RATE_MBPS, HI8561_SCREEN_MIPI_DSI_DPI_CLK_MHZ, LCD_COLOR_PIXEL_FORMAT_RGB565,
                      0, HI8561_SCREEN_WIDTH, HI8561_SCREEN_HEIGHT, HI8561_SCREEN_MIPI_DSI_HSYNC, HI8561_SCREEN_MIPI_DSI_HBP,
                      HI8561_SCREEN_MIPI_DSI_HFP, HI8561_SCREEN_MIPI_DSI_VSYNC, HI8561_SCREEN_MIPI_DSI_VBP, HI8561_SCREEN_MIPI_DSI_VFP,
                      HI8561_SCREEN_BITS_PER_PIXEL_RGB565, mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

    return true;
}

void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves,
                                  size_t camera_buf_len, void *user_data)
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

    ppa_srm_oper_config_t srm_config =
        {
            .in =
                {
                    .buffer = camera_buf,
                    .pic_w = camera_buf_hes,
                    .pic_h = camera_buf_ves,
                    .block_w = camera_buf_hes,
                    .block_h = camera_buf_ves,
                    .block_offset_x = (camera_buf_hes > HI8561_SCREEN_WIDTH) ? (camera_buf_hes - HI8561_SCREEN_WIDTH) / 2 : 0,
                    .block_offset_y = (camera_buf_ves > HI8561_SCREEN_HEIGHT) ? (camera_buf_ves - HI8561_SCREEN_HEIGHT) / 2 : 0,
                    .srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,
                },

            .out =
                {
                    .buffer = lcd_buffer[camera_buf_index],
                    .buffer_size = ALIGN_UP(HI8561_SCREEN_WIDTH * HI8561_SCREEN_HEIGHT * (APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? 2 : 3), data_cache_line_size),
                    .pic_w = HI8561_SCREEN_WIDTH,
                    .pic_h = HI8561_SCREEN_HEIGHT,
                    .block_offset_x = 0,
                    .block_offset_y = 0,
                    .srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,
                },

            .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
            .scale_x = 1,
            .scale_y = 1,
            .mirror_x = true,
            .mirror_y = true,
            .rgb_swap = false,
            .byte_swap = false,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };

    if (camera_buf_hes > HI8561_SCREEN_WIDTH || camera_buf_ves > HI8561_SCREEN_HEIGHT)
    {
        // The resolution of the camera does not match the LCD resolution. Image processing can be done using PPA, but there will be some frame rate loss

        srm_config.in.block_w = (camera_buf_hes > HI8561_SCREEN_WIDTH) ? HI8561_SCREEN_WIDTH : camera_buf_hes;
        srm_config.in.block_h = (camera_buf_ves > HI8561_SCREEN_HEIGHT) ? HI8561_SCREEN_HEIGHT : camera_buf_ves;

        esp_err_t assert = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
        if (assert != ESP_OK)
        {
            printf("ppa_do_scale_rotate_mirror fail (error code: %#X)\n", assert);
        }

        assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 + 120,
                                           srm_config.in.block_w, srm_config.in.block_h + (HI8561_SCREEN_HEIGHT - srm_config.in.block_h) / 2 - 120, lcd_buffer[camera_buf_index]);
        if (assert != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
        }
    }
    else
    {
        // esp_err_t assert = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, 0, camera_buf_hes, camera_buf_ves, camera_buf);
        // if (assert != ESP_OK)
        // {
        //     printf("esp_lcd_panel_draw_bitmap fail (error code: %#X)\n", assert);
        // }
    }
}

bool App_Video_Init()
{
    esp_lcd_panel_handle_t mipi_dpi_panel = NULL;

    if (Mipi_Dsi_Init(CAMERA_DATA_LANE_NUM, CAMERA_LANE_BIT_RATE_MBPS, CAMERA_MIPI_DSI_DPI_CLK_MHZ, LCD_COLOR_PIXEL_FORMAT_RGB565,
                      CONFIG_EXAMPLE_CAM_BUF_COUNT, CAMERA_WIDTH, CAMERA_HEIGHT, 0, 0, 0, 0, 0, 0, CAMERA_BITS_PER_PIXEL_RGB565, &mipi_dpi_panel) == false)
    {
        printf("Mipi_Dsi_Init fail\n");
        return false;
    }

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

    assert = app_video_main(SGM38121_IIC_Bus->_iic_bus_handle);
    if (assert != ESP_OK)
    {
        printf("video_init fail (error code: %#X)\n", assert);
        return false;
    }

    video_cam_fd0 = app_video_open(EXAMPLE_CAM_DEV_PATH, APP_VIDEO_FMT);
    if (video_cam_fd0 < 0)
    {
        printf("video cam open fail (video_cam_fd0: %ld)\n", video_cam_fd0);
        return false;
    }

#if CONFIG_EXAMPLE_CAM_BUF_COUNT == 2
    assert = esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 2, &lcd_buffer[0], &lcd_buffer[1]);
#else
    assert = esp_lcd_dpi_panel_get_frame_buffer(mipi_dpi_panel, 3, &lcd_buffer[0], &lcd_buffer[1], &lcd_buffer[2]);
#endif
    if (assert != ESP_OK)
    {
        printf("esp_lcd_dpi_panel_get_frame_buffer fail (error code: %#X)\n", assert);
        return false;
    }

    // #if CONFIG_EXAMPLE_USE_MEMORY_MAPPING
    //     ESP_LOGI(TAG, "Using map buffer");
    //     // When setting the camera video buffer, it can be written as NULL to automatically allocate the buffer using mapping
    //     assert = app_video_set_bufs(app_video_set_bufs(video_cam_fd0, EXAMPLE_CAM_BUF_NUM, NULL));
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #elif CONFIG_CAMERA_CAMERA_MIPI_RAW8_1280X720_30FPS
    //     printf("using user defined buffer\n");
    //     assert = app_video_set_bufs(video_cam_fd0, CONFIG_EXAMPLE_CAM_BUF_COUNT, (const void **)lcd_buffer);
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #else
    //     void *camera_buf[EXAMPLE_CAM_BUF_NUM];
    //     for (int i = 0; i < EXAMPLE_CAM_BUF_NUM; i++)
    //     {
    //         camera_buf[i] = heap_caps_aligned_calloc(data_cache_line_size, 1, app_video_get_buf_size(), MALLOC_CAP_SPIRAM);
    //     }
    //     assert = app_video_set_bufs(video_cam_fd0, EXAMPLE_CAM_BUF_NUM, (const void **)camera_buf);
    //     if (assert != ESP_OK)
    //     {
    //         printf("app_video_set_bufs fail (error code: %#X)\n", assert);
    //         return false;
    //     }
    // #endif

    assert = app_video_set_bufs(video_cam_fd0, CONFIG_EXAMPLE_CAM_BUF_COUNT, (const void **)lcd_buffer);
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

    assert = app_video_stream_task_start(video_cam_fd0, 0, NULL);
    if (assert != ESP_OK)
    {
        printf("app_video_stream_task_start fail (error code: %#X)\n", assert);
        return false;
    }

    app_video_stream_task_stop(video_cam_fd0);

    // // Get the initial time for frame rate statistics
    // start_time = esp_timer_get_time();

    return true;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    // Hardware_Usb_Cdc_Init();

    XL9535->begin(500000);
    printf("XL9535 ID: %#X\n", XL9535->get_device_id());
    XL9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    // 开关3.3v电压时候必须先将GPS断电
    XL9535->pin_mode(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    // 开关3.3v电压时候必须先将ESP32C6断电
    XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("Ciallo1\n");
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("Ciall2\n");
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("Ciall3\n");

    // vTaskDelay(pdMS_TO_TICKS(1000));

    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("Ciall4\n");
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("Ciall5\n");
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("Ciall6\n");

    XL9535->pin_mode(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_ETHERNET_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    Ethernet_Init();

    // 这个必须放在以太网后面
    ESP32P4->create_pwm(HI8561_SCREEN_BL, ledc_channel_t::LEDC_CHANNEL_0, 2000);

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    SGM38121->begin();
#if defined CONFIG_CAMERA_SC2336
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 2800);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#elif defined CONFIG_CAMERA_OV2710
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, 1500);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, 1800);
    SGM38121->set_output_voltage(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, 3100);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::DVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_1, Cpp_Bus_Driver::Sgm38121::Status::ON);
    SGM38121->set_channel_status(Cpp_Bus_Driver::Sgm38121::Channel::AVDD_2, Cpp_Bus_Driver::Sgm38121::Status::ON);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

    bsp_enable_dsi_phy_power();

    vTaskDelay(pdMS_TO_TICKS(100));

    if (App_Video_Init() == false)
    {
        printf("App_Video_Init fail\n");
    }

    Screen_Init(&Screen_Mipi_Dpi_Panel);

    esp_err_t assert = esp_lcd_panel_reset(Screen_Mipi_Dpi_Panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_reset fail (error code: %#X)\n", assert);
    }
    assert = esp_lcd_panel_init(Screen_Mipi_Dpi_Panel);
    if (assert != ESP_OK)
    {
        printf("esp_lcd_panel_init fail (error code: %#X)\n", assert);
    }

    XL9535->pin_mode(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    HI8561_T_IIC_Bus->_iic_bus_handle = XL9535_IIC_Bus->_iic_bus_handle;
    HI8561_T->begin();

    XL9535->pin_mode(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    XL9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    // if (Sdmmc_Init(SD_BASE_PATH) == false)
    // {
    //     printf("Sdmmc_Init fail\n");
    // }

    // if (Sdspi_Init(SD_BASE_PATH) == false)
    // {
    //     printf("Sdspi_Init fail\n");
    // }

    PCF8563_IIC_Bus->_iic_bus_handle = XL9535_IIC_Bus->_iic_bus_handle;
    PCF8563->begin();

    // ESP32C6复位模式
    // XL9535->pin_mode(XL9535_ESP32C6_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Esp32c6_At_Init();

    BQ27220_IIC_Bus->_iic_bus_handle = XL9535_IIC_Bus->_iic_bus_handle;
    BQ27220->begin();

    // 设置的电池容量会在没有电池插入的时候自动还原为默认值
    BQ27220->set_design_capacity(2000);
    BQ27220->set_temperature_mode(Cpp_Bus_Driver::Bq27220xxxx::Temperature_Mode::EXTERNAL_NTC);
    BQ27220->set_sleep_current_threshold(50);

    AW86224_IIC_Bus->_iic_bus_handle = SGM38121_IIC_Bus->_iic_bus_handle;
    AW86224->begin(500000);
    // printf("AW86224 input voltage: %.06f V\n", AW86224->get_input_voltage());

    // RAM播放
    AW86224->init_ram_mode(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170, sizeof(Cpp_Bus_Driver::aw862xx_haptic_ram_12k_0809_170));

    ES8311_IIC_Bus->_iic_bus_handle = SGM38121_IIC_Bus->_iic_bus_handle;
    ES8311_Init();

    ICM20948_IIC_Bus->_iic_bus_handle = SGM38121_IIC_Bus->_iic_bus_handle;
    ICM20948_Init();

    XL9535->pin_write(XL9535_GPS_WAKE_UP, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    L76K->begin();
    printf("get_baud_rate:%ld\n", L76K->get_baud_rate());
    L76K->set_baud_rate(Cpp_Bus_Driver::L76k::Baud_Rate::BR_115200_BPS);
    printf("set_baud_rate:%ld\n", L76K->get_baud_rate());
    L76K->set_update_frequency(Cpp_Bus_Driver::L76k::Update_Freq::FREQ_5HZ);
    L76K->clear_rx_buffer_data();

    XL9535->pin_mode(XL9535_LORA_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    // LORA复位
    XL9535->pin_mode(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LORA_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    SX1262->begin(10000000);
    // SX1262->config_lora_params(920.0, Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_125000Hz, 140, 22);
    // SX1262->clear_buffer();

    // SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    // SX1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE,
    //                          Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE,
    //                          Cpp_Bus_Driver::Sx126x::Irq_Flag::DISABLE);
    // SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Flag::RX_DONE);

    // 设置整个屏幕为白色
    size_t screen_size = HI8561_SCREEN_WIDTH * HI8561_SCREEN_HEIGHT * 2; // RGB565: 2 bytes per pixel
    size_t data_cache_line_size = 16;                                    // 通常16或32，具体可查芯片手册
    void *white_buf = heap_caps_aligned_calloc(data_cache_line_size, 1, screen_size, MALLOC_CAP_SPIRAM);
    if (white_buf)
    {
        uint16_t *p = (uint16_t *)white_buf;
        for (size_t i = 0; i < HI8561_SCREEN_WIDTH * HI8561_SCREEN_HEIGHT; ++i)
        {
            p[i] = 0xFFFF; // RGB565白色
        }
        esp_err_t err = esp_lcd_panel_draw_bitmap(Screen_Mipi_Dpi_Panel, 0, 0, HI8561_SCREEN_WIDTH, HI8561_SCREEN_HEIGHT, white_buf);
        if (err != ESP_OK)
        {
            printf("esp_lcd_panel_draw_bitmap (white) fail (error code: %#X)\n", err);
        }
        heap_caps_free(white_buf);
    }

    // Start PWM backlight after LVGL refresh is complete
    ESP32P4->start_pwm_gradient_time(100, 500);

    vTaskDelay(pdMS_TO_TICKS(1000));

    // IIC_Scan();

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

    xTaskCreate(sleep_task, "sleep_task", 4 * 1024, NULL, 6, NULL);
}
