/******************************************************************************
 * Compiler_types.h -
 ******************************************************************************
 * ORIGIN DATE:  November 21, 2014
 * TESTBENCH:
 * AUTHOR:       Mario Deschenes
 * E-MAIL:       mdeschenes@aclaratech.com
 *****************************************************************************/

#ifndef _COMPILER_TYPES_H_
#define _COMPILER_TYPES_H_

#include <stdint.h>

#define U8  uint8_t
#define U16 uint16_t
#define U32 uint32_t

#define S8  int8_t
#define S16 int16_t
#define S32 int32_t

typedef union UU16
{
    U16 U16;
    S16 S16;
    U8 U8[2];
    S8 S8[2];
} UU16;

typedef union UU32
{
    U32 U32;
    S32 S32;
    UU16 UU16[2];
    U16 U16[2];
    S16 S16[2];
    U8 U8[4];
    S8 S8[4];
} UU32;

#endif /* _COMPILER_TYPES_H_ */
