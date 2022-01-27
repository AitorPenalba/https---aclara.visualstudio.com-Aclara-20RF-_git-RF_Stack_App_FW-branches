/***********************************************************************************************************************

   Filename: historyd.c

   Global Designator: HD_

   Contents: Daily historical data and daily bubble-up

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2015-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************

   $Log$ msingla Created Feb 9, 2015

 **********************************************************************************************************************/
/* INCLUDE FILES */
#include "project.h"

#define HD_GLOBALS
#include "historyd.h"
#undef HD_GLOBALS

#include "stddef.h"
#include "file_io.h"
#include "ansi_tables.h"
#include "filenames.h"
#include "pack.h"
#include "APP_MSG_Handler.h"
#include "time_util.h"
#include "time_DST.h"
#include "timer_util.h"
#include "intf_cim_cmd.h"
#include "DBG_SerialDebug.h"
#include "EVL_event_log.h"
#include "demand.h"
#include "rand.h"
#include "ID_intervalTask.h"
#include "partition_cfg.h"
#include "mode_config.h"

#if ( PHASE_DETECTION == 1 )
#include "PhaseDetect.h"
#endif

/* MACRO DEFINITIONS */
/* Using 4 sectors (4K each) for historical/daily data. The low level driver can erase 1 sector
   for wear-leveling. This will leave 3 sectors i.e. 12K space.
   With 35 days requirement, max storage available per day = 1024 * 12 = 12288 / 35 = 351 bytes.

   16K space will be diveded among 348 bytes slots (348 is a multible of 4). So there will be
   47 slots out of which 35 will always be avaibale.
 */
#define HD_NUM_OF_HISTORY_DAYS            (uint8_t)((uint32_t)(PART_NV_HD_SIZE) / sizeof(hdFileData_t))          //Number of entries in the HD array
#define HD_SLOT_SIZE                      ((lAddr)sizeof(hdFileData_t))            //Size of each slot
#define HD_FILE_UPDATE_RATE_SRC_INFO      ((uint32_t)0xFFFFFFFF)  //Infrequent updates
#define HD_INDEX_FILE_UPDATE_RATE         (SECONDS_PER_DAY/2)     //Once or twice a day

#define HD_BU_DATA_REDUNDANCY_DEFAULT     ((uint8_t)1)
#define HD_BU_DATA_REDUNDANCY_MAX         ((uint8_t)3)

#define HD_BU_MAX_TIME_DIVERSITY_DEFAULT  ((uint8_t)120)
#define HD_BU_MAX_TIME_DIVERSITY_MAX      ((uint8_t)240)

#define HD_BU_TRAFFIC_CLASS_DEFAULT       ((uint8_t)32)
#define HD_BU_TRAFFIC_CLASS_MAX           ((uint8_t)63)

#define HD_SELF_READ_TIME_DEFAULT         ((uint32_t)0)
#define HD_SELF_READ_TIME_MAX             ((uint32_t)86399)

#define HD_SLOT_OFFSET(x) (x * HD_SLOT_SIZE)
//this macro assumes one quality code at most
#define HD_MAX_BYTES_PER_ALL_CHANNELS  (((uint16_t)sizeof(MeterRead) * HD_TOTAL_CHANNELS)\
                                       + ( (HD_TOTAL_CHANNELS/READING_QTY_MAX + 1 ) * VALUES_INTERVAL_END_SIZE\
                                       + ( (HD_TOTAL_CHANNELS/READING_QTY_MAX + 1 ) * EVENT_AND_READING_QTY_SIZE)))




//Macros to access one of 32 bits indicating if an error was encountered when performing a read for the channel
#define HD_CHAN_ERROR_GET(x) ( ( hdFileData_.Data.histDataChErrors  &  ((uint64_t) 1 << x) ) ? (bool)true : (bool)false )
#define HD_CHAN_ERROR_SET(x)   ( hdFileData_.Data.histDataChErrors |=  ((uint64_t) 1 << x) )



/* TYPE DEFINITIONS */
typedef struct
{
   uint8_t  histDayIndex;                                         //Current index
   /*lint -e{658}  anonymous unions allowed  */
   union
   {
      meterProgInfo_t   lastProgInfo;             //Most recent meter programmed information (I210+C, KV2c)
      uint8_t           flags;                    //Meter readings reset (zeroed) (I210+)
   };
}hdFileIndex_t;

typedef struct
{
  uint32_t CRC;
  struct{
    uint32_t histShiftDateTime;                   //Date and time of the crossing (in seconds)
    uint64_t histDataChErrors;                    //Bit field for each channel for error encounterd when reading
    int64_t  histData[HD_TOTAL_CHANNELS];         //Store all signals for each crossing
    uint32_t histDateTime[HD_TOTAL_CHANNELS];     //Date and time of all timestamped capable channels
  }Data;
}hdFileData_t;

typedef struct
{
   uint8_t dsBuDataRedundancy;
   uint8_t dsBuMaxTimeDiversity;
   uint8_t dsBuTrafficClass;
   bool dsBuEnabled;     /* is daily shifted data allowed to bubble up */
   uint32_t dailySelfReadTime;
   meterReadingType dsBuReadingTypes[HD_TOTAL_CHANNELS];
}hdParams_t;  //Source for each channel

/* FILE VARIABLE DEFINITIONS */
static FileHandle_t           hdFileHndlIndex_;             //Contains the file handle information
static FileHandle_t           hdParamsFileHndl_;            //Contains the file handle information
static hdFileData_t           hdFileData_;                  //Local copy of the file
static hdFileIndex_t          hdFileIndex_;                 //Local copy of the file
static hdParams_t             hdParams_;                    //Local copy of the file
static PartitionData_t const  *pHdPartitions_;              //Pointer to partition that contains data array
static OS_MSGQ_Obj            HD_MsgQ_;                     //Message queue for HD
static OS_MUTEX_Obj           HD_Mutex_;
static uint16_t               bubbleUpTimerId = 0;          //Timer ID used by time diversity.
static uint8_t                dsBuDataRedundancyLocal = 0;
static uint8_t                alarmId_ = NO_ALARM_FOUND;    //Alarm ID

/* CONSTANTS */
static const bool historicalRecoverySupported = (bool)true; //Indicates whether endpoint supports historical recovery data
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
static const uint8_t zeros[ 5 ] = { 0, 0, 0, 0, 0 };  /* Used for checking for valid cal/prog dates   */
#endif

/* FUNCTION PROTOTYPES */
static returnStatus_t   HD_SetBubbleUpTimer(uint32_t timeMilliseconds);
static void             HD_bubbleUpTimerExpired(uint8_t cmd, void *pData);
static void             HD_checkMeterProgInfo(void);
static uint16_t         addAllDailyShiftChannels(uint8_t *startBuf);
static void             addDailyAlarm(void);

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function name: HD_init

   Purpose: Adds registers to the Object list and opens files.  This must be called at power before any other function
            in this module is called and before the register module is initialized.

   Arguments: None

   Returns: returnStatus_t - result of file open.

   Side Effects: None

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
returnStatus_t HD_init( void )
{
   FileStatus_t fileStatus;  //Indicates if the file is created or already exists
   returnStatus_t retVal = eFAILURE;

   if ( !(OS_MSGQ_Create(&HD_MsgQ_) && (OS_MUTEX_Create(&HD_Mutex_))) )
   {
      return eFAILURE;
   }

   //Open file containing Index and meter program info
   if ( eSUCCESS == FIO_fopen(&hdFileHndlIndex_,           /* File Handle */
                              ePART_SEARCH_BY_TIMING,      /* Open file in the given partition */
                              (uint16_t)eFN_HD_INDEX,      /* File ID (filename) */
                              (lCnt)sizeof(hdFileIndex_t), /* Size of the data in the file */
                              0,                           /* File attributes to use */
                              HD_INDEX_FILE_UPDATE_RATE,   /* Ignore the update rate as it is going in special partition */
                              &fileStatus) )
   {
      if (fileStatus.bFileCreated)
      {  // the file was created
         hdFileIndex_.histDayIndex = HD_NUM_OF_HISTORY_DAYS - 1; //Start at the first slot on virgin power-up
         (void)memset( (uint8_t *)&hdFileIndex_.lastProgInfo, 0, sizeof( meterProgInfo_t ) );
         retVal = FIO_fwrite(&hdFileHndlIndex_,
                             (fileOffset)offsetof(hdFileIndex_t, histDayIndex),
                             (uint8_t const *)&hdFileIndex_.histDayIndex,
                             (lCnt)sizeof(hdFileIndex_.histDayIndex)); //save the index
       }
       else
       {
         retVal = FIO_fread(&hdFileHndlIndex_, (uint8_t *)&hdFileIndex_, 0, (lCnt)sizeof(hdFileIndex_t)); //Read the complete file
       }
   }
   if (eSUCCESS == retVal)
   {
      PartitionTbl_t const *pPartFunctions = &PAR_partitionFptr;
      retVal = pPartFunctions->parOpen(&pHdPartitions_, ePART_HD, (uint32_t)0);
   }

   if (retVal != eSUCCESS)
   {
      return eFAILURE;
   }

   if ( eSUCCESS == FIO_fopen(&hdParamsFileHndl_,          /* File Handle */
                              ePART_SEARCH_BY_TIMING,      /* Search for best partition according to the timing.*/
                              (uint16_t)eFN_HD_FILE_INFO,  /* File ID (filename) */
                              (lCnt)sizeof(hdParams_t),    /* Size of the data in the file */
                              0,                           /* File attributes to use */
                              HD_FILE_UPDATE_RATE_SRC_INFO,/* Update rate of this file */
                              &fileStatus) )
   {
      if ( fileStatus.bFileCreated )
      {  // the file was created
         (void)memset(&hdParams_, 0, sizeof(hdParams_));
         hdParams_.dsBuDataRedundancy = HD_BU_DATA_REDUNDANCY_DEFAULT;
         hdParams_.dsBuMaxTimeDiversity = HD_BU_MAX_TIME_DIVERSITY_DEFAULT;
         hdParams_.dsBuTrafficClass = HD_BU_TRAFFIC_CLASS_DEFAULT;
         hdParams_.dailySelfReadTime = HD_SELF_READ_TIME_DEFAULT;
         hdParams_.dsBuEnabled = true;
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
         /*TODO:  SMG: Update list for correct default settings
                  dailyMaxFwdDmdKW is compiled out for I210+C because its not available in all the demand modes */
         hdParams_.dsBuReadingTypes[0] = fwdkWh;
         hdParams_.dsBuReadingTypes[1] = revkWh;
         hdParams_.dsBuReadingTypes[2] = netkWh;
         hdParams_.dsBuReadingTypes[3] = switchPositionStatus;
         hdParams_.dsBuReadingTypes[4] = powerQuality;
#if ( HMC_I210_PLUS == 1 )
         hdParams_.dsBuReadingTypes[5] = dailyMaxFwdDmdKW;
#endif
         hdParams_.dsBuReadingTypes[6] = demandResetCount;
         hdParams_.dsBuReadingTypes[7] = maxPrevFwdDmdKW;
#else
         hdParams_.dsBuReadingTypes[0] = fwdkWh;
         hdParams_.dsBuReadingTypes[1] = revkWh;
         hdParams_.dsBuReadingTypes[2] = fwdkVArh;
         hdParams_.dsBuReadingTypes[3] = totkVAh;
         hdParams_.dsBuReadingTypes[4] = powerQuality;
         hdParams_.dsBuReadingTypes[5] = maxPresFwdDmdKW;
         hdParams_.dsBuReadingTypes[6] = demandResetCount;
         hdParams_.dsBuReadingTypes[7] = maxPrevFwdDmdKW;
#endif
         retVal = FIO_fwrite(&hdParamsFileHndl_, 0, (uint8_t const *)&hdParams_, (lCnt)sizeof(hdParams_t)); //save the reading types
      }
      else
      {  //Read the file
         retVal = FIO_fread(&hdParamsFileHndl_, (uint8_t *)&hdParams_, 0, (lCnt)sizeof(hdParams_t)); //Read the complete file
      }
   }
   return retVal;
}

/***********************************************************************************************************************

   Function Name: addDailyAlarm

   Purpose: Subscribes to daily alarm at daily shift time. If already subscribed, delete the current alarm and
            re-subscribes (need if the daily read  time changes)

   Arguments: None

   Side Effects: none

   Re-entrant: No

   Returns: None

   Notes: None

 ***********************************************************************************************************************/
static void addDailyAlarm ( void )
{
   tTimeSysPerAlarm   alarmSettings;    // Configure the periodic alarm for time

   if ( alarmId_ != NO_ALARM_FOUND )
   {  //Already subscribed, delete current alarm
      (void)TIME_SYS_DeleteAlarm ( alarmId_ );
      alarmId_ = NO_ALARM_FOUND;
   }
   //Set up a periodic alarm
   alarmSettings.bOnInvalidTime = false;                 /* Only alarmed on valid time, not invalid */
   alarmSettings.bOnValidTime = true;                    /* Alarmed on valid time */
   alarmSettings.bSkipTimeChange = true;                 /* do NOT notify on time change */
   alarmSettings.bUseLocalTime = true;                   /* Use local time */
   alarmSettings.pQueueHandle = NULL;                    /* do NOT use queue */
   alarmSettings.pMQueueHandle = &HD_MsgQ_;              /* Uses the message Queue */
   alarmSettings.ucAlarmId = 0;                          /* 0 will create a new ID, anything else will reconfigure. */
   alarmSettings.ulOffset = hdParams_.dailySelfReadTime * TIME_TICKS_PER_SEC; /* Offset past midnight for daily shift */
   alarmSettings.ulPeriod = TIME_TICKS_PER_DAY;          /* Once per day, at Billing Shift time   */
   (void)TIME_SYS_AddPerAlarm(&alarmSettings);
   alarmId_ = alarmSettings.ucAlarmId;
}

/***********************************************************************************************************************

   Function Name: addAllDailyShiftChannels

   Purpose: Add all the channels readings (in compact meter format).

   Arguments: uint8_t *startBuf: First byte of the buffer for this record

   Side Effects: none

   Re-entrant: No

   Returns: uint16_t: Number of bytes added

   Notes: The data is in hdFileData_ structure

 ***********************************************************************************************************************/
static uint16_t addAllDailyShiftChannels( uint8_t *startBuf )
{
   bool     addQualityCodeSuspect       = false; //true if record CRC did not match calculated
   uint16_t length;            //Total length added
   uint8_t  eventCount;        //Counter for alarms and readings
   pack_t   packCfg;           //Struct needed for PACK functions
   uint8_t  i;
   uint32_t dateTimeLocal = hdFileData_.Data.histShiftDateTime;
   uint32_t crcValue;          //calculated CRC of hdFileData_ excluding CRC itself
   uint16_t indexOfEventCount = VALUES_INTERVAL_END_SIZE;

   PACK_init( (uint8_t*)&startBuf[0], &packCfg ); //init the return buffer
   PACK_Buffer( -(VALUES_INTERVAL_END_SIZE_BITS), (uint8_t *)&dateTimeLocal, &packCfg ); //Add timestamp
   length = VALUES_INTERVAL_END_SIZE;

   //check if calculated CRC matches the record CRC
   crcValue = CRC_32_Calculate( (uint8_t *)&hdFileData_.Data, sizeof(hdFileData_.Data) );
   addQualityCodeSuspect = ( crcValue == hdFileData_.CRC ) ? false : true;

   //Insert the reading quantity, it will be adjusted later to the correct value
   eventCount = 0;
   PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
   length += sizeof(eventCount);

   for ( i = 0; i < HD_TOTAL_CHANNELS; i++ )
   {  //Get the values of each channel
      uint8_t  powerOf10;
      uint8_t  powerOf10Code;
      uint8_t  numberOfBytes;
      bool     addTimestamp;     //Add timestamp to the current reading
      bool     addQualityCodeKMR;        //Add Known Missing Reading Quality Code to the current reading
      uint8_t  RdgInfo;          //Bit field ahead of every reading qty
      uint16_t RdgType;          // Reading identifier
      int64_t  value = hdFileData_.Data.histData[i];

      if ( invalidReadingType == hdParams_.dsBuReadingTypes[i] )
      {  //Not configured, skip this channel
         continue;
      }

      //Since there could be more than 15 entries, check if 15 are already present
      if (15 == eventCount)
      {
         startBuf[indexOfEventCount] = eventCount; //Close the current set
         //start new set
         PACK_Buffer( -(VALUES_INTERVAL_END_SIZE_BITS), (uint8_t *)&dateTimeLocal, &packCfg); //Add timestamp from above
         length += VALUES_INTERVAL_END_SIZE;
         eventCount = 0;
         PACK_Buffer( sizeof(eventCount) * 8, (uint8_t *)&eventCount, &packCfg );
         indexOfEventCount = length;
         length += sizeof(eventCount);
      }

      //if CRC matches then default, No timestamp or QC present, Power Of Ten and Reading Value Size are 0
      //if CRC does not match then add suspect QC
      RdgInfo = (addQualityCodeSuspect) ? COMPACT_MTR_RDG_QC_PRESENT : 0;
      addTimestamp   = (bool)false;
      addQualityCodeKMR = (bool)false;

      //May have a time stamp associated with this channel
      if ( hdFileData_.Data.histDateTime[i] != 0 )
      {  //Valid time stamp, add to the data
         addTimestamp = (bool)true;
         RdgInfo      |= COMPACT_MTR_RDG_TS_PRESENT; //Time stamp present
      }

      if ( HD_CHAN_ERROR_GET(i) )
      {  //Error during reading, add QC to the message
         addQualityCodeKMR = (bool)true;
         RdgInfo       |= COMPACT_MTR_RDG_QC_PRESENT; //Quality Code present
      }
      else
      {  // acquisition of reading was successful
         if( INTF_CIM_CMD_errorCodeQCRequired( hdParams_.dsBuReadingTypes[i], value) )
         {  // is the reading value an informational code?  If so, we need set the quality code present bit
            RdgInfo |= COMPACT_MTR_RDG_QC_PRESENT;
         }
      }

      //Search the mutually exclusive Summation and Demand UOM lists for the reading type to find the "base" type
#if ( ENABLE_DEMAND_TASKS == 1 )
#if ( DEMAND_IN_METER == 1 )
      if ( demandResetCount == hdParams_.dsBuReadingTypes[i] )
#else // Demand in the Module
      if (  dailyMaxFwdDmdKW == hdParams_.dsBuReadingTypes[i] ||
            maxPresFwdDmdKW  == hdParams_.dsBuReadingTypes[i] ||
            maxPrevFwdDmdKW  == hdParams_.dsBuReadingTypes[i] ||
            demandResetCount == hdParams_.dsBuReadingTypes[i] )
#endif
      {  //Special handling for Demand Readings
         //Just need to get the correct powerOfTen
         ValueInfo_t readingInfo; // variable to retrieve information for reading
         (void)DEMAND_GetDmdValue (hdParams_.dsBuReadingTypes[i], &readingInfo);
         powerOf10 = readingInfo.power10;
      }
      else
#endif // #if ( ENABLE_DEMAND_TASKS == 1 )
      {
#if( ANSI_STANDARD_TABLES == 1 )
         powerOf10 = HMC_MTRINFO_getMeterReadingPowerOfTen( hdParams_.dsBuReadingTypes[i] );
#else
         quantitySig_t *pSig;

         powerOf10 = 0; //Default
         if (eSUCCESS == OL_Search(OL_eHMC_QUANT_SIG, (ELEMENT_ID)uom, (void **)&pSig))
         {
            if (eANSI_MTR_LOOKUP == pSig->eAccess)
            {
               powerOf10 = (uint8_t)pSig->sig.multCfg.multiplierOveride;
            }
            else if (eANSI_DIRECT == pSig->eAccess)
            {  //For direct reads, use multiplier12 to convert to business values
               powerOf10 = (uint8_t)pSig->tbl.multiplier12;
            }
         }
#endif
      }

      ++eventCount;
      powerOf10Code = HEEP_getPowerOf10Code(powerOf10, &value);
      // TODO: intValue is technically wrong. The person configurating reading types should alway use integer readings
      // but there is no requirement or filter. HEEP_getMinByteNeeded() expects only integer typecasts
      // and will return a length of 0 bytes with other typecasts.
      numberOfBytes = HEEP_getMinByteNeeded(value, intValue, 0);

      INFO_printf( "DS Ch=%2d, index=%d, val=%lld, power=%d, NumBytes=%d", i, hdFileIndex_.histDayIndex, value, powerOf10Code, numberOfBytes);
      RdgInfo |= (uint8_t)(powerOf10Code << COMPACT_MTR_POWER10_SHIFT);
      RdgInfo |= (uint8_t)((uint8_t)(numberOfBytes -1) << COMPACT_MTR_RDG_SIZE_SHIFT); // Insert the reading info bit field
      PACK_Buffer(sizeof(RdgInfo) * 8, &RdgInfo, &packCfg);
      length += sizeof(RdgInfo);

      RdgType = (uint16_t)hdParams_.dsBuReadingTypes[i];
      PACK_Buffer(-(int16_t)(sizeof(RdgType) * 8), (uint8_t *)&RdgType, &packCfg);
      length += sizeof(RdgType);

      if ( addTimestamp )
      {  //Add timestamp here
         uint32_t lDateTime = hdFileData_.Data.histDateTime[i];
         PACK_Buffer( -(VALUES_INTERVAL_END_SIZE_BITS), (uint8_t *)&lDateTime, &packCfg );
         length += VALUES_INTERVAL_END_SIZE;
      }



      if ( addQualityCodeKMR )
      {  //Add Quality Code here (only one valid code)
        enum_CIM_QualityCode qualCode = CIM_QUALCODE_KNOWN_MISSING_READ;
        qualCode |= ( addQualityCodeSuspect ) ?  0x80 : 0x00; //set MSB, additional QC if suspect data
        PACK_Buffer( READING_QUALITY_SIZE_BITS, (uint8_t *)&qualCode, &packCfg );
        length += READING_QUALITY_SIZE;
      }
      else
      {  // acquisition of reading was successful
        if( INTF_CIM_CMD_errorCodeQCRequired( hdParams_.dsBuReadingTypes[i], value) )
        {  // is the reading an informational code?  If so, we need to pack a quality code in the message
          enum_CIM_QualityCode qualCode = CIM_QUALCODE_ERROR_CODE;
          qualCode |= ( addQualityCodeSuspect ) ?  0x80 : 0x00; //set MSB, additional QC if suspect data
          PACK_Buffer( READING_QUALITY_SIZE_BITS, (uint8_t *)&qualCode, &packCfg );
          length += READING_QUALITY_SIZE;
        }
      }

      if( addQualityCodeSuspect )
      {
        enum_CIM_QualityCode qualCode = CIM_QUALCODE_SUSPECT;
        PACK_Buffer( READING_QUALITY_SIZE_BITS, (uint8_t *)&qualCode, &packCfg );
        length += READING_QUALITY_SIZE;
      }

      //get the correct number of bytes
      PACK_Buffer(-(numberOfBytes * 8), (uint8_t *)&value, &packCfg);
      length += numberOfBytes;
   }
   //Insert the correct number of readings
   startBuf[indexOfEventCount] = eventCount;

   return length;
}

/***********************************************************************************************************************

   Function Name: HD_MsgHandler

   Purpose: Handle OR_DS (On-request readings)

   Arguments:  APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
               void *payloadBuf           - pointer to data payload
               length                     - payload length

   Side Effects: none

   Re-entrant: No

   Returns: none

   Notes:

 ***********************************************************************************************************************/
//lint -efunc(818, HD_MsgHandler) arguments cannot be const because of API requirements
void HD_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length )
{
   (void)length;
   if ( ( method_get == (enum_MessageMethod)heepReqHdr->Method_Status ) && ( MODECFG_get_rfTest_mode() == 0 ) )
   { //Process the get request

      //Prepare Compact Meter Read structure
      HEEP_APPHDR_t heepRespHdr;          // Application header/QOS info
      buffer_t      *pBuf;                // pointer to message
      uint16_t      bits=0;               // Keeps track of the bit position within the Bitfields
      union TimeSchRdgQty_u qtys;         // For EventTypeQty and ReadingTypeQty

      OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails

      //Application header/QOS info
      HEEP_initHeader( &heepRespHdr );
      heepRespHdr.TransactionType = (uint8_t)TT_RESPONSE;
      heepRespHdr.Resource = (uint8_t)or_ds;
      heepRespHdr.Method_Status = (uint8_t)OK;
      heepRespHdr.Req_Resp_ID = (uint8_t)heepReqHdr->Req_Resp_ID;
      heepRespHdr.qos = (uint8_t)heepReqHdr->qos;
      heepRespHdr.callback = NULL;
      heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;


      pBuf = BM_alloc(APP_MSG_MAX_DATA); //Allocate max buffer
      if ( pBuf != NULL )
      {
         uint8_t i,j;

         pBuf->x.dataLen = 0;
         qtys.TimeSchRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS); //Parse the fields passed in

         /* Process each time pairs, ignore the reading quantities (return all the channels)  */
         for (i=0; i< qtys.bits.timeSchQty; i++)
         {
            uint32_t curSlotTime;  //Date time at current index
            uint32_t nextSlotTime; //Date time at next index
            uint8_t index;         //Slot index
            uint32_t startTime = UNPACK_bits2_uint32(payloadBuf, &bits, VALUES_INTERVAL_START_SIZE_BITS);
            uint32_t endTime = UNPACK_bits2_uint32(payloadBuf, &bits, VALUES_INTERVAL_END_SIZE_BITS);

            if (startTime >= endTime)
            { //Bad request
               heepRespHdr.Method_Status = (uint8_t)BadRequest;
               break;
            }

            //Read the date and time of the shift
            index = (hdFileIndex_.histDayIndex + 1U) % HD_NUM_OF_HISTORY_DAYS; //Start at oldest entry
            (void)PAR_partitionFptr.parRead((uint8_t *)&nextSlotTime,
                                       ( HD_SLOT_OFFSET(index) + offsetof(hdFileData_t, Data.histShiftDateTime) ),
                                       sizeof(nextSlotTime),
                                       pHdPartitions_);

            //Check the Daily shift time of all entries and send that match date criteria
            for (j=0; j< HD_NUM_OF_HISTORY_DAYS; j++)
            {
               uint8_t nextIndex = (index + 1U) % HD_NUM_OF_HISTORY_DAYS; //get the next entry

               curSlotTime = nextSlotTime;
               (void)PAR_partitionFptr.parRead((uint8_t *)&nextSlotTime,
                                               ( HD_SLOT_OFFSET(nextIndex) + offsetof(hdFileData_t, Data.histShiftDateTime) ),
                                               sizeof(nextSlotTime),
                                               pHdPartitions_);

               if ( (0 == curSlotTime) || (curSlotTime == nextSlotTime) )
               {  //If invalid date/time or If both the entries are for same date/time, ignore the oldest entry
                  index = (index + 1U) % HD_NUM_OF_HISTORY_DAYS; //next entry
                  continue;
               }

               if(curSlotTime >= startTime && curSlotTime < endTime)
               { //Entry matched, send all channels for this date
                  if ((APP_MSG_MAX_DATA - pBuf->x.dataLen) < HD_MAX_BYTES_PER_ALL_CHANNELS)
                  {  //limit the number of entries to avoid buffer overrun
                     heepRespHdr.Method_Status = (uint8_t)PartialContent;
                     break;
                  }
                  //Read the record
                  (void)PAR_partitionFptr.parRead((uint8_t *)&hdFileData_, HD_SLOT_OFFSET(index), sizeof(hdFileData_), pHdPartitions_);
                  pBuf->x.dataLen += addAllDailyShiftChannels(&pBuf->data[pBuf->x.dataLen]);
               }
               index = (index + 1U) % HD_NUM_OF_HISTORY_DAYS; //next entry
            }
         }

         if ( heepRespHdr.Method_Status == (uint8_t)BadRequest || pBuf->x.dataLen == 0 )
         {  //Just send the header
            (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
            INFO_printf("DS No matching Date/Time or Bad request");
            BM_free(pBuf);
         }
         else
         {  //send the message to message handler . The called function will free the buffer
            (void)HEEP_MSG_Tx (&heepRespHdr, pBuf);
         }
      }
      else
      {  //No buffer available
         heepRespHdr.Method_Status = (uint8_t)ServiceUnavailable;
         (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
         ERR_printf("ERROR: DS No buffer available");
      }
      OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
   }  /*lint !e456 Mutex unlocked if successfully locked */
}  /*lint !e454 !e456 Mutex unlocked if successfully locked */

/*lint -esym(715,Arg0) */
/***********************************************************************************************************************

   Function name: HD_DailyShiftTask

   Purpose: This task will wait for a signal from time module and then perform historical data shift and sends
            bubble up daily shift message

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: None

   Side Effects: None

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void HD_DailyShiftTask ( uint32_t Arg0 )
{
   tTimeSysMsg        timeMsg;          //Stores the time message
   uint32_t           msgTime;          // Time converted to seconds since epoch
   uint32_t           uDatePendingMessage = 0;
   uint32_t           uTimePendingMessage = 0;
   uint32_t           uTimeDiversity;
   uint16_t           i;                // Loop counter
   HEEP_APPHDR_t      heepHdr;          // Application header/QOS info
   buffer_t           *pBuf;            // pointer to message
   buffer_t*          pMessage = NULL;
   sysTimeCombined_t  currentTime;      // Current date and time
   sysTime_t          utcTime;          // Current UTC time

   addDailyAlarm(); //Add the daily alarm to perform daily shift.

   //Get local time to check if shift is needed on power-up
   if ( TIME_SYS_GetSysDateTime(&utcTime) )
   {  //Time is valid, check if need to perform daily shift
      sysTime_t shiftTimeUTC;     //When shift takes place
      uint32_t  lastShiftTime;    //Date time of last daily shift
      sysTimeCombined_t lastShiftComTime;  //Date time of last daily shift
      sysTime_t lastShiftSysTime; //Date time of last daily shift
      sysTime_dateFormat_t sDT;

      //Get the time of the day for daily shift (in UTC)
      shiftTimeUTC.date = utcTime.date; //Does not matter, only interested in time. Any non-zero day will be OK
      shiftTimeUTC.time = hdParams_.dailySelfReadTime * TIME_TICKS_PER_SEC; /* Offset past midnight for daily shift */
      DST_ConvertLocaltoUTC(&shiftTimeUTC);

      //Read the date and time of the last shift
      (void)PAR_partitionFptr.parRead((uint8_t *)&lastShiftTime,
                                      ( HD_SLOT_OFFSET(hdFileIndex_.histDayIndex) + offsetof(hdFileData_t, Data.histShiftDateTime)),
                                      sizeof(lastShiftTime),
                                      pHdPartitions_);
      lastShiftComTime = (sysTimeCombined_t)lastShiftTime * TIME_TICKS_PER_SEC;
      TIME_UTIL_ConvertSysCombinedToSysFormat((sysTimeCombined_t const *)&lastShiftComTime, &lastShiftSysTime);

      (void)TIME_UTIL_ConvertSysFormatToDateFormat((sysTime_t const *)&utcTime, &sDT);
      /*lint -e(123) 'min' is a struct element, not a macro */
      INFO_printf("DS Current time            date=%lu time=%lu, hr=%u min=%u sec=%u", utcTime.date, utcTime.time, sDT.hour, sDT.min, sDT.sec);

      (void)TIME_UTIL_ConvertSysFormatToDateFormat((sysTime_t const *)&shiftTimeUTC, &sDT);
      /*lint -e(123) 'min' is a struct element, not a macro */
      INFO_printf("DS Scheduled shift Time    date=%lu time=%lu, hr=%u min=%u sec=%u", shiftTimeUTC.date, shiftTimeUTC.time, sDT.hour, sDT.min, sDT.sec);

      (void)TIME_UTIL_ConvertSysFormatToDateFormat((sysTime_t const *)&lastShiftSysTime, &sDT);
      /*lint -e(123) 'min' is a struct element, not a macro */
      INFO_printf("DS Last daily shift time   date=%lu time=%lu, hr=%u min=%u sec=%u", lastShiftSysTime.date, lastShiftSysTime.time, sDT.hour, sDT.min, sDT.sec);

      if ( !(  (lastShiftSysTime.date == 0) || //no shift yet, wait for the shift time
               (lastShiftSysTime.date == utcTime.date) || //Already shifted today
               ((lastShiftSysTime.date == (utcTime.date - 1)) && (utcTime.time < shiftTimeUTC.time)) ) ) //Wait for the shift time
      {
         tTimeSysMsg    alarmMsg;            /* Alarm Message */

         (void)memset(&alarmMsg, 0, sizeof(alarmMsg));
         INFO_printf("**Performing Daily Shift on power-up**");
         alarmMsg.bSystemTime  = (bool)true;
         alarmMsg.date         = utcTime.date;
         alarmMsg.time         = utcTime.time;

         pBuf = BM_alloc(sizeof(alarmMsg)); //Get a buffer
         if (NULL != pBuf) // Is the buffer valid?
         {
            pBuf->eSysFmt = eSYSFMT_TIME;
            (void)memcpy(&pBuf->data[0], (uint8_t *)&alarmMsg, sizeof(alarmMsg));
            OS_MSGQ_Post(&HD_MsgQ_, pBuf); // Function will not return if it fails
         }
         else
         {
            INFO_printf("**ERROR** No buffer available");
         }
      }
   }

#if ( LP_IN_METER == 1 )
   OS_TASK_Sleep( ONE_SEC * 20 ); //Wait for HMC to be ready as this is no longer gated by LP module
#endif

   for ( ;; )
   {
      /* Wait for a message from the system time denoting daily shift time has occurred   */
      (void)OS_MSGQ_Pend( &HD_MsgQ_, (void *)&pBuf, OS_WAIT_FOREVER );
      if (eSYSFMT_TIME_DIVERSITY == pBuf->eSysFmt)
      {
         // Time diveristy timer expired, send pending message.
         // Send the message to message handler. The called function will free the buffer

         if (NULL != pMessage)
         {
            if ( dsBuDataRedundancyLocal > 0 )
            {  //Repeats left, make a copy and send it
               HEEP_APPHDR_t heepHdrCopy; // Application header/QOS info
               buffer_t      *pMessageCopy;   // pointer to message

               dsBuDataRedundancyLocal--;
               pMessageCopy = BM_alloc(pMessage->x.dataLen);
               if ( pMessageCopy != NULL )
               {
                  (void)memcpy(&pMessageCopy->data[0], &pMessage->data[0], pMessage->x.dataLen);
                  pMessageCopy->x.dataLen = pMessage->x.dataLen;

                  //Set the header again, if something changed
                  HEEP_initHeader( &heepHdrCopy );
                  heepHdrCopy.TransactionType = heepHdr.TransactionType; /*lint !e530 heepHdr initialized if pMessage not null */
                  heepHdrCopy.Resource = heepHdr.Resource;
                  heepHdrCopy.Method_Status = heepHdr.Method_Status;
                  heepHdrCopy.Req_Resp_ID = heepHdr.Req_Resp_ID;
                  heepHdrCopy.qos = heepHdr.qos;
                  heepHdrCopy.callback = heepHdr.callback;
                  heepHdrCopy.TransactionContext = heepHdr.TransactionContext;
                  (void)HEEP_MSG_Tx (&heepHdrCopy, pMessageCopy);
               }
               else
               {  //No buffer availabe, abort additional repeats
                  ERR_printf("DS No buffer available for repeats");
               }
               //Set a new timer
               uTimeDiversity = (uint32_t)hdParams_.dsBuMaxTimeDiversity * TIME_TICKS_PER_MIN;
               INFO_printf("DS Time Diversity: %u Milliseconds", uTimeDiversity);
               (void)HD_SetBubbleUpTimer(uTimeDiversity);
            }
            else
            {  //No more repeat left, send the message
               (void)HEEP_MSG_Tx(&heepHdr, pMessage); /*lint !e603 heepHdr initialized if pMessage not null; see below  */
               pMessage = NULL;
            }
         }
      }

      else
      {
         (void)memcpy((uint8_t *)&timeMsg, &pBuf->data[0], sizeof(timeMsg)); //lint !e740
         BM_free(pBuf); //Free the buffer, time message is not used by the daily shift

         // If a previous message is pending due to time diversity, go ahead and send it.
         if (NULL != pMessage)
         {
            // Make sure that the message pending because of time diversity is older than the message
            // which is about to be generated.  This could happen because of a backwards time jump.
            if (  (uDatePendingMessage < timeMsg.date) ||
                  ((uDatePendingMessage == timeMsg.date) && (uTimePendingMessage < timeMsg.time)))
            {
               (void)HEEP_MSG_Tx(&heepHdr, pMessage);
               pMessage = NULL;
            }
            // Else the pending message is not older than the new message about to be generated.  Just
            // discard it.
            else
            {
               BM_free(pMessage);
               pMessage = NULL;
            }
         }

         while ( ID_isModuleBusy() || DEMAND_isModuleBusy() )
         {  //Load profile or Demand busy, wait for them to be done
            OS_TASK_Sleep(50);
         }
         //Perform daily shift
         OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails

         //Clear the data structure
         (void)memset(&hdFileData_, 0, sizeof(hdFileData_));
         hdFileIndex_.histDayIndex = (hdFileIndex_.histDayIndex + 1U) % HD_NUM_OF_HISTORY_DAYS; //Point to the next entry in the circular buffer

         //Add current timestamp
         (void)TIME_UTIL_GetTimeInSysCombined(&currentTime); //The time is valid, ignore the return value
         msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
         hdFileData_.Data.histShiftDateTime = msgTime; //Store the date and time of the shift

         ValueInfo_t  reading;
         for ( i = 0; i < HD_TOTAL_CHANNELS; i++ )
         {  //Get the values of each channel
            enum_CIM_QualityCode retVal;
            bool addTimestamp;   //Add timestamp to the current reading

            if ( invalidReadingType == hdParams_.dsBuReadingTypes[i] )
            {  //Not configured, skip this channel
               continue;
            }
#if ( DEMAND_IN_METER == 0 )   // Only for Demand in the Module
            if ( dailyMaxFwdDmdKW == hdParams_.dsBuReadingTypes[i] )
            {  //Special handling Daily Max demand
               peak_t demandVal;    //Demand and associated timestamp

               retVal = DEMAND_Shift_Daily_Peak(&demandVal); //Shift and get the shifted value
               value = (int64_t)demandVal.energy;
               msgTime = (uint32_t)(demandVal.dateTime / TIME_TICKS_PER_SEC);
               addTimestamp = (bool)true;
            }
#endif
            else
            {
               uint32_t       fracSec;

               retVal = INTF_CIM_CMD_getMeterReading( hdParams_.dsBuReadingTypes[i], &reading );
               addTimestamp = (bool)false;
               TIME_UTIL_ConvertSysFormatToSeconds( ( sysTime_t * )&reading.timeStamp, &msgTime, &fracSec);
               if ( 0 != msgTime )
               {
                  addTimestamp = (bool)true;
               }
            }
            if ( CIM_QUALCODE_SUCCESS == retVal )
            {
               hdFileData_.Data.histData[i] = reading.Value.intValue; //Save the data
               INFO_printf( "DS Ch=%2d, index=%d, val=%lld", i, hdFileIndex_.histDayIndex, reading.Value.intValue);

               if ( addTimestamp )
               {  //Add timestamp here
                  hdFileData_.Data.histDateTime[i] = msgTime; //Store the date and time of max daily demand
               }
            }
            else
            {
               HD_CHAN_ERROR_SET(i); //Indicate the reading had an error
               INFO_printf( "DS Error getting value Ch=%2d, index=%d, error code=%u", i, hdFileIndex_.histDayIndex, retVal);
            }
         } //end for loop, all reading done
         hdFileData_.CRC = CRC_32_Calculate( (uint8_t *)&hdFileData_.Data, sizeof(hdFileData_.Data)); //do not include the CRC member

         //store the new index and save the data
         (void)FIO_fwrite( &hdFileHndlIndex_,
                           (fileOffset)offsetof(hdFileIndex_t, histDayIndex),
                           (uint8_t const *)&hdFileIndex_.histDayIndex,
                           (lCnt)sizeof(hdFileIndex_.histDayIndex)); //save the index
         (void)PAR_partitionFptr.parWrite(HD_SLOT_OFFSET(hdFileIndex_.histDayIndex), (uint8_t *)&hdFileData_, sizeof(hdFileData_), pHdPartitions_);


         if ( hdParams_.dsBuEnabled ) // is daily shifted bubble up enabled?
         {
            //Bubble up daily data
            HEEP_initHeader( &heepHdr );
            heepHdr.TransactionType = (uint8_t)TT_ASYNC;
            heepHdr.Resource = (uint8_t)bu_ds;
            heepHdr.Method_Status = (uint8_t)method_post;
            heepHdr.Req_Resp_ID = 0;
            heepHdr.qos = hdParams_.dsBuTrafficClass;
            heepHdr.callback = NULL;

            pBuf = BM_alloc(HD_MAX_BYTES_PER_ALL_CHANNELS);
            if ( pBuf != NULL )
            {
               pBuf->x.dataLen = addAllDailyShiftChannels(&pBuf->data[0]);

               dsBuDataRedundancyLocal = hdParams_.dsBuDataRedundancy;
               // Delay for a random amount of time based on time diversity.
               if (hdParams_.dsBuMaxTimeDiversity > 0)
               {
                  uTimeDiversity = aclara_randu(0, ((uint32_t)hdParams_.dsBuMaxTimeDiversity * TIME_TICKS_PER_MIN));
//                uTimeDiversity = 1000;
                  INFO_printf("DS Time Diversity: %u Milliseconds", uTimeDiversity);
                  (void)HD_SetBubbleUpTimer(uTimeDiversity);

                  // Save the date and time from the alarm of the pending message.  This is needed to
                  // determine if the message needs to be discarded in case of a time jump.
                  uDatePendingMessage = timeMsg.date;
                  uTimePendingMessage = timeMsg.time;
                  pMessage = pBuf;
               }
               else
               { //send the message to message handler. The called function will free the buffer
                  HEEP_APPHDR_t heepHdrCopy; // Application header/QOS info
                  buffer_t      *pBufCopy;   // pointer to message

                  while ( dsBuDataRedundancyLocal > 0 )
                  {
                     dsBuDataRedundancyLocal--;
                     pBufCopy = BM_alloc(pBuf->x.dataLen);
                     if ( pBufCopy != NULL )
                     {
                        (void)memcpy(&pBufCopy->data[0], &pBuf->data[0], pBuf->x.dataLen);
                        pBufCopy->x.dataLen = pBuf->x.dataLen;

                        //Set the header again, if something changed
                        HEEP_initHeader( &heepHdrCopy );
                        heepHdrCopy.TransactionType = heepHdr.TransactionType;
                        heepHdrCopy.Resource = heepHdr.Resource;
                        heepHdrCopy.Method_Status = heepHdr.Method_Status;
                        heepHdrCopy.Req_Resp_ID = heepHdr.Req_Resp_ID;
                        heepHdrCopy.qos = heepHdr.qos;
                        heepHdrCopy.callback = heepHdr.callback;
                        heepHdrCopy.TransactionContext = heepHdr.TransactionContext;
                        (void)HEEP_MSG_Tx (&heepHdrCopy, pBufCopy);
                     }
                     else
                     {  //No buffer availabe, abort additional repeats
                        dsBuDataRedundancyLocal = 0;
                        ERR_printf("DS No buffer available for repeats");
                     }
                  }
                  (void)HEEP_MSG_Tx (&heepHdr, pBuf);
                  pBuf = NULL;
               }
            }
         }
         HD_checkMeterProgInfo(); //Check if meter reprogrammed or re-calibrated

         OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails

      }  /*lint !e456 Mutex unlocked if successfully locked */
   }  /*lint !e456 Mutex unlocked if successfully locked */
}
/*lint +esym(715,Arg0) */

/***********************************************************************************************************************

   Function Name: HD_checkMeterProgInfo

   Purpose: For KV2  : Check if meter is reprogrammed or re-calibrated. Log if this happens
            For I210+: Check if meter reset tamper is set

   Arguments: none

   Returns: none

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static void HD_checkMeterProgInfo( void )
{
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   meterProgInfo_t      progInfo;   /* Partial MT78 reading  */
   EventKeyValuePair_s  keyVal;     /* Event key/value pair info  */
   EventData_s          progEvent;  /* Event info  */

   if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterProgramInfo( (uint8_t *)&progInfo ) )
   {
      /* Check for programming date change   */
      if ( memcmp( hdFileIndex_.lastProgInfo.dtLastProgrammed, progInfo.dtLastProgrammed, sizeof( progInfo.dtLastProgrammed ) ) != 0 )
      {
         /* Check for valid last programmed date. It'll only be all zeros if the file was just created */
         if( memcmp( hdFileIndex_.lastProgInfo.dtLastProgrammed, zeros, sizeof( zeros ) ) != 0 )
         {  /* Log event   */
            progEvent.eventId                     = (uint16_t)electricMeterConfigurationProgramChanged;
            *(uint16_t *)(void *)&keyVal.Key[ 0 ] = (uint16_t)meterProgramCount;
            (void)memcpy( keyVal.Value, &progInfo.numberOfTimesProgrammed, sizeof( progInfo.numberOfTimesProgrammed ) );
            progEvent.eventKeyValuePairsCount     = 1;
            progEvent.eventKeyValueSize           = sizeof( progInfo.numberOfTimesProgrammed );
            (void)EVL_LogEvent( 190, &progEvent, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
         }
         /* Update local last program info  */
         (void)memcpy( &hdFileIndex_.lastProgInfo, &progInfo, sizeof( hdFileIndex_.lastProgInfo ) );
      }
      /* Check for calibration date change   */
      if ( memcmp( hdFileIndex_.lastProgInfo.dtLastCalibrated, progInfo.dtLastCalibrated, sizeof( progInfo.dtLastCalibrated ) ) != 0 )
      {
         /* Check for valid last calibrated date. It'll only be all zeros if the file was just created */
         if( memcmp( hdFileIndex_.lastProgInfo.dtLastCalibrated, zeros, sizeof( zeros ) ) != 0 )
         {  /* Log event   */
            progEvent.eventId                 = (uint16_t)electricMeterMetrologyReadingsResetOccurred;
            keyVal.Key[ 0 ]                   = 0;
            keyVal.Value[ 0 ]                 = 0;
            progEvent.eventKeyValuePairsCount = 0;
            progEvent.eventKeyValueSize       = 0;
            (void)EVL_LogEvent( 113, &progEvent, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
         }
         /* Update local last calibration info  */
         (void)memcpy( &hdFileIndex_.lastProgInfo, &progInfo, sizeof( hdFileIndex_.lastProgInfo ) );
      }
   }
#elif HMC_I210_PLUS
   EventKeyValuePair_s  keyVal;     /* Event key/value pair info  */
   EventData_s          progEvent;  /* Event info  */
   uint8_t flags;
   if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getElectricMetermetrologyReadingsResetOccurred( &flags ) )
   {
      if( ( flags & 0x80 ) != 0 )
      {  /* Log event   */
         progEvent.eventId = (uint16_t)electricMeterMetrologyReadingsResetOccurred;
         progEvent.markSent = (bool)false;
         keyVal.Key[ 0 ] = 0;
         keyVal.Value[ 0 ] = 0;
         progEvent.eventKeyValuePairsCount = 0;
         progEvent.eventKeyValueSize = 0;
         (void)EVL_LogEvent( 113, &progEvent, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
      }
      /* Update local last calibration info  */
      (void)memcpy( &hdFileIndex_.flags, &flags, sizeof( hdFileIndex_.flags ) );
   }
#endif
}

/***********************************************************************************************************************

   Function Name: HD_SetBubbleUpTimer(uint32_t timeMilliseconds)

   Purpose: Create the timer to re-send the historical data after the given number
            of milliseconds have elapsed.

   Arguments: uint32_t timeRemainingMSec - The number of milliseconds until the timer goes off.

   Returns: returnStatus_t - indicates if the timer was successfully created

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
static returnStatus_t HD_SetBubbleUpTimer( uint32_t timeMilliseconds )
{
   returnStatus_t retStatus;

   if ( 0 == bubbleUpTimerId )
   {  //First time, add the timer
      timer_t tmrSettings;             // Timer for sending pending ID bubble up message

      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.bOneShot = true;
      tmrSettings.bOneShotDelete = false;
      tmrSettings.bExpired = false;
      tmrSettings.bStopped = false;
      tmrSettings.ulDuration_mS = timeMilliseconds;
      tmrSettings.pFunctCallBack = HD_bubbleUpTimerExpired;
      retStatus = TMR_AddTimer(&tmrSettings);
      if (eSUCCESS == retStatus)
      {
         bubbleUpTimerId = tmrSettings.usiTimerId;
      }
      else
      {
         ERR_printf("DS unable to add bubble up timer");
      }
   }
   else
   {  //Timer already exists, reset the timer
      retStatus = TMR_ResetTimer(bubbleUpTimerId, timeMilliseconds);
   }
   return(retStatus);
}

/***********************************************************************************************************************

   Function Name: HD_bubbleUpTimerExpired(uint8_t cmd, void *pData)

   Purpose: Called by the timer to send the historical data bubble up message.  Time diveristy for historical
            data is implemented in this manner so that in the case of a time jump, it might be necessary to
            either send the message before the time diversity delay has expired or it might be necessary to
            discard the pending message in the case of a backward time jump.
            Since this code is called from within the timer task, we can't muck around with the
            timer, so we need to notify the app message class to do the actual work.

   Arguments:  uint8_t cmd - from the ucCmd member when the timer was created.
               void *pData - from the pData member when the timer was created

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
/*lint -esym(715,818,cmd,pData) */
static void HD_bubbleUpTimerExpired( uint8_t cmd, void *pData )
{
   static buffer_t buf;

   /***
   * Use a static buffer instead of allocating one because of the allocation fails,
   * there wouldn't be much we could do - we couldn't reset the timer to try again
   * because we're already in the timer callback.
   ***/
   BM_AllocStatic(&buf, eSYSFMT_TIME_DIVERSITY);
   OS_MSGQ_Post(&HD_MsgQ_, (void *)&buf);
}/*lint !e715 !e818  parameters are not used, required by API */

/**************************/
/* Get and Set parameters */
/**************************/

/***********************************************************************************************************************

   Function name: HD_getDsBuDataRedundancy

   Purpose: Get dsBuDataRedundancy from Daily Shift configuration

   Arguments: None

   Returns: uint8_t dsBuDataRedundancy_Local

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint8_t HD_getDsBuDataRedundancy( void )
{
   uint8_t dsBuDataRedundancy_Local;
   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   dsBuDataRedundancy_Local = hdParams_.dsBuDataRedundancy;
   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails

   return(dsBuDataRedundancy_Local);
}

/***********************************************************************************************************************

   Function name: HD_setDsBuDataRedundancy

   Purpose: Set dsBuDataRedundancy in Daily Shift configuration

   Arguments: uint8_t dsBuDataRedundancy_Local

   Returns: returnStatus_t

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
returnStatus_t HD_setDsBuDataRedundancy( uint8_t dsBuDataRedundancy_Local )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (dsBuDataRedundancy_Local <= HD_BU_DATA_REDUNDANCY_MAX)
   {
      OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
      hdParams_.dsBuDataRedundancy = dsBuDataRedundancy_Local;
      (void)FIO_fwrite(&hdParamsFileHndl_, 0, (uint8_t const *)&hdParams_, (lCnt)sizeof(hdParams_t)); //save file
      OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
      retVal = eSUCCESS;
   }
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HD_getDsBuMaxTimeDiversity

   Purpose: Get dsBuMaxTimeDiversity from Daily Shift configuration

   Arguments: None

   Returns: uint8_t dsBuMaxTimeDiversity_Local

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint8_t HD_getDsBuMaxTimeDiversity( void )
{
   uint8_t dsBuMaxTimeDiversity_Local;
   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   dsBuMaxTimeDiversity_Local = hdParams_.dsBuMaxTimeDiversity;
   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails

   return(dsBuMaxTimeDiversity_Local);
}

/***********************************************************************************************************************

   Function name: HD_setDsBuMaxTimeDiversity

   Purpose: Set dsBuMaxTimeDiversity in Daily Shift configuration

   Arguments: uint8_t dsBuMaxTimeDiversity_Local

   Returns: returnStatus_t

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
returnStatus_t HD_setDsBuMaxTimeDiversity( uint8_t dsBuMaxTimeDiversity_Local )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (dsBuMaxTimeDiversity_Local <= HD_BU_MAX_TIME_DIVERSITY_MAX)
   {
      OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
      hdParams_.dsBuMaxTimeDiversity = dsBuMaxTimeDiversity_Local;
      (void)FIO_fwrite(&hdParamsFileHndl_, 0, (uint8_t const *)&hdParams_, (lCnt)sizeof(hdParams_t)); //save file
      OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
      retVal = eSUCCESS;
   }
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HD_getDsBuEnabled

   Purpose: Get dsBuBuEnabled from Daily Shift parameters

   Arguments: None

   Returns: bool uDsBuEnabled

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
bool HD_getDsBuEnabled( void )
{
   bool uDsBuEnabled;
   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   uDsBuEnabled = hdParams_.dsBuEnabled;
   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails

   return(uDsBuEnabled);
}

/***********************************************************************************************************************

   Function name: HD_setDsBuEnabled

   Purpose: Set dsBuBuEnabled in Daily Shift parameters

   Arguments: uint8_t dsBuMaxTimeDiversity_Local

   Returns: bool uDsBuEnabled

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
returnStatus_t HD_setDsBuEnabled( uint8_t uDsBuEnabled )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   // Check if we are setting the proper value.  Boolean parameter, either 0 or 1.
   if( uDsBuEnabled <= 1 )
   {
      OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
      hdParams_.dsBuEnabled = uDsBuEnabled;
      (void)FIO_fwrite(&hdParamsFileHndl_, 0, (uint8_t const *)&hdParams_, (lCnt)sizeof(hdParams_t)); //save file
      OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
      retVal = eSUCCESS;
   }
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HD_getDsBuTrafficClass

   Purpose: Get dsBuTrafficClass from Daily Shift configuration

   Arguments: None

   Returns: uint8_t dsBuTrafficClass_Local

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint8_t HD_getDsBuTrafficClass( void )
{
   uint8_t dsBuTrafficClass_Local;
   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   dsBuTrafficClass_Local = hdParams_.dsBuTrafficClass;
   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails

   return(dsBuTrafficClass_Local);
}

/***********************************************************************************************************************

   Function name: HD_setDsBuTrafficClass

   Purpose: Set dsBuTrafficClass in Daily Shift configuration

   Arguments: uint8_t dsBuTrafficClass_Local

   Returns: returnStatus_t

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
returnStatus_t HD_setDsBuTrafficClass( uint8_t dsBuTrafficClass_Local )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (dsBuTrafficClass_Local <= HD_BU_TRAFFIC_CLASS_MAX)
   {
      OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
      hdParams_.dsBuTrafficClass = dsBuTrafficClass_Local;
      (void)FIO_fwrite(&hdParamsFileHndl_, 0, (uint8_t const *)&hdParams_, (lCnt)sizeof(hdParams_t));  //save file
      OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
      retVal = eSUCCESS;
   }
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HD_getDailySelfReadTime

   Purpose: Get dailySelfReadTime from Daily Shift configuration

   Arguments: None

   Returns: uint32_t dailySelfReadTime_Local

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint32_t HD_getDailySelfReadTime( void )
{
   uint32_t dailySelfReadTime_Local;
   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   dailySelfReadTime_Local = hdParams_.dailySelfReadTime;
   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails

   return(dailySelfReadTime_Local);
}

/***********************************************************************************************************************

   Function name: HD_setDailySelfReadTime

   Purpose: Set dailySelfReadTime in Daily Shift configuration

   Arguments: uint32_t dailySelfReadTime_Local

   Returns: returnStatus_t

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
returnStatus_t HD_setDailySelfReadTime( uint32_t dailySelfReadTime_Local )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;
#if (DEMAND_IN_METER == 0 )
   uint8_t dayOfMonth;
#endif
   if (dailySelfReadTime_Local <= HD_SELF_READ_TIME_MAX)
   {
      OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
      hdParams_.dailySelfReadTime = dailySelfReadTime_Local;
      (void)FIO_fwrite(&hdParamsFileHndl_, 0, (uint8_t const *)&hdParams_, (lCnt)sizeof(hdParams_t));  //save file
      addDailyAlarm(); //re-subscribe to daily alarm based on new daily read time
      OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
#if (DEMAND_IN_METER == 0 )
      //Reschdule the demand reset as it is based on dailySelfReadTime
      DEMAND_GetResetDay(&dayOfMonth);
      (void)DEMAND_SetResetDay(dayOfMonth);
#endif
      retVal = eSUCCESS;
   }
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HD_getHistoricalRecovery

   Purpose: Get HistoricalRecovery status from historica data parameters

   Arguments: none

   Returns: bool

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
bool HD_getHistoricalRecovery( void )
{
   return( historicalRecoverySupported );
}

/***********************************************************************************************************************

   Function name: HD_getDsBuReadingTypes

   Purpose: Get dsBuReadingTypes from Daily Shift configuration

   Arguments: uint16_t* pDsBuReadingType

   Returns: None

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void HD_getDsBuReadingTypes( uint16_t* pDsBuReadingType )
{
   uint8_t i; //counter for loop

   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   for (i = 0; i < HD_TOTAL_CHANNELS; ++i)
   {
      pDsBuReadingType[i] = (uint16_t)hdParams_.dsBuReadingTypes[i];
   }
   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
}

/***********************************************************************************************************************

   Function name: HD_setDsBuReadingTypes

   Purpose: Set dsBuReadingTypes in Daily Shift configuration

   Arguments: uint16_t* pDsBuReadingType

   Returns: none

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void HD_setDsBuReadingTypes( uint16_t const * pDsBuReadingType )
{
   uint8_t i;

   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   for (i = 0; i < HD_TOTAL_CHANNELS; ++i)
   {
      hdParams_.dsBuReadingTypes[i] = (meterReadingType)pDsBuReadingType[i];
   }
   (void)FIO_fwrite(&hdParamsFileHndl_, 0, (uint8_t const *)&hdParams_, (lCnt)sizeof(hdParams_t));  //save file

   hdFileIndex_.histDayIndex = HD_NUM_OF_HISTORY_DAYS - 1; //Start at the first slot on virgin power-up
   (void)FIO_fwrite( &hdFileHndlIndex_,
                     (fileOffset)offsetof(hdFileIndex_t, histDayIndex),
                     (uint8_t const *)&hdFileIndex_.histDayIndex,
                     (lCnt)sizeof(hdFileIndex_.histDayIndex)); //save the index
   (void)PAR_partitionFptr.parErase(0, PART_NV_HD_SIZE, pHdPartitions_);   // Erase partition

   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
}


/***********************************************************************************************************************

   Function name: HD_clearHistoricalData

   Purpose: Clear all historical data currently stored in endpoint.

   Arguments: None

   Returns: None

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void HD_clearHistoricalData()
{
   OS_MUTEX_Lock(&HD_Mutex_); // Function will not return if it fails
   hdFileIndex_.histDayIndex = HD_NUM_OF_HISTORY_DAYS - 1; //Start at the first slot
   (void)FIO_fwrite( &hdFileHndlIndex_,
                     (fileOffset)offsetof(hdFileIndex_t, histDayIndex),
                     (uint8_t const *)&hdFileIndex_.histDayIndex,
                     (lCnt)sizeof(hdFileIndex_.histDayIndex)); //save the index
   (void)PAR_partitionFptr.parErase(0, PART_NV_HD_SIZE, pHdPartitions_);   // Erase partition
   OS_MUTEX_Unlock(&HD_Mutex_); // Function will not return if it fails
}


/***********************************************************************************************************************

   Function Name: HD_dailySelfReadTimeHandler

   Purpose: Get/Set dailySelfReadTime

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) */
returnStatus_t HD_dailySelfReadTimeHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(hdParams_.dailySelfReadTime) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint32_t *)value = HD_getDailySelfReadTime();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)hdParams_.dailySelfReadTime, uintValue, 0);
            attr->rValTypecast = (uint8_t)timeValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = HD_setDailySelfReadTime(*(uint32_t *)value);

      if (retVal != eSUCCESS)
      {
         retVal = eAPP_INVALID_VALUE;
      }

   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: HD_dsBuDataRedundancyHandler

   Purpose: Get/Set dsBuDataRedundancy

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) */
returnStatus_t HD_dsBuDataRedundancyHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(hdParams_.dsBuDataRedundancy) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = HD_getDsBuDataRedundancy();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(hdParams_.dsBuDataRedundancy);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = HD_setDsBuDataRedundancy(*(uint8_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: HD_dsBuMaxTimeDiversityHandler

   Purpose: Get/Set dsBuMaxTimeDiversity

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) */
returnStatus_t HD_dsBuMaxTimeDiversityHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(hdParams_.dsBuMaxTimeDiversity) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = HD_getDsBuMaxTimeDiversity();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(hdParams_.dsBuMaxTimeDiversity);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = HD_setDsBuMaxTimeDiversity(*(uint8_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: HD_dsBuTrafficClassHandler

   Purpose: Get/Set dsBuTrafficClass

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) */
returnStatus_t HD_dsBuTrafficClassHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( hdParams_.dsBuTrafficClass) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = HD_getDsBuTrafficClass();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof( hdParams_.dsBuTrafficClass);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = HD_setDsBuTrafficClass(*(uint8_t *)value);

      if (retVal != eSUCCESS)
      {
         retVal = eAPP_INVALID_VALUE;
      }

   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: HD_dsBuReadingTypesHandler

   Purpose: Get/Set dsBuReadingTypes

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) */
returnStatus_t HD_dsBuReadingTypesHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( hdParams_.dsBuReadingTypes) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         HD_getDsBuReadingTypes((uint16_t *)value);
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof( hdParams_.dsBuReadingTypes);
            attr->rValTypecast = (uint8_t)uint16_list_type;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      if( attr->rValLen == ( HD_TOTAL_CHANNELS * sizeof(uint16_t) ) )
      {
         HD_setDsBuReadingTypes((uint16_t *)value);
         retVal = eSUCCESS;
      }
      else
      {
         retVal = eAPP_INVALID_VALUE;
      }
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: HD_dsBuEnabledHandler

   Purpose: Get/Set dsBuEnabled parameter of the history module

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) */
returnStatus_t HD_dsBuEnabledHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( hdParams_.dsBuEnabled) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = HD_getDsBuEnabled();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof( hdParams_.dsBuEnabled);
            attr->rValTypecast = (uint8_t)Boolean;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = HD_setDsBuEnabled(*(uint8_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: HD_historicalRecoveryHandler

   Purpose: Get/Set dsBuEnabled parameter of the history module

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,id) */
returnStatus_t HD_historicalRecoveryHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( historicalRecoverySupported) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = historicalRecoverySupported;
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof( historicalRecoverySupported);
            attr->rValTypecast = (uint8_t)Boolean;
         }
      }
   }

   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched
