/***********************************************************************************************************************

   Filename:   dvr_encryption.c

   Global Designator: DVR_ENCRYPT_

   Contents: encrypts the data going to/from the memory.

 ***********************************************************************************************************************
   Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
   or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************

   $Log$ kdavlin Created August 20, 2013

 ***********************************************************************************************************************
   Revision History:
   v0.1 - KAD 08/20/2013 - Initial Release

 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "portable_freescale.h"
#define dvr_encryption_GLOBAL
#include "dvr_encryption.h"
#undef  dvr_encryption_GLOBAL

#include "partition_cfg.h"

#ifdef TM_ENCRYPT_UNIT_TEST
#include "DBG_SerialDebug.h"
#endif

#include "dvr_intFlash_cfg.h" /* Contains the location to store the AES key */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define ENCRYPT_BUFF_SIZE     ((uint8_t)64) /* Needs to be a multiple of 16 bytes.  Example 16, 32, 64, 128... */

#define AES_KEY_LENGTH 16 /* Length in bytes of the AES key */

/* For Unit Testing, we need to return a FAILURE in place of the assert.  */
#ifdef TM_DVR_ENCRYPT_UNIT_TEST
#undef ASSERT
#define ASSERT(__e) if (!(__e)) return(eFAILURE)
#endif
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint8_t  aesKey[AES_KEY_LENGTH];  /* Key accessible in bytes */
}dvrEnAesKey_t;  /* AES key, accessible by words or bytes.  A valid key can not be all 0xFFs. */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#if RTOS
static OS_MUTEX_Obj encryptMutex_; /* Serialize access to the cache driver */
static bool encryptMutexCreated_ = false;
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static returnStatus_t   init( PartitionData_t const *pPartition, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   close( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   write( const dSize DestOffset, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   setPowerMode( const ePowerMode esetPowerMode, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t   restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static bool             timeSlice(PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver);

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

static void generateAesKey(dvrEnAesKey_t *pKey);
static returnStatus_t getAesKey(dvrEnAesKey_t *pKey);

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* The driver table must be defined after the function definitions. The list below are the supported commands for this
 * driver. */
const DeviceDriverMem_t sDeviceDriver_Encryption =
{
   init,          // Init function - Creates mutex and restores memory if needed
   open,          // Open Command - For this implementation it only calls the lower level drivers.
   close,         // Close Command - For this implementation it only calls the lower level drivers.
   setPowerMode,  // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost
   read,          // Read Command - Reads from cache
   write,         // Write Command - Writes to cache
   erase,         // Erases a portion (or all) of the cache memory and then flushes the memory.
   flush,         // Write the cache content to the lower layer driver
   ioctl,         // ioctl function - Does Nothing for this implementation
   restore,       // Restores the RAM content from NV memory
   timeSlice      // For cache driver, this does nothing but calls the lower next driver.
};

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: init

   Purpose: Initializes the encrypted memory.  The initialize function should only be called once at power up.

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver's table.

   Returns: returnStatus_t - As defined by error_codes.h

   Side Effects: NV memory may be accessed (CPU Time)

   Reentrant Code: No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t init( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, init ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

#if RTOS
   if ( encryptMutexCreated_ == false ) /* If the mutex has not been created, create it. */
   {
      /* Create mutex to protect the cache driver modules critical section */
      if ( true == OS_MUTEX_Create(&encryptMutex_) )
      {
         encryptMutexCreated_ = true;
      } /* end if() */
   } /* end if() */
#endif

   return ((*pNxtDvr)->devInit(pParData, pNxtDvr + 1));  /* Initialize the memory driver */
}
/***********************************************************************************************************************

   Function Name: open

   Purpose: The encryption driver doesn't have anything to open; the information is passed to the next driver.

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver's table.

   Returns: returnStatus_t - As defined by error_codes.h

   Side Effects: None

   Reentrant Code:  No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, open ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return((*pNxtDvr)->devOpen(pParData, pNxtDvr + 1)); /* Open the driver */
}
/***********************************************************************************************************************

   Function Name: close

   Purpose: closes the encryption driver

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
static returnStatus_t close( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, close ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */
   return ((*pNxtDvr)->devClose(pParData, pNxtDvr + 1));
}
/***********************************************************************************************************************

   Function Name: read

   Purpose: reads data from memory and decrypts it

   Arguments:
      uint8_t *pDest:  Location to write data
      dSize srcOffset:  Location of the source data in memory
      lCnt cnt:  Number of bytes to read.
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                            DeviceDriverMem_t const * const * pNxtDvr )
{
   uint8_t        buffer[ENCRYPT_BUFF_SIZE]; /* Place to store data read to decrypt */
   dSize          srcAdj;                    /* Adjusted source address for reading blocks of data */
   dSize          srcOffAdj = 0;             /* Adjusted source offset - Added to srcOffset */
   lCnt           numToCopy;                 /* Number of bytes to read */
   lCnt           totalCnt = cnt;            /* Total number of bytes to write */
   dSize          blockOffset;               /* Offset into buffer where the requested data starts */
   returnStatus_t retVal;                    /* Return status */
   aesKey_T       aesKey;
   dvrEnAesKey_t  aelMstrKey;

   retVal = getAesKey(&aelMstrKey);
   if(eSUCCESS == retVal)
   {
      aesKey.pKey = &aelMstrKey.aesKey[0];
      aesKey.pInitVector = (uint8_t *)AES_128_INIT_VECTOR;

      OS_MUTEX_Lock(&encryptMutex_);
      while(totalCnt && (eSUCCESS == retVal))
      {
         /* Calculate the starting location for the block. */
         srcAdj = (srcOffset + srcOffAdj) % sizeof(buffer);
         blockOffset = (srcOffset + srcOffAdj) - srcAdj;

         /* Read the entire block of data */
         retVal = (*pNxtDvr)->devRead(&buffer[0], blockOffset, sizeof(buffer), pParData, pNxtDvr + 1);
         if (eSUCCESS == retVal)
         {
            /* Decrypt the data */
            AES_128_DecryptData ( &buffer[0], &buffer[0], sizeof(buffer), &aesKey );

            /* Copy buffer to pDest */
            numToCopy = MINIMUM(totalCnt, sizeof(buffer)-srcAdj);
            (void)memcpy(pDest, buffer+srcAdj, numToCopy);

            /* Adjust the data: pDest, srcOffset and totalCnt */
            pDest += numToCopy;
            srcOffAdj += numToCopy;
            totalCnt -= numToCopy;
         }
      }
      OS_MUTEX_Unlock(&encryptMutex_);
   }
   return (retVal);
}
/***********************************************************************************************************************

   Function Name: write

   Purpose: Encrypts data and writes it to memory

   Arguments:
      dSize DestOffset:  Location of the data to write to the NV Memory
      uint8_t *pSrc:  Location of the source data to write to Flash
      lCnt Cnt:  Number of bytes to write to the NV Memory.
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t write( const dSize DestOffset, uint8_t const *pSrc, const lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNxtDvr )
{
   uint8_t        buffer[ENCRYPT_BUFF_SIZE]; /* Place to store data read to decrypt */
   dSize          srcAdj;                    /* Adjusted source address for reading blocks of data */
   dSize          srcOffAdj = 0;             /* Adjusted source offset - Added to srcOffset */
   dSize          dstOffAdj = 0;             /* Adjusted destination offset - Added to DestOffset */
   lCnt           numToCopy;                 /* Number of bytes to read */
   dSize          blockOffset;               /* Offset into buffer where the requested data starts */
   lCnt           totalCnt = cnt;            /* Total number of bytes to write */
   returnStatus_t retVal;                    /* Return status */
   aesKey_T       aesKey;
   dvrEnAesKey_t  aelMstrKey;

   retVal = getAesKey(&aelMstrKey);
   if (eSUCCESS != retVal)
   {
      generateAesKey(&aelMstrKey);
   }
   aesKey.pKey = &aelMstrKey.aesKey[0];
   aesKey.pInitVector = (uint8_t *)AES_128_INIT_VECTOR;

   OS_MUTEX_Lock(&encryptMutex_);
   while(totalCnt && (eSUCCESS == retVal))
   {
      /* Calculate the starting location for the block. */
      srcAdj = (DestOffset + dstOffAdj) % sizeof(buffer);
      blockOffset = (DestOffset + dstOffAdj) - srcAdj;

      /* Read the entire block of data */
      retVal = (*pNxtDvr)->devRead(&buffer[0], blockOffset, sizeof(buffer), pParData, pNxtDvr + 1);
      if (eSUCCESS == retVal)
      {
         /* Decrypt the data */
         AES_128_DecryptData ( &buffer[0], &buffer[0], sizeof(buffer), &aesKey );

         /* Copy buffer to pDest */
         numToCopy = MINIMUM(totalCnt, sizeof(buffer)-srcAdj);
         (void)memcpy(buffer + srcAdj, pSrc + srcOffAdj, numToCopy);

         /* Encrypt the data */
         AES_128_EncryptData(&buffer[0], &buffer[0], sizeof(buffer), &aesKey);

         /* Write the block of data to memory. */
         retVal = (*pNxtDvr)->devWrite(blockOffset, &buffer[0], sizeof(buffer), pParData, pNxtDvr + 1);

         /* Adjust the data: pDest, srcOffset and totalCnt */
         dstOffAdj += numToCopy;
         srcOffAdj += numToCopy;
         totalCnt -= numToCopy;
      }
   }
   OS_MUTEX_Unlock(&encryptMutex_);
   return (retVal);
}
/***********************************************************************************************************************

   Function Name: flush

   Purpose: Does nothing for the encryption driver

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, flush ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */
   return ((*pNxtDvr)->devFlush(pParData, pNxtDvr + 1));
}
/***********************************************************************************************************************

   Function Name: erase

   Purpose: Does nothing

   Arguments:
      dSize offset - Address to erase
      lCnt cnt - number of bytes to erase
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNxtDvr )
{
   returnStatus_t retVal = eSUCCESS;
   uint8_t          buffer[ENCRYPT_BUFF_SIZE]; /* Place to store data read to decrypt */

   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, erase ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   /* Todo:  Get encryption key! */

   (void)memset(&buffer[0], 0, sizeof(buffer)); /* Clear the buffer to "erase" the device */

   OS_MUTEX_Lock(&encryptMutex_);
   while(cnt && (eSUCCESS == retVal))
   {
      lCnt bytesWritten = MINIMUM(cnt, sizeof(buffer));
      retVal = write( destOffset, &buffer[0], bytesWritten, pParData, pNxtDvr );
      destOffset += bytesWritten;
      cnt -= bytesWritten;
   }
   OS_MUTEX_Unlock(&encryptMutex_);
   return(retVal);
}
/***********************************************************************************************************************

   Function Name: setPowerMode

   Purpose: The encryption driver doesn't have a power mode; the information is passed to the next driver.

   Arguments:
      enum esetPowerMode - uses ePowerMode enumeration
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t setPowerMode( const ePowerMode esetPowerMode, PartitionData_t const *pParData,
                                    DeviceDriverMem_t const * const * pNxtDvr )
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, setPowerMode ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNxtDvr)->devSetPwrMode(esetPowerMode, pParData, pNxtDvr + 1));
}
/***********************************************************************************************************************

   Function Name: ioctl

   Purpose: The encryption driver doesn't support ioctl commands

   Arguments:
      void *pCmd - Command to execute:
      void *pData - Data to device.
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver�s table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNxtDvr )
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, ioctl ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNxtDvr)->devIoctl(pCmd, pData, pParData, pNxtDvr + 1));
}
/***********************************************************************************************************************

   Function Name: restore

   Purpose: Does nothing

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr - Points to the next driver�s table.

   Returns: returnStatus_t - As defined by error_codes.h

   Side Effects: NV memory may be accessed (CPU Time) and the cache memory updated.

   Reentrant Code: No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t restore(lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr)
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   return(eSUCCESS);
} /*lint !e715 !e818 */
/***********************************************************************************************************************

   Function Name: timeSlice

   Purpose: Common API function, allows for the driver to perform any housekeeping that may be required.  For this
            module, nothing is to be completed.

   Arguments: PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver

   Returns: bool - 0 (false)

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static bool timeSlice(PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver)
{
   return ((*pNextDriver)->devTimeSlice(pParData, pNextDriver + 1));
}
/* ****************************************************************************************************************** */
/* Local Functions */

/***********************************************************************************************************************

   Function Name: getAesKey

   Purpose: Gets the key and verifies it is valid.

   Arguments: dvrEnAesKey_t *pKey - Location to store the key

   Returns: returnStatus_t - eSUCCESS or eFAILURE

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t getAesKey(dvrEnAesKey_t *pKey)
{
   uint8_t                 i;
   returnStatus_t          retVal = eFAILURE;
   PartitionData_t const   *pParTbl;

   (void)memset(pKey, 0xFF, sizeof(*pKey));

   (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_ENCRYPT_KEY, 0);
   (void)PAR_partitionFptr.parRead((uint8_t *)pKey, (dSize)offsetof(intFlashNvMap_t, aesKey),
                                   (lCnt)sizeof(pKey->aesKey), pParTbl);

   for (i = 0; i < sizeof(pKey->aesKey); i++)
   {
      if (0xFF != pKey->aesKey[i])
      {
         retVal = eSUCCESS;
         break;
      }
   }
   return (retVal);
}
/***********************************************************************************************************************

   Function Name: generateAesKey

   Purpose: Generates the key and writes it to NV.

   Arguments: dvrEnAesKey_t *pKey - Location to store the generated key

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static void generateAesKey(dvrEnAesKey_t *pKey)
{
   bool isKeyValid = false; /* Indicator for a valid key, the key starts out as invalid */
   PartitionData_t const *pParTbl; /* Pointer to partition */

   (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_ENCRYPT_KEY, 0);
   (void)memset(pKey, 0xFF, sizeof(*pKey)); /* Invalidate the key */

   do
   {
      uint8_t i;  /* Used as a counter for the for loops. */

      /* Populate the Key with random numbers */
      RNG_GetRandom_Array ( &(pKey->aesKey[0]), AES_KEY_LENGTH );

      for (i = 0; i < sizeof(pKey->aesKey); i++)
      {
         if (0xFF != pKey->aesKey[i])
         {
            isKeyValid = true;
            break;
         }
      }
   }while(!isKeyValid);

   /* The key is now valid - Write the key to NV memory */
   (void)PAR_partitionFptr.parWrite((dSize)offsetof(intFlashNvMap_t, aesKey), (uint8_t *)pKey,
                                    sizeof(pKey->aesKey), pParTbl);
}
/* ****************************************************************************************************************** */
/* Unit Test */

/***********************************************************************************************************************

   Function Name: DVR_ENCRYPT_unitTest

   Purpose: Performs unit testing - After this test completes, all data will be lost!  The processor will also be reset.

   Arguments: none

   Returns: none

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void DVR_ENCRYPT_unitTest(void)
{
#ifdef TM_ENCYRPT_UNIT_TEST
   PartitionData_t const   *pParTbl;
   uint16_t                  i;                      /* Index counter for loops */
   uint8_t                   wrData[PART_NV_APP_CACHED_SIZE/4];
   uint8_t                   rdData[sizeof(wrData)];
   returnStatus_t          callRetVal;

   for (i = 0; i < sizeof(wrData); i++)  /* Fill data with values to test. */
   {
      wrData[i] = (uint8_t)i + 1;
   }

   /* Step 1:  Erase the entire NV device */
   DBG_logPrintf('U', "  Erasing Device");
   (void)PAR_partitionFptr.open(&pParTbl, ePART_WHOLE_NV, 0);
   (void)PAR_partitionFptr.erase(0, EXT_FLASH_SIZE, pParTbl);

   /* Step 2:  Initialize the Cached Area - Print the Address of the underlying Bank. */
   DBG_logPrintf('U', "  Open Cache Driver - %i Banks, %i Bytes, %i Bytes/Bank",
                  PART_NV_APP_CACHED_BANKS,
                  PART_NV_APP_CACHED_SIZE * PART_NV_APP_CACHED_BANKS,
                  PART_NV_APP_CACHED_SIZE);
   (void)PAR_partitionFptr.init();
   (void)PAR_partitionFptr.open(&pParTbl, ePART_NV_APP_CACHED, 0);
   (void)PAR_partitionFptr.erase(0,
                                 PART_NV_APP_CACHED_SIZE - ( sizeof(bankMetaData_t) + sizeof(cacheMetaData_t)),
                                 pParTbl);

   DBG_logPrintf('U', "  Write/Read Testing:");
   /* Step 3:  Test Read/Write */
   for (i = 0; i < (PART_NV_APP_CACHED_BANKS + 1); i++)
   {
      wrData[i%sizeof(wrData)] = 0;
      rdData[i%sizeof(wrData)] = 0x55;
      DBG_printfNoCr("    Step %3i/%3i: ", i+1, PART_NV_APP_CACHED_BANKS + 1);
      callRetVal = PAR_partitionFptr.write((dSize)0, &wrData[0], (lCnt)sizeof(wrData), pParTbl);
      DBG_printfNoCr("Resp = %i, Offset = %6i, Seq = %3i",
               callRetVal,
               sCached_Banked_MetaData.BankOffset,
               sCached_Banked_MetaData.CurrentBankSequence);
      (void)memset(&sCached_Banked_MetaData, 0, sizeof(sCached_Banked_MetaData));  /* wipe out the meta data to make sure it restores */
      (void)PAR_partitionFptr.init();
      (void)PAR_partitionFptr.open(&pParTbl, ePART_NV_APP_CACHED, 0);

      callRetVal = PAR_partitionFptr.read(&rdData[0], (dSize)0, (lCnt)sizeof(rdData), pParTbl);
      if ((eSUCCESS != callRetVal) || (0 != memcmp(&wrData[0], &rdData[0], sizeof(rdData))))
      {
         DBG_logPrintf('U', "    Read Verify Failed!");
      }
      wrData[i%sizeof(wrData)] = 0xFF;
      (void)PAR_partitionFptr.timeSlice();
      OS_TASK_Sleep ( 30 );
      (void)PAR_partitionFptr.timeSlice();
      DBG_logPrintf('U', ", Seq Init: %3d", sCached_Banked_MetaData.CurrentBankSequence);
      OS_TASK_Sleep ( 10 );
   }
   OS_TASK_Sleep ( 10 );

   /* Step 4:  Test Read/Write with Flush - Should see the seq # increment.  */
   DBG_logPrintf('U', "  Write/Read Testing: Flush after write (Seq Increments)");
   for (i = 0; i < sizeof(wrData); i++)  /* Fill data with all 0xFF to force bank increments when writing later. */
   {
      wrData[i] = 0xFF;
   }
   for (i = 0; i < (PART_NV_APP_CACHED_BANKS + 1); i++)
   {
      wrData[i%sizeof(wrData)] = 0;
      rdData[i%sizeof(wrData)] = 0x55;
      DBG_printfNoCr("    Step %3i/%3i: ", i+1, PART_NV_APP_CACHED_BANKS + 1);
      callRetVal = PAR_partitionFptr.write((dSize)0, &wrData[0], (lCnt)sizeof(wrData), pParTbl);
      DBG_printfNoCr("Resp = %i, Offset = %6i, Seq = %3i",
               callRetVal,
               sCached_Banked_MetaData.BankOffset,
               sCached_Banked_MetaData.CurrentBankSequence);
      (void)PAR_partitionFptr.flush(pParTbl);
      (void)memset(&sCached_Banked_MetaData, 0, sizeof(sCached_Banked_MetaData));  /* wipe out the meta data to make sure it restores */
      (void)PAR_partitionFptr.init();
      (void)PAR_partitionFptr.open(&pParTbl, ePART_NV_APP_CACHED, 0);
      (void)PAR_partitionFptr.read(&rdData[0], (dSize)0, (lCnt)sizeof(rdData), pParTbl);
      if ((eSUCCESS != callRetVal) || (0 != memcmp(&wrData[0], &rdData[0], sizeof(rdData))))
      {
         DBG_logPrintf('U', "    Read Verify Failed!");
      }
      wrData[i%sizeof(wrData)] = 0xFF;
      (void)PAR_partitionFptr.timeSlice();
      OS_TASK_Sleep ( 30 );
      (void)PAR_partitionFptr.timeSlice();
      DBG_logPrintf('U', ", Seq Init: %3d", sCached_Banked_MetaData.CurrentBankSequence);
      OS_TASK_Sleep ( 10 );
   }

   (void)PAR_partitionFptr.timeSlice();
   OS_TASK_Sleep ( 30 );
   (void)PAR_partitionFptr.timeSlice();

   /* Step 5:  Write to last byte. */
   OS_TASK_Sleep ( 100 );
   wrData[0] = 0x0F;
   DBG_printfNoCr("  Writing Data to the last byte of the Cached Memory - ");
   callRetVal = PAR_partitionFptr.write((dSize)((PART_NV_APP_CACHED_SIZE -
                                                (sizeof(bankMetaData_t) + sizeof(cacheMetaData_t))) - 1),
                                        &wrData[0], (lCnt)1, pParTbl);
   rdData[0] = 0xFF;
   (void)PAR_partitionFptr.read(&rdData[0],
                                (dSize)((PART_NV_APP_CACHED_SIZE -
                                        (sizeof(bankMetaData_t) + sizeof(cacheMetaData_t))) - 1),
                                (lCnt)1, pParTbl);
   if (rdData[0] == wrData[0])
   {
      DBG_logPrintf('U', "Passed");
   }
   else
   {
      DBG_logPrintf('U', "Failed");
   }

   /* Step 6:  Writing past the last byte. */
   OS_TASK_Sleep ( 100 );
   wrData[0] = 0x0F;
   DBG_printfNoCr("  Writing Data that overruns Cached Memory - ");
   (void)PAR_partitionFptr.write((dSize)((PART_NV_APP_CACHED_SIZE -
                                         (sizeof(bankMetaData_t) + sizeof(cacheMetaData_t))) - 1),
                                 &wrData[0], (lCnt)sizeof(wrData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_printfNoCr("WR Passed, ");
   }
   else
   {
      DBG_printfNoCr("WR FAILED, ");
   }
   callRetVal = PAR_partitionFptr.read(&rdData[0],
                                       (dSize)((PART_NV_APP_CACHED_SIZE -
                                               (sizeof(bankMetaData_t) + sizeof(cacheMetaData_t))) - 1),
                                       (lCnt)sizeof(rdData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_logPrintf('U', "RD Passed, ");
   }
   else
   {
      DBG_logPrintf('U', "RD FAILED, ");
   }

   /* Step 7:  Writing outside of Cached Memory. */
   OS_TASK_Sleep ( 100 );
   wrData[0] = 0x0F;
   DBG_printfNoCr("  Writing outside of Cached Memory - ");
   callRetVal = PAR_partitionFptr.write((dSize)((PART_NV_APP_CACHED_SIZE -
                                                (sizeof(bankMetaData_t) + sizeof(cacheMetaData_t))) + 1),
                                        &wrData[0],
                                        (lCnt)sizeof(wrData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_printfNoCr("WR Passed, ");
   }
   else
   {
      DBG_printfNoCr("WR FAILED, ");
   }
   callRetVal = PAR_partitionFptr.read(&rdData[0],
                                       (dSize)((PART_NV_APP_CACHED_SIZE -
                                               (sizeof(bankMetaData_t) + sizeof(cacheMetaData_t))) + 1),
                                       (lCnt)sizeof(rdData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_logPrintf('U', "RD Passed, ");
   }
   else
   {
      DBG_logPrintf('U', "RD FAILED, ");
   }

   /* Step 8:  Write, Flush and then restore data. */
   OS_TASK_Sleep ( 100 );
   DBG_printfNoCr("  Write-Flush-Restore-Read - ");
   (void)PAR_partitionFptr.write((dSize)0,  &wrData[0], (lCnt)sizeof(wrData), pParTbl);
   (void)PAR_partitionFptr.flush(pParTbl);
   (void)memset(&rdData[0], 0x99, sizeof(rdData));
   (void)PAR_partitionFptr.write((dSize)0,  &rdData[0], (lCnt)sizeof(rdData), pParTbl);
   (void)memset(&rdData[0], 0, sizeof(rdData));
   (void)PAR_partitionFptr.restore(0,
                                   (lCnt)(PART_NV_APP_CACHED_SIZE -
                                          (sizeof(bankMetaData_t) + sizeof(cacheMetaData_t))),
                                   pParTbl);
   (void)PAR_partitionFptr.read(&rdData[0], (dSize)0, (lCnt)sizeof(rdData), pParTbl);
   if (0 == memcmp(&wrData[0], &rdData[0], sizeof(rdData)))
   {
      DBG_logPrintf('U', "Passed");
   }
   else
   {
      DBG_logPrintf('U', "Failed");
   }

   OS_TASK_Sleep ( 100 );
   /* Step 9:  Corrupt the data, data should be restored from NV */
   DBG_printfNoCr("  Write-Flush-Corrupt-ReInit - ");
   (void)PAR_partitionFptr.write((dSize)0,  &wrData[0], (lCnt)sizeof(wrData), pParTbl);
   (void)PAR_partitionFptr.flush(pParTbl);
   persistCache_[0] = 0xAA;
   (void)PAR_partitionFptr.init();
   (void)memset(&rdData[0], 0x99, sizeof(rdData));
   (void)PAR_partitionFptr.read(&rdData[0], (dSize)0, (lCnt)sizeof(rdData), pParTbl);
   if (0 == memcmp(&wrData[0], &rdData[0], sizeof(rdData)))
   {
      DBG_logPrintf('U', "Passed");
   }
   else
   {
      DBG_logPrintf('U', "Failed");
   }

   DBG_logPrintf('U', "Resetting...");
   DBG_logPrintf('U', " ");
   OS_TASK_Sleep ( 3000 );
   RESET(); // Reset the processor to re-initialize the NV memory and re-create files properly.
#endif
}
