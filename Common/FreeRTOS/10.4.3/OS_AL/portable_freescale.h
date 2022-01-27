/******************************************************************************
 *
 * Filename:    portable_freescale.h
 *
 * Contents:    Portable data type declarations for the freescale Family
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 * Revision History:
 * v0.1 - AG Added revision history and copyright information
 ******************************************************************************
 *
 * $Log$
 *
 *****************************************************************************/

#ifndef PORTABLE_freescale_H
#define PORTABLE_freescale_H

/* INCLUDE FILES */

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

#define ARRAY_IDX_CNT( x )    (sizeof(x) / sizeof(x[0]))
#define MINIMUM(a,b)          ((a)<(b)?(a):(b))

#define PACK_BEGIN            __packed
#define PACK_MID
#define PACK_INLINE           __packed
#define PACK_END

#define PROCESSOR_BIG_ENDIAN     0  /* 0 = Little endian machine, 1 = Big endian machine */
#define PROCESSOR_LITTLE_ENDIAN  1  /* 1 = Little endian machine, 0 = Big endian machine */
#define BIT_ORDER_LOW_TO_HIGH    1  /* 1 = Structures are defined with bit 0 as the 1st entry, 0 = bit 0 is last entry. */

/*lint -emacro(572, BIG_ENDIAN) -emacro(578, BIG_ENDIAN) -emacro(778, BIG_ENDIAN) -emacro(835, BIG_ENDIAN)   */
#if ( PROCESSOR_BIG_ENDIAN == 1 )
#define BIG_ENDIAN_16BIT(x)      (x)
#define LITTLE_ENDIAN_16BIT(x)   ((uint16_t)(((uint16_t)(x)<<8) | ((uint16_t)(x)>>8)))
#else
#define LITTLE_ENDIAN_16BIT(x)   (x)
#define BIG_ENDIAN_16BIT(x)      ((uint16_t)(((uint16_t)(x)<<8) | ((uint16_t)(x)>>8)))
#endif

#define far
#define near

/* TYPE DEFINITIONS */
typedef uint32_t  nvAddr;  		/* This type is to hold the address of an item in NV memory */
typedef int16_t   nvSize;  		/* This type is to hold the size of NV memory */
typedef uint32_t  flAddr;   		/* this type is to hold the address of an item in program memory */
typedef uint32_t  flSize;   		/* this type is to hold the address of an item in program memory */

typedef uint32_t  lAddr;        /*!<  Logical Address */
typedef uint32_t  fileOffset;   /*!<  Offset into a file */
typedef uint32_t  lCnt;         /*!<  Logical Address Count */
typedef uint32_t  dSize;        /*!<  Device Size */
typedef uint32_t  phAddr;       /*!<  physical Address */

PACK_BEGIN
typedef struct PACK_MID
{
   unsigned b0 :  1;
   unsigned b1 :  1;
   unsigned b2 :  1;
   unsigned b3 :  1;
   unsigned b4 :  1;
   unsigned b5 :  1;
   unsigned b6 :  1;
   unsigned b7 :  1;
} BitStruct;
PACK_END
PACK_BEGIN
typedef struct PACK_MID
{
   unsigned b0 :  1;
   unsigned b1 :  1;
   unsigned b2 :  1;
   unsigned b3 :  1;
   unsigned b4 :  1;
   unsigned b5 :  1;
   unsigned b6 :  1;
   unsigned b7 :  1;
   unsigned b8 :  1;
   unsigned b9 :  1;
   unsigned b10 :  1;
   unsigned b11 :  1;
   unsigned b12 :  1;
   unsigned b13 :  1;
   unsigned b14 :  1;
   unsigned b15 :  1;
   unsigned b16 :  1;
   unsigned b17 :  1;
   unsigned b18 :  1;
   unsigned b19 :  1;
   unsigned b20 :  1;
   unsigned b21 :  1;
   unsigned b22 :  1;
   unsigned b23 :  1;
   unsigned b24 :  1;
   unsigned b25 :  1;
   unsigned b26 :  1;
   unsigned b27 :  1;
   unsigned b28 :  1;
   unsigned b29 :  1;
   unsigned b30 :  1;
   unsigned b31 :  1;
} BitStruct32;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   unsigned b0 :  1;
   unsigned b1 :  1;
   unsigned b2 :  1;
   unsigned b3 :  1;
   unsigned b4 :  1;
   unsigned b5 :  1;
   unsigned b6 :  1;
   unsigned b7 :  1;
   unsigned b8 :  1;
   unsigned b9 :  1;
   unsigned b10 :  1;
   unsigned b11 :  1;
   unsigned b12 :  1;
   unsigned b13 :  1;
   unsigned b14 :  1;
   unsigned b15 :  1;
} BitStruct16;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   BitStruct   Bits;
   uint8_t       Byte;
} uint8_BA;
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct PACK_MID
   {
      uint8_t Lsb;
      uint8_t Msb;
   } LE;          /* Looking at the data as if it is Little Endian */
   struct PACK_MID
   {
      uint8_t Msb;
      uint8_t Lsb;
   } BE;          /* Looking at the data as if it is Big Endian */
   struct PACK_MID
   {
      uint8_t Lsb;
      uint8_t Msb;
   } Byte;  /* Native value */
   uint16_t n16; /* Native 16-bit value */
} uint16_BA;   /* Byte Addressable uint16_t */
PACK_END

PACK_BEGIN
typedef union PACK_MID
{
   struct PACK_MID
   {
      uint16_BA Lsw;
      uint16_BA Msw;
   } LE;                /* Looking at the data as if it is Little Endian */
   struct PACK_MID
   {
      uint16_BA Msw;
      uint16_BA Lsw;
   } BE;                /* Looking at the data as if it is Big Endian */
   struct PACK_MID      //lint !e129
   {
      uint16_BA Lsw;
      uint16_BA Msw;
   } Word;           /* Native value */
   uint32_t n32;       /* Native 32-bit value */
   BitStruct32 bits; /* Bit Access */
}uint32_BA;          /* Bit Addressable uint32_t */
PACK_END

typedef union
{
   uint64_t u64;
   int64_t  i64;
}var64_SI;

PACK_BEGIN
typedef union  PACK_MID /* Used for the math operations */
{
   struct PACK_MID
   {
      uint32_BA Msdw;
      uint32_BA Lsdw;
   }BE;
   struct PACK_MID
   {
      uint32_BA Lsdw;
      uint32_BA Msdw;
   }LE;
   struct PACK_MID
   {
      uint32_BA Lsdw;
      uint32_BA Msdw;
   }Dword;
//   BitStruct64 Bits;
   uint64_t      uVal;         /* unsigned 64-bit value */
   int64_t       sVal;         /* signed 64-bit value */
   uint8_t       aVal[8];      /* Char array, used for signed to unsigned conversions */
}uint64_BA;
PACK_END

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

#endif
