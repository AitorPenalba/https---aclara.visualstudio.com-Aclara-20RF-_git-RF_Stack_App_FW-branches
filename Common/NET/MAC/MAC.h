/******************************************************************************
 *
 * Filename: MAC.h
 *
 * Global Designator: MAC_
 *
 * Contents:
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2014-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/
#ifndef MAC_H
#define MAC_H

/* INCLUDE FILES */
#include "PHY_Protocol.h"
#include "MAC_Protocol.h"
#include "stack.h"
#include "HEEP_util.h"

/* #DEFINE DEFINITIONS */

#define MAC_MAX_DEVICES        3   // Number of devices that are tracked
#define MAC_RSSI_HISTORY_SIZE  4   // Frame RSSI RSSI History

/* see MAC spec document for details */
#define MAC_MAX_PAYLOAD_SIZE ((uint16_t)1280)
#define MAC_ACK_WINDOW ((uint8_t)8)
#define MAC_ACK_MODULUS ((uint8_t)((MAC_ACK_WINDOW)*2))
#define MAC_CSMA_MIN_BACKOFF_TIME_DEFAULT          30       // Default Minimum Backoff Time (msec)
#define MAC_CSMA_MIN_BACKOFF_TIME_LIMIT           100       // Maximum Value of the Min Backoff Time (msec)
#define MAC_CSMA_MAX_BACKOFF_TIME_DEFAULT         100       // Default Maximum Backoff Time (msec)
#define MAC_CSMA_MAX_BACKOFF_TIME_LIMIT           500       // Maximum Value of the Maximum Backoff Time (msec)


/* MACRO DEFINITIONS */

#define PHY_DBM_TO_MAC_SCALING(phy_dbm)    ((uint16_t) ( ( ( (double) ( phy_dbm ) + 200.0) * 16.0) + 0.5 ))
                                           // This converts the PHY RSSI dBm representation to the MAC representation.
                                           // The PHY value -200dBm is mapped to MAC 0 and the +55dBm PHY value is mapped to MAC 4080
#define MAC_RPT_BASE                      (-130.00f) /* dbm */
#define MAC_SCALE_RPT(rpt_val)            ( float ) ( ( ( rpt_val - MAC_RPT_BASE  ) * 0.0156f ) * 255 + 0 )
                                          /* Scale the Tx Power according to MAC-Link Parameter requirements */
                                          /* Notes:   1. Formula: ((Input - InputLow) / (InputHigh - InputLow))* (OutputHigh - OutputLow) + OutputLow;
                                                      2. Input Range: -130 to -66
                                                      3. Should use the roundf() whenever this macro is called */

/* TYPE DEFINITIONS */
typedef struct
{
   uint16_t TimerId;
   bool     InQueue;
   bool     InBlockingWindow;
   bool     TimePushPending;
   bool     InTimeDiversity;
}TimeSync_t;

typedef struct
{
   uint32_t    FirstHeardTime;    // Time when unit was added
   uint32_t    LastHeardTime;     // Time of the most recent update
   uint32_t    RxCount;
   int16_t     rssi_dbm[MAC_RSSI_HISTORY_SIZE];
#if NOISE_TEST_ENABLED == 1
   int16_t     noise_dbm[MAC_RSSI_HISTORY_SIZE];
#endif
   bool        inUse;
   uint8_t     xid[MAC_ADDRESS_SIZE];
}  mac_stats_t;

/* This structure contains the MAC attributes that changed very frequently */
typedef struct
{
   TIMESTAMP_t LastResetTime;            /*!< Last reset timestamp. */
   uint32_t AcceptedFrameCount;          /*!< The number of valid received frames that are accepted by the MAC as new unique frames.*/
   uint32_t ArqOverflowCount;            /*!< The number of received frames where the SN was outside of the allowable receive window. */
   uint32_t ChannelAccessFailureCount;   /*!< The number of times the MAC failed to access the channel and aborted the packet. */
   uint32_t DuplicateFrameCount;         /*!< The number of valid received frames that were determined to be duplicate frames. */
   uint32_t FailedFcsCount;              /*!< The number of received frames that failed the frame check sequence validation. */
   uint32_t InvalidRecipientCount;       /*!< */
   uint32_t MalformedAckCount;           /*!< The number of received frames (ACK frame or piggyback) where the acknowledgement */
   uint32_t MalformedFrameCount;         /*!< The number of received frames that failed to be parsed due to a malformed MAC header. */
   uint32_t PacketConsumedCount;         /*!< The number of packets that were received and consumed by the MAC layer. */
   uint32_t PacketFailedCount;           /*!< The number of packets that failed to be reassembled.  */
   uint32_t PacketReceivedCount;         /*!< The number of packets that were sent to the upper layers.  */
   uint32_t AdjacentNwkFrameCount;       /*!< The number of received frames that were sent from a device whose network ID did ...  */
   uint32_t RxFrameCount;                /*!< The number of frames received from the PHY layer. */
   uint32_t RxOverflowCount;             /*!< The number of times that a frame was received but was unable to be
                                              processed due to insufficient receiver memory allocation.*/
   uint32_t TransactionOverflowCount;    /*!< The number of transaction requests rejected because the transaction queue was full. */
   uint32_t TxFrameCount;                /*!< The number of frames transmitted by the PHY layer. */
   uint32_t TxPacketCount;               /*!< The number of packets transmitted by the MAC layer. */
   uint32_t TxPacketFailedCount;         /*!< The number of packets failed packets.    */
#if ( EP == 1 )
   mac_stats_t stats[MAC_MAX_DEVICES];
#endif
   uint16_t PingCount;                   /*!< The Number of Ping Request Commands Received by the MAC Layer */
   uint8_t  PacketId;                    /*!< The ID of the current in-process packet. */
   uint32_t TransactionTimeoutCount;     /*!( The number of times the MAC had to abort a transaction for taking too long. */
   uint32_t TxLinkDelayCount;            /*!( The number of times the MAC had to delay a frame due to the PHY indicating a delay was necessary. */
   uint32_t TxLinkDelayTime;             /*!( The total amount of time in milliseconds that the MAC has delayed due to the PHY indicating a delay was necessary. */
}MacCachedAttr_t;

/* This structure defines the attributes that must be saved/restored for last gasp
 * This currently requires 25 bytes
 */
typedef struct mac_config_s
{
      float    CsmaPValue;
      int16_t  CcaThreshold;
      uint16_t CsmaMaxBackOffTime;
      uint16_t CsmaMinBackOffTime;
      uint16_t TxChannel;                      // The Channel (i.e Frequency) used for Inbound
      uint8_t  NetworkId;
      uint8_t  CsmaMaxAttempts;
      uint8_t  PacketId;
#if ( EP == 1 )
      bool     CsmaQuickAbort;   //There will be a pad byte between this and CcaThreshold
#endif
}mac_config_t;

typedef void (*MAC_IndicationHandler)(buffer_t *indication);

typedef enum
{
   TIME_SLOT_INVALID,
   TIME_SLOT_1,
   TIME_SLOT_2,
   TIME_SLOT_3
} TimeSlots_e;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
#ifdef PHY_Protocol_H
//extern mac_defaults_t mac_defaults;	/* default parameters to used during testing */
#endif

/* FUNCTION PROTOTYPES */
#ifdef ERROR_CODES_H_
returnStatus_t MAC_init ( void );
#endif
void MAC_Task ( taskParameter );

void MAC_Request(buffer_t *request);
void MAC_RegisterIndicationHandler(MAC_IndicationHandler pCallback);
void MAC_RegisterCommStatusIndicationHandler(MAC_IndicationHandler pCallback);

void MAC_ProcessRxPacket( buffer_t *mac_buffer, uint8_t frame_type );

void MAC_PauseTx( bool pause );

uint8_t MAC_ReliabilityRetryCount_Get ( MAC_Reliability_e reliability );

uint8_t MAC_NextPacketId(void);

uint8_t MAC_NextPacketId_Get(void);
void    MAC_NextPacketId_Set(uint8_t NextPacketId);

uint16_t MAC_getMacPingCount(void);
void    MAC_NetworkId_Set(uint8_t NetworkId);
uint8_t MAC_NetworkId_Get(void); // Needs to be called by other than MAC task
uint8_t NetworkId_Get(void); // Needs to be called by MAC task only

void MAC_MacAddress_Get(uint8_t       macAddress[MAC_ADDRESS_SIZE]);
void MAC_MacAddress_Set(uint8_t const macAddress[MAC_ADDRESS_SIZE]);
#if 0 /* TODO: Queue handle for FreeRTOS */
returnStatus_t MAC_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif
/* this function goes away once we have a MAC address in the security chip */
/*lint -esym(578,macAddress)  */
// This just gets the 40-bit extension address

// Extended Unique Identifier - Address (64-bits)
void MAC_EUI_Address_Get( eui_addr_t       *eui_addr );
void MAC_EUI_Address_Set( eui_addr_t const *eui_addr );

// Organizational Unique Identifier - Address (24-bits)
void MAC_OUI_Address_Get( oui_addr_t       *oui_addr );
void MAC_OUI_Address_Set( oui_addr_t const *oui_addr );

// Extension Identifier - Address (40-bits)
void MAC_XID_Address_Get( xid_addr_t       *xid_addr );
void MAC_XID_Address_Set( xid_addr_t const *xid_addr );

void MAC_SaveConfig_To_BkupRAM(void);
void MAC_RestoreConfig_From_BkupRAM(void);
OS_MSGQ_Obj *MAC_GetMsgQueue(void);
void MAC_TimeSyncParameters_Print(void);
bool MAC_TimeQueryReq(void);
void MAC_Stats(void);
#if BOB_PADDED_PING == 0
uint32_t MAC_PingRequest( uint16_t handle, eui_addr_t const dst_address, uint8_t counterReset, uint8_t rspMode );
#else
uint32_t MAC_PingRequest( uint16_t handle, eui_addr_t const dst_address, uint8_t counterReset, uint8_t rspMode, uint16_t extra_DL_bytes, uint16_t extra_UL_bytes );
#endif

#ifdef MAC_PROTOCOL_H
uint16_t MAC_RandomChannel_Get(MAC_CHANNEL_SETS_e channelSets, MAC_CHANNEL_SET_INDEX_e channelSetIndex);
bool     TxChannels_init(MAC_CHANNEL_SETS_e channelSets, MAC_CHANNEL_SET_INDEX_e channelSetIndex);
#endif
#if 0 /* TODO: Queue handle for FreeRTOS */
returnStatus_t MAC_TimeSourceHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif


typedef enum MAC_COUNTER_e
{
   eMAC_AcceptedFrameCount,
   eMAC_AdjacentNwkFrameCount,
   eMAC_ArqOverflowCount,
   eMAC_ChannelAccessFailureCount,
   eMAC_DuplicateFrameCount,
   eMAC_FailedFcsCount,
   eMAC_InvalidRecipientCount,
   eMAC_MalformedAckCount,
   eMAC_MalformedFrameCount,
   eMAC_PacketConsumedCount,
   eMAC_PacketFailedCount,
   eMAC_PacketReceivedCount,
   eMAC_PingCount,
   eMAC_RxFrameCount,
   eMAC_RxOverflowCount,
   eMAC_TransactionOverflowCount,
   eMAC_TransactionTimeoutCount,
   eMAC_TxFrameCount,
   eMAC_TxLinkDelayCount,
   eMAC_TxPacketCount,
   eMAC_TxPacketFailedCount,
}MAC_COUNTER_e;

void MAC_CounterInc(MAC_COUNTER_e eCounter);

void     MAC_PingCounter_Reset(void);
uint16_t MAC_PingCounter_Inc(void);


char* MAC_StartStatusMsg(MAC_START_STATUS_e status);
char* MAC_StopStatusMsg( MAC_STOP_STATUS_e  status);
char* MAC_ResetStatusMsg(MAC_RESET_STATUS_e status);
char* MAC_GetStatusMsg(  MAC_GET_STATUS_e   status);
char* MAC_SetStatusMsg(  MAC_SET_STATUS_e   status);
char* MAC_TimeQueryStatusMsg( MAC_TIME_QUERY_STATUS_e  status);
char* MAC_FlushStatusMsg(MAC_FLUSH_STATUS_e status);
char* MAC_PurgeStatusMsg(MAC_PURGE_STATUS_e status);
char* MAC_DataStatusMsg( MAC_DATA_STATUS_e  status);
char* MAC_PingStatusMsg( MAC_PING_STATUS_e  status);

uint32_t      MAC_StartRequest(                            void (*confirm_cb)(MAC_Confirm_t const *pConfirm));
uint32_t      MAC_StopRequest(                             void (*confirm_cb)(MAC_Confirm_t const *pConfirm));
MAC_GetConf_t MAC_GetRequest( MAC_ATTRIBUTES_e eAttribute);
MAC_SetConf_t MAC_SetRequest( MAC_ATTRIBUTES_e eAttribute, void const *val);
uint32_t      MAC_ResetRequest(MAC_RESET_e eType,          void (*confirm_cb)(MAC_Confirm_t const *pConfirm));
uint32_t      MAC_FlushRequest(                            void (*confirm_cb)(MAC_Confirm_t const *pConfirm));

uint32_t MAC_DataRequest_Srfn(
   NWK_Address_t const *dst_address,
   uint8_t       const *payload,                    /* Pointer to the payload data */
   uint16_t             payloadLength,              /* The size of the payload in bits */
   bool                 ackRequired,
   uint8_t              priority,
   bool                 droppable,
   MAC_Reliability_e    reliability,
   MAC_CHANNEL_SET_INDEX_e channelSetIndex,
   MAC_dataConfCallback dataConfirm_cb,
   void (*confirm_cb)(MAC_Confirm_t const *pConfirm) );

uint32_t MAC_DataRequest_Star(
   uint8_t       const *payload,                    /* Pointer to the payload data */
   uint16_t             payloadLength,              /* The size of the payload in bits */
   bool                 fastMessage,
   bool                 skip_cca);

uint32_t                MAC_PurgeRequest(uint16_t handle, void (*confirm_cb)(MAC_Confirm_t const *pConfirm));
uint32_t                MAC_TimeQueryRequest(void (*confirm_cb)(MAC_Confirm_t const *pConfirm));
MAC_TIME_PUSH_STATUS_e  MAC_TimePush_Request ( uint8_t broadCastMode, uint8_t *srcAddr, MAC_ConfirmHandler confirm_cb , uint32_t time_diversity );
void                    MAC_SendConfirm(MAC_Confirm_t const *pConf, buffer_t const *pReqBuf);

mac_stats_t            *MAC_STATS_GetRecord(uint32_t index);

void                    MAC_CsmaParameters_Print(void);
void                    MAC_ConfigPrint(void);

#if ( DCU == 1 )
void                    MAC_TxPacketDelay(void);
bool                    MAC_STAR_DataIndication(const PHY_DataInd_t *phy_indication );
#endif

#if ( TX_THROTTLE_CONTROL == 1 )
void                    MAC_SetTxThrottle(uint32_t throttle);
uint32_t                MAC_GetTxThrottle(void);
void                    MAC_TxThrottleTimerManage ( uint8_t hours );
uint16_t                MAC_GetTxThrottleTimer ( void );
#endif

#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
void    MAC_getNextMacAddr( void );
uint8_t MAC_getMacOffset( void );
#endif

float32_t MAC_GetLastMsgRSSI( void );

#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
bool MAC_IsTxPaused( void );
#endif

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
void           MAC_LinkParameters_Config_Print( void );
void           MAC_LinkParameters_EventDriven_Push( void );
returnStatus_t MAC_LinkParametersPush_Request ( uint8_t broadCastMode, uint8_t *srcAddr, MAC_ConfirmHandler confirm_cb );
returnStatus_t LinkParameters_RxPower_Threshold_Get( uint8_t nChannels, uint8_t *RxPowerThreshold, uint16_t const *RxChannels, MAC_CHANNEL_SETS_e channelSets,
                                                      MAC_CHANNEL_SET_INDEX_e channelSetIndex, bool useOtherTBmax );
void           MAC_Set_otherTBpowerThreshold( float maxRPT );
float          MAC_Get_maxLocalThreshold( void );
#endif

#endif /* this must be the last line of the file */
