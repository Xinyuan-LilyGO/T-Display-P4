/*
 * @Description: es8311_sd_wav
 * @Author: LILYGO_L
 * @Date: 2025-03-31 15:23:33
 * @LastEditTime: 2026-03-17 09:54:00
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include <fstream>

#define MUSIC_FILE_PATH SD_BASE_PATH "/music.wav"

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define NUM_CHANNEL 2

// WAV 文件头结构体
struct Wav_Header
{
    char riff_header[4];      // "RIFF" 标记，表示这是一个 RIFF 文件
    uint32_t riff_size;       // 整个 RIFF 块的大小，不包括 "RIFF" 标记和 riff_size 本身 (文件大小 - 8)
    char wave_header[4];      // "WAVE" 标记，表示这是一个 WAVE 文件
    char fmt_header[4];       // "fmt " 标记，表示这是格式块
    uint32_t fmt_chunk_size;  // 格式块的大小，通常是 16 (PCM) 或 18/40 (有附加信息)
    uint16_t audio_format;    // 音频格式，1 表示 PCM (未压缩)，其他值表示压缩格式
    uint16_t num_channel;     // 声道数，1 表示单声道，2 表示立体声
    uint32_t sample_rate;     // 采样率，例如 44100 Hz, 48000 Hz
    uint32_t byte_rate;       // 字节率，每秒的字节数 (sample_rate * num_channel * bits_per_sample / 8)
    uint16_t block_align;     // 块对齐，每个采样需要的字节数 (num_channel * bits_per_sample / 8)
    uint16_t bits_per_sample; // 位深度，每个采样的位数，例如 8, 16, 24, 32
    char data_header[4];      // "data" 标记，表示这是数据块
    uint32_t data_size;       // 数据块的大小，即音频数据的字节数
};

auto Es8311_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_0);
auto Xl9535_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_1);

auto Es8311_Iis_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK,
                                                                     i2s_port_t::I2S_NUM_0, Cpp_Bus_Driver::Hardware_Iis::Data_Mode::INPUT_OUTPUT, Cpp_Bus_Driver::Hardware_Iis::Iis_Mode::STD,
                                                                     i2s_clock_src_t::I2S_CLK_SRC_APLL);

auto Es8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(Es8311_Iic_Bus, Es8311_Iis_Bus, ES8311_IIC_ADDRESS);
auto Xl9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9535_Iic_Bus, XL9535_IIC_ADDRESS);

void Iic_Scan(void)
{
    std::vector<uint8_t> address;
    if (Es8311_Iic_Bus->scan_7bit_address(&address) == true)
    {
        for (size_t i = 0; i < address.size(); i++)
        {
            printf("discovered iic devices[%u]: %#x\n", i, address[i]);
        }
    }
    else
    {
        printf("No IIC device found\n");
    }
}

bool Play_Wav_File(const char *file_path)
{
    std::ifstream file(file_path, std::ios::binary);

    if (file.is_open() == false)
    {
        printf("failed to open wav file: %s\n", file_path);
        return false;
    }

    Wav_Header wav_header;
    if (!file.read(reinterpret_cast<char *>(&wav_header), sizeof(wav_header)))
    {
        printf("failed to read wav header\n");
        file.close();
        return false;
    }

    // 分别检查 WAV 文件头的每个部分
    if (strncmp(wav_header.riff_header, "RIFF", 4) != 0)
    {
        printf("invalid wav file format: riff_header is not 'RIFF'\n");
        // file.close();
        // return false;
    }
    else if (strncmp(wav_header.wave_header, "WAVE", 4) != 0)
    {
        printf("invalid wav file format: wave_header is not 'WAVE'\n");
        // file.close();
        // return false;
    }
    else if (strncmp(wav_header.fmt_header, "fmt ", 4) != 0)
    {
        printf("invalid wav file format: fmt_header is not 'fmt '\n");
        // file.close();
        // return false;
    }
    else if (strncmp(wav_header.data_header, "data", 4) != 0)
    {
        printf("invalid wav file format: data_header is not 'data'\n");
        // file.close();
        // return false;
    }

    printf("sample rate: %ld\n", wav_header.sample_rate);
    printf("channels: %d\n", wav_header.num_channel);
    printf("bits per sample: %d\n", wav_header.bits_per_sample);

    // 检查采样率、通道数和位深度是否与 I2S 配置匹配 (如果使用 I2S)
    if (wav_header.sample_rate != SAMPLE_RATE ||
        wav_header.num_channel != NUM_CHANNEL ||
        wav_header.bits_per_sample != BITS_PER_SAMPLE)
    {
        printf("wav file parameters do not match i2s configuration audio may not play correctly\n");
        file.close();
        return false;
    }

    // 计算播放时间
    double duration = 0.0;
    if (wav_header.sample_rate > 0 && wav_header.num_channel > 0 && wav_header.bits_per_sample > 0)
    {
        duration = static_cast<double>(wav_header.data_size) / (wav_header.sample_rate * wav_header.num_channel * (wav_header.bits_per_sample / 8.0));
    }

    printf("duration: %.2f s\n", duration);

    // 读取并播放音频数据
    std::unique_ptr<char[]> data_buffer = std::make_unique<char[]>(1024);

    if (data_buffer == nullptr)
    {
        printf("failed to allocate memory for audio buffer\n");
        file.close();
        return false;
    }

    while (file.good())
    {
        file.read(data_buffer.get(), 1024);
        std::streamsize bytes_read = file.gcount(); // 获取实际读取的字节数

        if (bytes_read > 0)
        {
            Es8311->write_data(data_buffer.get(), bytes_read); // 这一行需要根据你的 I2S 驱动实现来修改
        }
        // else
        // {
        //     break; // 结束循环，如果读取的字节数为 0
        // }
    }

    file.close();
    return true;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    Xl9535->begin();
    Xl9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    Xl9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    Xl9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Xl9535->pin_mode(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9535->pin_write(XL9535_SD_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    Lilygo_Device_Driver::Init_Ldo_Channel_Power(4, 3300);

    vTaskDelay(pdMS_TO_TICKS(100));

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
    Es8311->set_dac_volume(191);

    // 将ADC的数据自动输出到DAC上
    // Es8311->set_adc_data_to_dac(true);

    if (Lilygo_Device_Driver::Sdmmc_Init(SD_BASE_PATH) == false)
    {
        printf("Sdmmc_Init fail\n");
    }

    // if (Lilygo_Device_Driver::Sdspi_Init(SD_BASE_PATH) == false)
    // {
    //     printf("Sdspi_Init fail\n");
    // }

    if (Play_Wav_File(MUSIC_FILE_PATH) == false)
    {
        printf("Play_Wav_File fail\n");
    }
    else
    {
        printf("Play_Wav_File complete\n");
    }

    while (1)
    {
        // Iic_Scan();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
