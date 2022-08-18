/* ************************************************************************* */
/******************************************************************************
 *
 * Filename:   byteswap.c
 *
 * Global Designator: BS
 *
 * Contents: Swaps bytes of a string for little to big endian converstions
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
 * $Log$ KAD Created 100604
 *
 ******************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 * v0.2 - Added String swap function
 * v0.3 - Found a whammy and fixed it.  The middle bytes were not swapping correctly
 *        in the Byte_Swap_BL function.
 * v0.4 - Removed "BL" from the function names.
 * v0.5 - AG Initialized ucBSDSCount to zero in Byte_Swap_Data_String().  Added a
 *        wrapper for Byte_Swap2
 *************************************************************************** */

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project_BL.h"
#include "byteswap_BL.h"

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
 * Function name: Byte_Swap()
 *
 * Purpose: Swaps bytes in a string.  Used to convert from big/little endian.
 *
 * Arguments: None
 *
 * Returns: None
 *
 *****************************************************************************/
void Byte_Swap(uint8_t *sBSStringToConvert, uint8_t ucBSNumOfBytes)
{
   uint8_t ucBSCount;
   uint8_t ucBSTemp;

   if ( 0 != ucBSNumOfBytes )
   {
      ucBSNumOfBytes--;
      for (ucBSCount = 0; ucBSCount < ucBSNumOfBytes; ucBSNumOfBytes--, ucBSCount++)
      {
         ucBSTemp = sBSStringToConvert[ucBSCount];
         sBSStringToConvert[ucBSCount] = sBSStringToConvert[ucBSNumOfBytes];
         sBSStringToConvert[ucBSNumOfBytes] = ucBSTemp;
      }
   }
}

#if ( RTOS_SELECTION == FREE_RTOS ) /* FREE_RTOS */
/******************************************************************************
 *
 * Function name: htonx_FreeRTOS
 *
 * Purpose: Swaps bytes in a string.  Uses internal Byte_Swap mechanism
 *
 * Arguments: uint32_t value, uint8_t numOfBytes
 *
 * Returns: uint32_t
 *
 *****************************************************************************/
uint32_t htonx_FreeRTOS( uint32_t value, uint8_t numOfBytes )
{
   uint8_t *byteSwapVal = ( uint8_t * ) &value;
   Byte_Swap( byteSwapVal, numOfBytes );
   return value;
}

#endif

/******************************************************************************
 *
 * Function name: Byte_Swap_Data_String()
 *
 * Purpose: Swaps bytes in a string.  Used to convert from big/little endian.
 *
 * Arguments: None
 *
 * Returns: None
 *
 *****************************************************************************/
#if 0
void Byte_Swap_Data_String(far uint8_t *sBSDSStringToConvert, uint8_t BSDSElements, uint8_t ucBSDSElementSize)
{
   uint8_t ucBSDSCount = 0;

   do
   {
      Byte_Swap(&sBSDSStringToConvert[ucBSDSCount * ucBSDSElementSize], ucBSDSElementSize);
      ucBSDSCount++;
   }while (--BSDSElements);
}
#endif
/* ************************************************************************* */
