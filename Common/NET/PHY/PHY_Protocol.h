/***********************************************************************************************************************
 *
 * Filename: PHY_Protocol.h
 *
 * Contents: typedefs and #defines for the PHY implementation
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef PHY_Protocol_H
#define PHY_Protocol_H

/* INCLUDE FILES */
#include "buffer.h"
#include "portable_aclara.h"

/* MACRO DEFINITIONS */
#define TEST_900MHZ 0

#if ( TEST_900MHZ == 1 )
#define PHY_AFC_MIN_FREQ_STEP          28.6f  // Synthesizer frequency resolution from 850MHz to 1050MHz according to data sheet
#else
#define PHY_AFC_MIN_FREQ_STEP          14.3f  // Synthesizer frequency resolution from 420MHz to 525MHz according to data sheet
#endif

#define PHY_MAX_CHANNEL                 32u   /*!< The maximum number of channels supported by the PHY. This is not the number of radio. */
#define PHY_MAX_PAYLOAD                239u   /*!< The maximum number of bytes that can be received or transmitted in the PHY Payload field */
#define PHY_MIN_TX_PAYLOAD             125u   /*!< The minimum number of bytes that can be transmitted in the PHY Payload field */
#define PHY_DEFAULT_TX_PAYLOAD         PHY_MIN_TX_PAYLOAD /*!< The default number of bytes that can be transmitted in the PHY Payload field */
#define PHY_MAX_TX_PAYLOAD             PHY_MAX_PAYLOAD  /*!< The maximum number of bytes that can be transmitted in the PHY Payload field */
#define PHY_CCA_OFFSET_DEFAULT           15   /*!< The difference between noise floor and CCA threshold in dBm */
#if ( LG_WORST_CASE_TEST == 0 )
#define PHY_CCA_OFFSET_MAX               30   /*!< Max value */
#else
#define PHY_CCA_OFFSET_MAX               100   /*!< Max value for the Test Case*/
#endif
#define PHY_CCA_THRESHOLD_DEFAULT      -103   /*!< The threshold against which the current RSSI is compared to determine if the channel is idle or busy. (-200 to +55)  */
#define PHY_CCA_THRESHOLD_MIN          -200   /*!< The min CCA threshold */
#define PHY_CCA_THRESHOLD_MAX            55   /*!< The max CCA threshold */
#define PHY_NOISE_ESTIMATE_RATE_DEFAULT  15   /*!< Every 15 minutes */
#define PHY_NOISE_ESTIMATE_COUNT_MIN      1   /*!< Minimum value */
#define PHY_NOISE_ESTIMATE_COUNT_MAX     20   /*!< Maximum value */
#define PHY_RSSI_JUMP_THRESHOLD_DEFAULT  16   /*!< The threshold in dB for the RSSI change above which the receiver will reset and reacquire the newer signal */
#define PHY_RSSI_JUMP_THRESHOLD_MIN       0   /*!< Minimum value */
#define PHY_RSSI_JUMP_THRESHOLD_MAX      20   /*!< Maximum value */
#define PHY_TX_CHANNEL_INDEX_DEFAULT      0   /*!< Default TX Channel index */
#define PHY_RX_CHANNEL_INDEX_DEFAULT      0   /*!< Default RX Channel index */
#define PHY_HEADER_SIZE                  8u   /*!< The PHY header size */
#define PHY_FEC_SIZE                    16u   /*!< The PHY FEC size for RS(255,239) */
#define PHY_FREQUENCY_OFFSET_MIN      -2000   /*!< Minimum value */
#define PHY_FREQUENCY_OFFSET_MAX       2000   /*!< Maximum value */

#define PHY_MAX_FRAME                  (PHY_HEADER_SIZE + PHY_MAX_PAYLOAD + PHY_FEC_SIZE) /*!< Maximum over the air frame */
                                              /*!< Header (8 bytes) + PHY_MAX_PAYLOAD + Max FEC (16 bytes for RS(255,239) ) */

#define PHY_SRFN_OVERHEAD               (24U + 8U + PHY_HEADER_SIZE + 16U) /*!( This is the preamble + sync + PHY header + FEC. This should probably retrieve through a PHY attribute instead of hardcoded. */
#define PHY_STAR_OVERHEAD               (11U + 3U + 3U) /*!( This is the preamble + sync + FEC. This should probably retrieve through a PHY attribute instead of hardcoded. */

#if ( EP == 1 )
#define PHY_RCVR_COUNT 1u  /*!<  The number of receivers defined within the PHY. For EP    = 1 */
#else
#define PHY_RCVR_COUNT 9u  /*!<  The number of receivers defined within the PHY. For Frodo = 9 */
#endif

#define PHY_INVALID_FREQ       999999999u /*!< Invalid frequency */
#if ( TEST_900MHZ == 1)
#define PHY_MIN_FREQ           900000000u /*!< Minimum valid frequency */
#define PHY_MAX_FREQ           941000000u /*!< Maximum valid frequency */
#else
#define PHY_MIN_FREQ           450000000u /*!< Minimum valid frequency */
#define PHY_MAX_FREQ           470000000u /*!< Maximum valid frequency */
#endif
#define PHY_CHANNEL_BANDWIDTH       6250u /*!< Channel bandwidth */
#define PHY_INVALID_FREQ_TO_CHANNEL (PHY_MAX_FREQ+PHY_CHANNEL_BANDWIDTH) /*!< Equivalent of invalid channel PHY_INVALID_CHANNEL */
#define PHY_NUM_CHANNELS       ((PHY_MAX_FREQ-PHY_MIN_FREQ)/PHY_CHANNEL_BANDWIDTH) /*!< Number of channels available */
#define PHY_INVALID_CHANNEL    (PHY_NUM_CHANNELS+1) /*!< Invalid channel */

#define PHY_STAR_MAX_LENGTH           48u /*!< Maximum length when FEC is present. Must be modulo 3 */
                                          /*!< This is the maximum message size over the air based on the fact that a record  */
                                          /*!< between J-board and main board is 64 bytes and 15 bytes are used for metadata. */

#define CHANNEL_TO_FREQ(x)     ((x*PHY_CHANNEL_BANDWIDTH)+PHY_MIN_FREQ)
#define FREQ_TO_CHANNEL(x)     (uint16_t)((x-PHY_MIN_FREQ)/PHY_CHANNEL_BANDWIDTH)

#define RSSI_RAW_TO_DBM(x)  ((((float)(x)/2.0f)-64.0f)-70.0f)
#define RSSI_DBM_TO_RAW(x)  (uint8_t)((((float)(x)+70.0f)+64.0f)*2.0f)

// todo: 2/16/2015 4:10:44 PM [BAH] - Convert all message to PHY Messages
#define eSYSFMT_STACK_PHY_MSG  eSYSFMT_STACK_DATA_CONF

/* TYPE DEFINITIONS */

// Seconds in fixed point Q32.32 format
// WARNING: QFrac has to be before "seconds" for the QSecFrac union to work properly.
// This is because of the little endian nature of the processor
typedef struct
{
   union { // anonymous union
      struct {
         uint32_t QFrac;   /* Fractional time in 1/2^32 second */
         uint32_t seconds; /* Time in seconds since January 1st, 1970 */
      }; // anonymous struct
      uint64_t QSecFrac;   /* Combined seconds and QFrac */
   };
} TIMESTAMP_t;

/*! CCA characterization type */
typedef enum CCA_RSSI_TYPEe
{
   eCCA_RSSI_TYPE_AVERAGE,    /*!< Average RSSI method was used */
   eCCA_RSSI_TYPE_STABLE,     /*!< Stable RSSI method was used */
   eCCA_RSSI_TYPE_UNKNOWN     /*!< Can't be categorized as either average or stable */
}CCA_RSSI_TYPEe;

/*! PHY Modes */
typedef enum PHY_MODE_e
{
   ePHY_MODE_0 = 0,           /*!< 4-GFSK,                      , 4800 baud */
   ePHY_MODE_1,               /*!< 4-GFSK,                      , 4800 baud */
   ePHY_MODE_2,               /*!< 2-FSK,   (63,59) Reed-Solomon, 7200 baud */
   ePHY_MODE_3,               /*!< 2-FSK,                       , 2400 baud */
#if ( TEST_DEVIATION == 1 )
#if ( EP == 1 )
   ePHY_MODE_4,               /*!< 4-GFSK,                      , 4800 baud */ // 600Hz deviation test
   ePHY_MODE_5,               /*!< 4-GFSK,                      , 4800 baud */ // 700Hz deviation test
   ePHY_MODE_6,               /*!< 4-GFSK,                      , 4800 baud */ // 800Hz deviation test
#else
   ePHY_MODE_4,               /*!< 4-GFSK,                      , 4800 baud */ // 700Hz deviation test IF 6.69
   ePHY_MODE_5,               /*!< 4-GFSK,                      , 4800 baud */ // 700Hz deviation test    7.43
   ePHY_MODE_6,               /*!< 4-GFSK,                      , 4800 baud */ // 800Hz deviation test    7.43
   ePHY_MODE_7,               /*!< 4-GFSK,                      , 4800 baud */ // 800Hz deviation test    8.23
#endif
#endif
   ePHY_MODE_LAST
// ePHY_MODE_NONE = 15        /*!< none, none, none */
}PHY_MODE_e;

/*! PHY Mode parameters */
typedef enum PHY_MODE_PARAMETERS_e
{
   ePHY_MODE_PARAMETERS_0 = 0, /*!< RS(63,59)   Reed-Solomon */
   ePHY_MODE_PARAMETERS_1,     /*!< RS(255,239) Reed-Solomon */
   ePHY_MODE_PARAMETERS_LAST
}PHY_MODE_PARAMETERS_e;

/*! PHY Detection */
typedef enum PHY_DETECTION_e
{                             /*!< Preamble pattern            Sync word    Preamble     Sync word    */
                              /*!<                                          detection    Match        */
                              /*!<                                          count (bits) Count (bits) */
   ePHY_DETECTION_0 = 0,      /*!< 0xAAAAAAAAAAAAAAAAAAAAAAAA, 0x89445BC1,  32,          32 */
   ePHY_DETECTION_1,          /*!< 0x5555555555555555555555  , 0x55D2B4,    20,          23 */
   ePHY_DETECTION_2,          /*!< 0x55555555555555          , 0xCCCCCCD2,  20,          32 */
   ePHY_DETECTION_3,          /*!< 0x5555555555555555555555  , 0x55AB49,    20,          23 */
   ePHY_DETECTION_4,          /*!< 0xAAAAAAAAAAAAAA          , 0x3333332D,   8,          32 */
   ePHY_DETECTION_5,          /*!< 0xAAAAAAAAAAAA            , 0x89445BC1,  16,          32 */
   ePHY_DETECTION_LAST
}PHY_DETECTION_e;

/*! PHY Framing */
typedef enum PHY_FRAMING_e
{
   ePHY_FRAMING_0 = 0,        /*!< Standard format   */
   ePHY_FRAMING_1,            /*!< STAR Generation 1 */
   ePHY_FRAMING_2,            /*!< STAR Generation 2 */
   ePHY_FRAMING_LAST
}PHY_FRAMING_e;

/*! PHY TX constrain */
typedef enum PHY_TX_CONSTRAIN_e
{
   ePHY_TX_CONSTRAIN_REGULAR = 0, /*!< Send packet ASAP */
   ePHY_TX_CONSTRAIN_EXACT,       /*!< Send packet at exact time */
   ePHY_TX_CONSTRAIN_LAST
}PHY_TX_CONSTRAIN_e;

/*! PHY TX Frame Structure */
typedef struct
{
   PHY_MODE_e            Mode;          /*!< The type of modulation, baud rate and channel coding used on the PHY payload.  See Section 3.1.3 for more details. */
   PHY_MODE_PARAMETERS_e ModeParameters;/*!< Provides additionnal information to the specific mode. */
   PHY_FRAMING_e         Framing;       /*!< The framing used: SRFN, STAR Gen I or Gen II */
   PHY_DETECTION_e       Detection;     /*!< The detection preamble and sync used for TX */
   uint8_t               Version;       /*!< The version of this protocol that is being used.  Currently set to 0. */
   uint8_t               Length;        /*!< the length of the PHY payload in bytes (0 to PhyMaxload) */
   uint8_t               Payload[PHY_MAX_FRAME]; /*!<  */
} TX_FRAME_t;

/*! PHY RX frame Structure */
typedef struct
{
   TIMESTAMP_t           syncTime;               // Sync word detection timestamp (seconds, fractional seconds)
   float32_t             rssi_dbm;               // RSSI Value (-200 to +50)
#if NOISE_TEST_ENABLED == 1
   float32               noise_dbm;              // NOISE Value (-200 to +50)
#endif
   uint32_t              syncTimeCYCCNT;         // Sync word detection timestamp in DWT_CYCCNT.
   int16_t               frequencyOffset;        // Frequency offset as reported by the radio
   uint8_t               Payload[PHY_MAX_FRAME]; // Maximum PHY frame size
   PHY_MODE_e            mode;                   // The PHY mode of operation used when transmitting the payload
   PHY_MODE_PARAMETERS_e modeParameters;         // Provides additional information to the specific mode. */
   PHY_FRAMING_e         framing;                // The framing used: SRFN, STAR Gen I or Gen II
} RX_FRAME_t;

/*! PHY Operational States */
typedef enum
{
   ePHY_STATE_STANDBY,  // This must match ePHY_START_STANDBY
   ePHY_STATE_READY,    // This must match ePHY_START_READY
   ePHY_STATE_READY_TX, // This must match ePHY_START_READY_RX
   ePHY_STATE_READY_RX, // This must match ePHY_START_READY_TX
   ePHY_STATE_SHUTDOWN
} PHY_STATE_e;
/*!
 * Request Types to PHY
 */
typedef enum
{
   ePHY_CCA_REQ,         /*!< PHY CCA_REQ        MAC to PHY */
   ePHY_GET_REQ,         /*!< PHY GET_REQ        MAC to PHY */
   ePHY_SET_REQ,         /*!< PHY SET_REQ        MAC to PHY */
   ePHY_START_REQ,       /*!< PHY START_REQ      MAC to PHY */
   ePHY_STOP_REQ,        /*!< PHY STOP_REQ       MAC to PHY */
   ePHY_RESET_REQ,       /*!< PHY RESET_REQ      MAC to PHY */
   ePHY_DATA_REQ,        /*!< PHY DATA_REG       MAC to PHY */
} PHY_REQ_TYPE_t;

/*!
 * Confirm Types returned from PHY
 */
typedef enum
{
   ePHY_CCA_CONF,        /*!< PHY CCA_CONFIRM      PHY to MAC */
   ePHY_GET_CONF,        /*!< PHY GET_CONFIRM      PHY to MAC */
   ePHY_SET_CONF,        /*!< PHY SET_CONFIRM      PHY to MAC */
   ePHY_START_CONF,      /*!< PHY START_CONFIRM    PHY to MAC */
   ePHY_STOP_CONF,       /*!< PHY STOP_CONFIRM     PHY to MAC */
   ePHY_RESET_CONF,      /*!< PHY STOP_CONFIRM     PHY to MAC */
   ePHY_DATA_CONF,       /*!< PHY DATA_CONFIRM     PHY to MAC */
} PHY_CONF_TYPE_t;

/*!
 * Indication types from the PHY
 */
typedef enum
{
   ePHY_DATA_IND,        /*!< PHY_DATA_INDICATION  PHY to MAC */
   //ePHY_NOISE_IND,       /*!< PHY_NOISE_INDICATION  PHY to MAC */
} PHY_IND_TYPE_t;

/*!
 * PHY Get/Set Attributes
 */
typedef enum
{
   ePhyAttr_AfcAdjustment,                /* Read Only  */
#if ( EP == 1 )
   ePhyAttr_AfcEnable,                    /* Read/Write */
   ePhyAttr_AfcRssiThreshold,             /* Read/Write */
   ePhyAttr_AfcTemperatureRange,          /* Read/Write */
#endif
   ePhyAttr_AvailableChannels,            /* Read Only  */
   ePhyAttr_CcaAdaptiveThreshold,         /* Read Only  */
   ePhyAttr_CcaAdaptiveThresholdEnable,   /* Read/Write */
   ePhyAttr_CcaOffset,                    /* Read/Write */
   ePhyAttr_CcaThreshold,                 /* Read/Write */
   ePhyAttr_DataRequestCount,             /* Read Only  */
   ePhyAttr_DataRequestRejectedCount,     /* Read Only  */
   ePhyAttr_Demodulator,                  /* Read/Write */
   ePhyAttr_FailedFrameDecodeCount,       /* Read Only  */
   ePhyAttr_FailedHcsCount,               /* Read Only  */
   ePhyAttr_FailedHeaderDecodeCount,      /* Read Only  */
   ePhyAttr_FailedTransmitCount,          /* Read Only  */
   ePhyAttr_FramesReceivedCount,          /* Read Only  */
   ePhyAttr_FramesTransmittedCount,       /* Read Only  */
   ePhyAttr_FrontEndGain,                 /* Read/Write */
   ePhyAttr_LastResetTime,                /* Read Only  */
   ePhyAttr_MalformedHeaderCount,         /* Read Only  */
   ePhyAttr_MinRxBytes,                   /* Read/Write */
   ePhyAttr_MaxTxPayload,                 /* Read/Write */
   ePhyAttr_NoiseEstimate,                /* Read/Write */
#if ( EP == 1 )
   ePhyAttr_NoiseEstimateBoostOn,         /* Read/Write */
#endif
   ePhyAttr_NoiseEstimateCount,           /* Read/Write */
   ePhyAttr_NoiseEstimateRate,            /* Read/Write */
   ePhyAttr_NumChannels,                  /* Read Only  */
   ePhyAttr_PowerSetting,                 /* Read/Write */
   ePhyAttr_PreambleDetectCount,          /* Read/Write */
   ePhyAttr_RadioWatchdogCount,           /* Read Only  */
   ePhyAttr_RcvrCount,                    /* Const      */
   ePhyAttr_RssiJumpCount,                /* Read Only  */
   ePhyAttr_RssiJumpThreshold,            /* Read/Write */
#if ( DCU == 1 )
   ePhyAttr_RssiStats10m,                 /* Read/Write */ /* Note: 1/13/21: System Architect's comments: This isn't official PHY Attribute. This needs to be a new PHY service which provides Noise info*/
#if ( MAC_LINK_PARAMETERS == 1 )
   ePhyAttr_RssiStats10m_Copy,            /* Read/Write */ /* Note: 1/13/21: System Architect's comments: This isn't official PHY Attribute. This needs to be a new PHY service which provides Noise info*/
#endif
   ePhyAttr_RssiStats60m,                 /* Read/Write */ /* Note: 1/13/21: System Architect's comments: This isn't official PHY Attribute. This needs to be a new PHY service which provides Noise info*/
#endif
   ePhyAttr_RxAbortCount,                 /* Read/Write */
   ePhyAttr_RxChannels,                   /* Read/Write */
   ePhyAttr_RxDetectionConfig,            /* Read/Write */
   ePhyAttr_RxFramingConfig,              /* Read/Write */
   ePhyAttr_RxMode,                       /* Read/Write */
   ePhyAttr_State,                        /* Read Only  */
   ePhyAttr_SyncDetectCount,              /* Read Only  */
#if ( TEST_SYNC_ERROR == 1 )
   ePhyAttr_SyncError,                    /* Read/Write */
#endif
   ePhyAttr_Temperature,                  /* Read Only  */
   ePhyAttr_ThermalControlEnable,         /* Read/Write */
   ePhyAttr_ThermalProtectionCount,       /* Read Only  */
   ePhyAttr_ThermalProtectionEnable,      /* Read/Write */
   ePhyAttr_ThermalProtectionEngaged,     /* Read Only  */
   ePhyAttr_TxAbortCount,                 /* Read/Write */
   ePhyAttr_TxChannels,                   /* Read/Write */
   ePhyAttr_TxOutputPowerMax,             /* Read/Write */
   ePhyAttr_TxOutputPowerMin,             /* Read/Write */
   ePhyAttr_VerboseMode,                  /* Read/Write */
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   ePhyAttr_ReceivePowerMargin,           /* Read/Write */
   ePhyAttr_ReceivePowerThreshold,        /* Read Only */	/* Note: This might not be an official Phy Attribute */
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   ePhyAttr_fngVSWR,                      /* Read Only  */
   ePhyAttr_fngVswrNotificationSet,       /* Read/Write */
   ePhyAttr_fngVswrShutdownSet,           /* Read/Write */
   ePhyAttr_fngForwardPower,              /* Read Only  */
   ePhyAttr_fngReflectedPower,            /* Read Only  */
   ePhyAttr_fngForwardPowerSet,           /* Read/Write */
   ePhyAttr_fngFowardPowerLowSet,         /* Read/Write */
   ePhyAttr_fngfemMfg,                    /* Read/Write */
#endif

#if ( DCU == 1 )
   ePhyAttr_txEnabled,                    /* Read/Write  */
#endif
} PHY_ATTRIBUTES_e;

#if ( DCU == 1 )
/*!
 * PHY RSSI Avg Statistics
 */
typedef struct {
   uint16_t Num;        /*!< The number of RSSI measurements previously used in calculations */
   uint8_t  Prev;       /*!< The previous (historical) RSSI measurement, to allow replacement if taken during message reception */
   uint8_t  Min;        /*!< Min RSSI measurements */
   uint8_t  Max;        /*!< Max RSSI measurements */
   float    LogAvg;     /*!< RSSI running Log scale average */
   float    StdDev;     /*!< Running standard deviation (requires additional calculation before reporting) */
} PHY_RssiStats_s;
#endif

/*!
 * PHY Get/Set Parameters
 */
typedef union{
   int16_t     AfcAdjustment[PHY_RCVR_COUNT];            /*!< The amount of frequency adjustment made by the AFC algorithm in units of aMinFreqStep. */
#if ( EP == 1 )
   bool        AfcEnable;                                /*!< Indicates if the AFC function on this device is enabled (TRUE) or disabled (FALSE), default = true */
   int16_t     AfcRssiThreshold;                         /*!< The RSSI threshold in dBm above which AFC measurements must exceed in order to be used for processing. ( -200 to +55, default -115 ) */
   int8_t      AfcTemperatureRange[2];                   /*!< The range of temperature in Celsius within which an AFC sample must fall in order to be used for processing. */
#endif
   uint16_t    AvailableChannels[PHY_MAX_CHANNEL];       /*!< List of Available Channels */
   int16_t     CcaAdaptiveThreshold[PHY_MAX_CHANNEL];    /*!< The adaptive CCA threshold for each channel */
   bool        CcaAdaptiveThresholdEnable;               /*!< Indicates if the CCA threshold will be dynamically adjusted for each channel */
   uint8_t     CcaOffset;                                /*!< CCA Offset */
   int16_t     CcaThreshold;                             /*!< CCA Threshold */
   uint32_t    DataRequestCount;                         /*!< The number of DATA.requests received from the upper layers. */
   uint32_t    DataRequestRejectedCount;                 /*!< The number of DATA.requests received from the upper layers that were rejected. */
   uint8_t     Demodulator[PHY_RCVR_COUNT];              /*!< List of demodulator used by the each respective receiver. */
   uint32_t    FailedFrameDecodeCount[PHY_RCVR_COUNT];   /*!< Failed Frame Decode Count (0-(2^32-1))*/
   uint32_t    FailedHcsCount[PHY_RCVR_COUNT];           /*!< Failed Hcs Count (0-(2^32-1))*/
   uint32_t    FailedHeaderDecodeCount[PHY_RCVR_COUNT];  /*!< Failed Header Decode Count (0-(2^32-1))*/
   uint32_t    FailedTransmitCount;                      /*!< The number of frame transmission attempts that failed for some internal reason. */
   uint32_t    FramesReceivedCount[PHY_RCVR_COUNT];      /*!< Frames Received Count (0-(2^32-1))*/
   uint32_t    FramesTransmittedCount;                   /*!< Frames Transmitted Count (0-(2^32-1))*/
   int8_t      FrontEndGain;                             /*!< Receive gain between antenna and receiver input */
   TIMESTAMP_t LastResetTime;                            /*!< Last reset timestamp. */
   uint32_t    MalformedHeaderCount[PHY_RCVR_COUNT];     /*!< Malformed Header Count (0-(2^32-1))*/
   uint16_t    MinRxBytes;                               /*!< The minimum number of bytes for the PHY to read in OTA after a sync word is detected. */
   uint16_t    MaxTxPayload;                             /*!< The maximum number of bytes in the PHY TX payload. */
   int16_t     NoiseEstimate[PHY_MAX_CHANNEL];           /*!< Noise floor estimate for each channel */
#if ( EP == 1 )
   int16_t     NoiseEstimateBoostOn[PHY_MAX_CHANNEL];    /*!< Noise floor estimate for each channel with capacitor boost on */
#endif
   uint8_t     NoiseEstimateCount;                       /*!< Number of valid noise reading for noise average */
   uint8_t     NoiseEstimateRate;                        /*!< Frequency that RSSI is measured */
   uint8_t     NumChannels;                              /*!< Actual Number of Channels Programmed */
   uint8_t     PowerSetting;                             /*!< Transmit Power setting */
   uint32_t    PreambleDetectCount[PHY_RCVR_COUNT];      /*!< Preamble Detect Count (0-(2^32-1))*/
   uint32_t    RadioWatchdogCount[PHY_RCVR_COUNT];       /*!< The number of times the radio failed to respond in the given amount of time to a PHY request action */
   uint8_t     RcvrCount;                                /*!< The number of receivers defined within the PHY */
   uint32_t    RssiJumpCount[PHY_RCVR_COUNT];            /*!< Rssi Jump Count (0-(2^32-1))*/
   uint8_t     RssiJumpThreshold;                        /*!< Rssi Jump Threshold (0-20)*/
#if ( DCU == 1 )
   PHY_RssiStats_s RssiStats[PHY_RCVR_COUNT];            /*!< RSSI Avg statistics */
#endif
   uint32_t    RxAbortCount[PHY_RCVR_COUNT];             /*!< RX abort Count (0-(2^32-1))*/
   uint16_t    RxChannels[PHY_RCVR_COUNT];               /*!< Receive Channels */
   PHY_DETECTION_e RxDetectionConfig[PHY_RCVR_COUNT];    /*!< The detection configuration the PHY will use on the corresponding receiver  */
   PHY_FRAMING_e RxFramingConfig[PHY_RCVR_COUNT];        /*!< The framing configuration the PHY will use on the corresponding receiver  */
   PHY_MODE_e  RxMode[PHY_RCVR_COUNT];                   /*!< The PHY mode used on the corresponding receiver  */
   PHY_STATE_e State;                                    /*!< The current state of the PHY layer */
   uint32_t    SyncDetectCount[PHY_RCVR_COUNT];          /*!< Sync Detect Count (0-(2^32-1))*/
#if ( TEST_SYNC_ERROR == 1 )
   uint8_t     SyncError;                                /*!< Sync bit error*/
#endif
   int16_t     Temperature;                              /*!< Radio Temperature in Celcius */
   bool        ThermalControlEnable;                     /*!< Indicates if the thermal control feature is enabled (TRUE) or disabled (FALSE). */
   uint32_t    ThermalProtectionCount;                   /*!< The number of times the thermal protection feature has detected a peripheral temperature exceeding a given limit. */
   bool        ThermalProtectionEnable;                  /*!< Indicates if the thermal protection feature is enabled (TRUE) or disabled (FALSE). */
   bool        ThermalProtectionEngaged;                 /*!< Indicates if a PHY peripheral has exceeded an allowed temperature limit.  If phyThermalProtectionEnable is set to FALSE, this field will also be set to FALSE. */
   uint32_t    TxAbortCount;                             /*!< TX abort Count (0-(2^32-1))*/
   uint16_t    TxChannels[PHY_MAX_CHANNEL];              /*!< Transmit Channels */
   float       TxOutputPowerMax;                         /*!( The maximum acceptable power level in dBm for the transmitter at the device output. */
   float       TxOutputPowerMin;                         /*!( The maximum acceptable power level in dBm for the transmitter at the device output. */
   bool        VerboseMode;                              /*!< Indicates if verbose mode is turned on causing DATA_VERBOSE.indications to be sent upon receipt of OTA frames. */
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   uint8_t     ReceivePowerMargin;                       /*!< A node specific margin added to the calculation of receive power thresholds(RPT) to optimize performance on a per receiver node resolution. */
   float       ReceivePowerThreshold[PHY_RCVR_COUNT];    /*!< RPT used by MAC-Link-Parameters command. Note: This is for the ALC-MVP, may not be an official PHY attribute */
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
   uint16_t    fngVSWR;                                  /*!< Latest measured VSWR ratio */
   uint16_t    fngVswrNotificationSet;                   /*!< VSWR ratio that triggers an alarm */
   uint16_t    fngVswrShutdownSet;                       /*!< VSWR ratio that triggers transmitter shut down */
   uint16_t    fngForwardPower;                          /*!< Measured RF forward power (dBm) */
   uint16_t    fngReflectedPower;                        /*!< Measured RF reflected power (dBm) */
   uint16_t    fngForwardPowerSet;                       /*!< Desired RF transmit power   */
   uint16_t    fngFowardPowerLowSet;                     /*!< Measured RF forward power (dBm) that triggers transmitter shutdown */
   uint8_t     fngfemMfg;                                /*!< FEM manufacturer   */
#endif
#if ( DCU == 1 )
   bool        txEnabled;                                /*!< Enables/disables TB transmitter   */
#endif
} PHY_ATTRIBUTES_u;


/*!
 * Start.Request - parameters
 */
typedef enum
{
   ePHY_START_STANDBY,   /*!< Go to standby  */ // This must match ePHY_STATE_STANDBY
   ePHY_START_READY,     /*!< Go to ready    */ // This must match ePHY_STATE_READY
   ePHY_START_READY_TX,  /*!< Go to ready TX */ // This must match ePHY_STATE_TX
   ePHY_START_READY_RX   /*!< Go to ready RX */ // This must match ePHY_STATE_RX
} PHY_START_e;

/*!
 * Reset.Request - parameters
 */
typedef enum
{
   ePHY_RESET_ALL,       /*!< Reset all attributes */
   ePHY_RESET_STATISTICS /*!< Reset statistics */
} PHY_RESET_e;

/*!
 * PHY CCA - Confirmation Status
 */
typedef enum
{
   ePHY_CCA_SUCCESS = 0,         /*!< PHY CCA - Success */
   ePHY_CCA_SERVICE_UNAVAILABLE, /*!< PHY CCA - Service Unavailable */
   ePHY_CCA_BUSY,                /*!< PHY CCA - Busy */
   ePHY_CCA_INVALID_CONFIG,      /*!< PHY CCA - Invalid Config */
   ePHY_CCA_INVALID_PARAMETER    /*!< PHY CCA - Invalid Parameter */
} PHY_CCA_STATUS_e;

/*!
 * PHY Get - Confirmation Status
 */
typedef enum
{
   ePHY_GET_SUCCESS = 0,           /*!< PHY GET - Success */
   ePHY_GET_UNSUPPORTED_ATTRIBUTE, /*!< PHY GET - Unsupported attribute */
   ePHY_GET_SERVICE_UNAVAILABLE    /*!< PHY GET - Service Unavailable */
} PHY_GET_STATUS_e;

/*!
 * PHY Set - Confirmation Status
 */
typedef enum
{
   ePHY_SET_SUCCESS = 0,           /*!< PHY SET - Success */
   ePHY_SET_READONLY,              /*!< PHY SET - Read only */
   ePHY_SET_UNSUPPORTED_ATTRIBUTE, /*!< PHY SET - Unsupported attribute */
   ePHY_SET_INVALID_PARAMETER,     /*!< PHY SET - Invalid paramter */
   ePHY_SET_SERVICE_UNAVAILABLE    /*!< PHY SET - Service Unavailable */
} PHY_SET_STATUS_e;

/*!
 * PHY Start - Confirmation Status
 */
typedef enum
{
   ePHY_START_SUCCESS = 0,       /*!< PHY START - Success */
   ePHY_START_FAILURE,           /*!< PHY START - Failure */
   ePHY_START_INVALID_PARAMETER, /*!< PHY START - Invalid Parameter */
} PHY_START_STATUS_e;

/*!
 * PHY Stop - Confirmation Status
 */
typedef enum
{
   ePHY_STOP_SUCCESS = 0,        /*!< PHY STOP - Success */
   ePHY_STOP_FAILURE,            /*!< PHY STOP - Failure */
   ePHY_STOP_SERVICE_UNAVAILABLE /*!< PHY STOP - Service Unavailable */
} PHY_STOP_STATUS_e;

/*!
 * PHY Reset - Confirmation Status
 */
typedef enum
{
   ePHY_RESET_SUCCESS = 0,         /*!< PHY RESET - Success */
   ePHY_RESET_FAILURE,             /*!< PHY RESET - Failure */
   ePHY_RESET_SERVICE_UNAVAILABLE, /*!< PHY RESET - Service Unavailable */
   ePHY_RESET_INVALID_PARAMETER    /*!< PHY RESET - Invalid Parameter */
} PHY_RESET_STATUS_e;

/*!
 * PHY Data - Confirmation Status
 */
typedef enum
{
   ePHY_DATA_SUCCESS = 0,         /*!< PHY DATA - Success */
   ePHY_DATA_SERVICE_UNAVAILABLE, /*!< PHY DATA - Service Unavailable */
   ePHY_DATA_TRANSMISSION_FAILED, /*!< PHY DATA - TX Failed */
   ePHY_DATA_INVALID_PARAMETER,   /*!< PHY DATA - Invalid Paramter */
   ePHY_DATA_BUSY,                /*!< PHY DATA - Busy */
   ePHY_DATA_THERMAL_OVERRIDE     /*!< PHY DATA - Thermal override */
} PHY_DATA_STATUS_e;

/*!
 * CCA.Confirm
 */
typedef struct
{
   PHY_CCA_STATUS_e eStatus; /*!< Result of the request for CCA (whether the CCA occurred, not if channel is clear) */
   bool             busy;    /*!< indicates whether the channel is busy, ignore if status is not SUCCESS */
   int16_t          rssi;    /*!< averaged RSSI value vs. the threshold during CCA, ignore if status is not SUCCESS */
} PHY_CCAConf_t;

/*! Get.Confirm */
typedef struct
{
   PHY_ATTRIBUTES_e  eAttribute;  /*!< Specifies the attribute requested */
   PHY_GET_STATUS_e  eStatus;     /*!< Execution status SUCCESS, UNSUPPORTED_ATTRIBUTE, SERVICE_UNAVAILABLE */
   PHY_ATTRIBUTES_u  val;         /*!< Value Returned */
} PHY_GetConf_t;

/*! Set.Confirm */
typedef struct
{
   PHY_ATTRIBUTES_e  eAttribute;  /*!< Specifies the attribute */
   PHY_SET_STATUS_e  eStatus;     /*!< SUCCESS, READ_ONLY, UNSUPPORTED_ATTRIBUTE, INVALID_PARAMETER, SERVICE_UNAVAILABLE */
} PHY_SetConf_t;

/*!
 * Start.Confirm
 */
typedef struct
{
   PHY_START_STATUS_e eStatus;  /*!< SUCCESS, FAILURE */
} PHY_StartConf_t;

/*!
 * Stop.Confirm
 */
typedef struct
{
   PHY_STOP_STATUS_e eStatus;  /*!< SUCCESS, FAILURE */
} PHY_StopConf_t;

/*!
 * Reset.Confirm
 */
typedef struct
{
   PHY_RESET_STATUS_e eStatus;  /*!< SUCCESS, FAILURE */
} PHY_ResetConf_t;

/*!
 * Data.Confirm
 */
typedef struct
{
   PHY_DATA_STATUS_e eStatus;  /*!< Status of the transmission, 0 - success */
   uint32_t          delay;    /*!< The amount of time in milliseconds the upper layers should wait before attempting to access the DATA service again.
                                    This implies the DATA service is likely to be unavailable until the delay time has expired. */
} PHY_DataConf_t;

/*!
 * CCA.Request
 */
typedef struct
{
   uint16_t channel;         /*!< The channel on which clear channel assessment is performed */
} PHY_CCAReq_t;

/*!
 * Get.Request
 */
typedef struct
{
   PHY_ATTRIBUTES_e eAttribute;  /*!< Specifies the attribute requested */
} PHY_GetReq_t;

/*!
 * Set.Request
 */
typedef struct
{
   PHY_ATTRIBUTES_e  eAttribute; /*!< Specifies the attribute to set */
   PHY_ATTRIBUTES_u  val;        /*!< Specifies the value to set     */
} PHY_SetReq_t;

/*!
 * Start.Request
 */
typedef struct
{
   PHY_START_e state;  /*!< READY, STANDBY */
} PHY_StartReq_t;

/*!
 * Stop.Request
 */
typedef struct
{
   uint8_t dummy; // There are not paramters for this
} PHY_StopReq_t;

/*!
 * Reset.Request
 */
typedef struct
{
   PHY_RESET_e eType;  /*!< ALL, STATISTICS */
} PHY_ResetReq_t;

/*!
 * Data.Request
 */
typedef struct
{
   uint32_t              RxTime;         /*!<  Time associated with when the payload needs to be received (i.e. sync detected) */
   uint16_t              channel;        /*!<  The channel to use */
   uint16_t              payloadLength;  /*!<  The size of the payload in bytes (0-PHY_MAX_PAYLOAD) */
   PHY_MODE_e            mode;           /*!<  The phy mode to use for transmitting */
   PHY_MODE_PARAMETERS_e modeParameters; /*!<  The PHY mode specific parameters to be used when transmitting the frame */
   PHY_FRAMING_e         framing;        /*!<  The framing used: SRFN, STAR Gen I or Gen II. */
   PHY_DETECTION_e       detection;      /*!<  The detection preamble and sync used */
   PHY_TX_CONSTRAIN_e    TxPriority;     /*!(  How the time structure should be interpreted */
   float                *power;          /*!<  The power level setting for the transmitter to use for the output data frame in dBm. */
   uint8_t              *payload;        /*!<  Payload Data */
} PHY_DataReq_t;


/*!
 * Data.Indication
 */
typedef struct
{
   TIMESTAMP_t           timeStamp;       /*!<  UTC time associated with when the payload was received */
   uint32_t              timeStampCYCCNT; /*!<  CYCCNT time associated with when the payload was received */
   uint32_t              sync_time;       /*!<  This is here to be used to measure processing delays */
   float32_t             rssi;            /*!<  RSSI value for this frame */
   float32_t             danl;            /*!<  DANL value for this frame */
   uint16_t              payloadLength;   /*!<  The size of the payload in bytes */
   uint16_t              channel;         /*!<  the channel the data was received on */
   int16_t               frequencyOffset; /*!<  Frequency offset as reported by the radio */
   uint8_t               receiverIndex;   /*!(  Zero based index of the receiver on chich the reception was made */
   PHY_MODE_e            mode;            /*!<  The PHY mode of operation used when transmitting the payload */
   PHY_MODE_PARAMETERS_e modeParameters;  /*!<  The PHY mode specific parameters used when transmitting the frame */
   PHY_FRAMING_e         framing;         /*!<  The framing used: SRFN, STAR Gen I or Gen II. */
   uint8_t              *payload;         /*!<  Payload Data */
} PHY_DataInd_t;

/*!
 * Noise.Indication
 */
typedef struct
{
   uint16_t   period;
   uint32_t   startTime;
   float      receivePowerThreshold[];

} PHY_NoiseInd_t;

/*!
 * This specifies the confirmation interface from the PHY.
 */
typedef struct
{
   PHY_CONF_TYPE_t MsgType;
   union
   {
      // Management Services
      PHY_CCAConf_t      CcaConf;    /*!< CCA   Confirmation to MAC   */
      PHY_GetConf_t      GetConf;    /*!< Get   Confirmation to MAC   */
      PHY_SetConf_t      SetConf;    /*!< Set   Confirmation to MAC   */
      PHY_StartConf_t    StartConf;  /*!< Start Confirmation to MAC   */
      PHY_StopConf_t     StopConf;   /*!< Stop  Confirmation to MAC   */
      PHY_ResetConf_t    ResetConf;  /*!< Reset Confirmation to MAC   */

      // Data Services
      PHY_DataConf_t     DataConf;   /*!< Data  Confirmation to MAC   */
   };
} PHY_Confirm_t;

/*!
 * This specifies the indication interface from the PHY.
 */
typedef struct
{
   PHY_IND_TYPE_t        Type;
   union
   {
      PHY_DataInd_t      DataInd;    /*!< Data Indication from the PHY*/
   };
} PHY_Indication_t;

/*!
 * This struct specifies the request interface to the PHY.
 */
#ifdef _BUFFER_H_
typedef void (*PHY_ConfirmHandler)(PHY_Confirm_t const *confirm, buffer_t const *pReqBuf);

typedef struct
{
   PHY_REQ_TYPE_t     MsgType;
   PHY_ConfirmHandler pConfirmHandler;
   PHY_Confirm_t     *pConfirm;
   union
   {
      // Management Services
      PHY_CCAReq_t     CcaReq;      /*!< CCA Request to PHY */
      PHY_GetReq_t     GetReq;      /*!< Get Request to PHY */
      PHY_SetReq_t     SetReq;      /*!< Set Request to PHY */
      PHY_StartReq_t   StartReq;    /*!< Request to Start the PHY */
      PHY_StopReq_t    StopReq;     /*!< Request to Stop  the PHY */
      PHY_ResetReq_t   ResetReq;    /*!< Request to Reset the PHY */

      // Data Services
      PHY_DataReq_t    DataReq;     /*!< Request to Send data frame to PHY*/
   };
} PHY_Request_t;
#endif
/* EXTERN FUNCTION PROTOTYPES */

#endif /* PHY_Protocol_H */




