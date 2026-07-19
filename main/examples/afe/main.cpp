/*
 * @Description: Afe
 * @Author: LILYGO_L
 * @Date: 2025-07-22 15:02:53
 * @LastEditTime: 2026-03-26 14:58:43
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#include "esp_afe_sr_models.h"
#include "hiesp.h"
#include "hilexin.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 16000

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Es8311_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);

auto Es8311_Iis_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Es8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(Es8311_Iic_Bus, Es8311_Iis_Bus, ES8311_IIC_ADDRESS);
auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

esp_afe_sr_iface_t *afe_handle = NULL;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);

    auto iis_buffer = std::make_unique<int16_t[]>(audio_chunksize * sizeof(int16_t));

    while (1)
    {
        Es8311->read_data(iis_buffer.get(), audio_chunksize * sizeof(uint16_t));

        // Es8311->write_data(iis_buffer, audio_chunksize * sizeof(uint16_t));

        // for (uint8_t i = 0; i < 10; i++)
        // {
        //     printf("read_data: %d\n", iis_buffer[i]);
        // }

        afe_handle->feed(afe_data, iis_buffer.get());

        vTaskDelay(pdMS_TO_TICKS(50));
    }

    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    printf("------------detect start------------\n");

    // modify wakenet detection threshold
    // afe_handle->set_wakenet_threshold(afe_data, 1, 0.6); // set model1's threshold to 0.6
    // afe_handle->set_wakenet_threshold(afe_data, 2, 0.6); // set model2's threshold to 0.6
    afe_handle->reset_wakenet_threshold(afe_data, 1); // reset model1's threshold to default
    afe_handle->reset_wakenet_threshold(afe_data, 2); // reset model2's threshold to default

    size_t cycle_time = 0;
    while (1)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL)
        {
            printf("fetch error!\n");
            break;
        }

        if (esp_log_timestamp() > cycle_time)
        {
            // 这里检测是否有语音
            printf("vad state: %d\n", res->vad_state);
            cycle_time = esp_log_timestamp() + 1000;
        }

        if (res->wakeup_state == WAKENET_DETECTED)
        {
            printf("wakeword detected\n");
            printf("model index:%d, word index:%d\n", res->wakenet_model_index, res->wake_word_index);
            printf("-----------LISTENING-----------\n");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

void Afe_Init()
{
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models)
    {
        for (int i = 0; i < models->num; i++)
        {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL)
            {
                printf("wakenet model in flash: %s\n", models->model_name[i]);
            }
        }
    }

    afe_config_t *afe_config = afe_config_init("MM", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);

    // print/modify wake word model.
    if (afe_config->wakenet_model_name)
    {
        printf("wakeword model in AFE config: %s\n", afe_config->wakenet_model_name);
    }
    if (afe_config->wakenet_model_name_2)
    {
        printf("wakeword model in AFE config: %s\n", afe_config->wakenet_model_name_2);
    }

    afe_handle = const_cast<esp_afe_sr_iface_t *>(esp_afe_handle_from_config(afe_config));
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);

    //
    afe_config_free(afe_config);

    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void *)afe_data, 5, NULL, 1);
}

void Wakenet_Init()
{
    srmodel_list_t *models = esp_srmodel_init("model");
    char *model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "hilexin");
    esp_wn_iface_t *wakenet = (esp_wn_iface_t *)esp_wn_handle_from_name(model_name);
    model_iface_data_t *model_data = wakenet->create(model_name, DET_MODE_95);

    int audio_chunksize = wakenet->get_samp_chunksize(model_data) * sizeof(int16_t);
    int16_t *buffer = (int16_t *)malloc(audio_chunksize);
    unsigned char *data = NULL;
    size_t data_size = 0;
    if (strstr(model_name, "hiesp") != NULL)
    {
        data = (unsigned char *)hiesp;
        data_size = sizeof(hiesp);
        printf("wake word: %s, size:%d\n", "hiesp", data_size);
    }
    else if (strstr(model_name, "hilexin") != NULL)
    {
        data = (unsigned char *)hilexin;
        data_size = sizeof(hilexin);
        printf("wake word: %s, size:%d\n", "hilexin", data_size);
    }

    int chunks = 0;
    while (1)
    {
        if ((chunks + 1) * audio_chunksize <= data_size)
        {
            memcpy(buffer, data + chunks * audio_chunksize, audio_chunksize);
        }
        else
        {
            chunks = 0;
            // break;
        }

        wakenet_state_t state = wakenet->detect(model_data, buffer);
        if (state == WAKENET_DETECTED)
        {
            printf("Detected\n");
        }
        chunks++;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    wakenet->destroy(model_data);
    vTaskDelete(NULL);
}

extern "C" void app_main()
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

    Es8311->begin(MCLK_MULTIPLE, SAMPLE_RATE, i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT);

    while (1)
    {
        if (Es8311->begin(50000) == true)
        {
            printf("es8311 initialization success\n");
            break;
        }
        else
        {
            printf("es8311 initialization fail\n");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
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
    Es8311->set_dac_volume(220);

    // 将ADC的数据自动输出到DAC上
    // Es8311->set_adc_data_to_dac(true);

    // Wakenet_Init();
    Afe_Init();
}
