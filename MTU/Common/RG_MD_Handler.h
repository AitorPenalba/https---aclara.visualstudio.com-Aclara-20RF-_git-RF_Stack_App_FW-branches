/***********************************************************************************************************************
 *
 * Filename: RG_MD_Handler.h
 *
 * Contents:  #defs, typedefs, and function prototypes
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************
 *
 * $Log$ cmusial Created Mar 20, 2015
 *
 **********************************************************************************************************************/

#ifndef RG_MD_Handler_H_
#define RG_MD_Handler_H_

/* INCLUDE FILES */
#include "APP_MSG_Handler.h"

#ifdef RG_MD_GLOBALS
#define RG_MD_EXTERN
#else
#define RG_MD_EXTERN extern
#endif

/* GLOBAL DEFINTION */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
/* Endpoint Registration Metadata */
typedef struct
{
   uint8_t  registrationStatus;            /*!< REG_STATUS_ constants below */
   uint8_t  pad;
   uint16_t initialRegistrationTimeout;    /* the initial value for registration timeout */
   uint16_t minRegistrationTimeout;        /* minumum delay following an initial failed attempt to publish meta data */
   uint32_t maxRegistrationTimeout;        /* maximum delay following a failed attempt to publish meta data */
   uint32_t nextRegistrationTimeout;       /* next max registration timeout limit  */
   uint32_t activeRegistrationTimeout;     /* timeout for active registration metadata */
   uint32_t timeRegistrationSent;          /* date/time /rg/md message was last sent */
} RegistrationInfo_t;

 RG_MD_EXTERN RegistrationInfo_t RegistrationInfo;

/* CONSTANTS */
#define REG_STATUS_NOT_SENT 0
#define REG_STATUS_NOT_CONFIRMED 1
#define REG_STATUS_CONFIRMED 2

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */
RG_MD_EXTERN void RG_MD_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length);
RG_MD_EXTERN buffer_t *RG_MD_BuildMetadataPacket(void);

#endif

