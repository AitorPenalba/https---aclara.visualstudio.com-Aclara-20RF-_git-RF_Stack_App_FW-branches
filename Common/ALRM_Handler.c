/******************************************************************************

   Filename: ALRM_Handler.c

   Global Designator:

   Contents:

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <string.h>
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#endif
#include "time_util.h"
#include "pack.h"
#include "EVL_event_log.h"
#include "HEEP_util.h"
#include "ALRM_Handler.h"
#include "APP_MSG_Handler.h"
#include "pack.h"
#include "buffer.h"
#include "file_io.h"
#include "Temperature.h"
#include "DFW_App.h"
#if ( EP == 1 ) && ( LOG_IN_METER == 1 )
#include "PWR_Task.h"
#endif
#if ( EP == 1 )
#include "PHY.h"
#if ( CLOCK_IN_METER == 1 )
#include "hmc_time.h"
#endif
#include "SM_Protocol.h"
#include "SM.h"
#include "intf_cim_cmd.h"
#if ENABLE_METER_EVENT_LOGGING != 0
#if ( ( METER_TROUBLE_SIGNAL == 1 ) && ( ANSI_STANDARD_TABLES == 1 ) )
#include "hmc_diags.h"     /* For meter events file info.   */
#endif
#endif
#if ( ACLARA_LC == 0 ) && (ACLARA_DA == 0)
#include "hmc_mtr_info.h"
#endif
#endif
#include "DBG_SerialDebug.h"

#if ( DCU == 1)
#define LOG_IN_METER         0
#define ANSI_STANDARD_TABLES 0
#define METER_TROUBLE_SIGNAL 0
#endif

#define TM_ALRM_UNIT_TEST 0
#if TM_ALRM_UNIT_TEST
#warning "Don't release with TM_ALRM_UNIT_TEST set"   /*lint !e10 !e16 #warning is not recognized  */
#include <stdio.h>
#include "DBG_SerialDebug.h"
static uint8_t  alrmPbuff[ 64 ];
#endif

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */
/*lint -esym(750,ALRM_PRNT_INFO,ALRM_PRNT_HEX_INFO)   */
/*lint -esym(750,ALRM_PRNT_WARN,ALRM_PRNT_HEX_WARN)   */
/*lint -esym(750,ALRM_PRNT_ERROR,ALRM_PRNT_HEX_ERROR)   */

#if ( ENABLE_PRNT_ALRM_INFO == 1 )
#define ALRM_PRNT_INFO( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define ALRM_PRNT_HEX_INFO( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define ALRM_PRNT_INFO( a, fmt,... )
#define ALRM_PRNT_HEX_INFO( a, fmt,... )
#endif

#if ( ENABLE_PRNT_ALRM_WARN == 1 )
#define ALRM_PRNT_WARN( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define ALRM_PRNT_HEX_WARN( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define ALRM_PRNT_WARN( a, fmt,... )
#define ALRM_PRNT_HEX_WARN( a, fmt,... )
#endif

#if ( ENABLE_PRNT_ALRM_ERROR == 1 )
#define ALRM_PRNT_ERROR( a, fmt,... )     DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define ALRM_PRNT_HEX_ERROR( a, fmt,... ) DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define ALRM_PRNT_ERROR( a, fmt,... )
#define ALRM_PRNT_HEX_ERROR( a, fmt,... )
#endif

/* TYPE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */

static FileHandle_t  fileHndlEvent_;      /* Contains the registration file handle information. */

#if ( ( EP == 1 ) && ( METER_TROUBLE_SIGNAL == 1 ) && ( ANSI_STANDARD_TABLES == 1 ) )
#if ( ENABLE_METER_EVENT_LOGGING != 0 )
#if ( LOG_IN_METER == 1 )
static OS_SEM_Obj    MeterTroubleSem;                    /* Semaphore used by trouble signal isr to announce meter event.  */
static uint8_t       updateLastReadData_[ 3 ];           /* Holds procedure parameters:
                                                            1st entry (offset 0) = 1 for event log
                                                            2nd entry  offset 1, 2 bytes) = last read entry */
static bool          clearEventLogs_ = ( bool ) false;   /* If set, clear the event logs in the meter */
static bool          eventLogging;                       /* Read from MFG table 0 upgrades section */
static bool          voltageEventMonitor;                /* Read from MFG table 0 upgrades section */
#endif
#endif
#endif

/* FUNCTION PROTOTYPES */
#if ( ( EP == 1 ) && ( METER_TROUBLE_SIGNAL == 1 ) && ( ANSI_STANDARD_TABLES == 1 ) )
#if ( ENABLE_METER_EVENT_LOGGING != 0 )
#if ( LOG_IN_METER == 1 )
#if ( MCU_SELECTED == NXP_K24 )
static void       meter_trouble_isr_busy( void );
#endif
#endif
#endif
#endif

/* CONSTANTS */

/* FUNCTION DEFINITIONS */


#if ( MCU_SELECTED == RA6E1 )
/***********************************************************************************************************************

   Function Name: meter_trouble_isr_init

   Purpose: Initialize and configure the meter trouble is(meter_trouble_isr) on RA6E1

   Arguments: None

   Returns: fsp_err_t

   Reentrant Code: No

 **********************************************************************************************************************/
static fsp_err_t meter_trouble_isr_init( void )
{
   fsp_err_t err = FSP_SUCCESS;

   /* Open external IRQ/ICU module */
   err = R_ICU_ExternalIrqOpen( &hmc_trouble_busy_ctrl, &hmc_trouble_busy_cfg );
   if(FSP_SUCCESS == err)
   {
      /* Enable ICU module */
      DBG_printf("\nOpen Meter Trouble Busy IRQ");
      err = R_ICU_ExternalIrqEnable( &hmc_trouble_busy_ctrl );
   }
   return err;
}
#endif

/*******************************************************************************

   Function name: ALRM_init

   Purpose: Initialize all of the necessary data for the ALRM module before the
            tasks start

   Arguments:

   Returns: retVal - indicates success or the reason for error

   Notes:

*******************************************************************************/
returnStatus_t ALRM_init ( void )
{
   /*lint -esym(438,peventHistory) last value not used*/
   returnStatus_t  retVal;  /* Return status  */
   FileStatus_t    fileStatus;
   eventHistory_t *peventHistory; /* Pointer to the event history data type             */

   /* Note: if FIO_fopen creates the file, its contents are set to 0. No need to initialize the contents!   */
   retVal = FIO_fopen( &fileHndlEvent_, ePART_NV_APP_BANKED, ( uint16_t )eFN_EVENTS,
                       ( lCnt )sizeof( eventHistory_t ), FILE_IS_NOT_CHECKSUMED,
                       0, &fileStatus );
   if ( eSUCCESS == retVal )
   {
      /* Not used.  Included so DFW file offset extraction has visablity of the struct typedef */
      peventHistory = NULL;
      peventHistory = peventHistory;   /* Keep compiler warning quite about being set but not used */
   }

#if ( EP == 1 )
#if ( ENABLE_METER_EVENT_LOGGING != 0 )
#if ( ( METER_TROUBLE_SIGNAL == 1 ) && ( ANSI_STANDARD_TABLES == 1 ) )
   if ( eSUCCESS == retVal )
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      if ( OS_SEM_Create( &MeterTroubleSem, 0 ) )
      {
#if ( RTOS_SELECTION == MQX_RTOS )
         /* Set up the ISR for Meter trouble signal. This is only on some meters.   */
         if ( NULL != _int_install_isr( HMC_TROUBLE_IRQIsrIndex, ( INT_ISR_FPTR )meter_trouble_isr_busy, NULL ) )
         {
            if ( MQX_OK == _bsp_int_init( HMC_TROUBLE_IRQIsrIndex,
                                          HMC_TROUBLE_ISR_PRI, HMC_TROUBLE_ISR_SUB_PRI, ( bool )true ) )
            {
               HMC_TROUBLE_TRIS();           /* Enable the pin as an GPIO input. */
               HMC_TROUBLE_BUSY_IRQ_EI();    /* Enable the ISR */
            }
         }
#elif ( RTOS_SELECTION == FREE_RTOS )
         meter_trouble_isr_init();
#endif
      }
      else
      {
         retVal = eFAILURE;
      }
   }
#endif
#endif
#endif

   return ( retVal );
}
#if ( EP == 1 )
#if ( ENABLE_METER_EVENT_LOGGING != 0 )
/*******************************************************************************

   Function name: ALRM_clearAllAlarms

   Purpose: Reset all alarms. Called from power task when ship bit set.

   Arguments: none

   Returns: returnStatus_t success /failure of operation

   Notes:

*******************************************************************************/
returnStatus_t ALRM_clearAllAlarms ( void )
{
   buffer_t       *pEvent;       /* Pointer to copy of event history file     */
   returnStatus_t retVal = eFAILURE;

   pEvent = BM_alloc( sizeof ( eventHistory_t ) );

   if ( pEvent != NULL )
   {
      ( void )memset ( pEvent->data, 0, sizeof( eventHistory_t ) );
      retVal = FIO_fwrite( &fileHndlEvent_, 0, pEvent->data, ( lCnt )sizeof( eventHistory_t ) );
      BM_free( pEvent );
   }
#if ( LOG_IN_METER == 1 )
   clearEventLogs_ = ( bool )true;
#endif
   return retVal;
}

#if ( LOG_IN_METER == 1 )
/*******************************************************************************

   Function name: clearEventLogs

   Purpose: Reset event monitor list pointers in the meter.

   Arguments: none

   Returns: None

   Notes:

*******************************************************************************/
static void clearEventLogs( void )
{
   /*lint -esym(438,procResult) last value not used*//*lint -esym(550,procResult) not accessed */
   static uint8_t       resetListPtr;  /* Argument sent to procedure.   */
   buffer_t             *pBuf;
   uint16_t             nbrUnread;
   enum_CIM_QualityCode mfgResult;     /* Return value from INTF_CIM_CMD_ansiProcedure() */
   enum_CIM_QualityCode stdResult;     /* Return value from INTF_CIM_CMD_ansiProcedure() */
   enum_CIM_QualityCode procResult;    /* Return value from INTF_CIM_CMD_ansiProcedure() */


   if ( eventLogging )
   {
      /* Read Table 76 to get the number of log entries  */
      pBuf = BM_alloc( sizeof( stdTbl76_t ) );
      if( pBuf != NULL )
      {
         if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( pBuf->data, STD_TBL_EVENT_LOG_DATA, 0, ( uint16_t )sizeof( stdTbl76_t ) ) )
         {
            pStdTbl76_t tbl76 = ( pStdTbl76_t )( ( void * )pBuf->data );
            nbrUnread = tbl76->nbrUnread;
            ALRM_PRNT_WARN( 'A', "clearEventLogs: Unread std event count = %d", nbrUnread );
            if ( nbrUnread != 0 )
            {
               updateLastReadData_[ 0 ] = 1;                               /* Event log   */
               updateLastReadData_[ 1 ] = ( uint8_t )( nbrUnread & 0xff ); /* nbrUnread LSB   */
               updateLastReadData_[ 2 ] = ( uint8_t )( nbrUnread >> 8 );   /* nbrUnread MSB   */
               /* Execute standard procedure to update the number of events read.   */
               procResult = INTF_CIM_CMD_ansiProcedure( updateLastReadData_, STD_PROC_UPDATE_LAST_READ_ENTRY,
                            sizeof( updateLastReadData_ ) );
               if ( procResult != CIM_QUALCODE_SUCCESS )
               {
                  ALRM_PRNT_ERROR( 'A', "Procedure %d with %d returns %d", STD_PROC_UPDATE_LAST_READ_ENTRY,
                                  nbrUnread, ( uint32_t )procResult );
               }
            }
         }
         else
         {
            ALRM_PRNT_ERROR( 'A', "clearEventLogs: unable to read std table %d", STD_TBL_EVENT_LOG_DATA );
         }
         BM_free( pBuf );
      }
   }

   if( voltageEventMonitor )
   {
      pBuf = BM_alloc( sizeof( mfgTbl112_t ) );
      if( pBuf != NULL )
      {
         if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( pBuf->data, MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG, 0, ( uint16_t )sizeof( mfgTbl112_t ) ) )
         {
            pMfgTbl112_t tbl112 = ( pMfgTbl112_t )( ( void * )pBuf->data );
            nbrUnread = tbl112->nbrUnread;
            ALRM_PRNT_WARN( 'A', "clearEventLogs: Unread mfg event count = %d", nbrUnread );
            if ( nbrUnread  != 0 )  /* Any sag/swell stopped entries retrieved?  */
            {
               updateLastReadData_[ 0 ] = 1;                               /* Event log   */
               updateLastReadData_[ 1 ] = ( uint8_t )( nbrUnread & 0xff ); /* nbrUnread LSB   */
               updateLastReadData_[ 2 ] = ( uint8_t )( nbrUnread >> 8 );   /* nbrUnread MSB   */
               /* Execute manufacturer procedure to update the number of VEM log entries read.   */
               procResult = INTF_CIM_CMD_ansiProcedure( updateLastReadData_,
                            MFG_PROC_UPDATE_LAST_READ_ENTRY, sizeof( updateLastReadData_ ) );
               if ( procResult != CIM_QUALCODE_SUCCESS )
               {
                  ALRM_PRNT_ERROR( 'A', "Procedure %d with %d returns %d", MFG_PROC_UPDATE_LAST_READ_ENTRY,
                                  nbrUnread, ( uint32_t )procResult );
               }
            }
         }
         else
         {
            ALRM_PRNT_ERROR( 'A', "clearEventLogs: unable to read mfg table %d", MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG - 2048 );
         }
         BM_free( pBuf );
      }
   }

   resetListPtr = 1;   /* standard event log; also VEM log   */

   if ( eventLogging )  /* Reset list pointers  */
   {
      stdResult = INTF_CIM_CMD_ansiProcedure( &resetListPtr,
                                              STD_PROC_RESET_LIST_POINTERS, sizeof( resetListPtr ) );
      if ( stdResult != CIM_QUALCODE_SUCCESS )
      {
         ALRM_PRNT_ERROR( 'A', "clearEventLogs Procedure %d with %d returns %d", STD_PROC_RESET_LIST_POINTERS, resetListPtr, ( uint32_t )stdResult );
      }
   }
   else
   {
      stdResult = CIM_QUALCODE_SUCCESS;
   }

   if ( voltageEventMonitor )
   {
      /* Execute manufacturer procedure to update the number of VEM log entries read.   */
      mfgResult = INTF_CIM_CMD_ansiProcedure( &resetListPtr,
                                              MFG_PROC_RST_VOLT_EVENT_MON_PTR, sizeof( resetListPtr ) );
      if ( mfgResult != CIM_QUALCODE_SUCCESS )
      {
         ALRM_PRNT_ERROR( 'A', "clearEventLogs Procedure %d with %d returns %d", MFG_PROC_RST_VOLT_EVENT_MON_PTR, resetListPtr, ( uint32_t )mfgResult );
      }
   }
   else
   {
      mfgResult = CIM_QUALCODE_SUCCESS;
   }

   if ( ( mfgResult == CIM_QUALCODE_SUCCESS ) && ( stdResult == CIM_QUALCODE_SUCCESS ) )
   {
      clearEventLogs_ = ( bool )false; /* Logs successfully cleared; reset request flag.  */
      ALRM_PRNT_WARN( 'A', "clearEventLogs: Meter logs successfully cleared" );
   }
}
#endif
#endif
#endif

/*******************************************************************************

   Function name: ALRM_RealTimeTask

   Purpose: This task monitors the Meter Trouble Queue and when alarms are present, this task logs them. The logger
            handles transmitting the real time alarms. The bubble up routines handle sending the opportunistic alarms.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns:

   Notes:

*******************************************************************************/
void ALRM_RealTimeTask ( taskParameter )
{
#if ( LOG_IN_METER == 1 )
   sysTime_t               eventTime;
   buffer_t               *pBuf;          /* Holds meter table readings                         */
   UpgradesBfld_t         *softSwitches;  /* Upgrades info from meter.                          */
   const stdTbl76Lookup_t *alarmPtr;      /* Pointer to the constant array of alarms            */
   uint32_t                fractionalSec;
   uint16_t                nbrUnread;     /* Number of events not yet read from meter           */
   uint16_t                VEMreadCnt;    /* Number of voltage event monitor log entries read   */
   uint16_t                nbrRead;
   uint16_t                seqIndex;
   enum_CIM_QualityCode    procResult;    /* Return value from INTF_CIM_CMD_ansiProcedure()     */
   uint16_t                eventNum;      /* Offset into meterAlarms array                      */
   buffer_t               *pEvent;        /* Used to hold event entry info from meter logs      */
   ValueInfo_t readingInfo;               /* Used to gather nvp data for end device events      */

#if ( ENABLE_PRNT_ALRM_INFO == 1 )
   sysTime_dateFormat_t    sysTime;
#endif
#endif
#if ( EP == 1 )
   EventKeyValuePair_s     keyVal[ 9 ];   /* Event key/value pair info                          */
   EventData_s             progEvent;     /* Event info                                         */
   TemperatureStatus_t     status = eNORMAL_TEMP;
   bool                    stackDisabled = (bool)false;       //flag to indicate, if stack disabled due to high temperature
#endif
   bool                    firstEntry = (bool)true;           // Used to skip the wait(5 mins) at very first entry to the loop.
#if ( LOG_IN_METER == 1 )
   bool                 resetProc;
#endif

#if ( ANSI_STANDARD_TABLES == 1 )
   /* Before starting the ALRM_RealTimeTask loop, retrieve the softswitches pertinent to event logging.  */
   while ( HMC_MTRINFO_getSoftswitches( &softSwitches ) != eSUCCESS )
   {
      OS_TASK_Sleep ( ONE_SEC );
   }
   eventLogging         = ( bool )softSwitches->eventLogging;
   voltageEventMonitor  = ( bool )softSwitches->voltageEventMonitor;
#endif

   /* Task begins here  */
   for ( ;; )
   {
#if ( LOG_IN_METER == 1 )
      resetProc = false;
#endif
      if(firstEntry)
      {
         firstEntry = (bool)false;
      }
      else
      {

#if ( ENABLE_PRNT_ALRM_INFO == 1 )  /* Poll more often when debugging   */
   #define ALRM_POLL_RATE           (ONE_SEC * 15)
#if ( LOG_IN_METER == 1 )
   #define ALRM_RESET_GRACE_PERIOD  (ONE_SEC * 13) //Allow a little less than poll rate
#endif
#else
   #define ALRM_POLL_RATE           (FIVE_MIN)
#if ( LOG_IN_METER == 1 )
   #define ALRM_RESET_GRACE_PERIOD  (ONE_MIN * 4)  //Allow a little less than poll rate
#endif
#endif

#if (  METER_TROUBLE_SIGNAL == 1 )
      /* If IRQ occurred, run immediately; else poll periodically */
         ( void )OS_SEM_Pend( &MeterTroubleSem, ALRM_POLL_RATE );
#else
         /* No meter trouble signal; just poll periodically */
         OS_TASK_Sleep ( ALRM_POLL_RATE );
#endif
      }

#if ( LOG_IN_METER == 1 )
      if ( eventLogging )  /* Event logging softswitch installed? */
      {
#if (  METER_TROUBLE_SIGNAL == 1 )
         ALRM_PRNT_INFO( 'A', "HMC_TROUBLE = %d", HMC_TROUBLE() );
         /* Run if IRQ occurred, previous check had events set, or trouble signal is active. */
#endif
         if ( clearEventLogs_ )
         {
            clearEventLogs();
            /* Since we just executed SP5 to reset the event log in the meter, we need to execute a
               snapshot procedure to ensure ST76 is immediately updated for the next table read*/
            procResult = INTF_CIM_CMD_ansiProcedure( updateLastReadData_, MFG_PROC_SNAPSHOT_REVENUE_DATA, 0 );
            if ( procResult != CIM_QUALCODE_SUCCESS )
            {
               ALRM_PRNT_ERROR( 'A', "Procedure %d returns %d", ( int32_t )MFG_PROC_SNAPSHOT_REVENUE_DATA, ( int32_t )procResult);
            }
         }

         // Initialize the event read counter variables, they will be updated if there are any unread events
         nbrRead = 0;
         VEMreadCnt = 0;

         /* Read Table 76 to get the number of log entries  */
         pBuf = BM_alloc( sizeof( stdTbl76_t ) );
         if( pBuf != NULL )
         {
            ALRM_PRNT_INFO( 'I', "Reading table %d to get nbrUnread", STD_TBL_EVENT_LOG_DATA );
            if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( pBuf->data, STD_TBL_EVENT_LOG_DATA,
                                                                  0, ( uint16_t )sizeof( stdTbl76_t ) ) )
            {
               pStdTbl76_t tbl76 = ( pStdTbl76_t )( ( void * )pBuf->data );
               nbrUnread = tbl76->nbrUnread;
               ALRM_PRNT_INFO( 'A', "stdtbl76 nbrUnread = %d", nbrUnread );
               if ( nbrUnread != 0 )
               {
                  pEvent = BM_alloc( sizeof ( stdTbl76Event_t ) );
                  if ( pEvent != NULL )
                  {
                     /*lint -esym(529,event) not subsequently accessed *//*lint -esym(438,event) last value not used*/
                     pStdTbl76Event_t event = ( pStdTbl76Event_t )( void * )pEvent->data;
                     uint16_t seqNbr = ( uint16_t )(tbl76->lastEntry - ( nbrUnread - 1 ) );
                     /* Loop through the unread entries and process them.  */
                     for ( uint16_t nbrToRead = nbrUnread ; nbrToRead != 0; nbrToRead-- )
                     {
                        if ( CIM_QUALCODE_SUCCESS ==
                              INTF_CIM_CMD_ansiRead( pEvent->data, STD_TBL_EVENT_LOG_DATA,
                                                     ( uint16_t )( offsetof( stdTbl76_t, eventArray ) +
                                                           seqNbr * ( uint16_t )sizeof( stdTbl76Event_t ) ),
                                                     ( uint16_t )sizeof( stdTbl76Event_t ) ) )
                        {
                           /* Attempt to locate this entry in the logevents table   */
#if ( ENABLE_PRNT_ALRM_INFO == 1 )
                           ( void )TIME_UTIL_ConvertMtrHighFormatToSysFormat( ( MtrTimeFormatHighPrecision_t * )event->dateTime, &eventTime );
                           ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &eventTime, &sysTime );
                           ALRM_PRNT_INFO( 'A', "Event time: %02d/%02d/%04d %02d:%02d:%02d", sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec );
#endif
                           ALRM_PRNT_WARN( 'A', "table: %d", STD_TBL_EVENT_LOG_DATA );
                           ALRM_PRNT_WARN( 'A', "tblProcNbr: %d", event->eventCode.tblProcNbr );
                           ALRM_PRNT_WARN( 'A', "mfgFlag   : %d", event->eventCode.mfgFlag );
                           ALRM_PRNT_WARN( 'A', "Argument  : %d", event->argument );
                           ALRM_PRNT_WARN( 'A', "SeqNbr    : %d\n", event->seqNbr );
                           for ( eventNum = 0, alarmPtr = logEvents; eventNum < ARRAY_IDX_CNT( logEvents ); alarmPtr++, eventNum++ )
                           {
                              if ( ( event->eventCode.tblProcNbr == alarmPtr->code.tblProcNbr ) && ( event->eventCode.mfgFlag == alarmPtr->code.mfgFlag ) )
                              {
                                 if ( ( alarmPtr->mask == 0 ) || ( alarmPtr->mask & event->argument ) )
                                 {
                                    *( uint16_t * )keyVal[ 0 ].Key = 0;
                                    *( uint16_t * )keyVal[ 0 ].Value = 0;
                                    progEvent.markSent = (bool)false;
                                    progEvent.eventId = ( uint16_t )alarmPtr->eventType;
                                    progEvent.eventKeyValuePairsCount = 0;
                                    progEvent.eventKeyValueSize = 0;

                                    /* TODO: need to update this process into a single function call that would fill
                                       NVP data strucure with all nvp required data based upon the event ID */
                                    // Check to see if any events require adding nvp data
                                    switch ( progEvent.eventId )   /*lint !e788 not all enums used within switch */
                                    {
#if ( COMMERCIAL_METER == 1 )
                                       case  (uint16_t)electricMeterPowerPhaseACurrentLimitReached:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( iA, &readingInfo ) )
                                          {  // get the current value for phase A
                                             readingInfo.Value.uintValue = readingInfo.Value.uintValue / 10; // convert deci amps to amps
                                             progEvent.eventKeyValueSize = (uint8_t)readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
                                       case  (uint16_t)electricMeterPowerPhaseBCurrentLimitReached:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( iB, &readingInfo ) )
                                          {  // get the current value for phase A
                                             readingInfo.Value.uintValue = readingInfo.Value.uintValue / 10; // convert deci amps to amps
                                             progEvent.eventKeyValueSize = (uint8_t)readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
                                       case  (uint16_t)electricMeterPowerPhaseCCurrentLimitReached:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( iC, &readingInfo ) )
                                          {  // get the current value for phase A
                                             readingInfo.Value.uintValue = readingInfo.Value.uintValue / 10; // convert deci amps to amps
                                             progEvent.eventKeyValueSize = (uint8_t)readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
#else
                                       case  (uint16_t)electricMeterPowerCurrentMaxLimitReached:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( iA, &readingInfo ) )
                                          {  // get the current value for phase A
                                             readingInfo.Value.uintValue = readingInfo.Value.uintValue / 10; // convert deci amps to amps
                                             progEvent.eventKeyValueSize = readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
#endif
#if ( REMOTE_DISCONNECT == 1 )
                                       case  (uint16_t)electricMeterRcdSwitchConnected:
                                       case  (uint16_t)electricMeterRcdSwitchDisconnected:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( switchPositionStatus, &readingInfo ) )
                                          {  // get switch position status
                                             progEvent.eventKeyValueSize = readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( energizationLoadSide, &readingInfo ) )
                                          {  // get the load side voltage status
                                             if( progEvent.eventKeyValueSize < readingInfo.valueSizeInBytes )
                                             {  // is this reading larger than current nvp size, if so update the nvp size
                                                progEvent.eventKeyValueSize = readingInfo.valueSizeInBytes;
                                             }
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
                                       case  (uint16_t)electricMeterRCDSwitchSupplyCapacityLimitDisconnected:
                                       case  (uint16_t)electricMeterRCDSwitchPrepaymentCreditExpired:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( switchPositionCount, &readingInfo ) )
                                          {  // get the switch position count
                                             progEvent.eventKeyValueSize = readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
#endif
                                       case  (uint16_t)electricMeterPowerPowerQualityHighDistortion:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( highTHDcount, &readingInfo ) )
                                          {
                                             progEvent.eventKeyValueSize = (uint8_t)readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
                                       case  (uint16_t)electricMeterConfigurationProgramChanged:
                                       {
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( meterProgramCount, &readingInfo ) )
                                          {  // get meter program count
                                             progEvent.eventKeyValueSize = (uint8_t)readingInfo.valueSizeInBytes;
                                             *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                             (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                             progEvent.eventKeyValuePairsCount++;
                                          }
                                          break;
                                       }
                                       default:
                                       {  // no nvp data needs to be added, no updates to the nvp info
                                          break;
                                       }
                                    } /*lint !e788 not all enums used within switch */

                                    ( void )TIME_UTIL_ConvertMtrHighFormatToSysFormat( ( MtrTimeFormatHighPrecision_t * )event->dateTime, &eventTime );
                                    TIME_UTIL_ConvertSysFormatToSeconds( &eventTime, &progEvent.timestamp, &fractionalSec );
                                    ( void )EVL_LogEvent( alarmPtr->priority, &progEvent, keyVal, TIMESTAMP_PROVIDED, NULL );

                                    // Check to see if any events requiring additional action
                                    switch ( alarmPtr->eventType )   /*lint !e788 not all enums used within switch */
                                    {
                                       case ( electricMeterConfigurationProgramChanged ):
                                       {   /* We got a meter program change event.  This event could have also been
                                              caused by the endpoint setting the meter's date and time.  If a meter's
                                              congiguration has been changed, a reboot is necessary to make sure the
                                              endpoint's status of current meter tables is up to date. */
#if ( CLOCK_IN_METER == 1 )
                                          // acquire the current meter programmed date/time from Mfg Table #78
                                          //ValueInfo_t readingInfo;
                                          if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( edProgrammedDateTime, &readingInfo ))
                                          {
                                             /* Compare the programmed date/time from Mfg Table #78, to the current
                                                value. If they match, then the endpoint setting date/time in the meter
                                                is reponsible for the programmed change event and we should not reboot. */
                                             if( (uint32_t)readingInfo.Value.uintValue != HMC_TIME_getMfgPrc70TimeParameters() )
                                             {  /* The HMC Time setting applet did not cause the event. The meter Program
                                                   just changed, reset processor so meter info meter tables to be re-read. */
                                                resetProc = true;
                                             }
                                          }
#else
                                          resetProc = true;
#endif
                                          break;
                                       }
                                       // An electricMeterDemandResetOccurred could cause the /dr message to be sent
                                       default:
                                       {
                                          break;
                                       }
                                    } /*lint !e788 not all enums used within switch */
                                    break;
                                 }
                              }
                           }
                           nbrRead++;
                        }
                        seqNbr++;
                     }
                     BM_free( pEvent );
                  }
               }
               if ( nbrRead != 0 )
               {
                  updateLastReadData_[ 0 ] = 1;   /* Event log   */
                  updateLastReadData_[ 1 ] = ( uint8_t )( nbrRead & 0xff ); /* nbrRead LSB   */
                  updateLastReadData_[ 2 ] = ( uint8_t )( nbrRead >> 8 );   /* nbrRead MSB   */
                  /* Execute standard procedure to update the number of events read.   */
                  procResult = INTF_CIM_CMD_ansiProcedure( updateLastReadData_, STD_PROC_UPDATE_LAST_READ_ENTRY,
                               sizeof( updateLastReadData_ ) );
                  if ( procResult != CIM_QUALCODE_SUCCESS )
                  {
                     /* Do not allow a reset.  STD_PROC_UPDATE_LAST_READ_ENTRY failed so next time ST76 is read, the meter event
                        will still "exist" and cause another reset if allowed. */
                     resetProc = false;
                     ALRM_PRNT_ERROR( 'A', "Procedure %d with %d returns %d", STD_PROC_UPDATE_LAST_READ_ENTRY,
                                     nbrRead, ( uint32_t )procResult );
                  }
               }
            }
            BM_free( pBuf );
         }

         /* Now process the Voltage Event Monitor log.   */
         if ( voltageEventMonitor )  /* Voltage Event Monitor softswitch installed? */
         {
            pBuf = BM_alloc( sizeof( mfgTbl112_t ) );
            if ( pBuf != NULL )
            {
               pMfgTbl112_t VEMtbl = ( pMfgTbl112_t )( void * )pBuf->data;

               ALRM_PRNT_INFO( 'I', "Reading mfg table %d to get nbrUnread", MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG - 2048 );
               if ( CIM_QUALCODE_SUCCESS ==
                     INTF_CIM_CMD_ansiRead( VEMtbl, MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG, 0, ( uint16_t )sizeof( mfgTbl112_t ) ) )
               {
                  nbrUnread = VEMtbl->nbrUnread;
                  ALRM_PRNT_INFO( 'A', "mfgtbl112 nbrUnread = %d", nbrUnread );

                  if ( nbrUnread != 0 )
                  {
                     seqIndex = ( uint16_t )( VEMtbl->lastEntry - ( nbrUnread - 1 ) );
                     pEvent = BM_alloc( sizeof( sagSwellEntry_t ) );
                     if ( pEvent != NULL )
                     {
                        pSagSwellEntry_t  pEntry = ( pSagSwellEntry_t  )( void * )pEvent->data;
                        for ( uint16_t nbrToRead = nbrUnread ; nbrToRead != 0; nbrToRead-- )
                        {
                           if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( pEntry, MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG,
                                                                                 ( uint16_t )( offsetof( mfgTbl112_t, eventArray ) +
                                                                                 seqIndex * ( uint16_t )sizeof( sagSwellEntry_t ) ),
                                                                                 ( uint16_t )sizeof( sagSwellEntry_t ) ) )
                           {
                              VEMreadCnt++;  /* Update number of VEM entries read   */
#if ( ENABLE_PRNT_ALRM_INFO == 1 )
                              ( void )TIME_UTIL_ConvertMtrHighFormatToSysFormat( ( MtrTimeFormatHighPrecision_t * )pEntry->eventTime, &eventTime );
                              ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &eventTime, &sysTime );
                              ALRM_PRNT_INFO( 'V', "Event time: %02d/%02d/%04d %02d:%02d:%02d", sysTime.month, sysTime.day, sysTime.year, sysTime.hour, sysTime.min, sysTime.sec );
#endif
                              ALRM_PRNT_WARN( 'V', "mfg table : %d", MFG_TBL_VOLTAGE_EVENT_MONITOR_LOG - 2048 );
                              ALRM_PRNT_WARN( 'V', "event type: %s", pEntry->eventType == 0 ? "sag" : "swell" );
                              ALRM_PRNT_WARN( 'V', "Duration  : %d", pEntry->eventDuration );
                              ALRM_PRNT_WARN( 'V', "Ph A Volts: %d", pEntry->voltagePhA );
                              ALRM_PRNT_WARN( 'V', "Ph B Volts: %d", pEntry->voltagePhB );
                              ALRM_PRNT_WARN( 'V', "Ph C Volts: %d", pEntry->voltagePhC );
                              ALRM_PRNT_WARN( 'V', "Ph A Amps : %d", pEntry->currentPhA );
                              ALRM_PRNT_WARN( 'V', "Ph B Amps : %d", pEntry->currentPhB );
                              ALRM_PRNT_WARN( 'V', "Ph C Amps : %d\n", pEntry->currentPhC );
                               /* Log event   */
                              if ( pEntry->eventType == 0 )
                              {
                                 progEvent.eventId = ( uint16_t )electricMeterPowerPhaseAVoltageSagStopped;
                              }
                              else
                              {
                                 progEvent.eventId = ( uint16_t )electricMeterPowerPhaseAVoltageSwellStopped;
                              }
                              progEvent.markSent = (bool)false;
                              progEvent.eventKeyValuePairsCount = 0;
                              uint8_t NVPIndex = 0;
                              progEvent.eventKeyValueSize = sizeof( pEntry->voltagePhA );       /* All values have the same size */
                              *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )sag;               /*lint !e2445 cast increases alignment req.  */
                              *( uint16_t * )keyVal[ NVPIndex ].Value = VEMtbl->sagCounter;            /*lint !e2445 cast increases alignment req.  */
                              NVPIndex++;
                              *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )swell;             /*lint !e2445 cast increases alignment req.  */
                              *( uint16_t * )keyVal[ NVPIndex ].Value = VEMtbl->swellCounter;          /*lint !e2445 cast increases alignment req.  */
                              NVPIndex++;
                              *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )sagSwellDuration;  /*lint !e2445 cast increases alignment req.  */
                              *( uint16_t * )keyVal[ NVPIndex ].Value = pEntry->eventDuration;         /*lint !e2445 cast increases alignment req.  */
                              NVPIndex++;
#if ( HMC_I210_PLUS_C == 1 )  // Voltage Event Monitor is only supported by I210+c and KV2c.  I210+c only supports Phase A
                              if( pEntry->eventType == 0 )
                              {  // a sag occured, nvp key voltage should be minimum
                                 *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )minIndVRMSA;
                              }
                              else
                              {  // a swell occured, nvp key voltage should be maximum
                                 *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )maxIndVRMSA;    /*lint !e2445 cast increases alignment req.  */
                              }
                              // voltage is in decivolts, so round the value
                              *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->voltagePhA + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                              NVPIndex++;
#else // KV2c supports three phases
                              if( pEntry->eventType == 0 )
                              {   // a sag occured, nvp key voltage should be minimum

                                 if(pEntry->voltagePhA != V_I_ANGLE_MEAS_UNSUPPORTED )
                                 {
                                    *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )minIndVRMSA;
                                    // voltage is in decivolts, so round the value
                                    *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->voltagePhA + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                    NVPIndex++;
                                 }
                                 if(pEntry->voltagePhB != V_I_ANGLE_MEAS_UNSUPPORTED )
                                 {
                                    *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )minIndVRMSB;    /*lint !e2445 cast increases alignment req.  */
                                    // voltage is in decivolts, so round the value
                                    *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->voltagePhB + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                    NVPIndex++;
                                 }
                                 if(pEntry->voltagePhC != V_I_ANGLE_MEAS_UNSUPPORTED )
                                 {
                                    *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )minIndVRMSC;    /*lint !e2445 cast increases alignment req.  */
                                    // voltage is in decivolts, so round the value
                                    *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->voltagePhC + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                    NVPIndex++;
                                 }
                              }
                              else
                              {  // a swell occured, nvp key voltage should be maximum
                                 if(pEntry->voltagePhA != V_I_ANGLE_MEAS_UNSUPPORTED )
                                 {
                                    *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )maxIndVRMSA;    /*lint !e2445 cast increases alignment req.  */
                                    *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->voltagePhA + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                    NVPIndex++;
                                 }
                                 if(pEntry->voltagePhB != V_I_ANGLE_MEAS_UNSUPPORTED )
                                 {
                                    *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )maxIndVRMSB;    /*lint !e2445 cast increases alignment req.  */
                                    *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->voltagePhB + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */                                   *( uint16_t * )keyVal[ 4 ].Value = ( pEntry->voltagePhB + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                    NVPIndex++;
                                 }
                                 if(pEntry->voltagePhC != V_I_ANGLE_MEAS_UNSUPPORTED )
                                 {
                                    *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )maxIndVRMSC;    /*lint !e2445 cast increases alignment req.  */
                                    *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->voltagePhC + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                    NVPIndex++;
                                 }
                              }

                              if( pEntry->currentPhA != V_I_ANGLE_MEAS_UNSUPPORTED )
                              {
                                 *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )iA;                  /*lint !e2445 cast increases alignment req.  */
                                 *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->currentPhA + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                 NVPIndex++;
                              }

                              if( pEntry->currentPhB != V_I_ANGLE_MEAS_UNSUPPORTED )
                              {
                                 *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )iB;                  /*lint !e2445 cast increases alignment req.  */
                                 *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->currentPhB + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                 NVPIndex++;
                              }
                              if( pEntry->currentPhC != V_I_ANGLE_MEAS_UNSUPPORTED )
                              {
                                 *( uint16_t * )keyVal[ NVPIndex ].Key   = ( uint16_t )iC;                  /*lint !e2445 cast increases alignment req.  */
                                 *( uint16_t * )keyVal[ NVPIndex ].Value = ( pEntry->currentPhC + 5 ) / 10; /*lint !e2445 cast increases alignment req.  */
                                 NVPIndex++;
                              }

#endif
                              progEvent.eventKeyValuePairsCount = NVPIndex;
                              ( void )TIME_UTIL_ConvertMtrHighFormatToSysFormat( ( MtrTimeFormatHighPrecision_t * )pEntry->eventTime, &eventTime );
                              TIME_UTIL_ConvertSysFormatToSeconds( &eventTime, &progEvent.timestamp, &fractionalSec );
                              ( void )EVL_LogEvent( 87, &progEvent, keyVal, TIMESTAMP_PROVIDED, NULL );
                           }
                        }
                        if ( VEMreadCnt != 0 )  /* Any sag/swell stopped entries retrieved?  */
                        {
                           updateLastReadData_[ 0 ] = 1;   /* Event log   */
                           updateLastReadData_[ 1 ] = ( uint8_t )( VEMreadCnt & 0xff ); /* VEMreadCnt LSB   */
                           updateLastReadData_[ 2 ] = ( uint8_t )( VEMreadCnt >> 8 );   /* VEMreadCnt MSB   */
                           /* Execute manufacturer procedure to update the number of VEM log entries read.   */
                           procResult = INTF_CIM_CMD_ansiProcedure( updateLastReadData_,
                                                                      MFG_PROC_UPDATE_LAST_READ_ENTRY, sizeof( updateLastReadData_ ) );
                           if ( procResult != CIM_QUALCODE_SUCCESS )
                           {
                              ALRM_PRNT_ERROR( 'A', "Procedure %d with %d returns %d", MFG_PROC_UPDATE_LAST_READ_ENTRY,
                                                VEMreadCnt, ( uint32_t )procResult );
                           }
                        }
                        BM_free( pEvent );
                     }
                  }
               }
               BM_free( pBuf );
            }
         }

         /* We have read events from the meter's event log and executed a standard procedure #5. Execute
            manufacturer procedure 84 to ensure ST76 and MT112 are updated to the latest status. */
         if( 0 !=  nbrRead || 0 != VEMreadCnt )
         {
            procResult = INTF_CIM_CMD_ansiProcedure( updateLastReadData_, MFG_PROC_SNAPSHOT_REVENUE_DATA, 0 );
            if ( procResult != CIM_QUALCODE_SUCCESS )
            {
               ALRM_PRNT_ERROR( 'A', "Procedure %d returns %d", ( int32_t )MFG_PROC_SNAPSHOT_REVENUE_DATA, ( int32_t )procResult);
            }
         }

#if (  ( METER_TROUBLE_SIGNAL == 1 ) && ( HMC_TROUBLE_EDGE_TRIGGERED == 0 ) )
         /* If level sensitive and NOT asserted, enable the interrupt.  */
         if ( !HMC_TROUBLE() )
         {
            HMC_TROUBLE_BUSY_IRQ_EI();    /* Enable the ISR */
         }
#endif
      }  /* End of if event logging softswitch installed */
#endif
#if ( EP == 1 )
      /* Now check all the comDevice base events   */
      /* Check the temperature and manage stack based on processor temperature   */
      static eventHistory_t eventHistory;

      ( void )FIO_fread( &fileHndlEvent_, (uint8_t *)&eventHistory, ( fileOffset )offsetof( eventHistory_t, comEvents_t ), ( lCnt )sizeof( eventHistory_t ) );

      progEvent.markSent = (bool)false;
      status = TEMPERATURE_getEpTemperatureStatus();
      if ( status == eHIGH_TEMP_THRESHOLD )
      {
         if ( eventHistory.comEvents_t.highTempThresholdEvent == 0 )
         {
            eventHistory.comEvents_t.highTempThresholdEvent = 1;  /* Update the status bit. */
            /* Log event   */
            progEvent.eventId = ( uint16_t )comDeviceTemperatureThresholdMaxLimitReached;
            *( uint16_t * )keyVal[ 0 ].Key = 0;    /*lint !e2445 cast increases alignment req.  */
            *( uint16_t * )keyVal[ 0 ].Value = 0;  /*lint !e2445 cast increases alignment req.  */
            progEvent.eventKeyValuePairsCount = 0;
            progEvent.eventKeyValueSize = 0;

            // add the NVP data for this alarm
            PHY_GetConf_t   GetConf;
            GetConf = PHY_GetRequest( ePhyAttr_Temperature );   /* Read the Radio 0 Temperature*/
            if( ePHY_GET_SUCCESS == GetConf.eStatus )
            {
               progEvent.eventKeyValuePairsCount++;
               progEvent.eventKeyValueSize = HEEP_getMinByteNeeded( (int64_t)GetConf.val.Temperature,
                                                                     intValue, sizeof(GetConf.val.Temperature) );
               *( uint16_t * )keyVal[ 0 ].Key = (uint16_t)temperature;
               (void)memcpy(  keyVal[ 0 ].Value,(uint8_t *)&GetConf.val.Temperature, progEvent.eventKeyValueSize );
            }

            /*lint -e{413} NULL pointer OK - no call back provided   */
            ( void )EVL_LogEvent( 205, &progEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
            (void)FIO_fwrite(&fileHndlEvent_, ( fileOffset )offsetof( eventHistory_t, comEvents_t ),
                             (uint8_t*) &eventHistory, (lCnt)sizeof(eventHistory.comEvents_t)); /* Update the file*/
         }
      }
      else if ( status == eLOW_TEMP_THRESHOLD )
      {
         if ( eventHistory.comEvents_t.lowTempThresholdEvent == 0 )
         {
            eventHistory.comEvents_t.lowTempThresholdEvent = 1;  /* Update the status bit. */
            /* Log event   */
            progEvent.eventId = ( uint16_t )comDeviceTemperatureThresholdMinLimitReached;
            *( uint16_t * )keyVal[ 0 ].Key = 0;    /*lint !e2445 cast increases alignment req.  */
            *( uint16_t * )keyVal[ 0 ].Value = 0;  /*lint !e2445 cast increases alignment req.  */
            progEvent.eventKeyValuePairsCount = 0;
            progEvent.eventKeyValueSize = 0;
            /*lint -e{413} NULL pointer OK - no call back provided   */
            ( void )EVL_LogEvent( 90, &progEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
            (void)FIO_fwrite(&fileHndlEvent_, ( fileOffset )offsetof( eventHistory_t, comEvents_t ),
                             (uint8_t*) &eventHistory, (lCnt)sizeof(eventHistory.comEvents_t)); /* Update the file*/
         }
      }
      else if ( status == eNORMAL_TEMP )
      {
         if ( eventHistory.comEvents_t.highTempThresholdEvent == 1 )
         {
            eventHistory.comEvents_t.highTempThresholdEvent = 0;  /* Update the status bit. */
            /* Log event   */
            progEvent.eventId = ( uint16_t )comDeviceTemperatureThresholdMaxLimitCleared;
            *( uint16_t * )keyVal[ 0 ].Key = 0;    /*lint !e2445 cast increases alignment req.  */
            *( uint16_t * )keyVal[ 0 ].Value = 0;  /*lint !e2445 cast increases alignment req.  */
            progEvent.eventKeyValuePairsCount = 0;
            progEvent.eventKeyValueSize = 0;
            ( void )EVL_LogEvent( 180, &progEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
            (void)FIO_fwrite(&fileHndlEvent_, ( fileOffset )offsetof( eventHistory_t, comEvents_t ),
                             (uint8_t*) &eventHistory, (lCnt)sizeof(eventHistory.comEvents_t)); /* Update the file*/
         }
         else if ( eventHistory.comEvents_t.lowTempThresholdEvent == 1 )
         {
            eventHistory.comEvents_t.lowTempThresholdEvent = 0;  /* Update the status bit. */
            /* Log event   */
            progEvent.eventId = ( uint16_t )comDeviceTemperatureThresholdMinLimitCleared;
            *( uint16_t * )keyVal[ 0 ].Key = 0;    /*lint !e2445 cast increases alignment req.  */
            *( uint16_t * )keyVal[ 0 ].Value = 0;  /*lint !e2445 cast increases alignment req.  */
            progEvent.eventKeyValuePairsCount = 0;
            progEvent.eventKeyValueSize = 0;
            ( void )EVL_LogEvent( 90, &progEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
            (void)FIO_fwrite(&fileHndlEvent_, ( fileOffset )offsetof( eventHistory_t, comEvents_t ),
                             (uint8_t*) &eventHistory, (lCnt)sizeof(eventHistory.comEvents_t)); /* Update the file*/
         }
      }

      /* Manage the Stack */
      // MKD 2019-08-06 Some of this code was handled in temperature.c but it was flawed.
      // I moved back the stack management here to decouple it from the alarm mamangement.
      // This code need to go away.
      // The TB already handles this beahvior in the PHY and EPs need to do the same but it's not trivial because the algorithm would have to change in some fundamental ways.
      int32_t procTemp;

      // Avoid using radio temperature because it takes 1ms and this is disruptive to the soft-modem (when used).
      procTemp = (int32_t)ADC_Get_uP_Temperature( TEMP_IN_DEG_C );

      // Check if stack is enabled/disabled
      if ( stackDisabled ) {
         // Stack is disabled, check if we can re-enable it.
         if ( (procTemp <= (MAX_TEMP_LIMIT - STACK_HYSTERISIS)) && (procTemp >= (MIN_TEMP_LIMIT + STACK_HYSTERISIS)) ) {
            (void)SM_StartRequest(eSM_START_STANDARD, NULL);
            stackDisabled = ( bool )false;
         }
      } else {
         // Stack is enabled, check if we need to disable it.
         if ( (procTemp > MAX_TEMP_LIMIT) || (procTemp < MIN_TEMP_LIMIT) ) {
            (void)SM_StartRequest(eSM_START_MUTE, NULL);
            stackDisabled = ( bool )true;
         }
      }
#endif
#if ( LOG_IN_METER == 1 )
      // A condition was discovered above requiring the processor to be reset
      if ( resetProc )
      {
#if 0 // TODO: RA6E1 Enable once DFW is in place
         (void)DFWA_WaitForSysIdle(ALRM_RESET_GRACE_PERIOD);   // Do not need unlock mutex since reset inevitable
#endif
         //Keep all other tasks from running
         /* Increase the priority of the power and idle tasks. */
         (void)OS_TASK_Set_Priority(pTskName_Pwr, 10);
         (void)OS_TASK_Set_Priority(pTskName_Alrm, 11);
         (void)OS_TASK_Set_Priority(pTskName_Idle, 12);
         PWR_SafeReset();
      }
#endif
   }  /* End of ALRM_RealTimeTask forever loop  */
}
/*lint +esym(529,event) not subsequently accessed *//*lint +esym(438,event) last value not used*/
/***********************************************************************************************************************

   Function name: ALRM_AddOpportunisticAlarms

   Purpose: Adds opporutnistic alarms to the current HEEP message

   Arguments:  uint16_t          msgLen - length of existing msg including header. Used to calculate space for alarm data
               uint16_t          *alarmLen - pointer to number of bytes of alarm data added.
               uint8_t           *alarmData - pointer to destination of alarm data
               uint8_t           *alarmID - alarm index/identifier as logged.
               EventMarkSent_s   *markedSent -  Used to mark events that have been sent to head end

   Returns: Number of alarms packed into the buffer.

   Note: alarm array is sized to guarantee that the 1280 byte max payload to the MAC is NOT exceeded.

 **********************************************************************************************************************/
uint8_t ALRM_AddOpportunisticAlarms(   uint16_t msgLen,
                                       uint16_t *alarmLen,
                                       uint8_t *alarmData,
                                       uint8_t *alarmID,
                                       EventMarkSent_s *markedSent )
{
   pack_t               packCfg;                   /* Struct needed for PACK functions          */
   EventHeadEndData_s   *event;                    /* Pointer to event data returned in alarm   */
   uint8_t              alarm[ MAX_ALARM_MEMORY ]; /* Holder for current alarm                  */
   uint8_t              *pair = alarm;             /* Pointer to event pairs in buffer from log */
   int16_t              alarmBytes;                /* Number of bytes returned from event log (Error if < 0)  */
   uint8_t              alarmCount = 0;            /* Number of alarms added to buffer          */
   uint8_t              data;                      /* Local byte storage for bit values         */

   PACK_init( alarmData, &packCfg );
   *alarmLen = 0;

   /* Pass back the alarm ID; this becomes the request/response ID  */
   /*lint --e{661,662} area being packed guaranteed to have enough room for event pairs  */
   /* NOTE:                                    Do not use APP_MSG_MAX_DATA    because the header is already included */
   alarmBytes = ( int16_t )EVL_GetLog( min( ( uint16_t )( APP_MSG_MAX_PAYLOAD - msgLen ), MAX_ALARM_MEMORY ), alarm, alarmID, markedSent );
   if ( 0 < alarmBytes )   // If zero, then no alarms. If less than 0, then an error occurred and there are no alarms.
   {
      while ( 0 != alarmBytes )
      {
         /* point into alarm data array at proper offset */
         event = ( EventHeadEndData_s * )( void * )&alarm[ *alarmLen ];
         /* Add the event data to the output buffer (packCfg)  */
         alarmCount++;

         /* EndDeviceEvents.createdDateTime - 4 bytes */
         PACK_Buffer(-( (int16_t)sizeof( event->timestamp ) * 8 ), ( uint8_t * )&event->timestamp, &packCfg);

         /* EndDeviceEvents.EndDeviceEventType - 2 bytes */
         PACK_Buffer(-(int16_t)sizeof( event->eventId ) * 8, ( uint8_t * )&event->eventId, &packCfg);

         /* NameValuePairQty - 4 bits  */
         data = event->EventKeyHead_s.nPairs;
         PACK_Buffer( 4, &data, &packCfg );         /* Number of event/value pairs is packed in 4 bits */

         /* EndDevEDValueSize - 4 bits */
         data = event->EventKeyHead_s.valueSize;
         PACK_Buffer( 4, &data, &packCfg );         /* Size of values is packed in 4 bits              */

         alarmBytes  -= ( int16_t )sizeof( EventHeadEndData_s );
         *alarmLen   += sizeof( EventHeadEndData_s );

         /* Loop through the name value pairs in the returned buffer */
         pair += sizeof( EventHeadEndData_s );
         for ( uint8_t pairCount = 0; pairCount < event->EventKeyHead_s.nPairs; pairCount++ )
         {
            /* EndDeviceEvents.EndDeviceEventDetails.name - 2 bytes, byte swapped   */
            PACK_Buffer(-(int16_t)sizeof( uint16_t ) * 8, pair, &packCfg);
            pair        += sizeof( uint16_t );  /* Advance by key EndDeviceEventDetails.name size - 2 bytes */
            alarmBytes  -= ( int16_t )sizeof( uint16_t );
            *alarmLen   += sizeof( uint16_t );

            /* EndDeviceEvents.EndDeviceEventDetails.value - EndDevEDValueSizeBytes, byte swapped */
            PACK_Buffer(-(int16_t)event->EventKeyHead_s.valueSize * 8, pair, &packCfg);
            pair        += event->EventKeyHead_s.valueSize;  /* Advance by key Value size */
            alarmBytes  -= event->EventKeyHead_s.valueSize;
            *alarmLen   += event->EventKeyHead_s.valueSize;
         }
      }
   }

   *alarmID = EVL_GetRealTimeIndex(); /* Get the last logged real time event index ID */

   return alarmCount;
}

#if ( EP == 1 )
#if ( ENABLE_METER_EVENT_LOGGING != 0 )
#if ( ( METER_TROUBLE_SIGNAL == 1 ) && ( ANSI_STANDARD_TABLES == 1 ) )
/***********************************************************************************************************************

   Function Name: meter_trouble_isr_busy

   Purpose: Interrupt on busy signal (Used for SST devices in AAI mode). Posts to the meter trouble semaphore.

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#if ( MCU_SELECTED == NXP_K24 )
static void meter_trouble_isr_busy( void )
#elif ( MCU_SELECTED == RA6E1 )
void meter_trouble_isr_busy(external_irq_callback_args_t * p_args)
#endif
{
#if ( HMC_TROUBLE_EDGE_TRIGGERED == 0 )
   HMC_TROUBLE_BUSY_IRQ_DI();       /* Disable the ISR */
#else
   HMC_TROUBLE_BUSY_IRQ_EI()        /* This resets the ISF flag   */
#endif
#if ( RTOS_SELECTION == MQX_RTOS )
   OS_SEM_Post( &MeterTroubleSem ); /* Post the semaphore */
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_SEM_Post_fromISR( &MeterTroubleSem ); /* Post the semaphore */
#endif
   ( void )HMC_DIAGS_DoDiags( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, NULL );
}
#endif
#endif
#endif
