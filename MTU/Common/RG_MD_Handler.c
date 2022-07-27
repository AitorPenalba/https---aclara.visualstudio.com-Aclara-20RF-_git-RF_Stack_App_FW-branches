/***********************************************************************************************************************
 *
 * Filename: RG_MD_Handler.c
 *
 * Global Designator: RG_MD_
 *
 * Contents:  Handles New Endpoint Registration Metadata (/rg/md resource)
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
 * $Log$ cmusial Created Mar 20, 2015
 *
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "project.h"

//lint -esym(750,RG_MD_GLOBALS)
#define RG_MD_GLOBALS
#include "RG_MD_Handler.h"
#undef RG_MD_GLOBALS

#include "ansi_tables.h"
#include "intf_cim_cmd.h"
#include "endpt_cim_cmd.h"
#include "time_util.h"
#include "pack.h"
#include "byteswap.h"
#include "DBG_SerialDebug.h"
#include "mode_config.h"

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* GLOBAL VARIABLE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */
static const meterReadingType RG_MD_RdgType_[] =
{
   comDeviceFirmwareVersion,
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
#else    /* Only devices with a bootloader   */
   comDeviceBootloaderVersion,
#endif
   installationDateTime,
   comDeviceHardwareVersion,
   comDeviceMACAddress,
   comDeviceType,
   comDeviceMicroMPN,
#if ( ACLARA_LC != 1 ) && (ACLARA_DA != 1) /* meter specific code */
   demandPresentConfiguration,
   edFwVersion,
   lpBuSchedule,
   newRegistrationRequired,
#if ( REMOTE_DISCONNECT == 1 )
   disconnectCapable,
#endif
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   edHwVersion,
   edMfgSerialNumber,
   edModel,
   edProgramID,
#endif
#if ( REMOTE_DISCONNECT == 1 )
   switchPositionStatus,
#endif
#else
   edMfgSerialNumber,
   edHwVersion,
   edFwVersion,
   edModel,
   newRegistrationRequired,
#endif
#if (ACLARA_DA == 1)
   edManufacturer,
   edBootVersion,
   edBspVersion,
   edKernelVersion,
   edCarrierHwVersion,
#endif
   timeZoneDstHash,
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   ansiTableOID,
   edProgrammedDateTime,
   edProgrammerName,
#endif
   opportunisticAlarmIndexID,
   realTimeAlarmIndexID,
   capableOfEpPatchDFW,
   capableOfEpBootloaderDFW,
   capableOfMeterPatchDFW,
   capableOfMeterBasecodeDFW,
   capableOfMeterReprogrammingOTA
};
#define RG_MD_MAX_READINGS (ARRAY_IDX_CNT(RG_MD_RdgType_))

/* LOCAL FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */


/***********************************************************************************************************************
 *
 * Function Name: RG_MD_MsgHandler
 *
 * Purpose: Handle RG_MD (New Endpoint Registration Metadata)
 *
 * Arguments:
 *     APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
 *     void *payloadBuf           - pointer to data payload; not used, required for API
 *     length                     - payload length
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
//lint -esym(715, payloadBuf) not referenced
//lint -efunc(818, RG_MD_MsgHandler) arguments cannot be const because of API requirements
void RG_MD_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length)
{
   (void)length;
   if ( ( method_get == (enum_MessageMethod)heepReqHdr->Method_Status ) && ( MODECFG_get_rfTest_mode() == 0 ) )
   { //Process the get request
      buffer_t      *pBuf;            // pointer to message

      pBuf = RG_MD_BuildMetadataPacket();
      if (pBuf != NULL)                            // Send the message to message handler.
      {
          HEEP_APPHDR_t  heepRespHdr;      // Application header/QOS info

         //Application header/QOS info
         HEEP_initHeader( &heepRespHdr );
         heepRespHdr.TransactionType = (uint8_t)TT_RESPONSE;
         heepRespHdr.Resource = (uint8_t)rg_md;
         heepRespHdr.Method_Status = (uint8_t)OK;
         heepRespHdr.Req_Resp_ID = (uint8_t)heepReqHdr->Req_Resp_ID;
         heepRespHdr.qos = (uint8_t)heepReqHdr->qos;
         heepRespHdr.callback = NULL;
         heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;


         (void)HEEP_MSG_Tx (&heepRespHdr, pBuf);   // The called function will free the buffer
      }
      //else No buffer available, drop the message for now
   }
   //else Other methods not supported
}//lint !e715 !e818
//lint +esym(715, payloadBuf) not referenced

/*smglint -esym(550,disconnCapableVal) not accessed, based on meter type   */
buffer_t *RG_MD_BuildMetadataPacket(void)
{
   //Prepare Full Meter Read structure
   sysTimeCombined_t currentTime;   // Get current time
   uint32_t      msgTime;           // Time converted to seconds since epoch
   uint32_t      dtTime;            // Used to convert date/time reading values
   uint8_t       eventCount = 0;    // Counter for alarms and readings
   uint8_t       i;                 // Loop counter
   int64_t       value = 0;         // Reading value
   uint16_t      RdgType;           // Reading identifier
   uint8_t       powerOf10;         // Power of 10
   uint8_t       RdgInfo;           // Bit field ahead of every reading qty
   pack_t        packCfg;           // Struct needed for PACK functions
   buffer_t      *pBuf;             // pointer to message
   uint16_t      indexOfEventCount; // Position of the Event and Reading Qty

   /* Allocate a big buffer to accommodate worst case payload.
      Each reading, worst case is
         readingQty_t   : 5 bytes ( fixed )
         Time stamp     : 4 bytes ( variable; 0 or 4 )
         Quality code   : 1 byte  ( variable; 0 or 1 )
         Reading value  :15 bytes ( variable; 1 - 15 )
      Assume average reading value of 8 bytes, no quality codes, all with time stamp, gives:
         12 = 8 + 0 + 4
      Add sizeof FullMeterRead for the overhead in the bit struct. Note: FullMeterRead includes size of 1 worst case
      reading, so there is room for additional data. */

#if 0
   DBG_logPrintf( 'I', "Size of FullMeterRead: %d, eventHdr_t: %d, uint32_t: %d, EVEventType: %d, uint8_t:%d, readingQty_t: %d",
                     sizeof( FullMeterRead), sizeof( eventHdr_t ), sizeof( uint32_t ), sizeof( EDEventType ),
                     sizeof( uint8_t ), sizeof( readingQty_t ) );
#endif

   pBuf = BM_alloc( ( sizeof( FullMeterRead ) + RG_MD_MAX_READINGS * 12 ) );
   if ( pBuf != NULL )
   {
#if ( REMOTE_DISCONNECT == 1 )
      int32_t  disconnCapableVal=-1;
#endif
      /* TODO: as items are packed into pBuf, verify that original length request is not exceeded. */
      PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

      indexOfEventCount = VALUES_INTERVAL_END_SIZE;

      //Add current timestamp
      (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
      msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
      PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
      pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

      //Insert the reading quantity, it will be adjusted later to the correct value
      PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
      pBuf->x.dataLen += sizeof(eventCount);

      //Read the values and add them
      for ( i = 0; i < RG_MD_MAX_READINGS; i++ )
      {
         uint8_t               powerOf10Code = 0;
         uint8_t               numberOfBytes = 0;
         uint8_t               str[31];
         ReadingsValueTypecast rdgDataType;
         enum_CIM_QualityCode  getRdgValueStatus;

         (void)memset( str, 0, sizeof(str) );

         //Since there could be more than 15 entries, check if 15 are already present
         if (15 == eventCount)
         {
            pBuf->data[indexOfEventCount] = eventCount; //Close the current set

            //start new set
            PACK_Buffer( -(VALUES_INTERVAL_END_SIZE_BITS), (uint8_t *)&msgTime, &packCfg); //Add timestamp from above
            pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
            eventCount = 0;
            PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
            indexOfEventCount = pBuf->x.dataLen;
            pBuf->x.dataLen += sizeof(eventCount);
         }

#if ( REMOTE_DISCONNECT == 1 )
         // Verify RCDC switch is present.
         if (switchPositionStatus == RG_MD_RdgType_[i])
         {
            uint8_t uRCDCPresent;

            /***
            * If we've already determined that this meter is not capable of
            * disconnect operation, don't bother checking any further.
            ***/
            if (0 == disconnCapableVal)
            {
               continue;
            }

            /***
            * If we haven't yet determined if the meter is disconnect capable,
            * check now, and if it isn't, bail
            ***/
            if (disconnCapableVal < 0)
            {
               if (CIM_QUALCODE_SUCCESS != INTF_CIM_CMD_getRCDCSwitchPresent(&uRCDCPresent, (bool)false))
               {
                  continue;
               }
               else if (0 == uRCDCPresent)
               {
                  continue;
               }
            }
         }
#endif

         /***
         * First check if this is a reading type to retrieve from the host meter
         ***/
         ValueInfo_t reading; // declare variable to retrieve host meter readings
         if (CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading(RG_MD_RdgType_[i], &reading) )
         {
            getRdgValueStatus = CIM_QUALCODE_SUCCESS;
            ( void )memcpy( str, reading.Value.buffer, min(sizeof(str), reading.valueSizeInBytes) );
            rdgDataType = reading.typecast;

            switch ( reading.typecast )
            {
               case intValue:
               case uintValue:   // for now we'll treat the same as an int, but may need to change
                  powerOf10Code = HEEP_getPowerOf10Code( reading.power10, (int64_t *)&reading.Value.intValue );
                  numberOfBytes = HEEP_getMinByteNeeded( reading.Value.intValue, reading.typecast, 0);
                  Byte_Swap( &str[0], 8 );
                  DBG_logPrintf( 'I', "Ch=%2d, IntVal=%lld, power=%d, NumBytes=%d",
                                 i, reading.Value.intValue, powerOf10Code, numberOfBytes );
                  break;
               case ASCIIStringValue:
                  powerOf10Code = 0;   // n/a for string values
                  numberOfBytes = (uint8_t)reading.valueSizeInBytes;
                  DBG_logPrintf( 'I', "Ch=%2d, StrVal='%s', NumBytes=%d", i, str, reading.valueSizeInBytes);
                  break;
               case Boolean:
                  if (str[0] != 0)
                  {
                     str[0] = 1;   // Force value to be either 0 or 1
                  }
                  numberOfBytes = 1;
                  powerOf10Code = 0;   // n/a for boolean values
                  DBG_logPrintf( 'I', "Ch=%2d, BoolVal=%s", i, str[0] ? "True" : "False");
                  break;
               case dateTimeValue:
                  powerOf10Code = 0;   // n/a for date/time values
                  numberOfBytes = (uint8_t)reading.valueSizeInBytes;
                  DBG_logPrintf( 'I', "Ch=%2d, DateTimeVal=%lu", i, (uint32_t)reading.Value.uintValue);
                  Byte_Swap( (uint8_t *)&str[0], (uint8_t)reading.valueSizeInBytes );
                  break;
               case hexBinary:
               {
                  char dbgStr[37];    // Enough for: "Ch=99, HexVal=0xWWXXYYZZ, NumBytes=4"
                     uint8_t j;
                     uint8_t lastLoc;

                     powerOf10Code = 0;   // n/a for hexBinary values
                     numberOfBytes = (uint8_t)reading.valueSizeInBytes;
                     lastLoc = ( uint8_t )snprintf ( &dbgStr[0], sizeof( dbgStr ), "Ch=%2d, HexVal=0x", i );
                     for ( j = 0; j < reading.valueSizeInBytes; j++ )
                     {
                        lastLoc += ( uint8_t )snprintf( &dbgStr[lastLoc], sizeof( dbgStr ) - lastLoc, "%02X", str[j] );
                     }
                     ( void )snprintf ( &dbgStr[lastLoc], sizeof( dbgStr ) - lastLoc, ", NumBytes=%d", reading.valueSizeInBytes );
                     DBG_logPrintf( 'I', &dbgStr[0]);

                  break;
               }
               default:
                  getRdgValueStatus = CIM_QUALCODE_CODE_INDETERMINATE;
                  DBG_logPrintf( 'E', "Reading Type %d has unknown data type: %d", RG_MD_RdgType_[i], reading.typecast);
                  break;
            } /*lint !e788 not all enumbs used in switch   */
         }
         else
         {
            /***
            * Not from the host meter - better be from the endpoint
            ***/
            rdgDataType = ENDPT_CIM_CMD_getDataType( RG_MD_RdgType_[i] );
            switch (rdgDataType)
            {
               case intValue:
               case uintValue:   // for now we'll treat the same as an int, but may need to change
                  getRdgValueStatus   = ENDPT_CIM_CMD_getIntValueAndMetadata(RG_MD_RdgType_[i], &value, &powerOf10);
                  if (getRdgValueStatus == CIM_QUALCODE_SUCCESS)
                  {
                     powerOf10Code = HEEP_getPowerOf10Code(powerOf10, &value);
                     numberOfBytes = HEEP_getMinByteNeeded(value, rdgDataType, 0);
                     (void)memcpy(&str[0], &value, 8);
                     Byte_Swap(&str[0], 8);
                     DBG_logPrintf( 'I', "Ch=%2d, IntVal=%lld, power=%d, NumBytes=%d", i, value, powerOf10Code, numberOfBytes);
                  }

                  break;
               case ASCIIStringValue:
                  getRdgValueStatus   = ENDPT_CIM_CMD_getStrValue( RG_MD_RdgType_[i], str, sizeof(str), (uint8_t *)&numberOfBytes );
                  if (getRdgValueStatus == CIM_QUALCODE_SUCCESS)
                  {
                     powerOf10Code = 0;   // n/a for string values
                     DBG_logPrintf( 'I', "Ch=%2d, StrVal='%s', NumBytes=%d", i, str, numberOfBytes);
                  }

                  break;
               case Boolean:
                  getRdgValueStatus   = ENDPT_CIM_CMD_getBoolValue( RG_MD_RdgType_[i], &str[0] );
                  if (getRdgValueStatus == CIM_QUALCODE_SUCCESS)
                  {
                     if (str[0] != 0)
                     {
                        str[0] = 1;   // Force value to be either 0 or 1
                     }

                     numberOfBytes = 1;
                     powerOf10Code = 0;   // n/a for boolean values
                     DBG_logPrintf( 'I', "Ch=%2d, BoolVal=%s", i, str[0] ? "True" : "False");
                  }
                  break;
               case dateTimeValue:
                  getRdgValueStatus   = ENDPT_CIM_CMD_getDateTimeValue( RG_MD_RdgType_[i], &dtTime );
                  if (getRdgValueStatus == CIM_QUALCODE_SUCCESS)
                  {
                     powerOf10Code = 0;   // n/a for date/time values
                     numberOfBytes = sizeof(dtTime);
                     DBG_logPrintf( 'I', "Ch=%2d, DateTimeVal=%lu", i, dtTime);
                     Byte_Swap( (uint8_t *)&dtTime, 4 );
                     (void)memcpy (str, &dtTime, 4);
                  }

                  break;
               default:
                  getRdgValueStatus = CIM_QUALCODE_CODE_INDETERMINATE;
                  DBG_logPrintf( 'E', "Reading Type %d has unknown data type: %d", RG_MD_RdgType_[i], rdgDataType);
                  break;
            } /*lint !e788 not all enumbs used in switch   */
         }

         if ( CIM_QUALCODE_SUCCESS == getRdgValueStatus )
         {
            ++eventCount;

            //No timestamp or quality codes present
            RdgInfo = 0; //TODO May need to add timestamp or quality codes later

            RdgInfo |= (uint8_t)(powerOf10Code);
            PACK_Buffer((sizeof(RdgInfo) * 8), &RdgInfo, &packCfg);
            pBuf->x.dataLen += sizeof(RdgInfo);

            /***
            * High-order 8 bits of the 12-byte value size
            ***/
            RdgInfo =  (uint8_t)((numberOfBytes & 0x0F00) >> 4);
            RdgInfo |= (uint8_t)((numberOfBytes & 0x00F0) >> 4);
            PACK_Buffer((sizeof(RdgInfo) * 8), &RdgInfo, &packCfg);
            pBuf->x.dataLen += sizeof(RdgInfo);

            /***
            * Low-order 4 bites of 12-byte value size + value typecast
            ***/
            RdgInfo = (uint8_t)((numberOfBytes & 0x000F) << 4);
            RdgInfo |= ((uint8_t)rdgDataType & 0x07);
            PACK_Buffer((sizeof(RdgInfo) * 8), &RdgInfo, &packCfg);
            pBuf->x.dataLen += sizeof(RdgInfo);

            /***
            * Reading type
            ***/
            RdgType = (uint16_t)RG_MD_RdgType_[i];
            PACK_Buffer(-(int16_t)(sizeof(RdgType) * 8), (uint8_t *)&RdgType, &packCfg);
            pBuf->x.dataLen += sizeof(RdgType);

            /***
            * Finally the reading values.
            * For int values, the value is stored at the end of the str array, so
            * need to copy from the end.  Otherwise copy from the front of the array.
            ***/
            if ((rdgDataType == intValue) || (rdgDataType == uintValue))
            {
               PACK_Buffer((numberOfBytes * 8), &str[8 - numberOfBytes], &packCfg);
            }
            else
            {
               PACK_Buffer((numberOfBytes * 8), &str[0], &packCfg);
            }

            pBuf->x.dataLen += numberOfBytes;

#if ( REMOTE_DISCONNECT == 1 )
            if (disconnectCapable == RG_MD_RdgType_[i])
            {
               disconnCapableVal = str[0];
            }
#endif
         }
         else  // No reading for this channel
         {
#if ( REMOTE_DISCONNECT == 1 )
            if (disconnectCapable == RG_MD_RdgType_[i])
            {
               disconnCapableVal = 0;
            }
#endif
            DBG_logPrintf( 'E', "No reading available for channel %d", i);
         }
      }

      //Insert the correct number of readings
      pBuf->data[indexOfEventCount] = eventCount;

   }

   return pBuf;
}
