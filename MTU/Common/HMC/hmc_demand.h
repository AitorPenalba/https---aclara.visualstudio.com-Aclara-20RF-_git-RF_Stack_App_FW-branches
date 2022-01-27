/******************************************************************************
 *
 * Filename:    hmc_demand.h
 *
 * Contents:    #defs, typedefs, and function prototypes for Meter Demand
 *    functions
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

#ifndef hmc_demand_H  /* replace filetemp with this file's name and remove this comment */
#define hmc_demand_H

#ifdef HMC_DMD_GLOBAL
   #define HMC_DMD_EXTERN
#else
   #define HMC_DMD_EXTERN extern
#endif

/* INCLUDE FILES */

#include "project.h"
#include "hmc_app.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef void (*pDemandResetCallBack)( returnStatus_t result );

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

uint8_t              HMC_DMD_Reset(uint8_t, void *);
bool                 HMC_DMD_ResetBusy(void);
pDemandResetCallBack HMC_DMD_RegisterCallBack( pDemandResetCallBack callBack );

#undef HMC_DMD_EXTERN

#endif
