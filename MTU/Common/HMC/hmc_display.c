/***********************************************************************************************************************
 *
 * Filename: hmc_display.c
 *
 * Global Designator: HMC_DISP_
 *
 * Contents: Sets the host meter's display LCD.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 * Revision History:
 * 31/03/2020 - Initial Version
 ******************************************************************************************************************** */

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
#include "hmc_display.h"
#include "hmc_start.h"
#include "meter_ansi_sig.h"

#if ( PHASE_DETECTION == 1 )
#include "time_sys.h"
#include "PhaseDetect.h"
#endif

/* TYPE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */
static uint8_t    DisplayNewBuf_[HMC_DISP_TOTAL_BUFF_LENGTH];
static uint8_t    DisplayOldBuf_[HMC_DISP_TOTAL_BUFF_LENGTH];
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
static uint8_t    HMC_DISP_ContinuousDisplayUpdate_ = 0;
#define CONTINUOUS 0xA5
#endif

static uint8_t    password_[METER_PASSWORD_SIZE] = {0};               /* used to store the reader access password */
static uint8_t    mPassword_[METER_PASSWORD_SIZE] = {0};              /* used to store the master access password */

/* MACRO DEFINITIONS */
/*lint -esym(750,HMC_SYNC_PRNT_INFO,HMC_SYNC_PRNT_WARN,HMC_SYNC_PRNT_ERROR)   not used */
#define HMC_DISP_RETRIES         ((uint8_t)2)     /* Number of application level retries */

#if ENABLE_PRNT_HMC_DISP_INFO
#define HMC_DISP_PRNT_INFO( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_DISP_PRNT_INFO( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_SYNC_WARN
#define HMC_DISP_PRNT_WARN( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_DISP_PRNT_WARN( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_SYNC_ERROR
#define HMC_DISP_PRNT_ERROR( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_DISP_PRNT_ERROR( a, fmt,... )
#endif

/* CONSTANTS */
static cmdFnct_t const     *pCurrentCmd_ = NULL;         /* Ptr to command being executed.
                                                            pCurrentCmd_ is used also as a flag to indicate that the
                                                            applet is idle when NULL. */

static const uint8_t SecurityStr[] = { CMD_PASSWORD };

static const HMC_PROTO_TableCmd Security[] =
{
   {
      HMC_PROTO_MEM_PGM, sizeof(SecurityStr), (uint8_t far *)SecurityStr
   },
   {
      HMC_PROTO_MEM_PGM, sizeof(password_), password_
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
      HMC_PROTO_MEM_PGM, sizeof(mPassword_), mPassword_
   },
   {
      HMC_PROTO_MEM_NULL
   }  /*lint !e785 too few initializers   */
};

static const tWritePartial Table_MtrDisp[] =
{
   { /* Meter Display Information */
      CMD_TBL_WR_PARTIAL,
      BIG_ENDIAN_16BIT( MFG_TBL_NETWORK_STATUS_DISPLAY_DATA ),   /* MFG.Tbl 119 will be written with */
      {
         (uint8_t) 0, (uint8_t) 0, (uint8_t) 0    /* Offset MFG.Tbl 119                        */
      },
      BIG_ENDIAN_16BIT((uint16_t)ARRAY_IDX_CNT(DisplayNewBuf_))    /* Length in bytes for the MFG.Tbl 119       */
   }
};

static const HMC_PROTO_TableCmd Cmd_MtrDisp[] =
{
   {(HMC_PROTO_MEM_PGM  ), (uint8_t)sizeof(Table_MtrDisp),         (uint8_t far *)&Table_MtrDisp[0]},
   {(HMC_PROTO_MEM_PGM  ), (uint8_t)ARRAY_IDX_CNT(DisplayNewBuf_), (uint8_t far *)&DisplayNewBuf_[0]},
   {(HMC_PROTO_MEM_NULL ), (uint8_t)0,                             (uint8_t far *)0}
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

static const HMC_PROTO_Table Tables_MtrDisp[] =
{
   {  Cmd_MtrDisp },
   {  (void *)NULL   }
};

static const cmdFnct_t MtrDisplayTable_[] =
{
   { (HMC_PROTO_Table  const *)&MasterSecurityTbl[0], NULL },
   { (HMC_PROTO_Table  const *)&Tables_MtrDisp[0], NULL },
   { (HMC_PROTO_Table  const *)&SecurityTbl[0], NULL },
   {NULL}
};

static const cmdFnct_t * const pMtrDisplayTable_[] =
{
   &MtrDisplayTable_[0]
};

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */


/* *********************************************************************************************************************
 *
 * Function name: HMC_DISP_applet
 *
 * Purpose: Writes to the Meters MT119 Display table to show the messages on the LCD
 *
 * Arguments: uint8_t ucCmd, void *pData
 *
 * Returns: uint8_t  - See cfg_app.h enum 'HMC_APP_API_RPLY' for response codes.
 *
 ******************************************************************************************************************** */
uint8_t HMC_DISP_applet( uint8_t ucCmd, void *pData )
{
   uint8_t                 retVal;                    /* Stores the return value */
   static uint8_t          retries_ = 0;                  /* Number of application level retries */
   static bool             bWriteToMeter_	= false;

   retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;          /* Assume the applet is idle */

   switch ( ucCmd )                                /* Command passed in */
   {
     case (uint8_t)HMC_APP_API_CMD_STATUS:
	  {
#if ( MCU_SELECTED == NXP_K24 )
         HMC_DISP_PRNT_INFO( 'I',  "DISP  HMC_APP_API_CMD_STATUS0" ); // This is pretty useless debug
#endif
         if (bWriteToMeter_)
         {
            HMC_DISP_PRNT_INFO( 'I',  "DISP  HMC_APP_API_CMD_STATUS1" );
            retVal = (uint8_t)HMC_APP_API_RPLY_READY_COM;
         }
         break;
	  }
	  case (uint8_t)HMC_APP_API_CMD_ACTIVATE:
      {
         HMC_DISP_PRNT_INFO( 'I',  "DISP  HMC_APP_API_CMD_ACTIVATE" );
         bWriteToMeter_ = true;
         retries_ = HMC_DISP_RETRIES;
         /* Get the latest passwords and update it to security cmds */
         (void)HMC_STRT_GetPassword( password_ );
         (void)HMC_STRT_GetMasterPassword( mPassword_ );
         /* Point to the first cmd of the display table */
         pCurrentCmd_   = pMtrDisplayTable_[0];
         break;
      }
      case (uint8_t)HMC_APP_API_CMD_PROCESS:
      {
         HMC_DISP_PRNT_INFO( 'I',  "DISP  HMC_APP_API_CMD_PROCESS" );
         if ( NULL != pCurrentCmd_ )
         {
            /* Process the applet until all commands get executed */
            retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            /* Point to the command to execute in the table */
            ((HMC_COM_INFO *) pData)->pCommandTable = (uint8_t far *)pCurrentCmd_->pCmd;
         }
         else
         {
            bWriteToMeter_ = false;
         }
         break;
      }
      case (uint8_t)HMC_APP_API_CMD_MSG_COMPLETE:
      {
         HMC_DISP_PRNT_INFO( 'I',  "DISP  HMC_APP_API_CMD_MSG_COMPLETE" );
         if ( NULL != pCurrentCmd_ )  /* Do not do anything if we get here with a NULL in pCurrentCmd_ */
         {
            /* Check if the current successful cmd is table_mtrdisp  */
            if ( (HMC_PROTO_Table  const *)&Tables_MtrDisp[0] == pCurrentCmd_->pCmd )
            {
               HMC_DISP_PRNT_INFO( 'I',  "DISP  Copying the buffers");
               (void)memcpy( DisplayOldBuf_, DisplayNewBuf_, HMC_DISP_TOTAL_BUFF_LENGTH );
#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
               if ( HMC_DISP_ContinuousDisplayUpdate_ == CONTINUOUS )
               {
                  pCurrentCmd_--; /* Back up and repeat the write to Table 119 without logging out and back in */
               }
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
            }
            pCurrentCmd_++;
            /* Is the next item in the table NULL?  If so, that is all, we're all done.  */
            /* Handler may have aborted, so check pCurrentCmd_ first!   */
            if ( ( NULL != pCurrentCmd_ ) && ( NULL == pCurrentCmd_->pCmd) )
            {
               HMC_DISP_PRNT_INFO( 'I',  "DISP  All commands executed" );
               /* All commands in the table executed */
               pCurrentCmd_   = NULL;
               bWriteToMeter_ = false;
            }
            else
            {
               retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
          }
         break;
      }
      case (uint8_t)HMC_APP_API_CMD_INIT:
      {
         HMC_DISP_PRNT_INFO( 'I',  "DISP  HMC_APP_API_CMD_INIT" );
         break;
      }
      case (uint8_t)HMC_APP_API_CMD_TBL_ERROR:
      case (uint8_t)HMC_APP_API_CMD_ABORT:
      case (uint8_t)HMC_APP_API_CMD_ERROR:
      {
         /* Try the same command again */
         retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
         if ( 0 == ( retries_-- ) )
         {
            /* Skip to next command in sequence */
            pCurrentCmd_++;
            /* Is the next item in the table NULL?  If so, that is all, we're all done.  */
            if ( NULL == pCurrentCmd_ )
            {
               pCurrentCmd_   = NULL;
               bWriteToMeter_ = false;
               retVal = (uint8_t)HMC_APP_API_RPLY_IDLE;          /* Assume the applet is idle */
            }
         }
         break;
      }
      default:
      {
         retVal = (uint8_t)HMC_APP_API_RPLY_UNSUPPORTED_CMD;
         break;
      }

   }
   return( retVal );
}

/* *********************************************************************************************************************
 *
 * Function name: HMC_DISP_Init
 *
 * Purpose: Initialize the buffers and Set/Update PD LCD Message
 *
 * Arguments: None
 *
 * Returns: eSUCCESS
 *
 ******************************************************************************************************************** */
returnStatus_t HMC_DISP_Init ( void )
{
#if ( PHASE_DETECTION == 1 )
   char pdLcdMessage[PD_LCD_MSG_SIZE+1];

   memset( pdLcdMessage, 0, sizeof( pdLcdMessage ) );
#endif
   (void)memset( DisplayNewBuf_, 0, HMC_DISP_TOTAL_BUFF_LENGTH );
   (void)memset( DisplayOldBuf_, 0, HMC_DISP_TOTAL_BUFF_LENGTH );

#if ( PHASE_DETECTION == 1 )
   ( void )PD_LcdMsg_Get( pdLcdMessage );
    HMC_DISP_UpdateDisplayBuffer( pdLcdMessage, HMC_DISP_POS_PD_LCD_MSG ); //Update the Display
#endif
   return ( eSUCCESS );
}

/* *********************************************************************************************************************
 *
 * Function name: HMC_DISP_UpdateDisplayBuffer
 *
 * Purpose: To set the messages to the LCD by writing to the Meter MT119 Table
 *
 * Arguments: void const *msg, uint8_t pos
 *
 * Returns: None
 *
 ******************************************************************************************************************** */
void HMC_DISP_UpdateDisplayBuffer ( void const *msg, uint8_t pos )
{
   uint8_t              buf[HMC_DISP_MSG_LENGTH];

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   if ( ( HMC_DISP_ContinuousDisplayUpdate_ != CONTINUOUS ) ||
        ( 0 == memcmp( msg, HMC_DISP_MSG_NOISEBAND, HMC_DISP_MSG_LENGTH ) ) )
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
   {
      /* Check whether position is valid or not */
      if ( HMC_DISP_POS_NIC_MODE >= pos )
      {
         /* Clear the buffer */
         (void)memset( buf, 0, sizeof( buf ) );
         /* Copy the messag to the local buffer */
         (void)memcpy( buf, msg, HMC_DISP_MSG_LENGTH );
         /* Copy the messag to the new buffer */
         (void)memcpy( &DisplayNewBuf_[pos*HMC_DISP_MSG_LENGTH], buf, HMC_DISP_MSG_LENGTH );
         /* Compare the old and new buffers */
         if( 0 != memcmp( DisplayNewBuf_, DisplayOldBuf_, HMC_DISP_TOTAL_BUFF_LENGTH ) )
         {
            /* Trigger the applet to write the new values to the meter */
            HMC_DISP_PRNT_INFO( 'I',  "DISP  TRIGGER %s  %d", msg, pos );
            (void)HMC_DISP_applet( (uint8_t)HMC_APP_API_CMD_ACTIVATE, NULL );
         }
      }
   }
}

#if ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
/* *********************************************************************************************************************
 *
 * Function name: HMC_DISP_SetContinuousUpdate
 *
 * Purpose: Causes the HMC_DISP applet to continuously write the most recently requested string to the meter's display
 *          as fast as possible to create as much HMC UART traffic as possible during noiseband testing
 *
 * Arguments: None
 *
 * Returns: None
 *
 ******************************************************************************************************************** */
void HMC_DISP_SetContinuousUpdate( void )
{
   HMC_DISP_ContinuousDisplayUpdate_ = CONTINUOUS;
}

/* *********************************************************************************************************************
 *
 * Function name: HMC_DISP_ClearContinuousUpdate
 *
 * Purpose: Reverts the HMC_DISP applet back into its normal mode for writes to the meter's display
 *
 * Arguments: None
 *
 * Returns: None
 *
 ******************************************************************************************************************** */
void HMC_DISP_ClearContinuousUpdate( void )
{
   HMC_DISP_ContinuousDisplayUpdate_ = 0;
}
#endif // ( TM_ENHANCE_NOISEBAND_FOR_RA6E1 == 1 )
#endif
