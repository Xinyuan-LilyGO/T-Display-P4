/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-07-31 16:06:23
 * @LastEditTime: 2025-08-04 13:53:54
 * @License: GPL 3.0
 */
#include "radiolib_bridge_driver.h"

void (*Global_Interrupt_Callback)(void) = nullptr;

void IRAM_ATTR Interrupt_Callback(void *arg)
{
    if (Global_Interrupt_Callback != nullptr)
    {
        Global_Interrupt_Callback();
    }
}

void inline Radiolib_Cpp_Bus_Driver_Hal::pinMode(uint32_t pin, uint32_t mode)
{
    if (pin == RADIOLIB_NC)
    {
        return;
    }
    _bus->pin_mode(pin, static_cast<Cpp_Bus_Driver::Tool::Pin_Mode>(mode));
}

void inline Radiolib_Cpp_Bus_Driver_Hal::digitalWrite(uint32_t pin, uint32_t value)
{
    if (pin == RADIOLIB_NC)
    {
        return;
    }
    _bus->pin_write(pin, value);
}

uint32_t inline Radiolib_Cpp_Bus_Driver_Hal::digitalRead(uint32_t pin)
{
    if (pin == RADIOLIB_NC)
    {
        return 0;
    }
    return _bus->pin_read(pin);
}

void inline Radiolib_Cpp_Bus_Driver_Hal::attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode)
{
    if (interruptNum == RADIOLIB_NC)
    {
        return;
    }

    Global_Interrupt_Callback = interruptCb;

    // 使用静态函数作为回调函数
    _bus->create_gpio_interrupt(interruptNum, static_cast<Cpp_Bus_Driver::Tool::Interrupt_Mode>(mode), Interrupt_Callback);
}

void inline Radiolib_Cpp_Bus_Driver_Hal::detachInterrupt(uint32_t interruptNum)
{
    if (interruptNum == RADIOLIB_NC)
    {
        return;
    }
    _bus->delete_gpio_interrupt(interruptNum);
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
        return (0);
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
    _bus->begin(_freq_hz, _cs);
}

void inline Radiolib_Cpp_Bus_Driver_Hal::spiBeginTransaction()
{
    return;
}

void Radiolib_Cpp_Bus_Driver_Hal::spiTransfer(uint8_t *out, size_t len, uint8_t *in)
{
    _bus->write_read(out, in, len);
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
    _bus->begin(_freq_hz, _cs);
}

void inline Radiolib_Cpp_Bus_Driver_Hal::yield()
{
    _bus->delay_ms(10);
}
