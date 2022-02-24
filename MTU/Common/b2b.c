/** ****************************************************************************
@file b2b.c

Implements the B2B protocol for the SRFN DA project.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdint.h>

#include "project.h" // required for a lot of random headers so be safe and include 1st
#include "b2b_req_handler.h"
#include "b2bmessage.h"
#include "buffer.h"
#include "DBG_SerialDebug.h"
#include "hdlc_frame.h"

#include "b2b.h"

/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/
#if( RTOS_SELECTION == FREE_RTOS )
#define B2B_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define B2B_NUM_MSGQ_ITEMS 0 
#endif
/*******************************************************************************
Local #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
*******************************************************************************/

/*******************************************************************************
Static Function Prototypes
*******************************************************************************/

static void HandleFrame(hdlc_addr_t addr, hdlc_ctrl_t ctrl, const uint8_t *data, uint16_t nr_of_bytes, bool overflow);
static void SendHdlcData(uint8_t data);

static B2BPacketHeader_t GetPacketHeader(const uint8_t *data);

static bool IsRespPacket(B2BMsg_t type);

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

/** Handle for sent data incremented on all requests. */
static uint16_t handle;

static OS_MUTEX_Obj cmdRespMutex;
static OS_MUTEX_Obj handleMutex;
static OS_MSGQ_Obj respQueue;

static bool waitingForResp;
static B2BMsg_t expectedResp;
static uint16_t expectedRespHandle;

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Initializes the B2B functionality.

@return eSUCCESS on success else eFAILURE.
*/
returnStatus_t B2BInit(void)
{
   if (!OS_MUTEX_Create(&cmdRespMutex))
   {
      ERR_printf("Error creating B2B command and response mutex");
      return eFAILURE;
   }

   if (!OS_MUTEX_Create(&handleMutex))
   {
      ERR_printf("Error creating B2B handle mutex");
      return eFAILURE;
   }

   if (!OS_MSGQ_Create(&respQueue, B2B_NUM_MSGQ_ITEMS))
   {
      ERR_printf("Error creating B2B response queue");
      return eFAILURE;
   }

   handle = 0;
   waitingForResp = false;

   HdlcFrameInit(SendHdlcData, HandleFrame);

   return eSUCCESS;
}

/**
Task used to watch for incoming B2B traffic.

@param arg0 required for MQX tasks but unused.
*/
void B2BRxTask(uint32_t arg0)
{
   uint8_t rxByte;
   (void)arg0;

   while (UART_read(UART_HOST_COMM_PORT, &rxByte, 1))
   {
      HdlcFrameOnRxByte(rxByte);
   }
}

/**
Sends a B2B request and waits for a response.

@param data Pointer to the packet data and frame information.
@param retries Number of times to retry if there is no valid response.
@param timeout Time to wait for a single response in mSec.
@param expResp Specifies the expected response message type.
@param expRespHandle Specifies expected handle of response message.

@return Pointer to response buffer if response detected else NULL.
*/
buffer_t *SendRequest(const HdlcFrameData_t *data, uint8_t retries, uint16_t timeout, B2BMsg_t expResp, uint16_t expRespHandle)
{
   buffer_t *result = NULL;

   // only support 1 command with response message at a time
   OS_MUTEX_Lock(&cmdRespMutex);

   // make sure resp queue is clear
   while (OS_MSGQ_Pend(&respQueue, (void*)&result, 0))
   {
      BM_free(result);
   }

   result = NULL;
   expectedResp = expResp;
   expectedRespHandle = expRespHandle;

   waitingForResp = true;

   do
   {
      // transmit the request
      HdlcFrameTx(data);

      // wait for the response until timeout is over
      if (OS_MSGQ_Pend(&respQueue, (void*)&result, timeout))
      {
         // Got result data so can exit the wait
         break;
      }
   }
   while (retries-- != 0);

   waitingForResp = false;

   OS_MUTEX_Unlock(&cmdRespMutex);

   return result;
}

/**
Gets the next available handle for a packet to use.

@return Next handle number.
*/
uint16_t GetNextHandle(void)
{
   uint16_t result;

   OS_MUTEX_Lock(&handleMutex);

   result = handle;
   handle++;

   OS_MUTEX_Unlock(&handleMutex);

   return result;
}

/**
Stores a Uint16 value in the B2B byte format.

@param buffer Pointer to B2B packet buffer.
@param value The value to store.
*/
void B2BStoreUint16(uint8_t *buffer, uint16_t value)
{
   *buffer++ = (uint8_t)(value >> 8);
   *buffer = (uint8_t)value;
}

/**
Stores a Uint32 value in the B2B byte format.

@param buffer Pointer to B2B packet buffer.
@param value The value to store.
*/
void B2BStoreUint32(uint8_t *buffer, uint32_t value)
{
   *buffer++ = (uint8_t)(value >> 24);
   *buffer++ = (uint8_t)(value >> 16);
   *buffer++ = (uint8_t)(value >> 8);
   *buffer = (uint8_t)value;
}

/**
Gets a Uint16 value from a B2B packet.

@param buffer Pointer to B2B packet buffer.

@return Value in processor byte order.
*/
uint16_t B2BGetUint16(const uint8_t *buffer)
{
   uint16_t result = *buffer++ << 8;
   result |= *buffer;

   return result;
}

/**
Gets a Uint32 value from a B2B packet.

@param buffer Pointer to B2B packet buffer.

@return Value in processor byte order.
*/
uint32_t B2BGetUint32(const uint8_t *buffer)
{
   uint32_t result = *buffer++ << 24; /*lint !e701 */
   result |= *buffer++ << 16;
   result |= *buffer++ << 8;
   result |= *buffer;

   return result;
}

/**
This function will be called from the RX task context to parse received HDLC frames.

@param addr The address the frame was sent to.
@param ctrl The control value of the frame.
@param data pointer to any information included with the frame.
@param nr_of_bytes The munber of information bytes.
@param overflow Set if there was not enough space to store all of the information bytes.
*/
static void HandleFrame(hdlc_addr_t addr, hdlc_ctrl_t ctrl, const uint8_t *data, uint16_t nr_of_bytes, bool overflow)
{
   (void)ctrl; // not checked in connectionless code

   if (overflow)
   {
      ERR_printf("B2B HDLC frame overflow detected");
      return;
   }

   // search for responses and handle them directly
   if (addr == B2B_HDLC_ADDR)
   {
      if (nr_of_bytes < B2B_PKT_HDR_SZ)
      {
         ERR_printf("B2B packet smaller than header detected");
         return;
      }

      B2BPacketHeader_t header = GetPacketHeader(data);

      if ((header.version != B2B_VER) || (header.length != nr_of_bytes))
      {
         ERR_printf("B2B invalid header detected");
         return;
      }

      const uint8_t *params = &data[B2B_PKT_HDR_SZ];

      if (IsRespPacket(header.messageType))
      {
         if (waitingForResp && (header.handle == expectedRespHandle) && (header.messageType == expectedResp))
         {
            // this is a response to an active request so store response info for processing
            uint16_t respDataLen = nr_of_bytes - B2B_PKT_HDR_SZ;
            buffer_t *respData = BM_alloc(respDataLen);

            if (respData != NULL)
            {
               (void)memcpy(respData->data, params, respDataLen);

               // as byte processing thread could technically get another packet right away clear wait
               waitingForResp = false;

               INFO_printf("B2B Got resp %d", header.messageType);

               OS_MSGQ_Post(&respQueue, (void*)respData);
            }
            else
            {
               ERR_printf("B2B could not buffer response type %u of length %u", header.messageType, respDataLen);
            }
         }
         else
         {
            ERR_printf("B2B dropping unexpected response type %u handle %u", header.messageType, header.handle);
         }
      }
      else
      {
         INFO_printf("B2B Got request %d", header.messageType);

         // Not a response so call platform specific handler code
         HandleB2BRequest(header, params);
      }
   }
   else
   {
      INFO_printf("B2B Got non-B2B data on address %d", addr);

      // non b2b type packet so call platform specific handler
      HandleNonB2BRequest(addr, data, nr_of_bytes);
   }
}

/**
Function for the HDLC layer to use when transmitting a byte of data.

@param data The byte to transfer.
*/
static void SendHdlcData(uint8_t data)
{
   (void)UART_write(UART_HOST_COMM_PORT, &data, 1);
}

/**
Gets the B2B header information from raw packet bytes.

@param data Pointer to the raw packet bytes.

@return Structure with all of the header information.
*/
static B2BPacketHeader_t GetPacketHeader(const uint8_t *data)
{
   B2BPacketHeader_t value;

   value.version = *data++;
   value.length = B2BGetUint16(data);
   data += sizeof(uint16_t);
   value.messageType = (B2BMsg_t)B2BGetUint16(data);
   data += sizeof(uint16_t);
   value.handle = B2BGetUint16(data);

   return value;
}

/**
Checks if a message type is a response to a request.

@param type The message type.

@return true if message is a response, else false.
*/
static bool IsRespPacket(B2BMsg_t type)
{
   if (type == B2B_MT_GET_RESP || type == B2B_MT_ECHO_RESP
       || type == B2B_MT_SET_RESP || type == B2B_MT_TIME_RESP || type == B2B_MT_IDENT_RESP)
   {
      return true;
   }

   return false;
}
