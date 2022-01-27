// <editor-fold defaultstate="collapsed" desc="File Header Information">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:   ascii.c
 *
 * Global Designator: ASCII_
 *
 * Contents: Conversion utilities for ascii to binary and binary to ascii
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2014 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created May 2, 2014
 *
 ***********************************************************************************************************************
 * Revision History:
 * 05/02/2014 - Initial Release
 *
 * #since      2014-05-02
 **********************************************************************************************************************/
 // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#define ascii_GLOBALS
#include "ascii.h"
#undef  ascii_GLOBALS

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Constants">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Variables (File Static)">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************
 *
 * Function name: ASCII_atoh
 *
 * Purpose:  Converts a byte of ASCII to one nibble0 of hex.  For example:  ASCII 0x31 is 0x1 in Hex
 *
 * Arguments: uint8_t *pByte - Pointer to byte to convert.  Will be overwritten to store the converted ASCII Byte
 *
 * Returns: returnStatus_t - eSUCCESS if the byte passed in is valid ASCII
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t ASCII_atoh(uint8_t *pByte)
{
   returnStatus_t retVal = eFAILURE;

   /* 1st Validate the byte is in range */
   if ((*pByte >= '0') && (*pByte <= '9'))
   {
      *pByte -= '0';
      retVal = eSUCCESS;
   }
   else if ((*pByte >= 'A') && (*pByte <= 'F'))
   {
     *pByte -= ('A'-10);
     retVal = eSUCCESS;
   }
   else if ((*pByte >= 'a') && (*pByte <= 'f'))
   {
     *pByte -= ('a'-10);
     retVal = eSUCCESS;
   }
   return(retVal);
}
/* ****************************************************************************************************************** */

/***********************************************************************************************************************
 *
 * Function name: ASCII_atohByte
 *
 * Purpose:  Converts 2 bytes of ASCII to one byte of hex.  For example:  ASCII (0x31, 0x41) is 0x1A in Hex.
 *
 * Arguments: upper - Pointer to upper nibble byte to convert.
 *            lower - lower nibble byte to convert.
 *            result - Pointer to resulting converted byte.
 *
 * Returns: returnStatus_t - eSUCCESS if the byte passed in is valid ASCII, if eSUCCESS is returned result will contain
 *                converted byte.  if eFAILURE is returned, result value is undefined
 *
 * Side Effects: None
 *
 * Reentrant Code: yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t ASCII_atohByte(uint8_t upper, uint8_t lower, uint8_t *result)
{
   returnStatus_t retVal = eFAILURE;
   uint8_t tempByte;

   tempByte = upper;
   if (ASCII_atoh(&tempByte) == eSUCCESS)
   {
      *result = (uint8_t)(tempByte << 4);
      tempByte = lower;
      if (ASCII_atoh(&tempByte) == eSUCCESS)
      {
         *result += tempByte;
         retVal = eSUCCESS;
      }
   }
   return(retVal);
}
/* ****************************************************************************************************************** */


/***********************************************************************************************************************
Function name: ASCII_atohU16

Purpose:  Converts 4 consecutive bytes of ASCII to one uint16_t of hex.  For example:  ASCII (0x31, 0x41, 0x36, 0x46) is 0x1A6F in Hex.

Arguments: pSource_u8  - Pointer to most significant nibble byte to convert.
           pResult_u16 - Pointer to resulting converted uint16_t.

Returns: returnStatus_t - eSUCCESS if the bytes passed are valid ASCII, if eSUCCESS is returned result will contain
                          converted byte.  if eFAILURE is returned, result value is undefined
**********************************************************************************************************************/
returnStatus_t ASCII_atohU16 ( uint8_t const *pSource_u8, uint16_t *pResult_u16)
{
   returnStatus_t retVal = eFAILURE;
   uint8_t temp_u8 = 0;
   uint16_t temp_u16 = 0;

   /* Convert  byte 1, i.e. the most significant byte */
   if ( eSUCCESS == ASCII_atohByte( *pSource_u8, *( pSource_u8 + 1 ), &temp_u8 ) )
   {
      temp_u16 += (uint16_t)( ( (uint16_t) temp_u8 ) << 8 ) & 0xFF00;

      /* Convert  byte 2, i.e. the least significant byte */
      if ( eSUCCESS == ASCII_atohByte( *( pSource_u8 + 2 ), *( pSource_u8 + 3 ), &temp_u8 ) )
      {
         temp_u16 +=  ( (uint16_t) temp_u8 ) & 0x00FF;
         *pResult_u16 = temp_u16;
         retVal = eSUCCESS;
      }
   }

   return retVal;
}


/***********************************************************************************************************************
Function name: ASCII_atohU32

Purpose:  Converts 8 consecutive bytes of ASCII to one uint32_t of hex.  For example:  ASCII (0x31, 0x41, 0x36, 0x46, 0x30, 0x32, 0x39, 0x42 ) is 0x1A6F029B in Hex.

Arguments: pSource_u8  - Pointer to most significant nibble byte to convert.
           pResult_u32 - Pointer to resulting converted uint32_t.

Returns: returnStatus_t - eSUCCESS if the bytes passed are valid ASCII, if eSUCCESS is returned result will contain
                          converted byte.  if eFAILURE is returned, result value is undefined
**********************************************************************************************************************/
returnStatus_t ASCII_atohU32 ( uint8_t const *pSource_u8, uint32_t *pResult_u32)
{
   returnStatus_t retVal = eFAILURE;
   uint8_t temp_u8 = 0;
   uint32_t temp_u32 = 0;

   /* Convert  byte 1, i.e. the most significant byte */
   if ( eSUCCESS == ASCII_atohByte( *pSource_u8, *( pSource_u8 + 1 ), &temp_u8 ) )
   {
      temp_u32 += ( ( (uint32_t) temp_u8 ) << 24 ) & 0xFF000000;

      /* Convert  byte 2 */
      if ( eSUCCESS == ASCII_atohByte( *( pSource_u8 + 2 ), *( pSource_u8 + 3 ), &temp_u8 ) )
      {
         temp_u32 +=  ( ( (uint32_t) temp_u8 ) << 16 ) & 0x00FF0000;

         /* Convert  byte 3 */
         if ( eSUCCESS == ASCII_atohByte( *( pSource_u8 + 4 ), *( pSource_u8 + 5 ), &temp_u8 ) )
         {
            temp_u32 += ( ( (uint32_t) temp_u8 ) << 8 ) & 0x0000FF00;

            /* Convert  byte 4, i.e. the least significant byte */
            if ( eSUCCESS == ASCII_atohByte( *( pSource_u8 + 6 ), *( pSource_u8 + 7 ), &temp_u8 ) )
            {
               temp_u32 += ( (uint32_t) temp_u8 ) & 0x000000FF;
               *pResult_u32 = temp_u32;
               retVal = eSUCCESS;
            }
         }
      }
   }

   return retVal;
}


/***********************************************************************************************************************
 *
 * Function name: ASCII_htoa
 *
 * Purpose:  Converts a byte of hex to two bytes of ASCII
 *
 * Arguments: uint8_t *pDest, uint8_t src
 *
 * Returns: None
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
void ASCII_htoa(uint8_t *pDest, uint8_t src)
{
   uint8_t nibble;

   nibble = (src & 0xF0) >> 4; /* Get the MS nibble */
   if (nibble <= 9)
   {
      pDest[0] = nibble + '0';
   }
   else
   {
     pDest[0] = nibble + ('A'-10);
   }

   nibble = src & 0x0F; /* Get the LS nibble */
   if (nibble <= 9)
   {
      pDest[1] = nibble + '0';
   }
   else
   {
      pDest[1] = nibble + ('A'-10);
   }
}
/* ****************************************************************************************************************** */
