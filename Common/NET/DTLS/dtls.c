/******************************************************************************

   Filename: DTLS.c

   Global Designator: DTLS_

   Contents: This module contains functionality to receive and parse payloads containing DTLS headers, and to
               encode messages adding that headers.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2015-2021 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

#include "project.h"
#if ( USE_DTLS == 1 )

/* INCLUDE FILES */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "file_io.h"
#include "DBG_SerialDebug.h"
#include "MAC.h"
#include "rand.h"
#include "STACK.h"
#include "dtls.h"
#include "EVL_event_log.h"
#include "partition_cfg.h"
#include "dvr_intFlash_cfg.h"
#include "MFG_port.h"
#include "pwr_task.h"
#include "timer_util.h"
#include "time_util.h"
#include "time_sys.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "fio.h"
#endif
#include "ecc108_lib_return_codes.h"
#include "ecc108_apps.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include "ecc108_mqx.h"
#endif
#include "byteswap.h"
#include "user_settings.h"  // Added for defining WOLFSSL_DTLS  /* TODO: RA6E1: SG Determine if the need for this can be migrated to FreeRTOS headers */
#include <wolfssl/ssl.h>
#include <wolfssl/internal.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/quicksession.h>
#include <wolfssl/wolfcrypt/logging.h>
#include "App_Msg_Handler.h"
#include "sys_busy.h"

#if ( DCU == 1 )
#include "version.h"
#endif

#if ( EP == 1 )
#include "pwr_last_gasp.h"
#endif

#if ( DTLS_DEBUG == 1 )
#include "DBG_CommandLine.h"
#endif
#if ( USE_USB_MFG == 1 )
#include "virtual_com.h"
#endif

/* #DEFINE DEFINITIONS */
#define DROP_SESSION_ON_EXPIRED_CERT 0 /* Set non-zero to require "current" certificate at head to process messages. */

#define SIZE_OF_OID ( 5 )                                   /* OID size */
#if ( BUILD_FVT_SHORT_DTLS_TIMEOUT == 0 )
#define DTLS_SESSION_TIMEOUT_RF ( 60 * 60 * 24 * 3 * 1000 ) /* 3 days */
#else
#define DTLS_SESSION_TIMEOUT_RF ( 5 * 60 * 1000 )           /* 5 minutes */
#endif
#define DTLS_TRANSPORT_TIMEOUT_RF ( 60 * 10 * 1000 )        /* 10 minutes */
#define DTLS_SESSION_TIMEOUT_SERIAL ( 60 * 5 * 1000 )       /* 5 minutes */
#define DTLS_TRANSPORT_TIMEOUT_SERIAL ( 10 * 1000 )         /* 10 seconds */
#define DTLS_GROUP_MESSAGES ( 1 )                           /* Process group messages from DTLS */
#define DTLS_MAX_CERT_SIZE ( 512 )                          /* Maximum cert size */
#define DTLS_MILLISECONDS_IN_SECOND ( 1000 )                /* Number of millisconds in a second */
#define DTLS_MAX_RX_COUNT ( 5 )                             /* Max number of failed receives before connection is torn down */
#define DTLS_RF_QOS ( 0x39 )
#define DTLS_EXPECTED_MAX_RX ( 1300 )
#define DTLS_TIME_CHECK_DELAY ( 30 * 1000 )
#define DTLS_MFG_SERIAL_PORT ( 128 )                        /* MFg Serial Port Interface */
#if ( DCU == 1 )
#define DTLS_DCUII_HACK_INTERFACE ( 254 )                   /* DCUII Hack Interface */
#endif
#if ( EP == 1 )
#define DTLS_MAC_RF_PORT ( 0xFF )                           /* MAC RF Interface */
#endif
#define DTLS_COMMAND_EVENT_TIMEOUT ( 5000 )

#define DTLS_MAJOR_MAX_SIZE_WOLF36 ( 1024 )
//#define DTLS_MINOR_MAX_SIZE_WOLF36 ( 96 )

#define DTLS_WOLFSSL4x_SESSION_MAX_SIZE ( 500 )    /* Ming: Added for wolfSSL 3.6 to 4.x upgrade   */
#define QUIET_SHUTDOWN  ( bool )true
#define NORMAL_SHUTDOWN ( bool )false

#define DTLS_MIN_AUTH_TIMEOUT_DEFAULT  ( 600 )    /* The default value for minAuthorizationTimeout */
#define DTLS_MAX_AUTH_TIMEOUT_DEFAULT  ( 86400 )  /* The default value for maxauthorizationTimeout */
#define DTLS_INIT_AUTH_TIMEOUT_DEFAULT ( 1800 )   /* The default initial value for authorization timeout */
#define DTLS_MAX_AUTH_UPPER_BOUND      ( 172800 ) /* Maximum allowable set point (seconds) for maxAuthorizationTimeout */
#define DTLS_MAX_AUTH_LOWER_BOUND      ( 43200 )  /* Minimum allowable set point (seconds) for maxAuthorizationTimeout */
#define DTLS_MIN_AUTH_UPPER_BOUND      ( 600 )    /* Maximum allowable set point (seconds) for minAuthorizationTimeout */
#define DTLS_MIN_AUTH_LOWER_BOUND      ( 30 )     /* Minimum allowable set point (seconds) for minAuthorizationTimeout */
#define DTLS_INITIAL_AUTH_UPPER_BOUND  ( 43200 )  /* Maximum allowable set point (seconds) for initialAuthorizationTimeout */
#define DTLS_INITIAL_AUTH_LOWER_BOUND  ( 1800 )   /* Minimum allowable set point (seconds) for initialAuthorizationTimeout */

/*
#if ( DTLS_SESSION_MAJOR_MAX_SIZE > PART_NV_DTLS_MAJOR_BANK_SIZE )
#error "DTLS_SESSION_MAJOR_MAX_SIZE exceeds PART_NV_DTLS_MAJOR_BANK_SIZE"
#endif

#if ( DTLS_SESSION_MINOR_MAX_SIZE > PART_NV_DTLS_MINOR_BANK_SIZE )
#error "DTLS_SESSION_MINOR_MAX_SIZE exceeds PART_NV_DTLS_MINOR_BANK_SIZE"
#endif
*/

/* MACRO DEFINITIONS */
#if( RTOS_SELECTION == FREE_RTOS )
#define DTLS_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define DTLS_NUM_MSGQ_ITEMS 0
#endif
/* TYPE DEFINITIONS */
/* An indicator of which pipe the DTLS is connected to */
typedef enum
{
   DTLS_TRANSPORT_NONE_e = 0,       /* No dtls active on either RF or SERIAL */
   DTLS_TRANSPORT_RF_e,             /* RF dtls active */
   DTLS_TRANSPORT_SERIAL_e          /* SERIAL active */
} DtlsTransport_e;

/* DTLS Task commands */
typedef enum
{
   DTLS_CMD_CONNECT_SERIAL_e = 0,   /* Connect the DTLS to the SERIAL port */
   DTLS_CMD_DISCONNECT_SERIAL_e,    /* Disconnect the DTLS from the SERAIL port */
   DTLS_CMD_CONNECT_RF_e,           /* Connect the DTLS to the RF port */
   DTLS_CMD_DISCONNECT_RF_e,        /* Disconnect the DTLS from the RF port */
   DTLS_CMD_RECONNECT_RF_e,         /* Stops current connections, invalidates session cache, attempts reconnect */
   DTLS_CMD_TRANSPORT_RX_e,         /* Encrypted data received from the transport */
   DTLS_CMD_APPDATA_TX_e,           /* App data that needs to be encrypted and transported */
   DTLS_CMD_APP_SECURITY_MODE_CHANGED_e,      /* The app security mode has changed */
   DTLS_CMD_SHUTDOWN_e              /* Shutdown connections, sleep forever */
} DtlsCommand_e;

/*lint -esym(714,DtlsEvents_e) not referenced   */
/*lint -esym(528,DtlsEvents_e) not referenced   */
/*lint -esym(843,DtlsEvents_e) not referenced   */
typedef enum
{
   DTLS_EVENT_SHUTDOWN_COMPLETE_e = 1
} DtlsEvents_e;

/* The following struct was created to allow patching SRFN DCU's.  In v1.40, the DtlsMajorSession_s structure contained
   a member of type sysTime_t sized at 8 bytes.  This type, sysTime_t, grew larger in size during v1.50 from
   8 bytes to 16 bytes.  This resulted in the size of DtlsMajorSession_s growing by 8 bytes.  Currenlty there is
   no way in the DFW process to modify the parition to accomadate the structures change in size. As result, patching
   from v1.40 to v1.70 is not possible because after updating to v1.70, the firmware is expecting the
   DtlsMajorSession_s parition data to be 8 bytes larger and device startup will fail attempting to read the file.
   This change will be updated to only be included in the SRFN DCU product.

   SRFN I210+c and DCU2+ fw versions have been released using the larger 16 byte sysTime_t.  Patches for these
   products do not need to use the special DtlsTime_t data structure.  In the future, when the ability to update the
   DtlsMajorSession_s becomes available via DFW, this structure can be removed from the code.  All released prodcuct
   can then used the 16 byte sysTime_t in the DtlsMajorSession_s partition. */
#if ( DCU == 1 )
typedef struct /* DCU2 ONLY */
{
   uint32_t date;         /* Number of days since epoc */
   uint32_t time;         /* Number of MS since midnight */
} DtlsTime_t;

typedef struct /* DCU2 ONLY */
{
   DtlsTime_t serverCertificateExpirationSysTime;  /* declared to accomadate SRFN DCU patches, see comment in DtlsTime_t struct declaration */
   uint16_t dataSize;
   uint8_t data[DTLS_MAJOR_MAX_SIZE_WOLF36];       /* Ming: Temprory change to DTLS_MAJOR_MAX_SIZE size to make the buff size the same as wolfSSL36 */
} DCU2_DtlsMajorSession_s;
#endif

typedef struct
{
   sysTime_t serverCertificateExpirationSysTime;
   uint16_t dataSize;
   uint8_t data[DTLS_MAJOR_MAX_SIZE_WOLF36];       /* Ming: Temprory change to DTLS_MAJOR_MAX_SIZE size to make the buff size the same as wolfSSL36 */
} DtlsMajorSession_s;

#if 0
typedef struct
{
   uint16_t dataSize;
   uint8_t data[DTLS_MINOR_MAX_SIZE_WOLF36];
} DtlsMinorSession_wolf36_s;
#endif

typedef struct
{
   uint16_t dataSize;
   uint8_t data[DTLS_SESSION_MINOR_MAX_SIZE];
} DtlsMinorSession_s;

#if ( DCU == 1 )
typedef struct /* DCU2 ONLY */
{
   DtlsTime_t serverCertificateExpirationSysTime; /* declared to accomadate SRFN DCU patches, see comment in DtlsTime_t struct declaration */
   uint16_t dataSize;
   bool isAvailable;    /*Ming: When this flag is true, it indicates the FW was just upgraded from the wolfSSL 3.6 and the data is fresh. We
                                   shall recover the session from this structure instead of Major & Minor.
                                When this flag is false, it indicates the FW was rebootd from wolfSSL 4.x and the data contained is out
                                   dated.  We shall recover the session using Major & Minor. But there may be the case that the file is not
                                   initialized, so the flag is unpredictable. To avoid the case, determine if data is available by checking
                                   both isAvailable and dataSize == DTLS_WOLFSSL4x_SESSION_MAX_SIZE. If by any case, the dataSize and the
                                   flag is just the right value but the data actually is not. The import should be failed.
                                If failed, the code should try to recover from Major & Minor instead. */
   uint8_t data[DTLS_WOLFSSL4x_SESSION_MAX_SIZE];
} DCU2_DtlsWolfSSL4xSession_s;  /* Ming: Added for wolfSSL 3.6 to 4.x upgrade   */
#endif

typedef struct
{
   sysTime_t serverCertificateExpirationSysTime;
   uint16_t dataSize;
   bool isAvailable;    /*Ming: When this flag is true, it indicates the FW was just upgraded from the wolfSSL 3.6 and the data is fresh. We
                                   shall recover the session from this structure instead of Major & Minor.
                                When this flag is false, it indicates the FW was rebootd from wolfSSL 4.x and the data contained is out
                                   dated.  We shall recover the session using Major & Minor. But there may be the case that the file is not
                                   initialized, so the flag is unpredictable. To avoid the case, determine if data is available by checking
                                   both isAvailable and dataSize == DTLS_WOLFSSL4x_SESSION_MAX_SIZE. If by any case, the dataSize and the
                                   flag is just the right value but the data actually is not. The import should be failed.
                                If failed, the code should try to recover from Major & Minor instead. */
   uint8_t data[DTLS_WOLFSSL4x_SESSION_MAX_SIZE];
} DtlsWolfSSL4xSession_s;  /* Ming: Added for wolfSSL 3.6 to 4.x upgrade   */

/* To Do: Update module using file variables instead of current define values */
typedef struct
{
   uint32_t  maxAuthenticationTimeout;     /* The maximum delay following DTLS SSL_FATAL_ERROR */
   uint16_t  minAuthenticationTimeout;     /* The delay following an initial DTLS SSL_FATAL_ERROR */
   uint16_t  initialAuthenticationTimeout; /* The initial value for authenticationTimeout */
} DtlsConfigAttr_s;

typedef struct
{
   DTLS_counters_t   counters;                        /* These are the DTLS Statistics */
   uint32_t          dtlsLastSessionSuccessSysTime;   /* Used to track stale session. Moved here to prevent reboots from starting this over. */
} DtlsCachedAttr_s;

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
} DtlsFile_s;

/* DTLS command message structure */
typedef struct
{
   DtlsCommand_e command;                  /* DTLS command as described in eDtlsCommand_e */
   DtlsTransport_e transport;              /* Transport that created command if applicable */
   buffer_t* data;                         /* Message data */
} DtlsMessage_s;

/* CONSTANTS */

static const char * const cipherSuiteList = "ECDHE-ECDSA-AES256-CCM-8";  /* Cipher Suite encoding */

static const uint8_t _MeterKey[] =
{
   0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x21, 0xb0, 0x48, 0x37, 0x8b,
   0x45, 0x04, 0x19, 0xd8, 0x24, 0x1a, 0x34, 0xe2, 0xef, 0x7e, 0xaf, 0x24,
   0x61, 0x04, 0x19, 0x14, 0xc8, 0x57, 0x6f, 0xcb, 0x9c, 0x61, 0xdc, 0x32,
   0x74, 0xf8, 0x52, 0xa0, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
   0x03, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0x2e, 0xab, 0x11,
   0x83, 0xa7, 0x8b, 0xc6, 0x6e, 0xf5, 0x11, 0x9d, 0xcc, 0x5f, 0x8a, 0x50,
   0xf5, 0x64, 0xaf, 0x42, 0x19, 0xa3, 0x19, 0x5c, 0xe5, 0x49, 0x5b, 0x3b,
   0x1e, 0x32, 0xc3, 0xe6, 0x54, 0xdd, 0x08, 0x23, 0xff, 0x81, 0x94, 0x5f,
   0x6a, 0x73, 0x9f, 0xb7, 0x08, 0x1c, 0x8f, 0xe8, 0x81, 0x07, 0x38, 0x7b,
   0xe3, 0xb5, 0xc0, 0x57, 0x2f, 0x77, 0x56, 0xe3, 0xd7, 0xae, 0xc4, 0x48,
   0xad
};

static const uint32_t _MeterKeyLength = 121;

/* FILE VARIABLE DEFINITIONS */
static DTLS_IndicationHandler _indicationHandlerCallback[ IP_PORT_COUNT ]; /*lint !e641 converting enum to integral. */

static DTLS_SessionStateChangedCallback_t _sessionStateChangedCallback = NULL;
static DTLS_ConnectResultCallback_t _connectResultCallback = NULL;   /* Callback function for DTLS connection results */

static WOLFSSL_CTX         *_dtlsCtx = NULL;                         /* WolfSSL Context pointer */
static WOLFSSL             *_dtlsSsl = NULL;                         /* WolfSSL session pointer */
static uint32_t            _dtlsTaskTimeout = DTLS_TRANSPORT_TIMEOUT_RF; /* DTLS msg command timeout */
static DtlsTransport_e     _dtlsTransport = DTLS_TRANSPORT_NONE_e;   /* DTLS connection indicator */
static DtlsSessionState_e  _dtlsSessionState = DTLS_SESSION_NONE_e;  /* Session state */
static const buffer_t      *_dtlsTransportRxBuffer = NULL;           /* Transport receive buffer */
static const buffer_t      *_dtlsTransportTxBuffer = NULL;           /* Transport transmit buffer */
static bool                _dtlsRxTimeout = false;                   /* True: timeout during receive, false otherwise */
static uint32_t            _dtlsAuthenicationTimeoutMax = DTLS_INIT_AUTH_TIMEOUT_DEFAULT;
static DtlsConfigAttr_s    _dtlsConfigAttr = { 0 };                  /* DTLS Configuration Attributes */
static DtlsCachedAttr_s    _dtlsCachedAttr = { 0 };                  /* DTLS Cached Attributes (counters) */
static FileHandle_t        _dtlsCachedSessionMajorHndl = { 0 };      /* Major file handle (written rarely) */
static FileHandle_t        _dtlsCachedSessionMinorHndl = { 0 };      /* Minor file handle (written often) */
static FileHandle_t        _dtlsCachedSessionMinor_wolfSSL36 = { 0 };/* Ming: wolf4x minor is larger than wolf36. To make the file struct
                                                                        the same as wolf36, reserve the file at the original place. */
static OS_MSGQ_Obj         _dtlsMSGQ = { 0 };                        /* DTLS command queue */
static timer_t             _dtlsBackoffTimer = { 0 };                /* DTLS backoff timer started after failed
                                                                        connection */
static sysTime_t           _dtlsServerCertificateExpirationSysTime = { 0 };
static OS_EVNT_Obj         _dtlsEvent = { 0 };
static bool                _sessionCacheInitialized = false;
static uint8_t             _dtlsServerCertificateSerial[DTLS_SERVER_CERTIFICATE_SERIAL_MAX_SIZE] = { 0 };
static uint16_t            _dtlsServerCertificateSerialSize = 0;
static bool                _skipRandomNumber = false;
static OS_MUTEX_Obj        dtlsConfigMutex_;     /* Serialize access to DtlsConfig data-structure  */

/*  DTLS Configuration Data File */
static DtlsFile_s ConfigFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "DTLS_CONFIG",
   .FileId          = eFN_DTLS_CONFIG,             /* Configuration Data   */
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &_dtlsConfigAttr,
   .Size            = sizeof( _dtlsConfigAttr ),
   .UpdateFreq      = 0xFFFFFFFF                   /* Updated Seldom   */
}; /*lint !e785 too few initializers.  */

/*  DTLS Cached Data File ( DTLS related errors )  */
static DtlsFile_s CachedFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "DTLS_CACHED",
   .FileId          = eFN_DTLS_CACHED,             /* Cached Data   */
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &_dtlsCachedAttr,
   .Size            = sizeof( _dtlsCachedAttr ),
   .UpdateFreq      = 0                            /* Updated Often */
}; /*lint !e785 too few initializers.  */

static DtlsFile_s * const Files[2] =
{
   &ConfigFile,
   &CachedFile
};
#if ( EP == 1 )
typedef enum
{
   eSESSION_FILE_READ_UNKNOWN,      /* Initial state */
   eSESSION_FILE_READ_ATTEMPTING,   /* Process started for reading file - i.e. getting ctx/SSL info */
   eSESSION_FILE_READ_FAILED,       /* Read Failed */
   eSESSION_FILE_READ_UNABLE,       /* Unable to read file due to session info (ctx or SSL) */
   eSESSION_FILE_READ_SUCCESS       /* File read successfully */
}SessionFileReadStatus_e;

static SessionFileReadStatus_e sessionFilesRead = eSESSION_FILE_READ_UNKNOWN;
#endif

/* FUNCTION PROTOTYPES */
static void             DtlsInitConfigAttr( void );
static void             DtlsInitCachedAttr( bool bWrite );
static bool             DtlsU32CounterInc( uint32_t* pCounter, bool bRollover );

/* Callback routines for wolfssl */
static int32_t          DtlsRcvCallback( WOLFSSL *ssl, char *buf, int32_t sz, void *ctx );
static int32_t          DtlsSendCallback( WOLFSSL *ssl, char *buf, int32_t sz, void *ctx );
static int32_t          EccSignCallback( WOLFSSL* ssl, const uint8_t* in, uint32_t inSz, uint8_t* out, uint32_t* outSz, const uint8_t* keyDer,
                                          uint32_t keySz, void* ctx );
static void             *DtlsSslRealloc( void *ptr, size_t newSize );
static returnStatus_t   DtlsGetCertificate( uint8_t* cert, int32_t offset, uint32_t size );
static void             DtlsLogEvent( uint8_t priority, uint16_t eventId, const uint8_t *serialNumber, uint16_t serialNumberSz );
static returnStatus_t   DtlsConvertASN1SubjectToString( const uint8_t* subject, uint32_t subjectSize, char* outString, uint32_t outStringSize );
static char             *DtlsGetOidPrefix( const uint8_t* oid );
static int32_t          DtlsVerifyCallback( int32_t currentAuthStatus, WOLFSSL_X509_STORE_CTX *pX509 );

/* File local functions */
static bool             DtlsCanConnectRf( void );
static void             DtlsStartBackoff( void );
static void             DtlsBackoffTimerExpired( uint8_t cmd, uint8_t const *pData );
static WOLFSSL          *DtlsImportSessionCache_from_wolfSSL36( WOLFSSL_CTX *context );

static WOLFSSL_CTX      *DtlsRfCreateContext( void );
static WOLFSSL_CTX      *DtlsSerialContextCreate( void );
static int32_t          DtlsLoadRfCerts( WOLFSSL_CTX *context );
static returnStatus_t   DtlsSerialLoadCerts( WOLFSSL_CTX *context );

static WOLFSSL          *DtlsCreateSession( WOLFSSL_CTX *context, int32_t timeoutInit, int32_t timeoutMax );

static void             DtlsRfConnectNonBlocking( bool connectOnlyQuickSession );
static void             DtlsSerialConnect( void );

/* - Quicksession Related */
static returnStatus_t   DtlsInitializeSessionCache( void );
static WOLFSSL          *DtlsGetSessionCache( WOLFSSL_CTX *context );
static void             DtlsSaveSessionCache( WOLFSSL* ssl );
static returnStatus_t   DtlsUpdateSessionCache( WOLFSSL* ssl );
static void             DtlsInvalidateSessionCache( void );
static void             DtlsConnectResultCallback( returnStatus_t result );
static void             DtlsFree( void );
static returnStatus_t   DtlsSendCommand( DtlsCommand_e command );
static returnStatus_t   DtlsSendTransportCommand( DtlsCommand_e command, DtlsTransport_e transport, buffer_t *data );
static void             DtlsFreeCommand( buffer_t* buffer );
static void             DtlsConnectRf( bool connectOnlyQuickSession );
static void             DtlsConnectSerial( void );
static void             DtlsDisconnectSerial( void );
static void             DtlsDisconnectRf( bool quietShutdown );
static bool             DtlsTransportRx( buffer_t *pBuf );
static void             DtlsApplicationTx( const buffer_t *pBuf );
static void             DtlsApplicationRx( buffer_t *pBuf, buffer_t *decryptedStackBuffer );
static int32_t          DtlsTransportTx( const char *pBuf, uint32_t size );
static void             DtlsSessionStateChange( DtlsSessionState_e sessionState );
static void             DtlsSessionStateChangedCallback( DtlsSessionState_e sessionState );
static void             DtlsPrintMessage( const DtlsMessage_s* message );
static uint8_t          DtlsGetAppSecurityAuthMode( void );
static void             DtlsBlockOnAppSecurityAuthModeOff( void );
static void             DtlsUpdateLastSessionSuccessSysTime( void );
static void             DtlsCheckIfSessionIsStale( void );
static const char*      DtlsGetTransportString( DtlsTransport_e transport );
static void             DtlsConvertAsn1TimeStringToSysTime( const uint8_t* asn1Time, sysTime_t* sysTime );
static void             DtlsCheckIfServerCertificateHasExpired( void );

#if ( DTLS_DEBUG == 1 )
static void wolfSSL_LogMessage( const int32_t logLevel, const char *const logMessage );
static const char *ENTER_FUNC = "Enter ";
static const char *EXIT_FUNC  = "Exit ";
#define DTLS_INFO( fmt, ... ) \
{ \
   INFO_printf( fmt, ##__VA_ARGS__ ); \
   OS_TASK_Sleep( 10 ); \
}
//#define DTLS_DBG_CommandLine_Buffers() ( void )DBG_CommandLine_Buffers( 0, NULL )
#define DTLS_DBG_CommandLine_Buffers() ( void )0
#else
#define DTLS_INFO( fmt, ... ) ( void )0
#define DTLS_DBG_CommandLine_Buffers() ( void )0
#endif

#define DTLS_ERROR( fmt, ... ) \
{ \
   ERR_printf( fmt, ##__VA_ARGS__ ); \
}
/***********************************************************************************************************************

   Function name: DTLS_init

   Purpose: Initialize this module before tasks start running in the system

   Arguments: None

   Returns: eSUCCESS or eFAILURE

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
returnStatus_t DTLS_init( void )
{
   returnStatus_t ret = eFAILURE; /* Return Value */

   if (  ( !OS_MSGQ_Create( &_dtlsMSGQ, DTLS_NUM_MSGQ_ITEMS, "DTLS" ) )         || ( !OS_EVNT_Create ( &_dtlsEvent ) ) ||
         ( !OS_MUTEX_Create( &dtlsConfigMutex_ ) ) || ( eSUCCESS != DtlsInitializeSessionCache() ) )
   {
      return eFAILURE;
   }

   DtlsInitConfigAttr();                  /* Reset the configuration attributes to default values */
   DtlsInitCachedAttr( ( bool ) false );  /* Clear the counter values */

   /* Normal Mode */
   uint8_t i;
   FileStatus_t fileStatus;

   for ( i = 0; i < ( uint8_t )IP_PORT_COUNT; i++ )
   {
      _indicationHandlerCallback[i] = NULL;
   }

   for ( i = 0; i < ( sizeof( Files ) / sizeof( *( Files ) ) ); i++ )
   {
      DtlsFile_s *pFile = Files[i];

      if ( eSUCCESS == FIO_fopen( &pFile->handle,              /* File Handle so that PHY access the file. */
                                  pFile->ePartition,           /* Search for best partition according to the timing. */
                                  ( uint16_t ) pFile->FileId,  /* File ID (filename) */
                                  pFile->Size,                 /* Size of the data in the file. */
                                  pFile->Attr,                 /* File attributes to use. */
                                  pFile->UpdateFreq,           /* The update rate of the data in the file. */
                                  &fileStatus ) )              /* Contains the file status */
      {
         if ( fileStatus.bFileCreated )
         {
            /* The file was just created for the first time.  Save the default values to the file */
            ret = FIO_fwrite( &pFile->handle, 0, pFile->Data, pFile->Size );
         }
         else
         {
            /* Read the STACK Cached File Data */
            ret = FIO_fread( &pFile->handle, pFile->Data, 0, pFile->Size );
         }
      }
      if ( eFAILURE == ret )
      {
         break;
      }
   }

   if ( ret == eSUCCESS )
   {
      NWK_RegisterIndicationHandler( ( uint8_t )UDP_DTLS_PORT, DTLS_RfTransportIndication );
#if ( USE_IPTUNNEL == 1 )
      NWK_RegisterIndicationHandler( ( uint8_t )UDP_IP_DTLS_PORT, DTLS_RfTransportIndication );
#endif
   }
   ( void ) wolfSSL_SetAllocators( ( wolfSSL_Malloc_cb )BM_SpecLibMalloc,
                                   ( wolfSSL_Free_cb )BM_SpecLibFree,
                                   ( wolfSSL_Realloc_cb )DtlsSslRealloc );

   return ret;
}
/******************************************************************************************************************** */
/*lint -esym(715,Arg0) */
/***********************************************************************************************************************
   Function Name: DTLS_Task

   Purpose: Task for the DTLS networking layer (combination of IP/UDP handling) Initialize the DTLS library and setup
   a context.  We won't try to connect until the task starts up.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: none
***********************************************************************************************************************/
void DTLS_Task( taskParameter )
{
   buffer_t       *commandBuf;   /* DTLS command buffer */
   DtlsMessage_s  *msg;          /* DTLS command message data */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DtlsBlockOnAppSecurityAuthModeOff();

#if ( DTLS_DEBUG == 1 )
// DBG_SetTaskFilter( _task_get_id() );         /* Only allow DTLS debug print!  */
   ( void )wolfSSL_SetLoggingCb( wolfSSL_LogMessage );
   ( void )wolfSSL_Debugging_ON();
#endif

   int32_t sslStat = wolfSSL_Init();
   if ( sslStat != SSL_SUCCESS )
   {
      DTLS_ERROR( "wolfSSL_Init failed: %d", sslStat );
   }

   for ( ;; )
   {
#if 0 /* use to debug serial connection time out only */
      if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
      {
         _dtlsTaskTimeout = 20000;
      }
#endif
      if ( OS_MSGQ_Pend( &_dtlsMSGQ, ( void * )&commandBuf, _dtlsTaskTimeout ) )
      {
         DTLS_INFO( "%s dequeued 0x%p", __func__, commandBuf->data );
         _dtlsRxTimeout = false;
         msg = ( DtlsMessage_s * )( void * )commandBuf->data;
         DtlsPrintMessage( msg );
         DTLS_DBG_CommandLine_Buffers();

         switch ( msg->command )
         {
            case DTLS_CMD_CONNECT_SERIAL_e:           /* Secure serial connection being requested  */
               DtlsDisconnectRf( QUIET_SHUTDOWN );
               DtlsConnectSerial();
               break;

            case DTLS_CMD_DISCONNECT_SERIAL_e:        /* Secure serial session ending  */
               DtlsDisconnectSerial();
               DtlsConnectRf( FALSE );
               break;

            case DTLS_CMD_CONNECT_RF_e:               /* Either backoff timer expired, or appsecurityauthmode just turned ON */
               DtlsConnectRf( FALSE );
               break;

            case DTLS_CMD_DISCONNECT_RF_e:            /* TODO: this command is NEVER used.   */
               DtlsDisconnectRf( NORMAL_SHUTDOWN );
               break;

            case DTLS_CMD_RECONNECT_RF_e:             /* Certificate expired, or stale session, or system shutting down (reboot) */
               DtlsDisconnectSerial();
               DtlsDisconnectRf( NORMAL_SHUTDOWN );   /* Handle case where handshake was in progress. */
               DtlsConnectRf( TRUE );                 /* Load quick session if there was one.   */
               DtlsDisconnectRf( NORMAL_SHUTDOWN );   /* Disconnect quick session.  */
               DtlsInvalidateSessionCache();
               DtlsConnectRf( FALSE );
               break;

            case DTLS_CMD_TRANSPORT_RX_e:
               /* If a message is received for serial and we are in rf mode ignore it and vice versa. */
               if ( _dtlsTransport == msg->transport && !DtlsTransportRx( ( buffer_t * )msg->data ) )
               {
                  msg->data = NULL;
               }
               break;

            case DTLS_CMD_APPDATA_TX_e:
               DtlsApplicationTx( ( buffer_t * )msg->data );
               break;

            case DTLS_CMD_APP_SECURITY_MODE_CHANGED_e:   /* This is only invoked when turning DTLS OFF! If turning ON, task is waiting in
                                                            DtlsBlockOnAppSecurityAuthModeOff()   */
               DtlsDisconnectSerial();
               DtlsDisconnectRf( NORMAL_SHUTDOWN );      /* If turning OFF, notify HE. */
               DtlsInvalidateSessionCache();
               DtlsBlockOnAppSecurityAuthModeOff();      /* Wait until mode enabled, then start an RF connection. This leaves the commandBuf buffer
                                                            allocated until App security is reenabled. */
               break;

            case DTLS_CMD_SHUTDOWN_e:
            {
               DtlsEvents_e event = DTLS_EVENT_SHUTDOWN_COMPLETE_e;
               DtlsDisconnectSerial();
               OS_EVNT_Set( &_dtlsEvent, ( uint32_t )event );
               OS_TASK_Sleep( OS_WAIT_FOREVER );
               break;
            }

            default:
               break;
         }
         DTLS_INFO( "%s free  = 0x%p", __func__, commandBuf->data );
         DtlsFreeCommand( commandBuf );
      }
      else
      {
         /* Transport timeout occured, depending on state we need to get wolfssl to retransmit. */
         _dtlsRxTimeout = true;
         ( void )DtlsTransportRx( NULL );
      }
   }
}
/*lint +esym(715,Arg0) */
/***********************************************************************************************************************

   Function name: DTLS_DataRequest

   Purpose: This function is called by stack to send data to the DTLS task.

   Arguments:
         uint8_t qos               - Quality of service
         uint16_t size             - Size of the data
         uint8_t  const data[]     - Data
         NWK_Address_t const *dst_addr - Destination address
         NWK_Override_t const *override - Override settings
         MAC_dataConfCallback callback - Mac callback function
         NWK_ConfirmHandler confirm_cb * - Pointre to the confirm handler.

   Returns:
         uint32_t                  - handle or 0 for error

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
uint32_t DTLS_DataRequest( uint8_t port, uint8_t qos, uint16_t size, uint8_t  const data[], NWK_Address_t const *dst_addr,
                           NWK_Override_t const *override, MAC_dataConfCallback callback, NWK_ConfirmHandler confirm_cb )
{

   uint16_t allocSize;

   /* Account for the NWK_Request Header including pad, then include the NWK Data request */
   allocSize  = ( sizeof( NWK_Request_t ) - sizeof( NWK_Services_u ) ) + sizeof( NWK_DataReq_t );
   allocSize += size;            /* Account for data space */
   buffer_t *pBuf = BM_allocStack( allocSize );

   if ( pBuf != NULL )
   {
      ( void )memset( pBuf->data, 0, pBuf->bufMaxSize );
      pBuf->eSysFmt               = eSYSFMT_NWK_REQUEST;
      NWK_Request_t *pReq         = ( NWK_Request_t * )pBuf->data; /*lint !e740 !e826 */
      pReq->MsgType               = eNWK_DATA_REQ;
      pReq->handleId              = 300;                     /* TODO: HandleId use is currently broken/undefined  */
      pReq->pConfirmHandler       = confirm_cb;
      pReq->Service.DataReq.callback      = callback;
      pReq->Service.DataReq.qos           = qos;
      pReq->Service.DataReq.payloadLength = size;
      pReq->Service.DataReq.port          = port;
      pReq->Service.DataReq.override      = *override;
      ( void ) memcpy( pReq->Service.DataReq.data, data, size );

      /* Populate destination address fields */
      if ( dst_addr->addr_type == eEXTENSION_ID )
      {
         pReq->Service.DataReq.dstAddress.addr_type = eEXTENSION_ID;
         ( void )memcpy( pReq->Service.DataReq.dstAddress.ExtensionAddr, dst_addr->ExtensionAddr, MAC_ADDRESS_SIZE );
      }
      else if ( dst_addr->addr_type == eCONTEXT )
      {
         pReq->Service.DataReq.dstAddress.addr_type = eCONTEXT;
         pReq->Service.DataReq.dstAddress.context = dst_addr->context;
      }

      DTLS_RfTransportIndication( pBuf );

      return pReq->handleId;
   }
   else
   {
      return 0;
   }
}
/***********************************************************************************************************************
   Function Name: DTLS_RegisterIndicationHandler

   Purpose: This function is called by an upper layer module to
            register a callback function.  That function will accept an
            indication (data.confirmation, data.indication, etc) from this
            module.  That function is also responsible for freeing the
            indication buffer.

   Arguments: pointer to a funciton of type DTLS_RegisterIndicationHandler

   Returns: none

   Note: Currently only one upper layer can receive indications but
         the function could be expanded to have an list of callbacks if
         needed.
 **********************************************************************************************************************/
void DTLS_RegisterIndicationHandler( uint8_t port, DTLS_IndicationHandler pCallback )
{
   _indicationHandlerCallback[port] = pCallback;
}
/***********************************************************************************************************************
   Function Name: DTLS_RegisterSessionStateChangedCallback

   Purpose: This function is called to register a callback function. The callback function
            is called when there is a state change to the DTLS.

   Arguments:
            sessionStateChangedCallback      - Callback function to register.

   Returns: none

   Note:
 **********************************************************************************************************************/
void DTLS_RegisterSessionStateChangedCallback( DTLS_SessionStateChangedCallback_t sessionStateChangedCallback )
{
   _sessionStateChangedCallback = sessionStateChangedCallback;
}
#if ( DTLS_DEBUG == 1 )
static void DTLS_dumpState( const char *title, WOLFSSL *ssl )
{
   DBG_printf( "\n%s\n"
               "peerSeqHi     %4u\n"
               "peerSeqLo     %4u\n"
               "seqHi         %4u\n"
               "seqLo         %4u\n"
               "nextEpoch     %4hu\n"
               "nextSeqHi     %4hu\n"
               "nextSeqLo     %4u\n"
               "curEpoch      %4hu\n"
               "curSeqHi      %4hu\n"
               "curSeqLo      %4u\n"
               "prevSeqHi     %4hu\n"
               "prevSeqLo     %4u\n"
               "peerHshake    %4hu\n"
               "expPerHshake  %4hu\n"
               "dtlsSeqHi     %4hu\n"
               "dtlsSeqLo     %4u\n"
               "dtlsPrevSeqHi %4hu\n"
               "dtlsPrevSeqLo %4u\n"
               "dtlsPpoch     %4hu\n"
               "dtlsHshake_no %4hu\n"
               "encryptSz     %4u\n"
               "padSz         %4u\n"
               "encryptionOn  %4hhu\n"
               "decryptedCur  %4hhu\n",
               title,
               ssl->keys.peer_sequence_number_hi,
               ssl->keys.peer_sequence_number_lo,
               ssl->keys.sequence_number_hi,
               ssl->keys.sequence_number_lo,
               ssl->keys.peerSeq[0].nextEpoch,
               ssl->keys.peerSeq[0].nextSeq_hi,
               ssl->keys.peerSeq[0].nextSeq_lo,
               ssl->keys.curEpoch,
               ssl->keys.curSeq_hi,
               ssl->keys.curSeq_lo,
               ssl->keys.peerSeq[0].prevSeq_hi,
               ssl->keys.peerSeq[0].prevSeq_lo,
               ssl->keys.dtls_peer_handshake_number,
               ssl->keys.dtls_expected_peer_handshake_number,
               ssl->keys.dtls_sequence_number_hi,
               ssl->keys.dtls_sequence_number_lo,
               ssl->keys.dtls_prev_sequence_number_hi,
               ssl->keys.dtls_prev_sequence_number_lo,
               ssl->keys.dtls_epoch,
               ssl->keys.dtls_handshake_number,
               ssl->keys.encryptSz,
               ssl->keys.padSz,
               ssl->keys.encryptionOn,
               ssl->keys.decryptedCur );
}
#endif
/***********************************************************************************************************************

   Function Name: DtlsFree

   Purpose: This function is called to free all the resources associated with a DTLS context and session.

   Arguments: none

   Returns: none

 **********************************************************************************************************************/
static void DtlsFree()
{
   if ( _dtlsSsl != NULL )
   {
      ( void )wolfSSL_shutdown( _dtlsSsl );
      wolfSSL_free( _dtlsSsl );
      _dtlsSsl = NULL;
   }

   if ( _dtlsCtx != NULL )
   {
      wolfSSL_CTX_free( _dtlsCtx );
      _dtlsCtx = NULL;
   }

   ( void )wolfSSL_Cleanup();
}
/***********************************************************************************************************************

   Function name: DtlsSendCommand

   Purpose: This function is called to send a message to the DTLS task.

   Arguments:
         eDtlsCommand_e command    - DTLS Command

   Returns: eSUCCESS or eFAILURE

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t DtlsSendCommand( DtlsCommand_e command )
{
   return DtlsSendTransportCommand( command, DTLS_TRANSPORT_NONE_e, NULL );
}
/***********************************************************************************************************************

   Function name: DtlsSendTransportCommand

   Purpose: This function is called to send a transport message to the DTLS task.

   Arguments:
         eDtlsCommand_e command    - DTLS Command
         DtlsTransport_e transport - Transport that generated the message if applicable.
         buffer_t *data            - Data to pass to the DTLS task.

   Returns: eSUCCESS or eFAILURE

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t DtlsSendTransportCommand( DtlsCommand_e command, DtlsTransport_e transport, buffer_t *data )
{
   returnStatus_t ret = eSUCCESS;            /* Return status */
   uint16_t size = sizeof( DtlsMessage_s );  /* Size of the command message */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   buffer_t* pBuffer = BM_allocStack( size );     /* Buffer allocated for the command */
   DTLS_INFO( "%s got  = 0x%p", __func__, pBuffer->data );

   if ( pBuffer != NULL )
   {
      DtlsMessage_s* pDtlsMessage = ( DtlsMessage_s* )( void * )pBuffer->data;
      pDtlsMessage->command = command;
      pDtlsMessage->transport = transport;
      pDtlsMessage->data = data;

      OS_MSGQ_Post( &_dtlsMSGQ, ( void * )pBuffer ); /* Function will not return if it fails */
   }
   else
   {
      DTLS_ERROR( "DtlsSendCommand failed to allocate a command buffer for CMD: %d", ( int32_t )command );
      if ( data != NULL )
      {
         DTLS_INFO( "%s free  = 0x%p", __func__, pBuffer->data );  /*lint !e413 NULL ptr OK in print statemnt   */
         BM_free( data );     /* Be sure to free the buffer passed in, if not sent on! */
      }
      ret = eFAILURE;
   }

   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return ret;
}
/***********************************************************************************************************************

   Function name: DtlsFreeCommand

   Purpose: This function is called to free resources associated with the DTLS command

   Arguments:
         buffer_t *buffer          - Dtls command buffer

   Returns: eSUCCESS or eFAILURE

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsFreeCommand( buffer_t* pBuffer )
{
   DtlsMessage_s* pDtlsMessage = ( DtlsMessage_s* )( void * )pBuffer->data; /* Address of buffer to free */

   if ( pDtlsMessage->data != NULL )
   {
      BM_free( pDtlsMessage->data );
      pDtlsMessage->data = NULL;
   }

   BM_free( pBuffer );
}
/**********************************************************************************************************************
   SERIAL APIS
 *********************************************************************************************************************/
/***********************************************************************************************************************

   Function name: DTLS_UartConnect

   Purpose: This function is called to initiate a serial connection

   Arguments:
         DTLS_ConnectResultCallback_t pResultCallback - Function to call with DTLS connect status.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
returnStatus_t DTLS_UartConnect( DTLS_ConnectResultCallback_t pResultCallback )
{
   returnStatus_t ret;                           /* Return status */

   ret = DtlsSendCommand( DTLS_CMD_CONNECT_SERIAL_e );
   if ( ret == eSUCCESS )
   {
      _connectResultCallback = pResultCallback;
   }
   return ret;
}
/***********************************************************************************************************************

   Function name: DTLS_UartDisconnect

   Purpose: This function is called to disconnect the serial DTLS connection

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void DTLS_UartDisconnect( void )
{
   ( void )DtlsSendCommand( DTLS_CMD_DISCONNECT_SERIAL_e );
}
/***********************************************************************************************************************

   Function name: DtlsCanConnectRf

   Purpose: This function is called to verify if there is a valid system time and RF connection can proceed.

   Arguments: None

   Returns: TRUE if connection over RF should continue, FALSE otherwise.

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static bool DtlsCanConnectRf( void )
{
   bool           returnVal = ( bool )true;  /* Assume time is valid, and OK to initiate RF connection.  */
   buffer_t       *commandBuf;               /* Use to get message while time is invalid.                */
   DtlsMessage_s  *msg;                      /* Dequeued DtlsMessage_s structure                         */

   /* Exit loop once we have valid time OR receive a request to connect to the serial port.  */
   while ( !TIME_SYS_IsTimeValid() )
   {
      if ( OS_MSGQ_Pend( &_dtlsMSGQ, ( void * )&commandBuf, DTLS_TIME_CHECK_DELAY ) )
      {
         msg = ( DtlsMessage_s * )( void * )commandBuf->data;

         /* Check and see if this is a connect serial message, if so send it to ourselves. */
         if ( msg->command == DTLS_CMD_CONNECT_SERIAL_e )
         {
            if ( eSUCCESS == DtlsSendCommand( DTLS_CMD_CONNECT_SERIAL_e ) )
            {
               returnVal = ( bool )false;
            }
         }

         DTLS_INFO( "%s free  = 0x%p", __func__, commandBuf->data );
         DtlsFreeCommand( commandBuf );   /* Free the buffer with the request!         */
         if ( !returnVal )                /* if returnVal changed, need to exit loop   */
         {
            break;
         }
      }
   }

   return returnVal;
}
/***********************************************************************************************************************

   Function name: DtlsBackoffTimerExpired

   Purpose: This function is called when the backoff timer has expired to attempt another RF connection.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsBackoffTimerExpired( uint8_t cmd, uint8_t const *pData )
{
   ( void ) cmd;
   ( void ) pData;
   DTLS_INFO( "DtlsBackoffTimerExpired" );
   ( void )memset( &_dtlsBackoffTimer, 0, sizeof( _dtlsBackoffTimer ) ); /* Clear the timer values */
   ( void )DtlsSendCommand( DTLS_CMD_CONNECT_RF_e );
}
/***********************************************************************************************************************

   Function name: DtlsStartBackoff

   Purpose: This function is called when the backoff timer has expired to attempt another RF connection.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsStartBackoff( void )
{
   DTLS_INFO( "DtlsStartBackoff" );

   DtlsDisconnectRf( NORMAL_SHUTDOWN );
   DtlsInvalidateSessionCache();

   _dtlsSessionState = DTLS_SESSION_FAILED_e;

   uint32_t dtlsAuthenticationTimeout = ( uint32_t )aclara_randu( DTLS_getMinAuthenticationTimeout(), _dtlsAuthenicationTimeoutMax );
   _dtlsTaskTimeout = OS_WAIT_FOREVER;

   ( void )memset( &_dtlsBackoffTimer, 0, sizeof( _dtlsBackoffTimer ) ); /* Clear the timer values */
   _dtlsBackoffTimer.bOneShot = ( bool )true;
   _dtlsBackoffTimer.bOneShotDelete = ( bool )true;
   _dtlsBackoffTimer.bExpired = ( bool )false;
   _dtlsBackoffTimer.bStopped = ( bool )false;
   _dtlsBackoffTimer.ulDuration_mS  = dtlsAuthenticationTimeout * 1000;
   _dtlsBackoffTimer.pFunctCallBack = ( vFPtr_u8_pv )DtlsBackoffTimerExpired;

   if ( eSUCCESS == TMR_AddTimer( &_dtlsBackoffTimer ) )
   {
      DTLS_INFO( "Started back off timer with duration %u ms.",  dtlsAuthenticationTimeout );
   }
   else
   {
      DTLS_ERROR( "Unable to start backoff timer." );
   }

   _dtlsAuthenicationTimeoutMax *= 2;

   if ( _dtlsAuthenicationTimeoutMax > DTLS_getMaxAuthenticationTimeout() )
   {
      _dtlsAuthenicationTimeoutMax = DTLS_getMaxAuthenticationTimeout();
   }
}
/***********************************************************************************************************************

   Function name: DtlsConnectSerial

   Purpose: This function is called to start the "unblocked" DTLS seraial connection

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsConnectSerial( void )
{
   DTLS_INFO( "DtlsConnectSerial" );

   _dtlsCtx = DtlsSerialContextCreate();
   if ( NULL != _dtlsCtx )
   {
      _dtlsTransport = DTLS_TRANSPORT_SERIAL_e;
      _dtlsTaskTimeout = DTLS_TRANSPORT_TIMEOUT_SERIAL;
      _dtlsSessionState = DTLS_SESSION_CONNECTING_e;
      DtlsSerialConnect();
   }
}
/***********************************************************************************************************************
   Function Name: DtlsDisconnectSerial

   Purpose: This function is called to disconnect the DTLS serial connection.

   Arguments: None

   Returns: none
***********************************************************************************************************************/
static void DtlsDisconnectSerial( void )
{
   DTLS_INFO( "DtlsDisconnectSerial" );

   if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
   {
      DtlsFree();

      MFGP_DtlsConnectionClosed();

      _dtlsTransport = DTLS_TRANSPORT_NONE_e;
      _dtlsSessionState = DTLS_SESSION_NONE_e;
   }
}
/**********************************************************************************************************************
   RF APIS
 *********************************************************************************************************************/
/***********************************************************************************************************************
   Function Name: DtlsConnectRf

   Purpose: This function is called to connect an RF DTLS session.

   Arguments:
         connectOnlyQuickSession - True if only cached sessions should be loaded, false otherwise.

   Returns: None
***********************************************************************************************************************/
static void DtlsConnectRf( bool connectOnlyQuickSession )
{
   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   if ( DtlsCanConnectRf() )
   {
#if ( EP == 1 )
      sessionFilesRead = eSESSION_FILE_READ_ATTEMPTING;
#endif
      _dtlsCtx = DtlsRfCreateContext();
      DTLS_INFO( "%s _dtlsCtx: 0x%p", __func__, _dtlsCtx );
      if ( NULL != _dtlsCtx )
      {
         _dtlsTransport    = DTLS_TRANSPORT_RF_e;
         _dtlsTaskTimeout  = DTLS_TRANSPORT_TIMEOUT_RF;
         _dtlsSessionState = DTLS_SESSION_CONNECTING_e;
         DtlsRfConnectNonBlocking( connectOnlyQuickSession );
      }
      else
      {
#if ( EP == 1 )
         /* Unable to create context, likely due to certs not loaded, keep trying? */
         sessionFilesRead = eSESSION_FILE_READ_UNABLE;
#endif
         DtlsStartBackoff();
      }
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
}
/***********************************************************************************************************************
   Function Name: DtlsDisconnectRf

   Purpose: This function is called to Disconnect the RF session

   Arguments: bool quietShutdown - true -> quiet shutdown

   Returns: none
***********************************************************************************************************************/
static void DtlsDisconnectRf( bool quietShutdown )
{
   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
   {
      if ( _dtlsBackoffTimer.usiTimerId != 0 )
      {
         if ( eSUCCESS != TMR_DeleteTimer( _dtlsBackoffTimer.usiTimerId ) )
         {
            DTLS_ERROR( "TMR_DeleteTimer failed." );
         }

         ( void )memset( &_dtlsBackoffTimer, 0, sizeof( _dtlsBackoffTimer ) );
      }

      if ( quietShutdown
            && _dtlsSsl != NULL )
      {
         wolfSSL_set_quiet_shutdown( _dtlsSsl, 1 );
      }

      DtlsFree();

      _dtlsTransport = DTLS_TRANSPORT_NONE_e;
      _dtlsSessionState = DTLS_SESSION_NONE_e;
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
}
/***********************************************************************************************************************
   Function Name: DtlsLoadRfCerts

   Purpose: This function is called to load the RF certs during a RF DTLS connect.

   Arguments:
         WOLFSSL_CTX *ctx          - Wolfssl context.

   Returns: none
 **********************************************************************************************************************/
static int32_t DtlsLoadRfCerts( WOLFSSL_CTX *ctx )
{
   int32_t ret;                                  /* Return status */
   uint8_t  eccRes;                              /* Ecc return status */
   uint32_t certLen = 0;                         /* Certificate length */
   uint8_t  certBuff[DTLS_MAX_CERT_SIZE];        /* Certificate buffer */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   ( void )memset( certBuff, 0, DTLS_MAX_CERT_SIZE );

   ret = WOLFSSL_BAD_FILE;
   eccRes = ecc108e_GetDeviceCert( DeviceCert_e, certBuff, &certLen );
   DTLS_INFO( "Got DeviceCert ret = %u", eccRes );
   if ( ECC108_SUCCESS == eccRes )
   {
      ret = wolfSSL_CTX_use_certificate_buffer( ctx, certBuff, ( int32_t )certLen, SSL_FILETYPE_ASN1 );
      if ( SSL_SUCCESS == ret )
      {
         DTLS_INFO( "Got DeviceCert. Len = %u", certLen );

         ret = WOLFSSL_BAD_FILE;
         eccRes = ecc108e_GetDeviceCert( dtlsNetworkRootCA_e, certBuff, &certLen );
         DTLS_INFO( "Got NetworkRootCA ret = %u", eccRes );
         if ( ECC108_SUCCESS == eccRes )
         {
            DTLS_INFO( "Got NetworkRootCA. Len = %u", certLen );

            ret = wolfSSL_CTX_load_verify_buffer( ctx, certBuff, ( int32_t )certLen, SSL_FILETYPE_ASN1 );
            if ( SSL_SUCCESS == ret )
            {
               ret = wolfSSL_CTX_use_PrivateKey_buffer( ctx, _MeterKey, ( int32_t )_MeterKeyLength, SSL_FILETYPE_ASN1 );
               if ( SSL_SUCCESS != ret )
               {
                  DTLS_ERROR( "wolfSSL_CTX_use_PrivateKey_buffer failed" );
               }
            }
            else
            {
               DTLS_ERROR( "wolfSSL_CTX_load_verify_buffer failed" );
            }
         }
         else
         {
            DTLS_ERROR( "Get dtlsNetworkRootCA failed" );
         }
      }
      else
      {
         DTLS_ERROR( "wolfSSL_CTX_use_certificate_chain_buffer failed" );
      }
   }
   else
   {
      DTLS_ERROR( "Get DeviceCert failed" );
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return ret;
}
/***********************************************************************************************************************
   Function Name: DtlsRfCreateContext

   Purpose: This function is called to create and initialize a WOLFSSL_CTX structure for use with the radio link.

   Arguments:
         None

   Returns: Pointer to the initailized context or NULL

 **********************************************************************************************************************/
static WOLFSSL_CTX *DtlsRfCreateContext( void )
{
   WOLFSSL_CTX *ctx = NULL;                      /* WolfSSL context pointer */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   SetSkipValidateDate( 0 );

   WOLFSSL_METHOD *dtlsClientMethod = wolfDTLSv1_2_client_method();
   DTLS_INFO( "%s, wolfDTLSv1_2_client_method returned 0x%p",  __func__, dtlsClientMethod );
   if ( dtlsClientMethod != NULL )
   {
      ctx = wolfSSL_CTX_new( dtlsClientMethod );
      DTLS_INFO( "%s, wolfSSL_CTX_new returned 0x%p",  __func__, ctx );
      if ( ctx != NULL )
      {
#if ( DTLS_GROUP_MESSAGES == 1 )
         ( void )wolfSSL_CTX_set_group_messages( _dtlsCtx );
#endif
         ( void )wolfSSL_CTX_set_timeout( ctx, ( DTLS_SESSION_TIMEOUT_RF / DTLS_MILLISECONDS_IN_SECOND ) );
         wolfSSL_SetIORecv( ctx, DtlsRcvCallback );
         wolfSSL_SetIOSend( ctx, DtlsSendCallback );

         if ( SSL_SUCCESS == DtlsLoadRfCerts( ctx ) )
         {
            DTLS_INFO( "Certificates loaded" );
            wolfSSL_CTX_SetEccSignCb( ctx, EccSignCallback );
         }
         else
         {
            wolfSSL_CTX_free( ctx );
            ctx = NULL;
         }
      }
      else
      {
         DTLS_ERROR( "wolfSSL_CTX_new failed" );
      }
   }
   else
   {
      DTLS_ERROR( "CyaDTLSv1_2_client_method failed" );
   }
   DTLS_INFO( "%s%s return: 0x%p", EXIT_FUNC, __func__, ctx );
   DTLS_DBG_CommandLine_Buffers();
   return ctx;
}
/*lint -e{774} */
/***********************************************************************************************************************
   Function Name: DtlsRfConnectNonBlocking

   Purpose: This function is called to start kickoff the DTLS non blocking connection.

   Arguments:
         connectOnlyQuickSession - true if only cached session should be loaded, false otherwise.

   Returns: None

 **********************************************************************************************************************/
static void DtlsRfConnectNonBlocking( bool connectOnlyQuickSession )
{
   int32_t sslStat;  /* SSL return status */
   WOLFSSL* ssl;     /* Session pointer */
   int32_t sslError; /* WolfSSL return status */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   ssl = DtlsGetSessionCache( _dtlsCtx );

   if ( NULL != ssl )
   {
      DTLS_INFO( "Cached Session restored" );
      _dtlsSsl = ssl;
      wolfSSL_set_using_nonblock( _dtlsSsl, 1 ); /* non-blocking */
      DTLS_INFO( "Session restored after wolfSSL_set_using_nonblock" );
      DTLS_DBG_CommandLine_Buffers();
      ( void )wolfSSL_set_timeout( _dtlsSsl, ( DTLS_SESSION_TIMEOUT_RF / DTLS_MILLISECONDS_IN_SECOND ) );
      _dtlsSessionState = DTLS_SESSION_CONNECTED_e;
      _dtlsTaskTimeout = DTLS_SESSION_TIMEOUT_RF;
      DtlsSessionStateChangedCallback( DTLS_SESSION_CONNECTED_e );
   }
#if ( EP == 1 )

   else if ( PWRLG_LastGasp() == 1 )
   {
      OS_TASK_Sleep( OS_WAIT_FOREVER );
   }

#endif
   else if ( !connectOnlyQuickSession )
   {
      DTLS_INFO( "Creating New Session" );
      ssl = DtlsCreateSession( _dtlsCtx, DTLS_TRANSPORT_TIMEOUT_RF / DTLS_MILLISECONDS_IN_SECOND,
                               ( DTLS_TRANSPORT_TIMEOUT_RF / DTLS_MILLISECONDS_IN_SECOND ) * DTLS_MAX_RX_COUNT );
      DTLS_INFO( "New session after DtlsCreateSession" );
      DTLS_DBG_CommandLine_Buffers();

      if ( NULL != ssl )
      {
         _dtlsSsl = ssl;

         ( void )wolfSSL_set_timeout( _dtlsSsl, ( DTLS_SESSION_TIMEOUT_RF / DTLS_MILLISECONDS_IN_SECOND ) );

         wolfSSL_SetEccVerifyCtx( _dtlsSsl, _dtlsCtx );

         wolfSSL_set_verify( _dtlsSsl, SSL_VERIFY_PEER, DtlsVerifyCallback );
         wolfSSL_set_using_nonblock( _dtlsSsl, 1 ); /* Non blocking */

         sslStat = wolfSSL_connect( _dtlsSsl );
         sslError = wolfSSL_get_error( _dtlsSsl, 0 );
         DTLS_INFO( "New Session after wolfSSL_connect" );
         DTLS_DBG_CommandLine_Buffers();
         if (  ( sslStat != SSL_SUCCESS ) &&
               ( ( sslError == SSL_ERROR_WANT_READ ) || ( sslError == SSL_ERROR_WANT_WRITE ) ) )
         {
            DTLS_INFO( "DTLS RF connection starting" );
         }
         else
         {
            DTLS_ERROR( "wolfSSL connect failed %d", sslError );
         }
      }
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return;
}
/***********************************************************************************************************************
   Function Name: DtlsCreateSession

   Purpose: This function is called to create and initalize the WOLFSSL object that will represent the connection.

   Arguments:
         WOLFSSL_CTX *context      - Pointer to an initialize context structure.

   Returns: Pointer to the initialize SSL object or NULL

 **********************************************************************************************************************/
static WOLFSSL *DtlsCreateSession( WOLFSSL_CTX *context, int32_t timeoutInit, int32_t timeoutMax )
{
   WOLFSSL *ssl = NULL;                          /* WolfSSL session handle */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   if ( NULL != context )
   {
      ssl = wolfSSL_new( context );
      if ( NULL != ssl )
      {
         ( void )wolfSSL_set_cipher_list( ssl, cipherSuiteList );
         ssl->dtls_expected_rx = DTLS_EXPECTED_MAX_RX;   /* so we don't need to allocate a larger buffer */

         ( void )wolfSSL_dtls_set_timeout_max( ssl, timeoutMax );
         ( void )wolfSSL_dtls_set_timeout_init( ssl, timeoutInit );
      }
      else
      {
         DTLS_ERROR( "wolfSSL_new failed" );
      }
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return ssl;
}
/***********************************************************************************************************************

   Function name: GetTransportRxBuffer

   Purpose: This function is called to get a transport buffer over RF

   Arguments:
         uint8_t *buf             - Location for transport data
         int32_t sz               - Size of the data buffer

   Returns: int32_t               - Total number of bytes

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static int32_t GetTransportRxBuffer( char *buf )
{
   DTLS_INFO( "GetTransportRxBuffer" );

   int32_t  totalBytes = 0;
   if ( ( _dtlsTransportRxBuffer != NULL ) && ( buf != NULL ) )
   {
      if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
      {
         ( void )memcpy( buf, ( char * )_dtlsTransportRxBuffer->data, _dtlsTransportRxBuffer->x.dataLen );
         totalBytes = _dtlsTransportRxBuffer->x.dataLen;
      }
      else if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
      {
         NWK_DataInd_t* dataIndication = ( NWK_DataInd_t* )( void * )_dtlsTransportRxBuffer->data;
         ( void )memcpy( buf, ( char * )dataIndication->payload, dataIndication->payloadLength );
         totalBytes = dataIndication->payloadLength;
      }
   }
   _dtlsTransportRxBuffer = NULL;

   return totalBytes;
}
/*lint -e{715} */
/*lint -e{818} */
/***********************************************************************************************************************
   Function Name: DtlsRcvCallback

   Purpose: Called by the dtls library to retrieve a buffer to be processed

   Arguments:  ssl - Pointer to this particular session
               buf - The buffer into which the data will be copied
               sz - Size of the buffer
               ctx - The context for this session

   Returns: The number of bytes read or
            WOLFSSL_CBIO_ERR_WANT_READ if not blocking and there is no data available
 **********************************************************************************************************************/
static int32_t DtlsRcvCallback( WOLFSSL *ssl, char *buf, int32_t sz, void *ctx )
{
   int32_t  totalBytes = GetTransportRxBuffer( buf );                   /* Bytes in the message */

   INFO_printf( "DtlsRcvCallback sz = %d totalBytes = %d", sz, totalBytes );

   if ( totalBytes == 0 )
   {
      if ( _dtlsRxTimeout )
      {
         _dtlsRxTimeout = false;

         if ( _dtlsSessionState == DTLS_SESSION_CONNECTING_e )
         {
            _dtlsTaskTimeout *= 2;
            INFO_printf( "DtlsRcvCallback WOLFSSL_CBIO_ERR_TIMEOUT" );
            return ( int32_t )WOLFSSL_CBIO_ERR_TIMEOUT;
         }
      }

      INFO_printf( "DtlsRcvCallback WOLFSSL_CBIO_ERR_WANT_READ" );
      return ( int32_t )WOLFSSL_CBIO_ERR_WANT_READ;
   }
   return totalBytes;
}
/*lint -esym(818,buf) */
/***********************************************************************************************************************
   Function Name: DtlsSendCallback

   Purpose: Called by the dtls library to write out a buffer that has been processed

   Arguments:  ssl - Pointer to this particular session
               buf - The buffer to be outputd
               sz - Number of bytes to be output
               ctx - The context for this session

   Returns: The number of bytes written
 **********************************************************************************************************************/
static int32_t DtlsSendCallback( WOLFSSL *ssl, char *buf, int32_t sz, void *ctx )   /*lint !e818 could be pointer to const */
{
   ( void )ssl;                                  /* Unused */
   ( void )ctx;                                  /* Unused */
   DTLS_INFO( "DtlsSendCallback size = %d", sz );

   ( void )DtlsTransportTx( buf, ( uint32_t )sz );
   return sz;
}
/*lint +esym(818,buf) */
/***********************************************************************************************************************
   Function Name: dtlsSignature64ToAsn

   Purpose: Converts a 64 byte signature to ASN1 format.

   Arguments:  sig - Pointer to signature.
               sigSize - Size of the signature.
               asn - Buffer that will receive ASN.1 formatted value.
               asnSize - Pointer to value that will receive size of ASN1 structure.

   Returns: The number of bytes written
 **********************************************************************************************************************/
static int32_t DtlsSignature64ToAsn( const uint8_t* sig, uint32_t sigSize, uint8_t* asn, uint32_t* asnSize )
{
   if ( *asnSize == 0 )
   {
      return -1;
   }
   if ( sigSize == 0 )
   {
      return -1;
   }
   if ( *asnSize < ( sigSize + 6 ) )
   {
      return -1;
   }

   uint8_t* start = asn;
   *asn++ = 0x30;
   asn++; /* Skip length, will set it at end. */
   *asn++ = 0x02; /* This is an INTEGER. */
   if ( *sig & 0x80 )
   {
      *asn++ = 0x21;
      *asn++ = 0x00;
   }
   else
   {
      *asn++ = 0x20;
   }
   ( void )memcpy( asn, sig, 32 );
   asn += 32;
   sig += 32;

   *asn++ = 0x02; /* This is an INTEGER. */
   if ( *sig & 0x80 )
   {
      *asn++ = 0x21;
      *asn++ = 0x00;
   }
   else
   {
      *asn++ = 0x20;
   }
   ( void )memcpy( asn, sig, 32 );
   asn += 32;

   start[1] = ( uint8_t )( ( asn - start ) - 2 ); /* Update the ASN size. */
   *asnSize = ( uint32_t )( asn - start ); /* Update the total size. */

   return 0;
}
/*lint -esym(715,outSz,keySz,keyDer) */
/*lint -esym(818,outSz,keySz,keyDer) */
/***********************************************************************************************************************
   Function Name: EccSignCallback

   Purpose: This is supplied to the wolfSSL and is called when signing is required from the security chip.

   Arguments:
         WOLFSSL* ssl             - WolfSSL session
         const uint8_t* in        - Buffer in to sign
         uint32_t inSz            - Size of the in buffer
         uint8_t* out             - Signed buffer
         uint32_t* outSz          - Size of the signed buffer
         const uint8_t* keyDer    - Derived key
         uint32_t keySz           - Size of the drived key
         void* ctx                - WolfSSL context

   Returns: The number of bytes written
 **********************************************************************************************************************/
/*lint -esym(715,ssl,ctx) parameters not referenced */
/*lint -esym(818,ssl,ctx) could be ptr to const */
static int32_t EccSignCallback( WOLFSSL* ssl, const uint8_t* in, uint32_t inSz, uint8_t* out, uint32_t* outSz,
                                const uint8_t* keyDer, uint32_t keySz, void* ctx )  /*lint !e818 could be pointer to const */
{
   uint8_t result;                                 /* Return status */
   uint8_t signResult[VERIFY_256_SIGNATURE_SIZE];  /* Signed result buffer */
   uint32_t signSize = VERIFY_256_SIGNATURE_SIZE;  /* Signed result size */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   result = ecc108e_Sign( ( uint16_t ) inSz, ( uint8_t* ) in, ( uint8_t* )signResult );
   DTLS_INFO( "ecc108e_Sign ret = %u", result );
   if ( ECC108_SUCCESS != result )
   {
      DTLS_ERROR( "eccSign operation failed with %u", result );
   }
   else
   {
      /* Now need to wrap the result in ASN.1 */
      if ( DtlsSignature64ToAsn( signResult, signSize, out, outSz ) != 0 )
      {
         return SSL_UNKNOWN;
      }
   }


   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   return ( ECC108_SUCCESS == result ) ? SSL_ERROR_NONE : SSL_UNKNOWN;
}
/***********************************************************************************************************************
   Function Name: DtlsSslRealloc

   Purpose: Callback to inplement realloc for wolfSSL.

   Arguments:
         void *ptr                 - original buffer
         size_t newSize            - New size of the buffer

   Returns: Pointer to the allocated buffer.

   NOTE:
   Not been called by wolf 3.6. Here just in case.
 **********************************************************************************************************************/
static void *DtlsSslRealloc( void *ptr, size_t newSize ) /*lint !e818 could be pointer to const */
{
   ( void ) ptr;
   ( void )newSize;
   DTLS_ERROR( "DtlsSslRealloc NOT implemented" );
   return NULL;
}
/***********************************************************************************************************************

   Function name: DtlsGetOidPrefix

   Purpose: This function is called to get the OID prefix

   Arguments: const uint8_t* oid - The oid string


   Returns: uint8_t *   - Text prefix of oid

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static char* DtlsGetOidPrefix( const uint8_t* oid )
{
   if ( oid[0] == 0x06 && oid[1] == 0x03 && oid[2] == 0x55 && oid[3] == 0x04 )
   {
      switch ( oid[4] )
      {
         case 0x06:
            return "/C=";
         case 0x08:
            return "/ST=";
         case 0x07:
            return "/L=";
         case 0x0A:
            return "/O=";
         case 0x03:
            return "/CN=";
         default:
            return NULL;
      }
   }
   return NULL;
}
/***********************************************************************************************************************

   Function name: DtlsConvertASN1SubjectToString

   Purpose: This function is called to convert the ASN1 subject to a string.

   Arguments:  const uint8_t* subject        - The ASN1 Subject string
               int32_t subjectSize           - The size of the subject string
               uint8_t* outString            - Location to write the converted subject
               int32_t outStringSize         - The size of the string converted

   Returns: int32_t - eFAILURE|failure eSUCCESS|success

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t DtlsConvertASN1SubjectToString( const uint8_t* subject, uint32_t subjectSize, char *outString, uint32_t outStringSize )
{

   if ( subject == NULL )
   {
      return eFAILURE;
   }
   if ( outString == NULL )
   {
      return eFAILURE;
   }
   if ( subjectSize == 0 )
   {
      return eFAILURE;
   }
   if ( outStringSize == 0 )
   {
      return eFAILURE;
   }
   if ( subject[0] != 0x30 )
   {
      return eFAILURE;
   }

   ( void )memset( outString, 0, ( uint32_t )outStringSize );
   const uint8_t* currentByte = subject + 1;

   currentByte += 2;
   int32_t subjectLength = *currentByte & 0x80 ? ( *currentByte & 0x7F ) : *currentByte;
   currentByte++;

   while ( currentByte < ( subject + subjectLength ) )
   {
      if ( *currentByte++ != 0x31 )
      {
         return eFAILURE;
      }

      currentByte++;
      if ( *currentByte++ != 0x30 )
      {
         return eFAILURE;
      }

      currentByte++;

      char* oidPrefix = DtlsGetOidPrefix( currentByte );

      if ( oidPrefix != NULL )
      {
         currentByte += SIZE_OF_OID;

         if ( *currentByte != 0x13 && *currentByte != 0x0C )
         {
            return eFAILURE;
         }
         currentByte++;

         uint32_t attributeLength = *currentByte & 0x80 ? ( *currentByte & 0x7F ) : *currentByte;

         if ( currentByte + attributeLength > ( subject + subjectSize ) )
         {
            return eFAILURE;
         }

         currentByte++;

         ( void )strcat( outString, oidPrefix );
         ( void )strncat( outString, ( const char* )currentByte, attributeLength );
         currentByte += attributeLength;
      }
      else
      {
         return eFAILURE;
      }

   }
   return eSUCCESS;
}
/*lint -esym(715,serverSubjectSz) */
/***********************************************************************************************************************

   Function name: DtlsSubjectsAreEqual

   Purpose: This function is called to convert and compare the subjects from the Server and the Endpoint.

   Arguments:  const uint8_t *serverSubject   - Subject string from the server
               int32_t serverSubjectSz       - Size of the subject string
               const uint8_t *asn1Subject    - Subject from endpoint
               int32_t asn1SubjectSz         - Size of the subject from the endpoint

   Returns: int32_t - eFAILURE|failure eSUCCESS|success

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t DtlsSubjectsAreEqual(   const char* serverSubject, int32_t serverSubjectSz, const uint8_t* asn1Subject, uint32_t asn1SubjectSz )
{
   uint8_t translatedSubject[sizeof( MSSubject_t )]; /* Translated subject */

   ( void )memset( translatedSubject, 0, sizeof( MSSubject_t ) );
   if ( eSUCCESS != DtlsConvertASN1SubjectToString( asn1Subject, asn1SubjectSz, ( char * )translatedSubject, sizeof( translatedSubject ) ) )
   {
      return eFAILURE;
   }

   if ( 0 != strncmp( ( char * )serverSubject, ( char * )translatedSubject, ( size_t )asn1SubjectSz ) )
   {
      return eFAILURE;
   }

   return eSUCCESS;
}
/*lint +esym(715,serverSubjectSz) */
/*lint -esym(818,pX509) */
/***********************************************************************************************************************

   Function name: DtlsVerifyCallback

   Purpose: This function is called by the DTLS task to verify the subject of the certificate matches the subject on the
            the endpoint.

   Arguments:  int32_t currentAuthStatus     - Passed in as the current authentication ret
               WOLFSSL_X509_STORE_CTX *pX509 - Current certificate used for authorization

   Returns: int32_t - 0|failure 1|success

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static int32_t DtlsVerifyCallback( int32_t currentAuthStatus, WOLFSSL_X509_STORE_CTX *pX509 )
{
   uint8_t  subject1[sizeof( MSSubject_t )];
   uint32_t subjectSz1 = sizeof( MSSubject_t );
   uint8_t  subject2[sizeof( MSSubject_t )];
   uint32_t subjectSz2 = sizeof( MSSubject_t );
#if ( EP == 1 )
   bool     pubkeyExtracted = ( bool )false;
#endif

   DTLS_INFO( "DtlsVerifyCallback currentAuthStatus = %d", currentAuthStatus );

   _dtlsServerCertificateSerialSize =
      ( uint16_t )(  pX509->current_cert->serialSz > DTLS_SERVER_CERTIFICATE_SERIAL_MAX_SIZE ?
                     DTLS_SERVER_CERTIFICATE_SERIAL_MAX_SIZE : pX509->current_cert->serialSz );
   ( void )memcpy( _dtlsServerCertificateSerial, pX509->current_cert->serial, _dtlsServerCertificateSerialSize );

#if 0 /* Pre WolfSSL V4.x  */
   DtlsConvertAsn1TimeStringToSysTime( pX509->current_cert->notAfter.data, &_dtlsServerCertificateExpirationSysTime );
#else
   DtlsConvertAsn1TimeStringToSysTime( pX509->current_cert->notAfter, &_dtlsServerCertificateExpirationSysTime );
#endif

   /* If authorization failed, don't override it.  */
   if ( currentAuthStatus == 0 )
   {
      return 0;
   }

   /* Get subject1 and subject 2 for comparison, they come from different places depending on RF or Serial transports.  */
   ( void )memset( subject1, 0, sizeof( MSSubject_t ) );
   ( void )memset( subject2, 0, sizeof( MSSubject_t ) );

   if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
   {
      ( void )memcpy( subject1, ROM_dtlsMfgSubject1, min( subjectSz1, sizeof( ROM_dtlsMfgSubject1 ) ) );

      if ( eSUCCESS != DtlsGetCertificate( subject2, offsetof( intFlashNvMap_t, dtlsMfgSubject2 ), subjectSz2 ) )
      {
         DTLS_ERROR( "DtlsGetCertificate returned eFAILURE for dtlsMfgSubject2" );
      }
   }
   else
   {
      if ( ECC108_SUCCESS != ecc108e_GetDeviceCert( dtlsHESubject_e, subject1, &subjectSz1 ) )
      {
         DTLS_ERROR( "ecc108e_GetDeviceCert returned ECC108_FAILURE for dtlsHESubject_e" );
      }

      if ( ECC108_SUCCESS != ecc108e_GetDeviceCert( dtlsMSSubject_e, subject2, &subjectSz2 ) )
      {
         DTLS_ERROR( "ecc108e_GetDeviceCert returned ECC108_FAILURE for dtlsMSSubject_e" );
      }
   }

   /* Compare the subjects.   */
   if (  ( eSUCCESS == DtlsSubjectsAreEqual( ( char * )pX509->current_cert->subject.staticName, pX509->current_cert->subject.sz, subject1,
                                                subjectSz1 ) ) ||
         ( eSUCCESS == DtlsSubjectsAreEqual( ( char * )pX509->current_cert->subject.staticName, pX509->current_cert->subject.sz, subject2,
                                               subjectSz2 ) ) )
   {
      /* Just received response to Client Hello. Delay 5 seconds after receiving certs ( EP, RF only ). */
      if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
#if ( EP == 1 )
      {
         PartitionData_t const   *pPart;     /* Used to access the security info partition      */
         buffer_t                *decodeBuf; /* used to retrieve/decode public key from WolfSSL */
         buffer_t                *eccBuf;    /* public key extracted from handshake data  */
         buffer_t                *pKeyBuf;   /* holds current NETWORK_PUB_KEY read from security device */
         buffer_t                *pKey;      /* holds x9.63 key data */
         uint8_t                 *pubkey;    /* pointer to pKeyBuf data */
         int32_t                 bufSize;    /* size of public key buffer returned by WolfSSL   */
         uint32_t                idx = 0;
         int32_t                 decodeRes;  /* Return value from decode routines   */

         /* Extract the public key associated with the current cert. */
         if ( wolfSSL_X509_get_pubkey_buffer( pX509->current_cert, NULL, &bufSize ) == WOLFSSL_SUCCESS )  /* Get size of buffer needed. */
         {
            decodeBuf = BM_allocStack( ( uint16_t )bufSize );   /* Allocate buffer of needed size.  */
            if ( decodeBuf != NULL )
            {
               if ( wolfSSL_X509_get_pubkey_buffer( pX509->current_cert, decodeBuf->data, &bufSize ) == WOLFSSL_SUCCESS )
               {
                  eccBuf = BM_allocStack( sizeof( ecc_key ) );
                  if ( eccBuf != NULL )
                  {
                     if ( 0 == wc_ecc_init( ( ecc_key * )( void * )eccBuf->data ) )
                     {
                        /* Got WolfSSL public key in decodeBuf->data. Now translate to needed format.  */
                        decodeRes = wc_EccPublicKeyDecode( decodeBuf->data, &idx, ( ecc_key * )( void * )eccBuf->data, ( uint32_t )bufSize );
                        if ( decodeRes == 0 )
                        {
                           decodeRes = wc_ecc_export_x963( ( ecc_key * )( void * )eccBuf->data, NULL, ( uint32_t * )&bufSize );
                           pKey = BM_allocStack( ( uint16_t )bufSize );

                           if ( pKey != NULL )
                           {
                              decodeRes = wc_ecc_export_x963( ( ecc_key * )( void * )eccBuf->data, pKey->data, ( uint32_t * )&bufSize );
                              if ( 0 == decodeRes )
                              {
                                 pubkeyExtracted = ( bool )true;
                                 pKeyBuf  = BM_allocStack( VERIFY_256_KEY_SIZE + 1U );
                                 if ( pKeyBuf != NULL )
                                 {
#if ( DTLS_DEBUG == 1 )
                                    DBG_logPrintHex( 'I', "\n\npublic key: ", pKey->data, VERIFY_256_KEY_SIZE + 1U );
#endif
                                    pubkey = pKeyBuf->data;
                                    /* Need to save the public key from the cert for use in verifying mtls signatures.  */
                                    pPart = SEC_GetSecPartHandle();                 /* Obtain handle to access internal NV variables   */

                                    /* Read public key from security device to determine if the public key has changed.  */
                                    ( void )memset( pubkey, 0, VERIFY_256_KEY_SIZE );
                                    ( void )ecc108e_ReadKey( NETWORK_PUB_KEY, VERIFY_256_KEY_SIZE, pubkey );

                                    if ( memcmp( pubkey, &pKey->data[1], VERIFY_256_KEY_SIZE ) != 0 )  /* Skip leading 04   */
                                    {
                                       buffer_t *shaBuf;
                                       Sha      *sha;

                                       shaBuf = BM_allocStack( sizeof( Sha ) );
                                       if ( shaBuf != NULL )
                                       {
                                          sha = ( Sha * )( void * )shaBuf->data;
                                          /* Write to internal flash, skippping first byte:  compression point value (4), not needed */
                                          ( void )PAR_partitionFptr.parWrite( ( dSize )offsetof( intFlashNvMap_t, publicKey ), &pKey->data[1], ( lCnt )VERIFY_256_KEY_SIZE, pPart );
                                          /* Perform sha1 on public key with 0x04 prepended to get subject key ID.   */
                                          ( void )wc_InitSha( sha );
                                          ( void )wc_ShaUpdate( sha, pKey->data, VERIFY_256_KEY_SIZE + 1 );
                                          ( void )wc_ShaFinal( sha, pubkey );
                                          ( void )PAR_partitionFptr.parWrite( ( dSize )offsetof( intFlashNvMap_t, subjectKeyId ), pubkey, ( lCnt )SHA_DIGEST_SIZE, pPart );

                                             /* Also write to NETWORK_PUB_KEY (12) slot of the security chip.  */
                                          decodeRes = ( int32_t )ecc108e_WriteKey(  NETWORK_PUB_KEY, VERIFY_256_KEY_SIZE, &pKey->data[1] );
                                          if ( ECC108_SUCCESS != decodeRes )
                                          {
                                             NOP();      /* Convenient spot for breakpoint.  */
                                             pubkeyExtracted = ( bool )false;
                                             DTLS_ERROR( "ecc108e_WriteKey returned: %d", decodeRes );
                                          }
                                          BM_free( shaBuf );
                                       }
                                    }
                                    BM_free( pKeyBuf );
                                 }
                              }
                           }
                           else
                           {
                              DTLS_ERROR( "wc_ecc_export_x963 returned: %d", decodeRes );
                           }
                           BM_free( pKey );
                        }
                        else
                        {
                           DTLS_ERROR( "wc_EccPublicKeyDecode returned: %d", decodeRes );
                        }
                     }
                     ( void )wc_ecc_free( ( ecc_key * )( void * )eccBuf->data );
                     BM_free( eccBuf );
                  }
               }
               BM_free( decodeBuf );
            }
         }
      }
      if ( ( _dtlsTransport == DTLS_TRANSPORT_RF_e ) && ( !pubkeyExtracted ) )
      {
         DTLS_ERROR( "Failed to extract public key and/or subject key ID! MTLS may not work." );
         return 0;
      }
      INFO_printf( "DTLS Certs received." );
#if 0
      OS_TASK_Sleep( 5000 );
#endif
      INFO_printf( "DTLS handshake continuing." );
#else
      {
         INFO_printf( "DTLS Certs received." );
#if 0
         OS_TASK_Sleep( 5000 );
#endif
         INFO_printf( "DTLS handshake continuing." );
      }
#endif
   }
   return 1;
}

/***********************************************************************************************************************

   Function name: DTLS_GetServerCertificateSerialNumber

   Purpose: Retrieve certificate serial number.

   Arguments: void *ptr - Serial number destination

   Returns: uint16_t length - Serial number length

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
uint16_t DTLS_GetServerCertificateSerialNumber( void *ptr )
{
   ( void )memcpy( ptr, _dtlsServerCertificateSerial, _dtlsServerCertificateSerialSize );

   return _dtlsServerCertificateSerialSize;
}
/***********************************************************************************************************************

   Function name: DTLS_GetPublicKey

   Purpose: Allows access to the public key used to verify signatures on multicast messages.

   Arguments: uint8_t  *key   - populated with a pointer to the public key

   Returns: uint16_t keyLen   - number of bytes in the public key

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
uint16_t DTLS_GetPublicKey( uint8_t *key )
{
   uint16_t retVal = 0;
   if ( ECC108_SUCCESS == ecc108e_ReadKey( NETWORK_PUB_KEY, VERIFY_256_KEY_SIZE, key ) )
   {
      retVal = VERIFY_256_KEY_SIZE;
   }
   return retVal;
}
/*lint +esym(818,pX509) */
/***********************************************************************************************************************

   Function name: DtlsLogEvent

   Purpose: This function is called by the DTLS task to verify the subject of the certificate matches the subject on the
            the endpoint.

   Arguments:  uint8_t e_priority   - Event Priority as defined in HEEP
               uint16_t eventId     - Event ID
               const uint8_t *serialNumber - serial number
               uint16_t serialNumberWz - size of the serial number string.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsLogEvent( uint8_t e_priority, uint16_t eventId, const uint8_t *serialNumber, uint16_t serialNumberSz )
{
   EventKeyValuePair_s  keyVal[2];     /* Event key/value pair info  */
   EventData_s          dtlsLogEvent;  /* Event info  */

   ( void )memset( keyVal, 0, sizeof( keyVal ) );

   /* Log the event */

   dtlsLogEvent.markSent = ( bool )false;
   dtlsLogEvent.eventId = eventId;
   *( ( uint16_t * )&keyVal[0].Key[ 0 ] ) = ( uint16_t )comDeviceInterfaceIdentifier;  /*lint !e826 area too small   */
   if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
   {
      keyVal[0].Value[0] = DTLS_MFG_SERIAL_PORT;
   }
   else
   {
#if ( EP == 1 )
      keyVal[0].Value[0] = DTLS_MAC_RF_PORT;
#else
      keyVal[0].Value[0] = DTLS_DCUII_HACK_INTERFACE;
#endif
   }

   *( ( uint16_t * )&keyVal[1].Key[ 0 ] ) = ( uint16_t )dtlsServerCertificateSerialNum;   /*lint !e826 area too small   */
   if ( serialNumberSz > EVENT_VALUE_MAX_LENGTH )
   {
      DTLS_INFO( "Certificate bigger than key value size, truncating" );
      serialNumberSz = EVENT_VALUE_MAX_LENGTH;
   }

   dtlsLogEvent.eventKeyValuePairsCount = 2;
   if ( serialNumberSz > 0 )
   {
      ( void )memcpy( keyVal[1].Value, serialNumber, serialNumberSz );
      Byte_Swap( keyVal[1].Value, ( uint8_t )serialNumberSz );
      dtlsLogEvent.eventKeyValueSize = ( uint8_t )serialNumberSz;
   }
   else
   {
      dtlsLogEvent.eventKeyValueSize = 1;
   }

   /* Priority needs to be added to the HEEP header file */
   ( void )EVL_LogEvent( e_priority, &dtlsLogEvent, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
}
/***********************************************************************************************************************

   Function name: DtlsGetCertificate

   Purpose: This function is called to retrieve the certificate from the NVRAM

   Arguments:  uint8_t *cert                 - Location to place the cert data
               int32_t offset                - Offset into the NVRAM
               uint32_t size                 - Size of the data to extract

   Returns: int32_t - 0|failure 1|success

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t DtlsGetCertificate( uint8_t* cert, int32_t offset, uint32_t size )
{
   PartitionData_t const *pPart;                      /* Used to access the security info partition  */

   pPart = SEC_GetSecPartHandle();

   /* Read the data from secROM  */
   if ( eSUCCESS == ( returnStatus_t )PAR_partitionFptr.parRead( cert, ( dSize )offset, ( lCnt )size, pPart ) )
   {
      return eSUCCESS;
   }
   return eFAILURE;
}
/***********************************************************************************************************************

   Function name: DtlsSerialLoadCerts

   Purpose: This function is called to load the certs for a serial connection

   Arguments: WOLFSSL_CTX *ctx              - WOLF SSL context pointer

   Returns: int32_t - 0|failure 1|success

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t DtlsSerialLoadCerts( WOLFSSL_CTX *ctx )
{
   int32_t ret = SSL_FAILURE;    /* Return status */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   uint8_t *pCert = ( uint8_t * )dtlsMfgRootCA;

   buffer_t* pCertBuf = BM_allocStack( ( uint16_t )dtlsMfgRootCASz );

   if ( pCertBuf != NULL )
   {
      ( void )memcpy( pCertBuf->data, pCert, dtlsMfgRootCASz );

      ret = wolfSSL_CTX_load_verify_buffer( ctx, ( uint8_t* )pCertBuf->data, pCertBuf->x.dataLen, SSL_FILETYPE_ASN1 );
      if ( ret != SSL_SUCCESS )
      {
         DTLS_INFO( "wolfSSL_CTX_load_verify_buffer failed" );
      }
      BM_free( pCertBuf );
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   return ret == SSL_SUCCESS ? eSUCCESS : eFAILURE;
}
/***********************************************************************************************************************

   Function name: DtlsSerialContextCreate

   Purpose: This function is called to create a wolfssl context for the serial connection

   Arguments: None

   Returns: WOLFSSL_CTX *ctx                - Wolfssl Context

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static WOLFSSL_CTX *DtlsSerialContextCreate( void )
{
   WOLFSSL_CTX *ctx = NULL;                      /* WolfSSL context */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   SetSkipValidateDate( 1 );

   WOLFSSL_METHOD *dtlsClientMethod = wolfDTLSv1_2_client_method();
   if ( dtlsClientMethod != NULL )
   {
      ctx = wolfSSL_CTX_new( dtlsClientMethod );
      if ( ctx != NULL )
      {
#if ( DTLS_GROUP_MESSAGES == 1 )
         ( void )wolfSSL_CTX_set_group_messages( ctx );
#endif
         ( void )wolfSSL_CTX_set_timeout( ctx, ( DTLS_SESSION_TIMEOUT_SERIAL / DTLS_MILLISECONDS_IN_SECOND ) );
         wolfSSL_SetIORecv( ctx, DtlsRcvCallback );
         wolfSSL_SetIOSend( ctx, DtlsSendCallback );

         if ( eSUCCESS != DtlsSerialLoadCerts( ctx ) )
         {
            wolfSSL_CTX_free( ctx );
            ctx = NULL;
         }
      }
      else
      {
         DTLS_ERROR( "wolfSSL_CTX_new failed" );
      }
   }
   else
   {
      DTLS_ERROR( "wolfDTLSv1_2_client_method failed" );
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return ctx;
}
/***********************************************************************************************************************

   Function name: DtlsSerialConnect

   Purpose: This function is called to initiate a DTLS connection via the serial port

   Arguments: None

   Returns: void

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsSerialConnect( void )
{
   int32_t sslStat;  /* Return status */
   int32_t error;    /* WolfSSL return status */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   _dtlsSsl = DtlsCreateSession( _dtlsCtx, ( DTLS_TRANSPORT_TIMEOUT_SERIAL / DTLS_MILLISECONDS_IN_SECOND ),
                                 ( DTLS_TRANSPORT_TIMEOUT_SERIAL / DTLS_MILLISECONDS_IN_SECOND ) * DTLS_MAX_RX_COUNT );
   if ( _dtlsSsl != NULL )
   {
      wolfSSL_set_verify( _dtlsSsl, SSL_VERIFY_PEER, DtlsVerifyCallback );
      wolfSSL_set_using_nonblock( _dtlsSsl, 1 );

      DTLS_INFO( "WolfSSL_connect Serial" );
      sslStat = wolfSSL_connect( _dtlsSsl );

      error = wolfSSL_get_error( _dtlsSsl, 0 );

      if ( ( sslStat != SSL_SUCCESS ) && ( ( error == SSL_ERROR_WANT_READ ) || ( error == SSL_ERROR_WANT_WRITE ) ) )
      {
         DTLS_INFO( "Attempting to connect to the DTLS" );
      }
      else
      {
         DTLS_ERROR( "wolfSSL_connect failed with error %d", error );
      }
   }
   else
   {
      DTLS_ERROR( "Failed to create serial session" );
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return;
}
/***********************************************************************************************************************

   Function name: DtlsInitializeSessionCache

   Purpose: This function is called to open the cached session file and check if it contains a valid session. It
            sets the file local variabl _dtlsSessionCacheIsValid. If the file was create write 0 to SessionValid to
            indicate there is no valid session in the file.

   Arguments: None

   Returns: returnStatus_t : status success or failure

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static returnStatus_t DtlsInitializeSessionCache( void )
{
   FileStatus_t   fileStatus;    /* File open status */
   returnStatus_t retVal;
   PartitionData_t const   *pParTbl;

   DTLS_INFO( "DtlsInitializeSessionCache" );
   _sessionCacheInitialized = false;

   /* Attempt to open MAJOR file. */
   retVal = FIO_fopen(  &_dtlsCachedSessionMajorHndl, ePART_DTLS_MAJOR_DATA, ( uint16_t )eFN_DTLS_MAJOR, sizeof( DtlsMajorSession_s ),
                        FILE_IS_NOT_CHECKSUMED, 0, &fileStatus );

#if ( DCU == 1 )
   // At this time, conversion between the old and new MAJOR format is needed only between T-board 1.40.xx and 1.70.xx (or newer).
   // This would also be needed for EPs but there is no plan on updating EPs from 1.40.xx to 1.70.xx (or newer).
   if ( eFILEIO_FILE_SIZE_MISMATCH == retVal )
   {
      // Open failed because it was the old MAJOR file format (i.e. size mismatch)
      // We are going to convert the old format into the new one by:
      // 1) Re-open the file using the old MAJOR format.
      // 2) Convert the old format data to new format data.
      // 3) Erease the MAJOR partition (and hope for no power outage).
      // 4) Re-open the MAJOR partition using the new format. This will create an empty file.
      // 5) Write the new format data back to NV.
      retVal = FIO_fopen(  &_dtlsCachedSessionMajorHndl, ePART_DTLS_MAJOR_DATA, ( uint16_t )eFN_DTLS_MAJOR, sizeof( DCU2_DtlsMajorSession_s ),
                           FILE_IS_NOT_CHECKSUMED, 0, &fileStatus );

      if ( eSUCCESS == retVal )
      {
         /* Convert old MAJOR format to new format */
         buffer_t *pBufOld = BM_alloc( sizeof( DCU2_DtlsMajorSession_s ) );
         buffer_t *pBufNew = BM_alloc( sizeof( DtlsMajorSession_s ) );

         DCU2_DtlsMajorSession_s *pOldStruct;
         DtlsMajorSession_s      *pNewStruct;

         pOldStruct = ( DCU2_DtlsMajorSession_s * )( void * )pBufOld->data;
         pNewStruct = ( DtlsMajorSession_s * )( void * )pBufNew->data;

         // Read old file
         ( void )FIO_fread( &_dtlsCachedSessionMajorHndl, ( uint8_t * )pOldStruct, 0, ( lCnt )sizeof( DCU2_DtlsMajorSession_s ) );

         // Remap data between old and new structs
         memset( pNewStruct, 0, sizeof( DtlsMajorSession_s ) );
         pNewStruct->serverCertificateExpirationSysTime.date = pOldStruct->serverCertificateExpirationSysTime.date;
         pNewStruct->serverCertificateExpirationSysTime.time = pOldStruct->serverCertificateExpirationSysTime.time;
         pNewStruct->dataSize                                = pOldStruct->dataSize;
         memcpy( pNewStruct->data, pOldStruct->data, sizeof( pNewStruct->data ) );

         // Erase the MAJOR partition to force creation on next open
         ( void )PAR_partitionFptr.parOpen( &pParTbl, ePART_DTLS_MAJOR_DATA, 0 );   /* Erase old ePART_DTLS_MAJOR_DATA */
         if ( eSUCCESS == PAR_partitionFptr.parErase( 0, PART_NV_DTLS_MAJOR_SIZE, pParTbl ) )
         {
            // Open using new MAJOR format. This will create an empty file.
            ( void )FIO_fopen(  &_dtlsCachedSessionMajorHndl, ePART_DTLS_MAJOR_DATA, ( uint16_t )eFN_DTLS_MAJOR, sizeof( DtlsMajorSession_s ),
                                FILE_IS_NOT_CHECKSUMED, 0, &fileStatus );

            // Write back data using new format
            ( void )FIO_fwrite( &_dtlsCachedSessionMajorHndl, 0, ( uint8_t * )pNewStruct, ( lCnt )sizeof( DtlsMajorSession_s ) );
         }
         BM_free( pBufOld );
         BM_free( pBufNew );
      }
      else
      {
         DTLS_ERROR( "Unable to open old DTLS MAJOR partition\n" );
      }
   }
#endif

   if ( eSUCCESS == retVal )  /* Attempt to open normal WolfSSL V4.x minor file. */
   {
      retVal = FIO_fopen(  &_dtlsCachedSessionMinorHndl,    /* File Handle so that PHY access the file. */
                           ePART_DTLS_MINOR_DATA,           /* Search for the best partition according to the timing. */
                           ( uint16_t )eFN_DTLS_MINOR,      /* File ID (filename) */
                           sizeof( DtlsMinorSession_s ),    /* Size of the data in the file. */
                           FILE_IS_NOT_CHECKSUMED,          /* File attributes to use. */
                           0,                               /* The update rate of the data in the file. */
                           &fileStatus );                   /* Contains the file status */

      /* If the minor file fails to open, this unit must have established a session with WolfSSL V3.6. Look for the V3.6 exported file.  */
      if ( retVal != eSUCCESS )
      {

         /* Attempt to open V3.6 exported file. If it doesn't exist, OK, just use the new blank minor file created/opened below.   */
#if ( DCU == 1 )
         if ( VER_getDCUVersion() == eDCU2 )
         {
            ( void )FIO_fopen( &_dtlsCachedSessionMinor_wolfSSL36, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_DTLS_WOLFSSL4X,
                                 sizeof( DCU2_DtlsWolfSSL4xSession_s ), FILE_IS_NOT_CHECKSUMED, 0xffffffff, &fileStatus );
         }
         else
#endif
         { //EP and DCU2+
            ( void )FIO_fopen( &_dtlsCachedSessionMinor_wolfSSL36, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_DTLS_WOLFSSL4X,
                               sizeof( DtlsWolfSSL4xSession_s ), FILE_IS_NOT_CHECKSUMED, 0xffffffff, &fileStatus );
         }

         ( void )PAR_partitionFptr.parOpen( &pParTbl, ePART_DTLS_MINOR_DATA, 0 );   /* Erase old ePART_DTLS_MINOR_DATA   */
         if ( eSUCCESS == PAR_partitionFptr.parErase( 0, PART_NV_DTLS_MINOR_SIZE, pParTbl ) )
         {
            /* Now attempt to open the new (blank) V4.x minor file. If this fails, we have a problem!!!  */
            retVal = FIO_fopen(  &_dtlsCachedSessionMinorHndl, ePART_DTLS_MINOR_DATA, ( uint16_t )eFN_DTLS_MINOR, sizeof( DtlsMinorSession_s ),
                                 FILE_IS_NOT_CHECKSUMED, 0, &fileStatus );

         }
         else
         {
            DTLS_ERROR( "Unable to erase old DTLS MINOR partition\n" );
         }
      }
   }

   if ( eSUCCESS == retVal )
   {
      _sessionCacheInitialized = true;
   }
   return retVal;
}
/***********************************************************************************************************************

   Function name: DtlsGetSessionCache

   Purpose: This function is called to load the cached session and attempt to creat the WOLFSSL struct.

   Arguments: WOLFSSL_CTX *context          - WolfSSL context

   Returns: WOLFSSL *                       - The WOLFSSL session

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static WOLFSSL *DtlsGetSessionCache( WOLFSSL_CTX *context )
{
   WOLFSSL              *ssl = NULL;
   DtlsMajorSession_s   *majorSession = NULL;
   DtlsMinorSession_s   *minorSession = NULL;

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   if ( _sessionCacheInitialized )
   {
      DTLS_INFO( "%s, Reading session files", __func__ );
      /* Ming: wolf4x wolfSSL_new() will check random seed. To pass the test I cannot skip seed generation.
               _skipRandomNumber = true;
         Ming: Check and try to recovery from the wolfSSL 3.6 exported file first. */
      ssl = DtlsImportSessionCache_from_wolfSSL36( context );
      if ( NULL != ssl )
      {
         /* Ming: recoverd from wolfSSL 3.6.0
            Save the recoverd Session in 4.x major/minor format */
         DtlsSaveSessionCache( ssl );
         ( void )DtlsUpdateSessionCache( ssl );
      }
      else
      {
         /* Ming: Try to recover from major and minor data */
         majorSession = ( DtlsMajorSession_s* )BM_SpecLibMalloc( ( uint32_t )sizeof( DtlsMajorSession_s ) );
         minorSession = ( DtlsMinorSession_s* )BM_SpecLibMalloc( ( uint32_t )sizeof( DtlsMinorSession_s ) );
         if ( majorSession != NULL && minorSession != NULL )
         {
#if ( EP == 1 )
            if ( !PWRLG_LastGasp() )   /* DO NOT set SYSBUSY during last gasp   */
#endif
            {
               SYSBUSY_setBusy();
            }
            if (  eSUCCESS == FIO_fread( &_dtlsCachedSessionMajorHndl, ( uint8_t * )majorSession, 0, ( lCnt )sizeof( DtlsMajorSession_s ) ) &&
                  eSUCCESS == FIO_fread( &_dtlsCachedSessionMinorHndl, ( uint8_t * )minorSession, 0, ( lCnt )sizeof( DtlsMinorSession_s ) ) )
            {
               if ( ( majorSession->dataSize > 0 ) && ( minorSession->dataSize > 0 ) )
               {
                  ( void )memcpy(  &_dtlsServerCertificateExpirationSysTime,
                                   &( majorSession->serverCertificateExpirationSysTime ), sizeof( sysTime_t ) );

                  DTLS_INFO( "%s: calling wolfSSL_restore_session_with_window", __func__ );
                  ssl = wolfSSL_restore_session_with_window( context, ( void * )majorSession->data, ( int32_t )majorSession->dataSize,
                                                            ( void * )minorSession->data, ( int32_t )minorSession->dataSize );
#if ( EP == 1 )
                  if ( !PWRLG_LastGasp() )   /* Do NOT increment sequence during last gasp */
#endif
                  {
                     /* Increment sequence since Last gasp does not (always?) save the increment */
                     uint32_t seq = (uint32_t)ssl->keys.dtls_sequence_number_lo++;
                     if (seq > (uint32_t)ssl->keys.dtls_sequence_number_lo)
                     {
                        /* handle rollover */
                        ssl->keys.dtls_sequence_number_hi++;
                     }
                  }
                  if ( !QUICK_sessionEncrypted() )
                  {
                     DtlsSaveSessionCache( ssl );  /* From a previous, unencrypted version, update with encryption   */
                  }

#if ( DTLS_DEBUG == 1 )
                  DTLS_dumpState( __func__, ssl );
#endif
               }
            }
            else
            {
               DTLS_ERROR( "%s FIO_fread failed for DtlsMajorSession_s or DtlsMinorSession_s.", __func__ );
            }
#if ( EP == 1 )
            if ( !PWRLG_LastGasp() )   /* DO NOT set SYSBUSY during last gasp   */
#endif
            {
               SYSBUSY_clearBusy();
            }
         }
         else
         {
            DTLS_ERROR( "%s BM_SpecLibMalloc failed for DtlsMajorSession_s or DtlsMinorSession_s.", __func__ );
         }
      }
   }
   else
   {
       ssl = NULL;
   }

   if ( majorSession != NULL )
   {
      BM_SpecLibFree( majorSession );
   }

   if ( minorSession != NULL )
   {
      BM_SpecLibFree( minorSession );
   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   DTLS_INFO( "%s returns 0x%p", __func__, ssl );
#if ( EP == 1 )
   sessionFilesRead = eSESSION_FILE_READ_SUCCESS;
   DTLS_INFO( "%s, sessionFilesRead read", __func__ );
#endif
   return ssl;
}
/***********************************************************************************************************************

   Function name: DtlsSaveSessionCache

   Purpose: This function is called to save the wolfssl session into NVRAM

   Arguments: WOLFSSL *ssl                  - pointer to the WOLFSSL session to save

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsSaveSessionCache( WOLFSSL* ssl )
{
   DtlsMajorSession_s* majorSession;

   DTLS_INFO( "%s", __func__ );

#if ( EP == 1 )
   if ( !PWRLG_LastGasp() )   /* Do not update dtls session data during last gasp   */
#endif
   {
      if ( _sessionCacheInitialized )
      {
         majorSession = ( DtlsMajorSession_s* )BM_SpecLibMalloc( sizeof( DtlsMajorSession_s ) );
         if ( majorSession != NULL )
         {
            ( void )memset( majorSession, 0, sizeof( DtlsMajorSession_s ) );
            ( void )memcpy( &( majorSession->serverCertificateExpirationSysTime ), &_dtlsServerCertificateExpirationSysTime, sizeof( sysTime_t ) );
#if ( TEST_UNENCRYPTED != 0 )    /* V4.x not using encryption, for testing purposes ONLY!  */
#warning "Don't release with TEST_UNENCRYPTED set non-zero!"
            int32_t size;
            ( void )wolfSSL_dtls_export( ssl, NULL, &size );
            if (  ( size > 0 ) &&
                  ( size <= sizeof( majorSession->data ) ) &&
                  ( wolfSSL_dtls_export( ssl, majorSession->data, &size ) >= 0 ) )
#else    /* V4.x quicksession.c using encryption   */
            int32_t size = wolfSSL_get_suspend_memsize( ssl );
            if (  ( size > 0 ) &&
                  ( size <= ( int32_t )sizeof( majorSession->data ) ) &&
                  ( 0 == wolfSSL_suspend_session( ssl, majorSession->data, ( int32_t )sizeof( majorSession->data ) ) ) )
#endif
            {
#if ( DTLS_DEBUG == 1 )
               DTLS_dumpState( __func__, ssl );
#endif
               majorSession->dataSize = ( uint16_t )size;
               DTLS_INFO( "%s: size = %d", __func__, majorSession->dataSize );
               SYSBUSY_setBusy();
               if ( eSUCCESS != FIO_fwrite( &_dtlsCachedSessionMajorHndl, 0, ( uint8_t * )majorSession, ( lCnt )sizeof( DtlsMajorSession_s ) ) )
               {
                  DTLS_ERROR( "%s FIO_fwrite failed.", __func__  );
               }
               SYSBUSY_clearBusy();
            }
            else
            {
               DTLS_ERROR( "%s failed. suspend size = %d", __func__, size );
            }
            BM_SpecLibFree( majorSession );
         }
         else
         {
            DTLS_ERROR( "%s BM_SpecLibMalloc failed for DtlsMajorSession_s.", __func__ );
         }
      }
   }
}
/***********************************************************************************************************************

   Function name: DtlsUpdateSessionCache

   Purpose: This function is called to update the cached session context in the handle _dtlsCachedSessionMinorHndl

   Arguments: WOLFSSL *ssl                  - pointer to the WOLFSSL session to update

   Returns: eSUCCESS or eFAILURE

   Side Effects: Modifies nvram

   Reentrant Code: No

   Notes: None

 ******************************************************************************************************************** */
static returnStatus_t DtlsUpdateSessionCache( WOLFSSL* ssl )
{
   DtlsMinorSession_s   *minorData;
   int32_t              read_sz;
   returnStatus_t       retStatus = eFAILURE;

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   if ( !_sessionCacheInitialized )
   {
      return retStatus;
   }
#if ( EP == 1 )
   if ( !PWRLG_LastGasp() )   /* Do not update dtls session data during last gasp   */
#endif
   {
      minorData = ( DtlsMinorSession_s* )BM_SpecLibMalloc( sizeof( DtlsMinorSession_s ) );
      if ( minorData == NULL )
      {
         DTLS_ERROR( "%s BM_SpecLibMalloc failed.", __func__ );
         return retStatus;
      }

      ( void )memset( minorData, 0, sizeof( DtlsMinorSession_s ) );

#if 0 /* V3.x quicksession.c  */
      read_sz = wolfSSL_export_session_state( ssl, minorData->data, ( int32_t )DTLS_SESSION_MINOR_MAX_SIZE );
      if ( read_sz < 0 )

#else /* V4.x quicksession.c  */
      ( void )wolfSSL_dtls_export_state_only( ssl, NULL, ( uint32_t * )&read_sz ); /* Get size needed, in read_sz, by sending NULL param 2  */
      if (  ( read_sz > ( int32_t )sizeof( minorData->data ) ||
            ( wolfSSL_dtls_export_state_only( ssl, minorData->data, ( uint32_t * )&read_sz ) < 0 ) ) )
#endif

      {
         DTLS_ERROR( "%s wolfSSL_dtls_export_state_only failed. Read size = %d", __func__, read_sz );
      }
      else
      {
#if ( DTLS_DEBUG == 1 )
         DTLS_dumpState( __func__, ssl );
#endif
         minorData->dataSize = ( uint16_t )read_sz;
         DTLS_INFO( "DTLS export state size = %d", read_sz );
         SYSBUSY_setBusy();
         if ( eSUCCESS != FIO_fwrite( &_dtlsCachedSessionMinorHndl, 0, ( uint8_t * )minorData, ( lCnt )sizeof( DtlsMinorSession_s ) ) )
         {
            DTLS_ERROR( "%s FIO_fwrite failed.", __func__ );
         }
         SYSBUSY_clearBusy();
         retStatus = eSUCCESS;
      }

      BM_SpecLibFree( minorData );
   }
#if ( EP == 1 )
   else  /* In last gasp mode */
   {
      retStatus = eSUCCESS;
   }
#endif

   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return retStatus;
}
/***********************************************************************************************************************

   Function name: DtlsInvalidateSessionCache

   Purpose: This function invalidates the session contained in the handle _dtlsCachedSessionMajorHndl

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsInvalidateSessionCache( void )
{
   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

#if ( EP == 1 )
   if ( !PWRLG_LastGasp() )   /* Do not update dtls session data during last gasp   */
#endif
   {
      DtlsMajorSession_s* majorSession = ( DtlsMajorSession_s* )BM_SpecLibMalloc( sizeof( DtlsMajorSession_s ) );
      if ( majorSession != NULL )
      {
         ( void )memset( majorSession, 0, sizeof( DtlsMajorSession_s ) );
         SYSBUSY_setBusy();
         if ( eSUCCESS != FIO_fwrite( &_dtlsCachedSessionMajorHndl, 0, ( uint8_t * )majorSession, ( lCnt )sizeof( DtlsMajorSession_s ) ) )
         {
            DTLS_ERROR( "DtlsInvalidateSessionCache FIO_fwrite failed." );
         }
         SYSBUSY_clearBusy();
         BM_SpecLibFree( majorSession );
      }
      else
      {
         DTLS_ERROR( "DtlsInvalidateSessionCache BM_SpecLibMalloc failed." );
      }
   }

   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
}

/***********************************************************************************************************************

   Function name: DtlsImportSessionCache_from_wolfSSL36

   Purpose: This function is called to import the Minor session data in 3.6 format for use with WolfSSL v4.1.

   Arguments: WOLFSSL *ssl                  - pointer to the WOLFSSL session to update

   Returns: eSUCCESS or eFAILURE

   Side Effects: Modifies nvram

   Reentrant Code: No

   Notes: None

 ******************************************************************************************************************** */
static WOLFSSL* DtlsImportSessionCache_from_wolfSSL36( WOLFSSL_CTX *context )
{
   WOLFSSL                 *ssl = NULL;
#if ( DCU == 1 )
   DCU2_DtlsWolfSSL4xSession_s *DCU2_sessionFromV36 = NULL;
#endif
   DtlsWolfSSL4xSession_s  *sessionFromV36 = NULL;
   returnStatus_t          retVal = eFAILURE;

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

#if ( DCU == 1 )
   if ( VER_getDCUVersion() == eDCU2 ) { //DCU2 Only
      if ( _sessionCacheInitialized )
      {
         DCU2_sessionFromV36 = ( DCU2_DtlsWolfSSL4xSession_s* )BM_SpecLibMalloc( ( uint32_t )sizeof( DCU2_DtlsWolfSSL4xSession_s ) );

         if ( DCU2_sessionFromV36 != NULL )
         {
            SYSBUSY_setBusy();
            retVal = FIO_fread( &_dtlsCachedSessionMinor_wolfSSL36, ( uint8_t* )DCU2_sessionFromV36, 0, ( lCnt )sizeof( DCU2_DtlsWolfSSL4xSession_s ) );
            SYSBUSY_clearBusy();
            if ( eSUCCESS == retVal )
            {
               /* Ming: If the data is available, it means we are reboot from wolfSSL 3.6 version. Then do a import. */
               if ( DCU2_sessionFromV36->dataSize > 0 || DCU2_sessionFromV36->isAvailable )
               {
                  /* sizeof( DtlsTime_t ) used to accomadate SRFN DCU patches, see comment in DtlsTime_t struct declaration */
                  ( void )memcpy(   &_dtlsServerCertificateExpirationSysTime,
                                    &( DCU2_sessionFromV36->serverCertificateExpirationSysTime ), sizeof( DtlsTime_t ) );

                  ssl = wolfSSL_new( context ); /* if _dtlsCtx is NULL, session data was invalid   . */
                  if ( ssl != NULL )
                  {
                     if ( ( wolfSSL_dtls_import( ssl, DCU2_sessionFromV36->data, DCU2_sessionFromV36->dataSize ) ) <= 0 )
                     {
                        DTLS_ERROR( "Unable to import old session from v3.6.0 %d\n" );
                     }
                     else
                     {
                        FileStatus_t            fileStatus;
                        memset( ( uint8_t * )&_dtlsCachedSessionMinorHndl, 0, sizeof( _dtlsCachedSessionMinorHndl ) );
                        retVal = FIO_fopen( &_dtlsCachedSessionMinorHndl,     /* Re-open the new DTLS minor file  */
                                             ePART_DTLS_MINOR_DATA,
                                             ( uint16_t )eFN_DTLS_MINOR,
                                             sizeof( DtlsMinorSession_s ),
                                             FILE_IS_NOT_CHECKSUMED,
                                             0,
                                             &fileStatus );
                        if ( eSUCCESS == retVal )
                        {
                           /* Save the new minor session info. */
                           retVal = FIO_fwrite( &_dtlsCachedSessionMinorHndl, 0, DCU2_sessionFromV36->data, ( lCnt )sizeof( DtlsMinorSession_s ) );
                           if ( retVal != eSUCCESS )
                           {
                              DTLS_ERROR( "Unable to write new minor session data.\n" );
                           }
                        }
                     }
                  }
                  else
                  {
                     DTLS_ERROR( "Failed for to create a SSL object." );
                  }
               }

               /* Ming: Invalidate the data after import */
               ( void )memset( DCU2_sessionFromV36, 0, sizeof( DCU2_DtlsWolfSSL4xSession_s ) );
               SYSBUSY_setBusy();
               retVal = FIO_fwrite( &_dtlsCachedSessionMinor_wolfSSL36, 0, ( uint8_t* )DCU2_sessionFromV36, ( lCnt )sizeof( DCU2_DtlsWolfSSL4xSession_s ) );
               SYSBUSY_clearBusy();
               if ( eSUCCESS != retVal )
               {
                  DTLS_ERROR( "Failed to invalidate file _dtlsCachedSessionMinor_wolfSSL36." );
               }
            }
            else
            {
               DBG_logPrintf( 'I', "No WolfSSL v3.6 file found." );
            }
         }
         else
         {
            DTLS_ERROR( "BM_SpecLibMalloc failed for _dtlsCachedSessionMinor_wolfSSL36." );
         }

         if ( DCU2_sessionFromV36 != NULL )
         {
            BM_SpecLibFree( DCU2_sessionFromV36 );
         }
      }
   }
   else
#endif
   { //EP and DCU2+
#if ( EP == 1 )
      if ( !PWRLG_LastGasp() && _sessionCacheInitialized )  /* Do not attempt import during last gasp  */
#else
      if ( _sessionCacheInitialized )  /* Do not attempt import during last gasp  */
#endif
      {
         sessionFromV36 = ( DtlsWolfSSL4xSession_s* )BM_SpecLibMalloc( ( uint32_t )sizeof( DtlsWolfSSL4xSession_s ) );

         if ( sessionFromV36 != NULL )
         {
            SYSBUSY_setBusy();
            retVal = FIO_fread( &_dtlsCachedSessionMinor_wolfSSL36, ( uint8_t * )sessionFromV36, 0, ( lCnt )sizeof( DtlsWolfSSL4xSession_s ) );
            if ( eSUCCESS == retVal )
            {
               /* Ming: If the data is available, it means we are reboot from wolfSSL 3.6 version. Then do a import. */
               if ( sessionFromV36->dataSize > 0 || sessionFromV36->isAvailable )
               {
                  ( void )memcpy(   &_dtlsServerCertificateExpirationSysTime,
                                    &( sessionFromV36->serverCertificateExpirationSysTime ), sizeof( sysTime_t ) );
                  ssl = wolfSSL_new( context ); /* if _dtlsCtx is NULL, session data was invalid   . */
                  if ( ssl != NULL )
                  {
                     if ( ( wolfSSL_dtls_import( ssl, sessionFromV36->data, sessionFromV36->dataSize ) ) <= 0 )
                     {
                        DTLS_ERROR( "Unable to import old session from v3.6.0 %d\n" );
                     }
                     else
                     {
                        FileStatus_t            fileStatus;
                        memset( ( uint8_t * )&_dtlsCachedSessionMinorHndl, 0, sizeof( _dtlsCachedSessionMinorHndl ) );
                        retVal = FIO_fopen( &_dtlsCachedSessionMinorHndl,     /* Re-open the new DTLS minor file  */
                                             ePART_DTLS_MINOR_DATA,
                                             ( uint16_t )eFN_DTLS_MINOR,
                                             sizeof( DtlsMinorSession_s ),
                                             FILE_IS_NOT_CHECKSUMED,
                                             0,
                                             &fileStatus );
                        if ( eSUCCESS == retVal )
                        {
                           /* Save the new minor session info. */
                           retVal = FIO_fwrite( &_dtlsCachedSessionMinorHndl, 0, sessionFromV36->data, ( lCnt )sizeof( DtlsMinorSession_s ) );
                           if ( retVal != eSUCCESS )
                           {
                              DTLS_ERROR( "Unable to write new minor session data.\n" );
                           }
                        }
                     }
                  }
                  else
                  {
                     DTLS_ERROR( "Failed for to create a SSL object." );
                  }
               }

               /* Ming: Invalidate the data after import */
               ( void )memset( sessionFromV36, 0, sizeof( DtlsWolfSSL4xSession_s ) );
               retVal = FIO_fwrite( &_dtlsCachedSessionMinor_wolfSSL36, 0, ( uint8_t * )sessionFromV36, ( lCnt )sizeof( DtlsWolfSSL4xSession_s ) );
               if ( eSUCCESS != retVal )
               {
                  DTLS_ERROR( "Failed to invalidate file _dtlsCachedSessionMinor_wolfSSL36." );
               }
            }
            else
            {
               DBG_logPrintf( 'I', "No WolfSSL v3.6 file found." );
            }
            SYSBUSY_clearBusy();
         }
         else
         {
            DTLS_ERROR( "BM_SpecLibMalloc failed for _dtlsCachedSessionMinor_wolfSSL36." );
         }

         if ( sessionFromV36 != NULL )
         {
            BM_SpecLibFree( sessionFromV36 );
         }
      }
   }

   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   return ssl;
}

/*lint -esym(715,os) */
/*lint -esym(818,os) */
/***********************************************************************************************************************

   Function name: DtlsGenerateSeed

   Purpose: This function is called to seed the random number generator for WolfSSL. It is a callback

   Arguments:  void *os                      - Not used
               uint8_t output                - Location to place the random number
               int32_t                       - Size of the Random Number

   Returns: 0|success, 1 failed to generate seed

   Side Effects: None

   Reentrant Code: No

   Notes: The ECC_KEY_SIZE is 32 and WOLFSSL seed is 48.

 ******************************************************************************************************************** */
int32_t DtlsGenerateSeed( uint8_t* output, uint32_t sz )
{
   uint8_t  *random; /* Pointer to random number returned by ecc108e_SecureRandom() */
   uint32_t copy;    /* Number of bytes copied per loop  */

   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   /* The random number generator has a limited lifespan since it writes the seed each time, no need to initialize random number if a
      session already exists.  */
   if ( !_skipRandomNumber ) /*lint !e506 !e774 */
   {
      ( void )memset( output, 0, sz );

      /* The WolfSSL seed requires a real 48 bytes random number (it will call wc_RNG_TestSeed() to check
         if it is a real random number). the ecc108e_SecureRandom() will only return 32 bytes random number
         for each call, so we need call it twice to fill up the 48 bytes seed. */
      while ( sz )
      {
         random = ecc108e_SecureRandom();

         if ( random == NULL )
         {
            DTLS_ERROR( "ecc108e_SecureRandom failed." );
            return 1;
         }

         copy = min( sz, ECC108_KEY_SIZE );      /* Compute number of bytes to copy  */
         ( void )memcpy( output, random, copy ); /* Copy to output */
         output   += copy;                       /* Bump output pointer by amount copied   */
         sz       -= copy;                       /* Reduce number of bytes needed by amount copied  */
      }
   }

   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   return 0;
}
/*lint +esym(715,os) */
/*lint +esym(818,os) */
/***********************************************************************************************************************

   Function name: DTLS_GetCounters

   Purpose: Used to get a copy of the DTLS Counters.
            It will copy the data to the destination buffer provided.

   Arguments:
         DTLS_counters_t *pDst - Counters

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void DTLS_GetCounters( DTLS_counters_t *pDst )
{
   ( void )memcpy( pDst, &_dtlsCachedAttr.counters, sizeof( DTLS_counters_t ) );
}
/***********************************************************************************************************************

   Function name: DTLS_CounterInc

   Purpose: This function is used to increment a DTLS counter.

   Arguments:
         DTLS_COUNTER_e eDtlsCounter

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:   This function is intended to handle the rollover if required. If the counter was modified the CachedFile
            will be updated.

 ******************************************************************************************************************** */
void DTLS_CounterInc( DTLS_COUNTER_e eDtlsCounter )
{
   bool bStatus = false;
   switch ( eDtlsCounter )
   {
      /*                                               Counter                                   rollover   */
      case eDTLS_IfInUcastPkts                         :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfInUcastPkts           , ( bool ) true );
         break;
      case eDTLS_IfInSecurityErrors                    :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfInSecurityErrors      , ( bool ) true );
         break;
      case eDTLS_IfInNoSessionErrors                   :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfInNoSessionErrors     , ( bool ) true );
         break;
      case eDTLS_IfInDuplicates                        :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfInDuplicates          , ( bool ) true );
         break;
      case eDTLS_IfInNonApplicationPkts                :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfInNonApplicationPkts  , ( bool ) true );
         break;
      case eDTLS_IfOutUcastPkts                        :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfOutUcastPkts          , ( bool ) true );
         break;
      case eDTLS_IfOutNonApplicationPkts               :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfOutNonApplicationPkts , ( bool ) true );
         break;
      case eDTLS_IfOutNoSessionErrors                  :
         bStatus = DtlsU32CounterInc( &_dtlsCachedAttr.counters.dtlsIfOutNoSessionErrors    , ( bool ) true );
         break;
   }
   if ( bStatus )
   {
      /* The counter was modified   */
      ( void )FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size );
   }
}
/***********************************************************************************************************************

   Function name: DTLS_CounterInc

   Purpose: This function is used to increment a u32 counter. The counter is not incremented if the value UINT32_MAX
            and rollover is set to false.

   Arguments:
         uint32_t *pCounter       - Pointer ot the counter
         bool bRollover           - Roll over indicator

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static bool DtlsU32CounterInc( uint32_t* pCounter, bool bRollover )
{
   bool bStatus = false;
   if ( bRollover || ( *pCounter < UINT32_MAX ) )
   {
      ( *pCounter )++;
      bStatus = true;
   }
   return bStatus;
}
/***********************************************************************************************************************

   Function name: DtlsInitConfigAttr

   Purpose: This function will reset the configuration attributes to default values.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
static void DtlsInitConfigAttr( void )
{
   OS_MUTEX_Lock( &dtlsConfigMutex_ );   /* Function will not return if it fails */
   _dtlsConfigAttr.maxAuthenticationTimeout = DTLS_MAX_AUTH_TIMEOUT_DEFAULT;
   _dtlsConfigAttr.minAuthenticationTimeout = DTLS_MIN_AUTH_TIMEOUT_DEFAULT;
   _dtlsConfigAttr.initialAuthenticationTimeout = DTLS_INIT_AUTH_TIMEOUT_DEFAULT;
   OS_MUTEX_Unlock( &dtlsConfigMutex_ );  /* Function will not return if it fails   */
}
/***********************************************************************************************************************

   Function name: DtlsInitCachedAttr

   Purpose: This function will clear all of the cached attributes (counters).

   Arguments:
         bool bWrite              - Write flag

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsInitCachedAttr( bool bWrite )
{
   ( void ) memset( &_dtlsCachedAttr, 0, sizeof( _dtlsCachedAttr ) );
   if ( bWrite )
   {
      ( void )FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size );
   }
}
/***********************************************************************************************************************

   Function name: DTLS_Reset

   Purpose: This function resets the configurations.

   Arguments:
         DTLS_RESET_e type        - Reset type

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DTLS_Reset( DTLS_RESET_e type )
{
   if ( type == eDTLS_RESET_ALL )
   {
      DtlsInitConfigAttr();
   }
   DtlsInitCachedAttr( ( bool )true );
}
/***********************************************************************************************************************

   Function name: DTLS_Stats

   Purpose: This function is used to print the DTLS status to the terminal.

   Arguments:
         bool option              - true resets the stats.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void DTLS_Stats( bool option )
{
   if ( option )
   {
      /* Reset the just the statistics */
      DTLS_INFO( "Resetting Stats" );
      DTLS_Reset( eDTLS_RESET_STATISTICS );
   }

   INFO_printf( "DTLS_STATS:%s %d", "IfInUcastPkts          ", _dtlsCachedAttr.counters.dtlsIfInUcastPkts );
   INFO_printf( "DTLS_STATS:%s %d", "IfInSecurityErrors     ", _dtlsCachedAttr.counters.dtlsIfInSecurityErrors );
   INFO_printf( "DTLS_STATS:%s %d", "IfInNoSessionErrors    ", _dtlsCachedAttr.counters.dtlsIfInNoSessionErrors );
   INFO_printf( "DTLS_STATS:%s %d", "IfInDuplicates         ", _dtlsCachedAttr.counters.dtlsIfInDuplicates );
   INFO_printf( "DTLS_STATS:%s %d", "IfInNonApplicationPkts ", _dtlsCachedAttr.counters.dtlsIfInNonApplicationPkts );
   INFO_printf( "DTLS_STATS:%s %d", "IfOutUcastPkts         ", _dtlsCachedAttr.counters.dtlsIfOutUcastPkts );
   INFO_printf( "DTLS_STATS:%s %d", "IfOutNonApplicationPkts", _dtlsCachedAttr.counters.dtlsIfOutNonApplicationPkts );
   INFO_printf( "DTLS_STATS:%s %d", "IfOutNoSessionErrors   ", _dtlsCachedAttr.counters.dtlsIfOutNoSessionErrors );
}
/***********************************************************************************************************************

   Function name: DtlsConnectResultCallback

   Purpose: This function will call the a callback function with the current session connection status.

   Arguments:
         returnStatus_t result    - Session connection staus.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsConnectResultCallback( returnStatus_t result )
{
   if ( _connectResultCallback != NULL )
   {
      _connectResultCallback( result );
   }
}
/***********************************************************************************************************************

   Function name: DtlsSessionStateChangedCallback

   Purpose: This function is called on a session status chane.

   Arguments:
         DtlsSessionState_e sessionState    - New state.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsSessionStateChangedCallback( DtlsSessionState_e sessionState )
{
   if ( _sessionStateChangedCallback != NULL )
   {
      _sessionStateChangedCallback( sessionState );
   }
}
/***********************************************************************************************************************

   Function name: DTLS_ReconnectRf

   Purpose: This function is called to reconnect to DTLS.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void DTLS_ReconnectRf( void )
{
   ( void )DtlsSendCommand( DTLS_CMD_RECONNECT_RF_e );
}
/***********************************************************************************************************************

   Function name: DTLS_Shutdown

   Purpose: This function is called to shutdown DTLS.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void DTLS_Shutdown( void )
{
   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   DtlsEvents_e event = DTLS_EVENT_SHUTDOWN_COMPLETE_e;
   ( void )DtlsSendCommand( DTLS_CMD_SHUTDOWN_e );
   ( void )OS_EVNT_Wait ( &_dtlsEvent, ( uint32_t )event, ( bool )false , DTLS_COMMAND_EVENT_TIMEOUT );
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
}
/***********************************************************************************************************************

   Function name: DTLS_GetSessionState

   Purpose: This function is called to get the current session state.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
DtlsSessionState_e DTLS_GetSessionState( void )
{
   return _dtlsSessionState;
}
/***********************************************************************************************************************

   Function name: DTLS_PrintSessionStateInformation

   Purpose: This function prints session state information.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void DTLS_PrintSessionStateInformation( void )
{
   char* state = NULL;

   switch ( _dtlsSessionState )
   {

      case DTLS_SESSION_CONNECTING_e:
         state = "DTLS_SESSION_CONNECTING_e";
         break;

      case DTLS_SESSION_CONNECTED_e:
         state = "DTLS_SESSION_CONNECTED_e";
         break;

      case DTLS_SESSION_SUSPENDED_e:
         state = "DTLS_SESSION_SUSPENDED_e";
         break;

      case DTLS_SESSION_FAILED_e:
         state = "DTLS_SESSION_FAILED_e";
         break;

      case DTLS_SESSION_SHUTDOWN_e:
         state = "DTLS_SESSION_SHUTDOWN_e";
         break;

      case DTLS_SESSION_NONE_e:
      default:
         state = "DTLS_SESSION_NONE_e";
         break;
   }

   INFO_printf( "State = %s Transport = %s", state, DtlsGetTransportString( _dtlsTransport ) );

   if ( _dtlsSsl != NULL )
   {
      switch ( _dtlsSsl->options.clientState )
      {
         case CLIENT_HELLO_COMPLETE:
            INFO_printf( " clientState = CLIENT_HELLO_COMPLETE" );
            break;
         case CLIENT_KEYEXCHANGE_COMPLETE:
            INFO_printf( " clientState = CLIENT_KEYEXCHANGE_COMPLETE" );
            break;
         case CLIENT_FINISHED_COMPLETE:
            INFO_printf( " clientState = CLIENT_FINISHED_COMPLETE" );
            break;
         case HANDSHAKE_DONE:
            INFO_printf( " clientState = HANDSHAKE_DONE" );
            break;
         default:
            INFO_printf( " clientState = UNKNOWN" );
            break;
      }

      INFO_printf( " error = %d", _dtlsSsl->error );
      INFO_printf( " got_server_hello = %d", _dtlsSsl->msgsReceived.got_server_hello );
      INFO_printf( " got_certificate = %d", _dtlsSsl->msgsReceived.got_certificate );
      INFO_printf( " got_server_key_exchange = %d", _dtlsSsl->msgsReceived.got_server_key_exchange );
      INFO_printf( " got_certificate_request = %d", _dtlsSsl->msgsReceived.got_certificate_request );
      INFO_printf( " got_server_hello_done = %d", _dtlsSsl->msgsReceived.got_server_hello_done );
      INFO_printf( " got_change_cipher = %d", _dtlsSsl->msgsReceived.got_change_cipher );
      INFO_printf( " got_finished = %d", _dtlsSsl->msgsReceived.got_finished );

      sysTime_t currentTime;

      if (  _dtlsSessionState == DTLS_SESSION_CONNECTED_e   &&
            _dtlsTransport == DTLS_TRANSPORT_RF_e           &&
            TIME_SYS_GetSysDateTime( &currentTime ) )
      {
         uint32_t currentSeconds;
         uint32_t currentFractionalSeconds;

         TIME_UTIL_ConvertSysFormatToSeconds( &currentTime, &currentSeconds, &currentFractionalSeconds );
         uint32_t totalTimeSpanInMilliseconds = ( currentSeconds - _dtlsCachedAttr.dtlsLastSessionSuccessSysTime ) * DTLS_MILLISECONDS_IN_SECOND;

         INFO_printf( " maximum session age %d sec, current age %d sec", DTLS_SESSION_TIMEOUT_RF / DTLS_MILLISECONDS_IN_SECOND,
                        totalTimeSpanInMilliseconds / DTLS_MILLISECONDS_IN_SECOND );
      }
   }
}
/***********************************************************************************************************************

   Function name: DTLS_IsSessionEstablished

   Purpose: This function is called to return RF connection status..

   Arguments:
         None

   Returns: true if RF session is established, false otherwise.

   Side Effects: None

   Reentrant Code: No

   Notes:   There exists the potential for a race condition regarding whether a cached session is established.
            Implemented a loop of up to 5 tries (10ms apart) to avoid a false negative report. Testing/characterization
            shows a max of 2 needed. As of 8/4/2020, Last Gasp testing has shown this still returns a false negative
            some number(3-4)of last gasp transmissions. Added test for major/minor files read complete.

 ******************************************************************************************************************** */
bool DTLS_IsSessionEstablished( void )
{
   uint32_t  i;
   bool      isEstablished;

#if ( ( DTLS_DEBUG == 1 ) && ( EP==1 ) )
   if ( PWRLG_LastGasp() == 1 )
   {
      DTLS_INFO( "%s, waiting for session files to be read", __func__ );
   }
#endif

#if ( EP == 1 )
   while ( eSESSION_FILE_READ_ATTEMPTING == sessionFilesRead ) /* Loop until at least the major/minor files have been read */
   {
      OS_TASK_Sleep( 10 );
   };
#endif

#if ( ( DTLS_DEBUG == 1 ) && ( EP == 1 ) )
   if ( PWRLG_LastGasp() == 1 )
   {
      DTLS_INFO( "%s, read", __func__ );
   }
#endif

   for ( i = 0; i < 5; i++ )
   {
      isEstablished = ( ( _dtlsSessionState == DTLS_SESSION_CONNECTED_e ) && ( _dtlsTransport == DTLS_TRANSPORT_RF_e ) );
      if ( isEstablished )
      {
         break;
      }
      OS_TASK_Sleep( 10 );
   }

#if ( ( DTLS_DEBUG == 1 ) && ( EP == 1 ) )
   if ( PWRLG_LastGasp() == 1 )
   {
      DTLS_INFO( "%s, isEstablished loop cnt: %d, result: %d\n", __func__, i, isEstablished );
   }
#endif

   return ( isEstablished );
}
/***********************************************************************************************************************

   Function name: DtlsSessionStateChange

   Purpose: This function is called handle the session states.

   Arguments:
         eDtlsSessionState_t sessionState - Session state change.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsSessionStateChange( DtlsSessionState_e sessionState )
{
   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   _dtlsSessionState = sessionState;
   switch ( sessionState )
   {
      case DTLS_SESSION_NONE_e:
         break;

      case DTLS_SESSION_CONNECTING_e:
         if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
         {
            DtlsSessionStateChangedCallback( DTLS_SESSION_CONNECTING_e );
         }
         break;

      case DTLS_SESSION_CONNECTED_e:
         DtlsLogEvent( 110, ( uint16_t )comDeviceSecuritySessionSucceeded, _dtlsServerCertificateSerial, _dtlsServerCertificateSerialSize );
         if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
         {
            DtlsUpdateLastSessionSuccessSysTime();
            DtlsSessionStateChangedCallback( DTLS_SESSION_CONNECTED_e );
            _dtlsTaskTimeout = DTLS_SESSION_TIMEOUT_RF;
            _dtlsAuthenicationTimeoutMax = DTLS_getInitialAuthenticationTimeout();
            DtlsSaveSessionCache( _dtlsSsl );
            ( void )DtlsUpdateSessionCache( _dtlsSsl );
         }
         else if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
         {
            _dtlsTaskTimeout = DTLS_SESSION_TIMEOUT_SERIAL;
         }

         INFO_printf( "connected: %s", _dtlsTransport == DTLS_TRANSPORT_RF_e ? "RF" : "SERIAL" );
         DtlsConnectResultCallback( eSUCCESS );
         break;

      case DTLS_SESSION_FAILED_e:
         DtlsLogEvent( 112, ( uint16_t )comDeviceSecuritySessionFailed, _dtlsServerCertificateSerial, _dtlsServerCertificateSerialSize );
         if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
         {
            DtlsStartBackoff();
         }
         else if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
         {
            DTLS_UartDisconnect();
         }

         DtlsConnectResultCallback( eFAILURE );
         break;

      case DTLS_SESSION_SHUTDOWN_e:
         if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
         {
            DTLS_UartDisconnect();
         }
         else
         {
            DTLS_ReconnectRf();
         }
         break;

      case DTLS_SESSION_SUSPENDED_e:
         if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
         {
            DtlsSessionStateChangedCallback( DTLS_SESSION_SUSPENDED_e );
         }
         break;

      default:
         break;

   }
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
}
/***********************************************************************************************************************

   Function name: DTLS_ProcessDataConfirm

   Purpose: This function handles the DataConfirm message from a lower layer.

   Arguments:
         buffer_t *stack_buffer - Pointer to a buffer containing a data confirmation at stack_buffer->data.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DTLS_ProcessDataConfirm( buffer_t *stack_buffer )
{
   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
   BM_free( stack_buffer );
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
}
/***********************************************************************************************************************

   Function name: DTLS_UartTransportIndication

   Purpose: Called by upper layer to send a request to DTLS, also
   registered with MAC layer as handler for indications

   Arguments:
         request - buffer that contains a dtls request or

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void DTLS_UartTransportIndication( buffer_t *request )
{
#if ( DTLS_DEBUG == 1 )
   DTLS_INFO( "DTLS_UartTransportIndication eSysFmt:%d dataLen:%d", request->eSysFmt, request->x.dataLen );
#endif

   switch ( request->eSysFmt )   /*lint !e788 not all enums used within switch */
   {
      case eSYSFMT_NWK_REQUEST:
         ( void )DtlsSendTransportCommand( DTLS_CMD_APPDATA_TX_e, DTLS_TRANSPORT_SERIAL_e, ( void * )request );
         break;

      case eSYSFMT_NWK_INDICATION:
         ( void )DtlsSendTransportCommand( DTLS_CMD_TRANSPORT_RX_e, DTLS_TRANSPORT_SERIAL_e, ( void * )request );
         break;

      case eSYSFMT_STACK_DATA_CONF:
      case eSYSFMT_NWK_CONFIRM:
         DTLS_ProcessDataConfirm( request );
         break;

      default:
         BM_free( request );
         break;
   }  /*lint !e788 not all enums used within switch */
   return;
}
/***********************************************************************************************************************

   Function name: DTLS_RfTransportIndication

   Purpose: Called by upper layer to send a request to DTLS, also
   registered with MAC layer as handler for indications

   Arguments:
         buffer_t *request - buffer that contains a dtls request or

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void DTLS_RfTransportIndication( buffer_t *request )
{

#if ( DTLS_DEBUG == 1 )
   DTLS_INFO( "DTLS_RfTransportIndication eSysFmt:%d dataLen:%d", request->eSysFmt, request->x.dataLen );
#endif

   switch ( request->eSysFmt )   /*lint !e788 not all enums used within switch */
   {
      case eSYSFMT_NWK_REQUEST:
         ( void )DtlsSendTransportCommand( DTLS_CMD_APPDATA_TX_e, DTLS_TRANSPORT_RF_e, ( void * )request );
         break;

      case eSYSFMT_NWK_INDICATION:
         ( void )DtlsSendTransportCommand( DTLS_CMD_TRANSPORT_RX_e, DTLS_TRANSPORT_RF_e, ( void * )request );
         break;

      case eSYSFMT_STACK_DATA_CONF:
      case eSYSFMT_NWK_CONFIRM:
         DTLS_ProcessDataConfirm( request );
         break;

      default:
         BM_free( request );
         break;
   }  /*lint !e788 not all enums used within switch */
   return;
}
/***********************************************************************************************************************

   Function name: DTLS_AppSecurityAuthModeChanged

   Purpose: Called to nofity DTLS_Task that the auto mode has changed.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void DTLS_AppSecurityAuthModeChanged( void )
{
   ( void )DtlsSendCommand( DTLS_CMD_APP_SECURITY_MODE_CHANGED_e );
}
/***********************************************************************************************************************

   Function name: DtlsRfWrite

   Purpose: This function is called to write data throught the network transport.

   Arguments:
         const char   *pBuf       - Buffer to send
         int32_t      size        - size of the buffer to send

   Returns: int32_t               - Returns how many characters sent

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static int32_t DtlsRfWrite( const char *pBuf, int32_t size )
{
   uint32_t       result;
   NWK_Address_t  dst_addr;
   NWK_Override_t override = { eNWK_LINK_OVERRIDE_NULL, eNWK_LINK_SETTINGS_OVERRIDE_NULL };
   NWK_GetConf_t  GetConf;

   /* Retrieve Head End context ID  */
   GetConf = NWK_GetRequest( eNwkAttr_ipHEContext );

   if ( GetConf.eStatus != eNWK_GET_SUCCESS )
   {
      GetConf.val.ipHEContext = DEFAULT_HEAD_END_CONTEXT; /* Use default in case of error */
   }

   dst_addr.addr_type = eCONTEXT;
   dst_addr.context   = GetConf.val.ipHEContext;

   if ( _dtlsSessionState == DTLS_SESSION_CONNECTED_e && _dtlsTransportTxBuffer != NULL )
   {
      NWK_Request_t* request = ( NWK_Request_t* )( void* )_dtlsTransportTxBuffer->data;

      result = NWK_DataRequest( request->Service.DataReq.port,
                                request->Service.DataReq.qos,
                                ( uint16_t )size,
                                ( uint8_t* )pBuf,
                                &request->Service.DataReq.dstAddress,
                                &request->Service.DataReq.override,
                                request->Service.DataReq.callback,
                                request->pConfirmHandler );

      DTLS_CounterInc( eDTLS_IfOutUcastPkts );
   }
   else
   {
      /* This is an internal handshake or alert packet.  Add network parameters */
      result = NWK_DataRequest( ( uint8_t )UDP_DTLS_PORT, DTLS_RF_QOS, ( uint16_t )size, ( uint8_t* )pBuf, &dst_addr, &override, NULL, NULL );
      DTLS_CounterInc( eDTLS_IfOutNonApplicationPkts );
   }

   if ( 0 == result )
   {
      size = -1;
   }

   return size;
}
/***********************************************************************************************************************

   Function name: DtlsTransportTx

   Purpose: This function is called to transmit DTLS data. It deterimines the which port is connected and will
            make the appropriate call.

   Arguments:
         const uint8_t *pBuf       - Buffer to send
         int32_t size              - size of the buffer to send

   Returns: int32_t               - Returns how many characters sent

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static int32_t DtlsTransportTx( const char *pBuf, uint32_t size )
{
   DTLS_INFO( "DtlsTransportTx size = %d", size );
   int32_t ret = ( int32_t )size;
   if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
   {
      MFGP_UartWrite( pBuf, ( int32_t )size );
   }
   else if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
   {
      ret = DtlsRfWrite( pBuf, ( int32_t )size );
   }
   _dtlsTransportTxBuffer = NULL;
   return ret;
}
/***********************************************************************************************************************

   Function name: DtlsTransportRx

   Purpose: This function is called to handle received DTLS data. It deterimines which port is connected and will
            make the appropriate call.

   Arguments:
         const uint8_t *pBuf    - Buffer to write the data into

   Returns: bool               - True if buffer should be free'd

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static bool DtlsTransportRx( buffer_t *pBuf )
{
   int32_t  ret;
   int32_t  sslError;                  /* Error status from WolfSSL calls */
   bool     freeBuffer = ( bool )true;

   DTLS_INFO( "DtlsTransportRx" );
   _dtlsTransportRxBuffer = pBuf;

   if ( _dtlsSessionState == DTLS_SESSION_CONNECTING_e )
   {
      ret = wolfSSL_connect( _dtlsSsl );

      sslError = wolfSSL_get_error( _dtlsSsl, 0 );
      if ( ret != SSL_SUCCESS && !( sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE ) )
      {
         DTLS_ERROR( "%s wolfSSL_connect returned %d error %d", __func__, ret, sslError );
         DtlsSessionStateChange( DTLS_SESSION_FAILED_e );
         if ( ( _dtlsTransport == DTLS_TRANSPORT_RF_e ) && ( pBuf != NULL ) )
         {
            DTLS_CounterInc( eDTLS_IfInNoSessionErrors );
         }
      }
      else if ( ret == SSL_SUCCESS )
      {
         DtlsSessionStateChange( DTLS_SESSION_CONNECTED_e );
         if ( ( _dtlsTransport == DTLS_TRANSPORT_RF_e ) && ( pBuf != NULL ) )
         {
            DTLS_CounterInc( eDTLS_IfInNonApplicationPkts );
         }
      }
      else if ( ret == ( int32_t )DUPLICATE_MSG_E )
      {
         if ( ( _dtlsTransport == DTLS_TRANSPORT_RF_e ) && ( pBuf != NULL ) )
         {
            DTLS_CounterInc( eDTLS_IfInDuplicates );
         }
      }
      else
      {
         if ( ( _dtlsTransport == DTLS_TRANSPORT_RF_e ) && ( sslError != SSL_ERROR_WANT_READ ) && sslError != SSL_ERROR_WANT_WRITE )
         {
            DTLS_CounterInc( eDTLS_IfInNonApplicationPkts );
         }
      }
   }
   else if ( _dtlsSessionState == DTLS_SESSION_CONNECTED_e )
   {
      if ( pBuf )
      {
         buffer_t *decryptedStackBuffer = BM_allocStack( pBuf->x.dataLen );

         if ( decryptedStackBuffer != NULL )
         {
            ( void )memset( decryptedStackBuffer->data, 0, decryptedStackBuffer->x.dataLen );

            ret = wolfSSL_read( _dtlsSsl, decryptedStackBuffer->data, ( int32_t )decryptedStackBuffer->x.dataLen );

#if ( DTLS_DEBUG == 1 )
            DBG_logPrintHex( 'I', "from wolfSSL_read: ", decryptedStackBuffer->data, decryptedStackBuffer->x.dataLen );
#endif
            if ( ret > 0 )
            {
               decryptedStackBuffer->x.dataLen = ( uint16_t )ret;
               DtlsApplicationRx( pBuf, decryptedStackBuffer );
               if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
               {
                  DtlsUpdateLastSessionSuccessSysTime();
                  ( void )DtlsUpdateSessionCache( _dtlsSsl );
                  DTLS_CounterInc( eDTLS_IfInUcastPkts );
                  freeBuffer = false;
               }
            }
            else if ( ret == 0 ) /* Server disconnected (wolfSSL_shutdown) */
            {
               DtlsSessionStateChange( DTLS_SESSION_SHUTDOWN_e );
               if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
               {
                  DTLS_CounterInc( eDTLS_IfInNonApplicationPkts );
               }
            }
            else
            {
               sslError = wolfSSL_get_error( _dtlsSsl, ret );
               DTLS_ERROR( "DtlsTransportRx wolfSSL returned %d error %d", ret, sslError );

               if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
               {
                  if ( sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE )
                  {
                     DTLS_CounterInc( eDTLS_IfInDuplicates );
                  }
                  else
                  {
                     if ( DECRYPT_ERROR == _dtlsSsl->error )
                     {
                        //build and log comDeviceSecurityDecryptionFailed event
                        EventData_s          eventData;
                        EventKeyValuePair_s  keyVal[ 1 ];   /* Need only one "placeholder" */

                        eventData.markSent                          = (bool)false;
                        eventData.timestamp                         = 0;
                        eventData.eventId                           = (uint16_t)comDeviceSecurityDecryptionFailed;
                        eventData.alarmId                           = 0;
                        eventData.eventKeyValuePairsCount           = 0;
                        eventData.eventKeyValueSize                 = 0;
                        *( uint16_t * )keyVal[ 0 ].Key              = 0;
                        *( uint16_t * )keyVal[ 0 ].Value            = 0;
                        (void)EVL_LogEvent( 111, &eventData, keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
                     }
                     DtlsCheckIfSessionIsStale();
                     DTLS_CounterInc( eDTLS_IfInSecurityErrors );
                  }
               }
            }

            BM_free( decryptedStackBuffer );
         }
         else
         {
            DTLS_ERROR( "DtlsTransportRx failed to allocate decrypted buffer" );
            DtlsSessionStateChange( DTLS_SESSION_FAILED_e );
         }
         if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
         {
            DtlsCheckIfServerCertificateHasExpired(); /*lint !e522 !e523 no side effect.   */
         }
      }
      else
      {
         if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
         {
            DTLS_INFO( "DtlsTransportRx called with timeout shut down connection." );
            DtlsSessionStateChange( DTLS_SESSION_SHUTDOWN_e );
         }
      }
   }
   else
   {
      DTLS_CounterInc( eDTLS_IfInNoSessionErrors );
   }

   /*****************************************************************************************************************
      If a packet is received during a connection failure, render it null. NOTE the buffer will be cleared in the main task.
   *****************************************************************************************************************/
   _dtlsTransportRxBuffer = NULL;

   return freeBuffer;
}
/***********************************************************************************************************************

   Function name: DtlsApplicationRx

   Purpose: This function is called to receive application data.

   Arguments:
         *pBuf                               - Original RX Buffer
         *decryptedStackBuffer               - Decrypted buffer

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsApplicationRx( buffer_t *pBuf, buffer_t *decryptedStackBuffer )
{
   DTLS_INFO( "DtlsApplicationRx" );
   if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
   {
      MFGP_ProcessDecryptedCommand( decryptedStackBuffer );
      DTLS_INFO( "decryptedStackBuffer: %p, len: hu", decryptedStackBuffer, decryptedStackBuffer->x.dataLen );
   }
   else if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
   {
      NWK_DataInd_t *nwk_indication = ( NWK_DataInd_t * )( void * )pBuf->data;

      /* Make sure the buffer is clear of the encrypted data before copying over it */
      ( void )memset( nwk_indication->payload, 0, nwk_indication->payloadLength );

      /* Reset the payload length to the de-crypted length */
      nwk_indication->payloadLength = decryptedStackBuffer->x.dataLen;

      /* Copy the decrypted data into the payload of the original data */
      ( void )memcpy( nwk_indication->payload, decryptedStackBuffer->data, nwk_indication->payloadLength );

      /* Combine the decrypted data and network indication into the DTLS buffer and send to APP */
      ( void )memcpy( decryptedStackBuffer->data, pBuf->data, sizeof( NWK_DataInd_t ) );
      decryptedStackBuffer->eSysFmt = eSYSFMT_NWK_INDICATION;

      /* get the UDP index so the decrypted data can be routed properly */
      uint8_t port = NWK_UdpPortNumToId( nwk_indication->dstUdpPort );

      if ( NULL != _indicationHandlerCallback[port] )
      {
         _indicationHandlerCallback[port]( pBuf );
      }
      else
      {
         DTLS_ERROR( "DtlsApplicationRx no handler registered" );
         DtlsSessionStateChange( DTLS_SESSION_FAILED_e );
      }
   }
}
/***********************************************************************************************************************

   Function name: DtlsApplicationTx

   Purpose: This function is called to transmit application data.

   Arguments:
         const uint8_t *pBuf   - Buffer to write

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsApplicationTx( const buffer_t *pBuf )
{
   int32_t ret = 0;

   DTLS_INFO( "DtlsApplicationTx" );
   _dtlsTransportTxBuffer = pBuf;

   if ( pBuf != NULL )
   {
      if ( _dtlsTransport == DTLS_TRANSPORT_SERIAL_e )
      {
         ret = wolfSSL_write( _dtlsSsl, pBuf->data, pBuf->x.dataLen );
      }
      else if ( _dtlsTransport == DTLS_TRANSPORT_RF_e )
      {
         NWK_Request_t *pReq = ( NWK_Request_t * )( void * )pBuf->data;
         ret = wolfSSL_write( _dtlsSsl, pReq->Service.DataReq.data, pReq->Service.DataReq.payloadLength );
         if ( ret > 0 )
         {
            ( void )DtlsUpdateSessionCache( _dtlsSsl );
         }
      }

      if ( ret <= 0 )
      {
         /* Ming: not all the cases that ret <= 0 are errors, such as SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE.   */
         DTLS_ERROR( "DtlsApplicationTx failed on wolfSSL_write for _dtlsTransport: %d" , ( int32_t )_dtlsTransport );
      }
   }
   else
   {
      DTLS_ERROR( "Tried to send a null buffer" );
   }
}
/***********************************************************************************************************************

   Function name: DtlsBlockOnAppSecurityAuthModeOff

   Purpose: Blocks DTLS_Task until auth mode is turned on.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsBlockOnAppSecurityAuthModeOff( void )
{
   DTLS_INFO( "%s%s", ENTER_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();

   uint8_t appSam = DtlsGetAppSecurityAuthMode();

   while ( appSam == 0 )
   {
      buffer_t* commandBuf;

      if ( OS_MSGQ_Pend( &_dtlsMSGQ, ( void * )&commandBuf, OS_WAIT_FOREVER ) )
      {
         DTLS_INFO( "%s free  = 0x%p", __func__, commandBuf->data );
         DtlsFreeCommand( commandBuf );
      }
      appSam = DtlsGetAppSecurityAuthMode();
   }

   ( void ) DtlsSendCommand( DTLS_CMD_CONNECT_RF_e );
   DTLS_INFO( "%s%s", EXIT_FUNC, __func__ );
   DTLS_DBG_CommandLine_Buffers();
}
/***********************************************************************************************************************

   Function name: DtlsGetAppSecurityAuthMode

   Purpose: Gets and returns the current app security auth mode.

   Arguments: None

   Returns: Current app security auth mode.

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static uint8_t DtlsGetAppSecurityAuthMode( void )
{
   uint8_t  securityMode;

   ( void )APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &securityMode, NULL );

   return securityMode;
}
/***********************************************************************************************************************

   Function name: DtlsConvertAsn1TimeStringToSysTime

   Purpose: Converts an ASN1 time string to sysTime.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes: http://luca.ntop.org/Teaching/Appunti/asn1.html

 ******************************************************************************************************************** */
static void DtlsConvertAsn1TimeStringToSysTime( const uint8_t* asn1Time, sysTime_t* sysTime )
{
   sysTime_dateFormat_t t;
   const char* str = ( const char* ) asn1Time + 2;
   int32_t i = 0;

   ( void )memset( &t, 0, sizeof( t ) );

   if ( asn1Time[0] == ( int32_t )ASN_UTC_TIME ) /* Two digit year */
   {
      t.year = ( uint16_t )( ( str[i++] - '0' ) * 10 );
      t.year += ( uint16_t )( ( str[i++] - '0' ) );
      if ( t.year < 70 )
      {
         t.year += 2000;
      }
      else
      {
         t.year += 1900;
      }
   }
   else if ( asn1Time[0] == ( int32_t )ASN_GENERALIZED_TIME ) /* Four digit year  */
   {
      t.year  = ( uint16_t )( ( str[i++] - '0' ) * 1000 );
      t.year += ( uint16_t )( ( str[i++] - '0' ) * 100   );
      t.year += ( uint16_t )( ( str[i++] - '0' ) * 10    );
      t.year += ( uint16_t )( ( str[i++] - '0' )         );
      t.year -= 1900;
   }

   t.month   = ( uint8_t )( ( str[i++] - '0' ) * 10 );
   t.month  += ( uint8_t )( ( str[i++] - '0' ) );
   t.day     = ( uint8_t )( ( str[i++] - '0' ) * 10 );
   t.day    += ( uint8_t )( ( str[i++] - '0' ) );
   t.hour    = ( uint8_t )( ( str[i++] - '0' ) * 10 );
   t.hour   += ( uint8_t )( ( str[i++] - '0' ) );
   t.min     = ( uint8_t )( ( str[i++] - '0' ) * 10 );
   t.min    += ( uint8_t )( ( str[i++] - '0' ) );
   t.sec     = ( uint8_t )( ( str[i++] - '0' ) * 10 );
   t.sec    += ( uint8_t )( ( str[i]   - '0' ) );

   ( void )TIME_UTIL_ConvertDateFormatToSysFormat( &t, sysTime );
}
/***********************************************************************************************************************

   Function name: DtlsCheckIfServerCertificateHasExpired

   Purpose: Checks to see if the server certificate associated with the current session has expired.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsCheckIfServerCertificateHasExpired( void )
{
#if ( DROP_SESSION_ON_EXPIRED_CERT != 0 )
   sysTime_t currentTime;

   if ( TIME_SYS_GetSysDateTime( &currentTime ) )
   {
      if (  ( currentTime.date > _dtlsServerCertificateExpirationSysTime.date )     ||
            ( ( currentTime.date == _dtlsServerCertificateExpirationSysTime.date )  &&
              ( currentTime.time >= _dtlsServerCertificateExpirationSysTime.time ) ) )
      {
         ( void )DtlsSendCommand( DTLS_CMD_RECONNECT_RF_e );
      }
   }
#endif
}
/***********************************************************************************************************************

   Function name: DtlsCheckIfSessionIsStale

   Purpose: Checks to see if the current session is stale by comparing the current time to the time of the last
            successful DTLS message.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsCheckIfSessionIsStale( void )
{
   sysTime_t currentTime;

   if ( TIME_SYS_GetSysDateTime( &currentTime ) )
   {
      uint32_t currentSeconds;
      uint32_t currentFractionalSeconds;

      TIME_UTIL_ConvertSysFormatToSeconds( &currentTime, &currentSeconds, &currentFractionalSeconds );
      uint32_t totalTimeSpanInMilliseconds = ( currentSeconds - _dtlsCachedAttr.dtlsLastSessionSuccessSysTime ) * DTLS_MILLISECONDS_IN_SECOND;

      if ( totalTimeSpanInMilliseconds > DTLS_SESSION_TIMEOUT_RF )
      {
         ( void )DtlsSendCommand( DTLS_CMD_RECONNECT_RF_e );
      }
   }
}
/***********************************************************************************************************************

   Function name: DtlsUpdateLastSessionSuccessSysTime

   Purpose: Updates the last time a successful DTLS message was exchanged.

   Arguments: None

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsUpdateLastSessionSuccessSysTime( void )
{
   sysTime_t   sysTime;
   uint32_t    fractionalSec;

   (void)TIME_SYS_GetSysDateTime( &sysTime );
   TIME_UTIL_ConvertSysFormatToSeconds(&sysTime, &_dtlsCachedAttr.dtlsLastSessionSuccessSysTime, &fractionalSec);
}
/***********************************************************************************************************************

   Function name: DtlsPrintMessage

   Purpose: This function is called to print a DTLS command message.

   Arguments:
         const DtlsMessage_s* message        - DTLS command header.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
static void DtlsPrintMessage( const DtlsMessage_s* message )
{
   switch ( message->command )
   {
      case DTLS_CMD_CONNECT_SERIAL_e:
         DTLS_INFO( "DTLS_CMD_CONNECT_SERIAL_e" );
         break;
      case DTLS_CMD_DISCONNECT_SERIAL_e:
         DTLS_INFO( "DTLS_CMD_DISCONNECT_SERIAL_e" );
         break;
      case DTLS_CMD_CONNECT_RF_e:
         DTLS_INFO( "DTLS_CMD_CONNECT_RF_e" );
         break;
      case DTLS_CMD_DISCONNECT_RF_e:
         DTLS_INFO( "DTLS_CMD_DISCONNECT_RF_e" );
         break;
      case DTLS_CMD_TRANSPORT_RX_e:
         DTLS_INFO( "DTLS_CMD_TRANSPORT_RX_e size = %d transport = %s", message->data->x.dataLen, DtlsGetTransportString( message->transport ) );
         break;
      case DTLS_CMD_APPDATA_TX_e:
         DTLS_INFO( "DTLS_CMD_APPDATA_TX_e size = %d transport = %s", message->data->x.dataLen, DtlsGetTransportString( message->transport ) );
         break;
      case DTLS_CMD_RECONNECT_RF_e:
         DTLS_INFO( "DTLS_CMD_RECONNECT_RF_e" );
         break;
      case DTLS_CMD_APP_SECURITY_MODE_CHANGED_e:
         DTLS_INFO( "DTLS_CMD_APP_SECURITY_MODE_CHANGED_e" );
         break;
      case DTLS_CMD_SHUTDOWN_e:
         DTLS_INFO( "DTLS_CMD_SHUTDOWN_e" );
         break;
      default:
         DTLS_ERROR( "%d is not a valid command", ( int32_t )message->command );
   }
}

/***********************************************************************************************************************
   Function Name: DtlsGetTransportString

   Purpose: Returns string representation of transport type.

   Arguments: transport - type of transport.

   Returns: none
***********************************************************************************************************************/
static const char* DtlsGetTransportString( DtlsTransport_e transport )
{
   switch ( transport )
   {
      case DTLS_TRANSPORT_RF_e:
         return "DTLS_TRANSPORT_RF_e";

      case DTLS_TRANSPORT_SERIAL_e:
         return "DTLS_TRANSPORT_SERIAL_e";

      case DTLS_TRANSPORT_NONE_e:
      default:
         return "DTLS_TRANSPORT_NONE_e";
   }
}
#if ( DTLS_DEBUG == 1 )
/***********************************************************************************************************************
   Function Name: wolfSSL_LogMessage

   Purpose: Task for the DTLS networking layer (combination of IP/UDP handling)
   Initialize the DTLS library and setup a context.  We won't
   try to connect until the task starts up.

   Arguments: Arg0 - Not used, but required here because this is a task

   Returns: none
***********************************************************************************************************************/
static void wolfSSL_LogMessage( const int32_t logLevel, const char *const logMessage )
{
   DTLS_INFO( "[%d] %s", logLevel, logMessage );
}

#endif
//#endif  // Removed this endif to remove the compilation error of DTLS usage when DTLS is turned off. Added at the EOF
/*!
 **********************************************************************************************************************

   \fn         DTLS_setMaxAuthenticationTimeout

   \brief

   \param      uint32_t

   \return     returnStatus_t

   \details    This function will update the static _dtlsConfigAttr data structure and store
               the updated data structure in NV.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
returnStatus_t DTLS_setMaxAuthenticationTimeout( uint32_t uMaxAuthenticationTimeout )
{
   returnStatus_t retVal = eFAILURE;

   /* Verify if value requested is valid for the parameter  */
   if (  uMaxAuthenticationTimeout >= DTLS_MAX_AUTH_LOWER_BOUND &&
         uMaxAuthenticationTimeout <= DTLS_MAX_AUTH_UPPER_BOUND )
   {
      OS_MUTEX_Lock( &dtlsConfigMutex_ ); /* Function will not return if it fails   */
      /* Take mutex, critical section  */
      _dtlsConfigAttr.maxAuthenticationTimeout = uMaxAuthenticationTimeout;
      retVal = FIO_fwrite( &ConfigFile.handle, 0, ( uint8_t * )&_dtlsConfigAttr, ( lCnt )sizeof( _dtlsConfigAttr ) ); /* write the file back   */
      OS_MUTEX_Unlock( &dtlsConfigMutex_ );  /* End critical section */ /* Function will not return if it fails   */
   }
   else /* value is not valid */
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}

/*!
 **********************************************************************************************************************

   \fn          DTLS_getMaxAuthenticationTimeout

   \brief      Return the maxAuthenticationTimeout parameter.

   \param      void

   \return     uint32_t

   \details    Return the maxAuthenticationTimeout parameter.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
uint32_t DTLS_getMaxAuthenticationTimeout( void )
{
   uint32_t uMaxAuthenticationTimeout;

   OS_MUTEX_Lock( &dtlsConfigMutex_ ); /* Function will not return if it fails   */
   uMaxAuthenticationTimeout = _dtlsConfigAttr.maxAuthenticationTimeout;
   OS_MUTEX_Unlock( &dtlsConfigMutex_ );/* Function will not return if it fails  */

   return( uMaxAuthenticationTimeout );
}

/*!
 **********************************************************************************************************************

   \fn         DTLS_setMinAuthenticationTimeout

   \brief

   \param      uint32_t

   \return     returnStatus_t

   \details    This function will update the static _dtlsConfigAttr data structure and store
               the updated data structure in NV.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
returnStatus_t DTLS_setMinAuthenticationTimeout( uint16_t uMinAuthenticationTimeout )
{
   returnStatus_t retVal = eFAILURE;   /* Return value   */

   /* Verify if the value requested is a valid value for the parameter  */
   if ( uMinAuthenticationTimeout >= DTLS_MIN_AUTH_LOWER_BOUND &&
        uMinAuthenticationTimeout <= DTLS_MIN_AUTH_UPPER_BOUND )
   {
      OS_MUTEX_Lock( &dtlsConfigMutex_ ); /* Function will not return if it fails   */
      /* Take mutex, critical section  */
      _dtlsConfigAttr.minAuthenticationTimeout = uMinAuthenticationTimeout;
      retVal = FIO_fwrite( &ConfigFile.handle, 0, ( uint8_t * )&_dtlsConfigAttr, ( lCnt )sizeof( _dtlsConfigAttr ) ); /* write the file back   */
      OS_MUTEX_Unlock( &dtlsConfigMutex_ );  /* Function will not return if it fails   */
   }
   else /* value requested is invalid  */
   {
      retVal = eAPP_INVALID_VALUE;
   }
   return retVal;
}

/*!
 **********************************************************************************************************************

   \fn         DTLS_getMinAuthenticationTimeout

   \brief      Return the minAuthenticationTimeout parameter.

   \param      void

   \return     uint16_t

   \details    Return the minAuthenticationTimeout parameter.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
uint16_t DTLS_getMinAuthenticationTimeout( void )
{
   uint16_t uMinAuthenticationTimeout;

   OS_MUTEX_Lock( &dtlsConfigMutex_ );   /* Function will not return if it fails */
   uMinAuthenticationTimeout = _dtlsConfigAttr.minAuthenticationTimeout;
   OS_MUTEX_Unlock( &dtlsConfigMutex_ ); /* Function will not return if it fails */

   return( uMinAuthenticationTimeout );
}

/*!
 **********************************************************************************************************************

   \fn         DTLS_setInitialAuthenticationTimeout

   \brief

   \param      uint16_t

   \return     returnStatus_t

   \details    This function will update the static _dtlsConfigAttr data structure and store
               the updated data structure in NV.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
returnStatus_t DTLS_setInitialAuthenticationTimeout( uint16_t uInitialAuthenticationTimeout )
{
   returnStatus_t retVal = eFAILURE;   /* Return value   */

   /* Verify if the value requested is valid for the parameter */
   if (  uInitialAuthenticationTimeout >= DTLS_INITIAL_AUTH_LOWER_BOUND &&
         uInitialAuthenticationTimeout <= DTLS_INITIAL_AUTH_UPPER_BOUND )
   {
      OS_MUTEX_Lock( &dtlsConfigMutex_ );   /* Function will not return if it fails */
      _dtlsConfigAttr.initialAuthenticationTimeout = uInitialAuthenticationTimeout;
      retVal = FIO_fwrite( &ConfigFile.handle, 0, ( uint8_t * )&_dtlsConfigAttr, ( lCnt )sizeof( _dtlsConfigAttr ) ); /* write the file back   */
      OS_MUTEX_Unlock( &dtlsConfigMutex_ ); /* Function will not return if it fails */
   }
   else /* Value requested is invalid  */
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}

/*!
 **********************************************************************************************************************

   \fn          DTLS_getInitialAuthenticationTimeout

   \brief      Return the initialAuthenticationTimeout parameter.

   \param      void

   \return     uint16_t

   \details    Return the initialAuthenticationTimeout parameter.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
uint16_t DTLS_getInitialAuthenticationTimeout( void )
{
   uint16_t uInitialAuthenticationTimeout;

   OS_MUTEX_Lock( &dtlsConfigMutex_ );  /* Function will not return if it fails  */
   uInitialAuthenticationTimeout = _dtlsConfigAttr.initialAuthenticationTimeout;
   OS_MUTEX_Unlock( &dtlsConfigMutex_ );/* Function will not return if it fails  */

   return( uInitialAuthenticationTimeout );
}

/*!
 **********************************************************************************************************************

   \fn         DTLS_getDtlsValue

   \brief      Get DTLS values based upon reading type supplied to function

   \param      meterReadingType - determines which value to acquire
               uint8_t * - pointer to location to store data

   \return     returnStatus_t

   \details    Helper function for OR_PM requests of DTLS information.  This function reads data from the partition
               based upon the reading type provided.

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
returnStatus_t DTLS_getDtlsValue( meterReadingType id, uint8_t *dtlsValue )
{
   returnStatus_t retVal = eFAILURE;
   PartitionData_t const *pPart;
   pPart = SEC_GetSecPartHandle();

   switch ( id )  /*lint !e788 not all enums used within switch */
   {
      case dtlsNetworkRootCA :
      {
         retVal = PAR_partitionFptr.parRead( dtlsValue, ( uint16_t )offsetof( intFlashNvMap_t, dtlsNetworkRootCA ),
                                             ( lCnt )sizeof( NetWorkRootCA_t ), pPart );
         break;
      }
      case dtlsNetworkHESubject :
      {
         retVal = PAR_partitionFptr.parRead( dtlsValue, ( uint16_t )offsetof( intFlashNvMap_t, dtlsHESubject ),
                                             ( lCnt )sizeof( HESubject_t ), pPart );
         break;
      }
      case dtlsNetworkMSSubject :
      {
         retVal = PAR_partitionFptr.parRead( dtlsValue, ( uint16_t )offsetof( intFlashNvMap_t, dtlsMSSubject ),
                                             ( lCnt )sizeof( MSSubject_t ), pPart );
         break;
      }
      case dtlsMfgSubject1 :
      {
         ( void )memcpy( dtlsValue, ( uint8_t * )ROM_dtlsMfgSubject1, sizeof( ROM_dtlsMfgSubject1 ) );
         retVal = eSUCCESS;
         break;
      }
      case dtlsMfgSubject2 :
      {
         retVal = PAR_partitionFptr.parRead( dtlsValue, ( uint16_t )offsetof( intFlashNvMap_t, dtlsMfgSubject2 ),
                                             ( lCnt )sizeof( MSSubject_t ), pPart );
         break;
      }
      default :
      {

         break;
      }
   }  /*lint !e788 not all enums used within switch */

   return retVal;
}

/*!
 **********************************************************************************************************************

   \fn         computeCertLength

   \brief      Determine the length of a the variable lenght DTLS parameter

   \param      uint8_t * - pointer to the data which length needs to be computed for

   \return     uint16_t - the length of the DTLS parameter

   \details

   \sideeffect None

   \reentrant  Yes

 ***********************************************************************************************************************/
uint16_t computeCertLength( uint8_t *cert )
{

   uint8_t           numBytes;     /* Use to compute length of cert for printing  */
   uint8_t           *pCert;       /* Pointer to next byte in cert                */
   uint32_t          certLen = 0;  /* Computed length of cert for printing        */

   if ( cert[ 0 ] == 0x30 )
   {
      if ( ( cert[ 1 ] & 0x80 ) != 0 )
      {
         numBytes = ( uint8_t )( cert[ 1 ] & ~0x80 );
         pCert = &cert[ 2 ];
      }
      else
      {
         numBytes = 1;
         pCert = &cert[ 1 ];
      }
      while ( numBytes-- != 0 )    /* Loop through the length field of the cert */
      {
         certLen = ( certLen << 8 ) + *pCert++;
      }
      certLen += ( uint32_t )( pCert - cert ); /* Account for tag/tag-length fields */
   }

   return ( uint16_t )min( sizeof( NetWorkRootCA_t ), certLen ); /* return the value that is appropriate. */
}
#endif
