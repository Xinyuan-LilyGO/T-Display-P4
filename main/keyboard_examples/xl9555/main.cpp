/*
 * @Description: xl9535
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2026-03-26 15:02:26
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"
#include "cpp_bus_driver_library.h"

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
#include "kode_bq25896.h"
#endif

volatile bool Interrupt_Flag = false;

auto Xl9555_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

#if defined CONFIG_BOARD_VERSION_T_DISPLAY_P4_V1_1
auto Bq25896_Dev = std::make_shared<Kode_Bq25896::bq25896_dev_t>();
Kode_Bq25896::bq25896_handle_t Bq25896_Handle = Bq25896_Dev.get();

auto Bq25896_Iic_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(BQ25896_SDA, BQ25896_SCL, I2C_NUM_0);
#endif

auto Xl9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(Xl9555_Iic_Bus, XL9555_IIC_ADDRESS);

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

    // Xl9555->create_gpio_interrupt(XL9535_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING,
    //                               [](void *arg) -> IRAM_ATTR void
    //                               {
    //                                   //   if ((uint32_t)arg == XL9535_INT)
    //                                   //   {
    //                                   Interrupt_Flag = true;
    //                                   //   }
    //                               });

    Xl9555->begin();
    Xl9555->pin_mode(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    Xl9555->pin_mode(Cpp_Bus_Driver::Xl95x5::Pin::IO7, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);

    Xl9555->clear_irq_flag();

    while (1)
    {
        // if (interrupt_flag == true)
        // {
        //     printf("XL9555 Interrupt_Flag triggered\n");

        //     Xl9555->clear_irq_flag();
        //     Interrupt_Flag = false;
        // }

        Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
        printf("XL9555 IO3: %d\n", Xl9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO7));
        vTaskDelay(pdMS_TO_TICKS(1000));
        Xl9555->pin_write(XL9555_LED_1, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        Xl9555->pin_write(XL9555_LED_2, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        Xl9555->pin_write(XL9555_LED_3, Cpp_Bus_Driver::Xl95x5::Value::LOW);
        printf("XL9555 IO3: %d\n", Xl9555->pin_read(Cpp_Bus_Driver::Xl95x5::Pin::IO7));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
