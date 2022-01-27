/***********************************************************************************************************************
 *
 * Filename: OR_MR_Handler.c
 *
 * Global Designator: OR_MR_
 *
 * Contents:  Handles on-request readings (/OR/MR resource)
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2018 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************
 *
 * $Log$ msingla Created Jan 26, 2015
 *
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"

//lint -esym(750,OR_MR_GLOBALS)
#define OR_MR_GLOBALS
#include "OR_MR_Handler.h"
#undef OR_MR_GLOBALS

#include "file_io.h"
#include "ansi_tables.h"
#include "intf_cim_cmd.h"
#include "time_util.h"
#include "pack.h"
#include "DBG_SerialDebug.h"
#include "demand.h"
#include "hmc_mtr_info.h"
#include "mode_config.h"

/* MACRO DEFINITIONS */
#define OR_MR_FILE_UPDATE_RATE  ((uint32_t)0xFFFFFFFF)  //Infrequent updates

/* TYPE DEFINITIONS */
typedef struct
{
   meterReadingType orReadList[OR_MR_MAX_READINGS];
}orParams_t;  //Source for each channel


/* GLOBAL VARIABLE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */
static FileHandle_t  orFileHndl_;        //Contains the file handle information
static orParams_t    orParams_;          //Local copy of the file
static OS_MUTEX_Obj  OR_MR_Mutex_;

/* LOCAL FUNCTION PROTOTYPES */
static void doMeterReadings(const meterReadingType *rdgList, uint16_t lenRdgList, HEEP_APPHDR_t* rspHdr, buffer_t *pBuf);

/* FUNCTION DEFINITIONS */


/***********************************************************************************************************************
 *
 * Function name: OR_MR_init
 *
 * Purpose: Opens files and set default values, if virgin
 *
 * Arguments: None
 *
 * Returns: returnStatus_t - result of file open.
 *
 * Side Effects: None
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t OR_MR_init( void )
{
   FileStatus_t fileStatus;  //Indicates if the file is created or already exists
   returnStatus_t retVal = eFAILURE;

   if ( OS_MUTEX_Create(&OR_MR_Mutex_) )
   {
      if ( eSUCCESS == FIO_fopen(&orFileHndl_,                /* File Handle */
                                 ePART_SEARCH_BY_TIMING,      /* Search for best partition according to the timing.*/
                                 (uint16_t)eFN_OR_MR_INFO,    /* File ID (filename) */
                                 (lCnt)sizeof(orParams_t),    /* Size of the data in the file */
                                 0,                           /* File attributes to use */
                                 OR_MR_FILE_UPDATE_RATE,      /* Update rate of this file */
                                 &fileStatus) )
      {
         if (fileStatus.bFileCreated)
         {  // the file was created
            (void)memset(&orParams_, 0, sizeof(orParams_));
#if ( COMMERCIAL_METER == 0 )
            orParams_.orReadList[0] = fwdkWh;
            orParams_.orReadList[1] = revkWh;
            orParams_.orReadList[2] = vRMS;
            orParams_.orReadList[3] = switchPositionStatus;
#elif ( COMMERCIAL_METER == 1 )
            orParams_.orReadList[0] = fwdkWh;
            orParams_.orReadList[1] = revkWh;
            orParams_.orReadList[2] = vRMSA;
            orParams_.orReadList[3] = vRMSB;
            orParams_.orReadList[4] = vRMSC;
#else
            orParams_.orReadList[0] = fwdkWh;
            orParams_.orReadList[1] = revkWh;
            orParams_.orReadList[2] = vRMSA;
            orParams_.orReadList[3] = switchPositionStatus;
#endif
            retVal = FIO_fwrite(&orFileHndl_, 0, (uint8_t const *)&orParams_, (lCnt)sizeof(orParams_t)); //save the reading types
         }
         else
         {
            retVal = FIO_fread(&orFileHndl_, (uint8_t *)&orParams_, 0, (lCnt)sizeof(orParams_t)); //Read the complete file
         }
      }
   }
   return retVal;
}

/***********************************************************************************************************************
 *
 * Function Name: OR_MR_MsgHandler
 *
 * Purpose: Handle OR_MR (On-request readings)
 *
 * Arguments:
 *     APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
 *     void *payloadBuf           - pointer to data payload
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
//lint -efunc(818, OR_MR_MsgHandler) arguments cannot be const because of API requirements
void OR_MR_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length)
{
   (void)length;
   if ( ( method_get == (enum_MessageMethod)heepReqHdr->Method_Status ) && ( MODECFG_get_rfTest_mode() == 0 ) )
   { //Process the get request

      //Prepare Compact Meter Read structure
      HEEP_APPHDR_t  heepRespHdr;         // Application header/QOS info
      buffer_t       *pBuf;
      meterReadingType rdgList [OR_MR_MAX_READINGS] = {invalidReadingType}; //invalid reading is zero
      union TimeSchRdgQty_u qtys;         // For Time Qty and Reading Qty
      uint16_t      bits = 0;             // Keeps track of the bit position within the Bitfields
      uint16_t      bytesLeft = length;
      bool          dfltRdgs = true;
      uint16_t NumRdgs = 0;
      uint16_t RdgType;                   //unpacked reading type
      uint8_t i;                              //loop variable

      OS_MUTEX_Lock(&OR_MR_Mutex_); // Function will not return if it fails

      //Application header/QOS info
      HEEP_initHeader( &heepRespHdr );
      heepRespHdr.TransactionType = (uint8_t)TT_RESPONSE;
      heepRespHdr.Resource = (uint8_t)or_mr;
      heepRespHdr.Method_Status = (uint8_t)OK;
      heepRespHdr.Req_Resp_ID = (uint8_t)heepReqHdr->Req_Resp_ID;
      heepRespHdr.qos = (uint8_t)heepReqHdr->qos;
      heepRespHdr.callback = NULL;
      heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;

      if( length < EVENT_AND_READING_QTY_SIZE )
      { //Invalid request,
         //Must have at least one mini header for readings requested
         heepRespHdr.Method_Status = (uint8_t)BadRequest;
         (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
      }
      else
      {
         qtys.TimeSchRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS); //Parse the fields passed in

         if( qtys.bits.timeSchQty > 0)
         { //Invalid request, send the header back
            heepRespHdr.Method_Status = (uint8_t)BadRequest;
            (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
         }
         else
         {
            //Allocate a big buffer to accomadate worst case payload.
            pBuf = BM_alloc( APP_MSG_MAX_DATA );
            if ( pBuf != NULL )
            {
               dfltRdgs = ( 0 == qtys.bits.rdgQty );
               if ( dfltRdgs )
               {
                  //the default Orreadlist has all valid rdgtypes pushed the front w/ trailing zeros
                  //orreadlist SET cannot do this preemptivley becuase the GET must match the input over the MFG
                  for( i = 0;  i< OR_MR_MAX_READINGS; ++i)
                  {
                     if( invalidReadingType != orParams_.orReadList[i] )
                     {
                        rdgList[NumRdgs] = orParams_.orReadList[i];
                        NumRdgs++;
                     }
                  }
               }
               else //dfltRdgs == false
               {  //Reading list passed in the request
                  //move back to the begining of the buffer to begin parsing
                  bits = 0;
                  //make sure that we have at least one mini header to unpack as we go through message
                  while(  EVENT_AND_READING_QTY_SIZE <= bytesLeft )
                  {
                     qtys.TimeSchRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS); //Parse the fields passed in
                     bytesLeft -= EVENT_AND_READING_QTY_SIZE;
                     //the default reading list check ensures that the first rdg qty is non zero. Any
                     //subsequent header with a rdg qty 0 is considered a bad request
                     if( 0 == qtys.bits.rdgQty )
                     { //Invalid request, send only the header back
                        heepRespHdr.Method_Status = (uint8_t)BadRequest;
                        break;
                     }
                     bytesLeft -= (qtys.bits.timeSchQty * 8); //Skip the time qty pairs, if present
                     bits      += (qtys.bits.timeSchQty * 8) * 8;

                     if( bytesLeft < (qtys.bits.rdgQty *  sizeof(RdgType) ) )
                     {  //check we will not overun buffer when unpacking reading types
                        heepRespHdr.Method_Status = (uint8_t)BadRequest;
                        break;
                     }

                     for(i = 0; (i < qtys.bits.rdgQty) && (NumRdgs < OR_MR_MAX_READINGS) ; i++)
                     {
                        RdgType          = UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(RdgType) * 8);
                        bytesLeft        -= sizeof(RdgType);
                        rdgList[NumRdgs] = ( meterReadingType )RdgType;
                        NumRdgs++;

                     }
                     if( ( OR_MR_MAX_READINGS == NumRdgs ) && bytesLeft )
                     {  //reach maximum allowed reading types with more following
                        //message will only contain partial content in the response
                        heepRespHdr.Method_Status = (uint8_t) PartialContent;
                        break;
                     }
                  }
               }
               if( (uint8_t)BadRequest == heepRespHdr.Method_Status )
               {
                  (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
                  //must free the allocated buffer since 'Tx Header Only' does NOT!
                  BM_free( ( buffer_t * )pBuf );
               }
               else
               {  //perform meter readings and build APP msg
                  doMeterReadings(rdgList, NumRdgs, &heepRespHdr, pBuf);
                  // send the message. The called function will free the buffer
                  (void)HEEP_MSG_Tx (&heepRespHdr, pBuf);
               }
            }
            else
            { //else No buffer available, drop the message for now
               heepRespHdr.Method_Status = (uint8_t)Conflict;
               (void)HEEP_MSG_TxHeepHdrOnly(&heepRespHdr);
            }
         }
      }
      OS_MUTEX_Unlock(&OR_MR_Mutex_); // Function will not return if it fails
   }
   //else Other methods not supported
}
//lint +esym(715, payloadBuf) not referenced

/***********************************************************************************************************************
 *
 * Function Name: doMeterReadings
 *
 * Purpose: Perform the meter readings and build APP msg
 *
 * Arguments:
 *     meterReadingType *rdgList     -array of reading types to be read from the meter
 *     uint16_t lenRdgList           -number of rdg types in the array
 *     buffer_t *pBuf                - buffer in which to pack the APP msg for meter readings
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
static void doMeterReadings(const meterReadingType *rdgList, uint16_t lenRdgList, HEEP_APPHDR_t* rspHdr, buffer_t *pBuf)
{
  uint8_t   RdgInfo;                         // Bit field ahead of every reading qty
  //uint8_t   powerOf10;                       // Power of 10
  uint8_t   eventCount        = 0;           // Counter for alarms and readings for a single bitstruture(0-15)
  uint16_t  readingQtyIndex;                 // index into rding qty of current bit structure
  uint16_t  i;                               // loop variable
  uint16_t  RdgType;                         // Reading identifier
  uint32_t  msgTime;                         // Time converted to seconds since epoch
  int64_t   value             = 0;           // Reading value
  bool      addTimestamp      = (bool)false; // Add timestamp to the current reading
  sysTimeCombined_t  currentTime;            // Get current time
  enum_CIM_QualityCode  retVal;              // return status
  pack_t packCfg;
#if ( DEMAND_IN_METER == 0 )
  peak_t demandVal            = { 0 };       // Demand and associated timestamp
#endif
  ValueInfo_t  reading;

  PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );
  //Add current timestamp
  (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
  msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
  PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
  pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;
  readingQtyIndex = pBuf->x.dataLen;

  //Insert the reading quantity, it will be adjusted later to the correct value
  PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
  pBuf->x.dataLen += sizeof(eventCount);

  for( i = 0; i < lenRdgList; i++)
  {

     RdgType = (uint16_t) rdgList[i];
     //add new CMR header if at max reading count
     if( READING_QTY_MAX == eventCount)
     {
        pBuf->data[ readingQtyIndex ] = eventCount; //insert corret number of readings
        //Add timestamp for next group in response chain
        msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
        PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
        pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
        readingQtyIndex = pBuf->x.dataLen; //update event index into current bit structure
        eventCount = 0; //reinitialize the event count
        PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
        pBuf->x.dataLen += sizeof(eventCount); //eventcount will be updated later


     }

     if( invalidReadingType == (meterReadingType) RdgType)
     {
        ++eventCount;
        RdgInfo = 0x40;
        PACK_Buffer((sizeof(RdgInfo) * 8), &RdgInfo, &packCfg);
        pBuf->x.dataLen += sizeof(RdgInfo);

        PACK_Buffer(-(int16_t)(sizeof(RdgType) * 8), (uint8_t *)&RdgType, &packCfg);
        pBuf->x.dataLen += sizeof(RdgType);

        retVal = CIM_QUALCODE_CODE_UNKNOWN_READING_TYPE;
        PACK_Buffer((sizeof(retVal) * 8), (uint8_t *)&retVal, &packCfg); //Add quality code
        pBuf->x.dataLen += sizeof(retVal);

        retVal = (enum_CIM_QualityCode)0; //Add 1 byte dummy read as 0 length is not supported
        PACK_Buffer((sizeof(retVal) * 8), (uint8_t *)&retVal, &packCfg);
        pBuf->x.dataLen += sizeof(retVal);

        rspHdr->Method_Status = (uint8_t) PartialSuccess;
        continue;
     }
#if ( DEMAND_IN_METER == 0 )
     // Daily Demand is handled only in the module for SRFNI-210+
     else if ( ( uint16_t )dailyMaxFwdDmdKW == RdgType )
     {  //Special handling Daily Max demand
        retVal = DEMAND_Current_Daily_Peak(&demandVal); //Get the shifted value
        value = (int64_t)demandVal.energy;
        msgTime = (uint32_t)(demandVal.dateTime / TIME_TICKS_PER_SEC);
        powerOf10 = 4;        //Source value in deci-watts, report in kWatts
        addTimestamp = (bool)true;
        RdgInfo = 0x80; //Time stamp present
     }
#endif
     else
     { //valid reading type, get the meter reading
        uint32_t       fracSec;
        retVal = INTF_CIM_CMD_getMeterReading( (meterReadingType)RdgType, &reading );

        addTimestamp = (bool)false;
        RdgInfo      = 0; //No timestamp or quality codes present

        // retrieve the  information for the reading type
        retVal = INTF_CIM_CMD_getMeterReading( (meterReadingType)RdgType, &reading );

        TIME_UTIL_ConvertSysFormatToSeconds((sysTime_t *)&reading.timeStamp, &msgTime, &fracSec);
        if ( 0 != msgTime )
        {
           addTimestamp = (bool)true;
           RdgInfo      = 0x80; //Timestamp present
        }
        //CMR only supports integer tyepcasts.
        if( ( CIM_QUALCODE_SUCCESS == retVal ) &&
           ( ( reading.typecast != intValue ) && ( reading.typecast != uintValue ) ) )
        {
           DBG_printf( "Ch=%2d Typecast Mismatch", i);
           retVal = CIM_QUALCODE_CODE_TYPECAST_MISMATCH;

        }

     }

     if ( CIM_QUALCODE_SUCCESS == retVal )
     {
        uint8_t powerOf10Code;
        uint8_t numberOfBytes;

#if ( REMOTE_DISCONNECT == 1 )
        // Verify RCDC switch is present.
        if (( uint16_t )switchPositionStatus == RdgType)
        {
           uint8_t uRCDCPresent;
           if (CIM_QUALCODE_SUCCESS != INTF_CIM_CMD_getRCDCSwitchPresent(&uRCDCPresent, (bool)false))
           {
              continue;
           }
           else if (0 == uRCDCPresent)
           {
              continue;
           }
        }
#endif
        ++eventCount;

        powerOf10Code = HEEP_getPowerOf10Code(reading.power10, (int64_t *)&reading.Value.intValue);
        //non integer typecasts reading types should NOT get here
        numberOfBytes = HEEP_getMinByteNeeded(reading.Value.intValue, reading.typecast, 0);


        DBG_printf( "Ch=%2d, val=%lld, power=%d, NumBytes=%d", i, value, powerOf10Code, numberOfBytes);
        RdgInfo |= (uint8_t)(powerOf10Code << COMPACT_MTR_POWER10_SHIFT);
        RdgInfo |= (uint8_t)((uint8_t)(numberOfBytes -1) << COMPACT_MTR_RDG_SIZE_SHIFT); // Insert the reading info bit field

        if( INTF_CIM_CMD_errorCodeQCRequired( (meterReadingType)RdgType, reading.Value.intValue) )
        {  // value brought back from reading represents an error/information code, set bit to include quality code
           RdgInfo |= 0x40;
           retVal = CIM_QUALCODE_ERROR_CODE;
        }

        PACK_Buffer((sizeof(RdgInfo) * 8), &RdgInfo,&packCfg);
        pBuf->x.dataLen += sizeof(RdgInfo);

        PACK_Buffer(-(int16_t)(sizeof(RdgType) * 8), (uint8_t *)&RdgType, &packCfg);
        pBuf->x.dataLen += sizeof(RdgType);

        if ( addTimestamp )
        {  //Add timestamp here
           PACK_Buffer(-(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime,&packCfg);
           pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
        }

        if( CIM_QUALCODE_SUCCESS != retVal )
        {  // quality code needs to be included in the reponse for this reading
           PACK_Buffer(-(int16_t)(sizeof(retVal) * 8), (uint8_t *)&retVal, &packCfg);
           pBuf->x.dataLen += sizeof(retVal);
        }

        //get the correct number of bytes
        PACK_Buffer(-(numberOfBytes * 8), (uint8_t *)&reading.Value.intValue, &packCfg);
        pBuf->x.dataLen += numberOfBytes;
     }
     else
     { //Reading failed
        ++eventCount;
        RdgInfo = 0x40; //quality code present
        PACK_Buffer((sizeof(RdgInfo) * 8), &RdgInfo, &packCfg);
        pBuf->x.dataLen += sizeof(RdgInfo);

        PACK_Buffer(-(int16_t)(sizeof(RdgType) * 8), (uint8_t *)&RdgType, &packCfg);
        pBuf->x.dataLen += sizeof(RdgType);

        PACK_Buffer((sizeof(retVal) * 8), (uint8_t *)&retVal, &packCfg); //Add quality code
        pBuf->x.dataLen += sizeof(retVal);

        retVal = (enum_CIM_QualityCode)0; //Add 1 byte dummy read as 0 length is not supported
        PACK_Buffer((sizeof(retVal) * 8), (uint8_t *)&retVal, &packCfg);
        pBuf->x.dataLen += sizeof(retVal);
     }


  }//end for loop
  pBuf->data[ readingQtyIndex ] = eventCount; //insert correct number of readings


}

/***********************************************************************************************************************
 *
 * Function name: OR_MR_getReadingTypes
 *
 * Purpose: Get Reading Types from On demand read configuration
 *
 * Arguments: uint16_t* pReadingType
 *
 * Returns: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void OR_MR_getReadingTypes(uint16_t* pReadingType)
{
   uint8_t i;

   OS_MUTEX_Lock(&OR_MR_Mutex_); // Function will not return if it fails
   for (i = 0; i < OR_MR_MAX_READINGS; ++i)
   {
      pReadingType[i] = (uint16_t)orParams_.orReadList[i];
   }
   OS_MUTEX_Unlock(&OR_MR_Mutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function name: OR_MR_setReadingTypes
 *
 * Purpose: Set Reading Types in On demand read configuration
 *
 * Arguments: uint16_t* pReadingType
 *
 * Returns: void
 *
 * Re-entrant Code: Yes
 *
 * Notes: must set all reading types up to OR_MR_MAX_READINGS, including Invalid
 *
 **********************************************************************************************************************/
void OR_MR_setReadingTypes(uint16_t* pReadingType)
{
   uint8_t i;
   OS_MUTEX_Lock(&OR_MR_Mutex_); // Function will not return if it fails
   for (i = 0; i < OR_MR_MAX_READINGS; ++i)
   {
      orParams_.orReadList[i] = (meterReadingType)pReadingType[i];

   }
   (void)FIO_fwrite(&orFileHndl_, 0, (uint8_t const *)&orParams_, (lCnt)sizeof(orParams_t)); //save file
   OS_MUTEX_Unlock(&OR_MR_Mutex_); // Function will not return if it fails
}  /*lint !e818 pReadingType could be pointer to const   */

/***********************************************************************************************************************

   Function Name: OR_MR_OrReadListHandler( action, id, value, attr )

   Purpose: Get/Set orReadList

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t OR_MR_OrReadListHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( (sizeof(uint16_t)*OR_MR_MAX_READINGS) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         OR_MR_getReadingTypes((uint16_t*)value);
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)(sizeof(uint16_t)*OR_MR_MAX_READINGS);
            attr->rValTypecast = (uint8_t)uint16_list_type;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {

      if( attr->rValLen == ( OR_MR_MAX_READINGS * sizeof(uint16_t) ) )
      {
         uint16_t uReadingType[OR_MR_MAX_READINGS] = {0};
         uint16_t *valPtr = value;

         uint8_t cnt;
         for(cnt = 0; cnt < (attr->rValLen / sizeof(uint16_t)); cnt++)
         {
            uReadingType[cnt] = valPtr[cnt];
         }
         OR_MR_setReadingTypes(&uReadingType[0]);
         retVal = eSUCCESS;
      }
      else
      {
         retVal = eAPP_INVALID_VALUE;
      }
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched
