/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-02-25 09:09:45
 * @LastEditTime: 2026-03-17 09:49:35
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#include "esp_audio_dec_default.h"
#include "esp_audio_dec.h"

#define AUDIO_MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS 2
#define AUDIO_BITS_PER_SAMPLE i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT
// #define MP3_FILE_PATH SD_BASE_PATH "/Nocturne, Op.9 No.2 in E-flat major-Aya Higuchi (piano).mp3"
#define MP3_FILE_PATH SD_BASE_PATH "/music.mp3"

size_t Cycle_Time = 0;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Es8311_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);

auto Es8311_Iis_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK,
                                                                     i2s_port_t::I2S_NUM_0, Cpp_Bus_Driver::Hardware_Iis::Data_Mode::INPUT_OUTPUT, Cpp_Bus_Driver::Hardware_Iis::Iis_Mode::STD,
                                                                     i2s_clock_src_t::I2S_CLK_SRC_APLL);

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Es8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(Es8311_Iic_Bus, Es8311_Iis_Bus, ES8311_IIC_ADDRESS);

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

uint32_t synchsafe_to_uint32(const uint8_t *buf)
{
    return ((buf[0] & 0x7F) << 21) | ((buf[1] & 0x7F) << 14) | ((buf[2] & 0x7F) << 7) | (buf[3] & 0x7F);
}

uint32_t bigendian_to_uint32(const uint8_t *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

size_t parse_and_print_id3v2(FILE *f)
{
    uint8_t header[10];
    if (fread(header, 1, 10, f) != 10)
    {
        printf("Failed to read ID3v2 header\n");
        rewind(f);
        return 0;
    }

    if (memcmp(header, "ID3", 3) != 0)
    {
        printf("No ID3v2 tag found\n");
        rewind(f);
        return 0;
    }

    uint8_t major_version = header[3];
    uint8_t revision = header[4];
    uint8_t flags = header[5];
    uint32_t tag_size = synchsafe_to_uint32(header + 6);

    printf("ID3v2.%d.%d tag detected, flags: 0x%02X, size: %lu bytes\n", major_version, revision, flags, tag_size);

    bool has_extended_header = (flags & 0x40) != 0;

    size_t pos = 10;
    uint32_t extended_size = 0;

    if (has_extended_header)
    {
        uint8_t ext_header_size_buf[4];
        if (fread(ext_header_size_buf, 1, 4, f) != 4)
        {
            printf("Failed to read extended header size\n");
            rewind(f);
            return 0;
        }
        if (major_version == 4)
        {
            extended_size = synchsafe_to_uint32(ext_header_size_buf);
        }
        else
        {
            extended_size = bigendian_to_uint32(ext_header_size_buf);
        }
        // 跳过扩展头的剩余部分
        fseek(f, extended_size - 4, SEEK_CUR);
        pos += extended_size;
    }

    // 解析帧，直到达到标签大小或填充区域
    while (pos < 10 + tag_size)
    {
        char frame_id[5] = {0};
        if (fread(frame_id, 1, 4, f) != 4)
            break;
        pos += 4;

        if (frame_id[0] == 0)
            break; // 已到达填充区域

        uint8_t frame_size_buf[4];
        if (fread(frame_size_buf, 1, 4, f) != 4)
            break;
        pos += 4;

        uint32_t frame_size;
        if (major_version == 4)
        {
            frame_size = synchsafe_to_uint32(frame_size_buf);
        }
        else
        {
            frame_size = bigendian_to_uint32(frame_size_buf);
        }

        uint8_t frame_flags[2];
        if (fread(frame_flags, 1, 2, f) != 2)
            break;
        pos += 2;

        auto frame_data = std::make_unique<uint8_t[]>(frame_size);
        if (fread(frame_data.get(), 1, frame_size, f) != frame_size)
            break;
        pos += frame_size;

        // 打印帧信息
        printf("Frame %s (size %lu, flags 0x%02X%02X): ", frame_id, frame_size, frame_flags[0], frame_flags[1]);

        if (frame_size < 1)
        {
            printf("Empty frame\n");
            continue;
        }

        uint8_t encoding = frame_data[0];
        const uint8_t *text_start = frame_data.get() + 1;
        size_t text_len = frame_size - 1;

        // 通过查找空终止符来确定实际字符串长度
        size_t actual_text_len = text_len;
        if (encoding == 0 || encoding == 3)
        { // ISO-8859-1 或 UTF-8：单个空字符
            for (size_t i = 0; i < text_len; ++i)
            {
                if (text_start[i] == 0)
                {
                    actual_text_len = i;
                    break;
                }
            }
        }
        else if (encoding == 1 || encoding == 2)
        { // UTF-16 变体：双空字符
            for (size_t i = 0; i + 1 < text_len; i += 2)
            {
                if (text_start[i] == 0 && text_start[i + 1] == 0)
                {
                    actual_text_len = i;
                    break;
                }
            }
        }

        if (frame_id[0] == 'T' && strcmp(frame_id, "TXXX") != 0)
        { // 标准文本帧（TIT2, TPE1等）
            printf("Text (%s): ", encoding == 0 ? "ISO-8859-1" : encoding == 1 ? "UTF-16 (with BOM)"
                                                             : encoding == 2   ? "UTF-16BE"
                                                             : encoding == 3   ? "UTF-8"
                                                                               : "Unknown");

            if (encoding == 0)
            {
                // ISO-8859-1： 直接作为char*打印
                printf("%.*s\n", (int)actual_text_len, text_start);
            }
            else if (encoding == 3)
            {
                // UTF-8： 在大多数控制台中可以直接打印
                printf("%.*s\n", (int)actual_text_len, text_start);
            }
            else if (encoding == 1 || encoding == 2)
            {
                // UTF-16： 为简化，可以打印十六进制或跳过完整解码（C++ std::wstring_convert较重）
                // 这里只显示前几个字符的十六进制，避免在嵌入式代码中进行复杂转换
                printf("(UTF-16 data, first 20 bytes hex): ");
                for (size_t i = 0; i < std::min(actual_text_len, (size_t)20); ++i)
                {
                    printf("%02X ", text_start[i]);
                }
                printf("...\n");
                // 如果确实需要可读的UTF-16文本，需要UTF-16转UTF-8转换器（例如iconv或手动实现）
                // 但在ESP32上通常只支持0和3编码更简单
            }
            else
            {
                printf("Unsupported encoding %d\n", encoding);
            }
        }
        else if (strcmp(frame_id, "APIC") == 0)
        {
            printf("Attached picture (not printing binary data)\n");
        }
        else
        {
            // 其他帧：显示原始十六进制预览
            printf("Raw data (hex first 16 bytes): ");
            for (size_t i = 0; i < std::min((size_t)16, text_len + 1); ++i)
            { // +1 包含编码字节
                printf("%02X ", frame_data[i]);
            }
            if (frame_size > 16)
                printf("...\n");
            else
                printf("\n");
        }
    }

    // 返回MP3数据的起始偏移量
    return 10 + tag_size;
}

void parse_and_print_id3v1(FILE *f)
{
    long original_pos = ftell(f);

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    if (file_size < 128)
    {
        fseek(f, original_pos, SEEK_SET);
        return;
    }

    fseek(f, -128, SEEK_END);
    uint8_t tag[128];
    if (fread(tag, 1, 128, f) != 128)
    {
        fseek(f, original_pos, SEEK_SET);
        return;
    }

    if (memcmp(tag, "TAG", 3) != 0)
    {
        printf("No ID3v1 tag found\n");
        fseek(f, original_pos, SEEK_SET);
        return;
    }

    printf("ID3v1 tag detected:\n");
    printf("Title: %.30s\n", reinterpret_cast<char *>(tag + 3));
    printf("Artist: %.30s\n", reinterpret_cast<char *>(tag + 33));
    printf("Album: %.30s\n", reinterpret_cast<char *>(tag + 63));
    printf("Year: %.4s\n", reinterpret_cast<char *>(tag + 93));

    if (tag[125] == 0 && tag[126] != 0)
    { // ID3v1.1
        printf("Comment: %.28s\n", reinterpret_cast<char *>(tag + 97));
        printf("Track: %u\n", tag[126]);
    }
    else
    {
        printf("Comment: %.30s\n", reinterpret_cast<char *>(tag + 97));
    }
    printf("Genre: %u\n", tag[127]);

    fseek(f, original_pos, SEEK_SET);
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    Esp32p4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);

    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_mode(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(4, 3300);

    vTaskDelay(pdMS_TO_TICKS(100));

    Es8311->begin(AUDIO_MCLK_MULTIPLE, AUDIO_SAMPLE_RATE, i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT);

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

    Es8311->set_clock_coeff(AUDIO_MCLK_MULTIPLE, AUDIO_SAMPLE_RATE);

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
    Es8311->set_dac_volume(191);

    if (Lilygo_Device_Driver::Sdmmc_Init(SD_BASE_PATH) == false)
    {
        printf("Sdmmc_Init fail\n");
    }

    while (1)
    {
        // 监测按键，按下后触发 MP3 解码播放
        if (Esp32p4->pin_read(ESP32P4_BOOT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));
            printf("play_mp3 start\n");

            esp_audio_dec_register_default();
            esp_audio_dec_cfg_t dec_cfg;
            dec_cfg.type = ESP_AUDIO_TYPE_MP3;

            esp_audio_dec_handle_t decoder = NULL;
            if (esp_audio_dec_open(&dec_cfg, &decoder) != ESP_AUDIO_ERR_OK)
            {
                printf("esp_audio_dec_open fail\n");
            }
            else
            {
                FILE *f_in = fopen(MP3_FILE_PATH, "rb");
                if (f_in == nullptr)
                {
                    printf("fopen mp3 fail: %s\n", MP3_FILE_PATH);
                }
                else
                {
                    // Parse and print ID3v2, get offset to MP3 data
                    size_t mp3_offset = parse_and_print_id3v2(f_in);

                    // Parse and print ID3v1 (restores file position)
                    parse_and_print_id3v1(f_in);

                    // Seek to the start of actual MP3 data
                    fseek(f_in, mp3_offset, SEEK_SET);

                    auto read_buf = std::make_unique<uint8_t[]>(4 * 1024);
                    auto pcm_buf = std::make_unique<uint8_t[]>(8 * 1024);
                    size_t remain_bytes = 0;

                    // 流式读取与解码循环
                    while (1)
                    {
                        size_t read_bytes = fread(read_buf.get() + remain_bytes, 1, (4 * 1024) - remain_bytes, f_in);
                        size_t total_bytes = remain_bytes + read_bytes;

                        if (total_bytes == 0)
                        {
                            break; // 读取完毕
                        }

                        esp_audio_dec_in_raw_t in_raw = {.buffer = read_buf.get(), .len = total_bytes};
                        esp_audio_dec_out_frame_t out_frame = {.buffer = pcm_buf.get(), .len = 8 * 1024};

                        while (in_raw.len > 0)
                        {
                            esp_audio_err_t assert = esp_audio_dec_process(decoder, &in_raw, &out_frame);
                            if (assert != ESP_AUDIO_ERR_OK)
                            {
                                if (assert == ESP_AUDIO_ERR_FAIL)
                                {
                                    printf("esp_audio_dec_process fail\n");
                                }

                                break;
                            }

                            // 如果成功解码出数据，将其推入 ES8388 播放
                            if (out_frame.decoded_size > 0)
                            {
                                Es8311->write_data(out_frame.buffer, out_frame.decoded_size);
                            }

                            // 消耗量为 0，说明当前数据不足以解码一帧，需跳出内循环读取更多数据
                            if (in_raw.consumed == 0)
                            {
                                break;
                            }

                            in_raw.buffer += in_raw.consumed;
                            in_raw.len -= in_raw.consumed;
                        }

                        // 将剩余未处理的数据移到 buffer 头部
                        remain_bytes = in_raw.len;
                        if (remain_bytes > 0)
                        {
                            memmove(read_buf.get(), in_raw.buffer, remain_bytes);
                        }

                        if (read_bytes == 0 && remain_bytes == total_bytes)
                        {
                            break; // 遇到 EOF 且无法再消耗数据
                        }
                    }

                    fclose(f_in);
                }
                esp_audio_dec_close(decoder);
            }
            esp_audio_dec_unregister_default();

            printf("play_mp3 finish\n");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}