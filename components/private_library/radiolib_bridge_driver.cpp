/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-07-31 16:06:23
 * @LastEditTime: 2025-08-08 14:32:14
 * @License: GPL 3.0
 */
#include "radiolib_bridge_driver.h"

struct Interrupt_Arg
{
    std::function<void(void)> interrupt_function;
};

void IRAM_ATTR Interrupt_Callback_Template(void *arg)
{
    Interrupt_Arg *local_arg = (Interrupt_Arg *)arg;
    if (local_arg->interrupt_function != nullptr)
    {
        local_arg->interrupt_function();
    }
}

void inline Radiolib_Cpp_Bus_Driver_Hal::pinMode(uint32_t pin, uint32_t mode)
{
    if (pin == RADIOLIB_NC)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "pinMode fail (pin == RADIOLIB_NC)\n");
        return;
    }
    _bus->pin_mode(pin, static_cast<Cpp_Bus_Driver::Tool::Pin_Mode>(mode));
}

void inline Radiolib_Cpp_Bus_Driver_Hal::digitalWrite(uint32_t pin, uint32_t value)
{
    if (pin == RADIOLIB_NC)
    {
        // _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "digitalWrite fail (pin == RADIOLIB_NC)\n");
        return;
    }
    _bus->pin_write(pin, value);
}

uint32_t inline Radiolib_Cpp_Bus_Driver_Hal::digitalRead(uint32_t pin)
{
    if (pin == RADIOLIB_NC)
    {
        // _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "digitalRead fail (pin == RADIOLIB_NC)\n");
        return 0;
    }
    return _bus->pin_read(pin);
}

void inline Radiolib_Cpp_Bus_Driver_Hal::attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode)
{
    if (interruptNum == RADIOLIB_NC)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "attachInterrupt fail (interruptNum == RADIOLIB_NC)\n");
        return;
    }

    auto interrupt_arg = std::make_unique<Interrupt_Arg>(interruptCb);

    if (_bus->create_gpio_interrupt(interruptNum, static_cast<Cpp_Bus_Driver::Tool::Interrupt_Mode>(mode),
                                    Interrupt_Callback_Template, interrupt_arg.get()) == false)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "create_gpio_interrupt fail\n");
    }
}

void inline Radiolib_Cpp_Bus_Driver_Hal::detachInterrupt(uint32_t interruptNum)
{
    if (interruptNum == RADIOLIB_NC)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "detachInterrupt fail (interruptNum == RADIOLIB_NC)\n");
        return;
    }
    if (_bus->delete_gpio_interrupt(interruptNum) == false)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "delete_gpio_interrupt fail\n");
    }
}

void inline Radiolib_Cpp_Bus_Driver_Hal::delay(RadioLibTime_t ms)
{
    _bus->delay_ms(static_cast<uint32_t>(ms));
}

void inline Radiolib_Cpp_Bus_Driver_Hal::delayMicroseconds(RadioLibTime_t us)
{
    _bus->delay_us(static_cast<uint32_t>(us));
}

RadioLibTime_t inline Radiolib_Cpp_Bus_Driver_Hal::millis()
{
    return _bus->get_system_time_ms();
}

RadioLibTime_t inline Radiolib_Cpp_Bus_Driver_Hal::micros()
{
    return _bus->get_system_time_us();
}

long inline Radiolib_Cpp_Bus_Driver_Hal::pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout)
{
    if (pin == RADIOLIB_NC)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "pulseIn fail (pin == RADIOLIB_NC)\n");
        return 0;
    }

    _bus->pin_mode(pin, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);
    uint32_t start = _bus->get_system_time_us();
    uint32_t curtick = _bus->get_system_time_us();

    while (_bus->pin_read(pin) == state)
    {
        if ((_bus->get_system_time_us() - curtick) > timeout)
        {
            return 0;
        }
    }

    return (_bus->get_system_time_us() - start);
}

void inline Radiolib_Cpp_Bus_Driver_Hal::spiBegin()
{
    if (_bus->begin(_freq_hz, _cs) == false)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
    }
}

void inline Radiolib_Cpp_Bus_Driver_Hal::spiBeginTransaction()
{
    return;
}

void Radiolib_Cpp_Bus_Driver_Hal::spiTransfer(uint8_t *out, size_t len, uint8_t *in)
{
    if (_bus->write_read(out, in, len))
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "write_read fail\n");
    }
}

void inline Radiolib_Cpp_Bus_Driver_Hal::spiEndTransaction()
{
    return;
}

void inline Radiolib_Cpp_Bus_Driver_Hal::spiEnd()
{
    return;
}

void Radiolib_Cpp_Bus_Driver_Hal::init()
{
    if (_bus->begin(_freq_hz, _cs) == false)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
    }
}

void inline Radiolib_Cpp_Bus_Driver_Hal::yield()
{
    _bus->delay_ms(10);
}
