/***********************************************************************************************************************

   Filename:   dvr_sectPreErase.c

   Global Designator: DVR_SPE_

   Contents:   Erases the sector when the 1st byte to that sector is written to.  If using the partition manager, you must
               make this partition a multiple of the sector size.

 ***********************************************************************************************************************
   Copyright (c) 2012-2020 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
   or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
   Revision History:
   20110505, v0.1, KAD - Initial Release

 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdbool.h>
#include <string.h>

#define dvr_sectPreErase_GLOBAL
#include "dvr_sectPreErase.h"
//#include "dvr_sectPreErase_cfg.h"
#undef  dvr_sectPreErase_GLOBAL

#include "partition_cfg.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
/* For Unit Testing, we need to return a FAILURE in place of the assert.  */
#ifdef TM_DVR_BANKED_UNIT_TEST
#undef ASSERT
#define ASSERT(__e) if (!(__e)) return(eFAILURE)
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#if RTOS
static OS_MUTEX_Obj preEraseMutex_; /* Serialize access to the cache driver */
static bool preEraseMutexCreated_ = false;
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static returnStatus_t init( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t close( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                            DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t write( dSize destOffset, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t setPowerMode( const ePowerMode, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNextDriver );
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );

static bool        timeSlice(PartitionData_t const *, struct tagDeviceDriverMem const * const *);

/* ****************************************************************************************************************** */
/* CONSTANTS */
/* The driver table must be defined after the function definitions. The list below are the supported commands for this
 * driver. */
const DeviceDriverMem_t sDeviceDriver_eSpe =
{
   init,          // Init function - Creates mutex & calls lower drivers
   open,          // Open Command - Initializes the banked pointers and calls lower drivers.
   close,         // Close Command - For this implementation it only calls the lower level drivers.
   setPowerMode,  // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost */
   read,          // Read Command
   write,         // Write Command
   erase,         // Erases all of the banked memory partition.
   flush,         // Not used, calls the next driver.
   ioctl,         // ioctl function - Does Nothing for this implementation
   restore,       // Restore function not used for this module, but calls the next layer of code.
   timeSlice      // Will clear the 'next' bank of data if needed and the system is in an idle state.
};

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: init

   Purpose: Initializes the sect pre erase memory.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                                  partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: returnStatus_t � As defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t init( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, init ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   if ( preEraseMutexCreated_ == false ) /* If the mutex has not been created, create it. */
   {
      /* Create mutex to protect the cache driver modules critical section */
      if ( true == OS_MUTEX_Create(&preEraseMutex_) )
      {
         preEraseMutexCreated_ = true;
      } /* end if() */
   } /* end if() */

   return((*pNextDriver)->devInit(pParData, pNextDriver + 1)); /* Open the driver */
}
/***********************************************************************************************************************

   Function Name: open

   Purpose: The banked driver will find the bank with the newest data and set up the variable to correctly access the
            data.  The next layer is also opened.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: returnStatus_t � As defined by error_codes.h

   Side Effects: None

   Reentrant Code:  No

   Note: This should only be called at power up before the scheduler is running

 **********************************************************************************************************************/
static returnStatus_t open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver )
{
   returnStatus_t eRetVal;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, open ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   /* Open the driver */
   eRetVal = (*pNextDriver)->devOpen(pParData, pNextDriver + 1);
   if ( eSUCCESS == eRetVal )
   {
      /* Check here to validate the partition starting addr is on a sector boundary. */
      ASSERT((0 == (pParData->PhyStartingAddress % pParData->sAttributes.EraseBlockSize)));
   }

   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: close

   Purpose: The banked driver doesn�t have anything to close; the information is passed to the next driver.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.


   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t close( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, close ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNextDriver)->devClose(pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: read

   Purpose: reads data from the nv memory

   Arguments:
      uint8_t *pDest:  Location to write data
      dSize srcOffset:  Location of the source data in NV memory
      phCnt Cnt:  Number of bytes to Read from the NV Memory.
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to read.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                            DeviceDriverMem_t const * const * pNextDriver )
{
   returnStatus_t eRetVal;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, read ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   OS_MUTEX_Lock(&preEraseMutex_);
   eRetVal = (*pNextDriver)->devRead(pDest, srcOffset, cnt, pParData, pNextDriver + 1);
   OS_MUTEX_Unlock(&preEraseMutex_);

   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: write

   Purpose: Writes data to NV memory.  If the data is written to the 1st byte on a sector, that sector will be earased.

   Arguments:
      dSize destOffset:  Location of the data to write to the NV Memory
      uint8_t *pSrc:  Location of the source data to write to Flash
      lCnt Cnt:  Number of bytes to write to the NV Memory.
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t write( dSize destOffset, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNextDriver )
{
   returnStatus_t eRetVal = eSUCCESS;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, write ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   if ( cnt ) /* Any bytes to write? */
   {
      OS_MUTEX_Lock(&preEraseMutex_); /* Take the mutex */

      dSize offsetInSector = destOffset % pParData->sAttributes.EraseBlockSize;
      if (  (0 == offsetInSector) ||                                              /* Writing in a new Sector? */
            (offsetInSector + cnt) > pParData->sAttributes.EraseBlockSize)        /* Writing spans to a new sector? */
      {
         /* Erase the next sector */
         eRetVal = (*pNextDriver)->devErase(destOffset +
                                             ((pParData->sAttributes.EraseBlockSize - offsetInSector) %
                                               pParData->sAttributes.EraseBlockSize),
                                               pParData->sAttributes.EraseBlockSize, pParData, pNextDriver + 1);
      }
      if (eSUCCESS == eRetVal)
      {
         /* Write data. */
         eRetVal = (*pNextDriver)->devWrite(destOffset, pSrc, cnt, pParData, pNextDriver + 1);
      }
      OS_MUTEX_Unlock(&preEraseMutex_);
   }
   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: flush

   Purpose: The banked driver doesn�t have anything to flush; the information is passed to the next driver.

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, flush ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */
   return ((*pNextDriver)->devFlush(pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: erase

   Purpose: Erases a portion of the current bank of memory.

   Arguments:
      dSize destOffset - Offset into the partition to erase
      lCnt cnt - number of bytes to erase
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNextDriver )
{
   returnStatus_t eRetVal;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -e{613} : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   eRetVal = (*pNextDriver)->devErase(destOffset, cnt, pParData, pNextDriver + 1);
   if ( eSUCCESS == eRetVal )
   {
      eRetVal = flush(pParData, pNextDriver); /* The next driver could be a device that needs to be flushed. */
   }
   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: setPowerMode

   Purpose: The banked driver doesn�t have a power mode; the information is passed to the next driver.

   Arguments:
      enum ePwrMode � uses ePowerMode enumeration
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t setPowerMode( const ePowerMode ePwrMode, PartitionData_t const *pParData,
                                    DeviceDriverMem_t const * const * pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, setPowerMode ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */
   return ((*pNextDriver)->devSetPwrMode(ePwrMode, pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: ioctl

   Purpose: The banked driver doesn't support ioctl commands

   Arguments:
      void *pCmd � Command to execute:
      void *pData � Data to device.
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const * pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, ioctl ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNextDriver)->devIoctl(pCmd, pData, pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: restore

   Purpose: The banked driver doesn't support the restore command

   Arguments:
      PartitionData_t const *pParData � Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver � Points to the next driver�s table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, restore ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNextDriver)->devRestore(lDest, cnt, pParData, pNextDriver + 1));
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
/*lint -efunc( 715, timeSlice) argument not referenced */
/*lint -efunc( 818, timeSlice) could be ptr to const */
static bool timeSlice(PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver)
{
   return(0); // Not busy!
}
