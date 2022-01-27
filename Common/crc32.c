/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:	crc32.c
 *
 * Global Designator: CRC32_
 *
 * Contents: This CRC32 function conforms to the CRC32 CCITT Standards
 *
 ***********************************************************************************************************************
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
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created July 24, 2004
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 * v0.2 - Changed code to use 'CHAR_BIT' from limits.h
 * v0.3 - Modified for system 2014
 ******************************************************************************************************************** */

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "crc32.h"

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function name: CRC32_init()
 *
 * Purpose: Initializes the CRC meta data so that the CRC calculation function can be called.
 *
 * Arguments: uint32_t startVal, uint32_t poly, eCRC32_Result_t invertResult, crc32Cfg_t *pCrcCfg
 *
 * Returns: None
 *
 * Re-entrant Code: Yes
 *
 * Notes: Call this before starting a new crc calculation
 *
 **********************************************************************************************************************/
void CRC32_init(uint32_t startVal, uint32_t poly, eCRC32_Result_t invertResult, crc32Cfg_t *pCrcCfg)
{
   pCrcCfg->crcVal = startVal;
   pCrcCfg->poly = poly;
   pCrcCfg->invertResult = invertResult;
}
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Function name: CRC32_calc
 *
 * Purpose: Calculates the CRC on an array of data.  This module can be called over and over again to perform CRC checks
 *          on very large amounts of data.
 *
 * Arguments: uint8_t *pData, uint32_t cnt, crc32Cfg_t *pCrcCfg
 *
 * Returns: uint32_t - Generated CRC32 Value
 *
 * Re-entrant Code: Yes
 *
 * Notes: This function may be called over and over again until all of the data has been processed.  Note:  The calling
 *        function should not modify the contents of "crc32Cfg_t *pCrcCfg"
 *
 **********************************************************************************************************************/
uint32_t CRC32_calc(void const *pData, uint32_t cnt, crc32Cfg_t *pCrcCfg)
{
   uint8_t *pDataVal = (uint8_t *)pData;
   if (cnt)
   {
      do
      {
         uint8_t index = 8;
         uint8_t dataByte = *pDataVal++;
         do
         {
            if ((pCrcCfg->crcVal & 1) ^ (dataByte & 1))
            {
               pCrcCfg->crcVal >>= 1;
               pCrcCfg->crcVal ^= pCrcCfg->poly;
            }
            else
               pCrcCfg->crcVal >>= 1;
            dataByte >>= 1;
         }while(--index);
      }while(--cnt);
   }

   uint32_t calCrc;
   if (pCrcCfg->invertResult)
   {
      calCrc = ~pCrcCfg->crcVal;
   }
   else
   {
      calCrc = pCrcCfg->crcVal;
   }
   return(calCrc);
}
/* ****************************************************************************************************************** */
