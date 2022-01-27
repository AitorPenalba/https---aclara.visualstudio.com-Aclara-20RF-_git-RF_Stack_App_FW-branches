/***********************************************************************************************************************
 *
 * Filename: OR_MR_Handler.h
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
 * $Log$ msingla Created Jan 26, 2015
 *
 **********************************************************************************************************************/

#ifndef OR_MR_Handler_H_
#define OR_MR_Handler_H_

/* INCLUDE FILES */
#include "APP_MSG_Handler.h"

/* GLOBAL DEFINTION */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */
/* CONSTANTS */

#define OR_MR_MAX_READINGS (uint8_t) (32) //if changed you must update the MFG orreadlist cmd


/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */
returnStatus_t OR_MR_init( void );
void OR_MR_MsgHandler(HEEP_APPHDR_t *heepHdr, void *payloadBuf, uint16_t length);
void OR_MR_getReadingTypes(uint16_t* pReadingType);
void OR_MR_setReadingTypes(uint16_t* pReadingType);
returnStatus_t OR_MR_OrReadListHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif

