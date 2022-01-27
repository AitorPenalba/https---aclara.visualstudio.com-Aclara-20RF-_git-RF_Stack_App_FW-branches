/***********************************************************************************************************************
 *
 * Filename: RG_CF_Handler.c
 *
 * Global Designator: RG_CF_
 *
 * Contents:  Handles New Endpoint Registration Confirmation (/rg/cf resource)
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************
 *
 * $Log$ cmusial Created Mar 20, 2015
 *
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include <stdbool.h>

//lint -esym(750,RG_CF_GLOBALS)
#define RG_CF_GLOBALS
#include "RG_CF_Handler.h"
#undef RG_CF_GLOBALS

#include "ansi_tables.h"
#include "time_util.h"
#include "timer_util.h"
#include "DBG_SerialDebug.h"
#include "APP_MSG_Handler.h"
#include "RG_MD_Handler.h"
#include "ID_intervalTask.h"

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* GLOBAL VARIABLE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */

/* LOCAL FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */


/***********************************************************************************************************************
 *
 * Function Name: RG_CF_MsgHandler
 *
 * Purpose: Handle RG_CF (New Endpoint Registration Confirmation)
 *
 * Arguments:
 *     APP_MSG_Rx_t *appMsgRxInfo - Pointer to application header structure from head end
 *     void *payloadBuf           - pointer to data payload
 *     length                     - payload length
 *
 * Side Effects: none
 *
 * Re-entrant: No
 *
 * Returns: none
 *
 * Notes:
 *
 ***********************************************************************************************************************/
//lint -esym(715, payloadBuf) not referenced
//lint -efunc(818, RG_CF_MsgHandler) arguments cannot be const because of API requirements
void RG_CF_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length)
{
   (void)length;
   if ( method_put == (enum_MessageMethod)heepReqHdr->Method_Status)
   { //Process the put request
       HEEP_APPHDR_t  heepRespHdr;      // Application header/QOS info

      /***
      * For now there won't be any configuration parameters coming across, but
      * that will change in the future.  We'll consider receipt of this message
      * sufficient notification that the head-end received the /rg/md message,
      * so bubble-up can proceed.
      ***/
      if (metadataRegTimerId != 0)
      {   // Stop the timer for re-sending the metadata registration
        (void)TMR_DeleteTimer(metadataRegTimerId);
        metadataRegTimerId = 0;
      }
      RegistrationInfo.registrationStatus = REG_STATUS_CONFIRMED;   // registration is complete
      RegistrationInfo.nextRegistrationTimeout = RegistrationInfo.initialRegistrationTimeout;
      APP_MSG_updateRegistrationTimeout();   // sets epRegistrationInfo.activeRegistrationTimeout to a random value
      APP_MSG_UpdateRegistrationFile();

      //Application header/QOS info
      HEEP_initHeader( &heepRespHdr );
      heepRespHdr.TransactionType = (uint8_t)TT_RESPONSE;
      heepRespHdr.Resource        = (uint8_t)rg_cf;
      heepRespHdr.Method_Status   = (uint8_t)OK;
      heepRespHdr.Req_Resp_ID     = (uint8_t)heepReqHdr->Req_Resp_ID;
      heepRespHdr.qos             = (uint8_t)heepReqHdr->qos;
      heepRespHdr.callback        = NULL;
      heepRespHdr.TransactionContext = heepReqHdr->TransactionContext;


      (void)HEEP_MSG_TxHeepHdrOnly (&heepRespHdr);
   }
   //else Other methods not supported
}//lint !e715 !e818
//lint +esym(715, payloadBuf) not referenced

