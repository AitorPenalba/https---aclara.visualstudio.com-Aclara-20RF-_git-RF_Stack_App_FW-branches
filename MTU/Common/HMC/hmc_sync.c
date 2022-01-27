// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************
 *
 * Filename: hmc_sync.c
 *
 * Global Designator: HMC_SYNC_
 *
 * Contents: Sets the host meter's date/time.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved. This program may not be reproduced, in
 * whole or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 * Revision History:
 * v1.0 - Initial Release
 ******************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "psem.h"
#include "time_sys.h"
#include "time_util.h"
#include "hmc_time.h"
#include "hmc_eng.h"
#include "DBG_SerialDebug.h"
#include "ansi_tables.h"
#include "time_util.h"
#include "hmc_sync.h"

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Variables">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/*lint -esym(750,HMC_SYNC_PRNT_INFO,HMC_SYNC_PRNT_WARN,HMC_SYNC_PRNT_ERROR)   not used */
#define HMC_SYNC_RETRIES         ((uint8_t)3)     /* Number of application level retries */

#if ENABLE_PRNT_HMC_SYNC_INFO
#define HMC_SYNC_PRNT_INFO( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_SYNC_PRNT_INFO( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_SYNC_WARN
#define HMC_SYNC_PRNT_WARN( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_SYNC_PRNT_WARN( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_SYNC_ERROR
#define HMC_SYNC_PRNT_ERROR( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_SYNC_PRNT_ERROR( a, fmt,... )
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// <editor-fold defaultstate="collapsed" desc="Read Time Commands - Clock Table: ST52">
static const tReadPartial tblReadTimeTbl_[] =    /* Reads procedure results tbl to verify the procedure executed */
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_CLOCK),          /* As defined in psem.h */ /*lint !e572 !e778 */
   { 0, 0, 0 },                              /* Offset */               /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(MtrTimeFormatHighPrecision_t))   /* Length to read */       /*lint !e572 !e778 */
};

static const HMC_PROTO_TableCmd tblCmdReadTime_[] =  /* Information to send to the host meter. */
{
   { HMC_PROTO_MEM_PGM, (uint8_t)sizeof(tblReadTimeTbl_), (uint8_t far *)&tblReadTimeTbl_[0] },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 too few initializers   */
};

static const HMC_PROTO_Table tblReadTime_[] = /* Performs 2 operations, Full Tbl Wr (std proc 10) & reads results. */
{
   &tblCmdReadTime_[0],  /* Reads the meter time. */
   NULL              /* No other commands - NULL terminate. */
}; // </editor-fold>
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Definitions">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="uint8_t HMC_SYNC_applet(uint8_t ucCmd, void *pData)">
/* *********************************************************************************************************************
 *
 * Function name: HMC_SYNC_applet
 *
 * Purpose: Reads the meter time and pauses the HMC task until the meter time is >= the system time.  If the meter is
 *          more than x seconds behind the system time, the HMC task will not wait.
 *
 * Arguments: uint8_t ucCmd, void *pData
 *
 * Returns: uint8_t  - See cfg_app.h enum 'HMC_APP_API_RPLY' for response codes.
 *
 ******************************************************************************************************************** */
uint8_t HMC_SYNC_applet( uint8_t ucCmd, void *pData )
{
   uint8_t                retVal;                    /* Stores the return value */
   static uint8_t         retries_;                  /* Number of application level retries */
   static bool       bFeatureEnabled_ = true;   /* Flag to indicate this module is enabled or disabled */
   static bool       bDoMeterCom_ = false;      /* Flag that indicates HMC is required for this module. */

   retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;          /* Assume the applet is idle */

   switch ( ucCmd )                                /* Command passed in */
   {
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_DISABLE_MODULE">
      case (uint8_t)HMC_APP_API_CMD_DISABLE_MODULE:  /* Command to disable this applet? */
      {
         bFeatureEnabled_ = false;                 /* Disable the applet */
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ENABLE_MODULE">
      case (uint8_t)HMC_APP_API_CMD_ENABLE_MODULE:   /* Command to enable this applet? */
      {
         bFeatureEnabled_ = true;                  /* Enable the applet */
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_INIT">
      case (uint8_t)HMC_APP_API_CMD_INIT:            /* Initialize the applet? */
      {
         retries_ = HMC_SYNC_RETRIES;              /* Set application level retry count */
         bDoMeterCom_ = true;
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ACTIVATE">
      case (uint8_t)HMC_APP_API_CMD_ACTIVATE:        /* Activate command passed in? */
      {
         retries_ = HMC_SYNC_RETRIES;              /* Set application level retry count */
         bDoMeterCom_ = true;
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_STATUS">
      case (uint8_t)HMC_APP_API_CMD_STATUS:          /* Command to check to see if the applet needs communication */
      {
         /* Check if application is enabled AND applet is activated AND time is valid */
         if (bFeatureEnabled_ && bDoMeterCom_ && TIME_SYS_IsTimeValid() && (0 != retries_))
         {
            retVal = (uint8_t)HMC_APP_API_RPLY_READY_COM;
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_PROCESS">
      case (uint8_t)HMC_APP_API_CMD_PROCESS:         /* Command to get the table information to start comm. */
      {
         if ( bFeatureEnabled_ && bDoMeterCom_) /* Again, check to make sure the applet is enabled. */
         {
            ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *)tblReadTime_;
            retVal = (uint8_t)HMC_APP_API_RPLY_READY_COM;
         }
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_MSG_COMPLETE">
      case (uint8_t)HMC_APP_API_CMD_MSG_COMPLETE: /* Communication to the host meter was successful! */
      {
         sysTime_t sysTime;
         if (TIME_SYS_GetSysDateTime(&sysTime)) /* Get the system time. */
         {
            sysTime_t       mtrTimeSysFmt;
            MtrTimeFormatHighPrecision_t  *pMtrDateTime = /* Points to the rx buffer */
                              (MtrTimeFormatHighPrecision_t *)&((HMC_COM_INFO *)pData)->RxPacket.uRxData.sTblData.sRxBuffer[0];  /*lint !e826 area too small   */
            (void)TIME_UTIL_ConvertMtrHighFormatToSysFormat( pMtrDateTime, &mtrTimeSysFmt);
            uint64_t mtrDateTime = TIME_UTIL_ConvertSysFormatToSysCombined(&mtrTimeSysFmt) / TIME_TICKS_PER_SEC;
            uint64_t sysDateTime = TIME_UTIL_ConvertSysFormatToSysCombined(&sysTime) / TIME_TICKS_PER_SEC;
            if ((sysDateTime > mtrDateTime) && (sysDateTime <= (mtrDateTime + (TIME_ADJ_THRESHOLD_SEC + 1))))
            {  /* Pause HMC for the delta time of the meter to system. */
               HMC_SYNC_PRNT_INFO('I', "Sync: %d sec! ", (uint32_t)(sysDateTime - mtrDateTime));
               OS_TASK_Sleep ( (uint32_t)((sysDateTime - mtrDateTime) * 1000) );
            }
            else
            {
               HMC_SYNC_PRNT_INFO('I', "Sync - Sync'd");
            }
            bDoMeterCom_ = false;
         }
         else
         {
            retries_--; /* Okay, this should NEVER happen, but if it does, do a retry.  This should never happen because
                           this applet wouldn't have activated without valid time! */
         }
         break;
      }
      // </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_TBL_ERROR">
      case (uint8_t)HMC_APP_API_CMD_TBL_ERROR: /* The response code contains an error. */
      {
         if (RESP_DLK == ((HMC_COM_INFO *)pData)->RxPacket.ucResponseCode)  /* Data Locked Error? */
         {
            retVal = (uint8_t)HMC_APP_API_RPLY_ABORT_SESSION;  /* Abort the session */
         }
         else
         {
            if (0 != retries_)
            {
               retries_--;
            }
            else
            {
               HMC_SYNC_PRNT_ERROR('E',  "Sync Failed - TBL");
               bDoMeterCom_ = false;
               retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;
            }
         }
         break;
      }
      // FALLTHROUGH
      // </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ABORT">
      case (uint8_t)HMC_APP_API_CMD_ABORT:  /* Communication protocol error, session was aborted */
      {
         if (0 != retries_)
         {
            retries_--;
         }
         else
         {
            HMC_SYNC_PRNT_ERROR('E',  "Time Sync Failed - Abort");
            bDoMeterCom_ = false;
            retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;
         }
         break;
      }
      // </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ERROR">
      case (uint8_t)HMC_APP_API_CMD_ERROR:  /* Command error */
      {
         HMC_SYNC_PRNT_ERROR('E',  "Sync Failed - Command Error");
         bDoMeterCom_ = false;
         retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;
         break;
      }// </editor-fold>
      // <editor-fold defaultstate="collapsed" desc="default">
      default: /* No other commands are supported, if we get here, the command was not recognized by this applet. */
      {
         retVal = (uint8_t)HMC_APP_API_RPLY_UNSUPPORTED_CMD; /* Respond with unsupported command. */
         break;
      }// </editor-fold>
   }
   if (0 == retries_)            /* Have all the retries been exhausted? */
   {
     bDoMeterCom_ = false;   /* No more retries, switch to idle state */
     /* Todo:  Send Error Alarm to something!  */
   }
   return(retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>
