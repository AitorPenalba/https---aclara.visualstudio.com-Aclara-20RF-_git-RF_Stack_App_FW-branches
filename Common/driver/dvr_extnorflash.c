// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************

   Filename:   dvr_extnorflash.c

   Global Designator: DRV_ExtNorFlash

   Contents: Driver for the External NOR Flash Memory.

             The flash driver is a 16 bit device.   All accesses must start on a 16 bit boundary and all operations are in words


             The layers above this code work in bytes and byte addresses.

             for example if a region is supposed to start at Flashbase + 0x1000, this is really at address 0x500 in the flash.

             We do all the conversions in this file.  the low level driver (lld) flash code operates exlusively in words

 ***********************************************************************************************************************
   Copyright (c) 2011-2015 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in
   whole or in part, in any form or by any means whatsoever without the written permission of:
                  ACLARA POWER-LINE SYSTEMS INC.
                  ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************

   $Log$


 **********************************************************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/*lint -esym(818, argc, argv) argc, argv could be const */

/* INCLUDE FILES */

#include "project.h"
#include <stdbool.h>
#include <stdlib.h>

#include "dvr_extNorFlash.h"

/* Should include ext_ram.h or remove from this debug routine */
#include "ext_ram.h"
//extern returnStatus_t EXTRAM_test(bool verbose);

#include <mqx.h>
#include <bsp.h>
#include <dcu_k64f120m.h>
#include "partitions.h"
#include "invert_bits.h"
#include "DBG_SerialDebug.h"
#include "dvr_sharedMem.h"
#include "lld_IS29LV032T.h"

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define ERASE_BEFORE_WRITE             ((bool)true)
#define WRITE_WITHOUT_ERASING          ((bool)false)

//#define ERASED_FLASH_BYTE           ((uint8_t)0xFF)  /* Data value of an erased byte */

/* For Unit Testing, we need to return a FAILURE in place of the assert.  */
#if defined TM_EFLASH_UNIT_TEST || defined TM_DVR_PARTITION_UNIT_TEST
#undef ASSERT
#define ASSERT(__e) if (!(__e)) return(eFAILURE)
#endif


// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */


// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* Functions accessed via a table entry (indirect call) */
static returnStatus_t dvr_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_read( uint8_t *pDest, const dSize srcOffset, lCnt Cnt, PartitionData_t const *pParData,
                                DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_init( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_close( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_pwrMode( const ePowerMode ePwrMode, PartitionData_t const *pPartitionData,
                               DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_write( dSize destOffset, uint8_t const *pSrc, lCnt Cnt, PartitionData_t const *pParData,
                                 DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_erase( dSize destOffset, lCnt Cnt, PartitionData_t const *pPartitionData,
                             DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_flush( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_ioctl( const void *pCmd, void *pData, PartitionData_t const *pPartitionData,
                                 DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData,
                               DeviceDriverMem_t const * const * pNextDriver );
static bool        timeSlice( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );

static returnStatus_t flashRead ( uint16_t *pDest, uint16_t * pFlashAddr, lCnt byteCount );
static returnStatus_t flashWrite( uint16_t* pFlashAddr, uint16_t const *pSrc, lCnt byteCount, bool const bEraseBeforeWrite );
static returnStatus_t flashEraseSector( uint16_t * pFlash) ; /* erase the flash sector with this address */

static void hexDump (void *addr, int len) ;

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constatnt Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */


/* The driver table must be defined after the function definitions. The list below are the supported commands for this
   driver. */
/* For the bootloader code we use open and read only */
DeviceDriverMem_t sDeviceDriver_eNorFlash =
{
   dvr_init,       // Init function - Creates mutex & calls lower drivers
   dvr_open,   // Open Command - For this implementation it only calls the lower level drivers.
   dvr_close,      // Close Command - For this implementation it only calls the lower level drivers.
   dvr_pwrMode,    // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost
   dvr_read,   // Read Command
   dvr_write,  // Write Command
   dvr_erase,      // Erases a portion (or all) of the banked memory.
   dvr_flush,      // Write the cache content to the lower layer driver
   dvr_ioctl,  // ioctl function - Does Nothing for this implementation
   restore,    // Not supported - API support only
   timeSlice   // Not supported - API support only
};

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Variable Definitions">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static OS_SEM_Obj        extFlashSem_;           /* Semaphore used when waiting for busy signal for external flash ISR */
static bool              flashSemCreated_ = false;  /* Flag to determine if the low level flash driver is initialized */
static unsigned int flashID = 0;
STATIC uint8_t       dvrOpenCnt_ = 0;        /* # of times driver has been opened, can close driver when value is 0. */

static int traceFlashAccess = 0;

/* Sector Definitions

   Sector (A20–A12)     Sector_Size    Addr_Range(x8)    Addr_Range(x16)
                        (KB/KW)
   SA0   000000xxx      64/32          000000–00FFFF     000000–007FFF
   SA1   000001xxx      64/32          010000–01FFFF     008000–00FFFF
   SA2   000010xxx      64/32          020000–02FFFF     010000–017FFF
   SA3   000011xxx      64/32          030000–03FFFF     018000–01FFFF
   SA4   000100xxx      64/32          040000–04FFFF     020000–027FFF
   SA5   000101xxx      64/32          050000–05FFFF     028000–02FFFF
   SA6   000110xxx      64/32          060000–06FFFF     030000–037FFF
   SA7   000111xxx      64/32          070000–07FFFF     038000–03FFFF
   SA8   001000xxx      64/32          080000–08FFFF     040000–047FFF
   SA9   001001xxx      64/32          090000–09FFFF     048000–04FFFF
   SA10  001010xxx      64/32          0A0000–0AFFFF     050000–057FFF
   SA11  001011xxx      64/32          0B0000–0BFFFF     058000–05FFFF
   SA12  001100xxx      64/32          0C0000–0CFFFF     060000–067FFF
   SA13  001101xxx      64/32          0D0000–0DFFFF     068000–06FFFF
   SA14  001110xxx      64/32          0E0000–0EFFFF     070000–077FFF
   SA15  001111xxx      64/32          0F0000–0FFFFF     078000–07FFFF
   SA16  010000xxx      64/32          100000–10FFFF     080000–087FFF
   SA17  010001xxx      64/32          110000–11FFFF     088000–08FFFF
   SA18  010010xxx      64/32          120000–12FFFF     090000–097FFF
   SA19  010011xxx      64/32          130000–13FFFF     098000–09FFFF
   SA20  010100xxx      64/32          140000–14FFFF     0A0000–0A7FFF
   SA21  010101xxx      64/32          150000–15FFFF     0A8000–0AFFFF
   SA22  010110xxx      64/32          160000–16FFFF     0B0000–0B7FFF
   SA23  010111xxx      64/32          170000–17FFFF     0B8000–0BFFFF
   SA24  011000xxx      64/32          180000–18FFFF     0C0000–0C7FFF
   SA25  011001xxx      64/32          190000–19FFFF     0C8000–0CFFFF
   SA26  011010xxx      64/32          1A0000–1AFFFF     0D0000–0D7FFF
   SA27  011011xxx      64/32          1B0000–1BFFFF     0D8000–0DFFFF
   SA28  011100xxx      64/32          1C0000–1CFFFF     0E0000–0E7FFF
   SA29  011101xxx      64/32          1D0000–1DFFFF     0E8000–0EFFFF
   SA30  011110xxx      64/32          1E0000–1EFFFF     0F0000–0F7FFF
   SA31  011111xxx      64/32          1F0000–1FFFFF     0F8000–0FFFFF
   SA32  100000xxx      64/32          200000–20FFFF     100000–107FFF
   SA33  100001xxx      64/32          210000–21FFFF     108000–10FFFF
   SA34  100010xxx      64/32          220000–22FFFF     110000–117FFF
   SA35  100011xxx      64/32          230000–23FFFF     118000–11FFFF
   SA36  100100xxx      64/32          240000–24FFFF     120000–127FFF
   SA37  100101xxx      64/32          250000–25FFFF     128000–12FFFF
   SA38  100110xxx      64/32          260000–26FFFF     130000–137FFF
   SA39  100111xxx      64/32          270000–27FFFF     138000–13FFFF
   SA40  101000xxx      64/32          280000–28FFFF     140000–147FFF
   SA41  101001xxx      64/32          290000–29FFFF     148000–14FFFF
   SA42  101010xxx      64/32          2A0000–2AFFFF     150000–157FFF
   SA43  101011xxx      64/32          2B0000–2BFFFF     158000–15FFFF
   SA44  101100xxx      64/32          2C0000–2CFFFF     160000–167FFF
   SA45  101101xxx      64/32          2D0000–2DFFFF     168000–16FFFF
   SA46  101110xxx      64/32          2E0000–2EFFFF     170000–177FFF
   SA47  101111xxx      64/32          2F0000–2FFFFF     178000–17FFFF
   SA48  110000xxx      64/32          300000–30FFFF     180000–187FFF
   SA49  110001xxx      64/32          310000–31FFFF     188000–18FFFF
   SA50  110010xxx      64/32          320000–32FFFF     190000–197FFF
   SA51  110011xxx      64/32          330000–33FFFF     198000–19FFFF
   SA52  110100xxx      64/32          340000–34FFFF     1A0000–1A7FFF
   SA53  110101xxx      64/32          350000–35FFFF     1A8000–1AFFFF
   SA54  110110xxx      64/32          360000–36FFFF     1B0000–1B7FFF
   SA55  110111xxx      64/32          370000–37FFFF     1B8000–1BFFFF
   SA56  111000xxx      64/32          380000–38FFFF     1C0000–1C7FFF
   SA57  111001xxx      64/32          390000–39FFFF     1C8000–1CFFFF
   SA58  111010xxx      64/32          3A0000–3AFFFF     1D0000–1D7FFF
   SA59  111011xxx      64/32          3B0000–3BFFFF     1D8000–1DFFFF
   SA60  111100xxx      64/32          3C0000–3CFFFF     1E0000–1E7FFF
   SA61  111101xxx      64/32          3D0000–3DFFFF     1E8000–1EFFFF
   SA62  111110xxx      64/32          3E0000–3EFFFF     1F0000–1F7FFF
   SA63  111111000      8/4            3F0000–3F1FFF     1F8000–1F8FFF
   SA64  111111001      8/4            3F2000–3F3FFF     1F9000–1F9FFF
   SA65  111111010      8/4            3F4000–3F5FFF     1FA000–1FAFFF
   SA66  111111011      8/4            3F6000–3F7FFF     1FB000–1FBFFF
   SA67  111111100      8/4            3F8000–3F9FFF     1FC000–1FCFFF
   SA68  111111101      8/4            3FA000–3FBFFF     1FD000–1FDFFF
   SA69  111111110      8/4            3FC000–3FDFFF     1FE000–1FEFFF
   SA70  111111111      8/4            3FE000–3FFFFF     1FF000–1FFFFF

   To Determine the sector, take the offset, mask with 0x1F8000 and shift right by 11

   During a sector erase operation, a valid address is any sector address within the sector being erased.
   During chip erase, a valid address is any non-protected sector address
   64kb sector # is offset / 0x10000 if less than 0x1f8000.
   8KB sectors # is (offset - 0x1f8000)/0x2000
End Sector Definitions */



/*******************************************************************************

   Function name: DBG_CommandLine_Flash

   Purpose: This function will exercise flash debug

   Arguments:  argc - Number of Arguments passed to this function
               argv - pointer to the list of arguments passed to this function

   Returns: FuncStatus - Successful status of this function - currently always 0 (success)

   Notes:

*******************************************************************************/
uint32_t DRV_ExtNorFlash_Debug ( uint32_t argc, char *argv[] )
{
   uint32_t offset = 0;
   DEVSTATUS stat;

   if ( argc == 1 )
   {
      DBG_printf( "External Memory Info:" );

      DBG_printf( "ExtFlash Base:Size %p  %lu", BSP_EXTERNAL_NORFLASH_ROM_BASE,
                                                BSP_EXTERNAL_NORFLASH_ROM_SIZE);

      DBG_printf( "ExtRam   Base:Size %p  %lu", BSP_EXTERNAL_MRAM_RAM_BASE,
                                                BSP_EXTERNAL_MRAM_RAM_SIZE);

      DBG_printf( "flashID = %04X", flashID);
   }
   else if ( argc > 1)
   {
      if (0 == strcmp (argv[1], "?"))
      {
         DBG_printf ("Valid Options are:");
         DBG_printf (" <EMPTY>                     Print out status of Flash & Ram");
         DBG_printf ("  ?                          This menu");
         DBG_printf ("  readflash <offset> <cnt>   Dump flash memory at the offset (in hex) specified");
         DBG_printf ("  erase chip/sector <offset> Erase the entire Flash or Erase sector where offset (in hex) resides");
         DBG_printf ("\nSome routines to test out the larger ram:");
         DBG_printf ("  testram                    Execute Destructive RAM memory test.  (Reboot afer as you will crash RTOS)");
         DBG_printf ("  readram <offset> <cnt>     Dump external Ram memory at the offset (in hex) specified");

         DBG_printf ("test");
      }
      else if (0 == strcasecmp(argv[1], "test"))
      {
         uint32_t rval;
         DBG_printf ("Testing Bank File system....");
         rval = DVR_ENFL_UnitTest(1);
         DBG_printf ("FLASH TEST rval %d", rval );
      }
      else if (0 == strcasecmp(argv[1], "testram"))
      {
         returnStatus_t rval;
         DBG_printf ("Testing External RAM....");
         rval = EXTRAM_test ((bool) 1);
         DBG_printf ("RAM TEST rval %d", rval );
      }
      else if (0 == strcasecmp(argv[1], "erase"))
      {
         if (argc > 2 )
         {
            if (0==strcasecmp(argv[2], "chip"))
            {
               DBG_printf ("Erasing entire Flash");
               stat = lld_ChipEraseOp ( );
               DBG_printf ("lld_ChipEraseOp returned status of %d", stat);
            }
            else
            {
               if (argc > 3)
               {
                  uint32_t sector = 0;
                  offset =  (uint32_t) strtol(argv[3], NULL, 16);
                  if (offset < 0x1f8000)
                     sector = offset / 0x10000;                  // large sector
                  else
                     sector = (offset - 0x1f8000)/ 0x2000 + 62 ;   // 8k sector
                  DBG_printf("erase sector offset=%08X    Sector=%d", offset, sector);
                  offset = offset >> 1;                                   //  divde by 2 to get into words
                  stat = lld_SectorEraseOp ( offset);
                  DBG_printf ("lld_SectorEraseOp returned status of %d", stat);
               }
               else
                  DBG_printf("invalid offset");
            }
         }
      }
      else if (0 == strcasecmp(argv[1], "readflash"))
      {
         char buf [260];
         uint32_t pAddr =  (uint32_t) BSP_EXTERNAL_NORFLASH_ROM_BASE ;
         uint16_t cnt = 16;
         uint16_t wordCount;

         DBG_printf ("Read Flash");
         if (argc > 2)
         {
             offset =  (uint32_t) strtol(argv[2], NULL, 16);
             if (argc > 3)
                cnt =  (uint16_t) strtol(argv[3], NULL, 0);
         }
         if (cnt > 255)
            cnt = 256;

         if (offset %2 )
            offset++;

         DBG_printf("Addr= %08X     offset= %08X    Cnt= %d", pAddr, offset, cnt);
         wordCount = cnt>>1;
         offset = offset >> 1;                                   //  divde by 2 to get into words

         lld_ReadBufferOp ( offset, (uint16_t*) buf, wordCount );

         hexDump (buf, cnt );
      }
      else if (0 == strcasecmp(argv[1], "readram"))
      {
         uint32_t pAddr =  (unsigned long) BSP_EXTERNAL_MRAM_RAM_BASE ;
         uint16_t cnt = 16;

         DBG_printf ("Read Memory");
         if (argc > 2)
         {
             offset =  (unsigned long) strtol(argv[2], NULL, 16);
             if (argc > 3)
                cnt =  (uint16_t) strtol(argv[3], NULL, 0);
         }
         DBG_printf ( "Addr= %08X     offset= %08X    Cnt= %d", pAddr, offset, cnt);
         // insert memory dump routine here.
         hexDump ((void*) (pAddr+offset), cnt);
      }
   }
   return (0);
}





// </editor-fold>
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t init()">
/***********************************************************************************************************************

   Function Name: dvr_init

   Purpose: Init Function

   Arguments: PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr

   Returns: returnStatus_t

   Side Effects:

   Reentrant Code: no

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_init ) : Parameters are not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_init ) : Parameters are not used.  It is a part of the common API. */
static returnStatus_t dvr_init( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr )
{
   traceFlashAccess = 0;
   /* Create share memory */
   ( void )dvr_shm_init();
   if ( flashSemCreated_ == false )
   {
      lld_InitCmd ( (FLASHDATA *) pPartitionData->PhyStartingAddress );

      if ( true == OS_SEM_Create( &extFlashSem_ ) )
      {
         flashSemCreated_ = true;
      } /* end if() */
   }
   return ( eSUCCESS );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t dvr_open()">


/***********************************************************************************************************************

   Function Name: dvr_open

   Purpose: Initializes the External Flash Driver.  Sets the write protect and hold I/O pins, creates the Mutex for the
            write and read functions and creates the Semaphore for the busy function.

   Arguments: PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr

   Returns: returnStatus_t

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_open ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_open ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr )
{
   returnStatus_t eRetVal = eSUCCESS;

   if ( dvrOpenCnt_ == 0 )       /* Only need to open the device once   */
   {
      ( void ) dvr_shm_lock();   /* Take mutex, critical section */

      flashID = lld_GetDeviceId();

      /* Flash ID should be 0x9DF6
         Manufacturer= 0x9D
         Device = 0xF6
       */
      ( void ) dvr_shm_unlock(); /* End critical section */
   }
   dvrOpenCnt_++;             /* Update the open count.  */
   return ( eRetVal ); /*lint -esym(715,pNextDvr) */
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t close()">
/***********************************************************************************************************************

   Function Name: dvr_close

   Purpose: Closes the external flash

   Arguments:
      PartitionData_t const *pParData  Points to a partition table entry.  This contains all information to access the
                                    partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_close ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_close ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_close( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr )
{
   if ( dvrOpenCnt_ )
   {
      dvrOpenCnt_--;
   }
   if ( 0 == dvrOpenCnt_ ) /* If driver has been closed the same number of times it has been opened, close it. */
   {

   }
   return ( eSUCCESS );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t pwrMode()">
/***********************************************************************************************************************

   Function Name: dvr_pwrMode

   Purpose: Sets the power mode for the busy wait condition.  The power mode defaults to Normal at power up.

   Arguments:  ePowerMode ePwrMode
               PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                            the partition to initialize.
               DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: None

   Side Effects: Sets the global variable ePwrMode_.  After a write/erase the micro will go to sleep in LOW power mode.

   Reentrant Code: Yes - because it is an atomic operation.

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_pwrMode ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_pwrMode ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_pwrMode( const ePowerMode ePwrMode, PartitionData_t const *pPartitionData,
                               DeviceDriverMem_t const * const * pNextDvr )
{
   return ( eSUCCESS );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t dvr_write()">
/***********************************************************************************************************************

   Function Name: dvr_write

   Purpose: Writes to the flash memory if data to write is different than the data in the flash memory.  An erase
            command is issued if the flash memory isnt already cleared.

   Arguments:
      dSize DestOffset:  Location of the data to write to the NV Memory
      uint8_t *pSrc:  Location of the source data to write to Flash
      lCnt Cnt:  Number of bytes to write to the NV Memory.
      PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                    the partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: returnStatus_t

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_write ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_write ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_write( dSize destOffset, uint8_t const * pSrc, lCnt Cnt, PartitionData_t const *pParData,
                                 DeviceDriverMem_t const * const * pNextDvr )
{
   returnStatus_t eRetVal = eSUCCESS;

   lCnt phDestAddr = pParData->PhyStartingAddress + destOffset; /* Get the phy address. */

   ASSERT( Cnt <= EXT_FLASH_SECTOR_SIZE ); /* Count must be <= the sector size. */

   if (traceFlashAccess)
      DBG_printf ("dvr_write: offset=%08X   phyAddress=%08X   Cnt=%d", destOffset, pParData->PhyStartingAddress, Cnt);

   if (Cnt)
   {
      ( void ) dvr_shm_lock(); /* Take mutex, critical section */

      eRetVal = flashRead( (uint16_t*)&dvr_shm_t.pExtSectBuf_[0], (uint16_t*)phDestAddr, Cnt );

      if ( eSUCCESS == eRetVal )
      {
         if ( memcmp( &dvr_shm_t.pExtSectBuf_[0], pSrc, Cnt ) ) /*lint !e670 Cnt checked in assert above. */
         {
            /* The string is different, so data needs to be written. Check if the flash is erased */
            bool bEraseBeforeWrite = WRITE_WITHOUT_ERASING; /* Assume we'll write without erasing the sector(s) */
            uint8_t *pFlashData;             /* Local pointer for the flash buffer data. */
            lCnt  i;                         /* Used as a counter for the for loops below. */
            for ( pFlashData = &dvr_shm_t.pExtSectBuf_[0], i = Cnt; i; i--, pFlashData++ )
            {
               if ( *pFlashData != 0 )  /* Data is inverted.  This is an inverted 0xFF */
               {
                  /* Need to erase before we can write data. */
                  bEraseBeforeWrite = ERASE_BEFORE_WRITE;
                  break;
               }
            }
            eRetVal = flashWrite( (uint16_t*) (lCnt) phDestAddr, (uint16_t*) (lCnt) pSrc, (lCnt) Cnt, bEraseBeforeWrite );
         }
      }
      ( void ) dvr_shm_unlock(); /* End critical section */
   }
   return ( eRetVal );
}


// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t dvr_read()">
/***********************************************************************************************************************

   Function Name: dvr_read

   Purpose: Reads the External Flash memory

   Arguments:
      uint8_t *pDest:  Location to write data
      dSize srcOffset:  Location of the source data in memory
      lCnt cnt:  Number of bytes to read.
      PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                    the partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: returnStatus_t

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_read ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_read ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_read( uint8_t *pDest, const dSize srcOffset, const lCnt Cnt, PartitionData_t const *pParData,
                                DeviceDriverMem_t const * const * pNextDvr )
{
   returnStatus_t eRetVal = eSUCCESS;

   lCnt phDestAddr = pParData->PhyStartingAddress + srcOffset; /* Get the phy address. */

   ASSERT( Cnt <= EXT_FLASH_SECTOR_SIZE ); /* Count must be <= the sector size. */

   if (traceFlashAccess)
      DBG_printf ("dvr_read: offset=%08X   phyAddress=%08X   Cnt=%d", srcOffset, pParData->PhyStartingAddress, Cnt);

   if ( Cnt )
   {
      ( void ) dvr_shm_lock(); /* Take mutex, critical section */

      eRetVal = flashRead( (uint16_t*) (lCnt) pDest, (uint16_t*)phDestAddr, (lCnt) Cnt  );   /* 16 bit reads */

      ( void ) dvr_shm_unlock(); /* End critical section */
   }
   return ( eRetVal );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t erase()">
/***********************************************************************************************************************

   Function Name: dvr_erase

   Purpose: Erases the external flash memory.

   Arguments:
      dSize offset - Address to erase
      lCnt cnt - number of bytes to erase
      PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                    the partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: returnStatus_t

   Side Effects:

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_erase ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_erase ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_erase( dSize destOffset, lCnt Cnt, PartitionData_t const *pPartitionData,
                             DeviceDriverMem_t const * const * pNextDvr )
{

   returnStatus_t eRetVal = eFAILURE;
   lCnt phDestOffset = destOffset + pPartitionData->PhyStartingAddress; /* Get the actual address in NV. */
   lCnt nBytesOperatedOn; /* Number of bytes to operate on, Sector or Block */

   if ( ( destOffset + Cnt ) <= EXT_FLASH_SIZE )
   {
      eRetVal = eSUCCESS; /* Assume Success - Also checked in while loop below. */

      /* Take mutex, critical section */
      ( void ) dvr_shm_lock();

      while ( Cnt && ( eSUCCESS == eRetVal ) )
      {
         nBytesOperatedOn = EXT_FLASH_SECTOR_SIZE;

         if ( ( !( phDestOffset % EXT_FLASH_SECTOR_SIZE ) ) && ( Cnt >= EXT_FLASH_SECTOR_SIZE ) )
         {
            eRetVal = flashEraseSector ( (uint16_t*)phDestOffset );
         }
         else /* Need to erase a partial sector. */
         {
            phAddr nStartingAddr = phDestOffset - ( phDestOffset % EXT_FLASH_SECTOR_SIZE );

            eRetVal = flashRead( (uint16_t*)&dvr_shm_t.pExtSectBuf_[0], (uint16_t*)nStartingAddr, (lCnt) EXT_FLASH_SECTOR_SIZE );

            if ( eSUCCESS == eRetVal )
            {
               eRetVal = flashEraseSector ( (uint16_t*)phDestOffset );

               if (eSUCCESS == eRetVal)
               {
                  if ( ( phDestOffset % sizeof( dvr_shm_t.pExtSectBuf_ ) + Cnt ) > sizeof( dvr_shm_t.pExtSectBuf_ ) )
                  {
                     nBytesOperatedOn = sizeof( dvr_shm_t.pExtSectBuf_ ) -
                        ( phDestOffset % sizeof( dvr_shm_t.pExtSectBuf_ ) );
                  }
                  else
                  {
                     nBytesOperatedOn = Cnt;
                  }
                  ( void )memset( &dvr_shm_t.pExtSectBuf_[phDestOffset % sizeof ( dvr_shm_t.pExtSectBuf_ )], 0, nBytesOperatedOn );

                  eRetVal = flashWrite( (uint16_t*)nStartingAddr, (uint16_t*)&dvr_shm_t.pExtSectBuf_[0], (size_t) EXT_FLASH_SECTOR_SIZE, WRITE_WITHOUT_ERASING );
               }
            }
         }
         phDestOffset += nBytesOperatedOn;
         Cnt -= nBytesOperatedOn;
      }
      ( void ) dvr_shm_unlock(); /* End critical section */
   }
   return ( eRetVal );
}

/***********************************************************************************************************************

   Function Name: flashErase Sector

   Purpose:        Erases flash sector with this address

   Arguments:      pFlash

   Returns:       returnStatus_t

   Side Effects:   the sector is erased

   Reentrant Code: No - Mutex must already be locked

 **********************************************************************************************************************/
static returnStatus_t flashEraseSector ( uint16_t *pFlashAddr )
{
   returnStatus_t eRetVal = eFAILURE;

   ASSERT( pFlashAddr >= EXT_FLASH_BASE_ADDR );

   if ((uint32_t) pFlashAddr % 2 )
   {
      pFlashAddr= (uint16_t*) ((uint32_t) pFlashAddr+1);
   }
   uint32_t offset = ((uint32_t) pFlashAddr - (uint32_t) BSP_EXTERNAL_NORFLASH_ROM_BASE ) >> 1;

   DEVSTATUS devStatus = lld_SectorEraseOp(offset);
   if (devStatus == DEV_NOT_BUSY)
   {
      eRetVal = eSUCCESS;
   }
   return ( eRetVal );
}


/***********************************************************************************************************************

   Function Name: flashRead

   Purpose:        Read data from NV memory to *pData.

   Arguments: uint8_t *pDest,  phAddr nSrc,  lCnt Cnt, void *pDevice

   Returns: returnStatus_t

   Side Effects: The External Flash memory is read

   Reentrant Code: No - Mutex must already be locked

 **********************************************************************************************************************/
static returnStatus_t flashRead ( uint16_t *pDest, uint16_t * flashAddr, lCnt byteCount )
{
   returnStatus_t eRetVal = eSUCCESS;
   lCnt wordCount;
   uint8_t* pBuf = (uint8_t*) pDest;

   ASSERT( pFlashAddr >= EXT_FLASH_BASE_ADDR );

   uint32_t offset = ((uint32_t) flashAddr - (uint32_t) BSP_EXTERNAL_NORFLASH_ROM_BASE) >> 1;  // Divide by 2.

   // both the count and flash address must be even.

   if ( byteCount % 2 )
      byteCount++;   /* Need even number bytes */

   wordCount = byteCount >> 1;

   // Make sure the flash address is 16 bit aligned.
   if ( (uint32_t) flashAddr % 2 )
   {
      offset++;
   }

   if (traceFlashAccess)
      DBG_printf("readingFlash: Offset=%08X Words=%08X", offset, wordCount);

   lld_ReadBufferOp (offset, (FLASHDATA*) pDest, (FLASHDATA) wordCount);

   INVB_invertBits( pBuf, byteCount ); /* Invert Every Byte Read */

   return ( eRetVal );
}


// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t localWrite()">
/***********************************************************************************************************************

   Function Name: flashWrite

   Purpose: Writes data to NV memory erasing sectors as commanded.

   Arguments: phAddr nDest, uint8_t *pSrc, lCnt Cnt, enum bEraseBeforeWrite

   Returns: returnStatus_t


   Reentrant Code: No - Locked by mutex in the calling function(s)

 **********************************************************************************************************************/
static returnStatus_t flashWrite( uint16_t* pFlashAddr, uint16_t const *pSrc, lCnt byteCount, bool const bEraseBeforeWrite )
{
   DEVSTATUS devStatus;
   returnStatus_t eRetVal = eSUCCESS; /* Return status for this function */\
   lCnt wordCount;

   ASSERT( pFlashAddr >= EXT_FLASH_BASE_ADDR );

   if ( byteCount != 0 )
   {
      if ((uint32_t) pFlashAddr % 2 )
      {
         pFlashAddr = (uint16_t*) ((uint32_t) pFlashAddr+1);
      }
      if (byteCount%2)
      {
         if (traceFlashAccess)
            DBG_printf("flashWrite received Odd # bytes: %d ", byteCount);

         byteCount++;
      }

      if (traceFlashAccess)
          DBG_printf("flashWrite: pFlashAddr=%08X  Cnt=%d    eraseBeforewrite=%d", pFlashAddr, byteCount, (int) bEraseBeforeWrite);

      if (bEraseBeforeWrite) /* Erase before writing?  If so, a read-erase-write process is performed. */
      {
         uint8_t numOfSectorsToWrite; /* Number of sectors to be written */
         /* Write the data to memory, if needed an read-erase will occur before the write.  The write may span
            multiple sectors, so a loop is used to go through each sector. */
         for ( numOfSectorsToWrite = ( uint8_t ) ( ( ( ( (uint32_t) pFlashAddr + byteCount - 1 ) / EXT_FLASH_SECTOR_SIZE ) -
                                                    ( (uint32_t) pFlashAddr / EXT_FLASH_SECTOR_SIZE ) ) + 1 );
            ( eSUCCESS == eRetVal ) && numOfSectorsToWrite && byteCount;
            numOfSectorsToWrite-- )
         {
            dSize sectorStartAddr = ( (uint32_t) pFlashAddr / EXT_FLASH_SECTOR_SIZE ) * EXT_FLASH_SECTOR_SIZE; /* Calc sector addr */
            if (traceFlashAccess)
                 DBG_printf  ("Reading: Addr=%08X  Cnt=%lu", (uint16_t*) sectorStartAddr,( lCnt )sizeof( dvr_shm_t.pExtSectBuf_ ));

            eRetVal = flashRead ( (uint16_t*) &dvr_shm_t.pExtSectBuf_[0], (uint16_t*) sectorStartAddr,( lCnt )sizeof( dvr_shm_t.pExtSectBuf_ )); /* Read entire sector */

            if ( eSUCCESS == eRetVal ) /* Successfully read? */
            {
               if (traceFlashAccess)
                   DBG_printf("Erasing: addr=%08X", (uint16_t*) sectorStartAddr);

               /* Erase the sector */
               eRetVal = flashEraseSector ( (uint16_t*) sectorStartAddr );

               if (eSUCCESS == eRetVal)
               {
                  dSize numBytesToCopyToBuffer = byteCount; /* Calc number of bytes to copy to the local buffer from pSrc */
                  if ( numOfSectorsToWrite > 1 )
                  {
                     /* If this is not the 1st sector, then the rest of the data from pSrc is copied. */
                     numBytesToCopyToBuffer = EXT_FLASH_SECTOR_SIZE - ( (uint32_t) pFlashAddr % EXT_FLASH_SECTOR_SIZE );
                  }
                  /* Copy the pSrc data to the local buffer.  Since the sector is being written, the data can start
                     anywhere in the local buffer.  For example, if address 10 is written, the offset into the local
                     buffer will be 10 also.  If the data spans to the next sector, then the offset is zero for the next
                     sector.  */
                  ( void )memcpy( &dvr_shm_t.pExtSectBuf_[( sectorStartAddr + ( (uint32_t) pFlashAddr % EXT_FLASH_SECTOR_SIZE ) ) % EXT_FLASH_SECTOR_SIZE],
                                  pSrc, numBytesToCopyToBuffer );

                  INVB_invertBits( &dvr_shm_t.pExtSectBuf_[0], sizeof ( dvr_shm_t.pExtSectBuf_ ) );

                  pFlashAddr += numBytesToCopyToBuffer; /* Increment to the next destination address */
                  pSrc += numBytesToCopyToBuffer; /* Increment to the next source data address */
                  byteCount -= numBytesToCopyToBuffer; /* Decrement the number of bytes remaining to be copied. */

                  uint32_t offset = sectorStartAddr - (uint32_t) BSP_EXTERNAL_NORFLASH_ROM_BASE;
                  offset = offset >>  1;                  /* Since we offset to the flash controller, we need to convert the offset to words. */
                  // We write in words.  Divide count by 2.

                  if (traceFlashAccess)
                     DBG_printf("writingFlash:  Offset=%08X Words=%08X", offset, EXT_FLASH_SECTOR_SIZE/2);

                  devStatus = lld_WriteBufferProgramOp (offset, EXT_FLASH_SECTOR_SIZE/2, (uint16_t*) &dvr_shm_t.pExtSectBuf_[0] );
                  if (devStatus == DEV_NOT_BUSY)
                  {
                     eRetVal = eSUCCESS;
                  }
                  else
                  {
                     eRetVal = eFAILURE;
                  }
               }
               else
               {
                  eRetVal = eFAILURE;
               }
            }
         }
      }
      else /* Nothing to erase, just write the bytes! */
      {
         uint32_t offset =  (uint32_t) pFlashAddr - (uint32_t) BSP_EXTERNAL_NORFLASH_ROM_BASE;

         INVB_memcpy(&dvr_shm_t.pExtSectBuf_[0], (uint8_t *)pSrc, byteCount); /* Invert the data. */

         wordCount = byteCount >> 1;

         offset = offset >> 1;      // Flash controller needs a word offset

         if (traceFlashAccess)
            DBG_printf("writingFlash: (NothingToErase) Offset=%08X Words=%08X", offset, wordCount);

         devStatus = lld_WriteBufferProgramOp (offset, wordCount, (uint16_t*)  &dvr_shm_t.pExtSectBuf_[0] );
         if (devStatus == DEV_NOT_BUSY)
         {
            eRetVal = eSUCCESS;
         }
         else
         {
            eRetVal = eFAILURE;
         }
      }
   }
   return ( eRetVal );
}


// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t flush()">
/***********************************************************************************************************************

   Function Name: dvr_flush

   Purpose: Keeps the API the same for this driver.  Performs no functionality.

   Arguments:
      PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                    the partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_flush ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_flush ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_flush( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr )
{
   lld_ResetCmd ( );

   return ( eSUCCESS );
}

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t dvr_ioctl()">
/***********************************************************************************************************************

   Function Name: dvr_ioctl

   Purpose: The eflash driver doesn't support ioctl commands

   Arguments:
      void *pCmd  Command to execute:
      void *pData  Data to device.
      PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                    the partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_ioctl ) : It is a part of the common API. */
/*lint -efunc( 818, dvr_ioctl ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_ioctl( const void *pCmd, void *pData, PartitionData_t const *pPartitionData,
                                 DeviceDriverMem_t const * const * pNextDvr )
{
   returnStatus_t retVal = eSUCCESS;

   return ( retVal );
}

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static returnStatus_t restore()">
/***********************************************************************************************************************

   Function Name: restore

   Purpose: The flash driver doesn't support the restore command

   Arguments:
      PartitionData_t const *pParData  Points to a partition table entry.  This contains all information to access the
                               partition to initialize.
      DeviceDriverMem_t const * const * pNextDriver  Points to the next drivers table.

   Returns: eSUCCESS

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData,
                               DeviceDriverMem_t const * const * pNextDriver )
{
   /*lint -efunc( 715, restore ) : Parameters are not used.  It is a part of the common API. */
   /*lint -efunc( 818, restore ) : Parameters are not used.  It is a part of the common API. */
   return ( eSUCCESS );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="static bool timeSlice() ">
/***********************************************************************************************************************

   Function Name: timeSlice

   Purpose: Common API function, allows for the driver to perform any housekeeping that may be required.  For this
            module, nothing is to be completed.

   Arguments: PartitionData_t const *, struct DeviceDriverMem_t const *

   Returns: bool - 1 if the nv device is busy, 0 if not busy

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, timeSlice ) : It is a part of the common API. */
/*lint -efunc( 818, timeSlice ) : It is a part of the common API. */
static bool timeSlice( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver )
{
   return( false );
}
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void DVR_EFL_unitTest(void)">
/* ****************************************************************************************************************** */
/* Unit Test Code */

#ifdef TM_DVR_EXT_FL_UNIT_TEST
#include "partitions.h"
#include "partition_cfg.h"
/*lint -e{578} argument same as enum OK  */
static uint32_t incCountLimit( uint32_t count, uint32_t limit )
{
   if( count < limit )
   {
      count++;
   }
   return count;
}
/***********************************************************************************************************************

   Function Name: DVR_ENFL_UnitTest

   Purpose: Unit test module for External Flash Device Driver.  This unit test does the following:
      Writes a pattern to the bank
      Reads the bank and verifies the data (address = data)
      Repeats the read as many times as requested in by parameter ReadRepeat

   Arguments: ReadRepeat - number of times to read the bank and verify

   Returns: eSUCCESS if all operations and comparisons succeed; eFAILURE otherwise

   Side Effects: Flash memory will be left in an erased state.

   Reentrant Code: No

 **********************************************************************************************************************/
uint32_t DVR_ENFL_UnitTest( uint32_t ReadRepeat )
{
   returnStatus_t          retVal;
   uint32_t                failCount = 0;
   uint8_t                 unitTestBuf[254]; /* Make this the size of a bank, minus the meta data  */
   PartitionData_t const * partitionData;    /* Pointer to partition information   */
   uint8_t                 startPattern;

#if 0
   traceFlashAccess = 1;
#endif

   startPattern = ( uint8_t )aclara_rand();
   /* Find the NV test partition - don't care about update rate   */
   retVal = PAR_partitionFptr.parOpen( &partitionData, ePART_NV_TEST, 0xffffffff );
   if ( eSUCCESS == retVal  )
   {
      /* Fill the buffer with an pattern  */
      for ( uint32_t i = 0; i < sizeof( unitTestBuf ); i++ )
      {
         unitTestBuf[i] = ( uint8_t )i + startPattern;
      }
      /* Fill the partition with the pattern */
      for( uint32_t i = 0; i < partitionData->lDataSize / sizeof( unitTestBuf ); i++ )
      {
         if (traceFlashAccess)
            DBG_printf("parWrite(%d) to phyAddr=%08X  lDataSize=%08X  lSize=%08X", i, partitionData->PhyStartingAddress, partitionData->lDataSize, partitionData->lSize);
         if ( eSUCCESS != PAR_partitionFptr.parWrite( 0, &unitTestBuf[0],
                                     sizeof( unitTestBuf ), partitionData ) )
         {
            if (traceFlashAccess)
               DBG_printf ("Write FAILED");
            if( failCount < 0xffff )
            {
               failCount = incCountLimit( failCount, 0xffff );
            }
         }
      }
      if (traceFlashAccess)
         DBG_printf ("Verify...");

      /* Read the partition back, and compare with expected pattern (multiple times)  */
      for ( uint32_t readLoop = ReadRepeat; readLoop; readLoop-- )
      {
         if ( eSUCCESS == retVal )
         {
            for( uint32_t i = 0; i < partitionData->lDataSize / sizeof( unitTestBuf ); i++ )
            {
               ( void )memset( unitTestBuf, 0, sizeof( unitTestBuf ) );


               if (traceFlashAccess)
                  DBG_printf ("parRead (%d) to phyAddr=%08X  lDataSize=%08X  lSize=%08X", i, partitionData->PhyStartingAddress, partitionData->lDataSize, partitionData->lSize);
               if ( eSUCCESS != PAR_partitionFptr.parRead( &unitTestBuf[0], 0, sizeof( unitTestBuf ), partitionData ))
               {
                  DBG_printf( "NV Test - Read failed at 0x%x, len = 0x%x", 0, sizeof( unitTestBuf ) );
               }
               for ( uint32_t j = 0; j < sizeof( unitTestBuf ); j++ )
               {
                  if( unitTestBuf[j] != ( uint8_t )( startPattern + j ) )
                  {
                     failCount = incCountLimit( failCount, 0xffff );
                     DBG_printf( "NV Test - Compare failed at 0x%x, is = 0x%02x, s/b 0x%02x\n",
                                 partitionData->PhyStartingAddress + j,
                                 unitTestBuf[j], (uint8_t)( startPattern + j ) );
                  }
               }
            }
         }
         else
         {
            failCount = incCountLimit( failCount, 0xffff );
         }
      }
   }
   else
   {
      failCount = incCountLimit( failCount, 0xffff );
   }

   traceFlashAccess = 0;

   return failCount;
}
#endif



/***********************************************************************************************************************

   Function Name: hexDump

   Purpose: module for dumping memory to validate the flash/ram drivers

   Arguments: addr & cnt.

   Returns: none

   Side Effects: outputs debug

   Reentrant Code: No

 **********************************************************************************************************************/
static void hexDump (void *addr, int len)
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    buff [0] = 0;

    if (len <= 0)
    {
        DBG_printf("  Invalid Length");
        return;
    }

    DBG_printf ("hexDump at %08X [cnt=%d]]", addr, len);

    // Process every byte in the data.
    for (i = 0; i < len; i++)
    {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0)
        {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
            {
               /* Print the line out */
               DBG_printfNoCr ("     ");
               DBG_printf ((char*) buff);
            }
            // Output the offset.
            DBG_printfNoCr( "  %04x ", i);
        }

        // Now the hex code for the specific character.
        DBG_printfNoCr (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0)
    {
       DBG_printfNoCr ("   ");
       i++;
    }

    // And print the final Line.

    DBG_printfNoCr ("     ");
    DBG_printf ((char*) buff);
}



