/******************************************************************************
 *
 * Filename: portable_aclara.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2014 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef portable_aclara_H
#define portable_aclara_H

/* INCLUDE FILES */
#include "rand.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */
// Macro used to access a 1-D array as a 2-D array
#define COORD(i, j, sizej) (((i) * (sizej)) + (j))

#define DIV_8_RNDUP(a)   ( ((a) + 7) >> 3)

/*
 *  ======== min ========
 */
#define min(a, b)   ((a) <= (b) ? (a) : (b))

/*
 *  ======== max ========
 */
#define max(a,b)   (((a) >  (b))? (a) : (b))

/* Swap bytes of 32 bit qty   */
#define ENDIAN_SWAP_32BIT(x)  (((x)>>24) | (((x)&0xff0000)>>8) | (((x)&0xff00)<<8) | (((x)&0xff)<<24))

/* Swap bytes of 16 bit qty   */
#define ENDIAN_SWAP_16BIT(x)  (((x)>>8) | (((x)&0xff)<<8))


/*
 *  ======== Replace stdlib rand/srand functions ========
 */
#define rand   aclara_rand
#define srand  aclara_srand

/* TYPE DEFINITIONS */
/* Note:  We should begin transitioning this project to using the standard C data types
          as you write new code use the uint8_t definitions and include the <stdint.h> at the top of your *.c files */
#if 1
#ifndef _STDINT
#include <stdint.h>
#endif
#endif /* 1 */

typedef float  float32_t; /* Floating point 32 bit type */
typedef double float64_t; /* Floating point 64 bit type */

typedef struct
{
   float32_t real;
   float32_t imag;
} cplx_t;

typedef struct
{
   int16_t real;
   int16_t imag;
} int16_cplx_t;

typedef struct
{
   int8_t real;
   int8_t imag;
} int8_cplx_t;

typedef struct
{
   uint16_t row; // Row coordinate for sparse matrix
   uint16_t col; // Col coordinate for sparse matrix
} sparse_t;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
