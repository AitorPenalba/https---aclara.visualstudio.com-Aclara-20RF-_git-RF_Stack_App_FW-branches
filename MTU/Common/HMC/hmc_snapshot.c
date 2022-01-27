// <editor-fold defaultstate="collapsed" desc="File Header Information">
/***********************************************************************************************************************

   Filename:   hmc_snapshot.c

   Global Designator: HMC_SS_

   Contents: Sends the snap shot procedure to the host meter.  This is required for KV2 meters.

   NOTE: For this module to work properly, it must be higher priority in the hmc_tsk_lst module than any other applet
         that reads data that requires a snapshot.

   Per the KV2 Document:
   This procedure will make a copy of all revenue data, as well as the voltage event monitor, load profile and the event
   log, at the point in time that the procedure is executed.  This provides the mechanism for insuring that a consistent
   set of data can be read by the reading software.  The procedure should be executed after logon and prior to reading
   or writing any revenue data tables in the communication session.  If revenue tables are written during the session,
   the Stop programming procedure should be executed before the session is terminated.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013-2016 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author Karl Davlin

   $Log$ Created April 4, 2013

 ***********************************************************************************************************************
   Revision History:
   v0.1 - KAD 04/04/2013 - Initial Release

   @version    0.1
   #since      2013-04-04
 **********************************************************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "hmc_prg_mtr.h"
#endif
#define hmc_snapshot_GLOBALS
#include "hmc_snapshot.h"
#undef  hmc_snapshot_GLOBALS
#include "psem.h"
#if ( ANSI_STANDARD_TABLES == 1 )
#include "ansi_tables.h"
#endif
#if ( HMC_SNAPSHOT == 1 )
#include "hmc_prot.h"
#include "hmc_seq_id.h"
#include "meter_ansi_sig.h"
#include "DBG_SerialDebug.h"
#endif
#if ( ANSI_LOGON == 1 )
#include "hmc_start.h"
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

//lint -esym(750,HMC_SS_PRNT_INFO,HMC_SS_PRNT_WARN,HMC_SS_PRNT_ERROR)

#if ( HMC_SNAPSHOT == 1 )
#define HMC_SS_RETRIES           ((uint8_t)1) /* Number of retries */
#endif

#if ENABLE_PRNT_HMC_SS_INFO
#define HMC_SS_PRNT_INFO( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_SS_PRNT_INFO( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_SS_WARN
#define HMC_SS_PRNT_WARN( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_SS_PRNT_WARN( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_SS_ERROR
#define HMC_SS_PRNT_ERROR( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_SS_PRNT_ERROR( a, fmt,... )
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Variables (File Static)">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#if ( HMC_SNAPSHOT == 1 )
static HMC_SS_Status_t  snapshotStatus_         = eHMC_SS_NOT_SNAPPED;
/* The procedure # and ID are constants that are sent to the host meter */
static procNumId_t      snapshotData_;  /* Procedure Number MSB and LSB & Procedure ID (Sequence Number) */
static bool             bFeatureEnabled_ = true;
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constants">
/* ****************************************************************************************************************** */
/* CONSTANTS */

static const tWriteFull Table_SnapShot_[] =
{
   CMD_TBL_WR_FULL,
   BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE ),    /* ST.Tbl 7 */
};

static const HMC_PROTO_TableCmd Cmd_SnapShot_[] =
{
   {HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( Table_SnapShot_ ), ( uint8_t far * )&Table_SnapShot_[0]},
   {HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( snapshotData_ ),  ( uint8_t far * )&snapshotData_}, /* Proc and ID */
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few initializers   */
};

static const HMC_PROTO_Table tblSnapShot_[] = /* Performs 2 operations, Full Tbl Wr (std proc 10) & reads results. */
{
   &Cmd_SnapShot_[0],  /* MFG Procedure 70 (Full Table Write) - Perform Snapshot. */
   NULL                /* No other commands - NULL terminate. */
};
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="uint8_t HMC_SS_applet(uint8_t cmd, void *pData)">
/***********************************************************************************************************************

   Function Name: HMC_SS_applet

   Purpose: Executes a snap shot procedure

   Arguments: uint8_t cmd, void *pData

   Returns: uint8_t

   Side Effects: N/A

   Re-entrant Code: No

   Notes:   Care must be taken to not perform the snap shot too often.  This
            could have an impact on the host meter NV memory endurance.

 **********************************************************************************************************************/
//lint -efunc(818,HMC_SS_applet)
uint8_t HMC_SS_applet( uint8_t cmd, void *pData )
{
   uint8_t   retVal = ( uint8_t )HMC_APP_API_RPLY_IDLE; /* Stores the return value */

#if ( HMC_SNAPSHOT == 1 )
   static bool bDoCom_ = false;
   static uint8_t   retries_;

   switch( cmd )                                   /* Command passed in */
   {
      case HMC_APP_API_CMD_DISABLE_MODULE:         /* Command to disable this applet? */
      {
         bFeatureEnabled_ = false;                 /* Disable the applet */
         snapshotStatus_ = eHMC_SS_DISABLED;
         break;
      }
      case HMC_APP_API_CMD_ENABLE_MODULE:          /* Command to enable this applet? */
      {
         bFeatureEnabled_ = true;                  /* Enable the applet */
         break;
      }
      case HMC_APP_API_CMD_INIT:                   /* Initialize the applet? */
      {
         bDoCom_ = false;                          /* Assume no meter communication is needed */
         retries_ = HMC_SS_RETRIES;                /* Set application level retry count */
         bDoCom_ = true;                           /* Go ahead and do meter communication */
         break;
      }
      case HMC_APP_API_CMD_ACTIVATE:               /* Activate command passed in? */
      {
         if ( eHMC_SS_SNAPPED != snapshotStatus_ ) /* Will do a snapshot if it hasn't been done. */
         {
            retries_ = HMC_SS_RETRIES;             /* Set application level retry count */
            bDoCom_ = true;                        /* Go ahead and do meter communication */
         }
         break;
      }
      case HMC_APP_API_CMD_STATUS:                 /* Command to check to see if the applet needs communication */
      {
         if ( bFeatureEnabled_ )
         {
            if ( bDoCom_ ) /* Has this module been activated? */
            {
               if ( eHMC_SS_SNAPPED == snapshotStatus_ ) /* Has snapshot already been executed for this session? */
               {
                  bDoCom_ = false;  /* Yes, snapshot has been done, don't do it again. */
               }
               else
               {
                  retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST; /* Since doing a proc, set to persistent */
               }
            }
         }
         if ( HMC_APP_API_RPLY_LOGGED_IN !=
               ( enum HMC_APP_API_RPLY )HMC_STRT_LogOn( ( uint8_t )HMC_APP_API_CMD_LOGGED_IN, ( uint8_t far * )0 ) )
         {
            snapshotStatus_ = eHMC_SS_NOT_SNAPPED;
         }
         break;
      }
      case HMC_APP_API_CMD_PROCESS:                /* Command to get the table information to start comm. */
      {
         if ( bFeatureEnabled_ && bDoCom_ )        /* Again, check to make sure the applet is enabled. */
         {
            /* The mfg proc num is always the same. */
            snapshotData_.procNum = ( uint16_t )MFG_PROC_SNAPSHOT_REVENUE_DATA;
            snapshotData_.procId = HMC_getSequenceId();        /* Get a unique sequence ID */
            ( ( HMC_COM_INFO * )pData )->pCommandTable = ( uint8_t far * )tblSnapShot_;
            retVal = ( uint8_t )HMC_APP_API_RPLY_RDY_COM_PERSIST;
         }
         break;
      }
      case HMC_APP_API_CMD_MSG_COMPLETE: /* Communication to the host meter was successful! */
      {
         HMC_SS_PRNT_INFO( 'I',  "SS - Snapshot Completed" );
         bDoCom_ = false;                       /* The time set is complete, set applet to inactive. */
         snapshotStatus_ = eHMC_SS_SNAPPED;     /* The snapshot has been successfully performed!  */
         break;                                 /* Done, break-out */
      }
      case HMC_APP_API_CMD_LOG_OFF:
      {
          snapshotStatus_ = eHMC_SS_NOT_SNAPPED;     /* The snapshot must be depreiated if logging out*/
          HMC_SS_PRNT_INFO( 'I',  "SS - Snapshot Depriacted" );
          break;
      }
      case HMC_APP_API_CMD_TBL_ERROR: /* The response code contains an error. */
      {
         /* If we get here, the snapshot can't be performed.  Either the message definition is wrong or the meter
            doesn't support a snapshot.  Either way, there is no point retrying */
         bDoCom_ = false;
         snapshotStatus_ = eHMC_SS_ERROR;
         HMC_SS_PRNT_ERROR( 'E',  "SS - Tbl" );
         retVal = ( uint8_t )HMC_APP_API_RPLY_ABORT_SESSION; /* Abort the session, something is very wrong */
         break;
      }
      case HMC_APP_API_CMD_ABORT:  /* Communication protocol error, session was aborted */
      {
         if ( 0 != retries_ )
         {
            retries_--;
            retVal = ( uint8_t )HMC_APP_API_RPLY_ABORT_SESSION; /* Abort the session, something is very wrong */
         }
         else
         {
            HMC_SS_PRNT_ERROR( 'E',  "SS - Session Aborted" );
            bDoCom_ = false;  /* No retries left, stop trying to perform the snapshot */
         }
         break;
      }
      case HMC_APP_API_CMD_ERROR:  /* Command error - File Issue? */
      {
         HMC_SS_PRNT_ERROR( 'E',  "SS - Command Error" );
         bDoCom_ = false;
         break;
      }
      default: /* No other commands are supported, if we get here, the command was not recognized by this applet. */
      {
         retVal = ( uint8_t )HMC_APP_API_RPLY_UNSUPPORTED_CMD; /* Respond with unsupported command. */
         break;
      }
   }
#endif
   return( retVal );
}//lint !e715 pData not referenced
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="HMC_SS_Status_t HMC_SS_isSnapped(void)">
/***********************************************************************************************************************

   Function Name: HMC_SS_isSnapped

   Purpose: Returns the status of the snapshot module

   Arguments: None

   Returns: HMC_SS_Status_t - current state

   Side Effects: N/A

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
HMC_SS_Status_t HMC_SS_isSnapped( void )
{
#if ( HMC_SNAPSHOT == 1 )
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   if ( HMC_PRG_MTR_IsSynced( ) ) // RCZ added
   {
       return( eHMC_SS_SNAPPED );
   }
#endif
   return( snapshotStatus_ );
#else
   return( eHMC_SS_SNAPPED );
#endif
}
/* ****************************************************************************************************************** */
// </editor-fold>
