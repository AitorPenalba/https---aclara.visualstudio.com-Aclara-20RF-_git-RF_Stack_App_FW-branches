/***********************************************************************************************************************
 *
 * Filename: PHY.c
 *
 * Global Designator: PHY_
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2018-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>

#include "buffer.h"
#include "DBG_SerialDebug.h"
#include "PHY_Protocol.h"        /* PHY Layer API */
#include "time_sys.h"
#include "radio.h"               /* Generic Radio API */
#include "..\Radio\Si446x_Radio\xradio.h"
#include "radio_hal.h"
#include "si446x_api_lib.h"
#include "si446x_cmd.h"
#include "PHY.h"                 /* PHY API */
#include "MAC.h"
#include "rs.h"
#include "time_util.h"
#include "timer_util.h"
#include "sys_clock.h"
#include "pwr_task.h"
#if ( EP == 1 )
#include "pwr_last_gasp.h"       /* Last Gasp API */ // Not supported on DCU
#include "SoftDemodulator.h"
#endif
#include "rsfec.h"
#include "file_io.h"
#if ( DCU == 1 )
#include "MAINBD_Handler.h" //DCU2+
#include "version.h"
#endif
#include "math.h"
#include "SM_Protocol.h"
#include "SM.h"
#include "BSP_aclara.h"
#include "DBG_CommandLine.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
// PHY Events
#define PHY_MESSAGE_PENDING        0x00000001   /*!< PHY Message     Pending */
#define PHY_RADIO_TX_DONE_BASE       1 // Event bit 1
#define PHY_RADIO_RX_DATA_BASE       2 // Event bit 2 to 10
#define PHY_RADIO_CTS_LINE_LOW_BASE 11 // Event bit 11 to 19
#define PHY_RADIO_INT_PENDING_BASE  20 // Event bit 20 to 28. Interrupt pending bit 20 for radio 0 and bit 21 for all RX radios on TB 101-9975T but bits 21 to 28 for all RX radioes on TB 101-9985T
#if ( RTOS_SELECTION == MQX_RTOS )
#define PHY_EVENT_MASK             0xFFFFFFFF
#elif ( RTOS_SELECTION == FREE_RTOS )
#define PHY_EVENT_MASK             0x00108007 /* FreeRTOS only allows use of the lower 24 bits for event flags; we only need bits 0, 1, 2, 11 and 20 for an end-point with 1 radio */
#if ( EP != 1 )
#error "Attempt to compile with FreeRTOS on something other than an end-point; EP != 1"
#endif
#endif

#define PHY_CONFIG_FILE_UPDATE_RATE ((uint32_t)0xFFFFFFFF)/* File updates every now and then */

/*lint -esym(750,PHY_AFC_AFC_ENABLE_DEFAULT,PHY_AFC_RSSI_THRESHOLD_DEFAULT,PHY_AFC_RSSI_THRESHOLD_MIN)   */
/*lint -esym(750,PHY_AFC_RSSI_THRESHOLD_MAX,PHY_AFC_SAMPLECNT_THRESHOLD_DEFAULT,PHY_AFC_TEMPERATURE_MIN_DEFAULT)  */
/*lint -esym(750,PHY_AFC_TEMPERATURE_MAX_DEFAULT,PHY_AFC_TEMPERATURE_MIN,PHY_AFC_TEMPERATURE_MAX) */
/*lint -esym(750,RPT_PRNT_INFO,RECEIVE_POWER_MARGIN_MAX,PHY_HARD_DEMODULATOR,PHY_SOFT_DEMODULATOR,BKRSSI_MIN_SAMPLES) */
#define PHY_AFC_AFC_ENABLE_DEFAULT          true
#define PHY_AFC_RSSI_THRESHOLD_DEFAULT      -115
#define PHY_AFC_RSSI_THRESHOLD_MIN          -200
#define PHY_AFC_RSSI_THRESHOLD_MAX            55
#define PHY_AFC_SAMPLECNT_THRESHOLD_DEFAULT  100
#define PHY_AFC_TEMPERATURE_MIN_DEFAULT      -30
#define PHY_AFC_TEMPERATURE_MAX_DEFAULT      +70
#define PHY_AFC_TEMPERATURE_MIN              -50
#define PHY_AFC_TEMPERATURE_MAX             +100
#if ( DCU == 1 )
// On TBs, TCXO frequency doesn't change much so we can have a small threshold
#define PHY_AFC_ADJUSTMENT_THRESHOLD           5 // Update flash when TCXO is more than 5Hz off (i.e. .16ppm)
#else
// On EPs, TCXO is trimmed by averaging many DCUs.
// I don't have a good feeling for how much the adjusment can move in time.
// This wouldn't be a problem if there was a way to only use trimmed DCU2+.
// In the mean time, I'll use a wider threshold to avoid wearing the Flash prematurely.
#define PHY_AFC_ADJUSTMENT_THRESHOLD          30 // Update flash when TCXO is more than 30Hz off (i.e. 1ppm)
#endif

#define PHY_CCA_ADAPTIVE_THRESHOLD_ENABLE_DEFAULT true /*!< Indicates if the CCA threshold will be dynamically adjusted for each channel */

#define PHY_FRONT_END_GAIN_DEFAULT             0  /*!( Default correction gain */

#define PHY_NOISE_ESTIMATE_COUNT_DEFAULT       8  /*!< Number of measurements used for average */

#if ( DCU == 1 )
#define PHY_VIRTUAL_TEMP_REFRESH_RATE          TEN_MSEC  // Computation rate of the virtual temperature
#if ( ( TENTH_SEC >= PHY_VIRTUAL_TEMP_REFRESH_RATE ) && ( TENTH_SEC % PHY_VIRTUAL_TEMP_REFRESH_RATE ) == 0 )
#define PHY_VIRTUAL_TEMP_PA_REFRESH_RATE       (TENTH_SEC/PHY_VIRTUAL_TEMP_REFRESH_RATE) // PA temp reading rate is 0.1 sec
#else
#error "PHY_VIRTUAL_TEMP_REFRESH_RATE must evenly divide one second"
#endif
#define PHY_VIRTUAL_TEMP_RISE_PER_FRAME       ((1.155f/149.0f)*(float)PHY_MAX_FRAME)    // degC rise per frame. The original number was computed from a 149 byte SRFN frame.
#define PHY_VIRTUAL_TEMP_RISE_PER_BYTE        (PHY_VIRTUAL_TEMP_RISE_PER_FRAME/149.0f)  // degC rise per byte. The original number was computed from a 149 byte SRFN frame.
#define PHY_VIRTUAL_TEMP_RISE_PER_MS          (PHY_VIRTUAL_TEMP_RISE_PER_FRAME/150.83f) // degC rise per ms. The original number was computed from a 150.8ms SRFN frame.
#define PHY_VIRTUAL_TEMP_TRANSITION_POINT      7.0f      // Transition point in degC to switch between fast and slow rate
#define PHY_VIRTUAL_TEMP_FAST_DROP             2.53f     // Drop in degC for the fast part of the slope per second
#define PHY_VIRTUAL_TEMP_SLOW_DROP             0.6f      // Drop in degC for the slow part of the slope per second
#define PHY_VIRTUAL_TEMP_LIMIT                 91.0f     // Temperature limit to not exceed. Needs to be defined
#define PHY_VIRTUAL_TEMP_MAX                   125.0f    // Maximum vTemp used for sanity check
#define PHY_THERMAL_DELAY_DEFAULT              50        // Tell MAC to wait at least 50 ms when Thermal Protection Engaged is TRUE but we don't have an explicit delay
#endif

#define PHY_MIN_RX_BYTES                       0         // The minimum number of bytes for the PHY to read in OTA after a sync word is detected

#define PHY_HARD_DEMODULATOR                   (uint8_t)0
#define PHY_SOFT_DEMODULATOR                   (uint8_t)1
#define BKRSSI_MIN_SAMPLES                     (uint8_t)20     /* Min number of samples required to calculate Rx Power Threshold */

#define RECEIVE_POWER_MARGIN_MAX               (uint8_t)127

#define ENABLE_PRNT_PHY_RPT_INFO                1                 /* 1 = Enables the Receive Power Threshold Print information
                                                                     Note: Might be wise to turn this off for Production Release. */

#if ( ENABLE_PRNT_PHY_RPT_INFO == 1 )
#define RPT_PRNT_INFO( category, fmt, ... )    DBG_log( category, ADD_LF|PRINT_DATE_TIME, fmt, ##__VA_ARGS__)
#else
#define RPT_PRNT_INFO( category, fmt, ... )
#endif

#if( RTOS_SELECTION == FREE_RTOS )
#define PHY_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define PHY_NUM_MSGQ_ITEMS 0
#endif


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
static struct
{
   PHY_STATE_e            state;
   PHY_IndicationHandler  pIndicationHandler;
} _phy = {
   .state = ePHY_STATE_SHUTDOWN,
   .pIndicationHandler  = NULL,
};

static struct {
  uint32_t lengthInBytes; // Frame length in byte
  uint32_t lengthInMs;    // Frame length in msec
  float vTemp;            // Current PA virtual temperature
} virtualTemperature;

// Data to keep in raw memory (data that doesn't change often)
// WARNING: If a variable is added or removed, Process_ResetReq might need to be updated
typedef struct
{
   int16_t  AfcAdjustment[PHY_RCVR_COUNT];   /*!< The amount of frequency adjustment made by the AFC algorithm in units of aMinFreqStep. */
#if ( EP == 1 )
   bool     AfcEnable;                       /*!< */
   int16_t  AfcRssiThreshold;                /*!< */
   uint16_t AfcSampleCount;                  /*!< */
   int8_t   AfcTemperatureRange[2];          /*!< */
#endif
   int16_t  CcaThreshold;                    /*!< The threshold against which the current RSSI is compared to determine if the channel is idle or busy.  (-200 to +55) */
   uint8_t  NoiseEstimateRate;               /*!< How often in minutes that noise measurements are taken */
   uint8_t  RssiJumpThreshold;               /*!< The threshold in dB for the RSSI change above which the receiver will reset and reacquire the newer signal */
   uint16_t RxChannels[PHY_RCVR_COUNT];      /*!< List of the Receive Channels */
   uint16_t TxChannels[PHY_MAX_CHANNEL];     /*!< List of the Transmit Channels */
   uint16_t Channels[  PHY_MAX_CHANNEL];     /*!< List of all channels supported by this device */
   uint8_t  PowerSetting;                    /*!< Transmit power setting */
   uint8_t  NoiseEstimateCount;              /*!< The number of valid noise readings the are averaged together for the noise estimation */
   bool     CcaAdaptiveThresholdEnable;      /*!< Indicates if the CCA threshold will be dynamically adjusted for each channel */
   uint8_t  CcaOffset;                       /*!< CCA offset added to noise floor */
   int8_t   FrontEndGain;                    /*!< The amount af receive gain between the output of the antenna and the input of the receiver in 0.5 dBm increment */
   PHY_DETECTION_e RxDetectionConfig[PHY_RCVR_COUNT]; /*!< The detection configuration the PHY will use on the corresponding receiver  */
   PHY_FRAMING_e   RxFramingConfig[PHY_RCVR_COUNT];   /*!< The framing configuration the PHY will use on the corresponding receiver  */
   PHY_MODE_e      RxMode[PHY_RCVR_COUNT];            /*!< The PHY mode used on the corresponding receiver  */
   uint16_t MinRxBytes;                      /*!< The minimum number of bytes for the PHY to read in OTA after a sync word is detected. */
   bool     ThermalControlEnable;            /*!< Indicates if the thermal control feature is enabled (TRUE) or disabled (FALSE). */
   bool     ThermalProtectionEnable;         /*!< Indicates if the thermal protection feature is enabled (TRUE) or disabled (FALSE). */
   float    TxOutputPowerMax;                /*!( The maximum acceptable power level in dBm for the transmitter at the device output. */
   float    TxOutputPowerMin;                /*!( The maximum acceptable power level in dBm for the transmitter at the device output. */
   bool     VerboseMode;                     /*!< Indicates if verbose mode is turned on causing DATA_VERBOSE.indications to be sent upon receipt of OTA frames. */
   uint8_t  Demodulator[PHY_RCVR_COUNT];     /*!< List of demodulator used by the each respective receiver. */
   uint16_t MaxTxPayload;                    /*!( The maximum number of bytes transmited in the PHY payload */
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   uint8_t  ReceivePowerMargin;
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   uint16_t fngVswrNotificationSet;          /*!< VSWR limit controlling notification to HE  */
   uint16_t fngVswrShutdownSet;              /*!< VSWR limit controlling transmitter shutdown  */
   uint16_t fngForwardPowerSet;              /*!< Desired Forward Power setting  */
   uint16_t fngFowardPowerLowSet;            /*!< Forward Power limit controlling shutdown of transmitter */
   uint8_t  fngfemMfg;                       /*!< Manufacturer of FEM hardware   */
#endif
#if ( DCU == 1 )
   bool     txEnabled;                       /*!< Determines whether board is allowed to transmit. */
#endif
#if ( TEST_SYNC_ERROR == 1 )
   uint8_t  SyncError;                       /*!< SYNC bit error allowed */
#endif
}PhyConfigAttr_t;

// This should really be in cache partition because it can change often but we don't have space anymore so this shadows some of the config (RAW) partition data.
// It's a poor's man cache driver.
typedef struct
{
   int16_t AfcAdjustment[PHY_RCVR_COUNT];   /*!< The amount of frequency adjustment made by the AFC algorithm in units of aMinFreqStep. */
}PhyShadowAttr_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* These are used for printing messages */
static const char * const phy_attr_names[] =
{
   [ePhyAttr_AfcAdjustment           ] = "AfcAdjustment",
#if ( EP == 1 )
   [ePhyAttr_AfcEnable               ] = "AfcEnable",
   [ePhyAttr_AfcRssiThreshold        ] = "AfcRssiThreshold",
   [ePhyAttr_AfcTemperatureRange     ] = "AfcTemperatureRange",
#endif
   [ePhyAttr_AvailableChannels       ] = "AvailableChannels",
   [ePhyAttr_CcaAdaptiveThreshold    ] = "CcaAdaptiveThreshold",
   [ePhyAttr_CcaAdaptiveThresholdEnable ] = "CcaAdaptiveThresholdEnable",
   [ePhyAttr_CcaOffset               ] = "CcaOffset",
   [ePhyAttr_CcaThreshold            ] = "CcaThreshold",
   [ePhyAttr_DataRequestCount        ] = "DataRequestCount",
   [ePhyAttr_DataRequestRejectedCount] = "DataRequestRejectedCount",
   [ePhyAttr_Demodulator             ] = "Demodulator",
   [ePhyAttr_FailedFrameDecodeCount  ] = "FailedFrameDecodeCount",
   [ePhyAttr_FailedHcsCount          ] = "FailedHcsCount",
   [ePhyAttr_FailedHeaderDecodeCount ] = "FailedHeaderDecodeCount",
   [ePhyAttr_FailedTransmitCount     ] = "FailedTransmitCount",
   [ePhyAttr_FramesReceivedCount     ] = "FramesReceivedCount",
   [ePhyAttr_FramesTransmittedCount  ] = "FramesTransmittedCount",
   [ePhyAttr_FrontEndGain            ] = "FrontEndGain",
   [ePhyAttr_LastResetTime           ] = "LastResetTime",
   [ePhyAttr_MalformedHeaderCount    ] = "MalformedHeaderCount",
   [ePhyAttr_MinRxBytes              ] = "MinRxBytes",
   [ePhyAttr_MaxTxPayload            ] = "MaxTxPayload",
   [ePhyAttr_NoiseEstimate           ] = "NoiseEstimate",
#if ( EP == 1 )
   [ePhyAttr_NoiseEstimateBoostOn    ] = "NoiseEstimateBoostOn",
#endif
   [ePhyAttr_NoiseEstimateCount      ] = "NoiseEstimateCount",
   [ePhyAttr_NoiseEstimateRate       ] = "NoiseEstimateRate",
   [ePhyAttr_NumChannels             ] = "NumChannels",
   [ePhyAttr_PowerSetting            ] = "PowerSetting",
   [ePhyAttr_PreambleDetectCount     ] = "PreambleDetectCount",
   [ePhyAttr_RadioWatchdogCount      ] = "RadioWatchdogCount",
   [ePhyAttr_RssiJumpCount           ] = "RssiJumpCount",
   [ePhyAttr_RssiJumpThreshold       ] = "RssiJumpThreshold",
#if ( DCU == 1 )
   [ePhyAttr_RssiStats10m            ] = "RssiStats10m",
#if ( MAC_LINK_PARAMETERS == 1 )
   [ePhyAttr_RssiStats10m_Copy       ] = "RssiStats10mCopy",
#endif
   [ePhyAttr_RssiStats60m            ] = "RssiStats60m",
#endif
   [ePhyAttr_RxAbortCount            ] = "RxAbortCount",
   [ePhyAttr_RxChannels              ] = "RxChannels",
   [ePhyAttr_RxDetectionConfig       ] = "RxDetectionConfig",
   [ePhyAttr_RxFramingConfig         ] = "RxFramingConfig",
   [ePhyAttr_RxMode                  ] = "RxMode",
   [ePhyAttr_State                   ] = "State",
   [ePhyAttr_SyncDetectCount         ] = "SyncDetectCount",
#if ( TEST_SYNC_ERROR == 1 )
   [ePhyAttr_SyncError               ] = "SyncError",
#endif
   [ePhyAttr_Temperature             ] = "RadioTemperature",
   [ePhyAttr_ThermalControlEnable    ] = "ThermalControlEnable",
   [ePhyAttr_ThermalProtectionCount  ] = "ThermalProtectionCount",
   [ePhyAttr_ThermalProtectionEnable ] = "ThermalProtectionEnable",
   [ePhyAttr_ThermalProtectionEngaged] = "ThermalProtectionEngaged",
   [ePhyAttr_TxAbortCount            ] = "TxAbortCount",
   [ePhyAttr_TxChannels              ] = "TxChannels",
   [ePhyAttr_TxOutputPowerMax        ] = "TxOuputPowerMax",
   [ePhyAttr_TxOutputPowerMin        ] = "TxOuputPowerMin",
   [ePhyAttr_VerboseMode             ] = "VerboseMode",
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   [ePhyAttr_ReceivePowerMargin      ] = "ReceivePowerMargin",
   [ePhyAttr_ReceivePowerThreshold   ] = "ReceivePowerThreshold",
#endif
#if ( ( DCU == 1 ) && (VSWR_MEASUREMENT == 1) )
   [ePhyAttr_fngVSWR                 ] = "fngVSWR",
   [ePhyAttr_fngVswrNotificationSet  ] = "fngVswrNotificationSet",
   [ePhyAttr_fngVswrShutdownSet      ] = "fngVswrShutdownSet",
   [ePhyAttr_fngForwardPower         ] = "fngForwardPower",
   [ePhyAttr_fngReflectedPower       ] = "fngReflectedPower",
   [ePhyAttr_fngForwardPowerSet      ] = "fngForwardPowerSet",
   [ePhyAttr_fngFowardPowerLowSet    ] = "fngFowardPowerLowSet",
   [ePhyAttr_fngfemMfg               ] = "fngFemManufacturer",
#endif
#if ( DCU == 1 )
   [ePhyAttr_txEnabled               ] = "txEnabled",
#endif
};


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static bool         PHY_Initialized = (bool)false;
static OS_MSGQ_Obj  PHY_msgQueue;       // message queue
static OS_MUTEX_Obj PHY_AttributeMutex_;
static OS_MUTEX_Obj PHY_Mutex_;
static OS_SEM_Obj   PHY_AttributeSem_;

static buffer_t *pRequestBuf     = NULL; // Handle to unconfirmed request buffers
static buffer_t *pDataRequestBuf = NULL; // Handle to unconfirmed data request buffers

static OS_EVNT_Obj  events;             // PHY Events

static FileHandle_t      PHYcachedFileHndl_ = {0};    /*!< Contains the file handle information. */

static PhyConfigAttr_t ConfigAttr =
{
#if ( DCU == 1 )
   .AfcAdjustment          = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
#else
   .AfcAdjustment          = { 0 },
   .AfcEnable              = PHY_AFC_AFC_ENABLE_DEFAULT,
   .AfcRssiThreshold       = PHY_AFC_RSSI_THRESHOLD_DEFAULT,
   .AfcSampleCount         = PHY_AFC_SAMPLECNT_THRESHOLD_DEFAULT,
   .AfcTemperatureRange    = {PHY_AFC_TEMPERATURE_MIN_DEFAULT, PHY_AFC_TEMPERATURE_MAX_DEFAULT},
#endif
   .CcaThreshold           = PHY_CCA_THRESHOLD_DEFAULT,
   .NoiseEstimateRate      = PHY_NOISE_ESTIMATE_RATE_DEFAULT,
   .RssiJumpThreshold      = PHY_RSSI_JUMP_THRESHOLD_DEFAULT,
#if ( DCU == 1 )
   // On the DCU the first rx channel is set to listen to the outbound channel
   // Note: RADIO 0 doesn't have a receiver on this hardware.
   .RxChannels = { PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL },
#else
   .RxChannels = { PHY_INVALID_CHANNEL },
#endif
   .TxChannels = { PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL,
                   PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL,
                   PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL,
                   PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL },
   .Channels   = { PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL,
                   PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL,
                   PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL,
                   PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL, PHY_INVALID_CHANNEL },
   .PowerSetting           = POWER_LEVEL_DEFAULT,

   .NoiseEstimateCount     = PHY_NOISE_ESTIMATE_COUNT_DEFAULT,
   .CcaAdaptiveThresholdEnable = PHY_CCA_ADAPTIVE_THRESHOLD_ENABLE_DEFAULT,
   .CcaOffset              = PHY_CCA_OFFSET_DEFAULT,
   .FrontEndGain           = PHY_FRONT_END_GAIN_DEFAULT,
#if ( DCU == 1 )
   .RxDetectionConfig      = { ePHY_DETECTION_0, ePHY_DETECTION_0, ePHY_DETECTION_0, ePHY_DETECTION_0, ePHY_DETECTION_0, ePHY_DETECTION_0, ePHY_DETECTION_0, ePHY_DETECTION_0, ePHY_DETECTION_0 },
   .RxFramingConfig        = { ePHY_FRAMING_0, ePHY_FRAMING_0, ePHY_FRAMING_0, ePHY_FRAMING_0, ePHY_FRAMING_0, ePHY_FRAMING_0, ePHY_FRAMING_0, ePHY_FRAMING_0, ePHY_FRAMING_0 },
   .RxMode                 = { ePHY_MODE_1, ePHY_MODE_1, ePHY_MODE_1, ePHY_MODE_1, ePHY_MODE_1, ePHY_MODE_1, ePHY_MODE_1, ePHY_MODE_1, ePHY_MODE_1 },
#else
   .RxDetectionConfig      = { ePHY_DETECTION_0 },
   .RxFramingConfig        = { ePHY_FRAMING_0 },
   .RxMode                 = { ePHY_MODE_1 },
#endif
   .MinRxBytes             = PHY_MIN_RX_BYTES,
   .ThermalControlEnable   = TRUE,
   .ThermalProtectionEnable= TRUE,
   .TxOutputPowerMax       = PHY_TX_OUTPUT_POWER_MAX,
   .TxOutputPowerMin       = PHY_TX_OUTPUT_POWER_MIN,
   .VerboseMode            = FALSE,
#if ( DCU == 1 )
   .Demodulator            = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
#else
   .Demodulator            = { 0 },
#endif
   .MaxTxPayload           = PHY_DEFAULT_TX_PAYLOAD,
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   .ReceivePowerMargin     = RECEIVE_POWER_MARGIN_DEFAULT,
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   .fngVswrNotificationSet = 50U,                  /* 5.0:1 VSWR generates alarm */
   .fngVswrShutdownSet     = 60U,                  /* 6.0:1 VSWR shuts down the transmitter */
   .fngForwardPowerSet     = 320U,                 /* Default output power is 32 dBm */
   .fngFowardPowerLowSet   = 300U,                 /* Measured output power below 30 dBM generates an alarm */
   .fngfemMfg              = MINI_CIRCUITS,        /* Default to MiniCircuits */
#endif
#if ( DCU == 1 )
   .txEnabled              = (bool)true,
#endif
#if ( TEST_SYNC_ERROR == 1 )
   .SyncError              = 0,
#endif
};

static PHY_Confirm_t   PhyConfMsg;                 /*!< Static buffer used for confirmation message */
static PHY_Confirm_t   PhyDataConfMsg;             /*!< Static buffer used for data confirmation message */

static FileHandle_t    PHYconfigFileHndl_ = {0};   /*!< Contains the file handle information. */

static PhyCachedAttr_t CachedAttr;                 /*!< PHY data saved in cached memory */

static PhyShadowAttr_t ShadowAttr;                 /*!< PHY data shadowed */

static bool ChannelAdded;                          /*!< Used to kick off noise floor computation went a channel is added */
#if ( DCU == 1 )
static bool ComputeRssiStats;                                  /*!< Used by callback to kick off RSSI stats computation */
static PHY_RssiStats_s PhyRssiStats10m[PHY_RCVR_COUNT];        /*!< RSSI Avg statistics for reporting RSSI peak and average for Uplink Power Control (ALC + NextGen) */
#if ( MAC_LINK_PARAMETERS == 1 )
static PHY_RssiStats_s PhyRssiStats10m_Copy_[PHY_RCVR_COUNT];  /*!< Copy of the PhyRssiStats10m used for MAC-Link Parameters */
#endif
static PHY_RssiStats_s PhyRssiStats60m[PHY_RCVR_COUNT];        /*!< RSSI Avg statistics for reporting RSSI peak and average for noise characterization */
#endif

extern const uint8_t GenILength[];                 /*!< Used for length size of Gen I messages */

static uint16_t NoiseEstimateRateTmrID;            /*!< Handle to access the noise estimate rate */

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

static bool Process_CCAReq(   PHY_Request_t const *pReq);
static bool Process_GetReq(   PHY_Request_t const *pReq);
static bool Process_SetReq(   PHY_Request_t const *pReq);
static bool Process_StartReq( PHY_Request_t const *pReq);
static bool Process_StopReq(  PHY_Request_t const *pReq);
static bool Process_ResetReq( PHY_Request_t const *pReq);
static bool Process_DataReq(  PHY_Request_t const *pReq);

static void PHY_Request(buffer_t *request);

static void     HeaderVersion_Set(uint8_t *pData, uint8_t version);
static void     HeaderMode_Set(uint8_t    *pData, PHY_MODE_e mode);

static void     HeaderParityCheck_Calc(uint8_t const *pData);

static uint16_t HeaderCheckSequence_Calc(uint8_t     const *pData);
static void     HeaderCheckSequence_Set(uint8_t            *pData, uint16_t hcs);
static uint16_t HeaderCheckSequence_Get(uint8_t      const *pData);

static uint16_t FrameParityCheck_Calc(uint8_t *pData, uint8_t num_bytes, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters);

//PHY_SET_STATUS_e PHY_Attribute_Set( PHY_SetReq_t const *pSetReq); // Prototype should be only used by phy.c and radio.c only.
//                                                                  // We don't want outsiders to call this function directly.
//                                                                  // PHY_SetRequest is used for that.
//PHY_GET_STATUS_e PHY_Attribute_Get( PHY_GetReq_t const *pGetReq, PHY_ATTRIBUTES_u *val); // Prototype should be only used by phy.c and radio.c only.
//                                                                                         // We don't want outsiders to call this function directly.
//                                                                                         // PHY_GetRequest is used for that.

static void SendConfirm(PHY_Confirm_t const *pConf, buffer_t const *pReqBuf);

static void Indication_Send(buffer_t *pBuf);

#if ( RTOS_SELECTION == MQX_RTOS )
static void RadioEvent_Set(RADIO_EVENT_t event_type, uint8_t chan);
#elif ( RTOS_SELECTION == FREE_RTOS )
static void RadioEvent_Set(RADIO_EVENT_t event_type, uint8_t chan, bool fromISR);
#else
#error "RTOS_SELECTION is invalid!"
#endif
static void RadioEvent_TxDone(uint8_t chan);
static void RadioEvent_RxData(uint8_t radioNum);
static void RadioEvent_CTSLineLow(uint8_t radioNum);

static char* PHY_StateMsg(PHY_STATE_e state);

static void PHY_TestMode(void);
static uint8_t NumChannels_Get(void);

#if ( DCU == 1 )
static uint32_t virtualTemperature_Delay( void );
static void PHY_UpdateRssiStats(PHY_RssiStats_s RssiStats[], const uint32_t i, const uint8_t newRSSI);
#endif

static void Reset_ReceiveStats( void );
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
static PHY_GET_STATUS_e Demodulator_SINR_Get( uint8_t *demodulatorSINR );
static PHY_SET_STATUS_e Receive_Power_Margin_Set( uint8_t rpm );
static uint8_t          Receive_Power_Margin_Get( void );
static PHY_GET_STATUS_e Receive_Power_Threshold_Calculate( float *RxPowerThreshold );
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
static void  phyVSWRTimeout( uint8_t param, void *ptr );

#if 1
#define VSWR_TIMEOUT (15U * TIME_TICKS_PER_MIN) /* time out of 15 minutes   */
#else
#define VSWR_TIMEOUT (15U * TIME_TICKS_PER_SEC) /* time out of 15 seconds - for testing only!!!!   */
#endif
static timer_t          VSWRtimerCfg;           /* rfTestMode timeout Timer configuration */
#endif

static void writeCache( void );
static void writeConfig( void );

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION DEFINITIONS */


/***********************************************************************************************************************

 Function Name: PHY_init

 Purpose: This function is called before starting the scheduler. Creates all resources needed by this task.

 Arguments: none

 Returns: eSUCCESS/eFAILURE indication

***********************************************************************************************************************/
returnStatus_t PHY_init( void )
{
   returnStatus_t retVal = eFAILURE;
   FileStatus_t fileStatus;
   uint32_t i;
   //TODO RA6: NRJ: determine if semaphores need to be counting
   if ( OS_SEM_Create(&PHY_AttributeSem_, 0) && OS_MUTEX_Create(&PHY_AttributeMutex_) && OS_MUTEX_Create(&PHY_Mutex_) &&
        OS_EVNT_Create(&events) && OS_MSGQ_Create(&PHY_msgQueue, PHY_NUM_MSGQ_ITEMS, "PHY") )
   {
#if 0 // TODO: attempt to get through PHY_init without a working file system
      // Open PHY cached data (mainly counters)
      if ( eSUCCESS == FIO_fopen(&PHYcachedFileHndl_,              /* File Handle so that PHY access the file. */
                                 ePART_SEARCH_BY_TIMING,           /* Search for the best partition according to the timing. */
                                 (uint16_t)eFN_PHY_CACHED,         /* File ID (filename) */
                                 (lCnt)sizeof(PhyCachedAttr_t),    /* Size of the data in the file. */
                                 FILE_IS_NOT_CHECKSUMED,           /* File attributes to use. */
                                 0,                                /* The update rate of the data in the file. */
                                 &fileStatus)) {                   /* Contains the file status */
         if ( fileStatus.bFileCreated )
         {  //File was created for the first time, set defaults.
#endif // 0
            (void)memset(&CachedAttr, 0, sizeof(CachedAttr));
            (void)TIME_UTIL_GetTimeInSecondsFormat(&CachedAttr.LastResetTime);
            for (i=0; i<PHY_MAX_CHANNEL; i++)
            {
               CachedAttr.NoiseEstimate[i]        = PHY_CCA_THRESHOLD_MAX;
#if ( EP == 1 )
               CachedAttr.NoiseEstimateBoostOn[i] = PHY_CCA_THRESHOLD_MAX;
#endif
            }
#if 0 // TODO: attempt to get through PHY_init without a working file system
            retVal = FIO_fwrite( &PHYcachedFileHndl_, 0, (uint8_t*)&CachedAttr, sizeof(CachedAttr));
         }
         else
         {
            retVal = FIO_fread( &PHYcachedFileHndl_, (uint8_t*)&CachedAttr, 0, sizeof(CachedAttr));
         }
         if (eSUCCESS == retVal)
         {
#endif
            // Open PHY config data (configuration data that doesn't change often)
            if ( eSUCCESS == FIO_fopen(&PHYconfigFileHndl_,              /* File Handle so that PHY access the file. */
                                       ePART_NV_APP_CACHED, // TODO: RA6E1 ePART_SEARCH_BY_TIMING,           /* Search for the best partition according to the timing. */
                                       (uint16_t)eFN_PHY_CONFIG,         /* File ID (filename) */
                                       (lCnt)sizeof(ConfigAttr),         /* Size of the data in the file. */
                                       FILE_IS_NOT_CHECKSUMED,           /* File attributes to use. */
                                       PHY_CONFIG_FILE_UPDATE_RATE,      /* The update rate of the data in the file. */
                                       &fileStatus) )                    /* Contains the file status */
            {
               if (1) // ( fileStatus.bFileCreated ) // TODO: RA6E1 Bob: temporarily ALWAYS initialize PHY config to defaults
               {  // File was created for the first time, set defaults.
                  retVal = FIO_fwrite( &PHYconfigFileHndl_, 0, (uint8_t*)&ConfigAttr, sizeof(ConfigAttr));
               }
               else
               {
                  retVal = FIO_fread( &PHYconfigFileHndl_, (uint8_t*)&ConfigAttr, 0, sizeof(ConfigAttr));
               }
               if (eSUCCESS == retVal)
               {
                  PHY_Initialized = (bool)true;
               }
               // Update shadow data
               (void)memcpy( ShadowAttr.AfcAdjustment, ConfigAttr.AfcAdjustment, sizeof( ShadowAttr.AfcAdjustment ));
            }
#if 0 // TODO: attempt to get through PHY_init without a working file system
         }
      }
#endif
   }
#if ( EP == 1 )
   if (retVal == eSUCCESS)
   {
#if 0 // TODO: RA6E1 Bob: temporarily without soft demodulator
      retVal = SoftDemodulator_Initialize();
#endif // 0
   }
#endif
   return(retVal);
}

typedef void (*EventHandler)( uint8_t chan );


/* Radio Event Handler Table */
static struct{
   EventHandler fxn;
   uint8_t      radio_num;
}const radio_events[32] = {
  { NULL,                    0 } ,  // 0x01 <<  0  PHY_MESSAGE_PENDING has it's own handler
  { RadioEvent_TxDone,       0 } ,  // 0x01 <<  1

  { RadioEvent_RxData,       0 } ,  // 0x01 <<  2  PHY_RADIO_RX_DATA_BASE     +0
  { RadioEvent_RxData,       1 } ,  // 0x01 <<  3  PHY_RADIO_RX_DATA_BASE     +1
  { RadioEvent_RxData,       2 } ,  // 0x01 <<  4  PHY_RADIO_RX_DATA_BASE     +2
  { RadioEvent_RxData,       3 } ,  // 0x01 <<  5  PHY_RADIO_RX_DATA_BASE     +3
  { RadioEvent_RxData,       4 } ,  // 0x01 <<  6  PHY_RADIO_RX_DATA_BASE     +4
  { RadioEvent_RxData,       5 } ,  // 0x01 <<  7  PHY_RADIO_RX_DATA_BASE     +5
  { RadioEvent_RxData,       6 } ,  // 0x01 <<  8  PHY_RADIO_RX_DATA_BASE     +6
  { RadioEvent_RxData,       7 } ,  // 0x01 <<  9  PHY_RADIO_RX_DATA_BASE     +7
  { RadioEvent_RxData,       8 } ,  // 0x01 << 10  PHY_RADIO_RX_DATA_BASE     +8

  { RadioEvent_CTSLineLow,   0 } ,  // 0x01 << 11  PHY_RADIO_CTS_LINE_LOW     +0
  { RadioEvent_CTSLineLow,   1 } ,  // 0x01 << 12  PHY_RADIO_CTS_LINE_LOW     +1
  { RadioEvent_CTSLineLow,   2 } ,  // 0x01 << 13  PHY_RADIO_CTS_LINE_LOW     +2
  { RadioEvent_CTSLineLow,   3 } ,  // 0x01 << 14  PHY_RADIO_CTS_LINE_LOW     +3
  { RadioEvent_CTSLineLow,   4 } ,  // 0x01 << 15  PHY_RADIO_CTS_LINE_LOW     +4
  { RadioEvent_CTSLineLow,   5 } ,  // 0x01 << 16  PHY_RADIO_CTS_LINE_LOW     +5
  { RadioEvent_CTSLineLow,   6 } ,  // 0x01 << 17  PHY_RADIO_CTS_LINE_LOW     +6
  { RadioEvent_CTSLineLow,   7 } ,  // 0x01 << 18  PHY_RADIO_CTS_LINE_LOW     +7
  { RadioEvent_CTSLineLow,   8 } ,  // 0x01 << 19  PHY_RADIO_CTS_LINE_LOW     +8

  { RadioEvent_Int,   0 } ,  // 0x01 << 20  PHY_RADIO_INT_PENDING_BASE +0
#if ( MCU_SELECTED == NXP_K24 )
  { RadioEvent_Int,   1 } ,  // 0x01 << 21  PHY_RADIO_INT_PENDING_BASE +1
  { RadioEvent_Int,   2 } ,  // 0x01 << 22  PHY_RADIO_INT_PENDING_BASE +2
  { RadioEvent_Int,   3 } ,  // 0x01 << 23  PHY_RADIO_INT_PENDING_BASE +3
  { RadioEvent_Int,   4 } ,  // 0x01 << 24  PHY_RADIO_INT_PENDING_BASE +4
  { RadioEvent_Int,   5 } ,  // 0x01 << 25  PHY_RADIO_INT_PENDING_BASE +5
  { RadioEvent_Int,   6 } ,  // 0x01 << 26  PHY_RADIO_INT_PENDING_BASE +6
  { RadioEvent_Int,   7 } ,  // 0x01 << 27  PHY_RADIO_INT_PENDING_BASE +7
  { RadioEvent_Int,   8 } ,  // 0x01 << 28  PHY_RADIO_INT_PENDING_BASE +8
#elif ( MCU_SELECTED == RA6E1 )
  { NULL,   0 } ,  // 0x01 << 21
  { NULL,   0 } ,  // 0x01 << 22
  { NULL,   0 } ,  // 0x01 << 23
  { NULL,   0 } ,  // 0x01 << 24
  { NULL,   0 } ,  // 0x01 << 25
  { NULL,   0 } ,  // 0x01 << 26
  { NULL,   0 } ,  // 0x01 << 27
  { NULL,   0 } ,  // 0x01 << 28
#endif
  { NULL,   0 } ,  // 0x01 << 29
  { NULL,   0 } ,  // 0x01 << 30
  { NULL,   0 } ,  // 0x01 << 31
};

// Used to put the PHY into test mode
static struct
{
   bool           enabled;
   uint16_t       UseMarksTxAlgo;     // Use Mark's algorithm for constant TX power
   uint16_t       UseDennisTempCheck; // Use Dennis' algorithm for PA protection
   uint16_t       UseDynamicBackoff;  // Use fixed backoff delay (0) or dynamic (1)
   uint16_t       T;                  // Time the signal is active
   uint16_t       Period;             // Time for one full cycle
   uint16_t       RepeatCnt;          // How many time to repeat the loop (65535 is infinity)
   uint16_t       Delay2;             // Delay between 2 loops
   uint16_t       RepeatCnt2;         // How many time to repeat the 2nd loop (65535 is infinity)
   RADIO_MODE_t   mode;
} test_mode;


/***********************************************************************************************************************

 Function Name: PHY_TestMode_Enable

 Purpose: Enable txmode with some specific duty cycle

 Arguments: delay              - How long to delay the test in seconds
            UseMarksTxAlgo     - Use Mark's algorithm for constant TX power
            UseDennisTempCheck - Use Dennis' algorithm for PA protection
            UseDynamicBackoff  - Use fixed backoff delay (0) or dynamic (1)
            T                  - How long the test runs in msec
            Period             - Period time. Time the test is on (T) plus time the test is off in msec.
            RepeatCnt          - How many times the test will run. 65535 is forever.
            Delay2             - Second delay (in second)
            RepeatCnt2         - Repeat second loop (65535 is forever)
            mode               - CW or PN9

 Returns: none

***********************************************************************************************************************/
void PHY_TestMode_Enable(uint16_t delay, uint16_t UseMarksTxAlgo, uint16_t UseDennisTempCheck, uint16_t UseDynamicBackoff, uint16_t T, uint16_t Period, uint16_t RepeatCnt, uint16_t Delay2, uint16_t RepeatCnt2, RADIO_MODE_t mode)
{

   if (RepeatCnt > 0)
   {
      if(test_mode.enabled == false)
      {
         // Prevent MAC from sending Data.request
         (void) MAC_StopRequest( NULL );
         OS_TASK_Sleep ( ONE_SEC );

         OS_TASK_Sleep ( delay*ONE_SEC );

         test_mode.UseMarksTxAlgo     = UseMarksTxAlgo;
         test_mode.UseDennisTempCheck = UseDennisTempCheck;
         test_mode.UseDynamicBackoff  = UseDynamicBackoff;
         test_mode.T                  = T;
         test_mode.Period             = Period;
         test_mode.RepeatCnt          = RepeatCnt;
         test_mode.Delay2             = Delay2;
         test_mode.RepeatCnt2         = RepeatCnt2;
         test_mode.mode               = mode;

#if !( (DCU == 1) && (VSWR_MEASUREMENT == 1) )  /* Don't do this on 9985T hardware with FEM. */
         RADIO_Mode_Set(mode);
#endif

         test_mode.enabled = true; // Enable txmode
      }
   }else
   {
      test_mode.RepeatCnt = 0;
      (void) MAC_StartRequest( NULL );
   }
}

// Execute the test mode
static void PHY_TestMode(void)
{
   uint32_t cycle1 = 0;
   uint32_t cycle2 = 0;
#if ( DCU == 1)
   int32_t PaTemp;    // PA temperature
   static bool txdisabled = false;

   double   computedPower = POWER_LEVEL_DEFAULT; // Computed power level
   double   PaPower;   // Radio power level
#if ( VSWR_MEASUREMENT == 0 )
   double   val;       // Value to pass to log
#else
   union    si446x_cmd_reply_union Si446xCmd;
#endif
   int16_t  radioTemp; // Radio temperature
   int16_t  clippedRadioTemp; // Radio temperature
   uint32_t delay;
#endif

   if(test_mode.enabled == true)
   {
      DBG_logPrintf( 'I', "Test Mode : UseMarksTxAlgo=%u UseDennisTempCheck=%u UseDynamicBackoff=%u T=%u msec Period=%u msec Cycles=%u ",
                     test_mode.UseMarksTxAlgo, test_mode.UseDennisTempCheck, test_mode.UseDynamicBackoff, test_mode.T, test_mode.Period, test_mode.RepeatCnt );
      do
      {
         do
         {
            DBG_logPrintf( 'I', "Test Mode # Cycle %u of %u (outer loop %u)", ++cycle1, test_mode.RepeatCnt, cycle2+1 );
#if ( DCU == 1 )
            (void)RADIO_Temperature_Get( (uint8_t)RADIO_0, &radioTemp );
            clippedRadioTemp = radioTemp;

            // Clip temperature if needed.
            if ( radioTemp < RADIO_TEMPERATURE_MIN ) {
               clippedRadioTemp = RADIO_TEMPERATURE_MIN;
            } else if ( radioTemp > RADIO_TEMPERATURE_MAX ) {
               clippedRadioTemp = RADIO_TEMPERATURE_MAX;
            }
            PaTemp = (int32_t)ADC_Get_PA_Temperature( TEMP_IN_DEG_C );

            // Force TX by default
            delay = 0;
            txdisabled = (bool)false;

            // Check if the temperature check is needed
            if ( test_mode.UseDennisTempCheck ) {
               delay = (uint32_t)(virtualTemperature_Delay()*ONE_SEC); // Get delay in ms
               if ( delay > 0 ) {
                  txdisabled = (bool)true; // disable TX because of backoff time
               }
            }

#if ( VSWR_MEASUREMENT == 0 )
            if ( test_mode.UseMarksTxAlgo ) {
               val = 1.21574-(0.0352986512723369*PHY_TX_OUTPUT_POWER_DEFAULT);

               // Sanitize input
               if ( val < 0.01 ) {
                  val = 0.01;
               }

               // Compute radio PA level
               computedPower = -4.4*log(val)*(0.7915131 - 0.774956*(1 - exp(0.01309945*clippedRadioTemp)))*( 0.000000003571429*(ConfigAttr.TxChannels[0]*6250+450000000) - 0.6178571);

               // Sanitize input
               if ( computedPower < 0 ) {
                  computedPower = 0;
               } else if ( computedPower > RADIO_PA_LEVEL_MAX ) {
                  computedPower = RADIO_PA_LEVEL_MAX;
               }

               PaPower = computedPower;

               if (txdisabled == true) {
                  PaPower = 0;
                  (void)si446x_change_state((uint8_t)RADIO_0, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY); // Leave RX/TX state
                  RDO_PA_EN_OFF(); // Disable PA
               } else {
                  RADIO_Power_Level_Set( (uint8_t)PaPower );
                  RDO_PA_EN_ON(); // Enable PA
                  /* START immediately, Packet according to PH */
                  (void)si446x_start_tx((uint8_t)RADIO_0, 0u, 0u, 0u, 0u);
                  virtualTemperature.lengthInMs = test_mode.T;
               }
            } else {
               if (txdisabled) {
                  //RADIO_Power_Level_Set(0);
                  PaPower = computedPower = 0;
                  (void)si446x_change_state((uint8_t)RADIO_0, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY); // Leave RX/TX state
                  RDO_PA_EN_OFF(); // Disable PA for safety while switching
               } else {
                  PaPower = computedPower = ConfigAttr.PowerSetting;
                  RADIO_Power_Level_Set(ConfigAttr.PowerSetting);
                  RDO_PA_EN_ON(); // Enable PA
                  /* START immediately, Packet according to PH */
                  (void)si446x_start_tx((uint8_t)RADIO_0, 0u, 0u, 0u, 0u);
                  virtualTemperature.lengthInMs = test_mode.T;
               }
            }
#else       /* Using FEM to control TX power level */
            if (txdisabled) {
               /* Disable FEM PA and Vfwd signal   */
               (void)si446x_gpio_pin_cfg((uint8_t)RADIO_0,
                                         SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,  // <-- GPIO_0 on TBs connects to FTM0_CH4. There is no FTM on that pin for EPs.
                                         SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_EN_PA,
                                         SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE1,   // 1
                                         SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TX_STATE, // 1 while transmitting then 0
                                         SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                         SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                         SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5,
                                         &Si446xCmd);
               (void)si446x_change_state((uint8_t)RADIO_0, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY); // Leave RX/TX state
            } else {

               RADIO_Mode_Set( test_mode.mode );   /* Invokes RADIO_SetPower and setPA, and starts Transmission.  */
            }
#endif
            INFO_printf("PA settings: radio temp = %dC, used temp = %dC, PA temp = %dC, computed level(*100) %u, programmed level %u", radioTemp, clippedRadioTemp, PaTemp,
                           (uint32_t)(computedPower*100), (uint8_t)PaPower);

            // ON duty cycle part
            if (txdisabled == true) {
               // Don't transmit
               if ( (test_mode.UseDennisTempCheck == 0) ||                                         // if we don't use Dennis algorithm or
                    (test_mode.UseDennisTempCheck     ) && (test_mode.UseDynamicBackoff == 0) ) {  // we use his algorithm but we have to use a fixed delay
                  // Use fixed backoff delay
                  OS_TASK_Sleep ( (uint32_t)(test_mode.T-1U) );
               }
            } else {
               uint32_t i;
               for (i=0; i<(test_mode.T/10); i++ ) {
                  OS_TASK_Sleep ( 5 ); // This value will force the OS to sleep for 10 ms (OS granularity)
                  if ( ( i % 100U ) == 0 ) {
                     (void)DBG_CommandLine_GetHWInfo ( 0, NULL );
                  }
               }
            }
#else /* Not DCU  */
            // ON duty cycle part
            RADIO_Power_Level_Set(ConfigAttr.PowerSetting);
            RDO_PA_EN_ON(); // Enable PA
            /* START immediately, Packet according to PH */
            (void)si446x_start_tx((uint8_t)RADIO_0, 0u, 0u, 0u, 0u);
            OS_TASK_Sleep ( test_mode.T-1 );
#endif
            // OFF duty cycle part
            (void)si446x_change_state((uint8_t)RADIO_0, SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY); // Leave RX/TX state
#if !( (DCU == 1) && (VSWR_MEASUREMENT == 1) )  /* All hardware but 9985T   */
            RDO_PA_EN_OFF(); // Disable PA for safety while switching
#else
            /* Disable FEM PA and Vfwd signal   */
            (void)si446x_gpio_pin_cfg((uint8_t)RADIO_0,
                                      SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DIV_CLK,  // <-- GPIO_0 on TBs connects to FTM0_CH4. There is no FTM on that pin for EPs.
                                      SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_EN_PA,
                                      SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_DRIVE1,   // 1
                                      SI446X_CMD_GPIO_PIN_CFG_ARG_GPIO_GPIO_MODE_ENUM_TX_STATE, // 1 while transmitting then 0
                                      SI446X_CMD_GPIO_PIN_CFG_ARG_NIRQ_NIRQ_MODE_ENUM_DONOTHING,
                                      SI446X_CMD_GPIO_PIN_CFG_ARG_SDO_SDO_MODE_ENUM_DONOTHING,
                                      SI446X_CMD_GPIO_PIN_CFG_ARG_GEN_CONFIG_DRV_STRENGTH_ENUM_LOW << 5,
                                      &Si446xCmd);
#endif

#if ( DCU == 1 )
            if ( (delay > 0) && (test_mode.UseDynamicBackoff) ) {
               // Use dynamic OFF delay
               OS_TASK_Sleep ( delay );
            } else
#endif
            {
               // Use fixed OFF delay
               OS_TASK_Sleep ( (uint32_t)((test_mode.Period - test_mode.T) - 1U) );
            }
         } while ((cycle1 < test_mode.RepeatCnt ) || (test_mode.RepeatCnt == 65535) );
         cycle1 = 0;
         cycle2++;

         // Dont sleep on last loop
         if ((cycle2 < test_mode.RepeatCnt2 ) || (test_mode.RepeatCnt2 == 65535) ) {
            OS_TASK_Sleep ( test_mode.Delay2*ONE_SEC );
         }

      } while ((cycle2 < test_mode.RepeatCnt2 ) || (test_mode.RepeatCnt2 == 65535) );
      RADIO_Mode_Set(eRADIO_MODE_NORMAL);
      test_mode.enabled = false;
      (void)SM_StopRequest(NULL );
      (void)SM_StartRequest( eSM_START_STANDARD, NULL );
   }
}

/*!
 ********************************************************************************************************
 *
 *  \fn           noiseFloor_CB
 *
 *  \brief        Kick start noisefloor computation
 *
 *  \param        uint8_t, void *
 *
 *  \return       None
 *
 *  \details      This function will set a flag that will in turn force a noisefloor update.
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
static void noiseFloor_CB(uint8_t cmd, void *pData)
{
   (void)cmd;
   (void)pData;

   // Fake a channel was changed to kick start noisefloor computation.
   ChannelAdded = true;
}

/*!
 ********************************************************************************************************
 *
 *  \fn           virtualTemperature_Init
 *
 *  \brief        Initialize the virtual temperature structure
 *
 *  \param        void
 *
 *  \return       None
 *
 *  \details
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
static void virtualTemperature_Init( void )
{
   (void)memset( &virtualTemperature, 0, sizeof( virtualTemperature ) );
}

#if ( DCU == 1 )
/*!
 ********************************************************************************************************
 *
 *  \fn           virtualTemperature_CB
 *
 *  \brief        Compute PA virtual temperature
 *
 *  \param        void
 *
 *  \return       None
 *
 *  \details      This function will compute the extimate PA temperature based on passed transmission.
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
static void virtualTemperature_CB(uint8_t cmd, void *pData)
{
   (void)cmd;
   (void)pData;
   static float PATemp;
   static uint32_t cntr=0;

   // Read PA every .1 second. It doesn't change that fast anyway and consumes CPU time.
   if ( (cntr % PHY_VIRTUAL_TEMP_PA_REFRESH_RATE) == 0 ) {
      PATemp = ADC_Get_PA_Temperature( TEMP_IN_DEG_C );
   }

   // Increase virtual temperature if a frame was just sent
   // Only one of the 2 length should be set
   virtualTemperature.vTemp += PHY_VIRTUAL_TEMP_RISE_PER_BYTE*virtualTemperature.lengthInBytes;
   virtualTemperature.vTemp += PHY_VIRTUAL_TEMP_RISE_PER_MS  *virtualTemperature.lengthInMs;

   virtualTemperature.lengthInBytes = 0;
   virtualTemperature.lengthInMs    = 0;

   // Decrease virtual temperature of a previously transmitted frame
   if ( (virtualTemperature.vTemp - PATemp) <  PHY_VIRTUAL_TEMP_TRANSITION_POINT ) {
      virtualTemperature.vTemp -= PHY_VIRTUAL_TEMP_SLOW_DROP*PHY_VIRTUAL_TEMP_REFRESH_RATE/ONE_SEC;
   } else {
      virtualTemperature.vTemp -= PHY_VIRTUAL_TEMP_FAST_DROP*PHY_VIRTUAL_TEMP_REFRESH_RATE/ONE_SEC;
   }

   // Virtual temperature can never be colder than PA temp sensor
   if ( virtualTemperature.vTemp < PATemp ) {
      virtualTemperature.vTemp = PATemp;
   }

   // Sanity check
   // We clip the maximum temperature if needed
   // The minimum temperature check is taken care of by checking against the PA temperature sensor
   if ( virtualTemperature.vTemp > PHY_VIRTUAL_TEMP_MAX ) {
      virtualTemperature.vTemp = PHY_VIRTUAL_TEMP_MAX;
   }

   // Check if thermal was exceeded
   if ( (virtualTemperature.vTemp > PHY_VIRTUAL_TEMP_LIMIT) && ConfigAttr.ThermalProtectionEnable ) {
      if (CachedAttr.ThermalProtectionEngaged == FALSE) {
         CachedAttr.ThermalProtectionEngaged = TRUE;
         PHY_CounterInc(ePHY_ThermalProtectionCount, 0);
      }
   } else {
      CachedAttr.ThermalProtectionEngaged = FALSE;
   }

   if ( (cntr % PHY_VIRTUAL_TEMP_PA_REFRESH_RATE) == 0 ) {
//      INFO_printf("vTemp %d PA %d", (int32_t)(virtualTemperature.vTemp*100), (int32_t)(PATemp*100) );
   }

   cntr++;
}

/*!
 ********************************************************************************************************
 *
 *  \fn           virtualTemperature_Delay
 *
 *  \brief        Compute delay needed for PA virtual temperature to come back below threshold
 *
 *  \param        void
 *
 *  \return       Minimum dealy in ms before alowing transmission
 *
 *  \details
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
static uint32_t virtualTemperature_Delay( void )
{
   float PATemp;
   float SlowDeg;
   float FastDeg;
   float DelTime;

   PATemp = ADC_Get_PA_Temperature( TEMP_IN_DEG_C );

   SlowDeg = PATemp + PHY_VIRTUAL_TEMP_TRANSITION_POINT - PHY_VIRTUAL_TEMP_LIMIT;

   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // Disable all interrupts. We are accessing virtualTemperature structure that can be modified by the timer callback.
   // Check if we are in the slow of fast drop part
   if ( SlowDeg < 0 ) {
      SlowDeg = 0;
      FastDeg = virtualTemperature.vTemp + PHY_VIRTUAL_TEMP_RISE_PER_FRAME - PHY_VIRTUAL_TEMP_LIMIT;
   } else {
      FastDeg = ((virtualTemperature.vTemp + PHY_VIRTUAL_TEMP_RISE_PER_FRAME) - PHY_VIRTUAL_TEMP_TRANSITION_POINT) - PATemp;
   }
   __set_PRIMASK(primask); // Restore interrupts

   if ( FastDeg < 0 ) {
      FastDeg = 0;
   }

   DelTime = (FastDeg / PHY_VIRTUAL_TEMP_FAST_DROP) + (SlowDeg / PHY_VIRTUAL_TEMP_SLOW_DROP);
   INFO_printf("delay %d PAtemp %d vTemp %d", (int32_t)(DelTime*1000), (int32_t)(PATemp*100), (int32_t)(virtualTemperature.vTemp*100) );

   return ( (uint32_t)(DelTime*1000) );
}
#endif

/*!
 ********************************************************************************************************
 *
 *  \fn           PHY_VirtualTemp_Get
 *
 *  \brief        Return virtual temperature
 *
 *  \param        void
 *
 *  \return
 *
 *  \details
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
float PHY_VirtualTemp_Get( void )
{
   return (virtualTemperature.vTemp);
}

#if ( DCU == 1)
/********************************************************************************************************
 *  \fn           computeRssiStats_CB
 *
 *  \brief        Kick start RSSI averaging computation
 *
 *  \param        uint8_t, void *
 *
 *  \return       None
 *
 *  \details      This function will set a flag that will in turn force a RSSI stats update.
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
static void computeRssiStats_CB(uint8_t cmd, void *pData)
{
   (void)cmd;
   (void)pData;

   ComputeRssiStats = true;
}

/********************************************************************************************************
 *  \fn           PHY_ReplaceLastRssiStat
 *
 *  \brief        Replaces last (buffered) RSSI Stat Measurement for specified radio
 *
 *  \param        uint8_t radioNum - Radio to retake RSSI measurement on
 *
 *  \return       None
 *
 *  \details      This function replaces the last buffered background RSSI measurement if it was possible
 *                that a message could have been recieved during the reading.
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
void PHY_ReplaceLastRssiStat(uint8_t radioNum)
{
   uint8_t newRssi;
#if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 2 )
   uint8_t tempRSSI = PhyRssiStats10m[radioNum].Prev;
#endif
   newRssi = RADIO_Get_CurrentRSSI(radioNum); //Record new value into historical buffers
   PhyRssiStats10m[radioNum].Prev = newRssi;
   PhyRssiStats60m[radioNum].Prev = newRssi;
#if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 2 )
   INFO_printf("RSSI Radio%u Ch%-4u Replaced %idBm with %idBm", radioNum, ConfigAttr.RxChannels[radioNum],
               (int16_t)RSSI_RAW_TO_DBM(tempRSSI), (int16_t)RSSI_RAW_TO_DBM(newRssi) );
#endif
}
#endif

/********************************************************************************************************
 *  \fn           PHY_FlushShadowAttr
 *
 *  \brief        Flush shadow attributes into config attributes and save to NV if needed
 *
 *  \param        None
 *
 *  \return       None
 *
 *  \details      Flush shadow attributes into config attributes and save to NV if needed
 *
 *  \sideeffect   None
 *
 *  \reentrant    Yes
 *******************************************************************************************************/
static void PHY_FlushShadowAttr( void )
{
   uint32_t i; // loop counter
   bool updateNV = false;

   for ( i=0; i<PHY_RCVR_COUNT; i++ ) {
      // Check if the new AFC adjustment needs to be commited to flash
      if ( abs(ShadowAttr.AfcAdjustment[i] - ConfigAttr.AfcAdjustment[i]) > PHY_AFC_ADJUSTMENT_THRESHOLD ) {
         ConfigAttr.AfcAdjustment[i] = ShadowAttr.AfcAdjustment[i];
         updateNV = (bool)true;
      }
   }
   if ( updateNV ) {
      writeConfig();
   }
}

/***********************************************************************************************************************

 Function Name: PHY_Task

 Purpose:

 Arguments: Arg0 - Not used, but required here because this is a task

 Returns: none

***********************************************************************************************************************/
void PHY_Task( taskParameter )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   (void)   Arg0;
#endif
   uint32_t timeout;
   uint32_t defaultTimeout;
#if ( DCU == 1 )
   uint32_t i;
   uint8_t  tempRssi;
   bool     delayRssiStats;
#endif

#if 0
   // used to check RS codes
   static uint8_t original[256] = {0};
   static uint8_t corrupted[256] = {0};
   static uint8_t decoded[256] = {0};

   uint32_t j,k;
   uint32_t count=0;
   uint32_t count2=0;
   uint32_t pos;

#define PAYLOAD_LENGTH 239

   srand(1);

   for (j=0; j<1000000; j++) {
      // build msg
      for (i=0; i<PAYLOAD_LENGTH; i++) {
        original[i] = rand();
      }
      // Encode msg in RS(255,239)
      FrameParityCheck_Calc(original, PAYLOAD_LENGTH, 1, 1);

      // Save original to be corrupted
      (void)memcpy(corrupted,original,PAYLOAD_LENGTH+16);

      // Corrupt msg
      for (k=0; k<8; k++) {
//         pos = ((uint32_t)rand())%1128;
//         msg[pos/8] ^= 1 << (pos%8);

         corrupted[((uint32_t)rand())%(PAYLOAD_LENGTH+16)] = (uint8_t)rand();
      }

      // Save corrupted to be decoded
      //memcpy(decoded,corrupted,PAYLOAD_LENGTH+16);

      // Decode msg
      if (FrameParityCheck_Validate(corrupted, PAYLOAD_LENGTH, 1, 1)) {
         count++;
      } else {
         OS_TASK_Sleep(1);
      }

      if (memcmp(original,corrupted,PAYLOAD_LENGTH+16) == 0) {
         count2++;
      } else {
         OS_TASK_Sleep(1);

         for (i=0; i<PAYLOAD_LENGTH+16; i++) {
            if (original[i] != corrupted[i]) {
               OS_TASK_Sleep(1);
            }
         }
      }

      if ((j&0xFF) == 0) {
         OS_TASK_Sleep(1);
      }
   }

   INFO_printf("count %u count2 %u",count, count2);
#endif
   timeout = ONE_SEC; // Do some background processing every second.
   defaultTimeout = timeout;

   INFO_printf("PHY_Task starting...");
   pRequestBuf = NULL;

   virtualTemperature_Init();

   // Start in Shutdown state
   Radio.Shutdown();
   _phy.state = ePHY_STATE_SHUTDOWN;

   if ( PHY_Initialized == (bool)true )
   {
      timer_t tmrSettings;

      // Configure periodic timer for noise estimate
      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.ulDuration_mS  = ONE_MIN; // This is really just to create the timer since we can't create a timer with a duration of 0
      tmrSettings.pFunctCallBack = noiseFloor_CB;
      if ( TMR_AddTimer(&tmrSettings) != eSUCCESS ) {
         ERR_printf("Failed to create a callback timer for noisefloor computation");
      } else {
         NoiseEstimateRateTmrID = tmrSettings.usiTimerId; // Save timer ID for later
         // Program the delay
         if ( ConfigAttr.NoiseEstimateRate ) {
            (void)TMR_ResetTimer( NoiseEstimateRateTmrID, ConfigAttr.NoiseEstimateRate*ONE_MIN );
         } else {
            // We can't use TMR_ResetTimer with 0 because it won't stop the timer. It would only restart it.
            (void)TMR_StopTimer( NoiseEstimateRateTmrID );
         }
      }
#if ( DCU == 1 )
      // Configure periodic timer for virtual temperature estimate
      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.ulDuration_mS  = PHY_VIRTUAL_TEMP_REFRESH_RATE;
      tmrSettings.pFunctCallBack = virtualTemperature_CB;
      if ( TMR_AddTimer(&tmrSettings) != eSUCCESS ) {
         ERR_printf("Failed to create a callback timer for virtual temperature computation");
      }

      // Configure periodic timer for Background RSSI Statistics (used for characterizing noise/interference on a channel)
      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.ulDuration_mS  = ONE_SEC * 6; //Every six seconds take a measurement
      tmrSettings.pFunctCallBack = computeRssiStats_CB;
      ComputeRssiStats = true;
      (void)memset( PhyRssiStats10m, 0, sizeof(PhyRssiStats10m) ); //Clear the historical buffers, Sums, counts, and peak values
      (void)memset( PhyRssiStats60m, 0, sizeof(PhyRssiStats60m) ); //Clear the historical buffers, Sums, counts, and peak values
#if ( MAC_LINK_PARAMETERS == 1 )
      (void)memset( PhyRssiStats10m_Copy_, 0, sizeof(PhyRssiStats10m_Copy_) ); //Clear the Copy of PhyRssiStats10m
#endif
      if ( TMR_AddTimer(&tmrSettings) != eSUCCESS ) {
         ERR_printf("Failed to create a callback timer for RSSI reports");
      }
#if ( VSWR_MEASUREMENT == 1 )
      // Configure timer for VSWR
      (void)memset( &VSWRtimerCfg, 0, sizeof(VSWRtimerCfg) );
      VSWRtimerCfg.ulDuration_mS = VSWR_TIMEOUT;
      VSWRtimerCfg.bOneShot = 1U;
      VSWRtimerCfg.bStopped = 1U;
      VSWRtimerCfg.pFunctCallBack = phyVSWRTimeout;
      if ( TMR_AddTimer( &VSWRtimerCfg ) != eSUCCESS ) {
         ERR_printf("Failed to create VSWR timer");
      }
#endif
#endif
      for (;;)
      {
         uint32_t event_flags;

         PHY_TestMode(); // Run CW or PN9 test

         // Wait for a message to process
         event_flags = OS_EVNT_Wait ( &events, PHY_EVENT_MASK, (bool)false , timeout );

         OS_MUTEX_Lock( &PHY_Mutex_ ); // Function will not return if it fails

         // Restore timeout value
         timeout = defaultTimeout;

         // Give priority to interrupt processing
         if (event_flags & 0xFFFFFFFE)
         {  // Handle the Radio Events
            uint32_t event_id;
            uint32_t eventFlagsCopy = event_flags;
            while ( eventFlagsCopy )
            {  // Find event
               event_id = 31U - __CLZ(eventFlagsCopy);   // This is an ARM intrinsic instruction that returns the number of leading zero.
                                                         // So if the MSB is set, CLZ will return 0 and event_id will be 31.
               {
                  if (radio_events[event_id].fxn != NULL) {
#pragma calls=RadioEvent_TxDone,RadioEvent_RxData,RadioEvent_CTSLineLow,RadioEvent_Int
                     radio_events[event_id].fxn(radio_events[event_id].radio_num);
                  }
               }
               eventFlagsCopy &= ~(1U << event_id); // Clear event
            }
         }

         // Check if TX has been ON for too long. If so, we need to go back to RX.
         // We don't check for ePHY_STATE_READY_TX since in that state we remain in TX after TX is over. The watchdog would revert back to RX which we don't want.
         // We don't check for ePHY_STATE_READY_RX since in that state, we never TX so no need to check for TX timeout.
         if (_phy.state == ePHY_STATE_READY) {
            RADIO_TX_Watchdog();
         }

         // Check if we are in the proper state to update noise floor
         if ((_phy.state == ePHY_STATE_READY) || (_phy.state == ePHY_STATE_READY_RX)) {

            // Make sure soft-demod is still receiving interrupts
            // Sometimes interrupts are no longer generated if something like pausing the debuger or taking too long to service the interrupts.
#if ( EP == 1)
            RADIO_RX_Watchdog();
#endif
            // Check if the channel list was modified or it is time to refresh the noise floor
            if ( ChannelAdded ) {
               // Make sure we are not transmitting something before processing
               if ( RADIO_Is_TX() ) {
                  timeout = FIVE_MSEC; // Play nice with rest of system but complete rest of processing ASAP.
               } else {
                  // The noise floor will be updated through multiple calls since we can't afford to stay in a call for too long or we risk losing interrupts from other radios.
                  if (!RADIO_Update_Noise_Floor()) {
                     // Noise floor update is not done
                     timeout = FIVE_MSEC; // Play nice with rest of system but complete rest of processing ASAP.
                  } else {
                     // Noise floor update is done
                     ChannelAdded = false;
                  }
               }
            }

#if ( DCU == 1 )
            // Compute background RSSI only when in normal mode
            if ( ComputeRssiStats && (RADIO_TxMode_Get() == eRADIO_MODE_NORMAL) ) {
               if ( RADIO_Is_TX() ) {
                  timeout = FIVE_MSEC; // Play nice with rest of system but complete rest of processing ASAP.
                  INFO_printf("Background RSSI Stats delayed by RF transmission");
               } else {
                  delayRssiStats = false;
                  for (i=(uint32_t)RADIO_FIRST_RX; i<(uint32_t)PHY_RCVR_COUNT; i++) {
                     if(RADIO_Get_RxStartOccured((uint8_t)i)==1) {
                        if( ( DWT_CYCCNT - RADIO_Get_RxStartCYCCNTTimeStamp((uint8_t)i) ) < (3*(getCoreClock()/1000)) ) { // 3ms*120000ccpms is (msecs)*(clock cycles per msec)
                           timeout = FIVE_MSEC; // Give the radio enough time to collect samples and generate a valid RSSI
                           delayRssiStats = true;
                           #if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 1 )
                           INFO_printf("Background RSSI Stats delayed by Radio %u", i);
                           #endif
                        } else { // more than 3ms has elapsed since the radio restarted
                           RADIO_Clear_RxStartOccured((uint8_t)i);
                           #if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 1 )
                           INFO_printf("Background RSSI Stats cleared Radio %u startRx flag. CYC: %u", i, (DWT_CYCCNT - RADIO_Get_RxStartCYCCNTTimeStamp(i)));
                           #endif
                        }
                     }
                  }

                  if ( delayRssiStats == false ) {
                     for (i=(uint32_t)RADIO_FIRST_RX; i<(uint32_t)PHY_RCVR_COUNT; i++) {
                        if (ConfigAttr.RxChannels[i] < PHY_INVALID_CHANNEL) {
                           tempRssi = RADIO_Get_CurrentRSSI((uint8_t)i);
                           PHY_UpdateRssiStats(PhyRssiStats10m, i, tempRssi);
                           PHY_UpdateRssiStats(PhyRssiStats60m, i, tempRssi);
                        }
                     }
                     #if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 3 )
                     INFO_printf("Background RSSI Stats measurement completed");
                     #endif

                     ComputeRssiStats = false;
                  }
               }
            }
#endif
         }

         // Process messages from other task
         if (event_flags & PHY_MESSAGE_PENDING)
         {
            buffer_t *pReqBuf;

            while (OS_MSGQ_Pend( &PHY_msgQueue, (void *)&pReqBuf, 0))
            {
               if (pReqBuf->eSysFmt == eSYSFMT_PHY_REQUEST)
               {  // Request
                  if (pRequestBuf != NULL)
                  {  // Got a new request before previous was confirmed!
                     // Need to get back in sync with other layer!!!
                     INFO_printf("Unexpected request");
                     BM_free(pRequestBuf);
                     pRequestBuf = NULL;
                  }

                  // There are no requests that have not been confirmed
                  PHY_Request_t *pReq;
                  bool bConfirmStatus = false;

                  // Save the request buffer
                  pRequestBuf = pReqBuf;
                  pReq = (PHY_Request_t*) &pReqBuf->data[0];  /*lint !e740 !e826 */

                  switch (pReq->MsgType)
                  {
                     case ePHY_CCA_REQ:   bConfirmStatus = Process_CCAReq(  pReq); break;
                     case ePHY_GET_REQ:   bConfirmStatus = Process_GetReq(  pReq); break;
                     case ePHY_SET_REQ:   bConfirmStatus = Process_SetReq(  pReq); break;
                     case ePHY_START_REQ: bConfirmStatus = Process_StartReq(pReq); break;
                     case ePHY_STOP_REQ:  bConfirmStatus = Process_StopReq( pReq); break;
                     case ePHY_RESET_REQ: bConfirmStatus = Process_ResetReq(pReq); break;
                     case ePHY_DATA_REQ:  bConfirmStatus = Process_DataReq( pReq); break;
                     default:
                        // unsupported request type
                        INFO_printf("UnknownReq");
                        BM_free(pRequestBuf);
                        pRequestBuf = NULL;
                     break;
                  }
                  if ( bConfirmStatus )
                  {  // Message handler returned the confirm
                     // So send the confirmation
                     SendConfirm(&PhyConfMsg, pRequestBuf);
                     BM_free(pRequestBuf);
                     pRequestBuf = NULL;
                  }
               } else
               {
                  INFO_printf("Unsupported eSysFmt");
                  BM_free(pReqBuf);
               }
            }
         }
         RADIO_Update_Freq_Watchdog(); // Make sure TCXO trimming is active
         PHY_FlushShadowAttr();        // Take this opportunity to save shadow attributes to flash (if needed)

         OS_MUTEX_Unlock( &PHY_Mutex_ ); // Function will not return if it fails
      }
   }
   else
   {
      ERR_printf("PHY did not successfully initialize" );
   }
}

/***********************************************************************************************************************

 Function Name: PHY_UpdateRssiStats

 Purpose:

 Arguments:

 Returns: none

***********************************************************************************************************************/
#if (DCU == 1)
static void PHY_UpdateRssiStats(PHY_RssiStats_s RssiStats[], const uint32_t i, const uint8_t newRSSI)
{

   uint8_t  tempRssi;
   float    tempRssiF; //Linear scale dB
   float    oldAvg;

   tempRssi  =                        RssiStats[i].Prev; // Remove value from historical buffer
   tempRssiF = (float)RSSI_RAW_TO_DBM(RssiStats[i].Prev); // Convert from raw dB, store in a double
   RssiStats[i].Prev = newRSSI; // Record new value into historical buffer

   #if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 1 )
   if ( RssiStats[i].Prev < 18 ) { // Should not happen. -129dBm has a raw value of 10. -125dBm is raw 18.
      INFO_printf("Background RSSI Stats Recorded Radio%u RSSI:%-4idBm", i, (int16_t)RSSI_RAW_TO_DBM(RssiStats[i].Prev) );
   }
   #endif

   if(RssiStats[i].Num == 1) { // If this is the first value out of the historical buffer
      RssiStats[i].Min = tempRssi;
      RssiStats[i].Max = tempRssi;
      RssiStats[i].LogAvg = tempRssiF;
   } else if (RssiStats[i].Num > 1) {
      if(tempRssi < RssiStats[i].Min){
         RssiStats[i].Min = tempRssi;
      }
      if(tempRssi > RssiStats[i].Max){
         RssiStats[i].Max = tempRssi;
      }

      // Welford's method via https://www.johndcook.com/blog/standard_deviation/
      // StdDev and Avg are computed from log scale dB
      oldAvg = RssiStats[i].LogAvg;
      RssiStats[i].LogAvg = oldAvg              + ((tempRssiF - oldAvg) / (float)RssiStats[i].Num);
      RssiStats[i].StdDev = RssiStats[i].StdDev + ((tempRssiF - oldAvg) * (tempRssiF - RssiStats[i].LogAvg));
   }


   #if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 3 )
   if(RssiStats[i].Num != 0) {
      char floatStr[PRINT_FLOAT_SIZE];
      char floatStr2[PRINT_FLOAT_SIZE];
      INFO_printf("RSSI Radio%u Ch%-4u Measurement #%-3u \"Current\":%-4idBm  Mn:%-4idBm  Mx:%-4idBm  Avg:%sdBm  StdDev:%sdBm", i,
                  ConfigAttr.RxChannels[i],
                  RssiStats[i].Num,
                  (int16_t)RSSI_RAW_TO_DBM(tempRssi),
                  (int16_t)RSSI_RAW_TO_DBM(RssiStats[i].Min), // Min/Max both have a precision of 0.5, but printing them as integers to save memory
                  (int16_t)RSSI_RAW_TO_DBM(RssiStats[i].Max),
                  DBG_printFloat(floatStr,  RssiStats[i].LogAvg, 1),
                  DBG_printFloat(floatStr2, sqrt(RssiStats[i].StdDev/(float)RssiStats[i].Num), 1) );
   }
   #endif

   RssiStats[i].Num++;

}
#endif

/***********************************************************************************************************************

 Function Name: PHY_Lock

 Purpose: Prevent PHY from executing

 Arguments: None

 Returns: None

***********************************************************************************************************************/
void PHY_Lock(void)
{
   OS_MUTEX_Lock( &PHY_Mutex_ ); // Function will not return if it fails
}


/***********************************************************************************************************************

 Function Name: PHY_Unlock

 Purpose: Restore PHY processing

 Arguments: None

 Returns: None

***********************************************************************************************************************/
void PHY_Unlock(void)
{
   OS_MUTEX_Unlock( &PHY_Mutex_ );   /*lint !e455 Mutex previously locked */   // Function will not return if it fails
}


/***********************************************************************************************************************
Function Name: PHY_Request

Purpose: This function is called by the MAC to send a message to the PHY

Arguments: pointer to a buffer_t.  The buffer_t eSysFmt should be eSYSFMT_MFGP_DATA_LAYER, and the data and length
           members should be the phy message data & length.

Returns: True if successful
***********************************************************************************************************************/
static void PHY_Request(buffer_t *request)
{
   if ( PHY_Initialized == (bool)true ) {
      // Send the message to the task's queue
      OS_MSGQ_Post(&PHY_msgQueue, (void *)request); // Function will not return if it fails
      OS_EVNT_Set ( &events, PHY_MESSAGE_PENDING);  // Function will not return if it fails
   } else {
      ERR_printf("PHY_Request: PHY not initialized");
   }
}


/***********************************************************************************************************************
Function Name: PHY_RegisterIndicationHandler

Purpose: This function is called by an upper layer module to register a callback function.  That function will accept
   an indication (data.confirmation, data.indication, etc) from this module.  That function is also responsible for
   freeing the indication buffer.

Arguments: pointer to a funciton of type PHY_IndicationHandler

Returns: None

Note:  Currently only one upper layer can receive indications but the function could be expanded to have an list
   of callbacks if needed.
***********************************************************************************************************************/
void PHY_RegisterIndicationHandler(PHY_IndicationHandler pCallback)
{
   _phy.pIndicationHandler = pCallback;
}

static void writeCache( void ) {
   (void)FIO_fwrite( &PHYcachedFileHndl_, 0, (uint8_t*)&CachedAttr, sizeof(CachedAttr));
}

static void writeConfig( void ) {
   (void)FIO_fwrite( &PHYconfigFileHndl_, 0, (uint8_t*)&ConfigAttr, sizeof(ConfigAttr));
}

/*********************************************************************************************************************
 * Get/Set Functions
 *********************************************************************************************************************/
/*!
 * Used to set the AFC adjustment SHOULD ONLY be used by radio
*/
void PHY_AfcAdjustment_Set_By_Radio( uint8_t radioNum, int16_t adjustment )
{
   // Sanity check
   if ( radioNum < (uint8_t)PHY_RCVR_COUNT ) {
      ShadowAttr.AfcAdjustment[radioNum] = adjustment;
   }
}
#if ( EP == 1 )
/*!
 * Used to Enable/Disable the Afc function
 */
static PHY_SET_STATUS_e AfcEnable_Set( bool enable )
{
   ConfigAttr.AfcEnable = enable;
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the Afc Rssi Threshold
 * This is the RSSI threshold in dBm above which AFC measurements must exceed in order to be used for processing.
 * Range is -200 to +55 dBm
 */
static PHY_SET_STATUS_e AfcRssiThreshold_Set( int16_t threshold )
{
   if(( threshold >= PHY_AFC_RSSI_THRESHOLD_MIN ) &&
      ( threshold <= PHY_AFC_RSSI_THRESHOLD_MAX ))
   {
      ConfigAttr.AfcRssiThreshold = threshold;
      writeConfig();
      return ePHY_SET_SUCCESS;
   }else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set the Afc Temperature Range, Centigrade
 * The range is -50 to +100
 */
static PHY_SET_STATUS_e AfcTemperatureRange_Set( int8_t const TemperatureRange[2] )
{
   if ( (TemperatureRange[0] >=  PHY_AFC_TEMPERATURE_MIN ) &&
        (TemperatureRange[1] <=  PHY_AFC_TEMPERATURE_MAX ) &&
        (TemperatureRange[0] <   TemperatureRange[1]))
   {
       ConfigAttr.AfcTemperatureRange[0] = TemperatureRange[0];
       ConfigAttr.AfcTemperatureRange[1] = TemperatureRange[1];
       writeConfig();
       return ePHY_SET_SUCCESS;
   }else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}
#endif

/*!
 * Used to enable/disable the PHY CCA Adaptive Threshold feature
 */
static PHY_SET_STATUS_e CcaAdaptiveThresholdEnable_Set( bool val )
{
   ConfigAttr.CcaAdaptiveThresholdEnable = val;
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the PHY CCA offset
 */
static PHY_SET_STATUS_e CcaOffset_Set( uint8_t val )
{
   if(val <= PHY_CCA_OFFSET_MAX)
   {
      ConfigAttr.CcaOffset = val;
      /* Note: During, Last Gasp Worst Case Test, we are modifying this parameter and hence no need to update
         the NV Copy during the Test. */
#if ( LG_WORST_CASE_TEST == 0 )
      writeConfig();
#endif
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set the PHY CCA Threshold
 * The threshold must be between -200 and 55, inclusive
 */
static PHY_SET_STATUS_e CcaThreshold_Set( int16_t val )
{
   if((val >= PHY_CCA_THRESHOLD_MIN) && (val <= PHY_CCA_THRESHOLD_MAX))
   {
      ConfigAttr.CcaThreshold = val;
      /* Note: During, Last Gasp Worst Case Test, we are modifying this parameter and hence no need to update
         the NV Copy during the Test. */
#if ( LG_WORST_CASE_TEST == 0 )
      writeConfig();
#endif
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set the detector list
 */
static PHY_SET_STATUS_e Demodulator_Set( uint8_t const *demodulator )
{
   PHY_STATE_e state = _phy.state; // Save current PHY state.

   // [MKD] 2021-02-08 15:08 Modify to support an array of demodualtors for TB.
   // Sanity check
   if ( demodulator[0] < 2 ) {
      // Don't process request if already in the requested mode
      if ( ConfigAttr.Demodulator[0] != demodulator[0] ) {
         // Update demodulator mode
         (void)memcpy(ConfigAttr.Demodulator, demodulator, sizeof(ConfigAttr.Demodulator));
         writeConfig();

         // Stop and restart PHY for the changes to take effect
         (void)PHY_StopRequest(NULL);
         (void)PHY_StartRequest((PHY_START_e) state, NULL);
      }
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set the PHY front end gain
 */
static PHY_SET_STATUS_e FrontEndGain_Set( int8_t val )
{
   ConfigAttr.FrontEndGain = val;
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the PHY RSSI Jump Threshold
 * The threshold must be between 0 and 20, inclusive
 */
static PHY_SET_STATUS_e RssiJumpThreshold_Set( uint8_t val )
{
   if(val <= PHY_RSSI_JUMP_THRESHOLD_MAX)
   {
      ConfigAttr.RssiJumpThreshold = val;
      writeConfig();
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set MinRxByte
 * The value must be between 0 and 511, inclusive
 */
static PHY_SET_STATUS_e MinRxBytes_Set( uint16_t val )
{
   if( val <= 511 )
   {
      ConfigAttr.MinRxBytes = val;
      writeConfig();
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set MaxTxPayload
 * The value must be between PHY_MIN_TX_PAYLOAD and PHY_MAX_TX_PAYLOAD, inclusive
 */
static PHY_SET_STATUS_e MaxTxPayload_Set( uint16_t val )
{
   if( (val >= PHY_MIN_TX_PAYLOAD) && (val <= PHY_MAX_TX_PAYLOAD) )
   {
      ConfigAttr.MaxTxPayload = val;
      writeConfig();
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

#if ( TEST_SYNC_ERROR == 1 )
/*!
 * Used to set SyncError
 * The value must be between 0 and 7, inclusive
 */
static PHY_SET_STATUS_e SyncError_Set( uint8_t val )
{
   if( val <= 7 )
   {
      ConfigAttr.SyncError = val;
      writeConfig();
      RADIO_Set_SyncError( val );
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}
#endif

/*!
 * Used to Enable/Disable thermal control function
 */
static PHY_SET_STATUS_e ThermalControlEnable_Set( bool enable )
{
   ConfigAttr.ThermalControlEnable = enable;
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to Enable/Disable thermal protection function
 */
static PHY_SET_STATUS_e ThermalProtectionEnable_Set( bool enable )
{
   ConfigAttr.ThermalProtectionEnable = enable;
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set TxOutputPowerMax
 */
static PHY_SET_STATUS_e TxOutputPowerMax_Set( float val )
{
   if( val <= PHY_TX_OUTPUT_POWER_MAX )
   {
      ConfigAttr.TxOutputPowerMax = val;
      writeConfig();
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set TxOutputPowerMin
 */
static PHY_SET_STATUS_e TxOutputPowerMin_Set( float val )
{
   if( val <= PHY_TX_OUTPUT_POWER_MAX )
   {
      ConfigAttr.TxOutputPowerMin = val;
      writeConfig();
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to Enable/Disable verbose mode function
 */
static PHY_SET_STATUS_e VerboseMode_Set( bool enable )
{
   ConfigAttr.VerboseMode = enable;
   writeConfig();
   return ePHY_SET_SUCCESS;
}

#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
/*!
  Use to set Notification set point for VSWR
*/
static PHY_SET_STATUS_e TxVSWR_NotificationSet( uint16_t setPoint )
{
   PHY_SET_STATUS_e retVal = ePHY_SET_INVALID_PARAMETER;

   if ( ( 2U * 10U <= setPoint ) && ( setPoint <= 10U * 10U ) )
   {
      ConfigAttr.fngVswrNotificationSet = setPoint;
      writeConfig();
      retVal = ePHY_SET_SUCCESS;
   }
   return retVal;
}

/*!
  Use to set transmitter shutown set point for VSWR
*/
static PHY_SET_STATUS_e TxVSWR_ShutdownSet( uint16_t setPoint )
{
   PHY_SET_STATUS_e retVal = ePHY_SET_INVALID_PARAMETER;

   if ( ( 2U * 10U <= setPoint ) && ( setPoint <= 10U * 10U ) )
   {
      ConfigAttr.fngVswrShutdownSet = setPoint;
      writeConfig();
      retVal = ePHY_SET_SUCCESS;
   }
   return retVal;
}

/*!
  Use to set Notification set point for forward power
*/
static PHY_SET_STATUS_e TxVFowardPowerSet( uint16_t setPoint )
{
   PHY_SET_STATUS_e retVal = ePHY_SET_INVALID_PARAMETER;

   if ( ( 28U * 10U <= setPoint ) && ( setPoint <= 36U * 10U ) )
   {
      ConfigAttr.fngForwardPowerSet = setPoint;
      writeConfig();
      retVal = ePHY_SET_SUCCESS;
   }
   return retVal;
}

/*!
  Use to set transmitter shutdown set point for forward power
*/
static PHY_SET_STATUS_e TxLowForwardPowerSet( uint16_t setPoint )
{
   PHY_SET_STATUS_e retVal = ePHY_SET_INVALID_PARAMETER;

   if ( ( 27U * 10U <= setPoint ) && ( setPoint <= 35U * 10U ) )
   {
      ConfigAttr.fngFowardPowerLowSet = setPoint;
      writeConfig();
      retVal = ePHY_SET_SUCCESS;
   }
   return retVal;
}

/*!
  Use to set FEM manufacturer
*/
static PHY_SET_STATUS_e setFEMmfg( uint8_t mfg )
{
   PHY_SET_STATUS_e retVal = ePHY_SET_INVALID_PARAMETER;

   if ( mfg <= MICRO_ANT )
   {
      ConfigAttr.fngfemMfg = mfg;
      writeConfig();
      retVal = ePHY_SET_SUCCESS;
   }
   return retVal;
}

#endif

#if ( DCU == 1 )
/*!
  Use to enable/disable the TB transmitter
*/
static PHY_SET_STATUS_e setTxEnabled( uint8_t enable )
{
   PHY_SET_STATUS_e retVal = ePHY_SET_INVALID_PARAMETER;

   if ( enable <= 1U )
   {
      ConfigAttr.txEnabled = enable;
      writeConfig();
      retVal = ePHY_SET_SUCCESS;
   }
   return retVal;
}
#endif
/*!
 * Used to reset some statistics when attributes related to channels are changed.
 */
static void Reset_ReceiveStats( void )
{
   (void)memset(CachedAttr.FailedFrameDecodeCount , 0, sizeof(CachedAttr.FailedFrameDecodeCount));
   (void)memset(CachedAttr.FailedHeaderDecodeCount, 0, sizeof(CachedAttr.FailedHeaderDecodeCount));
   (void)memset(CachedAttr.FailedHcsCount         , 0, sizeof(CachedAttr.FailedHcsCount));
   (void)memset(CachedAttr.FramesReceivedCount    , 0, sizeof(CachedAttr.FramesReceivedCount));
   (void)memset(CachedAttr.MalformedHeaderCount   , 0, sizeof(CachedAttr.MalformedHeaderCount));
   (void)memset(CachedAttr.PreambleDetectCount    , 0, sizeof(CachedAttr.PreambleDetectCount));
   (void)memset(CachedAttr.RadioWatchdogCount     , 0, sizeof(CachedAttr.RadioWatchdogCount));
   (void)memset(CachedAttr.RssiJumpCount          , 0, sizeof(CachedAttr.RssiJumpCount));
   (void)memset(CachedAttr.RxAbortCount           , 0, sizeof(CachedAttr.RxAbortCount));
   (void)memset(CachedAttr.SyncDetectCount        , 0, sizeof(CachedAttr.SyncDetectCount));

   writeCache();
}

/*!
 * Used to set the PHY RX list
 */
static PHY_SET_STATUS_e RxChannels_Set( uint16_t const *RxChannels )
{
   uint32_t i; // Loop counter
   uint8_t  radioNum;
   uint16_t OldRxChannels[PHY_RCVR_COUNT];      /*!< List of the Receive Channels */

   // Validate all inputs
   for ( i = 0; i < PHY_RCVR_COUNT; i++ )
   {
#if ( DCU == 1 ) && ( RADIO_FIRST_RX != 0 )
      // Some radios that can't RX must have an invalid channel
      if ( (i < (uint32_t)RADIO_FIRST_RX) && (RxChannels[i] != PHY_INVALID_CHANNEL) )
      {  // Invalid parameter
         return ePHY_SET_INVALID_PARAMETER;
      }
#endif
      if (RxChannels[i] > PHY_INVALID_CHANNEL)
      {  // Invalid parameter
         return ePHY_SET_INVALID_PARAMETER;
      }
   }

   // Save Rx channels
   // We need to do that because we need to update the RX list before we call radio.start
   // because if we call radio.start with a channel that used to be invalid,
   // a check inside radio.start will fail and prevent the radio from starting.
   (void)memcpy(OldRxChannels, ConfigAttr.RxChannels, sizeof(OldRxChannels));

   // Update with new settings
   (void)memcpy(ConfigAttr.RxChannels, RxChannels, sizeof(ConfigAttr.RxChannels));
   writeConfig();

   // Make sure we are on a state where we can update radios
   if (_phy.state != ePHY_STATE_SHUTDOWN) {
      // Update radio state
      for (radioNum=(uint8_t)RADIO_FIRST_RX; radioNum<PHY_RCVR_COUNT; radioNum++) {
         // Validate channel
         if (PHY_IsChannelValid(RxChannels[radioNum]) || (RxChannels[radioNum] == PHY_INVALID_CHANNEL)) {
            Reset_ReceiveStats();
            // Check if new setting is different then current settings
            if (OldRxChannels[radioNum] != RxChannels[radioNum]) {
               switch (_phy.state) {
                  case ePHY_STATE_STANDBY:
                  case ePHY_STATE_READY_TX:
                     (void) Radio.SleepRx(radioNum);
                     break;

                  case ePHY_STATE_READY:
                  case ePHY_STATE_READY_RX:
                     if (RxChannels[radioNum] == PHY_INVALID_CHANNEL) {
                        (void) Radio.SleepRx(radioNum);
                     } else {
                        (void) Radio.StartRx(radioNum, RxChannels[radioNum]);
                        // A channel was started. Update noise floor in case this is the first time we have a receiver.
                        ChannelAdded = true;
#if ( DCU == 1 )
                        (void)memset(&PhyRssiStats10m[radioNum], 0, sizeof(PHY_RssiStats_s)); // Reset the RSSI stats for the radio since we have a new receive channel
                        (void)memset(&PhyRssiStats60m[radioNum], 0, sizeof(PHY_RssiStats_s)); // Reset the RSSI stats for the radio since we have a new receive channel
#if ( MAC_LINK_PARAMETERS == 1 )
                        (void)memset(&PhyRssiStats10m_Copy_[radioNum], 0, sizeof(PHY_RssiStats_s)); // Reset the RSSI stats for the radio since we have a new receive channel
                        RPT_PRNT_INFO('I',"BxRSSI10mCopy Reset for Radio %u (new channel)",radioNum );
#endif
                        //#if ( PRINT_RSSI_STATS_DBG_MESSAGES >= 1 )
                        INFO_printf("Background RSSI Stats reset for Radio %u (new channel)",radioNum);
                        //#endif
#endif
                     }
                     break;

                  case ePHY_STATE_SHUTDOWN: // shut up lint
                  default: break;
               }
            }
         } else {
            (void) Radio.SleepRx(radioNum);
         }
      }
   }

   return ePHY_SET_SUCCESS;
}

/***********************************************************************************************************************
Function Name: RxDetectionConfig_Set

Purpose: Configure each radio with the RX dectection configuration

Arguments: RxDetectionConfig - configuration for each radio

Returns: None
***********************************************************************************************************************/
static PHY_SET_STATUS_e RxDetectionConfig_Set( PHY_DETECTION_e const *RxDetectionConfig )
{
   uint32_t i; // Loop counter
   uint8_t  radioNum;

   // Validate all inputs
   for ( i = 0; i < PHY_RCVR_COUNT; i++ )
   {
      if (RxDetectionConfig[i] >= ePHY_DETECTION_LAST)
      {  // Invalid parameter
         return ePHY_SET_INVALID_PARAMETER;
      }
   }

   // Update with new settings
   (void)memcpy(ConfigAttr.RxDetectionConfig, RxDetectionConfig, sizeof(ConfigAttr.RxDetectionConfig));
   writeConfig();

   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      if (RADIO_RxDetectionConfig(radioNum, RxDetectionConfig[radioNum], (bool)false)) {
         // Restart radio if needed
         if (RADIO_RxChannelGet(radioNum) != PHY_INVALID_CHANNEL) {
            Reset_ReceiveStats();
            // A valid channel means the radio was configured as a receiver
            (void)Radio.StartRx(radioNum, RADIO_RxChannelGet(radioNum));
         }
      }
   }

   return ePHY_SET_SUCCESS;
}

/***********************************************************************************************************************
Function Name: RxFramingConfig_Set

Purpose: Configure each radio with the RX framing configuration

Arguments: RxFramingConfig - configuration for each radio

Returns: None
***********************************************************************************************************************/
static PHY_SET_STATUS_e RxFramingConfig_Set( PHY_FRAMING_e const *RxFramingConfig )
{
   uint32_t i; // Loop counter
   uint8_t  radioNum;

   // Validate all inputs
   for ( i = 0; i < PHY_RCVR_COUNT; i++ )
   {
      if (RxFramingConfig[i] >= ePHY_FRAMING_LAST)
      {  // Invalid parameter
         return ePHY_SET_INVALID_PARAMETER;
      }
   }

   // Update with new settings
   (void)memcpy(ConfigAttr.RxFramingConfig, RxFramingConfig, sizeof(ConfigAttr.RxFramingConfig));
   writeConfig();

   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      if (RADIO_RxFramingConfig(radioNum, RxFramingConfig[radioNum], (bool)false)) {
         // Restart radio if needed
         if (RADIO_RxChannelGet(radioNum) != PHY_INVALID_CHANNEL) {
            Reset_ReceiveStats();
            // A valid channel means the radio was configured as a receiver
            (void)Radio.StartRx(radioNum, RADIO_RxChannelGet(radioNum));
         }
      }
   }

   return ePHY_SET_SUCCESS;
}

/***********************************************************************************************************************
Function Name: RxMode_Set

Purpose: Configure each radio with the RX mode configuration

Arguments: RxMode - configuration for each radio

Returns: None
***********************************************************************************************************************/
static PHY_SET_STATUS_e RxMode_Set( PHY_MODE_e const *RxMode )
{
   uint32_t i; // Loop counter
   uint8_t  radioNum;

   // Validate all inputs
   for ( i = 0; i < PHY_RCVR_COUNT; i++ )
   {
      if ((RxMode[i] == ePHY_MODE_0) || (RxMode[i] >= ePHY_MODE_LAST))
      {  // Invalid parameter
         return ePHY_SET_INVALID_PARAMETER;
      }
   }

   // Update with new settings
   (void)memcpy(ConfigAttr.RxMode, RxMode, sizeof(ConfigAttr.RxMode));
   writeConfig();

   for (radioNum=(uint8_t)RADIO_0; radioNum<(uint8_t)MAX_RADIO; radioNum++) {
      if (RADIO_RxModeConfig(radioNum, RxMode[radioNum], (bool)false)) {
         // Restart radio if needed
         if (RADIO_RxChannelGet(radioNum) != PHY_INVALID_CHANNEL) {
            Reset_ReceiveStats();
            // A valid channel means the radio was configured as a receiver
            (void)Radio.StartRx(radioNum, RADIO_RxChannelGet(radioNum));
         }
      }
   }

   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the Available channel list
 */
static PHY_SET_STATUS_e Channels_Set( uint16_t const *Channels )
{
   (void)memcpy(ConfigAttr.Channels, Channels, sizeof(ConfigAttr.Channels));
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the Tx list
 */
static PHY_SET_STATUS_e TxChannels_Set( uint16_t const *TxChannels )
{
   (void)memcpy(ConfigAttr.TxChannels, TxChannels, sizeof(ConfigAttr.TxChannels));
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the Noise Estimate
 */
static PHY_SET_STATUS_e NoiseEstimate_Set( int16_t const * NoiseEstimate )
{
   (void)memcpy(CachedAttr.NoiseEstimate, NoiseEstimate, sizeof(CachedAttr.NoiseEstimate));
   writeCache();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the Noise Estimate Boost On
 */
#if ( EP == 1 )
static PHY_SET_STATUS_e NoiseEstimateBoostOn_Set( int16_t const * NoiseEstimate )
{
   (void)memcpy(CachedAttr.NoiseEstimateBoostOn, NoiseEstimate, sizeof(CachedAttr.NoiseEstimateBoostOn));
   writeCache();
   return ePHY_SET_SUCCESS;
}
#endif
/*!
 * Used to set the Noise Estimate Rate
 */
static PHY_SET_STATUS_e NoiseEstimateRate_Set( uint8_t val )
{
   ConfigAttr.NoiseEstimateRate = val;

   // Reconfigure the timer as long as the timer is valid
   if ( NoiseEstimateRateTmrID ) {
      if ( ConfigAttr.NoiseEstimateRate ) {
         (void)TMR_ResetTimer( NoiseEstimateRateTmrID, ConfigAttr.NoiseEstimateRate*ONE_MIN );
      } else {
         // We can't use TMR_ResetTimer with 0 because it won't stop the timer. It would only restart it.
         (void)TMR_StopTimer( NoiseEstimateRateTmrID );
      }
   }
   writeConfig();
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to set the Noise Estimate count
 */
static PHY_SET_STATUS_e NoiseEstimateCount_Set( uint8_t val )
{
   if ((val >= PHY_NOISE_ESTIMATE_COUNT_MIN) && (val <= PHY_NOISE_ESTIMATE_COUNT_MAX)) {
      ConfigAttr.NoiseEstimateCount = val;
      writeConfig();
      return ePHY_SET_SUCCESS;
   }
   else
   {  // Invalid parameter
      return ePHY_SET_INVALID_PARAMETER;
   }
}

/*!
 * Used to set the power setting
 */
static PHY_SET_STATUS_e PowerSetting_Set( uint8_t val )
{
   ConfigAttr.PowerSetting = val;
   writeConfig();
   RADIO_Power_Level_Set(val);
   return ePHY_SET_SUCCESS;
}

/*!
 * Used to get the radio temperature
 */
static PHY_GET_STATUS_e Temperature_Get( int16_t *val )
{
   if ( RADIO_Temperature_Get( (uint8_t) RADIO_0, val) ) {
      return ePHY_GET_SUCCESS;
   } else {
      return ePHY_GET_SERVICE_UNAVAILABLE;
   }
}

uint16_t  PHY_GetMaxTxPayload(void)               { return(ConfigAttr.MaxTxPayload); }

/*********************************************************************************************************************
 * Process a CCA Request message
 ********************************************************************************************************************/
static bool Process_CCAReq( PHY_Request_t const *pReq )
{
   uint8_t  rssi =    0;
   int16_t  rssi_dbm = 0;
   bool     busy = false;
   uint32_t i; // Loop counter
   PHY_CCA_STATUS_e CCAstatus;

   INFO_printf("CCAReq channel=%d", pReq->CcaReq.channel);

   // Only allowed in ready state
   if((_phy.state == ePHY_STATE_READY) || (_phy.state == ePHY_STATE_READY_TX))
   {
      // Valid parameter
      if (pReq->CcaReq.channel < PHY_INVALID_CHANNEL) {
         // Cause the radio to get RSSI
         CCAstatus = Radio.Do_CCA(pReq->CcaReq.channel, &rssi);

         // Check for success
         if (CCAstatus == ePHY_CCA_SUCCESS) {
            rssi_dbm = (int16_t)RSSI_RAW_TO_DBM(rssi);

            if ((ConfigAttr.CcaAdaptiveThresholdEnable == true) && (ConfigAttr.NoiseEstimateRate != 0)) {
               // Find channel offset used by CCA computation
               for (i=0; i<PHY_MAX_CHANNEL; i++) {
                  if (ConfigAttr.Channels[i] == pReq->CcaReq.channel)
                     break;
               }

               if (i == PHY_MAX_CHANNEL) { // this should not happen
                  CCAstatus = ePHY_CCA_INVALID_CONFIG;
               }  else {
                  int16_t noiseEstimate;

                  // Get noise estimate value
                  noiseEstimate = CachedAttr.NoiseEstimate[i];
#if ( EP == 1 )
                  // Use a different noise estimate when in last gasp
#if 0 // TODO: RA6E1 Bob: defer this until the Last Gasp module is ready
                  if ( PWRLG_LastGasp() ) {
                     noiseEstimate = CachedAttr.NoiseEstimateBoostOn[i];
                  }
#endif
#endif
                  if (rssi_dbm >= (noiseEstimate+ConfigAttr.CcaOffset)) { // Test rssi against the noise of this channel
                     // channel is busy
                     busy = true;
                  }
                  INFO_printf("CCAReq: busy:%u rssi:%d NoiseEstimate:%d CcaOffset:%d", (uint32_t)busy, rssi_dbm, noiseEstimate, ConfigAttr.CcaOffset);
               }
            } else {
               if(rssi_dbm >= ConfigAttr.CcaThreshold) {
                  // channel is busy
                  busy = true;
               }
               INFO_printf("CCAReq: busy:%u rssi:%d CcaThreshold:%d", (uint32_t)busy, rssi_dbm, ConfigAttr.CcaThreshold);
            }
         }
      } else {
         CCAstatus = ePHY_CCA_INVALID_PARAMETER;
      }
   } else {
      CCAstatus = ePHY_CCA_SERVICE_UNAVAILABLE;
   }

   // Create the confirmation
   PhyConfMsg.MsgType         = ePHY_CCA_CONF;
   PhyConfMsg.CcaConf.busy    = busy;
   PhyConfMsg.CcaConf.rssi    = rssi;
   PhyConfMsg.CcaConf.eStatus = CCAstatus;

   return true;
}


/*********************************************************************************************************************
 * Process a Set Request message
 *
 * returns:
 *    ePHY_SET_SUCCESS
 *    ePHY_SET_READONLY
 *    ePHY_SET_UNSUPPORTED
 *    ePHY_SET_INVALID_PARAMETER
 *    ePHY_SET_???                  - not allowed at this time
 *
 *********************************************************************************************************************/
static bool Process_SetReq( PHY_Request_t const *pReq )
{
   PhyConfMsg.MsgType = ePHY_SET_CONF;

   // Not supported in shutdown
   if(_phy.state != ePHY_STATE_SHUTDOWN) {
      PhyConfMsg.SetConf.eStatus = PHY_Attribute_Set(&pReq->SetReq);
   } else {
      PhyConfMsg.SetConf.eStatus = ePHY_SET_SERVICE_UNAVAILABLE;
   }

   return true;
}

/********************************************************************************************************************
 * Process a Get Request message
 *
 * returns:
 *    ePHY_GET_SUCCESS,
 *    ePHY_GET_UNSUPPORTED
 *
 ********************************************************************************************************************/
static bool Process_GetReq( PHY_Request_t const *pReq )
{
   PhyConfMsg.MsgType = ePHY_GET_CONF;

   // Not supported in shutdown
   if( _phy.state != ePHY_STATE_SHUTDOWN ) {
      PhyConfMsg.GetConf.eStatus    = PHY_Attribute_Get( &pReq->GetReq, &PhyConfMsg.GetConf.val);
      PhyConfMsg.GetConf.eAttribute = pReq->GetReq.eAttribute;
   } else{
      PhyConfMsg.GetConf.eStatus = ePHY_GET_SERVICE_UNAVAILABLE;
   }
   return true;
}

/*********************************************************************************************************************
 * Process a Start Request message

   options:
      ePHY_START_STANDBY
      ePHY_START_READY
      ePHY_START_READY_TX
      ePHY_START_READY_RX

   returns:
      ePHY_START_SUCCESS
      ePHY_START_FAILURE
      ePHY_START_INVALID_PARAMETER


 ********************************************************************************************************************
 */
static bool Process_StartReq( PHY_Request_t const *pReq )
{
   PhyConfMsg.MsgType = ePHY_START_CONF;

   INFO_printf("StartReq:%s", PHY_StartStateMsg(pReq->StartReq.state));

   // Make sure requested state is valid
   if (!((pReq->StartReq.state == ePHY_START_STANDBY)  ||
         (pReq->StartReq.state == ePHY_START_READY   ) ||
         (pReq->StartReq.state == ePHY_START_READY_RX) ||
         (pReq->StartReq.state == ePHY_START_READY_TX)))
   {
      PhyConfMsg.StartConf.eStatus = ePHY_START_INVALID_PARAMETER;
   }
   else if ((uint32_t)_phy.state == (uint32_t)pReq->StartReq.state) // Already in the requested stated?
   {
      PhyConfMsg.StartConf.eStatus = ePHY_START_SUCCESS;
   }
   else
   {
      // Need to change state

      // Initialize Radios if we are transitioning from shutdown
      if(_phy.state == ePHY_STATE_SHUTDOWN) {
         // Initialize the radios
         (void) Radio.Init(RadioEvent_Set);
      }

      // Transition from current state ...
      switch (_phy.state) {
         case ePHY_STATE_STANDBY:
            // To requested state
            switch (pReq->StartReq.state) {
               case ePHY_START_READY:    Radio.Ready();   break;
               case ePHY_START_READY_TX: Radio.ReadyTx(); break;
               case ePHY_START_READY_RX: Radio.ReadyRx(); break;
               case ePHY_START_STANDBY: // shut up lint
               default: break;
            }
            break;

         case ePHY_STATE_READY:
            // To requested state
            switch (pReq->StartReq.state) {
               case ePHY_START_STANDBY:  Radio.Standby();   break;
               case ePHY_START_READY_TX: Radio.StandbyRx(); break;
               case ePHY_START_READY_RX: Radio.StandbyTx(); break;
               case ePHY_START_READY: // shut up lint
               default: break;
            }
            break;

         case ePHY_STATE_READY_TX:
            // To requested state
            switch (pReq->StartReq.state) {
               case ePHY_START_STANDBY:  Radio.Standby();                    break;
               case ePHY_START_READY:    Radio.ReadyRx();                    break;
               case ePHY_START_READY_RX: Radio.StandbyTx(); Radio.ReadyRx(); break;
               case ePHY_START_READY_TX: // shut up lint
               default: break;
            }
            break;

         case ePHY_STATE_READY_RX:
            // To requested state
            switch (pReq->StartReq.state) {
               case ePHY_START_STANDBY:  Radio.Standby();                    break;
               case ePHY_START_READY:    Radio.ReadyTx();                    break;
               case ePHY_START_READY_TX: Radio.StandbyRx(); Radio.ReadyTx(); break;
               case ePHY_START_READY_RX: // shut up lint
               default: break;
            }
            break;

         case ePHY_STATE_SHUTDOWN:
            // To requested state
            switch (pReq->StartReq.state) {
               case ePHY_START_STANDBY:  Radio.Standby(); break;
               case ePHY_START_READY:    Radio.Ready();   break;
               case ePHY_START_READY_TX: Radio.ReadyTx(); break;
               case ePHY_START_READY_RX: Radio.ReadyRx(); break;
               default: break;
            }
            break;

         default: break;
      }

      // This always succeed
      _phy.state = (PHY_STATE_e)pReq->StartReq.state;
      PhyConfMsg.StartConf.eStatus = ePHY_START_SUCCESS;
   }
   INFO_printf("%s", PHY_StateMsg(_phy.state));
   return true;
}

/*********************************************************************************************************************
 * Process a Stop Request message

   options:
      none

   returns:
      ePHY_STOP_SUCCESS
      ePHY_STOP_FAILURE

 ********************************************************************************************************************/
static bool Process_StopReq( PHY_Request_t const *pReq )
{
   (void) pReq;  // Not used

   INFO_printf("StopReq");

   // We will always send confirm
   PhyConfMsg.MsgType = ePHY_STOP_CONF;

   if (_phy.state == ePHY_STATE_SHUTDOWN)
   {
      PhyConfMsg.StopConf.eStatus = ePHY_STOP_SERVICE_UNAVAILABLE;
   } else {
      // Stop the PHY
      Radio.Shutdown();
      _phy.state = ePHY_STATE_SHUTDOWN;
      PhyConfMsg.StopConf.eStatus = ePHY_STOP_SUCCESS;
   }
   INFO_printf("%s", PHY_StateMsg(_phy.state));
   return true;
}

/*********************************************************************************************************************
 * Process a Reset Request message

   options:
      ePHY_RESET_ALL
      ePHY_RESET_STATISTICS

   returns:
      ePHY_RESET_SUCCESS
      ePHY_RESET_FAILURE

 ********************************************************************************************************************
 */
static bool Process_ResetReq( PHY_Request_t const *pReq )
{
   uint8_t radioNum;
   int16_t NoiseEstimate[PHY_MAX_CHANNEL];               /*!< Noise floor value for each channel */
#if ( EP == 1 )
   int16_t NoiseEstimateBoostOn[PHY_MAX_CHANNEL];        /*!< Noise floor value for each channel when the boost capacitor is turned ON */
#endif

   PhyConfMsg.MsgType = ePHY_RESET_CONF;

   INFO_printf("ResetReq:%d", pReq->ResetReq.eType);

   // Not supported in shutdown
   if(_phy.state != ePHY_STATE_SHUTDOWN)
   {
      // Validate range
      if((pReq->ResetReq.eType == ePHY_RESET_ALL) || (pReq->ResetReq.eType == ePHY_RESET_STATISTICS))
      {
         // Save these values because they will be erased by memset but we want to keep them
         (void)memcpy( NoiseEstimate, CachedAttr.NoiseEstimate, sizeof( NoiseEstimate ));
#if ( EP == 1 )
         (void)memcpy( NoiseEstimateBoostOn, CachedAttr.NoiseEstimateBoostOn, sizeof( NoiseEstimateBoostOn ));
#endif
         // Reset statistics
         (void)memset(&CachedAttr, 0, sizeof(CachedAttr));

         // Restore previous values because these are not considered statistics.
         (void)memcpy( CachedAttr.NoiseEstimate, NoiseEstimate, sizeof( CachedAttr.NoiseEstimate ));
#if ( EP == 1 )
         (void)memcpy( CachedAttr.NoiseEstimateBoostOn, NoiseEstimateBoostOn, sizeof( CachedAttr.NoiseEstimateBoostOn ));
#endif

         // Reset the rest of attributes
         if(pReq->ResetReq.eType == ePHY_RESET_ALL)
         {
            (void)memset( ShadowAttr.AfcAdjustment, 0, sizeof( ShadowAttr.AfcAdjustment ));
            (void)memset( ConfigAttr.AfcAdjustment, 0, sizeof( ConfigAttr.AfcAdjustment ));
#if ( EP == 1 )
            ConfigAttr.AfcEnable              = PHY_AFC_AFC_ENABLE_DEFAULT;
            ConfigAttr.AfcRssiThreshold       = PHY_AFC_RSSI_THRESHOLD_DEFAULT;
            ConfigAttr.AfcSampleCount         = PHY_AFC_SAMPLECNT_THRESHOLD_DEFAULT;
            ConfigAttr.AfcTemperatureRange[0] = PHY_AFC_TEMPERATURE_MIN_DEFAULT;
            ConfigAttr.AfcTemperatureRange[1] = PHY_AFC_TEMPERATURE_MAX_DEFAULT;
#endif
            ConfigAttr.CcaAdaptiveThresholdEnable = PHY_CCA_ADAPTIVE_THRESHOLD_ENABLE_DEFAULT;
            ConfigAttr.CcaOffset                  = PHY_CCA_OFFSET_DEFAULT;
            ConfigAttr.CcaThreshold               = PHY_CCA_THRESHOLD_DEFAULT;
            ConfigAttr.MinRxBytes                 = PHY_MIN_RX_BYTES;
            ConfigAttr.NoiseEstimateCount         = PHY_NOISE_ESTIMATE_COUNT_DEFAULT;
            ConfigAttr.NoiseEstimateRate          = PHY_NOISE_ESTIMATE_RATE_DEFAULT;
            ConfigAttr.RssiJumpThreshold          = PHY_RSSI_JUMP_THRESHOLD_DEFAULT;
            ConfigAttr.ThermalControlEnable       = TRUE;
            ConfigAttr.ThermalProtectionEnable    = TRUE;
            ConfigAttr.TxOutputPowerMax           = PHY_TX_OUTPUT_POWER_MAX;
            ConfigAttr.TxOutputPowerMin           = PHY_TX_OUTPUT_POWER_MIN;
            ConfigAttr.VerboseMode                = FALSE;

            if (FIO_fwrite( &PHYconfigFileHndl_, 0, (uint8_t*)&ConfigAttr, sizeof(ConfigAttr)) != eSUCCESS) {
               PhyConfMsg.ResetConf.eStatus = ePHY_RESET_FAILURE;
               return true;
            }
            // Reset TCXO to nominal value since AFC adjustment was reset
            for ( radioNum = (uint8_t)RADIO_0; radioNum < (uint8_t)MAX_RADIO; radioNum++ ) {
               (void)RADIO_TCXO_Set( radioNum, RADIO_TCXO_NOMINAL, eTIME_SYS_SOURCE_NONE, (bool)true );
            }
         }
         // Save time
         (void)TIME_UTIL_GetTimeInSecondsFormat(&CachedAttr.LastResetTime);

         if (FIO_fwrite( &PHYcachedFileHndl_, 0, (uint8_t*)&CachedAttr, sizeof(CachedAttr)) != eSUCCESS)
         {
            PhyConfMsg.ResetConf.eStatus = ePHY_RESET_FAILURE;
         } else
         {
            PhyConfMsg.ResetConf.eStatus = ePHY_RESET_SUCCESS;
#if ( DCU == 1 )
           /* This is a good time to flush all statistics on the T-board. */
           /* This is because if we lose power, the statistics are not flushed and since they are not flushed, the counters could appeared to have rooled backward next time they are read. */
           /* Flush all partitions (only cached actually flushed) and ensures all pending writes are completed */
           (void)PAR_partitionFptr.parFlush( NULL );
#endif
         }
      } else {
         PhyConfMsg.ResetConf.eStatus = ePHY_RESET_INVALID_PARAMETER;
      }
   } else {
      PhyConfMsg.ResetConf.eStatus = ePHY_RESET_SERVICE_UNAVAILABLE;
   }
   return true;
}

#define PHY_HEADER_VERSION_OFFSET          0u   /*!< Offset to the Version field */
#define PHY_HEADER_MODE_OFFSET             0u   /*!< Offset to the Mode    field */
#define PHY_HEADER_MODE_PARAMETERS_OFFSET  1u   /*!< Offset to the mode parameters  field */
#define PHY_HEADER_LENGTH_OFFSET           2u   /*!< Offset to the Length  field */
#define PHY_HEADER_HCS_OFFSET              3u   /*!< Offset to the Header Check Sequence field*/
#define PHY_HEADER_HPC_OFFSET              5u   /*!< Offset to the Parity Check Field field*/

/*! Get the Version from the PHY Header */
uint8_t HeaderVersion_Get(uint8_t const *data)
{
   return( data[PHY_HEADER_VERSION_OFFSET] & 0xF0) >> 4;
}

/*! Set the Version in the PHY Header */
static void HeaderVersion_Set(uint8_t *pData, uint8_t version)
{
// assert(mode <= 15)
   pData[PHY_HEADER_VERSION_OFFSET] = (pData[PHY_HEADER_VERSION_OFFSET] & 0xF) | (uint8_t)(version << 4);
}


/*! Get the Mode from the PHY Header */
PHY_MODE_e HeaderMode_Get(uint8_t const *pData)
{
   return (PHY_MODE_e) (pData[PHY_HEADER_MODE_OFFSET] & 0x0F);
}

/*! Set the mode in the PHY Header */
static void HeaderMode_Set(uint8_t *pData, PHY_MODE_e mode)
{
// assert(mode <= 14)
   pData[PHY_HEADER_MODE_OFFSET] = (pData[PHY_HEADER_MODE_OFFSET] & 0xF0) | ((uint8_t) mode & 0x0F);
}

/*! Get the Mode Parameters from the PHY Header */
PHY_MODE_PARAMETERS_e HeaderModeParameters_Get(uint8_t const *pData)
{
   return (PHY_MODE_PARAMETERS_e)pData[PHY_HEADER_MODE_PARAMETERS_OFFSET];
}

/*! Set the Mode Parameters in the PHY Header */
static void HeaderModeParameters_Set(uint8_t *pData, PHY_MODE_PARAMETERS_e modeParameter)
{
   pData[PHY_HEADER_MODE_PARAMETERS_OFFSET] = (uint8_t)modeParameter;
}

/*! Get the Length from the PHY Header */
uint8_t HeaderLength_Get(PHY_FRAMING_e framing, uint8_t const *pData)
{
   uint8_t length=0;

   // Get lentgth for SRFN
   if (framing == ePHY_FRAMING_0) {
      length = pData[PHY_HEADER_LENGTH_OFFSET];
   }
   // Get length for STAR Gen I
   else if (framing == ePHY_FRAMING_1) {
      length = (GenILength[pData[0] & 0x0F] + 1U) >> 1U; // Convert nibbles in bytes. Round up.
   }
   // Get length for STAR Gen II
   else if (framing == ePHY_FRAMING_2) {
      // This is a GEN II message
      length = (pData[2] >> 3) + 5; // + 5 is for header and CRC size

      switch (pData[1] & 0xF0) {
         case 0x40:
         case 0x50:
         case 0xC0:
         case 0xD0:
            length += 16;
            break;

         default:
            break;
      }
   }
   return length;
}

/*! Set the Length in the PHY Header */
static void HeaderLength_Set(uint8_t *pData, uint8_t length)
{
// assert ( length <= PHY_MAX_PAYLOAD);
   pData[PHY_HEADER_LENGTH_OFFSET] = length;
}

/*! Get the HeaderCheckSequence from the PHY Header */
static uint16_t HeaderCheckSequence_Get(uint8_t const *pData)
{
   return((pData[PHY_HEADER_HCS_OFFSET] << 8U) | (pData[PHY_HEADER_HCS_OFFSET+1U]));
}

/*! Set the HeaderCheckSequence in the PHY Header */
static void HeaderCheckSequence_Set(uint8_t *pData, uint16_t hcs)
{
   pData[PHY_HEADER_HCS_OFFSET]   = hcs >> 8;
   pData[PHY_HEADER_HCS_OFFSET+1] = hcs & 0xFF;
}

/*!
 *********************************************************************************************************************
 *
 * \fn       Process_DataReq
 *
 * \brief    Process a Data Request message
 *
 * \param    PHY_Request_t - Should be a DataRequest
 *
 * \return   true   - Confirm is ready
 *           false  - The command has been send to the radio
 *
 * \details
 *
 *    This function will populate the DataConfirm_t with the status
 *
 *    SUCCESS                      ePHY_DATA_SUCCESS,
 *    SERVICE_UNAVAILABLE          ePHY_DATA_SERVICE_UNAVAILABLE
 *    TRANSMISSION_FAILED          ePHY_DATA_TRANSMISSION_FAILED
 *    INVALID_PARAMETER            ePHY_DATA_INVALID_PARAMETER
 *    BUSY                         ePHY_DATA_BUSY
 *    THERMAL_OVERRIDE             ePHY_DATA_THERMAL_OVERRIDE
 *
 *********************************************************************************************************************
 */
static bool Process_DataReq( PHY_Request_t const *pReq )
{
   PHY_DATA_STATUS_e eStatus = ePHY_DATA_SUCCESS;
   bool        retVal = false;
   uint32_t    delay = 0;
   float32_t   power;
   uint32_t    RxTime;
   uint16_t    num_bytes = 0;

   // Ping 7200 bps
//   uint8_t msg[]={ 0x0A, 0x41, 0x48, 0x45, 0xCF, 0xC4, 0x04, 0x40, 0x89, 0x48, 0xC0, 0x05, 0xAF, 0x11, 0xBF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

   // Ping 2400 bps
//   uint8_t msg[]={ 0x4b, 0x4b, 0xcd, 0xcc, 0x4c, 0xcd, 0xca, 0xca, 0xcc, 0xb2, 0x52, 0x33, 0x33, 0x33, 0x33, 0x4b, 0x4b, 0x4b, 0x4b, 0x33, 0x35, 0xad, 0x34, 0x35, 0xb3, 0x34, 0xcd, 0xca, 0xb4, 0x8c};

   PHY_CounterInc(ePHY_DataRequestCount, 0);

   // Some check on power
#if ( DCU == 1 )
#if ( VSWR_MEASUREMENT == 1 )
   power = ( (uint32_t *)pReq->DataReq.power == NULL ? ConfigAttr.fngForwardPowerSet/10.0f : *pReq->DataReq.power );
   /* Force power setting to be within limits   */
   if ( power < FEM_MIN_POWER_DBM )
   {
      power = FEM_MIN_POWER_DBM;
   }
   else if ( power > FEM_MAX_POWER_DBM )
   {
      power = FEM_MAX_POWER_DBM;
   }
#else
   power = ( pReq->DataReq.power == NULL ? PHY_TX_OUTPUT_POWER_DEFAULT : *pReq->DataReq.power );
#endif

#else
   power = 0;
#endif
   // Some check on RxTime
#if ( MCU_SELECTED == NXP_K24 )
   RxTime = pReq->DataReq.RxTime == 0 ? 0 : (uint32_t)(((uint64_t)(pReq->DataReq.RxTime - DWT_CYCCNT))*1000/getCoreClock());
#elif ( MCU_SELECTED == RA6E1 )
   // TODO: RA6E1 [name_Suriya] - Modify variable to get core clock from a function
   RxTime = pReq->DataReq.RxTime == 0 ? 0 : (uint32_t)(((uint64_t)(pReq->DataReq.RxTime - DWT->CYCCNT))*1000/SystemCoreClock);
#endif
   INFO_printf("DataReq: %u msec channel=%u power=%d.%1u mode=%u modeParameters=%u framing=%u detection=%u len=%u prio=%u",
      RxTime, // print future time in msec
      pReq->DataReq.channel,
      (int32_t)power, (int32_t)(power*10)%10,
      pReq->DataReq.mode,
      pReq->DataReq.modeParameters,
      pReq->DataReq.framing,
      pReq->DataReq.detection,
      pReq->DataReq.payloadLength,
      pReq->DataReq.TxPriority);

   if((_phy.state == ePHY_STATE_READY) || (_phy.state == ePHY_STATE_READY_TX)
#if ( DCU == 1 )
      || ConfigAttr.txEnabled
#endif
      )
   {
      if((PHY_IsTXChannelValid(pReq->DataReq.channel))            && // Must be a valid channel
         (pReq->DataReq.payloadLength > 0)                        &&
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
         (pReq->DataReq.payloadLength == 76)                      &&
#else
         (pReq->DataReq.payloadLength <= ConfigAttr.MaxTxPayload) &&
#endif
         (pReq->DataReq.mode          < ePHY_MODE_LAST)           &&
         (pReq->DataReq.framing       < ePHY_FRAMING_LAST)        &&
         (pReq->DataReq.detection     < ePHY_DETECTION_LAST)      &&
         (power                       <= PHY_TX_OUTPUT_POWER_MAX) &&
         (power                       >= PHY_TX_OUTPUT_POWER_MIN)
         )
      {
         // Check that we are not already transmitting
         if (!RADIO_Is_TX()) {
            TX_FRAME_t *txFrame = RADIO_Transmit_Buffer_Get();

            txFrame->Version        = 0;                            // Set Protocol version
            txFrame->Mode           = pReq->DataReq.mode;           // Set Mode
#if ( PORTABLE_DCU == 1)
            if (currentMode == eRADIO_MODE_NORMAL_DEV_600) {
               txFrame->ModeParameters = ePHY_MODE_PARAMETERS_0;    // Force mode parameters to RS(63,59)
            } else {
               txFrame->ModeParameters = ePHY_MODE_PARAMETERS_1;    // Force mode parameters to RS(255,239). This is applicable to SRFN only.
            }
#else
            txFrame->ModeParameters = ePHY_MODE_PARAMETERS_1;       // Force mode parameters to RS(255,239). This is applicable to SRFN only.
#endif
            txFrame->Framing        = pReq->DataReq.framing;        // Set framing
            txFrame->Detection      = pReq->DataReq.detection;      // Set detection
#if ( TEST_TDMA == 1 )
            txFrame->Detection      = ePHY_DETECTION_5;             // Set for test. Short preamble.
#endif
            // Number of bytes of payload
            txFrame->Length = (uint8_t) pReq->DataReq.payloadLength;

            // Copy the request payload to the PHY payload buffer
            (void) memcpy(txFrame->Payload, pReq->DataReq.payload, pReq->DataReq.payloadLength);

            num_bytes = Frame_Encode(txFrame); // Compute CRC/FEC
#if ( DCU == 1)
            // Check if we have enough thermal margin to transmit
            delay = virtualTemperature_Delay();
            if ( ((delay > 0) && ( ConfigAttr.ThermalControlEnable )) || CachedAttr.ThermalProtectionEngaged )
            {
               // Can't transmit now
               INFO_printf("Thermal override: delay=%u, ThermalControlEnable=%u, ThermalProtectionEnaged=%u", delay, ConfigAttr.ThermalControlEnable, CachedAttr.ThermalProtectionEngaged);

               // if delay is 0 then it means we got here because ThermalProtectionEngaged is TRUE which means we need to wait a minimum time
               if ( delay == 0) {
                  delay = PHY_THERMAL_DELAY_DEFAULT;
               }
               eStatus = ePHY_DATA_THERMAL_OVERRIDE;
            } else {
               eStatus = Radio.SendData((uint8_t)RADIO_0, pReq->DataReq.channel, txFrame->Mode, txFrame->Detection, txFrame->Payload, pReq->DataReq.RxTime, pReq->DataReq.TxPriority, power, num_bytes);
            }
#else
            eStatus = Radio.SendData((uint8_t)RADIO_0, pReq->DataReq.channel, txFrame->Mode, txFrame->Detection, txFrame->Payload, pReq->DataReq.RxTime, pReq->DataReq.TxPriority, power, num_bytes);
#endif
         }else
         {
            eStatus = ePHY_DATA_BUSY;
            delay = PHY_MAX_FRAME*8*1000/9600; // Compute worst transmit case in ms. We could compute a better delay if we used TX start time, the message length and the current time.
         }
      }
      else
      {  // Invalid parameter
         eStatus = ePHY_DATA_INVALID_PARAMETER;
      }
   }
   else
   {  // Not in the ready state
      eStatus = ePHY_DATA_SERVICE_UNAVAILABLE;
   }

   // We are done so create the confirmation
   if ( eStatus != ePHY_DATA_SUCCESS )
   {
      PHY_CounterInc(ePHY_DataRequestRejectedCount, 0);
      PhyConfMsg.MsgType          = ePHY_DATA_CONF;
      PhyConfMsg.DataConf.eStatus = eStatus;
      PhyConfMsg.DataConf.delay   = delay;

      retVal = true;
   } else
   {
      // Update the virtual temperature
      virtualTemperature.lengthInBytes = num_bytes;

      // Since we are going to transmit, let the TxDone Send the Data Confirmation
      // Note: We return false here, because the TxDone event will cause the confirm to be sent

      // Check that another transmission acknowledge isn't pending.
      // This could happen if the TxDone event never happened.
      if( pDataRequestBuf != NULL)
      {  // The radio has a message that was transmitted
         // Free the request buffer
         BM_free(pDataRequestBuf);
      }
      pDataRequestBuf = pRequestBuf; // Save buffer
      pRequestBuf = NULL; // Release buffer
   }

   return retVal;
}

/***********************************************************************************************************************
 * Create a Data Indication
 ***********************************************************************************************************************/
static buffer_t *DataIndication_Create(RX_FRAME_t const * const rx_buffer, uint8_t num_bytes, uint16_t chan, uint8_t index)
{
   buffer_t *pBuf = BM_allocStack(sizeof(PHY_Indication_t) + num_bytes);
   if (pBuf != NULL)
   {
      PHY_Indication_t *pMsg = (PHY_Indication_t *)pBuf->data;  /*lint !e826  !e740  */
      pMsg->Type   = ePHY_DATA_IND;

      // Since we allocated space for the data, then set pointer to the data
      pMsg->DataInd.payload = (uint8_t*)&pBuf->data[(sizeof(PHY_Indication_t))];
      pMsg->DataInd.payloadLength      = num_bytes;
      pMsg->DataInd.timeStamp          = rx_buffer->syncTime;
      pMsg->DataInd.timeStampCYCCNT    = rx_buffer->syncTimeCYCCNT;
      pMsg->DataInd.rssi               = rx_buffer->rssi_dbm;
#if NOISE_TEST_ENABLED == 1
      pMsg->DataInd.danl               = rx_buffer->noise_dbm;
#else
      pMsg->DataInd.danl               = 0;
#endif
      pMsg->DataInd.channel            = chan;
      pMsg->DataInd.mode               = rx_buffer->mode;
      pMsg->DataInd.modeParameters     = rx_buffer->modeParameters;
      pMsg->DataInd.framing            = rx_buffer->framing;
      pMsg->DataInd.receiverIndex      = index;
      pMsg->DataInd.frequencyOffset    = rx_buffer->frequencyOffset;

      if (rx_buffer->framing == ePHY_FRAMING_0) {
         // Copy the payload to the message buffer for SRFN
         (void) memcpy(pMsg->DataInd.payload, &rx_buffer->Payload[PHY_HEADER_SIZE], num_bytes);
      } else {
         // Copy the payload to the message buffer for STAR Gen I and Gen II
         (void) memcpy(pMsg->DataInd.payload, rx_buffer->Payload, num_bytes);
      }
   }
   else{
      INFO_printf("failed to allocate memory" );
   }
   return(pBuf);
}

/*!
   |--------------------------------------------|
   | Section 3.1.3 - Modes of Operation         |
   |---------|------------|---------------------|------------|------------------------------------|
   |  Mode   | Modulation |      FEC            | Baud Rate  | Comments                           |
   |---------|------------|---------------------|------------|------------------------------------|
   | 0       |    4GFSK   |(63,59) Reed-Solomon |   9600     |                                    |
   |---------|------------|---------------------|------------|------------------------------------|
   | 1 – 14  |  Reserved  |      Reserved       |  Reserved  |                                    |
   |---------|------------|---------------------|------------|------------------------------------|
   | *15     |   None     |      None           |   None     |                                    |
   |---------|------------|---------------------|------------|------------------------------------|
   | Mode = 15 is a special mode used for AFC.  When receiving a PHY header specifying mode 15,   |
   |           the receiver knows that after the header the AFC pattern will be transmitted for   |
   |           a period of phyAfcDuration.                                                        |
   |----------------------------------------------------------------------------------------------|
*/

/*! Computes the Phy Header Check Sequence */
static uint16_t HeaderCheckSequence_Calc(uint8_t const *pData)
{
   return(CRC_16_PhyHeader ( (uint8_t*)pData, PHY_HEADER_HCS_OFFSET));
}

/*! Validate the Phy Header Check Sequence */
bool HeaderCheckSequence_Validate(uint8_t const *pData)
{
   return((HeaderCheckSequence_Get(pData) == CRC_16_PhyHeader ( (uint8_t*) pData, PHY_HEADER_HCS_OFFSET)));
}

/*! Computes the Phy Header Parity Check */
static void HeaderParityCheck_Calc(uint8_t const *pData)
{
   // Always encode header with RS(63,59)
   RS_Encode(RS_SRFN_63_59, pData, (uint8_t*)&pData[PHY_HEADER_HPC_OFFSET], PHY_HEADER_HPC_OFFSET);
}

/*! Validates the Phy Header Parity Check */
bool HeaderParityCheck_Validate(uint8_t *pData)
{
   return RS_Decode(RS_SRFN_63_59, pData, &pData[PHY_HEADER_HPC_OFFSET], PHY_HEADER_HPC_OFFSET);
}

/*! Computes the length of the frame */
uint16_t FrameLength_Get(PHY_FRAMING_e framing, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters, uint8_t const * const pData)
{
   uint8_t  segments;      // How many segments to break the payload into
   uint16_t length;
   RSCode_e rscode;     // RS code used for SRFN

   // Get SFRN payload length or STAR message length
   length = HeaderLength_Get(framing, pData);

   // Adjust length for SRFN by adding header length and FEC length
   if (mode == ePHY_MODE_1) {
      // Get RS code
      rscode = RS_GetCode(mode, modeParameters);

      // Find how many segments the payload needs to be broken into
      segments = (uint8_t)((length*8+((RS_GetMM(rscode)*RS_GetKK(rscode))-1))/(RS_GetMM(rscode)*RS_GetKK(rscode)));

      length = (uint16_t)(PHY_HEADER_SIZE+length+(segments*RS_GetParityLength(rscode)));
   }

   return length;
}

/*! Computes the Phy Frame Parity Check */
static uint16_t FrameParityCheck_Calc(uint8_t *pData, uint8_t num_bytes, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters)
{
   uint32_t i;                  // Loop counter
   uint32_t segments;           // How many segments to break the payload into
   uint32_t segLengthBits;      // Segment length in bits
   uint32_t segLengthBytes;     // Segment length in bytes
   uint32_t payloadLength = num_bytes; // Payload length
   uint32_t length;             // Length of the segment to tranmit
   uint32_t msgPos;             // Point to part of the message to encode
   uint32_t parityPos;          // Point to where to save the parity bits
   RSCode_e rscode;             // RS code used for SRFN

   // Nothing to do if length is 0
   if (num_bytes == 0) {
      return (0);
   }

   // Get RS code
   rscode = RS_GetCode(mode, modeParameters);

   // Find how many segments the payload needs to be broken into
   segments = (uint32_t)((num_bytes*8+((RS_GetMM(rscode)*RS_GetKK(rscode))-1))/(RS_GetMM(rscode)*RS_GetKK(rscode)));

   // Find the length of a segment
   segLengthBits = (num_bytes*8+(segments-1))/segments;
   segLengthBytes = (segLengthBits+7)/8;

   msgPos = 0;
   parityPos = num_bytes;

   // Encode payload in smaller segments
   for (i=0; i<segments; i++) {
      length = min(segLengthBytes, payloadLength);

      // Check that we have enough space to encode the segment
      if ((parityPos+RS_GetParityLength(rscode)) <= (PHY_MAX_FRAME-PHY_HEADER_SIZE)) {
         RS_Encode(rscode, &pData[msgPos], &pData[parityPos], (uint8_t)length);
      }

      // Adjust variables. Do this even if we didn't have enough space to encode.
      msgPos += length;
      parityPos += RS_GetParityLength(rscode);
      payloadLength -= length;
   }

   return((uint16_t)parityPos);
}

/*! Validates the Phy Frame Parity Check */
bool FrameParityCheck_Validate(uint8_t *pData, uint8_t num_bytes, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters)
{
   uint32_t i;                  // Loop counter
   uint32_t segments;           // How many segments to break the payload into
   uint32_t segLengthBits;      // Segment length in bits
   uint32_t segLengthBytes;     // Segment length in bytes
   uint32_t payloadLength = num_bytes; // Payload length
   uint32_t length;             // Length of the segment to tranmit
   uint32_t msgPos;             // Point to part of the message to encode
   uint32_t parityPos;          // Point to where to save the parity bits
   RSCode_e rscode;             // RS code used for SRFN

   // Get RS code
#if ( TEST_DEVIATION == 1)
   // We high jacked mode for deviation testing but need to overwrite the value here to properly decode a PHY frame
   rscode = RS_GetCode(ePHY_MODE_1, modeParameters);
#else
   rscode = RS_GetCode(mode, modeParameters);
#endif

   // Find how many segments the payload needs to be broken into
   segments = (uint32_t)((num_bytes*8+((RS_GetMM(rscode)*RS_GetKK(rscode))-1))/(RS_GetMM(rscode)*RS_GetKK(rscode)));

   // Find the length of a segment
   segLengthBits = (num_bytes*8+(segments-1))/segments;
   segLengthBytes = (segLengthBits+7)/8;

   msgPos = 0;
   parityPos = num_bytes;

   // Decode payload in smaller segments
   for (i=0; i<segments; i++) {
      length = min(segLengthBytes, payloadLength);

      if (!RS_Decode(rscode, &pData[msgPos], &pData[parityPos], (uint8_t)length)) {
         return(false);
      }

      // Adjust variables.
      msgPos += length;
      parityPos += RS_GetParityLength(rscode);
      payloadLength -= length;
   }

   return(true);
}


/*!
    This function encodes a PHY_FRAME into a buffer for transmitting

                   |-------------------------------------------------|
                   |                    PHY HEADER                   |
   |--------|------|---------|---------|---------|---------|---------|-----------------|
   |preamble| sync | ver|mode|   mode  |  length |  hcs    |  hpc    |   payload       |
   |        |      |         |Parameter|         |         |         |                 |
   |--------|------|---------|---------|---------|---------|---------|-----------------|
   |   96   |  32  |  4 | 4  |    8    |    8    |   16    |   24    | [ 0-MaxPayload] | bits
   |--------|------|---------|---------|---------|---------|---------|-----------------|
   |   12   |   4  |    1    |    1    |    1    |    2    |    3    | [ 1-MaxPayload] | bytes
   |--------|------|---------|---------|---------|---------|---------|-----------------|

 */
uint16_t Frame_Encode(TX_FRAME_t const *txFrame)
{
   uint16_t hcs ;    // Header Check Sequence

   uint8_t* pData = (uint8_t*)txFrame->Payload;
   uint16_t num_bytes = 0;

   // Encode SRFN frame
   if (txFrame->Mode == ePHY_MODE_1)
   {
      // Move payload to proper place
      (void)memmove(&pData[PHY_HEADER_SIZE], pData, txFrame->Length);

      // Encode the Version and Mode
      HeaderVersion_Set(pData, txFrame->Version );
      HeaderMode_Set(pData, txFrame->Mode );

      // Encode the mode parameters
      HeaderModeParameters_Set(pData, txFrame->ModeParameters);

      // Encode the Length
      HeaderLength_Set(pData, txFrame->Length);

      // Compute the HCS
      hcs = HeaderCheckSequence_Calc(pData);
      HeaderCheckSequence_Set(pData, hcs );

      // Calculate the header Parity Check
      HeaderParityCheck_Calc(pData);

      pData     += PHY_HEADER_SIZE;
      num_bytes  = PHY_HEADER_SIZE;

      // Compute the Frame Parity Check and append to the end of the frame
      num_bytes += FrameParityCheck_Calc(pData, txFrame->Length, txFrame->Mode, txFrame->ModeParameters);
   }
   // STAR Gen II 7200 bps
   else if (txFrame->Mode == ePHY_MODE_2) {
      uint32_t i; // Loop counter
      uint16_t CRC;

      num_bytes = txFrame->Length;
      // Compute CRC
      CRC = 0xFFFF;
      for (i=0; i<txFrame->Length; i++)
      {
         CRC = RnCrc_Gen(CRC, pData[i] & 0x0F);
         CRC = RnCrc_Gen(CRC, pData[i] >> 4);
      }
      // Save CRC
      pData[num_bytes++] = (uint8_t)CRC;
      pData[num_bytes++] = (uint8_t)(CRC>>8);

      // Compute FEC
      if ( (pData[2] & 0x4) == 0) {
#if 0
         // todo: 01/24/17 2:12 PM [MKD] - This code might not work for message length 43 and 44 bytes (not sure 45 is legal). Need to investigate. I know the decoder doesn't work so I put the old code back.
         RS_Encode(RS_STAR_63_59, pData, &pData[num_bytes], (uint8_t)num_bytes);
#else
         RSEncode (pData, (uint8_t)num_bytes);
#endif

         num_bytes += STAR_FEC_LENGTH;
      }
   }
   else
   {
      INFO_printf("FrameEncode - Mode %u is not supported", txFrame->Mode);
#if 0
      // Code not need but able to generate STAR Gen I message but it doesn't follow all the STAR  Gen I requierments.
      // Use to test the TB receivers.

      uint32_t i; // Loop counter
      uint16_t CRC;

      num_bytes = txFrame->Length;
      // Compute CRC
      CRC = 0xFFFF;
      for (i=0; i<txFrame->Length; i++)
      {
         CRC = RnCrc_Gen(CRC, pData[i] & 0x0F);
         CRC = RnCrc_Gen(CRC, pData[i] >> 4);
      }
      // Save CRC
      pData[num_bytes++] = (uint8_t)CRC;
      pData[num_bytes++] = (uint8_t)(CRC>>8);

      //these values start with 1 in MSB
      const char ManTable[] =
      {
          0xCC,   //0
          0xCD,   //1
          0xCB,   //2
          0xCA,   //3
          0xD3,   //4
          0xD2,   //5
          0xD4,   //6
          0xD5,   //7
          0xB3,   //8
          0xB2,   //9
          0xB4,   //A
          0xB5,   //B
          0xAC,   //C
          0xAD,   //D
          0xAB,   //E
          0xAA    //F
      };

      //process first byte separate since first bit always starts with 1
      db = *inbuffer&0x0F;               //read lower nibble
//      db = FlipBitsTable[db];
      *outbuffer = ManTable[db];         //convert to Manchester format always start with 1 in very first bit
      db = *inbuffer&0xF0;               //read upper nibble
      db >>= 4;                          //shift down
//      db = FlipBitsTable[db];
      db = ManTable[db];                 //convert to Manchester
      if((*outbuffer)&0x01)//lsb else 01 for msb              //if previous bit was 1
      {
          db = ~db;                        //then invert byte
      }
      outbuffer++;                       //next byte
      *outbuffer = db;                   //save data
      size--;
      //process remaining bytes
      for( ; size!=0; size--)
      {
          inbuffer++;
          db = *inbuffer&0x0F;               //read lower nibble
//          db = FlipBitsTable[db];
          db = ManTable[db];                 //convert to Manchester
          if((*outbuffer)&0x01)              //if previous bit was 1
          {
              db = ~db;                        //then invert byte
          }
          outbuffer++;                       //next byte
          *outbuffer = db;                   //save data
          db = *inbuffer&0xF0;               //read upper nibble
          db >>= 4;                          //shift down
//          db = FlipBitsTable[db];
          db = ManTable[db];                 //convert to Manchester
          if((*outbuffer)&0x01)              //if previous bit was 1
          {
              db = ~db;                        //then invert byte
          }
          outbuffer++;                       //next byte
          *outbuffer = db;                   //save data
      }
      num_bytes *= 2; // The message size double because of Manchester encoding
#endif
   }
   return(num_bytes);
}

#if 0
static void decode_frame(PHY_FRAME_t*  rxFrame, phy_buffer_t* rx_buffer)
{

   rxFrame->Version  = (rx_buffer->header.x & 0xF0) >> 4;
   rxFrame->Mode     = (rx_buffer->header.x & 0x0F);
   rxFrame->Length   =  rx_buffer->header.length;

   (void) memcpy(rxFrame->Payload, &rx_buffer->data, rxFrame->Length);
}
#endif


/*********************************************************************************************************************
 * Used to set a phy attribute
 * This does not block

   ePHY_SET_SUCCESS
   ePHY_SET_READONLY
   ePHY_SET_UNSUPPORTED
   ePHY_SET_INVALID_PARAMETER
   ePHY_SET_SERVICE_UNAVAILABLE

 *********************************************************************************************************************/
PHY_SET_STATUS_e  PHY_Attribute_Set( PHY_SetReq_t const *pSetReq)
{
   PHY_SET_STATUS_e eStatus = ePHY_SET_SERVICE_UNAVAILABLE;

#if 0
   // This function should only be called inside the PHY task
   if ( _task_get_id() != _task_get_id_from_name( "PHY" ) ) {
     ERR_printf("WARNING: PHY_Attribute_Set should only be called from the PHY task. Please use PHY_SetRequest instead.");
   }
#endif
   switch(pSetReq->eAttribute) /*lint !e788 not all enums used within switch */
   {
   // Writeable Attributes
      case ePhyAttr_AfcAdjustment:              eStatus = ePHY_SET_READONLY;                                                        break;
#if ( EP == 1 )
      case ePhyAttr_AfcEnable:                  eStatus = AfcEnable_Set(          pSetReq->val.AfcEnable);                          break;
      case ePhyAttr_AfcRssiThreshold:           eStatus = AfcRssiThreshold_Set(   pSetReq->val.AfcRssiThreshold);                   break;
      case ePhyAttr_AfcTemperatureRange:        eStatus = AfcTemperatureRange_Set((int8_t *)pSetReq->val.AfcTemperatureRange);      break;
#endif
      case ePhyAttr_AvailableChannels:          eStatus = Channels_Set( pSetReq->val.AvailableChannels );                           break;
      case ePhyAttr_CcaAdaptiveThreshold:       eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_CcaAdaptiveThresholdEnable: eStatus = CcaAdaptiveThresholdEnable_Set( pSetReq->val.CcaAdaptiveThresholdEnable); break;
      case ePhyAttr_CcaOffset:                  eStatus = CcaOffset_Set( pSetReq->val.CcaOffset);                                   break;
      case ePhyAttr_CcaThreshold:               eStatus = CcaThreshold_Set( pSetReq->val.CcaThreshold);                             break;
      case ePhyAttr_DataRequestCount:           eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_DataRequestRejectedCount:   eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_Demodulator:                eStatus = Demodulator_Set((uint8_t*)pSetReq->val.Demodulator);                      break;
      case ePhyAttr_FailedFrameDecodeCount:     eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_FailedHcsCount:             eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_FailedHeaderDecodeCount:    eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_FailedTransmitCount:        eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_FramesReceivedCount:        eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_FramesTransmittedCount:     eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_FrontEndGain:               eStatus = FrontEndGain_Set ( pSetReq->val.FrontEndGain);                            break;
      case ePhyAttr_LastResetTime:              eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_MalformedHeaderCount:       eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_MinRxBytes:                 eStatus = MinRxBytes_Set( pSetReq->val.MinRxBytes);                                 break;
      case ePhyAttr_MaxTxPayload:               eStatus = MaxTxPayload_Set( pSetReq->val.MaxTxPayload);                             break;
      case ePhyAttr_NoiseEstimate:              eStatus = NoiseEstimate_Set( pSetReq->val.NoiseEstimate);                           break;
#if ( EP == 1 )
      case ePhyAttr_NoiseEstimateBoostOn:       eStatus = NoiseEstimateBoostOn_Set(pSetReq->val.NoiseEstimateBoostOn);              break;
#endif
      case ePhyAttr_NoiseEstimateCount:         eStatus = NoiseEstimateCount_Set(pSetReq->val.NoiseEstimateCount);                  break;
      case ePhyAttr_NoiseEstimateRate:          eStatus = NoiseEstimateRate_Set(pSetReq->val.NoiseEstimateRate);                    break;
      case ePhyAttr_NumChannels:                eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_PowerSetting:               eStatus = PowerSetting_Set(pSetReq->val.PowerSetting);                              break;
      case ePhyAttr_PreambleDetectCount:        eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_RadioWatchdogCount:         eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_RcvrCount:                  eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_RssiJumpCount:              eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_RssiJumpThreshold:          eStatus = RssiJumpThreshold_Set(pSetReq->val.RssiJumpThreshold);                    break;
#if ( DCU == 1 )
      case ePhyAttr_RssiStats10m:               (void)memcpy(PhyRssiStats10m, pSetReq->val.RssiStats, sizeof(PhyRssiStats10m));
                                                eStatus = ePHY_SET_SUCCESS;                                                         break;
#if ( MAC_LINK_PARAMETERS == 1 )
      case ePhyAttr_RssiStats10m_Copy:          (void)memcpy(PhyRssiStats10m_Copy_, pSetReq->val.RssiStats, sizeof(PhyRssiStats10m_Copy_));
                                                eStatus = ePHY_SET_SUCCESS;                                                         break;
#endif
      case ePhyAttr_RssiStats60m:               (void)memcpy(PhyRssiStats60m, pSetReq->val.RssiStats, sizeof(PhyRssiStats60m));
                                                eStatus = ePHY_SET_SUCCESS;                                                         break;
#endif
      case ePhyAttr_RxAbortCount:               eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_RxChannels:                 eStatus = RxChannels_Set((uint16_t*)pSetReq->val.RxChannels);                       break;
      case ePhyAttr_RxDetectionConfig:          eStatus = RxDetectionConfig_Set((PHY_DETECTION_e*)pSetReq->val.RxDetectionConfig);  break;
      case ePhyAttr_RxFramingConfig:            eStatus = RxFramingConfig_Set((PHY_FRAMING_e*)pSetReq->val.RxFramingConfig);        break;
      case ePhyAttr_RxMode:                     eStatus = RxMode_Set((PHY_MODE_e*)pSetReq->val.RxMode);                             break;
      case ePhyAttr_State:                      eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_SyncDetectCount:            eStatus = ePHY_SET_READONLY;                                                        break;
#if ( TEST_SYNC_ERROR == 1 )
      case ePhyAttr_SyncError:                  eStatus = SyncError_Set( pSetReq->val.SyncError);                                   break;
#endif
      case ePhyAttr_Temperature:                eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_ThermalControlEnable:       eStatus = ThermalControlEnable_Set(pSetReq->val.ThermalControlEnable);              break;
      case ePhyAttr_ThermalProtectionCount:     eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_ThermalProtectionEnable:    eStatus = ThermalProtectionEnable_Set(pSetReq->val.ThermalProtectionEnable);        break;
      case ePhyAttr_ThermalProtectionEngaged:   eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_TxAbortCount:               eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_TxChannels:                 eStatus = TxChannels_Set((uint16_t*)pSetReq->val.TxChannels);                       break;
      case ePhyAttr_TxOutputPowerMax:           eStatus = TxOutputPowerMax_Set(pSetReq->val.TxOutputPowerMax);                      break;
      case ePhyAttr_TxOutputPowerMin:           eStatus = TxOutputPowerMin_Set(pSetReq->val.TxOutputPowerMin);                      break;
      case ePhyAttr_VerboseMode:                eStatus = VerboseMode_Set(pSetReq->val.VerboseMode);                                break;
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
      case ePhyAttr_ReceivePowerMargin:         eStatus = Receive_Power_Margin_Set(pSetReq->val.ReceivePowerMargin);break;
#endif
#if ( ( DCU == 1 ) && (VSWR_MEASUREMENT == 1) )
      case ePhyAttr_fngVSWR:                    eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_fngForwardPower:            eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_fngReflectedPower:          eStatus = ePHY_SET_READONLY;                                                        break;
      case ePhyAttr_fngVswrNotificationSet:     eStatus = TxVSWR_NotificationSet(pSetReq->val.fngVswrNotificationSet);              break;
      case ePhyAttr_fngVswrShutdownSet:         eStatus = TxVSWR_ShutdownSet(pSetReq->val.fngVswrShutdownSet);                      break;
      case ePhyAttr_fngForwardPowerSet:         eStatus = TxVFowardPowerSet(pSetReq->val.fngForwardPowerSet);                       break;
      case ePhyAttr_fngFowardPowerLowSet:       eStatus = TxLowForwardPowerSet(pSetReq->val.fngFowardPowerLowSet);                  break;
      case ePhyAttr_fngfemMfg:                  eStatus = setFEMmfg(pSetReq->val.fngfemMfg);                                        break;
#endif
#if ( DCU == 1 )
      case ePhyAttr_txEnabled:                  eStatus = setTxEnabled(pSetReq->val.txEnabled);                                     break;
#endif

      default:
         eStatus   = ePHY_SET_UNSUPPORTED_ATTRIBUTE;
         break;
   } /*lint !e788 not all enums used within switch */
#if ( DCU == 1 )
   // Notify the MB that some configuration changed
   if ((VER_getDCUVersion() != eDCU2) && (eStatus == ePHY_SET_SUCCESS)) {
      switch(pSetReq->eAttribute)   //lint !e788
      {
         case ePhyAttr_RxChannels:
         case ePhyAttr_RxDetectionConfig:
         case ePhyAttr_RxFramingConfig:
         case ePhyAttr_RxMode:
         case ePhyAttr_TxChannels:
            MAINBD_TB_Config_Changed();
            break;

         default:
            break;
      }  //lint !e788
   }
#endif
   if(eStatus != ePHY_SET_SUCCESS)
   {
      INFO_printf("PHY_Attribute_Set:Attr=%s Status=%s", phy_attr_names[pSetReq->eAttribute], PHY_SetStatusMsg(eStatus ));
   }

   return eStatus;
}

/*!
 *********************************************************************************************************************
 *
 * \fn uint8_t NumChannels_Get()
 *
 * \brief This function will return the number of valid channels
 *
 * \details This is a helper funtion that is used by PHY_Attribute_Get()
 *
 * \return  uint8_t number of valid channels
 *
 *********************************************************************************************************************
 */
static uint8_t NumChannels_Get(void)
{
   uint32_t i;
   uint8_t NumChannels = 0;
   for(i=0;i<PHY_MAX_CHANNEL;i++) {
      if(ConfigAttr.Channels[i] != PHY_INVALID_CHANNEL) {
         NumChannels++;
      }
   }
   return NumChannels;
}

/*********************************************************************************************************************
 * Used to get a PHY Attribute
 *
 *  returns :
 *     ePHY_GET_SUCCESS
 *     ePHY_GET_UNSUPPORTED_ATTRIBUTE
 *     ePHY_GET_SERVICE_UNAVAILABLE
 *
 *********************************************************************************************************************/
PHY_GET_STATUS_e PHY_Attribute_Get( PHY_GetReq_t const *pGetReq, PHY_ATTRIBUTES_u *val)
{
   PHY_GET_STATUS_e eStatus = ePHY_GET_SUCCESS;

#if 0
   // This function should only be called inside the PHY task
   if ( ( _task_get_id() != _task_get_id_from_name( "PHY" ) )     &&
        ( _task_get_id() != _task_get_id_from_name( "PSLSNR" ) ) ) {
     ERR_printf("WARNING: PHY_Attribute_Get should only be called from the PHY task. Please use PHY_GetRequest instead.");
   }
#endif

   switch (pGetReq->eAttribute)
   {
      case ePhyAttr_AfcAdjustment:           (void)memcpy(val->AfcAdjustment,            ShadowAttr.AfcAdjustment          , sizeof(val->AfcAdjustment))          ; break;
#if ( EP == 1 )
      case ePhyAttr_AfcEnable:                            val->AfcEnable               = ConfigAttr.AfcEnable                                                     ; break;
      case ePhyAttr_AfcRssiThreshold:                     val->AfcRssiThreshold        = ConfigAttr.AfcRssiThreshold                                              ; break;
      case ePhyAttr_AfcTemperatureRange:     (void)memcpy(val->AfcTemperatureRange,      ConfigAttr.AfcTemperatureRange    , sizeof(val->AfcTemperatureRange))    ; break;
#endif
      case ePhyAttr_AvailableChannels:       (void)memcpy(val->AvailableChannels,        ConfigAttr.Channels               , sizeof(val->AvailableChannels))      ; break;
      case ePhyAttr_CcaAdaptiveThreshold:
      {  // Return CCA threshold only for matching TX channels
         uint32_t      i,j,k;
         uint8_t       numChannels;

         // Get the number of channels
         numChannels = NumChannels_Get();

         // Clear array
         for(i=0;i<numChannels;i++) val->CcaAdaptiveThreshold[i] = PHY_CCA_THRESHOLD_MAX;
         // Return CCA threshold list
         for(i=0,j=0;i<numChannels;i++) {
            // For each valid channel in the TX list
            if(ConfigAttr.TxChannels[i] != PHY_INVALID_CHANNEL) {
               // Find the same channel in the channel list and use the offset to retrieve the noise floor.
               for (k=0; k<PHY_MAX_CHANNEL; k++) {
                  if (ConfigAttr.TxChannels[i] == ConfigAttr.Channels[k]) {
                     val->CcaAdaptiveThreshold[j++] = CachedAttr.NoiseEstimate[k]+ConfigAttr.CcaOffset;
                     break;
                  }
               }
            }
         }
      }
      break;
      case ePhyAttr_CcaAdaptiveThresholdEnable:           val->CcaAdaptiveThresholdEnable = ConfigAttr.CcaAdaptiveThresholdEnable                                 ; break;
      case ePhyAttr_CcaOffset:                            val->CcaOffset               = ConfigAttr.CcaOffset                                                     ; break;
      case ePhyAttr_CcaThreshold:                         val->CcaThreshold            = ConfigAttr.CcaThreshold                                                  ; break;
      case ePhyAttr_DataRequestCount:                     val->DataRequestCount        = CachedAttr.DataRequestCount                                              ; break;
      case ePhyAttr_DataRequestRejectedCount:             val->DataRequestRejectedCount= CachedAttr.DataRequestRejectedCount                                      ; break;
      case ePhyAttr_Demodulator:             (void)memcpy(val->Demodulator             , ConfigAttr.Demodulator            , sizeof(val->Demodulator))            ; break;
      case ePhyAttr_FailedFrameDecodeCount:  (void)memcpy(val->FailedFrameDecodeCount  , CachedAttr.FailedFrameDecodeCount , sizeof(val->FailedFrameDecodeCount)) ; break;
      case ePhyAttr_FailedHcsCount:          (void)memcpy(val->FailedHcsCount          , CachedAttr.FailedHcsCount         , sizeof(val->FailedHcsCount))         ; break;
      case ePhyAttr_FailedHeaderDecodeCount: (void)memcpy(val->FailedHeaderDecodeCount , CachedAttr.FailedHeaderDecodeCount, sizeof(val->FailedHeaderDecodeCount)); break;
      case ePhyAttr_FailedTransmitCount:                  val->FailedTransmitCount     = CachedAttr.FailedTransmitCount                                           ; break;
      case ePhyAttr_FramesReceivedCount:     (void)memcpy(val->FramesReceivedCount     , CachedAttr.FramesReceivedCount    , sizeof(val->FramesReceivedCount))    ; break;
      case ePhyAttr_FramesTransmittedCount:               val->FramesTransmittedCount  = CachedAttr.FramesTransmittedCount                                        ; break;
      case ePhyAttr_FrontEndGain:                         val->FrontEndGain            = ConfigAttr.FrontEndGain                                                  ; break;
      case ePhyAttr_LastResetTime:                        val->LastResetTime           = CachedAttr.LastResetTime                                                 ; break;
      case ePhyAttr_MalformedHeaderCount:    (void)memcpy(val->MalformedHeaderCount    , CachedAttr.MalformedHeaderCount   , sizeof(val->MalformedHeaderCount))   ; break;
      case ePhyAttr_MinRxBytes:                           val->MinRxBytes              = ConfigAttr.MinRxBytes                                                    ; break;
      case ePhyAttr_MaxTxPayload:                         val->MaxTxPayload            = ConfigAttr.MaxTxPayload                                                  ; break;
      case ePhyAttr_NoiseEstimate:           (void)memcpy(val->NoiseEstimate           , CachedAttr.NoiseEstimate          , sizeof(val->NoiseEstimate))          ; break;
#if ( EP == 1 )
      case ePhyAttr_NoiseEstimateBoostOn:    (void)memcpy(val->NoiseEstimateBoostOn    , CachedAttr.NoiseEstimateBoostOn   , sizeof(val->NoiseEstimateBoostOn))   ; break;
#endif
      case ePhyAttr_NoiseEstimateCount:                   val->NoiseEstimateCount      = ConfigAttr.NoiseEstimateCount                                            ; break;
      case ePhyAttr_NoiseEstimateRate:                    val->NoiseEstimateRate       = ConfigAttr.NoiseEstimateRate                                             ; break;
      case ePhyAttr_NumChannels:                          val->NumChannels             = NumChannels_Get()                                                        ; break; // Count the number of valid channels
      case ePhyAttr_PowerSetting:                         val->PowerSetting            = ConfigAttr.PowerSetting                                                  ; break;
      case ePhyAttr_PreambleDetectCount:     (void)memcpy(val->PreambleDetectCount     , CachedAttr.PreambleDetectCount    , sizeof(val->PreambleDetectCount))    ; break;
      case ePhyAttr_RadioWatchdogCount:      (void)memcpy(val->RadioWatchdogCount      , CachedAttr.RadioWatchdogCount     , sizeof(val->RadioWatchdogCount))     ; break;
      case ePhyAttr_RcvrCount:                            val->RcvrCount               = PHY_RCVR_COUNT                                                           ; break;
      case ePhyAttr_RssiJumpCount:           (void)memcpy(val->RssiJumpCount           , CachedAttr.RssiJumpCount          , sizeof(val->RssiJumpCount))          ; break;
      case ePhyAttr_RssiJumpThreshold:                    val->RssiJumpThreshold       = ConfigAttr.RssiJumpThreshold                                             ; break;
#if ( DCU == 1 )
      case ePhyAttr_RssiStats10m:
      {
         uint32_t i;
         (void)memcpy(val->RssiStats, PhyRssiStats10m, sizeof(val->RssiStats));
         for (i=(uint32_t)RADIO_FIRST_RX; i<(uint32_t)PHY_RCVR_COUNT; i++) {
            if(val->RssiStats[i].Num > 1){ //If there are any records (past the one historical record)
               val->RssiStats[i].StdDev /= (float)(val->RssiStats[i].Num-1);  //Still "biased", just ignoring uncalculated historical value. Un-biased would be N-2
               val->RssiStats[i].StdDev  = sqrtf(val->RssiStats[i].StdDev);  //Complete final calculation
            }
         }
      }
      break;
#if ( MAC_LINK_PARAMETERS == 1 )
      case ePhyAttr_RssiStats10m_Copy:
      {
         (void)memcpy(val->RssiStats, PhyRssiStats10m_Copy_,  sizeof(val->RssiStats));
      }
      break;
#endif
      case ePhyAttr_RssiStats60m:
      {
         uint32_t i;
         (void)memcpy(val->RssiStats, PhyRssiStats60m, sizeof(val->RssiStats));
         for (i=(uint32_t)RADIO_FIRST_RX; i<(uint32_t)PHY_RCVR_COUNT; i++) {
            if(val->RssiStats[i].Num > 1){ //If there are any records (past the one historical record)
               val->RssiStats[i].StdDev /= (float)(val->RssiStats[i].Num-1);  //Still "biased", just ignoring uncalculated historical value. Un-biased would be N-2
               val->RssiStats[i].StdDev  = sqrtf(val->RssiStats[i].StdDev);  //Complete final calculation
            }
         }
      }
      break;
#endif
      case ePhyAttr_RxAbortCount:            (void)memcpy(val->RxAbortCount,             CachedAttr.RxAbortCount           , sizeof(val->RxAbortCount))           ; break;
      case ePhyAttr_RxChannels:              (void)memcpy(val->RxChannels,               ConfigAttr.RxChannels             , sizeof(val->RxChannels))             ; break;
      case ePhyAttr_RxDetectionConfig:       (void)memcpy(val->RxDetectionConfig,        ConfigAttr.RxDetectionConfig      , sizeof(val->RxDetectionConfig))      ; break;
      case ePhyAttr_RxFramingConfig:         (void)memcpy(val->RxFramingConfig,          ConfigAttr.RxFramingConfig        , sizeof(val->RxFramingConfig))        ; break;
      case ePhyAttr_RxMode:                  (void)memcpy(val->RxMode,                   ConfigAttr.RxMode                 , sizeof(val->RxMode))                 ; break;
      case ePhyAttr_State:                                val->State                   =  _phy.state                                                              ; break;
      case ePhyAttr_SyncDetectCount:         (void)memcpy(val->SyncDetectCount         , CachedAttr.SyncDetectCount        , sizeof(val->SyncDetectCount))        ; break;
#if ( TEST_SYNC_ERROR == 1 )
      case ePhyAttr_SyncError:                            val->SyncError               = ConfigAttr.SyncError                                                     ; break;
#endif
      case ePhyAttr_Temperature:                eStatus = Temperature_Get(&val->Temperature)                                                                      ; break;
      case ePhyAttr_ThermalControlEnable:                 val->ThermalControlEnable    = ConfigAttr.ThermalControlEnable                                          ; break;
      case ePhyAttr_ThermalProtectionCount:               val->ThermalProtectionCount  = CachedAttr.ThermalProtectionCount                                        ; break;
      case ePhyAttr_ThermalProtectionEnable:              val->ThermalProtectionEnable = ConfigAttr.ThermalProtectionEnable                                       ; break;
      case ePhyAttr_ThermalProtectionEngaged:             val->ThermalProtectionEngaged= CachedAttr.ThermalProtectionEngaged                                      ; break;
      case ePhyAttr_TxAbortCount:                         val->TxAbortCount            = CachedAttr.TxAbortCount                                                  ; break;
      case ePhyAttr_TxChannels:
      {  // Return valid TX channels
         uint32_t      i,j;

         // Return TX list
         for(i=0,j=0;i<PHY_MAX_CHANNEL;i++) {
            val->TxChannels[i] = PHY_INVALID_CHANNEL; // Clear entry
            if(ConfigAttr.TxChannels[i] != PHY_INVALID_CHANNEL) {
                val->TxChannels[j++] = ConfigAttr.TxChannels[i];
            }
         }
      }
      break;
      case ePhyAttr_TxOutputPowerMax:        val->TxOutputPowerMax         = ConfigAttr.TxOutputPowerMax                                     ; break;
      case ePhyAttr_TxOutputPowerMin:        val->TxOutputPowerMin         = ConfigAttr.TxOutputPowerMin                                     ; break;
      case ePhyAttr_VerboseMode:             val->VerboseMode              = ConfigAttr.VerboseMode                                          ; break;
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
      case ePhyAttr_ReceivePowerMargin:      val->ReceivePowerMargin       = Receive_Power_Margin_Get()                                               ; break;
      case ePhyAttr_ReceivePowerThreshold:        eStatus  = Receive_Power_Threshold_Calculate( val->ReceivePowerThreshold )                                      ; break;
#endif
#if ( ( DCU == 1 ) && (VSWR_MEASUREMENT == 1) )
      case ePhyAttr_fngVSWR               :  val->fngVSWR                  = (uint16_t)(getVSWRvalue( fngVSWR ))                             ; break;
      case ePhyAttr_fngVswrNotificationSet:  val->fngVswrNotificationSet   = ConfigAttr.fngVswrNotificationSet                               ; break;
      case ePhyAttr_fngVswrShutdownSet    :  val->fngVswrShutdownSet       = ConfigAttr.fngVswrShutdownSet                                   ; break;
      case ePhyAttr_fngForwardPower       :  val->fngForwardPower          = (uint16_t)(getVSWRvalue( fngForwardPower ))                     ; break;
      case ePhyAttr_fngReflectedPower     :  val->fngReflectedPower        = (uint16_t)(getVSWRvalue( fngReflectedPower ))                   ; break;
      case ePhyAttr_fngForwardPowerSet    :  val->fngForwardPowerSet       = ConfigAttr.fngForwardPowerSet                                   ; break;
      case ePhyAttr_fngFowardPowerLowSet  :  val->fngFowardPowerLowSet     = ConfigAttr.fngFowardPowerLowSet                                 ; break;
      case ePhyAttr_fngfemMfg             :  val->fngfemMfg                = ConfigAttr.fngfemMfg                                            ; break;
#endif
#if ( DCU == 1 )
      case ePhyAttr_txEnabled             :  val->txEnabled                = ConfigAttr.txEnabled                                            ; break;
#endif
      default:
         eStatus = ePHY_GET_UNSUPPORTED_ATTRIBUTE;
         break;
   }

   if(eStatus != ePHY_GET_SUCCESS)
   {
      INFO_printf("PHY_Attribute_Get:Attr=%s Status=%s", phy_attr_names[pGetReq->eAttribute], PHY_GetStatusMsg(eStatus ));
   }

   return eStatus;
}

/*!
 * Set PHY Demodulators
 */
bool PHY_Demodulator_Set(uint8_t const *demodulator)
{
   PHY_SetConf_t SetConf;
   bool          retVal = false;

   // Set the demodulators
   SetConf = PHY_SetRequest( ePhyAttr_Demodulator, demodulator);
   if (SetConf.eStatus == ePHY_SET_SUCCESS) {
      retVal = true;
   }
   return retVal;
}

/*!
 * Get PHY Demodulators
 */
bool PHY_Demodulator_Get(uint8_t *demodulator)
{
   PHY_GetConf_t GetConf;
   bool          retVal = false;

   // Get the Demodulators
   GetConf = PHY_GetRequest( ePhyAttr_Demodulator );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      (void)memcpy(demodulator, GetConf.val.Demodulator, sizeof(GetConf.val.Demodulator));
      retVal = true;
   }
   return retVal;
}

/*!
 * Set a PHY Rx Channel
 */
bool PHY_RxChannel_Set(uint8_t index, uint16_t chan)
{
   PHY_GetConf_t GetConf;
   PHY_SetConf_t SetConf;
   bool          retVal = false;

   // Get the Rx Channels
   GetConf = PHY_GetRequest( ePhyAttr_RxChannels );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      // Update with new channel
      GetConf.val.RxChannels[index] = chan;

      // Set the Rx Channels
      SetConf = PHY_SetRequest( ePhyAttr_RxChannels, &GetConf.val.RxChannels[0]);
      if (SetConf.eStatus == ePHY_SET_SUCCESS) {
         retVal = true;
      }
   }
   return retVal;
}

/*!
 * Get a PHY Rx Channel
 */
bool PHY_RxChannel_Get(uint8_t index, uint16_t *Channel)
{
   PHY_GetConf_t GetConf;
   bool          retVal = false;

   *Channel = PHY_INVALID_CHANNEL;
   // Get the Rx Channels
   GetConf = PHY_GetRequest( ePhyAttr_RxChannels );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      *Channel = GetConf.val.RxChannels[index];
      retVal = true;
   }
   return retVal;
}

/*!
 * Set a PHY Rx detection list
 */
bool PHY_RxDetectionConfig_Set(uint8_t const *detection)
{
   PHY_SetConf_t SetConf;
   bool          retVal = false;

   SetConf = PHY_SetRequest( ePhyAttr_RxDetectionConfig, (void*)detection);
   if (SetConf.eStatus == ePHY_SET_SUCCESS) {
       retVal = true;
   }
   return retVal;
}

/*!
 * Get a PHY Rx detection list
 */
bool PHY_RxDetectionConfig_Get(uint8_t *detection)
{
   PHY_GetConf_t GetConf;
   bool          retVal = false;

   // Get the detection list
   GetConf = PHY_GetRequest( ePhyAttr_RxDetectionConfig );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      (void)memcpy(detection, (uint8_t *)GetConf.val.RxDetectionConfig, sizeof(GetConf.val.RxDetectionConfig));
      retVal =  true;
   }
   return retVal;
}

/*!
 * Set a PHY Rx framing list
 */
bool PHY_RxFramingConfig_Set(uint8_t const *framing)
{
   PHY_SetConf_t SetConf;
   bool          retVal = false;

   // Set the framing list
   SetConf = PHY_SetRequest( ePhyAttr_RxFramingConfig, (void*)framing);
   if (SetConf.eStatus == ePHY_SET_SUCCESS) {
      retVal = true;
   }
   return retVal;
}

/*!
 * Get a PHY Rx framing list
 */
bool PHY_RxFramingConfig_Get(uint8_t *framing)
{
   PHY_GetConf_t GetConf;
   bool          retVal = false;

   // Get the framing list
   GetConf = PHY_GetRequest( ePhyAttr_RxFramingConfig );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      (void)memcpy(framing, (uint8_t *)GetConf.val.RxFramingConfig, sizeof(GetConf.val.RxFramingConfig));
      retVal = true;
   }
   return retVal;
}

/*!
 * Set a PHY Rx mode list
 */
bool PHY_RxMode_Set(uint8_t const *mode)
{
   PHY_SetConf_t SetConf;
   bool          retVal = false;

   // Set the mode list
   SetConf = PHY_SetRequest( ePhyAttr_RxMode, (void*)mode);
   if (SetConf.eStatus == ePHY_SET_SUCCESS) {
      retVal = true;
   }
   return retVal;
}

/*!
 * Get a PHY Rx mode list
 */
bool PHY_RxMode_Get(uint8_t *mode)
{
   PHY_GetConf_t GetConf;
   bool          retVal = false;

   // Get the mode list
   GetConf = PHY_GetRequest( ePhyAttr_RxMode );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      (void)memcpy(mode, (uint8_t *)GetConf.val.RxMode, sizeof(GetConf.val.RxMode));
      retVal = true;
   }
   return retVal;
}

#if DCU == 1
/*!
 * Set the front end gain
 * The value must be in the range of -128 to 127
 */
bool PHY_FrontEndGain_Set(int8_t const *gain)
{
   PHY_SetConf_t SetConf;
   bool          retVal = false;

   // Set the mode list
   SetConf = PHY_SetRequest( ePhyAttr_FrontEndGain, (void*)gain);
   if (SetConf.eStatus == ePHY_SET_SUCCESS) {
      retVal = true;
   }
   return retVal;
}

/*!
 * Read the front end gain
 */
bool PHY_FrontEndGain_Get(int8_t *gain)
{
   PHY_GetConf_t GetConf;
   bool          retVal = false;

   // Get the mode list
   GetConf = PHY_GetRequest( ePhyAttr_FrontEndGain );
   if (GetConf.eStatus == ePHY_GET_SUCCESS) {
      (void)memcpy(gain, &GetConf.val.FrontEndGain, sizeof(GetConf.val.FrontEndGain));
      retVal = true;
   }
   return retVal;
}
#endif

/*!
 * Set a PHY Tx Channel
 */
bool PHY_TxChannel_Set(uint8_t index, uint16_t chan)
{
   uint16_t TxChannels[PHY_MAX_CHANNEL];
   PHY_SetConf_t SetConf;
   bool          retVal = false;

   // Get the Tx Channels
   (void) memcpy(&TxChannels[0], ConfigAttr.TxChannels, sizeof(TxChannels));

   // Update with new channel
   TxChannels[index] = chan;

   // Set the Tx Channels
   SetConf = PHY_SetRequest( ePhyAttr_TxChannels, &TxChannels[0]);
   if (SetConf.eStatus == ePHY_SET_SUCCESS) {
      retVal = true;
   }
   return retVal;
}

#if 0
/*!
  Set cached attribute values. Bypasses check for READ ONLY. Should only be called from a "friend" function (e.g., radio.c).
*/
PHY_SET_STATUS_e PHY_friend_SetCachedValue( PHY_SetReq_t const *pSetReq )
{
   PHY_SET_STATUS_e eStatus = ePHY_SET_SUCCESS;
   switch( pSetReq->eAttribute ) /*lint !e788 not all enums used within switch */
   {
      case     ePhyAttr_fngVSWR:             CachedAttr.fngVSWR            = pSetReq->val.fngVSWR;             break;
      case     ePhyAttr_fngForwardPower:     CachedAttr.fngForwardPower    = pSetReq->val.fngForwardPower;     break;
      case     ePhyAttr_fngReflectedPower:   CachedAttr.fngReflectedPower  = pSetReq->val.fngReflectedPower;   break;
      default:                               eStatus = ePHY_SET_SERVICE_UNAVAILABLE;                           break;
   }  /*lint !e788 not all enums used within switch */
   return eStatus;
}
#endif


/*!
 * Get a PHY Tx Channel
 */
bool PHY_TxChannel_Get(uint8_t index, uint16_t *Channel)
{
   *Channel = ConfigAttr.TxChannels[index];
   return true;
}

/*!
 * Set the PHY channels
 */
bool PHY_Channel_Set(uint8_t index, uint16_t chan)
{
   ConfigAttr.Channels[index] = chan;
#if ( EP == 1 )
#if 0 // TODO: RA6E1 Bob: Defer this until Last Gasp module is ready
   if(PWRLG_LastGasp() == false)
#endif
#endif
   {  // Not in low power mode, so save this
      writeConfig();

      if (chan != PHY_INVALID_CHANNEL) {
         // Channel was added, update noise floor
         ChannelAdded = true;
      }
   }
   return true;
}

/*!
 * Get the PHY channel
 */
bool PHY_Channel_Get(uint8_t index, uint16_t *Channel)
{
   *Channel = ConfigAttr.Channels[index];
   return true;
}

/*!
 * Update local copy of ConfigAttr from NV
 */
returnStatus_t PHY_configRead( void )
{
   return ( FIO_fread( &PHYconfigFileHndl_, (uint8_t*)&ConfigAttr, 0, sizeof(ConfigAttr) ) );
}

/*!
 * Used to a send a Data.Indication message to the MAC
 * The MAC must have registered a handler
 */
static void Indication_Send(buffer_t *pBuf)
{
   if (pBuf != NULL)
   {
      // Set the type to PHY Indication
      pBuf->eSysFmt = eSYSFMT_PHY_INDICATION;
      if ( _phy.pIndicationHandler != NULL )
      { // Now send to MAC
         (*_phy.pIndicationHandler)(pBuf);
      }else
      {  // no message handler
         INFO_printf("No indication handler registered");
      }
   }
}

/*!
 * This functions is called to send a PHY Confirmation to a PHY Request
 *
 * If a confirmation handler was specified in the request, it will call that handler.
 * else it will print the results of the request.
 */
static void SendConfirm(PHY_Confirm_t const *pConf, buffer_t const *pReqBuf)
{
   PHY_Request_t *pReq = (PHY_Request_t*) &pReqBuf->data[0];  /*lint !e740 !e826 !e613*/

   if (pReq->pConfirmHandler != NULL )
   {  // Call the handler specified
      (pReq->pConfirmHandler)(pConf, pReqBuf);
   }else
   {
      char* msg;
      switch(pConf->MsgType)
      {
         case ePHY_CCA_CONF    :  msg = PHY_CcaStatusMsg(   pConf->CcaConf.eStatus);   break;
         case ePHY_GET_CONF    :  msg = PHY_GetStatusMsg(   pConf->GetConf.eStatus);   break;
         case ePHY_SET_CONF    :  msg = PHY_SetStatusMsg(   pConf->SetConf.eStatus);   break;
         case ePHY_START_CONF  :  msg = PHY_StartStatusMsg( pConf->StartConf.eStatus); break;
         case ePHY_STOP_CONF   :  msg = PHY_StopStatusMsg(  pConf->StopConf.eStatus);  break;
         case ePHY_RESET_CONF  :  msg = PHY_ResetStatusMsg( pConf->ResetConf.eStatus); break;
         case ePHY_DATA_CONF   :  msg = PHY_DataStatusMsg(  pConf->DataConf.eStatus);  break;
         default               :  msg = "Unknown";
      }  /*lint !e788 not all enums are handled by the switch  */
      INFO_printf("CONFIRM:%s", msg);
   }
}


/*!
 * Used to set a radio event
 */
#if ( RTOS_SELECTION == FREE_RTOS )
static void RadioEvent_Set(RADIO_EVENT_t event_type, uint8_t chan, bool fromISR)
{
   if ( fromISR )
   {
      switch (event_type)
      {
         case eRADIO_TX_DONE:
            OS_EVNT_Set_from_ISR( &events, 0x01u << (PHY_RADIO_TX_DONE_BASE            ));
            break;
         case eRADIO_RX_DATA:
            OS_EVNT_Set_from_ISR( &events, 0x01u << (PHY_RADIO_RX_DATA_BASE      + chan));
            break;
         case eRADIO_CTS_LINE_LOW:
            OS_EVNT_Set_from_ISR( &events, 0x01u << (PHY_RADIO_CTS_LINE_LOW_BASE + chan));
            break;
         case eRADIO_INT:
            OS_EVNT_Set_from_ISR( &events, 0x01u << (PHY_RADIO_INT_PENDING_BASE  + chan));
            break;
         default:
            break;
      }
   }
   else
#elif ( RTOS_SELECTION == MQX_RTOS )
static void RadioEvent_Set(RADIO_EVENT_t event_type, uint8_t chan)
{
#endif
   {
      switch (event_type)
      {
         case eRADIO_TX_DONE:
            OS_EVNT_Set( &events, 0x01u << (PHY_RADIO_TX_DONE_BASE            ));
            break;
         case eRADIO_RX_DATA:
            OS_EVNT_Set( &events, 0x01u << (PHY_RADIO_RX_DATA_BASE      + chan));
            break;
         case eRADIO_CTS_LINE_LOW:
            OS_EVNT_Set( &events, 0x01u << (PHY_RADIO_CTS_LINE_LOW_BASE + chan));
            break;
         case eRADIO_INT:
            OS_EVNT_Set( &events, 0x01u << (PHY_RADIO_INT_PENDING_BASE  + chan));
            break;
         default:
            break;
      }
   }
}


/*!
 * Handler for radio tx done event
 */
static void RadioEvent_TxDone(uint8_t radio_num)
{
   (void) radio_num;

   INFO_printf("RadioEvent_TxDone:%d", radio_num);

   PHY_CounterInc(ePHY_FramesTransmittedCount, 0);

   if( pDataRequestBuf != NULL)
   {  // The radio has a message that was transmitting

      // Create the Data.Confirmation
      PhyDataConfMsg.MsgType          = ePHY_DATA_CONF;
      PhyDataConfMsg.DataConf.eStatus = ePHY_DATA_SUCCESS;

      // Send the Data.Confirmation
      SendConfirm(&PhyDataConfMsg, pDataRequestBuf);

      // Free the request buffer
      BM_free(pDataRequestBuf);
      pDataRequestBuf = NULL;
   }
#if ( ( DCU == 1 ) && (VSWR_MEASUREMENT == 1) )
   (void)BSP_PaDAC_SetValue( 0 );   /* Set FEM output power value to 0  */
#endif
}

/*!
 * Handler for radio receive data event
 */
static void RadioEvent_RxData(uint8_t radioNum)
{
   // Get the packet data and sent to MAC
   buffer_t   *pBuf;
   RX_FRAME_t *rx_buffer;  // Buffer for reading data from radio
   uint8_t    *pRxData;
   uint8_t     length;

   INFO_printf("RadioEvent_RxData:%u", radioNum);

   rx_buffer = Radio.ReadData(radioNum);

   pRxData = rx_buffer->Payload;

   // Get length
   length = HeaderLength_Get(rx_buffer->framing, pRxData);

   // Create the Data.Indication
   pBuf = DataIndication_Create(rx_buffer, length, ConfigAttr.RxChannels[radioNum], radioNum );

   // Send the Data.Indication to the MAC
   Indication_Send(pBuf);

#if ( EP == 1 )
   if ( ConfigAttr.Demodulator[radioNum] != 0 ) {
      OS_EVNT_Set(&SD_Events,SOFT_DEMOD_PROCESSED_PAYLOAD_BYTES); // This event will unblock the blocked sync&payload Demod task

      // This is clunky and should not be needed but we need to restart the FIFO because it took so long to run the RS over the payload that we missed at least one FIFO interrupt.
      // When that happens, the radio stops to generates interrupts.
      //INFO_printf("FIFO Cleared (preventative measure)");
      //uint8_t temp[129]; //64 in each RX/TX buffer, plus the unused TX shift barrel
      //si446x_read_rx_fifo(radioNum, 128, temp);
      //(void)si446x_get_int_status_fast_clear((uint8_t)radioNum);
   }
#endif
}

/*!
 * Handler for radio CTS Line Low
 */
static void RadioEvent_CTSLineLow(uint8_t radioNum)
{
   DBG_logPrintf('E', "Radio: CTS line is low for too long for radio %u.", radioNum);
   RadioEvent_RadioWatchdog(radioNum);
   (void)PHY_StopRequest(NULL);
   (void)PHY_StartRequest((PHY_START_e) _phy.state, NULL);
}

/*!
 * Handler for radio rssi jump event
 */
void RadioEvent_RssiJump(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_RssiJumpCount, radioNum);
}

/*!
 * Handler for radio sync detect event
 */
/*lint -e{715} */
void RadioEvent_SyncDetect(uint8_t radioNum, int16_t rssi_dbm)
{
   PHY_CounterInc(ePHY_SyncDetectCount, radioNum);

#if ( EP == 1 )
#if NOISE_TEST_ENABLED == 0
   // Save to the next location
   CachedAttr.SyncHistory.Rssi[CachedAttr.SyncHistory.Index%10] = rssi_dbm;
   CachedAttr.SyncHistory.Index++;
   writeCache();
#endif
#endif
}

/*!
 * Handler for radio preamble detect event
 */
void RadioEvent_PreambleDetect(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_PreambleDetectCount, radioNum);
}

/*!
 * Handler for radio watchdog event
 */
void RadioEvent_RadioWatchdog(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_RadioWatchdogCount, radioNum);
}

/*!
 * Handler for radio Rx Abort event
 */
void RadioEvent_RxAbort(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_RxAbortCount, radioNum);
}

/*!
 * Handler for radio Tx Abort event
 */
void RadioEvent_TxAbort(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_TxAbortCount, radioNum);
}

/*!
 * Increment Failed Header Decode Count
 */
void PHY_FailedHeaderDecodeCount_Inc(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_FailedHeaderDecodeCount, radioNum);
}

/*!
 * Increment Failed HCS Count
 */
void PHY_FailedHcsCount_Inc(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_FailedHcsCount, radioNum);
}

/*!
 * Increment Malformed Header Count
 */
void PHY_MalformedHeaderCount_Inc(uint8_t radioNum)
{
   PHY_CounterInc(ePHY_MalformedHeaderCount, radioNum);
}

/*!
 * Insert a phy payload into the MAC layer
 */
void PHY_InsertTestMsg(uint8_t const *payload, uint16_t length, PHY_FRAMING_e framing, PHY_MODE_e mode)
{
   buffer_t     *pBuf;

   // Create the Data.Indication
   RX_FRAME_t rx_buffer;

   (void)memset(&rx_buffer, 0, sizeof(rx_buffer));
   (void) TIME_UTIL_GetTimeInSecondsFormat(&rx_buffer.syncTime);
   rx_buffer.rssi_dbm = 0;

   rx_buffer.framing = framing;
   rx_buffer.mode = mode;

   (void)memcpy(&rx_buffer.Payload[PHY_HEADER_SIZE], payload, PHY_MAX_FRAME-PHY_HEADER_SIZE);

   pBuf = DataIndication_Create(&rx_buffer, (uint8_t)length, 0, (uint8_t)RADIO_0);

   // Send the Data.Indication to the MAC
   Indication_Send(pBuf);
}

/*!
 * Checks if the channel is in the valid channel list for this device
 */
bool PHY_IsChannelValid(uint16_t chan)
{
   uint32_t i;
   bool isValid = false;
   for( i=0;i<PHY_MAX_CHANNEL;i++ )
   {
      if((ConfigAttr.Channels[i] != PHY_INVALID_CHANNEL) && (ConfigAttr.Channels[i] == chan))
      {
         isValid = true;
         break;
      }
   }
   return isValid;
}

/*!
 * Checks if the RX channel is in the valid channel list for this device
 */
bool PHY_IsRXChannelValid(uint16_t chan)
{
   uint32_t i;
   bool isValid = false;

   for (i=0; i<PHY_RCVR_COUNT; i++) {
      // Is the channel in the list of RX channels
      if (ConfigAttr.RxChannels[i] == chan) {
         isValid = PHY_IsChannelValid(chan);
         if (isValid == true) {
            break;
         }
      }
   }
   return isValid;
}

/*!
 * Checks if the TX channel is in the valid channel list for this device
 */
bool PHY_IsTXChannelValid(uint16_t chan)
{
   uint32_t i;
   bool isValid = false;

   for (i=0; i<PHY_MAX_CHANNEL; i++) {
      // Is the channel in the list of TX channels
      if (ConfigAttr.TxChannels[i] == chan) {
         isValid = PHY_IsChannelValid(chan);
         break;
      }
   }
   return isValid;
}

/*!
 * This function is used to print the phy configuration to the terminal
 */
void PHY_ConfigPrint(void)
{
#if ( EP == 1 )
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_AfcEnable           ], ConfigAttr.AfcEnable               );
   INFO_printf( "PHY_CONFIG:%s %i",      phy_attr_names[ePhyAttr_AfcRssiThreshold    ], ConfigAttr.AfcRssiThreshold        );
   INFO_printf( "PHY_CONFIG:%s %i %i",   phy_attr_names[ePhyAttr_AfcTemperatureRange ], ConfigAttr.AfcTemperatureRange[0],
                                                                                        ConfigAttr.AfcTemperatureRange[1]  );
#endif

   INFO_printf( "PHY_CONFIG:%s %i",      phy_attr_names[ePhyAttr_CcaAdaptiveThresholdEnable], ConfigAttr.CcaAdaptiveThresholdEnable );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_CcaOffset           ], ConfigAttr.CcaOffset               );
   INFO_printf( "PHY_CONFIG:%s %i",      phy_attr_names[ePhyAttr_CcaThreshold        ], ConfigAttr.CcaThreshold            );
   INFO_printf( "PHY_CONFIG:%s %i",      phy_attr_names[ePhyAttr_FrontEndGain        ], ConfigAttr.FrontEndGain            );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_MinRxBytes          ], ConfigAttr.MinRxBytes              );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_NoiseEstimateCount  ], ConfigAttr.NoiseEstimateCount      );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_NoiseEstimateRate   ], ConfigAttr.NoiseEstimateRate       );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_PowerSetting        ], ConfigAttr.PowerSetting            );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_RssiJumpThreshold   ], ConfigAttr.RssiJumpThreshold       );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_ThermalControlEnable], ConfigAttr.ThermalControlEnable    );
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_ThermalProtectionEnable], ConfigAttr.ThermalProtectionEnable );
   INFO_printf( "PHY_CONFIG:%s %d.%1u",  phy_attr_names[ePhyAttr_TxOutputPowerMax    ], (int32_t)ConfigAttr.TxOutputPowerMax, (int32_t)(ConfigAttr.TxOutputPowerMax*10)%10);
   INFO_printf( "PHY_CONFIG:%s %d.%1u",  phy_attr_names[ePhyAttr_TxOutputPowerMin    ], (int32_t)ConfigAttr.TxOutputPowerMin, (int32_t)(ConfigAttr.TxOutputPowerMin*10)%10);
   INFO_printf( "PHY_CONFIG:%s %u",      phy_attr_names[ePhyAttr_VerboseMode         ], ConfigAttr.VerboseMode);

// INFO_printf("PHY_CONFIG:RxChannels[PHY_RCVR_COUNT]
// INFO_printf("PHY_CONFIG:TxChannels[PHY_MAX_CHANNEL]
// INFO_printf("PHY_CONFIG:Channels[  PHY_MAX_CHANNEL]

// PHY_DETECTION_e    INFO_printf("MAC_CONFIG:RxDetectionConfig[PHY_RCVR_COUNT]; /*!< The detection configuration the PHY will use on the corresponding receiver  */
// PHY_FRAMING_e      INFO_printf("MAC_CONFIG:RxFramingConfig[PHY_RCVR_COUNT];   /*!< The framing configuration the PHY will use on the corresponding receiver  */
// PHY_MODE_e         INFO_printf("MAC_CONFIG:RxMode[PHY_RCVR_COUNT];            /*!< The PHY mode used on the corresponding receiver  */
#if ( ( DCU == 1 ) && (VSWR_MEASUREMENT == 1) )
   INFO_printf( "PHY_CONFIG:%s %u.%1u",      phy_attr_names[ePhyAttr_fngVswrNotificationSet  ], ConfigAttr.fngVswrNotificationSet/10, ConfigAttr.fngVswrNotificationSet%10);
   INFO_printf( "PHY_CONFIG:%s %u.%1u",      phy_attr_names[ePhyAttr_fngVswrShutdownSet      ], ConfigAttr.fngVswrShutdownSet/10, ConfigAttr.fngVswrShutdownSet%10);
   INFO_printf( "PHY_CONFIG:%s %u.%1u",      phy_attr_names[ePhyAttr_fngFowardPowerLowSet    ], ConfigAttr.fngFowardPowerLowSet/10, ConfigAttr.fngFowardPowerLowSet%10);
   INFO_printf( "PHY_CONFIG:%s %u.%1u",      phy_attr_names[ePhyAttr_fngForwardPowerSet      ], ConfigAttr.fngForwardPowerSet/10, ConfigAttr.fngForwardPowerSet%10);
#endif
#if ( DCU == 1 )
   INFO_printf( "PHY_CONFIG:%s %hhu",        phy_attr_names[ePhyAttr_txEnabled               ], ConfigAttr.txEnabled );
#endif
}

/*!
 * This function is used to print the phy stats to the terminal
 */
void PHY_Stats(void)
{
   sysTime_t            sysTime;
   sysTime_dateFormat_t sysDateFormat;

   TIME_UTIL_ConvertSecondsToSysFormat(CachedAttr.LastResetTime.seconds, 0, &sysTime);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat);
   INFO_printf("PHY_STATS:%s=%02u/%02u/%04u %02u:%02u:%02u",phy_attr_names[ePhyAttr_LastResetTime],
                sysDateFormat.month, sysDateFormat.day, sysDateFormat.year, sysDateFormat.hour, sysDateFormat.min, sysDateFormat.sec); /*lint !e123 */
#if PHY_RCVR_COUNT == 1
   INFO_printf( "PHY_STATS:%s %i", phy_attr_names[ePhyAttr_AfcAdjustment           ],ShadowAttr.AfcAdjustment[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_DataRequestCount        ],CachedAttr.DataRequestCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_DataRequestRejectedCount],CachedAttr.DataRequestRejectedCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FailedFrameDecodeCount  ],CachedAttr.FailedFrameDecodeCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FailedHcsCount          ],CachedAttr.FailedHcsCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FailedHeaderDecodeCount ],CachedAttr.FailedHeaderDecodeCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FramesReceivedCount     ],CachedAttr.FramesReceivedCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FailedTransmitCount     ],CachedAttr.FailedTransmitCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FramesTransmittedCount  ],CachedAttr.FramesTransmittedCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_MalformedHeaderCount    ],CachedAttr.MalformedHeaderCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_PreambleDetectCount     ],CachedAttr.PreambleDetectCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_RadioWatchdogCount      ],CachedAttr.RadioWatchdogCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_RssiJumpCount           ],CachedAttr.RssiJumpCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_RxAbortCount            ],CachedAttr.RxAbortCount[0]);
   INFO_printf( "PHY_STATS:%s %u (%s)", phy_attr_names[ePhyAttr_State              ],_phy.state, PHY_StateMsg(_phy.state));
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_SyncDetectCount         ],CachedAttr.SyncDetectCount[0]);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_ThermalProtectionCount  ],CachedAttr.ThermalProtectionCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_ThermalProtectionEngaged],CachedAttr.ThermalProtectionEngaged);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_TxAbortCount            ],CachedAttr.TxAbortCount);
   PHY_RssiHistory_Print();
#else
   // DCU
   uint32_t i;

   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_DataRequestCount        ],CachedAttr.DataRequestCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_DataRequestRejectedCount],CachedAttr.DataRequestRejectedCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FailedTransmitCount     ],CachedAttr.FailedTransmitCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_FramesTransmittedCount  ],CachedAttr.FramesTransmittedCount);
   INFO_printf( "PHY_STATS:%s %u (%s)", phy_attr_names[ePhyAttr_State              ],_phy.state, PHY_StateMsg(_phy.state));
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_ThermalProtectionCount  ],CachedAttr.ThermalProtectionCount);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_ThermalProtectionEngaged],CachedAttr.ThermalProtectionEngaged);
   INFO_printf( "PHY_STATS:%s %u", phy_attr_names[ePhyAttr_TxAbortCount            ],CachedAttr.TxAbortCount);

   for(i=0;i<PHY_RCVR_COUNT;i++)
   {
      INFO_printf(
         " PHY_STATS radio %u:"
         " AfcAdjustment %u,"
         " FailedFrameDecode %u,"
         " FailedHcs %u,"
         " FailedHeaderDecode %u,"
         " FramesReceived %u,"
         " MalformedHeader %u,"
         " PreambleDetect %u,"
         " RadioWatchdogCount %u,"
         " RssiJump %u,"
         " RxAbortCount %u,"
         " SyncDetect %u",
         i,
         ConfigAttr.AfcAdjustment[i],
         CachedAttr.FailedFrameDecodeCount[i],
         CachedAttr.FailedHcsCount[i],
         CachedAttr.FailedHeaderDecodeCount[i],
         CachedAttr.FramesReceivedCount[i],
         CachedAttr.MalformedHeaderCount[i],
         CachedAttr.PreambleDetectCount[i],
         CachedAttr.RadioWatchdogCount[i],
         CachedAttr.RssiJumpCount[i],
         CachedAttr.RxAbortCount[i],
         CachedAttr.SyncDetectCount[i]);
   }
#endif
}


/*
 * Returns the Message for CCA Status
 */
char* PHY_CcaStatusMsg(PHY_CCA_STATUS_e CCAstatus)
{
   char* msg="";
   switch(CCAstatus)
   {
   case ePHY_CCA_SUCCESS:              msg = "PHY_CCA_SUCCESS";              break;
   case ePHY_CCA_SERVICE_UNAVAILABLE:  msg = "PHY_CCA_SERVICE_UNAVAILABE";   break;
   case ePHY_CCA_BUSY:                 msg = "PHY_CCA_BUSY";                 break;
   case ePHY_CCA_INVALID_CONFIG:       msg = "PHY_CCA_INVALID_CONFIG";       break;
   case ePHY_CCA_INVALID_PARAMETER:    msg = "PHY_CCA_INVALID_PARAMETER";    break;
   }
   return msg;
}

/*
 * Returns the Message for GET Status
 */
char* PHY_GetStatusMsg(PHY_GET_STATUS_e GETstatus)
{
   char* msg="";
   switch(GETstatus)
   {
      case ePHY_GET_SUCCESS:               msg = "PHY_GET_SUCCESS";               break;
      case ePHY_GET_UNSUPPORTED_ATTRIBUTE: msg = "PHY_GET_UNSUPPORTED_ATTRIBUTE"; break;
      case ePHY_GET_SERVICE_UNAVAILABLE:   msg = "PHY_GET_SERVICE_UNAVAILABLE";   break;
   }
   return msg;
}

/*
 * Returns the Message for SET Status
 */
char* PHY_SetStatusMsg(PHY_SET_STATUS_e SETstatus)
{
   char* msg="";
   switch(SETstatus)
   {
      case ePHY_SET_SUCCESS:               msg = "PHY_SET_SUCCESS";              break;
      case ePHY_SET_READONLY:              msg = "PHY_SET_READONLY";             break;
      case ePHY_SET_UNSUPPORTED_ATTRIBUTE: msg = "PHY_SET_UNSUPPORTED";          break;
      case ePHY_SET_INVALID_PARAMETER:     msg = "PHY_SET_INVALID_PARAMETER";    break;
      case ePHY_SET_SERVICE_UNAVAILABLE:   msg = "PHY_SET_SERVICE_UNAVAILABLE";  break;
   }
   return msg;
}

/*
 * Returns the Message for START Status
 */
char* PHY_StartStatusMsg(PHY_START_STATUS_e STARTstatus)
{
   char* msg="";
   switch(STARTstatus)
   {
      case ePHY_START_SUCCESS:            msg = "PHY_START_SUCCESS";            break;
      case ePHY_START_FAILURE:            msg = "PHY_START_FAILURE";            break;
      case ePHY_START_INVALID_PARAMETER:  msg = "PHY_START_INVALID_PARAMETER";  break;
   }
   return msg;
}

/*
 * Returns the Message for STOP Status
 */
char* PHY_StopStatusMsg(PHY_STOP_STATUS_e STOPstatus)
{
   char* msg="";
   switch(STOPstatus)
   {
      case ePHY_STOP_SUCCESS:             msg = "PHY_STOP_SUCCESS";             break;
      case ePHY_STOP_FAILURE:             msg = "PHY_STOP_FAILURE";             break;
      case ePHY_STOP_SERVICE_UNAVAILABLE: msg = "PHY_STOP_SERVICE_UNAVAILABLE"; break;
   }
   return msg;
}

/*
 * Returns the Message for RESET Status
 */
char* PHY_ResetStatusMsg(PHY_RESET_STATUS_e RESETstatus)
{
   char* msg="";
   switch(RESETstatus)
   {
      case ePHY_RESET_SUCCESS:             msg = "PHY_RESET_SUCCESS";             break;
      case ePHY_RESET_FAILURE:             msg = "PHY_RESET_FAILURE";             break;
      case ePHY_RESET_SERVICE_UNAVAILABLE: msg = "PHY_RESET_SERVICE_UNAVAILABLE"; break;
      case ePHY_RESET_INVALID_PARAMETER:   msg = "PHY_RESET_INVALID_PARAMETER";   break;
   }
   return msg;
}

/*
 * Returns the Message for DATA Status
 */
char* PHY_DataStatusMsg(PHY_DATA_STATUS_e DATAstatus)
{
   char* msg="";
   switch(DATAstatus)
   {
      case ePHY_DATA_SUCCESS:             msg = "PHY_DATA_SUCCESS";              break;
      case ePHY_DATA_SERVICE_UNAVAILABLE: msg = "PHY_DATA_SERVICE_UNAVAILABLEY"; break;
      case ePHY_DATA_TRANSMISSION_FAILED: msg = "PHY_DATA_TRANSMISSION_FAILED";  break;
      case ePHY_DATA_INVALID_PARAMETER:   msg = "PHY_DATA_INVALID_PARAMETER";    break;
      case ePHY_DATA_BUSY:                msg = "PHY_DATA_BUSY";                 break;
      case ePHY_DATA_THERMAL_OVERRIDE:    msg = "PHY_DATA_THERMAL_OVERRIDE";     break;
   }
   return msg;
}

/*
 * Returns the Message for START state
 */
char* PHY_StartStateMsg(PHY_START_e STARTstate)
{
   char* msg="";
   switch(STARTstate)
   {
      case ePHY_START_STANDBY:  msg = "ePHY_START_STANDBY";  break;
      case ePHY_START_READY:    msg = "ePHY_START_READY";    break;
      case ePHY_START_READY_TX: msg = "ePHY_START_READY_TX"; break;
      case ePHY_START_READY_RX: msg = "ePHY_START_READY_RX"; break;
   }
   return msg;
}

/*!
 * Returns the Message for PHY_STATE_e
 */
static char* PHY_StateMsg(PHY_STATE_e state)
{
   char* msg="";
   switch(state)
   {
   case ePHY_STATE_STANDBY  : msg = "ePHY_STATE_STANDBY";  break;
   case ePHY_STATE_READY    : msg = "ePHY_STATE_READY";    break;
   case ePHY_STATE_READY_TX : msg = "ePHY_STATE_READY_TX"; break;
   case ePHY_STATE_READY_RX : msg = "ePHY_STATE_READY_RX"; break;
   case ePHY_STATE_SHUTDOWN : msg = "ePHY_STATE_SHUTDOWN"; break;
   }
   return msg;
}

/*
 * Get a PHY Request Buffer
 */
static buffer_t *RequestBuffer_Get(uint16_t size)
{
   buffer_t *pBuf = BM_allocStack(size);   // Allocate the buffer
   if(pBuf)
   {
      pBuf->eSysFmt = eSYSFMT_PHY_REQUEST;
   }
   return pBuf;
}


/*!
 *  Send a Start Request to the PHY
 */
bool PHY_StartRequest(PHY_START_e state, PHY_ConfirmHandler cb_confirm)
{
   bool retVal = false;

   // Allocate the buffer
   buffer_t *pBuf = RequestBuffer_Get(sizeof(PHY_Request_t));
   if (pBuf != NULL)
   {
      PHY_Request_t *pReq = (PHY_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the PHY_Request_t  information */
      pReq->MsgType         = ePHY_START_REQ;
      pReq->pConfirmHandler = cb_confirm;
      pReq->StartReq.state  = state;
      PHY_Request(pBuf);
      retVal = true;
   }
   return retVal;
}

/*!
 *  Send a Stop Request to the PHY
 */
bool PHY_StopRequest(PHY_ConfirmHandler cb_confirm)
{
   bool retVal = false;

   // Allocate the buffer
   buffer_t *pBuf = RequestBuffer_Get(sizeof(PHY_Request_t));
   if (pBuf != NULL)
   {
      PHY_Request_t *pReq = (PHY_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the PHY_Request_t  information */
      pReq->MsgType         = ePHY_STOP_REQ;
      pReq->pConfirmHandler = cb_confirm;
      PHY_Request(pBuf);
      retVal = true;
   }
   return retVal;
}

/*!
 *  Send a CCA Request to the PHY
 */
bool PHY_CcaRequest(uint16_t chan, PHY_ConfirmHandler cb_confirm)
{
   bool retVal = false;

   /* Build a Data.Request message from this data and send to the PHY layer */
   buffer_t *pBuf = RequestBuffer_Get(sizeof(PHY_Request_t));
   if (pBuf != NULL)
   {
      PHY_Request_t *pReq = (PHY_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the PHY_Request_t  information */
      pReq->MsgType         = ePHY_CCA_REQ;
      pReq->pConfirmHandler = cb_confirm;
      pReq->CcaReq.channel  = chan;
      PHY_Request(pBuf);
      retVal = true;
   }
   return retVal;
}

/*!
 *  Send a RESET Request to the PHY
 */
bool PHY_ResetRequest(PHY_RESET_e eType, PHY_ConfirmHandler cb_confirm)
{
   bool retVal = false;

   /* Build a Data.Request message from this data and send to the PHY layer */
   buffer_t *pBuf = RequestBuffer_Get(sizeof(PHY_Request_t));
   if (pBuf != NULL)
   {
      PHY_Request_t *pReq = (PHY_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the PHY_Request_t  information */
      pReq->MsgType         = ePHY_RESET_REQ;
      pReq->pConfirmHandler = cb_confirm;
      pReq->ResetReq.eType  = eType;
      PHY_Request(pBuf);
      retVal = true;
   }
   return retVal;
}

/*!
 *  Send a Data Request to the PHY
 */
bool PHY_DataRequest(uint8_t const *data, uint8_t size, uint16_t chan, float *power, PHY_DETECTION_e detection,
                         PHY_FRAMING_e framing, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters,
                         PHY_ConfirmHandler cb_confirm, uint32_t RxTime, PHY_TX_CONSTRAIN_e TxPriority)
{
   bool retVal = false;

   /* Build a Data.Request message from this data and send to the PHY layer */
   buffer_t *pBuf = RequestBuffer_Get(sizeof(PHY_Request_t)+size);
   if (pBuf != NULL)
   {
      PHY_Request_t *pReq = (PHY_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the PHY_Request_t  information */

      pReq->MsgType                = ePHY_DATA_REQ;
      pReq->pConfirmHandler        = cb_confirm;
      pReq->DataReq.RxTime         = RxTime;
      pReq->DataReq.detection      = detection;
      pReq->DataReq.framing        = framing;
      pReq->DataReq.mode           = mode;
      pReq->DataReq.modeParameters = modeParameters;
      pReq->DataReq.channel        = chan;
      pReq->DataReq.TxPriority     = TxPriority;
      pReq->DataReq.power          = power;
      pReq->DataReq.payload        = (uint8_t*)&pBuf->data[(sizeof(PHY_Request_t))];
      pReq->DataReq.payloadLength  = size;
      (void)memcpy(pReq->DataReq.payload, data, size );
      PHY_Request(pBuf);
      retVal = true;
   }
   return retVal;
}

/*!
 *  Confirmation handler for Get/Set Request
 */
static void PHY_GetSetRequest_CB(PHY_Confirm_t const *confirm, buffer_t const *pReqBuf)
{
   PHY_Request_t *pReq = (PHY_Request_t*) &pReqBuf->data[0];  /*lint !e740 !e826 !e613*/
   PHY_Confirm_t *pConf = (PHY_Confirm_t*) pReq->pConfirm;

   (void)memcpy( pConf, confirm, sizeof(PHY_Confirm_t) );

   // Notify calling function that PHY has got/set attribute
   OS_SEM_Post ( &PHY_AttributeSem_ );
} /*lint !e715 */

/*!
 *  Send a Get Request to the PHY
 */
PHY_GetConf_t PHY_GetRequest( PHY_ATTRIBUTES_e eAttribute )
{
   PHY_GetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = ePHY_GET_SERVICE_UNAVAILABLE;

   OS_MUTEX_Lock( &PHY_AttributeMutex_ ); // Function will not return if it fails
   /* Build a Data.Request message from this data and send to the PHY layer */
   buffer_t *pBuf = RequestBuffer_Get(sizeof(PHY_Request_t));
   if (pBuf != NULL)
   {
      buffer_t *pConf = RequestBuffer_Get(sizeof(PHY_Confirm_t));
      if (pConf != NULL)
      {
         PHY_Request_t *pReq     = (PHY_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the PHY_Request_t  information */
         pReq->MsgType           = ePHY_GET_REQ;
         pReq->pConfirmHandler   = PHY_GetSetRequest_CB;
         pReq->pConfirm          = (PHY_Confirm_t*)pConf->data; /*lint !e740 !e826 */
         pReq->GetReq.eAttribute = eAttribute;
         PHY_Request(pBuf);
         // Wait for PHY to retrieve attribute
         if (OS_SEM_Pend ( &PHY_AttributeSem_, 10*ONE_SEC )) { // Timeout is large because booting the radio can take many seconds on Frodo
            (void)memcpy( &confirm, &((PHY_Confirm_t*)pConf->data)->GetConf, sizeof(PHY_GetConf_t) );  /*lint !e826 */
         }
         BM_free(pConf);
      }
   }
   OS_MUTEX_Unlock( &PHY_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != ePHY_GET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex lock states */
      INFO_printf("ERROR: PHY_GetRequest: status %s, attribute %s", PHY_GetStatusMsg(confirm.eStatus), phy_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 *  Send a Set Request to the PHY
 */
PHY_SetConf_t PHY_SetRequest( PHY_ATTRIBUTES_e eAttribute, void const *val)
{
   PHY_SetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = ePHY_SET_SERVICE_UNAVAILABLE;

   OS_MUTEX_Lock( &PHY_AttributeMutex_ ); // Function will not return if it fails
   /* Build a Data.Request message from this data and send to the PHY layer */
   buffer_t *pBuf = RequestBuffer_Get(sizeof(PHY_Request_t));

   if (pBuf != NULL)
   {
      buffer_t *pConf = RequestBuffer_Get(sizeof(PHY_Confirm_t));
      if (pConf != NULL)
      {
         PHY_Request_t *pReq     = (PHY_Request_t*)pBuf->data; /*lint !e740 !e826 payload holds the PHY_Request_t  information */
         pReq->MsgType           = ePHY_SET_REQ;
         pReq->pConfirmHandler   = PHY_GetSetRequest_CB;
         pReq->pConfirm          = (PHY_Confirm_t*)pConf->data; /*lint !e740 !e826 */
         pReq->SetReq.eAttribute = eAttribute;
         (void)memcpy((uint8_t *)&pReq->SetReq.val, (uint8_t *)val, sizeof(PHY_ATTRIBUTES_u)); /*lint !e420 Apparent access beyond array for function 'memcpy' */
         PHY_Request(pBuf);
         // Wait for PHY to set attribute
         if (OS_SEM_Pend ( &PHY_AttributeSem_, 10*ONE_SEC )) { // Timeout is large because booting the radio can take many seconds on Frodo
            (void)memcpy( &confirm, &((PHY_Confirm_t*)pConf->data)->SetConf, sizeof(PHY_SetConf_t) ); /*lint !e740 !e826 */
         }
         BM_free(pConf);
      }
   }
   OS_MUTEX_Unlock( &PHY_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != ePHY_SET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex lock states */
      INFO_printf("ERROR: PHY_SetRequest: status %s, attribute %s", PHY_SetStatusMsg(confirm.eStatus), phy_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 * This function is used to increment a u32 counter
 * The counter is not incremented if the value = UINT32_MAX and rollover is set to false
 */
static bool u32_counter_inc(uint32_t* pCounter, bool bRollover)
{
   bool bStatus = false;
   if((bRollover == true) || ( *pCounter < UINT32_MAX ))
   {
      (*pCounter)++;
      bStatus = true;
   }
   return bStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn void PHY_CounterInc(PHY_COUNTER_e ePhyCounter)
 *
 * \brief  This function is used to increment the PHY Counters
 *
 * \param  ePhyCounter Enumerated valued for the counter
 *
 * \return  none
 *
 * \details This functions is intended to handle the rollover if required.
 *          If the counter was modified the CacheFile will be updated.
 *
 ********************************************************************************************************************
 */

void PHY_CounterInc(PHY_COUNTER_e ePhyCounter, uint32_t index)
{
   bool bStatus = false;
   switch(ePhyCounter)
   {  //                                                                        Counter                         rollover
      case ePHY_DataRequestCount             :  bStatus = u32_counter_inc(&CachedAttr.DataRequestCount                , (bool) true); break;
      case ePHY_DataRequestRejectedCount     :  bStatus = u32_counter_inc(&CachedAttr.DataRequestRejectedCount        , (bool) true); break;
      case ePHY_FailedFrameDecodeCount       :  bStatus = u32_counter_inc(&CachedAttr.FailedFrameDecodeCount[index]   , (bool) true); break;
      case ePHY_FailedHcsCount               :  bStatus = u32_counter_inc(&CachedAttr.FailedHcsCount[index]           , (bool) true); break;
      case ePHY_FramesReceivedCount          :  bStatus = u32_counter_inc(&CachedAttr.FramesReceivedCount[index]      , (bool) true); break;
      case ePHY_FramesTransmittedCount       :  bStatus = u32_counter_inc(&CachedAttr.FramesTransmittedCount          , (bool) true); break;
      case ePHY_FailedHeaderDecodeCount      :  bStatus = u32_counter_inc(&CachedAttr.FailedHeaderDecodeCount[index]  , (bool) true); break;
      case ePHY_FailedTransmitCount          :  bStatus = u32_counter_inc(&CachedAttr.FailedTransmitCount             , (bool) true); break;
      case ePHY_MalformedHeaderCount         :  bStatus = u32_counter_inc(&CachedAttr.MalformedHeaderCount[index]     , (bool) true); break;
      case ePHY_PreambleDetectCount          :  bStatus = u32_counter_inc(&CachedAttr.PreambleDetectCount[index]      , (bool) true); break;
      case ePHY_RadioWatchdogCount           :  bStatus = u32_counter_inc(&CachedAttr.RadioWatchdogCount[index]       , (bool) true); break;
      case ePHY_RssiJumpCount                :  bStatus = u32_counter_inc(&CachedAttr.RssiJumpCount[index]            , (bool) true); break;
      case ePHY_RxAbortCount                 :  bStatus = u32_counter_inc(&CachedAttr.RxAbortCount[index]             , (bool) true); break;
      case ePHY_SyncDetectCount              :  bStatus = u32_counter_inc(&CachedAttr.SyncDetectCount[index]          , (bool) true); break;
      case ePHY_ThermalProtectionCount       :  bStatus = u32_counter_inc(&CachedAttr.ThermalProtectionCount          , (bool) true); break;
      case ePHY_TxAbortCount                 :  bStatus = u32_counter_inc(&CachedAttr.TxAbortCount                    , (bool) true); break;
   }
   if(bStatus == true)
   {  // The counter was modified
      writeCache();
   }
}

#if ( EP == 1 )
void PHY_SyncHistory(PHY_SyncHistory_t *pSyncHistory)
{
   (void)memcpy(pSyncHistory, &CachedAttr.SyncHistory, sizeof(PHY_SyncHistory_t));
}

void PHY_RssiHistory_Print(void)
{
   INFO_printf("PHY_RssiHistory[%d %d %d %d %d %d %d %d %d %d ]",
      // These are printed from oldest to newest
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+0)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+1)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+2)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+3)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+4)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+5)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+6)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+7)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+8)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.SyncHistory.Rssi[(CachedAttr.SyncHistory.Index+9)%PHY_SYNC_HISTORY_SIZE]);
#if NOISE_TEST_ENABLED == 1
   INFO_printf("PHY_NoiseHistory[%d %d %d %d %d %d %d %d %d %d ]",
      // These are printed from oldest to newest
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+0)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+1)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+2)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+3)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+4)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+5)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+6)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+7)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+8)%PHY_SYNC_HISTORY_SIZE],
      CachedAttr.NoiseHistory.Rssi[(CachedAttr.SyncHistory.Index+9)%PHY_SYNC_HISTORY_SIZE]);
#endif
}

#endif

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
/*! ********************************************************************************************************************
 *
 * \fn PHY_GET_STATUS_e Demodulator_SINR_Get( uint8_t *demodulatorSINR )
 *
 * \brief Get SINR target for the Modulator/demodulator being used by each radio.
 *
 * \param uint8_t *demodulatorSINR
 *
 * \return  PHY_GET_STATUS_e
 *
 *********************************************************************************************************************/
static PHY_GET_STATUS_e Demodulator_SINR_Get( uint8_t *demodulatorSINR )
{

   for ( uint8_t i=0; i < PHY_RCVR_COUNT; i++ )
   {
      if( PHY_HARD_DEMODULATOR == ConfigAttr.Demodulator[i]  )
      {
         demodulatorSINR[i] = PHY_HARD_DEMODULATOR_SINR_DEFAULT;
      }
      else if( PHY_SOFT_DEMODULATOR == ConfigAttr.Demodulator[i] )
      {
         demodulatorSINR[i] = PHY_SOFT_DEMODULATOR_SINR_DEFAULT;
      }
   }

   return ePHY_GET_SUCCESS;
}

/*! ********************************************************************************************************************
 *
 * \fn PHY_SET_STATUS_e Receive_Power_Margin_Set ( uint8_t rpm )
 *
 * \brief Set the the Receive Power Margin
 *
 * \param uint8_t rpm
 *
 * \return  PHY_SET_STATUS_e
 *
 *********************************************************************************************************************/
static PHY_SET_STATUS_e Receive_Power_Margin_Set( uint8_t rpm )
{
   PHY_SET_STATUS_e     eStatus = ePHY_SET_INVALID_PARAMETER;
   if ( rpm <= RECEIVE_POWER_MARGIN_MAX )
   {
      ConfigAttr.ReceivePowerMargin = rpm;
      writeConfig();
      eStatus = ePHY_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn uint8_t Receive_Power_Margin_Get ( void )
 *
 * \brief Get the Receive Power Margin
 *
 * \param none
 *
 * \return  uint8_t ReceivePowerMargin
 *
 *********************************************************************************************************************/
static uint8_t Receive_Power_Margin_Get( void )
{
   return ( ConfigAttr.ReceivePowerMargin );
}

/*! **********************************************************************************************************************
 *
 * \fn Receive_Power_Threshold_Calculate( float *RxPowerThreshold )
 *
 * \brief To Calculate the RPT
 *
 * \param float *RxPowerThreshold
 *
 * \return PHY_GET_STATUS_e
 *
 * ***********************************************************************************************************************/
static PHY_GET_STATUS_e Receive_Power_Threshold_Calculate( float *RxPowerThreshold )
{
   uint8_t           demodSINRList[ PHY_RCVR_COUNT ]  = {0};
   uint8_t           rxPowerMargin;
#if ( ENABLE_PRNT_PHY_RPT_INFO == 1 )
   char              floatStr[PRINT_FLOAT_SIZE];
#endif

   (void)Demodulator_SINR_Get( demodSINRList );
   rxPowerMargin = Receive_Power_Margin_Get();

   for ( uint8_t i = RADIO_FIRST_RX; i < PHY_RCVR_COUNT; i++ )
   {
      if ( ConfigAttr.RxChannels[i] < PHY_INVALID_CHANNEL )  // Check the validity of the channel
      {
         if( PhyRssiStats10m_Copy_[i].Num >= BKRSSI_MIN_SAMPLES )
         {
            RxPowerThreshold[i] = PhyRssiStats10m_Copy_[i].LogAvg + 2 *( PhyRssiStats10m_Copy_[i].StdDev ) + demodSINRList[i] + rxPowerMargin;
         }
         else
         {
            // TODO: DG: Add "PhyCounters" for field debug purpose
            RxPowerThreshold[i] = NAN; /*lint !e10 */
            //TODO: DG: PHY_CounterInc( ePHY_InvalidRPTsCount, i);  // Increment the counter
            RPT_PRNT_INFO('I',"Warning! %d Radio, Nsamples: %d", i, PhyRssiStats10m_Copy_[i].Num );
         }
      }
      else
      {
         RxPowerThreshold[i] = NAN; /*lint !e10 */
         RPT_PRNT_INFO('I',"Radio: %d is NOT configured!", i );
      }
      RPT_PRNT_INFO( 'R', "RPT for %d RxChannel  = %-6s", i, DBG_printFloat( floatStr, RxPowerThreshold[i], 3 ) );
   }

   return ( ePHY_GET_SUCCESS );
}

#endif // end of ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) ) section
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
/***********************************************************************************************************************
   Function Name: phyVSWRtimerStart

   Purpose: Starts VSWR hold off timer.

   Arguments:  none

   Returns: void
***********************************************************************************************************************/
void  phyVSWRtimerStart( void )
{
   (void)TMR_ResetTimer( VSWRtimerCfg.usiTimerId, 0 );
}
/***********************************************************************************************************************
   Function Name: phyVSWRTimeout

   Purpose: Callback function invoked on expiration of VSWR holdoff timer.

   Arguments:  uint8_t  param - not used
               void     *ptr  - pointer to VSWRholdOff

   Returns: void
***********************************************************************************************************************/
static void  phyVSWRTimeout( uint8_t param, void *ptr )
{
   (void)param;
   (void)ptr;

   (void)SM_StartRequest( eSM_START_STANDARD, NULL ); /* Re-enable the transmitter    */
   ERR_printf( "VSWR timer expired; re-enabling transmitter" );
}
#endif
