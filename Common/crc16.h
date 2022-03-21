/**********************************************************************************************************************

  Filename: crc16.h

  Global Designator: GENCRC_gencrc

  Contents: Sofware crc16 generator from TWACS

 **********************************************************************************************************************
  A product of
  Aclara Technologies LLC
  Confidential and Proprietary
  Copyright YearOfCreation-CurrentYear Aclara.  All Rights Reserved.

  PROPRIETARY NOTICE
  The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
  (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
  authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
  be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************

   @author

 ****************************************************************************************************************** */
#ifndef CRC16_H
#define CRC16_H
/* INCLUDE FILES */

/* ****************************************************************************************************************** */

/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */

/* TYPE DEFINITIONS */
typedef union PACK_MID
{
   BitStruct   Bits;
   uint8_t     Byte;
} BitAddressableChar;

PACK_BEGIN

typedef union PACK_MID
{
   struct PACK_MID
   {
      uint8_t Lsb;
      uint8_t Msb;
   } Bytes;  /* Native value */
   struct PACK_MID
   {
      BitAddressableChar LSB;
      BitAddressableChar MSB;
   } bitBytes;  /* Native value */

   uint16_t Word; /* Native 16-bit value */
} uint16_BAT;   /* Byte Addressable uint16 */
PACK_END

typedef union PACK_MID
{
   struct PACK_MID
   {
      uint16_BAT Lsw;
      uint16_BAT Msw;
   } Words;           /* Native value */
   uint32_t Dword; /* Native 32-bit value */
}WordAddressableLong;   /* Bit Addressable uint32 */

/* ****************************************************************************************************************** */

/* CONSTANTS */

/* ****************************************************************************************************************** */

/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */

/* FUNCTION PROTOTYPES */
uint16_t GENCRC_gencrc ( uint8_t *pCrcBuf, uint8_t ucLength );

#endif /* CRC16_H */