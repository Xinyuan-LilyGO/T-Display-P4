/*
 * @Description: radiolib_lr2021_send_receive
 * @Author: LILYGO_L
 * @Date: 2026-05-14
 * @License: GPL 3.0
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "RadioLib.h"
#include "radiolib_bridge_driver.h"

#ifndef LR2021_CS
#define LR2021_CS SX1262_CS
#endif

#ifndef LR2021_BUSY
#define LR2021_BUSY SX1262_BUSY
#endif

#ifndef LR2021_SCLK
#define LR2021_SCLK SX1262_SCLK
#endif

#ifndef LR2021_MOSI
#define LR2021_MOSI SX1262_MOSI
#endif

#ifndef LR2021_MISO
#define LR2021_MISO SX1262_MISO
#endif

#ifndef XL9535_LR2021_RST
#define XL9535_LR2021_RST Cpp_Bus_Driver::Xl95x5::Pin::IO16
#endif

#ifndef XL9535_LR2021_DIO1
#define XL9535_LR2021_DIO1 Cpp_Bus_Driver::Xl95x5::Pin::IO17
#endif

static uint8_t send_package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
static uint8_t receive_package[255] = {0};
static constexpr float lr2021_freq_mhz = 915.0f;
// 工作在 2.4 G ，建议配置输出功率参数“ #define c_HF_power  8 或 9 ”，
// 此时，在模块内部，LR2021 RFO_HF 输出功率为 0 dBm ，ANT 脚功率为 + 20 dBm 。
// 如果配置输出功率参数 > 9 ，ANT 脚功率为 + 20 dBm ，但总电流会增加。
// 禁止配置输出功率参数 > 12，此时，在模块内部，LR2021 RFO_HF 输出功率极可能 > +5 dBm ，
// 可能会损坏 PCBA 内部 FEM 芯片
static constexpr bool kDirectRfCableTest = false;
static constexpr int8_t lr2021_output_power_dbm = kDirectRfCableTest ? ((lr2021_freq_mhz >= 1000.0f) ? -19 : -9) : ((lr2021_freq_mhz >= 1000.0f) ? 8 : 22);
static constexpr uint32_t kGpioLow = 0;
static constexpr uint32_t kGpioHigh = 1;
static constexpr float kInvalidRssiThresholdDbm = -200.0f;
static constexpr float kOverloadSnrThresholdDb = 10.0f;

static auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_0);
static auto SPI_Bus_2 = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(LR2021_MOSI, LR2021_SCLK, LR2021_MISO, SPI2_HOST, 0);

static auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);
static auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

static RadioLibHal *Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(SPI_Bus_2, 10000000, LR2021_CS);
static LR2021 Lr2021 = new Module(Radiolib_Hal,
                                  static_cast<uint32_t>(RADIOLIB_NC),
                                  static_cast<uint32_t>(RADIOLIB_NC),
                                  static_cast<uint32_t>(RADIOLIB_NC),
                                  LR2021_BUSY);

static const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_NC,
    RADIOLIB_LR2021_DIO6,
    RADIOLIB_LR2021_DIO7,
    RADIOLIB_NC,
    RADIOLIB_NC,
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
      // mode               DIO5   DIO6  DIO7
    {LR2021::MODE_STBY, {kGpioLow, kGpioLow, kGpioLow}},
    {LR2021::MODE_RX, {kGpioLow, kGpioLow, kGpioLow}},
    {LR2021::MODE_TX, {kGpioLow, kGpioLow, kGpioLow}},
    {LR2021::MODE_RX_HF, {kGpioLow, kGpioHigh, kGpioLow}},
    {LR2021::MODE_TX_HF, {kGpioLow, kGpioLow, kGpioHigh}},
    END_OF_MODE_TABLE,
};

static void lr2021_reset(void)
{
    XL9535->pin_mode(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_LR2021_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static float lr2021_read_rssi(float *packet_rssi, float *signal_rssi, float *instant_rssi, bool *rssi_overload)
{
    float packet = Lr2021.getRSSI();
    float signal = 0.0f;
    float instant = 0.0f;
    float snr = 0.0f;
    uint8_t packet_length = 0;
    int16_t status = Lr2021.getLoRaPacketStatus(nullptr, nullptr, &packet_length, &snr, nullptr, &signal);
    bool packet_invalid = packet < kInvalidRssiThresholdDbm;
    bool signal_invalid = signal < kInvalidRssiThresholdDbm;
    bool overload = (status == RADIOLIB_ERR_NONE) && packet_invalid && signal_invalid &&
                    (packet_length > 0) && (snr >= kOverloadSnrThresholdDb);

    if (packet_invalid)
    {
        instant = Lr2021.getRSSI(false, true);
    }

    if (packet_rssi != nullptr)
    {
        *packet_rssi = packet;
    }
    if (signal_rssi != nullptr)
    {
        *signal_rssi = (status == RADIOLIB_ERR_NONE) ? signal : 0.0f;
    }
    if (instant_rssi != nullptr)
    {
        *instant_rssi = instant;
    }
    if (rssi_overload != nullptr)
    {
        *rssi_overload = overload;
    }

    if (overload)
    {
        printf("LR2021 packet RSSI filtered: likely RX overload, packet: %.2f dBm, signal: %.2f dBm, instant(noise): %.2f dBm, snr: %.2f dB, packet len: %u\n",
               packet, signal, instant, snr, static_cast<unsigned int>(packet_length));
    }
    else if ((status == RADIOLIB_ERR_NONE) && packet_invalid)
    {
        printf("LR2021 packet RSSI invalid: %.2f dBm, signal: %.2f dBm, instant(noise): %.2f dBm, snr: %.2f dB, packet len: %u\n",
               packet, signal, instant, snr, static_cast<unsigned int>(packet_length));
    }
    else if (status != RADIOLIB_ERR_NONE)
    {
        printf("LR2021 getLoRaPacketStatus fail (error code: %d), packet rssi: %.2f dBm\n", status, packet);
    }

    if (packet >= kInvalidRssiThresholdDbm)
    {
        return packet;
    }
    if ((status == RADIOLIB_ERR_NONE) && (signal >= kInvalidRssiThresholdDbm))
    {
        return signal;
    }
    return NAN;
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");

    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_LR2021_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_mode(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(10));

    ESP32P4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);
    ESP32P4->pin_mode(LR2021_BUSY, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLDOWN);

    lr2021_reset();

    Lr2021.irqDioNum = 11;
    int16_t status = Lr2021.begin(lr2021_freq_mhz, 125.0, 12, 7,
                                  RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE,
                                  lr2021_output_power_dbm, 8, 3.3);
    if (status == RADIOLIB_ERR_NONE)
    {
        printf("LR2021 init success\n");
    }
    else
    {
        printf("LR2021 init fail (error code: %d)\n", status);
    }

    Lr2021.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

    status = Lr2021.startReceive();
    if (status == RADIOLIB_ERR_NONE)
    {
        printf("LR2021 start receive\n");
    }
    else
    {
        printf("LR2021 startReceive fail (error code: %d)\n", status);
    }

    while (1)
    {
        if (ESP32P4->pin_read(ESP32P4_BOOT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));

            printf("LR2021 send package\n");
            status = Lr2021.startTransmit(send_package, sizeof(send_package));
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("LR2021 startTransmit fail (error code: %d)\n", status);
            }
            else
            {
                uint16_t timeout_count = 0;
                while (1)
                {
                    if (XL9535->pin_read(XL9535_LR2021_DIO1) == 1)
                    {
                        status = Lr2021.finishTransmit();
                        if (status == RADIOLIB_ERR_NONE)
                        {
                            printf("LR2021 send success\n");
                        }
                        else
                        {
                            printf("LR2021 finishTransmit fail (error code: %d)\n", status);
                        }
                        break;
                    }

                    timeout_count++;
                    if (timeout_count > 1000)
                    {
                        printf("LR2021 send timeout\n");
                        Lr2021.finishTransmit();
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }

            status = Lr2021.startReceive();
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("LR2021 startReceive fail (error code: %d)\n", status);
            }
        }

        if (XL9535->pin_read(XL9535_LR2021_DIO1) == 1)
        {
            memset(receive_package, 0, sizeof(receive_package));
            size_t length = Lr2021.getPacketLength();
            if (length > sizeof(receive_package))
            {
                length = sizeof(receive_package);
            }

            status = Lr2021.readData(receive_package, length);
            if (status == RADIOLIB_ERR_NONE)
            {
                float packet_rssi = 0.0f;
                float signal_rssi = 0.0f;
                float instant_rssi = 0.0f;
                bool rssi_overload = false;
                float rssi = lr2021_read_rssi(&packet_rssi, &signal_rssi, &instant_rssi, &rssi_overload);
                if (isnan(rssi))
                {
                    printf("LR2021 rssi: filtered(%s), packet: %.2f dBm, signal: %.2f dBm, instant: %.2f dBm, snr: %.2f dB\n",
                           rssi_overload ? "overload" : "invalid",
                           packet_rssi, signal_rssi, instant_rssi, Lr2021.getSNR());
                }
                else
                {
                    printf("LR2021 rssi: %.2f dBm, packet: %.2f dBm, signal: %.2f dBm, instant: %.2f dBm, snr: %.2f dB\n",
                           rssi, packet_rssi, signal_rssi, instant_rssi, Lr2021.getSNR());
                }
                for (size_t i = 0; i < length; i++)
                {
                    printf("get LR2021 data[%d]: %d\n", static_cast<int>(i), receive_package[i]);

                }
            }
            else
            {
                printf("LR2021 readData fail (error code: %d)\n", status);
            }

            // 连续接收模式下，readData() 后 LR2021 会继续保持接收。
            // 这里再次调用 startReceive() 可能会让芯片拒绝 SetRx 命令。
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
