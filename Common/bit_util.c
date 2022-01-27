/* ************************************************************************* */
/******************************************************************************
 *
 * Filename:   bit_util.c
 *
 * Global Designator: BU_
 *
 * Contents: BR_BitRev prototype
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
 * $Log$ MRO created 11/07/12
 *
 ******************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 * v0.2 - Changed filename and added BIT_getBit() function
 *************************************************************************** */

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"

#define BU_GLOBALS
#include "bit_util.h"
#undef BU_GLOBALS

#ifdef USE_MAPPING
#pragma psect text=bstxt
#pragma psect const=bsconst
#pragma psect mconst=bsmconst
#pragma psect nearidata=b24nidata
#pragma psect neardata=b24ndata
#pragma psect nearbss=blbss
#pragma psect bss=blbbss
#pragma psect bitbss=blbitbss
#endif

/* ************************************************************************* */
/* CONSTANTS */

/* ************************************************************************* */
/* MACRO DEFINITIONS */

/* ************************************************************************* */
/* TYPE DEFINITIONS */

/* ************************************************************************* */
/* FILE VARIABLE DEFINITIONS */

/* ************************************************************************* */
/* FUNCTION PROTOTYPES */

/* ************************************************************************* */
/* FUNCTION DEFINITIONS */

/******************************************************************************
 *
 * Function name: BU_BitRev()
 *
 * Purpose: Reverse the bits of a variable bit length word
 *
 * Arguments: Word, NumBits
 *
 * Returns: Nothing
 *
 * Reentrant: No
 *
 *****************************************************************************/
void BU_BitRev(far uint32_t *Word, uint8_t NumBits)
{
   uint32_t BitMask = 1;         //Set initial bit mask to 1
   uint32_BA LsbWord;  //Make copy of word to be bit reversed for Lsb comparison
   uint32_t BitRevWord = 0;      //Bit reversed word
   uint16_t ShiftCnt;           //Shift counter for bit reverse procedure

   LsbWord.n32 = *Word;
   BitMask <<= (NumBits - 1);     //Create bit mask

   for(ShiftCnt = 0; ShiftCnt < NumBits; ShiftCnt++)
   {
      if( LsbWord.Word.Lsw.Byte.Lsb & 1 ) //Is Lsb equal to one
      {
         BitRevWord ^= BitMask;
      }
      BitMask >>= 1;
      LsbWord.n32 >>= 1;
   }
   *Word = BitRevWord;
   return;
}
/* ************************************************************************* */

/******************************************************************************
 *
 * Function name: BU_getBit()
 *
 * Purpose: Get a specific bit from an array
 *
 * Arguments: void *pLoc, uint8_t bytesInArray, uint16_t bitNum
 *
 * Returns: int8_t = -1 invalid request, 0 or 1.
 *
 * Reentrant: yes
 *
 *****************************************************************************/
int8_t BU_getBit(void *pLoc, uint8_t bytesInArray, uint16_t bitNum)
{
   int8_t retVal = BU_ERROR;    /* Set to invalid */

   if ((bitNum / 8) <= bytesInArray)
   {
      uint8_t *pData = (uint8_t *)pLoc;
      /* Calculate offset into pLoc */
      retVal = 0;
      if ( pData[bitNum / 8] & (1 << (bitNum % 8)) )
      {
         retVal = 1;
      }
   }
   return(retVal);
}
/* ************************************************************************* */
