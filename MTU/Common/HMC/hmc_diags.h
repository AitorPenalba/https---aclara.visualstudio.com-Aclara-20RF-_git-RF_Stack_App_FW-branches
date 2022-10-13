/******************************************************************************
 *
 * Filename:    hmc_diags.h
 *
 * Contents:    #defs, typedefs, and function prototypes for reading
 *    diagnostics from the meter.
 *
 ******************************************************************************
 * Copyright (c) 2001-2011 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$
 *
 *****************************************************************************/

#ifndef hmc_diags_H  /* replace filetemp with this file's name and remove this comment */
#define hmc_diags_H

#ifdef HMC_DIAGS_GLOBAL
#define HMC_DIAGS_EXTERN
#else
#define HMC_DIAGS_EXTERN extern
#endif

/* INCLUDE FILES */

#include "hmc_app.h"

/* CONSTANT DEFINITIONS */
/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */


PACK_BEGIN
typedef struct PACK_MID
{
   EDEventType       eventSet;         /* HEEP value associated with event occurring.  */
   uint8_t           prioritySet;      /* Priority assigned to event occurring         */
   EDEventType       eventCleared;     /* HEEP value associated with event clearing.   */
   uint8_t           priorityCleared;  /* Priority assigned to event clearing.         */
} alarmInfo_t;
PACK_END

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

extern uint8_t HMC_DIAGS_DoDiags(uint8_t, void *);
extern void    HMC_DIAGS_PurgeQueue ( void );

#undef HMC_DIAGS_EXTERN

#endif
