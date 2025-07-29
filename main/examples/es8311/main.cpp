/*
 * @Description: es8311
 * @Author: LILYGO_L
 * @Date: 2024-12-23 15:18:58
 * @LastEditTime: 2025-06-13 09:21:54
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_config.h"
#include "cpp_bus_driver_library.h"
#include "c2_b16_s44100.h"

#define MCLK_MULTIPLE i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_256
#define SAMPLE_RATE 44100

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(ES8311_SDA, ES8311_SCL, I2C_NUM_0);

auto IIC_Bus_1 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9535_SDA, XL9535_SCL, I2C_NUM_1);

auto IIS_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iis>(ES8311_ADC_DATA, ES8311_DAC_DATA, ES8311_WS_LRCK, ES8311_BCLK, ES8311_MCLK);

auto ES8311 = std::make_unique<Cpp_Bus_Driver::Es8311>(IIC_Bus_0, IIS_Bus, ES8311_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto XL9535 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_1, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

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

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9535->begin();
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);

    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

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
    ES8311->set_dac_volume(220);

    // 将ADC的数据自动输出到DAC上
    // ES8311->set_adc_data_to_dac(true);

    // 配置mclk_multiple为256
    // 配置sample_rate为44100
    // 配置data_bit_length为16
    // [2025-03-14 11:35:11.633] es8311 register[0]: 0X84
    // [2025-03-14 11:35:11.633] es8311 register[1]: 0X3F
    // [2025-03-14 11:35:11.633] es8311 register[2]: 0
    // [2025-03-14 11:35:11.633] es8311 register[3]: 0X10
    // [2025-03-14 11:35:11.633] es8311 register[4]: 0X10
    // [2025-03-14 11:35:11.633] es8311 register[5]: 0
    // [2025-03-14 11:35:11.633] es8311 register[6]: 0X3
    // [2025-03-14 11:35:11.633] es8311 register[7]: 0
    // [2025-03-14 11:35:11.633] es8311 register[8]: 0XFF
    // [2025-03-14 11:35:11.633] es8311 register[9]: 0XC
    // [2025-03-14 11:35:11.633] es8311 register[10]: 0XC
    // [2025-03-14 11:35:11.633] es8311 register[11]: 0
    // [2025-03-14 11:35:11.633] es8311 register[12]: 0X20
    // [2025-03-14 11:35:11.633] es8311 register[13]: 0X1
    // [2025-03-14 11:35:11.633] es8311 register[14]: 0XA
    // [2025-03-14 11:35:11.633] es8311 register[15]: 0
    // [2025-03-14 11:35:11.633] es8311 register[16]: 0X13
    // [2025-03-14 11:35:11.633] es8311 register[17]: 0X7C
    // [2025-03-14 11:35:11.633] es8311 register[18]: 0
    // [2025-03-14 11:35:11.633] es8311 register[19]: 0X10
    // [2025-03-14 11:35:11.633] es8311 register[20]: 0X1A
    // [2025-03-14 11:35:11.633] es8311 register[21]: 0
    // [2025-03-14 11:35:11.633] es8311 register[22]: 0X3
    // [2025-03-14 11:35:11.633] es8311 register[23]: 0XBF
    // [2025-03-14 11:35:11.633] es8311 register[24]: 0
    // [2025-03-14 11:35:11.633] es8311 register[25]: 0
    // [2025-03-14 11:35:11.633] es8311 register[26]: 0
    // [2025-03-14 11:35:11.633] es8311 register[27]: 0XC
    // [2025-03-14 11:35:11.633] es8311 register[28]: 0X6A
    // [2025-03-14 11:35:11.633] es8311 register[29]: 0
    // [2025-03-14 11:35:11.633] es8311 register[30]: 0
    // [2025-03-14 11:35:11.633] es8311 register[31]: 0
    // [2025-03-14 11:35:11.633] es8311 register[32]: 0
    // [2025-03-14 11:35:11.633] es8311 register[33]: 0
    // [2025-03-14 11:35:11.633] es8311 register[34]: 0
    // [2025-03-14 11:35:11.633] es8311 register[35]: 0
    // [2025-03-14 11:35:11.633] es8311 register[36]: 0
    // [2025-03-14 11:35:11.633] es8311 register[37]: 0
    // [2025-03-14 11:35:11.633] es8311 register[38]: 0
    // [2025-03-14 11:35:11.633] es8311 register[39]: 0
    // [2025-03-14 11:35:11.633] es8311 register[40]: 0
    // [2025-03-14 11:35:11.633] es8311 register[41]: 0
    // [2025-03-14 11:35:11.633] es8311 register[42]: 0
    // [2025-03-14 11:35:11.633] es8311 register[43]: 0
    // [2025-03-14 11:35:11.633] es8311 register[44]: 0
    // [2025-03-14 11:35:11.633] es8311 register[45]: 0
    // [2025-03-14 11:35:11.633] es8311 register[46]: 0
    // [2025-03-14 11:35:11.633] es8311 register[47]: 0
    // [2025-03-14 11:35:11.633] es8311 register[48]: 0
    // [2025-03-14 11:35:11.633] es8311 register[49]: 0
    // [2025-03-14 11:35:11.633] es8311 register[50]: 0XDC
    // [2025-03-14 11:35:11.633] es8311 register[51]: 0
    // [2025-03-14 11:35:11.633] es8311 register[52]: 0
    // [2025-03-14 11:35:11.633] es8311 register[53]: 0
    // [2025-03-14 11:35:11.633] es8311 register[54]: 0
    // [2025-03-14 11:35:11.633] es8311 register[55]: 0X8
    // [2025-03-14 11:35:11.633] es8311 register[56]: 0
    // [2025-03-14 11:35:11.633] es8311 register[57]: 0
    // [2025-03-14 11:35:11.633] es8311 register[58]: 0
    // [2025-03-14 11:35:11.633] es8311 register[59]: 0
    // [2025-03-14 11:35:11.633] es8311 register[60]: 0
    // [2025-03-14 11:35:11.633] es8311 register[61]: 0
    // [2025-03-14 11:35:11.633] es8311 register[62]: 0
    // [2025-03-14 11:35:11.633] es8311 register[63]: 0
    // [2025-03-14 11:35:11.633] es8311 register[64]: 0
    // [2025-03-14 11:35:11.633] es8311 register[65]: 0
    // [2025-03-14 11:35:11.633] es8311 register[66]: 0
    // [2025-03-14 11:35:11.633] es8311 register[67]: 0
    // [2025-03-14 11:35:11.633] es8311 register[68]: 0
    // [2025-03-14 11:35:11.633] es8311 register[69]: 0
    // [2025-03-14 11:35:11.633] es8311 register[70]: 0
    // [2025-03-14 11:35:11.633] es8311 register[71]: 0
    // [2025-03-14 11:35:11.633] es8311 register[72]: 0
    // [2025-03-14 11:35:11.633] es8311 register[73]: 0
    // [2025-03-14 11:35:11.633] es8311 register[74]: 0
    // [2025-03-14 11:35:11.633] es8311 register[75]: 0
    // [2025-03-14 11:35:11.633] es8311 register[76]: 0
    // [2025-03-14 11:35:11.633] es8311 register[77]: 0
    // [2025-03-14 11:35:11.633] es8311 register[78]: 0
    // [2025-03-14 11:35:11.633] es8311 register[79]: 0
    // [2025-03-14 11:35:11.633] es8311 register[80]: 0
    // [2025-03-14 11:35:11.633] es8311 register[81]: 0
    // [2025-03-14 11:35:11.633] es8311 register[82]: 0XFE
    // [2025-03-14 11:35:11.633] es8311 register[83]: 0
    // [2025-03-14 11:35:11.633] es8311 register[84]: 0
    // [2025-03-14 11:35:11.633] es8311 register[85]: 0
    // [2025-03-14 11:35:11.633] es8311 register[86]: 0
    // [2025-03-14 11:35:11.633] es8311 register[87]: 0
    // [2025-03-14 11:35:11.633] es8311 register[88]: 0
    // [2025-03-14 11:35:11.633] es8311 register[89]: 0
    // [2025-03-14 11:35:11.633] es8311 register[90]: 0
    // [2025-03-14 11:35:11.633] es8311 register[91]: 0
    // [2025-03-14 11:35:11.633] es8311 register[92]: 0
    // [2025-03-14 11:35:11.633] es8311 register[93]: 0
    // [2025-03-14 11:35:11.633] es8311 register[94]: 0
    // [2025-03-14 11:35:11.633] es8311 register[95]: 0
    // [2025-03-14 11:35:11.633] es8311 register[96]: 0
    // [2025-03-14 11:35:11.633] es8311 register[97]: 0
    // [2025-03-14 11:35:11.633] es8311 register[98]: 0
    // [2025-03-14 11:35:11.633] es8311 register[99]: 0
    // [2025-03-14 11:35:11.633] es8311 register[100]: 0
    // [2025-03-14 11:35:11.633] es8311 register[101]: 0
    // [2025-03-14 11:35:11.633] es8311 register[102]: 0
    // [2025-03-14 11:35:11.633] es8311 register[103]: 0
    // [2025-03-14 11:35:11.633] es8311 register[104]: 0
    // [2025-03-14 11:35:11.633] es8311 register[105]: 0
    // [2025-03-14 11:35:11.633] es8311 register[106]: 0
    // [2025-03-14 11:35:11.633] es8311 register[107]: 0
    // [2025-03-14 11:35:11.633] es8311 register[108]: 0
    // [2025-03-14 11:35:11.633] es8311 register[109]: 0
    // [2025-03-14 11:35:11.633] es8311 register[110]: 0
    // [2025-03-14 11:35:11.633] es8311 register[111]: 0
    // [2025-03-14 11:35:11.633] es8311 register[112]: 0
    // [2025-03-14 11:35:11.633] es8311 register[113]: 0
    // [2025-03-14 11:35:11.633] es8311 register[114]: 0
    // [2025-03-14 11:35:11.633] es8311 register[115]: 0
    // [2025-03-14 11:35:11.633] es8311 register[116]: 0
    // [2025-03-14 11:35:11.633] es8311 register[117]: 0
    // [2025-03-14 11:35:11.633] es8311 register[118]: 0
    // [2025-03-14 11:35:11.633] es8311 register[119]: 0
    // [2025-03-14 11:35:11.633] es8311 register[120]: 0
    // [2025-03-14 11:35:11.633] es8311 register[121]: 0
    // [2025-03-14 11:35:11.633] es8311 register[122]: 0
    // [2025-03-14 11:35:11.633] es8311 register[123]: 0
    // [2025-03-14 11:35:11.633] es8311 register[124]: 0
    // [2025-03-14 11:35:11.633] es8311 register[125]: 0
    // [2025-03-14 11:35:11.633] es8311 register[126]: 0
    // [2025-03-14 11:35:11.633] es8311 register[127]: 0
    // [2025-03-14 11:35:11.633] es8311 register[128]: 0
    // [2025-03-14 11:35:11.633] es8311 register[129]: 0
    // [2025-03-14 11:35:11.633] es8311 register[130]: 0
    // [2025-03-14 11:35:11.633] es8311 register[131]: 0
    // [2025-03-14 11:35:11.633] es8311 register[132]: 0
    // [2025-03-14 11:35:11.633] es8311 register[133]: 0
    // [2025-03-14 11:35:11.633] es8311 register[134]: 0
    // [2025-03-14 11:35:11.633] es8311 register[135]: 0
    // [2025-03-14 11:35:11.633] es8311 register[136]: 0
    // [2025-03-14 11:35:11.633] es8311 register[137]: 0
    // [2025-03-14 11:35:11.633] es8311 register[138]: 0
    // [2025-03-14 11:35:11.633] es8311 register[139]: 0
    // [2025-03-14 11:35:11.633] es8311 register[140]: 0
    // [2025-03-14 11:35:11.633] es8311 register[141]: 0
    // [2025-03-14 11:35:11.633] es8311 register[142]: 0
    // [2025-03-14 11:35:11.633] es8311 register[143]: 0
    // [2025-03-14 11:35:11.633] es8311 register[144]: 0
    // [2025-03-14 11:35:11.633] es8311 register[145]: 0
    // [2025-03-14 11:35:11.633] es8311 register[146]: 0
    // [2025-03-14 11:35:11.633] es8311 register[147]: 0
    // [2025-03-14 11:35:11.633] es8311 register[148]: 0
    // [2025-03-14 11:35:11.633] es8311 register[149]: 0
    // [2025-03-14 11:35:11.633] es8311 register[150]: 0
    // [2025-03-14 11:35:11.633] es8311 register[151]: 0
    // [2025-03-14 11:35:11.633] es8311 register[152]: 0
    // [2025-03-14 11:35:11.633] es8311 register[153]: 0
    // [2025-03-14 11:35:11.633] es8311 register[154]: 0
    // [2025-03-14 11:35:11.633] es8311 register[155]: 0
    // [2025-03-14 11:35:11.633] es8311 register[156]: 0
    // [2025-03-14 11:35:11.633] es8311 register[157]: 0
    // [2025-03-14 11:35:11.633] es8311 register[158]: 0
    // [2025-03-14 11:35:11.633] es8311 register[159]: 0
    // [2025-03-14 11:35:11.633] es8311 register[160]: 0
    // [2025-03-14 11:35:11.633] es8311 register[161]: 0
    // [2025-03-14 11:35:11.633] es8311 register[162]: 0
    // [2025-03-14 11:35:11.633] es8311 register[163]: 0
    // [2025-03-14 11:35:11.633] es8311 register[164]: 0
    // [2025-03-14 11:35:11.633] es8311 register[165]: 0
    // [2025-03-14 11:35:11.633] es8311 register[166]: 0
    // [2025-03-14 11:35:11.633] es8311 register[167]: 0
    // [2025-03-14 11:35:11.633] es8311 register[168]: 0
    // [2025-03-14 11:35:11.633] es8311 register[169]: 0
    // [2025-03-14 11:35:11.633] es8311 register[170]: 0
    // [2025-03-14 11:35:11.633] es8311 register[171]: 0
    // [2025-03-14 11:35:11.633] es8311 register[172]: 0
    // [2025-03-14 11:35:11.633] es8311 register[173]: 0
    // [2025-03-14 11:35:11.633] es8311 register[174]: 0
    // [2025-03-14 11:35:11.633] es8311 register[175]: 0
    // [2025-03-14 11:35:11.633] es8311 register[176]: 0
    // [2025-03-14 11:35:11.633] es8311 register[177]: 0
    // [2025-03-14 11:35:11.633] es8311 register[178]: 0
    // [2025-03-14 11:35:11.633] es8311 register[179]: 0
    // [2025-03-14 11:35:11.633] es8311 register[180]: 0
    // [2025-03-14 11:35:11.633] es8311 register[181]: 0
    // [2025-03-14 11:35:11.633] es8311 register[182]: 0
    // [2025-03-14 11:35:11.633] es8311 register[183]: 0
    // [2025-03-14 11:35:11.633] es8311 register[184]: 0
    // [2025-03-14 11:35:11.633] es8311 register[185]: 0
    // [2025-03-14 11:35:11.633] es8311 register[186]: 0
    // [2025-03-14 11:35:11.633] es8311 register[187]: 0
    // [2025-03-14 11:35:11.633] es8311 register[188]: 0
    // [2025-03-14 11:35:11.633] es8311 register[189]: 0
    // [2025-03-14 11:35:11.633] es8311 register[190]: 0
    // [2025-03-14 11:35:11.633] es8311 register[191]: 0
    // [2025-03-14 11:35:11.633] es8311 register[192]: 0
    // [2025-03-14 11:35:11.633] es8311 register[193]: 0
    // [2025-03-14 11:35:11.633] es8311 register[194]: 0
    // [2025-03-14 11:35:11.633] es8311 register[195]: 0
    // [2025-03-14 11:35:11.633] es8311 register[196]: 0
    // [2025-03-14 11:35:11.633] es8311 register[197]: 0
    // [2025-03-14 11:35:11.633] es8311 register[198]: 0
    // [2025-03-14 11:35:11.633] es8311 register[199]: 0
    // [2025-03-14 11:35:11.633] es8311 register[200]: 0
    // [2025-03-14 11:35:11.633] es8311 register[201]: 0
    // [2025-03-14 11:35:11.633] es8311 register[202]: 0
    // [2025-03-14 11:35:11.633] es8311 register[203]: 0
    // [2025-03-14 11:35:11.633] es8311 register[204]: 0
    // [2025-03-14 11:35:11.633] es8311 register[205]: 0
    // [2025-03-14 11:35:11.633] es8311 register[206]: 0
    // [2025-03-14 11:35:11.633] es8311 register[207]: 0
    // [2025-03-14 11:35:11.633] es8311 register[208]: 0
    // [2025-03-14 11:35:11.633] es8311 register[209]: 0
    // [2025-03-14 11:35:11.633] es8311 register[210]: 0
    // [2025-03-14 11:35:11.633] es8311 register[211]: 0
    // [2025-03-14 11:35:11.633] es8311 register[212]: 0
    // [2025-03-14 11:35:11.633] es8311 register[213]: 0
    // [2025-03-14 11:35:11.633] es8311 register[214]: 0
    // [2025-03-14 11:35:11.633] es8311 register[215]: 0
    // [2025-03-14 11:35:11.633] es8311 register[216]: 0
    // [2025-03-14 11:35:11.633] es8311 register[217]: 0
    // [2025-03-14 11:35:11.633] es8311 register[218]: 0
    // [2025-03-14 11:35:11.633] es8311 register[219]: 0
    // [2025-03-14 11:35:11.633] es8311 register[220]: 0
    // [2025-03-14 11:35:11.633] es8311 register[221]: 0
    // [2025-03-14 11:35:11.633] es8311 register[222]: 0
    // [2025-03-14 11:35:11.633] es8311 register[223]: 0
    // [2025-03-14 11:35:11.633] es8311 register[224]: 0
    // [2025-03-14 11:35:11.633] es8311 register[225]: 0
    // [2025-03-14 11:35:11.633] es8311 register[226]: 0
    // [2025-03-14 11:35:11.633] es8311 register[227]: 0
    // [2025-03-14 11:35:11.633] es8311 register[228]: 0
    // [2025-03-14 11:35:11.633] es8311 register[229]: 0
    // [2025-03-14 11:35:11.633] es8311 register[230]: 0
    // [2025-03-14 11:35:11.633] es8311 register[231]: 0
    // [2025-03-14 11:35:11.633] es8311 register[232]: 0
    // [2025-03-14 11:35:11.633] es8311 register[233]: 0
    // [2025-03-14 11:35:11.633] es8311 register[234]: 0
    // [2025-03-14 11:35:11.633] es8311 register[235]: 0
    // [2025-03-14 11:35:11.633] es8311 register[236]: 0
    // [2025-03-14 11:35:11.633] es8311 register[237]: 0
    // [2025-03-14 11:35:11.633] es8311 register[238]: 0
    // [2025-03-14 11:35:11.633] es8311 register[239]: 0
    // [2025-03-14 11:35:11.633] es8311 register[240]: 0
    // [2025-03-14 11:35:11.633] es8311 register[241]: 0
    // [2025-03-14 11:35:11.633] es8311 register[242]: 0
    // [2025-03-14 11:35:11.633] es8311 register[243]: 0
    // [2025-03-14 11:35:11.633] es8311 register[244]: 0
    // [2025-03-14 11:35:11.633] es8311 register[245]: 0
    // [2025-03-14 11:35:11.633] es8311 register[246]: 0
    // [2025-03-14 11:35:11.633] es8311 register[247]: 0
    // [2025-03-14 11:35:11.633] es8311 register[248]: 0
    // [2025-03-14 11:35:11.633] es8311 register[249]: 0
    // [2025-03-14 11:35:11.633] es8311 register[250]: 0
    // [2025-03-14 11:35:11.633] es8311 register[251]: 0
    // [2025-03-14 11:35:11.633] es8311 register[252]: 0X70
    // [2025-03-14 11:35:11.633] es8311 register[253]: 0X83
    // [2025-03-14 11:35:11.633] es8311 register[254]: 0X11
    // [2025-03-14 11:35:11.633] es8311 register[255]: 0X1

    // // 打印所有寄存器
    // uint8_t buffer = 0;
    // for (size_t i = 0; i < 256; i++)
    // {
    //     IIC_Bus_0->Bus_Iic_Guide::read(i, &buffer);
    //     printf("es8311 register[%d]: %#X\n", i, buffer);
    // }

    XL9535->Tool::pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT, Cpp_Bus_Driver::Tool::Pin_Status::PULLUP);

    size_t play_count = 1;

    // 播放音乐测试
    ES8311->write_data(c2_b16_s44100, sizeof(c2_b16_s44100));

    while (1)
    {
        // Iic_Scan();
        // vTaskDelay(pdMS_TO_TICKS(1000));

        // ADC和DAC相互回环测试
        // size_t data_lenght = 2048;
        // std::shared_ptr<uint16_t[]> data = std::make_shared<uint16_t[]>(data_lenght);
        // if (ES8311->read_data(data.get(), data_lenght * sizeof(uint16_t)) > 0)
        // {
        //     // for (uint8_t i = 0; i < 10; i++)
        //     // {
        //     //     printf("read_data: %d\n", data[i]);
        //     // }

        //     ES8311->write_data(data.get(), data_lenght * sizeof(uint16_t));
        // }

        if (XL9535->Tool::pin_read(ESP32P4_BOOT) == 0)
        {
            Iic_Scan();
            uint8_t buffer = 0;
            for (size_t i = 0; i < 256; i++)
            {
                IIC_Bus_0->Bus_Iic_Guide::read(i, &buffer);
                printf("es8311 register[%d]: %#X\n", i, buffer);
            }

            play_count++;
            printf("play_count: %d\n", play_count);

            // 播放音乐测试
            ES8311->write_data(c2_b16_s44100, sizeof(c2_b16_s44100));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
