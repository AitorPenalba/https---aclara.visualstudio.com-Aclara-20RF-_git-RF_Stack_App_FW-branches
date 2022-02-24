/***********************************************************************************************************************

   Filename: hmc_time.c

   Global Designator: HMC_TIME_

   Contents: Sets the host meter's date/time.

 ***********************************************************************************************************************
   Copyright (c) 2001-2021 Aclara Technologies LLC.  All rights reserved. This program may not be reproduced, in
   whole or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA TECHNOLOGIES LLC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
   Revision History:
   v1.0 - Initial Release
   v1.1 - Updated per code review
        - Fixed the retry error
   v1.2 - KD 03-08-10 Needed to typecast READ_NV_STRING and WRITE_NV_STRING NV Address
   v2.0 - Updated for new host meter communications
   v2.1 - KD 10-20-11 Added option of setting the host meter time daily at 23:55:00.
                      Added comments
                      Added typedefs for configuration register and data being sent to the host meter.
                      Corrected 'INDEX_OF_PROC_ID' and time is not byte-swapped
   v2.2 - KD 11-29-11 Changed automatic mode (daily) from 22:55:00 to 23:55:00.
   v2.3 - KD 04-03-12 Added day of the week to the string sent to the meter.  This will take the Focus meter (v5.34 and
                      later) out of standby mode.  If the Focus is in standby, the date/time will not be accepted
                      without sending the day of the week.  This will also clear the DST and GMT flags in the Focus
                      meter.
   v2.4 - KD 04-10-12 Now sets DST_APPLIED_FLAG in the TIME_DATE_QUAL_BFLD to make the Focus meter use its own settings.
   v3.0 - KD 08-01-13 Reads time and corrects it if it is off by x.  Implements a "dead-band" around midnight.
 ******************************************************************************************************************** */
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "psem.h"
#include "time_sys.h"
#if ( CLOCK_IN_METER == 1 )
#include "time_util.h"
#endif
#include "hmc_time.h"
#if ( CLOCK_IN_METER == 1 )
#include "hmc_eng.h"
#include "hmc_seq_id.h"
#include "hmc_mtr_info.h"
#include "meter_ansi_sig.h"
#include "time_DST.h"
#endif
#include "DBG_SerialDebug.h"

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#if ( CLOCK_IN_METER == 1 )
typedef enum
{
   eCLOCK_EXIST,           /* Check for existence of clock/battery in meter   */
   eREAD_TIME_ONLY,        /* Read meter's time                               */
   eUPDATE_TIME,           /* Read and potentialy set meter's time            */
   /***************************************************************************************************
      NOTE:
      ePROC_WRITE_TIME through eTIME_APP_IDLE must be consecutive; state machine steps thru them.
   ****************************************************************************************************/
   ePROC_WRITE_TIME,
   ePROC_REMOTE_RESET,
   ePROC_RST_LIST_PTRS,
   ePROC_PGM_CTRL,
   eTIME_APP_IDLE
} eSTATES_t;

PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t          cmd;         /* Command to send, example: Start programming, stop programming, etc... */
   MtrTimeFormatLowPrecision_t  dateTimeRcd;   /* Date/Time format:  YY MM DD HH MM */
} mfgProcPgmCtrl_t;              /* See comment below */
PACK_END
/* This procedure may be used to bracket a programming session with the meter in order to protect against incomplete
   programming if the session is abnormally terminated for any reason.  When the meter receives a  Start Programming
   command, it will buffer all subsequent changes to programmed data until a Stop Programming is received, at which
   point the changes will be 'committed', or saved to permanent storage.  If the session is terminated before a Stop
   Programming is received, buffered changes will be discarded and the meter will continue to operate with the
   parameters that were active before the session began.  Date and time should be sent with this procedure; this
   information will be saved in the Security Log Table (Manufacturer Table 78).
   This procedure will not be executed when the meter is in Unconfigured or Uncalibrated Mode.
   Note:  In the kV2, the Stop Programming version of this procedure will not cause automatic logoff/termination; i.e.
   for the first parameter, only the values 0, 3 and 4 are supported.  The other values are used by other GE devices. */

/*lint -esym(751, mfgProcPgmCtrlParam_t) */
/*lint -esym(749, eMP_PGMCTRL_STRT_PGM, eMP_PGMCTRL_STP_PGM_MX_DEMANDS_LOGOFF, eMP_PGMCTRL_STP_NO_CLR_LOGOFF) */
/*lint -esym(749, eMP_PGMCTRL_STP_CLR_MAX_DMD_NO_LOGOFF ) some of the enums are not used  */
typedef enum mfgProcPgmCtrlParam
{
   eMP_PGMCTRL_STRT_PGM = ( uint8_t )0,
   eMP_PGMCTRL_STP_PGM_MX_DEMANDS_LOGOFF,    /* Stop Programming/Clear Max Demands/Logoff */
   eMP_PGMCTRL_STP_NO_CLR_LOGOFF,            /* Stop Programming/No clear/Logoff */
   eMP_PGMCTRL_STP_NO_CLR_NO_LOGOFF,         /* Stop Programming/No clear/No Logoff */
   eMP_PGMCTRL_STP_CLR_MAX_DMD_NO_LOGOFF     /* Stop Programming/Clear Max Demands/No Logoff */
} mfgProcPgmCtrlParam_t;                     /* Valid commands to send with MFG Procedure 70, Program Control */
typedef enum
{
   eCLOCK_UNKNOWN,
   eCLOCK_PRESENT,
   eCLOCK_NOT_PRESENT
} mtrExistence;

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static setDateTimeProcParam_t setDateTimeProcParam_;           /* Contains data string that is sent with std proc #10. */
static mfgProcPgmCtrl_t       _prgCtrlMfgProcParam;            /* Contains data string for mfg procedure 70. */
static procNumId_t            procedureNumberAndId_;           /* The procedure # and ID are sent to the host meter */
static mtrExistence           clockExists = eCLOCK_UNKNOWN;    /* Meter has a clock/battery  */
static uint8_t                bigEndian_;                      /* Indicator of meter's "endianess"; 0 - little, 1 - big */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define  HMC_TIME_PERIODIC_READ_ONLY  0     /* 0: periodically read/set meter's time; 1: periodically read (only)
                                               meter's time */

/* Configuration parameters that could change from product to product or if the specifications change. */
#define HMC_TIME_RETRIES               ((uint8_t)1)           /* Application Level Retries */
#define HMC_TIME_SESSION_FAILURES_CNT  ((uint8_t)3)           /* After x session failures, send alarm */
#define TIME_RESET_THRESHOLD           ((uint16_t)60 * TIME_TICKS_PER_SEC)
#define TIME_MIDNIGHT_RANGE_MIN        ((uint32_t)15)         /* Range where time set will NOT be performed */
#define TIME_MIDNIGHT_NEG_VAL          (TIME_TICKS_PER_DAY - (TIME_MIDNIGHT_RANGE_MIN * TIME_TICKS_PER_MIN))
#define TIME_MIDNIGHT_POS_VAL          (TIME_MIDNIGHT_RANGE_MIN * TIME_TICKS_PER_MIN)
#define TIME_IN_MIDNIGHT_RANGE(time)   ((time > TIME_MIDNIGHT_NEG_VAL) || (time < TIME_MIDNIGHT_POS_VAL))

#define MFG_PROC_PROGRAM_CTRL          ((uint16_t)70 | PROC_MFG)         /* MFG procedure for programming */

/* Alarm configuration parameters */
#define TIME_ALARM_PEROID_MIN          ((uint16_t)24 * 60)         /* Called once a day */
#define TIME_ALARM_OFFSET_SEC          (((uint16_t)60 * 18) + 30)  /* Offset of 18.5 minutes, needs  to be more than TIME_MIDNIGHT_RANGE_MIN, if ones a day */

/* Compensate for communication latency:  This is done by looking at the average log-in time (3 packets) + the time
   it takes to send the command (procedure 10) to set the date/time. */
#define TIME_SLEEP_COM_LATENCY_mS      ((uint32_t)200 + 25)

/*lint -esym( 750, HMC_TIME_PRNT_INFO, HMC_TIME_PRNT_WARN, HMC_TIME_PRNT_ERROR ) May not be referenced */
#if ENABLE_PRNT_HMC_TIME_INFO
#define HMC_TIME_PRNT_INFO( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_TIME_PRNT_INFO( a, fmt,... ) ( void )0
#endif

#if ENABLE_PRNT_HMC_TIME_WARN
#define HMC_TIME_PRNT_WARN( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_TIME_PRNT_WARN( a, fmt,... ) ( void )0
#endif

#if ENABLE_PRNT_HMC_TIME_ERROR
#define HMC_TIME_PRNT_ERROR( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_TIME_PRNT_ERROR( a, fmt,... ) ( void )0
#endif

#if( RTOS_SELECTION == FREE_RTOS )
#define HMC_TIME_QUEUE_SIZE 10 //NRJ: TODO Figure out sizing
#else
#define HMC_TIME_QUEUE_SIZE 0 
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */

static const tReadPartial tblReadTimeTbl_[] =    /* Reads procedure results tbl to verify the procedure executed */
{
   {
      CMD_TBL_RD_PARTIAL,
      BIG_ENDIAN_16BIT( STD_TBL_CLOCK ),                          /* As defined in psem.h */
      { 0, 0, 0 },                                                /* Offset */
      BIG_ENDIAN_16BIT( sizeof( MtrTimeFormatHighPrecision_t ) )  /* Length to read */
   }
};

static const HMC_PROTO_TableCmd tblCmdReadTime_[] =  /* Information to send to the host meter. */
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblReadTimeTbl_ ), ( uint8_t far * )&tblReadTimeTbl_[0] },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_Table tblReadTime_[] = /* Performs 2 operations, Full Tbl Wr (std proc 10) & reads results. */
{
   &tblCmdReadTime_[0],  /* Reads the meter time. */
   NULL              /* No other commands - NULL terminate. */
};

static const tWriteFull tblTimeProcedure_[] =  /* Full tbl write to execute std proc 10. */
{
   CMD_TBL_WR_FULL,        /* Full Table Write - Must be used for procedures. */
   BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE )   /* Procedure Table ID */
};

static const tReadPartial tblTimeProcResult_[] =    /* Reads procedure results tbl to verify the procedure executed */
{
   CMD_TBL_RD_PARTIAL,
   PSEM_PROCEDURE_TBL_RES,                /* As defined in psem.h */
   TBL_PROC_RES_OFFSET,
   BIG_ENDIAN_16BIT( sizeof( tProcResult ) )  /* Length to read */
};

static const HMC_PROTO_TableCmd tblCmdTimeProc_[] =  /* Information to send to the host meter. */
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblTimeProcedure_ ),   ( uint8_t far * ) & tblTimeProcedure_[0] },     /* Tlb info */
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( procedureNumberAndId_ ),  ( uint8_t far * ) & procedureNumberAndId_ }, /* Proc and ID */
   { HMC_PROTO_MEM_RAM, ( uint8_t )sizeof( setDateTimeProcParam_ ), ( uint8_t far * ) & setDateTimeProcParam_ },  /* Var Data */
   { HMC_PROTO_MEM_NULL }  /* End of cmd *//*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_TableCmd tblCmdTimeRes_[] =
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblTimeProcResult_ ), ( uint8_t far * )tblTimeProcResult_ }, /* Read Proc Result */
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_Table tblTimeSet_[] = /* Performs 2 operations, Full Tbl Wr (std proc 10) & reads results. */
{
   &tblCmdTimeProc_[0],  /* Std Procedure 10 (Full Table Write) - sets the Date/Time in the host meter. */
   &tblCmdTimeRes_[0],   /* Read the results from the procedure. */
   NULL                 /* No other commands - NULL terminate. */
};

#if 0    // xxx Not used because the structures are not being packed properly!
static const remoteResetProcParam_t remoteRstParam_ = /* Remote reset parameters */
{
   0,  /* Demand Rst = 0 - Do not do a demand reset */
   0,  /* Self Read = 0 - Do not do a self read */
   1,  /* seasonChange = 1 - Do a season change according to KV */
   0   /* new Season = 0 */
};
#else
static const uint8_t remoteRstParam_ = 4; /* Remote reset parameters */
#endif

static const tWriteFull tblRemoteRstProcedure_[] =  /* Full tbl write to execute std proc 4. */
{
   CMD_TBL_WR_FULL,                                   /* Full Table Write - Must be used for procedures. */
   BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE )       /* Procedure Table ID */
};

static const tReadPartial tblRemoteRstProcResult_[] = /* Reads proc results tbl to verify the procedure executed */
{
   CMD_TBL_RD_PARTIAL,
   PSEM_PROCEDURE_TBL_RES,                            /* As defined in psem.h */
   TBL_PROC_RES_OFFSET,
   BIG_ENDIAN_16BIT( sizeof( tProcResult ) )              /* Length to read */
};

static const HMC_PROTO_TableCmd tblCmdRemoteRstProc_[] = /* Information to send to the host meter. */
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblRemoteRstProcedure_ ), ( uint8_t far * )&tblRemoteRstProcedure_[0] },
   { HMC_PROTO_MEM_RAM, ( uint8_t )sizeof( procedureNumberAndId_ ),    ( uint8_t far * )&procedureNumberAndId_ },
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( remoteRstParam_ ), ( uint8_t far * )&remoteRstParam_ },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_TableCmd tblCmdRemoteRstRes_[] =
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblRemoteRstProcResult_ ), ( uint8_t far * )tblRemoteRstProcResult_ }, /* Rd Proc Res */
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_Table tblRemoteRst_[] = /* Performs 2 operations, Full Tbl Wr (std proc 10) & reads results. */
{
   &tblCmdRemoteRstProc_[0],  /* Std Procedure 9 (Full Table Write) - Remote Reset. */
   &tblCmdRemoteRstRes_[0],   /* Read the results from the procedure. */
   NULL                       /* No other commands - NULL terminate. */
};

static const uint8_t rstLstPtrsProcParam_ = ( uint8_t )
      eLIST_TYPE_LP_DATA_SET; /* Reinit load profile data per KV2 spec. */

static const tWriteFull tblRstLstPtrsProcedure_[] =  /* Full tbl write to execute std proc 4. */
{
   CMD_TBL_WR_FULL,                                   /* Full Table Write - Must be used for procedures. */
   BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE )       /* Procedure Table ID */
};

static const tReadPartial tblRstLstPtrsProcResult_[] = /* Reads proc results tbl to verify the procedure executed */
{
   CMD_TBL_RD_PARTIAL,
   PSEM_PROCEDURE_TBL_RES,                            /* As defined in psem.h */
   TBL_PROC_RES_OFFSET,
   BIG_ENDIAN_16BIT( sizeof( tProcResult ) )              /* Length to read */
};

static const HMC_PROTO_TableCmd tblCmdRstLstPtrsProc_[] = /* Information to send to the host meter. */
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblRstLstPtrsProcedure_ ), ( uint8_t far * )&tblRstLstPtrsProcedure_[0] },
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( procedureNumberAndId_ ),    ( uint8_t far * )&procedureNumberAndId_ },
   { HMC_PROTO_MEM_RAM, ( uint8_t )sizeof( rstLstPtrsProcParam_ ), ( uint8_t far * )&rstLstPtrsProcParam_ },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_TableCmd tblCmdRstLstPtrsRes_[] =
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblRstLstPtrsProcResult_ ), ( uint8_t far * )tblRstLstPtrsProcResult_ },  /* Rd Proc Res */
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_Table Tables_RstLstPtrs[]
= /* Performs 2 operations, Full Tbl Wr (std proc 10) & reads results. */
{
   &tblCmdRstLstPtrsProc_[0], /* Std Procedure 4 (Full Table Write) - Reset List Pointers */
   &tblCmdRstLstPtrsRes_[0],  /* Read the results from the procedure. */
   NULL                  /* No other commands - NULL terminate. */
};

static const tWriteFull tblMfgProcPgmCtrlProcedure_[] =   /* Full tbl write to execute MFG proc 70. */
{
   CMD_TBL_WR_FULL,                                         /* Full Table Write - Must be used for procedures. */
   BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE )             /* Procedure Table ID */
};

static const tReadPartial tblMfgProcPgmCtrlProcResult_[] = /* rd proc results tbl to verify the procedure executed */
{
   CMD_TBL_RD_PARTIAL,
   PSEM_PROCEDURE_TBL_RES,                                  /* As defined in psem.h */
   TBL_PROC_RES_OFFSET,
   BIG_ENDIAN_16BIT( sizeof( tProcResult ) )                    /* Length to read */
};

static const HMC_PROTO_TableCmd tblCmdMfgProcPgmCtrlProc_[] =    /* Information to send to the host meter. */
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblMfgProcPgmCtrlProcedure_ ),  ( uint8_t far * )&tblMfgProcPgmCtrlProcedure_[0] },
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( procedureNumberAndId_ ),          ( uint8_t far * )&procedureNumberAndId_ },
   { HMC_PROTO_MEM_RAM, ( uint8_t )sizeof( _prgCtrlMfgProcParam ),           ( uint8_t far * )&_prgCtrlMfgProcParam },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_TableCmd tblMfgProcPgmCtrlRes_[] =
{
   /* Rd Proc Res */
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblMfgProcPgmCtrlProcResult_ ), ( uint8_t far * )tblMfgProcPgmCtrlProcResult_ },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_Table tblMfgProcPgmCtrl_[]
= /* Performs 2 operations, Full Tbl Wr (std proc 10) & reads results. */
{
   &tblCmdMfgProcPgmCtrlProc_[0],  /* MFG Procedure 70 (Full Table Write) - Stop Programming */
   &tblMfgProcPgmCtrlRes_[0],      /* Read the results from the procedure. */
   NULL                            /* No other commands - NULL terminate. */
};

/* Commands needed to determine if meter has a clock/battery. This dictates whether demand data has time stamps.  */
static const tReadPartial tblChkMeterHasClock[] =  /* Read mfg table to determine meter's "mode"   */
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT( MFG_TBL_GE_DEVICE_TABLE ), /* As defined in meter_ansi_sig.h (MFG Table 0) */
   0, 0, 15 ,                                   /* Offset */
   BIG_ENDIAN_16BIT( sizeof( uint8_t ) )        /* Length to read */
};

static const HMC_PROTO_TableCmd tblCmdReadMode_[] =  /* Read mfg table 0. */
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblChkMeterHasClock ), ( uint8_t far * )&tblChkMeterHasClock[0] },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 extra NULLs not required in initializer. */
};

static const HMC_PROTO_Table tblCheckClock_[] = /* Read MFG Table 0, mode  */
{
   &tblCmdReadMode_[0], /* Reads the meter mode. */
   NULL                 /* No other commands - NULL terminate. */
};
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/* *********************************************************************************************************************

   Function name: HMC_TIME_Set_Time

   Purpose: Sets the date and time in the host meter if the following conditions are met:
      1.  The applet is enabled
      2.  The applet is activated (can be activated by a time tick or the 'HMC_APP_API_CMD_ACTIVATE' command).
      3.  Time is valid
      4.  Date is >= Jan 1, 2000 (The meter will not accept any date prior to 1/1/2000)

   Arguments: uint8_t ucCmd, void *pData

   Returns: uint8_t  - See cfg_app.h enum 'HMC_APP_API_RPLY' for response codes.

 ******************************************************************************************************************** */
uint8_t HMC_TIME_Set_Time( uint8_t ucCmd, void *pData )
{
   uint8_t              retVal;                    /* Stores the return value */
#if ( CLOCK_IN_METER == 1 )
   uint8_t              dayOfWeek;                 /* Contains day of week to send to the host meter, 0 = Sunday */
   static uint8_t       retries_;                  /* Number of application level retries */
   static uint8_t       sessionFailures_ = HMC_TIME_SESSION_FAILURES_CNT;  /* After x session failures, send alarm */
   static bool          bFeatureEnabled_ = true;   /* Flag to indicate this module is enabled or disabled */
   static eSTATES_t     eState_ = eTIME_APP_IDLE;  /* State of this module, reading time or writing time. */
   static bool          bMeterDateChange_ = false; /* Flag is set if the meter's date changes during an update */
   static uint8_t       alarmId_ = MAX_ALARMS;     /* Alarm ID; set to invalid value, initially */
   static OS_QUEUE_Obj  hmcTimeQueueHandle;        /* Used to read meter's time every 5 minutes, offset 2.5 minutes  */
#endif

   retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;      /* Assume the applet is idle */

#if ( CLOCK_IN_METER == 1 )
   if ( clockExists != eCLOCK_NOT_PRESENT )
   {
      switch ( ucCmd )                                /* Command passed in */
      {
         case ( uint8_t )HMC_APP_API_CMD_DISABLE_MODULE: /* Command to disable this applet? */
         {
            bFeatureEnabled_ = false;                 /* Disable the applet */
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_ENABLE_MODULE: /* Command to enable this applet? */
         {
            bFeatureEnabled_ = true;                  /* Enable the applet */
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_INIT:                /* Initialize the applet? */
         {
            retries_ = HMC_TIME_RETRIES;                    /* Set application level retry count */
            eState_ = eCLOCK_EXIST;                         /* Kick off process of discovering existence of clock */
            retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_MSG:         /* Activate command passed in? */
         {
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_ACTIVATE:    /* Kick off process of reading/setting meter's date/time */
         {  /* Activated by user (debug port) or demand reset function  */
            retries_ = HMC_TIME_RETRIES;              /* Set application level retry count */
            eState_ = eUPDATE_TIME;                   /* Read/update meter's time   */
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_STATUS:        /* Command to check to see if the applet needs communication */
         {
            if ( eState_ == eCLOCK_EXIST )
            {
               /* Haven't yet discovered existence of meter's clock. */
               retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
            }
            else
            {
               buffer_t *pBuf;  //Buffer used to point to the data in the msg queue

               pBuf = OS_QUEUE_Dequeue( &hmcTimeQueueHandle ); /* See if there is a message in the queue */
               if ( NULL != pBuf )
               {
                  //Got message, check the alarm ID
                  tTimeSysMsg *pTimeSysMsg = ( tTimeSysMsg * )( void * )&pBuf->data[0]; //Alarm message

                  if ( pTimeSysMsg->alarmId == alarmId_ )
                  {
                     //Alarm is for this module
                     if ( bFeatureEnabled_ )
                     {
#if ( HMC_TIME_PERIODIC_READ_ONLY == 0 )
                        eState_ = eUPDATE_TIME;
#else
                        eState_ = eREAD_TIME_ONLY;
#endif
                        retries_ = HMC_TIME_RETRIES;                    /* Set application level retry count */
                     }
                  }
                  else
                  {
                     NOP();
                  }
                  BM_free( pBuf );
               }

               /* Check if application is enabled AND applet is activated AND time is valid */
               if ( bFeatureEnabled_ && ( eTIME_APP_IDLE != eState_ ) && TIME_SYS_IsTimeValid() )
               {
                  switch( eState_ ) /*lint !e788 not all enums used in switch   */
                  {
                     case eUPDATE_TIME:
                     case eREAD_TIME_ONLY:
                     {
                        sysTime_t sysTime;
                        if ( TIME_SYS_GetSysDateTime( &sysTime ) ) /* Get the system time */
                        {
                           if ( !TIME_IN_MIDNIGHT_RANGE( sysTime.time ) ) /* Near midnight (example: within 15 min.) */
                           {
                              retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
                           }
                        }
                        break;
                     }
                     case ePROC_WRITE_TIME:
                     case ePROC_REMOTE_RESET:
                     case ePROC_RST_LIST_PTRS:
                     case ePROC_PGM_CTRL:
                     {
                        retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST; /* For proc, set to comm to persistent */
                        break;
                     }

                     case eTIME_APP_IDLE:
                     default:
                     {
                        break;
                     }
                  }  /*lint !e788 not all enums used in switch   */
               }
            }

            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_PROCESS:       /* Command to get the table information to start comm. */
         {
            if ( bFeatureEnabled_ )                   /* Again, check to make sure the applet is enabled. */
            {
               switch( eState_ )
               {
                  case eUPDATE_TIME:
                  case eREAD_TIME_ONLY:
                  {
                     ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )tblReadTime_;
                     retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
                     break;
                  }
                  case ePROC_WRITE_TIME:
                  {
                     uint64_t   combSysTime;      /* Contains the Combined Date/Time in STUs */
                     uint64_t   adjCombSysTime;   /* Contains the Combined Date/Time in STUs */
                     sysTime_t sysTime;

                     ( void )TIME_SYS_GetSysDateTime( &sysTime );                /* Gets current system date/time */
                     combSysTime = TIME_UTIL_ConvertSysFormatToSysCombined( &sysTime );    /* Combine date and time */
                     /* Set the combined time to the next second boundary. */
                     adjCombSysTime = TIME_TICKS_PER_SEC *
                                      ( ( combSysTime + TIME_TICKS_PER_SEC ) / TIME_TICKS_PER_SEC );
                     TIME_UTIL_ConvertSysCombinedToSysFormat( &adjCombSysTime, &sysTime );
                     dayOfWeek = ( sysTime.date + DAY_AT_DATE_0 ) % TIME_NUM_DAYS_IN_WEEK;   /* 1/1/1970 is a Thursday */

                     ( void )memset( &setDateTimeProcParam_, 0, sizeof( setDateTimeProcParam_ ) );

                     if ( eSUCCESS ==
                           TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sysTime, &setDateTimeProcParam_.DateTime ) )
                     {
                        /* Since a procedure is being executed, on a failure this applet should be the 1st applet to run
                           after logging back into the host meter.  That is why Persistent is used. */
                        retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;

                        /* The next line clears SET_TIME_DATE_QUAL flags.  This is done so that the TIME_DATE_QUAL_BFLD
                           is not used by the host meter and does not change "Day of the Week", DST, GMT, "Time Zone"
                           and "DST Applied" settings in the host meter.  */
#if 0    // Not used because the structures are not being packed properly!
                        setDateTimeProcParam_.setMask.setMaskAllFields = 0;               /* Clear all fields mask */
                        setDateTimeProcParam_.setMask.setMaskBfld.setTime = true;         /* Setting the time */
                        if ( bMeterDateChange_ ) /* only set the date if the date has changed! */
                        {
                           setDateTimeProcParam_.setMask.setMaskBfld.setDate = true;         /* Setting the date */
                        }
                        setDateTimeProcParam_.setMask.setMaskBfld.setTimeDateQual = true; /* Setting time/date Qual */
                        setDateTimeProcParam_.timeDateQual.timeDateQualAllFields = 0;     /* Clear fields (masked above) */
                        setDateTimeProcParam_.timeDateQual.timeDateQualBfld.dayOfWeek = dayOfWeek; /* Day of Week, 0-6 */
                        setDateTimeProcParam_.timeDateQual.timeDateQualBfld.dstApplied = 1; /* Meter uses its Calendar */
#else
                        setDateTimeProcParam_.setTime = true;           /* Setting the time */
                        if ( bMeterDateChange_ ) /* only set the date if the date has changed! */
                        {
                           setDateTimeProcParam_.setDate = true;        /* Setting the date */
                        }
                        setDateTimeProcParam_.setTimeDateQual = true;   /* Setting the time/date Qual */
                        setDateTimeProcParam_.dayOfWeek = dayOfWeek;    /* Day of Week, 0-6 */

                        if( DST_IsActive() )
                        {  // if DST is currently active, set the dst flag for standard procedure 10
                           setDateTimeProcParam_.dst = 1;
                        }
#endif
                        /* Set the procedure number.  Note:  If the meter is big endian, swap the procedure number. */
                        if ( bigEndian_ )  /* Big Endian Meter? */
                        {
                           /* Yes - Big Endian */
                           /* Set proc # */
                           procedureNumberAndId_.procNum = BIG_ENDIAN_16BIT( STD_PROC_SET_DATE_AND_OR_TIME );
                        }
                        else /* No - Little Endian */
                        {
                           procedureNumberAndId_.procNum = STD_PROC_SET_DATE_AND_OR_TIME; /* Set proc # */
                        }
                        procedureNumberAndId_.procId = HMC_getSequenceId();   /* Get unique ID */

                        ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )tblTimeSet_;
                        /* Sleep til next 1 second boundary - Compensate for the communication latency! */
                        int32_t sleepTime = ( int32_t )( ( ( adjCombSysTime - combSysTime ) ) - TIME_SLEEP_COM_LATENCY_mS );
                        if ( sleepTime > 0 )
                        {
                           OS_TASK_Sleep( ( uint32_t )sleepTime );
                        }
                     }
                     break;
                  }
                  case ePROC_REMOTE_RESET:
                  {
                     procedureNumberAndId_.procNum = STD_PROC_REMOTE_RESET;   /* Set proc # */
                     procedureNumberAndId_.procId = HMC_getSequenceId();      /* Get unique ID */
                     ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )tblRemoteRst_;
                     retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;
                     break;
                  }
                  case ePROC_RST_LIST_PTRS:
                  {
                     procedureNumberAndId_.procNum = STD_PROC_RESET_LIST_POINTERS; /* Set proc # */
                     procedureNumberAndId_.procId = HMC_getSequenceId();           /* Get unique ID */
                     ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )Tables_RstLstPtrs;
                     retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;
                     break;
                  }
                  case ePROC_PGM_CTRL:
                  {
                     _prgCtrlMfgProcParam.cmd = ( uint8_t )eMP_PGMCTRL_STP_NO_CLR_NO_LOGOFF;
                     _prgCtrlMfgProcParam.dateTimeRcd.year     = setDateTimeProcParam_.DateTime.year;
                     _prgCtrlMfgProcParam.dateTimeRcd.month    = setDateTimeProcParam_.DateTime.month;
                     _prgCtrlMfgProcParam.dateTimeRcd.day      = setDateTimeProcParam_.DateTime.day;
                     _prgCtrlMfgProcParam.dateTimeRcd.hour     = setDateTimeProcParam_.DateTime.hours;
                     _prgCtrlMfgProcParam.dateTimeRcd.minute   = setDateTimeProcParam_.DateTime.minutes;
                     procedureNumberAndId_.procNum = MFG_PROC_PROGRAM_CTRL;   /* Set proc # */
                     procedureNumberAndId_.procId = HMC_getSequenceId();                        /* Get unique ID */
                     ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )tblMfgProcPgmCtrl_;
                     retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;
                     break;
                  }
                  case eCLOCK_EXIST:
                  {
                     /* Ready to perform first read operation in this module. STD table 0 must have been read. */
                     ( void )HMC_MTRINFO_endian( &bigEndian_ ); /* Set this local variable once  */

                     /* Initiate read of MFG Table 0 to get mode  */
                     ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )tblCheckClock_;
                     retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
                     break;
                  }
                  case eTIME_APP_IDLE:

                  /* FALLTHROUGH */
                  default:
                  {
                     break;
                  }
               }
            }
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_MSG_COMPLETE: /* Communication to the host meter was successful! */
         {
            switch( eState_ )
            {
               case eUPDATE_TIME:
               case eREAD_TIME_ONLY:
               {
                  sysTime_t sysTime;
                  if ( TIME_SYS_GetSysDateTime( &sysTime ) ) /* Get system time - Assume not in the midnight range */
                  {
                     sysTime_t                     mtrTimeSysFmt;
                     sysTime_dateFormat_t          sysDateTime;
                     MtrTimeFormatHighPrecision_t  *pMtrDateTime;    /* Points to the rx buffer */

                     ( void )TIME_UTIL_GetTimeInDateFormat( &sysDateTime );
                     pMtrDateTime = ( MtrTimeFormatHighPrecision_t * )((HMC_COM_INFO *)pData)->RxPacket.uRxData.sTblData.sRxBuffer;
                     ( void )TIME_UTIL_ConvertMtrHighFormatToSysFormat( pMtrDateTime, &mtrTimeSysFmt ); //Convert meter time(local) to sys time format(UTC)

                     HMC_TIME_PRNT_INFO( 'I',  "Meter's Date/Time Local: %02d/%02d/%02d %02d:%02d:%02d", pMtrDateTime->month,
                                         pMtrDateTime->day,
                                         pMtrDateTime->year,
                                         pMtrDateTime->hours,
                                         pMtrDateTime->minutes,
                                         pMtrDateTime->seconds   );
                     if ( eState_ == eUPDATE_TIME )
                     {
                        /* Compute delta between sys & mtr time */
                        uint32_t deltaSysMeter = ( uint32_t )abs( ( int32_t )( sysTime.time - mtrTimeSysFmt.time ) );

                        if ( deltaSysMeter > TIME_RESET_THRESHOLD || sysTime.date != mtrTimeSysFmt.date )
                        {  //Update meter time, off by more than TIME_RESET_THRESHOLD
                           eState_ = ePROC_WRITE_TIME;
                           if ( sysTime.date != mtrTimeSysFmt.date )
                           {
                              bMeterDateChange_ = true;
                              HMC_TIME_PRNT_INFO( 'I',  "Time Set - Setting Meter's Date/Time" );
                           }
                           else
                           {
                              bMeterDateChange_ = false;
                              HMC_TIME_PRNT_INFO( 'I',  "Time Set - Setting Meter's Time" );
                           }
                        }
                        else
                        {
                           HMC_TIME_PRNT_INFO( 'I', "Time Set - Already in Sync" );
                           eState_ = eTIME_APP_IDLE;
                        }
                     }
                     else  /* Read time only; done.   */
                     {
                        eState_ = eTIME_APP_IDLE;
                     }
                  }
                  else
                  {
                     /* Todo:  Failure? */
                  }
                  break;
               }
               case ePROC_WRITE_TIME:
               case ePROC_REMOTE_RESET:
               case ePROC_RST_LIST_PTRS:
               case ePROC_PGM_CTRL:
               {
                  sessionFailures_ = HMC_TIME_SESSION_FAILURES_CNT;
                  tTbl8ProcResponse *pProcRes; /* Pointer to table 8 procedure results */

                  /* The results of reading the Procedure Results Table are located in the sRxBuffer.  The 1st byte in
                     sRxBuffer will contain the ID of last procedure executed.  The ID should be the same ID we used to
                     set Date/Time. This is checked to make sure some other procedure didn't somehow get executed and
                     the expected procedure did.  */
                  pProcRes = ( tTbl8ProcResponse * )( void * ) & ( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.sRxBuffer[0];
                  if ( procedureNumberAndId_.procId == pProcRes->seqNbr )
                  {
                     stdTbl1_t *pSt1;

                     ( void )HMC_MTRINFO_getStdTable1( &pSt1 );
                     HMC_ENG_Execute( ( uint8_t )HMC_ENG_PROCEDURE_CHK, pData ); /* Checks for Errors in Procedure Response. */

                     /* The procedure ID matches what we expect.  Time was set successfully. */
                     if ( bMeterDateChange_ )
                     {
                        eState_++;
                        if ( eState_ == eTIME_APP_IDLE )    /* The time set succeeded. */
                        {
                           HMC_TIME_PRNT_INFO( 'I',  "Meter's Date/Time set." );
                        }
                        else if( eState_ == ePROC_RST_LIST_PTRS )
                        {
                           eState_++;
                        }
                     }
                     else
                     {
                        eState_ = eTIME_APP_IDLE;  /* The time set failed, set applet to inactive. */
                        HMC_TIME_PRNT_INFO( 'I',  "Meter's Time set." );
                     }
                  }
                  else
                  {
                     if ( 0 != retries_ )
                     {
                        retries_--;
                     }
                  }
                  break;
               }
               case eCLOCK_EXIST:
               {
                  /* Just got response from reading MFG Table 0. Module not enabled yet; depends on meter's mode value.
                     0 - Demand only; 1 - Demand/LP; 2 - TOU   */
                  uint8_t meter_mode = ( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.sRxBuffer[0];
                  if ( ( meter_mode == MODE_DEMAND_LP ) || ( meter_mode == MODE_DEMAND_TOU ) )
                  {
                     clockExists = eCLOCK_PRESENT; /* Meter has a clock/battery  */
                     HMC_TIME_PRNT_INFO( 'I',  "Meter clock present" );
                     /* Clock exists. Now read it and update it if necessary/appropriate. */
                     eState_ = eUPDATE_TIME;

                     tTimeSysPerAlarm alarmSettings;                 /* Configure the periodic alarm for time */
                     int32_t offsetLocal = -DST_GetLocalOffset(); //Get the local offset (in ticks)
                     offsetLocal += (int32_t)TIME_TICKS_PER_DAY; //To get a positive value
                     offsetLocal += (int32_t)(TIME_TICKS_PER_HR * 12); //Local noon
                     offsetLocal %= (int32_t)TIME_TICKS_PER_DAY; //Local noon, same day

                     ( void )OS_QUEUE_Create( &hmcTimeQueueHandle, HMC_TIME_QUEUE_SIZE );
                     alarmSettings.pMQueueHandle = NULL;             /* Don't use the message queue */
                     alarmSettings.bOnInvalidTime = false;           /* Only alarmed on valid time, not invalid */
                     alarmSettings.bOnValidTime = true;              /* Alarmed on valid time */
                     alarmSettings.bSkipTimeChange = true;           /* do NOT notify on time change */
                     alarmSettings.bUseLocalTime = false;            /* Use UTC time */
                     alarmSettings.pQueueHandle = &hmcTimeQueueHandle;
                     alarmSettings.ucAlarmId = 0;                    /* Alarm Id */
                     alarmSettings.ulOffset = (TIME_TICKS_PER_SEC * TIME_ALARM_OFFSET_SEC) + (uint32_t)offsetLocal;
                     alarmSettings.ulPeriod = TIME_TICKS_PER_MIN * TIME_ALARM_PEROID_MIN;
                     ( void )TIME_SYS_AddPerAlarm( &alarmSettings );
                     alarmId_ = alarmSettings.ucAlarmId;             /* Store alarm ID that was given by Time Sys module. */

                     HMC_TIME_PRNT_INFO( 'I',  "Offset: %d %d", offsetLocal, alarmSettings.ulOffset);

                     retVal = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
                  }
                  else
                  {
                     clockExists = eCLOCK_NOT_PRESENT; /* Meter has no clock/battery  */
                     HMC_TIME_PRNT_INFO( 'I',  "Meter clock NOT present" );
                     bFeatureEnabled_ = ( bool )false;   /* Disable this whole module  */
                     eState_ = eTIME_APP_IDLE;
                     retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;      /* applet now idle */
                  }
                  break;
               }
               case eTIME_APP_IDLE:
               default:
               {
                  break;
               }
            }
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_TBL_ERROR: /* The response code contains an error. */
         {
            HMC_TIME_PRNT_ERROR( 'E',  "Time Set - Tbl Error" );
            if ( 0 == --retries_ )
            {
               eState_ = eTIME_APP_IDLE;   /* No more retries, switch to idle state */
               if ( 0 == --sessionFailures_ )
               {
                  HMC_TIME_PRNT_ERROR( 'E',  "Time Set - Aborting - Sending Alarm" );
                  /* Todo:  Send Error Alarm to something!  */
               }
            }
            else
            {
               switch( eState_ )
               {
                  case eUPDATE_TIME:
                  case eREAD_TIME_ONLY:
                  {
                     if ( RESP_DLK == ( ( HMC_COM_INFO * )pData )->RxPacket.ucResponseCode ) /* Data Locked Error? */
                     {
                        retVal = ( uint8_t )HMC_APP_API_RPLY_ABORT_SESSION; /* Abort the session */
                        eState_ = eTIME_APP_IDLE;
                     }
                     break;
                  }
                  case ePROC_WRITE_TIME:
                  case ePROC_REMOTE_RESET:
                  case ePROC_RST_LIST_PTRS:
                  case ePROC_PGM_CTRL:
                  case eTIME_APP_IDLE:
                  case eCLOCK_EXIST:
                  default:
                  {
                     break;               /* Done, break-out */
                  }
               }
            }
            if ( 0 != retries_ ) /* Have all the retries been exhausted? */
            {
               if ( 0 == --retries_ )
               {
                  eState_ = eTIME_APP_IDLE;   /* No more retries, switch to idle state */
                  if ( 0 == --sessionFailures_ )
                  {
                     HMC_TIME_PRNT_ERROR( 'E',  "Time Set - Sending Alarm" );
                     /* Todo:  Send Error Alarm to something!  */
                  }
               }
            }
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_ABORT: /* Communication protocol error, session was aborted */
         {
            HMC_TIME_PRNT_ERROR( 'E',  "Time Set - Abort" );
            if ( 0 != retries_ ) /* Have all the retries been exhausted? */
            {
               if ( 0 == --retries_ )
               {
                  eState_ = eTIME_APP_IDLE;   /* No more retries, switch to idle state */
                  if ( 0 == --sessionFailures_ )
                  {
                     HMC_TIME_PRNT_ERROR( 'E',  "Time Set - Sending Alarm" );
                     /* Todo:  Send Error Alarm to something!  */
                  }
               }
            }
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_ERROR: /* Command error */
         {
            HMC_TIME_PRNT_ERROR( 'E',  "Time Set - Cmd Error" );
            if ( 0 != retries_ ) /* Have all the retries been exhausted? */
            {
               if ( 0 == --retries_ )
               {
                  eState_ = eTIME_APP_IDLE;   /* No more retries, switch to idle state */
                  if ( 0 == --sessionFailures_ )
                  {
                     HMC_TIME_PRNT_ERROR( 'E',  "Time Set - Sending Alarm" );
                     /* Todo:  Send Error Alarm to something!  */
                  }
               }
            }
            break;
         }

         default: /* No other commands are supported, if we get here, the command was not recognized by this applet. */
         {
            retVal = ( uint8_t )HMC_APP_API_RPLY_UNSUPPORTED_CMD; /* Respond with unsupported command. */
            break;
         }
      }
   }
#endif
   return( retVal );
}  /*lint !e715  arguments not used in i210 build */ /*lint !e818 pData could be const   */
/* *********************************************************************************************************************

   Function name: HMC_TIME_getMfgPrc70TimeParameters( void )

   Purpose: Returns the most recent manufacturing procedure #70 parameter values for date/time when manufacturing
            procedure #70 is executed by the HMC time applet.  The parameter values are converted into seconds since
            epoch and then retured to the caller.

   Arguments: None

   Returns: uint32_t

 ******************************************************************************************************************** */
#if ( CLOCK_IN_METER == 1 )
uint32_t HMC_TIME_getMfgPrc70TimeParameters( void )
{
   MtrTimeFormatHighPrecision_t       pMtrTime; // variable to store the mfg procedure #70 date/time parameters
   sysTime_t                          pSysTime; // variable to store converted MtrTimeFormatHighPrecision_t to sysTime_t
   uint32_t                            seconds; // variable to store sysTime_t variable converted into seconds
   uint32_t                      fractionalSec; // variable to store sysTime_t variable converted into fractional seconds

   // set the MtrTimeFormatHighPrecision_t variable to match the latest mfg procedure #70 date/time parameters
   pMtrTime.year    = _prgCtrlMfgProcParam.dateTimeRcd.year;
   pMtrTime.day     = _prgCtrlMfgProcParam.dateTimeRcd.day;
   pMtrTime.month   = _prgCtrlMfgProcParam.dateTimeRcd.month;
   pMtrTime.hours   = _prgCtrlMfgProcParam.dateTimeRcd.hour;
   pMtrTime.minutes = _prgCtrlMfgProcParam.dateTimeRcd.minute;
   pMtrTime.seconds = 0; // seconds value is not used as a parameter for mfg procedure #70

   // convert MtrTimeFormatHighPrecision_t to sysTime_t
   (void)TIME_UTIL_ConvertMtrHighFormatToSysFormat( &pMtrTime, &pSysTime);

   // convert sysTime_t into seconds and fractional seconds
   TIME_UTIL_ConvertSysFormatToSeconds(&pSysTime, &seconds, &fractionalSec);

   // return the converted seconds value to the caller
   return seconds;
}
#endif

