/******************************************************************************

   Filename: DTLS.h

   Global Designator:

   Contents:

 ******************************************************************************
   Copyright (c) 2014-2020 ACLARA.  All rights reserved.
   This program may not be reproduced, in whole or in part, in any form or by
   any means whatsoever without the written permission of:
      ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef DTLS_H
#define DTLS_H

#ifdef DTLS_GLOBALS
#define DTLS_EXTERN
#else
#define DTLS_EXTERN extern
#endif

/* INCLUDE FILES */
#include <wolfssl/wolfcrypt/random.h>

/* #DEFINE DEFINITIONS */
#define DTLS_SESSION_STATE_STRING_MAX (96)
#define DTLS_SERVER_CERTIFICATE_SERIAL_MAX_SIZE (32)

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef enum
{
   DTLS_SESSION_NONE_e,
   DTLS_SESSION_CONNECTING_e,
   DTLS_SESSION_CONNECTED_e,
   DTLS_SESSION_SUSPENDED_e,
   DTLS_SESSION_FAILED_e,
   DTLS_SESSION_SHUTDOWN_e
} DtlsSessionState_e;

/* Counter indicators */
typedef enum DTLS_COUNTER_e
{
   eDTLS_IfInUcastPkts,             /* Number of Unicast in packets */
   eDTLS_IfInSecurityErrors,        /* Number of sercurity errors */
   eDTLS_IfInNoSessionErrors,       /* Number of session failures */
   eDTLS_IfInDuplicates,            /* Number of duplicate */
   eDTLS_IfInNonApplicationPkts,    /* Number of Non-Application Incoming packets */
   eDTLS_IfOutUcastPkts,            /* Number of Unicast Outgoing packets */
   eDTLS_IfOutNonApplicationPkts,   /* Number of Non-Application Outgoing packets */
   eDTLS_IfOutNoSessionErrors       /* Number of Session errors */
} DTLS_COUNTER_e;

typedef void ( *DTLS_IndicationHandler )( buffer_t *indication );
typedef void ( *DTLS_ConnectResultCallback_t )( returnStatus_t status );
typedef void ( *DTLS_SessionStateChangedCallback_t )( DtlsSessionState_e sessionState );

/*!
   Reset.Request - parameters
*/
typedef enum
{
   eDTLS_RESET_ALL,       /*!< Reset all statistics */
   eDTLS_RESET_STATISTICS /*!< Reset some staistics */
} DTLS_RESET_e;

/*!
   DTLS Get/Set Attributes
*/
typedef enum
{
   eDtlsAttr_dtlsIfInUcastPkts,               /*!< The number of In Unicast Packets */
   eDtlsAttr_dtlsIfInSecurityErrors,          /*!< The number of In Security Errors */
   eDtlsAttr_dtlsIfInNoSessionErrors,         /*!< The number of In Session Errors  */
   eDtlsAttr_dtlsIfInDuplicates,              /*!< The number of In Duplicates */
   eDtlsAttr_dtlsIfInNonApplicationPkts,      /*!< The number of In Non Application Packets */
   eDtlsAttr_dtlsIfOutUcastPkts,              /*!< The number of Out Unicast Packets */
   eDtlsAttr_dtlsIfOutNonApplicationPkts,     /*!< The number of Out Non Application Packets */
   eDtlsAttr_dtlsIfOutNoSessionErrors         /*!< The number of Out Session Errors */
// eDtlsAttr_appIfInSecurityErrors            /*!< The number of In Security Errors */
} DTLS_ATTRIBUTES_e;

/* These are the DTLS Counters */
typedef struct
{
   uint32_t dtlsIfInUcastPkts;
   uint32_t dtlsIfInSecurityErrors;
   uint32_t dtlsIfInNoSessionErrors;
   uint32_t dtlsIfInDuplicates;
   uint32_t dtlsIfInNonApplicationPkts;
   uint32_t dtlsIfOutUcastPkts;
   uint32_t dtlsIfOutNonApplicationPkts;
   uint32_t dtlsIfOutNoSessionErrors;
} DTLS_counters_t;

/* CONSTANTS */

/* GLOBAL VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
returnStatus_t DTLS_init( void );
void DTLS_Task( taskParameter );
void DTLS_Stats( bool option );
void DTLS_Request( buffer_t *request );
void DTLS_RegisterIndicationHandler( uint8_t port, DTLS_IndicationHandler pCallback );
returnStatus_t DTLS_UartConnect( DTLS_ConnectResultCallback_t pResultCallback );
void DTLS_UartDisconnect( void );
void DTLS_GetCounters( DTLS_counters_t *dtls_counters );
void DTLS_CounterInc( DTLS_COUNTER_e eDtlsCounter );
uint32_t DTLS_DataRequest( uint8_t port,
                           uint8_t qos,
                           uint16_t size,
                           uint8_t const data[],
                           NWK_Address_t const *dst_addr,
                           NWK_Override_t const *override,
                           MAC_dataConfCallback callback,
                           NWK_ConfirmHandler confirm_cb );
bool DTLS_IsSessionEstablished( void );
void DTLS_UartTransportIndication( buffer_t *request );
void DTLS_RfTransportIndication( buffer_t *request );
void DTLS_RegisterSessionStateChangedCallback( DTLS_SessionStateChangedCallback_t sessionStateChangedCallback );
uint16_t DTLS_GetServerCertificateSerialNumber( void *ptr );
DtlsSessionState_e DTLS_GetSessionState( void );
void DTLS_PrintSessionStateInformation( void );
void DTLS_ReconnectRf( void );
void DTLS_Shutdown( void );
void DTLS_AppSecurityAuthModeChanged( void );
returnStatus_t DTLS_setMaxAuthenticationTimeout( uint32_t uMaxAuthenticationTimeout );
uint32_t DTLS_getMaxAuthenticationTimeout( void );
returnStatus_t DTLS_setMinAuthenticationTimeout( uint16_t uMinAuthenticationTimeout );
uint16_t DTLS_getMinAuthenticationTimeout( void );
returnStatus_t DTLS_setInitialAuthenticationTimeout( uint16_t uInitialAuthenticationTimeout );
uint16_t DTLS_getInitialAuthenticationTimeout( void );
returnStatus_t DTLS_getDtlsValue( meterReadingType id, uint8_t *dtlsValue );
uint16_t computeCertLength( uint8_t *cert );
uint16_t DTLS_GetPublicKey( uint8_t * key );
int32_t DtlsGenerateSeed( uint8_t* output, uint32_t sz );
#endif /* DTLS_H */
