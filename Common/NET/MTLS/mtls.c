/******************************************************************************

   Filename: MTLS.c

   Global Designator: MTLS_

   Contents: This module contains functionality to receive and parse payloads containing MTLS headers, and to
               validate those messages.

 ******************************************************************************
   Copyright (c) 2017 ACLARA.  All rights reserved.
   This program may not be reproduced, in whole or in part, in any form or by
   any means whatsoever without the written permission of:
      ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

#include "project.h"
#if (USE_MTLS == 1)

/* INCLUDE FILES */
#include "file_io.h"
#include "DBG_SerialDebug.h"
#include "MAC.h"
#include "STACK.h"
#include "mtls.h"
#include "dtls.h"
#include "partition_cfg.h"
#include "MFG_port.h"
#include "pwr_task.h"
#include "time_util.h"
#include "time_sys.h"
#include "fio.h"
#include "ecc108_lib_return_codes.h"
#include "ecc108_apps.h"
#include "ecc108_mqx.h"
#include "byteswap.h"
#include "App_Msg_Handler.h"
#if ( EP == 1 )
#include "pwr_last_gasp.h"
#endif
#include <wolfssl/internal.h>

/***********************************************************************************************************************
   #DEFINE DEFINITIONS
***********************************************************************************************************************/

#define REPLAY_BUFFERS            50
#define MTLS_AUTH_WINDOW         300
#define MTLS_NET_TIME_VARIATION    5

/***********************************************************************************************************************
   MACRO DEFINITIONS
***********************************************************************************************************************/
/*lint -esym(750,MTLS_DBG_CommandLine_Buffers,MTLS_INFO)   */
#if (MTLS_DEBUG == 1)
#define MTLS_INFO(fmt, ... ) \
{ \
   INFO_printf(fmt, ##__VA_ARGS__ ); \
   OS_TASK_Sleep(10); \
}
#define MTLS_DBG_CommandLine_Buffers() (void)DBG_CommandLine_Buffers( 0, NULL )
#else
#define MTLS_INFO(fmt, ... ) (void)0
#define MTLS_DBG_CommandLine_Buffers() (void)0
#endif

#define MTLS_ERROR(fmt, ... ) \
{ \
   ERR_printf( fmt, ##__VA_ARGS__); \
}  /*lint !e2715 token pasting of , is GNU extension. */

/***********************************************************************************************************************
   TYPE DEFINITIONS
***********************************************************************************************************************/

/* Replay buffer  */
PACK_BEGIN
typedef struct
{
   uint16_t signature;  /* 2 bytes of signature field in MTLS message   */
   uint32_t tstamp;     /* 4 bytes of time stamp in MTLS message   */
} replayBuf_t;
PACK_END

/* MTLS configurable attributes file   */
typedef struct
{
   uint32_t  mtlsAuthenticationWindow;     /* The amount of time in seconds that a received message can lag behind the
                                              current device time and still pass message validation. */
   uint32_t  mtlsNetworkTimeVariation;     /* The amount of time in seconds that a received message timestamp can exceed
                                              the current device time and still pass message validation. */
} MtlsConfigAttr_s;

typedef struct
{
   FileHandle_t          handle;
   char           const *FileName;         /*!< File Name           */
   ePartitionName const  ePartition;       /*!<                     */
   filenames_t    const  FileId;           /*!< File Id             */
   void*          const  Data;             /*!< Pointer to the data */
   lCnt           const  Size;             /*!< Size of the data    */
   FileAttr_t     const  Attr;             /*!< File Attributes     */
   uint32_t       const  UpdateFreq;       /*!< Update Frequency    */
} mtlsFile_s;

/***********************************************************************************************************************
   FILE VARIABLE DEFINITIONS
***********************************************************************************************************************/
static uint16_t               mtlsReplayEntries;                  /* Count of entries in the replay buffer. */
static OS_MSGQ_Obj            mtlsMSGQ;                           /* MTLS command queue */
static MtlsConfigAttr_s       mtlsConfigAttr;                     /* MTLS configurable Attributes  */
static mtlsReadOnlyAttributes mtlsAttribs;                        /* Grouped read only attributes. */
static replayBuf_t            mtlsReplayBuffer[ REPLAY_BUFFERS ]; /* Holds digital signature and message timestamp pairs
                                                                     for accepted messages. */
static indicationHandler      indicationHandlerCallback = NULL;
/***********************************************************************************************************************
   CONSTANTS
***********************************************************************************************************************/

static mtlsFile_s ConfigFile =                  /* MTLS Configuration Data File  */
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "MTLS_CONFIG",
   .FileId          = eFN_MTLS_CONFIG,          /* MTLS configuration data */
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &mtlsConfigAttr,
   .Size            = sizeof( mtlsConfigAttr ),
   .UpdateFreq      = 0xffffffff                /* Updated rarely */
}; /*lint !e785 too few initializers   */

static mtlsFile_s AttribsFile =                 /* MTLS Counters Data File  */
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "MTLS_ATTRIBUTES",
   .FileId          = eFN_MTLS_ATTRIBS,         /* MTLS stats data */
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &mtlsAttribs,
   .Size            = sizeof( mtlsAttribs ),
   .UpdateFreq      = 0                         /* Updated frequently */
}; /*lint !e785 too few initializers   */


static mtlsFile_s * const Files[2] =
{
   &ConfigFile,
   &AttribsFile
};

/***********************************************************************************************************************
   File local functions
***********************************************************************************************************************/
/***********************************************************************************************************************

   Function name: mtls_ValidateHeader

   Purpose: Validate MTLS message header.

   Arguments:  MTLS_Packet_t *msg      - incoming message header to check.

   Returns:    boolean true if message header is valid

   Side Effects: None

   Reentrant Code: yes

   Notes:

 **********************************************************************************************************************/
static bool mtls_ValidateHeader( const MTLS_Packet_t *msg )
{
   TIMESTAMP_t currentTime;
   TIMESTAMP_t msgTime;
   bool        retVal = ( bool )false;

   if ( msg->header.version == MTLS_VERSION )
   {
      /* Potentially good header.   */
      /* Get current time for comparison to message timestamp. */
      (void)TIME_UTIL_GetTimeInSecondsFormat( &currentTime );
      msgTime.seconds = msg->header.timeStamp;
      Byte_Swap( (uint8_t *)&msgTime.seconds, sizeof( msgTime.seconds ) );
      /* Now check for timestamp of message in valid range. */
      if ( ( msgTime.seconds > ( currentTime.seconds + mtlsConfigAttr.mtlsNetworkTimeVariation ) ) ||
           ( msgTime.seconds < ( currentTime.seconds - mtlsConfigAttr.mtlsAuthenticationWindow ) ) )
      {
         /* Bad header  */
         mtlsAttribs.mtlsInvalidTimeCount++; /* invalid timestamp. */
         MTLS_ERROR( "mtls: timestamp out of range" );
#if (MTLS_DEBUG == 1)
         {
            sysTime_dateFormat_t sysTime;
            sysTime_t            sTime;

            TIME_UTIL_ConvertSecondsToSysFormat( msgTime.seconds, 0, &sTime );
            ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
            INFO_printf( "Message Time:          %02d/%02d/%04d %02d:%02d:%02d",
                                                         sysTime.month, sysTime.day, sysTime.year,
                                                         sysTime.hour,  sysTime.min, sysTime.sec );

            TIME_UTIL_ConvertSecondsToSysFormat( currentTime.seconds - mtlsConfigAttr.mtlsAuthenticationWindow, 0, &sTime );
            ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
            INFO_printf( "Earliest Allowed Time: %02d/%02d/%04d %02d:%02d:%02d",
                                                         sysTime.month, sysTime.day, sysTime.year,
                                                         sysTime.hour,  sysTime.min, sysTime.sec );

            TIME_UTIL_ConvertSecondsToSysFormat( currentTime.seconds + mtlsConfigAttr.mtlsNetworkTimeVariation, 0, &sTime );
            ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
            INFO_printf( "Latest allowed Time:   %02d/%02d/%04d %02d:%02d:%02d",
                                                         sysTime.month, sysTime.day, sysTime.year,
                                                         sysTime.hour,  sysTime.min, sysTime.sec );

         }
#endif
      }
      else
      {
         /* Potentially good header.   */
         /* Need the key. If no DTLS session is established, can't accept MTLS messages.  */
         if ( !DTLS_IsSessionEstablished() )
         {
            /* Bad Header - no DTLS session established.   */
            mtlsAttribs.mtlsNoSesstionCount++;
            MTLS_ERROR( "mtls: no dtls session established." );
         }
         else
         {
            /* Potentially good header.   */
            /* Check the key ID against the subject Key Id.  */
            PartitionData_t const *p;           /* Used to access the security info partition      */
            uint8_t  key[  SHA_DIGEST_SIZE ];   /* 20 byte subject Key ID.  */

            p = SEC_GetSecPartHandle();                 /* Obtain handle to access internal NV variables   */
            (void)PAR_partitionFptr.parRead( key, (dSize)offsetof( intFlashNvMap_t, subjectKeyId), sizeof( key ), p );

            if ( memcmp( key, &msg->header.keyID, sizeof( msg->header.keyID ) ) != 0 )
            {
               /* Bad header  */
               mtlsAttribs.mtlsInvalidKeyIdCount++; /* Bad key  */
               MTLS_ERROR( "mtls: bad key" );
            }
            else
            {
               retVal = ( bool )true;   /* Passed all checks!   */
            }
         }
      }
   }
   else  /* Bad header  */
   {
      mtlsAttribs.mtlsMalformedHeaderCount++;    /* Bad protocol version.   */
      MTLS_ERROR( "mtls: bad protocol version" );
   }
   return retVal;
}
/***********************************************************************************************************************

   Function name: mtls_CheckReplay

   Purpose: Check for message timestamp in valid range and not present in replay buffer.

   Arguments:  MTLS_Packet_t    *msg        - incoming message header to check.
               TIMESTAMP_t       msgTime     - timestamp of incoming message.

   Returns: bool = true if message passes replay check.

   Side Effects: None

   Reentrant Code: yes

   Notes:

 **********************************************************************************************************************/
static bool mtls_CheckReplay( const MTLS_Packet_t *msg, TIMESTAMP_t msgTime )
{
   replayBuf_t *entry;                    /* Pointer to entries in replay buffer.   */
   uint16_t    idx;                       /* Index into replay buffer.  */
   bool        retVal = ( bool )false;    /* Assume invalid message. */

   if ( msgTime.seconds < mtlsAttribs.mtlsFirstSignedTimestamp )
   {
      mtlsAttribs.mtlsTimeOutOfBoundsCount++;   /* Invalid message.  */
   }
   else
   {
      if ( msgTime.seconds > mtlsAttribs.mtlsLastSignedTimestamp ) /* De Morgan of NOT <=  */
      {
         retVal = true; /* Time stamp outside range of replay buffer, but in bounds: Valid message. */
      }
      else  /* Time stamp falls in range of replay buffer; check for duplicate.  */
      {

         retVal = ( bool )true;
         /* Check for duplicate message. Loop through replay buffer looking for signature matching incoming message
            signature.  */
         for ( idx = 0, entry = mtlsReplayBuffer; idx < ARRAY_IDX_CNT( mtlsReplayBuffer ); entry++, idx++ )
         {
            if ( memcmp( &entry->signature, msg->signature, sizeof( entry->signature ) ) == 0 )
            {
               mtlsAttribs.mtlsDuplicateMessageCount++;
               retVal = ( bool )false;
               break;
            }
         }
      }
   }
   if ( !retVal )
   {
      MTLS_ERROR( "mtls: replay check failed" );
   }

   return retVal;
}
/***********************************************************************************************************************

   Function name: findLatestEntry

   Purpose: Locate latest time stamp in replay buffer

   Arguments:  none

   Returns:    replayBuf_t * pointing to oldest time stamp entry

   Side Effects: None

   Reentrant Code: No

   Notes:

 **********************************************************************************************************************/
static replayBuf_t *findLatestEntry( void )
{
   replayBuf_t *entry;           /* Pointer to entries in replay buffer.   */
   replayBuf_t *latest;          /* Pointer to entries in replay buffer.   */
   uint32_t    latestTime;       /* latest buffered msg time in seconds   */
   uint16_t    idx;              /* Index into replay buffer.  */

   entry    = mtlsReplayBuffer;  /* Set both pointers to start of array.   */
   latest   = mtlsReplayBuffer;
   latestTime = 0;               /* All entries in buffer will be older than date/time 0. */

   /* Loop through all entries in the buffer.   */
   for ( idx = 0; idx < ARRAY_IDX_CNT( mtlsReplayBuffer ); idx++ )
   {
      if ( entry->tstamp > latestTime ) /* Found an older entry:   */
      {
        latestTime = entry->tstamp; /* Update latestTime.      */
        latest = entry;             /* Update entry pointer.   */
      }
      entry++;
   }
   /* latest now points to the slot in mtlsReplayBuffer with the latest entry. */
   return latest;
}
/***********************************************************************************************************************

   Function name: findFirstEntry

   Purpose: Locate First (earliest time) time stamp in replay buffer

   Arguments:  none

   Returns:    replayBuf_t * pointing to First time stamp entry

   Side Effects: None

   Reentrant Code: No

   Notes:

 **********************************************************************************************************************/
static replayBuf_t *findFirstEntry( void )
{
   replayBuf_t *entry;           /* Pointer to entries in replay buffer.   */
   replayBuf_t *first;           /* Pointer to earliest entry in replay buffer.   */
   uint32_t    firstTime;        /* First buffered msg time in seconds   */
   uint16_t    idx;              /* Index into replay buffer.  */

   entry       = mtlsReplayBuffer;  /* Set both pointers to start of array.   */
   first       = mtlsReplayBuffer;
   firstTime   = (uint32_t)-1;     /* All entries in buffer will be earlier than date/time 0xffffffff. */

   /* Loop through all entries in the buffer.   */
   for ( idx = 0; idx < ARRAY_IDX_CNT( mtlsReplayBuffer ); idx++ )
   {
      if ( entry->tstamp < firstTime ) /* Found an earlier entry:   */
      {
        firstTime = entry->tstamp;     /* Update firstTime.      */
        first = entry;                 /* Update entry pointer.   */
      }
      entry++;
   }
   /* first now points to the slot in mtlsReplayBuffer with the first entry. */
   return first;
}
/***********************************************************************************************************************

   Function name: mtls_UpdateReplayBuffer

   Purpose: Adds message to replay buffer.

   Arguments:  uint8_t        *sig     - incoming msg signature
               uint32_t       msgTime  - timestamp of incoming message.

   Returns:    none

   Side Effects: None

   Reentrant Code: No

   Notes:

 *********************************************************************************************************************/
static void mtls_UpdateReplayBuffer( const uint8_t *sig, uint32_t msgTime )
{
   replayBuf_t *add;    /* Pointer to entries in replay buffer.      */
   replayBuf_t *last;   /* Pointer to last entry in replay buffer. */
   bool        removed;

   /* Is there room? */
   if ( mtlsReplayEntries < ARRAY_IDX_CNT( mtlsReplayBuffer ) )
   {
      add = & mtlsReplayBuffer[ mtlsReplayEntries ];   /* Point to first open slot.  */
      mtlsReplayEntries++;
      removed = (bool)false;
   }
   else  /* Make room by removing the first entry.   */
   {
      add = findFirstEntry();
      removed = (bool)true;
   }
   if ( removed && ( add->tstamp >= mtlsAttribs.mtlsFirstSignedTimestamp ) )
   {
      /* add now points to the to be removed entry in mtlsReplayBuffer which has the first timestamp entry. */
      mtlsAttribs.mtlsFirstSignedTimestamp = add->tstamp + 1;
   }
   /* Replace empty or first entry.   */
   add->tstamp = msgTime;
   ( void )memcpy( ( uint8_t *)&add->signature, sig, sizeof( add->signature ) );

   /* Update last signed timestamp  */
   last = findLatestEntry();
   mtlsAttribs.mtlsLastSignedTimestamp = last->tstamp;
}
/***********************************************************************************************************************

   Function name: mtls_VerifySignature

   Purpose: Adds message to replay buffer.

   Arguments:  MTLS_Packet_t    *msg     - incoming message header to check.
               NWK_DataInd_t     *ind     - incoming network indication

   Returns:    boolean true if signature is verified; false otherwise

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 *********************************************************************************************************************/
static bool mtls_VerifySignature( const MTLS_Packet_t *msg, const NWK_DataInd_t *ind )
{
   uint8_t     *sig;
   uint16_t    msgLen;
   uint8_t     verifyResult;
   bool        retVal = ( bool )false; /* Assume invalid message. */

   msgLen = ind->payloadLength - sizeof( msg->signature );  /* Subtract size of signature for msg length.   */
   sig    = ( uint8_t *)&msg->header + msgLen;              /* Signature is just after msg.                 */

   verifyResult = ecc108e_Verify( NETWORK_PUB_KEY, msgLen, ( uint8_t *)&msg->header, sig );
   if ( ECC108_SUCCESS == verifyResult )
   {
      mtls_UpdateReplayBuffer( sig, ind->timeStamp.seconds );
      retVal = ( bool )true;
   }
   else
   {
      mtlsAttribs.mtlsFailedSignatureCount++;
      DBG_logPrintf( 'E', "Signature validation failed: return value: 0x%02hhx.", verifyResult );
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: MTLS_IndicationHandler

   Purpose: Called by upper layer to send a request to MTLS, also
   registered with MAC layer as handler for indications

   Arguments:
        buffer_t *request - buffer that contains a mtls request or

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void MTLS_IndicationHandler( buffer_t *request )
{

#if (MTLS_DEBUG == 1)
   MTLS_INFO( "MTLS_IndicationHandler eSysFmt:%d dataLen:%d", request->eSysFmt, request->x.dataLen ); /*lint !e641 implicit conversion of enum to integral type   */
#endif

   OS_MSGQ_Post( &mtlsMSGQ, ( void * )request  ); /* Function will not return if it fails   */
   return;
}

/* Global functions  */
/***********************************************************************************************************************

   Function name: getMtlsLastSignedTimestamp

   Purpose: Retrieve last time stamped entry in replay buffer

   Arguments:  None

   Returns:    uint32_t last time stamp in replay buffer

   Side Effects: None

   Reentrant Code: yes

   Notes:

 *********************************************************************************************************************/
uint32_t getMtlsLastSignedTimestamp( void )
{
   return findLatestEntry()->tstamp;
}
/***********************************************************************************************************************

   Function name: getMtlsLastSignedTimestamp

   Purpose: Retrieve last time stamped entry in replay buffer

   Arguments:  None

   Returns:    uint32_t last time stamp in replay buffer

   Side Effects: None

   Reentrant Code: yes

   Notes:

 *********************************************************************************************************************/
mtlsReadOnlyAttributes *getMTLSstats( void )
{
   return &mtlsAttribs;
}

/***********************************************************************************************************************

   Function name: MTLS_init

   Purpose: Initialize this module before tasks start running in the system

   Arguments: None

   Returns: eSUCCESS or eFAILURE

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
returnStatus_t MTLS_init( void )
{
   returnStatus_t ret = eSUCCESS; /* Return Value */
   mtlsFile_s     *pFile;

   if ( false == OS_MSGQ_Create( &mtlsMSGQ ) )
   {
      ret = eFAILURE;
   }

   if ( eSUCCESS == ret )
   {
      /* Clear all of the attributes   */
      ( void ) memset( &mtlsConfigAttr, 0, sizeof( mtlsConfigAttr ) );
      ( void ) memset( &mtlsAttribs, 0, sizeof( mtlsAttribs ) );
      mtlsConfigAttr.mtlsAuthenticationWindow = MTLS_AUTH_WINDOW;
      mtlsConfigAttr.mtlsNetworkTimeVariation = MTLS_NET_TIME_VARIATION;

      FileStatus_t fileStatus;

      for( uint8_t i = 0; i < ARRAY_IDX_CNT( Files ); i++ )
      {
         pFile = (mtlsFile_s *)Files[i];
         if ( eSUCCESS == FIO_fopen( &pFile->handle,            /* File Handle so that PHY access the file. */
                                     pFile->ePartition,         /* Search for best partition according to the timing. */
                                     ( uint16_t )pFile->FileId, /* File ID (filename) */
                                     pFile->Size,               /* Size of the data in the file. */
                                     pFile->Attr,               /* File attributes to use. */
                                     pFile->UpdateFreq,         /* The update rate of the data in the file. */
                                     &fileStatus ) )              /* Contains the file status */
         {
            if ( fileStatus.bFileCreated )
            {
               ret = FIO_fwrite( &pFile->handle, 0, pFile->Data, pFile->Size);
            }
            else
            {
               ret = FIO_fread( &pFile->handle, pFile->Data, 0, pFile->Size);
            }
         }
         else
         {
            ret = eFAILURE;
         }
      }

      if ( ret == eSUCCESS )
      {
         NWK_RegisterIndicationHandler( ( uint8_t )UDP_MTLS_PORT, MTLS_IndicationHandler );
      }
   }
   mtlsAttribs.mtlsFirstSignedTimestamp = mtlsAttribs.mtlsLastSignedTimestamp + 1;  /* Default value  */
   mtlsReplayEntries = 0;  /* No entries in the buffer at power up!  */

   return ret;
}
/******************************************************************************************************************** */
/*lint -esym(715,Arg0) */
/***********************************************************************************************************************
   Function Name: MTLS_Task

   Purpose: Task for the MTLS networking layer (combination of IP/UDP handling) Initialize the MTLS library and setup
   a context.  We won't try to connect until the task starts up.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: none
***********************************************************************************************************************/
void MTLS_Task( uint32_t Arg0 )
{
   buffer_t       *commandBuf;         /* MTLS command buffer */
   NWK_DataInd_t  *nwkIndication;      /* Incoming message type   */
   MTLS_Packet_t  *msg;
   uint16_t       msgLength;

   MTLS_INFO( "MTLS_Task starting" );

   for ( ;; )
   {
      (void)OS_MSGQ_Pend( &mtlsMSGQ, ( void * )&commandBuf, OS_WAIT_FOREVER );
      mtlsAttribs.mtlsInputMessageCount++;     /* Received an indication; bump counter   */

      /* Extract the message. */
      nwkIndication = ( NWK_DataInd_t *)( void * )commandBuf->data;
      msg = ( MTLS_Packet_t *)( void * )nwkIndication->payload;

      msgLength = nwkIndication->payloadLength;
      msgLength -= sizeof( msg->header );
      msgLength -= sizeof( msg->signature );
#if (MTLS_DEBUG == 1)
      DBG_logPrintHex ( 'M', "Header: ", ( uint8_t *)&msg->header, sizeof( msg->header ) );
      DBG_logPrintHex ( 'M', "Payload: ", msg->msg, msgLength );
      DBG_logPrintHex ( 'M', "Signature: ", ( uint8_t *)msg->msg + msgLength, sizeof( msg->signature ) );
#endif

      if (  mtls_ValidateHeader( msg )                         &&
            mtls_CheckReplay( msg, nwkIndication->timeStamp )  &&
            mtls_VerifySignature( msg, nwkIndication )         &&
            ( NULL != indicationHandlerCallback )
         )
      {

         buffer_t *appMsg = BM_allocStack( sizeof(NWK_DataInd_t) + msgLength );

         if ( appMsg != NULL )
         {
            NWK_DataInd_t *stack_indication; /* Create a pointer to NWK_DataInd_t, located in the appMsg buffer.  */

            appMsg->eSysFmt                  = eSYSFMT_NWK_INDICATION;
            stack_indication                 = ( NWK_DataInd_t * )( void * )appMsg->data;
            stack_indication->timePresent    = nwkIndication->timePresent; /* Copy timestamp present, thru UDP ports.  */
            stack_indication->timeStamp      = nwkIndication->timeStamp;
            stack_indication->qos            = nwkIndication->qos;
            stack_indication->payloadLength  = msgLength;
            stack_indication->dstUdpPort     = nwkIndication->dstUdpPort;
            stack_indication->srcUdpPort     = nwkIndication->srcUdpPort;
            /* Point the payload to the data immediately following the payload pointer */
            stack_indication->payload        = appMsg->data + offsetof( NWK_DataInd_t, payload ) + sizeof( stack_indication->payload );


            (void)memcpy(&stack_indication->dstAddr, &nwkIndication->dstAddr, sizeof(stack_indication->dstAddr));
            (void)memcpy(&stack_indication->srcAddr, &nwkIndication->srcAddr, sizeof(stack_indication->srcAddr));
            (void)memcpy( stack_indication->payload, msg->msg, msgLength );
            ( *indicationHandlerCallback )( appMsg  );
            mtlsAttribs.mtlsOutputMessageCount++;  /* Message sent to upper layers; bump counter   */
         }
      }
      ( void )FIO_fwrite( &AttribsFile.handle, 0, AttribsFile.Data, AttribsFile.Size );   /* Update all counters with each MTLS message   */

      BM_free( commandBuf );     /* Be sure to free the buffer passed in! */
   }
}

/***********************************************************************************************************************
   Function Name: MTLS_RegisterIndicationHandler

   Purpose: This function is called by an upper layer module to * register a callback function.  That function will
            accept an indication (data.confirmation, data.indication, etc) from this * module.  That function is also
            responsible for freeing the indication buffer.

   Arguments: pointer to a funciton of type MTLS_RegisterIndicationHandler

   Returns: none

   Note: Currently only one upper layer can receive indications but
     the function could be expanded to have an list of callbacks if
     needed.
 **********************************************************************************************************************/
void MTLS_RegisterIndicationHandler ( uint8_t port, indicationHandler pCallback )
{
   ( void )port;
   indicationHandlerCallback = pCallback;
}

/***********************************************************************************************************************
   Function Name: MTLS_setMTLSnetworkTimeVariation

   Purpose: API to set the network time variation value

   Arguments: MTLS network time variation value

   Returns: Returns true if a valid value is received. False, otherwise

 **********************************************************************************************************************/
uint32_t MTLS_setMTLSnetworkTimeVariation( uint32_t NetworkTimeVariation )
{
   if ( NetworkTimeVariation <= 30 )
   {
      mtlsConfigAttr.mtlsNetworkTimeVariation = NetworkTimeVariation;
      ( void )FIO_fwrite( &ConfigFile .handle, 0, ConfigFile .Data, ConfigFile .Size );
   }
   return mtlsConfigAttr.mtlsNetworkTimeVariation;
}

/***********************************************************************************************************************
   Function Name: MTLS_setMTLSauthenticationWindow

   Purpose: API to set the Authentication Window value

   Arguments: MTLS Authentication Window value

   Returns: Returns true if a valid value is received. False, otherwise

 **********************************************************************************************************************/
uint32_t MTLS_setMTLSauthenticationWindow( uint32_t AuthenticationWindow )
{
   if ( AuthenticationWindow <= 1800 )
   {
      mtlsConfigAttr.mtlsAuthenticationWindow = AuthenticationWindow;
      ( void )FIO_fwrite( &ConfigFile .handle, 0, ConfigFile .Data, ConfigFile .Size );
   }
   return mtlsConfigAttr.mtlsAuthenticationWindow;
}
/***********************************************************************************************************************

   \fn         MTLS_OR_PM_Handler

   \brief      Get/Set parameters related to MTLS

   \param      action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   \return     returnStatus_t

   \details    If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
returnStatus_t MTLS_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t retVal = eFAILURE;   /* Success/failure   */
   uint32_t       locValue;

   if ( method_get == action )         /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case mtlsAuthenticationWindow:
         {
            if ( sizeof( mtlsConfigAttr.mtlsAuthenticationWindow ) <= MAX_OR_PM_PAYLOAD_SIZE )  /*lint !e506 !e774 constant comparison */
            {  //The reading will fit in the buffer
               *(uint32_t *)value = mtlsConfigAttr.mtlsAuthenticationWindow;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mtlsConfigAttr.mtlsAuthenticationWindow, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case mtlsNetworkTimeVariation:
         {
            if ( sizeof( mtlsConfigAttr.mtlsNetworkTimeVariation ) <= MAX_OR_PM_PAYLOAD_SIZE )  /*lint !e506 !e774 constant comparison */
            {  //The reading will fit in the buffer
               *(uint32_t *)value = mtlsConfigAttr.mtlsNetworkTimeVariation;
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)mtlsConfigAttr.mtlsNetworkTimeVariation, uintValue, 0);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         default:
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      locValue = *(uint32_t * )value;
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case mtlsAuthenticationWindow:
         {
            if ( locValue == MTLS_setMTLSauthenticationWindow( locValue ) )   /* Attempt to set new value.  */
            {
               retVal = eSUCCESS;
            }
            break;
         }
         case mtlsNetworkTimeVariation:
         {
            if ( locValue == MTLS_setMTLSnetworkTimeVariation( locValue ) )   /* Attempt to set new value.  */
            {
               retVal = eSUCCESS;
            }
            break;
         }
         default:
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}
#if (MTLS_DEBUG == 1)
void MTLS_dumpReplayBuffer( void )
{
   sysTime_dateFormat_t sysTime;
   sysTime_t            sTime;
   uint16_t             idx;
   replayBuf_t          *entry;                    /* Pointer to entries in replay buffer.   */
   uint8_t              *sig;

   DBG_logPrintf( 'M', "MTLS Replay buffer ( %d entries )", mtlsReplayEntries );
   for ( idx = 0, entry = mtlsReplayBuffer; idx < mtlsReplayEntries; entry++, idx++ )
   {
      sig = ( uint8_t *)&entry->signature;
      TIME_UTIL_ConvertSecondsToSysFormat( entry->tstamp, 0, &sTime );
      ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
      INFO_printf( "%2d Time: %02d/%02d/%04d %02d:%02d:%02d (0x%08x), Signature: 0x%02hhx%02hhx",
                                                idx+1,
                                                sysTime.month, sysTime.day, sysTime.year,
                                                sysTime.hour,  sysTime.min, sysTime.sec,
                                                entry->tstamp,
                                                sig[0],
                                                sig[1] );
   }
   DBG_printfNoCr( "\n\n");
}
#endif
/***********************************************************************************************************************

   Function name: MTLS_Stats

   Purpose: This function is used to print the MTLS status to the terminal.

   Arguments:

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void MTLS_Stats( void )
{
   sysTime_dateFormat_t sysTime;
   sysTime_t            sTime;

#if (MTLS_DEBUG == 1)
   MTLS_dumpReplayBuffer();
#endif   /* MTLS_DEBUG  */

   INFO_printf( "mtlsInputMessageCount:     %d", mtlsAttribs.mtlsInputMessageCount );
   INFO_printf( "mtlsOutputMessageCount:    %d", mtlsAttribs.mtlsOutputMessageCount );
   INFO_printf( "mtlsTimeOutOfBoundsCount:  %d", mtlsAttribs.mtlsTimeOutOfBoundsCount );
   INFO_printf( "mtlsInvalidTimeCount:      %d", mtlsAttribs.mtlsInvalidTimeCount );
   INFO_printf( "mtlsFailedSignatureCount:  %d", mtlsAttribs.mtlsFailedSignatureCount );
   INFO_printf( "mtlsMalformedHeaderCount:  %d", mtlsAttribs.mtlsMalformedHeaderCount );
   INFO_printf( "mtlsInvalidKeyIdCount:     %d", mtlsAttribs.mtlsInvalidKeyIdCount );
   INFO_printf( "mtlsNoSesstionCount:       %d", mtlsAttribs.mtlsNoSesstionCount );
   INFO_printf( "mtlsDuplicateMessageCount: %d", mtlsAttribs.mtlsDuplicateMessageCount );
   TIME_UTIL_ConvertSecondsToSysFormat( mtlsAttribs.mtlsLastResetTime.seconds, 0, &sTime );
   ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
   INFO_printf( "mtlsLastResetTime:         %02d/%02d/%04d %02d:%02d:%02d, %ul fractional seconds",
                                            sysTime.month, sysTime.day, sysTime.year,
                                            sysTime.hour,  sysTime.min, sysTime.sec,
                                            mtlsAttribs.mtlsLastResetTime.QFrac );

   INFO_printf( "mtlsAuthenticationWindow:  %d", mtlsConfigAttr.mtlsAuthenticationWindow );
   INFO_printf( "mtlsNetworkTimeVariation:  %d", mtlsConfigAttr.mtlsNetworkTimeVariation );

   TIME_UTIL_ConvertSecondsToSysFormat( mtlsAttribs.mtlsFirstSignedTimestamp, 0, &sTime );
   ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
   INFO_printf( "mtlsFirstSignedTimestamp:  %02d/%02d/%04d %02d:%02d:%02d (0x%08x)",
                                             sysTime.month, sysTime.day, sysTime.year,
                                             sysTime.hour,  sysTime.min, sysTime.sec,
                                             mtlsAttribs.mtlsFirstSignedTimestamp );

   TIME_UTIL_ConvertSecondsToSysFormat( mtlsAttribs.mtlsLastSignedTimestamp, 0, &sTime );
   ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &sTime, &sysTime );
   INFO_printf( "mtlsLastSignedTimestamp:   %02d/%02d/%04d %02d:%02d:%02d (0x%08x)",
                                             sysTime.month, sysTime.day, sysTime.year,
                                             sysTime.hour,  sysTime.min, sysTime.sec,
                                             mtlsAttribs.mtlsLastSignedTimestamp );
}
#if 0
/***********************************************************************************************************************

   Function name: MTLS_GetCounters

   Purpose: Used to get a copy of the MTLS Counters.
            It will copy the data to the destination buffer provided.

   Arguments:
         MTLS_counters_t *pDst - Counters

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void MTLS_GetCounters( MTLS_counters_t *pDst )
{
   ( void )memcpy( pDst, &_mtlsCachedAttr.counters, sizeof( MTLS_counters_t ) );
}
#endif

/***********************************************************************************************************************

   Function name: MTLS_Reset

   Purpose: This function resets the read only attributes.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
returnStatus_t  MTLS_Reset( void )
{
   ( void ) memset( &mtlsAttribs, 0, sizeof( mtlsAttribs ) );                       /* Clear readonly attributes. */
   ( void ) memset( mtlsReplayBuffer, 0, sizeof( mtlsReplayBuffer ) );              /* Clear replay buffer.       */
   mtlsReplayEntries = 0;                                                           /* Clear replay buffer index. */
   mtlsAttribs.mtlsFirstSignedTimestamp = mtlsAttribs.mtlsLastSignedTimestamp + 1;  /* Default value  */
   ( void )TIME_UTIL_GetTimeInSecondsFormat( &mtlsAttribs.mtlsLastResetTime );
   return FIO_fwrite( &AttribsFile.handle, 0, ( uint8_t * )&mtlsAttribs, sizeof( mtlsAttribs ) );
}

#else
#error "USE_MTLS not defined"
#endif

