/*
 * @Description: radiolib_bridge_driver
 * @Author: LILYGO_L
 * @Date: 2025-07-31 16:06:23
 * @LastEditTime: 2025-08-01 10:18:46
 * @License: GPL 3.0
 */
#pragma once

#include "TypeDef.h"
#include "Hal.h"
#include "cpp_bus_driver_library.h"

class Radiolib_Cpp_Bus_Driver_Hal : public RadioLibHal
{
public:
    std::shared_ptr<Cpp_Bus_Driver::Bus_Spi_Guide> _bus;
    int32_t _freq_hz, _cs;

    Radiolib_Cpp_Bus_Driver_Hal(std::shared_ptr<Cpp_Bus_Driver::Bus_Spi_Guide> bus, int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE)
        : RadioLibHal(static_cast<uint32_t>(Cpp_Bus_Driver::Tool::Pin_Mode::INPUT), static_cast<uint32_t>(Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT),
                      0, 1, static_cast<uint32_t>(Cpp_Bus_Driver::Tool::Interrupt_Mode::RISING), static_cast<uint32_t>(Cpp_Bus_Driver::Tool::Interrupt_Mode::FALLING)),
          _bus(bus), _freq_hz(freq_hz), _cs(cs)
    {
    }

    // implementations of pure virtual RadioLibHal methods
    void pinMode(uint32_t pin, uint32_t mode) override;
    void digitalWrite(uint32_t pin, uint32_t value) override;
    uint32_t digitalRead(uint32_t pin) override;
    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override;
    void detachInterrupt(uint32_t interruptNum) override;
    void delay(RadioLibTime_t ms) override;
    void delayMicroseconds(RadioLibTime_t us) override;
    RadioLibTime_t millis() override;
    RadioLibTime_t micros() override;
    long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override;
    void spiBegin() override;
    void spiBeginTransaction() override;
    void spiTransfer(uint8_t *out, size_t len, uint8_t *in) override;
    void spiEndTransaction() override;
    void spiEnd() override;

    // implementations of virtual RadioLibHal methods
    void init() override;
    // void term() override;
    // void tone(uint32_t pin, unsigned int frequency, RadioLibTime_t duration = 0) override;
    // void noTone(uint32_t pin) override;
    // void yield() override;
    // uint32_t pinToInterrupt(uint32_t pin) override;
};
