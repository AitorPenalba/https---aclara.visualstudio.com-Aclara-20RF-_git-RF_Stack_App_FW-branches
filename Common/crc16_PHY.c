/**********************************************************************************************************************

  Filename: crc16_PHY.c

  Global Designator: CRC16PHY

  Contents: Software CRC16 calculator for PHY header and the reference for the code was taken from:
            https://github.com/plhx/CRC16 and also https://crccalc.com/ was used for development

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2022 Aclara.  All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************

   @author

 ****************************************************************************************************************** */

/* INCLUDE FILES */
#include "project.h"
#include "crc16_PHY.h"

/* ****************************************************************************************************************** */
/* #DEFINE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */

/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */

/* CONSTANTS */

/* ****************************************************************************************************************** */

/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

   Function name: CRC16PHY_calculate

   Purpose: Calculates crc16 for the given Poly and there is no refin/refout support available

   Arguments: uint16_t crc16Polynomial - Polynomial for the crc16 calculation
              uint16_t seed - inital value of the CRC
              const void *data - data buffer
              size_t length  - size of the data buffer

   Returns: calculated CRC16 value

	Notes: this API will be able to calculate for all the crc16 polynomials
               mentioned in: https://crccalc.com/ but RefIn, RefOut and XorOut
               must be false/0

*******************************************************************************/
uint16_t CRC16PHY_calculate( uint16_t crc16Polynomial, uint16_t seed, const void *data, size_t length )
{
   uint16_t calculatedCRC = seed;

   size_t i, j;

   const unsigned char *x = (const unsigned char *)data;
   for( i = 0; i < length; i++ )
   {
       calculatedCRC  ^= x[i] << 8;
       for( j = 0; j < 8; j++ )
       {
           if( calculatedCRC  & 0x8000 )
               calculatedCRC = ( calculatedCRC << 1 ) ^ crc16Polynomial;
           else
               calculatedCRC <<= 1;
       }
   }

   return calculatedCRC;
}
