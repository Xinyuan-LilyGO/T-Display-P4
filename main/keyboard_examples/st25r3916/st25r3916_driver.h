/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-08-09 11:13:53
 * @LastEditTime: 2026-04-27 15:54:56
 * @License: GPL 3.0
 */
#pragma once
#include "rfal_rfst25r3916.h"

void St25r3916_Init(bool bus_init_flag = false);
void St25r3916_Loop(void);

extern RfalRfST25R3916Class rfst25r3916;