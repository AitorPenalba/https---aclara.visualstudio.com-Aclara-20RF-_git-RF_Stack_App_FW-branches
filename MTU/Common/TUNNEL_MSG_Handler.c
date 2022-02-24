/** ****************************************************************************
@file tunnel_msg_handler.c

General tunneling message retrieval/storage functions.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

/* INCLUDE FILES */
#include "tunnel_msg_handler.h"

#include "b2b.h"
#include "buffer.h"
#include "hdlc_frame.h"
#include "project.h"
#include "STACK.h"

#if (USE_DTLS==1)
#include "dtls.h"
#endif

#if (USE_IPTUNNEL == 1)
/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/
#if( RTOS_SELECTION == FREE_RTOS )
#define TUNNEL_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define TUNNEL_NUM_MSGQ_ITEMS 0 
#endif
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

static void TunnelMsgHandler(buffer_t *indication);

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

/** Stores the HDLC control value as it will not change. */
static hdlc_ctrl_t defCtrlValue;
static OS_MSGQ_Obj tunnelMsgQueue;

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Create queue for messages and register functions with lower layers

@return status code eSUCCESS or failure reason
*/
returnStatus_t TUNNEL_MSG_init(void)
{
   if (!OS_MSGQ_Create(&tunnelMsgQueue, TUNNEL_NUM_MSGQ_ITEMS))
   {
      return eFAILURE;
   }

   // create the control byte used for the HDLC frame
   hdlc_control_info_t defInfo;

   defInfo.type = HDLC_FRAME_U;
   defInfo.uCommand = HDLC_UCMD_UI;
   defInfo.pfBit = false;

   defCtrlValue = HdlcFrameCreateControlValue(defInfo);

   // Note: there is no diffrent handling for secured/unsecured data as the data
   // is just passed directly to the host board
#if (USE_DTLS == 1)
   DTLS_RegisterIndicationHandler((uint8_t)UDP_IP_DTLS_PORT, TunnelMsgHandler);
#endif
   NWK_RegisterIndicationHandler((uint8_t)UDP_IP_NONDTLS_PORT, TunnelMsgHandler);

   return eSUCCESS;
}

/**
Task for handling IP Tunneling Messages.

@param arg0 Not used, but required here because this is a task

@note The HDLC/B2B framework must be running in order to process messages properly.
*/
void TUNNEL_MSG_HandlerTask(uint32_t arg0)
{
   (void)arg0;

   while (true) /*lint !e716 */
   {
      buffer_t *pBuf;
      (void)OS_MSGQ_Pend(&tunnelMsgQueue, (void *)&pBuf, OS_WAIT_FOREVER);

      if (pBuf->eSysFmt == eSYSFMT_NWK_INDICATION)
      {
         NWK_DataInd_t *indication = (NWK_DataInd_t*)pBuf->data; /*lint !e826 */

         // Just transfer data directly to the Host board
         HdlcFrameData_t frame = { NW_HDLC_ADDR, defCtrlValue, indication->payload, indication->payloadLength };
         HdlcFrameTx(&frame);
      }

      BM_free(pBuf);
   }
}

/**
Called by lower layers to send indication to Tunneling layer

@param indication buffer with message data
*/
static void TunnelMsgHandler(buffer_t *indication)
{
   OS_MSGQ_Post(&tunnelMsgQueue, (void *)indication);
}
#endif
