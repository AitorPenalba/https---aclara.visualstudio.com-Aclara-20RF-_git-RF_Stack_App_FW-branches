/***********************************************************************************************************************
 *
 * Filename:    hmc_diags_kv2.h
 *
 * Contents:    #defs, typedefs, and function prototypes for reading  diagnostics from the meter.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$
 *
 **********************************************************************************************************************/

#ifndef hmc_diags_kv2_H  /* replace filetemp with this file's name and remove this comment */
#define hmc_diags_kv2_H

#ifdef HMC_DIAGS_KV2_GLOBAL
#define HMC_DIAGS_KV2_EXTERN
#else
#define HMC_DIAGS_KV2_EXTERN extern
#endif

/* INCLUDE FILES */

#include "project.h"
#include "hmc_app.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

extern uint8_t HMC_DIAGS_KV2_app(uint8_t, void *);

#undef HMC_DIAGS_KV2_EXTERN

#endif
