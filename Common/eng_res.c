/******************************************************************************
 *
 * Filename: eng_res.c
 *
 * Global Designator:  eng_res
 *
 * Contents: This file contains functions that handle incoming bu_en resource requests and the generation of bubble up
 *   engineering messages.
 *
 ******************************************************************************
 * Copyright (c) 2018 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h> // rand()
#include <stdbool.h>
#include <string.h>
#if (DCU == 1)
#include "partitions.h"
#endif
#include "DBG_SerialDebug.h"

#include "time_util.h"
#include "pack.h"

#include "PHY.h"
#include "MAC_Protocol.h"
#include "MAC.h"
#include "STACK.h"
#include "STACK_Protocol.h"
#include "dtls.h"
#include "HEEP_util.h"
#if (EP == 1)
#include "pwr_task.h"
#endif
#if (USE_MTLS == 1) || (DCU == 1) // Remove DCU == 1 when MTLS is enabled for TB
#include "mtls.h"
#endif
#include "APP_MSG_Handler.h"
#include "eng_res.h"

#define DBG_ENG_RES 0 /* Set to non-zero to print buffer sizes allocated.   */

/* #DEFINE DEFINITIONS */

#if (EP == 1)
#define MAX_REST_SRC 16
#endif
#define PORT_BIT_CT 16
#define RADIO_BIT_CT 10
#define LAYER_BIT_CT 3
#define SWITCH_BIT_CT 1
#define SIZE_BIT_CT 2

/* the size of overhead fields for a readingtype (isCoincidentTrigger, PendingPowerOfTen, ReadingType fields, etc) */
#define READINGS_OVERHEAD_SIZE ((uint16_t)(READING_INFO_SIZE + READING_TYPE_SIZE))

/* MACRO DEFINITIONS */
#define FULL_REPORT (bool)true
#define TRUNCATED_REPORT (bool)false

/* TYPE DEFINITIONS */
typedef uint16_t ( *nF )(uint8_t **ppDst, uint32_t val);
/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
#if (EP == 1)
static uint16_t add_rssiLinkReading(uint8_t *address);
#endif
static uint16_t add_statsReading(bool full, uint8_t *address);

/***********************************************************************************************************************
Function Name: eng_res_MsgHandler

Purpose: handle a head end request for an on-demand bubble up of network statistics

Arguments: heepHdr - pointer to the HEEP protocol header
           payloadBuf = poitner to the HEEP protocol application payload (GetMeterReads structure)
           length - payload length

Returns: none
***********************************************************************************************************************/
//lint -efunc(818,eng_res_MsgHandler)    arguments can't be const due to generic API requirements
void eng_res_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   (void)length;
   HEEP_APPHDR_t appMsgTxInfo; // Application header/QOS info
   uint16_t bits = 0; // Keeps track of the bit position within the Bitfields
   uint8_t i; // loop index
   TimeSchRdgQty_t qtys; // For TimeScheduleQty and ReadingTypeQty
   meterReadingType readingType; // Reading Type
   buffer_t *pBuf; // pointer to message
   buffer_t *pPayload; // pointer to payload
   uint16_t maxSize; // Maximum buffer size
   uint16_t engData1Size; // Eng data 1 size
#if (EP == 1)
   uint16_t engData2Size; // Eng data 2 size
#endif

   //Application header/QOS info
   HEEP_initHeader(&appMsgTxInfo);
   appMsgTxInfo.appSecurityAuthMode = 0;
   appMsgTxInfo.TransactionType     = (uint8_t)TT_RESPONSE;
   appMsgTxInfo.Resource            = (uint8_t)bu_en;
   appMsgTxInfo.Req_Resp_ID         = appMsgRxInfo->Req_Resp_ID;
   appMsgTxInfo.qos                 = appMsgRxInfo->qos;
   appMsgTxInfo.Method_Status       = (uint8_t)OK;
   appMsgTxInfo.TransactionContext = appMsgRxInfo->TransactionContext;


   /* Find out what is the maximum buffer size needed */
   /* We cut corners here. We assume that engData1 and engData2 will be returned in one packet */
   /* So we reserve the largest possible buffer even if it is not necessary */
   maxSize      = VALUES_INTERVAL_END_SIZE + EVENT_AND_READING_QTY_SIZE; // HEEP header
   engData1Size = add_statsReading(FULL_REPORT, NULL); // Retrieve engData1 payload size
   maxSize     += READINGS_OVERHEAD_SIZE + engData1Size;
#if (EP == 1)
   engData2Size = add_rssiLinkReading(NULL); // Retrieve engData2 payload size
   maxSize     += READINGS_OVERHEAD_SIZE + engData2Size;
#endif
   pBuf = BM_alloc(min(maxSize, APP_MSG_MAX_DATA));

   if (pBuf != NULL)
   {
      struct readings_s readings;

      // Initialize the CompactMeterRead Header
      HEEP_ReadingsInit(&readings, 1, (uint8_t *)&pBuf->data[0]);

      if (method_get == (enum_MessageMethod)appMsgRxInfo->Method_Status)
      {
         //Parse the packet payload - an implied GetMeterReads bit structure
         qtys.TimeSchRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);

         if (0 == qtys.bits.timeSchQty)
         {
            /* create a response for every ReadingType requested */
            for (i = 0; i < qtys.bits.rdgQty; i++)
            {
               readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);

               switch (readingType) /*lint !e788 not all enums used within switch */
               {
                  case engData1:
                     pPayload = BM_alloc(engData1Size);

                     if (pPayload != NULL)
                     {
                        (void)add_statsReading(FULL_REPORT, (uint8_t *)&pPayload->data[0]);
                        HEEP_Put_HexBinary(&readings, engData1, &pPayload->data[0], engData1Size);
                        BM_free(pPayload);
                     }

                     break;

#if (EP == 1)
                  case engData2:
                     pPayload = BM_alloc(engData2Size);

                     if (pPayload != NULL)
                     {
                        (void)add_rssiLinkReading((uint8_t *)&pPayload->data[0]);
                        HEEP_Put_HexBinary(&readings, engData2, &pPayload->data[0], engData2Size);
                        BM_free(pPayload);
                     }

                     break;

#endif
                  default:
                     //Invalid reading type, drop the entire message
                     appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
                     break;
               } //lint !e788  Some enums in meterReadingType aren't used. //End of switch on reading type

               if ((uint8_t)OK != appMsgTxInfo.Method_Status)
               {
                  break; //Abort loop to drop the message
               }
            }
         }
         else
         {
            // Code is not yet ready to handle events.
            ERR_printf("TimeScheduleQty expected to be zero in /bu/en handler");
            appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
         }
      }
      else
      {
         /* unexpected request method field */
         ERR_printf("unexpected method field to /bu/en: (%d), expected (%d)", (int32_t)method_get, appMsgRxInfo->Method_Status);
         appMsgTxInfo.Method_Status   = (uint8_t)NotModified;
      }

      if (appMsgTxInfo.Method_Status == (uint8_t)OK)
      {
         /* Update payload size and send */
         pBuf->x.dataLen = readings.size;

         /* pBuf is freed by this call no matter what */
         if (eSUCCESS != HEEP_MSG_Tx(&appMsgTxInfo, pBuf))
         {
            ERR_printf("HEEP_MSG_Tx returned an error in /bu/en handler");
         }
      }
      else
      {
         BM_free(pBuf);

         if (eSUCCESS != HEEP_MSG_TxHeepHdrOnly(&appMsgTxInfo))
         {
            ERR_printf("HEEP_MSG_TxHeepHdrOnly returned an error in /bu/en handler");
         }
      }
   }
   else
   {
      ERR_printf("/bu/en on-request handler unable to allocate memory");
   }
}

/*
 * Copy a u8 value to network byte order
 */
static uint16_t u8_to_network(uint8_t **ppDst, uint8_t val)
{
   uint8_t *p = *ppDst;

   if (p)
   {
      *p++ = (uint8_t)(val);
      *ppDst = p;
   }

   return sizeof(uint8_t);
}

/*
 * Copy a u16 value to network byte order
 */
static uint16_t u16_to_network(uint8_t **ppDst, uint32_t val)
{
   uint8_t *p = *ppDst;

   if (p)
   {
      *p++ = (uint8_t)(val >>  8);
      *p++ = (uint8_t)(val);
      *ppDst = p;
   }

   return sizeof(uint16_t);
}

/*
 * Copy a u32 value to network byte order
 */
static uint16_t u32_to_network(uint8_t **ppDst, uint32_t val)
{
   uint8_t *p = *ppDst;

   if (p)
   {
      *p++ = (uint8_t)(val >> 24);
      *p++ = (uint8_t)(val >> 16);
      *p++ = (uint8_t)(val >>  8);
      *p++ = (uint8_t)(val);
      *ppDst = p;
   }

   return sizeof(uint32_t);
}

#if (EP == 1)
// Outbound Link Payload
static uint16_t add_rssiLinkReading(uint8_t *address)
{
   uint32_t i;
   uint8_t *pDst  = address;
   uint16_t bitNo = 0;
   uint8_t version = 1;

   // Add the version to message
   bitNo = PACK_uint8_2bits(&version, 8, pDst, bitNo);

   // Add the N devices to the message, 152 bits per record
   for (i = 0; i < MAC_MAX_DEVICES; i++)
   {
      // extension id        40 bits
      // First Heard Time    32 bits
      // Last  Heard Time    32 bits
      // extension id      4*12 bits

      mac_stats_t *pRecord = MAC_STATS_GetRecord(i);

      //   memcpy(pDst, pRecord->xid, 5);
      bitNo = PACK_uint8_2bits(&pRecord->xid[0], 8, pDst, bitNo);
      bitNo = PACK_uint8_2bits(&pRecord->xid[1], 8, pDst, bitNo);
      bitNo = PACK_uint8_2bits(&pRecord->xid[2], 8, pDst, bitNo);
      bitNo = PACK_uint8_2bits(&pRecord->xid[3], 8, pDst, bitNo);
      bitNo = PACK_uint8_2bits(&pRecord->xid[4], 8, pDst, bitNo);

      bitNo = PACK_uint32_2bits(&pRecord->FirstHeardTime, 32, pDst, bitNo);
      bitNo = PACK_uint32_2bits(&pRecord->LastHeardTime, 32, pDst, bitNo);

      // Send these in order from oldest to newest
      uint8_t j;

      for (j = 0; j < MAC_RSSI_HISTORY_SIZE; j++)
      {
         uint16_t rssi = PHY_DBM_TO_MAC_SCALING(pRecord->rssi_dbm[(pRecord->RxCount + j) % MAC_RSSI_HISTORY_SIZE]);
         bitNo = PACK_uint16_2bits(&rssi, 12, pDst, bitNo);
      }
   }

   PHY_SyncHistory_t SyncHistory; // Location to save the sync history

   // This will make a copy of the sync history
   PHY_SyncHistory(&SyncHistory);

   for (i = 0; i < PHY_SYNC_HISTORY_SIZE; i++)
   {
      uint16_t rssi = PHY_DBM_TO_MAC_SCALING(SyncHistory.Rssi[(SyncHistory.Index + i) % PHY_SYNC_HISTORY_SIZE]);
      bitNo = PACK_uint16_2bits(&rssi, 12, pDst, bitNo);
   }

   return (bitNo + 7) / 8; // return size in bytes
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn ENG_RES_statsReading
 *
 * \brief Used to get the stats reading data (engData1)
 *
 * \param  readingType
 * \param  dst
 *
 * \return uint16_t - Number of bytes returned in the buffer
 *
 ********************************************************************************************************************
 */
uint16_t ENG_RES_statsReading(bool readingType, uint8_t *dst)
{
   return add_statsReading(readingType, dst);
}

#if (EP == 1)
/*! ********************************************************************************************************************
 *
 * \fn ENG_RES_rssiLinkReading
 *
 * \brief  Used to get the rssi data (engData2)
 *
 * \param  dst
 *
 * \return uint16_t - Number of bytes returned in the buffer
 *
 ********************************************************************************************************************
 */
uint16_t ENG_RES_rssiLinkReading(uint8_t *dst)
{
   return add_rssiLinkReading(dst);
}
#endif

/*! ********************************************************************************************************************
 *
 * \fn add_statsReading
 *
 * \brief  Used to build the statictics payload for engData1. Function will not build payload if "address" is NULL but will still return the buffer size.
 *
 * \param  full - Full stats if true, half stats of false.
 *         address - destination buffer.
 *
 * \return uint16_t - Number of bytes returned in the buffer
 *
 ********************************************************************************************************************
 */
static uint16_t add_statsReading(bool full, uint8_t *address)
{
   //Prepare Full Meter Read structure
   nF u_to_n; /* pointer to uxx to network handler  */
   uint8_t *pDst = address;
   uint16_t i;
   uint16_t used = 0;
   uint8_t version;

   if (full)
   {
      version = 5; // non-truncated report uses version 5
      u_to_n = u32_to_network; // 32 bit counters reported without truncation
   }
   else
   {
      version = 6;
      u_to_n = u16_to_network; // 32 bit counters reported with truncation
   }

   // Set the version
   used += u8_to_network(&pDst, version);

   // Transport Layer Port Count
   if (pDst != NULL)
   {
      for (i = 0; i < PORT_BIT_CT; i++)
      {
         uint8_t portUsed = (i < (uint16_t)IP_PORT_COUNT) ? 1 : 0;

         (void)PACK_uint8_2bits(&portUsed, 1, pDst, i);
      }

      pDst += PORT_BIT_CT / 8;
   }

   used += PORT_BIT_CT / 8;

   // Physical Layer Radio Count
   if (pDst != NULL)
   {
      for (i = 0; i < RADIO_BIT_CT; i++)
      {
         uint8_t radioUsed = (i < PHY_RCVR_COUNT) ? 1 : 0;

         (void)PACK_uint8_2bits(&radioUsed, 1, pDst, i);
      }

      // Data Link Layer Count
      uint8_t layerCt = 1; /// @todo should get this dynamically later
      (void)PACK_uint8_2bits(&layerCt, LAYER_BIT_CT, pDst, i);
      i += LAYER_BIT_CT;

      /// Device Switch
#if (EP == 1)
      uint8_t afc = 1;
#else
      uint8_t afc = 0;
#endif

      (void)PACK_uint8_2bits(&afc, SWITCH_BIT_CT, pDst, i);
      i += SWITCH_BIT_CT;

      /// Size Adjustement
      uint8_t size = full ? 0 : 1;
      (void)PACK_uint8_2bits(&size, SIZE_BIT_CT, pDst, i);

      pDst += (RADIO_BIT_CT + LAYER_BIT_CT + SWITCH_BIT_CT + SIZE_BIT_CT) / 8;
   }

   used += (RADIO_BIT_CT + LAYER_BIT_CT + SWITCH_BIT_CT + SIZE_BIT_CT) / 8;

   {
      // EP=42 DCU=265
      // Get the PHY Stats and put them into the buffer
      PHY_GetConf_t GetConf;

#if (EP == 1)
      GetConf = PHY_GetRequest(ePhyAttr_AfcAdjustment);
      used += u16_to_network(&pDst, (uint16_t)GetConf.val.AfcAdjustment[0]);
#endif

      if (full)
      {
         GetConf = PHY_GetRequest(ePhyAttr_RxAbortCount);
         for (i = 0; i < PHY_RCVR_COUNT; i++)
         {
            used += u_to_n(&pDst, GetConf.val.RxAbortCount[i]);
         }
      }

      GetConf = PHY_GetRequest(ePhyAttr_PreambleDetectCount);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.PreambleDetectCount[i]);
      }

      GetConf = PHY_GetRequest(ePhyAttr_RadioWatchdogCount);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.RadioWatchdogCount[i]);
      }

      GetConf = PHY_GetRequest(ePhyAttr_ThermalProtectionCount);
      used += u_to_n(&pDst, GetConf.val.ThermalProtectionCount);

      GetConf = PHY_GetRequest(ePhyAttr_DataRequestCount);
      used += u_to_n(&pDst, GetConf.val.DataRequestCount);

      GetConf = PHY_GetRequest(ePhyAttr_DataRequestRejectedCount);
      used += u_to_n(&pDst, GetConf.val.DataRequestRejectedCount);

      if (full)
      {
         GetConf = PHY_GetRequest(ePhyAttr_TxAbortCount);
         used += u_to_n(&pDst, GetConf.val.TxAbortCount);

         GetConf = PHY_GetRequest(ePhyAttr_FailedTransmitCount);
         used += u_to_n(&pDst, GetConf.val.FailedTransmitCount);

         GetConf = PHY_GetRequest(ePhyAttr_FailedFrameDecodeCount);
         for (i = 0; i < PHY_RCVR_COUNT; i++)
         {
            used += u_to_n(&pDst, GetConf.val.FailedFrameDecodeCount[i]);
         }

         GetConf = PHY_GetRequest(ePhyAttr_FailedHcsCount);
         for (i = 0; i < PHY_RCVR_COUNT; i++)
         {
            used += u_to_n(&pDst, GetConf.val.FailedHcsCount[i]);
         }
      }

      GetConf = PHY_GetRequest(ePhyAttr_FramesReceivedCount);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.FramesReceivedCount[i]);
      }

      GetConf = PHY_GetRequest(ePhyAttr_FramesTransmittedCount);
      used += u_to_n(&pDst, GetConf.val.FramesTransmittedCount);

      if (full)
      {
         GetConf = PHY_GetRequest(ePhyAttr_FailedHeaderDecodeCount);
         for (i = 0; i < PHY_RCVR_COUNT; i++)
         {
            used += u_to_n(&pDst, GetConf.val.FailedHeaderDecodeCount[i]);
         }
      }

      GetConf = PHY_GetRequest(ePhyAttr_LastResetTime);
      used += u32_to_network(&pDst, GetConf.val.LastResetTime.seconds);
      used += u32_to_network(&pDst, GetConf.val.LastResetTime.QFrac);

      if (full)
      {
         GetConf = PHY_GetRequest(ePhyAttr_MalformedHeaderCount);
         for (i = 0; i < PHY_RCVR_COUNT; i++)
         {
            used += u_to_n(&pDst, GetConf.val.MalformedHeaderCount[i]);
         }

         GetConf = PHY_GetRequest(ePhyAttr_RssiJumpCount);
         for (i = 0; i < PHY_RCVR_COUNT; i++)
         {
            used += u_to_n(&pDst, GetConf.val.RssiJumpCount[i]);
         }
      }

      GetConf = PHY_GetRequest(ePhyAttr_SyncDetectCount);
      for (i = 0; i < PHY_RCVR_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.SyncDetectCount[i]);
      }

#if (DBG_ENG_RES != 0)
      DBG_logPrintf('I', "After Phy, used = %u", used);
#endif
   }

   {
      // Get the MAC Stats and put them into the buffer
      MAC_GetConf_t GetConf;

      // EP=80 DCU=80
      GetConf = MAC_GetRequest(eMacAttr_AcceptedFrameCount);
      used += u_to_n(&pDst, GetConf.val.AcceptedFrameCount);

      if (full)
      {
         GetConf = MAC_GetRequest(eMacAttr_ArqOverflowCount);
         used += u_to_n(&pDst, GetConf.val.ArqOverflowCount);
      }

      GetConf = MAC_GetRequest(eMacAttr_ChannelAccessFailureCount);
      used += u_to_n(&pDst, GetConf.val.ChannelAccessFailureCount);

      if (full)
      {
         GetConf = MAC_GetRequest(eMacAttr_DuplicateFrameCount);
         used += u_to_n(&pDst, GetConf.val.DuplicateFrameCount);

         GetConf = MAC_GetRequest(eMacAttr_FailedFcsCount);
         used += u_to_n(&pDst, GetConf.val.FailedFcsCount);

         GetConf = MAC_GetRequest(eMacAttr_InvalidRecipientCount);
         used += u_to_n(&pDst, GetConf.val.InvalidRecipientCount);

         GetConf = MAC_GetRequest(eMacAttr_MalformedAckCount);
         used += u_to_n(&pDst, GetConf.val.MalformedAckCount);

         GetConf = MAC_GetRequest(eMacAttr_MalformedFrameCount);
         used += u_to_n(&pDst, GetConf.val.MalformedFrameCount);
      }

      GetConf = MAC_GetRequest(eMacAttr_PacketConsumedCount);
      used += u_to_n(&pDst, GetConf.val.PacketConsumedCount);

      if (full)
      {
         GetConf = MAC_GetRequest(eMacAttr_PacketFailedCount);
         used += u_to_n(&pDst, GetConf.val.PacketFailedCount);
      }

      GetConf = MAC_GetRequest(eMacAttr_PacketReceivedCount);
      used += u_to_n(&pDst, GetConf.val.PacketReceivedCount);

      if (full)
      {
         GetConf = MAC_GetRequest(eMacAttr_LastResetTime);
         used += u32_to_network(&pDst, GetConf.val.LastResetTime.seconds);
         used += u32_to_network(&pDst, GetConf.val.LastResetTime.QFrac);

         GetConf = MAC_GetRequest(eMacAttr_AdjacentNwkFrameCount);
         used += u_to_n(&pDst, GetConf.val.AdjacentNwkFrameCount);

         GetConf = MAC_GetRequest(eMacAttr_RxFrameCount);
         used += u_to_n(&pDst, GetConf.val.RxFrameCount);
      }

      GetConf = MAC_GetRequest(eMacAttr_RxOverflowCount);
      used += u_to_n(&pDst, GetConf.val.RxOverflowCount);

      /// @todo convert to macSelfDetectCount (Doesn't exist atm)
      //GetConf = MAC_GetRequest(eMacAttr_MacSelfDetectCount);
      //used += u_to_n(&pDst, GetConf.val.MacSelfDetectCount);
      used += u_to_n(&pDst, 0);

      GetConf = MAC_GetRequest(eMacAttr_TransactionTimeoutCount);
      used += u_to_n(&pDst, GetConf.val.TransactionTimeoutCount);

      GetConf = MAC_GetRequest(eMacAttr_TxLinkDelayCount);
      used += u_to_n(&pDst, GetConf.val.TxLinkDelayCount);

      GetConf = MAC_GetRequest(eMacAttr_TxLinkDelayTime);
      used += u_to_n(&pDst, GetConf.val.TxLinkDelayTime);

      GetConf = MAC_GetRequest(eMacAttr_TransactionOverflowCount);
      used += u_to_n(&pDst, GetConf.val.TransactionOverflowCount);

      GetConf = MAC_GetRequest(eMacAttr_TxFrameCount);
      used += u_to_n(&pDst, GetConf.val.TxFrameCount);

      GetConf = MAC_GetRequest(eMacAttr_TxPacketCount);
      used += u_to_n(&pDst, GetConf.val.TxPacketCount);

      GetConf = MAC_GetRequest(eMacAttr_TxPacketFailedCount);
      used += u_to_n(&pDst, GetConf.val.TxPacketFailedCount);

#if (DBG_ENG_RES != 0)
      DBG_logPrintf('I', "After MAC, used = %u", used);
#endif
   }

   {
      // Get the NWK Stats and put them into the buffer
      NWK_GetConf_t GetConf;

      // EP= DCU=120
      GetConf = NWK_GetRequest(eNwkAttr_ipIfInDiscards);
      used += u_to_n(&pDst, GetConf.val.ipIfInDiscards);

      GetConf = NWK_GetRequest(eNwkAttr_ipIfInErrors);
      for (i = 0; i < IP_LOWER_LINK_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfInErrors[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfInMulticastPkts);
      for (i = 0; i < (uint32_t)IP_PORT_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfInMulticastPkts[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfInOctets);
      for (i = 0; i < IP_LOWER_LINK_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfInOctets[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfInUcastPkts);
      for (i = 0; i < (uint32_t)IP_PORT_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfInUcastPkts[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfInUnknownProtos);
      for (i = 0; i < IP_LOWER_LINK_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfInUnknownProtos[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfInPackets);
      for (i = 0; i < IP_LOWER_LINK_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfInPackets[i]);
      }

      if (full)
      {
         GetConf = NWK_GetRequest(eNwkAttr_ipIfLastResetTime);
         used += u32_to_network(&pDst, GetConf.val.ipIfLastResetTime.seconds);
         used += u32_to_network(&pDst, GetConf.val.ipIfLastResetTime.QFrac);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfOutDiscards);
      used += u_to_n(&pDst, GetConf.val.ipIfOutDiscards);

      GetConf = NWK_GetRequest(eNwkAttr_ipIfOutErrors);
      for (i = 0; i < IP_LOWER_LINK_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfOutErrors[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfOutMulticastPkts);
      for (i = 0; i < (uint32_t)IP_PORT_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfOutMulticastPkts[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfOutOctets);
      for (i = 0; i < IP_LOWER_LINK_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfOutOctets[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfOutUcastPkts);
      for (i = 0; i < (uint32_t)IP_PORT_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfOutUcastPkts[i]);
      }

      GetConf = NWK_GetRequest(eNwkAttr_ipIfOutPackets);
      for (i = 0; i < IP_LOWER_LINK_COUNT; i++)
      {
         used += u_to_n(&pDst, GetConf.val.ipIfOutPackets[i]);
      }

#if (DBG_ENG_RES != 0)
      DBG_logPrintf('I', "After NWK, used = %u", used);
#endif
   }

#if (USE_DTLS == 1)
   {
      // Get a snapshot of the dtls statistics
      DTLS_counters_t dtls_counters;

      DTLS_GetCounters(&dtls_counters);

      // Copy the values to the message buffer
      // EP=32 DCU=32
      used += u_to_n(&pDst, dtls_counters.dtlsIfInUcastPkts);
      used += u_to_n(&pDst, dtls_counters.dtlsIfInSecurityErrors);
      used += u_to_n(&pDst, dtls_counters.dtlsIfInNoSessionErrors);
      used += u_to_n(&pDst, dtls_counters.dtlsIfInDuplicates);
      used += u_to_n(&pDst, dtls_counters.dtlsIfInNonApplicationPkts);
      used += u_to_n(&pDst, dtls_counters.dtlsIfOutUcastPkts);
      used += u_to_n(&pDst, dtls_counters.dtlsIfOutNonApplicationPkts);
      used += u_to_n(&pDst, dtls_counters.dtlsIfOutNoSessionErrors);
   }
#endif

   {
      // These are dummy app statistics
      // EP=12 DCU=12
      uint32_t appIfInSecurityErrors[(uint32_t)IP_PORT_COUNT] = { 0 };
      for (i = 0; i < (uint32_t)IP_PORT_COUNT; i++)
      {
         used += u_to_n(&pDst, appIfInSecurityErrors[0]);
      }

#if (DBG_ENG_RES != 0)
      DBG_logPrintf('I', "After DTLS, used = %u", used);
#endif
   }

#if (EP == 1)
   {
      uint32_t tib; // Bit mask used to loop through all possible reset causes
      uint16_t const *counter; // Pointer to individual counters from power fail file
      const pwrFileData_t *pwrFileData; // Pointer to power fail file

      pwrFileData = PWR_getpwrFileData();
      counter = (uint16_t const *)&pwrFileData->uPowerDownCount;

      static uint16_t lastReading[MAX_REST_SRC];

      if (full)
      {
         for (i = 0, tib = RESET_SOURCE_POWER_ON_RESET; tib <= RESET_ANOMALY_COUNT; tib <<= 1, i++)
         {
            lastReading[i] = *counter;
            used += u16_to_network(&pDst, *counter);
            counter++;
         }
      }
      else
      {
         bool readingChanged[MAX_REST_SRC];

         /// convert to bool for trimmed message
         for (i = 0, tib = RESET_SOURCE_POWER_ON_RESET; tib <= RESET_ANOMALY_COUNT; tib <<= 1, i++)
         {
            readingChanged[i] = (lastReading[i] != *counter);
            lastReading[i] = *counter;
            counter++;
         }

         while (i < MAX_REST_SRC)
         {
            readingChanged[i] = false;
            i++;
         }

         // store values
         if (pDst != NULL)
         {
            for (i = 0; i < MAX_REST_SRC; i++)
            {
               (void)PACK_uint8_2bits((uint8_t *)&readingChanged[i], 1, pDst, i);
            }

            pDst += MAX_REST_SRC / 8;
         }

         used += MAX_REST_SRC / 8;
      }

      // EP=2 DCU=2
      uint16_t temp = PWR_getResetCause();
      used += u16_to_network(&pDst, temp);

#if (DBG_ENG_RES != 0)
      DBG_logPrintf('I', "After Reset Counters, used = %u", used);
#endif
   }
#endif

   {
      bufferStats_t bufferStats;

      BM_getStats(&bufferStats);
      used += u_to_n(&pDst, bufferStats.allocFailApp);
      used += u_to_n(&pDst, bufferStats.allocFailDebug);
      used += u_to_n(&pDst, bufferStats.allocFailStack);

      used += u_to_n(&pDst, bufferStats.freeFail);
      used += u_to_n(&pDst, bufferStats.freeDoubleFree);

#if (DBG_ENG_RES != 0)
      DBG_logPrintf('I', "After Buffers, used = %u", used);
#endif
   }

#if (USE_MTLS == 1) || (DCU == 1) // Remove DCU == 1 when MTLS is enabled for TB
   {
      // EP=52 DCU=52
#if (DCU == 1) // Remove DCU == 1 when MTLS is enabled for TB
      // We are fakeing MTLS stats for now
      mtlsReadOnlyAttributes mtlsAttributes = { 0 };
#else
      mtlsReadOnlyAttributes mtlsAttributes = *getMTLSstats();
#endif

      used += u_to_n(&pDst, mtlsAttributes.mtlsDuplicateMessageCount);
      used += u_to_n(&pDst, mtlsAttributes.mtlsFailedSignatureCount);
      used += u_to_n(&pDst, mtlsAttributes.mtlsFirstSignedTimestamp);
      used += u_to_n(&pDst, mtlsAttributes.mtlsInputMessageCount);
      used += u_to_n(&pDst, mtlsAttributes.mtlsInvalidKeyIdCount);
      used += u_to_n(&pDst, mtlsAttributes.mtlsInvalidTimeCount);
      used += u32_to_network(&pDst, mtlsAttributes.mtlsLastResetTime.seconds);
      used += u32_to_network(&pDst, mtlsAttributes.mtlsLastResetTime.QFrac);
      used += u32_to_network(&pDst, mtlsAttributes.mtlsLastSignedTimestamp);
      used += u_to_n(&pDst, mtlsAttributes.mtlsMalformedHeaderCount);
      used += u_to_n(&pDst, mtlsAttributes.mtlsNoSesstionCount);
      used += u_to_n(&pDst, mtlsAttributes.mtlsOutputMessageCount);
      used += u_to_n(&pDst, mtlsAttributes.mtlsTimeOutOfBoundsCount);

#if (DBG_ENG_RES != 0)
      DBG_logPrintf('I', "After MTLS, used = %u", used);
      OS_TASK_Sleep(200);
#endif
   }
#endif

#if (DCU == 1)
   /* This is a good time to flush all statistics on the T-board. */
   /* This is because if we lose power, the statistics are not flushed and since they are not flushed, the counters could appeared to have rooled backward next time they are read. */
   /* Flush all partitions (only cached actually flushed) and ensures all pending writes are completed */
   (void)PAR_partitionFptr.parFlush(NULL);
#endif

   return used;
}

#if (EP == 1)
// Outbound Link Payload
buffer_t *eng_res_Build_OB_LinkPayload(void)
{
   buffer_t *pBuf; // pointer to message
   buffer_t *pPayload; // pointer to payload
   uint16_t maxSize; // Maximum buffer size
   uint16_t engData2Size; // Eng data 2 size
   struct   readings_s readings;

   // Find out what is the maximum buffer size needed
   maxSize      = VALUES_INTERVAL_END_SIZE + EVENT_AND_READING_QTY_SIZE; // HEEP header
   engData2Size = add_rssiLinkReading(NULL); // Retrieve engData2 payload size
   maxSize     += READINGS_OVERHEAD_SIZE + engData2Size;

   // Allocate a buffer big enough to accommodate payload.
   pBuf = BM_alloc(maxSize);

   if (pBuf != NULL)
   {
      // Initialize the CompactMeterRead Header
      HEEP_ReadingsInit(&readings, 1, (uint8_t *)&pBuf->data[0]);

      pPayload = BM_alloc(engData2Size);

      if (pPayload != NULL)
      {
         (void)add_rssiLinkReading((uint8_t *)&pPayload->data[0]);
         HEEP_Put_HexBinary(&readings, engData2, &pPayload->data[0], engData2Size);
         BM_free(pPayload);

         /* Update payload size */
         pBuf->x.dataLen = readings.size;
      }
      else
      {
         // Can't allocate second buffer
         BM_free(pBuf);
         pBuf = NULL;
      }
   }

   return pBuf;
}
#endif

/*
 * Build Statistics Payload
 */
buffer_t *eng_res_BuildStatsPayload(void)
{
   buffer_t *pBuf; // pointer to message
   buffer_t *pPayload; // pointer to payload
   uint16_t maxSize; // Maximum buffer size
   uint16_t engData1Size; // Eng data 1 size
   struct   readings_s readings;

   // Find out what is the maximum buffer size needed
   maxSize      = VALUES_INTERVAL_END_SIZE + EVENT_AND_READING_QTY_SIZE; // HEEP header
   engData1Size = add_statsReading(TRUNCATED_REPORT, NULL); // Retrieve engData1 payload size
   maxSize     += READINGS_OVERHEAD_SIZE + engData1Size;

   // Allocate a buffer big enough to accommodate payload.
   pBuf = BM_alloc(maxSize);

   if (pBuf != NULL)
   {
      // Initialize the CompactMeterRead Header
      HEEP_ReadingsInit(&readings, 1, (uint8_t *)&pBuf->data[0]);

      pPayload = BM_alloc(engData1Size);

      if (pPayload != NULL)
      {
         (void)add_statsReading(TRUNCATED_REPORT, (uint8_t *)&pPayload->data[0]);
         HEEP_Put_HexBinary(&readings, engData1, &pPayload->data[0], engData1Size);
         BM_free(pPayload);

         /* Update payload size */
         pBuf->x.dataLen = readings.size;
      }
      else
      {
         // Can't allocate second buffer
         BM_free(pBuf);
         pBuf = NULL;
      }
   }

   return pBuf;
}
