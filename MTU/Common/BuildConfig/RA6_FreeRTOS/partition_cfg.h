// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************
 *
 * Filename: partition_cfg.h
 *
 * Contents: Contains partition table definitions.  The contents of this file may change with each project.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ KAD Created Apr 29, 2011
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 04/29/2011 - Initial Release
 *
 * @version    0.1
 * #since      2011-04-29
 **********************************************************************************************************************/

#ifndef PARTITION_CFG_H_
#define PARTITION_CFG_H_

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "dvr_extflash.h"
#include "dvr_banked.h"
#include "dvr_cache.h"
#include "dvr_encryption.h"
#include "dvr_sectPreErase.h"
#include "IF_intFlash.h"

// </editor-fold>
/* ****************************************************************************************************************** */
/* partitions_EXTERN DEFINTION */

#if defined partitions_GLOBAL
   #define partitions_EXTERN
#else
   #define partitions_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#if ( DCU == 1 )
#error "This partition cfg does not apply to DCU's"
#endif

#define UNLIMTED_ENDURANCE_UPDATE_RATE ((uint16_t)0)  /* Unlimited writes. */

/* Define Sector sizes for all flash devices. */
#define EXT_FLASH_ERASE_SIZE        ((phAddr)ERASE_SECTOR_SIZE_BYTES)    /* External Flash Erase Size */
#define INT_FLASH_ERASE_SIZE        ((phAddr)ERASE_SECTOR_SIZE_BYTES)    /* Internal Flash Erase Size */

/* Define Number of Banks in each partition that uses the bank driver */
#define PART_NV_APP_BANKED_BANKS    ((uint8_t)16)     /* # of banks used */
#define PART_NV_APP_CACHED_BANKS    ((uint8_t)8)      /* # of banks used by banked drvr, under cached drvr */
#define PART_NV_DTLS_MAJOR_BANKS    ((uint8_t)4)      /* # of banks used by banked drvr, under cached drvr */
#define PART_NV_DTLS_MINOR_BANKS    ((uint8_t)96)     /* # of banks used by banked drvr, under cached drvr */
#define PART_NV_TEST_BANKS          ((uint8_t)(EXT_FLASH_ERASE_SIZE*2/PART_NV_TEST_SIZE)) /* # of banks used by banked drvr, under cached drvr */


/* Define the partition sizes in NV memory */
#define PART_NV_APP_RAW_SIZE        ((lCnt)8192)      /* Application Raw partition size */
#define PART_NV_APP_BANKED_SIZE     ((lCnt)1024)      /* Application Banked partition size */
#define PART_NV_APP_RSVD0_SIZE      ((lCnt)16384)     /* HD Application partition size */
#define PART_NV_APP_CACHED_SIZE     ((lCnt)2048)      /* Application Cached partition size */
#define PART_NV_APP_RAW_SIG_SIZE    ((lCnt)4096)      /* Signature partition size */
#define PART_NV_MAC_PHY_SIZE        ((lCnt)4096)      /* MAC PHY partition */
#define PART_NV_RAW_ENCRYPT_SIZE    ((lCnt)4096)      /* Encrypted partition size */
#define PART_NV_DFW_BITFIELD_SIZE   ((lCnt)4096)      /* Bitfield for DFW packets */
#define PART_NV_HD_SIZE             ((lCnt)49152)     /* Size of HD Part*/
#define PART_NV_RSVD_2_8_SIZE       ((lCnt)PART_NV_HD_SIZE) /*size of Rsve Partitions 2 - 8*/
#define PART_NV_DTLS_MAJOR_BANK_SIZE ((lCnt)2048)     /* DTLS session major information */
#define PART_NV_DTLS_MINOR_BANK_SIZE ((lCnt)128)      /* DTLS session minor information */

#define PART_NV_DFW_PATCH_SIZE      ((lCnt)266240)    /* DFW patch partition size (65 sectors) */
#define PART_NV_DFW_PGM_IMAGE_SIZE  INTERNAL_FLASH_SIZE /* DFW image raw partition size. */
#define PART_NV_DFW_NV_IMAGE_SIZE      ((lCnt)139264)    /* DFW NV Image (build NV Image) partition size. */


#define PART_NV_HIGH_PRTY_ALRM_SIZE ((lCnt)4096 * 10) /* High priority alarm partition size. */
#define PART_NV_LOW_PRTY_ALRM_SIZE  ((lCnt)4096 * 25) /* Low priority alarm partition size. */
#define PART_NV_TEST_SIZE           ((lCnt)256)      /* NV test partition size  */
#define PART_NV_DTLS_MAJOR_SIZE ((lCnt)PART_NV_DTLS_MAJOR_BANK_SIZE * PART_NV_DTLS_MAJOR_BANKS)
#define PART_NV_DTLS_MINOR_SIZE ((lCnt)PART_NV_DTLS_MINOR_BANK_SIZE * PART_NV_DTLS_MINOR_BANKS)
#define PART_NV_MTR_INFO_SIZE    ((lCnt)INT_FLASH_ERASE_SIZE)


/* Define the partition locations in external memory */
#define PART_NV_APP_RAW_OFFSET      ((lCnt)0)
#define PART_NV_APP_BANKED_OFFSET   ((lCnt)(PART_NV_APP_RAW_OFFSET      + PART_NV_APP_RAW_SIZE))
#define PART_NV_APP_RSVD0_OFFSET    ((lCnt)(PART_NV_APP_BANKED_OFFSET   + (PART_NV_APP_BANKED_SIZE * PART_NV_APP_BANKED_BANKS)))
#define PART_NV_APP_CACHED_OFFSET   ((lCnt)(PART_NV_APP_RSVD0_OFFSET       + PART_NV_APP_RSVD0_SIZE))
#define PART_NV_APP_RAW_SIG_OFFSET  ((lCnt)(PART_NV_APP_CACHED_OFFSET   + (PART_NV_APP_CACHED_SIZE * PART_NV_APP_CACHED_BANKS)))
#define PART_NV_MAC_PHY_OFFSET      ((lCnt)(PART_NV_APP_RAW_SIG_OFFSET  + PART_NV_APP_RAW_SIG_SIZE))
#define PART_NV_RAW_DFW_BTF_OFFSET  ((lCnt)(PART_NV_MAC_PHY_OFFSET      + PART_NV_MAC_PHY_SIZE))
#define PART_NV_RAW_ENCRYPT_OFFSET  ((lCnt)(PART_NV_RAW_DFW_BTF_OFFSET  + PART_NV_DFW_BITFIELD_SIZE))


#define PART_NV_DFW_NV_IMAGE_OFFSET   ((lCnt)(PART_NV_RAW_ENCRYPT_OFFSET  + PART_NV_RAW_ENCRYPT_SIZE))


#define PART_NV_ID_HD_OFFSET          ((lCnt)(PART_NV_DFW_NV_IMAGE_OFFSET         + PART_NV_DFW_NV_IMAGE_SIZE))
#define PART_NV_ID_RSVD2_OFFSET  ((lCnt)(PART_NV_ID_HD_OFFSET                + PART_NV_RSVD_2_8_SIZE))
#define PART_NV_ID_RSVD3_OFFSET  ((lCnt)(PART_NV_ID_RSVD2_OFFSET        + PART_NV_RSVD_2_8_SIZE))
#define PART_NV_ID_RSVD4_OFFSET  ((lCnt)(PART_NV_ID_RSVD3_OFFSET        + PART_NV_RSVD_2_8_SIZE))
#define PART_NV_ID_RSVD5_OFFSET  ((lCnt)(PART_NV_ID_RSVD4_OFFSET        + PART_NV_RSVD_2_8_SIZE))
#define PART_NV_ID_RSVD6_OFFSET  ((lCnt)(PART_NV_ID_RSVD5_OFFSET        + PART_NV_RSVD_2_8_SIZE))

#define PART_NV_ID_RSVD7_OFFSET       ((lCnt)(PART_NV_ID_RSVD6_OFFSET       + PART_NV_RSVD_2_8_SIZE))
#define PART_NV_ID_RSVD8_OFFSET       ((lCnt)(PART_NV_ID_RSVD7_OFFSET       + PART_NV_RSVD_2_8_SIZE))
#define PART_NV_DTLS_MAJOR_OFFSET          ((lCnt)(PART_NV_ID_RSVD8_OFFSET       + PART_NV_RSVD_2_8_SIZE))
#define PART_NV_DTLS_MINOR_OFFSET   ((lCnt)(PART_NV_DTLS_MAJOR_OFFSET   + PART_NV_DTLS_MAJOR_SIZE))

#define PART_NV_HIGH_PRTY_ALRM_OFFSET ((lCnt)(PART_NV_DTLS_MINOR_OFFSET + PART_NV_DTLS_MINOR_SIZE))
#define PART_NV_LOW_PRTY_ALRM_OFFSET  ((lCnt)(PART_NV_HIGH_PRTY_ALRM_OFFSET + PART_NV_HIGH_PRTY_ALRM_SIZE))
#define PART_NV_MTR_INFO_OFFSET     ((lCnt)(PART_NV_LOW_PRTY_ALRM_OFFSET   + PART_NV_LOW_PRTY_ALRM_SIZE))

/* Keep this define last and up to date to properly compute NV usage */
#define PART_NV_TEST_OFFSET         ((lCnt)(PART_NV_MTR_INFO_OFFSET  + PART_NV_MTR_INFO_SIZE))
#define PART_NV_DFW_PATCH_OFFSET    ((lCnt)(PART_NV_TEST_OFFSET      + (PART_NV_TEST_SIZE * PART_NV_TEST_BANKS)))
#define PART_NV_DFW_PGM_IMAGE_OFFSET ((lCnt)(PART_NV_DFW_PATCH_OFFSET     + PART_NV_DFW_PATCH_SIZE))
#define PART_NV_LAST_OFFSET          ((lCnt)(PART_NV_DFW_PGM_IMAGE_OFFSET + PART_NV_DFW_PGM_IMAGE_SIZE - 1))



/* Define the partition sizes in PGM memory */
#define INTERNAL_FLASH_SIZE         ((lCnt)0x00100000)   /* Internal Flash Size */
/* If we do not move the Encrypt Key partition and keep the swap indicator sector */
/* Sections in low bank */
#define PART_BL_CODE_SIZE           BL_CODE_SIZE      /* Bootloader Size from linker script */
#define PART_APP_CODE_SIZE          APP_CODE_SIZE     /* Lower App Code Size from linker script */
#define PART_ENCRYPT_KEY_SIZE       ENCRYPT_KEY_SIZE  /* Encryption Keys Size from linker script */
#define PART_SWAP_STATE_SIZE        SWAP_STATE_SIZE   /* Flash Memory Swap State Size from linker script !ONLY use when swap is or has been enabled! */
/* Sections in upper bank */
#define PART_APP_UPPER_CODE_SIZE    APP_UPPER_CODE_SIZE  /* Upper App Code Size from linker script */
#define PART_BL_BACKUP_SIZE         BL_BACKUP_SIZE       /* Bootloader Backup Size from linker script */
#define PART_DFW_BL_INFO_SIZE       DFW_BL_INFO_SIZE     /* DFW Bootloader Info Size from linker script */

#if 0
/* If we do move the Encrypt Key partition and remove the swap indicator sector only need these definitions */
   /* Sections in low bank */
   #define PART_BL_CODE_SIZE           BL_CODE_SIZE      /* Bootloader Size from linker script */
   #define PART_APP_CODE_SIZE          APP_CODE_SIZE     /* App Code Size from linker script */
   /* Sections in upper bank */
   #define PART_BL_BACKUP_SIZE         BL_BACKUP_SIZE    /* Bootloader Backup Size from linker script */
   #define PART_ENCRYPT_KEY_SIZE       ENCRYPT_KEY_SIZE  /* Encryption Keys Size from linker script */
   #define PART_DFW_BL_INFO_SIZE       DFW_BL_INFO_SIZE  /* DFW Bootloader Info Size from linker script */
#endif


/* Define the partition locations in internal flash memory */


/*****************************************************************************************
If we do not move the Encrypt Key partition and keep the swap indicator sector */
#define PART_BL_CODE_OFFSET         BL_CODE_START
#define PART_APP_CODE_OFFSET        APP_CODE_START
#define PART_ENCRYPT_KEY_OFFSET     ENCRYPT_KEY_START
#define PART_SWAP_STATE_OFFSET      SWAP_STATE_START
#define PART_APP_UPPER_CODE_OFFSET  APP_UPPER_CODE_START
#define PART_BL_BACKUP_OFFSET       BL_BACKUP_START
#define PART_DFW_BL_INFO_OFFSET     DFW_BL_INFO_START
/* MUST keep this reserved. If swap ever used, cannot write until this offset */
#define PART_DFW_BL_INFO_DATA_OFFSET  ((lCnt)16)
/*****************************************************************************************/

/*****************************************************************************************
If we do move the Encrypt Key partition and remove the swap indicator sector:
   #define PART_BL_CODE_OFFSET         ((flAddr)__LNKR_BL_CODE_START)
   #define PART_APP_CODE_OFFSET        ((flAddr)__LNKR_APP_CODE_START)
   #define PART_ENCRYPT_KEY_OFFSET     ((flAddr)__LNKR_ENCRYPT_KEY_START)
   #define PART_BL_BACKUP_OFFSET       ((flAddr)__LNKR_BL_BACKUP_START)
   #define PART_DFW_BL_INFO_OFFSET     ((flAddr)__LNKR_DFW_BL_INFO_START)
*****************************************************************************************/


// The Flash Image is in the non-Active internal Flash Bank when no Bootloader and in external NV with the Bootloader
   //Used when DFW patches in external Flash, then bootloader write internal flash
   #define DFW_PGM_IMAGE_OFFSET        PART_NV_DFW_PGM_IMAGE_OFFSET
   #define DFW_PGM_IMAGE_SIZE          PART_NV_DFW_PGM_IMAGE_SIZE
   #define DFW_PGM_IMAGE_DRVR          pRawDriver
   #define DFW_PGM_IMAGE_DRVR_CFG      ((void *) &sSpiFlashDevice)
   #define DFW_PGM_IMAGE_BUS           _sExtNvBus
   #define DFW_PGM_IMAGE_TYPE          _sExtNvType
   #define DFW_PGM_IMAGE_ERASE_SZ      EXT_FLASH_ERASE_SIZE

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* Global VARIABLES */

/***************************************************************************
  Symbols for BL and App internal Flash location coordination
 **************************************************************************/
#define INTERNAL_FLASH_SECTOR_SIZE   ((lCnt)0x00001000) /* 4096 byte sectors */

#define BL_VECTOR_TABLE_START   ((flAddr)0x00000000)
#define BL_VECTOR_TABLE_SIZE    ((lCnt)0x00000400)

#define BL_CODE_START           BL_VECTOR_TABLE_START
#define BL_CODE_SIZE            ((lCnt)0x00004000)

#define APP_VECTOR_TABLE_START  (BL_CODE_START + BL_CODE_SIZE)
#define APP_VECTOR_TABLE_END    ((flAddr)0x000003FF)
#define APP_VECTOR_TABLE_SIZE   ((lCnt)0x00000400)

#ifndef __BOOTLOADER
#define APP_CODE_START          APP_VECTOR_TABLE_START
#define APP_CODE_SIZE           (ENCRYPT_KEY_START - APP_CODE_START)       /* 0x0007A000 */
#else
/* Give Bootloader full access except for DFW_BL_INFO partition */
#define APP_CODE_START          ((flAddr)APP_VECTOR_TABLE_START)
#define APP_CODE_SIZE           (DFW_BL_INFO_START - APP_CODE_START)
#endif

/* This is an absolute/fixed address - next to last sector in low 512K bank */
#define ENCRYPT_KEY_START       ((flAddr)0x0007E000)  /* This is an absolute/fixed address */
#define ENCRYPT_KEY_SIZE        INTERNAL_FLASH_SECTOR_SIZE

/* This is an absolute/fixed address - last sector in low 512K bank.
   PRESERVE for units that used flash swap mechanism! (V1.40.138 and older) */
#define SWAP_STATE_START        ((flAddr)0x0007F000)
#define SWAP_STATE_SIZE         INTERNAL_FLASH_SECTOR_SIZE

/* This is an absolute/fixed address - first sector in high 512K bank */
#define APP_UPPER_CODE_START    ((flAddr)0x00080000)
#define APP_UPPER_CODE_SIZE     (BL_BACKUP_START - APP_UPPER_CODE_START)   /* 0x00077000 */

/* Starts BL_CODE_SIZE prior to DFW_BL Info */
#define BL_BACKUP_START         (DFW_BL_INFO_START - BL_CODE_SIZE)         /* 0x000FB000 */
#define BL_BACKUP_SIZE          BL_CODE_SIZE                                      /* MUST be same size as BL */

/* This is an absolute/fixed address - Last Sector in internal Flash (high 512K bank) */
#define DFW_BL_INFO_START       (INTERNAL_FLASH_SIZE - INTERNAL_FLASH_SECTOR_SIZE)   /* 0x000FF000 */
#define DFW_BL_INFO_SIZE        INTERNAL_FLASH_SECTOR_SIZE
/***************************************************************************
  End of symbols for BL and App internal Flash location coordination
 **************************************************************************/

/* ****************************************************************************************************************** */
/* CONSTANTS */

// <editor-fold defaultstate="collapsed" desc="SpiFlashDevice_t const sSpiFlashDevic - SPI configuration">
partitions_EXTERN SpiFlashDevice_t const sSpiFlashDevice  /* SPI configuration */
#ifdef partitions_GLOBAL
=
{
   NV_SPI_PORT_NUM,   /* Spi Port */
   EXT_FLASH_CS       /* Spi CS */
}
#endif
; // </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Partition Descriptions - Text Strings for the partition table. ">

#ifndef __BOOTLOADER
partitions_EXTERN const uint8_t partMacPhyDesc_[]
#ifdef partitions_GLOBAL
   = "MacPhy"
#endif
;
#endif   /* BOOTLOADER  */

partitions_EXTERN const uint8_t _sExtNvBus[]
#ifdef partitions_GLOBAL
   = "SPI Port 0"
#endif
;

partitions_EXTERN const uint8_t _sIntFlash[]
#ifdef partitions_GLOBAL
   = "K24 Flash"
#endif
;

partitions_EXTERN const uint8_t _sExtNvType[]
#ifdef partitions_GLOBAL
   = "Ext Flash, 2MB"
#endif
;

partitions_EXTERN const uint8_t _sIntFlashType[]
#ifdef partitions_GLOBAL
   = "Int Flash, 1MB"
#endif
;

partitions_EXTERN const uint8_t partAppRawDesc_[]
#ifdef partitions_GLOBAL
   = "RAW"
#endif
;

#ifndef __BOOTLOADER
partitions_EXTERN const uint8_t partAppBankedDesc_[]
#ifdef partitions_GLOBAL
   = "Banked"
#endif
;

partitions_EXTERN const uint8_t partRSVD0Desc_[]
#ifdef partitions_GLOBAL
   = "Reserved Part 0"
#endif
;

partitions_EXTERN const uint8_t partAppCachedBankedDesc_[]
#ifdef partitions_GLOBAL
   = "Cached"
#endif
;

partitions_EXTERN const uint8_t partAppRawSignatureDesc_[]
#ifdef partitions_GLOBAL
   = "Sig"
#endif
;

partitions_EXTERN const uint8_t _sPartHDDesc_[]
#ifdef partitions_GLOBAL
   = "Daily or HD"
#endif
;

partitions_EXTERN const uint8_t _sPartRSVD2Desc_[]
#ifdef partitions_GLOBAL
   = "Reserved Part 2"
#endif
;

partitions_EXTERN const uint8_t _sPartRSVD3Desc_[]
#ifdef partitions_GLOBAL
   = "Reserved Part 3"
#endif
;

partitions_EXTERN const uint8_t _sPartRSVD4Desc_[]
#ifdef partitions_GLOBAL
   = "Reserved Part 4"
#endif
;

partitions_EXTERN const uint8_t _sPartRSVD5Desc_[]
#ifdef partitions_GLOBAL
   = "Reserved Part 5"
#endif
;

partitions_EXTERN const uint8_t _sPartRSVD6Desc_[]
#ifdef partitions_GLOBAL
   = "Reserved Part 6"
#endif
;


partitions_EXTERN const uint8_t _sPartRSVD7Desc_[]
#ifdef partitions_GLOBAL
= "Reserved Part 7"
#endif
;

partitions_EXTERN const uint8_t _sPartRSVD8Desc_[]
#ifdef partitions_GLOBAL
= "Reserved Part 8"
#endif
;


partitions_EXTERN const uint8_t DTLS_major_[]
#ifdef partitions_GLOBAL
   = "DTLS large data"
#endif
;

partitions_EXTERN const uint8_t DTLS_minor_[]
#ifdef partitions_GLOBAL
   = "DTLS small data"
#endif
;

partitions_EXTERN const uint8_t partHighPrty_[]
#ifdef partitions_GLOBAL
   = "High Prty Alrms"
#endif
;

partitions_EXTERN const uint8_t partLowPrty_[]
#ifdef partitions_GLOBAL
   = "Low Prty Alrms"
#endif
;

partitions_EXTERN const uint8_t partPatchNv_[]
#ifdef partitions_GLOBAL
   = "Patch NV"
#endif
;

partitions_EXTERN const uint8_t partDlPatch_[]
#ifdef partitions_GLOBAL
   = "DL Patch"
#endif
;
#endif   /* BOOTLOADER  */

partitions_EXTERN const uint8_t partDfwImage_[]
#ifdef partitions_GLOBAL
   = "NV Image"
#endif
;

#ifndef __BOOTLOADER
partitions_EXTERN const uint8_t partNVtest_[]
#ifdef partitions_GLOBAL
   = "NV Test"
#endif
;
#endif   /* BOOTLOADER  */

partitions_EXTERN const uint8_t partAppWholeNvDesc_[]
#ifdef partitions_GLOBAL
   = "Whole NV"
#endif
;

#ifndef __BOOTLOADER
partitions_EXTERN const uint8_t partAppEncryptDesc_[]
#ifdef partitions_GLOBAL
   = "Encrypted Raw"
#endif
;

partitions_EXTERN const uint8_t _sPartDfwBtfDesc[]
#ifdef partitions_GLOBAL
   = "DFW Bitfield Raw"
#endif
;
#endif   /* BOOTLOADER  */

partitions_EXTERN const uint8_t partCodeDesc_[]
#ifdef partitions_GLOBAL
   = "Code Space"
#endif
;
partitions_EXTERN const uint8_t partBLbackup_[]
#ifdef partitions_GLOBAL
   = "Backup Bootloader Space"
#endif
;

#ifndef __BOOTLOADER
partitions_EXTERN const uint8_t partAesKeysDesc_[]
#ifdef partitions_GLOBAL
   = "PGM AES Key Raw"
#endif
;
#endif   /* BOOTLOADER  */

partitions_EXTERN const uint8_t partImageDesc_[]
#ifdef partitions_GLOBAL
   = "DFW PGM Image"
#endif
;

#ifndef __BOOTLOADER
partitions_EXTERN const uint8_t partRadioPatchDesc_[]
#ifdef partitions_GLOBAL
   = "Radio Patch"
#endif
;

partitions_EXTERN const uint8_t partMrtInfo_[]
#ifdef partitions_GLOBAL
   = "MTR Info"
#endif
;
#endif   /* BOOTLOADER  */
// </editor-fold>

#ifndef TM_DVR_PARTITION_UNIT_TEST

#ifdef partitions_GLOBAL

#ifndef __BOOTLOADER
PartitionMetaData_t sBanked_MetaData;
PartitionMetaData_t sCached_Banked_MetaData;
PartitionMetaData_t sBanked_MetaData_DTLS_major;
PartitionMetaData_t sBanked_MetaData_DTLS_minor;
PartitionMetaData_t sBanked_NVtestMetaData;
#endif   /* BOOTLOADER  */

/* This MUST be defined as persistent RAM! */
#ifndef __BOOTLOADER
#ifdef TM_CACHE_UNIT_TEST
PERSISTENT uint8_t persistCache_[PART_NV_APP_CACHED_SIZE];
#else
PERSISTENT STATIC uint8_t persistCache_[PART_NV_APP_CACHED_SIZE];
#endif
#endif

#else // For testing allow other applications see this data

#ifdef TM_BANKED_UNIT_TEST
extern PartitionMetaData_t sBanked_MetaData;
#endif

#ifdef TM_CACHE_UNIT_TEST
extern PartitionMetaData_t sCached_Banked_MetaData;
extern PERSISTENT uint8_t persistCache_[PART_NV_APP_CACHED_SIZE];
#endif

#endif      // #ifdef partitions_GLOBAL

// <editor-fold defaultstate="collapsed" desc="const DeviceDriverMem_t *pCacheDriver[]">
/* Contains the list of drivers for the cached driver */
#ifndef __BOOTLOADER
partitions_EXTERN const DeviceDriverMem_t *pCacheDriver[]
#ifdef partitions_GLOBAL
=
{
   &sDeviceDriver_Cache, /* Points to the cache driver. */
   &sDeviceDriver_eBanked, /* Under the cache driver is a banked driver. */
   &sDeviceDriver_eFlash, /* Lowest driver layer, the flash driver.  (yes, there is a spi driver under that) */
   (DeviceDriverMem_t *) NULL
}
#endif
; // </editor-fold>
#endif   /* BOOTLOADER  */

// <editor-fold defaultstate="collapsed" desc="const DeviceDriverMem_t *pCacheRawDriver[]">
/* Contains the list of drivers for the cached driver */
#ifndef __BOOTLOADER
partitions_EXTERN const DeviceDriverMem_t *pCacheRawDriver[]
#ifdef partitions_GLOBAL
=
{
   &sDeviceDriver_Cache, /* Points to the cache driver. */
   &sDeviceDriver_eFlash, /* Lowest driver layer, the flash driver.  (yes, there is a spi driver under that) */
   (DeviceDriverMem_t *) NULL
}
#endif
; // </editor-fold>
#endif   /* BOOTLOADER  */

// <editor-fold defaultstate="collapsed" desc="const DeviceDriverMem_t *pBankedDriver[]">
/* Contains the list of drivers for the cached driver */
#ifndef __BOOTLOADER
partitions_EXTERN const DeviceDriverMem_t *pBankedDriver[]
#ifdef partitions_GLOBAL
=
{
   &sDeviceDriver_eBanked,
   &sDeviceDriver_eFlash,
   (DeviceDriverMem_t *) NULL
}
#endif
; // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="const DeviceDriverMem_t *pSpeDriver[]">
/* Contains the list of drivers for the Sect Pre-Erase driver */
partitions_EXTERN const DeviceDriverMem_t *pSpeDriver[]
#ifdef partitions_GLOBAL
=
{
   &sDeviceDriver_eSpe,
   &sDeviceDriver_eFlash,
   (DeviceDriverMem_t *) NULL
}
#endif
; // </editor-fold>
#endif   /* BOOTLOADER  */

// <editor-fold defaultstate="collapsed" desc="const DeviceDriverMem_t *pIntFlashDriver[]">
/* Contains the list of drivers for the internal flash driver */
partitions_EXTERN const DeviceDriverMem_t *pIntFlashDriver[]
#ifdef partitions_GLOBAL
=
{
   &IF_deviceDriver,
   (DeviceDriverMem_t *) NULL
}
#endif
; // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="const DeviceDriverMem_t *pRawDriver[]">
#ifndef TM_DVR_PARTITION_UNIT_TEST
partitions_EXTERN const DeviceDriverMem_t *pRawDriver[]
#ifdef partitions_GLOBAL
=
{
   &sDeviceDriver_eFlash,
   (DeviceDriverMem_t *) NULL
}
#endif
;
#endif
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="const DeviceDriverMem_t *pRawEncryptDriver[]">
#if 0
#ifndef TM_DVR_PARTITION_UNIT_TEST
partitions_EXTERN const DeviceDriverMem_t *pRawEncryptDriver[]
#ifdef partitions_GLOBAL
=
{
   &sDeviceDriver_Encryption,
   &sDeviceDriver_eFlash,
   (DeviceDriverMem_t *) NULL
}
#endif
;
#endif
#endif
// </editor-fold>

#endif

#ifndef TM_DVR_PARTITION_UNIT_TEST

#ifdef partitions_GLOBAL
partitions_EXTERN const PartitionData_t sPartitionData[]
=
{
#ifndef __BOOTLOADER
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_APP_RAW - Application Data in file format">
   {
      ePART_NV_APP_RAW,                      /* Partition Name */
      PART_NV_APP_RAW_OFFSET,                /* Start Offset */
      PART_NV_APP_RAW_SIZE,                  /* Size of partition */
      PART_NV_APP_RAW_SIZE,                  /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {  /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partAppRawDesc_,                    /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         DFW_MANIP,                          /* Updateable during DFW? */
         FILE_SYS                            /* partition uses the file system */
      },
      {  /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_APP_BANKED - Application Data in file format, Banked">
   {
      ePART_NV_APP_BANKED,                   /* Partition Name */
      PART_NV_APP_BANKED_OFFSET,             /* Start Offset */
      PART_NV_APP_BANKED_SIZE,               /* Size of partition */
      PART_NV_APP_BANKED_SIZE - sizeof(bankMetaData_t), /* Size of partition */
      pBankedDriver,                         /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partAppBankedDesc_,                 /* Describes the partition */
         PAR_BANKED,                         /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         PAR_AUTO_ERASE_BANK,                /* Automatically erase the 'old' bank of memory? */
         DFW_MANIP,                          /* Updateable during DFW? */
         FILE_SYS                            /* partition uses the file system */
      },
      {  /* Attributes: */
         &sBanked_MetaData,                  /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_BANKED_MAX_UPDATE_RATE_SEC,     /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS(PART_NV_APP_BANKED_BANKS)  /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   {
      ePART_NV_RSVD0,                        /* Partition Name */
      PART_NV_APP_RSVD0_OFFSET,                 /* Start Offset */
      PART_NV_APP_RSVD0_SIZE,                   /* Size of partition */
      PART_NV_APP_RSVD0_SIZE,                   /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partRSVD0Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {  /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1)                /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
     // <editor-fold defaultstate="collapsed" desc="ePART_NV_APP_CACHED - Application Data in file format, Cached, Banked">
   {
      ePART_NV_APP_CACHED,                   /* Partition Name */
      PART_NV_APP_CACHED_OFFSET,             /* Start Offset */
      PART_NV_APP_CACHED_SIZE,               /* Size of partition */
      PART_NV_APP_CACHED_SIZE - ( sizeof(bankMetaData_t) + sizeof(cacheMetaData_t) ), /* Size of partition */
      pCacheDriver,                          /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partAppCachedBankedDesc_,           /* Describes the partition */
         PAR_BANKED,                         /* Banked? */
         PAR_CACHED,                         /* Cached? */
         PAR_AUTO_ERASE_BANK,                /* Automatically erase the 'old' bank of memory? */
         DFW_MANIP,                          /* Updateable during DFW? */
         FILE_SYS                            /* partition uses the file system */
      },
      {                                      /* Attributes: */
         &sCached_Banked_MetaData,           /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)&persistCache_,          /* Location of the cached area */
         UNLIMTED_ENDURANCE_UPDATE_RATE,     /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS(PART_NV_APP_CACHED_BANKS)  /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_VIRGIN_SIGNATURE - Contains the NV signature">
   {
      ePART_NV_VIRGIN_SIGNATURE,             /* Partition Name */
      PART_NV_APP_RAW_SIG_OFFSET,            /* Start Offset */
      PART_NV_APP_RAW_SIG_SIZE,              /* Size of partition */
      PART_NV_APP_RAW_SIG_SIZE,              /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partAppRawSignatureDesc_,           /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_MAC_PHY - PHY partition">
   {
      ePART_NV_MAC_PHY,                      /* Partition Name */
      PART_NV_MAC_PHY_OFFSET,                /* Start Offset */
      PART_NV_MAC_PHY_SIZE,                  /* Size of partition */
      PART_NV_MAC_PHY_SIZE,                  /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {  /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partMacPhyDesc_,                    /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {  /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_DFW_BITFIELD - DFW bitfield for missing packets">
   {
      ePART_NV_DFW_BITFIELD,                 /* Partition Name */
      PART_NV_RAW_DFW_BTF_OFFSET,            /* Start Offset */
      PART_NV_DFW_BITFIELD_SIZE,             /* Size of partition */
      PART_NV_DFW_BITFIELD_SIZE,             /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartDfwBtfDesc,                   /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   },// </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ENCRYPT_NV - Application data that is encrypted">
   {
      ePART_ENCRYPT_NV,                      /* Partition Name */
      PART_NV_RAW_ENCRYPT_OFFSET,            /* Start Offset */
      PART_NV_RAW_ENCRYPT_SIZE,              /* Size of partition */
      PART_NV_RAW_ENCRYPT_SIZE,              /* Size of partition */
//TODO: Put back when encryp is functional      pRawEncryptDriver,                     /* Driver access table */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partAppEncryptDesc_,                /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   },// </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_DFW_IMAGE - Area that patches the Logical NV memory">
   {
      ePART_NV_DFW_NV_IMAGE,                 /* Partition Name */
      PART_NV_DFW_NV_IMAGE_OFFSET,           /* Start Offset */
      PART_NV_DFW_NV_IMAGE_SIZE,             /* Size of partition */
      PART_NV_DFW_NV_IMAGE_SIZE,             /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partDfwImage_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   },// </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH1 - Interval Data, Channels 1 - 6">
   {
      ePART_HD,                          /* Partition Name */
      PART_NV_ID_HD_OFFSET,                 /* Start Offset */
      PART_NV_HD_SIZE,                       /* Size of partition */
      PART_NV_HD_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartHDDesc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                          /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH2">
   {
      ePART_NV_RSVD2,                          /* Partition Name */
      PART_NV_ID_RSVD2_OFFSET,                /* Start Offset */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartRSVD2Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH3">
   {
      ePART_NV_RSVD3,                          /* Partition Name */
      PART_NV_ID_RSVD3_OFFSET,                 /* Start Offset */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartRSVD3Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH4">
   {
      ePART_NV_RSVD4,                           /* Partition Name */
      PART_NV_ID_RSVD4_OFFSET,                 /* Start Offset */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartRSVD4Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* This partition can only be opened by name so write frequency has no meaning */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH5">
   {
      ePART_NV_RSVD5,                           /* Partition Name */
      PART_NV_ID_RSVD5_OFFSET,                 /* Start Offset */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartRSVD5Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* This partition can only be opened by name so write frequency has no meaning */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH6">
   {
      ePART_NV_RSVD6,                           /* Partition Name */
      PART_NV_ID_RSVD6_OFFSET,                 /* Start Offset */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartRSVD6Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* This partition can only be opened by name so write frequency has no meaning */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH7">
   {
      ePART_NV_RSVD7,                           /* Partition Name */
      PART_NV_ID_RSVD7_OFFSET,                 /* Start Offset */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartRSVD7Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* This partition can only be opened by name so write frequency has no meaning */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_ID_CH8">
   {
      ePART_NV_RSVD8,                           /* Partition Name */
      PART_NV_ID_RSVD8_OFFSET,                 /* Start Offset */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      PART_NV_RSVD_2_8_SIZE,                       /* Size of partition */
      pSpeDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         _sPartRSVD8Desc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* This partition can only be opened by name so write frequency has no meaning */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_DTLS_MAJOR data">
   {
      ePART_DTLS_MAJOR_DATA,                 /* Partition Name */
      PART_NV_DTLS_MAJOR_OFFSET,             /* Start Offset */
      PART_NV_DTLS_MAJOR_BANK_SIZE,          /* Size of partition */
      PART_NV_DTLS_MAJOR_BANK_SIZE - sizeof(bankMetaData_t),  /* Usable size of partition */
      pBankedDriver,                         /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {  /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         DTLS_major_,                        /* Describes the partition */
         PAR_BANKED,                         /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         DFW_MANIP,                          /* Updateable during DFW? */
         FILE_SYS                            /* partition uses the file system */
      },
      {  /* Attributes: */
         &sBanked_MetaData_DTLS_major,       /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_BANKED_MAX_UPDATE_RATE_SEC,     /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( PART_NV_DTLS_MAJOR_BANKS )    /* Number of banks (Must have at least 1 bank!) */
      }
   },  // <editor-fold defaultstate="collapsed" desc="ePART_NV_DTLS_MAJOR data">
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_DTLS_MINOR data">
   {
      ePART_DTLS_MINOR_DATA,                 /* Partition Name */
      PART_NV_DTLS_MINOR_OFFSET,             /* Start Offset */
      PART_NV_DTLS_MINOR_BANK_SIZE,          /* Size of partition */
      PART_NV_DTLS_MINOR_BANK_SIZE - sizeof(bankMetaData_t),  /* Usable size of partition */
      pBankedDriver,                         /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {  /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         DTLS_minor_,                        /* Describes the partition */
         PAR_BANKED,                         /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         PAR_AUTO_ERASE_BANK,                /* Automatically erase the 'old' bank of memory? */
         DFW_MANIP,                          /* Updateable during DFW? */
         FILE_SYS                            /* partition uses the file system */
      },
      {  /* Attributes: */
         &sBanked_MetaData_DTLS_minor,       /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_BANKED_MAX_UPDATE_RATE_SEC,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( PART_NV_DTLS_MINOR_BANKS )    /* Number of banks (Must have at least 1 bank!) */
      }
   },  // <editor-fold defaultstate="collapsed" desc="ePART_NV_DTLS_MINOR data">
   {
      ePART_NV_HIGH_PRTY_ALRM,               /* Partition Name */
      PART_NV_HIGH_PRTY_ALRM_OFFSET,         /* Start Offset */
      PART_NV_HIGH_PRTY_ALRM_SIZE,           /* Size of partition */
      PART_NV_HIGH_PRTY_ALRM_SIZE,           /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {  /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partHighPrty_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {  /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_LOW_PRTY_ALRM - Low priority alarm data">
   {
      ePART_NV_LOW_PRTY_ALRM,                /* Partition Name */
      PART_NV_LOW_PRTY_ALRM_OFFSET,          /* Start Offset */
      PART_NV_LOW_PRTY_ALRM_SIZE,            /* Size of partition */
      PART_NV_LOW_PRTY_ALRM_SIZE,            /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {  /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partLowPrty_,                       /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {  /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   },// </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_MTR_INFO_DATA - Information read from meter">
   {
      ePART_MTR_INFO_DATA,                   /* Partition Name */
      PART_NV_MTR_INFO_OFFSET,               /* Start Offset */
      PART_NV_MTR_INFO_SIZE,                 /* Size of partition */
      PART_NV_MTR_INFO_SIZE,                 /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partMrtInfo_,                       /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         FILE_SYS                            /* partition uses the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_TEST - NV interface test">
#if 0
   {
      ePART_NV_TEST,                         /* Partition Name */
      PART_NV_TEST_OFFSET,                   /* Start Offset */
      PART_NV_TEST_SIZE,                     /* Size of partition */
      PART_NV_TEST_SIZE,                     /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partNVtest_,                        /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   },
#else
   {
      ePART_NV_TEST,                         /* Partition Name */
      PART_NV_TEST_OFFSET,                   /* Start Offset */
      PART_NV_TEST_SIZE,                     /* Size of partition */
      PART_NV_TEST_SIZE - sizeof(bankMetaData_t), /* Usable size of partition */
      pBankedDriver,                         /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partNVtest_,                        /* Describes the partition */
         PAR_BANKED,                         /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         PAR_AUTO_ERASE_BANK,                /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         &sBanked_NVtestMetaData,            /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_BANKED_MAX_UPDATE_RATE_SEC,     /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( PART_NV_TEST_BANKS )  /* Number of banks (Must have at least 1 bank!) */
      }
   },
#endif
   // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="ePART_NV_DFW_PATCH - Contains downloaded patch">
   {
      ePART_NV_DFW_PATCH,                    /* Partition Name */
      PART_NV_DFW_PATCH_OFFSET,              /* Start Offset */
      PART_NV_DFW_PATCH_SIZE,                /* Size of partition */
      PART_NV_DFW_PATCH_SIZE,                /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partDlPatch_,                       /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         DVR_EFL_MAX_UPDATE_RATE_SECONDS,    /* Maximum write frequency to any one cell */
         PAR_NUM_OF_BANKS( 1 )               /* Number of banks (Must have at least 1 bank!) */
      }
   },
   // </editor-fold>
#endif   /* BOOTLOADER  */
   {  // This partition is in internal Flash for the K22 and T-board or external Flash for K24
      ePART_DFW_PGM_IMAGE,                   /* Partition Name */
      DFW_PGM_IMAGE_OFFSET,                  /* Start Offset */
      DFW_PGM_IMAGE_SIZE,                    /* Size of partition */
      DFW_PGM_IMAGE_SIZE,                    /* Size of partition */
      DFW_PGM_IMAGE_DRVR,                    /* Driver access table */
      DFW_PGM_IMAGE_DRVR_CFG,                /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         DFW_PGM_IMAGE_BUS,                  /* Describes the Bus being used */
         DFW_PGM_IMAGE_TYPE,                 /* Describes the chip being accessed */
         partImageDesc_,                     /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         DFW_PGM_IMAGE_ERASE_SZ,             /* Memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0,                                  /* Number of banks (Must have at least 1 bank!) */
      }
   },
#ifndef __BOOTLOADER
   {
      ePART_WHOLE_NV,                        /* Partition Name */
      0,                                     /* Start Offset */
      EXT_FLASH_SIZE,                        /* Size of partition */
      EXT_FLASH_SIZE,                        /* Size of partition */
      pRawDriver,                            /* Driver access table */
      (void *) &sSpiFlashDevice,             /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sExtNvBus,                         /* Describes the Bus being used */
         _sExtNvType,                        /* Describes the chip being accessed */
         partAppWholeNvDesc_,                /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         EXT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0,                                  /* Number of banks (Must have at least 1 bank!) */
      }
   },// </editor-fold>
   {
      ePART_BL_CODE,                         /* Partition Name */
      PART_BL_CODE_OFFSET,                   /* Start Offset */
      PART_BL_CODE_SIZE,                     /* Size of partition */
      PART_BL_CODE_SIZE,                     /* Size of partition */
      pIntFlashDriver,                       /* Driver access table */
      (void *)NULL,                          /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sIntFlash,                         /* Describes the Bus being used */
         _sIntFlashType,                     /* Describes the chip being accessed */
         partCodeDesc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         INT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   },// </editor-fold>
#endif   // end of #ifndef __BOOTLOADER
   {
      ePART_APP_CODE,                        /* Partition Name */
      PART_APP_CODE_OFFSET,                  /* Start Offset */
      PART_APP_CODE_SIZE,                    /* Size of partition */
      PART_APP_CODE_SIZE,                    /* Size of partition */
      pIntFlashDriver,                       /* Driver access table */
      (void *)NULL,                          /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sIntFlash,                         /* Describes the Bus being used */
         _sIntFlashType,                     /* Describes the chip being accessed */
         partCodeDesc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         INT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   },
#ifndef __BOOTLOADER
   {
      ePART_ENCRYPT_KEY,                     /* Partition Name */
      PART_ENCRYPT_KEY_OFFSET,               /* Start Offset */
      PART_ENCRYPT_KEY_SIZE,                 /* Size of partition */
      PART_ENCRYPT_KEY_SIZE,                 /* Size of partition */
      pIntFlashDriver,                       /* Driver access table */
      (void *)NULL,                          /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sIntFlash,                         /* Describes the Bus being used */
         _sIntFlashType,                     /* Describes the chip being accessed */
         partAesKeysDesc_,                   /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         INT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   },// </editor-fold>
   {
      ePART_SWAP_STATE,                      /* Partition Name */
      PART_SWAP_STATE_OFFSET,                /* Start Offset */
      PART_SWAP_STATE_SIZE,                  /* Size of partition */
      PART_SWAP_STATE_SIZE,                  /* Size of partition */
      pIntFlashDriver,                       /* Driver access table */
      (void *)NULL,                          /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sIntFlash,                         /* Describes the Bus being used */
         _sIntFlashType,                     /* Describes the chip being accessed */
         partAesKeysDesc_,                   /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         INT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   },
    {
      ePART_APP_UPPER_CODE,                  /* Partition Name */
      PART_APP_UPPER_CODE_OFFSET,            /* Start Offset */
      PART_APP_UPPER_CODE_SIZE,              /* Size of partition */
      PART_APP_UPPER_CODE_SIZE,              /* Size of partition */
      pIntFlashDriver,                       /* Driver access table */
      (void *)NULL,                          /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sIntFlash,                         /* Describes the Bus being used */
         _sIntFlashType,                     /* Describes the chip being accessed */
         partCodeDesc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         INT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   },
   {
      ePART_BL_BACKUP,                       /* Partition Name */
      PART_BL_BACKUP_OFFSET,                 /* Start Offset */
      PART_BL_BACKUP_SIZE,                   /* Size of partition */
      PART_BL_BACKUP_SIZE,                   /* Size of partition */
      pIntFlashDriver,                       /* Driver access table */
      (void *)NULL,                          /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sIntFlash,                         /* Describes the Bus being used */
         _sIntFlashType,                     /* Describes the chip being accessed */
         partBLbackup_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         INT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   },
#endif   // End of #ifndef __BOOTLOADER
   {
      ePART_DFW_BL_INFO,                     /* Partition Name */
      PART_DFW_BL_INFO_OFFSET,               /* Start Offset */
      PART_DFW_BL_INFO_SIZE,                 /* Size of partition */
      PART_DFW_BL_INFO_SIZE,                 /* Size of partition */
      pIntFlashDriver,                       /* Driver access table */
      (void *)NULL,                          /* Driver Config (Port, Address, etc...) */
      {                                      /* Partition Description */
         _sIntFlash,                         /* Describes the Bus being used */
         _sIntFlashType,                     /* Describes the chip being accessed */
         partCodeDesc_,                      /* Describes the partition */
         !PAR_BANKED,                        /* Banked? */
         !PAR_CACHED,                        /* Cached? */
         !PAR_AUTO_ERASE_BANK,               /* Automatically erase the 'old' bank of memory? */
         !DFW_MANIP,                         /* Updateable during DFW? */
         !FILE_SYS                           /* partition does NOT use the file system */
      },
      {                                      /* Attributes: */
         (PartitionMetaData_t *)NULL,        /* Location to store meta data */
         INT_FLASH_ERASE_SIZE,               /* External NV memory erase block size */
         (uint8_t *)NULL,                    /* Location of the cached area */
         0,                                  /* Maximum write frequency to any one cell */
         0                                   /* Number of banks (Must have at least 1 bank!) */
      }
   }
};

#endif
#endif

#undef partitions_EXTERN
#endif
