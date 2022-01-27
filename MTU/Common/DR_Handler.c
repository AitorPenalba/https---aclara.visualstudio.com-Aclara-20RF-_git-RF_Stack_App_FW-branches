/***********************************************************************************************************************
 *
 * Filename: DR_Handler.c
 *
 * Global Designator: DR_
 *
 * Contents:  Handles Demand Reset (/dr resource)
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************
 *
 * $Log$ cmusial Created May 1, 2015
 *
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"

//lint -esym(750,DR_GLOBALS)
#define DR_GLOBALS
#include "DR_Handler.h"
#undef DR_GLOBALS

#include "time_util.h"
#include "pack.h"
#include "byteswap.h"
#include "DBG_SerialDebug.h"
#include "intf_cim_cmd.h"

/* MACRO DEFINITIONS */
#define DR_MAX_READINGS 2
#define EXCH_WITH_ID_QTY_SIZE_BITS 4
#define EXCH_WITH_ID_EVENTTYPE_SIZE_BITS 4

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* GLOBAL VARIABLE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */

/* LOCAL FUNCTION PROTOTYPES */
static buffer_t *DR_PerformReset(void *payloadBuf, uint8_t *respCode);

/* FUNCTION DEFINITIONS */


/***********************************************************************************************************************
 *
 * Function Name: DR_MsgHandler
 *
 * Purpose: Handle DR (Demand Reset)
 *
 * Arguments:
 *     APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
 *     void *payloadBuf           - pointer to data payload
 *
 * Side Effects: none
 *
 * Re-entrant: No
 *
 * Returns: none
 *
 * Notes:
 *
 ***********************************************************************************************************************/
//lint -efunc(818, DR_MsgHandler) arguments cannot be const because of API requirements
void DR_MsgHandler(APP_MSG_Rx_t *appMsgRxInfo, void *payloadBuf)
{
   if ( method_post == (enum_MessageMethod)appMsgRxInfo->Method_Status)
   { //Process the post request
      uint8_t responseCode;
      buffer_t      *pBuf;            // pointer to message

      pBuf = DR_PerformReset(payloadBuf, &responseCode);
      if (pBuf != NULL)                            // Send the message to message handler.
      {
         APP_MSG_Tx_t  appMsgTxInfo;      // Application header/QOS info

         //Application header/QOS info
         appMsgTxInfo.TransactionType = (uint8_t)TT_RESPONSE;
         appMsgTxInfo.Resource = (uint8_t)dr;
         appMsgTxInfo.Method_Status = responseCode;
         appMsgTxInfo.Req_Resp_ID = (uint8_t)appMsgRxInfo->Req_Resp_ID;
         appMsgTxInfo.qos = (uint8_t)appMsgRxInfo->qos;
         appMsgTxInfo.callback = NULL;

         (void)APP_MSG_Tx (&appMsgTxInfo, pBuf);   // The called function will free the buffer
      }
      //else No buffer available, drop the message for now
   }
   //else Other methods not supported
}

/***********************************************************************************************************************
 *
 * Function Name: DR_PerformReset
 *
 * Purpose: Perform the actual demand reset functionality
 *
 * Arguments:
 *     void *payloadBuf - pointer to application data payload
 *     uint8_t respCode - response code for this request
 *
 * Side Effects: none
 *
 * Re-entrant: No
 *
 * Returns: buffer_t - Buffer containing the demand reset response
 *
 * Notes:
 *
 ***********************************************************************************************************************/
static buffer_t *DR_PerformReset(void *payloadBuf, uint8_t *respCode)
{
  //Prepare Full Meter Read structure
  sysTimeCombined_t currentTime;   // Get current time
  uint32_t      msgTime;           // Time converted to seconds since epoch
  uint8_t       eventCount = 0;    // Counter for alarms and readings
  pack_t        packCfg;          // Struct needed for PACK functions
  buffer_t      *pBuf;            // pointer to message

  //Allocate a big buffer to accomadate worst case payload.
  //We'll assume 12 bytes / reading type - it should never really be this big
  pBuf = BM_alloc( (sizeof(FullMeterRead) + 12 * DR_MAX_READINGS) + VALUES_INTERVAL_END_SIZE + EVENT_AND_READING_QTY_SIZE );
  if ( pBuf != NULL )
  {
     uint8_t eventQty, readingQty, nvPairQty, evDetValQty;
     uint16_t bitNo=0;    //Used by unpack

     PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

   /***
   * Read the event and reading quantities
   ***/
     eventQty = UNPACK_bits2_uint8(payloadBuf, &bitNo, EXCH_WITH_ID_QTY_SIZE_BITS);
     readingQty = UNPACK_bits2_uint8(payloadBuf, &bitNo, EXCH_WITH_ID_QTY_SIZE_BITS);
     if ((eventQty == 1) && (readingQty == 1))
     {
       uint16_t eventType;

       eventType = UNPACK_bits2_uint16(payloadBuf, &bitNo, EXCH_WITH_ID_EVENTTYPE_SIZE_BITS);
       if (eventType == electricMeterDemandDemandReset)
       {
         nvPairQty = UNPACK_bits2_uint8(payloadBuf, &bitNo, EXCH_WITH_ID_QTY_SIZE_BITS);
         evDetValQty = UNPACK_bits2_uint8(payloadBuf, &bitNo, EXCH_WITH_ID_QTY_SIZE_BITS);
         if ((nvPairQty == 0) && (evDetValQty == 0))
         {
           int8_t readingValPow10;
           uint8_t readingValTypecast, readingFlags;
           uint16_t readingType, readingValNumBytes;

           readingType = APP_MSG_GetNextReadingInfo(payloadBuf, &bitNo, &readingValNumBytes,
                                                    &readingValTypecast, &readingValPow10, &readingFlags);

         /***
         * The reading type passed in thie request needs to be edMfgSerialNumber - the
         * value of that reading will be compared against this meter's serial number,
         * and only if they match will the reset be done.
         ***/
           if (readingType == edMfgSerialNumber)
           {
             uint8_t meterSerNum[31];

             if (INTF_CIM_CMD_getMfgSerNum(meterSerNum) == CIM_QUALCODE_SUCCESS)
             {
               if ((strlen((char *)meterSerNum) == readingValNumBytes) &&
                   (memcmp(&((uint8_t *)payloadBuf)[bitNo>>3], meterSerNum, readingValNumBytes) == 0))
               {
                 sysTimeCombined_t currentTimeTicks;

               /***
               * Serial numbers match - check that we have a valid time, and also if we've recently done
               * a demand reset (within the demandResetLockoutPeriod)  If we have done a reset within
               * the lockout period, we won't do another one, but we will return the readings, since
               * this duplicate demand reset is likely issued because the response was never received.
               ***/
                 if (TIME_SYS_IsTimeValid())
                 {   // have a valid time - see how long since we last sent the metadata
                   (void)TIME_UTIL_GetTimeInSysCombined(&currentTimeTicks);

                   uint32_t currTimeSecs = (uint32_t)(currentTimeTicks / TIME_TICKS_PER_SEC);

                   *respCode = Created;
                 }
                 else
                   *respCode = ServiceUnavailable;   // we don't currently have a valid time
               }
               else
                 *respCode = NotAuthorized;   // serial numbers don't match
             }
             else
               *respCode = InternalServerError;   // unable to read serial number
           }
           else
             *respCode = BadRequest;   // only edMfgSerialNumber is allowed
         }
         else
           *respCode = BadRequest;   // shouldn't be an name/value pairs or event details values
       }
       else
         *respCode = BadRequest;   // only Demand Reset event command type allowed
     }
     else
       *respCode = BadRequest;   // only 1 event/reading allowed

     //Add current timestamp
     (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
     msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
     PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
     pBuf->dataLen = VALUES_INTERVAL_END_SIZE;

     //Insert the reading quantity, it will be adjusted later to the correct value
     PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
     pBuf->dataLen += sizeof(eventCount);

// Todo - results go here

     //Insert the correct number of readings
     pBuf->data[VALUES_INTERVAL_END_SIZE] = eventCount;

     DBG_logPrintf( 'D', " Metadata response:");

   /***
   * Print the resulting message in hex format, but we may need to split it
   * across multiple lines if the hex string is longer than the max debug
   * line size.  We'll also remove some from the debug msg size since the
   * date/time and task also go into the output
   ***/
#if ENABLE_DEBUG_PRINT
     {
       int bytesToPrint, bytesPrinted = 0;
       for (bytesToPrint=pBuf->dataLen; bytesToPrint > 0;
            bytesToPrint = pBuf->dataLen - bytesPrinted)
       {
         if (bytesToPrint*2+2 > (DEBUG_MSG_SIZE-50))
           bytesToPrint = (DEBUG_MSG_SIZE-50-2) / 2;
         DBG_logPrintHex( 'D',& pBuf->data[bytesPrinted], bytesToPrint );
         bytesPrinted += bytesToPrint;
       }
     }
#endif
  }
  return pBuf;
}