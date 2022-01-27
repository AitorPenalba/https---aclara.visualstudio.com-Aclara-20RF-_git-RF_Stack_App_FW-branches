/** ****************************************************************************
@file nw_connected.c

Contains the routines to support the SRFN Registration.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include "buffer.h"
#include "project.h" // Note: due to structure of sm.h all these headers are required
#include "PHY_Protocol.h"
#include "SM_Protocol.h"
#include "SM.h"

#include "nw_connected.h"

/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/

/*******************************************************************************
Local #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
*******************************************************************************/

/*******************************************************************************
Local Const Definitions
*******************************************************************************/

/*******************************************************************************
Static Function Prototypes
*******************************************************************************/

static void NwConnectedIndHandler(buffer_t *indication);

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Initialize network connection tracking.

@return eSUCCESS
*/
returnStatus_t NwConnectedInit(void)
{
   SIGNAL_NETWORK_DOWN();

   SM_RegisterEventIndicationHandler(NwConnectedIndHandler);

   return eSUCCESS;
}

/**
Handles indications from the stack manager and tracks current network status.

@param indication Buffer with SM indication.
*/
static void NwConnectedIndHandler(buffer_t *indication)
{
   if (indication->eSysFmt == eSYSFMT_SM_INDICATION)
   {
      SM_Indication_t *msg = (SM_Indication_t*)indication->data; /*lint !e826 */

      if ((msg->Type == eSM_EVENT_IND) && (msg->EventInd.type == (uint8_t)SM_EVENT_NW_STATE))
      {
         if (msg->EventInd.nwState.networkConnected)
         {
            SIGNAL_NETWORK_UP();
         }
         else
         {
            SIGNAL_NETWORK_DOWN();
         }
      }
   }

   // clean up the indication memory now that it was serviced
   BM_free(indication);
}
