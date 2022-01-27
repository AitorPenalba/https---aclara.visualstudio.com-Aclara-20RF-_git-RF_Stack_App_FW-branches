/******************************************************************************
 *
 * Filename: MAC_FrameManagement.h
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
#ifndef MAC_FrameManagement_H
#define MAC_FrameManagement_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */
#define MAX_MAC_FRAME_BUFFERS 12

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef struct
{
   OS_QUEUE_Element QueueElement;
   mac_frame_t MsgData;
   bool SentToNextLayer; /* Tx functions send to PHY layer, Rx functions send to Packet/Upper layer */
   TIMESTAMP_t TimeTxRx; /* Used for both Tx and Rx - time sent if Tx - time received if Rx */
   uint16_t rssi;        /* RSSI 0-4095 */
   uint16_t channel;

} MAC_FrameManagBuf_s;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
returnStatus_t MAC_FrameManag_init ( void );
bool MAC_FrameManag_AllocTxData ( MAC_FrameManagBuf_s **FrameData );
void MAC_FrameManag_Add_Tx ( MAC_FrameManagBuf_s *FrameData );
MAC_FrameManagBuf_s *MAC_FrameManag_GetNextTxMsg ( void );
void MAC_FrameManag_CheckTimeouts ( void );
uint8_t MAC_FrameManag_Get_NextReqNum ( void );
uint16_t MAC_FrameManag_GetTxQueueSize ( void );
void MAC_FrameManag_PhyDataConfirm ( MAC_DATA_STATUS_e status );
bool MAC_FrameManag_DataIndHandler ( const mac_frame_t *macFrame, TIMESTAMP_t TimeStamp, uint32_t TimeStampCYCCNT, float rssi_dbm, float danl_dbm, uint16_t chan , uint32_t sync_time );
bool MAC_FrameManag_IsPacketInReassembly(void);
bool MAC_FrameManag_IsUnicastPacketInReassembly(void);
bool MAC_FrameManag_IsUnitTransmitting(uint8_t const mac_addr[MAC_ADDRESS_SIZE]);


/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
