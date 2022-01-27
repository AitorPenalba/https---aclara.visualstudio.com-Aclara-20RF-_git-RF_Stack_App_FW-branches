/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dvr_banked.h
 *
 * Contents: Banked Memory Driver
 *
 ***********************************************************************************************************************
 * Copyright (c) 2011 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * 20110715, v1.0, KAD:  Initial Release
 *
 **********************************************************************************************************************/
#ifndef DVR_BANKED_H_
#define DVR_BANKED_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "partitions.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef dvr_banked_GLOBAL
   #define dvr_banked_EXTERN
#else
   #define dvr_banked_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint8_t sequence;      /* Contains the information that indicates which bank contains the latest data. */
   uint8_t eraseSector;   /* Uses macros: NEXT_SECTOR_ERASE and NEXT_SECTOR_ERASED */
}bankMetaData_t; /* Used for meta data, using a structure here so that added meta data in the future will be simpler. */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

extern const DeviceDriverMem_t sDeviceDriver_eBanked;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

void DVR_BANKED_unitTest(void);

#undef dvr_banked_EXTERN

#endif
