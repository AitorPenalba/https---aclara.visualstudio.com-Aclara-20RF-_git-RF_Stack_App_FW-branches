/***********************************************************************************************************************
 *
 * Filename: PHY.h
 *
 * Contents:
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
#ifndef PHY_H
#define PHY_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "Phy_Protocol.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define PHY_SYNC_HISTORY_SIZE 10   // Frame RSSI RSSI History
#define PRINT_RSSI_STATS_DBG_MESSAGES 0   // Prints DBG info for Background RSSI Statistics
                                          //    0: Off
                                          //    1: Only delay warnings, and < -125dB warnings
                                          //    2: Warnings, and RSSI replacements from message recieves
                                          //    3: Warnings, Replacements, and prints RSSI values recorded every 6 seconds
#if (PRINT_RSSI_STATS_DBG_MESSAGES != 0)
#warning "PRINT_RSSI_STATS_DBG_MESSAGES debug messages are enabled"
#endif

#if ( DCU == 1 )
#if ( VSWR_MEASUREMENT == 1 )
#define PHY_TX_OUTPUT_POWER_MIN                28.0f     // The minimum acceptable power level in dBm for the transmitter at the device output
#define PHY_TX_OUTPUT_POWER_MAX                36.0f     // The maximum acceptable power level in dBm for the transmitter at the device output
#define PHY_TX_OUTPUT_POWER_DEFAULT            28.0f     // Default power level in dBm for the transmitter at the device output
#else
#define PHY_TX_OUTPUT_POWER_MIN                0.0f      // The minimum acceptable power level in dBm for the transmitter at the device output
#define PHY_TX_OUTPUT_POWER_MAX                32.8f     // The maximum acceptable power level in dBm for the transmitter at the device output
#define PHY_TX_OUTPUT_POWER_DEFAULT            32.8f     // Default power level in dBm for the transmitter at the device output
#endif
#else
#define PHY_TX_OUTPUT_POWER_MIN                0.0f      // The minimum acceptable power level in dBm for the transmitter at the device output
#define PHY_TX_OUTPUT_POWER_MAX                32.8f     // The maximum acceptable power level in dBm for the transmitter at the device output
#endif

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
#define PHY_HARD_DEMODULATOR_SINR_DEFAULT             ( uint8_t )12     /* ( in dB ) Used in RPT calculation ( MAC-Link Parameters ) */
#define PHY_SOFT_DEMODULATOR_SINR_DEFAULT             ( uint8_t )9      /* ( in dB ) Used in RPT calculation ( MAC-Link Parameters ) */
#define PHY_BACKGROUND_RSSI_AVG_DEFAULT               ( int8_t )-119    /* ( in dB ) Used in RPT calculation ( MAC-Link Parameters ) */
#define PHY_BACKGROUND_RSSI_STD_DEVIATION_DEFAULT     ( uint8_t )3      /* ( in dB ) Used in RPT calculation ( MAC-Link Parameters ) */
#define RECEIVE_POWER_MARGIN_DEFAULT                  ( uint8_t )3      /* in dB */
#endif
#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
#define FEM_MIN_POWER_DBM                             28.0f
#define FEM_MAX_POWER_DBM                             36.0f
/* Various manufacturers of FEM hardware  */
#define MINI_CIRCUITS                                 0
#define MICRO_ANT                                     1
#endif
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
#ifdef _BUFFER_H_
typedef void (*PHY_IndicationHandler)(buffer_t *indication);
#endif

typedef enum PHY_COUNTER_e
{
   ePHY_DataRequestCount        ,
   ePHY_DataRequestRejectedCount,
   ePHY_FailedFrameDecodeCount  ,
   ePHY_FailedHcsCount          ,
   ePHY_FramesReceivedCount     ,
   ePHY_FramesTransmittedCount  ,
   ePHY_FailedHeaderDecodeCount ,
   ePHY_FailedTransmitCount     ,
   ePHY_MalformedHeaderCount    ,
   ePHY_PreambleDetectCount     ,
   ePHY_RadioWatchdogCount      ,
   ePHY_RssiJumpCount           ,
   ePHY_RxAbortCount            ,
   ePHY_SyncDetectCount         ,
   ePHY_TxAbortCount            ,
   ePHY_ThermalProtectionCount
}PHY_COUNTER_e;

typedef struct
{
   int16_t    Rssi[PHY_SYNC_HISTORY_SIZE];  // Array of Rssi values for syncs
   uint8_t    Index;                        // Index for saving the next value
}PHY_SyncHistory_t;

/* PHY Configurable attributes */
// Data to keep in cache memory
// WARNING: If a variable is added or removed, Process_ResetReq might need to be updated
typedef struct
{  /* PHY Communication counters */
   TIMESTAMP_t   LastResetTime;                                /*!< Last reset timestamp. */
   int16_t       NoiseEstimate[PHY_MAX_CHANNEL];               /*!< Noise floor value for each channel */
   uint32_t      FailedFrameDecodeCount[PHY_RCVR_COUNT];       /*!< The number of received PHY frames that failed FEC decoding (i.e. no valid code word was found).  */
   uint32_t      FailedHcsCount[PHY_RCVR_COUNT];               /*!< The number of received PHY frame headers that failed the header check sequence validation.  */
   uint32_t      FramesReceivedCount[PHY_RCVR_COUNT];          /*!< The number of frame received */
   uint32_t      FramesTransmittedCount;                       /*!< The number of frames transmitted */
   uint32_t      FailedHeaderDecodeCount[PHY_RCVR_COUNT];      /*!< The number of received PHY frame headers that failed FEC decoding (i.e. no valid code word was found). */
   uint32_t      MalformedHeaderCount[PHY_RCVR_COUNT];         /*!< The number of received frame headers that failed to be parsed due to a malformed PHY header.  */
   uint32_t      PreambleDetectCount[PHY_RCVR_COUNT];          /*!< The number of times a valid preamble is detected. */
   uint32_t      RadioWatchdogCount[PHY_RCVR_COUNT];           /*!< The number of times a each radios timedout.  */
   uint32_t      RssiJumpCount[PHY_RCVR_COUNT];                /*!< The number of times each respective receiver has jumped to a stronger RSSI detection while in process of receiving a frame.  */
   uint32_t      RxAbortCount[PHY_RCVR_COUNT];                 /*!< The number of times each respective receiver has been in process of receiving a frame and had to abort due to a request transmission. */
   uint32_t      SyncDetectCount[PHY_RCVR_COUNT];              /*!< The number of times a valid sync word is detected. */
   uint32_t      TxAbortCount;                                 /*!< The number of times the transmitter has been in process of transmitting a frame and had to abort due to a state change. */
#if EP == 1
   PHY_SyncHistory_t SyncHistory;
   int16_t       NoiseEstimateBoostOn[PHY_MAX_CHANNEL];        /*!< Noise floor value for each channel when the boost capacitor is turned ON */
#if NOISE_TEST_ENABLED == 1
   PHY_SyncHistory_t NoiseHistory;
#endif
#endif
   uint32_t      DataRequestCount;                          /*!< The number of DATA.requests received from the upper layers. */
   uint32_t      DataRequestRejectedCount;                  /*!< The number of DATA.requests received from the upper layers that were rejected. */
   uint32_t      FailedTransmitCount;                       /*!< The number of frame transmission attempts that failed for some internal reason. */
   uint32_t      ThermalProtectionCount;                    /*!< The number of times the thermal protection feature has detected a peripheral temperature exceeding a given limit. */
   bool          ThermalProtectionEngaged;                  /*!< Indicates if a PHY peripheral has exceeded an allowed temperature limit.  If phyThermalProtectionEnable is set to FALSE, this
                                                                 field will also be set to FALSE. */
}PhyCachedAttr_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
#ifdef ERROR_CODES_H_
returnStatus_t PHY_init( void );
returnStatus_t PHY_configRead( void );  // Update local copy of config data
#endif
void PHY_Task ( uint32_t Arg0 );

#ifdef RADIO_H_
void PHY_TestMode_Enable(uint16_t delay, uint16_t UseMarksTxAlgo, uint16_t UseDennisTempCheck, uint16_t UseDynamicBackoff, uint16_t T, uint16_t Period, uint16_t RepeatCnt, uint16_t Delay2,
      uint16_t RepeatCnt2, RADIO_MODE_t mode);
#endif

#ifdef _BUFFER_H_
void PHY_RegisterIndicationHandler(PHY_IndicationHandler pCallback);
#endif

/* Functions to Get PHY Attributes*/
uint16_t  PHY_GetMaxTxPayload(void);
//extern uint8_t  PHY_GetMaxChannel(void);
//extern uint8_t  PHY_GetMaxPayload(void);
//extern uint8_t  PHY_GetCcaThreshold(void);
//extern uint8_t  PHY_GetRcvrCount(void);
//extern uint16_t PHY_GetFailedFrameDecodeCount(void);
//extern uint16_t PHY_GetFailedHcsCount(void);
//extern uint16_t PHY_GetFailedHeaderDecodeCount(void);
//extern uint16_t PHY_GetMalformedHeaderCount(void);

void PHY_Lock(void);
void PHY_Unlock(void);

/* Functions to Set PHY Attributes*/
#ifdef PHY_Protocol_H
bool PHY_Demodulator_Set(uint8_t const *demodulator);
bool PHY_Demodulator_Get(uint8_t *demodulator);
bool PHY_RxChannel_Set(uint8_t index, uint16_t chan);
bool PHY_RxChannel_Get(uint8_t index, uint16_t *channel);
bool PHY_RxDetectionConfig_Set(uint8_t const *detection);
bool PHY_RxDetectionConfig_Get(uint8_t *detection);
bool PHY_RxFramingConfig_Set(uint8_t const *framing);
bool PHY_RxFramingConfig_Get(uint8_t *framing);
bool PHY_RxMode_Set(uint8_t const *mode);
bool PHY_RxMode_Get(uint8_t *mode);
bool PHY_TxChannel_Set(uint8_t index, uint16_t chan);
bool PHY_TxChannel_Get(uint8_t index, uint16_t *channel);

bool PHY_FrontEndGain_Set(int8_t const *gain);
bool PHY_FrontEndGain_Get(int8_t *gain);

void RadioEvent_RssiJump(uint8_t radioNum);
void RadioEvent_SyncDetect(uint8_t radioNum, int16_t rssi_dbm);
void RadioEvent_PreambleDetect(uint8_t radioNum);
void RadioEvent_RadioWatchdog(uint8_t radioNum);
void RadioEvent_RxAbort(uint8_t radioNum);
void RadioEvent_TxAbort(uint8_t radioNum);

uint16_t Frame_Encode(TX_FRAME_t const *txFrame);
#endif

bool     PHY_Channel_Set(uint8_t index, uint16_t channel);
bool     PHY_Channel_Get(uint8_t index, uint16_t *channel);
uint8_t  HeaderLength_Get(PHY_FRAMING_e framing, uint8_t const *data);
bool     HeaderCheckSequence_Validate(uint8_t const *data);
bool     HeaderParityCheck_Validate(uint8_t *pData);
uint8_t  HeaderVersion_Get(uint8_t const *data);
uint16_t FrameLength_Get(PHY_FRAMING_e framing, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters, uint8_t const * const pData);
#ifdef PHY_Protocol_H
PHY_SET_STATUS_e  PHY_Attribute_Set( PHY_SetReq_t const *pSetReq);                        /* Should only be called while operating in the PHY task!   */
PHY_GET_STATUS_e  PHY_Attribute_Get( PHY_GetReq_t const *pGetReq, PHY_ATTRIBUTES_u *val); /* Should only be called while operating in the PHY task!   */
PHY_MODE_e        HeaderMode_Get(uint8_t const *data);
#endif
PHY_MODE_PARAMETERS_e HeaderModeParameters_Get(uint8_t const *data);
void PHY_FailedHeaderDecodeCount_Inc(uint8_t radioNum);
void PHY_FailedHcsCount_Inc(uint8_t radioNum);
void PHY_MalformedHeaderCount_Inc(uint8_t radioNum);

void PHY_InsertTestMsg(uint8_t const *payload, uint16_t length, PHY_FRAMING_e framing, PHY_MODE_e mode);
void PHY_Stats(void);
bool PHY_IsChannelValid(uint16_t channel);
bool PHY_IsRXChannelValid(uint16_t chan);
bool PHY_IsTXChannelValid(uint16_t chan);

char* PHY_CcaStatusMsg(  PHY_CCA_STATUS_e   CCAstatus);
char* PHY_GetStatusMsg(  PHY_GET_STATUS_e   GETstatus);
char* PHY_SetStatusMsg(  PHY_SET_STATUS_e   SETstatus);
char* PHY_StartStatusMsg(PHY_START_STATUS_e STARTstatus);
char* PHY_StopStatusMsg( PHY_STOP_STATUS_e  STOPstatus);
char* PHY_ResetStatusMsg(PHY_RESET_STATUS_e RESETstatus);
char* PHY_DataStatusMsg( PHY_DATA_STATUS_e  DATAstatus);
char* PHY_StartStateMsg( PHY_START_e        STARTstate);

bool PHY_StartRequest(PHY_START_e state, PHY_ConfirmHandler cb_confirm);
bool PHY_StopRequest(PHY_ConfirmHandler cb_confirm);
bool PHY_CcaRequest(uint16_t chan, PHY_ConfirmHandler cb_confirm);
bool PHY_ResetRequest(PHY_RESET_e eType, PHY_ConfirmHandler cb_confirm);
bool PHY_DataRequest(uint8_t const *data, uint8_t size, uint16_t chan, float *power, PHY_DETECTION_e detection, PHY_FRAMING_e framing, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modePerameters,
      PHY_ConfirmHandler cb_confirm, uint32_t RxTime, PHY_TX_CONSTRAIN_e priority);
PHY_GetConf_t PHY_GetRequest( PHY_ATTRIBUTES_e eAttribute );
PHY_SetConf_t PHY_SetRequest( PHY_ATTRIBUTES_e eAttribute, void const *val);

float PHY_VirtualTemp_Get( void );
void PHY_CounterInc(PHY_COUNTER_e ePhyCounter, uint32_t index);

void PHY_SyncHistory(PHY_SyncHistory_t *pSyncHistory);
void PHY_RssiHistory_Print(void);
void PHY_ConfigPrint(void);
bool FrameParityCheck_Validate(uint8_t *pData, uint8_t num_bytes, PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters);

#if ( DCU == 1 )
void PHY_ReplaceLastRssiStat(uint8_t radioNum);
PHY_SET_STATUS_e PHY_friend_SetCachedValue( PHY_SetReq_t const *pSetReq ); /* "Friend" interface to allow setting READ ONLY cached variables.   */
#endif

void PHY_AfcAdjustment_Set_By_Radio( uint8_t radioNum, int16_t adjustment );

#if ( (DCU == 1) && (VSWR_MEASUREMENT == 1) )
void  phyVSWRtimerStart( void );
#endif

#endif /* PHY_H */
