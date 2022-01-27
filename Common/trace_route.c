/******************************************************************************
 *
 * Filename: trace_route.c
 *
 * Global Designator:  TraceRoute
 *
 * Contents: This file contains functions that handle the Trace Rounte request
 *
 ******************************************************************************
 * Copyright (c) 2016 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <stdint.h>
#include <stdbool.h>

#include "heep.h"
#include "HEEP_util.h"

#include "trace_route.h"
#include "CompileSwitch.h"

#include "DBG_SerialDebug.h"
#include "time_util.h"
#include "pack.h"
#include "MAC.h"

/* #DEFINE DEFINITIONS */
#define MAX_TR_RESOURCE_SIZE 125 /*!< Maximum size of any TR Resource */


/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

__no_init struct{
   uint16_t InitCode;  /*!< Used to detect powerup    */
   uint16_t Handle;    /*!< DCU's Trace Route Handle  */
}  TraceRoute_;


/* LOCAL FUNCTION PROTOTYPES */

static uint16_t GetNextHandle(void);
static void PingRequest(uint16_t handle, eui_addr_t const eui_addr);

static uint8_t HE_Context; //context for the last recieved tr request


/*!
 **********************************************************************************************************************
 *
 * \fn void TraceRoute_MsgHandler
 *
 * \brief This is the message handler for a TraceRoute message.
 *
 * \param  appMsgRxInfo
 * \param  payloadBuf
 * \param  length
 *
 * \return  none
 *
 * \details
 *
 *    This function is only supported on the DCU (Frodo).
 *
 *    The Trace Route function is used to "Ping" a device.  This can be either the DCU (i.e Frodo)
 *    of an End Point.
 *
 *    When the device matches the IP address of this device, the response will be sent immediatley
 *    and no RF will occur.
 *
 *    When the Target Device does not match, a MAC_Ping Request() is created, causing a Ping Request
 *    Command to transmitted over the RF link.
 *
 *    The Trace Route Request has 1 to 3 paramters.
 *       trTargetMacAddress [required]
 *       trRspAddrMode      [optional, default = BROADCAST]
 *       trPingCountReset   [optional, default = false]
 *
 *    If trTargetMacAddress is not specified, or
 *       a parameter that is not supported is included or
 *       the parameter is not the expected type
 *
 *       the appMsgTxInfo.Status = BadRequest
 *       The command command is aborted.
 *
 *    If the Status == OK
 *       the Frodo will generate a new Handle
 *       then it will send a response to the request.
 *
 *       Header:
 *          Interface Revision = 0
 *          TransactionType    = Response
 *          resource           = /tr
 *          Status             = OK
 *          RequestId          = Value from the request
 *
 *       Payload:
 *          trObRxReqTS       - Time when the request was received
 *          trHandle          - New Handle
 *
 *    If the request was successful
 *       The Frodo will compare the trTargetMacAddress with its EUI address [64 bits]
 *
 *       If the address matches
 *          it will call PingRequest()
 *       else
 *          if will call MAC_PingRequest()
 *
 *
 **********************************************************************************************************************
 */
/*lint -esym(818,TraceRoute_MsgHandler, appMsgRxInfo) could be ponter to const   */
void TraceRoute_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   (void)length;
   buffer_t             *pBuf;          // pointer to message

   // Allocate a buffer to accommodate worst case payload.
   pBuf = BM_alloc( MAX_TR_RESOURCE_SIZE );
   if ( pBuf != NULL )
   {

      HEEP_APPHDR_t        appMsgTxInfo;  // Application header/QOS info

      //Application header/QOS info
      HEEP_initHeader( &appMsgTxInfo );

      /* Use the security mode from the request */
      appMsgTxInfo.appSecurityAuthMode = appMsgRxInfo->appSecurityAuthMode;

      appMsgTxInfo.TransactionType     = (uint8_t)TT_RESPONSE;
      appMsgTxInfo.Resource            = (uint8_t)tr;
      appMsgTxInfo.Req_Resp_ID         = appMsgRxInfo->Req_Resp_ID;
      appMsgTxInfo.qos                 = appMsgRxInfo->qos;
      appMsgTxInfo.Method_Status       = (uint8_t)OK;
      appMsgTxInfo.TransactionContext  = appMsgRxInfo->TransactionContext;
      HE_Context                       = appMsgRxInfo->TransactionContext;
      // Get the current time
      TIMESTAMP_t RxTime;
      (void) TIME_UTIL_GetTimeInSecondsFormat(&RxTime);

      /*
       * When the comEvalPingCountReset and comEvalRespAddrMode are unspecified in
       * the request, they default to the values indicated in their definition.
       */
      struct{
         uint8_t    PingCountReset;  // trPingCountReset
         uint8_t    RspAddrMode;     // trRspAddrMode
         eui_addr_t TargetAddress;   // trTargetMacAddress
      }params =
      {
         .PingCountReset = 0,                // Default to FALSE
         .RspAddrMode    = 0,                // Default to BROADCAST
         .TargetAddress  = {0,0,0,0,0,0,0,0} // trTargetMacAddress
      };


      struct readings_s readings;
      uint16_t Handle = 0;

      // Initialize the CompactMeterRead Header
      HEEP_ReadingsInit(&readings, 1, pBuf->data);

      // the /tr method must be a post
      if ( (enum_MessageMethod)appMsgRxInfo->Method != method_post )
      {
         /* unexpected request method field */
         appMsgTxInfo.Method_Status = (uint8_t) BadRequest;
         ERR_printf("unexpected method field to /tr (%d), expected (%d)", method_put, appMsgRxInfo->Method);

      }else
      {

         EventRdgQty_u  qtys;          // For EventTypeQty and ReadingTypeQty

         uint16_t       bits  = 0;       // Keeps track of the bit position within the Bitfields
         uint16_t       bytes = 0;       // Keeps track of byte position within Bitfields

         qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
         bytes           += EVENT_AND_READING_QTY_SIZE;


         bool bValidAddress = false;

         if (qtys.bits.eventQty != 0)
         {
            appMsgTxInfo.Status = (uint8_t)BadRequest;
            ERR_printf("EDEventQty expected to be zero in /tr handler");

         }else
         {
            uint8_t *pPayload = (uint8_t*)payloadBuf;

            // for each reading qty
            for (int i=0; i < qtys.bits.rdgQty; i++)
            {
               meterReadingType           readingType;   // Reading Type
               ExWId_ReadingInfo_u        readingInfo;   // Reading Info for the reading Type and Value

               // Copy the 3 bytes of reading info
               (void)memcpy(&readingInfo.bytes[0], &((uint8_t *)payloadBuf)[bytes], sizeof(readingInfo.bytes));
               bits       += READING_INFO_SIZE_BITS;
               bytes      += READING_INFO_SIZE;

               // Get the Reading Type ( 2 bytes )
               readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);
               bytes      += READING_TYPE_SIZE;


               // Get the Time Stamp if Present
               if (readingInfo.bits.tsPresent)
               {
                  bits  += READING_TIME_STAMP_SIZE_BITS;   //Skip over since we do not use/need it
                  bytes += READING_TIME_STAMP_SIZE;
               }

               // Get the Reading Quantity if Present
               if (readingInfo.bits.RdgQualPres)
               {
                  bits  += READING_QUALITY_SIZE_BITS; //Skip over since we do not use/need it
                  bytes += READING_QUALITY_SIZE;
               }

               /*lint -e{732} bit struct to uint16 */
               uint16_t                   rdgSize;       // For the reading value size
               rdgSize = readingInfo.bits.RdgValSizeU;
               rdgSize = ((rdgSize << RDG_VAL_SIZE_U_SHIFT) & RDG_VAL_SIZE_U_MASK) | readingInfo.bits.RdgValSize;

               switch (readingType) /*lint !e788 not all enums used within switch */
               {
                  case trTargetMacAddress:
                     // Field must be uintValue, <= 8 bytes
                     if((readingInfo.bits.typecast == (uint8_t) uintValue) && (rdgSize <= sizeof(eui_addr_t)))
                     {
                        (void) memcpy(&params.TargetAddress[8-rdgSize], &pPayload[bytes], rdgSize);
                        INFO_printHex("trTargetMacAddress:", params.TargetAddress,  sizeof(params.TargetAddress));
                        bValidAddress = true;
                        bits += (uint16_t) rdgSize*8; /*lint !e734 */
                     }else
                     {
                       appMsgTxInfo.Status = (uint8_t)BadRequest;
                       bits += (uint16_t) rdgSize*8; /*lint !e734 */
                     }
                     break;

                  case trPingCountReset:
                     // Field must be Boolean, 1 byte
                     if((readingInfo.bits.typecast == (uint8_t) Boolean) && (rdgSize == sizeof(uint8_t)))
                     {
                        params.PingCountReset = UNPACK_bits2_uint8(pPayload, &bits, (uint8_t)(rdgSize * 8));
                        INFO_printf("trPingCountReset:%d",params.PingCountReset);
                     }else
                     {
                       appMsgTxInfo.Status = (uint8_t)BadRequest;
                       bits += (uint16_t) rdgSize*8; /*lint !e734 */
                     }
                     break;

                  case trRspAddrMode:
                     // Field must be Boolean, 1 byte
                     if((readingInfo.bits.typecast == (uint8_t) Boolean) && (rdgSize == sizeof(uint8_t)))
                     {
                        params.RspAddrMode = UNPACK_bits2_uint8(pPayload, &bits, (uint8_t)(rdgSize * 8));
                        INFO_printf("trRspAddrMode:%d",params.RspAddrMode);
                     }
                     else{
                       appMsgTxInfo.Status = (uint8_t)BadRequest;
                       bits += (uint16_t) rdgSize*8; /*lint !e734 */
                     }
                     break;

                  default:
                     // Invalid reading type. Skip the value and continue.
                     ERR_printf("Not supported");
                     bits += (uint16_t) rdgSize*8; /*lint !e734 */
                     break;
               }  /*lint !e787 !e788 not all enums used with the switch  */

               bytes+=rdgSize;

               if (appMsgTxInfo.Status != (uint8_t)OK)
               {
                  break; // Abort loop to drop the message
               }
            }


            // Determine if we received a Target Address
            if (!bValidAddress)
            {
               /* If the trTargetMacAddress is unspecified, the synchronous response should
                * carry a status code of bad request.
                */
               appMsgTxInfo.Method_Status = (uint8_t) BadRequest;

            }else
            {
               /* Send a Synchronous Response with Status = OK. */
               TIMESTAMP_t TxTime;
               (void) TIME_UTIL_GetTimeInSecondsFormat(&TxTime);

               Handle = GetNextHandle();

               HEEP_Put_DateTimeValue( &readings, trObRxOfReqTS, &RxTime);
               HEEP_Put_U16(           &readings, trHandle   ,  Handle, 0);

               // Update the length field
               pBuf->x.dataLen = readings.size;
            }
         }
      }
      if (appMsgTxInfo.Method_Status == (uint8_t)OK)
      {
         if (HEEP_MSG_Tx(&appMsgTxInfo, pBuf) != eSUCCESS )
         {
            ERR_printf("HEEP_MSG_Tx returned an error in /tr handler");
         }else
         {
            // Now process the message

            // Determine if the address matched this device
            eui_addr_t eui_addr;
            MAC_EUI_Address_Get( (eui_addr_t *)eui_addr );

            if (!memcmp(params.TargetAddress, eui_addr, sizeof(eui_addr)))
            {
               /* The message is for this device */
               PingRequest( Handle, params.TargetAddress );
            }else
            {  /* The message is for another device, so send a ping request over the RF link */
               (void) MAC_PingRequest( Handle, params.TargetAddress, params.PingCountReset, params.RspAddrMode);
            }
         }
      }else
      {
         /* Free the buffer because there is no data */
         BM_free(pBuf);

         if (eSUCCESS != HEEP_MSG_TxHeepHdrOnly(&appMsgTxInfo))
         {
            ERR_printf("HEEP_MSG_TxHeepHdrOnly returned an error in /tr handler");
         }
      }
   }
   else
   {
      ERR_printf("/tr on-request handler unable to allocate memory");
   }

}
/*lint +esym(818,TraceRoute_MsgHandler, appMsgRxInfo) could be ponter to const   */


/*!
 **********************************************************************************************************************
 *
 * \fn PingRequest
 *
 * \brief This function called to handle an Ping Request for this device
 *
 * \details
 *
 *    It will send a BU message to the head end with the following:
 *
 *       Header
 *          Interface Revision = 0
 *          TransactionType    = TT_ASYNC
 *          resource           = /tr
 *          Method             = post
 *          RequestId          = 0
 *
 *       Payload:
 *         EDEventQty      = 0;
 *         Reading Quanity = 3;
 *
 *         trTargetMacAddress - The EUI address of this device
 *         trIbTxRspTS        - The Current Time
 *
 * \param   handle        - Handle associated with this request
 *          eui_addr      - TargetAddress
 *
 * \return  none
 *
 **********************************************************************************************************************
 */
static void PingRequest(
   uint16_t             handle,          /* 0-65535 */
   eui_addr_t const     eui_addr)        /* 64-bit extention address  */
{

   /* Send the Asynchronous Response */

   // Allocate a buffer to accommodate worst case payload.
   buffer_t *pBuf;

   pBuf = BM_alloc( MAX_TR_RESOURCE_SIZE );
   if ( pBuf != NULL )
   {
      HEEP_APPHDR_t  appMsgTxInfo;   // Application header/QOS info

      // Application header/QOS info
      HEEP_initHeader( &appMsgTxInfo );

      appMsgTxInfo.appSecurityAuthMode = 0;
      appMsgTxInfo.TransactionType     = (uint8_t)TT_ASYNC;
      appMsgTxInfo.Resource            = (uint8_t)tr;
      appMsgTxInfo.RspID               = 0;
      appMsgTxInfo.qos                 = 0;
      appMsgTxInfo.Method              = (uint8_t)method_post;


      // this will use ExchangeWithId format
      struct readings_s readings;
      HEEP_ReadingsInit(&readings, 1, pBuf->data);

      TIMESTAMP_t TxTime;
      (void) TIME_UTIL_GetTimeInSecondsFormat(&TxTime);

      // EUI address of the target device as uintValue
      uint64_t u64_addr;
      (void) memcpy( &u64_addr, eui_addr, sizeof(uint64_t));
      u64_addr = ntohll(u64_addr);
      HEEP_Put_U64( &readings, trTargetMacAddress, u64_addr);

      // Time when the response was transmitted as DateTimeValue
      HEEP_Put_DateTimeValue( &readings, trIbTxOfRspTS     , &TxTime);

      // Handle assigned to the request
      HEEP_Put_U16( &readings, trHandle   ,  handle, 0);
      pBuf->x.dataLen = readings.size;

      if (eSUCCESS != HEEP_MSG_Tx(&appMsgTxInfo, pBuf))
      {
         ERR_printf("HEEP_MSG_Tx returned an error in /tr handler");
      }
   }
}

/*!
 **********************************************************************************************************************
 *
 * \fn TraceRoute_IndicationHandler
 *
 * \brief This function is called to handle an Ping Indication from the MAC Layer
 *
 * \details  It will build a /tr (TRACE ROUTE) message and send it to the head end
 *
 *    The message will contain the following:
 *
 *       Header
 *          Interface Revision = 0
 *          TransactionType    = TT_ASYNC
 *          resource           = /tr
 *          Method             = post
 *          RequestId          = 0
 *
 *       Payload:
 *         EDEventQty          = 0;
 *         Reading Quantity    = 12;
 *
 *         trTargetMacAddress : the origin address (ep)
 *         trIbTxRspTS        : Current Time
 *         trRequestorMacAddress: requestor mac address (dcu)
 *         trPingCount        : the ping counter value
 *         trObTxReqTS        : the time when the request was transmitted by the dcu
 *         trTargetRxReqTS    : the time when the request was received by the ep
 *         trTargetTxRspTS    : the time when the response was transmitted the ep
 *         trIbRxRspTS        : the time when the response was transmitted the dcu
 *         trReqRSSI          : rssi value meausured by the target
 *         trReqChannel       : channel which target received the request
 *         trRspRSSI          : rssi value measured by the dcu
 *         trRspChannel       : channel which dcu received the response
 *         trHandle           : the handle assign by the originating dcu
 *
 * \param pPingIndication - Pointer to the ping indication from the MAC
 *
 * \return  none
 *
 **********************************************************************************************************************
 */
void TraceRoute_IndicationHandler(MAC_PingInd_t const *pPingIndication)
{
   buffer_t *pBuf;

   // Allocate a buffer to accommodate worst case payload.
   pBuf = BM_alloc( MAX_TR_RESOURCE_SIZE );

   if ( pBuf != NULL )
   {

      HEEP_APPHDR_t  appMsgTxInfo;   // Application header/QOS info

      // Application header/QOS info
      HEEP_initHeader( &appMsgTxInfo );

      appMsgTxInfo.appSecurityAuthMode = 0;
      appMsgTxInfo.TransactionType     = (uint8_t)TT_ASYNC;
      appMsgTxInfo.Resource            = (uint8_t)tr;
      appMsgTxInfo.RspID               = 0;
      appMsgTxInfo.qos                 = 0;
      appMsgTxInfo.Method              = (uint8_t)method_post;
      appMsgTxInfo.TransactionContext = HE_Context;

      struct readings_s readings;
      HEEP_ReadingsInit(&readings, 1, pBuf->data);

      TIMESTAMP_t TxTime;
      (void) TIME_UTIL_GetTimeInSecondsFormat(&TxTime);


      // Convert to the eui_addr to uint64_t so we can transmit as a uint
      uint64_t addr;
      (void)memcpy((uint8_t*) &addr, pPingIndication->src_eui, sizeof(addr));
      addr = ntohll(addr);
      HEEP_Put_U64(           &readings, trTargetMacAddress, addr);

      // Current Time
      HEEP_Put_DateTimeValue( &readings, trIbTxOfRspTS     , &TxTime                        );

      // Requestor ID
      ( void )memcpy((uint8_t*) &addr, pPingIndication->origin_eui, sizeof(addr));
      addr = ntohll(addr);
      HEEP_Put_U64(           &readings, trRequestorMacAddress , addr);

      // the ping counter value
      HEEP_Put_U16(           &readings, macPingCount       , pPingIndication->PingCount, 0     );

      // the time when the request was transmitted by the dcu
      HEEP_Put_DateTimeValue( &readings, trObTxOfReqTS     , &pPingIndication->ReqTxTime    );

      // the time when the request was received by the ep
      HEEP_Put_DateTimeValue( &readings, trTargetRxOfReqTS , &pPingIndication->ReqRxTime    );

      // the time when the response was transmitted the ep
      HEEP_Put_DateTimeValue( &readings, trTargetTxOfRspTS , &pPingIndication->RspTxTime    );

      // the time when the response was received by the dcu
      HEEP_Put_DateTimeValue( &readings, trIbRxOfRspTS     , &pPingIndication->RspRxTime    );

      // rssi value meausured by the target in units of 0.1dBm
      HEEP_Put_I16(           &readings, trReqRSSI         ,  pPingIndication->ReqRssi,   1 );

      // channel which target received the request
      HEEP_Put_U16(           &readings, trReqChannel      ,  pPingIndication->ReqChannel, 0);

      // rssi value measured by the dcu in units of 0.1dBm
      HEEP_Put_I16(           &readings, trRspRSSI         ,  pPingIndication->RspRssi,    1);

      // channel which dcu received the response
      HEEP_Put_U16(           &readings, trRspChannel      ,  pPingIndication->RspChannel, 0);

      // the handle assign by the originating dcu
      HEEP_Put_U16(           &readings, trHandle          ,  pPingIndication->Handle,     0) ;

      pBuf->x.dataLen = readings.size;

      if (eSUCCESS != HEEP_MSG_Tx(&appMsgTxInfo, pBuf))
      {
         ERR_printf("HEEP_MSG_Tx returned an error in /tr handler");
      }
      HE_Context = DEFAULT_HEAD_END_CONTEXT;
   }
}


/*!
 **********************************************************************************************************************
 *
 * \fn GetNextHandle()
 *
 * \brief This is used to get the next handle that is used by a trace route
 *
 * \param  void
 *
 * \return Handle_
 *
 * \details  These values are __no_init.   If the InitCode != 0xAA55 it is assumed this is a powerup, so the Handle
 *           is reset to 0, and the Initcode is set to 0xAA55.
 *
 **********************************************************************************************************************
 */
static uint16_t GetNextHandle()
{
   if (TraceRoute_.InitCode != 0xAA55)
   {
      TraceRoute_.InitCode = 0xAA55;
      TraceRoute_.Handle   = 0;
   }
   ++TraceRoute_.Handle;
   return TraceRoute_.Handle;
}

