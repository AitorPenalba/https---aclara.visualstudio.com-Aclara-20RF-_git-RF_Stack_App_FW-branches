/***********************************************************************************************************************

   Filename: pwr_last_gasp

   Global Designator: PWRLG_

   Contents: Last gasp message build and transmission.

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

   @author David McGraw

 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <string.h>
#include <stdarg.h>
#if ( MCU_SELECTED == NXP_K24 )
#include <bsp.h>
#endif
#include "compiler_types.h"
#include "App_Msg_Handler.h"
#include "DBG_SerialDebug.h"
#include "STRT_Startup.h"
#include "pack.h"
#include "file_io.h"
#include "mode_config.h"
#include "time_sys.h"
#include "MAC.h"
#include "STACK.h"
#include "radio_hal.h"
#include "vbat_reg.h"
#include "pwr_config.h"
#include "pwr_task.h"
#include "SM_Protocol.h"
#include "SM.h"
#if ( MCU_SELECTED == RA6E1 )
//#include "lpm_app.h" // TODO: RA6E1: DG: Add later
#include "AGT.h"
#include "bsp_pin_cfg.h"
#endif

#define PWRLG_GLOBALS
#include "pwr_last_gasp.h"
#undef PWRLG_GLOBALS

#if ( ( USE_DTLS == 1 ) && ( DTLS_FIELD_TRIAL == 0 ) )
#include "dtls.h"
#endif
#include "dvr_sharedMem.h"
#include "version.h"
#if ( LAST_GASP_SIMULATION == 1 )
#include "EVL_event_log.h"
#endif


/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

extern const OS_TASK_Template_t OS_template_list_last_gasp[];

extern const STRT_FunctionList_t startUpTbl[];
extern const uint8_t uStartUpTblCnt;

/* ****************************************************************************************************************** */
/* CONSTANTS */

#define PWRLG_PRINT_ENABLE       1  /* Enable to use LG_PRNT_INFO() */

#define ENABLE_LED               0  /* Set to non-zero to used LEDs for debugging purposes   */

#if ( DEBUG_PWRLG != 0 )
#define LG_TEST_PULLUP           0    /* If set, enables the PF signal pull up. Note: Enable or disable based on your last gasp testing set-up/connection. */
#warning "Don't release with DEBUG_PWRLG set"   /*lint !e10 !e16  */
#endif

#if ( PWRLG_PRINT_ENABLE != 0 )
#warning "Don't release with PWRLG_PRINT_ENABLE set"   /*lint !e10 !e16  */
#endif

#if ( ENABLE_LED != 0 )
#warning "LEDs are not available on REV C HW"   /*lint !e10 !e16  */
#warning "Don't release with ENABLE_LED set"   /*lint !e10 !e16  */
#endif

#if ( DEBUG_PWRLG == 0 )
#define PWRLG_FIRST_MSG_WINDOW   ((uint32_t)30000) /* First message window after initial outageDelay */
#else
#define PWRLG_FIRST_MSG_WINDOW   ((uint32_t)5000)  /* First message window after initial outageDelay */
#endif
#define PWR_STABLE_MILLISECONDS   ((uint16_t)100)  /* Power fail signal change debounce time in milliseconds */
#if ( PWRLG_PRINT_ENABLE != 0 )
#if ( LG_WORST_CASE_TEST == 1 )
#define OS_SLEEP_PRINT_DELAY_MS   ((uint32_t)150)  /* Print sleep delay */
#endif
#endif
#define OS_SLEEP_PRINT_DELAY_MS   ((uint32_t)100)  /* Print sleep delay */

//These MAY become configurable
static const float fCapacitanceRevB          = ( float )10.0;     /* Rev B HW's Super cap capacitance value in Farads */
static const float fCapacitanceRevC          = ( float )5.0;      /* Rev C HW's Super cap capacitance value in Farads */
static const float fEnergy30SecondSleep      = ( float )0.08;     /* Energy used to sleep 30 seconds */
static const float fEnergyCollisionDetection = ( float )0.0;      /* Energy used to detect a collision */
static const float fEnergyTransmit           = ( float )0.65;     /* Energy used to transmit the message */
static const float fMinimumRunVoltageRevB    = ( float )1.7;      /* Minimum supercap voltage needed to run micro */
static const float fMinimumRunVoltageRevC    = ( float )1.0;      /* Minimum supercap voltage needed to run micro ( Minimum transmit voltage ) */
#if ( LG_WORST_CASE_TEST == 0 )
static const float fMinimumCsmaPValue        = ( float )0.3;      /* Minimum P-persistence value */
#else
static const float   fMinimumCsmaPValue      = ( float )0.0;      /* Use to ensure all but the last attempt to always fail for the LG test */
static const float   fMaxCsmaPValue          = ( float )1.0;      /* Use to ensure the last attempt will always succeed for the LG test */
static const int16_t phyCCAThreshold         = PHY_CCA_THRESHOLD_MIN;
#endif
static const float fMinimumStartVoltage      = ( float )1.8;      /* Minimum supercap voltage needed to start micro */

static const uint32_t uMinSleepMilliseconds  = ( uint32_t )10;    /* Minimum time between message transmissions */
static const uint32_t uTotalMilliseconds     = ( uint32_t )20 * 60 * 1000; /* 20 minutes in milliseconds */

static PWRLG_SysRegisterFilePtr      const pSysMem  = PWRLG_RFSYS_BASE_PTR;
static VBATREG_VbatRegisterFilePtr   const pVbatMem = VBATREG_RFSYS_BASE_PTR;

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
/*lint -esym(749,PWRLG_LPTMR_Mode::LPTMR_MODE_START)  */
typedef enum
{
   LPTMR_MODE_PROGRAM_ONLY = 0,
   LPTMR_MODE_START,
   LPTMR_MODE_WAIT,
   LPTMR_MODE_ENBLE_INT
} PWRLG_LPTMR_Mode;

typedef enum
{
   LPTMR_SECONDS = 0,
   LPTMR_MILLISECONDS,
   LPTMR_SLEEP_FOREVER
} PWRLG_LPTMR_Units;

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static OS_SEM_Obj       TxDoneSem;            /* Used to signal message transmit completed. */
static volatile uint8_t TxSuccessful;
#if ( DEBUG_PWRLG != 0 )
#define uLLWU_F3     ( VBATREG_RFSYS_BASE_PTR->uLLWU_F3 )
#define uLLWU_FILT1  ( VBATREG_RFSYS_BASE_PTR->uLLWU_FILT1 )
#endif
static uint8_t hwRevLetter_;                  /* Used to store the HW revision letter */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

//extern void task_exception_handler( _mqx_uint para, void * stack_ptr );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
static uint32_t         CalculateSleepWindow( uint8_t uMessagesRemaining );
#if ( MCU_SELECTED == NXP_K24 )
static void             Configure_LLWU( void );
static void             EnterVLLS( uint16_t uCounter, PWRLG_LPTMR_Units eUnits, uint8_t uMode );
static void             EnterLLS( uint16_t uCounter, PWRLG_LPTMR_Units eUnits );
#endif
static void             HardwareShutdown( void );
static void             NextSleep( void );
static void             TxCallback( MAC_DATA_STATUS_e status, uint16_t Req_Resp_ID );
static void             LptmrStart( uint16_t uCounter, PWRLG_LPTMR_Units eUnits, PWRLG_LPTMR_Mode eMode );
static void             LptmrEnable( bool bEnableInterrupt );
static bool             powerStable( bool curState );
#if ( PWRLG_PRINT_ENABLE != 0 )
#define LG_PRNT_INFO( fmt, ... ) UART_polled_printf( fmt, ##__VA_ARGS__ )
#else
#define LG_PRNT_INFO( fmt, ... )
#endif // #if ( PWRLG_PRINT_ENABLE != 0 )

#if 0
#if ( MCU_SELECTED == RA6E1 )
static void EnterLowPowerMode( uint16_t uCounter, PWRLG_LPTMR_Units eUnits, uint8_t uMode );

#define EnterVLLS(counter, eUnits, uMode)    EnterLowPowerMode(counter, eUnits, uMode)  // TODO: RA6: DG: Convert the old code or the new?
#endif
#else
#define EnterVLLS(counter, eUnits, uMode)  /* TODO: RA6: DG: Remove later */
#endif // if 0

#if ( MCU_SELECTED == NXP_K24 )
/***********************************************************************************************************************

   Function Name: PWRLG_LLWU_ISR

   Purpose: Interrupt due to LLWU when in LLS mode only.

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void PWRLG_LLWU_ISR( void )
{
   /* after exiting LLS clear the wake-up flag in the LLWU-write one to clear the flag */
   volatile uint8_t LLWU_F1_TMP;
   volatile uint8_t LLWU_F2_TMP;
   // volatile uint8_t LLWU_F3_TMP; TODO: Check how this flag needs to be cleared

    /* Read LLWU_Fx into temporary LLWU_Fx_TMP variables */

    LLWU_F1_TMP = LLWU_F1;
    LLWU_F2_TMP = LLWU_F2;
    // LLWU_F3_TMP = LLWU_F3;

    /* clean wakeup flags */
    LLWU_F1 = LLWU_F1_TMP;
    LLWU_F2 = LLWU_F2_TMP;

    if(LLWU_FILT1 & LLWU_FILT1_FILTF_MASK)
    {
        LLWU_FILT1 |= LLWU_FILT1_FILTF_MASK;
    }

    if(LLWU_FILT2 & LLWU_FILT2_FILTF_MASK)
    {
        LLWU_FILT2 |= LLWU_FILT2_FILTF_MASK;
    }
}
#endif // #if ( MCU_SELECTED == NXP_K24 )

/***********************************************************************************************************************

   Function name: PWRLG_Task

   Purpose: This is the first task installed during a power up in "last gasp" mode. See mqx_main.c
            Init the functions in the startUpTbl[] that have the "STRT_FLAG_LAST_GASP" flag set.
            Start the tasks in the MQX_template_list_last_gasp[].
            Set up to transmit outage alarm messages, if any remain. If none remain, reset the micro.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: void

   Re-entrant Code:

   Notes:

 **********************************************************************************************************************/
void PWRLG_Task( taskParameter )
{

   OS_TICK_Struct             endTime;
   OS_TICK_Struct             startTime;
#if ( RTOS_SELECTION == MQX_RTOS )
   _task_id                   taskID;
#endif
   STRT_FunctionList_t const  *pFunct;
   OS_TASK_Template_t  const *pTaskList; /* Pointer to task list which contains all tasks in the system */
//   bool                       bOverflow = ( bool )false;
   uint8_t                    startUpIdx;
   uint8_t                    hwVerString[VER_HW_STR_LEN];

   // Create the semaphore used to signal TX complete.
   //TODO NRJ: determine if semaphores need to be counting
   ( void )OS_SEM_Create( &TxDoneSem, 0 );

   // Seed the random number generator for P persistence test in the MAC.
   aclara_srand( PWRLG_MILLISECONDS() );

   /* Initialize all of the modules here: */
   for ( startUpIdx = 0, pFunct = &startUpTbl[0]; startUpIdx < uStartUpTblCnt; startUpIdx++, pFunct++ )
   {
      if ( pFunct->uFlags & STRT_FLAG_LAST_GASP )
#if ( DEBUG_PWRLG == 0 )
      {
         ( void )pFunct->pFxnStrt();
      }
#else
      {
         returnStatus_t response = pFunct->pFxnStrt();
         if ( eSUCCESS != response )
         {
            /* This condition should only show up in development. This should help someone figure out
               that there is an issue initializing a task. */
            ( void )printf( "\n\t\t#####################\n" );
            ( void )printf( "\nPWRLG_Startup Failure - Call to %s failed, Code: %u\n",
                              pFunct->name, ( uint16_t )response );
            ( void )printf( "\n\t\t#####################\n" );
         }
      }
#endif
   }
   printf("LAST GASP TASK");

#if ( RTOS_SELECTION == MQX_RTOS )
   /* Install exception handler */
   ( void )_int_install_exception_isr();

   /* Install LLWU ISR */

   if ( NULL != _int_install_isr( LLWU_LG_IRQInterruptIndex, ( INT_ISR_FPTR )PWRLG_LLWU_ISR, ( void * )NULL ) )
   {
      ( void )_bsp_int_init( LLWU_LG_IRQInterruptIndex, LLWU_LG_ISR_PRI, LLWU_LG_ISR_SUB_PRI, ( bool )true );
   }
#endif

   /* Create/start all the tasks */
   for ( pTaskList = &OS_template_list_last_gasp[0]; 0 != pTaskList->TASK_TEMPLATE_INDEX; pTaskList++ )
   {
      if ( !( pTaskList->TASK_ATTRIBUTES & MQX_AUTO_START_TASK ) )
      {
#if ( RTOS_SELECTION == MQX_RTOS )
         taskID = OS_TASK_Create( pTaskList );
         ( void )_task_set_exception_handler( taskID, task_exception_handler );
#elif (RTOS_SELECTION == FREE_RTOS)
         if ( pdPASS != OS_TASK_Create(pTaskList) )
         {
            printf("\nUnable to Start Tasks\n");
         }
         /* TODO: RA6: Add exception Handler */
#endif
      }
   }

   /* Start the communications stack in DEAF mode. */
   ( void )SM_StartRequest( eSM_START_DEAF, NULL );

   char  floatStr[PRINT_FLOAT_SIZE];
   float Vcap = ADC_Get_SC_Voltage();
   // Get HW revision letter
   ( void )VER_getHardwareVersion ( &hwVerString[0], sizeof(hwVerString) );
   hwRevLetter_ = hwVerString[0];
   DBG_logPrintf( 'I', "\n" );
   DBG_logPrintf( 'I', "Running: %u of %u", pSysMem->uMessageCounts.sent + 1, pSysMem->uMessageCounts.total  );
   DBG_logPrintf( 'I', "PWRLG_STATE: %d", PWRLG_STATE() );
   DBG_logPrintf( 'I', "Vcap:            %-6s", DBG_printFloat( floatStr, Vcap, 3 ) );
   DBG_logPrintf( 'I', "PWR Qual count: %d", pVbatMem->pwrQualCount );
   DBG_logPrintf( 'I', "outage: %d, Short outage: %d", PWRLG_OUTAGE(), VBATREG_SHORT_OUTAGE );
   DBG_logPrintf( 'I', "HW Rev Letter: %c ", hwRevLetter_);

   if ( PWRLG_MESSAGE_NUM() != 0 )
   {
      DBG_logPrintf( 'I', "Previous Message %u ms", PREV_MSG_TIME() );
   }
   DBG_logPrintf( 'I', "Remaining time:  %ums", PWRLG_MILLISECONDS() );
   DBG_logPrintf( 'I', "CSMA Total/Last: %ums/%ums, TxFail: %u",
                     TOTAL_BACK_OFF_TIME(), CUR_BACK_OFF_TIME(), TX_FAILURES() );

   OS_TASK_Sleep( OS_SLEEP_PRINT_DELAY_MS ); // TODO: RA6E1: DG: Remove

   if ( PWRLG_STATE_TRANSMIT == PWRLG_STATE() )   /* This should ALWAYS be the case.  */
   {
      OS_TICK_Get_CurrentElapsedTicks( &startTime );
      PWRLG_STATE_SET( PWRLG_STATE_WAIT_FOR_RADIO );

      // Don't send last gasp messages when in ship mode, decommission mode or quiet mode.
      if ( 0 == MODECFG_get_ship_mode() && 0 == MODECFG_get_decommission_mode() && 0 == MODECFG_get_quiet_mode() )
      {
         DBG_logPrintf( 'I', "Not ship mode" );

         /* Wait for the stack to be ready. Poll the stack manager for state change from "unknown".
            This also allows time for DTLS to determine if the session has been established.
         */
         SM_WaitForStackInitialized();

#if 0 /* This test is already made by the HEEP_MSG_Tx routine */
#if ( USE_DTLS == 1 )
         uint8_t  securityMode;

         ( void )APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &securityMode, NULL );

         if ( securityMode == 2 )
         {
#if ( DTLS_FIELD_TRIAL == 0 )
            DBG_logPrintf( 'I', "Waiting for DTLS" );
            while ( !DTLS_IsSessionEstablished() ) /* Wait for session established (NV read completes)   */
#endif
            {
               OS_TASK_Sleep( 1 );
            }
         }
#endif
#endif
         PWRLG_SetCsmaParameters();    /* Set parameters as needed for last gasp mode, only. */
         TxSuccessful = false;
         while ( !TxSuccessful && ( VBATREG_CURR_TX_ATTEMPT < VBATREG_MAX_TX_ATTEMPTS ) )
         {
            RDO_PA_EN_ON();  /* Enable the Radio PA  */

#if ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
            // Option 2
            TRACE_D1_HIGH();
#endif
            if ( eSUCCESS == PWRLG_TxMessage( PWRLG_TIME_OUTAGE() ) )
            {
               DBG_logPrintf( 'I', "Current Attempt: %d of Max %d Attempts , SemPend 500ms", VBATREG_CURR_TX_ATTEMPT, ( VBATREG_MAX_TX_ATTEMPTS - 1 ) );
               ( void )OS_SEM_Pend( &TxDoneSem, 500 ); /* Wait up to 500ms for tx complete.   */
            }
            VBATREG_CURR_TX_ATTEMPT++;
#if ( PWRLG_PRINT_ENABLE != 0 )
            OS_TASK_Sleep( OS_SLEEP_PRINT_DELAY_MS ); // Adding for Print
#endif
            if ( !TxSuccessful )
            {
               RDO_PA_EN_OFF();  /* Disable the Radio PA  */
#if ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
               // Option 2
               TRACE_D1_LOW();
#endif

               DBG_logPrintf('I',"Failed TX!!");
               PWRLG_TxFailure();
            }
            else
            {
               DBG_logPrintf('I', "TxSuccesss ");
            }
         }

         OS_TICK_Get_CurrentElapsedTicks( &endTime );
         // Save the time spent transmitting
         PREV_MSG_TIME_SET(  ( uint16_t ) ( OS_TICK_Get_Diff_InMicroseconds( &startTime, &endTime ) / 1000 ) );

         PWRLG_SENT_SET( 1 );
      }
   }

   //Radio shutdown to ensure transmissions are disabled ASAP
   radio_hal_RadioImmediateSDN();

   DBG_logPrintf( 'I', "Wait for NV" );
   dvr_shm_lock();/*lint !e454 */                   /* Wait for any NV activity to complete   */
#if ( ( DEBUG_PWRLG != 0 ) || ( LG_QUICK_TEST != 0 ) )
   OS_TASK_Sleep( 150 );
#endif
   NextSleep();   /* Never returns; implements VLLSx and waits for interrupt, which causes a reset.   */

   // We should never reach this point.
   DBG_logPrintf( 'I', "Completed" );
#if 0  // Dead code
   for( ;; )
   {
      OS_TASK_Sleep( ONE_SEC );
   }
#endif
} /*lint !e454 !e715 Arg0 not used; required by task template   */

/***********************************************************************************************************************

   Function name: PWRLG_SetCsmaParameters

   Purpose: Set the minimum p persistence, CSMA max attempts, and CSMA quick abort values for last gasp.

   Arguments: void

   Returns: void

   Re-entrant Code: No

   Notes:  Expect these values to not be permanent (i.e. the file is not written)

 **********************************************************************************************************************/
void PWRLG_SetCsmaParameters( void )
{
   MAC_GetConf_t GetConf;
#if 0
   uint8_t       CsmaMaxAttempts = 1;
#endif
   bool          CsmaQuickAbort  = true;

   // Set p persistence to minimum allowed by last gasp.
   GetConf = MAC_GetRequest( eMacAttr_CsmaPValue );
   if ( GetConf.eStatus == eMAC_GET_SUCCESS )
   {
#if ( LG_WORST_CASE_TEST == 0 )
      if ( GetConf.val.CsmaPValue < fMinimumCsmaPValue )
      {
         ( void )MAC_SetRequest( eMacAttr_CsmaPValue, &fMinimumCsmaPValue );
      }
#else
      if ( VBATREG_CURR_TX_ATTEMPT < ( VBATREG_MAX_TX_ATTEMPTS - 1 )  )
      {
         ( void )MAC_SetRequest( eMacAttr_CsmaPValue, &fMinimumCsmaPValue );
         ( void )PHY_SetRequest( ePhyAttr_CcaThreshold, &phyCCAThreshold );
      }
#endif
   }
#if 0 // No need to set the max attempts to 1
   // Set CSMA max attempts to 1.  OK to update since it is not actually writing the file
   ( void )MAC_SetRequest( eMacAttr_CsmaMaxAttempts, &CsmaMaxAttempts );
#endif
   // Set CSMA quick abort to true.  OK to update since it is not actually writing the file
   ( void )MAC_SetRequest( eMacAttr_CsmaQuickAbort, &CsmaQuickAbort );
}

/***********************************************************************************************************************

   Function name: TxCallback

   Purpose: Waits for a callback from the MAC layer indicating that the APP_MSG_Tx has completed and posts the
            TxDoneSem to signal to PWRLG_Task that message transmit is complete.

   Arguments: mac_tx_status_e status, uint8_t Req_Resp_ID

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
/*lint -e{123} status argument same name as macro elsewhere is OK */
static void TxCallback( MAC_DATA_STATUS_e status, uint16_t Req_Resp_ID )
{
   ( void )Req_Resp_ID;

   if ( eMAC_DATA_SUCCESS == status )
   {
      TxSuccessful = true;
   }

#if ( LAST_GASP_SIMULATION == 1 )
   /* This is to differentiate function flow between LG Sim path and Real Last Gasp */
   if ( eSimLGDisabled == EVL_GetLGSimState() )
   {
#endif
      OS_SEM_Post( &TxDoneSem );   /* Post the semaphore */
#if ( LAST_GASP_SIMULATION == 1 )
   }
   else
   {
      EVL_LGSimTxDoneSignal( eMAC_DATA_SUCCESS == status );
   }
#endif
   return;
}

/***********************************************************************************************************************

   Function name: PWRLG_TxMessage

   Purpose:

   Arguments: None

   Returns: Success/Failure

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
returnStatus_t PWRLG_TxMessage( uint32_t timeOutage )
{
   buffer_t             *pMessage;
   pack_t               packCfg;                /* Struct needed for PACK functions */
   pFullMeterRead       payload;                /* Pointer to response payload */
   returnStatus_t       returnStatus = eFAILURE;
   sysTimeCombined_t    timeCombinedMessage;
   EventRdgQty_u        EventRdgQty;            /* Count of events/readings in the payload */
   meterReadingType     eventType;
   uint32_t             timeMessage;

#if ( LAST_GASP_SIMULATION == 1 )
   if ( eSimLGDisabled == EVL_GetLGSimState() )
   {
#endif
      eventType = ( meterReadingType )BIG_ENDIAN_16BIT ( comDevicePowerFailed );
#if ( LAST_GASP_SIMULATION == 1 )
   }
   else
   {
      eventType = ( meterReadingType )BIG_ENDIAN_16BIT ( comDevicepowertestfailed );
   }
#endif
   pMessage = BM_alloc( sizeof( FullMeterRead ) );

   if ( pMessage != NULL )
   {
      HEEP_APPHDR_t heepHdr;

      ( void )TIME_UTIL_GetTimeInSysCombined( &timeCombinedMessage );
      timeMessage = ( uint32_t )( timeCombinedMessage / TIME_TICKS_PER_SEC );

      /* meter payload begins at data element */
      payload = ( pFullMeterRead )&pMessage->data[0]; //lint !e826

      /* Prepare a message to the head end
         Application header/QOS info */
      HEEP_initHeader( &heepHdr );
      heepHdr.TransactionType   = ( uint8_t )TT_ASYNC;
      heepHdr.Resource          = ( uint8_t )bu_am;
      heepHdr.Method_Status     = ( uint8_t )method_post;
      heepHdr.Req_Resp_ID       = 0;
      heepHdr.qos               = 0x38;
      heepHdr.callback          = TxCallback;

      /* valuesInterval.end = current time */
      PACK_init( ( uint8_t* ) &payload->eventHeader.intervalTime, &packCfg );
      PACK_Buffer(-(int16_t)sizeof( timeMessage ) * 8, ( uint8_t * )&timeMessage, &packCfg);

      EventRdgQty.bits.eventQty = 1;            /*  One event  */
      EventRdgQty.bits.rdgQty = 0;              /*  No readings */
      PACK_Buffer( sizeof( EventRdgQty ) * 8, ( uint8_t * )&EventRdgQty, &packCfg );

      /* EndDeviceEvents.createdDateTime = outage time */
      PACK_Buffer(-(int16_t)sizeof( timeOutage ) * 8, ( uint8_t * )&timeOutage, &packCfg);

      /* EndDeviceEvents.createdDateTime = outage time */
      PACK_Buffer( sizeof( eventType ) * 8, ( uint8_t * )&eventType, &packCfg );

      payload->nvp = 0;
      PACK_Buffer( sizeof( payload->nvp ) * 8, ( uint8_t * )&payload->nvp, &packCfg );

      /* Fill in the size of the payload to send to the head end
         Length is current "pack" pointer - pMessage->data */
      pMessage->x.dataLen = ( uint16_t )( packCfg.pArray - ( uint8_t* )pMessage->data );

      returnStatus = HEEP_MSG_Tx( &heepHdr, pMessage );
      if ( eSUCCESS == returnStatus )
      {
         VBATREG_PACKET_ID++;  /* Ensure that messages have unique packet IDs to prevent T-board from rejecting as duplicate. */
      }
   }

   return( returnStatus );
}

/***********************************************************************************************************************

   Function name: PWRLG_TxFailure

   Purpose: Called when the message transmission fails. Compute next transmission time. Sleep or enter low power mode
            based on the delay.

   Arguments: None

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWRLG_TxFailure( void )
{
   static const uint32_t uCsmaSleepThreshold = 30;
   uint32_t uMilliseconds;
#if ( LG_WORST_CASE_TEST == 1 )
   MAC_GetConf_t GetConf;
   int16_t       ccaThreshold  = PHY_CCA_THRESHOLD_MAX;
   uint8_t       ccaOffset     = PHY_CCA_OFFSET_MAX;
   /* Update the P-persist value for last CCA TX attempt */
   if ( VBATREG_CURR_TX_ATTEMPT == ( VBATREG_MAX_TX_ATTEMPTS - 1 ) )
   {
      LG_PRNT_INFO("\nTxFail():Update P-Persist value\n ");

      ( void )MAC_SetRequest( eMacAttr_CsmaPValue, &fMaxCsmaPValue );
      ( void )PHY_SetRequest( ePhyAttr_CcaThreshold, &ccaThreshold );
      ( void )PHY_SetRequest( ePhyAttr_CcaOffset, &ccaOffset );
      /* No need to worry about the resetting the parameters, as in this mode, we are only making the changes to the RAM copy. */
   }
#endif

#if ( LG_WORST_CASE_TEST == 0 )
   uMilliseconds = aclara_randu( MAC_CSMA_MIN_BACKOFF_TIME_DEFAULT, MAC_CSMA_MAX_BACKOFF_TIME_DEFAULT );
#else
   GetConf = MAC_GetRequest( eMacAttr_CsmaMaxBackOffTime );
   if ( GetConf.eStatus == eMAC_GET_SUCCESS )
   {
      uMilliseconds = GetConf.val.CsmaMaxBackOffTime;
      LG_PRNT_INFO("\nCsmaMaxBackOffTime\n");
   }
#endif

   if ( PWRLG_MILLISECONDS() > uMilliseconds )
   {
      PWRLG_MILLISECONDS() -= uMilliseconds;
   }

   TOTAL_BACK_OFF_TIME_SET( TOTAL_BACK_OFF_TIME()+(uint16_t)uMilliseconds );  /* Update total backoff milliseconds  */
   CUR_BACK_OFF_TIME_SET( ( uint16_t )uMilliseconds );                        /* Update backoff time for this attempt */
   TX_FAILURES_SET( TX_FAILURES() + ( uint16_t )1 );                          /* Update Tx failure count */

   if ( uMilliseconds <= uCsmaSleepThreshold )
   {
      // The CSMA back-off is short enough, just sleep.
      OS_TASK_Sleep( uMilliseconds );
   }
   else
   {
#if ( LAST_GASP_SIMULATION == 1 )
      /* This is to differentiate function flow between LG Sim path and Real Last Gasp */
      if ( eSimLGDisabled == EVL_GetLGSimState() )
      {
#endif

#if ( MCU_SELECTED == NXP_K24 )
         /* The CSMA back-off is long, switch to LLS mode.  */
#if ( PWRLG_PRINT_ENABLE == 0 )
         EnterLLS( ( uint16_t )( uMilliseconds ), LPTMR_MILLISECONDS );
#else
         /* Subtracting the sleep delay added for print*/
         EnterLLS( ( uint16_t )( uMilliseconds - OS_SLEEP_PRINT_DELAY_MS ), LPTMR_MILLISECONDS );
#endif
#endif // #if ( MCU_SELECTED == NXP_K24 )  // TODO: RA6: Add SS Support for RA6
#if ( LAST_GASP_SIMULATION == 1 )
      }
      else
      {
         //TODO: How to treat this for LG Simulation path?
         OS_TASK_Sleep( uMilliseconds );
      }
#endif
   }
}

#if ( RTOS_SELECTION == MQX_RTOS )
/***********************************************************************************************************************

   Function name: PWRLG_Idle_Task

   Purpose:

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWRLG_Idle_Task( uint32_t Arg0 )
{
   ( void )Arg0;

   for( ;; )
   {
   }
}
#endif  // ( RTOS_SELECTION == MQX_RTOS )
/***********************************************************************************************************************

   Function name: PWRLG_startup

   Purpose: Called from mqx_main when processor is reset. Either sets up
            the next sleep cycle, if necessary, or enters VLLS1 until woken up by a power on reset.

   Arguments: None

   Returns: void

   Re-entrant Code: No

   Notes:   This code is called from mqx_main, before the OS is running. So, in that case, no OS dependent routines may
            be used ( e.g., DBG_logPrintf, OS_TASK_Sleep, etc. )

 **********************************************************************************************************************/
void PWRLG_Startup( void )
{
#if ( MCU_SELECTED == NXP_K24 )
   /* Enable the Low-power Run or Stop mode */
   /* Note: According to K24 reference manual: PMPROT register can be written only once after any system reset */
   if( !(SMC_PMPROT & (SMC_PMPROT_AVLP_MASK | SMC_PMPROT_ALLS_MASK | SMC_PMPROT_AVLLS_MASK)) )
   {
      SMC_PMPROT = (SMC_PMPROT_AVLP_MASK | SMC_PMPROT_ALLS_MASK | SMC_PMPROT_AVLLS_MASK);
   }
#endif
   VBATREG_EnableRegisterAccess();
   /* Last Gasp or Restoration not active in Ship mode or Shop mode(a.k.a decommission mode) */
   if ( ( 0 == VBATREG_SHIP_MODE ) && ( 0 == VBATREG_METER_SHOP_MODE ) )
   {
#if ( DEBUG_PWRLG == 0 )
#if ( MCU_SELECTED == NXP_K24 )
      uint8_t uLLWU_F3    = LLWU_F3;         /* Copy of LLWU module wakeup status register (LPTMR0 expired status).  */
      uint8_t uLLWU_FILT1 = LLWU_FILT1;      /* Copy of LLWU pin detect wakeup status register (PF changed status).  */
      PWRLG_LLWU_FILT1_SET( uLLWU_FILT1 );

      BRN_OUT_TRIS();                        /* Enable the /PF signal GPIO input. */
      LLWU_FILT1  = LLWU_FILT1_FILTF_MASK;   /* Reset PF changed interrupt flag, if any.   */
#elif ( MCU_SELECTED == RA6E1 )
      /* TODO: RA6: Enable PF meter Interrupt */
#endif
#else  //if ( DEBUG_PWRLG == 1 )
      BRN_OUT_IRQ_EI();                      /* Enable the /PF signal GPIO input and ISR */
#endif

#if ( DEBUG_PWRLG != 0 )
      GRN_LED_PIN_TRIS();
      GRN_LED_PIN_DRV_LOW();

      RED_LED_PIN_TRIS();
      RED_LED_PIN_DRV_LOW();

      BLU_LED_PIN_TRIS();
      BLU_LED_PIN_DRV_LOW();

      BLU_LED_TOGGLE();                      /* Toggle the RED led upon every entry to this startup routine.   */
#endif
#if ( MCU_SELECTED == NXP_K24 )
      LG_PRNT_INFO( "PWRLG_Startup: F3: 0x%02x, latched FILT1: 0x%02x, FILT1: 0x%02x\n\r", uLLWU_F3, uLLWU_FILT1, LLWU_FILT1  );
#endif
      LG_PRNT_INFO("\nLG State:%d\n\r", PWRLG_STATE() );
#if ( MCU_SELECTED == NXP_K24 )
      // Save the low order 8 bits of the RTC Status Register saved at reset before MQX modifies it.
      VBATREG_RTC_SR() = ( uint8_t )( RTC_SR & ( uint32_t )0xFF );
#elif ( MCU_SELECTED == RA6E1 )
      /* TODO: RA6: Do we need to special code for RA6? */
#endif
#if ( MCU_SELECTED == NXP_K24 )
      PWRLG_LLWU_F3_SET( uLLWU_F3 );
#endif

#if ( MCU_SELECTED == NXP_K24 )
      /* Based on observation while using the emulator, it appears that every LLWU wakeup has the PF change signal asserted.
         So, instead of relying on that signal alone, check the LP timer as the cause of the wakeup, first. */
      /* This block handles timer event.   */
      if ( ( uLLWU_F3 & LLWU_F3_MWUF0_MASK ) != 0  )
#elif ( MCU_SELECTED == RA6E1 )
         /* TODO: RA6: DG: Do we need to check DPSRSTF before entering here?? ?*/
      if ( R_SYSTEM->DPSIFR2_b.DRTCAIF != 0 )  /* RTC Alarm Interrupt */
#endif
      {
         if ( !BRN_OUT() )                         /* If power is on... */
         {
            if ( PWRLG_STATE() != PWRLG_STATE_WAIT_RESTORATION ) /* do not increment PQE if awoke waiting restore*/
            {
               if( RTC_Valid() )
               {
#if ( MCU_SELECTED == NXP_K24 )
                  RESTORATION_TIME_SET ( RTC_TSR );   /* Record RTC seconds at power restored time.   */
#elif ( MCU_SELECTED == RA6E1 )
                  /* TODO: RA6: DG: Update Restoration Time */
#endif
               }
               VBATREG_PWR_QUAL_COUNT++;           /* Increment the power quality count.  */
            }

            if ( PWRLG_OUTAGE() == 1 )             /* Check for an outage declared. */
            {
               if ( PWRLG_STATE() == PWRLG_STATE_WAIT_RESTORATION )  /* Outage declared, if the restoration time has been
                                                                        met, exit last gasp.  */
               {
#if 0 /* For debugging only, with power actually removed. */
                  RED_LED_PIN_TRIS();
                  RED_LED_PIN_DRV_LOW();
                  for ( uint32_t i = 0; i < 10000000; i++ )
                  {
                     RED_LED_ON();
                  }
#endif
                  VBATREG_POWER_RESTORATION_MET = 1;
                  PWRLG_STATE_SET( PWRLG_STATE_COMPLETE );
                  PWRLG_LLWU_SET( 0 ); /* Don't re-enter last gasp on next reset. */
#if ( DEBUG_PWRLG != 0 )
                  uLLWU_F3 = 0;        /* Clear the cause(s) of the wakeup.   */
                  uLLWU_FILT1 = 0;
#endif
                  RESET(); /* Power restored, reset to restart MTU.  PWR_SafeReset() not necessary */
               }
               else        /* Outage declared, restoration not met. Set state to "wait for restoration" and sleep.   */
               {
                  LG_PRNT_INFO( "Wait for Restoration on Timer\n\r" );
                  PWRLG_LLWU_SET( 1 );       /* Re-enter last gasp on next reset.   */
                  PWRLG_STATE_SET( PWRLG_STATE_WAIT_RESTORATION );
                  EnterVLLS( VBATREG_POWER_RESTORATION_TIMER, LPTMR_SECONDS, 1 );
               }
            }
            else  /* Outage NOT declared, check for LAST_GASP_EXIT delay met. */
            {
               if ( PWRLG_STATE() == PWRLG_STATE_WAIT_LP_EXIT )
               {
#if 0 /* For debugging only, with power actually removed. */
                  BLU_LED_PIN_TRIS();
                  BLU_LED_PIN_DRV_LOW();
                  for ( uint32_t i = 0; i < 10000000; i++ )
                  {
                     BLU_LED_ON();
                  }
#endif
                  VBATREG_POWER_RESTORATION_MET = 1;
                  PWRLG_STATE_SET( PWRLG_STATE_COMPLETE );
                  PWRLG_LLWU_SET( 0 ); /* Don't re-enter last gasp on next reset. */
#if ( DEBUG_PWRLG != 0 )
                  uLLWU_F3 = 0;        /* Clear the cause(s) of the wakeup.   */
                  uLLWU_FILT1 = 0;
#endif
                  RESET();             /* Power restored, reset to restart MTU.   */
               }
               else
               {
                  LG_PRNT_INFO( "Wait for LP EXIT\n\r" );
                  PWRLG_STATE_SET( PWRLG_STATE_WAIT_LP_EXIT );
                  EnterVLLS( VBATREG_POWER_RESTORATION_TIMER, LPTMR_MILLISECONDS, 1 );  //DG: Why???
               }
            }
         }
         /* At this point, the LP timer is the cause of the wakeup and power is NOT present. Declare an outage and continue
            with next step of last gasp.  */
         PWRLG_OUTAGE_SET( 1 );     /* Outage declared.  */
         VBATREG_SHORT_OUTAGE = 0;  /* No longer a short outage!  */
         LG_PRNT_INFO("Long_Outage");
         NextSleep();
         /* If next sleep returns, either the current state is SLEEP, and the initial declaration delay has just expired,
            or all the messages have been sent.  */
         if ( PWRLG_STATE() != PWRLG_STATE_TRANSMIT )
         {
#if ( DEBUG_PWRLG != 0 )
            uLLWU_F3 = 0;        /* Clear the cause(s) of the wakeup.   */
            uLLWU_FILT1 = 0;
#endif
            LG_PRNT_INFO("\n Don't Re-Enter LG \n");
            PWRLG_LLWU_SET( 0 ); /* Don't re-enter last gasp on next reset. */
         }
      }
#if ( MCU_SELECTED == NXP_K24 )
      /* At this point, the LP timer was NOT the cause of the wake up. Might be the PF changing state.  */
      else if ( ( uLLWU_FILT1 & LLWU_FILT1_FILTF_MASK ) != 0 )
#elif ( MCU_SELECTED == RA6E1 )
      /* Note: If the PF_METER pin were to change on the RA6E1, the below line needs to be updated */
      else if( R_SYSTEM->DPSIFR1_b.DIRQ11F ) /* IRQ11-DS - Change in PF_METER */
#endif
      {
#if ( DEBUG_PWRLG != 0 )
         while ( !powerStable( ( bool )BRN_OUT() ) ) /* Make sure power isn't "bouncing". Probably needed only when debugging. */
         {}
#endif
         {
            if ( !BRN_OUT() ) /* Power restored. */
            {
               RESTORATION_TIME_SET( 0 ); /* assume the RTC time is invalid */
               if( RTC_Valid() )
               {
#if ( MCU_SELECTED == NXP_K24 )
                  RESTORATION_TIME_SET ( RTC_TSR );   /* Record RTC seconds at power restored time.   */
#elif ( MCU_SELECTED == RA6E1 )
                  /* TODO: RA6: DG: Update Restoration Time */
#endif
               }
               VBATREG_PWR_QUAL_COUNT++;              /* Increment the power quality count.  */
               if ( ( 0 == VBATREG_POWER_RESTORATION_MET ) && ( 1 == PWRLG_OUTAGE() ) )
               {
                  /* Outage declared, but restoration delay not met. Sleep for restoration delay. */
                  PWRLG_STATE_SET( PWRLG_STATE_WAIT_RESTORATION );
                  LG_PRNT_INFO( "Wait for Restoration on /PF \n\r" );
                  EnterVLLS( VBATREG_POWER_RESTORATION_TIMER, LPTMR_SECONDS, 1 );
               }
               else
               {
                  /* Outage declared. Restoration delay met. Exit low power mode.   */
                  VBATREG_POWER_RESTORATION_MET = 1;
                  LG_PRNT_INFO( "Restoration Delay Met\n\r" );
                  PWRLG_LLWU_SET( 0 ); /* Don't re-enter last gasp on next reset. */
#if   ( DEBUG_PWRLG != 0 )
                  uLLWU_F3 = 0;        /* Clear the cause(s) of the wakeup.   */
                  uLLWU_FILT1 = 0;
#endif
                  RESET(); /* PWR_SafeReset() not necessary */
               }
            }
            else  /* Power fail asserted while in last gasp.   */
            {
               LG_PRNT_INFO("Power failed in LG\n\r");
               if ( PWRLG_OUTAGE() == 0 )   /* Outage not declared  */
               {
                  /* Restart last gasp. Still early in the last gasp process - assume there is sufficient capacity to start
                     over with the declaration delay. Reset sleep time to Outage declaration value. */
                  PWRLG_MESSAGE_NUM_SET( 0 );            /* Init number of messages sent. */
                  VBATREG_CURR_TX_ATTEMPT = 0;           /* Init number of transmission attempts   */
                  PWRLG_STATE_SET( PWRLG_STATE_SLEEP );  /* Set state to "sleep"  */

                  PWRLG_SOFTWARE_RESET_SET( 1 );   /* This bit is set so it can be detmerined if last gasp was terminated
                                                   before or after all last gasp messages were sent. This bit will
                                                   remain set if last gasp was terminated by power being restore before
                                                   all capacity has been used. */
                  if ( VBATREG_OUTAGE_DECLARATION_DELAY <= 60000 )
                  {
                     EnterVLLS( ( uint16_t )VBATREG_OUTAGE_DECLARATION_DELAY, LPTMR_MILLISECONDS, 1 );
                  }
                  else
                  {
                     EnterVLLS( ( uint16_t )( VBATREG_OUTAGE_DECLARATION_DELAY / 1000 ), LPTMR_SECONDS, 1 );
                  }
               }
               else
               {
                  /* Outage declared. Continue with messages left. */
                  if ( PWRLG_MESSAGE_NUM() < PWRLG_MESSAGE_COUNT() ) /* Any messages left?   */
                  {
                     PWRLG_STATE_SET( PWRLG_STATE_TRANSMIT );
                     EnterVLLS( ( uint16_t ) 0, LPTMR_MILLISECONDS, 1 );   /* Continue with current time out value.  */
                  }
                  else  /* All messages sent, wait for power restored.   */
                  {
                     /* Done. Sleep until PF signal released.  */
                     PWRLG_LLWU_SET( 1 );    /*  Stay in last gasp mode at next reset. */
                     PWRLG_STATE_SET( PWRLG_STATE_WAIT_FOR_RADIO );
                     EnterVLLS( 0, LPTMR_SLEEP_FOREVER, 1 );
                  }
               }
            }
         }
      }
   }
   else /* In Ship mode or Shop mode. Hence Do not perform Last Gasp feature */
   {
      PWRLG_LLWU_SET( 0 ); /* Don't re-enter last gasp on next reset. */
      LG_PRNT_INFO( " Last Gasp is not active \n" );
      PWRLG_STATE_SET( PWRLG_STATE_SLEEP );  /* Set state to "sleep"  */
   }
}

/***********************************************************************************************************************

   Function name: PWRLG_Restoration

   Purpose: Check and execute "Outage Restoration" feature if required

   Arguments: uint16_t powerAnomalyCount - Read from the NV.

   Returns: None

   Re-entrant Code: No

   Notes: 1. Reset Causes/Source:
            * LOW_LEAKAGE_WAKEUP : * observed if the power restored after sending one message and while in VLLS1 mode.
                                   * observed when meter is supplying power but /PF asserted.
            * LOW_VOLTAGE: observed when meter is supplying power but /PF asserted
         2. VBATREG_PWR_QUAL_COUNT : Is modified in this function

 **********************************************************************************************************************/
void PWRLG_Restoration( uint16_t powerAnomalyCount )
{

   VBATREG_POWER_RESTORATION_TIMER = PWRCFG_get_restoration_delay_seconds(); // TODO:DG: We can write to VBAT while initializing

   /* Read the Modes from NV */
   if( ( 0 == MODECFG_get_ship_mode() ) && ( 0 == MODECFG_get_decommission_mode() ) )
   {
      uint16_t resetCause;

      resetCause = PWR_getResetCause();
      LG_PRNT_INFO("\n Reset Cause: %d \n ",resetCause);
      if( ( resetCause & RESET_SOURCE_POWER_ON_RESET ) ||
              ( resetCause & RESET_SOURCE_LOW_LEAKAGE_WAKEUP ) ||
                 ( resetCause & RESET_SOURCE_LOW_VOLTAGE ) )
      {
         if ( 0 == RESTORATION_TIME() )/* If never updated, bump the power quality count. */
         {
            if( RTC_Valid() )
            {
#if ( MCU_SELECTED == NXP_K24 ) /* TODO: RA6: DG:  Set restoration time for RA6 */
               RESTORATION_TIME_SET ( RTC_TSR );                  /* Record RTC seconds at power restored time. */
#endif
            }
            VBATREG_PWR_QUAL_COUNT = powerAnomalyCount + 1;    /* Increment the power quality/anomaly count. */
         }
         if ( ( 0 == VBATREG_SHORT_OUTAGE ) && ( 0 == PWRLG_OUTAGE() ) )
         {
            /* Meaning it was an outage long enough to drain VBAT Super Cap */
            /* Wait for Restoration */
            LG_PRNT_INFO( "PWRLG_Restoration: state = 0x%02x, outage: 0x%02x\n", PWRLG_STATE(), PWRLG_OUTAGE() );
            PWRLG_OUTAGE_SET( 1 );     /* Declare it was an outage. */
            PWRLG_LLWU_SET( 1 );       /* Re-enter last gasp on next reset. */
            PWRLG_STATE_SET( PWRLG_STATE_WAIT_RESTORATION );
            EnterVLLS( VBATREG_POWER_RESTORATION_TIMER, LPTMR_SECONDS, 1 );
         }
         else if( ( 0 == VBATREG_SHORT_OUTAGE ) && ( 1 == PWRLG_OUTAGE() ) ) /* RFSYS Reg is valid */
         {
            if( 0 == VBATREG_POWER_RESTORATION_MET )
            {
               /* Outage declared, but restoration delay not met. Sleep for restoration delay. */
               PWRLG_LLWU_SET( 1 );       /* Re-enter last gasp on next reset. */
               PWRLG_STATE_SET( PWRLG_STATE_WAIT_RESTORATION );
               LG_PRNT_INFO( "PWRLG_Restoration: Wait for Restoration\n\r" );
               EnterVLLS( VBATREG_POWER_RESTORATION_TIMER, LPTMR_SECONDS, 1 );
            }
         }
         /* Continue processing the inits if Restoration delay has been met in PWRLG_Startup */
         PWRLG_LLWU_SET( 0 ); /* Do not enter last gasp on next reset. */
         PWRLG_STATE_SET( PWRLG_STATE_COMPLETE );
      }
      else
      {
         LG_PRNT_INFO( "PWRLG_Restoration: Skip Restoration: Not one of the reset causes\n\r" );
      }
   }
}

/***********************************************************************************************************************

   Function name: NextSleep

   Purpose: Compute the next sleep interval. If all messages have been sent, outage has been declared.:
               -  Write the power quality count to NV and wait stay in last gasp mode for power restoration time, or
                  until power restored.
               - Else, compute sleep time until next message.
            In finer detail:
               If the state is "wait for radio", a message has just been transmitted:
                  Bump the message number and set the state to "sleep".
               If all messages completed:
                  Write the power anomaly count to NV.
                  Set LP timer to outage restoration delay. Sleep until timer expires, or PF released. Note: this
                  preserves the super cap more than switching to the LDO and resetting the micro, due to leakage paths
                  in the A/D circuit when the processor's VDD drops too low.
               Else:
                  check for additional sleep time needed. (Times > 2 minutes are split into seconds and milliseconds).
                  If seconds remain:
                     program LP timer for seconds remaining and go to sleep.
                  Else if milliseconds remain:
                     program LP timer for milliseconds remaining and go to sleep.

                  Else
                     return
   Arguments: None

   Returns: void

   Re-entrant Code: No

   Notes:   This code is sometimes called from mqx_main, before the OS is running. So, in that case, no OS dependent
            routines may be used ( e.g., DBG_logPrintf, OS_TASK_Sleep, etc. ). If the state is "wait for radio", the OS
            is running.

 **********************************************************************************************************************/
static void NextSleep( void )
{
   uint16_t uCounter;

   LG_PRNT_INFO("\nNSleep()\n");
   if ( PWRLG_STATE_WAIT_FOR_RADIO == PWRLG_STATE() ) /* Indicates just transmitted a LG message.  */
   {
      PWRLG_MESSAGE_NUM_SET( PWRLG_MESSAGE_NUM() + 1 );
      PWRLG_STATE_SET( PWRLG_STATE_SLEEP );

      if ( PWRLG_MESSAGE_NUM() < PWRLG_MESSAGE_COUNT() )
      {
         PWRLG_CalculateSleep( PWRLG_MESSAGE_NUM() );
         if( ( PWRLG_SLEEP_SECONDS() == 0 ) && ( PWRLG_SLEEP_MILLISECONDS() == 0 ) )   /* Insufficient super cap energy left; sleep forever.  */
         {
            LG_PRNT_INFO("\nInsufficient Cap Energy!\n\r");
            PWRLG_LLWU_SET( 1 );    /*  Stay in last gasp mode at next reset. */
            PWRLG_STATE_SET( PWRLG_STATE_WAIT_RESTORATION );
            EnterVLLS( 0, LPTMR_SLEEP_FOREVER, 1 );
         }
      }
      else  /* Just sent last message, OS is running. Update power quality count in NV.   */
      {
         LG_PRNT_INFO("\nNS_NVWrite\n");
         pwrFileData_t  pwrFileData;
         FileHandle_t   fileHndlPowerDownCount; /* Contains the powerDown file handle information. */
         FileStatus_t   fileStatusCfg;          /* Contains the file status */
         ( void )FIO_fopen( &fileHndlPowerDownCount, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_PWR,
                              ( lCnt )sizeof( pwrFileData ), FILE_IS_NOT_CHECKSUMED, 0, &fileStatusCfg );
         /* Read the file. */
         ( void )FIO_fread( &fileHndlPowerDownCount, ( uint8_t* ) &pwrFileData, 0,
                              ( lCnt )sizeof( pwrFileData ) );
         /* Update the power anomaly count, if necessary.  */
         if( pwrFileData.uPowerAnomalyCount != VBATREG_PWR_QUAL_COUNT )
         {
            pwrFileData.uPowerAnomalyCount = VBATREG_PWR_QUAL_COUNT;
            // TODO: DG: Don't write to NV while on SC? Remove or test to ensure we have enough enrgy to perform write
            ( void )FIO_fwrite( &fileHndlPowerDownCount, 0, ( uint8_t* ) &pwrFileData,
                                 ( lCnt )sizeof( pwrFileData ) );
         }
      }
      /* Check to see if power restored during transmission of last gasp message. If so, set timer to very short
         duration so next reset handles this, rather than duplicate all of that logic here. */
      bool brnOut = ( bool )BRN_OUT();
#if ( MCU_SELECTED == NXP_K24 )  // TODO: RA6E1: DG: Add Support for RA6
      if ( !brnOut && powerStable( brnOut ) )
      {
         EnterVLLS( 1, LPTMR_MILLISECONDS, 1 );
      }
#endif
   }
   if ( PWRLG_MESSAGE_NUM() >= PWRLG_MESSAGE_COUNT() )   /* Sent all the messages planned?   */
   {
      /* Done. Sleep until PF signal released.  */
      LG_PRNT_INFO( "\nAll messages sent, wait for power to return\n\r" );
      PWRLG_LLWU_SET( 1 );    /*  Stay in last gasp mode at next reset. */
      PWRLG_STATE_SET( PWRLG_STATE_WAIT_RESTORATION );
      EnterVLLS( 0, LPTMR_SLEEP_FOREVER, 1 );
   }
   else
   {
      if ( PWRLG_SLEEP_SECONDS() != 0 )  /* If there are seconds left, next wakeup will not transmit.   */
      {
         LG_PRNT_INFO("\nTX_S\n");
         uCounter = PWRLG_SLEEP_SECONDS();
         PWRLG_SLEEP_SECONDS() = 0;
         EnterVLLS( uCounter, LPTMR_SECONDS, 1 );
      }
      else if ( PWRLG_SLEEP_MILLISECONDS() != 0 )
      {
         LG_PRNT_INFO("\nTX_MS\n");
         uCounter = PWRLG_SLEEP_MILLISECONDS();
         PWRLG_SLEEP_MILLISECONDS() = 0;
         PWRLG_STATE_SET( PWRLG_STATE_TRANSMIT );  /* Next wakeup should transmit. */
         VBATREG_CURR_TX_ATTEMPT = 0;              /* Reset the Tx attempts for this message */
         EnterVLLS( uCounter, LPTMR_MILLISECONDS, 1 );
      }
   }
   /* At this point, the initial outage declaration delay has just been met. Calling routine sets outage declared flag
      and computes time to sleep before first transmission. */
}
#if 1 //( MCU_SELECTED == NXP_K24 )
/***********************************************************************************************************************

   Function name: PWRLG_Begin

   Purpose: Called by PWR_task when the meter has signaled a loss of power. Sets up the needed information for
            last gasp and enters VLLS1 (very low leakage stop 1) for the first time.

   Arguments: sysTimeCombined_t timeCombinedOutage - Outage Time
              uint16_t anomalyCount - Anomaly Count

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWRLG_Begin( uint16_t anomalyCount )
{
   uint32_t             uFirstSleepMilliseconds;
   uint8_t              hwVerString[VER_HW_STR_LEN];
   sysTimeCombined_t    currentTime;
   sysTime_t            now;  /* Current system time. */
   sysTime_dateFormat_t dt;   /* Current system time, in human readable format. */
   char                 floatStr[PRINT_FLOAT_SIZE];
   float                Vcap;

#if ( DEBUG_PWRLG != 0 )
   uLLWU_F3    = 0;
   uLLWU_FILT1 = 0;
   _bsp_int_disable( INT_PORTD ); /* Disable the Port D interrupt from actually interrupting. */
#endif
#if ( MCU_SELECTED == NXP_K24 )
   LLWU_FILT1                       = LLWU_FILT1_FILTF_MASK;   /* Reset /PF interrupt flag, if any.   */
#endif
   VBATREG_PWR_QUAL_COUNT           = anomalyCount;            /* Save the current power quality (anomaly) count. */
   VBATREG_POWER_RESTORATION_MET    = 0;                       /* Reset the outage restoration delay met flag. */
   VBATREG_POWER_RESTORATION_TIMER  = PWRCFG_get_restoration_delay_seconds();  // TODO:DG: We can write to VBAT while initializing
   uFirstSleepMilliseconds          = ( ( uint32_t ) PWRCFG_get_outage_delay_seconds() ) * 1000;
   VBATREG_OUTAGE_DECLARATION_DELAY = uFirstSleepMilliseconds; /* Make a copy of this to be used while in last gasp mode.  */

   if ( PWRLG_TIME_OUTAGE() == 0 )
   {   // PQE is not currently in process, update the outage time
      ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );  // If this fails, currentTime is 0, considered invalid.
      PWR_setOutageTime( currentTime );
      PWRLG_TIME_OUTAGE_SET( ( uint32_t )( currentTime / TIME_TICKS_PER_SEC ) );
   }
   else
   {  // we are still in a current PQE state, acquire the current outage time stored in NV.
      currentTime = PWR_getOutageTime();
   }

   // Flush all partitions. (only cached partitions will actually flush)
   ( void )PAR_partitionFptr.parFlush( NULL );  /* TODO: This should be moved to prior to calling this function then make similar to PWR_SafeReset */

   // Display current recorded outage time on debug port
   TIME_UTIL_ConvertSysCombinedToSysFormat( &currentTime, &now );
   ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &now, &dt );
   DBG_LW_printf( "Outage time: %04u/%02u/%02u %02u:%02u:%02u\n", dt.year, dt.month, dt.day, dt.hour, dt.min,   dt.sec );

   // Save the MAC Configuration for later recovery
   MAC_SaveConfig_To_BkupRAM();

   PWRLG_MILLISECONDS() = uTotalMilliseconds - uFirstSleepMilliseconds;
   Vcap                 = ADC_Get_SC_Voltage();
   // Get HW Revision letter
   ( void )VER_getHardwareVersion ( &hwVerString[0], sizeof(hwVerString) );
   hwRevLetter_ = hwVerString[0];
   DBG_LW_printf( "Vcap: %-6s\n", DBG_printFloat( floatStr, Vcap, 3 ) );
   DBG_logPrintf( 'I', "HW Rev Letter: %c ", hwRevLetter_);
#if ( LG_WORST_CASE_TEST == 1 )
   DBG_LW_printf(" \n !!!! This is Last Gasp Worst Case Test Code( Revision: 12) !!!! \n");
#warning "Update the Revision"
#endif
   PWRLG_SetupLastGasp();
   DBG_LW_printf( "Sleep First: %d, Window 0 seconds: %d, milliseconds: %d\n",
                  uFirstSleepMilliseconds, PWRLG_SLEEP_SECONDS(), PWRLG_SLEEP_MILLISECONDS() );
   OS_TASK_Sleep( 10 );    // let print out finish
   PWRLG_FLAGS().byte = 0;
   HardwareShutdown();

   TOTAL_BACK_OFF_TIME_SET( 0 );
   CUR_BACK_OFF_TIME_SET( 0 );
   TX_FAILURES_SET( 0 );
   RESTORATION_TIME_SET( 0 );
   //TODO: DG: Don't execute PWRLG_Begin function if in Ship Mode or Meter Shop mode.
   VBATREG_SHORT_OUTAGE = 1;

#if ( DEBUG_PWRLG == 0 )
   PWR_USE_BOOST();  /* Use the Super Cap to run the system.   */
#endif
   if ( PWRLG_MESSAGE_COUNT() != 0 )  /* If there's enough energy to send any message... */
   {
      PWRLG_LLWU_SET( 1 );       /* Re-enter last gasp on next reset.   */
//    PWRLG_CAP_BEFORE() = ADC_Get_SC_Voltage();
      PWRLG_SOFTWARE_RESET_SET( 1 );   /* This bit is set so it can be determined if last gasp was terminated before or
                                          after all last gasp messages were sent. This bit will remain set if last gasp
                                          was terminated by power being restore before all last gasp messages were sent.
                                          */

      /* LP time cannot exceed 65535 */
      if ( uFirstSleepMilliseconds <= 60000 )
      {
         EnterVLLS( ( uint16_t )uFirstSleepMilliseconds, LPTMR_MILLISECONDS, 1 );   /* Goes into low power mode, never returns */
      }
      else
      {
         EnterVLLS( ( uint16_t )( uFirstSleepMilliseconds / 1000 ), LPTMR_SECONDS, 1 );   /* Goes into low power mode, never returns */
      }
   }
   else  /* There's not enough power to send even 1 last gasp message or last gasp messages are disabled. */
   {
      /* Last gasp messages are disabled if lastGaspMaxNumAttempts is set to zero */
      if ( 0 == PWRCFG_get_LastGaspMaxNumAttempts() )
      {
         LG_PRNT_INFO("\nLast Gasp messages are disabled \n\r");
      }
      else
      {
         LG_PRNT_INFO("\nNot enough Power \n\r");
      }
      PWRLG_OUTAGE_SET(1);                      /* Assume long outage, if we have a PQE without cap voltage, timestamps will be used to label it as such */
      VBATREG_SHORT_OUTAGE = 0;
      PWRLG_LLWU_SET( 1 );                      /* Re-enter last gasp on next reset.   */
      EnterVLLS( 0, LPTMR_SLEEP_FOREVER, 1 );   /* Goes into low power mode, never returns */
      RESET();                                  /* In case EnterVLLS() exits due to Power fail signal released. PWR_SafeReset() not necessary */
   }
}
#endif  //#if ( MCU_SELECTED == NXP_K24 )
/***********************************************************************************************************************

   Function name: HardwareShutdown

   Purpose: Disable all uneeded GPIO and other hardware before entering last gasp VLLS1(K24) or Deep Software Standby(RA6)

   Arguments: NONE

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
static void HardwareShutdown( void )
{
#if ( MCU_SELECTED == NXP_K24 )
#if ( DEBUG_PWRLG == 0 )
   /* GPIO shutdown masks. A '1' in this table causes the corresponding GPIO pin to be disabled.
                                                                       1  1  1  1  1  1  1  1  1  1  2  2  2  2  2  2  2  2  2  2  3  3
                                         0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9  0  1 */
   static const uint8_t uPTAMask[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
   static const uint8_t uPTBMask[32] = { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
   static const uint8_t uPTCMask[32] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#if ( PWRLG_PRINT_ENABLE == 0 )
   static const uint8_t uPTDMask[32] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#else
   /* Leave the debug UART port control (PORTD_PCR3) as is.  */
   static const uint8_t uPTDMask[32] = { 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#endif
#if ( ( ENABLE_LED == 0 ) && ( ENABLE_TRACE_PINS_LAST_GASP == 0 ) )
   static const uint8_t uPTEMask[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#endif
#if ( ENABLE_LED != 0 ) && ( ENABLE_TRACE_PINS_LAST_GASP == 0 )
   /* Leave the LED port controls (PORTE_PCR0-2) as is.  */
   static const uint8_t uPTEMask[32] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#endif
#if ( ENABLE_LED == 0 ) && ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
   /* Leave the TRACE_D0 & TRACE_D1 port controls ( PORTE_PCR4 & PORTE_PCR3 ) as is.  */
   static const uint8_t uPTEMask[32] = { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#endif
#if ( ( ENABLE_LED != 0 ) && ( ENABLE_TRACE_PINS_LAST_GASP != 0 ) )
   /* Leave the LED port controls (PORTE_PCR0-2) &  TRACE_D1 port controls ( PORTE_PCR4 & PORTE_PCR3 ) as is.  */
   static const uint8_t uPTEMask[32] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#endif
   uint8_t i;

   // Disable all GPIO pins except the ones masked out in the table above.
   for ( i = 0; i < 32; ++i )
   {
      if ( 1 == uPTAMask[i] )
      {
         PORTA_PCR( i ) = 0;
      }

      if ( 1 == uPTBMask[i] )
      {
         PORTB_PCR( i ) = 0;
      }

      if ( 1 == uPTCMask[i] )
      {
         PORTC_PCR( i ) = 0;
      }

      if ( 1 == uPTDMask[i] )
      {
         PORTD_PCR( i ) = 0;
      }

      if ( 1 == uPTEMask[i] )
      {
         PORTE_PCR( i ) = 0;
      }
   }
#endif
#elif ( MCU_SELECTED == RA6E1 )

   /* Set all the pins except PF_METER, LDO Enable,  to Input Mode before going to Deep Software Standby Mode */
   ( void )R_IOPORT_PinsCfg(&g_ioport_ctrl, &g_bsp_pin_cfg_lpm);
#if ( PWRLG_PRINT_ENABLE == 0 )
   /* TODO: RA6: DG: Configure the Boost Pin */
#else
   /* Enable the DBG UART pins for Printing */
   ( void )R_IOPORT_PinCfg(&g_ioport_ctrl, UART4_TX_DBG, ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_SCI0_2_4_6_8));
   ( void )R_IOPORT_PinCfg(&g_ioport_ctrl, UART4_RX_DBG, ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_SCI0_2_4_6_8));
#endif

#endif  //#if ( MCU_SELECTED == NXP_K24 )
}

/***********************************************************************************************************************

   Function name: PWRLG_LastGasp

   Purpose: Returns true if this is a boot up in Last Gasp mode otherwise returns false.

   Arguments: NONE

   Returns: uint8 true/false

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
uint8_t PWRLG_LastGasp( void )
{
   uint8_t retVal = false;

   /* Last Gasp or Restoration active only when MTU is not in Ship mode or Shop mode*/
   if ( ( 0 == VBATREG_SHIP_MODE ) && ( 0 == VBATREG_METER_SHOP_MODE ) )
   {
#if ( MCU_SELECTED == NXP_K24 )
#if ( DEBUG_PWRLG == 0)

      if ( ( ( RCM_SRS0 & RCM_SRS0_WAKEUP_MASK ) != 0 )  && /* Reset due to LLWU */
         (
            ( ( LLWU_F3 & LLWU_F3_MWUF0_MASK )  != 0 )  || /* LPTRM0 was the wakeup source  */
            ( 0 != PWRLG_LLWU() )                          /* LLWU cause set in LP RAM   */
         )
         )
#else
      if ( ( ( LLWU_F3 & LLWU_F3_MWUF0_MASK )  != 0 )  || /* LPTRM0 was the wakeup source  */
             ( 0 != PWRLG_LLWU() )                        /* LLWU cause set in LP RAM   */
      )
#endif
#elif ( MCU_SELECTED == RA6E1)
      /* TODO: RA6: DG: Do we need to further check the wake-up source? */
      if  ( R_SYSTEM->RSTSR0_b.DPSRSTF ) /* Deep Software Standby Reset Flag */
#endif
      {
         retVal = true;
      }
   }
   return ( retVal );
}


/***********************************************************************************************************************

   Function name: PWRLG_BSP_Setup

   Purpose: Enables the minimum GPIO when starting in last gasp

   Arguments: NONE

   Returns: None

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWRLG_BSP_Setup( void )
{
   if ( PWRLG_LastGasp() )
   {
#if ( MCU_SELECTED == NXP_K24 )
      // Set LDO, BOOST as outputs, disable LDO/enable BOOST
      // All other signals have required board or device pull up/down to keep in low power state
      SIM_SCGC5 |= ( uint16_t )( SIM_SCGC5_PORTA_MASK |
                                 SIM_SCGC5_PORTB_MASK |
                                 SIM_SCGC5_PORTC_MASK |
                                 SIM_SCGC5_PORTD_MASK |
                                 SIM_SCGC5_PORTE_MASK  );   /* Enable All Port clock gates */
#endif
#if ( DEBUG_PWRLG == 0)
      PWR_3P6LDO_EN_TRIS_OFF();
      // No need to delay, the outputs will already be this state before the ACKISO
      PWR_3V6BOOST_EN_TRIS_ON();
#else //if ( DEBUG_PWRLG == 1)

#if ( LG_TEST_PULLUP == 1 )
      BRN_OUT_PULLUP_ENABLE();      /* Enable PF signal pullup */
#else //PULL DOWN
      BRN_OUT_PULLDN_ENABLE();      /* Enable PF signal pull down */
#endif
      PWR_3P6LDO_EN_TRIS_ON();
      for ( uint32_t i = 0; i < 12000000; i++ );
      PWR_3V6BOOST_EN_TRIS_OFF();
#endif
#if ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
#if ( MCU_SELECTED == NXP_K24 )
      TRACE_D0_TRIS();
      TRACE_D1_TRIS();
      // Wake up from VLLS1 mode
      TRACE_D0_HIGH();
#else
#warning "NOT Supported on RA6"
#endif
#endif
   }
#if ( MCU_SELECTED == NXP_K24 )
   //Clear the Port Latches if coming up from a low power mode
   if ( RCM_SRS0 & RCM_SRS0_WAKEUP_MASK )
   {
      PMC_REGSC |= PMC_REGSC_ACKISO_MASK;
   }

   /* Enable the RTC Peripheral clock */
   SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;
   /* If the RTC OSC is NOT enabled */
   if ( ( RTC_CR & RTC_CR_OSCE_MASK ) == 0u )
   {
      /* Only if the OSCILLATOR is not already enabled */
      /* Disable all the OSC load capacitors */
      RTC_CR &= ( uint32_t )~( uint32_t )( RTC_CR_SC2P_MASK |
                                           RTC_CR_SC4P_MASK |
                                           RTC_CR_SC8P_MASK |
                                           RTC_CR_SC16P_MASK );
      /* Enable the OSC */
      RTC_CR |= RTC_CR_OSCE_MASK | RTC_CR_SCxP_DEFAULT;
      /* Enable the RTC Clock for other peripherals */
      RTC_CR &= ( uint32_t )~( uint32_t )( RTC_CR_CLKO_MASK );
   }
#elif ( MCU_SELECTED == RA6E1 )
   /* TODO: RA6: Any special handling required??? */
   /* TODO: RA6: Disable Unused Ports and Clocks ???? */
#endif
}

/***********************************************************************************************************************

   Function name: SetupLastGasp

   Purpose: Determines number of last gasp messages that can be sent and calculates the random spacing of the
            messages to send. This information is stored in the 32 byte system register file which persists
            through VLLS1 mode.

   Arguments: None

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWRLG_SetupLastGasp( void )
{
   PWRLG_MESSAGE_COUNT_SET( PWRLG_CalculateMessages() ); /* Calculate number of messages to send and save in LP RAM. */

   PWRLG_MESSAGE_NUM_SET( 0 );            /* Init number of messages sent. */
   VBATREG_CURR_TX_ATTEMPT = 0;           /* Init number of transmission attempts   */
#if ( LAST_GASP_SIMULATION == 1 )
   /* This is to differentiate function flow between LG Sim path and Real Last Gasp */
   if ( eSimLGDisabled == EVL_GetLGSimState() )
#endif
   {
      PWRLG_STATE_SET( PWRLG_STATE_SLEEP );  /* Set state to "sleep"  */
   }

   PWRLG_CalculateSleep( 0 );                   /* Calculate sleep time for window 0.  */
}

/***********************************************************************************************************************

   Function name: PWRLG_CalculateMessages

   Purpose: Calculate the number of last gasp messages to attempt based on the super cap voltage.

   Arguments: None

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
uint16_t PWRLG_CalculateMessages( void )
{
   float    fSuperCapEnergy;
   float    fSuperCapVoltage;
   float    fCapacitance;
   float    fMinimumRunVoltage;
   uint16_t uAttempts = 0;
   uint16_t uMaxAttempts;

#if ( LG_WORST_CASE_TEST == 1 )
   char     floatStr[PRINT_FLOAT_SIZE];
#endif
#if ( LAST_GASP_SIMULATION == 1 )
   /* This is to differentiate function flow between LG Sim path and Real Last Gasp */
   if ( eSimLGDisabled == EVL_GetLGSimState() )
   {
#endif
      uMaxAttempts = ( uint16_t ) PWRCFG_get_LastGaspMaxNumAttempts(); /* Get the Max number of LG attempts dynamically */
#if ( LAST_GASP_SIMULATION == 1 )
   }
   else
   {
      uMaxAttempts = ( uint16_t ) PWRCFG_get_SimulateLastGaspMaxNumAttempts(); /* Get the Max number of LG Sim attempts dynamically */
   }
#endif
   /* Change the fCapacitance and fMinimumRunVoltage as per HW revision */
   if ( 'C' == hwRevLetter_ )
   {
      fCapacitance         = fCapacitanceRevC;
      fMinimumRunVoltage   = fMinimumRunVoltageRevC;
   }
   else
   {
      fCapacitance         = fCapacitanceRevB;
      fMinimumRunVoltage   = fMinimumRunVoltageRevB;
   }

   fSuperCapVoltage = ADC_Get_SC_Voltage();
   if ( fSuperCapVoltage > fMinimumStartVoltage )
   {
      fSuperCapEnergy = ( float )( 0.5 * fCapacitance *
                                 ( ( fSuperCapVoltage * fSuperCapVoltage ) -
                                   ( fMinimumRunVoltage * fMinimumRunVoltage ) ) );
      uAttempts = ( uint16_t ) ( fSuperCapEnergy /
                                 ( fEnergyTransmit + fEnergyCollisionDetection + fEnergy30SecondSleep ) );

      if ( uAttempts > uMaxAttempts )
      {
         uAttempts = uMaxAttempts;
      }
#if ( LG_WORST_CASE_TEST == 1 )
      DBG_LW_printf( "CapVolt: %-6sV \n", DBG_printFloat( floatStr, fSuperCapVoltage, 3 ));
      DBG_LW_printf( "CapEnergy: %-6s\n", DBG_printFloat( floatStr, fSuperCapEnergy, 3 ) );
#endif
   }
   return( uAttempts );
}

/***********************************************************************************************************************

   Function name: PWRLG_CalculateSleep

   Purpose: Determines the number of seconds/milliseconds to sleep for the given last gasp step (message number).

   Arguments: step - Last gasp step.

   Returns: void

   Re-entrant Code: No

   Notes:   This code is called from mqx_main, before the OS is running. So, in that case, no OS dependent routines may
            be used ( e.g., DBG_logPrintf, OS_TASK_sleep, etc. )

 **********************************************************************************************************************/
void PWRLG_CalculateSleep( uint8_t step )
{
   uint32_t        halfSleepSecs;
   uint32_t        sleepMilliSecs;
   uint32_t        sleepSecs;
   uint32_t        messageWindow;

#if ( MCU_SELECTED == NXP_K24 )
   aclara_srand( RTC_TPR );   /* Use the RTC prescale register as a seed to the random function.   */
#elif ( MCU_SELECTED == RA6E1 )
   aclara_srand( R_RTC->R64CNT ); // TODO: RA6: DG: Review this
#endif
   switch( step )
   {
      case 0:  /* First window is a fixed time */
      {
         messageWindow = PWRLG_FIRST_MSG_WINDOW;
         break;
      }
      default: /* All others are dependent on remaining step count */
      {
         messageWindow = CalculateSleepWindow( PWRLG_MESSAGE_COUNT() - step );
         break;
      }
   }
   if ( messageWindow <= uMinSleepMilliseconds )
   {
      sleepMilliSecs = uMinSleepMilliseconds;
   }
   else
   {
#if ( DEBUG_PWRLG == 0 )
#if ( LG_QUICK_TEST == 0 )
#if ( LG_WORST_CASE_TEST == 0 )
      sleepMilliSecs = aclara_randu( uMinSleepMilliseconds, messageWindow );
#else
      sleepMilliSecs = messageWindow; /* Use the max time for the worst case LG test */
#endif
#else // ( LG_QUICK_TEST == 1 )
      //For quick testing - makes all windows the same as the outage declaration delay
      sleepMilliSecs = VBATREG_OUTAGE_DECLARATION_DELAY;
#endif
#else // for ( DEBUG_PWRLG == 0 )
      if ( step == 0 )
      {
         sleepMilliSecs = 1000;
      }
      else
      {
         sleepMilliSecs = aclara_randu( uMinSleepMilliseconds, messageWindow );
      }
#endif
   }
   PWRLG_MILLISECONDS() -= min( sleepMilliSecs, PWRLG_MILLISECONDS() );  /* Don't wrap under! */
   // If the sleep can done in a single sleep of milliseconds (limited by timers 16b counter).
   if ( sleepMilliSecs <= 60000u )
   {
      sleepSecs = 0;
   }
   else
   {
      sleepSecs       = sleepMilliSecs / 1000;
      sleepMilliSecs %= 1000;

      // Try to balance the sleep time between seconds and milliseconds.
      if ( sleepSecs > 120 )
      {
         // Move 60 seconds from s to ms
         sleepSecs      -= 60;
         sleepMilliSecs += 60000;
      }
      else
      {
         // Take half of the total sleep in seconds
         halfSleepSecs = sleepSecs / 2;
         if ( !( sleepSecs & 1 ) )  /* If sleepSecs is even.   */
         {
            --halfSleepSecs;
         }
         // And move to ms
         sleepMilliSecs += halfSleepSecs * 1000;
         sleepSecs      -= halfSleepSecs;
      }
   }
   LG_PRNT_INFO("\nCS\n\r");
   /* Adjust the millisecond counter to allow for the fact that the timing unit is really 1/1024 seconds, not 1/1000
      seconds.*/
#if ( DEBUG_PWRLG == 0 )
   sleepMilliSecs             = ( uint32_t ) ( ( ( double ) sleepMilliSecs ) * 1.024 );
   PWRLG_SLEEP_SECONDS()      = ( uint16_t ) sleepSecs;
   PWRLG_SLEEP_MILLISECONDS() = ( uint16_t ) sleepMilliSecs;
#else
   PWRLG_SLEEP_SECONDS()      = ( uint16_t ) 0;
   PWRLG_SLEEP_MILLISECONDS() = ( uint16_t ) 2000;
#endif
}

/***********************************************************************************************************************

   Function name: CalculateSleepWindow

   Purpose: Compute sleep time based on energy left in supercap
            Energy = 1/2 C * V^2.
               To calculate the energy remaining in the super cap, calculate Energy now and subtract E at Vminimum:
               Eremaining  = ( 0.5 * C* Vnow^2 ) - ( 0.5 * C * Vmin^2 )
                           = ( 0.5 * C ) * (Vnow^2 - Vmin^2)
            Sleep energy is supercap energy - ( no. msgs remaining * ( energy for collision detection + energy for tx )
               where energy values are constants.
            Sleep seconds remaining = ( Sleep energy / 0.08 ) * 30
            Sleep milliseconds remaining = sleep seconds remaining * 1000

   Arguments: uint8_t uMessagesRemaining - Number of messages remaining to be transmitted.

   Returns: Amount of time this message has for random wake up

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
static uint32_t CalculateSleepWindow( uint8_t uMessagesRemaining )
{
   float    fSleepSecondsRemaining;
   float    fSuperCapEnergy;
   float    fSuperCapEnergyForSleep;
   float    fSuperCapVoltage;
   float    fCapacitance;
   float    fMinimumRunVoltage;
   uint32_t uSleepMillisecondsRemaining;
   uint32_t uSleepWindow;

   /* Change the fCapacitance and fMinimumRunVoltage as per HW revision */
   if ( 'C' == hwRevLetter_ )
   {
      LG_PRNT_INFO("\nCSW(): HW_REV: C\n\r");
      fCapacitance         = fCapacitanceRevC;
      fMinimumRunVoltage   = fMinimumRunVoltageRevC;
   }
   else /* if ( 'B' == hwRevLetter_ )*/
   {
      LG_PRNT_INFO("\nCSW():HW_REV: B\n\r");
      fCapacitance         = fCapacitanceRevB;
      fMinimumRunVoltage   = fMinimumRunVoltageRevB;
   }

   fSuperCapVoltage = ADC_Get_SC_Voltage();
   // Ecap = 1/2C(Vnow^2-Vmin^2)
   fSuperCapEnergy = ( float )( 0.5 * fCapacitance * ( ( fSuperCapVoltage * fSuperCapVoltage ) - ( fMinimumRunVoltage *
                                fMinimumRunVoltage ) ) );
   // Esleep = Ecap - NumMsgs(Ecoll + Etx)
   fSuperCapEnergyForSleep = fSuperCapEnergy - ( uMessagesRemaining * ( fEnergyCollisionDetection + fEnergyTransmit ) );
   // Tremain = 30sec(Esleep/E30secSleep)
   fSleepSecondsRemaining      = ( float )( 30.0 * fSuperCapEnergyForSleep / fEnergy30SecondSleep );
   if ( fSleepSecondsRemaining < 0 )
   {
      fSleepSecondsRemaining = 0;
   }
   uSleepMillisecondsRemaining = ( uint32_t )fSleepSecondsRemaining * ( uint32_t )1000;

   if ( uSleepMillisecondsRemaining >= PWRLG_MILLISECONDS() )
   {
      // More available energy than time remaining so limit to time remaining
      uSleepMillisecondsRemaining = PWRLG_MILLISECONDS();
   }
   else
   {
      // Less available energy than time remaining so limit to time for available energy
      PWRLG_MILLISECONDS() = uSleepMillisecondsRemaining;
   }
   uSleepWindow = uSleepMillisecondsRemaining / ( ( uint32_t ) uMessagesRemaining );
   LG_PRNT_INFO("\nSleepWindow_MS:%d\n\r",uSleepWindow);
   return( uSleepWindow );
}

#if ( MCU_SELECTED == NXP_K24 )
/***********************************************************************************************************************

   Function name: Configure_LLWU

   Purpose:

   Arguments:

   Returns: void

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
static void Configure_LLWU( void )
{
   /* Disable all external pins as wakeup sources  */
   LLWU_PE1 = 0;
   LLWU_PE2 = 0;
   LLWU_PE3 = 0;
   LLWU_PE4 = 0;
   LLWU_ME  = 0;
   /* Reset all wake up source indicator bits by writing 1's   */
   LLWU_F1    = 0xFF;
   LLWU_F2    = 0xFF;
   LLWU_F3    = 0xFF;
   LLWU_FILT2 = LLWU_FILT2_FILTF_MASK;   /* Reset interrupt flag.   */
   /* Enable Digital Filter 1 feature for pin 12  */
   LLWU_FILT1 = ( ( LLWU_FILT1_FILTE( 3 ) ) |    /* Either edge */
                    LLWU_FILT1_FILTSEL( 12 ) );  /* LLWU_P12 ->PTD0 (/PF_METER) */
}

/***********************************************************************************************************************

   Function name: EnterVLLS

   Purpose: Setup the LPTMR (lower power time) and enters VLLS1 (very low leakage stop 1) mode.

   Arguments:  uint16_t uCounter - Counter value for LPTMR.
               PWRLG_LPTMR_Units eUnits - LPTMR_SECONDS, LPTMR_MILLISECONDS, LPTMR_SLEEP_FOREVER
               uint8_t uMode - The VLLSx mode to put the processor in

   Returns: void

   Re-entrant Code: No

   Notes:   If LPTMR_SLEEP_FOREVER specified, PF change still causes wakeup.
            If counter = 0, resume with existing LPTMR count.

 **********************************************************************************************************************/
static void EnterVLLS( uint16_t uCounter, PWRLG_LPTMR_Units eUnits, uint8_t uMode )
{
#if ( DEBUG_PWRLG == 0 )
   uint32_t scr;
#endif
   LG_PRNT_INFO( "\nEnter VLLs, count: %d, units: %d, mode: %d\n", uCounter, ( uint8_t )eUnits, uMode );
   LG_PRNT_INFO("\n HW Rev Letter: %c \n\r", hwRevLetter_);
#if ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
   TRACE_D0_LOW();
#endif
   HardwareShutdown();

#if ( DEBUG_PWRLG == 0 )
   // Clear SLEEPDEEP
   scr     = SCB_SCR;
   scr    &= ~( SCB_SCR_SLEEPDEEP_MASK | SCB_SCR_SLEEPONEXIT_MASK );
   SCB_SCR = scr;
#endif

   Configure_LLWU();

#if ( DEBUG_PWRLG == 0 )
   if ( LPTMR_SLEEP_FOREVER == eUnits )
   {
      /* If HW Revision C, keep the boost ON and the processor in the sleep state.
      The remaining power in the Super Cap shall be utilized to keep the RTC running */
      if ( 'C' == hwRevLetter_ )
      {
         LG_PRNT_INFO("EnterVLLS: Boost kept ON to keep the RTC running\n\r");
      }
      else
      {
         /* TODO: RA6: DG: Add conditional compilation for RA6 to eliminate this code */
         LG_PRNT_INFO("EnterVLLS: Turning off the Boost \n\r");
         /* Turning off the Boost. LDO is already turned off. */
         PWR_3V6BOOST_EN_TRIS_OFF();
         /* Already switched off both LDO and Boost. It is very less likely to come to execute this case.
         So, run forever. */
         for(;;)
            ;
      }
   }
#endif

   /* Per the Kinetis data sheet, wait at least 5 LPO clock cycles (1kHz) before entering VLLSx mode to allow filter to
      initialize; use 10 for margin. Without this delay, the PF change interrupt false triggers immmediately upon
      entering VLLS1 mode. */
   LptmrStart( 10, LPTMR_MILLISECONDS, LPTMR_MODE_WAIT );

   if ( uCounter != 0 ) /* Not "resuming", set actual delay.   */
   {
      LptmrStart( uCounter, eUnits, LPTMR_MODE_PROGRAM_ONLY );
   }
   else  /* Restart delay. */
   {
      if ( PWRLG_SLEEP_SECONDS() != 0 )   /* Using seconds? */
      {
         LptmrStart( PWRLG_SLEEP_SECONDS(), LPTMR_SECONDS, LPTMR_MODE_PROGRAM_ONLY );
      }
      else  /* Using milliseconds   */
      {
         LptmrStart( max( PWRLG_SLEEP_MILLISECONDS(), 1 ), LPTMR_MILLISECONDS, LPTMR_MODE_PROGRAM_ONLY );
      }
   }

   // Enable LPTMR wakeup if not sleeping forever.
   if ( LPTMR_SLEEP_FOREVER != eUnits )
   {
      LLWU_ME    = ( uint8_t )LLWU_ME_WUME0_MASK;    /* Wake up source Module 0 is LPTMR */
   }
#if 0
/* TODO: mkv this section of code only deals with the processor's reset pin. Since it is not connected to anything but a
   pullup, is it necessary?   */
   /* Enable digital filter and reset pin as means to exit low leakage mode   */
   LLWU_RST = (uint8_t)( LLWU_RST_RSTFILT_MASK | LLWU_RST_LLRSTE_MASK );
/* end TODO */
#endif

   /* Disable CME if enabled before changing Power mode */
   MCG_C6 &= ~( MCG_C6_CME0_MASK );            /* Clear CME */

#if ( DEBUG_PWRLG == 0 )
   // Enable VLLS1 mode.
   SMC_PMCTRL   = SMC_PMCTRL_STOPM( 4 ) | SMC_PMCTRL_LPWUI_MASK; /* LP wake up enabled and VLLSx selected  */

#if ( MQX_CPU == PSP_CPU_MK22FN512 )
   SMC_STOPCTRL = SMC_STOPCTRL_LLSM( uMode );   /* VLLS set to uMode (currently always 1) */
#else
   SMC_VLLSCTRL = SMC_VLLSCTRL_VLLSM( uMode );  /* VLLS set to uMode (currently always 1) */
#endif
#endif

#if 0
   /* ENGR00178898 workaround  Shut down SPI0, SPI1 peripheral. Preventing entering stop mode for some reason */
#ifdef SIM_SCGC6_SPI0_MASK
   SIM_SCGC6 &= ~SIM_SCGC6_SPI0_MASK;
#endif
#ifdef SIM_SCGC6_SPI1_MASK
   SIM_SCGC6 &= ~SIM_SCGC6_SPI1_MASK;
#endif
   _ASM_SLEEP( NULL );
#ifdef SIM_SCGC6_SPI0_MASK
   SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK;
#endif
#ifdef SIM_SCGC6_SPI1_MASK
   SIM_SCGC6 |= SIM_SCGC6_SPI1_MASK;
#endif
#endif

#if ( DEBUG_PWRLG == 0 )
   /* Set SLEEPDEEP. This register is documented in the ARM Core manual */
   /* The bit indicates that the wakeup time from the suspended execution can be longer than if the bit is not set.
      Typically this can be used to determine whether a PLL or other clock generator can be suspended.*/
   scr     = SCB_SCR;
   scr    |= SCB_SCR_SLEEPDEEP_MASK | SCB_SCR_SLEEPONEXIT_MASK;
   SCB_SCR = scr;
   /* OS_TASK_Sleep Does not work after this */
#endif

   if ( LPTMR_SLEEP_FOREVER != eUnits )
   {
#if ( DEBUG_PWRLG == 0 )
      // Enable the LPTMR and the LPTMR interrupt.
      LptmrEnable( ( bool )true );

#else /* For debugger, have to simulate the timer expiration and WFI */
      LptmrEnable( ( bool )false );
      do
      {
         __asm( "cpsid i" );
         WDOG_REFRESH = 0xA602;  /* CLRWDT() faults when debugging! */
         WDOG_REFRESH = 0xB480;
         __asm( "cpsie i" );
         uLLWU_F3 = ( LPTMR0_CSR & LPTMR_CSR_TCF_MASK ); /* Capture the time out indicator.  */
      } while ( ( uLLWU_F3 == 0 ) && ( BRN_OUT_ISF() == 0 ) );
      if ( uLLWU_F3 != 0 )
      {
         uLLWU_F3 = LLWU_F3_MWUF0_MASK;   /* SetTimer expired.  */
         uLLWU_FILT1 = 0;                 /* Set PF didn't change state. */
      }
      else
      {
         uLLWU_FILT1 = LLWU_FILT1_FILTF_MASK;   /* Set PF changed state. */
         BRN_OUT_CLR_IF();                      /* Clear the interrupt. */
      }
      RESET(); /* PWR_SafeReset() not necessary */
#endif

   }

#if ( DEBUG_PWRLG != 0 )   /* For debugging, add the sleep forever code.   */
   else
   {
      do
      {
         __asm( "cpsid i" );
         WDOG_REFRESH = 0xA602;  /* CLRWDT() faults when debugging! */
         WDOG_REFRESH = 0xB480;
         __asm( "cpsie i" );
      } while ( BRN_OUT_ISF() == 0 );  /* Only exit on /PF signal change.  */
      uLLWU_F3 = 0;                          /* Timer did not expire. */
      uLLWU_FILT1 =  LLWU_FILT1_FILTF_MASK;  /* PF changed state. */
      RESET(); /* PWR_SafeReset() not necessary */
   }
#endif

#if ( DEBUG_PWRLG == 0 )
   asm( "WFI" );  /* Sleep until a qualified interrupt occurs. */
#endif
}

/***********************************************************************************************************************

   Function name: EnterLLS

   Purpose: Setup the LPTMR (lower power time) and enters LLS (low leakage stop) mode.

   Arguments:  uint16_t uCounter - Counter value for LPTMR.
               PWRLG_LPTMR_Units eUnits - LPTMR_SECONDS, LPTMR_MILLISECONDS, LPTMR_SLEEP_FOREVER
               uint8_t uMode - Not used

   Returns: void

   Re-entrant Code: No

   Notes:
 **********************************************************************************************************************/
static void EnterLLS( uint16_t uCounter, PWRLG_LPTMR_Units eUnits )
{
   volatile uint32_t scr;
   static   uint8_t mcg,cme;
   /* Mode is not used */

   DBG_logPrintf('I',"Enter LLs, count: %d, units: %d\n", uCounter, ( uint8_t )eUnits );
#if ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
   TRACE_D0_LOW();
#endif
    // Clear SLEEPDEEP
   scr     = SCB_SCR;
   scr    &= ~( SCB_SCR_SLEEPDEEP_MASK | SCB_SCR_SLEEPONEXIT_MASK );
   SCB_SCR = scr;

   Configure_LLWU();

   LptmrStart( uCounter, eUnits, LPTMR_MODE_PROGRAM_ONLY );

   // Enable LPTMR wakeup
   LLWU_ME    = ( uint8_t )LLWU_ME_WUME0_MASK;    /* Wake up source Module 0 is LPTMR */

   /* Enable digital filter and reset pin as means to exit low leakage mode   */
   LLWU_RST = (uint8_t)( LLWU_RST_RSTFILT_MASK | LLWU_RST_LLRSTE_MASK );

   /* Keep status of MCG before mode change */
   mcg = MCG_S & MCG_S_CLKST_MASK;

   /* Disable CME if enabled before entering changing Power mode */
   cme = MCG_C6 & MCG_C6_CME0_MASK;            /* Save CME state */
   MCG_C6 &= ~(MCG_C6_CME0_MASK);              /* Clear CME */
   /* Disable CME if enabled before changing Power mode */
   MCG_C6 &= ~( MCG_C6_CME0_MASK );            /* Clear CME */

   SMC_PMCTRL &= ~SMC_PMCTRL_STOPM_MASK ;
   SMC_PMCTRL = SMC_PMCTRL_STOPM( 0x3 ) | SMC_PMCTRL_LPWUI_MASK; // (MC2) Set STOPM = 0b11 & enable the LP Wake up interrupt

   /* wait for write to complete to SMC before stopping core */
   scr = SMC_PMCTRL;
   /* Set the SLEEPDEEP bit to enable deep sleep mode */
   SCB_SCR |= SCB_SCR_SLEEPDEEP_MASK;
   // Enable the LPTMR and the LPTMR interrupt.
   LptmrEnable( ( bool )true );

   /* ENGR00178898 workaround  Shut down SPI0, SPI1 peripheral. Preventing entering stop mode for some reason */
#ifdef SIM_SCGC6_SPI0_MASK
   SIM_SCGC6 &= ~SIM_SCGC6_SPI0_MASK;
#endif
#ifdef SIM_SCGC6_SPI1_MASK
   SIM_SCGC6 &= ~SIM_SCGC6_SPI1_MASK;
#endif
   /* WFI instruction will start entry into low-power mode */
   _ASM_SLEEP( NULL );  //asm("WFI");

#ifdef SIM_SCGC6_SPI0_MASK
   SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK;
#endif
#ifdef SIM_SCGC6_SPI1_MASK
   SIM_SCGC6 |= SIM_SCGC6_SPI1_MASK;
#endif

 /* After returning from LLS mode, reconfigure MCG */
   if ( ( MCG_S_CLKST(3) == mcg ) && ( MCG_S_CLKST(2) == (MCG_S & MCG_S_CLKST_MASK) ) )
   {
      MCG_C1 &= (~ ( MCG_C1_CLKS_MASK | MCG_C1_IREFS_MASK ) );
      while ( 0 == ( MCG_S & MCG_S_LOCK0_MASK ) )
      { };
   }
   /* Restore CME */
   MCG_C6 |= cme; /*lint !e734 */

#if ( ENABLE_TRACE_PINS_LAST_GASP != 0 )
   /* LLS to Run  */
   TRACE_D0_HIGH();
#endif

}

/***********************************************************************************************************************

   Function name: LptmrStart

   Purpose: Setup and if specified, start the LPTMR (lower power time).

   Arguments:  uint16_t uCounter - Counter value for LPTMR.
               PWRLG_LPTMR_Units eUnits - LPTMR_SECONDS, LPTMR_MILLISECONDS, LPTMR_SLEEP_FOREVER
               PWRLG_LPTMR_Mode eMode - LPTMR_MODE_PROGRAM_ONLY, LPTMR_MODE_START, LPTMR_MODE_WAIT, LPTMR_MODE_ENBLE_INT

   Returns: void

   Re-entrant Code: No

   Notes:
            If the counter value is 0, continue with existing count. If the existing count is 0, set to 1.

            This code is called from mqx_main, before the OS is running. So, in that case, no OS dependent routines may
            be used ( e.g., DBG_logPrintf, OS_TASK_sleep, etc. )

 **********************************************************************************************************************/
static void LptmrStart( uint16_t uCounter, PWRLG_LPTMR_Units eUnits, PWRLG_LPTMR_Mode eMode )
{
   // Select RTC 32.768kHz oscillator as source for LPTMR.
   SIM_SOPT1 = ( SIM_SOPT1 & ~SIM_SOPT1_OSC32KSEL_MASK ) | SIM_SOPT1_OSC32KSEL( 2 );

   SIM_SCGC5 |= SIM_SCGC5_LPTIMER_MASK;

   /* Setup the Low Power Timer (LPTMR)
      Clear Timer Compare Flag and disable LPTMR for programming. */
   LPTMR0_CSR = LPTMR_CSR_TCF_MASK;

   if ( uCounter != 0 ) /* If 0 time requested, resume with existing settings.   */
   {
      if ( LPTMR_SECONDS == eUnits )
      {
         /* Select ERCLK32K and prescale 32K for one second per tick timer. Prescaler is 2^(n+1).
            To achieve 32768, n = 14. LPTMR_PSR_PCS( 2 ) selects the external 32KHz oscillator as the source. */
         LPTMR0_PSR = LPTMR_PSR_PRESCALE( 14 ) | LPTMR_PSR_PCS( 2u );
      }
      else
      {
         /* Select ERCLK32K and prescale 32 for millisecond timer.
            Again, prescaler = 2^(n+1), so for divide by 32, n = 4. Note, this results in 1024Hz counter. */
         LPTMR0_PSR = LPTMR_PSR_PRESCALE( 4u ) | LPTMR_PSR_PCS( 2u );
      }

      // Set the time out value.
      LPTMR0_CMR = uCounter;
   }
   else  if ( LPTMR0_CMR == 0 )  /* Make sure that there is a non-zero value in the compare register! */
   {
      LPTMR0_CMR = 1;
   }

   if ( LPTMR_MODE_PROGRAM_ONLY != eMode )
   {
      if ( LPTMR_MODE_ENBLE_INT == eMode )
      {
         // Enable the LPTMR and the LPTMR interrupt.
         LptmrEnable( ( bool )true );
      }
      else
      {
         // Enable the LPTMR.
         LptmrEnable( ( bool )false );

         if ( LPTMR_MODE_WAIT == eMode )
         {
            while ( 0 == ( LPTMR0_CSR & LPTMR_CSR_TCF_MASK ) ) /* Wait for Timer Compare Flag - time out */
            {
#if ( DEBUG_PWRLG != 0 )
               __asm( "cpsid i" );
               WDOG_REFRESH = 0xA602;  /* CLRWDT() faults when debugging! */
               WDOG_REFRESH = 0xB480;
               __asm( "cpsie i" );
#endif
            }

            LPTMR0_CSR = LPTMR_CSR_TCF_MASK;   /* Disable the LP timer.   */
         }
      }
   }
}

/***********************************************************************************************************************

   Function name: LptmrEnable

   Purpose: Start the LPTMR (lower power time) and enable its interrupt is specified.

   Arguments: bool bEnableInterrupt

   Returns: void

   Re-entrant Code: No

   Notes:   This code is called from mqx_main, before the OS is running. So, in that case, no OS dependent routines may
            be used ( e.g., DBG_logPrintf, OS_TASK_sleep, etc. )

 **********************************************************************************************************************/
static void LptmrEnable( bool bEnableInterrupt )
{
   if ( bEnableInterrupt )
   {
      // Enable the LPTMR and the LPTMR interrupt.
      LPTMR0_CSR = LPTMR_CSR_TIE_MASK | LPTMR_CSR_TCF_MASK | LPTMR_CSR_TEN_MASK;
   }
   else
   {
      // Enable the LPTMR.
      LPTMR0_CSR = LPTMR_CSR_TCF_MASK | LPTMR_CSR_TEN_MASK;
   }
}
#endif // #if ( MCU_SELECTED == NXP_K24 )

/***********************************************************************************************************************

   Function name: PWRLG_MacConfigHandle()

   Purpose: This function is used to get the handle where the mac configuration is persisted.

   Arguments: None

   Returns: pointer to mac_config_t  This is in Vbat?

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
mac_config_t * PWRLG_MacConfigHandle( void )
{
   return( &VBATREG_RFSYS_BASE_PTR->mac_config );
}


#if 0 // DG: Not used
/***********************************************************************************************************************

   Function name: PWRLG_RandomRange()

   Purpose:

   Arguments: None

   Returns:

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
uint32_t PWRLG_RandomRange( uint32_t uLow, uint32_t uHigh )
{
   double dRandomValue;
   static const double dRandomMax = ( ( double ) ( ( uint32_t ) RAND_MAX ) ) + 1.0;

   dRandomValue = ( ( double ) rand() ) / dRandomMax;
   dRandomValue *= ( double ) ( uHigh - uLow );
   dRandomValue += ( double ) uLow;

   return( ( uint32_t ) dRandomValue );
}
#endif

#if ( MCU_SELECTED == NXP_K24 )  /* TODO: RA6E1: Add support for RA6 */
/***********************************************************************************************************************

   Function name: powerStable

   Purpose: debounce power fail change signal

   Arguments: bool curState

   Returns: true if power stable for PWR_STABLE_MILLISECONDS ( either state ).

   Re-entrant Code: Yes

   Notes:

 **********************************************************************************************************************/
static bool powerStable( bool curState )
{
   uint32_t powerGoodCount = 0;
   do
   {
      LptmrStart( 1, LPTMR_MILLISECONDS, LPTMR_MODE_WAIT );   /* Delay one millisecond.  */
      if ( curState == ( bool )BRN_OUT() )   /* PF signal unchanged? */
      {
         ++powerGoodCount;                   /* Continue testing PF signal.   */
      }
      else
      {
         /* Delay for the balance of the PWR_STABLE_MILLISECONDS */
         LptmrStart( ( uint16_t )( ( PWR_STABLE_MILLISECONDS + 1 ) - powerGoodCount ),
                     LPTMR_MILLISECONDS, LPTMR_MODE_WAIT );
         powerGoodCount = 0;        /* PF signal changed. Exit.   */
         break;
      }
   } while ( powerGoodCount < PWR_STABLE_MILLISECONDS );
   return ( powerGoodCount != 0 );
}
#endif
/***********************************************************************************************************************

   Function name: PWRLG_RestLastGaspFlags

   Purpose: Initializes the last gasp module for the next outage condition

   Arguments: none

   Returns: none

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
void PWRLG_RestLastGaspFlags( void )
{
   VBATREG_EnableRegisterAccess();
   PWRLG_OUTAGE_SET( 0 );        // Clear the "sustained" outage flag.
   VBATREG_SHORT_OUTAGE = 0;     // Clear the "short" outage flag.
   VBATREG_PWR_QUAL_COUNT = 0;   // Clear the RAM based power quality count.
   PWRLG_TIME_OUTAGE_SET( 0 );   // Clear the time of the last outage.
   RESTORATION_TIME_SET( 0 );    // Clear the Restoration time.
   VBATREG_DisableRegisterAccess();
}

#if ( MCU_SELECTED == RA6E1 )
/***********************************************************************************************************************

   Function name: PWRLG_GetSleepCancelSource

   Purpose: Identify and return the Deep Software Standby Cancel Source

   Arguments: none

   Returns: cancel Source

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
uint8_t PWRLG_GetSleepCancelSource( void )
{
   uint8_t cancelSoruce = 0;

   /* TODO: RA6: Create an Enum for the Cancel Sources */
   /* TODO : RA6: DG: Review Figure 5.4 Pg. No 109 of the RA6E1 HRM */
   if( R_SYSTEM->DPSIFR2_b.DLVD1IF )
   {
      cancelSoruce = 1;
      DBG_printf("LVD1 Deep Standby Cancel Flag");
   }
   else if( R_SYSTEM->DPSIFR2_b.DRTCAIF )
   {
      cancelSoruce = 2;
      DBG_printf("RTC Alarm Interrupt");
   }
   /* TODO: RA6 Select & add appropriate reg for Power Fail signal */
   else if( R_SYSTEM->DPSIFR1_b.DIRQ11F ) /* IRQ11-DS - Change in PF_METER */
   {
      cancelSoruce = 3;
      DBG_printf("IRQ5-DS Pin Edge Select");
   }

   return (cancelSoruce);

}

/***********************************************************************************************************************

   Function name: LptmrStart

   Purpose: Setup and if specified, start the LPTMR (lower power time).

   Arguments:  uint16_t uCounter - Counter value for LPTMR.
               PWRLG_LPTMR_Units eUnits - LPTMR_SECONDS, LPTMR_MILLISECONDS, LPTMR_SLEEP_FOREVER
               PWRLG_LPTMR_Mode eMode - LPTMR_MODE_PROGRAM_ONLY, LPTMR_MODE_START, LPTMR_MODE_WAIT, LPTMR_MODE_ENBLE_INT

   Returns: void

   Re-entrant Code: No

   Notes:
            If the counter value is 0, continue with existing count. If the existing count is 0, set to 1.

            This code is called from mqx_main, before the OS is running. So, in that case, no OS dependent routines may
            be used ( e.g., DBG_logPrintf, OS_TASK_sleep, etc. )

 **********************************************************************************************************************/
static void LptmrStart( uint16_t uCounter, PWRLG_LPTMR_Units eUnits, PWRLG_LPTMR_Mode eMode )
{
   /* Note: In RA6E1, RTC Alarm is used for LPTMR_SECONDS and Deep Software Standby Mode
           In RA6E1, AGT Alarm is used for LPTMR_MILLISECONDS and Software Standby Mode */
   (void)eMode; /* Not used for this function */

   switch( eUnits )
   {
      case LPTMR_SECONDS:
      {
         RTC_ConfigureRTCCalendarAlarm(uCounter);
         break;
      }
      case LPTMR_MILLISECONDS:
      {
         AGT_LPM_Timer_Configure(uCounter);
         break;
      }
      case LPTMR_SLEEP_FOREVER:
      {
         /* No need to set the timer */
         // TODO: RA6: Review: 1. Disable RTC ALARM ? OR Set Timer to max value? OR Reconfigure Deep SW Standby?
         break;
      }
      default:
      {
         break;
      }
   }
}

#endif // #if ( MCU_SELECTED == RA6E1 )
