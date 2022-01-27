/******************************************************************************
 *
 * Filename: MAC_PacketManagement.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2013 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef MAC_PacketManagement_H
#define MAC_PacketManagement_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

typedef struct
{
   OS_QUEUE_Element QueueElement; /*lint -esym(754,QueueElement) */
   buffer_t *memToFree;
   uint16_t handle;               /* the handle to pass in confirmation indication once things tx succeeds or fails */
   uint16_t chan;                 /* Channel to use for all frames in this packet */
   bool txInProgress;             /* flag indicating if this message is currently being transmitted (in-process) */
   uint8_t NextFragmentId;
   uint8_t NumAcked;
   uint8_t frame_type;
   bool    skip_cca;              /* false - normal (cca) true - skip cca */
} MacPacket_s;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
returnStatus_t MAC_PacketManag_init ( void );
MacPacket_s *MAC_PacketManag_GetNextTxMsg ( void );
#ifdef PHY_Protocol_H
void MAC_PacketManag_Acknowledge ( uint8_t seqNum, MAC_DATA_STATUS_e status );
#endif
bool MAC_PacketManag_AckPending ( uint8_t *reqNum );
uint8_t MAC_PacketManag_GetRetryCount ( MAC_Reliability_e reliability );
bool MAC_Transaction_Create ( buffer_t *macBuffer, uint8_t frameType, bool skip_cca );
bool MAC_PacketManag_IsTxMessagePending(MacPacket_s **PacketData);
MAC_DataReq_t *MAC_GetDataReqFromBuffer(buffer_t const *macRequestBuff);
uint8_t MAC_CalcNumSegments(MAC_DataReq_t const *dataReq, uint16_t maxTxPayload);

bool MAC_PacketManag_Purge (uint16_t handle );
bool MAC_PacketManag_Flush ( void );
/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
