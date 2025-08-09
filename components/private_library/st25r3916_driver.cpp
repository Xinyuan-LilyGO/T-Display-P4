/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-08-09 11:13:23
 * @LastEditTime: 2025-08-09 17:19:03
 * @License: GPL 3.0
 */

/*! \file
 *
 *  \author SRA
 *
 *  \brief Demo application
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how
 *  to exchange data with these devices, using the RFAL library.
 *
 *  This demo does not fully implement the activities according to the standards,
 *  it performs the required to communicate with a card/device and retrieve
 *  its UID. Also blocking methods are used for data exchange which may lead to
 *  long periods of blocking CPU/MCU.
 *  For standard compliant example please refer to the Examples provided
 *  with the RFAL library.
 *
 */

#include "nfc_utils.h"
#include "rfal_nfc.h"
#include "ndef_class.h"
#include "ndef_t5t.h"
#include "st25r3916_driver.h"

/* P2P communication data */
static uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};

/* P2P communication data */
static uint8_t ndefLLCPSYMM[] = {0x00, 0x00};
static uint8_t ndefInit[] = {0x05, 0x20, 0x06, 0x0F, 0x75, 0x72, 0x6E, 0x3A, 0x6E, 0x66, 0x63, 0x3A, 0x73, 0x6E, 0x3A, 0x73, 0x6E, 0x65, 0x70, 0x02, 0x02, 0x07, 0x80, 0x05, 0x01, 0x02};
static const uint8_t ndefSnepPrefix[] = {0x13, 0x20, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00};
static const uint8_t URL[] = "st.com";
static ndefConstBuffer bufURL = {URL, sizeof(URL) - 1};
static uint8_t ndefUriBuffer[255];

static uint8_t *ndefStates[] = {
    (uint8_t *)"INVALID",
    (uint8_t *)"INITIALIZED",
    (uint8_t *)"READ/WRITE",
    (uint8_t *)"READ-ONLY"};

static const uint8_t *ndefDemoFeatureDescription[NDEF_DEMO_MAX_FEATURES] = {
    (uint8_t *)"1. Tap a tag to read its content",
#if NDEF_FEATURE_FULL_API
    (uint8_t *)"2. Present a tag to write a Text record",
    (uint8_t *)"3. Present a tag to write a URI record and an Android Application record",
    (uint8_t *)"4. Present an ST tag to format",
#endif /*NDEF_FEATURE_FULL_API */
};

#if NDEF_FEATURE_FULL_API
static uint8_t ndefURI[] = "st.com";
static uint8_t ndefTEXT[] = "Welcome to ST NDEF demo";
static uint8_t ndefTextLangCode[] = "en";

static uint8_t ndefAndroidPackName[] = "com.st.st25nfc";
#endif /*NDEF_FEATURE_FULL_API */

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
                                     0x00, 0x00};                                    /* RD                                       */
#endif                                                                               /*RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif                                                                               /*  RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
static rfalNfcDiscoverParam discParam;
static uint8_t state = DEMO_ST_NOTINIT;

static uint8_t ndefDemoFeature = NDEF_DEMO_READ;
static uint8_t ndefDemoPrevFeature = 0xFF;
static bool verbose = false;

static uint8_t rawMessageBuf[DEMO_RAW_MESSAGE_BUF_LEN];

static uint32_t timer;
static uint32_t timerLed;
static bool ledOn;

#define MAX_HEX_STR 4
#define MAX_HEX_STR_LENGTH 128
char hexStr[MAX_HEX_STR][MAX_HEX_STR_LENGTH];
uint8_t hexStrIdx = 0;

int PushButtonState = 0;

/* SPI, Component and NFC */
#ifndef I2C_ENABLED
RfalRfST25R3916Class rfst25r3916(&SPI, CS_PIN, IRQ_PIN);
#else
RfalRfST25R3916Class rfst25r3916(&dev_i2c, IRQ_PIN);
#endif
RfalNfcClass rfal_nfc(&rfst25r3916);
NdefClass ndef(&rfal_nfc);

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/

static void demoNdef(rfalNfcDevice *nfcDevice);
static void ndefCCDump(void);
#if NDEF_FEATURE_T5T
static void ndefDumpSysInfo(void);
#endif /* RFAL_FEATURE_T5T */

#if NDEF_FEATURE_FULL_API
static bool ndefIsSTTag(void);
static void LedNotificationWriteDone(void);
#endif /* NDEF_FEATURE_FULL_API */

static void demoP2P(void);
ReturnCode demoTransceiveBlocking(uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt);

static void ledsOn(void);
static void ledsOff(void);

static ReturnCode ndefRecordDump(const ndefRecord *record, bool verbose);
static ReturnCode ndefMessageDump(const ndefMessage *message, bool verbose);
static ReturnCode ndefEmptyTypeDump(const ndefType *empty);
static ReturnCode ndefRtdDeviceInfoDump(const ndefType *devInfo);
static ReturnCode ndefRtdTextDump(const ndefType *text);
static ReturnCode ndefRtdUriDump(const ndefType *uri);
static ReturnCode ndefRtdAarDump(const ndefType *ext);
static ReturnCode ndefMediaVCardDump(const ndefType *vCard);
static ReturnCode ndefMediaWifiDump(const ndefType *wifi);
static ReturnCode ndefRecordDumpType(const ndefRecord *record);
static ReturnCode ndefBufferDump(const char *string, const ndefConstBuffer *bufPayload, bool verbose);
static ReturnCode ndefBufferPrint(const char *prefix, const ndefConstBuffer *bufPayload, const char *suffix);
static ReturnCode ndefBuffer8Print(const char *prefix, const ndefConstBuffer8 *bufPayload, const char *suffix);

/*! Table to associate enums to pointer to function */
typedef struct
{
    ndefTypeId typeId;                        /*!< NDEF type Id             */
    ReturnCode (*dump)(const ndefType *type); /*!< Pointer to dump function */
} ndefTypeDumpTable;

#if NDEF_TYPE_VCARD_SUPPORT
/*! Type to associate a property and string description */
typedef struct
{
    ndefConstBuffer bufType;
    char *string;
} ndefVCardTypeTable;
#endif

#if NDEF_TYPE_BLUETOOTH_SUPPORT
/*! Type to associate EIRs and string description */
typedef struct
{
    uint8_t eirType;
    char *string;
} ndefEirTable;
#endif

static char *hex2Str(unsigned char *data, size_t dataLen);

/*!
 *****************************************************************************
 * \brief Check user button
 *
 *  This function check whether the user button has been pressed
 *****************************************************************************
 */

static void checkUserButton(void)
{
    /* Check if USER button is pressed */
    if (digitalRead(USER_BTN) == PushButtonState)
    {
        ndefDemoFeature++;
        ndefDemoFeature %= NDEF_DEMO_MAX_FEATURES;

        ledsOff();
        ndefDemoPrevFeature = ndefDemoFeature;
        Serial.print((char *)ndefDemoFeatureDescription[ndefDemoFeature]);
        Serial.print("\r\n");

        /* Debounce button */
        delay(50);

        /* Wait until the button is released */
        while ((digitalRead(USER_BTN) == PushButtonState))
            ;

        /* Debouncing */
        delay(50);

        if (ndefDemoFeature != NDEF_DEMO_READ)
        {
            timer = rfst25r3916.timerCalculateTimer(NDEF_WRITE_FORMAT_TIMEOUT);
            timerLed = rfst25r3916.timerCalculateTimer(NDEF_LED_BLINK_DURATION);
        }
    }
}

/*!
 *****************************************************************************
 * \brief Show usage
 *
 *  This function displays usage information
 *****************************************************************************
 */
static void ndefShowDemoUsage()
{
#if NDEF_FEATURE_FULL_API
    uint32_t i;

    Serial.print("Use the User button to cycle among the different modes:\r\n");
    for (i = 0; i < SIZEOF_ARRAY(ndefDemoFeatureDescription); i++)
    {
        Serial.print((char *)ndefDemoFeatureDescription[i]);
        Serial.print("\r\n");
    }
    Serial.print("In Write or Format mode (menu 2, 3 or 4), the demo returns to Read mode (menu 1) if no tag detected after ");
    Serial.print((NDEF_WRITE_FORMAT_TIMEOUT / 1000));
    Serial.print(" seconds\r\n\n");
#endif /* NDEF_FEATURE_FULL_API */
}

void St25r3916_Init(void)
{
#ifndef I2C_ENABLED
    SPI.begin(ST25R3916_SPI_SCLK, ST25R3916_SPI_MISO, ST25R3916_SPI_MOSI, CS_PIN);
#else
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    Wire.begin();
    Wire.setClock(400000);
#endif

    pinMode(LED_A_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    pinMode(LED_F_PIN, OUTPUT);
    pinMode(LED_V_PIN, OUTPUT);
    pinMode(LED_AP2P_PIN, OUTPUT);
    pinMode(LED_FIELD_PIN, OUTPUT);
    pinMode(USER_BTN, INPUT);

    /* Check what is the Push Button State when the button is not pressed. It can change across families */
    PushButtonState = (digitalRead(USER_BTN)) ? 0 : 1;

    Serial.println("st25r3916 start");

    ndefShowDemoUsage();

    ReturnCode err = rfal_nfc.rfalNfcInitialize();
    if (err == ERR_NONE)
    {
        discParam.compMode = RFAL_COMPLIANCE_MODE_NFC;
        discParam.devLimit = 1U;
        discParam.nfcfBR = RFAL_BR_212;
        discParam.ap2pBR = RFAL_BR_424;

        ST_MEMCPY(&discParam.nfcid3, NFCID3, sizeof(NFCID3));
        ST_MEMCPY(&discParam.GB, GB, sizeof(GB));
        discParam.GBLen = sizeof(GB);

        discParam.notifyCb = NULL;
        discParam.totalDuration = 1000U;
        discParam.wakeupEnabled = false;
        discParam.wakeupConfigDefault = true;
#if RFAL_FEATURE_NFCA
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_A;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_B;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_F;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_V;
#endif /* RFAL_FEATURE_NFCV */

#if RFAL_FEATURE_ST25TB
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_ST25TB;
#endif /* RFAL_FEATURE_ST25TB */

#if ST25R95
        discParam.isoDepFS = RFAL_ISODEP_FSXI_128; /* ST25R95 cannot support 256 bytes of data block */
#endif                                             /* ST25R95 */

#if RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE
        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE */

#if DEMO_CARD_EMULATION_ONLY
        discParam.totalDuration = 60000U;          /* 60 seconds */
        discParam.techs2Find = RFAL_NFC_TECH_NONE; /* Overwrite any previous poller modes */
#endif                                             /* DEMO_CARD_EMULATION_ONLY */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE

#if RFAL_SUPPORT_MODE_LISTEN_NFCA
        /* Set configuration for NFC-A CE */
        ST_MEMCPY(discParam.lmConfigPA.SENS_RES, ceNFCA_SENS_RES, RFAL_LM_SENS_RES_LEN); /* Set SENS_RES / ATQA */
        ST_MEMCPY(discParam.lmConfigPA.nfcid, ceNFCA_NFCID, RFAL_LM_NFCID_LEN_04);       /* Set NFCID / UID */
        discParam.lmConfigPA.nfcidLen = RFAL_LM_NFCID_LEN_04;                            /* Set NFCID length to 7 bytes */
        discParam.lmConfigPA.SEL_RES = ceNFCA_SEL_RES;                                   /* Set SEL_RES / SAK */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_A;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
        /* Set configuration for NFC-F CE */
        ST_MEMCPY(discParam.lmConfigPF.SC, ceNFCF_SC, RFAL_LM_SENSF_SC_LEN);                /* Set System Code */
        ST_MEMCPY(&ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN], ceNFCF_nfcid2, RFAL_NFCID2_LEN);    /* Load NFCID2 on SENSF_RES */
        ST_MEMCPY(discParam.lmConfigPF.SENSF_RES, ceNFCF_SENSF_RES, RFAL_LM_SENSF_RES_LEN); /* Set SENSF_RES / Poll Response */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_F;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

        /* Check for valid configuration by calling Discover once */
        rfal_nfc.rfalNfcDiscover(&discParam);
        rfal_nfc.rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);

        state = DEMO_ST_START_DISCOVERY;

        Serial.printf("st25r3916 init success\n");
    }
    else
    {
        Serial.printf("st25r3916 init fail (error code: %d)\n", err);
    }
}

void St25r3916_Loop(void)
{
    static rfalNfcDevice *nfcDevice;

#if RFAL_FEATURE_NFCA
    rfalNfcaSensRes sensRes;
    rfalNfcaSelRes selRes;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
    static rfalNfcbSensbRes sensbRes;
    static uint8_t sensbResLen;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
    uint8_t devCnt = 0;
    rfalFeliCaPollRes cardList[1];
    uint8_t collisions = 0U;
    rfalNfcfSensfRes *sensfRes;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
    rfalNfcvInventoryRes invRes;
    uint16_t rcvdLen;
#endif /* RFAL_FEATURE_NFCV */

    rfal_nfc.rfalNfcWorker(); /* Run RFAL worker periodically */

    if ((ndefDemoFeature != NDEF_DEMO_READ) && (rfst25r3916.timerIsExpired(timer)))
    {
        Serial.print("Timer expired, back to Read mode...\r\n");
        ledsOff();
        ndefDemoFeature = NDEF_DEMO_READ;
    }

    if (ndefDemoFeature != ndefDemoPrevFeature)
    {
        ndefDemoPrevFeature = ndefDemoFeature;
        Serial.print((char *)ndefDemoFeatureDescription[ndefDemoFeature]);
        Serial.print("\r\n");
    }

    if (ndefDemoFeature != NDEF_DEMO_READ)
    {
        if (rfst25r3916.timerIsExpired(timerLed))
        {
            timerLed = rfst25r3916.timerCalculateTimer(NDEF_LED_BLINK_DURATION);
            ledOn = !ledOn;
        }
        if (ledOn)
        {
            ledsOn();
        }
        else
        {
            ledsOff();
        }
    }

    checkUserButton();

    switch (state)
    {
    /*******************************************************************************/
    case DEMO_ST_START_DISCOVERY:
        ledsOff();

        rfal_nfc.rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
        rfal_nfc.rfalNfcDiscover(&discParam);

        state = DEMO_ST_DISCOVERY;
        break;

    /*******************************************************************************/
    case DEMO_ST_DISCOVERY:
        if (rfalNfcIsDevActivated(rfal_nfc.rfalNfcGetState()))
        {
            rfal_nfc.rfalNfcGetActiveDevice(&nfcDevice);

            ledsOff();
            delay(50);
            ndefDemoPrevFeature = 0xFF; /* Force the display of the prompt */
            switch (nfcDevice->type)
            {
                /*******************************************************************************/
#if RFAL_FEATURE_NFCA
            case RFAL_NFC_LISTEN_TYPE_NFCA:

                digitalWrite(LED_A_PIN, HIGH);
                switch (nfcDevice->dev.nfca.type)
                {
                case RFAL_NFCA_T1T:
                    Serial.print("ISO14443A/Topaz (NFC-A T1T) TAG found. UID: ");
                    Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                    Serial.print("\r\n");
                    rfal_nfc.rfalNfcaPollerSleep();
                    break;

                case RFAL_NFCA_T4T:
                    Serial.print("NFCA Passive ISO-DEP device found. UID: ");
                    Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                    Serial.print("\r\n");
                    demoNdef(nfcDevice);
                    rfal_nfc.rfalIsoDepDeselect();
                    break;

                case RFAL_NFCA_T4T_NFCDEP:
                case RFAL_NFCA_NFCDEP:
                    Serial.print("NFCA Passive P2P device found. NFCID: ");
                    Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                    Serial.print("\r\n");
                    demoP2P();
                    break;

                default:
                    Serial.print("ISO14443A/NFC-A card found. UID: ");
                    Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                    Serial.print("\r\n");
                    demoNdef(nfcDevice);
                    rfal_nfc.rfalNfcaPollerSleep();
                    break;
                }
                /* Loop until tag is removed from the field */
                Serial.print("Operation completed\r\nTag can be removed from the field\r\n");
                rfal_nfc.rfalNfcaPollerInitialize();
                while (rfal_nfc.rfalNfcaPollerCheckPresence(RFAL_14443A_SHORTFRAME_CMD_WUPA, &sensRes) == ERR_NONE)
                {
                    if (((nfcDevice->dev.nfca.type == RFAL_NFCA_T1T) && (!rfalNfcaIsSensResT1T(&sensRes))) ||
                        ((nfcDevice->dev.nfca.type != RFAL_NFCA_T1T) && (rfal_nfc.rfalNfcaPollerSelect(nfcDevice->dev.nfca.nfcId1, nfcDevice->dev.nfca.nfcId1Len, &selRes) != ERR_NONE)))
                    {
                        break;
                    }
                    rfal_nfc.rfalNfcaPollerSleep();
                    delay(130);
                }
                break;
#endif /* RFAL_FEATURE_NFCA */

                /*******************************************************************************/
#if RFAL_FEATURE_NFCB
            case RFAL_NFC_LISTEN_TYPE_NFCB:

                Serial.print("ISO14443B/NFC-B card found. UID: ");
                Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                Serial.print("\r\n");
                digitalWrite(LED_B_PIN, HIGH);

                if (rfalNfcbIsIsoDepSupported(&nfcDevice->dev.nfcb))
                {
                    demoNdef(nfcDevice);
                    rfal_nfc.rfalIsoDepDeselect();
                }
                else
                {
                    rfal_nfc.rfalNfcbPollerSleep(nfcDevice->dev.nfcb.sensbRes.nfcid0);
                }
                /* Loop until tag is removed from the field */
                Serial.print("Operation completed\r\nTag can be removed from the field\r\n");
                rfal_nfc.rfalNfcbPollerInitialize();
                while (rfal_nfc.rfalNfcbPollerCheckPresence(RFAL_NFCB_SENS_CMD_ALLB_REQ, RFAL_NFCB_SLOT_NUM_1, &sensbRes, &sensbResLen) == ERR_NONE)
                {
                    if (ST_BYTECMP(sensbRes.nfcid0, nfcDevice->dev.nfcb.sensbRes.nfcid0, RFAL_NFCB_NFCID0_LEN) != 0)
                    {
                        break;
                    }
                    rfal_nfc.rfalNfcbPollerSleep(nfcDevice->dev.nfcb.sensbRes.nfcid0);
                    delay(130);
                }
                break;
#endif /* RFAL_FEATURE_NFCB */

                /*******************************************************************************/
#if RFAL_FEATURE_NFCF
            case RFAL_NFC_LISTEN_TYPE_NFCF:

                if (rfalNfcfIsNfcDepSupported(&nfcDevice->dev.nfcf))
                {
                    Serial.print("NFCF Passive P2P device found. NFCID: ");
                    Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                    Serial.print("\r\n");
                    demoP2P();
                }
                else
                {
                    Serial.print("Felica/NFC-F card found. UID: ");
                    Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                    Serial.print("\r\n");
                    demoNdef(nfcDevice);
                }

                digitalWrite(LED_F_PIN, HIGH);
                /* Loop until tag is removed from the field */
                Serial.print("Operation completed\r\nTag can be removed from the field\r\n");
                devCnt = 1;
                rfal_nfc.rfalNfcfPollerInitialize(RFAL_BR_212);
                while (rfal_nfc.rfalNfcfPollerPoll(RFAL_FELICA_1_SLOT, RFAL_NFCF_SYSTEMCODE, RFAL_FELICA_POLL_RC_NO_REQUEST, cardList, &devCnt, &collisions) == ERR_NONE)
                {
                    /* Skip the length field byte */
                    sensfRes = (rfalNfcfSensfRes *)&((uint8_t *)cardList)[1];
                    if (ST_BYTECMP(sensfRes->NFCID2, nfcDevice->dev.nfcf.sensfRes.NFCID2, RFAL_NFCF_NFCID2_LEN) != 0)
                    {
                        break;
                    }
                    delay(130);
                }
                break;
#endif /* RFAL_FEATURE_NFCF */

                /*******************************************************************************/
#if RFAL_FEATURE_NFCV
            case RFAL_NFC_LISTEN_TYPE_NFCV:
            {
                uint8_t devUID[RFAL_NFCV_UID_LEN];

                ST_MEMCPY(devUID, nfcDevice->nfcid, nfcDevice->nfcidLen); /* Copy the UID into local var */
                REVERSE_BYTES(devUID, RFAL_NFCV_UID_LEN);                 /* Reverse the UID for display purposes */
                Serial.print("ISO15693/NFC-V card found. UID: ");
                Serial.print(hex2Str(devUID, RFAL_NFCV_UID_LEN));
                Serial.print("\r\n");

                digitalWrite(LED_V_PIN, HIGH);

                demoNdef(nfcDevice);

                /* Loop until tag is removed from the field */
                Serial.print("Operation completed\r\nTag can be removed from the field\r\n");
                rfal_nfc.rfalNfcvPollerInitialize();
                while (rfal_nfc.rfalNfcvPollerInventory(RFAL_NFCV_NUM_SLOTS_1, RFAL_NFCV_UID_LEN * 8U, nfcDevice->dev.nfcv.InvRes.UID, &invRes, &rcvdLen) == ERR_NONE)
                {
                    delay(130);
                }
            }
            break;
#endif /* RFAL_FEATURE_NFCV */

            /*******************************************************************************/
#if RFAL_FEATURE_ST25TB
            case RFAL_NFC_LISTEN_TYPE_ST25TB:

                Serial.print("ST25TB card found. UID: ");
                Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                Serial.print("\r\n");
                digitalWrite(LED_B_PIN, HIGH);
                break;
#endif /* RFAL_FEATURE_ST25TB */

            /*******************************************************************************/
            case RFAL_NFC_LISTEN_TYPE_AP2P:

                Serial.print("NFC Active P2P device found. NFCID3: ");
                Serial.print(hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                Serial.print("\r\n");
                digitalWrite(LED_AP2P_PIN, HIGH);

                demoP2P();
                break;

            /*******************************************************************************/
            default:
                break;
            }

            rfal_nfc.rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
            delay(500);
            state = DEMO_ST_START_DISCOVERY;
        }
        break;

    /*******************************************************************************/
    case DEMO_ST_NOTINIT:
    default:
        break;
    }
}

/*!
 *****************************************************************************
 * \brief Demo P2P Exchange
 *
 * Sends a NDEF URI record 'http://www.ST.com' via NFC-DEP (P2P) protocol.
 *
 * This method sends a set of static predefined frames which tries to establish
 * a LLCP connection, followed by the NDEF record, and then keeps sending
 * LLCP SYMM packets to maintain the connection.
 *
 *
 *****************************************************************************
 */
void demoP2P(void)
{
#if RFAL_FEATURE_NFC_DEP && NDEF_FEATURE_FULL_API
    uint16_t *rxLen;
    uint8_t *rxData;
    ReturnCode err;

    ndefBuffer bufPayload;
    ndefMessage message;
    ndefRecord record;
    ndefType uri;

    Serial.print(" Initialize device .. ");
    err = demoTransceiveBlocking(ndefInit, sizeof(ndefInit), &rxData, &rxLen, RFAL_FWT_NONE);
    if (err != ERR_NONE)
    {
        Serial.print("failed.");
        return;
    }
    Serial.print("succeeded.\r\n");

    err = ndefRtdUriInit(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufURL);
    err |= ndefRtdUriToRecord(&uri, &record);

    err |= ndefMessageInit(&message);
    err |= ndefMessageAppend(&message, &record); /* To get MB and ME bits set */

    /* Build the SNEP buffer made of the prefix, the length byte and the record */
    ST_MEMCPY(ndefUriBuffer, ndefSnepPrefix, sizeof(ndefSnepPrefix));

    /* Skip 1 byte for length byte */
    bufPayload.buffer = ndefUriBuffer + sizeof(ndefSnepPrefix) + 1;
    bufPayload.length = sizeof(ndefUriBuffer) - sizeof(ndefSnepPrefix);
    err |= ndefMessageEncode(&message, &bufPayload);

    ndefUriBuffer[sizeof(ndefSnepPrefix)] = bufPayload.length;

    bufPayload.buffer = ndefUriBuffer;
    bufPayload.length = sizeof(ndefSnepPrefix) + 1 + bufPayload.length;

    if (err != ERR_NONE)
    {
        Serial.print("NDEF message creation failed\r\n");
        return;
    }

    ndefBufferDump("URL converted to SNEP:\r\n", (ndefConstBuffer *)&bufPayload, true);

    Serial.print(" Push NDEF Uri: www.ST.com .. ");
    err = demoTransceiveBlocking(bufPayload.buffer, bufPayload.length, &rxData, &rxLen, RFAL_FWT_NONE);
    if (err != ERR_NONE)
    {
        Serial.print("failed.");
        return;
    }
    Serial.print("succeeded.\r\n");

    Serial.print(" Device present, maintaining connection ");
    while (err == ERR_NONE)
    {
        err = demoTransceiveBlocking(ndefLLCPSYMM, sizeof(ndefLLCPSYMM), &rxData, &rxLen, RFAL_FWT_NONE);
        Serial.print(".");
        delay(50);
    }
    Serial.print("\r\n Device removed.\r\n");
#endif /* RFAL_FEATURE_NFC_DEP */
}

/*!
 *****************************************************************************
 * \brief Demo Blocking Transceive
 *
 * Helper function to send data in a blocking manner via the rfalNfc module
 *
 * \warning A protocol transceive handles long timeouts (several seconds),
 * transmission errors and retransmissions which may lead to a long period of
 * time where the MCU/CPU is blocked in this method.
 * This is a demo implementation, for a non-blocking usage example please
 * refer to the Examples available with RFAL
 *
 * \param[in]  txBuf      : data to be transmitted
 * \param[in]  txBufSize  : size of the data to be transmitted
 * \param[out] rxData     : location where the received data has been placed
 * \param[out] rcvLen     : number of data bytes received
 * \param[in]  fwt        : FWT to be used (only for RF frame interface,
 *                                          otherwise use RFAL_FWT_NONE)
 *
 *
 *  \return ERR_PARAM     : Invalid parameters
 *  \return ERR_TIMEOUT   : Timeout error
 *  \return ERR_FRAMING   : Framing error detected
 *  \return ERR_PROTO     : Protocol error detected
 *  \return ERR_NONE      : No error, activation successful
 *
 *****************************************************************************
 */
ReturnCode demoTransceiveBlocking(uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxData, uint16_t **rcvLen, uint32_t fwt)
{
    ReturnCode err;

    err = rfal_nfc.rfalNfcDataExchangeStart(txBuf, txBufSize, rxData, rcvLen, fwt);
    if (err == ERR_NONE)
    {
        do
        {
            rfal_nfc.rfalNfcWorker();
            err = rfal_nfc.rfalNfcDataExchangeGetStatus();
        } while (err == ERR_BUSY);
    }
    return err;
}

static void demoNdef(rfalNfcDevice *pNfcDevice)
{
    ReturnCode err;
    ndefMessage message;
    uint32_t rawMessageLen;
    ndefInfo info;
    ndefBuffer bufRawMessage;
    ndefConstBuffer bufConstRawMessage;

#if NDEF_FEATURE_FULL_API
    ndefRecord record1;
    ndefRecord record2;

    ndefType text;
    ndefType uri;
    ndefType aar;

    ndefConstBuffer8 bufTextLangCode;
    ndefConstBuffer bufTextLangText;
    ndefConstBuffer bufUri;
    ndefConstBuffer bufAndroidPackName;
#endif /* NDEF_FEATURE_FULL_API */

    /*
     * Perform NDEF Context Initialization
     */
    err = ndef.ndefPollerContextInitializationWrapper(pNfcDevice);
    if (err != ERR_NONE)
    {
        Serial.print("NDEF NOT DETECTED (ndefPollerContextInitializationWrapper returns ");
        Serial.print(err);
        Serial.print(")\r\n");
        return;
    }

#if NDEF_FEATURE_T5T
    if (verbose & (pNfcDevice->type == RFAL_NFC_LISTEN_TYPE_NFCV))
    {
        ndefDumpSysInfo();
    }
#endif /* RFAL_FEATURE_T5T */

    /*
     * Perform NDEF Detect procedure
     */
    err = ndef.ndefPollerNdefDetectWrapper(&info);
    if (err != ERR_NONE)
    {
        Serial.print("NDEF NOT DETECTED (ndefPollerNdefDetectWrapper returns ");
        Serial.print(err);
        Serial.print(")\r\n");
        if (ndefDemoFeature != NDEF_DEMO_FORMAT_TAG)
        {
            return;
        }
    }
    else
    {
        Serial.print((char *)ndefStates[info.state]);
        Serial.print(" NDEF detected.\r\n");
        ndefCCDump();

        if (verbose)
        {
            Serial.print("NDEF Len: ");
            Serial.print(ndef.ctx.messageLen);
            Serial.print(", Offset=");
            Serial.print(ndef.ctx.messageOffset);
            Serial.print("\r\n");
        }
    }

    switch (ndefDemoFeature)
    {
    /*
     * Demonstrate how to read the NDEF message from the Tag
     */
    case NDEF_DEMO_READ:
        if (info.state == NDEF_STATE_INITIALIZED)
        {
            /* Nothing to read... */
            return;
        }
        err = ndef.ndefPollerReadRawMessageWrapper(rawMessageBuf, sizeof(rawMessageBuf), &rawMessageLen, true);
        if (err != ERR_NONE)
        {
            Serial.print("NDEF message cannot be read (ndefPollerReadRawMessageWrapper returns ");
            Serial.print(err);
            Serial.print(")\r\n");
            return;
        }
        if (verbose)
        {
            bufRawMessage.buffer = rawMessageBuf;
            bufRawMessage.length = rawMessageLen;
            ndefBufferDump(" NDEF Content", (ndefConstBuffer *)&bufRawMessage, verbose);
        }
        bufConstRawMessage.buffer = rawMessageBuf;
        bufConstRawMessage.length = rawMessageLen;
        err = ndefMessageDecode(&bufConstRawMessage, &message);
        if (err != ERR_NONE)
        {
            Serial.print("NDEF message cannot be decoded (ndefMessageDecode  returns ");
            Serial.print(err);
            Serial.print(")\r\n");
            return;
        }
        err = ndefMessageDump(&message, verbose);
        if (err != ERR_NONE)
        {
            Serial.print("NDEF message cannot be displayed (ndefMessageDump returns ");
            Serial.print(err);
            Serial.print(")\r\n");
            return;
        }
        break;

#if NDEF_FEATURE_FULL_API
    /*
     * Demonstrate how to encode a text record and write the message to the tag
     */
    case NDEF_DEMO_WRITE_MSG1:
        ndefDemoFeature = NDEF_DEMO_READ; /* returns to READ mode after write */
        err = ndefMessageInit(&message);  /* Initialize message structure */
        bufTextLangCode.buffer = ndefTextLangCode;
        bufTextLangCode.length = strlen((char *)ndefTextLangCode);

        bufTextLangText.buffer = ndefTEXT;
        bufTextLangText.length = strlen((char *)ndefTEXT);

        err |= ndefRtdTextInit(&text, TEXT_ENCODING_UTF8, &bufTextLangCode, &bufTextLangText); /* Initialize Text type structure */
        err |= ndefRtdTextToRecord(&text, &record1);                                           /* Encode Text Record */
        err |= ndefMessageAppend(&message, &record1);                                          /* Append Text record to message */
        if (err != ERR_NONE)
        {
            Serial.print("Message creation failed\r\n");
            return;
        }
        err = ndef.ndefPollerWriteMessageWrapper(&message); /* Write message */
        if (err != ERR_NONE)
        {
            Serial.print("Message cannot be written (ndefPollerWriteMessageWrapper return ");
            Serial.print(err);
            Serial.print(")\r\n");
            return;
        }
        Serial.print("Wrote 1 record to the Tag\r\n");
        if (verbose)
        {
            /* Dump raw message */
            bufRawMessage.buffer = rawMessageBuf;
            bufRawMessage.length = sizeof(rawMessageBuf);
            err = ndefMessageEncode(&message, &bufRawMessage);
            if (err == ERR_NONE)
            {
                ndefBufferDump("Raw message", (ndefConstBuffer *)&bufRawMessage, verbose);
            }
        }
        LedNotificationWriteDone();
        break;

    /*
     * Demonstrate how to encode a URI record and a AAR record, how to encode the message to a raw buffer and then how to write the raw buffer
     */
    case NDEF_DEMO_WRITE_MSG2:
        ndefDemoFeature = NDEF_DEMO_READ; /* returns to READ mode after write */
        err = ndefMessageInit(&message);  /* Initialize message structure */
        bufUri.buffer = ndefURI;
        bufUri.length = strlen((char *)ndefURI);
        err |= ndefRtdUriInit(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufUri); /* Initialize URI type structure */
        err |= ndefRtdUriToRecord(&uri, &record1);                      /* Encode URI Record */

        bufAndroidPackName.buffer = ndefAndroidPackName;
        bufAndroidPackName.length = sizeof(ndefAndroidPackName) - 1U;
        err |= ndefRtdAarInit(&aar, &bufAndroidPackName); /* Initialize AAR type structure */
        err |= ndefRtdAarToRecord(&aar, &record2);        /* Encode AAR record */

        err |= ndefMessageAppend(&message, &record1); /* Append URI to message */
        err |= ndefMessageAppend(&message, &record2); /* Append AAR to message (record #2 is an example of preformatted record) */

        bufRawMessage.buffer = rawMessageBuf;
        bufRawMessage.length = sizeof(rawMessageBuf);
        err |= ndefMessageEncode(&message, &bufRawMessage); /* Encode the message to the raw buffer */
        if (err != ERR_NONE)
        {
            Serial.print("Raw message creation failed\r\n");
            return;
        }
        err = ndef.ndefPollerWriteRawMessageWrapper(bufRawMessage.buffer, bufRawMessage.length);
        if (err != ERR_NONE)
        {
            Serial.print("Message cannot be written (ndefPollerWriteRawMessageWrapper return ");
            Serial.print(err);
            Serial.print(")\r\n");
            return;
        }
        Serial.print("Wrote 2 records to the Tag\r\n");
        if (verbose)
        {
            /* Dump raw message */
            ndefBufferDump("Raw message", (ndefConstBuffer *)&bufRawMessage, verbose);
        }
        LedNotificationWriteDone();
        break;

    /*
     * Demonstrate how to format a Tag
     */
    case NDEF_DEMO_FORMAT_TAG:
        ndefDemoFeature = NDEF_DEMO_READ;
        if (!ndefIsSTTag())
        {
            Serial.print("Manufacturer ID not found or not an ST tag. Format aborted \r\n");
            return;
        }
        Serial.print("Formatting Tag...\r\n");
        /* Format Tag */
        err = ndef.ndefPollerTagFormatWrapper(NULL, 0);
        if (err != ERR_NONE)
        {
            Serial.print("Tag cannot be formatted (ndefPollerTagFormatWrapper returns ");
            Serial.print(err);
            Serial.print(")\r\n");
            return;
        }
        Serial.print("Tag formatted\r\n");
        LedNotificationWriteDone();
        break;
#endif /* NDEF_FEATURE_FULL_API */

    default:
        ndefDemoFeature = NDEF_DEMO_READ;
        break;
    }
    return;
}

#if NDEF_FEATURE_T2T
static void ndefT2TCCDump()
{
    ndefConstBuffer bufCcBuf;

    Serial.print(" * Magic: ");
    Serial.print(ndef.ctx.cc.t2t.magicNumber, HEX);
    Serial.print("h Version: ");
    Serial.print(ndef.ctx.cc.t2t.majorVersion);
    Serial.print(".");
    Serial.print(ndef.ctx.cc.t2t.minorVersion);
    Serial.print(" Size: ");
    Serial.print(ndef.ctx.cc.t2t.size);
    Serial.print(" (");
    Serial.print((ndef.ctx.cc.t2t.size * 8U));
    Serial.print(" bytes) \r\n * readAccess: ");
    Serial.print(ndef.ctx.cc.t2t.readAccess, HEX);
    Serial.print("h writeAccess: ");
    Serial.print(ndef.ctx.cc.t2t.writeAccess, HEX);
    Serial.print("h \r\n");
    bufCcBuf.buffer = ndef.ctx.ccBuf;
    bufCcBuf.length = 4;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T2T */

#if NDEF_FEATURE_T3T
static void ndefT3TAIBDump()
{
    ndefConstBuffer bufCcBuf;

    Serial.print(" * Version: ");
    Serial.print(ndef.ctx.cc.t3t.majorVersion);
    Serial.print(".");
    Serial.print(ndef.ctx.cc.t3t.minorVersion);
    Serial.print(" Size: ");
    Serial.print(ndef.ctx.cc.t3t.nMaxB);
    Serial.print(" (");
    Serial.print((ndef.ctx.cc.t3t.nMaxB * 16U));
    Serial.print(" bytes) NbR: ");
    Serial.print(ndef.ctx.cc.t3t.nbR);
    Serial.print(" NbW: ");
    Serial.print(ndef.ctx.cc.t3t.nbW);
    Serial.print("\r\n * WriteFlag: ");
    Serial.print(ndef.ctx.cc.t3t.writeFlag, HEX);
    Serial.print("h RWFlag: ");
    Serial.print(ndef.ctx.cc.t3t.rwFlag, HEX);
    Serial.print("h \r\n");
    bufCcBuf.buffer = ndef.ctx.ccBuf;
    bufCcBuf.length = 16;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T3T */

#if NDEF_FEATURE_T4T
static void ndefT4TCCDump()
{
    ndefConstBuffer bufCcBuf;

    Serial.print(" * CCLEN: ");
    Serial.print(ndef.ctx.cc.t4t.ccLen);
    Serial.print(" T4T_VNo: ");
    Serial.print(ndef.ctx.cc.t4t.vNo, HEX);
    Serial.print("h MLe: ");
    Serial.print(ndef.ctx.cc.t4t.mLe);
    Serial.print(" MLc: ");
    Serial.print(ndef.ctx.cc.t4t.mLc);
    Serial.print(" FileId: ");
    Serial.print(ndef.ctx.cc.t4t.fileId[0], HEX);
    Serial.print(ndef.ctx.cc.t4t.fileId[1], HEX);
    Serial.print("h FileSize: ");
    Serial.print(ndef.ctx.cc.t4t.fileSize);
    Serial.print("\r\n * readAccess: ");
    Serial.print(ndef.ctx.cc.t4t.readAccess, HEX);
    Serial.print("h writeAccess: ");
    Serial.print(ndef.ctx.cc.t4t.writeAccess, HEX);
    Serial.print("h\r\n");
    bufCcBuf.buffer = ndef.ctx.ccBuf;
    bufCcBuf.length = ndef.ctx.cc.t4t.ccLen;
    ndefBufferDump(" CC File Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T4T */

#if NDEF_FEATURE_T5T
static void ndefT5TCCDump()
{
    ndefConstBuffer bufCcBuf;

    Serial.print(" * Block Length: ");
    Serial.print(ndef.ctx.subCtx.t5t.blockLen);
    Serial.print("\r\n");
    Serial.print(" * ");
    Serial.print(ndef.ctx.cc.t5t.ccLen);
    Serial.print(" bytes CC\r\n * Magic: ");
    Serial.print(ndef.ctx.cc.t5t.magicNumber, HEX);
    Serial.print("h Version: ");
    Serial.print(ndef.ctx.cc.t5t.majorVersion);
    Serial.print(".");
    Serial.print(ndef.ctx.cc.t5t.minorVersion);
    Serial.print(" MLEN: ");
    Serial.print(ndef.ctx.cc.t5t.memoryLen);
    Serial.print(" (");
    Serial.print((ndef.ctx.cc.t5t.memoryLen * 8U));
    Serial.print(" bytes) \r\n * readAccess: ");
    Serial.print(ndef.ctx.cc.t5t.readAccess, HEX);
    Serial.print("h writeAccess: ");
    Serial.print(ndef.ctx.cc.t5t.writeAccess, HEX);
    Serial.print("h \r\n");
    Serial.print(" * [");
    Serial.print((ndef.ctx.cc.t5t.specialFrame ? 'X' : ' '));
    Serial.print("] Special Frame\r\n");
    Serial.print(" * [");
    Serial.print((ndef.ctx.cc.t5t.multipleBlockRead ? 'X' : ' '));
    Serial.print("] Multiple block Read\r\n");
    Serial.print(" * [");
    Serial.print((ndef.ctx.cc.t5t.lockBlock ? 'X' : ' '));
    Serial.print("] Lock Block\r\n");
    bufCcBuf.buffer = ndef.ctx.ccBuf;
    bufCcBuf.length = ndef.ctx.cc.t5t.ccLen;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T5T */

static void ndefCCDump()
{
    if (!verbose)
    {
        return;
    }
    Serial.print(((ndef.ctx.device.type == NDEF_DEV_T3T) ? "NDEF Attribute Information Block\r\n" : "NDEF Capability Container\r\n"));
    switch (ndef.ctx.device.type)
    {
#if NDEF_FEATURE_T2T
    case NDEF_DEV_T2T:
        ndefT2TCCDump();
        break;
#endif /* NDEF_FEATURE_T2T */
#if NDEF_FEATURE_T3T
    case NDEF_DEV_T3T:
        ndefT3TAIBDump();
        break;
#endif /* NDEF_FEATURE_T3T */
#if NDEF_FEATURE_T4T
    case NDEF_DEV_T4T:
        ndefT4TCCDump();
        break;
#endif /* NDEF_FEATURE_T4T */
#if NDEF_FEATURE_T5T
    case NDEF_DEV_T5T:
        ndefT5TCCDump();
        break;
#endif /* NDEF_FEATURE_T5T */
    default:
        break;
    }
}

#if NDEF_FEATURE_T5T
static void ndefDumpSysInfo()
{
    ndefSystemInformation *sysInfo;

    if (!verbose)
    {
        return;
    }

    if (!ndef.ctx.subCtx.t5t.sysInfoSupported)
    {
        return;
    }

    sysInfo = &ndef.ctx.subCtx.t5t.sysInfo;
    Serial.print("System Information\r\n");
    Serial.print(" * ");
    Serial.print(ndefT5TSysInfoMOIValue(sysInfo->infoFlags) + 1);
    Serial.print(" byte(s) memory addressing\r\n");
    if (ndefT5TSysInfoDFSIDPresent(sysInfo->infoFlags))
    {
        Serial.print(" * DFSID=");
        Serial.print(sysInfo->DFSID, HEX);
        Serial.print("h\r\n");
    }
    if (ndefT5TSysInfoAFIPresent(sysInfo->infoFlags))
    {
        Serial.print(" * AFI=");
        Serial.print(sysInfo->AFI, HEX);
        Serial.print("h\r\n");
    }
    if (ndefT5TSysInfoMemSizePresent(sysInfo->infoFlags))
    {
        Serial.print(" * ");
        Serial.print(sysInfo->numberOfBlock);
        Serial.print(" blocks, ");
        Serial.print(sysInfo->blockSize);
        Serial.print(" bytes per block\r\n");
    }
    if (ndefT5TSysInfoICRefPresent(sysInfo->infoFlags))
    {
        Serial.print(" * ICRef=");
        Serial.print(sysInfo->ICRef, HEX);
        Serial.print("h\r\n");
    }
    if (ndefT5TSysInfoCmdListPresent(sysInfo->infoFlags))
    {
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoReadSingleBlockSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ReadSingleBlock                \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoWriteSingleBlockSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] WriteSingleBlock               \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoLockSingleBlockSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] LockSingleBlock                \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoReadMultipleBlocksSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ReadMultipleBlocks             \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoWriteMultipleBlocksSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] WriteMultipleBlocks            \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoSelectSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] Select                         \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoResetToReadySupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ResetToReady                   \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoGetMultipleBlockSecStatusSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] GetMultipleBlockSecStatus      \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoWriteAFISupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] WriteAFI                       \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoLockAFISupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] LockAFI                        \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoWriteDSFIDSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] WriteDSFID                     \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoLockDSFIDSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] LockDSFID                      \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoGetSystemInformationSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] GetSystemInformation           \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoCustomCmdsSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] CustomCmds                     \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoFastReadMultipleBlocksSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] FastReadMultipleBlocks         \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoExtReadSingleBlockSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ExtReadSingleBlock             \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoExtWriteSingleBlockSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ExtWriteSingleBlock            \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoExtLockSingleBlockSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ExtLockSingleBlock             \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoExtReadMultipleBlocksSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ExtReadMultipleBlocks          \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoExtWriteMultipleBlocksSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ExtWriteMultipleBlocks         \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoExtGetMultipleBlockSecStatusSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] ExtGetMultipleBlockSecStatus   \r\n");
        Serial.print(" * [");
        Serial.print(ndefT5TSysInfoFastExtendedReadMultipleBlocksSupported(sysInfo->supportedCmd) ? 'X' : ' ');
        Serial.print("] FastExtendedReadMultipleBlocks \r\n");
    }
    return;
}
#endif /* NDEF_FEATURE_T5T */

#if NDEF_FEATURE_FULL_API
static bool ndefIsSTTag()
{
    bool ret = false;

    switch (ndef.ctx.device.type)
    {
    case RFAL_NFC_LISTEN_TYPE_NFCA:
        if ((ndef.ctx.device.dev.nfca.nfcId1Len != 4) && (ndef.ctx.device.dev.nfca.nfcId1[0] == 0x02))
        {
            ret = true;
        }
        break;
    case RFAL_NFC_LISTEN_TYPE_NFCF:
        break;
    case RFAL_NFC_LISTEN_TYPE_NFCB:
        break;
    case RFAL_NFC_LISTEN_TYPE_NFCV:
        if (ndef.ctx.device.dev.nfcv.InvRes.UID[6] == 0x02)
        {
            ret = true;
        }
        break;
    default:
        break;
    }
    return (ret);
}

static void LedNotificationWriteDone(void)
{
    uint32_t i;

    for (i = 0; i < 3; i++)
    {
        ledsOn();
        delay(100);

        ledsOff();
        delay(100);
    }
}
#endif /* NDEF_FEATURE_FULL_API */

static void ledsOn(void)
{
    digitalWrite(LED_A_PIN, HIGH);
    digitalWrite(LED_B_PIN, HIGH);
    digitalWrite(LED_F_PIN, HIGH);
    digitalWrite(LED_V_PIN, HIGH);
    digitalWrite(LED_AP2P_PIN, HIGH);
    digitalWrite(LED_FIELD_PIN, HIGH);
}

static void ledsOff(void)
{
    digitalWrite(LED_A_PIN, LOW);
    digitalWrite(LED_B_PIN, LOW);
    digitalWrite(LED_F_PIN, LOW);
    digitalWrite(LED_V_PIN, LOW);
    digitalWrite(LED_AP2P_PIN, LOW);
    digitalWrite(LED_FIELD_PIN, LOW);
}

/*****************************************************************************/
static bool isPrintableASCII(const uint8_t *str, uint32_t strLen)
{
    uint32_t i;

    if ((str == NULL) || (strLen == 0))
    {
        return false;
    }

    for (i = 0; i < strLen; i++)
    {
        if ((str[i] < 0x20U) || (str[i] > 0x7EU))
        {
            return false;
        }
    }

    return true;
}

/*****************************************************************************/
ReturnCode ndefRecordDump(const ndefRecord *record, bool verbose)
{
    static uint32_t index;
    const uint8_t *ndefTNFNames[] = {
        (uint8_t *)"Empty",
        (uint8_t *)"NFC Forum well-known type [NFC RTD]",
        (uint8_t *)"Media-type as defined in RFC 2046",
        (uint8_t *)"Absolute URI as defined in RFC 3986",
        (uint8_t *)"NFC Forum external type [NFC RTD]",
        (uint8_t *)"Unknown",
        (uint8_t *)"Unchanged",
        (uint8_t *)"Reserved"};
    uint8_t *headerSR = (uint8_t *)"";
    ReturnCode err;

    if (record == NULL)
    {
        Serial.print("No record\r\n");
        return ERR_NONE;
    }

    if (ndefHeaderIsSetMB(record))
    {
        index = 1U;
    }
    else
    {
        index++;
    }

    if (verbose == true)
    {
        headerSR = (uint8_t *)(ndefHeaderIsSetSR(record) ? " - Short Record" : " - Standard Record");
    }

    Serial.print("Record #");
    Serial.print(index);
    Serial.print((char *)headerSR);
    Serial.print("\r\n");

    /* Well-known type dump */
    err = ndefRecordDumpType(record);
    if (verbose == true)
    {
        /* Raw dump */
        // Serial.print(" MB:%d ME:%d CF:%d SR:%d IL:%d TNF:%d\r\n", ndefHeaderMB(record), ndefHeaderME(record), ndefHeaderCF(record), ndefHeaderSR(record), ndefHeaderIL(record), ndefHeaderTNF(record));
        Serial.print(" MB ME CF SR IL TNF\r\n");
        Serial.print("  ");
        Serial.print(ndefHeaderMB(record));
        Serial.print("  ");
        Serial.print(ndefHeaderME(record));
        Serial.print("  ");
        Serial.print(ndefHeaderCF(record));
        Serial.print("  ");
        Serial.print(ndefHeaderSR(record));
        Serial.print("  ");
        Serial.print(ndefHeaderIL(record));
        Serial.print("  ");
        Serial.print(ndefHeaderTNF(record));
        Serial.print("\r\n");
    }
    if ((err != ERR_NONE) || (verbose == true))
    {
        Serial.print(" Type Name Format: ");
        Serial.print((char *)ndefTNFNames[ndefHeaderTNF(record)]);
        Serial.print("\r\n");

        uint8_t tnf;
        ndefConstBuffer8 bufRecordType;
        ndefRecordGetType(record, &tnf, &bufRecordType);
        if ((tnf == NDEF_TNF_EMPTY) && (bufRecordType.length == 0U))
        {
            Serial.print(" Empty NDEF record\r\n");
        }
        else
        {
            ndefBuffer8Print(" Type: \"", &bufRecordType, "\"\r\n");
        }

        if (ndefHeaderIsSetIL(record))
        {
            /* ID Length bit set */
            ndefConstBuffer8 bufRecordId;
            ndefRecordGetId(record, &bufRecordId);
            ndefBuffer8Print(" ID: \"", &bufRecordId, "\"\r\n");
        }

        ndefConstBuffer bufRecordPayload;
        ndefRecordGetPayload(record, &bufRecordPayload);
        ndefBufferDump(" Payload:", &bufRecordPayload, verbose);
        if (ndefRecordGetPayloadLength(record) != bufRecordPayload.length)
        {
            Serial.print(" Payload stored as a well-known type\r\n");
        }
    }

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefMessageDump(const ndefMessage *message, bool verbose)
{
    ReturnCode err;
    ndefRecord *record;

    if (message == NULL)
    {
        Serial.print("Empty NDEF message\r\n");
        return ERR_NONE;
    }
    else
    {
        Serial.print("Decoding NDEF message\r\n");
    }

    record = ndefMessageGetFirstRecord(message);

    while (record != NULL)
    {
        err = ndefRecordDump(record, verbose);
        if (err != ERR_NONE)
        {
            return err;
        }
        record = ndefMessageGetNextRecord(record);
    }

    return ERR_NONE;
}

#if NDEF_TYPE_FLAT_SUPPORT
/*****************************************************************************/
ReturnCode ndefFlatPayloadTypeDump(const ndefType *type)
{
    if ((type == NULL) || (type->id != NDEF_TYPE_ID_FLAT))
    {
        return ERR_PARAM;
    }

    ndefBufferDump("Flat payload", (ndefConstBuffer *)&type->data.bufPayload, "");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_EMPTY_SUPPORT
/*****************************************************************************/
ReturnCode ndefEmptyTypeDump(const ndefType *type)
{
    if ((type == NULL) || (type->id != NDEF_TYPE_ID_EMPTY))
    {
        return ERR_PARAM;
    }

    Serial.print(" Empty record\r\n");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
/*****************************************************************************/
ReturnCode ndefRtdDeviceInfoDump(const ndefType *type)
{
    ndefTypeRtdDeviceInfo devInfoData;
    ReturnCode err;
    uint32_t info;
    uint32_t i;

    static const uint8_t *ndefDeviceInfoName[] = {
        (uint8_t *)"Manufacturer",
        (uint8_t *)"Model",
        (uint8_t *)"Device",
        (uint8_t *)"UUID",
        (uint8_t *)"Firmware version",
    };

    err = ndefGetRtdDeviceInfo(type, &devInfoData);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" Device Information:\r\n");

    for (info = 0; info < NDEF_DEVICE_INFO_TYPE_COUNT; info++)
    {
        if (devInfoData.devInfo[info].buffer != NULL)
        {
            Serial.print(" - ");
            Serial.print((char *)ndefDeviceInfoName[devInfoData.devInfo[info].type]);
            Serial.print(": ");

            if (info != NDEF_DEVICE_INFO_UUID)
            {
                for (i = 0; i < devInfoData.devInfo[info].length; i++)
                {
                    Serial.print(devInfoData.devInfo[info].buffer[i]); /* character */
                }
            }
            else
            {
                for (i = 0; i < devInfoData.devInfo[info].length; i++)
                {
                    Serial.print(devInfoData.devInfo[info].buffer[i], HEX); /* hex number */
                }
            }
            Serial.print("\r\n");
        }
    }

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_RTD_TEXT_SUPPORT
/*****************************************************************************/
ReturnCode ndefRtdTextDump(const ndefType *type)
{
    uint8_t utfEncoding;
    ndefConstBuffer8 bufLanguageCode;
    ndefConstBuffer bufSentence;
    ReturnCode err;

    err = ndefGetRtdText(type, &utfEncoding, &bufLanguageCode, &bufSentence);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBufferPrint(" Text: \"", &bufSentence, "");

    Serial.print("\" (");
    Serial.print((utfEncoding == TEXT_ENCODING_UTF8 ? "UTF8" : "UTF16"));
    Serial.print(",");

    ndefBuffer8Print(" language code \"", &bufLanguageCode, "\")\r\n");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_RTD_URI_SUPPORT
/*****************************************************************************/
ReturnCode ndefRtdUriDump(const ndefType *type)
{
    ndefConstBuffer bufProtocol;
    ndefConstBuffer bufUriString;
    ReturnCode err;

    err = ndefGetRtdUri(type, &bufProtocol, &bufUriString);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBufferPrint("URI: (", &bufProtocol, ")");
    ndefBufferPrint("", &bufUriString, "\r\n");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_RTD_AAR_SUPPORT
/*****************************************************************************/
ReturnCode ndefRtdAarDump(const ndefType *type)
{
    ndefConstBuffer bufAarString;
    ReturnCode err;

    err = ndefGetRtdAar(type, &bufAarString);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBufferPrint(" AAR Package: ", &bufAarString, "\r\n");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_RTD_WLC_SUPPORT
/*****************************************************************************/
ReturnCode ndefRtdWlcCapabilityDump(const ndefType *type)
{
    ndefTypeRtdWlcCapability wlcCap;
    ReturnCode err;

    err = ndefGetRtdWlcCapability(type, &wlcCap);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" WLC Capabilities: \r\n");
    Serial.print(" WLC Protocol Version: 0x");
    Serial.print(wlcCap.wlcProtocolVersion, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Config - Mode Req: 0x");
    Serial.print(wlcCap.wlcConfigModeReq, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Config - Wait Time Retry: 0x");
    Serial.print(wlcCap.wlcConfigWaitTimeRetry, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Config - Nego Wait: 0x");
    Serial.print(wlcCap.wlcConfigNegoWait, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Config - Rd Conf: 0x");
    Serial.print(wlcCap.wlcConfigRdConf, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC CapWtIntRfu: 0x");
    Serial.print(wlcCap.capWtIntRfu, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC CapWtInt: 0x");
    Serial.print(wlcCap.capWtInt, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC NDEF Rd Wt: 0x");
    Serial.print(wlcCap.ndefRdWt, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC NDEF Write To Int: 0x");
    Serial.print(wlcCap.ndefWriteToInt, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC NDEF Write Wt Int: 0x");
    Serial.print(wlcCap.ndefWriteWtInt, HEX);
    Serial.print("\r\n");

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefRtdWlcStatusInfoDump(const ndefType *type)
{
    ndefTypeRtdWlcStatusInfo wlcStatusInfo;
    ReturnCode err;

    err = ndefGetRtdWlcStatusInfo(type, &wlcStatusInfo);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" WLC Status Info: \r\n");
    Serial.print(" Control byte 1: 0x");
    Serial.print(wlcStatusInfo.controlByte1, HEX);
    Serial.print(" ");

    bool bat = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_BATTERY_LEVEL_MASK;
    bool pow = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_POWER_MASK;
    bool vol = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_VOLTAGE_MASK;
    bool cur = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RECEIVE_CURRENT_MASK;
    bool tpb = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_BATTERY_MASK;
    bool tpw = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_TEMPERATURE_WLCL_MASK;
    bool rfu = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_RFU_MASK;
    bool cb2 = wlcStatusInfo.controlByte1 & NDEF_WLC_STATUSINFO_CONTROLBYTE1_CONTROL_BYTE_2_MASK;

    Serial.print("(0b");
    Serial.print(cb2);
    Serial.print(rfu);
    Serial.print(tpw);
    Serial.print(tpb);
    Serial.print(cur);
    Serial.print(vol);
    Serial.print(pow);
    Serial.print(bat);
    Serial.print(")\r\n");

    Serial.print(" Battery level : 0x");
    Serial.print(wlcStatusInfo.batteryLevel, HEX);
    Serial.print("\r\n");
    Serial.print(" Receive power: 0x");
    Serial.print(wlcStatusInfo.receivePower, HEX);
    Serial.print("\r\n");
    Serial.print(" Receive voltage: 0x");
    Serial.print(wlcStatusInfo.receiveVoltage, HEX);
    Serial.print("\r\n");
    Serial.print(" Receive current: 0x");
    Serial.print(wlcStatusInfo.receiveCurrent, HEX);
    Serial.print("\r\n");
    Serial.print(" Temperature battery: 0x");
    Serial.print(wlcStatusInfo.temperatureBattery, HEX);
    Serial.print("\r\n");
    Serial.print(" Temperature WLCL: 0x");
    Serial.print(wlcStatusInfo.temperatureWlcl, HEX);
    Serial.print("\r\n");
    Serial.print(" RFU: 0x");
    Serial.print(wlcStatusInfo.rfu, HEX);
    Serial.print("\r\n");
    Serial.print(" Control byte 2: 0x");
    Serial.print(wlcStatusInfo.controlByte2, HEX);
    Serial.print("\r\n");

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefRtdWlcInfoDump(const ndefType *type)
{
    ndefTypeRtdWlcPollInfo wlcPollInfo;
    ReturnCode err;

    err = ndefGetRtdWlcPollInfo(type, &wlcPollInfo);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" WLC Poll Info: \r\n");
    Serial.print(" WLC P Tx: 0x");
    Serial.print(wlcPollInfo.pTx, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC WLC-P Cap: 0x");
    Serial.print(wlcPollInfo.wlcPCap, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Power Class: 0x");
    Serial.print(wlcPollInfo.powerClass, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC tot Power State: 0x");
    Serial.print(wlcPollInfo.totPowerSteps, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC cur Power State: 0x");
    Serial.print(wlcPollInfo.curPowerStep, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC next Min Step Inc: 0x");
    Serial.print(wlcPollInfo.nextMinStepInc, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC next Min Step Dec: 0x");
    Serial.print(wlcPollInfo.nextMinStepDec, HEX);
    Serial.print("\r\n");

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefRtdWlcControlDump(const ndefType *type)
{
    ndefTypeRtdWlcListenCtl wlcListenCtl;
    ReturnCode err;

    err = ndefGetRtdWlcListenCtl(type, &wlcListenCtl);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" WLC Listen Control: \r\n");
    Serial.print(" WLC Status Info - Error Flag: 0x");
    Serial.print(wlcListenCtl.statusInfoErrorFlag, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Status Info - Baterry Status: 0x");
    Serial.print(wlcListenCtl.statusInfoBatteryStatus, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Status Info - Cnt: 0x");
    Serial.print(wlcListenCtl.statusInfoCnt, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC WPT Config - WPT Req: 0x");
    Serial.print(wlcListenCtl.wptConfigWptReq, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC WPT Config - WPT Duration: 0x");
    Serial.print(wlcListenCtl.wptConfigWptDuration, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC WPT Config - Info Req: 0x");
    Serial.print(wlcListenCtl.wptConfigInfoReq, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Power Adj Req: 0x");
    Serial.print(wlcListenCtl.powerAdjReq, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Battery Level: 0x");
    Serial.print(wlcListenCtl.batteryLevel, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Drv Info - Flag: 0x");
    Serial.print(wlcListenCtl.drvInfoFlag, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Drv Info - Int: 0x");
    Serial.print(wlcListenCtl.drvInfoInt, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Hold Off Wt Int: 0x");
    Serial.print(wlcListenCtl.holdOffWtInt, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Error Info - Error: 0x");
    Serial.print(wlcListenCtl.errorInfoError, HEX);
    Serial.print("\r\n");
    Serial.print(" WLC Error Info - Temperature: 0x");
    Serial.print(wlcListenCtl.errorInfoTemperature, HEX);
    Serial.print("\r\n");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_RTD_WPCWLC_SUPPORT
/*****************************************************************************/
ReturnCode ndefRtdWpcWlcDump(const ndefType *type)
{
    ndefConstBuffer wpcWlc;
    ReturnCode err;
    uint8_t appliProfile;
    uint32_t len;
    uint32_t i;

    static const uint8_t *ndefKiData[NDEF_KI_V10_PAYLOAD_LENGTH] = {
        (uint8_t *)"Application Profile",
        (uint8_t *)"Version",
        (uint8_t *)"Alive FDT",
        (uint8_t *)"Read Data Start Address",
        (uint8_t *)"Write Data Start Address",
        (uint8_t *)"Read Data Buffer Size",
        (uint8_t *)"Write Data Buffer Size",
        (uint8_t *)"Read Command Code",
        (uint8_t *)"Write Commande Code",
        (uint8_t *)"Maximum T_SLOT FOD",
        (uint8_t *)"Minimum T_POWER",
        (uint8_t *)"T_SUSPEND",
        (uint8_t *)"T_COMM_LAG,MAX",
        (uint8_t *)"Write Sequence Length",
        (uint8_t *)"Minimum Power",
        (uint8_t *)"Maximum Power",
    };

    err = ndefGetRtdWpcWlc(type, &wpcWlc);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" WPC WLC: \r\n");
    appliProfile = wpcWlc.buffer[NDEF_KI_APPLICATION_PROFILE_OFFSET];
    if ((appliProfile == NDEF_KI_APPLICATION_PROFILE) &&
        (wpcWlc.length != NDEF_KI_V10_PAYLOAD_LENGTH))
    {
        Serial.print(" Unexpected buffer length, decoding known fields...\r\n");
    }

    len = (wpcWlc.length < NDEF_KI_V10_PAYLOAD_LENGTH) ? wpcWlc.length : NDEF_KI_V10_PAYLOAD_LENGTH;

    for (i = 0; i < len; i++)
    {
        Serial.print(" Ki ");
        Serial.print((char *)ndefKiData[i]);
        Serial.print(": 0x");
        Serial.print(wpcWlc.buffer[i], HEX);
        Serial.print("\r\n");
    }
    for (i = len; i < wpcWlc.length; i++)
    {
        Serial.print(" Ki payload[");
        Serial.print(i);
        Serial.print("]: 0x");
        Serial.print(wpcWlc.buffer[i], HEX);
        Serial.print("\r\n");
    }

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_RTD_TNEP_SUPPORT
/*****************************************************************************/
ReturnCode ndefRtdTnepServiceParamDump(const ndefType *type)
{
    uint8_t tnepVersion;
    ndefConstBuffer bufServiceUri;
    uint8_t comMode;
    uint8_t minWaitingTime;
    uint8_t maxExtensions;
    uint16_t maxMessageSize;
    ReturnCode err;

    err = ndefGetRtdTnepServiceParameter(type, &tnepVersion, &bufServiceUri, &comMode, &minWaitingTime, &maxExtensions, &maxMessageSize);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" TNEP Service Parameter: \r\n");
    Serial.print(" TNEP version: 0x");
    Serial.print(tnepVersion, HEX);
    Serial.print("\r\n");
    ndefBufferPrint(" TNEP Service URI: ", &bufServiceUri, "\r\n");
    Serial.print(" TNEP communication mode: 0x");
    Serial.print(comMode, HEX);
    Serial.print("\r\n");
    Serial.print(" TNEP min Waiting Time: 0x");
    Serial.print(minWaitingTime, HEX);
    Serial.print("\r\n");
    Serial.print(" TNEP max Extensions:  0x");
    Serial.print(maxExtensions, HEX);
    Serial.print("\r\n");
    Serial.print(" TNEP max Message Size: 0x");
    Serial.print(maxMessageSize, HEX);
    Serial.print("\r\n");

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefRtdTnepServiceSelectDump(const ndefType *type)
{
    ndefConstBuffer bufServiceUri;
    ReturnCode err;

    err = ndefGetRtdTnepServiceSelect(type, &bufServiceUri);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" TNEP Service Select: \r\n");
    ndefBufferPrint(" TNEP Service URI: ", &bufServiceUri, "\r\n");

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefRtdTnepStatusDump(const ndefType *type)
{
    uint8_t statusType;
    ReturnCode err;

    err = ndefGetRtdTnepStatus(type, &statusType);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" TNEP Status: \r\n");
    Serial.print(" TNEP Status type: 0x");
    Serial.print(statusType, HEX);
    Serial.print("\r\n");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_MEDIA_SUPPORT
/*****************************************************************************/
ReturnCode ndefMediaTypeDump(const ndefType *type)
{
    ndefConstBuffer8 bufType;
    ndefConstBuffer bufPayload;
    ReturnCode err;

    err = ndefGetMedia(type, &bufType, &bufPayload);
    if (err != ERR_NONE)
    {
        return err;
    }

    ndefBuffer8Print(" Media Type: ", &bufType, "\r\n");
    ndefBufferPrint(" Payload: ", &bufPayload, "\r\n");

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_BLUETOOTH_SUPPORT
/*****************************************************************************/
ReturnCode ndefBluetoothEirDump(const char *string, const uint8_t *eir, bool verbose)
{
    char message[128];

    if (eir == NULL)
    {
        Serial.print(" EIR");
        Serial.print((string ? string : ""));
        Serial.print(" None\r\n");
        return ERR_PARAM;
    }

    uint8_t eir_length = ndefBluetoothEirLength(eir);
    uint8_t data_length = ndefBluetoothEirDataLength(eir);
    uint8_t type = ndefBluetoothEirType(eir);
    const uint8_t *data = ndefBluetoothEirData((uint8_t *)eir);

    ndefConstBuffer bufEir = {data, data_length};

    snprintf(message, sizeof(message), " EIR%s (EIR length: 0x%.2X, EIR type: 0x%.2X)", string, eir_length, type);

    return ndefBufferDump(message, &bufEir, verbose);
}

/*****************************************************************************/
ReturnCode ndefBluetoothDump(const ndefType *type)
{
    ndefTypeBluetooth bluetooth;
    bool verbose = true;

    const ndefEirTable EIR_list[] = {
        {NDEF_BT_EIR_FLAGS, " Flags:"},
        {NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_16, " ClassUUID16_partial:"},
        {NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_16, " ClassUUID16:"},
        {NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_32, " ClassUUID32_partial:"},
        {NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_32, " ClassUUID32:"},
        {NDEF_BT_EIR_SERVICE_CLASS_UUID_PARTIAL_128, " ClassUUID128_partial:"},
        {NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_128, " ClassUUID128:"},
        {NDEF_BT_EIR_SHORT_LOCAL_NAME, " Short Local Name:"},
        {NDEF_BT_EIR_COMPLETE_LOCAL_NAME, " Local Name:"},
        {NDEF_BT_EIR_TX_POWER_LEVEL, " TxPowerLevel:"},
        {NDEF_BT_EIR_DEVICE_CLASS, " Device Class:"},
        {NDEF_BT_EIR_SIMPLE_PAIRING_HASH, " SimplePairingHash:"},
        {NDEF_BT_EIR_SIMPLE_PAIRING_RANDOMIZER, " SimplePairingRandomizer:"},
        {NDEF_BT_EIR_SECURITY_MANAGER_TK_VALUE, " SecurityManagerTK:"},
        {NDEF_BT_EIR_SECURITY_MANAGER_FLAGS, " Security Manager Flags:"},
        {NDEF_BT_EIR_SLAVE_CONNECTION_INTERVAL_RANGE, " SlaveConnIntervalRange:"},
        {NDEF_BT_EIR_SERVICE_SOLICITATION_16, " ServiceSolicitation16:"},
        {NDEF_BT_EIR_SERVICE_SOLICITATION_128, " ServiceSolicitation128:"},
        {NDEF_BT_EIR_SERVICE_DATA, " ServiceData:"},
        {NDEF_BT_EIR_APPEARANCE, " Appearance:"},
        {NDEF_BT_EIR_LE_DEVICE_ADDRESS, " LE Device Address:"},
        {NDEF_BT_EIR_LE_ROLE, " Role:"},
        {NDEF_BT_EIR_LE_SECURE_CONN_CONFIRMATION_VALUE, " Secure Connection Confirmation Value:"},
        {NDEF_BT_EIR_LE_SECURE_CONN_RANDOM_VALUE, " Secure Connection Random Value:"},
        {NDEF_BT_EIR_MANUFACTURER_DATA, " Manufacturer Data:"},
    };

    if (type == NULL)
    {
        return ERR_PARAM;
    }

    ReturnCode err = ndefGetBluetooth(type, &bluetooth);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" Bluetooth:\r\n");
    Serial.print(" Type: 0x");
    Serial.print(type->id, HEX);
    Serial.print("\r\n");

    /* bufDeviceAddress is reversed, use ndefBluetoothGetEirDataReversed() to overcome this */
    ndefBufferDump(" Device Address:", &bluetooth.bufDeviceAddress, false);

    const uint8_t *eir;
    for (uint32_t i = 0; i < SIZEOF_ARRAY(EIR_list); i++)
    {
        eir = ndefBluetoothGetEir(&bluetooth, EIR_list[i].eirType);
        if ((eir != NULL) || verbose)
        {
            ndefBluetoothEirDump(EIR_list[i].string, eir, false);
        }
    }

    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_16);
    if ((eir != NULL) || verbose)
    {
        Serial.print(" nbUUID16: ");
        Serial.print((ndefBluetoothEirDataLength(eir) / 2U));
        Serial.print("\r\n");
    }
    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_32);
    if ((eir != NULL) || verbose)
    {
        Serial.print(" nbUUID32: ");
        Serial.print((ndefBluetoothEirDataLength(eir) / 4U));
        Serial.print("\r\n");
    }
    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_CLASS_UUID_COMPLETE_128);
    if ((eir != NULL) || verbose)
    {
        Serial.print(" nbUUID128: ");
        Serial.print((ndefBluetoothEirDataLength(eir) / 16U));
        Serial.print("\r\n");
    }

    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_SOLICITATION_16);
    if ((eir != NULL) || verbose)
    {
        Serial.print(" nbServiceSolicitation16: 0x");
        Serial.print((ndefBluetoothEirDataLength(eir) / 2U), HEX);
        Serial.print("\r\n");
    }
    eir = ndefBluetoothGetEir(&bluetooth, NDEF_BT_EIR_SERVICE_SOLICITATION_128);
    if ((eir != NULL) || verbose)
    {
        Serial.print(" nbServiceSolicitation128: 0x");
        Serial.print((ndefBluetoothEirDataLength(eir) / 16U), HEX);
        Serial.print("\r\n");
    }

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_VCARD_SUPPORT
/*****************************************************************************/
ReturnCode ndefMediaVCardDump(const ndefType *type)
{
    ndefTypeVCard vCard;

    const uint8_t NDEF_VCARD_N[] = "N";
    const uint8_t NDEF_VCARD_FN[] = "FN";
    const uint8_t NDEF_VCARD_ADR[] = "ADR";
    const uint8_t NDEF_VCARD_TEL[] = "TEL";
    const uint8_t NDEF_VCARD_EMAIL[] = "EMAIL";
    const uint8_t NDEF_VCARD_TITLE[] = "TITLE";
    const uint8_t NDEF_VCARD_ORG[] = "ORG";
    const uint8_t NDEF_VCARD_URL[] = "URL";
    const uint8_t NDEF_VCARD_PHOTO[] = "PHOTO";

    const ndefConstBuffer bufVCard_N = {NDEF_VCARD_N, sizeof(NDEF_VCARD_N) - 1U};
    const ndefConstBuffer bufVCard_FN = {NDEF_VCARD_FN, sizeof(NDEF_VCARD_FN) - 1U};
    const ndefConstBuffer bufVCard_ADR = {NDEF_VCARD_ADR, sizeof(NDEF_VCARD_ADR) - 1U};
    const ndefConstBuffer bufVCard_TEL = {NDEF_VCARD_TEL, sizeof(NDEF_VCARD_TEL) - 1U};
    const ndefConstBuffer bufVCard_EMAIL = {NDEF_VCARD_EMAIL, sizeof(NDEF_VCARD_EMAIL) - 1U};
    const ndefConstBuffer bufVCard_TITLE = {NDEF_VCARD_TITLE, sizeof(NDEF_VCARD_TITLE) - 1U};
    const ndefConstBuffer bufVCard_ORG = {NDEF_VCARD_ORG, sizeof(NDEF_VCARD_ORG) - 1U};
    const ndefConstBuffer bufVCard_URL = {NDEF_VCARD_URL, sizeof(NDEF_VCARD_URL) - 1U};
    const ndefConstBuffer bufVCard_PHOTO = {NDEF_VCARD_PHOTO, sizeof(NDEF_VCARD_PHOTO) - 1U};

    const ndefVCardTypeTable vCardType[] = {
        {bufVCard_N, "Name"},
        {bufVCard_FN, "Formatted Name"},
        {bufVCard_ADR, "Address"},
        {bufVCard_TEL, "Phone"},
        {bufVCard_EMAIL, "Email"},
        {bufVCard_TITLE, "Title"},
        {bufVCard_ORG, "Org"},
        {bufVCard_URL, "URL"},
        {bufVCard_PHOTO, "Photo"},
    };

    ReturnCode err = ndefGetVCard(type, &vCard);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" vCard decoded: \r\n");

    for (uint32_t i = 0; i < SIZEOF_ARRAY(vCardType); i++)
    {
        ndefConstBuffer bufProperty;
        err = ndefVCardGetProperty(&vCard, &vCardType[i].bufType, &bufProperty);
        if (err == ERR_NONE)
        {
            ndefConstBuffer bufType, bufSubtype, bufValue;
            err = ndefVCardParseProperty(&bufProperty, &bufType, &bufSubtype, &bufValue);
            if (err != ERR_NONE)
            {
                Serial.print("ndefVCardParseProperty error ");
                Serial.print(err);
                Serial.print("\r\n");
            }

            if (bufValue.buffer != NULL)
            {
                /* Type */
                Serial.print(" ");
                Serial.print(vCardType[i].string);

                /* Subtype, if any */
                if (bufSubtype.buffer != NULL)
                {
                    ndefBufferPrint(" (", &bufSubtype, ")");
                }

                /* Value */
                if (ndefBufferMatch(&bufType, &bufVCard_PHOTO) == false)
                {
                    ndefBufferPrint(": ", &bufValue, "\r\n");
                }
                else
                {
                    Serial.print("Photo: <Not displayed>\r\n");
                }
            }
        }
    }

    return ERR_NONE;
}
#endif

#if NDEF_TYPE_WIFI_SUPPORT
/*****************************************************************************/
ReturnCode ndefMediaWifiDump(const ndefType *type)
{
    ndefTypeWifi wifiConfig;

    ReturnCode err = ndefGetWifi(type, &wifiConfig);
    if (err != ERR_NONE)
    {
        return err;
    }

    Serial.print(" Wifi config: \r\n");
    ndefBufferDump(" Network SSID:", &wifiConfig.bufNetworkSSID, false);
    ndefBufferDump(" Network Key:", &wifiConfig.bufNetworkKey, false);
    Serial.print(" Authentication: ");
    Serial.print(wifiConfig.authentication);
    Serial.print("\r\n");
    Serial.print(" Encryption: ");
    Serial.print(wifiConfig.encryption);
    Serial.print("\r\n");

    return ERR_NONE;
}
#endif

/*****************************************************************************/
ReturnCode ndefTypeDump(const ndefType *type)
{
    static const ndefTypeDumpTable typeDumpTable[] = {
        {NDEF_TYPE_ID_NONE, NULL},
#if NDEF_TYPE_FLAT_SUPPORT
        {NDEF_TYPE_ID_FLAT, ndefFlatPayloadTypeDump},
#endif
#if NDEF_TYPE_EMPTY_SUPPORT
        {NDEF_TYPE_ID_EMPTY, ndefEmptyTypeDump},
#endif
#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
        {NDEF_TYPE_ID_RTD_DEVICE_INFO, ndefRtdDeviceInfoDump},
#endif
#if NDEF_TYPE_RTD_TEXT_SUPPORT
        {NDEF_TYPE_ID_RTD_TEXT, ndefRtdTextDump},
#endif
#if NDEF_TYPE_RTD_URI_SUPPORT
        {NDEF_TYPE_ID_RTD_URI, ndefRtdUriDump},
#endif
#if NDEF_TYPE_RTD_AAR_SUPPORT
        {NDEF_TYPE_ID_RTD_AAR, ndefRtdAarDump},
#endif
#if NDEF_TYPE_RTD_WLC_SUPPORT
        {NDEF_TYPE_ID_RTD_WLCCAP, ndefRtdWlcCapabilityDump},
        {NDEF_TYPE_ID_RTD_WLCSTAI, ndefRtdWlcStatusInfoDump},
        {NDEF_TYPE_ID_RTD_WLCINFO, ndefRtdWlcInfoDump},
        {NDEF_TYPE_ID_RTD_WLCCTL, ndefRtdWlcControlDump},
#endif
#if NDEF_TYPE_RTD_WPCWLC_SUPPORT
        {NDEF_TYPE_ID_RTD_WPCWLC, ndefRtdWpcWlcDump},
#endif
#if NDEF_TYPE_RTD_TNEP_SUPPORT
        {NDEF_TYPE_ID_RTD_TNEP_SERVICE_PARAMETER, ndefRtdTnepServiceParamDump},
        {NDEF_TYPE_ID_RTD_TNEP_SERVICE_SELECT, ndefRtdTnepServiceSelectDump},
        {NDEF_TYPE_ID_RTD_TNEP_STATUS, ndefRtdTnepStatusDump},
#endif
#if NDEF_TYPE_MEDIA_SUPPORT
    // ndefMediaTypeDump
#endif
#if NDEF_TYPE_BLUETOOTH_SUPPORT
        {NDEF_TYPE_ID_BLUETOOTH_BREDR, ndefBluetoothDump},
        {NDEF_TYPE_ID_BLUETOOTH_LE, ndefBluetoothDump},
        {NDEF_TYPE_ID_BLUETOOTH_SECURE_BREDR, ndefBluetoothDump},
        {NDEF_TYPE_ID_BLUETOOTH_SECURE_LE, ndefBluetoothDump},
#endif
#if NDEF_TYPE_VCARD_SUPPORT
        {NDEF_TYPE_ID_MEDIA_VCARD, ndefMediaVCardDump},
#endif
#if NDEF_TYPE_WIFI_SUPPORT
        {NDEF_TYPE_ID_MEDIA_WIFI, ndefMediaWifiDump}
#endif
    };

    if (type == NULL)
    {
        return ERR_PARAM;
    }

    for (uint32_t i = 0; i < SIZEOF_ARRAY(typeDumpTable); i++)
    {
        if (type->id == typeDumpTable[i].typeId)
        {
            /* Call the appropriate function to the matching record type */
            if (typeDumpTable[i].dump != NULL)
            {
                return typeDumpTable[i].dump(type);
            }
        }
    }

    return ERR_NOT_IMPLEMENTED;
}

/*****************************************************************************/
ReturnCode ndefRecordDumpType(const ndefRecord *record)
{
    ReturnCode err;
    ndefType type;

    err = ndefRecordToType(record, &type);
    if (err != ERR_NONE)
    {
        return err;
    }

    return ndefTypeDump(&type);
}

/*****************************************************************************/
static ReturnCode ndefBufferDumpLine(const uint8_t *buffer, const uint32_t offset, uint32_t lineLength, uint32_t remaining)
{
    uint32_t j;

    if (buffer == NULL)
    {
        return ERR_PARAM;
    }

    Serial.print(" [");
    Serial.print(offset, HEX);
    Serial.print("] ");

    /* Dump hex data */
    for (j = 0; j < remaining; j++)
    {
        Serial.print(buffer[offset + j], HEX);
        Serial.print(" ");
    }
    /* Fill hex section if needed */
    for (j = 0; j < lineLength - remaining; j++)
    {
        Serial.print("   ");
    }

    /* Dump characters */
    Serial.print("|");
    for (j = 0; j < remaining; j++)
    {
        /* Dump only ASCII characters, otherwise replace with a '.' */
        Serial.print((isPrintableASCII(&buffer[offset + j], 1) ? (char)buffer[offset + j] : '.'));
    }
    /* Fill ASCII section if needed */
    for (j = 0; j < lineLength - remaining; j++)
    {
        Serial.print("  ");
    }
    Serial.print(" |\r\n");

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefBufferDump(const char *string, const ndefConstBuffer *bufPayload, bool verbose)
{
    uint32_t bufferLengthMax = 32;
    const uint32_t lineLength = 8;
    uint32_t displayed;
    uint32_t remaining;
    uint32_t offset;

    if ((string == NULL) || (bufPayload == NULL))
    {
        return ERR_PARAM;
    }

    displayed = bufPayload->length;
    remaining = bufPayload->length;

    Serial.print(string);
    Serial.print(" (length ");
    Serial.print(bufPayload->length);
    Serial.print(")\r\n");
    if (bufPayload->buffer == NULL)
    {
        Serial.print(" <No chunk payload buffer>\r\n");
        return ERR_NONE;
    }

    if (verbose == true)
    {
        bufferLengthMax = 256;
    }
    if (bufPayload->length > bufferLengthMax)
    {
        /* Truncate output */
        displayed = bufferLengthMax;
    }

    for (offset = 0; offset < displayed; offset += lineLength)
    {
        ndefBufferDumpLine(bufPayload->buffer, offset, lineLength, remaining > lineLength ? lineLength : remaining);
        remaining -= lineLength;
    }

    if (displayed < bufPayload->length)
    {
        Serial.print(" ... (truncated)\r\n");
    }

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefBufferPrint(const char *prefix, const ndefConstBuffer *bufString, const char *suffix)
{
    uint32_t i;

    if ((prefix == NULL) || (bufString == NULL) || (suffix == NULL))
    {
        return ERR_PARAM;
    }

    Serial.print(prefix);
    if (bufString->buffer == NULL)
    {
        Serial.print("<No buffer>");
    }
    else
    {
        for (i = 0; i < bufString->length; i++)
        {
            Serial.print((char)bufString->buffer[i]);
        }
    }
    Serial.print(suffix);

    return ERR_NONE;
}

/*****************************************************************************/
ReturnCode ndefBuffer8Print(const char *prefix, const ndefConstBuffer8 *bufString, const char *suffix)
{
    ndefConstBuffer buf;

    if (bufString == NULL)
    {
        return ERR_PARAM;
    }

    buf.buffer = bufString->buffer;
    buf.length = bufString->length;

    return ndefBufferPrint(prefix, &buf, suffix);
}

char *hex2Str(unsigned char *data, size_t dataLen)
{
    unsigned char *pin = data;
    const char *hex = "0123456789ABCDEF";
    char *pout = hexStr[hexStrIdx];
    uint8_t i = 0;
    uint8_t idx = hexStrIdx;
    size_t len;

    if (dataLen == 0)
    {
        pout[0] = 0;
    }
    else
    {
        /* Trim data that doesn't fit in buffer */
        len = MIN(dataLen, (MAX_HEX_STR_LENGTH / 2));

        for (; i < (len - 1); ++i)
        {
            *pout++ = hex[(*pin >> 4) & 0xF];
            *pout++ = hex[(*pin++) & 0xF];
        }
        *pout++ = hex[(*pin >> 4) & 0xF];
        *pout++ = hex[(*pin) & 0xF];
        *pout = 0;
    }

    hexStrIdx++;
    hexStrIdx %= MAX_HEX_STR;

    return hexStr[idx];
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
