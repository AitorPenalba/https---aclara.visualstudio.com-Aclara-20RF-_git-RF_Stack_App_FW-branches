/******************************************************************************
 *
 * Filename: bit_util.h
 *
 * Contents: Bit Utility prototypes
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log$ MRO created 11/08/12
 *
 *****************************************************************************/

#ifndef BIT_UTIL_H
#define BIT_UTIL_H

/* INCLUDE FILES */

//#include "portable_pic24.h"
#include "project.h"

#ifdef BU_GLOBALS
   #define BU_EXTERN
#else
   #define BU_EXTERN extern
#endif

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

#define BU_ERROR  ((int8_t)-1)

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

void BU_BitRev(far uint32_t *, uint8_t);
int8_t BU_getBit(void *pLoc, uint8_t bytesInArray, uint16_t bitNum);

#undef BU_EXTERN

#endif
