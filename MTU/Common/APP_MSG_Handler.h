/******************************************************************************
 *
 * Filename: APP_MSG_Handler.h
 *
 * Contents: Application message handler
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2014 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ msingla Created Nov 26, 2014
 *
 **********************************************************************************************************************/

#ifndef APP_MSG_HANDLER_H
#define APP_MSG_HANDLER_H

/* INCLUDE FILES */
#include "HEEP_UTIL.h"
#include "portable_freescale.h"
#include "project.h"

#ifdef APP_MSG_GLOBALS
#define APP_MSG_EXTERN
#else
#define APP_MSG_EXTERN extern
#endif

/* CONSTANTS */
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
/* Max application payload available due to limited RAM resources */
#define APP_MSG_MAX_PAYLOAD  ((uint16_t)1024)
#else
/* Max application payload as defined in HEEP/Tengwar Docs. (1245) minus the DTLS portion (21) */
/* A stack manager query could be added for the actual size based on the message addressing, security etc. settings */
#define APP_MSG_MAX_PAYLOAD  ((uint16_t)1224)
#endif
#define APP_MSG_MAX_DATA     (APP_MSG_MAX_PAYLOAD - HEEP_APP_HEADER_SIZE)
#define APP_MSG_MAX_READINGS (15)
#define APP_MSG_MAX_ALARMS   (15)
#define APP_MAX_SERVICE_TIME_Sec            150            /*maximum amount of time to service a single request*/

#define  EXECUTE_PENDING_WRITE_CMD          ((uint8_t)56)   /* Execute Pending Write Registers */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/*!
 * Application Security file contents
 */
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  f_appSecurityAuthMode;  /*!< Application security/authorization mode.   */
   uint8_t  pad[ 3 ];               /*!< Align to next 32 bit boundary              */
   uint32_t reserved[ 3 ];          /*!< Save room for more variables               */
} appSecurity_t;
PACK_END

#if   ( ( ( USE_MTLS == 1 ) || ( DCU == 1 ) ) && ( USE_IPTUNNEL == 1 ) ) // If MTLS and IPTUNNELING (Remove DCU == 1 when MTLS is enabled for TB)
   #define APP_LOWER_LAYER_COUNT 5                                       // Ports 0-4
#elif ( USE_IPTUNNEL == 1 )                                              // else if IPTUNNELING
   #define APP_LOWER_LAYER_COUNT 4                                       // Ports 0, 1, 3, 4
#elif ( USE_MTLS == 1 ) || ( DCU == 1 )                                  // else if MTLS
   #define APP_LOWER_LAYER_COUNT 3                                       // Ports 0-2
#else                                                                    // else
   #define APP_LOWER_LAYER_COUNT 2                                       // Ports 0-1
#endif

/*!
 * Application Layer Attributes file contents (excluding appSecurityAuthMode)
 */
PACK_BEGIN
typedef struct PACK_MID
{
   uint32_t appIfLastResetTime;                             /* The value of the system time at the moment of the last
                                                               RESET.request being received when the SetDefault parameter was set
                                                               to TRUE. Thisvalue only contains the UTC time in seconds and does
                                                               not contain the fractional seconds portion. */
   uint32_t appIfInErrors        [APP_LOWER_LAYER_COUNT];   /* The number of packets received, which were discarded because they
                                                               contained errors preventing them from being further propagated.
                                                               The count value will roll over. */
   uint32_t appIfInSecurityErrors[APP_LOWER_LAYER_COUNT];   /* The number of packets received, which were discarded because they
                                                               did not provide all required security properties. The count value
                                                               will roll over. */
   uint32_t appIfInOctets        [APP_LOWER_LAYER_COUNT];   /* The number of octets received via the corresponding lower layer
                                                               including HE-EP overhead.  The count value will roll over.  */
   uint32_t appIfInPackets       [APP_LOWER_LAYER_COUNT];   /* The number of packets received via the corresponding lower layer.
                                                               The count value will roll over.*/
   uint32_t appIfOutErrors       [APP_LOWER_LAYER_COUNT];   /* The number of packets attempting transmission via the
                                                               corresponding lower layer which failed due to errors.  The count
                                                               value will roll over.*/
   uint32_t appIfOutOctets       [APP_LOWER_LAYER_COUNT];   /* The number of octets transmitted via the corresponding lower layer
                                                               including HE-EP overhead.  The count value will roll over. */
   uint32_t appIfOutPackets      [APP_LOWER_LAYER_COUNT];   /* The number of packets transmitted via the corresponding lower
                                                               layer. The count value will roll over. */
} appCached_t;
PACK_END

/*!
 * APP Get/Set Attributes
 */
typedef enum
{
   eAppAttr_appIfInErrors,          /* See description in appCached_t */
   eAppAttr_appIfInSecurityErrors,  /* See description in appCached_t */
   eAppAttr_appIfInOctets,          /* See description in appCached_t */
   eAppAttr_appIfInPackets,         /* See description in appCached_t */
   eAppAttr_appIfLastResetTime,     /* See description in appCached_t */
   eAppAttr_appIfOutErrors,         /* See description in appCached_t */
   eAppAttr_appIfOutOctets,         /* See description in appCached_t */
   eAppAttr_appIfOutPackets,        /* See description in appCached_t */
   eAppAttr_appLowerLayerCount,     /* See description in APP_ATTRIBUTES_u */
} APP_ATTRIBUTES_e;

/*!
 * APP Get/Set Parameters
 */
typedef union
{
   uint32_t appIfInErrors[        APP_LOWER_LAYER_COUNT]; /* See description in appCached_t */
   uint32_t appIfInSecurityErrors[APP_LOWER_LAYER_COUNT]; /* See description in appCached_t */
   uint32_t appIfInOctets[        APP_LOWER_LAYER_COUNT]; /* See description in appCached_t */
   TIMESTAMP_t appIfLastResetTime;                        /* See description in appCached_t */
   uint32_t appIfInPackets[       APP_LOWER_LAYER_COUNT]; /* See description in appCached_t */
   uint32_t appIfOutErrors[       APP_LOWER_LAYER_COUNT]; /* See description in appCached_t */
   uint32_t appIfOutOctets[       APP_LOWER_LAYER_COUNT]; /* See description in appCached_t */
   uint32_t appIfOutPackets[      APP_LOWER_LAYER_COUNT]; /* See description in appCached_t */
   uint32_t appLowerLayerCount;                           /* The number of UDP ports interfaced with this HE-EP layer.
                                                             2 for EP (+1 for MTLS, +2 for Tunneling), 2 for DCU */
} APP_ATTRIBUTES_u;

/*! Function Pointer for Opcode Handlers */
typedef void( *fpAppMsgHandlerFunc_t ) (HEEP_APPHDR_t *, void *, uint16_t length);

/*! Data fields required for an opcode definition */
typedef struct
{
   uint8_t resourceID;           //6 bit resource field
   fpAppMsgHandlerFunc_t pfAppMsgHandlerFunc;  //Function associated with this opcode
   struct
   {
      unsigned serial :1;        //Indicates if the function is allowed via the Serial Port
      unsigned ami    :1;        //Indicates if the function is allowed via the AMI Port (RF, TWACS ETC)
      unsigned rsvd   :6;        //Reserved
   } validPort;
} APP_MSG_HandlerInfo_t;

/* GLOBAL VARIABLE DEFINITIONS */
APP_MSG_EXTERN uint16_t metadataRegTimerId;

/* GLOBAL FUNCTION PROTOTYPES */
#ifdef ERROR_CODES_H_
APP_MSG_EXTERN returnStatus_t APP_MSG_init(void);
APP_MSG_EXTERN returnStatus_t APP_MSG_SendRegistrationMetadata(void);
APP_MSG_EXTERN returnStatus_t APP_MSG_setMetadataRegistrationTimer (uint32_t timeRemaining);
APP_MSG_EXTERN returnStatus_t APP_MSG_SecurityHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
APP_MSG_EXTERN returnStatus_t APP_MSG_SetDmdResetDateHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif
APP_MSG_EXTERN void APP_MSG_HandlerTask(uint32_t Arg0);
APP_MSG_EXTERN bool APP_MSG_HandlerReady(void);
APP_MSG_EXTERN void APP_MSG_updateRegistrationTimeout(void);
APP_MSG_EXTERN void APP_MSG_UpdateRegistrationFile(void);
APP_MSG_EXTERN void APP_MSG_ReEnableRegistration(void);
APP_MSG_EXTERN void APP_MSG_initRegistrationInfo(void);
APP_MSG_EXTERN returnStatus_t APP_MSG_setInitialRegistrationTimeout(uint16_t uInitialRegistrationTimeout);
APP_MSG_EXTERN returnStatus_t APP_MSG_setMinRegistrationTimeout(uint16_t uMinRegistrationTimeout);
APP_MSG_EXTERN returnStatus_t APP_MSG_setMaxRegistrationTimeout(uint32_t uMaxRegistrationTimeout);
APP_MSG_EXTERN returnStatus_t APP_MSG_initialRegistrationTimeoutHandler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);
APP_MSG_EXTERN returnStatus_t APP_MSG_maxRegistrationTimeoutHandler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);
APP_MSG_EXTERN returnStatus_t APP_MSG_minRegistrationTimeoutHandler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);

#undef APP_MSG_EXTERN

#endif

