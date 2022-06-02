/***********************************************************************************************************************

   Filename: APP_MSG_Handler.c

   Global Designator: APP_MSG_

   Contents:  This file contains the functions for processing two-way commands.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2014-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************

   $Log$ msingla Created Nov 26, 2014

 **********************************************************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"
#include "rand.h"

#define APP_MSG_GLOBALS
#include "App_Msg_Handler.h"
#include "App_Msg_HandlerTable.h"
#undef APP_MSG_GLOBALS

#include "rand.h"
#include "buffer.h"
#include "STACK.h"
#include "STACK_Protocol.h"
#include "time_sys.h"
#include "time_util.h"
#include "timer_util.h"
#include "DBG_SerialDebug.h"
#include "RG_MD_Handler.h"
#include "ID_intervalTask.h"
#if ( PHASE_DETECTION == 1 )
#include "PhaseDetect.h"
#endif

#if (USE_DTLS==1)
#include "dtls.h"
#endif
#if (USE_MTLS==1)
#include "mtls.h"
#endif
#include "mode_config.h"

/* MACRO DEFINITIONS */
#define MIN_REGISTRATION_TIMEOUT_WAIT             (10 * 60)
#define MAX_REGISTRATION_TIMEOUT_WAIT             (24 * 60 * 60)
#define DEFAULT_INITIAL_REGISTRATION_TIMEOUT      1800
#define INITIAL_REGISTRATION_TIMEOUT_MAX_VALUE    43200
#define INITIAL_REGISTRATION_TIMEOUT_MIN_VALUE    1800
#define MIN_REGISTRATION_TIMEOUT_MAX_VALUE        600
#define MIN_REGISTRATION_TIMEOUT_MIN_VALUE        30
#define MAX_REGISTRATION_TIMEOUT_MAX_VALUE        172800
#define MAX_REGISTRATION_TIMEOUT_MIN_VALUE        43200
#if( RTOS_SELECTION == FREE_RTOS )
#define APP_MSG_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define APP_MSG_NUM_MSGQ_ITEMS 0
#endif


/* TYPE DEFINITIONS */

/* CONSTANTS */

#if 0 //TODO: Include when all App Stats are incorporated
/* These are used for printing messages */
static const char * const app_attr_names[] = /* Don't forget to update the size using the length of the longest
                                                element + 1.  */
{
   [eAppAttr_appIfInErrors                      ] = "appIfInErrors",
   [eAppAttr_appIfInSecurityErrors              ] = "appIfInSecurityErrors",
   [eAppAttr_appIfLastResetTime                 ] = "appIfLastResetTime",
   [eAppAttr_appIfInOctets                      ] = "appIfInOctets",
   [eAppAttr_appIfInPackets                     ] = "appIfInPackets",
   [eAppAttr_appIfOutErrors                     ] = "appIfOutErrors",
   [eAppAttr_appIfOutOctets                     ] = "appIfOutOctets",
   [eAppAttr_appIfOutPackets                    ] = "appIfOutPackets",
   [eAppAttr_appLowerLayerCount                 ] = "appLowerLayerCount",
};
#endif

#if (USE_DTLS==1)
static const enum_MessageResource resourceWhiteList[] =
{
   bu_en,
   df_dp,
#if (PHASE_DETECTION == 1)
   pd
#endif
};
#endif

//Only resources allowed when receiving a multicast address
static const enum_MessageResource multicastWhiteList[] =
{
   df_dp,
   tn_tw_mp,
#if (PHASE_DETECTION == 1)
   pd
#endif
};

static const uint8_t       regMetadataQOS = 0x39;
/*lint -e{446} offsetof is a side effect? */
/*lint -e(35)  offsetof is a side effect? */
static const HEEP_FileInfo_t Security[] =
{
#if (USE_DTLS==1)
   { appSecurityAuthMode,  ( uint16_t )offsetof( appSecurity_t, f_appSecurityAuthMode ), sizeof( uint8_t ), DTLS_AppSecurityAuthModeChanged },  /*lint !e34 non-compile time constant  */
   { ( meterReadingType )0,  0u,                                                         ( uint8_t )0,      NULL } // NULL terminator
#else
   { appSecurityAuthMode,  ( uint16_t )offsetof( appSecurity_t, f_appSecurityAuthMode ), sizeof( uint8_t ), NULL },
   { ( meterReadingType )0,  0u,                                                         ( uint8_t )0,      NULL } // NULL terminator
#endif
};

/* LOCAL FUNCTION PROTOTYPES */
static void APP_MSG_HandleMessage( buffer_t *indication );
static void APP_MSG_resendRegistrationMetadata ( uint8_t cmd, void *pData );
static void APP_MSG_CheckRegistrationStatus( void );
#if (USE_DTLS==1)
static void APP_MSG_DTLSSessionStateChanged( DtlsSessionState_e sessionState );
static void APP_MSG_UnsecureHandleMessage( buffer_t *indication );
#endif

/* GLOBAL VARIABLE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */
static OS_MSGQ_Obj   AppMsgHandler_MsgQ_;
static FileHandle_t  fileHndlReg;            /* Contains the registration file handle information. */
static FileHandle_t  fileHndlSecurity;       /* Contains the security file handle information. */
static FileHandle_t  fileHndlAppCached;      /* Contains the security file handle information. */
static bool          appMsgHandlerReady = false; // Used for apps that may need to know if handler is ready for msgs
static appSecurity_t appSecurity_;
static appCached_t   appCached_;
static buffer_t      resendRegMsgbuf_ = { 0 }; //buffer used by timer callback. This is used to resend registration message
/* FUNCTION DEFINITIONS */

#if (USE_DTLS == 1)
/***********************************************************************************************************************

   Function Name: APP_MSG_UnsecureHandleMessage

   Purpose: Called by lower layers to send indication to Application layer. Handles the "unsecure" UDP port. Check HEEP
            header for resource and verify it is in the white list. If so, pass to the secured message queue. If not,
            drop the message.

   Arguments: buffer_t

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
static void APP_MSG_UnsecureHandleMessage( buffer_t *indication )
{
   HEEP_APPHDR_t        heepHdr;       //Application header
   NWK_DataInd_t        *pBuf = ( NWK_DataInd_t * )( void * )indication->data;
   uint16_t             bitNo = 0;     //Used by unpack
   bool                 pass = ( bool )false;

   if ( eSYSFMT_NWK_INDICATION == indication->eSysFmt )
   {
      uint8_t port = NWK_UdpPortNumToId(pBuf->dstUdpPort);
#if ( DTLS_FIELD_TRIAL == 0 )
      if ( appSecurity_.f_appSecurityAuthMode == 0 )       /* Not running in secure mode, all indications allowed */
#else
      if (  ( appSecurity_.f_appSecurityAuthMode == 0 ) || /* Not running in secure mode, all indications allowed */
            ( !DTLS_IsSessionEstablished() ) )           /* OR session not established */
#endif
      {
         pass = ( bool )true;
      }
      else if ( HEEP_UTIL_unpackHeader ( pBuf, &bitNo, &heepHdr ) )
      {  // For the unsecured port only allow these resources
         for ( uint8_t i = 0; i < ARRAY_IDX_CNT( resourceWhiteList ); i++ )
         {
            if ( resourceWhiteList[ i ] == ( enum_MessageResource )heepHdr.Resource )
            {
               pass = ( bool )true; // resource is on the white list
               break;
            }
         }
      }
      else
      {
         // If at any time header validation fails, then the packet is discarded and appIfInErrors is incremented
         appCached_.appIfInErrors[port]++;
      }
   }
   if ( pass )
   {
      OS_MSGQ_Post( &AppMsgHandler_MsgQ_, ( void * )indication );
   }
   else
   {
      BM_free( indication );
      // Any incoming packets that do not provide all required security properties are dropped, appIfInSecurityErrors is incremented
      // Port zero always the appropriate one here
      appCached_.appIfInSecurityErrors[0]++;
   }
}
#endif

/***********************************************************************************************************************

   Function Name: APP_MSG_HandleMessage

   Purpose: Called by lower layers to send indication to Application layer

   Arguments: buffer_t

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
static void APP_MSG_HandleMessage( buffer_t *indication )
{
   OS_MSGQ_Post( &AppMsgHandler_MsgQ_, ( void * )indication );
}

/***********************************************************************************************************************

   Function Name: APP_MSG_HandlerReady

   Purpose: Called by applications that need to know if task is ready to accept message (e.g. DFW after power cycle)

   Arguments: None

   Returns: true if App task is ready to accept messages, otherwise false

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
bool APP_MSG_HandlerReady( void )
{
   return appMsgHandlerReady;
}

/***********************************************************************************************************************

   Function Name: APP_MSG_init

   Purpose: Create queue for Application messages and register functions with lower layers

   Arguments: None

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
returnStatus_t APP_MSG_init ( void )
{
   returnStatus_t retVal = eFAILURE  ; //Return status
   FileStatus_t fileStatus;

   if ( OS_MSGQ_Create( &AppMsgHandler_MsgQ_, APP_MSG_NUM_MSGQ_ITEMS, "APP_MSG" ) )
   {
      appMsgHandlerReady = true; //Let apps know handler is ready to accept messages

      if ( eSUCCESS == FIO_fopen( &fileHndlReg, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_REG_STATUS,
                                  ( lCnt )sizeof( RegistrationInfo ), FILE_IS_NOT_CHECKSUMED,
                                  DVR_BANKED_MAX_UPDATE_RATE_SEC, &fileStatus ) )
      {
         if ( fileStatus.bFileCreated )
         {
            APP_MSG_initRegistrationInfo();  /* Set default values on virgin device.   */
            APP_MSG_UpdateRegistrationFile();
            retVal = eSUCCESS;
         }
         else  /* Read the MetaData file  */
         {
            retVal = FIO_fread( &fileHndlReg, ( uint8_t * )&RegistrationInfo, 0, ( lCnt )sizeof( RegistrationInfo ) );
         }
      }
      if (eSUCCESS == retVal)
      {
         if ( eSUCCESS == FIO_fopen( &fileHndlSecurity, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_SECURITY,
                                     ( lCnt )sizeof( appSecurity_t ), FILE_IS_NOT_CHECKSUMED,
                                     0xFFFFFFFF, &fileStatus ) )
         {
            if ( fileStatus.bFileCreated )
            {  /* The file was just created for the first time. Save the default values to the file */
               appSecurity_.f_appSecurityAuthMode = 0;   /* On "virgin" unit, default is ALWAYS unsecure mode  */
               retVal = FIO_fwrite( &fileHndlSecurity, 0, ( uint8_t * )&appSecurity_, ( lCnt )sizeof( appSecurity_ ) );
            }
            else
            {  // Read the Cached MetaData
               retVal = FIO_fread( &fileHndlSecurity, ( uint8_t * )&appSecurity_, 0, ( lCnt )sizeof( appSecurity_ ) );
            }
         }
      }
      if (eSUCCESS == retVal)
      {
         if ( eSUCCESS == FIO_fopen( &fileHndlAppCached, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_APP_CACHED,
                                     ( lCnt )sizeof( appCached_t ), FILE_IS_NOT_CHECKSUMED,
                                     0, &fileStatus ) )
         {
            if ( fileStatus.bFileCreated )
            {  /* The file was just created for the first time. Save the default values to the file */
               (void) memset ( &appCached_, 0, sizeof (appCached_t) );
               retVal = FIO_fwrite( &fileHndlAppCached, 0, ( uint8_t * )&appCached_, ( lCnt )sizeof( appCached_t ) );
            }
            else
            {  // Read the Cached MetaData
               retVal = FIO_fread( &fileHndlAppCached, ( uint8_t * )&appCached_, 0, ( lCnt )sizeof( appCached_t ) );
            }
         }
      }
   }

#if (USE_DTLS == 1)
   /* Register normal app handler for DTLS and Unsecured handler for NON-DTLS */
   DTLS_RegisterIndicationHandler ( ( uint8_t )UDP_DTLS_PORT, APP_MSG_HandleMessage );
   NWK_RegisterIndicationHandler( ( uint8_t )UDP_NONDTLS_PORT, APP_MSG_UnsecureHandleMessage );
   /* If using DTLS, register a callback when the DTLS session state changes.  This callback will wait until a session
      has been established and once it has, will handle sending the registration metadata if necessary.  If DTLS is not
      being used, then the registration metadata will be checked immediately to see if it should be sent.  */
   DTLS_RegisterSessionStateChangedCallback( APP_MSG_DTLSSessionStateChanged );
#if ( USE_MTLS == 1 )   /* MTLS requires DTLS!  */
   MTLS_RegisterIndicationHandler ( ( uint8_t )UDP_MTLS_PORT, APP_MSG_HandleMessage );
#endif
#else
   /* Only one handler in NON-DTLS system */
   NWK_RegisterIndicationHandler( (uint8_t)UDP_NONDTLS_PORT, APP_MSG_HandleMessage );
#endif

   resendRegMsgbuf_.x.flag.isFree = 1; //Mark as the buffer is free
   return ( retVal );
}

/***********************************************************************************************************************

   Function Name: APP_MSG_HandlerTask(void)

   Purpose: Task for handling Appilcation Messages

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
//lint -esym(715,Arg0)  // Arg0 required for generic API, but not used here.
void APP_MSG_HandlerTask ( taskParameter )
{
   buffer_t *pBuf;            //pointer to message
   bool     handlerMatchFound = ( bool )false;

   INFO_printf( "APP_MSG_Handler_Task starting..." );

   OS_TASK_Sleep ( 5000 );

   /* On reset, check for need to send registration information. */
   if( RegistrationInfo.registrationStatus != REG_STATUS_CONFIRMED )
   {
      /* Set timeout to be about 2 mins ( 119.7 to be precise ). It is 3 times HMC_STRT_RestartDelay(), used in HMC_STRT_LogOn() */
      ( void )APP_MSG_setMetadataRegistrationTimer ( 3 * ( HMC_MSG_TRAFFIC_TIME_OUT_mS +
                                                          (HMC_MSG_RESPONSE_TIME_OUT_mS * HMC_MSG_RETRIES) + RESTART_DELAY_FUDGE_MS ) );
   }
   else  /* Confirmed; check back after MAX time out (1 day)   */
   {
      ( void )APP_MSG_setMetadataRegistrationTimer ( MAX_REGISTRATION_TIMEOUT_WAIT * 1000 );
   }

   /*
      Task loop begins
   */
   for ( ;; )
   {
      (void)OS_MSGQ_Pend( &AppMsgHandler_MsgQ_, ( void * )&pBuf, OS_WAIT_FOREVER ); /* Message in the queue? */
      DBG_logPrintf( 'M', "Got  pBuf = 0x%08x", ( uint32_t )pBuf );

      /* Got the message for the application, process it */
      if ( eSYSFMT_NWK_INDICATION == pBuf->eSysFmt )
      {
         //Process data indication
         HEEP_APPHDR_t                 heepHdr;             //Application header
         uint16_t                      bitNo      = 0;      //Used by unpack
         APP_MSG_HandlerInfo_t const  *appMsgHandlerInfo;   //point to the app msg handler table
         NWK_DataInd_t                *indication = ( NWK_DataInd_t* )( void * )pBuf->data;
         TRlayers                      port       = (TRlayers)NWK_UdpPortNumToId(indication->dstUdpPort);

         // When an HE-EP packet is received from any of the lower layers the appIfInOctets attribute is incremented by the packet
         //  length and appIfInPackets is incremented.
         // Increment appIfInOctets by indication->payloadLength
         appCached_.appIfInOctets[port] += indication->payloadLength;
         // Increment appIfInPackets here
         appCached_.appIfInPackets[port]++;

         HEEP_initHeader( &heepHdr );
         //Security, Resource, Port and addressing form validations:
         // Not in secure mode
         //    Unicast message
         //       All messages allowed
         //    A Multicast message
         //       whitelisted message allowed
         //       specific resources must be through MTLS port
         //       All others rejected
         // In secure mode
         //    Port 0 handler validates only the allowed resources pass through
         //    /tn/tw/mp must be through MTLS port (Port 2)
         //
         //Validate the interface revision etc.
         if ( HEEP_UTIL_unpackHeader ( indication, &bitNo, &heepHdr ) )
         {
            bool pass = ( bool )false; // Indicate if message is accepted (true) or rejected (false)
#if ( DTLS_FIELD_TRIAL == 0 )
            if ( appSecurity_.f_appSecurityAuthMode == 0 )       /* Not running in secure mode, all indications allowed */
#else
            if (  ( appSecurity_.f_appSecurityAuthMode == 0 ) || /* Not running in secure mode, all indications allowed */
                  ( !DTLS_IsSessionEstablished() ) )           /* OR session not established */
#endif
            {
               // Unicast message (an EP does not support eCONTEXT)
               if ( ( eEXTENSION_ID == indication->dstAddr.addr_type ) || ( eCONTEXT == indication->dstAddr.addr_type ) )
               {
                  pass = ( bool )true; // All messages allowed in this address type
               }
               // A Multicast message and a one-way transaction
               else if ( ( eMULTICAST == indication->dstAddr.addr_type ) && ( TT_ASYNC == (enum_TransactionType)heepHdr.TransactionType ) )
               {
                  pass = ( bool )true; //Common Multicast qualification performed below
               }
            }
            else //Security enabled
            {
               // Each port (Unsecured/DTLS/MTLS) validates the message (will not make here if the checks by the port do not pass)
               // Special case for /tn/tw/which mp must be through MTLS port if MULTICAST and DTLS port if Unicast
               if ( tn_tw_mp == ( enum_MessageResource )heepHdr.Resource )
               {
#if ( USE_MTLS == 1 )
                  if ( ( ( UDP_MTLS_PORT == port ) && ( eMULTICAST    == indication->dstAddr.addr_type ) ) )
                  {
                     pass = ( bool )true;
                  }
#endif
#if ( USE_DTLS == 1 )
                  if ( ( !pass ) &&
                       ( ( UDP_DTLS_PORT == port ) && ( eEXTENSION_ID == indication->dstAddr.addr_type ) ) )
                  {
                     pass = ( bool )true;
                  }
#endif
               }
               else
               {
                  pass = ( bool )true; // All other resources OK since passed port handler
               }
               // Any incoming packets that do not provide all required security properties are dropped, appIfInSecurityErrors is incremented
               //   Unsecured Port handler increments when security on and NOT one of the allowed resources
               //   If /tn/tw/mp, Multicast and NOT through MTLS port increments below
               //   Fails in the Port handler not applicable here
            }
            // Ensure when Multicast only certain resource are allowed
            if ( pass && ( eMULTICAST == indication->dstAddr.addr_type ) )
            {
               pass = ( bool )false;
               if ( TT_ASYNC == (enum_TransactionType)heepHdr.TransactionType )
               {  // Only allow these resources
                  for ( uint8_t i = 0; i < ARRAY_IDX_CNT( multicastWhiteList ); i++ )
                  {
                     if ( multicastWhiteList[ i ] == ( enum_MessageResource )heepHdr.Resource )
                     {  //TODO: Can the Resource check be removed since all of the allowed ones must come through MTLS port?
                        if ( tn_tw_mp == ( enum_MessageResource )heepHdr.Resource )
                        {
#if ( USE_MTLS == 1 )
                           if ( UDP_MTLS_PORT == port )
#endif
                           {
                              pass = ( bool )true; // resource is MUST  come through the MTLS port
                              break;
                           }
                        }
                        else
                        {
                           pass = ( bool )true; // All other resources OK since in the white list
                           break;
                        }
                     }
                  }
               }
               // All others rejected when multicast
               // Any incoming multicast packets that do not match an entry are dropped and appIfInErrors is incremented.
               appCached_.appIfInErrors[port]++;
#if ( DTLS_FIELD_TRIAL == 0 )
               if ( appSecurity_.f_appSecurityAuthMode != 0 )       /* Running in secure mode */
#else
               if (  ( appSecurity_.f_appSecurityAuthMode != 0 ) && /* Running in secure mode */
                     ( DTLS_IsSessionEstablished() ) )              /* AND session established */
#endif
               {
                  // Any incoming packets that do not provide all required security properties are dropped and appIfInSecurityErrors is incremented
                  appCached_.appIfInSecurityErrors[port]++;
               }
            }
            if ( pass )
            {
               /* Valid transaction type. Search the handle and make the function call.   */
               handlerMatchFound = ( bool )false;
               for ( appMsgHandlerInfo = &APP_MSG_HandlerTable[0]; appMsgHandlerInfo->pfAppMsgHandlerFunc != NULL;
                     appMsgHandlerInfo++ )
               {
                  if ( appMsgHandlerInfo->resourceID == heepHdr.Resource )
                  {
                     //resource handle found, execute resource handler. Pass app header info and application payload

#if ( PHASE_DETECTION == 1 )
                    // If this was a phase detection resource, then save the time stamp
                    // This is required becuase the application level does get the time!
                     if(heepHdr.Resource == ( uint8_t )pd)
                     {
                        PD_SyncTime_Set(indication->timeStamp);
                     }
#endif


                     appMsgHandlerInfo->pfAppMsgHandlerFunc( &heepHdr, &indication->payload[HEEP_APP_HEADER_SIZE], indication->payloadLength - HEEP_APP_HEADER_SIZE );
                     handlerMatchFound = ( bool )true;
                     break;
                  }
               }
               if ( !handlerMatchFound )
               {
                  DBG_logPrintf ( 'E', "No application handler registered for this message" );
               }
            }
            else
            {
               DBG_logPrintf ( 'E', "Invalid port or broadcast for application" );
            }
         }
         else
         {
            DBG_logPrintf ( 'E', "Invalid application header" );
            //If at any time header validation fails, then the packet is discarded and appIfInErrors is incremented
            appCached_.appIfInErrors[port]++;
         }
      }
      else if ( eSYSFMT_STACK_DATA_CONF == pBuf->eSysFmt )
      {
         //Process data confirm

      }
      else if ( eSYSFMT_TIME == pBuf->eSysFmt )
      {
         /* Need to re-send the metadata registration message? */
         APP_MSG_CheckRegistrationStatus();
      }
      //else Unrecognized message type, drop the message
      DBG_logPrintf( 'M', "Free pBuf = 0x%08x", ( uint32_t )pBuf );
      BM_free( pBuf );
   }
}
//lint +esym(715,Arg0)  // Arg0 required for generic API, but not used here.

/***********************************************************************************************************************

   Function Name: APP_MSG_CheckRegistrationStatus(void)

   Purpose: Check the current registration status.  If the registration has never been sent, or it's been
            sent but no response received within the allotted time, (re)send the registration metadata
            message.

   Arguments: None

   Returns: None

   Side Effects: Sets a timer to re-send the metadata if a response is not received within the
                 allotted time.  Updates some of the registration timeout parameters.

   Reentrant Code: NO

 **********************************************************************************************************************/
static void APP_MSG_CheckRegistrationStatus( void )
{
   static bool       confirmed_ = ( bool )false;
   sysTimeCombined_t currentTime;
   uint32_t          currTimeSecs;
   uint32_t          elapsedSecs;
   uint32_t          itries = 0;
   returnStatus_t    retStat;

   /* Process the registration state machine */

   if ( MODECFG_get_rfTest_mode() == 0 )
   {
      switch ( RegistrationInfo.registrationStatus )  /*lint !e788 not all enums used within switch */
      {
         case REG_STATUS_NOT_SENT:   // Registration not yet sent.  Do so now
         {
            do
            {
               switch ( retStat = APP_MSG_SendRegistrationMetadata() )  /*lint !e788 not all enums used within switch */
               {
                  case eSUCCESS:
                     APP_MSG_updateRegistrationTimeout();
                     ( void )APP_MSG_setMetadataRegistrationTimer( RegistrationInfo.activeRegistrationTimeout * 1000 );
                     break;
                  case eBUFFER_ALLOC_FAILED:
                     /* Unable to acquire a buffer to send the message - try again in 10 seconds.  */
                     retStat = APP_MSG_setMetadataRegistrationTimer ( 10 * 1000 );
                     break;
                  case eHMC_LP_INVALID_REQ_TIME_RANGE:
                     /* Installation time not yet set and we haven't received a time sync. Wait 1 minute and try again. */
                     retStat = APP_MSG_setMetadataRegistrationTimer ( 1 * 60 * 1000 );
                     break;
                  case eFAILURE:
                  default:
                     /* If unable to create a new timer to check if the registration confirmation came back, wait a bit and
                        try again.  We have to do it this way because if we can't create a timer to check the confirmation,
                        we can't create a timer to retry again.  */
                     OS_TASK_Sleep ( 100 );
                     itries++;
               }  /*lint !e788 not all enums used within switch */
            }
            while ( ( retStat == eFAILURE ) &&
                    ( itries < 10 ) ); // TODO - should there be a limit on this, or should we retry indefinitely?
            if ( retStat != eSUCCESS )
            {
               ( void )APP_MSG_setMetadataRegistrationTimer ( 10 * 1000 ); /* Set timer if all attempts failed.   */
            }
            break;
         }  /*lint !e788 not all enums used within switch */
         case REG_STATUS_NOT_CONFIRMED:   // Registration sent, but confirmation not received
         {
            /* Check for REG_STATUS_CONFIRMED met sometime before. If so, reset state to REG_STATUS_NOT_SENT   */
            if ( confirmed_ )
            {
               RegistrationInfo.registrationStatus = REG_STATUS_NOT_SENT;
               confirmed_ = ( bool )false;
               APP_MSG_updateRegistrationTimeout();
               ( void )APP_MSG_setMetadataRegistrationTimer ( RegistrationInfo.nextRegistrationTimeout * 1000 );
            }
            else
            {
               /* Check how long since the registration was last sent.  If more than the given interval, re-send the
                  registration.  Otherwise set a timer for the time remaining */
               if ( TIME_SYS_IsTimeValid() )
               {
                  // have a valid time - see how long since we last send the metadata
                  ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );
                  currTimeSecs = ( uint32_t )( currentTime / TIME_TICKS_PER_SEC );
                  elapsedSecs = currTimeSecs - RegistrationInfo.timeRegistrationSent;

                  if ( ( currTimeSecs > RegistrationInfo.timeRegistrationSent ) &&
                        ( elapsedSecs < RegistrationInfo.activeRegistrationTimeout ) )
                  {
                     /* We've already sent the metadata registration message, haven't received a confirmation, but we're
                        still within the window to wait for the confirmation.  Set the timer for the remaining time we
                        should wait for a response.  If no response is received within this remaining time, we'll send
                        another metadata registration.  */
                     ( void )APP_MSG_setMetadataRegistrationTimer ( ( RegistrationInfo.activeRegistrationTimeout - elapsedSecs ) * 1000 );
                  }
                  else
                  {
                     /* Either the current time is before when we think we last sent the metadata message, or the specified
                        timeframe to wait for a confirmation has elapsed. In either case, re-send the metadata message.  */
                     APP_MSG_updateRegistrationTimeout();
                     switch ( APP_MSG_SendRegistrationMetadata() )   /*lint !e788 not all enums used within switch */
                     {
                        case eSUCCESS:
                           ( void )APP_MSG_setMetadataRegistrationTimer( RegistrationInfo.activeRegistrationTimeout * 1000 );
                           break;
                        case eBUFFER_ALLOC_FAILED:
                           /* Unable to acquire a buffer to send the message - try again in 10 seconds.  */
                           ( void )APP_MSG_setMetadataRegistrationTimer ( 10 * 1000 );
                           break;
                        case eHMC_LP_INVALID_REQ_TIME_RANGE:
                        case eFAILURE:
                        default:
                           /* Installation time not yet set and we haven't received a time sync or DTLS session not
                              established.  Wait 1 minute and try again.  */
                           ( void )APP_MSG_setMetadataRegistrationTimer ( 1 * 60 * 1000 );
                           break;
                     }  /*lint !e788 not all enums used within switch */
                  }
               }
               else  /* Time not valid; wait one minute and retry.   */
               {
                  ( void )APP_MSG_setMetadataRegistrationTimer ( 1 * 60 * 1000 );
               }
            }
            break;
         }
         case REG_STATUS_CONFIRMED:   // Registration sent and confirmation received - nothing to do
         {
            confirmed_ = ( bool )true;
            /* Set timer to try again, just in case status resets to not confirmed. */
            ( void )APP_MSG_setMetadataRegistrationTimer( MAX_REGISTRATION_TIMEOUT_WAIT * 1000 );
            break;
         }
         default:
         {
            /* Unknown state - reset to REG_STATUS_NOT_SENT */
            RegistrationInfo.registrationStatus = REG_STATUS_NOT_SENT;
            confirmed_ = ( bool )false;
            APP_MSG_updateRegistrationTimeout();
            ( void )APP_MSG_setMetadataRegistrationTimer( RegistrationInfo.activeRegistrationTimeout * 1000 );
            ( void )APP_MSG_SendRegistrationMetadata(); /* Send registration message now.   */
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
}

#if (USE_DTLS==1)
/***********************************************************************************************************************

   Function Name: APP_MSG_DTLSSessionStateChanged(void)

   Purpose: Callback invokved when the DTLS session state changes.  If the new session state is
            Connected, then the registration status is checked to see if the registration metadata
            needs to be sent.

   Arguments: ANone

   Returns: None

   Side Effects: See APP_MSG_CheckRegistrationStatus

   Reentrant Code: NO

 **********************************************************************************************************************/
static void APP_MSG_DTLSSessionStateChanged( DtlsSessionState_e sessionState )
{
   if ( sessionState == DTLS_SESSION_CONNECTED_e )
   {
      APP_MSG_CheckRegistrationStatus();
   }
}

#endif

/***********************************************************************************************************************

   Function Name: APP_MSG_updateRegistrationTimeout(void)

   Purpose: Updates the registration timeout values - the active registration timeout (i.e., the timeout that will
            be used for the next registration command) will be set to a random number between 30 and the current
            registration timeout (which will be between the min and max registration timeout).  Then the
            current registration timeout will be doubled )up to the max registration timeout).

   Arguments:

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
void APP_MSG_updateRegistrationTimeout()
{
   /* Need to retry after some random period between RegistrationInfo.minRegistrationTimeout and
      RegistrationInfo.nextRegistrationTimeout seconds. Then double the RegistrationInfo.nextRegistrationTimeout for
      next time if needed, up to a maximum of RegistrationInfo.maxRegistrationTimeout seconds */
   RegistrationInfo.activeRegistrationTimeout = aclara_randu( RegistrationInfo.minRegistrationTimeout,
                                                                  RegistrationInfo.nextRegistrationTimeout );
   DBG_logPrintf ( 'D', "Registration Timeout Range: %d - %d; Active Timeout: %d",
                   RegistrationInfo.minRegistrationTimeout,
                   RegistrationInfo.nextRegistrationTimeout,
                   RegistrationInfo.activeRegistrationTimeout );

   RegistrationInfo.nextRegistrationTimeout *= 2;
   if ( RegistrationInfo.nextRegistrationTimeout > RegistrationInfo.maxRegistrationTimeout )
   {
      RegistrationInfo.nextRegistrationTimeout = RegistrationInfo.maxRegistrationTimeout;
   }
}

/***********************************************************************************************************************

   Function Name: APP_MSG_EnableReRegistration(void)

   Purpose: Updates the registration file with the current registration information

   Arguments:

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
void APP_MSG_ReEnableRegistration( void )
{
   /* Regardless of where the registration is at, reset it to start over.
      TODO determine if we need to clear out any timeout timers.   */
   RegistrationInfo.nextRegistrationTimeout      = RegistrationInfo.initialRegistrationTimeout;
   RegistrationInfo.timeRegistrationSent         = 0;
   RegistrationInfo.registrationStatus           = REG_STATUS_NOT_SENT;
   APP_MSG_UpdateRegistrationFile();
   ( void )APP_MSG_setMetadataRegistrationTimer ( RegistrationInfo.nextRegistrationTimeout * 1000 );
}

/***********************************************************************************************************************

   Function Name: APP_MSG_UpdateRegistrationFile(void)

   Purpose: Updates the registration file with the current registration information

   Arguments:

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
void APP_MSG_UpdateRegistrationFile( void )
{
   ( void )FIO_fwrite( &fileHndlReg, 0, ( uint8_t * )&RegistrationInfo, ( lCnt )sizeof( RegistrationInfo ) );
}

/***********************************************************************************************************************

   Function Name: APP_MSG_SendRegistrationMetadata(void)

   Purpose: Generates and sends the /rg/md bubble-up message to the head-end

   Arguments:

   Returns: returnStatus_t - success indicator.
                             eHMC_LP_INVALID_REQ_TIME_RANGE if installation date/time not yet available
                             eBUFFER_ALLOC_FAILED if unable to acquire a buffer
                             eFAILURE if unable to acquire a new timer.

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
returnStatus_t APP_MSG_SendRegistrationMetadata( void )
{
   returnStatus_t retStat = eFAILURE;
   buffer_t      *pBuf;            // pointer to message

   /* Check for a special case where the meter is first installed and has not yet received a timestamp.  If this
      happens, the installation date/time is returned as 0.  We don't want to send the metadata yet until we get that
      timesync, so return an error for now.  The caller should re-try */
   if ( TIME_SYS_GetInstallationDateTime() == 0 )
   {
      if( !TIME_SYS_IsTimeValid() )
      {
         return eHMC_LP_INVALID_REQ_TIME_RANGE;
      }
      else  /* System reset or powered up with valid (RTC) time. Get the current system time and set the installation
               date/time. */
      {
         sysTime_t curTime;
         uint32_t  seconds;
         uint32_t fractionalSec;
         ( void )TIME_SYS_GetSysDateTime( &curTime );
         TIME_UTIL_ConvertSysFormatToSeconds( &curTime, &seconds, &fractionalSec );
         TIME_SYS_SetInstallationDateTime( seconds );
      }
   }

   pBuf = RG_MD_BuildMetadataPacket();
   if ( pBuf != NULL )                          // Send the message to message handler.
   {
      sysTimeCombined_t currentTime;   // Get current time
      HEEP_APPHDR_t  heepHdr;

      //Application header/QOS info
      HEEP_initHeader( &heepHdr );
      heepHdr.TransactionType = ( uint8_t )TT_ASYNC;
      heepHdr.Resource = ( uint8_t )rg_md;
      heepHdr.Method_Status = ( uint8_t )method_post;
      heepHdr.Req_Resp_ID = ( uint8_t )0;
      heepHdr.qos = regMetadataQOS;
      heepHdr.callback = NULL;

      /* Transmission may fail if DTLS session not established. */
      retStat = HEEP_MSG_Tx ( &heepHdr, pBuf ); // The called function will free the buffer

      /* Flag that we've sent off the metadata registration.  */
      RegistrationInfo.registrationStatus = REG_STATUS_NOT_CONFIRMED;
      ( void )TIME_UTIL_GetTimeInSysCombined( &currentTime );
      RegistrationInfo.timeRegistrationSent = ( uint32_t )( currentTime / TIME_TICKS_PER_SEC );
      APP_MSG_UpdateRegistrationFile();
   }
   else
   {
      retStat = eBUFFER_ALLOC_FAILED;  //No buffer available, don't send the message for now
   }

   return retStat;
}

/***********************************************************************************************************************

   Function Name: APP_MSG_setMetadataRegistrationTimer(uint32_t timeRemainingMSec)

   Purpose: Create the timer to re-send the metadata registration after the given number
            of milliseconds have elapsed.

   Arguments: uint32_t timeRemainingMSec - The number of milliseconds until the timer goes off.

   Returns: returnStatus_t - indicates if the timer was successfully created

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
returnStatus_t APP_MSG_setMetadataRegistrationTimer ( uint32_t timeRemainingMSec )
{
   timer_t tmrSettings;            // Timer for re-issuing metadata registration
   returnStatus_t retStatus = eSUCCESS;

   RegistrationInfo.activeRegistrationTimeout  = timeRemainingMSec / 1000;
   if ( metadataRegTimerId == INVALID_TIMER_ID )   /* Check for timer already created  */
   {
      ( void )memset( &tmrSettings, 0, sizeof( tmrSettings ) );
      tmrSettings.ulDuration_mS = timeRemainingMSec;
      tmrSettings.pFunctCallBack = APP_MSG_resendRegistrationMetadata;
      retStatus = TMR_AddTimer( &tmrSettings );
      if ( eSUCCESS == retStatus )
      {
         metadataRegTimerId = (uint16_t)tmrSettings.usiTimerId;
      }
      else
      {
         DBG_logPrintf ( 'E', "Unable to add Metadata registration timer" );
      }
   }
   else  /* Already created; restart it.  */
   {
      ( void )TMR_ResetTimer( metadataRegTimerId, timeRemainingMSec );
   }
   return retStatus;
}

/***********************************************************************************************************************

   Function Name: APP_MSG_resendRegistrationMetadata(uint8_t cmd, void *pData)

   Purpose: Called by the timer to re-send the metadata registration.  When the registration is sent,
            a timer will be created - if a confirmation for this registration is received, all is good
            and the timer can be removed.  However if the timer goes off, no confirmation was received,
            so the metadata registration needs to be re-sent.
            Since this code is called from within the timer task, we can't muck around with the
            timer, so we need to notify the app message class to do the actual work.

   Arguments: uint8_t cmd - from the ucCmd member when the timer was created.
              void *pData - from the pData member when the timer was created

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
/*lint -esym(715, cmd, pData ) not used; required by API  */
static void APP_MSG_resendRegistrationMetadata ( uint8_t cmd, void *pData )
{
   /* Use a static buffer. Do not allocate buffer from timer callback */
   if ( resendRegMsgbuf_.x.flag.isFree )
   {
      BM_AllocStatic( &resendRegMsgbuf_, eSYSFMT_TIME );
      OS_MSGQ_Post( &AppMsgHandler_MsgQ_, ( void * )&resendRegMsgbuf_ );
   }
}  /*lint !e818 pData could be pointer to const */
/*lint +esym(715, cmd, pData ) not used; required by API  */

/***********************************************************************************************************************

   Function Name: APP_MSG_initRegistrationInfo(void)

   Purpose: Initialize the Registration Info structure to default values.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: NO

 **********************************************************************************************************************/
void APP_MSG_initRegistrationInfo( void )
{
   RegistrationInfo.pad                          = 0;
   RegistrationInfo.initialRegistrationTimeout   = DEFAULT_INITIAL_REGISTRATION_TIMEOUT;
   RegistrationInfo.minRegistrationTimeout       = MIN_REGISTRATION_TIMEOUT_WAIT;
   RegistrationInfo.maxRegistrationTimeout       = MAX_REGISTRATION_TIMEOUT_WAIT;
   RegistrationInfo.nextRegistrationTimeout      = RegistrationInfo.initialRegistrationTimeout;
   RegistrationInfo.timeRegistrationSent         = 0;
   RegistrationInfo.registrationStatus           = REG_STATUS_NOT_SENT;   // no registratration metadata sent
}

/***********************************************************************************************************************

   Function Name: APP_MSG_SecurityHandler( action, id, value, attr )

   Purpose: Update the value associated with id in the appSecurity_t file.

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t APP_MSG_SecurityHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   uint8_t                 curVal[ 16 ];              // Value currently in the file
   const HEEP_FileInfo_t   *psecFile;                 // Used to search for id
   returnStatus_t          retVal = eAPP_NOT_HANDLED; // Success/failure
   bool                    update;                    // New value is different from previous

   for( psecFile = Security; psecFile->vSize != 0; psecFile++ )
   {
      if ( psecFile->elementID == id ) // Found the element
      {
         if ( method_get == action )   /* Used to "get" the variable. */
         {
            if ( (uint16_t)psecFile->vSize <= MAX_OR_PM_PAYLOAD_SIZE) //lint !e650 !e587 !e685
            {  //The reading will fit in the buffer
               retVal = FIO_fread( &fileHndlSecurity, ( uint8_t * )value, ( fileOffset )psecFile->vOffset,
                                   ( lCnt )psecFile->vSize );
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)psecFile->vSize;
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;   /* Search complete, exit the for loop. */
         }
         else  /* method_put, method_post used to "set" the variable.   */
         {
            retVal = FIO_fread( &fileHndlSecurity, curVal, ( fileOffset )psecFile->vOffset, ( lCnt )psecFile->vSize );
            if ( eSUCCESS == retVal )
            {
               switch ( id )  /*lint !e788 not all enums used within switch */
               {
                  case appSecurityAuthMode:
                  {
                     if ( ( *( uint8_t * )value != 0 ) && ( *( uint8_t * )value != 2 ) )
                     {
                        retVal = eAPP_INVALID_VALUE;
                     }
                     break;
                  }
                  default:    /* Should never get here! All handled cases should have a case block.   */
                  {
                     break;
                  }
               }  /*lint !e788 Not all meterReadingTypes handled, intentionally.   */
               if ( eSUCCESS == retVal )  /* Check for need to update file, only if new value is valid.   */
               {
                  update = ( bool )memcmp( value, curVal, psecFile->vSize ); /* Non-zero means value changed  */
                  if ( update )
                  {
                     /* Update local copy */
                     ( void )memcpy( ( uint8_t * )&appSecurity_.f_appSecurityAuthMode, ( uint8_t * )value,
                                     sizeof( appSecurity_.f_appSecurityAuthMode ) );
                     retVal = FIO_fwrite( &fileHndlSecurity, ( fileOffset )psecFile->vOffset,
                                          ( uint8_t * )value, ( lCnt )psecFile->vSize );
                     if ( ( eSUCCESS == retVal ) && ( psecFile->notify != NULL ) )  /* Check for notification handler */
                     {
                        psecFile->notify();
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
                        LED_checkModeStatus();
#endif
                     }
                  }
               }
            }
         }
         break;   /* Search complete, exit the for loop. */
      }
   }
   return retVal;
}

/***********************************************************************************************************************

   Function Name: APP_MSG_SetDmdResetDateHandler( action, id, value, attr )

   Purpose: Update the value associated with id in the Demand file.

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -esym(715,value) not referenced in Aclara_LC  */
/*lint -esym(818,value) not referenced in Aclara_LC  */
returnStatus_t APP_MSG_SetDmdResetDateHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( scheduledDemandResetDay == id ) // Found the element
   {
      if ( method_get == action )   /* Used to "get" the variable. */
      {
         if (attr != NULL)
         {
            attr->rValLen = (uint8_t)sizeof(uint8_t);
            attr->rValTypecast = (uint8_t)uintValue;
         }
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
         DEMAND_GetResetDay( (uint8_t *)value );
#endif
         retVal= eSUCCESS;
      }
      else  /* method_put, method_post used to "set" the variable.   */
      {
#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
         retVal = DEMAND_SetResetDay( *(uint8_t *)value );
#else
         retVal= eSUCCESS;
#endif
      }
   }
   return retVal;
}


/***********************************************************************************************************************

   Function Name: APP_MSG_setInitialRegistrationTimeout

   Purpose: Update the value for initial registration timeout

   Arguments:  uint16_t uInitialRegistrationTimeout

   Returns:  returnStatus_t based on whether a valid value was submitted

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t APP_MSG_setInitialRegistrationTimeout(uint16_t uInitialRegistrationTimeout)
{

   returnStatus_t retVal = eFAILURE;

   if ( (uInitialRegistrationTimeout >= INITIAL_REGISTRATION_TIMEOUT_MIN_VALUE) &&
        (uInitialRegistrationTimeout <= INITIAL_REGISTRATION_TIMEOUT_MAX_VALUE) )
   {
      RegistrationInfo.initialRegistrationTimeout = uInitialRegistrationTimeout;
      APP_MSG_UpdateRegistrationFile();
      retVal = eSUCCESS;
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }
   return retVal;
}


/***********************************************************************************************************************

   Function Name: APP_MSG_setMinRegistrationTimeout

   Purpose: Update the value for initial registration timeout

   Arguments:  uint16_t uMinRegistrationTimeou

   Returns:  returnStatus_t based on whether a valid value was submitted

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t APP_MSG_setMinRegistrationTimeout(uint16_t uMinRegistrationTimeout)
{
returnStatus_t retVal = eFAILURE;

   if ( (uMinRegistrationTimeout >= MIN_REGISTRATION_TIMEOUT_MIN_VALUE) &&
        (uMinRegistrationTimeout <= MIN_REGISTRATION_TIMEOUT_MAX_VALUE) )
   {
      RegistrationInfo.minRegistrationTimeout = uMinRegistrationTimeout;
      APP_MSG_UpdateRegistrationFile();
      retVal = eSUCCESS;
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}


/***********************************************************************************************************************

   Function Name: APP_MSG_setMaxRegistrationTimeout

   Purpose: Update the value for initial registration timeout

   Arguments:  uint32_t uMaxRegistrationTimeout

   Returns:  returnStatus_t based on whether a valid value was submitted

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t APP_MSG_setMaxRegistrationTimeout(uint32_t uMaxRegistrationTimeout)
{
returnStatus_t retVal = eFAILURE;

   if ( (uMaxRegistrationTimeout >= MAX_REGISTRATION_TIMEOUT_MIN_VALUE) &&
        (uMaxRegistrationTimeout <= MAX_REGISTRATION_TIMEOUT_MAX_VALUE) )
   {
      RegistrationInfo.maxRegistrationTimeout = uMaxRegistrationTimeout;
      APP_MSG_UpdateRegistrationFile();
      retVal = eSUCCESS;
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}


/***********************************************************************************************************************

   Function Name: APP_MSG_initialRegistrationTimeoutHandler

   Purpose: Update the value of initial registration timeout

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be stored
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t APP_MSG_initialRegistrationTimeoutHandler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr) /*lint !e715 parameter not referenced */
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(RegistrationInfo.initialRegistrationTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint16_t *)value = RegistrationInfo.initialRegistrationTimeout;
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(RegistrationInfo.initialRegistrationTimeout);
            attr->rValTypecast = (uint16_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = APP_MSG_setInitialRegistrationTimeout(*(uint16_t *)value);

      if (retVal != eSUCCESS)
      {
        retVal = eAPP_INVALID_VALUE;
      }
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: APP_MSG_minRegistrationTimeoutHandler

   Purpose: Update the value of min registration timeout

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be stored
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t APP_MSG_maxRegistrationTimeoutHandler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr)  /*lint !e715 parameter not referenced  */
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(RegistrationInfo.maxRegistrationTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint32_t *)value = RegistrationInfo.maxRegistrationTimeout;
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)HEEP_getMinByteNeeded(*(int64_t *)value, uintValue, 0);
            attr->rValTypecast = (uint32_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = APP_MSG_setMaxRegistrationTimeout(*(uint32_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: APP_MSG_maxRegistrationTimeoutHandler

   Purpose: Update the value of max registration timeout

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be stored
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t APP_MSG_minRegistrationTimeoutHandler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr)  /*lint !e715 parameter not referenced  */
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(RegistrationInfo.minRegistrationTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint16_t *)value = RegistrationInfo.minRegistrationTimeout;
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(RegistrationInfo.minRegistrationTimeout);
            attr->rValTypecast = (uint16_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = APP_MSG_setMinRegistrationTimeout(*(uint16_t *)value);

      if (retVal != eSUCCESS)
      {
        retVal = eAPP_INVALID_VALUE;
      }
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched
