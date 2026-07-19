/*
 * @Description: es8311_sd_wav
 * @Author: LILYGO_L
 * @Date: 2025-03-31 15:23:33
 * @LastEditTime: 2026-04-25 09:39:47
 * @License: GPL 3.0
 */
#include <fstream>
#include <string>

#include "lilygo_device_driver_library.h"

namespace board = lilygo_device_driver::t_display_p4;

const std::string kMusicFilePath =
    std::string(board::device::sd::kBasePath) + "/music.wav";

// WAV 文件头结构体
struct WavHeader {
  char riff_header[4];  // "RIFF" 标记，表示这是一个 RIFF 文件
  uint32_t riff_size;   // 整个 RIFF 块的大小，不包括 "RIFF" 标记和 riff_size
                        // 本身 (文件大小 - 8)
  char wave_header[4];  // "WAVE" 标记，表示这是一个 WAVE 文件
  char fmt_header[4];   // "fmt " 标记，表示这是格式块
  uint32_t
      fmt_chunk_size;     // 格式块的大小，通常是 16 (PCM) 或 18/40 (有附加信息)
  uint16_t audio_format;  // 音频格式，1 表示 PCM (未压缩)，其他值表示压缩格式
  uint16_t num_channel;   // 声道数，1 表示单声道，2 表示立体声
  uint32_t sample_rate;   // 采样率，例如 44100 Hz, 48000 Hz
  uint32_t byte_rate;     // 字节率，每秒的字节数 (sample_rate * num_channel *
                          // bits_per_sample / 8)
  uint16_t block_align;   // 块对齐，每个采样需要的字节数 (num_channel *
                          // bits_per_sample / 8)
  uint16_t bits_per_sample;  // 位深度，每个采样的位数，例如 8, 16, 24, 32
  char data_header[4];       // "data" 标记，表示这是数据块
  uint32_t data_size;        // 数据块的大小，即音频数据的字节数
};

bool PlayWavFile(const char* file_path) {
  std::ifstream file(file_path, std::ios::binary);

  if (!file.is_open()) {
    printf("Failed to open wav file: %s\n", file_path);
    return false;
  }

  WavHeader wav_header;
  if (!file.read(reinterpret_cast<char*>(&wav_header), sizeof(wav_header))) {
    printf("Failed to read wav header\n");
    file.close();
    return false;
  }

  // 分别检查 WAV 文件头的每个部分
  if (strncmp(wav_header.riff_header, "RIFF", 4) != 0) {
    printf("Invalid wav file format: riff_header is not 'RIFF'\n");
    // file.close();
    // return false;
  } else if (strncmp(wav_header.wave_header, "WAVE", 4) != 0) {
    printf("Invalid wav file format: wave_header is not 'WAVE'\n");
    // file.close();
    // return false;
  } else if (strncmp(wav_header.fmt_header, "fmt ", 4) != 0) {
    printf("Invalid wav file format: fmt_header is not 'fmt '\n");
    // file.close();
    // return false;
  } else if (strncmp(wav_header.data_header, "data", 4) != 0) {
    printf("Invalid wav file format: data_header is not 'data'\n");
    // file.close();
    // return false;
  }

  printf("Sample rate: %ld\n", wav_header.sample_rate);
  printf("Channel: %d\n", wav_header.num_channel);
  printf("Bits per sample: %d\n", wav_header.bits_per_sample);

  // 检查采样率、通道数和位深度是否与 I2S 配置匹配 (如果使用 I2S)
  if (wav_header.sample_rate != board::device::es8311::kSampleRate ||
      wav_header.num_channel != board::device::es8311::kChannel ||
      wav_header.bits_per_sample != board::device::es8311::kBitsPerSample) {
    printf(
        "Wav file parameters do not match i2s configuration audio may not play "
        "correctly\n");
    file.close();
    return false;
  }

  // 计算播放时间
  double duration = 0.0;
  if (wav_header.sample_rate > 0 && wav_header.num_channel > 0 &&
      wav_header.bits_per_sample > 0) {
    duration = static_cast<double>(wav_header.data_size) /
               (wav_header.sample_rate * wav_header.num_channel *
                (wav_header.bits_per_sample / 8.0));
  }

  printf("Duration: %.2fs\n", duration);

  // 读取并播放音频数据
  std::unique_ptr<char[]> data_buffer = std::make_unique<char[]>(1024);

  if (data_buffer == nullptr) {
    printf("Failed to allocate memory for audio buffer\n");
    file.close();
    return false;
  }

  while (file.good()) {
    file.read(data_buffer.get(), 1024);
    std::streamsize bytes_read = file.gcount();  // 获取实际读取的字节数

    if (bytes_read > 0) {
      lilygo_device_driver::TDisplayP4Driver::GetInstance()
          .chip()
          .es8311->WriteI2s(data_buffer.get(), bytes_read);
    }
  }

  file.close();
  return true;
}

extern "C" void app_main(void) {
  printf("Ciallo\n");

  lilygo_device_driver::TDisplayP4Driver::GetInstance().Init();

  if (!PlayWavFile(kMusicFilePath.c_str())) {
    printf("PlayWavFile failed\n");
  } else {
    printf("PlayWavFile success\n");
  }

  while (1) {
    // Iic_Scan();

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
