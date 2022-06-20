/**********************************************************************************************************************

   Filename: MAC.c

   Global Designator: MAC_

   Contents:

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2014-2022 Aclara. All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 ********************************************************************************************************************* */

/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>
#if ( DCU == 1 )
#include <stdio.h>
#endif
#include <math.h>
#include <string.h>
#include "DBG_SerialDebug.h"
#include "buffer.h"
#include "MAC_Protocol.h"
#include "file_io.h"
#include "crc32.h"
#include "rand.h"
#include "PHY_Protocol.h"
#include "MAC_FrameEncoder.h"    // MAC_Codec.h
#include "STACK.h"
#if ( TEST_TDMA == 1 )
#include "sys_clock.h"
#endif
#if ( DCU == 1 )
#include "trace_route.h"
#include "mainbd_handler.h"
#include "star.h"
#include "version.h"
#include "APP_MSG_Handler.h" //DCU2+
#endif
#include "MAC.h"
#include "PHY.h"
#include "indication.h"
#include "pwr_task.h"
#if ( EP == 1 )
#include "pwr_last_gasp.h" // DCU doesn't support last gasp
#include "dvr_intFlash_cfg.h"
#endif
#include "MAC_PacketManagement.h"
#include "MAC_FrameManagement.h"
#include "pack.h"
#include "time_util.h"
#include "timer_util.h"
#include "time_sync.h"
#include "time_sys.h"  // TODO: RA6E1: Do we need this include?
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
#include "hmc_display.h"
#endif
#if ( PHASE_DETECTION == 1 )
#include "PhaseDetect.h"
#endif
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
#include "radio_hal.h"
#include "float.h"
#endif


/* #DEFINE DEFINITIONS */
#define MAC_ACK_WAIT_DURATION_DEFAULT              60       // Default AckWaitDuration
#define MAC_ACK_DELAY_DURATION_DEFAULT             10       // Default AckDelayDuration
#define MAC_CSMA_MAX_ATTEMPTS_DEFAULT              10
#define MAC_CSMA_MAX_ATTEMPTS_LIMIT                20

#define MAC_CSMA_PVALUE_DEFAULT                    0.5F
#define MAC_CSMA_QUICK_ABORT_DEFAULT               false
#define MAC_PACKET_TIMEOUT_DEFAULT                 2000
#define MAC_REASSEMBLY_TIMEOUT_DEFAULT             60

#define MAC_RELIABILITY_HIGH_RETRY_COUNT_DEFAULT       2    // High   Reliability Retry Count
#define MAC_RELIABILITY_MEDIUM_RETRY_COUNT_DEFAULT     1    // Medium Reliability Retry Count
#define MAC_RELIABILITY_LOW_RETRY_COUNT_DEFAULT        0    // Low    Reliability Retry Count
#define MAC_RELIABILITY_RANGE_MAX                      7    // maximum range value for reliablity count parameter

#define MAC_TIME_SET_BLOCKING_DELAY            5000   // 5 seconds

#define MAC_CHANNEL_SETS_0_START               2656   // 466600000Hz
#define MAC_CHANNEL_SETS_0_STOP                2880   // 468000000Hz
#define MAC_CHANNEL_SETS_1_START                960   // 456000000Hz
#define MAC_CHANNEL_SETS_1_STOP                1280   // 458000000Hz

#define PHY_CONFIRM_TIMEOUT_MS                 5000   // Timeout for a PHY Confirmation ( 5.0 seconds )
#define PHY_CCA_CONFIRM_TIMEOUT_MS              500   // Timeout for a PHY CCA  Confirmation ( 0.5 seconds )
/*lint -esym(750,PHY_DATA_CONFIRM_TIMEOUT_MS,MAC_MAX_NETWORK_ID,MAC_LINK_PARAM_START_DEFAULT,LINK_PARAMS_FREQUENCY_ACCURACY_LENGTH,MAC_LINK_PARAM_START_MAX) not referenced */
/*lint -esym(750,MAC_CMD_RESP_MAX_TIME_DIVERSITY_LIMIT,LINK_PARAMS_N_RPTS_VALUE,LINK_PARAMS_RPT_LENGTH,LINK_PARAMS_N_RPTS_LENGTH,MAC_LINK_PARAM_PERIOD_DEFAULT) not referenced */
/*lint -esym(750,LINK_PARAMS_CHANNEL_BAND_LENGTH,LINK_PARAMS_CHANNEL_BAND_VALUE,MAC_LINK_PARAMS_FREQUENCY_ACCURACY_WITHOUT_GPS) not referenced */
/*lint -esym(750,NODE_ROLE_FNG,LINK_PARAMS_CHANNEL_OFFSET_VALUE,LINK_PARAMS_NODE_ROLE_LENGTH,LINK_PARAMS_SCALE_FACTOR,LINK_PARAMS_RESERVED_LENGTH) not referenced */
/*lint -esym(750,LINK_PARAMS_SCALE_TX_POWER,MAC_LINK_PARAM_MAX_OFFSET_DEFAULT,LINK_PARAMS_TX_POWER_LENGTH,MAC_LINK_PARAM_MAX_OFFSET_MAX,LINK_PARAMS_TX_POWER_HEADROOM_LENGTH) not referenced */
/*lint -esym(750,MAC_CMD_RESP_MAX_TIME_DIVERSITY_DEFAULT,LINK_PARAMS_CHANNEL_OFFSET_LENGTH,MAC_LINK_PARAMS_FREQUENCY_ACCURACY_WITH_GPS) not referenced */
#define PHY_DATA_CONFIRM_TIMEOUT_MS            5000   // Timeout for a PHY Data Confirmation ( 5.0 seconds )
#define MAC_MAX_NETWORK_ID                       15
#if BOB_PADDED_PING == 0
#define PING_REQ_CMD_SIZE                        12
#else
#define PING_REQ_CMD_SIZE                        16
#endif

#define MAC_TRANSACTION_TIMEOUT_DEFAULT          60  // Maximum time a transaction can be in progress until canceled

#define MAC_LINK_PARAM_MAX_OFFSET_DEFAULT                (10000)  /* in milliseconds */
#define MAC_LINK_PARAM_MAX_OFFSET_MAX                    (20000)  /* in milliseconds */
#define MAC_LINK_PARAM_START_DEFAULT                     (1765)   /* 00:29:25 */
#define MAC_LINK_PARAM_START_MAX                         (86399)
#define MAC_LINK_PARAM_PERIOD_DEFAULT                    (3600)   /* in seconds */
#define MAC_CMD_RESP_MAX_TIME_DIVERSITY_DEFAULT          (1000)   /* Default Value of MAC Command Response Max Time Diversity (msecs) */
#define MAC_CMD_RESP_MAX_TIME_DIVERSITY_LIMIT            (10000)  /* Maximum Value of MAC Command Response Max Time Diversity (msecs) */
#define MAC_LINK_PARAMS_FREQUENCY_ACCURACY_WITH_GPS      (0)      /* (PPM) when scaled as needed by MAC-Link-Parameters command format */
#define MAC_LINK_PARAMS_FREQUENCY_ACCURACY_WITHOUT_GPS   (4)      /* (PPM) when scaled as needed by MAC-Link-Parameters command format */

#define LINK_PARAMS_NODE_ROLE_LENGTH                     3  /* in Bits*/
#define LINK_PARAMS_RESERVED_LENGTH                      5  /* in Bits*/
#define LINK_PARAMS_TX_POWER_LENGTH                      8  /* in Bits*/
#define LINK_PARAMS_TX_POWER_HEADROOM_LENGTH             LINK_PARAMS_TX_POWER_LENGTH  /* in Bits*/
#define LINK_PARAMS_FREQUENCY_ACCURACY_LENGTH            3  /* in Bits*/
#define LINK_PARAMS_N_RPTS_LENGTH                        5  /* in Bits*/
#define LINK_PARAMS_CHANNEL_BAND_LENGTH                  4  /* in Bits*/
#define LINK_PARAMS_CHANNEL_OFFSET_LENGTH                12 /* in Bits*/
#define LINK_PARAMS_RPT_LENGTH                           8  /* in Bits*/

#define LINK_PARAMS_N_RPTS_VALUE                         1  /* For MVP only 1 RPT is transmitted in the MAC-Link Parameters Message */

#define MAC_RPT_BASE                                     (-130.00f) /* dbm */
#define MAX_RPT                                          ( -66.0f ) /* Max value for RPT scaling */
#define SCALE_LIMIT_MAX_TX_POWER                         (int16_t)64 /* Max value of TxPower used for scaling. NOTE: Shall only be used for scaling */


#define ENABLE_PRNT_MAC_LINK_PARAMS_INFO                 1        /* 1 = Enables the MAC-Link Parameters Print Information
                                                                     Note: Might be wise to turn this off for Production Release. */

/*lint -esym(750,LINK_PARAMS_PRNT_INFO)   local macro not referenced */
#if ENABLE_PRNT_MAC_LINK_PARAMS_INFO
#define LINK_PARAMS_PRNT_INFO( category, fmt, ... )    DBG_log( category, ADD_LF|PRINT_DATE_TIME, fmt, ##__VA_ARGS__)
#else
#define LINK_PARAMS_PRNT_INFO( category, fmt, ... )
#endif

#if ( TX_THROTTLE_CONTROL == 1 )
#define TX_THROTTLE_PRINT_EN     0    /* 0 = Do not print throttle debug info, 1=Print throttle debug info */
#if ( TX_THROTTLE_PRINT_EN )
#define THROTTLE_PRINT(...)     INFO_printf(__VA_ARGS__)
#else
#define THROTTLE_PRINT(...)
#endif

#define TX_THROTTLE_PERIOD           1     /* Throttling period in minutes. */
#define TX_THROTTLE_INTERVALS        6     /* Interval periods to be evaluated */
#define TX_THROTTLE_PERCENT         10
/* Bytes per minute over throttle period: ((Period * 9600bits/sec * 60sec/min / 8bits/byte) * ThrottleRate / 100 persent)
   Rewritten to ensure no integer truncation */
#define TX_THROTTLE_MAX_CAPACITY    ( ( ( TX_THROTTLE_PERCENT * TX_THROTTLE_PERIOD * 9600 * 60 / 8 ) / 100 ) * TX_THROTTLE_INTERVALS )
#define TX_THROTTLE_INIT_CAPACITY   ( TX_THROTTLE_MAX_CAPACITY / 2 )
   /* Interval preset to emmulate useage over the period that would result in TX_THROTTLE_INIT_CAPACITY */
#define TX_THROTTLE_INTV_INIT       ( ( TX_THROTTLE_MAX_CAPACITY - TX_THROTTLE_INIT_CAPACITY ) / ( TX_THROTTLE_INTERVALS - 1 ) )
#endif
#if( RTOS_SELECTION == FREE_RTOS )
#define MAC_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define MAC_NUM_MSGQ_ITEMS 0
#endif

/* TYPE DEFINITIONS */

/* CONSTANTS */
static MAC_IndicationHandler pIndicationHandler_callback = NULL;
static IndicationObject_t commIndication;

static buffer_t resendTimeSyncMsgbuf_ = { 0 }; //buffer used by timer callback

#if BOB_PADDED_PING
static const uint8_t pseudoRandomPad[] =
    { 0x9F,0xBF,0xFC,0xE5,0x0A,0xB9,0x0A,0x89,0x10,0x8A,0x67,0x4E,0xFE,0xAD,0xA3,0x50, \
      0x80,0xB2,0x63,0x01,0x51,0xFA,0x25,0xC0,0xDF,0xFE,0xEE,0x44,0x58,0xE6,0x8A,0x37, \
      0xCC,0x3A,0xF8,0xA9,0x03,0xA3,0x64,0x34,0x79,0x7C,0xB8,0x47,0x64,0x80,0x6C,0xBE, \
      0xF0,0x86,0x43,0x04,0x19,0xD6,0x8B,0x88,0x76,0x8D,0x78,0x80,0xD3,0xC6,0xF7,0x10, \
      0x1C,0x36,0xAF,0xDA,0xD1,0x8E,0x56,0x11,0xAF,0x88,0x5F,0x20,0x7D,0xC1,0xD6,0xF0, \
      0xF2,0xC2,0xF2,0x88,0xC2,0xF9,0x36,0x17,0xBF,0xB7,0x26,0xB2,0x41,0xB5,0x1C,0xE2, \
      0x40,0x6B,0xCA,0xC4,0x41,0x36,0x39,0x4B,0xAF,0xEB,0xF8,0x9C,0x08,0xFD,0xA0,0x74, \
      0x7D,0xBA,0xB8,0x9A,0xAC,0xE4,0x3C,0xEE,0x61,0x2C,0x99,0xEC,0x69,0xCA,0x68,0x0E };
#endif
/* These are used for printing messages */
static const char mac_attr_names[][27] = // Don't forget to update the size using the length of the longest element + 1.
{
   [eMacAttr_AcceptedFrameCount        ] =  "AcceptedFrameCount",
   [eMacAttr_AckDelayDuration          ] =  "AckDelayDuration",
   [eMacAttr_AckWaitDuration           ] =  "AckWaitDuration",
   [eMacAttr_AdjacentNwkFrameCount     ] =  "AdjacentNwkFrameCount",
   [eMacAttr_ArqOverflowCount          ] =  "ArqOverflowCount",
   [eMacAttr_ChannelAccessFailureCount ] =  "ChannelAccessFailureCount",
   [eMacAttr_ChannelSetsSRFN           ] =  "ChannelSetsSRFN",
   [eMacAttr_ChannelSetsSTAR           ] =  "ChannelSetsSTAR",
   [eMacAttr_ChannelSetsCountSRFN      ] =  "ChannelSetsCountSRFN",
   [eMacAttr_ChannelSetsCountSTAR      ] =  "ChannelSetsCountSTAR",
   [eMacAttr_CsmaMaxAttempts           ] =  "CsmaMaxAttempts",
   [eMacAttr_CsmaMaxBackOffTime        ] =  "CsmaMaxBackOffTime",
   [eMacAttr_CsmaMinBackOffTime        ] =  "CsmaMinBackOffTime",
   [eMacAttr_CsmaPValue                ] =  "CsmaPValue",
   [eMacAttr_CsmaQuickAbort            ] =  "CsmaQuickAbort",
   [eMacAttr_DuplicateFrameCount       ] =  "DuplicateFrameCount",
   [eMacAttr_FailedFcsCount            ] =  "FailedFcsCount",
   [eMacAttr_InvalidRecipientCount     ] =  "InvalidRecipientCount",
   [eMacAttr_LastResetTime             ] =  "LastResetTime",
   [eMacAttr_MalformedAckCount         ] =  "MalformedAckCount",
   [eMacAttr_MalformedFrameCount       ] =  "MalformedFrameCount",
   [eMacAttr_NetworkId                 ] =  "NetworkId",
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
   [eMacAttr_NumMac                    ] =  "NumMac",
#endif
   [eMacAttr_PacketConsumedCount       ] =  "PacketConsumedCount",
   [eMacAttr_PacketFailedCount         ] =  "PacketFailedCount",
   [eMacAttr_PacketId                  ] =  "PacketId",
   [eMacAttr_PacketReceivedCount       ] =  "PacketReceivedCount",
   [eMacAttr_PacketTimeout             ] =  "PacketTimeout",
   [eMacAttr_PingCount                 ] =  "PingCount",
   [eMacAttr_ReassemblyTimeout         ] =  "ReassemblyTimeout",
   [eMacAttr_ReliabilityHighCount      ] =  "ReliabilityHighCount",
   [eMacAttr_ReliabilityLowCount       ] =  "ReliabilityLowCount",
   [eMacAttr_ReliabilityMediumCount    ] =  "ReliabilityMediumCount",
   [eMacAttr_RxFrameCount              ] =  "RxFrameCount",
   [eMacAttr_RxOverflowCount           ] =  "RxOverflowCount",
   [eMacAttr_State                     ] =  "State",
   [eMacAttr_TransactionOverflowCount  ] =  "TransactionOverflowCount",
   [eMacAttr_TransactionTimeout        ] =  "TransactionTimeout",
   [eMacAttr_TransactionTimeoutCount   ] =  "TransactionTimeoutCount",
   [eMacAttr_TxFrameCount              ] =  "TxFrameCount",
   [eMacAttr_TxLinkDelayCount          ] =  "TxLinkDelayCount",
   [eMacAttr_TxLinkDelayTime           ] =  "TxLinkDelatTime",
   [eMacAttr_TxPacketCount             ] =  "TxPacketCount",
#if DCU == 1
   [eMacAttr_TxPacketDelay             ] =  "TxPacketDelay",
#endif
   [eMacAttr_TxPacketFailedCount       ] =  "TxPacketFailedCount",
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   [eMacAttr_LinkParamMaxOffset        ] =  "LinkParamsMaxOffset",
   [eMacAttr_LinkParamPeriod           ] =  "LinkParamsPeriod",
   [eMacAttr_LinkParamStart            ] =  "LinkParamsStart",
#endif // end of ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) ) section
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
   [eMacAttr_CmdRespMaxTimeDiversity   ] =  "CmdRespMaxTimeDiversity",
#endif
   [eMacAttr_IsChannelAccessConstrained ] =  "IsChannelAccessConstrained",
   [eMacAttr_IsFNG                      ] =  "IsFNG",
   [eMacAttr_IsIAG                      ] =  "IsIAG",
   [eMacAttr_IsRouter                   ] =  "IsRouter"
};

/* FILE VARIABLE DEFINITIONS */
static MAC_STATE_e  _state = eMAC_STATE_IDLE;
static OS_MSGQ_Obj  MAC_msgQueue;
static OS_MUTEX_Obj MAC_AttributeMutex_;
static OS_MUTEX_Obj MAC_TimeSyncAttributeMutex_; //Serialize access to time-sync related variables
static OS_SEM_Obj   MAC_AttributeSem_;
static OS_MUTEX_Obj Mutex_;
#if ( DCU == 1 )
static float32_t  STAR_last_msg_rssi = 0;   /* RSSI for an last received Packet     */
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
static uint16_t   macCmdResp_TimeDiversity_TimerID_   = INVALID_TIMER_ID;
#endif
#if ( MAC_LINK_PARAMETERS == 1 )
static uint8_t    linkParam_P_AlaramID_               = NO_ALARM_FOUND;    /* Alarm ID to store the ID of alarm used by Link Param Periodic Message */
static bool       send_MacLinkParams_                 = (bool)false;       /* Flag use to see if the MAC-Time Set command is in the Tx Queue because of the MAC-Time Request/Query */
static float      receivePowerThreshold_ [PHY_RCVR_COUNT] = { 0 };
static float      maxLocalThreshold = 0;                                   /* Maximum power threshold sensed by this TB.   */
static float      otherTBpowerThreshold = 0;                               /* Secondary TB receive power threshold   */

static void             LinkParameters_ManageAlarm( void );
static buffer_t*        Link_Parameters_Create( uint8_t addr_mode, uint8_t dst_addr[MAC_ADDRESS_SIZE], MAC_dataConfCallback callback );
static MAC_SET_STATUS_e LinkParameters_MaxOffset_Set( uint16_t  maxOffset );
static MAC_SET_STATUS_e LinkParameters_Period_Set( uint32_t val );
static uint32_t         LinkParameters_Period_Get( void );
static MAC_SET_STATUS_e LinkParameters_Start_Set( uint32_t start );
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
static uint8_t          LinkParameters_FrequencyAccuracy_Get( void );
#endif
static uint8_t          LinkParameters_NodeRole_Get( void );
#endif // end of ( MAC_LINK_PARAMETERS == 1 ) section
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
static MAC_SET_STATUS_e macCommandResponse_MaxTimeDiversity_Set( uint16_t timeoutMs );
static uint32_t         Random_CommandResponseTimeDiversity_Get( void );
static returnStatus_t   macCommandResponse_TimeDiversityTimer_Create( void );
static void             macCommandResponse_TimeDiversityTimer_Start( uint32_t timeDiversityMs );
#endif
static void             MAC_SetLastMsgRSSI( float rssi ) ;
#endif // end of ( DCU == 1 ) section

#if (TX_THROTTLE_CONTROL == 1 )
/* TX_THROTTLE_PERIOD (min) * 60 sec/min * 9600bits/sec / 8 bits/sec = 72000 bytes/min of transmission capacity */
static uint32_t         TxThrottle                                = TX_THROTTLE_INIT_CAPACITY;    /* Remaning bytes to be transmitted. */
static uint32_t         TxMaxThrottle                             = TX_THROTTLE_INIT_CAPACITY;    /* Subsequent period capacity */
static uint32_t         TxThrottleSum                             = TX_THROTTLE_MAX_CAPACITY/2;   /* Assume Half capacity at power up */
static uint16_t         TxThrottleInterval[TX_THROTTLE_INTERVALS];
static uint16_t         TxThrottle_ID                             = INVALID_TIMER_ID;             /* radio throttle disable timeout timer ID */
static bool             TxThrottle_enabled                        = (bool)true;                   /* Unit powers up with throttling enabled. */
static timer_t          TxThrottle_timer;                                  /* TxThrottling timeout Timer configuration */
#endif

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( EP == 1 ) )
static void             Process_LinkParametersCommand( MAC_DataInd_t const *pDataInd );
#endif

static bool MAC_txPaused = (bool)false;
static void GenerateFailedConfirm( PHY_CONF_TYPE_t confType );

static macAddr_t MacAddress_;                /*! Local copy of the 64-bit Extended Unique Identifier (EUI-64) */
#if ( MULTIPLE_MAC != 0 )
static xid_addr_t LastMacAddressMatch_;      /*! Last macaddr that matched this device's mac address (in range)   */
static int8_t     lastMacOffset_ = 0;        /*! Last modifier to base mac address for outgoing messages.         */
#endif

static bool    IsAckPending(void);
static uint8_t NumChannelSetsSRFN_Get(void);
static uint8_t NumChannelSetsSTAR_Get(void);
#if ( TEST_TDMA == 1 )
uint32_t timeSlotDelay = 50; // 50 ms delay
#endif

/* This structure contains the MAC attributes that are configurable and are rarely changed */
typedef struct
{
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
   // This is no longer used, but remains here to keep the file size the same
   uint8_t  NumMac;                       /* Number of mac IDs to emulate  */
   uint8_t  rsvd[ sizeof(macAddr_t) - 1 ];/*! Not used; left in to keep file size from changing.  */
#else
   macAddr_t rsvd;                        /*! Mac address - 64-bit Extended Unique Identifier (EUI-64) */
#endif

   uint8_t  NetworkId;                    /*!< The identifier for the network on which the device is permitted to
                                                communicate.*/
   uint16_t AckWaitDuration;              /*!< The maximum amount of time in milliseconds to wait for an
                                                acknowledgment to arrive following a transmitted data or command frame
                                                where AR was set to TRUE - Read Only */
   uint16_t AckDelayDuration;             /*!< The number of milliseconds to wait after the receipt of a frame with AR
                                               set to TRUE before sending an ACK frame. Read Only */
   CH_SETS_t ChannelSetsSRFN[MAC_MAX_CHANNEL_SETS_SRFN]; /*!< The list of SRFN channel sets that the MAC supports for
                                               transmission of packets. */
   CH_SETS_t ChannelSetsSTAR[MAC_MAX_CHANNEL_SETS_STAR]; /*!< The list of STAR channel sets that the MAC supports for
                                               transmission of packets. */
   uint8_t  CsmaMaxAttempts;              /*!< The maximum number of attempts by the MAC to gain access to the channel
                                               before failing the transaction request. (0-20) */
   uint16_t CsmaMaxBackOffTime;           /*!< The maximum amount of back off time in milliseconds for the CSMA
                                               algorithm. (macCsmaMinBackOffTime to 500 */
   uint16_t CsmaMinBackOffTime;           /*!< The minimum amount of back off time in milliseconds for the CSMA
                                               algorithm (0 - 100) */
   float    CsmaPValue;                   /*!< The probability that the CSMA algorithm will decide to transmit when an
                                               idle channel is found.  (0-1, default 0.1) */
   bool     CsmaQuickAbort;               /*!< Indicates if the CSMA algorithm will abort frame transmission upon the
                                               first p-persistence choice to not transmit. */
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
   uint8_t  CsmaSkip;
#endif

   uint16_t PacketTimeout;                /*!< The number of milliseconds to maintain a given device's packet ID pair
                                               before clearing the information from the MAC.  Range 0-65535, default =
                                               2000*/
   uint8_t  ReassemblyTimeout;            /*!< The amount of time in seconds allowed to elapse between received segments
                                               of a single packet before the MAC will discard the entire packet and
                                               request any associated ARQ windows.*/
   uint8_t  ReliabilityHighCount;         /*!< The number of repeats or retries that are used to satisfy the QoS
                                               reliability level of high for packets that are sent with AR = FALSE or
                                               TRUE respectively. (0-30, default = 2) */
   uint8_t  ReliabilityMediumCount;       /*!< The number of repeats or retries that are used to satisfy the QoS
                                               reliability level of medium for packets that are sent with AR = FALSE or
                                               TRUE respectively. (0-30, default = 1) */
   uint8_t  ReliabilityLowCount;          /*!< The number of repeats or retries that are used to satisfy the QoS
                                               reliability level of low for packets that are sent with AR = FALSE or
                                               TRUE respectively. (0-30, default = 0) */
   uint16_t TransactionTimeout;           /*!< The number of seconds that a transaction is allowed to be in process before being aborted. */
#if ( DCU == 1 )
   uint16_t TxPacketDelay;
#endif
   uint16_t TxChannel;
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
   uint8_t  TxSlot;
#endif
#if ( DCU == 1 )
#if ( MAC_LINK_PARAMETERS == 1 )

   uint16_t LinkParamMaxOffset;          /*!< The maximum amount of random offset in milliseconds from macLinkParameterStart
                                              for which the source will begin transmitting the MAC Link Parameters command */
   uint32_t LinkParamPeriod;             /*!< The period, in seconds, for which the MAC Link Parameters command broadcast
                                              schedule is computed.  A value of zero will prevent the DCU from automatically
                                              transmitting the MAC Link Parameters command.
                                              Values 0, 900, 1800, 3600, 7200, 10800, 14400, 21600, 28800, 43200, 86400
                                              default = 3600  */
   uint32_t LinkParamStart;              /*!< The start time in seconds relative to the DCU system time of midnight UTC for
                                              starting the DCU MAC Link Parameters broadcast schedule.  Range: 0-86399, default = 1765 */
#endif // end of ( MAC_LINK_PARAMETERS == 1 ) section
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
   uint16_t macCmdRespMaxTimeDiversity;  /*!< When the DCU receives a broadcast MAC command from the air interface, it shall set a flag so that
                                              a time diversity can be used to delay putting the response MAC command message in the transmit queue.
                                              This is to avoid the situation where multiple responses from different DCUs may collide if time diversity was not applied.
                                              A uniform random value between 0 and macCommandResponseMaxTimeDiversity shall be used to avoid these collisions.
                                              Time in ms. Range 0 - 10000ms. Default: 1000ms */
#endif
#endif  // end of ( DCU == 1 ) section
   bool  IsChannelAccessConstrained;
   bool  IsFNG;
   bool  IsIAG;
   bool  IsRouter;
}MacConfigAttr_t;

static MacConfigAttr_t MAC_ConfigAttr;
static MAC_Confirm_t ConfirmMsg;
static MAC_Confirm_t * const pMacConf = &ConfirmMsg;

static MacCachedAttr_t  CachedAttr;

typedef struct
{
   FileHandle_t          handle;
   char           const *FileName;         /*!< File Name           */
   ePartitionName const  ePartition;       /*!<                     */
   filenames_t    const  FileId;           /*!< File Id             */
   void*          const  Data;             /*!< Pointer to the data */
   lCnt           const  Size;             /*!< Size of the data    */
   FileAttr_t     const  Attr;             /*!< File Attributes     */
   uint32_t       const  UpdateFreq;       /*!< Update Frequency    */
} file_t;


// MAC Configuration Data File
static file_t ConfigFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "MAC_CONFIG",
   .FileId          = eFN_MAC_CONFIG,               // Configuration Data
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &MAC_ConfigAttr,
   .Size            = sizeof(MAC_ConfigAttr),
   .UpdateFreq      = 0xFFFFFFFF                    // Updated Seldom
}; /*lint !e785 too few initializers   */

// MAC Cached Data File
static file_t CachedFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "MAC_CACHED",
   .FileId          = eFN_MAC_CACHED,               // Cached Data
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &CachedAttr,
   .Size            = sizeof(CachedAttr),
   .UpdateFreq      = 0                            // Updated Often
};/*lint !e785 too few initializers   */

static file_t * const Files[2] =
{
   &ConfigFile,
   &CachedFile
};

TimeSync_t TimeSync;

/* This is a list of the TX Channels */
static struct{
   uint8_t  cnt;                              // Number of TX Channels
   uint16_t channels[PHY_MAX_CHANNEL];        // List of   TX Channels
} tx_channels;

/* FUNCTION PROTOTYPES */
static void             MAC_Reset(MAC_RESET_e type);
static bool             Process_CcaConfirm(     const PHY_CCAConf_t   *pCcaConf  );
static bool             Process_GetConfirm(     const PHY_GetConf_t   *pGetConf  );
static bool             Process_SetConfirm(     const PHY_SetConf_t   *pSetConf  );
static bool             Process_ResetConfirm(   const PHY_ResetConf_t *pResetConf);
static bool             Process_DataConfirm(    const PHY_DataConf_t  *pDataConf );
static void             Process_DataIndication( const PHY_DataInd_t   *pDataInd);
static bool             u32_counter_inc(uint32_t* pCounter, bool bRollover);
static bool             u16_counter_inc(uint16_t* pCounter, bool bRollover);
static uint32_t         RandomBackoffDelay_Get(void);
static bool             P_Persistence_Get(void);
static uint32_t         NextRequestHandle();
static returnStatus_t   TimeSet_BlockingTimer_Create(void);
static void             TimeSet_BlockingTimer_CB(uint8_t cmd, void *pData);
static void             TimeSet_BlockingTimer_Start(void);
static buffer_t         *TimeSet_Create(uint8_t addr_mode, uint8_t dst_addr[MAC_ADDRESS_SIZE], MAC_dataConfCallback callback );

static bool             IsTxAllowed(void);    // Check TX throttle (always enabled even if TX_THROTTLE_CONTROL != 1)
#if (TX_THROTTLE_CONTROL == 1 )
static returnStatus_t   TxThrottle_Init( void );
static void             UpdateTxThrottle( void );
static void             TxThrottleTimerCallBack( uint8_t cmd, const void *pData );
#endif

#if (EP == 1)
static bool             Process_TimeQueryReq(MAC_TimeQueryReq_t const *pTimeReq);
#endif

// Time the latest DATA request was received.
static OS_TICK_Struct DataReqTime;

// Parameters returned from the ping response
typedef struct
{
   uint16_t    Handle;           // Handle from the ping request
   xid_addr_t  Origin_xid;       // 5-byte Extension Identifier.
   uint16_t    ReqRssi;          // Request RSSI value
   uint16_t    ReqPNLevel;       // Request Perceived Noise Level
   uint16_t    ReqChannel;       // Channel from the ping request
   uint16_t    PingCount;        // Ping Counter value
   uint8_t     CounterResetFlag;
   uint8_t     ResponseMode;
   uint8_t     Rsvd;
   TIMESTAMP_t ReqTxTime;       // Time from the MAC Request
   TIMESTAMP_t ReqRxTime;       // Time when the request was received
   TIMESTAMP_t RspTxTime;       // Time when the response was send to the PHY?
#if BOB_PADDED_PING == 1
   uint16_t    ExtraReqBytes;
   uint16_t    ExtraRspBytes;
#endif
}ping_rsp_t;

// Parameters returned from the ping request
typedef struct
{
   uint16_t    Handle;
   xid_addr_t  Origin_xid;        // Originator XID (Requestor?)
   uint8_t     CounterResetFlag;
   uint8_t     ResponseMode;
   uint8_t     Rsvd;
   TIMESTAMP_t ReqTxTime;  // Time Stamp from the original request
   TIMESTAMP_t ReqRxTime;  // Time Stamp for when the response is sent to the phy
#if BOB_PADDED_PING == 1
   uint16_t    ExtraReqBytes;
   uint16_t    ExtraRspBytes;
#endif
} ping_req_t;

#if ( MAC_LINK_PARAMETERS == 1 )

typedef struct
{
   uint16_t    channelBand;    /* ( 4 bits )*/
   uint16_t    channelOffset;  /* ( 12 bits )*/
   uint8_t     RPT;
} rpt_info_t;
typedef struct
{
   uint8_t     NodeRole;               /* ( 3 bits )*/
   uint8_t     reserved;               /* ( 5 bits )*/
   uint8_t     TxPower;                /* ( 8 bits ) The Power at which this packet was transmitted ( dBm )*/
   uint8_t     TxPowerHeadroom;        /* ( 8 bits ). The difference between the device maximum Tx power and the Tx Power field above.  */
   uint8_t     frequencyAccuracy;      /* ( 3 bits ) TBD */
   uint8_t     nRPTsIncluded;          /* ( 5 bits ) Number of RPTs Included ( fixed for MVP ) */
   //rpt_info_t  RPT_info[];
}link_parameters_t;
#endif

static buffer_t *PingRsp_Create(
   MAC_dataConfCallback callback,
   uint8_t              dst_addr_mode,               /* Destination Address Mode */
   uint8_t    const     dst_addr[MAC_ADDRESS_SIZE],  /* Destination Address */
   ping_rsp_t const     *params);

static bool Process_GetReq(   MAC_GetReq_t   const *pReq);
static bool Process_ResetReq( MAC_ResetReq_t const *pReq);
static bool Process_SetReq(   MAC_SetReq_t   const *pReq);
static bool Process_StartReq( MAC_StartReq_t const *pReq);
static bool Process_StopReq(  MAC_StopReq_t  const *pReq);
static bool Process_FlushReq( MAC_FlushReq_t const *pReq);
static bool Process_PurgeReq( MAC_PurgeReq_t const *pReq);

static bool Process_DataReq( buffer_t *pBuf );
static bool Process_PingReq( buffer_t *pBuf );

static void Process_CmdFrame(MAC_DataInd_t const *pDataInd);

static void Process_PingReqFrame(MAC_DataInd_t const *pDataInd);

#if ( DCU == 1 )
static void Process_PingRspFrame(MAC_DataInd_t const *pDataInd);
#endif

#if ( EP == 1 )
static buffer_t *TimeReq_Create(MAC_CHANNEL_SET_INDEX_e chanIdx, MAC_dataConfCallback callback);
static void TimeReq_Done(MAC_DATA_STATUS_e status, uint16_t handle_id);
// Functions for the MTU Outbound Stats
#if NOISE_TEST_ENABLED == 1
static void         stats_update(uint8_t xid[MAC_ADDRESS_SIZE], int16_t rssi_dbm, int16_t danl_dbm);
#else
static void         stats_update(uint8_t const xid[MAC_ADDRESS_SIZE], int16_t rssi);
#endif
static void         stats_dump(void);
static void         stats_print(mac_stats_t const *pStats);
static mac_stats_t *stats_find(uint8_t const xid[MAC_ADDRESS_SIZE]);
static mac_stats_t *stats_add(uint8_t const xid[MAC_ADDRESS_SIZE]);
#if ( MULTIPLE_MAC != 0 )
static bool isMyMac( const xid_addr_t );
#endif
#endif

static uint32_t NextRequestHandle(void);//lint !e752

typedef enum xMAC_STATE_e
{
// eMAC_STATE_PHY_STOPPED,   // The PHY is stopped
// eMAC_STATE_PHY_STARTING,  // A Start.Rrequest has been sent to the PHY
   eMAC_STATE_PHY_READY,     // The PHY is ready
   eMAC_STATE_CCA_PENDING,   // Waiting for the channel to be clear before sending data request
   eMAC_STATE_CCA_DELAY,     // Delaying until next attempt to send cca request
   eMAC_STATE_TRANSMITTING   // A Data Request has been sent to the PHY.
} xMAC_STATE_e;


// This is used to manage the request/response with the PHYPingCount
static struct
{
   xMAC_STATE_e          eState;            // Current State
   bool                  confirmPending;    // Indicates that a request was sent and a confirmation is Pending from the PHY
   PHY_CONF_TYPE_t       pendingConfirmType;// Type of confirm that is pending
   OS_TICK_Struct        confirmTimeout;    // when to give up on arrival of a pending confirm
   MAC_FrameManagBuf_s   *Frame;            // The frame that is currently being processed
   PHY_DETECTION_e       ePhyDetection;      //
   PHY_FRAMING_e         ePhyFraming;        //
   PHY_MODE_e            ePhyMode;           //
   PHY_MODE_PARAMETERS_e ePhyModeParameters; //

   MAC_TxType_e         tx_type;           // Transmit Type STAR/SRFN
   uint16_t             channel;           // Transmit Channel index to use
   uint8_t              gain;              // Gain Value
   uint8_t              size;              // Transmit payload size
   uint8_t              data[PHY_MAX_PAYLOAD]; // Transmit data buffer

   bool                 channelClear;      // indicates if the channel is clear
   OS_TICK_Struct       ccaDelayStart;     // Start Time for CCA Delay, used for testing only
   OS_TICK_Struct       ccaDelayEnd;       // End   Time for CCA Delay
   uint8_t              csmaAttempts;      // Number of csma attempts for this frame
}phy;

static MAC_SET_STATUS_e MAC_Attribute_Set(MAC_SetReq_t const *pSetReq); // Prototype should be only used by MAC.c.
                                                                        // We don't want outsiders to call this function directly.
                                                                        // MAC_SetRequest is used for that.
MAC_GET_STATUS_e MAC_Attribute_Get(MAC_GetReq_t const *pGetReq, MAC_ATTRIBUTES_u *val);  // Prototype should be only used by MAC.c and MAC_FrameManagement.c.
                                                                                         // We don't want outsiders to call this function directly.
                                                                                         // MAC_GetRequest is used for that.

#if ( EP == 1 )
/*!
 * This function is used to restore the MAC Configuration from the VBat Register File
 * Note: Since all of the mac parameters are not required for the last gasp to function, only the critical ones are saved.
 *       The remaining ones should be set to default.
 */
void MAC_RestoreConfig_From_BkupRAM()
{
   mac_config_t            *pMacConfig;
   PartitionData_t const   *pSecurePart;  /* Partition handle for security information */

   /* Get mac address from internal flash (ROM) */
   if (eSUCCESS == PAR_partitionFptr.parOpen(&pSecurePart, ePART_ENCRYPT_KEY, 0))
   {
      ( void )PAR_partitionFptr.parRead( ( uint8_t * )&MacAddress_, ( dSize )offsetof( intFlashNvMap_t, mac ),
                                          sizeof( MacAddress_ ), pSecurePart );
   }

   // Get the handle to the location where the data was saved
   pMacConfig = PWRLG_MacConfigHandle();

   // BAH - This is here until this is being used!!!
   if( pMacConfig )
   {
      // Only the critical values required for the last gasp are restored.
      // The reset will be set to defaults
      MAC_ConfigAttr.NetworkId              = pMacConfig->NetworkId;
      MAC_ConfigAttr.CsmaMaxAttempts        = pMacConfig->CsmaMaxAttempts;
      MAC_ConfigAttr.CsmaMaxBackOffTime     = pMacConfig->CsmaMaxBackOffTime;
      MAC_ConfigAttr.CsmaMinBackOffTime     = pMacConfig->CsmaMinBackOffTime;
      MAC_ConfigAttr.CsmaPValue             = pMacConfig->CsmaPValue;
      MAC_ConfigAttr.CsmaQuickAbort         = pMacConfig->CsmaQuickAbort;
      CachedAttr.PacketId                   = pMacConfig->PacketId;
   }
}

/*!
 * This function is used to save the MAC Configuration to the VBat Register
 * Note: Since all of the mac parameters are not required for the last gasp to function, only the critical ones are saved.
 *       The remaining ones should be set to default.
 */
void MAC_SaveConfig_To_BkupRAM(void)
{
   mac_config_t *pMacConfig;

   // Get the handle to the location where the data will be saved
   pMacConfig = PWRLG_MacConfigHandle();

   // BAH - This is here until this is being used!!!
   if( pMacConfig )
   {
      pMacConfig->NetworkId                         = MAC_ConfigAttr.NetworkId;
      pMacConfig->CsmaMaxAttempts                   = MAC_ConfigAttr.CsmaMaxAttempts;
      pMacConfig->CsmaMaxBackOffTime                = MAC_ConfigAttr.CsmaMaxBackOffTime;
      pMacConfig->CsmaMinBackOffTime                = MAC_ConfigAttr.CsmaMinBackOffTime;
      pMacConfig->CsmaPValue                        = MAC_ConfigAttr.CsmaPValue;
      pMacConfig->CsmaQuickAbort                    = MAC_ConfigAttr.CsmaQuickAbort ;
      pMacConfig->PacketId                          = (CachedAttr.PacketId + 1 ) % 4;
   }
}
#endif  // #if ( EP == 1 )

/*!
 *   This function returns the next packet id.
 */
uint8_t MAC_NextPacketId()
{
   CachedAttr.PacketId  = (CachedAttr.PacketId +1 ) % 4;
   (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);

   return CachedAttr.PacketId;
}

/***********************************************************************************************************************
Function Name: MAC_GetMsgQueue

Purpose: Returns the message queue handler. This is implemented so that time sync does not have to create a new task.
         Time sync will set up a alarm to send time-message to this message queue and MAC will send the time-sync out.

Arguments: buffer_t - pointer to a buffer that contains a request

Returns: none
***********************************************************************************************************************/
OS_MSGQ_Obj *MAC_GetMsgQueue(void)
{
   return(&MAC_msgQueue);
}

/***********************************************************************************************************************
Function Name: MAC_Request

Purpose: Called by upper layer to send an request to STACK, also registered with PHY layer as handler for indications

Arguments: buffer_t - pointer to a buffer that contains a request

Returns: None
***********************************************************************************************************************/
void MAC_Request(buffer_t *request)
{
   OS_MSGQ_Post(&MAC_msgQueue, (void *)request); // Function will not return if it fails
}
#if ( MULTIPLE_MAC != 0 )
#if ( NUM_MACS != 1 )
#warning "Compiling for MULTIPLE MAC addresses!"
#endif
/***********************************************************************************************************************
Function Name: isMyMac

Purpose: Used in test endpoint to accept multiple MAC addresses.
         Use base mac address and check for OUI match. Then check lower 32 bits for match. The check for byte following
         OUI to be between 0 and MAC_ConfigAttr.NumMac - 1.

Arguments: mac - pointer to an xid_addr_t

Returns: true if mac is in range of this device's mac addresses, false otherwise
***********************************************************************************************************************/
static bool isMyMac( const xid_addr_t mac )
{
   bool match = (bool)false;     /* Assume no match   */

   if ( 0 == memcmp( &mac[ 1 ], &MacAddress_.xid[ 1 ], sizeof( xid_addr_t ) - 1 ) ) /* "base" address match?   */
   {
      if ( (int8_t)mac[ 0 ] > -(int8_t)MAC_ConfigAttr.NumMac )
      {
         /* Record last "matching" mac address received. */
         (void)memcpy( LastMacAddressMatch_, mac, sizeof( LastMacAddressMatch_ ) );
         match = (bool)true;
      }
   }
   return match;
}

/***********************************************************************************************************************
Function Name: MAC_getNextMacAddr

Purpose: Used in test endpoint to step through multiple MAC addresses.

Arguments: none

Returns: void

Side effects: Updates MacAddress_;
***********************************************************************************************************************/
void MAC_getNextMacAddr( void )
{
   if ( MAC_ConfigAttr.NumMac > 1 )
   {
      lastMacOffset_ = ( int8_t )( ( lastMacOffset_ - 1 ) % MAC_ConfigAttr.NumMac );
      MacAddress_.xid[ 0 ] = ( uint8_t )lastMacOffset_;
   }
}

/***********************************************************************************************************************
Function Name: MAC_getNextMacAddr

Purpose: Used in test endpoint to step through multiple MAC addresses.

Arguments: none

Returns: void

Side effects: none;
***********************************************************************************************************************/
uint8_t MAC_getMacOffset( void )
{
   return ( uint8_t )lastMacOffset_;
}
#endif

/***********************************************************************************************************************
Function Name: phy_confirm_cb

Purpose: Called by PHY layer to return a confirmation to the MAC

Arguments: buffer_t - pointer to a buffer that contains a confirm

Returns: none
***********************************************************************************************************************/
static void phy_confirm_cb(PHY_Confirm_t const *confirm, buffer_t const *pReqBuf)
{
   (void)pReqBuf;
   // Allocate the buffer
   buffer_t *pBuf = BM_allocStack(sizeof(PHY_Confirm_t));   // Allocate the buffer
   if(pBuf != NULL )
   {
      // Set the type
      pBuf->eSysFmt = eSYSFMT_PHY_CONFIRM;

      PHY_Confirm_t *pConf = (PHY_Confirm_t *)pBuf->data; /*lint !e2445 !e2445 !e740 !e826 */
      (void) memcpy(pConf, confirm, sizeof(PHY_Confirm_t) );
      OS_MSGQ_Post(&MAC_msgQueue, (void *)pBuf); // Function will not return if it fails
   }
}

/***********************************************************************************************************************
Function Name: MAC_Indication

Purpose: Called by PHY layer to return an indication to the MAC

Arguments: buffer_t - pointer to a buffer that contains an indication

Returns: none
***********************************************************************************************************************/
static void MAC_Indication(buffer_t *indication)
{
   OS_MSGQ_Post(&MAC_msgQueue, (void *)indication);
}


/*
 * This function will initialize all of the the "Configuration" attributes to default values
 */
static void ConfigAttr_Init(bool bWrite)
{
   uint32_t i;

   // Reset the attributes to default values
   /* Always initialize to defaults.  If the file exists they will be loaded from NV */
   MAC_ConfigAttr.NetworkId              = DEFAULT_NETWORK_ID;

#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
   MAC_ConfigAttr.NumMac                 = NUM_MACS;                     /* Default Number of mac IDs to emulate  */
#endif
   MAC_ConfigAttr.AckWaitDuration        = MAC_ACK_WAIT_DURATION_DEFAULT;
   MAC_ConfigAttr.AckDelayDuration       = MAC_ACK_DELAY_DURATION_DEFAULT;

   // Default SRFN configuration
   MAC_ConfigAttr.ChannelSetsSRFN[0].start   = MAC_CHANNEL_SETS_0_START;
   MAC_ConfigAttr.ChannelSetsSRFN[0].stop    = MAC_CHANNEL_SETS_0_STOP;
   MAC_ConfigAttr.ChannelSetsSRFN[1].start   = MAC_CHANNEL_SETS_1_START;
   MAC_ConfigAttr.ChannelSetsSRFN[1].stop    = MAC_CHANNEL_SETS_1_STOP;
   for (i=2; i<MAC_MAX_CHANNEL_SETS_SRFN; i++) {
      MAC_ConfigAttr.ChannelSetsSRFN[i].start = PHY_INVALID_CHANNEL;
      MAC_ConfigAttr.ChannelSetsSRFN[i].stop  = PHY_INVALID_CHANNEL;
   }

   // Default STAR configuration
   MAC_ConfigAttr.ChannelSetsSTAR[0].start   = MAC_CHANNEL_SETS_0_START;
   MAC_ConfigAttr.ChannelSetsSTAR[0].stop    = MAC_CHANNEL_SETS_0_STOP;
   MAC_ConfigAttr.ChannelSetsSTAR[1].start   = MAC_CHANNEL_SETS_1_START;
   MAC_ConfigAttr.ChannelSetsSTAR[1].stop    = MAC_CHANNEL_SETS_1_STOP;

   MAC_ConfigAttr.CsmaMaxAttempts            = MAC_CSMA_MAX_ATTEMPTS_DEFAULT;
   MAC_ConfigAttr.CsmaMaxBackOffTime         = MAC_CSMA_MAX_BACKOFF_TIME_DEFAULT;
   MAC_ConfigAttr.CsmaMinBackOffTime         = MAC_CSMA_MIN_BACKOFF_TIME_DEFAULT;
   MAC_ConfigAttr.CsmaPValue                 = MAC_CSMA_PVALUE_DEFAULT;
   MAC_ConfigAttr.CsmaQuickAbort             = MAC_CSMA_QUICK_ABORT_DEFAULT;
   MAC_ConfigAttr.PacketTimeout              = MAC_PACKET_TIMEOUT_DEFAULT;
   MAC_ConfigAttr.ReassemblyTimeout          = MAC_REASSEMBLY_TIMEOUT_DEFAULT;

   MAC_ConfigAttr.ReliabilityHighCount       = MAC_RELIABILITY_HIGH_RETRY_COUNT_DEFAULT;
   MAC_ConfigAttr.ReliabilityMediumCount     = MAC_RELIABILITY_MEDIUM_RETRY_COUNT_DEFAULT;
   MAC_ConfigAttr.ReliabilityLowCount        = MAC_RELIABILITY_LOW_RETRY_COUNT_DEFAULT;
#if ( DCU == 1 )
   MAC_ConfigAttr.TxPacketDelay              = 0;
#endif
   MAC_ConfigAttr.TransactionTimeout         = MAC_TRANSACTION_TIMEOUT_DEFAULT;
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   MAC_ConfigAttr.LinkParamMaxOffset         = MAC_LINK_PARAM_MAX_OFFSET_DEFAULT;
   MAC_ConfigAttr.LinkParamPeriod            = MAC_LINK_PARAM_PERIOD_DEFAULT;
   MAC_ConfigAttr.LinkParamStart             = MAC_LINK_PARAM_START_DEFAULT;
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
   MAC_ConfigAttr.macCmdRespMaxTimeDiversity = MAC_CMD_RESP_MAX_TIME_DIVERSITY_DEFAULT;
#endif
#if ( DCU == 1 )
   MAC_ConfigAttr.IsFNG                      = (bool)true;
#else  // EP
   MAC_ConfigAttr.IsFNG                      = (bool)false;
#endif
   MAC_ConfigAttr.IsIAG                      = (bool)false;
   MAC_ConfigAttr.IsRouter                   = (bool)false;

   if(bWrite)
   {   // Write the data to the file
      (void)FIO_fwrite( &ConfigFile.handle, 0, ConfigFile.Data, ConfigFile.Size);
   }
}

/*
 * This function will initialize all of the "Counter" attributes to default values
 */
static void CachedAttr_Init(bool bWrite)
{
   (void) memset(&CachedAttr, 0, sizeof(CachedAttr));
   (void) TIME_UTIL_GetTimeInSecondsFormat( &CachedAttr.LastResetTime );
   if(bWrite)
   {   // Write the data to the file
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
#if ( DCU == 1 )
      /* This is a good time to flush all statistics on the T-board. */
      /* This is because if we lose power, the statistics are not flushed and since they are not flushed, the counters could appeared to have rolled backward next time they are read. */
      /* Flush all partitions (only cached actually flushed) and ensures all pending writes are completed */
      (void)PAR_partitionFptr.parFlush( NULL );
#endif
   }
}

/***********************************************************************************************************************
Function Name: MAC_init

Purpose: Initialize the necessary items before tasks start running in the system

Arguments: RetVal - indicates success or the reason for error

Returns: none
***********************************************************************************************************************/
returnStatus_t MAC_init ( void )
{
   TIMESTAMP_t CurrentTime;
   returnStatus_t retVal = eFAILURE;

   (void) TIME_UTIL_GetTimeInSecondsFormat( &CurrentTime );
   if (eFAILURE == TimeSet_BlockingTimer_Create())
   {
      return eFAILURE;
   }

   if (!IndicationCreate(&commIndication))
   {
      return eFAILURE;
   }

   // Create the data mutex used by STAR
   if ( ! OS_MUTEX_Create( &Mutex_ ) )
   {
      return eFAILURE;
   }
   //TODO RA6: NRJ: determine if semaphores need to be counting
   if (OS_SEM_Create(&MAC_AttributeSem_, 0) && OS_MUTEX_Create(&MAC_AttributeMutex_) && OS_MSGQ_Create(&MAC_msgQueue, MAC_NUM_MSGQ_ITEMS, "MAC") && OS_MUTEX_Create(&MAC_TimeSyncAttributeMutex_) &&
       (eSUCCESS == MAC_FrameManag_init()) && (eSUCCESS == MAC_PacketManag_init()) )
   {
      // Register the message handler
      PHY_RegisterIndicationHandler(MAC_Indication);

      // Initialize the attributes to default values, but don't write
      ConfigAttr_Init( (bool) false);

      // Clear the counter values, and set the Last Update Time, don't write
      CachedAttr_Init( (bool) false);

      FileStatus_t fileStatus;
      uint8_t i;
#if ( EP == 1 )
      if(PWRLG_LastGasp() == false)
#endif
      {  // Normal Mode
         for( i=0; i < (sizeof(Files)/sizeof(*(Files))); i++ )
         {
            file_t *pFile = (file_t *)Files[i];

            if ( eSUCCESS == FIO_fopen(&pFile->handle,      /* File Handle so that PHY access the file. */
                                       pFile->ePartition,   /* Search for the best partition according to the timing. */
                                       (uint16_t) pFile->FileId, /* File ID (filename) */
                                       pFile->Size,         /* Size of the data in the file. */
                                       pFile->Attr,         /* File attributes to use. */
                                       pFile->UpdateFreq,   /* The update rate of the data in the file. */
                                       &fileStatus) )       /* Contains the file status */
            {
               if ( fileStatus.bFileCreated )
               {  // The file was just created for the first time.
                  // Save the default values to the file
                  retVal = FIO_fwrite( &pFile->handle, 0, pFile->Data, pFile->Size);
               }
               else
               {  // Read the MAC File( Config & Cached ) Data
                  retVal = FIO_fread( &pFile->handle, pFile->Data, 0, pFile->Size);
               }
            }
            if (eFAILURE == retVal)
            {
               return eFAILURE;
            }
         }
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
         LinkParameters_ManageAlarm();
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
         (void)macCommandResponse_TimeDiversityTimer_Create();
#endif
      }
#if ( EP == 1 )
      else
      {  // Low Power Mode
         // We need to recover the configuration from VbatRegisterFile
         MAC_RestoreConfig_From_BkupRAM();

         // Read MAC channel sets from flash
         file_t *pFile = (file_t *)Files[0];
         if ( eSUCCESS == FIO_fopen(&pFile->handle,       /* File Handle so that PHY access the file. */
                                     pFile->ePartition,   /* Search for the best partition according to the timing. */
                          (uint16_t) pFile->FileId,       /* File ID (filename) */
                                     pFile->Size,         /* Size of the data in the file. */
                                     pFile->Attr,         /* File attributes to use. */
                                     pFile->UpdateFreq,   /* The update rate of the data in the file. */
                                     &fileStatus) )            /* Contains the file status */
         {
            // Read the MAC channel sets
            (void)FIO_fread( &pFile->handle, (uint8_t*)MAC_ConfigAttr.ChannelSetsSRFN,
                              ( uint32_t )offsetof( MacConfigAttr_t, ChannelSetsSRFN ), sizeof(MAC_ConfigAttr.ChannelSetsSRFN));
            retVal = eSUCCESS;
         }
      }
#endif
#if ( TX_THROTTLE_CONTROL == 1 )
      retVal = TxThrottle_Init();
#endif   /* ( TX_THROTTLE_CONTROL == 1 ) */
   }
   return ( retVal );
}

/*!
 * This is used to determine if we are waiting for an ack after transmitting
 */
static bool IsAckPending(void)
{
   // Acks not supported in this version
   return (bool)false;
}

/*!
 * This functions is called to handle MAC Confirmations to any MAC Request
 *
 * If a confirmation handler was specified in the request, it will call that handler.
 * else it will print the results of the request.
 */
void MAC_SendConfirm(MAC_Confirm_t const *pConf, buffer_t const *pReqBuf)
{
   MAC_Request_t *pReq = (MAC_Request_t*) &pReqBuf->data[0];  /*lint !e2445 !e740 !e826 */

   if (pReq->pConfirmHandler != NULL )
   {  // Call the handler specified
      (pReq->pConfirmHandler)(pConf);
   }else
   {
      char* msg;
      switch(pConf->Type)
      {
      case eMAC_GET_CONF    :  msg = MAC_GetStatusMsg(   pConf->GetConf.eStatus);   break;
      case eMAC_RESET_CONF  :  msg = MAC_ResetStatusMsg( pConf->ResetConf.eStatus); break;
      case eMAC_SET_CONF    :  msg = MAC_SetStatusMsg(   pConf->SetConf.eStatus);   break;
      case eMAC_START_CONF  :  msg = MAC_StartStatusMsg( pConf->StartConf.eStatus); break;
      case eMAC_STOP_CONF   :  msg = MAC_StopStatusMsg(  pConf->StopConf.eStatus);  break;
#if ( EP == 1 )
      case eMAC_TIME_QUERY_CONF:  msg = MAC_TimeQueryStatusMsg(pConf->TimeQueryConf.eStatus);  break;
#endif
      case eMAC_DATA_CONF   :  msg = MAC_DataStatusMsg(  pConf->DataConf.status);   break;
      case eMAC_PURGE_CONF  :  msg = MAC_PurgeStatusMsg( pConf->PurgeConf.eStatus); break;
      case eMAC_FLUSH_CONF  :  msg = MAC_FlushStatusMsg( pConf->FlushConf.eStatus); break;
      case eMAC_PING_CONF   :  msg = MAC_PingStatusMsg(  pConf->PingConf.eStatus); break;
      default               :  msg = "Unknown";
      }  /*lint !e788 not all enums are handled by the switch  */
      INFO_printf("CONFIRM:%s, handle:%d", msg, pConf->handleId);
   }
}

/***********************************************************************************************************************
Function Name: ConfigureTimeout

Purpose: Configure a timeout in case a PHY acknowledge never arrives.

Arguments: confType - Confirmation type
           timeout  - timeout length in msec

Returns: none
***********************************************************************************************************************/
static void ConfigurePHYTimeout( PHY_CONF_TYPE_t confType, uint32_t timeout )
{
   phy.confirmPending = (bool)true;
   phy.pendingConfirmType = confType;
   OS_TICK_Get_CurrentElapsedTicks ( &phy.confirmTimeout );
#if ( MCU_SELECTED == NXP_K24 )
   (void) _time_add_msec_to_ticks ( &phy.confirmTimeout, timeout );
#elif ( MCU_SELECTED == RA6E1 )
   ( void ) OS_TICK_Add_msec_to_ticks( &phy.confirmTimeout, timeout );
#endif
}

/***********************************************************************************************************************
Function Name: CheckTimeout

Purpose: Check for a timeout.

Arguments: none

Returns: none
***********************************************************************************************************************/
static void CheckPHYTimeout( void )
{
   OS_TICK_Struct CurrentTicks;
   OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );
   if( OS_TICK_Is_FutureTime_Greater ( &phy.confirmTimeout, &CurrentTicks ) )
   {
      ERR_printf("Expected PHY confirmation never arrived, attempting to generate a failure confirmation");
      GenerateFailedConfirm( phy.pendingConfirmType );
   }
}

/***********************************************************************************************************************
Function Name: MAC_PHY_CheckForFailure

Purpose: Check that a frame was sent to PHY in a timely manner after a MAC dataRequest was received.
         We saw evidence that sometimes the MAC keep receiving packets from NWK but fails to send frames to PHY.

Arguments: none

Returns: none
***********************************************************************************************************************/
static void MAC_PHY_CheckForFailure( void )
{
   OS_TICK_Struct time;
   uint32_t TimeDiff;

   // Check if we have received a data request
#if ( MCU_SELECTED == NXP_K24 )
   bool Overflow;
   if ( (DataReqTime.TICKS[0] != 0) || (DataReqTime.TICKS[1] != 0) )
   {
      // Check if PHY as been accessed recently
      _time_get_elapsed_ticks(&time);

      TimeDiff = (uint32_t)_time_diff_seconds ( &time, &DataReqTime, &Overflow );

      if ( ( TimeDiff > 600 ) || Overflow )
      {
         // More than 5 minutes elapsed between MAC/PHY
         ERR_printf("PHY didn't received a frame in a timely manner");
         OS_TASK_Sleep(5*ONE_SEC);
         PWR_SafeReset();
      }
   }
#elif ( MCU_SELECTED == RA6E1 )
   if ( ( DataReqTime.tickCount != 0 ) || ( DataReqTime.xNumOfOverflows != 0 ) )
   {
      // Check if PHY as been accessed recently
      OS_TICK_Get_CurrentElapsedTicks(&time);

      TimeDiff = (uint32_t)OS_TICK_Get_Diff_InSeconds( &time, &DataReqTime );

      // TODO: RA6E1 Overflow check removed as there is no support. Add support to overflow flag
      if ( ( TimeDiff > 600 ) /* || Overflow */ )
      {
         // More than 5 minutes elapsed between MAC/PHY
         ERR_printf("PHY didn't received a frame in a timely manner");
         OS_TASK_Sleep(5*ONE_SEC);
         PWR_SafeReset();
      }
   }
#endif
}

/***********************************************************************************************************************
Function Name: MAC_Task

Purpose: Task for the MAC networking layer

Arguments: Arg0 - Not used, but required here because this is a task

Returns: none
***********************************************************************************************************************/
void MAC_Task ( taskParameter )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   (void)         Arg0;
#endif
   buffer_t       *pBuf;
   uint16_t       numBits;
   TIMESTAMP_t    CurrentTime;
   uint16_t       uMessagePend = 500;
   MAC_DataReq_t  *macRequest;
#if ( EP == 1 )
   uint8_t        uLastGasp = false;
#endif
   uint32_t       RxTime; // Tell PHY when the receiver SYNC should happen. This is for deterministic transmission.

   buffer_t *pRequestBuf = NULL;        // Handle to unconfirmed NWK request buffers

   INFO_printf("MAC_Task starting...");

#if ( EP == 1 )
   if( PWRLG_LastGasp() == true )
   {  // Booting in Low Power mode, so set the tx frequency to the value that was saved
      // When booting in normal mode the phy will recover from NV.
      uMessagePend = 100;
      uLastGasp = true;
   }
#endif

   for ( ;; )
   {
      if(phy.eState == eMAC_STATE_CCA_DELAY)
      {  // When in the CCA Delay state, msec resolution is required, or at least desired!
         // Also note that the minimum delay is 5 msec.
         // Do not set less than 5 msec due to issue in OS_MSGQ_Pend()
         uMessagePend = 5;
      }
      else{
#if ( EP == 1 )
         if (uLastGasp)
         {   // For last Gasp just delay 100 msec
            uMessagePend = 100;
         }
         else
         {
#if ( TEST_TDMA == 1 )
           uMessagePend = 10; // Make MAC more responsive during test
#else
           uMessagePend = 500;
#endif
         }
#else
         uMessagePend = 500;
#endif
      }

      if (OS_MSGQ_Pend( &MAC_msgQueue, (void *)&pBuf, uMessagePend))
      {
         bool bConfirmReady = (bool)false;
         switch (pBuf->eSysFmt)  /*lint !e788 not all enums used within switch */
         {

            case eSYSFMT_TIME_DIVERSITY:
            {
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
               uint16_t *timerID = (uint16_t*)&pBuf->data[0]; /*lint !e826 */
               INFO_printf("TimerID: %d", *timerID);
               if( *timerID == macCmdResp_TimeDiversity_TimerID_ )
               {
                   INFO_printf("MAC Command Response TimerDiversity_Event");
                   (void)MAC_TimePush_Request( BROADCAST_MODE, NULL, NULL, 0 );
               }
               else
#endif
               {
#if ( PHASE_DETECTION == 1 )
                  PD_HandleTimeDiversity_Event( );
                  INFO_printf("PD_TimerDiversity_Event");
#endif
               }
               BM_free( pBuf ); //Free the buffer
            }
            break;

            case eSYSFMT_TIME:
            {
               // get the alarm info from the buffer
              tTimeSysMsg *alarmMsg = (tTimeSysMsg *)pBuf->data; /*lint !e826 */

              if( ( alarmMsg->alarmId == TIME_SYNC_PeriodicAlaramID_Get() ) ||   /* Periodic Alarm */
                  ( pBuf->data[0] == TimeSync.TimerId ) )                        /* Blocking Timer driven Time Sync */
              {  //It must be a Time Sync Timer Event
                 INFO_printf("Sending Time-sync");
                 (void)MAC_TimePush_Request( BROADCAST_MODE, NULL, NULL, 0 );
              }
#if ( PHASE_DETECTION == 1 )
              else if( PD_HandleSysTimer_Event( alarmMsg ) )
              {  // This was a Phase Detection Sys Timer Event
                 INFO_printf("PD_SysTimer_Event");
              }
#endif
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
              else if( alarmMsg->alarmId == linkParam_P_AlaramID_ )
              {
                  PHY_GetConf_t  GetConf;

                  GetConf = PHY_GetRequest( ePhyAttr_txEnabled );
                  if ( ( ePHY_GET_SUCCESS == GetConf.eStatus ) && MAINBD_IsPresent() && ( TxChannels_init( eMAC_CHANNEL_SETS_SRFN, eMAC_CHANNEL_SET_INDEX_1 ) && GetConf.val.txEnabled ) )
                  {
                     // It is a MAC Link Parameters Periodic Timer Event
                     INFO_printf("MAC-Link-Parameters Periodic Event");
                     (void)MAC_LinkParametersPush_Request( BROADCAST_MODE, NULL, NULL );
                  }
              }
#endif
              //Free the buffer
              BM_free(pBuf);
            }
            break;

            case eSYSFMT_PHY_INDICATION:
            {
               PHY_Indication_t    *pMsg = (PHY_Indication_t*) pBuf->data; /*lint !e2445 !e740 !e826 */
               switch(pMsg->Type)
               {
                  case ePHY_DATA_IND:   Process_DataIndication( &pMsg->DataInd ) ; break;
                  default:  break;
               }
               BM_free(pBuf);
            }
            break;

            case eSYSFMT_PHY_CONFIRM:           // Confirmation from PHY
            {
               // Process a confirmation from the PHY
               PHY_Confirm_t    *pPhyConfirm = (PHY_Confirm_t*) pBuf->data; /*lint !e2445 !e740 !e826 */
               bool bConfirmStatus = (bool)false;

               // We communicated with PHY or at least we tried (got timeout)
               // Clear MAC/PHY watchdog
               (void)memset(&DataReqTime, 0, sizeof( DataReqTime ));

               switch(pPhyConfirm->MsgType)  /*lint !e788 not all enums used within switch */
               {
                  case ePHY_CCA_CONF:   bConfirmStatus = Process_CcaConfirm(   &pPhyConfirm->CcaConf   ); break;
                  case ePHY_GET_CONF:   bConfirmStatus = Process_GetConfirm(   &pPhyConfirm->GetConf   ); break;
                  case ePHY_SET_CONF:   bConfirmStatus = Process_SetConfirm(   &pPhyConfirm->SetConf   ); break;
                  case ePHY_RESET_CONF: bConfirmStatus = Process_ResetConfirm( &pPhyConfirm->ResetConf ); break;
                  case ePHY_DATA_CONF:  bConfirmStatus = Process_DataConfirm(  &pPhyConfirm->DataConf  );
                     INFO_printf("PHY_READY");phy.eState = eMAC_STATE_PHY_READY;  break;
                  default:  break;
               }  /*lint !e788 not all enums used within switch */
               if( bConfirmStatus )
               {
                  phy.confirmPending = (bool)false;
               }
               BM_free(pBuf); // Always free the PHY_Confirm buffer
            }
            break;

            case eSYSFMT_MAC_REQUEST:
            {
               // Request message from MAC Interface
               MAC_Request_t *pReq =  (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */

               switch(pReq->MsgType)
               {
                  // MCPS
                  case eMAC_DATA_REQ:  bConfirmReady = Process_DataReq(  pBuf                    ); break;
                  case eMAC_PING_REQ:  bConfirmReady = Process_PingReq(  pBuf                    ); break;
                  case eMAC_PURGE_REQ: bConfirmReady = Process_PurgeReq( &pReq->Service.PurgeReq ); break;
                  case eMAC_FLUSH_REQ: bConfirmReady = Process_FlushReq( &pReq->Service.FlushReq ); break;
#if ( EP == 1 )
                  case eMAC_TIME_QUERY_REQ:  bConfirmReady = Process_TimeQueryReq(&pReq->Service.TimeQueryReq); break;
#endif

                  // MLME
                  case eMAC_GET_REQ:   bConfirmReady = Process_GetReq(   &pReq->Service.GetReq   ); break;
                  case eMAC_RESET_REQ: bConfirmReady = Process_ResetReq( &pReq->Service.ResetReq ); break;
                  case eMAC_SET_REQ:   bConfirmReady = Process_SetReq(   &pReq->Service.SetReq   ); break;
                  case eMAC_START_REQ: bConfirmReady = Process_StartReq( &pReq->Service.StartReq ); break;
                  case eMAC_STOP_REQ:  bConfirmReady = Process_StopReq(  &pReq->Service.StopReq  ); break;
                  default:
                    INFO_printf("MAC_REQUEST:UnknownReq");
                    BM_free(pBuf);
                    phy.confirmPending = (bool)false;
                    break;
               }
               pRequestBuf = pBuf;
               break;
            }
#if ( EP == 1 )
            case eSYSFMT_VISUAL_INDICATION:
               // Delete timer and turn off LED
#if 0 // TODO: RA6E1 Enable LED functions
               (void) TMR_DeleteTimer(LED_getLedTimerId());
#endif
               BM_free(pBuf);
               break;
#endif
            default:
               BM_free(pBuf);
               break;
         }  /*lint !e788 not all enums used within switch */
         // Determine if there is a MAC Confirmation that was created by either the MAC_Request or PHY Confirm
         // Only one confirmation can be created each time a MAC Request or PHY Confirm is processed
         if( bConfirmReady && ( pRequestBuf != NULL ) )
         {
            MAC_Request_t *pReq = (MAC_Request_t *) pRequestBuf->data;  /*lint !e2445 !e613 !e740 !e826 */
            ConfirmMsg.handleId = pReq->handleId;
            MAC_SendConfirm(&ConfirmMsg, pRequestBuf);
            BM_free(pRequestBuf);
         }
      }

      MacPacket_s *pPacket = NULL;

      MAC_FrameManag_CheckTimeouts();

      MAC_PHY_CheckForFailure();

      if ( ! MAC_txPaused )
      {
         if( phy.eState == eMAC_STATE_PHY_READY)
         {
            // Is there an In-process transaction or Ack pending

            if( IsAckPending() )
            {  // There is an ACK pending
               // todo: 6/5/2015 12:41:00 PM [BAH] - ACK's not yet supported
            }
            else if ( MAC_PacketManag_IsTxMessagePending(&pPacket))
            {  // There is a Tx Message Pending, and no acks!
               // Resolve Transmit Frame when no pending Acks exist
               if( MAC_FrameManag_IsPacketInReassembly() )
               {  // Currently receiving at least one packet
                  // Check if there are any UNICAST packets in reassembly?
                  // This implies means there are packets in re-assembly using destination address mode == UNICAST
                  if( MAC_FrameManag_IsUnicastPacketInReassembly() )
                  {  // Can't transmit if there is a unicast packet in reassembly
                     INFO_printf("Can't tx, receiving unicast packet");
                  }else
                  {  // Only broadcast packets are in re-assembly
                     // Check if the destination for this packet is unicast
                     // and the destination address matches the source in the rx buffer
                     macRequest = MAC_GetDataReqFromBuffer((buffer_t*)pPacket->memToFree);

                     if(macRequest->dst_addr_mode == UNICAST_MODE)
                     {  // Are we receiving from the unit we want to transmit to?
                        if( ! MAC_FrameManag_IsUnitTransmitting(macRequest->dst_addr) )
                        {  // OK to transmit, if that unit is not transmitting
                           (void) MAC_PacketManag_GetNextTxMsg();
                        }
                        else{
                          INFO_printf("Can't tx unicast packet, receiving broadcast from dst_addr");
                        }
                     }else
                     {  // OK to transmit
                        (void) MAC_PacketManag_GetNextTxMsg();
                     }
                  }
               }else
               {  // Not currently receiving any packets so it is OK start transmitting
                  // Make sure we don't have a frame that needs to be retransmitted
                  if ( MAC_FrameManag_GetNextTxMsg() == NULL ) {
                     // No frame in TX queue, process next packet
                     (void) MAC_PacketManag_GetNextTxMsg();
                  }
               }
            }
         }
      }

      // are we waiting for a confirm
      if( ! phy.confirmPending )
      {
         switch(phy.eState)
         {
            case eMAC_STATE_PHY_READY:
            {
               phy.Frame = MAC_FrameManag_GetNextTxMsg();
               if ( phy.Frame != (void *)0 )
               {
                  if( ( tx_channels.cnt > 0 ) && ( pPacket != NULL ) )
                  {
                     phy.gain      = 0;
                     phy.channel   = pPacket->chan;

#if (DCU == 1)
                     // The EP Does not support STAR Modes
                     if(( pPacket->frame_type == MAC_STAR_DATA_FRAME ) ||
                        ( pPacket->frame_type == MAC_STAR_FAST_MESSAGE ))
                     {
                        (void) memcpy(phy.data, phy.Frame->MsgData.data, phy.Frame->MsgData.thisFrameLen);
                        phy.size    = (uint8_t) phy.Frame->MsgData.thisFrameLen;
                        phy.tx_type = eTX_TYPE_STAR;

                        // STAR Gen II 7200 bps (Normal STAR preamble and sync)
                        phy.ePhyDetection = ePHY_DETECTION_1;
                        if( pPacket->frame_type == MAC_STAR_FAST_MESSAGE )
                        {
                           // STAR Gen II 7200 bps fast message
                           phy.ePhyDetection = ePHY_DETECTION_3;
                        }
                        phy.ePhyFraming         = ePHY_FRAMING_2,          /*!< STAR Generation 2 */
                        phy.ePhyMode            = ePHY_MODE_2;             /*!< 2-FSK,   (63,59) Reed-Solomon, 7200 baud */
                        phy.ePhyModeParameters  = ePHY_MODE_PARAMETERS_0;  /*!< RS(63,59)   Reed-Solomon */

                        if( pPacket->skip_cca )
                        {  // Just send the request without doing CCA
                           RxTime = 0; // Reset packet RX time

                           if( IsTxAllowed() )  // Check TX throttle (always enabled if TX_THROTTLE_CONTROL != 1)
                           {
                              // Transmit
                              /* Build a Data.Request message from this data and send to the PHY layer */
                              INFO_printf("TRANSMITTING");
                              if(PHY_DataRequest(phy.data, phy.size, phy.channel, NULL, phy.ePhyDetection, phy.ePhyFraming,
                                                 phy.ePhyMode, phy.ePhyModeParameters, phy_confirm_cb, RxTime, ePHY_TX_CONSTRAIN_REGULAR))
                              {
                                 ConfigurePHYTimeout( ePHY_DATA_CONF, PHY_DATA_CONFIRM_TIMEOUT_MS );
#if ( TX_THROTTLE_CONTROL == 1 )
                                 UpdateTxThrottle();
#endif
                                 phy.eState = eMAC_STATE_TRANSMITTING;
                              }else
                              {
                                 // The data request failed, so we need to fail this transaction
                                 ERR_printf("DataReq:Error");
                                 MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                                 phy.eState = eMAC_STATE_PHY_READY;
                              }
                           } else {
                              // The data request failed, so we need to fail this transaction
                              ERR_printf("DataReq:TX limit reached");
                              MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                              phy.eState = eMAC_STATE_PHY_READY;
                           }
                        }else
                        {
                           INFO_printf("CCA_PENDING");
                           if(PHY_CcaRequest(phy.channel, phy_confirm_cb))
                           {
                              ConfigurePHYTimeout( ePHY_CCA_CONF, PHY_CCA_CONFIRM_TIMEOUT_MS );

                              phy.csmaAttempts = 0;
                              phy.eState = eMAC_STATE_CCA_PENDING;   // Waiting for a clear channel
                           }else
                           {  // The CCA Request failed, so abort this transaction
                              ERR_printf("CCA:Error");
                              MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                           }
                        }
                     }
                     else
#endif
                     {
                        // For SRFN, Call the Encode Function
                        numBits       = MAC_Codec_Encode(&phy.Frame->MsgData, phy.data, &RxTime );
                        phy.size      = (uint8_t) ((numBits+7)/8);
                        phy.tx_type   = eTX_TYPE_SRFN;

                        phy.ePhyDetection      = ePHY_DETECTION_0;        // SRFN preamble and sync
                        phy.ePhyFraming        = ePHY_FRAMING_0;          // SRFN
                        phy.ePhyMode           = ePHY_MODE_1;             // 4-GFSK, 4800 baud
                        phy.ePhyModeParameters = ePHY_MODE_PARAMETERS_0;  // RS(63,59)  Reed-Solomon

                        INFO_printf("CCA_PENDING");
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
                        if ( MAC_ConfigAttr.CsmaSkip ) {
                           phy.csmaAttempts = 0;
                           phy.eState = eMAC_STATE_CCA_PENDING;   // Waiting for a clear channel
                        } else {
#endif
                        if(PHY_CcaRequest(phy.channel, phy_confirm_cb))
                        {
                           ConfigurePHYTimeout( ePHY_CCA_CONF, PHY_CCA_CONFIRM_TIMEOUT_MS );

                           phy.csmaAttempts = 0;
                           phy.eState = eMAC_STATE_CCA_PENDING;   // Waiting for a clear channel
                        }else
                        {  // The CCA Request failed, so abort this transaction
                           ERR_printf("CCA:Error");
                           MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                        }
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
                        }
#endif
                     }
                  }else
                  {  // Fail the command since there are no valid channels configured
                     MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                  }
               }
            }
            break;

            case eMAC_STATE_CCA_PENDING:  // Waiting for a clear channel
            {
#if (DCU == 1)
               // The EP Does not support STAR Modes
               if( phy.tx_type == eTX_TYPE_STAR )
               {  // STAR Mode
                  // If the channel is clear, or we have used up all of the attempts ( 500 msec )
                  if( phy.channelClear || (phy.csmaAttempts >= MAC_ConfigAttr.CsmaMaxAttempts))
                  {
                     RxTime = 0; // Reset packet RX time

                     if( IsTxAllowed() )  // Check TX throttle (always enabled if TX_THROTTLE_CONTROL != 1)
                     {
                        // Transmit
                        /* Build a Data.Request message from this data and send to the PHY layer */
                        INFO_printf("TRANSMITTING");
                        if(PHY_DataRequest(phy.data, phy.size, phy.channel, NULL, phy.ePhyDetection, phy.ePhyFraming,
                                           phy.ePhyMode, phy.ePhyModeParameters, phy_confirm_cb, RxTime, ePHY_TX_CONSTRAIN_REGULAR))

                        {
                           ConfigurePHYTimeout( ePHY_DATA_CONF, PHY_DATA_CONFIRM_TIMEOUT_MS );
#if ( TX_THROTTLE_CONTROL == 1 )
                           UpdateTxThrottle();
#endif
                           phy.eState = eMAC_STATE_TRANSMITTING;
                        }else
                        {
                           // The data request failed, so we need to fail this transaction
                           ERR_printf("DataReq:Error");
                           MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                           phy.eState = eMAC_STATE_PHY_READY;
                        }
                     } else {
                        // The data request failed, so we need to fail this transaction
                        ERR_printf("DataReq:TX limit reached");
                        MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                        phy.eState = eMAC_STATE_PHY_READY;
                     }
                  }else
                  {  // Do another CCA attempt
                     phy.csmaAttempts++;

                     // Delay for 50 msec and try again.
                     // This is intended to limit the total CSMA time to 500 msec ( 50 * 10 attempts )
                     OS_TASK_Sleep ( 50 );

                     if(PHY_CcaRequest(phy.channel, phy_confirm_cb))
                     {
                        phy.confirmPending = (bool)true;
                        phy.eState = eMAC_STATE_CCA_PENDING;   // Waiting for a clear channel
                     }else
                     {  // The CCA Request failed, so abort this transaction
                        ERR_printf("CCA:Error");
                        MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                        phy.eState = eMAC_STATE_PHY_READY;
                     }
                  }
               }else
#endif
               {  // SRFN Mode
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
                  if( phy.channelClear || MAC_ConfigAttr.CsmaSkip )
                  {
                     if( P_Persistence_Get() || MAC_ConfigAttr.CsmaSkip)
#else
                  if( phy.channelClear )
                  {
                     if( P_Persistence_Get() )
#endif
                     {
                        /* Build a Data.Request message from this data and send to the PHY layer */
   //                   INFO_printHex("Data:", phy.data, phy.size);

                        PHY_TX_CONSTRAIN_e txConstrain = ePHY_TX_CONSTRAIN_REGULAR; // Default constrain

                        RxTime = 0; // Reset packet RX time

                        if(phy.Frame->MsgData.frame_type == MAC_CMD_FRAME)
                        { /* If this is a command frame, and it is a set time command,
                             the time must be updated, so re-encode the frame
                          */
                           mac_frame_t *p = &phy.Frame->MsgData;

                           numBits  = MAC_Codec_Encode(p, phy.data, &RxTime );

                           if ( p->frame_type == MAC_CMD_FRAME )
                           {
                              if ( p->data[0] == MAC_TIME_SET_CMD )
                              {  //Start the blocking timer for time-sync
#if ( MFG_MODE_DCU == 0 )        //Blocking enabled (disabled for MFG_MODE)
                                 TimeSet_BlockingTimer_Start();
#endif
                                 txConstrain = ePHY_TX_CONSTRAIN_EXACT; // Deterministic PHY TX
                              }
                           }
                           phy.size = (uint8_t) ((numBits+7)/8);
                        }
                        if( IsTxAllowed() )  // Check TX throttle (always enabled if TX_THROTTLE_CONTROL != 1)
                        {
                           INFO_printf("TRANSMITTING");
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
                           txConstrain = ePHY_TX_CONSTRAIN_EXACT; // Deterministic PHY TX
                           sysTime_t   sysTime;
                           sysTime_t   sysTime1;
                           volatile uint64_t    syncFuture;
                           uint32_t    time;
                           uint32_t primask = __get_PRIMASK();
                           __disable_interrupt(); // Disable all interrupts. This section is time critical but fast fortunatly.
                           TIME_SYS_GetSysDateTime( &sysTime );
                           time = DWT_CYCCNT; // Get time in CPU cycles
                           __set_PRIMASK(primask); // Restore interrupts
                           do {
                              uint32_t primask = __get_PRIMASK();
                              __disable_interrupt(); // Disable all interrupts. This section is time critical but fast fortunatly.
                              TIME_SYS_GetSysDateTime( &sysTime1 );
                              time = DWT_CYCCNT; // Get time in CPU cycles
                              __set_PRIMASK(primask); // Restore interrupts
                           } while ( sysTime.time == sysTime1.time );
                           sysTime = sysTime1;

                           uint32_t currentTime, futureTime;
                           currentTime = sysTime.time % 1000;

                           // Check if next time slot is in this "second" or the next "second"
                           if ( MAC_ConfigAttr.TxSlot > ((sysTime.time/200) % 5) ) {
                              // Time slot is in this second
                              futureTime = (uint32_t)MAC_ConfigAttr.TxSlot*200;
                           } else {
                              // Time slot is in next time slot
                              futureTime = ((uint32_t)MAC_ConfigAttr.TxSlot*200) + 1000U;
                           }

                           syncFuture = futureTime - currentTime;
#if 0
                           switch ( (sysTime.time/200) % 5) {
                              case 0: syncFuture = ((sysTime.time/200)*200 + 400) - sysTime.time;
                                 break;
                              case 1: syncFuture = ((sysTime.time/200)*200 + 600) - sysTime.time;
                                 break;
                              case 2: syncFuture = ((sysTime.time/200)*200 + 400) - sysTime.time;
                                 break;
                              case 3: syncFuture = ((sysTime.time/200)*200 + 400) - sysTime.time;
                                 break;
                              case 4: syncFuture = ((sysTime.time/200)*200 + 600) - sysTime.time;
                                 break;
                              default: break;
                           }
#endif
                           uint32_t cpuFreq;
                           (void)TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL );
//                           RxTime = time + (uint32_t)(syncFuture * (uint64_t)cpuFreq / 1000ULL) - (DWT_CYCCNT - time) + 3237416U + (timeSlotDelay * (cpuFreq/1000U));
                           RxTime  = time - sysTime.elapsedCycle; // Adjust for elapsed time
                           RxTime += (uint32_t)((uint64_t)(syncFuture + timeSlotDelay) * (uint64_t)cpuFreq / 1000ULL); // Add delay for future time slot and slot delay
                           RxTime += (uint32_t)((uint64_t)80 * (uint64_t)cpuFreq / 4800ULL); // Add delay for short preamble and SYNC
#endif
                           if(PHY_DataRequest(phy.data, phy.size, phy.channel, NULL, phy.ePhyDetection, phy.ePhyFraming,
                                              phy.ePhyMode, phy.ePhyModeParameters, phy_confirm_cb, RxTime, txConstrain))

                           {
                              ConfigurePHYTimeout( ePHY_DATA_CONF, PHY_DATA_CONFIRM_TIMEOUT_MS );
#if ( TX_THROTTLE_CONTROL == 1 )
                              UpdateTxThrottle();
#endif
                              (void)TIME_UTIL_GetTimeInSecondsFormat( &CurrentTime );
                              phy.Frame->TimeTxRx        = CurrentTime;
                              phy.Frame->SentToNextLayer = (bool)true;
                              phy.eState                 = eMAC_STATE_TRANSMITTING;
                           }else
                           {
                              // The data request failed, so we need to fail this transaction
                              ERR_printf("DataReq:Error");
                              MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                              phy.eState = eMAC_STATE_PHY_READY;
                           }
                        }
#if ( DCU == 1 )
                        else {
                           // The data request failed, so we need to fail this transaction
                           ERR_printf("DataReq:TX limit reached");
                           MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                           phy.eState = eMAC_STATE_PHY_READY;
                        }
#endif
                     }else
                     {  // decided not to transmit, so delay and try again later
                        // unless quick abort is set
                        if (MAC_ConfigAttr.CsmaQuickAbort)
                        {
                           INFO_printf("MAC!!!!QuickAbort");
                           (void) TIME_UTIL_GetTimeInSecondsFormat( &CurrentTime );
                           MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                           INFO_printf("PHY_READY");
                           phy.eState = eMAC_STATE_PHY_READY;
                        }
                        else
                        {
                           INFO_printf("p-persistent: delay");
                           INFO_printf("csmaAttempts=%d", phy.csmaAttempts);
                        }
                     }
                  }else
                  {  // Channel is not clear, so increment the csma counter.
                     phy.csmaAttempts++;
                  }
                  // Are we still in the CCA Pending State?
                  if( phy.eState == eMAC_STATE_CCA_PENDING )
                  {  // If we did not transmit, so either the channel was busy or p-persistent decided to wait
                     if (phy.csmaAttempts <= MAC_ConfigAttr.CsmaMaxAttempts)
                     {
                        // Get a random backoff delay
                        uint16_t msec_delay = (uint16_t) RandomBackoffDelay_Get();
                        INFO_printf("cca_delay=%d msec", msec_delay);

                        // If the delay is less than 500 msec, then just delay here!
                        // todo: 4/30/2015 8:13:34 AM [BAH] - Review this. May want this to be less for Last Gasp!
                        if( msec_delay < 500 )
                        {
                           OS_TICK_Struct CurrentTicks;
                           OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );
                           phy.ccaDelayStart = CurrentTicks;                // Save this for measurement
                           phy.ccaDelayEnd   = CurrentTicks;
                           OS_TICK_Sleep ( &phy.ccaDelayEnd, msec_delay );

                           // Debug only!
                           // Print the meaured delay
                           OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );
                           INFO_printf("actual_delay: %li usec", OS_TICK_Get_Diff_InMicroseconds ( &phy.ccaDelayStart, &CurrentTicks  ));

                           // Now send another request
                           INFO_printf("CCA_PENDING");
                           if(PHY_CcaRequest(phy.channel, phy_confirm_cb))
                           {
                              ConfigurePHYTimeout( ePHY_CCA_CONF, PHY_CCA_CONFIRM_TIMEOUT_MS );

                              phy.eState = eMAC_STATE_CCA_PENDING;   // Waiting for a clear channel
                           }else
                           {  // The CCA Request failed, so abort this transaction
                              ERR_printf("CCA:Error");
                              MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                              phy.eState = eMAC_STATE_PHY_READY;
                           }
                        }
                        else
                        {  // Set the Timeout for the next cca attempt
                           OS_TICK_Struct CurrentTicks;
                           OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );
                           phy.ccaDelayStart = CurrentTicks;                // Save this for measurement
                           phy.ccaDelayEnd   = CurrentTicks;
#if ( MCU_SELECTED == NXP_K24 )
                           (void) _time_add_msec_to_ticks ( &phy.ccaDelayEnd, msec_delay );
#elif ( MCU_SELECTED == RA6E1 )
                           ( void ) OS_TICK_Add_msec_to_ticks( &phy.ccaDelayEnd, msec_delay );
#endif
                           INFO_printf("CCA_DELAY");
                           phy.eState = eMAC_STATE_CCA_DELAY;
                        }
                     }else
                     {  // Max Csma Attempts exceeded
                        INFO_printf("CCA:Max Attempts Exceeded %d ", MAC_ConfigAttr.CsmaMaxAttempts);
                        MAC_CounterInc(eMAC_ChannelAccessFailureCount );

                        // cleanup, etc
                        (void) TIME_UTIL_GetTimeInSecondsFormat( &CurrentTime );
                        MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);

                        INFO_printf("PHY_READY");
                        phy.eState = eMAC_STATE_PHY_READY;
                     }
                  }
               }
            }
            break;

            case eMAC_STATE_CCA_DELAY:
            {
               // Has the cca delay period expired?
               OS_TICK_Struct CurrentTicks;
               OS_TICK_Get_CurrentElapsedTicks ( &CurrentTicks );
               if( OS_TICK_Is_FutureTime_Greater ( &phy.ccaDelayEnd, &CurrentTicks ) )
               {  // Delay period has expired

                  // Print the measured delay
                  INFO_printf("delay : %li usec", OS_TICK_Get_Diff_InMicroseconds ( &phy.ccaDelayStart, &CurrentTicks  ));

                  INFO_printf("CCA_PENDING");
                  if(PHY_CcaRequest(phy.channel, phy_confirm_cb))
                  {
                     ConfigurePHYTimeout( ePHY_CCA_CONF, PHY_CCA_CONFIRM_TIMEOUT_MS );

                     phy.eState = eMAC_STATE_CCA_PENDING;   // Waiting for a clear channel
                  }else
                  {  // The CCA Request failed, so abort this transaction
                     INFO_printf("CCA:Error");
                     MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
                     phy.eState = eMAC_STATE_PHY_READY;
                  }
               }
            }
            break;

            case eMAC_STATE_TRANSMITTING:
            {
               // Waiting for the TxDone
               INFO_printf("Waiting for the TxDone");
               CheckPHYTimeout();
            }
            break;

            default:
            {
               ERR_printf("Unexpected phy.eState %u", (uint32_t)phy.eState);
            }
            break;

         }
      }
      else
      {
         // A confirm is pending, check if has timed out
#if !(( EP == 1 ) && ( TEST_TDMA == 1 ))
         INFO_printf("confirmPending");
#endif
         CheckPHYTimeout();
      }
#if ( TX_THROTTLE_CONTROL == 1 )
      /* Check for TxThrottle time out   */
      (void)TMR_ReadTimer( &TxThrottle_timer );
      if ( TxThrottle_timer.bExpired )
      {
         (void)TMR_ResetTimer( TxThrottle_ID, TX_THROTTLE_PERIOD * TIME_TICKS_PER_MIN );
      }
#endif
   }
}

/*!
 *  CCA Confirm from the PHY
 */
static bool Process_CcaConfirm( const PHY_CCAConf_t *pCcaConf)
{
   if ( pCcaConf->eStatus == ePHY_CCA_SUCCESS )
   {
      if( ! pCcaConf->busy )
      {  // channel not busy
         phy.channelClear = (bool)true;
      }else
      {  // Channel is busy
         phy.channelClear = (bool)false;
      }
   }else
   {  // the phy had a problem and could not do the cca
      // This should not happen, but just in case is does, handle like the channel is not clear!
      phy.channelClear = (bool)false;
   }
   INFO_printf("CcaConfirm:%s busy:%d rssi:%d dBm", PHY_CcaStatusMsg(pCcaConf->eStatus), pCcaConf->busy, (int32_t)RSSI_RAW_TO_DBM(pCcaConf->rssi));
   return (bool)true;
}


/*!
 *  Get Confirm from the PHY
 */
static bool Process_GetConfirm( const PHY_GetConf_t   *pGetConf)
{
   pMacConf->Type   = eMAC_GET_CONF;
   INFO_printf("GetConfirm:%s", PHY_GetStatusMsg(pGetConf->eStatus));
   return (bool)true;
}

/*!
 *  Set Confirm from the PHY
 */
static bool Process_SetConfirm( const PHY_SetConf_t   *pSetConf)
{
   pMacConf->Type   = eMAC_SET_CONF;
   INFO_printf("SetConfirm:%s", PHY_SetStatusMsg(pSetConf->eStatus));
   return (bool)true;
}

/*!
 *  Reset Confirm from the PHY
 */
static bool Process_ResetConfirm( const PHY_ResetConf_t *pResetConf)
{
   INFO_printf("ResetConfirm:%s", PHY_ResetStatusMsg(pResetConf->eStatus));
   pMacConf->Type = eMAC_RESET_CONF;
   pMacConf->ResetConf.eStatus = eMAC_RESET_SUCCESS;
   return (bool)true;
}

/*! ********************************************************************************************************************
 *
 * \fn void TxLinkDelayTimeInc(uint32_t delay)
 *
 * \brief  This function is used to increment the MAC TxLinkDelayTime counter
 *
 * \param  delay - Time in ms to add to TxLinkDelayTime
 *
 * \return  none
 *
 ********************************************************************************************************************
 */
static void TxLinkDelayTimeInc( uint32_t delay )
{
   CachedAttr.TxLinkDelayTime += delay;
   (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
}

/*!
 *  Data Confirm from the PHY
 */
static bool Process_DataConfirm( const PHY_DataConf_t *pDataConf)
{
   INFO_printf("DataConfirm:%s", PHY_DataStatusMsg(pDataConf->eStatus));

   switch( pDataConf->eStatus )
   {
      case ePHY_DATA_SUCCESS:             // Radio transmitted the frame
         MAC_CounterInc(eMAC_TxFrameCount);
         MAC_FrameManag_PhyDataConfirm(eMAC_DATA_SUCCESS);
         break;

      case ePHY_DATA_SERVICE_UNAVAILABLE: // PHY is not in ePHY_STATE_READY or ePHY_STATE_READY_TX
      case ePHY_DATA_TRANSMISSION_FAILED: // Radio is not configured or a deterministic TX is too far in the future or is in the past
      case ePHY_DATA_INVALID_PARAMETER:
         MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
         break;

      case ePHY_DATA_BUSY:                // PHY is already transmitting
      case ePHY_DATA_THERMAL_OVERRIDE:    // PHY can't transmit to protect HW
         MAC_CounterInc(eMAC_TxLinkDelayCount);
         TxLinkDelayTimeInc(pDataConf->delay);
         phy.Frame->SentToNextLayer = (bool)false; // Force frame to be resent
         OS_TASK_Sleep(pDataConf->delay); // This should probably check against some upper limit. We don't want to have MAC paused for an extended period of time.
         break;

      default: // Unhandled error code. Purge the frame
         MAC_FrameManag_PhyDataConfirm(eMAC_DATA_TRANSACTION_FAILED);
         break;
   }
   return (bool)true;
}


/*!
 *  Data indication from the PHY
 */
static void Process_DataIndication( const PHY_DataInd_t *phy_indication )
{
   char floatStr[PRINT_FLOAT_SIZE];

   MAC_CounterInc(eMAC_RxFrameCount);

#if ( DCU == 1 )
   // Check for STAR message
   if((phy_indication->framing == ePHY_FRAMING_1) || /* STAR Generation 1 */
      (phy_indication->framing == ePHY_FRAMING_2))   /* STAR Generation 2 */
   {
      INFO_printf("DataIndication len=%u chan=%u mode=%u framing=%u rssi=%s dBm",
         phy_indication->payloadLength,
         phy_indication->channel,
         phy_indication->mode,
         phy_indication->framing,
         DBG_printFloat(floatStr, phy_indication->rssi,1));

      // Check for STAR message
      if ( !MAC_STAR_DataIndication( phy_indication ) )
      {
         // PHY Frame Type Not supported
         INFO_printf("PHY Frame Type - Not Supported or STAR DCU-to-DCU command rejected");
      }
   }
   else
#endif
   {

#if EP == 1
      if(PWRLG_LastGasp() == false)
#endif
      {  // Normal Mode
         mac_frame_t rx_frame;
         uint32_t crc_value;

         // Check for SRFN message
         if (phy_indication->framing == ePHY_FRAMING_0) {
            crc_value = CRC_32_Calculate(phy_indication->payload, phy_indication->payloadLength);
         } else {
            // CRC always good for STAR when we reach this point.
            // A bad CRC would be rejected at the PHY
            crc_value = 0;
         }

         char* szMsg;
         if( crc_value == 0){
            szMsg = "Valid";
         }else
         {
            szMsg = "Invalid-CRC";
         }

#if NOISE_TEST_ENABLED == 1
         INFO_printf("DataIndication len=%u chan=%u mode=%u framing=%u rssi=%d dBm danl=%d dBm [%s]",
            phy_indication->payloadLength,
            phy_indication->channel,
            phy_indication->mode,
            phy_indication->framing,
            DBG_printFloat(floatStr, phy_indication->rssi,1),
            DBG_printFloat(floatStr, phy_indication->danl,1),
            szMsg );
#else
         INFO_printf("DataIndication len=%u chan=%u mode=%u modeParameters=%u framing=%u rssi=%s dBm [%s]",
            phy_indication->payloadLength,
            phy_indication->channel,
            (uint32_t)phy_indication->mode,
            (uint32_t)phy_indication->modeParameters,
            (uint32_t)phy_indication->framing,
            DBG_printFloat(floatStr, phy_indication->rssi,1),
            szMsg );
#endif

         if( 0 == crc_value )
         {
            /* frame is valid and another module has registered to consume indications */
            if (MAC_Codec_Decode ( phy_indication->payload, phy_indication->payloadLength, &rx_frame ))
            {
               // send COMM-STATUS indication
               buffer_t *indication = BM_allocStack(sizeof(MAC_Indication_t));
               if (indication != NULL)
               {
                  indication->eSysFmt = eSYSFMT_MAC_INDICATION;
                  MAC_Indication_t *macInd = (MAC_Indication_t*)indication->data; /*lint !e2445 !e826 */

                  macInd->Type = eMAC_COMM_STATUS_IND;

                  macInd->CommStatus.channel         = phy_indication->channel;
                  (void)memcpy(macInd->CommStatus.dst_addr, rx_frame.dst_addr, MAC_ADDRESS_SIZE);
                  macInd->CommStatus.dst_addr_mode   = rx_frame.dst_addr_mode;
                  macInd->CommStatus.frame_type      = rx_frame.frame_type;
                  macInd->CommStatus.network_id      = rx_frame.network_id;
                  macInd->CommStatus.payloadLength   = phy_indication->payloadLength;
                  macInd->CommStatus.rssi            = PHY_DBM_TO_MAC_SCALING( phy_indication->rssi );
                  (void)memcpy(macInd->CommStatus.src_addr, rx_frame.src_addr, MAC_ADDRESS_SIZE);
                  macInd->CommStatus.timeStamp       = phy_indication->timeStamp;

#if ( PHASE_DETECTION == 1 )
                  // This is here for debugging only
                  // This will calculate and print the phase angle for all message, not just the Survey Message
                  (void)PD_DebugCommand(phy_indication->timeStamp, rx_frame.src_addr);
#endif
                  IndicationSend(&commIndication, indication);
               }

               if(rx_frame.network_id == NetworkId_Get())
               {  // Network Id matched
#if ( EP == 1 )
                  // Updated the RX (Outbound) stats if the network id matches
#if NOISE_TEST_ENABLED == 1
                  stats_update(rx_frame.src_addr, (int16_t)phy_indication->rssi, (int16_t)phy_indication->danl);
#else
                  stats_update(rx_frame.src_addr, (int16_t) phy_indication->rssi);
#endif
                  // Print the updated stats
                  stats_dump();
#endif
                  if(0 != memcmp(rx_frame.src_addr, MacAddress_.xid, MAC_ADDRESS_SIZE)) // Filter out frames this unit transmitted
                  {
                     if( ((rx_frame.dst_addr_mode == UNICAST_MODE) &&
#if ( MULTIPLE_MAC == 0 )
                          (0 == memcmp(rx_frame.dst_addr, MacAddress_.xid, MAC_ADDRESS_SIZE))) ||  // Is this our address
#else
                          isMyMac ( rx_frame.dst_addr ))                                       ||  // Is this our address
#endif
                          (rx_frame.dst_addr_mode == BROADCAST_MODE) )                             // or a broadcast command
                     {
                        if ( rx_frame.ack_present == ACK_PRESENT )
                        {
                           INFO_printf("ERR:ACK_PRESENT Not Supported");
                           // xxx jmb:  update for this project:  MAC_FrameManag_Acknowledge ( &rx_frame );
                        }
                        /* send the frame to the data indication handler for re-assembly */
                        if( pIndicationHandler_callback != NULL )
                        {
                           (void)MAC_FrameManag_DataIndHandler(&rx_frame,
                                  phy_indication->timeStamp, // is this required?
                                  phy_indication->timeStampCYCCNT,
#if NOISE_TEST_ENABLED == 1
                                  10*phy_indication->rssi,
                                  10*phy_indication->danl,
#else
                                  phy_indication->rssi,
                                  phy_indication->danl,
#endif
                                  phy_indication->channel,
                                  phy_indication->sync_time);
                        }else
                        {
                           INFO_printf("ERR:No indication handler");
                        }
                     }else
                     {
                        INFO_printf("ERR:InvalidRecipient");
                        MAC_CounterInc(eMAC_InvalidRecipientCount);
                     }
                  } // Ignoring packet that we transmitted
               }else
               {  // Network Id did not match
                  INFO_printf("ERR:AdjacentNwk");
                  MAC_CounterInc(eMAC_AdjacentNwkFrameCount);
               }
            }else
            {  // Decode error
               INFO_printf("ERR:MalformedFrame:Decode Error");
               MAC_CounterInc(eMAC_MalformedFrameCount);
            }
         }else
         {  // CRC Error
            INFO_printf("ERR:FailedFcs");
            MAC_CounterInc(eMAC_FailedFcsCount);
         }
      }
   }
}

/***********************************************************************************************************************
Function Name: MAC_RegisterIndicationHandler

Purpose: This function is called by an upper layer module to register a callback function.  That function will accept
   an indication (data.confirmation, data.indication, etc) from this module.  That function is also responsible for
   freeing the indication buffer.

Arguments: pointer to a function of type MAC_RegisterIndicationHandler

Returns: none

Note:  Currently only one upper layer can receive indications but the function could be expanded to have an list
   of callbacks if needed.
***********************************************************************************************************************/
void MAC_RegisterIndicationHandler(MAC_IndicationHandler pCallback)
{
   pIndicationHandler_callback = pCallback;
}

/**
This function is called by an upper layer module to register a callback function.
   That function will accept an indication for COMM_STATUS from this module.
   That function is also responsible for freeing the indication buffer.

@param pCallback pointer to a function of type MAC_IndicationHandler
*/
void MAC_RegisterCommStatusIndicationHandler(MAC_IndicationHandler pCallback)
{
   (void)IndicationRegisterHandler(&commIndication, pCallback);
}

/*!
 * Used to get the Extended Unique Identifier - Address - 64-bits
 */
void MAC_EUI_Address_Get( eui_addr_t *eui_addr )
{
   (void)memcpy((uint8_t *)eui_addr, (uint8_t *)MacAddress_.eui, sizeof(eui_addr_t));
}

/*!
 * Used to set the Extended Unique Identifier - Address - 64-bits
 */
void MAC_EUI_Address_Set( eui_addr_t const *eui_addr )
{
   (void)memcpy((uint8_t *)MacAddress_.eui, (uint8_t *)eui_addr, sizeof(eui_addr_t));
// (void)FIO_fwrite( &ConfigFile.handle, 0, ConfigFile.Data, ConfigFile.Size);
}

#if 0
/*!
 * Used to get the the 40-bit Extension Address
 */
void MAC_XID_Address_Get( xid_addr_t *xid_addr )
{
   (void)memcpy(xid_addr, MAC_ConfigAttr.MacAddress.xid, sizeof(xid_addr_t));
}
#endif

/*!
 * Used to set the 40-bit Extension Address
 */
void MAC_XID_Address_Set( xid_addr_t const *xid_addr )
{
   (void)memcpy((uint8_t *)MacAddress_.xid, (uint8_t *)xid_addr, sizeof(xid_addr_t));
}


/*!
 * This function returns the 40-bit Extension Address
 */
void MAC_MacAddress_Get( uint8_t xid_addr[MAC_ADDRESS_SIZE] )
{
   (void)memcpy(xid_addr,   MacAddress_.xid, sizeof(xid_addr_t));
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
   if ( MAC_ConfigAttr.NumMac > 1 )
   {
      xid_addr[ 0 ] = (uint8_t)lastMacOffset_;
   }
#endif
}

#if 0
/*!
 * This sets the 40-bit extension address
 */
void MAC_MacAddress_Set( uint8_t const xid_addr[MAC_ADDRESS_SIZE] )
{
   (void)memcpy(MAC_ConfigAttr.MacAddress.xid, xid_addr, sizeof(xid_addr_t));
   (void)FIO_fwrite( &ConfigFile.handle, 0, ConfigFile.Data, ConfigFile.Size);
}
#endif


/***********************************************************************************************************************
Function Name: MAC_ProcessRxPacket

Purpose: This function allows hiding the indication handler pointer.  Other mac files that want to generate an
   indication to an upper layer can use this function to do so.

Arguments: pointer to a buffer_t containing an indication for an upper layer

Returns: none
***********************************************************************************************************************/
void MAC_ProcessRxPacket( buffer_t *mac_buffer, uint8_t frame_type )
{
   switch(frame_type)
   {
      case MAC_DATA_FRAME:
      {  // Data Frames are handle by the stack
         if (pIndicationHandler_callback != NULL)
         {
            // Only count packets if sent to upper layer
            MAC_CounterInc(eMAC_PacketReceivedCount);
            INFO_printf("dataLen=%d", mac_buffer->x.dataLen);
            //INFO_printHex("", (uint8_t*)mac_buffer->data, mac_buffer->x.dataLen);
            (*pIndicationHandler_callback)(mac_buffer);   // Post it to the NWK_msgQueue
         }else
         {  // No callback registered, so free this buffer
            BM_free(mac_buffer);
         }
         break;
      }
      case MAC_ACK_FRAME:
      {
         // Count ack packets as consumed
         MAC_CounterInc(eMAC_PacketConsumedCount);
         INFO_printf("ACK_FRAME - Not implemented");
         BM_free(mac_buffer);
         break;
      }
      case MAC_CMD_FRAME:
      {  // Handle a Command Frame
         // Count CMD Frames as consumed
         MAC_CounterInc(eMAC_PacketConsumedCount);
         INFO_printf("CMD_FRAME");
         MAC_DataInd_t* mac_indication = (MAC_DataInd_t*) mac_buffer->data; /*lint !e2445 !e740 !e826 */
         Process_CmdFrame(mac_indication);
         BM_free(mac_buffer);
         break;
      }
      default:
         BM_free(mac_buffer);
         break;
   }
}
/***********************************************************************************************************************
Function Name: MAC_PauseTx

Purpose: This function allows for packet prioritization testing by pausing transmission.  This lets a test pause
   transmission, send a low priority message, send a high priority message, then unpause transmission, and confirm
   messages transmit in the proper order.

Arguments: true to pause tx, false to unpause

Returns: none
***********************************************************************************************************************/
void MAC_PauseTx( bool pause )
{
   MAC_txPaused = pause;
#if ( EP == 1 )
   if( pause )
   {
#if ( MCU_SELECTION == NXP_K24 ) // TODO: RA6E1 Bob: Is this needed anymore?
      LED_setRedLedStatus(RADIO_DISABLED);
#endif // TODO: RA6E1 Bob: Is this needed anymore?
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
      HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_HOT, HMC_DISP_POS_NIC_MODE);
#endif
   }
   else
   {
#if ( MCU_SELECTION == NXP_K24 ) // TODO: RA6E1 Bob: Is this needed anymore?
      LED_setRedLedStatus(RADIO_ENABLED);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
      LED_checkModeStatus();           /* Radio got enabled replace the msg Hot with Mode */
#endif
#endif // TODO: RA6E1 Bob: Is this needed anymore?
   }
#endif
}

#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
/***********************************************************************************************************************
Function Name: MAC_IsTxPaused

Purpose: To check whether the radio is disabled or not for hot condition

Arguments: none

Returns: true or false
***********************************************************************************************************************/
bool MAC_IsTxPaused( void )
{
   return MAC_txPaused;
}
#endif

static buffer_t *ReqBuffer_Alloc(MAC_REQ_TYPE_t type, uint16_t data_size);

/*!
 ********************************************************************************************************************
 *
 * \fn ReqBuffer_Alloc()
 *
 * \brief Allocates a MAC Request Buffer
 *
 * \param type - type of buffer to allocate
 *        data_size - Size of the data for a eMAC_DATA_REQ.
 *
 * \return buffer_t
 *
 * \details This is used to allocated a buffer for a MAC Data Request
 *          If it is successful, it will set the eSysFmt to eSYSFMT_MAC_REQUEST, and return the buffer.
 *          If the type if invalid, or there are no buffers avaiable it will return NULL
 *
 ********************************************************************************************************************
 */
static buffer_t *ReqBuffer_Alloc(MAC_REQ_TYPE_t type, uint16_t data_size)
{
   uint16_t size = 0;
   char* msg = "ReqBuffer_Alloc:%s";

   switch(type)
   {
      case eMAC_GET_REQ    :    size = sizeof( MAC_GetReq_t   ); break;
      case eMAC_RESET_REQ  :    size = sizeof( MAC_ResetReq_t ); break;
      case eMAC_SET_REQ    :    size = sizeof( MAC_SetReq_t   ); break;
      case eMAC_START_REQ  :    size = sizeof( MAC_StartReq_t ); break;
      case eMAC_STOP_REQ   :    size = sizeof( MAC_StopReq_t  ); break;
#if ( EP == 1 )
      case eMAC_TIME_QUERY_REQ:    size = sizeof(MAC_TimeQueryReq_t); break;
#endif
      case eMAC_DATA_REQ   :    size = sizeof( MAC_DataReq_t  ); break;
      case eMAC_PURGE_REQ  :    size = sizeof( MAC_FlushReq_t ); break;
      case eMAC_FLUSH_REQ  :    size = sizeof( MAC_PurgeReq_t ); break;
      case eMAC_PING_REQ   :    size = sizeof( MAC_PingReq_t  ); break;
      default              :   ERR_printf(msg, "Invalid type");  break;
   }

   buffer_t *pBuf = NULL;

   if (size > 0)
   {
      // Account for the data space
      size += data_size;

      // Account for the MAC_Request Header including pad
      size += sizeof(MAC_Request_t) - sizeof( MAC_Services_u );

      pBuf = BM_allocStack( size );

      if (pBuf != NULL)
      {
         pBuf->eSysFmt       = eSYSFMT_MAC_REQUEST;
         MAC_Request_t *pMsg = (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
         pMsg->MsgType       = type;
      }else
      {
         ERR_printf(msg, "Failed");
      }
   }
   return(pBuf);
}

/*!
 ********************************************************************************************************************
 *
 * \fn MAC_RandomChannel_Get()
 *
 * \brief Used to get an random channel for transmitting
 *
 * \param channelSets     - Channel sets (STAR or SRFN) to use for channel selection
 * \param channelSetIndex - Channel set index inside a channel sets
 *
 * \return channel
 *
 * \details This function will return a random channel from the list of transmit channels.
 *
 ********************************************************************************************************************
 */
uint16_t MAC_RandomChannel_Get(MAC_CHANNEL_SETS_e channelSets, MAC_CHANNEL_SET_INDEX_e channelSetIndex)
{
   uint32_t index = 0;

   // Always re-load the tx channels
   (void)TxChannels_init(channelSets, channelSetIndex);

   // Get a random value between 0 and num_channel-1
#if ( EP == 1 )
   if(tx_channels.cnt>0)
   {
      index = aclara_randu( 0, (uint32_t)(tx_channels.cnt-1) );
   }
#else
   // DCU Always used channel index 0
#endif

   // Return the channel
   return tx_channels.channels[index];
}

/*!
 ********************************************************************************************************************
 *
 * \fn RandomBackoffDelay_Get()
 *
 * \brief Used to get a random back-off delay
 *
 * \return  value in milliseconds
 *
 * \details The value returned is in milliseconds between CsmaMinBackoffTime and CsmaMaxBackoffTime.
 *
 ********************************************************************************************************************
 */
static uint32_t RandomBackoffDelay_Get()
{
   return( aclara_randu(MAC_ConfigAttr.CsmaMinBackOffTime, MAC_ConfigAttr.CsmaMaxBackOffTime));
}

/*!
 ********************************************************************************************************************
 *
 * \fn P_Persistence_Get()
 *
 * \brief This function is used to determine if a frame will be transmitted.
 *
 * \param   void
 *
 * \return  true  - OK to transmit
 *          false -
 *
 * \details It computes a random number between 0.0 and 1.0.  If the value is <= CsmaPValue, it will return true
 *          and the frame will be transmitted.
 *
 ********************************************************************************************************************
 */
static bool P_Persistence_Get(void)
{
   bool returnVal;
   float random_val;
   char  floatStr[2][PRINT_FLOAT_SIZE];

   // Get a random number between 0.0 and 1.0
   random_val = aclara_randf( 0.0F, 1.0F );        /* float from minVal to maxVal      */
   INFO_printf("P_Persistence_Get: %s < %s", DBG_printFloat(floatStr[0], random_val, 6),
                                             DBG_printFloat(floatStr[1], MAC_ConfigAttr.CsmaPValue, 6));

   if(random_val <= MAC_ConfigAttr.CsmaPValue)
   {
      returnVal = (bool)true;
   }
   else{
      returnVal = (bool)false;
   }
   return returnVal;
}

/*!
 *********************************************************************************************************************
 *
 * \fn uint8_t NumChannelSetsSRFN_Get()
 *
 * \brief This function will return the number of SRFN channel sets
 *
 * \details This is a helper function that is used by MAC_AttributeSRFN_Get()
 *
 * \return  uint8_t number of valid channel sets
 *
 *********************************************************************************************************************
 */
static uint8_t NumChannelSetsSRFN_Get(void)
{
   uint32_t i;
   uint8_t NumChannelSets = 0;
   for(i=0;i<MAC_MAX_CHANNEL_SETS_SRFN;i++) {
      if(MAC_ConfigAttr.ChannelSetsSRFN[i].start != PHY_INVALID_CHANNEL) {
         NumChannelSets++;
      }
   }
   return NumChannelSets;
}

/*!
 *********************************************************************************************************************
 *
 * \fn uint8_t NumChannelSetsSTAR_Get()
 *
 * \brief This function will return the number of STAR channel sets
 *
 * \details This is a helper function that is used by MAC_AttributeSTAR_Get()
 *
 * \return  uint8_t number of valid channel sets
 *
 *********************************************************************************************************************
 */
static uint8_t NumChannelSetsSTAR_Get(void)
{
   uint32_t i;
   uint8_t NumChannelSets = 0;
   for(i=0;i<MAC_MAX_CHANNEL_SETS_STAR;i++) {
      if(MAC_ConfigAttr.ChannelSetsSTAR[i].start != PHY_INVALID_CHANNEL) {
         NumChannelSets++;
      }
   }
   return NumChannelSets;
}


/*!
 ********************************************************************************************************************
 *
 * \fn MAC_GET_STATUS_e MAC_Attribute_Get(MAC_GetReq_t const *pGetReq, MAC_ATTRIBUTES_u *val)
 *
 * \brief Used to read a MAC attribute
 *
 * \param  pGetReq
 * \param  *val
 *
 * \return  MAC_GET_STATUS_e
 *    eMAC_GET_SUCCESS
 *    eMAC_GET_UNSUPPORTED
 *    eMAC_GET_ERROR
 *
 * \details  The value is returned in val.
 *
 ********************************************************************************************************************
 */
MAC_GET_STATUS_e MAC_Attribute_Get( MAC_GetReq_t const *pGetReq, MAC_ATTRIBUTES_u *val)
{
   MAC_GET_STATUS_e eStatus = eMAC_GET_SUCCESS;

   // This function should only be called inside the MAC task
   if ( 0 != strcmp("MAC", OS_TASK_GetTaskName()) ) {
     ERR_printf("WARNING: MAC_Attribute_Get should only be called inside the MAC task. Please use MAC_GetRequest instead.");
   }

   switch (pGetReq->eAttribute)
   {
      case eMacAttr_AcceptedFrameCount:            val->AcceptedFrameCount        = CachedAttr.AcceptedFrameCount;                  break;
      case eMacAttr_AckWaitDuration:               val->AckWaitDuration           = MAC_ConfigAttr.AckWaitDuration;                 break;
      case eMacAttr_AckDelayDuration:              val->AckDelayDuration          = MAC_ConfigAttr.AckDelayDuration;                break;
      case eMacAttr_AdjacentNwkFrameCount:         val->AdjacentNwkFrameCount     = CachedAttr.AdjacentNwkFrameCount;               break;
      case eMacAttr_ArqOverflowCount:              val->ArqOverflowCount          = CachedAttr.ArqOverflowCount;                    break;
      case eMacAttr_ChannelAccessFailureCount:     val->ChannelAccessFailureCount = CachedAttr.ChannelAccessFailureCount;           break;
      case eMacAttr_ChannelSetsSRFN:
         {  // Return channels sets list
            (void)memcpy( val->ChannelSets, MAC_ConfigAttr.ChannelSetsSRFN, sizeof(CH_SETS_t)*NumChannelSetsSRFN_Get() );
         }
         break;
      case eMacAttr_ChannelSetsSTAR:
         {  // Return channels sets list
            (void)memcpy( val->ChannelSets, MAC_ConfigAttr.ChannelSetsSTAR, sizeof(CH_SETS_t)*NumChannelSetsSTAR_Get() );
         }
      break;
      case eMacAttr_ChannelSetsCountSRFN:
         {  // Count the number of valid channel sets
            val->ChannelSetsCount = NumChannelSetsSRFN_Get();
         }
         break;
      case eMacAttr_ChannelSetsCountSTAR:
         {  // Count the number of valid channel sets
            val->ChannelSetsCount = NumChannelSetsSTAR_Get();
         }
      break;
      case eMacAttr_CsmaMaxAttempts:               val->CsmaMaxAttempts           = MAC_ConfigAttr.CsmaMaxAttempts;                 break;
      case eMacAttr_CsmaMaxBackOffTime:            val->CsmaMaxBackOffTime        = MAC_ConfigAttr.CsmaMaxBackOffTime;              break;
      case eMacAttr_CsmaMinBackOffTime:            val->CsmaMinBackOffTime        = MAC_ConfigAttr.CsmaMinBackOffTime;              break;
      case eMacAttr_CsmaPValue:                    val->CsmaPValue                = MAC_ConfigAttr.CsmaPValue;                      break;
      case eMacAttr_CsmaQuickAbort:                val->CsmaQuickAbort            = MAC_ConfigAttr.CsmaQuickAbort;                  break;
#if ( EP == 1 ) && ( TEST_TDMA == 1)
      case eMacAttr_CsmaSkip:                      val->CsmaSkip                  = MAC_ConfigAttr.CsmaSkip;                        break;
#endif
      case eMacAttr_DuplicateFrameCount:           val->DuplicateFrameCount       = CachedAttr.DuplicateFrameCount;                 break;
      case eMacAttr_FailedFcsCount:                val->FailedFcsCount            = CachedAttr.FailedFcsCount;                      break;
      case eMacAttr_InvalidRecipientCount:         val->InvalidRecipientCount     = CachedAttr.InvalidRecipientCount;               break;
      case eMacAttr_LastResetTime:                 val->LastResetTime             = CachedAttr.LastResetTime;                       break;
      case eMacAttr_MalformedAckCount:             val->MalformedAckCount         = CachedAttr.MalformedAckCount;                   break;
      case eMacAttr_MalformedFrameCount:           val->MalformedFrameCount       = CachedAttr.MalformedFrameCount;                 break;
      case eMacAttr_NetworkId:                     val->NetworkId                 = MAC_ConfigAttr.NetworkId;                       break;
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
      case eMacAttr_NumMac:                        val->NumMac                    = MAC_ConfigAttr.NumMac;                          break;
#endif
      case eMacAttr_PacketConsumedCount:           val->PacketConsumedCount       = CachedAttr.PacketConsumedCount;                 break;
      case eMacAttr_PacketFailedCount:             val->PacketFailedCount         = CachedAttr.PacketFailedCount;                   break;
      case eMacAttr_PacketId:                      val->PacketId                  = CachedAttr.PacketId;                            break;
      case eMacAttr_PacketReceivedCount:           val->PacketReceivedCount       = CachedAttr.PacketReceivedCount;                 break;
      case eMacAttr_PacketTimeout:                 val->PacketTimeout             = MAC_ConfigAttr.PacketTimeout;                   break;
      case eMacAttr_PingCount:                     val->PingCount                 = CachedAttr.PingCount;                           break;
      case eMacAttr_ReassemblyTimeout:             val->ReassemblyTimeout         = MAC_ConfigAttr.ReassemblyTimeout;               break;
      case eMacAttr_ReliabilityHighCount:          val->ReliabilityHighCount      = MAC_ConfigAttr.ReliabilityHighCount;            break;
      case eMacAttr_ReliabilityMediumCount:        val->ReliabilityMediumCount    = MAC_ConfigAttr.ReliabilityMediumCount;          break;
      case eMacAttr_ReliabilityLowCount:           val->ReliabilityLowCount       = MAC_ConfigAttr.ReliabilityLowCount;             break;
      case eMacAttr_RxFrameCount:                  val->RxFrameCount              = CachedAttr.RxFrameCount;                        break;
      case eMacAttr_RxOverflowCount:               val->RxOverflowCount           = CachedAttr.RxOverflowCount;                     break;
      case eMacAttr_State:                         val->State                     = _state;                                         break;
      case eMacAttr_TransactionOverflowCount:      val->TransactionOverflowCount  = CachedAttr.TransactionOverflowCount;            break;
      case eMacAttr_TransactionTimeout:            val->TransactionTimeout        = MAC_ConfigAttr.TransactionTimeout;              break;
      case eMacAttr_TransactionTimeoutCount:       val->TransactionTimeoutCount   = CachedAttr.TransactionTimeoutCount;             break;
      case eMacAttr_TxFrameCount:                  val->TxFrameCount              = CachedAttr.TxFrameCount;                        break;
      case eMacAttr_TxLinkDelayCount:              val->TxLinkDelayCount          = CachedAttr.TxLinkDelayCount;                    break;
      case eMacAttr_TxLinkDelayTime:               val->TxLinkDelayTime           = CachedAttr.TxLinkDelayTime;                     break;
      case eMacAttr_TxPacketCount:                 val->TxPacketCount             = CachedAttr.TxPacketCount;                       break;
#if ( DCU == 1 )
      case eMacAttr_TxPacketDelay:                 val->TxPacketDelay             = MAC_ConfigAttr.TxPacketDelay;                   break;
#endif
      case eMacAttr_TxPacketFailedCount:           val->TxPacketFailedCount       = CachedAttr.TxPacketFailedCount;                 break;
#if ( EP == 1 ) && ( TEST_TDMA == 1)
      case eMacAttr_TxSlot:                        val->TxSlot                    = MAC_ConfigAttr.TxSlot;                          break;
#endif
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
      case eMacAttr_LinkParamMaxOffset:            val->LinkParamMaxOffset        = MAC_ConfigAttr.LinkParamMaxOffset;              break;
      case eMacAttr_LinkParamPeriod:               val->LinkParamPeriod           = LinkParameters_Period_Get();                    break;
      case eMacAttr_LinkParamStart:                val->LinkParamStart            = MAC_ConfigAttr.LinkParamStart;                  break;
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
      case eMacAttr_CmdRespMaxTimeDiversity:       val->macCmdRespMaxTimeDiversity = MAC_ConfigAttr.macCmdRespMaxTimeDiversity;     break;
#endif
      case eMacAttr_IsChannelAccessConstrained:    val->IsChannelAccessConstrained = MAC_ConfigAttr.IsChannelAccessConstrained; break;
      case eMacAttr_IsFNG:                         val->IsFNG                      = MAC_ConfigAttr.IsFNG;                      break;
      case eMacAttr_IsIAG:                         val->IsIAG                      = MAC_ConfigAttr.IsIAG;                      break;
      case eMacAttr_IsRouter:                      val->IsRouter                   = MAC_ConfigAttr.IsRouter;                   break;

      default:
         eStatus = eMAC_GET_UNSUPPORTED;
         break;
   }

   if (eStatus != eMAC_GET_SUCCESS)
   {
      ERR_printf("MAC_Attribute_Get:Attr=%s Status=%s", mac_attr_names[pGetReq->eAttribute], MAC_GetStatusMsg(eStatus));
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e NetworkId_Set(uint8_t NetworkId)
 *
 * \brief private API to Set Network Id (assumes called from MAC_Attribute_Set)
 *
 * \param  NetworkId (0-15)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e NetworkId_Set( uint8_t NetworkId)
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if (NetworkId < 16)
   {
      MAC_ConfigAttr.NetworkId = NetworkId;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

#if ( ( MULTIPLE_MAC != 0 ) && ( EP == 1 ) )
/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e NumMacSet(uint8_t NumMac)
 *
 * \brief private API to Set NumMac (assumes called from MAC_Attribute_Set)
 *
 * \param  NumMac (1-50)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e NumMacSet( uint8_t NumMac )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if ( ( NumMac >= 1 ) && ( NumMac <= 50) )
   {
      MAC_ConfigAttr.NumMac = NumMac;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}
#endif


/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e ChannelSets_Set
 *
 * \brief Set the channel sets
 *
 * \param  channelsSets - Update channel sets list
 *         dest         - Where to save the updated list
 *         maxChannelSets - Maximum number of sets
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e ChannelSets_Set( CH_SETS_t const *ChannelSets, void * dest, uint32_t maxChannelSets)
{
   uint32_t i; // Loop counter
   bool valid = (bool)true;
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;

   // Check that all channels are in the valid range
   for (i = 0; i < maxChannelSets; i++) {
      if ( ChannelSets[i].start > PHY_INVALID_CHANNEL ) {
         valid = (bool)false;
         break;
      }
      if ( ChannelSets[i].stop > PHY_INVALID_CHANNEL ) {
         valid = (bool)false;
         break;
      }
   }

   if ( valid ) {
      // Check that if one channel is invalid then both channel are invalid
      for (i = 0; i < maxChannelSets; i++) {
         if ( (ChannelSets[i].start == PHY_INVALID_CHANNEL) && (ChannelSets[i].stop != PHY_INVALID_CHANNEL) ) {
            valid = (bool)false;
            break;
         }
         if ( (ChannelSets[i].start != PHY_INVALID_CHANNEL) && (ChannelSets[i].stop == PHY_INVALID_CHANNEL) ) {
            valid = (bool)false;
            break;
         }
      }
   }

   if ( valid ) {
      // Check that the stop channel is greater than or equal to start channel
      for (i = 0; i < maxChannelSets; i++) {
         if ( ChannelSets[i].start > ChannelSets[i].stop ) {
            valid = (bool)false;
            break;
         }
      }
   }

   if ( valid ) {
      // Check that there is no gap in channel sets (e.g. chan A, chan B, gap, gap, chan C, chan D)
      // Find first instance of empty sets
      for (i = 0; i < maxChannelSets; i++) {
         if ( ChannelSets[i].start == PHY_INVALID_CHANNEL ) {
            break;
         }
      }
      // Find if there is a valid sets after an empty sets
      for (; i < maxChannelSets; i++) {
         if ( ChannelSets[i].start != PHY_INVALID_CHANNEL ) {
            valid = (bool)false; // Found a valid set after an empty set
            break;
         }
      }
   }

   if ( valid ) {
      (void)memcpy( (CH_SETS_t *)dest, ChannelSets, sizeof(CH_SETS_t)*maxChannelSets );
      eStatus = eMAC_SET_SUCCESS;
   }

   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e CsmaMaxAttempts_Set(uint8_t CsmaMaxAttempts)
 *
 * \brief Set the Maximum Csma Attempts
 *
 * \param  CsmaMaxAttempts (0-20)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e CsmaMaxAttempts_Set( uint8_t  CsmaMaxAttempts )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;

   if (CsmaMaxAttempts <= MAC_CSMA_MAX_ATTEMPTS_LIMIT)
   {
      MAC_ConfigAttr.CsmaMaxAttempts          = CsmaMaxAttempts;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e CsmaMaxBackOffTime_Set(uint16_t CsmaMaxBackOffTime_msec)
 *
 * \brief Set the Maximum CSMA Backoff Time (msec)
 *
 * \param  CsmaMaxBackoffTime (MAC_ConfigAttr.CsmaMinBackoffTime to MAC_CSMA_MAX_ATTEMPTS)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e CsmaMaxBackOffTime_Set( uint16_t CsmaMaxBackOffTime_msec )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if((CsmaMaxBackOffTime_msec     >= MAC_ConfigAttr.CsmaMinBackOffTime ) &&
      (CsmaMaxBackOffTime_msec     <= MAC_CSMA_MAX_BACKOFF_TIME_LIMIT ))
   {
      MAC_ConfigAttr.CsmaMaxBackOffTime       = CsmaMaxBackOffTime_msec;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e CsmaMinBackOffTime_Set(uint16_t CsmaMinBackOffTime_msec)
 *
 * \brief Set the Minimum CSMA Backoff Time (msec)
 *
 * \param  CsmaMinBackoffTime (0 to min(100,CsmaMaxBackOffTime)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e CsmaMinBackOffTime_Set( uint16_t CsmaMinBackOffTime_msec )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if( ( CsmaMinBackOffTime_msec <= MAC_ConfigAttr.CsmaMaxBackOffTime ) &&
       ( CsmaMinBackOffTime_msec <= MAC_CSMA_MIN_BACKOFF_TIME_LIMIT ) )
   {
      MAC_ConfigAttr.CsmaMinBackOffTime       = CsmaMinBackOffTime_msec;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e CsmaPValue_Set(float CsmaPValue)
 *
 * \brief Set the CSMA Probabity Value
 *
 * \param  CsmaPValue (0.0 to 1.0)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static MAC_SET_STATUS_e CsmaPValue_Set( float    CsmaPValue )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;

   if((CsmaPValue             >= 0.0F) &&
      (CsmaPValue             <= 1.0F))
   {
      MAC_ConfigAttr.CsmaPValue               = CsmaPValue;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e CsmaQuickAbort_Set( bool Value )
 *
 * \brief Set the CsmaQuickAbort_Set
 *
 * \param  CsmaPValue (true or false)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static MAC_SET_STATUS_e CsmaQuickAbort_Set( bool Value )
{
   MAC_ConfigAttr.CsmaQuickAbort  = Value;
   return eMAC_SET_SUCCESS;
}

#if ( EP == 1 ) && ( TEST_TDMA == 1 )
/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e CsmaSkip_Set( uint8_t Value )
 *
 * \brief Set the CsmaSkip_Set
 *
 * \param  CsmaSkip
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static MAC_SET_STATUS_e CsmaSkip_Set( uint8_t Value )
{
   MAC_ConfigAttr.CsmaSkip = Value;
   return eMAC_SET_SUCCESS;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e TxSlot_Set( uint8_t Value )
 *
 * \brief Set the TxSlot_Set
 *
 * \param  TxSlot
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static MAC_SET_STATUS_e TxSlot_Set( uint8_t Value )
{
   MAC_ConfigAttr.TxSlot = Value;
   return eMAC_SET_SUCCESS;
}
#endif

/** *****************************************************************************************************************
 *
 ** *****************************************************************************************************************/
static MAC_SET_STATUS_e IsFNG_Set( bool Value )
{
   MAC_ConfigAttr.IsFNG  = Value;
   return eMAC_SET_SUCCESS;
}

/** *****************************************************************************************************************
 *
 ** *****************************************************************************************************************/
static MAC_SET_STATUS_e IsIAG_Set( bool Value )
{
   MAC_ConfigAttr.IsIAG  = Value;
   return eMAC_SET_SUCCESS;
}

/** *****************************************************************************************************************
 *
 ** *****************************************************************************************************************/
 static MAC_SET_STATUS_e IsRouter_Set( bool Value )
{
   MAC_ConfigAttr.IsRouter  = Value;
   return eMAC_SET_SUCCESS;
}



/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e PacketTimeout_Set(uint16_t PacketTimeOut_msec)
 *
 * \brief Set the Packet Timeout
 *
 * \param  PacketTimeOut (0-65535 msec)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e PacketTimeout_Set( uint16_t PacketTimeout_msec )
{
   MAC_ConfigAttr.PacketTimeout = PacketTimeout_msec;
   return eMAC_SET_SUCCESS;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e ReassemblyTimeout_Set(uint8_t ReassemblyTimeout_msec)
 *
 * \brief Set the Reassembly Timeout
 *
 * \param  ReassemblyTimeout (1-60 sec)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e ReassemblyTimeout_Set( uint8_t  ReassemblyTimeout_sec )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if ((ReassemblyTimeout_sec >   0) &&
       (ReassemblyTimeout_sec <= 60))
   {
      MAC_ConfigAttr.ReassemblyTimeout        = ReassemblyTimeout_sec;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e ReliabilityHighCount_Set(uint8_t ReliabilityHighCount)
 *
 * \brief Set the High Reliability retry count
 *
 * \param  ReliabilityHighCount (0-7)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e ReliabilityHighCount_Set( uint8_t  ReliabilityHighCount )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if (ReliabilityHighCount   <= MAC_RELIABILITY_RANGE_MAX)
   {
      MAC_ConfigAttr.ReliabilityHighCount     = ReliabilityHighCount;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e ReliabilityMediumCount_Set(uint8_t ReliabilityMediumCount)
 *
 * \brief Set the Medium Reliability retry count
 *
 * \param  ReliabilityMediumCount (0-7)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e ReliabilityMediumCount_Set( uint8_t  ReliabilityMediumCount )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if (ReliabilityMediumCount <= MAC_RELIABILITY_RANGE_MAX)
   {
      MAC_ConfigAttr.ReliabilityMediumCount   = ReliabilityMediumCount;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e ReliabilityLowCount_Set(uint8_t ReliabilityLowCount)
 *
 * \brief Set the Low Reliability retry count
 *
 * \param  ReliabilityLowCount (0-7)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e ReliabilityLowCount_Set( uint8_t  ReliabilityLowCount )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if (ReliabilityLowCount <= MAC_RELIABILITY_RANGE_MAX)
   {
      MAC_ConfigAttr.ReliabilityLowCount   = ReliabilityLowCount;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

#if ( DCU == 1 )
/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e TxPacketDelay_Set( uint16_t  PacketDelay )
 *
 * \brief Set the TxPacketDelay
 *
 * \param  TxPacketDelay (0-10,000)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e TxPacketDelay_Set( uint16_t  PacketDelay )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if (PacketDelay <= 10000)
   {
      MAC_ConfigAttr.TxPacketDelay   = PacketDelay ;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e TransactionTimeout( uint16_t  Timeout )
 *
 * \brief Set the Transaction Timeout
 *
 * \param  timeout (1-300)
 *
 * \return  MAC_SET_STATUS_e
 *
 ********************************************************************************************************************
 */
static
MAC_SET_STATUS_e TransactionTimeout_Set( uint16_t  timeout )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if ( timeout && (timeout <= 300) )
   {
      MAC_ConfigAttr.TransactionTimeout = timeout ;
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e LinkParameters_MaxOffset_Set( uint16_t  Timeout )
 *
 * \brief Set the Link Parameter MaxOffset
 *
 * \param  maxOffset ( 0 - 20000 ) in mili-seconds
 *
 * \return  MAC_SET_STATUS_e
 *
 *********************************************************************************************************************/
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
static MAC_SET_STATUS_e LinkParameters_MaxOffset_Set( uint16_t  maxOffset )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if ( maxOffset <= MAC_LINK_PARAM_MAX_OFFSET_MAX )
   {
      MAC_ConfigAttr.LinkParamMaxOffset = maxOffset;
      LinkParameters_ManageAlarm();
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e LinkParameters_Period_Set( uint32_t  val )
 *
 * \brief Set the Link Parameter Period
 *
 * \param  val in seconds
 *
 * \return  MAC_SET_STATUS_e
 *
 ***********************************************************************************************************************/
static MAC_SET_STATUS_e LinkParameters_Period_Set( uint32_t val )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;

   if ( val ==     0 ||
        val ==   900 ||    // 00:15
        val ==  1800 ||    // 00:30
        val ==  3600 ||    // 01:00
        val ==  7200 ||    // 02:00
        val == 10800 ||    // 03:00
        val == 14400 ||    // 04:00
        val == 21600 ||    // 06:00
        val == 28800 ||    // 08:00
        val == 43200 ||    // 12:00
        val == 86400 )     // 24:00
   {
      MAC_ConfigAttr.LinkParamPeriod = val ;
      LinkParameters_ManageAlarm();
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e LinkParameters_Start_Set( uint16_t  Timeout )
 *
 * \brief Set the Link Parameter Start
 *
 * \param  start ( 0 - 86399 ) seconds
 *
 * \return  MAC_SET_STATUS_e
 *
 *********************************************************************************************************************/
static MAC_SET_STATUS_e LinkParameters_Start_Set( uint32_t start )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if ( start <= MAC_LINK_PARAM_START_MAX )
   {
      MAC_ConfigAttr.LinkParamStart = start;
      LinkParameters_ManageAlarm();
      eStatus = eMAC_SET_SUCCESS;
   }
   return eStatus;
}

#endif // end of ( ( DCU == 1 ) && ( MAC_LINK_PARAMETERS == 1 ) ) section


/*! ********************************************************************************************************************
 *
 * \fn MAC_SET_STATUS_e MAC_Attribute_Set(MAC_SetReq_t const *pSetReq)
 *
 * \brief Used to set a MAC attribute
 *
 * \param  pSetReq
 *
 * \return  MAC_SET_STATUS_e
 *    ePHY_SET_SUCCESS
 *    ePHY_SET_READONLY
 *    ePHY_SET_UNSUPPORTED
 *    ePHY_SET_INVALID_PARAMETER
 *    ePHY_SET_ERROR
 *
 * \details
 *
 ********************************************************************************************************************
 */

static MAC_SET_STATUS_e  MAC_Attribute_Set( MAC_SetReq_t const *pSetReq)
{
   MAC_SET_STATUS_e eStatus;

   // This function should only be called inside the MAC task
   if ( 0 != strcmp("MAC", OS_TASK_GetTaskName()) ) {
     ERR_printf("WARNING: MAC_Attribute_Set should only be called inside the MAC task. Please use MAC_Attribute_Set instead.");
   }

   switch (pSetReq->eAttribute)  /*lint !e788 not all enums used within switch */
   {
      // Writable Attributes
      case eMacAttr_AcceptedFrameCount:            eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_AckDelayDuration:              eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_AckWaitDuration:               eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_AdjacentNwkFrameCount:         eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_ArqOverflowCount:              eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_ChannelAccessFailureCount:     eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_ChannelSetsSRFN:               eStatus = ChannelSets_Set(                pSetReq->val.ChannelSets, MAC_ConfigAttr.ChannelSetsSRFN, MAC_MAX_CHANNEL_SETS_SRFN ); break;
      case eMacAttr_ChannelSetsSTAR:               eStatus = ChannelSets_Set(                pSetReq->val.ChannelSets, MAC_ConfigAttr.ChannelSetsSTAR, MAC_MAX_CHANNEL_SETS_STAR ); break;
      case eMacAttr_ChannelSetsCountSRFN:          eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_ChannelSetsCountSTAR:          eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_CsmaMaxAttempts:               eStatus = CsmaMaxAttempts_Set(            pSetReq->val.CsmaMaxAttempts          ); break;
      case eMacAttr_CsmaMaxBackOffTime:            eStatus = CsmaMaxBackOffTime_Set(         pSetReq->val.CsmaMaxBackOffTime       ); break;
      case eMacAttr_CsmaMinBackOffTime:            eStatus = CsmaMinBackOffTime_Set(         pSetReq->val.CsmaMinBackOffTime       ); break;
      case eMacAttr_CsmaPValue:                    eStatus = CsmaPValue_Set(                 pSetReq->val.CsmaPValue               ); break;
      case eMacAttr_CsmaQuickAbort:                eStatus = CsmaQuickAbort_Set(             pSetReq->val.CsmaQuickAbort           ); break;
#if ( EP == 1 ) && ( TEST_TDMA == 1)
      case eMacAttr_CsmaSkip:                      eStatus = CsmaSkip_Set(                   pSetReq->val.CsmaSkip                 ); break;
#endif
      case eMacAttr_DuplicateFrameCount:           eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_FailedFcsCount:                eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_InvalidRecipientCount:         eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_LastResetTime:                 eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_MalformedAckCount:             eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_MalformedFrameCount:           eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_NetworkId:                     eStatus = NetworkId_Set(                  pSetReq->val.NetworkId);                 break;
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
      case eMacAttr_NumMac:                        eStatus = NumMacSet(                      pSetReq->val.NumMac );                   break;
#endif
      case eMacAttr_PacketConsumedCount:           eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_PacketFailedCount:             eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_PacketId:                      eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_PacketReceivedCount:           eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_PacketTimeout:                 eStatus = PacketTimeout_Set(              pSetReq->val.PacketTimeout            ); break;
      case eMacAttr_PingCount:                     eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_ReassemblyTimeout:             eStatus = ReassemblyTimeout_Set(          pSetReq->val.ReassemblyTimeout        ); break;
      case eMacAttr_ReliabilityHighCount:          eStatus = ReliabilityHighCount_Set(       pSetReq->val.ReliabilityHighCount     ); break;
      case eMacAttr_ReliabilityMediumCount:        eStatus = ReliabilityMediumCount_Set(     pSetReq->val.ReliabilityMediumCount   ); break;
      case eMacAttr_ReliabilityLowCount:           eStatus = ReliabilityLowCount_Set(        pSetReq->val.ReliabilityLowCount      ); break;
      case eMacAttr_RxFrameCount:                  eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_RxOverflowCount:               eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_State:                         eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_TransactionOverflowCount:      eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_TransactionTimeout:            eStatus = TransactionTimeout_Set(         pSetReq->val.TransactionTimeout       ); break;
      case eMacAttr_TransactionTimeoutCount:       eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_TxFrameCount:                  eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_TxLinkDelayCount:              eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_TxLinkDelayTime:               eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_TxPacketCount:                 eStatus = eMAC_SET_READONLY; break;
#if ( EP == 1 ) && ( TEST_TDMA == 1)
      case eMacAttr_TxSlot:                        eStatus = TxSlot_Set(                     pSetReq->val.TxSlot                   ); break;
#endif
      case eMacAttr_IsChannelAccessConstrained:    eStatus = eMAC_SET_READONLY; break;
      case eMacAttr_IsFNG:                         eStatus = IsFNG_Set   (  pSetReq->val.IsFNG                    ); break;
      case eMacAttr_IsIAG:                         eStatus = IsIAG_Set   (  pSetReq->val.IsIAG                    ); break;
      case eMacAttr_IsRouter:                      eStatus = IsRouter_Set(  pSetReq->val.IsRouter                 ); break;

#if ( DCU == 1 )
      case eMacAttr_TxPacketDelay:                 eStatus = TxPacketDelay_Set(              pSetReq->val.TxPacketDelay            ); break;
#endif
      case eMacAttr_TxPacketFailedCount:           eStatus = eMAC_SET_READONLY; break;
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
      case eMacAttr_LinkParamMaxOffset:            eStatus = LinkParameters_MaxOffset_Set(     pSetReq->val.LinkParamMaxOffset     ); break;
      case eMacAttr_LinkParamPeriod:               eStatus = LinkParameters_Period_Set(        pSetReq->val.LinkParamPeriod        ); break;
      case eMacAttr_LinkParamStart:                eStatus = LinkParameters_Start_Set(         pSetReq->val.LinkParamStart         ); break;
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
      case eMacAttr_CmdRespMaxTimeDiversity:       eStatus = macCommandResponse_MaxTimeDiversity_Set(pSetReq->val.macCmdRespMaxTimeDiversity); break;
#endif
      default:
         eStatus   = eMAC_SET_UNSUPPORTED ;
         break;
   }  /*lint !e788 not all enums used within switch */
#if ( DCU == 1 )
   // Notify the MB that some configuration changed
   if ((VER_getDCUVersion() != eDCU2) && (eStatus == eMAC_SET_SUCCESS)) {
      switch(pSetReq->eAttribute)   /*lint !e788 not all enums used within switch */
      {
         case eMacAttr_ChannelSetsSRFN:
         case eMacAttr_ChannelSetsSTAR:
            MAINBD_TB_Config_Changed();
            break;

         default:
            break;
      }//lint !e788
   }
#endif

   if (eStatus == eMAC_SET_SUCCESS)
   {
#if EP == 1
      // Only allow configuration changes if NOT in Last Gasp Mode
      if( PWRLG_LastGasp() == false )
#endif
      {
         file_t *pFile = (file_t *)Files[0];
         (void)FIO_fwrite( &pFile->handle, 0, pFile->Data, pFile->Size);
      }
   }else
   {
      ERR_printf("MAC_Attribute_Set:Attr=%s Status=%s", mac_attr_names[pSetReq->eAttribute], MAC_SetStatusMsg(eStatus));
   }
   return eStatus;
}

/***********************************************************************************************************************
Function Name: MAC_ReliabilityRetryCount_Get

Purpose: This function returns the number of times to retry a packet based upun the reliability level.

Arguments: reliability level

Returns: number of times to transmit the packet based on reliability.  If an unknown reliability value is entered, the
            count for MAC_RELIABILITY_LOW_RETRY_COUNT is returned.
***********************************************************************************************************************/
uint8_t MAC_ReliabilityRetryCount_Get ( MAC_Reliability_e reliability )
{
   uint8_t retryCount;

   if ( eMAC_RELIABILITY_HIGH == reliability )
   {
      retryCount = MAC_ConfigAttr.ReliabilityHighCount;
   }
   else if ( eMAC_RELIABILITY_MED == reliability )
   {
      retryCount = MAC_ConfigAttr.ReliabilityMediumCount;
   }
   else // if ( eMAC_RELIABILITY_LOW == reliability )
   {
      retryCount = MAC_ConfigAttr.ReliabilityLowCount;
   }
   return retryCount;
}

/***********************************************************************************************************************
 *
 * Function Name: NetworkId_Get
 *
 * Purpose: Used to get the configured network id
 *
 * Arguments: none
 *
 * Returns: networkId
 *
 * NOTE: This is to be called by the MAC task only
 *
 * ***********************************************************************************************************************/
uint8_t NetworkId_Get(void)
{
   return MAC_ConfigAttr.NetworkId;
}

/***********************************************************************************************************************
 *
 * Function Name: MAC_NetworkId_Get
 *
 * Purpose: Used to get the configured network id
 *
 * Arguments: none
 *
 * Returns: networkId
 *
 * NOTE: This is to be called by tasks other than MAC task
 *
 * ***********************************************************************************************************************/
uint8_t MAC_NetworkId_Get(void)
{
   MAC_GetConf_t GetConf;
   GetConf = MAC_GetRequest( eMacAttr_NetworkId );

   return GetConf.val.NetworkId;
}

/***********************************************************************************************************************
 *
 * Function Name: TxChannels_init
 *
 * Purpose: Used to get a count and list of the TX Channels from the PHY
 *          The list is filtered according to the channelSet
 *
 * Arguments: channelSets     - Channel sets (STAR or SRFN) to use for channel selection
 *            channelSetIndex - Channel set index inside a channel sets
 *
 * Returns: True if a TX channel exists. False otherwise
 *
 * ***********************************************************************************************************************/
bool TxChannels_init(MAC_CHANNEL_SETS_e channelSets, MAC_CHANNEL_SET_INDEX_e channelSetIndex)
{
   uint8_t       i; // Loop counter
   PHY_GetConf_t GetConf;
   uint8_t       numChannels = 0;
   CH_SETS_t     filter = { 0, PHY_INVALID_CHANNEL-1 }; // Default filter includes all channels

   tx_channels.cnt = 0;

   // Get the number of available channels
   GetConf = PHY_GetRequest( ePhyAttr_NumChannels );
   if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
      numChannels = GetConf.val.NumChannels;

      // Get the list of Tx Channels
      GetConf = PHY_GetRequest( ePhyAttr_TxChannels );
      if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
         // Check if we have at least one TX channel
         if (PHY_IsChannelValid(GetConf.val.TxChannels[0])) {
            // Check if we need to filter the TX list at all
            if (channelSetIndex == eMAC_CHANNEL_SET_INDEX_ANY) {
               // Just return the whole TX list
               for(i=0;i<numChannels;i++)
               {
                  if(PHY_IsChannelValid(GetConf.val.TxChannels[i]))
                  {
                     tx_channels.channels[tx_channels.cnt] = GetConf.val.TxChannels[i];
                     tx_channels.cnt++;
                  }
               }
            }
            // Check if we have one and only one TX channel
            // In that case, we don't care about channel filtering (MAC channel sets)
            else if (PHY_IsChannelValid(GetConf.val.TxChannels[1])) {
               // We have at least 2 TX channels, we need to filter

               channelSetIndex--; // Convert from 1 based to 0 based

               // Filter TX channels
               // Sanity check
               if ( channelSets < eMAC_CHANNEL_SETS_LAST ) {
                  if ( channelSets == eMAC_CHANNEL_SETS_SRFN ) {
                     filter = MAC_ConfigAttr.ChannelSetsSRFN[channelSetIndex]; // Get channel set limits
                  } else if ( channelSets == eMAC_CHANNEL_SETS_STAR ) {
                     filter = MAC_ConfigAttr.ChannelSetsSTAR[channelSetIndex]; // Get channel set limits
                  }
                  for(i=0;i<numChannels;i++)
                  {
                     if(PHY_IsChannelValid(GetConf.val.TxChannels[i]))
                     {
                        // Filter TX channel based on channel set
                        if ( (GetConf.val.TxChannels[i] >= filter.start) && (GetConf.val.TxChannels[i] <= filter.stop) ) {
                           tx_channels.channels[tx_channels.cnt] = GetConf.val.TxChannels[i];
                           tx_channels.cnt++;
                        }
                     }
                  }
               }
            } else {
               // Only one TX channel is available. Use it regardless of MAC channel sets settings
               tx_channels.channels[0] = GetConf.val.TxChannels[0];
               tx_channels.cnt = 1;
            }
         }

         // This is just for information
         if ( tx_channels.cnt ) {
            for(i=0;i<tx_channels.cnt;i++)
            {
               INFO_printf("tx_chan[%u] = %u", i, tx_channels.channels[i]);
            }
         } else {
            ERR_printf("ERROR: No TX channel available");
         }
      }
   }

   // This is just for information
   INFO_printf("Total channels = %u", numChannels);

   return ( tx_channels.cnt != 0 );  /* Returns correct bool value */
}

/***********************************************************************************************************************
 *
 * Function Name: MAC_NetworkId_Set
 *
 * Purpose: Public external interface to set the network id
 *
 * Arguments: none
 *
 * Returns: none
 *
 * ***********************************************************************************************************************/
void MAC_NetworkId_Set(uint8_t NetworkId)
{
   ( void )MAC_SetRequest( eMacAttr_NetworkId, &NetworkId );
}

/***********************************************************************************************************************
 *
 * Function Name: MAC_CsmaParameters_Print()
 *
 * Purpose: Used to print the csma parameters
 *
 * Arguments: none
 *
 * Returns: void
 *
 * ***********************************************************************************************************************/
void MAC_CsmaParameters_Print()
{
   char    floatStr[PRINT_FLOAT_SIZE];

   INFO_printf("CsmaMaxAttempts   = %d",    MAC_ConfigAttr.CsmaMaxAttempts    );
   INFO_printf("CsmaMaxBackOffTime= %d",    MAC_ConfigAttr.CsmaMaxBackOffTime );
   INFO_printf("CsmaMinBackOffTime= %d",    MAC_ConfigAttr.CsmaMinBackOffTime );
   INFO_printf("CsmaPValue        = %s",    DBG_printFloat(floatStr, MAC_ConfigAttr.CsmaPValue, 2));
}

#if ( EP == 1 )
/***********************************************************************************************************************
 *
 * Function Name: TimeReq_Done(void)
 *
 * Purpose: This is the call-back function for when the TimeReq is completed.
 *
 * Arguments:
 *
 * Returns: void
 *
 * ***********************************************************************************************************************/
static void TimeReq_Done(MAC_DATA_STATUS_e status, uint16_t handle)
{
   INFO_printf("TimeReq_Done[%d]:%s", handle, MAC_DataStatusMsg(status));
}
#endif

/***********************************************************************************************************************
 *
 * Function Name: PingRsp_Create
 *
 * Purpose: This function is used to create a Ping Response message
 *
 * Arguments:
 *
 * Returns: buffer_t* - The allocated buffer with the response.
 *
 * Note: this function assumes the calling function already checked comDeviceGatewayConfig != star
 *
 * ***********************************************************************************************************************/
static buffer_t *PingRsp_Create(
   MAC_dataConfCallback callback,
   uint8_t              dst_addr_mode,               /* Destination Address Mode */
   uint8_t    const     dst_addr[MAC_ADDRESS_SIZE],  /* Destination Address */
   ping_rsp_t const     *ping_req)
{
   (void) callback;

#if NOISE_TEST_ENABLED == 1
   #define PING_RSP_CMD_SIZE 40
#else
   #define PING_RSP_CMD_SIZE 38
#endif
#if BOB_PADDED_PING == 1
   uint16_t padLength = ping_req->ExtraRspBytes;
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_DATA_REQ, PING_RSP_CMD_SIZE + padLength);
#else
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_DATA_REQ, PING_RSP_CMD_SIZE);
#endif
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */

      pReq->pConfirmHandler = NULL;
      pReq->Service.DataReq.handle          = 0;
      pReq->Service.DataReq.dst_addr_mode   = dst_addr_mode;
      (void) memcpy(pReq->Service.DataReq.dst_addr, dst_addr, MAC_ADDRESS_SIZE);

      pReq->Service.DataReq.callback        = NULL;
      pReq->Service.DataReq.ackRequired     = (bool)false;
      pReq->Service.DataReq.droppable       = (bool)true;
      pReq->Service.DataReq.reliability     = eMAC_RELIABILITY_LOW;
      pReq->Service.PingReq.priority        = 7;
      pReq->Service.DataReq.channelSets     = eMAC_CHANNEL_SETS_SRFN;
#if ( DCU == 1)
      pReq->Service.DataReq.channelSetIndex = eMAC_CHANNEL_SET_INDEX_1;
#else
      pReq->Service.DataReq.channelSetIndex = eMAC_CHANNEL_SET_INDEX_2;
#endif
      // Create the CMD and encode the payload
      pReq->Service.DataReq.payload[0]      = MAC_PING_RSP_CMD;
#if BOB_PADDED_PING == 1
      pReq->Service.DataReq.payloadLength   = PING_RSP_CMD_SIZE + 4 + padLength;
#else
      pReq->Service.DataReq.payloadLength   = PING_RSP_CMD_SIZE;
#endif
      uint16_t bitNo = 0;
      uint8_t* pDst = &pReq->Service.DataReq.payload[1];

      bitNo = PACK_uint16_2bits(&ping_req->Handle,              16,  pDst, bitNo);    // Handle          ( 16 bits )

      // Originator ( 40 bits), from the request
      bitNo = PACK_uint8_2bits( &ping_req->Origin_xid[0],        8,  pDst, bitNo);    // Requestor ID    ( 40 bits )
      bitNo = PACK_uint8_2bits( &ping_req->Origin_xid[1],        8,  pDst, bitNo);
      bitNo = PACK_uint8_2bits( &ping_req->Origin_xid[2],        8,  pDst, bitNo);
      bitNo = PACK_uint8_2bits( &ping_req->Origin_xid[3],        8,  pDst, bitNo);
      bitNo = PACK_uint8_2bits( &ping_req->Origin_xid[4],        8,  pDst, bitNo);

      bitNo = PACK_uint32_2bits( &ping_req->ReqTxTime.seconds,  32,  pDst, bitNo);    // Request TX Time ( 64 bits )
      bitNo = PACK_uint32_2bits( &ping_req->ReqTxTime.QFrac,    32,  pDst, bitNo);

      bitNo = PACK_uint8_2bits(  &ping_req->CounterResetFlag,    1,  pDst, bitNo);    // Ping Count Reset
      bitNo = PACK_uint8_2bits(  &ping_req->ResponseMode,        1,  pDst, bitNo);    // Response Addressing Mode
      bitNo = PACK_uint8_2bits(  &ping_req->Rsvd,                6,  pDst, bitNo);    // Reserved

      bitNo = PACK_uint32_2bits( &ping_req->ReqRxTime.seconds,  32,  pDst, bitNo);    // Request RX Time ( 64 bits )
      bitNo = PACK_uint32_2bits( &ping_req->ReqRxTime.QFrac,    32,  pDst, bitNo);

      bitNo = PACK_uint32_2bits( &ping_req->RspTxTime.seconds,  32,  pDst, bitNo);    // Response TX Time ( 64 bits )
      bitNo = PACK_uint32_2bits( &ping_req->RspTxTime.QFrac,    32,  pDst, bitNo);

      bitNo = PACK_uint16_2bits( &ping_req->PingCount,          16,  pDst, bitNo);    // Ping Count       ( 16 bits )
      bitNo = PACK_uint16_2bits( &ping_req->ReqRssi,            12,  pDst, bitNo);    // Request RSSI     ( 12 bits )
#if NOISE_TEST_ENABLED == 1
      bitNo = PACK_uint16_2bits( &ping_req->ReqChannel,         12,  pDst, bitNo);    // Request Channel  ( 12 bits )
      (void)  PACK_uint16_2bits( &ping_req->ReqPNLevel,         12,  pDst, bitNo);    // Request PNLevel  ( 12 bits )
#else
      (void)  PACK_uint16_2bits( &ping_req->ReqChannel,         12,  pDst, bitNo);    // Request Channel  ( 12 bits )
#endif
#if BOB_PADDED_PING
      bitNo = PACK_uint16_2bits( &ping_req->ExtraReqBytes,      16,  pDst, bitNo);    // Extra Req Bytes  ( 16 bits )
      bitNo = PACK_uint16_2bits( &ping_req->ExtraRspBytes,      16,  pDst, bitNo);    // Extra Resp Bytes ( 16 bits )
      if ( padLength > sizeof(pseudoRandomPad) ) { padLength = (uint16_t)sizeof(pseudoRandomPad); }
      for (uint16_t ndx = 0; ndx < padLength; ndx++)
      {
         bitNo = PACK_uint8_2bits( &pseudoRandomPad[ndx],        8,  pDst, bitNo);    // Extra data       (  8 bits )
      }
#endif
   } /*lint !e438  Last value assigned to variable 'bitNo' not used" */

   return pBuf;
}

#if ( EP == 1 )
/*
 * This function is used to create a Time Request Command (End Point)
 */
#define TIME_REQ_CMD_SIZE 1

static buffer_t *TimeReq_Create(MAC_CHANNEL_SET_INDEX_e chanIdx, MAC_dataConfCallback callback)
{
   (void) callback;
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_DATA_REQ, TIME_REQ_CMD_SIZE);
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */

      pReq->pConfirmHandler = NULL;

      pReq->Service.DataReq.handle          = 0;
      pReq->Service.DataReq.dst_addr_mode   = BROADCAST_MODE;

      pReq->Service.DataReq.callback = NULL;
      pReq->Service.DataReq.ackRequired     = (bool)false;
      pReq->Service.DataReq.droppable       = (bool)false;
      pReq->Service.DataReq.priority        = 7;
      pReq->Service.DataReq.reliability     = eMAC_RELIABILITY_LOW;
      pReq->Service.DataReq.channelSets     = eMAC_CHANNEL_SETS_SRFN;
      pReq->Service.DataReq.channelSetIndex = chanIdx;
      pReq->Service.DataReq.payloadLength   = TIME_REQ_CMD_SIZE;
      pReq->Service.DataReq.payload[0]      = MAC_TIME_REQ_CMD;
   }
   return pBuf;
}
#endif

/*
 * This function is used to create a Time Set Command
 */
static buffer_t * TimeSet_Create(uint8_t addr_mode, uint8_t dst_addr[MAC_ADDRESS_SIZE], MAC_dataConfCallback callback )  /*lint !e715 !e818 parameter not
                                                                                                                           referenced, could be ptr to const  */
{
   #define TIME_SET_CMD_SIZE 11

   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_DATA_REQ, TIME_SET_CMD_SIZE);
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */

      pReq->pConfirmHandler = NULL;

      pReq->Service.DataReq.handle          = 0;

      pReq->Service.DataReq.dst_addr_mode   =  (uint8_t)addr_mode;
      if(addr_mode == UNICAST_MODE)
      {
         (void)memcpy(pReq->Service.DataReq.dst_addr, dst_addr, MAC_ADDRESS_SIZE);
      }
      // DG: 1/14/2021: According to the Tengwar Time Spec: For Time-Set Cmd: droppable = false, priority = 5
      pReq->Service.DataReq.callback = NULL;
      pReq->Service.DataReq.ackRequired     = (bool)false;

      pReq->Service.DataReq.droppable       = (bool)false;
      pReq->Service.DataReq.priority        = 5;

      pReq->Service.DataReq.reliability     = eMAC_RELIABILITY_LOW;

      pReq->Service.DataReq.channelSets     = eMAC_CHANNEL_SETS_SRFN;
      pReq->Service.DataReq.channelSetIndex = eMAC_CHANNEL_SET_INDEX_1;
      pReq->Service.DataReq.payloadLength   = TIME_SET_CMD_SIZE;

      // Encode the time set
      (void)memset(pReq->Service.DataReq.payload, 0, TIME_SET_CMD_SIZE );
      pReq->Service.DataReq.payload[0] = MAC_TIME_SET_CMD;
   }
   return pBuf;
}  /*lint !e715 !e818 callback not referenced; dst_addr could be pointer to const */

/************************************************************************
 * Handle TIME_PUSH.request
 * Send the time out (time push from time module, debug port or response
 * to time request from other device)
 ************************************************************************/
/*lint -efunc( 715, MAC_TimePush_Request ) : confirm_cb currently not used */
MAC_TIME_PUSH_STATUS_e MAC_TimePush_Request ( uint8_t broadCastMode, uint8_t *srcAddr, MAC_ConfirmHandler confirm_cb , uint32_t time_diversity )
{
   MAC_TIME_PUSH_STATUS_e eStatus = eMAC_TIME_TRANSACTION_FAILED; //Default
   uint8_t source = TIME_SYNC_TimeSource_Get();
   bool isTimeValid;

   // Must be a time source or both.
   // Must have valid time
   // If there is already a TimeSync in the queue, then return.
   // If there is a Time Sync in TimeDiversity then return.
   // If there in a blocking window then set the TimePushPending flag and return.
   // If the request has TimeDiversity > 0, then set the InTimeDiversity flag and return.
   // else create the TimeSetCommand and put it in the queue.
   // then if the src is not null, and the link paramters period > 0 and this is a broadcast command
   // set send_MacLinkParams_ = true


   OS_MUTEX_Lock( &MAC_TimeSyncAttributeMutex_ ); // Function will not return if it fails
   //Build a TIME_PUSH.Request message
   isTimeValid = TIME_SYS_IsTimeValid();
   if ( (source == TIME_SYNC_SOURCE || source == TIME_SYNC_BOTH) && isTimeValid )
   {
      if ( TimeSync.InQueue )
      { //Time.push is in queue
         eStatus = eMAC_TIME_PENDING;
         INFO_printf("TimeSync.InQueue is true");
      }
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
      else if ( TimeSync.InTimeDiversity )
      {
         // Drop or ignore new Time.push requests
         // Time.push is in TimeDiversity
         eStatus = eMAC_TIME_PENDING;
         INFO_printf("TimeSync.InTimeDiversity is true");
      }
#endif
      else if ( TimeSync.InBlockingWindow )
      { //Blocking window is active
         eStatus = eMAC_TIME_PENDING;
         //Set the flag to send time.push when blocking window expires
         TimeSync.TimePushPending = true;
         INFO_printf("TimeSync.InBlockingWindow is true");
      }
      else
      {
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
         if ( 0 != time_diversity ) // Should we add Time Diversity?
         {
            INFO_printf( "MAC Command Response Time Diversity: %d ms\n", time_diversity );
            macCommandResponse_TimeDiversityTimer_Start( time_diversity );

            TimeSync.InTimeDiversity = true; // in TimeDiversity State now
            eStatus = eMAC_TIME_PENDING;

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
            // Note: TBD: Determine if srcAddr is always provided in an EP originated Time Req ( a.k.a Time Query )
            if ( ( NULL != srcAddr ) && ( 0 != LinkParameters_Period_Get() ) && ( BROADCAST_MODE == broadCastMode )  )
            {
               // Set the flag indicate that we have added MAC-Time Set CMD to the Tx Queue, because of the MAC_TIME_REQ
               send_MacLinkParams_ = (bool)true;
            }
#endif // end of section ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
         }
         else
#endif // end of ( MAC_CMD_RESP_TIME_DIVERSITY == 1 ) section
         {
#if ( DCU == 1 )
            /* Check for a transmitter available on the TB. If not, send the time sync request to another TB.  */
            if ( (MAINBD_IsPresent() && !TxChannels_init( eMAC_CHANNEL_SETS_SRFN, eMAC_CHANNEL_SET_INDEX_1 ) ) )
            {
               uint8_t starRecord[STAR_RECORD_SIZE] = {0};
               uint8_t offset = STAR_OPCODE_INDEX;

               (void)memset( starRecord, 0xFF, 8 ); // First 8 bytes are 0xFF, rest is 0
               starRecord[offset++] = (uint8_t)STAR_SRFN_TimeSync;
               starRecord[offset++] = (uint8_t)0;

               // Add meta data to STAR message and send to the main board
               STAR_add_metadata( starRecord, offset, 0 );
            }
            else
#endif
            {
               // No need to add the Time Diversity
               //Try to send the time.push
               buffer_t *pBuf;
               pBuf = TimeSet_Create(broadCastMode, srcAddr, NULL);
               if (pBuf != NULL)
               {  // Create a new MAC Transaction, and add to the Transaction Que
                  if( MAC_Transaction_Create (pBuf, MAC_CMD_FRAME, (bool)false) )
                  {
                     TimeSync.InQueue = true; //in queue now
                     eStatus = eMAC_TIME_SUCCESS;
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
                     // Note: TBD: Determine if srcAddr is always provided in an EP originated Time Req ( a.k.a Time Query )
                     if ( ( NULL != srcAddr ) && ( 0 != LinkParameters_Period_Get() ) && ( BROADCAST_MODE == broadCastMode )  )
                     {
                        // Set the flag indicate that we have added MAC-Time Set CMD to the Tx Queue, because of the MAC_TIME_REQ
                        send_MacLinkParams_ = (bool)true;
                     }
#endif // end of section ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
                  }else
                  {  // Free the buffer
                     eStatus = eMAC_TIME_TRANSACTION_OVERFLOW;
                     BM_free(pBuf);
                     ERR_printf("ERROR: MAC_Transaction_Create() failed");
                  }
               }
               else
               {
                  ERR_printf("ERROR: TimeSet_Create() failed");
               }
            }
         }
      }
   }
   else
   {
      ERR_printf("ERROR: Source is %u (should be 0 or 2). isTimeValid is %u (should be true)", source, (uint32_t)isTimeValid);
      eStatus = eMAC_TIME_INVALID_REQUEST; //Not a time source or Time is invalid
   }
   OS_MUTEX_Unlock( &MAC_TimeSyncAttributeMutex_ );  // Function will not return if it fails

   return eStatus; /*lint !e454 !e456 Mutex unlocked if successfully locked */
}  /*lint !e454 !e456 Mutex unlocked if successfully locked */

#if ( EP == 1 )
/*
 * This function is called by the Command Line to cause an EP to send a Time Request Command Frame over the RF Link
 * Only Supported on END POINT
 */
bool MAC_TimeQueryReq()
{
   bool execStatus = (bool)false;
   // Handle a time request command from the End Point
   buffer_t *pBuf;
   pBuf = TimeReq_Create(eMAC_CHANNEL_SET_INDEX_2, TimeReq_Done);
   if (pBuf != NULL)
   {  // Create a new MAC Transaction, and add to the Transaction Que
      if(MAC_Transaction_Create (pBuf, MAC_CMD_FRAME, (bool)false) )
      {
         execStatus = (bool)true;
         INFO_printf(" MAC-TIME Query REQ ");
      }else
      {  // Free the buffer
         BM_free(pBuf);
      }
   }
   return execStatus;
}
#endif

/*!
 * Call back function for Time Set Blocking Timer
 * This clears the flag.
 */
static void TimeSet_BlockingTimer_CB(uint8_t cmd, void *pData) /*lint !e715 parameter not referenced  */
{
   TimeSync.InBlockingWindow = false;    // Clear the flag

   if ( TimeSync.TimePushPending )
   {  //Need to send time sync. Use a static buffer. Do not allocate buffer from timer callback.
      if ( resendTimeSyncMsgbuf_.x.flag.isFree )
      {
         BM_AllocStatic( &resendTimeSyncMsgbuf_, eSYSFMT_TIME );
         resendTimeSyncMsgbuf_.data = (uint8_t*)&TimeSync.TimerId;
         OS_MSGQ_Post( &MAC_msgQueue, ( void * )&resendTimeSyncMsgbuf_ );
      }
      TimeSync.TimePushPending = false;
   }
}  /*lint !e715 !e818 cmd, pData not referenced; pData could be pointer to const  */

/*!
 * This function creates the timer used by the time set function to limit the rate that
 * time set commands can be sent
 */
static returnStatus_t TimeSet_BlockingTimer_Create(void)
{
   timer_t tmrSettings;            // Timer for re-issuing metadata registration
   returnStatus_t retVal = eFAILURE;

   (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
   tmrSettings.bOneShot       = true;
   tmrSettings.bExpired       = true;
   tmrSettings.bStopped       = true;
   tmrSettings.ulDuration_mS  = MAC_TIME_SET_BLOCKING_DELAY;
   tmrSettings.pFunctCallBack = TimeSet_BlockingTimer_CB;
   if ( eSUCCESS == TMR_AddTimer(&tmrSettings) )
   {
      TimeSync.TimerId = tmrSettings.usiTimerId;
      retVal = eSUCCESS;
   }
   //Initialize the flags
   TimeSync.InQueue = false;
   TimeSync.InBlockingWindow = false;
   TimeSync.TimePushPending = false;
   resendTimeSyncMsgbuf_.x.flag.isFree = 1; //Mark as the buffer is free

   return retVal;
}

/*!
 * This function is used to start the TimeSet blocking timer.
 */
static void TimeSet_BlockingTimer_Start(void)
{
   TimeSync.InBlockingWindow = true;
   (void)TMR_ResetTimer( TimeSync.TimerId, MAC_TIME_SET_BLOCKING_DELAY);
}

/*!
 * Reset.Request - parameters
 */
static void MAC_Reset(MAC_RESET_e type)
{
   if(type == eMAC_RESET_ALL)
   {
      ConfigAttr_Init( (bool)true);
   }
   // This will update the LastReset Time
   CachedAttr_Init( (bool)true);
}


/*!
 * This function is used to print the MAC Configuration to the terminal
 */
void MAC_ConfigPrint(void)
{
   char  floatStr[PRINT_FLOAT_SIZE];

   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_NetworkId              ], MAC_ConfigAttr.NetworkId              );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_AckWaitDuration        ], MAC_ConfigAttr.AckWaitDuration        );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_AckDelayDuration       ], MAC_ConfigAttr.AckDelayDuration       );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_CsmaMaxAttempts        ], MAC_ConfigAttr.CsmaMaxAttempts        );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_CsmaMaxBackOffTime     ], MAC_ConfigAttr.CsmaMaxBackOffTime     );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_CsmaMinBackOffTime     ], MAC_ConfigAttr.CsmaMinBackOffTime     );
   INFO_printf( "MAC_CONFIG:%s %s", mac_attr_names[eMacAttr_CsmaPValue             ], DBG_printFloat(floatStr, MAC_ConfigAttr.CsmaPValue, 2));
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_CsmaQuickAbort         ], MAC_ConfigAttr.CsmaQuickAbort         );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_PacketTimeout          ], MAC_ConfigAttr.PacketTimeout          );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_ReassemblyTimeout      ], MAC_ConfigAttr.ReassemblyTimeout      );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_ReliabilityHighCount   ], MAC_ConfigAttr.ReliabilityHighCount   );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_ReliabilityMediumCount ], MAC_ConfigAttr.ReliabilityMediumCount );
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_ReliabilityLowCount    ], MAC_ConfigAttr.ReliabilityLowCount    );
#if ( DCU == 1 )
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_TxPacketDelay          ], MAC_ConfigAttr.TxPacketDelay          );
#endif
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_TransactionTimeout     ], MAC_ConfigAttr.TransactionTimeout     );
#if  ( ( MAC_CMD_RESP_TIME_DIVERSITY == 1 ) && ( DCU == 1 ) )
   INFO_printf( "MAC_CONFIG:%s %d", mac_attr_names[eMacAttr_CmdRespMaxTimeDiversity], MAC_ConfigAttr.macCmdRespMaxTimeDiversity );
#endif
}

/*!
 * This function is used to print the MAC stats to the terminal
 */
void MAC_Stats(void)
{

   sysTime_t            sysTime;
   sysTime_dateFormat_t sysDateFormat;

   TIME_UTIL_ConvertSecondsToSysFormat(CachedAttr.LastResetTime.seconds, 0, &sysTime);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat);
   INFO_printf("MAC_STATS:LastResetTime=%02u/%02u/%04u %02u:%02u:%02u",
                 sysDateFormat.month, sysDateFormat.day, sysDateFormat.year, sysDateFormat.hour, sysDateFormat.min, sysDateFormat.sec); /*lint !e123 */

   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_ArqOverflowCount         ], CachedAttr.ArqOverflowCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_ChannelAccessFailureCount], CachedAttr.ChannelAccessFailureCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_FailedFcsCount           ], CachedAttr.FailedFcsCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_MalformedAckCount        ], CachedAttr.MalformedAckCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_MalformedFrameCount      ], CachedAttr.MalformedFrameCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_AdjacentNwkFrameCount    ], CachedAttr.AdjacentNwkFrameCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_RxFrameCount             ], CachedAttr.RxFrameCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_RxOverflowCount          ], CachedAttr.RxOverflowCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_TransactionOverflowCount ], CachedAttr.TransactionOverflowCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_TxFrameCount             ], CachedAttr.TxFrameCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_PacketId                 ], CachedAttr.PacketId);

   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_AcceptedFrameCount       ], CachedAttr.AcceptedFrameCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_DuplicateFrameCount      ], CachedAttr.DuplicateFrameCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_InvalidRecipientCount    ], CachedAttr.InvalidRecipientCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_PacketConsumedCount      ], CachedAttr.PacketConsumedCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_PacketReceivedCount      ], CachedAttr.PacketReceivedCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_PacketFailedCount        ], CachedAttr.PacketFailedCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_TxPacketCount            ], CachedAttr.TxPacketCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_TxPacketFailedCount      ], CachedAttr.TxPacketFailedCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_State                    ], (uint32_t)_state);

   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_TransactionTimeoutCount  ], CachedAttr.TransactionTimeoutCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_TxLinkDelayCount         ], CachedAttr.TxLinkDelayCount);
   INFO_printf( "MAC_STATS:%s %u", mac_attr_names[eMacAttr_TxLinkDelayTime          ], CachedAttr.TxLinkDelayTime);
#if ( EP == 1 )
   // Also print the RSSI Stats
   int i;
   for(i=0;i<MAC_MAX_DEVICES;i++)
   {
      stats_print(&CachedAttr.stats[i]);
   }
#endif
}

/*
 * This function is used to process a Command Frame
 */
static void Process_CmdFrame(MAC_DataInd_t const *pDataInd)
{
   /* A command frame consists of
      uint8_t cmd_id;
      uint8_t data[]
   */
   switch(pDataInd->payload[0])
   {
      case MAC_PING_REQ_CMD:
      {  // Handle a ping request command (EP Only)
         Process_PingReqFrame(pDataInd);
      }
      break;
#if ( DCU == 1 )
      case MAC_PING_RSP_CMD:
      {  // Handle a ping response command (DCU Only)
         Process_PingRspFrame(pDataInd);
      }
      break;
#endif
#if ( EP == 1 )
      case MAC_TIME_SET_CMD:
      {  // Handle a time set command (EP Only)
         (void)TIME_SYS_SetDateTimeFromMAC(pDataInd);
      }
      break;
#endif
      case MAC_TIME_REQ_CMD:
      {  // Validate and process the time request message
#if ( ( PORTABLE_DCU == 1 ) || ( MFG_MODE_DCU == 1 ) )
         (void)MAC_TimePush_Request( UNICAST_MODE, (uint8_t*)pDataInd->src_addr, NULL, 0);
#else
         // Send the time request either broadcast or unicast depending on setting
         if ( TIME_SYNC_TimeQueryResponseMode_Get() == BROADCAST_MODE )
         {
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
            (void)MAC_TimePush_Request( BROADCAST_MODE, (uint8_t*)pDataInd->src_addr, NULL, Random_CommandResponseTimeDiversity_Get() );
#else
            (void)MAC_TimePush_Request( BROADCAST_MODE, (uint8_t*)pDataInd->src_addr, NULL, 0 );
#endif
         }
         else if ( TIME_SYNC_TimeQueryResponseMode_Get() == UNICAST_MODE )
         {
            (void)MAC_TimePush_Request( UNICAST_MODE, (uint8_t*)pDataInd->src_addr, NULL, 0 );
         }
         else
         {
            INFO_printf("TSync request ignored");
         }
#endif
      }
      break;

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( EP == 1 ) )
      case MAC_LINK_PARAM_CMD:
      {
         INFO_printf("MAC-Link Parameters Command");
         Process_LinkParametersCommand( pDataInd );
      }
      break;
#endif

      default:
         INFO_printf("CommandId:Unsupported");
         break;
   }
}

 /**********************************************************************
 *
 * \fn void Process_PingReqFrame(MAC_DataInd_t const *pDataInd)
 *
 * \brief  This is a ping request from the DCU
 *
 * \details
 *
 * \param pDataInd
 *
 * \return  none
 *
 **********************************************************************
 */
static void Process_PingReqFrame(MAC_DataInd_t const *pDataInd)
{
   ping_req_t ping_req;

   uint16_t bitNo = 0;
   uint8_t const *p = &pDataInd->payload[1];

   // get the time stamp from the request
   ping_req.Handle             = UNPACK_bits2_uint16( p, &bitNo,                     16 ); // Handle                   ( 16 bit )

   ping_req.Origin_xid[0] = pDataInd->src_addr[0];
   ping_req.Origin_xid[1] = pDataInd->src_addr[1];
   ping_req.Origin_xid[2] = pDataInd->src_addr[2];
   ping_req.Origin_xid[3] = pDataInd->src_addr[3];
   ping_req.Origin_xid[4] = pDataInd->src_addr[4];

   ping_req.ReqTxTime.seconds    = UNPACK_bits2_uint32( p, &bitNo,                     32 ); // Request Timestamp        ( 64 bit )
   ping_req.ReqTxTime.QFrac      = UNPACK_bits2_uint32( p, &bitNo,                     32 );
   ping_req.CounterResetFlag     = UNPACK_bits2_uint8(  p, &bitNo,                      1 ); // Ping Count Reset         (  1 bit )
   ping_req.ResponseMode         = UNPACK_bits2_uint8(  p, &bitNo,                      1 ); // Response Addressing Mode (  1 bit )
   ping_req.Rsvd                 = UNPACK_bits2_uint8(  p, &bitNo,                      6 ); // Reserved                 (  6 bit )
#if BOB_PADDED_PING == 1
   if ( pDataInd->payloadLength >= ( ( 8+16+32+32+1+1+6+16+16 ) / 8 ) )
   {
      ping_req.ExtraReqBytes   = UNPACK_bits2_uint16( p, &bitNo,                     16 ); // Extra request bytes      ( 16 bit )
      ping_req.ExtraRspBytes   = UNPACK_bits2_uint16( p, &bitNo,                     16 ); // Extra response bytes     ( 16 bit )
      if ( ( ping_req.ExtraReqBytes > sizeof(pseudoRandomPad) ) || ( ping_req.ExtraRspBytes > sizeof(pseudoRandomPad) ) )
      {
         ping_req.ExtraReqBytes   = 0;
         ping_req.ExtraRspBytes   = 0;
      }
   }
   else
   {
      ping_req.ExtraReqBytes   = 0;
      ping_req.ExtraRspBytes   = 0;
   }
#endif

   // Get the current time
   (void) TIME_UTIL_GetTimeInSecondsFormat(&ping_req.ReqRxTime);

#if BOB_PADDED_PING == 1
   INFO_printf("PingReqFrame: Handle:%02x time_stamp: %08x %08x reset_counter:%02x rsp_mode:%02x extra:%u/%u",
      ping_req.Handle,                 // handle
      ping_req.ReqTxTime.time,         // tx time
      ping_req.ReqTxTime.fracTime,     // tx time
      ping_req.CounterResetFlag,       // reset_flag
      ping_req.ResponseMode,           // response mode
      ping_req.ExtraReqBytes,          // extra request bytes
      ping_req.ExtraRspBytes           // extra response bytes
   );
#else
   INFO_printf("PingReqFrame: Handle:%02x time_stamp: %08x %08x reset_counter:%02x rsp_mode:%02x extra:%u/%u",
      ping_req.Handle,                 // handle
      ping_req.ReqTxTime.seconds,      // tx time
      ping_req.ReqTxTime.QFrac,        // tx time
      ping_req.CounterResetFlag,       // reset_flag
      ping_req.ResponseMode,           // response mode
      0, 0
   );
#endif

   // Check is the counter is to be reset
   if(ping_req.CounterResetFlag != 0)
   {
      CachedAttr.PingCount = 0;
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
   }

   // Increment the Ping Count
   MAC_CounterInc(eMAC_PingCount);

   // Create the ping response
   // The ping handle from the ping request
   // The timestamp in the original ping request
   // The timestamp of when the original ping request was received (from the PHY.DATA.indication)
   // The timestamp the ping response was sent to the PHY layer (i.e. the handoff to the PHY layer adjusted for Tx time)
   // The RSSI of the original ping request frame (from the PHY.DATA.indication)
   // The current ping counter value
   ping_rsp_t ping_rsp;

   ping_rsp.Handle     =  ping_req.Handle,        // handle from the request
   ping_rsp.PingCount  =  CachedAttr.PingCount;
   ping_rsp.ReqRssi    =  pDataInd->rssi;         // Measured RSSI
   ping_rsp.ReqPNLevel =  pDataInd->danl;         // Measured DANL
   ping_rsp.ReqChannel =  pDataInd->channel;      // Channel on which the Ping Request was received

   ping_rsp.CounterResetFlag = ping_req.CounterResetFlag;
   ping_rsp.ResponseMode     = ping_req.ResponseMode;
   ping_rsp.Rsvd             = 0;

   ping_rsp.ReqTxTime     = ping_req.ReqTxTime;
   ping_rsp.ReqRxTime     = pDataInd->timeStamp;

   // Get the current time
   (void) TIME_UTIL_GetTimeInSecondsFormat(&ping_rsp.RspTxTime);

   ping_rsp.Origin_xid[0] = ping_req.Origin_xid[0];
   ping_rsp.Origin_xid[1] = ping_req.Origin_xid[1];
   ping_rsp.Origin_xid[2] = ping_req.Origin_xid[2];
   ping_rsp.Origin_xid[3] = ping_req.Origin_xid[3];
   ping_rsp.Origin_xid[4] = ping_req.Origin_xid[4];
#if BOB_PADDED_PING == 1
   ping_rsp.ExtraReqBytes = ping_req.ExtraReqBytes;
   ping_rsp.ExtraRspBytes = ping_req.ExtraRspBytes;
#endif
   buffer_t *pBuf;
   if(ping_req.ResponseMode == 0)
   {
      pBuf = PingRsp_Create(NULL, BROADCAST_MODE, pDataInd->src_addr, &ping_rsp);
   }else
   {
      pBuf = PingRsp_Create(NULL, UNICAST_MODE,   pDataInd->src_addr, &ping_rsp);
   }

   if (pBuf != NULL)
   {  // Create a new MAC Transaction, and add to the Transaction Que
      if( !MAC_Transaction_Create (pBuf, MAC_CMD_FRAME, (bool)false) )
      {  // Failed, so free the buffer
         BM_free(pBuf);
      }
   }
}

#if DCU == 1
/*!
 * This function prints the EUI format = xx.xx.xx.xx.xx.xx.xx.xx
 */
/*lint -esym(528, sprintf_eui) Not referenced   */
static int32_t sprintf_eui ( char *buffer, char const *msg, eui_addr_t const eui )
{
   int32_t cnt;
   cnt = sprintf( buffer, "%s%02hhx.%02hhx.%02hhx.%02hhx.%02hhx.%02hhx.%02hhx.%02hhx" ,
      msg, eui[0], eui[1], eui[2], eui[3], eui[4] , eui[5], eui[6], eui[7] );
   return cnt;
}

/*!
 **********************************************************************
 *
 * \fn void Process_PingRspFrame(MAC_DataInd_t const *pDataInd)
 *
 * \brief
 *
 * \details
 *
 * \param pDataInd
 *
 * \return  none
 *
 **********************************************************************
 */
static void Process_PingRspFrame(MAC_DataInd_t const *pDataInd)
{
   ping_rsp_t ping_rsp;

   uint8_t const *p = &pDataInd->payload[1];

   uint16_t bitNo = 0;

   ping_rsp.Handle              = UNPACK_bits2_uint16( p, &bitNo, 16 ); // Handle                   ( 16 bit )

   ping_rsp.Origin_xid[0]       = UNPACK_bits2_uint8(  p, &bitNo,  8 );  // Originator Extension ID ( 40 bits ), from the request
   ping_rsp.Origin_xid[1]       = UNPACK_bits2_uint8(  p, &bitNo,  8 );
   ping_rsp.Origin_xid[2]       = UNPACK_bits2_uint8(  p, &bitNo,  8 );
   ping_rsp.Origin_xid[3]       = UNPACK_bits2_uint8(  p, &bitNo,  8 );
   ping_rsp.Origin_xid[4]       = UNPACK_bits2_uint8(  p, &bitNo,  8 );

   ping_rsp.ReqTxTime.seconds   = UNPACK_bits2_uint32( p, &bitNo, 32 ); //                         ( 64 bits )
   ping_rsp.ReqTxTime.QFrac     = UNPACK_bits2_uint32( p, &bitNo, 32 );

   ping_rsp.CounterResetFlag    = UNPACK_bits2_uint8(  p, &bitNo,  1 );
   ping_rsp.ResponseMode        = UNPACK_bits2_uint8(  p, &bitNo,  1 );
   ping_rsp.Rsvd                = UNPACK_bits2_uint8(  p, &bitNo,  6 );

   ping_rsp.ReqRxTime.seconds   = UNPACK_bits2_uint32( p, &bitNo, 32 ); //                         ( 64 bit )
   ping_rsp.ReqRxTime.QFrac     = UNPACK_bits2_uint32( p, &bitNo, 32 );

   ping_rsp.RspTxTime.seconds   = UNPACK_bits2_uint32( p, &bitNo, 32 ); //                         ( 64 bit )
   ping_rsp.RspTxTime.QFrac     = UNPACK_bits2_uint32( p, &bitNo, 32 );

   ping_rsp.PingCount           = UNPACK_bits2_uint16( p, &bitNo, 16 );
   ping_rsp.ReqRssi             = UNPACK_bits2_uint16( p, &bitNo, 12 );
   ping_rsp.ReqChannel          = UNPACK_bits2_uint16( p, &bitNo, 12 );

   /* If the target devices returned a DANL (noise) measurement, then decode it */
   if (pDataInd->payloadLength == 40)
   {  // the noise is present
      ping_rsp.ReqPNLevel                 = UNPACK_bits2_uint16( p, &bitNo, 12 );
   }else
   {
      ping_rsp.ReqPNLevel                 = 0;
   }
#if BOB_PADDED_PING
   (void) UNPACK_bits2_uint16( p, &bitNo,  4 );
   uint16_t extraReqBytes = 0, extraRspBytes = 0;
   if (pDataInd->payloadLength >= 42)
   {
      extraReqBytes             = UNPACK_bits2_uint16( p, &bitNo, 16 );
      extraRspBytes             = UNPACK_bits2_uint16( p, &bitNo, 16 );
   }
#endif
   MAC_PingInd_t PingIndicationData;
   MAC_PingInd_t *PingInd = &PingIndicationData;

   PingInd->Handle           = ping_rsp.Handle;

   // Copy the device OUI to the src address
   (void) memcpy(&PingInd->src_eui[0], MacAddress_.oui, sizeof(MacAddress_.oui));

   //  Copy the 5 bytes of the Src Extended Unique Id to the source EUI
   (void)memcpy( &PingInd->src_eui[3],     pDataInd->src_addr,  sizeof(xid_addr_t) );

   // Copy the device OUI to the origin
   (void) memcpy(&PingInd->origin_eui[0], MacAddress_.oui, sizeof(MacAddress_.oui));

   //  Copy the 5 bytes of the Extended Unique Id to the origin EUI
   (void)memcpy( &PingInd->origin_eui[3],  ping_rsp.Origin_xid, sizeof(xid_addr_t) );

   PingInd->CounterResetFlag   = ping_rsp.CounterResetFlag;  // pingReset
   PingInd->RspMode            = ping_rsp.ResponseMode;      // rspAddrMode
   PingInd->ReqTxTime          = ping_rsp.ReqTxTime;         // reqTxTimestamp
   PingInd->ReqRxTime          = ping_rsp.ReqRxTime;         // reqRxTimestamp
   PingInd->RspTxTime          = ping_rsp.RspTxTime;         // rspTxTimestamp
   PingInd->RspRxTime          = pDataInd->timeStamp;        // rspRxTimestamp

   // reqRssi
   PingInd->ReqRssi            = ((int16_t)(ping_rsp.ReqRssi * 10 / 16)) - 2000;  // reqRssi converted to units of 0.1dBm
   PingInd->ReqRssi            = (int16_t)((PingInd->ReqRssi/10)*10);             // Remove decimal part until HE is fixed. This crashes it.

   // reqPNlevel
   PingInd->ReqPNLevel         = ((int16_t)(ping_rsp.ReqPNLevel * 10 / 16)) - 2000;  // ReqPNLevel converted to units of 0.1dBm
   PingInd->ReqPNLevel         = (int16_t)((PingInd->ReqPNLevel/10)*10);             // Remove decimal part until HE is fixed. This crashes it.

   PingInd->ReqChannel         = ping_rsp.ReqChannel;                             // reqChannel

   // rspRssi
   PingInd->RspRssi            = ((int16_t)(pDataInd->rssi * 10 / 16)) - 2000;    // rspRssi converted to units of 0.1dBm
   PingInd->RspRssi            = (int16_t)((PingInd->RspRssi/10)*10);             // Remove decimal part until HE is fixed. This crashes it.

   // rspPNlevel
   PingInd->RspPNLevel         = ((int16_t)(pDataInd->danl * 10 / 16)) - 2000;    // rspDANL converted to units of 0.1dBm
   PingInd->RspPNLevel         = (int16_t)((PingInd->RspPNLevel/10)*10);          // Remove decimal part until HE is fixed. This crashes it.
   PingInd->RspChannel         = pDataInd->channel;                               // rspChannel

   {
      // Print the Print the Ping Indication
#if BOB_PADDED_PING == 1
      buffer_t *pBuf = BM_allocStack( 320 );
#else
      buffer_t *pBuf = BM_allocStack( 300 );
#endif
      if(pBuf != NULL)
      {
         char* buffer = (char*) pBuf->data;
         int32_t offset = 0;
         offset += sprintf(     &buffer[offset],  "Handle:%hu"          , PingInd->Handle);
         offset += sprintf_eui (&buffer[offset], ",org:"                , PingInd->origin_eui);
         offset += sprintf_eui (&buffer[offset], ",src:"                , PingInd->src_eui);
         offset += sprintf(     &buffer[offset], ",CounterReset:%hhu"   , PingInd->CounterResetFlag);
         offset += sprintf(     &buffer[offset], ",RspMode:%d"          , PingInd->RspMode);
         offset +=
            TIME_UTIL_SecondsToDateStr( &buffer[offset], ",ReqTxTS:"    , PingInd->ReqTxTime );
         offset +=
            TIME_UTIL_SecondsToDateStr( &buffer[offset], ",ReqRxTS:"    , PingInd->ReqRxTime );
         offset +=
            TIME_UTIL_SecondsToDateStr( &buffer[offset], ",RspTxTS:"    , PingInd->RspTxTime );
         offset +=
            TIME_UTIL_SecondsToDateStr( &buffer[offset], ",RspRxTS:"    , PingInd->RspRxTime );

         offset += sprintf(     &buffer[offset], ",ReqRssi:%hd/%hd"    , PingInd->ReqRssi,
                                                                         PingInd->ReqPNLevel );
         offset += sprintf(     &buffer[offset], ",RspRssi:%hd/%hd"    , PingInd->RspRssi,
                                                                         PingInd->RspPNLevel );


         offset += sprintf(     &buffer[offset], ",Count:%hu"          , PingInd->PingCount);
         offset += sprintf(     &buffer[offset], ",ReqChan:%hu"        , PingInd->ReqChannel);
#if BOB_PADDED_PING == 1
         offset += sprintf(     &buffer[offset], ",Extra:%hu/%hu"      , extraReqBytes,
                                                                         extraRspBytes );
#endif
         INFO_printf("{\"PingInd\":{%s}}", buffer);
         BM_free(pBuf);
      }
   }
   // Call the Trace Route Indication handler
   TraceRoute_IndicationHandler(PingInd);
}
#endif

/*!
 * Returns the Message for START Status
 */
char* MAC_StartStatusMsg(MAC_START_STATUS_e status)
{
   char* msg="";
   switch(status) /*lint !e788 not all enums used within switch */
   {
      case    eMAC_START_SUCCESS       :  msg = "MAC_START_SUCCESS";             break;
      case    eMAC_START_RUNNING       :  msg = "MAC_START_RUNNING";             break;
   }  /*lint !e788 not all enums used within switch */
   return msg;
}

/*!
 * Returns the Message for STOP Status
 */
char* MAC_StopStatusMsg(MAC_STOP_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case    eMAC_STOP_SUCCESS:        msg = "MAC_STOP_SUCCESS";          break;
      case    eMAC_STOP_ERROR:          msg = "MAC_STOP_ERROR";            break;
   }
   return msg;
}
/*!
 * Returns the Message for RESET Status
 */
char* MAC_ResetStatusMsg(MAC_RESET_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eMAC_RESET_SUCCESS      :  msg = "MAC_RESET_SUCCESS ";              break;
      case eMAC_RESET_FAILURE      :  msg = "MAC_RESET_FAILURE ";              break;
   }
   return msg;
}

/*!
 * Returns the Message for GET Status
 */
char* MAC_GetStatusMsg(MAC_GET_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eMAC_GET_SUCCESS:              msg = "MAC_GET_SUCCESS";              break;
      case eMAC_GET_UNSUPPORTED:          msg = "MAC_GET_UNSUPPORTED";          break;
      case eMAC_GET_SERVICE_UNAVAILABLE:  msg = "MAC_GET_SERVICE_UNAVAILABLE";  break;
   }
   return msg;
}

/*!
 * Returns the Message for SET Status
 */
char* MAC_SetStatusMsg(MAC_SET_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eMAC_SET_SUCCESS:              msg = "MAC_SET_SUCCESS";              break;
      case eMAC_SET_READONLY:             msg = "MAC_SET_READONLY";             break;
      case eMAC_SET_UNSUPPORTED:          msg = "MAC_SET_UNSUPPORTED";          break;
      case eMAC_SET_INVALID_PARAMETER:    msg = "MAC_SET_INVALID_PARAMETER";    break;
      case eMAC_SET_SERVICE_UNAVAILABLE:  msg = "MAC_SET_SERVICE_UNAVAILABLE";  break;
   }
   return msg;
}

/*!
* Returns the Message for TIME_Query Status
 */
char* MAC_TimeQueryStatusMsg(MAC_TIME_QUERY_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eMAC_TIME_QUERY_SUCCESS              : msg = "MAC_TIME_SUCCESS             "; break;
      case eMAC_TIME_QUERY_MAC_IDLE             : msg = "MAC_TIME_MAC_IDLE            "; break;
      case eMAC_TIME_QUERY_TRANSACTION_FAILED   : msg = "MAC_TIME_TRANSACTION_FAILED  "; break;
      case eMAC_TIME_QUERY_TRANSACTION_OVERFLOW : msg = "MAC_TIME_TRANSACTION_OVERFLOW"; break;
   }
   return msg;
}

/*!
 * Returns the Message for FLUSH Status
 */
char* MAC_FlushStatusMsg(MAC_FLUSH_STATUS_e status)
{
     char* msg="";
   switch(status)
   {
      case eMAC_FLUSH_SUCCESS             : msg = "MAC_FLUSH_SUCCESS"            ; break;
      case eMAC_FLUSH_MAC_IDLE            : msg = "MAC_FLUSH_MAC_IDLE"           ; break;
   }
   return msg;
}

/*!
 * Returns the Message for PURGE Status
 */
char* MAC_PurgeStatusMsg(MAC_PURGE_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eMAC_PURGE_SUCCESS            : msg = "MAC_PURGE_SUCCESS"             ; break;
      case eMAC_PURGE_IDLE               : msg = "MAC_PURGE_IDLE"                ; break;
      case eMAC_PURGE_INVALID_HANDLE     : msg = "MAC_PURGE_INVALID_HANDLE"      ; break;
   }
   return msg;
}

/*!
 * Returns the Message for DATA Status
 */
char* MAC_DataStatusMsg(MAC_DATA_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eMAC_DATA_SUCCESS              : msg = "MAC_DATA_SUCCESS"             ; break;
      case eMAC_DATA_MAC_IDLE             : msg = "MAC_DATA_MAC_IDLE"            ; break;
      case eMAC_DATA_TRANSACTION_OVERFLOW : msg = "MAC_DATA_TRANSACTION_OVERFLOW"; break;
      case eMAC_DATA_TRANSACTION_FAILED   : msg = "MAC_DATA_TRANSACTION_FAILED"  ;break;
      case eMAC_DATA_INVALID_PARAMETER    : msg = "MAC_DATA_INVALID_PARAMETER"   ;break;
      case eMAC_DATA_INVALID_HANDLE       : msg = "MAC_DATA_INVALID_HANDLE"      ;break;
   }
   return msg;
}


/*!
 * Returns the Message for PING Request Status
 */
char* MAC_PingStatusMsg(MAC_PING_STATUS_e status)
{
   char* msg="";
   switch(status)
   {
      case eMAC_PING_SUCCESS              : msg = "MAC_PING_SUCCESS"             ; break;
      case eMAC_PING_MAC_IDLE             : msg = "MAC_PING_MAC_IDLE"            ; break;
      case eMAC_PING_TRANSACTION_OVERFLOW : msg = "MAC_PING_TRANSACTION_OVERFLOW"; break;
      case eMAC_PING_TRANSACTION_FAILED   : msg = "MAC_PING_TRANSACTION_FAILED"  ; break;
      case eMAC_PING_INVALID_PARAMETER    : msg = "MAC_PING_INVALID_PARAMETER"   ; break;
      case eMAC_PING_INVALID_HANDLE       : msg = "MAC_PING_INVALID_HANDLE"      ; break;
   }
   return msg;
}


/*!
 * Returns the Message for _state
 */
static char* MAC_StateMsg(MAC_STATE_e state)
{
   char* msg="";
   switch(state)
   {
      case eMAC_STATE_IDLE           : msg = "eMAC_STATE_IDLE"          ; break;
      case eMAC_STATE_OPERATIONAL    : msg = "eMAC_STATE_OPERATIONAL"   ; break;
   }
   return msg;
}

/*! ********************************************************************************************************************
 *
 * \fn void MAC_CounterInc(MAC_COUNTER_e eMacCounter)
 *
 * \brief  This function is used to increment the MAC Counters
 *
 * \param  eMacCounter Enumerated valued for the counter
 *
 * \return  none
 *
 * \details This functions is intended to handle the rollover if required.
 *          If the counter was modified the CacheFile will be updated.
 *
 ********************************************************************************************************************
 */
void MAC_CounterInc(MAC_COUNTER_e eMacCounter)
{
   bool bStatus = false;
   switch(eMacCounter)
   {  //                                                                           Counter                      rollover
      case eMAC_AcceptedFrameCount       :   bStatus = u32_counter_inc(&CachedAttr.AcceptedFrameCount         , (bool) false); break;
      case eMAC_AdjacentNwkFrameCount    :   bStatus = u32_counter_inc(&CachedAttr.AdjacentNwkFrameCount      , (bool) false); break;
      case eMAC_ArqOverflowCount         :   bStatus = u32_counter_inc(&CachedAttr.ArqOverflowCount           , (bool) false); break;
      case eMAC_ChannelAccessFailureCount:   bStatus = u32_counter_inc(&CachedAttr.ChannelAccessFailureCount  , (bool) false); break;
      case eMAC_DuplicateFrameCount      :   bStatus = u32_counter_inc(&CachedAttr.DuplicateFrameCount        , (bool) false); break;
      case eMAC_FailedFcsCount           :   bStatus = u32_counter_inc(&CachedAttr.FailedFcsCount             , (bool) false); break;
      case eMAC_InvalidRecipientCount    :   bStatus = u32_counter_inc(&CachedAttr.InvalidRecipientCount      , (bool) false); break;
      case eMAC_MalformedAckCount        :   bStatus = u32_counter_inc(&CachedAttr.MalformedAckCount          , (bool) false); break;
      case eMAC_MalformedFrameCount      :   bStatus = u32_counter_inc(&CachedAttr.MalformedFrameCount        , (bool) false); break;
      case eMAC_PacketConsumedCount      :   bStatus = u32_counter_inc(&CachedAttr.PacketConsumedCount        , (bool) false); break;
      case eMAC_PacketFailedCount        :   bStatus = u32_counter_inc(&CachedAttr.PacketFailedCount          , (bool) false); break;
      case eMAC_PacketReceivedCount      :   bStatus = u32_counter_inc(&CachedAttr.PacketReceivedCount        , (bool) false); break;
      case eMAC_PingCount                :   bStatus = u16_counter_inc(&CachedAttr.PingCount                  , (bool)  true); break;
      case eMAC_RxFrameCount             :   bStatus = u32_counter_inc(&CachedAttr.RxFrameCount               , (bool) false); break;
      case eMAC_RxOverflowCount          :   bStatus = u32_counter_inc(&CachedAttr.RxOverflowCount            , (bool) false); break;
      case eMAC_TransactionOverflowCount :   bStatus = u32_counter_inc(&CachedAttr.TransactionOverflowCount   , (bool) false); break;
      case eMAC_TransactionTimeoutCount  :   bStatus = u32_counter_inc(&CachedAttr.TransactionTimeoutCount    , (bool)  true); break;
      case eMAC_TxFrameCount             :   bStatus = u32_counter_inc(&CachedAttr.TxFrameCount               , (bool) false); break;
      case eMAC_TxLinkDelayCount         :   bStatus = u32_counter_inc(&CachedAttr.TxLinkDelayCount           , (bool)  true); break;
      case eMAC_TxPacketCount            :   bStatus = u32_counter_inc(&CachedAttr.TxPacketCount              , (bool) false); break;
      case eMAC_TxPacketFailedCount      :   bStatus = u32_counter_inc(&CachedAttr.TxPacketFailedCount        , (bool) false); break;
   }
   if(bStatus)
   {   // The counter was modified
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
   }
}

/*!
 * This function is used to increment a u32 counter
 * The counter is not incremented if the value = UINT32_MAX and rollover is set to false
 */
static bool u32_counter_inc(uint32_t* pCounter, bool bRollover)
{
   bool bStatus = false;
   if((bRollover) || ( *pCounter < UINT32_MAX ))
   {
      (*pCounter)++;
      bStatus = true;
   }
   return bStatus;
}

/*!
 * This function is used to increment a u16 counter
 * The counter is not incremented if the value = UINT8_MAX and rollover is set to false
 */
static bool u16_counter_inc(uint16_t* pCounter, bool bRollover)
{
   bool bStatus = false;
   if((bRollover) || ( *pCounter < UINT16_MAX ))
   {
      (*pCounter)++;
      bStatus = true;
   }
   return bStatus;
}

/*!
 *
 */
static uint32_t NextRequestHandle(void)
{
   static uint32_t handle = 0;
   return(++handle);
}

#if (EP == 1)
/**
Create a TIME Request and send to the MAC

@param confirm_cb Function pointer for confirmation callback.  Set to NULL if not required.

@return Handle ID or 0 on error.
*/
uint32_t MAC_TimeQueryRequest(void(*confirm_cb)(MAC_Confirm_t const *pConfirm))
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_TIME_QUERY_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pMsg = (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
      pMsg->pConfirmHandler = confirm_cb;
      pMsg->handleId = NextRequestHandle();
      pMsg->Service.TimeQueryReq.channelSetIndex = eMAC_CHANNEL_SET_INDEX_2;
      MAC_Request(pBuf);
      return pMsg->handleId;
   }

   return 0;
}
#endif

/*!
 *  Create a START Request and send to the MAC
 */
uint32_t MAC_StartRequest(void (*confirm_cb)(MAC_Confirm_t const *pConfirm) )
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_START_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pMsg = (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
      pMsg->pConfirmHandler = confirm_cb;
      pMsg->handleId = NextRequestHandle();
      MAC_Request(pBuf);
      return pMsg->handleId;
   }
   return 0;
}

/*!
 *  Create a STOP Request and send to the MAC
 */
uint32_t MAC_StopRequest(void (*confirm_cb)(MAC_Confirm_t const *request) )
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_STOP_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pMsg = (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
      pMsg->pConfirmHandler = confirm_cb;
      pMsg->handleId = NextRequestHandle();
      MAC_Request(pBuf);
      return pMsg->handleId;
   }
   return 0;
}

/*!
 *  Confirmation handler for Get/Set Request
 */
static void MAC_GetSetRequest_CB(MAC_Confirm_t const *confirm) /*lint !e715 parameter not referenced  */
{
   // Notify calling function that MAC has got/set attribute
   OS_SEM_Post ( &MAC_AttributeSem_ );
} /*lint !e715 */

/*!
 *  Create a Get Request and send to the MAC
 */
MAC_GetConf_t MAC_GetRequest( MAC_ATTRIBUTES_e eAttribute )
{
   MAC_GetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = eMAC_GET_SERVICE_UNAVAILABLE;

   //This mutex serialize the access to the pend semaphore. Only one task will pend on this semaphore at any given time.
   OS_MUTEX_Lock( &MAC_AttributeMutex_ ); // Function will not return if it fails
   /* Build a Data.Request message from this data and send to the MAC layer */
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_GET_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
      pReq->pConfirmHandler   = MAC_GetSetRequest_CB;
      pReq->handleId          = NextRequestHandle();
      pReq->Service.GetReq.eAttribute = eAttribute;
      MAC_Request(pBuf);
      // Wait for MAC to retrieve attribute
      if (OS_SEM_Pend ( &MAC_AttributeSem_, 10*ONE_SEC )) {
         confirm = ConfirmMsg.GetConf; // Save data
      }
   }
   OS_MUTEX_Unlock( &MAC_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != eMAC_GET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex lock states */
      INFO_printf("ERROR: MAC_GetRequest: status %s, attribute %s", MAC_GetStatusMsg(confirm.eStatus), mac_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 *  Create a Set Request and send to the MAC
 */
MAC_SetConf_t MAC_SetRequest( MAC_ATTRIBUTES_e eAttribute, void const *val)
{
   MAC_SetConf_t confirm;

   (void)memset(&confirm, 0, sizeof(confirm));

   confirm.eStatus = eMAC_SET_SERVICE_UNAVAILABLE;

   //This mutex serialize the access to the pend semaphore. Only one task will pend on this semaphore at any given time.
   OS_MUTEX_Lock( &MAC_AttributeMutex_ ); // Function will not return if it fails
   /* Build a Data.Request message from this data and send to the MAC layer */
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_SET_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
      pReq->pConfirmHandler   = MAC_GetSetRequest_CB;
      pReq->handleId          = NextRequestHandle();
      pReq->Service.SetReq.eAttribute = eAttribute;
      (void)memcpy((uint8_t *)&pReq->Service.SetReq.val, (uint8_t *)val, sizeof(MAC_ATTRIBUTES_u)); /*lint !e420 Apparent access beyond array for function 'memcpy' */
      MAC_Request(pBuf);
      // Wait for MAC to retrieve attribute
      if (OS_SEM_Pend ( &MAC_AttributeSem_, 10*ONE_SEC )) {
         confirm = ConfirmMsg.SetConf; // Save data
      }
   }
   OS_MUTEX_Unlock( &MAC_AttributeMutex_ ); // Function will not return if it fails

   if (confirm.eStatus != eMAC_SET_SUCCESS) { /*lint !e456 Two execution paths are being combined with different mutex lock states */
      INFO_printf("ERROR: MAC_SetRequest: status %s, attribute %s", MAC_SetStatusMsg(confirm.eStatus), mac_attr_names[eAttribute]);
   }

   return confirm; /*lint !e454 A thread mutex has been locked but not unlocked */
}

/*!
 *  Create a Reset Request and send to the MAC
 */
uint32_t MAC_ResetRequest(MAC_RESET_e eType, void (*confirm_cb)(MAC_Confirm_t const *request))
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_RESET_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pMsg = (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
      pMsg->Service.ResetReq.eType = eType;
      pMsg->pConfirmHandler = confirm_cb;
      pMsg->handleId = NextRequestHandle();
      MAC_Request(pBuf);
      return pMsg->handleId;
   }
   return 0;
}

/*!
 *  Create a Ping Request and send to the MAC
 */
uint32_t MAC_PingRequest(
   uint16_t             handle,          /* 0-65535 */
   eui_addr_t const     dst_addr,        /* 40-bit extention address  */
   uint8_t              counterReset,    /* 0 - Don't Reset,   1 - Reset Counter   */
#if BOB_PADDED_PING == 0
   uint8_t              rspMode)         /* 0 - BROADCAST_MODE 1 - UNICAST_MODE  1 */
#else
   uint8_t              rspMode,         /* 0 - BROADCAST_MODE 1 - UNICAST_MODE  1 */
   uint16_t             extra_DL_bytes,  /* number of extra bytes to send in request  */
   uint16_t             extra_UL_bytes)  /* number of extra bytes to send in response */
#endif
{
   uint16_t payloadLength = PING_REQ_CMD_SIZE;

#if BOB_PADDED_PING == 1
   uint16_t padLength;

   padLength = max( extra_DL_bytes, sizeof( pseudoRandomPad ) );
   payloadLength += padLength;
#endif

   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_PING_REQ, payloadLength);
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
      pReq->pConfirmHandler = NULL;
      pReq->handleId = NextRequestHandle();
      pReq->Service.PingReq.handle = (uint16_t) pReq->handleId;
      pReq->Service.DataReq.tx_type = eTX_TYPE_SRFN;

      pReq->Service.PingReq.dst_addr_mode = UNICAST_MODE;
      (void) memcpy(pReq->Service.PingReq.dst_addr, &dst_addr[3], MAC_ADDRESS_SIZE);

      pReq->Service.PingReq.callback        = NULL;
      pReq->Service.PingReq.ackRequired     = (bool)false;
      pReq->Service.PingReq.droppable       = (bool)true;
      pReq->Service.PingReq.reliability     = eMAC_RELIABILITY_LOW;
      pReq->Service.PingReq.priority        = 7;
      pReq->Service.PingReq.channelSets     = eMAC_CHANNEL_SETS_SRFN;
#if ( DCU == 1)
      pReq->Service.PingReq.channelSetIndex = eMAC_CHANNEL_SET_INDEX_1;
#else
      pReq->Service.PingReq.channelSetIndex = eMAC_CHANNEL_SET_INDEX_2;
#endif
      pReq->Service.PingReq.payloadLength   = payloadLength;
      {
         // For a Ping Request, we build the payload here
         TIMESTAMP_t currentTime;
         (void) TIME_UTIL_GetTimeInSecondsFormat(&currentTime);

         // Create the CMD and encode the payload
         pReq->Service.PingReq.payload[0]      = MAC_PING_REQ_CMD;
#if BOB_PADDED_PING == 0
         pReq->Service.PingReq.payloadLength   = PING_REQ_CMD_SIZE;
#else
         pReq->Service.PingReq.payloadLength   = payloadLength;
#endif

         uint16_t bitNo = 0;
         uint8_t* pDst  = &pReq->Service.PingReq.payload[1];

         uint8_t Rsvd = 0;

         bitNo = PACK_uint16_2bits(&handle,                        16, pDst, bitNo);   // Handle                   ( 16 bit )

         bitNo = PACK_uint32_2bits(&currentTime.seconds,           32, pDst, bitNo);   // Request Timestamp        ( 64 bit )
         bitNo = PACK_uint32_2bits(&currentTime.QFrac,             32, pDst, bitNo);

         bitNo = PACK_uint8_2bits( &counterReset,                   1, pDst, bitNo);   // Ping Count Reset         (  1 bit )
         bitNo = PACK_uint8_2bits( &rspMode,                        1, pDst, bitNo);   // Response Addressing Mode (  1 bit )
         bitNo = PACK_uint8_2bits( &Rsvd,                           6, pDst, bitNo);   // Reserved                 (  6 bit )
#if BOB_PADDED_PING == 1
         bitNo = PACK_uint16_2bits( &extra_DL_bytes,               16, pDst, bitNo);   // Extra bytes in request   ( 16 bit )
         bitNo = PACK_uint16_2bits( &extra_UL_bytes,               16, pDst, bitNo);   // Extra bytes in response  ( 16 bit )
         for (uint16_t ndx = 0; ndx < padLength; ndx++)
         {
            bitNo = PACK_uint8_2bits( &pseudoRandomPad[ndx],        8, pDst, bitNo);
         }
#endif
      }  //lint !e438 last bitNo not used due to loop
      MAC_Request(pBuf);
      return pReq->handleId;
   }
   // If a buffer error occurred, should this send a ping confirmation?
   return 0;
}

/*!
 *  Create a SRFN Data Request and send to the MAC
 */
uint32_t MAC_DataRequest_Srfn(
/*lint -esym(578, priority) priority same name as enum OK  */
   NWK_Address_t const *dst_address,
   uint8_t       const *payload,                    /* Pointer to the payload data */
   uint16_t             payloadLength,              /* The size of the payload bytes */
   bool                 ackRequired,
   uint8_t              priority,
   bool                 droppable,
   MAC_Reliability_e    reliability,
   MAC_CHANNEL_SET_INDEX_e channelSetIndex,
   MAC_dataConfCallback dataConfirm_cb,
   void (*confirm_cb)(MAC_Confirm_t const *pConfirm) )
{

  buffer_t *pBuf = ReqBuffer_Alloc(eMAC_DATA_REQ, payloadLength);

   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
      pReq->pConfirmHandler = confirm_cb;
      pReq->handleId = NextRequestHandle();
      pReq->Service.DataReq.handle = (uint16_t) pReq->handleId;
      pReq->Service.DataReq.tx_type = eTX_TYPE_SRFN;

      if ( ( dst_address != NULL) && (dst_address->addr_type == eEXTENSION_ID) )
      {
         pReq->Service.DataReq.dst_addr_mode = UNICAST_MODE;
         (void)memcpy(pReq->Service.DataReq.dst_addr, dst_address->ExtensionAddr, MAC_ADDRESS_SIZE);
      }else
      {
         pReq->Service.DataReq.dst_addr_mode = BROADCAST_MODE;
         (void)memset(pReq->Service.DataReq.dst_addr, 0,                          MAC_ADDRESS_SIZE);
      }
      pReq->Service.DataReq.callback = dataConfirm_cb;
      pReq->Service.DataReq.payloadLength   = payloadLength;
      (void)memcpy(pReq->Service.DataReq.payload, payload, payloadLength);

      pReq->Service.DataReq.ackRequired     = ackRequired;
      pReq->Service.DataReq.droppable       = droppable;
      pReq->Service.DataReq.priority        = priority;
      pReq->Service.DataReq.reliability     = reliability;
      pReq->Service.DataReq.channelSets     = eMAC_CHANNEL_SETS_SRFN;
      pReq->Service.DataReq.channelSetIndex = channelSetIndex;

      MAC_Request(pBuf);
      return pReq->handleId;
   }
   // Buffer error
   INFO_printf("MAC_DataRequest_Srfn() failed to get a buffer");
   return 0;
}

/*!
 *  Create a STAR Data Request and send to the MAC
 */
uint32_t MAC_DataRequest_Star(
   uint8_t       const *payload,                    /* Pointer to the payload data */
   uint16_t             payloadLength,              /* The size of the payload bytes */
   bool                 fastMessage,
   bool                 skip_cca)
{
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_DATA_REQ, payloadLength);

   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
      pReq->pConfirmHandler = NULL;
      pReq->handleId = NextRequestHandle();

      pReq->Service.DataReq.handle = (uint16_t) pReq->handleId;
      pReq->Service.DataReq.tx_type = eTX_TYPE_STAR;

      pReq->Service.DataReq.dst_addr_mode = BROADCAST_MODE;                   // Not needed for STAR, But needs to be set
      (void)memset(pReq->Service.DataReq.dst_addr, 0, MAC_ADDRESS_SIZE);

      pReq->Service.DataReq.payloadLength   = payloadLength;
      (void)memcpy(pReq->Service.DataReq.payload,  payload, payloadLength);

      pReq->Service.DataReq.ackRequired     = (bool)false;              // No acks for STAR
      pReq->Service.DataReq.droppable       = (bool)false;              // Do not drop
      pReq->Service.DataReq.priority        = 1;                        //
      pReq->Service.DataReq.reliability     = eMAC_RELIABILITY_LOW;     // Must be set to LOW
      pReq->Service.DataReq.channelSets     = eMAC_CHANNEL_SETS_STAR;
      pReq->Service.DataReq.channelSetIndex = eMAC_CHANNEL_SET_INDEX_1;
      pReq->Service.DataReq.callback        = NULL;
      pReq->Service.DataReq.fastMessage     = fastMessage;
      pReq->Service.DataReq.skip_cca        = skip_cca;
      MAC_Request(pBuf);
      return pReq->handleId;
   }
   else{
      INFO_printf("MAC_DataRequest_Star() failed to get a buffer");
      return 0;
    }
}

/*!
 *  Create a Flush Request and send to the MAC
 */
uint32_t MAC_FlushRequest(void (*confirm_cb)(MAC_Confirm_t  const *request))
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_FLUSH_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
      pReq->pConfirmHandler = confirm_cb;
      pReq->handleId = NextRequestHandle();
      MAC_Request(pBuf);
      return pReq->handleId;
   }
   // Buffer error
   return 0;
}


/*!
 *  Create a Flush Request and send to the MAC
 */
uint32_t MAC_PurgeRequest(uint16_t handle, void (*confirm_cb)(MAC_Confirm_t const *request))
{
   // Allocate the buffer
   buffer_t *pBuf = ReqBuffer_Alloc(eMAC_PURGE_REQ, 0);
   if (pBuf != NULL)
   {
      MAC_Request_t *pReq = (MAC_Request_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
      pReq->pConfirmHandler = confirm_cb;
      pReq->handleId = NextRequestHandle();

      pReq->Service.PurgeReq.handle = handle;
      pReq->handleId = NextRequestHandle();
      MAC_Request(pBuf);
      return pReq->handleId;
   }
   return 0;
}



#if ( DCU == 1 )
/*!
 **********************************************************************
 *
 * \fn void MAC_PingCounter_Reset()
 *
 * \brief This function reset the ping counter
 *
 * \details
 *
 * \return  none
 *
 **********************************************************************
 */
void MAC_PingCounter_Reset()
{
   CachedAttr.PingCount = 0;
   (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
   }

/*!
 **********************************************************************
 *
 * \fn uint16_t MAC_PingCounter_Inc()
 *
 * \brief This function increments the ping counter
 *
 * \details
 *
 * \return  uint16_t - ping counter
 *
 **********************************************************************
 */
uint16_t MAC_PingCounter_Inc()
{
   MAC_CounterInc(eMAC_PingCount);
   return CachedAttr.PingCount;
}
#endif

/*!
 ******************************************************************************
 *
 * \fn         Process_GetReq
 *
 * \brief      Get.Request - handler
 *
 * \param      pointer to the MAC_GetReq_t parameters
 *
 * \return     true   - Confirmation is ready
 *
 * \details
 *
 ******************************************************************************
 */
static bool Process_GetReq( MAC_GetReq_t const *pGetReq )
{
   pMacConf->Type               = eMAC_GET_CONF;
   pMacConf->GetConf.eStatus    = MAC_Attribute_Get( pGetReq, &pMacConf->GetConf.val);
   pMacConf->GetConf.eAttribute = pGetReq->eAttribute;
   return true;
}

/*!
 ******************************************************************************
 *
 * \fn         Process_SetReq
 *
 * \brief      Set.Request - handler
 *
 * \param      pointer to the MAC_SetReq_t parameters
 *
 * \return     true   - Confirmation is ready
 *
 * \details
 *
 ******************************************************************************
 */
static bool Process_SetReq( MAC_SetReq_t const *pSetReq )
{
   pMacConf->Type               = eMAC_SET_CONF;
   pMacConf->SetConf.eStatus    = MAC_Attribute_Set(pSetReq);
   pMacConf->SetConf.eAttribute = pSetReq->eAttribute;
   return true;
}

/*!
 ******************************************************************************
 *
 * \fn         Process_ResetReq
 *
 * \brief      Reset.Request - handler
 *
 * \param      pointer to the MAC_ResetReq_t parameters
 *
 * \return     true   - Confirmation is ready
 *
 * \details
 *
 ******************************************************************************
 */
static bool Process_ResetReq( MAC_ResetReq_t const *pResetReq )
{
   INFO_printf("Process_ResetReq");
   pMacConf->Type = eMAC_RESET_CONF;

   if((pResetReq->eType == eMAC_RESET_ALL) ||
      (pResetReq->eType == eMAC_RESET_STATISTICS))
   {
      MAC_Reset(pResetReq->eType);
      pMacConf->ResetConf.eStatus = eMAC_RESET_SUCCESS;
   }
   else{
      pMacConf->ResetConf.eStatus = eMAC_RESET_FAILURE;
   }
   return true;
}

/*!
 ******************************************************************************
 *
 * \fn         Process_DataReq
 *
 * \brief      Data.Request - handler
 *
 * \param      pointer to a buffer containing the request parameters
 *
 * \return     true   - Confirmation is ready
 *             false  - Execution is pending
 *
 * \details
 *
 ******************************************************************************
 */
static bool Process_DataReq( buffer_t *pBuf )
{
   MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the MAC_DataReq_t information */
   MAC_DataReq_t *macRequest = (MAC_DataReq_t *)&pReq->Service.DataReq;    /*lint !e740 !e826 */
   bool TxChannelAvailable;

   INFO_printf("Process_DataReq");
   pMacConf->Type = eMAC_DATA_CONF;
   if(_state == eMAC_STATE_OPERATIONAL)
   {
      // Get TX channel
      TxChannelAvailable = TxChannels_init(macRequest->channelSets, macRequest->channelSetIndex);

      // Validate parameters
      if ( ((macRequest->dst_addr_mode == BROADCAST_MODE) || (macRequest->dst_addr_mode == UNICAST_MODE)) &&
            (macRequest->payloadLength >  0)                     &&
            (macRequest->payloadLength <= MAC_MAX_PAYLOAD_SIZE)  &&
            (macRequest->priority      <= 7)                     &&
            (macRequest->reliability   <= eMAC_RELIABILITY_HIGH) &&
            (TxChannelAvailable ) )
      {        // Make sure a TX channel is available

         uint8_t frame_type;
#if ( DCU == 1 )
         if ( macRequest->tx_type == eTX_TYPE_STAR )
         {
            if(pReq->Service.DataReq.fastMessage)
            {
               frame_type = MAC_STAR_FAST_MESSAGE;
            }else{
               frame_type = MAC_STAR_DATA_FRAME;
            }
         }else
#endif
         {
            frame_type = MAC_DATA_FRAME;
         }

         if( !MAC_Transaction_Create ( pBuf, frame_type, pReq->Service.DataReq.skip_cca) )
         {  // Transaction queue full
            ERR_printf("ERROR:Que Full");
            pMacConf->DataConf.status = eMAC_DATA_TRANSACTION_OVERFLOW;
            pMacConf->DataConf.handle = macRequest->handle;
            return true;
         }else
         {
            // The data request is valid and was added to the PacketTxQueue.
            // Let's use this opportunity to timestamp this event.
            // We want to track this event and use it as part of a watchdog to make sure that the PHY did its job.

            // Update time if it is empty
#if ( MCU_SELECTED == NXP_K24 )
            if ( (DataReqTime.TICKS[0] == 0) && (DataReqTime.TICKS[1] == 0) )
            {
               _time_get_elapsed_ticks(&DataReqTime);
            }
#elif ( MCU_SELECTED == RA6E1 )
            if ( ( DataReqTime.tickCount == 0 ) && (DataReqTime.xNumOfOverflows == 0) )
            {
               OS_TICK_Get_CurrentElapsedTicks(&DataReqTime);
            }
#endif
            return false;
         }
      } else {
         ERR_printf("ERROR:Invalid Parameter"
                     " addr_mode:%u"
                     " payloadLength:%u"
                     " priority:%u"
                     " reliability:%u"
                     " channelSets:%u"
                     " channelSetIndex:%u"
                     " TxChannelAvailable:%s",
               (uint32_t)macRequest->dst_addr_mode,
               (uint32_t)macRequest->payloadLength,
               (uint32_t)macRequest->priority,
               (uint32_t)macRequest->reliability,
               (uint32_t)macRequest->channelSets,
               (uint32_t)macRequest->channelSetIndex,
               TxChannelAvailable?"True":"False");
         pMacConf->DataConf.status = eMAC_DATA_INVALID_PARAMETER;
         pMacConf->DataConf.handle = macRequest->handle;
         return true;
      }
   }else
   {
      ERR_printf("ERROR:state!=OPERATIONAL");
      pMacConf->DataConf.status = eMAC_DATA_MAC_IDLE;
      pMacConf->DataConf.handle = macRequest->handle;
      return true;
   }
}

/*!
 ******************************************************************************
 *
 * \fn         Process_PingReq
 *
 * \brief      This function handles a ping request
 *
 * \param      pBuf  buffer containing the request parameters
 *
 * \return     true   - Confirmation is ready
 *             false  - Execution is pending
 *
 * \details
 *
 ******************************************************************************
 */
static bool Process_PingReq( buffer_t *pBuf )
{
   MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the MAC_DataReq_t information */
   MAC_DataReq_t *macRequest = (MAC_DataReq_t *)&pReq->Service.DataReq;    /*lint !e740 !e826 */

   INFO_printf("Process_PingReq");
   pMacConf->Type = eMAC_PING_CONF;

   if( (_state == eMAC_STATE_OPERATIONAL) )
   {
      // Validate parameters
      if ( ((macRequest->dst_addr_mode == BROADCAST_MODE) || (macRequest->dst_addr_mode == UNICAST_MODE)) &&
            (macRequest->payloadLength <= MAC_MAX_PAYLOAD_SIZE)  &&
            (macRequest->priority      <= 7)                     &&
            (macRequest->reliability   <= eMAC_RELIABILITY_HIGH) &&
            (TxChannels_init(macRequest->channelSets, macRequest->channelSetIndex)) ) { // Make sure a TX channel is available

         if( !MAC_Transaction_Create ( pBuf, MAC_CMD_FRAME, (bool)false ) )
         {  // Transaction queue full
            MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e578 !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
            MAC_DataReq_t *macRequest = (MAC_DataReq_t *)&pReq->Service.DataReq;    /*lint !e578 !e2445 !e740 !e826 */

            pMacConf->PingConf.eStatus = eMAC_PING_TRANSACTION_OVERFLOW;
            pMacConf->PingConf.handle  = macRequest->handle;
            return true;
         }else
         {
            return false;
         }
      }else
      {
         MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e578 !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
         MAC_DataReq_t *macRequest = (MAC_DataReq_t *)&pReq->Service.DataReq;    /*lint !e578 !e2445 !e740 !e826 */

         pMacConf->PingConf.eStatus = eMAC_PING_INVALID_PARAMETER;
         pMacConf->PingConf.handle  = macRequest->handle;
         return true;
      }
   }else
   {
      MAC_Request_t *pReq = (MAC_Request_t*)pBuf->data; /*lint !e578 !e2445 !e740 !e826 payload holds the PHY_DataReq_t information */
      MAC_DataReq_t *macRequest = (MAC_DataReq_t *)&pReq->Service.DataReq;    /*lint !e578 !e2445 !e740 !e826 */

      pMacConf->PingConf.eStatus = eMAC_PING_MAC_IDLE;
      pMacConf->PingConf.handle  = macRequest->handle;
      return true;
   }
}

/*!
 ******************************************************************************
 *
 * \fn         Process_StartReq
 *
 * \brief      Start.Request - handler
 *
 * \param      pointer to the MAC_StartReq_t parameters
 *
 * \return     true   - Confirmation is ready
 *             false  - Execution is pending
 *
 * \details    Upon receipt of a START.request a device will attempt to transition to its operational state as
 *             described in Section 3.1.3.1.
 *
 ******************************************************************************
 */
static bool Process_StartReq( MAC_StartReq_t const *pStartReq )
{
   (void) pStartReq;

   INFO_printf("Process_StartReq");
   pMacConf->Type = eMAC_START_CONF;

   if(_state == eMAC_STATE_IDLE)
   {
      phy.eState = eMAC_STATE_PHY_READY;
      _state = eMAC_STATE_OPERATIONAL;
      pMacConf->StartConf.eStatus = eMAC_START_SUCCESS;
   }else
   {
      pMacConf->StartConf.eStatus = eMAC_START_RUNNING;
   }

   INFO_printf("%s", MAC_StateMsg( _state ) );
   return (bool)true;

}

/*!
 ******************************************************************************
 *
 * \fn         Process_StopReq
 *
 * \brief      Stop.Request - handler
 *
 * \param      pointer to the MAC_StopReq_t parameters
 *
 * \return     true   - Confirmation is ready
 *
 * \details    The MAC issues a STOP.confirm primitive after receiving a STOP.request.
 *             If the stop operation encounters an error for any reason, the status field is set to ERROR;
 *             otherwise it is set to SUCCESS.
 *
 ******************************************************************************
 */
static bool Process_StopReq( MAC_StopReq_t const *pStopReq )
{
   (void) pStopReq;

   INFO_printf("Process_StopReq");

   if(_state == eMAC_STATE_OPERATIONAL)
   {
      _state = eMAC_STATE_IDLE;
      pMacConf->StopConf.eStatus = eMAC_STOP_SUCCESS;
   }else
   {
      pMacConf->Type = eMAC_STOP_CONF;
      pMacConf->StopConf.eStatus = eMAC_STOP_ERROR;
   }
   INFO_printf("%s", MAC_StateMsg( _state ) );
   return (bool)true;
}

/*!
 ******************************************************************************
 *
 * \fn         Process_FlushReq
 *
 * \brief      Stop.Request - handler
 *
 * \param      pointer to the MAC_FlushReq_t parameters
 *
 * \return     true   - Confirmation is ready
 *
 * \details
 *
 ******************************************************************************
 */
static bool Process_FlushReq( MAC_FlushReq_t const *pFlushReq )
{
   (void) pFlushReq;
   INFO_printf("FlushReq");

   pMacConf->Type = eMAC_FLUSH_CONF;

   if(_state == eMAC_STATE_IDLE)
   {
        pMacConf->FlushConf.eStatus = eMAC_FLUSH_MAC_IDLE;
   }else
   {
      (void) MAC_PacketManag_Flush();
      pMacConf->FlushConf.eStatus = eMAC_FLUSH_SUCCESS;
   }
   return true;
}

/*!
 ******************************************************************************
 *
 * \fn         Process_PurgeReq
 *
 * \brief      Purge.Request - handler
 *
 * \param      pointer to the MAC_PurgeReq_t parameters
 *
 * \return     true   - Confirmation is ready
 *
 * \details
 *
 ******************************************************************************
 */
static bool Process_PurgeReq( MAC_PurgeReq_t const *pPurgeReq )
{
   INFO_printf("PurgeReq: handle = %d", pPurgeReq->handle);

   pMacConf->Type = eMAC_PURGE_CONF;

   if(_state == eMAC_STATE_IDLE)
   {
        pMacConf->PurgeConf.eStatus = eMAC_PURGE_IDLE;
   }else
   {
      if(MAC_PacketManag_Purge ( pPurgeReq->handle ))
      {
         pMacConf->PurgeConf.eStatus = eMAC_PURGE_SUCCESS;
      }else
      {
         pMacConf->PurgeConf.eStatus = eMAC_PURGE_INVALID_HANDLE;
      }
   }
   return true;
}

#if (EP == 1)
/**
Time_Query.Request - handler

@param pTimeReq pointer to the MAC_TimeReq_t parameters

@return true on success, false on failure
*/
static bool Process_TimeQueryReq(MAC_TimeQueryReq_t const *pTimeReq)
{
   INFO_printf("TimeReq");

   bool execStatus = false;

   buffer_t *pBuf = TimeReq_Create(pTimeReq->channelSetIndex, TimeReq_Done);

   if (pBuf != NULL)
   {
      // Create a new MAC Transaction, and add to the Transaction Queue
      if (MAC_Transaction_Create(pBuf, MAC_CMD_FRAME, false)) /*lint !e747 stdbool */
      {
         execStatus = true;
      }
      else
      {
         // Free the buffer
         BM_free(pBuf);
      }
   }

   pMacConf->Type = eMAC_TIME_QUERY_CONF;

   if (execStatus)
   {
      pMacConf->TimeQueryConf.eStatus = eMAC_TIME_QUERY_SUCCESS;
   }
   else
   {
      pMacConf->TimeQueryConf.eStatus = eMAC_TIME_QUERY_TRANSACTION_FAILED;
   }

   return true;
}
#endif

#if EP == 1
/*!
 * This function is called to print the mac statistics for a single record
 */
/*lint --esym(123,min)  */
static void stats_print(mac_stats_t const *pStats)
{
   sysTime_t sysTime;
   sysTime_dateFormat_t sysDateFormat1;
   sysTime_dateFormat_t sysDateFormat2;

   TIME_UTIL_ConvertSecondsToSysFormat(pStats->FirstHeardTime,    0, &sysTime);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat1);

   TIME_UTIL_ConvertSecondsToSysFormat(pStats->LastHeardTime, 0, &sysTime);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat2);

#if NOISE_TEST_ENABLED == 1
   INFO_printf("MAC_STATS:"
      "xid:%02x:%02x:%02x:%02x:%02x "
      "Added:%02d/%02d/%04d %02d:%02d:%02d "
      "Updated:%02d/%02d/%04d %02d:%02d:%02d "
      "RxCnt:%li "
      "Rssi=[%3d %3d %3d %3d]dBm",
      "Noise=[%3d %3d %3d %3d]dBm ",
                 pStats->xid[0], pStats->xid[1], pStats->xid[2], pStats->xid[3], pStats->xid[4],
                 sysDateFormat1.month, sysDateFormat1.day, sysDateFormat1.year, sysDateFormat1.hour,
                 sysDateFormat1.min, sysDateFormat1.sec,
                 sysDateFormat2.month, sysDateFormat2.day, sysDateFormat2.year,
                 sysDateFormat2.hour, sysDateFormat2.min, sysDateFormat2.sec,
                 pStats->RxCount,
                 pStats->rssi_dbm[(pStats->RxCount+0)%MAC_RSSI_HISTORY_SIZE],
                 pStats->rssi_dbm[(pStats->RxCount+1)%MAC_RSSI_HISTORY_SIZE],
                 pStats->rssi_dbm[(pStats->RxCount+2)%MAC_RSSI_HISTORY_SIZE],
                 pStats->rssi_dbm[(pStats->RxCount+3)%MAC_RSSI_HISTORY_SIZE],

                 pStats->noise_dbm[(pStats->RxCount+0)%MAC_RSSI_HISTORY_SIZE],
                 pStats->noise_dbm[(pStats->RxCount+1)%MAC_RSSI_HISTORY_SIZE],
                 pStats->noise_dbm[(pStats->RxCount+2)%MAC_RSSI_HISTORY_SIZE],
                 pStats->noise_dbm[(pStats->RxCount+3)%MAC_RSSI_HISTORY_SIZE]); /*lint !e123 */
#else
   INFO_printf("MAC_STATS:"
      "xid:%02x:%02x:%02x:%02x:%02x "
      "Added:%02d/%02d/%04d %02d:%02d:%02d "
      "Updated:%02d/%02d/%04d %02d:%02d:%02d "
      "RxCnt:%li "
      "Rssi=[%3d %3d %3d %3d]dBm",
                 pStats->xid[0], pStats->xid[1], pStats->xid[2], pStats->xid[3], pStats->xid[4],
                 sysDateFormat1.month, sysDateFormat1.day, sysDateFormat1.year,
                 sysDateFormat1.hour, sysDateFormat1.min, sysDateFormat1.sec,
                 sysDateFormat2.month, sysDateFormat2.day, sysDateFormat2.year,
                 sysDateFormat2.hour, sysDateFormat2.min, sysDateFormat2.sec,
                 pStats->RxCount,
                 pStats->rssi_dbm[(pStats->RxCount+0)%MAC_RSSI_HISTORY_SIZE],
                 pStats->rssi_dbm[(pStats->RxCount+1)%MAC_RSSI_HISTORY_SIZE],
                 pStats->rssi_dbm[(pStats->RxCount+2)%MAC_RSSI_HISTORY_SIZE],
                 pStats->rssi_dbm[(pStats->RxCount+3)%MAC_RSSI_HISTORY_SIZE]); /*lint !e123 */
#endif
}

/*!
 * This function is called to determine if the unit already exists in the mac statistics list
 */
static mac_stats_t *stats_find(uint8_t const xid[MAC_ADDRESS_SIZE])
{
   int i;
   mac_stats_t *pStats = NULL;

   for(i=0;i<MAC_MAX_DEVICES;i++)
   {
      if((memcmp(CachedAttr.stats[i].xid, xid, MAC_ADDRESS_SIZE) == 0))
      {  // A match was found
         pStats = &CachedAttr.stats[i];
         break;
      }
   }
   return pStats;
}

/*!
 * This function is called to add new unit to the mac statistics
 * Should check first to verify that it does not alreay exist
 * If the list is full, it will replace the oldest in the list with this unit.
 */
static mac_stats_t *stats_add(uint8_t const xid[MAC_ADDRESS_SIZE])
{
   int i;
   TIMESTAMP_t ts;

   mac_stats_t *pStats;

   pStats = &CachedAttr.stats[0];

   // For each record
   for(i=0;i<MAC_MAX_DEVICES;i++)
   {
      // Add this mac address to the list
      if(!CachedAttr.stats[i].inUse)
      {  // not in use, so use this one!
         pStats = &CachedAttr.stats[i];
         break;
      }else
      {  // Keep track of the oldest one in the list, in case we need to replace
         if(CachedAttr.stats[i].FirstHeardTime < pStats->FirstHeardTime)
         {  // is this one older than the oldest we have?
            pStats = &CachedAttr.stats[i];
         }
      }
   }

   // This will replace the one in that slot
   (void) memset(pStats,0, sizeof(mac_stats_t));

   pStats->inUse = true;
   (void) TIME_UTIL_GetTimeInSecondsFormat( &ts);
   pStats->FirstHeardTime = ts.seconds;
   (void) memcpy(pStats->xid, xid, MAC_ADDRESS_SIZE);
   return pStats;
}

/*!
 * This function is called to updated the mac statistics
 */
#if NOISE_TEST_ENABLED == 1
static void stats_update(uint8_t xid[MAC_ADDRESS_SIZE], int16_t rssi_dbm, int16_t danl_dbm)
#else
static void stats_update(uint8_t const xid[MAC_ADDRESS_SIZE], int16_t rssi_dbm)
#endif
{
   mac_stats_t *pStats;

   // try to find this unit
   pStats = stats_find(xid);

   if(pStats == NULL)
   {  // Add to the list
      pStats = stats_add(xid);
   }

   if(pStats != NULL)
   {
      TIMESTAMP_t ts;
      (void) TIME_UTIL_GetTimeInSecondsFormat( &ts);
      pStats->LastHeardTime = ts.seconds;
      pStats->rssi_dbm[ pStats->RxCount%MAC_RSSI_HISTORY_SIZE] = rssi_dbm;
#if NOISE_TEST_ENABLED == 1
      pStats->noise_dbm[pStats->RxCount%MAC_RSSI_HISTORY_SIZE] = danl_dbm;
#endif
      pStats->RxCount++;

      // Update the file
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
#if NOISE_TEST_ENABLED == 1
      stats_print(pStats);
#endif
   }
}

/*!
 * This should print the same information that is sent in the ??? message
 */
static void stats_dump(void)
{
   int i;

   for(i=0;i<MAC_MAX_DEVICES;i++)
   {
      if(CachedAttr.stats[i].inUse)
      {  // Only print the ones in use
         stats_print(&CachedAttr.stats[i]);
      }
   }
   PHY_RssiHistory_Print();
}

/*!
 **********************************************************************
 *
 * \fn mac_stats_t* MAC_STATS_GetRecord(uint32_t index)
 *
 * \brief  This is used to get the stats record for a single unit
 *
 * \details
 *
 * \param index
 *
 * \return  mac_stats_t*
 *
 **********************************************************************
 */
mac_stats_t *MAC_STATS_GetRecord(uint32_t index)
{
   return &CachedAttr.stats[index];
}

#endif

/*!
 **********************************************************************
 *
 * \fn GenerateFailedConfirm
 *
 * \brief  This function generates a failed Confirm based on input type
 *    in order to cause the MAC layer to recover when an expected confirm
 *    has not shown up in a reasonable amount time.  This is typically caused
 *    by insufficient buffers when the PHY attempted to generate a confirm.
 *    If this function fails to get a buffer, the code checking for timeouts
 *    will continue to execute this function.
 *
 * \details
 *
 * \param confirm type
 *
 * \return  none
 *
 **********************************************************************
 */
static void GenerateFailedConfirm( PHY_CONF_TYPE_t confType )
{
   buffer_t *pBuf;
   bool sendConfirm = (bool)true;

   // Allocate a buffer for the message
   pBuf = BM_allocStack(sizeof(PHY_Confirm_t));
   if (pBuf != NULL)
   {
      pBuf->eSysFmt = eSYSFMT_PHY_CONFIRM;
      PHY_Confirm_t *pConf = (PHY_Confirm_t *)pBuf->data; /*lint !e2445 !e740 !e826 */
      pConf->MsgType   = confType;
      switch(pConf->MsgType)  /*lint !e788 not all enums used within switch */
      {
         case ePHY_CCA_CONF:
            pConf->CcaConf.eStatus = ePHY_CCA_SERVICE_UNAVAILABLE;
            break;
         case ePHY_RESET_CONF:
            pConf->ResetConf.eStatus = ePHY_RESET_FAILURE;
            break;
         case ePHY_DATA_CONF:
            pConf->DataConf.eStatus = ePHY_DATA_TRANSMISSION_FAILED;
            break;
         default:
            INFO_printf("Unexpected confirmation timeout type" );
            sendConfirm = (bool)false;
            BM_free(pBuf);
      }  /*lint !e788 not all enums are handled by the switch  */

      /* extend the timeout again to avoid race condition if a data.request gets into the MAC msg queue before this
         confirm gets processed.  In that scenario this function might be called twice and put 2 recovery confirms
         into the msg queue */
#if ( MCU_SELECTED == NXP_K24 )
      (void) _time_add_msec_to_ticks ( &phy.confirmTimeout, PHY_CONFIRM_TIMEOUT_MS );
#elif ( MCU_SELECTED == RA6E1 )
      ( void ) OS_TICK_Add_msec_to_ticks( &phy.confirmTimeout, PHY_CONFIRM_TIMEOUT_MS );
#endif
      if ( sendConfirm )
      {
         MAC_Request(pBuf);
      }
   }
}

/***********************************************************************************************************************

   \fn Function Name:  IsTxAllowed

   \brief Purpose:  Check if TX is allowed

   \param none

   \return Returns: TRUE or FALSE

***********************************************************************************************************************/
static bool IsTxAllowed( void )
{
#if ( TX_THROTTLE_CONTROL == 1 )
#if ( DCU == 1 )
   // Always allow TX when we send a Time Sync
   if( ((phy.tx_type == eTX_TYPE_STAR) && (phy.data[0] == (uint8_t)MTU2_MSG) && (phy.data[1] == (uint8_t)MTU2_TIME_SYNC)) ||                        // STAR Time Sync
       ((phy.tx_type == eTX_TYPE_SRFN) && (phy.Frame->MsgData.frame_type == MAC_CMD_FRAME) && (phy.Frame->MsgData.data[0] == MAC_TIME_SET_CMD)) )   // SRFN Time Sync
   {
      return TRUE;
   }
   else
#endif
   {
       //Throttling disabled or non-zero capacity means OK to transmit
       /* TODO SG: Make sure we have enough bytes to send the message?
                   At this time, allowing it to occasionally exceed is OK/acceptable by FCC */
      if ( !TxThrottle_enabled || ( TxThrottle != 0 ))
      {
         return TRUE;
      }
      else
      {
         /* TODO SG: Tell MAC to pause processing the Tx queue but still allow time sync's until next Tx Throttle window */
         return FALSE; // Throttling enabled and no capacity.
      }
   }
#else
      return TRUE;
#endif
}

#if ( TX_THROTTLE_CONTROL == 1 )
/***********************************************************************************************************************

   \fn Function Name:  TxThrottle_init

   \brief Purpose:  SInititalizes TX throttle

   \param None

   \return Returns:  returnStatus

   Note:

***********************************************************************************************************************/
static returnStatus_t TxThrottle_Init( void )
{
   returnStatus_t retVal;
   uint8_t        i;

   for ( i = 0; i < TX_THROTTLE_INTERVALS; i++ )
   {
      TxThrottleInterval[i] = TX_THROTTLE_INTV_INIT;
   }
   (void)memset( &TxThrottle_timer, 0, sizeof( TxThrottle_timer ) ); /* Clear the timer values */
   TxThrottle_timer.bOneShot        = (bool)true;
   TxThrottle_timer.ulDuration_mS   = TX_THROTTLE_PERIOD * TIME_TICKS_PER_MIN;
   TxThrottle_timer.pFunctCallBack  = (vFPtr_u8_pv)TxThrottleTimerCallBack;
   retVal                           = TMR_AddTimer( &TxThrottle_timer );     /* Create the throttle update timer   */
   if ( eSUCCESS == retVal )
   {
      TxThrottle_ID = TxThrottle_timer.usiTimerId;
   }
   return retVal;
}
/***********************************************************************************************************************

   \fn Function Name:  MAC_SetTxThrottle

   \brief Purpose:  Sets the TX throttle

   \param uint32_t throttle

   \return Returns:  void

   Note: this function is called from MAINBD_Set_TxThrottle(). If the values is 0, then low power mode is over; revert to normal throttle settings.

***********************************************************************************************************************/
void MAC_SetTxThrottle(uint32_t throttle)
{
   if ( throttle != 0 )                /* Low power mode just entered. */
   {  /* TODO SG: Re-evaluate to determine a better way to handle low power mode (i.e. change duty cycle to 3%) */
      TxThrottle         = throttle;
      TxMaxThrottle      = throttle;       /* Subsequent periods use this as the max throttle value.   */
      TxThrottle_enabled = (bool)true;
   }
   else                                /* Low power mode just exited. */
   {
      TxMaxThrottle = TX_THROTTLE_MAX_CAPACITY;   /* Subsequent periods use this as the max throttle value.   */
      TxThrottle    = TX_THROTTLE_MAX_CAPACITY;
   }
   TxThrottleSum = 0;
   memset( (uint8_t *)TxThrottleInterval, 0, sizeof ( TxThrottleInterval ) );
}

/***********************************************************************************************************************

   \fn Function Name:  MAC_GetTxThrottle

   \brief Purpose:  Sets the TX throttle

   \param uint32_t

   \return Returns:  void

***********************************************************************************************************************/
uint32_t MAC_GetTxThrottle(void)
{
   return TxThrottle;
}

/*******************************************************************************

   Function name: MAC_GetTxThrottleTimer

   Purpose: Used to manage the radio transmit throttle timer

   Arguments: Time out value (hours) - if 0, disable timer and re-establish transmitter throttling

   Returns: none

*******************************************************************************/
uint16_t MAC_GetTxThrottleTimer ( void )
{
   (void)TMR_ReadTimer( &TxThrottle_timer );
   return (uint16_t)( TxThrottle_timer.ulDuration_mS / TIME_TICKS_PER_MIN );
}

/*******************************************************************************

   Function name: MAC_TxThrottleTimerManage

   Purpose: Used to manage the radio transmit throttle timer

   Arguments: Hours to disable throttling. Zero enables throttling

   Returns: none

*******************************************************************************/
void MAC_TxThrottleTimerManage ( uint8_t hours )
{
   if ( hours != 0 )
   {
      (void)TMR_ResetTimer( TxThrottle_ID, TIME_TICKS_PER_HR * (uint32_t)hours );
      TxThrottle_enabled = (bool)false;
   }
   else
   {
      /* Set timer value to 1 tick -> 0 minutes. When timeout occurs, normal throttling picks up where it left off */
      (void)TMR_ResetTimer( TxThrottle_ID , 1U );
      (void)TMR_StopTimer( TxThrottle_ID );
      TxThrottle_enabled = (bool)true;
      (void)TMR_ReadTimer( &TxThrottle_timer );
   }
}

/***********************************************************************************************************************

   \fn Function Name:  UpdateTxThrottle

   \brief Purpose:  Update TX throttle by bytes transmitted

   \param none

   \return Returns:  void

***********************************************************************************************************************/
static void UpdateTxThrottle( void )
{
   if ( TxThrottle_enabled && ( TxThrottle != 0 ) ) { // TX throttling is enabled and there is capacity remaining
      THROTTLE_PRINT("Throttle was %d", TxThrottle );
#if ( DCU == 1 )
       // This is a STAR message
      if( phy.tx_type == eTX_TYPE_STAR )
      {
         // Skip update if this is a time sync
         TxThrottle -= min(TxThrottle, phy.size+PHY_STAR_OVERHEAD); // Don't let this wrap!
      }
      else
#endif
      {
         // This is a SRFN message
         // Skip update if this is a time sync
         TxThrottle -= min(TxThrottle, phy.size+PHY_SRFN_OVERHEAD); // Don't let this wrap!
      }
      THROTTLE_PRINT("Throttle now %d", TxThrottle );
   }
}

/*******************************************************************************

   Function name: TxThrottleTimerCallBack

   Purpose: This function is called when the radio tx throttle timer expires.
            When the timer expires, the capacity is set to the capacity available for the next period.

   Arguments:  uint8_t cmd - Not used, but required due to generic API
               void *pData - Not used, but required due to generic API

   Returns: none

   Notes:   TxThrottle_enabled is set to true.

*******************************************************************************/
static void TxThrottleTimerCallBack( uint8_t cmd, const void *pData )
{
   (void)cmd;
   (void)pData;
   static uint8_t  index = TX_THROTTLE_INTERVALS-1;         // Assume latest entry at power up
   uint8_t  oldIndex;

   THROTTLE_PRINT("Throttle old %d", TxThrottle );
   oldIndex                  = (index + 1) % TX_THROTTLE_INTERVALS;      // Index of oldest entry
   TxThrottleSum            -= TxThrottleInterval[oldIndex];             // Remove bytes used by oldest entry
   TxThrottleInterval[index] = TxMaxThrottle - TxThrottle;               // Insert bytes used during this interval
   TxThrottleSum            += TxThrottleInterval[index];                // New sum
   index                     = oldIndex;                                 // Newest index is now where oldest was
   TxMaxThrottle             = TX_THROTTLE_MAX_CAPACITY - TxThrottleSum; // Tx bytes available in the next interval
   TxThrottle                = TxMaxThrottle;
   TxThrottle_enabled        = (bool)true;
   THROTTLE_PRINT("Throttle new %d", TxThrottle );
}
#endif   /* ( TX_THROTTLE_CONTROL == 1 ) */

#if ( DCU == 1 )
/*!
 **********************************************************************
 *
 * \fn
 *
 * \brief
 *
 * \details
 *
 * \param
 *
 * \return  delay
 *
 **********************************************************************
 */
void MAC_TxPacketDelay()
{
   uint16_t delay_msec = MAC_ConfigAttr.TxPacketDelay;

   if(delay_msec > 0 )
   {
      INFO_printf("TxPacketDelay:%d msec:", delay_msec );
      OS_TASK_Sleep ( delay_msec );
   }
}

/***********************************************************************************************************************

   \fn Function Name:  MAC_STAR_DataIndication

   \brief Purpose: Used to handle a STAR Data Indication

   \param Arguments:  *phy_indication - Pointer to the phy indication.

   \return Returns: true  - Message was processed
                    false - Message was not processed

***********************************************************************************************************************/
bool MAC_STAR_DataIndication(const PHY_DataInd_t *phy_indication )
{
   bool retVal = (bool)false;

   // Check for STAR message
   if((phy_indication->framing == ePHY_FRAMING_1) || /* STAR Generation 1 */
      (phy_indication->framing == ePHY_FRAMING_2))   /* STAR Generation 2 */
   {
      /* CRC checked in PHY for star protocol */
      uint8_t starRecord[STAR_RECORD_SIZE];
      uint8_t slotNumber;
      uint8_t offset = MAX_OTA_STAR_BYTES;
      uint16_t crc;
      uint8_t index;
      int16_t avg_rssi;
      sysTime_t sysTimeTemp;
      sysTime_dateFormat_t DateTime;
      int16_t  NoiseEstimate[PHY_MAX_CHANNEL];
      uint16_t Channels[PHY_MAX_CHANNEL];
      uint8_t *ota_bytes = phy_indication->payload;
      uint16_t length = phy_indication->payloadLength;
      PHY_GetConf_t GetConf;

#if ( DCU == 1)
      uint8_t security = 0; // Default is security off

      // Retrieve current security mode
      if (VER_getDCUVersion() != eDCU2) {
         (void)APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &security, NULL );
      }

      // Filter out (ignore) MTU2_DCU2_COMMAND on DCU2+ to improve security as long as the DCU2+ is secured.
      // If the DCU2+ is not secured then we will accept STAR DCU-to-DCU commands.
      if ( (VER_getDCUVersion() == eDCU2) ||                                     // If Not a DCU2+, execute regardles
          ((ota_bytes[1] != (uint8_t)MTU2_DCU2_COMMAND) ||                       // Not a DCU-to-DCU command so process it.
          ((ota_bytes[1] == (uint8_t)MTU2_DCU2_COMMAND) && (security == 0))) )   // STAR DCU-to-DCU command but TB is not secure so process the command.
#endif
      {
         /* Begin multi-step process to get average RSSI from PHY.  First get the noise estimate for all channels */
         GetConf = PHY_GetRequest(ePhyAttr_NoiseEstimate);
         if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
            (void)memcpy((uint8_t *)NoiseEstimate, (uint8_t *)&GetConf.val, sizeof(NoiseEstimate));

            /* get the array of configured channels */
            GetConf = PHY_GetRequest(ePhyAttr_AvailableChannels);
            if ( GetConf.eStatus == ePHY_GET_SUCCESS ) {
               (void)memcpy((uint8_t *)Channels, (uint8_t *)&GetConf.val, sizeof(Channels));

               /* use channel from the phy indication to find correct index in Channels */
               for ( index = 0; index < PHY_MAX_CHANNEL; index++ )
               {
                  if ( Channels[index] == phy_indication->channel )
                  {
                     break;
                  }
               }
               if ( index >= PHY_MAX_CHANNEL )
               {
                  ERR_printf("PHY Data Indication channel value was not found in channel list");
                  avg_rssi = 0;
               }
               else
               {
                  /* index into Channels table will match the index into noise estimate table we want */
                  avg_rssi = (uint8_t)(NoiseEstimate[index] * -1);
               }

               (void)memcpy(starRecord, ota_bytes, length);
               if (length < MAX_OTA_STAR_BYTES)
               {
                  /* zero fill bytes if ota message is less than MAX_OTA_STAR_BYTES */
                  (void)memset(&starRecord[length], 0, (size_t)(MAX_OTA_STAR_BYTES - length));
               }

               /* length in nibbles */
               starRecord[offset++] = (uint8_t)(length*2);

               /* status (2400, FEC not applied, DCU CRC good, FEC not present, CRC good) */
               starRecord[offset] = 0;
               if (phy_indication->mode == ePHY_MODE_2)
               {
                  /* update baud rate bit to 7200 */
                  starRecord[offset] |= 0x20;
               }
               offset++;

               /* msg rssi msb */
               starRecord[offset++] = (uint8_t)(phy_indication->rssi * -1);

               MAC_SetLastMsgRSSI(phy_indication->rssi);

               /* deviation */
               starRecord[offset++] = (int8_t)(phy_indication->frequencyOffset / 16U);

               /* avg rssi msb */
               starRecord[offset++] = (uint8_t)avg_rssi;

               TIME_UTIL_ConvertSecondsToSysFormat(phy_indication->timeStamp.seconds,
                                                   phy_indication->timeStamp.QFrac,
                                                   &sysTimeTemp);
               (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sysTimeTemp, &DateTime);
               /* convert year to STAR system expectation - 0-63 for 2000-2063 */
               DateTime.year -= (uint16_t)2000;

               /* time */
               starRecord[offset++] = (uint8_t)((DateTime.hour / 10) << 4) | (DateTime.hour % 10);
               starRecord[offset++] = (uint8_t)((DateTime.min  / 10) << 4) | (DateTime.min  % 10);
               starRecord[offset++] = (uint8_t)((DateTime.sec  / 10) << 4) | (DateTime.sec  % 10);

               /* date */
               starRecord[offset++] = (uint8_t)((DateTime.month / 10) << 4) | (DateTime.month % 10);
               starRecord[offset++] = (uint8_t)((DateTime.day   / 10) << 4) | (DateTime.day   % 10);
               starRecord[offset++] = (uint8_t)((DateTime.year  / 10) << 4) | (DateTime.year  % 10);

               /* msg rssi/deviation */
               starRecord[offset] = (uint8_t)abs((int)(phy_indication->rssi * 10) % 10);
               starRecord[offset++] |= (phy_indication->frequencyOffset & 0xF) * 16U;

               /* extended avg rssi field is overwritten with Radio number */
               starRecord[offset] = phy_indication->receiverIndex;
               SLOT_ADDR(slotNumber);
               starRecord[offset++] |= (uint8_t)(slotNumber << 4);

               crc = CRC_16_DcuHack( starRecord, (STAR_RECORD_SIZE - 2) );
               starRecord[offset++] = (uint8_t)(crc & 0xFF);
               starRecord[offset  ] = (uint8_t)((crc >> 8) & 0xFF);

               MAINBD_GenerateStarRecord(starRecord);

               retVal = true;
            }
         }
      }
   }

   return retVal;
}

/***********************************************************************************************************************

   \fn Function Name:  MAC_GetLastMsgRSSI

   \brief Purpose:  returns the most recently received message RSSI

   \param Arguments:  none

   \return Returns:  int16_t - RSSI

   \details This wraps the Lock()/Unlock()

***********************************************************************************************************************/
float32_t MAC_GetLastMsgRSSI( void )
{
   float32_t rssi;

   OS_MUTEX_Lock(&Mutex_); // Function will not return if it fails
   rssi = STAR_last_msg_rssi;
   OS_MUTEX_Unlock( &Mutex_ );  // Function will not return if it fails

   return rssi;
}

/***********************************************************************************************************************

   \fn Function Name:  MAC_SetLastMsgRSSI

   \brief Purpose:  Sets the recently received message RSSI

   \param float rssi

   \return Returns:  void

   \details This wraps the Lock()/Unlock()

***********************************************************************************************************************/
static void MAC_SetLastMsgRSSI( float rssi )
{
   OS_MUTEX_Lock(&Mutex_); // Function will not return if it fails
   STAR_last_msg_rssi = rssi;
   OS_MUTEX_Unlock( &Mutex_ );  // Function will not return if it fails
}

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
/*! **********************************************************************************************************************

   \fn LinkParameters_ManageAlarm( void )

   \brief If the MAC-Link Parameters Period is valid and enabled, Create/ Update the  MAC Link Parameter Periodic Alarm.

   \param none

   \return void

   \note MAC Link-Parameter Periodic broadcast feature is disabled in DCU MFG Mode & in Porable DCUs

***********************************************************************************************************************/
static void LinkParameters_ManageAlarm ( void )
{
#if ( ( MFG_MODE_DCU == 0 ) && ( PORTABLE_DCU == 0 ) )
   if ( 0 != LinkParameters_Period_Get() )  /* TODO:  ( MAC_ConfigAttr.linkParam_Source == MAC_LINK_PARAM_SOURCE ). Maybe expand to add when Ep is also a LP source */
   {
      // MAC Link-Param period is valid and is enabled. Add periodic alarm
      tTimeSysPerAlarm alarmSettings;

      if ( NO_ALARM_FOUND != linkParam_P_AlaramID_ )
      { // Alarm already active, delete it
         (void)TIME_SYS_DeleteAlarm ( linkParam_P_AlaramID_ );
         linkParam_P_AlaramID_ = NO_ALARM_FOUND;
      }
      //Set up a periodic alarm
      (void)memset(&alarmSettings, 0, sizeof(alarmSettings));   // Clear settings, only set what we need
      alarmSettings.bOnValidTime    = true;                     /* Alarmed on valid time */
      alarmSettings.bSkipTimeChange = true;                     /* do NOT notify on time change */
      alarmSettings.pMQueueHandle   = MAC_GetMsgQueue();        /* Uses the message Queue */
      alarmSettings.ulPeriod        = LinkParameters_Period_Get() * TIME_TICKS_PER_SEC;
      alarmSettings.ulOffset        = ( MAC_ConfigAttr.LinkParamStart * TIME_TICKS_PER_SEC ) + aclara_randu(0, MAC_ConfigAttr.LinkParamMaxOffset );
      alarmSettings.ulOffset       %= alarmSettings.ulPeriod;   //offset can not be >= period
      alarmSettings.ulOffset        = (alarmSettings.ulOffset / SYS_TIME_TICK_IN_mS) * SYS_TIME_TICK_IN_mS; //round down to the tick

      (void)TIME_SYS_AddPerAlarm( &alarmSettings );

      linkParam_P_AlaramID_ = alarmSettings.ucAlarmId; //save the alarm ID
      LINK_PARAMS_PRNT_INFO('I',"LinkParam: P-Alarm Updated");
      LINK_PARAMS_PRNT_INFO('I',"LinkParam: Periodic Alarm ID: %d", linkParam_P_AlaramID_);

   }
   else
   {  // MAC Link-Param Periodic Broadcast is disabled. Delete alarm, if available
      (void)TIME_SYS_DeleteAlarm ( linkParam_P_AlaramID_ ) ;
      linkParam_P_AlaramID_ =  NO_ALARM_FOUND;
   }
#endif // end of ( MFG_MODE_DCU == 0 ) && ( PORTABLE_DCU == 0 )  section
}

/*! *****************************************************************************************************************************
 *
 * \fn static buffer_t* Link_Parameters_Create( uint8_t addr_mode, uint8_t dst_addr[MAC_ADDRESS_SIZE], MAC_dataConfCallback callback )
 *
 * \brief To create/build MAC Link Parameters Command ( DCU Only )
 *
 * \param[in]  uint8_t addr_mode - Address Mode [ Broadcast or Unitcast ]
 * \param[in]  uint8_t dst_addr  - MAC Address if the addr_mode is Unicast.
 * \param[in]  MAC_dataConfCallback callback  - Not used. Reserved for future use
 *
 * \return buffer_t *
 *
 * \todo Implementation of Noise Indication Handler
 * \todo Support more than one RPT
 ********************************************************************************************************************************/
static buffer_t* Link_Parameters_Create( uint8_t addr_mode, uint8_t dst_addr[MAC_ADDRESS_SIZE], MAC_dataConfCallback callback )
{
#define MAC_CMD_SIZE                1 // MAC Command Size : 1 Byte
#define LINK_PARAMETERS_CMD_SIZE    ((( LINK_PARAMS_NODE_ROLE_LENGTH + LINK_PARAMS_RESERVED_LENGTH + LINK_PARAMS_TX_POWER_LENGTH \
                                      + LINK_PARAMS_TX_POWER_HEADROOM_LENGTH + LINK_PARAMS_FREQUENCY_ACCURACY_LENGTH \
                                      + LINK_PARAMS_N_RPTS_LENGTH + LINK_PARAMS_CHANNEL_BAND_LENGTH + LINK_PARAMS_CHANNEL_OFFSET_LENGTH \
                                      + LINK_PARAMS_RPT_LENGTH )/8) + MAC_CMD_SIZE)  // In Bytes
   (void)callback;
   (void)dst_addr;

   buffer_t *pBuf = ReqBuffer_Alloc( eMAC_DATA_REQ, LINK_PARAMETERS_CMD_SIZE );

   if ( pBuf != NULL )
   {
      MAC_Request_t     *pReq = (MAC_Request_t*)pBuf->data; /*lint !e2445 !e740 !e826 payload holds the MAC_Request_t information */
      uint16_t          bitNo = 0;
      uint8_t           *pDst;
      PHY_GetConf_t     GetPHYConf;
      link_parameters_t macLinkParameters;
      rpt_info_t        rptInfo;

      memset( &macLinkParameters, 0, sizeof(macLinkParameters) );
      memset( &rptInfo, 0, sizeof(rptInfo) );

      pReq->pConfirmHandler                  = NULL;
      pReq->Service.DataReq.handle           = 0;
      pReq->Service.DataReq.dst_addr_mode    = (uint8_t)addr_mode;

      pReq->Service.DataReq.callback         = NULL;
      pReq->Service.DataReq.ackRequired      = (bool)false;
      pReq->Service.DataReq.droppable        = (bool)false;
      pReq->Service.DataReq.reliability      = eMAC_RELIABILITY_LOW;
      pReq->Service.DataReq.priority         = 5;                    // TODO: DG: Create an Enum
      pReq->Service.DataReq.channelSets      = eMAC_CHANNEL_SETS_SRFN;
      pReq->Service.DataReq.channelSetIndex  = eMAC_CHANNEL_SET_INDEX_1;

      // Create the CMD and encode the payload
      pReq->Service.DataReq.payload[0]       = MAC_LINK_PARAM_CMD;

      pDst = &pReq->Service.DataReq.payload[1];

      GetPHYConf = PHY_GetRequest( ePhyAttr_ReceivePowerThreshold ); // TODO: DG: Handle Error condition?
      memcpy( receivePowerThreshold_, GetPHYConf.val.ReceivePowerThreshold, sizeof( receivePowerThreshold_ ) ); // TODO: DG: Should be handled in Noise.Indication Handler

      GetPHYConf = PHY_GetRequest( ePhyAttr_RxChannels );
      (void)LinkParameters_RxPower_Threshold_Get( 1, &rptInfo.RPT, GetPHYConf.val.RxChannels ,eMAC_CHANNEL_SETS_SRFN, eMAC_CHANNEL_SET_INDEX_2, (bool)true );

      macLinkParameters.NodeRole          = LinkParameters_NodeRole_Get();
      macLinkParameters.TxPower           = HEEP_scaleNumber((double)PHY_TX_OUTPUT_POWER_DEFAULT,(int16_t)PHY_TX_OUTPUT_POWER_MIN,SCALE_LIMIT_MAX_TX_POWER );
      macLinkParameters.TxPowerHeadroom   = HEEP_scaleNumber((double)(PHY_TX_OUTPUT_POWER_MAX-PHY_TX_OUTPUT_POWER_DEFAULT),(int16_t)PHY_TX_OUTPUT_POWER_MIN,SCALE_LIMIT_MAX_TX_POWER);
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
      macLinkParameters.frequencyAccuracy = LinkParameters_FrequencyAccuracy_Get();
#endif

      bitNo = PACK_uint8_2bits( &(macLinkParameters.NodeRole),                     LINK_PARAMS_NODE_ROLE_LENGTH,pDst,bitNo);           // Node Role  ( 3 bits )
      bitNo = PACK_uint8_2bits( &(macLinkParameters.reserved),                     LINK_PARAMS_RESERVED_LENGTH,pDst,bitNo);            // Reserved  ( 5 bits )
      bitNo = PACK_uint8_2bits( (uint8_t *)&(macLinkParameters.TxPower),           LINK_PARAMS_TX_POWER_LENGTH,pDst,bitNo );           // TxPower (dBm) ( 8 bits )
      bitNo = PACK_uint8_2bits( (uint8_t *)&(macLinkParameters.TxPowerHeadroom),   LINK_PARAMS_TX_POWER_HEADROOM_LENGTH,pDst,bitNo );  // Tx Power Headroom (dBm) ( 8 bits )
      bitNo = PACK_uint8_2bits( (uint8_t *)&(macLinkParameters.frequencyAccuracy), LINK_PARAMS_FREQUENCY_ACCURACY_LENGTH,pDst,bitNo);  // Frequency Accuracy ( 3 bits )
      bitNo = PACK_uint8_2bits( &(uint8_t){LINK_PARAMS_N_RPTS_VALUE},              LINK_PARAMS_N_RPTS_LENGTH,pDst,bitNo);              // Number of RPTs Included  ( 5 bits )
      bitNo = PACK_uint8_2bits( (uint8_t *)&(rptInfo.channelBand),                 LINK_PARAMS_CHANNEL_BAND_LENGTH,pDst,bitNo);        // Channel 1 Band ( 4 bits )
      bitNo = PACK_uint16_2bits( (uint16_t *)&(rptInfo.channelOffset),             LINK_PARAMS_CHANNEL_OFFSET_LENGTH,pDst,bitNo);      // Channel 1 Offset ( 12 bits )
      bitNo = PACK_uint8_2bits( (uint8_t *)&(rptInfo.RPT),                         LINK_PARAMS_RPT_LENGTH,pDst,bitNo);                 // RPT ( 8 bits )

      pReq->Service.DataReq.payloadLength    = bitNo/8 + MAC_CMD_SIZE; // TODO: DG: Update the size based on the Number of RPTs included.

      LINK_PARAMS_PRNT_INFO('I',"NodeRole     :%d", macLinkParameters.NodeRole );
      LINK_PARAMS_PRNT_INFO('I',"TxPower      :%d", macLinkParameters.TxPower );
      LINK_PARAMS_PRNT_INFO('I',"FreqAccuracy :%d", macLinkParameters.frequencyAccuracy );
#if 0 // DBG Print
      LINK_PARAMS_PRNT_INFO('I',"TxPHeadRoom  :%d", macLinkParameters.TxPowerHeadroom );
      LINK_PARAMS_PRNT_INFO('I',"ChannelBand  :%d", rptInfo.channelBand );
      LINK_PARAMS_PRNT_INFO('I',"ChannelOffset:%d", rptInfo.channelOffset );
#endif
   }

   return pBuf;
}

/*! ***********************************************************************************************************************

  \fn returnStatus_t MAC_LinkParametersPush_Request( uint8_t broadCastMode, uint8_t *srcAddr, MAC_ConfirmHandler confirm_cb )

  \brief Build & Push the Mac-LinkParameters Command

  \param[in]  uint8_t broadCastMode
  \param[in]  uint8_t *srcAddr - Reserved for future use
  \param[in]  MAC_ConfirmHandler confirm_cb - Reserved for future use

  \return returnStatus_t eSUCCESS or eFAILURE

  \note Only BROADCAST_MODE is supported for now.

***********************************************************************************************************************/
returnStatus_t MAC_LinkParametersPush_Request ( uint8_t broadCastMode, uint8_t *srcAddr, MAC_ConfirmHandler confirm_cb )
{
   PHY_GetConf_t  GetConf;
   char           floatStr[PRINT_FLOAT_SIZE];
   buffer_t       *pBuf;
   float          localThreshold;
   returnStatus_t eStatus = eSUCCESS; //Default
   uint8_t        maxRPT;

   (void)confirm_cb;

   if( BROADCAST_MODE == broadCastMode )
   {
      GetConf = PHY_GetRequest( ePhyAttr_txEnabled );
      /* Check for being installed in a backplane, a transmitter available on the TB, and enabled. If not, send the link parameters request to another TB.  */
      if ( ( ePHY_GET_SUCCESS == GetConf.eStatus ) && MAINBD_IsPresent() && !( TxChannels_init( eMAC_CHANNEL_SETS_SRFN, eMAC_CHANNEL_SET_INDEX_1 ) && GetConf.val.txEnabled ) )
      {
         uint8_t starRecord[STAR_RECORD_SIZE] = {0};
         uint8_t offset = STAR_OPCODE_INDEX;

         /* Force update of local power control data  */
         GetConf = PHY_GetRequest( ePhyAttr_RxChannels );
         (void)LinkParameters_RxPower_Threshold_Get( 1U, &maxRPT, GetConf.val.RxChannels, eMAC_CHANNEL_SETS_SRFN, eMAC_CHANNEL_SET_INDEX_2, (bool)false );
         localThreshold = MAC_Get_maxLocalThreshold();

         (void)memset( starRecord, 0xFF, 8 ); // First 8 bytes are 0xFF, rest is 0
         starRecord[offset++] = (uint8_t)STAR_SRFN_LinkParameters;
         (void)DBG_printFloat( floatStr, localThreshold, 3 );
         offset += (uint8_t)snprintf( (char *)&starRecord[ offset ], sizeof( starRecord ) - offset, "%hhu,%p,%p,%s", broadCastMode, srcAddr, confirm_cb, floatStr );

         // Add meta data to STAR message and send to the main board
         STAR_add_metadata( starRecord, offset, 0 );
      }
      else
      {
         //Try to send the Link Parameters Message
         LINK_PARAMS_PRNT_INFO('I',"Sending MAC-Link Parameters");

         pBuf = Link_Parameters_Create( broadCastMode, srcAddr, NULL );
         if ( pBuf != NULL )
         {  // Create a new MAC Transaction, and add to the Transaction Que
            if( !MAC_Transaction_Create ( pBuf, MAC_CMD_FRAME, (bool)false ) )
            {  // Free the buffer
               BM_free( pBuf );
               ERR_printf("ERROR: MAC_Transaction_Create() failed");
               eStatus = eFAILURE;
            }
         }
      }
   }

   return eStatus;
}

/*! ***********************************************************************************************************************
 *
 * \fn MAC_LinkParameters_Config_Print()
 *
 * \brief Used to print the MAC Link-Parameters Config
 *
 * \param none
 *
 * \return void
 *
 * \note Meant to be called only from DBG port
 * ***********************************************************************************************************************/
void MAC_LinkParameters_Config_Print( void )
{
   INFO_printf("LinkParameter_MaxOffset = %d", MAC_ConfigAttr.LinkParamMaxOffset );
   INFO_printf("LinkParameter_Period    = %ld",LinkParameters_Period_Get() );
   INFO_printf("LinkParameter_Start     = %ld",MAC_ConfigAttr.LinkParamStart );
}

/*! **************************************************************************************************************************
 *
 * \fn LinkParameters_Period_Get()
 *
 * \brief Used to get the value of the MAC Link Parameter Period
 *
 * \param none
 *
 * \return uint32_t LinkParamPeriod
 *
 * ***********************************************************************************************************************/
static uint32_t LinkParameters_Period_Get( void )
{
   return ( MAC_ConfigAttr.LinkParamPeriod );
}

/*! ***********************************************************************************************************************
 *
 * \fn LinkParameters_NodeRole_Get()
 *
 * \brief Used to get the Node Role
 *
 * \param none
 *
 * \return uint8_t ( bitmapped field 0: EP, 1: FNG, 2: IAG, 4: Router)
 *
 * ***********************************************************************************************************************/
static uint8_t LinkParameters_NodeRole_Get( void )
{
  return ( ( (uint8_t)MAC_ConfigAttr.IsFNG | (uint8_t)MAC_ConfigAttr.IsIAG << 1U | (uint8_t)MAC_ConfigAttr.IsRouter << 2U ) );
}

/*! **************************************************************************************************************************
 *
 * \fn LinkParameters_RxPower_Threshold_Get ( uint8_t nChannels, uint8_t *RxPowerThreshold, uint16_t const *RxChannels,
 *                                            MAC_CHANNEL_SETS_e channelSets, MAC_CHANNEL_SET_INDEX_e channelSetIndex, bool useOtherTBmax )
 *
 * \brief Return the RPT Avg* or RPTs per channel based on the input
 *
 * \param[in]   uint8_t  nChannels - Number of RxChannels of RPTs to return
 *                                 - Valid Values : 1 to return the Max RPT and PHY_RCVR_COUNT to return RPT per RxChannel
 * \param[out]  uint8_t  *RxPowerThreshold - RPTs are scaled as per MAC-Link Parameters Requirement
 * \param[in]   uint16_t *RxChannels - PHY Rx Channel Config
 * \param[in]   MAC_CHANNEL_SETS_e channelSets
 * \param[in]   MAC_CHANNEL_SET_INDEX_e channelSetIndex
 * \param[in]   bool useOtherTBmax - true means include otherTBpowerThreshold in computing report value
 *
 * \return returnStatus_t
 *
 * \note Only Aggregate of RPTs is supported at this time.
 * ***********************************************************************************************************************/
returnStatus_t LinkParameters_RxPower_Threshold_Get( uint8_t nChannels, uint8_t *RxPowerThreshold, uint16_t const *RxChannels, MAC_CHANNEL_SETS_e channelSets,
                                                      MAC_CHANNEL_SET_INDEX_e channelSetIndex, bool useOtherTBmax )
{
   float             maxRPT = MAC_RPT_BASE;                 // Assign Min value. RPT value could never be less than -130.0f
   uint8_t           nValidRPTs = 0;                        // Number of Valid Records in receivePowerThreshold_ list
   //Note:DG: nValidRPTs could have been avoided, but we might use it in the future and might be helpful for Diagnostics
   CH_SETS_t         filter = { 0, PHY_INVALID_CHANNEL-1 }; // Default filter includes all channels
#if ( ENABLE_PRNT_MAC_LINK_PARAMS_INFO == 1 )
   char              floatStr[PRINT_FLOAT_SIZE];
#endif

#if 0  // DBG purpose
   LINK_PARAMS_PRNT_INFO('I', "SET0 Filter.Start %ld ",MAC_ConfigAttr.ChannelSetsSRFN[0].start);
   LINK_PARAMS_PRNT_INFO('I', "SET0 Filter.Stop %ld ",MAC_ConfigAttr.ChannelSetsSRFN[0].stop);
   LINK_PARAMS_PRNT_INFO('I', "SET1 Filter.Start %ld ",MAC_ConfigAttr.ChannelSetsSRFN[1].start);
   LINK_PARAMS_PRNT_INFO('I', "SET1 Filter.Stop %ld ",MAC_ConfigAttr.ChannelSetsSRFN[1].stop);
   LINK_PARAMS_PRNT_INFO('I', "SET2 Filter.Start %ld ",MAC_ConfigAttr.ChannelSetsSRFN[2].start);
   LINK_PARAMS_PRNT_INFO('I', "SET2 Filter.Stop %ld ",MAC_ConfigAttr.ChannelSetsSRFN[2].stop);
#endif
   // Filter RX channels
   if ( eMAC_CHANNEL_SET_INDEX_2 == channelSetIndex ) // Filter on a set on which gateway devices are expected to receive ( set 2 ).
   {
      channelSetIndex--; // Convert from 1 based to 0 based
      if ( channelSets < eMAC_CHANNEL_SETS_LAST ) // Sanity check
      {
         if ( channelSets == eMAC_CHANNEL_SETS_SRFN )
         {
            filter = MAC_ConfigAttr.ChannelSetsSRFN[channelSetIndex]; // Get channel set limits
            LINK_PARAMS_PRNT_INFO('I', "Filter.Start %ld ",filter.start);
            LINK_PARAMS_PRNT_INFO('I', "Filter.Stop %ld ",filter.stop);
         }
         else if ( channelSets == eMAC_CHANNEL_SETS_STAR )
         {
            filter = MAC_ConfigAttr.ChannelSetsSTAR[channelSetIndex]; // Get channel set limits
         }
      }
      for ( uint8_t i = RADIO_FIRST_RX; i < PHY_RCVR_COUNT; i++ ) // Number of PhyRxChannels is same as PHY_RCVR_COUNT
      {
         // Is it a valid Number?
         if ( isnormal( receivePowerThreshold_[i] ) )   /*lint !e26 !e505 !e10 !e746 !e718*/
         {
            // Filter Rx channel based on channel set. No need to check the validity of Channel as the PHY takes care of that.
            if ( ( RxChannels[i] >= filter.start ) && ( RxChannels[i] <= filter.stop ) )
            {
               if( 1 == nChannels ) // Find the Max RPT
               {
                  if ( receivePowerThreshold_[i] > maxRPT )
                  {
                     maxRPT = receivePowerThreshold_[i];
                  }
               }
               nValidRPTs++;
            }
            else
            {
               LINK_PARAMS_PRNT_INFO('E',"Incorrect Channel Config! RX Channel Index: %d", i );
            }
         }
         else
         {
            LINK_PARAMS_PRNT_INFO('I',"NaN! RX_Ch Index :%d", i);
         }
      }
      maxLocalThreshold = maxRPT;      /* Keep latest threshold, so it can be sent to primary TB, if needed.   */
      if ( 1 == nChannels )
      {
         if( 0 == nValidRPTs )
         {
            // Use the default values
            LINK_PARAMS_PRNT_INFO('I',"Use DEFAULTS to calculate RPT");
            maxRPT = ( float )( PHY_BACKGROUND_RSSI_AVG_DEFAULT + ( 2 * PHY_BACKGROUND_RSSI_STD_DEVIATION_DEFAULT ) + PHY_HARD_DEMODULATOR_SINR_DEFAULT + RECEIVE_POWER_MARGIN_DEFAULT );
            maxLocalThreshold = maxRPT;      /* Keep latest threshold, so it can be sent to primary TB, if needed.   */
         }
         // Print Info
         LINK_PARAMS_PRNT_INFO( 'I',"nValidRPTs = %d", nValidRPTs );
         LINK_PARAMS_PRNT_INFO( 'I',"2nd TB RPT = %-6s", DBG_printFloat( floatStr, otherTBpowerThreshold, 3 ) );
         LINK_PARAMS_PRNT_INFO( 'R',"RPT        = %-6s", DBG_printFloat( floatStr, maxRPT, 3 ) );

         //Range Check
         if( maxRPT < MAC_RPT_BASE )
         {
            maxRPT = MAC_RPT_BASE;
            LINK_PARAMS_PRNT_INFO('I',"RPT<-130");
         }
         if( maxRPT > MAX_RPT )
         {
            maxRPT = MAX_RPT;
            LINK_PARAMS_PRNT_INFO('I',"RPT>-66");
         }
         /* Check for power threshold received from secondary TB  */
         if ( useOtherTBmax && ( otherTBpowerThreshold != 0 )  )
         {
            if ( otherTBpowerThreshold > maxRPT )
            {
               maxRPT = otherTBpowerThreshold;
            }
         }
         *RxPowerThreshold = ( uint8_t )roundf( MAC_SCALE_RPT( maxRPT ) );  // Update
         LINK_PARAMS_PRNT_INFO( 'R',"RPTScaled = %d", *RxPowerThreshold );
      }
      else
      {
         // TODO: To be handled in the future
         LINK_PARAMS_PRNT_INFO('E',"Not Supported. nChannels: %d", nChannels);
      }
   }
   else
   {
      // TODO: To be handled in the future
      LINK_PARAMS_PRNT_INFO('E',"Invalid channelSetIndex: %d", channelSetIndex);
   }

   return ( eSUCCESS );
}

/*!
*************************************************************************************************************************
 *
 * \fn MAC_LinkParameters_EventDriven_Push()
 *
 * \brief Determine if we need to broadcast LinkParameters MAC command, if so create the command and Push it to the MAC Queue
 *
 * \param none
 *
 * \return void
 *
 * \details
 * ***********************************************************************************************************************/
void MAC_LinkParameters_EventDriven_Push( void )
{
   if ( 0 != LinkParameters_Period_Get() ) // If the feature is enabled
   {
      if( send_MacLinkParams_ ) // Do we need to broadcast the MAC-Link Parameters?
      {
         send_MacLinkParams_ = (bool)false; // Clear the flag
         (void)MAC_LinkParametersPush_Request( BROADCAST_MODE, NULL, NULL );
         // Note: DG: It's okay if we were unable to add Link-Parameters to the Tx Queue
      }
   }
}

/*!
**********************************************************************************************************************
 *
 * \fn LinkParameters_FrequencyAccuracy_Get( void )
 *
 * \brief Get the "scaled "Frequency Accuracy required for MAC-Link-Parameters command
 *
 * \param none
 *
 * \return Frequency Accuracy in PPM in scaled format
 *
 * \note Frequency Accuracy in PPM in scaled format. TODO: Frequency Accuracy should be in the PHY and
 *       this function should get the value and scale based on the MAC-Link Parameters command
 ************************************************************************************************************************/
static uint8_t LinkParameters_FrequencyAccuracy_Get( void )
{
   if( TIME_SYS_IsGpsPresent() && TIME_SYS_IsGpsTimeValid() && TIME_SYS_IsGpsPhaseLocked() )
   {
      // Note: If GPS present, the frequency Accuracy is 0.1 PPM, which is 0 when scaled
      return ( MAC_LINK_PARAMS_FREQUENCY_ACCURACY_WITH_GPS );
   }
   else
   {
      // Note: If GPS is NOT present, the frequency Accuracy is 2.0 PPM, which is 4 when scaled
      return ( MAC_LINK_PARAMS_FREQUENCY_ACCURACY_WITHOUT_GPS );
   }
}
/************************************************************************************************************************/
#endif // end of ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )section

#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
/*!
************************************************************************************************************************
 *
 * \fn macCommandResponse_MaxTimeDiversity_Set
 *
 * \brief Set the Mac Command Response MaxTimeDiversity ( timeout in ms )
 *
 * \param uint16_t timeoutMs
 *
 * \return MAC_SET_STATUS_e
 *
 * \details range 0 to 10,000 ms
 * ***********************************************************************************************************************/
static MAC_SET_STATUS_e macCommandResponse_MaxTimeDiversity_Set( uint16_t timeoutMs )
{
   MAC_SET_STATUS_e eStatus = eMAC_SET_INVALID_PARAMETER;
   if ( timeoutMs <= MAC_CMD_RESP_MAX_TIME_DIVERSITY_LIMIT )
   {
      MAC_ConfigAttr.macCmdRespMaxTimeDiversity = timeoutMs;
      eStatus = eMAC_SET_SUCCESS;
   }

   return eStatus;
}
/*!
***********************************************************************************************************************
 *
 * \fn Random_CommandResponseTimeDiversity_Get
 *
 * \brief Generate and get the Random Time Diversity if the feature is enabled
 *
 * \param none
 *
 * \return uint32_t - Random MAC Command Response time diversity
 *
 * ***********************************************************************************************************************/
static uint32_t Random_CommandResponseTimeDiversity_Get( void )
{
   uint32_t timeDiversityMs = 0;

   if ( 0 != MAC_ConfigAttr.macCmdRespMaxTimeDiversity ) // If the feature is enabled
   {
      timeDiversityMs = aclara_randu( 0, MAC_ConfigAttr.macCmdRespMaxTimeDiversity );
   }

   return ( timeDiversityMs );
}
/*!
***********************************************************************************************************************

   \fn MAC_CommandResponse_TimeDiversityTimer_CB

   \brief Call Back function for MAC Command Response TimeDiversity Timer

   \param uint8_t cmd - from the ucCmd member when the timer was created.
                     void *pData - from the pData member when the timer was created

   \return void

   \note Also clears the TimeSync.inTimeDiversity "flag"

***********************************************************************************************************************/
static void MAC_CommandResponse_TimeDiversityTimer_CB( uint8_t cmd, void *pData )
{
   (void) cmd;
   (void) pData;

   static buffer_t buff;

   TimeSync.InTimeDiversity = false; // Not in TimeDiversity Anymore

   /***
   * Use a static buffer instead of allocating one because of the allocation fails,
   * there wouldn't be much we could do - we couldn't reset the timer to try again
   * because we're already in the timer callback.
   ***/
   BM_AllocStatic( &buff, eSYSFMT_TIME_DIVERSITY );
   buff.data = (uint8_t*)&macCmdResp_TimeDiversity_TimerID_;

   OS_MSGQ_Post( &MAC_msgQueue, ( void * )&buff );
}

/*!
************************************************************************************************************************
*
* \fn  macCommandResponse_TimeDiversityTimer_Create
*
* \brief Create the MAC-CommandResponse Time Diversity Timer
*
* \param None
*
* \return returnStatus_t
*
* \details
*
***********************************************************************************************************************/
static returnStatus_t macCommandResponse_TimeDiversityTimer_Create( void )
{
   timer_t tmrSettings;            // Timer for re-issuing metadata registration
   returnStatus_t retVal = eFAILURE;

   (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
   tmrSettings.bOneShot       = true;
   tmrSettings.bExpired       = true;
   tmrSettings.bStopped       = true;
   tmrSettings.ulDuration_mS  = MAC_CMD_RESP_MAX_TIME_DIVERSITY_DEFAULT;
   tmrSettings.pFunctCallBack = MAC_CommandResponse_TimeDiversityTimer_CB;

   if ( eSUCCESS == TMR_AddTimer(&tmrSettings) )
   {
      macCmdResp_TimeDiversity_TimerID_ = tmrSettings.usiTimerId;
      retVal = eSUCCESS;
   }

   return retVal;
}
/*!
*************************************************************************************************************************
 *
 * \fn macCommandResponse_TimeDiversityTimer_Start()
 *
 * \brief Start the MAC-Command Response Time Diversity Timer
 *
 * \param uint32_t timeDiversityMs
 *
 * \return void
 *
 * ***********************************************************************************************************************/
static void macCommandResponse_TimeDiversityTimer_Start( uint32_t timeDiversityMs )
{
   INFO_printf( "Start MAC_Command_Response_Time_Diversity Timer, ID : %ld \n", macCmdResp_TimeDiversity_TimerID_ );
   (void)TMR_ResetTimer( macCmdResp_TimeDiversity_TimerID_, timeDiversityMs );
}
#endif // end of ( MAC_CMD_RESP_TIME_DIVERSITY == 1 ) section

#endif // end ( DCU == 1 ) section

#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( EP == 1 ) )
/*!************************************************************************************************************************
 *
 * \fn Process_LinkParametersCommand( MAC_DataInd_t const *pDataInd )
 *
 * \brief
 *
 * \param none
 *
 * \return void
 *
 * ***********************************************************************************************************************/
static void Process_LinkParametersCommand( MAC_DataInd_t const *pDataInd )
{
   uint16_t          bitNo = 0;
   uint8_t const     *payload = &pDataInd->payload[1];
   char              floatStr[PRINT_FLOAT_SIZE];
   link_parameters_t macLinkParameters;
   rpt_info_t        rptInfo;

   (void)memset( &macLinkParameters, 0, sizeof( macLinkParameters ));
   (void)memset( &rptInfo, 0, sizeof( rptInfo ));

   macLinkParameters.NodeRole          = UNPACK_bits2_uint8( payload, &bitNo, LINK_PARAMS_NODE_ROLE_LENGTH );
   macLinkParameters.reserved          = UNPACK_bits2_uint8( payload, &bitNo, LINK_PARAMS_RESERVED_LENGTH );
   macLinkParameters.TxPower           = UNPACK_bits2_uint8( payload, &bitNo, LINK_PARAMS_TX_POWER_LENGTH );
   macLinkParameters.TxPowerHeadroom   = UNPACK_bits2_uint8( payload, &bitNo, LINK_PARAMS_TX_POWER_HEADROOM_LENGTH );
   macLinkParameters.frequencyAccuracy = UNPACK_bits2_uint8( payload, &bitNo, LINK_PARAMS_FREQUENCY_ACCURACY_LENGTH );
   macLinkParameters.nRPTsIncluded     = UNPACK_bits2_uint8( payload, &bitNo, LINK_PARAMS_N_RPTS_LENGTH );
   // TODO: DG: Handle more than 1 RPTs
   rptInfo.channelBand                 = UNPACK_bits2_uint16( payload, &bitNo, LINK_PARAMS_CHANNEL_BAND_LENGTH );
   rptInfo.channelOffset               = UNPACK_bits2_uint16( payload, &bitNo, LINK_PARAMS_CHANNEL_OFFSET_LENGTH );
   rptInfo.RPT                         = UNPACK_bits2_uint8( payload, &bitNo,  LINK_PARAMS_RPT_LENGTH );

   LINK_PARAMS_PRNT_INFO('I',"NodeRole         :%d", macLinkParameters.NodeRole );
   LINK_PARAMS_PRNT_INFO('I',"reserved         :%d", macLinkParameters.reserved );
   LINK_PARAMS_PRNT_INFO('I',"TxPower          :%d", macLinkParameters.TxPower );
   LINK_PARAMS_PRNT_INFO('I',"TxPower_UNSCALED :%-6s",DBG_printFloat(floatStr,HEEP_UnscaleNumber(macLinkParameters.TxPower,(int16_t)0,(int16_t)SCALE_LIMIT_MAX_TX_POWER),2));
   LINK_PARAMS_PRNT_INFO('I',"TxPowerHeadroom  :%d", macLinkParameters.TxPowerHeadroom );
   LINK_PARAMS_PRNT_INFO('I',"frequencyAccuracy:%d", macLinkParameters.frequencyAccuracy );
   LINK_PARAMS_PRNT_INFO('I',"nRPTsIncluded    :%d", macLinkParameters.nRPTsIncluded );
   LINK_PARAMS_PRNT_INFO('I',"channelBand      :%d", rptInfo.channelBand );
   LINK_PARAMS_PRNT_INFO('I',"channelOffset    :%d", rptInfo.channelOffset );
   LINK_PARAMS_PRNT_INFO('I',"RPT              :%d", rptInfo.RPT );
   LINK_PARAMS_PRNT_INFO('I',"RPT_UNSCALED     :%-6s",DBG_printFloat(floatStr,HEEP_UnscaleNumber(rptInfo.RPT,(int16_t)MAC_RPT_BASE,(int16_t)MAX_RPT),2));
}
#endif
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
void MAC_Set_otherTBpowerThreshold( float maxRPT )
{
   otherTBpowerThreshold = maxRPT;
}
float MAC_Get_maxLocalThreshold( void )
{
   return maxLocalThreshold;
}
#endif

#define UNIT_TEST_RPT_SCALING   0
#if ( UNIT_TEST_RPT_SCALING == 1 )
// TODO:  Add Time Sync Unit Test
// Configure the Time Diversity to Max
// Simulate Multiple Time Requests during :
  // 1. During Time Diversity
  // 2. During Blocking Window
  // 3. After the Blocking Window

static void MAC_SCALE_RPT_UnitTest( void );
static void MAC_SCALE_RPT_UnitTest( void )
{
   uint8_t rpt_scaled = 0;
   float  rpt = -130.00;
   char   floatStr[PRINT_FLOAT_SIZE];
   while( rpt <= -66.00 )
   {
      rpt_scaled = HEEP_scaleNumber( rpt,(int16_t)MAC_RPT_BASE, (int16_t)MAX_RPT );
      DBG_logPrintf( 'R', "RPT input= %-6s, RPT_SCALED: %d ", DBG_printFloat( floatStr, rpt, 3 ), rpt_scaled );
      rpt += 0.25;
   }
}
#endif // end of ( UNIT_TEST_RPT_SCALING == 1 ) section
