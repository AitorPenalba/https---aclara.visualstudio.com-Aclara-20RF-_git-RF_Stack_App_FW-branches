/***********************************************************************************************************************

   Filename:   ID_intervalTask.c

   Global Designator: ID_ (Interval Data)

   Contents: routines to support interval data

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
/*lint -esym(750,ID_DEBUG)  local macro not referenced; only used for internal ID */
#define LP_VERBOSE               0  /* Additional info printed */
#define LP_DUMP_VALUES           0  /* Readings printed */
#define ID_DEBUG                 0  /* Set non-zero to force cache flush on start of interval data task.   */

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include <math.h>
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#include "ID_intervalTask.h"
#include "file_io.h"
#if ( LP_IN_METER == 0 )
#include "partition_cfg.h"
#endif
#include "DBG_SerialDebug.h"
#include "pwr_task.h"
#include "sys_busy.h"
#include "hmc_mtr_info.h"
#include "intf_cim_cmd.h"
#if ( LP_IN_METER == 0 )
#include "object_list.h"
#endif
#include "time_sys.h"
#include "timer_util.h"
#include "HEEP_util.h"
#include "pack.h"
#include "rand.h"
#include "APP_MSG_Handler.h"

#if ( LP_IN_METER == 1 )
#include "time_DST.h"
#endif

#ifdef CDL_SLAVE_SUPPORT
#include "cdl.h"
#endif
#include "mode_config.h"

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
#if ( LP_IN_METER == 0 )
PACK_BEGIN
typedef struct PACK_MID
{
   var64_SI    prevVal;                   /* Contains the previous source reading. */
   uint64_t    smplDateTime;              /* Date time of the latest sample */
   uint32_t    smplIndex;                 /* The Index in the partition of the latest sample */
   uint32_t    totNumInts;                /* Total number of intervals in this channel. */
   uint32_t    maxValidInts;              /* maximum number of intervals in this channel that can be read */
   uint32_t    numValidInts;              /* Number of valid intervals that can be read */
   qualCodes_u qualCodes;                 /* Quality codes to apply to the interval being operated on */
} chMetaData_t;                           /* Meta data for each channel of interval data */
PACK_END

typedef struct
{
   chMetaData_t   metaData;               /* Internal configuration and settings. */
   idChCfg_t      cfg;                    /* Register Configuration */
} chCfg_t;                                /* Contains both the register data and the internal configuration. */

typedef struct
{
   chMetaData_t metaData[(uint8_t)ID_MAX_CHANNELS]; /* The configuration data. */
} idFileDataMeta_t;

typedef struct
{
   idChCfg_t cfg[(uint8_t)ID_MAX_CHANNELS];      /* The configuration data. */
} idFileDataCfg_t;
#endif

#if ( LP_IN_METER == 0 )
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t     qualCodes;                 /* Quality Codes */
   int64_t     sample;                    /* 64-bit sample */
} idSampleData_t;                         /* Data formatted to be placed in the interval data array. */
PACK_END
#endif

/* Since all the individual members are atomic, no MUTEX lock required to read them. If member
   size changes to larger than CPU bus, change following #define to 1   */
#define NEED_MUTEX_TO_READ_MEMBER 0
typedef struct
{
   uint8_t  lpBuDataRedundancy;
   uint8_t  lpBuMaxTimeDiversity;
   uint8_t  lpBuTrafficClass;    /* Qos   */
   bool     lpBuEnabled;         /* Defines is LP data is allowed to bubble   */
   uint16_t lpBuSchedule;        /* Global bubble up schedule  */
   uint8_t  extraSpace[8];       /* Use this space to expand the file, if needed */
} idFileParams_t;

#if ( LP_IN_METER == 1 )
typedef struct
{
   bool     loadTables;      /* Indicate if the LP config table needs to be re-read */
   bool     tableReadError;  /* Error reading config table  */
   bool     configError;     /* Config not supported  */
   uint8_t  statusBytes;     /* Number of status bytes per interval (for all channels) */
   uint8_t  dataBytes;       /* Number of data bytes (for all channels) */
   uint8_t  readOffset;      /* offset where the intervals in the block start */
   uint8_t  powerOf10[ID_MAX_CHANNELS];
   uint32_t multiplier[ID_MAX_CHANNELS];
   uint16_t blockSize;       /* Size of each block */
   uint16_t readingType[ID_MAX_CHANNELS]; /* Reading type associated with each channel */
} LP_Config_t;
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#if ( LP_IN_METER == 0 )
#define ID_FILE_UPDATE_RATE_META_SEC      ((uint32_t)5)                    /* File updates every 5 minutes */
#define ID_FILE_UPDATE_RATE_CFG_SEC       ((uint32_t)0xFFFFFFFF)           /* Infrequent updates */
#define ID_SAMPLE_RATE_24H                ((uint64_t)TIME_TICKS_PER_DAY)
#define ID_MAX_STORAGE_LEN                ((uint8_t)8)                     /* Maximum storage length allowed by interval data */
#define ID_MIN_STORAGE_LEN                ((uint8_t)1)                     /* Minimum storage length allowed by interval data */
#endif

#if ( LP_IN_METER == 1 )
#define TBL64_BLK_END_TIME_SIZE           ((uint8_t)5)
#define TBL64_BLK_END_READING_SIZE        ((uint8_t)6)
#endif

#define ID_FILE_UPDATE_RATE_PARAMS_SEC    ((uint32_t)0xFFFFFFFF)           /* Infrequent updates */

#if ( COMMERCIAL_METER == 1 )
#define LP_INTERVAL_IN_MINUTES  ((uint16_t)5)
#else
#define LP_INTERVAL_IN_MINUTES  ((uint16_t)15)
#endif

#define LP_BU_TRAFFIC_CLASS_DEFAULT       ((uint8_t)32)
#define LP_BU_TRAFFIC_CLASS_MAX           ((uint8_t)63)

#define LP_BU_DATA_REDUNDANCY_DEFAULT     ((uint8_t)0)
#define LP_BU_DATA_REDUNDANCY_MAX         ((uint8_t)2)

#if ( COMMERCIAL_METER == 1 )
#define LP_BU_MAX_TIME_DIVERSITY_DEFAULT  ((uint8_t)4)
#else
#define LP_BU_MAX_TIME_DIVERSITY_DEFAULT  ((uint8_t)14)
#endif

#define LP_BU_MAX_TIME_DIVERSITY_MAX      ((uint8_t)60)
#define ID_MAX_BYTES_PER_ALL_CHANNELS     ((uint16_t)256)

#if( RTOS_SELECTION == FREE_RTOS )
#define INTERVAL_NUM_MSGQ_ITEMS 1
#else
#define INTERVAL_NUM_MSGQ_ITEMS 0
#endif

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#if ( LP_IN_METER == 0 )
static FileHandle_t           fileHndlMeta_;                               /* Contains the file handle information. */
static FileHandle_t           fileHndlCfg_;                                /* Contains the file handle information. */
static chCfg_t                chCfg_[(uint8_t)ID_MAX_CHANNELS];            /* Contains the configuration of the channel and meta data */
static bool                   bModuleBusy_ = (bool)false;                  /* Indicates module is busy or idle */
static PartitionData_t  const *pIdPartitions_[(uint8_t)ID_MAX_CHANNELS];   /* Pointer to partition that contains data array. */
static uint64_t               readingAtPwrUp_[(uint8_t)ID_MAX_CHANNELS];   /* Readings at power-up, if we boot-up with invalid time */
#endif

static FileHandle_t           fileHndlParam_;                              /* Contains the file handle information. */
static idFileParams_t         fileParams_;
static HEEP_APPHDR_t          heepHdr;                                     /* Application header/QOS info */
static OS_MUTEX_Obj           idMutex_;                                    /* Serialize access to ID module */
static OS_MSGQ_Obj            mQueueHandle_;                               /* Message Queue Handle */
static uint32_t               buScheduleInMs;                              /* Bubble up schedule in milli-seconds */
static uint16_t               bubbleUpTimerId = 0;                         /* Timer ID used by time diversity. */


#if ( LP_IN_METER == 1 )
static stdTbl61_t tbl61_;
static stdTbl62_t tbl62_;
static stdTbl63_t tbl63_;
static LP_Config_t LP_Config_;
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
/* The following is the partition name for each channel. */
#if ( LP_IN_METER == 0 )
static const ePartitionName partId_[] =
{
#if ( ID_MAX_CHANNELS > 6 )
   ePART_ID_CH1, ePART_ID_CH2, ePART_ID_CH3, ePART_ID_CH4, ePART_ID_CH5, ePART_ID_CH6, ePART_ID_CH7, ePART_ID_CH8
#else
   ePART_ID_CH1, ePART_ID_CH2, ePART_ID_CH3, ePART_ID_CH4, ePART_ID_CH5, ePART_ID_CH6
#endif
};
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static uint16_t PackQC( uint8_t QCbyte, pack_t *pPackCfg, bool doAdd );
#if ( LP_IN_METER == 0 )
static void IntervalDataCh( uint8_t ch, tTimeSysMsg const *pTimeMsg );
static returnStatus_t readCfg( uint8_t ch, chCfg_t *pChCfg );
static uint64_t readHostMeter( chCfg_t *pChCfg );
static void StoreNextID_Data( uint8_t ch, chCfg_t *pChCfg, uint64_t valToBeStored, bool metaDataOnly );
static void resetChannel ( uint8_t ch, chCfg_t *pChCfg );
static void cfgAllIntervalData( void );
static int16_t addAllLoadProfileChannels( uint32_t closeTime, uint8_t *buf, uint16_t room );
static returnStatus_t lpBuAllChannelsHandler( uint8_t ch, enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
static ID_QC_e getPriorityQC( ID_QC_e newQC, ID_QC_e oldQC );
#if ( REMOTE_DISCONNECT == 1 )
static bool runI210Channel( uint8_t ch );
#endif
#else
static void LoadLPTables( void );
static int16_t addMeterLoadProfileChannels( uint32_t closeTime, uint8_t *buf, uint16_t room );
#endif

static buffer_t* IntervalBubbleUp( tTimeSysMsg const *pTimeMsg, uint8_t trafficClass, uint8_t *buStatus );
static returnStatus_t ID_SetBubbleUpTimer( uint32_t timeMilliseconds );
static void ID_bubbleUpTimerExpired( uint8_t cmd, void *pData );

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION DEFINITIONS */
/***********************************************************************************************************************

   Function name: ID_init

   Purpose: Adds registers to the Object list and opens files.  This must be called at power before any other function
            in this module is called and before the register module is initialized.

   Arguments: None

   Returns: returnStatus_t - result of file open.

   Re-entrant Code: No

   Notes:  None

 **********************************************************************************************************************/
returnStatus_t ID_init( void )
{
   FileStatus_t   fileStatusParams;       /* Contains the file status */
   returnStatus_t retVal = eSUCCESS;

#if ( LP_IN_METER == 0 )
   FileStatus_t   fileStatusMeta;         /* Contains the file status */
   FileStatus_t   fileStatusCfg;          /* Contains the file status */

   /* Suppress the compiler warning message for the next two statements */
#pragma diag_suppress=Pe177
   /* The idFileDataMeta_t and idFileDataCfg_t typedefs are never instantiated. They are only used to get offsets into
      the repsective files. Two pointers are instantiated here (but never used) so the typedef info is included in the
      DWARF debug info of the output file.  This allows the NV file extraction post-processing application to capture
      the offset info for the two files.
   */
   /*lint -esym(438, pMeta, pCfg) not used OK */
   /*lint -esym(529, pMeta, pCfg) not referenced OK */
   const idFileDataMeta_t *const pMeta = 0;
   const idFileDataCfg_t  *const pCfg = 0;
#pragma diag_default=Pe177
#else
   LP_Config_.loadTables = true;      /* load the tables on power-up */
#endif

   if ( ( OS_MUTEX_Create( &idMutex_ ) ) && ( OS_MSGQ_Create( &mQueueHandle_, INTERVAL_NUM_MSGQ_ITEMS, "Interval" ) ) &&
#if ( LP_IN_METER == 0 )
         ( eSUCCESS == FIO_fopen( &fileHndlMeta_,                       /* File Handle so that Interval Data can get meta data. */
                                  ePART_SEARCH_BY_TIMING,               /* Search for best partition according to the timing.*/
                                  (uint16_t)eFN_ID_META,                /* File ID (filename) */
                                  ( lCnt )sizeof( idFileDataMeta_t ),   /* Size of the data in the file. */
                                  FILE_IS_NOT_CHECKSUMED,               /* File attributes to use. */
                                  ID_FILE_UPDATE_RATE_META_SEC,         /* The update rate of the data in the file. */
                                  &fileStatusMeta ) )
                                                                                 &&

         ( eSUCCESS == FIO_fopen( &fileHndlCfg_,                        /* File Handle so that Interval Data can get CFG data. */
                                  ePART_SEARCH_BY_TIMING,               /* Search for best partition according to the timing. */
                                  (uint16_t)eFN_ID_CFG,                 /* File ID (filename) */
                                  ( lCnt )sizeof( idFileDataCfg_t ),    /* Size of the data in the file. */
                                  FILE_IS_NOT_CHECKSUMED,               /* File attributes to use. */
                                  ID_FILE_UPDATE_RATE_CFG_SEC,          /* The update rate of the data in the file. */
                                  &fileStatusCfg ) )
                                                                                 &&

#endif
         ( eSUCCESS == FIO_fopen( &fileHndlParam_,                      /* File Handle so that Interval Data can configurable parameters */
                                  ePART_SEARCH_BY_TIMING,               /* Search for best partition according to the timing. */
                                  (uint16_t)eFN_ID_PARAMS,              /* File ID (filename) */
                                  ( lCnt )sizeof( idFileParams_t ),     /* Size of the data in the file. */
                                  FILE_IS_NOT_CHECKSUMED,               /* File attributes to use. */
                                  ID_FILE_UPDATE_RATE_PARAMS_SEC,       /* The update rate of the data in the file. */
                                  &fileStatusParams ) ) )
   {
#if ( LP_IN_METER == 0 )
      uint8_t  i;
      for ( i = 0; i < (uint8_t)ID_MAX_CHANNELS; i++ )
      {
         PartitionTbl_t  const   *pPartFunctions = &PAR_partitionFptr;
         if ( eFAILURE == pPartFunctions->parOpen( &pIdPartitions_[i], partId_[i], (uint32_t)0 ) )
         {
            return ( eFAILURE );
         }
      }
#endif

      if ( fileStatusParams.bFileCreated )
      {
         /*  The file was just created for the first time, set the default */
         (void)memset( &fileParams_, 0, sizeof( fileParams_ ) ); /* clear the file   */
         fileParams_.lpBuDataRedundancy = LP_BU_DATA_REDUNDANCY_DEFAULT;
         fileParams_.lpBuMaxTimeDiversity = LP_BU_MAX_TIME_DIVERSITY_DEFAULT;
         fileParams_.lpBuTrafficClass = LP_BU_TRAFFIC_CLASS_DEFAULT;
         fileParams_.lpBuSchedule = LP_INTERVAL_IN_MINUTES;
         fileParams_.lpBuEnabled = (bool)true;
         retVal = FIO_fwrite( &fileHndlParam_, 0, ( uint8_t * )&fileParams_, ( lCnt )sizeof( idFileParams_t ) );
      }
      else
      {
         retVal = FIO_fread( &fileHndlParam_, ( uint8_t * )&fileParams_, 0, ( lCnt )sizeof( idFileParams_t ) );
      }
      if ( eFAILURE == retVal )
      {
         return ( eFAILURE );
      }
      buScheduleInMs = fileParams_.lpBuSchedule * TIME_TICKS_PER_MIN;

#if ( LP_IN_METER == 0 )
      if ( fileStatusCfg.bFileCreated )
      {
         cfgAllIntervalData();
      }
      /* Read all the channels config and metadata */
      for ( i = 0; i < (uint8_t)ID_MAX_CHANNELS; i++ )
      {
         if ( eFAILURE == readCfg( i, &chCfg_[i] ) ) /* Read the configuration data  */
         {
            retVal = eFAILURE;
         }
      }
#endif
   }
   else
   {
      retVal = eFAILURE;
   }

   return( retVal );
}
/*lint +esym(438, pMeta, pCfg) not used OK */
/*lint +esym(529, pMeta, pCfg) not referenced OK */

#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: addAllLoadProfileChannels

   Purpose: Adds all the LP channels that closed at closeTime

   Arguments:  uint32_t closeTime:  Close time of the intervals (in seconds since epoch)
               uint8_t *buf:        Add the data in  this buffer
               uint16_t room:       Length of buffer
   Returns: Number of bytes added to the buffer (-1 if not enough room in buffer)

 **********************************************************************************************************************/
static int16_t addAllLoadProfileChannels( uint32_t closeTime, uint8_t *buf, uint16_t room )
{
   uint8_t           ch;                           /* Channel  */
   sysTimeCombined_t timeAtLpBoundary;             /* Time Message in combined format  */
   int16_t           len = 0;                      /* Number of bytes added to the buffer */
   uint16_t          rdgSize;                      /* Number of bytes needed for current reading   */
   pack_t            packCfg;                      /* Struct needed for PACK functions */
   bool              headerAdded = (bool)false;    /* Keeps track, if bit-structure header is added   */
   uint8_t           rdgCount = 0;                 /* Number of readings added in  the bit-structure  */
   uint8_t           maxChannels = min( (uint8_t)ID_MAX_CHANNELS, tbl61_.lpDataSet1.nbrChnsSet );

   timeAtLpBoundary = ( sysTimeCombined_t )closeTime * TIME_TICKS_PER_SEC;

   for ( ch = 0; ch < maxChannels; ch++ )
   {
      /* Get the value of each channel at this time and pack them in one bit-structure */
      idSampleData_t smplData;   /* packed buffer with quality codes   */
      uint32_t       totLen;
      uint32_t       index;
      uint32_t       intervalToRead;
      lAddr          offset;
      qualCodes_u    qualCodes;

      if (  ( 0 == chCfg_[ch].cfg.sampleRate_mS )                    ||    /* Channel disabled  */
            ( timeAtLpBoundary > chCfg_[ch].metaData.smplDateTime )  ||    /* Requested data in the future   */
            ( ( timeAtLpBoundary % chCfg_[ch].cfg.sampleRate_mS ) != 0 ) ) /* Not on channel boundary   */
      {
         continue;
      }
      intervalToRead = (uint32_t)( ( chCfg_[ch].metaData.smplDateTime - timeAtLpBoundary ) / chCfg_[ch].cfg.sampleRate_mS );
      if ( intervalToRead >= chCfg_[ch].metaData.numValidInts )   /* Too far in the past  */
      {
         continue;
      }
      totLen = ( chCfg_[ch].cfg.storageLen + (uint32_t)sizeof( chCfg_[ch].metaData.qualCodes.byte ) );
      index = ( ( (uint32_t)chCfg_[ch].metaData.smplIndex + chCfg_[ch].metaData.totNumInts ) - intervalToRead ) %
                  ( lAddr )chCfg_[ch].metaData.totNumInts;
      offset = index * totLen;

      (void)memset( &smplData, 0, sizeof( smplData ) );
      if ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t * )&smplData, offset, totLen, pIdPartitions_[ch] ) )
      {
         quantitySig_t        *pSig;
         uint64_t             sampleVal;
         sysTime_dateFormat_t dTime;
         sysTime_t            sTime;

         qualCodes.byte = smplData.qualCodes; /* Quality code for this interval  */

         TIME_UTIL_ConvertSecondsToSysFormat( closeTime, 0, &sTime );
         (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &dTime );
#if LP_VERBOSE
         INFO_printf(   "LP %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu Ch=%hhu QC=0x%X Val=%llu", dTime.month, dTime.day, dTime.year,
                        dTime.hour, dTime.min, dTime.sec, ch, smplData.qualCodes, smplData.sample ); /*lint !e123 */

#endif
         if ( eSUCCESS == OL_Search( OL_eHMC_QUANT_SIG, ( ELEMENT_ID )chCfg_[ch].cfg.rdgTypeID, ( void ** )&pSig ) )
         {
            uint8_t  RdgInfo;       /* Bit field ahead of every reading qty */
            uint16_t RdgType;       /* Reading identifier */
            uint8_t  powerOf10 = 0;
            uint8_t  powerOf10Code;
            uint8_t  numberOfBytes;

            if ( eANSI_MTR_LOOKUP == pSig->eAccess )
            {
               powerOf10 = (uint8_t)pSig->sig.multCfg.multiplierOveride;
            }
            else if ( eANSI_DIRECT == pSig->eAccess ) /* For direct reads, use multiplier12 to convert to business values  */
            {
               powerOf10 = (uint8_t)pSig->tbl.multiplier12;
            }

            /* Convert to raw format   */
            sampleVal = smplData.sample * (uint32_t)pow( 10.0, ( double )chCfg_[ch].cfg.trimDigits );
            if ( (   (  vRMSA == ( meterReadingType )chCfg_[ch].cfg.rdgTypeID                            ||
                        vRMSB == ( meterReadingType )chCfg_[ch].cfg.rdgTypeID                            ||
                        vRMSC == ( meterReadingType )chCfg_[ch].cfg.rdgTypeID ) && 0x7FFF == sampleVal ) ||
                     (  vRMS == ( meterReadingType )chCfg_[ch].cfg.rdgTypeID && 0 == sampleVal && 0 == qualCodes.byte ) )
            {
               /* Signal does not exist, ignore  */
               continue;
            }

            /* get the powerof10code and number of bytes */
            powerOf10Code = HEEP_getPowerOf10Code( powerOf10, (int64_t *)&sampleVal );
            numberOfBytes = HEEP_getMinByteNeeded( (int64_t)sampleVal, intValue, 0 );

            if ( !headerAdded )  /* Add the bit structure fields  */
            {
               if ( room >= ( VALUES_INTERVAL_END_SIZE + sizeof( rdgCount ) ) +  /* Room for header   */
                  ( PackQC( qualCodes.byte, &packCfg, (bool)false ) + numberOfBytes + sizeof( RdgType ) + sizeof( RdgInfo ) ) ) /* Room for rdg */
               {
                  uint32_t msgTime;

                  PACK_init( ( uint8_t* )&buf[0], &packCfg ); /* init the return buffer  */
                  headerAdded = (bool)true;
                  msgTime = closeTime;   /* Add the timestamp  */
                  PACK_Buffer( -( VALUES_INTERVAL_END_SIZE_BITS ), ( uint8_t * )&msgTime, &packCfg );
                  len = VALUES_INTERVAL_END_SIZE;

                  rdgCount = 0;   /* Insert the reading quantity, it will be adjusted later to the correct value   */
                  PACK_Buffer( sizeof( rdgCount ) * 8, ( uint8_t * )&rdgCount, &packCfg );
                  len += sizeof( rdgCount );
                  room -= ( VALUES_INTERVAL_END_SIZE + sizeof( rdgCount ) );
               }
               else
               {  /* Insufficient room left */
                  len = -1;
                  break;
               }
            }
            rdgSize = PackQC( qualCodes.byte, &packCfg, (bool)false ) + numberOfBytes + sizeof( RdgType ) + sizeof( RdgInfo );
            if ( room >= rdgSize )
            {
               /* Pack rest of the data, including the quality code  */
               RdgInfo = 0; /* No timestamp present or quality codes present  */
               if ( qualCodes.byte != 0 )
               {
                  RdgInfo |= 0x40; /* Quality codes present */
               }
               RdgInfo |= (uint8_t)( powerOf10Code << COMPACT_MTR_POWER10_SHIFT );
               RdgInfo |= (uint8_t)( (uint8_t)( numberOfBytes - 1U ) << COMPACT_MTR_RDG_SIZE_SHIFT );
               /*  Insert the reading info bit field  */
               PACK_Buffer( sizeof( RdgInfo ) * 8, &RdgInfo, &packCfg );
               len += sizeof( RdgInfo );

               RdgType = (uint16_t)getLpReadingType( ( meterReadingType )chCfg_[ch].cfg.rdgTypeID, chCfg_[ch].cfg.sampleRate_mS );
               PACK_Buffer( -(int16_t)( sizeof( RdgType ) * 8 ), ( uint8_t * )&RdgType, &packCfg );
               len += sizeof( RdgType );

               len += PackQC( qualCodes.byte, &packCfg, (bool)true ); /* Add QC */

               /* get the correct number of bytes and pack the data  */
               PACK_Buffer( -(int16_t)( numberOfBytes * 8 ), ( uint8_t * )&sampleVal, &packCfg );
               len += numberOfBytes;

               ++rdgCount; /*lint !e644 rdgCount initialized when header is added*/
               room -= rdgSize;
            }
            else
            {  /* Insufficient room left */
               len = -1;
               break;
            }
         }/* OL_Search  */
      }/* Partition read   */
   }/* For each channel, for loop   */

   if ( headerAdded )   /* Correct the event count */
   {
      buf[VALUES_INTERVAL_END_SIZE] = rdgCount;
   }
   return len;
}
#endif

/***********************************************************************************************************************

   Function Name: ID_MsgHandler

   Purpose: Handle OR_LP (On-request readings)

   Arguments:
         APP_MSG_Rx_t   *heepReqHdr - Pointer to application header structure from head end
         void           *payloadBuf - pointer to data payload (e.g. 105E8B6E205E8B79D8 for 1 range (UTC) 4/6/20 18:00 to 4/6/20 18:50 )
         uint16_t       length      - payload length

   Side Effects: none

   Re-entrant: No

   Returns: none

   Notes:

 ***********************************************************************************************************************/
/*lint -e{715,818} arguments not used and/or cannot be const because of API requirements  */
void ID_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length )
{
   MtrTimeFormatHighPrecision_t  tblTime; /* Time yy mm dd hh mm ss */
   sysTime_t                     sTime;

   if ( ( method_get == ( enum_MessageMethod )heepReqHdr->Method_Status ) && ( MODECFG_get_rfTest_mode() == 0 ) ) /* Process the get request  */
   {
      OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */

      /* Prepare Compact Meter Read structure   */
      HEEP_APPHDR_t heepRespHdr;          /*  Application header/QOS info  */
      buffer_t      *pBuf;                /*  pointer to message  */
      buffer_t      *pBufSmall;           /*  pointer to message  */

      /* Application header/QOS info   */
      HEEP_initHeader( &heepRespHdr );
      heepRespHdr.TransactionType = (uint8_t)TT_RESPONSE;
      heepRespHdr.Resource = (uint8_t)or_lp;
      heepRespHdr.Method_Status = (uint8_t)OK;
      heepRespHdr.Req_Resp_ID = (uint8_t)heepReqHdr->Req_Resp_ID;
      heepRespHdr.qos = (uint8_t)heepReqHdr->qos;
      heepRespHdr.callback = NULL;
      heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;

      pBuf = BM_alloc( APP_MSG_MAX_DATA ); /* Allocate max buffer   */
      pBufSmall = BM_alloc( ID_MAX_BYTES_PER_ALL_CHANNELS ); /* Allocate small buffer to build message  */
      if ( pBuf != NULL && pBufSmall != NULL )
      {
         uint16_t                bits = 0;   /*  Keeps track of the bit position within the Bitfields */
         union TimeSchRdgQty_u   qtys;       /*  For EventTypeQty and ReadingTypeQty   */
         uint8_t                 i;
         int16_t                 len;

         pBuf->x.dataLen = 0;
         qtys.TimeSchRdgQty = UNPACK_bits2_uint8( payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS ); /* Parse the fields passed in */

         /* Process each time pairs, ignore the reading quantities (return all the channels)  */
         for ( i = 0; i < qtys.bits.timeSchQty; i++ )
         {
            uint32_t startTime   = UNPACK_bits2_uint32( payloadBuf, &bits, VALUES_INTERVAL_START_SIZE_BITS );
            uint32_t endTime     = UNPACK_bits2_uint32( payloadBuf, &bits, VALUES_INTERVAL_END_SIZE_BITS );
            uint32_t lastStartTime; /* used to detect rollover */

#if LP_VERBOSE
            TIME_UTIL_ConvertSecondsToSysFormat( startTime, 0, &sTime );         /* Convert input seconds to sysTime_t format */
            (void)TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sTime, &tblTime ); /* Convert to meter format, local   */
            INFO_printf( "heep start: 0x%0lX, %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu (local)", startTime , tblTime.month, tblTime.day, tblTime.year + 2000U,
                           tblTime.hours, tblTime.minutes, tblTime.seconds );

            TIME_UTIL_ConvertSecondsToSysFormat( endTime, 0, &sTime );           /* Convert input seconds to sysTime_t format */
            (void)TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sTime, &tblTime ); /* Convert to meter format, local   */
            INFO_printf( "heep end  : 0x%0lX, %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu (local)", endTime, tblTime.month, tblTime.day, tblTime.year + 2000U,
                           tblTime.hours, tblTime.minutes, tblTime.seconds );
#endif
            if ( startTime >= endTime )
            {
               /* Bad request  */
               heepRespHdr.Method_Status = (uint8_t)BadRequest;
               break;
            }
            /* if not on 5 minute boundary, get to previous 5 minute boundary.   */
            startTime = startTime - ( startTime % SECONDS_PER_5MINUTE );

            for( ;; )
            {
               /* get as many entries that will fit in this timeframe */
               lastStartTime = startTime;
               startTime = startTime + SECONDS_PER_5MINUTE; /* Find the next 5 minute boundary  */
               if ( 0 == ( startTime % SECONDS_PER_DAY ) )
               {
                  /* Crossed midnight boundary, let other tasks run  */
                  OS_TASK_Sleep( TEN_MSEC );
               }
               if ( startTime > endTime || lastStartTime >= startTime )
               {
                  /* Done with the timeframe */
                  break; /* All done for this range   */
               }
#if ( LP_IN_METER == 0 )
               len = addAllLoadProfileChannels( startTime, &pBufSmall->data[0], pBufSmall->x.dataLen );   /* Add all channels at startTime  */
#else
               len = addMeterLoadProfileChannels( startTime, &pBufSmall->data[0], pBufSmall->x.dataLen ); /* Add all channels at startTime   */
#endif
               TIME_UTIL_ConvertSecondsToSysFormat( startTime, 0, &sTime );         /* Convert input seconds to sysTime_t format */
               (void)TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sTime, &tblTime ); /* Convert to meter format */
               INFO_printf( "Length at local time %02hhu/%02hhu/%04hu %02hhu:%02hhu:00: %hu, Total length: %4hu, room %4hu",
                              tblTime.month, tblTime.day, tblTime.year + 2000U, tblTime.hours, tblTime.minutes, len, pBuf->x.dataLen,
                              ( APP_MSG_MAX_DATA - pBuf->x.dataLen ) );
               if ( len > 0 )
               {
                  if ( ( APP_MSG_MAX_DATA - pBuf->x.dataLen ) < len )   /* If the size of this range is greater than what can fit in the message... */
                  {
                     heepRespHdr.Method_Status = (uint8_t)PartialContent;  /* limit the number of entries to avoid buffer overrun   */
                     break; /* All done for this range   */
                  }
                  else  /* This range can fit... */
                  {
                     (void)memcpy( &pBuf->data[pBuf->x.dataLen], &pBufSmall->data[0], (uint16_t)len );
                     pBuf->x.dataLen += (uint16_t)len;
                  }
               }
            }/* Outer for loop   */
         } /* Process all time pairs   */

         if ( heepRespHdr.Method_Status == (uint8_t)BadRequest || pBuf->x.dataLen == 0 )  /* Just send the header */
         {
            (void)HEEP_MSG_TxHeepHdrOnly ( &heepRespHdr );
            INFO_printf( "LP No matching Date/Time or Bad request" );
            BM_free( pBuf );
            BM_free( pBufSmall );
         }
         else  /* send the message to message handler . The called function will free the buffer   */
         {
            INFO_printf( "Final Length %hu", pBuf->x.dataLen );
            (void)HEEP_MSG_Tx ( &heepRespHdr, pBuf );
            BM_free( pBufSmall );
         }
      }
      else  /* No buffer available  */
      {
         heepRespHdr.Method_Status = (uint8_t)ServiceUnavailable;
         (void)HEEP_MSG_TxHeepHdrOnly ( &heepRespHdr );
         ERR_printf( "ERROR: LP No buffer available" );
         if ( pBuf != NULL )
         {
            BM_free( pBuf );
         }
         if ( pBufSmall != NULL )
         {
            BM_free( pBufSmall );
         }
      }
      OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
   }
}
#if ( LP_IN_METER == 1 )
/***********************************************************************************************************************

   Function name: LoadLPTables

   Purpose: Read and validate the meter LP configuration.

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
static void LoadLPTables( void )
{
   LP_Config_.loadTables      = (bool)false; /* Attempted to read the tables */
   LP_Config_.tableReadError  = (bool)true ; /* Default, not yet tried reading the tables   */
   LP_Config_.configError     = (bool)false; /* Default, the tables are configured correctly   */
   LP_Config_.statusBytes     = 0;           /* Safe value in case of config error */

#if LP_VERBOSE
      INFO_printf( "Reading tables 61 and 63" );
#endif
   if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tbl61_, STD_TBL_ACTUAL_LOAD_PROFILE_LIMITING, 0, (uint16_t)sizeof( tbl61_ ) ) &&
      ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tbl63_, STD_TBL_LOAD_PROFILE_STATUS,          0, (uint16_t)sizeof( tbl63_ ) ) ) )
   {
      /* Successfully read table 61 */
      /* Get LP_SEL_SET for all channels  */
      uint32_t offset         = 0;
      uint16_t size           = sizeof( lpSelSet_t ) * tbl61_.lpDataSet1.nbrChnsSet;
      uint16_t totalBytes;
      uint16_t bytesToRead;
      uint8_t  channels       = tbl61_.lpDataSet1.nbrChnsSet;

      LP_Config_.tableReadError = (bool)false;   /* At least read tables 61 & 63.   */
#if LP_VERBOSE
      INFO_printf( "LoadLPTables channels=%hhu, statusBytes=%hhu, size=%hu", channels, LP_Config_.dataBytes, size );
#endif

      totalBytes = size;
      do
      {
         bytesToRead = min( totalBytes, HMC_REQ_MAX_BYTES );
#if LP_VERBOSE
      INFO_printf( "Reading table 62: offset: %hu, size: %hu", offset, bytesToRead );
#endif
         if ( CIM_QUALCODE_SUCCESS != INTF_CIM_CMD_ansiRead( (uint8_t *)&tbl62_ + offset, STD_TBL_LOAD_PROFILE_CONTROL, offset, bytesToRead ) )
         {
            LP_Config_.tableReadError = (bool)true;   /* One or more table read failed  */
            break;
         }
         totalBytes -= bytesToRead;
         offset += bytesToRead;
      } while ( ( totalBytes != 0 ) && !LP_Config_.tableReadError );

      if ( !LP_Config_.tableReadError )
      {
         /* Get the format code  */
         offset = size; /* remember the number of bytes read   */
         size = sizeof(uint8_t);

         if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tbl62_.intFmtCde, STD_TBL_LOAD_PROFILE_CONTROL, offset, size ) )
         {
            /* Get the scalar sets  */
            offset += size; /* remember the number of bytes read  */
            size = sizeof(uint16_t) * channels;
#if LP_VERBOSE
            INFO_printf( "Reading table 62: offset: %hu, size: %hu", offset, size );
#endif

            if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tbl62_.scalarsSet[0], STD_TBL_LOAD_PROFILE_CONTROL, offset, size ) )
            {
               /* Get the divisors sets   */
               offset += size; /* remember the number of bytes read  */
               size = sizeof(uint16_t) * channels;

#if LP_VERBOSE
               INFO_printf( "Reading table 62: offset: %hu, size: %hu", offset, size );
#endif
               if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tbl62_.divisors_set[0], STD_TBL_LOAD_PROFILE_CONTROL, offset, size ) )
               {
                  /* Validate configuration and compute offsets needed to parse LP data tables  */
                  /* As per R&P guide all following parameters in table STD Table 61 and 62 are fixed.
                     Check if they are are the default value */

                  if (  tbl61_.lpFlags.lpSet1InhibitOvfFlag  == 1 ||
                        tbl61_.lpFlags.lpSet2InhibitOvfFlag  == 1 ||
                        tbl61_.lpFlags.lpSet3InhibitOvfFlag  == 1 ||
                        tbl61_.lpFlags.lpSet4InhibitOvfFlag  == 1 ||
                        tbl61_.lpFlags.blkEndReadFlag        == 0 ||
                        tbl61_.lpFlags.blkEndPulseFlag       == 1 ||
                        tbl61_.lpFlags.scalarDivisorFlagSet1 == 0 ||
                        tbl62_.divisors_set[0]               == 0 || /* To avoid divide by 0 error  */
                        tbl61_.lpFlags.scalarDivisorFlagSet2 == 1 ||
                        tbl61_.lpFlags.scalarDivisorFlagSet3 == 1 ||
                        tbl61_.lpFlags.scalarDivisorFlagSet4 == 1 ||
                        tbl61_.lpFlags.extendedIntStatusFlag == 0 ||
                        tbl61_.lpFlags.simpleIntStatusFlag   == 1 )
                  {
                     LP_Config_.configError = true;
                  }

                  if (  tbl61_.lpFormats.invUnit8Flag  == 1 ||
                        tbl61_.lpFormats.invUnit16Flag == 1 ||
                        tbl61_.lpFormats.invUnit32Flag == 1 ||
                        tbl61_.lpFormats.invInt8Flag   == 1 ||
                        tbl61_.lpFormats.invInt16Flag  == 0 ||
                        tbl61_.lpFormats.invInt32Flag  == 1 ||
                        tbl61_.lpFormats.invNiFmt1Flag == 1 ||
                        tbl61_.lpFormats.invNiFmt2Flag == 1 )
                  {
                     LP_Config_.configError = true;
                  }

                  if (  tbl61_.lpDataSet1.nbrChnsSet < 1  ||
                        tbl61_.lpDataSet1.nbrChnsSet > ID_MAX_CHANNELS )
                  {
                     LP_Config_.configError = true;
                  }

                  if ( tbl61_.lpDataSet1.nbrMaxIntTimeSet == 0 )  /* LP disabled */
                  {
                     LP_Config_.configError = true;
                  }

                  if ( !(  tbl61_.lpDataSet1.nbrMaxIntTimeSet == 5  ||
                           tbl61_.lpDataSet1.nbrMaxIntTimeSet == 15 ||
                           tbl61_.lpDataSet1.nbrMaxIntTimeSet == 30 ||
                           tbl61_.lpDataSet1.nbrMaxIntTimeSet == 60 ) )
                  {
                     LP_Config_.configError = true;
                  }

                  if ( tbl62_.lpSelSet[0].chnlFlag.endRdgFlag == 0 && tbl61_.lpDataSet1.nbrChnsSet >= 1 )
                  {
                     LP_Config_.configError = true;
                  }
                  if ( tbl62_.lpSelSet[1].chnlFlag.endRdgFlag == 0 && tbl61_.lpDataSet1.nbrChnsSet >= 2 )
                  {
                     LP_Config_.configError = true;
                  }
                  if ( tbl62_.lpSelSet[2].chnlFlag.endRdgFlag == 0 && tbl61_.lpDataSet1.nbrChnsSet >= 3 )
                  {
                     LP_Config_.configError = true;
                  }
                  if ( tbl62_.lpSelSet[3].chnlFlag.endRdgFlag == 0 && tbl61_.lpDataSet1.nbrChnsSet >= 4 )
                  {
                     LP_Config_.configError = true;
                  }
                  if ( tbl62_.intFmtCde != 16 )
                  {
                     LP_Config_.configError = true;
                  }

                  if (  tbl63_.lpSelSet1.blockOrder               == 1 ||
                        tbl63_.lpSelSet1.listType                 == 1 ||
                        tbl63_.lpSelSet1.blockInhibitOverflowFlag == 1 ||
                        tbl63_.lpSelSet1.intervalOrder            == 1 ||
                        tbl63_.lpSelSet1.testMode                 == 1 ||
                        tbl63_.lpSelSet1.lpPowerFailRec           == 1 )
                  {
                     /* If get this error it is likey due to lpPowerFailRec until reconstruction completes */
                     INFO_printf( "LPSetStatusFlags Error" );
                     LP_Config_.configError = true;
                  }

                  /* Find the unit of measure for each channel */
                  stdTbl12_t        st12;             /* contents of one ST12 entry at LP index value mod 20  */
                  uint8_t           st14[2] = {0};    /* contents of ST14 at LP index value   */
                  stdTbl15_t        st15;             /* contents of one ST15 entry at LP index value mod 20 */
                  buffer_t          *tbl12;           /* Holds entire standard table 12, so it only is read once here.  */
                  buffer_t          *tbl14;           /* Holds entire standard table 14, so it only is read once here.  */
                  buffer_t          *tbl15;           /* Holds entire standard table 15, so it only is read once here.  */
                  stdTbl11_t        *tbl11 = NULL;    /* Pointer to standard table 11 read at boot up.   */
                  uint8_t           ch;
                  meterReadingType  cUom;
                  uint8_t           powerOf10 = 0;
                  uint8_t           maxChannels = min( (uint8_t)ID_MAX_CHANNELS, tbl61_.lpDataSet1.nbrChnsSet );

                  (void)HMC_MTRINFO_getStdTable11( &tbl11 ); /* Since task waited for HMC_MTRINFO_Ready, this will succeed. */
                  tbl12 = BM_alloc( tbl11->nbrUomEntries * sizeof( stdTbl12_t ) );
                  tbl14 = BM_alloc( sizeof( stdTbl14_t ) );
                  tbl15 = BM_alloc( tbl11->nbrConstantsEntries * sizeof( stdTbl15_t ) );
                  if ( ( tbl12 != NULL ) && ( tbl14 != NULL ) && ( tbl15 != NULL ) && ( tbl11 != NULL ) )
                  {
                     /* Read all of standard table 12 */
                     offset = 0;
                     totalBytes = tbl11->nbrUomEntries * sizeof( stdTbl12_t );
                     do
                     {
                        bytesToRead = min( totalBytes, HMC_REQ_MAX_BYTES );
#if LP_VERBOSE
                        INFO_printf( "Reading table 12: dest: %p offset: %hu: len: %hu", tbl12->data+offset, offset, bytesToRead );
#endif
                        if ( CIM_QUALCODE_SUCCESS != INTF_CIM_CMD_ansiRead( tbl12->data+offset, STD_TBL_UNITS_OF_MEASURE_ENTRY, offset, bytesToRead ) )
                        {
                           LP_Config_.tableReadError = (bool)true;   /* One or more table read failed  */
                           break;
                        }
                        totalBytes -= bytesToRead;
                        offset += bytesToRead;
                     } while ( ( totalBytes != 0 ) && !LP_Config_.tableReadError );

                     /* Read all of standard table 14 */
                     offset = 0;
                     totalBytes = sizeof( stdTbl14_t );
                     while ( ( totalBytes != 0 ) && !LP_Config_.tableReadError )
                     {
                        bytesToRead = min( totalBytes, HMC_REQ_MAX_BYTES );
#if LP_VERBOSE
                        INFO_printf( "Reading table 14: dest: %p offset: %hu: len: %hu", tbl14->data+offset, offset, bytesToRead );
#endif
                        if ( CIM_QUALCODE_SUCCESS != INTF_CIM_CMD_ansiRead( tbl14->data + offset, STD_TBL_DATA_CONTROL, offset, bytesToRead ) )
                        {
                           LP_Config_.tableReadError = (bool)true;   /* One or more table read failed  */
                           break;
                        }
                        totalBytes -= bytesToRead;
                        offset += bytesToRead;
                     }

                     /* Read all of standard table 15 */
                     offset = 0;
                     totalBytes = tbl11->nbrConstantsEntries * sizeof( stdTbl15_t );
                     while ( ( totalBytes != 0 ) && !LP_Config_.tableReadError )
                     {
                        bytesToRead = min( totalBytes, HMC_REQ_MAX_BYTES );
#if LP_VERBOSE
                        INFO_printf( "Reading table 15: dest: %p offset: %hu: len: %hu", tbl15->data+offset, offset, bytesToRead );
#endif
                        if ( CIM_QUALCODE_SUCCESS != INTF_CIM_CMD_ansiRead( tbl15->data + offset, STD_TBL_CONSTANTS, offset, bytesToRead ) )
                        {
                           LP_Config_.tableReadError = (bool)true;   /* One or more table read failed  */
                           break;
                        }
                        totalBytes -= bytesToRead;
                        offset += bytesToRead;
                     }

                     if ( !LP_Config_.tableReadError )
                     {
			uint8_t periodLP = tbl61_.lpDataSet1.nbrMaxIntTimeSet;
                        for ( ch = 0; ch < maxChannels; ch++ )
                        {
#if LP_VERBOSE
                           INFO_printf( "ch: %hhu srcSel: %hhu", ch, tbl62_.lpSelSet[ch].lpSrcSel );
#endif
                           (void)memcpy( (uint8_t *)(void *)&st12, tbl12->data + ( tbl62_.lpSelSet[ch].lpSrcSel * sizeof( st12 ) ), sizeof( st12 ) );
                           (void)memcpy( &st14[0], tbl14->data + ( tbl62_.lpSelSet[ch].lpSrcSel * sizeof( st14 ) ), sizeof( st14 ) );
                           (void)memcpy( (uint8_t *)(void *)&st15, tbl15->data + ( ( tbl62_.lpSelSet[ch].lpSrcSel % TABLE_15_MODULO ) * sizeof( st15 ) ), sizeof( st15 ) );
                           INFO_printf( "ch=%hhu id=%hhu, timebase=%hhu, mul=%hhu, q1=%hhu, q2=%hhu, q3=%hhu, q4=%hhu, "
                                        "net=%hhu, seg=%hhu, har=%hhu, r1=%hhu, r2=%hhu nfs=%hhu ST14.b1=%hhu ST14.b2=%hhu",
                                        ch, st12.idCode, st12.timeBase, st12.multiplier,
                                        st12.q1Accountability, st12.q2Accountability, st12.q3Accountability, st12.q4Accountability,
                                        st12.netFlowAccountability, st12.segmentation, st12.harmonic, st12.reserved1, st12.reserved2, st12.nfs,
                                        st14[0], st14[1] );
                           if  ( CIM_QUALCODE_SUCCESS == HMC_MTRINFO_getLPReadingType( &st12, &st14[0], periodLP, &cUom, &powerOf10 ) )
                           {
                              LP_Config_.readingType[ch] = (uint16_t)cUom;
                              LP_Config_.powerOf10[ch] = powerOf10;
                              (void)memcpy( (uint8_t *)&LP_Config_.multiplier[ch], st15.multiplier, sizeof(LP_Config_.multiplier[ch]) );
                              INFO_printf( "SUCCESS UOM=%hu LP_Period=%hu ReadingType=%hu Powerof10=%hhu multiplier=%u",
                                           cUom, tbl61_.lpDataSet1.nbrMaxIntTimeSet, LP_Config_.readingType[ch],
                                           LP_Config_.powerOf10[ch], LP_Config_.multiplier[ch] );
                           }
                           else  /* We don't understand this qty. Print some details.  */
                           {
                              uint8_t quadrants[4];

                              quadrants[0] = st12.q1Accountability;
                              quadrants[1] = st12.q2Accountability;
                              quadrants[2] = st12.q3Accountability;
                              quadrants[3] = st12.q4Accountability;

                              INFO_printf( "" );
                              INFO_printf( "ERROR UOM not found, ch: %hhu", ch );
                              INFO_printf( "ID %hhu, timebase %hhu, mul %hhu quadrants %hhu%hhu%hhu%hhu netFlow %hhu nfs %hhu, st14 %hhu %hhu\n",
                                           st12.idCode, st12.timeBase, st12.multiplier, quadrants[0], quadrants[1], quadrants[2], quadrants[3],
                                           st12.netFlowAccountability, st12.nfs, st14[0], st14[1] );
                              LP_Config_.readingType[ch] = (uint16_t)invalidReadingType;
                           }
                           OS_TASK_Sleep(FIFTY_MSEC); /* Wait for printout to complete */
                        }
                     }
                  }
                  if ( tbl12 != NULL )
                  {
                     BM_free( tbl12 );
                  }

                  if ( tbl14 != NULL )
                  {
                     BM_free( tbl14 );
                  }

                  if ( tbl15 != NULL )
                  {
                     BM_free( tbl15 );
                  }

                  if ( !LP_Config_.configError )   /* Configs valid, compute the offsets  */
                  {
                     LP_Config_.statusBytes = ( channels / 2U ) + 1U;   /* Bytes for status  */
                     LP_Config_.dataBytes = channels * 2U;              /*lint !e734 (9 bits to 8 bits, bytes for data, 2 per channel)   */
                     /* readOffset gives where the intervals in the block start  */
                     LP_Config_.readOffset = TBL64_BLK_END_TIME_SIZE;                     /* Bytes for the date and time of the last read */
                     LP_Config_.readOffset += ( TBL64_BLK_END_READING_SIZE * channels );  /*lint !e734 Bytes for end block reads   */
                     LP_Config_.blockSize = (uint16_t)(( LP_Config_.statusBytes + LP_Config_.dataBytes ) *
                                                tbl61_.lpDataSet1.nbrBlksIntsSet);        /*lint !e734 (Bytes for all the readings in block) */
                     LP_Config_.blockSize += LP_Config_.readOffset;
                     INFO_printf(   "StatusSize=%hhu, DataSize=%hhu MetaSize=%hhu, BlockSize=%hu", LP_Config_.statusBytes, LP_Config_.dataBytes,
                                    LP_Config_.readOffset, LP_Config_.blockSize );
                     INFO_printf( "LP Table read successful and are configured correctly" );
                  }
                  else
                  {
                     INFO_printf( "LP Table configuration error" );
                  }
               }
            }
         }
      }
   }

   if ( LP_Config_.tableReadError )
   {
      INFO_printf( "LP Table read Failed" );
   }
}
#endif
#if ( LP_IN_METER == 1 )
/***********************************************************************************************************************

   Function name: addMeterLoadProfileChannels

   Purpose: Adds all the LP channels that closed at closeTime

   Arguments:  uint32_t closeTime:  Close time of the intervals (in seconds since epoch)
               uint8_t  *buf:       Add the data in  this buffer
               uint16_t room:       Length of buffer
   Returns: Number of bytes added to the buffer (-1 if insufficient room in buffer)

 **********************************************************************************************************************/
static int16_t addMeterLoadProfileChannels( uint32_t closeTime, uint8_t *buf, uint16_t room )
{


   MtrTimeFormatHighPrecision_t  tblTime;                      /* Time yy mm dd hh mm ss */
   uint16_t                      chVals[ID_MAX_CHANNELS];      /* Holds interval values   */
   uint8_t                       LPrecord[((5U*ID_MAX_CHANNELS)/2U)+1U];   /* Holds one interval record (qual codes and values   */
   stdTbl64Flags_t               tblFlags;
   sysTime_t                     sTime;
   pack_t                        packCfg;                      /* Struct needed for PACK functions */
   uint32_t                      offset;
   int16_t                       len = 0;                      /* Number of bytes added to the buffer */
   uint16_t                      indexOfEvtRdgQty;             /* Index for next Event/Reading Qty field */
   uint16_t                      lastIntThisBlock;             /* Number of intervals in current block   */
   uint16_t                      rdgSize;                      /* Number of bytes needed for current reading   */
   uint8_t                       rdgCount = 0;                 /* Number of readings added in  the bit-structure   */
   bool                          headerAdded = (bool)false;    /* Keeps track, if bit-structure header is added */
   bool                          needHeader = (bool)true;      /* Used to force "mini-header" after coincident values. */

   /* Check, if close time is on interval boundary */
   if ( closeTime % ( tbl61_.lpDataSet1.nbrMaxIntTimeSet * SECONDS_PER_MINUTE ) != 0 ) /* Not on channel boundary   */
   {
      return 0; /* No bytes added, not on interval boundary  */
   }

   indexOfEvtRdgQty = VALUES_INTERVAL_END_SIZE;                         /* Index for first Event/Reading Qty field */
   TIME_UTIL_ConvertSecondsToSysFormat( closeTime, 0, &sTime );         /* Convert input seconds to sysTime_t format */
   (void)TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sTime, &tblTime ); /* Convert to meter format */
#if LP_VERBOSE
   INFO_printf( "" );
   INFO_printf(   "Requested UTC date: 0x%08lX ->local %02u/%02u/%04u %02u:%02u:%02u", closeTime, tblTime.month, tblTime.day, tblTime.year + 2000U,
                  tblTime.hours, tblTime.minutes, tblTime.seconds );
   INFO_printf(   "StatusSize=%hhu, DataSize=%hhu readOffset=%hu, BlockSize=%hu", LP_Config_.statusBytes, LP_Config_.dataBytes, LP_Config_.readOffset,
                  LP_Config_.blockSize );

#endif
   /* Read tables that change with each interval   */
   if ( INTF_CIM_CMD_ansiRead( &tbl63_, STD_TBL_LOAD_PROFILE_STATUS, 0, (uint16_t)sizeof( stdTbl63_t ) ) )
   {
      return 0; /* Error reading meter table. No bytes added   */
   }
#if LP_VERBOSE
   INFO_printf( "T63 nbrValidBlks=%hu, lastBlk=%hu, nbrValidInt=%hu", tbl63_.nbrValidBlocks, tbl63_.lastBlockElement, tbl63_.nbrValidInt );
#endif

   /* Optimized search by computing offset into table 64 with requested date range entries.  */
   /* Compute offset in to table 64:
      Table 61 has number of intervals per block (nbr_blks_ints_set)
      Table 63 has Number of valid blocks (nbr_valid_blks), last block element (last_blk) and number of valid intervals in last block
      (nbr_valid_int).

      Offset into table 64 for last_blk = last_blk * LP_Config_.blockSize
      Get the end time of the last valid interval in the last_blk.

      Compute difference in time between that reading and the requested reading and convert to number of intervals.
      If time difference is greater than (nbr_valid_blks-1)*intervals_per_block + nbr_valid_intervals, then the requested date is too far in the
      past.
      Else, compute block in which target resides and index in target block for interval requested.
         Number of intervals to back up is
            intervalsDiff = ( ( meterSeconds - closeTime ) / ( SECONDS_PER_MINUTE * tbl61_.lpDataSet1.nbrMaxIntTimeSet ) );
            Note that since these times end on interval close times, the quotient is an integer.

         Since power failures and time changes affect the time span included in a block. Check the block start time. Move forward backward as needed
         to find block with requested interval.

         If intervalsDiff < nbr_valid_int then target interval is in last block
         Else, Number of blocks to back up is ( ( intervalsDiff - nbr_valid_int ) / nbr_blks_ints_set ) + 1
            blocksDiff = (( intervalsDiff - nbr_valid_int ) / tbl61_.lpDataSet1.nbrBlksIntsSet ) + 1

         Compute offset from beginning of block to nbr_valid_int by taking modulo of intervalsDiff an nbr_blks_ints_set and place in intervalsDiff
         Target interval is located intervalsDiff from offset of nbr_valid_int in target block.
         If intervalsDiff > nbr_valid_int Target interval is before nbr_valid_int offset
         Else it is after that offset (or at that offset if modulo operation results in 0 )
   */
   PACK_init( buf, &packCfg );   /* init the return buffer  */
   offset = (uint32_t)( tbl63_.lastBlockElement * LP_Config_.blockSize );  /* Beginning of block with most recent interval */
#if LP_VERBOSE
   INFO_printf( "" );
   INFO_printf( "Reading T64 @ 0x%lX", offset );
#endif

   /* Read the date and time from table 64   */
   if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &tblTime, STD_TBL_LOAD_PROFILE_DATASET_1, offset, (uint16_t)sizeof( stdTbl64Time_t ) ) )
   {
      uint32_t meterSeconds;     /* Used in conversion between meter time and UTC time.   */
      uint32_t microSeconds;     /* Used in conversion between meter time and UTC time.   */
      uint32_t intervalsDiff;    /* Number of intervals between last valid interval and interval at requested close time. */

      tblTime.seconds = 0;       /* Meter doesn't record the seconds value */

      (void)TIME_UTIL_ConvertMtrHighFormatToSysFormat( &tblTime, &sTime);           /* sTime is meter UTC date in sysTime_t  */
      TIME_UTIL_ConvertSysFormatToSeconds( &sTime, &meterSeconds, &microSeconds );  /* Convert meter time time to seconds since epoch */

#if LP_VERBOSE
      /* convert to system time  */
      sysTime_dateFormat_t ttemp;
      (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &ttemp );             /* convert to date format  */
      INFO_printf( "Last end local date = %02hhu/%02hhu/%4hu %02hhu:%02hhu:00", tblTime.month, tblTime.day, tblTime.year + 2000U,
                     tblTime.hours, tblTime.minutes );
      INFO_printf( "Last end UTC   date = %02hhu/%02hhu/%4hu %02hhu:%02hhu:00", ttemp.month, ttemp.day, ttemp.year, ttemp.hour, ttemp.min );
      INFO_printf( "MeterSeconds=0x%08X closeTime=0x%08X, diff = %u", meterSeconds, closeTime, meterSeconds - closeTime );
#endif

      /* Compute number of intervals of target interval before most current interval in table 64   */
      intervalsDiff = ( ( meterSeconds - closeTime ) / ( SECONDS_PER_MINUTE * tbl61_.lpDataSet1.nbrMaxIntTimeSet ) );

      /* Check for requested interval with range of table 64 */
      if ( intervalsDiff < ( ( ( tbl63_.nbrValidBlocks - 1U ) * tbl61_.lpDataSet1.nbrBlksIntsSet ) + tbl63_.nbrValidInt ) )
      {
         uint16_t blocksDiff; /* Number of blocks before last valid block  */
#if LP_VERBOSE
         INFO_printf( "intervalsDiff: (%u - %u) / %hu =  %u", meterSeconds, closeTime, 60U * tbl61_.lpDataSet1.nbrMaxIntTimeSet, intervalsDiff );
#endif
         if ( intervalsDiff < tbl63_.nbrValidInt )    /* Requested range is in the last block.  */
         {
            blocksDiff = (uint16_t)tbl63_.lastBlockElement; /* Index of last valid block */;
         }
         else
         {
            blocksDiff = (uint16_t)((( intervalsDiff - tbl63_.nbrValidInt ) / tbl61_.lpDataSet1.nbrBlksIntsSet ) + 1);  /* full blocks to back up. */

            /* Check for wrapping around! */
            /* offset currently points to start of last valid block in tbl 64.   */
            if ( blocksDiff > tbl63_.lastBlockElement )
            {
               offset  = (uint32_t)( blocksDiff - tbl63_.lastBlockElement ) * LP_Config_.blockSize;   /* Point to start of taret block */
            }
            else
            {
               offset -= blocksDiff * LP_Config_.blockSize;                      /* Offset of block with requested values  */
            }
            blocksDiff = (uint16_t)( offset / LP_Config_.blockSize );

#if LP_VERBOSE
            INFO_printf( "target blk readings start @: 0x%X", offset );
#endif
            intervalsDiff %= tbl61_.lpDataSet1.nbrBlksIntsSet;                /* intervalsDiff now is difference from last valid interval. */


         }

         uint32_t blockStartSeconds;
         uint32_t searchOffset;
         uint32_t lastMeterSeconds  = 0;
         bool     firstCheck        = (bool)true;  /* true first estimate being verified. */
         bool     found             = (bool)false; /* Target interval found   */

         offset = blocksDiff * LP_Config_.blockSize;
         searchOffset = offset;
         do
         {
            /* Check for block being the last valid block in the meter and set number of intervals this block, accordingly */
            if ( searchOffset == ( ( tbl63_.nbrValidBlocks - 1U ) * LP_Config_.blockSize ) )
            {
               lastIntThisBlock = tbl63_.nbrValidInt;                /* Number of intervals in this block   */
            }
            else
            {
               lastIntThisBlock = tbl61_.lpDataSet1.nbrBlksIntsSet;  /* Number of intervals in this block   */
            }

            /* Read block of interest end time  */
            (void)INTF_CIM_CMD_ansiRead( &tblTime, STD_TBL_LOAD_PROFILE_DATASET_1, searchOffset, (uint16_t)sizeof( stdTbl64Time_t ) );
            (void)TIME_UTIL_ConvertMtrHighFormatToSysFormat( &tblTime, &sTime);                 /* sTime is block end UTC date in sysTime_t  */
            TIME_UTIL_ConvertSysFormatToSeconds( &sTime, &meterSeconds, &microSeconds );        /* Convert meter time time to seconds since epoch */
            blockStartSeconds = meterSeconds - ( ( lastIntThisBlock - 1U ) * ( tbl61_.lpDataSet1.nbrMaxIntTimeSet * SECONDS_PER_MINUTE ) );
#if LP_VERBOSE
            (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &ttemp );             /* convert to date format  */
            INFO_printf( "searchOffset: 0x%lX", searchOffset );
            INFO_printf( "Block end   UTC date: %02hhu/%02hhu/%4hu %02hhu:%02hhu:00", ttemp.month, ttemp.day, ttemp.year, ttemp.hour, ttemp.min );
            TIME_UTIL_ConvertSecondsToSysFormat( blockStartSeconds, 0, &sTime );
            DST_ConvertUTCtoLocal( &sTime );                                              /* Convert UTC to local */
            (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &ttemp );             /* convert to date format  */
            INFO_printf( "Block start UTC date: %02hhu/%02hhu/%4hu %02hhu:%02hhu:00", ttemp.month, ttemp.day, ttemp.year, ttemp.hour, ttemp.min );
            INFO_printf( "block start secs 0x%08X requested secs 0x%08X block end secs 0x%08lX", blockStartSeconds, closeTime, meterSeconds );
#endif
            /* Check for close time between block start time and block end time. */
            if ( ( closeTime <= meterSeconds ) && ( closeTime >= blockStartSeconds ) )
            {
               found = (bool)true;
               break;
            }
            else if ( closeTime < blockStartSeconds ) /* target block is before this one. */
            {
#if LP_VERBOSE
               INFO_printf( "requested date before block start date" );
#endif
               if ( firstCheck )
               {
                  firstCheck = (bool)false;
                  lastMeterSeconds = meterSeconds + 1;
               }
               if ( meterSeconds < lastMeterSeconds ) /* Check that last meter time was after this one. If not, there's no need to search further. */
               {
                  if ( searchOffset < LP_Config_.blockSize ) /* Check for wrap backwards   */
                  {
                     searchOffset = (uint32_t)( tbl63_.lastBlockElement * LP_Config_.blockSize );  /* Beginning of block with most recent interval */
                  }
                  else
                  {
                     searchOffset -= (uint32_t)LP_Config_.blockSize; /* Beginning of previous block   */
                  }
               }
               else  /* While looking backwards in time, found a meter time that moved forward. No need to look any further.  */
               {
#if LP_VERBOSE
                  INFO_printf( "Aborting backward search.\n" );
#endif
                  break;
               }
            }
            else  /* target block is after this one.  */
            {
#if LP_VERBOSE
               INFO_printf( "requested date after block end date" );
#endif
               if ( firstCheck )
               {
                  firstCheck = (bool)false;
                  lastMeterSeconds = meterSeconds - 1;
               }
               if ( meterSeconds > lastMeterSeconds ) /* Check that last meter time was before this one. If not, there's no need to search further. */
               {
                  if ( searchOffset >= (uint32_t)( tbl63_.lastBlockElement * LP_Config_.blockSize ) ) /* Check for wrap forward  */
                  {
                     searchOffset = 0;  /* First block */
                  }
                  else
                  {
                     searchOffset += (uint32_t)LP_Config_.blockSize; /* Beginning of previous block   */
                  }
               }
               else  /* While looking forward in time, found a meter time that moved backward. No need to look any further.  */
               {
#if LP_VERBOSE
                  INFO_printf( "Aborting forward search.\n" );
#endif
                  break;
               }
            }
            /* Check for wrapped all the way around without finding the target block.  */
            if ( searchOffset == offset )
            {
               break;
            }

            lastMeterSeconds = meterSeconds;
         } while ( !found );
         if ( found )
         {
            offset = searchOffset + LP_Config_.readOffset;   /* Start of block with target interval */
            intervalsDiff = ( ( closeTime - blockStartSeconds ) / ( SECONDS_PER_MINUTE * tbl61_.lpDataSet1.nbrMaxIntTimeSet ) );
#if LP_VERBOSE
            INFO_printf( "Interval: %hu", intervalsDiff );
#endif
            offset += intervalsDiff * ( LP_Config_.statusBytes + LP_Config_.dataBytes );
            meterSeconds = closeTime;
         }
         else
         {
            meterSeconds = closeTime + 1;
#if LP_VERBOSE
            INFO_printf( "\nInterval not found.\n" );
#endif
         }

#if ( ( HMC_REQ_MAX_BYTES ) < ( ID_MAX_CHANNELS * 2U ) )   /* Just the reading portion of the record is too large.  */
#error "ID_MAX_CHANNELS is too large to be able to read one LP record!"
#elif ( ( ID_MAX_CHANNELS ) > ( ( 2 * (  HMC_REQ_MAX_BYTES - 1U ) ) / 3U ) )
         /* Break read into status bytes and data bytes to keep from exceeding psem packet size limit */
         if (  ( meterSeconds == closeTime )                                                                            &&
               ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &Precord[0], STD_TBL_LOAD_PROFILE_DATASET_1, offset,
                                       (uint16_t)( LP_Config_.statusBytes ) ) )                                         &&
               ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &LPrecord[LP_Config_.statusBytes], STD_TBL_LOAD_PROFILE_DATASET_1,
                                       offset + (uint16_t)LP_Config_.statusBytes, (uint16_t)LP_Config_.dataBytes ) )
            )  /* read all and copy later to save time */
#else
         if (  ( meterSeconds == closeTime )                                                                            &&
               ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &LPrecord[0], STD_TBL_LOAD_PROFILE_DATASET_1, offset,
                                       (uint16_t)( LP_Config_.statusBytes + LP_Config_.dataBytes ) ) )
            )  /* read all and copy later to save time */
#endif
         {
            (void)memcpy( (uint8_t *)&tblFlags, &LPrecord[0], LP_Config_.statusBytes );
            (void)memcpy( (uint8_t *)&chVals[0], &LPrecord[LP_Config_.statusBytes], LP_Config_.dataBytes );
#if LP_VERBOSE
            INFO_printf(   "offset=0x%x(%lu) len %hu Byte0=0x%hhX Bits=%hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu  "
                           "Byte1=0x%hhX Bits=%hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu "
                           "Byte2=0x%hhX Bits=%hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu Data=%hu %hu %hu %hu",
                           offset, offset, (uint16_t)( LP_Config_.statusBytes + LP_Config_.dataBytes ), tblFlags.bytes[0],
                           tblFlags.bits.b07, tblFlags.bits.b06, tblFlags.bits.b05, tblFlags.bits.b04, tblFlags.bits.b03, tblFlags.bits.b02,
                           tblFlags.bits.b01, tblFlags.bits.b00,
                           tblFlags.bytes[1],
                           tblFlags.bits.b17, tblFlags.bits.b16, tblFlags.bits.b15, tblFlags.bits.b14, tblFlags.bits.b13, tblFlags.bits.b12,
                           tblFlags.bits.b11, tblFlags.bits.b10,
                           tblFlags.bytes[2],
                           tblFlags.bits.b27, tblFlags.bits.b26, tblFlags.bits.b25, tblFlags.bits.b24, tblFlags.bits.b23, tblFlags.bits.b22,
                           tblFlags.bits.b21, tblFlags.bits.b20 );
#elif LP_DUMP_VALUES
//            INFO_printf(   "offset=0x%x (%lu)", offset, offset );
#endif

            /* Add the data to the buffer */
            uint8_t ch;

            for ( ch = 0; ch < tbl61_.lpDataSet1.nbrChnsSet; ch++ )
            {
               qualCodes_u qualCodes;
               uint64_t    sampleVal;
               uint16_t    divisor = 1;
               uint16_t    scalar = 1;
               uint8_t     RdgInfo;       /* Bit field ahead of every reading qty */
               uint16_t    RdgType;       /* Reading identifier */
               uint8_t     powerOf10Code;
               uint8_t     numberOfBytes;

               qualCodes.byte = 0;        /* Default   */
               qualCodes.flags.clockChanged = ( tblFlags.bits.b07 || tblFlags.bits.b06 );
               qualCodes.flags.powerFail    = tblFlags.bits.b05;
               if ( ( ch % 2 ) == 0 )     /* Even channel   */
               {
                  qualCodes.flags.qualCode = tblFlags.bytes[( ch + 1 ) / 2] & 0x0F;
               }
               else                       /* Odd channel */
               {
                  qualCodes.flags.qualCode = ( tblFlags.bytes[( ch + 1 ) / 2] & 0xF0 ) >> 4;
               }
               if ( tbl61_.lpFlags.scalarDivisorFlagSet1 == true )
               {
                  divisor = tbl62_.divisors_set[ch];
                  scalar = tbl62_.scalarsSet[ch];
               }
#if LP_VERBOSE || LP_DUMP_VALUES
               INFO_printf(   "Ch=%2hhu clockChange=%hhu PowerFail=%hhu QC=%hhu scalar=%hu divisor=%3hu powerof10=%hhu multiplier=%hu val=%hu",
                              ch, qualCodes.flags.clockChanged, qualCodes.flags.powerFail, qualCodes.flags.qualCode, scalar, divisor,
                              LP_Config_.powerOf10[ch], LP_Config_.multiplier[ch], chVals[ch] );
#else
//               INFO_printf(   "Ch=%2hhu val=%hu", ch, chVals[ch] );
#endif
               sampleVal = chVals[ch];
               if ( LP_Config_.multiplier[ch] != 0 )
               {
                  sampleVal = ( sampleVal * divisor * LP_Config_.multiplier[ch] ) / scalar;
               }
               /* get the powerof10code and number of bytes */
               powerOf10Code = HEEP_getPowerOf10Code( LP_Config_.powerOf10[ch], (int64_t *)&sampleVal );
               numberOfBytes = HEEP_getMinByteNeeded( (int64_t)sampleVal, intValue, 0);

               if ( needHeader )
               {
                  if ( room >= ( VALUES_INTERVAL_END_SIZE + sizeof( rdgCount ) ) +  /* Room for header   */
                     ( PackQC( qualCodes.byte, &packCfg, (bool)false ) + numberOfBytes + sizeof( RdgType ) + sizeof( RdgInfo ) ) ) /* Room for rdg */
                  {
                     headerAdded = (bool)true;
                     /* Add the bit structure fields  */
                     uint32_t msgTime;

                     buf[indexOfEvtRdgQty] = READING_QTY_MAX;       /* Update the current set  */
                     msgTime = closeTime;   /* Add the timestamp  */
                     PACK_Buffer( -( VALUES_INTERVAL_END_SIZE_BITS ), ( uint8_t * )&msgTime, &packCfg );
                     len += VALUES_INTERVAL_END_SIZE;

                     /* Insert the reading quantity, it will be adjusted later to the correct value   */
                     indexOfEvtRdgQty = (uint16_t)len; /* Update the offset to this mini-header rdtqty */
                     rdgCount = 0;
                     PACK_Buffer( sizeof( rdgCount ) * 8, ( uint8_t * )&rdgCount, &packCfg );
                     len += (int16_t)sizeof( rdgCount );
                     needHeader = (bool)false; /* Just added a "mini-header" */
                     room -= ( VALUES_INTERVAL_END_SIZE + sizeof( rdgCount ) );
                  }
                  else  /* No room for more; exit  */
                  {
                     len = -1;
                     break;
                  }
               }
               /* Check for enough room to add reading   */
               rdgSize = PackQC( qualCodes.byte, &packCfg, (bool)false ) + numberOfBytes + sizeof( RdgType ) + sizeof( RdgInfo );
               if ( room >= rdgSize )
               {
                  /* Pack rest of the data, including the quality code  */
                  RdgInfo = 0; /* No timestamp present or quality codes present  */
                  if ( qualCodes.byte != 0 )
                  {
                     RdgInfo |= 0x40; /* Quality codes present */
                  }
                  RdgInfo |= (uint8_t)( powerOf10Code << COMPACT_MTR_POWER10_SHIFT );
                  RdgInfo |= (uint8_t)( (uint8_t)( numberOfBytes - 1U ) << COMPACT_MTR_RDG_SIZE_SHIFT );
                  /*  Insert the reading info bit field  */
                  PACK_Buffer( (int16_t)sizeof( RdgInfo ) * 8, &RdgInfo, &packCfg );
                  len += (int16_t)sizeof( RdgInfo );

                  RdgType = LP_Config_.readingType[ch];
                  PACK_Buffer( -(int16_t)( sizeof( RdgType ) * 8 ), ( uint8_t * )&RdgType, &packCfg );
                  len += (int16_t)sizeof( RdgType );
                  len += (int16_t)PackQC( qualCodes.byte, &packCfg, (bool)true ); /* Add QC */

                  /* get the correct number of bytes and pack the data  */
                  PACK_Buffer( -(int16_t)( numberOfBytes * 8 ), ( uint8_t * )&sampleVal, &packCfg );
                  len += numberOfBytes;

                  ++rdgCount;
                  if ( rdgCount >= READING_QTY_MAX )
                  {
                     needHeader = (bool)true; /* Need to add a "mini-header" */
                  }
                  room -= rdgSize;  // Already accounts for each of the len +='s above
               }
               else  /* No room left   */
               {
                  len = -1;
                  break;
               }
            }  /* Channel reading loop */
         }  /* Matching times and able to read table 64   */
      }  /* Requested interval exists within table 64 */
      else
      {
         INFO_printf( "Interval too old." );
      }
   }  /* Able to read table 64   */
   if ( headerAdded )
   {
      /* Correct the event count */
      buf[indexOfEvtRdgQty] = rdgCount;
   }
   return len;
}
#endif

/***********************************************************************************************************************

   Function name: ID_task

   Purpose: This is the interval data task.

   Arguments: uint32_t Arg0

   Returns: None

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void ID_task( taskParameter )
{
   buffer_t*    pMessage = NULL;
   tTimeSysMsg  timeMsg;             /* Stores the time message   */
#if ( LP_IN_METER == 0 )
   uint8_t      ch;                  /* Channel currently be operated on   */
#endif
   uint32_t     uDatePendingMessage = 0;
   uint32_t     uTimeDiversity;
   uint32_t     uTimePendingMessage = 0;
   uint8_t      trafficClass;
   uint8_t      status;

#if ( LP_IN_METER == 0 )
   bModuleBusy_ = (bool)true; /* Keeps ID busy  */
#endif
   while( !HMC_MTRINFO_Ready() )
   {
      OS_TASK_Sleep(ONE_SEC); /* Wait for HMC to be ready after device power up, response message may need to respond with data from the meter */
   }

#if ( REMOTE_DISCONNECT == 1 )
   uint8_t uRCDCSwitchPresent;
   (void)INTF_CIM_CMD_getRCDCSwitchPresent( &uRCDCSwitchPresent, (bool)true );
#endif

   /* Set a 5 minute periodic alarm. All the channels will be called every 5 minutes.
      The channels will run only, if its their time to run */
   {
      tTimeSysPerAlarm alarmSettings;                    /* Configure the periodic alarm for time */
      alarmSettings.bOnInvalidTime = false;              /* Only alarmed on valid time, not invalid */
      alarmSettings.bOnValidTime = true;                 /* Alarmed on valid time */
#if ( LP_IN_METER == 1 )
      alarmSettings.bSkipTimeChange = true;              /* Do not notify on time change */
#else
      alarmSettings.bSkipTimeChange = false;             /* Notify on time change */
#endif
      alarmSettings.bUseLocalTime = false;               /* Use UTC time */
      alarmSettings.pQueueHandle = NULL;                 /* do NOT use queue */
      alarmSettings.pMQueueHandle = &mQueueHandle_;      /* Use the message Queue */
      alarmSettings.ucAlarmId = 0;                       /* 0 will create a new ID, anything else will reconfigure. */
#if ( LP_IN_METER == 1 )
      alarmSettings.ulOffset = TIME_TICKS_PER_MIN * 2;   /* 2 minute offset from LP boundary */
#else
      alarmSettings.ulOffset = 0;                        /* No offset, send queue on the boundary */
#endif
      alarmSettings.ulPeriod = TIME_TICKS_PER_5MIN;      /* Sample rate */
      (void)TIME_SYS_AddPerAlarm( &alarmSettings );
   }

   (void)memset( &timeMsg, 0, sizeof( timeMsg ) ); /* Clear the time message  */
#if ( LP_IN_METER == 0 )
   SYSBUSY_setBusy();
   bModuleBusy_ = (bool)true;
   for ( ch = 0; ch < (uint8_t)ID_MAX_CHANNELS; ch++ )
   {
      readingAtPwrUp_[ch] = 0;
      INFO_printf( "INIT LP Task Ch: %u", ch );
      IntervalDataCh( ch, &timeMsg ); /* Send power-up message to each channel  */
   }
   bModuleBusy_ = (bool)false;
   SYSBUSY_clearBusy();
#else
   if ( LP_Config_.loadTables )
   {
      LoadLPTables();
   }
#endif

   for ( ;; )  /* Task Loop */
   {
      buffer_t  *pBuf;  /* Buffer used to point to the data in the msg queue   */
      (void)OS_MSGQ_Pend( &mQueueHandle_, ( void * )&pBuf, OS_WAIT_FOREVER );

      if ( eSYSFMT_TIME_DIVERSITY == pBuf->eSysFmt )
      {
         /*  Time diveristy timer expired, send pending message. Send the message to message handler. The called function will free the buffer. */
         if ( NULL != pMessage )
         {
            if ( heepHdr.Method_Status != RequestedEntityTooLarge )
            {
               (void)HEEP_MSG_Tx( &heepHdr, pMessage );
            }
            else  /* No content to send, but must send header with status. */
            {
               (void)HEEP_MSG_TxHeepHdrOnly( &heepHdr );
            }
            pMessage = NULL;
         }
      }

      else
      {
         (void)memcpy( (uint8_t *)&timeMsg, &pBuf->data[0], sizeof( timeMsg ) ); /*lint !e740  */
         BM_free( pBuf );

         INFO_printf( "\n\n\n" );
         INFO_printf( "LP Date=%lu Time=%lu", timeMsg.date, timeMsg.time );
#if ( LP_IN_METER == 1 )
         if ( LP_Config_.configError || LP_Config_.tableReadError )  /* Meter not configured correctly or error reading confif tables  */
         {
            INFO_printf( "Meter LP config error. Retrying reading LP configuration" );
            LoadLPTables();
            continue;
         }
#endif
         trafficClass = ID_getQos();

#if ( LP_IN_METER == 0 )
         SYSBUSY_setBusy();
         bModuleBusy_ = (bool)true;
         for ( ch = 0; ch < (uint8_t)ID_MAX_CHANNELS; ch++ )
         {
#if ( REMOTE_DISCONNECT == 1 )
            if ( !runI210Channel( ch ) )
            {
               continue;
            }
#endif
            IntervalDataCh( ch, &timeMsg );            /* Run the channel  */
         }
         bModuleBusy_ = (bool)false;
         SYSBUSY_clearBusy();
#endif

#if ( LP_IN_METER == 1 )
         timeMsg.time -= ( timeMsg.time % TIME_TICKS_PER_5MIN );
         INFO_printf( "LP New Date=%lu Time=%lu", timeMsg.date, timeMsg.time );
         /* convert to date format  */
         sysTime_dateFormat_t ttemp;
         sysTime_t            sTime;

         sTime.date = timeMsg.date;
         sTime.time = timeMsg.time;
         (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &ttemp );
         INFO_printf( "LP New Date Time = %02d/%02d/%02d %02d:%02d:00", ( ttemp.year - 2000U ), ttemp.month, ttemp.day, ttemp.hour, ttemp.min );
#endif

         if ( fileParams_.lpBuEnabled && buScheduleInMs != 0 ) /* Bubble up enabled */
         {
            if ( 0 == ( timeMsg.time % buScheduleInMs ) )      /* Time to bubble up */
            {
               if ( NULL != pMessage ) /* If a previous message is pending due to time diversity, go ahead and send it. */
               {
                  /*  Make sure that the message pending because of time diversity is older than the message   */
                  /*  which is about to be generated.  This could happen because of a backwards time jump.  */
                  if (  ( uDatePendingMessage < timeMsg.date ) ||
                        ( ( uDatePendingMessage == timeMsg.date ) && ( uTimePendingMessage < timeMsg.time ) ) )
                  {
                     if ( heepHdr.Method_Status != RequestedEntityTooLarge )
                     {
                        (void)HEEP_MSG_Tx( &heepHdr, pMessage );
                     }
                     else  /* No content to send, but must send header with status. */
                     {
                        (void)HEEP_MSG_TxHeepHdrOnly( &heepHdr );
                     }
                     pMessage = NULL;
                  }
                  else  /* Else the pending message is not older than the new message about to be generated. Just discard it. */
                  {
                     BM_free( pMessage );
                     pMessage = NULL;
                  }
               }

               pMessage = IntervalBubbleUp( &timeMsg, trafficClass, &status );   /*lint !e838 pMessage was set to NULL previously */

               if ( ( status == (uint8_t)OK ) || ( status == (uint8_t)PartialContent ) )  /* If OK or PartialContent, then pMessage is NOT NULL */
               {
                  /*  Delay for a random amount of time based on time diversity. */
                  if ( fileParams_.lpBuMaxTimeDiversity > 0 )
                  {
                     uTimeDiversity = aclara_randu( 0, ( (uint32_t)fileParams_.lpBuMaxTimeDiversity ) * TIME_TICKS_PER_MIN );
                     INFO_printf( "LP Time Diversity: %u Milliseconds", uTimeDiversity );
                     (void)ID_SetBubbleUpTimer( uTimeDiversity );

                     /* Save the date and time from the alarm of the pending message. This is needed to
                        determine if the message needs to be discarded in case of a time jump. */
                     uDatePendingMessage = timeMsg.date;
                     uTimePendingMessage = timeMsg.time;
                  }

                  else  /*  Else, not using time diversity, send the message immediately. */
                  {
                     if ( NULL != pMessage )
                     {
                        if ( heepHdr.Method_Status != RequestedEntityTooLarge )
                        {
                           /* Send the message to message handler. The called function will free the buffer */
                           (void)HEEP_MSG_Tx( &heepHdr, pMessage );
                        }
                        else  /* No content to send, but must send header with status. */
                        {
                           (void)HEEP_MSG_TxHeepHdrOnly( &heepHdr );
                        }
                        pMessage = NULL;
                     }
                  }
               }
               else  /* Only other value returned by IntervalBubbleup() is RequestedEntityTooLarge */
               {
                  /* Need to send a Header only message with RequestedEntityTooLarge as the Method_Status   */
                  (void)HEEP_MSG_TxHeepHdrOnly( &heepHdr );
               }
            }
         }
      }
   }
} /*lint !e715 !e438 func param not used, varialble last value not used */
/***********************************************************************************************************************

   Function Name: ID_isModuleBusy()

   Purpose: Returns if module is busy or idle

   Arguments: None

   Returns: bool - true, if busy, false otherwise

   Side Effects: None

   Reentrant Code: yes

 **********************************************************************************************************************/
bool ID_isModuleBusy( void )
{
#if ( LP_IN_METER == 0 )
   return bModuleBusy_;
#else
   return false;
#endif
}

/***********************************************************************************************************************

   Function Name: ID_SetBubbleUpTimer(uint32_t timeMilliseconds)

   Purpose: Create the timer to re-send the interval data after the given number
            of milliseconds have elapsed.

   Arguments: uint32_t timeRemainingMSec - The number of milliseconds until the timer goes off.

   Returns: returnStatus_t - indicates if the timer was successfully created

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
static returnStatus_t ID_SetBubbleUpTimer( uint32_t timeMilliseconds )
{
   returnStatus_t retStatus;

   if ( 0 == bubbleUpTimerId )
   {
      /* First time, add the timer  */
      timer_t tmrSettings;             /*  Timer for sending pending ID bubble up message */

      (void)memset( &tmrSettings, 0, sizeof( tmrSettings ) );
      tmrSettings.bOneShot       = true;
      tmrSettings.bOneShotDelete = false;
      tmrSettings.bExpired       = false;
      tmrSettings.bStopped       = false;
      tmrSettings.ulDuration_mS  = timeMilliseconds;
      tmrSettings.pFunctCallBack = ID_bubbleUpTimerExpired;
      retStatus = TMR_AddTimer( &tmrSettings );
      if ( eSUCCESS == retStatus )
      {
         bubbleUpTimerId = (uint16_t)tmrSettings.usiTimerId;
      }
      else
      {
         ERR_printf( "ERROR: LP unable to add bubble up timer" );
      }
   }
   else  /* Timer already exists, reset the timer  */
   {
      retStatus = TMR_ResetTimer( bubbleUpTimerId, timeMilliseconds );
   }
   return( retStatus );
}
/***********************************************************************************************************************

   Function Name: ID_bubbleUpTimerExpired(uint8_t cmd, void *pData)

   Purpose: Called by the timer to send the interval data bubble up message.  Time diveristy for interval
            data is implemented in this manner so that the interval data reads are not interrupted.
            Interval data is read every 5 minutes but can be configured to be send to the meter as
            infrequently as once per hour.  So it is possible that because of time diversity, it will
            be necessary to perform a new interval data reading while a previous reading it still
            pending to be bubbled up.
            Since this code is called from within the timer task, we can't muck around with the
            timer, so we need to notify the app message class to do the actual work.

   Arguments:  uint8_t cmd - from the ucCmd member when the timer was created.
               void *pData - from the pData member when the timer was created

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
/*lint -esym(715, cmd, pData ) not used; required by API  */
static void ID_bubbleUpTimerExpired( uint8_t cmd, void *pData )
{
   static buffer_t buf;

   /* Use a static buffer instead of allocating one because of the allocation fails, there wouldn't be much we could do - we couldn't reset the timer
      to try again because we're already in the timer callback.  */
   BM_AllocStatic( &buf, eSYSFMT_TIME_DIVERSITY );
   OS_MSGQ_Post( &mQueueHandle_, ( void * )&buf );
}  /*lint !e818 pData could be pointer to const */
/*lint +esym(715, cmd, pData ) not used; required by API  */
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: cfgAllIntervalData

   Purpose: Configures all channels on configuration change.

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
static void cfgAllIntervalData( void )
{
   idChCfg_t idChCfg;

   (void)memset( ( uint8_t * )&idChCfg, 0, sizeof( idChCfg ) );
#if ( COMMERCIAL_METER == 0 )
   /* Set up Channels 0-5 */
   idChCfg.sampleRate_mS   = (uint32_t)LP_INTERVAL_IN_MINUTES * TIME_TICKS_PER_MIN;
   idChCfg.storageLen      = 3;
   idChCfg.mode            = ID_MODE_DELTA_e;
   idChCfg.trimDigits      = 0;

   idChCfg.rdgTypeID       = (uint16_t)fwdkWh;
   (void)ID_cfgIntervalData( 0, &idChCfg );
   idChCfg.rdgTypeID       = (uint16_t)revkWh;
   (void)ID_cfgIntervalData( 1, &idChCfg );

   idChCfg.mode            = ID_MODE_ABS_e;
   idChCfg.rdgTypeID       = (uint16_t)vRMS;
   (void)ID_cfgIntervalData( 2, &idChCfg );

   idChCfg.rdgTypeID       = (uint16_t)temperature;
   (void)ID_cfgIntervalData( 3, &idChCfg );

   (void)memset( ( uint8_t * )&idChCfg, 0, sizeof( idChCfg ) ); /* Disable the following channels   */
   (void)ID_cfgIntervalData( 4, &idChCfg );
   (void)ID_cfgIntervalData( 5, &idChCfg );
#if ( ID_MAX_CHANNELS > 6 )
   (void)ID_cfgIntervalData( 6, &idChCfg );
   (void)ID_cfgIntervalData( 7, &idChCfg );
#endif
#else
   idChCfg.sampleRate_mS   = (uint32_t)LP_INTERVAL_IN_MINUTES * TIME_TICKS_PER_MIN;
   idChCfg.storageLen      = 3;
   idChCfg.mode = ID_MODE_DELTA_e;
   idChCfg.trimDigits      = 5;

   idChCfg.rdgTypeID       = (uint16_t)fwdkWh;
   (void)ID_cfgIntervalData( 0, &idChCfg );
   idChCfg.rdgTypeID       = (uint16_t)revkWh;
   (void)ID_cfgIntervalData( 1, &idChCfg );
   idChCfg.rdgTypeID       = (uint16_t)fwdkVArh;
   (void)ID_cfgIntervalData( 2, &idChCfg );

   idChCfg.mode = ID_MODE_ABS_e;
   idChCfg.trimDigits = 0;

   idChCfg.rdgTypeID       = (uint16_t)vRMSA;
   (void)ID_cfgIntervalData( 3, &idChCfg );
   idChCfg.rdgTypeID       = (uint16_t)vRMSB;
   (void)ID_cfgIntervalData( 4, &idChCfg );
   idChCfg.rdgTypeID       = (uint16_t)vRMSC;
   (void)ID_cfgIntervalData( 5, &idChCfg );
#if ( ID_MAX_CHANNELS > 6 )
   (void)memset( (uint8_t *)&idChCfg, 0, sizeof( idChCfg ) ); /* Disable the following channels   */
   (void)ID_cfgIntervalData( 6, &idChCfg );
   (void)ID_cfgIntervalData( 7, &idChCfg );
#endif
#endif
}
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: ID_cfgIntervalData

   Purpose: Configures a channel of interval data.

   Arguments: uint8_t ch, idChCfg_t const *pIdCfg

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t ID_cfgIntervalData( uint8_t ch, idChCfg_t const *pIdCfg )
{
   returnStatus_t retVal;
   chCfg_t        chCfg;               /* Contains the configuration of the channel */

   retVal = eAPP_ID_CFG_CH;
   if ( ch < (uint8_t)ID_MAX_CHANNELS )   /* Make sure max channels isn't exceeded */
   {
      retVal = eNV_EXTERNAL_NV_ERROR;
      if ( eSUCCESS == readCfg( ch, &chCfg ) ) /* Read the configuration */
      {
         retVal = eAPP_ID_CFG_SAMPLE;
         if ( 0 != pIdCfg->sampleRate_mS ) /* Is sample rate valid? */
         {
            /* Validate the configuration */
            retVal = eAPP_ID_CFG_PARAM;

            /* Valid configuration? */
            if (  ( 0 == ( ID_SAMPLE_RATE_24H % pIdCfg->sampleRate_mS ) )                                            &&
                  ( ( ID_MAX_STORAGE_LEN >= pIdCfg->storageLen ) && ( ID_MIN_STORAGE_LEN <= pIdCfg->storageLen ) )   &&
                  ( ( ID_MODE_DELTA_e == pIdCfg->mode ) || ( ID_MODE_ABS_e == pIdCfg->mode ) ) )
            {
               if ( memcmp( pIdCfg, &chCfg.cfg, sizeof( chCfg.cfg ) ) ) /* Did the config change? */
               {
                  PartitionTbl_t  const   *pPartFunctions = &PAR_partitionFptr;  /* get pointer to partition table */
                  lCnt partSize; /* Contains partition size */

                  (void)memcpy( &chCfg.cfg, pIdCfg, sizeof( chCfg.cfg ) ); /* Copy the new config */
                  (void)memset( &chCfg.metaData, 0, sizeof( chCfg.metaData ) );
                  chCfg.metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_CONFIGURATION_CHANGED_QC_e,
                        ( ID_QC_e )chCfg.metaData.qualCodes.flags.qualCode );
                  retVal = ePAR_NOT_FOUND;
                  if ( eSUCCESS == pPartFunctions->parSize( partId_[ch], &partSize ) )
                  {
                     uint32_t dataLen = chCfg.cfg.storageLen + sizeof( chCfg.metaData.qualCodes.byte );

                     retVal = eNV_EXTERNAL_NV_ERROR;
                     chCfg.metaData.totNumInts = partSize / dataLen;
                     chCfg.metaData.maxValidInts = ( partSize - EXT_FLASH_ERASE_SIZE ) / dataLen;

                     OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */

                     if ( eSUCCESS == FIO_fwrite( &fileHndlMeta_, ( fileOffset )offsetof( idFileDataMeta_t,
                                                   metaData ) + ( ch * sizeof( chCfg.metaData ) ), ( uint8_t * )&chCfg.metaData,
                                                   ( lCnt )sizeof( chCfg.metaData ) ) )
                     {
                        if ( eSUCCESS == FIO_fwrite( &fileHndlCfg_, ( fileOffset )offsetof( idFileDataCfg_t, cfg ) + ( ch * sizeof( chCfg.cfg ) ),
                                                     ( uint8_t * )&chCfg.cfg, ( lCnt )sizeof( chCfg.cfg ) ) )
                        {
                           (void)PAR_partitionFptr.parErase( 0, partSize, pIdPartitions_[ch] );
                           (void)FIO_fflush( &fileHndlMeta_ );
                           (void)FIO_fflush( &fileHndlCfg_ );
                           retVal = eSUCCESS;
                        }
                     }
                     (void)readCfg( ch, &chCfg_[ch] ); /* Read the configuration data  */
                     OS_MUTEX_Unlock( &idMutex_ );       /* Function will not return if it fails */
                  }
               }
            }
         }  /* if (0 != sampleRate_S) - Sample rate valid? */
         else  /* Sample rate is invalid */
         {
            OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */

            (void)memset( &chCfg.cfg, 0, sizeof( chCfg.cfg ) );
            if ( eSUCCESS == FIO_fwrite( &fileHndlCfg_, ( fileOffset )offsetof( idFileDataCfg_t, cfg ) + ( ch * sizeof( chCfg.cfg ) ),
                                         ( uint8_t * )&chCfg.cfg, ( lCnt )sizeof( chCfg.cfg ) ) )
            {
               (void)FIO_fflush( &fileHndlCfg_ );
               (void)readCfg( ch, &chCfg_[ch] ); /* Read the configuration data  */
               retVal = eSUCCESS;
            }
            OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
         }
      }
   }
   return( retVal );
}
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: ID_rdIntervalData

   Purpose: Read interval data.  The interval requested must be at the "ending" boundary.  For example, if the 1:45 to
            1:50 interval is requested, the requested time will be 1:50.  If an interval is requested that isn't on an
            interval boundary, in the future or too far in the past, an error response will be sent.

   Arguments: char *pDest, uint8_t ch, sysTime_t *pReadingTime

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t ID_rdIntervalData( uint8_t *pQual, double *pVal, uint8_t ch, sysTime_t const *pReadingTime )
{
   returnStatus_t retVal;

   *pVal = 0;     /* Initialize the result, if an error occurs, at least 0 is returned vs. garbage */
   *pQual = 0;    /* Initialize the result, if an error occurs, at least 0 is returned vs. garbage */

   retVal = eAPP_ID_CFG_CH;
   if ( ch < (uint8_t)ID_MAX_CHANNELS ) /* Make sure max channels isn't exceeded  */
   {
      OS_MUTEX_Lock( &idMutex_ );         /* Function will not return if it fails   */

      retVal = eAPP_ID_REQ_FUTURE;
      uint64_t readingTime = TIME_UTIL_ConvertSysFormatToSysCombined( pReadingTime ); /* combine reading time */
      if ( readingTime <= chCfg_[ch].metaData.smplDateTime )     /* Requested data in the future? */
      {
         retVal = eAPP_ID_REQ_TIME_BOUNDARY;
         if ( 0 == ( readingTime % chCfg_[ch].cfg.sampleRate_mS ) ) /* requested time is on a boundary? */
         {
            retVal = eAPP_ID_REQ_PAST;
            /* Calculate the index requested */
            uint32_t index = (uint32_t)( ( ( chCfg_[ch].metaData.smplDateTime - readingTime ) / chCfg_[ch].cfg.sampleRate_mS ) );
            if ( index < chCfg_[ch].metaData.numValidInts )      /* Make sure we aren't too far into the past */
            {
               idSampleData_t smplData;                     /* packed buffer with quality codes */
               uint32_t totLen = ( chCfg_[ch].cfg.storageLen + (uint32_t)sizeof( chCfg_[ch].metaData.qualCodes.byte ) );
               index =  ( ( (uint32_t)chCfg_[ch].metaData.smplIndex + chCfg_[ch].metaData.totNumInts ) - index ) %
                        ( lAddr )chCfg_[ch].metaData.totNumInts;
               lAddr offset = index * totLen;

               retVal = eNV_EXTERNAL_NV_ERROR;
               (void)memset( &smplData, 0, sizeof( smplData ) );
               if ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t * )&smplData, offset, totLen, pIdPartitions_[ch] ) )
               {
                  int64_t  sample = smplData.sample; /* get the sample data into a local buffer, byte aligned */

                  retVal = eSUCCESS;
                  /* Apply the scale factor(s) */
                  *pVal = ( double )sample * pow( 10.0, ( double )chCfg_[ch].cfg.trimDigits );
                  *pQual = smplData.qualCodes;
               }
               else
               {
                  ERR_printf( "LP Failed to read partition data" );
               }
            }
            else
            {
               ERR_printf( "LP Reading too far in past" );
            }
         }
         else
         {
            ERR_printf( "LP Not on sample boundary" );
         }
      }
      else
      {
         ERR_printf( "LP Invalid(future) time for reading" );
      }
      OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
   }
   return( retVal );
}
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: ID_rdIntervalDataCfg

   Purpose: Read interval data configuration

   Arguments: idChCfg_t *pIdCfg, uint8_t ch

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t ID_rdIntervalDataCfg( idChCfg_t *pIdCfg, uint8_t ch )
{
   returnStatus_t retVal;

   retVal = eAPP_ID_CFG_CH;
   if ( ch < (uint8_t)ID_MAX_CHANNELS )                           /* Make sure max channels isn't exceeded */
   {
      (void)memcpy( pIdCfg, &chCfg_[ch].cfg, sizeof( *pIdCfg ) ); /* Copy only config data, not the meta data */
      retVal = eSUCCESS;
   }
   return( retVal );
}
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: runI210Channel

   Purpose: Determine if a channel needs to run for the I210+.  This is necessary because channels 5 and 11, the
            RCDC switch position, may appear or disappear if the meter is reconfigured.

   Arguments: None

   Returns: bool

 **********************************************************************************************************************/
#if ( REMOTE_DISCONNECT == 1 )
static bool runI210Channel( uint8_t ch )
{
   bool     bNeedToRun = (bool)true;
   uint8_t  uSwitchPresent = 0;

   if ( ch == ID_MAX_CHANNELS - 1 )
   {
      (void)INTF_CIM_CMD_getRCDCSwitchPresent( &uSwitchPresent, (bool)true );

      if ( 0 == uSwitchPresent )
      {
         bNeedToRun = (bool)false;
      }
   }
   return( bNeedToRun );
}
#endif
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: readHostMeter

   Purpose: Reads host meter, scales the value as per channel configuration and computes the value that will be
            stored in ID

   Arguments: chCfg_t *pChCfg: Pointer to the channel configuration

   Returns: uint64_t: Value that will be stored in ID

 **********************************************************************************************************************/
static uint64_t readHostMeter( chCfg_t *pChCfg )
{
   CimValueInfo_t       rdgPair;       /* Raw Value from host meter (ignoring timestamp if present)   */
   var64_SI             srcVal;        /* Scaled value   */
   var64_SI             valToBeStored; /* Value that will be stored  */
   enum_CIM_QualityCode status;        /* read meter status */

   /* Get the current reading from the meter */
   srcVal.u64 = 0;
   status = INTF_CIM_CMD_getIntValueAndMetadata( ( meterReadingType )pChCfg->cfg.rdgTypeID, &rdgPair );
   if ( CIM_QUALCODE_SUCCESS == status )
   {
      /* Apply the scalars */
      srcVal.u64 = (uint64_t)( rdgPair.ValTime.Value / pow( 10.0, ( double )pChCfg->cfg.trimDigits ) );

      /* Prepare the data to be stored */
      if ( ID_MODE_DELTA_e == pChCfg->cfg.mode )
      {
         valToBeStored.i64 = srcVal.i64 - pChCfg->metaData.prevVal.i64; /*lint !e644  initialized aboved */
      }
      else
      {
         valToBeStored.u64 = srcVal.u64;
      }
      int64_t max = (int64_t)( (uint64_t)1 << ( ( pChCfg->cfg.storageLen * 8 ) - 1U ) );
      int64_t localmin = -max;
      max--;

      if ( ( valToBeStored.i64 > max ) || ( valToBeStored.i64 < localmin ) )
      {
         pChCfg->metaData.qualCodes.flags.qualCode =  (uint8_t)getPriorityQC( ID_OVERFLOW_QC_e,
                                                                                 ( ID_QC_e )pChCfg->metaData.qualCodes.flags.qualCode );
      }
      pChCfg->metaData.prevVal.u64 = srcVal.u64; /* Save the current reading as previous  */
   }
   else
   {
      /* reading error  */
      /* TODO: need different QC for read error */
      pChCfg->metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_OVERFLOW_QC_e, ( ID_QC_e )pChCfg->metaData.qualCodes.flags.qualCode );
      valToBeStored.u64 = 0;
      INFO_printf( "LP: Reading meter failed. Status=%d", status );
   }
   return valToBeStored.u64;
}
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: StoreNextID_Data

   Purpose: Stores the value in ID. Under certain conditions, this module will save meta-data only.

   Arguments:  uint8_t ch: ID channel
               chCfg_t *pChCfg: Pointer to channel configuration
               uint64_t valToBeStored: Value  to be stored
               bool metaDataOnly: If true, store meta-data only

   Returns: None

 **********************************************************************************************************************/
static void StoreNextID_Data( uint8_t ch, chCfg_t *pChCfg, uint64_t valToBeStored, bool metaDataOnly )
{
   static   uint8_t        lastCh = 0xff; /* Limit the printing on time jump  */
            idSampleData_t sampleData;
            lAddr          offset;
            uint32_t       dataLen = pChCfg->cfg.storageLen + sizeof( sampleData.qualCodes );

   PWR_lockMutex( PWR_MUTEX_ONLY ); /* Lock the power down mutex, the following data HAS to be written together. */

   if ( !metaDataOnly )
   {
      pChCfg->metaData.smplIndex = ( pChCfg->metaData.smplIndex + 1 ) % pChCfg->metaData.totNumInts;
      pChCfg->metaData.smplDateTime += pChCfg->cfg.sampleRate_mS;
      if ( pChCfg->metaData.numValidInts < pChCfg->metaData.maxValidInts )
      {
         pChCfg->metaData.numValidInts++;
      }
      offset = pChCfg->metaData.smplIndex * dataLen;
      sampleData.qualCodes = (uint8_t)pChCfg->metaData.qualCodes.byte;
      sampleData.sample = (int64_t)valToBeStored;

      /* Store load profile with meta-data   */
      pChCfg->metaData.qualCodes.byte = 0; /* clear the QC  */
      (void)PAR_partitionFptr.parWrite( offset, ( uint8_t * )&sampleData, dataLen, pIdPartitions_[ch] );
      if ( lastCh != ch )
      {
         INFO_printf( "LP Ch=%hhu QC=0x%X Val=%llu Index=%lu", ch, sampleData.qualCodes, sampleData.sample, pChCfg->metaData.smplIndex );
      }
      lastCh = ch;
   }
   else
   {
      INFO_printf( "LP Meta Ch=%hhu QC=0x%X Index=%lu", ch, pChCfg->metaData.qualCodes.byte, pChCfg->metaData.smplIndex );
   }
   (void)FIO_fwrite(  &fileHndlMeta_,
                        ( fileOffset )offsetof( idFileDataMeta_t, metaData ) + ( ch * sizeof( pChCfg->metaData ) ),
                        ( uint8_t * )&pChCfg->metaData, ( lCnt )sizeof( pChCfg->metaData ) );

#if ID_DEBUG
   (void)FIO_fflush(&fileHndlMeta_);   /* XXX Only needed when debugging. Cache does not flush between debug sessions   */
#endif
   PWR_unlockMutex( PWR_MUTEX_ONLY );
   /* End Critical section */
}
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: resetChannel

   Purpose: Clear all data in the given channel.

   Arguments:  uint8_t ch: ID channel
               chCfg_t *pChCfg: Pointer to channel configuration

   Returns: None

 **********************************************************************************************************************/
static void resetChannel ( uint8_t ch, chCfg_t *pChCfg )
{
   uint64_t  timeMsgComb;   /* Time Message in combined format */

   (void)TIME_UTIL_GetTimeInSysCombined( &timeMsgComb );
   pChCfg->metaData.smplDateTime = timeMsgComb - ( timeMsgComb % pChCfg->cfg.sampleRate_mS ); /* store the last boundary as previous  */
   pChCfg->metaData.numValidInts = 0;
   pChCfg->metaData.smplIndex = ch; /* So that all channels do not cross sector boundary in the same interval  */

   if ( ID_MODE_DELTA_e == pChCfg->cfg.mode )
   {
      /* Store the previous value to avoid first huge delta */
      CimValueInfo_t rdgPair;       /* Raw Value from host meter (ignoring timestamp if present)   */
      var64_SI srcVal;              /* Scaled value   */
      enum_CIM_QualityCode status;  /* read meter status */

      /* Get the current reading from the meter */
      pChCfg->metaData.prevVal.u64 = 0;
      status = INTF_CIM_CMD_getIntValueAndMetadata( ( meterReadingType )pChCfg->cfg.rdgTypeID, &rdgPair );
      if ( CIM_QUALCODE_SUCCESS == status )
      {
         /* Apply the scalars */
         srcVal.u64 = (uint64_t)( rdgPair.ValTime.Value / pow( 10.0, ( double )pChCfg->cfg.trimDigits ) );
         pChCfg->metaData.prevVal.u64 = srcVal.u64; /* Save the current reading as previous  */
      }
      else
      {
         /* reading error  */
         /* TODO: need different QC for read error */
         pChCfg->metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_OVERFLOW_QC_e,
                                                                              ( ID_QC_e )pChCfg->metaData.qualCodes.flags.qualCode );
         INFO_printf( "LP: Reading meter failed. Status=%d", status );
      }
   }
   (void)FIO_fwrite(  &fileHndlMeta_,
                        ( fileOffset )offsetof( idFileDataMeta_t, metaData ) + ( ch * sizeof( pChCfg->metaData ) ),
                        ( uint8_t * )&pChCfg->metaData, ( lCnt )sizeof( pChCfg->metaData ) );

   /* Erase the first sector better performance */
   (void)PAR_partitionFptr.parErase( 0, EXT_FLASH_ERASE_SIZE, pIdPartitions_[ch] );
   INFO_printf( "LP Ch=%hhu reset", ch );
}
#endif
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: getPriorityQC

   Purpose: Returns the higher priority quality code.

   Arguments:  ID_QC_e newQC: New quality code
               ID_QC_e oldQC: Current quality code

   Returns: ID_QC_e: Higher priority quality code

 **********************************************************************************************************************/
static ID_QC_e getPriorityQC( ID_QC_e newQC, ID_QC_e oldQC )
{
   ID_QC_e localNewQC = oldQC;

   if ( ID_GOOD_DATA_QC_e == oldQC )
   {
      /* No previous QC, return new QC  */
      localNewQC = newQC;
   }
   else
   {
      /* check the priority */
      if ( ( newQC != ID_GOOD_DATA_QC_e ) && ( newQC < oldQC ) )
      {
         /* New QC have higher priority, return new QC  */
         localNewQC = newQC;
      }
   }
   return localNewQC;
}
#endif
/***********************************************************************************************************************

   Function Name: PackQC

   Purpose: Adds quality codes to the message

   Arguments:
         uint8_t  QCbyte:     Quality code bits
         pack_t   *pPackCfg:  Data needed by pack  module
         bool     doAdd:      False-> just return size; True add data to buffer

   Returns: Number of bytes packed (or space required if doAdd is false)

 ***********************************************************************************************************************/
static uint16_t PackQC( uint8_t QCbyte, pack_t *pPackCfg, bool doAdd )
{
   qualCodes_u qualCodes;                    /* Quality code structure   */
   bool        addQcToMsg = (bool)false;     /* Flag to indicate that msgQC have valid QC  */
   uint8_t     msgQC = 0;                    /* QC that will go in the message */
   uint16_t    length = 0;                   /* Number of bytes added to the buffer  */

   qualCodes.byte = QCbyte;

   /* Add the quality code, up to 5 quality  */
   if ( 1 == qualCodes.flags.SignificantTimeBias )
   {
      msgQC = (uint8_t)CIM_QUALCODE_SIGNIFICANT_TIME_BIAS;
      addQcToMsg = (bool)true;
   }
   if ( 1 == qualCodes.flags.clockChanged )
   {
      if ( addQcToMsg )
      {
         /* Store the last QC first */
         msgQC |= 0x80; /* Add flag indicating additional QC will follow   */
         if( doAdd )
         {
            PACK_Buffer( (int16_t)sizeof( msgQC ) * 8, &msgQC, pPackCfg );
         }
         length += sizeof( msgQC );
      }
      msgQC = (uint8_t)CIM_QUALCODE_HMC_CLOCK_CHANGE;
      addQcToMsg = (bool)true;
   }
   if ( 1 == qualCodes.flags.powerFail )
   {
      if ( addQcToMsg )
      {
         /* Store the last QC first */
         msgQC |= 0x80; /* Add flag indicating additional QC will follow   */
         if ( doAdd )
         {
            PACK_Buffer( (int16_t)sizeof( msgQC ) * 8, &msgQC, pPackCfg );
         }
         length += sizeof( msgQC );
      }
      msgQC = (uint8_t)CIM_QUALCODE_POWER_FAIL;
      addQcToMsg = (bool)true;
   }
   if ( 1 == qualCodes.flags.ServiceDisconnectSwitch )
   {
      if ( addQcToMsg )
      {
         /* Store the last QC first */
         msgQC |= 0x80; /* Add flag indicating additional QC will follow   */
         if ( doAdd )
         {
            PACK_Buffer( (int16_t)sizeof( msgQC ) * 8, &msgQC, pPackCfg );
         }
         length += sizeof( msgQC );
      }
      msgQC = (uint8_t)CIM_QUALCODE_SERVICE_DISCONNECT;
      addQcToMsg = (bool)true;
   }
   if ( qualCodes.flags.qualCode != 0 )
   {
      if ( addQcToMsg )
      {
         /* Store the last QC first */
         msgQC |= 0x80; /* Add flag indicating additional QC will follow   */
         if ( doAdd )
         {
            PACK_Buffer( (int16_t)sizeof( msgQC ) * 8, &msgQC, pPackCfg );
         }
         length += sizeof( msgQC );
      }
      addQcToMsg = (bool)true;
#if ( LP_IN_METER == 1 )
      /* Map meter QC to Heep QC */
      msgQC = (uint8_t)qualCodes.flags.qualCode + 2;
#else
      switch ( qualCodes.flags.qualCode )
      {
         case ID_OVERFLOW_QC_e:
            msgQC = (uint8_t)CIM_QUALCODE_OVERFLOW;
            break;
         case ID_SKIPPED_INTERVAL_QC_e:
            msgQC = (uint8_t)CIM_QUALCODE_SKIPPED_INTERVAL;
            break;
         case ID_TEST_DATA_QC_e:
            msgQC = (uint8_t)CIM_QUALCODE_TEST_DATA;
            break;
         case ID_CONFIGURATION_CHANGED_QC_e:
            msgQC = (uint8_t)CIM_QUALCODE_CONFIG_CHANGE;
            break;
         case ID_ESTIMATED_DATA_QC_e:
            msgQC = (uint8_t)CIM_QUALCODE_ESTIMATED;
            break;
         case ID_LONG_INTERVAL_QC_e:
            msgQC = (uint8_t)CIM_QUALCODE_LONG_INTERVAL;
            break;
         case ID_PARTIAL_INTERVAL_QC_e:
            msgQC = (uint8_t)CIM_QUALCODE_PARTIAL_INTERVAL;
            break;
         default:
            msgQC = 0;
      }
#endif
   }
   if ( addQcToMsg )
   {
      if( doAdd )
      {
         /* Store the last QC first */
         PACK_Buffer( (int16_t)sizeof( msgQC ) * 8, &msgQC, pPackCfg );
      }
      length += sizeof( msgQC );
   }
   return length;
}
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: IntervalDataCh

   Purpose: Performs interval data on the channel passed in.

   Arguments: uint8_t ch, tTimeSysMsg *pTimeMsg

   Returns: None

 **********************************************************************************************************************/
static void IntervalDataCh( uint8_t ch, tTimeSysMsg const *pTimeMsg )
{
   sysTime_t timeMsg;       /* Used to store the pTimeMsg to be converted to combined date/time */
   uint64_t  timeMsgComb;   /* Time Message in combined format */
   var64_SI  valToBeStored = { 0 }; /*lint !e708 union init OK; Value that will be stored   */
   ID_QC_e   qualCode;      /* Quality code  */

   OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */

   if ( 0 != chCfg_[ch].cfg.sampleRate_mS )
   {
      /* combine date and time   */
      timeMsg.date = pTimeMsg->date;
      timeMsg.time = pTimeMsg->time;
      timeMsgComb = TIME_UTIL_ConvertSysFormatToSysCombined( &timeMsg );

      /* Check for time-jumps, power-up. Adjust the meta-data and fill or clear the load profile as needed */
      if ( 0 == timeMsgComb )
      {
         /*  power-up   */
         if ( eSUCCESS != TIME_UTIL_GetTimeInSysCombined( &timeMsgComb ) )
         {
            /* System time invalid. Store the current values, will be used when we get valid time  */
            readingAtPwrUp_[ch] = readHostMeter( &chCfg_[ch] ); /* read the meter  */
         }
         else
         {
            /* System time valid. Adjust the meta-data and fill or clear the load profile as needed   */
            if ( 0 == chCfg_[ch].metaData.smplDateTime )
            {
               /* first time power-up   */
               resetChannel ( ch, &chCfg_[ch] );
               chCfg_[ch].metaData.qualCodes.flags.powerFail = 1;
               StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true );   /* store meta-data only */
            }
            else if ( timeMsgComb >= ( chCfg_[ch].metaData.smplDateTime + chCfg_[ch].cfg.sampleRate_mS ) )
            {
               /* Outage across the boundary (maybe on interval boundary)  */
               uint32_t numIntervals;
               uint32_t cnt;

               numIntervals = (uint32_t)( ( timeMsgComb - chCfg_[ch].metaData.smplDateTime ) / chCfg_[ch].cfg.sampleRate_mS );
               if ( numIntervals >= chCfg_[ch].metaData.totNumInts )
               {
                  /* Clear the channel */
                  resetChannel ( ch, &chCfg_[ch] );
               }
               else
               {
                  valToBeStored.u64 = 0;
                  if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
                  {
                     valToBeStored.u64 = readHostMeter( &chCfg_[ch] ); /* read the meter */
                  }
                  for ( cnt = 0; cnt < numIntervals; cnt++ )
                  {
                     /* Store the first(for delta mode) and skip the rest of the intervals   */
                     qualCode = ID_SKIPPED_INTERVAL_QC_e;
                     if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
                     {
                        qualCode = ID_ESTIMATED_DATA_QC_e;
                     }
                     chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( qualCode, (ID_QC_e)chCfg_[ch].metaData.qualCodes.flags.qualCode );
                     chCfg_[ch].metaData.qualCodes.flags.powerFail = 1;
                     StoreNextID_Data( ch, &chCfg_[ch], valToBeStored.u64, (bool)false );
                     valToBeStored.u64 = 0;
                  }
                  if ( timeMsgComb % chCfg_[ch].cfg.sampleRate_mS != 0 )
                  {
                     /* set the quality codes for the current interval  */
                     if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
                     {
                        chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_ESTIMATED_DATA_QC_e,
                                                                                          ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
                     }
                     chCfg_[ch].metaData.qualCodes.flags.powerFail = 1;
                     StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true ); /* store meta-data only */
                  }
               }
            }
            else  /* else power-outage within the interval, set the quality code */
            {
               chCfg_[ch].metaData.qualCodes.flags.powerFail = 1;
               StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true ); /* store meta-data only */
            }
         } /* System time valid  */
      } /* else first time power-up */
      else if ( pTimeMsg->bTimeChangeForward && pTimeMsg->bTimeChangeBackward )
      {
         /* System got valid time   */
         (void)TIME_UTIL_GetTimeInSysCombined( &timeMsgComb );
         if ( timeMsgComb >= ( chCfg_[ch].metaData.smplDateTime + chCfg_[ch].cfg.sampleRate_mS ) )
         {
            /* Outage + invalid time duration across the boundary (maybe on interval boundary)  */
            uint32_t numIntervals;
            uint32_t cnt;

            numIntervals = (uint32_t)( ( timeMsgComb - chCfg_[ch].metaData.smplDateTime ) / chCfg_[ch].cfg.sampleRate_mS );
            if ( numIntervals >= chCfg_[ch].metaData.totNumInts || 0 == chCfg_[ch].metaData.smplDateTime )
            {
               /* Clear the channel */
               resetChannel ( ch, &chCfg_[ch] );
            }
            else
            {
               valToBeStored.u64 = 0;
               if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
               {
                  valToBeStored.u64 = readingAtPwrUp_[ch]; /* Use the stored reading   */
               }
               for ( cnt = 0; cnt < numIntervals; cnt++ )
               {
                  /* Skip the rest of the intervals   */
                  qualCode = ID_SKIPPED_INTERVAL_QC_e;
                  if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
                  {
                     qualCode = ID_ESTIMATED_DATA_QC_e;
                  }
                  chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( qualCode, ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
                  chCfg_[ch].metaData.qualCodes.flags.powerFail = 1;
                  StoreNextID_Data( ch, &chCfg_[ch], valToBeStored.u64, (bool)false );
                  valToBeStored.u64 = 0;
               }
               if ( timeMsgComb % chCfg_[ch].cfg.sampleRate_mS != 0 )
               {
                  /* Set the quality codes for the current interval  */
                  if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
                  {
                     chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_ESTIMATED_DATA_QC_e,
                                                                                             ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
                  }
                  chCfg_[ch].metaData.qualCodes.flags.powerFail = 1;
                  StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true );  /* store meta-data only */
               }
            }
         }
         else
         {
            /* power-outage within the interval and got valid time with-in the interval. Set quality code   */
            /* Adjust the previous value for delta channels as it was modified on power-up   */
            if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
            {
               chCfg_[ch].metaData.prevVal.u64 -= readingAtPwrUp_[ch];
            }
            chCfg_[ch].metaData.qualCodes.flags.powerFail = 1;
            StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true );  /* store meta-data only */
         }
      }

      else if ( pTimeMsg->bTimeChangeForward )
      {
         /*  time jumped forward */
         if ( timeMsgComb >= ( chCfg_[ch].metaData.smplDateTime + chCfg_[ch].cfg.sampleRate_mS ) )
         {
            /* Jumped forward across the boundary (maybe on interval boundary)   */
            uint32_t numIntervals;
            uint32_t cnt;

            numIntervals = (uint32_t)( ( timeMsgComb - chCfg_[ch].metaData.smplDateTime ) / chCfg_[ch].cfg.sampleRate_mS );
            if ( numIntervals >= chCfg_[ch].metaData.totNumInts || 0 == chCfg_[ch].metaData.smplDateTime )
            {
               resetChannel ( ch, &chCfg_[ch] );   /* Clear the channel */
            }
            else
            {
               if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
               {
                  /* Read the meter and store the value  */
                  valToBeStored.u64 = readHostMeter( &chCfg_[ch] ); /* read the meter */
                  chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_PARTIAL_INTERVAL_QC_e,
                                                                                          ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
               }
               else  /* No need to read for absolute mode   */
               {
                  valToBeStored.u64 = 0;
                  chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_SKIPPED_INTERVAL_QC_e,
                                                                                          ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
               }
               chCfg_[ch].metaData.qualCodes.flags.clockChanged = 1;
               StoreNextID_Data( ch, &chCfg_[ch], valToBeStored.u64, (bool)false );

               for ( cnt = 1; cnt < numIntervals; cnt++ )
               {
                  /* Skip the rest of the intervals   */
                  valToBeStored.u64 = 0;
                  chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_SKIPPED_INTERVAL_QC_e,
                                                                                          ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
                  chCfg_[ch].metaData.qualCodes.flags.clockChanged = 1;
                  StoreNextID_Data( ch, &chCfg_[ch], valToBeStored.u64, (bool)false );
               }
            }
         }
         /* else jumped forward with-in the interval  */
         if ( timeMsgComb % chCfg_[ch].cfg.sampleRate_mS != 0 )
         {
            /* Did not jump to interval boundary, set the quality codes for the next interval   */
            int32_t timeSigOffset_local = (int32_t)TIME_SYS_getTimeSigOffset();

            if ( pTimeMsg->timeChangeDelta <= -timeSigOffset_local || pTimeMsg->timeChangeDelta >= timeSigOffset_local )
            {
               if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
               {
                  chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_PARTIAL_INTERVAL_QC_e,
                                                                                          ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
               }
               chCfg_[ch].metaData.qualCodes.flags.clockChanged = 1;
               StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true );  /* store meta-data only */
            }
         }
      } /* endif time moved forward */
      else if ( pTimeMsg->bTimeChangeBackward )
      {
         /*  time jumped backward   */
         if ( timeMsgComb <= chCfg_[ch].metaData.smplDateTime )    /* Jumped backwards across a boundary? */
         {
            /* Jumped backward across the boundary (maybe on interval boundary)  */
            uint32_t numIntervals;
            uint32_t cnt;

            chCfg_[ch].metaData.qualCodes.flags.clockChanged = 1;
            if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
            {
               /* Set this QC for delta mode only  */
               chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_LONG_INTERVAL_QC_e,
                                                                                       ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
            }

            numIntervals = (uint32_t)( ( chCfg_[ch].metaData.smplDateTime - timeMsgComb ) / chCfg_[ch].cfg.sampleRate_mS ) + 1;
            if ( numIntervals >= chCfg_[ch].metaData.numValidInts || 0 == chCfg_[ch].metaData.smplDateTime )
            {
               resetChannel ( ch, &chCfg_[ch] );   /* Clear the channel */
            }
            else
            {
               if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
               {
                  for ( cnt = 0; cnt < numIntervals; cnt++ )   /* add all the previous intervals and adjust the previous reading */
                  {
                     /* Read all the intervals and add them */
                     idSampleData_t smplData;    /*  packed buffer with quality codes  */
                     uint32_t totLen = ( chCfg_[ch].cfg.storageLen + (uint32_t)sizeof( chCfg_[ch].metaData.qualCodes.byte ) );
                     uint32_t index = ( ( (uint32_t)chCfg_[ch].metaData.smplIndex + chCfg_[ch].metaData.totNumInts ) - cnt ) %
                                      ( lAddr )chCfg_[ch].metaData.totNumInts;
                     lAddr offset = index * totLen;
                     (void)memset( &smplData, 0, sizeof( smplData ) );
                     if ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t * )&smplData, offset, totLen, pIdPartitions_[ch] ) )
                     {
                        /* TODO: Set the QC (OR the bit flags and take care of the priority enum as well if possible */
                        /* *pQual = smplData.qualCodes;  */
                        chCfg_[ch].metaData.prevVal.i64 -= smplData.sample;
                     }
                     else
                     {
                        /* Read error  */
                        /* TODO: need different QC for read error */
                        chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_SKIPPED_INTERVAL_QC_e,
                                                                                                ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
                     }
                  }
               }
               /* else No adjustment for Absolute mode   */
               chCfg_[ch].metaData.smplIndex = ( chCfg_[ch].metaData.smplIndex + chCfg_[ch].metaData.totNumInts - numIntervals ) %
                                                   chCfg_[ch].metaData.totNumInts;
               chCfg_[ch].metaData.smplDateTime -= (uint64_t)numIntervals * chCfg_[ch].cfg.sampleRate_mS;
               chCfg_[ch].metaData.numValidInts -= numIntervals;
            }
            if ( 0 == timeMsgComb % chCfg_[ch].cfg.sampleRate_mS )
            {
               /* jumped to interval boundary, read the value and store */
               valToBeStored.u64 = readHostMeter( &chCfg_[ch] ); /* read the meter */
               StoreNextID_Data( ch, &chCfg_[ch], valToBeStored.u64, (bool)false );
            }
            else  /* Store meta-data only */
            {
               StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true );
            }
         }
         else  /* jumped back with in the same interval  */
         {
            int32_t timeSigOffset_local = (int32_t)TIME_SYS_getTimeSigOffset();

            if ( pTimeMsg->timeChangeDelta <= -timeSigOffset_local || pTimeMsg->timeChangeDelta >= timeSigOffset_local )
            {
               chCfg_[ch].metaData.qualCodes.flags.clockChanged = 1;
               if ( ID_MODE_DELTA_e == chCfg_[ch].cfg.mode )
               {
                  /* Set this QC for delta mode only  */
                  chCfg_[ch].metaData.qualCodes.flags.qualCode = (uint8_t)getPriorityQC( ID_LONG_INTERVAL_QC_e,
                                                                                          ( ID_QC_e )chCfg_[ch].metaData.qualCodes.flags.qualCode );
               }
               StoreNextID_Data( ch, &chCfg_[ch], (uint64_t)0, (bool)true );
            }
         }
      } /* endif time moved backward   */
      else  /* Normal case */
      {
         if ( 0 == chCfg_[ch].metaData.smplDateTime )
         {
            resetChannel ( ch, &chCfg_[ch] );   /* Clear the channel */
         }
         else if ( timeMsgComb >= ( chCfg_[ch].metaData.smplDateTime + chCfg_[ch].cfg.sampleRate_mS ) )
         {
            valToBeStored.u64 = readHostMeter( &chCfg_[ch] ); /* read the meter */
            StoreNextID_Data( ch, &chCfg_[ch], valToBeStored.u64, (bool)false );
         }
      }
   } /* Channel is enabled */
   else
   {
      INFO_printf( "LP Ch=%hhu disabled", ch );
   }
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
}
#endif
/***********************************************************************************************************************

   Function name: IntervalBubbleUp

   Purpose: Bubble up interval data

   Arguments: uint8_t ch, tTimeSysMsg *pTimeMsg, uint8_t trafficClass

   Returns: buffer_t* pMessage

 **********************************************************************************************************************/
static buffer_t* IntervalBubbleUp( tTimeSysMsg const *pTimeMsg, uint8_t trafficClass, uint8_t *buStatus )
{
   uint32_t          moveBackInMs;     /* in milli-seconds  */
   buffer_t          *pBuf;            /* pointer to message   */
   buffer_t          *pBufSmall;       /* pointer to message   */
   sysTime_t         timeMsg;          /* Used to store the pTimeMsg to be converted to combined date/time  */
   sysTimeCombined_t timeAtBuBoundary; /* Time Message in combined format  */
   sysTimeCombined_t timeAtLpBoundary; /* Time Message in combined format  */
   uint8_t           i;                /* Loop counter   */
   uint8_t           period = 1;       /* Redunandant reporting period counter   */
   int16_t           len = 0;          /* Cumulative size of lp data */
   uint16_t          finalLen = 0;     /* Final size of bubble up message  */
   uint16_t          room;             /* Space left in buffer allocated. */

   OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */

   /* Application header/QOS info   */
   HEEP_initHeader( &heepHdr );
   heepHdr.TransactionType = (uint8_t)TT_ASYNC;
   heepHdr.Resource        = (uint8_t)bu_lp;
   heepHdr.qos             = trafficClass;
   heepHdr.callback        = NULL;
   heepHdr.Method_Status   = (uint8_t)RequestedEntityTooLarge;   /* Assume all bubble up data won't fit.   */
   *buStatus               = RequestedEntityTooLarge;

   /* Allocate a big buffer to accomadate worst case payload.  */
   pBuf = BM_alloc( APP_MSG_MAX_DATA ); /* Allocate max buffer   */
   if ( pBuf != NULL )
   {
      room             = (uint16_t)APP_MSG_MAX_DATA;
      pBuf->x.dataLen  = 0;
      /* Get the time of the last LP   */
      timeMsg.date     = pTimeMsg->date;
      timeMsg.time     = pTimeMsg->time;
      timeAtBuBoundary = TIME_UTIL_ConvertSysFormatToSysCombined( &timeMsg );

      /* Loop through all intervals, in 5 minute increments, that fall within the bubble up schedule + up to 2 redundant bu periods */
      for( i = 0; i < 36; i++ ) /* 36 = (60 max comm schedule * (1 current + 2 redundant)) / 5 minimum interval (Commercial - worst case) */
      {
         /* For simpler implementation, scan all channels for worst case   */
         moveBackInMs     = i * TIME_TICKS_PER_5MIN;
         timeAtLpBoundary = timeAtBuBoundary - moveBackInMs;

#if ( LP_IN_METER == 1 )
         /* Add data at this time boundary. If it doesn't fit, returned length is 0.   */
         len = addMeterLoadProfileChannels( (uint32_t)( timeAtLpBoundary / TIME_TICKS_PER_SEC ), &pBuf->data[pBuf->x.dataLen], room );
#else
         len = addAllLoadProfileChannels( (uint32_t)( timeAtLpBoundary / TIME_TICKS_PER_SEC ), &pBuf->data[pBuf->x.dataLen], room );
#endif
         if ( len < 0 ) /* Insufficient room for this interval; done */
         {
            /* Check if current reporting interval fully collected. If loop counter is less than fileParams_.lpBuSchedule/5,
               then return RequestedEntityTooLarge  */
            if ( i < ( fileParams_.lpBuSchedule / 5U ) )
            {
               heepHdr.Method_Status = (uint8_t)RequestedEntityTooLarge;
               *buStatus             = RequestedEntityTooLarge;
               pBuf->x.dataLen       = 0;
            }
            break;
         }
         /* Got data requested.  */
         pBuf->x.dataLen += (uint16_t)len;
         room            -= (uint16_t)len;

#if LP_VERBOSE
         INFO_printf( "moveBackInMs %lu, period %hu, buScheduleInMs %lu, test value %llu",
                        moveBackInMs, period, buScheduleInMs, (uint64_t)( period * (uint64_t)buScheduleInMs ) );
#endif
         /* If at buScheduleInMs (redundancy) boundary, update buffer length to be returned. */
         if ( moveBackInMs == ( period * buScheduleInMs ) - TIME_TICKS_PER_5MIN )
         {
            finalLen = pBuf->x.dataLen;
            INFO_printf( "Interval %hhu, Redundancy %hhu, room %hu, finalLen %hu", i, period, room, finalLen );
            period++;
            heepHdr.Method_Status = (uint8_t)PartialContent; /* At least some of the data fit, set status to partial content  */
            *buStatus             = (uint8_t)PartialContent;
         }

         if ( moveBackInMs >= ( buScheduleInMs + ( buScheduleInMs * fileParams_.lpBuDataRedundancy ) ) - TIME_TICKS_PER_5MIN )
         {
            /* Got all the readings for this bubble-up   */
            pBuf->x.dataLen       = finalLen;
            heepHdr.Method_Status = (uint8_t)method_post; /* All the data fit, set status to normal post  */
            *buStatus             = (uint8_t)OK;
            break;
         }
#if LP_VERBOSE
         INFO_printf( "Interval %2hhu, period %2hhu, len %3hd, finalLen %3hu, room %4hu", i, period, len, finalLen, room );
#endif
      }
      if ( 0 == pBuf->x.dataLen )   /* No channel configured, free the buffer */
      {
         BM_free( pBuf ); /* Free the buffer   */
         pBuf = NULL;
      }
      else
      {
         if ( pBuf->x.dataLen < ID_MAX_BYTES_PER_ALL_CHANNELS )   /* Can fit in smaller buffer. Copy data and free the larger buffer   */
         {
            pBufSmall = BM_alloc( pBuf->x.dataLen );
            if ( pBufSmall != NULL )
            {
               (void)memcpy( &pBufSmall->data[0], &pBuf->data[0], pBuf->x.dataLen );
               pBufSmall->x.dataLen = pBuf->x.dataLen;
               BM_free( pBuf ); /* Free the buffer   */
               pBuf = pBufSmall;
            }
         }
      }
   }
   /* else No buffer available, drop the message for now */
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */

   return( pBuf );
}
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: readCfg

   Purpose: Reads interval data configuration

   Arguments: uint8_t ch, chCfg_t *pChCfg

   Returns: eSUCCESS or eFAILURE

 **********************************************************************************************************************/
static returnStatus_t readCfg( uint8_t ch, chCfg_t *pChCfg )
{
   returnStatus_t retVal;
   uint8_t        cntr;

   /* try 5 times */
   for ( cntr = 0; cntr < 5; cntr++ )
   {
      retVal = FIO_fread(  &fileHndlMeta_, ( uint8_t * )&pChCfg->metaData,
                           ( fileOffset )offsetof( idFileDataMeta_t, metaData ) + ( ch * ( fileOffset )sizeof( pChCfg->metaData ) ),
                           ( lCnt )sizeof( pChCfg->metaData ) );
      if( retVal == eSUCCESS )
      {
         break;
      }
   }
   if( retVal == eSUCCESS )
   {
      for ( cntr = 0; cntr < 5; cntr++ )
      {
         retVal = FIO_fread( &fileHndlCfg_, ( uint8_t * )&pChCfg->cfg,
                             ( fileOffset )offsetof( idFileDataCfg_t, cfg ) + ( ch * ( fileOffset )sizeof( pChCfg->cfg ) ),
                             ( lCnt )sizeof( pChCfg->cfg ) );
         if( retVal == eSUCCESS )
         {
            break;
         }
      }
   }
   if( retVal == eFAILURE )
   {
      pChCfg->cfg.sampleRate_mS = 0;
   }
   return( retVal );
}
#endif



#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function Name: ID_lpSamplePeriod( action, id, value, attr )

   Purpose: Get/Set ID_lpSamplePeriod

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpSamplePeriod( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   uint32_t       i;
   idChCfg_t      chCfg;
   uint16_t       *ptr;
   returnStatus_t retVal = eAPP_NOT_HANDLED; /* Success/failure */

   ptr = value;
   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( ( sizeof(uint16_t)*ID_MAX_CHANNELS ) <= MAX_OR_PM_PAYLOAD_SIZE ) /*lint !e506 !e774   */
      {
         /* The reading will fit in the buffer  */
         for ( i = 0; i < ID_MAX_CHANNELS; i++ )
         {
            (void)ID_rdIntervalDataCfg( &chCfg, (uint8_t)i );
            *ptr = (uint16_t)( chCfg.sampleRate_mS / TIME_TICKS_PER_MIN );
            ptr++;
         }
         retVal = eSUCCESS;
         if ( attr != NULL )
         {
            attr->rValLen = (uint16_t)( sizeof(uint16_t) * ID_MAX_CHANNELS );
            attr->rValTypecast = (uint8_t)uint16_list_type;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      /*  Make sure we have enough inputs */
      if ( attr->rValLen == ( sizeof(uint16_t)*ID_MAX_CHANNELS ) )
      {
         for ( i = 0; i < ID_MAX_CHANNELS; i++ )
         {
            (void)ID_rdIntervalDataCfg( &chCfg, (uint8_t)i );             /* Retrieve current settings */
            chCfg.sampleRate_mS = (uint32_t)( *ptr ) * TIME_TICKS_PER_MIN;  /* Update attribute */
            ptr++;
            (void)ID_cfgIntervalData( (uint8_t)i, &chCfg );               /* Save settings */
         }
         retVal = eSUCCESS;
      }
   }
   return retVal;
} /*lint !e715 symbol id not referenced. This funtion is only called when id is matched  */
/***********************************************************************************************************************

   Function Name: lpBuAllChannelsHandler( ch, action, id, value, attr )

   Purpose: Get/Set lpBuChannels

   Arguments:  ch    -> Interval data channel number (index 0 based)
               action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t lpBuAllChannelsHandler( uint8_t ch, enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t retVal = eAPP_NOT_HANDLED; /*  Success/failure */
   idChCfg_t      chCfg;
   uint16_t       *valPtr = value;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( ( sizeof(uint16_t) * 5 ) <= MAX_OR_PM_PAYLOAD_SIZE ) /*lint !e506 !e774  */
      {
         /* The reading will fit in the buffer  */
         if ( eSUCCESS == ID_rdIntervalDataCfg( &chCfg, ch ) ) /* Read the channel  */
         {
            /* pack the data  */
            *valPtr = (uint16_t)chCfg.rdgTypeID;
            valPtr++;
            *valPtr = (uint16_t)( chCfg.sampleRate_mS / TIME_TICKS_PER_MIN );
            valPtr++;
            *valPtr = (uint16_t)chCfg.mode;
            valPtr++;
            *valPtr = (uint16_t)chCfg.trimDigits;
            valPtr++;
            *valPtr = (uint16_t)chCfg.storageLen;

            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen = (uint16_t)( sizeof(uint16_t) * 5 );
               attr->rValTypecast = (uint8_t)uint16_list_type;
            }
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      /*  verify request is sending the proper amount of data from pointer reference   */
      if( attr->rValLen == ( 5 * sizeof(uint16_t) ) )
      {
         (void)memset( ( uint8_t * )&chCfg, 0, sizeof( chCfg ) );
         chCfg.rdgTypeID = *valPtr;
         valPtr++;
         chCfg.sampleRate_mS = ( (uint32_t) * valPtr ) * TIME_TICKS_PER_MIN;
         valPtr++;
         chCfg.mode = ( mode_e ) * valPtr;
         valPtr++;
         chCfg.trimDigits = (uint8_t) * valPtr;
         valPtr++;
         chCfg.storageLen = (uint8_t) * valPtr;

         retVal = ID_cfgIntervalData( ch, &chCfg );
      }

      else
      {
         retVal = eAPP_INVALID_VALUE;
      }
   }
   return retVal;
} /*lint !e715 symbol id not referenced. This funtion is only called when id is matched  */
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel1Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel1

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel1Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 0, action, id, value, attr ) );
}
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel2Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel2

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel2Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 1, action, id, value, attr ) );
}
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel3Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel3

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel3Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 2, action, id, value, attr ) );
}
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel4Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel4

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel4Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 3, action, id, value, attr ) );
}
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel5Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel5

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel5Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 4, action, id, value, attr ) );
}
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel6Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel6

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel6Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 5, action, id, value, attr ) );
}
#if ( ID_MAX_CHANNELS > 6 )
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel7Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel7

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel7Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 6, action, id, value, attr ) );
}
/***********************************************************************************************************************

   Function Name: ID_lpBuChannel8Handler( action, id, value, attr )

   Purpose: Get/Set lpBuChannel8

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ID_lpBuChannel8Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   return( lpBuAllChannelsHandler( 7, action, id, value, attr ) );
}
#endif
#endif
/*
   Get and Set parameters
*/
/***********************************************************************************************************************

   Function name: ID_getLpBuSchedule

   Purpose: Returns buble up schedule

   Arguments: None

   Returns: uint16_t

 **********************************************************************************************************************/
uint16_t ID_getLpBuSchedule( void )
{
   uint16_t lpBuSchedule_local;
   /* Since this value is "atomic", no MUTEX is needed here. If changed to larger value or processor with bus smaller than variable size, re-enable
      mutex.  */
#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
#endif

   lpBuSchedule_local = fileParams_.lpBuSchedule;

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
#endif

   return( lpBuSchedule_local );
}
/***********************************************************************************************************************

   Function name: ID_setLpBuSchedule

   Purpose: Sets the buble up schedule

   Arguments: returnStatus_t

   Returns: none

 **********************************************************************************************************************/
returnStatus_t ID_setLpBuSchedule( uint16_t buSchedule )
{
   returnStatus_t retVal = eFAILURE;

   if ( 0 == buSchedule || 5 == buSchedule || 15 == buSchedule || 30 == buSchedule || 60 == buSchedule )
   {
      /* Valid bubble up schedule   */
      OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
      fileParams_.lpBuSchedule = buSchedule;
      (void)FIO_fwrite( &fileHndlParam_, 0, ( uint8_t * )&fileParams_, ( lCnt )sizeof( idFileParams_t ) );
      buScheduleInMs = fileParams_.lpBuSchedule * TIME_TICKS_PER_MIN;
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
   }
   else
   {
      ERR_printf( "LP Bubble up schedule write failed, Value Out of Range" );
   }
   return( retVal );
}
/***********************************************************************************************************************

   Function Name: ID_LpBuScheduleHandler( action, id, value, attr )

   Purpose: Get/Set lpBuSchedule

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -e{715} some function parameters not use; required by API  */
returnStatus_t ID_LpBuScheduleHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; /*  Success/failure */

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( fileParams_.lpBuSchedule ) <= MAX_OR_PM_PAYLOAD_SIZE ) /*lint !e506 !e774  */
      {
         /* The reading will fit in the buffer  */
         *( uint16_t * )value = ID_getLpBuSchedule();
         retVal = eSUCCESS;
         if ( attr != NULL )
         {
            attr->rValLen = (uint16_t)sizeof( fileParams_.lpBuSchedule );
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = ID_setLpBuSchedule( *( uint16_t * )value );
   }
   return retVal;
} /*lint !e715 symbol id not referenced. This funtion is only called when id is matched  */
/***********************************************************************************************************************

   Function name: ID_getQos

   Purpose: Returns the LP Qos setting (lpBuTrafficClass)

   Arguments: none

   Returns: uint8_t

 **********************************************************************************************************************/
uint8_t ID_getQos( void )
{
   uint8_t lpBuTrafficClass_local;
   /* Since this value is "atomic", no MUTEX is needed here. If changed to larger value or processor with bus smaller than variable size, re-enable
      mutex.  */

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
#endif

   lpBuTrafficClass_local = fileParams_.lpBuTrafficClass;

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
#endif

   return( lpBuTrafficClass_local );
}
/***********************************************************************************************************************

   Function name: ID_setQos

   Purpose: Sets the LP Qos setting (lpBuTrafficClass)

   Arguments: uint8_t

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t ID_setQos( uint8_t trafficClass )
{
   returnStatus_t retVal = eFAILURE;

   if ( trafficClass <= LP_BU_TRAFFIC_CLASS_MAX )
   {
      OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
      fileParams_.lpBuTrafficClass = trafficClass;
      (void)FIO_fwrite( &fileHndlParam_, 0, ( uint8_t * )&fileParams_, ( lCnt )sizeof( idFileParams_t ) );
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
   }
   else
   {
      ERR_printf( "LP Traffic class param write failed, Value Out of Range" );
   }
   return( retVal );
}
/***********************************************************************************************************************

   Function Name: ID_LpBuTrafficClassHandler( action, id, value, attr )

   Purpose: Get/Set lpBuTrafficClass

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -e{715} some function parameters not use; required by API  */
returnStatus_t ID_LpBuTrafficClassHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; /*  Success/failure */

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( fileParams_.lpBuTrafficClass ) <= MAX_OR_PM_PAYLOAD_SIZE ) /*lint !e506 !e774 */
      {
         /* The reading will fit in the buffer  */
         *( uint8_t * )value = ID_getQos();
         retVal = eSUCCESS;
         if ( attr != NULL )
         {
            attr->rValLen = (uint16_t)sizeof( fileParams_.lpBuTrafficClass );
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = ID_setQos( *( uint8_t * )value );
   }
   return retVal;
} /*lint !e715 symbol id not referenced. This funtion is only called when id is matched  */
/***********************************************************************************************************************

   Function name: ID_getLpBuDataRedundancy

   Purpose: Returns the lpBuDataRedundancy

   Arguments: none

   Returns: uint8_t

 **********************************************************************************************************************/
uint8_t ID_getLpBuDataRedundancy( void )
{
   uint8_t lpBuDataRedundancy_local;

   /* Since this value is "atomic", no MUTEX is needed here. If changed to larger value or processor with bus smaller than variable size, re-enable
      mutex.  */

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
#endif

   lpBuDataRedundancy_local = fileParams_.lpBuDataRedundancy;

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
#endif

   return( lpBuDataRedundancy_local );
}
/***********************************************************************************************************************

   Function name: ID_setLpBuDataRedundancy

   Purpose: Sets the lpBuDataRedundancy

   Arguments: uint8_t

   Returns: returnStatus_t

 **********************************************************************************************************************/
/*lint -efunc(578,ID_setLpBuDataRedundancy) */
returnStatus_t ID_setLpBuDataRedundancy( uint8_t lpBuDataRedundancy )
{
   returnStatus_t retVal = eFAILURE;

   if ( lpBuDataRedundancy <= LP_BU_DATA_REDUNDANCY_MAX )
   {
      OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
      fileParams_.lpBuDataRedundancy = lpBuDataRedundancy;
      (void)FIO_fwrite( &fileHndlParam_, 0, ( uint8_t * )&fileParams_, ( lCnt )sizeof( idFileParams_t ) );
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
   }
   else
   {
      ERR_printf( "lpBuDataRedundancy param write failed, Value Out of Range" );
   }
   return( retVal );
}
/***********************************************************************************************************************

   Function Name: ID_LpBuDataRedundancyHandler( action, id, value, attr )

   Purpose: Get/Set lpBuDataRedundancy

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -e{715} some function parameters not use; required by API  */
returnStatus_t ID_LpBuDataRedundancyHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; /*  Success/failure */

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( fileParams_.lpBuDataRedundancy ) <= MAX_OR_PM_PAYLOAD_SIZE ) /*lint !e506 !e774  */
      {
         /* The reading will fit in the buffer  */
         *( uint8_t * )value = ID_getLpBuDataRedundancy();
         retVal = eSUCCESS;
         if ( attr != NULL )
         {
            attr->rValLen = (uint16_t)sizeof( fileParams_.lpBuDataRedundancy );
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = ID_setLpBuDataRedundancy( *( uint8_t * )value );
   }
   return retVal;
} /*lint !e715 symbol id not referenced. This funtion is only called when id is matched  */
/***********************************************************************************************************************

   Function name: ID_getLpBuMaxTimeDiversity

   Purpose: Returns the lpBuMaxTimeDiversity

   Arguments: none

   Returns: uint8_t

 **********************************************************************************************************************/
uint8_t ID_getLpBuMaxTimeDiversity( void )
{
   uint8_t lpBuMaxTimeDiversity_local;
   /* Since this value is "atomic", no MUTEX is needed here. If changed to larger value or processor with bus smaller than variable size, re-enable
      mutex.  */

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
#endif

   lpBuMaxTimeDiversity_local = fileParams_.lpBuMaxTimeDiversity;

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
#endif

   return( lpBuMaxTimeDiversity_local );
}
/***********************************************************************************************************************

   Function name: ID_setLpBuMaxTimeDiversity

   Purpose: Sets the lpBuMaxTimeDiversity

   Arguments: uint8_t

   Returns: returnStatus_t

 **********************************************************************************************************************/
/*lint -efunc(578,ID_setLpBuMaxTimeDiversity) */
returnStatus_t ID_setLpBuMaxTimeDiversity( uint8_t lpBuMaxTimeDiversity )
{
   returnStatus_t retVal = eFAILURE;

   if ( lpBuMaxTimeDiversity <= LP_BU_MAX_TIME_DIVERSITY_MAX )
   {
      OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
      fileParams_.lpBuMaxTimeDiversity = lpBuMaxTimeDiversity;
      (void)FIO_fwrite( &fileHndlParam_, 0, ( uint8_t * )&fileParams_, ( lCnt )sizeof( idFileParams_t ) );
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
   }
   else
   {
      ERR_printf( "lpBuMaxTimeDiversity param write failed, Value Out of Range" );
   }
   return( retVal );
}
/***********************************************************************************************************************

   Function Name: ID_LpBuMaxTimeDiversityHandler( action, id, value, attr )

   Purpose: Get/Set lpBuMaxTimeDiversity

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -e{715} some function parameters not use; required by API  */
returnStatus_t ID_LpBuMaxTimeDiversityHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; /*  Success/failure */

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( fileParams_.lpBuMaxTimeDiversity ) <= MAX_OR_PM_PAYLOAD_SIZE ) /*lint !e506 !e774   */
      {
         /* The reading will fit in the buffer  */
         *( uint8_t * )value = ID_getLpBuMaxTimeDiversity();
         retVal = eSUCCESS;
         if ( attr != NULL )
         {
            attr->rValLen = (uint16_t)sizeof( fileParams_.lpBuMaxTimeDiversity );
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = ID_setLpBuMaxTimeDiversity( *( uint8_t * )value );
   }
   return retVal;
} /*lint !e715 symbol id not referenced. This funtion is only called when id is matched  */
/***********************************************************************************************************************

   Function Name: ID_lpBuEnabledHandler( action, id, value, attr )

   Purpose: Get/Set lpBuEnabled

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -e{715} some function parameters not use; required by API  */
returnStatus_t ID_LpBuEnabledHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; /*  Success/failure */

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( fileParams_.lpBuEnabled ) <= MAX_OR_PM_PAYLOAD_SIZE ) /*lint !e506 !e774   */
      {
         /* The reading will fit in the buffer  */
         *( uint8_t * )value = ID_getLpBuEnabled();
         retVal = eSUCCESS;
         if ( attr != NULL )
         {
            attr->rValLen = (uint16_t)sizeof( fileParams_.lpBuEnabled );
            attr->rValTypecast = (uint8_t)Boolean;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = ID_setLpBuEnabled( *( uint8_t * )value );
   }
   return retVal;
} /*lint !e715 symbol id not referenced. This funtion is only called when id is matched  */
/***********************************************************************************************************************

   Function name: HD_getLpBuEnabled

   Purpose: Get lpBuEnabled in interval data configuration

   Arguments: None

   Returns: bool uDsBuEnabled

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
bool ID_getLpBuEnabled( void )
{
   bool uLpBuEnabled;
   /* Since this value is "atomic", no MUTEX is needed here. If changed to larger value or processor with bus smaller than variable size, re-enable
      mutex.  */

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
#endif

   uLpBuEnabled = fileParams_.lpBuEnabled;

#if NEED_MUTEX_TO_READ_MEMBER
   OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
#endif
   return ( uLpBuEnabled );
}
/***********************************************************************************************************************

   Function name: HD_setLpBuEnabled

   Purpose: Set lpBuEnabled in interval data configuration

   Arguments: bool uDsBuEnabled

   Returns: returnStatus_t

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
returnStatus_t ID_setLpBuEnabled( uint8_t uLpBuEnabled )
{
   returnStatus_t retVal = eFAILURE;

   if( uLpBuEnabled <= 1 )
   {
      OS_MUTEX_Lock( &idMutex_ ); /*  Function will not return if it fails   */
      fileParams_.lpBuEnabled = uLpBuEnabled;
      (void)FIO_fwrite( &fileHndlParam_, 0, ( uint8_t * )&fileParams_, ( lCnt )sizeof( idFileParams_t ) );
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &idMutex_ ); /*  Function will not return if it fails */
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return( retVal );
}
#if ( LP_IN_METER == 0 )
/***********************************************************************************************************************

   Function name: ID_clearAllIdChannels

   Purpose: Resets all interval data channels in endpoint.

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
void ID_clearAllIdChannels()
{
   uint8_t ch;
   for ( ch = 0; ch < ID_MAX_CHANNELS; ch++ )
   {
      /* if channels is configured to store ID, reset the channel */
      if ( 0 != chCfg_[ch].cfg.sampleRate_mS )
      {
         resetChannel( ch, &chCfg_[ch] );
      }
   }
}
#endif

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
/***********************************************************************************************************************

   Function name: ID_LoadLPTables_Helper

   Purpose: Allows noiseband command to invoke LoadLPTables() during the run to create HMC traffic

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
void ID_LoadLPTables_Helper( void )
{
   LoadLPTables();
}
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )