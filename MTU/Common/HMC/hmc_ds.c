/***********************************************************************************************************************
 *
 * Filename:   hmc_ds.c
 *
 * Global Designator: HMC_DS_
 *
 * Contents:   Meter disconnect switch applet.  This module contains connect/disconnect commands, switch state/status
 *             and actuation counts.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ Created March 27, 2012
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 03/27/2012 - Initial Release
 * v0.2 - AG 6/13/2014 - Fixed OpCode 151, registers 512 and 515
 * 09/11/2014 mkv - Lint clean up.
 * 01/19/2015 mkv - Port to RF electric
 * 02/26/2015 mkv - Moved some info to .h file
 *                  Update to latest requirements:
 *                  Response delayed until meter transactions completed.
 *                  OK response when no switch action required, instead of NotModified.
 *                  Failure response on time-out of switch action.
 * 08/04/2015 mkv - Fix LSV issue. MT117 results are not reliable. Use only MT115 for operation completion decision.
 *                  Also, find that meter can be fooled into reporting the switch closed (both AMR port and LCD) in LSV
 *                  condition: open switch; apply LSV; close switch before cap charges; open switch; close again. Change
 *                  process to not accept a command that moves the switch while the cap voltage is too low.
 * 09/25/2015 mkv - Add sending alarm on switch closing due to removal of load-side voltage.
 **********************************************************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include "project.h"

#if ( REMOTE_DISCONNECT == 1 )

#include <stdbool.h>
#include <assert.h>
#include "timer_util.h"
#include "ansi_tables.h"
#include "EVL_event_Log.h"
//#include "byteswap.h"
#if ( HMC_I210_PLUS_C == 1 )
#include "hmc_start.h"
#endif

#define HMC_DS_GLOBALS
#define TM_SD_UNIT_TEST 0
#include "hmc_ds.h"
#undef  HMC_DS_GLOBALS
#include "time_sys.h"
#include "time_util.h"
#include "pack.h"

#if ( TWACS_REGISTERS_SUPPORTED == 1 )
#include "object_list.h"
#endif

#if TM_SD_UNIT_TEST
#define VERBOSE 0    /* Control level of debug printing  */
#include <stdio.h>
#include "DBG_SerialDebug.h"
#endif
#include "mode_config.h"

#include "intf_cim_cmd.h"

/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define HMC_DS_PTC9_PULSE 0
#define MAX_NUM_KEY_PAIRS_FOR_RCDS 3   // number of key pairs returned for rc switch connect operation alarm
#if HMC_DS_PTC9_PULSE
#warning "Don't release with HMC_DS_PTC9_PULSE set!"
#endif

#define INCLUDE_SWITCH_ACTIVATION_COUNT 0
#define SUPPORTS_LOCKOUT_TIMER 0

/*lint -esym(750,DS_APP_RETRIES,DS_METER_STATUS_RETRIES,DS_RES_DO_NOT_ADVANCE_TABLE,DS_RES_NEXT_TABLE)   */
/*lint -esym(750,AC_LINE_SIDE,AC_LOAD_SIDE,CAP_V_OK,CAP_V_LOW,CAP_V_HIGH,AUTONOMOS_CMD,CLOSE_STATE,OPEN_STATE) */

#define STATUS_RETRY_PROC_mS ((uint32_t)1 * TIME_TICKS_PER_SEC)   /* ms to wait after a status table read retry */
#define LSV_RETRY_TIME ((uint32_t)10 * TIME_TICKS_PER_SEC)        /* ms between re-reads in LSV condition  */
#define LOW_CAP_V_TIME ((uint32_t)2 * TIME_TICKS_PER_SEC)         /* ms between re-reads when cap voltage is low   */

#define DS_APP_RETRIES                 ((uint8_t)2)   /* Application Level Retries */
/* Retries to read MT115 - based on worst case meter capacitor charge time of 180 seconds. */
#define DS_METER_STATUS_RETRIES        ((uint8_t)(((180 * TIME_TICKS_PER_SEC)/(LOW_CAP_V_TIME))+1))

#define DS_RES_DO_NOT_ADVANCE_TABLE    ((uint8_t)1)   /* Don't advance to the next table */
#define DS_RES_NEXT_TABLE              ((uint8_t)0)   /* Process the next table */

#define AUTONOMOS_CMD                  ((uint8_t)0)   /* Used as an indicator when an autonomos command is detected*/

#define CLOSE_STATE                    ((bool    )1)  /* Meter Switch state 1 = CLOSE, 0 = OPEN*/
#define OPEN_STATE                     ((bool    )0)  /* Meter Switch state 1 = CLOSE, 0 = OPEN*/

/* Frequency of the file IO update needed to select the class of memory to be used by the file IO driver  */
#define HMC_DS_FILE_UPDATE_RATE_SEC    (SECONDS_PER_DAY / 6)      /* Updates 6 times a day */

/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes (static)">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#if SUPPORTS_LOCKOUT_TIMER
static void             startOpCodeLockOutTmr( void );
static void             opcodeTmrCallBack_ISR( uint8_t , uint8_t const *);
#endif
static bool             canAcceptNewProcedure(void);
static void             updateSwitchStateCmdStatus(eSdCmdStatus_t);
static void             statusReadRetryCallBack_ISR( uint8_t , uint8_t const * );
static void             tx_FMR( HEEP_APPHDR_t const *appMsgInfo, enum_status methodStatus, EDEventType eventID, bool updateEventTime );
static returnStatus_t   HMC_DS_init( void );

#if TM_SD_UNIT_TEST
static void             printStats( const char *title, HMC_COM_INFO const *pData );
#endif

/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Static Variable Definitions">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static FileHandle_t        mtrDsFileHndl_;               /* Contains the file handle information. */
static mtrDsFileData_t     mtrDsFileData_;               /* File data for the service disconnect variables  */

static bool                bFeatureEn_ = (bool)FALSE;    /* SD feature may be enabled or disabled. Start as disabled */
static uint8_t             dsRetries_;                   /* Application Level Retries */
static bool                SaveISR_Data_ = (bool)FALSE;  /* Flag to save ISR modified data */
static cmdFnct_t const     *pCurrentCmd_ = NULL;         /* Ptr to command being executed.
                                                            pCurrentCmd_ is used also as a flag to indicate that the
                                                            applet is idle when NULL. */
static eSdCmdStatus_t      cmdStatus_ISR_;               /* Command status updated in an ISR. */
static eTableRetryStatus_t meterWaitStatus_ = eTableRetryNotWaiting;       /* Status of MT115 read - Switch status */
static uint8_t             meterStatusRetries_ = DS_METER_STATUS_RETRIES;  /* Retry counter for Read MT115 */

/* Last operation attempted to the meter. This is set after we send a command to the meter */
static uint8_t                   swStateBeforeCmd;       /* Records switch state prior to cmds send to meter   */
static LastOperationAtempt_t     lastOperationAttempted_ = ePowerUpState;
static EDEventType lastHEEPcmd_                          = (EDEventType )0;   /* Last command received from HE,
                                                                                 a non zero value indicates we are enabled and initialized */
static EDEventType swResult_                             = (EDEventType )0;      /* Last switch operation result  */
static HEEP_APPHDR_t             lastHEEPappHdr_;        /* Copy of the last accepted request from the HE   */
static uint32_t                  HEEPtime_;              /* Time stamp of most current request from HE */
static bool                      LSVsent = (bool)false;  /* True = Load side voltage message has been sent.   */
static uint16_t                  rcdTimer_id;            /* S/D switch in-progress timer  */
static RCDCStatus                lastMT117res = {0};     /* Last MT117 read result        */

#if SUPPORTS_LOCKOUT_TIMER
static bool                clearLockout_ = (bool)FALSE;  /* Update the lockout flag */
static uint16_t            lockTimer_id;                 /* S/D switch lockout timer      */
#endif

#if TM_SD_UNIT_TEST
static char prntBuf[256]; /* Buffer to store print statements for the UART */
#endif

#if ( HMC_I210_PLUS_C == 1 )
static uint8_t password[METER_PASSWORD_SIZE] = {0};     // used to store the reader access password
static uint8_t mPassword[METER_PASSWORD_SIZE] = {0};    // used to store the master access password
#endif

/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */
static eMeterType_t const        eMtrType_ = eDEFAULT_METER;

// <editor-fold defaultstate="collapsed" desc="mtrDsRegTbl: Define table that will be used for the object list entry">
/* Define the table that will be used for the object list entry. */
#if ( TWACS_REGISTERS_SUPPORTED == 1 )
static const OL_tTableDef mtrDsRegTbl_ =
{
   (tElement  *)(void *)&mtrDsRegDefTbl_[0].sRegProp.uiRegNumber, /* Points to the 1st element in the register table. */
   ARRAY_IDX_CNT(mtrDsRegDefTbl_)                                 /* Number of registers defined for the time module. */
};
#endif

#if ( HMC_I210_PLUS_C == 1 )
static const uint8_t SecurityStr[] = { CMD_PASSWORD };
static const HMC_PROTO_TableCmd Security[] =
{
   {
      HMC_PROTO_MEM_PGM, sizeof(SecurityStr), (uint8_t far *)SecurityStr
   },
   {
      HMC_PROTO_MEM_PGM, sizeof(password), password
   },
   {
      HMC_PROTO_MEM_NULL
   }  /*lint !e785 too few initializers   */
};

static const HMC_PROTO_TableCmd MasterSecurity[] =
{
   {
      HMC_PROTO_MEM_PGM, sizeof(SecurityStr), (uint8_t far *)SecurityStr
   },
   {
      HMC_PROTO_MEM_PGM, sizeof(mPassword), mPassword
   },
   {
      HMC_PROTO_MEM_NULL
   }  /*lint !e785 too few initializers   */
};

static const HMC_PROTO_Table SecurityTbl[] =
{
   {  Security },
   {  (void *)NULL }
};

static const HMC_PROTO_Table MasterSecurityTbl[] =
{
   {  MasterSecurity },
   {  (void *)NULL }
};
#endif

static const cmdFnct_t openSwitchTable_[] =
{
#if ( HMC_I210_PLUS_C == 1 )
   { (HMC_PROTO_Table  const *)&MasterSecurityTbl[0], NULL },
#endif
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchStateforOpen },
   { (HMC_PROTO_Table  const *)&tblSwOpen_[0],        (fptr_ds)procedureResultsSwitchCmd },
   { (HMC_PROTO_Table  const *)&readTblMT117[0],      (fptr_ds)ReadMT117Results },
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchState },
#if ( HMC_I210_PLUS_C == 1 )
   { (HMC_PROTO_Table  const *)&SecurityTbl[0], NULL },
#endif
   { NULL, NULL}
#elif HMC_FOCUS
   { (HMC_PROTO_Table  const *)&tblSwOpen_[0],        (fptr_ds)procedureResultsSwitchCmd },
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchStateforOpen },
#if 0 // TODO - fix when support for FOCUS needed
   { (HMC_PROTO_Table  const *)&readTblMT115[0],      (fptr_ds)ReadMT115Results },
   { &_Tbl_Reg1900Update[0], procResultsReg1900},
#endif
   { NULL, NULL}
#else
   { NULL, NULL}
#endif
};

static const cmdFnct_t closeSwitchTable_[] =
{  // Need to update 512 and then determine if its okay to close sw.
#if ( HMC_I210_PLUS_C == 1 )
   { (HMC_PROTO_Table  const *)&MasterSecurityTbl[0], NULL },
#endif
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchStateforClose },
   { (HMC_PROTO_Table  const *)&readTblMT117[0],      (fptr_ds)ReadMT117Results },
   { (HMC_PROTO_Table  const *)&tblSwClose_[0],       (fptr_ds)procedureResultsSwitchCmd },
   { (HMC_PROTO_Table  const *)&readTblMT117[0],      (fptr_ds)ReadMT117Results },
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchState },
#if ( HMC_I210_PLUS_C == 1 )
   { (HMC_PROTO_Table  const *)&SecurityTbl[0], NULL },
#endif
   { NULL, NULL}
#elif HMC_FOCUS
   { (HMC_PROTO_Table  const *)&tblSwClose_[0],       (fptr_ds)procedureResultsSwitchCmd },
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchStateforClose },
#if 0 // TODO - fix when support for FOCUS needed
   { (HMC_PROTO_Table  const *)&readTblMT117[0],      (fptr_ds)ReadMT117Results },
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchState },
#endif
   { NULL, NULL}
#else
   { NULL, NULL}
#endif
};

static const cmdFnct_t * const pOpenSwitchTables_[] =
{
#if HMC_I210_PLUS || HMC_FOCUS || HMC_I210_PLUS_C
   &openSwitchTable_[0]
#else
   NULL
#endif
};

static const cmdFnct_t * const pCloseSwitchTables_[] =
{
#if HMC_I210_PLUS || HMC_FOCUS || HMC_I210_PLUS_C
   &closeSwitchTable_[0]
#else
   NULL
#endif
};

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Function Definitions">
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t HMC_DS_init(void)">
/***********************************************************************************************************************
 *
 * Function name: HMC_DS_init
 *
 * Purpose: Adds registers to the Object list and opens files.  This must be called at power before any other function
 *          in this module is called and before the register module is initialized.
 *
 * Arguments: None
 *
 * Returns: returnStatus_t - result of file open.
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
static returnStatus_t HMC_DS_init( void )
{
   FileStatus_t status;
#if ( TWACS_REGISTERS_SUPPORTED == 1 )
   (void)OL_Add(OL_eREGISTER, sizeof(mtrDsRegDefTbl_[0]), &mtrDsRegTbl_); /* Add the reg to object list. */
#endif
#if 1
   (void)FIO_fopen(&mtrDsFileHndl_,                  /* File Handle */
                     ePART_SEARCH_BY_TIMING,          /* Search for the best paritition according to the timing. */
                     (uint16_t)eFN_MTR_DS_REGISTERS,  /* File ID (filename) */
                     (lCnt)sizeof(mtrDsFileData_t),   /* Size of the data in the file. */
                     0,                               /* File attributes to use. */
                     HMC_DS_FILE_UPDATE_RATE_SEC,     /* The update rate of the data in the file. */
                     &status);
   /* Read the entire structure  */
   return FIO_fread(&mtrDsFileHndl_, (uint8_t *)&mtrDsFileData_, 0, (lCnt)sizeof(mtrDsFileData_));
#else
   return eSUCCESS;
#endif
}
/* ****************************************************************************************************************** */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="uint8_t HMC_DS_Applet(uint8_t cmd, void far *pData)">
/***********************************************************************************************************************
 *
 * Function Name: HMC_DS_Applet
 *
 * Purpose: Sends commends to the host meter to open/close/arm the switch and update meter disconnect related registers.
 *
 * Arguments: uint8_t - Command, void *pData
 *
 * Returns: uint8_t - HMC_APP_API_RPLY (see mtr_app.h)
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 * Note: pCurrentCmd_ is used also as a flag to inicate that the applet is idle when NULL.
 *
 **********************************************************************************************************************/
uint8_t HMC_DS_Applet(uint8_t cmd, void far *pData)
{
   uint8_t retVal = (uint8_t) HMC_APP_API_RPLY_IDLE; /* Assume the module is Idle */

   if (SaveISR_Data_)  /* Do any flags need to be saved in the background ? */
   {
#if SUPPORTS_LOCKOUT_TIMER
      if (clearLockout_)  /* If the timer ISR has expired, clear the lockout flag in NV.*/
      {
#if TM_SD_UNIT_TEST
         DBG_printf( "Lock out expired\n" );
#endif
         mtrDsFileData_.dsInfo.OpCodeLockOut = FALSE;
         (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
         clearLockout_ = (bool)FALSE;
      }
      else if ( 0 == meterStatusRetries_) /* The procedure retries have expired, save the command error code to NV */
#endif
      {
         regStateStatus_u sdReg;                               /* Temporary storage for Switch state/status */
         (void)FIO_fread(&mtrDsFileHndl_, (uint8_t *)&sdReg, (uint16_t)offsetof(mtrDsFileData_t, dsStatusReg),
                        (lCnt)sizeof(sdReg));
         sdReg.bits.sdCmdStatus = (uint16_t)cmdStatus_ISR_;    /* Command Failed to Open or Close  */
         (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
         if( eSwFailedToClose == cmdStatus_ISR_ )              /* Switch failed to connect.   */
         {
            swResult_ = electricMeterRCDSwitchConnectFailed;
         }
         else                                                  /* Switch failed to disconnect.   */
         {
            swResult_ = electricMeterRCDSwitchDisconnectFailed;
         }

         /* Notify head end of failure */
         tx_FMR( &lastHEEPappHdr_, NotModified, swResult_, (bool)false );     /* Send a response to the head end   */

         pCurrentCmd_        = NULL; /* Done trying to read ST8*/
         meterStatusRetries_ = DS_METER_STATUS_RETRIES;
      }
      SaveISR_Data_ = (bool)FALSE;
   }
   switch (cmd) /* Decode the command sent to this applet */
   {
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ACTIVATE">
      case (uint8_t) HMC_APP_API_CMD_ACTIVATE: /* Prepare to update all registers IF this module is NOT active */
      {
         /* Is this module enabled and is the applet not active? */
         if (bFeatureEn_ && (NULL == pCurrentCmd_))
         {
            pCurrentCmd_        = pRegUpdateTables_[eMtrType_]; /* Get the current table information. */
            dsRetries_          = DS_APP_RETRIES; /* Setup the number of retries in case of an error. */
            meterStatusRetries_ = DS_METER_STATUS_RETRIES;/* Retry counter for the reading of MT115 - status  */
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_INIT">
      case (uint8_t) HMC_APP_API_CMD_INIT: /* Initialize the Applet */
      {
         /* Create the timers used by this applet  */
         timer_t   timerCfg;        /* Timer configuration */

         (void)HMC_DS_init();             /* Open/read the switch metadata */
         (void)memset(&timerCfg, 0, sizeof(timerCfg));  /* Clear the timer values */
         timerCfg.bOneShot       = true;
         timerCfg.bExpired       = true;
         timerCfg.bStopped       = true;
         timerCfg.ulDuration_mS  = STATUS_RETRY_PROC_mS;
         timerCfg.pFunctCallBack = (vFPtr_u8_pv)statusReadRetryCallBack_ISR;

         (void)TMR_AddTimer(&timerCfg);               /* Create the status update timer   */
         rcdTimer_id = timerCfg.usiTimerId;

#if SUPPORTS_LOCKOUT_TIMER
         /* On every powerup start the lockout timer again because the switch could be discharged due to an extended
            power outage.  This also updated mtrDsFileData_ on every power up*/
         timerCfg.bStopped = false;

         /* Set time-out, converting seconds to system time ticks per scond  */
         timerCfg.ulDuration_mS  = chargeTimeSec_[eMtrType_].chargeTimeSec * (uint32_t)TIME_TICKS_PER_SEC;
         timerCfg.usiTimerId     = 0;
         timerCfg.pFunctCallBack = (vFPtr_u8_pv)opcodeTmrCallBack_ISR;
         (void)TMR_AddTimer(&timerCfg);               /* Create the lock-out timer  */
         lockTimer_id            = timerCfg.usiTimerId;
#endif

         meterWaitStatus_        = eTableRetryNotWaiting;   /* The procedure status can be cleared at power up. */
         meterStatusRetries_     = DS_METER_STATUS_RETRIES; /* Retry counter for the reading of MT115 - status  */
         lastOperationAttempted_ = ePowerUpState;           /* The Last operation attempted has not been set */
         pCurrentCmd_            = NULL;                    /* Initialize, No commands are being performed. */

#if SUPPORTS_LOCKOUT_TIMER
         /* Upon power up, if the hardware exists (remembered from previous power on), start the lock out timer   */
         if( 1 == mtrDsFileData_.dsInfo.hwExists ) /* Does switch exist?   */
         {
            startOpCodeLockOutTmr();
         }
#endif
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_STATUS">
      case (uint8_t) HMC_APP_API_CMD_STATUS: /* Check if meter communication is required */
      {
         if (bFeatureEn_) /* Only check on status if the feature is enabled. */
         {
            /* Is the pointer valid (pointing to a table)? and not waiting on a retry  */
            if ((NULL != pCurrentCmd_) && (eTableRetryNotWaiting == meterWaitStatus_))
            {
               retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
            else if (eTableRetryTmrExpired == meterWaitStatus_) /* Are we ready for a re-read of MT115? */
            {
               /* Re-read the MT115 status table. */
               pCurrentCmd_     = &regUpdateTable_[0];   /*Set the function pointer to the status table */
               meterWaitStatus_ = eTableRetryNotWaiting;

               retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_PROCESS">
      case (uint8_t) HMC_APP_API_CMD_PROCESS: /* Status must have been ready for comm, now load the pointer. */
      {
         if (bFeatureEn_ && (NULL != pCurrentCmd_)) /* Verify, is applet enabled and pointing to a table? */
         {
            retVal = (uint8_t)HMC_APP_API_RPLY_PAUSE; /* assume applet is in a wait state - set to pause meter app. */

            if (eTableRetryNotWaiting == meterWaitStatus_) /* waiting? to re read MT115 */
            {
               retVal                                  = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST; /* return ready! */
               //lint -e{413} pData will not be NULL here
               ((HMC_COM_INFO *) pData)->pCommandTable = (uint8_t far *)pCurrentCmd_->pCmd;         /* Set up the pointer. */
            }
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD - Any Error:  Fall Through to Complete. ">
      case (uint8_t) HMC_APP_API_CMD_TBL_ERROR: /* The meter reported back a table error, set a diags flag */
      case (uint8_t) HMC_APP_API_CMD_ABORT: /* Comm Error, the session was aborted.  */
      case (uint8_t) HMC_APP_API_CMD_ERROR: /* Comm Error, the session was aborted.  */
      {
#if HMC_DS_PTC9_PULSE
         GPIOC_PTOR |= 1<<9;  /* Toggle output  */
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         asm( "nop" );
         GPIOC_PTOR |= 1<<9;  /* Toggle output  */
#endif
#if TM_SD_UNIT_TEST
#if VERBOSE>=1
         uint16_t  msglen = 0;             /* Length of message being built */

         /* Pointer to table 8 procedure results */
         HMC_COM_INFO      *p        = (HMC_COM_INFO *)pData;
         tTbl8ProcResponse *pProcRes = (tTbl8ProcResponse *)(&p->RxPacket.uRxData.sTblData.sRxBuffer[0]);
         tTbl8ProcResponse  response;

         /* Copy the response to the local struct  */
         response = *(tTbl8ProcResponse *)p->RxPacket.uRxData.sTblData.sRxBuffer;
         /* The procedure number must be byte swapped. Can't find an appropriate
            structure definition to make this happen implicitly.  */
         *(uint16_t *)&response = BIG_ENDIAN_16BIT((*(uint16_t *)(void *)&response));

         msglen +=  (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "HMC_DS API command error.\n" );
         msglen +=  (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf),
                              "HMC_APP_API_CMD_TBL_ERROR: cmd = %d\n", cmd );
         msglen +=  (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf),
                              "Response code: %d\n", p->RxPacket.ucResponseCode );
         for ( uint8_t i = 0; i < sizeof(tTbl8ProcResponse); i++ )
         {
            msglen +=  (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "%02x",
                                 p->RxPacket.uRxData.sTblData.sRxBuffer[i] );
         }
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "\ntblProcNbr: %d\n",
                              (response.tblProcNbr) );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "mfgFlag: %d\n",
                              pProcRes->mfgFlag );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "selector: 0x%x\n",
                              pProcRes->selector );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "seqNbr: 0x%02x\n",
                              pProcRes->seqNbr );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "resultCode: 0x%02x\n",
                              pProcRes->resultCode );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "respDataRcd: 0x%02x\n",
                              pProcRes->respDataRcd );
         DBG_printf( prntBuf );
#endif
#endif
         if (0 == (--dsRetries_) )  //Retry if HMC_APP_API_CMD_TBL_ERROR, HMC_APP_API_CMD_ABORT or HMC_APP_API_CMD_ERROR
         {
            pCurrentCmd_++;      /* Skip to next command in sequence.   *//*lint !e613 NULL ptr OK */
            /* Is the next item in the table NULL?  If so, that is all, we're all done.  */
            /* Handler may have aborted, so check pCurrentCmd_ first!   */
            if ( ( NULL != pCurrentCmd_ ) && ( NULL == pCurrentCmd_->pCmd) )
            {
               pCurrentCmd_     = NULL;
               meterWaitStatus_ = eTableRetryNotWaiting; /* Set indicator flag to not waiting. */
            }
         }
         break;
      }
      // </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_MSG_COMPLETE">
      case (uint8_t) HMC_APP_API_CMD_MSG_COMPLETE: /* Communication complete, no errors and data has been saved! */
      {
         if (NULL != pCurrentCmd_)  /* Do not do anything if we get here with a NULL in pCurrentCmd_ */
         {
            if (NULL != pCurrentCmd_->fpFunct)  /* Is there a handler for this?  */
            {
               /* Call the local handler and check the results */
               if (DS_RES_NEXT_TABLE == pCurrentCmd_->fpFunct(cmd, (sMtrRxPacket *)pData ))
               {
                  pCurrentCmd_++;
               }
            }
            else
            {
               /* No handler, just bump to next command. */
               pCurrentCmd_++;
            }
            /* Is the next item in the table NULL?  If so, that is all, we're all done.  */
            /* Handler may have aborted, so check pCurrentCmd_ first!   */
            if ( ( NULL != pCurrentCmd_ ) && ( NULL == pCurrentCmd_->pCmd) )
            {
               pCurrentCmd_     = NULL;
               meterWaitStatus_ = eTableRetryNotWaiting; /* Set indicator flag to not waiting. */
            }
            else
            {
               retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
          }
         break;
      }
      // </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ENABLE_MODULE">
      case (uint8_t) HMC_APP_API_CMD_ENABLE_MODULE: /* Comnmand to enable this applet */
      {  /* Switch state/status needs to be updated every time a meter id(reg 2243)read shows that a meter with a switch
            has been detected */
         dsMetaData hwInfo;
         bFeatureEn_ = (bool)TRUE;  /* Enable the applet */
#if TM_SD_UNIT_TEST
         assert(NULL == pCurrentCmd_);
#endif
         /* Record the fact that switch h/w exists   */
         (void)FIO_fread(&mtrDsFileHndl_, (uint8_t *)&hwInfo, (uint16_t)offsetof(mtrDsFileData_t, dsInfo), (lCnt)sizeof(hwInfo));
         hwInfo.hwExists       = 1;
         mtrDsFileData_.dsInfo = hwInfo;
         (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
         pCurrentCmd_        = pRegUpdateTables_[eMtrType_]; /* Set tables needed to update switch state/status. No need to
                                                                wait for INIT state*/
         dsRetries_          = DS_APP_RETRIES; /* Setup the number of table read retries in case of an error. */
         meterStatusRetries_ = DS_METER_STATUS_RETRIES;/* Retry counter for the reading of MT115 - status  */
#if HMC_DS_PTC9_PULSE
         PORTC_PCR9  = 0x100; /* Make GPIO      */
         GPIOC_PCOR  = 1<<9;  /* Set output low */
         GPIOC_PDDR |= 1<<9;  /* Make output    */
#endif
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_DISABLE_MODULE">
      case (uint8_t) HMC_APP_API_CMD_DISABLE_MODULE: /* Comnmand to disable this applet? */
      {
         dsMetaData hwInfo;
         bFeatureEn_ = (bool)FALSE; /* Disable the applet */
         lastHEEPcmd_ = (EDEventType)0;

         /* Record the fact that switch h/w doesn't exist   */
         (void)FIO_fread(&mtrDsFileHndl_, (uint8_t *)&hwInfo,
                           (uint16_t)offsetof(mtrDsFileData_t, dsInfo), (lCnt)sizeof(hwInfo));
         hwInfo.hwExists       = 0;
         mtrDsFileData_.dsInfo = hwInfo;
         (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="Default">
      default: // No other commands are supported in this applet
      {
         retVal = (uint8_t) HMC_APP_API_RPLY_UNSUPPORTED_CMD;
         break;
      }// </editor-fold>
   }
   return (retVal);
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static uint8_t procedureResultsSwitchCmd()">
/***********************************************************************************************************************
 *
 * Function Name: procedureResultsSwitchCmd
 *
 * Purpose: Processes the results of the procedure to open or close the switch.  These results are in ST8.
 *
 * Arguments:
 *             HMC_COM_INFO *pData - Results of the communication with the meter
 *             uint8_t cmd           -  API commands, HMC_APP_API_CMD_PROCESS, HMC_APP_API_CMD_MSG_COMPLETE etc.
 *
 * Returns: uint8_t:  DS_RES_DO_NOT_ADVANCE_TABLE or DS_RES_NEXT_TABLE
 *
 * Side Effects: none
 *
 * Reentrant Code: no
 *
 **********************************************************************************************************************/
static uint8_t procedureResultsSwitchCmd( uint8_t cmd, HMC_COM_INFO const *pData )
{
   uint8_t retVal = DS_RES_DO_NOT_ADVANCE_TABLE;           /* This will retry the current command. */

   /* Pointer to table 8 procedure results */
   tTbl8ProcResponse *pProcRes = (tTbl8ProcResponse *)&pData->RxPacket.uRxData.sTblData.sRxBuffer[0]; /*lint !e2445 !e826 cast increases alignment req, area too small   */

   /* The command has to be Success to process the data and the procedure results have to be good. */
   if (((uint8_t)HMC_APP_API_CMD_MSG_COMPLETE == cmd) && (PROC_SEQ_NUM == pProcRes->seqNbr))
   {
#if TM_SD_UNIT_TEST
#if VERBOSE>1 // mkv - debug/test code
      uint16_t  msglen;              /* Length of message being built */
      tTbl8ProcResponse response;

      /* Copy the response to the local struct  */
      response = *(tTbl8ProcResponse *)pData->RxPacket.uRxData.sTblData.sRxBuffer;
      /* The procedure number must be byte swapped. Can't find an appropriate
         structure definition to make this happen implicitly.  */
      *(uint16_t *)&response = BIG_ENDIAN_16BIT(*(uint16_t *)&response);

      msglen =  (uint16_t)snprintf(prntBuf, sizeof(prntBuf), "procResCmd: 0x%02x",
                           pData->RxPacket.uRxData.sTblData.sRxBuffer[0] );
      for ( uint8_t i = 1; i < sizeof(tTbl8ProcResponse); i++ )
      {
         msglen +=  (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "%02x",
                              pData->RxPacket.uRxData.sTblData.sRxBuffer[i] );
      }
      msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "\ntblProcNbr: %d\n",
                           (response.tblProcNbr) );
      msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "mfgFlag: %d\n",
                           pProcRes->mfgFlag );
      msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "selector: 0x%x\n",
                           pProcRes->selector );
      msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "seqNbr: 0x%02x\n",
                           pProcRes->seqNbr );
      msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "resultCode: 0x%02x\n",
                           pProcRes->resultCode );
      msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "respDataRcd: 0x%02x\n",
                           pProcRes->respDataRcd );
      DBG_printf( prntBuf );
#endif
#endif
      if (  (PROC_RESULT_COMPLETED == pProcRes->resultCode)   ||        /* Accepted and Completed  */
            (PROC_RESULT_NOT_COMPLETED == pProcRes->resultCode)  ||     /* Accepted and in process */

            (  (mtrDsFileData_.dsStatusReg.bits.swClosed == SWITCH_STATE_OPEN  ) && /* Switch is currently open   */
               (mtrDsFileData_.dsStatusReg.bits.lastOperationAttempted == 0) &&     /* Last command was close     */
               (lastMT117res.acot != 0 || lastMT117res.exac != 0 ) ) )              /* Load side voltage detected */
      {
         regStateStatus_u sdReg;                            /* Temporary storage for switch state/status */
         (void)FIO_fread(&mtrDsFileHndl_, (uint8_t *)&sdReg, (uint16_t)offsetof(mtrDsFileData_t, dsStatusReg),
                        (lCnt)sizeof(sdReg));
         sdReg.bits.sdCmdStatus = (uint16_t)eCmdAccepted;   /* Command Accepted */
         sdReg.bits.lastOperationAttempted = (uint16_t)lastOperationAttempted_;  /* 0 = Close, 1 = OPEN Note that this
                                                                                    is inverted logic compared to Switch
                                                                                    Status field */
         (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
         retVal = DS_RES_NEXT_TABLE;
      }
      else
      {
         /* Was there a setup or configuration error with the procedure?  If so, retries will not do any good. */
         if (0 == meterStatusRetries_)  /* MT115 Table  read retries have expired */
         {
            pCurrentCmd_ = NULL;
         }
      }
   }
   else if (0 == (--dsRetries_))//Retry if HMC_APP_API_CMD_TBL_ERROR, HMC_APP_API_CMD_ABORT or HMC_APP_API_CMD_ERROR
   {
      pCurrentCmd_ = NULL;
   }
   else
   {
      retVal = DS_RES_DO_NOT_ADVANCE_TABLE;
   }
   return (retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static uint8_t processResultsSwitchStateforClose()">
/***********************************************************************************************************************
 *
 * Function Name: processResultsSwitchStateforClose
 *
 * Purpose: Process the meter response to the Close command and populate Switch state/status to determine if the switch
 *          can close
 *
 * Arguments:  uint8_t cmd - command status from meter
 *             HMC_COM_INFO *pData - Results of the communication with the meter (MT115)
 *
 * Returns: bool    :  0 = Do NOT retry, 1 = Retry the procedure
 *
 * Side Effects:
 *
 * Reentrant Code:
 *
 **********************************************************************************************************************/
//lint -e{715} suppress "pData not referenced"
static uint8_t processResultsSwitchStateforClose( uint8_t cmd, HMC_COM_INFO const *pData)
{
   uint8_t retVal = DS_RES_DO_NOT_ADVANCE_TABLE;
   swResult_      = (EDEventType )0;

   if ( (uint8_t)HMC_APP_API_CMD_MSG_COMPLETE == cmd )   /* The command has to be Success to process the data. */
   {
      retVal = DS_RES_NEXT_TABLE;
      meterWaitStatus_ = eTableRetryNotWaiting; /* Set indicator flag to not waiting. */
      swStateBeforeCmd =
         ((ldCtrlStatusTblGPlus_t *)&pData->RxPacket.uRxData.sTblData.sRxBuffer[0])->rcdcState.actual_switch_state;  /*lint !e2445 !e826 cast increases alignment req, area too small   */

#if TM_SD_UNIT_TEST
      printStats( "procResClose(MT115)", pData );
#endif

   }
   else if ((0 == dsRetries_) || (0 == --dsRetries_))
   {
      dsRetries_--;  //Retry if HMC_APP_API_CMD_TBL_ERROR, HMC_APP_API_CMD_ABORT or HMC_APP_API_CMD_ERROR
   }
   else
   {
      pCurrentCmd_ = NULL;
   }
   return (retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static uint8_t processResultsSwitchStateforOpen()">
/***********************************************************************************************************************
 *
 * Function Name: processResultsSwitchStateforOpen
 *
 * Purpose: Process the meter response to the Open command and populate Switch state/status to determine if the switch
 *          can operate
 *
 * Arguments: uint8_t cmd         - API command performed
 *            HMC_COM_INFO *pData - Results of the communication with the meter (MT115)
 *
 * Returns: uint8_t:  0 = DS_RES_NEXT_TABLE(Retry the procedure), 1 = DS_RES_DO_NOT_ADVANCE_TABLE (Do NOT retry)
 *
 * Side Effects: Updates Switch state/status
 *
 * Reentrant Code:
 *
 **********************************************************************************************************************/
//lint -e{715} // pData not referenced - needed for generic API
static uint8_t processResultsSwitchStateforOpen( uint8_t cmd, HMC_COM_INFO const *pData )
{
   uint8_t retVal = DS_RES_DO_NOT_ADVANCE_TABLE;
   swResult_      = (EDEventType )0;

   if ( (uint8_t)HMC_APP_API_CMD_MSG_COMPLETE == cmd )   /* The command has to be Success to process the data. */
   {
      retVal = DS_RES_NEXT_TABLE;
      meterWaitStatus_ = eTableRetryNotWaiting;          /* Set indicator flag to not waiting. */
      swStateBeforeCmd =
         ((ldCtrlStatusTblGPlus_t *)&pData->RxPacket.uRxData.sTblData.sRxBuffer[0])->rcdcState.actual_switch_state;  /*lint !e2445 !e826 cast increases alignment req, area too small   */
#if TM_SD_UNIT_TEST
      printStats( "procResOpen(MT115)", pData );
#endif

   }
   else if ((0 == dsRetries_) || (0 == --dsRetries_))
   {
      dsRetries_--;  //Retry if HMC_APP_API_CMD_TBL_ERROR, HMC_APP_API_CMD_ABORT or HMC_APP_API_CMD_ERROR
   }
   else
   {
      pCurrentCmd_ = NULL;
   }
   return (retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static uint8_t processResultsSwitchState()">
/***********************************************************************************************************************
 *
 * Function Name: processResultsSwitchState
 *
 * Purpose: Process the meter responses that are common to both the Open and Close commands and populate Switch
 *          state/status.
 *          This is the last function called as a result of a switch open/close request. It reports the results to the
 *          head end.
 *
 * Arguments:  uint8_t cmd          - API command performed
 *             HMC_COM_INFO *pData  - Results contining the response of the meter (MT115)
 *
 * Returns: uint8_t:  DS_RES_NEXT_TABLE(0) = Do NOT retry, DS_RES_DO_NOT_ADVANCE_TABLE(1) = Retry the procedure
 *
 * Side Effects:
 *
 * Reentrant Code:
 *
 **********************************************************************************************************************/
static uint8_t processResultsSwitchState( uint8_t cmd, HMC_COM_INFO const *pData )
{
   uint8_t           retVal = (uint8_t)DS_RES_NEXT_TABLE;
   bool              updateTime = (bool)true;   /* Assume switch action occurred    */
   bool              sendReply = (bool)false;   /* Assume no message to head end needed   */
   bool              sendAlarm = (bool)false;   /* Assume no alarm to be logged  */
   mtrDsFileData_t   fileCopy;                  /* local copy of the file data   */
   enum_status       methodStatus = OK;         /* Command Response Codes - Assume operation succeeded */
   EventKeyValuePair_s  keyVal[MAX_NUM_KEY_PAIRS_FOR_RCDS];   // Event key_value pair info
   EventData_s          swEvent;                              // Event info
   swEvent.markSent = (bool)false;
   ValueInfo_t reading;

   (void)memset( (uint8_t *)keyVal, 0, sizeof(keyVal)); // initialize the key value pair array
   if ( (uint8_t)HMC_APP_API_CMD_MSG_COMPLETE == cmd )   /* The command has to be Success to process the data. */
   {
      (void) FIO_fread(&mtrDsFileHndl_, (uint8_t *) &mtrDsFileData_, 0, (lCnt)sizeof (mtrDsFileData_));
      fileCopy = mtrDsFileData_;
      if (eGPLUS_RD == eMtrType_)   /*lint !e506 !e774 eMtrType_ may not be constant in future project   */
      {
         ldCtrlStatusTblGPlus_t *pCtrlStat = (ldCtrlStatusTblGPlus_t *)&pData->RxPacket.uRxData.sTblData.sRxBuffer[0];  /*lint !e2445 !e826 cast increases alignment req, area too small   */
         /* If the actual desired states are not equal, the meter is in the middle of an operation, set a timer to
            re-read MT115
          * Note: MT115 Actual and desired switch states are 0 = OPEN, 1 = CLOSE */
         /* MT117 data is more current than MT115, but often incorrect - DON'T use MT117 to determine actual state of
          * switch.
          *
          * Actual == desired? */
         if ( pCtrlStat->rcdcState.actual_switch_state != pCtrlStat->rcdcState.desired_switch_state ||
              pCtrlStat->rcdcState.actual_switch_state != !lastOperationAttempted_ )
         {  /* Not in desired state. Check for trying to close in presence of LSV.  */
            if ( ( SWITCH_STATE_CLOSED == !lastOperationAttempted_ )  && /* Want switch closed?  */
               ( lastMT117res.acot != 0 || pCtrlStat->status.bypassed != 0 || pCtrlStat->status.alternate_source ) )        /* Load side voltage detected?      */
            {
               /* Reset the retry counter in case of LSV - never time out! */
               meterStatusRetries_  = DS_METER_STATUS_RETRIES;
               assert( eSUCCESS == TMR_ResetTimer( rcdTimer_id, LSV_RETRY_TIME ) );
               if ( !LSVsent )
               {
                  LSVsent = (bool)true;
                  sendReply = (bool)true;
                  mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eCmdRejectedLsv;     /* Command failed   */
                  mtrDsFileData_.dsStatusReg.bits.lastOperationAttempted = 0; /* Last operation attempted was close  */
                  mtrDsFileData_.dsInfo.OpCodeProcessed = false; /* No command in progress, now  */
                  mtrDsFileData_.dsStatusReg.bits.swClosed = pCtrlStat->rcdcState.actual_switch_state;
                  methodStatus = Conflict;
                  swResult_ = electricMeterRCDSwitchConnectFailed;
#if TM_SD_UNIT_TEST
                  DBG_printf( "LSV\n" );
#endif
               }
            }
#if 0
            else if ( ( pCtrlStat->lastCommandStatus & 1 ) != 0 )    /* Last operation failed? */
            {
               if ( pCtrlStat->rcdcState.desired_switch_state == SWITCH_STATE_OPEN )
               {  /* Desired state is OPEN; operation failed   */
                  mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eSwFailedToOpen;
                  swResult_ = electricMeterRCDSwitchDisconnectFailed;
               }
               else
               {  /* Desired state is CLOSED; operation failed   */
                  mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eSwFailedToClose;
                  swResult_ = electricMeterRCDSwitchConnectFailed;
               }
               methodStatus = NotModified;
               sendReply = (bool)true;
            }
#endif
            else  /* Switch Operation not complete, check next reason */
            {
               retVal = DS_RES_DO_NOT_ADVANCE_TABLE;
               sendReply = (bool)true;
               /* Check for fatal errors  */
               if (lastMT117res.acot || pCtrlStat->status.alternate_source )
               {
                  mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eCmdRejectedLsv;
                  swResult_ = electricMeterRCDSwitchConnectFailed;
                  methodStatus = ServiceUnavailable;
                  pCurrentCmd_ = NULL;
               }
               else if (pCtrlStat->status.switch_failed_to_close)
               {
                  mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eSwFailedToClose;
                  swResult_ = electricMeterRCDSwitchConnectFailed;
                  pCurrentCmd_ = NULL;
               }
               else if (pCtrlStat->status.switch_failed_to_open)
               {
                  mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eSwFailedToOpen;
                  swResult_ = electricMeterRCDSwitchDisconnectFailed;
                  pCurrentCmd_ = NULL;
               }
               else
               {  /* Non-fatal - just not completed, yet */
                  sendReply = (bool)false;
                  uint32_t timerValue = STATUS_RETRY_PROC_mS;
                  retVal = DS_RES_NEXT_TABLE;
                  if ( lastHEEPcmd_ != (EDEventType)0 )  /* Not just starting up?  */
                  {
                     if ( lastMT117res.ccst != CAP_V_OK )            /* Low cap voltage detected?  */
                     {
                        timerValue = LOW_CAP_V_TIME;
                     }
                     else
                     {  /* No need to update the switch status while cap voltage is low   */
                        mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eCmdInProcess;  /* Still In progress */
                        meterWaitStatus_ = eTableRetryWaiting;/* Set indicator flag to waiting . This should be cleared
                                                                 after timer expires */
                        /* For Switch state/status: LAST OPERATION (0 = CLOSE, 1 = OPEN) and
                                        MT115 desired_switch_state (1 = CLOSE, 0 = OPEN)   */
                        /* Copy MT115 to Switch state/status */
                        mtrDsFileData_.dsStatusReg.bits.lastOperationAttempted =
                                    !pCtrlStat->rcdcState.desired_switch_state;
                     }
                     assert( eSUCCESS == TMR_ResetTimer( rcdTimer_id, timerValue  ) );
                  }
                  else if( pCtrlStat->rcdcState.outage_open_in_effect && ePowerUpState == lastOperationAttempted_ )
                  {  // we just powered up and the disconnect is outage management mode, send an alarm
                     swEvent.eventId = (uint16_t)electricMeterPowerStatusDisconnected;
                     swEvent.eventKeyValuePairsCount = 1;
                     swEvent.eventKeyValueSize = sizeof(uint16_t);
                     *( uint16_t * )keyVal[0].Key = (uint16_t)switchPositionCount;  /*lint !e2445 case increases alignment requirement */
#if ( HMC_I210_PLUS_C == 1 )
                     *( uint16_t * )keyVal[0].Value = (uint16_t)(pCtrlStat->rcdc_switch_count / 2);   /*lint !e2445 case increases alignment requirement */
#else
                     *( uint16_t * )keyVal[0].Value = mtrDsFileData_.sdSwActCntr;
#endif
                     (void)EVL_LogEvent( (uint8_t)110, &swEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
                  }
               }
            }
         }
         else
         {  /* Switch is in desired position */
            /* Check for switch closing due to removal of LSV. If so, this is an alarm condition.  */
            if(   ( SWITCH_STATE_CLOSED == pCtrlStat->rcdcState.actual_switch_state ) &&
                  ( mtrDsFileData_.dsStatusReg.bits.sdCmdStatus == (uint16_t)eCmdRejectedLsv ) )
            {
                sendAlarm = (bool)true;
            }

            LSVsent = (bool)false;
            sendReply = (bool)true;

            /* Update Switch state/status */
            mtrDsFileData_.dsStatusReg.bits.sdCmdStatus = (uint16_t)eCmdAccepted;

            /* Note: Opposite logic in actual_switch_state (1 = CLOSE, 0 = OPEN) and
                                    lastOperationAttempted (0 = CLOSE, 1 = OPEN)
               Is the last OperationAttempted by OpCode equal to the actual_switch_state or first power up ?
               MT115 ActualSwitchState: (1 = CLOSE, 0 = OPEN)
            */
            if((  ( SWITCH_STATE_CLOSED == pCtrlStat->rcdcState.actual_switch_state ) &&
                  ( electricMeterRCDSwitchConnect == mtrDsFileData_.OpCodeCmd )           ) ||
               (  ( SWITCH_STATE_OPEN == pCtrlStat->rcdcState.actual_switch_state ) &&
                  ( electricMeterRCDSwitchDisconnect == mtrDsFileData_.OpCodeCmd  )       ) ||
                  ( ePowerUpState == lastOperationAttempted_) ) //Set the lastOperationSource to AMR if just powered up
            {
               /* Did the switch move? Check the state of the switch recorded before the command was sent.  */
               if ( swStateBeforeCmd != pCtrlStat->rcdcState.actual_switch_state )
               {
                  /* Update "open" counter, if appropriate. */
                  /* Only need to look at current state. If open, it just opened. */
                  if ( SWITCH_STATE_OPEN == pCtrlStat->rcdcState.actual_switch_state )
                  {
                     mtrDsFileData_.sdSwActCntr++;  /* Increment Switch Actuation Counter */
                  }
#if SUPPORTS_LOCKOUT_TIMER
                  /* Switch actually changed position, start the lockout timer   */
                  startOpCodeLockOutTmr();
#endif
               }
               else
               {
                  /* Operation "succeeded", but no change in the switch position required */
                  updateTime = (bool)false;  /* Preserve last "event" time */
               }
               mtrDsFileData_.dsStatusReg.bits.lastOperationSource = (uint16_t)eCmdAmr;   /* AMR made last request   */

               /* Operation succeeded, set swResult_ for reporting to HE   */
               if (electricMeterRCDSwitchConnect == mtrDsFileData_.OpCodeCmd)
               {
                  swResult_ = electricMeterRcdSwitchConnected ;
               }
               else if (electricMeterRCDSwitchDisconnect == mtrDsFileData_.OpCodeCmd)
               {
                  swResult_ = electricMeterRcdSwitchDisconnected ;
               }
               else  /* In case mtrDsFileData_.OpCodeCmd in an unknown (corrupted) state  */
               {
#if TM_SD_UNIT_TEST
#if VERBOSE>=1
                  DBG_printf( "OpCodeCmd value not recognized: %02x\n", mtrDsFileData_.OpCodeCmd);
#endif
#endif
                  if ( SWITCH_STATE_OPEN == pCtrlStat->rcdcState.actual_switch_state )
                  {
                     swResult_ =  electricMeterRcdSwitchDisconnected;
                     mtrDsFileData_.OpCodeCmd = electricMeterRCDSwitchDisconnect;
                  }
                  else
                  {
                     swResult_ =  electricMeterRcdSwitchConnected;
                     mtrDsFileData_.OpCodeCmd = electricMeterRCDSwitchConnect;
                  }
               }
            }
            else
            {
               updateTime   = (bool)false;
               methodStatus = NotModified;
               swResult_    = ( (electricMeterRCDSwitchConnect == mtrDsFileData_.OpCodeCmd) ?
                                 electricMeterRCDSwitchConnectFailed :     /* Execution failed to close.   */
                                 electricMeterRCDSwitchDisconnectFailed ); /* Execution failed to open  */

               /* Meter Soft Fuse or an optical probe must have changed it */
               mtrDsFileData_.dsStatusReg.bits.lastOperationSource = (uint16_t)eAutonomous;
               /* Set to flag to prevent incorrectly setting an AMR source during repeated optical commands */
               mtrDsFileData_.OpCodeCmd = (EDEventType)AUTONOMOS_CMD;
            }
            /* Update the switch state with acutal switch state from the meter reguardless of the last operation source
               For Switch state/status: STATE (1 = CLOSE, 0 = OPEN)  for
                     MT115 ActualSwitchState: (1 = CLOSE, 0 = OPEN)  */
            mtrDsFileData_.dsStatusReg.bits.swClosed = pCtrlStat->rcdcState.actual_switch_state;
         }
         /* Any updates made to file?   */
         /* memcmp works because these structs are packed   */
         if( 0 != memcmp( (uint8_t*)&fileCopy, (uint8_t*)&mtrDsFileData_, sizeof(fileCopy) ) )
         {
            (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
         }
      }
   }
   else if ((0 == dsRetries_) || (0 == --dsRetries_))
   {
      /* Application retries have timed out and failed   */
      pCurrentCmd_ = NULL;
      retVal       = DS_RES_DO_NOT_ADVANCE_TABLE;
      sendReply    = (bool)true;
      updateTime   = (bool)false;
      methodStatus = NotModified;
      swResult_    = ( (electricMeterRCDSwitchConnect == mtrDsFileData_.OpCodeCmd) ?
                        electricMeterRCDSwitchConnectFailed :     /* Execution failed to close.   */
                        electricMeterRCDSwitchDisconnectFailed ); /* Execution failed to open  */
   }

#if TM_SD_UNIT_TEST
   printStats( "procResStat(MT115)", pData );
#endif

   /* Check for need to send a replay AND there is a valid pending request from the head end.   */
   if ( sendAlarm )
   {
      // initialize the logged information for the switch connected event
      swEvent.eventId = (uint16_t)electricMeterRcdSwitchConnected;
      swEvent.eventKeyValuePairsCount = 0;
      swEvent.eventKeyValueSize = 0;

      if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading(switchPositionStatus, &reading) )
      {  // acquired switch position status
         *( uint16_t * )keyVal[swEvent.eventKeyValuePairsCount].Key = (uint16_t)reading.readingType; /*lint !e2445 case increases alignment requirement */
         *( uint8_t * )keyVal[swEvent.eventKeyValuePairsCount].Value = (uint8_t)reading.Value.uintValue;
         swEvent.eventKeyValuePairsCount++;
      }
      if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading(energizationLoadSide, &reading) )
      {  // acquired energization load side status
         *( uint16_t * )keyVal[swEvent.eventKeyValuePairsCount].Key = (uint16_t)reading.readingType; /*lint !e2445 case increases alignment requirement */
         *( uint8_t * )keyVal[swEvent.eventKeyValuePairsCount].Value = (uint8_t)reading.Value.uintValue;
         swEvent.eventKeyValuePairsCount++;
      }

      if( swEvent.eventKeyValuePairsCount > 0 )
      {  // nvp values are included in the event, update the nvp value size
         swEvent.eventKeyValueSize = sizeof(uint8_t);
      }

      // log the event
      (void)EVL_LogEvent( (uint8_t)250, &swEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );

      sendAlarm = (bool)false;
   }
   else if ( sendReply && ( lastHEEPcmd_ != (EDEventType)0 ) )
   {  /*last HEEP cmd will be zero when we have enabled the applet and run though pRegUpdateTables_*/
      tx_FMR( &lastHEEPappHdr_, methodStatus, swResult_, updateTime );     /* Send a response to the head end   */

   }
   return (retVal);  /*lint !e438 last value assigned to retVal not used - OK */
}
// <editor-fold defaultstate="collapsed" desc="static uint8_t ReadMT117Results()">
/***********************************************************************************************************************
 *
 * Function Name: ReadMT117Results
 *
 * Purpose: Called at the completion of reading MT117
 *
 * Arguments: sMtrRxPacket *pData - Results contining the response of the meter
 *
 * Returns: uint8_t:  DS_RES_NEXT_TABLE(0)
 *
 * Side Effects:
 *
 * Reentrant Code:
 *
 **********************************************************************************************************************/
static uint8_t ReadMT117Results( uint8_t cmd, HMC_COM_INFO const *pData )
{
   uint8_t     retVal = (uint8_t)DS_RES_NEXT_TABLE;
   pRCDCStatus pStatus = (pRCDCStatus)&pData->RxPacket.uRxData.sTblData.sRxBuffer[0];  /*lint !e826 area too small   */
   lastMT117res = *pStatus;   /* Copy the results to a local variable   */

   if ( (uint8_t)HMC_APP_API_CMD_MSG_COMPLETE == cmd )   /* The command has to be Success to process the data. */
   {
      (void) FIO_fread(&mtrDsFileHndl_, (uint8_t *) &mtrDsFileData_, 0, (lCnt)sizeof (mtrDsFileData_));
      /* Check to see if switch position has changed since last time endpoint commanded it to move. Could have been
       * changed by an optical port command.
       */
      if (eGPLUS_RD == eMtrType_)   /*lint !e506 !e774 eMtrType_ may not be constant in future project   */
      {
#if TM_SD_UNIT_TEST
         uint16_t msglen;
         sysTime_dateFormat_t sysTime;

         (void)TIME_UTIL_GetTimeInDateFormat(&sysTime);
         msglen =  (uint16_t)snprintf(prntBuf, sizeof(prntBuf), "%02d:%02d:%02d\n",
                                 sysTime.hour, sysTime.min, sysTime.sec );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "ReadMT117Res: 0x%02x",
                              pData->RxPacket.uRxData.sTblData.sRxBuffer[0] );
         for ( uint8_t i = 1; i < sizeof(lastMT117res); i++ )
         {
            msglen +=  (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "%02x",
                                 pData->RxPacket.uRxData.sTblData.sRxBuffer[i] );
         }
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "\n" );
#if VERBOSE>=1
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "loxli: %d\n",
                              pStatus->loxli );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "lohz: %d\n",
                              pStatus->lohz );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "ccst: Cap V %s\n",
                              pStatus->ccst == 0 ? "OK" :
                              pStatus->ccst == 1 ? "LOW" :
                              pStatus->ccst == 2 ? "HI" : "?" );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "swft: %d\n",
                              pStatus->swft );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "cflt: %d\n",
                              pStatus->cflt );
#endif
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "swst: %d\n",
                              pStatus->swst );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "cmst: %d\n",
                              pStatus->cmst );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "acot: %d\n",
                              pStatus->acot );
         msglen += (uint16_t)snprintf( prntBuf + msglen, sizeof(prntBuf)-msglen, "acst: %s\n",
                              pStatus->acst == 0 ? "No AC" :
                              pStatus->acst == 1 ? "Line AC" :
                              pStatus->acst == 2 ? "Load AC" : "Line/Load AC" );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "exac: %d\n",
                              pStatus->exac );
#if VERBOSE>=1
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "armf: %d\n",
                              pStatus->armf );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "capf: %d\n",
                              pStatus->capf );
#endif
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "cvr: %d = %2.2fV\n",
                              pStatus->cvr, (float)pStatus->cvr * 0.196 );
#if VERBOSE>=1
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "rev: %d\n",
                              pStatus->revision );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "ver: %d\n",
                              pStatus->version );
#endif
         DBG_printf( "%s", prntBuf );
#endif
      }
   }
   else if (0 == (--dsRetries_))//Retry if HMC_APP_API_CMD_TBL_ERROR, HMC_APP_API_CMD_ABORT or HMC_APP_API_CMD_ERROR
   {
      pCurrentCmd_ = NULL;
   }
   else
   {
      retVal = DS_RES_DO_NOT_ADVANCE_TABLE;
   }
   return (retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static void updateSwitchStateCmdStatus(eSdCmdStatus_t newStatus)">
/***********************************************************************************************************************
 *
 * Function Name: updateSwitchStateCmdStatus
 *
 * Purpose: Updates the Remote Disconnect Command Status field in Switch state/status are
 *          unaffected.
 *
 * Arguments: eSdCmdStatus_t newStatus - New status value to write to Switch state/status
 *
 * Returns: None
 *
 * Side Effects: Updates NV location of Switch state/status
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
static void updateSwitchStateCmdStatus(eSdCmdStatus_t newStatus)
{
   regStateStatus_u sdReg;    /* Switch state/status, it will be updated in NV memory. */

   /* Performs a read-modify-write operation. */
   (void)FIO_fread(&mtrDsFileHndl_, (uint8_t *)&sdReg, (uint16_t)offsetof(mtrDsFileData_t, dsStatusReg),
                     (lCnt)sizeof(sdReg));
   sdReg.bits.sdCmdStatus = (uint8_t)newStatus;  /* Update Switch Command Status. */
   (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static void startOpCodeLockOutTmr(void)">
/***********************************************************************************************************************
 *
 * Function Name: startOpCodeLockOutTmr
 *
 * Purpose: Sets the procedure timer.  This timer is needed to allow the meter to update its internal regsiters.
 *
 * Arguments: time value in seconds; if 0, use the default set with the timer
 *
 * Returns: None
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
#if SUPPORTS_LOCKOUT_TIMER
static void startOpCodeLockOutTmr( void )
{
   if( 0 == mtrDsFileData_.dsInfo.OpCodeLockOut )    /* Don't reset the lockout timer if in progress!   */
   {
      /* Set the timer to allow the switch movement to occur before updating the registers. */
      (void)TMR_ResetTimer( lockTimer_id , chargeTimeSec_[eMtrType_].chargeTimeSec * (uint32_t)TIME_TICKS_PER_SEC );

#if TM_SD_UNIT_TEST
         assert(eMtrType_ == eDEFAULT_METER);
         DBG_printf( "Lock out starting.\n" );
#endif

      /* Set the lockout flag */
      mtrDsFileData_.dsInfo.OpCodeLockOut = TRUE;
      (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
   }
}
#endif
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static void opcodeTmrCallBack_ISR(uint8_t cmd, uint8_t *pData)">
/***********************************************************************************************************************
 *
 * Function Name: opcodeTmrCallBack_ISR
 *
 * Purpose: Timer Expired, call back function.  In the background this clears the 'OpCode LockOut' flag indicating
 *          communications with the host meter can resume or an opcode can be executed.  THIS IS CALLED FROM THE
 *          TIMER ISR AND IS RUNNING AT INTERRUPT LEVEL. FILEIO FUNCTIONS SHOULD NOT BE CALLED FROM THIS FUNCTION.
 *
 *
 * Arguments: uint8_t cmd, uint8_t *pData - Both variables are not used for this call-back function.
 *
 * Returns: None
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
//lint -e{715} suppress "cmd and pData not referenced"
#if SUPPORTS_LOCKOUT_TIMER
static void opcodeTmrCallBack_ISR( uint8_t cmd, uint8_t const *pData )
{
   clearLockout_ = (bool)TRUE;
   SaveISR_Data_ = (bool)TRUE;
}
#endif
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static void statusReadRetryCallBack_ISR(uint8_t cmd, uint8_t *pData)">
/***********************************************************************************************************************
 *
 * Function Name: statusReadRetryCallBack_ISR
 *
 * Purpose: Timer Expired, call back function.  Reads the meter status table again.  This is called only if the expected
 *          and actual switch status do not match. THIS IS CALLED FROM THE TIMER ISR AND IS RUNNING AT INTERRUPT LEVEL.
 *          FILEIO FUNCTIONS SHOULD NOT BE CALLED FROM THIS FUNCTION.
 *
 *
 * Arguments: uint8_t cmd, uint8_t *pData - Both variables are not used for this call-back function.
 *
 * Returns: None
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
//lint -e{715} suppress "cmd and pData not referenced"
static void statusReadRetryCallBack_ISR( uint8_t cmd, uint8_t const *pData )
{
   if (meterStatusRetries_ > 0 )
   {
      meterStatusRetries_--;
      meterWaitStatus_ = eTableRetryTmrExpired;   /* The table read status was waiting and now timer has expired */
   }
   else
   {
      meterWaitStatus_ = eTableRetryNotWaiting;  /* The retries have expired, command execution has failed */
      if (electricMeterRCDSwitchConnect == mtrDsFileData_.OpCodeCmd)
      {  /* cmdStatus_ISR_ will be updated in the background in the API function */
         cmdStatus_ISR_ = eSwFailedToClose;         /* Execution failed to close.   */
      }
      else
      {
         cmdStatus_ISR_  = eSwFailedToOpen;         /* Execution failed to open  */
      }
      SaveISR_Data_ = (bool)TRUE;
   }
}
/**********************************************************************************************************************
 *
 * Function Name: acceptNewProcedure
 *
 * Purpose: check whether a new ds request can be accepted.
 *
 * Arguments:  Void
 *
 * Returns: True a new procedure can run, False it cannot run
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 *
 **********************************************************************************************************************/

static bool canAcceptNewProcedure(void)
{
   timer_t rcdTimer;
   bool accept         = (bool) false;
   returnStatus_t  res;

   rcdTimer.usiTimerId = rcdTimer_id;
   res = TMR_ReadTimer(&rcdTimer);
   if( res ==  eSUCCESS &&  NULL == pCurrentCmd_ && rcdTimer.bStopped && eTableRetryTmrExpired != meterWaitStatus_ )
   {

#if SUPPORTS_LOCKOUT_TIMER
      if( false == mtrDsFileData_.dsInfo.OpCodeLockOut )
#endif
      {
         accept = true;
      }
   }

   return accept;
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static void tx_FMR()">
/**********************************************************************************************************************
 *
 * Function Name: tx_FMR
 *
 * Purpose: Send a Full Meter Reading to the head end
 *
 * Arguments:  APP_MSG_Rx_t * - points to application header from head end
 *             enum_status - response code to be placed in response header method/status field
 *             bool updateEventTime - if true, update the last recorded event time in NV (switch actually moved).
 *
 * Returns: void
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 * Note: Uses lastHEEPcmd_ to determine reading value returned in the Full Meter Read bit structure.
 *
 **********************************************************************************************************************/
static void tx_FMR( HEEP_APPHDR_t const *heepHdr, enum_status methodStatus, EDEventType eventID, bool updateEventTime )
{
   buffer_t       *pBuf;                  /* pointer to message   */

#if INCLUDE_SWITCH_ACTIVATION_COUNT == 0
   pBuf = BM_alloc( sizeof( FullMeterRead ) + sizeof( readingQty_t ) + sizeof(meterReadingType) + sizeof(uint8_t) );
#else
   pBuf = BM_alloc( sizeof( FullMeterRead ) + ( sizeof( readingQty_t ) * 2 ) + sizeof(meterReadingType) + sizeof(uint8_t) );
#endif

   if ( pBuf != NULL )
   {
      HEEP_APPHDR_t     heepResponse;  /* Response to command  */
      pack_t            packCfg;       /* Struct needed for PACK functions */
      uint8_t           *payload;      /* Pointer to response payload   */
      sysTime_t         ReadingTime;   /* Used to retrieve system date/time   */
      uint32_t          msgTime;       /* Time converted to seconds since epoch  */
      uint16_t          param;         /* Used to pack 16 bit quantities into response buffer   */
      fmrRdgValInfo_t   bfield;        /* Used to populate the readings value information bitfield */
      EventRdgQty_u     EventRdgQty;   /* Count of events/readings in the payload      */
      EventData_s disconnectEvent;     /* Event info  */
      EventKeyValuePair_s  keyVal;

      /* Use now as the event time  */
      (void)TIME_SYS_GetSysDateTime( &ReadingTime );
      msgTime = (uint32_t)(TIME_UTIL_ConvertSysFormatToSysCombined(&ReadingTime) / TIME_TICKS_PER_SEC);


      /*  meter payload begins at data element  */
      payload = pBuf->data;

      /* Prepare a message to the head end   */
      /* Application header   */
      HEEP_initHeader( &heepResponse );
      heepResponse.TransactionType   = (uint8_t)TT_RESPONSE;
      heepResponse.Resource          = heepHdr->Resource;
      heepResponse.Method_Status     = (uint8_t)methodStatus;
      heepResponse.Req_Resp_ID       = heepHdr->Req_Resp_ID;
      heepResponse.qos               = heepHdr->qos;
      heepResponse.callback          = NULL;
      heepResponse.TransactionContext = heepHdr->TransactionContext;

      /* valuesInterval.end = current time   */
      PACK_init( payload, &packCfg );
      PACK_Buffer( -(int16_t)sizeof(msgTime) * 8, (uint8_t *)&msgTime, &packCfg );
      pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

      if ( methodStatus == NotFound )
      {
         EventRdgQty.bits.eventQty = 0;   /*  No events  */
         EventRdgQty.bits.rdgQty   = 0;   /*  No readings */
         PACK_Buffer( sizeof(EventRdgQty) * 8, (uint8_t *)&EventRdgQty, &packCfg );
         pBuf->x.dataLen += EVENT_AND_READING_QTY_SIZE;
      }
      else
      {
         EventRdgQty.bits.eventQty = 1;   /*  One event  */
         if( methodStatus == BadRequest )
         {  /* bad request response does not include an event */
            EventRdgQty.bits.eventQty = 0;
         }

         if( electricMeterRCDSwitchInProgress == eventID )
         {
           EventRdgQty.bits.rdgQty = 0;
         }
         else
         {
#if INCLUDE_SWITCH_ACTIVATION_COUNT == 0
           EventRdgQty.bits.rdgQty = 2;     /*  Two readings - switch position and load side voltage */
#else
           EventRdgQty.bits.rdgQty = 3;     /*  Three readings - switch position, load side voltage and switch activation
           counter */
#endif
         }

         /* pack the event/reading quantity into the response */
         PACK_Buffer( sizeof(EventRdgQty) * 8, (uint8_t *)&EventRdgQty, &packCfg );
         pBuf->x.dataLen += EVENT_AND_READING_QTY_SIZE;

         if( EventRdgQty.bits.eventQty )
         {  /* we have an event that needs to be included in the response */
            if ( updateEventTime ||                      /* Did an action actually occur? */
               ( mtrDsFileData_.lastHEEPtime == 0 ) )    /* Or is this the very first command received?  */
            {
               /* Yes, use the timestamp of the current request   */
               mtrDsFileData_.lastHEEPtime = HEEPtime_;
               (void)FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof(mtrDsFileData_));
            }

            /* Setup the event data structure to log the event */
            disconnectEvent.eventId                 = (uint16_t)eventID;

            disconnectEvent.eventKeyValuePairsCount = 0;
            disconnectEvent.eventKeyValueSize       = 0;
            if( electricMeterRCDSwitchInProgress == eventID )
            {
			      /*electricMeterRCDSwitchInProgress is conincident with when we build the message*/
               disconnectEvent.timestamp               = msgTime;
            }
            else
            {
              disconnectEvent.timestamp               = mtrDsFileData_.lastHEEPtime;
            }

            /* initialize the key value pair information for the logged event */
            (void)memset( (uint8_t *)&keyVal, 0, sizeof(keyVal) );

            /* Since we are sending the event in the response, mark the event sent before sending it to the
               event logger. */
            LoggedEventDetails   loggedEventData;
            disconnectEvent.markSent = (bool)true;
            ( void )EVL_LogEvent( 250, &disconnectEvent, &keyVal, TIMESTAMP_PROVIDED, &loggedEventData );

            /* Pack the EndDeviceEvents.createdDateTime = time of receipt of request from HE */
            PACK_Buffer( -(int16_t)sizeof(disconnectEvent.timestamp) * 8, (uint8_t *)&disconnectEvent.timestamp, &packCfg );
            pBuf->x.dataLen += sizeof(disconnectEvent.timestamp);

            /* Pack the EndDeviceEvents.eventType = enumeration id of the event */
            PACK_Buffer( -(int16_t)sizeof(disconnectEvent.eventId) * 8, (uint8_t *)&disconnectEvent.eventId, &packCfg );
            pBuf->x.dataLen += sizeof(disconnectEvent.eventId);

            /* We need to include the alarm index id of the logged event.
               pack the nvp description information (quantity and size of nvp data ) */
            union nvpQtyDetValSz_u nvpDesc;
            nvpDesc.bits.nvpQty   = 1;
            nvpDesc.bits.DetValSz = 1;
            PACK_Buffer( sizeof(nvpDesc) * 8, (uint8_t *)&nvpDesc, &packCfg );
            pBuf->x.dataLen += sizeof(nvpDesc);

            /* Pack the NVP name = the buffer id where the event was logged */
            PACK_Buffer( -(int16_t)sizeof(loggedEventData.alarmIndex) * 8, (uint8_t *)&loggedEventData.alarmIndex, &packCfg );
            pBuf->x.dataLen += sizeof(loggedEventData.alarmIndex);

            /* Pack the NVP value = the index value of the event when stored into the alarm buffer */
            PACK_Buffer( (int16_t)sizeof(loggedEventData.indexValue) * 8, (uint8_t *)&loggedEventData.indexValue, &packCfg );
            pBuf->x.dataLen += sizeof(loggedEventData.indexValue);

         /* Set the event type   */
#if TM_SD_UNIT_TEST
#if VERBOSE > 1
            DBG_printf( "Event type: %d\n", swResult_ );
#endif
#endif
         }

         if ( swResult_ == (EDEventType)0 )
         {
            asm( "nop" );
         }

         if( 0 < EventRdgQty.bits.rdgQty )
         {
           /* Load the reading information. */
           bfield.isCoinTrig    = 0;
           bfield.tsPresent     = 0;
           bfield.RdgQualPres   = 0;
           bfield.rsvd2         = 0;
           bfield.pendPowerof10 = 0;
// TODO: K24 Bob: This should be converted to uintValue in the K24, as well.
           bfield.typecast      = (uint16_t)uintValue; // TODO: RA6E1 Bob: should not be building it here but this needs to be uintValue
#if ( OTA_CHANNELS_ENABLED == 0 )
           bfield.rsvd1         = 0;
#endif
           bfield.RdgValSize    = 1;
           bfield.RdgValSizeU   = 0;
           bfield.rdgType       = BIG_ENDIAN_16BIT( switchPositionStatus );
           PACK_Buffer( sizeof(bfield) * 8, ( uint8_t *)&bfield, &packCfg );
           pBuf->x.dataLen += sizeof(bfield);

           /* Since no quality codes, time stamp and quality fields are not used. Next element is the reading value */
           param = mtrDsFileData_.dsStatusReg.bits.swClosed;
           PACK_Buffer( bfield.RdgValSize * 8, ( uint8_t *)&param, &packCfg );
           pBuf->x.dataLen += bfield.RdgValSize;

           /* Now add the load side voltage reading (uintValue) */
           bfield.typecast = (uint16_t)uintValue;
           bfield.rdgType  = BIG_ENDIAN_16BIT( energizationLoadSide );
           PACK_Buffer( sizeof(bfield) * 8, ( uint8_t *)&bfield, &packCfg );
           pBuf->x.dataLen += sizeof(bfield);

           /* Since no quality codes, time stamp and quality fields are not used. Next element is the reading value */
           if( LINE_LOAD_SIDE_VOLTAGE_PRESENT == lastMT117res.acst )
           {
             if( lastMT117res.exac )
             {
               param = LOAD_SIDE_VOLTAGE_NOT_SYNCHRONIZED_WITH_LINE_SIDE;
             }
             else
             {
               param = LOAD_SIDE_VOLTAGE_SYNCHRONIZED_WITH_LINE_SIDE;
             }
           }
           else if( ONLY_LOAD_SIDE_VOLTAGE_PRESENT == lastMT117res.acst )
           {
             param = LOAD_SIDE_VOLTAGE_COMPARE_NOT_AVAILABLE;
           }
           else
           {
             param = LOAD_SIDE_VOLTAGE_DEAD;
           }

           // pack the load side status value
           PACK_Buffer( bfield.RdgValSize * 8, ( uint8_t *)&param, &packCfg );
           pBuf->x.dataLen += bfield.RdgValSize;

#if INCLUDE_SWITCH_ACTIVATION_COUNT == 1
           /* Now add the switch activation counter  */
           bfield.typecast = (uint16_t)uintValue;
           bfield.RdgValSize = 2;
           bfield.rdgType = BIG_ENDIAN_16BIT( switchPositionCount );

           param = BIG_ENDIAN_16BIT( mtrDsFileData_.sdSwActCntr );
           PACK_Buffer( sizeof(bfield) * 8, ( uint8_t *)&bfield, &packCfg );
           pBuf->x.dataLen += sizeof(bfield.RdgValSize);
           PACK_Buffer( bfield.RdgValSize * 8, ( uint8_t *)&param, &packCfg );
           pBuf->x.dataLen += sizeof(bfield.RdgValSize);
#endif
         }
      }

#if TM_SD_UNIT_TEST
      uint16_t  msglen = 0;              /* Length of message being built */
      msglen +=
         (uint16_t)snprintf( prntBuf + msglen, sizeof(prntBuf)-msglen, "%02x%02x%02x%02x",
                              0,
                              (uint8_t)( heepResponse.TransactionType << 6) | (heepResponse.Resource & 0x3f),
                              heepResponse.Method_Status,
                              heepResponse.Req_Resp_ID);
      for ( uint8_t *p = (uint8_t *)payload; p < packCfg.pArray; p++ )
      {
         msglen += (uint16_t)snprintf( prntBuf + msglen, sizeof(prntBuf)-msglen, "%02x", *p );
      }
      DBG_printf( "%s\n", prntBuf );
#endif

      (void)HEEP_MSG_Tx (&heepResponse, pBuf);
   }
}
// <editor-fold defaultstate="collapsed" desc="void HMC_DS_ExecuteSD()">
/***********************************************************************************************************************
*
* Function name: HMC_DS_ExecuteSD()
*
* Purpose: Process the commands related to Service Disconnect Switch
*
* Arguments:
*     APP_MSG_Rx_t *appMsgInfo - Pointer to application header structure from head end
*     void *payloadBuf         - pointer to data payload (in this case a ExchangeWithID structure)
*     length                   - payload length
*
* Side Effects:
*     It can potentially affect the state of the switch and its associated registers
*
* Re-entrant: No
*
* Returns: none
*
* Notes:
*     payloadBuf will be deallocated by the calling routine.
*
***********************************************************************************************************************/
//lint -efunc(818,HMC_DS_ExecuteSD)    arguments can't be const due to generic API requirements
void HMC_DS_ExecuteSD(HEEP_APPHDR_t *heepHdr, void *payloadBuf, uint16_t length)
{
   (void)length;
   bool                 sendReply = (bool)true;             /* Assume response to be sent immediately */
   enum_status          methodStatus = ServiceUnavailable;  /* Command Response Codes - Assume module is disabled. */
   pExchangeWithID      action;                             /* Requested action - open/close */
   uint32_t             msgTime;                            /* Current system time  */
   sysTime_t            ReadingTime;                        /* Used to retrieve system date/time   */
   sysTime_dateFormat_t sysTime;
   EDEventType          swEvent = ( EDEventType ) 0;                            /*event ID in case of error*/

    //throw away duplicate requests while request is executing
   if( ( lastHEEPappHdr_.ReqID != heepHdr->ReqID ) || ( canAcceptNewProcedure() ))
   {
      if ( ( method_put == (enum_MessageMethod)heepHdr->Method_Status ) && ( MODECFG_get_rfTest_mode() == 0 ) )
      {  //Process the put request, prepare Full Meter Read structure

         action = (pExchangeWithID)payloadBuf;     /* Type-safe, local copy of payload  */
         /* If the EventQty not one, or the ReadingQty is not zero, EP shall reply with a status code indicating a “BAD REQUEST”. */
         if ( ( 1 == action->eventRdgQty.bits.eventQty ) && ( 0 == action->eventRdgQty.bits.rdgQty ) )
         {
#if ( HMC_I210_PLUS_C == 1 )
            /* Get the latest password values stored in the endpoint. Before service disconnect commmand is
            executed we will logon with the master password.  After command is completed, we will return
            logon status back to reader password. These password values will be used by the command tables. */
            (void)HMC_STRT_GetPassword( password );
            (void)HMC_STRT_GetMasterPassword( mPassword );
#endif

            /* 16 bit quantities must be byte swapped */
            action->eventType = (EDEventType)BIG_ENDIAN_16BIT(action->eventType);

            /* Get current status of the switch  */
            (void) FIO_fread( &mtrDsFileHndl_, (uint8_t *)&mtrDsFileData_, 0, (lCnt)sizeof (mtrDsFileData_));

            if ( bFeatureEn_ )   /* Is the Disconnect Switch Present and Enabled? */
            {
#if SUPPORTS_LOCKOUT_TIMER
               methodStatus = ServiceUnavailable;
#endif
               if ( canAcceptNewProcedure() )

               {
                  /* feature must be fully enabled and intitialzed before accepting commands */
                  (void)TIME_UTIL_GetTimeInDateFormat(&sysTime);
                  (void)TIME_SYS_GetSysDateTime( &ReadingTime );
                  msgTime      = (uint32_t)(TIME_UTIL_ConvertSysFormatToSysCombined(&ReadingTime) / TIME_TICKS_PER_SEC);
                  HEEPtime_    = msgTime;
                  lastHEEPcmd_ = action->eventType;
                  /* Copy the header to a local structure and the command to a local variable   */
                  lastHEEPappHdr_ = *heepHdr;

                  sendReply    = (bool)false;   /* Switch present/enabled, valid command, no immediate response   */
                  methodStatus = OK;

                  /* Update info in NV */
                  mtrDsFileData_.OpCodeCmd              = action->eventType;  /* Update the latest command */
                  mtrDsFileData_.dsInfo.OpCodeProcessed = FALSE;              /* Command in progress */

                  (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));

                  switch ( action->eventType ) /* What to do? */  /*lint !e788 not all enums used within switch */
                  {
                     case electricMeterRCDSwitchConnect: /* Close the meter's switch */
                     {
                        pCurrentCmd_            = pCloseSwitchTables_[eMtrType_];
                        lastOperationAttempted_ = eCloseSd;
                        break;
                     }
                     case electricMeterRCDSwitchDisconnect: /* Open the meter's switch. */
                     {
                        pCurrentCmd_            = pOpenSwitchTables_[eMtrType_];
                        lastOperationAttempted_ = eOpenSd;
                        break;
                     }
                     case electricMeterRCDSwitchArmForClosure:
                     case electricMeterRCDSwitchArmForOpen:
                     //FALLTHROUGH
                     default:  /* Unrecognized command for this resource! */
                     {
                        methodStatus = BadRequest;
                        sendReply    = (bool)true;   /* Bad command, immediate response   */
                        break;
                     }
                  } /*lint !e788 not all enums used within switch */
                  /* Set up to send command sequence to meter, even if already in requested state. */
                  if ( methodStatus != BadRequest )
                  {
                     dsRetries_                            = DS_APP_RETRIES;           /* Setup the number of retries in case of an error. */
                     meterStatusRetries_                   = DS_METER_STATUS_RETRIES;  /* Retry counter for reading of MT115 - status  */
                     mtrDsFileData_.dsInfo.OpCodeProcessed = TRUE;                     /* OpCode processed and waiting to be sent to the meter */
                     LSVsent                               = (bool)false;              /*  Clear this upon receipt of a new command  */
                  }
               }
               else  /* Received a command to operate the switch during a lockout period or while command already in process */
               {
#if SUPPORTS_LOCKOUT_TIMER
#if TM_SD_UNIT_TEST
                  DBG_printf( "Lock out active\n" );
#endif
#endif
                  methodStatus = Conflict;
                  swEvent      = electricMeterRCDSwitchInProgress;
               }
            }
            else  /* Switch not enabled or not present */
            {
               if( 0 == mtrDsFileData_.dsInfo.hwExists )     /* Switch does not exist   */
               {
                  methodStatus = NotFound;
                  updateSwitchStateCmdStatus(eCmdNoSd);  /* SD switch is not present or enabled, indicate cmd failure in Switch
                  state/status   */
               }
               else  /* Switch exists, but module has not been enabled, yet   */
               {
                  /* methodStatus is already set to ServiceUnavailable */
                  if (electricMeterRCDSwitchConnect == action->eventType)
                  {
                     swEvent = electricMeterRCDSwitchConnectFailed;
                  }
                  else if (electricMeterRCDSwitchDisconnect == action->eventType)
                  {
                     swEvent = electricMeterRCDSwitchDisconnectFailed;
                  }
                  else  /* Unrecognized command for this resource! */
                  {
                     methodStatus = BadRequest;
                     swEvent = (EDEventType )0;
                  }
               }
            }
         }
         else  /* Invalid Event or Reading Qty */
         {
            methodStatus = BadRequest;
         }
#if TM_SD_UNIT_TEST
#if VERBOSE>1    /*  mkv - debug/test code  */
         uint16_t  msglen;              /* Length of message being built */
         msglen =  (uint16_t)snprintf(prntBuf, sizeof(prntBuf), "swClosed: %d\n", mtrDsFileData_.dsStatusReg.bits.swClosed );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "sdCmdStatus: 0x%x\n",
                                       mtrDsFileData_.dsStatusReg.bits.sdCmdStatus );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "lastOperationSource: 0x%x\n",
                                       mtrDsFileData_.dsStatusReg.bits.lastOperationSource );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "lastOperationAttempted: 0x%x\n",
                                       mtrDsFileData_.dsStatusReg.bits.lastOperationAttempted );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen,
                                       "sdSwActCntr: 0x%x\n", mtrDsFileData_.sdSwActCntr );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "OpCodeCmd: %d\n", mtrDsFileData_.OpCodeCmd );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen,
                                       "OpCodeLockOut: 0x%x\n", mtrDsFileData_.dsInfo.OpCodeLockOut );
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen,
                                       "OpCodeProcessed: 0x%x\n", mtrDsFileData_.dsInfo.OpCodeProcessed );
#if TM_SD_UNIT_TEST
       HMC_COM_INFO dummy;
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "sizeof(HMC_COM_INFO.TxPacket): %d\n",
                                       sizeof(dummy.TxPacket));
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "sizeof(HMC_COM_INFO.TxPacket.uTxData): %d\n",
                                       sizeof(dummy.TxPacket.uTxData));
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "sizeof(HMC_COM_INFO.RxPacket.uRxData): %d\n",
                                       sizeof(dummy.RxPacket.uRxData));
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "sizeof(HMC_COM_INFO): %d\n",
                                       sizeof(dummy));
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "sizeof(HMC_COM_INFO.RxPacket): %d\n",
                                       sizeof(dummy.RxPacket));
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen,
                                       "sizeof(HMC_COM_INFO.RxPacket.sTblData.sRxBuffer): %d\n",
                                       sizeof(dummy.RxPacket.uRxData.sTblData.sRxBuffer));
#endif
         msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen,
                                    "offsetof(HMC_COM_INFO.RxPacket.sTblData.sRxBuffer): %d\n",
                                    offsetof(HMC_COM_INFO, RxPacket.uRxData.sTblData.sRxBuffer));
         DBG_printf( "msglen = %d\n%s", msglen, prntBuf );
#endif
#endif
#if TM_SD_UNIT_TEST
         (void)snprintf(prntBuf, sizeof(prntBuf), "%02d/%02d/%04d %02d:%02d:%02d\n",
                        sysTime.month, sysTime.day, sysTime.year,
                        sysTime.hour, sysTime.min, sysTime.sec );
         DBG_printf( "%s", prntBuf );
#endif
         if ( sendReply )
         {
            tx_FMR( heepHdr, methodStatus, swEvent, (bool)false );   /* Send a response to the head end   */
         }
      }
   }
}

#if TM_SD_UNIT_TEST
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void UnitTest_ResponseCodes(void)">
/***********************************************************************************************************************
 *
 * Function name: UnitTest_ResponseCodes
 *
 * Purpose: Simulates calling OpCode 151 with various command parameters and sends the response codes to the
 *          manufacturing serial port.
 *
 * Arguments: void
 *
 * Returns: void
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
static const HEEP_APPHDR_t CloseRelay =
{
   (uint8_t)TT_REQUEST,
   (uint8_t)rc,
   (uint8_t)method_put,
   (uint8_t)electricMeterRCDSwitchConnect,
   0,
   NULL,
   (uint8_t)0
};
void UnitTest_ResponseCodes(void)
{
   //   PRNT_string(" OpCode 151 - Service Disconnect ");PRNT_crlf();
   //   PRNT_string("     +-------------+");PRNT_crlf();
   //   PRNT_string("     |   COMMAND   |");PRNT_crlf();
   //   PRNT_string("     |   PARAMETER |");PRNT_crlf();
   //   PRNT_string("     +-------------+");PRNT_crlf();
   //   PRNT_string("     |   Command   |");PRNT_crlf();
   //   PRNT_string("     |     Code    |");PRNT_crlf();
   //   PRNT_string("+----+-------------+");PRNT_crlf();
   //   PRNT_string("|Bits|      8      |");PRNT_crlf();
   //   PRNT_string("+----+-------------+");PRNT_crlf();
   //   PRNT_crlf();
   //   PRNT_string("     +----------+");PRNT_crlf();
   //   PRNT_string("     | RESPONSE |");PRNT_crlf();
   //   PRNT_string("     +----------+");PRNT_crlf();
   //   PRNT_string("     |  Status  |");PRNT_crlf();
   //   PRNT_string("+----+----------+");PRNT_crlf();
   //   PRNT_string("|Bits|     8    |");PRNT_crlf();
   //   PRNT_string("+----+----------+");PRNT_crlf();
   //
   //
   //   PRNT_string("OpCode 151 with and OPEN command");PRNT_crlf();
   //   PRNT_string("Objective: This test validates the response codes to an OPEN command");PRNT_crlf();

   HEEP_APPHDR_t   heepHdr;    /* Message header information */
   ExchangeWithID payloadBuf;    /* Request information  */    /* Request information  */
/*
   PRNT_string("COMMAND, RESPONSE");
   PRNT_crlf();
*/
   (void)memcpy((uint8_t *)&heepHdr, (uint8_t *)&CloseRelay, sizeof(heepHdr));
   (void)memset((void *)&payloadBuf, 0, sizeof(payloadBuf));
   payloadBuf.eventType = electricMeterRCDSwitchDisconnect;
   HMC_DS_ExecuteSD(&heepHdr, &payloadBuf);
}
#endif
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void HMC_DS_UnitTest(void)">
#if TM_SD_UNIT_TEST
/***********************************************************************************************************************
 *
 * Function name: printStats
 *
 * Purpose: Debugging utility for printing out the fields from MT115
 *
 * Arguments:  char * title - printed out
 *             HMC_COM_INFO - treated as a response from reading MT115
 *
 * Returns: void
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
static void printStats( const char *title, HMC_COM_INFO const *pData )
{
   uint16_t  msglen;              /* Length of message being built */
   ldCtrlStatusTblGPlus_t *pCtrlStat = (ldCtrlStatusTblGPlus_t *)&pData->RxPacket.uRxData.sTblData.sRxBuffer[0];
   sysTime_dateFormat_t sysTime;

   (void)TIME_UTIL_GetTimeInDateFormat(&sysTime);
   msglen =  (uint16_t)snprintf(prntBuf, sizeof(prntBuf), "%02d:%02d:%02d\n",
                                 sysTime.hour, sysTime.min, sysTime.sec );

   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "%s: 0x%02x", title,
                        pData->RxPacket.uRxData.sTblData.sRxBuffer[0] );
   for ( uint8_t i = 1; i < offsetof( ldCtrlStatusTblGPlus_t, lcState ); i++ )
   {
      msglen +=  (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "%02x",
                           pData->RxPacket.uRxData.sTblData.sRxBuffer[i] );
   }
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "\nactual: %d\n",
                        pCtrlStat->rcdcState.actual_switch_state );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "desired: %d\n",
                        pCtrlStat->rcdcState.desired_switch_state );
#if VERBOSE>=1
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "ohold: %d\n",
                        pCtrlStat->rcdcState.open_hold_for_command );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "arm: %d\n",
                        pCtrlStat->rcdcState.armed_waiting_to_close );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "out: %d\n",
                        pCtrlStat->rcdcState.outage_open_in_effect );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "lock: %d\n",
                        pCtrlStat->rcdcState.lockout_in_effect );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "wait: %d\n",
                        pCtrlStat->rcdcState.waiting_to_arm );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "commerr: %d\n",
                        pCtrlStat->status.communication_error );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "ctrl err: %d\n",
                        pCtrlStat->status.switch_controller_error );
#endif
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "ftc: %d\n",
                        pCtrlStat->status.switch_failed_to_close );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "LSV: %d\n",
                        pCtrlStat->status.alternate_source );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "byp: %d\n",
                        pCtrlStat->status.bypassed );
#if VERBOSE>=1
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "fto: %d\n",
                        pCtrlStat->status.switch_failed_to_open );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "lnfe: %d\n",
                        pCtrlStat->status.line_side_freq_error );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "ldfe: %d\n",
                        pCtrlStat->status.load_side_freq_error );
#endif
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "Last cmd status: 0x%02x\n",
                        pCtrlStat->lastCommandStatus );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "Retries: %d\n",
                        DS_METER_STATUS_RETRIES - meterStatusRetries_ );
   msglen += (uint16_t)snprintf(prntBuf + msglen, sizeof(prntBuf)-msglen, "\nSw open cnt: %d\n",
                        mtrDsFileData_.sdSwActCntr);
   DBG_printf( "%s", prntBuf );
}
// </editor-fold>
#endif
#if TM_SD_UNIT_TEST
// <editor-fold defaultstate="collapsed" desc="void HMC_DS_UnitTest(void)">
/***********************************************************************************************************************
 *
 * Function name: HMC_DS_UnitTest
 *
 * Purpose: Main unit test function for remote disconnect.  It calls the different function to test various features.
 *
 * Arguments: void
 *
 * Returns: void
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes: This function does not test power outages or time shifts
 *
 **********************************************************************************************************************/
void HMC_DS_UnitTest(void)
{
   UnitTest_ResponseCodes();
}
#endif
/***********************************************************************************************************************
*
* Function name: HMC_DS_ExecuteSD()
*
* Purpose: Send a response when Remote Disconnect hardware is not present.(Ex: SRFN- KV2C)
*
* Arguments:
*     APP_MSG_Rx_t *appMsgInfo - Pointer to application header structure from head end
*     void *payloadBuf         - pointer to data payload (Not used for this condition)
*
* Side Effects:
*     None
*
* Re-entrant: No
*
* Returns: none
*
* Notes:
*     payloadBuf is not used for this condition.
*
***********************************************************************************************************************/

#else // for #if ( REMOTE_DISCONNECT == 1 )

#include "hmc_ds.h" /* If this file is made common (not conditioned by REMOTE_DISCONNECT), then get several compile errors. Must be dependent on order
                       of header files. */

//lint -e{715,818} suppress "pData not referenced"
void HMC_DS_ExecuteSD(HEEP_APPHDR_t *heepHdr, void *payloadBuf, uint16_t length)
{
  (void)length;
  heepHdr->TransactionType = (uint8_t)TT_RESPONSE;
  heepHdr->Method_Status = (uint8_t)NotFound;
  (void)HEEP_MSG_TxHeepHdrOnly (heepHdr);
}

// </editor-fold>
#endif // for #if ( REMOTE_DISCONNECT == 1 )
/***********************************************************************************************************************
 *
 * Function name: HMC_DS_getSwitchPositionCount
 *
 * Purpose: Get the number of switch openings observed by the meter disconncec switch
 *
 * Arguments: void
 *
 * Returns: uint16_t
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
#if ( REMOTE_DISCONNECT == 1 )
uint16_t HMC_DS_getSwitchPositionCount( void )
{
   return mtrDsFileData_.sdSwActCntr;
}
#endif
