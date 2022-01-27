/***********************************************************************************************************************
 *
 * Filename: eng_res.h
 *
 * Contents: This file contains functions that handle incoming bu_en resource requests and the generation of bubble up
 *   engineering messages.
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
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef ENG_RES_H
#define ENG_RES_H

/* INCLUDE FILES */
#include "HEEP_util.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* Data.Request */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

extern void eng_res_MsgHandler(HEEP_APPHDR_t *heepHdr, void *payloadBuf, uint16_t length);

extern buffer_t *eng_res_BuildStatsPayload(void);
#if ( EP == 1 )
extern buffer_t *eng_res_Build_OB_LinkPayload(void);
#endif

extern uint16_t ENG_RES_statsReading( bool full, uint8_t *address );
extern uint16_t ENG_RES_rssiLinkReading( uint8_t *dst );

#endif /* ENG_RES_H */
