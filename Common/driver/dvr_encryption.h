/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dvr_encryption.h
 *
 * Contents: Contains the encryption driver table that allows access the encryption driver functions.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created August 20, 2013
 *
 **********************************************************************************************************************/
#ifndef DVR_ENCRYPTION_H_
#define DVR_ENCRYPTION_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "partitions.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef dvr_encryption_GLOBAL
   #define dvr_encryption_EXERN
#else
   #define dvr_encryption_EXERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

extern const DeviceDriverMem_t sDeviceDriver_Encryption;  /* Contains the encryption driver function table */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

void DVR_ENCRYPT_unitTest(void);

#undef dvr_encryption_EXERN

#endif
