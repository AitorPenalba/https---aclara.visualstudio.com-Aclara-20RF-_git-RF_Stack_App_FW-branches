/******************************************************************************
 *
 * Filename:    hmc_sync.h
 *
 * Contents:    #defs, typedefs, and function prototypes for setting Meter Time
 *
 ******************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$
 *
 *****************************************************************************/

#ifndef hmc_sync_H  /* replace filetemp with this file's name and remove this comment */
#define hmc_sync_H

#ifdef TIME_SYNC_GLOBAL
   #define TIME_SYNC_EXTERN
#else
   #define TIME_SYNC_EXTERN extern
#endif


/* INCLUDE FILES */

#include "psem.h"
#include "hmc_app.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

extern uint8_t HMC_SYNC_applet(uint8_t, void *);

#endif
