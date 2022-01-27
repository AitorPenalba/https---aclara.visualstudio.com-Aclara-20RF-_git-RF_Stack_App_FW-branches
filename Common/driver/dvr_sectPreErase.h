/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dvr_sectPreErase.h
 *
 * Contents: Erases the sector when the 1st byte to that sector is written to.  If using the partition manager, you must
 *           make this partition a multiple of the sector size.
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
#ifndef dvr_sectPreErase_H_
#define dvr_sectPreErase_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "partitions.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef dvr_sectPreErase_GLOBAL
   #define dvr_spe_EXTERN
#else
   #define dvr_spe_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

extern const DeviceDriverMem_t sDeviceDriver_eSpe;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#undef dvr_spe_EXTERN

#endif
