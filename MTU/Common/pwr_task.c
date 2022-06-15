/***********************************************************************************************************************

   Filename:   pwr_task.c

   Global Designator: PWR_

   Contents:   Power Task - Monitors power down condition. The module creates a semaphore and mutex when initialized.
               The task then waits for a semaphore given by the power down ISR. Then waits for the mutex to start
               calling a list of functions to get the system ready to power down. The last item in the power down list
               should be flushing the cache memory. Once the cache memory is flushed, the unit will wait for power to
               die. If power should come back and the unit 'debounces' the power signal, the processor will reset.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013 - 2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$ kdavlin Created May 6, 2013

 **********************************************************************************************************************/
/* INCLUDE FILES */
#include "project.h"
#if ( MCU_SELECTED == NXP_K24 )
#include <mqx.h>
#endif
#include "pwr_task.h"
#include "pwr_last_gasp.h"
#include "file_io.h"
#include "pwr_restore.h"
#include "pwr_config.h"
#include "hmc_app.h"
#include "mode_config.h"
#include "time_util.h"
#include "demand.h"
#include "MAC.h"
#include "PHY.h"
#include "SM.h"
#if (USE_MTLS == 1)
#include "mtls.h"
#endif
#if ( USE_DTLS == 1)
#include "dtls.h"
#endif
#include "radio_hal.h"
#include "RG_MD_Handler.h"
#include "buffer.h"

#include "vbat_reg.h"
#if ( MCU_SELECTED == NXP_K24 )  /* TODO: RA6E1: Add includes for RA6 */
#include "fio.h"           /* For ecc108_mqx.h" */
#include "ecc108_mqx.h"    /* For the delay_xx functions */
#endif
#include "EVL_event_log.h"
#if ( ENABLE_METER_EVENT_LOGGING != 0 )
#include "ALRM_Handler.h"    /* For the delay_xx functions */
#endif
#if ( LP_IN_METER == 0 )
#include "ID_intervaltask.h"
#endif
#include "historyd.h"

/* MACRO DEFINITIONS */
#define POWER_DOWN_SIGNATURE ((uint64_t)0x01020304abcdef7A)

#if ( MCU_SELECTED == NXP_K24 )
/* The following macro is in bsp.h, however, pclint gives many warnings over the partitions.h structures and bsp.h
   structures. So, this isn't the best idea to put this here, however, at this moment in the project, I don't have
   time to figure it all out. So I'm adding this label:  TODO  */
#define _bsp_int_init(num, prior, subprior, enable)     _nvic_int_init(num, prior, enable)
#endif

#if ( SIMULATE_POWER_DOWN == 1 )
OS_SEM_Obj    PWR_SimulatePowerDn_Sem;
#endif

/* TYPE DEFINITIONS */
typedef returnStatus_t ( *tFPtr_v )( void );
typedef struct
{
   tFPtr_v fptr;
} exeTable_t;

/* FUNCTION PROTOTYPES */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
static FileHandle_t  fileHndlPowerDownCount = {0};       /* Contains the powerDown file handle information. */
static OS_MUTEX_Obj  PWR_Mutex;                          /* Serialize access to the power module */
static OS_MUTEX_Obj  Embedded_Pwr_Mutex;                 /* Used to protect NV and last gasp updates at restoration */
static OS_SEM_Obj    PWR_Sem;                            /* Used for the power down interrupt */
static uint16_t      rstReason_;                         /* Contains the reason for executing the reset vector. */
static pwrFileData_t pwrFileData = {0};
#if ( SIMULATE_POWER_DOWN == 1 )
static OS_TICK_Struct   PWR_startTick_  = {0};
static OS_TICK_Struct   PWR_endTick_    = {0};
#endif
/* Power Down Table - Define all modules that require a call-back for powering down below. */
static exeTable_t powerDownTbl[] =
{
   ADC_ShutDown,
#if ( MCU_SELECTED == RA6E1 )
   CRC_Shutdown,
#endif
#if 1  /* TODO: RA6E1: Remove */
   NULL
#else
#if ( ENABLE_HMC_TASKS == 1 )
   HMC_APP_TaskPowerDown   /* Shut down the HMC application */
#endif   /* end of ENABLE_HMC_TASKS  == 1 */
#endif
};

#define PWR_COMMON_CALLS ADC_ShutDown

#if ( ENABLE_HMC_TASKS == 1 )
#define PWR_HMC_CALLS ,HMC_APP_TaskPowerDown   /* Shut down the HMC application */
#else   /* end of ENABLE_HMC_TASKS  == 1 */
#define PWR_HMC_CALLS
#endif


/* LOCAL FUNCTION DEFINITIONS */

static void           ShipMode( void );
static void           PowerGoodDebounce( void );
static returnStatus_t PowerFailDebounce( void );

/* FUNCTION DEFINITIONS */

#if ( MCU_SELECTED == RA6E1 )
/***********************************************************************************************************************

   Function Name: brownOut_isr_init

   Purpose: Initialize and configure the brown Out ISR(isr_brownOut) on RA6E1

   Arguments: None

   Returns: fsp_err_t

   Reentrant Code: No

 **********************************************************************************************************************/
static fsp_err_t brownOut_isr_init( void )
{
   fsp_err_t err = FSP_SUCCESS;

   /* Open external IRQ/ICU module */
   err = R_ICU_ExternalIrqOpen( &pf_meter_ctrl, &pf_meter_cfg );
   if(FSP_SUCCESS == err)
   {
      /* Enable ICU module */
      err = R_ICU_ExternalIrqEnable( &pf_meter_ctrl );
   }
   return err;
}
#endif
/***********************************************************************************************************************

   Function Name: PWR_task

   Purpose: Power down task - This should be the highest priority task in the system. This task simply pends until a
            power down event occurs. Once the power down event occurs, it will call all applications (that subscribed),
            flush all partitions and wait to die. If the power down signal comes back (and is de-bounced), the system
            will reset.

            At the start of this task, the reason we went through the reset vector will be saved.

   Arguments:  uint32_t Arg0

   Returns: None

   Side Effects:

   Reentrant Code: No

 **********************************************************************************************************************/
void PWR_task( taskParameter )
{
   returnStatus_t powerFail = eFAILURE;

   DBG_logPrintf( 'I', "PWR_task starting." );

#if 1 // TODO: RA6: DG: Test code
   sysTime_dateFormat_t RTC_time;
   if( !RTC_isRunning() )
   {
      printf("RTC Stopped\n");
   }
   else
   {
      RTC_GetDateTime(&RTC_time);
      INFO_printf("RTC Time:  %02u/%02u/%04u %02u:%02u:%02u.%03u",RTC_time.month, RTC_time.day, RTC_time.year, RTC_time.hour, RTC_time.min, RTC_time.sec, RTC_time.msec);
   }
#endif
#if 0
   ( void ) DBG_CommandLine_GetHWInfo ( 0, NULL );
#endif

   // Do ship mode processing if necessary.
   ShipMode();

   // Signal to outage restoration that the power task has initialized.
   PWROR_PWR_signal();

#if ( MCU_SELECTED == NXP_K24 )
   // Install ISR for Brown Out
   if ( NULL != _int_install_isr( BRN_OUT_IRQIsrIndex, ( INT_ISR_FPTR )isr_brownOut, ( void * )NULL ) )
   {
      ( void )_bsp_int_init( BRN_OUT_IRQIsrIndex, BRN_OUT_ISR_PRI, BRN_OUT_ISR_SUB_PRI, ( bool )true );
   }
   BRN_OUT_IRQ_EI();
#elif ( MCU_SELECTED == RA6E1 ) /*  RA6 */
   ( void )brownOut_isr_init();
#endif
#if 1 /* TEST */ // TODO: RA6E1: DG: Remove later 
   PWRLG_print_LG_Flags();
#endif
   // Loop until receiving a valid PF_METER
   while ( eFAILURE == powerFail )
   {
      // Enable the Interrupt
      BRN_OUT_IRQ_EI();

      ( void )OS_SEM_Pend( &PWR_Sem, OS_WAIT_FOREVER ); // Wait for power down semaphore!
      OS_SEM_Reset( &PWR_Sem );

      /******************************************************************************************************
                                                  POWER FAIL DETECTED!
      *******************************************************************************************************/

      powerFail = PowerFailDebounce(); //Returns eSUCCES if power fail debounce indicates power fail is immenent
   }

#if ( LAST_GASP_SIMULATION == 1 )
   /* Cancel if any LG Simulation is in progress during Real Last Gasp*/
   EVL_LGSimCancel();
#endif
   /* If running from Boost regulator such as during the noise estimate switch back to the LDO to preserve the Super cap energy
      while preparing to shut down */
   if ( PWR_BOOST_IN_USE() )
   {
      //Need this to keep from allowing task switch
      PWR_USE_LDO();
   }

   //Radio shutdown to ensure transmissions are disabled ASAP
   radio_hal_RadioImmediateSDN();

   /* Before we can continue the power down process, we need to make sure no tasks are in the middle of updating
      critical information in NV or important system status flags tied to changes in NV such as last gasp flags.
      Lock the power mutex in the PWR_AND_EMBEDDED_PWR_MUTEX state.  This will ensure all NV writes have completed,
     important system flags have been updated, and the device is ready to complete the power down process.  Once
     the mutex is acquired, the power module will not release the mutex in order to prevent other tasks from making
     updates and will be released upon power restoration. */
   PWR_lockMutex( PWR_AND_EMBEDDED_PWR_MUTEX );

   /* Call all items in the power down list */
   /* Initialize all of the modules here: */
   {
      uint8_t     powerDownIdx;
      exeTable_t  *pFunct;

      for ( powerDownIdx = 0, pFunct = &powerDownTbl[0]; powerDownIdx < ARRAY_IDX_CNT( powerDownTbl ); powerDownIdx++, pFunct++ )
      {
         if ( NULL != pFunct->fptr )
         {
_Pragma ( "calls = \
                 PWR_COMMON_CALLS \
                 PWR_HMC_CALLS \
                 " )
            ( void )pFunct->fptr();
         }
      }
   }
#if ( SIMULATE_POWER_DOWN == 1 )
   if( DBG_CommandLine_isPowerDownSimulationFeatureEnabled() ) // Is the feature Enabled ?
   {
      PWR_startTick_ = DBG_CommandLine_GetStartTick();
      // TODO: Make below lines a function call
      pwrFileData.uPowerDownSignature  = 0;  /* Clear power down signature */
      pwrFileData.outageTime           = 0;
      VBATREG_PWR_QUAL_COUNT           = 0;  /* Clear the RAM based power quality count.      */
      ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* ) &pwrFileData, ( lCnt )sizeof( pwrFileData ) );
      PWRLG_SOFTWARE_RESET_SET( 1 );
      PWRLG_TIME_OUTAGE_SET( 0 );   /* Clear the time of the last outage.  */

      /* Get OS Ticks*/
      OS_TICK_Get_CurrentElapsedTicks(&PWR_endTick_);
      DBG_logPrintf( 'R'," Power Down Simulation total Time : %li usec", OS_TICK_Get_Diff_InMicroseconds ( &PWR_startTick_, &PWR_endTick_ ));
      OS_TASK_Sleep( 200 );         /* Wait long enough for the printing to complete  */

      RESET(); /* PWR_SafeReset() not necessary */
   }
#endif

   /* Increase the priority of the power and idle tasks. */
   ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
   ( void )OS_TASK_Set_Priority( pTskName_Idle, 11 );

   pwrFileData.uPowerDownSignature = POWER_DOWN_SIGNATURE; // Write power down signature

#if ( ENABLE_LAST_GASP_TASK == 1 )
   ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* ) &pwrFileData, ( lCnt )sizeof( pwrFileData ) );
   /* TODO: Shouldn't Partition Flush be here instead of in PWRLG_Begin? */
   /********************************************************************/
   /*****               Next function never returns                *****/
   /********************************************************************/
   /* TODO: RA6: DG: Check for Ship and Shop Mode  */
   PWRLG_Begin( pwrFileData.uPowerAnomalyCount ); /* Never returns  */
#else
   VBATREG_SHORT_OUTAGE = 0;  //Ensure we do not see this as a short outage

   sysTimeCombined_t currentTime;
   ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );  /* If this fails, currentTime is 0, considered invalid. */
   pwrFileData.outageTime = currentTime;
   ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* ) &pwrFileData, ( lCnt )sizeof( pwrFileData ) );
   ( void )PAR_partitionFptr.parFlush( NULL ); // Flush all partitions. (only cached partitions will actually flush)

   /* Wait for either power to go away, or the power down signal to return to an "okay" state. If the power signal
      should return to a "okay" state, then reset the processor.  */
   while ( eSUCCESS == PowerFailDebounce() )
   {
      ;
   }
   RESET(); /* PWR_SafeReset() not necessary */
#endif
} /*lint !e715 Arg0 is not used */

/***********************************************************************************************************************

   Function name: PWR_setPowerFailCount

   Purpose: Set Aclara Power Signal Fail Count.

   Arguments: uint16_t value to place in counter

   Returns: void

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void PWR_setPowerFailCount( uint32_t pwrFailCnt )
{
   pwrFileData.uPowerDownCount = ( uint16_t )pwrFailCnt;
   ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* )&pwrFileData, ( lCnt )sizeof( pwrFileData ) );
}

/***********************************************************************************************************************

   Function name: PWR_setOutageTime

   Purpose: Set the recorded outage time.

   Arguments: sysTimeCombined_t value to be written to file

   Returns: void

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void PWR_setOutageTime( sysTimeCombined_t outageTime )
{
   pwrFileData.outageTime = outageTime;
   ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* )&pwrFileData, ( lCnt )sizeof( pwrFileData ) );
}

/***********************************************************************************************************************

   Function name: PWR_setPowerAnomalyCount

   Purpose: Set Aclara Blink Count.

   Arguments: uint16_t value to place in counter

   Returns: uint16_t: Power down counter

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint16_t PWR_setPowerAnomalyCount( uint16_t powerAnomalyCount )
{
   pwrFileData.uPowerAnomalyCount = powerAnomalyCount;
   return pwrFileData.uPowerAnomalyCount;
}

/***********************************************************************************************************************

   Function name: PWR_setSpuriousResetCnt

   Purpose: Sets the spurious reset spurCount

   Arguments: Desired value

   Returns: Nothing

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void PWR_setSpuriousResetCnt( uint32_t spurCount )
{
   /*lint !e578 argument name same as enum is OK   */
   pwrFileData.uSpuriousResets = ( uint16_t )spurCount;
   ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* )&pwrFileData, ( lCnt )sizeof( pwrFileData ) );
}

/***********************************************************************************************************************

   Function name: PWR_getSpuriousResetCnt

   Purpose: Returns the spurious reset count

   Arguments: None

   Returns: uint32_t: Spurious reset count

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint32_t PWR_getSpuriousResetCnt( void )
{
   return pwrFileData.uSpuriousResets;
}

/***********************************************************************************************************************

   Function name: PWR_getOutageTime

   Purpose: Returns the current recorded outage time

   Arguments: None

   Returns: sysTimeCombined_t: outage time from file

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
sysTimeCombined_t PWR_getOutageTime( void )
{
   return pwrFileData.outageTime;
}

/***********************************************************************************************************************

   Function name: PWR_getWatchDogResetCnt

   Purpose: Used to Read the watchdog reset count

   Arguments: None

   Returns: uint32_t: watchdog reset count

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint32_t PWR_getWatchDogResetCnt( void )
{
   return pwrFileData.WatchDogResets;
}

/***********************************************************************************************************************

   Function name: PWR_setWatchDogResetCnt

   Purpose: Used to Set the watchdog reset count

   Arguments: None

   Returns: void

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
void PWR_setWatchDogResetCnt( uint16_t Count )
{
   pwrFileData.WatchDogResets = Count;
}

/***********************************************************************************************************************

   Function name: PWR_TSK_init

   Purpose: Adds registers to the Object list and opens files. This must be called at power before any other function
            in this module is called.

   Arguments: None

   Returns: returnStatus_t - result of file open.

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
returnStatus_t PWR_TSK_init( void )
{

   char                 dt[25];              /* time, in human readable format. */
   TIMESTAMP_t          t;                   /* Holds times to be printed  */
   returnStatus_t       retVal = eFAILURE;
   uint16_t             reason;              /* bit mask to extract reset reason    */
   uint16_t             *counter;            /* Pointer to counter in file          */
   FileStatus_t         fileStatusCfg;       /* Contains the file status */

   PWR_USE_LDO();

   DBG_logPrintf( 'I', "\n" );
   DBG_logPrintHex( 'I', "VBAT RAM : ", (uint8_t *)PWRLG_RFSYS_BASE_PTR, 32);
   DBG_logPrintHex( 'I', "RFSYS RAM: ", (uint8_t *)VBATREG_RFSYS_BASE_PTR, 32);
   DBG_logPrintf( 'I', "outage: %d, Short outage: %d, PWR Qual count (RAM): %d", PWRLG_OUTAGE(), VBATREG_SHORT_OUTAGE, VBATREG_PWR_QUAL_COUNT );
   DBG_logPrintf( 'I', "Restoration timer: %d, Last sleep secs: %d, ms: %d", VBATREG_POWER_RESTORATION_TIMER, PWRLG_SLEEP_SECONDS(), PWRLG_SLEEP_MILLISECONDS() );

   t.seconds = PWRLG_TIME_OUTAGE();
   t.QFrac   = 0;
   ( void )TIME_UTIL_SecondsToDateStr( dt, "", t );
   DBG_logPrintf( 'I', "     Outage time: %s", dt );

   t.seconds = RESTORATION_TIME();
   ( void )TIME_UTIL_SecondsToDateStr( dt, "", t );
   DBG_logPrintf( 'I', "Restoration time: %s", dt );
   //TODO RA6: NRJ: determine if semaphores need to be counting
   if ( OS_MUTEX_Create( &PWR_Mutex ) && OS_MUTEX_Create(&Embedded_Pwr_Mutex) && OS_SEM_Create( &PWR_Sem, 0 ) )
   {
#if ( SIMULATE_POWER_DOWN == 1 )
      //TODO RA6: NRJ: determine if semaphores need to be counting
      (void) OS_SEM_Create( &PWR_SimulatePowerDn_Sem, 0 );     /* Creating the Semaphore */
#endif
      if ( eSUCCESS == FIO_fopen( &fileHndlPowerDownCount, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_PWR,
                                  ( lCnt )sizeof( pwrFileData ), FILE_IS_NOT_CHECKSUMED, 0, &fileStatusCfg ) )
      {
         if ( fileStatusCfg.bFileCreated )
         {
            ( void )memset( ( uint8_t * )&pwrFileData, 0, sizeof( pwrFileData ) );
            retVal = FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* ) &pwrFileData, ( lCnt )sizeof( pwrFileData ) );
         }
         else
         {
            retVal = FIO_fread( &fileHndlPowerDownCount, ( uint8_t* ) &pwrFileData, 0,
                                ( lCnt )sizeof( pwrFileData ) );
            if ( eSUCCESS == retVal )
            {
               DBG_logPrintf( 'I', "PWR Qual count(file): %d, PDSignature: 0x%016llx", pwrFileData.uPowerAnomalyCount, pwrFileData.uPowerDownSignature  );
               if ( POWER_DOWN_SIGNATURE == pwrFileData.uPowerDownSignature )
               {
                  pwrFileData.uPowerDownCount++;
                    /* TODO: RA6: DG: Add below code */
//                  PWRLG_Restoration( pwrFileData.uPowerAnomalyCount ); /* Note: VBATREG_PWR_QUAL_COUNT could be modified in this function */
               }
               else  /* Not a valid power-down signature - no outage. */
               {
                  PWRLG_OUTAGE_SET( 0 );     /* Not an outage  */
                  VBATREG_SHORT_OUTAGE = 0;  /* Not a short outage   */
               }
               counter = ( uint16_t * )&pwrFileData.uSpuriousResets;
               for( reason = RESET_SOURCE_EXTERNAL_RESET_PIN; reason <=  RESET_SOURCE_JTAG; reason <<= 1 )
               {
                  if( ( rstReason_ & reason ) != 0 )
                  {
                     if( *counter < ( ( 1 << ( sizeof( *counter ) * 8 ) ) - 1 ) )
                     {
                        *counter += 1;
                     }
                  }
                  counter++;
               }
               pwrFileData.uPowerDownSignature = 0; // clear the power down signature
               retVal = FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* )&pwrFileData, ( lCnt )sizeof( pwrFileData ) );
               DBG_logPrintf( 'I', "eFN_PWR Power Down Cnt: %u, Spurious Reset Cnt: %u", pwrFileData.uPowerDownCount, pwrFileData.uSpuriousResets );
            }
         }
         /* Check for need to update power quality count.   */
         if ( VBATREG_PWR_QUAL_COUNT > pwrFileData.uPowerAnomalyCount )
         {
            pwrFileData.uPowerAnomalyCount = VBATREG_PWR_QUAL_COUNT;
            retVal |= FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* ) &pwrFileData, ( lCnt )sizeof( pwrFileData ) );  /*lint !e655 !e641 using bitwise operator on retVal OK   */
         }
      }
#if ( MCU_SELECTED == RA6E1 )
      CGC_Stop_Unused_Clocks(); /* TODO: RA6: DG: Move to appropriate location  */
#endif
   }
   return( retVal );
}

/***********************************************************************************************************************

   Function Name: PWR_cfgAlarm

   Purpose: Create a 30 second alarm to wait until time to send the last gasp message.

   Arguments:  None

   Returns: returnStatus_t - eSUCCESS or eFAILURE

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
#if 0
static void cfgAlarm()
{
   tTimeSysPerAlarm alarmSettings;                       /* Configure the periodic alarm for time */

   alarmSettings.bOnInvalidTime = false;                 /* Only alarmed on valid time, not invalid */
   alarmSettings.bOnValidTime = true;                    /* Alarmed on valid time */
   alarmSettings.bSkipTimeChange = false;                /* do NOT notify on time change */
   alarmSettings.bUseLocalTime = false;                  /* Use UTC time */
   alarmSettings.pQueueHandle = NULL;                    /* do NOT use queue */
   alarmSettings.pMQueueHandle = &mQueueHandle_;         /* Uses the message Queue */
   alarmSettings.ucAlarmId = 0;                          /* 0 will create a new ID, anything else will reconfigure. */
   alarmSettings.ulOffset = 0;                           /* No offset, send queue on the boundary */
   alarmSettings.ulPeriod = 10 * 1000;                   /* Sample rate */
   ( void )TIME_SYS_AddPerAlarm( &alarmSettings );
}
#endif
/***********************************************************************************************************************

   Function Name: PWR_lockMutex

   Purpose: Locks mutexes associated with critical events that need to complete before power down.
            This is used for tasks that may have multiple writes to NV memory that must complete before the system
            flushes memory.

            PWR_MUTEX_ONLY -           This mutex lock state is utilized to ensure multiple writes to NV must be
                                       completed before the power down procedure can execute. For example, if two
                                       different pieces of data dependent upon each others values for proper operation.
                                       This mutex state is used to ensure all modifications are completed while losing
                                       power in the middle of the update process.

            EMBEDDED_PWR_MUTEX -       Sometimes a scenario occurs where multiple operation need to be completed at power
                                       down, but the PWR_MUTEX is embedded and utilized by another function/process within
                                       the multiple operation process.  An example of this is during power restoration
                                       where multiple events must be logged.  While processing each event a function
                                       call to log the event utilizes the PWR_Mutex. This mutex state utilizes a separate mutex,
                                       PWR_Restore_Mutex, allowing the ability to bracket the entire operation process
                                       allowing the PWR_MUTEX to be acquired as many times as necessary during the process.

            PWR_AND_EMBEDDED_PWR_MUTEX- This mutex state is utilized before starting the power down procedure/reset
                                       procedure which results in the partitions being flushed.  This mutex state is
                                       required to acquire both the EMBEDDED_PWR_MUTEX and PWR_MUTEX_ONLY ensuring all
                                       NV updates have completed successfully.  This allows the power down process to
                                       complete successfully preventing data corruption in NV.

   Arguments:  enum_PwrMutexState_t - This enumeration dictates proper mutex locking for NV writes critical to be
                                      completed at power down.

   Returns: None

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
void PWR_lockMutex( enum_PwrMutexState_t reqMutexState )
{
   switch(reqMutexState)
   {
      case PWR_AND_EMBEDDED_PWR_MUTEX:
      {
         OS_MUTEX_Lock( &Embedded_Pwr_Mutex );  /*lint !e454   Mutex is locked and not unlocked. */ // Function will not return if it fails
         OS_MUTEX_Lock( &PWR_Mutex );          /*lint !e454   Mutex is locked and not unlocked. */ // Function will not return if it fails
         break;
      }
      case EMBEDDED_PWR_MUTEX:
      {
         OS_MUTEX_Lock( &Embedded_Pwr_Mutex ); // Function will not return if it fails
         break;
      }
      case PWR_MUTEX_ONLY:
      {
         OS_MUTEX_Lock( &PWR_Mutex ); // Function will not return if it fails
         break;
      }
      default:
      {  // Invalid mutex state was requested in function call, force mutex lock fail
         OS_MUTEX_Lock( NULL );
         break;
      }
   }
}

/***********************************************************************************************************************

   Function Name: PWR_unlockMutex

   Purpose: Unlocks mutexes associated with critical events that need to complete before power down.

   Arguments:  enum_PwrMutexState_t

   Returns: None

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
void PWR_unlockMutex( enum_PwrMutexState_t reqMutexState )
{
   switch(reqMutexState)
   {
      case EMBEDDED_PWR_MUTEX:
      {
         OS_MUTEX_Unlock( &Embedded_Pwr_Mutex ); // Function will not return if it fails
         break;
      }
      case PWR_MUTEX_ONLY:
      {
         OS_MUTEX_Unlock( &PWR_Mutex ); // Function will not return if it fails
         break;
      }
      default:
      {  // Invalid mutex state was requested in function call, force mutex unlock fail
         OS_MUTEX_Lock( NULL );
         break;
      }
   }
}

/***********************************************************************************************************************

   Function Name: PWR_waitForStablePower

   Purpose: De-bounces the power down signal and captures the reason for the reset.

   Arguments: None

   Returns: None

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t PWR_waitForStablePower( void )
{
#if ( MCU_SELECTED == NXP_K24 )
   BRN_OUT_TRIS();
#endif
   PowerGoodDebounce();

   return( eSUCCESS );
}

/***********************************************************************************************************************

   Function Name: PWR_powerUp

   Purpose: Capture the reason for the reset.

   Arguments: None

   Returns: None

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t PWR_powerUp( void )
{
   /* Read the Reset Status Register to find the cause of the previous restart */
   rstReason_ = BSP_Get_ResetStatus();
   return( eSUCCESS );
}

/***********************************************************************************************************************

   Function Name: PWR_getResetCause

   Purpose: returns the reset cause.

   Arguments: None

   Returns: reset cause

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
uint16_t PWR_getResetCause( void )
{
   return rstReason_;
}

/***********************************************************************************************************************

   Function Name: PWR_printResetCause

   Purpose: Prints the reset cause.

   Arguments: None

   Returns: None

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t PWR_printResetCause( void )
{
   /* Save the reason we went through the reset vector */
   if ( ( rstReason_ & RESET_SOURCE_POWER_ON_RESET ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Power On Reset" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_EXTERNAL_RESET_PIN ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was External Reset Pin" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_WATCHDOG ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Watchdog" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_LOSS_OF_EXT_CLOCK ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Loss of External Clock" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_LOW_VOLTAGE ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Low Voltage" );
   } /* end if() */
#if ( MCU_SELECTED == NXP_K24 )
   if ( ( rstReason_ & RESET_SOURCE_LOW_LEAKAGE_WAKEUP ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Low Leakage Wakeup" );
   } /* end if() */
#elif ( MCU_SELECTED == RA6E1 )
   if ( ( rstReason_ & RESET_SOURCE_DEEP_SW_STANDBY_CANCEL ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Deep SW Standby Cancel Wakeup" );
   } /* end if() */
#endif
   if ( ( rstReason_ & RESET_SOURCE_STOP_MODE_ACK_ERROR ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Stop Mode Ack Error" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_EZPORT_RESET ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was EzPort Reset" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_MDM_AP ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was MDM-AP" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_SOFTWARE_RESET ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was Software Reset" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_CORE_LOCKUP ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was an ARM Core Lockup" );
   } /* end if() */
   if ( ( rstReason_ & RESET_SOURCE_JTAG ) != 0 )
   {
      DBG_logPrintf( 'I', "Last Reset was JTAG Reset" );
   } /* end if() */

   return( eSUCCESS );
}

/***********************************************************************************************************************

   Function Name: PWR_getPowerFailCount

   Purpose: Get Aclara Power Signal Fail Count.

   Arguments: none

   Returns: uint16_t

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
uint16_t PWR_getPowerFailCount( void )
{
   return( pwrFileData.uPowerDownCount );
}

/***********************************************************************************************************************

   Function Name: PWR_getPowerAnomalyCount

   Purpose: Get Aclara Power Anomaly Count.

   Arguments: uint32_t *pValue

   Returns: returnStatus_t

   Side Effects:

   Reentrant Code: Yes

**********************************************************************************************************************/
returnStatus_t PWR_getPowerAnomalyCount( uint32_t *pValue )
{
   *pValue = pwrFileData.uPowerAnomalyCount;
   return( eSUCCESS );
}
/***********************************************************************************************************************

   Function Name: PWR_resetCounters

   Purpose: Resets all the counters in pwrFileData.

   Arguments: none

   Returns: none

   Side Effects:

   Reentrant Code: Yes

**********************************************************************************************************************/
void PWR_resetCounters( void )
{
   ( void )memset( ( uint8_t * )&pwrFileData, 0, sizeof( pwrFileData ) );
   VBATREG_EnableRegisterAccess();
   VBATREG_PWR_QUAL_COUNT = 0;
   VBATREG_DisableRegisterAccess();
}

/***********************************************************************************************************************

   Function Name: PWR_SafeReset

   Purpose: Do a "clean" Software Reset command.

   Arguments: None

   Returns: None

   Side Effects: NV is flushed and processor is reset

   Reentrant Code: No

 **********************************************************************************************************************/
void PWR_SafeReset( void )
{
   /* Before we can continue the reset process, we need to make sure no tasks are in the middle of updating
      critical information in NV or important system status flags tied to changes in NV such as last gasp flags.
      Lock the power mutex in the PWR_AND_EMBEDDED_PWR_MUTEX state.  This will ensure all NV writes have completed,
     important system flags have been updated, and the device is ready to complete the reset process.  Once the mutex
     is acquired, the power module will not release the mutex in order to prevent other tasks from making updates
     and will be released upon reset completion. */
   PWR_lockMutex( PWR_AND_EMBEDDED_PWR_MUTEX );
   VBATREG_EnableRegisterAccess();
   pwrFileData.uPowerDownSignature  = 0;  /* Clear power down signature */
   pwrFileData.outageTime           = 0;
   VBATREG_PWR_QUAL_COUNT           = 0;  /* Clear the RAM based power quality count.      */

   ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* ) &pwrFileData, ( lCnt )sizeof( pwrFileData ) );
   /* Flush all partitions (only cached actually flushed) and ensures all pending writes are completed */
   ( void )PAR_partitionFptr.parFlush( NULL );
   PWRLG_SOFTWARE_RESET_SET( 1 ); // TODO 2016-02-02 SMG Change to call into PWRLG
   PWRLG_TIME_OUTAGE_SET( 0 );   /* Clear the time of the last outage.  */
   VBATREG_DisableRegisterAccess();

   RESET();
}

/***********************************************************************************************************************

   Function Name: isr_brownOut

   Purpose: Interrupt on brown-out signal

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#if ( MCU_SELECTED == NXP_K24 )
void isr_brownOut( void )
#elif ( MCU_SELECTED == RA6E1 )
void isr_brownOut( external_irq_callback_args_t * p_args )
#endif
{
   BRN_OUT_IRQ_DI();        /* Disable the ISR */
#if ( SIMULATE_POWER_DOWN == 1 )
   /* Adding a semaphore to keep the ISR as short as possible */
#if ( RTOS_SELECTION == MQX_RTOS )
   OS_SEM_Post( &PWR_SimulatePowerDn_Sem ); /* Post the semaphore */
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_SEM_Post_fromISR( &PWR_SimulatePowerDn_Sem ); /* Post the semaphore */
#endif
#endif
#if ( RTOS_SELECTION == MQX_RTOS )
   OS_SEM_Post( &PWR_Sem ); /* Post the semaphore */
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_SEM_Post_fromISR( &PWR_Sem );
#endif
}

/***********************************************************************************************************************

   Function Name: PWR_getpwrFileData

   Purpose: Interrupt on brown-out signal

   Arguments:  None

   Returns: Pointer to pwrFileData

   Side Effects: N/A

   Reentrant Code: Yes

 **********************************************************************************************************************/
pwrFileData_t const *  PWR_getpwrFileData( void )
{
   return &pwrFileData;
}

/***********************************************************************************************************************

   Function name: ShipMode

   Purpose: Performs ship mode check and processing at start up.

   Arguments: None

   Returns: void

   Re-entrant Code: No

   Notes:   According to Heep, the endpoint is required to perform certain actions at power up when in shipmode or
            meter shopmode.  The definitions in the Heep documentation for each mode at power up are currently the same.
            As a result, this function will handle processing both modes at power up.  If either mode is set, the
            processes described in the Heep will be performed.

 **********************************************************************************************************************/
static void ShipMode( void )
{
   uint8_t meterShopModeValue = MODECFG_get_decommission_mode();
   uint8_t shipModeValue = MODECFG_get_ship_mode();

   if( ( meterShopModeValue > 0 )  || ( shipModeValue > 0 ) )
   {
      DBG_logPrintf('I', "PWR_task: MeterShopMode/ShipMode Set, Performing Initialization.");

      /* Wait for stack to init */
      SM_WaitForStackInitialized();
      /* Force a new registration. */
      APP_MSG_ReEnableRegistration();

      if( shipModeValue > 0 ) /* if shipmode > 0, reset powerAnamolyCount #128 */
      {
         /* Reset blink counter when ship mode detected. */
         ( void )PWR_setPowerAnomalyCount( 0 );

         /* invalidate SYS and RTC Time */
         (void)TIME_UTIL_SetTimeFromSeconds(0, 0);
      }

      /* if shipmode or decommission mode is equal to 1, clear network statistics, events, and metering values */
      if( shipModeValue == 1  || meterShopModeValue == 1 )
      {

         /* Clear the installation date and time. */
         TIME_SYS_SetInstallationDateTime( 0 );
         /* Clear network statistics. */
         (void)NWK_ResetRequest( eNWK_RESET_STATISTICS, NULL );
         (void)MAC_ResetRequest( eMAC_RESET_STATISTICS, NULL );
         (void)PHY_ResetRequest( ePHY_RESET_STATISTICS, NULL );

#if (USE_MTLS == 1)
         ( void )MTLS_Reset();
#endif
#if (USE_DTLS == 1)
         DTLS_Stats( ( bool ) true); /*resets DTLS Stats*/
#endif
         PWR_resetCounters(); /*reset all PWR counters*/
         rstReason_ = 0;      /*clear last reset reason*/
#if 0 /* mkv 7/8/20 - These stats are cleared during BM_init. Clearing them at this time causes buffer allocation errors and eventually a fault
         condition.  */
         BM_resetStats();
#endif


#if ( ENABLE_HMC_TASKS == 1 )
#if ( DEMAND_IN_METER == 0 )
         DEMAND_clearDemand();
#endif
#if ( LP_IN_METER == 0 )
         ID_clearAllIdChannels();
#endif
         HD_clearHistoricalData();
#endif   /* end of ENABLE_HMC_TASKS  == 1 */
         EVL_clearEventLog();

#if ( ENABLE_METER_EVENT_LOGGING != 0 )
         ( void )ALRM_clearAllAlarms ();
#endif
         PWRLG_RestLastGaspFlags(); // coming out of shipmode or decommission mode, clear the last gasp flags
      }

      /* Clear the Meter Shop Mode if necessary */
      if( meterShopModeValue != 0 )
      {
         (void)MODECFG_set_decommission_mode( (uint8_t)0 );
      }

      /* Clear the Ship Mode if necessary */
      if ( shipModeValue != 0 )
      {
         (void)MODECFG_set_ship_mode( (uint8_t)0 );
      }
   }
}

/***********************************************************************************************************************

   Function Name: PowerGoodDebounce

   Purpose: De-bounce the brown-out signal until indicates power is good

   Arguments:  None

   Returns: None

   Side Effects: This will block until the brown-out signal until indicates power is good. Clears WDT which is may not
                  always be enabled.

   Reentrant Code: No

 **********************************************************************************************************************/
static void PowerGoodDebounce( void )
{
   uint16_t debounceCount = 0;

   // Make sure PF_METER deasserted for 1ms.
   while ( debounceCount < DEBOUNCE_CNT_RST_VAL )
   {
#if ( RTOS_SELECTION == MQX_RTOS )
      delay_10us( DEBOUNCE_DELAY_VAL );  /* TODO: RA6E1: Create delay_10us() for FreeRTOS */
#elif ( RTOS_SELECTION == FREE_RTOS )
      R_BSP_SoftwareDelay( DEBOUNCE_DELAY_VAL * 10 , BSP_DELAY_UNITS_MICROSECONDS );
#endif
      CLRWDT();
      if ( BRN_OUT() )
      {
         debounceCount = 0;
      }
      else
      {
         ++debounceCount;
      }
   }

   return;
}

/***********************************************************************************************************************

   Function Name: PowerFailDebounce

   Purpose: Debounce the brown-out signal or return immediately if brown-out signal indicates power is restored

   Arguments:  None

   Returns: eSUCCESS if brown-out signal indicates power has failed, otherwise eFAILURE

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
static returnStatus_t PowerFailDebounce( void )
{
   returnStatus_t brownOut       = eSUCCESS;
   uint16_t       debounceCount  = 0;

   // Make sure BRN_OUT asserted for 1ms.
   while ( ( eSUCCESS == brownOut ) && ( debounceCount < DEBOUNCE_CNT_RST_VAL ) )
   {
#if ( RTOS_SELECTION == MQX_RTOS )
      delay_10us( DEBOUNCE_DELAY_VAL );  /* TODO: RA6E1: Create delay_10us() for FreeRTOS */
#elif ( RTOS_SELECTION == FREE_RTOS )
      R_BSP_SoftwareDelay( DEBOUNCE_DELAY_VAL * 10 , BSP_DELAY_UNITS_MICROSECONDS );
#endif
      if ( BRN_OUT() )
      {
         ++debounceCount;
      }
      else
      {
         brownOut = eFAILURE;
      }
   }

   return ( brownOut );
}

#if ( EP == 1 )
/***********************************************************************************************************************

   Function Name: PWR_OR_PM_Handler

   Purpose: Get/Set parameters related to PWR over the air

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t PWR_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case watchdogResetCount :
         {
            *(uint16_t *)value = (uint16_t)PWR_getWatchDogResetCnt();
            retVal = eSUCCESS;
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(pwrFileData.WatchDogResets);
               attr->rValTypecast = (uint8_t)uintValue;
            }
            break;
         }

         case spuriousResetCount :
         {
            if ( sizeof(pwrFileData.uSpuriousResets) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = (uint16_t)PWR_getSpuriousResetCnt();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(pwrFileData.uSpuriousResets);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case PowerFailCount :
         {
            if ( sizeof(pwrFileData.uPowerDownCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint16_t *)value = PWR_getPowerFailCount();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(pwrFileData.uPowerDownCount);
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
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case watchdogResetCount :
         {
            PWR_setWatchDogResetCnt(*(uint16_t *)value);
            retVal = eSUCCESS;
            break;
         }

         case spuriousResetCount :
         {
            PWR_setSpuriousResetCnt(*(uint32_t *)value);
            retVal = eSUCCESS;
            break;
         }
         case PowerFailCount  :
         {
            PWR_setPowerFailCount(*(uint32_t *)value);
            retVal = eSUCCESS;
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}
#endif  // #if ( EP == 1 )