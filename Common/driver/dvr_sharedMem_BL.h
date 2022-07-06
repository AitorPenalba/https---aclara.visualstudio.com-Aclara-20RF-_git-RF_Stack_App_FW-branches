/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dvr_sharedMem.h
 *
 * Contents: Driver for shared memory
 *
 ***********************************************************************************************************************
 * Copyright (c) 2011-2015 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in
 * whole or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ mdeschenenes Created Aug 17, 2015
 *
 **********************************************************************************************************************/
#ifndef DVR_SHAREDMEM_H_
#define DVR_SHAREDMEM_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "partitions_BL.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define INT_FLASH_SECTOR_SIZE    (EXT_FLASH_SECTOR_SIZE) // Internal flash size is the same as external for now.

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

union dvr_shm_type {
   uint8_t pExtSectBuf_[EXT_FLASH_SECTOR_SIZE];
   uint8_t pIntSectBuf_[INT_FLASH_SECTOR_SIZE];
};

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

extern union dvr_shm_type dvr_shm_t;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
returnStatus_t dvr_shm_init( void );
void dvr_shm_lock( void );
void dvr_shm_unlock( void );

#endif
