/******************************************************************************
 *
 * Filename: CRC.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2015 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
#ifndef CRC_H
#define CRC_H
/* INCLUDE FILES */
#include "error_codes.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/*******************************************************************************

  Function name: CRC_initialize

  Purpose: This function is used to initialize the Kinetis CRC module so it will work

  Arguments:

  Returns: returnStatus_t

  Notes:

*******************************************************************************/
returnStatus_t CRC_initialize ( void );

/*******************************************************************************

  Function name: CRC_16_Calculate

  Purpose: Calculate a 16 bit CRC value for the passed in Data string

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it)

  Returns: The CRC value that was calculated

  Notes:

*******************************************************************************/
extern uint16_t CRC_16_Calculate ( uint8_t *Data, uint32_t Length );

/*******************************************************************************

  Function name: CRC_16_DcuHack

  Purpose: Calculate a 16 bit CRC value for the passed in Data string using the polynomial specified by DCU hack
      document.

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it)

  Returns: The CRC value that was calculated

  Notes:  Could eventually be merged into a generic function to handle passed in polynomial

*******************************************************************************/
extern uint16_t CRC_16_DcuHack ( uint8_t *Data, uint32_t Length );

/*******************************************************************************

  Function name: CRC_16_PhyHeader

  Purpose: Calculate a 16 bit CRC value for the phy header.

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it)

  Returns: The CRC value that was calculated

  Notes:  Could eventually be merged into a generic function to handle passed in polynomial
          The CRC is not computed as expected when the seed is 0xFFFF
          We get the expected result with a seed of 0x280A

*******************************************************************************/
extern uint16_t CRC_16_PhyHeader ( uint8_t *Data, uint32_t Length );

/*******************************************************************************

  Function name: CRC_32_Calculate

  Purpose: Calculate a 32 bit CRC value for the passed in Data string

  Arguments: Data - Byte pointer to the beginning of the Data string
             Length - the length in bytes of the Data string (that is to have CRC calculated on it

  Returns: The CRC value that was calculated

  Notes:

*******************************************************************************/
extern uint32_t CRC_32_Calculate ( uint8_t *Data, uint32_t Length );

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
extern void CRC_ecc108_crc ( uint32_t length, uint8_t *data, uint8_t *crc, uint32_t seed, bool KeepLock );

#endif
