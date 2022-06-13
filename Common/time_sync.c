/***********************************************************************************************************************
 *
 * Filename:    time_sync.c
 *
 * Global Designator: TIME_SYNC_
 *
 * Contents: Functions related to time sync between EP's and DCU
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdlib.h>
#include <math.h>
#include "portable_freescale.h"
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#include "time_sync.h"
#include "time_sys.h"
#include "time_util.h"
#include "BSP_aclara.h"
#include "file_io.h"
#include "DBG_SerialDebug.h"
#include "mac.h"
#include "radio.h"
#include "radio_hal.h"
#if (DCU == 1)
#include "version.h"
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define TIME_SYNC_FILE_UPDATE_RATE    ((uint32_t)24 * 60 * 60) //Once a day or less
#if ( EP == 1 )
#define MAX_CLOCK_DRIFT               ((uint32_t)270) // EP uses the crystal watch that drifts up to +/-270 ppm
#define TIME_ACCURACY_DEFAULT         ((int8_t)5)
#else
#define MAX_CLOCK_DRIFT               ((uint32_t)50)  // T-board uses an industrial grade crystal that drifts up to +/-50 ppm
#define TIME_ACCURACY_DEFAULT         ((int8_t)1)     // 2 sec precision
#define TIME_ACCURACY_DEFAULT_GPS     ((int8_t)-20)   // 1 usec precision
#endif
#define MAX_PRECISION_DRIFT_MICROSEC  ((uint32_t)32000001)
#define TIME_SET_MAX_OFFSET_DEFAULT   (10000) //in milli-seconds
#define TIME_SET_MAX_OFFSET_MAX       (20000) //in milli-seconds
#define TIME_SET_START_DEFAULT        (1765)  //00:29:25
#define TIME_SET_START_MAX            (86399)
#define TIME_SET_PERIOD_DEFAULT       (3600)  //in seconds

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  timeLeapIndication;  /* Indicates how to manage leap seconds or if time source has not been updated recently */
   int8_t   timeDefaultAccuracy; /* The accuracy of the system clock on the device.  In log base 2 format. */
   uint16_t timeSetMaxOffset;    /* The maximum amount of random offset in milliseconds from timeSetStart for which
                                    the source will begin transmitting the time set MAC command. */
   uint32_t timeSetPeriod;       /* The period, in seconds, for which the time set MAC command broadcast
                                    schedule is computed.  A value of zero will prevent the MAC from
                                    automatically transmitting the time set MAC command.
                                    Values 0, 900, 1800, 3600, 7200, 10800, 14400, 21600, 28800, 43200, 86400
                                    default = 3600  */
   uint32_t timeSetStart;         /* The start time in seconds relative to the source time of midnight UTC
                                     for starting the broadcast schedule. Range: 0-86399, default = 1765 */
   uint8_t  timeSource;           /* Enables the time-sync broadcast and consumer. Default: DCU - Source, EP - Consumer */
   uint8_t  timeQueryResponseMode;/* When receiving a time request, send the time in broadcast (0, default), unicast(1) or ignore the request (2). */
}time_sync_vars_t;
PACK_END


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static OS_MUTEX_Obj  timeSyncMutex_;                     /* Serialize access to time sync data-structure */
static uint8_t       timeSyncAlarmID_ =  NO_ALARM_FOUND; /* Alarm ID to store the ID of alarm used by time-sync */
static FileHandle_t  fileHndlTimeSync_ = {0};            /* Contains the file handle information */
static time_sync_vars_t timeSyncVars_;                   /* Time related variable */
static int8_t        _timeLastUpdatedPrecision;          /* The last updated precision of the system clock on the device.  In log base 2 format. */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
STATIC void     TimeSync_ManageAlarm(void);
static uint32_t TIME_SYNC_timeMaxClockDrift_Get( void );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/*****************************************************************************************************************
 *
 * Function name: TIME_SYNC_Init
 *
 * Purpose: This function is called before starting the scheduler. Initialize data-structures and create resources
 *          needed by this module
 *
 * Arguments: None
 *
 * Returns: returnStatus_t eSUCCESS/eFAILURE indication
 *
 * Reentrant: NO. This function should be called once before starting the scheduler
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYNC_Init( void )
{
   returnStatus_t retVal = eFAILURE;   /* Return value */

   if ( OS_MUTEX_Create(&timeSyncMutex_) )
   {  //Mutex create succeeded, initialize the data structure
      FileStatus_t   fileStatusCfg;  //Contains the file status

      if (eSUCCESS == FIO_fopen(&fileHndlTimeSync_, ePART_SEARCH_BY_TIMING, (uint16_t)eFN_TIME_SYNC,
                                (lCnt)sizeof(timeSyncVars_), FILE_IS_NOT_CHECKSUMED,
                                TIME_SYNC_FILE_UPDATE_RATE, &fileStatusCfg) )
      {
         if (fileStatusCfg.bFileCreated)
         {
            (void)memset(&timeSyncVars_, 0, sizeof(timeSyncVars_));
            timeSyncVars_.timeLeapIndication    = LEAP_INDICATOR_NO_WARNING;
            timeSyncVars_.timeDefaultAccuracy   = TIME_ACCURACY_DEFAULT;
            timeSyncVars_.timeQueryResponseMode = BROADCAST_MODE;
            timeSyncVars_.timeSetMaxOffset      = TIME_SET_MAX_OFFSET_DEFAULT;
            timeSyncVars_.timeSetPeriod         = TIME_SET_PERIOD_DEFAULT;
            timeSyncVars_.timeSetStart          = TIME_SET_START_DEFAULT;
#if ( EP == 1 )
            timeSyncVars_.timeSource            = TIME_SYNC_CONSUMER;
#else
            timeSyncVars_.timeSource            = TIME_SYNC_SOURCE;
#endif
            retVal = FIO_fwrite(&fileHndlTimeSync_, 0, (uint8_t*)&timeSyncVars_, (lCnt)sizeof(timeSyncVars_));
         }
         else
         {  //read the file
            retVal = FIO_fread(&fileHndlTimeSync_, (uint8_t *)&timeSyncVars_, 0, (lCnt)sizeof(timeSyncVars_));
         }
         _timeLastUpdatedPrecision = timeSyncVars_.timeDefaultAccuracy;
      }
   }
#if ( TEST_TDMA == 0 )
   TimeSync_ManageAlarm(); //Add periodic alarm to send timesync
#endif
   return retVal;
}

/*****************************************************************************************************************
 * This function computes the max possible error(in micro-seconds) based on precision value passed.
 * Reentrant: YES
 ******************************************************************************************************************/
uint32_t TIME_SYNC_LogPrecisionToPrecisionInMicroSec(int8_t precision)
{
   uint32_t maxErrorInTimeSync;

   if ( precision < 0 )
   {
      maxErrorInTimeSync = (uint32_t)0x1 << (precision * -1); //lint !e504 precision was negative before * -1
      maxErrorInTimeSync = 1000000 /  maxErrorInTimeSync;
   }
   else
   {
      maxErrorInTimeSync = (uint32_t)0x1 << precision;
      maxErrorInTimeSync = 1000000 * maxErrorInTimeSync;
   }
   return (maxErrorInTimeSync);
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYNC_timeMaxClockDrift_Get
 *
 * Purpose: Return the current clock drift in PPM
 *
 * Arguments: None
 *
 * Returns: Clock drift in PPM
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
static uint32_t TIME_SYNC_timeMaxClockDrift_Get( void )
{
   uint32_t drift = MAX_CLOCK_DRIFT;

   // On an EP and DCU2, the drift is hard coded based on the clock source.
   // On a DCU2+, it depends if we are using GPS or not.
#if ( DCU == 1 )
   if ( (VER_getDCUVersion() != eDCU2) && //We are a DCU2+
         TIME_SYS_IsGpsTimeValid() &&  // If we have received 'DT' at least twice
         TIME_SYS_IsGpsPresent()   &&  // and we are currently receiving PPS
        (TIME_SYS_PhaseErrorUsec() == 0) ) // and we are phase locked to GPS (i.e. error is sub microsecond)
   {
      drift = 0; // No drift
   }
#endif
   return drift;
}

/*****************************************************************************************************************
 * This function computes the max possible error(in micro-seconds) based on precision value of the last accepted
 * time-sync and the time since last time-sync was accepted and the accuracy of the clock.
 * Reentrant: YES
 ******************************************************************************************************************/
uint32_t TIME_SYNC_TimePrecisionMicroSeconds_Get(void)
{
   uint64_t maxDriftInMicroSec = MAX_PRECISION_DRIFT_MICROSEC; //Default
   sysTimeCombined_t sCombTime;

   //Compute seconds since last update
   if (eSUCCESS == TIME_UTIL_GetTimeInSysCombined(&sCombTime) )
   {
      maxDriftInMicroSec = sCombTime / TIME_TICKS_PER_SEC; //Current time in seconds
      maxDriftInMicroSec = abs(maxDriftInMicroSec - TIME_SYS_GetTimeLastUpdated()); /*lint !e712 !e732 !e747 */ //Seconds since last update
      maxDriftInMicroSec *= TIME_SYNC_timeMaxClockDrift_Get(); //Max possible drift since last update in micro-seconds
      maxDriftInMicroSec += TIME_SYNC_LogPrecisionToPrecisionInMicroSec(TIME_SYNC_TimeLastUpdatedPrecision_Get()); //Max possible error in current time (in micro-seconds)
      if (maxDriftInMicroSec > MAX_PRECISION_DRIFT_MICROSEC)
      {
         maxDriftInMicroSec = MAX_PRECISION_DRIFT_MICROSEC;
      }
   }
   return ((uint32_t)maxDriftInMicroSec);
}

/*****************************************************************************************************************
 *  Manage TimeSync Alarm. Add or delete based on period and timeSource
 ******************************************************************************************************************/
static void TimeSync_ManageAlarm(void)
{
#if ( MFG_MODE_DCU == 0 ) //Periodic time sync are disabled in DCU MFG Mode
   if ( 0 != TIME_SYNC_TimeSetPeriod_Get() &&
        ( timeSyncVars_.timeSource == TIME_SYNC_SOURCE || timeSyncVars_.timeSource == TIME_SYNC_BOTH) )
   {  //Time-sync period is valid and is enabled. Add periodic alarm
      tTimeSysPerAlarm alarmSettings;

      if (timeSyncAlarmID_ != NO_ALARM_FOUND)
      { // Alarm already active, delete it
         (void)TIME_SYS_DeleteAlarm (timeSyncAlarmID_) ;
         timeSyncAlarmID_ = NO_ALARM_FOUND;
      }

      //Set up a periodic alarm
      (void)memset(&alarmSettings, 0, sizeof(alarmSettings));   // Clear settings, only set what we need
      alarmSettings.bOnValidTime    = true;                     /* Alarmed on valid time */
      alarmSettings.bSkipTimeChange = true;                     /* do NOT notify on time change */
      alarmSettings.pMQueueHandle   = MAC_GetMsgQueue();        /* Uses the message Queue */
      alarmSettings.ulPeriod        = TIME_SYNC_TimeSetPeriod_Get() * TIME_TICKS_PER_SEC;
      alarmSettings.ulOffset        = (timeSyncVars_.timeSetStart   * TIME_TICKS_PER_SEC) + aclara_randu(0, timeSyncVars_.timeSetMaxOffset);
      alarmSettings.ulOffset       %= alarmSettings.ulPeriod;   //offset can not be >= period
      alarmSettings.ulOffset        = (alarmSettings.ulOffset / SYS_TIME_TICK_IN_mS) * SYS_TIME_TICK_IN_mS; //round down to the tick
      (void)TIME_SYS_AddPerAlarm(&alarmSettings);

      timeSyncAlarmID_ = alarmSettings.ucAlarmId; //save the alarm ID
   }
   else
   {  //Time-sync disabled. Delete alarm, if available
      (void)TIME_SYS_DeleteAlarm (timeSyncAlarmID_) ;
      timeSyncAlarmID_ =  NO_ALARM_FOUND;
   }
#endif
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYNC_AddTimeSyncPayload
 *
 * Purpose: MAC layer calls this function to get the payload of time sync command
 *
 * Arguments: uint8_t *: payload
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_SYNC_AddTimeSyncPayload(uint8_t *payload)
{
   uint8_t val;
  //The calling function already added date and time in the first eight bytes. Add rest of the payload here
  //Change later to move all the code here so that everything is done in one place
   if ( TIME_SYNC_TimeSetPeriod_Get() == 0 ) {
      // When SetPrecision is 0, we want to make the EP to beleive that the time will never be transmitted again (i.e. long time)
      val = 0xFF;
   } else {
      val = (uint8_t)round(log2((double)TIME_SYNC_TimeSetPeriod_Get()));
   }
   payload[8] = ((timeSyncVars_.timeLeapIndication << 6) & 0xC0) | (val & 0x3F);
#if 1 // TODO: RA6E1 Bob: Remove this later
   payload[9] = (uint8_t)TIME_SYNC_TimePrecision_Get();
#else
   payload[9] = (uint8_t)TIME_PRECISION_MIN;
#warning "This code always sends TIME_PRECISION_MIN in a Time Sync message.  Do not release it this way!!!"
#endif
}

/***********************************************************************************************************************
 * This function sets the timeLastUpdatedPrecision
 * ***********************************************************************************************************************/
returnStatus_t TIME_SYNC_TimeLastUpdatedPrecision_Set(int8_t val)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if ( (val >= TIME_PRECISION_MIN) && (val <= TIME_PRECISION_MAX) )//Time precision range check
   {
      OS_MUTEX_Lock( &timeSyncMutex_ ); // Function will not return if it fails
      _timeLastUpdatedPrecision = val;
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &timeSyncMutex_ ); // Function will not return if it fails
   }
   return retVal;
}

/***********************************************************************************************************************
 * This function sets the timeDefaultAccuracy
 * ***********************************************************************************************************************/
returnStatus_t TIME_SYNC_TimeDefaultAccuracy_Set(int8_t val)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if ( (val >= TIME_PRECISION_MIN) && (val <= TIME_PRECISION_MAX) )//Time precision range check
   {
      OS_MUTEX_Lock( &timeSyncMutex_ ); // Function will not return if it fails
      timeSyncVars_.timeDefaultAccuracy = val;
      (void)FIO_fwrite(&fileHndlTimeSync_, (uint16_t)offsetof(time_sync_vars_t,timeDefaultAccuracy),
                      (uint8_t*)&timeSyncVars_.timeDefaultAccuracy, (lCnt)sizeof(timeSyncVars_.timeDefaultAccuracy));
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &timeSyncMutex_ ); // Function will not return if it fails
   }
   return retVal;
}

/***********************************************************************************************************************
 * This function sets the timeQueryResponseMode
 * ***********************************************************************************************************************/
returnStatus_t TIME_SYNC_TimeQueryResponseMode_Set(uint8_t val)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if ( val <= IGNORE_QUERY_MODE )//Range check
   {
      OS_MUTEX_Lock( &timeSyncMutex_ ); // Function will not return if it fails
      timeSyncVars_.timeQueryResponseMode = val;
      (void)FIO_fwrite(&fileHndlTimeSync_, (uint16_t)offsetof(time_sync_vars_t,timeQueryResponseMode),
                      (uint8_t*)&timeSyncVars_.timeQueryResponseMode, (lCnt)sizeof(timeSyncVars_.timeQueryResponseMode));
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &timeSyncMutex_ ); // Function will not return if it fails
   }
   return retVal;
}

/***********************************************************************************************************************
 * This function sets the timeSetMaxOffset
 * ***********************************************************************************************************************/
returnStatus_t TIME_SYNC_TimeSetMaxOffset_Set(uint16_t val)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (val <= TIME_SET_MAX_OFFSET_MAX)
   {
      OS_MUTEX_Lock( &timeSyncMutex_ ); // Function will not return if it fails
      timeSyncVars_.timeSetMaxOffset = val;
      (void)FIO_fwrite(&fileHndlTimeSync_, (uint16_t)offsetof(time_sync_vars_t,timeSetMaxOffset),
                       (uint8_t*)&timeSyncVars_.timeSetMaxOffset, (lCnt)sizeof(timeSyncVars_.timeSetMaxOffset));
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &timeSyncMutex_ ); // Function will not return if it fails
      TimeSync_ManageAlarm();
   }
   return retVal;
}

/***********************************************************************************************************************
 * This function sets the timeSetPeriod
 * ***********************************************************************************************************************/
returnStatus_t TIME_SYNC_TimeSetPeriod_Set(uint32_t val)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (val ==     0 ||
       val ==   900 ||
       val ==  1800 ||
       val ==  3600 ||
       val ==  7200 ||
       val == 10800 ||
       val == 14400 ||
       val == 21600 ||
       val == 28800 ||
       val == 43200 ||
       val == 86400)
   {
      OS_MUTEX_Lock( &timeSyncMutex_ ); // Function will not return if it fails
      timeSyncVars_.timeSetPeriod = val;
      (void)FIO_fwrite(&fileHndlTimeSync_, (uint16_t)offsetof(time_sync_vars_t,timeSetPeriod),
                       (uint8_t*)&timeSyncVars_.timeSetPeriod, (lCnt)sizeof(timeSyncVars_.timeSetPeriod));
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &timeSyncMutex_ ); // Function will not return if it fails
      TimeSync_ManageAlarm();
   }
   return retVal;
}

/***********************************************************************************************************************
 * This function sets the timeSetStart
 * ***********************************************************************************************************************/
returnStatus_t TIME_SYNC_TimeSetStart_Set(uint32_t val)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (val <= TIME_SET_START_MAX)
   {
      OS_MUTEX_Lock( &timeSyncMutex_ ); // Function will not return if it fails
      timeSyncVars_.timeSetStart = val;
      (void)FIO_fwrite(&fileHndlTimeSync_, (uint16_t)offsetof(time_sync_vars_t,timeSetStart),
                       (uint8_t*)&timeSyncVars_.timeSetStart, (lCnt)sizeof(timeSyncVars_.timeSetStart));
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &timeSyncMutex_ ); // Function will not return if it fails
      TimeSync_ManageAlarm();
   }
   return retVal;
}

/***********************************************************************************************************************
 * This function sets the timeSource
 * ***********************************************************************************************************************/
returnStatus_t TIME_SYNC_TimeSource_Set(uint8_t val)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (val == TIME_SYNC_SOURCE || val == TIME_SYNC_CONSUMER || val == TIME_SYNC_BOTH )
   {
      OS_MUTEX_Lock( &timeSyncMutex_ ); // Function will not return if it fails
      timeSyncVars_.timeSource = val;
      (void)FIO_fwrite(&fileHndlTimeSync_, (uint16_t)offsetof(time_sync_vars_t,timeSource),
                       (uint8_t*)&timeSyncVars_.timeSource, (lCnt)sizeof(timeSyncVars_.timeSource));
      retVal = eSUCCESS;
      OS_MUTEX_Unlock( &timeSyncMutex_ ); // Function will not return if it fails
      TimeSync_ManageAlarm();
   }
   return retVal;
}


/***********************************************************************************************************************
 * This function returns the timeLastUpdatedPrecision
 * ***********************************************************************************************************************/
int8_t TIME_SYNC_TimeLastUpdatedPrecision_Get(void)
{
#if ( DCU == 1 )
   // On DCU2+, the precision can change depending on GPS use or not
   if ( VER_getDCUVersion() != eDCU2 ) {
      if (  TIME_SYS_IsGpsTimeValid() &&  // If we have received 'DT' at least twice
            TIME_SYS_IsGpsPresent()   &&  // and we are currently receiving PPS
           (TIME_SYS_PhaseErrorUsec() == 0) ) // and we are phase locked to GPS (i.e. error is sub microsecond)
      {
         (void)TIME_SYNC_TimeLastUpdatedPrecision_Set(TIME_ACCURACY_DEFAULT_GPS);           // Getting time from GPS
      } else {
         (void)TIME_SYNC_TimeLastUpdatedPrecision_Set(TIME_SYNC_TimeDefaultAccuracy_Get()); // Getting time from Aclara One (likely)
      }
   }
#endif
   return(_timeLastUpdatedPrecision);
}

/***********************************************************************************************************************
 * This function returns the timePrecision
 * ***********************************************************************************************************************/
int8_t TIME_SYNC_TimePrecision_Get(void)
{
   double precision;

   precision = (double)TIME_SYNC_TimePrecisionMicroSeconds_Get() / (double)1000000;

   // Make sure precision is not smaller than the minimum value.
   // This will trap cases where precision could be 0 or negative in which case log2 would fail
   if ( precision < pow((double)2, (double)TIME_PRECISION_MIN) ) {
      precision = TIME_PRECISION_MIN;
   } else {
      precision = log2(precision);
      precision = ceil(precision);
      if ( precision < TIME_PRECISION_MIN )
      {
         precision = TIME_PRECISION_MIN;
      }
      else if ( precision > TIME_PRECISION_MAX )
      {
         precision = TIME_PRECISION_MAX;
      }
   }
   return((int8_t)precision);
}

/***********************************************************************************************************************
 * This function returns the timeDefaultAccuracy
 * ***********************************************************************************************************************/
int8_t TIME_SYNC_TimeDefaultAccuracy_Get(void)
{
   return(timeSyncVars_.timeDefaultAccuracy);
}

/***********************************************************************************************************************
 * This function returns the timeQueryResponseMode
 * ***********************************************************************************************************************/
uint8_t TIME_SYNC_TimeQueryResponseMode_Get(void)
{
   return(timeSyncVars_.timeQueryResponseMode);
}

/***********************************************************************************************************************
 * This function returns the timeSetMaxOffset
 * ***********************************************************************************************************************/
uint16_t TIME_SYNC_TimeSetMaxOffset_Get(void)
{
   return(timeSyncVars_.timeSetMaxOffset);
}

/***********************************************************************************************************************
 * This function returns the timeSetPeriod
 * ***********************************************************************************************************************/
uint32_t TIME_SYNC_TimeSetPeriod_Get(void)
{
   return(timeSyncVars_.timeSetPeriod);
}

/***********************************************************************************************************************
 * This function returns the timeSetStart
 * ***********************************************************************************************************************/
uint32_t TIME_SYNC_TimeSetStart_Get(void)
{
   return(timeSyncVars_.timeSetStart);
}

/***********************************************************************************************************************
 * This function returns the timeSource
 * ***********************************************************************************************************************/
uint8_t TIME_SYNC_TimeSource_Get(void)
{
   return(timeSyncVars_.timeSource);
}

/***********************************************************************************************************************
 * Prints the time sync parameters
 * ***********************************************************************************************************************/
void TIME_SYNC_Parameters_Print(void)
{
   sysTime_t             sysTime;
   sysTime_dateFormat_t  sysDateTime;
   uint8_t               radioNum;
   uint32_t              TimeLastUpdated = TIME_SYS_GetTimeLastUpdated();
   uint32_t              CPUfreq, TCXOfreq;
   uint32_t              CPUlastUpdate, TCXOlastUpdate;
   bool                  CPUretVal, TCXOretVal;
   TIME_SYS_SOURCE_e     CPUsource, TCXOsource;

   TIME_UTIL_ConvertSecondsToSysFormat( TimeLastUpdated, 0, &sysTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sysTime, &sysDateTime);

   INFO_printf("TimeLastUpdated           = %02u/%02u/%04u %02u:%02u:%02u (%u)", sysDateTime.month, sysDateTime.day, sysDateTime.year, sysDateTime.hour, sysDateTime.min, sysDateTime.sec, TimeLastUpdated );
   INFO_printf("TimeLeapIndication        = %u", timeSyncVars_.timeLeapIndication    );
   INFO_printf("TimeDefaultAccuracy       = %d", TIME_SYNC_TimeDefaultAccuracy_Get() );
   INFO_printf("TimeQueryResponseMode     = %u", timeSyncVars_.timeQueryResponseMode );
   INFO_printf("TimePrecision             = %d", TIME_SYNC_TimePrecision_Get()       );
   INFO_printf("TimePrecisionMicroSeconds = %u", TIME_SYNC_TimePrecisionMicroSeconds_Get() );
   INFO_printf("TimeLastUpdatedPrecision  = %d", _timeLastUpdatedPrecision           );
   INFO_printf("TimeMaxClockDrift         = %u", TIME_SYNC_timeMaxClockDrift_Get()   );
   INFO_printf("TimeSetMaxOffset          = %u", timeSyncVars_.timeSetMaxOffset      );
   INFO_printf("TimeSetPeriod             = %u", TIME_SYNC_TimeSetPeriod_Get()       );
   INFO_printf("TimeSetStart              = %u", timeSyncVars_.timeSetStart          );
   INFO_printf("TimeSource                = %u", timeSyncVars_.timeSource            );

   CPUretVal  = TIME_SYS_GetRealCpuFreq( &CPUfreq, &CPUsource, &CPUlastUpdate );

   for ( radioNum = (uint8_t)RADIO_0; radioNum < (uint8_t)MAX_RADIO; radioNum++ ) {
      TCXOretVal = RADIO_TCXO_Get( radioNum, &TCXOfreq, &TCXOsource, &TCXOlastUpdate );
      TIME_UTIL_ConvertSecondsToSysFormat( TCXOlastUpdate, 0, &sysTime );
      (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sysTime, &sysDateTime );
      INFO_printf("radio %u TCXO  (ref. %s) = %u  (%15s)(Last updated %02u/%02u/%04u %02u:%02u:%02u)",
         radioNum,
         TIME_SYS_GetSourceMsg( TCXOsource ),
         TCXOfreq,
         TCXOretVal==(bool)true?"up to date":"stale frequency",
         sysDateTime.month, sysDateTime.day, sysDateTime.year, sysDateTime.hour, sysDateTime.min, sysDateTime.sec);
   }

   TIME_UTIL_ConvertSecondsToSysFormat(CPUlastUpdate, 0, &sysTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat( &sysTime, &sysDateTime);

   INFO_printf("CPU frequency (ref. %s) = %u (%15s)(Last updated %02u/%02u/%04u %02u:%02u:%02u)",
      TIME_SYS_GetSourceMsg( CPUsource ),
      CPUfreq,
      CPUretVal==(bool)true?"up to date":"stale frequency",
      sysDateTime.month, sysDateTime.day, sysDateTime.year, sysDateTime.hour, sysDateTime.min, sysDateTime.sec);

#if ( DCU == 1 )
   if (VER_getDCUVersion() != eDCU2)
   {
      INFO_printf("GPS Phase error (nsec)    = %d", TIME_SYS_PhaseErrorNsec()           );
      INFO_printf("isGPSPresent              = %u", TIME_SYS_IsGpsPresent()             );
      INFO_printf("isGPSTimeValid            = %u", TIME_SYS_IsGpsTimeValid()           );
   }
#endif
}

/***********************************************************************************************************************
 * This function handles OTA get/set of OR PM params
 *  Arguments:  action-> set or get
 *              id    -> HEEP enum associated with the value
 *              value -> pointer to new value to be placed in file
 *              attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 * *********************************************************************************************************************/
returnStatus_t TIME_SYNC_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case timePrecision :
         {
            if ( sizeof(int8_t) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(int8_t *)value = TIME_SYNC_TimePrecision_Get();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(int8_t);
                  attr->rValTypecast = (uint8_t)intValue;
               }
            }
            break;
         }
         case timeDefaultAccuracy :
         {
            if ( sizeof(timeSyncVars_.timeDefaultAccuracy) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(int8_t *)value = TIME_SYNC_TimeDefaultAccuracy_Get();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeSyncVars_.timeDefaultAccuracy);
                  attr->rValTypecast = (uint8_t)intValue;
               }
            }
            break;
         }
         case timeQueryResponseMode :
         {
            if ( sizeof(timeSyncVars_.timeQueryResponseMode) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint8_t *)value = TIME_SYNC_TimeQueryResponseMode_Get();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeSyncVars_.timeQueryResponseMode);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case timeSetMaxOffset :
         {
            if ( sizeof(timeSyncVars_.timeSetMaxOffset) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = TIME_SYNC_TimeSetMaxOffset_Get();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeSyncVars_.timeSetMaxOffset);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case timeSetPeriod :
         {
            if ( sizeof(timeSyncVars_.timeSetPeriod) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint32_t *)value = TIME_SYNC_TimeSetPeriod_Get();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeSyncVars_.timeSetPeriod);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case timeSetStart :
         {
            if ( sizeof(timeSyncVars_.timeSetStart) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint32_t *)value = TIME_SYNC_TimeSetStart_Get();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeSyncVars_.timeSetStart);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case timeSource :
         {
            if ( sizeof(timeSyncVars_.timeSource) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint8_t *)value = TIME_SYNC_TimeSource_Get();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeSyncVars_.timeSource);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      } /*end switch *//*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case timeDefaultAccuracy :
         {
            retVal = TIME_SYNC_TimeDefaultAccuracy_Set(*(int8_t *)value);
            break;
         }
         case timeQueryResponseMode :
         {
            retVal = TIME_SYNC_TimeQueryResponseMode_Set(*(uint8_t *)value);
            break;
         }
         case timeSetMaxOffset :
         {
            retVal = TIME_SYNC_TimeSetMaxOffset_Set(*(uint16_t *)value);
            break;
         }
         case timeSetPeriod :
         {
            retVal = TIME_SYNC_TimeSetPeriod_Set(*(uint32_t *)value);
            break;
         }
         case timeSetStart :
         {
            retVal = TIME_SYNC_TimeSetStart_Set(*(uint32_t *)value);
            break;
         }
         case timeSource :
         {
            retVal = TIME_SYNC_TimeSource_Set(*(uint8_t *)value);
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      } /*end switch*//*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}
/*****************************************************************************************************************
 *
 * Function name: TIME_SYNC_AlaramID_Get
 *
 * Purpose: To get the Time Sync Periodic Timer/Alaram ID
 *
 * Arguments: None
 *
 * Returns: Alaram ID
 *
 * Reentrant: NO.
 *
 ******************************************************************************************************************/
uint8_t TIME_SYNC_PeriodicAlaramID_Get ( void )
{
   return timeSyncAlarmID_;
}