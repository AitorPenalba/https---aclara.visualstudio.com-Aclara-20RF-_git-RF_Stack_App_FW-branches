/* ************************************************************************* */
/******************************************************************************
 *
 * Filename:   hmc_finish.c
 *
 * Global Designator: HMC_FINISH_
 *
 * Contents: ends the meter communication session
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log$ KAD Created 2008, 9/8
 *
 ******************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 *************************************************************************** */

/* ************************************************************************* */
/* PSECTS */

#ifdef USE_MAPPING
#pragma psect text=tatxt
#pragma psect const=tacnst
#pragma psect bss=tabss
#pragma psect bigbss=tabbss
#endif

/* ************************************************************************* */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "project.h"
#include "hmc_prot.h"
#include "hmc_msg.h"
#include "hmc_finish.h"
#include "hmc_start.h"
#include "hmc_snapshot.h"
#include "DBG_SerialDebug.h"

/* ************************************************************************* */
/* CONSTANTS */
static const uint8_t terminateStr_[]= {CMD_TERMINATE}; // 0x21

#if 0
static const uint8_t logOffStr_[]   = {CMD_LOG_OFF};
static const HMC_PROTO_TableCmd logOff_[] =
{
   HMC_PROTO_MEM_PGM,
   sizeof(logOffStr_),
   (uint8_t far *)logOffStr_,
   HMC_PROTO_MEM_NULL
};
#endif
static const HMC_PROTO_TableCmd terminate_[] =
{
   {
      HMC_PROTO_MEM_PGM,
      sizeof(terminateStr_),
      (uint8_t far *)terminateStr_,
   },
   {
      HMC_PROTO_MEM_NULL
   }  /*lint !e785 too few initializers   */
};

static const HMC_PROTO_Table closeSessionTbl_[] =
{
   {
      terminate_
   },
   {
      (void *)NULL
   }
};

/* ************************************************************************* */
/* MACRO DEFINITIONS */

#if ENABLE_PRNT_HMC_FINISH_INFO
#define HMC_FINISH_PRNT_INFO(x,y)        DBG_logPrintf(x,y)
#else
#define HMC_FINISH_PRNT_INFO(x,y)
#endif

#if ENABLE_PRNT_HMC_FINISH_WARN
//#define HMC_FINISH_PRNT_WARN(x,y)      DBG_logPrintf(x,y)
#else
//#define HMC_FINISH_PRNT_WARN(x,y)
#endif

#if ENABLE_PRNT_HMC_FINISH_ERROR
#define HMC_FINISH_PRNT_ERROR(x,y)       DBG_logPrintf(x,y)
#else
#define HMC_FINISH_PRNT_ERROR(x,y)
#endif

/* ************************************************************************* */
/* TYPE DEFINITIONS */


/* ************************************************************************* */
/* FILE VARIABLE DEFINITIONS */
#if (NEGOTIATE_HMC_BAUD == 1)
static bool firstLogOff = false;
#endif

/* ************************************************************************* */
/* FUNCTION PROTOTYPES */

/* ************************************************************************* */
/* FUNCTION DEFINITIONS */

/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: HMC_FINISH_LogOff()
 *
 * Purpose: Logs off of the host meter
 *
 * Arguments: uint8_t - Command
 *
 * Returns: uint8_t - STATUS
 *
 *****************************************************************************/
uint8_t HMC_FINISH_LogOff(uint8_t ucCmd, void far *pPtr)
{
   static uint8_t update_ = (uint8_t)HMC_APP_API_RPLY_IDLE;

   switch(ucCmd)
   {
      case HMC_APP_API_CMD_ACTIVATE:
      {
          // HMC_STRT_logInStatus() always returns non-zero, so "else" is
          // never executed.  RCZ
         if (HMC_STRT_logInStatus())  // returns loggedIn_
         {
            update_ = (uint8_t)HMC_APP_API_RPLY_READY_COM;
#if ( NEGOTIATE_HMC_BAUD == 1)
            firstLogOff = true;
#endif
         }
         else
         {
            update_ = (uint8_t)HMC_APP_API_RPLY_IDLE;
         }
         break;
      }
      case HMC_APP_API_CMD_INIT:
      {
         update_ = (uint8_t)HMC_APP_API_RPLY_IDLE;
         break;
      }
      case HMC_APP_API_CMD_PROCESS:
      {
         HMC_FINISH_PRNT_INFO('I',"Finish - Logging Off");
         ((HMC_COM_INFO *)pPtr)->pCommandTable = (uint8_t far *)&closeSessionTbl_[0];
         update_ = (uint8_t)HMC_APP_API_RPLY_READY_COM;
         break;
      }
      case HMC_APP_API_CMD_STATUS:
      {
         break;
      }
      case HMC_APP_API_CMD_ABORT:
      case HMC_APP_API_CMD_ERROR:
      {
         if ( (uint8_t)HMC_APP_API_CMD_ABORT == ucCmd )
         {
            HMC_FINISH_PRNT_ERROR('E',"Finish - Log Off Aborted");
         }
         else
         {
            HMC_FINISH_PRNT_ERROR('E',"Finish - Log Off Error");
         }
         HMC_STRT_loggedOff(); // Sets  loggedIn_ = RPLY_LOGGED_OFF, update_ = RPLY_IDLE
         HMC_SS_applet( HMC_APP_API_CMD_LOG_OFF, NULL);
         update_ = (uint8_t)HMC_APP_API_RPLY_IDLE;
         HMC_STRT_RestartDelay();
#if ( HMC_I210_PLUS_C == 1 )
         HMC_MUX_CTRL_RELEASE();
#endif
         break;
      }
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {
#if (NEGOTIATE_HMC_BAUD == 1)
        if( true == firstLogOff )
        {
          if(((HMC_COM_INFO *)pPtr)->RxPacket.ucResponseCode != RESP_OK )
          {
            //we failed to terminate the session, try again at the deafult baudrate
            HMC_STRT_SetDefaultBaud(); //return uart to default baudrate
            update_ = (uint8_t)HMC_APP_API_RPLY_READY_COM;
            firstLogOff = false;
          }
          else
          {
            HMC_FINISH_PRNT_INFO('I',"Finish - Logged Off");
            HMC_STRT_loggedOff();
            HMC_SS_applet( HMC_APP_API_CMD_LOG_OFF, NULL);
            update_ = (uint8_t)HMC_APP_API_RPLY_IDLE;
            firstLogOff = false;
          }
        }
        else
#endif
        {
         HMC_FINISH_PRNT_INFO('I',"Finish - Logged Off");
         HMC_STRT_loggedOff();
         HMC_SS_applet( HMC_APP_API_CMD_LOG_OFF, NULL);
         update_ = (uint8_t)HMC_APP_API_RPLY_IDLE;

#if ( HMC_I210_PLUS_C == 1 )
         HMC_MUX_CTRL_RELEASE();
#endif
        }
         break;
      }
      default:
      {
         break;
      }
   }
   return(update_);
}
/* ************************************************************************* */
