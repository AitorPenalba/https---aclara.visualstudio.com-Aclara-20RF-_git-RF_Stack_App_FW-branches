/********************************************************************************************************************

   Filename: MAC_FrameManagement.c

   Global Designator: MAC_FrameManag_

   Contents:

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2013-2021 Aclara.  All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 ********************************************************************************************************************* */

/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "timer_util.h"
#include "time_util.h"
#include "DBG_SerialDebug.h"
#include "BUF_MemoryBuffers.h"
#if (DCU == 1)
#include "MAINBD_Handler.h" //DCU2+
#include "STAR.h"           //DCU2+
#include "version.h"
#endif
#include "MAC.h"
#include "MAC_Protocol.h"
#include "MAC_PacketManagement.h"
#include "MAC_FrameManagement.h"
#include "MAC_FrameEncoder.h"

/* #DEFINE DEFINITIONS */
// #define MAX_MAC_SEQ_NUMBER 15

#if ( DCU == 1 )
#define NUM_NODES_TRACKED ((uint16_t)800)
#if ( PORTABLE_DCU == 0 )
#define NUM_CONCURRENT_RX_BUFFERS ((uint16_t)850) // 950
#else
#define NUM_CONCURRENT_RX_BUFFERS ((uint16_t)850)
#endif
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#define SEGMENT_STORAGE_POOL_SIZE ((uint16_t)1275) //max is ~12932
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )
#define SEGMENT_STORAGE_POOL_SIZE ((uint16_t)1017)
#endif
#else
#define NUM_NODES_TRACKED ((uint8_t)10)
#define NUM_CONCURRENT_RX_BUFFERS ((uint16_t)5)
#define SEGMENT_STORAGE_POOL_SIZE ((uint16_t)26)
#endif

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
/*lint -esym(750,MAC_FRAME_RETRIES)   */
#define MAC_FRAME_RETRIES 3

typedef enum
{
   eMAC_RXBUFF_UNUSED = 0,   /*!< Buffer is not in use */
   eMAC_RXBUFF_INUSE,        /*!< Buffer is in use */
   eMAC_RXBUFF_STALE         /*!< Buffer contains stale data and needs to be reclaimed */
} MAC_RXBUFF_STATE_e;

typedef struct
{
   uint8_t packetId;
   uint8_t macAddr[MAC_ADDRESS_SIZE];
   uint16_t timerId; /* ID for the alarm which will fire when this packet/macaddr pair should be forgotten */
   bool inUse;
} PacketIdPair_s;

/* used to track and support possible reassembly of incoming MAC Frames */
typedef struct
{
   OS_QUEUE_Element unused; /* OS uses this when passed in messages. */
   uint16_t totalExpectedBytes;  /* The # of bytes segment 1 says should be present in the assembled packet */
   uint16_t bytesReceived; /* Running total of payload bytes in received frames for this packet */
   uint8_t  segmentsRxd[MAX_MAC_FRAME_BUFFERS];   /* an array used to track which segments have been received */
   uint8_t  segmentCount;
   uint8_t  packetId;
   uint16_t timerId; /* ID for alarm which will fire if it time to give up on missing segments */
   TIMESTAMP_t timeStamp; /* TimeStamp associated with when the 1st segment was recieved */
   uint32_t timeStampCYCCNT;            /* CYCCNT time stamp associated with when the payload was received */
   uint8_t dstAddr[MAC_ADDRESS_SIZE];
   uint8_t srcAddr[MAC_ADDRESS_SIZE];
   uint8_t dstAddrMode;
   uint8_t frameType;
   OS_QUEUE_Obj frameQueue;
   uint16_t channel;
   MAC_RXBUFF_STATE_e state;
   uint32_t sync_time; // This is used by set time to measure the delta time
} RxBufferTracking_s;

/* this structure contains only the information that is specific to a segment.  packet wide elements that are the same
   across all segments are stored in the RxBufferTracking_s.  */
typedef struct
{
   OS_QUEUE_Element QueueElement;
   uint8_t  data[PHY_MAX_PAYLOAD];
   uint16_t thisFrameLen;
   float    rssi_dbm;
   float    danl_dbm;
} RxSegmentInfo_s;

static struct{
   uint16_t           InUseCount;    // Total number of buffers in use
   uint16_t           UnicastCount;  // Total number of Unicast in use
   RxBufferTracking_s Buffers[NUM_CONCURRENT_RX_BUFFERS];
#if (DCU == 1 )
} RxBuffers @ "EXTERNAL_RAM" ; /*lint !e430*/
#else
} RxBuffers;
#endif

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
static OS_QUEUE_Obj MAC_FrameTxQueueHandle;
static OS_QUEUE_Obj RxAssemblyTimeoutQueue;
static OS_MSGQ_Obj RxBufferExpiredQueue;
static BUF_Obj MAC_FrameTxBufObj;
static BUF_Obj MAC_FrameRxBufObj;

#if DCU
static RxSegmentInfo_s RxSegmentStorage[(SEGMENT_STORAGE_POOL_SIZE)] @ "EXTERNAL_RAM" ; /*lint !e430*/
static PacketIdPair_s PacketIdPairs[NUM_NODES_TRACKED]               @ "EXTERNAL_RAM" ; /*lint !e430*/
static MAC_FrameManagBuf_s FrameManagTxData[MAX_MAC_FRAME_BUFFERS]   @ "EXTERNAL_RAM" ; /*lint !e430*/
#else
static RxSegmentInfo_s RxSegmentStorage[(SEGMENT_STORAGE_POOL_SIZE)];
static PacketIdPair_s PacketIdPairs[NUM_NODES_TRACKED];
static MAC_FrameManagBuf_s FrameManagTxData[MAX_MAC_FRAME_BUFFERS];
#endif

static uint8_t NextReqNum = 0;

extern TimeSync_t TimeSync;

/* FUNCTION PROTOTYPES */
static void MAC_FrameManag_AssemblePacket( RxBufferTracking_s *rxBuff );
static void MAC_FrameManag_EmptyRxBuffer( RxBufferTracking_s *rxFrameTracking );
static bool MAC_FrameManag_IsDuplicate( const mac_frame_t *frame, RxBufferTracking_s **RxBufferAddr );
static bool MAC_FrameManag_AllocRxData ( RxBufferTracking_s *rxBuff );

static returnStatus_t addRxAssemblyTimer (RxBufferTracking_s *rxBufferToTrack);
static void MAC_FrameManagement_RxAssemblyExpired( uint8_t cmd, void *pData );

static returnStatus_t addPacketIdTrackingTimeout (PacketIdPair_s *rxBufferToTrack );
static void MAC_FrameManagement_PacketTrackingExpired (uint8_t cmd, void *pData);
static void FindRxBuffer( const uint8_t *macAddr, RxBufferTracking_s **matchingBuffer );
static void FindUnusedRxBuffer( RxBufferTracking_s **unusedBuffer );
static void FindPacketIdPair( const uint8_t *macAddr, PacketIdPair_s **matchingPair );
static void FindUnusedPacketIdPair( PacketIdPair_s **unusedPair );

extern MAC_GET_STATUS_e MAC_Attribute_Get(MAC_GetReq_t const *pGetReq, MAC_ATTRIBUTES_u *val);  // Prototype should be only used by MAC.c and MAC_FrameManagement.c.
                                                                                                // We don't want outsiders to call this function directly.
                                                                                                // MAC_GetRequest is used for that.
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************
Function Name: MAC_FrameManag_init

Purpose: Initialize the Frame Management Module before any tasks start in the system

Arguments: none

Returns: RetVal - indicates success or the reason for error
***********************************************************************************************************************/
returnStatus_t MAC_FrameManag_init ( void )
{
   returnStatus_t RetVal = eSUCCESS;
   uint16_t i;

   if ( ! OS_QUEUE_Create(&MAC_FrameTxQueueHandle) )
   {
      RetVal = eFAILURE;
   }

   if ( ! OS_QUEUE_Create(&RxAssemblyTimeoutQueue) )
   {
      RetVal = eFAILURE;
   }

   if ( ! BUF_Create(&MAC_FrameTxBufObj, &(FrameManagTxData[0]), MAX_MAC_FRAME_BUFFERS, sizeof(MAC_FrameManagBuf_s)) )
   {
      RetVal = eFAILURE;
   }

   if ( ! BUF_Create(&MAC_FrameRxBufObj, &(RxSegmentStorage[0]), SEGMENT_STORAGE_POOL_SIZE, sizeof(RxSegmentInfo_s)) )
   {
      RetVal = eFAILURE;
   }

   if ( ! OS_MSGQ_Create(&RxBufferExpiredQueue) )
   {
      RetVal = eFAILURE;
   }

   RxBuffers.InUseCount = 0;
   for (i = 0; i < NUM_CONCURRENT_RX_BUFFERS; i++)
   {

      RxBuffers.Buffers[i].unused.dataLen       = 0;    // Unused
      RxBuffers.Buffers[i].unused.flag.isStatic = true; // Mark buffer as static to avoid freeing
      RxBuffers.Buffers[i].unused.flag.inQueue  = 0;    // Not on queue yet
      RxBuffers.Buffers[i].unused.bufPool       = 0;    // Unused
      RxBuffers.Buffers[i].totalExpectedBytes = 0xFFFF;
      RxBuffers.Buffers[i].bytesReceived      = 0;
      (void)memset(RxBuffers.Buffers[i].segmentsRxd, 0, sizeof(RxBuffers.Buffers[i].segmentsRxd));
      (void)memset(RxBuffers.Buffers[i].dstAddr, 0, sizeof(RxBuffers.Buffers[i].dstAddr));
      (void)memset(RxBuffers.Buffers[i].srcAddr, 0, sizeof(RxBuffers.Buffers[i].srcAddr));
      RxBuffers.Buffers[i].packetId = 0;
      if ( ! OS_QUEUE_Create(&RxBuffers.Buffers[i].frameQueue) )
      {
         RetVal = eFAILURE;
      }
      RxBuffers.Buffers[i].state = eMAC_RXBUFF_UNUSED;
   }

   for (i = 0; i < NUM_NODES_TRACKED; i++)
   {
      PacketIdPairs[i].inUse = (bool)false;
   }

   NextReqNum = 0;

   return ( RetVal );
} /* end MAC_FrameManag_init () */

/***********************************************************************************************************************
Function Name: MAC_FrameManag_AllocTxData

Purpose: This function is used to allocate a Frame Buffer to transmit so a new message can be added to the Frame
   Management Buffer list

Arguments: FrameData - address of the pointer to the Data Buffer (populated by this function)

Returns: BufferAllocated - true if allocation was successful, false otherwise
***********************************************************************************************************************/
bool MAC_FrameManag_AllocTxData ( MAC_FrameManagBuf_s **FrameData )
{
   bool BufferAllocated = (bool)false;

   if ( BUF_Get(&MAC_FrameTxBufObj, (void **)FrameData) == BUF_ERR_NONE )
   {
      BufferAllocated = (bool)true;
   }

   return ( BufferAllocated );
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_AllocRxData

Purpose: This function allocates enough RxSegmentInfo_s's to hold an incoming MAC packet.  Allocation is all or nothing.
   If insufficient memory is free or there is a problem adding the memory to the RxBuffer's segment queue, this function
   will free any segments it successfully allocated and set the rx buffer as not in use before returning.

Arguments: rxBuff - pointer to the rx buffer that will

Returns: successful - true if allocation was successful, false otherwise
***********************************************************************************************************************/
static bool MAC_FrameManag_AllocRxData ( RxBufferTracking_s *rxBuff )
{
   bool successful = (bool)true;
   RxSegmentInfo_s *segment;
   uint8_t i;

   for (i = 0; i < ( rxBuff->segmentCount + 1); i++)
   {
      if ( BUF_Get(&MAC_FrameRxBufObj, (void **)&segment) == BUF_ERR_NONE )
      {
         OS_QUEUE_Enqueue(&rxBuff->frameQueue, segment); // Function will not return if it fails
      }
      else
      {
         successful = (bool)false;
         break;
      }
   }
   if ( ! successful )
   {
      /* something went wrong, free any allocated memory */
      MAC_FrameManag_EmptyRxBuffer( rxBuff );
   }

   return successful;
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_Add_Tx

Purpose: This function is used to add/insert a Frame Data Buffer into the Frame Buffer list (which will then be sent
   to the PHY)

Arguments: FrameData - pointer to the Data Buffer

Returns: None
***********************************************************************************************************************/
void MAC_FrameManag_Add_Tx ( MAC_FrameManagBuf_s *frame )
{
   frame->SentToNextLayer = (bool)false;

   OS_QUEUE_Enqueue(&MAC_FrameTxQueueHandle, frame); // Function will not return if it fails
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_GetNextTxMsg

Purpose: This function is used to search the Frame Buffer and return the next Frame that should be sent to the PHY
   layer (if there is one)

Arguments: FrameData - Pointer to the Data Buffer that is ready to be sent to the PHY (or NULL if no messages are ready)

Returns: Calling function must check the returned pointer for NULL (only use if it is non-NULL)
***********************************************************************************************************************/
MAC_FrameManagBuf_s *MAC_FrameManag_GetNextTxMsg ( void )
{
   uint16_t NumElements;
   uint16_t i;
   MAC_FrameManagBuf_s *FrameData = (void *)0;
   uint8_t reqNumber;
   MAC_FrameManagBuf_s *ackFrame;

   NumElements = OS_QUEUE_NumElements ( &MAC_FrameTxQueueHandle );
   if ( NumElements > 0 )
   {
      /* Search for a message that is ready to send */
      FrameData = OS_QUEUE_Head ( &MAC_FrameTxQueueHandle );
      for ( i=0; i<NumElements; i++ )
      {
         if ( FrameData->SentToNextLayer == (bool)false )
         {
            /* This is the message we should return */
            break;
         }
         /* Move to the next element in the Queue for the next loop */
         FrameData = OS_QUEUE_Next ( &MAC_FrameTxQueueHandle, FrameData );
      }
   }

   /* generate an ACK frame if we have pending ACKs but no data frames they can piggy back on */
   if ( ( FrameData == (void *)0 ) && ( MAC_PacketManag_AckPending( &reqNumber ) ) )
   {
      if ( ! MAC_FrameManag_AllocTxData(&ackFrame) )
      {
         DBG_logPrintf('E',"MAC_FrameManag_AllocTxData() failed" );
      }
      else
      {
         MAC_Codec_SetDefault(&ackFrame->MsgData, MAC_ACK_FRAME);
         ackFrame->MsgData.dst_addr_mode = UNICAST_MODE;
         // xxx jmb todo:  need to track who the ARQ Ack's go back to for destination address
         (void) memset(ackFrame->MsgData.dst_addr, 0, sizeof(ackFrame->MsgData.dst_addr));
         ackFrame->MsgData.ack_present = true;
         ackFrame->MsgData.req_num = reqNumber;
         FrameData = ackFrame;
      }
   }

   return ( FrameData );
} /* end MAC_FrameManag_GetNextTxMsg () */

/***********************************************************************************************************************
Function Name: MAC_FrameManag_CheckTimeouts

Purpose: This function will check various timeouts.  Currently only rx buffer timeouts, but eventually tx buffer
   timeouts.

Arguments: none

Returns: none
***********************************************************************************************************************/
void MAC_FrameManag_CheckTimeouts ( void )
{
   RxBufferTracking_s *rxBuff;

   while ( OS_MSGQ_Pend(&RxBufferExpiredQueue, (void *)&rxBuff, 0) )
   {
      DBG_logPrintf('E',"rx buffer is stale timer expired");
      MAC_FrameManag_EmptyRxBuffer( rxBuff );
   }
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_Get_NextReqNum

Purpose: This function is used to retrieve the next Request Number that we are expecting.  this data can be used to
   populate an Ack Frame message

Arguments: none

Returns: NextReqNum - The next request number (sequence number) that we are expecting
***********************************************************************************************************************/
uint8_t MAC_FrameManag_Get_NextReqNum ( void )
{
   return ( NextReqNum );
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_GetTxQueueSize

Purpose: expose the number of elements in the frame tx queue

Arguments: none

Returns: number of elements in the frame tx queue
***********************************************************************************************************************/
uint16_t MAC_FrameManag_GetTxQueueSize ( void )
{
   return OS_QUEUE_NumElements ( &MAC_FrameTxQueueHandle );
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_PhyDataConfirm

Purpose: Notify the frame manager code that the frame just given to the PHY layer was acknowledged.
         Note that if ACK was requested for that frame, it must be kept around in case no ACK is received & a retransmit is needed

Arguments: none

Returns: number of elements in the frame tx queue
***********************************************************************************************************************/
void MAC_FrameManag_PhyDataConfirm ( MAC_DATA_STATUS_e status)
{
   uint16_t NumElements;
   uint16_t i;
   MAC_FrameManagBuf_s *QuePtr;
   uint8_t sequenceNumber = 0xff;

   /* find the newest element in the tx frame queue.  It was just successfully transmitted */
   NumElements = OS_QUEUE_NumElements ( &MAC_FrameTxQueueHandle );
   if ( NumElements > 0 )
   {
      QuePtr = OS_QUEUE_Head ( &MAC_FrameTxQueueHandle );
      for ( i=1; i < NumElements; i++ )
      {
         QuePtr = OS_QUEUE_Next ( &MAC_FrameTxQueueHandle, QuePtr );
      }

      if ( false == QuePtr->MsgData.ack_required )
      {
         /* this frame can be removed from the tx frame queue */
         OS_QUEUE_Remove ( &MAC_FrameTxQueueHandle, QuePtr );
         if ( BUF_Put(&MAC_FrameTxBufObj, QuePtr) != BUF_ERR_NONE )
         {
            DBG_logPrintf('E',"BUF_PUT failed" );
         }

         // Reset Timesync flag when TX frame is dequeued
         if ( ( QuePtr->MsgData.frame_type == MAC_CMD_FRAME ) && ( QuePtr->MsgData.data[0] == MAC_TIME_SET_CMD ) )
         {
            TimeSync.InQueue = false;
#if ( MAC_LINK_PARAMETERS == 1 )
#if ( DCU == 1 )
            /* Notes: This logic relies on the MAC TIME SET Command being a single frame */
            // Check to see if we have to do anything for MAC_LINK_PARAMETERS Event driven scenario.
            MAC_LinkParameters_EventDriven_Push();
#endif
#endif  // end of ( MAC_LINK_PARAMETERS == 1 ) section
         }

         /* tell packet management code a non ARQ frame was acknowledged */
         MAC_PacketManag_Acknowledge(sequenceNumber, status);
      }
   }
   else
   {
      DBG_logPrintf('E',"Unexpected error:  phy confirmation received but no elements in frame tx queue\n");
   }

}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_DataIndHandler

Purpose: process an incoming phy buffer that contains a mac frame payload

Arguments: none

Returns: number of elements in the frame tx queue
***********************************************************************************************************************/
bool MAC_FrameManag_DataIndHandler ( const mac_frame_t *macFrame, TIMESTAMP_t TimeStamp, uint32_t TimeStampCYCCNT, float rssi_dbm, float danl_dbm, uint16_t chan , uint32_t sync_time )
{
   RxBufferTracking_s *rxBuff = NULL;
   bool isDuplicate;
   bool processingSuccessful = (bool)false;
   RxSegmentInfo_s *segment;
   uint8_t i;

   isDuplicate = MAC_FrameManag_IsDuplicate(macFrame, &rxBuff);

   if ( ! isDuplicate )
   {
      if ( rxBuff == NULL )
      {
         /* dup detection function indicated this is the first received segment (not necessarily segement ID 0) for a new packet */
         FindUnusedRxBuffer(&rxBuff);
         if ( rxBuff != NULL )
         {
            /* this was the first segment received for this packet */
            rxBuff->state           = eMAC_RXBUFF_INUSE;
            rxBuff->segmentCount    = macFrame->segment_count;
            rxBuff->frameType       = macFrame->frame_type;
            rxBuff->sync_time       = sync_time;
            rxBuff->timeStamp       = TimeStamp;
            rxBuff->timeStampCYCCNT = TimeStampCYCCNT;
            rxBuff->dstAddrMode     = macFrame->dst_addr_mode;
            rxBuff->channel         = chan;
            rxBuff->packetId        = macFrame->packet_id;
            (void)memcpy(rxBuff->srcAddr, macFrame->src_addr, MAC_ADDRESS_SIZE);
            (void)memcpy(rxBuff->dstAddr, macFrame->dst_addr, MAC_ADDRESS_SIZE);

            /* Track the number of buffers in use */
            RxBuffers.InUseCount++;
            /* Track the number of unicast packets */
            if(rxBuff->dstAddrMode == UNICAST_MODE)
            {
               RxBuffers.UnicastCount++;
            }

            processingSuccessful = MAC_FrameManag_AllocRxData(rxBuff);
         }
         if ( processingSuccessful )
         {
            (void)addRxAssemblyTimer(rxBuff);
         }
         else
         {
            MAC_CounterInc(eMAC_RxOverflowCount);
            DBG_logPrintf('E',"all RX buffers in use, dropping frame" );
         }
      }
      else
      {
         /* Confirm fields match */
         if ( (rxBuff->dstAddrMode  == macFrame->dst_addr_mode) &&
              (rxBuff->segmentCount == macFrame->segment_count) &&
              (rxBuff->channel      == chan) )
         {
            processingSuccessful = (bool)true;
         }
         else
         {
            MAC_CounterInc(eMAC_MalformedFrameCount);
         }
      }
      /*lint -e{613} if processingSuccessful, rxBuff is NOT null */
      if ( processingSuccessful )
      {
         MAC_CounterInc(eMAC_AcceptedFrameCount);

         if ( ( FRAME_NOT_SEGMENTATED == macFrame->segmentation_enabled ) ||
              ( 0 == macFrame->segment_id ) )
         {
            rxBuff->totalExpectedBytes = macFrame->length;
         }

         // put segment data into proper segment slot
         segment = OS_QUEUE_Head ( &rxBuff->frameQueue );
         for ( i = 0; i < macFrame->segment_id; i++ )
         {
            segment = OS_QUEUE_Next( &rxBuff->frameQueue, segment );
         }
         segment->rssi_dbm = rssi_dbm;
         segment->danl_dbm = danl_dbm;
         segment->thisFrameLen = macFrame->thisFrameLen;
         (void)memcpy(segment->data, macFrame->data, macFrame->thisFrameLen);
         rxBuff->segmentsRxd[macFrame->segment_id] = 1;

         rxBuff->bytesReceived += macFrame->thisFrameLen;

         /* check if this was the last frame in the packet.  For now, allow received bytes being larger
            than expected bytes in case PHY layer changes to tell us full payload lengh instead
            of exact num bytes received */
         if ( rxBuff->bytesReceived >= rxBuff->totalExpectedBytes )
         {
            for ( i = 0; i < (rxBuff->segmentCount+1); i++)
            {
               if ( rxBuff->segmentsRxd[i] == 0 )
               {
                  DBG_logPrintf('E', "All bytes received but still missing segments");
                  processingSuccessful = (bool)false;
               }
            }
            if (processingSuccessful == (bool)true)
            {
               MAC_FrameManag_AssemblePacket( rxBuff );  /*lint !e613 rxBuff is not NULL here */
            }
         }
      }
   }else
   {
      MAC_CounterInc(eMAC_DuplicateFrameCount);
   }

   return processingSuccessful;
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_AssemblePacket

Purpose:

  Arguments:

  Returns:
***********************************************************************************************************************/
/*lint -e{613} rxBuff is known to be non-null in the calling routine */
static void MAC_FrameManag_AssemblePacket( RxBufferTracking_s *rxBuff )
{
   buffer_t *mac_buffer;
   MAC_DataInd_t *mac_indication;
   RxSegmentInfo_s *segment;
   PacketIdPair_s *packetIdPair = NULL;
   uint16_t i;
   uint16_t NumElements;
   uint16_t numBytesCopied = 0;
   float totalRssi = 0.0;
   float totalDanl = 0.0;

   /* no longer need to have timer for recipt of outstanding frames, all are received */
   (void) TMR_DeleteTimer(rxBuff->timerId);
#if 1
   mac_buffer = BM_allocStack( (sizeof(MAC_DataInd_t)) + rxBuff->totalExpectedBytes);
#else
   mac_buffer = BM_allocStack( (sizeof(MAC_Indication_t)) + rxBuff->totalExpectedBytes);
#endif
   NumElements = OS_QUEUE_NumElements ( &rxBuff->frameQueue );

   if (mac_buffer != NULL)
   {
      /* get first segment from rx frame queue */
      segment = OS_QUEUE_Head ( &rxBuff->frameQueue );

      mac_buffer->eSysFmt = eSYSFMT_MAC_INDICATION;

#if 1
      mac_indication = (MAC_DataInd_t*)mac_buffer->data;   /*lint !e826 data holds the MAC_DataInt_t  */
#else
      MAC_Indication_t *pInd =  (MAC_Indication_t *)mac_buffer->data; /*lint !e740 !e826 */
      pInd->Type = eMAC_DATA_IND;
      mac_indication = &pInd->DataInd;
#endif

      mac_indication->dst_addr_mode = rxBuff->dstAddrMode;
      (void)memcpy(mac_indication->dst_addr, rxBuff->dstAddr, MAC_ADDRESS_SIZE);
      (void)memcpy(mac_indication->src_addr, rxBuff->srcAddr, MAC_ADDRESS_SIZE);

      mac_indication->payloadLength = rxBuff->totalExpectedBytes;

      /* loop through frames and add their individual payloads to the frame payload */
      uint16_t bytes_remaining = rxBuff->totalExpectedBytes;
      for (i = 0; i < NumElements; i++)
      {
         // Determine the number of bytes to copy, and copy the payload for the segment
         uint16_t num_bytes = min(bytes_remaining, segment->thisFrameLen);

         (void)memcpy(&mac_indication->payload[numBytesCopied], segment->data, num_bytes);

         numBytesCopied  += num_bytes;
         bytes_remaining -= num_bytes;

         totalRssi += powf((float)10.0, (float) segment->rssi_dbm / (float) 10.0 );
         totalDanl += powf((float)10.0, (float) segment->danl_dbm / (float) 10.0 );

         segment = OS_QUEUE_Next ( &rxBuff->frameQueue, segment );
      }
      mac_indication->segmentCount = rxBuff->segmentCount;
      mac_indication->channel = rxBuff->channel;

      mac_indication->rssi = PHY_DBM_TO_MAC_SCALING( 10.0 * log10( (double)totalRssi / max(NumElements,1 ) ) ); // Average the linear RSSI across all segments and convert back to dBm then remap to 0-4095
      mac_indication->danl = PHY_DBM_TO_MAC_SCALING( 10.0 * log10( (double)totalDanl / max(NumElements,1 ) ) ); // Average the linear RSSI across all segments and convert back to dBm then remap to 0-4095

      mac_indication->timeStamp       = rxBuff->timeStamp;
      mac_indication->timeStampCYCCNT = rxBuff->timeStampCYCCNT;

      // Process the incoming packet that was received
      MAC_ProcessRxPacket(mac_buffer, rxBuff->frameType );

      /* continue to track packet ID and MAC address pairs in case messasge is repeated */
      FindUnusedPacketIdPair(&packetIdPair);
      if ( packetIdPair != NULL )
      {
         packetIdPair->inUse = (bool)true;
         packetIdPair->packetId = rxBuff->packetId;
         (void)memcpy(packetIdPair->macAddr, rxBuff->srcAddr, MAC_ADDRESS_SIZE);
         (void)addPacketIdTrackingTimeout(packetIdPair);
      }
      else
      {
         DBG_logPrintf('I',"Unable to find room for an incoming packet ID pair" );
      }
   }

   MAC_FrameManag_EmptyRxBuffer( rxBuff );
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_IsDuplicate

Purpose:  Handle the logic of determining if this frame is a duplicate, if it is a frame for a new packet, etc.

Arguments:
   packet - the frame to be checked
   RxBufferAddr - will be populated with the rx buffer address that contains other segments packet belongs to if
      packet was not a duplicate, will be set to NULL if packet was a duplicate, or if segment was not part of an
      existing rx buffer.

Returns: true if packet is a duplicate.  False otherwise.

Side Effects:  if this frame has a new packet ID, any current in-progress frames received are thrown out and this new
   packet will be processed.
***********************************************************************************************************************/
static bool MAC_FrameManag_IsDuplicate( const mac_frame_t *frame, RxBufferTracking_s **RxBufferAddr )
{
   PacketIdPair_s *packetIdPair = NULL;
   bool isDuplicate = (bool)false;

/*
   OS_TICK_Struct time1,time2;
   uint32_t TimeDiff;
   boolean Overflow;

_task_stop_preemption();
_time_get_elapsed_ticks(&time1);
*/
   /* check if previous frames from this device are still in reassembly */
   FindRxBuffer(frame->src_addr, RxBufferAddr);
/*
   _time_get_elapsed_ticks(&time2);

   TimeDiff = (uint32_t)_time_diff_milliseconds ( &time2, &time1, &Overflow );
   INFO_printf("test took %ums, TimeDiff);

_task_start_preemption();
*/
   if ( *RxBufferAddr == NULL )
   {
      /* if mac address matches one in packet ID tracking structure */
      FindPacketIdPair(frame->src_addr, &packetIdPair);
      if ( packetIdPair != NULL )
      {
         /* if packet matches tracked packet ID */
         if ( frame->packet_id == packetIdPair->packetId )
         {
            (void) TMR_DeleteTimer(packetIdPair->timerId);
            (void) addPacketIdTrackingTimeout(packetIdPair);
            isDuplicate = (bool)true;
         }
         else
         {
            /* source address device moved on to different packet ID, no longer need to track this one */
            (void) TMR_DeleteTimer(packetIdPair->timerId);
            packetIdPair->inUse = (bool)false;
         }
      }
   }

   else
   {
      /* incoming frame is from same device with a message in assembly, is it the same packet ID? */
      /* if incoming packet ID matches what's in reassmebly */
      if ( frame->packet_id == (*RxBufferAddr)->packetId )
      {
         /* reset timer */
         (void) TMR_DeleteTimer((*RxBufferAddr)->timerId);
         (void) addRxAssemblyTimer(*RxBufferAddr);

         /* is this one of the missing segments? */
         if ( (*RxBufferAddr)->segmentsRxd[frame->segment_id] == 1 )
         {
            isDuplicate = (bool)true;
         }
      }
      else
      {
         /* incoming packet ID does not match what is in reassembly */
         (void) TMR_DeleteTimer((*RxBufferAddr)->timerId);
         MAC_FrameManag_EmptyRxBuffer( *RxBufferAddr );
         *RxBufferAddr = NULL;
      }
   }

   if (isDuplicate )
   {
      DBG_logPrintf('I',"Duplicate MAC frame detected" );
   }
   return isDuplicate;
}


/***********************************************************************************************************************
Function Name: MAC_FrameManag_EmptyRxBuffer

Purpose:  Remove all frame buffers currently in the Rxbuffer structure

Arguments:  queue:  the linked list to empty

Returns: none
***********************************************************************************************************************/
/*lint -e{613} rxBuff is known to be non-null in the calling routine */
static void MAC_FrameManag_EmptyRxBuffer( RxBufferTracking_s *rxBuff )
{
   MAC_FrameManagBuf_s *Packet;

   Packet = OS_QUEUE_Dequeue ( &rxBuff->frameQueue );
   while ( Packet != NULL )
   {
      if ( BUF_Put(&MAC_FrameRxBufObj, Packet) != BUF_ERR_NONE )
      {
         DBG_logPrintf('E',"MAC_FrameManag_EmptyRxBuffer:  BUF_PUT failed" );
      }
      Packet = OS_QUEUE_Dequeue ( &rxBuff->frameQueue );
   }

   // Decrement the number of buffers in use
   RxBuffers.InUseCount--;
   // If this was tracking a unicast packet.
   if(rxBuff->dstAddrMode == UNICAST_MODE)
   {
      // Decrement the number of unicast packets tracked
      RxBuffers.UnicastCount--;
   }
   rxBuff->totalExpectedBytes = 0xFFFF;
   rxBuff->bytesReceived = 0;
   (void)memset(rxBuff->segmentsRxd, 0, sizeof(rxBuff->segmentsRxd));

   rxBuff->state = eMAC_RXBUFF_UNUSED;

   if (RxBuffers.InUseCount > NUM_CONCURRENT_RX_BUFFERS)
   {
      DBG_logPrintf ('E', "###In Use RX Buffer tracking is out of sync###");
   }
}

/***********************************************************************************************************************
Function Name: addRxAssemblyTimer

Purpose: Submit a timer to detect when it has been too long since we received a frame for this overall mac frame.  If
   that timer expires, give up on receiving new frames a clear out tracking to make room for new frames.

Arguments:
   cmd - from the ucCmd member when the timer was created.
   pData - from the pData member when the timer was created

Returns: none
***********************************************************************************************************************/
static returnStatus_t addRxAssemblyTimer (RxBufferTracking_s *rxBufferToTrack)
{
   timer_t tmrSettings;
   returnStatus_t retStatus = eFAILURE;
   uint8_t unused = 0;
   MAC_GetReq_t attribRequest;
   MAC_ATTRIBUTES_u attrib;

   if(rxBufferToTrack != NULL)
   {
      /* get ReassemblyTimeout */
      attribRequest.eAttribute = eMacAttr_ReassemblyTimeout;
      (void)MAC_Attribute_Get(&attribRequest, &attrib);

      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.bOneShot = true;
      tmrSettings.bOneShotDelete = true;
      tmrSettings.bExpired = false;
      tmrSettings.bStopped = false;
      tmrSettings.ulDuration_mS = (attrib.ReassemblyTimeout*1000);
      tmrSettings.pData = rxBufferToTrack;
      tmrSettings.pFunctCallBack = MAC_FrameManagement_RxAssemblyExpired;
      retStatus = TMR_AddTimer(&tmrSettings);
      if (eSUCCESS == retStatus)
      {
         rxBufferToTrack->timerId = tmrSettings.usiTimerId;
      }
      else
      {
         MAC_FrameManagement_RxAssemblyExpired(unused, rxBufferToTrack);
         DBG_logPrintf ('E', "Unable to add Rx Buffer timer");
      }
   }
   return retStatus;
}

/***********************************************************************************************************************
Function Name: MAC_FrameManagement_RxAssemblyExpired

Purpose: Called by the timer to indicate reassembly timed out.  If a segment sits in the rx buffer queue too long
   before the remaining segments arrive, eventually we need to give up on the overall packet and free up memory.  If
   this timer callback is called, it means the rx buffer pointed to by pData is stale and needs to be reclaimed.

Arguments:
   cmd - from the ucCmd member when the timer was created.
   pData - from the pData member when the timer was created

Returns: none
***********************************************************************************************************************/
static void MAC_FrameManagement_RxAssemblyExpired (uint8_t cmd, void *pData)
{
   (void)cmd;
   RxBufferTracking_s *rxBuff = (RxBufferTracking_s*)pData;

   /* submit message to message queue and process in mac thread */
   rxBuff->state = eMAC_RXBUFF_STALE;
   OS_MSGQ_Post(&RxBufferExpiredQueue, pData);
}

/***********************************************************************************************************************
Function Name: MAC_FrameManagement_PacketTrackingExpired

Purpose: Called by the timer to indicate duplicate packet detection should stop for a node.  This will happen if a
   node hasn't been heard from for eMacAttr_PacketTimeOut milliseconds.

Arguments:
   cmd - from the ucCmd member when the timer was created.
   pData - from the pData member when the timer was created

Returns: none
***********************************************************************************************************************/
static void MAC_FrameManagement_PacketTrackingExpired (uint8_t cmd, void *pData)
{
   (void)cmd;
   PacketIdPair_s *packetTracked = (PacketIdPair_s*)pData;

   packetTracked->inUse = (bool)false;
}

/***********************************************************************************************************************
Function Name: addPacketIdTrackingTimeout

Purpose: Submit a timer to signal duplicate packet detection should stop for a node

Arguments:
   cmd - from the ucCmd member when the timer was created.
   pData - from the pData member when the timer was created

Returns: none
***********************************************************************************************************************/
static returnStatus_t addPacketIdTrackingTimeout (PacketIdPair_s *PacketIdPairToTrack )
{
   timer_t           tmrSettings;
   returnStatus_t    retStatus = eFAILURE;
   uint8_t           unused = 0;
   MAC_GetReq_t      attribRequest;
   MAC_ATTRIBUTES_u  attrib;

   /* get ReassemblyTimeout */
   attribRequest.eAttribute = eMacAttr_PacketTimeout;
   (void)MAC_Attribute_Get(&attribRequest, &attrib);

   if (0 < attrib.PacketTimeout)
   {
      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.bOneShot = true;
      tmrSettings.bOneShotDelete = true;
      tmrSettings.bExpired = false;
      tmrSettings.bStopped = false;
      tmrSettings.ulDuration_mS = attrib.PacketTimeout;
      tmrSettings.pData = PacketIdPairToTrack;
      tmrSettings.pFunctCallBack = MAC_FrameManagement_PacketTrackingExpired;
      retStatus = TMR_AddTimer(&tmrSettings);
      if (eSUCCESS == retStatus)
      {
         PacketIdPairToTrack->timerId = tmrSettings.usiTimerId;
      }
      else
      {
         MAC_FrameManagement_PacketTrackingExpired(unused, PacketIdPairToTrack);
         DBG_logPrintf ('E', "Unable to add Packet Id pair timer");
      }
   }
   else
   {
      MAC_FrameManagement_PacketTrackingExpired(unused, PacketIdPairToTrack);
   }
   return retStatus;
}

/***********************************************************************************************************************
Function Name: FindRxBuffer

Purpose:  search rx buffers for a buffer containing rx segments from a specified mac address.

Arguments:
   macAddr - pointer to 5 byte MAC address to search against.
   matchingBuffer - Populated with buffer address if matching buffer was found.  Populated with NULL otherwise.

Returns: none
***********************************************************************************************************************/
static void FindRxBuffer( const uint8_t *macAddr, RxBufferTracking_s **matchingBuffer )
{
   uint16_t i;
   *matchingBuffer = NULL;

   for (i = 0; i < NUM_CONCURRENT_RX_BUFFERS; i++)
   {
      if ( ( RxBuffers.Buffers[i].state == eMAC_RXBUFF_INUSE ) &&
           ( memcmp( RxBuffers.Buffers[i].srcAddr, macAddr, MAC_ADDRESS_SIZE) == 0 ) )
      {
         *matchingBuffer = &RxBuffers.Buffers[i];
         break;
      }
   }
}

/***********************************************************************************************************************
Function Name: FindPacketIdPair

Purpose:  search tracked Packet ID pairs for a buffer containing a matching mac address.

Arguments:
   macAddr - pointer to 5 byte MAC address to search against.
   matchingPair - Populated with address if matching Packet ID pair was found.  Populated with NULL otherwise.

Returns: none
***********************************************************************************************************************/
static void FindPacketIdPair( const uint8_t *macAddr, PacketIdPair_s **matchingPair )
{
   uint16_t i;
   *matchingPair = NULL;

   for (i = 0; i < NUM_NODES_TRACKED; i++)
   {
      /* check if device has been heard from recently */
      if ( ( PacketIdPairs[i].inUse ) && ( memcmp( PacketIdPairs[i].macAddr, macAddr, MAC_ADDRESS_SIZE) == 0 ) )
      {
         *matchingPair = &PacketIdPairs[i];
         break;
      }
   }
}

/***********************************************************************************************************************
Function Name: FindUnusedRxBuffer

Purpose:  search rx buffers for an unused buffer

Arguments:
   unusedBuffer - Populated with buffer address if unsued buffer was found.  Populated with NULL otherwise.

Returns: none
***********************************************************************************************************************/
static void FindUnusedRxBuffer( RxBufferTracking_s **unusedBuffer )
{
   uint16_t i;
   *unusedBuffer = NULL;

   for (i = 0; i < NUM_CONCURRENT_RX_BUFFERS; i++)
   {
      if ( RxBuffers.Buffers[i].state == eMAC_RXBUFF_UNUSED )
      {
         *unusedBuffer = &RxBuffers.Buffers[i];
         break;
      }
   }
}

/***********************************************************************************************************************
Function Name: FindUnusedPacketIdPair

Purpose:  search packet ID pair tracking for an unused entry

Arguments:
   unusedBuffer - Populated with tracking entry address if unsued entry was found.  Populated with NULL otherwise.

Returns: none
***********************************************************************************************************************/
static void FindUnusedPacketIdPair( PacketIdPair_s **unusedPair )
{
   uint16_t i;
   *unusedPair = NULL;

   for (i = 0; i < NUM_NODES_TRACKED; i++)
   {
      if ( ! PacketIdPairs[i].inUse )
      {
         *unusedPair = &PacketIdPairs[i];
         break;
      }
   }
}

/***********************************************************************************************************************
Function Name: MAC_FrameManag_IsPacketInReassembly

Purpose:  This function is used to determine if there are any packets in re-assembly.

Arguments:

Returns: true/false
***********************************************************************************************************************/
bool MAC_FrameManag_IsPacketInReassembly()
{
   bool bPacketInReassembly = (bool)false;
   if(RxBuffers.InUseCount > 0)
   {
      bPacketInReassembly = (bool)true;
   }
   return bPacketInReassembly;
}


/*
 * Check if the unit with this address has a packet in reassembly
 */
bool MAC_FrameManag_IsUnitTransmitting(uint8_t const mac_addr[MAC_ADDRESS_SIZE])
{
   uint16_t i;
   bool bPacketInReassembly = (bool)false;
   for (i = 0; i < NUM_CONCURRENT_RX_BUFFERS; i++)
   {
      if ( RxBuffers.Buffers[i].state == eMAC_RXBUFF_INUSE )
      {
         if(memcmp(mac_addr, RxBuffers.Buffers[i].srcAddr, MAC_ADDRESS_SIZE) == 0)
         {
            bPacketInReassembly = (bool)true;
            break;
         }
      }
   }
   return bPacketInReassembly;
}


/*
 * Check if there are any unicast packets in reassembly
 */
bool MAC_FrameManag_IsUnicastPacketInReassembly(void)
{
   bool bPacketInReassembly = (bool)false;
   if(RxBuffers.UnicastCount > 0)
   {
      bPacketInReassembly = (bool)true;
   }
   return bPacketInReassembly;
}

