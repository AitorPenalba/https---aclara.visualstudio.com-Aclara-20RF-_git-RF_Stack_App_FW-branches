/******************************************************************************
 *
 * Filename: crc16_m.c
 *
 * Global Designator: CRC16_M_
 *
 * Contents: meter specific CRC routines.
 *
 * CRC 16 implementation reference for C12.18-1996 and C12.21-19xx
 *
 ******************************************************************************
 * Copyright (c) 2001-2011 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ******************************************************************************
 *
 * $Log$
 *
 *  Created 07/20/07     KAD  Created
 *
 ******************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 *
 *************************************************************************** */
/* ************************************************************************* */
/* INCLUDE FILES */
#include "project.h"
#include "crc16_m.h"

/* ************************************************************************* */
/* CONSTANTS */

#define MTR_CNST  0x8408

/* ************************************************************************* */
/* MACRO DEFINITIONS */

/* ************************************************************************* */
/* TYPE DEFINITIONS */

/* ************************************************************************* */
/* FILE VARIABLE DEFINITIONS */

static uint16_BA _crc16;

/* ************************************************************************* */
/* FUNCTION PROTOTYPES */

/* ************************************************************************* */
/* FUNCTION DEFINITIONS */

/* ************************************************************************* */
/*lint -esym(714,CRC16_CalcMtr,CRC16_InitMtr,CRC16_ResultMtr)  functions defined but not referenced */
/******************************************************************************
 *
 * Function name: CRC16_Calc
 *
 * Purpose: Calculates the CRC16 a byte at a time
 *
 * Arguments: uint8_t ByteToProcess
 *
 * Returns: void
 *
 *****************************************************************************/
void CRC16_CalcMtr( uint8_t ucByte )
{
   uint8_t i = 8;
   uint8_t ucTest;

   ucTest = ucByte;

   do
   {
      if (_crc16.Byte.Lsb & 0x01)
      {
         _crc16.n16 >>= 1;            /* Rotate it all right */
         if (ucTest & 1)              /* Check if LS bit is set */
         {
            _crc16.Byte.Msb |= 0x80;  /* Check if MS bit is set */
         }
         _crc16.n16 ^= MTR_CNST;      /* 0x1021 reflected = 1000 0100 0000 1000 */
      }
      else
      {
         _crc16.n16 >>= 1;            /* Rotate it all right */
         if (ucTest & 1)              /* Check if LS bit is set */
         {
            _crc16.Byte.Msb |= 0x80;  /* Check if MS bit is set */
         }
      }
      ucTest >>= 1;
   }while(--i);
}

/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: CRC16_Init
 *
 * Purpose: Initialize the CRC
 *
 * Arguments: uint8_t *
 *
 * Returns: void
 *
 *****************************************************************************/
void CRC16_InitMtr(uint8_t const *pChars)
{
   _crc16.Byte.Lsb = ( uint8_t )~*pChars;
   _crc16.Byte.Msb = ( uint8_t )~*(pChars + 1);
}
/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: CRC16_Result
 *
 * Purpose: Returns the result of the CRC16
 *
 * Arguments: None
 *
 * Returns: uint16_t CRC16
 *
 *****************************************************************************/
uint16_t CRC16_ResultMtr(void)
{
   return ( uint16_t )~_crc16.n16;   /* Invert the CRC result */
}
/* ************************************************************************* */
