/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 14:54:01
 * @LastEditTime: 2025-06-30 15:39:27
 * @License: GPL 3.0
 */
#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <numeric>
#include <functional>
#include <string.h>
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
#include "driver/i2s_std.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

#define MCU_FRAMEWORK_ESPIDF

#elif defined ARDUINO

#define MCU_FRAMEWORK_ARDUINO

#else
#error "development framework for mcu not selected"
#endif

#include "tool.h"

#define CPP_BUS_LOG_LEVEL_DEBUG
#define CPP_BUS_LOG_LEVEL_INFO
#define CPP_BUS_LOG_LEVEL_BUS
#define CPP_BUS_LOG_LEVEL_CHIP

#define DEFAULT_CPP_BUS_DRIVER_VALUE -1

#define DEFAULT_CPP_BUS_DRIVER_IIC_FREQ_HZ 100000
#define DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS 1000

#define DEFAULT_CPP_BUS_DRIVER_SPI_FREQ_HZ 10000000

#define DEFAULT_CPP_BUS_DRIVER_QSPI_FREQ_HZ 10000000
#define DEFAULT_CPP_BUS_DRIVER_QSPI_WAIT_TIMEOUT_MS 1000

#define DEFAULT_CPP_BUS_DRIVER_UART_BAUD_RATE 115200
#define DEFAULT_CPP_BUS_DRIVER_UART_WAIT_TIMEOUT_MS 1000

#define DEFAULT_CPP_BUS_DRIVER_IIS_WAIT_TIMEOUT_MS 1000

#define DEFAULT_CPP_BUS_DRIVER_SDIO_FREQ_HZ SDMMC_FREQ_DEFAULT
