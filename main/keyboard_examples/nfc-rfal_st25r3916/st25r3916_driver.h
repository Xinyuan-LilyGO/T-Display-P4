/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-08-09 11:13:53
 * @LastEditTime: 2026-04-27 15:54:56
 * @License: GPL 3.0
 */
#pragma once
#include <memory>

namespace cpp_bus_driver {
class HardwareSpi;
}

bool St25r3916_Init(
    const std::shared_ptr<cpp_bus_driver::HardwareSpi>& spi_bus);
void St25r3916_Loop(void);
