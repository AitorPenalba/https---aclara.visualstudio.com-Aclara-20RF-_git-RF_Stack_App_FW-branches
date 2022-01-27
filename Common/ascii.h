/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: ascii.h
 *
 * Contents: Prototypes for conversion utilities for ascii to binary and binary to ascii
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
#ifndef ascii_H_
#define ascii_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "error_codes.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef ascii_GLOBALS
   #define ascii_EXTERN
#else
   #define ascii_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * ASCII_atoh - Converts a byte of ASCII to one nibble0 of hex.  For example:  ASCII 0x31 is 0x1 in Hex.
 *
 * @param  uint8_t *pByte - Pointer to byte to convert.
 * @return returnStatus_t - eSUCCESS or eFAILURE
 */
returnStatus_t ASCII_atoh(uint8_t *pByte);

/**
 * ASCII_atohByte - Converts 2 bytes of ASCII to one byte of hex.  For example:  ASCII (0x31, 0x41) is 0x1A in Hex.
 *
 * @param  uint8_t upper - upper nibble byte to convert.
 *         uint8_t lower - lower nibble byte to convert.
 *         uint8_t *result - Pointer to resulting converted byte.
 * @return returnStatus_t - eSUCCESS or eFAILURE
 */
returnStatus_t ASCII_atohByte(uint8_t upper, uint8_t lower, uint8_t *result);


/***********************************************************************************************************************
Function name: ASCII_atohU16

Purpose:  Converts 4 consecutive bytes of ASCII to one uint16_t of hex.  For example:  ASCII (0x31, 0x41, 0x36, 0x46) is 0x1A6F in Hex.

Arguments: pSource_u8  - Pointer to most significant nibble byte to convert.
           pResult_u16 - Pointer to resulting converted uint16_t.

Returns: returnStatus_t - eSUCCESS if the bytes passed are valid ASCII, if eSUCCESS is returned result will contain
                          converted byte.  if eFAILURE is returned, result value is undefined
**********************************************************************************************************************/
returnStatus_t ASCII_atohU16 ( uint8_t const *pSource_u8, uint16_t *pResult_u16);


/***********************************************************************************************************************
Function name: ASCII_atohU32

Purpose:  Converts 8 consecutive bytes of ASCII to one uint32_t of hex.  For example:  ASCII (0x31, 0x41, 0x36, 0x46, 0x30, 0x32, 0x39, 0x42 ) is 0x1A6F029B in Hex.

Arguments: pSource_u8  - Pointer to most significant nibble byte to convert.
           pResult_u32 - Pointer to resulting converted uint32_t.

Returns: returnStatus_t - eSUCCESS if the bytes passed are valid ASCII, if eSUCCESS is returned result will contain
                          converted byte.  if eFAILURE is returned, result value is undefined
**********************************************************************************************************************/
returnStatus_t ASCII_atohU32 ( uint8_t const *pSource_u8, uint32_t *pResult_u32);

/**
 * ASCII_htoa - Converts a byte of hex to two bytes of ASCII
 *
 * @param  uint8_t *pDest - Pointer to store the 2-bytes of data.
 * @param  uint8_t src  -   source data (1-byte of data to convert)
 * @return None
 */
void ASCII_htoa(uint8_t *pDest, uint8_t src);

/* ****************************************************************************************************************** */
#undef ascii_EXTERN

#endif
