/******************************************************************************
 *
 * Filename: MAC_PacketManagement.c
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

/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>
#include "OS_aclara.h"
#include "BSP_aclara.h"
#include "BUF_MemoryBuffers.h"
#include "DBG_SerialDebug.h"
#include "MAC_Protocol.h"
#include "MAC_FrameManagement.h"
#include "MAC_PacketManagement.h"
#if ( DCU == 1 )
#include "STAR.h"    // DCU2+
#include "version.h"
#endif
#include "MAC.h"
#include "PHY.h"

/* #DEFINE DEFINITIONS */
#if ( DCU == 1 )
#define MAX_TX_PACKET_BUFFERS 2000 /* number of MAC packets that can be buffered prior to transmission */
#else
#define MAX_TX_PACKET_BUFFERS 20 /* number of MAC packets that can be buffered prior to transmission */
#endif

#if ( DCU == 1 ) // DCU2+
#define SRFN_TX_DISABLE_TIMEOUT 15 /* Don't disable SRFN for more than 15 seconds */
#endif

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* this structure contains status of the currently being transmitted unsegmented MAC Packet */
typedef struct
{
   uint16_t bytesTransmitted; /* number of bytes that have been put into frames for this packet */
   uint8_t nextSequenceToTx;  /* the ARQ sequence number to use when generating an ACK required frame */
   uint8_t nextSegmentToTx;   /* the next segment to use when generating an segmented frame */
   uint8_t retryCount;
   uint8_t segmentTxd[MAX_MAC_FRAME_BUFFERS];
   uint8_t packet_id;
   uint8_t segment_count;     /* Total number of segments in this packet */
   uint8_t NumAcked;
   uint8_t packetTxCount;     /* number of times the packet has been transmitted */
} MacPacketTracking_s;

static MacPacketTracking_s CurrentPacketTracking;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* When messages to transmit are coming in faster than the Radio can send them
   out (typically head end generated), this queue is where messages buffer up. */
static OS_List_Obj MAC_PacketTxQueue;
static BUF_Obj MAC_PacketTxBufObj;

#if ( DCU == 1 )
static MacPacket_s PacketTxData[MAX_TX_PACKET_BUFFERS] @ "EXTERNAL_RAM" ; /*lint !e430*/
#else
static MacPacket_s PacketTxData[MAX_TX_PACKET_BUFFERS];
#endif

/* FUNCTION PROTOTYPES */
static bool PrioritizeAndInsert( MacPacket_s *packet );

/* FUNCTION DEFINITIONS */

/******************************************************************************

  Function Name: MAC_PacketManag_init

  Purpose: Initialize the Packet Management Module before any tasks start in the system

  Arguments:

  Returns: RetVal - indicates success or the reason for error

  Notes:

******************************************************************************/
returnStatus_t MAC_PacketManag_init ( void )
{
/* Local Variables */
   returnStatus_t RetVal = eSUCCESS;
/* End Local Variables */

   if ( !OS_LINKEDLIST_Create(&MAC_PacketTxQueue) )
   {
      RetVal = eFAILURE;
   } /* end if() */

   if ( !BUF_Create(&MAC_PacketTxBufObj, &(PacketTxData[0]), MAX_TX_PACKET_BUFFERS, sizeof(MacPacket_s)) )
   {
      RetVal = eFAILURE;
   } /* end if() */

   /* initialize in-progress packet tracking */
   CurrentPacketTracking.bytesTransmitted = 0;
   CurrentPacketTracking.nextSequenceToTx = 0;
   CurrentPacketTracking.nextSegmentToTx = 0;
   CurrentPacketTracking.retryCount = 0;
   CurrentPacketTracking.NumAcked = 0;
   (void)memset(CurrentPacketTracking.segmentTxd, 0, MAX_MAC_FRAME_BUFFERS);
   CurrentPacketTracking.packetTxCount = 0;

   return ( RetVal );
} /* end MAC_PacketManag_init () */

/***********************************************************************************************************************

Function Name: MAC_PacketManag_IsTxMessagePending()

Purpose: This function is used to check if there is a packet in the Tx Queue

Arguments: none

Returns: true/false

***********************************************************************************************************************/
bool MAC_PacketManag_IsTxMessagePending(MacPacket_s **PacketData)
{
   MacPacket_s *pPacketData;

   pPacketData = OS_LINKEDLIST_Head ( &MAC_PacketTxQueue );

   if ( NULL != pPacketData )
   {
      *PacketData = pPacketData;
      return (bool)true;
   }
   else{
      return (bool)false;
   }
}

/***********************************************************************************************************************

Function Name: MAC_PacketManag_Purge

Purpose: This function is used purge a specific transaction from the tx queue

Arguments: handle

Returns: true/false

***********************************************************************************************************************/
bool MAC_PacketManag_Purge (uint16_t handle )
{
   MacPacket_s *PacketData;
   int i;
   uint16_t numElements;
   MAC_DataReq_t *macRequest;
   numElements = OS_LINKEDLIST_NumElements ( &MAC_PacketTxQueue );

   INFO_printf("numElements = %d:",  numElements);

   if ( numElements > 0 )
   {
      PacketData = OS_LINKEDLIST_Head ( &MAC_PacketTxQueue );
      for ( i=0; i<numElements; i++ )
      {
         macRequest = MAC_GetDataReqFromBuffer( (buffer_t*)(PacketData->memToFree) );

         INFO_printf("handle = %d", macRequest->handle);
         if(macRequest->handle == handle)
         {
            OS_LINKEDLIST_Remove ( &MAC_PacketTxQueue, PacketData );
            if ( BUF_Put(&MAC_PacketTxBufObj, PacketData) != BUF_ERR_NONE )
            {
               DBG_logPrintf('E',"BUF_PUT failed" );
            }
            return (bool)true;
         }
         PacketData = OS_LINKEDLIST_Next ( &MAC_PacketTxQueue, PacketData );
      }
   }
   return (bool)false;
}

/***********************************************************************************************************************

Function Name: bool MAC_PacketManag_Flush ( void )

Purpose: This function is used flush all transactions from the tx queue

Arguments: none

Returns: true/false

***********************************************************************************************************************/
bool MAC_PacketManag_Flush ( void )
{
   MacPacket_s *PacketData;
   int i;
   uint16_t numElements;
   numElements = OS_LINKEDLIST_NumElements ( &MAC_PacketTxQueue );
   INFO_printf("numElements = %d:",  numElements);

   if ( numElements > 0 )
   {
      PacketData = OS_LINKEDLIST_Head ( &MAC_PacketTxQueue );
      for ( i=0; i<numElements; i++ )
      {
         INFO_printf("handle = %d", PacketData->handle);
         OS_LINKEDLIST_Remove ( &MAC_PacketTxQueue, PacketData );
         if ( BUF_Put(&MAC_PacketTxBufObj, PacketData) != BUF_ERR_NONE )
         {
            DBG_logPrintf('E',"BUF_PUT failed" );
         }
         PacketData = OS_LINKEDLIST_Next ( &MAC_PacketTxQueue, PacketData );
      }
   }
   return (bool)true;
}

/***********************************************************************************************************************
Function Name: MAC_PacketManag_GetNextTxMsg

Purpose: This function will look through the Packet messages and see if there is a message ready to send down to the
   Frame Buffer level - and attempt to send it if there is one

Arguments: none

Returns: none
***********************************************************************************************************************/
MacPacket_s *MAC_PacketManag_GetNextTxMsg ( void )
{
   uint8_t macHeaderLen = MIN_MAC_HEADER_LEN;
   uint16_t numElements;
   uint8_t reqNumber;
   MacPacket_s *PacketData;
   MAC_FrameManagBuf_s *MacMsg;
   MAC_DataReq_t *macRequest;
   static uint16_t maxTxPayload = PHY_DEFAULT_TX_PAYLOAD; // Use a static variable because we want to avoid changing the segment size in the middle of segmenting a frame.

   PacketData = OS_LINKEDLIST_Head ( &MAC_PacketTxQueue );
   if ( NULL != PacketData )
   {
      macRequest = MAC_GetDataReqFromBuffer(PacketData->memToFree);

      if ( 0 == CurrentPacketTracking.bytesTransmitted )
      {
         /* this new packet is now being transmitted */
         PacketData->txInProgress = (bool)true;
         if ( CurrentPacketTracking.retryCount == 0 )
         {

            MAC_CounterInc(eMAC_TxPacketCount);

            /* only increment packet ID when transmitting new Packet, QoS retries keep same Packet Id */
            CurrentPacketTracking.packet_id = MAC_NextPacketId();

            maxTxPayload = PHY_GetMaxTxPayload(); // Use the latest TX payload length and keep the same value during segmentation
            CurrentPacketTracking.segment_count = MAC_CalcNumSegments(macRequest, maxTxPayload);

            /* set all unused count slots to successfully tx'd (segment_count is one less that count) */
            (void)memset(&CurrentPacketTracking.segmentTxd[(CurrentPacketTracking.segment_count+1)],
                         1,
                         ( MAX_MAC_FRAME_BUFFERS - ( CurrentPacketTracking.segment_count + 1 ) ));

            PacketData->chan = MAC_RandomChannel_Get(macRequest->channelSets, macRequest->channelSetIndex);
         }
      }
      /* check if there are still frames to be generated for this packet */
      if ( CurrentPacketTracking.bytesTransmitted < macRequest->payloadLength )
      {
         numElements = MAC_FrameManag_GetTxQueueSize();

         /* make a new frame if (no ARQ and tx frame queue is empty) or if (ARQ is on and ARQ window has space) */
         if ( ( ( false == macRequest->ackRequired ) && ( 0 == numElements ) ) ||
              ( ( true == macRequest->ackRequired ) && ( numElements < MAC_ACK_WINDOW ) ) )
         {
            if ( !MAC_FrameManag_AllocTxData(&MacMsg) )
            {
               DBG_logPrintf('E',"MAC_FrameManag_AllocTxData() failed" );
            }
            else
            {
               MacMsg->MsgData.frame_type = PacketData->frame_type;
               MacMsg->MsgData.dst_addr_mode = macRequest->dst_addr_mode;
               if ( UNICAST_MODE == MacMsg->MsgData.dst_addr_mode )
               {
                  macHeaderLen += MAC_ADDRESS_SIZE;
               }
               MacMsg->MsgData.ack_required = macRequest->ackRequired;
               if (true == MAC_PacketManag_AckPending(&reqNumber))
               {
                  /* add any pending ACKs to piggy back on this Frame */
                  MacMsg->MsgData.ack_present = true;
                  MacMsg->MsgData.req_num = reqNumber;
               }
               else
               {
                  MacMsg->MsgData.req_num = 0;
               }
               MacMsg->MsgData.packet_id = CurrentPacketTracking.packet_id;
               MacMsg->MsgData.network_id = NetworkId_Get();
               MAC_MacAddress_Get(MacMsg->MsgData.src_addr);
               (void)memcpy(MacMsg->MsgData.dst_addr, macRequest->dst_addr, sizeof(MacMsg->MsgData.dst_addr));

               if ( true == MacMsg->MsgData.ack_required )
               {
                  MacMsg->MsgData.seq_num = CurrentPacketTracking.nextSequenceToTx;
               }
               else
               {
                  MacMsg->MsgData.seq_num = 0;
               }

               if ( ( true == MacMsg->MsgData.ack_required ) || ( true == MacMsg->MsgData.ack_present ) )
               {
                  macHeaderLen++;
               }

               MacMsg->MsgData.segment_count = CurrentPacketTracking.segment_count;
               if ( 0 == CurrentPacketTracking.nextSegmentToTx )
               {
                  /* this is the first frame generated for this packet, determine if the packet needs to be segmented */

                  /* frame will include 2 bytes of length */
                  macHeaderLen += 2;
                  if ( ( maxTxPayload - macHeaderLen ) < macRequest->payloadLength )
                  {
                     MacMsg->MsgData.segmentation_enabled = true;
                     MacMsg->MsgData.segment_id = CurrentPacketTracking.nextSegmentToTx;
                     /* frame will include 1 byte of segment ID */
                     macHeaderLen++;
                  }
                  else
                  {
                     MacMsg->MsgData.segmentation_enabled = false;
                  }
               }
               else
               {
                  MacMsg->MsgData.segmentation_enabled = true;
                  MacMsg->MsgData.segment_id = CurrentPacketTracking.nextSegmentToTx;
                  /* frame will include 1 byte of segment ID */
                  macHeaderLen++;
               }

               MacMsg->MsgData.length = macRequest->payloadLength;
               MacMsg->MsgData.thisFrameLen = macRequest->payloadLength - CurrentPacketTracking.bytesTransmitted;
               if ( ( maxTxPayload - macHeaderLen ) < MacMsg->MsgData.thisFrameLen )
               {
                  /* there will be at least one more segment after this one, send as much as fits */
                  MacMsg->MsgData.thisFrameLen = ( maxTxPayload - macHeaderLen );
               }
               (void)memcpy( MacMsg->MsgData.data,
                      &macRequest->payload[CurrentPacketTracking.bytesTransmitted],
                      MacMsg->MsgData.thisFrameLen);

               MAC_FrameManag_Add_Tx(MacMsg);
               CurrentPacketTracking.bytesTransmitted += MacMsg->MsgData.thisFrameLen;
            }
         }
      } /* end if() */
   } /* end if() */
   return PacketData;
} /* end MAC_PacketManag_GetNextTxMsg () */


/***********************************************************************************************************************
Function Name: MAC_PacketManag_Acknowledge

Purpose: This function is called after a PHY DATA.Confirm.  This module keeps track of each frame in a packet
   to know when the whole packet can be acknowledged to the upper layer

Arguments: seqNum - sequence number of frame ACK'd if ARQ is enabled.  If argument == 0xFF, this was a no ARQ ack
      indicating the phy layer successfully transmitted.

Returns: none
***********************************************************************************************************************/
void MAC_PacketManag_Acknowledge ( uint8_t seqNum, MAC_DATA_STATUS_e status )
{
   MacPacket_s *PacketData;
   MAC_DATA_STATUS_e finalStatus = eMAC_DATA_SUCCESS;
   uint8_t i;
   MAC_DataReq_t *macRequest;

   PacketData = OS_LINKEDLIST_Head ( &MAC_PacketTxQueue );
   if ( NULL != PacketData )
   {
      macRequest = MAC_GetDataReqFromBuffer( (buffer_t*)(PacketData->memToFree) );

      if ( status == eMAC_DATA_SUCCESS )
      {
         CurrentPacketTracking.segmentTxd[CurrentPacketTracking.nextSegmentToTx] = 1;
      }
      else
      {
         DBG_logPrintf('E',"%s", MAC_DataStatusMsg(status) );
      }

      if ( 0xFF == seqNum )
      {
         // Check if we should abort transmission.
         // This happens when we are in the process of transmitting the packet for the last time and that we know that a frame failed to be transmitted in all attempts.
         // This implies that the receiver will never be able to rebuild the packet and we should just abort transmission.
         if ( MAC_ReliabilityRetryCount_Get( macRequest->reliability) <= CurrentPacketTracking.retryCount )
         {
            // Check if this frame been transmitted at least once.
            if ( CurrentPacketTracking.segmentTxd[CurrentPacketTracking.nextSegmentToTx] == 0 )
            {
               // We are missing a frame. Need to abort transmission.
               // This next 2 statements will force the rest of the code to believe that all frames were transmited and all retries were done and thus cause the transmission to stop
               CurrentPacketTracking.bytesTransmitted = macRequest->payloadLength;
               CurrentPacketTracking.retryCount       = 255; // This value is large enough to stop retries
            }
         }

         if ( CurrentPacketTracking.bytesTransmitted >= macRequest->payloadLength )
         {
            if (CurrentPacketTracking.bytesTransmitted > macRequest->payloadLength)
            {
               DBG_logPrintf('E',"CurrentPacketTracking.bytesTransmitted > macRequest->payloadLength" );
            }
            /* This packet has been fully transmitted */
            CurrentPacketTracking.bytesTransmitted = 0;
            CurrentPacketTracking.nextSegmentToTx = 0;
#if ( DCU == 1 )
            // Delay after each packet is complete
            MAC_TxPacketDelay();
#endif
            if ( MAC_ReliabilityRetryCount_Get( macRequest->reliability) > CurrentPacketTracking.retryCount )
            {
               CurrentPacketTracking.retryCount++;
            }
            else
            {
#if ( ( EP == 1 ) && ( MULTIPLE_MAC != 0 ) )
               CurrentPacketTracking.retryCount = 0;  /* Reset the retry count for the next mac address. */
               MAC_getNextMacAddr();                  /* Bump MAC address to next "ghost" address  */
               if ( 0 == MAC_getMacOffset() )
#endif
               {
                  /* transmission of this packet is complete, notify the upper layer */

                  /* if each segment made it out at least once successfully, send success to callback */
                  for (i = 0; i < (CurrentPacketTracking.segment_count+1); i++)
                  {
                     if ( CurrentPacketTracking.segmentTxd[i] == 0 )
                     {
                        finalStatus = eMAC_DATA_TRANSACTION_FAILED;
                        MAC_CounterInc(eMAC_TxPacketFailedCount);
                        break;
                     }
                  }
                  /* transmission of this packet is complete, notify the upper layer */
                  if ( macRequest->callback != NULL )
                  {
                     (*macRequest->callback)(finalStatus, PacketData->handle);
                  }

                  // call the confirm handler
                  {
                     MAC_Confirm_t Conf;
                     Conf.Type = eMAC_DATA_CONF;
                     Conf.handleId = PacketData->handle;

                     // Create the confirmation and call the Confirm Handler
                     Conf.DataConf.status        = finalStatus;
                     Conf.DataConf.handle        = PacketData->handle;
                     Conf.DataConf.payloadLength = macRequest->payloadLength;
                     MAC_SendConfirm(&Conf, PacketData->memToFree);
                  }

                  /* free the memory passed down with the upper layer request */
                  if (NULL != PacketData->memToFree )
                  {
                     BM_free(PacketData->memToFree);
                  }

                  /* free the packet from tx buffer */
                  OS_LINKEDLIST_Remove ( &MAC_PacketTxQueue, PacketData );
                  if ( BUF_Put(&MAC_PacketTxBufObj, PacketData) != BUF_ERR_NONE )
                  {
                     DBG_logPrintf('E',"BUF_PUT failed" );
                  }

                  CurrentPacketTracking.retryCount = 0;
                  (void)memset(CurrentPacketTracking.segmentTxd, 0, MAX_MAC_FRAME_BUFFERS);
               }
            }
         }
         else
         {
            CurrentPacketTracking.nextSegmentToTx++;
         }
      }
      else
      {
         DBG_logPrintf('E',"Unexpected;  ARQ may have been enabled on a packet" );
      }
   }
} /* end MAC_PacketManag_Acknowledge () */

/***********************************************************************************************************************

  Function Name: MAC_Transaction_Create

  Purpose: This function creates a MacPacket_s and adds it to the transmit transaction queue.

  Arguments: macBuffer - Pointer to a request containing the request packet to transmit
             frameType - Specifies the frame type to use
             skip_cca  - For STAR allows skipping the CCA process, argument ignored for SRFN comDeviceGatewayConfig

  Returns:  true  - request was successfully added to transmit queue
            false - An error occurred and the macBuffer is freed.

  Notes: If this function fails, the caller must delete the macBuffer!

 ***********************************************************************************************************************/
bool MAC_Transaction_Create ( buffer_t *macBuffer, uint8_t frameType, bool skip_cca )
{
   MacPacket_s *PacketData;
   bool returnVal = (bool)true;

   MAC_Request_t *pReq = (MAC_Request_t*)macBuffer->data; /*lint !e740 !e826 payload holds the PHY_DataReq_t information */
   MAC_DataReq_t *macRequest = (MAC_DataReq_t *)&pReq->Service.DataReq;    /*lint !e740 !e826 */

   if ( BUF_Get(&MAC_PacketTxBufObj, (void **)&PacketData) == BUF_ERR_NONE )
   {
      PacketData->memToFree    = macBuffer;
      PacketData->handle       = macRequest->handle;
      PacketData->txInProgress = (bool)false;
      PacketData->frame_type   = frameType;
      PacketData->skip_cca     = skip_cca; /* This is only used for STAR transmitters */

      /* current MAC layer does not support ARQ, ignore requests for ARQ */
      macRequest->ackRequired = NO_ACK_REQUIRED;

      INFO_printf("\t\t>>>packet Total segments:  %u", (MAC_CalcNumSegments(macRequest, PHY_GetMaxTxPayload()) + 1) );
      returnVal = PrioritizeAndInsert(PacketData);
      if ( !returnVal )
      {
         if ( BUF_Put(&MAC_PacketTxBufObj, PacketData) != BUF_ERR_NONE )
         {  // Failed, but there is not recovery.  Not sure why this would ever occur!
            ERR_printf("Unable to return tx queue buffer back" );
         }
      }
   }else
   {
      ERR_printf("Failed to get a MAC tx queue buffer" );
      returnVal = (bool)false;
   }

   if ( !returnVal ) {
      MAC_CounterInc(eMAC_TransactionOverflowCount);
   }
   return returnVal;
}


/***********************************************************************************************************************
Function Name: PrioritizeAndInsert

Purpose: This function attempts to insert a packet into the packet tx queue according to QoS rules

  Arguments: MacPacket_s - pointer to the packet to insert

  Returns:  true if packet was successfully added to queue
            false if the packet did not fit or a memory error occurred
***********************************************************************************************************************/
static bool PrioritizeAndInsert( MacPacket_s *packet )
{
   MacPacket_s *TrackPointPacket;
   MacPacket_s *QosSortingPacket;
   uint16_t i;
   uint16_t NumElements;
   bool returnVal = (bool)true;
   MAC_DataReq_t *newMacRequest;
   MAC_DataReq_t *sortingMacRequest;

   newMacRequest = MAC_GetDataReqFromBuffer( (buffer_t*)(packet->memToFree) );

   NumElements = OS_LINKEDLIST_NumElements ( &MAC_PacketTxQueue );
   if ( 0 == NumElements )
   {
      /* no existing elements in queue to prioritize against */
      OS_LINKEDLIST_Enqueue(&MAC_PacketTxQueue, packet); // Function will not return if it fails
   }
   else
   {
      if ( NumElements == MAX_TX_PACKET_BUFFERS )
      {
         /* queue is full, see if an existing element is both droppable & lower priority than the new element */
         QosSortingPacket = OS_LINKEDLIST_Head ( &MAC_PacketTxQueue );
         TrackPointPacket = NULL;
         /* find the lowest priority packet that is droppable and drop it */
         for ( i=0; i<NumElements; i++ )
         {
            sortingMacRequest = MAC_GetDataReqFromBuffer( (buffer_t*)(QosSortingPacket->memToFree) );
            if ( ( sortingMacRequest->droppable ) &&
                 ( !QosSortingPacket->txInProgress ) &&
                 ( sortingMacRequest->priority >= newMacRequest->priority ) )
            {
               /* we found a droppable element, but keep searching in case there is an even lower priority candidate */
               TrackPointPacket = QosSortingPacket;
            }
            QosSortingPacket = OS_LINKEDLIST_Next( &MAC_PacketTxQueue, QosSortingPacket );
         }
         if (TrackPointPacket == NULL)
         {
            /* no droppable messages found, drop the new element instead */
            DBG_logPrintf('I',"Transmit queue full, new packet tx request will be dropped" );
            returnVal = (bool)false;
         }
         else
         {
            OS_LINKEDLIST_Remove( &MAC_PacketTxQueue, TrackPointPacket );
         }
      }

      if (returnVal)
      {
         /* there is space in the queue, insert this element based on priority */
         QosSortingPacket = OS_LINKEDLIST_Head ( &MAC_PacketTxQueue );
         sortingMacRequest = MAC_GetDataReqFromBuffer( (buffer_t*)(QosSortingPacket->memToFree) );
         if ( (sortingMacRequest->priority < newMacRequest->priority) && (!QosSortingPacket->txInProgress ) )
         {
            /* the first element is lower priority than the new element, add it to the begining */
            if ( !OS_LINKEDLIST_Insert( &MAC_PacketTxQueue, NULL, packet ) )
            {
               returnVal = (bool)false;
               DBG_logPrintf('E',"OS_LINKEDLIST_Insert(MAC_PacketTxQueue) failed" );
            }
         }
         else
         {
            /* First element is higher priority than new element or is already transmitting, so start from there */
            TrackPointPacket = QosSortingPacket;
            QosSortingPacket = OS_LINKEDLIST_Next( &MAC_PacketTxQueue, QosSortingPacket );
            for ( i=1; i<NumElements; i++ )
            {
               sortingMacRequest = MAC_GetDataReqFromBuffer( (buffer_t*)(QosSortingPacket->memToFree) );
               if (sortingMacRequest->priority < newMacRequest->priority)
               {
                  /* TrackPointPacket points to the previous element, insert after that element */
                  if ( !OS_LINKEDLIST_Insert( &MAC_PacketTxQueue, TrackPointPacket, packet ) )
                  {
                     returnVal = (bool)false;
                     DBG_logPrintf('E',"OS_LINKEDLIST_Insert(MAC_PacketTxQueue) failed" );
                  }
                  break;
               }
               TrackPointPacket = QosSortingPacket;
               QosSortingPacket = OS_LINKEDLIST_Next( &MAC_PacketTxQueue, QosSortingPacket );
            }
            if ( NULL == QosSortingPacket )
            {
               /* the new element is the lowest priority packet, add to the end */
               (void)OS_LINKEDLIST_Insert( &MAC_PacketTxQueue, TrackPointPacket, packet );
            }
         }
      }
   }
   return returnVal;
}

/***********************************************************************************************************************
Function Name: MAC_PacketManag_AckPending

Purpose: provides information about any frames recieved that need to be ACK'd

Arguments: pointer to be popluated with Request Number (highest Sequence number received in the rx ARQ window)

Returns: true if there are any frames received that need to be ACK'd (and reqNum contains valid value)
         false if there are no frames received that need to be ACK'd (reqNum is unchanged)
***********************************************************************************************************************/
/*lint -efunc(818,MAC_PacketManag_AckPending) arguments not const */
/*lint -esym(715, reqNum) argument not referenced  */
bool MAC_PacketManag_AckPending ( uint8_t *reqNum )
{
   return (bool)false;
}
/*lint +esym(715, reqNum) argument not referenced  */

/******************************************************************************

  Function Name: MAC_CalcNumSegments(MAC_DataReq_t *dataReq)

  Purpose: This function calculates the number of segments that are required to
           transmit a packet.

  Arguments: *dataReq pointer to a MAC Data.request
             maxTxPayload - TX payload size

  Returns: number of segments ( 0 based )

  Notes:

******************************************************************************/
uint8_t MAC_CalcNumSegments(MAC_DataReq_t const *dataReq, uint16_t maxTxPayload)
{
   uint8_t segmentCount = 0;
   uint8_t macHeaderLen = MIN_MAC_HEADER_LEN;

   /* this new packet is now being transmitted */
   if ( dataReq->ackRequired )
   {
      macHeaderLen++;
   }

   if ( dataReq->dst_addr_mode == UNICAST_MODE )
   {
      macHeaderLen += MAC_ADDRESS_SIZE;
   }

   /* 2 bytes of length (length in 1st frame only) */
   macHeaderLen += 2;

   if ( ( maxTxPayload - macHeaderLen ) < dataReq->payloadLength )
   {  // Frame will be segmented
      uint16_t remainingBytes;

      /* 1 byte of segmentation */
      macHeaderLen++;

      /* segment count is 'actual number of segment - 1', so count how many frames needed to send the
         data that didn't fit into the first segment */
      remainingBytes = dataReq->payloadLength - (maxTxPayload - macHeaderLen);

      /* rest of the segments have no length field */
      macHeaderLen -= 2;

      segmentCount = (uint8_t)(remainingBytes / (maxTxPayload - macHeaderLen));
      if ( ( remainingBytes % (maxTxPayload - macHeaderLen) )  != 0 )
      {
         segmentCount++;
      }
   }
   return segmentCount;
}

/***********************************************************************************************************************
Function Name: MAC_GetDataReqFromBuffer

Purpose: Get the data pointer of a MAC data.Request burried in a buffer_t structure.

Arguments: pointer to a buffer_t who has a data pointer to a generic mac request.

Returns: pointer to the MAC_DataReq_t within the buffer_t
***********************************************************************************************************************/
MAC_DataReq_t *MAC_GetDataReqFromBuffer( buffer_t const *macRequestBuff )
{
   MAC_DataReq_t *macRequest;
   MAC_Request_t *pReq;

   pReq = (MAC_Request_t*)macRequestBuff->data; /*lint !e740 !e826 payload holds the MAC_Request_t information */
   macRequest = (MAC_DataReq_t *)&pReq->Service.DataReq;  /*lint !e740 !e826 payload holds the MAC_DataReq_t information */

   return macRequest;
}
