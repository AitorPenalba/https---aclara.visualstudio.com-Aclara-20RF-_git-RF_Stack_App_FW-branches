/******************************************************************************

   Filename: EVL_event_log.c

   Global Designator: EVL

   Contents:
      This file contains the Event Logger API calls. It also contains the
      internal calls to support the APIs.

 ******************************************************************************
   Copyright (c) 2015-2021 ACLARA.  All rights reserved.
   This program may not be reproduced, in whole or in part, in any form or by
   any means whatsoever without the written permission of:
      ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "partition_cfg.h"
#include "EVL_event_log.h"
#include "HEEP_util.h"
#include "file_io.h"
#include "DBG_SerialDebug.h"
#include "PHY_Protocol.h"
#include "time_util.h"
#include "timer_util.h"
#include "pwr_task.h"
#include "byteswap.h"
#include "APP_MSG_Handler.h"
#include "ALRM_Handler.h"

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
#include "time_util.h"
#include "time_sys.h"
#include "pwr_last_gasp.h"
#include "pwr_restore.h"
#include "vbat_reg.h"
#include "MAC.h"
#include "MAC_Protocol.h"
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
#define EVL_EVENT_SENT  (0x1)

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define RINGBUFFER_AVAILABLE_DATA(B)         ((B)->length)
#define RINGBUFFER_AVAILABLE_SPACE(B)        ((B)->bufferSize - (B)->length)
#define RINGBUFFER_END(B)                    (((B)->start + (B)->length) % (B)->bufferSize)
#define RINGBUFFER_AVAILABLE_SPACE_TO_END(B) ((B)->bufferSize - RINGBUFFER_END(B))
#define RINGBUFFER_STARTS_AT(B, C)           ((((B)->start + (C)) % (B)->bufferSize))
#define RINGBUFFER_COMMIT_READ(B, A)         ((B)->start = ((B)->start + (A)) % (B)->bufferSize)
#if ( EP == 1 )
#define PRIORITY_INVALID(A)                  ((A) < _EvlMetaData.oThreshold)
#else
#define PRIORITY_INVALID(A)                  ((A) < _EvlMetaData.rtThreshold)
#endif

#define ID_FILE_UPDATE_RATE_META_SEC      ((uint32_t)5)           /* File updates every 5 minutes */
#define DEFAULT_AM_BU_MAX_TIME_DIVERSITY  ((uint8_t)4)
#define MAX_AM_BU_MAX_TIME_DIVERSITY      ((uint8_t)15)

/* When reading the event log, a buffer is populated with as much data as possbile until the size limit of the the
   buffer is reached.  The amount of space avaiable in the buffer will be limited based upon how much overhead is
   required in the message sent back to the head end.  This overhead can vary pending on how the reponse is sent.  A
   basic response is limited to 15 events.  Additional CompactMeterRead bit structures, with additional overhead
   added to the response, can be appended until the size limits of the buffer have been met.  The overhead for each
   bit structure in the response will include the valuesIntervalEnd and eventReadingQuantity.  The worst case amount
   of overhead added to a resonse will occur if every event returned contains no name/vale pair information.

   A max bit structure response containing no name/value pair data = 110 bytes (including overhead)
                                                                     105 bytest (without the overhead)

   The max maximum instances overhead will need to added to the reponse:
   = 11 -> floor(APP_MSG_MAX_PAYLOAD / 110) for K24 and DCU
   =  9 -> floor(APP_MSG_MAX_PAYLOAD / 110) for K22
   floor was taken just to be safe

   The size of the buffer with 11 instances of overhead = 11 * 105 -> 1155 bytes.
   The size of the buffer with 11 instances of overhead =  9 * 105 ->  945 bytes.

   The K22 will not have additional buffers available for 945 bytes, so 256 will be used. */
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
#define EVENT_RECOVERY_BUFF_SIZE                  ( 256 )
#else
#define EVENT_RECOVERY_BUFF_SIZE                  ( 1155 )
#endif
#if( RTOS_SELECTION == FREE_RTOS )
#define EVL_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define EVL_NUM_MSGQ_ITEMS 0
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint32_t                   length;     /* Data used in buffer. */
   uint32_t                   start;      /* Location of the start of the ring buffer data. */
   uint32_t                   bufferSize; /* Buffer size. */
} RingBufferHead_s;

typedef struct
{
   RingBufferHead_s           HighBufferHead;       /* High Priority buffer meta data */
   RingBufferHead_s           NormalBufferHead;     /* Normal Priority buffer meta data */
   uint8_t                    HighAlarmSequence;    /* Sequence number that increments for every event */
   uint8_t                    NormalAlarmSequence;  /* Sequence number that increments for every event */
   uint8_t                    rtThreshold;          /* Real Time Threshold. */
   uint8_t                    oThreshold;           /* Opportunistic Threshold. */
   bool                       Initialized;          /* The file has been initialized. */
   bool                       realTimeAlarm;        /* defines if real time alarms are supported */
   uint8_t                    amBuMaxTimeDiversity; /* time in minutes during which a /bu/am message may bubble-in subsequent to an event occuring */
} EventLogFile_s;

typedef struct
{
   meterReadingType alarmIndexReadingType;
   uint8_t                alarmIndexValue;
} EventLogCoruptionClearedEvent_s;

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static FileHandle_t           _EvlFileHandleMeta;     /* Handle to the EVL buffer meta data file */
static FileStatus_t           _EvlFileStatusMeta;     /* Status of the EVL buffer meta data file */
static EventLogFile_s         _EvlMetaData;           /* The contents of the meta data file */
static PartitionData_t const  *_EvlHighFlashHandle;   /* Handle to the High priority buffer in flash */
static PartitionData_t const  *_EvlNormalFlashHandle; /* Handle to the Normal priority buffer in flash */
static OS_MUTEX_Obj           _EVL_MUTEX;             /* Event Logger Mutex */
static OS_MSGQ_Obj            EvlAlarmHandler_MsgQ_;  /* High priority events to be transmitted.      */
static uint16_t               evlTimerId = INVALID_TIMER_ID;  /* Timer ID used by for time diversity. */
static timer_t                tmrSettings;            /* Timer for sending pending alarm message(s)   */
static bool                   _Initialized = false;

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
static uint8_t                schSimLGStartAlarmId_ 		= NO_ALARM_FOUND;    /* Sim LG Start alarm ID */
static uint8_t                schSimLGDurationAlarmId_ 	= NO_ALARM_FOUND;    /* Sim LG Duration alarm */
static uint8_t                schSimLGSleepTimerAlarmId_ = NO_ALARM_FOUND; 	/* Sim LG Duration alarm */
static uint16_t               powerOnStateTimerId_ 		= INVALID_TIMER_ID;  /* Sim LG Power On state timer ID */
static volatile uint8_t       simLGTxSuccessful_ 			= 0;                 /* Flag to check if Sim LG message is transmetted */
static eSimLGState_t          simLGState_ 					= eSimLGDisabled;    /* Store the current state of LG Simulation */
static bool                   simLGMACConfigSaved_       = false;             /* Safest way to know when to restore Config */
static OS_SEM_Obj             SimLGTxDoneSem;                              	/* Used to signal message transmit completed. */
static PWRLG_SysRegisterFilePtr      const pSysMem       = PWRLG_RFSYS_BASE_PTR;
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static int32_t EventLogWrite( PartitionData_t const *nvram, RingBufferHead_s *head, uint8_t const * const data,
                              uint32_t length, bool overwrite );
static int32_t EventLogRead( PartitionData_t const *nvram,  RingBufferHead_s *head, uint8_t * const target,
                             uint32_t amount, bool purge );
static int32_t EventLogLoadBuffer( uint8_t priority, uint8_t *target, uint32_t size, uint32_t *totalSize,
                                   uint8_t *alarmId, EventQuery_s query, EventMarkSent_s *passback );
static returnStatus_t EventLogMakeRoom( PartitionData_t const *nvram, RingBufferHead_s *head, uint32_t length );
static returnStatus_t EventInitializeMetaData( void );
static void evlInitMsg( pHEEP_APPHDR_t appHdr );
static uint16_t evlAddAlarm( EventStoredData_s *alarmInfo, pack_t *packCfg );
static bool obsoleteAlarmIndex( RingBufferHead_s *rHead, PartitionData_t const *nvram, uint8_t indexValue );
static void timerDiversityTimerExpired( uint8_t cmd, void *pData );

static returnStatus_t eraseEventLogPartition( ePartitionName partName );
static void generateEventLogCoruptionClearedEvents( EventLogCoruptionClearedEvent_s const *eventBufClearedInfo );

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
static void           initSimLG( void );
static returnStatus_t startOneShotTimerAlarm( uint32_t sleepTime );
static returnStatus_t setSimLGStartAlarm( uint32_t simLGStartTime );
static returnStatus_t setSimLGDurationAlarm( uint16_t simLGDuration );
static void           initiateSimLGTx( void );
static returnStatus_t setSimLGPowerOnStateTimer(uint32_t timeMilliseconds);
static void           powerOnStateTimerExpired(uint8_t cmd, void *pData);
static void           resetSimLGCsmaParameters( void );
#endif

/***********************************************************************************************************************

   Function Name: EVL_AlarmHandlerTask(void)

   Purpose: Task for handling High Priority event log messages

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
//lint -esym(715,Arg0)  // Arg0 required for generic API, but not used here.
void EVL_AlarmHandlerTask ( uint32_t Arg0 )
{
   HEEP_APPHDR_t     heepHdr = {0}; /* Application header   */
   pack_t            packCfg;       /* Struct needed for PACK functions */
   sysTime_t         ReadingTime;   /* Used to retrieve system date/time   */
   buffer_t          *pAlarm;       /* Individual log entry buffer   */
   buffer_t          *pMsg = NULL;  /* Outgoing message buffer.      */
   uint32_t          msgTime;       /* Time converted to seconds since epoch  */
   uint32_t          numAlarms = 0; /* Number of alarms queued */
   EventStoredData_s *pAlarmInfo;   /* Alarm information contained in queue.  */
   bool              evlTimerInProgress = false;

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
   sysTimeCombined_t    restorationTime = 0; /* System time at entry to this task   */
   uint64_t             outageSeconds;
   pwrFileData_t const  *pwrFileData;        /* Pointer to pwrFileData  */
   uint32_t             fracSecs;
   /* We know an outage or Power Quality Event has been declared.   */
   /* Need to log an outage/restoration.  */
   EventData_s          pwrQualEvent;  /* Event info  */
   EventKeyValuePair_s  keyVal[ 4 ];   /* Event key/value pair info  */
   LoggedEventDetails   outageLoggedEventData;
   LoggedEventDetails   restLoggedEventData;

   pwrFileData = PWR_getpwrFileData();
   /* Re-Initialize the Start Alarm if a power failure had occured during the eSimLGArmed state */
   ( void )PWRCFG_set_SimulateLastGaspStart( PWRCFG_get_SimulateLastGaspStart() );
#endif

   if (_Initialized == true )
   {
      /* Task loop begins */
      for ( ;; )
      {
         /* Wait for a high priority alarm to be logged. */
         ( void )OS_MSGQ_Pend( &EvlAlarmHandler_MsgQ_, ( void * )&pAlarm, OS_WAIT_FOREVER );

         switch (pAlarm->eSysFmt) /*lint !e788 not all enums used within switch */
         {
#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
            case eSYSFMT_TIME:
            {
                tTimeSysMsg const *pTimeMsg;           //Pointer to time message
                pTimeMsg = (tTimeSysMsg *)( void * )&pAlarm->data[0];

               if ( schSimLGStartAlarmId_ == pTimeMsg->alarmId )
               {
                  tTimeSysCalAlarm alarmInfo;
                  ( void )TIME_SYS_GetCalAlarm( &alarmInfo, schSimLGStartAlarmId_);
                  /* Is this the Cal Alarm indicating we now have valid time? */
                  if ( ( alarmInfo.bSkipTimeChange == false )            &&
                       ( alarmInfo.ulAlarmDate == TIME_SYS_MAX_SEC_DAY ) &&
                       ( alarmInfo.ulAlarmTime == TIME_SYS_MAX_SEC_TIME ) )
                  {
                     ( void )PWRCFG_set_SimulateLastGaspStart( PWRCFG_get_SimulateLastGaspStart() );
                  }
                  else
                  {
                     INFO_printf( "Start Alarm" );
                     initSimLG();
                     uint32_t             uFirstSleepMilliseconds;
                     sysTimeCombined_t    currentTime = 0;

                     // Get time to wait before declaring an outage ( Delay time )
                     uFirstSleepMilliseconds = ( ( uint32_t ) PWRCFG_get_outage_delay_seconds() ) * 1000;
                     /* Make a copy of this to be used while in last gasp mode.  */
                     VBATREG_OUTAGE_DECLARATION_DELAY = uFirstSleepMilliseconds;
                     ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );  // If this fails, currentTime is 0, considered invalid.
                     if ( 0 == PWRLG_TIME_OUTAGE()  )
                     {   // PQE is not currently in process, update the outage time
                        PWR_setOutageTime( currentTime );
                        PWRLG_TIME_OUTAGE_SET( ( uint32_t )( currentTime / TIME_TICKS_PER_SEC ) );
                     }

                     sysTime_t            now;  /* Current system time. */
                     sysTime_dateFormat_t dt;   /* Current system time, in human readable format. */
                     TIME_UTIL_ConvertSysCombinedToSysFormat( &currentTime, &now );
                     ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &now, &dt );

                     DBG_LW_printf( "Outage time: %04u/%02u/%02u %02u:%02u:%02u\n", dt.year, dt.month, dt.day,
                                                                                       dt.hour, dt.min,   dt.sec );

                     PWRLG_MILLISECONDS() = (( uint32_t )20 * 60 * 1000) - uFirstSleepMilliseconds;
                     DBG_LW_printf( "Sleep First: %d, Window 0 seconds: %d, milliseconds: %d\n",
                                     uFirstSleepMilliseconds, PWRLG_SLEEP_SECONDS(), PWRLG_SLEEP_MILLISECONDS() );

                     ( void )setSimLGDurationAlarm( PWRCFG_get_SimulateLastGaspDuration() );
                     ( void )startOneShotTimerAlarm(uFirstSleepMilliseconds);
                     simLGState_ = eSimLGOutageDecl;
                  }
               }
               if ( schSimLGSleepTimerAlarmId_ == pTimeMsg->alarmId )
               {
                  switch ( simLGState_ )  /*lint !e788 not all enums used within switch */
                  {
                     case eSimLGOutageDecl :
                     {
                        INFO_printf( "LG Simulation: eSimLGOutageDecl");
                        PWRLG_OUTAGE_SET( 1 );     /* Outage declared.  */
                        VBATREG_SHORT_OUTAGE = 0;  /* No longer a short outage!  */
                        uint32_t uCounterMSecs  = PWRLG_SLEEP_SECONDS() * 1000;
                        uCounterMSecs += PWRLG_SLEEP_MILLISECONDS();
                        PWRLG_SLEEP_SECONDS() = 0;
                        PWRLG_SLEEP_MILLISECONDS() = 0;
                        ( void )startOneShotTimerAlarm( uCounterMSecs );

                        simLGState_ = eSimLGTx;
                     }
                     break;
                     case eSimLGTx :
                     {
                        if ( 0 != pSysMem->uMessageCounts.total )
                        {
                            initiateSimLGTx();
                        }
                        else
                        {
                           INFO_printf( "LG Simulation message count = 0, nothing to transmit" );
                        }
                     }
                     break;
                     default:
                     {
                         EVL_LGSimCancel();
                     }
                  } /*lint !e788 not all enums used within switch */
               }
               if ( schSimLGDurationAlarmId_ == pTimeMsg->alarmId )
               {
                  DBG_printf( "Duration Alarm" );
                  RESTORATION_TIME_SET(0); /* Force time to be invalid */
                  if ( RTC_Valid() ) /* If never updated, bump the power quality count. */
                  {
                     RESTORATION_TIME_SET ( RTC_TSR );   /* Record RTC seconds at power restored time.   */
                  }

                  /* Delete the Start Alarm if active */
                  if ( NO_ALARM_FOUND != schSimLGStartAlarmId_ )
                  {
                     if ( eSUCCESS == TIME_SYS_DeleteAlarm( schSimLGStartAlarmId_ ) )
                     {
                        schSimLGStartAlarmId_ = NO_ALARM_FOUND;
                     }
                     else
                     {
                        INFO_printf("Deleting of schSimLGStartAlarmId_ failed"); // Debugging purposes
                     }
                  }

                  /* Delete the Sleep Alarm if active */
                  if ( NO_ALARM_FOUND != schSimLGSleepTimerAlarmId_ )
                  {
                     if ( eSUCCESS == TIME_SYS_DeleteAlarm( schSimLGSleepTimerAlarmId_ ) )
                     {
                        schSimLGSleepTimerAlarmId_ = NO_ALARM_FOUND;
                     }
                     else
                     {
                        INFO_printf("Deleting of schSimLGSleepTimerAlarmId_ failed"); // Debugging purposes
                     }
                  }

                  if( eSimLGOutageDecl == simLGState_ )
                  {
                     /* Perform the Power Quality Event process */
                     DBG_logPrintf( 'I', "PQE declared" );
                     if ( 0 != RESTORATION_TIME() )      /* If RAM restoration time is valid, use it. */
                     {
                        TIME_UTIL_ConvertSecondsToSysCombined( RESTORATION_TIME(), 0, &restorationTime );
                     }

                     PWROR_printOutageStats( pwrFileData->outageTime, restorationTime );

                     pwrQualEvent.markSent = (bool)false;

                     /* Make sure that the powerQualityEventDuration has been met.  */
                     /* PQE started event */
                     TIMESTAMP_t pDateTime;                 /* Current date in seconds.   */
                     uint16_t    PowerQualityEventDuration; /* parameter value   */

                     ( void )TIME_UTIL_GetTimeInSecondsFormat( &pDateTime );
                     PowerQualityEventDuration = PWRCFG_get_PowerQualityEventDuration();
                     if ( ( PWRLG_TIME_OUTAGE() + PowerQualityEventDuration ) > pDateTime.seconds )
                     {
                        uint32_t sleepTime;
                        sleepTime = ( PWRLG_TIME_OUTAGE() + PowerQualityEventDuration ) - pDateTime.seconds;
                        sleepTime *= 1000;   /* Convert to milliseconds */
                        DBG_logPrintf( 'I', "waiting for PQE Duration: %d milliseconds", sleepTime );
                        ( void )setSimLGPowerOnStateTimer( sleepTime );
                     }
                     else
                     {
                        /* Start the PQ event as soon as possible */
                        ( void )setSimLGPowerOnStateTimer( 10 );
                     }
                  }
                  else
                  {
                     /* Perform the Power Restoration Event process */
                     DBG_logPrintf( 'I', "Outage declared" );
                     uint32_t             uSleepMilliseconds;  /* Random hold off time before sending restoration message. */

                     if ( 0 != RESTORATION_TIME() )      /* If RAM restoration time is valid, use it. */
                     {
                        TIME_UTIL_ConvertSecondsToSysCombined( RESTORATION_TIME(), 0, &restorationTime );
                     }

                     pwrQualEvent.markSent = (bool)true;

                     PWROR_printOutageStats( pwrFileData->outageTime, restorationTime );

                     /* We need to save multiple events to NV memory and update last gasp status flags.  A power down during
                        this process can create a misalignment of the events logged along with the last gasp status.  To guard
                        against this, lock the power mutex with the EMBEDDED_PWR_MUTEX mutex state. */
                     PWR_lockMutex( EMBEDDED_PWR_MUTEX );// Function will not return if it fails

                     /* Power failed event   */
                     pwrQualEvent.eventId = ( uint16_t )comDevicepowertestfailed;
                     *( uint16_t * )keyVal[ 0 ].Key = 0;
                     *( uint16_t * )keyVal[ 0 ].Value = 0;
                     pwrQualEvent.eventKeyValuePairsCount = 0;
                     pwrQualEvent.eventKeyValueSize = 0;
#if 0
                     pwrQualEvent.timestamp = PWRLG_TIME_OUTAGE();
#else
                     outageSeconds = pwrFileData->outageTime;
                     TIME_UTIL_ConvertSysCombinedToSeconds( &outageSeconds, &pwrQualEvent.timestamp, &fracSecs);
#endif
                     ( void )EVL_LogEvent( 200, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, &outageLoggedEventData );

                     /* Power restored event */
                     pwrQualEvent.eventId = ( uint16_t )comDevicePowertestrestored;
                     *( uint16_t * )keyVal[ 0 ].Key = 0;
                     *( uint16_t * )keyVal[ 0 ].Value = 0;
                     pwrQualEvent.eventKeyValuePairsCount = 0;
                     pwrQualEvent.eventKeyValueSize = 0;

                     TIME_UTIL_ConvertSysCombinedToSeconds( &restorationTime, &pwrQualEvent.timestamp, &fracSecs);
                     ( void )EVL_LogEvent( 199, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, &restLoggedEventData );

                     /* All updates to NV and last gasp flags have completed.  Release the EMBEDDED_PWR_MUTEX state of the
                        power mutex. */
                     PWR_unlockMutex( EMBEDDED_PWR_MUTEX );

                     // Sleep for a random delay period.
                     uSleepMilliseconds = aclara_randu( 0, EVL_getAmBuMaxTimeDiversity() * TIME_TICKS_PER_MIN );
                     DBG_logPrintf( 'I', "Sleeping %u Milliseconds", uSleepMilliseconds );
                     ( void )setSimLGPowerOnStateTimer( uSleepMilliseconds );
                  }
                  simLGState_ = eSimLGPowerOn;
               }
            }
            break;
#endif
            case eSYSFMT_EVL_ALARM_MSG:
            {
               uint32_t timeDiversity; /* Alarm message time diversity. */
#if ( EP == 1 )
               timeDiversity = aclara_randu( 0, EVL_getAmBuMaxTimeDiversity() * ONE_MIN );
#else
               // No need for diversity on TB
               timeDiversity = 1;
#endif
               if ( false == evlTimerInProgress )
               {
                  evlTimerInProgress = true;
                  /* Set and transmit a message with all the messages queued at this point in time.   */
                  evlInitMsg( &heepHdr );

                  numAlarms++;
                  INFO_printf( "Found %d alarm \n", numAlarms );
                  /* Allocate the maximum payload size and append the event alarms as when they occur to the same buffer */
                  INFO_printf( "Allocating %d bytes \n", APP_MSG_MAX_DATA );
                  pMsg = BM_alloc( ( uint16_t )APP_MSG_MAX_DATA );
                  if ( NULL == pMsg )
                  {
                     /* Allocate the memory from the debug buffer if the previous allocation fails */
                     pMsg = BM_allocDebug( ( uint16_t )APP_MSG_MAX_DATA );
                  }
                  if ( NULL != pMsg )
                  {
                     /* Start a timer and wait for it to expire */
                     ( void )TMR_ResetTimer( evlTimerId, timeDiversity );
                     INFO_printf( "Time diversity: %d\n", timeDiversity );

                     pFullMeterRead pRead = ( pFullMeterRead )( void * )pMsg->data;

                     /* Use now as the values interval end time  */
                     ( void )TIME_SYS_GetSysDateTime( &ReadingTime );
                     msgTime = ( uint32_t )( TIME_UTIL_ConvertSysFormatToSysCombined( &ReadingTime ) / TIME_TICKS_PER_SEC );

                     /* Add the event header to the payload   */
                     PACK_init( ( uint8_t * )&pRead->eventHeader.intervalTime, &packCfg );
                     PACK_Buffer( -( int8_t )sizeof( msgTime ) * 8, ( uint8_t * )&msgTime, &packCfg );
                     pMsg->x.dataLen = VALUES_INTERVAL_END_SIZE;

                     /* Add the NVP info to the message. */
                     uint8_t EvRdQty = ( uint8_t )( numAlarms << 4 );
                     PACK_Buffer( sizeof( EvRdQty  ) * 8, ( uint8_t * )&EvRdQty , &packCfg );
                     pMsg->x.dataLen += sizeof( EvRdQty );
                     pAlarmInfo = ( EventStoredData_s * )( void * )pAlarm->data;
                     pMsg->x.dataLen += evlAddAlarm( pAlarmInfo, &packCfg );
                  }
                  else
                  {
                     INFO_printf( "Unable to allocate size %d\n", APP_MSG_MAX_DATA);
                  }
               }
               else if ( NULL != pMsg )
               {
#if 0
                  INFO_printf( "sizeof( eventHdrt_t = %d, sizeof EDEventQty_t = %d, sizeof nvp_t = %d \n", sizeof( eventHdr_t),
                              sizeof( EDEventQty_t), sizeof( nvp_t ) );
#endif
                  /* The maximum buffer size allocated by BM_alloc is 1224 (including 4B header), so add the new alarms until the buffer is full
                     and the number of alarms is up to 15. Drop the rest of the alarms
                     TODO: There is a possibility for the last alarm to cross the boundary of the allocated buffer size of 1224 as the
                     data length is calculated on the go and if the last alarm exceeds the size of the available space then the alarm will
                     be sent without complete information */
                  if ( ( NULL != pAlarm ) && ( 15 > numAlarms ) && ( APP_MSG_MAX_DATA >= pMsg->x.dataLen ) ) /*lint !e644 at this point pMsg has been set */
                  {
                     numAlarms++;
                     INFO_printf( "Found %d alarms \n", numAlarms );
                     pAlarmInfo = ( EventStoredData_s * )( void * )pAlarm->data;
                     pMsg->x.dataLen += evlAddAlarm( pAlarmInfo, &packCfg );
                     INFO_printf ("Event Alarm data lenght: %d", pMsg->x.dataLen);
                  }
                  else
                  {
                     INFO_printf( "Event alarm dropped because of null buffer (or) max buffer size reached (or) max permissible alarms per message reached" );
                  }
               }
            }
            break;
            case eSYSFMT_TIME_DIVERSITY:
            {
#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
               uint16_t *timerID;
               timerID = (uint16_t*)&pAlarm->data[0]; /*lint !e826 */

               if( *timerID == powerOnStateTimerId_ )
               {
                  pwrFileData = PWR_getpwrFileData();

                  INFO_printf( "Power On State Timer" );
                  if ( eSimLGPowerOn == simLGState_ )
                  {
                     /* The LG Simulation is not consistent if it is started before the completion of
                        the real last gasp simulation and that is the reason PQ event is not checked with VBATREG_SHORT_OUTAGE */
                     if ( 0 == PWRLG_OUTAGE() )
                     {
                         INFO_printf( "PQ Event" );
                         /* We need to save multiple events to NV memory and update last gasp status flags.  A power down during
                         this process can create a misalignment of the events logged along with the last gasp status.  To guard
                         against this, lock the power mutex with the EMBEDDED_PWR_MUTEX mutex state. */
                         PWR_lockMutex( EMBEDDED_PWR_MUTEX );// Function will not return if it fails

                         pwrQualEvent.eventId = ( uint16_t )comDevicePowerPowerQualityTestEventStarted;
                         *( uint16_t * )keyVal[ 0 ].Key = 0;
                         *( uint16_t * )keyVal[ 0 ].Value = 0;
                         pwrQualEvent.eventKeyValuePairsCount = 0;
                         pwrQualEvent.eventKeyValueSize = 0;
                         outageSeconds = pwrFileData->outageTime;
                         TIME_UTIL_ConvertSysCombinedToSeconds( &outageSeconds, &pwrQualEvent.timestamp, &fracSecs);
                         ( void )EVL_LogEvent( 90, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, NULL );

                         /* PQE stopped event */
                         pwrQualEvent.eventId = ( uint16_t )comDevicePowerPowerQualityTestEventStopped;
                         *( uint16_t * )keyVal[ 0 ].Key = ( uint16_t )powerQuality;
                         *( uint16_t * )keyVal[ 0 ].Value = pwrFileData->uPowerAnomalyCount;
                         pwrQualEvent.eventKeyValuePairsCount = 1;
                         pwrQualEvent.eventKeyValueSize = 2;
                         TIME_UTIL_ConvertSysCombinedToSeconds( &restorationTime, &pwrQualEvent.timestamp, &fracSecs);
                         ( void )EVL_LogEvent( 89, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, NULL );

                         /* All updates to NV and last gasp flags have completed.  Release the EMBEDDED_PWR_MUTEX state of the
                            power mutex. */
                         PWR_unlockMutex( EMBEDDED_PWR_MUTEX );
                     }
                     else
                     {
                        INFO_printf( "PR Event" );
                        ( void )PWROR_tx_message( outageSeconds, &outageLoggedEventData, restorationTime, &restLoggedEventData );  /*lint !e645 !e644 init'd above   */
                     }
                  }
                  EVL_LGSimCancel();
               }
               else if( (*timerID == evlTimerId) )
#endif
               {
                  /* Since the alarms are added to the buffer as and when they occur
                     Update the event quantity to the message buffer */
                  if ( NULL != pMsg )
                  {
                     eventHdr_t *data = ( eventHdr_t* )pMsg->data; /*lint !e826 */
                     data->eventRdgQty.bits.eventQty = ( uint8_t )numAlarms;
                     /* send the built message */
                     ( void )HEEP_MSG_Tx ( &heepHdr, pMsg );

                     /* Reset the flag and the numnber of alarms */
                     evlTimerInProgress = false;
                     numAlarms = 0;
                     // TODO: check if any more alram is pending
                  }
               }
            }
            break;
            case eSYSFMT_EVL_ALARM_LOG_RESET_EVENT:
            {   /* event log corruption has just been observed, events to indicate the corruption and
                   buffer re-initialization need to be generated */
                /*lint -e{826} event info was put into pAlarm->data as (EventLogCoruptionClearedEvent_s) */
                generateEventLogCoruptionClearedEvents( (EventLogCoruptionClearedEvent_s const *)pAlarm->data );
                break;
            }
            default:
            {
               break;
            }
         } /*lint !e788 not all enums used within switch */

         BM_free( pAlarm );
      }
   }
   else
   {
      ERR_printf("Did not successfully initialize EVL" );
   }
}
//lint +esym(715,Arg0)  // Arg0 required for generic API, but not used here.
/***********************************************************************************************************************

   Function Name: evlInitMsg( pHEEP_APPHDR_t heepHdr )

   Purpose: Initialize alarm message header.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
static void evlInitMsg( pHEEP_APPHDR_t heepHdr )
{
   HEEP_initHeader( heepHdr );

   heepHdr->TransactionType   = ( uint8_t )TT_ASYNC;
   heepHdr->Resource          = ( uint8_t )bu_am;
   heepHdr->Method_Status     = ( uint8_t )method_post;
   heepHdr->qos               = 0x39;     /* Per the RF Electric Phase 1 spreadsheet   */
   heepHdr->callback          = NULL;

}
/***********************************************************************************************************************

   Function Name: evlAddAlarm( EventStoredData_s *alarmInfo, pack_t *packCfg )

   Purpose: Add an alarm to the inbound message.

   Arguments:  EventStoredData_s *alarmInfo - alarm information to add to inbound message
               pack_t *           packCfg - pack structure used to build message

   Returns: uint16_t - number of bytes added to message.

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
static uint16_t evlAddAlarm( EventStoredData_s *alarmInfo, pack_t *packCfg )
{
   uint32_t msgTime;    /* Time converted to seconds since epoch  */
   uint16_t param;      /* Used to pack 16 bit quantities into response buffer   */
   uint16_t bytesAdded; /* Number of bytes added to the message.  */
   uint8_t  *pair;      /* Pointer to event details name/value    */
   uint8_t  byteVal;    /* Needed for bit field packing  */
   uint8_t  tempBuf[EVENT_VALUE_MAX_LENGTH]; /* needed to store the alarm index ID for packing */

   (void)memset(tempBuf, 0, sizeof(tempBuf));  // initialize the temporary buffer
   (void)memcpy(tempBuf, (uint8_t *)&alarmInfo->alarmId, sizeof(alarmInfo->alarmId)); // store the alarm index ID value

   /* Time event was logged is sent to HE */
   msgTime = alarmInfo->timestamp.seconds;
   PACK_Buffer(-(int16_t)sizeof( msgTime ) * 8, ( uint8_t * )&msgTime, packCfg);
   bytesAdded = sizeof( msgTime );

   param = alarmInfo->eventId;   /* Event type next   */
   PACK_Buffer(-(int16_t)sizeof( param ) * 8, ( uint8_t * )&param, packCfg);
   bytesAdded += sizeof( param );

   // increment the number of pairs to accomadate the addition of the alarm index ID value
   byteVal = alarmInfo->EventKeyHead_s.nPairs + 1;
   PACK_Buffer( 4, &byteVal, packCfg );

   /* if the NVP size is zero, it must be updated to account for the addition of the alarm index ID size
      since the alarm index ID will be included as a NVP value */
   if( 0 == alarmInfo->EventKeyHead_s.valueSize )
   {
      byteVal = sizeof(alarmInfo->alarmId);
   }
   else
   {
      byteVal = alarmInfo->EventKeyHead_s.valueSize;
   }
   PACK_Buffer( 4, &byteVal, packCfg );
   bytesAdded += sizeof( byteVal );

   // add the alarm index ID as part of the NVP data
   uint16_t alarmIndexReadingType = (uint16_t)realTimeAlarmIndexID;
   PACK_Buffer(-(int16_t)sizeof( uint16_t ) * 8, (uint8_t *)&alarmIndexReadingType, packCfg);
   bytesAdded += sizeof( uint16_t );
   PACK_Buffer(-(int16_t)byteVal * 8, tempBuf, packCfg);
   bytesAdded += byteVal;

   pair = ( uint8_t * )alarmInfo + sizeof( *alarmInfo ); /* Event details follow structure   */
   for ( uint8_t pairCount = 0; pairCount < alarmInfo->EventKeyHead_s.nPairs; pairCount++ )
   {
      /* EndDeviceEvents.EndDeviceEventDetails.name - 2 bytes  */
      PACK_Buffer(-(int16_t)sizeof( uint16_t ) * 8, pair, packCfg);
      pair += sizeof( uint16_t );  /* Advance by key EndDeviceEventDetails.name size - 2 bytes */
      bytesAdded += sizeof( uint16_t );

      if( 0 == alarmInfo->EventKeyHead_s.valueSize )
      {  /* If the NVP size is zero, all NVP pairs will include a value of zero where the byte size of the value
            will be equal to the byte size of the alarm index ID */
         byteVal = 0;
         PACK_Buffer(-(int16_t)sizeof(byteVal) * 8, &byteVal, packCfg);
         bytesAdded += sizeof(byteVal);
      }
      else
      {  // NVP value size is greater than zero, just add the stored NVP value to the response buffer
         PACK_Buffer(-(int16_t)alarmInfo->EventKeyHead_s.valueSize * 8, pair, packCfg);
         pair += alarmInfo->EventKeyHead_s.valueSize;  // Advance by key Value size
         bytesAdded += alarmInfo->EventKeyHead_s.valueSize;
      }
   }
   return bytesAdded;
}

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/***********************************************************************************************************************

   Function name: EventLogMakeRoom

   Purpose: This function makes room in the buffer for more recent events.

   Arguments:
         PartitionData_t const *nvram  - Pointer to the nvram handle
         RingBufferHead_s *head        - Ring buffer meta data
         uint32_t length               - Space needed to clear

   Returns: returnStatus_t reVal - indicates whether creating more room to store an event was successful

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t EventLogMakeRoom( PartitionData_t const *nvram, RingBufferHead_s *head, uint32_t length )
{
   EventStoredData_s       sdata;             /* Current event, used to extract data about current event. */
   uint32_t                totalRemoved = 0;  /* Tally of the total number of space created in the buffer. */
   returnStatus_t          retVal = eSUCCESS; /* Status of attempt to make room in the event log for a new event */

   while ( totalRemoved < length )
   {
      if ( EventLogRead( nvram, head, ( uint8_t * )&sdata, sizeof( sdata ), FALSE ) >= ( int32_t )sizeof( sdata.size ) )
      {
         if ( EventLogRead( nvram, head, NULL, sdata.size, TRUE ) >= (int32_t)sizeof(sdata) ) /*lint !e644 sdata.size filled by previos EventLogRead()  */
         {
            totalRemoved += sdata.size;   /*lint !e644 sdata.size filled by previos EventLogRead()  */
         }
         else
         {  /* due to data corruption, we are unable to make room for the next logged event */
            totalRemoved = 0;
            retVal = eFAILURE;
            break;
         }
      }
   }

   if( totalRemoved > 0 )
   {  // we modified the head of the buffer to make room for new events, save the RAM copy describing the new buffer head and data length
      ( void )FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) );
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: EventInitializeMetaData

   Purpose: This function is called to initialize the Meta Data.

   Arguments: None

   Returns: returnStatus_t

   Side Effects: None

   Reentrant Code: Yes

   Notes:

   ******************************************************************************************************************** */
static returnStatus_t EventInitializeMetaData( void )
{
   returnStatus_t retVal = eFAILURE;

   _EvlMetaData.HighAlarmSequence    = 0;
   _EvlMetaData.NormalAlarmSequence  = 0;
   _EvlMetaData.oThreshold           = DEFAULT_OPPORTUNISTIC_THRESHOLD;
   _EvlMetaData.rtThreshold          = DEFAULT_REALTIME_THRESHOLD;
   _EvlMetaData.amBuMaxTimeDiversity = DEFAULT_AM_BU_MAX_TIME_DIVERSITY;

   _EvlMetaData.HighBufferHead.bufferSize = PRIORITY_EVENT_BUFFER_LENGTH;
   _EvlMetaData.HighBufferHead.length     = 0;
   _EvlMetaData.HighBufferHead.start      = 0;

   _EvlMetaData.NormalBufferHead.bufferSize = NORMAL_EVENT_BUFFER_LENGTH;
   _EvlMetaData.NormalBufferHead.length     = 0;
   _EvlMetaData.NormalBufferHead.start      = 0;
   _EvlMetaData.Initialized                 = ( bool )true;
   _EvlMetaData.realTimeAlarm               = ( bool )true;

#if ( EP == 1 )
   PWR_lockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
#else
   PWR_lockMutex() ; // Function will not return if it fails
#endif

   /*lint --e{655} ORing enum is OK here. Just looking for non-zero value. */
   if( eSUCCESS == FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData,
                               ( lCnt )sizeof( EventLogFile_s ) ) )
   {
      retVal = PAR_partitionFptr.parErase( 0, PART_NV_HIGH_PRTY_ALRM_SIZE, _EvlHighFlashHandle );
      /*lint --e{655} ORing enum is OK here. Just looking for non-zero value. */
      retVal |= PAR_partitionFptr.parErase( 0, PART_NV_LOW_PRTY_ALRM_SIZE, _EvlNormalFlashHandle );   /*lint !e641 implicit conversion of enum to integral */
   }
#if ( EP == 1 )
   PWR_unlockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
#else
   PWR_unlockMutex(); // Function will not return if it fails
#endif
   return retVal;
}

/***********************************************************************************************************************

   Function name: EventLogLoadBuffer

   Purpose: This function parses the ring buffer and returns only the event data as described in the event query.

   Arguments:
      uint8_t e_priority         - Which priority buffer to load from.
      char *target               - Location where the data is to be returned.
      uint32_t *size             - How much space is in the buffer passed in.
      uint32_t *totalSize        - Total amount of bytes returned from the request.
      uint8_t *alarmId           - Alarm ID of the last event returned.
      EventQueryRequest_e query  - Type of query to filter on if supplied.
      EventMarkSent_s *passback  - Pointer to passback data used in marking events sent.

   Returns: int32_t              - Returns EVL_ERROR on error or 0 on success

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static int32_t EventLogLoadBuffer( uint8_t e_priority, uint8_t *target, uint32_t size, uint32_t *totalSize,
                                   uint8_t *alarmId, EventQuery_s query, EventMarkSent_s *passback )
{
   uint16_t                nextSize;         /* Size of the next block of data. */
   buffer_t                *buf;             /* Temporary buffer used to read in one event block. */
   int32_t                 retVal = 0;       /* Return value EVL_ERROR on ERROR and 0 for SUCCESS. */
   bool                    addToOutput;      /* If set the data is included in the results. */
   uint16_t                eventId;          /* Event ID from log */
   uint32_t                eventTime;        /* Current Event time to be searched for. */
   EventStoredData_s       sdata;            /* Current event, used to extract data about current event. */
   uint32_t                totalBytes;       /* The number of available data in the ring buffer. */
   uint32_t                bytesRead;        /* The number of bytesRead out of the ring buffer. */
   uint32_t                sizeLeft;         /* The number of bytes left in the results buffer. */
   uint32_t                physOffset = 0;   /* The physical offset into the buffer used by EVL_MarkSent. */
   uint32_t                offset = 0;       /* Offset into the event data. */
   PartitionData_t const   *nvram;           /* Pointer to the nvram partition. */
   RingBufferHead_s        *rHead;           /* Pointer to the ring buffer metat data. */
   EventHeadEndData_s      he;               /* The converted data that goes back to the head end */
   //uint16_t                pairSize;         /* Size of the key pair data. */
   uint16_t                alarmSize;        /* total number of bytes needed to represent the alarm in a response buffer */
   uint8_t                 alarmIdx;         /* Current alarm index to be searched for. */
   meterReadingType indexBufferIndicator;    /* The alarm index value in the buffer for the event. */

   sizeLeft = size;
   if ( PRIORITY_INVALID( e_priority ) )
   {
      return EVL_ERROR;
   }

   if ( e_priority >= _EvlMetaData.rtThreshold )
   {
      nvram = _EvlHighFlashHandle;
      rHead = &_EvlMetaData.HighBufferHead;
      indexBufferIndicator = realTimeAlarmIndexID;
   }
   else
   {
      nvram = _EvlNormalFlashHandle;
      rHead = &_EvlMetaData.NormalBufferHead;
      indexBufferIndicator = opportunisticAlarmIndexID;
   }
   /* Save the meta data, to be returned to the original values after read is done. */
   uint32_t savedStart = rHead->start;
   uint32_t savedLength = rHead->length;

   /****************************************************************************
      Save the buffer pointers. Needs to be done so after the ring buffer is
      searched in can be put back to its original condition
   ***************************************************************************/
   totalBytes = RINGBUFFER_AVAILABLE_DATA( rHead );
   bytesRead = 0;

   /****************************************************************************
      Loop through the ring buffer for event data. If this is a query then the
      event data is looked at to see if it satisfies the query parameters. If
      it does satify the query parameters it is added to the results.
   ***************************************************************************/
   while ( ( sizeLeft != 0 ) && ( bytesRead < totalBytes ) && ( retVal == EVL_SUCCESS ) )
   {
      addToOutput = (bool)false;

      /* Get the size of the Event block */
      if ( EventLogRead( nvram, rHead, ( uint8_t * )&sdata, sizeof( sdata ), FALSE ) >= ( int32_t )sizeof( sdata ) )
      {
         nextSize = sdata.size; /*lint !e644 at this point sdata has been set by EventLogRead() */
         if ( nextSize < sizeof( EventStoredData_s ) )   /* This is a data corruption problem - abort!   *//*lint !e644 at this point nextSize has been set */
         {
            ERR_printf( "Improper length event found in partition data @ 0x%08x\n",
                        nvram->PhyStartingAddress + rHead->start );
            INFO_printHex( "_EvlMetaData: ", ( uint8_t * )&_EvlMetaData, sizeof( _EvlMetaData ) );
            INFO_printHex( "sdata: ", ( uint8_t * )&sdata, sizeof( sdata ) );

            // erase the corrupted partition
            if( eSUCCESS == eraseEventLogPartition(nvram->ePartition) )
            {
               INFO_printf( "Event log corruption forced event log clear during call to EventLogLoadBuffer" );
            }

            if ( e_priority >= _EvlMetaData.rtThreshold )
            {
               savedStart = _EvlMetaData.HighBufferHead.start;
               savedLength = _EvlMetaData.HighBufferHead.length;
            }
            else
            {
               savedStart = _EvlMetaData.NormalBufferHead.start;
               savedLength = _EvlMetaData.NormalBufferHead.length;
            }

            retVal = EVL_ERROR;
            break;
         }
         physOffset = rHead->start;

         if ( sdata.size > sizeof( sdata ) )    /* Event pairs attached... *//*lint !e644 at this point sdata has been set */
         {
            /* Allocate a buffer for the Event Data */
            buf = BM_alloc( ( uint16_t )nextSize );
            if ( buf == NULL )
            {
               ERR_printf( "Failed to allocate memory for EventLogLoadBuffer\n" );
               retVal = EVL_ERROR;
               break;
            }

            uint8_t *bufCopy = buf->data; /* Satisfy lint custodial pointer issue   */
            ( void )EventLogRead( nvram, rHead, bufCopy, nextSize, TRUE );
         }
         else
         {
            RINGBUFFER_COMMIT_READ( rHead, nextSize );   /* Needed because call to EventLogRead with purge skipped. This
                                                            handles wrapping at the end of the buffer. rHead restored at
                                                            end of routine.  */
            rHead->length -= nextSize;
            buf = NULL;    /* No dynamic buffer required.   */
         }
         bytesRead += nextSize;
         alarmIdx = sdata.alarmId;  /*lint !e644 at this point sdata has been set */
         eventTime = sdata.timestamp.seconds;

         /************************************************************************
            If a query exists then check the data to make sure it satisfies the
            query criteria. If there is no query active return all the data.
         ***********************************************************************/
         if ( query.qType == QUERY_BY_DATE_e )
         {  // this is a date range query, does event timestamp occur within supplied date range?
            if ( ( eventTime >= query.start_u.timestamp ) && ( eventTime <= query.end_u.timestamp ) )
            {  // set flag to return the event information
               addToOutput = (bool)true;
            }
         }
         else if ( query.qType == QUERY_BY_INDEX_e )
         {  // this is a query by alarm index id
            if ( query.start_u.eventId > query.end_u.eventId )
            {  // the start index value is larger than the end index value
               if( ( alarmIdx <= query.end_u.eventId ) || (alarmIdx >= query.start_u.eventId ) )   /*lint !e644 at this point alarmIdx has been set */
               {  // set flag to return this alarm index meets the selected query
                  addToOutput = (bool)true;
               }
            }
            else
            {  // the end index is larger than the start index
               if ( ( alarmIdx >= query.start_u.eventId ) && ( alarmIdx <= query.end_u.eventId ) ) /*lint !e644 at this point alarmIdx has been set */
               {  // set flag to return this alarm index meets the selected query
                  addToOutput = (bool)true;
               }
            }

            /* The event log circular buffer is very large.  New events are contiually added to the end of the
               buffer.  It is possible to have mutiple events with the same alarm index ID in the circular buffer.
               We need to determine if the current event is the latest.  If it is not the latest, we will update the
               addToOutput flag to "false" prevent including the older event. */
            if ( addToOutput )
            {
               if( obsoleteAlarmIndex( rHead, nvram, alarmIdx) )
               {  // the alarm index is obsolete, there is a more recent index in the event log, do not add event
                  addToOutput = (bool)false;
               }
            }
         }
         else
         {
            /**********************************************************************
               The passback is only used on a 'GET LOG' call. This is used by
               EVL_MarkSent to clear mark the event as sent. Also don't include
               any events that have already been sent.
            *********************************************************************/
            if ( !sdata.EventKeyHead_s.sent )
            {
               addToOutput = (bool)true;
            }
         }

         if ( addToOutput )
         {
            /*******************************************************************
               There is no need to pass all the saved data back to the HE. So
               this code will extract the pertinent data and return it.
            *******************************************************************/
            ( void )memset( &he, 0, sizeof( EventHeadEndData_s ) );

            *alarmId       = sdata.alarmId;           /*lint !e644 at this point sdata has been set */
            eventId        = sdata.eventId;           /*lint !e644 at this point sdata has been set */
            he.eventId     = sdata.eventId;           /*lint !e644 at this point sdata has been set */
            he.timestamp   = sdata.timestamp.seconds; /*lint !e644 at this point sdata has been set */

            /*******************************************************************
               Since the stored data has 5 bits and the return data has only
               four we mask off bit 5 NOTE: not changing the 5 bits because
               this will mess up the units in the field already running.  Also,
               the alarm index ID will be included in the response. Need to
               update the number of pairs to reflect the additional NVP.
            *******************************************************************/
            he.EventKeyHead_s.nPairs = (sdata.EventKeyHead_s.nPairs & 0xF) + 1; //TODO: add check for 14 or less nvp pairs
            he.EventKeyHead_s.valueSize = sdata.EventKeyHead_s.valueSize & 0xF;

            /* if the NVP value size is zero, we have two scenarios.  Either there is no NVP data or there are no NVP
               values associated with the name value pair data.  Either scenario, we need to update the number of pairs
               and the size of the the pairs to reflect the addition of the alarm index ID. */
            if( 0 == he.EventKeyHead_s.valueSize )
            {
               he.EventKeyHead_s.valueSize = sizeof(alarmIdx);
            }

            // size in bytes to represent the entire alarm
            alarmSize = sizeof( EventHeadEndData_s ) + ( he.EventKeyHead_s.nPairs * EVENT_KEY_MAX_LENGTH )
                        + ( he.EventKeyHead_s.valueSize * he.EventKeyHead_s.nPairs );

            /* Do we have enough room to add the event data to the target location?  We need to account for the addition
               of the alarm index ID for the event which will be added at the beginning of the NVP list. */
            /*lint -e{826,669,662} destination of memcpy calls are large enough to hold data  */
            if ( alarmSize <= ( uint16_t )sizeLeft )
            {
               // Copy the timestamp, eventID, and NVP information for the current event.  Adjust offset into destination.
               ( void )memcpy( target + offset, (uint8_t *)&he, sizeof( EventHeadEndData_s ) );
               offset += ( uint32_t )sizeof( EventHeadEndData_s );

               /* Add the alarm index to the beginning of the list of NVP's for this event and update the offset value
                  used to write to the destination location. */
               ( void )memcpy( target + offset, (uint8_t *)&indexBufferIndicator, sizeof( indexBufferIndicator ) );
               offset += (uint32_t)sizeof(indexBufferIndicator);
               ( void )memset(target + offset, 0, he.EventKeyHead_s.valueSize);
               ( void )memcpy( target + offset, &alarmIdx, sizeof( alarmIdx ) );
               offset += (uint32_t)he.EventKeyHead_s.valueSize;

               /*lint -e{613,644} if pairs exist, buf was allocated above.  */
               /* There will always be an alarm index in the NVP list hence at the very least there will always be 1 NVP.
                  Check to see if there are any additional NVP's that need to be added. */
               if ( he.EventKeyHead_s.nPairs > 1 )
               {  // we have additional NVP information that needs to be added to the reponse message
                  uint8_t *pdataBuf = buf->data;
                  pdataBuf += sizeof( EventStoredData_s ); // move pointer to start of NVP information
                  for( uint8_t i = 0; i < (sdata.EventKeyHead_s.nPairs & 0xF); i++ )
                  {
                     // add the name value pair name
                     ( void )memcpy( target + offset, pdataBuf, EVENT_KEY_MAX_LENGTH );
                     offset   += (uint32_t)EVENT_KEY_MAX_LENGTH;
                     pdataBuf += (uint32_t)EVENT_KEY_MAX_LENGTH;

                     // add the name value pair value
                     if( 0 == ( sdata.EventKeyHead_s.valueSize & 0xF) )
                     {  /* This is a special case where the name value pair size is zero. Since we are adding the
                           alarm index ID to the list of NVP's, we need to set the value for all the other NVP's
                           to be zero. */
                        (void)memset( target + offset, 0, he.EventKeyHead_s.valueSize );
                     }
                     else
                     {  // copy NVP data to target
                        ( void )memcpy( target + offset, pdataBuf, he.EventKeyHead_s.valueSize );
                        pdataBuf += he.EventKeyHead_s.valueSize;
                     }
                     offset   += he.EventKeyHead_s.valueSize;
                  }
               }

               /* update the running total of event data that has been added to the reponse buffer, this will include the
                  timestamp, event id, NVP information, and an additional NVP for the alarm index ID for the specified event. */
               sizeLeft    -= alarmSize;
               *totalSize  += alarmSize;

               if ( passback )
               {
                  passback->eventIds[passback->nEvents].eventId   = eventId;     /*lint !e644 at this point eventId has been set */
                  passback->eventIds[passback->nEvents].priority  = e_priority;
                  passback->eventIds[passback->nEvents].offset    = physOffset;
                  passback->eventIds[passback->nEvents].size      = nextSize;
                  passback->nEvents++;
               }
            }
            else
            {  // we have run out of room to store event information in the destination buffer
               retVal = EVL_SIZE_LIMIT_REACHED;
            }
         }
         if ( buf != NULL )
         {
            BM_free ( buf );
         }
      }
      else
      {
         break;
      }
   }
   /* Return buffer pointers to original positions */
   rHead->start = savedStart;
   rHead->length = savedLength;

   return retVal;
}

/***********************************************************************************************************************

   Function name: EventLogWrite

   Purpose: This functions writes a buffer supplied into the ring buffer.

   Arguments:
         PartitionData_t const *nvram  - Handle to the ring buffer nvram.
         RingBufferHead_s *head        - Ring buffer meta data.
         uint8_t *data                 - Pointer to the data to write to the ring buffer.
         uint32_t length               - Length of the dat to be written to the ring buffer.
         bool overwrite                - Overwrite data in the ring buffer.

   Returns: int32_t                    - Returns EVL_ERROR on ERROR and the number of bytes written on SUCCESS

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static int32_t EventLogWrite( PartitionData_t const *nvram, RingBufferHead_s *head,
                              uint8_t const * const data, uint32_t length, bool overwrite )
{
   uint32_t       nBytesOver    = 0;      /* Number of bytes over the end of the buffer. */
   uint32_t       nBytesLeft    = length; /* Number of bytes left to write. */
   uint32_t       nBytesToEnd   = 0;      /* Number of bytes to the end of the buffer. */
   uint32_t       nBytesWritten = 0;      /* Number of bytes written into the flash. */
   returnStatus_t retVal        = eSUCCESS;

   /* No room in the ring buffer for new data? Make room */
   if ( ( length > RINGBUFFER_AVAILABLE_SPACE( head ) ) && ( !overwrite ) )
   {
      nBytesOver = length - RINGBUFFER_AVAILABLE_SPACE( head );
      if( eFAILURE == EventLogMakeRoom( nvram, head, nBytesOver ) )
      {  /* We were unable to make room to store the next event due to a data corruption issue in the
	        event logger.  We need to erase the event log partition that has experienced corruption. */
         if( eFAILURE == ( retVal = eraseEventLogPartition( nvram->ePartition ) ) )
         {
           length = (uint32_t)EVL_ERROR;
         }
      }
   }

   if( eSUCCESS == retVal )
   {
      if ( overwrite )
      {
         nBytesToEnd = head->bufferSize - head->start;
      }
      else
      {
         nBytesToEnd = RINGBUFFER_AVAILABLE_SPACE_TO_END( head );
      }

#if ( EP == 1 )
      PWR_lockMutex( PWR_MUTEX_ONLY );// Function will not return if it fails
#else
      PWR_lockMutex();// Function will not return if it fails
#endif

      /* Will the write wrap around the end of the buffer */
      if ( length > nBytesToEnd )
      {
         /* Write the date to the end of the buffer and then finish the write down below. */
         if ( overwrite )
         {
            retVal = PAR_partitionFptr.parWrite( ( uint32_t )RINGBUFFER_STARTS_AT( head, 0 ),
	                                         data, ( lCnt )nBytesToEnd, nvram );
         }
         else
         {
            retVal = PAR_partitionFptr.parWrite( ( uint32_t )RINGBUFFER_END( head ), data, ( lCnt )nBytesToEnd, nvram );
         }

         head->length += nBytesToEnd;
         nBytesWritten = nBytesToEnd;
         nBytesLeft   -= nBytesToEnd;
      }

      /* Finish the write. If there was no wrap on the buffer then write all the data here. */
      if ( overwrite )
      {
         retVal = PAR_partitionFptr.parWrite( ( uint32_t )RINGBUFFER_STARTS_AT( head, nBytesWritten ),
	                                        data + nBytesWritten, ( lCnt )nBytesLeft, nvram );
      }
      else
      {
         retVal = PAR_partitionFptr.parWrite( ( uint32_t )RINGBUFFER_END( head ),
	                                      data + nBytesWritten, ( lCnt )nBytesLeft, nvram );
      }
#if ( EP == 1 )
      PWR_unlockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
#else
      PWR_unlockMutex(); // Function will not return if it fails
#endif

      if ( retVal != eSUCCESS )
      {
         ERR_printf( "EventLogWrite failed" );
         return EVL_ERROR;
      }

      head->length += nBytesLeft;
   }

   return ( int32_t )length;
}

/***********************************************************************************************************************

   Function name: EventLogRead

   Purpose: This function reads data from the ring buffer and places into the target.

   Arguments:
         PartitionData_t const *nvram  - Handle to the ring buffer nvram.
         RingBufferHead_s *head        - Ring buffer meta data.
         char *target                  - Location to return the read data.
         uint32_t amount               - Amount of data to read.
         bool purge                    - Set this true if you want to remove the data read.
                                         This increments the internal buffer pointers.

   Returns: int32_t                    - Return EVL_ERROR on ERROR, number of bytes read on SUCCESS.

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static int32_t EventLogRead( PartitionData_t const *nvram, RingBufferHead_s *head,
                             uint8_t * const target, uint32_t amount, bool purge )
{
   uint32_t       nBytesLeft  = amount;   /* Number of bytes left to read. */
   uint32_t       nBytesToEOB = 0;        /* Number of data bytes to the end of the buffer. */
   uint32_t       offset      = 0;        /* Offset in the result buffer. */
   returnStatus_t retVal;                 /* Return value of the nvram read. */

   uint32_t totalData = RINGBUFFER_AVAILABLE_DATA( head );  /* How much data is in the ring buffer?   */

   /* Trying to read more data than exists in the ring buffer? */
   if ( amount > totalData )
   {
      ERR_printf( "%S asking for %d and only %d is available." );
      return EVL_ERROR;
   }

   /* How much data is there to the end of the buffer */
   if ( head->start + head->length < head->bufferSize )
   {
      nBytesToEOB = 0;
   }
   else
   {
      nBytesToEOB = head->bufferSize - head->start;
   }

   /* Check for the wrap */
   if ( head->start + amount > head->bufferSize )
   {
      /* Read up to the end of the buffer and then finish the read below. */
      if ( target )
      {
         retVal = PAR_partitionFptr.parRead( ( uint8_t* )target, ( dSize )head->start, ( lCnt )nBytesToEOB, nvram );
         if ( retVal != eSUCCESS )
         {
            ERR_printf( "EVL failed to read NVRAM at line %d", __LINE__ );
            return EVL_ERROR;
         }
      }

      nBytesLeft -= nBytesToEOB;
      offset     += nBytesToEOB;

   }

   /* Finish the read of the data, if there is no wrap then read all the data here. */
   if ( target )
   {
      retVal = PAR_partitionFptr.parRead( ( uint8_t* )( target + offset ),
                                          ( dSize )RINGBUFFER_STARTS_AT( head, offset ),
                                          ( lCnt )nBytesLeft, nvram );
      if ( retVal != eSUCCESS )
      {
         ERR_printf( "EVL failed to read NVRAM at line %d", __LINE__ );
         return EVL_ERROR;
      }
   }

   /* This moves the pointer to the next event record.  If head is the "real" one instead of a local copy it essentially erases the data in the ring buffer. */
   if ( purge )
   {
      RINGBUFFER_COMMIT_READ( head, amount );
      head->length -= amount;
   }

   return ( int32_t )amount;
}

/***********************************************************************************************************************

   Function name: EVL_SetThresholds

   Purpose: This function is called to write the event thresholds.

   Arguments:
         uint8_t rtThreshold  - Real Time Event Threshold.
         uint8_t oThreshold   - Opportunistic Threshold.

   Returns: returnStatus_t

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
returnStatus_t EVL_SetThresholds( uint8_t rtThreshold, uint8_t oThreshold )
{
   returnStatus_t ret = eAPP_INVALID_VALUE;

   if( rtThreshold >= oThreshold )
   {
      _EvlMetaData.rtThreshold = rtThreshold;
      _EvlMetaData.oThreshold = oThreshold;
      ret = FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) );
   }

   return( ret );
}

/***********************************************************************************************************************

   Function name: EVL_GetThresholds

   Purpose: This function is called get the event threshold values.

   Arguments:
         uint8_t *rtThreshold - Location for the Real Time Threshold.
         uint8_t *oThreshold  - Location for the Opportunistic Threshold.

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void EVL_GetThresholds( uint8_t *rtThreshold, uint8_t *oThreshold )
{
   *rtThreshold = _EvlMetaData.rtThreshold;
   *oThreshold = _EvlMetaData.oThreshold;
}

/***********************************************************************************************************************

   Function name: EVL_getAmBuMaxTimeDiversity

   Purpose: This function is called when the system initializes to initialize the event logger.

   Arguments:
         bool initialize   - Reset the evl back to default settings.

   Returns: returnStatus_t - returns EVL_ERROR on ERROR and 0 on SUCCESS.

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
uint8_t EVL_getAmBuMaxTimeDiversity( void )
{
   uint8_t uAmBuMaxTimeDiversity;
   uAmBuMaxTimeDiversity =  _EvlMetaData.amBuMaxTimeDiversity;
   return( uAmBuMaxTimeDiversity );
}

/***********************************************************************************************************************

   Function name: EVL_setAmBuMaxTimeDiversity

   Purpose: This function is called when the system initializes to initialize the event logger.

   Arguments:
         bool initialize   - Reset the evl back to default settings.

   Returns: returnStatus_t - returns EVL_ERROR on ERROR and 0 on SUCCESS.

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
returnStatus_t EVL_setAmBuMaxTimeDiversity( uint8_t uAmBuMaxTimeDiversity )
{
   returnStatus_t retVal;

   if ( uAmBuMaxTimeDiversity <= MAX_AM_BU_MAX_TIME_DIVERSITY )
   {
      _EvlMetaData.amBuMaxTimeDiversity = uAmBuMaxTimeDiversity;
      retVal = FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) );
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }
   return( retVal );
}

/***********************************************************************************************************************

   Function name: EVL_Initalize

   Purpose: This function is called when the system initializes to initialize the event logger.

   Arguments:
         bool initialize   - Reset the evl back to default settings.

   Returns: returnStatus_t - returns EVL_ERROR on ERROR and 0 on SUCCESS.

   Side Effects: None

   Reentrant Code: Yes

   Notes:

   ******************************************************************************************************************** */
returnStatus_t EVL_Initalize( void )
{
   returnStatus_t rVal = eFAILURE;    /* Return value of the File commands. */

   if ( (eSUCCESS == PAR_partitionFptr.parOpen(&_EvlHighFlashHandle, ePART_NV_HIGH_PRTY_ALRM, 0L) ) &&
        (eSUCCESS == PAR_partitionFptr.parOpen(&_EvlNormalFlashHandle, ePART_NV_LOW_PRTY_ALRM, 0L) ) &&
        (eSUCCESS == FIO_fopen( &_EvlFileHandleMeta,          /* File Handle so that Interval Data can configurable parameters */
                                ePART_SEARCH_BY_TIMING,       /* Search for best partition according to the timing. */
                                ( uint16_t )eFN_EVL_DATA,     /* File ID (filename) */
                                ( lCnt )sizeof( EventLogFile_s ), /* Size of the data in the file. */
                                FILE_IS_NOT_CHECKSUMED,       /* File attributes to use. */
                                ID_FILE_UPDATE_RATE_META_SEC, /* The update rate of the data in the file. */
                                &_EvlFileStatusMeta ) ) )
   {
      if ( _EvlFileStatusMeta.bFileCreated )
      {
         rVal = EventInitializeMetaData();
      }
      else
      {
         rVal = FIO_fread( &_EvlFileHandleMeta, ( uint8_t * )&_EvlMetaData, 0, ( lCnt )sizeof( _EvlMetaData ) );
      }
      /*
        Even if the file exists doesnt mean it is initialize correctly. So if is not initialized correctly
        then initialize it.
      */
      if ( !_EvlMetaData.Initialized )
      {
         rVal = EventInitializeMetaData();
      }
   }

   if ( rVal == eSUCCESS )
   {
#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
      //TODO RA6: NRJ: determine if semaphores need to be counting
      if ( OS_MUTEX_Create(&_EVL_MUTEX) && OS_MSGQ_Create(&EvlAlarmHandler_MsgQ_, EVL_NUM_MSGQ_ITEMS) && OS_SEM_Create( &SimLGTxDoneSem, 0 ) )
#else
      if ( OS_MUTEX_Create(&_EVL_MUTEX) && OS_MSGQ_Create(&EvlAlarmHandler_MsgQ_, EVL_NUM_MSGQ_ITEMS) )
#endif
      {
         ( void )memset( &tmrSettings, 0, sizeof( tmrSettings ) );
         tmrSettings.pSemHandle     = NULL;
         tmrSettings.ulDuration_mS  = 1;        /* Must be non-zero to add a timer! */
         tmrSettings.bOneShotDelete = ( bool )false;
         tmrSettings.bExpired       = ( bool )true;
         tmrSettings.bStopped       = ( bool )true;
         tmrSettings.bOneShot       = ( bool )true;
         tmrSettings.pFunctCallBack = timerDiversityTimerExpired;
         rVal = TMR_AddTimer( &tmrSettings );
         if ( eSUCCESS == rVal )
         {
            evlTimerId  = ( uint16_t )tmrSettings.usiTimerId;
         }
      }
      else
      {
         rVal = eFAILURE;
      }
   }

   if ( rVal == eSUCCESS )
   {
      _Initialized = true;
   }

   return rVal;
}

/***********************************************************************************************************************

   Function name: EVL_QueryBy

   Purpose: Use this function to query the event log. QUERY_BY_DATE will look for timestamps that in the range provided
            for the date. QUERY_BY_EVENT will look for event ids that in the range provided for events. This
            function searches both queues and merges the data into one buffer.

   Arguments:
         EventQueryRequest_e query  - The query typy which includes ranges
         char *results              - Address of the buffer to place the results data.
         uint32_t sizeRequest       - Size of the data requested.
         uint32_t *sizeReturned     - Size of the actual data returned in the buffer.
         uint8_t *alarmId           - returned AlarmID

   Returns: int32_t                 - Returns EVL_ERROR on ERROR and 0 on SUCCESS.

   Side Effects: None

   Reentrant Code: Yes

   Notes:
 ******************************************************************************************************************** */
int32_t EVL_QueryBy( EventQuery_s query, uint8_t *results, uint32_t sizeRequest,
                     uint32_t *sizeReturned, uint8_t *alarmId )
{
   uint32_t totalReceived = 0;   /* Number of bytes received from the high priority buffer. */
   int32_t  retVal = 0;          /* Status variable from the calls to functions. */
   uint32_t sizeLeft;            /* Number of bytes left in the results buffer. */

   *sizeReturned = 0;

   if ( results == NULL )
   {
      ERR_printf( "Destination buffer is null" );
      return EVL_ERROR;
   }

   OS_MUTEX_Lock( &_EVL_MUTEX ); // Function will not return if it fails

   // If search by index, verify the severity requested is high priority, otherwise just get data from buffer
   if( ( QUERY_BY_INDEX_e == query.qType && HIGH_PRIORITY_SEVERITY == query.alarmSeverity ) ||
       ( QUERY_BY_INDEX_e != query.qType ) )
   {
      /* Get all the HIGH priority data that matches the search criteria. */
      if ( RINGBUFFER_AVAILABLE_DATA( &_EvlMetaData.HighBufferHead ) != 0 )
      {
         retVal = EventLogLoadBuffer( _EvlMetaData.rtThreshold, results, sizeRequest,
                                      &totalReceived, alarmId, query, NULL );
         if ( retVal == EVL_ERROR )
         {
            ERR_printf( "Failed to load the data from the HIGH priority buffer\n" );
            OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
            return retVal;
         }
      }
   }

   sizeLeft = sizeRequest - min( sizeRequest, totalReceived );

   // If search by index, verify the severity requested is medium priority, otherwise just get data from buffer
   if( ( QUERY_BY_INDEX_e == query.qType && MEDIUM_PRIORITY_SEVERITY == query.alarmSeverity ) ||
       ( QUERY_BY_INDEX_e != query.qType ) )
   {
      /* Get all the NORMAL priority date that matches the search criteria */
      if ( ( RINGBUFFER_AVAILABLE_DATA( &_EvlMetaData.NormalBufferHead ) != 0 ) && ( sizeLeft != 0 ) )
      {
         retVal = EventLogLoadBuffer( _EvlMetaData.oThreshold, results + totalReceived,
                                      sizeLeft, &totalReceived, alarmId, query, NULL );
         if ( retVal == EVL_ERROR )
         {
            OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
            return retVal;
         }
      }
   }

   *sizeReturned = totalReceived;

#if ( EP == 1 )
   PWR_lockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
#else
   PWR_lockMutex(); // Function will not return if it fails
#endif

   /* Make sure data is updated */
   ( void )FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) );
#if ( EP == 1 )
   PWR_unlockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
#else
   PWR_unlockMutex(); // Function will not return if it fails
#endif

   OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails

   return retVal;
}

/***********************************************************************************************************************

   Function name: EVL_MarkSent

   Purpose: This function is called to clear the event log of events that have been sent back to the head end and are
            no longer valid entries. This will free up space in the ring buffer. If there is an associated
            callback function with the event it will be called with the sent parameters.

   Arguments:
         const EventMarkSent_s   - The list of events to clear.

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void EVL_MarkSent( const EventMarkSent_s *events )
{
   uint16_t                i;                /* Loop control values. */
   uint32_t                totalCleared = 0; /* Keep track of all the events processed. */
   EventStoredData_s       data;             /* Pointer used to look ahead at the date in the buffer. */
   RingBufferHead_s        tempHead;         /* Set up overwrite to flash by updating a temp ring buffer meta data. */
   PartitionData_t const   *nvram;           /* Handle to the nvram ring buffer. */

   OS_MUTEX_Lock( &_EVL_MUTEX ); // Function will not return if it fails

   ( void )memset( &data, 0, sizeof( EventStoredData_s ) );

   /* Loop through the list of event ids passed in. */
   for ( i = 0; i < events->nEvents; i++ )
   {
      /* Use the passback data to set up the ring buffer meta data. */
      if ( events->eventIds[i].priority >= _EvlMetaData.rtThreshold )
      {
         tempHead.bufferSize = _EvlMetaData.HighBufferHead.bufferSize;
         tempHead.length     = events->eventIds[i].size;
         tempHead.start      = events->eventIds[i].offset;
         nvram               = _EvlHighFlashHandle;

         ( void )EventLogRead( nvram, &tempHead, ( uint8_t * )&data, sizeof( data ), FALSE );
      }
      else if ( events->eventIds[i].priority >= _EvlMetaData.oThreshold )
      {
         tempHead.bufferSize = _EvlMetaData.NormalBufferHead.bufferSize;
         tempHead.length     = events->eventIds[i].size;
         tempHead.start      = events->eventIds[i].offset;
         nvram               = _EvlNormalFlashHandle;

         ( void )EventLogRead( nvram, &tempHead, ( uint8_t * )&data, sizeof( data ), FALSE );
      }
      else
      {
         ERR_printf( "EVL_MarkSent passback has a bad priority for event: %d", events->eventIds[i].eventId );
         continue;
      }
      /**************************************************************************
         Make sure that the event ID is the correct one in the buffer. Then if
         the event is not marked sent, then make the callback if one exists.
      *************************************************************************/
      if ( data.eventId == events->eventIds[i].eventId )
      {
         if ( data.EventKeyHead_s.sent )
         {
            INFO_printf( "event: 0x%x on priority: %d sent already", events->eventIds[i].eventId,
                         events->eventIds[i].priority );
            continue;
         }
         /* Mark the event as sent */
         data.EventKeyHead_s.sent = EVL_EVENT_SENT;
         ( void )EventLogWrite( nvram, &tempHead, ( uint8_t * )&data, sizeof( EventStoredData_s ), TRUE );
         totalCleared++;
#if 0 /* Currently callbacks are not used. Would have to find every entry in file and update with each DFW! */
         if ( data.callback )
         {
            data.callback( data.eventId, SENT_e );
         }
         else
         {
            INFO_printf( "No Callback defined for event %d\n", data.eventId );
         }
#endif
      }
      else
      {
         INFO_printf( "Event %d does not exist anymore on queue: %d", events->eventIds[i].eventId,
                        events->eventIds[i].priority );
      }
   }
   /* Let the caller beware that not all the events were processed. */
   if ( totalCleared < events->nEvents )
   {
      INFO_printf( "WARNING only %d send items of %d were complete\n", totalCleared, events->nEvents );
   }
   /* Make sure data is updated */
   ( void )FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) );

   OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
}

/***********************************************************************************************************************

   Function name: EVL_GetLog

   Purpose: This function is called to return data from the queue fit to a supplied buffer. This function puts in as
            many events as can fit into the supplied buffer.

   Arguments:
         uint32_t size              - The size of the date buffer supplied.
         char *logData              - Buffer where the data is returned.
         uint8_t *alarmId           - The alarmID of the last event.
         EventMarkSent_s *passback  - The passback data used in the EVL_MarkSent function.

   Returns: int32_t                 - Returns the size of the data entered in the buffer.

   Side Effects: None

   Reentrant Code: Yes

   Notes:   This function only returns event data. The event date includes priority, timestamp, event id, number of key
            data pairs and the key data pair information.

 ******************************************************************************************************************** */
int32_t EVL_GetLog( uint32_t size, uint8_t *logData, uint8_t *alarmId, EventMarkSent_s *passback )
{
   uint32_t       totalSize = 0;    /* Number of bytes returned */
   EventQuery_s   query;            /* Query parameter set up with no ranges. */
   uint32_t       sizeLeft = size;  /* Amount of data left in the results buffer. */
   int32_t        returnValue;      /* Status value. */

   uint8_t *pBuf = logData;           /* Preserver the pointer to the data */
   ( void )memset( ( void * )passback, 0, sizeof( EventMarkSent_s ) );
   OS_MUTEX_Lock( &_EVL_MUTEX ); // Function will not return if it fails

   query.qType = NO_QUERY_e;
   query.start_u.eventId = 0;
   query.end_u.eventId = 0;

   /***************************************************************************
      Fill up the buffer with as many events as possible starting with the
      High priority events first and then the normal priority.
   ***************************************************************************/
   returnValue = EventLogLoadBuffer( _EvlMetaData.rtThreshold, pBuf, size, &totalSize, alarmId, query, passback );
   if ( returnValue == EVL_ERROR )
   {
      ERR_printf( "Failure on loading the log buffer" );
      OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
      return returnValue ;
   }
   sizeLeft = size - min( sizeLeft, totalSize );   /* Don't let sizeLeft wrap!   */
   if ( ( sizeLeft != 0 ) && ( totalSize <= size ) )
   {
      returnValue = EventLogLoadBuffer( _EvlMetaData.oThreshold, pBuf + totalSize, sizeLeft,
                                        &totalSize, alarmId, query, passback );
      if ( returnValue == EVL_ERROR )
      {
         ERR_printf( "Failure on loading the log buffer" );
         OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
         return returnValue ;
      }
   }
   /* Make sure data is updated */
   ( void )FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) );
   OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails

   return ( int32_t )totalSize;
}

/***********************************************************************************************************************

   Function name: EVL_LogEvent

   Purpose: This function is called to write an event into the data buffer. For high priority events the date is written
            into the high priority queue and then a call is made to send the data immediately.
            All normal priority events are placed into the queue.

   Arguments:
         EventPriority e_priority                  - Priority 0|NORMAL 1|HIGH
         EventData_s eventData                     - Event Data to write.
         EventKeyValuePair_s *eventKeyValuePairs   - Pointer to the event key pair data.
         bool timeStampProvided                    - True: caller provided event timestamp in eventData
                                                   - False: use current time for event timestamp
         LoggedEventDetails *loggedData            - Provide details (buffer and index id) back to the caller
                                                     about the event that was just logged.  If the caller,
                                                     does not need this information, a NULL can be provided
                                                     for the argument.

   Returns: EventAction_e

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
EventAction_e EVL_LogEvent( uint8_t e_priority, const EventData_s *eventData, const EventKeyValuePair_s *eventKeyValuePairs,
                            bool timeStampProvided, LoggedEventDetails *loggedData )
{
   int32_t                 retVal;     /* Status variable from function calls. */
   EventStoredData_s       sd;         /* Buffer of stored data to be written. */
   buffer_t                *buffer;    /* Buffer to write to flash. */
   uint32_t                offset = 0; /* Offset into the write data buffer. */
   uint8_t                 i;          /* Loop counter. */
   PartitionData_t const   *nvram;     /* Handle to the Ring buffer to write to. */
   RingBufferHead_s        *head;      /* Meta data for the ring buffer. */
   TIMESTAMP_t             DateTime;   /* Event time stamp  */
   bool                    freeBuf = (bool)true;   /* Assume malloc'd memory free'd here. */

   /* Static buffer needed to log/send catastrophic firmware failures */
   static buffer_t         errBuf;
   static uint8_t          errData[sizeof(EventStoredData_s) + ((EVENT_KEY_MAX_LENGTH + EVENT_VALUE_MAX_LENGTH) * 2)];

   /* If the e_priority is invalid drop the event */
   if ( PRIORITY_INVALID( e_priority ) )
   {
      INFO_printf("Event not logged. Priority too low. Event %u, Priority %u, real-time %u, opportunistic %u",
                  eventData->eventId, e_priority, _EvlMetaData.rtThreshold, _EvlMetaData.oThreshold);
      if( NULL != loggedData )
      {
         loggedData->alarmIndex = invalidReadingType;
         loggedData->indexValue = 0;
      }
      return DROPPED_e;
   }

   /* Need to protect against too many NVP pairs being logged and we need to ensure there is space for alarm index
      to be added to every event.  Only allow up to the ( max value - 1 ) to save room the alarm index. */
   if( ( NAME_VALUE_PAIR_MAX - 1 ) < eventData->eventKeyValuePairsCount )
   {  /* Event has too many NVP's, do not continue adding to the event log. */
      INFO_printf( "Logging the event failed. Too many NVP's for the event: %hhu.", eventData->eventKeyValuePairsCount );
      return ERROR_e;
   }

   /* Opportunistic alarms are bubbled up via various resources.  As a result, the size of a single event must
      be limited in order to allow the opportunistic event to be included in these messages. */
   if( ( e_priority < _EvlMetaData.rtThreshold ) && ( e_priority  >= _EvlMetaData.oThreshold ) )
   {  /* This is an opportunistic event, need to check if the size if allowed. */
      /* TODO: This calculation is incomplete, it does not include the two byte key * PairsCount.  Should be something like:
               if( ( sizeof( EventHeadEndData_s ) +
                    ( ( EVENT_KEY_MAX_LENGTH + eventData->eventKeyValueSize ) *
                      ( eventData->eventKeyValuePairsCount + 1 ) ) ) > MAX_ALARM_MEMORY
                 )
       */
      if( ( ( ( eventData->eventKeyValuePairsCount + 1 ) * eventData->eventKeyValueSize )
          + sizeof( EventHeadEndData_s ) ) > MAX_ALARM_MEMORY )
      {  /* The event to be stored is too large, do not continue adding to the event log. */
         INFO_printf( "Logging the event failed.");
         INFO_printf( "Event size too large.  Number of bytes allowed for event data limited to %d bytes.", MAX_ALARM_MEMORY );
         return ERROR_e;
      }
   }

   ( void )memset( ( uint8_t * )&sd, 0, sizeof( sd ) );
   sd.size                     = ( uint16_t )sizeof( EventStoredData_s ) +
                                 ( EVENT_KEY_MAX_LENGTH         * eventData->eventKeyValuePairsCount ) +
                                 ( eventData->eventKeyValueSize * eventData->eventKeyValuePairsCount );
   sd.EventKeyHead_s.sent      = 0;
   sd.EventKeyHead_s.nPairs    = eventData->eventKeyValuePairsCount & ( 0x1F );
   sd.EventKeyHead_s.valueSize = eventData->eventKeyValueSize & ( 0x1F );
   sd.eventId                  = eventData->eventId;

   if ( timeStampProvided )   /* Log the time provided.  */
   {
      sd.timestamp.seconds = eventData->timestamp;
   }
   else  /* Get/log the current time */
   {
      ( void )TIME_UTIL_GetTimeInSecondsFormat( &DateTime );
      sd.timestamp.seconds = DateTime.seconds;
   }

   OS_MUTEX_Lock( &_EVL_MUTEX ); // Function will not return if it fails
   /*****************************************************************************
      First compute and copy all the data to a byte buffer. Then write the
      byte buffer to the appropriate ring buffer.
   ****************************************************************************/
   if ( e_priority >= _EvlMetaData.rtThreshold )
   {  // logging a new real time alarm, increment the index ID to be stored
      sd.alarmId  = _EvlMetaData.HighAlarmSequence + 1; // logging a new real time alarm, increment the index ID to be stored
      sd.priority = e_priority;
      nvram       = _EvlHighFlashHandle;
      head        = &_EvlMetaData.HighBufferHead;
   }
   else
   {  // logging a new opportunistic time alarm, increment the index ID to be stored
      sd.alarmId  = _EvlMetaData.NormalAlarmSequence + 1;
      sd.priority = e_priority;
      nvram       = _EvlNormalFlashHandle;
      head        = &_EvlMetaData.NormalBufferHead;
   }

   if( 0 == sd.alarmId )
   {  // alarm index id zero indicates an empty event log buffer, skip over zero upon alarm id value rollover
      sd.alarmId++;
   }

   if ( eventData->eventId == (uint16_t)comDeviceFirmwareProgramError )
   { //Allocate static buffer for these failures.
      BM_AllocStatic(&errBuf, eSYSFMT_NULL);
      errBuf.data = &errData[0];
      buffer = &errBuf;
   }
   else
   {
      buffer = BM_alloc( sd.size );
   }

   if ( buffer == NULL )
   {
      ERR_printf( "Failed to allocate buffer for alarmID: 0x%x\n", sd.alarmId );
      OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
      return ERROR_e;
   }

   ( void )memcpy( buffer->data, ( uint8_t * )&sd, sizeof( EventStoredData_s ) );
   offset += sizeof( EventStoredData_s );

   for ( i = 0; i < eventData->eventKeyValuePairsCount; i++ )
   {
      ( void )memcpy( buffer->data + offset, eventKeyValuePairs[i].Key, EVENT_KEY_MAX_LENGTH );
      offset += EVENT_KEY_MAX_LENGTH;
      ( void )memcpy( buffer->data + offset, eventKeyValuePairs[i].Value, eventData->eventKeyValueSize );
      offset += eventData->eventKeyValueSize;
   }

   /* Note:  If no periodic "bubble up"tasks which send opportunistic alarms exist for a product,
      during configuration the realTime alarm threshold will be set to equal the opportunistic threshold
      in order to force all events to be bubble up via the bu_am resource. */
   if ( eventData->markSent || ( e_priority  >= _EvlMetaData.rtThreshold ) )
   {  /* This is a high priority event mark the event sent prior to saving it to the event log. Some
         events are already sent to the head end in a reponse message, if the event to be logged was
         already sent, again mark the event sent before storing it. */
      EventStoredData_s   *MarkSent;
      MarkSent = ( EventStoredData_s * )( void * )buffer->data;
      MarkSent->EventKeyHead_s.sent = ( bool )true;
   }

   /* Write to NV */
   retVal = EventLogWrite( nvram, head, buffer->data, sd.size, FALSE );

   if( retVal > 0 )
   {
      if( e_priority >= _EvlMetaData.rtThreshold )
      {
         _EvlMetaData.HighAlarmSequence++;
      }
      else
      {
         _EvlMetaData.NormalAlarmSequence++;
      }
   }

   if( NULL != loggedData )
   {  // if the event log write was successful, the return value will be equal to number bytes attempted to write
      if( retVal == (int32_t)sd.size )
      {  // we logged the event successfuly, acquire return the buffer id and alarm index id of the event
         if(e_priority >= _EvlMetaData.rtThreshold )
         {  // event was logged into the real time alarm buffer
            loggedData->alarmIndex = realTimeAlarmIndexID;
         }
         else
         {  // event was logged into the oppportunistic alarm buffer
            loggedData->alarmIndex = opportunisticAlarmIndexID;
         }
         loggedData->indexValue = sd.alarmId;
      }
      else
      {  // failed to log the event, update the alarm buffer type to invalid to indicate the event was not logged
         loggedData->alarmIndex = invalidReadingType;
         loggedData->indexValue = 0;
      }
   }

   /* Any high priority event must be sent immediately to the headend. */
   if ( e_priority  >= _EvlMetaData.rtThreshold )
   {  /* Has this event been sent already, if so do not post it to the bu_am queue? */
      if( !eventData->markSent )
      {  /* The event has not been sent to the head end yet, Queue the event for transmission
            with time diversity via the bu_am resource.  */
         buffer->eSysFmt = eSYSFMT_EVL_ALARM_MSG;

         OS_MSGQ_Post( &EvlAlarmHandler_MsgQ_, buffer ); // Function will not return if it fails
         freeBuf = ( bool )false;  /* Posted to another queue; will be free'd by receiving task!  */
      }
   }

   if ( freeBuf )
   {
      BM_free( buffer );   /* Not a high priority message, or post failed; free the buffer.   */
   }

   if ( retVal == EVL_ERROR )
   {
      ERR_printf( "Failed to write event HIGH: 0x%x\n", sd.alarmId );
      OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
      return DROPPED_e;
   }

   ( void )FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) );
   OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails

   return QUEUED_e;
}
/***********************************************************************************************************************

   Function name: EVL_GetNormalIndex

   Purpose: Returns "Normal" priority queue index that is current

   Arguments: none

   Returns: uint8_t

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
uint8_t EVL_GetNormalIndex( void )
{
   return _EvlMetaData.NormalAlarmSequence;
}
/***********************************************************************************************************************

   Function name: EVL_GetRealTimeIndex

   Purpose: Returns "High" priority queue index that is current

   Arguments: none

   Returns: uint8_t

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
uint8_t EVL_GetRealTimeIndex( void )
{
   return _EvlMetaData.HighAlarmSequence;
}

/***********************************************************************************************************************

   Function name: EVL_getRealTimeAlarm

   Purpose: Returns true if real time alarms are supported, false otherwise

   Arguments: none

   Returns: uint8_t
edTempHysteresis
   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
uint8_t EVL_getRealTimeAlarm( void )
{
   return (uint8_t)_EvlMetaData.realTimeAlarm;
}

/***********************************************************************************************************************

   Function name: EVL_clearEventLog

   Purpose:  Clears the event contents stored in the device

   Arguments: none

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void EVL_clearEventLog( void )
{
   _EvlMetaData.HighAlarmSequence = 0;
   _EvlMetaData.NormalAlarmSequence = 0;
   _EvlMetaData.HighBufferHead.length = 0;
   _EvlMetaData.HighBufferHead.start = 0;
   _EvlMetaData.NormalBufferHead.length = 0;
   _EvlMetaData.NormalBufferHead.start = 0;

#if ( EP == 1 )
   PWR_lockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
#else
   PWR_lockMutex(); // Function will not return if it fails
#endif

   if( eSUCCESS == FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData,
                               ( lCnt )sizeof( EventLogFile_s ) ) )
   {
      ( void )PAR_partitionFptr.parErase( 0, PART_NV_HIGH_PRTY_ALRM_SIZE, _EvlHighFlashHandle );
      ( void )PAR_partitionFptr.parErase( 0, PART_NV_LOW_PRTY_ALRM_SIZE, _EvlNormalFlashHandle );
   }
   ( void )FIO_fflush( &_EvlFileHandleMeta );
#if ( EP == 1 )
   PWR_unlockMutex( PWR_MUTEX_ONLY ); // Function will not return if it fails
#else
   PWR_unlockMutex(); // Function will not return if it fails
#endif
}


/***********************************************************************************************************************

   Function Name: EVL_OR_PM_Handler

   Purpose: Get/Set decommissionMode

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t EVL_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure
   uint8_t oThreshold;
   uint8_t rtThreshold;
   EVL_GetThresholds( &rtThreshold, &oThreshold );

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case opportunisticThreshold :
         {
            if ( sizeof(_EvlMetaData.oThreshold) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = _EvlMetaData.oThreshold;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(_EvlMetaData.oThreshold);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case realtimeThreshold :
         {
            if ( sizeof(_EvlMetaData.rtThreshold) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = _EvlMetaData.rtThreshold;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(_EvlMetaData.rtThreshold);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case realTimeAlarm :
         {
            if ( sizeof(_EvlMetaData.realTimeAlarm) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *(uint8_t *)value = (uint8_t)_EvlMetaData.realTimeAlarm;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(_EvlMetaData.realTimeAlarm);
                  attr->rValTypecast = (uint8_t)Boolean;
               }
            }
            break;
         }
         case amBuMaxTimeDiversity :
         {
            if ( sizeof(_EvlMetaData.amBuMaxTimeDiversity) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *((uint8_t *)value) = EVL_getAmBuMaxTimeDiversity();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(_EvlMetaData.amBuMaxTimeDiversity);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
          case realTimeAlarmIndexID :
         {
            if ( sizeof(_EvlMetaData.HighAlarmSequence) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *((uint8_t *)value) = EVL_GetRealTimeIndex();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(_EvlMetaData.HighAlarmSequence);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
          case opportunisticAlarmIndexID :
         {
            if ( sizeof(_EvlMetaData.NormalAlarmSequence) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       *((uint8_t *)value) = EVL_GetNormalIndex();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(_EvlMetaData.NormalAlarmSequence);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         default :
         {
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case opportunisticThreshold :
         {
            retVal = EVL_SetThresholds( rtThreshold, (*(uint8_t *)value));
            break;
         }
         case realtimeThreshold :
         {
            retVal = EVL_SetThresholds( (*(uint8_t *)value), oThreshold );
            break;
         }
         case amBuMaxTimeDiversity :
         {
            retVal = EVL_setAmBuMaxTimeDiversity(*(uint8_t *)value);
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return retVal;
} //lint !e715 symbol id not referenced. This function is only called when id is matched

/***********************************************************************************************************************

   Function Name: EVL_FirmwareError

   Purpose: Log a firmware error and reboot

   Arguments:  function function name of the failed call
               file     Filename of the calling function
               line     line number of the calling function

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
void EVL_FirmwareError( char *function, char *file, int line )
{
//   EventKeyValuePair_s  keyVal[2];     /* Event key/value pair info  */
//   EventData_s          progEvent;     /* Event info  */

   DBG_LW_printf("\nERROR: %s call failed %s %d\n", function, file, line);

   // MKD 2019-08-30 13:19 The next section was commented out because the code is recursive
   // Calling EVL_LogEvent calls many OS function that if they fail, will call EVL_FirmwareError
   // Original code left for documentation
#if 0
   ( void )memset( keyVal, 0, sizeof( keyVal ) );

   /* Log the event */
   progEvent.eventId = (uint16_t)comDeviceFirmwareProgramError;
   keyVal[0].Key[ 0 ] = 0; // File name
   (void)strncpy( (char *)keyVal[0].Value, file, EVENT_VALUE_MAX_LENGTH);
   keyVal[1].Key[ 0 ] = 1; // Line number
   (void)memcpy( keyVal[1].Value, &line, sizeof(line) );

   progEvent.eventKeyValuePairsCount = 2;
   progEvent.eventKeyValueSize = ( uint8_t )EVENT_VALUE_MAX_LENGTH;
   progEvent.markSent = (bool)false;

   /* Priority needs to be added to the HEEP header file */
   ( void )EVL_LogEvent( OS_EVENT_ID, &progEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
#endif
   // Wait for printf to print on debug port.
   OS_TASK_Sleep( 5 * ONE_SEC );

   for(;;)
      ; // Force watchdog reset
}

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
/***********************************************************************************************************************

   Function Name: EVL_LGSimStart

   Purpose: Start the Last Gasp Simulation Process

   Arguments:  StartTime     Start of the simulation time in seconds format

   Returns: Success/failure

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
returnStatus_t EVL_LGSimStart( uint32_t simLGStartTime )
{
   returnStatus_t  retVal = eSUCCESS ; // Success/failure

   /* Cancel if any LG Sim is in progress */
   EVL_LGSimCancel();

   if ( eSUCCESS == setSimLGStartAlarm( simLGStartTime ) )
   {
      simLGState_ = eSimLGArmed;
      if ( TIME_SYS_MAX_SEC == simLGStartTime )
      {
         simLGState_ = eSimLGDisabled; // Time not yet valid, keep simLG disabled until valid time AND scheduled
      }
   }
   else
   {
      retVal = eFAILURE;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function Name: EVL_LGSimActive

   Purpose: Returns if the the LG Simulation is active and simulating LG traffic only

   Arguments: None

   Returns: eSUCCESS for Active
            eFAILURE for not Active

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
returnStatus_t EVL_LGSimActive( void )
{
   returnStatus_t           retVal = eFAILURE;

   // Is a simulation active?
   if ( eSimLGOutageDecl <= (simLGState_ ) && (eSimLGTxDone > simLGState_) )
   {
      /* AND simulating Last Gasp Traffic only */
      if( PWRCFG_SIM_LG_TX_ONLY == PWRCFG_get_SimulateLastGaspTraffic() )
      {
         retVal = eSUCCESS;
      }
      else
      {
         retVal = eFAILURE;
      }
   }
   else
   {
      retVal = eFAILURE;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function Name: EVL_GetLGSimState

   Purpose: Returns the LG Simulation state

   Arguments: None

   Returns: eSUCCESS for Active
            eFAILURE for not Active

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
eSimLGState_t EVL_GetLGSimState( void )
{
   return simLGState_;
}

/***********************************************************************************************************************

   Function Name: EVL_LGSimAllowMsg

   Purpose: Returns if passed message can be sent through by the MAC when the LG Sim is active

   Arguments: APP_MSG_Tx_t * - Contains information regarding the QOS and Message.
              buffer_t * - Payload and the length of the payload

   Returns: True if the message can be allowed by the HEEP_MSG_Tx..() functions
            Flase if the message has to be dropped by the HEEP_MSG_Tx..() functions

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
returnStatus_t EVL_LGSimAllowMsg( HEEP_APPHDR_t const *heepMsgInfo, buffer_t const *payloadBuf )
{
   returnStatus_t           retVal = eFAILURE;

   if ( eSUCCESS == EVL_LGSimActive() )
   {
      /* If the message is the PowerFail event, return true */
      if (  (heepMsgInfo->TransactionType == (uint8_t)TT_ASYNC)
          &&(heepMsgInfo->Method_Status == (uint8_t)method_post)
          &&(heepMsgInfo->Resource == (uint8_t)bu_am) )
      {
         retVal = eSUCCESS;
      }
      else
      {
         /* Free the message buffer if the message is not allowed */
         if ( NULL  != payloadBuf )
         {
            BM_free( ( buffer_t * )payloadBuf );
         }
         retVal = eFAILURE;;
      }
   }
   else
   {
      retVal = eSUCCESS;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function Name: EVL_LGSimCancel

   Purpose: Cancels the Simulation and destroys the created resources

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
void EVL_LGSimCancel( void )
{
   /* If Sim last Gasp is in progress, then it is cancelled, and all
   modified parameters/variables shall be returned to normal oeperation */
   if ( simLGState_ > eSimLGDisabled )
   {
      INFO_printf( "The simulation is active and shall be cancelled" );

      /* Delete the Sim LG Start Alarm*/
      if ( NO_ALARM_FOUND != schSimLGStartAlarmId_ )
      {
         if ( eSUCCESS == TIME_SYS_DeleteAlarm( schSimLGStartAlarmId_ ) )
         {
            schSimLGStartAlarmId_ = NO_ALARM_FOUND;
         }
         else
         {
            INFO_printf("schSimLGStartAlarmId_ delete failed"); // Debugging purposes
         }
      }

      /* Delete the Sim LG Duration Alarm */
      if ( NO_ALARM_FOUND != schSimLGDurationAlarmId_ )
      {
         if ( eSUCCESS == TIME_SYS_DeleteAlarm(schSimLGDurationAlarmId_) )
         {
            schSimLGDurationAlarmId_ = NO_ALARM_FOUND;
         }
         else
         {
            INFO_printf("schSimLGDurationAlarmId_ delete failed"); // Debugging purposes
         }
      }

      /* Delete the Sim LG Sleep Timer Alarm */
      if ( NO_ALARM_FOUND != schSimLGSleepTimerAlarmId_ )
      {
         if ( eSUCCESS == TIME_SYS_DeleteAlarm(schSimLGSleepTimerAlarmId_) )
         {
            schSimLGSleepTimerAlarmId_ = NO_ALARM_FOUND;
         }
         else
         {
            INFO_printf( "Cancel Scheduled DMD Reset Failed" ); // Debugging purposes
         }
      }

      /* Delete the Power On State timer */
      if (0 != powerOnStateTimerId_)
      {
         (void)TMR_DeleteTimer(powerOnStateTimerId_);
         powerOnStateTimerId_ = 0;
      }

      /* If MAC Configuration saved during the initialization phase */
      if ( simLGMACConfigSaved_ )
      {
         /* Restore the MAC parameters which is backed up during the start of the Simulation */
         MAC_RestoreConfig_From_BkupRAM();
         simLGMACConfigSaved_ = false;

         /* Rest MAC parameters that were set during the Tx*/
         resetSimLGCsmaParameters();
      }
      /* Reset the last gasp flags */
      PWRLG_RestLastGaspFlags();

      /* Reset the Tx Attempts */
      VBATREG_CURR_TX_ATTEMPT = 0;

      /* Set the state to the default */
      simLGState_ = eSimLGDisabled;
   }
   else
   {
      INFO_printf( "Simulation is not active" );
      /* Delete the Sim LG Start Alarm if active */
      if ( NO_ALARM_FOUND != schSimLGStartAlarmId_ )
      {
         if ( eSUCCESS == TIME_SYS_DeleteAlarm( schSimLGStartAlarmId_ ) )
         {
            schSimLGStartAlarmId_ = NO_ALARM_FOUND;
         }
         else
         {
            INFO_printf("schSimLGStartAlarmId_ delete failed"); // Debugging purposes
         }
      }
   }
}
/***********************************************************************************************************************

   Function name: EVL_LGSimTxDoneSignal

   Purpose: Posts the LG Sim tx message done semaphore.

   Arguments: None

   Returns: None

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void EVL_LGSimTxDoneSignal( bool success )
{
   simLGTxSuccessful_ = success;
   OS_SEM_Post( &SimLGTxDoneSem ); // Function will not return if it fails
}

#endif

/***********************************************************************************************************************

   Function Name: OR_AM_MsgHandler

   Purpose: Handle OR_AM ( alarm query by date range, with optional event IDs )

   Arguments:  APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
               void *payloadBuf           - pointer to data payload, which is a getAlarms_t struct

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void OR_AM_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length )  /*lint !e818 heepReqHdr could be ptr to const   */
{
   /*Prepare Compact Meter Read structure */
   HEEP_APPHDR_t        heepRespHdr;            /* Application header/QOS info   */
   sysTimeCombined_t    currentTime;            /* Get current time  */
   EventQuery_s         query;
   pack_t               packCfg;                /* Struct needed for PACK functions */
   buffer_t             *pBuf;                  /* pointer to message   */
   buffer_t             *eventData;             /* Holds events returned by query.  */
   pTimeRange_t         pTimeRange;             /* pointer to time ranges in request.  */
   pGetAlarms_t         getAlarms;              /* Pointer to request from HE */
   uint8_t              *alarms;
   uint8_t              alarmID;                /* Last alarm index returned by query. */
   uint32_t             eventSize;              /* Number of bytes in query data returned.   */
   uint32_t             msgTime;                /* Current time reported to HE */
   uint16_t             bits = 0;               /* Keeps track of the bit position within the Bitfields  */
   uint8_t              numTimeRanges;          /* Number of time ranges requested. */
   uint8_t              timeNum;                /* time range loop counter */
   TimeSchRdgQty_t      qtys;                   /* For Time Qty and Event Reading Qty  */
   EventRdgQty_u        eventQtys;              /* For Event Reading Qty  */
   uint16_t             eventQtyLocation;       /* Track additional eventQtys locations in compact read bit struct */
   int32_t              evlQueryResult = 0;
   (void)length;

   /*Application header/QOS info */
   HEEP_initHeader( &heepRespHdr );
   heepRespHdr.TransactionType = ( uint8_t )TT_RESPONSE;
   heepRespHdr.Resource = ( uint8_t )or_am;
   heepRespHdr.Method_Status = ( uint8_t )OK;
   heepRespHdr.Req_Resp_ID = ( uint8_t )heepReqHdr->Req_Resp_ID;
   heepRespHdr.qos = ( uint8_t )heepReqHdr->qos;
   heepRespHdr.callback = NULL;
   heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;


   if ( method_get == ( enum_MessageMethod )heepReqHdr->Method_Status )
   {
      /*Process the get request  */

      query.qType = QUERY_BY_DATE_e;

      /*Allocate a big buffer to accomadate worst case payload.   */
      pBuf = BM_alloc( APP_MSG_MAX_DATA );
      if ( pBuf != NULL )
      {
         getAlarms = ( pGetAlarms_t )payloadBuf;
         eventData = BM_alloc( EVENT_RECOVERY_BUFF_SIZE );
         if ( eventData != NULL )
         {
            qtys.TimeSchRdgQty = UNPACK_bits2_uint8( payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS ); /*Parse the fields passed in   */
            bits  += sizeof( qtys ) * 8;   /* Skip the time qty pairs */

            PACK_init( ( uint8_t* )&pBuf->data[0], &packCfg );
            /*Add current timestamp */
            ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );
            msgTime = ( uint32_t )( currentTime / TIME_TICKS_PER_SEC );
            PACK_Buffer( -( VALUES_INTERVAL_END_SIZE * 8 ), ( uint8_t * )&msgTime, &packCfg );
            pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;
            eventQtyLocation = pBuf->x.dataLen;

            /*Insert the reading quantity, it will be adjusted later to the correct value */
            eventQtys.EventRdgQty = 0;
            PACK_Buffer( sizeof( eventQtys.EventRdgQty ) * 8, &eventQtys.EventRdgQty, &packCfg );
            pBuf->x.dataLen += sizeof( eventQtys.EventRdgQty );
#if 0
            if ( qtys.bits.rdgQty != 0 )
            {
               /* Event ID list passed in the request   */
               numEvents = qtys.bits.rdgQty;
            }
#endif

            if ( qtys.bits.timeSchQty != 0 )
            {
               /* Time range list passed in the request   */
               numTimeRanges = qtys.bits.timeSchQty;

               /* Loop through the number of time ranges requested.  */
               pTimeRange = &getAlarms->time;

               for ( timeNum = 0; timeNum < numTimeRanges; timeNum++ )
               {
                  alarms = eventData->data;  // reasign pointer to beginning of data buffer for next time schedule
                  query.start_u.timestamp = pTimeRange->startTime;
                  Byte_Swap( (uint8_t *)&query.start_u.timestamp, sizeof(query.start_u.timestamp) );
                  query.end_u.timestamp   = pTimeRange->endTime;
                  Byte_Swap( (uint8_t *)&query.end_u.timestamp, sizeof(query.end_u.timestamp) );

                  evlQueryResult = EVL_QueryBy( query, eventData->data, eventData->x.dataLen, &eventSize, &alarmID );
                  if ( EVL_ERROR != evlQueryResult )
                  {

                     while ( eventSize )
                     {
                        /* Copy alarm info from alarms structure to inbound packet. */

                        EventHeadEndData_s   *alarmInfo;    /* Pointer to structure returned by query */
                        uint16_t             param;         /* Used to pack 16 bit quantities into response buffer   */
                        uint16_t             bytesAdded;    /* Number of bytes added to the message.  */
                        uint8_t              *pair;         /* Pointer to event details name/value    */
                        uint8_t              byteVal;       /* Needed for bit field packing  */

                        if( READING_QTY_MAX > eventQtys.bits.eventQty )
                        {
                           /* Copy alarm info from alarms structure to inbound packet. */

                           alarmInfo = ( EventHeadEndData_s * )( void * )alarms;

                           /* Time event was logged is sent to HE */
                           msgTime = alarmInfo->timestamp;
                           PACK_Buffer(-(int16_t)sizeof( msgTime ) * 8, ( uint8_t * )&msgTime, &packCfg);
                           bytesAdded = sizeof( msgTime );

                           param = alarmInfo->eventId;   /* Event type next   */
                           PACK_Buffer(-(int16_t)sizeof( param ) * 8, ( uint8_t * )&param, &packCfg);
                           bytesAdded += sizeof( param );

                           byteVal = alarmInfo->EventKeyHead_s.nPairs;
                           PACK_Buffer( 4, &byteVal, &packCfg );
                           byteVal = alarmInfo->EventKeyHead_s.valueSize;
                           PACK_Buffer( 4, &byteVal, &packCfg );
                           bytesAdded += sizeof( byteVal );

                           pair = ( uint8_t * )alarmInfo + sizeof( *alarmInfo ); /* Event details follow structure   */
                           for ( uint8_t pairCount = 0; pairCount < alarmInfo->EventKeyHead_s.nPairs; pairCount++ )
                           {
                              /* EndDeviceEvents.EndDeviceEventDetails.name - 2 bytes  */
                              PACK_Buffer(-(int16_t)sizeof( uint16_t ) * 8, pair, &packCfg);
                              pair += sizeof( uint16_t );  /* Advance by key EndDeviceEventDetails.name size - 2 bytes */
                              bytesAdded += sizeof( uint16_t );

                              /* EndDeviceEvents.EndDeviceEventDetails.value - EndDevEDValueSizeBytes    */
                              PACK_Buffer(-( int8_t )alarmInfo->EventKeyHead_s.valueSize * 8, pair, &packCfg);
                              pair += alarmInfo->EventKeyHead_s.valueSize;  /* Advance by key Value size */
                              bytesAdded += alarmInfo->EventKeyHead_s.valueSize;
                           }

                           pBuf->x.dataLen += bytesAdded;
                           eventSize -= bytesAdded;
                           alarms += bytesAdded;
                           eventQtys.bits.eventQty++;
                        }
                        else
                        {  // we have reached the limit of 15 events, append an additional compact read for the next 15

                           // update the previous reading quantity from previous compact read bit structure
                           pBuf->data[eventQtyLocation] = eventQtys.EventRdgQty;

                           // Add current timestamp to the response message
                           PACK_Buffer( ( VALUES_INTERVAL_END_SIZE * 8 ), pBuf->data, &packCfg );
                           pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
                           eventQtyLocation = pBuf->x.dataLen;

                           // Insert the reading quantity, it will be adjusted later to the correct value
                           eventQtys.EventRdgQty = 0;
                           PACK_Buffer( sizeof( eventQtys.EventRdgQty ) * 8, &eventQtys.EventRdgQty, &packCfg );
                           pBuf->x.dataLen += sizeof( eventQtys.EventRdgQty );
                        }
                     }  // end of while (eventSize)

                  }
                  pTimeRange++;
               } /*end for */
            }
            else  /* Request for NO ranges not supported - bad request.   */
            {
               qtys.TimeSchRdgQty = 0;
               heepRespHdr.Method_Status = ( uint8_t )BadRequest;
            }

            /*Insert the correct number of readings   */
            //pBuf->data[VALUES_INTERVAL_END_SIZE] = eventQtys.EventRdgQty;
            pBuf->data[eventQtyLocation] = eventQtys.EventRdgQty;

            if ( EVL_SIZE_LIMIT_REACHED == evlQueryResult )
            {
               heepRespHdr.Status = (uint8_t)PartialContent;
            }

            /* send the message to message handler. The called function will free the buffer */
            ( void )HEEP_MSG_Tx ( &heepRespHdr, pBuf );

            BM_free( eventData );
         }
         else
         {  // failed to allocate buffer for alarm info
            heepRespHdr.Method_Status = (uint8_t)ServiceUnavailable;
            (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
            ERR_printf("ERROR: EVL No buffer available");
         }
      }
      else
      {  // failed to allocate buffer for response message to head end
         heepRespHdr.Method_Status = (uint8_t)ServiceUnavailable;
         (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
         ERR_printf("ERROR: EVL No buffer available");
      }
   }
   else
   {  //Wrong method, return header only with status to indicate the request was not perofrmed
      heepRespHdr.Method_Status = (uint8_t)BadRequest;
      (void)HEEP_MSG_TxHeepHdrOnly(&heepRespHdr);
      ERR_printf("ERROR: EVL Bad Request");
   }

}//lint !e438 !e818
/***********************************************************************************************************************

   Function Name: OR_HA_MsgHandler

   Purpose: Handle OR_HA ( alarm query by index range )

   Arguments:  APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
               void *payloadBuf           - pointer to data payload

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void OR_HA_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length )  /*lint !e818 heepReqHdr could be ptr to const   */
{
   (void)heepReqHdr;
   (void)payloadBuf;

   HEEP_APPHDR_t            heepRespHdr;  // Application header/QOS info
   buffer_t                       *pBuf;  // pointer to message
   buffer_t                  *eventData;  // Holds events returned by query.
   pack_t                       packCfg;  // Struct needed for PACK functions
   GetLogEntries_t    *histAlarmRequest;  // pointer to data payload supplied in request
   EventQuery_s                   query;  // type of query of the event log
   uint32_t                   eventSize;  // Number of bytes in query data returned
   uint8_t                      *alarms;  // pointer to data containing alarm information
   uint8_t                      alarmID;  // Last alarm index returned by query
   sysTimeCombined_t        currentTime;  // Get current time
   uint32_t                     msgTime;  // Current time reported to HE
   EventRdgQty_u                   qtys;  // For Event Reading Qty
   uint16_t            eventQtyLocation;  // Track additional eventQtys locations in compact read bit struct
   int32_t               evlQueryResult;
   (void)length;

   // Application header/QOS info to be returned in response
   HEEP_initHeader( &heepRespHdr );
   heepRespHdr.TransactionType = ( uint8_t )TT_RESPONSE;
   heepRespHdr.Resource = ( uint8_t )or_ha;
   heepRespHdr.Method_Status = ( uint8_t )OK;
   heepRespHdr.Req_Resp_ID = ( uint8_t )heepReqHdr->Req_Resp_ID;
   heepRespHdr.qos = ( uint8_t )heepReqHdr->qos;
   heepRespHdr.callback = NULL;
   heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;


   if ( method_get == ( enum_MessageMethod )heepReqHdr->Method_Status )
   {  //Process the get request

      query.qType = QUERY_BY_INDEX_e; // set the event log query to be via the alarm index value
      histAlarmRequest = ( GetLogEntries_t *)payloadBuf;
      query.alarmSeverity = histAlarmRequest->severity;

      if( MEDIUM_PRIORITY_SEVERITY == query.alarmSeverity ||
          HIGH_PRIORITY_SEVERITY == query.alarmSeverity )
      {
         // Allocate a big buffer to accomadate worst case payload.
         pBuf = BM_alloc( APP_MSG_MAX_DATA );

         if ( pBuf != NULL )
         {   // we got the buffer

            /* The response to the get request */
            eventData = BM_alloc( EVENT_RECOVERY_BUFF_SIZE );
            if ( eventData != NULL )
            {  // we got the buffer to store alarm data

               // process the request payload and acquire the start/end index values
               query.start_u.eventId = histAlarmRequest->startIndexId;
               query.end_u.eventId = histAlarmRequest->endIndexId;

               // set pointer to location where alarm info will be stored after processing the request
               alarms = eventData->data;

               // initialize the pack_t data struct
               PACK_init( ( uint8_t* )&pBuf->data[0], &packCfg );

               // Add current timestamp to the response message
               ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );
               msgTime = ( uint32_t )( currentTime / TIME_TICKS_PER_SEC );
               PACK_Buffer( -( VALUES_INTERVAL_END_SIZE * 8 ), ( uint8_t * )&msgTime, &packCfg );
               pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;
               eventQtyLocation = pBuf->x.dataLen;

               // Insert the reading quantity, it will be adjusted later to the correct value
               qtys.EventRdgQty = 0;
               PACK_Buffer( sizeof( qtys.EventRdgQty ) * 8, &qtys.EventRdgQty, &packCfg );
               pBuf->x.dataLen += sizeof( qtys.EventRdgQty );

               // initiate the query of the event log
               evlQueryResult = EVL_QueryBy( query, eventData->data, eventData->x.dataLen, &eventSize, &alarmID );
               if ( EVL_ERROR != evlQueryResult )
               {  // query was successful

                  while ( eventSize )
                  {  // while there is data left to process in the data buffer, pack each alarm into reponse buffer

                     EventHeadEndData_s   *alarmInfo;    // Pointer to structure returned by query
                     uint16_t             param;         // Used to pack 16 bit quantities into response buffer
                     uint16_t             bytesAdded;    // Number of bytes added to the message
                     uint8_t              *pair;         // Pointer to event details name/value
                     uint8_t              byteVal;       // Needed for bit field packing

                     if( READING_QTY_MAX > qtys.bits.eventQty )
                     {  // we still have more room to pack additional events into compact read

                        alarmInfo = ( EventHeadEndData_s * )( void * )alarms;

                        // pack the time stamp of the event
                        msgTime = alarmInfo->timestamp;
                        PACK_Buffer( -( int8_t )sizeof( msgTime ) * 8, ( uint8_t * )&msgTime, &packCfg );
                        bytesAdded = sizeof( msgTime );

                        // pack the event Id value
                        param = alarmInfo->eventId;
                        PACK_Buffer( -( int8_t )sizeof( param ) * 8, ( uint8_t * )&param, &packCfg );
                        bytesAdded += sizeof( param );

                        // pack the name/value pair information
                        byteVal = alarmInfo->EventKeyHead_s.nPairs;
                        PACK_Buffer( NVP_QTY_SIZE_BITS, &byteVal, &packCfg );
                        byteVal = alarmInfo->EventKeyHead_s.valueSize;
                        PACK_Buffer( EVENT_DETAIL_VALUE_SIZE_BITS, &byteVal, &packCfg );
                        bytesAdded += sizeof( byteVal );

                        // set pointer to location of  nvp data
                        pair = ( uint8_t * )alarmInfo + sizeof( *alarmInfo );

                        // If we have any nvp's, go ahead and pack them
                        for ( uint8_t pairCount = 0; pairCount < alarmInfo->EventKeyHead_s.nPairs; pairCount++ )
                        {
                           // EndDeviceEvents.EndDeviceEventDetails.name - 2 bytes
                           PACK_Buffer( -( int8_t )sizeof( uint16_t ) * 8, pair, &packCfg );
                           pair += sizeof( uint16_t );  // Advance by key EndDeviceEventDetails.name size - 2 bytes
                           bytesAdded += sizeof( uint16_t );

                           // EndDeviceEvents.EndDeviceEventDetails.value - EndDevEDValueSizeBytes
                           PACK_Buffer( -( int8_t )alarmInfo->EventKeyHead_s.valueSize * 8, pair, &packCfg );
                           pair += alarmInfo->EventKeyHead_s.valueSize;  // Advance by key Value size
                           bytesAdded += alarmInfo->EventKeyHead_s.valueSize;
                        }

                        pBuf->x.dataLen += bytesAdded; // update number of bytes placed into response buffer
                        eventSize -= bytesAdded;       // decrement the number of bytes process from alarmd data buffer
                        alarms += bytesAdded;          // move alarm pointer to the next alarm to process
                        qtys.bits.eventQty++;          // increment the event quantity
                     }
                     else
                     { // we cannot fit anymore events into the bitstructure, append an additional compact read

                        // update the previous reading quantity from previous compact read bit structure
                        pBuf->data[eventQtyLocation] = qtys.EventRdgQty;

                        // Add current timestamp to the response message
                        PACK_Buffer( ( VALUES_INTERVAL_END_SIZE * 8 ), pBuf->data, &packCfg );
                        pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
                        eventQtyLocation = pBuf->x.dataLen;

                        // Insert the reading quantity, it will be adjusted later to the correct value
                        qtys.EventRdgQty = 0;
                        PACK_Buffer( sizeof( qtys.EventRdgQty ) * 8, &qtys.EventRdgQty, &packCfg );
                        pBuf->x.dataLen += sizeof( qtys.EventRdgQty );
                     }

                  } // end of while( eventSize )

               } // end of if( EVL_SUCCESS == EVL_QueryBy() )

               // Insert the correct number of readings into response message
               pBuf->data[eventQtyLocation] = qtys.EventRdgQty;

               if ( EVL_SIZE_LIMIT_REACHED == evlQueryResult )
               {  // unable to fit all event data into buffer, report back to head end this is a partial data set
                  heepRespHdr.Status = (uint8_t)PartialContent;
               }

               // send the message to message handler. The called function will free the buffer.
               ( void )HEEP_MSG_Tx ( &heepRespHdr, pBuf );

               // free the buffer allocated to gather the alarm data
               BM_free( eventData );
            }
            else
            {  /* failed to allocate buffer to gather alarm data, free the message buffer since there is
                  no data to pack into the response.  The reponse will just be the application header. */
               BM_free( pBuf );
               heepRespHdr.Method_Status = (uint8_t)ServiceUnavailable;
               (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
               ERR_printf("ERROR: EVL No buffer available");
            }
         }
         else
         {  // failed to allocate buffer for response message to head end
            heepRespHdr.Method_Status = (uint8_t)ServiceUnavailable;
            (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
            ERR_printf("ERROR: EVL No buffer available");
         }
      }
      else
      {  // invalid serverity paramater provided in get request
         heepRespHdr.Method_Status = (uint8_t)BadRequest;
         (void)HEEP_MSG_TxHeepHdrOnly(&heepRespHdr);
         ERR_printf("ERROR: EVL Bad Request - invalid paramater utilized in the request");
      }
   }
   else
   {  //Wrong method, return header only with status to indicate the request was not performed
      heepRespHdr.Method_Status = (uint8_t)BadRequest;
      (void)HEEP_MSG_TxHeepHdrOnly(&heepRespHdr);
      ERR_printf("ERROR: EVL Bad Request");
   }
}

#ifdef EVL_TESTING
/***********************************************************************************************************************

   Function name: EventLogDumpOneBuffer

   Purpose: This function is called to loop through all entries of the ringbuffer and print them to the screen.

   Arguments:
         PartitionData_t const *nvram  - Handle to the nvram of the ring buffer.
         RingBufferHead_s *rHead       - Ring buffer meta date.

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static void EventLogDumpOneBuffer( PartitionData_t const *nvram, RingBufferHead_s *rHead )
{
   EventStoredData_s sdata;                        /* Current event, used to extract data about current event. */
   uint32_t          availableData;                /* Total data available in the buffer. */
   buffer_t          *data;
   uint8_t           *dataCopy;
   uint32_t          saveHead;
   uint32_t          saveLength;
   uint32_t          eventCount = 0;               /* Number of events found in buffer   */

   OS_MUTEX_Lock( &_EVL_MUTEX ); // Function will not return if it fails

   saveHead      = rHead->start;
   saveLength    = rHead->length;
   availableData = RINGBUFFER_AVAILABLE_DATA( rHead );

   if ( availableData != 0 )
   {
      INFO_printf( "" );
      while ( availableData != 0 )
      {
         eventCount++;
         INFO_printf( "Ring Start      : %d", rHead->start);
         INFO_printf( "Ring Length     : %d", rHead->length );
         /* Read next event.  *//*lint -e{644} sdata filled by EventLogRead   */
         if ( EventLogRead( nvram, rHead, ( uint8_t * )&sdata, sizeof( sdata ), FALSE ) >= ( int32_t )sizeof( sdata ) )
         {
            availableData -= sdata.size;
            data = BM_alloc( sdata.size );
            if ( data != NULL )  /* Make sure we got the requested memory! */
            {
               dataCopy = data->data;
               ( void )EventLogRead( nvram, rHead, dataCopy , sdata.size, TRUE );
               DumpEventData( data->data, sdata.size );
               BM_free( data );
            }
            else
            {
               break;
            }
         }
         else
         {
            break;
         }
      }
      INFO_printf( "Events found: %d\n\n", eventCount );
   }
   else
   {
      INFO_printf( "No data in buffer\n" );
   }
   rHead->start = saveHead;
   rHead->length = saveLength;

   OS_MUTEX_Unlock( &_EVL_MUTEX ); // Function will not return if it fails
}
/***********************************************************************************************************************

   Function name: EventLogDumpBuffers

   Purpose: This function is called to print out both ring buffers.

   Arguments:

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void EventLogDumpBuffers( void )
{
   INFO_printf( "Priority buffer dump:" );
   EventLogDumpOneBuffer( _EvlHighFlashHandle, &_EvlMetaData.HighBufferHead );

   INFO_printf( "Normal buffer dump:" );
   EventLogDumpOneBuffer( _EvlNormalFlashHandle, &_EvlMetaData.NormalBufferHead );


   INFO_printf( "HiAlarmSeq      : %d", EVL_GetRealTimeIndex() );
   INFO_printf( "NormAlarmSeq    : %d", EVL_GetNormalIndex() );
   INFO_printf( "rtThreshold     : %d", _EvlMetaData.rtThreshold);
   INFO_printf( "oThreshold      : %d", _EvlMetaData.oThreshold);
   INFO_printf( "Initialized     : %d", (int32_t)_EvlMetaData.Initialized);
   INFO_printf( "realTimeAlarm   : %d", (int32_t)_EvlMetaData.realTimeAlarm);
   INFO_printf( "amBuMax TimeDiv : %u", _EvlMetaData.amBuMaxTimeDiversity );
}
/***********************************************************************************************************************

   Function name: EventLogDumpBuffers

   Purpose: This function is called to print out both ring buffers.

   Arguments:

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void EventLogCallbackTest( uint16_t eventReference, EventAction_e eventAction )
{
   INFO_printf( "EventLogCallbackTest callback complete with event id 0x%x and action %d\n",
                  eventReference, eventAction );   /*lint !e641 implicit conversion of enum to integral type   */
}
/***********************************************************************************************************************

   Function name: DumpEventData

   Purpose: This function is called to print event data.

   Arguments:

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void DumpEventData( uint8_t const * const buffer, uint32_t numBytes )
{
   EventStoredData_s    eData;      /* Event data used to print. */
   EventKeyValuePair_s  ePair;      /* Key event data to print. */
   uint16_t             i;          /* Loop counter   */
   uint16_t             j;          /* Loop counter   */
   uint32_t             offset = 0; /* Offset into the log buffer. */
   uint16_t             keySize;
   uint32_t             bytesLeft = numBytes;
   uint8_t              keyOut[EVENT_VALUE_MAX_LENGTH * 5 + 1];
   uint8_t              value[5];
   sysTime_dateFormat_t sysTime;
   sysTime_t            sTime;

   ( void )memset( keyOut, 0, sizeof( keyOut ) );
   ( void )memset( value, 0, sizeof( value ) );
   ( void )memset( &sTime, 0, sizeof( sTime ) );

   if ( buffer != NULL) /* No need to process a null buffer */
   {
      while ( bytesLeft != 0 )
      {
         ( void )memcpy( ( char * ) &eData, ( char * )( buffer + offset ), sizeof( EventStoredData_s ) );
         TIME_UTIL_ConvertSecondsToSysFormat( eData.timestamp.seconds, 0, &sTime );
         ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );

         bytesLeft -= min( ( uint32_t )sizeof( EventStoredData_s ), bytesLeft );

         INFO_printf( "Size            : %u", eData.size );
         INFO_printf( "Timestamp(sec)  : %02d/%02d/%04d %02d:%02d:%02d (%d)",
                        sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec,
                        eData.timestamp.seconds );
         INFO_printf( "Alarm ID        : 0x%x", eData.alarmId );
         INFO_printf( "Priority        : %d", eData.priority );
         INFO_printf( "Sent            : %s", ( eData.EventKeyHead_s.sent ) ? "SENT" : "NOT SENT" );
         INFO_printf( "Event ID        : %d", eData.eventId );
         INFO_printf( "Num of Pairs    : %d", eData.EventKeyHead_s.nPairs );
         INFO_printf( "Value Size      : %d\n", eData.EventKeyHead_s.valueSize );

         offset += ( uint32_t )sizeof( EventStoredData_s );

         keySize = EVENT_KEY_MAX_LENGTH + eData.EventKeyHead_s.valueSize;
         for ( i = 0; i < eData.EventKeyHead_s.nPairs; i++ )
         {
            ( void )memcpy( ( char* ) &ePair, ( char * )( buffer + offset ), keySize );
            Byte_Swap( ePair.Value, eData.EventKeyHead_s.valueSize );
            INFO_printf( "Key: %d ", *( uint16_t * )&ePair.Key[0] ); /*lint !e2445 !e826 cast increases alignment requirement, area too small */
            for ( j = 0; j < eData.EventKeyHead_s.valueSize; j++ )
            {
               ( void )sprintf( ( char * )keyOut + ( j * 5 ), "0x%02hhx ", ePair.Value[j] );
            }
            INFO_printf( "Value: %s", keyOut );
            ( void )memset( keyOut, 0, sizeof( keyOut ) );

            offset += keySize;
            bytesLeft -= min( keySize, bytesLeft );
         }
         OS_TASK_Sleep( 100 );
      }
   }
}
/***********************************************************************************************************************

   Function name: DumpHeadEndData

   Purpose: This function is called to print headend data.

   Arguments:

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void DumpHeadEndData( uint8_t const * const buffer, uint32_t numBytes )
{
   EventHeadEndData_s   eData;      /* Event data used to print. */
   EventKeyValuePair_s  ePair;      /* Key event data to print. */
   uint16_t             i;          /* Loop counter. */
   uint16_t             j;          /* Loop counter. */
   uint32_t             offset = 0; /* Offset into the log buffer. */
   uint16_t             keySize;
   uint32_t             bytesLeft = numBytes;
   sysTime_dateFormat_t sysTime;
   sysTime_t            sTime;
   uint8_t              keyOut[EVENT_VALUE_MAX_LENGTH * 5 + 1];
   uint8_t              value[5];

   ( void )memset( keyOut, 0, sizeof( keyOut ) );
   ( void )memset( value,  0, sizeof( value ) );
   ( void )memset( &sTime, 0, sizeof( sTime ) );

   if ( buffer != NULL) /* No need to process a null buffer */
   {
      while ( bytesLeft != 0 )
      {
         ( void )memcpy( ( char * ) &eData, ( char * )( buffer + offset ), sizeof( EventHeadEndData_s ) );
         TIME_UTIL_ConvertSecondsToSysFormat( eData.timestamp, 0, &sTime );
         ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
         bytesLeft -= ( uint32_t )sizeof( EventHeadEndData_s );

         INFO_printf( "Event ID        : %d", eData.eventId );
         INFO_printf( "Timestamp(sec)  : %02d/%02d/%04d %02d:%02d:%02d (%d)",
                        sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec,
                        eData.timestamp );
         INFO_printf( "Num of Pairs    : %d", eData.EventKeyHead_s.nPairs );
         INFO_printf( "Value Size      : %d\n", eData.EventKeyHead_s.valueSize );

         offset += ( uint32_t )sizeof( EventHeadEndData_s );

         keySize = EVENT_KEY_MAX_LENGTH + eData.EventKeyHead_s.valueSize;
         for ( i = 0; i < eData.EventKeyHead_s.nPairs; i++ )
         {
            ( void )memcpy( ( char* ) &ePair, ( char * )( buffer + offset ), keySize );
            Byte_Swap( ePair.Value, eData.EventKeyHead_s.valueSize );
            INFO_printf( "Key: %d ", *( uint16_t * )&ePair.Key[0] ); /*lint !e2445 !e826 cast increases alignment requirement, area too small */
            for ( j = 0; j < eData.EventKeyHead_s.valueSize; j++ )
            {
               ( void )sprintf( ( char * )keyOut + ( j * 5 ), "0x%02hhx ", ePair.Value[j] );
            }
            INFO_printf( "Value: %s", keyOut );
            ( void )memset( keyOut, 0, sizeof( keyOut ) );

            offset += keySize;
            bytesLeft -= keySize;
         }
         OS_TASK_Sleep( 100 );
      }
   }
}
/***********************************************************************************************************************

   Function name: EventLogEraseFlash

   Purpose: This function is called to erase the NVRAM data

   Arguments:
         uint8_t whichBuffer  - 0-normal, 1-high

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void EventLogEraseFlash( uint8_t whichBuffer )
{
   returnStatus_t retVal;

   if ( whichBuffer )
   {
      retVal = PAR_partitionFptr.parErase( 0, PART_NV_HIGH_PRTY_ALRM_SIZE, _EvlHighFlashHandle );
   }
   else
   {
      retVal = PAR_partitionFptr.parErase( 0, PART_NV_LOW_PRTY_ALRM_SIZE, _EvlNormalFlashHandle );
   }
   if ( retVal != eSUCCESS )
   {
      INFO_printf( "Failed to erase flash" );
   }
}
/***********************************************************************************************************************

   Function name: EventLogDeleteMetaData

   Purpose: This function is called to delete all logged entries.

   Arguments:

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void EventLogDeleteMetaData( void )
{
   uint8_t rtThreshold;
   uint8_t oThreshold;

   EVL_GetThresholds( &rtThreshold, &oThreshold );       /* Retrieve current thresholds.     */
   ( void )EventInitializeMetaData();                    /* Initialize the log.              */
   ( void )EVL_SetThresholds( rtThreshold, oThreshold ); /* Restore the original thresholds. */

   INFO_printf( "All events deleted" );
}
#endif

/***********************************************************************************************************************

   Function name: obsoleteAlarmIndex

   Purpose: This function is called to determine if there is a newer instance of an alarm index in the
            event log circular buffer.  The buffer currently continues to append new alarms into the data
            buffer, event after the alarmSequenceID rolls over.  This implementation creates the opportunity for
            the event log to contain multiple instances of the same alarmIndexID, with the most recent alarms
            stored at the rear of the circular buffer.

   Arguments:  RingBufferHead_s * - information to descibe the ring buffer
               PartitionData_t const * - pointer to partion information
               uint8_t - index value we are searching for

   Returns: boolean - if a more recent index is found, return true, else return false

   Side Effects: None

   Reentrant Code: Yes

   Notes: This function can be eliminated in a future release if a new design for searching the log can begin
          with the newest alarms first.
 ******************************************************************************************************************** */
static bool obsoleteAlarmIndex( RingBufferHead_s *rHead, PartitionData_t const *nvram, uint8_t indexValue )
{
   EventStoredData_s          sdata;     /* Current event, used to extract data about current event. */
   uint32_t               bytesRead;  /* total number of bytes read so far */
   uint32_t            bufferLength;  /* total number of bytes of data in data buffer */
   uint16_t                nextSize;         /* Size of the next block of data. */
   bool                  laterIndex;

   // save the current state of the ring buffer head
   uint32_t savedStart = rHead->start;

   // initialize loop control variables
   bytesRead = 0;
   bufferLength = rHead->length;
   laterIndex = (bool)false;

   // search buffer until we reach the end of data in buffer
   while(bytesRead < bufferLength)
   {
      /* read next event data *//*lint -e{644} sdata filled by EventLogRead   */
      if ( EventLogRead( nvram, rHead, ( uint8_t * )&sdata, sizeof( sdata ), FALSE ) >= ( int32_t )sizeof( sdata ) )
      {
         // did we find an index later in the log
         if( indexValue == sdata.alarmId )
         {
            // we found a newer index, break out of loop and return the boolean status
            laterIndex = (bool)true;
            break;
         }

         // get the size of the current event block
         nextSize = sdata.size;

         // move the rHead to next event in memory
         RINGBUFFER_COMMIT_READ( rHead, nextSize );

         // update how many bytes we have read
         bytesRead += nextSize;
      }
      else
      {
         break;
      }
   }

   // Return buffer pointers to original positions
   rHead->start = savedStart;

   return laterIndex;
}

/***********************************************************************************************************************

   Function name: timerDiversityTimerExpired

   Purpose: call back function for the one shot timer for the Power On state, this call
            back post to the EvlAlarmHandler_MsgQ_ by creating a static buffer

   Arguments: uint8_t cmd - from the ucCmd member when the timer was created.
              void *pData - from the pData member when the timer was created

   Returns: void

   Re-entrant Code: No

   Notes:

**********************************************************************************************************************/
static void timerDiversityTimerExpired( uint8_t cmd, void *pData )
{
   ( void ) cmd;
   ( void ) pData;

   static buffer_t buf;

   /***
   * Use a static buffer instead of allocating one because of the allocation fails,
   * there wouldn't be much we could do - we couldn't reset the timer to try again
   * because we're already in the timer callback.
   ***/
   BM_AllocStatic(&buf, eSYSFMT_TIME_DIVERSITY);
   buf.data = (uint8_t*)&evlTimerId;

   OS_MSGQ_Post(&EvlAlarmHandler_MsgQ_, ( void * )&buf);
}

#if ( LAST_GASP_SIMULATION == 1 ) && ( EP == 1 )
/***********************************************************************************************************************

   Function Name: initSimLG

   Purpose: Initialize the resources for the Last Gasp Simulation

   Arguments:

   Returns:

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
static void initSimLG( void )
{
   SimStats_t *getLGSimStats;

   /* Reset the LG Sim statistics */
   getLGSimStats = PWRCFG_get_SimulateLastGaspStats();
   memset( getLGSimStats, 0, (sizeof(getLGSimStats) * 6) );

   PWRLG_SetupLastGasp();

   pSysMem->uMessageCounts.sent = 0;
   PWRLG_MILLISECONDS() = 0;
   PWRLG_SLEEP_SECONDS() = 0;

   PWRLG_TIME_OUTAGE_SET( 0 );
   TOTAL_BACK_OFF_TIME_SET( 0 );
   CUR_BACK_OFF_TIME_SET( 0 );
   TX_FAILURES_SET( 0 );
   RESTORATION_TIME_SET( 0 );
   VBATREG_SHORT_OUTAGE = 1;

   /* If simulating Last Gasp Traffic only, save the current MAC Configuration
      to the backup RAM.  This provides for changes during the LG simulation. */
   if ( PWRCFG_SIM_LG_TX_ONLY == PWRCFG_get_SimulateLastGaspTraffic() )
   {
      /* Save the MAC Configuration for later recovery */
      MAC_SaveConfig_To_BkupRAM();
      simLGMACConfigSaved_ = true;
   }
}

/***********************************************************************************************************************

   Function Name: setSimLGStartAlarm

   Purpose: Creates the calendar alarm to start the LG Simulation process

   Arguments: simLGStartTime - Start time in seconds format to start the process

   Returns: returns eFAILURE if the Calendar Alarm is not created

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
static returnStatus_t setSimLGStartAlarm( uint32_t simLGStartTime )
{
   returnStatus_t           retVal = eFAILURE;
   tTimeSysCalAlarm         StartAlarmID;
   sysTime_t                currSysTime;

   /*  Cancel if alarm currently active */
   if (NO_ALARM_FOUND != schSimLGStartAlarmId_)
   {
      if (eSUCCESS == TIME_SYS_DeleteAlarm(schSimLGStartAlarmId_))
      {
         schSimLGStartAlarmId_ = NO_ALARM_FOUND;
      }
      else
      {
         INFO_printf("Failed to delete schSimLGStartAlarmId_"); // Debugging purposes
      }
   }

   /* Convert the start time from seconds format to sysTime format */
   TIME_UTIL_ConvertSecondsToSysFormat( simLGStartTime, 0, &currSysTime);

   ( void )memset(&StartAlarmID, 0, sizeof(StartAlarmID));          // Clr cal settings, only set what we need.
   StartAlarmID.pMQueueHandle   = &EvlAlarmHandler_MsgQ_;           // Using message queue
   StartAlarmID.ulAlarmDate     = currSysTime.date;                 // Set the alarm date
   StartAlarmID.ulAlarmTime     = currSysTime.time;                 // Set the alarm time to be same as daily shift time
   StartAlarmID.bSkipTimeChange = true;                             // Don't notify on an time change
   if ( TIME_SYS_MAX_SEC == simLGStartTime )                        // Indicator that time is invalid and we want to let Handler know when time becomes valid
   {
      StartAlarmID.bSkipTimeChange = false;                         // Notify on an time change - this will allow proper scheduling on transition to valid time
   }
   StartAlarmID.bUseLocalTime   = false;                            // Don't use local time

   /* Set the alarm */
   if (eSUCCESS == TIME_SYS_AddCalAlarm(&StartAlarmID) )
   {
      schSimLGStartAlarmId_ = StartAlarmID.ucAlarmId;  // Alarm ID will be set after adding alarm.
      retVal = eSUCCESS;
   }
   else
   {
      DBG_logPrintf ('E', "Unable to add Start Alarm");
      retVal = eFAILURE;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function Name: setSimLGDurationAlarm

   Purpose: Creates the calendar alarm for the passed "Duration" (secs) of the LG Simulation

   Arguments: Duration - Duration of the LG simulation in seconds

   Returns: returns eFAILURE if the Calendar Alarm is not created

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
static returnStatus_t setSimLGDurationAlarm( uint16_t simLGDuration )
{
   returnStatus_t           retVal = eFAILURE;
   tTimeSysCalAlarm         DurationAlarmID;
   sysTime_dateFormat_t     currDateTime;
   sysTime_t                currSysTime;

   /*  Cancel if alarm currently active */
   if (NO_ALARM_FOUND != schSimLGDurationAlarmId_)
   {
      if (eSUCCESS == TIME_SYS_DeleteAlarm(schSimLGDurationAlarmId_))
      {
         schSimLGDurationAlarmId_ = NO_ALARM_FOUND;
      }
      else
      {
         INFO_printf( "Unable to delete Duration Alarm" ); // Debugging purposes
      }
   }

   if ( TIME_SYS_GetSysDateTime(&currSysTime) )
   {
       /* Convert to Y,M,D,H,m,s,ms format (just need the year and month portion) */
       ( void )TIME_UTIL_ConvertSysFormatToDateFormat ( &currSysTime, &currDateTime );
   }

   // Get current date/time plus duration, adjust if rolls into next day
   currSysTime.time += simLGDuration * 1000;
   if ( currSysTime.time >= TIME_TICKS_PER_DAY )
   {
      currSysTime.time -= (int32_t)TIME_TICKS_PER_DAY;
      currSysTime.date++;
   }

   ( void )memset(&DurationAlarmID, 0, sizeof(DurationAlarmID));       // Clr cal settings, only set what we need.
   DurationAlarmID.pMQueueHandle   = &EvlAlarmHandler_MsgQ_;           // Using message queue
   DurationAlarmID.ulAlarmDate     = currSysTime.date;                 // Set the alarm date
   DurationAlarmID.ulAlarmTime     = currSysTime.time;                 // Set the alarm time
   DurationAlarmID.bSkipTimeChange = true;                             // Don't notify on an time change
   DurationAlarmID.bUseLocalTime   = false;                            // Don't use local time

   /* Set the alarm */
   if (eSUCCESS == TIME_SYS_AddCalAlarm(&DurationAlarmID) )
   {
      schSimLGDurationAlarmId_ = DurationAlarmID.ucAlarmId;  // Alarm ID will be set after adding alarm.
      retVal = eSUCCESS;
   }
   else
   {
      DBG_logPrintf ('E', "Unable to add Duration Alarm");
   }

   return retVal;
}

/***********************************************************************************************************************

   Function Name: startOneShotTimerAlarm

   Purpose: Creates the calendar alarm for the passed milli secs value

   Arguments: sleepTime - Time to set the calendar alarm

   Returns: returns eFAILURE if the Calendar Alarm is not created

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
static returnStatus_t startOneShotTimerAlarm( uint32_t sleepTime )
{
   returnStatus_t           retVal = eFAILURE;
   tTimeSysCalAlarm         StartAlarmID;
   sysTime_dateFormat_t     currDateTime;
   sysTime_t                currSysTime;

   /*  Cancel if alarm currently active */
   if ( NO_ALARM_FOUND != schSimLGSleepTimerAlarmId_ )
   {
      if ( eSUCCESS == TIME_SYS_DeleteAlarm( schSimLGSleepTimerAlarmId_ ) )
      {
         schSimLGSleepTimerAlarmId_ = NO_ALARM_FOUND;
      }
      else
      {
         INFO_printf( "Unable to delete the Timer Alarm" ); // Debugging purposes
      }
   }

   /* Get the current time and add the sleep time to the millisecs, which will make the
     calendar alarm behave like an one shot timer */
   if ( TIME_SYS_GetSysDateTime(&currSysTime) )
   {
       /* Convert to Y,M,D,H,m,s,ms format (just need the year and month portion) */
       ( void )TIME_UTIL_ConvertSysFormatToDateFormat ( &currSysTime, &currDateTime );
   }

   /* Check for time roll over */
   uint32_t ulAlarmTimeMs = currSysTime.time + sleepTime;
   if ( TIME_TICKS_PER_DAY <= ulAlarmTimeMs )
   {
      currSysTime.date += ( ulAlarmTimeMs / TIME_TICKS_PER_DAY );
      ulAlarmTimeMs = ( ulAlarmTimeMs % TIME_TICKS_PER_DAY );
   }

   ( void )memset(&StartAlarmID, 0, sizeof(StartAlarmID));          // Clr cal settings, only set what we need.
   StartAlarmID.pMQueueHandle   = &EvlAlarmHandler_MsgQ_;           // Using message queue
   StartAlarmID.ulAlarmDate     = currSysTime.date;                 // Set the alarm date
   StartAlarmID.ulAlarmTime     = ulAlarmTimeMs;                    //10*1000;// // Set the alarm time to be same as daily shift time
   StartAlarmID.bSkipTimeChange = true;                             // Don't notify on an time change
   StartAlarmID.bUseLocalTime   = false;                            // Don't use local time

   /* Set the alarm */
   if ( eSUCCESS == TIME_SYS_AddCalAlarm( &StartAlarmID ) )
   {
      schSimLGSleepTimerAlarmId_ = StartAlarmID.ucAlarmId;  // Alarm ID will be set after adding alarm.
      DBG_printf("\n#####################################################");
      DBG_printf("Timer has been initiated for %d Milliseconds", sleepTime);
      DBG_printf("Wait for the LG Simulation to resume");
      DBG_printf("#####################################################\n");
      retVal = eSUCCESS;
   }
   else
   {
      DBG_logPrintf ('E', "Unable to add Timer Alarm");
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: initiateSimLGTx

   Purpose:

   Arguments: None

   Returns: void

   Re-entrant Code: No

   Notes:

**********************************************************************************************************************/
static void initiateSimLGTx( void )
{
   OS_TICK_Struct             endTime;
   OS_TICK_Struct             startTime;
   bool                       bOverflow = ( bool )false;
   SimStats_t                 *getLGSimStats;
   char                       floatStr[PRINT_FLOAT_SIZE];
   float                      Vcap;
   MAC_GetConf_t              GetConf;
   uint32_t                   uChannelAccessFailureCount = 0;
   uint8_t                    uSimulateLastGaspTrafficVal;

   Vcap = ADC_Get_SC_Voltage();
   getLGSimStats = PWRCFG_get_SimulateLastGaspStats();
   OS_TICK_Get_CurrentElapsedTicks( &startTime );

   DBG_logPrintf( 'I', "\n" );
   DBG_logPrintf( 'I', "Running: %u of %u", pSysMem->uMessageCounts.sent + 1, pSysMem->uMessageCounts.total );
   DBG_logPrintf( 'I', "Vcap:            %-6s", DBG_printFloat( floatStr, Vcap, 3 ) );
   DBG_logPrintf( 'I', "Simulation State: %d", simLGState_ );
   DBG_logPrintf( 'I', "outage: %d, Short outage: %d", PWRLG_OUTAGE(), VBATREG_SHORT_OUTAGE );

   if ( 0 != PWRLG_MESSAGE_NUM() )
   {
      DBG_logPrintf( 'I', "Previous Message %u ms", PREV_MSG_TIME() );
   }

   DBG_logPrintf( 'I', "Remaining time:  %ums", PWRLG_MILLISECONDS() );
   DBG_logPrintf( 'I', "CSMA Total/Last: %ums/%ums, TxFail: %u",
                     TOTAL_BACK_OFF_TIME(), CUR_BACK_OFF_TIME(), TX_FAILURES() );

   uSimulateLastGaspTrafficVal = PWRCFG_get_SimulateLastGaspTraffic();

   if( PWRCFG_SIM_LG_TX_ONLY == uSimulateLastGaspTrafficVal )
   {
      /* Alter the MAC parameters if simulating Last Gasp Traffic only */
      PWRLG_SetCsmaParameters();    /* Set parameters as needed for last gasp mode, only. */
      simLGTxSuccessful_ = false;

      while ( !simLGTxSuccessful_ && ( VBATREG_CURR_TX_ATTEMPT < VBATREG_MAX_TX_ATTEMPTS ) )
      {
         /* Capture Simulation stats only when doing Last Gasp Traffic - stats not meaningful in all traffic mode. */
         GetConf = MAC_GetRequest(eMacAttr_ChannelAccessFailureCount) ;
         if ( eMAC_GET_SUCCESS == GetConf.eStatus )
         {
           uChannelAccessFailureCount = GetConf.val.ChannelAccessFailureCount;
         }
         if ( eSimLGDisabled != simLGState_ )
         {
            if ( eSUCCESS == PWRLG_TxMessage( PWRLG_TIME_OUTAGE() ) )
            {
               DBG_logPrintf( 'I', "Attempt %d, SemPend 500ms", VBATREG_CURR_TX_ATTEMPT );
               ( void )OS_SEM_Pend( &SimLGTxDoneSem, 500 ); /* Wait up to 500ms for tx complete.   */
            }
            //simLGTxSuccessful_ = EVL_GetLGTxMsgSentStatus();
            if ( !simLGTxSuccessful_ )
            {
               PWRLG_TxFailure();
            }
            VBATREG_CURR_TX_ATTEMPT++;
         }
         else
         {
            /* Terminate the loop if a cancel LG Sim signal is received */
            break;
         }

         /* Capture Simulation stats when doing Last Gasp Traffic - stats not meaningful in all traffic mode. */
         GetConf = MAC_GetRequest(eMacAttr_ChannelAccessFailureCount) ;
         if ( eMAC_GET_SUCCESS == GetConf.eStatus )
         {
           if ( uChannelAccessFailureCount == GetConf.val.ChannelAccessFailureCount )
           {
              getLGSimStats[PWRLG_MESSAGE_NUM()].pPersistAttempts++;
           }
           else
           {
              getLGSimStats[PWRLG_MESSAGE_NUM()].CCA_Attempts++;
           }
         }
      }  //End of while ( !simLGTxSuccessful_ ...
   }
   else  // Sim Traffic is ALL, so just send the message
   {
      ( void )PWRLG_TxMessage( PWRLG_TIME_OUTAGE() );
      // PWRLG_TxMessage sends the msg to STACK queue with a callback that posts this SEM
      ( void )OS_SEM_Pend( &SimLGTxDoneSem, 2000 ); /* Wait long enough to ensure tx complete. */
   }

   /* Capture Simulation stats if doing Last Gasp Traffic only - stats not meaningful in all traffic mode. */
   if( PWRCFG_SIM_LG_TX_ONLY == uSimulateLastGaspTrafficVal )
   {
      getLGSimStats[PWRLG_MESSAGE_NUM()].MessageSent = simLGTxSuccessful_;
   }

   VBATREG_CURR_TX_ATTEMPT = 0;

   OS_TICK_Get_CurrentElapsedTicks( &endTime );
   // Save the time spent transmitting
   PREV_MSG_TIME_SET(  ( uint16_t ) ( _time_diff_microseconds( &endTime, &startTime, &bOverflow ) / 1000 ) );
   PWRLG_SENT_SET( 1 );

   PWRLG_MESSAGE_NUM_SET( PWRLG_MESSAGE_NUM() + 1 );

   if ( PWRLG_MESSAGE_NUM() >= PWRLG_MESSAGE_COUNT() )   /* Sent all the messages planned?   */
   {
      DBG_logPrintf( 'I',"\nAll messages sent, wait for the LG simulation duration to be completed\n\r" );
      simLGState_ = eSimLGTxDone;
   }
   else
   {
      if ( PWRLG_MESSAGE_NUM() < PWRLG_MESSAGE_COUNT() )
      {
         PWRLG_CalculateSleep( PWRLG_MESSAGE_NUM() );

         uint32_t sleepTimeMSecs = PWRLG_SLEEP_SECONDS() * 1000;
         sleepTimeMSecs += PWRLG_SLEEP_MILLISECONDS();
         PWRLG_SLEEP_SECONDS() = 0;
         PWRLG_SLEEP_MILLISECONDS() = 0;
         ( void )startOneShotTimerAlarm( sleepTimeMSecs );
      }
      else
      {
         simLGState_ = eSimLGTxDone;
         DBG_logPrintf( 'I',"\nAll messages sent, wait for power to return\n\r" );
      }
   }
}

/***********************************************************************************************************************

   Function name: setSimLGPowerOnStateTimer

   Purpose: Initiates a timer for the passed value for the PQ Event and the PR Event

   Arguments: uint32_t timeMilliseconds - interval the LG Sim should wait before the PQ Event is executed

   Returns: returns if the timer is successfully created or not

   Re-entrant Code: No

   Notes:

**********************************************************************************************************************/
static returnStatus_t setSimLGPowerOnStateTimer( uint32_t timeMilliseconds )
{
   returnStatus_t retStatus;

   if ( 0 == powerOnStateTimerId_ )
   {
      //First time, add the timer
      timer_t powerOnStateTmrSettings;             // Timer for sending pending ID bubble up message

      ( void )memset(&powerOnStateTmrSettings, 0, sizeof(powerOnStateTmrSettings));
      powerOnStateTmrSettings.bOneShot = true;
      powerOnStateTmrSettings.bOneShotDelete = false;
      powerOnStateTmrSettings.bExpired = false;
      powerOnStateTmrSettings.bStopped = false;
      if ( 0 < timeMilliseconds )
      {
         powerOnStateTmrSettings.ulDuration_mS = timeMilliseconds;
      }else{
         powerOnStateTmrSettings.ulDuration_mS = 1;
      }

      powerOnStateTmrSettings.pFunctCallBack = powerOnStateTimerExpired;

      retStatus = TMR_AddTimer(&powerOnStateTmrSettings);
      if (eSUCCESS == retStatus)
      {
         powerOnStateTimerId_ = powerOnStateTmrSettings.usiTimerId;
      }
      else
      {
         ERR_printf("PD unable to add timer");
      }
   }
   else
   {  //Timer already exists, reset the timer
      retStatus = TMR_ResetTimer(powerOnStateTimerId_, timeMilliseconds);
   }

   return(retStatus);
}

/***********************************************************************************************************************

   Function name: powerOnStateTimerExpired

   Purpose: call back function for the one shot timer for the Power On state, this call
            back post to the EvlAlarmHandler_MsgQ_ by creating a static buffer

   Arguments: uint8_t cmd - from the ucCmd member when the timer was created.
              void *pData - from the pData member when the timer was created

   Returns: void

   Re-entrant Code: No

   Notes:

**********************************************************************************************************************/
static void powerOnStateTimerExpired( uint8_t cmd, void *pData )
{
   ( void ) cmd;
   ( void ) pData;

   static buffer_t buf;

   /***
   * Use a static buffer instead of allocating one because of the allocation fails,
   * there wouldn't be much we could do - we couldn't reset the timer to try again
   * because we're already in the timer callback.
   ***/
   BM_AllocStatic(&buf, eSYSFMT_TIME_DIVERSITY);
   buf.data = (uint8_t*)&powerOnStateTimerId_;

   OS_MSGQ_Post(&EvlAlarmHandler_MsgQ_, ( void * )&buf);
}

/***********************************************************************************************************************

   Function name: resetSimLGCsmaParameters

   Purpose: Set the default value for minimum p persistence, CSMA max attempts, and CSMA quick abort.

   Arguments: void

   Returns: void

   Re-entrant Code: No

   Notes:  Expect these values to not be permanent (i.e. the file is not written)

 **********************************************************************************************************************/
static void resetSimLGCsmaParameters( void )
{
   float         csmaPValue      = VBATREG_CSMA_PVALUE;
   uint8_t       csmaMaxAttempts = VBATREG_MAX_TX_ATTEMPTS;
   bool          csmaQuickAbort  = VBATREG_CSMA_QUICK_ABORT;

   // Set p persistence default value. OK to update since it is not actually writting the file
   ( void )MAC_SetRequest( eMacAttr_CsmaPValue, &csmaPValue );

   // Set CSMA max attempts to default value..  OK to update since it is not actually writting the file
   ( void )MAC_SetRequest( eMacAttr_CsmaMaxAttempts, &csmaMaxAttempts );

   // Set CSMA quick abort to default value.  OK to update since it is not actually writting the file
   ( void )MAC_SetRequest( eMacAttr_CsmaQuickAbort, &csmaQuickAbort );
}
#endif

/***********************************************************************************************************************

   Function name: eraseEventLogPartition

   Purpose: Erase the real time alarm event log and post message to alarm task to log an event

   Arguments: void

   Returns: returnStatus_t - success/fail

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
static returnStatus_t eraseEventLogPartition( ePartitionName partName )
{
   returnStatus_t retVal = eFAILURE;   // buffer erase operation status

   if( ePART_NV_HIGH_PRTY_ALRM == partName || ePART_NV_LOW_PRTY_ALRM == partName )
   {
      if( ePART_NV_HIGH_PRTY_ALRM == partName )
      {  // reset the meta data for the real time alarms
         _EvlMetaData.HighBufferHead.start  = 0;
         _EvlMetaData.HighBufferHead.length = 0;
      }
      else
      {  // rest the meta data for the opportunistic alarms
         _EvlMetaData.NormalBufferHead.start  = 0;
         _EvlMetaData.NormalBufferHead.length = 0;
      }

      // After code review, decision not using mutex here to avoid large partition erase holding up at power down

      if( eSUCCESS == FIO_fwrite( &_EvlFileHandleMeta, 0, ( uint8_t* ) &_EvlMetaData, ( lCnt )sizeof( EventLogFile_s ) ) )
      {  // buffer metat data has been updated, no we need to erase the partition containing  the events logged
         if( ePART_NV_HIGH_PRTY_ALRM == partName )
         {  // erase the real time partition
            retVal = PAR_partitionFptr.parErase( 0, PART_NV_HIGH_PRTY_ALRM_SIZE, _EvlHighFlashHandle );
         }
         else
         {  // erase the opportunistic alarm partition
            retVal = PAR_partitionFptr.parErase( 0, PART_NV_LOW_PRTY_ALRM_SIZE, _EvlNormalFlashHandle );
         }
      }

      if( eSUCCESS == retVal )
      {  /* event log was successfully erased, we need to send a message to the EVL_AlarmHandlerTask.
            Logging the event here may not be an option due to when the corruption was discovered.  Send
            a message to the EVL_AlarmHandlerTask to generate the corruption/buffer initlize events.
            Use a static buffer instead of allocating one because of the allocation fails,
            there wouldn't be much we could do */
         static buffer_t buf;
         static EventLogCoruptionClearedEvent_s eventInfo_;
         BM_AllocStatic(&buf, eSYSFMT_EVL_ALARM_LOG_RESET_EVENT);
         buf.data = (uint8_t *)&eventInfo_;

         if( ePART_NV_HIGH_PRTY_ALRM == partName )
         {  // real time alarm buffer was cleared, set the reading type to the realTimeAlarmIndex
            eventInfo_.alarmIndexReadingType = realTimeAlarmIndexID;
            eventInfo_.alarmIndexValue = EVL_GetRealTimeIndex();
         }
         else
         {  // else, the oppurtunisticalarmIndexId needs to be sent
            eventInfo_.alarmIndexReadingType = opportunisticAlarmIndexID;
            eventInfo_.alarmIndexValue = EVL_GetNormalIndex();
         }
         OS_MSGQ_Post( &EvlAlarmHandler_MsgQ_, ( void * )&buf ); // Post the message to the task
         INFO_printf( "Event log corruption found - Partition: %u erased.", partName );
      }
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: generateEventLogCoruptionClearedEvents

   Purpose: Generate events needed to be logged when erasing an event log buffer

   Arguments: meterReadingType readingType - alarm index reading type enumeration

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
static void generateEventLogCoruptionClearedEvents( EventLogCoruptionClearedEvent_s const *eventBufClearedInfo )
{
   EventData_s                          eventInfo;   // Event info
   EventKeyValuePair_s                    nvpInfo;   // Event key/value pair info

   // Initializet the variables used to store an event
   (void)memset((uint8_t *)&eventInfo, 0, sizeof(eventInfo) );
   (void)memset((uint8_t *)&nvpInfo, 0, sizeof(nvpInfo) );

   // Update the event and nvp info
   eventInfo.eventId = comDeviceEventLogCorrupted;
   eventInfo.eventKeyValuePairsCount = 1;
   eventInfo.eventKeyValueSize = sizeof(eventBufClearedInfo->alarmIndexValue);
   (void)memcpy( nvpInfo.Key, (uint8_t *)&eventBufClearedInfo->alarmIndexReadingType, sizeof( nvpInfo.Key ) );
   (void)memcpy( nvpInfo.Value, &eventBufClearedInfo->alarmIndexValue, sizeof( eventBufClearedInfo->alarmIndexValue ) );

   // Log the event
   (void)EVL_LogEvent( 172, &eventInfo, &nvpInfo, TIMESTAMP_NOT_PROVIDED, NULL );

   /* A new event needs to be sent with the same nvp information, update the event id in the event info
      structure and then log the event */
   eventInfo.eventId = comDeviceEventLogInitialized;

   // Log the event
   (void)EVL_LogEvent( 171, &eventInfo, &nvpInfo, TIMESTAMP_NOT_PROVIDED, NULL );
}

#if ( EVL_UNIT_TESTING == 1 )
/***********************************************************************************************************************

   Function name: EVL_generateMakeRoomOpportunisticFailure

   Purpose: This function will generate a failure when attemting to make room logging a new event in the
            opportunistic event log buffer

   Arguments: None

   Returns: void

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void EVL_generateMakeRoomOpportunisticFailure( void )
{
   EventStoredData_s       sd;     // Buffer of stored data to be written
   RingBufferHead_s headCopy;      // Variable to store copy of the buffer head data

   (void)memset( (uint8_t *)&sd, 0, sizeof(sd) );

   // copy the current buffer head meta data contents
   (void)memcpy( (uint8_t *)&headCopy, (uint8_t *)&_EvlMetaData.NormalBufferHead, sizeof(headCopy) );

   headCopy.length = headCopy.bufferSize;  // force the buffer meta data into a full condition
   headCopy.start += sizeof(sd.size);      // shift the head past the size element of event log structure

   // attempt to log the event
   (void)EventLogWrite( _EvlNormalFlashHandle, &headCopy, (uint8_t const *) &sd, (uint32_t)sizeof(sd), FALSE );
}

/***********************************************************************************************************************

   Function name: EVL_generateMakeRoomRealtimeFailure

   Purpose: This function will generate a failure when attemting to make room logging a new event in the
            real time event log buffer

   Arguments: None

   Returns: void

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void EVL_generateMakeRoomRealtimeFailure( void )
{
   EventStoredData_s       sd;         // Buffer of stored data to be written
   RingBufferHead_s headCopy;          // Variable to store copy of the buffer head data

   (void)memset( (uint8_t *)&sd, 0, sizeof(sd) );

   // copy the current buffer head meta data contents
   (void)memcpy( (uint8_t *)&headCopy, (uint8_t *)&_EvlMetaData.HighBufferHead, sizeof(headCopy) );

   headCopy.length = headCopy.bufferSize;  // force the buffer meta data into a full condition
   headCopy.start += sizeof(sd.size);      // shift the head past the size element of event log structure

   // attempt to log the event
   (void)EventLogWrite( _EvlHighFlashHandle, &headCopy, (uint8_t const *) &sd, (uint32_t)sizeof(sd), FALSE );
}

/***********************************************************************************************************************

   Function name: EVL_loadLogBufferCorruption

   Purpose: This function will generate a corruption failure when attemting to load event from an event buffer
            using a call to EventLogLoadBuffer.

   Arguments: None

   Returns: void

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void EVL_loadLogBufferCorruption( uint8_t priorityThreshold, uint8_t alarmIndex )
{
   EventQuery_s query;               // varialbe to create the query
   EventStoredData_s       sd;       // Buffer of stored data to be written
   uint32_t bytesReturned = 0;       // needed for function call
   uint8_t lastAlarmIdReturned = 0;  // needed for function call

   (void)memset( (uint8_t *)&sd, 0, sizeof(sd) );

   if( priorityThreshold >= _EvlMetaData.rtThreshold )
   {  // bump the buffer head of the real time buffer meta data
      _EvlMetaData.HighBufferHead.start += sizeof(sd.size);
   }
   else
   {  // bump the buffer head of the opportunistic buffer meta data
      _EvlMetaData.NormalBufferHead.start += sizeof(sd.size);
   }

   query.qType = QUERY_BY_INDEX_e;        // set the query type to be by index, this is what OR_HA will be doing
   query.start_u.eventId = (uint16_t)alarmIndex;  // set the start index
   query.end_u.eventId = (uint16_t)alarmIndex;    // set the stop index

   // attempt retrieve the event at the provided index
   (void)EventLogLoadBuffer( priorityThreshold, (uint8_t *)&sd, (uint32_t)sizeof(sd), &bytesReturned, &lastAlarmIdReturned, query, NULL );
}
#endif
