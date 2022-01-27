/** ****************************************************************************
@file tunnel_util.c

General tunneling message retrieval/storage functions.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdint.h>
#include "project.h"
#include "stack_protocol.h"
#include "App_Msg_Handler.h"

#if (USE_DTLS == 1)
#include "dtls.h"
#endif

#include "tunnel_util.h"

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

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

/*******************************************************************************
Function Definitions
*******************************************************************************/

#if (USE_IPTUNNEL == 1)
/**
Called to send a tunneled IP message back to the head end.

@param data The data to send to the head end.
@param datLen Numer of bytes of data to send.
*/
returnStatus_t TUNNEL_MSG_Tx(const uint8_t *data, uint16_t dataLen)
{
   returnStatus_t retVal = eFAILURE;

   // verify passed data is valid
   if (data == NULL)
   {
      return retVal;
   }

   if (dataLen == 0)
   {
      return retVal;
   }

   NWK_Address_t  dst_address;
   NWK_Override_t override = { eNWK_LINK_OVERRIDE_NULL, eNWK_LINK_SETTINGS_OVERRIDE_NULL };
   NWK_GetConf_t  GetConf;

   // Retrieve Head End context ID
   GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );

   if (GetConf.eStatus != eNWK_GET_SUCCESS) {
      GetConf.val.ipHEContext = DEFAULT_HEAD_END_CONTEXT; // Use default in case of error
   }

   dst_address.addr_type = eCONTEXT;
   dst_address.context   = GetConf.val.ipHEContext;

   uint8_t securityMode;
   (void)APP_MSG_SecurityHandler(method_get, appSecurityAuthMode, &securityMode, NULL);

   // need QoS value of 63 for fastest response time
   uint8_t qos = 63;

#if (USE_DTLS == 1)
#if (DTLS_FIELD_TRIAL == 0)
   if (securityMode == 0)
#else
   // Allow unsecured communications until DTLS session established
   if ((securityMode == 0) || (!DTLS_IsSessionEstablished()))
#endif
#endif
   {
      if (NWK_DataRequest((uint8_t)UDP_IP_NONDTLS_PORT, qos, dataLen, data, &dst_address, &override, NULL, NULL) != 0)
      {
         retVal = eSUCCESS;
      }
   }
#if (USE_DTLS == 1)
   else
   {
      // Make sure DTLS session has been established
      if (DTLS_IsSessionEstablished())
      {
         if (DTLS_DataRequest((uint8_t)UDP_IP_DTLS_PORT, qos, dataLen, data, &dst_address, &override, NULL, NULL) != 0)
         {
            retVal = eSUCCESS;
         }
      }
      else
      {
         DTLS_CounterInc(eDTLS_IfOutNoSessionErrors);
      }
   }
#endif

   return retVal;
}
#endif
