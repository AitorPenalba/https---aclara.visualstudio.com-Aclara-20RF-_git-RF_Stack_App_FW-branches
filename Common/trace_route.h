/***********************************************************************************************************************
 *
 * Filename: trace_route
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2016 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

#ifndef TRACE_ROUTE_H
#define TRACE_ROUTE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "HEEP_util.h"

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

void TraceRoute_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);
void TraceRoute_IndicationHandler(MAC_PingInd_t const *pPingIndication);

void XXX_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf);

#ifdef __cplusplus
}
#endif

#endif /* TRACE_ROUTE_H */
