/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_check.h"
#include "es8311.h"
#include "example_config.h"
#include "pin_config.h"
#include "cpp_bus_driver_library.h"

static const char *TAG = "i2s_es8311";
static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"};
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;

/* Import music file as buffer */
#if CONFIG_EXAMPLE_MODE_MUSIC
extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[] asm("_binary_canon_pcm_end");
#endif

auto IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_2>(XL9535_SDA, XL9535_SCL, I2C_NUM_1);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

static esp_err_t es8311_codec_init(void)
{
/* Initialize I2C peripheral */
#if !defined(CONFIG_EXAMPLE_BSP)
    const i2c_config_t es_i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master =
            {
                .clk_speed = 100000,
            },
    };
    ESP_RETURN_ON_ERROR(i2c_param_config((i2c_port_t)I2C_NUM, &es_i2c_cfg), TAG, "config i2c failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install((i2c_port_t)I2C_NUM, I2C_MODE_MASTER, 0, 0, 0), TAG, "install i2c driver failed");
#else
    ESP_ERROR_CHECK(bsp_i2c_init());
#endif

    /* Initialize es8311 codec */
    es8311_handle_t es_handle = es8311_create((i2c_port_t)I2C_NUM, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
        .sample_frequency = EXAMPLE_SAMPLE_RATE};

    ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE, EXAMPLE_SAMPLE_RATE), TAG, "set es8311 sample frequency failed");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");
#if CONFIG_EXAMPLE_MODE_ECHO
    ESP_RETURN_ON_ERROR(es8311_microphone_gain_set(es_handle, (es8311_mic_gain_t)EXAMPLE_MIC_GAIN), TAG, "set es8311 microphone gain failed");
#endif
    return ESP_OK;
}

static esp_err_t i2s_driver_init(void)
{
#if !defined(CONFIG_EXAMPLE_BSP)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle));
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = (i2s_mclk_multiple_t)EXAMPLE_MCLK_MULTIPLE;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
#else
    ESP_LOGI(TAG, "Using BSP for HW configuration");
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;
    ESP_ERROR_CHECK(bsp_audio_init(&std_cfg, &tx_handle, &rx_handle));
    ESP_ERROR_CHECK(bsp_audio_poweramp_enable(true));
#endif
    return ESP_OK;
}

#if CONFIG_EXAMPLE_MODE_MUSIC
static void i2s_music(void *args)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;
    uint8_t *data_ptr = (uint8_t *)music_pcm_start;

    /* (Optional) Disable TX channel and preload the data before enabling the TX channel,
     * so that the valid data can be transmitted immediately */
    ESP_ERROR_CHECK(i2s_channel_disable(tx_handle));
    ESP_ERROR_CHECK(i2s_channel_preload_data(tx_handle, data_ptr, music_pcm_end - data_ptr, &bytes_write));
    data_ptr += bytes_write; // Move forward the data pointer

    /* Enable the TX channel */
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    while (1)
    {
        /* Write music to earphone */
        ret = i2s_channel_write(tx_handle, data_ptr, music_pcm_end - data_ptr, &bytes_write, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            /* Since we set timeout to 'portMAX_DELAY' in 'i2s_channel_write'
               so you won't reach here unless you set other timeout value,
               if timeout detected, it means write operation failed. */
            ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_write > 0)
        {
            ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", bytes_write);
        }
        else
        {
            ESP_LOGE(TAG, "[music] i2s music play failed.");
            abort();
        }
        data_ptr = (uint8_t *)music_pcm_start;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

#else
static void i2s_echo(void *args)
{
    int *mic_data = (int *)malloc(EXAMPLE_RECV_BUF_SIZE);
    if (!mic_data)
    {
        ESP_LOGE(TAG, "[echo] No memory for read data buffer");
        abort();
    }
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_write = 0;
    ESP_LOGI(TAG, "[echo] Echo start");

    // [2025-03-14 10:15:42.098] es8311 register[0]: 0X80
    // [2025-03-14 10:15:42.098] es8311 register[1]: 0X3F
    // [2025-03-14 10:15:42.098] es8311 register[2]: 0X48
    // [2025-03-14 10:15:42.098] es8311 register[3]: 0X10
    // [2025-03-14 10:15:42.098] es8311 register[4]: 0X10
    // [2025-03-14 10:15:42.098] es8311 register[5]: 0
    // [2025-03-14 10:15:42.098] es8311 register[6]: 0X3
    // [2025-03-14 10:15:42.098] es8311 register[7]: 0
    // [2025-03-14 10:15:42.098] es8311 register[8]: 0XFF
    // [2025-03-14 10:15:42.098] es8311 register[9]: 0XC
    // [2025-03-14 10:15:42.098] es8311 register[10]: 0XC
    // [2025-03-14 10:15:42.098] es8311 register[11]: 0
    // [2025-03-14 10:15:42.098] es8311 register[12]: 0X20
    // [2025-03-14 10:15:42.098] es8311 register[13]: 0X1
    // [2025-03-14 10:15:42.098] es8311 register[14]: 0X2
    // [2025-03-14 10:15:42.098] es8311 register[15]: 0
    // [2025-03-14 10:15:42.098] es8311 register[16]: 0X13
    // [2025-03-14 10:15:42.098] es8311 register[17]: 0X7C
    // [2025-03-14 10:15:42.098] es8311 register[18]: 0
    // [2025-03-14 10:15:42.098] es8311 register[19]: 0X10
    // [2025-03-14 10:15:42.098] es8311 register[20]: 0X1A
    // [2025-03-14 10:15:42.098] es8311 register[21]: 0
    // [2025-03-14 10:15:42.098] es8311 register[22]: 0X3
    // [2025-03-14 10:15:42.098] es8311 register[23]: 0XC8
    // [2025-03-14 10:15:42.098] es8311 register[24]: 0
    // [2025-03-14 10:15:42.098] es8311 register[25]: 0
    // [2025-03-14 10:15:42.098] es8311 register[26]: 0
    // [2025-03-14 10:15:42.098] es8311 register[27]: 0XC
    // [2025-03-14 10:15:42.098] es8311 register[28]: 0X6A
    // [2025-03-14 10:15:42.098] es8311 register[29]: 0
    // [2025-03-14 10:15:42.098] es8311 register[30]: 0
    // [2025-03-14 10:15:42.098] es8311 register[31]: 0
    // [2025-03-14 10:15:42.098] es8311 register[32]: 0
    // [2025-03-14 10:15:42.098] es8311 register[33]: 0
    // [2025-03-14 10:15:42.098] es8311 register[34]: 0
    // [2025-03-14 10:15:42.098] es8311 register[35]: 0
    // [2025-03-14 10:15:42.098] es8311 register[36]: 0
    // [2025-03-14 10:15:42.098] es8311 register[37]: 0
    // [2025-03-14 10:15:42.098] es8311 register[38]: 0
    // [2025-03-14 10:15:42.098] es8311 register[39]: 0
    // [2025-03-14 10:15:42.098] es8311 register[40]: 0
    // [2025-03-14 10:15:42.098] es8311 register[41]: 0
    // [2025-03-14 10:15:42.098] es8311 register[42]: 0
    // [2025-03-14 10:15:42.098] es8311 register[43]: 0
    // [2025-03-14 10:15:42.098] es8311 register[44]: 0
    // [2025-03-14 10:15:42.098] es8311 register[45]: 0
    // [2025-03-14 10:15:42.098] es8311 register[46]: 0
    // [2025-03-14 10:15:42.098] es8311 register[47]: 0
    // [2025-03-14 10:15:42.098] es8311 register[48]: 0
    // [2025-03-14 10:15:42.098] es8311 register[49]: 0
    // [2025-03-14 10:15:42.098] es8311 register[50]: 0XCB
    // [2025-03-14 10:15:42.098] es8311 register[51]: 0
    // [2025-03-14 10:15:42.098] es8311 register[52]: 0
    // [2025-03-14 10:15:42.098] es8311 register[53]: 0
    // [2025-03-14 10:15:42.098] es8311 register[54]: 0
    // [2025-03-14 10:15:42.098] es8311 register[55]: 0X8
    // [2025-03-14 10:15:42.098] es8311 register[56]: 0
    // [2025-03-14 10:15:42.098] es8311 register[57]: 0
    // [2025-03-14 10:15:42.098] es8311 register[58]: 0
    // [2025-03-14 10:15:42.098] es8311 register[59]: 0
    // [2025-03-14 10:15:42.098] es8311 register[60]: 0
    // [2025-03-14 10:15:42.098] es8311 register[61]: 0
    // [2025-03-14 10:15:42.098] es8311 register[62]: 0
    // [2025-03-14 10:15:42.098] es8311 register[63]: 0
    // [2025-03-14 10:15:42.098] es8311 register[64]: 0
    // [2025-03-14 10:15:42.098] es8311 register[65]: 0
    // [2025-03-14 10:15:42.098] es8311 register[66]: 0
    // [2025-03-14 10:15:42.098] es8311 register[67]: 0
    // [2025-03-14 10:15:42.098] es8311 register[68]: 0
    // [2025-03-14 10:15:42.098] es8311 register[69]: 0
    // [2025-03-14 10:15:42.098] es8311 register[70]: 0
    // [2025-03-14 10:15:42.098] es8311 register[71]: 0
    // [2025-03-14 10:15:42.098] es8311 register[72]: 0
    // [2025-03-14 10:15:42.098] es8311 register[73]: 0
    // [2025-03-14 10:15:42.098] es8311 register[74]: 0
    // [2025-03-14 10:15:42.098] es8311 register[75]: 0
    // [2025-03-14 10:15:42.098] es8311 register[76]: 0
    // [2025-03-14 10:15:42.098] es8311 register[77]: 0
    // [2025-03-14 10:15:42.098] es8311 register[78]: 0
    // [2025-03-14 10:15:42.098] es8311 register[79]: 0
    // [2025-03-14 10:15:42.098] es8311 register[80]: 0
    // [2025-03-14 10:15:42.098] es8311 register[81]: 0
    // [2025-03-14 10:15:42.098] es8311 register[82]: 0
    // [2025-03-14 10:15:42.098] es8311 register[83]: 0
    // [2025-03-14 10:15:42.098] es8311 register[84]: 0
    // [2025-03-14 10:15:42.098] es8311 register[85]: 0
    // [2025-03-14 10:15:42.098] es8311 register[86]: 0
    // [2025-03-14 10:15:42.098] es8311 register[87]: 0
    // [2025-03-14 10:15:42.098] es8311 register[88]: 0
    // [2025-03-14 10:15:42.098] es8311 register[89]: 0
    // [2025-03-14 10:15:42.098] es8311 register[90]: 0
    // [2025-03-14 10:15:42.098] es8311 register[91]: 0
    // [2025-03-14 10:15:42.098] es8311 register[92]: 0
    // [2025-03-14 10:15:42.098] es8311 register[93]: 0
    // [2025-03-14 10:15:42.098] es8311 register[94]: 0
    // [2025-03-14 10:15:42.098] es8311 register[95]: 0
    // [2025-03-14 10:15:42.098] es8311 register[96]: 0
    // [2025-03-14 10:15:42.098] es8311 register[97]: 0
    // [2025-03-14 10:15:42.098] es8311 register[98]: 0
    // [2025-03-14 10:15:42.098] es8311 register[99]: 0
    // [2025-03-14 10:15:42.098] es8311 register[100]: 0
    // [2025-03-14 10:15:42.098] es8311 register[101]: 0
    // [2025-03-14 10:15:42.098] es8311 register[102]: 0
    // [2025-03-14 10:15:42.098] es8311 register[103]: 0
    // [2025-03-14 10:15:42.098] es8311 register[104]: 0
    // [2025-03-14 10:15:42.098] es8311 register[105]: 0
    // [2025-03-14 10:15:42.098] es8311 register[106]: 0
    // [2025-03-14 10:15:42.098] es8311 register[107]: 0
    // [2025-03-14 10:15:42.098] es8311 register[108]: 0
    // [2025-03-14 10:15:42.098] es8311 register[109]: 0
    // [2025-03-14 10:15:42.098] es8311 register[110]: 0
    // [2025-03-14 10:15:42.098] es8311 register[111]: 0
    // [2025-03-14 10:15:42.098] es8311 register[112]: 0
    // [2025-03-14 10:15:42.098] es8311 register[113]: 0
    // [2025-03-14 10:15:42.098] es8311 register[114]: 0
    // [2025-03-14 10:15:42.098] es8311 register[115]: 0
    // [2025-03-14 10:15:42.098] es8311 register[116]: 0
    // [2025-03-14 10:15:42.098] es8311 register[117]: 0
    // [2025-03-14 10:15:42.098] es8311 register[118]: 0
    // [2025-03-14 10:15:42.098] es8311 register[119]: 0
    // [2025-03-14 10:15:42.098] es8311 register[120]: 0
    // [2025-03-14 10:15:42.098] es8311 register[121]: 0
    // [2025-03-14 10:15:42.098] es8311 register[122]: 0
    // [2025-03-14 10:15:42.098] es8311 register[123]: 0
    // [2025-03-14 10:15:42.098] es8311 register[124]: 0
    // [2025-03-14 10:15:42.098] es8311 register[125]: 0
    // [2025-03-14 10:15:42.098] es8311 register[126]: 0
    // [2025-03-14 10:15:42.098] es8311 register[127]: 0
    // [2025-03-14 10:15:42.098] es8311 register[128]: 0
    // [2025-03-14 10:15:42.098] es8311 register[129]: 0
    // [2025-03-14 10:15:42.098] es8311 register[130]: 0
    // [2025-03-14 10:15:42.098] es8311 register[131]: 0
    // [2025-03-14 10:15:42.098] es8311 register[132]: 0
    // [2025-03-14 10:15:42.098] es8311 register[133]: 0
    // [2025-03-14 10:15:42.098] es8311 register[134]: 0
    // [2025-03-14 10:15:42.098] es8311 register[135]: 0
    // [2025-03-14 10:15:42.098] es8311 register[136]: 0
    // [2025-03-14 10:15:42.098] es8311 register[137]: 0
    // [2025-03-14 10:15:42.098] es8311 register[138]: 0
    // [2025-03-14 10:15:42.098] es8311 register[139]: 0
    // [2025-03-14 10:15:42.098] es8311 register[140]: 0
    // [2025-03-14 10:15:42.098] es8311 register[141]: 0
    // [2025-03-14 10:15:42.098] es8311 register[142]: 0
    // [2025-03-14 10:15:42.098] es8311 register[143]: 0
    // [2025-03-14 10:15:42.098] es8311 register[144]: 0
    // [2025-03-14 10:15:42.098] es8311 register[145]: 0
    // [2025-03-14 10:15:42.098] es8311 register[146]: 0
    // [2025-03-14 10:15:42.098] es8311 register[147]: 0
    // [2025-03-14 10:15:42.098] es8311 register[148]: 0
    // [2025-03-14 10:15:42.098] es8311 register[149]: 0
    // [2025-03-14 10:15:42.098] es8311 register[150]: 0
    // [2025-03-14 10:15:42.098] es8311 register[151]: 0
    // [2025-03-14 10:15:42.098] es8311 register[152]: 0
    // [2025-03-14 10:15:42.098] es8311 register[153]: 0
    // [2025-03-14 10:15:42.098] es8311 register[154]: 0
    // [2025-03-14 10:15:42.098] es8311 register[155]: 0
    // [2025-03-14 10:15:42.098] es8311 register[156]: 0
    // [2025-03-14 10:15:42.098] es8311 register[157]: 0
    // [2025-03-14 10:15:42.098] es8311 register[158]: 0
    // [2025-03-14 10:15:42.098] es8311 register[159]: 0
    // [2025-03-14 10:15:42.098] es8311 register[160]: 0
    // [2025-03-14 10:15:42.098] es8311 register[161]: 0
    // [2025-03-14 10:15:42.098] es8311 register[162]: 0
    // [2025-03-14 10:15:42.098] es8311 register[163]: 0
    // [2025-03-14 10:15:42.098] es8311 register[164]: 0
    // [2025-03-14 10:15:42.098] es8311 register[165]: 0
    // [2025-03-14 10:15:42.098] es8311 register[166]: 0
    // [2025-03-14 10:15:42.098] es8311 register[167]: 0
    // [2025-03-14 10:15:42.098] es8311 register[168]: 0
    // [2025-03-14 10:15:42.098] es8311 register[169]: 0
    // [2025-03-14 10:15:42.098] es8311 register[170]: 0
    // [2025-03-14 10:15:42.098] es8311 register[171]: 0
    // [2025-03-14 10:15:42.098] es8311 register[172]: 0
    // [2025-03-14 10:15:42.098] es8311 register[173]: 0
    // [2025-03-14 10:15:42.098] es8311 register[174]: 0
    // [2025-03-14 10:15:42.098] es8311 register[175]: 0
    // [2025-03-14 10:15:42.098] es8311 register[176]: 0
    // [2025-03-14 10:15:42.098] es8311 register[177]: 0
    // [2025-03-14 10:15:42.098] es8311 register[178]: 0
    // [2025-03-14 10:15:42.098] es8311 register[179]: 0
    // [2025-03-14 10:15:42.098] es8311 register[180]: 0
    // [2025-03-14 10:15:42.098] es8311 register[181]: 0
    // [2025-03-14 10:15:42.098] es8311 register[182]: 0
    // [2025-03-14 10:15:42.098] es8311 register[183]: 0
    // [2025-03-14 10:15:42.098] es8311 register[184]: 0
    // [2025-03-14 10:15:42.098] es8311 register[185]: 0
    // [2025-03-14 10:15:42.098] es8311 register[186]: 0
    // [2025-03-14 10:15:42.098] es8311 register[187]: 0
    // [2025-03-14 10:15:42.098] es8311 register[188]: 0
    // [2025-03-14 10:15:42.098] es8311 register[189]: 0
    // [2025-03-14 10:15:42.098] es8311 register[190]: 0
    // [2025-03-14 10:15:42.098] es8311 register[191]: 0
    // [2025-03-14 10:15:42.098] es8311 register[192]: 0
    // [2025-03-14 10:15:42.098] es8311 register[193]: 0
    // [2025-03-14 10:15:42.098] es8311 register[194]: 0
    // [2025-03-14 10:15:42.098] es8311 register[195]: 0
    // [2025-03-14 10:15:42.098] es8311 register[196]: 0
    // [2025-03-14 10:15:42.098] es8311 register[197]: 0
    // [2025-03-14 10:15:42.098] es8311 register[198]: 0
    // [2025-03-14 10:15:42.098] es8311 register[199]: 0
    // [2025-03-14 10:15:42.098] es8311 register[200]: 0
    // [2025-03-14 10:15:42.098] es8311 register[201]: 0
    // [2025-03-14 10:15:42.098] es8311 register[202]: 0
    // [2025-03-14 10:15:42.098] es8311 register[203]: 0
    // [2025-03-14 10:15:42.098] es8311 register[204]: 0
    // [2025-03-14 10:15:42.098] es8311 register[205]: 0
    // [2025-03-14 10:15:42.098] es8311 register[206]: 0
    // [2025-03-14 10:15:42.098] es8311 register[207]: 0
    // [2025-03-14 10:15:42.098] es8311 register[208]: 0
    // [2025-03-14 10:15:42.098] es8311 register[209]: 0
    // [2025-03-14 10:15:42.098] es8311 register[210]: 0
    // [2025-03-14 10:15:42.098] es8311 register[211]: 0
    // [2025-03-14 10:15:42.098] es8311 register[212]: 0
    // [2025-03-14 10:15:42.098] es8311 register[213]: 0
    // [2025-03-14 10:15:42.098] es8311 register[214]: 0
    // [2025-03-14 10:15:42.098] es8311 register[215]: 0
    // [2025-03-14 10:15:42.098] es8311 register[216]: 0
    // [2025-03-14 10:15:42.098] es8311 register[217]: 0
    // [2025-03-14 10:15:42.098] es8311 register[218]: 0
    // [2025-03-14 10:15:42.098] es8311 register[219]: 0
    // [2025-03-14 10:15:42.098] es8311 register[220]: 0
    // [2025-03-14 10:15:42.098] es8311 register[221]: 0
    // [2025-03-14 10:15:42.098] es8311 register[222]: 0
    // [2025-03-14 10:15:42.098] es8311 register[223]: 0
    // [2025-03-14 10:15:42.098] es8311 register[224]: 0
    // [2025-03-14 10:15:42.098] es8311 register[225]: 0
    // [2025-03-14 10:15:42.098] es8311 register[226]: 0
    // [2025-03-14 10:15:42.098] es8311 register[227]: 0
    // [2025-03-14 10:15:42.098] es8311 register[228]: 0
    // [2025-03-14 10:15:42.098] es8311 register[229]: 0
    // [2025-03-14 10:15:42.098] es8311 register[230]: 0
    // [2025-03-14 10:15:42.098] es8311 register[231]: 0
    // [2025-03-14 10:15:42.098] es8311 register[232]: 0
    // [2025-03-14 10:15:42.098] es8311 register[233]: 0
    // [2025-03-14 10:15:42.098] es8311 register[234]: 0
    // [2025-03-14 10:15:42.098] es8311 register[235]: 0
    // [2025-03-14 10:15:42.098] es8311 register[236]: 0
    // [2025-03-14 10:15:42.098] es8311 register[237]: 0
    // [2025-03-14 10:15:42.098] es8311 register[238]: 0
    // [2025-03-14 10:15:42.098] es8311 register[239]: 0
    // [2025-03-14 10:15:42.098] es8311 register[240]: 0
    // [2025-03-14 10:15:42.098] es8311 register[241]: 0
    // [2025-03-14 10:15:42.098] es8311 register[242]: 0
    // [2025-03-14 10:15:42.098] es8311 register[243]: 0
    // [2025-03-14 10:15:42.098] es8311 register[244]: 0
    // [2025-03-14 10:15:42.098] es8311 register[245]: 0
    // [2025-03-14 10:15:42.098] es8311 register[246]: 0
    // [2025-03-14 10:15:42.098] es8311 register[247]: 0
    // [2025-03-14 10:15:42.098] es8311 register[248]: 0
    // [2025-03-14 10:15:42.098] es8311 register[249]: 0
    // [2025-03-14 10:15:42.098] es8311 register[250]: 0
    // [2025-03-14 10:15:42.098] es8311 register[251]: 0
    // [2025-03-14 10:15:42.098] es8311 register[252]: 0X70
    // [2025-03-14 10:15:42.098] es8311 register[253]: 0X83
    // [2025-03-14 10:15:42.098] es8311 register[254]: 0X11
    // [2025-03-14 10:15:42.098] es8311 register[255]: 0X1

    uint8_t buffer = 0;
    for (size_t i = 0; i < 256; i++)
    {
        uint8_t y = i;
        i2c_master_write_read_device((i2c_port_t)I2C_NUM, ES8311_ADDRRES_0, &y, 1, &buffer, 1, pdMS_TO_TICKS(1000));
        printf("es8311 register[%d]: %#X\n", i, buffer);
    }

    while (1)
    {
        memset(mic_data, 0, EXAMPLE_RECV_BUF_SIZE);
        /* Read sample data from mic */
        ret = i2s_channel_read(rx_handle, mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_read, 1000);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "[echo] i2s read failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        /* Write sample data to earphone */
        ret = i2s_channel_write(tx_handle, mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_write, 1000);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "[echo] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_read != bytes_write)
        {
            ESP_LOGW(TAG, "[echo] %d bytes read but only %d bytes are written", bytes_read, bytes_write);
        }
    }
    vTaskDelete(NULL);
}
#endif

extern "C" void app_main(void)
{
    printf("i2s es8311 codec example start\n-----------------------------\n");

    XL9535->begin();
    XL9535->pin_mode(XL9535_IO6_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_IO6_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(500));

    /* Initialize i2s peripheral */
    if (i2s_driver_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "i2s driver init failed");
        abort();
    }
    else
    {
        ESP_LOGI(TAG, "i2s driver init success");
    }
    /* Initialize i2c peripheral and config es8311 codec by i2c */
    if (es8311_codec_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "es8311 codec init failed");
        abort();
    }
    else
    {
        ESP_LOGI(TAG, "es8311 codec init success");
    }

#if CONFIG_EXAMPLE_MODE_MUSIC
    /* Play a piece of music in music mode */
    xTaskCreate(i2s_music, "i2s_music", 4096, NULL, 5, NULL);
#else
    /* Echo the sound from MIC in echo mode */
    xTaskCreate(i2s_echo, "i2s_echo", 8192, NULL, 5, NULL);
#endif
}
