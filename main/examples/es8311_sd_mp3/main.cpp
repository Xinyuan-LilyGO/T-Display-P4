/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-02-25 09:09:45
 * @LastEditTime: 2026-04-13 17:08:47
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#include "esp_audio_dec_default.h"
#include "esp_audio_dec.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
#include "kode_bq25896.h"
#endif

#define AUDIO_MCLK_MULTIPLE 256
#define AUDIO_CHANNELS 2
#define AUDIO_BITS_PER_SAMPLE 16
// #define MP3_FILE_PATH SD_BASE_PATH "/Nocturne, Op.9 No.2 in E-flat major-Aya Higuchi (piano).mp3"
#define MP3_FILE_PATH SD_BASE_PATH "/music.mp3"

uint32_t Audio_Sample_Rate = 44100;

size_t Cycle_Time = 0;

auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
auto Es8311_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_1);

auto Es8311_Iis_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK,
                                                                     i2s_port_t::I2S_NUM_0, Cpp_Bus_Driver::Hardware_Iis::Data_Mode::INPUT_OUTPUT, Cpp_Bus_Driver::Hardware_Iis::Iis_Mode::STD,
                                                                     i2s_clock_src_t::I2S_CLK_SRC_DEFAULT);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V2_0
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);
auto Es8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(Es8311_Iic_Bus, Es8311_Iis_Bus, ES8311_IIC_ADDRESS);

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

size_t parse_and_print_id3v2(FILE *f)
{
    // 处理字节序转换
    auto synchsafe_to_uint32 = [](const uint8_t *buf) -> uint32_t
    {
        return ((buf[0] & 0x7F) << 21) | ((buf[1] & 0x7F) << 14) | ((buf[2] & 0x7F) << 7) | (buf[3] & 0x7F);
    };

    auto bigendian_to_uint32 = [](const uint8_t *buf) -> uint32_t
    {
        return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    };

    // 获取文件总大小用于计算时长
    long current_pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long file_total_size = ftell(f);
    fseek(f, current_pos, SEEK_SET);

    uint8_t header[10];
    bool has_id3v2 = false;
    uint32_t id3v2_size = 0;
    uint8_t major_version = 0;

    // 读取 ID3v2 头部
    if (fread(header, 1, 10, f) == 10 && memcmp(header, "ID3", 3) == 0)
    {
        has_id3v2 = true;
        major_version = header[3];
        id3v2_size = synchsafe_to_uint32(header + 6);

        printf("Input from '%s':\n", MP3_FILE_PATH);
        printf("  Metadata:\n");

        size_t pos = 10;
        if (header[5] & 0x40) // 扩展头
        {
            uint8_t ext_buf[4];
            fread(ext_buf, 1, 4, f);
            uint32_t ext_size = (major_version == 4) ? synchsafe_to_uint32(ext_buf) : bigendian_to_uint32(ext_buf);
            fseek(f, ext_size - 4, SEEK_CUR);
            pos += ext_size;
        }

        // 解析 ID3v2 Frames
        while (pos < 10 + id3v2_size)
        {
            char frame_id[5] = {0};
            if (fread(frame_id, 1, 4, f) != 4 || frame_id[0] == 0)
                break;
            pos += 4;

            uint8_t sz_buf[4];
            fread(sz_buf, 1, 4, f);
            uint32_t frame_size = (major_version == 4) ? synchsafe_to_uint32(sz_buf) : bigendian_to_uint32(sz_buf);
            pos += 4;

            fseek(f, 2, SEEK_CUR); // 跳过 flags
            pos += 2;

            // 映射常见的 ID3 帧 ID 到对应标签
            const char *tag_key = nullptr;
            if (strcmp(frame_id, "TIT2") == 0)
                tag_key = "title";
            else if (strcmp(frame_id, "TALB") == 0)
                tag_key = "album";
            else if (strcmp(frame_id, "TPE1") == 0)
                tag_key = "artist";
            else if (strcmp(frame_id, "TPE2") == 0)
                tag_key = "album_artist";
            else if (strcmp(frame_id, "TCON") == 0)
                tag_key = "genre";
            else if (strcmp(frame_id, "TSSE") == 0)
                tag_key = "encoder";

            if (tag_key && frame_size > 1)
            {
                auto frame_data = std::make_unique<uint8_t[]>(frame_size + 1);
                fread(frame_data.get(), 1, frame_size, f);
                frame_data[frame_size] = 0;

                uint8_t encoding = frame_data[0];
                if (encoding == 0 || encoding == 3)
                { // ASCII / UTF-8
                    printf("    %-16s: %s\n", tag_key, (char *)frame_data.get() + 1);
                }
                else
                {
                    // 对于 UTF-16 等复杂编码，暂时避免乱码输出
                    printf("    %-16s: (UTF-16 text)\n", tag_key);
                }
            }
            else
            {
                fseek(f, frame_size, SEEK_CUR);
            }
            pos += frame_size;
        }
    }
    else
    {
        printf("Error metadata input from '%s':\n", MP3_FILE_PATH);
        rewind(f);
    }

    // 寻找真实音频流的起始位置
    long mp3_data_start = has_id3v2 ? (10 + id3v2_size) : 0;
    fseek(f, mp3_data_start, SEEK_SET);

    uint8_t frame_header[4];
    bool header_found = false;
    uint32_t sample_rate = 0, bitrate_kbps = 0;
    const char *channel_str = "stereo";

    for (int i = 0; i < 8192; ++i)
    {
        if (fread(frame_header, 1, 4, f) != 4)
            break;

        // 验证同步字 11位连续的1 (0xFFE0)
        if ((frame_header[0] == 0xFF) && ((frame_header[1] & 0xE0) == 0xE0))
        {
            uint8_t m_idx = (frame_header[1] >> 3) & 0x03;
            uint8_t l_idx = (frame_header[1] >> 1) & 0x03;
            uint8_t br_idx = (frame_header[2] >> 4) & 0x0F;
            uint8_t sr_idx = (frame_header[2] >> 2) & 0x03;
            uint8_t padding = (frame_header[2] >> 1) & 0x01;
            uint8_t ch_mode = (frame_header[3] >> 6) & 0x03;

            // 过滤无效帧或非 Layer III 帧
            if (l_idx != 1 || br_idx == 0 || br_idx == 15 || sr_idx == 3)
            {
                fseek(f, -3, SEEK_CUR);
                continue;
            }

            static const uint32_t sr_tbl[4][4] = {
                {11025, 12000, 8000, 0}, {0, 0, 0, 0}, {22050, 24000, 16000, 0}, {44100, 48000, 32000, 0}};
            sample_rate = sr_tbl[m_idx][sr_idx];

            static const uint16_t br_tbl[2][16] = {
                {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
                {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}};
            bitrate_kbps = br_tbl[(m_idx == 3) ? 1 : 0][br_idx];

            if (sample_rate > 0 && bitrate_kbps > 0)
            {
                // 计算当前帧的长度（字节）
                uint32_t frame_len = (m_idx == 3) ? (144000 * bitrate_kbps / sample_rate + padding) : (72000 * bitrate_kbps / sample_rate + padding);

                // 检查并跳过 Xing / Info 帧
                long sync_pos = ftell(f) - 4;
                uint8_t peek_buf[40] = {0};
                fread(peek_buf, 1, 40, f);

                bool is_xing_frame = false;
                for (int p = 0; p < 36; ++p)
                {
                    if (memcmp(peek_buf + p, "Xing", 4) == 0 || memcmp(peek_buf + p, "Info", 4) == 0)
                    {
                        is_xing_frame = true;
                        break;
                    }
                }

                if (is_xing_frame)
                {
                    // 如果这是 Xing/Info 帧，直接跳过整个帧大小，去读取下一帧（真实音频帧）
                    fseek(f, sync_pos + frame_len, SEEK_SET);
                    continue;
                }

                // 还原指针到帧头后面，说明找到了真实音频帧
                fseek(f, sync_pos + 4, SEEK_SET);

                if (ch_mode == 3)
                    channel_str = "mono";
                else if (ch_mode == 1 || ch_mode == 2)
                    channel_str = "stereo";

                header_found = true;
                break;
            }
        }
        fseek(f, -3, SEEK_CUR); // 如果不是同步字，逐字节往后挪动搜索
    }

    // 计算时长并输出最终信息
    if (header_found)
    {
        // (文件总大小 - ID3标签大小) * 8 / 比特率 = 时长（秒）
        double duration = ((file_total_size - mp3_data_start) * 8.0) / (bitrate_kbps * 1000.0);
        int h = (int)duration / 3600;
        int m = ((int)duration % 3600) / 60;
        double s = duration - (h * 3600) - (m * 60);

        printf("  Duration: %02d:%02d:%05.2f, start: 0.000000, bitrate: %lu kb/s\n", h, m, s, bitrate_kbps);
        printf("  Stream #0:0: Audio: mp3, %lu Hz, %s, fltp, %lu kb/s\n", sample_rate, channel_str, bitrate_kbps);
        printf("    Metadata:\n");
        printf("      encoder         : Lavc60.3\n");

        if (sample_rate != Audio_Sample_Rate)
        {
            printf("match sample rate %ldhz -> %ldhz\n", Audio_Sample_Rate, sample_rate);

            Es8311->set_iis_channel_enable(false);

            if (Es8311->set_clock_reconfig(AUDIO_MCLK_MULTIPLE, sample_rate) == false)
            {
                printf("set_clock_reconfig fail\n");
            }
            else
            {
                Audio_Sample_Rate = sample_rate;
            }

            Es8311->set_iis_channel_enable(true);
        }
    }
    else
    {
        printf("The ID3v2 header does not exist\n");
    }

    // 恢复指针到有效数据的开始
    fseek(f, mp3_data_start, SEEK_SET);
    return (size_t)mp3_data_start;
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

float get_mp3_duration_from_vbr_header(FILE *f, size_t mp3_offset)
{
    fseek(f, mp3_offset, SEEK_SET);

    uint8_t buf[576 + 256]; // 足够涵盖Xing/VBRI 位置
    size_t read_len = fread(buf, 1, sizeof(buf), f);
    if (read_len < 100)
    {
        return -1.0f;
    }

    // 找 "Xing" 或 "Info" (LAME 常用 Info 代替 Xing)
    for (size_t i = 0; i < read_len - 4; ++i)
    {
        if (memcmp(buf + i, "Xing", 4) == 0 || memcmp(buf + i, "Info", 4) == 0)
        {
            size_t pos = i + 4;

            uint32_t flags = (buf[pos] << 24) | (buf[pos + 1] << 16) | (buf[pos + 2] << 8) | buf[pos + 3];
            pos += 4;

            if (flags & 0x00000001) // Frames field present
            {
                uint32_t frames = (buf[pos] << 24) | (buf[pos + 1] << 16) | (buf[pos + 2] << 8) | buf[pos + 3];
                pos += 4;

                // MPEG1 Layer3: 1152 samples/frame, MPEG2: 576
                // 假设 MPEG1 Layer3
                uint32_t samples_per_frame = 1152;

                // 如果能从前面 frame header 拿到 version/layer 更好
                uint32_t total_samples = frames * samples_per_frame;

                return (float)total_samples / Audio_Sample_Rate;
            }
        }
        // VBRI (Fraunhofer 格式)
        else if (memcmp(buf + i, "VBRI", 4) == 0)
        {
            return -1.0f;
        }
    }

    return -1.0f; // 没找到 VBR header
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

    if (Es8311->begin() == true)
    {
        printf("Es8311->begin success\n");
    }
    else
    {
        printf("Es8311->begin fail\n");
    }

    Es8311->begin(AUDIO_MCLK_MULTIPLE, Audio_Sample_Rate, AUDIO_BITS_PER_SAMPLE);

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

                    float duration = get_mp3_duration_from_vbr_header(f_in, mp3_offset);
                    if (duration != -1.0)
                    {
                        printf("duration: %.2f s\n", duration);
                    }
                    else
                    {
                        printf("cannot determine duration\n");
                    }

                    // Seek to the start of actual MP3 data
                    fseek(f_in, mp3_offset, SEEK_SET);

                    auto read_buf = std::make_unique<uint8_t[]>(4 * 1024);
                    auto pcm_buf = std::make_unique<uint8_t[]>(8 * 1024);
                    size_t remain_bytes = 0;

                    float total_duration = -1.0f;
                    uint64_t total_pcm_bytes_sent = 0; // 累计已送出的 PCM 位元组数
                    const uint32_t bytes_per_second = Audio_Sample_Rate * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8);
                    uint32_t last_print_ms = 0;

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

                                total_pcm_bytes_sent += out_frame.decoded_size;

                                float current_time = (float)total_pcm_bytes_sent / bytes_per_second;

                                uint32_t now_ms = esp_timer_get_time() / 1000;
                                if (now_ms - last_print_ms >= 1000)
                                {
                                    last_print_ms = now_ms;

                                    printf("playing: %.2f s", current_time);

                                    if (total_duration > 0)
                                    {
                                        printf(" / %.2f s  (%.1f%%)\n", total_duration, (current_time / total_duration) * 100.0f);
                                    }
                                    else
                                    {
                                        printf("\n");
                                    }
                                }
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