/*
 * @Description: st25r3916
 * @Author: LILYGO_L
 * @Date: 2025-06-13 14:20:16
 * @LastEditTime: 2025-08-09 10:20:42
 * @License: GPL 3.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "t_display_p4_keyboard_config.h"
#include "cpp_bus_driver_library.h"
#include "rfal_rfst25r3916.h"
#include "rfal_nfc.h"

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT 0         /*!< Demo State:  Not initialized */
#define DEMO_ST_START_DISCOVERY 1 /*!< Demo State:  Start Discovery */
#define DEMO_ST_DISCOVERY 2       /*!< Demo State:  Discovery       */

/* P2P communication data */
const uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
const uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE
#if RFAL_SUPPORT_MODE_LISTEN_NFCA
/* NFC-A CE config */
/* 4-byte UIDs with first byte 0x08 would need random number for the subsequent 3 bytes.
 * 4-byte UIDs with first byte 0x*F are Fixed number, not unique, use for this demo
 * 7-byte UIDs need a manufacturer ID and need to assure uniqueness of the rest.*/
static uint8_t ceNFCA_NFCID[] = {0x5F, 'S', 'T', 'M'}; /* =_STM, 5F 53 54 4D NFCID1 / UID (4 bytes) */
static uint8_t ceNFCA_SENS_RES[] = {0x02, 0x00};       /* SENS_RES / ATQA for 4-byte UID            */
static uint8_t ceNFCA_SEL_RES = 0x20;                  /* SEL_RES / SAK                             */
#endif                                                 /*RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
/* NFC-F CE config */
static uint8_t ceNFCF_nfcid2[] = {0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
static uint8_t ceNFCF_SC[] = {0x12, 0xFC};
static uint8_t ceNFCF_SENSF_RES[] = {0x01,                                           /* SENSF_RES                                */
                                     0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, /* NFCID2                                   */
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00, /* PAD0, PAD01, MRTIcheck, MRTIupdate, PAD2 */
                                     0x00, 0x00}; /* RD                                       */
#endif /*RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /*  RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

uint8_t state = DEMO_ST_NOTINIT;

volatile bool Interrupt_Flag = false;

auto IIC_Bus_0 = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(XL9555_SDA, XL9555_SCL, I2C_NUM_0);

auto XL9555 = std::make_unique<Cpp_Bus_Driver::Xl95x5>(IIC_Bus_0, XL9555_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

auto ESP32P4 = std::make_unique<Cpp_Bus_Driver::Tool>();

auto St25r3916 = std::make_unique<RfalRfST25R3916Class>(&SPI, -1, -1);
auto Rfal_Nfc = std::make_unique<RfalNfcClass>(St25r3916.get());

rfalNfcDiscoverParam Disc_Param;

void St25r3916_Init(void)
{
    ReturnCode err = Rfal_Nfc->rfalNfcInitialize();
    if (err == ERR_NONE)
    {
        Disc_Param.compMode = RFAL_COMPLIANCE_MODE_NFC;
        Disc_Param.devLimit = 1U;
        Disc_Param.nfcfBR = RFAL_BR_212;
        Disc_Param.ap2pBR = RFAL_BR_424;

        ST_MEMCPY(&Disc_Param.nfcid3, NFCID3, sizeof(NFCID3));
        ST_MEMCPY(&Disc_Param.GB, GB, sizeof(GB));
        Disc_Param.GBLen = sizeof(GB);

        Disc_Param.notifyCb = NULL;
        Disc_Param.totalDuration = 1000U;
        Disc_Param.wakeupEnabled = false;
        Disc_Param.wakeupConfigDefault = true;
#if RFAL_FEATURE_NFCA
        Disc_Param.techs2Find |= RFAL_NFC_POLL_TECH_A;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
        Disc_Param.techs2Find |= RFAL_NFC_POLL_TECH_B;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
        Disc_Param.techs2Find |= RFAL_NFC_POLL_TECH_F;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
        Disc_Param.techs2Find |= RFAL_NFC_POLL_TECH_V;
#endif /* RFAL_FEATURE_NFCV */

#if RFAL_FEATURE_ST25TB
        Disc_Param.techs2Find |= RFAL_NFC_POLL_TECH_ST25TB;
#endif /* RFAL_FEATURE_ST25TB */

#if ST25R95
        Disc_Param.isoDepFS = RFAL_ISODEP_FSXI_128; /* ST25R95 cannot support 256 bytes of data block */
#endif                                              /* ST25R95 */

#if RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP
        Disc_Param.techs2Find |= RFAL_NFC_POLL_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE
        Disc_Param.techs2Find |= RFAL_NFC_LISTEN_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE */

#if DEMO_CARD_EMULATION_ONLY
        Disc_Param.totalDuration = 60000U;          /* 60 seconds */
        Disc_Param.techs2Find = RFAL_NFC_TECH_NONE; /* Overwrite any previous poller modes */
#endif                                              /* DEMO_CARD_EMULATION_ONLY */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE

#if RFAL_SUPPORT_MODE_LISTEN_NFCA
        /* Set configuration for NFC-A CE */
        ST_MEMCPY(Disc_Param.lmConfigPA.SENS_RES, ceNFCA_SENS_RES, RFAL_LM_SENS_RES_LEN); /* Set SENS_RES / ATQA */
        ST_MEMCPY(Disc_Param.lmConfigPA.nfcid, ceNFCA_NFCID, RFAL_LM_NFCID_LEN_04);       /* Set NFCID / UID */
        Disc_Param.lmConfigPA.nfcidLen = RFAL_LM_NFCID_LEN_04;                            /* Set NFCID length to 7 bytes */
        Disc_Param.lmConfigPA.SEL_RES = ceNFCA_SEL_RES;                                   /* Set SEL_RES / SAK */

        Disc_Param.techs2Find |= RFAL_NFC_LISTEN_TECH_A;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
        /* Set configuration for NFC-F CE */
        ST_MEMCPY(Disc_Param.lmConfigPF.SC, ceNFCF_SC, RFAL_LM_SENSF_SC_LEN);                /* Set System Code */
        ST_MEMCPY(&ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN], ceNFCF_nfcid2, RFAL_NFCID2_LEN);     /* Load NFCID2 on SENSF_RES */
        ST_MEMCPY(Disc_Param.lmConfigPF.SENSF_RES, ceNFCF_SENSF_RES, RFAL_LM_SENSF_RES_LEN); /* Set SENSF_RES / Poll Response */

        Disc_Param.techs2Find |= RFAL_NFC_LISTEN_TECH_F;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

        /* Check for valid configuration by calling Discover once */
        Rfal_Nfc->rfalNfcDiscover(&Disc_Param);
        Rfal_Nfc->rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);

        printf("st25r3916 init success\n");
    }
    else
    {
        printf("st25r3916 init fail (error code: %d)\n", err);
    }
}

extern "C" void app_main(void)
{
    printf("Ciallo\n");
    XL9555->begin();
    XL9555->pin_mode(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9555->pin_write(XL9555_T_MIXRF_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

    vTaskDelay(pdMS_TO_TICKS(10));

    ESP32P4->pin_mode(ESP32P4_BOOT, Cpp_Bus_Driver::Tool::Pin_Mode::INPUT);

    ESP32P4->pin_mode(T_MIXRF_CC1101_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    ESP32P4->pin_mode(T_MIXRF_NRF24L01_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    ESP32P4->pin_mode(T_MIXRF_ST25R3916_CS, Cpp_Bus_Driver::Tool::Pin_Mode::OUTPUT);
    ESP32P4->pin_write(T_MIXRF_CC1101_CS, 1);
    ESP32P4->pin_write(T_MIXRF_NRF24L01_CS, 1);
    ESP32P4->pin_write(T_MIXRF_ST25R3916_CS, 1);

    ESP32P4->create_gpio_interrupt(T_MIXRF_ST25R3916_INT, Cpp_Bus_Driver::Tool::Interrupt_Mode::RISING,
                                   [](void *arg) -> IRAM_ATTR void
                                   {
                                       Interrupt_Flag = true;
                                   });

    SPI.begin(T_MIXRF_ST25R3916_SCLK, T_MIXRF_ST25R3916_MISO, T_MIXRF_ST25R3916_MOSI, T_MIXRF_ST25R3916_CS);
    St25r3916_Init();

    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
