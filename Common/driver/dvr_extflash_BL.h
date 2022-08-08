/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dvr_extflash.h
 *
 * Contents: Driver for the External Flash Memory.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2011-2020 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in
 * whole or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created Feb 18, 2011
 *
 **********************************************************************************************************************/
#ifndef DVR_EXTFLASH_H_
#define DVR_EXTFLASH_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project_BL.h"
#include "partitions_BL.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef dvr_eflash_GLOBAL
   #define DVR_EFL_EXTERN
#else
   #define DVR_EFL_EXTERN extern
#endif


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* The following are used by the partition manager.  This goes in the partition manager's data table. */
#define DVR_BANKED_MAX_UPDATE_RATE_SEC    ((uint32_t)2 * 60 * 60) /* Maximum update rate for the banked driver. */
#define DVR_EFL_MAX_UPDATE_RATE_SECONDS   ((uint32_t)24 * 60 * 60) /* Maximum update rate for the flash devices supported */
#define DVR_EFL_DATA_RETENTION_YEARS      ((uint8_t)20)      /* Years before a refresh is required */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint8_t port;     /* SPI Port to use for the flash IC */
   uint8_t pcs;      /* Peripheral Chip Select pin on the SPI Port used for the flash IC. */
}SpiFlashDevice_t;   /* Flash SPI Port and Chip Select information */

typedef enum
{
   eAutoErase = (uint8_t)0,     /* Enables, Disables the auto erase before writing.  If enabled, the driver will erase
                               * a sector as necessary to perform a write operation.  If disabled, the driver will write
                               * to a sector without erasing if doing so will be successful.  Meaning, if a physical bit
                               * in the device is changing state from a 1 to a 0, it is allowed, but not from a 0 to a 1
                               * (will not be successful). */
   eRtosCmds                  /* Enables or Disables the use of RTOS functions.  If enabled, the driver will use
                               * semaphores to pend.  If disabled, the driver will not pend.  This is used at power up
                               * when all of the tasks may not be initialized before this driver is called.  If pend is
                               * disabled, the driver will use a polling method. */
}eDVR_EFL_Cmd_t;

typedef enum
{
   eAutoEraseOn = (bool)0, /* Erases memory if needed before a write.  */
   eAutoEraseOff     /* If a write requires an erase, failure is returned.  */
}eDVR_EFL_IoctlAutoErase_t;

typedef enum
{
   eRtosCmdsDis = (bool)0,  /* Disables this driver from using the semaphore pend commands. */
   eRtosCmdsEn                 /* Enables this driver to use the semaphore pend commands. */
}eDVR_EFL_IoctlRtosCmds_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

extern DeviceDriverMem_t sDeviceDriver_eFlash; /* Access Table for this driver. */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

uint32_t DVR_EFL_UnitTest( uint32_t ReadRepeat);

#undef DVR_EFL_EXTERN

#endif
