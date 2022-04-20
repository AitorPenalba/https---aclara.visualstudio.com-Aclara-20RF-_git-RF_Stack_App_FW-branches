/***********************************************************************************************************************

   Filename:   pwr_restore.c

   Global Designator: PWROR_

   Contents: Outage restoration message build and transmission.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author David McGraw

   $Log$

 ***********************************************************************************************************************
   Revision History:

   @version
   #since
  *********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdbool.h>
#include <string.h>
#include "compiler_types.h"
#include "error_codes.h"
#include "App_Msg_Handler.h"
#include "DBG_SerialDebug.h"
#include "intf_cim_cmd.h"
#include "buffer.h"
#include "time_util.h"
#include "pwr_last_gasp.h"
#include "vbat_reg.h"
#include "pwr_task.h"
#include "pwr_config.h"
#include "pwr_last_gasp.h"
#include "EVL_event_log.h"
#include "time_sys.h"
#include "mode_config.h"

#define PWROR_GLOBALS
#include "pwr_restore.h"
#undef PWROR_GLOBALS

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */
/* cannot find a requirement for these reading types */
static const meterReadingType readingTypes[] =
{
#if ( COMMERCIAL_METER == 0 )
#if ACLARA_DA == 1 || ACLARA_LC == 1
   powerQuality
#else
   energization, powerQuality, vRMS
#endif
#elif ( COMMERCIAL_METER == 1 )
   energization, powerQuality, vRMSA, vRMSB, vRMSC
#else
   energization, powerQuality, vRMSA, vRMSC
#endif
};

static const uint8_t uReadingCount = ARRAY_IDX_CNT( readingTypes );

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static OS_SEM_Obj PWROR_HMC_Sem;    /* Used to signal task that host meter communication has been initialized. */
static OS_SEM_Obj PWROR_PWR_Sem;    /* Used to signal task that power task has been initialized. */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function name: PWROR_Task

   Purpose: Send outage restoration message at MTU startup after power last gasp has declared an outage.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: void

   Re-entrant Code: No_st

   Notes:

 **********************************************************************************************************************/
/*lint -esym(715,Arg0)  not referenced but needed by generic API  */
void PWROR_Task( uint32_t Arg0 )
{
   sysTimeCombined_t    timeRestorationSysComb = 0; /* System time at entry to this task   */
   sysTimeCombined_t    outageTimeSysComb;
   MQX_TICK_STRUCT      endTime;             /* Used to compute elapsed time since restoration.  */
   MQX_TICK_STRUCT      startTime;           /* Used to compute elapsed time since restoration.  */
   int32_t              validRTCDeltaSec;       /* delta between restoration and valid RTC in Ticks  */
   pwrFileData_t const  *pwrFileData;        /* Pointer to pwrFileData  */
   uint32_t             uSleepMilliseconds;  /* Random hold off time before sending restoration message. */
   uint32_t             fracSecs;
   uint32_t             outageTimeSec;       /* Outage time in seconds */
   uint32_t             timeRestorationSec;  /* Restoratation time in seconds */
   uint32_t             pwrEventDurationSec;    /* Duration of power event used to determine the kind of event */
   bool                 bOverflow;           /* Used in elapsed time calculation.   */
   bool                 bRTCValid;           /* Used in elapsed time calculation.   */

   DBG_logPrintf( 'I', "Running" );
   OS_TICK_Get_CurrentElapsedTicks( &startTime );

   DBG_logPrintf( 'I', "Waiting for PWR_Task" );
   ( void )OS_SEM_Pend( &PWROR_PWR_Sem, ONE_SEC * 60 );

   if ( e_MODECFG_NORMAL_MODE == MODECFG_get_initialCFGMode() )
   {
      // If Last Gasp declared and outage.
      // TODO 2016-02-02 SMG Change these to calls into PWRLG and VBATREG modules
      if ( ( 0 != PWRLG_OUTAGE() ) || ( 0 != VBATREG_SHORT_OUTAGE ) )
      {

#if ( ACLARA_LC == 0 )
         /* Wait up to 1 minute for host meter communincation to be established, if after 1 minute it has not been
            established, go ahead and try to send the outage restoration anyway. */
         ( void )OS_SEM_Pend( &PWROR_HMC_Sem, ONE_SEC * 60 );
#endif

         /* Wait for a valid RTC. Currently we wait indefinitely, but this might be changed per systems group. */
         DBG_logPrintf( 'I', "Outage or PQE declared: Waiting for valid RTC" );

         do
         {
            bRTCValid = RTC_Valid();

            if ( bRTCValid )
            {
               if ( eSUCCESS == TIME_UTIL_GetTimeInSysCombined( &timeRestorationSysComb ) )
               {
                  OS_TICK_Get_CurrentElapsedTicks( &endTime ); /* Get "current" time   */
                  /* Compute seconds between entering this routine and now. This accounts for time to get signal from
                     PWR_task and waiting for RTC time valid and HMC startup.   */
                  validRTCDeltaSec = _time_diff_seconds( &endTime, &startTime, &bOverflow );
                  /* Restoration time was time at RTC valid. Subtract seconds from enterting task until now.   */
                  timeRestorationSysComb -= ( sysTimeCombined_t )( int64_t )validRTCDeltaSec * TIME_TICKS_PER_SEC;
               }
               else
               {
                  bRTCValid = false;
               }
            }

            if ( !bRTCValid )
            {
               OS_TASK_Sleep( ONE_SEC );
            }
         }while ( !bRTCValid );

         if ( RESTORATION_TIME() != 0 )      /* If RAM restoration time is valid, use it. */
         {
            TIME_UTIL_ConvertSecondsToSysCombined( RESTORATION_TIME(), 0, &timeRestorationSysComb );
         }

         pwrFileData = PWR_getpwrFileData();
         outageTimeSysComb = pwrFileData->outageTime;
         TIME_UTIL_ConvertSysCombinedToSeconds( &(outageTimeSysComb), &outageTimeSec, &fracSecs);
         TIME_UTIL_ConvertSysCombinedToSeconds( &timeRestorationSysComb, &timeRestorationSec, &fracSecs);
         pwrEventDurationSec = ( timeRestorationSec - outageTimeSec );
         DBG_logPrintf( 'I', "Power Event Duration %u seconds", pwrEventDurationSec);

         /* We know an outage or Power Quality Event has been declared.   */
         /* Need to log an outage/restoration.  */
         EventData_s          pwrQualEvent;  /* Event info  */
         EventKeyValuePair_s  keyVal[ 4 ];   /* Event key/value pair info  */

         if ( ( 0 != VBATREG_SHORT_OUTAGE ) ||
              ( pwrEventDurationSec <= PWRCFG_get_outage_delay_seconds() ) )    /* PQE   */
         {
            DBG_logPrintf( 'I', "PQE declared" );
            PWROR_printOutageStats( outageTimeSysComb, timeRestorationSysComb );

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
               OS_TASK_Sleep( sleepTime );
            }

            /* We need to save multiple events to NV memory and update last gasp status flags.  A power down during
               this process can create a misalignment of the events logged along with the last gasp status.  To guard
               against this, lock the power mutex with the EMBEDDED_PWR_MUTEX mutex state. */
            PWR_lockMutex( EMBEDDED_PWR_MUTEX );// Function will not return if it fails

            pwrQualEvent.eventId = ( uint16_t )comDevicePowerPowerQualityEventStarted;
            *( uint16_t * )keyVal[ 0 ].Key = 0;
            *( uint16_t * )keyVal[ 0 ].Value = 0;
            pwrQualEvent.eventKeyValuePairsCount = 0;
            pwrQualEvent.eventKeyValueSize = 0;
            pwrQualEvent.timestamp = outageTimeSec; //not technically an outage
            ( void )EVL_LogEvent( 90, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, NULL );

            /* PQE stopped event */
            pwrQualEvent.eventId = ( uint16_t )comDevicePowerPowerQualityEventStopped;
            *( uint16_t * )keyVal[ 0 ].Key = ( uint16_t )powerQuality;
            *( uint16_t * )keyVal[ 0 ].Value = pwrFileData->uPowerAnomalyCount;
            pwrQualEvent.eventKeyValuePairsCount = 1;
            pwrQualEvent.eventKeyValueSize = 2;
            pwrQualEvent.timestamp = timeRestorationSec;
            ( void )EVL_LogEvent( 89, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, NULL );

            PWRLG_RestLastGaspFlags(); // power quality event has completed, clear the last gasp flags

            /* All updates to NV and last gasp flags have completed.  Release the EMBEDDED_PWR_MUTEX state of the
               power mutex. */
            PWR_unlockMutex( EMBEDDED_PWR_MUTEX );
         }
         else  /* Power Outage.  */
         {
            DBG_logPrintf( 'I', "Outage declared" );

            pwrQualEvent.markSent = (bool)true;

#if ( ENABLE_HMC_TASKS == 1 )
            /* If the endpoint is connected to a meter, we need to gather voltage measurements from the meter
               to be stored as NVP data in the power restored event.  Make sure the HMC is ready to go before
               attempting to retrieve data out of the meter. */
            while ( !HMC_MTRINFO_Ready() )
            {
               OS_TASK_Sleep( 100 );
            }
#endif
            PWROR_printOutageStats( outageTimeSysComb, timeRestorationSysComb );

            /* We need to save multiple events to NV memory and update last gasp status flags.  A power down during
               this process can create a misalignment of the events logged along with the last gasp status.  To guard
               against this, lock the power mutex with the EMBEDDED_PWR_MUTEX mutex state. */
            PWR_lockMutex( EMBEDDED_PWR_MUTEX );// Function will not return if it fail

             //PWRCFG_get_outage_delay_seconds()

            /* Power failed event   */
            pwrQualEvent.eventId = ( uint16_t )comDevicePowerFailed;
            *( uint16_t * )keyVal[ 0 ].Key = 0;
            *( uint16_t * )keyVal[ 0 ].Value = 0;
            pwrQualEvent.eventKeyValuePairsCount = 0;
            pwrQualEvent.eventKeyValueSize = 0;
#if 0
            pwrQualEvent.timestamp = PWRLG_TIME_OUTAGE();
#else
            pwrQualEvent.timestamp = outageTimeSec;
#endif
            LoggedEventDetails outageLoggedEventData;
            ( void )EVL_LogEvent( 200, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, &outageLoggedEventData );

            /* Power restored event */
            pwrQualEvent.eventId = ( uint16_t )comDevicePowerRestored;
            *( uint16_t * )keyVal[ 0 ].Key = 0;
            *( uint16_t * )keyVal[ 0 ].Value = 0;
            pwrQualEvent.eventKeyValuePairsCount = 0;
            pwrQualEvent.eventKeyValueSize = 0;
            pwrQualEvent.timestamp = timeRestorationSec;
            LoggedEventDetails restLoggedEventData;
            ( void )EVL_LogEvent( 199, &pwrQualEvent, keyVal, TIMESTAMP_PROVIDED, &restLoggedEventData );

            PWRLG_RestLastGaspFlags(); // outage event has completed, clear the last gasp flags

            /* All updates to NV and last gasp flags have completed.  Release the EMBEDDED_PWR_MUTEX state of the
               power mutex. */
            PWR_unlockMutex( EMBEDDED_PWR_MUTEX );

            // Sleep for a random delay period.
            uSleepMilliseconds = aclara_randu( 0, EVL_getAmBuMaxTimeDiversity() * TIME_TICKS_PER_MIN );
            DBG_logPrintf( 'I', "Sleeping %u Milliseconds", uSleepMilliseconds );
            OS_TASK_Sleep( uSleepMilliseconds );

            /*lint !e644 timeRestorationSysComb is init'd in the do loop above */
            ( void )PWROR_tx_message( outageTimeSysComb, &outageLoggedEventData, timeRestorationSysComb, &restLoggedEventData );
         }
      }
      else
      {
         DBG_logPrintf( 'I', "Outage NOT Declared." );
      }
   }
   DBG_logPrintf( 'I', "Exiting." );

   /* TODO: While "returning" from a task automatically kills the task under MQX, this may require different handling
      under a different OS.  */
}
/*lint +esym(715,Arg0)  not referenced but needed by generic API  */

/***********************************************************************************************************************

   Function name: PWROR_init

   Purpose: Adds registers to the Object list and opens files.  This must be called at power before any other function
            in this module is called.

   Arguments: None

   Returns: returnStatus_t - result of file open.

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
returnStatus_t PWROR_init( void )
{
   returnStatus_t retVal = eFAILURE;
   //TODO RA6: NRJ: determine if semaphores need to be counting
   if ( ( true == OS_SEM_Create( &PWROR_HMC_Sem, 0 ) ) && ( true == OS_SEM_Create( &PWROR_PWR_Sem, 0 ) ) )
   {
      retVal = eSUCCESS;
   }

   return( retVal );
}

/***********************************************************************************************************************

   Function name: PWROR_HMC_signal

   Purpose: Posts the outage restoration semaphore to indicate that host meter communication has been initialized.

   Arguments: None

   Returns: None

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWROR_HMC_signal( void )
{
   OS_SEM_Post( &PWROR_HMC_Sem ); // Function will not return if it fails
}

/***********************************************************************************************************************

   Function name: PWROR_PWR_signal

   Purpose: Posts the outage restoration semaphore to indicate that power task has been initialized.

   Arguments: None

   Returns: None

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWROR_PWR_signal( void )
{
   OS_SEM_Post( &PWROR_PWR_Sem ); // Function will not return if it fails
}
/***********************************************************************************************************************

   Function name: PWROR_printOutageStats

   Purpose: Simple utility to print outage and restoration date/times in human readable format

   Arguments: sysTimeCombined_t  outageTime
              sysTimeCombined_t  timeRestorationSysComb

   Returns: None

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void PWROR_printOutageStats( sysTimeCombined_t outageTime, sysTimeCombined_t timeRestorationSysComb )
{
   sysTime_t            rTime;   /* Time to convert   */
   sysTime_dateFormat_t dt;      /* Converted time    */

   TIME_UTIL_ConvertSysCombinedToSysFormat( &outageTime, &rTime );
   ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &rTime, &dt );
   /*lint -e(123) 'min' is a struct element, not a macro */
   DBG_logPrintf( 'I', "Outage time:      %04d/%02d/%02d %02d:%02d:%02d",
                  dt.year, dt.month, dt.day,
                  dt.hour, dt.min,   dt.sec );

   TIME_UTIL_ConvertSysCombinedToSysFormat( &timeRestorationSysComb, &rTime );
   ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &rTime, &dt );
   /*lint -e(123) 'min' is a struct element, not a macro */
   DBG_logPrintf( 'I', "Restoration time: %04d/%02d/%02d %02d:%02d:%02d",
                  dt.year, dt.month, dt.day,
                  dt.hour, dt.min,   dt.sec );
}

/***********************************************************************************************************************

   Function name: PWROR_tx_message

   Purpose: Send power restoration message to head end.

   Arguments: sysTimeCombined_t outageTime              - the time the outage occured
              const LoggedEventDetails *outageEventData - event details of the com device outage event
			  sysTimeCombined_t timeRestorationSysComb         - the time the restoration occured
			  const LoggedEventDetails *restEventData   - event details of the com device restoration event

   Returns: Success/failure of transmission

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
returnStatus_t PWROR_tx_message( sysTimeCombined_t outageTime, const LoggedEventDetails *outageEventData,
                                        sysTimeCombined_t timeRestorationSysComb, const LoggedEventDetails *restEventData )
{
   buffer_t             *pMessage;
   pack_t               packCfg;                // Struct needed for PACK functions
   //pFullMeterRead       payload;                // Pointer to response payload
   fmrRdgValInfo_t      rdgValInfo;
   returnStatus_t       returnStatus = eFAILURE;
   sysTimeCombined_t    timeCombinedMessage;
   EventRdgQty_u        EventRdgQty;            // Count of events/readings in the payload
   uint16_t             uEventType = ( uint16_t )0;
   uint32_t             timeMessage;
   uint32_t             timeRestoration;
   uint32_t             timeOutage;
   uint8_t              eventCount = 0;         // Counter for alarms and readings
   uint8_t              i;
   enum_CIM_QualityCode qualCode;

   pMessage = BM_alloc( VALUES_INTERVAL_END_SIZE                   +
                        EVENT_AND_READING_QTY_SIZE                 +
                        ( ED_EVENT_CREATED_DATE_TIME_SIZE * 2 )    +
                        ( ED_EVENT_TYPE_SIZE * 2 )                 +
                        ( NVP_DET_VALUE_SIZE * 2 )                 +
                        ( ( sizeof(nvp_t) ) * 2 )                  +
                        ( sizeof( readingQty_t ) * uReadingCount ) );

   if ( pMessage != NULL )
   {
      HEEP_APPHDR_t heepHdr;

      ( void )TIME_UTIL_GetTimeInSysCombined( &timeCombinedMessage );
      timeMessage = ( uint32_t )( timeCombinedMessage / TIME_TICKS_PER_SEC );

      timeOutage = ( uint32_t )( outageTime / TIME_TICKS_PER_SEC );
      timeRestoration = ( uint32_t )( timeRestorationSysComb / TIME_TICKS_PER_SEC );

      // Prepare a message to the head end
      // Application header/QOS info
      HEEP_initHeader( &heepHdr );
      heepHdr.TransactionType   = ( uint8_t )TT_ASYNC;
      heepHdr.Resource          = ( uint8_t )bu_am;
      heepHdr.Method_Status     = ( uint8_t )method_post;
      heepHdr.Req_Resp_ID       = 0;
      heepHdr.qos               = 0x3c;
      heepHdr.callback          = NULL;

      // initialize the pack structure
      PACK_init( (uint8_t*)&pMessage->data[0], &packCfg );

      // valuesInterval.end = current time
      PACK_Buffer(-(int16_t)sizeof( timeMessage ) * 8, ( uint8_t * )&timeMessage, &packCfg);
      pMessage->x.dataLen = sizeof( timeMessage );

      // Event and Reading Quantity, will be updated later if addition readings are added
      EventRdgQty.bits.eventQty = 0;
      EventRdgQty.bits.rdgQty = 0;
      PACK_Buffer( (int16_t)sizeof( EventRdgQty ) * 8, ( uint8_t * )&EventRdgQty, &packCfg );
      pMessage->x.dataLen += sizeof( EventRdgQty );

      // EndDeviceEvents.createdDateTime = Outage time
      PACK_Buffer(-(int16_t)sizeof( timeOutage ) * 8, ( uint8_t * )&timeOutage, &packCfg);
      pMessage->x.dataLen += sizeof( timeOutage );

      // EndDeviceEvents.EndDeviceEventType = comDevice.power..Failed
#if ( LAST_GASP_SIMULATION == 1 )
      if ( EVL_GetLGSimState() == eSimLGDisabled )
      {
#endif
         uEventType = ( uint16_t )comDevicePowerFailed;
#if ( LAST_GASP_SIMULATION == 1 )
      }
      else
      {
         uEventType = ( uint16_t )comDevicepowertestfailed;
      }
#endif
      PACK_Buffer(-(int16_t)sizeof( uEventType ) * 8, ( uint8_t * )&uEventType, &packCfg);
      pMessage->x.dataLen += sizeof( uEventType );

      // Add NVP description details such as nvp quantity and size of data
      union nvpQtyDetValSz_u nvpDesc;
      nvpDesc.bits.nvpQty = 1;
      nvpDesc.bits.DetValSz = 1;
      PACK_Buffer( (int16_t)sizeof( nvpDesc ) * 8, ( uint8_t * )&nvpDesc, &packCfg );
      pMessage->x.dataLen += sizeof( nvpDesc );

      // need to alarm index information to the nvp data
      nvp_t               nvpData;
      nvpData.nvpName     = ( uint16_t )outageEventData->alarmIndex;
      nvpData.nvpValue[0] = ( int8_t )outageEventData->indexValue;
      PACK_Buffer( -(int16_t)sizeof( nvpData.nvpName ) * 8, ( uint8_t * )&nvpData.nvpName, &packCfg );
      pMessage->x.dataLen += sizeof( nvpData.nvpName );
      PACK_Buffer( (int16_t)nvpDesc.bits.DetValSz * 8, ( uint8_t * )&nvpData.nvpValue[0], &packCfg );
      pMessage->x.dataLen += nvpDesc.bits.DetValSz;

      // EndDeviceEvents.createdDateTime = Restoration time
      PACK_Buffer(-(int16_t)sizeof( timeRestoration ) * 8, ( uint8_t * )&timeRestoration, &packCfg);
      pMessage->x.dataLen += sizeof( timeRestoration );

      // EndDeviceEvents.EndDeviceEventType = comDevice.power..restored
#if ( LAST_GASP_SIMULATION == 1 )
      if ( EVL_GetLGSimState() == eSimLGDisabled )
      {
#endif
         uEventType = ( uint16_t )comDevicePowerRestored;
#if ( LAST_GASP_SIMULATION == 1 )
      }
      else
      {
         uEventType = ( uint16_t )comDevicePowertestrestored;
      }
#endif
      PACK_Buffer(-(int16_t)sizeof( uEventType ) * 8, ( uint8_t * )&uEventType, &packCfg);
      pMessage->x.dataLen += sizeof( uEventType );

      // Add NVP description details such as nvp quantity and size of data
      nvpDesc.bits.nvpQty = 1;
      nvpDesc.bits.DetValSz = 1;
      PACK_Buffer( (int16_t)sizeof( nvpDesc ) * 8, ( uint8_t * )&nvpDesc, &packCfg );
      pMessage->x.dataLen += sizeof( nvpDesc );

      // need to alarm index information to the nvp data
      nvpData.nvpName     = ( uint16_t )restEventData->alarmIndex;
      nvpData.nvpValue[0] = ( int8_t )restEventData->indexValue;
      PACK_Buffer( -(int16_t)sizeof( nvpData.nvpName ) * 8, ( uint8_t * )&nvpData.nvpName, &packCfg );
      pMessage->x.dataLen += sizeof( nvpData.nvpName );
      PACK_Buffer( (int16_t)nvpDesc.bits.DetValSz * 8, ( uint8_t * )&nvpData.nvpValue[0], &packCfg );
      pMessage->x.dataLen += nvpDesc.bits.DetValSz;

      // Fill in the reading values.
      for ( i = 0; i < uReadingCount; i++ )
      {
         ValueInfo_t reading; // declare variable to retrieve host meter readings
         qualCode = CIM_QUALCODE_SUCCESS;

         if (CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading(readingTypes[i], &reading))
         {
            uint8_t powerOf10Code;
            uint8_t numberOfBytes;

            powerOf10Code = HEEP_getPowerOf10Code(reading.power10, (int64_t *)&reading.Value.intValue);
            numberOfBytes = HEEP_getMinByteNeeded( reading.Value.intValue, reading.typecast, reading.valueSizeInBytes);


            DBG_printf("Ch=%2d, val=%lld, power=%d, NumBytes=%d", i, reading.Value.intValue, powerOf10Code,
                        numberOfBytes);

            ++eventCount;
            rdgValInfo.pendPowerof10 = powerOf10Code;
            rdgValInfo.rsvd2 = 0;
            rdgValInfo.RdgQualPres = 0;
            rdgValInfo.tsPresent = 0;
            rdgValInfo.isCoinTrig = 0;

#if ENABLE_HMC_TASKS == 1
            if( INTF_CIM_CMD_errorCodeQCRequired( readingTypes[i], reading.Value.intValue ) )
            {  // The value returned is an information code, need to add a quality code to message
               rdgValInfo.RdgQualPres = 1;
               qualCode = CIM_QUALCODE_ERROR_CODE;
            }
#endif

            rdgValInfo.RdgValSizeU = 0;
            rdgValInfo.typecast = ( uint16_t )reading.typecast;
#if OTA_CHANNELS_ENABLED == 0
            rdgValInfo.rsvd1 = 0;
#endif
            rdgValInfo.RdgValSize = numberOfBytes;

            rdgValInfo.rdgType = BIG_ENDIAN_16BIT( readingTypes[i] );  //Byte swap
            PACK_Buffer( ( int16_t )sizeof( rdgValInfo ) * 8, ( uint8_t * )&rdgValInfo, &packCfg );
            pMessage->x.dataLen += sizeof( rdgValInfo );

            if( CIM_QUALCODE_SUCCESS != qualCode )
            {  // pack the quality code value into the message
               PACK_Buffer( ( int16_t )sizeof( qualCode ) * 8, (uint8_t *)&qualCode, &packCfg );
               pMessage->x.dataLen += sizeof( qualCode );
            }

            PACK_Buffer( -( int16_t )( numberOfBytes * 8 ), (uint8_t *)&reading.Value.uintValue, &packCfg );
            pMessage->x.dataLen += numberOfBytes;
         }
      }

      //Insert the correct number of readings
      EventRdgQty.bits.eventQty = 2;
      EventRdgQty.bits.rdgQty = eventCount;
      pMessage->data[VALUES_INTERVAL_END_SIZE] = EventRdgQty.EventRdgQty;

      // send the message to message handler. The called function will free the buffer
      returnStatus = HEEP_MSG_Tx( &heepHdr, pMessage );
   }

   return( returnStatus );
}
