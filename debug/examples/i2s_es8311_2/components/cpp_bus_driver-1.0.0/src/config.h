/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 14:54:01
 * @LastEditTime: 2025-02-14 11:40:59
 * @License: GPL 3.0
 */
#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <numeric>
#include <functional>
#include <string.h>
#include "tool.h"
#include <algorithm>
#include <cstring>
#include <string>

#if defined CONFIG_IDF_INIT_VERSION
#include "driver/i2c_master.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

#define MCU_FRAMEWORK_ESPIDF
#elif defined ARDUINO
#define MCU_FRAMEWORK_ARDUINO
#else
#error "Development framework for MCU not selected."
#endif

#define CPP_BUS_LOG_LEVEL_INFO
#define CPP_BUS_LOG_LEVEL_BUS
#define CPP_BUS_LOG_LEVEL_CHIP

#define DEFAULT_CPP_BUS_DRIVER_VALUE -1

#define DEFAULT_CPP_BUS_DRIVER_IIC_FREQUENCY 100000
#define DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS 50

#define DEFAULT_CPP_BUS_DRIVER_SPI_FREQUENCY 10000000

#define DEFAULT_CPP_BUS_DRIVER_UART_BAUD_RATE 115200
#define DEFAULT_CPP_BUS_DRIVER_UART_WAIT_TIMEOUT_MS 1000
