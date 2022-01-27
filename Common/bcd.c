/* ************************************************************************* */
/******************************************************************************
 *
 * Filename:	bcd.c                       int8
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ******************************************************************************
 *
 * $Log$	KAD Created	062804
 *
 ******************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 * v0.2 - Changed a comment
 * v0.3 - Added bcd header file
 *
 *************************************************************************** */
/* PSECTS */
#ifdef USE_MAPPING
   #pragma psect text=utiltxt
   #pragma psect const=utilcnst
   #pragma psect mconst=utilmconst
   #pragma psect nearbss=utilbss
   #pragma psect bss=utilfbss
#endif
/* ************************************************************************* */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"
#include "bcd.h"

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

/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: BCD_2_HEX
 *
 * Purpose: Converts BCD to Hex
 *
 * Arguments: Value to Convert
 *
 * Returns: Hex value
 *
 *****************************************************************************/
uint8_t BCD_2_HEX(uint8_t ucB2HValue)
{
   return((uint8_t)( (( (uint8_t)((uint8_t)((uint8_t)(ucB2HValue & 0xF0) >> 4)) * 10)) + (uint8_t)(ucB2HValue & 0x0f)) );
}
/* ************************************************************************* */
/******************************************************************************
 *
 * Function name: HEX_2_BCD
 *
 * Purpose: Converts a hex value to BCD
 *
 * Arguments: Value to Convert
 *
 * Returns: Hex value
 *
 *****************************************************************************/
uint8_t HEX_2_BCD(uint8_t ucH2BValue)
{
   uint8_t ucH2BTemp1;
   uint8_t ucH2BTemp2;

   ucH2BTemp1 = (uint8_t)(ucH2BValue / (uint8_t)10);
   ucH2BTemp2 = (uint8_t)(ucH2BValue - ((uint8_t)(ucH2BTemp1 * (uint8_t)10)));
   ucH2BValue = (uint8_t)((uint8_t)(ucH2BTemp1 << (uint8_t)4) | ucH2BTemp2);
   return(ucH2BValue);
}
/* ************************************************************************* */
