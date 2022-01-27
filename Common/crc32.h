/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: crc32.h
 *
 * Contents: Calculates CRC32_
 *
 * Usage:
 *    1.  Call the init function
 *    2.  Call the calc function as many times as need and use the returned value as the CRC.
 *    Note:  Do not modify the data in the structure:  crc32Cfg_t
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
 * $Log$ Created December 12, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 12/12/2013 - Initial Release
 *
 * @version    0.1
 * #since      2013-07-22
 **********************************************************************************************************************/
#ifndef crc32_H
#define crc32_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define CRC32_DFW_START_VALUE    ((uint32_t)0xffffffff)
#define CRC32_DFW_POLY           ((uint32_t)0xEDB88320)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eCRC32_RESULT_NONINVERT = ((uint8_t)0),
   eCRC32_RESULT_INVERT
}eCRC32_Result_t;

 typedef struct
{
   uint32_t          poly;
   uint32_t          crcVal;
   eCRC32_Result_t   invertResult;
}crc32Cfg_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * CRC32_init - Initializes the CRC meta data so that the CRC calculation function can be called.
 *
 * @param  uint32_t startVal - Start value for the CRC
 * @param  uint32_t poly - Polynomial to be applied in the CRC calculation
 * @param  bool bInvertResult - Invert the bits = true, don't invert the bits when false.
 * @param  crc32Cfg_t *pCrcCfg - Configuration of the CRC (don't modify this!)
 * @return None
 */
void CRC32_init(uint32_t startVal, uint32_t poly, eCRC32_Result_t invertResult, crc32Cfg_t *pCrcCfg);

/**
 * CRC32_calc - Calculates the CRC on an array of data.  This module can be called over and over again to perform CRC
 *              checks on very large amounts of data.
 *
 * @param  uint8_t *pData - Source data for the CRC
 * @param  uint32_t cnt - Number of bytes in pData
 * @param  crc32Cfg_t *pCrcCfg - Configuration of the CRC (don't modify this!)
 * @return uint32_t - Resulting CRC32 (ignore this until the last CRC calculation is called)
 */
uint32_t CRC32_calc(void const *pData, uint32_t cnt, crc32Cfg_t *pCrcCfg);



#endif

