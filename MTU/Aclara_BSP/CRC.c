/******************************************************************************
 *
 * Filename: CRC.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2013 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <mqx.h>
#include <bsp.h>
#include "error_codes.h"
#include "crc.h"
#include "crc_kn.h"
#include "OS_aclara.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
static OS_MUTEX_Obj CRC_Mutex;

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: CRC_initialize

  Purpose: This function is used to initialize the Kinetis CRC module so it will work

  Arguments:

  Returns: returnStatus_t

  Notes:

*******************************************************************************/
returnStatus_t CRC_initialize ( void )
{
   (void)CRC_init();
   if ( OS_MUTEX_Create(&CRC_Mutex) == false )
   {
      (void)printf ( "ERROR - OS_MUTEX_Create(&CRC_Mutex) failed" );
      return(eFAILURE);
   } /* end if() */
   return(eSUCCESS);
} /* end CRC_initialize () */

#ifndef __BOOTLOADER
/*******************************************************************************

  Function name: CRC_16_Calculate

  Purpose: Calculate a 16 bit CRC value for the passed in Data string

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it)

  Returns: The CRC value that was calculated

  Notes:

*******************************************************************************/
uint16_t CRC_16_Calculate ( uint8_t *Data, uint32_t Length )
{
   uint16_t CRC16_Result;

   OS_MUTEX_Lock(&CRC_Mutex); // Function will not return if it fails

   (void)CRC_Config( 0x00001021, 0, 0, 0, 0 );
   CRC16_Result = (uint16_t)CRC_Cal_16 ( 0x00000000, Data, Length );

   OS_MUTEX_Unlock(&CRC_Mutex); // Function will not return if it fails

   return ( CRC16_Result );
} /* end CRC_16_Calculate () */

/*******************************************************************************

  Function name: CRC_16_DcuHack

  Purpose: Calculate a 16 bit CRC value for the passed in Data string using the polynomial specified by DCU hack
      document.

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it)

  Returns: The CRC value that was calculated

  Notes:  Could eventually be merged into a generic function to handle passed in polynomial

*******************************************************************************/
uint16_t CRC_16_DcuHack ( uint8_t *Data, uint32_t Length )
{
   uint16_t CRC16_Result;

   OS_MUTEX_Lock(&CRC_Mutex); // Function will not return if it fails

   (void)CRC_Config( 0x00001021, 0, 0, 0, 0 );
   CRC16_Result = (uint16_t)CRC_Cal_16 ( 0x0000FFFF, Data, Length );

   OS_MUTEX_Unlock(&CRC_Mutex); // Function will not return if it fails

   return ( CRC16_Result );
} /* end CRC_16_Calculate () */

/*******************************************************************************

  Function name: CRC_16_PhyHeader

  Purpose: Calculate a 16 bit CRC value for the phy header.

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it)

  Returns: The CRC value that was calculated

  Notes:  Could eventually be merged into a generic function to handle passed in polynomial
          The CRC is not computed as expected when the seed is 0xFFFF
          We get the expected result with a seed of 0x98FD

*******************************************************************************/
uint16_t CRC_16_PhyHeader ( uint8_t *Data, uint32_t Length )
{
   uint16_t CRC16_Result;

   OS_MUTEX_Lock(&CRC_Mutex); // Function will not return if it fails

   (void)CRC_Config( 0x000053EB, 0, 0, 0, 0 );
   CRC16_Result = (uint16_t)CRC_Cal_16 ( 0x000098FD, Data, Length );

   OS_MUTEX_Unlock(&CRC_Mutex); // Function will not return if it fails

   return ( CRC16_Result );
} /* end CRC_16_Calculate () */
#endif   /* BOOTLOADER  */

/*******************************************************************************

  Function name: CRC_32_Calculate

  Purpose: Calculate a 32 bit CRC value for the passed in Data string

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it

  Returns: The CRC value that was calculated

  Notes:

*******************************************************************************/
uint32_t CRC_32_Calculate ( uint8_t *Data, uint32_t Length )
{
   uint32_t CRC32_Result;

   OS_MUTEX_Lock(&CRC_Mutex); // Function will not return if it fails

   (void)CRC_Config( 0xF4ACFB13, 0, 0, 0, 1 );
   CRC32_Result = CRC_Cal_32 ( 0xFDBB3209, Data, Length );

   OS_MUTEX_Unlock(&CRC_Mutex); // Function will not return if it fails

   return ( CRC32_Result );
} /* end CRC_32_Calculate () */

#ifndef __BOOTLOADER
/*******************************************************************************

  Function name: CRC_ecc108_crc

  Purpose: Calculate a 16 bit CRC that is compatible with the ECCx08 security chip

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it
             crc - pointer to where to place results of crc calculation..
             seed - seed value of CRC.
             KeepLock - if set, don't release the mutex at the completion of the calculation - allows multiple
             uninterrupted calls.

  Returns: void

  Notes:

*******************************************************************************/
void CRC_ecc108_crc ( uint32_t length, uint8_t *data, uint8_t *crc, uint32_t seed, bool KeepLock )
{
   uint16_t crc_register;
   static bool gotMutex = false;

   if ( !gotMutex )
   {
      OS_MUTEX_Lock(&CRC_Mutex); // Function will not return if it fails
      gotMutex = true;
   }

   (void)CRC_Config( 0x8005, 1, 0, 0, 0 );   /*lint !e456 mutex unlocked when full CRC complete */
   crc_register = (uint16_t)CRC_Cal_16( seed, data, length );

   crc[0] = (uint8_t) (crc_register & 0x00FF);
   crc[1] = (uint8_t) (crc_register >> 8);

   if ( !KeepLock )
   {
      OS_MUTEX_Unlock(&CRC_Mutex); // Function will not return if it fails
      gotMutex = false;
   }
} /*lint !e454 !e456 mutex unlocked when full CRC complete  */
#endif   /* BOOTLOADER  */
/* end CRC_ecc108_crc() */
