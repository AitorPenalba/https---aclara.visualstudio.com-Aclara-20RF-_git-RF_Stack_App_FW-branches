
/***********************************************************************************************************************
 *
 * Filename:   ILC_TUNNELING_HANDLER.c
 *
 * Global Designator: ILC_TNL_
 *
 * Contents: Contains the routines to support the Serial Protocol Tunneling needs
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ KAD Created 080207
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 *
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "ilc_tunneling_handler.h"


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define ILC_TNL_TIME_OUT_mS             ( (uint32_t) 2000 )                          /* Time out when waiting for DRU */
#define DONT_WAIT_FOR_INBOUND_MESSAGE   false
#define WAIT_FOR_INBOUND_MESSAGE        true


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* GLOBAL VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static returnStatus_t GetOutboundMessageSize ( uint8_t const *pMessage_u8, uint8_t *pResult_u8 );


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/*******************************************************************************
  Function name: GetOutboundMessageSize

  Purpose: Calculates the size of an Outbound Message by counting how many bytes are there between the SOH and the EOT
           characters, including them.

  Arguments: uint8_t *pMessage_u8 - Pointer to Outbound Message to whom the size is to be calculated
             uint8_t *pResult_u8  - Pointer to where to store the calculated Outbound Message Size

  Returns: eSUCCESS/eFAILURE indication. If pMessage_u8 points to a valid MP-SPP message (i.e. it starts with an SOH,
           it ends with an EOT and is equal or smaller than the MP-SPP's maximum request size) the function will return
           eSUCCESS and it will store the calculated Outbound Message Size in the location pointed by pResult_u8. If
           eFAILURE is returned, result value is undefined and it shall not be used.
*******************************************************************************/
static returnStatus_t GetOutboundMessageSize ( uint8_t const *pMessage_u8, uint8_t *pResult_u8 )
{
   returnStatus_t retVal = eFAILURE;
   uint8_t tempResult_u8 = 1;

   *pResult_u8 = 0;

   if ( ( SOH_VALUE == *pMessage_u8 ) && ( NULL != pMessage_u8 ) )
   {
      do
      {
         tempResult_u8++;
         pMessage_u8++;
      }while ( ( EOT_VALUE != *pMessage_u8 ) && ( DRU_REQUEST_MAX_SIZE_IN_BYTES > tempResult_u8 ) );

      if ( EOT_VALUE == *pMessage_u8 )
      {
         *pResult_u8 = tempResult_u8;
         retVal = eSUCCESS;
      }
      else
      {
         ERR_printf( "The maximum amount of bytes for an Outbound Message has been received without an EOT" );
      }
   }
   else
   {
      ERR_printf( "Received Outbound Message did not started with SOH" );
   }

   return retVal;
}


/*******************************************************************************
   Function name: ILC_TNL_MsgHandler

   Purpose: Handles ILC Tunneling requests (One-way and Two-way)

   Arguments: HEEP_APPHDR_t *heepReqHdr - Pointer to application header structure from head end
              void *payloadBuf          - Pointer to data payload
              length                    - payoad length

   Returns: None
*******************************************************************************/
/*lint -esym(818,heepReqHdr) API doesn't allow for pointer to const  */
void ILC_TNL_MsgHandler( HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length )
{
   (void)length;
   enum_CIM_QualityCode TunnelResult = CIM_QUALCODE_KNOWN_MISSING_READ;
   uint8_t OutboundMessageSize_u8 = 0;
   uint8_t InboundMessageSize_u8 = 0;
   bool    InboundMessageAvailableFlag_b = false;

   HEEP_APPHDR_t heepRespHdr;

   /* Application header/QOS info */
   HEEP_initHeader( &heepRespHdr );
   heepRespHdr.TransactionType = (uint8_t)TT_RESPONSE;
   heepRespHdr.Resource = (uint8_t)tn_tw_mp;
   heepRespHdr.RspID = (uint8_t)heepReqHdr->ReqID;
   heepRespHdr.qos = (uint8_t)heepReqHdr->qos;
   heepRespHdr.callback = NULL;
   heepRespHdr.appSecurityAuthMode = heepReqHdr->appSecurityAuthMode;

   /* Calculate the Outbound message Size and store it in OutboundMessageSize_u8 */
   if ( ( eSUCCESS == GetOutboundMessageSize( (uint8_t *)payloadBuf, &OutboundMessageSize_u8 ) ) &&
        ( NULL != heepReqHdr ) &&
        ( NULL != payloadBuf ) )
   {
      /* If a One-way Outbound Message has been received, send the HEEP Payload as is to the DRU via MP-SPP */
      if ( ( TT_ASYNC     == (enum_TransactionType)heepReqHdr->TransactionType ) &&
            ( method_post == (enum_MessageMethod)heepReqHdr->Method ) )
      {
         /* Don't wait for a DRU Response */
         TunnelResult = ILC_DRU_DRIVER_tunnelOutboundMessage( (uint8_t *) payloadBuf, OutboundMessageSize_u8,
                                                               ILC_TNL_TIME_OUT_mS,
                                                               ( bool )DONT_WAIT_FOR_INBOUND_MESSAGE,
                                                               NULL, NULL, NULL );
         if ( CIM_QUALCODE_SUCCESS != TunnelResult )
         {
            ERR_printf( "One-way Outbound error in /tn/tw/mp handler" );
         }
         else
         {
            INFO_printf( "Successful One-way Outbound" );
         }
      }
      /* If a Two-way Outbound Message has been received, send the HEEP Payload as is to the DRU via MP-SPP  */
      else if ( ( TT_REQUEST == (enum_TransactionType)heepReqHdr->TransactionType ) &&
                  ( method_get == (enum_MessageMethod)heepReqHdr->Method ) )
      {
         buffer_t *pBuf;                                      /* Pointer to message */

         /* Allocate a buffer to accommodate worst case payload */
         pBuf = BM_alloc( DRU_RESPONSE_MAX_SIZE_IN_BYTES );
         if ( NULL != pBuf )
         {
            pBuf->x.dataLen = 0;

            TunnelResult = ILC_DRU_DRIVER_tunnelOutboundMessage( (uint8_t *) payloadBuf, OutboundMessageSize_u8,
                                                                  ILC_TNL_TIME_OUT_mS, ( bool )WAIT_FOR_INBOUND_MESSAGE,
                                                                  &InboundMessageAvailableFlag_b, pBuf->data,
                                                                  &InboundMessageSize_u8 );
            /* If a complete MP-SPP response was received before the time out, set the Status to OK */
            if ( CIM_QUALCODE_SUCCESS == TunnelResult )
            {
               heepRespHdr.Status = (uint8_t)OK;
               INFO_printf( "Successful Two-way Outbound" );

               /* Set the size of the Inbound Message to be transmitted */
               if ( true == InboundMessageAvailableFlag_b )
               {
                  pBuf->x.dataLen = InboundMessageSize_u8;
               }

               /* Transmit the Inbound Message over HEEP and free memory used by buffer */
               if ( eSUCCESS != HEEP_MSG_Tx( &heepRespHdr, pBuf ) )
               {
                  ERR_printf( "HEEP_MSG_Tx returned an error in /tn/tw/mp handler" );
               }
               else
               {
                  INFO_printf( "Successful Two-way Inbound" );
               }
            }
            /* If a complete MP-SPP response was NOT received before the time out, set the Status to Timeout */
            else
            {
               ERR_printf( "MP-SPP response was NOT received before the time out" );
               heepRespHdr.Status = (uint8_t)Timeout;

               /* Transmit only the HEEP Header over HEEP */
               if (eSUCCESS != HEEP_MSG_TxHeepHdrOnly(&heepRespHdr ) )
               {
                  ERR_printf( "HEEP_MSG_TxHeepHdrOnly returned an error in /tn/tw/mp handler" );
               }

               /* free memory used by buffer if a complete MP-SPP response was NOT received before the time out */
               BM_free( pBuf );
            }
         }
         else
         {
            ERR_printf( "Ran out of buffers in /tn/tw/mp Handler" );
            heepRespHdr.Status = (uint8_t)ServiceUnavailable;

            /* Transmit only the HEEP Header over HEEP */
            if ( eSUCCESS != HEEP_MSG_TxHeepHdrOnly( &heepRespHdr ) )
            {
               ERR_printf( "HEEP_MSG_TxHeepHdrOnly returned an error in /tn/tw/mp handler" );
            }
         }
      }
      else
      {
         ERR_printf( "/tn/tw/mp received an unsupported request" );
         heepRespHdr.Status = (uint8_t)BadRequest;

         /* Transmit only the HEEP Header over HEEP */
         if ( eSUCCESS != HEEP_MSG_TxHeepHdrOnly( &heepRespHdr ) )
         {
            ERR_printf( "HEEP_MSG_TxHeepHdrOnly returned an error in /tn/tw/mp handler" );
         }
      }
   }
   else
   {
      ERR_printf( "/tn/tw/mp received an invalid Outbound Message" );
      heepRespHdr.Status = (uint8_t)BadRequest;

      /* Transmit only the HEEP Header over HEEP */
      if ( eSUCCESS != HEEP_MSG_TxHeepHdrOnly( &heepRespHdr ) )
      {
         ERR_printf( "HEEP_MSG_TxHeepHdrOnly returned an error in /tn/tw/mp handler" );
      }
   }
}
/*lint +esym(818,heepReqHdr) */
/* End of file */
