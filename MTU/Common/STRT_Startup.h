/******************************************************************************
 *
 * Filename: STRT_Startup.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef STRT_Startup_H
#define STRT_Startup_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

#define STRT_FLAG_NONE        0u
#define STRT_FLAG_LAST_GASP   1u
#define STRT_FLAG_QUIET       2u
#define STRT_FLAG_RFTEST      4u


/* TYPE DEFINITIONS */

typedef returnStatus_t (*Fxn_Startup)(void);

typedef struct
{
   Fxn_Startup pFxnStrt;
   char *name;
   uint8_t uFlags;
} STRT_FunctionList_t;

typedef enum STRT_CPU_LOAD_PRINT_e
{
   eSTRT_CPU_LOAD_PRINT_ON,
   eSTRT_CPU_LOAD_PRINT_OFF,
   eSTRT_CPU_LOAD_PRINT_SMART,
} STRT_CPU_LOAD_PRINT_e;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
#ifdef __mqx_h__
extern void STRT_CpuLoadPrint ( STRT_CPU_LOAD_PRINT_e mode );
#endif /* __mqx_h__ */

extern void STRT_StartupTask ( taskParameter );
/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
