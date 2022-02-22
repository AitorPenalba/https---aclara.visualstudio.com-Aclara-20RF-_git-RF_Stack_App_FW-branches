/***********************************************************************************************************************

   Filename:   dvr_banked.c

   Global Designator: DVR_BANKED_

   Contents:   Contains all of the banked memory driver functions.  Each write command will read the data from the
               active bank, modify the data with the new data to be written and then write the data to the next bank.
               Optionally (in the partition config), the oldest bank is automatically cleared and that bank is ready to
               be written.

               Note:  The last x number of bytes in the sector allocated for the banked memory is used for meta data.
               Any data written to the last x locations will be discarded.

   NOTES:
      1. The number of banks must evenly fit into a sector.  If the sector size is 4096 bytes, valid bank sizes are
         4, 8, 16, 32, 64, 128, 256, 512, 1024 and 2048.
      2. The minimum bank size is 4 bytes.  xx bytes are needed for meta data.
      3. If a bank is larger than a sector size, the bank size must be a multiple of the sector size.
      4. The minimum number of banks allowed for this driver is 2.  At least two banks must be defined.
      5. The maximum number of banks allowed for this driver is 254.  Bank sequence number 0 is reserved
      6. This driver needs a time slice from the main loop to clear the next bank of data as needed.

 ***********************************************************************************************************************
   Copyright (c) 2012 - 2021 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
   or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
   Revision History:
   20110505, v0.1, KAD -   Initial Release
   20130820, v0.2, KAD -   Fixed timeSlice() function.  This module now handles banks that are an even multiple of the
                           sector size and bank sizes that are evenly divisible by the sector size.
 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#ifndef __BOOTLOADER
#if ( RTOS_SELECTION == MQX_RTOS ) 
#include <mqx.h>
#endif
#else
#include <string.h>
#endif   /* BOOTLOADER  */

#define dvr_banked_GLOBAL
#include "dvr_banked.h"
#include "dvr_banked_cfg.h"
#undef  dvr_banked_GLOBAL

#include "partition_cfg.h"

#ifdef TM_BANKED_UNIT_TEST
#ifndef __BOOTLOADER
#include "DBG_SerialDebug.h"
#endif   /* BOOTLOADER  */
#endif

#define DEBUG_BANK_NUMBER 0
#if ( DEBUG_BANK_NUMBER == 1 )
#warning "Don't release with DEBUG_BANK_NUMBER set!"
#include <stdio.h>
#endif
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define ERASED_BANK_STATE        ((uint8_t)0)  /* This value in meta data indicates that the bank is erased. */

/* For Unit Testing, we need to return a FAILURE in place of the assert.  */
#ifdef TM_DVR_BANKED_UNIT_TEST
#undef ASSERT
#define ASSERT(__e) if (!(__e)) return(eFAILURE)
#endif
#define NUM_OF_BANKS_OFFSET   ((uint8_t)1)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#ifdef TM_BANKED_DRIVER_VALIDATE
/* Updated during the initialize command to determine the largest erase sector size.  During the open command this
 * value will be used to malloc a buffer for read-modify write operations. */
static lCnt rdModWrBufSize_ = 0; /* Size is in bytes */
#endif

/* The size of this buffer MUST be at least 2 bytes.  The larger the buffer the less overhead of communicating with the
 * NV memory device.  This buffer should also be a power of 2.  */
static uint8_t rdModWrBuf_[256]; /* Points to buffer used for the read-mod-write operation. */

#if RTOS
static OS_MUTEX_Obj bankMutex_; /* Serialize access to the Banked driver */
static bool bankedMutexCreated_ = false;
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static returnStatus_t bnk_init( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_close( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                            DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_write( dSize destOffset, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_setPowerMode( const ePowerMode, PartitionData_t const *pParData, DeviceDriverMem_t const
                           * const *pNextDriver );
static returnStatus_t bnk_ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const *pNextDriver );
static returnStatus_t bnk_restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const
                           * const *pNextDriver );

static returnStatus_t findCurrentBank( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver );
static bool           bnk_timeSlice(PartitionData_t const *, DeviceDriverMem_t const * const *);

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* The driver table must be defined after the function definitions. The list below are the supported commands for this
 * driver. */
const DeviceDriverMem_t sDeviceDriver_eBanked ={
   bnk_init,          // Init function - Creates mutex & calls lower drivers
   bnk_open,          // Open Command - Initializes the banked pointers and calls lower drivers.
   bnk_close,         // Close Command - For this implementation it only calls the lower level drivers.
   bnk_setPowerMode,  // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost */
   bnk_read,          // Read Command
   bnk_write,         // Write Command
   bnk_erase,         // Erases all of the banked memory partition.
   bnk_flush,         // Not used, calls the next driver.
   bnk_ioctl,         // ioctl function - Does Nothing for this implementation
   bnk_restore,       // Restore function not used for this module, but calls the next layer of code.
   bnk_timeSlice      // Will clear the 'next' bank of data if needed and the system is in an idle state.
};
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: init

   Purpose: Initializes the banked memory.  The initialize function should only be called once at power up.

   Arguments:
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                                  partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: returnStatus_t  As defined by error_codes.h

   Side Effects: N/A

   Reentrant Code: No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t bnk_init( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   returnStatus_t   eRetVal;                /* Return Value */

#if RTOS
   if ( bankedMutexCreated_ == false ) /* If the mutex has not been created, create it. */
   {
      /* Create mutex to protect the banked driver modules critical section */
      if ( true == OS_MUTEX_Create(&bankMutex_) )
      {
         bankedMutexCreated_ = true;
      } /* end if() */
   }
#endif

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, init ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   eRetVal = (*pNextDriver)->devInit(pParData, pNextDriver + 1); /* Open the driver */

#ifdef TM_BANKED_DRIVER_VALIDATE
   /* Check if the erase block is larger than the value recorded so far.  This is used so that the banked driver will
      create a buffer large enough to handle the worst case flash memory erase sector size in the project.  */
   if ( pParData->sAttributes.EraseBlockSize > rdModWrBufSize_ ) /* Is the current device erase size the largest so far? */
   {
      rdModWrBufSize_ = pParData->sAttributes.EraseBlockSize; /* Use this new larger value */
   }
#endif

   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: open

   Purpose: The banked driver will find the bank with the newest data and set up the variable to correctly access the
            data.  The next layer is also opened.

   Arguments:
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: returnStatus_t  As defined by error_codes.h

   Side Effects: None

   Reentrant Code:  No

   Note: This should only be called at power up before the scheduler is running

 **********************************************************************************************************************/
static returnStatus_t bnk_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   returnStatus_t eRetVal;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, open ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   /* Open the driver */
   eRetVal = (*pNextDriver)->devOpen(pParData, pNextDriver + 1);
   if ( eSUCCESS == eRetVal )
   {
      /* Check here to validate the partition starting addr is on a sector boundary and more than one sector is used. */
      ASSERT(  (0 == (pParData->PhyStartingAddress % pParData->sAttributes.EraseBlockSize)) ||
               ((pParData->lSize * pParData->sAttributes.NumOfBanks) > pParData->sAttributes.EraseBlockSize) ||
               ((pParData->lSize * pParData->sAttributes.NumOfBanks) % pParData->sAttributes.EraseBlockSize));

      eRetVal = findCurrentBank(pParData, pNextDriver);
   }

#ifdef TM_BANKED_DRIVER_VALIDATE
   if (rdModWrBufSize_ != sizeof(rdModWrBuf_))
   {
//      eRetVal = eNV_BANKED_DRIVER_CFG_ERROR;
   }
#endif

   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: close

   Purpose: The banked driver doesnt have anything to close; the information is passed to the next driver.

   Arguments:
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.


   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_close( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, close ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNextDriver)->devClose(pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: read

   Purpose: reads data from the current bank of memory

   Arguments:
      uint8_t *pDest:  Location to write data
      dSize srcOffset:  Location of the source data in NV memory
      phCnt Cnt:  Number of bytes to Read from the NV Memory.
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to read.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                            DeviceDriverMem_t const * const *pNextDriver )
{
   returnStatus_t eRetVal;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, read ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

#ifdef TM_DVR_BANKED_UNIT_TEST
   /* Check the range - this is for unit testing where the partition mgr is by-passed. */
   ASSERT(cnt < pParData->lSize); /* One byte is used for meta data, so it is < and NOT <= */
#endif

#if RTOS
   OS_MUTEX_Lock(&bankMutex_); // Function will not return if it fails
#endif
   eRetVal = (*pNextDriver)->devRead(pDest,
                                     srcOffset + /* Source data offset */
                                     pParData->sAttributes.pMetaData->BankOffset, /* Bank offset */
                                     cnt,
                                     pParData,
                                     pNextDriver + 1);
#if RTOS
   OS_MUTEX_Unlock(&bankMutex_); // Function will not return if it fails
#endif

   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: write

   Purpose: Writes data to the next bank of memory by reading the current bank, modifying the data and writing it to
            the next bank.  Once this is complete, the next bank is optionally be cleared as specified by the partition
            table.

      NOTE: Data written at the end of the partition in the meta data will be overwritten by the meta data and will
      appear as garbage when read back!  Writing to this area will not corrupt the banked module's data, but the data
      that the user attempted to put there will be lost.

   Arguments:
      dSize destOffset:  Location of the data to write to the NV Memory
      uint8_t *pSrc:  Location of the source data to write to Flash
      lCnt Cnt:  Number of bytes to write to the NV Memory.
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_write( dSize destOffset, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const *pNextDriver )
{
   returnStatus_t eRetVal = eSUCCESS;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, write ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   /* The pointer must be initialized in the open command.This should be found at integration testing. */
   ASSERT(NULL != rdModWrBuf_);

#ifdef TM_DVR_BANKED_UNIT_TEST
   /* Check the range - this is for unit testing where the partition mgr is by-passed. */
   ASSERT(cnt < pParData->lSize); /* One byte is used for meta data, so it is < and NOT <= */
#endif

   if ( cnt ) /* Any bytes to write? */
   {
#if RTOS
      OS_MUTEX_Lock(&bankMutex_); /* Take the mutex */ // Function will not return if it fails
#endif
      {
         /* Increment the BankOffset to point to the next bank of data.  Note:  This will wrap around back to the first
            bank of data. */
         dSize nextBankOffset = (((pParData->sAttributes.pMetaData->BankOffset / pParData->lSize) + 1) %
                                 (pParData->sAttributes.NumOfBanks + 1))* pParData->lSize;
         lCnt bytesLeftInBank;
         lCnt numBytesToOperateOn;
         lCnt indexIntoBank;

         /* Performs a read-modify-write operation.  The 1st sector of the current bank is read, modified with the user
            information passed in (pSrc) and then written to the next bank.  Since a bank of memory may have more than
            1 sector, a loop is created to perform this step for each sector of a bank. */
         for ( indexIntoBank = 0, bytesLeftInBank = pParData->lSize;
               (0 != bytesLeftInBank) && (eSUCCESS == eRetVal);
               indexIntoBank += numBytesToOperateOn)            /*lint !e441  loop counter is correct */
         {
            /* Calculate the number of bytes for read-modify-write operation. */
            numBytesToOperateOn = MINIMUM(sizeof(rdModWrBuf_), bytesLeftInBank);

            /* Read a sector of the current banks data */
            eRetVal = (*pNextDriver)->devRead(rdModWrBuf_, indexIntoBank + pParData->sAttributes.pMetaData->BankOffset,
                        numBytesToOperateOn, pParData, pNextDriver + 1);
            if ( eSUCCESS == eRetVal )
            {
               /* Perform the modify of the data before writing.  The modify will copy data from pSrc to the buffer. */
               /* Note:  The data to be modified could be in any sector of the bank and could span multiple banks. */
               if (  (0 != cnt) &&
                     ((destOffset >= indexIntoBank) && (destOffset < (indexIntoBank + numBytesToOperateOn))) )
               {
                  lCnt offsetToBuf = destOffset - indexIntoBank;
                  lCnt byteCopied = MINIMUM(cnt, sizeof(rdModWrBuf_) - offsetToBuf);
                  /* Modify the data read with the new data to be written. */
                  (void)memcpy(&rdModWrBuf_[0] + offsetToBuf, pSrc, byteCopied);
                  pSrc += byteCopied;
                  destOffset += byteCopied;
                  cnt -= byteCopied;
               }
               bytesLeftInBank -= numBytesToOperateOn;
               if ( 0 == bytesLeftInBank ) /* If this is the last write to the new bank, update the meta data. */
               {
                  bankMetaData_t sBank; /* Bank meta data. */

                  (void)memset(&sBank, 0, sizeof(sBank));  /* Clear the Bank meta data */
                  sBank.sequence = pParData->sAttributes.pMetaData->CurrentBankSequence + 1;
                  if (ERASED_BANK_STATE == sBank.sequence)   /* Did the sequence wrap around? */
                  {
                     sBank.sequence++; /* Increment to the next sequence. */
                  }
                  pParData->sAttributes.pMetaData->CurrentBankSequence = sBank.sequence;
                  /* The statement below will write the meta data's sequence number to the last byte of the partition.
                     If this value should change in size or more data added, the following line will have to change. */
                  (void)memcpy(&rdModWrBuf_[0] + numBytesToOperateOn - sizeof(sBank), &sBank, sizeof(sBank));
               }

               /* Write the buffer to the 'next' bank. */
               eRetVal = (*pNextDriver)->devWrite(indexIntoBank + nextBankOffset, rdModWrBuf_, numBytesToOperateOn,
                                                  pParData, pNextDriver + 1);
            }
         }

         if ( eSUCCESS == eRetVal )
         {
            pParData->sAttributes.pMetaData->BankOffset = nextBankOffset;

            if (  pParData->PartitionType.AutoEraseBank &&            //Auto erase enabled?
                  (0 == (nextBankOffset % EXT_FLASH_SECTOR_SIZE)) )   //On the first bank in the sector?
            {  //Erase previous sector
               dSize eraseBankOffset; //sector offset to erase
               lCnt  numBytesToErase = EXT_FLASH_SECTOR_SIZE;  //Assume # of bytes in a bank is less than a sector size
               if (pParData->lSize > EXT_FLASH_SECTOR_SIZE)    //Is this bank larger than a sector?
               {
                  numBytesToErase = pParData->lSize;  //Clear multiple sectors
               }
               eraseBankOffset =  (pParData->sAttributes.NumOfBanks + 1)* pParData->lSize; //Total partition size
               eraseBankOffset += nextBankOffset;
               eraseBankOffset -= numBytesToErase;
               eraseBankOffset %= (pParData->sAttributes.NumOfBanks + 1)* pParData->lSize;
               (void)(*pNextDriver)->devErase(eraseBankOffset, numBytesToErase, pParData, pNextDriver + 1);
            }
         }
#if RTOS
       OS_MUTEX_Unlock(&bankMutex_); // Function will not return if it fails
#endif
      }
   } /*lint !e456 Mutex unlocked if successfully locked */
   return (eRetVal); /*lint !e454 !e456 Mutex unlocked if successfully locked */
}
/***********************************************************************************************************************

   Function Name: flush

   Purpose: The banked driver doesnt have anything to flush; the information is passed to the next driver.

   Arguments:
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, bnk_flush ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */
   return ((*pNextDriver)->devFlush(pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: erase

   Purpose: Erases a portion of the current bank of memory.

   Arguments:
      dSize destOffset - Offset into the partition to erase
      lCnt cnt - number of bytes to erase
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const *pNextDriver )
{
   returnStatus_t eRetVal = eFAILURE;

   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -e{613} : pNextDriver can never be NULL in production code.  The ASSERT catches this. */

   if ( ( (cnt != 0) && ( destOffset + cnt ) <= ( pParData->lSize * ( pParData->sAttributes.NumOfBanks + 1 ) ) ) )/* Validate total bytes to erase
                                                                                                                     within partition limits. */
   {
      if ( cnt == pParData->lSize * ( pParData->sAttributes.NumOfBanks + 1 ) )   /* Erasing the whole partition?  */
      {
         eRetVal = (*pNextDriver)->devErase(0, cnt, pParData, pNextDriver + 1);
      }
      else
      {
         eRetVal = (*pNextDriver)->devErase(pParData->sAttributes.pMetaData->BankOffset + destOffset, cnt, pParData, pNextDriver + 1);
      }
      if ( eSUCCESS == eRetVal )
      {
         eRetVal = bnk_flush(pParData, pNextDriver); /* The next driver could be a device that needs to be flushed. */
      }
   }
   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: setPowerMode

   Purpose: The banked driver doesnt have a power mode; the information is passed to the next driver.

   Arguments:
      enum ePwrMode  uses ePowerMode enumeration
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_setPowerMode( const ePowerMode ePwrMode, PartitionData_t const *pParData,
                                    DeviceDriverMem_t const * const *pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, setPowerMode ) : pNextDriver can never be NULL in production code.  The ASSERT catches this. */
   return ((*pNextDriver)->devSetPwrMode(ePwrMode, pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: ioctl

   Purpose: The banked driver doesn't support ioctl commands

   Arguments:
      void *pCmd  Command to execute:
      void *pData  Data to device.
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData,
                             DeviceDriverMem_t const * const *pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, ioctl ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNextDriver)->devIoctl(pCmd, pData, pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: restore

   Purpose: The banked driver doesn't support the restore command

   Arguments:
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t bnk_restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, restore ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNextDriver)->devRestore(lDest, cnt, pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: findCurrentBank

   Purpose: Initializes the banked memory.  This function will determine which bank of NV memory contains the most
            current information.  The meta data contains the information that will determine which bank to use.  The
            value in the meta data ranges from 0-255 where 0 is erased.  The bank to use will be the latest in a chain
            of values that will roll back to 1.

            Example: 4 Banks of data where the meta data is 0, 1, 2, 3 ---  The 4th bank is the latest data
            Example: 4 Banks of data where the meta data is 1, 0, 254, 255 ---  The 1st bank is the latest data
            Example: 4 Banks of data where the meta data is 1, 2, 254, 255 ---  The 2nd bank is the latest data
            Example: 4 Banks of data where the meta data is 0, 0, 0, 0 ---  1st bank will be chosen
            Example: 4 Banks of data where the meta data is 1, 2, 3, 0 ---  The 3rd bank is the latest data
            Example: 4 Banks of data where the meta data is 11, 12, 13, 0 ---  The 3rd bank is the latest data
            Example: 4 Banks of data where the meta data is 255, 252, 253, 254 ---  The 1st bank is the latest data

   Arguments:
      PartitionData_t const *pParData Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver Points to the next driver's table.

   Returns: returnStatus_t  As defined by error_codes.h

   Side Effects: NV memory will be accessed (CPU Time).

   Reentrant Code: No (this should only be called at power up before the scheduler is running)

 **********************************************************************************************************************/
static returnStatus_t findCurrentBank( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   /*lint -efunc( 613, findCurrentBank ) : pNextDriver is checked for NULL prior to calling this function. */

   returnStatus_t eRetVal = eSUCCESS;

   bankMetaData_t sBank;      /* Contains the meta data that is used to determine which bank to use */
   bankMetaData_t nextBank;   /* Contains the next sequence number that would be the lastest bank of data.  */
   uint8_t u8BankCount;       /* Loop Counter */
   dSize bankMetaOffset;      /* Contains the meta data offset in the bank of NV memory being evaluated. */
   bool gotValidSequence;
   /* 0 = Don't have a valid sequence #, 1 = Got a valid sequence number.  Initialized to 0, and once the 1st sequence
      number is found (valid is not erased i.e. 0). */

   /* Need to read from each bank until the bank last updated has been found.  The loop will execute at least once
      setting BankOffset for case where no banks are used. */
   for ( u8BankCount = 0,                                            /* Initialize the loop Control Variable. */
         bankMetaOffset = pParData->lSize - sizeof(bankMetaData_t),  /* Get the offset to start with. */
         pParData->sAttributes.pMetaData->BankOffset = 0,            /* Assume 1st bank is the latest */
         pParData->sAttributes.pMetaData->CurrentBankSequence = 1,   /* Assume the nv memory is erased & starts at 1 */
         gotValidSequence = false;                                   /* No valid sequence # has been read yet. */
         u8BankCount < (pParData->sAttributes.NumOfBanks + NUM_OF_BANKS_OFFSET); /* 0 based (0 = 1 bank) */
         u8BankCount++, bankMetaOffset += pParData->lSize )          /* Adjust bank found and offset. */
   {
      /* Read the bank sequence number */
      eRetVal = (*pNextDriver)->devRead((uint8_t *)&sBank, bankMetaOffset, (lCnt)sizeof(sBank), pParData, pNextDriver + 1);
      if ( eSUCCESS != eRetVal ) /* Validate eRetVal - was the meta data read properly? */
      {
         break; /* Had trouble reading, abort */
      }

      if ( ERASED_BANK_STATE != sBank.sequence )   /* Only check the data if the sequence number is valid. */
      {
         if (gotValidSequence)     /* Has a valid sequence number been read yet? */
         {
            if (nextBank.sequence != sBank.sequence)  /*lint !e530  Is this the sequence we're looking for? */
            {
               break;   /* The next sequence was not what we're looking for, so we must be at the end. */
            }
         }
         gotValidSequence = true;   /* A valid sequence number has been read. */
         /* Save the BankOffset */
         pParData->sAttributes.pMetaData->BankOffset = bankMetaOffset - (pParData->lSize - sizeof(bankMetaData_t));
         pParData->sAttributes.pMetaData->CurrentBankSequence = sBank.sequence;  /* Save the sequence # */
         nextBank.sequence = sBank.sequence + 1;   /* Look for the next sequence */
         if (0 == nextBank.sequence)               /* Did we wrap back around to 0? (0 = Erased and isn't valid) */
         {
            nextBank.sequence++;                   /* Sequence was 0, so look for sequence number 1. */
         }
      }
   }
   {
#if ( DEBUG_BANK_NUMBER == 1 )   /* For debugging nextBank.sequence issues only  */
      if ( eSUCCESS == eRetVal )
      {
         ( void )printf( "Partition: 0x%08lx, next bank = 0x%02x\n", pParData->PhyStartingAddress, nextBank.sequence );
      }
      else
      {
         ( void )printf( "Can't find next sequence for 0x%08lx\n", pParData->PhyStartingAddress );
      }
#endif
   }
   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: bnk_timeSlice

   Purpose: Common API function, allows for the driver to perform any housekeeping that may be required.  For this
            module, nothing is to be completed.

   Note: If the next sector is to be erased, the NV memory flag will still be set.  The next time a bank is written to,
         the NV memory flag will be cleared.  This does mean that if a power outage occurs, at the next power up, the
         next sector will be re-erased.

   Arguments: PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver

   Returns: bool - 0 (false)

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static bool bnk_timeSlice(PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver)
{
   return ((*pNextDriver)->devTimeSlice(pParData, pNextDriver + 1));
}
/* ****************************************************************************************************************** */
/* Unit Test */

/***********************************************************************************************************************

   Function Name: DVR_BANKED_unitTest

   Purpose: Performs unit testing - After this test completes, all data will be lost!  The processor will also be reset.

      NOTE:  This unit test should be run with multiple configurations such as banked size and number of banks.

   Arguments: none

   Returns: none

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void DVR_BANKED_unitTest(void)
{
#ifdef TM_BANKED_UNIT_TEST

   PartitionData_t const   *pParTbl;
   uint16_t                  i;                      /* Index counter for loops */
   uint8_t                   wrData[PART_NV_APP_BANKED_BANKS + 1];
   uint8_t                   rdData[PART_NV_APP_BANKED_BANKS + 1];
   returnStatus_t          callRetVal;

   for (i = 0; i < sizeof(wrData); i++)  /* Fill data with all 0xFF to force bank increments when writing later. */
   {
      wrData[i] = 0xFF;
   }
   DBG_logPrintf('U', "  Unit Testing the Banked Driver:");
   OS_TASK_Sleep ( 100 );
   DBG_logPrintf('U', "  NOTE: ENBF may be set multiple times due to re-initializing the driver.");
   OS_TASK_Sleep ( 1000 );
   /* Step 1:  Erase the entire NV device */
   DBG_logPrintf('U', "  Erasing Device");
   OS_TASK_Sleep ( 100 );
   (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_WHOLE_NV, 0);
   (void)PAR_partitionFptr.parErase(0, EXT_FLASH_SIZE, pParTbl);

   /* Step 2:  Initialize the Banked Area - Print the Address of the Current Bank. */
   DBG_logPrintf('U', "  Open Banked Driver - %i Banks, %i Bytes, %i Bytes/Bank",
                  PART_NV_APP_BANKED_BANKS,
                  PART_NV_APP_BANKED_SIZE * PART_NV_APP_BANKED_BANKS,
                  PART_NV_APP_BANKED_SIZE);
   (void)PAR_partitionFptr.parInit();
   (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_NV_APP_BANKED, 0);
   DBG_printfNoCr("  Meta Data: Offset = %i, Seq = %i, EraseNextBankFlag (ENBF) = ",
                           sBanked_MetaData.BankOffset,
                           sBanked_MetaData.CurrentBankSequence);
#if 0
   if (NEXT_SECTOR_ERASE == sBanked_MetaData.eraseNextBank)
   {
      DBG_logPrintf('U', "1");
   }
   else
   {
      DBG_logPrintf('U', "0");
   }
#endif

   /* Step 3:  Write to bank such that the bank increments, monitor the Erase Next Bank and Seq # */
   DBG_logPrintf('U', "  Writing Data that requires a bank increment");
   for (i = 0; i < 258; i++)
   {
      wrData[i%sizeof(wrData)] = 0;
      rdData[i%sizeof(wrData)] = 0x55;
      DBG_printfNoCr("    Step %3i/258: ", i+1);
      callRetVal = PAR_partitionFptr.parWrite((dSize)0, &wrData[0], (lCnt)sizeof(wrData), pParTbl);
      DBG_printfNoCr("Resp = %i, Offset = %6i, Seq = %3i, ENBF = ",
               callRetVal,
               sBanked_MetaData.BankOffset,
               sBanked_MetaData.CurrentBankSequence);

#if 0
      if (NEXT_SECTOR_ERASE == sBanked_MetaData.eraseNextBank)
      {
         DBG_printfNoCr("1/");
      }
      else
      {
         DBG_printfNoCr("0/");
      }
#endif

      /* Re-initialize as if coming up from a power outage. */
      (void)memset(&sBanked_MetaData, 0, sizeof(sBanked_MetaData));  /* wipe out the meta data to make sure it restores */
      (void)PAR_partitionFptr.parInit();
      (void)PAR_partitionFptr.parOpen(&pParTbl, ePART_NV_APP_BANKED, 0);

      callRetVal = PAR_partitionFptr.parRead(&rdData[0], (dSize)0, (lCnt)sizeof(rdData), pParTbl);
      if ((eSUCCESS != callRetVal) || (0 != memcmp(&wrData[0], &rdData[0], sizeof(rdData))))
      {
         DBG_logPrintf('U', "    Read Verify Failed!");
      }
      wrData[i%sizeof(wrData)] = 0xFF;
      (void)PAR_partitionFptr.parTimeSlice();
      OS_TASK_Sleep ( 30 );
      (void)PAR_partitionFptr.parTimeSlice();
#if 0
      if (NEXT_SECTOR_ERASE == sBanked_MetaData.eraseNextBank)
      {
         DBG_printfNoCr("1");
      }
      else
      {
         DBG_printfNoCr("0");
      }
#endif
      DBG_logPrintf('U', ", Seq Init: %3d", sBanked_MetaData.CurrentBankSequence);
      OS_TASK_Sleep ( 10 );
   }

   (void)PAR_partitionFptr.parTimeSlice();
   OS_TASK_Sleep ( 30 );
   (void)PAR_partitionFptr.parTimeSlice();

   /* Step 4:  Write to bank such that the bank does NOT increment (write same data twice). */
   wrData[0] = 0x0F;
   DBG_logPrintf('U', "  Writing Data that does NOT increment bank or Seq #:");
   DBG_printfNoCr("    Init Value 0x0F: ");
   callRetVal = PAR_partitionFptr.parWrite((dSize)0, &wrData[0], (lCnt)sizeof(wrData), pParTbl);
   DBG_printfNoCr("Resp = %i, Offset = %6i, Seq = %3i, ENBF = ",
            callRetVal,
            sBanked_MetaData.BankOffset,
            sBanked_MetaData.CurrentBankSequence);
#if 0
   if (NEXT_SECTOR_ERASE == sBanked_MetaData.eraseNextBank)
   {
      DBG_logPrintf('U', "1");
   }
   else
   {
      DBG_logPrintf('U', "0");
   }
#endif
   wrData[0] = 0xFF;
   callRetVal = PAR_partitionFptr.parWrite((dSize)0, &wrData[0], (lCnt)sizeof(wrData), pParTbl);
   DBG_printfNoCr("    New Value 0x1F: ");
   DBG_printfNoCr("Resp = %i, Offset = %6i, Seq = %3i, ENBF = ",
            callRetVal,
            sBanked_MetaData.BankOffset,
            sBanked_MetaData.CurrentBankSequence);

#if 0
   if (NEXT_SECTOR_ERASE == sBanked_MetaData.eraseNextBank)
   {
      DBG_printfNoCr("1/");
   }
   else
   {
      DBG_printfNoCr("0/");
   }
#endif

   (void)PAR_partitionFptr.parTimeSlice();
   OS_TASK_Sleep ( 60 );
   (void)PAR_partitionFptr.parTimeSlice();

#if 0
   if (NEXT_SECTOR_ERASE == sBanked_MetaData.eraseNextBank)
   {
      DBG_logPrintf('U', "1");
   }
   else
   {
      DBG_logPrintf('U', "0");
   }

#endif
   OS_TASK_Sleep ( 10 );

   /* Step 5:  Write to last byte. */
   wrData[0] = 0x0F;
   DBG_printfNoCr("  Writing Data to the last byte of the Banked Memory - ");
   (void)PAR_partitionFptr.parWrite((dSize)((PART_NV_APP_BANKED_SIZE - sizeof(bankMetaData_t)) - 1),
                                 &wrData[0], (lCnt)1, pParTbl);
   rdData[0] = 0xFF;
   (void)PAR_partitionFptr.parRead(&rdData[0], (dSize)((PART_NV_APP_BANKED_SIZE - sizeof(bankMetaData_t)) - 1),
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
   wrData[0] = 0x0F;
   DBG_printfNoCr("  Writing Data that overruns Banked Memory - ");
   callRetVal = PAR_partitionFptr.parWrite((dSize)((PART_NV_APP_BANKED_SIZE - sizeof(bankMetaData_t)) - 1),
                                        &wrData[0], (lCnt)sizeof(wrData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_printfNoCr("WR Passed, ");
   }
   else
   {
      DBG_printfNoCr("WR FAILED, ");
   }
   callRetVal = PAR_partitionFptr.parRead(&rdData[0], (dSize)((PART_NV_APP_BANKED_SIZE - sizeof(bankMetaData_t)) - 1),
                                        (lCnt)sizeof(rdData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_logPrintf('U', "RD Passed, ");
   }
   else
   {
      DBG_logPrintf('U', "RD FAILED, ");
   }

/* Step 7:  Writing outside of Banked Memory. */
   wrData[0] = 0x0F;
   DBG_printfNoCr("  Writing outside of Banked Memory - ");
   callRetVal = PAR_partitionFptr.parWrite((dSize)((PART_NV_APP_BANKED_SIZE - sizeof(bankMetaData_t)) + 1),
                                        &wrData[0], (lCnt)sizeof(wrData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_printfNoCr("WR Passed, ");
   }
   else
   {
      DBG_printfNoCr("WR FAILED, ");
   }
   callRetVal = PAR_partitionFptr.parRead(&rdData[0], (dSize)((PART_NV_APP_BANKED_SIZE - sizeof(bankMetaData_t)) + 1),
                                       (lCnt)sizeof(rdData), pParTbl);
   if (callRetVal != eSUCCESS)
   {
      DBG_logPrintf('U', "RD Passed, ");
   }
   else
   {
      DBG_logPrintf('U', "RD FAILED, ");
   }

   DBG_logPrintf('U', "Resetting...");
   DBG_logPrintf('U', " ");
   OS_TASK_Sleep ( 3000 );
   RESET(); // Reset the processor to re-initialize the NV memory and re-create files properly.
#endif
}
