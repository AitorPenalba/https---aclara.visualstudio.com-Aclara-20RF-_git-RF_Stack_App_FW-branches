/***********************************************************************************************************************

   Filename:   IF_intFlash.c

   Global Designator: IF_

   Contents: Internal Flash Driver - Writes to the internal flash memory for the K60.

   Notes about the K60 Flash Memory:
      - Erase Block Size:  128KB or 256KB
      - Sector Size:       4096

 ***********************************************************************************************************************
   A product of Aclara Technologies LLC Confidential and Proprietary Copyright 2013-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author Karl Davlin

   $Log$ KAD Created November 14, 2013

 ***********************************************************************************************************************
   Revision History:
   v0.1 - KAD 11/14/2013 - Initial Release

   @version    0.1
   #since      2013-11-13
 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#ifndef __BOOTLOADER
#include <mqx.h>
#include <bsp.h>
#include "DBG_SerialDebug.h"
#else
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#include <MK66F18.h>
#else
#include <MK24F12.h>
#endif
#include <string.h>
#endif   /* BOOTLOADER  */
#include "IF_intFlash.h"
#include "partition_cfg.h"
#include "dvr_sharedMem.h"
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define ASSERT(test)                                                 /* This should be moved to a file called ASSERT! */
#define WRITE_BLOCK_SIZE_IN_BYTES   ((uint8_t)8)                     /* Writes 8 bytes at a time */

#ifndef __BOOTLOADER
#define DEBUG_CLOCK_CHANGE          0                                /* Set to 1 to issue printf when clock speed changes. */
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( ( DCU == 1 ) && ( HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A ) ) ) )
#define MAX_SEM_WAIT_mS             ((uint32_t)2*1000)               /* Wait 2 seconds */
#endif

#endif   /* __BOOTLOADER   */
/* See "K60 Sub-Family Reference Manual" - 30.4.12.14 Swap Control command to get the FCCOBx value. */
/*lint -esym(750,FS_CURRENT_MODE_REG,FS_CURRENT_BLOCK_REG,FS_NEXT_BLOCK_REG,FS_SWAP_CONTROL_CODE_REG,FS_FLASH_CMD_REG)   */
#define FS_CURRENT_MODE_REG         ((eFS_CurrentSwapMode_t)FTFE_BASE_PTR->FCCOB5)
#define FS_CURRENT_BLOCK_REG        ((eFS_CurrentSwapBlockStatus_t)FTFE_BASE_PTR->FCCOB6)
#define FS_NEXT_BLOCK_REG           ((eFS_NextSwapBlockStatus_t)FTFE_BASE_PTR->FCCOB7)
#define FS_SWAP_CONTROL_CODE_REG    FTFE_BASE_PTR->FCCOB4
#define FS_FLASH_CMD_REG            FTFE_BASE_PTR->FCCOB0

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/*lint -esym(751,fCmd_e)   The structure may not be referenced, however, some of the elements are referenced. */
/*lint -esym(749,fCmd_e::*)   Some of the elements are not referenced   */
typedef enum
{
   /*lint --e{749}   Some of the elements are not referenced   */
   FC_READ_1s_BLOCK_e = ((uint8_t)0),           /* Verify that a program flash or data flash block is erased.  FlexNVM block must not be partitioned
                                                    for EEPROM. */
   FC_READ_1s_SECTION_e,                        /* Verify that a given number of program flash or data flash locations from a starting address. */
   FC_PROGRAM_CHECK_e,                          /* Tests previously programmed phrases at margin read levels. */
   FC_READ_SOURCE_e,                            /* Read 8 bytes from program flash IFR, data flash IFR, or version ID. */
   FC_PROGRAM_PHASE_e = ((uint8_t)0x7),         /* Program 8 bytes in a program flash block or a data flash block. */
   FC_ERASE_FLASH_BLOCK_e,                      /* Erase a program flash block or data flash block.  An erase of any flash block is only possible when
                                                   unprotected. FlexNVM block must not be partitioned for EEPROM. */
   FC_ERASE_FLASH_SECTOR_e,                     /* Erase all bytes in a program flash or data flash sector */
   FC_PROGRAM_SECTION_e = ((uint8_t)0x0B),      /* Program data from the Section Program Buffer to a program flash or data flash */
   FC_READ_1s_ALL_BLOCKS_e = ((uint8_t)0x40),   /* Verify that all program flash, data flash blocks, EEPROM backup data records, and data flash IFR
                                                   are erased then release MCU security. */
   FC_READ_ONCE_e,                              /* Read 8 bytes of a dedicated 64 byte field in the program flash 0 IFR. */
   FC_PROGRAM_ONCE_e = ((uint8_t)0x43),         /* One-time program of 8 bytes of a dedicated 64-byte field in the program flash 0 IFR. */
   FC_ERASE_ALL_BLOCKS_e,                       /* Erase all program flash, data flash blocks, FlexRAM, EEPROM backup data records, and data flash
                                                   IFR.  Then, verify-erase and release MCU security.  NOTE: An erase is only possible when all memory
                                                   locations are unprotected. */
   FC_VERIFY_BACKDOOR_ACCESS_KEY_e,             /* Release MCU security after comparing a set of user supplied security keys to those stored in the
                                                   program flash. */
   FC_SWAP_CONTROL_e,                           /* Handles swap-related activities. */
   FC_PROGRAM_PARTITION_e = ((uint8_t)0x80),    /* Program the FlexNVM Partition Code and EEPROM Data Set Size into the data flash IFR. format all
                                                   EEPROM backup data sectors allocated for EEPROM, initialize the FlexRAM. */
   FC_SET_FLEXRAM_FUNCTION_e                    /* Switches FlexRAM function between RAM and EEPROM.  When switching to EEPROM, FlexNVM is not
                                                   available while valid data records are being copied from EEPROM backup to FlexRAM. */
} fCmd_e;                                       /* Flash Commands - See definition in K60 Sub-Family Ref Manual, Rev. 2, sect 30.4.10.2 */
/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION PROTOTYPES - Accessed via a table */

/* START - Functions accessed via a table entry (indirect call) */
static returnStatus_t   dvr_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   dvr_read( uint8_t *pDest, const dSize srcOffset, lCnt Cnt, PartitionData_t const *pParData,
                                 DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   init( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   close( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   setPowerMode( const ePowerMode ePwrMode, PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   dvr_write( dSize destOffset, uint8_t const *pSrc, lCnt Cnt, PartitionData_t const *pParData,
                                    DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   erase( dSize destOffset, lCnt Cnt, PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   flush( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   dvr_ioctl( const void *pCmd, void *pData, PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const *pNextDvr );
static returnStatus_t   restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver );
static bool             timeSlice(PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver);
/* END - Functions accessed via a table entry (indirect call) */

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

static returnStatus_t   flashErase( uint32_t dest, uint32_t numBytes );
static returnStatus_t   flashWrite( uint32_t address, uint32_t cnt, uint8_t const *pSrc );
static returnStatus_t   exeFlashCmdSeq( uint32_t dest );
static returnStatus_t   exeFlashCmdSeqSameBank( void );
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) && ( HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A ) ) )
static returnStatus_t   exeFlashCmdSeqDifBank( void );
#endif
static void             setFlashAddr( uint32_t addr );
static void             setFlashData( uint8_t const *pData );

/* Note - ISRs aren't static, but this function is not in the header & should only be executed via an interrupt. */
#if !defined( __BOOTLOADER )
void                    ISR_InternalFlash( void );
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A ) || ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A ) )
static returnStatus_t   exeSwapSeq( eFS_command_t cmd, flashSwapStatus_t *pStatus );
static returnStatus_t   flashSwap( eFS_command_t cmd, flashSwapStatus_t *pStatus );
#endif   /* Swap type device  */
#endif   /* BOOTLOADER  */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* The driver table must be defined after the function definitions. The list below are the supported commands for this
 * driver. */
const DeviceDriverMem_t IF_deviceDriver =
{
   init,          // Init function - Creates mutex & calls lower drivers
   dvr_open,      // Open Command - For this implementation it only calls the lower level drivers.
   close,         // Close Command - For this implementation it only calls the lower level drivers.
   setPowerMode,  // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost
   dvr_read,      // Read Command
   dvr_write,     // Write Command
   erase,         // Erases a portion (or all) of the banked memory.
   flush,         // Write the cache content to the lower layer driver
   dvr_ioctl,     // ioctl function - Does Nothing for this implementation
   restore,       // Not supported - API support only
   timeSlice      // Not supported - API support only
};

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#ifndef __BOOTLOADER
STATIC bool       bIntSemCreated_ = (bool)false;
STATIC OS_SEM_Obj intFlashSem_    = {0};                 /* Semaphore given when flash access completed */
#endif   /* BOOTLOADER  */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: init

   Purpose: Create and mutexes this module requires and initialize any interrupts that are required.

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                                  partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: returnStatus_t - As defined by error_codes.h

   Side Effects: Mutex is created

   Re-entrant Code: No (this should only be called at power up before the scheduler is running)

   Notes:

 **********************************************************************************************************************/
static returnStatus_t init( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )  /*lint !e715 !e818  Parameters passed in may not be used */
{
#ifndef __BOOTLOADER
   returnStatus_t   eRetVal = eFAILURE;      /* Return Value */

   /* Create share memory */
   (void)dvr_shm_init();

   if ( !bIntSemCreated_ ) /* If the semaphore has not been created, create it. */
   {  /* Install ISRs and create sem & mutex to protect the banked driver modules critical section */
      if (NULL != _int_install_isr((int)INT_FTFE, (INT_ISR_FPTR)ISR_InternalFlash, NULL))
      {
         (void)_bsp_int_init((int)INT_FTFE, 5, 1, (bool)true);
         if (OS_SEM_Create(&intFlashSem_))
         {
            eRetVal = eSUCCESS;
            bIntSemCreated_ = (bool)true;
         }
      }
   }
   else
#else
   returnStatus_t   eRetVal;  /* Return Value */
#endif   /* BOOTLOADER  */
   {
      eRetVal = eSUCCESS;
   }
   return (eRetVal);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: dvr_open

   Purpose: There is nothing to open for this driver and there is no lower driver.

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code:  No

   Note: This should only be called at power up before the scheduler is running

 **********************************************************************************************************************/
static returnStatus_t dvr_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver ) /*lint !e715 !e818  Parameters passed in may not be used */
{
   return (eSUCCESS);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: close

   Purpose: Only here for the API, does nothing.

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.


   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t close( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver ) /*lint !e715 !e818  Parameters passed in may not be used */
{
   return (eSUCCESS);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: dvr_read

   Purpose: Reads data from flash memory, done with a simple memcpy.

   Arguments:
      uint8_t *pDest:  Location to write data
      dSize srcOffset:  Location of the source data in NV memory
      phCnt Cnt:  Number of bytes to Read from the NV Memory.
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to read.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t dvr_read( uint8_t *pDest, const dSize srcOffset, const lCnt cnt, PartitionData_t const *pParData,
                                DeviceDriverMem_t const * const *pNextDriver ) /*lint !e715 !e818  Parameters passed in may not be used */
{
   ASSERT(Cnt <= INTERNAL_FLASH_SIZE); /* Count must be <= the sector size. */
   dvr_shm_lock();
   (void)memcpy(pDest, (uint8_t *)pParData->PhyStartingAddress + srcOffset, cnt);
   dvr_shm_unlock();

   return (eSUCCESS);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: dvr_write

   Purpose: Writes data to internal flash memory.

   Arguments:
      dSize destOffset:  Location of the data to write to the NV Memory
      uint8_t *pSrc:  Location of the source data to write to Flash
      lCnt Cnt:  Number of bytes to write to the NV Memory.
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t dvr_write( dSize destOffset, uint8_t const *pSrc, lCnt cnt, PartitionData_t const *pParData,
                                 DeviceDriverMem_t const * const *pNextDriver )   /*lint !e715 !e818  Parameters passed in may not be used */
{
   returnStatus_t retVal = eSUCCESS;
   dSize          destAddr = pParData->PhyStartingAddress + destOffset;

   ASSERT(Cnt <= INTERNAL_FLASH_SIZE);    /* Count must be <= the sector size. */
   dvr_shm_lock();

#if !defined( __BOOTLOADER ) && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   if ( _bsp_get_clock_configuration() != BSP_CLOCK_CONFIGURATION_3 )
   {
#if DEBUG_CLOCK_CHANGE != 0
      DBG_logPrintf( 'I', "%s: Changing clock to configuration %hhu", __func__, BSP_CLOCK_CONFIGURATION_3 );
#endif
      retVal = (returnStatus_t)_bsp_set_clock_configuration( BSP_CLOCK_CONFIGURATION_3 ); /* Set clock to 120MHz, normal run mode.  */
      if( CM_ERR_OK == retVal )
      {
#endif
         if (0 != memcmp((uint8_t *)destAddr, pSrc, cnt)) /* Does flash need to be updated? */
         {  /* Flash has to be updated */
            while(cnt)
            {
               dSize destSectOffset = destAddr % ERASE_SECTOR_SIZE_BYTES;  /* Offset into the sector to start writing */
               dSize destSectBase = destAddr - destSectOffset;             /* Calc start of sector address */
               lCnt  opCnt = cnt;                                          /* Current operation count */
               if (opCnt > (ERASE_SECTOR_SIZE_BYTES - destSectOffset))
               {
                  opCnt = ERASE_SECTOR_SIZE_BYTES - destSectOffset;
               }

               (void)memcpy(dvr_shm_t.pIntSectBuf_, (void *)destSectBase, ERASE_SECTOR_SIZE_BYTES); /* Read the Sector */
               uint32_t i;                                              /* Counter for the loop below */
               uint32_t *p32BitIntSectBuf_;                             /* Leverage the 32-bit processor, compare 32-bits */
               for ( i = 0, p32BitIntSectBuf_ = (uint32_t *)dvr_shm_t.pIntSectBuf_;
                     i < (ERASE_SECTOR_SIZE_BYTES/sizeof(uint32_t));     /* Checking 4-bytes at a time, divide by 4 */
                     i++, p32BitIntSectBuf_++)                           /* Increment counter and 32-bit pointer */
               {
                  if (0xFFFFFFFF != *p32BitIntSectBuf_)  /* Are all 4 bytes erased? */
                  {
                     retVal = flashErase( destSectBase, ERASE_SECTOR_SIZE_BYTES );    /* Erase the Sector */
                     break;
                  }
               }
               if (eSUCCESS == retVal) /* Is it okay to write?  (If the erase failed, nothing will be written.) */
               {
                  /* Copy over the portion of pSrc to the sector read from memory. */
                  (void)memcpy(dvr_shm_t.pIntSectBuf_ + destSectOffset, pSrc, opCnt);
                  if (eSUCCESS != flashWrite(destSectBase, ERASE_SECTOR_SIZE_BYTES, dvr_shm_t.pIntSectBuf_))
                  {
                     retVal = eFAILURE;
                     break;
                  }
               }
               else
               {
                  break;
               }
               cnt -= opCnt;
               destAddr += opCnt;
               pSrc += opCnt;
            }
         }
#if !defined( __BOOTLOADER ) && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
      }
   }
   if ( _bsp_get_clock_configuration() != BSP_CLOCK_CONFIGURATION_DEFAULT )
   {
#if DEBUG_CLOCK_CHANGE != 0
      DBG_logPrintf( 'I', "%s: Changing clock to configuration %hhu", __func__, BSP_CLOCK_CONFIGURATION_DEFAULT );
#endif
      retVal = (returnStatus_t)_bsp_set_clock_configuration( BSP_CLOCK_CONFIGURATION_DEFAULT ); /* Set clock to 180MHz, high speed run mode.  */
   }
#endif
   dvr_shm_unlock();

   return(retVal);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: flush

   Purpose: Does nothing, only here to support the API.

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t flush( PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver ) /*lint !e715 !e818  Parameters passed in may not be used */
{
   return (eSUCCESS);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: erase

   Purpose: Erases a portion of the flash memory.

   Arguments:
      dSize destOffset - Offset into the partition to erase
      lCnt cnt - number of bytes to erase
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: returnStatus_t as defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t erase( dSize destOffset, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   returnStatus_t eRetVal = eFAILURE;
   /*lint -efunc( 715, erase ) : pNextDvr is not used.  It is a part of the common API. */
   /*lint -efunc( 818, erase ) : pNextDvr is declared correctly.  It is a part of the common API. */

#if !defined( __BOOTLOADER ) && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
   if ( _bsp_get_clock_configuration() != BSP_CLOCK_CONFIGURATION_3 )
   {
#if DEBUG_CLOCK_CHANGE != 0
      DBG_logPrintf( 'I', "%s: Changing clock to configuration %hhu", __func__, BSP_CLOCK_CONFIGURATION_3 );
#endif
      eRetVal = (returnStatus_t)_bsp_set_clock_configuration( BSP_CLOCK_CONFIGURATION_3 ); /* Set clock to 120MHz, normal run mode.  */
      if( CM_ERR_OK == eRetVal )
      {
#endif
         lCnt phDestOffset = destOffset + pParData->PhyStartingAddress; /* Get the actual address in NV. */
         lCnt nBytesOperatedOn; /* Number of bytes to operate on, Sector or Block */

         if ( (phDestOffset + cnt) <= INTERNAL_FLASH_SIZE )
         {
            eRetVal = eSUCCESS; /* Assume Seccess - Also checked in while loop below. */

            dvr_shm_lock(); /* Take mutex, critical section */

            while ( cnt && (eSUCCESS == eRetVal) )
            {
               nBytesOperatedOn = ERASE_SECTOR_SIZE_BYTES;

               /* Is the address on a boundary and the cnt >= the sector size?  This will determine if an entire sector
                  can be erased or if a partial sector needs to be erased.  If a partial sector needs to be erased, the
                  data that will remain unchanged needs to be read and written back after the sector has been erased.  */
               if ( (!(phDestOffset % ERASE_SECTOR_SIZE_BYTES)) && (cnt >= ERASE_SECTOR_SIZE_BYTES) )
               {
                  (void)flashErase( phDestOffset, sizeof(dvr_shm_t.pIntSectBuf_) );
               }
               else /* Need to erase a partial sector. */
               {
                  phAddr nStartingAddr = phDestOffset - (phDestOffset % ERASE_SECTOR_SIZE_BYTES);

                  (void)memcpy(&dvr_shm_t.pIntSectBuf_[0], (uint8_t *)nStartingAddr, sizeof(dvr_shm_t.pIntSectBuf_));
                  (void)flashErase( phDestOffset, sizeof(dvr_shm_t.pIntSectBuf_) );
                  if ( (phDestOffset % sizeof(dvr_shm_t.pIntSectBuf_) + cnt) > sizeof(dvr_shm_t.pIntSectBuf_) )
                  {
                     nBytesOperatedOn = sizeof(dvr_shm_t.pIntSectBuf_) - (phDestOffset % sizeof(dvr_shm_t.pIntSectBuf_));
                  }
                  else
                  {
                     nBytesOperatedOn = cnt;
                  }
                  (void)memset(&dvr_shm_t.pIntSectBuf_[phDestOffset % sizeof (dvr_shm_t.pIntSectBuf_)], 0xFF, (uint16_t)nBytesOperatedOn);
                  eRetVal = flashWrite( nStartingAddr, sizeof(dvr_shm_t.pIntSectBuf_), &dvr_shm_t.pIntSectBuf_[0] );
               }
               phDestOffset += nBytesOperatedOn;
               cnt -= nBytesOperatedOn;
            }
            dvr_shm_unlock(); /* End critical section */
         }
#if !defined( __BOOTLOADER ) && ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
      }
   }
   if ( _bsp_get_clock_configuration() != BSP_CLOCK_CONFIGURATION_DEFAULT )
   {
#if DEBUG_CLOCK_CHANGE != 0
      DBG_logPrintf( 'I', "%s: Changing clock to configuration %hhu", __func__, BSP_CLOCK_CONFIGURATION_DEFAULT );
#endif
      eRetVal = (returnStatus_t)_bsp_set_clock_configuration( BSP_CLOCK_CONFIGURATION_DEFAULT ); /* Set clock to 180MHz, high speed run mode.  */
   }
#endif
   return (eRetVal);
}
/***********************************************************************************************************************

   Function Name: setPowerMode

   Purpose: Does nothing, only here for the API

   Arguments:
      enum ePwrMode 1 uses ePowerMode enumeration
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: returnStatus_t - eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t setPowerMode( const ePowerMode ePwrMode, PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )  /*lint !e715 !e818  Parameters
                                                                                                                                                  passed in may not be used */
{
   return(eSUCCESS);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: dvr_ioctl

   Purpose: Does nothing, only here for the API

   Arguments:
      void *pCmd - Command to execute:
      void *pData - Data to device.
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: returnStatus_t - eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t dvr_ioctl( const void *pCmd, void *pData, PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )   /*lint !e715 !e818  Parameters passed in
                                                                                                                                             may not be used */
{
   returnStatus_t    retVal = eFAILURE;
#if !defined ( __BOOTLOADER ) && ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) && ( HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A ) ) )
   IF_ioctlCmdFmt_t  *pCommand = (IF_ioctlCmdFmt_t *)pCmd;

   if (eIF_IOCTL_CMD_FLASH_SWAP == pCommand->ioctlCmd) /* Is this a flash swap command? */
   {
      flashSwapStatus_t swapStatus;  /* Contains the status from the flash swap function */

      (void)memset(&swapStatus, 0xFF, sizeof(swapStatus));  /* Sets the status to all invalid */
      /* If this is a single "manual" command, call the flashSwap function and return the data. */
      dvr_shm_lock();
      if (  (eFS_GET_STATUS == pCommand->fsCmd) || (eFS_INIT == pCommand->fsCmd) || (eFS_UPDATE == pCommand->fsCmd) ||
            (eFS_COMPLETE == pCommand->fsCmd))
      {
         retVal = flashSwap( pCommand->fsCmd, &swapStatus );
      }
      dvr_shm_unlock();
      (void)memcpy(pData, ( void * )&swapStatus, sizeof(swapStatus));
   }
#endif   // __BOOTLOADER
   return(retVal);
} /*lint !e715 !e818  Parameters passed in may not be used */
/***********************************************************************************************************************

   Function Name: restore

   Purpose: The banked driver doesn't support the restore command

   Arguments:
      PartitionData_t const *pParData - Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const *pNextDriver - Points to the next driver's table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver )
{
   ASSERT(NULL != pNextDriver); /* The next driver must be defined, this can be caught in testing. */
   /*lint -efunc( 613, restore ) : pNxtDvr can never be NULL in production code.  The ASSERT catches this. */

   return ((*pNextDriver)->devRestore(lDest, cnt, pParData, pNextDriver + 1));
}
/***********************************************************************************************************************

   Function Name: timeSlice

   Purpose: Common API function, Does nothing.

   Arguments: PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver

   Returns: bool - 0 (false)

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static bool timeSlice(PartitionData_t const *pParData, DeviceDriverMem_t const * const *pNextDriver)   /*lint !e715 !e818  Parameters passed in may not be used */
{
   return((bool)false);
} /*lint !e715 !e818  Parameters passed in may not be used */

/* ****************************************************************************************************************** */
/* Local Function Definitions */

/***********************************************************************************************************************

   Function Name: flashErase

   Purpose: Erase n number of sectors.

   Arguments: uint32_t dest, uint32_t numBytes

   Returns: returnStatus_t

   Side Effects: TODO

   Re-entrant Code: TODO

   Notes: TODO

 **********************************************************************************************************************/
static returnStatus_t flashErase( uint32_t dest, uint32_t numBytes )
{
   returnStatus_t eRetVal = eFAILURE;

   /* Make sure the dest is on boundary & cnt is a x sector */
   if ( (0 == (numBytes % ERASE_SECTOR_SIZE_BYTES)) && (0 == (dest % ERASE_SECTOR_SIZE_BYTES)) )
   {
      FS_FLASH_CMD_REG = (uint8_t)FC_ERASE_FLASH_SECTOR_e;  /* Set Erase command */
      setFlashAddr(dest);                                   /* Set destination address */
      CLRWDT();                                             /* Should this be disabled? */
      eRetVal= exeFlashCmdSeq(dest);                        /* Execute Command sequence - ISRs enabled */

#if 0
      /* Unlock device when erease sector 0 */
#if BLOCK_1K
      if ( (dest >= 0x400) && (dest < 0x800) )
#else
      if ( dest < 0x800 )
#endif
      {
         FS_FLASH_CMD_REG = FC_PROGRAM_PHASE_e;     /* Write 8 bytes*/

         /*Set destination address*/
         setFlashAddr(dest);

         /*data*/
         FTFE_BASE_PTR->FCCOB4 = 0xFF;
         FTFE_BASE_PTR->FCCOB5 = 0xFF;
         FTFE_BASE_PTR->FCCOB6 = 0xFF;
         FTFE_BASE_PTR->FCCOB7 = 0xFF;
         FTFE_BASE_PTR->FCCOB8 = 0xFF;
         FTFE_BASE_PTR->FCCOB9 = 0xFF;
         FTFE_BASE_PTR->FCCOBA = 0xFF;
         FTFE_BASE_PTR->FCCOBB = 0xFE;
      }
#endif

#ifndef __BOOTLOADER
      _ICACHE_ENABLE();
#endif   /* BOOTLOADER  */

   }
   return(eRetVal);
}
/***********************************************************************************************************************

   Function Name: flashWrite

   Purpose: Writes blocks of 8 bytes to the Flash Memory.

   Arguments: uint32_t address, uint32_t cnt, char const *pSrc

   Returns: returnStatus_t

   Side Effects: Flash memory will be written, the flash cache will be cleared.

   Re-entrant Code: No

   Notes: N/A

 **********************************************************************************************************************/
static returnStatus_t flashWrite( uint32_t address, uint32_t cnt, uint8_t const *pSrc )
{
   returnStatus_t retVal = eFAILURE;
   /* Erased state are all 8 bytes of 0xFF */
   static const uint8_t erasedState_[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

   if ( (0 == (cnt % WRITE_BLOCK_SIZE_IN_BYTES)) && (0 == (address % WRITE_BLOCK_SIZE_IN_BYTES)) )
   {
      retVal = eSUCCESS;

#ifndef __BOOTLOADER
      _ICACHE_DISABLE();                              /* Disable the cache memory before running external RAM tests. */
#endif   /* BOOTLOADER  */
      FS_FLASH_CMD_REG = (int)FC_PROGRAM_PHASE_e;     /* Set Write command */
      while (cnt)
      {
         if (0 != memcmp(&erasedState_[0], pSrc, sizeof(erasedState_))) /* Only write if the data written is not 0xFF */
         {
            setFlashAddr(address);                    /* Set destination address register */
            setFlashData(pSrc);                       /* Set the destination data registers */
            if (eSUCCESS != exeFlashCmdSeq(address))  /* Execute the write command */
            {  /* Write failed!  */
               retVal = eFAILURE;
               break;
            }
         }
         cnt -= WRITE_BLOCK_SIZE_IN_BYTES;         /* Adjust the count */
         pSrc += WRITE_BLOCK_SIZE_IN_BYTES;        /* Adjust the source data */
         address += WRITE_BLOCK_SIZE_IN_BYTES;     /* Adjust the destination address */
      }
#ifndef __BOOTLOADER
      _ICACHE_ENABLE();
#endif   /* BOOTLOADER  */
   }

   return(retVal);
}
#ifndef __BOOTLOADER
/***********************************************************************************************************************

   Function Name: flashSwap

   Purpose: Executes a step (provided by cmd which is passed in) in the process of swapping execution program memory.

   Arguments:  eFS_command_t     cmd - Command to perform (init, update, complete or get status)
               flashSwapStatus_t *pStatus - Pointer to location to store the swap status

   Returns: returnStatus_t - eSUCCESS or eFAILURE (if failure, the pStatus will not be populated)

   Side Effects: Flash memory may be swapped after the update command is processed.

   Re-entrant Code: No

   Notes: N/A

 **********************************************************************************************************************/
#if !defined( __BOOTLOADER ) && ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) && ( HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A ) ) )
static returnStatus_t flashSwap( eFS_command_t cmd, flashSwapStatus_t *pStatus )
{
   returnStatus_t retVal;  /* Return value = Success or Failure. */

   /* Step 1:  Read the status of the flash swap module. */
   retVal = exeSwapSeq( eFS_GET_STATUS, pStatus );
   if (eSUCCESS == retVal)   /* Set up for 1st command.  Read the status. */
   {  /* Step 2:  Execute the command. */
      switch(cmd)
      {
         case eFS_INIT: /* Handle swap control code "Initialize swap system" */
         {
            if ( eFS_MODE_UNINITIALIZED == pStatus->currentMode )
            {
               retVal = exeSwapSeq( cmd, pStatus ); /* Execute the command to initialize */
            }
            break;
         }
         case eFS_UPDATE:  /* Handle swap control code "Set Swap in Update State" */
         {
            if ( eFS_MODE_READY == pStatus->currentMode )
            {
               retVal = exeSwapSeq( cmd, pStatus ); /* Execute the command to update */
            }
            break;
         }
         case eFS_COMPLETE: /* Handle swap control code "Set Swap in Complete State" */
         {
            if ( eFS_MODE_UPDATE_ERASED == pStatus->currentMode ) /* Already in the proper mode? */
            { /* The update was successful, then change the state to complete */
               /* call flash command sequence function to execute the command */
               retVal = exeSwapSeq( cmd, pStatus ); /* Execute the command to update */
            }
            break;
         }
         case eFS_GET_STATUS:
         {
            break;  /* retVal has already been set to success */
         }
         default:
         {
            retVal = eFAILURE; /* Not a valid command! */
            break;
         }
      }  /* switch(cmd) */
   } /* if (eSUCCESS == retVal) */
   return(retVal);
}
/***********************************************************************************************************************

   Function Name: exeSwapSeq

   Purpose: Executes the flash command sequence and populates the status if execution was successful.

   Arguments:  eFS_command_t cmd - command to execute
               flashSwapStatus_t *pStatus - Pointer to status

   Returns: returnStatus_t - Result of sequence execution

   Side Effects: Command code could be executed to perform a flash swap

   Re-entrant Code: NO

   Notes:

 **********************************************************************************************************************/
static returnStatus_t exeSwapSeq( eFS_command_t cmd, flashSwapStatus_t *pStatus )
{
   FS_FLASH_CMD_REG = (uint8_t)FC_SWAP_CONTROL_e;      /* preparing passing parameter to GET STATUS */
   setFlashAddr(PART_SWAP_STATE_OFFSET);              /* Set flash swap address */
   FS_SWAP_CONTROL_CODE_REG = (uint8_t)cmd;            /* Set up the command */
   FTFE_BASE_PTR->FCCOB5 = 0xFF;                      /* Set per a note in the "K60 Sub-Family Reference Manual" */
   FTFE_BASE_PTR->FCCOB6 = 0xFF;                      /* Set per a note in the "K60 Sub-Family Reference Manual" */
   FTFE_BASE_PTR->FCCOB7 = 0xFF;                      /* Set per a note in the "K60 Sub-Family Reference Manual" */
   returnStatus_t retVal = exeFlashCmdSeqSameBank();  /* Execute the command sequence */
   if (eSUCCESS == retVal)                            /* If sequence was executed properly, then populate the status */
   {  /* Sequence was executed successfully, populate the status */
      pStatus->currentMode    = FS_CURRENT_MODE_REG;  /* Save the mode */
      pStatus->currentBlock   = FS_CURRENT_BLOCK_REG; /* Current block */
      pStatus->nextBlock      = FS_NEXT_BLOCK_REG;    /* Next block */
   }
   else
   {
      NOP();
   }
   return(retVal);
}
#endif
#endif   /* BOOTLOADER  */
/***********************************************************************************************************************

   Function Name: exeFlashCmdSeq

   Purpose: Performs a flash command (the command must be setup before this function is called).

   Arguments: uint32_t dest

   Returns: returnStatus_t result

   Side Effects: Flash could be written, erased or lock state changed.

   Re-entrant Code: NO

   Notes:

 **********************************************************************************************************************/
static returnStatus_t exeFlashCmdSeq( uint32_t dest )
{
   returnStatus_t retVal;

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) && ( HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A ) ) )
   retVal = eFAILURE;
   if (ERASE_BLOCK_SIZE_BYTES <= dest)                /* Upper bank or same bank? */
   {  /* Upper bank */
      retVal= exeFlashCmdSeqDifBank();                /* Execute Command sequence - ISRs enabled */
   }
   else
#else
      (void)dest; //Keeps lint happy
#endif
   {  /* Same bank */
      retVal= exeFlashCmdSeqSameBank();               /* Execute command sequence - ISRs disabled */
   }
   return(retVal);
}
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) && ( HAL_TARGET_HARDWARE != HAL_TARGET_XCVR_9985_REV_A ) ) )
/***********************************************************************************************************************

   Function Name: exeFlashCmdSeqDifBank

   Purpose: Performs a flash command (the command must be setup before this function is called)

   Arguments: None

   Returns: returnStatus_t result

   Side Effects: Flash could be written, erased or lock state changed.

   Re-entrant Code: NO

   Notes:

 **********************************************************************************************************************/
static returnStatus_t exeFlashCmdSeqDifBank( void )
{
   returnStatus_t retVal = eSUCCESS;

   /* The following while removed since this function will not exit without completing and this is the only location in
      code where flash commands are executed! */
   while ( !(FTFE_BASE_PTR->FSTAT & FTFE_FSTAT_CCIF_MASK) )  /* If a command was executed, wait for it to end. */
   {}

   FTFE_BASE_PTR->FSTAT = FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK | FTFE_FSTAT_RDCOLERR_MASK;  /* Clear Flags */
   FTFE_BASE_PTR->FSTAT = FTFE_FSTAT_CCIF_MASK;   /* Launch Command */
   FTFE_BASE_PTR->FCNFG |= FTFE_FCNFG_CCIE_MASK;  /* Enable the interrupts */

#ifndef __BOOTLOADER
   if (!OS_SEM_Pend(&intFlashSem_, MAX_SEM_WAIT_mS))  /* Pend on flash execution completion */
   {
      retVal = eFAILURE;   /* Sem time-out, return w/error */
   }
#endif   /* BOOTLOADER  */

   if ( FTFE_BASE_PTR->FSTAT & (FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK | FTFE_FSTAT_MGSTAT0_MASK) )/* Error? */
   {
      retVal = eFAILURE;
   }
   return (retVal);
}
#endif
/***********************************************************************************************************************

   Function Name: exeFlashCmdSeqSameBank

   Purpose: Performs a flash command (the command must be setup before this function is called).  This function disables
            interrupts because we're accessing the same bank as we're executing out of.

   Arguments: None

   Returns: returnStatus_t result

   Side Effects: Flash could be written, erased or lock state changed.

   Re-entrant Code: NO

   Notes:

 **********************************************************************************************************************/
__ramfunc static returnStatus_t exeFlashCmdSeqSameBank( void ) /*lint !e129   __ramfunc is an IAR command */
{
   returnStatus_t retVal = eSUCCESS;

   /* The following while removed since this function will not exit without completing and this is the only location in
      code where flash commands are executed! */
   while ( !(FTFE_BASE_PTR->FSTAT & FTFE_FSTAT_CCIF_MASK) )  /* If a command was executed, wait for it to end. */
   {}

   FTFE_BASE_PTR->FSTAT = FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK | FTFE_FSTAT_RDCOLERR_MASK;  /* Clear Flags */

   /* NOTE:  Disabling interrupts goes against our coding standards.  When writing to the same bank in the K60 as we're
      executing out of, this is necessary. */
#ifndef __BOOTLOADER
   bool           intsEnabled;     /* True when interrupts are enabled, otherwise false */
   uint32_t       primask;
   intsEnabled = BSP_IS_GLOBAL_INT_ENABLED();
   if ( intsEnabled )
   {
      primask = __get_PRIMASK();
      __disable_interrupt( ); /* Disable interrupts! */
   }
#endif   /* BOOTLOADER  */

   FTFE_BASE_PTR->FSTAT = FTFE_FSTAT_CCIF_MASK;/*lint !e456 */   /* Launch Command */

   while ( !(FTFE_BASE_PTR->FSTAT & FTFE_FSTAT_CCIF_MASK) )  /* Command was executed, wait for it to end. */
   {}

#ifndef __BOOTLOADER
   if ( intsEnabled )
   {
      __set_PRIMASK(primask); // Restore interrupts
   }
#endif   /* BOOTLOADER  */

   if ( FTFE_BASE_PTR->FSTAT & (FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK | FTFE_FSTAT_MGSTAT0_MASK) )/*lint !e456 */ /* Error? */
   {
      retVal = eFAILURE;
   }
   return (retVal);/*lint !e454 */
}/*lint !e454 */
/***********************************************************************************************************************

   Function Name: setFlashAddr

   Purpose: Sets the flash address

   Arguments: uint32_t addr - Address to set

   Returns: None

   Side Effects: None

   Re-entrant Code: NO

   Notes:

 **********************************************************************************************************************/
static void setFlashAddr( uint32_t addr )
{

   FTFE_BASE_PTR->FCCOB1 = (uint8_t)((addr >> 16) & 0xFF);    /* Set destination address - MSB */
   FTFE_BASE_PTR->FCCOB2 = (uint8_t)((addr >> 8) & 0xFF);     /* Set destination address */
   FTFE_BASE_PTR->FCCOB3 = (uint8_t)(addr & 0xFF);            /* Set destination address - LSB */
}
/***********************************************************************************************************************

   Function Name: setFlashData

   Purpose: Sets the flash address

   Arguments: uint8_t *pData - Data

   Returns: None

   Side Effects: None

   Re-entrant Code: NO

   Notes:

 **********************************************************************************************************************/
static void setFlashData( uint8_t const *pData )
{
   FTFE_BASE_PTR->FCCOB8 = pData[7];       /* copy 8 bytes of data */
   FTFE_BASE_PTR->FCCOB9 = pData[6];
   FTFE_BASE_PTR->FCCOBA = pData[5];
   FTFE_BASE_PTR->FCCOBB = pData[4];
   FTFE_BASE_PTR->FCCOB4 = pData[3];
   FTFE_BASE_PTR->FCCOB5 = pData[2];
   FTFE_BASE_PTR->FCCOB6 = pData[1];
   FTFE_BASE_PTR->FCCOB7 = pData[0];
}
/***********************************************************************************************************************

   Function Name: ISR_InternalFlash

   Purpose: Internal flash ISR

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
void ISR_InternalFlash( void )
{
   FTFE_BASE_PTR->FCNFG &= ~FTFE_FCNFG_CCIE_MASK;      /* Enable the interrupts */
   OS_SEM_Post( &intFlashSem_ );
}
#endif   /* BOOTLOADER  */
