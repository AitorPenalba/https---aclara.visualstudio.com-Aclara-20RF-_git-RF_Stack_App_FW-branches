/***********************************************************************************************************************
 *
 * Filename: MAC_Protocol.h
 *
 * Contents: typedefs and #defines for the MAC protocol implementation
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

#ifndef MAC_PROTOCOL_H
#define MAC_PROTOCOL_H

/* INCLUDE FILES */
#include <stdint.h>
#include "PHY_Protocol.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

// todo: 6/15/2015 5:53:29 PM [BAH]
#define MAC_FRAME_VERSION_0  0   /* Version */
#define DEFAULT_NETWORK_ID   0   /* Customer specific network id */

/*! MAC Frame Types */
#define MAC_RESERVED_FRAME       (0)
#define MAC_DATA_FRAME           (1)
#define MAC_ACK_FRAME            (2)
#define MAC_CMD_FRAME            (3)
#define MAC_STAR_DATA_FRAME      (4)   /* Added for STAR Normal Message */
#define MAC_STAR_FAST_MESSAGE    (5)   /* Added for STAR Fast Message   */

/*! Destination Address Modes */
#define BROADCAST_MODE     (0)   /* Broadcast message.  No destination address field present */
#define UNICAST_MODE       (1)   /* EUI-64 destination address included */
#define IGNORE_QUERY_MODE  (2)   /* This is not a address mode but it is used as part as some address mode comparison */

/*! Acknowledgement Required */
#define NO_ACK_REQUIRED    (0)
#define ACK_REQUIRED       (1)

/*! Acknowledgement Present */
#define NO_ACK_PRESENT     (0)
#define ACK_PRESENT        (1)

/*! Segmentation Enabled */
#define FRAME_NOT_SEGMENTATED    (0)
#define FRAME_SEGMENTATED        (1)
#define MAX_VALID_SEGMENT_COUNT  (11)  /* Maximum number of segments per message (0 based, so 12 segments)   */

#define MAC_OUI_ADDR_SIZE (3)   // Size of the EUI-Organizatioanally Unique Identifier (24-bit)
#define MAC_XID_ADDR_SIZE (5)   /* Size of the EUI-Extension Identifier                (40-bits) */
#define MAC_EUI_ADDR_SIZE (8)   // Size of the EUI-Extended Unique Identifier (EUI-64) (64-bit)

#define MAC_ADDRESS_SIZE  (MAC_XID_ADDR_SIZE)  /* Same as EUI-Extension Identifier (40-bits) */

/* 2 bytes of control, 5 bytes of source address, and 4 bytes of CRC */
#define MIN_MAC_HEADER_LEN ((uint8_t)11)

#define MAC_TIME_SET_CMD      0u   /*!< Time Set Commmand Type     */
#define MAC_TIME_REQ_CMD      1u   /*!< Time Request Command Type  */
#define MAC_PING_REQ_CMD      2u   /*!< Ping Request  Command Type  DCU -> EP  */
#define MAC_PING_RSP_CMD      3u   /*!< Ping Response Command Type  EP  -> DCU */
#define MAC_LINK_PARAM_CMD    4u   /* Mac Link Parameters Command DCU -> EP */

#define MAC_MAX_CHANNEL_SETS_SRFN 15
#define MAC_MAX_CHANNEL_SETS_STAR  2

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* The EUI-64 is an IEEE defined value which is the concatenation of the Organizationally Unique Identifier (OUI)
 * assigned by the IEEE Registration Authority and the extension identifier assigned by Aclara.  The Aclara OUI value is
 * a 24 bit value, or OUI-24.  Expressed in hexadecimal, the Aclara OUI-24 is 0x001D24.
 * Note: the EUI-64 is globally unique and as such care must be taken that Aclara does not assign duplicate extension
 * identifiers across any product lines. */

typedef uint8_t oui_addr_t[MAC_OUI_ADDR_SIZE];  /*!< The 3-byte Organizationally Unique Identifier  */
typedef uint8_t xid_addr_t[MAC_XID_ADDR_SIZE];  /*!< The 5-byte Extension Identifier                */
typedef uint8_t eui_addr_t[MAC_EUI_ADDR_SIZE];  /*!< The 8-byte Extended Unique Identifier (EUI-64) */

typedef union
{
   struct
   {
      oui_addr_t oui;            /*!< Contains the 3-byte Organizationally Unique Identifier */
      xid_addr_t xid;            /*!< Contains the 5-byte Extension Identifier.              */
   };
   eui_addr_t eui;               /*!< Contains the 8-bytes of the eui address. */
}macAddr_t;                      /* Mac address - 64-bit Extended Unique Identifier (EUI-64) */

/*!
 * Channel sets range
 */
typedef struct
{
   uint16_t start;  /* First channel of the set */
   uint16_t stop;   /* Last channel of the set */
} CH_SETS_t;

/*!
 * MAC Channel Sets
 */
typedef enum{
   eMAC_CHANNEL_SETS_SRFN, // List of SRFN channel sets
   eMAC_CHANNEL_SETS_STAR, // List of STAR channel sets
   eMAC_CHANNEL_SETS_LAST
} MAC_CHANNEL_SETS_e;

/*!
 * MAC Channel Set index
 */
typedef enum{
   eMAC_CHANNEL_SET_INDEX_ANY,
   eMAC_CHANNEL_SET_INDEX_1, // Used for SRFN/STAR downstream (a.k.a. outbound)
   eMAC_CHANNEL_SET_INDEX_2, // Used for SRFN upstream (a.k.a. inbound)
   eMAC_CHANNEL_SET_INDEX_LAST
} MAC_CHANNEL_SET_INDEX_e;


/*!
 * Requests Supported by MAC
 */
typedef enum
{
   eMAC_GET_REQ,        /*!< MAC Get   Request */
   eMAC_RESET_REQ,      /*!< MAC Reset Request */
   eMAC_SET_REQ,        /*!< MAC Set   Request */
   eMAC_START_REQ,      /*!< MAC Start Request */
   eMAC_STOP_REQ,       /*!< MAC Stop  Request */
#if ( EP == 1 )
   eMAC_TIME_QUERY_REQ,       /*!< MAC Time Query Request */
#endif
   eMAC_DATA_REQ,       /*!< MAC Data  Request */
   eMAC_PURGE_REQ,      /*!< MAC Purge Request */
   eMAC_FLUSH_REQ,      /*!< MAC Flush Request */
   eMAC_PING_REQ        /*!< MAC Ping Request */
} MAC_REQ_TYPE_t;

/*!
 * Confirmations Returned from MAC
 */
typedef enum
{
   eMAC_GET_CONF,                   /*!< MAC Get   Confirm */
   eMAC_RESET_CONF,                 /*!< MAC Reset Confirm */
   eMAC_SET_CONF,                   /*!< MAC Set   Confirm */
   eMAC_START_CONF,                 /*!< MAC Start Confirm */
   eMAC_STOP_CONF,                  /*!< MAC Stop  Confirm */
#if ( EP == 1 )
   eMAC_TIME_QUERY_CONF,                  /*!< MAC Time Query Confirm */
#endif
   eMAC_DATA_CONF,                  /*!< MAC Data  Confirm */
   eMAC_PURGE_CONF,                 /*!< MAC Purge Confirm */
   eMAC_FLUSH_CONF,                 /*!< MAC Flush Confirm */
   eMAC_PING_CONF                   /*!< MAC Ping Confirm */
} MAC_CONF_TYPE_t;

/*!
 * Indication types from the MAC
 */
typedef enum
{
   eMAC_DATA_IND,        /*!< PHY_DATA_INDICATION  PHY to MAC */
   eMAC_COMM_STATUS_IND, /*!< Indicates network activity */
} MAC_IND_TYPE_t;

/*!
 * MAC Get/Set Attributes
 */
typedef enum
{
   eMacAttr_AcceptedFrameCount,            /* Read Only */
   eMacAttr_AckDelayDuration,              /* Read Only */
   eMacAttr_AckWaitDuration,               /* Read Only */
   eMacAttr_AdjacentNwkFrameCount,         /* Read Only */
   eMacAttr_ArqOverflowCount,              /* Read Only */
   eMacAttr_ChannelAccessFailureCount,     /* Read Only */
   eMacAttr_ChannelSetsSRFN,
   eMacAttr_ChannelSetsSTAR,
   eMacAttr_ChannelSetsCountSRFN,          /* Read Only */
   eMacAttr_ChannelSetsCountSTAR,          /* Read Only */
   eMacAttr_CsmaMaxAttempts,
   eMacAttr_CsmaMaxBackOffTime,
   eMacAttr_CsmaMinBackOffTime,
   eMacAttr_CsmaPValue,
   eMacAttr_CsmaQuickAbort,
   eMacAttr_CsmaSkip,
   eMacAttr_DuplicateFrameCount,
   eMacAttr_FailedFcsCount,                /* Read Only */
   eMacAttr_InvalidRecipientCount,         /* Read Only */
   eMacAttr_LastResetTime,                 /* Read Only */
   eMacAttr_MalformedAckCount,             /* Read Only */
   eMacAttr_MalformedFrameCount,           /* Read Only */
   eMacAttr_NetworkId,                     /* Read Only */
   eMacAttr_PacketConsumedCount,           /* Read Only */
   eMacAttr_PacketFailedCount,             /* Read Only */
   eMacAttr_PacketId,                      /* Read Only */
   eMacAttr_PacketReceivedCount,           /* Read Only */
   eMacAttr_PacketTimeout,
   eMacAttr_PingCount,                     /* Read Only */
   eMacAttr_ReassemblyTimeout,
   eMacAttr_ReliabilityHighCount,
   eMacAttr_ReliabilityLowCount,
   eMacAttr_ReliabilityMediumCount,
   eMacAttr_RxFrameCount,                  /* Read Only */
   eMacAttr_RxOverflowCount,               /* Read Only */
   eMacAttr_State,                         /* Read Only */
   eMacAttr_TransactionOverflowCount,      /* Read Only */
   eMacAttr_TransactionTimeout,
   eMacAttr_TransactionTimeoutCount,       /* Read Only */
   eMacAttr_TxFrameCount,                  /* Read Only */
   eMacAttr_TxLinkDelayCount,              /* Read Only */
   eMacAttr_TxLinkDelayTime,               /* Read Only */
   eMacAttr_TxPacketCount,                 /* Read Only */
   eMacAttr_TxPacketFailedCount,           /* Read Only */

#if ( DCU == 1 )
   eMacAttr_TxPacketDelay,
#endif
   eMacAttr_TxSlot,
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
   eMacAttr_NumMac,
#endif
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   eMacAttr_LinkParamMaxOffset,
   eMacAttr_LinkParamPeriod,
   eMacAttr_LinkParamStart,
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
   eMacAttr_CmdRespMaxTimeDiversity,
#endif

#if 1
   eMacAttr_IsChannelAccessConstrained,
   eMacAttr_IsFNG,
   eMacAttr_IsIAG,
   eMacAttr_IsRouter,
#endif



} MAC_ATTRIBUTES_e;

/*!
 * MAC state
 */
typedef enum
{
   eMAC_STATE_IDLE,           // Not operational
   eMAC_STATE_OPERATIONAL     // Operational
}MAC_STATE_e;

/*!
 * Start.Request - parameters
 *
 * No parameters!
 */

/*!
 * MAC Start - Confirmation Status
 */
typedef enum
{
   eMAC_START_SUCCESS = 0,      /*!< MAC START - Success */
   eMAC_START_RUNNING,          /*!< MAC START - Running */
} MAC_START_STATUS_e;

/*!
 * MAC Stop - Confirmation Status
 */
typedef enum
{
   eMAC_STOP_SUCCESS = 0,      /*!< MAC START - Success */
   eMAC_STOP_ERROR,            /*!< MAC START - Error   */
} MAC_STOP_STATUS_e;

/*!
 * Start.Request
 */
typedef struct
{
   uint32_t dummy;
} MAC_StartReq_t;

/*!
 * Start.Confirm
 */
typedef struct
{
   MAC_START_STATUS_e eStatus;
} MAC_StartConf_t;


/*!
 * Stop.Request
 */
typedef struct
{
   uint32_t dummy;
} MAC_StopReq_t;

/*!
 * Stop.Confirm
 */
typedef struct
{
   MAC_STOP_STATUS_e eStatus;
} MAC_StopConf_t;


/*!
 * Reset.Request - parameters
 */
typedef enum
{
   eMAC_RESET_ALL,       /*!< Reset all statistics */
   eMAC_RESET_STATISTICS /*!< Reset some staistics */
} MAC_RESET_e;

/*!
 * MAC Reset - Confirmation Status
 */
typedef enum
{
   eMAC_RESET_SUCCESS = 0,       /*!< PHY RESET - Success */
   eMAC_RESET_FAILURE            /*!< PHY RESET - Failure */
} MAC_RESET_STATUS_e;

/*!
 * Reset.Request
 */
typedef struct
{
   MAC_RESET_e eType;            /*!< ALL, STATISTICS */
} MAC_ResetReq_t;

/*!
 * Reset.Confirm
 */
typedef struct
{
   MAC_RESET_STATUS_e eStatus;  /*!< SUCCESS, FAILURE */
} MAC_ResetConf_t;


/*!
 * MAC Get - Confirmation Status
 */
typedef enum
{
   eMAC_GET_SUCCESS = 0,        /*!< MAC GET - Success */
   eMAC_GET_UNSUPPORTED,        /*!< MAC GET - Unsupported attribute */
   eMAC_GET_SERVICE_UNAVAILABLE /*!< MAC GET - Service Unavailable */
} MAC_GET_STATUS_e;


/*!
 * Get.Request
 */
typedef struct
{
   MAC_ATTRIBUTES_e eAttribute;  /*!< Specifies the attribute requested */
} MAC_GetReq_t;

/*!
 * MAC Set - Confirmation Status
 */
typedef enum
{
   eMAC_SET_SUCCESS = 0,        /*!< MAC SET - Success */
   eMAC_SET_READONLY,           /*!< MAC SET - Read only */
   eMAC_SET_UNSUPPORTED,        /*!< MAC SET - Unsupported attribute */
   eMAC_SET_INVALID_PARAMETER,  /*!< MAC SET - Invalid parameter */
   eMAC_SET_SERVICE_UNAVAILABLE /*!< MAC SET - Service Unavailable */
} MAC_SET_STATUS_e;

/*!
 * MAC Get/Set Parameters
 */
typedef union
{
   uint32_t AcceptedFrameCount;          /*!< */
   uint16_t AckDelayDuration;            /*!< The number of milliseconds to wait after the receipt of a frame with AR
                                              set to TRUE before sending
                                              an ACK frame. Read Only */
   uint16_t AckWaitDuration;             /*!< The maximum amount of time in milliseconds to wait for an acknowledgement
                                              to arrive following a transmitted data or command frame where AR was set
                                              to TRUE - Read Only */
   uint32_t AdjacentNwkFrameCount;       /*!< The number of received frames that were sent from a device whose network
                                              ID did ... Read Only */
   uint32_t ArqOverflowCount;            /*!< The number of received frames where the SN was outside of the allowable
                                              receive window. Read Only */
   uint32_t ChannelAccessFailureCount;   /*!< The number of times the MAC failed to access the channel and aborted the
                                              packet.        Read Only */
   CH_SETS_t ChannelSets[max(MAC_MAX_CHANNEL_SETS_SRFN,MAC_MAX_CHANNEL_SETS_STAR)];
                                         /*!< The list of SRFN channel sets that the MAC supports for
                                              transmission of packets. */
   uint8_t  ChannelSetsCount;            /*!< The number of defined channel sets within macChannelSets. */
   uint8_t  CsmaMaxAttempts;             /*!< The maximum number of attempts by the MAC to gain access to the channel
                                              before failing the transaction request. (0-20) */
   uint16_t CsmaMaxBackOffTime;          /*!< The maximum amount of back off time in milliseconds for the CSMA
                                              algorithm. (???) */
   uint16_t CsmaMinBackOffTime;          /*!< The minimum amount of back off time in milliseconds for the CSMA algorithm
                                              (0-TBD) */
   float    CsmaPValue;                  /*!< The probability that the CSMA algorithm will decide to transmit when an
                                              idle channel is found.  (0-1, default 0.1) */
   bool     CsmaQuickAbort;              /*!< */
   uint8_t  CsmaSkip;
   uint32_t DuplicateFrameCount;         /*!< */
   uint32_t FailedFcsCount;              /*!< Read Only - The number of received frames that failed the frame check
                                              sequence validation. */
   uint32_t InvalidRecipientCount;       /*!< */
   TIMESTAMP_t LastResetTime;            /*!< The date and time of the last RESET operation with SetDefault = TRUE. */
   uint32_t MalformedAckCount;           /*!< Read Only - The number of received frames (ACK frame or piggyback) where
                                              the acknowledgement */
   uint32_t MalformedFrameCount;         /*!< Read Only - The number of received frames that failed to be parsed due to
                                              a malformed MAC header.  */
   uint8_t  NetworkId;                   /*!< The identifier for the network on which the device is permitted to
                                              communicate.* (0-15) */
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
   uint8_t  NumMac;                      /*!<  Number of MAC addresses this unit emulates   */
#endif
   uint32_t PacketConsumedCount;         /*!< */
   uint32_t PacketFailedCount;           /*!< */
   uint8_t  PacketId;                    /*!< The ID of the current in-process packet (0-3) (Read Only) */
   uint32_t PacketReceivedCount;         /*!< */
   uint16_t PacketTimeout;               /*!< The number of milliseconds to maintain a given device's packet ID pair
                                              before clearing the information from the MAC.
                                              Range 0-65535, default = 2000*/
   uint16_t PingCount;                   /*!< Read Only - The number of MAC layer pings received */
   uint8_t  ReassemblyTimeout;           /*!< The amount of time in seconds allowed to elapse between received segments
                                              of a single packet before the MAC will discard the entire packet and
                                              request any associated ARQ windows.*/
   uint8_t  ReliabilityHighCount;        /*!< The number of repeats or retries that are used to satisfy the QoS
                                              reliability level of high for packets that are sent with AR = FALSE or
                                              TRUE respectively. (0-7, default = 2) */
   uint8_t  ReliabilityLowCount;         /*!< The number of repeats or retries that are used to satisfy the QoS
                                              reliability level of low for packets that are sent with AR = FALSE or
                                              TRUE (0-7, default = 0) */
   uint8_t  ReliabilityMediumCount;      /*!< The number of repeats or retries that are used to satisfy the QoS
                                              reliability level of medium for packets that are sent with AR = FALSE or
                                              TRUE respectively. (0-7, default = 1) */
   uint32_t RxFrameCount;                /*!< The number of frames received from the PHY layer.
                                              Read Only */
   uint32_t RxOverflowCount;             /*!< The number of times that a frame was received but was unable to be
                                              processed due (Read Only) */
   MAC_STATE_e State;                    /*!< The current state of the MAC layer */
   uint32_t TransactionOverflowCount;    /*!< The number of transaction requests rejected because the transaction queue
                                              was full. Read Only */
   uint16_t TransactionTimeout;          /*!< The number of seconds that a transaction is allowed to be in process before being aborted. */
   uint32_t TransactionTimeoutCount;     /*!( The number of times the MAC had to abort a transaction for taking too long.
                                              Read Only */
   uint32_t TxFrameCount;                /*!< The number of frames transmitted by the PHY layer.
                                              Read Only */
   uint32_t TxLinkDelayCount;            /*!( The number of times the MAC had to delay a frame due to the PHY indicating a delay was necessary.
                                              Read Only */
   uint32_t TxLinkDelayTime;             /*!( The total amount of time in milliseconds that the MAC has delayed due to the PHY indicating a delay was necessary.
                                              Read Only */
   uint32_t TxPacketCount;               /*!< The number of packets transmitted by the MAC layer.
                                              Read Only */
#if ( DCU == 1 )
   uint16_t TxPacketDelay;               /*!< */
#endif
   uint32_t TxPacketFailedCount;         /*!< The number of packets failed      by the MAC layer.
                                              Read Only */
   uint8_t  TxSlot;
#if ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
   uint16_t LinkParamMaxOffset;          /*!< The maximum amount of random offset in milliseconds from timeSetStart for which
                                                the source will begin transmitting the time set MAC command. */
   uint32_t LinkParamPeriod;             /*!< The period, in seconds, for which the time set MAC command broadcast
                                                schedule is computed. */
   uint32_t LinkParamStart;              /*!< The start time in seconds relative to the source time of midnight UTC
                                               for starting the broadcast schedule. Range: 0-86399, default = 1765 */
#endif
#if ( MAC_CMD_RESP_TIME_DIVERSITY == 1 )
   uint16_t macCmdRespMaxTimeDiversity;  /*!< The window of time in which DCU may add the Response MAC command message to its transmit queue.( msecs ) */
#endif

#if 1
// for now these are at the end.  Does not really matter
   bool     IsChannelAccessConstrained;  /*!< */
   bool     IsFNG;                       /*!< */
   bool     IsIAG;                       /*!< */
   bool     IsRouter;                    /*!< */
#endif
} MAC_ATTRIBUTES_u;


/*! Get.Confirm */
typedef struct
{
   MAC_ATTRIBUTES_e  eAttribute;  /*!< Specifies the attribute requested */
   MAC_GET_STATUS_e  eStatus;     /*!< Execution status SUCCESS, UNSUPPORTED */
   MAC_ATTRIBUTES_u  val;         /*!< Value Returned */
} MAC_GetConf_t;

/*! Set.Confirm */
typedef struct
{
   MAC_ATTRIBUTES_e  eAttribute;  /*!< Specifies the attribute */
   MAC_SET_STATUS_e  eStatus;     /*!< SUCCESS, READONLY, UNSUPPORTED */
} MAC_SetConf_t;

/*!
 * Set.Request
 */
typedef struct
{
   MAC_ATTRIBUTES_e  eAttribute;  /*!< Specifies the attribute to set */
   MAC_ATTRIBUTES_u  val;         /*!< Specifies the value to set     */
} MAC_SetReq_t;

/*!
* Time_Push.Request
*/
typedef struct
{
   uint32_t dummy;
} MAC_TimePushReq_t;

typedef enum
{
   eMAC_TIME_SUCCESS,              /*!< MAC TIME - Success              */
   eMAC_TIME_INVALID_REQUEST,      /*!< MAC TIME - Invalid request      */
   eMAC_TIME_MAC_IDLE,             /*!< MAC TIME - MAC in idle state    */
   eMAC_TIME_PENDING,              /*!< MAC TIME - Will send when MAC out of blocking mode */
   eMAC_TIME_TRANSACTION_FAILED,   /*!< MAC TIME - Transaction Failed   */
   eMAC_TIME_TRANSACTION_OVERFLOW  /*!< MAC TIME - Transaction Overflow */
} MAC_TIME_PUSH_STATUS_e;

/*!
* Time_Push.Confirm
*/
typedef struct
{
   MAC_TIME_PUSH_STATUS_e eStatus;
} MAC_TimePushConf_t;

/*!
* Time_Query.Request
*/
typedef struct
{
   MAC_CHANNEL_SET_INDEX_e channelSetIndex; /* Index in a given list of sets */
} MAC_TimeQueryReq_t;

typedef enum
{
   eMAC_TIME_QUERY_SUCCESS,
   eMAC_TIME_QUERY_MAC_IDLE,
   eMAC_TIME_QUERY_TRANSACTION_FAILED,
   eMAC_TIME_QUERY_TRANSACTION_OVERFLOW
} MAC_TIME_QUERY_STATUS_e;

/*!
* Time_Query.Confirm
*/
typedef struct
{
   MAC_TIME_QUERY_STATUS_e eStatus;
} MAC_TimeQueryConf_t;

/*!
 * Flush.Request
 */
typedef struct
{
   uint32_t dummy;
} MAC_FlushReq_t;

typedef enum
{
   eMAC_FLUSH_SUCCESS,     /*!< MAC FLUSH - Success              */
   eMAC_FLUSH_MAC_IDLE     /*!< MAC FLUSH - MAC in idle state    */
} MAC_FLUSH_STATUS_e;

/*!
 * Flush.Confirm
 */
typedef struct
{
   MAC_FLUSH_STATUS_e eStatus;
} MAC_FlushConf_t;


/*!
 * Purge.Request
 */
typedef struct
{
   uint16_t handle;     /*!< The handle of the packet to be purged.  This value is the same handle used in the original
                             DATA.request. */
}MAC_PurgeReq_t;

typedef enum
{
   eMAC_PURGE_SUCCESS,          /*!< MAC PURGE - Success              */
   eMAC_PURGE_IDLE,             /*!< MAC PURGE - MAC in idle state    */
   eMAC_PURGE_INVALID_HANDLE    /*!< MAC PURGE - Invalid handle       */
} MAC_PURGE_STATUS_e;

/*!
 * Purge.Confirm
 */
typedef struct
{
   MAC_PURGE_STATUS_e eStatus;
   uint16_t handle;
} MAC_PurgeConf_t;

/* MAC reliability values */
typedef enum
{
   eMAC_RELIABILITY_LOW = 0,
   eMAC_RELIABILITY_MED,
   eMAC_RELIABILITY_HIGH
} MAC_Reliability_e;

typedef enum
{
    eMAC_DATA_SUCCESS = 0,           /**< MAC DATA REQ - packet successfully transmitted (both acknowledged and
                                          unacknowledged). */
    eMAC_DATA_MAC_IDLE,              /**< MAC DATA REQ - MAC In idle state */
    eMAC_DATA_TRANSACTION_OVERFLOW,  /**< MAC DATA REQ - Transaction buffer is full */
    eMAC_DATA_TRANSACTION_FAILED,    /**< MAC DATA REQ - Failed after exhausting all MAC retries. */
    eMAC_DATA_INVALID_PARAMETER,     /**< MAC DATA REQ - Invalid request parameter */
    eMAC_DATA_INVALID_HANDLE         /**< MAC DATA REQ - Invalid handle */
} MAC_DATA_STATUS_e;


typedef enum
{
    eMAC_PING_SUCCESS = 0,           /**< MAC PING REQ - packet successfully transmitted (both acknowledged and
                                          unacknowledged). */
    eMAC_PING_MAC_IDLE,              /**< MAC PING REQ - MAC In idle state */
    eMAC_PING_TRANSACTION_OVERFLOW,  /**< MAC PING REQ - Transaction buffer is full */
    eMAC_PING_TRANSACTION_FAILED,    /**< MAC PING REQ - Failed after exhausting all MAC retries. */
    eMAC_PING_INVALID_PARAMETER,     /**< MAC PING REQ - Invalid request parameter */
    eMAC_PING_INVALID_HANDLE         /**< MAC PING REQ - Invalid handle */
} MAC_PING_STATUS_e;

typedef void (*MAC_dataConfCallback)( MAC_DATA_STATUS_e eStatus, uint16_t handle );


/*!
 * \brief Time Set - Command Frame Type (64 bits)
 */
typedef struct
{
   TIMESTAMP_t ts;            /* Time stamp received in time-sync message*/
   uint8_t  LeapIndicator;    /* Indicators if a leap second will be inserted or deleted in the last minute of the
                                 current UTC day */
   uint8_t  Poll;             /* Describes the maximum interval between successive time set commands sent.  This is an
                                 unsigned integer in log base 2 format.  The value is set to the closest power of 2 to
                                 macTimeSetPeriod  */
   int8_t   Precision;        /* This describes the resolution of the system clock on the transmitting device.
                                 This is sent as a signed integer in log base 2 format. */
}  time_set_t;

/*!
 * \brief Time Request - Command Frame Type (0 bits)
 */
// typedef struct time_req_t;


/*!
 * Command Frame Type
 */
typedef struct
{
   uint8_t cmd_id;
   union
   {
      time_set_t             time_set;              // Time Set Command               32 bits
//    time_req_t             time_req;              // Request For Current Time        0 bits
   };
} cmd_frame_t;

typedef struct {
   /* frame control */
   uint8_t frame_version;         /* 4 bit. frame version */
   uint8_t frame_type;            /* 2 bit. Frame type field */
   uint8_t dst_addr_mode;         /* 1 bit. Is destination address present? */
   uint8_t ack_required;          /* 1 bit. Is an ack frame required */
   uint8_t ack_present;           /* 1 bit. Is ack present? */
   uint8_t segmentation_enabled;  /* 1 bit. Is frame part of larger packet */
   uint8_t packet_id;             /* 2 bit. ID for the packet */
   /* addressing */
   uint8_t network_id;                 /*  4 bit. Network ID (typically unique between neighboring customers) */
   uint8_t src_addr[MAC_ADDRESS_SIZE]; /* 40 bit. Source address (extension identifier of EUI-64) */
   uint8_t dst_addr[MAC_ADDRESS_SIZE]; /* 40 bit. Destination address (extension identifier of EUI-64) */
   /* ack */
   uint8_t seq_num;              /* 4 bit. ID of this frame */
   uint8_t req_num;              /* 4 bit. ID of next needed frame (ACKs all previous frames */
   /* Framing */
   uint8_t segment_id;           /* 4 bit. ID of this segment of this frame */
   uint8_t segment_count;        /* 4 bit. Total number of segments in this packet */
   uint16_t length;              /* 16 bit. length of the MAC payload in bytes */
   uint16_t thisFrameLen;        /* how many of the payload bytes to send in this frame (mac header - phy size limit) */
   union{
      uint8_t       data[PHY_MAX_PAYLOAD]; /*!< Raw payload for this segment */
      cmd_frame_t   cmd_frame;             /*!< Command Frame */
   };
} mac_frame_t;

/* Data.Request  and Ping.Request */

/* MAC Transmit Type */
typedef enum
{
   eTX_TYPE_SRFN = 0,
   eTX_TYPE_STAR = 1
} MAC_TxType_e;

typedef struct
{
   uint16_t             handle;                      /* A handle needed by the Data.Confirm to reference the request.
                                                        0-65535 */
   MAC_TxType_e         tx_type;                     /* Transmit Type STAR/SRFN */
   uint8_t              dst_addr_mode;               /* Destination Address Mode */
   uint8_t              dst_addr[MAC_ADDRESS_SIZE];  /* Destination Address */
   uint16_t             payloadLength;               /* The size of the payload in Bytes */
   bool                 ackRequired;
   uint8_t              priority;
   bool                 droppable;
   MAC_Reliability_e    reliability;
   MAC_CHANNEL_SETS_e   channelSets;                 /* Which sets to use: SRFN or STAR */
   MAC_CHANNEL_SET_INDEX_e channelSetIndex;          /* Index in a given list of sets */
   bool                 fastMessage;                 /* flag to specify if message should be sent ASAP (skip CSMA, different PHY behavior) */
   bool                 skip_cca;                    /* flag to specify if mac will do cca */
   MAC_dataConfCallback callback;                    /* optional function pointer to be called with tx result (QoS dictates 'success') */
   uint8_t              payload[];
} MAC_DataReq_t, MAC_PingReq_t;

/* Data.Confirm */
typedef struct
{
   uint16_t          handle;        /*!< A handle used to reference the associated Data.Request. 0-65535 */
   uint16_t          payloadLength; /*! The size of the payload in bytes */
   MAC_DATA_STATUS_e status;        /*!< Data Confirmation Status */  /*lint !e123 */
} MAC_DataConf_t;

/* Data.Confirm */
typedef struct
{
   uint16_t         handle;      /*!< A handle used to reference the associated Data.Request. 0-65535 */
   MAC_PING_STATUS_e eStatus;    /*!< Ping Confirmation Status */  /*lint !e123 */
} MAC_PingConf_t;


/* Data.Indication */
typedef struct
{
   TIMESTAMP_t timeStamp;               /* TimeStamp associated with when the payload was received */
   uint32_t timeStampCYCCNT;            /* CYCCNT time stamp associated with when the payload was received */
   uint16_t rssi;                       /* RSSI value for this frame */
   uint16_t danl;                       /* DANL value for this frame */
   uint16_t channel;                    /* channel the packet was recieved on */
   uint8_t segmentCount;                /* number of segments that made up this packet */
   uint8_t src_addr[MAC_ADDRESS_SIZE];  /* 40 bit. Source address (extension identifier of EUI-64) */
   uint8_t dst_addr_mode;               /* Destination address mode */
   uint8_t dst_addr[MAC_ADDRESS_SIZE];  /* 40 bit. Destination address (extension identifier of EUI-64) */
   uint16_t payloadLength;              /* The size of the payload in bits */
   uint8_t  payload[];                  /* Payload Data */
} MAC_DataInd_t;


/* Ping.Indication */
typedef struct
{
   uint16_t    Handle;           /* The unique 16-bit identifier from the device that initiated the Ping Request MAC
                                    command.*/
   eui_addr_t  src_eui;          /* The EUI-64 extension address of the device that sent the Ping Response MAC
                                    command.*/
   eui_addr_t  origin_eui;       /* The EUI-64 extension address of the device that initiated the Ping Request MAC
                                    command.*/
   uint8_t     CounterResetFlag; /* Boolean  TRUE, FALSE Indicates if the receiving device was directed reset its ping
                                    counter.*/
   uint8_t     RspMode;          /* Enumeration BROADCAST, UNICAST   The destination addressing mode that the receiver
                                    was directed to use.*/
   TIMESTAMP_t ReqTxTime;        /* The time that the Ping Request  MAC command frame was transmitted as determined by
                                    the transmitting device.*/
   TIMESTAMP_t ReqRxTime;        /* The time that the Ping Request  MAC command frame was received as determined by the
                                    receiving device.*/
   TIMESTAMP_t RspTxTime;        /* The time that the Ping Response MAC command frame was transmitted as determined by
                                    the transmitting device.*/
   TIMESTAMP_t RspRxTime;        /* The time that the Ping Response MAC command frame was received as determined by the
                                    receiving device.*/
   int16_t     ReqRssi;          /* The RSSI value measured during reception of the Ping Request MAC command.
                                    In units of 0.1dBm. */
   int16_t     RspRssi;          /* The RSSI value measured during reception of the Ping Response MAC command.
                                    In units of 0.1dBm. */
   int16_t     ReqPNLevel;       /* The perceived noise level measured immediately after receipt of the Ping Request
                                    MAC command.  Dividing this value by 16 and subtracting 200 yields RSSI in units  of dBm. */
   int16_t     RspPNLevel;       /* The perceived noise level measured immediately after receipt of the Ping Response
                                    MAC command.  Dividing this value by 16 and subtracting 200 yields RSSI in units  of dBm. */
   uint16_t    PingCount;        /* The number of Ping Request MAC commands the device that sent the Ping Response MAC
                                    command has received since its last ping reset operation.*/
   uint16_t    ReqChannel;       /* Channel on which the Ping request was received*/
   uint16_t    RspChannel;       /* Channel on which the Ping response was received*/


} MAC_PingInd_t;

/* COMM-STATUS.indication */
typedef struct
{
   uint8_t network_id;
   uint8_t frame_type;
   uint8_t src_addr[MAC_ADDRESS_SIZE];
   uint8_t dst_addr_mode;
   uint8_t dst_addr[MAC_ADDRESS_SIZE];
   uint16_t payloadLength;
   TIMESTAMP_t timeStamp;
   uint32_t timeStampCYCCNT;            /* CYCCNT time stamp associated with when the payload was received */
   uint16_t rssi;
   uint16_t channel;
} MAC_Comm_Status_t;

typedef struct
{
   MAC_CONF_TYPE_t Type;
   uint32_t        handleId;         /*!< Handle for the request */
   union
   {
      // Management Services
      MAC_GetConf_t      GetConf;    /*!< Get   Confirmation from MAC   */
      MAC_ResetConf_t    ResetConf;  /*!< Reset Confirmation from MAC   */
      MAC_SetConf_t      SetConf;    /*!< Set   Confirmation from MAC   */
      MAC_StartConf_t    StartConf;  /*!< Start Confirmation from MAC   */
      MAC_StopConf_t     StopConf;   /*!< Get   Confirmation from MAC   */
      MAC_TimePushConf_t     TimePushConf;   /*!< Time  Confirmation from MAC   */
      MAC_TimeQueryConf_t TimeQueryConf;
      // Data Services
      MAC_DataConf_t     DataConf;   /*!< Data  Confirmation from MAC   */
      MAC_PingConf_t     PingConf;   /*!< Ping  Confirmation from MAC   */
      MAC_FlushConf_t    FlushConf;  /*!< Flush Confirmation from MAC */
      MAC_PurgeConf_t    PurgeConf;  /*!< Purge Confirmation from MAC */
   };
} MAC_Confirm_t;


/*!
 * This specifies the indication interface from the MAC.
 */
typedef struct
{
   MAC_IND_TYPE_t        Type;
   union
   {
      MAC_DataInd_t      DataInd;    /*!< Data Indication from the MAC */
      MAC_Comm_Status_t  CommStatus;
   };
} MAC_Indication_t;

typedef void (*MAC_ConfirmHandler)(MAC_Confirm_t const *pConfirm);

typedef union
{
   // Management Services
   MAC_GetReq_t     GetReq;     /*!< Get   Request to MAC */
   MAC_ResetReq_t   ResetReq;   /*!< Reset Request to MAC */
   MAC_SetReq_t     SetReq;     /*!< Set   Request to MAC */
   MAC_StartReq_t   StartReq;   /*!< Start Request to MAC */
   MAC_StopReq_t    StopReq;    /*!< Stop  Request to MAC */
   MAC_TimePushReq_t    TimePushReq;    /*!< Time  Request to MAC */
   MAC_TimeQueryReq_t   TimeQueryReq;   /*!< Time  Request to MAC */
   // Data Services
   MAC_DataReq_t    DataReq;    /*!< Data  Request to MAC */
   MAC_FlushReq_t   FlushReq;   /*!< Flush Request to MAC */
   MAC_PurgeReq_t   PurgeReq;   /*!< Purge Request to MAC */
   MAC_PingReq_t    PingReq;    /*!< Ping  Request to MAC */
} MAC_Services_u;

typedef struct
{
   MAC_REQ_TYPE_t     MsgType;
   uint32_t           handleId;    /*!< Handle for the request */
   MAC_ConfirmHandler pConfirmHandler;
   MAC_Services_u     Service;
} MAC_Request_t;

/* Prototypes */

#endif /* MAC_PROTOCOL_H */
