/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 14:54:01
 * @LastEditTime: 2024-12-24 14:36:19
 * @License: GPL 3.0
 */
#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <numeric>
#include <functional>
#include "tool.h"

#if defined CONFIG_IDF_INIT_VERSION
#include "driver/i2c_master.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"

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
