/***********************************************************************************************************************

   Filename: hmc_diags.c

   Global Designator: HMC_DIAGS_

   Contents:   Read Diagnostics from the meter.  This module does not have any application level retries.  However, if
               diagnostics fail to read x times in a row, then an alarm will be generated.

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

   $Log$

   Created 03/5/13     KAD  Created

 ***********************************************************************************************************************
 ***********************************************************************************************************************
   Revision History:
   v1.0 - Initial Release
 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#define HMC_DIAGS_GLOBAL
#include "hmc_diags.h"
#undef HMC_DIAGS_GLOBAL

#include "ansi_tables.h"
#include "time_sys.h"
#include "alarm_cfg.h"
#include "DBG_SerialDebug.h"
#include "alarm_task.h"
#include "ALRM_Handler.h"
#include "EVL_event_log.h"
#include "intf_cim_cmd.h"

/* ****************************************************************************************************************** */
/* #DEFINE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define DIAG_RETRIES                ((uint8_t)0)                 /* Application level retries */
#define DIAG_FAILURES_BEFORE_ALARM  ((uint8_t)3)                 /* Number of failures before sending an alarm. */
#define DIAG_ALARM_PERIOD_MIN       ((uint8_t)5)                 /* Called every 5 minutes */

#if ENABLE_PRNT_HMC_DIAGS_INFO
#define HMC_DIAGS_PRNT_INFO(a,fmt,...)         DBG_logPrintf(a,fmt,##__VA_ARGS__)
#else
#define HMC_DIAGS_PRNT_INFO(a,fmt,...)
#endif

#if ENABLE_PRNT_HMC_DIAGS_WARN
#define HMC_DIAGS_PRNT_WARN(a,fmt,...)         DBG_logPrintf(a,fmt,##__VA_ARGS__)
#else
#define HMC_DIAGS_PRNT_WARN(a,fmt,...)
#endif

#if ENABLE_PRNT_HMC_DIAGS_ERROR
#define HMC_DIAGS_PRNT_ERROR(a,fmt,...)        DBG_logPrintf(a,fmt,##__VA_ARGS__)
#else
#define HMC_DIAGS_PRNT_ERROR(a,fmt,...)
#endif

#if( RTOS_SELECTION == FREE_RTOS )
#define HMC_DIAG_QUEUE_SIZE 10 //NRJ: TODO Figure out sizing
#else
#define HMC_DIAG_QUEUE_SIZE 0
#endif



/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static   stdTbl3_t      mtrDiags_;                                   /* Standard Table 3 - Diags */
static   FileHandle_t   diagFileHndl_ = {0};                         /* Contains the file handle information. */
static   OS_QUEUE_Obj   hmcDiagQueueHandle_;                         /* Used for alarm as a trigger to update. */
static   bool           bModuleInitialized_ = false;                 /* true = Initialized, false = uninitialized. */
static   uint8_t        retries_ = DIAG_RETRIES;                     /* Used for application level retries */
static   bool           bReadFromMeter_ = false;                     /* true: Read from meter, false: Do nothing. */
static   uint8_t        alarmId_ = 0;                                /* Alarm ID */
static   uint8_t        failCntDwn_ = DIAG_FAILURES_BEFORE_ALARM;    /* After x failures, alarm msg will be sent */

/* ****************************************************************************************************************** */
/* CONSTANTS */
static const tReadPartial tblMtrDiags_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT( STD_TBL_END_DEVICE_MODE_STATUS ),  /* ST3 holds the diag info */ /*lint !e572 !e778 */
   { ( uint8_t )0, ( uint8_t )0, ( uint8_t )0 },        /* Offset for diag in ST.Tbl 3   */ /*lint !e651 */
   BIG_ENDIAN_16BIT( 0x0008 )                           /* Length for diag in ST.Tbl 3   */ /*lint !e572 !e778 */
};

static const HMC_PROTO_TableCmd cmdMtrDiags_[] =
{
   {HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblMtrDiags_ ), ( uint8_t far * )&tblMtrDiags_[0] },
   {
      ( enum HMC_PROTO_MEM_TYPE )( ( uint16_t )HMC_PROTO_MEM_RAM | ( uint16_t )HMC_PROTO_MEM_WRITE ),
      sizeof( mtrDiags_ ), ( uint8_t far * )&mtrDiags_
   },
   {HMC_PROTO_MEM_NULL }
};

static const HMC_PROTO_Table tblsMtrDiags_[] =
{
   { &cmdMtrDiags_[0] },
   { NULL }
};

/* Standard table 3 diagnositics reported. Each entry consumes 1 bit in file. */
static const alarmInfo_t diagAlarmIds[] =
{
   /* Bit 0 */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Metering */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Test Mode */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: MeterShopMode unsupported */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Filler */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Filler */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Filler */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Filler */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Filler */

   /* Start ED_STD_STATUS1_BFLD - Total 16 Bits */
#if HMC_I210_PLUS_C
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Unprogrammed, supported */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Config Error, unsupported */
   electricMeterSelfTestError,                        81,   electricMeterSelfTestErrorCleared,                 91,   /* R&P: SelfChkError, unsupported */
   electricMeterMemoryRAMFailed,                      161,  electricMeterMemoryRAMSucceeded,                   161,  /* R&P: RAMFAil, unsupported */
   electricMeterMemoryROMFailed,                      160,  electricMeterMemoryROMSucceeded,                   160,  /* R&P: ROMFail */
   electricMeterMemoryNVramFailed,                    163,  electricMeterMemoryNVramSucceeded,                 163,  /* R&P: NVFail */
   electricMeterClockError,                           100,  electricMeterClockErrorCleared,                    100,  /* R&P: ClkError */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: MeasurementError, unsupported */

   electricMeterBatteryChargeMinLimitReached,         88,   electricMeterBatteryChargeMinLimitCleared,         88,   /* R&P: Low Battery */
   electricMeterPowerVoltageLossDetected,             107,  electricMeterPowerVoltageNormal,                   106,  /* R&P: LowLosPotential */
   electricMeterDemandOverflow,                       176,  electricMeterDemandNormal,                         17,   /* R&P:DmdOverload */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: PowerFial, unsupported */
   electricMeterSecurityTilted,                       105,  electricMeterSecurityTamperCleared,                105,  /* R&P: Tamper, aka Meter Inversion */
   electricMeterSecurityCoverRemoved,                 175,  electricMeterSecurityCoverReplaced,                175,  /* R&P: CoverDetect, reserved */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Ffiller */
   electricMeterPowerCurrentMaxLimitReached,          100,  electricMeterPowerCurrentMaxLimitCleared,          100,  /* R&P: JighLineCurrent */
#else //KV2c
   electricMeterConfigurationProgramUninitialized,    181,  electricMeterConfigurationProgramInitialized,      181,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   electricMeterMemoryRAMFailed,                      161,  electricMeterMemoryRAMSucceeded,                   161,
   electricMeterMemoryROMFailed,                      160,  electricMeterMemoryROMSucceeded,                   160,
   electricMeterMemoryNVramFailed,                    163,  electricMeterMemoryNVramSucceeded,                 163,
   electricMeterClockError,                           100,  electricMeterClockErrorCleared,                    100,
   electricMeterMetrologyMeasurementError,            251,  electricMeterMetrologyMeasurementErrorCleared,     251,

   electricMeterBatteryChargeMinLimitReached,         88,   electricMeterBatteryChargeMinLimitCleared,         88,
   electricMeterPowerVoltageLossDetected,             107,  electricMeterPowerVoltageNormal,                   106,
   electricMeterDemandOverflow,                       176,  electricMeterDemandNormal,                         17,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   electricMeterSecurityPasswordInvalid,              92,   electricMeterSecurityPasswordUnlocked,             92,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
#endif

   /* Start ED_STD_STATUS2 BFLD - Total 8 bits */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: All not used */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,

   /* Start ED_MFG_STATUS_1 - Total 8 bits */
#if HMC_I210_PLUS_C
   electricMeterProcessorError,                       173,  electricMeterProcessorErrorCleared,                173,  /* R&P: MeteringError */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Filler */
   electricMeterError,                                90,   electricMeterErrorCleared,                         80,   /* R&P: SystemError */
   electricMeterSecurityRotationReversed,             240,  electricMeterSecurityRotationNormal,               100,  /* R&P: Rcvd KWH */
   electricMeterPowerReactivePowerReversed,           102,  electricMeterPowerReactivePowerNormal,             102,  /* R&P: Leading KVARH */
   electricMeterConfigurationProgramLossDetected,     190,  electricMeterConfigurationProgramReestablished,    190,  /* R&P: LossOfProgram */
   electricMeterTemperatureThresholdMaxLimitReached,  254,  electricMeterTemperatureThresholdMaxLimitCleared,  180,  /* R&P: HighTemperature */
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,    /* R&P: Filler */

#else //KV2c
   electricMeterProcessorError,                       173,  electricMeterProcessorErrorCleared,                163,
   ( EDEventType )eALRMID_RSVD,                       0,    ( EDEventType )eALRMID_RSVD,                       0,
   electricMeterError,                                90,   electricMeterErrorCleared,                         80,
   electricMeterSecurityRotationReversed,             90,   electricMeterSecurityRotationNormal,               100,
   electricMeterPowerReactivePowerReversed,           102,  electricMeterPowerReactivePowerNormal,             102,
   electricMeterConfigurationProgramLossDetected,     190,  electricMeterConfigurationProgramReestablished,    190,
   electricMeterMemoryProgramError,                   162,  electricMeterMemoryProgramErrorCleared,            162,
   electricMeterMemoryDataError,                      95,   electricMeterMemoryDataErrorCleared,               95
#endif
};
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/* ****************************************************************************************************************** */
/***********************************************************************************************************************

   Function name: HMC_DIAGS_DoDiags

   Purpose: Checks tasks

   Arguments: uint8_t cmd, void *pData

   Returns: uint8_t Results of request

 **********************************************************************************************************************/
uint8_t HMC_DIAGS_DoDiags( uint8_t cmd, void *pData )
{
   /* lint -esym(550,checkEvents,procResult,pendingEvent) not accessed */
   EventKeyValuePair_s  keyVal[ 9 ];                                 /* Event key/value pair info                          */
   EventData_s          progEvent;                                   /* Event info                                         */
   const alarmInfo_t   *alarmPtr;                                    /* Pointer to the constant array of alarms            */
   buffer_t            *pEvent;                                      /* Pointer to copy of event history file              */
   HMC_COM_INFO        *pResponse;                                   /* casted copy of pData                               */
   eventHistory_t      *peventHistory;                               /* Pointer to the event history data type             */
   uint32_t             idx;                                         /* Index into file/buffer                             */
   uint16_t             eventNum;                                    /* Offset into meterAlarms array                      */
   uint8_t              preValue;                                    /* Previous value of meter event (reduced to 0/1)     */
   uint8_t              mask;                                        /* Used to check event status in event history        */
   uint8_t              curVal;                                      /* Current value of meter event                       */
   uint8_t              retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;   /* Return value, assume always idle */
   bool                 eventChanged;                                /* Indicator of need to write the event history file  */

   pResponse = ( HMC_COM_INFO * )pData;
   switch( cmd )
   {
      case HMC_APP_API_CMD_INIT:
      {
         if ( !bModuleInitialized_ )
         {
            returnStatus_t   resFileOpen;
            returnStatus_t   resAddTimer;
            tTimeSysPerAlarm alarmSettings;                    /* Configure the periodic alarm for time */
            ( void )OS_QUEUE_Create( &hmcDiagQueueHandle_, HMC_DIAG_QUEUE_SIZE, "HMC_DIAGS" );
            alarmSettings.pMQueueHandle = NULL;                /* Don't use the message queue */
            alarmSettings.bOnInvalidTime = false;              /* Only alarmed on valid time, not invalid */
            alarmSettings.bOnValidTime = true;                 /* Alarmed on valid time */
            alarmSettings.bSkipTimeChange = true;              /* do NOT notify on time change */
            alarmSettings.bUseLocalTime = false;               /* Use UTC time */
            alarmSettings.pQueueHandle = &hmcDiagQueueHandle_;
            alarmSettings.ucAlarmId = 0;                       /* Alarm Id */
            alarmSettings.ulOffset = 0;
            alarmSettings.ulPeriod = TIME_TICKS_PER_MIN * DIAG_ALARM_PERIOD_MIN;
            resAddTimer = TIME_SYS_AddPerAlarm( &alarmSettings );
            alarmId_ = alarmSettings.ucAlarmId;                /* Store alarm ID that was given by Time Sys. module */

            FileStatus_t fileStatus;                           /* Contains the file status */


            /*TODO:
              In the future this file should only be opened by the ALRM tsk and
              accessed through getters/setters protected by a mutex.
              This elminates any possability of a race conditon.
            */
            resFileOpen = FIO_fopen( &diagFileHndl_,                    /* File Handle to access the file. */
                                     ePART_NV_APP_BANKED,               /* File is in the banked partition. */
                                     ( uint16_t )eFN_EVENTS,            /* File ID (filename) */
                                     ( lCnt )sizeof( eventHistory_t ),  /* Size of the data in the file. */
                                     FILE_IS_NOT_CHECKSUMED,            /* File attributes to use. */
                                     0,                                 /* The update rate of the data in the file. */
                                     &fileStatus );                     /* Contains the file status */
            /* Only call the module initialized if both calls were successful. */
            if ( ( eSUCCESS == resFileOpen ) && ( eSUCCESS == resAddTimer ) )
            {
               bModuleInitialized_ = true;
            }
            else
            {
               HMC_DIAGS_PRNT_ERROR( 'E', "Diags Failed to open File or add Diags Timer!" );
            }

         }
         ( void )memset( &mtrDiags_, 0, sizeof( mtrDiags_ ) );
         /* Fall through to Activate */
      }
      // FALLTHROUGH
      case HMC_APP_API_CMD_ACTIVATE: // Fall through to Init
      {
         retries_ = DIAG_RETRIES;
         bReadFromMeter_ = true;
         break;
      }
      case HMC_APP_API_CMD_STATUS:
      {
         buffer_t    *pBuf = NULL;  //Buffer used to point to the data in the msg queue

         HMC_DIAGS_PurgeQueue();
         pBuf = OS_QUEUE_Dequeue( &hmcDiagQueueHandle_ ); /* Dequeue the latest item  */
         if ( NULL != pBuf )
         {
            //Got message, check the alarm ID
            tTimeSysMsg *pTimeSysMsg = ( tTimeSysMsg * )( void * )&pBuf->data[0]; //Alarm message

            if ( pTimeSysMsg->alarmId == alarmId_ )
            {
               //Alarm is for this module
               if ( bModuleInitialized_ )
               {
                  retries_ = DIAG_RETRIES;
                  bReadFromMeter_ = true;
               }
            }
            BM_free( pBuf );
         }

         if ( bReadFromMeter_ )
         {
            retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
         }

         break;
      }
      case HMC_APP_API_CMD_PROCESS:
      {
         retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
         pResponse->pCommandTable = ( uint8_t far * )tblsMtrDiags_;
         bReadFromMeter_ = false;
         break;
      }
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {
         bReadFromMeter_ = false;
         failCntDwn_ = DIAG_FAILURES_BEFORE_ALARM;   /* Success!  reset the failure count */

         pEvent = BM_alloc( sizeof ( eventHistory_t ) );
         if ( pEvent != NULL )
         {
            peventHistory = (eventHistory_t *)( void * )pEvent->data;   /* Pointer to diagnostic flags as last written. */
            eventChanged = ( bool ) false;                              /* Initialize to "no events have changed".   */

            /* Read diagnostics file.  */
            if ( eSUCCESS == FIO_fread( &diagFileHndl_, peventHistory->meterDiags, ( fileOffset ) offsetof(eventHistory_t, meterDiags), sizeof( peventHistory->meterDiags) ) )
            {

               /* Loop through all the events in standard table 3.   */
               for ( eventNum = 0, alarmPtr = diagAlarmIds; eventNum < ARRAY_IDX_CNT( diagAlarmIds ); alarmPtr++, eventNum++ )
               {
                  if ( alarmPtr->eventSet != ( EDEventType )0 )
                  {
                     mask = ( uint8_t ) ( 1U << ( eventNum  % 8 ) );       /* Compute bit number in byte.   */
                     idx  = ( uint32_t )( eventNum / 8 );
                     preValue = peventHistory->meterDiags[ idx ] & mask;  /* Retrieve byte from history with desired bit. */
                     preValue >>= eventNum % 8;                            /* Convert bit to 0 or 1 by shifting bit to bit 0  */

                     curVal = pResponse->RxPacket.uRxData.sTblData.sRxBuffer[ idx ] & mask;

                     if ( curVal != 0 )
                     {
                        curVal = 1;
                     }
                     if ( ( preValue ^ curVal ) != 0 ) /* Did event change? */
                     {
                        eventChanged = ( bool ) true;

                        *( uint16_t * )keyVal[ 0 ].Key = 0;       /* Pre-fill eventlog info. */
                        *( uint16_t * )keyVal[ 0 ].Value = 0;
                        progEvent.eventKeyValuePairsCount = 0;
                        progEvent.eventKeyValueSize = 0;
                        progEvent.markSent = (bool)false;

                        if ( curVal != 0 ) /* Event just occurred  */
                        {
                           peventHistory->meterDiags[ idx ] |= mask;  /* Update status bit in memory image of file.   */
                           progEvent.eventId = ( uint16_t )alarmPtr->eventSet;

                           ValueInfo_t readingInfo;
                           if( (uint16_t)electricMeterPowerCurrentMaxLimitReached == progEvent.eventId )
                           {  // add nvp information for current threshold limit reached
                              if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( iA, &readingInfo ) )
                              {  // get the current value for phase A
                                 readingInfo.Value.uintValue = readingInfo.Value.uintValue / 10; // convert deci amps to amps
                                 progEvent.eventKeyValueSize = sizeof(uint16_t);
                                 *( uint16_t * )keyVal[ progEvent.eventKeyValuePairsCount ].Key   = ( uint16_t )readingInfo.readingType;
                                 (void)memcpy (&keyVal[ progEvent.eventKeyValuePairsCount ].Value[0], (uint8_t *)&readingInfo.Value.uintValue, readingInfo.valueSizeInBytes );
                                 progEvent.eventKeyValuePairsCount++;
                              }
                           }

                           ( void )EVL_LogEvent( alarmPtr->prioritySet, &progEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
                        }
                        else  /* Event just cleared   */
                        {
                           peventHistory->meterDiags[ idx ] &= ~mask; /* Update status bit in memory image of file.   */
                           if ( alarmPtr->eventCleared != ( EDEventType )0 )
                           {
                              /* Log event   */
                              progEvent.eventId = ( uint16_t )alarmPtr->eventCleared;
                              ( void )EVL_LogEvent( alarmPtr->priorityCleared, &progEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
                           }
                        }
                     }
                  }
               }
            }
            else
            {
               HMC_DIAGS_PRNT_ERROR( 'E', "Diags file read!" );
            }
            if ( eventChanged )  /* Need to write the history file.  */
            {
               HMC_DIAGS_PRNT_INFO( 'I', "Updating event history file" );
               ( void )FIO_fwrite( &diagFileHndl_, ( fileOffset ) offsetof(eventHistory_t, meterDiags), (void *) peventHistory->meterDiags, sizeof( peventHistory->meterDiags) );
            }
            BM_free( pEvent );
         }
         else
         {
            HMC_DIAGS_PRNT_ERROR( 'E', "Diags Buffer alloc!" );
         }
         HMC_DIAGS_PRNT_INFO( 'I', "Diags (ST3) - Updated" );
         break;
      }
      case HMC_APP_API_CMD_ABORT:
      case HMC_APP_API_CMD_TBL_ERROR:
      case HMC_APP_API_CMD_ERROR:
      {
         if ( 0 != retries_ ) /* retries already at zero? */
         {
            /* Yes, decrement counter, allow a retry */
            retries_--;
            HMC_DIAGS_PRNT_WARN( 'W', "Diags (ST3) - Retrying" );
         }
         else
         {
            /* No - Don't retry */
            bReadFromMeter_ = false;
            HMC_DIAGS_PRNT_ERROR( 'E', "Diags (ST3)" );
         }
         if ( 0 != failCntDwn_ ) /* If already 0, then don't resend the alarm */
         {
            if ( 0 == --failCntDwn_ ) /* Decrement, if 0, send the alarm. */
            {
               HMC_DIAGS_PRNT_INFO( 'I', "Diags (ST3) sending alarm" );
               /* Todo:  Send an alarm here! */
            }
         }
         break;
      }
      default: // No other commands are supported in this applet
      {
         retVal = ( uint8_t )HMC_APP_API_RPLY_UNSUPPORTED_CMD;
         break;
      }
   }
   return( retVal );
}
/* ****************************************************************************************************************** */
/***********************************************************************************************************************

   Function name: HMC_DIAGS_PurgeQueue

   Purpose: When the Host Meter Communication is broken either due to HW issue or due to incorrect password, purge all but one items from the Queue

   Arguments: None

   Returns: None

 **********************************************************************************************************************/
void HMC_DIAGS_PurgeQueue ( void )
{
   uint16_t     nQueueElements;
   buffer_t     *pBuf = NULL;  //Buffer used to point to the data in the msg queue

   nQueueElements = OS_QUEUE_NumElements( &hmcDiagQueueHandle_ );
   // DG: Avoid Buffer LEAK when HMC password is incorrect. HMC_DIAGs Allocates buffer every 5 min but service & Free once every 10 mins
   // Dequeue Messages in this queue. If there are more than one, delete the old one's and service the latest.
   while ( nQueueElements > 1 )
   {
      pBuf = OS_QUEUE_Dequeue( &hmcDiagQueueHandle_ );
      if ( nQueueElements > 1 )
      {
         HMC_DIAGS_PRNT_INFO('I', " Current No. of Elements in the Queue: %d", nQueueElements );
         BM_free( pBuf );
         HMC_DIAGS_PRNT_INFO('I', " Free stacked buffer !!");
      }
      nQueueElements--;
   }

}