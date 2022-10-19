/***********************************************************************************************************************

   Filename:   hmc_app.c

   Global Designator: HMC_APP_

   Contents: Contains the routines to support applet meter communication needs

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2018 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$ KAD Created 080207

 ***********************************************************************************************************************
   Revision History:
   v0.1 -   Initial Release
   v0.2 -   9/29/2011 - KAD - If the hmc_start applet returns 'Pause' (pause is a delay imposed by host meter; e.g.
            power up time delay or host meter time-out due to an abort), the applet scheduler time-out is reset to prevent
            re-initializing hmc_start applet.  This allows us to wait longer than the scheduler normally allows.
            The hmc_start applet may need to wait xx seconds at power up before talking to the host meter.  It may also
            have to wait nn seconds for retries if an abort occurs.
   v0.3 -   10/18/2011 - KAD - Initialized 'sProtocolStatus' at the beginning of the function.
                           - Moved the file static variables for 'HMC_APP_main()' into the function.  They were probably
                             moved outside the function for debugging.
                           - Set initial value of appletIndex_ and appletIndexPersistent_.
 **********************************************************************************************************************/
/* PSECTS */

#ifdef USE_MAPPING
#pragma psect text=hmctxt
#pragma psect const=hmccnst
#pragma psect bss=hmcbss
#pragma psect bigbss=hmcbbss
#endif
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"

#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif

#define HMC_APP_GLOBAL
#include "hmc_app.h"
#undef  HMC_APP_GLOBAL

#include "hmc_tsk_lst.h"
#include "hmc_eng.h"
#include "hmc_msg.h"
#include "timer_util.h"
#include "sys_busy.h"
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "intf_cim_cmd.h"
#endif
#include <wolfssl/quicksession.h>
#include <wolfssl/internal.h>

/* ****************************************************************************************************************** */
/* CONSTANTS */

#define PERSISTENT_INVALID       ((uint8_t)0xFF) /* Invalid value for the Persistent index variable */
#define APPLET_INDEX_LOGIN       ((uint8_t)1)
#define APPLET_INDEX_AFTER_LOGIN ((uint8_t)2)
#define APPLET_INDEX_LOGOFF      ((uint8_t)0)

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#undef NULL
#define NULL 0

#define APPLET_IDLE_COUNT  (uint8_t)20
#if( RTOS_SELECTION == FREE_RTOS )
#define HMC_APP_QUEUE_SIZE 4 //NRJ: TODO: RA6E1 Figure out sizing
#else
#define HMC_APP_QUEUE_SIZE 0
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#ifdef TM_HMC_APP    // this is used for debug - makes variables global ( see CompileSwitch.h )
HMC_COM_INFO   hmcRxTx_;                     /* Main Structure that contains RX and TX data */
uint8_t        appletIndex_ = 0;             /* Index into the array of applications to process */
uint8_t        appletIndexPersistent_ = 0;   /* High Priority apps may require to stay in a sequence of events */
bool           bSendComResponseToApplet_;    /* Time to send response to Applet */
bool           bReInitAllApps_;              /* Initialize should be the 1st thing call anyway! */
bool           bEnabled_;
bool           bProcessApplet_ = false;      /* Process an applet, don't poll the applets, just process */
uint8_t        allAppletsIdlePassCnt_ = APPLET_IDLE_COUNT;
uint8_t        appletResponse_;              /* Applet response when called */
#endif
static uint16_t      hmcAppletTimerId_ = 0;/* Timer Id for applets */ // Timeout in 6 Minutes
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static bool SkipToLogoff = false;
#endif
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
static uint32_t      taskScanDelay_ = HMC_APP_TASK_DELAY_MS;
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#if !RTOS
/***********************************************************************************************************************

   Function name: HMC_APP_powerUp()

   Purpose: Initializes and Enables HMC at power up.

   Arguments: None

   Returns: returnStatus_t - eSUCCESS

 **********************************************************************************************************************/
returnStatus_t HMC_APP_powerUp( void ) // No longer used - RCZ 7/18/18
{
   ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_INIT, ( void * )NULL ); /* Initialize all Meter Applets */
   ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_ENABLE, ( void * )NULL ); /* Enable the Meter Map Application */
   return( eSUCCESS );
}
#endif
/**********************************************************************************************************************

   Function name: HMC_APP_status()

   Purpose: returns true if HMC is logged in to the host meter, false if not.

   Arguments: None

   Returns: bool - true = logged in, false = Idle (not logged in)

 **********************************************************************************************************************/
bool HMC_APP_status( void ) // Not Used - RCZ 2/2/18
{
   bool retVal = false;

   if ( HMC_APP_main( ( uint8_t )HMC_APP_CMD_STATUS, ( void * )NULL ) == ( uint8_t )HMC_APP_STATUS_LOGGED_ON )
   {
      retVal = true;
   }
   return( retVal );
}
/* *********************************************************************************************************************

   Function name: HMC_APP_RTOS_Init

   Purpose: This function is called before starting the scheduler.  Creates message queue used for getting message to
   HMC task.

   Arguments: None

   Returns: eSUCCESS/eFAILURE indication

   Side effects: Creates message queue.

   Reentrant: NO. This function should be called once before starting the scheduler

 **********************************************************************************************************************/
returnStatus_t HMC_APP_RTOS_Init( void )
{
   returnStatus_t retVal = eSUCCESS;   /* Return value */

   /* Create queue needed for Meter Applets */

   if ( !OS_QUEUE_Create( &HMC_APP_QueueHandle, HMC_APP_QUEUE_SIZE, "HMC_APP" ) )
   {
      /* Queue create failed */
      retVal = eFAILURE;
   }

   ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_INIT, NULL ); /* Initialize the HMC App, lower layers and applets. */
   return retVal;
}
#if RTOS
/***********************************************************************************************************************

   Function name: HMC_APP_Task()

   Purpose: Manages the Meter Application in an RTOS environment

   Arguments: void *pvParameters

   Returns: None

 **********************************************************************************************************************/
void HMC_APP_Task( taskParameter )
{
   sysQueueEntry_t   *pSysQueue;
   bool              bHmcBusy = false;

#if ( ANSI_SECURITY == 1 )
   buffer_t *password;
   password = BM_alloc( 20 ); /* Size of ANSI meter passwords  */
   if( password != NULL )
   {
      ( void )HMC_STRT_GetPassword( password->data ); /* Get the meter password(s) from the file and decrypt if necessary. */
      BM_free( password );
   }
#endif
   ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_ENABLE, NULL ); /* Enable the HMC */
   for ( ; ; )
   {
      /* See if there is a message in the queue */
      pSysQueue = OS_QUEUE_Dequeue( &HMC_APP_QueueHandle ); // THIS QUEUE IS NEVER USED! (RCZ,1/25/18)
      if ( NULL != pSysQueue )                            // Is for messages from RTOS ? (RCZ,1/25/18)
      {
         DBG_printf( "HMC_app received a queue message: %lu", pSysQueue ); // TODO: RA6E1 Bob: checking claim that this queue is never used
         /* Got message, this message can be for applet serviced by this task */
         ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_MSG, pSysQueue );
      }
      ( void )HMC_APP_main( ( uint8_t )HMC_APP_CMD_PROCESS, NULL ); /* Process HMC application */

      /* Todo:  This next section may 'go away' if HMC only communicates when a message is received in the queue. */
      if ( HMC_APP_main( ( uint8_t )HMC_APP_CMD_STATUS, NULL ) == ( uint8_t )HMC_APP_STATUS_LOGGED_OFF ) /* Logged in? */
      {
         if ( bHmcBusy )
         {
            bHmcBusy = false; // bHmcBusy is not used anywhere but here
            SYSBUSY_clearBusy();
         }
#ifdef TM_TIME_INTEGRATION_TESTING
         OS_TASK_Sleep( HMC_APP_TASK_DELAY_MS_TST ); /* Test Code! */
#else
   #if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
         if ( taskScanDelay_ > 0 )
   #endif
         {
            OS_TASK_Sleep( HMC_APP_TASK_DELAY_MS ); /* Since we're NOT logged in, we're just scanning applets. */
         }
#endif
      }
      else
      {
         if ( !bHmcBusy )
         {
            bHmcBusy = true;
            SYSBUSY_setBusy();
         }
   #if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
         if ( taskScanDelay_ > 0 )
   #endif
         {
            OS_TASK_Sleep( 5 );
         }
      }
   }
}  /*lint !e715 !e818  pvParameters is not used */
#endif
#if RTOS
/***********************************************************************************************************************

   Function name: HMC_APP_TaskPowerDown()

   Purpose: Ends the HMC Task

   Arguments: returnStatus_t

   Returns: None

 **********************************************************************************************************************/
returnStatus_t HMC_APP_TaskPowerDown( void )
{
   //LED_HMC_OFF();
   //Todo:  Shut off the UART
#if ENABLE_HMC_TASKS
   OS_TASK_ExitId( pTskName_Hmc );
#endif
   return( eSUCCESS );
}  /*lint !e715 !e818  pvParameters is not used */
#endif

returnStatus_t HMC_APP_ResetAppletTimeout(void)
{
  returnStatus_t retVal;
  retVal = TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_APPLET_TIMEOUT_mS );
  return retVal;
}

/**********************************************************************************************************************

   Function name: HMC_APP_main()

   Purpose: Checks all of the applets to see if any meter com needs to take place.
            Calls the Logoff or login functions when needed.

   Arguments: uint8_t ucCmd:  Command list is in the .h file

   Returns:    uint8_t
               When Called with HMC_APP_CMD_PROCESS command
                  HMC_APP_API_LOW_CPU:  Very little time taken in module
                  HMC_APP_API_HIGH_CPU: Time taken to process task (CRC16 algorithm invoked or data read/written)
               When Called with HMC_APP_STATUS command
                  HMC_APP_API_LOGGED_OFF:  Meter Session is closed (logged out)
                  HMC_APP_API_LOGGED_ON:   Meter Session is open (logged in)
               When Called with an Unkown command:
                  HMC_APP_API_INV_CMD:  Unknown Command

 **********************************************************************************************************************/
uint8_t HMC_APP_main( uint8_t ucCmd, void *pData )
{
#ifndef TM_HMC_APP  // this is used for debug - makes variables global ( see CompileSwitch.h )
   static HMC_COM_INFO  hmcRxTx_ = {0};            /* Main Structure that contains RX and TX data */
   static uint8_t       appletIndex_ = 0;          /* Index into the array of applications to process */
   static uint8_t       appletIndexPersistent_ = 0;/* High Priority apps may require to stay in a sequence of events */
   static bool          bSendComResponseToApplet_; /* Time to send response to Applet */
   static bool          bReInitAllApps_;           /* Initialize should be the 1st thing call anyway! */
   static bool          bEnabled_;
   static bool          bProcessApplet_ = false;   /* Process an applet, don't poll the applets, just process */
   static uint8_t       allAppletsIdlePassCnt_ = APPLET_IDLE_COUNT; // counts down from 20, once each time finishes the applet list
   static uint8_t       appletResponse_;           /* Applet response when called */
#endif
   timer_t tmrCfg;

   HMC_PROTO_STATUS sProtocolStatus;         /* Status of the protocol layer, see bit definition in the hmc_proto.h */
   uint8_t ucComResultToApplet;                /* Response to send to the applet */
   uint8_t ucRetVal = ( uint8_t )HMC_APP_PROC_LOW_CPU; /* Value to return from this function, assume low CPU Time */
   enum HMC_APP_API_RPLY eLogInStatus;       /* Keeps track of the Log In Status, see .h file for values */
   uint8_t ucAppletIndexTmp;                   /* Used to keep track of the index when sending a message */

   sProtocolStatus.uiStatus = 0; /* Clear all status bits */
   eLogInStatus = ( enum HMC_APP_API_RPLY )HMC_STRT_LogOn( ( uint8_t )HMC_APP_API_CMD_LOGGED_IN, ( uint8_t far * )0 );
   switch ( ucCmd )  /* Figure out what command was sent to this function */
   {
      case HMC_APP_CMD_INIT:  /* Initializes MTR App Task and all other applets */
      {
         if ( 0 == hmcAppletTimerId_ )
         {
            timer_t tmrSettings; /* Used to setup the timer */

            ( void )memset( &tmrSettings, 0, sizeof( tmrSettings ) );
            tmrSettings.bOneShot = true;
            tmrSettings.ulDuration_mS = HMC_APP_APPLET_TIMEOUT_mS;
            if ( eSUCCESS == TMR_AddTimer( &tmrSettings ) )
            {
               hmcAppletTimerId_ = tmrSettings.usiTimerId;
            }
         }

         ( void )TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_APPLET_TIMEOUT_mS );
         bReInitAllApps_ = true;  /* Set flag to initialize the application and all applets */
         bEnabled_ = false;
         appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         /* The initialization code will be performed before returning below.  */
         break;
      }

      case HMC_APP_CMD_DISABLE:  /* Initializes MTR App Task and all other applets */
      {
         ucRetVal = ( uint8_t )HMC_APP_DISABLED;
         if ( HMC_APP_API_RPLY_LOGGED_OFF == eLogInStatus )
         {
            bEnabled_ = false;
            ( void )TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_DISABLED_TIMER_mS );
            appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         }
         if ( bEnabled_ )
         {
            ucRetVal = ( uint8_t )HMC_APP_ENABLED;
         }
         break;
      }

      case HMC_APP_CMD_ENABLE:  /* Initializes MTR App Task and all other applets */
      {
         bEnabled_ = true;
         ucRetVal = ( uint8_t )HMC_APP_ENABLED;
         appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         break;
      }

      case HMC_APP_CMD_TIME_TICK:  /* Time Tick */  // Not Used Anywhere (RCZ - 1/29/18)
      {
#ifndef RTOS
         if ( appletTimer_ > 0 )
         {
            appletTimer_--;
         }
#endif
         break;
      }

      case HMC_APP_CMD_SET_COM_DEFAULTS: /* Sets meter communication default parameters */ // Not Used Anywhere (RCZ - 1/29/18)
      {
         ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_APP_CMD_SET_COM_DEFAULTS, &hmcRxTx_ );
         break;
      }

      case HMC_APP_CMD_SET_COMPORT: /* Sets meter communication default parameters */ // Not Used Anywhere (RCZ - 1/29/18)
      {
         ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_WRITE_COMPORT, pData );
         break;
      }

      case HMC_APP_CMD_SET_PSEM_ID: /* Sets PSEM ID Byte */ // Not Used Anywhere (RCZ - 1/29/18)
      {
         ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_SET_ID, pData );
         break;
      }

      case HMC_APP_CMD_STATUS:   /* The calling application wishes to know if the session is open or closed */
      {
         /* Some other application may need to know if the meter application is running.  That application
            can call HMC_APP_TASK with HMC_APP_CMD_STATUS command to get the status. Logging on counts as
            logged on.  */
         ucRetVal = ( uint8_t )HMC_APP_STATUS_LOGGED_ON;
         if ( eLogInStatus == HMC_APP_API_RPLY_LOGGED_OFF )
         {
            ucRetVal = ( uint8_t )HMC_APP_STATUS_LOGGED_OFF; /* Set the response for the status command */
         }
         break;
      }

      case HMC_APP_CMD_PROCESS:  /* Process all applets here */
      {
         if ( bEnabled_ )
         {
            if ( !bProcessApplet_ )
            {
               /* Update all lower layers of code */
               sProtocolStatus.uiStatus = HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_PROCESS, &hmcRxTx_ );
               ucComResultToApplet = ( uint8_t )HMC_APP_API_CMD_STATUS; /* Do Status */
               /* Is the protocol layer busy? If not busy, see if there is anything to do */
               if ( !sProtocolStatus.Bits.bBusy )
               {
                  /* When not logged into the host meter and the message layer is NOT busy (the last ACK has been sent)
                     the TX pin will be set to high impedance, and the LED indicator turned off. */
                  if ( HMC_APP_API_RPLY_LOGGED_OFF == eLogInStatus )
                  {
                     //LED_HMC_OFF();
                  }

                  /* Meter Protocol Not busy, we can check if anything is to be done */

                  /* ------------------------------------------------------------------------------------------------ */
                  /* Process the Response of a processed command to the host meter. */

                  /* Was a command just processed?  If so notify the calling application */
                  //lint -e{727} Initialized before process is called, in the init.
                  if ( bSendComResponseToApplet_ )
                  {
                     bSendComResponseToApplet_ = false;

                     ucComResultToApplet = ( uint8_t )HMC_APP_API_CMD_MSG_COMPLETE; /* Init to Complete */
                     if ( sProtocolStatus.Bits.bRestart )
                     {
                        ucComResultToApplet = ( uint8_t )HMC_APP_API_CMD_ABORT; /* The applet has to abort */
                        HMC_ENG_Execute( ( uint8_t )HMC_ENG_ABORT, HMC_ENG_VARDATA );
                     }
                     else if ( sProtocolStatus.Bits.bGenError )
                     {
                        /* If there was a Error, notify the HMC_STRT applet.  It may decide to stop communications */
                        ucComResultToApplet = ( uint8_t )HMC_APP_API_CMD_ERROR;
                        HMC_ENG_Execute( ( uint8_t )HMC_ENG_GEN_ERROR, HMC_ENG_VARDATA );
                     }
                     else if ( sProtocolStatus.Bits.bTblError )
                     {
                        /* If there was a Table Error, notify the HMC_STRT applet. It may decide to stop
                           communications */
                        ucComResultToApplet = ( uint8_t )HMC_APP_API_CMD_TBL_ERROR;
                     }
                     else if ( sProtocolStatus.Bits.bInvRegister )
                     {
                        /* If there was an ISC, notify the HMC_STRT applet.  It may decide to stop communications */
                        ucComResultToApplet = ( uint8_t )HMC_APP_API_CMD_INV_REGISTER;
                        HMC_ENG_Execute( ( uint8_t )HMC_ENG_INV_REG, HMC_ENG_VARDATA );
                     }
                  }

                  /* ------------------------------------------------------------------------------------------------ */
                  /* Validate the Index variables */

                  /* Verify that the pointer index is in range and if we need to wrap back around */
                  if ( ( HMC_APP_MeterFunctions[appletIndex_] == NULL ) || ( appletIndex_ >= HMC_APP_NumOfApplets ) )
                  {
                     appletIndex_ = 0;   /* Finished the list, reset index */
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
                     // Signal to enter Program or Audit section. The Audit section is
                     // followed by DFW issuing a reset, so hmcPrgMtrIsSynced is left set.
                     if ( HMC_PRG_MTR_IsStarted( ) )
                     {
                        if ( !HMC_PRG_MTR_IsSynced( ) )
                        {
                           // Empty queue
                           HMC_PRG_MTR_SetSync( ); // hmcPrgMtrIsSynced = true;
                           HMC_PRG_MTR_SwitchTableAccessFunctions ( );
                           HMC_PRG_MTR_SyncQueue ( );
                           // Now force a logoff, then logon with
                           // hmcPrgMtrIsSynced set.
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           allAppletsIdlePassCnt_ = 0;
                           appletIndexPersistent_ = PERSISTENT_INVALID;
                           eLogInStatus = HMC_APP_API_RPLY_LOGGED_IN;
                           SkipToLogoff = true;
                        }
                     }
                     if ( !HMC_PRG_MTR_IsStarted( ) )
                     {
                        if ( HMC_PRG_MTR_IsSynced( ) )
                        {
                           // Empty queue

                           HMC_PRG_MTR_ReturnTableAccessFunctions ( );
                           HMC_PRG_MTR_ClearSync( ); // hmcPrgMtrIsSynced = false;
                           HMC_PRG_MTR_SyncQueue ( );
                           // Now force a PSEM Terminate.
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           allAppletsIdlePassCnt_ = 0;
                           appletIndexPersistent_ = PERSISTENT_INVALID;
                           eLogInStatus = HMC_APP_API_RPLY_LOGGED_IN;
                           SkipToLogoff = true;
                        }
                     }


#endif
                     if ( allAppletsIdlePassCnt_ > 0 )
                     {
                        allAppletsIdlePassCnt_--;
                     }
                  }
                  /* Verify that the Persistent pointer index is in range, if INVALID, this will set to INVALID again */
                  if ( appletIndexPersistent_ >= HMC_APP_NumOfApplets )
                  {
                     appletIndexPersistent_ = PERSISTENT_INVALID;
                  }

                  /* Is the session now Open? */
                  if (  ( HMC_APP_API_RPLY_LOGGED_IN == eLogInStatus ) &&
                        ( appletIndexPersistent_ != PERSISTENT_INVALID ) &&
                        ( ( uint8_t )HMC_APP_API_CMD_ABORT != ucComResultToApplet ) )
                  {
                     /* The Host meter session is open, start with the persistent applet 1st */
                     appletIndex_ = appletIndexPersistent_;
                  }

                  /* ------------------------------------------------------------------------------------------------ */
                  /* Poll the applets to see if anything is to be done */

                  /* check if any other applet has something to do if Applet response above was HMC_APP_API_RPLY_IDLE */
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
                  if ( ( HMC_APP_MeterFunctions[appletIndex_] != NULL ) && ( appletIndex_ < HMC_APP_NumOfApplets ) && !SkipToLogoff )
#else
                  if ( ( HMC_APP_MeterFunctions[appletIndex_] != NULL ) && ( appletIndex_ < HMC_APP_NumOfApplets ) )
#endif
                  {
                     if ( ( uint8_t )HMC_APP_API_RPLY_IDLE == appletResponse_ ) /* Response above could have been a command */
                     {
                        appletResponse_ = HMC_APP_MeterFunctions[appletIndex_]( ucComResultToApplet,
                                          ( void far * )&hmcRxTx_ );
                        /* If the applet was told to abort, abort the session no matter what the applet says. */
                        if ( ( uint8_t )HMC_APP_API_CMD_ABORT == ucComResultToApplet )
                        {
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_ABORT_SESSION;
                        }
                        /* Get the Login status.  The result of the last communication could have changed the status. */
                        eLogInStatus =
                           ( enum HMC_APP_API_RPLY )HMC_STRT_LogOn( ( uint8_t )HMC_APP_API_CMD_LOGGED_IN, ( uint8_t far * )0 );
                     }
                     ucComResultToApplet = ( uint8_t )HMC_APP_API_CMD_STATUS;

                     /* Is the persistant applet taken care of?  If so, clear the persistant index out. */
                     switch ( appletResponse_ )
                     {
                        case HMC_APP_API_RPLY_READY_COM:
                        {
                           bProcessApplet_ = true;
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           break;
                        }

                        case HMC_APP_API_RPLY_RDY_COM_PERSIST:
                        {
                           bProcessApplet_ = true;
                           appletIndexPersistent_ = appletIndex_;
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           break;
                        }

                        case HMC_APP_API_RPLY_IDLE:  /* Nothing to do?  Check next task */
                        {
                           if ( appletIndex_ == appletIndexPersistent_ )   /* is the persistant applet now IDLE? */
                           {
                              /* The persistent applet has been satisfied */
                              appletIndexPersistent_ = ( uint8_t )PERSISTENT_INVALID;
                           }
                           appletIndex_++;  /* Process the next applet */
                           /* Don't want a persistent application to get stuck either.  A persistent application could
                              go through log-off, log-on and then the persistent app over and over again.  If the index
                              incremented outside of the log-off/on index range, then a persistent app isn't stuck.  */
                           if ( appletIndex_ >= APPLET_INDEX_AFTER_LOGIN )
                           {
                              ( void )TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_APPLET_TIMEOUT_mS ); // 6 minutes
                           }
                           break;
                        }

                        case HMC_APP_API_RPLY_PAUSE:  /* Applet needs more time. */ // RCZ (1/29/18) returned once from hmc_ds.c
                        {
                           if ( APPLET_INDEX_LOGIN == appletIndex_ )
                           {
                              ( void )TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_APPLET_TIMEOUT_mS );
                           }
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           break;   /* Don't close the session, but don't do com either */
                        }

                        case HMC_APP_API_RPLY_ABORT_SESSION:   /* Check if we are to abort */
                        {
                           if ( HMC_APP_API_RPLY_LOGGED_OFF != eLogInStatus )
                           {
                              /* Call start because we may need a delay after an abort. */
                              ( void )HMC_STRT_LogOn( ( uint8_t )HMC_APP_API_CMD_ABORT, ( uint8_t far * )0 );
                              ( void )HMC_FINISH_LogOff( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, ( uint8_t far * )0 );
                           }
                           appletIndex_ = 0;
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           break;
                        }

                        case HMC_APP_API_RPLY_GET_COM_PARAM:/* Read communication parameters? */ // Not Used Anywhere (RCZ - 1/29/18)
                        {
                           /* Get the com parameters */
                           ( void )HMC_PROTO_Protocol( HMC_PROTO_CMD_READ_COM_PARAM,
                                                       ( void far * )hmcRxTx_.pCommandTable );
                           /* allow the applet to update the com parameters */
                           appletResponse_ = ( uint8_t )HMC_APP_MeterFunctions[appletIndex_]( ( uint8_t )HMC_APP_API_CMD_COM_PARAM,
                                             ( void far * )&hmcRxTx_ );
                           break;
                        }

                        case HMC_APP_API_RPLY_WRITE_COM_PARAM:/* Write communication parameters? */ // Not Used Anywhere (RCZ - 1/29/18)
                        {
                           ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_WRITE_COM_PARAM,
                                                       ( void far * )hmcRxTx_.pCommandTable );
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           break;
                        }

                        case HMC_APP_API_RPLY_GET_COMPORT_PARAM:  /* update Comport settings? */ // Not Used Anywhere (RCZ - 1/29/18)
                        {
                           /* Get the com parameters */
                           ( void )HMC_PROTO_Protocol( HMC_PROTO_CMD_READ_COMPORT, ( void far * )hmcRxTx_.pCommandTable );
                           appletResponse_ = ( uint8_t )HMC_APP_MeterFunctions[appletIndex_]( ( uint8_t )HMC_APP_API_CMD_COMPORT_PARAM,
                                             ( void far * )&hmcRxTx_ );
                           break;
                        }

                        case HMC_APP_API_RPLY_WRITE_COMPORT_PARAM:  /* update Comport settings? */  // Not Used Anywhere (RCZ - 1/29/18)
                        {
                           ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_WRITE_COMPORT,
                                                       ( void far * )hmcRxTx_.pCommandTable );
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           break;
                        }

                        case HMC_APP_API_RPLY_RE_INITIALIZE: // Not Used Anywhere (RCZ - 1/29/18)
                        {
                           if ( HMC_APP_MeterFunctions[appletIndex_]( ( uint8_t )HMC_APP_API_CMD_RINIT_VERIFY,
                                 ( void far * )&hmcRxTx_ ) ==
                                 ( uint8_t )HMC_APP_API_RPLY_RE_INIT_VERIFIED )
                           {
                              /* Update Eng with Re-Initialize */
                              HMC_ENG_Execute( ( uint8_t )HMC_ENG_RE_INIT, HMC_ENG_VARDATA );
                              bReInitAllApps_ = true; /* Set bit to re-initialize the entire meter com and applets */
                           }
                           else  /* Applet responded differently, there is something wrong, re-initialize that applet */
                           {
                              /* Update Eng with Applet Error */
                              HMC_ENG_Execute( ( uint8_t )HMC_ENG_APPLET_ERROR, HMC_ENG_VARDATA );
                              ( void )HMC_APP_MeterFunctions[appletIndex_]( ( uint8_t )HMC_APP_API_CMD_INIT,
                                    ( void far * )&hmcRxTx_ );
                              appletIndex_++;  /* See if the next applet has something to do */
                              appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE; /* Don't break out of the while loop */
                           }
                           break;
                        }

                        default:
                        {
                           appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                           ( void )HMC_APP_MeterFunctions[appletIndex_]( ( uint8_t )HMC_APP_API_CMD_INV_RESPONSE,
                                 ( void far * )&hmcRxTx_ );
                           appletIndex_++;  /* Process the next applet */
                           ( void )TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_APPLET_TIMEOUT_mS );
                           /* Update Eng with Applet Error */
                           HMC_ENG_Execute( ( uint8_t )HMC_ENG_APPLET_ERROR, HMC_ENG_VARDATA );
                           break;
                        }

                     }
                  }
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
                  SkipToLogoff = false;
#endif
               }
               /* --------------------------------------------------------------------------------------------------- */
               /* Process Applet Code, send the command to the host meter or start a login or logoff process */

               /* Nothing to send to the host meter.  If the status is logged in, then we must log off. */
               if (  ( ( uint8_t )HMC_APP_API_RPLY_LOGGED_OFF != ( uint8_t )eLogInStatus ) &&
                     ( ( uint8_t )HMC_APP_API_RPLY_IDLE == appletResponse_ )  &&
                     ( 0 == allAppletsIdlePassCnt_ ) ) /* Only place allAppletsIdlePassCnt_ is tested - counts down from 20 each time finishes the
                                                          applet list */
               {
                  /* Logged in, start the log off process */
                  ( void )HMC_FINISH_LogOff( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, ( uint8_t far * )0 );
                  appletIndex_ = 0; /* Reset the index to ensure we'll log off */
               }
            }

            else  /* If application has something to do, start doing it */
            {
               bProcessApplet_ = false;

               /* Check for an invalid state - The state should NEVER be logging in 'HMC_APP_API_RPLY_LOGGING_IN' and
                  logging off at the same time.  If this happens, increment the index and try to log off. */
               if ( ( ( uint8_t )HMC_APP_API_RPLY_LOGGING_IN == ( uint8_t )eLogInStatus ) &&
                     ( ( uint8_t )APPLET_INDEX_LOGOFF == appletIndex_ ) ) // RCZ corrected (1/29/18) - was HMC_APP_API_CMD_LOG_OFF == appletIndex_
               {
                  appletIndex_++;
               }

               /* Are we logged into the meter? */
               if ( ( HMC_APP_API_RPLY_LOGGED_IN == eLogInStatus ) ||
                     ( ( HMC_APP_API_RPLY_LOGGING_IN == eLogInStatus ) && ( APPLET_INDEX_LOGIN == appletIndex_ ) ) ||
                     ( ( HMC_APP_API_RPLY_LOGGING_IN == eLogInStatus ) && ( APPLET_INDEX_LOGOFF == appletIndex_ ) ) )
               {
                  /* We are already logged in, start another packet */
                  appletResponse_ = HMC_APP_MeterFunctions[appletIndex_]( ( uint8_t )HMC_APP_API_CMD_PROCESS,
                                    ( void far * )&hmcRxTx_ );
                  if (  ( ( uint8_t )HMC_APP_API_RPLY_READY_COM == appletResponse_ ) ||
                        ( ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST == appletResponse_ ) )
                  {
                     /* previous response is ready, so Launch the new command */
                     appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
                     bSendComResponseToApplet_ = true; /* Process the response after the protocol layer is complete */
                     ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_NEW_CMD, &hmcRxTx_ );
                     allAppletsIdlePassCnt_ = APPLET_IDLE_COUNT;
                  }
               }
               else
               {
                  /* Not logged in, start the log in process.  Can't send a message unless we're logged in */
                  ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_NEW_SESSION, &hmcRxTx_ );
                  ( void )HMC_STRT_LogOn( ( uint8_t )HMC_APP_API_CMD_ACTIVATE, ( uint8_t far * )0 );
                  appletIndex_ = 0;   /* Process the log on first, reset the index */
                  HMC_ENG_Execute( ( uint8_t )HMC_ENG_TOTAL_SESSIONS, HMC_ENG_VARDATA ); /* Increment Session Count */
                  allAppletsIdlePassCnt_ = APPLET_IDLE_COUNT;
               }
            }

         } /* end of "if ( bEnabled_ )" */
         /* If a CRC was performed in message layer or data read/written in the protocol layer, CPU time was taken */
         if ( sProtocolStatus.Bits.bCPUTime )
         {
            ucRetVal = ( uint8_t )HMC_APP_PROC_HIGH_CPU;
         }
         else  /* Disabled, check for disable time out */
         {
            tmrCfg.usiTimerId = hmcAppletTimerId_;
            if ( eSUCCESS == TMR_ReadTimer( &tmrCfg ) ) /* Get timer information */
            {
               if ( tmrCfg.bExpired )
               {
                  bEnabled_ = true;
               }
            }
         }
         break;
         } /* end of "case HMC_APP_CMD_PROCESS:" */

      case HMC_APP_CMD_MSG:   /* The calling application is sending a message, send it to all applications -- Currently Not Used - RTOS message (RCZ -
                                 1/29/18) */
      {
         ucAppletIndexTmp = 0;
         while ( HMC_APP_MeterFunctions[ucAppletIndexTmp] != 0 )    /* Send msg to all applets */
         {
            ( void )HMC_APP_MeterFunctions[ucAppletIndexTmp++]( ( uint8_t )HMC_APP_API_CMD_MSG, pData );
         }
         break;
      }

      default: /* Somebody sent something that doesn't make sense! */
      {
         ucRetVal = ( uint8_t )HMC_APP_INV_CMD;
         break;
      }

   }

   tmrCfg.usiTimerId = hmcAppletTimerId_;
   if ( eSUCCESS == TMR_ReadTimer( &tmrCfg ) ) /* Get timer information */
   {
      if ( tmrCfg.bExpired && bEnabled_ )
      {
         /* Re-initialize the applet, something is wrong! */
         ( void )HMC_APP_MeterFunctions[appletIndex_]( ( uint8_t )HMC_APP_API_CMD_INIT, ( void far * )&hmcRxTx_ );
         appletIndex_++;  /* Next time Process the next applet */
         ( void )TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_APPLET_TIMEOUT_mS );

         bSendComResponseToApplet_ = false;  /* Don't send response to applet (com could be in progress) */
         HMC_ENG_Execute( ( uint8_t )HMC_ENG_APPLET_TIMER_OUT, HMC_ENG_VARDATA ); /* Update Eng with Applet Time Out */
      }
   }
   /* Initialize all applets and Meter Com Code? */
   if ( bReInitAllApps_ )
   {
      bProcessApplet_ = false;
      bReInitAllApps_ = false;
      ( void )HMC_PROTO_Protocol( ( uint8_t )HMC_PROTO_CMD_INIT, &hmcRxTx_ ); /* Initialize the lower layers */
      appletIndex_ = 0;                                    /* Start at Index 0 */
      while ( HMC_APP_MeterFunctions[appletIndex_] != 0 )    /* Initialize all applets */
      {
         ( void )HMC_APP_MeterFunctions[appletIndex_++]( ( uint8_t )HMC_APP_API_CMD_INIT, ( void * )&hmcRxTx_ );
      }
      appletIndex_ = 0;                                    /* Reset Index again */
      appletIndexPersistent_ = PERSISTENT_INVALID;         /* Clear the persistant index */
      ucRetVal = ( uint8_t )HMC_APP_STATUS_LOGGED_OFF;       /* Set the return value to closed */
      bSendComResponseToApplet_ = false;                   /* Set the Com Response flag to false */
      ( void )TMR_ResetTimer( hmcAppletTimerId_, HMC_APP_APPLET_TIMEOUT_mS );
      appletResponse_ = ( uint8_t )HMC_APP_API_RPLY_IDLE;
   }
   return( ucRetVal );
}

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
/**********************************************************************************************************************

   Function name: HMC_APP_SetScanDelay()

   Purpose: Allow the delay between scans of Applets to be altered externally

   Arguments: uint32_t delay - number of milliseconds to delay between scans

   Returns:    nothing

 **********************************************************************************************************************/
void     HMC_APP_SetScanDelay(uint32_t delay)
{
   taskScanDelay_ = delay;
}
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )