/***********************************************************************************************************************

   Filename:   dvr_extflash.c

   Global Designator: DVR_EFL_

   Contents: Driver for the External Flash Memory.

 **********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2011-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************

   $Log$ kdavlin Created Feb 18, 2011

 ***********************************************************************************************************************
   Notes:   Low power mode uses a timer to ensure the processor sleeps for the proper amount of time.  In low power
            mode, all interrupts MUST be disabled in the system including the RTOS's ISR(s).

            Low Power Modes:
               * Busy After Erase - Processor goes to deep sleep, awakes with the low power timer.
               * Busy After Write - Processor does what?

            The configuration settings for this module are located in the HAL_??? header file.  This file should not
            change even if the SPI port changes.

 ***********************************************************************************************************************
   Revision History:
   v0.1 - KAD 02/22/2011 - Initial Release

 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */


#include "project_BL.h"
#define dvr_eflash_GLOBAL
#include "dvr_extflash_BL.h"
#undef  dvr_eflash_GLOBAL

#ifndef __BOOTLOADER
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#include "sys_clock_BL.h"
#elif ( RTOS_SELECTION == FREE_RTOS )
#include "hal_data.h"
#endif
#else   /* BOOTLOADER */
#include <string.h>
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
#include <MK66F18.h>
#elif (MCU_SELECTED == NXP_K24)
#include <MK24F12.h>
#elif (MCU_SELECTED == RA6E1)
#include "hal_data.h"
#endif
#endif  /* NOT BOOTLOADER */

#include "partitions_BL.h"
#include "spi_mstr_BL.h"
#include "invert_bits.h"
#include "byteswap.h"
#ifndef __BOOTLOADER
#include "DBG_SerialDebug.h"
#endif   /* BOOTLOADER  */
#include "dvr_sharedMem_BL.h"
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#ifndef __BOOTLOADER
#define DEBUG_HARDWARE_BUSY   0
#define DEBUG_POLL_BUSY   0
/*lint -esym(750,HOLD_PIN_ON,MISO_INT_DETECT_ENABLE,MISO_INT_DETECT_DISABLE,) */
/*lint -esym(750,FL_INSTR_WRITE_EN_STATUS_REG,FL_INSTR_READ_ID) */
/*lint -esym(749,eEraseCmd::eERASE_32K) */
#define USE_POWER_MODE              1                 /* Use the power mode in this version */

#define ERASED_FLASH_BYTE           ((uint8_t)0xFF)  /* Data value of an erased byte */
#endif   /* __BOOTLOADER   */

#ifdef EXT_FLASH_WP_USED
#if ( MCU_SELECTED == NXP_K24 )
#define WRITE_PROTECT_PIN_ON()      { NV_WP_TRIS(); NV_WP_ACTIVE(); }    /* Sets WP Pin on flash active (not writable) */
#define WRITE_PROTECT_PIN_OFF()     { NV_WP_TRIS(); NV_WP_INACTIVE(); }  /* Sets WP pin on flash inactive (writable) */
#elif ( MCU_SELECTED == RA6E1 )
#define WRITE_PROTECT_PIN_ON()      R_BSP_PinWrite(BSP_IO_PORT_05_PIN_04, BSP_IO_LEVEL_LOW)
#define WRITE_PROTECT_PIN_OFF()     R_BSP_PinWrite(BSP_IO_PORT_05_PIN_04, BSP_IO_LEVEL_HIGH);
#endif
#define WRITE_PROTECT_PIN_SPI()
#ifndef __BOOTLOADER
#if ( MCU_SELECTED == NXP_K24 )
#define WRITE_PROTECT_PIN_GPIO()    NV_WP_TRIS()      /* Set to GPIO Mode */
#elif ( MCU_SELECTED == RA6E1 )
#define WRITE_PROTECT_PIN_GPIO()
#endif
#endif   /* __BOOTLOADER   */
#else
#define WRITE_PROTECT_PIN_ON()
#define WRITE_PROTECT_PIN_OFF()
#define WRITE_PROTECT_PIN_SPI()
#define WRITE_PROTECT_PIN_GPIO()
#endif

#ifdef EXT_FLASH_HOLD
#define HOLD_PIN_OFF()              gpio_set_gpio_pin(EXT_FLASH_HOLD)   /* Don't hold the SPI Bus */
#define HOLD_PIN_SPI()              gpio_enable_gpio_pin(EXT_FLASH_HOLD) /* Set to SPI Mode */
#ifndef __BOOTLOADER
#define HOLD_PIN_ON()               gpio_clr_gpio_pin(EXT_FLASH_HOLD)   /* Hold the SPI Bus */
#define HOLD_PIN_GPIO()             gpio_set_gpio_open_drain_pin(EXT_FLASH_HOLD) /* Set to GPIO Mode */
#endif   /* __BOOTLOADER   */
#else
#define HOLD_PIN_OFF()
#define HOLD_PIN_SPI()
#ifndef __BOOTLOADER
#define HOLD_PIN_ON()
#define HOLD_PIN_GPIO()
#endif   /* __BOOTLOADER   */
#endif

#ifndef __BOOTLOADER
#ifdef ATMEL
/* For the SST part, the MISO (input) pin on the AVR32 can be used to detect when the device is no longer busy. */
#define MISO_INT_DETECT_ENABLE()       gpio_enable_pin_interrupt(EXT_FLASH_SI, GPIO_RISING_EDGE)
#define MISO_INT_DETECT_DISABLE()      gpio_disable_pin_interrupt(EXT_FLASH_SI)
#else
#define MISO_INT_DETECT_ENABLE()
#define MISO_INT_DETECT_DISABLE()
#endif
#endif   /* __BOOTLOADER   */

/* Flash Device Instruction Set */
#define FL_INSTR_HIGH_SPEED_READ       ((uint8_t)0x0B)  /* High speed read, uses a 'dummy' byte after the address. */
#ifndef __BOOTLOADER
#define FL_INSTR_4K_SECTOR_ERASE       ((uint8_t)0x20)  /* Erase 4K bytes, a sector */
#define FL_INSTR_32K_BLOCK_ERASE       ((uint8_t)0x52)  /* Erase 32K bytes, a block on some devices, not used! */
#define FL_INSTR_64K_BLOCK_ERASE       ((uint8_t)0xD8)  /* Erase 64K bytes, a block on all devices. */
#define FL_INSTR_CHIP_ERASE            ((uint8_t)0xC7)  /* Entire chip erase command value */
#define FL_INSTR_BYTE_PROGRAM          ((uint8_t)0x02)  /* Program a byte of flash memory */
#endif   /* __BOOTLOADER   */
#define FL_INSTR_WRITE_DISABLE         ((uint8_t)0x04)  /* Disables writes */
#define FL_INSTR_READ_STATUS_REG       ((uint8_t)0x05)  /* Reads the status register, used for polling the busy flag */
#ifndef __BOOTLOADER
#define FL_INSTR_WRITE_EN_STATUS_REG   ((uint8_t)0x50)  /* Enables writes */
#define FL_INSTR_WRITE_STATUS_REG      ((uint8_t)0x01)  /* Writes to the status register */
#define FL_INSTR_WRITE_ENABLE          ((uint8_t)0x06)  /* Enables writes */
#define FL_INSTR_READ_ID               ((uint8_t)0x90)  /* 0xAB may also be used for Reading the ID */
#define FL_INSTR_JEDEC_ID_READ         ((uint8_t)0x9f)  /* Reads the MFG ID, part size */

/* Device Specific Instructions */
#define FL_INSTR_AAI_WORD_PGM          ((uint8_t)0xAD)  /* Atmel Part Only! Auto Addr Inc Prog, multi-byte writes */
#define FL_INSTR_ENABLE_SO_BUSY        ((uint8_t)0x70)  /* This only works for the SST part. "EBSY" */
#define FL_INSTR_DISABLE_SO_BUSY       ((uint8_t)0x80)  /* This only works for the SST part. "DBSY" */
#define FL_INTSR_BLOCK_PRTC_UNLOCK     ((uint8_t)0x98)  /* Global Block Protection Unlock for SST26VF016B and W25Q16JV*/

#define ERASE_BEFORE_WRITE             ((bool)true)
#define WRITE_WITHOUT_ERASING          ((bool)false)
#endif   /* __BOOTLOADER   */

#define STATUS_BUSY_MASK               ((uint8_t)1)     /* Busy Mask for the FLASH Busy Flag */

#ifndef __BOOTLOADER
#define SST_GBP_MFG_ID                 ((uint8_t)0xBF)  /* Manufacturer ID for  SST 16Mb 26VF016B */
#define SST_GBP_DEVICE_TYPE            ((uint8_t)0x26)  /* Device Type for SST 16Mb 26VF016B */
#define SST_GBP_DEVICE_ID_16           ((uint8_t)0x41)  /* Devide ID for SST 16Mb 26VF016B */
#define SST_GBP_DEVICE_ID_32           ((uint8_t)0x42)  /* Devide ID for SST 32Mb 26VF032B */
#endif   /* __BOOTLOADER   */


/* For Unit Testing, we need to return a FAILURE in place of the assert.  */
#if defined TM_EFLASH_UNIT_TEST || defined TM_DVR_PARTITION_UNIT_TEST
#undef ASSERT
#define ASSERT(__e) if (!(__e)) return(eFAILURE)
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
/*lint -esym(749,eERASE_32K)  The symbols below may not be referenced! */
#ifndef __BOOTLOADER
typedef enum
{
   eERASE_4K   = FL_INSTR_4K_SECTOR_ERASE,   /* Erase a 4K Sector */
   eERASE_32K  = FL_INSTR_32K_BLOCK_ERASE,   /* Erase a 32K Sector - Not used! */
   eERASE_64K  = FL_INSTR_64K_BLOCK_ERASE,   /* Erase a 64K Block */
   eERASE_CHIP = FL_INSTR_CHIP_ERASE         /* Erase the entire chip */
} eEraseCmd; /* Used when calling the local erase function. */

/*lint -esym(754,u32PgmTimePage_uS)  The symbols below may not be referenced! */
typedef struct
{
   uint32_t u32EraseTimeChip_uS; /* Worst case timing to erase the chip. */
   uint32_t u32EraseTime64K_uS;  /* Worst case timing to erase 64K bytes */
   uint32_t u32EraseTime4K_uS;   /* Worst case timing to erase 4K bytes */
   uint32_t u32PgmTimeByte_uS;   /* Worst case timing to program a byte */
   uint32_t u32PgmTimePage_uS;   /* Worst case timing to write an array of bytes, AAI (auto address inc) mode */
   uint16_t u16MaxSeqWrite;      /* Maximum number of bytes that can be written at one time */
   uint8_t  u8MfgId;             /* This is a unique number in the JEDEC read instruction. */
   uint8_t  u8DeviceType;        /* Serial SPI flash, etc.  */
   uint8_t  u8DeviceID;          /* Specific device (e.g., SST25VF080B, SST25VF160B */
   bool     bBusyOnSiPin;        /* Device supports busy detection on the device SO pin (AVR32 MISO Pin) */
   bool     bAAI;                /* Supports Auto Address Increment */
} DeviceId_t; /* Used to define a table that identifies the MFG, Erase times and busy detection. */
#endif   /* __BOOTLOADER   */

PACK_BEGIN /* Needs to be packed since this data is sent directly to the flash device. */
typedef struct PACK_MID
{
   uint8_t instr;          /* The instruction is 1 byte */
   uint8_t address[3];     /* the address is a 3 byte field followed by a "dummy" byte.  */
   uint8_t dummyByte;      /* Dummy byte after address. */
} tDeviceReadInstrAddr;    /* Used for high-speed read instructions. */
PACK_END

#ifndef __BOOTLOADER
PACK_BEGIN /* Needs to be packed since this data is sent directly to the flash device. */
typedef struct PACK_MID
{
   uint8_t instr;       /* The instruction is 1 byte */
   uint8_t address[3];  /* the address is a 3 byte field */
} tDeviceInstrAddr;     /* Used for low-speed read, write and erase instructions */
PACK_END
PACK_BEGIN /* Needs to be packed since this data is sent directly to the flash device. */
typedef struct PACK_MID
{
   uint8_t instr;       /* The instruction is 1 byte */
   uint8_t address[3];  /* the address is a 3 byte field */
   uint8_t data[2];     /* 2 bytes of data   */
} tAAI;     /* Used for AAI write sequences  */
PACK_END
#endif   /* __BOOTLOADER   */
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* Functions accessed via a table entry (indirect call) */
static returnStatus_t dvr_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_read( uint8_t *pDest, const dSize srcOffset, lCnt Cnt, PartitionData_t const *pParData,
                                DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t init( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr );
#ifndef __BOOTLOADER
static returnStatus_t close( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t pwrMode( const ePowerMode ePwrMode, PartitionData_t const *pPartitionData,
                               DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_write( dSize destOffset, uint8_t const *pSrc, lCnt Cnt, PartitionData_t const *pParData,
                                 DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t erase( dSize destOffset, lCnt Cnt, PartitionData_t const *pPartitionData,
                             DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t flush( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t dvr_ioctl( const void *pCmd, void *pData, PartitionData_t const *pPartitionData,
                                 DeviceDriverMem_t const * const * pNextDvr );
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData,
                               DeviceDriverMem_t const * const * pNextDriver );
static bool        timeSlice( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver );
#endif

/* local functions */
static returnStatus_t localRead( uint8_t *pDest, const phAddr nSrc, const lCnt Cnt, const SpiFlashDevice_t *pDevice );
static void           disableWrites( uint8_t port );
static returnStatus_t busyCheck( const SpiFlashDevice_t *pDevice, uint32_t u32BusyTime_uS );
#ifndef __BOOTLOADER
static returnStatus_t localWrite( dSize nDest, uint8_t const *pSrc, lCnt Cnt, bool const bEraseBeforeWrite,
                                  const SpiFlashDevice_t *pDevice );
static returnStatus_t IdNvMemory( SpiFlashDevice_t const *pDevice );
static returnStatus_t localErase( const eEraseCmd eCmd,
                                  phAddr nAddr, uint32_t busyTime_uS, const SpiFlashDevice_t *pDevice );
static returnStatus_t localWriteBytesToSPI( dSize nDest, uint8_t *pSrc, lCnt Cnt, const SpiFlashDevice_t *pDevice );
static void           setBusyTimer( uint32_t busyTimer_uS );
static void           enableWrites( uint8_t port );
#if ( MCU_SELECTED == NXP_K24 )
static void           isr_busy( void );
static void           isr_tmr( void );
#elif ( MCU_SELECTED == RA6E1 )
static fsp_err_t      MisoBusy_isr_init( void );
#endif
#endif

#if (( RTOS == 1 ) && ( MCU_SELECTED == RA6E1 ) )
static OS_MUTEX_Obj qspiMutex_;        /* Mutex Lock for QSPI channel */
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
#define AAI_WORD_PGM_SIZE ((uint8_t)2)

/* Each entry represents the manufactures ID, worst case timing and if the SI pin supports a busy indicator. */
#ifndef __BOOTLOADER
static const DeviceId_t sDeviceId[] =
{
   /*   Chip     64K Erase  4K Erase  Byte PGM  Page PGM  Max Bytes   MFG   Device  Dev   MISO  AAI   */
   /* Erase uS       uS        uS       uS        uS         WR       ID     Type    ID   Busy        */
#if 0
   /* Disallow a "default" configuration. Require a device ID that is recognizable.                         */
   {  8000000,   3000000,     200000,   12,       5000,         1,    0x00,  0x00,  0x00,   0,    0 },  /* Default */
#endif
   {    50000,     25000,      25000,   10,         10,         1,    0xBF,  0x25,  0x8E,   1,    1 },  /* SST 8Mb */
   {    50000,     25000,      25000,   10,         10,         1,    0xBF,  0x25,  0x41,   0,    0 },  /* SST 16Mb 25VF016B */
   {    50000,     25000,      25000,   70,       1500,       256,    0xBF,  0x26,  0x41,   0,    0 },  /* SST 16Mb 26VF016B */
   {    50000,     25000,      25000,   70,       1500,       256,    0xBF,  0x26,  0x42,   0,    0 },  /* SST 32Mb 26VF032B */
   { 15000000,   1000000,     300000,   25,       1000,       256,    0x9D,  0x40,  0x15,   0,    0 },  /* ISSI 16Mb 25LQ080B */
   { 25000000,   3000000,     300000,  100,       2500,       256,    0x1F,  0x86,  0x01,   0,    0 },  /* Adesto 16Mb AT25SF161 */
   { 25000000,   2000000,     400000,  100,       3000,       256,    0xEF,  0x40,  0x15,   0,    0 },  /* Windbond 16Mb W25Q16JV */
#if 0
   /* Temporarily removed. If this JEDEC id is returned (errantly) with the SST device installed, writes fail. Found
      that even reducing the Max Bytes WR to 1 still resulted in failed writes. */
   { 80000000,   3000000,     150000, 5000,       5000,       256,    0x20,  0x00,  0x00,   0,    0 },  /* Numonyx */
#endif
   {  8000000,    950000,     200000,    7,       3000,         1,    0x1F,  0x00,  0x00,   0,    0 },  /* Atmel */
#if 0
   {    10000,     10000,      10000,    7,       5000,       256,    0x7F,  0x00,  0x00,   0,    0 }   /* ISSI */
#else
   /* Temporarily modify the Max Bytes WR so that if this JEDED id is returned (errantly) with the SST device installed,
      writes still pass.   */
   {    10000,     10000,      10000,    7,       5000,         1,    0x7F,  0x00,  0x00,   0,    0 }   /* ISSI */
#endif
};
#endif  /* NOT BOOTLOADER */

/* The driver table must be defined after the function definitions. The list below are the supported commands for this
   driver. */
/* For the bootloader code we use open and read only */
DeviceDriverMem_t sDeviceDriver_eFlash =
{
#ifdef __BOOTLOADER
   init,       // Init function - Empty function for BOOTLOADER
   dvr_open,   // Open Command - For this implementation it only calls the lower level drivers.
   NULL,       // Close Command - For this implementation it only calls the lower level drivers.
   NULL,       // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost
   dvr_read,   // Read Command
   NULL,       // Write Command
   NULL,       // Erases a portion (or all) of the banked memory.
   NULL,       // Write the cache content to the lower layer driver
   NULL,       // ioctl function - Does Nothing for this implementation
   NULL,       // Not supported - API support only
   NULL        // Not supported - API support only
#else
   init,       // Init function - Creates mutex & calls lower drivers
   dvr_open,   // Open Command - For this implementation it only calls the lower level drivers.
   close,      // Close Command - For this implementation it only calls the lower level drivers.
   pwrMode,    // Sets the power mode of the lower drivers (the app may set to low power mode when power is lost
   dvr_read,   // Read Command
   dvr_write,  // Write Command
   erase,      // Erases a portion (or all) of the banked memory.
   flush,      // Write the cache content to the lower layer driver
   dvr_ioctl,  // ioctl function - Does Nothing for this implementation
   restore,    // Not supported - API support only
   timeSlice   // Not supported - API support only
#endif  /* NOT BOOTLOADER */
};

#if ( MCU_SELECTED == NXP_K24 )
/* Spi port configuration for use with the external flash devices.  The #defines are located in the cfg_app.h. */
static const spiCfg_t _NV_spiCfg =
{
   EXT_FLASH_SPEED_KHZ,          /* SPI Clock Rate */
   EXT_FLASH_TX_BYTE_WHEN_RX,    /* SPI TX Byte when Receiving*/
   EXT_FLASH_SPI_MODE            /* SPI Mode */
};
#endif

#ifndef __BOOTLOADER
#if ( MCU_SELECTED == NXP_K24 )
#define NV_SPI_ChkSharedPortCfg(...)   SPI_ChkSharedPortCfg(__VA_ARGS__, &_NV_spiCfg)
#define NV_SPI_MutexLock(...)          SPI_MutexLock(__VA_ARGS__)
#define NV_SPI_MutexUnlock(...)        SPI_MutexUnlock(__VA_ARGS__)
#endif
#else
#define NV_SPI_ChkSharedPortCfg(...)
#define NV_SPI_MutexLock(...)
#define NV_SPI_MutexUnlock(...)
#endif

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#if RTOS == 1
static OS_SEM_Obj    extFlashSem_;           /* Semaphore used when waiting for busy signal for external flash ISR */
static bool          flashSemCreated_ = false;
#else
#ifndef __BOOTLOADER
STATIC volatile bool bBusyIsr_ = (bool)false; /* Used by the busy function when RTOS is disabled.  */
#endif   /* __BOOTLOADER   */
#endif

STATIC bool          bWrEnabled_ = 1;        /* 1 = Write has been enabled on the Flash device. */
STATIC uint8_t       dvrOpenCnt_ = 0;        /* # of times driver has been opened, can close driver when value is 0. */
#if ( DVR_EFL_BUSY_EDGE_TRIGGERED != 0 ) /* Edge triggered */
STATIC uint32_t      noReadyIRQ = 0;         /* Counter of hardwary busy check ready immediately. */
#endif

#ifndef __BOOTLOADER

#if USE_POWER_MODE
STATIC ePowerMode ePwrMode_ = ePWR_NORMAL;   /* Determines if this dvr will put the micro to sleep when busy */
STATIC volatile bool bTmrIsrTriggered_ = 1;  /* 0 = ISR Not triggered, 1 = ISR triggered */
#endif

/* Used to compare a string to write to the flash memory.  Also used when writing a sector (read-modify-write). */
STATIC bool bUseHardwareBsy_ = 0;            /* 1 = Hardware busy has been enabled, detect busy using MISO pin. */
STATIC uint32_t busyTime_uS_;                /* When checking for busy, contains the max time we should wait */
STATIC eDVR_EFL_IoctlRtosCmds_t rtosCmds_ = eRtosCmdsEn;   /* eRtosCmdsDis or eRtosCmdsEn */
//STATIC bool rtosCmds_ = (bool)eRtosCmdsEn;  /* eRtosCmdsDis or eRtosCmdsEn */
STATIC const DeviceId_t *pChipId_ = &sDeviceId[0];  /* Flash Mem MFG, Points to the sDeviceId table, init to DEFAULT. */

#endif
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
/***********************************************************************************************************************

   Function Name: init

   Purpose: Init Function

   Arguments: PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr

   Returns: returnStatus_t

   Side Effects: SPI Driver may change configuration

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, init ) : Parameters are not used.  It is a part of the common API. */
/*lint -efunc( 818, init ) : Parameters are not used.  It is a part of the common API. */
static returnStatus_t init( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr )
{
#if RTOS == 1
   /* Create share memory */
   (void)dvr_shm_init();
   if ( flashSemCreated_ == false )
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      if ( true == OS_SEM_Create( &extFlashSem_, 0) )
      {
         flashSemCreated_ = true;
      } /* end if() */
#if ( MCU_SELECTED == NXP_K24 )
      /* Set up the ISR for MISO busy signal.  This is only used for the SST device. */
      if ( NULL != _int_install_isr( DVR_EFL_BUSY_IRQIsrIndex, (INT_ISR_FPTR)isr_busy, NULL ) )
      {
         (void)_bsp_int_init( DVR_EFL_BUSY_IRQIsrIndex,
                              DVR_EFL_BUSY_ISR_PRI, DVR_EFL_BUSY_ISR_SUB_PRI, (bool)true );
      }

      /* Todo:  Abstract the following: */
      SIM_SOPT2 |= SIM_SOPT2_RTCCLKOUTSEL_MASK; /* use 32KHz clock */
      SIM_SCGC5 |= SIM_SCGC5_LPTMR_MASK; //lint !e40 !e737
      EXT_FLASH_TIMER_COMPARE = 0;
      LPTMR0_PSR = LPTMR_PSR_PCS( 1 ) | LPTMR_PSR_PBYP_MASK; /*lint !e835 */
      LPTMR0_CSR = LPTMR_CSR_TPS( 2 );
      if ( NULL != _int_install_isr( (int)INT_LPTimer, (INT_ISR_FPTR)isr_tmr, NULL ) )
      {
         (void)_bsp_int_init( (int)INT_LPTimer, 2, 0, (bool)true );
      }
#elif ( MCU_SELECTED == RA6E1 )
      OS_MUTEX_Create( &qspiMutex_);
      /* Initializes the timer module. */
      R_AGT_Open(&AGT0_ExtFlashBusy_ctrl, &AGT0_ExtFlashBusy_cfg);
      NV_SPI_PORT_INIT( &g_qspi0_ctrl, &g_qspi0_cfg );
      R_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);
      (void)MisoBusy_isr_init();

      /* TODO: RA6: Access needs to be enabled so we can operate the chip select of external flash during startup.
               Need to investigate if this should be moved to a more appropriate/generic location, otherwise having
               it here ensures it will be enabled before files stored in NV are utilized. */
      R_BSP_PinAccessEnable();
#endif
   }
#if ( (DEBUG_HARDWARE_BUSY == 1) || (DEBUG_POLL_BUSY == 1 ) ) /* For NV device ready */
   PORTE_PCR24 = PORT_PCR_MUX(1);   /* Make GPIO   */
   GPIOE_PDDR |= ( 1 << 24 );       /* Make output */
   GPIOE_PCOR = ( 1 << 24 );        /* Set output low */
#endif
#if ( MCU_SELECTED == NXP_K24 )
   return ( NV_SPI_PORT_INIT( ((SpiFlashDevice_t*)pPartitionData->pDriverCfg)->port ) );
#else
   return ( eSUCCESS );
#endif
#else
   return ( eSUCCESS );
#endif
}
/***********************************************************************************************************************

   Function Name: dvr_open

   Purpose: Initializes the External Flash Driver.  Sets the write protect and hold I/O pins, creates the Mutex for the
            write and read functions and creates the Semaphore for the busy function.

   Arguments: PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr

   Returns: returnStatus_t

   Side Effects: SPI Driver may change configuration

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_open ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_open ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_open( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr )
{
   returnStatus_t eRetVal = eSUCCESS;
   const SpiFlashDevice_t* pDevice = ((SpiFlashDevice_t*)pParData->pDriverCfg);

   if ( dvrOpenCnt_ == 0 )       /* Only need to open the device once   */
   {
      dvr_shm_lock();   /* Take mutex, critical section */

      HOLD_PIN_OFF();            /* Configure the Hold pin */
      WRITE_PROTECT_PIN_ON();    /* Configure the WR pin */
      WRITE_PROTECT_PIN_SPI();   /* Enable the WP GPIO Pin */
      HOLD_PIN_SPI();            /* Enable the HOLD GPIO Pin */

      NV_CS_INACTIVE();
      NV_CS_TRIS();

      /* Initialize the SPI Driver */
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexLock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
#if ( MCU_SELECTED == NXP_K24 )
      eRetVal = NV_SPI_PORT_OPEN( pDevice->port, &_NV_spiCfg, SPI_MASTER );
#endif
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif

      if ( eSUCCESS == eRetVal )
      {
#if 0 //( NV_SPI_PORT_CONTROLS_CS == 1 )
         /* Deprecated */
         /* Call SPI Driver Config, make the flash driver control the Chip Select. */
         eRetVal = SPI_CfgPort( pDevice->port,
                                pDevice->pcs,
                                &_NV_spiCfg );
#endif
         if ( eSUCCESS == eRetVal ) /*lint !e774  future spi ports may not always return eSuccess */
         {
            WRITE_PROTECT_PIN_OFF(); /* Turn off the write protect pin to the chip */
            /* Make sure the write is disabled.  If an SST part is connected and a spurious reset occurs in the middle
               of an AAI command, we need to disable writes so that we can identify the chip properly.  If this is not
               done, the chip will remain in AAI mode and not accept any write/read/erase commands. */
            do
            {
               bWrEnabled_ = 1;  /* Set the flag to force the write-disable to be written to the flash device. */
               disableWrites( pDevice->port ); /* Disable writes. */

               /* Wait for device NOT busy. May have just come through a software reset (reboot) with last flash
                  operation an erase.  Device may not be immediately ready. Not waiting for ready may result in
                  erroneous read of JEDEC ID. */
               eRetVal = busyCheck( pDevice, 1000 );
            } while ( eRetVal != eSUCCESS );

#ifndef __BOOTLOADER
            {
               /* Clear the "current level of block write protection" bits, bits 2-5 and 7.  This allows us to write to
                  all  of the NV memory with no restrictions. */
               uint8_t instrData[2]; /* Instruction to and data to write that will enable writes */
               enableWrites( pDevice->port ); /* Enable Writes. */
               instrData[0] = FL_INSTR_WRITE_STATUS_REG;
               instrData[1] = 0;
#if ( RTOS_SELECTION == MQX_RTOS )
               NV_SPI_MutexLock(pDevice->port);
               NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
               OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
               NV_CS_ACTIVE();   /* Need to send instruction to Write the Flash with address byte */
#if ( MCU_SELECTED == NXP_K24 )
               (void)NV_SPI_PORT_WRITE( pDevice->port, &instrData[0], sizeof(instrData) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
               (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instrData[0], sizeof(instrData), false); /* WR the Instr to the chip */
#endif

               NV_CS_INACTIVE();
#if ( RTOS_SELECTION == MQX_RTOS )
               NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
               OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
            }
            /* Id the Flash Memory */
            eRetVal = IdNvMemory( pParData->pDriverCfg );


            /* If SST26VF016B memory is installed, unlock global block protection */
            if (  (pChipId_->u8MfgId == SST_GBP_MFG_ID) && (pChipId_->u8DeviceType == SST_GBP_DEVICE_TYPE) &&
                  ( (pChipId_->u8DeviceID == SST_GBP_DEVICE_ID_16) || (pChipId_->u8DeviceID == SST_GBP_DEVICE_ID_32) ) )
            {
               uint8_t instr = FL_INTSR_BLOCK_PRTC_UNLOCK;
#if ( RTOS_SELECTION == MQX_RTOS )
               NV_SPI_MutexLock(pDevice->port);
               NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
               OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
               NV_CS_ACTIVE();
#if ( MCU_SELECTED == NXP_K24 )
               (void)NV_SPI_PORT_WRITE( pDevice->port, &instr, sizeof(instr));
#elif ( MCU_SELECTED == RA6E1 )
               (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof(instr), false);
#endif
               NV_CS_INACTIVE();
#if ( RTOS_SELECTION == MQX_RTOS )
               NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
               OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
            }
#endif  /* NOT BOOTLOADER */
         }
      }
      dvr_shm_unlock(); /* End critical section */
   }
   dvrOpenCnt_++;             /* Update the open count.  */
   return ( eRetVal ); /*lint -esym(715,pNextDvr) */
}
/*lint +esym(715,pNextDvr) */
/***********************************************************************************************************************

   Function Name: close

   Purpose: Closes the external flash, disables the SPI port and sets all SPI pins to GPIO

   Arguments:
      PartitionData_t const *pParData  Points to a partition table entry.  This contains all information to access the
                                    partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
/*lint -efunc( 715, close ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, close ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t close( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDvr )
{
   if ( dvrOpenCnt_ )
   {
      dvrOpenCnt_--;
   }
   if ( 0 == dvrOpenCnt_ ) /* If driver has been closed the same number of times it has been opened, close it. */
   {
#if ( MCU_SELECTED == NXP_K24 )
      (void)SPI_ClosePort( ((SpiFlashDevice_t*)pParData->pDriverCfg)->port );
#elif ( MCU_SELECTED == RA6E1 )
      NV_SPI_PORT_CLOSE( &g_qspi0_ctrl );
#endif

      HOLD_PIN_GPIO();
      WRITE_PROTECT_PIN_GPIO();
   }
   return ( eSUCCESS );
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: pwrMode

   Purpose: Sets the power mode for the busy wait condition.  The power mode defaults to Normal at power up.

   Arguments:  ePowerMode ePwrMode
               PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                            the partition to initialize.
               DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: None

   Side Effects: Sets the global variable ePwrMode_.  After a write/erase the micro will go to sleep in LOW power mode.

   Reentrant Code: Yes - because it is an atomic operation.

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
/*lint -efunc( 715, pwrMode ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, pwrMode ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t pwrMode( const ePowerMode ePwrMode, PartitionData_t const *pPartitionData,
                               DeviceDriverMem_t const * const * pNextDvr )
{
#if ( MCU_SELECTED == NXP_K24 ) // feature not used in RA6E1
#if ( USE_POWER_MODE == 1 )
   ePwrMode_ = ePwrMode;
   SPI_pwrMode( ePwrMode );
#endif
#endif
   return ( eSUCCESS );
}
#endif  /* NOT BOOTLOADER */
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

   Side Effects: SPI Driver may change configuration

   Reentrant Code: Yes

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
/*lint -efunc( 715, dvr_write ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_write ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_write( dSize destOffset, uint8_t const *pSrc, lCnt Cnt, PartitionData_t const *pParData,
                                 DeviceDriverMem_t const * const * pNextDvr )
{
   lCnt phDestAddr = pParData->PhyStartingAddress + destOffset; /* Get the phy address. */

   returnStatus_t eRetVal = eSUCCESS;

   ASSERT( Cnt <= EXT_FLASH_SECTOR_SIZE ); /* Count must be <= the sector size. */
   if ( Cnt )
   {
      dvr_shm_lock(); /* Take mutex, critical section */
#if 0 /* TODO: test only code */
      /* Debug breakpoint for detecting writes to the Signature partition  */
      /* Start in Signature partition  */
      if (
         (  ( pParData->PhyStartingAddress >= PART_NV_APP_RAW_SIG_OFFSET ) &&
            ( pParData->PhyStartingAddress <= ( PART_NV_APP_RAW_SIG_OFFSET + PART_NV_APP_RAW_SIG_SIZE ) )
         )
         ||
         /* End in Signature partition  */
         (  ( ( pParData->PhyStartingAddress + Cnt ) >= PART_NV_APP_RAW_SIG_OFFSET ) &&
            ( ( pParData->PhyStartingAddress + Cnt ) <= ( PART_NV_APP_RAW_SIG_OFFSET + PART_NV_APP_RAW_SIG_SIZE ) )
         )
      )
      {
         NOP();
      }
#endif

      /* Read the data in the flash to compare it to the data to be written. */
      eRetVal = localRead( &dvr_shm_t.pExtSectBuf_[0], phDestAddr, Cnt, pParData->pDriverCfg );
      if ( eSUCCESS == eRetVal )
      {
         /* Is the data to be written the same as the data in the flash? */
         if ( memcmp( &dvr_shm_t.pExtSectBuf_[0], pSrc, ( uint16_t )Cnt ) ) /*lint !e670 Cnt checked in assert above. */
         {
            /* The string is different, so data needs to be written. Check if the flash is erased */
            bool bEraseBeforeWrite = WRITE_WITHOUT_ERASING; /* Assume we'll write without erasing the sector(s) */
            uint8_t *pFlashData;             /* Local pointer for the flash buffer data. */
            lCnt  i;                         /* Used as a counter for the for loops below. */

            for ( pFlashData = &dvr_shm_t.pExtSectBuf_[0], i = Cnt; i; i--, pFlashData++ )
            {
               if ( *pFlashData != 0 )
               {
                  /* Need to erase before we can write data. */
                  bEraseBeforeWrite = ERASE_BEFORE_WRITE;
                  break;
               }
            }
            eRetVal = localWrite( phDestAddr, pSrc, Cnt, bEraseBeforeWrite, pParData->pDriverCfg );
         }
      }
      dvr_shm_unlock(); /* End critical section */
   }
   return ( eRetVal );
}
#endif  /* NOT BOOTLOADER */
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

   Side Effects: SPI Driver may change configuration

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715, dvr_read ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, dvr_read ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_read( uint8_t *pDest, const dSize srcOffset, const lCnt Cnt, PartitionData_t const *pParData,
                                DeviceDriverMem_t const * const * pNextDvr )
{
   returnStatus_t eRetVal = eSUCCESS;

   ASSERT( Cnt <= EXT_FLASH_SECTOR_SIZE ); /* Count must be <= the sector size. */
   if ( Cnt )
   {
#if RTOS == 1
      dvr_shm_lock(); /* Take mutex, critical section */
#endif
      eRetVal = localRead( pDest, pParData->PhyStartingAddress + srcOffset, Cnt, pParData->pDriverCfg );
#if RTOS == 1
      dvr_shm_unlock(); /* End critical section */
#endif
   }
   return ( eRetVal );
}
/***********************************************************************************************************************

   Function Name: erase

   Purpose: Erases the external flash memory.

   Arguments:
      dSize offset - Address to erase
      lCnt cnt - number of bytes to erase
      PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                    the partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: returnStatus_t

   Side Effects: SPI Driver may change configuration

   Reentrant Code: Yes

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
/*lint -efunc( 715, erase ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, erase ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t erase( dSize destOffset, lCnt Cnt, PartitionData_t const *pPartitionData,
                             DeviceDriverMem_t const * const * pNextDvr )
{
   lCnt phDestOffset = destOffset + pPartitionData->PhyStartingAddress; /* Get the actual address in NV. */
   uint32_t newBusyTime;

   returnStatus_t eRetVal = eFAILURE;
   eEraseCmd eECmd; /* Contains the amount of memory to erase. */
   lCnt nBytesOperatedOn; /* Number of bytes to operate on, Sector or Block */

#if 0 /* TODO: test only code */
   /* Debug breakpoint for detecting writes to the Signature partition  */
   /* Start in Signature partition  */
   if (
      (  ( pPartitionData->PhyStartingAddress >= PART_NV_APP_RAW_SIG_OFFSET ) &&
         ( pPartitionData->PhyStartingAddress <= ( PART_NV_APP_RAW_SIG_OFFSET + PART_NV_APP_RAW_SIG_SIZE ) )
      )
      ||
      /* End in Signature partition  */
      (  (  ( pPartitionData->PhyStartingAddress + Cnt ) >= PART_NV_APP_RAW_SIG_OFFSET ) &&
         (  ( pPartitionData->PhyStartingAddress + Cnt ) <=
            ( PART_NV_APP_RAW_SIG_OFFSET + PART_NV_APP_RAW_SIG_SIZE ) )
      )
   )
   {
      NOP();
   }
#endif

   if ( ( phDestOffset + Cnt ) <= EXT_FLASH_SIZE )
   {
      eRetVal = eSUCCESS; /* Assume Seccess - Also checked in while loop below. */

#if RTOS == 1
      /* Take mutex, critical section */
      dvr_shm_lock();
#endif

      while ( Cnt && ( eSUCCESS == eRetVal ) )
      {
         eECmd = eERASE_4K;
         nBytesOperatedOn = EXT_FLASH_SECTOR_SIZE;
         newBusyTime = pChipId_->u32EraseTime4K_uS;

         /* Is the address on a boundary and the Cnt >= the sector size?  This will determine if an entire sector
            can be erased or if a partial sector needs to be erased.  If a partial sector needs to be erased, the
            data that will remain unchanged needs to be read and written back after the sector has been erased.  */
         if ( ( !( phDestOffset % EXT_FLASH_SECTOR_SIZE ) ) && ( Cnt >= EXT_FLASH_SECTOR_SIZE ) )
         {
            /* An entire sector or more needs to be eased, no data needs to be saved. */
            /* Determine if a 64K erase will do. */
            if ( ( ( phDestOffset % EXT_FLASH_BLOCK_SIZE ) == 0 ) && Cnt >= EXT_FLASH_BLOCK_SIZE )
            {
               nBytesOperatedOn = EXT_FLASH_BLOCK_SIZE;
               eECmd = eERASE_64K;
               newBusyTime = pChipId_->u32EraseTime64K_uS;
               if ( ( 0 == phDestOffset ) && ( EXT_FLASH_SIZE == Cnt ) )
               {
                  eECmd = eERASE_CHIP; /* Erase the entire chip */
                  nBytesOperatedOn = EXT_FLASH_SIZE;
                  newBusyTime = pChipId_->u32EraseTimeChip_uS;
               }
            }
            (void)localErase( eECmd, phDestOffset, newBusyTime, pPartitionData->pDriverCfg );
         }
         else /* Need to erase a partial sector. */
         {
            phAddr nStartingAddr = phDestOffset - ( phDestOffset % EXT_FLASH_SECTOR_SIZE );

            eRetVal = localRead( &dvr_shm_t.pExtSectBuf_[0], nStartingAddr, EXT_FLASH_SECTOR_SIZE,
                                 pPartitionData->pDriverCfg );
            if ( eSUCCESS == eRetVal )
            {
               (void)localErase( eERASE_4K, phDestOffset, newBusyTime, pPartitionData->pDriverCfg );

               /* TODO: SMG, Jan26 2016.  What purpose does this serve?  The localErase sets the timer, and localWrite
                  waits for it to be done*/
               setBusyTimer( pChipId_->u32EraseTime4K_uS );
               if ( ( phDestOffset % sizeof( dvr_shm_t.pExtSectBuf_ ) + Cnt ) > sizeof( dvr_shm_t.pExtSectBuf_ ) )
               {
                  nBytesOperatedOn = sizeof( dvr_shm_t.pExtSectBuf_ ) -
                     ( phDestOffset % sizeof( dvr_shm_t.pExtSectBuf_ ) );
               }
               else
               {
                  nBytesOperatedOn = Cnt;
               }
               (void)memset( &dvr_shm_t.pExtSectBuf_[phDestOffset % sizeof(dvr_shm_t.pExtSectBuf_)], 0,
                             (uint16_t)nBytesOperatedOn );
               eRetVal = localWrite( nStartingAddr, &dvr_shm_t.pExtSectBuf_[0], EXT_FLASH_SECTOR_SIZE,
                                     WRITE_WITHOUT_ERASING, pPartitionData->pDriverCfg );
            }
         }
         phDestOffset += nBytesOperatedOn;
         Cnt -= nBytesOperatedOn;
      }
#if RTOS == 1
      dvr_shm_unlock(); /* End critical section */
#endif
   }
   return ( eRetVal );
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: flush

   Purpose: Keeps the API the same for this driver.  Performs no functionality.

   Arguments:
      PartitionData_t *pPartition  Points to a partition table entry.  This contains all information to access
                                    the partition to initialize.
      DeviceDriverMem_t const * const * pNextDvr  Points to the next drivers table.

   Returns: As defined by error_codes.h

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
/*lint -efunc( 715, flush ) : pNextDvr is not used.  It is a part of the common API. */
/*lint -efunc( 818, flush ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t flush( PartitionData_t const *pPartitionData, DeviceDriverMem_t const * const * pNextDvr )
{
   return ( eSUCCESS );
}
#endif  /* NOT BOOTLOADER */
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
#ifndef __BOOTLOADER
/*lint -efunc( 715, dvr_ioctl ) : It is a part of the common API. */
/*lint -efunc( 818, dvr_ioctl ) : pNextDvr is declared correctly.  It is a part of the common API. */
static returnStatus_t dvr_ioctl( const void *pCmd, void *pData, PartitionData_t const *pPartitionData,
                                 DeviceDriverMem_t const * const * pNextDvr )
{
   returnStatus_t retVal = eSUCCESS;

#if ( MCU_SELECTED == NXP_K24 ) /* Allows driver to use a different drive strength; slew rate is not controllable by RA6E1 but the driver capcity
                                   is controllable which can be changed from the RASC Configurator */
   if ( eRtosCmds == *( eDVR_EFL_Cmd_t * )pCmd )
   {
      rtosCmds_ = ( eDVR_EFL_IoctlRtosCmds_t ) * ( eDVR_EFL_IoctlRtosCmds_t * )pData;
      SPI_RtosEnable( ( bool )rtosCmds_ );
   }
#endif
   return ( retVal );
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: localErase

   Purpose: Erases a sector, block or the entire part based on eCmd

   Arguments: eEraseCmd eCmd, phAddr nAddr, void *pDevice

   Returns: returnStatus_t

   Side Effects: SPI Configuration may change

   Reentrant Code: No - Called while the mutex is locked

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
static returnStatus_t localErase(   const eEraseCmd eCmd,
                                    phAddr nAddr,
                                    uint32_t busyTime_uS,
                                    const SpiFlashDevice_t *pDevice )
{
   returnStatus_t eRetVal = eFAILURE;                            /* Return value */
#if ( MCU_SELECTED == RA6E1 )
   fsp_err_t      err;
#endif

   uint8_t        retries = EXT_FLASH_BUSY_RETRIES;   /* #of times to test for retries */

   /* Make sure the previous operation, if any, has finished.  */
   do
   {
      eRetVal = busyCheck( pDevice, busyTime_uS_ );
   } while( ( eSUCCESS != eRetVal ) && ( 0 != retries-- ) );

   if ( eSUCCESS == eRetVal )
   {
      WRITE_PROTECT_PIN_OFF(); /* Disable the hardware write protect */
      enableWrites( pDevice->port ); /* Enable Writes. */

      if ( eERASE_CHIP == eCmd )
      {
         uint8_t instr = (uint8_t)eCmd;
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexLock(pDevice->port);
         NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
         OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
         NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
         eRetVal = NV_SPI_PORT_WRITE( pDevice->port, &instr, sizeof(instr) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
         err = NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof(instr), false ); /* WR the Instr to the chip */
#endif
      }
      else
      {
         tDeviceInstrAddr sIA;
         sIA.instr = (uint8_t)eCmd;
         (void)memcpy( &sIA.address[0], (uint8_t*)&nAddr, sizeof(sIA.address) );
         Byte_Swap( (uint8_t*)&sIA.address[0], sizeof(sIA.address) );
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexLock(pDevice->port);
         NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
         OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
         NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
         eRetVal = NV_SPI_PORT_WRITE( pDevice->port, (uint8_t*)&sIA, sizeof(sIA) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
         err = NV_SPI_PORT_WRITE( &g_qspi0_ctrl, (uint8_t*)&sIA, sizeof(sIA), false ); /* WR the Instr to the chip */
#endif
      }
      NV_CS_INACTIVE(); /* Release the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
      //WRITE_PROTECT_PIN_ON();
   }
   /* In case this is an erase resulting from a flush (especially during power down), wait for the erase to complete
      before returning. */
   setBusyTimer( busyTime_uS );  /* This value set based on the specific device parameters and erase command (e.g., 4k,
                                    16k, whole device, etc. ) */
   bUseHardwareBsy_ = 0;         /* Make sure that the busy check reads the status register! */
   do
   {
      eRetVal = busyCheck( pDevice, busyTime_uS );
   } while( ( eSUCCESS != eRetVal ) && ( 0 != retries-- ) );
   WRITE_PROTECT_PIN_ON();
#if ( MCU_SELECTED == RA6E1 )
   if(FSP_SUCCESS == err)
   {
      eRetVal = eSUCCESS;
   }
#endif
   return ( eRetVal );  /*lint !e438 last value of retries not used  */
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: busyCheck

   Purpose: Checks if the Flash is busy writing or erasing.

   Arguments: void *pDevice, uint32_t u32BusyTime_uS - number of microseconds the part is expected to be busy.

   Returns: returnStatus_t

   Side Effects: Spi driver will change configuration

   Reentrant Code: yes

 **********************************************************************************************************************/
/*lint -efunc( 715, busyCheck ) : It is a part of the common API. */
static returnStatus_t busyCheck( const SpiFlashDevice_t *pDevice, uint32_t u32BusyTime_uS )
{
   returnStatus_t retVal = eSUCCESS;

   /* Is this an SST Device?  If so, the interrupts can be used to determine when the part is no longer busy. */
   /* bUseHardwareBsy_ is 1 when using an SST part AND performing an AAI command. */
#ifndef __BOOTLOADER
   if ( bUseHardwareBsy_ && pChipId_->bBusyOnSiPin )
   {
#if ( MCU_SELECTED == NXP_K24 )
      (void)NV_MISO_CFG( pDevice->port, SPI_MISO_GPIO_e ); /* Change pin to digital input from SPI MISO */
#elif ( MCU_SELECTED == RA6E1 )
      (void)NV_MISO_CFG( BSP_IO_PORT_05_PIN_03, (uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT );
#endif
#if !RTOS
      bBusyIsr_ = false;
#endif
#if USE_POWER_MODE
      if ( ePWR_NORMAL == ePwrMode_ )
      {
         if ( eRtosCmdsEn == rtosCmds_ )
         {
#if ( DVR_EFL_BUSY_EDGE_TRIGGERED != 0 ) /* Edge triggered */
            DVR_EFL_BUSY_IRQ_EI();                          /* Enable the ISR on MISO. */
#if ( RTOS_SELECTION == MQX_RTOS )
            NV_SPI_MutexLock(pDevice->port);
            NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
            OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
            NV_CS_ACTIVE();                                 /* Activate the chip select  */
#else /* Level triggered   */
#if ( RTOS_SELECTION == MQX_RTOS )
            NV_SPI_MutexLock(pDevice->port);
            NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
            OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
            NV_CS_ACTIVE();                                 /* Activate the chip select  */
            DVR_EFL_BUSY_IRQ_EI();                          /* Enable the ISR on MISO. */
#endif
#if ( DVR_EFL_BUSY_EDGE_TRIGGERED != 0 ) /* Edge triggered */
            if ( !NV_BUSY() ) /*  Is device ready immediately upon chip select?  */
            {
               noReadyIRQ++;  /* Count these for characterization, only */
            }
            /* Pend on ISR, timeout either after passed in time or (at least)1 OS tick, which ever is greater.  */
            else if ( !OS_SEM_Pend( &extFlashSem_, max( OS_TICK_mS + 1, u32BusyTime_uS / OS_TICK_uS ) ) )
#else
            if ( !OS_SEM_Pend( &extFlashSem_, max( OS_TICK_mS + 1, u32BusyTime_uS / OS_TICK_uS ) ) )
#endif
            {
#if (DEBUG_HARDWARE_BUSY == 1 ) /* For debugging missed ready/busy interrupts   */
               /* Missed the RY/BY# interrupt - good place for a scope trigger and breakpoint */
               GPIOE_PSOR = ( 1 << 24 );        /* Set output high, trigger scope */
               NOP();
               GPIOE_PCOR = ( 1 << 24 );        /* Set output low */
#endif
               retVal = (returnStatus_t)NV_BUSY();
            }
         }
#if !RTOS
         else
         {
#if ( DVR_EFL_BUSY_EDGE_TRIGGERED != 0 ) /* Edge triggered */
         DVR_EFL_BUSY_IRQ_EI();                             /* Enable the ISR on MISO. */
         SPI_MutexLock(pDevice->port);
         NV_CS_ACTIVE();                                    /* Activate the chip select  */
#else /* Level triggered   */
         SPI_MutexLock(pDevice->port);
         NV_CS_ACTIVE();                                    /* Activate the chip select  */
         DVR_EFL_BUSY_IRQ_EI();                             /* Enable the ISR on MISO. */
#endif
            while( !bBusyIsr_ )                             /* Wait for ISR completion */
            {}
         }
#endif
      }
      else
#endif   //end of #if USE_POWER_MODE
      {
         /* Set up a timer (or WDT) just in case the MISO ISR doesn't fire or fires before we go to sleep. */
         setBusyTimer( 2000 );
#if ( DVR_EFL_BUSY_EDGE_TRIGGERED != 0 ) /* Edge triggered */
         DVR_EFL_BUSY_IRQ_EI();                             /* Enable the ISR on MISO. */
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexLock(pDevice->port);
         NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
         OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
         NV_CS_ACTIVE();                                    /* Activate the chip select  */
#else /* Level triggered   */
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexLock(pDevice->port);
         NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
         OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
         NV_CS_ACTIVE();                                    /* Activate the chip select  */
         DVR_EFL_BUSY_IRQ_EI();                             /* Enable the ISR on MISO. */
#endif
         /* NOTE:  The MISO pin will wake the processor up via the ISR, the timer is a back-up. */
#if USE_POWER_MODE
         if ( !bTmrIsrTriggered_ )
         {
#if ( MCU_SELECTED == NXP_K24 ) //TODO Melvin: Need to identify the freeRTOS equivalent
            SLEEP_CPU_IDLE();   /*lint !e522 function has no side effects; OK   */
#endif
         }
#endif
      }
      NV_CS_INACTIVE();                                     /* Deactivate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
#if ( MCU_SELECTED == NXP_K24 )
      (void)NV_MISO_CFG( pDevice->port, SPI_MISO_SPI_e );   /* Make pin SPI again MISO */
#elif ( MCU_SELECTED == RA6E1 )
      (void)NV_MISO_CFG( BSP_IO_PORT_05_PIN_03, ((uint32_t) IOPORT_CFG_PERIPHERAL_PIN | (uint32_t) IOPORT_PERIPHERAL_QSPI) );
#endif
   }
   else
#endif   /* NOT BOOTLOADER  */
   {
      uint8_t nvStatus;                                       /* Contains the Flash nvStatus. */
      uint8_t instr = FL_INSTR_READ_STATUS_REG;               /* Instruction to send to the flash device. */

#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexLock(pDevice->port);
      NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
      NV_CS_ACTIVE();                           /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
      (void)NV_SPI_PORT_WRITE( pDevice->port, &instr, sizeof(instr) ); /* write the read command to the chip */
#elif ( MCU_SELECTED == RA6E1 )
      (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof(instr), true ); /* write the read command to the chip */
#endif
      nvStatus = STATUS_BUSY_MASK;                /* Set to true, it should be overwritten below. */

#ifndef __BOOTLOADER
      /* create start and end struct to maintain elapsed time during busy loop */
      OS_TICK_Struct startTime, endTime;
      OS_TICK_Get_CurrentElapsedTicks(&startTime);
      endTime = startTime;

      /* Calculate sleep time to be 20 percent of busy time, we don't want to sleep entire busyTime */
      OS_TICK_Struct TickTime;
      uint32_t sleepTime = u32BusyTime_uS / ( uint32_t )( 1000 * 5 );
#endif   /* NOT BOOTLOADER  */

      do /* Spin here until the chip returns a nvStatus of NOT busy. */
      {

#ifndef __BOOTLOADER
         /* Break out of loop if max write execution time of part has expired */
         if ( (uint32_t)OS_TICK_Get_Diff_InMicroseconds( &startTime, &endTime ) > u32BusyTime_uS )
         {
            retVal = eFAILURE;
            break;
         }

         /* Get current Ticktime and put the task to sleep 20 percent of busyTime */
         OS_TICK_Get_CurrentElapsedTicks ( &TickTime );
         OS_TICK_Sleep ( &TickTime, sleepTime );

         OS_TICK_Get_CurrentElapsedTicks(&endTime); /* update endtime to the latest ticktime */
#endif   /* NOT BOOTLOADER  */
#if ( MCU_SELECTED == NXP_K24 )
         (void)NV_SPI_PORT_READ( pDevice->port, &nvStatus, sizeof(nvStatus) ); /* check nvStatus for busy */
#elif ( MCU_SELECTED == RA6E1 )
         (void)NV_SPI_PORT_READ( &g_qspi0_ctrl, &nvStatus, sizeof(nvStatus), true );
#endif
      } while ( STATUS_BUSY_MASK & nvStatus ); /* break out of do while loop when status is not busy */

#if ( MCU_SELECTED == RA6E1 )
      /* We are done polling the status bit.  We need indicate that the transaction is complete by closing the
         SPI bus cycle and then turn off QSPI Direct Communication Mode */
      POLLING_READ_EXIT();
#endif

      NV_CS_INACTIVE(); /* Activate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
   }
#ifndef __BOOTLOADER
   busyTime_uS_ = 0;
#endif   /* NOT BOOTLOADER  */
   return ( retVal );
}
/***********************************************************************************************************************

   Function Name: localWrite

   Purpose: Writes data to NV memory erasing sectors as commanded.  Note:  All bytes are inverted when written to NV.

   Arguments: phAddr nDest, uint8_t *pSrc, lCnt Cnt, enum bEraseBeforeWrite, void *pDevice

   Returns: returnStatus_t

   Side Effects: SPI Driver Configuration may change

   Reentrant Code: No - Locked by mutex in the calling function(s)

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
static returnStatus_t localWrite( dSize nDest, uint8_t const *pSrc, lCnt Cnt, bool const bEraseBeforeWrite,
                                  const SpiFlashDevice_t *pDevice )
{
   returnStatus_t eRetVal; /* Return status for this function */

   eRetVal = busyCheck( pDevice, busyTime_uS_ ); /* Make sure the device isn't busy */
   if ( Cnt != 0 )
   {
      if ( eSUCCESS == eRetVal )
      {
         WRITE_PROTECT_PIN_OFF(); /* Turn off the write protect pin to the chip */

         if ( bEraseBeforeWrite ) /* Erase before writing?  If so, a read-erase-write process is performed. */
         {
            uint8_t numOfSectorsToWrite; /* Number of sectors to be written */

            /* Write the data to memory, if needed an read-erase will occur before the write.  The write may span
               multiple sectors, so a loop is used to go through each sector. */
            for ( numOfSectorsToWrite = (uint8_t)( ( ( ( nDest + Cnt - 1 ) / EXT_FLASH_SECTOR_SIZE ) -
                                                          ( nDest / EXT_FLASH_SECTOR_SIZE ) ) + 1 );
                  ( eSUCCESS == eRetVal ) && numOfSectorsToWrite && Cnt;
                  numOfSectorsToWrite-- )
            {
               dSize sectorStartAddr = ( nDest / EXT_FLASH_SECTOR_SIZE ) * EXT_FLASH_SECTOR_SIZE; /* Calc sector addr */
               /* Note:  the localRead function will invert all of the bits in the data being read, all of these bits
                  need to be inverted before writing the data back.  */
               eRetVal = localRead( &dvr_shm_t.pExtSectBuf_[0], sectorStartAddr,
                                    (lCnt)sizeof( dvr_shm_t.pExtSectBuf_ ), pDevice ); /* Read entire sector */
               if ( eSUCCESS == eRetVal ) /* Successfully read? */
               {
                  /* Erase the sector */
                  eRetVal = localErase( eERASE_4K, sectorStartAddr, pChipId_->u32EraseTime4K_uS, pDevice );
               }
               if ( eSUCCESS == eRetVal ) /* was the erase command successful? */
               {
                  dSize numBytesToCopyToBuffer = Cnt; /* Calc number of bytes to copy to the local buffer from pSrc */
                  if ( numOfSectorsToWrite > 1 )
                  {
                     /* If this is not the 1st sector, then the rest of the data from pSrc is copied. */
                     numBytesToCopyToBuffer = EXT_FLASH_SECTOR_SIZE - ( nDest % EXT_FLASH_SECTOR_SIZE );
                  }
                  /* Copy the pSrc data to the local buffer.  Since the sector is being written, the data can start
                     anywhere in the local buffer.  For example, if address 10 is written, the offset into the local
                     buffer will be 10 also.  If the data spans to the next sector, then the offset is zero for the next
                     sector.  */
                  (void)memcpy( &dvr_shm_t.pExtSectBuf_[( sectorStartAddr +
                                                          ( nDest % EXT_FLASH_SECTOR_SIZE ) ) % EXT_FLASH_SECTOR_SIZE],
                                  pSrc,
                                  (uint16_t)numBytesToCopyToBuffer );

                  INVB_invertBits( &dvr_shm_t.pExtSectBuf_[0], sizeof(dvr_shm_t.pExtSectBuf_) );

                  nDest += numBytesToCopyToBuffer; /* Increment to the next destination address */
                  pSrc += numBytesToCopyToBuffer; /* Increment to the next source data address */
                  Cnt -= numBytesToCopyToBuffer; /* Decrement the number of bytes remaining to be copied. */
                  eRetVal = busyCheck( pDevice, busyTime_uS_ ); /* Check for busy */
                  if ( eSUCCESS == eRetVal )
                  {
                     /* Write the data to the SPI Device */
                     eRetVal = localWriteBytesToSPI( sectorStartAddr, &dvr_shm_t.pExtSectBuf_[0],
                                                     EXT_FLASH_SECTOR_SIZE, pDevice );
                  }
               }
            }
         }
         else /* Nothing to erase, just write the bytes! */
         {
            uint8_t const *pSrcBuf = pSrc;   /* Default by using the source passed into this module. */

            INVB_memcpy( &dvr_shm_t.pExtSectBuf_[0], pSrcBuf, ( uint16_t )Cnt ); /* Invert the data. */
            (void)localWriteBytesToSPI( nDest, &dvr_shm_t.pExtSectBuf_[0], Cnt, pDevice );
         }
         (void)busyCheck( pDevice, busyTime_uS_ ); /* Check for busy */
         WRITE_PROTECT_PIN_ON(); /* Set the write protect pin (Write protect the device) */
      }
   }
   else
   {
      NOP();
   }
   return ( eRetVal );
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: localWriteBytesToSPI

   Purpose: Writes an array of bytes to the SPI port (this function assumes the target has been already erased)

   Arguments: phAddr nDest, uint8_t *pSrc, lCnt Cnt, void *pDevice

   Returns: returnStatus_t

   Side Effects: SPI Driver Configuration may change

   Reentrant Code: Yes

 **********************************************************************************************************************/
#ifndef __BOOTLOADER

static returnStatus_t localWriteBytesToSPI( dSize nDest, uint8_t *pSrc, lCnt Cnt, const SpiFlashDevice_t *pDevice )
{
   returnStatus_t eRetVal = eSUCCESS;  /* Asssume a successful transaction. */
   bool bWriteAaiCmd = 1;           /* 1 = write the AAI instruction. */
   lCnt  bytesToWrite;                 /* Set byte number of bytes to write to the chip maximum value. */

   while ( Cnt ) /* Write blocks of data until the Cnt number of bytes are written */
   {
      tAAI sIA;   /* Device Instruction, Address, and data . */

      /* If on a odd byte boundary or if there is only one byte to transmit, don't use AAI.  If the memory doesn't
         support AAI, use byte writes. */
      if ( ( nDest & 1 ) || !pChipId_->bAAI || ( 1 == Cnt ) )
      {
         /* Going to write as many bytes as we can. */
         /* Calculate the number of bytes that CAN be written starting at nDest address.  */
         bytesToWrite = pChipId_->u16MaxSeqWrite - ( nDest % pChipId_->u16MaxSeqWrite );
         if ( bytesToWrite > Cnt ) /* Do we need to write that many bytes? */
         {
            bytesToWrite = Cnt; /* No, only write Cnt number of bytes. */
         }
         Cnt -= bytesToWrite; /* Adjust the count by the number of bytes being written. */

         /* Since a multi-byte write is possible, don't write any bytes with leading FFs. */
         while ( ( 0 != bytesToWrite ) && ( ERASED_FLASH_BYTE == *pSrc ) )
         {
            bytesToWrite--;
            nDest++;
            pSrc++;
         }

         /* Check the trailing bytes to write.  Don't write any unessessary FFs at the end of the array of bytes. */
         lCnt  bytesToCheck = bytesToWrite;              /* Loop Counter */
         lCnt  actualBytesToWrite = bytesToWrite;        /* # of bytes written to the chip, account for FF @ the end. */
         uint8_t *pSrcToCheck = ( pSrc + bytesToCheck ) - 1; /* Start at the end of the array of bytes to write. */
         while ( bytesToCheck-- && ( ERASED_FLASH_BYTE == *pSrcToCheck-- ) ) /* while bytes left & Last byte FFs. */
         {
            actualBytesToWrite--;
         }

         if ( actualBytesToWrite ) /* Are there any bytes to write? */
         {
            /* If any of the SPI_WritePort calls fail, it is fatal and the system resets.  */
            enableWrites( pDevice->port ); /* Write enable the device */
            /* Write instruction and address to the device */
            sIA.instr = FL_INSTR_BYTE_PROGRAM; /* Initialize the instruction to byte program. */
            (void)memcpy( &sIA.address[0], (uint8_t*)&nDest, sizeof(sIA.address) );
            Byte_Swap( (uint8_t*)&sIA.address[0], sizeof(sIA.address) );
#if ( RTOS_SELECTION == MQX_RTOS )
            NV_SPI_MutexLock(pDevice->port);
            NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
            OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
            NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
            (void)NV_SPI_PORT_WRITE(pDevice->port, (uint8_t*)&sIA, sizeof(sIA.instr) + sizeof(sIA.address)); /* Send instr, addr */
            (void)NV_SPI_PORT_WRITE(pDevice->port, pSrc, (uint16_t)actualBytesToWrite); /* Send data */
#elif ( MCU_SELECTED == RA6E1 )
            (void)NV_SPI_PORT_WRITE(&g_qspi0_ctrl, (uint8_t*)&sIA, sizeof(sIA.instr) + sizeof(sIA.address),false); /* Send instr, addr */
            (void)NV_SPI_PORT_WRITE(&g_qspi0_ctrl, pSrc, (uint16_t)actualBytesToWrite, false); /* Send data */
#endif
            NV_CS_INACTIVE(); /* Deactivate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
            NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
            OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
            setBusyTimer( pChipId_->u32PgmTimeByte_uS * actualBytesToWrite );
            eRetVal = busyCheck( pDevice, busyTime_uS_ );
         }
      }
      else  /* Use AAI feature   */
      {
         /* If both bytes of data being written is in the erased state, skip the write.  */
         bytesToWrite = AAI_WORD_PGM_SIZE;  // TODO: RA6: Look at conditional compile solution to handle K24 and RA6 hex file compare
         if ( Cnt < bytesToWrite ) /* If the Cnt is less than the bytes to be written, only write Cnt # of bytes. */
         {
            bytesToWrite = Cnt;
         }
         Cnt -= bytesToWrite;
         if ( ( ERASED_FLASH_BYTE != *pSrc ) || ( ERASED_FLASH_BYTE != *( pSrc + 1 ) ) )
         {
            /* TODO: is the test for pChipId_->bAAI necessary? Already tested in if at top of routine... */
            if ( pChipId_->bAAI && bWriteAaiCmd ) /* Send AAI, need to write enable and write the AAI instruction. */
            {
               if ( pChipId_->bBusyOnSiPin ) /* Is this a 'special' part that we can check for busy the MISO pin? */
               {
                  uint8_t instr = FL_INSTR_ENABLE_SO_BUSY; /* Send instruction to Read the Flash with address byte */
#if ( RTOS_SELECTION == MQX_RTOS )
                  NV_SPI_MutexLock(pDevice->port);
                  NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
                  OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
                  NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
                  (void)NV_SPI_PORT_WRITE( pDevice->port, &instr, sizeof(instr) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
                  (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof(instr), false ); /* WR the Instr to the chip */
#endif
                  NV_CS_INACTIVE(); /* Deactivate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
                  NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
                  OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
                  bUseHardwareBsy_ = 1;
               }

               /* If any of the SPI_WritePort calls fail, it is fatal and the system resets.  */
               enableWrites( pDevice->port ); /* Write enable the device */

               /* Write instruction, address and data to the device */
               sIA.instr = FL_INSTR_AAI_WORD_PGM; /* Initialize the instruction to byte program. */
               (void)memcpy( &sIA.address[0], (uint8_t*)&nDest, sizeof(sIA.address) );
               Byte_Swap( (uint8_t*)&sIA.address[0], sizeof(sIA.address) );
               (void)memcpy( &sIA.data[0], pSrc, sizeof(sIA.data) );
#if ( RTOS_SELECTION == MQX_RTOS )
               NV_SPI_MutexLock(pDevice->port);
               NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
               OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
               NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
               (void)NV_SPI_PORT_WRITE( pDevice->port, (uint8_t*)&sIA, sizeof(sIA) ); /* instr/addr to chip */
#elif ( MCU_SELECTED == RA6E1 )
               (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, (uint8_t*)&sIA, sizeof(sIA),false ); /* instr/addr to chip */
#endif
               bWriteAaiCmd = 0;
            }
            else
            {
               /* This is a continuation of an AAI stream; AD, address, data, were sent previously. sIA.instr is set. Now
                  use two bytes of address field in structure to send data.   */
               (void)memcpy( &sIA.address[0], pSrc, sizeof(sIA.data) );  /* AAI command followed by 2 bytes of data   */
#if ( RTOS_SELECTION == MQX_RTOS )
               NV_SPI_MutexLock(pDevice->port);
               NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
               OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
               NV_CS_ACTIVE(); /* Activate the chip select  */
               /* Write instr, data to the device */
               //lint -e{603,645} sIA is always set prior to executing the following line.
#if ( MCU_SELECTED == NXP_K24 )
               (void)NV_SPI_PORT_WRITE( pDevice->port, (uint8_t*)&sIA.instr,
                                             sizeof(sIA.instr) + sizeof(sIA.data) );
#elif ( MCU_SELECTED == RA6E1 )
               (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, (uint8_t*)&sIA.instr,
                                             sizeof(sIA.instr) + sizeof(sIA.data), false );
#endif
            }
            NV_CS_INACTIVE(); /* Deactivate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
            NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
            OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
            setBusyTimer( pChipId_->u32PgmTimeByte_uS );
            eRetVal = busyCheck( pDevice, busyTime_uS_ );
            if ( Cnt <= 1 ) /* If there are 0-1 bytes left, need to disable the AAI function. */
            {
               disableWrites( pDevice->port ); /* Disable writes. */
            }
         }
         else  /* Data was skipped, so the next time data is written, the AAI cmd and address must be written. */
         {
            if ( !bWriteAaiCmd ) /* If this is the 1st time data is skipped, end the AAI command. */
            {
               disableWrites( pDevice->port ); /* Disable writes. */
            }
            bWriteAaiCmd = 1;
         }
      }
      nDest += bytesToWrite;
      pSrc += bytesToWrite; /*lint !e662 : Range is checked with an ASSERT before this is called. */
   }
   disableWrites( pDevice->port ); /* Disable writes. */
   return ( eRetVal );
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: setBusyTimer

   Purpose: Sets the busy timer and the busy variable

   Arguments: busyTimer_uS - timer value in microseconds

   Returns: None

   Side Effects: Enables the device writes

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
static void setBusyTimer( uint32_t busyTimer_uS )
{
   busyTime_uS_ = busyTimer_uS;
#if 0 // TODO: RA6E1: Melvin: revisit with a OS Timer
   EXT_FLASH_TIMER_DIS();
   EXT_FLASH_TIMER_COMPARE = busyTimer_uS / 1000;  /* 1mS timer, so convert to mS */
   EXT_FLASH_TIMER_RST();               /* Reset the timer */
#if USE_POWER_MODE
   bTmrIsrTriggered_ = 0;               /* Clear the triggered flag, will be set by ISR when tmr expires. */
#endif
   EXT_FLASH_TIMER_EN();                /* Enable Interrupt & Start the timer. */
#endif  // if 0

#if USE_POWER_MODE  // TODO: RA6E1: Remove if the above TODO is resolved
   bTmrIsrTriggered_ = 0;               /* Clear the triggered flag, will be set by ISR when tmr expires. */
#endif

   timer_info_t   info;
   uint32_t       timer_freq_hz;

   (void) R_AGT_InfoGet(&AGT0_ExtFlashBusy_ctrl, &info);

   timer_freq_hz = info.clock_frequency;
   uint32_t period_counts = (uint32_t) (((uint64_t) timer_freq_hz * busyTimer_uS) / 1000);

   R_AGT_PeriodSet( &AGT0_ExtFlashBusy_ctrl, period_counts );

   /* Start the timer. */
   (void) R_AGT_Start(&AGT0_ExtFlashBusy_ctrl);
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: enableWrites

   Purpose: Enables the writing to the flash device

   Arguments: uint8_t port

   Returns: None

   Side Effects: Enables the device writes

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
static void enableWrites( uint8_t port )
{
   uint8_t instr = FL_INSTR_WRITE_ENABLE; /* Need to send instruction to enable writes */

#if USE_POWER_MODE
   bTmrIsrTriggered_ = 0;               /* Clear the triggered flag, will be set by ISR when tmr expires. */
#endif
#if ( RTOS_SELECTION == MQX_RTOS )
   NV_SPI_MutexLock(port);
   NV_SPI_ChkSharedPortCfg(port);
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
   NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
   ( void )NV_SPI_PORT_WRITE( port, &instr, sizeof ( instr ) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
   ( void )NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof ( instr ), false ); /* WR the Instr to the chip */
#endif
   NV_CS_INACTIVE(); /* Activate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
   NV_SPI_MutexUnlock(port);
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
   bWrEnabled_ = 1;
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: disableWrites

   Purpose: Disables the write command and if needed, will disable SST hardware busy.

   Arguments: uint8_t port

   Returns: None

   Side Effects: Disables the device writes

   Reentrant Code: No

 **********************************************************************************************************************/
static void disableWrites( uint8_t port )
{
   if ( bWrEnabled_ )
   {
      uint8_t instr = FL_INSTR_WRITE_DISABLE; /* Disable the write commands in the device */
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexLock(port);
      NV_SPI_ChkSharedPortCfg(port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
      NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
      (void)NV_SPI_PORT_WRITE( port, &instr, sizeof(instr) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
      (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof(instr), false ); /* WR the Instr to the chip */
#endif
      NV_CS_INACTIVE(); /* Activate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexUnlock(port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
#ifndef __BOOTLOADER
      if ( bUseHardwareBsy_ && pChipId_->bBusyOnSiPin )
      {
         instr = FL_INSTR_DISABLE_SO_BUSY;
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexLock(port);
         NV_SPI_ChkSharedPortCfg(port);
#elif ( RTOS_SELECTION == FREE_RTOS )
         OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
         NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
         (void)NV_SPI_PORT_WRITE( port, &instr, sizeof(instr) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
         (void)NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof(instr), false ); /* WR the Instr to the chip */
#endif
         NV_CS_INACTIVE(); /* Activate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexUnlock(port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
         bUseHardwareBsy_ = 0;
      }
#endif
      bWrEnabled_ = 0;
   }
}
/***********************************************************************************************************************

   Function Name: localRead

   Purpose: Make sure the Flash memory isn't busy and then read data from NV memory to *pData.

   Arguments: uint8_t *pDest,  phAddr nSrc,  lCnt Cnt, void *pDevice

   Returns: returnStatus_t

   Side Effects: The External Flash memory is read, the SPI port configuration is changed.

   Reentrant Code: No - Mutex must already be locked

 **********************************************************************************************************************/
static returnStatus_t localRead( uint8_t *pDest, const dSize nSrc, const lCnt Cnt, const SpiFlashDevice_t *pDevice )
{
   returnStatus_t eRetVal = eFAILURE;
#if ( MCU_SELECTED == RA6E1 )
   fsp_err_t      err;
#endif
#ifndef __BOOTLOADER
   eRetVal = busyCheck( pDevice, busyTime_uS_ ); /*lint !e838  eRetVal is set twice */
   if ( eSUCCESS == eRetVal )
#endif
   {
      /* Need to send instruction to Read the Flash with address byte */
      tDeviceReadInstrAddr sIA;
      sIA.instr = FL_INSTR_HIGH_SPEED_READ;
      sIA.dummyByte = 0;
      (void)memcpy( &sIA.address[0], (uint8_t*)&nSrc, sizeof(sIA.address) );
      Byte_Swap( (uint8_t*)&sIA.address[0], sizeof(sIA.address) );
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexLock(pDevice->port);
      NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
      NV_CS_ACTIVE(); /* Assert the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
      eRetVal = NV_SPI_PORT_WRITE( pDevice->port, ( uint8_t * ) & sIA, sizeof ( sIA ) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
      err = NV_SPI_PORT_WRITE( &g_qspi0_ctrl, ( uint8_t * ) & sIA, sizeof ( sIA ), true); /* WR the Instr to the chip */
#endif

      if (
#if ( MCU_SELECTED == NXP_K24 )
         eSUCCESS == eRetVal
#elif ( MCU_SELECTED == RA6E1 )
         FSP_SUCCESS == err
#endif
         )
      {
         /* Read from the SPI port */
#if ( MCU_SELECTED == NXP_K24 )
         eRetVal = NV_SPI_PORT_READ( pDevice->port, pDest, (uint16_t)Cnt ); /* Read the data back */
#elif ( MCU_SELECTED == RA6E1 )
         err = NV_SPI_PORT_READ( &g_qspi0_ctrl, pDest, (uint16_t)Cnt, false ); /* Read the data back */
#endif
         if (
#if ( MCU_SELECTED == NXP_K24 )
         eSUCCESS == eRetVal
#elif ( MCU_SELECTED == RA6E1 )
         FSP_SUCCESS == err
#endif
         )
         {
            INVB_invertBits( pDest, (uint16_t)Cnt ); /* Invert Every Byte Read */
         }
      }
      NV_CS_INACTIVE(); /* Release the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
      NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
   }
#if ( MCU_SELECTED == RA6E1 )
   if(FSP_SUCCESS == err)
   {
      eRetVal = eSUCCESS;
   }
#endif
   return ( eRetVal );
}
/***********************************************************************************************************************

   Function Name: IdNvMemory

   Purpose: Get the MFG ID of the flash device.  This will help determine the timing for busy signal detection.

   Arguments: void *pDevice

   Returns: returnStatus_t eSUCCESS or eFAILURE

   Side Effects:  Sets the pointer to the MFG Id (see Table 1).  If the MFG is not found, the pointer is set to the
                  Default device.

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
static returnStatus_t IdNvMemory( SpiFlashDevice_t const *pDevice )
{
   returnStatus_t eRetVal = eFAILURE;
#if ( MCU_SELECTED == RA6E1 )
   fsp_err_t      err;
#endif
   uint8_t        instr = FL_INSTR_JEDEC_ID_READ; /* Instruction to read the device ID */
   bool           found = (bool)false;

   /* Stay in this loop until a valid ID found. If unknown device installed, FAIL here.   */
   do
   {
      eRetVal = busyCheck( pDevice, busyTime_uS_ );

      if ( eSUCCESS == eRetVal )
      {
         /* Need to send instruction to Read the Flash with address byte */
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexLock(pDevice->port);
         NV_SPI_ChkSharedPortCfg(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
         OS_MUTEX_Lock( &qspiMutex_ );  // Function will not return if it fails
#endif
         NV_CS_ACTIVE(); /* Activate the chip select  */
#if ( MCU_SELECTED == NXP_K24 )
         eRetVal = NV_SPI_PORT_WRITE( pDevice->port, &instr, sizeof ( instr ) ); /* WR the Instr to the chip */
#elif ( MCU_SELECTED == RA6E1 )
         err = NV_SPI_PORT_WRITE( &g_qspi0_ctrl, &instr, sizeof ( instr ), true ); /* WR the Instr to the chip */
#endif
         if (
#if ( MCU_SELECTED == NXP_K24 )
         eSUCCESS == eRetVal
#elif ( MCU_SELECTED == RA6E1 )
         FSP_SUCCESS == err
#endif
         )

         {
            uint8_t mfgId[3]; /* Will contain the manufacturing ID from the device */
            uint8_t index;    /* Used to index through the sDeviceId table to find a match */

            /* Read the Device ID */
#if ( MCU_SELECTED == NXP_K24 )
            eRetVal = NV_SPI_PORT_READ( pDevice->port, &mfgId[0], sizeof(mfgId) ); /* Read the data back */
#elif ( MCU_SELECTED == RA6E1 )
            err = NV_SPI_PORT_READ( &g_qspi0_ctrl, &mfgId[0], sizeof(mfgId), false ); /* Read the data back */
#endif
         if (
#if ( MCU_SELECTED == NXP_K24 )
         eSUCCESS == eRetVal
#elif ( MCU_SELECTED == RA6E1 )
         FSP_SUCCESS == err
#endif
         )
         {
               /* Search for the device ID in the table. */
            for ( index = 0; index < ARRAY_IDX_CNT( sDeviceId ); index++ )
            {
               if (  ( sDeviceId[index].u8MfgId       == mfgId[0] )     &&
                     ( sDeviceId[index].u8DeviceType  == mfgId[1] )     &&
                     ( sDeviceId[index].u8DeviceID    == mfgId[2] ) )
               {
                  pChipId_ = &sDeviceId[index]; /* Set the pointer to the Chip */
                  found = (bool)true;
                  break;
               }
            }
         }
         }
         NV_CS_INACTIVE(); /* Activate the chip select  */
#if ( RTOS_SELECTION == MQX_RTOS )
         NV_SPI_MutexUnlock(pDevice->port);
#elif ( RTOS_SELECTION == FREE_RTOS )
         OS_MUTEX_Unlock( &qspiMutex_ );   // Function will not return if it fails
#endif
      }
   } while ( !found );

#if ( MCU_SELECTED == RA6E1 )
   if(FSP_SUCCESS == err)
   {
      eRetVal = eSUCCESS;
   }
#endif

   return ( eRetVal );
}
#endif  /* NOT BOOTLOADER */
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
#ifndef __BOOTLOADER
static returnStatus_t restore( lAddr lDest, lCnt cnt, PartitionData_t const *pParData,
                               DeviceDriverMem_t const * const * pNextDriver )
{
   /*lint -efunc( 715, restore ) : Parameters are not used.  It is a part of the common API. */
   /*lint -efunc( 818, restore ) : Parameters are not used.  It is a part of the common API. */
   return ( eSUCCESS );
}
#endif  /* NOT BOOTLOADER */
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
#ifndef __BOOTLOADER
static bool timeSlice( PartitionData_t const *pParData, DeviceDriverMem_t const * const * pNextDriver )
{
   return( false );
}
#endif  /* NOT BOOTLOADER */
/***********************************************************************************************************************

   Function Name: isr_busy

   Purpose: Interrupt on busy signal (Used for SST devices in AAI mode)

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
#if ( MCU_SELECTED == NXP_K24 )
static void isr_busy( void )
#elif ( MCU_SELECTED == RA6E1 )
void isr_busy( external_irq_callback_args_t * p_args )
#endif
{
   DVR_EFL_BUSY_IRQ_DI();              /* Disable the ISR */
   if ( eRtosCmdsEn == rtosCmds_ )
   {
#if ( MCU_SELECTED == NXP_K24 )
      OS_SEM_Post( &extFlashSem_ );  /* Post the semaphore */
#elif ( MCU_SELECTED == RA6E1 )
      OS_SEM_Post_fromISR( &extFlashSem_ );  /* Post the semaphore */
#endif
   }
#if !RTOS
   bBusyIsr_ = true;
#endif
}
#endif  /* NOT BOOTLOADER */

#ifndef __BOOTLOADER
#if ( MCU_SELECTED == RA6E1 )
/***********************************************************************************************************************

   Function Name: MisoBusy_isr_init

   Purpose: Initializes the MISO busy IRQ setup

   Arguments: None

   Returns: fsp_err_t

   Reentrant Code: No

 **********************************************************************************************************************/
static fsp_err_t MisoBusy_isr_init( void )
{
   fsp_err_t err = FSP_SUCCESS;

   /* Open external IRQ/ICU module */
   err = R_ICU_ExternalIrqOpen( &miso_busy_ctrl, &miso_busy_cfg );

   return err;
}
#endif   /* MCU_SELECTED == RA6E1 */
#endif  /* NOT BOOTLOADER */

#ifndef __BOOTLOADER
#if ( MCU_SELECTED == NXP_K24 )
/***********************************************************************************************************************

   Function Name: isr_tmr

   Purpose: Interrupt on busy signal (Used for SST devices in AAI mode)

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
static void isr_tmr( void )
{
   EXT_FLASH_TIMER_DIS();
#if USE_POWER_MODE
   bTmrIsrTriggered_ = 1;
#endif
   LPTMR0_CSR &= ~( LPTMR_CSR_TEN_MASK | LPTMR_CSR_TIE_MASK );
}
#elif ( MCU_SELECTED == RA6E1 )
/***********************************************************************************************************************

   Function Name: g_timer0_callback

   Purpose: Timer ISR callback for AGT timer 0

   Arguments:  None

   Returns: None

   Side Effects: N/A

   Reentrant Code: No

 **********************************************************************************************************************/
void g_timer0_callback (timer_callback_args_t * p_args)
{
    if (TIMER_EVENT_CYCLE_END == p_args->event)
    {
        bTmrIsrTriggered_ = 1;
    }
}
#endif
#endif  /* NOT BOOTLOADER */
/* ****************************************************************************************************************** */
/* Unit Test Code */

#ifdef TM_DVR_EXT_FL_UNIT_TEST
#include "partitions_BL.h"
#include "partition_cfg_BL.h"
#ifndef __BOOTLOADER
/*lint -efunc(578,incCountLimit) argument same as enum OK  */
static uint32_t incCountLimit( uint32_t count, uint32_t limit )
{
   if( count < limit )
   {
      count++;
   }
   return count;
}
#endif   /* BOOTLOADER  */
/***********************************************************************************************************************

   Function Name: DVR_EFL_UnitTest

   Purpose: Unit test module for External Flash Device Driver.  This unit test does the following:
      Writes a pattern to the bank
      Reads the bank and verifies the data (address = data)
      Repeats the read as many times as requested in by parameter ReadRepeat

   Arguments: ReadRepeat - number of times to read the bank and verify

   Returns: eSUCCESS if all operations and comparisons succeed; eFAILURE otherwise

   Side Effects: Flash memory will be left in an erased state.

   Reentrant Code: No

 **********************************************************************************************************************/
#ifndef __BOOTLOADER
uint32_t DVR_EFL_UnitTest( uint32_t ReadRepeat )
{
   returnStatus_t          retVal;
   uint32_t                failCount = 0;
   uint8_t                 unitTestBuf[254]; /* Make this the size of a bank, minus the meta data  */
   PartitionData_t const * partitionData;    /* Pointer to partition information   */
   uint8_t                 startPattern;

#if 0 //TODO Melvin: random number generator to be implemented
   startPattern = ( uint8_t )aclara_rand();
#else
   startPattern = 0x00;
#endif
   /* Find the NV test partition - don't care about update rate   */
   retVal = PAR_partitionFptr.parOpen( &partitionData, ePART_NV_TEST, 0xffffffff );
   if ( eSUCCESS == retVal )
   {
      /* Fill the buffer with an pattern  */
      for ( uint32_t i = 0; i < sizeof( unitTestBuf ); i++ )
      {
         unitTestBuf[i] = ( uint8_t )i + startPattern;
      }
      /* Fill the partition with the pattern */
      for( uint32_t i = 0; i < partitionData->lDataSize / sizeof( unitTestBuf ); i++ )
      {
         if ( eSUCCESS != PAR_partitionFptr.parWrite( 0, &unitTestBuf[0],
                                     sizeof( unitTestBuf ), partitionData ) )
         {
            if( failCount < 0xffff )
            {
               failCount = incCountLimit( failCount, 0xffff );
            }
#ifndef __BOOTLOADER
            DBG_printf( "NV Test - Write failed at 0x%x, len = 0x%x", 0, partitionData->lDataSize );
#endif   /* BOOTLOADER  */
         }
      }

      /* Read the partition back, and compare with expected pattern (multiple times)  */
      for ( uint32_t readLoop = ReadRepeat; readLoop; readLoop-- )
      {
         if ( eSUCCESS == retVal )
         {
            for( uint32_t i = 0; i < partitionData->lDataSize / sizeof( unitTestBuf ); i++ )
            {
               (void)memset( unitTestBuf, 0, sizeof(unitTestBuf) );

               if ( eSUCCESS != PAR_partitionFptr.parRead( &unitTestBuf[0], 0, sizeof( unitTestBuf ), partitionData ))
               {
                  DBG_printf( "NV Test - Read failed at 0x%x, len = 0x%x", 0, sizeof( unitTestBuf ) );
               }
               for ( uint32_t j = 0; j < sizeof( unitTestBuf ); j++ )
               {
                  if( unitTestBuf[j] != (uint8_t)( startPattern + j ) )
                  {
                     failCount = incCountLimit( failCount, 0xffff );

                     DBG_printf( "NV Test - Compare failed at 0x%x, is = 0x%02x, s/b 0x%02x\n",
                                partitionData->PhyStartingAddress + j,
                                unitTestBuf[j], (uint8_t)( startPattern + j ) );
                  }
               }
            }
         }
#if 0 /* DG: Dead Code */
         else
         {
            failCount = incCountLimit( failCount, 0xffff );
         }
#endif
      }
   }
   else
   {
      failCount = incCountLimit( failCount, 0xffff );
   }
   return failCount;
}
#endif
#endif  /* NOT BOOTLOADER */
