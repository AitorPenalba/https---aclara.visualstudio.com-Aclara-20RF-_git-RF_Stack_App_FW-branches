/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dvr_cache.h
 *
 * Contents: Contains the cache driver table that allows access the cache driver functions.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2011-2020 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created May 4, 2011
 *
 **********************************************************************************************************************/
#ifndef DVR_CACHE_H_
#define DVR_CACHE_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "partitions.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef dvr_cache_GLOBAL
   #define dvr_cache_EXERN
#else
   #define dvr_cache_EXERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint16_t crc16;    /* Cache CRC */
}cacheMetaData_t;     /* Used for meta data, using a structure here so that added meta data in the future will be simpler. */

/* ****************************************************************************************************************** */
/* CONSTANTS */

extern const DeviceDriverMem_t sDeviceDriver_Cache;  /* Contains the cache driver function table */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

void DVR_CACHE_unitTest(void);

#undef dvr_cache_EXERN

#endif
