/******************************************************************************

   Filename: STRT_Startup.c

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
#include <stdbool.h>
#if 0
#include <mqx.h>
#include <fio.h>
#include "EVL_event_log.h"

#include "user_config.h"
#if ( MQX_USE_LOGS == 1 )
#define PRINT_LOGS 1
#warning "Don't release with MQX_USE_LOGS set in user_config.h"   /*lint !e10 !e16  */
#include <klog.h>
#endif

#if ENABLE_ALRM_TASKS
#include "ALRM_Handler.h"
#include "Temperature.h"
#endif

#include "DBG_SerialDebug.h"
#include "IDL_IdleProcess.h"

#include "timer_util.h"
#include "time_sync.h"
#include "time_sys.h"
#include "time_util.h"

#if ENABLE_HMC_TASKS
#include "hmc_start.h"
#include "hmc_eng.h"
#endif

#if ENABLE_SRFN_ILC_TASKS
   #include "ilc_dru_driver.h"
   #include "ilc_srfn_reg.h"
   #include "ilc_time_sync_handler.h"
#endif

#if ENABLE_SRFN_DA_TASKS
#include "b2b.h"
#include "da_srfn_reg.h"
#endif

#include "hmc_app.h"
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
#include "hmc_prg_mtr.h"
#endif
#include "STRT_Startup.h"
#include "sys_busy.h"
#include "virgin_device.h"
#include "dvr_extflash.h"
#include "vbat_reg.h"
#include "ed_config.h"

#if ENABLE_PWR_TASKS
#include "pwr_task.h"
#include "pwr_config.h"
#include "pwr_restore.h"
#endif

#include "MFG_Port.h"
#include "MIMT_info.h"
#include "demand.h"
#include "mode_config.h"
#if ENABLE_ID_TASKS
#include "ID_intervalTask.h"
#endif

#if (SIGNAL_NW_STATUS == 1)
#include "nw_connected.h"
#endif

#if ( USE_DTLS == 1 )
#include "DTLS/DTLS.h"
#endif
#if ( USE_MTLS == 1 )
#include "MTLS/mtls.h"
#endif

#if ENABLE_DFW_TASKS
#include "dfw_app.h"
#include "dfwtd_config.h"
#endif
#include "SM_Protocol.h"
#include "SM.h"
#if ( PHASE_DETECTION == 1 )
#include "PhaseDetect.h"
#endif
#include "PHY.h"
#include "MAC_Protocol.h"
#include "MAC.h"
#include "STACK_Protocol.h"
#include "STACK.h"
#include "smtd_config.h"
// Add this line to enable application layer loopback (rx'd packets are sent back down stack)
//#include "COMM.h"
#include "historyd.h"
#include "OR_MR_Handler.h"
#include "APP_MSG_Handler.h"
#include "tunnel_msg_handler.h"
#include "time_DST.h"
#include "SELF_test.h"
#include "ecc108_mqx.h"
#include "pwr_last_gasp.h"
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
#include "hmc_display.h"
#endif
#include "version.h"
#include "FTM.h" // Used for radio interrupt (FTM1_CH0), radio TCXO (FTM1_CH1) and ZCD_METER interrupt (FTM3_CH1)
#endif
/* TODO: DG: Remove Duplicate Includes */
#include "STRT_Startup.h"
#include "version.h"
#include "DBG_SerialDebug.h"
/* #DEFINE DEFINITIONS */
#define PRINT_CPU_STATS_IN_SEC 5 /* Print the Cpu statistics every x seconds */
#define PRINT_STACK_USAGE_AND_TASK_SUMMARY 28 /* Print after x seconds of 100% CPU load */

#define CPU_LOAD_SMART_PRINT_THRESHOLD 35

#if ENABLE_TEST_MESSAGEQ
#define MSGQ_MESSAGE_SIZE 8
#endif /* ENABLE_TEST_MESSAGEQ */

/* MACRO DEFINITIONS */

#define INIT(func, flags) func, #func, flags

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
static bool initSuccess_ = true; //Default, system init successful

static STRT_CPU_LOAD_PRINT_e CpuLoadPrint = eSTRT_CPU_LOAD_PRINT_SMART;


/* Power Up Table - Define all modules that require initialization below. */
const STRT_FunctionList_t startUpTbl[] =
{
   INIT( VER_Init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( DBG_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),        // We need this to print errors ASAP
#if 0
   INIT( WDOG_Init, STRT_FLAG_NONE ),                                               /* Watchdog needs to be kicked while waiting for stable power. */
#if ENABLE_PWR_TASKS
   INIT( PWR_waitForStablePower, STRT_FLAG_NONE ),
#endif
   /* Past this point, power is good.  Continue to power the unit up! */

   INIT( UART_init, STRT_FLAG_LAST_GASP ),                                          // We need this ASAP to print error messages to debug port

   INIT( CRC_initialize, STRT_FLAG_LAST_GASP ),

   INIT( FIO_finit, STRT_FLAG_LAST_GASP ),                                          // This must be after CRC_initialize because it uses CRC.
   INIT( BM_init, STRT_FLAG_LAST_GASP ),                                            // We need this to have buffers for DBG
   INIT( VDEV_init, STRT_FLAG_NONE ),                                               // Needed to be done ASAP because it might need to virgin the flash
   INIT( ADC_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),        /* Needed for proper aclara_randu behavior which is needed by
                                                                                       other init modules so it needs to be called ASAP */
   INIT( MODECFG_init, STRT_FLAG_LAST_GASP ),                                       /* Must be before PWR_TSK_init so the mode is available. Note,
                                                                                       quiet and rftest mode flags can't be checked before this init
                                                                                       has been run */

   INIT( VBATREG_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),    // Needed early to check validity of RTC.
   INIT( TIME_SYS_Init, (STRT_FLAG_LAST_GASP|STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),   // NOTE: This needs to be called as soon as possible in order to create the time mutexes early on
                                                                                    //       because the error logging (ERR_printf, DBG_logPrintf, etc) uses the clock
                                                                                    //       to time stamp the message and, in the process, uses the time mutex.
                                                                                    // NOTE2: This needs to be after FIO_init since it uses the file system.
   INIT( TMR_HandlerInit, (STRT_FLAG_LAST_GASP|STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
   INIT( DBG_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),        // We need this to print errors ASAP
   INIT( DST_Init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),                        // This should come before TIME_SYS_SetTimeFromRTC
   INIT( TIME_SYS_SetTimeFromRTC, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( TIME_SYNC_Init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( PWR_powerUp, STRT_FLAG_NONE),                                              // Read reset reason

   /***************************************************************/
   /* Note: Do not add anything above this unless there is a need */
   /***************************************************************/
   INIT( SELF_init, STRT_FLAG_NONE ),
#if ENABLE_PWR_TASKS
   INIT( PWRCFG_init, (STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),                         /* Must be before PWR_TSK_init so restoration delay is available*/
   INIT( PWR_TSK_init, (STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
   INIT( PWROR_init, (STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
#endif
   INIT( SYSBUSY_init, STRT_FLAG_NONE ),                                            // Needed for DFW, HMC and interval data
#if ENABLE_DFW_TASKS
   INIT( DFWA_init, STRT_FLAG_NONE ),
   INIT( DFWTDCFG_init, STRT_FLAG_NONE ),
#endif
#if ENABLE_ALRM_TASKS
   INIT( ALRM_init, (STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
   INIT( TEMPERATURE_init, STRT_FLAG_NONE ),
#endif
#if ENABLE_MFG_TASKS
   INIT( MFGP_init, (STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
   INIT( MFGP_cmdInit, (STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
#endif
   INIT( MIMTINFO_init, STRT_FLAG_NONE ),
#if ENABLE_SRFN_ILC_TASKS
   INIT( ILC_DRU_DRIVER_Init, STRT_FLAG_NONE ),
   INIT( ILC_SRFN_REG_Init, STRT_FLAG_NONE ),
   INIT( ILC_TIME_SYNC_Init, STRT_FLAG_NONE ),
#endif
#if ENABLE_SRFN_DA_TASKS
   INIT( B2BInit, STRT_FLAG_NONE ),
   INIT( DA_SRFN_REG_Init, STRT_FLAG_NONE ),
#endif
#if ENABLE_HMC_TASKS
#if END_DEVICE_PROGRAMMING_CONFIG == 1
   INIT( HMC_PRG_MTR_init, STRT_FLAG_NONE ),                                        /* RCZ Added - Necessary for meter access (R/W/Procedures)  */
#endif
   INIT( HMC_STRT_init, STRT_FLAG_NONE ),
   INIT( HMC_APP_RTOS_Init, STRT_FLAG_NONE ),
   INIT( HMC_ENG_init, STRT_FLAG_NONE ),
#endif
   INIT( PAR_initRtos, STRT_FLAG_NONE ),
#if ( ENABLE_HMC_TASKS == 1 )
   INIT( DEMAND_init, STRT_FLAG_NONE ),
   INIT( ID_init, STRT_FLAG_NONE ),
   //INIT(LPCFG_init, STRT_FLAG_NONE),
   //INIT(DSCFG_init, STRT_FLAG_NONE),
#endif   /* end of ENABLE_HMC_TASKS  == 1 */
   INIT( PHY_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),                        // PHY must be initialized before MAC
   INIT( MAC_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( NWK_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( SM_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( SMTDCFG_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( FTM3_Init, STRT_FLAG_LAST_GASP),
#if ( PHASE_DETECTION == 1 )                                                // Used for ZCD_METER interrupt (FTM3_CH1). Must be before PD_init.
   INIT( PD_init, STRT_FLAG_NONE ),
#endif
#if (SIGNAL_NW_STATUS == 1)
   INIT(NwConnectedInit, STRT_FLAG_NONE),                                           // Must be after SM as it registers for indications
#endif
#if ( USE_DTLS == 1 )
   INIT( DTLS_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
#endif
#if ( USE_MTLS == 1 )
   INIT( MTLS_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
#endif
   INIT( APP_MSG_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
#if (USE_IPTUNNEL == 1)
   INIT(TUNNEL_MSG_init, STRT_FLAG_NONE),
#endif
#if ( ENABLE_HMC_TASKS == 1 )
   INIT( HD_init, STRT_FLAG_NONE ),
   INIT( OR_MR_init, STRT_FLAG_NONE ),
#endif   /* end of ENABLE_HMC_TASKS  == 1 */
   INIT( SEC_init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( EDCFG_init, STRT_FLAG_NONE ),
   INIT( EVL_Initalize, STRT_FLAG_RFTEST ),
   INIT( VER_Init, (STRT_FLAG_LAST_GASP|STRT_FLAG_RFTEST) ),
   INIT( FIO_init, (STRT_FLAG_QUIET|STRT_FLAG_RFTEST) ),
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
   INIT( HMC_DISP_Init, STRT_FLAG_NONE ),
#endif
   INIT( LED_init, STRT_FLAG_NONE ),                              // LED init must happen after version init as it requires HW rev letter
#if ( HMC_I210_PLUS_C == 1 )                                      // TODO: Verify the changes GIPO Pin configs for other EPs, and then make necessary changes
   INIT( IO_init, STRT_FLAG_NONE ),                               // Configuring the GPIOs
#endif
#if ENABLE_PWR_TASKS
   INIT( PWR_printResetCause, STRT_FLAG_NONE ),                   //Prints the reset cause
#endif
   INIT( FTM1_Init, STRT_FLAG_LAST_GASP)                           // Used for radio interrupt (FTM1_CH0) and radio TCXO (FTM1_CH1).
#endif /* #if 0*/
};

const uint8_t uStartUpTblCnt = ARRAY_IDX_CNT( startUpTbl );

#if ENABLE_TEST_MESSAGEQ
static OS_MSGQ_Obj TestMsgq_MSGQ;
#endif /* ENABLE_TEST_MESSAGEQ */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: STRT_EnableCpuLoadPrint

  Purpose: This function is used to enable CPU Load printing

  Arguments:

  Returns:

  Notes: This can be used to save CPU cycles if necessary

*******************************************************************************/
void STRT_CpuLoadPrint ( STRT_CPU_LOAD_PRINT_e mode )
{
   if (  ( mode == eSTRT_CPU_LOAD_PRINT_ON ) ||
         ( mode == eSTRT_CPU_LOAD_PRINT_SMART ) ||
         ( mode == eSTRT_CPU_LOAD_PRINT_OFF ) )
   {
      CpuLoadPrint = mode;
   }
}

/*******************************************************************************

   Function name: STRT_StartupTask

   Purpose: This task will start the application tasks in a controlled fashion.
            Initialization functions are called first to ensure each module is
            setup correctly before execution begins.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns:

   Notes: Processing actually starts in an MQX function main() located inside MQX
         file mqx_main.c (mqx\source\bsp\twrk60...)
         MQX uses the MQX_template_list inside a structure MQX_init_struct to
         start the appropriate tasks.
         This task is in that list and is set to automatically start when MQX
         starts (by setting MQX_AUTO_START_TASK).

*******************************************************************************/
/*lint -e{715} Arg0 not used; required by API */
#if 0
void STRT_StartupTask ( uint32_t Arg0 )
#else
//void STRT_StartupTask ( void* pvParameters ) taskParameter
void STRT_StartupTask ( taskParameter )
#endif
{
   //FSP_PARAMETER_NOT_USED(taskParameter);
//   OS_TICK_Struct       TickTime;
   TickType_t           TickTime;
   STRT_FunctionList_t  *pFunct;
   uint32_t             CurrentIdleCount;
   uint32_t             PrevIdleCount;
   uint32_t             TempIdleCount;
   uint32_t             printStackAndTask = 0;
   uint32_t             CpuLoad[PRINT_CPU_STATS_IN_SEC];
   uint8_t              CpuIdx = 0;
   uint8_t              startUpIdx;
   uint8_t              quiet = 0;
   uint8_t              rfTest = 0;
#if 0
   // Enable to use DWT module.
   // This is needed for DWT_CYCCNT to work properly when the debugger (I-jet) is not plugged in.
   // When the debugger is plugged in, the following registers are programmed by the IAR environment.
   DEMCR = DEMCR | 0x01000000;

   // Enable cycle counter
   DWT_CTRL = DWT_CTRL | 1 ;
#endif
#if ( MQX_USE_LOGS == 1 )
   /* Create kernel log */
   if ( _klog_create( 192, 1 ) != MQX_OK )
   {
      (void)printf( "Main task - _klog_create failed!" );
   }
   else
   {
      /* Enable kernel log */
      _klog_control( KLOG_ENABLED | KLOG_CONTEXT_ENABLED, TRUE );
   }
#endif

   /* Initialize all of the modules here: */
   for ( startUpIdx = 0, pFunct = ( STRT_FunctionList_t* )&startUpTbl[0]; startUpIdx < uStartUpTblCnt; startUpIdx++, pFunct++ )
   {
      if (  ( ( quiet == 0 )  || ( ( pFunct->uFlags & STRT_FLAG_QUIET )  != 0 ) ) &&
            ( ( rfTest == 0 ) || ( ( pFunct->uFlags & STRT_FLAG_RFTEST ) != 0 ) ) )
      {
         returnStatus_t response;
#if 0
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#pragma calls=\
          WDOG_Init, \
          PWR_waitForStablePower, \
          UART_init, \
          CRC_initialize, \
          FIO_finit, \
          BM_init, \
          VDEV_init, \
          MODECFG_init, \
          VBATREG_init, \
          TIME_SYS_Init, \
          TMR_HandlerInit, \
          DBG_init, \
          ADC_init, \
          DST_Init, \
          TIME_SYS_SetTimeFromRTC, \
          TIME_SYNC_Init, \
          SELF_init, \
          PWRCFG_init, \
          PWR_TSK_init, \
          PWROR_init, \
          SYSBUSY_init, \
          DFWA_init, \
          DFWTDCFG_init, \
          ALRM_init, \
          TEMPERATURE_init, \
          LED_init, \
          IO_init,\
          MFGP_init, \
          MFGP_cmdInit, \
          MIMTINFO_init, \
          HMC_STRT_init, \
          HMC_APP_RTOS_Init, \
          HMC_ENG_init, \
          PAR_initRtos, \
          DEMAND_init, \
          ID_init, \
          PHY_init, \
          MAC_init, \
          NWK_init, \
          SM_init, \
          SMTDCFG_init, \
          DTLS_init, \
          MTLS_init, \
          APP_MSG_init, \
          TUNNEL_MSG_init, \
          HD_init, \
          OR_MR_init, \
          SEC_init, \
          EDCFG_init, \
          EVL_Initalize, \
          VER_Init, \
          FIO_init, \
          PWR_printResetCause
#else
#pragma calls=\
          WDOG_Init, \
          PWR_waitForStablePower, \
          UART_init, \
          CRC_initialize, \
          FIO_finit, \
          BM_init, \
          VDEV_init, \
          MODECFG_init, \
          VBATREG_init, \
          TIME_SYS_Init, \
          TMR_HandlerInit, \
          DBG_init, \
          ADC_init, \
          DST_Init, \
          TIME_SYS_SetTimeFromRTC, \
          TIME_SYNC_Init, \
          SELF_init, \
          PWRCFG_init, \
          PWR_TSK_init, \
          PWROR_init, \
          SYSBUSY_init, \
          DFWA_init, \
          DFWTDCFG_init, \
          LED_init, \
          IO_init, \
          MFGP_init, \
          MFGP_cmdInit, \
          MIMTINFO_init, \
          HMC_APP_RTOS_Init, \
          PAR_initRtos, \
          DEMAND_init, \
          PHY_init, \
          MAC_init, \
          NWK_init, \
          SM_init, \
          SMTDCFG_init, \
          DTLS_init, \
          MTLS_init, \
          APP_MSG_init, \
          TUNNEL_MSG_init, \
          HD_init, \
          OR_MR_init, \
          SEC_init, \
          EDCFG_init, \
          EVL_Initalize, \
          VER_Init, \
          FIO_init, \
          PWR_printResetCause
#endif
#endif // #if 0
         response = pFunct->pFxnStrt();
#if 0
         if ( pFunct->pFxnStrt == MODECFG_init )
         {
            quiet = MODECFG_get_quiet_mode();
            rfTest = MODECFG_get_rfTest_mode();
         }
#endif
         if ( eSUCCESS != response )
         {
            /* This condition should only show up in development.  This infinite loop should help someone figure out
               that there is an issue initializing a task. */
            ( void )printf( "\n\t\t#####################\n" );
            ( void )printf( "\nStartup Failure - Call to %s failed, Code: %u\n", pFunct->name, ( uint16_t )response );
            ( void )printf( "\n\t\t#####################\n" );
            initSuccess_ = false;
         }
      }
   }
#if (TM_MUTEX == 1)
   OS_MUTEX_Test();
#endif
   OS_TASK_Create_All(initSuccess_);   /* Start all of the tasks that were not auto started */
#if 0
   if (!initSuccess_)
   {
      //LED_setRedLedStatus(MANUAL_RED);
#if ( TEST_TDMA == 0 )
      LED_enableManualControl();
      LED_on(RED_LED);
#endif
   }

   // Reset all CPU stats
   // This MUST be done after the tasks are started
   (void)OS_TASK_UpdateCpuLoad();

   if ( quiet == 0 )
   {
      ( void )SM_StartRequest( eSM_START_STANDARD, NULL );  /* Start stack manager  */
   }

#ifdef NDEBUG /* If defined, this is release code (not debug) */
   DBG_logPrintf( 'I', "Running Release code" );
#else /* NDEBUG */
   DBG_logPrintf( 'I', "Running Debug code" );
#endif /* NDEBUG */

   if ( VDEV_wasVirgin() )
   {
      DBG_logPrintf( 'I', "Device was Virgin!" );
   }

   /* This should be placed directly before the for(;;) loop of the task to ensure
      we don't surpass the TickTime delay specified below
      Note:  We do have the CPU Load function below this, and that is acceptable
             to ensure we get an accurate CPU load value */
   OS_TICK_Get_CurrentElapsedTicks ( &TickTime );

   CurrentIdleCount = IDL_Get_IdleCounter();
   PrevIdleCount = CurrentIdleCount;
#endif  // #if 0
   for ( ;; )
   {
      vTaskSuspend(NULL); // TODO: DG: Remove
#if 0
      OS_TICK_Sleep ( &TickTime, ONE_SEC );

      CurrentIdleCount = IDL_Get_IdleCounter();
      TempIdleCount = CurrentIdleCount - PrevIdleCount;
      PrevIdleCount = CurrentIdleCount;

      CpuLoad[CpuIdx] = OS_TASK_UpdateCpuLoad();

      if ( TempIdleCount > 0 )
      {
         /* Idle Task has run within the last second, so go ahead and refresh/kick the Watchdog */
         CLRWDT();
         printStackAndTask = 0;
      }
      else if ( ++printStackAndTask >= PRINT_STACK_USAGE_AND_TASK_SUMMARY )
      {
         // If we are close to a watchdog reset, print some useful stats.
#if ( MQX_USE_LOGS == 1 )
         /* Read data from kernel log */
#if ( PRINT_LOGS == 1 )
         (void)printf( "\nklog:\n" );
         while ( _klog_display() )
         {
            CLRWDT();
         }
#endif
#else
         OS_TASK_Summary((bool)false);
#endif
         printStackAndTask = 0;
      }

      if ( ++CpuIdx >= PRINT_CPU_STATS_IN_SEC )
      {
         if ( CpuLoadPrint == eSTRT_CPU_LOAD_PRINT_ON )
         {
            DBG_logPrintf('C', "CPU= %u, %u, %u, %u, %u",
                          CpuLoad[0],
                          CpuLoad[1],
                          CpuLoad[2],
                          CpuLoad[3],
                          CpuLoad[4]);
         }
         else if (CpuLoadPrint == eSTRT_CPU_LOAD_PRINT_SMART)
         {
            if ((CpuLoad[0] > CPU_LOAD_SMART_PRINT_THRESHOLD) ||
                (CpuLoad[1] > CPU_LOAD_SMART_PRINT_THRESHOLD) ||
                (CpuLoad[2] > CPU_LOAD_SMART_PRINT_THRESHOLD) ||
                (CpuLoad[3] > CPU_LOAD_SMART_PRINT_THRESHOLD) ||
                (CpuLoad[4] > CPU_LOAD_SMART_PRINT_THRESHOLD))
            {
               DBG_logPrintf('C', "CPU= %u, %u, %u, %u, %u",
                             CpuLoad[0],
                             CpuLoad[1],
                             CpuLoad[2],
                             CpuLoad[3],
                             CpuLoad[4]);
            }
         }
         CpuIdx = 0;
      } /* end if() */
#endif // #if 0
   } /* end for() */
} /* end STRT_StartupTask () */
