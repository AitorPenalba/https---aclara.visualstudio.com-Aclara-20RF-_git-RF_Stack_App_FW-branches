/******************************************************************************
 *
 * Filename: NWK.h
 *
 * Global Designator: NWK
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2014 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef NWK_H
#define NWK_H

/* INCLUDE FILES */
#include "PHY_Protocol.h"
#include "STACK_Protocol.h"

/* #DEFINE DEFINITIONS */
#define MAX_NETWORK_PAYLOAD ((uint16_t)1271)

#define IGNORE_PARAMETER ((uint8_t)0)

#define RF_LINK        ((uint8_t)0)
#if ( DCU == 1 )
#define BACKPLANE_LINK ((uint8_t)1)
#endif

#define BROADCAST_MULTICAST_EP_ADDR    (uint8_t []) {0x35, 0x00, 0x80, 0x00, 0x00, 0x00}
#define BROADCAST_MULTICAST_DCU_ADDR   (uint8_t []) {0x35, 0x00, 0x80, 0x00, 0x00, 0x01}
#define BROADCAST_MULTICAST_LC_ADDR    (uint8_t []) {0x35, 0x00, 0x80, 0x00, 0x00, 0x02}

// List of supported next header type
#define NEXT_HEADER_NEXT_HOP_TYPE 253

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef struct
{
   TIMESTAMP_t LastResetTime;                           /*!< The value of the system time at the moment of the last
                                                          RESET.request being received when the SetDefault parameter was
                                                          set to TRUE.  This value only contains the UTC time in seconds
                                                          and does not contain the fractional seconds portion. */
   uint32_t ipIfInDiscards;                             /*!< The number of packets received that were discarded due to
                                                          no valid route being found.  */
   uint32_t ipIfInErrors[        IP_LOWER_LINK_COUNT ]; /*!< The number of packets received via the corresponding link
                                                          layer which contained errors preventing them from being
                                                          further propagated through the network */
   uint32_t ipIfInMulticastPkts[ IP_PORT_COUNT];        /*!< The number of packets, delivered by this layer to the upper
                                                          layers, which were addressed to a multicast address at this
                                                          layer.  */
   uint32_t ipIfInOctets[        IP_LOWER_LINK_COUNT ]; /*!< Each list element is the total number of octets received
                                                          via the corresponding link layer including network overhead.
                                                         */
   uint32_t ipIfInUcastPkts[     IP_PORT_COUNT];        /*!< The number of packets, delivered by this layer to the upper
                                                          layers, which were not addressed to a multicast or broadcast
                                                          address at this layer.  */
   uint32_t ipIfInUnknownProtos[ IP_LOWER_LINK_COUNT ];  /*!< Each list element is the number of packets received via
                                                           the corresponding link layer which were discarded because of
                                                           an unknown or unsupported protocol.  */
   uint32_t ipIfInPackets[       IP_LOWER_LINK_COUNT ]; /*!< */
   uint32_t ipIfOutDiscards;                            /*!< The number of packets requested to be sent by the upper
                                                          layers that were discarded due to no valid route being found.
                                                         */
   uint32_t ipIfOutErrors[       IP_LOWER_LINK_COUNT ]; /*!<  Each list element is the number of packets attempting
                                                          transmission via the corresponding link layer which failed due
                                                          to errors. */
   uint32_t ipIfOutMulticastPkts[IP_PORT_COUNT];        /*!< The total number of packets that the upper layers requested
                                                          be transmitted, and which were addressed to a multicast
                                                          address at this layer.  */
   uint32_t ipIfOutOctets[       IP_LOWER_LINK_COUNT ]; /*!< Each list element is the total number of octets transmitted
                                                          via the corresponding link layer including network overhead.
                                                         */
   uint32_t ipIfOutPackets[      IP_LOWER_LINK_COUNT ]; /*!< */
   uint32_t ipIfOutUcastPkts[    IP_PORT_COUNT];        /*!< The total number of packets that the upper layers requested
                                                          be transmitted that were not addressed to a multicast or
                                                          broadcast address at this layer.  */
#if ( EP == 1 )
   uint16_t invalidAddrModeCount;                       /*!< number of messages received by the EP which decoded,
                                                          validated, and contained an address mode that was NOT
                                                          supported */
#endif
}NWK_CachedAttr_t;

typedef void (*NWK_IndicationHandler)(buffer_t *indication);

typedef enum NWK_COUNTER_e
{
   eNWK_ipIfInDiscards,
   eNWK_ipIfInErrors,
   eNWK_ipIfInMulticastPkts,
   eNWK_ipIfInOctets,
   eNWK_ipIfInUcastPkts,
   eNWK_ipIfInUnknownProtos,
   eNWK_ipIfInPackets,
   eNWK_ipIfOutDiscards,
   eNWK_ipIfOutErrors,
   eNWK_ipIfOutMulticastPkts,
   eNWK_ipIfOutOctets,
   eNWK_ipIfOutPackets,
   eNWK_ipIfOutUcastPkts,
   eNWK_invalidAddrModeCount,
}NWK_COUNTER_e;


/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
#ifdef ERROR_CODES_H_
returnStatus_t NWK_init ( void );
#endif
void NWK_Task ( taskParameter );

void NWK_Request(buffer_t *request);

void NWK_RegisterIndicationHandler( uint8_t port, NWK_IndicationHandler pCallback);

bool NWK_FindNextHop(NWK_Address_t const *dstAddr, NWK_Address_t *nextHopAddr);
#ifdef MAC_PROTOCOL_H
void NWK_ElideAndSend(uint8_t *ipPacket, uint16_t length, IP_Frame_t const *ip_frame, NWK_Address_t dstAddr, MAC_CHANNEL_SET_INDEX_e channelSetIndex);
#endif
bool NWK_decode_frame ( uint8_t *p, uint16_t num_bytes, IP_Frame_t *pf, NWK_COUNTER_e *fail_reason );
void NWK_ProcessNextHeader(uint8_t * ipPacket, uint16_t length, IP_Frame_t *ip_frame);

uint8_t NWK_QosToParams(uint8_t trafficClass, bool *ackRequired, uint8_t *MsgPriority,
                        bool *droppable, MAC_Reliability_e *reliability);

void STACK_GetIPAddress( uint8_t *address );
void NWK_SendDataIndication(buffer_t *pBuf, IP_Frame_t const *rx_frame, TIMESTAMP_t timeStamp);
void NWK_Stats(void);

STACK_Status_e STK_MGR_ApplyFrequencies( void );
bool NWK_IsQosValueValid( uint8_t QoS );

void NWK_CounterInc(NWK_COUNTER_e eStackCounter, uint32_t IncCount, uint8_t index);

uint32_t NWK_StartRequest(NWK_ConfirmHandler confirm_cb);
uint32_t NWK_StopRequest(NWK_ConfirmHandler confirm_cb);
NWK_GetConf_t NWK_GetRequest(NWK_ATTRIBUTES_e eAttribute);
NWK_SetConf_t NWK_SetRequest(NWK_ATTRIBUTES_e eAttribute, void const *val);
uint32_t NWK_ResetRequest(NWK_RESET_e eType, NWK_ConfirmHandler confirm_cb);

uint32_t NWK_DataRequest(
   uint8_t port,  /* UDP port number   */
   uint8_t qos,
   uint16_t size,
   uint8_t  const data[],
   NWK_Address_t const *dst_addr,
   NWK_Override_t const *override,
   MAC_dataConfCallback callback,          // Data Confirm Callback
   NWK_ConfirmHandler confirm_cb);

uint16_t NWK_UdpPortIdToNum(uint8_t id);
uint8_t NWK_UdpPortNumToId(uint16_t num);

#endif /* NWK_H */
