/***********************************************************************************************************************
 *
 * Filename:   ILC_TIME_SYNC_HANDLER.c
 *
 * Global Designator: ILC_TIME_SYNC_
 *
 * Contents: Contains the routines to support the DRU's time synchronization
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ KAD Created 080207
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 *
 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "ilc_time_sync_handler.h"
#include "DBG_SerialDebug.h"


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define ILC_TIME_SYNC_TIME_OUT_mS   ( (uint32_t)2000 )                  /* Time out in ms when waiting for DRU */
#define TIME_SYNC_PERIOD_S          ( (uint32_t)( TIME_TICKS_PER_HR ) ) /* Time sync period in milliseconds */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


 /* ****************************************************************************************************************** */
/* GLOBAL VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static OS_MSGQ_Obj TimeSync_MsgQ_;                              /* Message queue for Time Sync's Periodic Alarm */
static uint8_t alarmId_ = NO_ALARM_FOUND;                       /* Alarm ID */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static void addTimeSyncAlarm ( void );


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/***********************************************************************************************************************
 Function Name: addTimeSyncAlarm

 Purpose: Subscribes Time Sync alarm. If already subscribed, delete the current alarm and re-subscribes (need if the
          daily read time changes)

 Arguments: None

 Returns: None
 ***********************************************************************************************************************/
static void addTimeSyncAlarm ( void )
{
   tTimeSysPerAlarm   alarmSettings;

   if ( alarmId_ != NO_ALARM_FOUND )
   {  /* Already subscribed, delete current alarm */
      (void)TIME_SYS_DeleteAlarm ( alarmId_ );
      alarmId_ = NO_ALARM_FOUND;
   }

   /* Set up a periodic alarm */
   alarmSettings.bOnInvalidTime = false;                 /* Only alarmed on valid time, not invalid */
   alarmSettings.bOnValidTime = true;                    /* Alarmed on valid time */
   alarmSettings.bSkipTimeChange = false;                /* Notify on time change */
   alarmSettings.bUseLocalTime = true;                   /* Use local time */
   alarmSettings.pQueueHandle = NULL;                    /* do NOT use queue */
   alarmSettings.pMQueueHandle = &TimeSync_MsgQ_;        /* Uses the message Queue */
   alarmSettings.ucAlarmId = 0;                          /* 0 will create a new ID, anything else will reconfigure. */
   alarmSettings.ulOffset = 0;                           /* Offset past midnight for daily shift */
   alarmSettings.ulPeriod = TIME_SYNC_PERIOD_S;          /* Time Sync period in ticks */
   (void)TIME_SYS_AddPerAlarm(&alarmSettings);           /* Add the Periodic Alarm */
   alarmId_ = alarmSettings.ucAlarmId;
}


/* *********************************************************************************************************************
  Function name: ILC_TIME_SYNC_Init

  Purpose:  Creates all the OS Objects that Time Sync Handler needs

  Arguments: None

  Returns: eSUCCESS/eFAILURE indication
 **********************************************************************************************************************/
returnStatus_t ILC_TIME_SYNC_Init( void )
{
   returnStatus_t retVal = eFAILURE;

   if ( OS_MSGQ_Create( &TimeSync_MsgQ_ ) )
   {
      retVal = eSUCCESS;
   }
   else
   {
      INFO_printf( "Unable to create TimeSync_MsgQ_" );
   }

   return retVal;
}

/***********************************************************************************************************************
  Function name: ILC_TIME_SYNC_Task

  Purpose: Triggers the request to DRU Driver to update the DRU's date/time with the current Sys Time

  Arguments: uint32_t Arg0 - Not used, but required here by MQX because this is a task

  Returns: None
 **********************************************************************************************************************/
void ILC_TIME_SYNC_Task( uint32_t Arg0 )
{
   buffer_t *pBuf;            /* pointer to message */
   sysTime_t curTime;

   addTimeSyncAlarm();        /* Set up periodic alarm to send time to ILC */

   do
   {
      OS_TASK_Sleep( 2000 );
   }
   while ( !TIME_SYS_GetSysDateTime( &curTime ) ); /* Make sure time is valid before initial time set cmd to ILC  */

   /* Send self alarm at start up to force time sync to DRU immediately.
      Contents of alarm structure don't matter, since the alarm is processed unconditionally in the task loop.  */
   pBuf = BM_alloc(sizeof(tTimeSysMsg));
   if ( pBuf != NULL )
   {
      OS_MSGQ_Post( &TimeSync_MsgQ_, pBuf );
   }

   /* Wait for a periodic alarm or for a time change */
   for( ;; )
   {
      (void)OS_MSGQ_Pend( &TimeSync_MsgQ_, (void *)&pBuf, OS_WAIT_FOREVER );

      /* Get current time in Sys Format */
      ( void )TIME_SYS_GetSysDateTime( &curTime );

      /* Convert current Sys Time to DRU format and update the DRU's date/time with it */
      if ( CIM_QUALCODE_SUCCESS != ILC_DRU_DRIVER_setDruDateTime( curTime, ILC_TIME_SYNC_TIME_OUT_mS ) )
      {
         INFO_printf( "Unsuccessful DRU Time Sync" );
      }
      else
      {
         INFO_printf( "Successful DRU Time Sync" );
      }
      /* Free buffer used in TimeSync_MsgQ_ by periodic alarm or time change */
      BM_free( pBuf );
   }
}

/*End of file */
