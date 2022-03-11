/**********************************************************************************************************************

  Filename: crc16_PHY.h

  Global Designator: CRC16PHY_

  Contents: Software CRC16 calculator for PHY header

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright 2022- Aclara.  All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************

   @author

 ****************************************************************************************************************** */
#ifndef CRC16_PHY_H
#define CRC16_PHY_H
/* INCLUDE FILES */

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
uint16_t CRC16PHY_calculate( uint16_t crc16Polynomial, uint16_t seed, const void *data, size_t length );

#endif /* CRC16_PHY_H */