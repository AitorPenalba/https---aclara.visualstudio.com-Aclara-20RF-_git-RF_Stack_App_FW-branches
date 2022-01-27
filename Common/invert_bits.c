/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:   invert_bits.c
 *
 * Global Designator: INVB_
 *
 * Contents: Inverts each bit in an array of bytes
 *
 ***********************************************************************************************************************
 * Copyright (c) 2011 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created Jul 5, 2011
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 07/05/2011 - Initial Release
 *
 **********************************************************************************************************************/
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define invert_bits_GLOBAL
#include "invert_bits.h"
#undef  invert_bits_GLOBAL

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************
 *
 * Function Name: INVB_invertBits
 *
 * Purpose: Inverts each bit in an array of bytes
 *
 * Arguments: uint8_t *pData - Data to invert, numBytes - number of bytes to invert
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
void INVB_invertBits(uint8_t *pData, uint32_t numBytes)
{
   while(numBytes--)
   {
      *pData++ ^= 0xFF;
   }
}
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function Name: INVB_memcpy
 *
 * Purpose: Copies an array of bytes from the source to the destination and inverts each byte
 *
 * Arguments: uint8_t *pData - Data to invert, numBytes - number of bytes to invert
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
#ifndef __BOOTLOADER
void INVB_memcpy(uint8_t *pDest, uint8_t const *pSrc, uint32_t numBytes)
{
   while(numBytes--)
   {
      *pDest++ = *pSrc++ ^ 0xFF;
   }
}
#endif
/* ****************************************************************************************************************** */
