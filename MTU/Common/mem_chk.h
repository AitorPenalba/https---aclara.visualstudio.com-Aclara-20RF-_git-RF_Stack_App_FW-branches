/******************************************************************************
 *
 * Filename: mem_chk.h
 *
 * Contents: Checks NV Memory
 *
 ******************************************************************************
 * Copyright (c) 2001-2010 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$ kad Created 062304
 *
 *****************************************************************************/

#ifndef mem_chk_H
#define mem_chk_H

/* INCLUDE FILES */

#include "project.h"
#include "partitions.h"

#ifdef MEMC_GLOBAL
   #define MEMC_EXTERN
#else
   #define MEMC_EXTERN extern
#endif

/* CONSTANT DEFINITIONS */

/* Device Part Number Definitions */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

void MEMC_Check(bool verbose, ePowerMode pwrMode);
void MEMC_testPowerDown(bool verbose);

#undef MEMC_EXTERN

#endif
