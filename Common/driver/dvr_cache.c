/***********************************************************************************************************************

   Filename:   dvr_cache.c

   Global Designator: DVR_CACHE_

   Contents:   The cache driver keeps a copy of an area of memory in RAM.  Can be used for fast access and/or to address
               NV memory endurance issues.

 ***********************************************************************************************************************
   Copyright (c) 2020 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
   or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************

   $Log$ kdavlin Created May 4, 2011

 ***********************************************************************************************************************
   Revision History:
   v0.1  - KAD 05/05/2011 - Initial Release
         - MS  02/11/2013 - Added CRC to cache

 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "project.h"

#define dvr_cache_GLOBAL
#include "dvr_cache.h"
#undef  dvr_cache_GLOBAL

#include "partition_cfg.h"

#ifdef TM_CACHE_UNIT_TEST
#include "DBG_SerialDebug.h"
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* For Unit Testing, we need to return a FAILURE in place of the assert.  */
#ifdef TM_DVR_CACHED_UNIT_TEST
#undef ASSERT
#define ASSERT(__e) if (!(__e)) return(eFAILURE)
#endif

#define CACHE_NOT_RESTORED      false
#define CACHE_RESTORED          true

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#if RTOS
static OS_MUTEX_Obj cacheMutex_; /* Serialize access to the cache driver */
static bool cacheMutexCreated_ = false;
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static returnStatus_t init( PartitionData_t const *pPartition, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t close( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                            DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t write( const dSize DestOffset, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t setPowerMode( const ePowerMode esetPowerMode, PartitionData_t const *pParData,
                                    DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNxtDvr );
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData,
                               DeviceDriverMem_t const * const * pNxtDvr );
static bool        timeSlice(PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver);

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* The driver table must be defined after the function definitions. The list below are the supported commands for this
 * driver. */
const DeviceDriverMem_t sDeviceDriver_Cache ={
   init,          // Init function - Creates mutex and restores memory if needed
   open,          // Open Command - For this implementation it only calls the lower level drivers.
   close,         // Close Command - For this implementation it only calls the lower level drivers.
   setPowerMode,  // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost */
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

   Purpose: Initializes the cached memory.  The initialize function should only be called once at power up.  If the
            power up cause is due to an actual power loss, then the NV memory data is read and placed into the RAM.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

   Returns: returnStatus_t � As defined by error_codes.h

   Side Effects: NV memory may be accessed (CPU Time) and the cache memory updated.

   Reentrant Code: No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t init( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, init ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

#if RTOS
   if ( cacheMutexCreated_ == false ) /* If the mutex has not been created, create it. */
   {
      /* Create mutex to protect the cache driver modules critical section */
      if ( true == OS_MUTEX_Create(&cacheMutex_) )
      {
         cacheMutexCreated_ = true;
      } /* end if() */
   }
#endif
   return ((*pNxtDvr)->devInit(pParData, pNxtDvr + 1));  /* Initialize the memory driver */
}
/***********************************************************************************************************************

   Function Name: open

   Purpose: The cache driver doesn�t have anything to open; the information is passed to the next driver.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

   Returns: returnStatus_t � As defined by error_codes.h

   Side Effects: None

   Reentrant Code:  No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   returnStatus_t eRetVal;    /* Return value */
   cacheMetaData_t sCache;    /* Cache meta data */

   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, open ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   pParData->sAttributes.pMetaData->cacheRestored = CACHE_NOT_RESTORED;
   eRetVal = (*pNxtDvr)->devOpen(pParData, pNxtDvr + 1); /* Open the driver */
   if ( eSUCCESS == eRetVal ) /* If the driver opened properly, restore the cache. */
   {
      /* Compute CRC on the whole chache area */
      uint16_t crc = CRC_16_Calculate(pParData->sAttributes.pCachedData, pParData->lDataSize);

      (void)memcpy( ( uint8_t *)&sCache, pParData->sAttributes.pCachedData + pParData->lDataSize, sizeof(sCache));

      if ( sCache.crc16 != crc )
      {  /* restore cache */
         eRetVal = restore((lAddr)0, (lCnt)0, pParData, pNxtDvr); /* 1st two parameters are ignored by the restore function */
      }
      pParData->sAttributes.pMetaData->cacheRestored = CACHE_RESTORED;
   }

   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: close

   Purpose: Saves the cached data to NV memory.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

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

   Purpose: reads data from the cached memory

   Arguments:
      uint8_t *pDest:  Location to write data
      dSize srcOffset:  Location of the source data in memory
      lCnt cnt:  Number of bytes to read.
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                            DeviceDriverMem_t const * const * pNxtDvr )
{
   /*lint -efunc( 715, read ) : pNxtDvr is not used here, but must be in the parameter list for the common API. */
   /*lint -efunc( 818, read ) : pNxtDvr is not used here, but must be in the parameter list for the common API. */

   ASSERT(cnt <= pParData->lSize); /* Count must be <= the sector size. */

   OS_MUTEX_Lock(&cacheMutex_);
   (void)memcpy(pDest, pParData->sAttributes.pCachedData + srcOffset, cnt); /* Read the data from the RAM */
   OS_MUTEX_Unlock(&cacheMutex_);

   return (eSUCCESS);
}
/***********************************************************************************************************************

   Function Name: write

   Purpose: writes data to the cached memory

   Arguments:
      dSize DestOffset:  Location of the data to write to the NV Memory
      uint8_t *pSrc:  Location of the source data to write to Flash
      lCnt Cnt:  Number of bytes to write to the NV Memory.
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t write( const dSize DestOffset, uint8_t const *pSrc, const lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNxtDvr )
{
   cacheMetaData_t sCache;    /* Cache meta data */
   /*lint -efunc( 715, write ) : pNxtDvr is not used here, but must be in the parameter list for the common API. */
   /*lint -efunc( 818, write ) : pNxtDvr is not used here, but must be in the parameter list for the common API. */

   ASSERT(cnt <= pParData->lSize); /* Count must be <= the sector size. */

   OS_MUTEX_Lock(&cacheMutex_);
   (void)memcpy(pParData->sAttributes.pCachedData + DestOffset, pSrc, cnt); /* Write the data to the RAM */
   /* Compute CRC on the whole cache area */
   //sCache.crc16 = CRC_16_Calculate(pParData->sAttributes.pCachedData, pParData->lDataSize);
   (void)memcpy(pParData->sAttributes.pCachedData + pParData->lDataSize, ( uint8_t *)&sCache, sizeof(sCache));
   OS_MUTEX_Unlock(&cacheMutex_);

   return (eSUCCESS);
}
/***********************************************************************************************************************

   Function Name: flush

   Purpose: Writes the cached memory to next layer.

   Arguments:
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   returnStatus_t eRetVal = eSUCCESS;

   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, flush ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   if ( CACHE_RESTORED == pParData->sAttributes.pMetaData->cacheRestored )
   {
      /* DevNote 07/05/18 SMG: Could add RAM cache CRC check to ensure RAM is not corrupted before saving.
                              Need to evalute the execution time hit at power down. */
      OS_MUTEX_Lock(&cacheMutex_);
      eRetVal = (*pNxtDvr)->devWrite((dSize)0,                                   /* Start at the begining of the memory. */
                                  pParData->sAttributes.pCachedData,             /* Points to the cache ram buffer. */
                                  pParData->lDataSize + sizeof(cacheMetaData_t), /* write all data to NV */
                                  pParData,                                      /* Partition information. */
                                  pNxtDvr + 1);                                  /* Next driver to go to. */
      OS_MUTEX_Unlock(&cacheMutex_);
   }
   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: erase

   Purpose: Erases the Cache memory and then flushes the buffer.

   Arguments:
      dSize offset - Address to erase
      lCnt cnt - number of bytes to erase
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr )
{
   returnStatus_t eRetVal;

   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, erase ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   OS_MUTEX_Lock(&cacheMutex_);
   (void)memset(pParData->sAttributes.pCachedData + destOffset, 0, cnt);
   eRetVal = (*pNxtDvr)->devWrite((dSize)0,                          /* Start at the begining of the memory. */
                               pParData->sAttributes.pCachedData,    /* Points to the cache ram buffer. */
                               pParData->lDataSize + sizeof(cacheMetaData_t),  /* write all data to NV */
                               pParData,                             /* Partition information. */
                               pNxtDvr + 1);                         /* Next driver to go to. */
   OS_MUTEX_Unlock(&cacheMutex_);
   return(eRetVal);
}
/***********************************************************************************************************************

   Function Name: setPowerMode

   Purpose: The cache driver doesn't have a power mode; the information is passed to the next driver.

   Arguments:
      enum esetPowerMode � uses ePowerMode enumeration
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

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

   Purpose: The cache driver doesn't support ioctl commands

   Arguments:
      void *pCmd � Command to execute:
      void *pData � Data to device.
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                  partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

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

   Purpose: If not a spurious reset: reads the cache memory from the lower driver and puts it into RAM.  The pNxtDvr is
            already validated before this function is called.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                        partition.
      DeviceDriverMem_t const * const * pNxtDvr � Points to the next driver�s table.

   Returns: returnStatus_t � As defined by error_codes.h

   Side Effects: NV memory may be accessed (CPU Time) and the cache memory updated.

   Reentrant Code: No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t restore(lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNxtDvr)
{
   ASSERT(NULL != pNxtDvr); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, restore ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */
   /*lint -efunc( 715, restore ) : DeviceDriverMem_t is not used.  Is is needed for a common API. */

   return ((*pNxtDvr)->devRead(pParData->sAttributes.pCachedData, (dSize)0, pParData->lDataSize + sizeof(cacheMetaData_t),
                            pParData, pNxtDvr + 1));
}
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
/* Unit Test */

/***********************************************************************************************************************

   Function Name: DVR_CACHE_unitTest

   Purpose: Performs unit testing - After this test completes, all data will be lost!  The processor will also be reset.

   Arguments: none

   Returns: none

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void DVR_CACHE_unitTest(void)
{
#ifdef TM_CACHE_UNIT_TEST
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
