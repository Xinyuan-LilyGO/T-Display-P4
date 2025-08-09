/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-08-09 11:13:53
 * @LastEditTime: 2025-08-09 16:58:08
 * @License: GPL 3.0
 */
#pragma once

#include "t_display_p4_keyboard_config.h"

#include "rfal_rfst25r3916.h"

/* Uncomment this line if you want to use the NFC reader with I2C bus instead of SPI */
// #define I2C_ENABLED

#ifndef I2C_ENABLED
#define ST25R3916_SPI_MOSI T_MIXRF_ST25R3916_MOSI
#define ST25R3916_SPI_MISO T_MIXRF_ST25R3916_MISO
#define ST25R3916_SPI_SCLK T_MIXRF_ST25R3916_SCLK
#endif

#define CS_PIN T_MIXRF_ST25R3916_CS
#define IRQ_PIN -1

#define LED_A_PIN -1
#define LED_B_PIN -1
#define LED_F_PIN -1
#define LED_V_PIN -1
#define LED_AP2P_PIN -1
#define LED_FIELD_PIN -1

#define USER_BTN ESP32P4_BOOT

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT 0         /*!< Demo State:  Not initialized */
#define DEMO_ST_START_DISCOVERY 1 /*!< Demo State:  Start Discovery */
#define DEMO_ST_DISCOVERY 2       /*!< Demo State:  Discovery       */

#define NDEF_DEMO_READ 0U       /*!< NDEF menu read               */
#define NDEF_DEMO_WRITE_MSG1 1U /*!< NDEF menu write 1 record     */
#define NDEF_DEMO_WRITE_MSG2 2U /*!< NDEF menu write 2 records    */
#define NDEF_DEMO_FORMAT_TAG 3U /*!< NDEF menu format tag         */

#define NDEF_DEMO_MAX_FEATURES 4U /*!< Number of menu items         */

#define NDEF_WRITE_FORMAT_TIMEOUT 10000U /*!< When write or format mode is selected, demo returns back to read mode after a timeout */
#define NDEF_LED_BLINK_DURATION 250U     /*!< Led blink duration         */

#define DEMO_RAW_MESSAGE_BUF_LEN 8192 /*!< Raw message buffer len     */

#define DEMO_ST_MANUFACTURER_ID 0x02U /*!< ST Manufacturer ID         */

void St25r3916_Init(void);
void St25r3916_Loop(void);

extern RfalRfST25R3916Class rfst25r3916;