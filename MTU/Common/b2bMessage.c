/** ****************************************************************************
@file b2bCommand.c

Implements commands to send the different B2B messages.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdint.h>

#include "b2b.h"
#include "b2b_req_handler.h"
#include "buffer.h"
#include "DBG_SerialDebug.h"
#include "hdlc_frame.h"
#include "OS_aclara.h"
#include "time_util.h"
#include "time_sync.h"

#include "b2bmessage.h"

/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/

#define B2B_GET_REQ_PARAM_SZ 2
#define B2B_BASE_GET_RESP_PARAM_SZ 3
#define B2B_BASE_SET_REQ_PARAM_SZ 2
#define B2B_BASE_SET_RESP_PARAM_SZ 3
#define B2B_PKT_TIME_PARAM_SZ 10

#define DEF_RETRIES  1
#define DEF_TIMEOUT 500

/*******************************************************************************
Local #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
*******************************************************************************/

/*******************************************************************************
Static Function Prototypes
*******************************************************************************/

static void StorePacketHeader(uint8_t ver, uint16_t len, B2BMsg_t type, uint16_t handle, uint8_t *buffer);

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

/** Stores the HDLC control value as it will not change. */
static const hdlc_ctrl_t defCtrlValue = (hdlc_ctrl_t)HDLC_FRAME_U;

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Requests the board identifying information from the attached interface board.

@paran buffer [out] Pointer to location to store the identification information.
@param bufferLen Number of bytes that can be stored.
@param numBytesRec [out] Pointer to location to store number of bytes received in response.

@return true if got response and stored it all, else false.
*/
bool B2BSendIdentityReq(uint8_t *buffer, uint16_t bufferLen, uint16_t *numBytesRec)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ;
   buffer_t *packet = BM_alloc(LENGTH);
   bool result = false;

   if (packet == NULL)
   {
      return result;
   }

   uint16_t handle = GetNextHandle();
   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_IDENT_REQ, handle, packet->data);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Ident Req");

   buffer_t *respData = SendRequest(&frame, DEF_RETRIES, 5000, B2B_MT_IDENT_RESP, handle);

   if (respData != NULL)
   {
      // parse and return result
      uint16_t respLen = respData->x.dataLen;
      uint8_t *params = respData->data;

      if (respLen <= bufferLen)
      {
         (void)memcpy(buffer, params, respLen);
         *numBytesRec = respLen;
         result = true;
      }

      BM_free(respData);
   }

   BM_free(packet);

   return result;
}

/**
Requests data from another device using the B2B protocol.

@param type The type of data to request.
@param buffer [out] Pointer to storage location for data.
@param bufferLen Number of storage bytes available.

@return Device response or B2B_GET_GEN_ERR if error sending message.
*/
B2BGetResp_t B2BGetParameter(meterReadingType type, void *buffer, uint16_t bufferLen)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ + B2B_GET_REQ_PARAM_SZ;
   buffer_t *packet = BM_alloc(LENGTH);
   B2BGetResp_t result = B2B_GET_GEN_ERR;

   if (packet == NULL)
   {
      return result;
   }

   uint16_t handle = GetNextHandle();
   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_GET_REQ, handle, packet->data);

   uint8_t *params = &packet->data[B2B_PKT_HDR_SZ];
   B2BStoreUint16(params, (uint16_t)type);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Get Param Req");

   buffer_t *respData = SendRequest(&frame, DEF_RETRIES, DEF_TIMEOUT, B2B_MT_GET_RESP, handle);

   if (respData != NULL)
   {
      // parse and return result
      uint16_t respLen = respData->x.dataLen;
      params = respData->data;

      result = (B2BGetResp_t)*params++;
      respLen--;

      uint16_t respType = B2BGetUint16(params);
      params += sizeof(uint16_t);
      respLen -= sizeof(uint16_t);

      if ((respType != (uint16_t)type) || (respLen > bufferLen))
      {
         // cannot store data or somehow got wrong data
         result = B2B_GET_GEN_ERR;
      }

      if (result == B2B_GET_SUCCESS)
      {
         (void)memcpy(buffer, params, respLen);
      }

      BM_free(respData);
   }

   BM_free(packet);

   return result;
}

/**
Sends a packet to other device to set a value.

@param type The type of value to set.
@param buffer [in/out] Passes the new value in.  Returns echo of new value.
@param bufferLen number of bytes of the new value.

@return response from device or B2B_SET_GEN_ERR if could not send.
*/
B2BSetResp_t B2BSetParameter(meterReadingType type, void *buffer, uint16_t bufferLen)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ + B2B_BASE_SET_REQ_PARAM_SZ + bufferLen;
   buffer_t *packet = BM_alloc(LENGTH);
   B2BSetResp_t result = B2B_SET_GEN_ERR;

   if (packet == NULL)
   {
      return result;
   }

   uint16_t handle = GetNextHandle();
   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_SET_REQ, handle, packet->data);

   uint8_t *params = &packet->data[B2B_PKT_HDR_SZ];

   B2BStoreUint16(params, (uint16_t)type);
   params += 2;

   (void)memcpy(params, buffer, bufferLen);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Set Param Req");

   buffer_t *respData = SendRequest(&frame, DEF_RETRIES, DEF_TIMEOUT, B2B_MT_SET_RESP, handle);

   if (respData != NULL)
   {
      // parse and return result
      uint16_t respLen = respData->x.dataLen;
      params = respData->data;

      result = (B2BSetResp_t)*params++;
      respLen--;

      uint16_t respType = B2BGetUint16(params);
      params += sizeof(uint16_t);
      respLen -= sizeof(uint16_t);

      if ((respType != (uint16_t)type) || (respLen > bufferLen))
      {
         // cannot store data or somehow got wrong data
         result = B2B_SET_GEN_ERR;
      }

      if (result == B2B_SET_SUCCESS)
      {
         (void)memcpy(buffer, params, respLen);
      }

      BM_free(respData);
   }

   BM_free(packet);

   return result;
}

/**
Requests the other devices time.

@param time [out] Pointer to storage location for time.

@return true if response received else false.
*/
bool B2BSendTimeReq(time_set_t *time)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ;
   buffer_t *packet = BM_alloc(LENGTH);
   bool result = false;

   if (packet == NULL)
   {
      return result;
   }

   uint16_t handle = GetNextHandle();
   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_TIME_REQ, handle, packet->data);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Time Req");

   buffer_t *respData = SendRequest(&frame, DEF_RETRIES, DEF_TIMEOUT, B2B_MT_TIME_RESP, handle);

   if (respData != NULL)
   {
      // parse and return result
      uint8_t *params = respData->data;

      time->ts.QFrac   = B2BGetUint32(params); // QFrac is before "seconds" in memory.
      params += sizeof(uint32_t);
      time->ts.seconds = B2BGetUint32(params);
      params += sizeof(uint32_t);

      uint8_t leapPoll = *params++;

      time->LeapIndicator = (leapPoll >> 6) & 0x3;
      time->Poll = leapPoll & 0x3F;

      time->Precision = (int8_t)*params;

      BM_free(respData);
      result = true;
   }

   BM_free(packet);

   return result;
}

/**
Sends an echo request packet.

@return true if got response, else false.
*/
bool B2BSendEchoReq(void)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ;
   buffer_t *packet = BM_alloc(LENGTH);
   bool result = false;

   if (packet == NULL)
   {
      return result;
   }

   uint16_t handle = GetNextHandle();
   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_ECHO_REQ, handle, packet->data);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Echo Req");

   buffer_t *respData = SendRequest(&frame , DEF_RETRIES, DEF_TIMEOUT, B2B_MT_ECHO_RESP, handle);

   if (respData != NULL)
   {
      // data was just an empty buffer to signal response came
      BM_free(respData);
      result = true;
   }

   BM_free(packet);

   return result;
}

/**
Sends a packet to request a reset.

@return true if sent, else false.
*/
bool B2BSendReset(void)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_RESET, GetNextHandle(), packet->data);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Reset Req");

   HdlcFrameTx(&frame);

   BM_free(packet);

   return true;
}

/**
Sends a packet to request decomissioning.

@return true if sent, else false.
*/
bool B2BSendDecommission(void)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_DECOM, GetNextHandle(), packet->data);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Decom Req ");

   HdlcFrameTx(&frame);

   return true;
}

/**
Sends a packet to request network status refresh.

@return true if sent, else false.
*/
bool B2BSendRefreshNw(void)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_REFRESH_NW_REQ, GetNextHandle(), packet->data);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Refresh NW Req");

   HdlcFrameTx(&frame);

   return true;
}

/**
Sends network gateway traffic to another device.

@param buffer [out] Pointer to the network data.
@param bufferLen Number of bytes of data.

@return true if messge sent else false.
*/
bool B2BSendGatewayTraffic(const uint8_t *buffer, uint16_t bufferLen)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ + bufferLen;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_BASIC_GATEWAY, GetNextHandle(), packet->data);

   uint8_t *params = &packet->data[B2B_PKT_HDR_SZ];

   (void)memcpy(params, buffer, bufferLen);

   HdlcFrameData_t frame = { NW_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Gateway Traffic");

   HdlcFrameTx(&frame);

   BM_free(packet);

   return true;
}

/**
Sends a time sync packet.

@param time The time sync information.

@return true if sent else false.
*/
bool B2BSendTimeSync(void)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ + B2B_PKT_TIME_PARAM_SZ;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_TIME_SYNC, GetNextHandle(), packet->data);

   uint8_t *params = &packet->data[B2B_PKT_HDR_SZ];

   // Adds info for leap/poll/precision and must be called with pointer to start of param space
   TIME_SYNC_AddTimeSyncPayload(params);

   TIMESTAMP_t time;
   (void)TIME_UTIL_GetTimeInSecondsFormat(&time);

   B2BStoreUint32(params, time.QFrac); // QFrac is before "seconds" in memory.
   params += sizeof(uint32_t);
   B2BStoreUint32(params, time.seconds);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Time Sync");

   HdlcFrameTx(&frame);

   BM_free(packet);

   return true;
}

/**
Sends a time request response packet.

@param reqHandle The handle of the request.

@return true if sent else false.
*/
bool B2BSendTimeResp(uint16_t reqHandle)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ + B2B_PKT_TIME_PARAM_SZ;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_TIME_RESP, reqHandle, packet->data);

   uint8_t *params = &packet->data[B2B_PKT_HDR_SZ];

   // Adds info for leap/poll/precision and must be called with pointer to start of param space
   TIME_SYNC_AddTimeSyncPayload(params);

   TIMESTAMP_t time;
   (void)TIME_UTIL_GetTimeInSecondsFormat(&time);

   B2BStoreUint32(params, time.QFrac); // QFrac is before "seconds" in memory.
   params += sizeof(uint32_t);
   B2BStoreUint32(params, time.seconds);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Time Sync Resp");

   HdlcFrameTx(&frame);

   BM_free(packet);

   return true;
}

/**
Sends a response message to a get request.

@param status Result of the request.
@param reqHandle Handle of the request.
@param type Type of data requested.
@param Buffer with the response data.
@param bufferLen Number of bytes in the response data.

@return true if sent, else false.
*/
bool B2BSendGetResp(B2BGetResp_t status, uint16_t reqHandle, meterReadingType type, const void *buffer, uint16_t bufferLen)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ + B2B_BASE_GET_RESP_PARAM_SZ + bufferLen;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_GET_RESP, reqHandle, packet->data);

   uint8_t *params = &packet->data[B2B_PKT_HDR_SZ];

   *params++ = (uint8_t)status;
   B2BStoreUint16(params, (uint16_t)type);
   params += sizeof(uint16_t);

   if (bufferLen != 0)
   {
      (void)memcpy(params, buffer, bufferLen);
   }

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Get Param Resp");

   HdlcFrameTx(&frame);

   BM_free(packet);

   return true;
}

/**
Sends a response message to a set request.

@param status Result of the request.
@param reqHandle Handle of the request.
@param type Type of data that was set.
@param Buffer to store the new value.
@param bufferLen Number of bytes that can be stored.

@return true if sent, else false.
*/
bool B2BSendSetResp(B2BSetResp_t status, uint16_t reqHandle, meterReadingType type, const void *buffer, uint16_t bufferLen)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ + B2B_BASE_SET_RESP_PARAM_SZ + bufferLen;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_SET_RESP, reqHandle, packet->data);

   uint8_t *params = &packet->data[B2B_PKT_HDR_SZ];

   *params++ = (uint8_t)status;
   B2BStoreUint16(params, (uint16_t)type);
   params += sizeof(uint16_t);

   if (bufferLen != 0)
   {
      (void)memcpy(params, buffer, bufferLen);
   }

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Set Param Resp");

   HdlcFrameTx(&frame);

   BM_free(packet);

   return true;
}

/**
Sends an echo response packet.

@param reqHandle The requests handle.

@return true if sent, else false.
*/
bool B2BSendEchoResp(uint16_t reqHandle)
{
   const uint16_t LENGTH = B2B_PKT_HDR_SZ;
   buffer_t *packet = BM_alloc(LENGTH);

   if (packet == NULL)
   {
      return false;
   }

   StorePacketHeader(B2B_VER, LENGTH, B2B_MT_ECHO_RESP, reqHandle, packet->data);

   HdlcFrameData_t frame = { B2B_HDLC_ADDR, defCtrlValue, packet->data, LENGTH };

   INFO_printf("B2B Send Echo Resp");

   HdlcFrameTx(&frame);

   BM_free(packet);

   return true;
}

/**
Stores B2B header in the B2B packet format.

@param ver The B2B protocol version.
@param len Entire length of the B2B packet.
@param type B2B packet type.
@param handle B2B message handle used to detect responses and dupes.
@param buffer Pointer to the packet buffer.
*/
static void StorePacketHeader(uint8_t ver, uint16_t len, B2BMsg_t type, uint16_t handle, uint8_t *buffer)
{
   *buffer++ = ver;
   B2BStoreUint16(buffer, len);
   buffer += sizeof(uint16_t);
   B2BStoreUint16(buffer, (uint16_t)type);
   buffer += sizeof(uint16_t);
   B2BStoreUint16(buffer, handle);
}
