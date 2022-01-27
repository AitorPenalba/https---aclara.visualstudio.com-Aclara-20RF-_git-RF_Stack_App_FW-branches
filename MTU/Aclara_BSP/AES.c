/******************************************************************************
 *
 * Filename: AES.c
 *
 * Global Designator: AES
 *
 * Contents:  The following AES algorithm uses CBC (Cipher Block Chaining) 128-bit encryption.
 *
 * NOTE:  Padding an length that is not a multiple of 16 bytes uses PKCS.  This is NOT tested!!!
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
#include "project.h"
#include "cau_api.h"
#include "string.h"

#ifdef TM_AES_UNIT_TEST
#include "DBG_SerialDebug.h"
#endif /* TM_AES_UNIT_TEST */

/* #DEFINE DEFINITIONS */
#define AES_128_BLOCK_LENGTH        ((uint8_t)128/8) /* 16 */
#define AES_128_KEY_SCHEDULE_SIZE   ((uint8_t)44)    /* size of the expanded key in uint32_t's */
#define AES_128_NUM_ROUNDS          ((uint8_t)10)
#define AES_128                     ((uint8_t)128)   /* Defines 128-bit encryption for MQX interface */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */
//#ifdef TM_AES_UNIT_TEST
/* KTL - 16 byte key: "ultrapassword123" - only used for testing */
const uint8_t AES_128_KEY[AES_128_BLOCK_LENGTH] = { 0x75, 0x6c, 0x74, 0x72, 0x61, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64, 0x31, 0x32, 0x33 };

/* KTL - 16 byte initialization vector: "mysecretpassword" - only used for testing (may also be refered as nonce?) */
const uint8_t AES_128_INIT_VECTOR[AES_128_BLOCK_LENGTH] = { 0x6d, 0x79, 0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64 };
//#endif /* TM_AES_UNIT_TEST */

/* FILE VARIABLE DEFINITIONS */
static OS_MUTEX_Obj AES_Mutex;  /* Serialize access to AES encryption */

/* FUNCTION PROTOTYPES */
#ifdef TM_AES_UNIT_TEST
static void printHexArray ( uint8_t *pStr, uint8_t *pData, uint8_t cnt );
#endif

/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

  Function Name: AES_128_Init

  Purpose: Initializes the mutex - Must be called before AES encryption or decryption is called.

  Arguments: None

  Returns: returnStatus_t

  Side Effects: None

  Re-entrant Code: No

  Notes: This function must be called before any data is encrypted or decrypted.

 **********************************************************************************************************************/
returnStatus_t AES_128_Init ( void )
{
   returnStatus_t retVal = eSUCCESS;
   if (!OS_MUTEX_Create(&AES_Mutex))
   {
      retVal = eFAILURE;
   }
   return(retVal);
}

/***********************************************************************************************************************

  Function Name: AES_128_EncryptData

  Purpose: This function is used to Encrypt the passed in data with AES 128.

  Arguments: pSrc - pointer to the Data that is to be encrypted
             pDest - pointer to the location that will contain the encrypted data output
             cnt - number of bytes to encrypt (length of pSrc)

  Returns: None

  Side Effects: Uses the hardware AES engine.

  Re-entrant Code: Yes

  Notes:

 **********************************************************************************************************************/
void AES_128_EncryptData ( uint8_t *pDest, const uint8_t *pSrc, uint32_t cnt, aesKey_T const *pAesKey )
{
      uint8_t  block[AES_128_BLOCK_LENGTH];
   uint8_t  initVect[AES_128_BLOCK_LENGTH];
   uint32_t keyExpanded[AES_128_KEY_SCHEDULE_SIZE];
   uint8_t  encryptedData[AES_128_KEY_SCHEDULE_SIZE];
   uint8_t  i;
   uint8_t  byteCnt;

   (void)memcpy( &initVect[0], pAesKey->pInitVector, sizeof(initVect) );

   OS_MUTEX_Lock(&AES_Mutex);

   cau_aes_set_key( pAesKey->pKey, AES_128, (uint8_t *) & keyExpanded[0] );   /* Set the key */
   while ( 0 != cnt )
   {
      byteCnt = (uint8_t)MINIMUM(sizeof(block), cnt);      /* Get the number of bytes to operate on. */
      (void)memcpy( &block[0], pSrc, byteCnt );          /* Copy source data */
      if ( cnt < sizeof(block) )                         /* Is cnt < 16 bytes?  Is there a remainder? */
      {
         uint8_t padVal = (uint8_t)(sizeof(block) - cnt);    /* Get the pad value, this may be 0. */
         (void)memset(&block[cnt], padVal, padVal);      /* Pad the remaining bytes */
      }
      for ( i = 0; i < sizeof(block); i++ )
      {
         block[i] ^= initVect[i];
      } /* end for() */

      cau_aes_encrypt( &block[0], (uint8_t *) & keyExpanded[0], AES_128_NUM_ROUNDS, &encryptedData[0] );

      (void)memcpy( pDest, &encryptedData[0], byteCnt );      /* Only copy the portion that will fit into pDest */
      (void)memcpy(&initVect[0], &encryptedData[0], sizeof(initVect) );

      pSrc  += byteCnt; /* Increment the pointers */
      pDest += byteCnt;
      cnt   -= byteCnt; /* Decrement the number of bytes to operate on */
   }
   OS_MUTEX_Unlock(&AES_Mutex);
} /* end AES_128_EncryptData () */

/***********************************************************************************************************************

  Function Name: AES_128_DecryptData

  Purpose: This function is used to Decrypt the passed in data with AES 128

  Arguments: pSrc - pointer to the Data that is to be decrypted
             pDest - pointer to the Data structure that will contain the decrypted data output
             cnt - number of bytes to decrypt (length of pSrc)

  Returns: None

  Side Effects: Uses the hardware AES engine.

  Re-entrant Code: Yes

  Notes:

 **********************************************************************************************************************/
void AES_128_DecryptData ( uint8_t *pDest, const uint8_t *pSrc, uint32_t cnt, aesKey_T const *pAesKey )
{
   uint32_t i;
   uint8_t  byteCnt;
   uint8_t  block[AES_128_BLOCK_LENGTH];
   uint8_t  initVect[AES_128_BLOCK_LENGTH];
   uint8_t  encryptedData[AES_128_KEY_SCHEDULE_SIZE];
   uint32_t keyExpanded[AES_128_KEY_SCHEDULE_SIZE];

   (void)memcpy( &initVect[0], pAesKey->pInitVector, sizeof(initVect) );

   OS_MUTEX_Lock(&AES_Mutex);
   cau_aes_set_key ( pAesKey->pKey, AES_128, (uint8_t *) & keyExpanded[0] );

   while ( 0 != cnt )
   {
      byteCnt = (uint8_t)MINIMUM(sizeof(block), cnt);      /* Get the number of bytes to operate on. */
      (void)memcpy( &block[0], pSrc, byteCnt );          /* Copy source data */
      if ( cnt < sizeof(block) )                         /* Is cnt < 16 bytes?  Is there a remainder? */
      {
         uint8_t padVal = (uint8_t)(sizeof(block) - cnt);    /* Get the pad value, this may be 0. */
         (void)memset(&block[cnt], padVal, padVal);      /* Pad the remaining bytes */
      }

      cau_aes_decrypt ( &block[0], (uint8_t *) & keyExpanded[0], AES_128_NUM_ROUNDS, &encryptedData[0] );

      for ( i = 0; i < byteCnt; i++ )
      {
         pDest[i] = ( uint8_t )( initVect[i] ^ encryptedData[i] );
      } /* end for() */

      (void)memcpy ( &initVect[0], &block[0], sizeof(initVect) );

      pSrc  += byteCnt;  /* Increment the pointers */
      pDest += byteCnt;
      cnt   -= byteCnt;  /* Decrement the number of bytes to operate on */
   }
   OS_MUTEX_Unlock(&AES_Mutex);
} /* end AES_128_DecryptData () */

/***********************************************************************************************************************

  Function Name: AES_128_UnitTest

  Purpose: Tests the AES encryption

  Arguments: None

  Returns: None

  Side Effects: N/A

  Re-entrant Code: No

  Notes:

 **********************************************************************************************************************/
void AES_128_UnitTest( void )
{
#ifdef TM_AES_UNIT_TEST
   uint8_t       tstArray[16 * 2];
   uint8_t       prntStr[(sizeof(tstArray) * 2) + 1];
   uint8_t       i;
   aesKey_T    aesKey;

   DBG_printf(" AES Testing");
   printHexArray( &prntStr[0], (uint8_t *)&AES_128_KEY[0], sizeof(AES_128_KEY) );
   DBG_printf("  Key: %s", prntStr);
   printHexArray( &prntStr[0], (uint8_t *)&AES_128_INIT_VECTOR[0], sizeof(AES_128_INIT_VECTOR) );
   DBG_printf("  Init Vector: %s", prntStr);

   aesKey.pKey = (uint8_t *)AES_128_KEY;
   aesKey.pInitVector = (uint8_t *)AES_128_INIT_VECTOR;

   DBG_printf(" ");
   DBG_printf("  Test 1:");
   for (i = 0; i < sizeof(tstArray); i++)
   {
      tstArray[i] = i + 1;
   }
   printHexArray( &prntStr[0], &tstArray[0], sizeof(tstArray) );
   DBG_printf("  Array:     %s", prntStr);
   AES_128_EncryptData(&tstArray[0], &tstArray[0], sizeof(tstArray), &aesKey);
   printHexArray( &prntStr[0], &tstArray[0], sizeof(tstArray) );
   DBG_printf("  Encrypted: %s", prntStr);
   AES_128_DecryptData ( &tstArray[0], &tstArray[0], sizeof(tstArray), &aesKey );
   printHexArray( &prntStr[0], &tstArray[0], sizeof(tstArray) );
   DBG_printf("  Decrypted: %s", prntStr);

   DBG_printf(" ");
   DBG_printf("  Test 2:");
   for (i = 0; i < sizeof(tstArray); i++)
   {
      tstArray[i] = (i + 2) * 2;
   }
   printHexArray( &prntStr[0], &tstArray[0], sizeof(tstArray) );
   DBG_printf("  Array:     %s", prntStr);
   AES_128_EncryptData(&tstArray[0], &tstArray[0], sizeof(tstArray), &aesKey);
   printHexArray( &prntStr[0], &tstArray[0], sizeof(tstArray) );
   DBG_printf("  Encrypted: %s", prntStr);
   AES_128_DecryptData ( &tstArray[0], &tstArray[0], sizeof(tstArray), &aesKey );
   printHexArray( &prntStr[0], &tstArray[0], sizeof(tstArray) );
   DBG_printf("  Decrypted: %s", prntStr);
   DBG_printf(" ");

#endif  //TM_AES_UNIT_TEST
}

#ifdef TM_AES_UNIT_TEST
/***********************************************************************************************************************

  Function Name: printHexArray

  Purpose: Prints array of hex to a string

  Arguments: None

  Returns: None

  Side Effects: N/A

  Re-entrant Code: No

  Notes:

 **********************************************************************************************************************/
static void printHexArray( uint8_t *pStr, uint8_t *pData, uint8_t cnt )
{
   uint8_t i;

   for (i = 0; i < cnt; i++, pData++, pStr+=2)
   {
      sprintf(pStr, "%02X", *pData);
   }
}
#endif  //TM_AES_UNIT_TEST
