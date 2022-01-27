/******************************************************************************
 *
 * Filename:    hmc_time.h
 *
 * Contents:    #defs, typedefs, and function prototypes for setting Meter Time
 *
 ******************************************************************************
 * Copyright (c) 2001-2011 Aclara Technologies LLC.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA TECHNOLOGIES LLC
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$
 *
 *****************************************************************************/

#ifndef hmc_time_H  /* replace filetemp with this file's name and remove this comment */
#define hmc_time_H

/* INCLUDE FILES */

#include "psem.h"
#include "hmc_app.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

extern uint8_t HMC_TIME_Set_Time(uint8_t, void *);
uint32_t HMC_TIME_getMfgPrc70TimeParameters( void );

#endif
