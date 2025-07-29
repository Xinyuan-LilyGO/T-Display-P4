#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "driver/sdmmc_host.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "esp_vfs_fat.h"
#include <fstream>

#define MOUNT_POINT "/sdcard"
#define SD_FILE_PATH_MUSIC "/sdcard/music.wav"

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

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(7, 8, I2C_NUM_0);

auto IIS_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(11, 9, 10, 12, 13);

auto ES8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(IIC_Bus_0, IIS_Bus, ES8311_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

void Iic_Scan(void)
{
    std::vector<uint8_t> address;
    if (IIC_Bus_0->scan_7bit_address(&address) == true)
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
            ES8311->write_data(data_buffer.get(), bytes_read); // 这一行需要根据你的 I2S 驱动实现来修改
        }
        // else
        // {
        //     break; // 结束循环，如果读取的字节数为 0
        // }
    }

    file.close();
    return true;
}

bool Sdmmc_Init(const char *base_path)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config =
        {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,
        };

    sdmmc_card_t *card;

    printf("initializing sd card\n");
    printf("using sdmmc peripheral\n");

    sd_pwr_ctrl_ldo_config_t ldo_config =
        {
            .ldo_chan_id = 4,
        };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    int32_t assert = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (assert != ESP_OK)
    {
        printf("failed to create a new on-chip ldo power control driver\n");
        return false;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;

    // host.max_freq_khz=SDMMC_FREQ_SDR50;

    host.pwr_ctrl_handle = pwr_ctrl_handle;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.clk = static_cast<gpio_num_t>(SD_SDIO_CLK);
    slot_config.cmd = static_cast<gpio_num_t>(SD_SDIO_CMD);
    slot_config.d0 = static_cast<gpio_num_t>(SD_SDIO_D0);
    slot_config.d1 = static_cast<gpio_num_t>(SD_SDIO_D1);
    slot_config.d2 = static_cast<gpio_num_t>(SD_SDIO_D2);
    slot_config.d3 = static_cast<gpio_num_t>(SD_SDIO_D3);

    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    printf("mounting filesystem\n");

    assert = esp_vfs_fat_sdmmc_mount(base_path, &host, &slot_config, &mount_config, &card);
    if (assert != ESP_OK)
    {
        printf("failed to mount filesystem\n");
        return false;
    }

    printf("filesystem mounted\n");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return true;
}

bool Sdspi_Init(const char *base_path)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config =
        {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,
        };

    sdmmc_card_t *card;

    printf("initializing sd card\n");
    printf("using sdspi peripheral\n");

    sd_pwr_ctrl_ldo_config_t ldo_config =
        {
            .ldo_chan_id = 4,
        };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    int32_t assert = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (assert != ESP_OK)
    {
        printf("failed to create a new on-chip ldo power control driver\n");
        return false;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;

    // host.max_freq_khz=SDMMC_FREQ_SDR50;

    host.pwr_ctrl_handle = pwr_ctrl_handle;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };

    assert = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (assert != ESP_OK)
    {
        printf("failed to initialize bus\n");
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = host.slot;

    printf("mounting filesystem\n");

    assert = esp_vfs_fat_sdspi_mount(base_path, &host, &slot_config, &mount_config, &card);
    if (assert != ESP_OK)
    {
        printf("failed to mount filesystem\n");
        return false;
    }

    printf("filesystem mounted\n");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return true;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    gpio_reset_pin(GPIO_NUM_53);
    gpio_set_direction(GPIO_NUM_53, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_53, 1);

    ES8311->begin(MCLK_MULTIPLE, SAMPLE_RATE, i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT);

    while (1)
    {
        if (ES8311->begin(50000) == true)
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
    ES8311->set_dac_volume(191);

    // 将ADC的数据自动输出到DAC上
    // ES8311->set_adc_data_to_dac(true);

    if (Sdmmc_Init(SD_BASE_PATH) == false)
    {
        printf("Sdmmc_Init fail\n");
    }

    // if (Sdspi_Init(MOUNT_POINT) == false)
    // {
    //     printf("Sdspi_Init fail\n");
    // }

    if (Play_Wav_File(SD_FILE_PATH_MUSIC) == false)
    {
        printf("Play_Wav_File fail\n");
    }
    else
    {
        printf("Play_Wav_File complete\n");
    }

    // esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    // printf("card unmounted\n");

    // ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
    // if (ret != ESP_OK)
    // {
    //     printf("failed to delete the on-chip ldo power control driver\n");
    // }

    while (1)
    {
        // Iic_Scan();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
