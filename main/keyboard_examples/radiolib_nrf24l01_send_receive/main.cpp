/*
 * @Description: radiolib_nrf24l01_send_receive
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-26 15:02:06
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"
#include "radiolib_bridge_driver.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
#include "kode_bq25896.h"
#endif

// uint8_t Send_Package[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

// 定义 const 数组并直接填入字符序列
const uint8_t Send_Package[] =
    {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

        'a', 'b', 'c', 'd', 'e', 'f', 'g'};

volatile bool Interrupt_Flag = false;

auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto Nrf24l01_Spi_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(T_MIXRF_NRF24L01_MOSI, T_MIXRF_NRF24L01_SCLK, T_MIXRF_NRF24L01_MISO, SPI2_HOST, 0);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);

RadioLibHal *Radiolib_Hal = new Radiolib_Cpp_Bus_Driver_Hal(Nrf24l01_Spi_Bus, 10000000, T_MIXRF_NRF24L01_CS);
nRF24 Nrf24l01 = new Module(Radiolib_Hal, static_cast<uint32_t>(RADIOLIB_NC),
                            static_cast<uint32_t>(T_MIXRF_NRF24L01_INT), static_cast<uint32_t>(T_MIXRF_NRF24L01_CE), static_cast<uint32_t>(RADIOLIB_NC));

auto Esp32p4 = std::make_unique<Cpp_Bus_Driver::Tool>();

extern "C" void app_main(void)
{
    printf("Ciallo\n");

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
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

    Xl9555_Iic_Bus->set_bus_handle(Bq25896_Iic_Bus->get_bus_handle());
#endif

    Xl9555->begin();
    Xl9555->pin_mode(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_write(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(10));

    Esp32p4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);

    Esp32p4->pin_mode(T_MIXRF_CC1101_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    Esp32p4->pin_mode(T_MIXRF_NRF24L01_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    Esp32p4->pin_mode(T_MIXRF_ST25R3916_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    Esp32p4->pin_write(T_MIXRF_CC1101_CS, 1);
    Esp32p4->pin_write(T_MIXRF_NRF24L01_CS, 1);
    Esp32p4->pin_write(T_MIXRF_ST25R3916_CS, 1);

    int16_t status = Nrf24l01.begin(2400.0, 1000.0, 0);
    if (status == RADIOLIB_ERR_NONE)
    {
        printf("nrf24l01 init success\n");
    }
    else
    {
        printf("nrf24l01 init fail (error code: %d)\n", status);
    }

    // 中断需要放在Nrf24l01.begin后面
    Esp32p4->create_gpio_interrupt(T_MIXRF_NRF24L01_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Interrupt_Flag = true;
                                   });

    uint8_t addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
    status = Nrf24l01.setTransmitPipe(addr);
    if (status != RADIOLIB_ERR_NONE)
    {
        printf("nrf24l01 setTransmitPipe fail (error code: %d)\n", status);
    }

    Nrf24l01.startReceive();

    // 连续波测试
    // Nrf24l01.transmitDirect(2400.0);

    while (1)
    {
        if (Esp32p4->pin_read(ESP32P4_BOOT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));

            printf("T_MIXRF_NRF24L01 send package\n");

            status = Nrf24l01.transmit(Send_Package, 32, 0);
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("transmit fail (error code: %d)\n", status);
            }

            status = Nrf24l01.startReceive();
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("startReceive fail (error code: %d)\n", status);
            }

            Interrupt_Flag = false;
        }

        if (Interrupt_Flag == true) // 接收完成中断
        {
            uint8_t receive_package[255] = {0};
            if (Nrf24l01.readData(receive_package, 9) == RADIOLIB_ERR_NONE)
            {
                for (uint8_t i = 0; i < 9; i++)
                {
                    printf("get T_MIXRF_NRF24L01 data[%d]: %d\n", i, receive_package[i]);
                }
            }

            status = Nrf24l01.startReceive();
            if (status != RADIOLIB_ERR_NONE)
            {
                printf("startReceive fail (error code: %d)\n", status);
            }

            Interrupt_Flag = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
