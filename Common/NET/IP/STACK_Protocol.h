/***********************************************************************************************************************
 *
 * Filename:   STACK_Protocol.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * Copyright (c) 2014 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 **********************************************************************************************************************/
#ifndef NWK_PROTOCOL_H
#define NWK_PROTOCOL_H

/* INCLUDE FILES */
#include "MAC_Protocol.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


#define IP_VERSION 0

/*! Next Header Compression */
#define NEXT_HEADER_NOT_PRESENT  (0x00)
#define NEXT_HEADER_PRESENT      (0x01)

#define IPV6_ADDRESS_SIZE (16)
#define MULTICAST_ADDRESS_SIZE (6)
#define OFFSET_TO_EUI40 ((uint8_t)11)

#define SIZE_OF_PRE_ADDR_FIELDS (2)

#define DEFAULT_HEAD_END_CONTEXT (0)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   /*
    * WARNING: Changing this enumaration will impact the value of IP_PORT_COUNT.
    *          If IP_PORT_COUNT is changed, it will mess up the statistics.
    *          To avoid this, a new statistics version needs to be defined.
    *          Also, new enumerations should only be added and never be conditionnaly compiled as to avoid IP_PORT_COUNT having different values accross products.
    *          Again, this as to do with statistics.
    *
    */
   UDP_NONDTLS_PORT = 0,
   UDP_DTLS_PORT = 1,
#if ( USE_MTLS == 1 ) || (DCU == 1 ) // Remove DCU == 1 when MTLS is enabled for TB
   UDP_MTLS_PORT = 2,
#endif
#if (USE_IPTUNNEL == 1)
   UDP_IP_NONDTLS_PORT = 3,
   UDP_IP_DTLS_PORT = 4,
#endif
   IP_PORT_COUNT           /* Number of ports for which stats are collected.  */
} TRlayers;

/*!
 * NWK State
 */
typedef enum{
   eNWK_STATE_IDLE,           // Not operational
   eNWK_STATE_OPERATIONAL     // Operational
}NWK_STATE_e;

/*!
 * NWK Get/Set Attributes
 */
typedef enum
{
   eNwkAttr_ipIfInDiscards,                   /*!< The number of packets received that were discarded due to no valid route being found */
   eNwkAttr_ipIfInErrors,                     /*!< The number of packets received via the them from being further propagated through the network. */
   eNwkAttr_ipIfInMulticastPkts,              /*!< The number of packets, delivered by this layer to the upper layers, which were addressed to a multicast address at this layer. */
   eNwkAttr_ipIfInOctets,                     /*!< The total number of octets received via the corresponding link layer including network overhead.  */
   eNwkAttr_ipIfInPackets,                    /*!< The number of packets received ... */
   eNwkAttr_ipIfInUcastPkts,                  /*!< The number of packets, delivered by this layer to the upper layers, which were not addressed to a multicast or broadcast address at this layer. */
   eNwkAttr_ipIfInUnknownProtos,              /*!< The number of packets received via the corresponding link layer which were discarded because of an unknown or unsupported protocol */
   eNwkAttr_ipIfLastResetTime,                /*!< The value of the system time at the moment of the last RESET.request being received */
   eNwkAttr_ipIfOutDiscards,                  /*!< The number of packets requested to be sent by the upper layers that were discarded due to no valid route being found. */
   eNwkAttr_ipIfOutErrors,                    /*!< The number of packets attempting transmission via the corresponding link layer which failed due to errors.   */
   eNwkAttr_ipIfOutMulticastPkts,             /*!< The total number of packets that the upper layers requested be transmitted, and which were addressed to a multicast address at this layer. */
   eNwkAttr_ipIfOutOctets,                    /*!< The total number of octets transmitted via the corresponding link layer including network overhead. */
   eNwkAttr_ipIfOutPackets,                   /*!< The number of packets transmitted ... */
   eNwkAttr_ipIfOutUcastPkts,                 /*!< The total number of packets that the upper layers requested be transmitted that were not addressed to a
                                                   multicast or broadcast address at this layer */
   eNwkAttr_ipLowerLinkCount,                 /*!< The number of lower link layers that are interfaced with this network and transport layer. */
   eNwkAttr_ipUpperLayerCount,                /*!< The number of UDP ports that are supported for this network and transport layer. */
#if ( EP == 1 )
   eNwkAttr_ipRoutingMaxPacketCount,          /*!< The number of previous packets received that the EP routing will keep track of. */
   eNwkAttr_ipRoutingEntryExpirationThreshold,/*!< The amount of time in hours that a routing table entry can exist before it is considered expired. */
   eNwkAttr_invalidAddrModeCount,             /*!< number of messages received by the EP which decoded, validated, and contained an address
                                                   mode that was NOT supported */
#endif
   eNwkAttr_ipHEContext,                      /*!< Specifies the IP Context for the Head End */
   eNwkAttr_ipState,                          /*!< The current state of the NWK layer */
#if ( FAKE_TRAFFIC == 1 )
   eNwkAttr_ipRfDutyCycle,
   eNwkAttr_ipBhaulGenCount,
#endif
#if ( HE == 1 )
   // Head End Only
   eNwkAttr_ipPacketHistory,
   eNwkAttr_ipRoutingHistory,
   eNwkAttr_ipRoutingMaxDcuCount,
   eNwkAttr_ipRoutingFrequency,
   eNwkAttr_ipMultipathForwardingEnabled,
#endif

} NWK_ATTRIBUTES_e;


#if ( DCU == 1 )
#define IP_LOWER_LINK_COUNT  2  /*!< The number of lower link layers that are interfaced with this network and transport layer. */
#else
#define IP_LOWER_LINK_COUNT  1  /*!< The number of lower link layers that are interfaced with this network and transport layer. */
#endif

/*!
 * NWK Get/Set Parameters
 */
typedef union
{
   uint32_t ipIfInDiscards;                            /* The number of packets received that were discarded due to no valid route being found.
                                                          The count value will roll over. */
   uint32_t ipIfInErrors[        IP_LOWER_LINK_COUNT]; /* Each list element is the number of packets received via the corresponding link layer which contained errors preventing
                                                          them from being further propagated through the network. */
   uint32_t ipIfInMulticastPkts[ IP_PORT_COUNT];       /* The number of packets, delivered by this layer to the upper layers, which were addressed to a multicast address at this layer. */
   uint32_t ipIfInOctets[        IP_LOWER_LINK_COUNT]; /* The size of the list is ipLowerLinkCount.  Each list element is the total number of octets received via the corresponding link layer including network overhead.  */
   uint32_t ipIfInPackets[       IP_LOWER_LINK_COUNT]; /* */
   uint32_t ipIfInUcastPkts[     IP_PORT_COUNT];       /* The number of packets, delivered by this layer to the upper layers, which were not addressed to a multicast or broadcast address at this layer.  */
   uint32_t ipIfInUnknownProtos[ IP_LOWER_LINK_COUNT]; /* Each list element is the number of packets received via the corresponding link layer which were discarded because of an unknown or unsupported protocol.  */
   TIMESTAMP_t ipIfLastResetTime;                      /* The value of the system time at the moment of the last RESET.request being received when the SetDefault parameter was set to TRUE.
                                                          This value only contains the UTC time in seconds and does not contain the fractional seconds portion. */
   uint32_t ipIfOutDiscards;                           /* The number of packets requested to be sent by the upper layers that were discarded due to no valid route being found.
                                                          The count value will roll over. */
   uint32_t ipIfOutErrors[       IP_LOWER_LINK_COUNT]; /* Each list element is the number of packets attempting transmission via the corresponding link layer which failed due to errors.
                                                          The count value will roll over. */
   uint32_t ipIfOutMulticastPkts[IP_PORT_COUNT];       /* The total number of packets that the upper layers requested be transmitted, and which were addressed to a multicast address at this layer. */
   uint32_t ipIfOutOctets[       IP_LOWER_LINK_COUNT]; /* Each list element is the total number of octets transmitted via the corresponding link layer including network overhead.
                                                          The count value will roll over. */
   uint32_t ipIfOutPackets[      IP_LOWER_LINK_COUNT];
   uint32_t ipIfOutUcastPkts[    IP_PORT_COUNT];       /* The total number of packets that the upper layers requested be transmitted that were not addressed to a multicast or broadcast address at this layer.  */
   uint32_t ipLowerLinkCount;                          /* The number of lower link layers that are interfaced with this network and transport layer. */
                                                       /* 1 for EP, 2 for DCU */
   uint32_t ipUpperLayerCount;                         /* The number of UDP ports that are supported for this network and transport layer. */
                                                       /* 2 for EP, DCU, and HE */
#if ( EP == 1 )
   uint8_t  ipRoutingMaxPacketCount;                   /* The number of previous packets received that the EP routing will keep track of. */
   uint8_t  ipRoutingEntryExpirationThreshold;         /* The amount of time that a routing table entry can stay exist before it is considered expired. */
   uint16_t invalidAddrModeCount;                      /*   */
#endif
   uint8_t  ipHEContext;                               /* Specifies the IP Context for the Head End.*/
   NWK_STATE_e ipState;                                /* The current state of the NWK layer */
#if ( FAKE_TRAFFIC == 1 )
   uint8_t  ipRfDutyCycle;                             /* this test version creates fake traffic:  RF duty cycle (1-10%) */
   uint8_t  ipBhaulGenCount;                           /* this test version creates fake traffic:  holds backhaul generation rate (765 bytes made per X
                                                         seconds), stored value is X, valid range 0-60 */
#endif
#if ( HE == 1 )
   // Head End Only
   uint8_t  ipPacketHistory;                           /* Specifies the amount of packet history in hours stored at the head end. */
   uint8_t  ipRoutingHistory;                          /*  Specifies the amount of packet history in hours included in HE Best DCU routing calculations (Section 3.3.2).
                                                          Only packets from the packet history received by a DCU within ipRoutingHistory hours are used in the RSSI calculation. */
   uint8_t  ipRoutingMaxDcuCount;                      /* Specifies the number of DCUs per EP with the highest computed average RSSI values that are saved in the routing table */
   uint16_t ipRoutingFrequency;                        /* Specifies the frequency that the HE Best DCU routing runs in minutes (Section 3.3.2). */
   bool     ipMultipathForwardingEnabled;              /* Specifies if multipath forwarding is enabled system wide. The number of paths is determined based on the traffic class of the packet.*/
#endif
} NWK_ATTRIBUTES_u;

/*!
 * Requests Supported by NWK
 */
typedef enum
{
   eNWK_GET_REQ,        /*!< Get Request */
   eNWK_RESET_REQ,      /*!< Reset Request */
   eNWK_SET_REQ,        /*!< Set Request */
   eNWK_START_REQ,      /*!< Start Request */
   eNWK_STOP_REQ,       /*!< Stop Request */
   eNWK_DATA_REQ,       /*!< Data Request */
} NWK_REQ_TYPE_t;

/*!
 * Confirmations from NWK
 */
typedef enum
{
   eNWK_GET_CONF,        /*!< Get   Confirm */
   eNWK_RESET_CONF,      /*!< Reset Confirm */
   eNWK_SET_CONF,        /*!< Set   Confirm */
   eNWK_START_CONF,      /*!< Start Confirm */
   eNWK_STOP_CONF,       /*!< Stop  Confirm */
   eNWK_DATA_CONF,       /*!< Data  Confirm */
} NWK_CONF_TYPE_t;

/*!
 * Indications from NWK
 */
typedef enum
{
   eNWK_DATA_IND         /*!< Data  Indication */
} NWK_IND_TYPE_t;


/*!
 * NWK Get - Confirmation Status
 */
typedef enum
{
   eNWK_GET_SUCCESS = 0,        /*!< NWK GET - Success */
   eNWK_GET_UNSUPPORTED,        /*!< NWK GET - Unsupported attribute */
   eNWK_GET_SERVICE_UNAVAILABLE /*!< NWK GET - Service Unavailable */
} NWK_GET_STATUS_e;


/*!
 * NWK Set - Confirmation Status
 */
typedef enum
{
   eNWK_SET_SUCCESS = 0,        /*!< NWK SET - Success */
   eNWK_SET_READONLY,           /*!< NWK SET - Read only */
   eNWK_SET_UNSUPPORTED,        /*!< NWK SET - Unsupported attribute */
   eNWK_SET_INVALID_PARAMETER,  /*!< NWK SET - Invalid paramter */
   eNWK_SET_SERVICE_UNAVAILABLE /*!< NWK SET - Service Unavailable */
} NWK_SET_STATUS_e;


/*!
 * Get.Request
 */
typedef struct
{
   NWK_ATTRIBUTES_e eAttribute;  /*!< Specifies the attribute requested */
} NWK_GetReq_t;


/*! Get.Confirm */
typedef struct
{
   NWK_ATTRIBUTES_e  eAttribute;  /*!< Specifies the attribute requested */
   NWK_GET_STATUS_e  eStatus;     /*!< Execution status SUCCESS, UNSUPPORTED */
   NWK_ATTRIBUTES_u  val;         /*!< Value Returned */
} NWK_GetConf_t;

/*!
 * NWK link override
 */
typedef enum
{
   eNWK_LINK_OVERRIDE_NULL = 0,
   eNWK_LINK_OVERRIDE_BACKHAUL,
   eNWK_LINK_OVERRIDE_TENGWAR_MAC,
} NWK_LINK_OVERRIDE_e;

/*!
 * NWK link settings override
 */
typedef enum
{
   eNWK_LINK_SETTINGS_OVERRIDE_NULL = 0,
   eNWK_LINK_SETTINGS_OVERRIDE_UNICAST_OB,
   eNWK_LINK_SETTINGS_OVERRIDE_UNICAST_IB,
   eNWK_LINK_SETTINGS_OVERRIDE_BROADCAST_OB,
   eNWK_LINK_SETTINGS_OVERRIDE_BROADCAST_IB,
} NWK_LINK_SETTINGS_OVERRIDE_e;

typedef struct
{
   NWK_LINK_OVERRIDE_e          linkOverride;
   NWK_LINK_SETTINGS_OVERRIDE_e linkSettingsOverride;
} NWK_Override_t;

/* compression types */
typedef enum
{
   eFULL_IPV6,
   eEXTENSION_ID,
   eCONTEXT,
   eELIDED,
   eMULTICAST,
}  NWK_AddrType_e;


/* NWK Status values */
typedef enum
{
   eNWK_SUCCESS = 0,
   eNWK_FAILURE,
   eNWK_INVALID_PARAMETER
} NWK_Status_e;



/* NWK Status values */
typedef enum
{
   eNWK_DATA_SUCCESS = 0,
   eNWK_DATA_NO_ROUTE,
   eNWK_DATA_MAC_IDLE,
   eNWK_DATA_TRANSACTION_OVERFLOW,
   eNWK_DATA_TRANSACTION_FAILED,
   eNWK_DATA_INVALID_PARAMETER,
   eNWK_DATA_INVALID_HANDLE
} NWK_DATA_STATUS_e;

typedef struct
{
   NWK_AddrType_e addr_type;
   union
   {
      uint8_t IPv6_address[IPV6_ADDRESS_SIZE];
      uint8_t ExtensionAddr[MAC_ADDRESS_SIZE];
      uint8_t context;
      uint8_t multicastAddr[MULTICAST_ADDRESS_SIZE];
   };
} NWK_Address_t;

typedef struct {
   uint16_t nextHopHeaderOffset;
   uint8_t subtype;
   uint8_t nextHeaderPresent;
   uint8_t reserved;
   uint8_t persist;
   uint8_t hopCount;
   uint8_t addressType;
   NWK_Address_t nextHop[4];
   uint8_t nextHeader;
} NWK_Next_Hop_Header_t;

typedef struct {
   uint8_t version;
   uint8_t qualityOfService;
   uint8_t nextHeaderPresent;
   NWK_Address_t srcAddress;
   NWK_Address_t dstAddress;
   uint8_t       nextHeader;
   NWK_Next_Hop_Header_t nextHopHeader;

   /* udp (including here for this phase of project) */
   uint8_t src_port;   /* 4 bit. source port */
   uint8_t dst_port;   /* 4 bit. destination port */
   uint16_t length;
   uint8_t *data; /*!< payload */
} IP_Frame_t;

/* Data.Request */
typedef struct
{
   uint8_t        handle; /* for use in tying a data.request to data.confirm */
   uint8_t        qos;    /* Quality of service */
   NWK_Address_t  dstAddress;
   NWK_Override_t override;
   uint16_t       payloadLength;
   MAC_dataConfCallback callback; /* optional function pointer to be called with tx result (QoS dictates 'success') */
   uint8_t        port;    /* UDP port number   */
   uint8_t        data[];
} NWK_DataReq_t;

/* Data.Confirm */
typedef struct
{
   uint8_t handle;            /* for use in tying a data.request to data.confirm */
   NWK_DATA_STATUS_e status;  /*lint !e123 same name as macro OK; Status of the transmission */
   TIMESTAMP_t timeStamp;     /* TimeStamp associated with when the payload was transmitted.
                                 Only valid when status = success */
} NWK_DataConf_t;

/* Data.Indication */
typedef struct
{
   uint8_t qos;              /* Quality of service, aka traffic class */
   bool timePresent;         /* Indicates if TimeStamp is valid */
   TIMESTAMP_t timeStamp;    /* TimeStamp associated with when the payload was received */
   NWK_Address_t dstAddr;    /* Destination address mode and value */
   uint16_t dstUdpPort;      /* The UDP port for the recipient of the packet. */
   NWK_Address_t srcAddr;    /* Source address mode and value */
   uint16_t srcUdpPort;      /* The UDP port for the originator of the packet. */
   uint16_t payloadLength;   /* Number of data bytes. */
   uint8_t *payload;         /* The data */
} NWK_DataInd_t;

/*!
 * Set.Request
 */
typedef struct
{
   NWK_ATTRIBUTES_e  eAttribute; /*!< Specifies the attribute to set */
   NWK_ATTRIBUTES_u  val;        /*!< Specifies the value to set     */
} NWK_SetReq_t;

/*!
 * Set.Confirm
 */
typedef struct
{
   NWK_ATTRIBUTES_e eAttribute;    /*!< Specifies the attribute */
   NWK_SET_STATUS_e eStatus;       /*!< */
} NWK_SetConf_t;

/*!
 * Reset.Request - parameters
 */
typedef enum
{
   eNWK_RESET_ALL,       /*!< Reset all statistics */
   eNWK_RESET_STATISTICS /*!< Reset some staistics */
} NWK_RESET_e;


/*!
 * Start.Request - parameters
 *
 * No parameters!
 */

/*!
 * NWK Reset - Confirmation Status
 */
typedef enum
{
   eNWK_RESET_SUCCESS = 0,       /*!< NWK RESET - Success */
   eNWK_RESET_FAILURE            /*!< NWK RESET - Failure */
} NWK_RESET_STATUS_e;

/*!
 * NWK Start - Confirmation Status
 */
typedef enum
{
   eNWK_START_SUCCESS,              /*!< NWK START - Success */
   eNWK_START_NETWORK_RUNNING,      /*!< NWK START - Running */
   eNWK_START_PARTIAL_LINK_RUNNING, /*!< NWK START - Partial Link Running */
   eNWK_START_NO_MAC_START          /*!< NWK START - MAC did not start */
} NWK_START_STATUS_e;

/*!
 * NWK Stop - Confirmation Status
 */
typedef enum
{
   eNWK_STOP_SUCCESS,           /*!< NWK STOP - Success */
   eNWK_STOP_ERROR,             /*!< NWK STOP - Error   */
} NWK_STOP_STATUS_e;

typedef struct
{
   NWK_RESET_e eType;           /*!< ALL, STATISTICS */
} NWK_ResetReq_t;

/*!
 * Start.Request
 */
typedef struct
{
   uint32_t dummy;
} NWK_StartReq_t;

/*!
 * Stop.Request
 */
typedef struct
{
   uint8_t dummy;
} NWK_StopReq_t;

/*!
 * Reset.Confirm
 */
typedef struct
{
   NWK_RESET_STATUS_e eStatus;     /*!< SUCCESS, FAILURE */
} NWK_ResetConf_t;


/*!
 * Start.Confirm
 */
typedef struct
{
   NWK_START_STATUS_e eStatus;     /*!< */
} NWK_StartConf_t;

/*!
 * Stop.Confirm
 */
typedef struct
{
   NWK_STOP_STATUS_e eStatus;      /*!< SUCCESS, FAILURE */
} NWK_StopConf_t;

typedef struct
{
   NWK_IND_TYPE_t      Type;
   union
   {
      NWK_DataInd_t     DataInd;   /*!< Data Indication from NWK */
   };
} NWK_Indication_t;


typedef struct
{
   NWK_CONF_TYPE_t      Type;
   uint32_t             handleId;   /*!< Handle for the request */
   union
   {
      // Management Services
      NWK_GetConf_t     GetConf;   /*!< Get   Confirm from NWK */
      NWK_ResetConf_t   ResetConf; /*!< Reset Confirm from NWK */
      NWK_SetConf_t     SetConf;   /*!< Set   Confirm from NWK */
      NWK_StartConf_t   StartConf; /*!< Start Confirm from NWK */
      NWK_StopConf_t    StopConf;  /*!< Stop  Confirm from NWK */
      NWK_DataConf_t    DataConf;  /*!< Data  Confirm from NWK */
   };
} NWK_Confirm_t;

typedef void (*NWK_ConfirmHandler)(NWK_Confirm_t const *pConfirm);

typedef union
{
   NWK_GetReq_t     GetReq;     /*!< Get   Request to NWK */
   NWK_ResetReq_t   ResetReq;   /*!< Reset Request to NWK */
   NWK_SetReq_t     SetReq;     /*!< Set   Request to NWK */
   NWK_StartReq_t   StartReq;   /*!< Start Request to NWK */
   NWK_StopReq_t    StopReq;    /*!< Stop  Request to NWK */
   NWK_DataReq_t    DataReq;    /*!< Data  Request to NWK */
} NWK_Services_u;

typedef struct
{
   NWK_REQ_TYPE_t     MsgType;
   uint32_t           handleId;   /*!< Handle for the request */
   NWK_ConfirmHandler pConfirmHandler;
   NWK_Services_u     Service;
} NWK_Request_t;

// for backward compatibility
#define STACK_Status_e NWK_Status_e

/* Prototypes */
#endif /* STACK_PROTOCOL_H */

