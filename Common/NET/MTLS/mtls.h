/******************************************************************************
 *
 * Filename: MTLS.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2014 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef MTLS_H
#define MTLS_H

#ifdef MTLS_GLOBALS
#define MTLS_EXTERN
#else
#define MTLS_EXTERN extern
#endif

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */
#define MTLS_VERSION 0

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/*!
 * Reset.Request - parameters
 */
typedef enum
{
   eMTLS_RESET_ALL,       /*!< Reset all statistics */
} MTLS_RESET_e;

/* MTLS message fields   */
PACK_BEGIN
typedef struct
{
   uint8_t  version;       /* revision of protocol - 0 initially. */
   uint32_t timeStamp;     /* Message timestamp.                  */
   uint16_t keyID;         /* Message key.                        */
} MTLS_Header_t;
PACK_END

PACK_BEGIN
typedef struct
{
   MTLS_Header_t  header;        /* MTLS header info, above.      */
   uint8_t        msg[1];        /* Variable length message body. */
   uint8_t        signature[64]; /* Message signature.            */
} MTLS_Packet_t;
PACK_END

typedef void ( *indicationHandler )( buffer_t *indication );

typedef struct
{
   /* Individual attributes that are NOT modifiable individually. They are managed by the firmware, individually; may be
      reset en-masse on command (reset)   */
   uint32_t    mtlsDuplicateMessageCount; /* The number of messages since Last reset that have failed the replay check
                                             because they were duplicates of already received messages.  */
   uint32_t    mtlsFailedSignatureCount;  /* Number of messages since the last reset that have failed signature
                                             verification.   */
   uint32_t    mtlsFirstSignedTimestamp;  /* Lower bound timestamp for accepting a message during the replay check
                                             function. */
   uint32_t    mtlsInputMessageCount;     /* Number of messages since the last reset that have been received from the lower
                                             layers.   */
   uint32_t    mtlsInvalidKeyIdCount;     /* Number of messages since the last reset that did not have a valid Key ID. */
   uint32_t    mtlsInvalidTimeCount;      /* number of messages since last reset that did not have a valid timestamp. */
   TIMESTAMP_t mtlsLastResetTime;         /* Date and time of the last reset operation. */
   uint32_t    mtlsLastSignedTimestamp;   /* Last time that a message was accepted as valid.   */
   uint32_t    mtlsMalformedHeaderCount;  /* Number of messages since the last reset that did not have a properly formatted
                                             MTLS header or that indicated an unsupported field value. */
   uint32_t    mtlsNoSesstionCount;       /* Count of messages received while no DTLS session established.  */
   uint32_t    mtlsOutputMessageCount;    /* Number of messages since the last reset that have passed all validations and
                                             been sent to the upper layers.   */
   uint32_t    mtlsTimeOutOfBoundsCount;  /* Number of messages since the last reset whose timestamp fell out of the
                                             acceptable range as determined by mtlsFirstSignedTimestamp and
                                             mtlsLastSignedTimestamp.  */
} mtlsReadOnlyAttributes;

/* CONSTANTS */

/* GLOBAL VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
returnStatus_t            MTLS_init( void );
void                      MTLS_Task( uint32_t Arg0 );
void                      MTLS_RegisterIndicationHandler ( uint8_t port, indicationHandler pCallback );
uint32_t                  MTLS_setMTLSnetworkTimeVariation( uint32_t mtlsNetworkTimeVariation );
uint32_t                  MTLS_setMTLSauthenticationWindow( uint32_t mtlsAuthenticationWindow );
void                      MTLS_Stats( void );
uint32_t                  getMtlsLastSignedTimestamp( void );
mtlsReadOnlyAttributes   *getMTLSstats( void );
returnStatus_t            MTLS_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t            MTLS_Reset( void );

#if (MTLS_DEBUG == 1)
void MTLS_dumpReplayBuffer( void );
#endif   /* MTLS_DEBUG  */

#endif   /* MTLS_H */
