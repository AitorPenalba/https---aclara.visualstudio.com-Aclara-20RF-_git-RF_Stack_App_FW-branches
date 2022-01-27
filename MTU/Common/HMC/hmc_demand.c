// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************

   Filename: hmc_demand.c

   Global Designator: HMC_DMD_

   Contents: Meter Demand Functions

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$

    Created 01/24/08     KAD  Created

 ***********************************************************************************************************************
 ***********************************************************************************************************************
   Revision History:
   v1.0 - Initial Release
   v1.1 - Updated to return false if demand is disabled
   v1.2 - Added function, HMC_DMD_ResetBusy, to determine if this applet is busy (needs HMC).
   v1.3 04/03/2012 KAD - Added a call to 'HMC_ENG_Execute' after the procedure read to validate the procedure results
                         and set any registers/diagnostic bits as needed.
   v2.0 08/06/2013 KAD - New for VLF
 **********************************************************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define HMC_DMD_GLOBAL
#include "hmc_demand.h"
#undef  HMC_DMD_GLOBAL

//#include "time.h"
#include "hmc_eng.h"
#include "ansi_tables.h"
#include "hmc_seq_id.h"
#include "hmc_start.h"
#include "hmc_snapshot.h"
#include "time_util.h"
#include "time_sys.h"
#include "DBG_SerialDebug.h"
#include "APP_MSG_Handler.h"

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define HMC_DMD_RETRIES       ((uint8_t)5)  /* Misc Constants used by the time function */

#define DR_LOGOUT_TIMEOUT_SEC  60 /*Have DR wait up to 1 minute for a new, non-snapped session*/
#if ( DR_LOGOUT_TIMEOUT_SEC > ( APP_MAX_SERVICE_TIME_Sec - 5 ) )
#error "DR logout time insufficient"
#endif
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Variable Definitions">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static bool                   bDoMeterCom_ = false;         /* true = This applet needs to perform HMC. */
static uint32_t               drRequestTime_sec_ = 0;
static procNumId_t            procedureNumberAndId_;        /* The procedure # and ID are sent to the host meter */
static volatile pDemandResetCallBack   dmdResetCallBack_;   /* Function to call upon completion of demand reset   */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */

static const uint8_t remoteRstParam_ = 1; /* Remote reset parameters  - Do demand reset */

static const tWriteFull tblDmdRstProcedure_[] =
{
   CMD_TBL_WR_FULL,                                /* Full Table Write - Must be used for procedures. */
   BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE )  /* Procedure Table ID */ /*lint !e572 !e778 */
};

static const tReadPartial tblDmdRstProcResult_[] =
{
   CMD_TBL_RD_PARTIAL,
   PSEM_PROCEDURE_TBL_RES,                         /* lint !e778 !e572 */
   TBL_PROC_RES_OFFSET,
   BIG_ENDIAN_16BIT( sizeof( tProcResult ) )       /* Length to read */ /* lint !e778 !e572 */
};

static const HMC_PROTO_TableCmd tblCmdDmdRstProc_[] =
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblDmdRstProcedure_ ), ( uint8_t far * )&tblDmdRstProcedure_[0] },
   { HMC_PROTO_MEM_RAM, ( uint8_t )sizeof( procedureNumberAndId_ ),  ( uint8_t far * )&procedureNumberAndId_ }, /* Proc and ID */
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( remoteRstParam_ ), ( uint8_t far * )&remoteRstParam_ },
   { HMC_PROTO_MEM_NULL }
};

static const HMC_PROTO_TableCmd tblCmdDmdRstRes_[] =
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblDmdRstProcResult_ ), ( uint8_t far * )tblDmdRstProcResult_ },
   { HMC_PROTO_MEM_NULL }
};

static const HMC_PROTO_Table tblDemandReset_[] =
{
   &tblCmdDmdRstProc_[0],
   &tblCmdDmdRstRes_[0],
   NULL
};
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static bool DR_Logout_Timeout( void );
// </editor-fold>
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="uint8_t HMC_DMD_Reset( uint8_t ucCMD, void *pData )">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************

   Function name: HMC_DMD_Reset

   Purpose: Sends a demand reset to the host meter

   Arguments: enum HMC_APP_API_CMD - Command to do something

   Returns: uint8_t SomethingToDo, true, false

 **********************************************************************************************************************/
uint8_t HMC_DMD_Reset( uint8_t ucCMD, void *pData )
{
   static bool bFeatureEnabled_ = true;                  /* true = Feature Enabled, False, Feature not enabled. */
   static uint8_t   retries_;                                 /* Number of application level retries */
   uint8_t          retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;  /* Return value for this function */

   switch ( ucCMD )
   {
      case HMC_APP_API_CMD_DISABLE_MODULE:
      {
         bFeatureEnabled_ = false;
         bDoMeterCom_ = false;
         drRequestTime_sec_ = 0;
         break;
      }
      case HMC_APP_API_CMD_ENABLE_MODULE:
      {
         bFeatureEnabled_ = true;
         break;
      }
      case HMC_APP_API_CMD_INIT:
      {
         retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE;
         bDoMeterCom_ = false;
         drRequestTime_sec_ = 0;
         break;
      }
      case HMC_APP_API_CMD_ACTIVATE: /* Used for Reset */
      {
         if ( bFeatureEnabled_ )
         {
            if ( pData != NULL && !bDoMeterCom_ && !drRequestTime_sec_)
            {
               /* Save the call back pointer in the local variable   */
               dmdResetCallBack_ = ( pDemandResetCallBack )pData;    /*lint !e611 cast is correct  */
               if( eHMC_SS_SNAPPED == HMC_SS_isSnapped() )
               { /*performing a DR will invslidate metering data, must wait till logoff*/
                  sysTime_t reqTime; /* time of activate request */
                  uint32_t fracSec = 0; /*Precision is not needed*/
                  TIME_SYS_GetPupDateTime(&reqTime);
                  TIME_UTIL_ConvertSysFormatToSeconds(&reqTime, &drRequestTime_sec_, &fracSec);
               }
               else
               { /* we are not snapped and are free to DR immediatley*/
                  retries_ = HMC_DMD_RETRIES;
                  bDoMeterCom_ = true;
               }
            }
         }
         break;
      }
      case HMC_APP_API_CMD_STATUS:
      {
         if( bFeatureEnabled_ )
         {
            if (  bDoMeterCom_ )
            {
               retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
            else if( drRequestTime_sec_ )
            { /* we currently have a valid request and are waiting for a new snapshot/session*/
               if( eHMC_SS_NOT_SNAPPED == HMC_SS_isSnapped() )
               { /*We are no longer snapped and can modify metering data with a DR*/
                  bDoMeterCom_ = true;
                  drRequestTime_sec_ = 0;
                  DBG_logPrintf('I', "Started DEMAND RESET Stopping Logout Timer");
                  retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;
               }
               else if( DR_Logout_Timeout() )
               { /*exceeded alotted time for DR to wait for new session*/
                  drRequestTime_sec_ = 0;
                  DBG_logPrintf('I', "Logout Timer Expired DR STOPPED");
                  if ( dmdResetCallBack_ != NULL )
                  {
                     dmdResetCallBack_( eAPP_DMD_RST_REQ_TIMEOUT );
                  }
               }
            }
         }
         break;
      }
      case HMC_APP_API_CMD_PROCESS:
      {
         if ( bFeatureEnabled_ && bDoMeterCom_ )
         {
            procedureNumberAndId_.procNum = STD_PROC_REMOTE_RESET;         /* Set proc # */
            procedureNumberAndId_.procId = HMC_getSequenceId();            /* Get unique ID */
            ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )tblDemandReset_;
            retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;
         }
         break;
      }
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {
         tTbl8ProcResponse *pProcRes; /* Pointer to table 8 procedure results */

         /* The results of reading the Procedure Results Table are located in the sRxBuffer.  The 1st byte in
            sRxBuffer will contain the ID of last procedure executed.  The ID should be the same ID we used to set
            Date/Time. This is checked to make sure some other procedure didn't somehow get executed and the
            expected procedure did.  */
         pProcRes = ( tTbl8ProcResponse * ) & ( ( HMC_COM_INFO * )pData )->RxPacket.uRxData.sTblData.sRxBuffer[0];
         if ( procedureNumberAndId_.procId == pProcRes->seqNbr )
         {
            bDoMeterCom_ = false;
            if ( dmdResetCallBack_ != NULL )
            {
               switch( pProcRes->resultCode )
               {
                  case PROC_RESULT_COMPLETED:
                     dmdResetCallBack_( eSUCCESS );
                     break;
                  case PROC_RESULT_TIMING_CONSTRAINT:
                     dmdResetCallBack_( eAPP_DMD_RST_MTR_LOCKOUT );
                     break;
                  default:
                     dmdResetCallBack_(eFAILURE);
                     break;
               }
            }
            break;
         }
         //else - Fall through and retry.
      }
      // FALLTHROUGH
      case HMC_APP_API_CMD_TBL_ERROR:
      case HMC_APP_API_CMD_ABORT:
      case HMC_APP_API_CMD_ERROR:
      {
         if ( retries_ != 0 )
         {
            retries_--;
         }
         else
         {
            bDoMeterCom_ = false;
            if ( dmdResetCallBack_ != NULL )
            {
               dmdResetCallBack_( eFAILURE );
            }
         }
         break;
      }
      default:
      {
         retVal = ( uint8_t )HMC_APP_API_RPLY_UNSUPPORTED_CMD;
         break;
      } // No other commands are supported in this applet
   }
   return( retVal );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="bool HMC_DMD_ResetBusy(void)">
/***********************************************************************************************************************

   Function name: HMC_DMD_ResetBusy

   Purpose: Returns the state of the module, busy or not.  Busy means this module requires host meter communications.

   Arguments: none

   Returns: busy status (true (1) = Busy, false (0) = Not busy

 **********************************************************************************************************************/
bool HMC_DMD_ResetBusy( void )
{
   return( bDoMeterCom_ );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="bool HMC_DMD_ResetBusy(void)">
/***********************************************************************************************************************

   Function name: HMC_DMD_RegisterCallBack

   Purpose: Supply the call back function to be invoked at completion of demand reset

   Arguments: function pointer to be called upon completion of demand reset.

   Returns: existing call back. Allows temporary assignment/reassignment.

 **********************************************************************************************************************/
pDemandResetCallBack HMC_DMD_RegisterCallBack( pDemandResetCallBack callBack )
{
   pDemandResetCallBack currentCallBack = dmdResetCallBack_;   /* Return value   */

   dmdResetCallBack_ = callBack;    /* Save the call back pointer in the local variable   */

   return currentCallBack;
}
/* ****************************************************************************************************************** */
// </editor-fold>
static bool DR_Logout_Timeout( void )
{
   sysTime_t curTime;
   uint32_t curSec;
   uint32_t fracSec = 0;
   bool retVal = false;
   TIME_SYS_GetPupDateTime(&curTime);
   TIME_UTIL_ConvertSysFormatToSeconds(&curTime, &curSec, &fracSec);
   if( (curSec - drRequestTime_sec_) > DR_LOGOUT_TIMEOUT_SEC )
   {
      retVal = true;
   }
   DBG_logPrintf('I', "%d elpased", (curSec - drRequestTime_sec_));
   return retVal;
}
