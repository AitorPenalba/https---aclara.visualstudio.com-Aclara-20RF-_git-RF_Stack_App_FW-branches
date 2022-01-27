/***********************************************************************************************************************
 *
 * Filename: RG_CF_Handler.h
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
 * $Log$ cmusial Created Mar 24, 2015
 *
 **********************************************************************************************************************/

#ifndef RG_CF_Handler_H_
#define RG_CF_Handler_H_

/* INCLUDE FILES */
#include "APP_MSG_Handler.h"

/* GLOBAL DEFINTION */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */
void RG_CF_MsgHandler(HEEP_APPHDR_t *heepReqHdr, void *payloadBuf, uint16_t length);

#endif

