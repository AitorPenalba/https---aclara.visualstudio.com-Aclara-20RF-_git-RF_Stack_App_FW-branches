/******************************************************************************
 *
 * Filename:            crc16.c/Gencrc.c
 *
 * Global Designator:   gencrc
 *
 * Contents:            Sofware crc16 generator from TWACS.
 *
 ******************************************************************************
 * Copyright (c) 2001-2022 Distribution Control Systems, Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 * 					DISTRIBUTION CONTROL SYSTEMS, INC.
 * 					ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$	Davis created 7/11/02
 *
 *****************************************************************************/
/* INCLUDE FILES */
#include "project.h"
#include	"crc16.h"

/* ************************************************************************* */
/* PSECTS */

/* ************************************************************************* */
/* CONSTANTS */
#define CRC_POLYNOMIAL 0XA001

/* ************************************************************************* */
/* MACRO DEFINITIONS */

/* ************************************************************************* */
/* TYPE DEFINITIONS */

/* ************************************************************************* */
/* FILE VARIABLE DEFINITIONS */

/* ************************************************************************* */
/* FUNCTION PROTOTYPES */
static uint16_t reverseBitsInWord( uint16_t calculatedCRC );

/* ************************************************************************* */
/* FUNCTION DEFINITIONS */


/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: reverseBitsInWord
 *
 * Purpose: Reverse bit in a uint16_t
 *
 * Arguments: uint16_t datatype to be reversed
 *
 * Returns: reversed Word
 *
 *****************************************************************************/
static uint16_t reverseBitsInWord( uint16_t calculatedCRC )
{
   uint16_t reversedData = 0;

   for (int i = 0; i<16; i++)
   {
      if (calculatedCRC & (1<<i))
      {
         reversedData |= 1 << (15-i);
      }
   }

   return reversedData;
}

/******************************************************************************
 *
 * Function name: GENCRC_gencrc
 *
 * Purpose: Generates the TWACS crc16 word.
 *
 * Arguments: Pointer to the array of data, length of that data.
 *
 * Returns: Calculated crc16(Little-Endian).
 *
 *****************************************************************************/
uint16_t GENCRC_gencrc ( uint8_t *pCrcBuf, uint8_t ucLength )
{
   BitAddressableChar   lsb;     /* Used to access the ls bit of the current shift. */
   uint8_t                i;       /* Just a loop counter. */
   WordAddressableLong  crc16;   /* Used as the 24 bit shift register. */

   crc16.Dword = 0;
   if ( ucLength > 0 ) {
      crc16.Words.Lsw.Bytes.Lsb = *pCrcBuf++;
      if ( ucLength > 1 )  {
         crc16.Words.Lsw.Bytes.Msb = *pCrcBuf++;
      }

      do {
         if ( ucLength > 2 )  {
            crc16.Words.Msw.Bytes.Lsb = *pCrcBuf++;
         }
         i = 8;
         do {
            lsb.Byte = crc16.Words.Lsw.Bytes.Lsb;
            crc16.Dword >>= 1;
            if ( lsb.Bits.b0 )   {
               crc16.Words.Lsw.Word ^= CRC_POLYNOMIAL;
            }
         } while ( --i );
      }while ( --ucLength );
   }

   return reverseBitsInWord( crc16.Words.Lsw.Word );
}
/* ************************************************************************* */
