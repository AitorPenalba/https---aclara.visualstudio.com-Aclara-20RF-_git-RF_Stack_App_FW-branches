/***********************************************************************************************************************
 *
 * Filename: radio.c
 *
 * Global Designator: RADIO_
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2018-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

#include "project.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#endif
#include "compileswitch.h"
#include "compiler_types.h"
#include "buffer.h"
#include "PHY_Protocol.h"
#include "phy.h"
#include "mac.h"
#include "time_sys.h"
#include "radio.h"
#include "radio_hal.h"
#include "xradio.h"
#include "si446x_api_lib.h"
#include "si446x_cmd.h"
#include "si446x_prop.h"
#include "DBG_SerialDebug.h"
#include "time_util.h"
#include "ascii.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "gpio.h"
#endif
#include "rs.h"
#include "rsfec.h"
#include "sys_clock.h" //DCU2+ or EP
#include "pwr_task.h"
#if ( EP == 1 )
#include "spi_mstr.h"
#include "SoftDemodulator.h"
#include "FTM.h"
#if ( PHASE_DETECTION == 1 )
#include "PhaseDetect.h"
#endif
#endif
#if ( DCU == 1 )
#include "version.h"
#endif
#if ( USE_USB_MFG != 0 )
#include "virtual_com.h"
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
#include "EVL_event_log.h"
#include "SM_Protocol.h"
#include "SM.h"
#endif

/*****************************************************************************
 *  Local Macros & Definitions
 *****************************************************************************/
/*lint -esym(750,ALPHA,AFC_MAX_OFFSET) not referenced */
#define FIFO_SIZE         128 // Up to 128 bytes can be in the radio FIFO

#define FIFO_FULL_THRESHOLD_HARD 4 // Don't change this. The radio threshold granularity is 4.
#if ( EP == 1 )
#define FIFO_FULL_THRESHOLD_SOFT MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT //From SoftDemodulator.h
#endif
#define FIFO_ALMOST_EMPTY_THRESHOLD 64 /*!< Generate TX FIFO almost empty threshold. */
#define ALPHA          0.025f // Moving average alpha coeficient. This is similar to a 100 samples average.
#define AFC_MAX_OFFSET     83 // Limit the AFC change to avoid sudden jump.
#define RSSI_AVERAGE_SIZE  10 // Used to average N RSSI measurements
#define RSSI_CLUSTER_SIZE   6 // How many RSSI values need to be close together to consider the RSSI values stable
#if ( DCU == 1 ) || ( TEST_TDMA == 1 )
#define START_TX_FUDGE_FACTOR  162216U   //  This is the time needed to process the START_TX command plus some other fudge factor. Value is 1.3518 msec in 120MHz unit
#define FRACSEC_TEN_MSEC      1200000    //  Value is 10 msec in 120MHz unit
#define FRACSEC_HALF_MSEC       60000    //  Value is 0.5 msec in 120MHz unit
#define FUTURE_TX_TIME      120000000    //  Future TX time must be less than this. 1 second in 120MHz unit
#endif

#define MAX_CCA_TRIES     100 // Used by noise floor to limit the maximum of time CCA can be called to get a valid reading

#define COMPARE             0 // Used to compare the template of different radio configurations

#define MINIMUM_RSSI_VALUE 10 // Clip the RSSI to a minimum. 10 is -129dBm. Systems requested that.

#define TEMPERATURE_MIN   -40 // Minimum valid radio temperature
#define TEMPERATURE_MAX    98 // Maximum valid radio temperature

#if ( EP == 1 )
#define SUPER_CAP_MIN_VOLTAGE 2.10f // TODO: RA6E1 Bob: does this vary from Maxim to TI?  Minimum voltage required to compute noise estimate with boost capacitor turned on
#endif

#define ENABLE_PSM          0 // Set to 1 to use preamble sense mode
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
#define VSWR_LIMIT_COUNT  25
#define TX_PWR_LOSS_COUNT 25
#endif

/*!
 * RADIO PA Mode
 */
typedef enum
{
   eRADIO_PA_MODE_RX = 0, /*!< RADIO PA MODE - Receive   */
   eRADIO_PA_MODE_TX      /*!< RADIO PA MODE - Transmit  */
} RADIO_PA_MODE_t;

typedef struct
{
   uint8_t Group;
   uint8_t Number;
   uint8_t mode[( uint8_t )ePHY_MODE_LAST];
} tModeConfiguration;

#if ( DCU == 1 )
typedef struct{
   uint32_t     port;                 /* GPIO_PORT_A   GPIO_PORT_B   GPIO_PORT_C  GPIO_PORT_D  GPIO_PORT_E  GPIO_PORT_F */
   uint32_t     pin;                  /* GPIO_PIN1   GPIO_PIN2 ....  GPIO_PIN31 */
   void         (*callback)(void);    /* Interrupt function to be called */
} INT_CONFIG_t;
#endif

/*****************************************************************************
 *  Global Variables
 *****************************************************************************/
// WARNING: this must match RADIO_MODE_t enum order
#if ( COMPARE == 0 )
static const tRadioConfiguration * const pRadioConfiguration[] = { &RadioConfiguration_normal,           // eRADIO_MODE_NORMAL
                                                                   &RadioConfiguration_normal,           // eRADIO_MODE_PN9
                                                                   &RadioConfiguration_normal,           // eRADIO_MODE_CW
                                                                   &RadioConfiguration_normal,           // eRADIO_MODE_4GFSK_BER      NOTE: Deprecated. Default to normal
#if ( PORTABLE_DCU == 1)
                                                                   &RadioConfiguration_normal,           // eRADIO_MODE_NORMAL_BER
                                                                   &RadioConfiguration_normal_dev_600 }; // eRADIO_MODE_NORMAL_DEV_600
#else
                                                                   &RadioConfiguration_normal };         // eRADIO_MODE_NORMAL_BER
#endif

#else
static const tRadioConfiguration * const pRadioConfiguration[] = { &RadioConfiguration_normal,
                                                                   &RadioConfiguration_STAR_7200,
                                                                   &RadioConfiguration_STAR_2400,
                                                                   &RadioConfiguration_normal_dev_700_IF_6_69_TCXO_29_491,
                                                                   &RadioConfiguration_normal_dev_700_IF_7_43_TCXO_29_491,
                                                                   &RadioConfiguration_normal_dev_800_IF_7_43_TCXO_29_491,
                                                                   &RadioConfiguration_normal_dev_800_IF_8_23_TCXO_29_491  };
#endif

extern uint32_t DMA_Complete_IRQ_ISR_Timestamp; // Used for a watchdog to make sure we are always doing TCXO triming

/*****************************************************************************
 *  Local Variables
 *****************************************************************************/
static RadioEvent_Fxn pEventHandler_Fxn;


#if ( MCU_SELECTED == NXP_K24 )
static void Radio0_IRQ_ISR(void);
#elif ( MCU_SELECTED == RA6E1 )
void Radio0_IRQ_ISR(external_irq_callback_args_t * p_args);
#endif

#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
static float computeAverage( float const *samples, uint32_t numSamples );
#endif

#if ( DCU == 1 )
static LWGPIO_STRUCT radioIRQIntPin[( uint8_t )MAX_RADIO];  // Radio IRQ Interrupt Pin
static void Radio1_IRQ_ISR(void);
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) // T-board 101-9985T
static void Radio2_IRQ_ISR(void);
static void Radio3_IRQ_ISR(void);
static void Radio4_IRQ_ISR(void);
static void Radio5_IRQ_ISR(void);
static void Radio6_IRQ_ISR(void);
static void Radio7_IRQ_ISR(void);
static void Radio8_IRQ_ISR(void);
static void Radio0_TxDone_ISR(void);
#endif
#endif

#if ( DCU == 1 )
static const INT_CONFIG_t radioIntConfig[] = { // Radio IRQ Interrupt Configuration
#if ( EP == 1 )                                             // All NICs
//   { GPIO_PORT_B, GPIO_PIN0, Radio0_IRQ_ISR },              // Radio 0 IRQ  This is now indirectly handled by a FTM1_CH0 interrupt.
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )  // T-board 101-9975T
   { GPIO_PORT_A, GPIO_PIN13, Radio0_IRQ_ISR },             // Radio 0 IRQ
   { GPIO_PORT_D, GPIO_PIN0,  Radio1_IRQ_ISR },             // Radio 1 through radio 8 IRQ (tied together)
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) // T-board 101-9985T
   { GPIO_PORT_A, GPIO_PIN12, Radio0_IRQ_ISR },             // Radio 0 IRQ U901
   { GPIO_PORT_D, GPIO_PIN7,  Radio1_IRQ_ISR },             // Radio 1 IRQ U800
   { GPIO_PORT_B, GPIO_PIN5,  Radio2_IRQ_ISR },             // Radio 2 IRQ U700
   { GPIO_PORT_A, GPIO_PIN26, Radio3_IRQ_ISR },             // Radio 3 IRQ U500
   { GPIO_PORT_D, GPIO_PIN0,  Radio4_IRQ_ISR },             // Radio 4 IRQ U600
   { GPIO_PORT_E, GPIO_PIN6,  Radio5_IRQ_ISR },             // Radio 5 IRQ U400
   { GPIO_PORT_E, GPIO_PIN12, Radio6_IRQ_ISR },             // Radio 6 IRQ U300
   { GPIO_PORT_E, GPIO_PIN10, Radio7_IRQ_ISR },             // Radio 7 IRQ U100
   { GPIO_PORT_E, GPIO_PIN28, Radio8_IRQ_ISR },             // Radio 8 IRQ U200
   { GPIO_PORT_A, GPIO_PIN7,  Radio0_TxDone_ISR },          // Radio 0 GPIO0 U901
#else
#error "unsuported hardware in radio.c"
#endif
};
#endif

static RADIO_MODE_t currentTxMode;

#if (DCU == 1)
#ifndef SD_RSSI_PIPELINE_DELAY
#define SD_RSSI_PIPELINE_DELAY 0
#warning "Soft Demodulator not included in project. prevRSSI size reduced"    /*lint !e10 !e16 #warning is not recognized  */
#endif
#endif

static struct
{
   OS_TICK_Struct CallibrationTime;     // Time for next calibration
   OS_TICK_Struct TxWatchDog;           // Time when TX was started.  Used as a watch dog.
   TIMESTAMP_t syncTime;                // Sync word interrupt dectection timestamp
   TIMESTAMP_t tentativeSyncTime;       // Tentative Sync word dectection timestamp
   float32_t   error;                   // Frequency offset error
   uint32_t    lastFIFOFullTimeStamp;   // Last time the soft-demodulator FIFO almost full interrupts was processed
   uint32_t    RxStartCYCCNT;           // Time of last restart (DWT_CYCCNT)
   uint32_t    intFIFOCYCCNT;           // When the last FIFO interrupt happened (DWT_CYCCNT)
   uint32_t    syncTimeCYCCNT;          // Sync word dectection timestamp in DWT_CYCCNT.
   bool        RxStartOccured;          // Indicates that a restart occured, and lastRestartTimeStamp has not overflowed
   union {
      RX_FRAME_t RxBuffer;              // Buffer of RX radio data to be sent to PHY
      TX_FRAME_t TxBuffer;              // Buffer of data that will be transmitted
   } buf;
   uint16_t rx_channel;                 // Channel the radio is receiving on
   uint16_t bytePos;                    // Position of next byte to write in buffer
   uint16_t length;                     // Message length (from header)
   int16_t  frequencyOffset;            // Frequency offset as reported by the radio
   int16_t  CWOffset;                   // CW Offset as reported by the radio (i.e. raw unit)
   bool     configured;                 // Radio was succesfully configured
   bool     validHeader;                // Header valid or not
   bool     isDeviceTX;                 // TRUE when transmitting
   bool     adjustTCXO;                 // TRUE when TCXO needs to be adjusted
   uint8_t  currentRSSI;                // For soft-demod. RSSI is continiously captured. This is the lastest value.
   uint8_t  rssi[56];                   // RSSI values in dB captured at regular interval
   uint8_t  rssiIndex;                  // Index into RSSI array
   uint8_t  prev_rssi[SD_RSSI_PIPELINE_DELAY+5]; // RSSI values in dB buffered for Soft Demodulator pipeline delay
   uint8_t  prev_rssiIndex;             // Index into RSSI array
   uint8_t  CCArssi;                    // In dB
   uint8_t  RssiJumpThreshold;          // Current RSSI jump threshold
   uint8_t  RadioBuffer[PHY_MAX_FRAME]; // Buffer for received data
   uint8_t  demodulator;                // Demodulator used by the each receiver. SOFT or HARD.
   PHY_DETECTION_e detection;           // The preamble and sync detection used
   PHY_FRAMING_e   framing;             // The framing used: SRFN, STAR Gen I or Gen II
   PHY_MODE_e      mode;                // The modulation mode used
   PHY_MODE_PARAMETERS_e modeParameters;// The mode parameters used in conjunction with mode 1
   uint32_t          TCXOFreq;          // (Almost) real TCXO freq
                                        // For T-board, this frequency will be adjusted if a GPS board is present.
                                        // For EPs, this frequency will be adjusted by converging to an average of all DCUs TX frequencies.
   uint32_t          TCXOLastUpdate;    // Last time TCXO freq was updated.
   TIME_SYS_SOURCE_e TCXOsource;        // Source of frequency computation
} radio[( uint8_t )MAX_RADIO];

#if ( VSWR_MEASUREMENT == 1 )
         float    cachedVSWR              = 0.0f;
#endif

#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
static   bool     VSWRholdOff             = (bool)false; /* Set when VSWR limit exceeded and stack placed in MUTE mode. */
static   uint32_t sampleNumber            = 0;
static   uint32_t VSWRlimitExceededCnt    = 0;
static   uint32_t VSWRwarningExceededCnt  = 0;
static   uint32_t TXPowerLossCounter      = 0;
static   float    cachedForwardVolts      = 0.0f;
static   float    cachedReflectedVolts    = 0.0f;
static   float    FwdPwrSamples[VSWR_AVERAGE_COUNT];
static   float    ReflPwrSamples[VSWR_AVERAGE_COUNT];

#define UNIT_TEST 0  /* Turn on for VSWR/TxPower unit testing only   */
#if ( UNIT_TEST != 0 )
#warning "radio.h UNIT_TEST is enabled! Do Not Release!"
float    VSWRmult  = 1.0f;
float    TxPwrMult = 1.0f;
#endif

#endif

bool MFG_Port_Print; // Used to print some PHY result to the MFG port

// List of propreties that are different between PHY modes
// Column 0 is ePHY_MODE_0 = 0,           /*!< 4-GFSK,                      , 4800 baud */ SRFN
// Column 1 is ePHY_MODE_1,               /*!< 4-GFSK,                      , 4800 baud */ SRFN (same as column 0)
// Column 2 is ePHY_MODE_2,               /*!< 2-FSK,   (63,59) Reed-Solomon, 7200 baud */ STAR Gen II
// Column 2 is ePHY_MODE_3,               /*!< 2-FSK,                       , 2400 baud */ STAR Gen I
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) // T-board 101-9985T
#if ( TEST_DEVIATION == 1 )
static const tModeConfiguration modeConfiguration [] = {
                                    // Dev 700   700   800   800
                                    // IF  6.69  7.43  7.43  8.23
   { 0x12, 0x06, { 0x22, 0x22, 0x03, 0x03, 0x22, 0x22, 0x22, 0x22 } }, // PKT
   { 0x12, 0x0F, { 0x14, 0x14, 0x04, 0x04, 0x14, 0x14, 0x14, 0x14 } },
   { 0x20, 0x00, { 0x05, 0x05, 0x03, 0x03, 0x05, 0x05, 0x05, 0x05 } }, // Modem
   { 0x20, 0x03, { 0x02, 0x02, 0x04, 0x01, 0x02, 0x02, 0x02, 0x02 } },
   { 0x20, 0x04, { 0xEE, 0xEE, 0x6A, 0x77, 0xEE, 0xEE, 0xEE, 0xEE } },
   { 0x20, 0x0C, { 0xD5, 0xD5, 0x8E, 0x8E, 0x95, 0x95, 0xAB, 0xAB } },
   { 0x20, 0x18, { 0x05, 0x05, 0x01, 0x01, 0x05, 0x05, 0x05, 0x05 } },
   { 0x20, 0x1E, { 0xB0, 0xB0, 0x70, 0xF0, 0xB0, 0xB0, 0xB0, 0xB0 } },
   { 0x20, 0x1F, { 0x20, 0x20, 0x10, 0x20, 0x20, 0x20, 0x20, 0x20 } },
   { 0x20, 0x23, { 0x60, 0x60, 0x55, 0x60, 0x60, 0x60, 0x60, 0x60 } },
   { 0x20, 0x24, { 0x05, 0x05, 0x06, 0x05, 0x05, 0x05, 0x05, 0x05 } },
   { 0x20, 0x25, { 0x55, 0x55, 0x06, 0x55, 0x55, 0x55, 0x55, 0x55 } },
   { 0x20, 0x26, { 0x55, 0x55, 0xD4, 0x55, 0x55, 0x55, 0x55, 0x55 } },
   { 0x20, 0x27, { 0x04, 0x04, 0x07, 0x03, 0x06, 0x06, 0x05, 0x05 } },
   { 0x20, 0x28, { 0x44, 0x44, 0xFF, 0x33, 0x18, 0x18, 0x55, 0x55 } },
   { 0x20, 0x2A, { 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02 } },
   { 0x20, 0x2F, { 0x2B, 0x2B, 0x40, 0x15, 0x2B, 0x2B, 0x2B, 0x2B } },
   { 0x20, 0x30, { 0x01, 0x01, 0x00, 0x02, 0x01, 0x01, 0x01, 0x01 } }, // AFC_LIMITER 1 WARNING!!! SRFN 9600 AFC limiter was changed from 0x188 to 0x14D to improve ACR
   { 0x20, 0x31, { 0x7F, 0x7F, 0xFC, 0x1E, 0x38, 0x5A, 0x5A, 0x7F } }, // AFC_LIMITER 0            STAR 7200 AFC limiter was changed from 0x105 to 0x0DE to improve ACR
   { 0x20, 0x39, { 0x15, 0x15, 0x13, 0x15, 0x15, 0x15, 0x15, 0x15 } }, //                          STAR 2400 AFC limiter was changed from 0x21E to 0x29A to improve ACR
   { 0x20, 0x3A, { 0x15, 0x15, 0x13, 0x15, 0x15, 0x15, 0x15, 0x15 } },
   { 0x20, 0x3B, { 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00 } },
   { 0x20, 0x3D, { 0x1A, 0x1A, 0x23, 0x6A, 0x12, 0x12, 0x15, 0x15 } },
   { 0x20, 0x3E, { 0xAB, 0xAB, 0x66, 0xAB, 0xAB, 0xAB, 0x55, 0x55 } },
   { 0x20, 0x3F, { 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10 } },
   { 0x20, 0x46, { 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 } },
   { 0x20, 0x47, { 0x0E, 0x0E, 0x87, 0x69, 0xBD, 0xBD, 0xD8, 0xD8 } },
   { 0x20, 0x54, { 0x03, 0x03, 0x03, 0x05, 0x03, 0x03, 0x03, 0x03 } },
   { 0x20, 0x5D, { 0x08, 0x08, 0x04, 0x0B, 0x06, 0x06, 0x06, 0x06 } },
   { 0x21, 0x00, { 0x5B, 0x5B, 0x0C, 0xCC, 0x23, 0x39, 0x39, 0x5B } }, // Modem CHFLT
   { 0x21, 0x01, { 0x47, 0x47, 0x01, 0xA1, 0x17, 0x2B, 0x2B, 0x47 } },
   { 0x21, 0x02, { 0x0F, 0x0F, 0xE4, 0x30, 0xF4, 0x00, 0x00, 0x0F } },
   { 0x21, 0x03, { 0xC0, 0xC0, 0xB9, 0xA0, 0xC2, 0xC3, 0xC3, 0xC0 } },
   { 0x21, 0x04, { 0x6D, 0x6D, 0x86, 0x21, 0x88, 0x7F, 0x7F, 0x6D } },
   { 0x21, 0x05, { 0x25, 0x25, 0x55, 0xD1, 0x50, 0x3F, 0x3F, 0x25 } },
   { 0x21, 0x06, { 0xF4, 0xF4, 0x2B, 0xB9, 0x21, 0x0C, 0x0C, 0xF4 } },
   { 0x21, 0x07, { 0xDB, 0xDB, 0x0B, 0xC9, 0xFF, 0xEC, 0xEC, 0xDB } },
   { 0x21, 0x08, { 0xD6, 0xD6, 0xF8, 0xEA, 0xEC, 0xDC, 0xDC, 0xD6 } },
   { 0x21, 0x09, { 0xDF, 0xDF, 0xEF, 0x05, 0xE6, 0xDC, 0xDC, 0xDF } },
   { 0x21, 0x0A, { 0xEC, 0xEC, 0xEF, 0x12, 0xE8, 0xE3, 0xE3, 0xEC } },
   { 0x21, 0x0B, { 0xF7, 0xF7, 0xF2, 0x11, 0xEE, 0xED, 0xED, 0xF7 } },
   { 0x21, 0x0C, { 0xFE, 0xFE, 0xF8, 0x0A, 0xF6, 0xF6, 0xF6, 0xFE } },
   { 0x21, 0x0D, { 0x01, 0x01, 0xFC, 0x04, 0xFB, 0xFD, 0xFD, 0x01 } },
   { 0x21, 0x0E, { 0x15, 0x15, 0x05, 0x15, 0x05, 0x15, 0x15, 0x15 } },
   { 0x21, 0x0F, { 0xF0, 0xF0, 0x00, 0xFC, 0xC0, 0xC0, 0xC0, 0xF0 } },
   { 0x21, 0x10, { 0xFF, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, 0xFF } },
   { 0x21, 0x11, { 0x03, 0x03, 0x0F, 0x00, 0x0F, 0x0F, 0x0F, 0x03 } },
   { 0x21, 0x12, { 0x5B, 0x5B, 0x0C, 0xCC, 0x23, 0x39, 0x39, 0x5B } },
   { 0x21, 0x13, { 0x47, 0x47, 0x01, 0xA1, 0x17, 0x2B, 0x2B, 0x47 } },
   { 0x21, 0x14, { 0x0F, 0x0F, 0xE4, 0x30, 0xF4, 0x00, 0x00, 0x0F } },
   { 0x21, 0x15, { 0xC0, 0xC0, 0xB9, 0xA0, 0xC2, 0xC3, 0xC3, 0xC0 } },
   { 0x21, 0x16, { 0x6D, 0x6D, 0x86, 0x21, 0x88, 0x7F, 0x7F, 0x6D } },
   { 0x21, 0x17, { 0x25, 0x25, 0x55, 0xD1, 0x50, 0x3F, 0x3F, 0x25 } },
   { 0x21, 0x18, { 0xF4, 0xF4, 0x2B, 0xB9, 0x21, 0x0C, 0x0C, 0xF4 } },
   { 0x21, 0x19, { 0xDB, 0xDB, 0x0B, 0xC9, 0xFF, 0xEC, 0xEC, 0xDB } },
   { 0x21, 0x1A, { 0xD6, 0xD6, 0xF8, 0xEA, 0xEC, 0xDC, 0xDC, 0xD6 } },
   { 0x21, 0x1B, { 0xDF, 0xDF, 0xEF, 0x05, 0xE6, 0xDC, 0xDC, 0xDF } },
   { 0x21, 0x1C, { 0xEC, 0xEC, 0xEF, 0x12, 0xE8, 0xE3, 0xE3, 0xEC } },
   { 0x21, 0x1D, { 0xF7, 0xF7, 0xF2, 0x11, 0xEE, 0xED, 0xED, 0xF7 } },
   { 0x21, 0x1E, { 0xFE, 0xFE, 0xF8, 0x0A, 0xF6, 0xF6, 0xF6, 0xFE } },
   { 0x21, 0x1F, { 0x01, 0x01, 0xFC, 0x04, 0xFB, 0xFD, 0xFD, 0x01 } },
   { 0x21, 0x20, { 0x15, 0x15, 0x05, 0x15, 0x05, 0x15, 0x15, 0x15 } },
   { 0x21, 0x21, { 0xF0, 0xF0, 0x00, 0xFC, 0xC0, 0xC0, 0xC0, 0xF0 } },
   { 0x21, 0x22, { 0xFF, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, 0xFF } },
   { 0x21, 0x23, { 0x03, 0x03, 0x0F, 0x00, 0x0F, 0x0F, 0x0F, 0x03 } },
#if 0  // Not RA6E1.  This was already removed in the K24 baseline code
   // This is not working for now
   { 0x22, 0x00, { 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25 } }, // SYNTH
   { 0x22, 0x01, { 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A } }, // 50kHz loop BW as per AN866.pdf
   { 0x22, 0x02, { 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A } },
   { 0x22, 0x03, { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 } },
   { 0x22, 0x04, { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F } },
   { 0x22, 0x05, { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F } },
   { 0x22, 0x06, { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 } },
#endif
   { 0x00, 0x00, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }
};
#else
static const tModeConfiguration modeConfiguration [] = {
   { 0x12, 0x06, { 0x22, 0x22, 0x03, 0x03 } }, // PKT
   { 0x12, 0x0F, { 0x14, 0x14, 0x04, 0x04 } },
   { 0x20, 0x00, { 0x05, 0x05, 0x03, 0x03 } }, // Modem
   { 0x20, 0x03, { 0x02, 0x02, 0x04, 0x01 } },
   { 0x20, 0x04, { 0xEE, 0xEE, 0x6A, 0x77 } },
   { 0x20, 0x0C, { 0xD5, 0xD5, 0x8E, 0x8E } },
   { 0x20, 0x18, { 0x05, 0x05, 0x01, 0x01 } },
   { 0x20, 0x1E, { 0xB0, 0xB0, 0x70, 0xF0 } },
   { 0x20, 0x1F, { 0x20, 0x20, 0x10, 0x20 } },
   { 0x20, 0x23, { 0x60, 0x60, 0x55, 0x60 } },
   { 0x20, 0x24, { 0x05, 0x05, 0x06, 0x05 } },
   { 0x20, 0x25, { 0x55, 0x55, 0x06, 0x55 } },
   { 0x20, 0x26, { 0x55, 0x55, 0xD4, 0x55 } },
   { 0x20, 0x27, { 0x04, 0x04, 0x07, 0x03 } },
   { 0x20, 0x28, { 0x44, 0x44, 0xFF, 0x33 } },
   { 0x20, 0x2A, { 0x02, 0x02, 0x00, 0x00 } },
   { 0x20, 0x2F, { 0x2B, 0x2B, 0x40, 0x15 } },
   { 0x20, 0x30, { 0x01, 0x01, 0x00, 0x02 } }, // AFC_LIMITER 1 WARNING!!! SRFN 9600 AFC limiter was changed from 0x188 to 0x14D to improve ACR
   { 0x20, 0x31, { 0x4D, 0x4D, 0xDE, 0x9A } }, // AFC_LIMITER 0            STAR 7200 AFC limiter was changed from 0x105 to 0x0DE to improve ACR
   { 0x20, 0x39, { 0x15, 0x15, 0x13, 0x15 } }, //                          STAR 2400 AFC limiter was changed from 0x21E to 0x29A to improve ACR
   { 0x20, 0x3A, { 0x15, 0x15, 0x13, 0x15 } },
   { 0x20, 0x3B, { 0x00, 0x00, 0x80, 0x80 } },
   { 0x20, 0x3D, { 0x1A, 0x1A, 0x23, 0x6A } },
   { 0x20, 0x3E, { 0xAB, 0xAB, 0x66, 0xAB } },
   { 0x20, 0x3F, { 0x10, 0x10, 0x00, 0x00 } },
   { 0x20, 0x46, { 0x01, 0x01, 0x00, 0x01 } },
   { 0x20, 0x47, { 0x0E, 0x0E, 0x87, 0x69 } },
   { 0x20, 0x54, { 0x03, 0x03, 0x03, 0x05 } },
   { 0x20, 0x5D, { 0x08, 0x08, 0x04, 0x0B } },
   { 0x21, 0x00, { 0x5B, 0x5B, 0x0C, 0xCC } }, // Modem CHFLT
   { 0x21, 0x01, { 0x47, 0x47, 0x01, 0xA1 } },
   { 0x21, 0x02, { 0x0F, 0x0F, 0xE4, 0x30 } },
   { 0x21, 0x03, { 0xC0, 0xC0, 0xB9, 0xA0 } },
   { 0x21, 0x04, { 0x6D, 0x6D, 0x86, 0x21 } },
   { 0x21, 0x05, { 0x25, 0x25, 0x55, 0xD1 } },
   { 0x21, 0x06, { 0xF4, 0xF4, 0x2B, 0xB9 } },
   { 0x21, 0x07, { 0xDB, 0xDB, 0x0B, 0xC9 } },
   { 0x21, 0x08, { 0xD6, 0xD6, 0xF8, 0xEA } },
   { 0x21, 0x09, { 0xDF, 0xDF, 0xEF, 0x05 } },
   { 0x21, 0x0A, { 0xEC, 0xEC, 0xEF, 0x12 } },
   { 0x21, 0x0B, { 0xF7, 0xF7, 0xF2, 0x11 } },
   { 0x21, 0x0C, { 0xFE, 0xFE, 0xF8, 0x0A } },
   { 0x21, 0x0D, { 0x01, 0x01, 0xFC, 0x04 } },
   { 0x21, 0x0E, { 0x15, 0x15, 0x05, 0x15 } },
   { 0x21, 0x0F, { 0xF0, 0xF0, 0x00, 0xFC } },
   { 0x21, 0x10, { 0xFF, 0xFF, 0xFF, 0x03 } },
   { 0x21, 0x11, { 0x03, 0x03, 0x0F, 0x00 } },
   { 0x21, 0x12, { 0x5B, 0x5B, 0x0C, 0xCC } },
   { 0x21, 0x13, { 0x47, 0x47, 0x01, 0xA1 } },
   { 0x21, 0x14, { 0x0F, 0x0F, 0xE4, 0x30 } },
   { 0x21, 0x15, { 0xC0, 0xC0, 0xB9, 0xA0 } },
   { 0x21, 0x16, { 0x6D, 0x6D, 0x86, 0x21 } },
   { 0x21, 0x17, { 0x25, 0x25, 0x55, 0xD1 } },
   { 0x21, 0x18, { 0xF4, 0xF4, 0x2B, 0xB9 } },
   { 0x21, 0x19, { 0xDB, 0xDB, 0x0B, 0xC9 } },
   { 0x21, 0x1A, { 0xD6, 0xD6, 0xF8, 0xEA } },
   { 0x21, 0x1B, { 0xDF, 0xDF, 0xEF, 0x05 } },
   { 0x21, 0x1C, { 0xEC, 0xEC, 0xEF, 0x12 } },
   { 0x21, 0x1D, { 0xF7, 0xF7, 0xF2, 0x11 } },
   { 0x21, 0x1E, { 0xFE, 0xFE, 0xF8, 0x0A } },
   { 0x21, 0x1F, { 0x01, 0x01, 0xFC, 0x04 } },
   { 0x21, 0x20, { 0x15, 0x15, 0x05, 0x15 } },
   { 0x21, 0x21, { 0xF0, 0xF0, 0x00, 0xFC } },
   { 0x21, 0x22, { 0xFF, 0xFF, 0xFF, 0x03 } },
   { 0x21, 0x23, { 0x03, 0x03, 0x0F, 0x00 } },
#if 0  // Not RA6E1.  This was already removed in the K24 baseline code
   // This is not working for now
   { 0x22, 0x00, { 0x25, 0x25, 0x25, 0x25 } }, // SYNTH
   { 0x22, 0x01, { 0x0A, 0x0A, 0x0A, 0x0A } }, // 50kHz loop BW as per AN866.pdf
   { 0x22, 0x02, { 0x0A, 0x0A, 0x0A, 0x0A } },
   { 0x22, 0x03, { 0x03, 0x03, 0x03, 0x03 } },
   { 0x22, 0x04, { 0x1F, 0x1F, 0x1F, 0x1F } },
   { 0x22, 0x05, { 0x7F, 0x7F, 0x7F, 0x7F } },
   { 0x22, 0x06, { 0x03, 0x03, 0x03, 0x03 } },
#endif
   { 0x00, 0x00, { 0x00, 0x00, 0x00, 0x00 } }
};
#endif
#else
#if ( EP == 1 ) && ( TEST_DEVIATION == 1 )
static const tModeConfiguration modeConfiguration [] = {
   { 0x12, 0x06, { 0x22, 0x22, 0x03, 0x03, 0x22, 0x22, 0x22 } }, // PKT
   { 0x12, 0x0F, { 0x14, 0x14, 0x04, 0x04, 0x14, 0x14, 0x14 } },
   { 0x20, 0x00, { 0x05, 0x05, 0x03, 0x03, 0x05, 0x05, 0x05 } }, // Modem
   { 0x20, 0x03, { 0x02, 0x02, 0x04, 0x01, 0x02, 0x02, 0x02 } },
   { 0x20, 0x04, { 0xEE, 0xEE, 0x6A, 0x77, 0xEE, 0xEE, 0xEE } },
   { 0x20, 0x0C, { 0xD2, 0xD2, 0x8C, 0x8C, 0x7E, 0x93, 0xA8 } },
   { 0x20, 0x18, { 0x05, 0x05, 0x01, 0x01, 0x05, 0x05, 0x05 } },
   { 0x20, 0x1E, { 0xB0, 0xB0, 0xB0, 0xF0, 0xB0, 0xB0, 0xB0 } },
   { 0x20, 0x23, { 0x62, 0x62, 0x41, 0x62, 0x62, 0x62, 0x62 } },
   { 0x20, 0x24, { 0x05, 0x05, 0x07, 0x05, 0x05, 0x05, 0x05 } },
   { 0x20, 0x25, { 0x3E, 0x3E, 0xE6, 0x3E, 0x3E, 0x3E, 0x3E } },
   { 0x20, 0x26, { 0x2D, 0x2D, 0x37, 0x2D, 0x2D, 0x2D, 0x2D } },
   { 0x20, 0x27, { 0x04, 0x04, 0x07, 0x03, 0x06, 0x05, 0x05 } },
   { 0x20, 0x28, { 0x2E, 0x2E, 0xFF, 0x22, 0x27, 0xF9, 0x39 } },
   { 0x20, 0x2A, { 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x02 } },
   { 0x20, 0x2F, { 0x2A, 0x2A, 0x3F, 0x15, 0x2A, 0x2A, 0x2A } },
   { 0x20, 0x30, { 0x01, 0x01, 0x00, 0x02, 0x01, 0x01, 0x01 } }, // AFC_LIMITER 1 WARNING!!! SRFN 9600 AFC limiter was changed from 0x188 to 0x14D to improve ACR
   { 0x20, 0x31, { 0x4D, 0x4D, 0xDE, 0x9A, 0xB2, 0xB2, 0xE4 } }, // AFC_LIMITER 0            STAR 7200 AFC limiter was changed from 0x105 to 0x0DE to improve ACR
   { 0x20, 0x39, { 0x15, 0x15, 0x0E, 0x15, 0x15, 0x15, 0x15 } }, //                          STAR 2400 AFC limiter was changed from 0x21E to 0x29A to improve ACR
   { 0x20, 0x3A, { 0x15, 0x15, 0x0E, 0x15, 0x15, 0x15, 0x15 } },
   { 0x20, 0x3B, { 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00 } },
   { 0x20, 0x3D, { 0x1A, 0x1A, 0x23, 0x6A, 0x10, 0x12, 0x15 } },
   { 0x20, 0x3E, { 0xAB, 0xAB, 0x66, 0xAB, 0x00, 0xAB, 0x55 } },
   { 0x20, 0x3F, { 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x10 } },
   { 0x20, 0x46, { 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00 } },
   { 0x20, 0x47, { 0x0A, 0x0A, 0xB1, 0x62, 0x9F, 0xBA, 0xD5 } },
   { 0x20, 0x54, { 0x03, 0x03, 0x03, 0x05, 0x03, 0x03, 0x03 } },
   { 0x20, 0x5D, { 0x08, 0x08, 0x05, 0x0A, 0x05, 0x05, 0x06 } },
   { 0x21, 0x00, { 0x5B, 0x5B, 0x5B, 0xCC, 0x7E, 0x7E, 0xA2 } }, // Modem CHFLT
   { 0x21, 0x01, { 0x47, 0x47, 0x47, 0xA1, 0x64, 0x64, 0x81 } },
   { 0x21, 0x02, { 0x0F, 0x0F, 0x0F, 0x30, 0x1B, 0x1B, 0x26 } },
   { 0x21, 0x03, { 0xC0, 0xC0, 0xC0, 0xA0, 0xBA, 0xBA, 0xAF } },
   { 0x21, 0x04, { 0x6D, 0x6D, 0x6D, 0x21, 0x58, 0x58, 0x3F } },
   { 0x21, 0x05, { 0x25, 0x25, 0x25, 0xD1, 0x0B, 0x0B, 0xEE } },
   { 0x21, 0x06, { 0xF4, 0xF4, 0xF4, 0xB9, 0xDD, 0xDD, 0xC8 } },
   { 0x21, 0x07, { 0xDB, 0xDB, 0xDB, 0xC9, 0xCE, 0xCE, 0xC7 } },
   { 0x21, 0x08, { 0xD6, 0xD6, 0xD6, 0xEA, 0xD6, 0xD6, 0xDB } },
   { 0x21, 0x09, { 0xDF, 0xDF, 0xDF, 0x05, 0xE6, 0xE6, 0xF2 } },
   { 0x21, 0x0A, { 0xEC, 0xEC, 0xEC, 0x12, 0xF6, 0xF6, 0x02 } },
   { 0x21, 0x0B, { 0xF7, 0xF7, 0xF7, 0x11, 0x00, 0x00, 0x08 } },
   { 0x21, 0x0C, { 0xFE, 0xFE, 0xFE, 0x0A, 0x03, 0x03, 0x07 } },
   { 0x21, 0x0D, { 0x01, 0x01, 0x01, 0x04, 0x03, 0x03, 0x03 } },
   { 0x21, 0x0E, { 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15 } },
   { 0x21, 0x0F, { 0xF0, 0xF0, 0xF0, 0xFC, 0xF0, 0xF0, 0xFC } },
   { 0x21, 0x10, { 0xFF, 0xFF, 0xFF, 0x03, 0x3F, 0x3F, 0x0F } },
   { 0x21, 0x11, { 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00 } },
   { 0x21, 0x12, { 0x5B, 0x5B, 0x5B, 0xCC, 0x7E, 0x7E, 0xA2 } },
   { 0x21, 0x13, { 0x47, 0x47, 0x47, 0xA1, 0x64, 0x64, 0x81 } },
   { 0x21, 0x14, { 0x0F, 0x0F, 0x0F, 0x30, 0x1B, 0x1B, 0x26 } },
   { 0x21, 0x15, { 0xC0, 0xC0, 0xC0, 0xA0, 0xBA, 0xBA, 0xAF } },
   { 0x21, 0x16, { 0x6D, 0x6D, 0x6D, 0x21, 0x58, 0x58, 0x3F } },
   { 0x21, 0x17, { 0x25, 0x25, 0x25, 0xD1, 0x0B, 0x0B, 0xEE } },
   { 0x21, 0x18, { 0xF4, 0xF4, 0xF4, 0xB9, 0xDD, 0xDD, 0xC8 } },
   { 0x21, 0x19, { 0xDB, 0xDB, 0xDB, 0xC9, 0xCE, 0xCE, 0xC7 } },
   { 0x21, 0x1A, { 0xD6, 0xD6, 0xD6, 0xEA, 0xD6, 0xD6, 0xDB } },
   { 0x21, 0x1B, { 0xDF, 0xDF, 0xDF, 0x05, 0xE6, 0xE6, 0xF2 } },
   { 0x21, 0x1C, { 0xEC, 0xEC, 0xEC, 0x12, 0xF6, 0xF6, 0x02 } },
   { 0x21, 0x1D, { 0xF7, 0xF7, 0xF7, 0x11, 0x00, 0x00, 0x08 } },
   { 0x21, 0x1E, { 0xFE, 0xFE, 0xFE, 0x0A, 0x03, 0x03, 0x07 } },
   { 0x21, 0x1F, { 0x01, 0x01, 0x01, 0x04, 0x03, 0x03, 0x03 } },
   { 0x21, 0x1F, { 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15 } },
   { 0x21, 0x21, { 0xF0, 0xF0, 0xF0, 0xFC, 0xF0, 0xF0, 0xFC } },
   { 0x21, 0x22, { 0xFF, 0xFF, 0xFF, 0x03, 0x3F, 0x3F, 0x0F } },
   { 0x21, 0x23, { 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00 } },
#else
static const tModeConfiguration modeConfiguration [] = {
   { 0x12, 0x06, { 0x22, 0x22, 0x03, 0x03 } }, // PKT
   { 0x12, 0x0F, { 0x14, 0x14, 0x04, 0x04 } },
   { 0x20, 0x00, { 0x05, 0x05, 0x03, 0x03 } }, // Modem
   { 0x20, 0x03, { 0x02, 0x02, 0x04, 0x01 } },
   { 0x20, 0x04, { 0xEE, 0xEE, 0x6A, 0x77 } },
   { 0x20, 0x0C, { 0xD2, 0xD2, 0x8C, 0x8C } },
   { 0x20, 0x18, { 0x05, 0x05, 0x01, 0x01 } },
   { 0x20, 0x1E, { 0xB0, 0xB0, 0xB0, 0xF0 } },
   { 0x20, 0x23, { 0x62, 0x62, 0x41, 0x62 } },
   { 0x20, 0x24, { 0x05, 0x05, 0x07, 0x05 } },
   { 0x20, 0x25, { 0x3E, 0x3E, 0xE6, 0x3E } },
   { 0x20, 0x26, { 0x2D, 0x2D, 0x37, 0x2D } },
   { 0x20, 0x27, { 0x04, 0x04, 0x07, 0x03 } },
   { 0x20, 0x28, { 0x2E, 0x2E, 0xFF, 0x22 } },
   { 0x20, 0x2A, { 0x02, 0x02, 0x00, 0x00 } },
   { 0x20, 0x2F, { 0x2A, 0x2A, 0x3F, 0x15 } },
   { 0x20, 0x30, { 0x01, 0x01, 0x00, 0x02 } }, // AFC_LIMITER 1 WARNING!!! SRFN 9600 AFC limiter was changed from 0x188 to 0x14D to improve ACR
   { 0x20, 0x31, { 0x4D, 0x4D, 0xDE, 0x9A } }, // AFC_LIMITER 0            STAR 7200 AFC limiter was changed from 0x105 to 0x0DE to improve ACR
   { 0x20, 0x39, { 0x15, 0x15, 0x0E, 0x15 } }, //                          STAR 2400 AFC limiter was changed from 0x21E to 0x29A to improve ACR
   { 0x20, 0x3A, { 0x15, 0x15, 0x0E, 0x15 } },
   { 0x20, 0x3B, { 0x00, 0x00, 0x80, 0x80 } },
   { 0x20, 0x3D, { 0x1A, 0x1A, 0x23, 0x6A } },
   { 0x20, 0x3E, { 0xAB, 0xAB, 0x66, 0xAB } },
   { 0x20, 0x3F, { 0x10, 0x10, 0x00, 0x00 } },
   { 0x20, 0x46, { 0x01, 0x01, 0x00, 0x01 } },
   { 0x20, 0x47, { 0x0A, 0x0A, 0xB1, 0x62 } },
   { 0x20, 0x54, { 0x03, 0x03, 0x03, 0x05 } },
   { 0x20, 0x5D, { 0x08, 0x08, 0x05, 0x0A } },
   { 0x21, 0x00, { 0x5B, 0x5B, 0x5B, 0xCC } }, // Modem CHFLT
   { 0x21, 0x01, { 0x47, 0x47, 0x47, 0xA1 } },
   { 0x21, 0x02, { 0x0F, 0x0F, 0x0F, 0x30 } },
   { 0x21, 0x03, { 0xC0, 0xC0, 0xC0, 0xA0 } },
   { 0x21, 0x04, { 0x6D, 0x6D, 0x6D, 0x21 } },
   { 0x21, 0x05, { 0x25, 0x25, 0x25, 0xD1 } },
   { 0x21, 0x06, { 0xF4, 0xF4, 0xF4, 0xB9 } },
   { 0x21, 0x07, { 0xDB, 0xDB, 0xDB, 0xC9 } },
   { 0x21, 0x08, { 0xD6, 0xD6, 0xD6, 0xEA } },
   { 0x21, 0x09, { 0xDF, 0xDF, 0xDF, 0x05 } },
   { 0x21, 0x0A, { 0xEC, 0xEC, 0xEC, 0x12 } },
   { 0x21, 0x0B, { 0xF7, 0xF7, 0xF7, 0x11 } },
   { 0x21, 0x0C, { 0xFE, 0xFE, 0xFE, 0x0A } },
   { 0x21, 0x0D, { 0x01, 0x01, 0x01, 0x04 } },
   { 0x21, 0x0E, { 0x15, 0x15, 0x15, 0x15 } },
   { 0x21, 0x0F, { 0xF0, 0xF0, 0xF0, 0xFC } },
   { 0x21, 0x10, { 0xFF, 0xFF, 0xFF, 0x03 } },
   { 0x21, 0x11, { 0x03, 0x03, 0x03, 0x00 } },
   { 0x21, 0x12, { 0x5B, 0x5B, 0x5B, 0xCC } },
   { 0x21, 0x13, { 0x47, 0x47, 0x47, 0xA1 } },
   { 0x21, 0x14, { 0x0F, 0x0F, 0x0F, 0x30 } },
   { 0x21, 0x15, { 0xC0, 0xC0, 0xC0, 0xA0 } },
   { 0x21, 0x16, { 0x6D, 0x6D, 0x6D, 0x21 } },
   { 0x21, 0x17, { 0x25, 0x25, 0x25, 0xD1 } },
   { 0x21, 0x18, { 0xF4, 0xF4, 0xF4, 0xB9 } },
   { 0x21, 0x19, { 0xDB, 0xDB, 0xDB, 0xC9 } },
   { 0x21, 0x1A, { 0xD6, 0xD6, 0xD6, 0xEA } },
   { 0x21, 0x1B, { 0xDF, 0xDF, 0xDF, 0x05 } },
   { 0x21, 0x1C, { 0xEC, 0xEC, 0xEC, 0x12 } },
   { 0x21, 0x1D, { 0xF7, 0xF7, 0xF7, 0x11 } },
   { 0x21, 0x1E, { 0xFE, 0xFE, 0xFE, 0x0A } },
   { 0x21, 0x1F, { 0x01, 0x01, 0x01, 0x04 } },
   { 0x21, 0x1F, { 0x15, 0x15, 0x15, 0x15 } },
   { 0x21, 0x21, { 0xF0, 0xF0, 0xF0, 0xFC } },
   { 0x21, 0x22, { 0xFF, 0xFF, 0xFF, 0x03 } },
   { 0x21, 0x23, { 0x03, 0x03, 0x03, 0x00 } },
#endif
#if 0  // Not RA6E1.  This was already removed in the K24 baseline code
   // This is not working for now
   { 0x22, 0x00, { 0x25, 0x25, 0x25, 0x25 } }, // SYNTH
   { 0x22, 0x01, { 0x0A, 0x0A, 0x0A, 0x0A } }, // 50kHz loop BW as per AN866.pdf
   { 0x22, 0x02, { 0x0A, 0x0A, 0x0A, 0x0A } },
   { 0x22, 0x03, { 0x03, 0x03, 0x03, 0x03 } },
   { 0x22, 0x04, { 0x1F, 0x1F, 0x1F, 0x1F } },
   { 0x22, 0x05, { 0x7F, 0x7F, 0x7F, 0x7F } },
   { 0x22, 0x06, { 0x03, 0x03, 0x03, 0x03 } },
#endif
   { 0x00, 0x00, { 0x00, 0x00, 0x00, 0x00 } }
};
#endif

/*********************************************************************/
/* REVERSE CRC-CCITT nibble Lookup Table                             */
/* Taken from 9975J board code                                       */
static const unsigned short rncrctab[] =
{
   0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
   0x8408, 0x9489, 0xA50A, 0xB58B, 0xC60C, 0xD68D, 0xE70E, 0xF78F,
};

// Undo manchester
static const char ManTable[] =
{
   0x0,   //0 b0000
   0x1,   //1 b0001
   0x1,   //2 b0010
   0x0,   //3 b0011
   0x2,   //4 b0100
   0x3,   //5 b0101
   0x3,   //6 b0110
   0x2,   //7 b0111
   0x2,   //8 b1000
   0x3,   //9 b1001
   0x3,   //A b1010
   0x2,   //B b1011
   0x0,   //C b1100
   0x1,   //D b1101
   0x1,   //E b1110
   0x0    //F b1111
};

// System Defaults
const uint8_t GenILength[] =
{
   // Nibble lengths of various MTU messages in nibbles ( header (10 nibbles) + payload + CRC16 (4 nibbles) )
   19, // MTU_ONE_PORT
    0, // MTU_ONE_PORT_WARN
   18, // MTU_ALARM
   34, // MTU_PASS_ALONG
   35, // MTU_WAKE_UP
   22, // MTU_ECODER_ONE_PORT
   30, // MTU_ECODER_TWO_PORT
   20, // MTU_ONE_PORT_SP
   24, // MTU_TWO_PORT
   0,  // MTU_TWO_PORT_WARN_A
   0,  // MTU2_MSG
   26, // MTU_TWO_PORT_SP
   39, // MTU_6327EB
   0,  // MTU_FREE2
   0,  // MTU_FREE3
   0   // MTU_SKIDWAY
};

/*****************************************************************************
 *  Local Function Declarations
 *****************************************************************************/

static void vRadio_PowerUp(void);

static uint8_t Radio_int_disable(uint8_t radioNum);
static uint8_t Radio_RX_int_enable(uint8_t radioNum);
static uint8_t Radio_TX_int_enable(uint8_t radioNum);

static uint8_t Radio_Enable_RSSI_Jump(uint8_t radioNum);
static uint8_t Radio_Disable_RSSI_Jump(uint8_t radioNum);

static void setPA(RADIO_PA_MODE_t mode);
static uint8_t Radio_Calibrate(uint8_t radioNum);

#if ( (HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A) || \
      (HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )  )
static bool ValidateGenII(uint8_t radioNum);
static bool ValidateGenI(uint8_t radioNum);
#endif
static bool Radio_RX_FIFO_Almost_Full(uint8_t radioNum);
static void processRadioInt(uint8_t radioNum);

static bool Init(RadioEvent_Fxn pCallbackFxn );
static RX_FRAME_t *ReadData(uint8_t radioNum);
static void wait_us(uint32_t time);
static uint8_t RADIO_Do_CCA(uint8_t radioNum, uint16_t chan);

static void Standby(void);
static bool StartRx(uint8_t radioNum, uint16_t chan);
static void StandbyTx(void);

static bool RADIO_Is_RX(uint8_t radioNum);
static void updateTCXO ( uint8_t radioNum );

/*lint -esym(401,SendData) not the same routine as in the wolfssl library  */
static PHY_DATA_STATUS_e SendData( uint8_t radioNum, uint16_t chan, PHY_MODE_e mode, PHY_DETECTION_e detection, const void *payload, uint32_t TxTime, PHY_TX_CONSTRAIN_e priority, float32_t
                                    power, uint16_t payload_len );

static OS_MUTEX_Obj radioMutex;

#if COMPARE == 1
void compbyte(uint8_t group, uint8_t number);
void compare(void);
#endif

#if 0  // TODO: RA6E1 Bob: This was removed from original project
void RF_SetHardwareMode(RFHardwareMode_e newMode)
{
   switch (newMode)
   {
      case SLEEP:
      {
         break;
      }

      case RECEIVER:
      {
         break;
      }

      case TRANSMITTER:
      {
         break;
      }

      case LISTEN_RSSI:
      {
         break;
      }
      default:
      {
         break;
      }
   }
}
#endif
/******************************************************************************

   Function Name: RADIO_TX_Watchdog

   Purpose: This function makes sure TX wasn't on for too long

   Arguments: none

   Returns: none

   Notes:

******************************************************************************/
void RADIO_TX_Watchdog(void)
{
#if 0 //TODO Melvin: include the code after dependent modules integrated
   OS_TICK_Struct time;
   uint32_t       TimeDiff;
   bool           Overflow;

   // Only valid if TX is active
#if ( PORTABLE_DCU == 1)
   if ((radio[RADIO_0].isDeviceTX) && ((currentTxMode == eRADIO_MODE_NORMAL) || (currentMode == eRADIO_MODE_NORMAL_DEV_600))) {
#else
   if ((radio[RADIO_0].isDeviceTX) && (currentTxMode == eRADIO_MODE_NORMAL)) {
#endif
      // Get diff between current time and time that TX started
#if ( MCU_SELECTED == NXP_K24 )
      _time_get_elapsed_ticks(&time);
      TimeDiff = (uint32_t)_time_diff_milliseconds ( &time, &radio[RADIO_0].TxWatchDog, &Overflow );
#elif ( MCU_SELECTED == RA6E1 )
      OS_TICK_Get_CurrentElapsedTicks(&time);
      TimeDiff = (uint32_t)OS_TICK_Get_Diff_InMilliseconds ( &radio[RADIO_0].TxWatchDog, &time );
#endif

      // TODO: RA6E1 - Overflow usage verify and modify the below statement
      // Check if TX has been ON for more than one seconds
      if ((TimeDiff > ONE_SEC) || Overflow) {
            PHY_CounterInc(ePHY_FailedTransmitCount, 0);
            ERR_printf("ERROR - PA was ON for too long");
#if ( EP == 1 )
         // Restart radio if it was also used as RX
         if (radio[(uint8_t)RADIO_0].rx_channel != PHY_INVALID_CHANNEL) { // This should always pass on an EP since we need both a TX and RX channel to properly fucntion.
#endif
            // A valid channel means the radio was configured as a receiver
            setPA(eRADIO_PA_MODE_RX); // This call will also set isDeviceTX to false
#if ( EP == 1 )
            vRadio_StartRX((uint8_t)RADIO_0, radio[(uint8_t)RADIO_0].rx_channel);
         }
#endif
      }
   }
#endif
}

/******************************************************************************

   Function Name: RADIO_RX_WatchdogService

   Purpose: This function services the soft-demodulator interrupt watchdog

   Arguments: none

   Returns: none

   Notes:

******************************************************************************/
void RADIO_RX_WatchdogService(uint8_t radioNum)
{
   OS_INT_disable( ); // Disable all interrupts. Variable shared between 2 tasks
#if ( MCU_SELECTED == NXP_K24 )
   radio[radioNum].lastFIFOFullTimeStamp = DWT_CYCCNT;
#elif ( MCU_SELECTED == RA6E1 )
   radio[radioNum].lastFIFOFullTimeStamp=DWT->CYCCNT;
#endif
   OS_INT_enable( ); // Enable interrupts.
}

/******************************************************************************

   Function Name: RADIO_RX_Watchdog

   Purpose: This function makes sure that soft-demodulators are stil generating interrupts

   Arguments: none

   Returns: none

   Notes:

******************************************************************************/
void RADIO_RX_Watchdog(void)
{
#if ( EP == 1 )
   uint8_t  temp[129]; //64 in each RX/TX buffer, plus the unused TX shift barrel
   uint32_t timeStamp;

   // Todo MKD adapt for many radios
   // Reset soft-demodulator if no interrupts for 2 seconds and radio is in RX
   if ( (radio[(uint8_t)RADIO_0].demodulator != 0 ) && (radio[(uint8_t)RADIO_0].rx_channel != PHY_INVALID_CHANNEL) ) {
      OS_INT_disable( ); // Disable all interrupts. Variable shared between 2 tasks
      timeStamp = radio[(uint8_t)RADIO_0].lastFIFOFullTimeStamp;
      OS_INT_enable( ); // Enable interrupts.
    #if ( MCU_SELECTED == NXP_K24 )
      if ( ((DWT_CYCCNT - timeStamp)/getCoreClock()) >= 2) {
    #elif ( MCU_SELECTED == RA6E1 ) // TODO: RA6E1 Modify variable to get core clock from a function
      if ( ((DWT->CYCCNT - timeStamp)/SystemCoreClock) >= 2) {
    #endif
         INFO_printf("Soft-Demod FIFO purged");
         // Read through the buffer
         si446x_read_rx_fifo((uint8_t)RADIO_0, 128, temp);
         (void)si446x_get_int_status_fast_clear((uint8_t)RADIO_0);
         RADIO_RX_WatchdogService((uint8_t)RADIO_0); // Reset time stamp to avoid being called again.
      }
   }
#endif
}

/******************************************************************************

   Function Name: RADIO_Update_Freq_Watchdog

   Purpose: This function makes sure TCXO trimming is still going on

   Arguments: none

   Returns: none

   Notes:   The watchdog is necessary because of a few conditions that would stop DMA_Complete_IRQ_ISR() from being called.
            1) We could have been in the middle of computing frequency from radio 1 to 8 and the PHY is set to DEAF.
            2) The RX channels configuration changes such that there are no RX radios anymore.
            In both case, we reset the triming which in turn will use radio 0 which should always be available.

******************************************************************************/
void RADIO_Update_Freq_Watchdog(void)
{
#if 0  //TODO Melvin: include the code after dependent modules integrated
   uint32_t timeStamp;

   // Make sure radio 0 is initialized because we won't get TCXO output until it is done
   if ( radio[(uint8_t)RADIO_0].configured ) {
      OS_INT_disable( ); // Disable all interrupts. Variable shared between 2 tasks
      timeStamp = DMA_Complete_IRQ_ISR_Timestamp;
      OS_INT_enable( ); // Enable interrupts.

#if ( MCU_SELECTED == NXP_K24 )

      if ( ((DWT_CYCCNT - timeStamp)/getCoreClock()) >= 2) {
#elif ( MCU_SELECTED == RA6E1 ) // TODO: RA6E1 Modify variable to get core clock from a function
      if ( ((DWT->CYCCNT - timeStamp)/SystemCoreClock) >= 2) {

#endif

         RADIO_Update_Freq();
      }
   }
#endif
}

/*!
 *  Call the event handler
 *
 *  @param event_type  -
 *  @param radioNum    - index of the radio (0, or 0-8 for frodo)
 *
 */
#if ( RTOS_SELECTION == MQX_RTOS )
void     RADIO_Event_Set(RADIO_EVENT_t event_type, uint8_t radioNum)
#elif ( RTOS_SELECTION == FREE_RTOS )
void     RADIO_Event_Set(RADIO_EVENT_t event_type, uint8_t radioNum, bool fromISR)
#endif
{
   if(pEventHandler_Fxn != NULL)
   {  // Handler is defined
#if ( RTOS_SELECTION == MQX_RTOS )
      pEventHandler_Fxn(event_type, radioNum);
#elif ( RTOS_SELECTION == FREE_RTOS )
      pEventHandler_Fxn(event_type, radioNum, fromISR);
#endif
   }
   else{
//      INFO_printf("No event handler");
   }
}

/*!
 *  Power up the Radio.
 *
 *  @note
 *
 */
static void vRadio_PowerUp(void)
{
   /* Hardware reset the chips */
   si446x_reset();

   OS_TASK_Sleep(TEN_MSEC); // Give some time to radio to get all out of reset

   // Monitor GPIO0 of radio 0 for power on reset
#if ( RTOS_SELECTION == MQX_RTOS )  //TODO Melvin: need to replace LWGPIO_VALUE_LOW with BSP_IO_LEVEL_LOW
   while( RDO_0_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
#elif (RTOS_SELECTION == FREE_RTOS)
   while( RDO_0_GPIO0() == (uint32_t)BSP_IO_LEVEL_LOW  ) {
#endif
      INFO_printf("Waiting on radio 0 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
#if ( DCU == 1 )
   // Monitor GPIO0 of radio 1 for power on reset
   while( RDO_1_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 1 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
   // Monitor GPIO0 of radio 2 for power on reset
   while( RDO_2_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 2 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
   // Monitor GPIO0 of radio 3 for power on reset
   while( RDO_3_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 3 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
   // Monitor GPIO0 of radio 4 for power on reset
   while( RDO_4_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 4 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
   // Monitor GPIO0 of radio 5 for power on reset
   while( RDO_5_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 5 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
   // Monitor GPIO0 of radio 6 for power on reset
   while( RDO_6_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 6 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
   // Monitor GPIO0 of radio 7 for power on reset
   while( RDO_7_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 7 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
   // Monitor GPIO0 of radio 8 for power on reset
   while( RDO_8_GPIO0() == (uint32_t)LWGPIO_VALUE_LOW ) {
      INFO_printf("Waiting on radio 8 to get out of reset");
      OS_TASK_Sleep(TEN_MSEC);
   }
#endif
}

/******************************************************************************
*
* Function Name     : Radio0_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
#if ( RTOS_SELECTION == MQX_RTOS )
static void Radio0_IRQ_ISR(void)
#elif (RTOS_SELECTION == FREE_RTOS )
void Radio0_IRQ_ISR(external_irq_callback_args_t * p_args)
#endif // RTOS_SELECTION
{
   uint32_t primask = __get_PRIMASK();
//#if ( RTOS_SELECTION == MQX_RTOS ) //TODO Melvin: include the code once interrupt enable/disable added
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
//#endif

   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   radio[(uint8_t)RADIO_0].tentativeSyncTime.QSecFrac = TIME_UTIL_GetTimeInQSecFracFormat();

#if ( EP == 1 )
#if 0 //TODO Melvin: include the code after replacement for Flexible Timer Module (FTM) is integrated
   uint32_t cycleCounter;
   uint32_t cpuFreq;
   uint16_t currentFTM;
   uint16_t capturedFTM;
   uint16_t delayFTM;
   uint32_t delayCore;

   // Need to read those 3 counters together so disable interrupts if they are not disabled already
   cycleCounter   = DWT_CYCCNT;
   currentFTM     = (uint16_t)FTM1_CNT; // Connected to radio interrupt
   capturedFTM    = (uint16_t)FTM1_C0V; // Captured counter when radio interrupt happened
   __set_PRIMASK(primask); // Restore interrupts

   FTM_CnSC_REG(FTM1_BASE_PTR, 0) &= ~FTM_CnSC_CHF_MASK; // Acknowledge channel interrupt

   /* Compute when the interrupt really happened relative to the cycle counter based on:
         1) The FTM capture time
         2) The current FTM time
         3) The current cycle counter
         4) The CPU frequency
      We go through this to make future computation easier (extended the FTM event to 32 bits) and based on the same common time reference (i.e. compare against FTM3 that doesn't run at FTM1
      speed)
      This interrupt needs to be processed faster than 1.1ms after it happened (i.e. avoid rollover 65535/60MHz) which is a resonable assumption.

      1) Compute delay between now and the FTM capture.  */
   delayFTM = currentFTM - capturedFTM;

   // 2) Convert FTM delay to core delay (i.e. delay in cycle count)
   // FTM runs at 60MHz and cycle counter runs at the core speed (120MHz)
   delayCore = (uint32_t)delayFTM*2; // Core is 120MHz and Bus is 60MHZ so multiply by 2

   // 3) Compute what the cycle counter must have been when the FIFO interrupt happened.
   radio[(uint8_t)RADIO_0].intFIFOCYCCNT = cycleCounter - delayCore;

   // Update the fractional Sync time to remove the delay between when the interrupt happened and when it was processed.
   (void)TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL );
   radio[(uint8_t)RADIO_0].tentativeSyncTime.QSecFrac -= ((uint64_t)delayCore << 32) / (uint64_t)cpuFreq;
#else
   __set_PRIMASK(primask); // Restore interrupts
#endif // 0
   if ((radio[(uint8_t)RADIO_0].demodulator == 0) || RADIO_Is_TX()) {
      RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_0, (bool)true); // Call PHY task for interrupt processing
   } else {
#if ( RTOS_SELECTION == MQX_RTOS )
      OS_EVNT_Set( &SD_Events, SOFT_DEMOD_RAW_PHASE_SAMPLES_AVAILABLE_EVENT); // Call soft-modem task for samples processing
#elif (RTOS_SELECTION == FREE_RTOS )
      OS_EVNT_Set_from_ISR( &SD_Events, SOFT_DEMOD_RAW_PHASE_SAMPLES_AVAILABLE_EVENT); // Call soft-modem task for samples processing
#endif // RTOS_SELECTION
   }

#if ( RTOS_SELECTION == MQX_RTOS )
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_0);
#elif (RTOS_SELECTION == FREE_RTOS )
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_0, (bool)true); /* Signal the event from the ISR */
#endif // RTOS_SELECTION
#endif // ( EP == 1 )
}

#if ( DCU == 1 )
/******************************************************************************
*
* Function Name     : Radio1_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio1_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_1].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_1); // RADIO_1 takes care of all radios on RX (Frodo 101-9975T)
}

#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
/******************************************************************************
*
* Function Name     : Radio0_TxDone_ISR
* Comments          : Radio0 GPIO1 (TxState) interrupt signifies transmit complete on falling edge.
*
******************************************************************************/
static void Radio0_TxDone_ISR(void)
{
   (void)BSP_PaDAC_SetValue( 0 );   /* Kill eFEM power   */
   RADIO_0_TXDONE_ISR_CLEAR();      /* Reset IRQ   */
}

/******************************************************************************
*
* Function Name     : Radio2_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio2_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_2].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_2);
}

/******************************************************************************
*
* Function Name     : Radio3_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio3_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_3].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_3);
}

/******************************************************************************
*
* Function Name     : Radio4_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio4_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_4].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_4);
}

/******************************************************************************
*
* Function Name     : Radio5_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio5_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_5].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_5);
}

/******************************************************************************
*
* Function Name     : Radio6_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio6_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_6].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_6);
}

/******************************************************************************
*
* Function Name     : Radio7_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio7_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_7].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_7); // RADIO_1 takes care of all radios on RX (Frodo)
}

/******************************************************************************
*
* Function Name     : Radio8_IRQ_ISR
* Comments          : The IRQ interrupt service routine triggered by gpio
*
******************************************************************************/
static void Radio8_IRQ_ISR(void)
{
   // There is no guarantee that this interrupt was from a time sync (it could be from preamble or FIFO almost full) but only the time sync interrupt will validate this value
   (void)TIME_UTIL_GetTimeInSecondsFormat(&radio[(uint8_t)RADIO_8].tentativeSyncTime);
   RADIO_Event_Set(eRADIO_INT, (uint8_t)RADIO_8);
}
#endif // ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#endif // ( DCU == 1 )

/******************************************************************************

   Function Name: Radio_int_disable

   Purpose: This function disables the radio interrupts

   Arguments: radioNum - radio to use

   Returns: 0 for success

   Notes:

******************************************************************************/
static uint8_t Radio_int_disable(uint8_t radioNum)
{
   // Disable global interrupts
   return ( si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_INT_CTL,                  // Property group.
                                 4,                                           // Number of property to be set.
                                 SI446X_PROP_GRP_INDEX_INT_CTL_ENABLE,        // Start sub-property address.
                                 0, 0, 0, 0) );                               // Disable all interrupts
}

/******************************************************************************

   Function Name: Radio_RX_int_enable

   Purpose: This function enables the radio RX interrupts

   Arguments: radioNum - radio to use

   Returns: 0 for success

   Notes:

******************************************************************************/
static uint8_t Radio_RX_int_enable(uint8_t radioNum)
{
   if ( radio[radioNum].demodulator == 0 ) {
      return ( si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_INT_CTL,                  // Property group.
                                    4,                                           // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_INT_CTL_ENABLE,        // Start sub-property address.
                                    SI446X_PROP_INT_CTL_ENABLE_MODEM_INT_STATUS_EN_BIT |      // Enable modem interrupt
                                    SI446X_PROP_INT_CTL_ENABLE_PH_INT_STATUS_EN_BIT,          // Enable PHY interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_PACKET_SENT_EN_BIT |        // Enable packet sent interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_PACKET_RX_EN_BIT |          // Enable packet RX interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_RX_FIFO_ALMOST_FULL_EN_BIT, // Enable RX FIFO almost full
                                    SI446X_PROP_INT_CTL_MODEM_ENABLE_RSSI_JUMP_EN_BIT |       // Enable RSSI jump
                                    SI446X_PROP_INT_CTL_MODEM_ENABLE_PREAMBLE_DETECT_EN_BIT | // Enable preamble detect interrupt
                                    SI446X_PROP_INT_CTL_MODEM_ENABLE_SYNC_DETECT_EN_BIT,      // Enable sync detect interrupt
                                    0) );                                                     // Disable all chip interrupts
   } else {
      return ( si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_INT_CTL,                  // Property group.
                                    4,                                           // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_INT_CTL_ENABLE,        // Start sub-property address.
                                    SI446X_PROP_INT_CTL_ENABLE_PH_INT_STATUS_EN_BIT,          // Enable PHY interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_RX_FIFO_ALMOST_FULL_EN_BIT, // Enable RX FIFO almost full
                                    0,                                                        // Disable all modem interrupts
                                    0) );                                                     // Disable all chip interrupts
   }
}

/******************************************************************************

   Function Name: Radio_TX_int_enable

   Purpose: This function enables the radio TX interrupts

   Arguments: radioNum - radio to use

   Returns: 0 for success

   Notes:

******************************************************************************/
static uint8_t Radio_TX_int_enable(uint8_t radioNum)
{
   if ( radio[radioNum].demodulator == 0 ) {
      return ( si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_INT_CTL,                  // Property group.
                                    4,                                           // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_INT_CTL_ENABLE,        // Start sub-property address.
                                    SI446X_PROP_INT_CTL_ENABLE_MODEM_INT_STATUS_EN_BIT |      // Enable modem interrupt
                                    SI446X_PROP_INT_CTL_ENABLE_PH_INT_STATUS_EN_BIT,          // Enable PHY interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_PACKET_SENT_EN_BIT |        // Enable packet sent interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_TX_FIFO_ALMOST_EMPTY_EN_BIT,// Enable TX FIFO almost empty
                                    SI446X_PROP_INT_CTL_MODEM_ENABLE_RSSI_JUMP_EN_BIT |       // Enable RSSI jump
                                    SI446X_PROP_INT_CTL_MODEM_ENABLE_PREAMBLE_DETECT_EN_BIT | // Enable preamble detect interrupt
                                    SI446X_PROP_INT_CTL_MODEM_ENABLE_SYNC_DETECT_EN_BIT,      // Enable sync detect interrupt
                                    0) );                                                     // Disable all chip interrupts
   } else {
      return ( si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_INT_CTL,                  // Property group.
                                    4,                                           // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_INT_CTL_ENABLE,        // Start sub-property address.
                                    SI446X_PROP_INT_CTL_ENABLE_PH_INT_STATUS_EN_BIT,          // Enable PHY interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_PACKET_SENT_EN_BIT |        // Enable packet sent interrupt
                                    SI446X_PROP_INT_CTL_PH_ENABLE_TX_FIFO_ALMOST_EMPTY_EN_BIT,// Enable TX FIFO almost empty
                                    0,                                                        // Disable all modem interrupts
                                    0) );                                                     // Disable all chip interrupts
   }
}

/******************************************************************************

   Function Name: Radio_latch_RSSI

   Purpose: This function programms the RSSI latch

   Arguments: radioNum - Radio to use
            sync     - TRUE - latch RSSI at sync. FALSE - latch RSSI ar RX

   Returns: 0 for success

   Notes:

******************************************************************************/
static uint8_t Radio_latch_RSSI(uint8_t radioNum, bool sync)
{
   if (sync) {
      return ( si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_MODEM,                 // Property group.
                                    1,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_MODEM_RSSI_CONTROL, // Start sub-property address.
                                    SI446X_PROP_MODEM_RSSI_CONTROL_AVERAGE_ENUM_AVERAGE4 | // Average over 4 bits
                                    SI446X_PROP_MODEM_RSSI_CONTROL_LATCH_ENUM_SYNC) );     // Latch at sync
   } else {
      return ( si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_MODEM,                 // Property group.
                                    1,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_MODEM_RSSI_CONTROL, // Start sub-property address.
                                    SI446X_PROP_MODEM_RSSI_CONTROL_AVERAGE_ENUM_AVERAGE4 | // Average over 4 bits
                                    SI446X_PROP_MODEM_RSSI_CONTROL_LATCH_ENUM_RX_STATE5) );// Latch at RX
   }
}

/******************************************************************************

   Function Name: Radio_Enable_RSSI_Jump

   Purpose: This function enables RSSI jump detection

   Arguments:

   Returns: 0 for success

   Notes:   RSSI jump down generates a spurious interrupt.
            Silabs recommended to not use RSSI jump down with our application.
            They believe it's not needed anyway.
            Silabs recommended JMPDLYLEN set to 2*Tb to reduce the risk of triggering on noise.
            Silabs confirmed that ENJMPRX is broken.

******************************************************************************/
static uint8_t Radio_Enable_RSSI_Jump(uint8_t radioNum)
{
   return ( si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,                  // Property group.
                                 1,                                         // Number of property to be set.
                                 SI446X_PROP_GRP_INDEX_MODEM_RSSI_CONTROL2, // Start sub-property address.
                                 SI446X_PROP_MODEM_RSSI_CONTROL2_RSSIJMP_UP_BIT |  // Jump up enabled
                                 SI446X_PROP_MODEM_RSSI_CONTROL2_ENRSSIJMP_BIT) ); // RSSI jump detection enabled
}

/******************************************************************************

   Function Name: Radio_Disable_RSSI_Jump

   Purpose: This function disables RSSI jump detection

   Arguments:

   Returns: 0 for success

   Notes:

******************************************************************************/
static uint8_t Radio_Disable_RSSI_Jump(uint8_t radioNum)
{
   return ( si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,                  // Property group.
                                 1,                                         // Number of property to be set.
                                 SI446X_PROP_GRP_INDEX_MODEM_RSSI_CONTROL2, // Start sub-property address.
                                 0) );                                      // Disable RSSI jump
}

/******************************************************************************

   Function Name: setPA

   Purpose: This function sets the PA (and LNA) to RX or TX mode

   Arguments: mode - TX ot RX

   Returns:

   Notes:

******************************************************************************/
static void setPA(RADIO_PA_MODE_t mode)
{
#if ( EP == 1 )
   // For EP
   // Set to RX
   if (mode == eRADIO_PA_MODE_RX) {
#if ( MCU_SELECTED == NXP_K24 ) // SPI Slewrate cannot be adjusted in RA6E1 instead drive capcity settings can be done
      SPI_SetFastSlewRate(RADIO_0_SPI_PORT_NUM, (bool)false); // This reduces the noise floor while receiving.
#endif
      RDO_RX0TX1_RX();
      RDO_PA_EN_ON();
      radio[RADIO_0].isDeviceTX = false;
   } else if (mode == eRADIO_PA_MODE_TX) {
#if ( MCU_SELECTED == NXP_K24 )// SPI Slewrate cannot be adjusted in RA6E1 instead drive capcity settings can be done
      SPI_SetFastSlewRate(RADIO_0_SPI_PORT_NUM, (bool)true); // This increase the SPI strength and prevent errors.
#endif
      RDO_RX0TX1_TX();
      RDO_PA_EN_ON();
#if ( MCU_SELECTED == NXP_K24 )
      _time_get_elapsed_ticks(&radio[RADIO_0].TxWatchDog);
#elif ( MCU_SELECTED == RA6E1 )
      OS_TICK_Get_CurrentElapsedTicks(&radio[RADIO_0].TxWatchDog);
#endif
      radio[RADIO_0].isDeviceTX = true;
   }
#else
   union si446x_cmd_reply_union Si446xCmd;

   // The radio needs to be configured to have access to the GPIO pins
   if (radio[RADIO_0].configured) {
      // For DCU
      RDO_PA_EN_OFF(); // Disable PA for safety while switching

      // Set to RX
      if (mode == eRADIO_PA_MODE_RX) {
         // Switch LNA to 10b state
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
         (void)si446x_gpio_pin_cfg((uint8_t)RADIO_0,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,     // GPIO0 outputs TCXO for TCXO trimming
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_EN_PA,       // GPIO1 - falling edge generates TX done IRQ
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE1,      // 1
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TX_STATE,    // GPIO3 1 while transmitting then 0
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5, // Hope to reduce unintended noise with this setting
                                   &Si446xCmd);
#else
         (void)si446x_gpio_pin_cfg((uint8_t)RADIO_0,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,  // GPIO0 outputs TCXO for TCXO trimming
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE1,   // 1
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE0,   // 0
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5, // Hope to reduce unintended noise with this setting
                                   &Si446xCmd);
#endif
         radio[RADIO_0].isDeviceTX = false;
         RADIO_Set_RxStart((uint8_t)RADIO_FIRST_RX, DWT_CYCCNT);
      } else if (mode == eRADIO_PA_MODE_TX) {
         // Switch LNA to 01b state
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
         /* GPIO_3 controlled by TX state of radio */
         (void)si446x_gpio_pin_cfg((uint8_t)RADIO_0,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,     // GPIO0 outputs TCXO for TCXO trimming
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_EN_PA,       // GPIO1 - falling edge generates TX done IRQ
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE0,      // 0
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TX_STATE,    // GPIO 3 1 while transmitting then 0
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5, // Hope to reduce unintended noise with this setting
                                   &Si446xCmd);
#else
         (void)si446x_gpio_pin_cfg((uint8_t)RADIO_0,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,  // GPIO0 outputs TCXO for TCXO trimming
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE0,   // 0
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE1,   // 1
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                   SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5, // Hope to reduce unintended noise with this setting
                                   &Si446xCmd);
#endif
         RDO_PA_EN_ON(); // Enable PA
#if ( MCU_SELECTED == NXP_K24 )
         _time_get_elapsed_ticks(&radio[RADIO_0].TxWatchDog);
#elif ( MCU_SELECTED == RA6E1 )
         OS_TICK_Get_CurrentElapsedTicks(&radio[RADIO_0].TxWatchDog);
#endif
         radio[RADIO_0].isDeviceTX = true;
      }
   }
#endif
}

/******************************************************************************

   Function Name: Radio_Calibrate

   Purpose: Calibrate the radio for image rejection

   Arguments: radioNum - Radio number

   Returns: 0 for success

   Notes:   One single frequency is needed to calibrate.
            It was observed experimentaly that calibrating at 450MHz is valid for the opperational range 450MHz to 470MHz.
            Interrupts are not restored since this call is made from start radio which will enable interrupts.

******************************************************************************/
static uint8_t Radio_Calibrate(uint8_t radioNum)
{
   uint8_t retVal;

   retVal = si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);
   if ( retVal ) return retVal;

   // Disable interrupts in case the radio latches on a preamble. We don't want to process that.
   retVal = Radio_int_disable(radioNum);
   if ( retVal ) return retVal;

   // Set calibration frequency
   // It seems like it doesn't matter what the real TCXO frequency is for this calibration.
   // WDS provides the same numbers for 30MHz and 29.4912MHz
   (void)si446x_set_property( radioNum,
                              SI446X_PROP_GRP_ID_FREQ_CONTROL,
                              8,
                              SI446X_PROP_GRP_INDEX_FREQ_CONTROL_INTE,
                              0x3B,
                              0x09,
                              0x00,
                              0x00,
                              0x03,
                              0x79,
                              0x20,
                              0xFE);

   /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
   retVal = si446x_start_rx(radioNum, 0, 0u, 0,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN );
   if ( retVal ) return retVal;

   // Do actual calibration
   retVal = si446x_ircal(radioNum, 0x56, 0x10, 0xCA, 0xF0);
   if ( retVal ) return retVal;
   retVal = si446x_ircal(radioNum, 0x13, 0x10, 0xCA, 0xF0);
   if ( retVal ) return retVal;

   // Force ready state
   return (si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY)); // Leave RX/TX state
}

/******************************************************************************

   Function Name: RADIO_RxDetectionConfig

   Purpose: Configure preamble and sync

   Arguments: radioNum  - Radio number
              detection - Detection to use
              forceSet  - Force an update to the radio

   Returns: TRUE if changes were made

   Notes:

******************************************************************************/
bool RADIO_RxDetectionConfig(uint8_t radioNum, PHY_DETECTION_e detection, bool forceSet)
{
#ifndef TEST_900MHZ
#error "TEST_900MHz not defined"
#endif

#if ( TEST_900MHZ == 1 )
   return true;
#else
   // Check if we need to reconfigure radio
   if ((radio[radioNum].detection != detection) || forceSet){
      // Configure preamble and sync word detection
      if (detection == ePHY_DETECTION_0) {
         // Configuration typicaly used for SRFN
         // Preamble 0xAAAAAAAAAAAAAAAAAAAAAAAA
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_PREAMBLE,              // Property group.
                                    5,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_PREAMBLE_TX_LENGTH, // Start sub-property address.
                                    0x0C,                                     // Preamble length in bytes
                                    0x20,                                     // Preamble detection count in bits
                                    0x00,                                     // unused (non-standard preamble)
                                    0x0F,                                     // RX preamble timeout
                                    0x31);                                    // Preamble 0xAAAAAA...

         // Sync word 0x89445BC1
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_SYNC,           // Property group.
                                    5,                                 // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                                    0x03,                              // Sync length (4 bytes), Sync sent as 2GFSK
                                    0x91,                              // Sync word
                                    0x22,                              // Sync word
                                    0xDA,                              // Sync word
                                    0x83);                             // Sync word
      } else if (detection == ePHY_DETECTION_1) {
         // Configuration typicaly used for STAR Gen II 7200 bps
         // Preamble 0x5555555555555555555555
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_PREAMBLE,              // Property group.
                                    5,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_PREAMBLE_TX_LENGTH, // Start sub-property address.
                                    0x0B,                                     // Preamble length in bytes
                                    0x14,                                     // Preamble detection count in bits
                                    0x00,                                     // unused (non-standard preamble)
                                    0x0F,                                     // RX preamble timeout
                                    0x12);                                    // Preamble 0x555555...

         // Sync word 0x55D2B4
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_SYNC,           // Property group.
                                    5,                                 // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                                    0x12,                              // One bit error, Sync length (3 bytes), Sync sent as 2GFSK
                                    0xAA,                              // Sync word
                                    0x4B,                              // Sync word
                                    0x2D,                              // Sync word
                                    0x00);                             // Sync word
      } else if (detection == ePHY_DETECTION_2) {
         // Configuration typicaly used for STAR Gen I and Gen II 2400 baud
         // Preamble 0x55555555555555
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_PREAMBLE,              // Property group.
                                    5,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_PREAMBLE_TX_LENGTH, // Start sub-property address.
                                    0x07,                                     // Preamble length in bytes
                                    0x08,                                     // Preamble detection count in bits
                                    0x00,                                     // unused (non-standard preamble)
                                    0x0F,                                     // RX preamble timeout
                                    0x12);                                    // Preamble 0x555555...

         // Sync word 0xCCCCCCD2
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_SYNC,           // Property group.
                                    5,                                 // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                                    0x03,                              // Sync length (4 bytes), Sync sent as 2GFSK
                                    0x33,                              // Sync word
                                    0x33,                              // Sync word
                                    0x33,                              // Sync word
                                    0x4B);                             // Sync word
      } else if (detection == ePHY_DETECTION_3) {
         // Configuration typicaly used for STAR Gen II 7200 bps fast message
         // Preamble 0x5555555555555555555555
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_PREAMBLE,              // Property group.
                                    5,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_PREAMBLE_TX_LENGTH, // Start sub-property address.
                                    0x0B,                                     // Preamble length in bytes
                                    0x14,                                     // Preamble detection count in bits
                                    0x00,                                     // unused (non-standard preamble)
                                    0x0F,                                     // RX preamble timeout
                                    0x12);                                    // Preamble 0x555555...

         // Sync word 0x55AB49
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_SYNC,           // Property group.
                                    5,                                 // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                                    0x12,                              // One bit error, Sync length (3 bytes), Sync sent as 2GFSK
                                    0xAA,                              // Sync word
                                    0xD5,                              // Sync word
                                    0x92,                              // Sync word
                                    0x00);                             // Sync word
      } else if (detection == ePHY_DETECTION_4) {
         // Configuration typicaly used for STAR Gen I and Gen II 2400 baud. Inverted pattern of ePHY_DETECTION_2 for some old MTUs
         // Preamble 0xAAAAAAAAAAAAAA
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_PREAMBLE,              // Property group.
                                    5,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_PREAMBLE_TX_LENGTH, // Start sub-property address.
                                    0x07,                                     // Preamble length in bytes
                                    0x08,                                     // Preamble detection count in bits
                                    0x00,                                     // unused (non-standard preamble)
                                    0x0F,                                     // RX preamble timeout
                                    0x31);                                    // Preamble 0xAAAAAA...

         // Sync word 0x3333332D
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_SYNC,           // Property group.
                                    5,                                 // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                                    0x03,                              // Sync length (4 bytes), Sync sent as 2GFSK
                                    0xCC,                              // Sync word
                                    0xCC,                              // Sync word
                                    0xCC,                              // Sync word
                                    0xB4);                             // Sync word
      } else if (detection == ePHY_DETECTION_5) {
         // Configuration typicaly used for SRFN. This is like ePHY_DETECTION_0 but the preamble and RX threshold is half as long.
         // Preamble 0xAAAAAAAAAAAA
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_PREAMBLE,              // Property group.
                                    5,                                        // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_PREAMBLE_TX_LENGTH, // Start sub-property address.
                                    0x06,                                     // Preamble length in bytes
                                    0x10,                                     // Preamble detection count in bits
                                    0x00,                                     // unused (non-standard preamble)
                                    0x0F,                                     // RX preamble timeout
                                    0x31);                                    // Preamble 0xAAAAAA...

         // Sync word 0x89445BC1
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_SYNC,           // Property group.
                                    5,                                 // Number of property to be set.
                                    SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                                    0x03,                              // Sync length (4 bytes), Sync sent as 2GFSK
                                    0x91,                              // Sync word
                                    0x22,                              // Sync word
                                    0xDA,                              // Sync word
                                    0x83);                             // Sync word
      }

      // Update with current configuration
      radio[radioNum].detection = detection;
#if ( TEST_SYNC_ERROR == 1 )
      PHY_GetReq_t GetReq;
      uint8_t      SyncError;

      GetReq.eAttribute = ePhyAttr_SyncError;
      (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&SyncError);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion
      RADIO_Set_SyncError( SyncError );
#endif
      return (bool)true;
   }
   return (bool)false;
#endif
}

/******************************************************************************

   Function Name: RADIO_RxFramingConfig

   Purpose: Configure radio framing

   Arguments: radioNum  - Radio number
            framing   - Framing to use
            forceSet  - Force an update to the radio

   Returns: TRUE if changes were made

   Notes:

******************************************************************************/
bool RADIO_RxFramingConfig(uint8_t radioNum, PHY_FRAMING_e framing, bool forceSet)
{
#if ( TEST_900MHZ == 1 )
   return true;
#else
   // Check if we need to reconfigure radio
   if ((radio[radioNum].framing != framing) || forceSet){
      // Update with current configuration
      radio[radioNum].framing = framing;

      return (bool)true;
   }
   return (bool)false;
#endif
}

/******************************************************************************

   Function Name: RADIO_RxModeConfig

   Purpose: Configure radio PHY mode

   Arguments: radioNum  - Radio number
              mode      - PHY mode to use
              forceSet  - Force an update to the radio

   Returns: TRUE if changes were made

   Notes:

******************************************************************************/
bool RADIO_RxModeConfig(uint8_t radioNum, PHY_MODE_e mode, bool forceSet)
{
   const tModeConfiguration *pMode;

#if ( TEST_900MHZ == 1 )
   return true;
#else
   // Valide input
   if ( (mode == ePHY_MODE_0) || (mode >= ePHY_MODE_LAST) ) {
      INFO_printf("ERROR - Invalid Configuration mode %u", ( uint32_t )mode);
   } else {
      // Check if we need to reconfigure radio
      if ((radio[radioNum].mode != mode) || forceSet){
         pMode = &modeConfiguration[0]; // Point to first property
         // Program each radio property one by one
         while (pMode->Group) {
            (void)si446x_set_property( radioNum,
                                       pMode->Group,  // Property group.
                                       1,             // Number of property to be set.
                                       pMode->Number, // Start sub-property address.
                                       pMode->mode[mode]);
            pMode++; // Go to next property
         }
         // Update with current configuration
         radio[radioNum].mode = mode;

         return (bool)true;
      }
   }
   return (bool)false;
#endif
}

/******************************************************************************

   Function Name: Rx Channel get

   Purpose: This function returns the channel configured for a radio

   Arguments: radioNum  - Radio number

   Returns: Rx channel

   Notes:

******************************************************************************/
uint16_t RADIO_RxChannelGet(uint8_t radioNum)
{
   return (radio[radioNum].rx_channel);
}

/******************************************************************************

   Function Name: Shutdown radios

   Purpose: This function shutdowns all radios and hardware to save energy

   Arguments: none

   Returns:

   Notes: This function is called even before the radios are configure so some processing is conditionnal

******************************************************************************/
static void Shutdown(void)
{
   // Put the PA in TX to save power and put radios in standby to gracefully stop them and increment appropriate counters.
   Standby();
#if 0 //TODO: RA6E1
   // Shutdown power amplifier
   RDO_PA_EN_TRIS();

   // Turn off radio oscillator
   RDO_OSC_EN_TRIS(); // Samwise only
#endif
   // Shutdown radios (pull SDN pin high)
   RDO_SDN_TRIS();      // First radio on Frodo and Samwise
   RX_RADIO_SDN_TRIS(); // All other radios on Frodo
}

#if COMPARE == 1
// Used to find diffs between templates
void compbyte(uint8_t group, uint8_t number) {
   uint8_t data[9] = {0}; // Compare up to 9 templates (i.e. we have 9 radios)
   uint32_t i;
   union si446x_cmd_reply_union Si446xCmd;

   // Pause every modulo 5 for debug port to catch up
   if ( (number%5 == 0) && number ) {
      OS_TASK_Sleep( HALF_SEC );
   }

   // Load all values
   for (i=0; i<sizeof(pRadioConfiguration)/sizeof(pRadioConfiguration[0]); i++) {
      si446x_get_property( (uint8_t)i,
                           group,   // Property group.
                           1,       // Number of property to be set.
                           number,  // Start sub-property address.
                           &Si446xCmd);

      data[i] = Si446xCmd.GET_PROPERTY.DATA[0];
   }

   // Compare each values and print if any differences
   for (i=1; i<sizeof(pRadioConfiguration)/sizeof(pRadioConfiguration[0]); i++) {
      if ( data[i] != data[i-1] ) {
         INFO_printf("   { 0x%02X, 0x%02X, { 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X } },",
                     group,
                     number,
                     data[0],
                     data[0],
                     data[1],
                     data[2],
                     data[3],
                     data[4],
                     data[5],
                     data[6],
                     data[7],
                     data[8]);
         break;
      }
   }
}

// Load radios 1,2 and 3 with different templates that need to be compared and run this code
void compare(void) {
   uint32_t i;

   INFO_printf("Global");
   for (i=0; i<= 0x0a; i++) {
      compbyte(0, i);
   }

   INFO_printf("Int_ctl");
   for (i=0; i<= 0x03; i++) {
      compbyte(1, i);
   }

   INFO_printf("FRR_ctl");
   for (i=0; i<= 0x03; i++) {
      compbyte(2, i);
   }

   INFO_printf("Preamble");
   for (i=0; i<= 0x0d; i++) {
      compbyte(0x10, i);
   }

   INFO_printf("Sync");
   for (i=0; i<= 0x09; i++) {
      compbyte(0x11, i);
   }

   INFO_printf("Pkt");
   for (i=0; i<= 0x39; i++) {
      compbyte(0x12, i);
   }

   INFO_printf("Modem");
   for (i=0; i<= 0x5f; i++) {
      compbyte(0x20, i);
   }

   INFO_printf("Modem CHFLT");
   for (i=0; i<= 0x23; i++) {
      compbyte(0x21, i);
   }

   INFO_printf("PA");
   for (i=0; i<= 0x06; i++) {
      compbyte(0x22, i);
   }

   INFO_printf("Synth");
   for (i=0; i<= 0x07; i++) {
      compbyte(0x23, i);
   }

   INFO_printf("Match");
   for (i=0; i<= 0x0b; i++) {
      compbyte(0x30, i);
   }

   INFO_printf("Freq control");
   for (i=0; i<= 0x07; i++) {
      compbyte(0x40, i);
   }
}
#endif

/******************************************************************************

   Function Name: RADIO_TCXO_Get

   Purpose: This function returns the current radio TCXO frequency

   Arguments: radioNum - Radio number
              TCXOfreq - The TCXO frequency
              source   - Source of the frequency computation
              seconds  - If not NULL, return the time of last update in seconds since 1970 and validate the current frequency

   Returns: TRUE if the frequency has been updated less than a minute ago.

   Notes:

******************************************************************************/
bool RADIO_TCXO_Get ( uint8_t radioNum, uint32_t *TCXOfreq, TIME_SYS_SOURCE_e *source, uint32_t *seconds )
{
   bool retVal = (bool)false;
   uint32_t currentTime;
   uint32_t lastUpdate;

   uint32_t primask = __get_PRIMASK();
#if ( RTOS_SELECTION == MQX_RTOS ) //TODO Melvin: add the below code once the disable interrupt is added
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
#endif
   *TCXOfreq  = radio[radioNum].TCXOFreq;
   lastUpdate = radio[radioNum].TCXOLastUpdate;
   if ( source != NULL ) {
      *source = radio[radioNum].TCXOsource;
   }
   __set_PRIMASK(primask); // Restore interrupts

   // TCXO frequency is considered valid for 30 days after the last update
   // After that it is considered stale.
   currentTime = TIME_UTIL_GetTimeInQSecFracFormat() >> 32; // Drop the fractionnal second. Keep the whole second part only.
   if ( (currentTime - lastUpdate) < (SECONDS_PER_DAY*30) ) {
      retVal = (bool)true;
   }
   if ( seconds != NULL ) {
      *seconds = lastUpdate;
   }

   return ( retVal );
}

/******************************************************************************

   Function Name: RADIO_TCXO_Set

   Purpose: This function keeps track of the latest radio TCXO frequency estimate

   Arguments:  radioNum - Radio number
               TCXOfreq - new frequency
               source   - source of the frequency computation
               reset    - reset last update time

   Returns: TRUE if frequency was updated.

   Notes:

******************************************************************************/
bool RADIO_TCXO_Set ( uint8_t radioNum, uint32_t TCXOfreq, TIME_SYS_SOURCE_e source, bool reset )
{
   bool retVal = (bool)false;

   // Sanity check on TCXO frequency
   // Make sure radio TCXO is within expected values (30MHz +/-25ppm)
   if ( (TCXOfreq > RADIO_TCXO_MIN) && (TCXOfreq < RADIO_TCXO_MAX) ) {
      uint32_t primask = __get_PRIMASK();
#if ( RTOS_SELECTION == MQX_RTOS ) //TODO Melvin: add the below code once disable interrupt is added
      __disable_interrupt(); // This is critical but fast. Disable all interrupts.
#endif

      // Check if the new TCXO frequency is different than the current one.
      if ( radio[radioNum].TCXOFreq != TCXOfreq ) {
         radio[radioNum].TCXOFreq = TCXOfreq; // This is now the new frequency
         retVal = (bool)true;
      }
      radio[radioNum].TCXOsource = source; // Update the source even if frequency wasn't updated. We want to know who called this function last.
      __set_PRIMASK(primask); // Restore interrupts

      // Update AFC error if TCXO frequency was changed
      if ( retVal ) {

         PHY_AfcAdjustment_Set_By_Radio(radioNum, (int16_t)((int32_t)TCXOfreq-(int32_t)RADIO_TCXO_NOMINAL));

      }

      // Update timestamp even if the frequency wasn't updated.
      // This way we can see that the updates are still happening (i.e. this function is called) even though the frequency didn't change.
      if ( reset ) {
         radio[radioNum].TCXOLastUpdate = 0;
      } else {
         radio[radioNum].TCXOLastUpdate = TIME_UTIL_GetTimeInQSecFracFormat() >> 32; // Drop fractional part
      }
   }
   return retVal;
}

/******************************************************************************

   Function Name: checkRadioPart

   Purpose: This function check for a valid Silabs part number

   Arguments: radioNum     - Radio number
              check number - Used to print a different message depending on the call.

   Returns: part info

   Notes: will reboot is part number is not valid

******************************************************************************/
static uint16_t checkRadioPart( uint8_t radioNum, uint32_t checkNum )
{
   uint8_t error;
   union   si446x_cmd_reply_union Si446xCmd;

   // Check radio part number
   error = si446x_part_info(radioNum, &Si446xCmd);
   if ( error || ((Si446xCmd.PART_INFO.PART != 0x4460) && (Si446xCmd.PART_INFO.PART != 0x4467) && (Si446xCmd.PART_INFO.PART != 0x4468)) ) {
      ERR_printf("Unsupported radio %u is Si%04X or radio error. Check #%u. Rebooting...", radioNum, Si446xCmd.PART_INFO.PART, checkNum);
      OS_TASK_Sleep(ONE_SEC); // Give time to print message
#if 0 //TODO Melvin: add this section once PWR_ module is added
      PWR_SafeReset();        // Execute Software Reset, with cache flush
#endif
   }
   return Si446xCmd.PART_INFO.PART;
}
/******************************************************************************

   Function Name: vRadio_Init

   Purpose: This function Initialize all radios to a specific mode

   Arguments: mode - Normal, PN9 or CW

   Returns:

   Notes:

******************************************************************************/
void vRadio_Init(RADIO_MODE_t radioMode)
{
   uint32_t i; // Loop counter
   uint8_t radioNum;
   uint8_t RssiJumpThreshold;
   uint8_t PowerSetting;
   uint8_t const *pConfiguration;
   PHY_GetReq_t GetReq;
   PHY_DETECTION_e detectionList[PHY_RCVR_COUNT]; // Detection list
   PHY_FRAMING_e   framingList[PHY_RCVR_COUNT];   // Framing list
   PHY_MODE_e      modeList[PHY_RCVR_COUNT];      // Mode list
   int16_t         AfcAdjustment[PHY_RCVR_COUNT];
   uint8_t         demodulatorList[PHY_RCVR_COUNT]; // Demodulator used by the each receiver. SOFT or HARD.
   uint16_t        partInfo[PHY_RCVR_COUNT];
   union           si446x_cmd_reply_union Si446xCmd;

#if ( DCU == 1 )
   GPIO_INIT_t     settings; // Holds GPIO interrupt configuration
#endif
   bool fastSPI = false;

   (void)OS_MUTEX_Create(&radioMutex);

   currentTxMode = radioMode; // Save radio mode

   // For PN9 and CW, BER test mode, we boot the radio in normal mode and the calling function
   // will put the radio in PN9 or CW or BER mode by setting the appropriate bits in the radio.
   if ( (radioMode == eRADIO_MODE_PN9) || (radioMode == eRADIO_MODE_CW) || (radioMode == eRADIO_MODE_4GFSK_BER) || (radioMode == eRADIO_MODE_NORMAL_BER) ){
      radioMode = eRADIO_MODE_NORMAL;
   }

   // Clear all variables
   (void)memset(radio, 0, sizeof(radio));

#if 1 //TODO RA6E1 Bob: As long as the file system and PHY task is running, these lines can be enabled
   GetReq.eAttribute = ePhyAttr_RssiJumpThreshold;
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&RssiJumpThreshold);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Get preamble & sync word detection
   GetReq.eAttribute = ePhyAttr_RxDetectionConfig;
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)detectionList); //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Get framing mode
   GetReq.eAttribute = ePhyAttr_RxFramingConfig;
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)framingList);   //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Get Rx mode
   GetReq.eAttribute = ePhyAttr_RxMode;
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)modeList);   //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Get demodulators
   GetReq.eAttribute = ePhyAttr_Demodulator;
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)demodulatorList);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Get AFC error
   GetReq.eAttribute = ePhyAttr_AfcAdjustment;
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)AfcAdjustment);   //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#else // TODO: RA6E1 Bob: temporary code to set default values until PHY_Attribute_Get is ready
   RssiJumpThreshold  = PHY_RSSI_JUMP_THRESHOLD_DEFAULT;
   detectionList[0]   = ePHY_DETECTION_0;
   framingList[0]     = ePHY_FRAMING_0;
   modeList[0]        = ePHY_MODE_1;
   demodulatorList[0] = 0; // Hard demodulator
   AfcAdjustment[0]   = 0;
#endif

   // Configure radio SDN pin and hold all radios in reset while configuring
   RDO_SDN_TRIS();      // First radio on Frodo and Samwise
   RX_RADIO_SDN_TRIS(); // All other radios on Frodo

   // Configure CPU pin connected to radio 0 GPIO0 pin
   RDO_0_GPIO0_TRIS(); // Digital input on Samwise and Frodo.
#if 1 //TODO: RA6E1 Bob: These have tentatively been completed, to be tested
   // Configure CPU pin connected to radio 0 GPIO1 pin
   RDO_0_GPIO1_TRIS(); // Digital input on Samwise and Frodo.

   // Configure CPU pin connected to radio 0 GPIO2 pin
   RDO_0_GPIO2_TRIS(); // Digital input on Samwise. Pin not connected on Frodo.

   // Configure CPU pin connected to radio 0 GPIO3 pin
   RDO_0_GPIO3_TRIS(); // Digital input on Samwise. Pin not connected on Frodo.

   // Configure power amplifier TX/RX pin. Set to RX.
   RDO_RX0TX1_TRIS();

   // Configure power amplifier enable pin. Set to disable.
   RDO_PA_EN_TRIS();

   // Turn on radio oscillator effectively starting the radio
   RDO_OSC_EN_TRIS(); // On Samwise only
#endif
   RDO_OSC_EN_ON();
   OS_TASK_Sleep( FIVE_MSEC ); // Wait at least 3 ms as per datasheet

   // Init SPI
   // Need to set the SPI bus to fast speed if any demodulator will use the soft-demodulator
   for (i=0; i<PHY_RCVR_COUNT; i++) {
      if (demodulatorList[i] == 1) {
         fastSPI = true;
      }
   }
   radio_hal_SpiInit(fastSPI);

   // Power Up the radio chips
   vRadio_PowerUp();

   OS_TASK_Sleep( TENTH_SEC ); // The radio type (e.g. Si4460) is sometimes not indentify properly. It seems that waiting after reset avoids some issues.

   // Power up radios and retrieve part info
   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      // Send Power Up command to the radio
      si446x_power_up(radioNum, 1, 1, RADIO_TCXO_NOMINAL);

      // Check radio part number
      partInfo[radioNum] = checkRadioPart( radioNum, 1);
   }

   // Now that we have the part info, redo the reset sequence to apply the patch and basic configuration
   // Power Up the radio chips
   vRadio_PowerUp();

   OS_TASK_Sleep( TENTH_SEC ); // The radio type (e.g. Si4460) is sometimes not indentify properly. It seems that waiting after reset avoids some issues.

   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      radio[radioNum].rx_channel        = PHY_INVALID_CHANNEL;
      radio[radioNum].RssiJumpThreshold = RssiJumpThreshold;
      radio[radioNum].configured        = (bool)false;
      radio[radioNum].detection         = detectionList[radioNum];
      radio[radioNum].framing           = framingList[radioNum];
      radio[radioNum].mode              = modeList[radioNum];
      radio[radioNum].demodulator       = demodulatorList[radioNum];

      // Select configuration
#if COMPARE == 1
      // This loads different templates in different radios to help find out the differences etween tamplates with the goal to update modeConfiguration
      if ( radioNum >= (sizeof(pRadioConfiguration)/sizeof(pRadioConfiguration[0])) ) {
         pConfiguration = pRadioConfiguration[0]->Radio_ConfigurationArray;
      } else {
         pConfiguration = pRadioConfiguration[radioNum]->Radio_ConfigurationArray;
      }
#else
      pConfiguration = pRadioConfiguration[radioMode]->Radio_ConfigurationArray;
#endif

      /* Load radio configuration */
      if (SI446X_SUCCESS != si446x_configuration_init(radioNum, pConfiguration, partInfo[radioNum]))
      {
         DBG_logPrintf('E', "Can't program radio %u. Will be marked as bad", radioNum);
         continue;
      }

      // This should not be needed since we went through that previously but let's check part info again just to be safe
      // Check radio part number
      (void)checkRadioPart( radioNum, 2);

      // Print radio part info
      INFO_printf("Radio %u is Si%04X", radioNum, partInfo[radioNum]);

      // Radio is configured for operation
      radio[radioNum].configured = (bool)true;
#if COMPARE == 0
      // NOTE: turn off this block if compare() needs to be executed
      // Configure RX radios to support SRF, GEN II or GEN I
//       if ( (mode == eRADIO_MODE_NORMAL) && (radioNum != (uint8_t)RADIO_0) ) {
      if ( (radioMode == eRADIO_MODE_NORMAL) ) {

         // Configure detection format
         (void)RADIO_RxDetectionConfig(radioNum, radio[radioNum].detection, (bool)true);

         // Configure framing format
         (void)RADIO_RxFramingConfig(radioNum, radio[radioNum].framing, (bool)true);

         // Configure mode
         (void)RADIO_RxModeConfig(radioNum, radio[radioNum].mode, (bool)true);
      }

      // Disable global radio interrupts until radio is started
      (void)Radio_int_disable(radioNum);

      // Force ready state because template sets the radio to either RX or TX.
      (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY); // Leave RX/TX state

      // Enable 129 byte FIFO and fast sequencer
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_GLOBAL,                     // Property group.
                                 1,                                             // Number of property to be set.
                                 SI446X_PROP_GRP_INDEX_GLOBAL_CONFIG,           // Start sub-property address.
                                 SI446X_PROP_GLOBAL_CONFIG_SEQUENCER_MODE_BIT | // Sequencer mode set to fast (default)
                                 SI446X_PROP_GLOBAL_CONFIG_FIFO_MODE_BIT);      // TX/RX FIFO are sharing with 129-byte size buffer.

      // Reset FIFO. Needed for 129 byte FIFO to take effect.
      (void)si446x_fifo_info(radioNum, SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT, &Si446xCmd);

      // Configure latched Read RSSI to be readable from fast register A
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_FRR_CTL,                               // Property group.
                                 1,                                                        // Number of property to be set.
                                 SI446X_PROP_GRP_INDEX_FRR_CTL_A_MODE,                     // Start sub-property address.
                                 SI446X_PROP_FRR_CTL_A_MODE_FRR_A_MODE_ENUM_LATCHED_RSSI); // Read latch RSSI

      // Configure RSSI jump threshold
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,                                 // Property group.
                                 1,                                                        // Number of property to be set.
                                 SI446X_PROP_GRP_INDEX_MODEM_RSSI_JUMP_THRESH,             // Start sub-property address.
                                 radio[radioNum].RssiJumpThreshold);                       // RSSI jump threshold

      // Latch RSSI as soon as possible
      (void)Radio_latch_RSSI(radioNum, (bool)false);

      // Set power level
      if (radioNum == (uint8_t)RADIO_0) {
         GetReq.eAttribute = ePhyAttr_PowerSetting;
#if 1  //TODO RA6E1 Bob: As long as the file system and PHY task is running, this can remain 1
         (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&PowerSetting); //lint !e826   Suspicious pointer-to-pointer conversion
#endif
         RADIO_Power_Level_Set(PowerSetting);
      }

      // Set TX almost empty threshold and RX FIFO almost full threshold
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_PKT,
                                 2,
                                 SI446X_PROP_GRP_INDEX_PKT_TX_THRESHOLD,
                                 FIFO_ALMOST_EMPTY_THRESHOLD,
#if ( EP == 1 )
                                 demodulatorList[radioNum] == 0 ? FIFO_FULL_THRESHOLD_HARD:FIFO_FULL_THRESHOLD_SOFT);
#else
                                 FIFO_FULL_THRESHOLD_HARD);
#endif
#if ( ENABLE_PSM == 1 )
      // Enable DSA for preamble detection source
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_PREAMBLE,
                                 1,
                                 SI446X_PROP_GRP_INDEX_PREAMBLE_CONFIG,
                                 0x91);

      // Change BCR GEAR
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,
                                 1,
                                 SI446X_PROP_GRP_INDEX_MODEM_BCR_GEAR,
                                 0x10);

      // Change AFC settings
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,
                                 4,
                                 SI446X_PROP_GRP_INDEX_MODEM_AFC_GAIN,
                                 0xC0,  // AFC GAIN 1
                                 0x0A,  // AFC GAIN 0
                                 0x11,  // AFC LIMITER 1
                                 0xC8); // AFC LIMITER 0

      // Change RAW CONTROL
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,
                                 1,
                                 SI446X_PROP_GRP_INDEX_MODEM_RAW_CONTROL,
                                 0x83);

      // Change ANT DIV mode
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,
                                 1,
                                 SI446X_PROP_GRP_INDEX_MODEM_ANT_DIV_MODE,
                                 0x01);

      // Change RSSI control
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,
                                 2,
                                 SI446X_PROP_GRP_INDEX_MODEM_RSSI_CONTROL,
                                 0x0A,
                                 0x1A);

      // Configure registers
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,
                                 2,
                                 SI446X_PROP_GRP_INDEX_MODEM_SPIKE_DET,
                                 0x83,  // SPIKE DET
                                 0xC6); // ONE SHOT AFC

      // Change PSM/DSA registers
      (void)si446x_set_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,
                                 7,
                                 SI446X_PROP_GRP_INDEX_MODEM_PSM,
                                 0x03,  // PSM 1
                                 0xD3,  // PSM 0
                                 0x63,  // DSA_CTRL1
                                 0x24,  // DSA_CTRL2
                                 0x08,  // DSA_QUAL
                                 0x78,  // DSA_RSSI
                                 0x24); // DSA_MISC
#endif
#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
      // Configure GPIO0 to handle sync interrupt
      if (radioNum != (uint8_t)RADIO_0) {
         si446x_gpio_pin_cfg( radioNum,
                              SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_SYNC_WORD_DETECT, // GPIO0
                              SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,        // GPIO1
                              SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,        // GPIO2
                              SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING,        // GPIO3
                              SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,        // NIRQ
                              SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,          // SDO
                              SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5);   // GEN_CONFIG
      }
#endif
      // Clear interrupts
      (void)si446x_get_int_status_fast_clear(radioNum);

      // Update TCXO frequency with up to date value
      (void)RADIO_TCXO_Set( radioNum, (uint32_t)((int32_t)RADIO_TCXO_NOMINAL+(int32_t)AfcAdjustment[radioNum]), eTIME_SYS_SOURCE_NONE, (bool)true );

      // Configure GPIO 0 to output TCXO clock
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
      (void)si446x_gpio_pin_cfg( (uint8_t)RADIO_0, // Only need to do this on radio 0 for 9985T
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,    // GPIO0 Output TCXO for TCXO trimming
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_EN_PA,      // GPIO1 - falling edge generates TX done IRQ
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE1,     // GPIO2 set high - disable FEM PA
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TX_STATE,   // GPIO3 1 while transmitting then 0
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5, // Hope to reduce unintended noise with this setting
                                 &Si446xCmd);
#else
      (void)si446x_gpio_pin_cfg( radioNum,
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,   // GPIO0 Output TCXO for TCXO trimming
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING, // GPIO1
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING, // GPIO2
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DONOTHING, // GPI03
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                 SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5, // Hope to reduce unintended noise with this setting
                                 &Si446xCmd);
#endif

      // Output TCXO clk/30
#if ( DCU == 1 ) //TODO: Disabled for EP's when adjusting the Sys Tick with the RTC crystal
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
      (void)si446x_set_property( (uint8_t)RADIO_0, // Only need to do this on radio 0 for 9985T
#else
      (void)si446x_set_property( radioNum,
#endif
                                 SI446X_PROP_GRP_ID_GLOBAL,
                                 1,
                                 SI446X_PROP_GRP_INDEX_GLOBAL_CLK_CFG,
                                 SI446X_PROP_GLOBAL_CLK_CFG_DIVIDED_CLK_EN_TRUE_BIT |          // Enable divided system clock output
                                 SI446X_PROP_GLOBAL_CLK_CFG_DIVIDED_CLK_SEL_ENUM_DIV_30 << 3); // Divide clock by 30
#endif   //#if ( DCU == 1 ) //Disable for EP's when adjusting the Sys Tick with the RTC crystal
#endif // #if COMPARE == 0
#if ( DCU == 1 )
      // On the T-board, booting all radios can take time and starve other tasks.
      // Be nice to other tasks.
      OS_TASK_Sleep( FIFTY_MSEC );
#endif
   }
// Keep this right after all the templates are loaded to avoid looking at differences that could be after the fact.
#if COMPARE == 1
   compare();
#endif
   // Set PA to RX
   // This function needs RADIO_0 to be configured to work
   setPA(eRADIO_PA_MODE_RX);
#if ( DCU == 1 )
   // Configure radio IRQ pin
   settings.lwgpio_dir  = LWGPIO_DIR_INPUT;
   settings.pu_pd_od    = LWGPIO_ATTR_PULL_UP;
   settings.isInterrupt = true;
   settings.interrupt_type = LWGPIO_INT_MODE_FALLING;
   settings.priority_level = 3;

   for ( i=0; i < (sizeof(radioIntConfig)/sizeof(INT_CONFIG_t)); i++) {
      settings.port     = radioIntConfig[i].port;
      settings.pin      = radioIntConfig[i].pin;
      settings.callback = radioIntConfig[i].callback;
      GPIO_init(&settings, &radioIRQIntPin[i]);
   }
#else
   RDO_0_IRQ_TRIS(); // Make CPU pin IRQ_Si4460 into FTM1_CH0 to capture when a radio interrupt happens

   // Configure FTM1_CH0 to capture timer when radio IRQ is detected.
#if ( MCU_SELECTED == NXP_K24 ) //TODO: Add the AGT Timer module for input capture
   (void)FTM1_Channel_Enable( 0, FTM_CnSC_CHIE_MASK | FTM_CnSC_ELSB_MASK, Radio0_IRQ_ISR ); // Capture on falling edge
#elif ( MCU_SELECTED == RA6E1 )
//TODO Melvin: Add AGT Timer
#endif
#endif
#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {

      // Program GPIO2 for RSSI jump test from Marius
      Pro2Cmd[0] = 0xF1; // POKE
      Pro2Cmd[1] = 0x50;
      Pro2Cmd[2] = 0xA4;
      Pro2Cmd[3] = 0x54;
      (void)radio_comm_SendCmd( radioNum, 4, Pro2Cmd );

      Pro2Cmd[0] = 0xF1; // POKE
      Pro2Cmd[1] = 0x50;
      Pro2Cmd[2] = 0x9B;
      Pro2Cmd[3] = 0x04;
      (void)radio_comm_SendCmd( radioNum, 4, Pro2Cmd );

      Pro2Cmd[0] = 0xF1; // POKE
      Pro2Cmd[1] = 0x50;
      Pro2Cmd[2] = 0xB0;
      Pro2Cmd[3] = 0x1C;
      (void)radio_comm_SendCmd( radioNum, 4, Pro2Cmd );

      // Program GPIO3 for RSSI jump test from Marius
      Pro2Cmd[0] = 0xF1; // POKE
      Pro2Cmd[1] = 0x50;
      Pro2Cmd[2] = 0xA5;
      Pro2Cmd[3] = 0x54;
      (void)radio_comm_SendCmd( radioNum, 4, Pro2Cmd );

      Pro2Cmd[0] = 0xF1; // POKE
      Pro2Cmd[1] = 0x50;
      Pro2Cmd[2] = 0x9C;
      Pro2Cmd[3] = 0x04;
      (void)radio_comm_SendCmd( radioNum, 4, Pro2Cmd );

      Pro2Cmd[0] = 0xF1; // POKE
      Pro2Cmd[1] = 0x50;
      Pro2Cmd[2] = 0xB2;
      Pro2Cmd[3] = 0x1C;
      (void)radio_comm_SendCmd( radioNum, 4, Pro2Cmd );
   }
#endif

   // Calibrate all radios and put in standby state
   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      if (radio[radioNum].configured) {
         (void)Radio_Calibrate(radioNum);
         if ( radioNum != (uint8_t)RADIO_0 ) {
            // Put radios in standby to save power but keep radio 0 in ready so that we can get the 30MHz output running
            (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);  // Force standby state
         }
      }
   }
#if ( DCU == 1 )
   // On the T-board, booting all radios can take time and starve other tasks.
   // Be nice to other tasks.
   OS_TASK_Sleep( FIFTY_MSEC );
#endif

   // Start triming CPU or TCXO frequency
#if 0  //TODO Melvin: add the below code once PHY  module is added
   RADIO_Update_Freq();
#endif
}

#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
static uint8_t reverseByte(uint8_t val)
{
   uint8_t result = 0;
   uint32_t i;

   for (i=0; i<8; i++)
   {
      result <<= 1;
      result |= (uint8_t)(val & 1);
      val = (uint8_t)(val >> 1);
   }

   return result;
}
#endif

/***********************************************************************************************************************
 *
 * Function name: printHex
 *
 * Purpose: Prints an array as hex ascii to MFG port with the log information.
 *
 * Arguments: char const    *rawStr
 *            const uint8_t *str
 *            uint16_t      num_bytes
 *
 * Returns: None
 *
 * Notes:
 *
 **********************************************************************************************************************/
void printHex ( char const *rawStr, const uint8_t *str, uint16_t num_bytes )
{
   // Use MFG port
#if ( USE_USB_MFG != 0 )
   uint32_t     i;
   char         pDst[2];
   // Print the raw string
   usb_puts( (char*)rawStr );

   // Print str in hex
   for (i=0; i<num_bytes; i++) {
      ASCII_htoa((uint8_t*) pDst, str[i]);
      usb_putc( pDst[0] );
      usb_putc( pDst[1] );
   }
   usb_putc( '\n' );
#else
#if 0 //TODO: RA6E1 Bob: need to figure out why this was needed and map to different printf function
   MQX_FILE_PTR stdout_ptr;       /* mqx file pointer for UART  */
   stdout_ptr = fopen("ittya:", NULL);

   // Print the raw string
   (void)fprintf( stdout_ptr, rawStr );

   // Print str in hex
   for (i=0; i<num_bytes; i++) {
      ASCII_htoa((uint8_t*) pDst, str[i]);
      (void)fputc( pDst[0], stdout_ptr );
      (void)fputc( pDst[1], stdout_ptr );
   }
   (void)fprintf( stdout_ptr, "\n" );
   // Close port
   (void)fflush( stdout_ptr );
   (void)fclose( stdout_ptr );
#endif
#endif
}

/***********************************************************************************************************************
 *
 * Function name: FEClength
 *
 * Purpose: Compute START GEN I and GEN II FEC length
 *
 * Arguments: uint8_t *buffer
 *
 * Returns: FEC length
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t FEClength(uint8_t const *buffer)
{
   // Check if FEC is present
   if ( (buffer[2] & 0x4) == 0)
      return STAR_FEC_LENGTH;
   else
      return 0;
}

/******************************************************************************

   Function Name: Radio_CaptureRSSI

   Purpose: Capture RSSI into a circular buffer

   Arguments: radioNum - radio to process

   Returns:

   Notes:

******************************************************************************/
uint16_t RnCrc_Gen (uint16_t accum, uint8_t ch)
{
   return ( uint16_t )((accum >> 4) ^ rncrctab[(accum ^ ch) & 0x00F]);
}

/******************************************************************************

   Function Name: Radio_CaptureRSSI

   Purpose: Capture RSSI into a circular buffer

   Arguments: radioNum - radio to process

   Returns:

   Notes:

******************************************************************************/
void RADIO_CaptureRSSI(uint8_t radioNum)
{
   const uint8_t SZ = (uint8_t)sizeof(radio[radioNum].prev_rssi);

   if ( radio[radioNum].demodulator == 0 ) {
      if ( radio[radioNum].rssiIndex < sizeof(radio[radioNum].rssi) ) {
         radio[radioNum].rssi[radio[radioNum].rssiIndex++] = RADIO_Get_CurrentRSSI(radioNum); // Capture now
      }
   } else { //Soft Demod
      if (radio[radioNum].prev_rssiIndex >= SZ) {
         radio[radioNum].prev_rssiIndex = 0;
      }
      radio[radioNum].prev_rssi[radio[radioNum].prev_rssiIndex++] = RADIO_Get_CurrentRSSI(radioNum); // Capture now and buffer
   }
}

/******************************************************************************

   Function Name: RADIO_UnBufferRSSI

   Purpose: Remove RSSI from the circular buffer into the linear array for RSSI averaging

   Arguments: radioNum - radio to process

   Returns:

   Notes: The value removed is SD_RSSI_PIPELINE_DELAY behind the current value

******************************************************************************/
void RADIO_UnBufferRSSI(uint8_t radioNum)
{
   const uint8_t SZ = (uint8_t)sizeof(radio[radioNum].prev_rssi);
   uint8_t i;
   if ( radio[radioNum].rssiIndex < sizeof(radio[radioNum].rssi) ) { // Make sure we don't overflow array
      i = (uint8_t)((radio[radioNum].prev_rssiIndex+SZ-SD_RSSI_PIPELINE_DELAY)%SZ); //Make sure i isn't start negative
      radio[radioNum].rssi[radio[radioNum].rssiIndex++] = radio[radioNum].prev_rssi[i];
   }
}


/***********************************************************************************************************************
 *
 * Function name: ValidateGenI
 *
 * Purpose: Validate a STAR Gen I message
 *
 * Arguments: radioNum - radio to process
 *
 * Returns: True if valid, false otherwise
 *
 * Side effect: none
 *
 **********************************************************************************************************************/
static bool ValidateGenI(uint8_t radioNum)
{
   uint32_t i; // Loop counter
   uint32_t j; // Loop counter
   uint32_t nibbleLength;
   uint16_t CRC = 0xFFFF;
   uint16_t msgCRC = 0;

   // Get length of the message in nibble without CRC
   nibbleLength = ( uint32_t )( GenILength[radio[radioNum].RadioBuffer[0] & 0xF] - 4 ); // Subtract CRC nibbles

   // Compute CRC over message
   for (i=0, j=0; i<nibbleLength; i++, j++)
   {
      if (j & 1) {
         // Process high nibble
         CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i/2] >> 4);
      } else {
         // Process low nibble
         CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i/2] & 0x0F);
      }
   }

   // Force the message MSB CRC to match calculated MSB CRC
   // The last bit is not properly sent on some legacy stuff so we ignore that bit.
   // The last transmitted bit is the CRC most significant bit.
   if (CRC & 0x8000) {
      // Set bit
      if ((i+3)&1) {
         // Process high nibble
         radio[radioNum].RadioBuffer[(i+3)/2] |= 0x80;
      } else {
         // Process low nibble
         radio[radioNum].RadioBuffer[(i+3)/2] |= 0x08;
      }
   } else {
      // Clear bit
      if ((i+3)&1) {
         // Process high nibble
         radio[radioNum].RadioBuffer[(i+3)/2] &= ~0x80;
      } else {
         // Process low nibble
         radio[radioNum].RadioBuffer[(i+3)/2] &= ~0x08;
      }
   }

   // Extract message CRC
   for (j=0; j<4; i++, j++)
   {
      if (i&1) {
         // Process high nibble
         msgCRC |= (uint16_t)((radio[radioNum].RadioBuffer[i/2] >> 4) << (j*4));//lint !e701 shift left of signed quantity
      } else {
         // Process low nibble
         msgCRC |= (uint16_t)((radio[radioNum].RadioBuffer[i/2] & 0x0F) << (j*4));//lint !e701 shift left of signed quantity
      }
   }

   // Zero out high nibble if message is odd length
   if (i&1) {
      radio[radioNum].RadioBuffer[i/2] &= 0x0F;
   }

   // Validate CRC
   if ( CRC == msgCRC )
   {
      return true;
   }
   return false;
}

/***********************************************************************************************************************
 *
 * Function name: ValidateGenII
 *
 * Purpose: Validate a STAR Gen II message
 *
 * Arguments: radioNum - radio to process
 *
 * Returns: True if valid, false otherwise
 *
 * Notes: We decode the message based on length but if it fails to decode,
 *        we iterate through all possible length size since the original length might have been corrupted.
 *
 **********************************************************************************************************************/
static bool ValidateGenII(uint8_t radioNum)
{
   uint32_t i; // Loop counter
   uint32_t length;
//   uint32_t startLength;
//   uint32_t endLength;
//   uint32_t overhead;
   uint16_t CRC;
   uint8_t  originalBuffer[PHY_STAR_MAX_LENGTH];
   bool     checkCRC = (bool)false;

   PrepGFMatLog();

   // Since we assume a Gen II type message, we can force Gen I message field to Gen II type.
   radio[radioNum].RadioBuffer[0] = (radio[radioNum].RadioBuffer[0] & 0xF0) | 0x0A;

   // Assume that FEC is always present on STAR Gen II
   radio[radioNum].RadioBuffer[2] &= ~0x04; // FEC is present when bit is 0

   // Save original message
   (void)memcpy(originalBuffer, radio[radioNum].RadioBuffer, sizeof(originalBuffer));

   // Decode message as is
   length = FrameLength_Get(radio[radioNum].framing, radio[radioNum].mode, ePHY_MODE_PARAMETERS_0, radio[radioNum].RadioBuffer);
   // Decode message with RS if FEC is present
   if ((radio[radioNum].RadioBuffer[2] & 0x04) == 0) {
#if 0  // Not RA6E1.  This was already removed in the K24 baseline code
      // todo: 01/24/17 1:11 PM [MKD] - This code doesn't work for message length 43 and 44 bytes (not sure 45 is legal). Need to investigate.

      // Decode message using Reed-Solomon
      if (RS_Decode(RS_STAR_63_59, radio[radioNum].RadioBuffer, &radio[radioNum].RadioBuffer[length], (uint8_t)length)) {
         // Sucessfully corrected a message
         // Check CRC
         checkCRC = (bool)true;
      }

#else
      int8_t   adjustedLength;
      adjustedLength = (int8_t)RSDecode(radio[radioNum].RadioBuffer, (uint8_t)(length+STAR_FEC_LENGTH));
      if ( adjustedLength < 0 ) {
         // Message has errors
         if ( RSRepair(radio[radioNum].RadioBuffer, (uint8_t)(-adjustedLength)) ) {
            // Fixed message so check CRC
            checkCRC = true;
         }
      } else {
         // No correction needed so check CRC
         checkCRC = true;
      }
#endif
   }

   // Validate CRC when FEC is suppress or FEC correction was sucessful
   if ((radio[radioNum].RadioBuffer[2] & 0x04) || checkCRC) {
      CRC = 0xFFFF;
      for (i=0; i<length; i++)
      {
         CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] & 0x0F);
         CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] >> 4);
      }
      if ( CRC == 0 )
      {
         return true;
      }
   }

#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
   // [MKD] 2019-01-31 11:46 I disable this code section because it increases false positive. It's better to have lower false positive then higher performance.
   // I keep the code incase we find a way to filter false positive out.

   // At this point, we can't decode the message so we will iterate through some fields.
   // We might get lucky and get a valid message.
   // We are going to assume that FEC is present.

   // The iteration of the length field will be done in 2 phases
   // Phase 1 will cover message group 0x, 2x, 3x, 8x, 9x and Bx
   // Phase 2 will cover message group 4x, 5x, Cx, and Dx (where 16 bytes are implicitely added to length)
   for (j=0; j<2; j++) {
      if (j == 0) {
         // Set iteration limits for message group 0x, 2x, 3x, 8x, 9x and Bx
         startLength = 5;  // Smallest message length possible including header and CRC length
         endLength   = 37; // Largest message length possible including header and CRC length
         overhead    = 5;  // Header length and CRC
      } else {
         // Set iteration limits for message group 4x, 5x, Cx, and Dx
         startLength = 21; // Smallest message length possible including header, CRC length and 16 bytes because of message group
         endLength   = 46; // Largest message length possible including header, CRC length and 16 bytes because of message group
         overhead    = 21; // Header length, CRC length and 16 bytes because of message group
      }

      // Decode message using Reed-Solomon by iterating the length field
      for (length = startLength; length < endLength; length++) {
         // Restore original message because it could have been altered by previous iteration
         (void)memcpy(radio[radioNum].RadioBuffer, originalBuffer, PHY_STAR_MAX_LENGTH);

         // Assume that FEC is present
         radio[radioNum].RadioBuffer[2] &= ~0x04; // FEC is present when bit is 0

         // Force length field
         radio[radioNum].RadioBuffer[2] = (uint8_t)((radio[radioNum].RadioBuffer[2] & 0x7) | ((length-overhead) << 3)); // Remove header and CRC size
#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
         // todo: 01/24/17 1:11 PM [MKD] - This code doesn't work for message length 43 and 44 bytes (not sure 45 is legal). Need to investigate.

         // Decode message
         if (RS_Decode(RS_STAR_63_59, radio[radioNum].RadioBuffer, &radio[radioNum].RadioBuffer[length], (uint8_t)length)) {
            // Sucessfully corrected a message. Validate CRC
            CRC = 0xFFFF;
            for (i=0; i<length; i++)
            {
               CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] & 0x0F);
               CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] >> 4);
            }
            if ( CRC == 0 )
            {
               return true;
            }
         }
#else
         int8_t   adjustedLength;
         adjustedLength = (int8_t)RSDecode(radio[radioNum].RadioBuffer, (uint8_t)(length+STAR_FEC_LENGTH));
         if ( adjustedLength < 0 ) {
            // Message has errors
            if ( RSRepair(radio[radioNum].RadioBuffer, (uint8_t)(-adjustedLength)) ) {
               // Sucessfully corrected a message. Validate CRC
               CRC = 0xFFFF;
               for (i=0; i<length; i++)
               {
                  CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] & 0x0F);
                  CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] >> 4);
               }
               if ( CRC == 0 )
               {
                  return true;
               }
            }
         }
#endif
      }
   }
#endif
   // At this point, we can't decode the message so we write back the original buffer
   (void)memcpy(radio[radioNum].RadioBuffer, originalBuffer, PHY_STAR_MAX_LENGTH);
   return (bool)false;
}

/***********************************************************************************************************************
 *
 * Function name: ValidateSrfnHeader
 *
 * Purpose: Validate a SRFN header
 *
 * Arguments: radioNum - radio to process
 *
 * Returns: True if valid, false otherwise
 *
 * Notes: We decode the message based on length but if it fails to decode,
 *        we iterate through all possible length size since the original length might have been corrupted.
 *
 **********************************************************************************************************************/
static bool validateSrfnHeader(uint8_t radioNum)
{
   bool       headerValid;
   uint8_t    raw[max(PHY_STAR_MAX_LENGTH,PHY_HEADER_SIZE)]; //lint !e506  Largest value between PHY_HEADER_SIZE and STAR message length
   uint8_t    version;
   PHY_MODE_e mode;
   PHY_MODE_PARAMETERS_e modeParameters;// The mode parameters used in conjunction with mode 1
   uint8_t    length;
   char       rawStr[]     = "Raw x ";
   char       decodedStr[] = "Decoded x ";
   char       floatStr[PRINT_FLOAT_SIZE];

   (void)memcpy(raw, radio[radioNum].RadioBuffer, sizeof(raw));

   // Do error correction
   if (headerValid = HeaderParityCheck_Validate(radio[radioNum].RadioBuffer)) { //lint !e720   Boolean test of assignment
      // Validate CRC
      if (headerValid = HeaderCheckSequence_Validate(radio[radioNum].RadioBuffer)) { //lint !e720   Boolean test of assignment
         // Valid header
         version        = HeaderVersion_Get(               radio[radioNum].RadioBuffer);
         mode           = HeaderMode_Get(                  radio[radioNum].RadioBuffer);
         modeParameters = HeaderModeParameters_Get(        radio[radioNum].RadioBuffer);
         length         = HeaderLength_Get(ePHY_FRAMING_0, radio[radioNum].RadioBuffer);

         // Save value for later
         radio[radioNum].modeParameters = modeParameters;

         if((version        == 0)                         &&
            (mode           == ePHY_MODE_1)               &&
            (modeParameters <  ePHY_MODE_PARAMETERS_LAST) &&
            (length         <= PHY_MAX_PAYLOAD))
         {
            // Header is sound
            radio[radioNum].length = FrameLength_Get(ePHY_FRAMING_0, mode, modeParameters, radio[radioNum].RadioBuffer);
            radio[radioNum].validHeader = true;
         } else {
            PHY_MalformedHeaderCount_Inc(radioNum);
            INFO_printf("ERROR - PHY header malformed on radio %u", radioNum);
            headerValid = false;
         }
      } else {
         PHY_FailedHcsCount_Inc(radioNum);
         INFO_printf("ERROR - PHY header invalid CRC on radio %u", radioNum);
      }
   } else {
      PHY_FailedHeaderDecodeCount_Inc(radioNum);
      INFO_printf("ERROR - PHY header invalid parity check on radio %u", radioNum);
   }

   if (!headerValid) {
      // Add radio number to string
      rawStr[    4] = radioNum + '0';
      decodedStr[8] = radioNum + '0';

      INFO_printHex(rawStr, raw, PHY_HEADER_SIZE);
      INFO_printHex(decodedStr, radio[radioNum].RadioBuffer, PHY_HEADER_SIZE);

      // Print some result to MFG port
      if (MFG_Port_Print) {
         char syncStr[25];

         // Add radio number to string
         ( void )sprintf( syncStr, "Sync %u RSSI %s dBm", radioNum,
                           DBG_printFloat(floatStr, RSSI_RAW_TO_DBM(radio[radioNum].rssi[0]), 1));
         printHex ( syncStr, NULL, 0 );
      }
      if (radio[radioNum].demodulator == 0) {
         // Restart Radio
         INFO_printf("Radio start from Radio_RX_FIFO_Almost_Full invalid PHY on radio %u", radioNum);
         vRadio_StartRX(radioNum, radio[radioNum].rx_channel);
      }
   }
   return (headerValid);
}

/***********************************************************************************************************************
 *
 * Function name: ValidatePhyPayload
 *
 * Purpose: Validate a PHY payload
 *
 * Arguments: radioNum - radio to process
 *
 * Returns:
 *
 * Notes:
 *
 **********************************************************************************************************************/
static void validatePhyPayload(uint8_t radioNum)
{
   uint32_t i, max; // Loop counter
   uint16_t CRC;
   bool eventSet = (bool)false;
   char rawStr[] = "Raw x "; // The x will be replaced by the radio number. It's just a place holder.
   char decodedStr[] = "Decoded x ";
   char floatStr[PRINT_FLOAT_SIZE];
   uint16_t originalLength;
   TX_FRAME_t txFrame;

   /* Add radio number to string */
   rawStr[4]     = radioNum + '0';
   decodedStr[8] = radioNum + '0';

   /* Start radio ASAP */
   if (radio[radioNum].demodulator == 0) {
      vRadio_StartRX(radioNum, radio[radioNum].rx_channel); // Restart radio.
      INFO_printf("Radio start from Radio_RX_FIFO_Almost_Full valid PHY on radio %u", radioNum);
   }
   // Print raw message to MFG port
   if (MFG_Port_Print) {
      char syncStr[25];

      ( void )sprintf( syncStr, "Sync %u RSSI %s dBm", radioNum, DBG_printFloat(floatStr, RSSI_RAW_TO_DBM(radio[radioNum].rssi[0]), 1));
      printHex ( syncStr, NULL, 0 );
      printHex ( rawStr, radio[radioNum].RadioBuffer, radio[radioNum].length);
   }

   // SRFN Processing
   if (radio[radioNum].framing == ePHY_FRAMING_0) {
      // Print header info
      INFO_printf(  "Received SRFN: Ver %u, Mode %u, Mode Parameters %u, Length %u, MSG length %u",
                     HeaderVersion_Get(radio[radioNum].RadioBuffer),
                     radio[radioNum].mode,
                     radio[radioNum].modeParameters,
                     HeaderLength_Get( ePHY_FRAMING_0, radio[radioNum].RadioBuffer),
                     radio[radioNum].length);
      // Print RAW message
      INFO_printHex(rawStr, radio[radioNum].RadioBuffer, radio[radioNum].length );

      // Validate message
      if (FrameParityCheck_Validate(&radio[radioNum].RadioBuffer[PHY_HEADER_SIZE],
                                    HeaderLength_Get( radio[radioNum].framing, radio[radioNum].RadioBuffer ),
                                    radio[radioNum].mode,
                                    radio[radioNum].modeParameters)) {
         eventSet = (bool)true; // Signal the RX DATA Event to PHY
      } else {
         INFO_printf("ERROR - PHY Frame Parity Check Failed on radio %u", radioNum);
         PHY_CounterInc(ePHY_FailedFrameDecodeCount, radioNum);
      }
      // Print decoded message
      INFO_printHex(decodedStr, radio[radioNum].RadioBuffer, radio[radioNum].length);
   }
   // Validate packet for STAR gen I
   else if (radio[radioNum].framing == ePHY_FRAMING_1) {
      // Print header info
      INFO_printf(  "Received Gen I: Msg Type 0x%1X, MSG length %u",
                     radio[radioNum].RadioBuffer[0] & 0xF,
                     radio[radioNum].length);
      // Print RAW message
      INFO_printHex(rawStr, radio[radioNum].RadioBuffer, radio[radioNum].length );

      // Validate message
      if ( ValidateGenI(radioNum) ) {
         eventSet = true; // Signal the RX DATA Event to PHY
      } else {
         INFO_printf("ERROR - PHY Gen I Frame Parity Check Failed on radio %u", radioNum);
         PHY_FailedHcsCount_Inc(radioNum);
      }
   }
   // Validate packet for STAR gen II
   else if (radio[radioNum].framing == ePHY_FRAMING_2) {
      // The length is not really protected by FEC so we grab it now. Watch out, it could have been corrupted so do some sanity check.
      originalLength = FrameLength_Get(radio[radioNum].framing, radio[radioNum].mode, ePHY_MODE_PARAMETERS_0, radio[radioNum].RadioBuffer);
      radio[radioNum].length = min(originalLength+STAR_FEC_LENGTH, PHY_STAR_MAX_LENGTH); // Add STAR_FEC_LENGTH because FEC is always present

      // Print header info
      INFO_printf(  "Received Gen II: Msg Type 0x%02X, MSG length %u",
                     radio[radioNum].RadioBuffer[1],
                     radio[radioNum].length);
      // Print RAW message
      INFO_printHex(rawStr, radio[radioNum].RadioBuffer, radio[radioNum].length );

      // Validate message
      if ( ValidateGenII(radioNum) ) {
         eventSet = true; // Signal the RX DATA Event to PHY

         // Print decoded message if valid so that we can see what bits were corrected
         // We need to rebuild the message because the FEC was lost as part of the validation process and might have been corrected so we need to rebuild anyway.
         // Same for the CRC.
         txFrame.Mode = ePHY_MODE_2;
         txFrame.Length = (uint8_t)((radio[radioNum].length-STAR_CRC_LENGTH)-STAR_FEC_LENGTH); // Get length of payload (i.e. no CRC and no FEC)
         (void)memcpy(txFrame.Payload, radio[radioNum].RadioBuffer, txFrame.Length);
         (void)Frame_Encode(&txFrame);                                                         // Re-encode payload
         (void)memcpy(radio[radioNum].RadioBuffer, txFrame.Payload, radio[radioNum].length);
         INFO_printHex(decodedStr, radio[radioNum].RadioBuffer, radio[radioNum].length);

         // Suppress FEC and update length. That's what NCC and AO expect.
         radio[radioNum].RadioBuffer[2] |= 0x04; // FEC is suppress when bit is 1
         radio[radioNum].length = FrameLength_Get(radio[radioNum].framing, radio[radioNum].mode, ePHY_MODE_PARAMETERS_0, radio[radioNum].RadioBuffer);

         // Recompute CRC and update since FEC was removed
         CRC = 0xFFFF;
         for (i=0; i<(uint32_t)(radio[radioNum].length-STAR_CRC_LENGTH); i++)
         {
            CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] & 0x0F);
            CRC = RnCrc_Gen(CRC, radio[radioNum].RadioBuffer[i] >> 4);
         }
         radio[radioNum].RadioBuffer[i++] = (uint8_t)CRC;
         radio[radioNum].RadioBuffer[i  ] = (uint8_t)(CRC >> 8);
      } else if (originalLength > PHY_STAR_MAX_LENGTH) {
         // Length is bad.
         INFO_printf("ERROR - PHY Gen II Header Malformed on radio %u", radioNum);
         PHY_CounterInc(ePHY_FailedFrameDecodeCount, radioNum);
      } else {
         INFO_printf("ERROR - PHY Gen II Frame Parity Check Failed on radio %u", radioNum);
         PHY_CounterInc(ePHY_FailedFrameDecodeCount, radioNum);
      }

   } else {
      ERR_printf("Unsupported framing %u", radio[radioNum].framing);
   }

   // Print decoded message to MFG port
   if (MFG_Port_Print) {
      printHex ( decodedStr, radio[radioNum].RadioBuffer, radio[radioNum].length);
   }

   if (eventSet) {   // Save RX data
      float sum=0.0f;
      // Drop the last n RSSI measurements because they are usualy captured past the end of the message so the values are invalid
      // The first value is always good because it was captured at SYNC
      max = radio[radioNum].rssiIndex; //Record a copy, since the PSLSNR could be called and increment the value while we are executing
      if (radio[radioNum].demodulator == 0)
      {
         if ( max > 2 ) {
            max -= 2;
         }
      }

      for (i=0; i<max; i++) {
         sum += powf(10.0f, (float)(radio[radioNum].rssi[i])/10.0f); // Convert to linear and average.
      }
      radio[radioNum].buf.RxBuffer.rssi_dbm = RSSI_RAW_TO_DBM(log10f(sum/max(1,i))*10.0f);   // Save RSSI in dBm. Avoid division by 0.

      (void)memcpy(  radio[radioNum].buf.RxBuffer.Payload,
                     radio[radioNum].RadioBuffer,
                     sizeof(radio[radioNum].buf.RxBuffer.Payload)); // Copy payload from Radio to PHY
      radio[radioNum].buf.RxBuffer.syncTime          = radio[radioNum].syncTime;
      radio[radioNum].buf.RxBuffer.syncTimeCYCCNT    = radio[radioNum].syncTimeCYCCNT;
      radio[radioNum].buf.RxBuffer.mode              = radio[radioNum].mode;
      radio[radioNum].buf.RxBuffer.modeParameters    = radio[radioNum].modeParameters;
      radio[radioNum].buf.RxBuffer.framing           = radio[radioNum].framing;
      radio[radioNum].buf.RxBuffer.frequencyOffset   = radio[radioNum].frequencyOffset;

#if NOISE_TEST_ENABLED == 1
      // After complete frame has been received read the noise RSSI on the channel
      char floatStr[PRINT_FLOAT_SIZE];
      (void) RADIO_Do_CCA(radioNum, radio[radioNum].rx_channel);

      radio[radioNum].buf.RxBuffer.noise_dbm  = (int16_t) RSSI_RAW_TO_DBM(radio[radioNum].CCArssi);
      INFO_printf("RxData");
      INFO_printf("Chan: %d Noise= %s dBm", radio[radioNum].rx_channel, DBG_printFloat(floatStr, RSSI_RAW_TO_DBM(radio[RADIO_0].CCArssi), 1));
#endif
      // A valid frame was received
      PHY_CounterInc(ePHY_FramesReceivedCount, radioNum);

#if ( EP == 1 )
#if NOISE_TEST_ENABLED == 1
      // For this test version, only update the PHY stats for a valid frame
      // Save to the next location
      CachedAttr.SyncHistory.Rssi[ CachedAttr.SyncHistory.Index%10] = rx_buffer->rssi_dbm;
      CachedAttr.SyncHistory.Index++;

      CachedAttr.NoiseHistory.Rssi[ CachedAttr.NoiseHistory.Index%10] = rx_buffer->noise_dbm;
      CachedAttr.NoiseHistory.Index++;
      writeCache();
#endif

      PHY_GetReq_t GetReq;
      int16_t Temperature; /*lint !e578 temperature same as local variable in another file  */
      bool    AfcEnable;
      int8_t  AfcTemperatureRange[2];

      // TCXO (aka AFC) aging compensation

      // Get AFC enable setting
      GetReq.eAttribute = ePhyAttr_AfcEnable;
      (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&AfcEnable);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

#if ( EP == 1 )
      // Skip AFC adjustment for now when doing soft-demod because this needs to be looked into and properly integrated.
      if (radio[(uint8_t)radioNum].demodulator == 1) {
         AfcEnable = false;
      }
#endif
      // Check that AFC is enabled
      if (AfcEnable) {
         // Get the temperature
         if ( RADIO_Temperature_Get(radioNum, &Temperature) ) {

            // Get temperature variable
            GetReq.eAttribute = ePhyAttr_AfcTemperatureRange;
#if 1 // TODO: RA6E1 Melvin: should be able to remove this now that file system and PHY task are working
            (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)AfcTemperatureRange);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#endif

            if ((Temperature >= AfcTemperatureRange[0]) && (Temperature <= AfcTemperatureRange[1])) {
               // Clip frequency if needed
               if (radio[radioNum].CWOffset > AFC_MAX_OFFSET) {
                  radio[radioNum].CWOffset = AFC_MAX_OFFSET;
               } else if (radio[radioNum].CWOffset < -AFC_MAX_OFFSET) {
                  radio[radioNum].CWOffset = -AFC_MAX_OFFSET;
               }
               // Compute frequency error and correct
               radio[radioNum].error = ((1-ALPHA)*radio[radioNum].error)+((ALPHA*radio[radioNum].CWOffset)*PHY_AFC_MIN_FREQ_STEP);
               radio[radioNum].adjustTCXO = true;
            }
         }
      }
#endif  // end of ( EP == 1 ) section
#if ( RTOS_SELECTION == MQX_RTOS )
      RADIO_Event_Set(eRADIO_RX_DATA, radioNum); // Signal the RX DATA Event to PHY
#elif ( RTOS_SELECTION == FREE_RTOS )
      RADIO_Event_Set(eRADIO_RX_DATA, radioNum, (bool)false); // Signal the RX DATA Event to PHY
#endif
   }

   return;
}

/******************************************************************************

   Function Name: Radio_RX_FIFO_Almost_Full

   Purpose: Process the RX FIFO almost full radio interrupt

   Arguments: radioNum - radio to process

   Returns: TRUE if radio was restarted, FALSE otherwise

   Notes:

******************************************************************************/
static bool Radio_RX_FIFO_Almost_Full(uint8_t radioNum)
{
   uint32_t i; // Loop counter
   bool     headerValid = (bool)true;
   bool     restart     = (bool)false;
   uint8_t  FIFOCount;
   union si446x_cmd_reply_union Si446xCmd;

//   si446x_get_modem_status(radioNum, 0xFF); // Don't clear int.  get_int_status needs those.
//   INFO_printf("FIFO RSSI = %sfdBm", DBG_printFloat(floatStr, RSSI_DB_TO_DBM(Si446xCmd.GET_MODEM_STATUS.CURR_RSSI), 1));

   /* Read the length of RX_FIFO */
   (void)si446x_fifo_info_fast_read(radioNum, &Si446xCmd);

   /* Read at least FIFO_FULL_THRESHOLD bytes (fixes bug where radio FIFO says it has 0 bytes. If we do not read at
      least the threshold amount, the radio may not clear interrupt cause)
   */
#if ( EP == 1 )
   FIFOCount = max(Si446xCmd.FIFO_INFO.RX_FIFO_COUNT, radio[(uint8_t)radioNum].demodulator == 0 ? FIFO_FULL_THRESHOLD_HARD:FIFO_FULL_THRESHOLD_SOFT);
#else
   FIFOCount = max(Si446xCmd.FIFO_INFO.RX_FIFO_COUNT, FIFO_FULL_THRESHOLD_HARD);
#endif
   // Undo bi-phase mark if necessary
   // This happens when using 2400bps
   if (radio[radioNum].mode == ePHY_MODE_3) {
      uint8_t  workbuff[FIFO_SIZE];
      uint8_t  byte;
      uint32_t pos;

      // Only read even number of bytes to make our life easier when undoing bi-phase mark
      FIFOCount &= 0xFE;

      // Save FIFO to work buffer
      si446x_read_rx_fifo(radioNum, FIFOCount, workbuff);

      // Retrieve index position into receive buffer
      pos = radio[radioNum].bytePos;

      // Undo bi-phase mark 2 bytes at the time
      for (i=0; i<FIFOCount; i+=2) {
         byte=0;
         byte |= ManTable[workbuff[i+1]>>4];
         byte <<= 2;
         byte |= ManTable[workbuff[i+1]&0xF];
         byte <<= 2;
         byte |= ManTable[workbuff[i  ]>>4];
         byte <<= 2;
         byte |= ManTable[workbuff[i  ]&0xF];
         // Save byte
         radio[radioNum].RadioBuffer[pos++] = byte;
      }
      // Adjust FIFO count size to half the original size to compensate for bi-phase mark conversion
      // If the original size was odd, it is first made even
      FIFOCount = (FIFOCount & 0xFE) >> 1;
   } else {
      // Make sure we will not overflow the buffer by limiting the number of bytes we can copy.
      FIFOCount = (uint8_t)min(FIFOCount, PHY_MAX_FRAME-radio[radioNum].bytePos);

      /* Packet RX */
      si446x_read_rx_fifo(radioNum, FIFOCount, &radio[radioNum].RadioBuffer[radio[radioNum].bytePos]);
   }

   // Adjust character pointer
   radio[radioNum].bytePos += FIFOCount;

   RADIO_CaptureRSSI(radioNum); // Capture the current RSSI to be averaged later

   // Validate header if header has not been validated yet and that we have received enough bytes to do so
   if ((radio[radioNum].validHeader == (bool)false) && (radio[radioNum].bytePos >= PHY_HEADER_SIZE)) {
      // Check for SRFN
      if (radio[radioNum].framing == ePHY_FRAMING_0) {
         headerValid = validateSrfnHeader(radioNum);
         restart = !headerValid; // If the header is invalid then the radio was restarted
      }
      // Check for STAR gen I
      else if (radio[radioNum].framing == ePHY_FRAMING_1) {
         // Validate message type using length
         // If length is 0 then it's an invalid message type.
         if ( GenILength[radio[radioNum].RadioBuffer[0] & 0xF] ){
            radio[radioNum].length = HeaderLength_Get(radio[radioNum].framing, radio[radioNum].RadioBuffer);
            radio[radioNum].validHeader = true;
         } else {
            // This is not a valid type
            INFO_printf("ERROR - Gen I invalid msg type %u. Radio start from Radio_RX_FIFO_Almost_Full radio %u", radio[radioNum].RadioBuffer[0] & 0xF, radioNum);
            vRadio_StartRX(radioNum, radio[radioNum].rx_channel);       // Restart radio.
            restart = true;
            PHY_MalformedHeaderCount_Inc(radioNum);
         }
      }
      // Check for STAR gen II
      else if (radio[radioNum].framing == ePHY_FRAMING_2) {
         // Unconditionally get maximum number of bytes OTA
         radio[radioNum].length = PHY_STAR_MAX_LENGTH;
         radio[radioNum].validHeader = (bool)true;
      }
      else
      // Any other framing mode unsupported for now
      {
         headerValid = (bool)false;
         INFO_printf("UNSUPPORTED MODE: Radio start from Radio_RX_FIFO_Almost_Full radio %u", radioNum);
         vRadio_StartRX(radioNum, radio[radioNum].rx_channel);       // Restart radio.
         restart = (bool)true;
      }
   }

   // Save data until message is received
   if ((radio[radioNum].validHeader == (bool)true) && headerValid ) {
      // Check if whole message is received
#if ( DCU == 1 )
   //if (radio[radioNum].bytePos >= radio[radioNum].length + FIFO_FULL_THRESHOLD_HARD) {
      if (radio[radioNum].bytePos >= radio[radioNum].length) {
#if ( TEST_TDMA == 1 )
         RED_LED_OFF();
#endif
         if ( (radio[radioNum].length%4) == 0) {
            wait_us(300);  //It was measured that the delay from Radio1_IRQ_ISR() to here was 670usec.
                           // The Radio takes roughly 900usec worst case scenario to flush the RSSI buffer after a message is received
                           // This delay is only necessary if the most recent FIFO read was full of message samples (L%4==0)
         }
         PHY_ReplaceLastRssiStat(radioNum); //Replace the old RSSI reading before radio is restarted
#else
      if (radio[radioNum].bytePos >= radio[radioNum].length) {
#endif
         validatePhyPayload(radioNum);
         restart = (bool)true;
      }
   }

   return restart;
}

/*!
 *  Process the Preamble Detected interrupt
 *
 *  Arguments: radioNum - Radio to use
 *
 *  @return
 *
 *  @note
 *
 */
void RADIO_PreambleDetected(uint8_t radioNum)
{
   RadioEvent_PreambleDetect(radioNum);
#if ( DCU == 1 )
   // We don't print this message on a K22 board because it increases the noise floor and prevents a successful decoding.
   // It is possible that printing is no longer a problem on the K24 board so we could re-enable this info but this hasn't been proven yet.
   INFO_printf("Preamble detected on radio %u", radioNum);
#endif
}

/*!
 *  Process the Sync Detected interrupt
 *
 *  Arguments: radioNum - Radio to use
 *             offset   - Radio frequency offset as computed by the soft-demodulator algorithm
 *
 *  @return
 *
 *  @note
 *
 */
void RADIO_SyncDetected(uint8_t radioNum, float32_t offset)
{
   char floatStr[PRINT_FLOAT_SIZE];
   union si446x_cmd_reply_union Si446xCmd;

   // Save sync detect timestamp
#if ( DCU == 1 )
#if ( TEST_TDMA == 1 )
   RED_LED_ON();
#endif
   if ( radioNum == (uint8_t)RADIO_0 ) {
      radio[radioNum].syncTime = radio[RADIO_0].tentativeSyncTime;
   } else {
      radio[radioNum].syncTime = radio[RADIO_1].tentativeSyncTime;
   }
#else
   if ( radio[(uint8_t)radioNum].demodulator == 0 ) {
      // The following is done only when using hard-demod
      // The soft-demod equivalent is done in SD_FindSync
      radio[radioNum].syncTime       = radio[RADIO_0].tentativeSyncTime;
      radio[radioNum].syncTimeCYCCNT = radio[RADIO_0].intFIFOCYCCNT;
#if ( PHASE_DETECTION == 1 )
      PD_SyncDetected(radio[radioNum].syncTime, radio[radioNum].syncTimeCYCCNT);
#endif
   }
#endif
   // Reset some variables
   radio[radioNum].bytePos = 0; // Reset byte position
   radio[radioNum].validHeader = (bool)false; // Reset flag

   if ( radio[radioNum].demodulator == 0 ) {
      // Save AFC offset
      (void)si446x_get_modem_status(radioNum, 0xFF, &Si446xCmd); // Don't clear int.  get_int_status needs those.
      offset = (int16_t)Si446xCmd.GET_MODEM_STATUS.AFC_FREQ_OFFSET;

      // Save RSSI
      radio[radioNum].rssi[0] = RADIO_Get_CurrentRSSI(radioNum);
      radio[radioNum].rssiIndex = 1;
   } else {
      // Save AFC offset
      offset /= PHY_AFC_MIN_FREQ_STEP; // Soft demod offset is already in Hz but needs to be converted back in frequency step for rest of the code.

      // Save RSSI (unbuffered in PSLSNR call to RADIO_CaptureRSSI)
      radio[radioNum].rssiIndex = 0;
   }
   radio[radioNum].CWOffset  = (int16_t)offset;

   radio[radioNum].frequencyOffset = (int16_t)(offset*PHY_AFC_MIN_FREQ_STEP); // Convert to Hz.
   if (radio[radioNum].frequencyOffset > PHY_FREQUENCY_OFFSET_MAX) {
      radio[radioNum].frequencyOffset = PHY_FREQUENCY_OFFSET_MAX;
   } else if (radio[radioNum].frequencyOffset < PHY_FREQUENCY_OFFSET_MIN) {
      radio[radioNum].frequencyOffset = PHY_FREQUENCY_OFFSET_MIN;
   }

   INFO_printf("Sync detected on radio %u, RSSI = %sdBm, CW offset %d", radioNum, DBG_printFloat(floatStr, RSSI_RAW_TO_DBM(radio[radioNum].rssi[0]), 1), radio[radioNum].frequencyOffset);
   if ( radio[radioNum].demodulator == 0 ) {
      (void)Radio_Enable_RSSI_Jump(radioNum); // Enable this only after previous variables are set or captured.
   }
   RadioEvent_SyncDetect(radioNum, (int16_t) RSSI_RAW_TO_DBM(radio[radioNum].rssi[0])); // Increment sync detection counter
}

uint8_t RADIO_Get_CurrentRSSI(uint8_t radioNum)
{
   int8_t       FrontEndGain;
   int16_t      rssi;
   union si446x_cmd_reply_union Si446xCmd;

   (void)si446x_get_modem_status(radioNum, 0xFF, &Si446xCmd); // Don't clear int.  get_int_status needs those.
   rssi = Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;

   // Adjust RSSI to compensate for front end gain
   PHY_GetReq_t GetReq;
   GetReq.eAttribute = ePhyAttr_FrontEndGain;
#if 1 // TODO: RA6E1 Bob: should be able to remove this now that the file system and PHY task are running
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)&FrontEndGain);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#else
   FrontEndGain = 0;
#endif
   rssi += FrontEndGain;
   if ( rssi < MINIMUM_RSSI_VALUE ){
      rssi = MINIMUM_RSSI_VALUE;
   } else if ( rssi > 255 ) {
      rssi = 255;
   }

   return (uint8_t)rssi;
}

static void processRXRadioInt(uint8_t radioNum, struct si446x_reply_GET_INT_STATUS_map getIntStatus)
{
   uint32_t restart;
   struct   si446x_reply_GET_INT_STATUS_map FIFOStatus;
   union    si446x_cmd_reply_union Si446xCmd;

   if ( radio[radioNum].demodulator == 0 ) {
      FIFOStatus = getIntStatus; // Save all Interrupts

      /* Check for preamble detected */
      if (getIntStatus.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_PEND_PREAMBLE_DETECT_PEND_BIT)
      {
         RADIO_PreambleDetected(radioNum);
      }

      /* Check for Sync detected */
      if (getIntStatus.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_PEND_SYNC_DETECT_PEND_BIT)
      {
         RADIO_SyncDetected(radioNum, 0.0F); // AFC offset will be computed inside function when using hard decoder
      }

      // RX FIFO almost full
      // Read FIFO until the FIFO almost full bit is cleared or RX is over.
      // If we read some bytes from the FIFO but not enough to allow FIFO almost full to be cleared,
      // the radio won't send an IRQ until it detects a FIFO empty to FIFO almost full transition.
      // In other words, no more IRQ even thought FIFO almost full is set.
      restart = false;
      while ((FIFOStatus.PH_STATUS & SI446X_CMD_GET_INT_STATUS_REP_PH_STATUS_RX_FIFO_ALMOST_FULL_BIT) ||
             (FIFOStatus.PH_STATUS & SI446X_CMD_GET_INT_STATUS_REP_PH_STATUS_PACKET_RX_BIT)) {
         restart = Radio_RX_FIFO_Almost_Full(radioNum);

         if (restart) {
            break; // We restarted the radio. No need to look at interrupts anymore.
         } else {
            (void)si446x_get_int_status(radioNum, 0xFF, 0xFF, 0xFF, &Si446xCmd); // Get interrupt status but don't clear them
            FIFOStatus = Si446xCmd.GET_INT_STATUS;
         }
      }

      // Check for RSSI jump
      // NOTE: Ignore the RSSI jump interrupt if the radio was restarted.
      //       One reason for restart is that the message is completely received which means
      //       the tranmitter stoped transmitting. When that happened, the end of transmission caused a drop
      //       in RSSI which will be detected as a jump but we don't want that.
      //       Restart radio if header was not received yet or
      //                     if header is valid but we are missing more than the few last bytes
      if ((restart == false) && (getIntStatus.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_PEND_RSSI_JUMP_PEND_BIT)) {
         INFO_printf("RSSI jump detected on radio %u, position %u", radioNum, radio[radioNum].bytePos);
#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
         // The idea of this code was to try to salvage a message is the RSSI jump happend toward the end
         // but we decided to not do that for now.
         if ( (radio[radioNum].validHeader == (bool)false) ||
             ((radio[radioNum].validHeader == (bool)true) && (((int16_t)radio[radioNum].length-(int16_t)radio[radioNum].bytePos) > FIFO_FULL_THRESHOLD)))
         {
            vRadio_StartRX(radioNum, radio[radioNum].rx_channel);       // Restart radio.
            RadioEvent_RssiJump(radioNum); // Notify PHY of a RSSI jump event
            INFO_printf("Radio start from RadioEvent_Int because of RSSI jump %u", radioNum);
         }
#else
         // For now, always restart when a RSSI jump is detected
         vRadio_StartRX(radioNum, radio[radioNum].rx_channel);       // Restart radio.
         RadioEvent_RssiJump(radioNum); // Notify PHY of a RSSI jump event
         INFO_printf("Radio start from RadioEvent_Int because of RSSI jump %u", radioNum);
#endif
      }
   }
}

/*!
 *  Process radio interrupts
 *
 *  Arguments: radioNum - Radio to use
 *
 *  @return TRUE if interrupt should be reprocessed
 *
 *  @note
 *
 */

static void processRadioInt(uint8_t radioNum)
{
   struct si446x_reply_GET_INT_STATUS_map getIntStatus;
   union si446x_cmd_reply_union Si446xCmd;

   // Process configured radios only
   if (radio[radioNum].configured) {
      /* Read ITs, clear pending ones */
      (void)si446x_get_int_status_fast_clear_read(radioNum, &Si446xCmd);

      getIntStatus = Si446xCmd.GET_INT_STATUS; // Save all Interrupts
#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
      INFO_printf("INT %u %02X %02X %02X %02X %02X %02X %02X %02X", radioNum, getIntStatus.INT_PEND, getIntStatus.INT_STATUS, getIntStatus.PH_PEND, getIntStatus.PH_STATUS,
                     getIntStatus.MODEM_PEND, getIntStatus.MODEM_STATUS, getIntStatus.CHIP_PEND, getIntStatus.CHIP_STATUS);
#endif
      // Process rx radios with valid channel only
      if (radio[radioNum].rx_channel != PHY_INVALID_CHANNEL) {
         processRXRadioInt(radioNum, getIntStatus);
      }

      // TX almost empty
      if (getIntStatus.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_TX_FIFO_ALMOST_EMPTY_PEND_BIT) {
         // Refill FIFO if needed
         if (radio[radioNum].bytePos < radio[radioNum].length) {
            uint8_t bytesLeft = (uint8_t)(radio[radioNum].length - radio[radioNum].bytePos);

            bytesLeft = (uint8_t)min(bytesLeft, FIFO_ALMOST_EMPTY_THRESHOLD);
            si446x_write_tx_fifo(radioNum, bytesLeft, &radio[radioNum].buf.TxBuffer.Payload[radio[radioNum].bytePos] );
            radio[radioNum].bytePos += bytesLeft;
         }
      }

      // Packet fully transmited
      if (getIntStatus.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT)
      {
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
         LED_off(RED_LED);
#endif
         INFO_printf("Sent");

         // Check if we need to stay in TX mode or switch back to RX mode
         if ((currentTxMode != eRADIO_MODE_PN9) && (currentTxMode != eRADIO_MODE_CW)) {
#if ( DCU == 1 )
            PHY_GetReq_t GetReq;
            PHY_STATE_e  state;

            // Retrieve PHY state
            GetReq.eAttribute = ePhyAttr_State;
#if 1 //TODO: RA6E1 Melvin: should be able to remove this now that file system and PHY task are running
            (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&state); //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#endif

            if (state == ePHY_STATE_READY_TX) {
               // Keep FEM in TX mode and shutdown PA to save power until next transmission
               RDO_PA_EN_OFF();
            } else
#endif
            {
               // Set PA to RX
               setPA(eRADIO_PA_MODE_RX);
            }

            // Restart the radio in RX mode only if RX channels is valid
            if(PHY_IsRXChannelValid(radio[radioNum].rx_channel)) {
               INFO_printf("Radio start from RadioEvent_Int on radio %u", radioNum);
               vRadio_StartRX(radioNum, radio[radioNum].rx_channel);       // Restart radio.
            }
            // Go to ready state if RX is not used on this radio to keep the 30MHz output running
            else {
               (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);
            }
         }

         // Signal the TX DONE Event
#if ( RTOS_SELECTION == MQX_RTOS )
         RADIO_Event_Set(eRADIO_TX_DONE, radioNum); // Signal the RX DATA Event to PHY
#elif ( RTOS_SELECTION == FREE_RTOS )
         RADIO_Event_Set(eRADIO_TX_DONE, radioNum, (bool)false); // Signal the RX DATA Event to PHY
#endif
      }
   }
}

/*!
 *  Check if Packet received IT flag is pending.
 *
 *  @return   TRUE - Packet successfully received / FALSE - No packet pending.
 *
 *  @note
 *
 */
void RadioEvent_Int(uint8_t radioNum)
{
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   uint32_t keepServicing;

   // Process a radio interrupt until all serviced
   do {
      processRadioInt(radioNum);
      keepServicing = 0;
	  // Check if IRQ is still asserted
      switch (radioNum) {
         case RADIO_0: keepServicing = RDO_0_IRQ(); break;
         case RADIO_1: keepServicing = RDO_1_IRQ(); break;
         case RADIO_2: keepServicing = RDO_2_IRQ(); break;
         case RADIO_3: keepServicing = RDO_3_IRQ(); break;
         case RADIO_4: keepServicing = RDO_4_IRQ(); break;
         case RADIO_5: keepServicing = RDO_5_IRQ(); break;
         case RADIO_6: keepServicing = RDO_6_IRQ(); break;
         case RADIO_7: keepServicing = RDO_7_IRQ(); break;
         case RADIO_8: keepServicing = RDO_8_IRQ(); break;
         default:
            break;
      }

   } while ( keepServicing == 0 );
#else
   // Process interrupts until all serviced
   if (radioNum == (uint8_t)RADIO_0) {
      // Process radio 0
      while (RDO_0_IRQ() == 0) // Samwise or Frodo radio 0 IRQ line
      {
         processRadioInt(radioNum);
      }
   }
#if ( DCU == 1 )
   else {
      // Process all RX radios while interrupt is active or we need to reprocess
      while (RDO_RX_IRQ() == 0)
      {
         for (radioNum=(uint8_t)RADIO_1; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
            processRadioInt(radioNum);
            // Short cut processing if no more interrupts
            if (RDO_RX_IRQ() != 0)
               break;
         }
      }
   }
#endif
#endif
}

/******************************************************************************

   Function Name: updateTCXO

   Purpose: This function updates the radio TCXO frequency to adjust for aging

   Arguments: radioNum - Radio to use

   Returns:

   Notes:

******************************************************************************/
static void updateTCXO ( uint8_t radioNum )
{
   uint32_t newTCXO;
   uint32_t currentTCXO;
   uint32_t freq;
   union si446x_cmd_reply_union Si446xCmd;

   uint32_t primask = __get_PRIMASK();
#if ( RTOS_SELECTION == MQX_RTOS ) //TODO: add the below line once the disable interrupt is added
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
#endif
   freq = radio[radioNum].TCXOFreq;
   __set_PRIMASK(primask); // Restore interrupts

   // Sanity check on TCXO frequency
   // Make sure radio TCXO is within expected values (30MHz +/-25ppm)
   if ( (freq > RADIO_TCXO_MIN) && (freq < RADIO_TCXO_MAX) ) {
      // Get current TCXO setting
      (void)si446x_get_property( radioNum,
                                 SI446X_PROP_GRP_ID_MODEM,                 // Property group.
                                 4,                                        // Number of property to get.
                                 SI446X_PROP_GRP_INDEX_MODEM_TX_NCO_MODE,  // Start sub-property address.
                                 &Si446xCmd);

      currentTCXO = (((uint32_t)Si446xCmd.GET_PROPERTY.DATA[0] << 24)|
                     ((uint32_t)Si446xCmd.GET_PROPERTY.DATA[1] << 16)|
                     ((uint32_t)Si446xCmd.GET_PROPERTY.DATA[2] <<  8)|
                     ((uint32_t)Si446xCmd.GET_PROPERTY.DATA[3]      ));

      // Update TXCO frequency if needed
      newTCXO = ((Si446xCmd.GET_PROPERTY.DATA[0] & 0xC) << 24) | freq;

      if ( newTCXO != currentTCXO) {
         (void)si446x_set_property( radioNum,
                                    SI446X_PROP_GRP_ID_MODEM,                 // Property group.
                                    4,                                        // Number of property to get.
                                    SI446X_PROP_GRP_INDEX_MODEM_TX_NCO_MODE,  // Start sub-property address.
                                    (uint8_t)(newTCXO >> 24),
                                    (uint8_t)(newTCXO >> 16),
                                    (uint8_t)(newTCXO >>  8),
                                    (uint8_t)(newTCXO      ));
      }
   }
}


/*!
 *  Set Radio to RX mode, fixed packet length.
 *
 *  @param chan Freq. Channel
 *
 *  @note
 *
 */
void vRadio_StartRX(uint8_t radioNum, uint16_t chan)
{
   OS_TICK_Struct CurrentTime;
   PHY_GetReq_t   GetReq;
   uint8_t        CurrentRssiJumpThreshold;
#if ( RTOS_SELECTION == MQX_RTOS ) // The overflow flag is only needed by the MQX _time_diff_hours function
   bool           overflow;
#endif
   union          si446x_cmd_reply_union Si446xCmd;

   if (!radio[radioNum].configured) {
      ERR_printf("ERROR - vRadio_StartRx: radio %u is disabled",radioNum);
      return;
   }

   if(!PHY_IsRXChannelValid(chan)) {
      ERR_printf("ERROR - vRadio_StartRx: radio %u channel is invalid",radioNum);
      return;
   }

#if (HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A)
   if (radioNum == (uint8_t)RADIO_0) {
      // No RX RADIO 0 on some hardware
      return;
   }
#endif

   // Leave RX state
   // NOTE: It should not be necessary to explicitly leave the RX state but it was observed that
   // sometimes, even if the radio is restarted, it stays in ready mode.
   // This seems to happend when the radio just received the last bit and the start was issued.
   // It looks like there might be a race condition inside the radio between those 2 events.
   // Stopping the radio first fixes this problem.
   if ( si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY) ) {
      return;
   }

   // Set PA to RX
   //setPA(eRADIO_PA_MODE_RX);

   // Disable RSSI jump first thing to avoid detection
   if ( Radio_Disable_RSSI_Jump(radioNum) ) {
      return;
   }

   INFO_printf("Start RX radio %hu on channel = %hu (%u Hz)", radioNum, chan, CHANNEL_TO_FREQ(chan));

#if ( RTOS_SELECTION == MQX_RTOS ) //TODO: RA6E1 Melvin: timer module
   // Get current time
   OS_TICK_Get_CurrentElapsedTicks(&CurrentTime);

   // TODO: TICKS[] structure variable usage handling
   // Recalibrate every 4 hours or after power up
   // MKD 4-24-14 This should be done when temperature changes by 30 degree C but it takes a long time to retrieve the temperatures so we recalibrate every 4 hours instead.
   if (( _time_diff_hours(&CurrentTime, &radio[radioNum].CallibrationTime, &overflow) > 3) ||
         ( (radio[radioNum].CallibrationTime.TICKS[0] == 0) && (radio[radioNum].CallibrationTime.TICKS[1] == 0) )) {
      INFO_printf("Radio calibration in progress");

      if ( Radio_Calibrate(radioNum) ) {
         return;
      }

      INFO_printf("Radio calibration done");

      // Set next refresh in 4 hours
      radio[radioNum].CallibrationTime = CurrentTime;
    }

#endif
   // Update RSSI jump threshold if it changed
   GetReq.eAttribute = ePhyAttr_RssiJumpThreshold;
#if 1 // TODO: RA6E1 Melvin: should be able to remove this now that file system and PHY task are running
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&CurrentRssiJumpThreshold);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#endif
   if (radio[radioNum].RssiJumpThreshold != CurrentRssiJumpThreshold) {
      radio[radioNum].RssiJumpThreshold = CurrentRssiJumpThreshold;
      // Configure RSSI jump threshold
      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_MODEM,                                 // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_MODEM_RSSI_JUMP_THRESH,             // Start sub-property address.
                                radio[radioNum].RssiJumpThreshold) ) {                    // RSSI jump threshold
         return;
      }
   }

   // Update TCXO frequency
   updateTCXO( radioNum );

   if ( radio[radioNum].demodulator != 0 ) {
      // Code needed for soft-modem
      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_MODEM,                                 // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_MODEM_AFC_GAIN,                     // Start sub-property address.
                                0x80) ) {                                                 // Disable adaptive RX bandwith
         return;
      }

      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_MODEM,                                 // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_MODEM_AFC_MISC,                     // Start sub-property address.
                                0xA0) ) {                                                 // Disable AFC correction value feedback to the PLL
         return;
      }

      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_MODEM,                                 // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_MODEM_ANT_DIV_MODE,                 // Start sub-property address.
                                0) ) {
         return;
      }

      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_PREAMBLE,                              // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_PREAMBLE_CONFIG_STD_1,              // Start sub-property address.
                                0) ) {                                                    // Disable rx threshold
         return;
      }

      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_PREAMBLE,                              // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_PREAMBLE_CONFIG_STD_2,              // Start sub-property address.
                                0) ) {                                                    // Disable preamble timeout
         return;
      }

      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_MODEM,                                 // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_MODEM_MDM_CTRL,                     // Start sub-property address.
                                1) ) {                                                    // Enable phase output
         return;
      }
   }

   // Set RX frequency
   SetFreq(radioNum, CHANNEL_TO_FREQ(chan));

   /* Reset FIFO */
   if ( si446x_fifo_info(radioNum, SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT, &Si446xCmd) ) {
      return;
   }

   // Clear interrupts
   if ( si446x_get_int_status_fast_clear(radioNum) ) {
      return;
   }

   // Enable RX interrupts
   if ( Radio_RX_int_enable(radioNum) ) {
      return;
   }

   /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
   if ( radio[radioNum].demodulator == 0 ) {
      (void)si446x_start_rx(radioNum, 0, 0u, PHY_MAX_FRAME,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_READY,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_RX );
   } else {
      // Code needed to help debug soft-modem
      (void)si446x_start_rx(radioNum, 0, 0u, 0,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN);
   }
}


/*!
 * Initialize the radio
 *
 * pCallback - the function that will be called when an event is triggered by the radio.
 *
 */
static bool Init(RadioEvent_Fxn pCallbackFxn )
{
   // Set the event handler
   pEventHandler_Fxn = pCallbackFxn;

   // Initialize the radios
   vRadio_Init(eRADIO_MODE_NORMAL);

   return (bool)true;   // SUCCESS
}

/*!
 * Set power - Set power level based on temperature and frequency.
 *
 *  @param frequency    - TX frequency
 *  @param powerOutput  - TX power output in dBm
 *
 */
#if ( DCU == 1 )
void RADIO_SetPower( uint32_t frequency, float powerOutput)
{
#if !( (DCU == 1) && (VSWR_MEASUREMENT == 1) )  /* Legacy 9975T hardware   */
   double  computedPower; // Computed power level
   double  val;           // Value to pass to log
   int16_t radioTemp;     // Radio temperature
   int16_t clippedRadioTemp; // Radio temperature
   int32_t PaTemp;        // PA temperature

   (void)RADIO_Temperature_Get( (uint8_t)RADIO_0, &radioTemp );
   clippedRadioTemp = radioTemp;

   // Clip temperature if needed.
   if ( radioTemp < RADIO_TEMPERATURE_MIN ) {
      clippedRadioTemp = RADIO_TEMPERATURE_MIN;
   } else if ( radioTemp > RADIO_TEMPERATURE_MAX ) {
      clippedRadioTemp = RADIO_TEMPERATURE_MAX;
   }
   PaTemp = (int32_t)ADC_Get_PA_Temperature( TEMP_IN_DEG_C );

   val = 1.21574-(0.0352986512723369*powerOutput);

   // Sanitize input
   if ( val < 0.01 ) {
      val = 0.01;
   }

   // Compute radio PA level
   computedPower = -4.4*log(val)*(0.7915131 - 0.774956*(1 - exp(0.01309945*clippedRadioTemp)))*( 0.000000003571429*frequency - 0.6178571);

   // Sanitize input
   if ( computedPower < 0 ) {
      computedPower = 0;
   } else if ( computedPower > RADIO_PA_LEVEL_MAX ) {
      computedPower = RADIO_PA_LEVEL_MAX;
   }

   RADIO_Power_Level_Set( (uint8_t)computedPower );

   INFO_printf("PA settings: radio temp = %dC, used temp = %dC, PA temp = %dC, computed level(*100) %u, programmed level %u",
                  radioTemp, clippedRadioTemp, PaTemp, (uint32_t)(computedPower*100), (uint8_t)computedPower);
#else                                  /* 9985T hardware and FEM  */
   (void)frequency;
   PHY_GetReq_t   GetReq;
   uint8_t PowerSetting;

   GetReq.eAttribute = ePhyAttr_PowerSetting;
#if 1 //TODO: RA6E1 Melvin: should be able to remove this now that file system and PHY task are running
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&PowerSetting); //lint !e826   Suspicious pointer-to-pointer conversion
#endif
   RADIO_Power_Level_Set( PowerSetting );       /* Radio chip internal gain setting. Default 20 for 9975T and 60 for 9985T.   */
   (void)BSP_PaDAC_Set_dBm_Out( powerOutput );
#endif
}
#endif
/*!
 * Transmit data - Prepare & transmit a packet.
 *
 *  @param radio_num   - index of the radio (0, or 0-8 for frodo)
 *  @param chan        - Channel to use (0-3200)
 *  @param mode        - TX mode i.e. 4GFSK, 2GFSK
 *  @param detection   - Preamble and sync detection to use
 *  @param *payload    - pointer to the data
 *  @param RxTime      - future time that we want SYNC to be detected on EPs
 *  @param TxPriority  - TX priority
 *  @param power       - TX power in dBm
 *  @param payload_len - number of bytes to transmit
 *
 *  NOTE: When changing this function, make sure that time sync (mac_time set) can still be sent in time.
 *        Adjust TIME_SYNC_FUTURE accordingly
 */
static PHY_DATA_STATUS_e SendData(uint8_t radioNum, uint16_t chan, PHY_MODE_e mode, PHY_DETECTION_e detection, const void *payload, uint32_t RxTime, PHY_TX_CONSTRAIN_e TxPriority, float32_t
                                    power, uint16_t payload_len)
{
   PHY_DATA_STATUS_e eStatus = ePHY_DATA_SUCCESS;
#if ( EP == 1 )
   (void)power;      // Shut up lint since not used on EPs
#endif
   uint16_t remainingBytes = payload_len;
   uint8_t  FIFOspace;
   uint8_t  *ptr = (uint8_t*)payload;
   union    si446x_cmd_reply_union Si446xCmd;
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   PHY_GetReq_t   getReq;
#endif

#if 0 // Not RA6E1.  This was already removed in the K24 baseline code
   // Code needed to help debug soft-modem
   uint32_t i;
   uint32_t end=1;

   switch (((uint8_t*)payload)[25]) {
      case 0: memset((void*)payload, 0xFF, payload_len);
      for (i=1; i<payload_len; i+= 2) {
         ((uint8_t*)payload)[i] = 0;
      }
      break;

      case 1: memset((void*)payload, 0xF0, payload_len);
      break;

      case 2: memset((void*)payload, 0xCC, payload_len);
      break;

      case 3:
      for (i=0; i<payload_len; i++) {
         ((uint8_t*)payload)[i] = rand();
      }
      break;

      case 4:
      for (i=0; i<payload_len; i++) {
         ((uint8_t*)payload)[i] = rand();
      }
      end = 100;
      break;
   }

   // Last byte always 0 to help with soft radio decoding
   if ( (((uint8_t*)payload)[25]) != 0xFF)
   {
      ((uint8_t*)payload)[payload_len++] = 0;
   }
#endif
   if (!radio[radioNum].configured) {
      ERR_printf("ERROR - SendData: radio %u is disabled",radioNum);
      return ePHY_DATA_TRANSMISSION_FAILED;
   }

   if ( RADIO_Is_RX(radioNum) ) {
      RadioEvent_RxAbort( radioNum );
   }

#if ( EP == 1 )
   // Disable RSSI jump first thing to avoid detection
   if ( Radio_Disable_RSSI_Jump(radioNum) ) {
      return ePHY_DATA_TRANSMISSION_FAILED;
   }
#endif

   // If in soft demod mode, switch off the phase output mode during transmission
   if ( radio[radioNum].demodulator != 0 ) {
      // Switching to SLEEP state rather than READY state appeared to solve the erroneous data
      // transmissions for the soft demod radio. It is also necessary to turn off the phase output
      // first and then make the transition.
      if ( si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_MODEM,                                 // Property group.
                                1,                                                        // Number of property to be set.
                                SI446X_PROP_GRP_INDEX_MODEM_MDM_CTRL,                     // Start sub-property address.
                                0) ) {
         return ePHY_DATA_TRANSMISSION_FAILED;
      }

      if ( si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP) ) {
         return ePHY_DATA_TRANSMISSION_FAILED;
      }
   } else {
      // On samwise: Leave RX state since radio 0 is in RX state when entering this function
      // On Frodo: Leave the sleep mode and transition to ready since this will be needed.
      //           we don't realy need to do that on Frodo but this speeds up the process in case we need to transmit at a specific time (i.e TxPriority == ePHY_TX_CONSTRAIN_EXACT).
      if ( si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY) ) {
         return ePHY_DATA_TRANSMISSION_FAILED;
      }
   }

   // Update TCXO frequency
   updateTCXO( radioNum );

   // Set TX frequency
   SetFreq(radioNum, CHANNEL_TO_FREQ(chan));

#if ( DCU == 1)
   // Set power compensating for temperature and frequency
   RADIO_SetPower( CHANNEL_TO_FREQ(chan), power);
#endif

   // Clear interrupts
   if ( si446x_get_int_status_fast_clear(radioNum) ) {
      return ePHY_DATA_TRANSMISSION_FAILED;
   }

   // Reset the FIFO
   if ( si446x_fifo_info(radioNum, SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT, &Si446xCmd) ) {
      return ePHY_DATA_TRANSMISSION_FAILED;
   }

   // Enable TX interrupts
   if ( Radio_TX_int_enable(radioNum) ) {
      return ePHY_DATA_TRANSMISSION_FAILED;
   }

   // Configure mode
   (void)RADIO_RxModeConfig(radioNum, mode, (bool)false);

   // Configure detection format
   // This is needed to switch betwen normal STAR messages and fast messages
   (void)RADIO_RxDetectionConfig(radioNum, detection, (bool)false);

   // Fill the TX FIFO with data
   radio[radioNum].length = payload_len;
   FIFOspace = (uint8_t)min(remainingBytes, FIFO_SIZE);
   si446x_write_tx_fifo(radioNum, FIFOspace, ptr);
   radio[radioNum].bytePos = FIFOspace;

   // Set PA to TX
   setPA(eRADIO_PA_MODE_TX);

   // Code needed to help debug soft-modem
   //INFO_printf("3");
   //OS_TASK_Sleep(ONE_SEC);
   //INFO_printf("2");
   //OS_TASK_Sleep(ONE_SEC);
   //INFO_printf("1");
   //OS_TASK_Sleep(ONE_SEC);

#if ( DCU == 1 ) || ( TEST_TDMA == 1 )
#if ( DCU == 1 )
   if (VER_getDCUVersion() != eDCU2) {
#else
   if (1) {
#endif
      // Set PHY to be deterministic
      if (TxPriority == ePHY_TX_CONSTRAIN_EXACT) {
         // Enable 129 byte FIFO and guaranteed sequencer
         if ( si446x_set_property( radioNum,
                                   SI446X_PROP_GRP_ID_GLOBAL,                           // Property group.
                                   1,                                                   // Number of property to be set.
                                   SI446X_PROP_GRP_INDEX_GLOBAL_CONFIG,                 // Start sub-property address.
                                   SI446X_PROP_GLOBAL_CONFIG_SEQUENCER_MODE_FALSE_BIT | // Sequencer mode set to guaranteed
                                   SI446X_PROP_GLOBAL_CONFIG_FIFO_MODE_BIT) ) {         // TX/RX FIFO are sharing with 129-byte size buffer.
            return ePHY_DATA_TRANSMISSION_FAILED;
         }

         // Compute how long the preamble and sync will take over the air.
         uint64_t nbsym;
         uint64_t symtime = 0;
         if ( si446x_get_property( radioNum,
                                   SI446X_PROP_GRP_ID_PREAMBLE,              // Property group.
                                   1,                                        // Number of property to be set.
                                   SI446X_PROP_GRP_INDEX_PREAMBLE_TX_LENGTH, // Start sub-property address.
                                   &Si446xCmd) ) {
            return ePHY_DATA_TRANSMISSION_FAILED;
         }

         nbsym = Si446xCmd.GET_PROPERTY.DATA[0]*8U; // length in symbols

         if ( si446x_get_property( radioNum,
                                   SI446X_PROP_GRP_ID_SYNC,           // Property group.
                                   1,                                 // Number of property to be set.
                                   SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                                   &Si446xCmd) ) {
            return ePHY_DATA_TRANSMISSION_FAILED;
         }

         nbsym += (Si446xCmd.GET_PROPERTY.DATA[0]+1U)*8U; // length in symbols

         // Convert symbols length into symbol time in nano sec
         uint32_t cpuFreq;
         (void)TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL );
         nbsym *= (uint64_t)cpuFreq;

         switch (mode) {
            case ePHY_MODE_0: /*!< 4-GFSK,                      , 4800 baud */
            case ePHY_MODE_1: /*!< 4-GFSK,                      , 4800 baud */
               symtime = nbsym/4800;
               break;

            case ePHY_MODE_2: /*!< 2-FSK,   (63,59) Reed-Solomon, 7200 baud */
               symtime = nbsym/7200;
               break;

            case ePHY_MODE_3: /*!< 2-FSK,                       , 2400 baud */
               symtime = nbsym/2400;
               break;

            case ePHY_MODE_LAST: // Shut up lint

            default:
               break;
         }

         // Make sure TX is in ready state because this state is expected for proper TX timing.
         do {
            if ( si446x_request_device_state(radioNum, &Si446xCmd) ) {
               return ePHY_DATA_TRANSMISSION_FAILED;
            }
         }
         while (Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE != SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_READY);

         // Remove preamble, sync and the time it takes to process START_TX and some other delays
         RxTime -= (uint32_t)(START_TX_FUDGE_FACTOR + symtime);

         // Sanity check on RxTime
         if ( (int32_t)(RxTime - DWT_CYCCNT) < 0 ) {
            // Send time is in the past
            INFO_printf("ERROR: Send time is in the past");
            RxTime = 0;
            eStatus = ePHY_DATA_TRANSMISSION_FAILED;
         } else if ( (int32_t)(RxTime - DWT_CYCCNT) > FUTURE_TX_TIME ) {
            // Send time is too far in future.
            INFO_printf("ERROR: Send time is too far in the future");
            RxTime = 0;
            eStatus = ePHY_DATA_TRANSMISSION_FAILED;
         } else {
            // Burn time until we are within 10 msec of firing while being nice to other tasks
            while ( (int32_t)(RxTime - DWT_CYCCNT) > FRACSEC_TEN_MSEC ) {
               // Burn time
               OS_TASK_Sleep(1); // Be nice to other tasks
            };

            // Burn time until we are within 0.5 msec of firing. Only higher priority tasks and interrupts will be able to preempt
            while ( (int32_t)(RxTime - DWT_CYCCNT) > FRACSEC_HALF_MSEC )
               // Burn time
               ;
         }
      } else {
         // Enable 129 byte FIFO and fast sequencer
         RxTime = 0; // Transmit now

         if ( si446x_set_property( radioNum,
                                   SI446X_PROP_GRP_ID_GLOBAL,                          // Property group.
                                   1,                                                  // Number of property to be set.
                                   SI446X_PROP_GRP_INDEX_GLOBAL_CONFIG,                // Start sub-property address.
                                   SI446X_PROP_GLOBAL_CONFIG_SEQUENCER_MODE_TRUE_BIT | // Sequencer mode set to fast (default)
                                   SI446X_PROP_GLOBAL_CONFIG_FIFO_MODE_BIT) ) {           // TX/RX FIFO are sharing with 129-byte size buffer.
            return ePHY_DATA_TRANSMISSION_FAILED;
         }
      }
   }
   else //not DCU2+
#endif
   {
      RxTime = 0; // Always Transmit now on EP (or DCU2)
   }

   // Transmit only if no error
   if ( eStatus == ePHY_DATA_SUCCESS ) //lint !e774 Always evalutate to true for EP
   {
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
      uint16_t     localVSWR;
      uint16_t     notifySetPoint;
      uint16_t     shutDownSetPoint;
      uint16_t     lowPowerSetPoint;
      EventData_s  progEvent;     /* Event info */

      getReq.eAttribute = ePhyAttr_fngVswrNotificationSet;
#if 1 //TODO: RA6E1 Melvin: should be able to remove this now that file system and PHY task are running
      (void)PHY_Attribute_Get( &getReq, (PHY_ATTRIBUTES_u*)(void *)&notifySetPoint);   //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#endif

      getReq.eAttribute = ePhyAttr_fngVswrShutdownSet;
#if 1 //TODO: RA6E1 Melvin: should be able to remove this now that file system and PHY task are running
      (void)PHY_Attribute_Get( &getReq, (PHY_ATTRIBUTES_u*)(void *)&shutDownSetPoint); //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#endif

      getReq.eAttribute = ePhyAttr_fngFowardPowerLowSet;
      (void)PHY_Attribute_Get( &getReq, (PHY_ATTRIBUTES_u*)(void *)&lowPowerSetPoint); //lint !e826 !e433  Suspicious pointer-to-pointer conversion
#endif

      if ( (TxPriority == ePHY_TX_CONSTRAIN_EXACT) && RxTime ) { /*lint !e774 */
         OS_INT_disable( ); // Disable all interrupts. This section is time critical when we try to be deterministic.
      }
      // Start sending packet, channel 0, START immediately, revert to Ready when done
      (void)si446x_start_tx(radioNum, 0,/*lint !e456 */
               SI446X_CMD_START_TX_ARG_CONDITION_TXCOMPLETE_STATE_ENUM_READY << SI446X_CMD_START_TX_ARG_CONDITION_TXCOMPLETE_STATE_LSB,
               payload_len, RxTime);

      if ( (TxPriority == ePHY_TX_CONSTRAIN_EXACT) && RxTime ) { /*lint !e774 */
         OS_INT_enable( ); // Enable interrupts.
      }

#if (  VSWR_MEASUREMENT == 1)
      // Wait a short time for the TX to begin and for the VSWR circuitry to energize/stabilize.
      OS_TASK_Sleep(5);
      cachedVSWR = ADC_Get_VSWR();
#endif

#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
      FwdPwrSamples [sampleNumber % VSWR_AVERAGE_COUNT]  = ADC_Get_FEM_V_Forward();    /* Saved as volts */
      ReflPwrSamples[sampleNumber % VSWR_AVERAGE_COUNT]  = ADC_Get_FEM_V_Reflected();  /* Saved as volts */
#if 1
      char floatStr[PRINT_FLOAT_SIZE];

      DBG_logPrintf( 'I', "fwdPower  : %sV"     , DBG_printFloat(floatStr, FwdPwrSamples[sampleNumber % VSWR_AVERAGE_COUNT], 3 ) );
      DBG_logPrintf( 'I', "fwdPower  : %sdBm"   , DBG_printFloat(floatStr, BSP_VFWD_to_DBM( FwdPwrSamples[sampleNumber % VSWR_AVERAGE_COUNT] ), 3 ) );
      DBG_logPrintf( 'I', "refPower  : %sV"     , DBG_printFloat(floatStr, ReflPwrSamples[sampleNumber % VSWR_AVERAGE_COUNT], 3 ) );
      DBG_logPrintf( 'I', "cachedVSWR: %s:1"    , DBG_printFloat(floatStr, cachedVSWR, 2 ) );
#endif
      sampleNumber++;
      if ( sampleNumber == 0 )   /* Check for wrap of uint32_t!   */
      {
         sampleNumber = VSWR_AVERAGE_COUNT;  /* Array is still full. */
      }
      cachedForwardVolts   = computeAverage( FwdPwrSamples,    sampleNumber < VSWR_AVERAGE_COUNT  ? sampleNumber : VSWR_AVERAGE_COUNT );
#if ( UNIT_TEST != 0 )
      cachedForwardVolts *= TxPwrMult;
#endif
      cachedReflectedVolts = computeAverage( ReflPwrSamples,   sampleNumber < VSWR_AVERAGE_COUNT  ? sampleNumber : VSWR_AVERAGE_COUNT );

      /* Compute the current VSWR, once the forward/reflected arrays are full,  based on the averages of the forward/reflected power readings.   */
      if ( sampleNumber >= VSWR_AVERAGE_COUNT ) /* Array is full. */
      {
         cachedVSWR  = ADC_ComputeVSWR( cachedForwardVolts, cachedReflectedVolts );
         localVSWR   = (uint16_t)( cachedVSWR * 10.0F );
#if ( UNIT_TEST != 0 )
         localVSWR *= VSWRmult;
#endif

         /* Now check whether the VSWR has exceeded the reporting and/or shutdown limits  */

#if 1
         /* All values are scaled up, so they can be compared directly   */
         if ( localVSWR > shutDownSetPoint ) /* Need to shut down the transmitter   */
         {
            ERR_printf( "VSWR (%hhu.%1hhu) exceeds shutdown limit. Setting stack to Mute mode.", (uint8_t)(localVSWR / 10.0f), (uint8_t)((uint16_t)localVSWR % 10U) );
            (void)SM_StartRequest(eSM_START_MUTE, NULL);
            phyVSWRtimerStart(); /* Don't attempt another transmission until the prescribed time out. */

            if ( !VSWRholdOff && ( VSWRlimitExceededCnt < VSWR_LIMIT_COUNT ) && ( ++VSWRlimitExceededCnt == VSWR_LIMIT_COUNT ) )
            {
               VSWRholdOff = (bool)true;
               ERR_printf( "logging collectorCommunicationRadioFailed" );
               progEvent.markSent = (bool)false;
               progEvent.eventId = (uint16_t)collectorCommunicationRadioFailed;
               progEvent.eventKeyValuePairsCount = 0;
               progEvent.eventKeyValueSize = 0;
               (void)EVL_LogEvent( 180U, &progEvent, NULL, TIMESTAMP_NOT_PROVIDED, NULL );
            }
         }
         else
         {
            if ( VSWRlimitExceededCnt != 0 )
            {
               VSWRlimitExceededCnt--;
            }

            if ( localVSWR > notifySetPoint )         /* Need to send an alarm   */
            {
               ERR_printf( "VSWR (%hhu.%1hhu) exceeds shutdown warning. Setting stack to Mute mode.", (uint8_t)(localVSWR / 10.0f), (uint8_t)((uint16_t)localVSWR % 10U) );
               (void)SM_StartRequest(eSM_START_MUTE, NULL);
               phyVSWRtimerStart(); /* Don't attempt another transmission until the prescribed time out. */

               if ( ( VSWRwarningExceededCnt < VSWR_LIMIT_COUNT ) && ( ++VSWRwarningExceededCnt == VSWR_LIMIT_COUNT ) )
               {
                  ERR_printf( "logging collectorCommunicationRadioMismatched" );
                  progEvent.markSent = (bool)false;
                  progEvent.eventId = (uint16_t)collectorCommunicationRadioMismatched;
                  progEvent.eventKeyValuePairsCount = 0;
                  progEvent.eventKeyValueSize = 0;
                  (void)EVL_LogEvent( 180U, &progEvent, NULL, TIMESTAMP_NOT_PROVIDED, NULL );
               }
            }
            else  /* localVSWR is below the notify limit. Check for need to restore transmitter and/or send all clear alarm. */
            {
               if ( ( VSWRwarningExceededCnt != 0 ) && ( --VSWRwarningExceededCnt == 0 ) )
               {
                  ERR_printf( "logging collectorCommunicationRadioLossRestored" );
                  progEvent.markSent = (bool)false;
                  progEvent.eventId = (uint16_t)collectorCommunicationRadioLossRestored;
                  progEvent.eventKeyValuePairsCount = 0;
                  progEvent.eventKeyValueSize = 0;
                  (void)EVL_LogEvent( 180U, &progEvent, NULL, TIMESTAMP_NOT_PROVIDED, NULL );

                  ERR_printf( "VSWR (%hhu.%1hhu) below shutdown limit. Setting stack to normal mode.", (uint8_t)( localVSWR / 10.0f), (uint8_t)((uint16_t)localVSWR % 10U) );
                  (void)SM_StartRequest(eSM_START_STANDARD, NULL);
                  VSWRholdOff = (bool)false;
               }
            }
         }
         /* Check for forward power dropping below set point   */
         if ( ( (uint32_t)BSP_VFWD_to_DBM( cachedForwardVolts ) * 10U ) < lowPowerSetPoint )
         {
            if ( ( TXPowerLossCounter < TX_PWR_LOSS_COUNT ) && ( ++TXPowerLossCounter == TX_PWR_LOSS_COUNT ) )
            {
               ERR_printf( "logging collectorCommunicationRadioLossDetected" );
               progEvent.markSent = (bool)false;
               progEvent.eventId = (uint16_t)collectorCommunicationRadioLossDetected;
               progEvent.eventKeyValuePairsCount = 0;
               progEvent.eventKeyValueSize = 0;
               (void)EVL_LogEvent( 180U, &progEvent, NULL, TIMESTAMP_NOT_PROVIDED, NULL );
            }
         }
         else
         {
            if ( ( TXPowerLossCounter != 0 ) && ( --TXPowerLossCounter == 0 ) )
            {
               ERR_printf( "logging CollectorCommunicationRadioRestored" );
               progEvent.markSent = (bool)false;
               progEvent.eventId = (uint16_t)CollectorCommunicationRadioRestored;
               progEvent.eventKeyValuePairsCount = 0;
               progEvent.eventKeyValueSize = 0;
               (void)EVL_LogEvent( 180U, &progEvent, NULL, TIMESTAMP_NOT_PROVIDED, NULL );
            }
         }
#endif
      }
#endif

      INFO_printf("TX Channel = %hu (%u Hz)", chan, CHANNEL_TO_FREQ(chan));/*lint !e456 */

      // Is this SRFN
      if ( mode == ePHY_MODE_1) {
         INFO_printf(   "Sending SRFN: Ver %u, Mode %u, Mode Parameters %u, Length %u, MSG length %u",
                        HeaderVersion_Get(payload),
                        HeaderMode_Get(payload),
                        HeaderModeParameters_Get(payload),
                        HeaderLength_Get(radio[radioNum].framing, payload),
                        payload_len);
      } else {
         INFO_printf("Sending STAR: MSG length %u", payload_len);
      }

      // Print bytes
      INFO_printHex("", payload, payload_len );
   } else {
      ERR_printf("ERROR - SendData: Failed %u", eStatus);
   }

   return eStatus;/*lint !e456 !e454*/
}

/*!
 * Read the received data from the radio
 *
 *  @param radio_num - index of the radio (0, or 0-8 for frodo)
 *
 *  @return RADIO_FRAME_t - Received buffer.
 *
 */
static RX_FRAME_t *ReadData(uint8_t radioNum)
{
   return (&radio[radioNum].buf.RxBuffer);
}

/***********************************************************************************************************************

   Function Name: SetFreq

   Purpose: Set TX/RX frequency. NOTE: it looks like setting the frequency takes some time to take effect.

   Arguments:  uint32_t freq - Frequency between 450000000 Hz and 470000000 Hz

   Returns: None

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
void SetFreq(uint8_t radioNum, uint32_t freq)
{
   uint8_t fc_inte;  // fractional-N PLL integer number
   uint32_t fc_fraq; // fractional-N PLL fraction number
   uint8_t fc_fraq_1;
   uint8_t fc_fraq_2;
   uint8_t fc_fraq_3;
   uint32_t TCXOfreq;
   double  temp;

   if ( !radio[radioNum].configured ) {
      ERR_printf( "ERROR - SetFreq: radio %u is disabled", radioNum );
      return;
   }

   // Save new frequency
//    radio[radioNum].Freq = freq;

#if ( EP == 1 )
   char floatStr[PRINT_FLOAT_SIZE];
   // Adjust TXCO frequency to compensate for drift due to aging and temperature
   // We use the error which is the difference between the local radio center frequency and the transmitter (likely a DCU) center frequency as reported by the radio.
   // We assume that DCUs are more precise than EPs especialy with DCU2+ using GPS.

   // Adjust TCXO if needed
   // This needs to be done only once after the error was computed.
   // We don't want to modify the TCXO everytime the radio is re-started because the radio can be restarted many times whitout a new error being mesured. (e.g. CCA, noise estimate)
   if ( radio[radioNum].adjustTCXO ) {
      radio[radioNum].adjustTCXO = false; // Don't adjust TCXO again until a new error was computed

      ( void )RADIO_TCXO_Get( radioNum, &TCXOfreq, NULL, NULL ),
      INFO_printf( "TCXO %uHz, TX/RX freq offset %dHz, residual error %sHz",
      TCXOfreq,
      radio[radioNum].frequencyOffset,
      DBG_printFloat( floatStr, radio[radioNum].error, 2 ) );

      // Compute current TXCO frequency based on error
      TCXOfreq -= ( uint32_t )( int32_t )( radio[radioNum].error / PHY_AFC_MIN_FREQ_STEP ); // The double casting is to make lint happy

      // Update TXCO frequency
      if ( RADIO_TCXO_Set( radioNum, TCXOfreq, eTIME_SYS_SOURCE_AFC, ( bool )false ) ) {
         radio[radioNum].error = 0; // Doing this changes the behavior of the error removal.
         // Instead of looking like a sine wave, it looks like a sawtooth.
         // The advantage is that the TCXO frequency won't oscillate around the target value
         // But slowly converge toward it more like a capacitor charge curve.
      }
   }
#endif

   // Formula based on datasheet section 5.3.1
   ( void )RADIO_TCXO_Get( radioNum, &TCXOfreq, NULL, NULL );
#if ( TEST_900MHZ == 1 )
   temp    = ( double )freq / ( TCXOfreq / ( double )2 ); // TCXO/2 is from 2*TCXO/4 (4 is for 850 to 1050 MHz band)
#else
   temp    = ( double )freq / ( TCXOfreq / ( double )4 ); // TCXO/4 is from 2*TCXO/8 (8 is for 420 to 525 MHz band)
#endif
   fc_inte = ( uint8_t )( temp - 1 );
   fc_fraq = ( uint32_t )( ( ( temp - ( double )fc_inte ) * ( double )524288 ) + 0.5f ); // Round up
   fc_fraq_1 = ( uint8_t )( fc_fraq >> 16 );
   fc_fraq_2 = ( uint8_t )( fc_fraq >> 8 );
   fc_fraq_3 = ( uint8_t )fc_fraq;

   ( void )si446x_set_property( radioNum,
                                SI446X_PROP_GRP_ID_FREQ_CONTROL,
                                4,
                                SI446X_PROP_GRP_INDEX_FREQ_CONTROL_INTE,
                                fc_inte,
                                fc_fraq_1,
                                fc_fraq_2,
                                fc_fraq_3 );

}

/***********************************************************************************************************************

   Function Name: wait_us

   Purpose: Wait some microseconds

   Arguments:  time - wait time in microseconds

   Returns: None

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
static void wait_us(uint32_t time)
{
   OS_TICK_Struct time1,time2;
   uint32_t       TimeDiff;

   if (time == 0) return;

#if ( MCU_SELECTED == NXP_K24 )
   bool           Overflow;
   _time_get_elapsed_ticks(&time1);

   // Wait for a while
   do {
      _time_get_elapsed_ticks(&time2);
      TimeDiff = (uint32_t)_time_diff_microseconds ( &time2, &time1, &Overflow );
   }
   while (TimeDiff < time);
#elif ( MCU_SELECTED == RA6E1 )
   #if 0 // TODO: RA6E1 Bob: This does not appear to be working properly
   OS_TICK_Get_CurrentElapsedTicks(&time1);

   // Wait for a while
   do {
      OS_TICK_Get_CurrentElapsedTicks(&time2);
      TimeDiff = (uint32_t)OS_TICK_Get_Diff_InMicroseconds ( &time1, &time2 );
   }
   while (TimeDiff < time);
   #else
   #warning "Using the Renesas R_BSP_SoftwareDelay instead of OS_TICK_Get_Diff_InMicroseconds and dividing by 2!"
   R_BSP_SoftwareDelay ((time+1)/2, BSP_DELAY_UNITS_MICROSECONDS);
   #endif
#endif

}
#if ( NOISE_HIST_ENABLED == 1 )
/***********************************************************************************************************************

   Function Name: wait_for_stable_RSSI

   Purpose: Wait until radio's RSSI is valid (non-zero) or 10 msec, whichever is longer

   Arguments:  radio_num      - index of the radio (0, or 0-8 for frodo)
               processingType - return the processing type that took place: STABLE or AVERAGE
               samplingRate   - RSSI sampling rate

   Returns: None

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
static float32 wait_for_stable_RSSI(uint8_t radioNum)
{
   OS_TICK_Struct time1,time2;
   uint32_t TimeDiff;
   bool Overflow;
   uint8_t rawRSSI;
#if ( MCU_SELECTED == NXP_K24 )
   _time_get_elapsed_ticks(&time1);
#elif ( MCU_SELECTED == RA6E1 )
   OS_TICK_Get_CurrentElapsedTicks(&time1);
#endif

   // Poll RSSI until valid
   do {
      si446x_get_modem_status(radioNum, 0xFF); // Don't clear int.  get_int_status needs those.
      rawRSSI = Si446xCmd.GET_MODEM_STATUS.LATCH_RSSI;
#if ( MCU_SELECTED == NXP_K24 )
      _time_get_elapsed_ticks(&time2);
      TimeDiff = (uint32_t)_time_diff_milliseconds ( &time2, &time1, &Overflow );
#elif ( MCU_SELECTED == RA6E1 )
      OS_TICK_Get_CurrentElapsedTicks(&time2);
      TimeDiff = (uint32_t)OS_TICK_Get_Diff_InMilliseconds ( &time1, &time2 );
#endif

      // Get out if we have been stuck here more than 10 ms.
      if (TimeDiff > 10) {
         break;
      }
#if 0  //TODO: RA6E1
      _sched_yield();
#endif
   } while (rawRSSI == 0); // We use the latch value as a way to consume time until RSSI is meaningful.

   return ( (float32)rawRSSI );
}

/***********************************************************************************************************************

   Function Name: RADIO_SetupNoiseReadings

   Purpose: Takes control over radio and sets up for a series of noise measurements

   Arguments:  channel        - SRFN channel to use (0 - 3200)
               radio_num      - index of the radio (0, or 0-8 for frodo)
               samplingRate   - RSSI sampling rate

   Returns: None

   Side Effects: Radio is started and ready to capture RSSI values as noise readings

   Reentrant Code: No

 **********************************************************************************************************************/
void RADIO_SetupNoiseReadings ( uint8_t radioNum, uint16_t samplingDelay, uint16_t channel )
{
   uint32_t i;
      // Check if radio 0 was in the middle of receiving a message
   if ( radioNum != RADIO_0 )
   {
      if ( RADIO_Is_RX((uint8_t)RADIO_0) )  RadioEvent_RxAbort( (uint8_t)RADIO_0 );
   }

   // Disable RSSI jump first thing to avoid detection
   Radio_Disable_RSSI_Jump(radioNum);

   // Check if radio was in the middle of receiving a message
   if (RADIO_Is_RX(radioNum)) {
      RadioEvent_RxAbort(radioNum);
   }

   // Set RX frequency
   SetFreq(radioNum, CHANNEL_TO_FREQ( channel ) );

   // Set PA to RX
   //setPA(eRADIO_PA_MODE_RX);

   // Disable interrupts in case the radio latches on a preamble. We don't want to process that.
   Radio_int_disable(radioNum);

   /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
   si446x_start_rx(radioNum, 0, 0u, PHY_MAX_FRAME,
                     SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                     SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                     SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN );

   ( void ) wait_for_stable_RSSI(radioNum);
   // Discard the first 4 samples because the RSSI is not stable yet.
   for (i=0; i<4; i++) {
      wait_us(samplingDelay);
      si446x_get_modem_status(radioNum, 0xFF); // Don't clear int.
   }
}

uint8_t RADIO_GetNoiseValue( uint8_t radioNum, uint16_t samplingDelay)
{
   wait_us( samplingDelay );
   si446x_get_modem_status(radioNum, 0xFF); // Don't clear int.
   return ( (uint8_t)( Si446xCmd.GET_MODEM_STATUS.CURR_RSSI ) );
}
/***********************************************************************************************************************

   Function Name: RADIO_TerminateNoiseReadings

   Purpose: Puts the radio back into normal receiving mode after noise readings are completed

   Arguments:  radio_num      - index of the radio (0, or 0-8 for frodo)

   Returns: None

   Side Effects: Radio is returned back to waiting for SYNC/PREAMBLE

   Reentrant Code: No

 **********************************************************************************************************************/
void RADIO_TerminateNoiseReadings(uint8_t radioNum)
{
   si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);

   // Clear interrupts
   si446x_get_int_status_fast_clear(radioNum);
}


#endif
/***********************************************************************************************************************

   Function Name: RADIO_Filter_CCA

   Purpose: Extract RSSI values and filter them.

   Arguments:  radio_num      - index of the radio (0, or 0-8 for frodo)
               processingType - return the processing type that took place: STABLE or AVERAGE
               samplingRate   - RSSI sampling rate

   Returns: current RSSI

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
float32_t RADIO_Filter_CCA(uint8_t radioNum, CCA_RSSI_TYPEe *processingType, uint16_t samplingRate)
{
   uint32_t  i,j;
   uint32_t  rssi[RSSI_AVERAGE_SIZE]={0};
   uint32_t  minValue, maxValue;
   float32_t sum=0;
   float32_t currentRSSI;
   OS_TICK_Struct time1,time2;
   uint32_t TimeDiff;
   PHY_GetReq_t GetReq;
   int8_t       FrontEndGain;
   union si446x_cmd_reply_union Si446xCmd;

#if ( MCU_SELECTED == NXP_K24 )
   _time_get_elapsed_ticks(&time1);
#elif ( MCU_SELECTED == RA6E1 )
   OS_TICK_Get_CurrentElapsedTicks(&time1);
#endif

   // Poll RSSI until valid
   do {
      (void)si446x_get_modem_status(radioNum, 0xFF, &Si446xCmd); // Don't clear int.  get_int_status needs those.
      currentRSSI = Si446xCmd.GET_MODEM_STATUS.LATCH_RSSI;

#if ( MCU_SELECTED == NXP_K24 )
      bool Overflow;
      _time_get_elapsed_ticks(&time2);
      TimeDiff = (uint32_t)_time_diff_milliseconds ( &time2, &time1, &Overflow );
#elif ( MCU_SELECTED == RA6E1 )
      OS_TICK_Get_CurrentElapsedTicks(&time2);
      TimeDiff = (uint32_t)OS_TICK_Get_Diff_InMilliseconds ( &time1, &time2 );
#endif

      // Get out if we have been stuck here more than 10 ms.
      if (TimeDiff > 10) {
         break;
      }
      OS_TASK_Yield(); // TODO: RA6E1 Bob: this needs to be tested for compatibility
   } while (currentRSSI == 0); // We use the latch value as a way to consume time until RSSI is meaningful.

   // Discard the first 4 samples because the radio internaly averages 4 symbols and the RSSI might not be stable yet.
   wait_us((uint32_t)833); // 4 symbols at 4800 baud is 833 us
   (void)si446x_get_modem_status(radioNum, 0xFF, &Si446xCmd); // Don't clear int.

   // Filter RSSI
   // Load first values
   for (i=0; i<RSSI_CLUSTER_SIZE-1; i++) {
      wait_us((uint32_t)samplingRate);
      (void)si446x_get_modem_status(radioNum, 0xFF, &Si446xCmd); // Don't clear int.
      rssi[i] = (uint32_t)Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;
   }

   // Load up to RSSI_AVERAGE_SIZE values
   for (; i<RSSI_AVERAGE_SIZE; i++) {
      wait_us((uint32_t)samplingRate);
      (void)si446x_get_modem_status(radioNum, 0xFF, &Si446xCmd); // Don't clear int.
      rssi[i] = Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;

      // Check if the last RSSI_CLUSTER_SIZE values are close thogether.
      // No more than 1dBm difference between max and min values.
      maxValue = 0;
      minValue = 255;
      for (j=(i-RSSI_CLUSTER_SIZE)+1; j<=i; j++) {
         if (rssi[j] > maxValue) {
            maxValue = rssi[j];
         }
         if (rssi[j] < minValue) {
            minValue = rssi[j];
         }
      }

      // Check if we found a stable RSSI value.
      if (maxValue-minValue <= 2) {
         currentRSSI = rssi[i];
         *processingType = eCCA_RSSI_TYPE_STABLE;
//         INFO_printf("Found RSSI");
         break;
      }
   }

   if (i == RSSI_AVERAGE_SIZE) {
      // We didn't find a stable RSSI value so it must be noise
      // Average the values together
      sum = 0;
      for (i=0; i<RSSI_AVERAGE_SIZE; i++) {
         sum += powf(10.0f, (float)(rssi[i])/10.0f); // Convert to linear and average.
      }
      currentRSSI = log10f(sum/RSSI_AVERAGE_SIZE)*10.0f; // Convert to dB

      // There should not be more than 8dBm difference between max and min values to be considered noise.
      // This is to reject unexpected distributions
      maxValue = 0;
      minValue = 255;
      for (i=0; i<RSSI_AVERAGE_SIZE; i++) {
         if (rssi[i] > maxValue) {
            maxValue = rssi[i];
         }
         if (rssi[i] < minValue) {
            minValue = rssi[i];
         }
      }

      if ( (maxValue - minValue) <= 24) {
         *processingType = eCCA_RSSI_TYPE_AVERAGE;
//         INFO_printf("Averaged RSSI");

      } else {
         *processingType = eCCA_RSSI_TYPE_UNKNOWN;
//       INFO_printf("Unknown RSSI");
      }
   }

   // Adjust RSSI to compensate for front end gain
   GetReq.eAttribute = ePhyAttr_FrontEndGain;
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)&FrontEndGain);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion
   currentRSSI += FrontEndGain;
   if ( currentRSSI < MINIMUM_RSSI_VALUE ){
      currentRSSI = MINIMUM_RSSI_VALUE;
   } else if (currentRSSI > 255 ) {
      currentRSSI = 255;
   }

   return currentRSSI;
}

/*!
 *  True if radio is in TX state
 *
 */
bool RADIO_Is_TX(void)
{
   return (radio[RADIO_0].isDeviceTX);
}

/*!
 *  True if radio is receiving a frame
 *
 *  @param radioNum - which radio to query
 *
 */
static bool RADIO_Is_RX(uint8_t radioNum)
{
   union si446x_cmd_reply_union Si446xCmd;

   /* Read ITs, clear pending ones */
   (void)si446x_get_int_status(radioNum, 0xFF, 0xFF, 0xFF, &Si446xCmd); // Get interrupt status but don't clear them

   if (  (Si446xCmd.GET_INT_STATUS.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_STATUS_PREAMBLE_DETECT_BIT) ||
         (Si446xCmd.GET_INT_STATUS.MODEM_PEND & SI446X_CMD_GET_INT_STATUS_REP_MODEM_STATUS_SYNC_DETECT_BIT)) {
      return (bool)true;
   } else {
      return (bool)false;
   }
}

/***********************************************************************************************************************

   Function Name: RADIO_Do_CCA

   Purpose: Do a Clear-Channel Assessment (CCA).

   Arguments:  radio_num - index of the radio (0, or 0-8 for frodo)
               chan      - channel to use for CCA

   Returns: RSSI

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
static uint8_t RADIO_Do_CCA(uint8_t radioNum, uint16_t chan)
{
   CCA_RSSI_TYPEe processingType;
   //char     floatStr[PRINT_FLOAT_SIZE];
   uint8_t  rssi;

   INFO_printf("CCA Channel = %hu (%u Hz) on radio %u", chan, CHANNEL_TO_FREQ(chan), radioNum);

   // Check if radio is already receiving on the desired channel
   if (radio[radioNum].rx_channel == chan) {
      // Just get RSSI
      rssi = (uint8_t)RADIO_Filter_CCA(radioNum, &processingType, CCA_SLEEP_TIME);
   } else {
      // Disable RSSI jump first thing to avoid detection
      (void)Radio_Disable_RSSI_Jump(radioNum);

      // Check if radio was in the middle of receiving a message
      if (RADIO_Is_RX(radioNum)) {
         RadioEvent_RxAbort(radioNum);
      }

      // Set PA to RX
      //setPA(eRADIO_PA_MODE_RX);

      // Set RX frequency
      SetFreq(radioNum, CHANNEL_TO_FREQ(chan));

      // Disable interrupts in case the radio latches on a preamble. We don't want to process that.
      (void)Radio_int_disable(radioNum);

      /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
      (void)si446x_start_rx(radioNum, 0, 0u, PHY_MAX_FRAME,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN );

      rssi = (uint8_t)RADIO_Filter_CCA(radioNum, &processingType, CCA_SLEEP_TIME);

      // Put radio back in previous state
      if (radio[radioNum].rx_channel != PHY_INVALID_CHANNEL) {
         // A valid channel means the radio was configured as a receiver
         vRadio_StartRX(radioNum, radio[radioNum].rx_channel);
      } else {
         if ( radioNum == (uint8_t)RADIO_0 ) {
            (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);  // Force ready to keep 30MHz output
         } else {
            (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);  // Force standby state when not in use
         }
      }
   }

   //INFO_printf("CCA RSSI = %sdBm", DBG_printFloat(floatStr, RSSI_RAW_TO_DBM(rssi), 1));

   return (rssi); /*lint !e438 last value assigned to processingType not used.   */
}

#if ( TEST_SYNC_ERROR == 1 )
/***********************************************************************************************************************

   Function Name: RADIO_Set_SyncError

   Purpose: Set SYNC bit error

   Arguments:  err - Defines the number of bit errors that are allowed in the Sync Word field during receive Sync Word detection

   Returns: none

   Side Effects: none

   Reentrant Code: No

 **********************************************************************************************************************/
void RADIO_Set_SyncError(uint8_t err)
{
   union   si446x_cmd_reply_union Si446xCmd;
   uint8_t sync_config;

   // Get current register value
   (void)si446x_get_property( (uint8_t)RADIO_0,
                              SI446X_PROP_GRP_ID_SYNC,           // Property group.
                              1,                                 // Number of property to be set.
                              SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                              &Si446xCmd );

   sync_config = Si446xCmd.GET_PROPERTY.DATA[0];

   // Clear SYNC error
   sync_config &= ~SI446X_PROP_SYNC_CONFIG_RX_ERRORS_MASK;

   sync_config |= err << SI446X_PROP_SYNC_CONFIG_RX_ERRORS_LSB;

   (void)si446x_set_property( (uint8_t)RADIO_0,
                              SI446X_PROP_GRP_ID_SYNC,           // Property group.
                              1,                                 // Number of property to be set.
                              SI446X_PROP_GRP_INDEX_SYNC_CONFIG, // Start sub-property address.
                              sync_config);
}
#endif

/***********************************************************************************************************************

   Function Name: RADIO_Get_RSSI

   Purpose: Read many RSSI values

   Arguments:  radio_num - index of the radio (0, or 0-8 for frodo)
               chan      - channel to read RSSI from
               buf       - buffer where to put the samples
               nSamples  - number of samples needed
               rate      - sampling rate
               boost     - use boost capacitor if true

   Returns: RSSI array
            function name returns the lowest observed super-Cap voltage if boost is non-zero

   Side Effects: The radio is not put back on its previous state

   Reentrant Code: No

 **********************************************************************************************************************/
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 0 )
void RADIO_Get_RSSI(uint8_t radioNum, uint16_t chan, uint8_t *buf, uint16_t nSamples, uint16_t rate, uint8_t boost)
#else
float RADIO_Get_RSSI(uint8_t radioNum, uint16_t chan, uint8_t *buf, uint16_t nSamples, uint16_t rate, uint8_t boost)
#endif
{
   uint32_t     i; // Loop counter
   int8_t       FrontEndGain;
   int32_t      tempRSSI;
   PHY_GetReq_t GetReq;
#if ( EP == 1 )
   float        fSuperCapV = SUPER_CAP_MIN_VOLTAGE;
#else
   (void)boost;
#endif
   union si446x_cmd_reply_union Si446xCmd;
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 1 )
   float lowestCapVoltage = 9.99;
#endif
   // Stop radio
   (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);

   // Set RX frequency
   SetFreq(radioNum, CHANNEL_TO_FREQ(chan));

   // Disable interrupts in case the radio latches on a preamble. We don't want to process that.
   (void)Radio_int_disable(radioNum);

   /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
   (void)si446x_start_rx(  radioNum, 0, 0u, PHY_MAX_FRAME,
                           SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                           SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                           SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN );

#if ( EP == 1 )
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 1 )
   fSuperCapV = ADC_Get_SC_Voltage();
   if ( fSuperCapV < lowestCapVoltage ) lowestCapVoltage = fSuperCapV;
#endif
   if ( boost ) {
#if 0
      // Turn on boost
      PWR_USE_BOOST();
#else
      PWR_3V6BOOST_EN_TRIS_ON();
      R_BSP_SoftwareDelay (((uint32_t)PWR_3V6BOOST_EN_ON_DLY_MS)*1000/2, BSP_DELAY_UNITS_MICROSECONDS);
      PWR_3P6LDO_EN_TRIS_OFF();
#endif
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 0 )
      // Get voltage
      fSuperCapV = ADC_Get_SC_Voltage();
#endif
      // Make sure the capacitor voltage is high enough to compute noise estimate with boost turned on.
      if ( fSuperCapV < SUPER_CAP_MIN_VOLTAGE ) {
#if 0
         PWR_3P6LDO_EN_TRIS_ON();
         R_BSP_SoftwareDelay (((uint32_t)PWR_3P6LDO_EN_ON_DLY_MS)*1000/2, BSP_DELAY_UNITS_MICROSECONDS);
         PWR_3V6BOOST_EN_TRIS_OFF();
#else
         PWR_USE_LDO();
#endif
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 0 )
         INFO_printf("vf %u", (uint32_t)(fSuperCapV*100));
#endif
      }
   }
#endif
   // Discard the first 4 samples because the radio internaly averages 4 symbols and the RSSI might not be stable yet.
   wait_us((uint32_t)1000); // 4 symbols at 4800 baud is 833 us

   // Get RSSI values
   for (i=0; i<nSamples; i++) {
      wait_us((uint32_t)rate);
      (void)si446x_get_modem_status(radioNum, 0xFF, &Si446xCmd); // Don't clear int.
      buf[i] = Si446xCmd.GET_MODEM_STATUS.CURR_RSSI;
   }
#if ( EP == 1 )
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 1 )
   fSuperCapV = ADC_Get_SC_Voltage();
   if ( fSuperCapV < lowestCapVoltage ) lowestCapVoltage = fSuperCapV;
#endif
   if ( boost ) {
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 0 )
      // Get voltage
      fSuperCapV = ADC_Get_SC_Voltage();
#endif
      // Turn off capacitor boost
      PWR_USE_LDO();
      // Make sure the capacitor voltage was high enough during the whole run
      if ( fSuperCapV < SUPER_CAP_MIN_VOLTAGE ) {
         // Mark all samples as bad
         memset( buf, 0xFF, nSamples );
      }
   }
#endif
   // Adjust RSSI to compensate for front end gain
   GetReq.eAttribute = ePhyAttr_FrontEndGain;
   (void) PHY_Attribute_Get(&GetReq, (PHY_ATTRIBUTES_u*)(void *)&FrontEndGain);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   for (i=0; i<nSamples; i++) {
      tempRSSI  = (int32_t)buf[i];
      tempRSSI += (int32_t)FrontEndGain;
      // Clip
      if ( tempRSSI < MINIMUM_RSSI_VALUE ){
         buf[i] = MINIMUM_RSSI_VALUE;
      } else if ( tempRSSI > 255 ) {
         buf[i] = 255;
      } else {
         buf[i] = (uint8_t)tempRSSI;
      }
   }
#if ( NOISEBAND_LOWEST_CAP_VOLTAGE == 1 )
   return ( lowestCapVoltage );
#endif
}

/*!
 * Do_CCA for a specific channel.
 *
 *  @param chan - channel to use (0-3200)
 *  @param rssi - computed RSSI
 *
 *  @return status
 *
 */
static PHY_CCA_STATUS_e Do_CCA(uint16_t chan, uint8_t *rssi)
{
   PHY_GetReq_t GetReq;
   uint16_t     RxChannels[PHY_RCVR_COUNT];  /*!< List of the Receive Channels */
   bool         transmitting = (bool)false;
   uint8_t      radioNum;

//   OS_TICK_Struct time1,time2;
//   uint32_t TimeDiff;
//   boolean Overflow;

//_task_stop_preemption();

   // Check to see if one receiver is already receiving on the requested channel.
   for (radioNum=(uint8_t)RADIO_FIRST_RX; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      if ((radio[radioNum].configured == (bool)true) && (radio[radioNum].rx_channel == chan)) {
         // We found a radio that is already receiving on the requested channel
         // Do CCA
         *rssi = RADIO_Do_CCA(radioNum, chan);
         return ePHY_CCA_SUCCESS;
      }
   }

   // Get RX channel list
   GetReq.eAttribute = ePhyAttr_RxChannels;
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)RxChannels);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // We didn't find a receiver so use the first RX radio not transmitting to do CCA
   for (radioNum=(uint8_t)RADIO_FIRST_RX; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      if ((radio[radioNum].configured == (bool)true) && (RxChannels[radioNum] != PHY_INVALID_CHANNEL)) {
         // Found a radio but is it transmitting?
         if ( (radioNum == (uint8_t)RADIO_0) && RADIO_Is_TX()) {
            // At least one radio is transmitting, keep looking
            transmitting = (bool)true;
         } else {
            // We found a radio but it is on the wrong channel so steal it
            *rssi = RADIO_Do_CCA(radioNum, chan);
            return ePHY_CCA_SUCCESS;
         }
      }
   }

//_time_get_elapsed_ticks(&time2);

//   TimeDiff = (uint32_t)_time_diff_milliseconds ( &time2, &time1, &Overflow );

//   INFO_printf("CCA time %u %u", TimeDiff, Overflow);

//_task_start_preemption();

   // We reach this point only if you could not find/use a radio to get RSSI
   if (transmitting) {
      // All radios were in TX mode
      return ePHY_CCA_BUSY;
   } else {
      // RX channels didn't contain a single valid channel
      return ePHY_CCA_INVALID_CONFIG;
   }
}

/*!
 * Find a radio tied to a receiver. If a valid channel is specified, find a RX radio that is his channel is the closest to the requested one.
 *
 *  @param radioNum - radio supporting RX
 *  @param ReqChannel  - Find radio closest to this channel
 *
 *  @return TRUE - radio was found
 */
static bool RADIO_Find_RX_Radio(uint8_t *radioNum, uint16_t ReqChannel)
{
   PHY_GetReq_t GetReq;
   uint16_t RxChannels[PHY_RCVR_COUNT];
   uint8_t  RadioId;
   int32_t  distance = ( int32_t )PHY_INVALID_CHANNEL; // This makes the distance large
   bool     radioFound = false;

   // Get RX channel list
   GetReq.eAttribute = ePhyAttr_RxChannels;
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)RxChannels);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Find a RX radio
   // 1) Scan through the list of RX radios looking for a valid RX radio
   // 2) If channel is valid, compute the distance between the 2 channels
   // 3) if distance is smallest yet, save radio number and distance
   // 4) Return radio number with the smallest distance
   RadioId = (uint8_t)RADIO_FIRST_RX;
   for(; RadioId < PHY_RCVR_COUNT; RadioId++) {
      if (PHY_IsRXChannelValid(RxChannels[RadioId])) {
         // Compute distance if needed
         if ( ReqChannel != PHY_INVALID_CHANNEL ) {
            if ( distance > (abs((int32_t)RxChannels[RadioId]-(int32_t)ReqChannel)) ) {
               // Update with new distance
               distance = abs((int32_t)RxChannels[RadioId]-(int32_t)ReqChannel);
               // Found a radio. Save it.
               *radioNum = RadioId;
               radioFound = true;
            }
         } else {
            // Found a radio. Save it and return
            *radioNum = RadioId;
            return (bool)true;
         }
      }
   }

   return radioFound;
}

/*!
 * Set the PHY Noise Estimate
 */
static bool RADIO_NoiseEstimate_Set(int16_t const *NoiseEstimate)
{
   bool bStatus = true;
   PHY_SetReq_t  SetReq;

   // Set the noise estimate rate
   SetReq.eAttribute = ePhyAttr_NoiseEstimate;
   (void)memcpy(SetReq.val.NoiseEstimate, NoiseEstimate, sizeof(SetReq.val.NoiseEstimate)) ;
   if(PHY_Attribute_Set(&SetReq) != ePHY_SET_SUCCESS)
   {
      bStatus = false;
   }
   return bStatus;
}

/*!
 * Set the PHY Noise Estimate Boost On
 */
#if ( EP == 1 )
static bool RADIO_NoiseEstimateBoostOn_Set(int16_t const *NoiseEstimate)
{
   bool bStatus = true;
   PHY_SetReq_t  SetReq;

   // Set the noise estimate rate
   SetReq.eAttribute = ePhyAttr_NoiseEstimateBoostOn;
   (void)memcpy(SetReq.val.NoiseEstimateBoostOn, NoiseEstimate, sizeof(SetReq.val.NoiseEstimateBoostOn)) ;
   if(PHY_Attribute_Set(&SetReq) != ePHY_SET_SUCCESS)
   {
      bStatus = false;
   }
   return bStatus;
}
#endif
/*!
 * Compute the sum of RSSI noise averages in linear domain if all values meet certain criteria.
 *
 *  @param RSSI              - Value to be insterted in the averaged RSSI list
 *  @param averageRSSI       - List of averaged RSSI values
 *  @param averageCount      - Number of items in averageRSSI list
 *  @param sum               - Sum of averaged RSSI values
 *  @param checkRange        - Force average to be within a certain range
 *
 *  @return   TRUE - Sum is valid
 *
 */
 static bool Compute_Noise_Sum(float32_t RSSI, float32_t averageRSSI[PHY_NOISE_ESTIMATE_COUNT_MAX], uint8_t averageCount, float32_t *sum, bool checkRange)
 {
   uint32_t  i,j;
   uint32_t  maxValuePos=0, minValuePos=0;
   float32_t maxValue=0;
   float32_t minValue=255;
   float32_t range;

   // Insert RSSI in list
   for ( i = 0; i < averageCount; i++ ) {
      if ( averageRSSI[i] == 0 ) {
         // Found an empty slot
         averageRSSI[i] = RSSI;
         break;
      }
   }

   // Compute number of filled slot in averageRSSI list
   for ( j = 0, i = 0; i < averageCount; i++ ) {
      if ( averageRSSI[i] != 0 ) {
         j++;
      }
   }

   // Check if averageRSSI list is full
   if ( j == averageCount ) {
      // List is full. Find max and min values
      for ( i = 0; i < averageCount; i++ ) {
         if ( maxValue < averageRSSI[i] ) {
            maxValue = averageRSSI[i];
            maxValuePos = i;
         }
         if ( minValue > averageRSSI[i] ) {
            minValue = averageRSSI[i];
            minValuePos = i;
         }
      }

      if ( checkRange ) {
         // Check that values are within 5dBm
         range = 10; // 5dBm in half dB count
      } else {
         range = 255; // Allow all values (i.e. range check always passes)
      }

      // Check if values are within range
      if ( ( maxValue - minValue ) <= range ) {
         // All values are good. Compute linear sum.
         *sum = 0;
         for ( i = 0; i < averageCount; i++ ) {
            *sum += powf( 10.0f, averageRSSI[i] / 10.0f ); // Convert to linear.
         }
         return true;
      } else {
         // Values are out of range
         // Discard the worst offenders
         averageRSSI[minValuePos] = 0;
         averageRSSI[maxValuePos] = 0;
         return false;
      }
   }

   return false;
}

/*!
 * Update the noise floor values
 *
 *  @param state   - PHY state used to restore radio state
 *
 *  @return   TRUE - Noise update done
 *
 * NOTE: It is assumed that no TX is taking place when this function is called.
 *       if it's not the case, TX will be stopped.
 *
 */
bool RADIO_Update_Noise_Floor(void)
{
#if PHY_MAX_CHANNEL != 32
#error "RADIO_Build_Noise_Floor function needs to be updated since radioMask is designed to support exactly 32 channels
#endif
   PHY_GetReq_t   GetReq;
   uint32_t       i,j; // loop counter
   uint32_t       mask;
   int16_t        NoiseEstimate[PHY_MAX_CHANNEL];
   uint16_t       Channels[PHY_MAX_CHANNEL];
   uint8_t        radioNum;
   float32_t      RSSI;
   float32_t      sum;
   CCA_RSSI_TYPEe processingType;

   static uint32_t radioMask = 0;
#if ( EP == 1 )
   static uint32_t radioMaskBoost = 0;
#endif
   static uint8_t   averageCount;
   static bool      done = (bool)false; // Done processing flag
   static bool      confRadio = (bool)false; // Radio is configured or not
   static uint32_t  maxTries;
   static float32_t averageRSSI[PHY_NOISE_ESTIMATE_COUNT_MAX]; // Hold averaged values
   union si446x_cmd_reply_union Si446xCmd;

#if ( EP == 1 )
   if ( (radioMask == 0) && (radioMaskBoost == 0) ) {
#else
   if (radioMask == 0) {
#endif
      INFO_printf("Noise estimation update starts");
      // This is first call
      radioMask = 0xFFFFFFFF;
#if ( EP == 1 )
      radioMaskBoost = 0xFFFFFFFF;
#endif
      // Get average count
      GetReq.eAttribute = ePhyAttr_NoiseEstimateCount;
      (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&averageCount);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion
   }

   // Get channel list
   GetReq.eAttribute = ePhyAttr_AvailableChannels;
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)Channels); //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Find if there is a RX radio that can be used to compute the noise floor
   if (!RADIO_Find_RX_Radio(&radioNum, PHY_INVALID_CHANNEL)) {
      INFO_printf("Noise estimation update aborted. No RX radio defined.");
      return (bool)true; // Didn't find a radio
   }

   // Get noise from radios
   for (i=0; i<PHY_MAX_CHANNEL; i++) {
      mask = 1<<i;   //lint !e701 left shift of signed qty OK
      // Check if this channel needs to be processed
#if ( EP == 1 )
      if ( (mask & radioMask) || (mask & radioMaskBoost) ) {
#else
      if ( mask & radioMask ) {
#endif
         // Is this a valid channel?
         if (PHY_IsChannelValid(Channels[i])) {
            // Configure radio for this channel if first call
            if (confRadio == (bool)false) {
               confRadio = (bool)true; // Don't configure the radio again for this channel
               maxTries  = 0;          // Reset counter for the maximum number of CCA tries
               done      = (bool)false;// Reset flag
               (void)memset(averageRSSI, 0, sizeof(averageRSSI)); // Reset average RSSI list
            }

            // Start radio
//            INFO_printf("Noise estimation of channel = %hu (%u Hz) on radio %u", Channels[i], CHANNEL_TO_FREQ(Channels[i]), RADIO_1);

            // Retrieve radio that has its channel closest to the one we want to gather statistics for.
            (void)RADIO_Find_RX_Radio(&radioNum, Channels[i]);

            // Disable interrupts to prevent radio from latching on a preamble. We don't want to process that.
            (void)Radio_int_disable(radioNum);

            // Check if radio was in the middle of receiving a message
            if (RADIO_Is_RX(radioNum)) {
               RadioEvent_RxAbort(radioNum);
            }

            // Set RX frequency
            SetFreq(radioNum, CHANNEL_TO_FREQ(Channels[i]));

            /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
            (void)si446x_start_rx(radioNum, 0, 0u, PHY_MAX_FRAME,
                                  SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                                  SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                                  SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN );

            // Make measurements
            sum = 0.0f;
            for (j=0; j<10; j++) {
#if ( EP == 1 )
               // On second pass, build noise estimate with capacitor boost on
               if ( (radioMask & mask) == 0) {
                  float fSuperCapV;
                  // Turn on boost
                  PWR_USE_BOOST();
                  // Get voltage
                  fSuperCapV = ADC_Get_SC_Voltage();

                  // Make sure the capacitor voltage is high enough to compute noise estimate with boost turned on.
                  if ( fSuperCapV < SUPER_CAP_MIN_VOLTAGE ) {
                     PWR_USE_LDO();
                     INFO_printf("SuperCap voltage too low to compute noise estimate for channel %u", Channels[i] );
                     radioMaskBoost &= ~mask;  // Remove from the Channel mask to not process this channel anymore
                     break; // Abort noise estimate
                  }
               }
#endif
               RSSI = RADIO_Filter_CCA(radioNum, &processingType, CCA_SLEEP_TIME);
#if ( EP == 1 )
               // On second pass, turn off capacitor boost
               if ( (radioMask & mask) == 0) {
                  // Turn off boost
                  PWR_USE_LDO();
               }
#endif
               if (processingType == eCCA_RSSI_TYPE_AVERAGE) {
                  if (Compute_Noise_Sum(RSSI, averageRSSI, averageCount, &sum, (bool)true)) {
                     // Retrieve noise estimate
#if ( EP == 1 )
                     if ( radioMask & mask ) {
                        GetReq.eAttribute = ePhyAttr_NoiseEstimate;
                     } else {
                        GetReq.eAttribute = ePhyAttr_NoiseEstimateBoostOn;
                     }
#else
                     GetReq.eAttribute = ePhyAttr_NoiseEstimate;
#endif
                     (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&NoiseEstimate[0]); //lint !e826 !e433  Suspicious pointer-to-pointer conversion

                     // Update noise
                     NoiseEstimate[i] = (int16_t)RSSI_RAW_TO_DBM(log10f(sum/averageCount)*10.0f); // Convert to dB

                     // Save updated noise estimate
#if ( EP == 1 )
                     if ( radioMask & mask ) {
                        ( void )RADIO_NoiseEstimate_Set(NoiseEstimate);
                     } else {
                        (void)RADIO_NoiseEstimateBoostOn_Set(NoiseEstimate);
                     }
#else
                     ( void )RADIO_NoiseEstimate_Set(NoiseEstimate);
#endif
                     done = true;
                     break;
                  }
               }
            }
            // Check if we reached maximum number of tries or are done with this channel
            if ((++maxTries >= MAX_CCA_TRIES) || done) {
#if ( EP == 1 )
               if ( radioMask & mask ) {
                  radioMask &= ~mask;       // Remove from the Channel mask to not process this channel anymore
               } else {
                  radioMaskBoost &= ~mask;  // Remove from the Channel mask to not process this channel anymore
               }
#else
               radioMask &= ~mask;       // Remove from the Channel mask to not process this channel anymore
#endif
               confRadio  = (bool)false; // Allow this radio to be reconfigured for a different channel
            }
            break; // Get out of loop
         } else {
#if ( EP == 1 )
            radioMaskBoost &= ~mask;  // Remove from the Channel mask to not process this channel anymore
#endif
            radioMask &= ~mask; // Remove from the Channel mask to not process this channel anymore
         }
      }
   }

   // Restore radio to previous state
   if (PHY_IsRXChannelValid(radio[radioNum].rx_channel)) {
      //MKD: Change this to call vRADIO_StartRX()?

      // Set PA to RX
      //setPA(eRADIO_PA_MODE_RX);

      // Set RX frequency
      SetFreq(radioNum, CHANNEL_TO_FREQ(radio[radioNum].rx_channel));

      /* Reset FIFO */
      (void)si446x_fifo_info(radioNum, SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT, &Si446xCmd);

      // Clear interrupts
      (void)si446x_get_int_status_fast_clear(radioNum);

      // Enable interrupts
      (void)Radio_RX_int_enable((uint8_t)radioNum);

      /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
      (void)si446x_start_rx(radioNum, 0, 0u, PHY_MAX_FRAME,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                            SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN );
   }

#if ( EP == 1 )
   if ( radioMask || radioMaskBoost ) {
#else
   if ( radioMask ) {
#endif
      return (bool)false;
   } else {
      // Done getting noise floor.
      INFO_printf("Noise estimation update done!");
      return true;
   }
}


/*!
 * Build a noise floor baseline
 *
 *  @param Channels_list     - List of PHY_MAX_CHANNEL channels to compute the noise for
 *  @param radioNum          - Which radio to use to do the noise estimate
 *  @param useSpecifiedRadio - True: Use the specified radio (previous argument). False: find radio from RX channel list
 *  @param samplingRate      - RSSI sampling rate
 *  @param useDefaultInError - True: Return the default RSSI value if function timed out. False: RSSI is -70dBm or true value.
 *  @param noiseEstimate     - Array of computed noise for each channel.
 *  @param noiseEstimateBoostOn - Array of computed noise when boost capacitor is on for each channel.
 *
 *  @return   TRUE - Noise floor was computed
 *
 * NOTE: This function doesn't restart the radio that was used for noise acquisition.
 */
bool RADIO_Build_Noise_Floor(uint16_t const *Channels_list, uint8_t radioNum, bool useSpecifiedRadio, uint16_t samplingRate, bool useDefaultInError, int16_t *noiseEstimate, int16_t
                              *noiseEstimateBoostOn)
{
   PHY_GetReq_t GetReq;
   uint32_t i,j; // loop counter
#if ( EP == 1 )
   uint32_t k; // loop counter
#else
   (void)noiseEstimateBoostOn; //Quiet Compiler
#endif
   uint8_t   averageCount;
   float32_t sum;
   int16_t   *pNoiseEstimate;
   uint16_t  Channels[PHY_MAX_CHANNEL];
   float32_t RSSI;
   float32_t averageRSSI[PHY_NOISE_ESTIMATE_COUNT_MAX]; // Hold averaged values
   CCA_RSSI_TYPEe processingType;

   // Get channel list
   if (Channels_list) {
      (void)memcpy(Channels, Channels_list, sizeof(Channels));
   } else {
      GetReq.eAttribute = ePhyAttr_AvailableChannels;
      (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)Channels); //lint !e826 !e433  Suspicious pointer-to-pointer conversion
   }

   // Get average rate
   GetReq.eAttribute = ePhyAttr_NoiseEstimateCount;
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)&averageCount);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // Do we need to find a RX radio
   if (useSpecifiedRadio == (bool)false) {
      // Find if there is a RX radio that can be used to compute the noise floor
      if (!RADIO_Find_RX_Radio(&radioNum, PHY_INVALID_CHANNEL)) {
         // Didn't find a radio
         INFO_printf("Noise estimation update aborted. No RX radio defined.");
         return false;
      }
   }

   // Stop radio if transmitting
   if (RADIO_Is_TX()) {
      StandbyTx(); // Make sure radio is not transmitting
      setPA(eRADIO_PA_MODE_RX); // Force RX
   }

   // Update noise floor for each RX channels
   for (i=0; i<PHY_MAX_CHANNEL; i++) {
      // Update noise estimate for valid channels
      if (Channels[i] != PHY_INVALID_CHANNEL) {
         // Start radio
//         INFO_printf("Noise estimation of channel = %hu (%u Hz) on radio %u", Channels[i], CHANNEL_TO_FREQ(Channels[i]), radioNum);

         // Retrieve radio that has its channel closest to the one we want to gather statistics for.
         if (useSpecifiedRadio == (bool)false) {
            (void)RADIO_Find_RX_Radio(&radioNum, Channels[i]);
         }

         // Disable RSSI jump first thing to avoid detection
         (void)Radio_Disable_RSSI_Jump(radioNum);

         // Check if radio was in the middle of receiving a message
         if (RADIO_Is_RX(radioNum)) {
            RadioEvent_RxAbort(radioNum);
         }

         // Set RX frequency
         SetFreq(radioNum, CHANNEL_TO_FREQ(Channels[i]));

         // Disable interrupts to prevent radio from latching on a preamble. We don't want to process that.
         (void)Radio_int_disable(radioNum);

         /* Start Receiving packet, channel 0, START immediately, Packet n bytes long */
         (void)si446x_start_rx(radioNum, 0, 0u, PHY_MAX_FRAME,
                               SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                               SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_REMAIN,
                               SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_REMAIN );

         // Point to noise estimate to update
         pNoiseEstimate = noiseEstimate;

#if ( EP == 1 )
         for (k=0; k<2; k++) {
            // On second pass, build noise estimate with capacitor boost on
            if ( k == 1 ) {
               //If do not want to do the Boost noise estimate
               if ( noiseEstimateBoostOn == NULL ) {
                  break;
               }
               float fSuperCapV;
               // Point to noise estimate to update
               pNoiseEstimate = noiseEstimateBoostOn;
               // Turn on boost
               PWR_USE_BOOST();
               // Get voltage
               fSuperCapV = ADC_Get_SC_Voltage();

               // Make sure the capacitor voltage is high enough to compute noise estimate with boost turned on.
               if ( fSuperCapV < SUPER_CAP_MIN_VOLTAGE ) {
                  PWR_USE_LDO();
                  noiseEstimateBoostOn = NULL; // Abort noise estimate with super cap on for all channels since super cap won't charge back up quickly enough for other channels.
                  INFO_printf("SuperCap voltage too low to compute noise estimate for channel %u", Channels[i] );
                  break; // Abort noise estimate
               }
            }
#endif
            // Make measurements
            (void)memset(averageRSSI, 0, sizeof(averageRSSI));
            sum = 0.0f;
            // Try up to a point to get a valid noise measurement
            for (j=0; j<MAX_CCA_TRIES; j++) {
               RSSI = RADIO_Filter_CCA(radioNum, &processingType, samplingRate);
               // We don't care about the processigType when building noise estimate for the noiseband command
               // This is because we want more raw RSSIs.
               // We know the call came from noiseband when Channels_list is not NULL
               if ( (processingType == eCCA_RSSI_TYPE_AVERAGE) || (Channels_list != NULL) ) {
                  if (Compute_Noise_Sum(RSSI, averageRSSI, averageCount, &sum, Channels_list == NULL?(bool)true:(bool)false)) {
                     break;
                  }
               }
            }

#if ( EP == 1 )
            // On second pass, turn off capacitor boost
            if ( k == 1 ) {
               // Turn off boost
               PWR_USE_LDO();
            }
#endif
            // Update noise estimate
            if (useDefaultInError == (bool)false) {
               if (j == MAX_CCA_TRIES) {
                  // Keep previous value
               } else {
                  // Update value
                  pNoiseEstimate[i] = (int16_t)RSSI_RAW_TO_DBM(log10f(sum/averageCount)*10.0f); // Convert to dB
               }
            } else {
               // Used default value if error
               if (j == MAX_CCA_TRIES) {
                  // Use default value
                  pNoiseEstimate[i] = PHY_CCA_THRESHOLD_MAX;
               } else {
                  // Update value
                  pNoiseEstimate[i] = (int16_t)RSSI_RAW_TO_DBM(log10f(sum/averageCount)*10.0f); // Convert to dB
               }
            }
            // Be nice to other tasks since we just used a lot of CPU time.
            // We don't pause when doing noiseband. We let noiseband pause after every n channels to speed up test
            if (Channels_list == NULL) {
               OS_TASK_Sleep(FIFTY_MSEC);
            }
#if ( EP == 1 )
         }
#endif
         if ( radioNum == (uint8_t)RADIO_0 ) {
            (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);  // Force ready to keep 30MHz output
         } else {
            (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);  // Force standby state
         }

         // Clear interrupts
         (void)si446x_get_int_status_fast_clear(radioNum);
      }
   }

   return (bool)true;
}

/*!
 * Set radios to standby TX mode
 *
 * Setting a radio to standby TX really just means stop TX
 *
 */
static void StandbyTx(void)
{
   INFO_printf("StandbyTx");

   if (radio[(uint8_t)RADIO_0].configured) {
      if (RADIO_Is_TX()) {
         RadioEvent_TxAbort((uint8_t)RADIO_0);               // Increment TxAbort counter
      }
      // Stop TX
      (void)Radio_int_disable((uint8_t)RADIO_0);                // Disable interrupts in case the radio latches on a preamble. We don't want to process that.
      (void)si446x_get_int_status_fast_clear((uint8_t)RADIO_0); // Clear interrupts
      (void)si446x_change_state((uint8_t)RADIO_0, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);  // Force ready to keep 30MHz output
      radio[RADIO_0].isDeviceTX = false;                        // Disable the TX watchdog

      // Restart radio if it was also used as RX
      if (radio[(uint8_t)RADIO_0].rx_channel != PHY_INVALID_CHANNEL) {
         // A valid channel means the radio was configured as a receiver
         setPA(eRADIO_PA_MODE_RX);
         vRadio_StartRX((uint8_t)RADIO_0, radio[(uint8_t)RADIO_0].rx_channel);
      }
   }
}

/*!
 * Set radios to standby RX mode
 *
 */
static void StandbyRx(void)
{
   uint8_t      radioNum;

   INFO_printf("StandbyRx");

   // For each radio
   // On the samwise board, the first radio (0) is configured with the RX Channel ( outbound channel )
   // On the frodo board, the first radio (0) is configured with the TX Channel
   // The remaining radios (1-8) are configured with the RX Channels
   for(radioNum = (uint8_t)RADIO_FIRST_RX; radioNum < PHY_RCVR_COUNT; radioNum++)
   {
      // Is radio configured?
      if (radio[radioNum].configured) {
         // Check if radio was in the middle of receiving a message
         if (RADIO_Is_RX(radioNum)) {
            RadioEvent_RxAbort(radioNum);
         }
         (void)Radio_int_disable(radioNum);
         (void)si446x_get_int_status_fast_clear(radioNum); // Clear interrupts
         radio[radioNum].rx_channel = PHY_INVALID_CHANNEL;
         if ( radioNum == (uint8_t)RADIO_0 ) {
            (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);  // Force ready to keep 30MHz output
         } else {
            (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);  // Force standby state
         }
      }
   }
}

/*!
 * Set radios to standby mode
 *
 */
static void Standby(void)
{
   setPA(eRADIO_PA_MODE_TX); // On Frodo this will set the front-end to TX.
                             // TX uses a lot less current than RX

   StandbyTx(); // Call this before StanbyRx
   StandbyRx();

   // Shutdown power amplifier
   RDO_PA_EN_OFF();
}

/*!
 * Set radios to ready TX mode
 *
 */
static void ReadyTx(void)
{
   // Nothing to do right now but this is a place holder.
   // We could have to access/configure some hardware in the future.
   INFO_printf("ReadyTx");
}

/*!
 * Set radios to ready RX mode
 *
 */
static void ReadyRx(void)
{
   PHY_GetReq_t GetReq;
   uint16_t     RxChannels[PHY_RCVR_COUNT];  /*!< List of the Receive Channels */
   uint8_t      radioNum;

   INFO_printf("ReadyRx");

   // Get RX channel list
   GetReq.eAttribute = ePhyAttr_RxChannels;
   (void)PHY_Attribute_Get( &GetReq, (PHY_ATTRIBUTES_u*)(void *)RxChannels);  //lint !e826 !e433  Suspicious pointer-to-pointer conversion

   // For each radio
   // On the samwise board, the first radio (0) is configured with the RX Channel ( outbound channel )
   // On the frodo board, the first radio (0) is configured with the TX Channel
   // The remaining radios (1-8) are configured with the RX Channel
   for(radioNum = (uint8_t)RADIO_FIRST_RX; radioNum < PHY_RCVR_COUNT; radioNum++)
   {
      // Is radio configured?
      if (radio[radioNum].configured) {
         // Is the channel configured?
         if(PHY_IsRXChannelValid(RxChannels[radioNum]))
         {
            // Force PA to RX. PA could have been in TX if we transition from MUTE to STANDARD for example
            setPA(eRADIO_PA_MODE_RX);
            // Start the radio in rx mode
            (void) Radio.StartRx(radioNum, RxChannels[radioNum]);
#if ( DCU == 1 )
            // On the T-board, booting all radios can take time and starve other tasks.
            // Be nice to other tasks.
            OS_TASK_Sleep( FIFTY_MSEC );
#endif
         } else {
            radio[radioNum].rx_channel = PHY_INVALID_CHANNEL;
            if ( radioNum == (uint8_t)RADIO_0 ) {
               (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);  // Force ready to keep 30MHz output
            } else {
               (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);  // Force standby state
            }
         }
      }
   }
}

/*!
 * Set radios to ready mode
 *
 */
static void Ready(void)
{
   PHY_GetReq_t     GetReq;
   PHY_ATTRIBUTES_u val;
   int16_t          noiseEstimate[PHY_MAX_CHANNEL];
   int16_t          noiseEstimateBoostOn[PHY_MAX_CHANNEL];

   // Get noise rate
   GetReq.eAttribute = ePhyAttr_NoiseEstimateRate;
   (void)PHY_Attribute_Get( &GetReq, &val);

   // Compute noise estimate
   if (val.NoiseEstimateRate) {
      INFO_printf("Noise estimate computation ...");

      // Get previous noise estimate
      GetReq.eAttribute = ePhyAttr_NoiseEstimate;
      (void)PHY_Attribute_Get( &GetReq, &val);
      (void)memcpy( noiseEstimate, val.NoiseEstimate, sizeof(noiseEstimate));
#if ( EP == 1 )
      // Get previous noise estimate with boost on
      GetReq.eAttribute = ePhyAttr_NoiseEstimateBoostOn;
      (void)PHY_Attribute_Get( &GetReq, &val);
      (void)memcpy( noiseEstimateBoostOn, val.NoiseEstimateBoostOn, sizeof(noiseEstimateBoostOn));
#endif
      // Gather noise statistics to build noise floor
      if (RADIO_Build_Noise_Floor(NULL, (uint8_t)RADIO_0, (bool)false, CCA_SLEEP_TIME, (bool)false, noiseEstimate, noiseEstimateBoostOn)) {
         INFO_printf("Noise estimate done!");
         (void)RADIO_NoiseEstimate_Set(noiseEstimate); // Save updated values or defaults
#if ( EP == 1 )
         (void)RADIO_NoiseEstimateBoostOn_Set(noiseEstimateBoostOn); // Save updated values or defaults
#endif
      }
   }

   ReadyTx();
   ReadyRx();
}

/*!
 *  Used to set the rx frequency and start the radio
 *
 *  @param radio_num    - index of the radio (0, or 0-8 for frodo)
 *  @param chan         - Channel to use (0-3200)
 *
 */
static bool StartRx(uint8_t radioNum, uint16_t chan)
{
   // Save the frequency
   radio[radioNum].rx_channel = chan;

   // Start the radio
   vRadio_StartRX(radioNum, radio[radioNum].rx_channel);
   return (bool)true;
}

/*!
 * Put radio to sleep mode
 *
 *  @param radio_num  0, or 0-8 for frodo
 *
 */
static bool SleepRx(uint8_t radioNum)
{
   radio[radioNum].rx_channel = PHY_INVALID_CHANNEL;

   // Leave current state
   if ( radioNum == (uint8_t)RADIO_0 ) {
      (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);  // Force ready to keep 30MHz output
   } else {
      (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);  // Force standby state
   }

   return (bool)true;
}

/*!
 *
 * Used to get the radio mode
 *
 */
RADIO_MODE_t RADIO_TxMode_Get(void)
{
   return ( currentTxMode );
}

/*!
 *  Used to set the radio mode
 *
 *  @param mode - Normal, PN9 or CW
 *
 */
void RADIO_Mode_Set(RADIO_MODE_t mode)
{
   uint8_t radioNum; // Loop counter
   PHY_GetReq_t GetReq;
   PHY_ATTRIBUTES_u val;
   union si446x_cmd_reply_union Si446xCmd;

   PHY_Lock(); // Function will not return if it fails

#if ( DCU == 1 )
   // Disable interrupt on GPIO peripheral module
   lwgpio_int_enable(&radioIRQIntPin[RADIO_0], FALSE);
   lwgpio_int_enable(&radioIRQIntPin[RADIO_1], (bool)FALSE);

   // Clear interrupt
   lwgpio_int_clear_flag(&radioIRQIntPin[RADIO_0]);
   lwgpio_int_clear_flag(&radioIRQIntPin[RADIO_1]);
#endif

   // Disable each radio
   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      if (radio[radioNum].configured == (bool)true) {
         (void)Radio_int_disable(radioNum);                 // Disable int to be safe
         (void)si446x_change_state(radioNum, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY); // Leave RX/TX state
         (void)si446x_get_int_status_fast_clear(radioNum);  // Clear interrupts
      }
   }

   MFG_Port_Print = (bool)false;

   // Re-enable MAC if we disabled it previously
   if ( ( currentTxMode == eRADIO_MODE_PN9 ) || ( currentTxMode == eRADIO_MODE_CW ) ) {
      ( void ) MAC_StartRequest( NULL );
      OS_TASK_Sleep ( QTR_SEC );
   }

   // Re-init the radios
#if !( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   vRadio_Init(mode); /*   Causes pulse on GPIO3 pin, possibly affecting eFEM */
#endif

   switch (mode) {
      case eRADIO_MODE_NORMAL:
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
         (void)BSP_PaDAC_SetValue( 0 );   /* Kill eFEM power   */
         setPA(eRADIO_PA_MODE_RX);
         break;
#endif

      case eRADIO_MODE_NORMAL_BER:
#if ( PORTABLE_DCU == 1)
      case eRADIO_MODE_NORMAL_DEV_600:
#endif
         if (mode == eRADIO_MODE_NORMAL_BER) {
            MFG_Port_Print = (bool)true;
         }
         // Start radios in RX mode
         ReadyRx();
         break;

      case eRADIO_MODE_PN9:
      case eRADIO_MODE_CW:
         // Prevent MAC from sending Data.request
#if !( (DCU == 1) && (VSWR_MEASUREMENT == 1) )  /* Legacy 9975T hardware   */
         (void) MAC_StopRequest( NULL );
         OS_TASK_Sleep ( QTR_SEC );
#endif
         setPA(eRADIO_PA_MODE_RX);

         // Get some variables
         GetReq.eAttribute = ePhyAttr_TxChannels;
         (void)PHY_Attribute_Get( &GetReq, &val );

         if (!PHY_IsTXChannelValid(val.TxChannels[RADIO_0])) {
               INFO_printf("Radio %u can't be configured because the channel is invalid", ( uint32_t )RADIO_0);
         } else {
            SetFreq((uint8_t)RADIO_0, CHANNEL_TO_FREQ(val.TxChannels[RADIO_0]));   // Configure TX frequency

            (void)Radio_int_disable((uint8_t)RADIO_0);                         // Disable interupts while in test mode
            (void)si446x_get_int_status_fast_clear((uint8_t)RADIO_0);          // Read ITs, clear pending ones
            // Enable CW
            if (mode == eRADIO_MODE_CW) {
               (void)si446x_set_property( (uint8_t)RADIO_0,
                                           SI446X_PROP_GRP_ID_MODEM,            // Property group.
                                           1,                                   // Number of property to be set.
                                           SI446X_PROP_GRP_INDEX_MODEM_MOD_TYPE,// Start sub-property address.
                                           0x08);                               // Source: Direct, Modulation type: CW
            } else {
               // Enable PN9
               (void)si446x_get_property( (uint8_t)RADIO_0,
                                          SI446X_PROP_GRP_ID_MODEM,              // Property group.
                                          1,                                     // Number of property to be set.
                                          SI446X_PROP_GRP_INDEX_MODEM_MOD_TYPE,  // Start sub-property address.
                                          &Si446xCmd);

               // Set modulation type to pseudo
               Si446xCmd.GET_PROPERTY.DATA[0] |= 0x10;

               (void)si446x_set_property( (uint8_t)RADIO_0,
                                          SI446X_PROP_GRP_ID_MODEM,             // Property group.
                                          1,                                    // Number of property to be set.
                                          SI446X_PROP_GRP_INDEX_MODEM_MOD_TYPE, // Start sub-property address.
                                          Si446xCmd.GET_PROPERTY.DATA[0]);      // PHY set to 2FSK
            }

            /* START immediately, Packet according to PH */
            // Set PA to TX
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
            GetReq.eAttribute = ePhyAttr_fngForwardPowerSet;
            (void)PHY_Attribute_Get( &GetReq, &val );
            RADIO_SetPower( 0, (float)val.fngForwardPowerSet / 10.0f ); /*  Frequency not used in RADIO_SetPower.  */
#endif
            setPA(eRADIO_PA_MODE_TX);
            (void)si446x_start_tx((uint8_t)RADIO_0, 0u, 0u, 0u, 0u);
         }
         break;

      case eRADIO_MODE_4GFSK_BER:
      case eRADIO_LAST_MODE:
      default:
         break;
   }

   PHY_Unlock(); // Function will not return if it fails
}


/*!
 *  Used to get the radio status
 *
 *  @param state - current opperating state
 *
 */
uint32_t RADIO_Status_Get(uint8_t radioNum, uint8_t *state, uint8_t *int_pend,   uint8_t *int_status,
                                                            uint8_t *ph_pend,    uint8_t *ph_status,
                                                            uint8_t *modem_pend, uint8_t *modem_status,
                                                            uint8_t *chip_pend,  uint8_t *chip_status)
{
   union si446x_cmd_reply_union Si446xCmd;

   if (radio[radioNum].configured) {
      (void)si446x_request_device_state(radioNum, &Si446xCmd);
      *state = Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE;

      (void)si446x_get_int_status(radioNum, 0xFF, 0xFF, 0xFF, &Si446xCmd); // Get interrupt status but don't clear them
      *int_pend     = Si446xCmd.GET_INT_STATUS.INT_PEND;
      *int_status   = Si446xCmd.GET_INT_STATUS.INT_STATUS;
      *ph_pend      = Si446xCmd.GET_INT_STATUS.PH_PEND;
      *ph_status    = Si446xCmd.GET_INT_STATUS.PH_STATUS;
      *modem_pend   = Si446xCmd.GET_INT_STATUS.MODEM_PEND;
      *modem_status = Si446xCmd.GET_INT_STATUS.MODEM_STATUS;
      *chip_pend    = Si446xCmd.GET_INT_STATUS.CHIP_PEND;
      *chip_status  = Si446xCmd.GET_INT_STATUS.CHIP_STATUS;

      return (bool)true;
   } else {
      return (bool)false;
   }
}

/*!
 *  Used to find if the radio started RX recently
 */
bool     RADIO_Get_RxStartOccured(uint8_t radioNum) { return (radio[radioNum].RxStartOccured); }

/*!
 *  Used to get the timestamp of when the radio started RX recently (only valid if radio[radioNum].RxStartOccured == 1)
 */
uint32_t RADIO_Get_RxStartCYCCNTTimeStamp(uint8_t radioNum) { return (radio[radioNum].RxStartCYCCNT); }

/*!
 *  Used to get the CYCCNT timestamp of when the radio generated an interrupt
 */
uint32_t RADIO_Get_IntFIFOCYCCNTTimeStamp(uint8_t radioNum) { return (radio[radioNum].intFIFOCYCCNT); }

/*!
 *  Used to clear the restart flag
 */
void     RADIO_Clear_RxStartOccured(uint8_t radioNum)
{
   radio[radioNum].RxStartOccured = false;
}

void     RADIO_Set_RxStart(uint8_t radioNum, uint32_t cyccnt) //both timestamp and boolean
{
   radio[radioNum].RxStartCYCCNT  = cyccnt;
   radio[radioNum].RxStartOccured = true;
}

void RADIO_Set_SyncTime(uint8_t radioNum, TIMESTAMP_t syncTime, uint32_t syncTimeCYCCNT)
{
   radio[radioNum].syncTime       = syncTime;
   radio[radioNum].syncTimeCYCCNT = syncTimeCYCCNT;
}

/*!
 *  Used to set the TX power level
 *
 *  @param powerLevel - Power level
 *
 */
void RADIO_Power_Level_Set(uint8_t powerLevel)
{
   (void)si446x_set_property( (uint8_t)RADIO_0,
                              SI446X_PROP_GRP_ID_PA,            // Property group.
                              1,                                // Number of property to be set.
                              SI446X_PROP_GRP_INDEX_PA_PWR_LVL, // Start sub-property address.
                              powerLevel);                      // Power Level
}

/*!
 *  Used to get the TX power level
 *
 */
uint8_t RADIO_Power_Level_Get(void)
{
   union si446x_cmd_reply_union Si446xCmd;

   // Get power level value
   (void)si446x_get_property( (uint8_t)RADIO_0,
                              SI446X_PROP_GRP_ID_PA,             // Property group.
                              1,                                 // Number of property to be set.
                              SI446X_PROP_GRP_INDEX_PA_PWR_LVL,  // Start sub-property address.
                              &Si446xCmd);

   return (Si446xCmd.GET_PROPERTY.DATA[0]);
}

/*!
 *  Used to get the radio temperature
 *
 */
bool RADIO_Temperature_Get(uint8_t radioNum, int16_t *temp)
{
   union si446x_cmd_reply_union Si446xCmd;

   if ( radio[radioNum].demodulator == 0 ) {
      if ( si446x_get_adc_reading(radioNum, SI446X_CMD_GET_ADC_READING_ARG_ADC_EN_TEMPERATURE_EN_BIT, 9, &Si446xCmd) == SI446X_SUCCESS ) {
         *temp = (int16_t)(((899.0/4096.0)*Si446xCmd.GET_ADC_READING.TEMP_ADC)-293);
      } else {
         return (bool)false;
      }
   } else {
      // Reading the radio temperature takes about 1,s which is way too long when using the soft-demodulator.
      // Use the CPU temperature as a proxy instead.
#if ( MCU_SELECTED == NXP_K24 )
      *temp = (int16_t)ADC_Get_uP_Temperature((bool)false);
#elif ( MCU_SELECTED == RA6E1 )
      *temp = 0; // TODO: RA6E1 Bob: this needs to use the new method of getting the Si446x temperature even if SD is running
      return (bool)false;
#endif
   }

   // Sanitize output
   if ( *temp < TEMPERATURE_MIN ) {
      *temp = TEMPERATURE_MIN;
   } else if ( *temp > TEMPERATURE_MAX ) {
      *temp = TEMPERATURE_MAX;
   }
   return (bool)true;
}

/*!
 *  Used to get the transmit buffer
 *
 */
TX_FRAME_t *RADIO_Transmit_Buffer_Get(void)
{
   return (&radio[RADIO_0].buf.TxBuffer);
}

/*!
 * Radio Driver
 */
struct radio_driver Radio =
{
   .Init               = Init,
   .StartRx            = StartRx,
   .SleepRx            = SleepRx,
   .SendData           = SendData,
   .ReadData           = ReadData,
   .Do_CCA             = Do_CCA,
   .Standby            = Standby,
   .StandbyTx          = StandbyTx,
   .StandbyRx          = StandbyRx,
   .Ready              = Ready,
   .ReadyTx            = ReadyTx,
   .ReadyRx            = ReadyRx,
   .Shutdown           = Shutdown,
};

#if ( EP == 1 )
bool ProcessSRFNHeaderPBytes(unsigned const char headerBytes[], unsigned int* lengthOfPayload)
{
   bool headerValid;

   memcpy(radio[RADIO_0].RadioBuffer, headerBytes, PHY_HEADER_SIZE);
   headerValid = validateSrfnHeader((uint8_t)RADIO_0);
   *lengthOfPayload = HeaderLength_Get(ePHY_FRAMING_0, radio[RADIO_0].RadioBuffer);
   return headerValid;
}

bool ProcessSRFNPayloadPBytes(unsigned const char payloadBytes[], unsigned short numberOfPayloadBytes)
{
   memcpy(&radio[RADIO_0].RadioBuffer[PHY_HEADER_SIZE], payloadBytes, numberOfPayloadBytes);
   validatePhyPayload((uint8_t)RADIO_0);

   // wait for the phy task to finish processing with the bytes before we unlock the mutex ( happens outside of function call)
   // to allow other tasks write to the radio buffers
   //INFO_printf("Wait for sync+payload");
   (void)OS_EVNT_Wait(&SD_Events, SOFT_DEMOD_PROCESSED_PAYLOAD_BYTES, (bool)false, 200);
   //INFO_printf("Received sync+payload");

   /* always returns false */
   return false;
}
#endif

void RADIO_Lock_Mutex(void)
{
   OS_MUTEX_Lock(&radioMutex);
}

void RADIO_UnLock_Mutex(void)
{
   OS_MUTEX_Unlock(&radioMutex);
}

#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
/*
   Function name: getVSWRvalue

   Purpose: Return requested VSWR related value

   Arguments:  enum meterReadingType (fngVSWR, fngForwardPower, or fngReflectedPower only)

   Returns: Current value
*/
float getVSWRvalue( meterReadingType value )
{
   float retVal;
   switch ( value )  /*lint !e788 not all enums included in switch   */
   {
      case fngVSWR            :   retVal = cachedVSWR;            break;
      case fngForwardPower    :   retVal = cachedForwardVolts;    break;
      case fngReflectedPower  :   retVal = cachedReflectedVolts;  break;
      default                 :   retVal = 0.0f;                  break;
   }  /*lint !e788 not all enums included in switch   */

   return retVal * 10.0f;

}
/*
   Function name: computeAverage

   Purpose: Compute average of an array of floats;

   Arguments:  float    *samples - array of input values
               uint32_t numSamples - number of values in the array.

   Returns: average of the input values
*/
static float computeAverage( float const *samples, uint32_t numSamples )
{
   float       answer = 0.0f;
   uint32_t    sampleCount = numSamples;

   while ( sampleCount-- )
   {
      answer += *samples++;
   }
   return answer / numSamples;
}
#endif
