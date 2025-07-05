/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:14:18
 * @LastEditTime: 2025-06-27 11:22:25
 * @License: GPL 3.0
 */

#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <numeric>

// #include "assert.h"

#include "./bus/bus_guide.h"
#include "./bus/iic/hardware_iic_1.h"
#include "./bus/iic/hardware_iic_2.h"
#include "./bus/spi/hardware_spi.h"
#include "./bus/spi/hardware_qspi.h"
#include "./bus/uart/hardware_uart.h"
#include "./bus/iis/hardware_iis.h"
#include "./bus/sdio/hardware_sdio.h"

#include "./chip/iic/xl95x5.h"
#include "./chip/iic/aw862xx.h"
#include "./chip/iic/sgm38121.h"
#include "./chip/iic/pcf8563x.h"
#include "./chip/iic/bq27220xxxx.h"
#include "./chip/iic/hi8561_touch.h"
#include "./chip/iic/cst2xxse.h"
#include "./chip/iic_iis/es8311.h"
#include "./chip/iic/ft3x68.h"
#include "./chip/spi/sx126x.h"
#include "./chip/spi/ecx336c.h"
#include "./chip/spi/co5300.h"
#include "./chip/uart/l76k.h"
#include "./chip/sdio/esp_at.h"



