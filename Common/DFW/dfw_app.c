/****************************************************************************************************************************************************************

   Filename:   dfw_app.c

   Global Designator: DFWA_

   Contents: DFW Application - This module will build the image from the original image and a diff. image.

***********************************************************************************************************************
   A product of Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2004-2021 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
***********************************************************************************************************************

   @author Karl Davlin

   $Log$ KAD Created November 25, 2013

***********************************************************************************************************************
   Revision History:
   v0.1 - KAD 11/25/2013 - Initial Release

   @version    0.1
   #since      2013-11-25
***************************************************************************************************************************************************************/


/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdio.h>
#include "project.h"
#define DFWA_GLOBALS
#include "dfw_app.h"    // Include self
#undef  DFWA_GLOBALS

#include "dfw_pckt.h"
#include "partitions.h"
#include "partition_cfg.h"
#include "file_io.h"
#include "DBG_SerialDebug.h"
#include "crc32.h"
#include "byteswap.h"
#include "sys_busy.h"
#include "dfw_interface.h"
#include "pwr_task.h"
#include "ecc108_apps.h"

#include "user_settings.h"  // Added for WolfSSL crypt settings (ECC)  /* TODO: RA6E1: SG Determine if the need for this can be migrated to FreeRTOS headers */
#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/ecc.h"
#include "wolfssl/wolfcrypt/sha.h"
#include "wolfssl/wolfcrypt/sha256.h"

#include "ecc108_lib_return_codes.h"
#include "EVL_event_log.h"
#if ( DCU == 1 )
#include "STAR.h"
#endif

#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
#include "hmc_prg_mtr.h"
#endif

#if ( EP == 1 )
#include "timer_util.h"
#include "rand.h"
#include "dfwtd_config.h"
#endif

#if DFW_XZCOMPRESS_BSPATCHING
#include "dfw_xzmini_interface.h"
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define DFW_SHA_CHUNK_SIZE       (32)              /* SHA chunk of data size */
#define DFW_AEC_CHUNK_SIZE       (32)              /* FW chunk of data size */

#define EXTRACT_LEN(a)           ((lCnt)a[0] << 8 | (lCnt)a[1])   /* Extracts the lCnt Length from x */

#define PWR_CheckPowerDown()  //Todo !

#if ENABLE_PRNT_DFW_INFO
#define DFW_PRNT_INFO(x)         INFO_printf(x)
#define DFW_PRNT_INFO_PARMS      INFO_printf
#else
#define DFW_PRNT_INFO(x)
#define DFW_PRNT_INFO_PARMS(x,...)
#endif

#if ENABLE_PRNT_DFW_WARN
//#define DFW_PRNT_WARN(x,y)       DBG_logPrintf(x,y)
//#define DFW_PRNT_WARN_PARMS      DBG_logPrintf
#else
//#define DFW_PRNT_WARN(x,y)
//#define DFW_PRNT_WARN_PARMS(x,y,...)
#endif

#if ENABLE_PRNT_DFW_ERROR
#define DFW_PRNT_ERROR(x)        ERR_printf(x)
#define DFW_PRNT_ERROR_PARMS     ERR_printf
#else
#define DFW_PRNT_ERROR(x)
#define DFW_PRNT_ERROR_PARMS(x,...)
#endif


/* ------------------------------------------------------------------------------------------------------------------ */
/* Unit Test Macros */

#if DFW_UT_ENABLE
#define DFW_UT_TOGGLE_PIN()
#define DFW_UT_1_ENABLE                         1
#define DFW_UT_2_ENABLE                         1
#define DFW_UT_3_ENABLE                         1
#define DFW_UT_4_ENABLE                         1
#define DFW_UT_5_ENABLE                         1
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
#define DFW_UT_6_ENABLE                         1    // enable power down unit tests for compatiblity test patch state
#define DFW_UT_7_ENABLE                         1    // enable power down unit tests for program script patch state
#define DFW_UT_8_ENABLE                         1    // enable power down unit tests for audit test patch state
#endif //( END_DEVICE_PROGRAMMING_CONFIG == 1 )
#else
#define DFW_UT_1_ENABLE                         0
#define DFW_UT_2_ENABLE                         0
#define DFW_UT_3_ENABLE                         0
#define DFW_UT_4_ENABLE                         0
#define DFW_UT_5_ENABLE                         0
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
#define DFW_UT_6_ENABLE                         0
#define DFW_UT_7_ENABLE                         0
#define DFW_UT_8_ENABLE                         0
#endif //( END_DEVICE_PROGRAMMING_CONFIG == 1 )
#endif

#if     DFW_UT_1_ENABLE == 0
#define dfwUnitTestToggleOutput_TST1(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST1(x, y, z)   dfwUnitTestToggleOutput(1, x, y, z)
#endif

#if     DFW_UT_2_ENABLE == 0
#define dfwUnitTestToggleOutput_TST2(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST2(x, y, z)   dfwUnitTestToggleOutput(2, x, y, z)
#endif

#if     DFW_UT_3_ENABLE == 0
#define dfwUnitTestToggleOutput_TST3(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST3(x, y, z)   dfwUnitTestToggleOutput(3, x, y, z)
#endif

#if     DFW_UT_4_ENABLE == 0
#define dfwUnitTestToggleOutput_TST4(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST4(x, y, z)   dfwUnitTestToggleOutput(4, x, y, z)
#endif

#if     DFW_UT_5_ENABLE == 0
#define dfwUnitTestToggleOutput_TST5(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST5(x, y, z)   dfwUnitTestToggleOutput(5, x, y, z)
#endif

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
#if     DFW_UT_6_ENABLE == 0
#define dfwUnitTestToggleOutput_TST6(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST6(x, y, z)   dfwUnitTestToggleOutput(6, x, y, z)
#endif

#if     DFW_UT_7_ENABLE == 0
#define dfwUnitTestToggleOutput_TST7(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST7(x, y, z)   dfwUnitTestToggleOutput(7, x, y, z)
#endif

#if     DFW_UT_8_ENABLE == 0
#define dfwUnitTestToggleOutput_TST8(x, y, z)
#else
#define dfwUnitTestToggleOutput_TST8(x, y, z)   dfwUnitTestToggleOutput(8, x, y, z)
#endif
#endif //( END_DEVICE_PROGRAMMING_CONFIG == 1 )

#define BANKED_UPDATE_RATE (SECONDS_PER_DAY/6)
#if ( EP == 1 )
#define UPDATE_FREQUENTLY (0)     // update rate for dfw counters in cache
#endif

#if( RTOS_SELECTION == FREE_RTOS )
#define DFWA_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define DFWA_DBG_NUM_MSGQ_ITEMS 0
#endif


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// Patch header
PACK_BEGIN
typedef struct  PACK_MID
{
   int32_t  patchLength;            // TODO: verify:This should be unsigned but it is signed to make a test in do_patch more robust
   uint32_t patchCRC32;             // CRC32 over the patch
   uint32_t newCRC32;               // CRC32 over the patched code
   uint8_t  patchFormat;            // Patch format version
   uint8_t  deviceType[COM_DEVICE_TYPE_LEN]; // Device Type string (must be NULL padded)
   uint8_t  rceFwVersion[4];        // Shall match RCE firmware version
} patchHeader_t;
PACK_END
/*lint +e129 */

typedef enum
{
   eDFWA_CMD_RESET_PROCESSOR = ( ( uint8_t )0 ), // Command to reset the processor
   eDFWA_CMD_WRITE_DATA,                         // Write data to PGM memory
   eDFWA_CMD_FILL_REGION,                        // Fill PGM region
   eDFWA_CMD_COPY_DATA,                          // Copy PGM data
   eDFWA_CMD_WRITE_DATA_NV,                      // Write data to NV
   eDFWA_CMD_FILL_REGION_NV,                     // Fill NV region
   eDFWA_CMD_COPY_DATA_NV,                       // Copy NV data to NV data
   eDFWA_CMD_PATCH_DECOMPRESS_TECHNOLOGY         // Decompress and patch technology
} eMacroCmd_t;                                   // Macro commands in the patch

typedef enum
{
   eDFW_ACLARA_PATCH = ( ( uint8_t )0 ),  // Acalara patch
   eDFW_MINIBSDIFF_PATCH                  // Minibsdiff patch
} ePatchTechnology;

typedef enum
{
   eDFW_NO_COMPRESSION = ( ( uint8_t )0 ),  // Acalara patch
   eDFW_XZ_COMPRESSION                      // Minibsdiff patch
} eCompressionTechnology;

typedef enum
{
   eCPY_NV_TO_IMAGE = ( ( uint8_t )0 ), // Copy NV data to image partition
   eCPY_IMAGE_TO_NV                 // Copy data NV from image partition back to its original partition(s)
} eCopyDir_t;                       // Copy Direction for NV Data Partitions and NV Image Partition

typedef enum
{
   eVALIDATE_PATCH_CRC_ONLY = ( uint8_t )0,
   eVALIDATE_PATCH_COMPLETELY
} eValidatePatch_t;

//lint -esym(754,command, resetParam, reset)        command and reset may not be referenced.
PACK_BEGIN
typedef struct  PACK_MID
{
   eMacroCmd_t   command: 8;    // Reset command
   uint8_t       resetParam;    // Reset parameter
} macroCmdRst_t;
PACK_END

PACK_BEGIN
typedef struct  PACK_MID
{
   eMacroCmd_t   command: 8;  // Write command
   uint8_t       destAddr[3]; // Destination address
   uint8_t       length[2];   // Length of the payload
} macroCmdWrite_t;
PACK_END

PACK_BEGIN
typedef struct  PACK_MID
{
   eMacroCmd_t   command: 8;  // Fill command
   uint8_t       destAddr[3]; // Destination addresses
   uint8_t       length[2];   // Length of the fill region
   uint8_t       data[2];     // Data used to fill with
} macroCmdFill_t;
PACK_END

PACK_BEGIN
typedef struct  PACK_MID
{
   eMacroCmd_t   command: 8;  // Copy command
   uint8_t       destAddr[3]; // Destination address
   uint8_t       srcAddr[3];  // Source address
   uint8_t       length[2];   // Number of bytes to copy
} macroCmdCopy_t;
PACK_END

PACK_BEGIN
typedef struct  PACK_MID
{
   eMacroCmd_t              command: 8;            // Patch & decompression type command
   ePatchTechnology         patchType: 4;          // Patch type
   eCompressionTechnology   compressionType: 4;    // Compression technology type
   uint8_t                  length[3];             // Length of the compressed data
} macroCmdPatchDecompressionType_t;
PACK_END

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
#else
typedef struct
{
   flAddr start;  //Starting address
   flAddr end;    //Ending address
} PartSection_t;

static const PartSection_t AppPartSections[] =
{
   {PART_APP_CODE_OFFSET,       PART_APP_CODE_OFFSET + PART_APP_CODE_SIZE - 1},
#ifdef PART_APP_UPPER_CODE_OFFSET
   {PART_APP_UPPER_CODE_OFFSET, PART_APP_UPPER_CODE_OFFSET + PART_APP_UPPER_CODE_SIZE - 1}
#endif
};

static const PartSection_t BLPartSections[] =
{
   {PART_BL_CODE_OFFSET, PART_BL_CODE_OFFSET + PART_BL_CODE_SIZE - 1}
};
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
/* Size of newCRC32 and patchFormat fields */
#define HDR_NEW_CRC_AND_FORMAT_SIZE (offsetof(patchHeader_t, deviceType[0]) - offsetof(patchHeader_t, newCRC32))
#define PATCH_CRC_OFFSET            (offsetof(patchHeader_t, patchCRC32))

#define PEM_TYPE_TAG_SEQUENCE       (0x30)            /* PEM file type tag indicating a SEQUENCE */
#define PEM_TYPE_TAG_INTEGER        (0x02)            /* PEM file type tag indicating an INTEGER */

#define PEM_33_BYTES_IN_KEY         (0x21)            /* 33 Bytes in the key */
#define PEM_32_BYTES_IN_KEY         (0x20)            /* 32 Bytes in the key */

#define PEM_EXTRA_BYTE              (0x00)            /* Extra byte to add when MSB bit is set */
#define PEM_MSB_MASK                (0x80)            /* BSB bit to check in KEY for extra byte */
#define PEM_KEY_SIZE                (32)              /* KEY SIZE */
#define PEM_MAX_SIZE                (0x48)            /* Maxixmum size of the PEM File custom built */
#define SIG_PACKET_SIZE             (0x40)            /* Max signature packet size 64 */
#define PEM_KEY_DATA_START          (0x04)            /* Location of the first key */
#define PEM_OVERHEAD                (0x02)            /* Two bytes at the beginning of the PEM */

#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
#define METER_FW_UPDATE_RETRY_COUNT (0x01)            /* Max number of retries allowed to update the Meter Fw when unsuccessful */
#endif

/* The following constant values are needed to accommadate dfw patches from version previous to v1.42, to
   versions v1.42 or higher.  A new "target" parameter in the dfw parameters was in introduced in v1.42 to handle
   other firmware versions such as the bootloader.  These constants are used to determine base upon the from
   firmware version in dfw whether the target parameter is available to be used in the DFW process. If not, the
   default target will become the application.
   NOTE - Once all field TBoard devices have been upgraded to v1.70, these constant values
   will no longer be required in the firmware. */
#if ( DCU == 1 )
#if ( DFW_TEST_KEY == 1 )
#define MUTLI_TARGET_VERSION_LIMIT           ((uint8_t)0xFF)   /* Test Keys enabled, version where multiple targets were introduced to DFW */
#define ENCRYPTION_KEY_CHANGE_VERSION_LIMIT  ((uint8_t)0xFF)   /* Test Keys enabled, version where encryption key structure changed  */
#define ENCRYPTION_KEY_CHANGE_BUILD_LIMIT    ((uint16_t)0x422) /* Test Keys enabled, build for DCU devices where encryption key structure changed */
#else
#define MUTLI_TARGET_VERSION_LIMIT           ((uint8_t)0x01)   /* Version where multiple targets were introduced to DFW  */
#define ENCRYPTION_KEY_CHANGE_VERSION_LIMIT  ((uint8_t)0x01)   /* Version where encryption key structure changed  */
#define ENCRYPTION_KEY_CHANGE_BUILD_LIMIT    ((uint8_t)0x3A)  /* Build for DCU devices where encryption key structure changed */
#endif  // endif for ( DFW_TEST_KEY == 1 )
#define MUTLI_TARGET_REVISION_LIMIT          ((uint8_t)0x2A)   /* Revision where multiple targets were introduced to DFW */
#define ENCRYPTION_KEY_CHANGE_REVISION_LIMIT ((uint8_t)0x32)   /* Revision where encryption key structure changed */
#endif  // endif for ( DCU == 1 )

#if ( EP == 1 )
#if ( DFW_TEST_KEY == 1 )
#define DTLS_MAJOR_MINOR_UPDATE_VERSION_LIMIT  ((uint8_t)0xFF) /* Version where DTLS file structure was smaller */
#else
#define DTLS_MAJOR_MINOR_UPDATE_VERSION_LIMIT  ((uint8_t)0x01) /* Version where DTLS file structure was smaller */
#endif
#define DTLS_MAJOR_MINOR_UPDATE_REVISION_LIMIT ((uint8_t)0x2A) /* Revision where DTLS file structure was smaller */
#endif

/* Currently DFW will be returning events in the /DF/CO resource response to indicate when ep firmware or meter
   configurations have changes via the DFW process.  The following values are compiled out, but could be utilized
   in the future if the event reporting is changed to the bubble up alarm resource.         */
#if 0
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
#define METER_CONFIG_NORMAL_EVENT_PRIORITY            (uint8_t)190
#define METER_CONFIG_CHANGED_EVENT_PRIORITY           (uint8_t)190
#define METER_CONFIG_ERROR_NORMAL_EVENT_PRIORITY      (uint8_t)179
#endif
#define COM_DEV_FW_STATUS_REPLACED_EVENT_PRIORITY     (uint8_t)111
#define COM_DEV_FW_STATUS_WRITE_FAILED_EVENT_PRIORITY (uint8_t)221
#endif

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
#else
#define DFW_BL_FAILCOUNT_DEFAULT    ((uint32_t)0xFFFFFFFF)  /* FailCount sent to BL, not this when send by BL */
#endif
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static bool _Initialized = false;

static PartitionData_t const *pDFWImagePTbl_;                  /* Image partition for PGM memory */
static PartitionData_t const *pDFWPatchPTbl_;                  /* Patch partition */
static PartitionData_t const *pDFWImageNvPTbl_;                /* Image partition for NV memory */
static PartitionData_t const *pDFWPatch;                       /* pointer of patch partition being used
                                                                     -Patch Partion used for NIC FW updates
                                                                     -Image Partion used for ED FW updates/configurations
                                                                     -reuse is to accomdate size of DFW KV2c meter FW
                                                               */
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
#else
static PartitionData_t const *pDFWBLInfoPar_;                  /* DFW_BL Info partition */
static PartitionData_t const *pAppCodePart_;                   /* APP code partition */
#endif
static FileHandle_t           DFWVarsFileHndl_;                /* DFW misc variables partition */
#if ( EP == 1 )
static FileHandle_t           DFWCachedFileHndl_;              /* DFW misc counters. */
static DfwCachedAttr_t        DfwCachedAttr = {0};             /* DFW data saved in cached memory */
#endif
static eDfwErrorCodes_t       eErrorCode_   = eDFWE_NO_ERRORS; /* Contains the error code for this module *//*lint -esym(838,eErrorCode_) previous value not used */
static uint8_t                alarmId_      = NO_ALARM_FOUND;  /* Alarm ID, set by the time calendar alarm */

static HEEP_APPHDR_t downloadCompletelTxInfo;               /* Application header/QOS info */
static HEEP_APPHDR_t patchCompletelTxInfo;                  /* Application header/QOS info */
static byte          derivedKey_[ECC108_KEY_SIZE];          /* Derived key used for AES decryption */

#if ( EP == 1 )
static buffer_t *pDownloadCompleteMessage = NULL;
static buffer_t *pPatchCompleteMessage    = NULL;
static uint16_t  uDownloadCompleteTimerId = 0;               /* Timer ID used by time diversity. */
static uint16_t  uPatchCompleteTimerId    = 0;               /* Timer ID used by time diversity. */
#endif

/* DFW feature support variables */
static const bool endPointDfwSupported_           = true;
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
static const bool bootloaderDfwSupported_         = false;
#else
static const bool bootloaderDfwSupported_         = true;
#endif

/*detemrmine the Possible patch types for End Device Firmware */
#if ( (EP == 1) && ( END_DEVICE_PROGRAMMING_FLASH == ED_PROG_FLASH_PATCH_ONLY ) )
static const bool meterPatchDfwSupported_         = true;
static const bool meterBasecodeDfwSupported_      = false;
#elif ( (EP == 1) && ( END_DEVICE_PROGRAMMING_FLASH == ED_PROG_FLASH_BASECODE_ONLY ) )
static const bool meterPatchDfwSupported_         = false;
static const bool meterBasecodeDfwSupported_      = true;
#elif ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH ==  ED_PROG_FLASH_SUPPORT_ALL ) )
static const bool meterBasecodeDfwSupported_      = true;
static const bool meterPatchDfwSupported_         = true;
#else
/*non EP and device which do not support ED programming flash*/
static const bool meterPatchDfwSupported_         = false;
static const bool meterBasecodeDfwSupported_      = false;
#endif

#if ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
static const bool meterReprogrammingDfwSupported_ = true;
#else
static const bool meterReprogrammingDfwSupported_ = false;
#endif

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

static returnStatus_t   doPatch( void );
static eDfwErrorCodes_t validatePatchHeader( eValidatePatch_t valPatchCmd, dl_dfwFileType_t fileType );
static eDfwErrorCodes_t copyNvData( eCopyDir_t Direction );
static dSize            getThreeByteAddr( uint8_t const *pAddr );
static void             readWriteBuffer( dSize DstAddr, dSize SrcAddr, uint8_t *Buffer, lCnt Cnt,
                                         const PartitionData_t *pDstPartition, const PartitionData_t *pSrcPartition );
static returnStatus_t   calcCRC( flAddr StartAddr, uint32_t cnt, bool start, uint32_t * const pResultCRC, PartitionData_t const *pPartition );
static returnStatus_t   verifyCrc( flAddr StartAddr, uint32_t cnt, uint32_t expectedCRC,
                                   PartitionData_t const * pPartition );
static eDfwErrorCodes_t executeMacroCmds( eDfwPatchState_t ePatchState, patchHeader_t const *pPatchHeader );
static eDfwErrorCodes_t getPatchHeader( patchHeader_t *pPatchHeader, const DFW_vars_t * const pDfwVars );
static void             brick( void );
static returnStatus_t   decryptPatch( DFW_vars_t * const pVars, patchHeader_t *pPatchHeader );
static void             createPemFromExtractedKey( const uint8_t *extractedKey, uint8_t *pem, uint8_t *pemSz );
static returnStatus_t   verifySignature( eDfwDecryptState_t decryptStatus, PartitionData_t const *pPartition,
                                         flAddr StartAddr, uint32_t cnt );
static returnStatus_t   decryptData( PartitionData_t const *pPartition, flAddr StartAddr, flAddr DecodedStart,
                                     const uint8_t *authTag, uint32_t authTagSz, uint32_t patchLen );
static bool DFWA_verifyPatchFormat( dl_dfwFileType_t fileType, uint8_t patchFormat );
#if ( EP == 1 )
static bool u16_counter_inc( uint16_t *pCounter, bool bRollover );
#endif
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
#else
static returnStatus_t updateBootloader( uint32_t crcBl );
static returnStatus_t copyPart2Part ( flAddr start, lCnt size,
                                      uint8_t * buffer, uint16_t bufSize,
                                      PartitionData_t const *pSrcPTbl, PartitionData_t const * pDstPTbl );
#endif

static void DFWA_DownloadComplete( DFW_vars_t* pDfwVars );
static void DFWA_PatchComplete( DFW_vars_t* pDfwVars );
static void setDfwToIdle ( DFW_vars_t *pDfwVars );
#if ( EP == 1 )
static void DFWA_DownloadCompleteTimerExpired( uint8_t cmd, void *pData );
static void DFWA_SetDownloadCompleteTimer( uint32_t timeMilliseconds );
static void DFWA_PatchCompleteTimerExpired( uint8_t cmd, void *pData );
static void DFWA_SetPatchCompleteTimer( uint32_t timeMilliseconds );
#endif

#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
static void DFWA_DecrementMeterFwRetryCounter( void );
#endif

#if DFW_UT_ENABLE
static void             dfwUnitTestToggleOutput( uint8_t seq, uint32_t numToggles, uint8_t step, uint8_t totSteps );
#endif
#if  ( DCU == 1 )
static returnStatus_t updateEncryptPartition( void );
#endif
#if ( EP == 1 )
static returnStatus_t updateDtlsMajorMinorFiles( void );
#endif

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION DEFINITIONS */

/* *********************************************************************************************************************

   Function Name: DFWA_init

   Purpose: This function is called before starting the scheduler.  Opens the file(s)/partition(s) needed for DFW.

   Arguments: None

   Returns: eSUCCESS/eFAILURE indication

   Side effects: Creates message queue.

   Reentrant: NO - This function should be called once before starting the scheduler

   Notes: Allowing DFW to decrypt a patch at startup requires excessive stack space for the Startup task.  This
   requirement would only apply under very unlikely situations (power loss immediately following the last packet
   download).
   Once decryption has started there is no recovery until successfully completed, then the application performs
   as if the patch was never encrypted. This implies new packets cannot be accepted since they will be encrypted
   and the patch has already been decrypted.

**********************************************************************************************************************/
returnStatus_t DFWA_init( void )
{
   returnStatus_t eStatus;
   FileStatus_t   fileStatus;
   DFW_vars_t     dfwVars;       // Contains the DFW file variables from the file

   alarmId_ = NO_ALARM_FOUND;    /* Initialize the alarm ID to invalid */

   if ( eSUCCESS == (eStatus = FIO_fopen(&DFWVarsFileHndl_,                /* File Handle so that PHY access the file. */
                                         ePART_SEARCH_BY_TIMING,           /* Search for the best partition according to the timing. */
                                         (uint16_t)eFN_DFW_VARS,           /* File ID (filename) */
                                         (lCnt)sizeof(DFW_vars_t),         /* Size of the data in the file. */
                                         FILE_IS_NOT_CHECKSUMED,           /* File attributes to use. */
                                         BANKED_UPDATE_RATE,               /* The update rate of the data in the file. */
                                         &fileStatus)))                    /* Contains the file status */
   {
      if ( fileStatus.bFileCreated )
      {
         // File was created for the first time, set defaults.
         ( void ) memset( &dfwVars, 0, sizeof( dfwVars ) );

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
         // initialize the dfw file section status parameters to default values
         dfwVars.fileSectionStatus.compatibilityTestStatus = DFW_FILE_SECTION_STATUS_DEFAULT_VALUE;
         dfwVars.fileSectionStatus.programScriptStatus = DFW_FILE_SECTION_STATUS_DEFAULT_VALUE;
         dfwVars.fileSectionStatus.auditTestStatus = DFW_FILE_SECTION_STATUS_DEFAULT_VALUE;
#endif
         eStatus = DFWA_setFileVars( &dfwVars );
      }
      else
      {
         eStatus = DFWA_getFileVars( &dfwVars );              // Read the DFW NV variables (status, etc...)
      }

      if ( eSUCCESS == eStatus )
      {
         // Open DFW PGM Image partition
         if ( eSUCCESS == (eStatus = PAR_partitionFptr.parOpen(&pDFWImagePTbl_, ePART_DFW_PGM_IMAGE, 0L)))
         {
            // Open DFW NV Image partition
         	if (eSUCCESS == (eStatus = PAR_partitionFptr.parOpen(&pDFWImageNvPTbl_, ePART_NV_DFW_NV_IMAGE, 0L)))
            {
               // Open DFW Patch partition
               if (eSUCCESS == (eStatus = PAR_partitionFptr.parOpen(&pDFWPatchPTbl_, ePART_NV_DFW_PATCH, 0L)))
               {
                  //Open DFW BL Info partition
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
#else
                  if ( eSUCCESS == (eStatus = PAR_partitionFptr.parOpen(&pDFWBLInfoPar_, ePART_DFW_BL_INFO, 0L)) &&
                       eSUCCESS == (eStatus = PAR_partitionFptr.parOpen(&pAppCodePart_, ePART_APP_CODE, 0L)) )
#endif
                  {
                     if ((bool)true == OS_MSGQ_Create(&DFWA_MQueueHandle, DFWA_NUM_MSGQ_ITEMS, "DFW"))
                     {
                        /* Semaphore correctly created, initialize the DFW packet module. */
                        eStatus = DFWP_init();
                        if ( eStatus ==  eSUCCESS )
                        {
                           _Initialized = true;
                        }
                     }
                     else
                     {
                        eStatus = eFAILURE;
                     }
                  }
               }
            }
         }
      }
      if ( eSUCCESS == eStatus )
      {
        (void) DFWA_SetPatchParititon ( dfwVars.fileType );
      }
   }

   if ( eSUCCESS == eStatus )
   {
      /*lint --e{644} dfwVars init'd in DFWA_getFileVars above */
      /* In the middle of performing a patch operation, but is not going to decrypt or started decrypting. */
      if ( (eDFWP_IDLE           != dfwVars.ePatchState)  && /* Executing, Waiting, Swapping or Copying NV */
           (eDFWP_WAIT_FOR_APPLY != dfwVars.ePatchState) ) /* Eliminate Waiting */
      {
         /* Decryption not required or decryption completed */
         /* (Checking for IDLE only expected when patching from a FW version that did not have DFW security
            to a FW version that does have DFW security) */
         if( (eDFWD_DONE == dfwVars.decryptState) || (eDFWD_IDLE == dfwVars.decryptState) ||
             (eDFWD_READY == dfwVars.decryptState) )
         {
            /* Yes, in the middle of a patch - Validate the patch has not been corrupted */
            ( void )validatePatchHeader( eVALIDATE_PATCH_CRC_ONLY, dfwVars.fileType ); /* eErrorCode_ is updated. */
            /* Yes, the Patch is valid */
            if ( eDFWE_NO_ERRORS == eErrorCode_ )
            {
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
               if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
               {
                  /* We need to restart the patch process.  HMC is not available yet so we need to send a message to
                     the DFW task to restart the patch process. */

                  if( eSUCCESS == HMC_PRG_MTR_PrepareToProgMeter() )
                  {
                     // hmc has been prepared for meter programming
                     buffer_t *pBuf = BM_alloc( sizeof( dfwFullApplyCmd_t ) ); // allocate buffer to send message to dfw task
                     if( NULL != pBuf )
                     {
                        // we got the buffer, set up message to DFW task based upon current patch state
                        pBuf->eSysFmt = eSYSFMT_DFW;

                        if( eDFWP_COMPATIBILITY_TEST == dfwVars.ePatchState )
                        {
                           // Compatiblity test was interrupted, we need to send a pre patch command
                           eDfwCommand_t *pMsgData = ( eDfwCommand_t * )&pBuf->data[0]; //lint !e826   Pointer to sys msg data
                           *pMsgData = eDFWC_PREPATCH;
                        }
                        else if( eDFWP_PROGRAM_SCRIPT == dfwVars.ePatchState ||
                                 eDFWP_AUDIT_TEST == dfwVars.ePatchState )
                        {
                           /* Program script or audit test was interrupted, we need to send a full apply command.  We
                              also need to send the apply schedule currently stored in the dfw variables */
                           dfwFullApplyCmd_t *pMsgData = ( dfwFullApplyCmd_t * )&pBuf->data[0]; //lint !e740 !e826
                           pMsgData->eCmd = eDFWC_FULL_APPLY;
                           ( void )memcpy( &pMsgData->applyVars.applySchedule, &dfwVars.applySchedule, sizeof( pMsgData->applyVars.applySchedule ) );
                        }

                        OS_MSGQ_Post( &DFWA_MQueueHandle, pBuf );  // Post the message, function will not return on fail
                     }
                     else
                     {
                        // Failed to allocate buffer.  DFW cannot conitue since we cannot send message to DFW task.
                        DFW_PRNT_ERROR( "BM_alloc()" );
                        eErrorCode_ = eDFWE_INVALID_STATE;
                     }
                  }
                  else
                  {
                     // Failed to setup hmc for meter programming.  DFW cannot continue set error code for processing.
                     DFW_PRNT_ERROR( "PrepairForProgramming()" );
                     eErrorCode_ = eDFWE_INVALID_STATE;
                  }

                  if( eDFWE_NO_ERRORS != eErrorCode_ )
                  {
                     // there was an error in the attempt to continue dfw meter config update process after power up
                     if( eDFWP_COMPATIBILITY_TEST == dfwVars.ePatchState )
                     {
                        // Set up dfw task to send a down load complete message with the error
                        dfwVars.eErrorCode        = eErrorCode_;               // power outage occured during decryption
                        dfwVars.eDlStatus         = eDFWS_INITIALIZED_WITH_ERROR;   // Set status (in the file) to error
                        dfwVars.ePatchState       = eDFWP_IDLE;                     // Set state to idle since patching is done
                        dfwVars.applyMode         = eDFWA_DO_NOTHING;               // clear the apply mode
                        dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;              // send confirmation message
                     }
                     else if( eDFWP_PROGRAM_SCRIPT == dfwVars.ePatchState || eDFWP_AUDIT_TEST == dfwVars.ePatchState )
                     {
                        /* The apply cannot be completed. Set dfw back to idle so a new dfw process can be
                           initiated and applied. */
                        setDfwToIdle( &dfwVars ); // apply complete message set when changing dfw to idle
                     }
                     ( void )DFWA_setFileVars( &dfwVars ); // Save the changes
                  }
               }
               else
#endif // ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
               {
                  // If patch state is updating NV or doing BL swap, start the process if the patch is for application or bootloader
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
                  if( eDFWP_EXEC_NV_COMMANDS == dfwVars.ePatchState || eDFWP_COPY_NV_IMAGE_TO_NV == dfwVars.ePatchState ||
                      eDFWP_DO_FLASH_SWAP == dfwVars.ePatchState )
#else
                  if( eDFWP_EXEC_NV_COMMANDS == dfwVars.ePatchState || eDFWP_COPY_NV_IMAGE_TO_NV == dfwVars.ePatchState ||
                      eDFWP_BL_SWAP == dfwVars.ePatchState || eDFWP_DO_FLASH_SWAP == dfwVars.ePatchState )
#endif
                  {
                     ( void )doPatch(); /* Continue patching */
                  }
               }
            }
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH > ED_PROG_FLASH_NOT_SUPPORTED ) ) )
            else
            {
               // we had an error during patch validation
               DFW_PRNT_ERROR( "Patch Header validation failed." );
               if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
               {
                  // current patch if for meter configuration update
                  if ( eDFWP_COMPATIBILITY_TEST == dfwVars.ePatchState )
                  {
                     /* CRC check failed on patch, cannot trust information in patch to continue with DFW.  Set dfw
                        status to idle so a new dfw can be applied if needed. */
                     DFW_PRNT_ERROR( "Patch Header validation failed." );
                     dfwVars.eErrorCode        = eDFWE_CRC32PATCH;               // power outage occured during decryption
                     dfwVars.eDlStatus         = eDFWS_INITIALIZED_WITH_ERROR;   // Set status (in the file) to error
                     dfwVars.ePatchState       = eDFWP_IDLE;                     // Set state to idle since patching is done
                     dfwVars.applyMode         = eDFWA_DO_NOTHING;               // clear the apply mode
                     dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;              // send confirmation message
                  }
                  else if ( eDFWP_PROGRAM_SCRIPT == dfwVars.ePatchState || eDFWP_AUDIT_TEST == dfwVars.ePatchState )
                  {
                     /* CRC check failed on patch, cannot trust information in patch to continue with DFW.  Set dfw
                        status to idle so a new dfw initiated and applied. */
                     setDfwToIdle( &dfwVars );           // apply complete message set when changing dfw to idle
                  }
                  ( void )DFWA_setFileVars( &dfwVars ); // Save the changes
               }
               else
               {
                  // current patch is for application/bootloader
                  if ( eDFWP_EXEC_PGM_COMMANDS != dfwVars.ePatchState ) //Already know not Idle or Wait For Apply...
                  {
                     //Can not reliably continue if beyond making the program image
                     brick();
                  }
               }
            }
#else
            else if ( eDFWP_EXEC_PGM_COMMANDS != dfwVars.ePatchState ) //Already know not Idle or Wait For Apply...
            {
               //Can not reliably continue if beyond making the program image
               brick();
            }
#endif
         }
         /* decryption failed to complete before power outage, set dfwVars to indicate error occured and allow for dfw recovery */
         else if( eDFWD_DECRYPTING == dfwVars.decryptState )
         {
            dfwVars.eErrorCode        = eDFWE_INVALID_STATE;            /* power outage occured during decryption            */
            dfwVars.eDlStatus         = eDFWS_INITIALIZED_WITH_ERROR;   /* Set status (in the file) to error                 */
            dfwVars.ePatchState       = eDFWP_IDLE;                     /* Set state to idle since patching is done.         */
            dfwVars.applyMode         = eDFWA_DO_NOTHING;               /* clear the apply mode                              */
            dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;              /* update dfw variables to send confirmation message */
            ( void )DFWA_setFileVars( &dfwVars );                       /* updated the DFW vars to indicate                  */
         }
      }
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
      /* If we get here after a power outage, we were waiting to apply meter cofigurtion update patch, we are in the
         wait_to_apply state, then we need to prepare HMC to program the meter again.  */
      else if( eDFWP_WAIT_FOR_APPLY == dfwVars.ePatchState &&
               DFW_ACLARA_METER_PROGRAM_FILE_TYPE == dfwVars.fileType )
      {
         // meter configuration update patch and we are waiting to apply the patch
         if( eSUCCESS != HMC_PRG_MTR_PrepareToProgMeter() )
         {
            // if we cant intitialize the HMC for the meter config update patch, set DFW to init with error and IDLE
            DFW_PRNT_ERROR( "PrepairForProgramming()" );
            dfwVars.eErrorCode        = eDFWE_INVALID_STATE;            /* power outage occured during decryption            */
            dfwVars.eDlStatus         = eDFWS_INITIALIZED_WITH_ERROR;   /* Set status (in the file) to error                 */
            dfwVars.ePatchState       = eDFWP_IDLE;                     /* Set state to idle since patching is done.         */
            dfwVars.applyMode         = eDFWA_DO_NOTHING;               /* clear the apply mode                              */
            dfwVars.eSendConfirmation = eDFWC_NONE;                     /* update dfw variables to send confirmation message */
            ( void )DFWA_setFileVars( &dfwVars );                       /* updated the DFW vars to indicate                  */
         }
      }
#endif
   }

#if ( EP == 1 )
   if( eSUCCESS == eStatus )
   {
      // Open DFW cached data (mainly counters)
      eStatus = FIO_fopen( &DFWCachedFileHndl_,              /* File Handle so that PHY access the file. */
                           ePART_SEARCH_BY_TIMING,           /* Search for the best partition according to the timing. */
                           ( uint16_t )eFN_DFW_CACHED,       /* File ID (filename) */
                           ( lCnt )sizeof( DfwCachedAttr_t ), /* Size of the data in the file. */
                           FILE_IS_NOT_CHECKSUMED,           /* File attributes to use. */
                           UPDATE_FREQUENTLY,                /* The update rate of the data in the file. */
                           &fileStatus );                    /* Contains the file status */
      if( eSUCCESS == eStatus )
      {
         if ( fileStatus.bFileCreated )
         {
            //File was created for the first time, set defaults.
            ( void )memset( &DfwCachedAttr, 0, sizeof( DfwCachedAttr ) );
            eStatus = FIO_fwrite( &DFWCachedFileHndl_, 0, ( uint8_t* )&DfwCachedAttr, sizeof( DfwCachedAttr ) );
         }
         else
         {
            eStatus = FIO_fread( &DFWCachedFileHndl_, ( uint8_t* )&DfwCachedAttr, 0, sizeof( DfwCachedAttr ) );
         }
      }
   }
#endif

   return( eStatus );
}
/******************************************************************************************************************** */


/***********************************************************************************************************************

   Function name: DFWA_task

   Purpose: DFW Task will build the image and apply the patch as a low priority task.

   Arguments: uint32_t Arg0

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

******************************************************************************************************************** */
void DFWA_task( taskParameter )
{
   DFW_vars_t  dfwVars; /* Contains all of the DFW file variables */

   if ( _Initialized == false )
   {  // Failed to open partitions. Stall here. Not much we can do.
      for(;;)
      {
         OS_TASK_Sleep( ONE_SEC );
      }
   }
   ( void )DFWA_getFileVars( &dfwVars ); // Read the DFW NV variables (status, etc...)

   // If download completed, but still need to decrypt or if decryption has completed, send message to task to (re)start execution
   if ( ( eDFWP_EXEC_PGM_COMMANDS == dfwVars.ePatchState ) &&
        ( ( eDFWD_READY == dfwVars.decryptState ) || ( eDFWD_DONE == dfwVars.decryptState ) ) )
   {
      buffer_t *pBufDLComplete = BM_alloc( sizeof( eDfwCommand_t ) ); // Get buffer for msg

      if ( NULL != pBufDLComplete ) // Was the buffer allocated?
      {
         eDfwCommand_t  *pMsgData = ( eDfwCommand_t * )&pBufDLComplete->data[0]; //lint !e826  Ptr to msg data

         dfwVars.ePatchState = eDFWP_IDLE;                           // So the execute process can process
         ( void )DFWA_setFileVars( &dfwVars );
         pBufDLComplete->eSysFmt = eSYSFMT_DFW;                      // Set sys msg format
         *pMsgData               = eDFWC_PREPATCH;                   // Set sys msg command
         OS_MSGQ_Post ( &DFWA_MQueueHandle, pBufDLComplete );        // Post it!
         DFW_PRNT_INFO( "Start DFW Execution" );
      }
      else
      {
         DFW_PRNT_ERROR( "BM_alloc()" );
      }
   }

   /* Is DFW scheduled to apply a patch? */
   if ( ( eDFWS_INITIALIZED_COMPLETE == dfwVars.eDlStatus ) && ( eDFWP_WAIT_FOR_APPLY == dfwVars.ePatchState ) &&
         ( eDFWA_FULL_APPLY == dfwVars.applyMode ) )
   {
      /* An apply is scheduled.  The simplest way to re-set the alarm is to simply call the apply command. */
      dfw_apply_t applyCmd;

      applyCmd.streamId      = dfwVars.initVars.streamId;
      applyCmd.applySchedule = dfwVars.applySchedule;
      ( void )DFWI_DownloadApply( &applyCmd );
   }

   // Is there a "pending" message from dfw patching that needs to be sent?
   if ( eDFWC_NONE != dfwVars.eSendConfirmation )
   {
      //wait forever for the App Msg task to be available
      while( true != APP_MSG_HandlerReady() )
      {
         OS_TASK_Sleep( FIFTY_MSEC );
      }

      OS_TASK_Sleep( 10 * ONE_SEC ); //Wait a little bit for other tasks to get running

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
      while( false == HMC_MTRINFO_Ready() )
      {
         //Wait for HMC to be ready after device power up, response message may need to respond with data from the meter
         OS_TASK_Sleep( ONE_SEC );
      }
#endif

      if ( eDFWC_DL_COMPLETE == dfwVars.eSendConfirmation )
      {
         dfwVars.eSendConfirmation = eDFWC_NONE;
         DFWA_DownloadComplete( &dfwVars );
      }
      else if ( eDFWC_APPLY_COMPLETE == dfwVars.eSendConfirmation )
      {
         dfwVars.eSendConfirmation = eDFWC_NONE;
         DFWA_PatchComplete( &dfwVars );
      }
      ( void )DFWA_setFileVars( &dfwVars ); // Save the changes
   }

   for ( ; ; )
   {
      sysTimeCombined_t combSysTime;  /* Contains the combined system time. */
      bool              bSaveVars;    /* Indicator to save the file variables */
      bool              bRunPatch;    /* Indicator to execute the patch. */
      buffer_t         *pBuf;         /* Raw buffer used by the buffer module */

      ( void )OS_MSGQ_Pend( &DFWA_MQueueHandle, ( void * )&pBuf, OS_WAIT_FOREVER ); /* Wait for a message! */
      bRunPatch    = ( bool )false;
      bSaveVars    = ( bool )false;
      ( void )DFWA_getFileVars( &dfwVars );    // Read the DFW NV variables (status, etc...)
      /* Point to the data part of the pBuf, this will be the system message */
      switch( pBuf->eSysFmt ) /*lint !e787 not all enums used within switch */
      {
         case eSYSFMT_TIME:
         {
            /* Pointer to the time message data in the sys msg */
            tTimeSysMsg *pTimeMsg = ( tTimeSysMsg * )&pBuf->data[0]; //lint !e740 !e826  Unusual pointer cast

            DFW_PRNT_INFO( "Time Msg Received" );
            /* Is alarm ID valid AND alarm ID matches AND the cause of the alarm is valid AND we're waiting for apply?  */
            if ( ( NO_ALARM_FOUND != pTimeMsg->alarmId ) && ( alarmId_ == pTimeMsg->alarmId ) &&
                  ( TIME_SYS_ALARM == pTimeMsg->bCauseOfAlarm ) && ( eDFWP_WAIT_FOR_APPLY == dfwVars.ePatchState ) )
            {
               if ( eSUCCESS == TIME_UTIL_GetTimeInSysCombined( &combSysTime ) ) // Read the system time (continue if valid)
               {
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                  if ( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
                  {
                     dfwVars.ePatchState = eDFWP_PROGRAM_SCRIPT;  // Start from meter program script
                  }
                  else
#endif
                  {
                     dfwVars.ePatchState = eDFWP_DO_FLASH_SWAP;  // Start from NV commands
                  }
                  bSaveVars           = ( bool )true;
                  bRunPatch           = ( bool )true;
               }
            }
            break;
         }
#if ( EP == 1 )
         case eSYSFMT_TIME_DIVERSITY:
         {
            if ( NULL != pPatchCompleteMessage )
            {
               ( void )HEEP_MSG_Tx( &patchCompletelTxInfo, pPatchCompleteMessage );
               dfwVars.eSendConfirmation = eDFWC_NONE;
               bSaveVars = ( bool )true;

               // Buffer was freed by HEEP_MSG_Tx.
               pPatchCompleteMessage = NULL;
               DFWA_CancelPending( true );
            }

            if ( NULL != pDownloadCompleteMessage )
            {
               ( void )HEEP_MSG_Tx( &downloadCompletelTxInfo, pDownloadCompleteMessage );
               dfwVars.eSendConfirmation = eDFWC_NONE;
               bSaveVars = ( bool )true;

               // Buffer was freed by HEEP_MSG_Tx.
               pDownloadCompleteMessage = NULL;
               DFWA_CancelPending( false );
            }
            break;
         }
#endif
         case eSYSFMT_DFW:
         {
            eDfwCommand_t dfwCmd = ( eDfwCommand_t ) * ( &pBuf->data[0] ); // Extract the command from the sys message.

            DFW_PRNT_INFO( "Cmd Msg Received" );
            switch( dfwCmd )
            {
               case eDFWC_PREPATCH:
               {
                  if ( eDFWP_IDLE == dfwVars.ePatchState )
                  {
                     dfwVars.eDlStatus   = eDFWS_INITIALIZED_COMPLETE;
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                     if ( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
                     {
                        // this is a meter configuration update, change patch state to perform compatiblit test
                        dfwVars.ePatchState = eDFWP_COMPATIBILITY_TEST;
                     }
                     else
#endif
                     {
                        // default dfw file, change patch state to execute program commands
                        dfwVars.ePatchState = eDFWP_EXEC_PGM_COMMANDS;
                     }
                     dfwVars.applyMode   = eDFWA_PREPATCH;
                     dfwVars.eErrorCode  = eDFWE_NO_ERRORS;
                     eErrorCode_         = eDFWE_NO_ERRORS;
                     bSaveVars           = ( bool )true;
                     bRunPatch           = ( bool )true;
                     DFW_PRNT_INFO( "DFW Pre-Apply" );
                  }
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                  else if ( eDFWP_COMPATIBILITY_TEST == dfwVars.ePatchState )
                  {
                     /* We had a power outage while the meter compatiblity test was being performed.  Since it did not
                        complete before the power outage, just restart the meter compatiblity test after waiting a
                        for other tasks to process after a power outage.*/
                     OS_TASK_Sleep( ONE_MIN );
                     bRunPatch = ( bool )true;
                  }
#endif
                  else
                  {
                     DFW_PRNT_ERROR( "Cmd Msg Rejected" );
                  }
                  break;
               }
               case eDFWC_FULL_APPLY:
               {
                  dfwFullApplyCmd_t *pCmd = ( dfwFullApplyCmd_t * )&pBuf->data[0]; //lint !e740 !e826

                  ( void )memcpy( &dfwVars.applySchedule, &pCmd->applyVars.applySchedule, sizeof( dfwVars.applySchedule ) );
                  ( void )TIME_UTIL_GetTimeInSysCombined( &combSysTime ); /* Convert system time to combined time for comparison below */

                  if ( pCmd->applyVars.applySchedule <= combSysTime ) /* Apply Immediately? */
                  {
                     //Currently redundant based on where eDFWC_FULL_APPLY is called
                     if ( eDFWS_INITIALIZED_COMPLETE == dfwVars.eDlStatus )
                     {
                        if ( eDFWP_IDLE == dfwVars.ePatchState )
                        {
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                           if ( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
                           {
                              dfwVars.ePatchState = eDFWP_COMPATIBILITY_TEST;
                           }
                           else
#endif
                           {
                              dfwVars.ePatchState = eDFWP_EXEC_PGM_COMMANDS;
                           }
                           dfwVars.applyMode   = eDFWA_FULL_APPLY;
                           bSaveVars           = ( bool )true;
                           bRunPatch           = ( bool )true;
                           DFW_PRNT_INFO( "DFW Apply w/Execute" );             // Debugging purposes
                        }
                        else if ( eDFWP_WAIT_FOR_APPLY == dfwVars.ePatchState )
                        {
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                           if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
                           {
                              dfwVars.ePatchState = eDFWP_PROGRAM_SCRIPT;
                           }
                           else
#endif
                           {
                              dfwVars.ePatchState = eDFWP_DO_FLASH_SWAP;
                           }
                           dfwVars.applyMode   = eDFWA_FULL_APPLY;
                           bSaveVars           = ( bool )true;
                           bRunPatch           = ( bool )true;
                           DFW_PRNT_INFO( "DFW Apply" );                      // Debugging purposes
                        }
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                        else if ( eDFWP_PROGRAM_SCRIPT == dfwVars.ePatchState ||
                                  eDFWP_AUDIT_TEST == dfwVars.ePatchState )
                        {
                           /* If we get here, we had a power outage while the patch was in the middle of completing
                              a program script or audit script.  This cannot be handled in DFW init like an enpoint
                              firmware patch because we need HMC to be running in order to process the patch commands.*/

                           DFW_PRNT_INFO( "Meter configuration/fw updates were interrupted by power outage." );
                           DFW_PRNT_INFO( "Meter configuration/fw updates will resume in 60 seconds." );

                           /* Wait a little bit for other tasks to process before jumping back
                              into the configuration updates after a power outage. */
                           OS_TASK_Sleep( ONE_MIN );

                           if( IS_END_DEVICE_TARGET( dfwVars.fileType )  && ( eDFWP_AUDIT_TEST == dfwVars.ePatchState ) )
                           {
                              /*  There is a small window when programming script completes for these dfw file types
                                 where if we lose power during the audit patch state, it is possible all modifications
                                 may not complete.  Resuming the audit after a power outage may result in the the audit
                                 section failing due to all updates not completing before power was lost. If a worst
                                 case time window was known by the meters group for all possible changes to complete,
                                 a delay could be added to the programming script before progressing to the audit.
                                 Then when power was restored after the outage, the audit script could be restarted
                                 knowing all changes should be complete. For now since this window of time is unknown,
                                 revert back to the programming patch state and redo the table writes to ensure all
                                 changes get completed. */
                              bSaveVars = ( bool )true;
                              dfwVars.ePatchState = eDFWP_PROGRAM_SCRIPT;
                           }

                           /* Restart the patch process in the current patch state to continue. */
                           bRunPatch = ( bool )true;
                           DFW_PRNT_INFO( "DFW Apply" );                      // Debugging purposes
                        }
#endif
                        else
                        {
                           DFW_PRNT_ERROR( "Cmd Msg Rejected" );      // Debugging purposes
                        }
                     }
                  }
                  else  // Not an immediate apply, so set the schedule
                  {
                     /* eDFWC_FULL_APPLY only called after a valid DlApply command with
                        INIT_COMPLETE status and in IDLE or WAIT_FOR_APPLY states
                     */
                     tTimeSysCalAlarm  calAlarm;
                     sysTime_t         calDateTime;

                     dfwVars.applyMode        = eDFWA_FULL_APPLY;
                     TIME_UTIL_ConvertSysCombinedToSysFormat( &pCmd->applyVars.applySchedule, &calDateTime );
                     ( void )memset( &calAlarm, 0, sizeof( calAlarm ) );      // Clr cal settings, only set what we need.
                     calAlarm.pMQueueHandle   = &DFWA_MQueueHandle;           // Using message queue
                     calAlarm.ulAlarmDate     = calDateTime.date;             // Set the alarm date
                     calAlarm.ulAlarmTime     = calDateTime.time;             // Set the alarm time
                     calAlarm.bSkipTimeChange = ( bool )true;                 // Don't notify on an time change
                     ( void )TIME_SYS_AddCalAlarm( &calAlarm );               // Setup the alarm!
                     alarmId_                 = calAlarm.ucAlarmId;           // Alarm ID will be set after adding alarm.
                     bSaveVars                = ( bool )true;
                     DFW_PRNT_INFO( "Apply Scheduled" );
                  }
                  break;
               }
               case eDFWC_CANCEL:
               {
                  if ( eDFWS_NOT_INITIALIZED != dfwVars.eDlStatus )
                  {
                     dfwVars.eDlStatus   = eDFWS_NOT_INITIALIZED;
                     dfwVars.ePatchState = eDFWP_IDLE;
                     dfwVars.applyMode   = eDFWA_DO_NOTHING;
                     bSaveVars           = ( bool )true;
                     if ( NO_ALARM_FOUND != alarmId_ )
                     {
                        if ( eSUCCESS == TIME_SYS_DeleteAlarm( alarmId_ ) )
                        {
                           alarmId_ = NO_ALARM_FOUND;
                           DFW_PRNT_INFO( "DFW Canceled (Alarm)" );    // Debugging purposes
                        }
                        else
                        {
                           DFW_PRNT_ERROR( "DFW Canceled (Alarm Failed)" ); // Debugging purposes
                        }
                     }
                     else
                     {
                        DFW_PRNT_INFO( "DFW Canceled" );             // Debugging purposes
                     }
                  }
                  else
                  {
                     DFW_PRNT_INFO( "DFW Cancel Ignored (not initialized)" ); // Debugging purposes
                  }
                  break;
               }
               case eDFWC_CANCEL_APPLY:
               {
                  if ( eDFWS_NOT_INITIALIZED != dfwVars.eDlStatus )
                  {
                     if ( NO_ALARM_FOUND != alarmId_ )    //If there is an alarm scheduled
                     {
                        if ( eSUCCESS == TIME_SYS_DeleteAlarm( alarmId_ ) )
                        {
                           alarmId_ = NO_ALARM_FOUND;
                           DFW_PRNT_INFO( "DFW Alarm Canceled" );    // Debugging purposes
                        }
                        else
                        {
                           DFW_PRNT_ERROR( "DFW(Alarm Cancel Failed" ); // Debugging purposes
                        }
                     }
                  }
                  break;
               }
               default:
               {
                  DFW_PRNT_ERROR( "Unknown Cmd!" );                   // Debugging purposes
                  break;
               }
            }
            break;
         }
      }  /*lint !e787 not all enums used within switch */
      BM_free( pBuf ); /* Free the buffer */
      if ( bSaveVars ) /* Save the file variables? */
      {
         ( void )DFWA_setFileVars( &dfwVars ); // Save the changes
      }
      if ( bRunPatch ) /* Run the patch? */
      {
         DFW_PRNT_INFO( "Running DFW" );
         ( void )doPatch();
      }
   }
} /*lint !e715  Arg0 is ignored */

#if ( EP == 1 )
/***********************************************************************************************************************

   Function name: DFWA_CancelPending

   Purpose: Cancel all messages which are pending due to time diversity or only the download complete message.
   Free the buffers associted with them and delete any pending timers.

   Arguments: uint8_t uCancelAll

   Returns: void

   Side Effects: None

   Reentrant Code: No

   Notes:

******************************************************************************************************************** */
void DFWA_CancelPending( uint8_t uCancelAll )
{
   if ( 0 != uDownloadCompleteTimerId )
   {
      ( void )TMR_DeleteTimer( uDownloadCompleteTimerId );
   }

   if ( NULL != pDownloadCompleteMessage )
   {
      BM_free( pDownloadCompleteMessage );
   }

   pDownloadCompleteMessage = NULL;
   uDownloadCompleteTimerId = 0;

   if ( true == uCancelAll )
   {
      if ( 0 != uPatchCompleteTimerId )
      {
         ( void )TMR_DeleteTimer( uPatchCompleteTimerId );
      }

      if ( NULL != pPatchCompleteMessage )
      {
         BM_free( pPatchCompleteMessage );
      }

      pPatchCompleteMessage = NULL;
      uPatchCompleteTimerId = 0;
   }
}
#endif

/***********************************************************************************************************************

   Function name: DFWA_DownloadComplete

   Purpose: Send download complete message to the DCU.  This will build the message and either send it
   immediately or schedule it to be sent later due to time diversity settings.

   Arguments: None

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

******************************************************************************************************************** */
static void DFWA_DownloadComplete( DFW_vars_t* pDfwVars )
{
   buffer_t* pMessage;

#if ( DCU == 1 )
   if ( VER_isComDeviceStar() )
   {
      STAR_SendDfwAlarm( STAR_DFW_DOWNLOAD_STATUS, ( uint8_t )pDfwVars->eErrorCode );
   }
#endif

   // Build the message.
   HEEP_initHeader( &downloadCompletelTxInfo );
   pMessage = DFWI_DnlComplete( *pDfwVars, &downloadCompletelTxInfo );
   if ( pMessage != NULL )
   {
#if ( EP == 1 )
      uint32_t uTimeDiversity;

      // If there is a pending time diversity timer from a previous attempt to send this message,
      // cancel it.
      DFWA_CancelPending( false );

      // If time diversity is not being used for this message, just send it immediately.
      if ( 0 == DFWTDCFG_get_download_confirm() )
      {
         ( void )HEEP_MSG_Tx( &downloadCompletelTxInfo, pMessage );
         pDfwVars->eSendConfirmation = eDFWC_NONE;
         ( void )DFWA_setFileVars( pDfwVars ); // Save the changes
      }
      else
      {
         pDownloadCompleteMessage = pMessage;

         // Set a timer so the message will be sent after the time diversity delay.
         uTimeDiversity = aclara_randu( 0, ( ( uint32_t ) DFWTDCFG_get_download_confirm() ) * TIME_TICKS_PER_MIN );
         INFO_printf( "Time Diversity: %u Milliseconds", uTimeDiversity );
         DFWA_SetDownloadCompleteTimer( uTimeDiversity );
      }
#else
      // Time diversity is not used for this message on the DCU, so send it immediately.
      ( void )HEEP_MSG_Tx( &downloadCompletelTxInfo, pMessage );
      pDfwVars->eSendConfirmation = eDFWC_NONE;
      ( void )DFWA_setFileVars( pDfwVars ); // Save the changes
#endif
   }
   else
   {
      DFW_PRNT_INFO( "DFW DL Complete, Failed to alloc buffer" );
   }
}

#if ( EP == 1 )
/***********************************************************************************************************************

   Function Name: DFWA_SetDownloadCompleteTimer(uint32_t timeMilliseconds)

   Purpose: Create the timer to re-send the historical data after the given number
   of milliseconds have elapsed.

   Arguments: uint32_t timeRemainingMSec - The number of milliseconds until the timer goes off.

   Returns: returnStatus_t - indicates if the timer was successfully created

   Side Effects: None

   Reentrant Code: NO

**********************************************************************************************************************/
static void DFWA_SetDownloadCompleteTimer( uint32_t timeMilliseconds )
{
   timer_t tmrSettings;             // Timer for sending pending ID bubble up message
   returnStatus_t retStatus;

   ( void )memset( &tmrSettings, 0, sizeof( tmrSettings ) );
   tmrSettings.bOneShot       = true;
   tmrSettings.bOneShotDelete = true;
   tmrSettings.bExpired       = false;
   tmrSettings.bStopped       = false;
   tmrSettings.ulDuration_mS  = timeMilliseconds;
   tmrSettings.pFunctCallBack = DFWA_DownloadCompleteTimerExpired;
   retStatus                  = TMR_AddTimer( &tmrSettings );

   if ( eSUCCESS == retStatus )
   {
      uDownloadCompleteTimerId = ( uint16_t )tmrSettings.usiTimerId;
   }
   else
   {
      uDownloadCompleteTimerId = 0;
      ERR_printf( "Unable to add Download Complete timer" );
   }
}

/***********************************************************************************************************************

   Function Name: DFWA_DownloadCompleteTimerExpired(uint8_t cmd, void *pData)

   Purpose: Called by the timer to send the historical data bubble up message.  Time diversity for historical
   data is implemented in this manner so that in the case of a time jump, it might be necessary to
   either send the message before the time diversity delay has expired or it might be necessary to
   discard the pending message in the case of a backward time jump.
   Since this code is called from within the timer task, we can't muck around with the
   timer, so we need to notify the app message class to do the actual work.

   Arguments:  uint8_t cmd - from the ucCmd member when the timer was created.
   void *pData - from the pData member when the timer was created

   Returns: None

   Side Effects: None

   Reentrant Code: NO

**********************************************************************************************************************/
/*lint -esym(715, cmd, pData ) not used; required by API  */
static void DFWA_DownloadCompleteTimerExpired( uint8_t cmd, void *pData )
{
   static buffer_t buf;

   /***
      Use a static buffer instead of allocating one because of the allocation fails,
      there wouldn't be much we could do - we couldn't reset the timer to try again
      because we're already in the timer callback.
   ***/
   BM_AllocStatic( &buf, eSYSFMT_TIME_DIVERSITY );
   OS_MSGQ_Post( &DFWA_MQueueHandle, ( void * )&buf );

   uDownloadCompleteTimerId = 0;
}  /*lint !e818 pData could be pointer to const */
#endif

/***********************************************************************************************************************

   Function name: DFWA_PatchComplete

   Purpose: Send patch complete message to the DCU.  This will build the message and either send it
   immediately or schedule it to be sent later due to time diversity settings.

   Arguments: None

   Returns: void

   Side Effects: None

   Reentrant Code: Yes

   Notes:

******************************************************************************************************************** */
static void DFWA_PatchComplete( DFW_vars_t* pDfwVars )
{
   buffer_t* pMessage;

   // Build the message.
   HEEP_initHeader( &patchCompletelTxInfo );
   pMessage = DFWI_PatchComplete( &patchCompletelTxInfo, pDfwVars );
   if ( pMessage != NULL )
   {
#if ( EP == 1 )
      uint32_t uTimeDiversity;

      // If there are any messages pending due to time diversity, cancel them.
      DFWA_CancelPending( true );

      // If time diversity is not being used for this message, just send it immediately.
      if ( 0 == DFWTDCFG_get_apply_confirm() )
      {
         ( void )HEEP_MSG_Tx( &patchCompletelTxInfo, pMessage );
         pDfwVars->eSendConfirmation = eDFWC_NONE;
         ( void )DFWA_setFileVars( pDfwVars ); // Save the changes
      }
      else
      {
         pPatchCompleteMessage = pMessage;

         // Set a timer so the message will be sent after the time diversity delay.
         uTimeDiversity = aclara_randu( 0, ( ( uint32_t ) DFWTDCFG_get_apply_confirm() ) * TIME_TICKS_PER_MIN );
         INFO_printf( "Time Diversity: %u Milliseconds", uTimeDiversity );
         DFWA_SetPatchCompleteTimer( uTimeDiversity );
      }
#else
      // Time diversity is not used for this message on the DCU, so send it immediately.
      ( void )HEEP_MSG_Tx( &patchCompletelTxInfo, pMessage );
      pDfwVars->eSendConfirmation = eDFWC_NONE;
      ( void )DFWA_setFileVars( pDfwVars ); // Save the changes
#endif
   }
   else
   {
      DFW_PRNT_INFO( "DFW Complete, Failed to alloc buffer" );
   }
}

#if ( EP == 1 )
/***********************************************************************************************************************

   Function Name: DFWA_SetPatchCompleteTimer(uint32_t timeMilliseconds)

   Purpose: Create the timer to re-send the historical data after the given number
   of milliseconds have elapsed.

   Arguments: uint32_t timeRemainingMSec - The number of milliseconds until the timer goes off.

   Returns: returnStatus_t - indicates if the timer was successfully created

   Side Effects: None

   Reentrant Code: NO

**********************************************************************************************************************/
static void DFWA_SetPatchCompleteTimer( uint32_t timeMilliseconds )
{
   timer_t tmrSettings;             // Timer for sending pending ID bubble up message
   returnStatus_t retStatus;

   ( void )memset( &tmrSettings, 0, sizeof( tmrSettings ) );
   tmrSettings.bOneShot       = true;
   tmrSettings.bOneShotDelete = true;
   tmrSettings.bExpired       = false;
   tmrSettings.bStopped       = false;
   tmrSettings.ulDuration_mS  = timeMilliseconds;
   tmrSettings.pFunctCallBack = DFWA_PatchCompleteTimerExpired;
   retStatus                  = TMR_AddTimer( &tmrSettings );

   if ( eSUCCESS == retStatus )
   {
      uPatchCompleteTimerId = ( uint16_t )tmrSettings.usiTimerId;
   }
   else
   {
      uPatchCompleteTimerId = 0;
      ERR_printf( "Unable to add Patch Complete timer" );
   }
}

/***********************************************************************************************************************

   Function Name: DFWA_PatchCompleteTimerExpired(uint8_t cmd, void *pData)

   Purpose: Called by the timer to send the historical data bubble up message.  Time diveristy for historical
   data is implemented in this manner so that in the case of a time jump, it might be necessary to
   either send the message before the time diversity delay has expired or it might be necessary to
   discard the pending message in the case of a backward time jump.
   Since this code is called from within the timer task, we can't muck around with the
   timer, so we need to notify the app message class to do the actual work.

   Arguments:  uint8_t cmd - from the ucCmd member when the timer was created.
   void *pData - from the pData member when the timer was created

   Returns: None

   Side Effects: None

   Reentrant Code: NO

**********************************************************************************************************************/
/*lint -esym(715, cmd, pData ) not used; required by API  */
static void DFWA_PatchCompleteTimerExpired( uint8_t cmd, void *pData )
{
   static buffer_t buf;

   /***
      Use a static buffer instead of allocating one because of the allocation fails,
      there wouldn't be much we could do - we couldn't reset the timer to try again
      because we're already in the timer callback.
   ***/
   BM_AllocStatic( &buf, eSYSFMT_TIME_DIVERSITY );
   OS_MSGQ_Post( &DFWA_MQueueHandle, ( void * )&buf );

   uPatchCompleteTimerId = 0;
}  /*lint !e818 pData could be pointer to const */
#endif


/***********************************************************************************************************************

   Function name: DFWA_setFileVars

   Purpose: Writes all of the DFW file variables to NV memory.

   Arguments: DFW_vars_t *pFile

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
returnStatus_t DFWA_setFileVars( DFW_vars_t const *pFile )
{
   return ( FIO_fwrite( &DFWVarsFileHndl_, 0, ( uint8_t* )pFile, sizeof( DFW_vars_t ) ) );
}

/***********************************************************************************************************************

   Function name: DFWA_getFileVars

   Purpose: Reads all of the DFW file variables from NV memory

   Arguments: DFW_vars_t *pFile

   Returns: returnStatus_t

   Side Effects: None

   Reentrant Code: Yes

   Notes:

**********************************************************************************************************************/
returnStatus_t DFWA_getFileVars( DFW_vars_t *pFile )
{
   returnStatus_t retVal;

   retVal = FIO_fread( &DFWVarsFileHndl_, ( uint8_t* )pFile, 0, sizeof( DFW_vars_t ) );
   if ( eSUCCESS != retVal )
   {
      eErrorCode_ = eDFWE_NV;
   }
   return( retVal );
}

/***********************************************************************************************************************

   Function name: DFWA_isCipherSupported

   Purpose: Determines if the cipher suite requsted is supported for the dfw file type utilized

   Arguments:  dl_cipher_t  - cipher suite to check being supported
   dl_dfwFileType_t - type of dfw file being applied

   Returns: eSUCCESS if supported, other wise eFAILURE

   Re-entrant Code: Yes

   Notes:

**********************************************************************************************************************/
returnStatus_t DFWA_SupportedCiphers( eDfwCiphers_t cipher, dl_dfwFileType_t fileType )
{
   returnStatus_t retVal = eFAILURE;
   if( DFW_ACLARA_METER_PROGRAM_FILE_TYPE == fileType )
   {
      //  dfw file type is for meter configuration update
      if( eDFWS_NONE == cipher )
      {
         retVal = eSUCCESS;
      }
   }
   else
   {
      //  patch is for application/bootloader - default dfw file type
      if ( eDFWS_AES_CCM == cipher )
      {
         retVal = eSUCCESS;
      }
   }
   return ( retVal );
}

/* ****************************************************************************************************************** */
/* LOCAL FUNCTIONS */


/***********************************************************************************************************************

   Function name: doPatch

   Purpose:  Run the downloaded patch to update the program memory, boot loader memory and the NV memory.

   Apply Steps:
   1 Copy internal flash (PGM) memory to program image partition
   2 Execute patch for application and bootloader (apply the patch to the image in NV memory)
   3 Check CRC32 on the image
   4 Copy application NV data to Image partition in NV memory
   5 Execute patch for application NV data (executes Write NV, Copy NV and Fill NV commands in the patch)
   6 Copy NV Image Data to Application NV memory

   Arguments: none

   Returns: returnStatus_t - eSUCCESS

   Side Effects: None

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static returnStatus_t doPatch( void )
{
   patchHeader_t     PatchHeader;                  // Contains the patch header
   bool              bPatchComplete = ( bool )false; // indicator to get out of patch while loop.
   sysTimeCombined_t combSysTime;                  // Contains the combined system time.
   DFW_vars_t        dfwVars;                      // Contains all of the DFW file variables
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
   returnStatus_t meterPrgRtnValue;                // return status from dfw file section execution
   eFileSection_t dfwFileSection;                  // dfw file section to be performed by dfw process
#endif

   ( void )DFWA_getFileVars( &dfwVars );                 // Read the DFW NV variables (status, etc...)
   if ( eDFWP_IDLE != dfwVars.ePatchState )              // Run if not idle (run if there is something to do) Apply or executing...
   {
      dfwUnitTestToggleOutput_TST1( 500, 1, 5 );
      ( void )getPatchHeader( &PatchHeader, &dfwVars );            // Read the patch header

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
      if ( eDFWP_EXEC_PGM_COMMANDS == dfwVars.ePatchState || eDFWP_COMPATIBILITY_TEST == dfwVars.ePatchState )
#else
      if ( eDFWP_EXEC_PGM_COMMANDS == dfwVars.ePatchState )
#endif
      {
         //Only need to validate entire patch once and needs to be in this state
         if ( eDFWE_NO_ERRORS != validatePatchHeader( eVALIDATE_PATCH_COMPLETELY, dfwVars.fileType ) )
         {
            ( void )DFWA_getFileVars( &dfwVars );           // Re-Read the DFW NV variables since may have changed
            dfwVars.eErrorCode = eErrorCode_;               // Set the error code (in the file)
            dfwVars.eDlStatus  = eDFWS_INITIALIZED_WITH_ERROR; // Set status (in the file) to error
            if ( true == APP_MSG_HandlerReady() )           //If here after a power cycle
            {
               DFWA_DownloadComplete( &dfwVars );  // df/dp complete, with error
            }
            else
            {
               dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;
            }
         }
         ( void )DFWA_getFileVars( &dfwVars );           // Read the DFW NV variables (status, etc...)
         ( void )getPatchHeader( &PatchHeader, &dfwVars ); // Read the patch header
      }

      dfwUnitTestToggleOutput_TST1( 500, 2, 5 );
      while( ( eDFWE_NO_ERRORS == eErrorCode_ ) &&       // While there are no errors and
             !bPatchComplete )                            // while the patch is incomplete
      {
         ( void )DFWA_getFileVars( &dfwVars );           // The NV variables can change going through the loop
         switch( dfwVars.ePatchState )                     /* Execute the appropriate state.   */ /*lint !e788 not all enums used within switch */
         {
            case eDFWP_EXEC_PGM_COMMANDS:
            {
               returnStatus_t retStatus = eSUCCESS;
               bool copyRequired = true;
#if DFW_XZCOMPRESS_BSPATCHING
               macroCmdPatchDecompressionType_t patchDecompressType;
               dSize   PatchOffset = ( dSize )sizeof( patchHeader_t );
               ( void ) PAR_partitionFptr.parRead( ( uint8_t * )&patchDecompressType, PatchOffset,
                                                   ( lCnt )sizeof( patchDecompressType ), pDFWPatch );
               if( eDFWA_CMD_PATCH_DECOMPRESS_TECHNOLOGY == patchDecompressType.command )
               {
                  if( ( eDFW_MINIBSDIFF_PATCH == patchDecompressType.patchType ) &&
                      ( eDFW_XZ_COMPRESSION == patchDecompressType.compressionType ) )
                  {
                     copyRequired = false;
                  }
               }
#endif
               if( copyRequired )
               {
                  DFW_PRNT_INFO("Copying PGM to Image");
                  /* TODO The next to last sector contain non-program data and MUST be excluded from Patching.  This
                   * sector contains data that may change between the time of the following pgm copy and the DFW Apply
                   * (swap time).  To ensure the sector is current, it needs to be copied just prior to the swap.  If this
                   * fails, then the swap cannot take place.
                   */
                  retStatus = DFWA_CopyPgmToPgmImagePartition(false, dfwVars.initVars.target); // Copy PGM to image, if no errors, patch it!
               }

               if ( eSUCCESS == retStatus )
               {
                  dfwUnitTestToggleOutput_TST1( 500, 3, 5 );
                  DFW_PRNT_INFO( "Exe Macro Cmds" );
                  if ( eDFWE_NO_ERRORS == executeMacroCmds( dfwVars.ePatchState, &PatchHeader ) ) // Patch the pgm mem
                  {
                     uint32_t crcPgm = PatchHeader.newCRC32;

                     dfwUnitTestToggleOutput_TST1( 500, 4, 5 );
                     Byte_Swap( ( uint8_t * )&crcPgm, sizeof( crcPgm ) );
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
                     //Perform CRC check on internal Flash image
                     DFW_PRNT_INFO_PARMS( "Check CRC32 on Image 0-0x%lX (0x%lX Bytes)",
                                          PART_PGM_FLASH_CODE_SIZE - 1, PART_PGM_FLASH_CODE_SIZE );
                     if ( eSUCCESS == verifyCrc( ( flAddr )0, PART_PGM_FLASH_CODE_SIZE, crcPgm, pDFWImagePTbl_ ) )
                     {
                        dfwUnitTestToggleOutput_TST1( 500, 5, 5 );
                        DFW_PRNT_INFO( "CRC32 Passed" );
                        if ( eDFWA_FULL_APPLY != dfwVars.applyMode )
                        {
                           DFW_PRNT_INFO( "Pre-Apply Complete!" );
                           dfwVars.ePatchState = eDFWP_WAIT_FOR_APPLY;
                           bPatchComplete      = ( bool )true;
                           //If App MSG Task running then call immediate, otherwise we just come out of reset so let
                           // dfw app task know it needs to send the message
                           if ( true == APP_MSG_HandlerReady() )  //If here after a power cycle
                           {
                              DFWA_DownloadComplete( &dfwVars );  // df/dp complete, no errors
                           }
                           else
                           {
                              dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;
                           }
                        }
                        else
                        {
                           /* This could only happen if the apply schedule was received before the last packet (when
                              state is still eDFWP_IDLE).  This is currently not allowed by the HEEP message processing,
                              where Apply is only allowed when status is eDFWS_INITIALIZED_COMPLETE which only occurs
                              AFTER receiving all the packets.  This is where it also changes state from eDFWP_IDLE to
                              eDFWP_EXEC_PGM_COMMANDS).  If the Apply was accepted, then the apply schedule would also
                              have to be in the past when the last packet arrives.
                              If all that is true does a download confirmation still need to be sent?  Time diversity
                              between the download and the apply confirmation could make them be Tx'd out of order.
                           */
                           dfwVars.ePatchState = eDFWP_DO_FLASH_SWAP;
                           ( void )DFWA_setFileVars( &dfwVars );
                        }
                     }
                     else
                     {
                        eErrorCode_ = eDFWE_CRC32;
                     }
#else
                     //Perform CRC check on external Flash image(s)
                     DFW_PRNT_INFO_PARMS( "Check CRC32 on PGM Image" );
                     uint32_t        crc;
                     returnStatus_t  crcStatus = eFAILURE;

                     if ( eFWT_APP == dfwVars.initVars.target )
                     {
                        //Calculate the lower App code section
                        if ( eSUCCESS == calcCRC( ( flAddr )PART_APP_CODE_OFFSET,
                                                  PART_APP_CODE_SIZE, ( bool )true, &crc, pDFWImagePTbl_ ) )
                        {
#ifdef PART_APP_UPPER_CODE_OFFSET
                           crc = ~crc; //since the calcCRC inverts the result...
                           //Calculate the upper App code section (without reinitializing the CRC)
                           if ( eSUCCESS == calcCRC( ( flAddr )PART_APP_UPPER_CODE_OFFSET,
                                                     PART_APP_UPPER_CODE_SIZE, ( bool )false, &crc, pDFWImagePTbl_ ) )
#endif
                           {
                              crcStatus = eSUCCESS;
                           }
                        }
                     }
                     else if ( eFWT_BL == dfwVars.initVars.target )
                     {
                        //Calculate the BL code section
                        if ( eSUCCESS == calcCRC( ( flAddr )PART_BL_CODE_OFFSET,
                                                  PART_BL_CODE_SIZE, ( bool )true, &crc, pDFWImagePTbl_ ) )
                        {
                           crcStatus = eSUCCESS;
                        }
                     }
                     if ( ( eSUCCESS == crcStatus ) && ( crc == crcPgm ) ) //lint !e644 if crcStatus is set, crc is set
                     {
                        dfwUnitTestToggleOutput_TST1( 500, 5, 5 );
                        DFW_PRNT_INFO( "CRC32 Passed" );
                        if ( eDFWA_FULL_APPLY != dfwVars.applyMode )
                        {
                           DFW_PRNT_INFO( "Pre-Apply Complete!" );
                           dfwVars.ePatchState = eDFWP_WAIT_FOR_APPLY;
                           bPatchComplete      = ( bool )true;
                           //If App MSG Task running then call immediate, otherwise we just come out of reset so let
                           // dfw app task know it needs to send the message
                           if ( true == APP_MSG_HandlerReady() )  //If here after a power cycle
                           {
                              DFWA_DownloadComplete( &dfwVars );  // df/dp complete, no errors
                           }
                           else
                           {
                              dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;
                           }
                        }
                        else
                        {
                           /* This could only happen if the apply schedule was received before the last packet (when
                              state is still eDFWP_IDLE).  This is currently not allowed by the HEEP message processing,
                              where Apply is only allowed when status is eDFWS_INITIALIZED_COMPLETE which only occurs
                              AFTER receiving all the packets.  This is where it also changes state from eDFWP_IDLE to
                              eDFWP_EXEC_PGM_COMMANDS).  If the Apply was accepted, then the apply schedule would also
                              have to be in the past when the last packet arrives.
                              If all that is true does a download confirmation still need to be sent?  Time diversity
                              between the download and the apply confirmation could make them be Tx'd out of order.
                           */
                           dfwVars.ePatchState = eDFWP_DO_FLASH_SWAP;
                           ( void )DFWA_setFileVars( &dfwVars );
                        }
                     }
                     else
                     {
                        eErrorCode_ = eDFWE_CRC32;
                     }
#endif
                  }
               }
               if ( eDFWE_NO_ERRORS != eErrorCode_ ) //Pre-Patch completed, but there were errors (InitializedWithErrors)
               {
                  //If App MSG Task running then call immediate, otherwise we just come out of reset so let
                  // dfw app task know it needs to send the message
                  dfwVars.eErrorCode = eErrorCode_;
                  dfwVars.eDlStatus  = eDFWS_INITIALIZED_WITH_ERROR;
                  if ( true == APP_MSG_HandlerReady() )  //If here after a power cycle
                  {
                     DFWA_DownloadComplete( &dfwVars );  // df/dp complete, with error
                  }
                  else
                  {
                     dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;
                  }
               }
               break;
            }
            case eDFWP_DO_FLASH_SWAP:
            {
               uint32_t       waitTime;   // Time to wait in mS for system to be idle
               returnStatus_t sysLocked;  // Indicates if the Wait for Idle locked the sys busy mutex
#if ( MCU_SELECTED == RA6E1 )
               uint32_t       crcDfwInfo;
#endif

               //TODO: Somewhere in here it needs to "purge" the HEEP/Stack messages before swapping
               dfwUnitTestToggleOutput_TST2( 500, 1, 3 );
               DFW_PRNT_INFO( "Waiting for System to be Idle" );
               // If current time plus grace period is later than expiration, set for expiration
               // Limit grace period to expiration time
               ( void ) TIME_UTIL_GetTimeInSysCombined( &combSysTime ); // Get the Current Time
               waitTime = dfwVars.initVars.gracePeriodSec * ( uint32_t )ONE_SEC; // Convert to system time format
               if ( ( combSysTime + waitTime ) > dfwVars.initVars.expirationTime ) // Is sys time past the expiration time?
               {
                  waitTime = ( uint32_t )( dfwVars.initVars.expirationTime - combSysTime );
               }
               sysLocked = DFWA_WaitForSysIdle( waitTime ); //Need to reset or unlock when done

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
               /* Only required for K22 and DCU applications (i.e., no bootloader exists). */
               if ( eFWT_APP == dfwVars.initVars.target )
               {
                  uint32_t i;                         // Loop iterator

                  // Copy the Encrypt Keys before swapping
                  PWR_lockMutex();
                  for ( i = PART_ENCRYPT_KEY_OFFSET; i < ( PART_ENCRYPT_KEY_OFFSET + PART_ENCRYPT_KEY_SIZE ); i += INT_FLASH_ERASE_SIZE )
                  {
                     if ( eSUCCESS != PAR_partitionFptr.parWrite( i, ( uint8_t * )i, ( lCnt )INT_FLASH_ERASE_SIZE, pDFWImagePTbl_ ) )
                     {
                        DFW_PRNT_ERROR_PARMS( "Copy Pgm %ld", i ); //Remove!
                        eErrorCode_ = eDFWE_FLASH;
                     }
                     CLRWDT();
                  }
                  PWR_unlockMutex();
               }
#endif
               if ( eDFWE_NO_ERRORS != eErrorCode_ ) //Pre-Patch completed, but there were errors (InitializedWithErrors)
               {
                  if ( eSUCCESS == sysLocked )
                  {
                     DFWA_UnlockSysIdleMutex(); //Apply failed, release now so other tasks can access again
                  }
                  //If App MSG Task running then call immediate, otherwise we just come out of reset so let
                  // dfw app task know it needs to send the message
                  dfwVars.eErrorCode = eErrorCode_;
                  dfwVars.eDlStatus  = eDFWS_INITIALIZED_WITH_ERROR;
                  if ( true == APP_MSG_HandlerReady() )  //If here after a power cycle
                  {
                     DFWA_DownloadComplete( &dfwVars );  // df/dp complete, with error
                  }
                  else
                  {
                     dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;
                  }
               }
               else
               {
                  //If the patch is for the Application...
                  if ( eFWT_APP == dfwVars.initVars.target )
                  {
                     //No need to unlock mutext when done since going to reset
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
                     DFW_PRNT_INFO( "Swapping Flash" );
                     ( void )DFWA_SetSwapState( eFS_COMPLETE );
                     DFW_PRNT_INFO( "Resetting Processor" );
                     dfwVars.ePatchState = eDFWP_EXEC_NV_COMMANDS;
#else
                     // Swap system in intFlash prevents modifying the sector with swap indicators while in the ready or update state
                     // The swap system must be uninitialized or in update-erased state to allow modification
                     flashSwapStatus_t pStatus; // to store the current swap system status
                     ( void )memset( &pStatus, 0xFF, sizeof( flashSwapStatus_t ) ); // init the status variable to all ff's
                     ( void )DFWA_GetSwapState( &pStatus ); // get the swap system information

                     if( pStatus.currentMode == eFS_MODE_READY )
                     {
                        ( void )DFWA_SetSwapState( eFS_UPDATE );
                        ( void )memset( &pStatus, 0xFF, sizeof( flashSwapStatus_t ) );
                        ( void )DFWA_GetSwapState( &pStatus ); // get the swap system information
                        // we could do erase here, but next condition will catch where the unit is left in the update state
                     }

                     if( pStatus.currentMode == eFS_MODE_UPDATE )
                     {
                        // erase the swap indicator sectort to put in update-erased state
                        ( void )PAR_partitionFptr.parErase( ( lAddr )0x0, INT_FLASH_ERASE_SIZE, pDFWBLInfoPar_ );
                     }

                     DfwBlInfo_t  DFWBLInfo[2];

                     /* Write the DFW_BL Info... */
                     /* Set the Source as the offset into the ePART_DFW_PGM_IMAGE partition */
                     DFWBLInfo[0].SrcAddr   = PART_APP_CODE_OFFSET;
                     /* Set the Destination as the offset into the ePART_APP_CODE partition as the BL sees it */
                     DFWBLInfo[0].DstAddr   = 0;
                     DFWBLInfo[0].Length    = PART_APP_CODE_SIZE;
                     DFWBLInfo[0].FailCount = DFW_BL_FAILCOUNT_DEFAULT;
#ifdef PART_APP_UPPER_CODE_OFFSET
                     /* Set the Source as the offset into the ePART_DFW_PGM_IMAGE partition */
                     DFWBLInfo[1].SrcAddr   = PART_APP_UPPER_CODE_OFFSET;
                     /* Set the Destination as the offset into the ePART_APP_CODE partition as the BL sees it */
                     DFWBLInfo[1].DstAddr   = PART_APP_UPPER_CODE_OFFSET - PART_APP_CODE_OFFSET;
                     DFWBLInfo[1].Length    = PART_APP_UPPER_CODE_SIZE;
                     DFWBLInfo[1].FailCount = DFW_BL_FAILCOUNT_DEFAULT;
#else
                     /* Set all values to "blank" - no operation required. */
                     DFWBLInfo[1].SrcAddr   = 0xffffffff;
                     DFWBLInfo[1].DstAddr   = 0xffffffff;
                     DFWBLInfo[1].Length    = 0xffffffff;
                     DFWBLInfo[1].FailCount = 0xffffffff;
#endif
                     /* Recalculate the CRC for each section independently */
                     ( void )calcCRC( ( flAddr )PART_APP_CODE_OFFSET,       PART_APP_CODE_SIZE,       ( bool )true, &DFWBLInfo[0].CRC, pDFWImagePTbl_ );
#ifdef PART_APP_UPPER_CODE_OFFSET
                     ( void )calcCRC( ( flAddr )PART_APP_UPPER_CODE_OFFSET, PART_APP_UPPER_CODE_SIZE, ( bool )true, &DFWBLInfo[1].CRC, pDFWImagePTbl_ );
#endif
                     if( eSUCCESS == PAR_partitionFptr.parWrite( PART_DFW_BL_INFO_DATA_OFFSET, ( uint8_t * )&DFWBLInfo[0], ( lCnt )sizeof( DFWBLInfo ), pDFWBLInfoPar_ ) )
                     {
#if ( MCU_SELECTED == RA6E1 )
                       /* Add a CRC following the DFWBLInfo structure since a length of 0xFFFFFFFF
                          is not guaranteed after erasing the DFW_BL_INFO partition as it is in K24 */
                       ( void )calcCRC( ( flAddr )PART_DFW_BL_INFO_DATA_OFFSET, ( lCnt )sizeof( DFWBLInfo ), ( bool )true, &crcDfwInfo, pDFWBLInfoPar_ );
                       ( void )PAR_partitionFptr.parWrite( ( PART_DFW_BL_INFO_DATA_OFFSET + ( lCnt )sizeof( DFWBLInfo ) ), ( uint8_t * )&crcDfwInfo,
                                                           ( lCnt )sizeof( crcDfwInfo ), pDFWBLInfoPar_ );
#endif
                       DFW_PRNT_INFO( "Resetting Processor" );
                       dfwVars.ePatchState = eDFWP_BL_SWAP;
                     }
                     else
                     {
                        // system will not allow write to intflash, need to update errorCode
                        dfwVars.eDlStatus = eDFWS_INITIALIZED_WITH_ERROR;
                        dfwVars.eErrorCode = eDFWE_FLASH;
                        if ( true == APP_MSG_HandlerReady() )
                        {
                           DFWA_DownloadComplete( &dfwVars );  // df/dp complete, with error
                        }
                        else
                        {
                           dfwVars.eSendConfirmation = eDFWC_DL_COMPLETE;
                        }
                     }
                  }
                  else if ( eFWT_BL == dfwVars.initVars.target ) //the patch is for the Bootloader...
                  {
                     uint32_t        crcPgm    = PatchHeader.newCRC32;

                     Byte_Swap( ( uint8_t * )&crcPgm, sizeof( crcPgm ) );
                     if ( eSUCCESS == updateBootloader( crcPgm ) )
                     {
                        DFW_PRNT_INFO( "Resetting Processor" );
                        /* BL patch Complete - NV changes never allowed for BL so indicate done and send confirmation */
                        setDfwToIdle( &dfwVars );
                        bPatchComplete = ( bool )true;
                     }
                     else
                     {
                        /* The Bootloader failed to update! */
                        brick();
                     }
#endif
                  }
                  else  // Error, Patch invalid - should never happen.  Still need to add conditions for Meter FW and TOU Schedules...
                  {
                     break;
                  }
                  ( void )DFWA_setFileVars( &dfwVars );
                  if ( ( bool )true == dfwVars.initVars.bRegister )
                  {
                     //Set up for re-Registration at power up
                     APP_MSG_ReEnableRegistration();
                  }
#if ENABLE_PRNT_DFW_INFO   /* Only delay if printing the status.  The delay allows time for info to be printed. */
                  OS_TASK_Sleep( QTR_SEC );
#endif
                  //Keep all other tasks from running
                  /* Increase the priority of the power and idle tasks. */
                  ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
                  ( void )OS_TASK_Set_Priority( pTskName_Dfw, 11 );
                  ( void )OS_TASK_Set_Priority( pTskName_Idle, 12 );
                  PWR_SafeReset();
               }
               break;   //lint !e527  This is unreachable, but here for completeness.
            }
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
#else
            case eDFWP_BL_SWAP:
            {
               DfwBlInfo_t  DFWBLInfo[2];

               ( void )memset( DFWBLInfo, 0, sizeof ( DFWBLInfo ) );
               ( void )PAR_partitionFptr.parRead( ( uint8_t * )&DFWBLInfo[0], PART_DFW_BL_INFO_DATA_OFFSET, ( lCnt )sizeof( DFWBLInfo ), pDFWBLInfoPar_ );

               //Did BL do the swap?
               if ( ( DFW_BL_FAILCOUNT_DEFAULT == DFWBLInfo[0].FailCount ) && ( DFW_BL_FAILCOUNT_DEFAULT == DFWBLInfo[1].FailCount ) )
               {
                  //TODO: Handle special case where BL did not attempt the swap?
                  // In the wrong state so abort the DFW
                  setDfwToIdle( &dfwVars );
                  ( void )DFWA_setFileVars( &dfwVars );
                  bPatchComplete = ( bool )true;
               }
               else  //Swap completed, continue to next state
               {
                  //TODO: Add logging of any failures?
                  dfwVars.ePatchState = eDFWP_EXEC_NV_COMMANDS;
                  ( void )DFWA_setFileVars( &dfwVars );
               }
               // In either case keep BL from updating
               ( void )memset( DFWBLInfo, 0xFF, sizeof ( DFWBLInfo ) );
               ( void )PAR_partitionFptr.parWrite( PART_DFW_BL_INFO_DATA_OFFSET, ( uint8_t * )&DFWBLInfo[0], ( lCnt )sizeof( DFWBLInfo ), pDFWBLInfoPar_ );
               // Erase entire NV Image partition
               ( void )PAR_partitionFptr.parErase( 0, PART_NV_DFW_PGM_IMAGE_SIZE, pDFWImagePTbl_ );
               break;
            }
#endif
            case eDFWP_EXEC_NV_COMMANDS:
            {
               dfwUnitTestToggleOutput_TST3( 500, 1, 3 );
               // FW version after a swap MUST be the "to" version, otherwise the update did not happen
               firmwareVersion_u currVersion;

               /* The target dfw init variable was added to DFW in v1.42.  This will cause a problem when patching
                  from any version before v1.42 to any version greater or equal to v1.42 because the target parameter
                  in the dfw variables will not be set during dfw initialization.  If we get here during the dfw
                  process and starting firmware version is less than v1.42, we need to explicity make the function to
                  aquire the current firmware version using APP as the target.  Otherwise if the starting firmware is
                  v1.42 or greater, use the target value contained in the dfw init vars for the firmware version
                  function call. */
#if ( DCU == 1 )
               if (  dfwVars.initVars.firmwareVersion.field.version == MUTLI_TARGET_VERSION_LIMIT &&
                     dfwVars.initVars.firmwareVersion.field.revision < MUTLI_TARGET_REVISION_LIMIT )
               {
                  // we are patching from pre v1.42, update the dfw target to indicate APP update
                  dfwVars.initVars.target = eFWT_APP;
                  dfwVars.fileType = DFW_ENDPOINT_FIRMWARE_FILE_TYPE;
               }
#endif

               // get the current firmware version
               currVersion = VER_getFirmwareVersion( dfwVars.initVars.target );

               if ( dfwVars.initVars.toFwVersion.packedVersion == currVersion.packedVersion )
               {
                  /* We are running from the new firmware version.  If this version is going from less than v1.50.58
                     to v1.50.58 or greater, the encryption key partition needs be updated to reflect
                     a change in the data structure representation between firmware versions.
                     TODO: Once TBoard field devices have been upgraded to v1.70, the if statement to upgrade the
                     encryption partition can be removed from the firmware. */
#if  ( DCU == 1 )
                  if (  dfwVars.initVars.firmwareVersion.field.version == ENCRYPTION_KEY_CHANGE_VERSION_LIMIT      &&
                        ( dfwVars.initVars.firmwareVersion.field.revision < ENCRYPTION_KEY_CHANGE_REVISION_LIMIT ) ||
                        ( dfwVars.initVars.firmwareVersion.field.revision == ENCRYPTION_KEY_CHANGE_REVISION_LIMIT  &&
                          dfwVars.initVars.toFwVersion.field.build < ENCRYPTION_KEY_CHANGE_BUILD_LIMIT            ) )
                  {
                     // we need to update the encryption keys to new structure in the encrypt partiion of intflash
                     if( eSUCCESS != updateEncryptPartition() )
                     {
                        /* We failed to update encryption paritition.  Messages will no longer be encrypted which
                           is not allowed so we cannot continue.  Instead of just bricking the unit, reboot and
                           attempt to update the encrypt partition again. */
                        ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
                        ( void )OS_TASK_Set_Priority( pTskName_Dfw, 11 );
                        ( void )OS_TASK_Set_Priority( pTskName_Idle, 12 );
                        PWR_SafeReset();
                     }
                  }
#endif

#if ( EP == 1 )
                  // check to see if we need to update the DTLS Major and Minor partition after the patch
                  if( ( dfwVars.initVars.firmwareVersion.field.version == DTLS_MAJOR_MINOR_UPDATE_VERSION_LIMIT  ) &&
                      ( dfwVars.initVars.firmwareVersion.field.revision <= DTLS_MAJOR_MINOR_UPDATE_REVISION_LIMIT )  )
                  {
                     (void)updateDtlsMajorMinorFiles();
                  }
#endif

                  //TODO Determine if copyNvData can be dependent on there actually being NV commands to execute
                  //     Yes!
                  //     When executeMacroCmds() in eDFWP_EXEC_PGM_COMMANDS sees a NV command, then set a flag indicating NV CMDs present.
                  //     If flag cleared here then skip this and setDfwToIdle ...
                  //     Caveate: DfwVars not used in executeMacroCmds() so pass pointer to variable so the function can set it
                  if ( eDFWE_NO_ERRORS == copyNvData( eCPY_NV_TO_IMAGE ) ) // Copy the NV data over, if no errors, patch it!
                  {
                     dfwUnitTestToggleOutput_TST3( 500, 2, 3 );
                     if ( eDFWE_NO_ERRORS == executeMacroCmds( dfwVars.ePatchState, &PatchHeader ) ) // Patch the NV memory
                     {
                        dfwUnitTestToggleOutput_TST3( 500, 3, 3 );
                        dfwVars.ePatchState = eDFWP_COPY_NV_IMAGE_TO_NV;
                        ( void )DFWA_setFileVars( &dfwVars );
                     }
                     else
                     {
                        brick();
                     }
                  }
                  else
                  {
                     brick();
                  }
               }
               else
               {
                  // Are we still executing the "from" version?
                  if ( dfwVars.initVars.firmwareVersion.packedVersion == currVersion.packedVersion )
                  {
                     // In the wrong state so abort the DFW
                     setDfwToIdle( &dfwVars );
                     /* Only get here after reset. The App MSG Task is not available yet so let
                        dfw app task know it needs to send the message
                     */
                     ( void )DFWA_setFileVars( &dfwVars );
                     //Flush any cache to ensure any NV updates are saved
                     ( void )PAR_partitionFptr.parFlush( NULL );
                     bPatchComplete = ( bool )true;
                     RESET(); //Reset to ensure all file handles are reloaded (safeReset not required since running from init
                     // Send DownloadComplete message with an error
                  }
                  else
                  {
                     // Really bad state - Trying to do NV commands and not either the from or to FW versions
                     brick();
                  }
               }
               break;
            }
            case eDFWP_COPY_NV_IMAGE_TO_NV:
            {
               FileStatus_t  fileStatus;
               dfwUnitTestToggleOutput_TST4( 500, 1, 3 );
               if ( eDFWE_NO_ERRORS == copyNvData( eCPY_IMAGE_TO_NV ) ) //TODO: See TODO in eDFWP_EXEC_NV_COMMANDS
               {
                  //Need to "re-open" the DFW Vars just in case the file location changed!
                  // Ignore return code since the file must have already been opend to get to this point
                  ( void )FIO_fopen( &DFWVarsFileHndl_,             /* File Handle so that PHY access the file. */
                                     ePART_SEARCH_BY_TIMING,           /* Search for partition according to the timing. */
                                     ( uint16_t )eFN_DFW_VARS,         /* File ID (filename) */
                                     ( lCnt )sizeof( DFW_vars_t ),     /* Size of the data in the file. */
                                     FILE_IS_NOT_CHECKSUMED,           /* File attributes to use. */
                                     BANKED_UPDATE_RATE,               /* The update rate of the data in the file. */
                                     &fileStatus );                    /* Contains the file status */
                  dfwUnitTestToggleOutput_TST4( 500, 3, 3 );
                  setDfwToIdle( &dfwVars );
                  /* Only get here after reset. The App MSG Task is not available yet so let
                     dfw app task know it needs to send the message
                  */
                  ( void )DFWA_setFileVars( &dfwVars );
                  bPatchComplete = ( bool )true;
                  //Flush any cache to ensure any NV updates are saved
                  ( void )PAR_partitionFptr.parFlush( NULL );
                  RESET(); //Reset to ensure all file handles are reloaded - Do not need to SafeReset here since can only get here during init
               }
               else
               {
                  brick();
               }
               break;
            }
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
            case eDFWP_COMPATIBILITY_TEST:
            {
               dfwUnitTestToggleOutput_TST6( 500, 1, 2 );
               DFW_PRNT_INFO( "Performing DFW File Compatiblity Check" );

               // prepare the HMC app for meter configuration updates
               if ( eSUCCESS == HMC_PRG_MTR_PrepareToProgMeter() )
               {
                  dfwFileSection = SECTION_COMPATIBILITY;
                  meterPrgRtnValue = HMC_PRG_MTR_RunProgramMeter( dfwFileSection );
                  dfwUnitTestToggleOutput_TST6( 500, 2, 2 );
                  if ( eSUCCESS == meterPrgRtnValue )
                  {
                     if ( eDFWA_FULL_APPLY != dfwVars.applyMode )
                     {
                        DFW_PRNT_INFO( "Pre-Apply Complete!" );
                        dfwVars.fileSectionStatus.compatibilityTestStatus = DFW_FILE_SECTION_PASSED;
                        dfwVars.ePatchState = eDFWP_WAIT_FOR_APPLY;
                        bPatchComplete      = ( bool )true;
                     }
                     else
                     {
                        /* This could only happen if the apply schedule was received before the last packet (when
                           state is still eDFWP_IDLE).  This is currently not allowed by the HEEP message processing,
                           where Apply is only allowed when status is eDFWS_INITIALIZED_COMPLETE which only occurs
                           AFTER receiving all the packets.  This is where it also changes state from eDFWP_IDLE to
                           eDFWP_EXEC_PGM_COMMANDS).  If the Apply was accepted, then the apply schedule would also
                           have to be in the past when the last packet arrives.
                           If all that is true does a download confirmation still need to be sent?  Time diversity
                           between the download and the apply confirmation could make them be Tx'd out of order.
                        */
                        dfwVars.ePatchState = eDFWP_PROGRAM_SCRIPT;
                        ( void )DFWA_setFileVars( &dfwVars );
                     }
                     DFWA_DownloadComplete( &dfwVars ); // df/dp complete, no errors
                  }
                  else if( eFAILURE == meterPrgRtnValue )
                  {
                     /* The compatiblity test section of the dfw file failed, set the compatiblity status
                        parameter and update the static errorCode variable in the module. */
                     DFW_PRNT_ERROR( "Failed DFW File Compatiblity Check." );
                     dfwVars.fileSectionStatus.compatibilityTestStatus = DFW_FILE_SECTION_FAILED;
                     eErrorCode_ = eDFWE_DFW_COMPATIBLITY_TEST; // indicate the compatibility section failed
                  }
                  else
                  {
                     /* compatiblity test was not able to be completed due to an error in either the dfw file or
                        possibliy an NV memory access failure.  Set error code to invalid state so appropriate
                        error messgae is returned in download complete messgae. */
                     DFW_PRNT_ERROR( "Error while attempting to process compatibilty section of dfw file." );
                     eErrorCode_ = eDFWE_INVALID_STATE;  // indicate we had an internal error
                  }
               }
               else
               {
                  /* There was a problem setting up HMC to for meter configuration upates, set error flag to break out
                     of doPatch while loop and appropriate message is returned in download complete message */
                  DFW_PRNT_ERROR( "Error while attempting to setup HMC for meter configuration updates." );
                  eErrorCode_ = eDFWE_INVALID_STATE;
               }

               if ( eDFWE_NO_ERRORS != eErrorCode_ )
               {
                  // we had an error, set the error code in dfw file and send download complete message
                  dfwVars.eErrorCode = eErrorCode_;
                  dfwVars.eDlStatus = eDFWS_INITIALIZED_WITH_ERROR;
                  DFWA_DownloadComplete( &dfwVars ); // df/dp complete, with error
               }
               break;
            }
            case eDFWP_PROGRAM_SCRIPT :
            {
               dfwUnitTestToggleOutput_TST7( 500, 1, 2 );
               DFW_PRNT_INFO( "Performing DFW FIle Programming Script" );

               dfwFileSection = SECTION_PROGRAM;
               meterPrgRtnValue = HMC_PRG_MTR_RunProgramMeter( dfwFileSection );
               dfwUnitTestToggleOutput_TST7( 500, 2, 2 );
               if ( eSUCCESS == meterPrgRtnValue )
               {
                  /* At this point, we have successfully changed the configuration of the meter.  Change the patch
                     so the audit test can be performed. */
                  dfwVars.fileSectionStatus.programScriptStatus = DFW_FILE_SECTION_PASSED;
                  dfwVars.ePatchState = eDFWP_AUDIT_TEST;
               }
               else if( eFAILURE == meterPrgRtnValue )
               {
                  /* We failed our attempt to update the configuration in the meter.  The GOOD NEWS, the meter is still
                     in its initial state. There will be no retries performing the program script.  Just update the
                     program script status, set DFW to IDLE, and send a patch complete messgae back to head end. */
                  DFW_PRNT_ERROR( "Failed DFW File Program Script." );
                  dfwVars.fileSectionStatus.programScriptStatus = DFW_FILE_SECTION_FAILED;
                  eErrorCode_ = eDFWE_DFW_PROGRAM_SCRIPT;
               }
               else
               {
                  // NV problems, possible error in programming script?
                  DFW_PRNT_ERROR( "An error occured while processing the DFW File Program Script." );
                  eErrorCode_ = eDFWE_INVALID_STATE;
               }

               if( eDFWE_NO_ERRORS != eErrorCode_  )
               {
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
                  if( IS_METER_FIRM_UPGRADE( dfwVars.fileType ) && ( DfwCachedAttr.dfwMeterFwRetriesLeft > 0 ) )
                  {
                     /* If the DFW patch is for a meter fw update and the patch attempt fails,
                        check for any necessary dfw retry attempts. */
                     eErrorCode_ = eDFWE_NO_ERRORS;               // clear the error code so doPatch() runs again in current patch state
                     DFWA_DecrementMeterFwRetryCounter();         // decrement the retry counter
                  }
                  else
#endif
                  {
                     dfwVars.eErrorCode = eErrorCode_;
                     setDfwToIdle( &dfwVars );
                     ( void )DFWA_setFileVars( &dfwVars );

                     // Increase the priority of the power and idle tasks.
                     ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
                     ( void )OS_TASK_Set_Priority( pTskName_Dfw, 11 );
                     ( void )OS_TASK_Set_Priority( pTskName_Idle, 12 );
                     PWR_SafeReset();
                  }
               }

               ( void )DFWA_setFileVars( &dfwVars ); // save the dfw variables

               break;
            }
            case eDFWP_AUDIT_TEST :
            {
               dfwUnitTestToggleOutput_TST8( 500, 1, 2 );
               DFW_PRNT_INFO( "Performing DFW File Audit Test" );

               dfwFileSection = SECTION_AUDIT;
               meterPrgRtnValue = HMC_PRG_MTR_RunProgramMeter( dfwFileSection );
               dfwUnitTestToggleOutput_TST8( 500, 2, 2 );
               if ( eSUCCESS == meterPrgRtnValue )
               {
                  /* The audit test passed and meter configuration has been changed successfully. */
                  dfwVars.fileSectionStatus.auditTestStatus = DFW_FILE_SECTION_PASSED;
               }
               else if( eFAILURE == meterPrgRtnValue )
               {
                  // this is bad, configuration in meter was changed but audit test failed
                  DFW_PRNT_ERROR( "Failed DFW File Audit Test." );
                  dfwVars.fileSectionStatus.auditTestStatus = DFW_FILE_SECTION_FAILED;
                  dfwVars.eErrorCode = eDFWE_DFW_AUDIT_TEST;
               }
               else
               {
                  // NV problems or failed to read meter?
                  DFW_PRNT_ERROR( "An error occured while processing the DFW file Audit Test." );
                  dfwVars.eErrorCode = eDFWE_INVALID_STATE;
               }

               if ( ( bool )true == dfwVars.initVars.bRegister )
               {
                  //Set up for re-Registration at power up
                  APP_MSG_ReEnableRegistration();
               }

#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
               if (  ( eDFWE_NO_ERRORS != eErrorCode_ ) &&
                     ( IS_METER_FIRM_UPGRADE( dfwVars.fileType ) ) &&
                     ( DfwCachedAttr.dfwMeterFwRetriesLeft > 0 ) )
               {
                  /* If the DFW patch is for a meter fw update and the patch attempt fails,
                     check for any necessary dfw retry attempts. */
                  eErrorCode_ = eDFWE_NO_ERRORS;               // clear the error code so doPatch() continues with current patch state
                  DFWA_DecrementMeterFwRetryCounter();         // decrement the retry counter
                  dfwVars.ePatchState = eDFWP_PROGRAM_SCRIPT;  // set patch state to program script for dfw retry attempt
                  ( void )DFWA_setFileVars( &dfwVars );        // save new patch state
               }
               else
#endif
               {
                  /* At this point in the process, the meter configuration has been changed.  Whether we passed the audit
                     or not, we need to set DFW back to an idle state.  If the audit failed, setting DFW back to idle state
                     will allow the possiblity to start a new DFW process in order to put the meter back into its initial
                     state. */
                  setDfwToIdle( &dfwVars );
                  ( void )DFWA_setFileVars( &dfwVars );

                  // Increase the priority of the power and idle tasks.
                  ( void )OS_TASK_Set_Priority( pTskName_Pwr, 10 );
                  ( void )OS_TASK_Set_Priority( pTskName_Dfw, 11 );
                  ( void )OS_TASK_Set_Priority( pTskName_Idle, 12 );
                  PWR_SafeReset();
               }

               break;
            }
#endif //( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED )
            default: // WAIT_FOR_APPLY and VERIFY_SWAP states(?) (IDLE not possible here)
            {
               DFW_PRNT_ERROR( "Invalid Patch State" );
               eErrorCode_ = eDFWE_INVALID_STATE;
               break;
            }
         } //lint !e788  Some enums in eDfwPatchState_t aren't used.
      }  //End of while((eDFWE_NO_ERRORS == eErrorCode_) && ...
      if ( eDFWE_NO_ERRORS != eErrorCode_ )              // Were there any errors?  If so, process error & Stop Patching
      {
         // Stop patching
         dfwVars.eErrorCode = eErrorCode_;               // Set the error code (in the file)
         dfwVars.eDlStatus  = eDFWS_INITIALIZED_WITH_ERROR; // Set status (in the file) to error
      }
      if ( eDFWP_WAIT_FOR_APPLY != dfwVars.ePatchState )   /* Only change the state to idle if not in wait for apply state
      (already know it cannot be in the idle state)  */
      {
         dfwVars.ePatchState = eDFWP_IDLE;               // Set state to idle since patching is done.
      }
      dfwVars.applyMode = eDFWA_DO_NOTHING;              // Clear the apply mode.
      ( void )DFWA_setFileVars( &dfwVars );              // Save all variables (save the file)
   }
   return( eSUCCESS );
}

/***********************************************************************************************************************

   Function Name: getPatchHeader

   Purpose: Reads the patch header from NV memory

   Arguments:  patchHeader_t    * pPatchHeader   - Pointer to where the patch header data will be stored
   const DFW_vars_t * const pDfwVars - Pointer to the Vars to get current states

   Returns: eDfwErrorCodes_t

   Side Effects: None

   Reentrant Code: No

   Notes:   The Patch header for the encryption implementation will be in either the 1st or 2nd sector of the NV
   partition.
   If encryption is not enabled, the header is always in the 1st sector.  If encryption is enabled, but
   decryption process not started yet, the header is in the 2nd sector.  If encryption is enabled and the
   decryption process started, partial header data is in the 1st sector.  The header data in the 2nd sector
   may be corrupted.  If encryption is enabled and the decryption process completed all header data is in the
   first sector and has been updated to looks just like a non-encrypted patch.
   Only when the decryption state is DECRYPTING the patch header cannot be retrieved.  Any other time, the
   patch header can be safely retrieved from the sector indicated.

**********************************************************************************************************************/
static eDfwErrorCodes_t getPatchHeader( patchHeader_t *pPatchHeader, const DFW_vars_t * const pDfwVars )
{
   // If Decrypting, there is no way to know if the patch header was decrypted correctly
   if ( eDFWD_DECRYPTING == pDfwVars->decryptState )
   {
      eErrorCode_ = eDFWE_DECRYPT;
   }
   else
   {
      if ( eSUCCESS != PAR_partitionFptr.parRead( ( uint8_t * )pPatchHeader,
            ( dSize )pDfwVars->patchSector * EXT_FLASH_ERASE_SIZE,
            ( lCnt )sizeof( patchHeader_t ),
            pDFWPatch ) )
      {
         eErrorCode_ = eDFWE_NV;
      }
   }
   return( eErrorCode_ );
}

/***********************************************************************************************************************

   Function name: validatePatchHeader

   Purpose: Validate the patch

   Arguments: eValidatePatch_t valPatchCmd

   Returns: eDfwErrorCodes_t

   Side Effects: eErrorCode_ may be modified

   Reentrant Code: No

**********************************************************************************************************************/
static eDfwErrorCodes_t validatePatchHeader( eValidatePatch_t valPatchCmd, dl_dfwFileType_t fileType )
{
   patchHeader_t    patchHeader;                   // Hold patch header
   DFW_vars_t       dfwVars;                       // Contains all of the DFW file variables
   uint32_t         patchOffset;                   // Set to include offset of patch in partition

   ( void )DFWA_getFileVars( &dfwVars );           // Read the DFW NV variables (status, etc...)

   eErrorCode_ = dfwVars.eErrorCode;               // Clear the Error code (SMG: This will "restore" the error)
   /* Retrieve patch information programmed in NV */
   if ( eDFWE_NO_ERRORS == getPatchHeader( &patchHeader, &dfwVars ) ) //Sets (if error detected) and returns eErrorCode_
   {
      Byte_Swap( ( uint8_t * )&patchHeader.patchLength, sizeof( patchHeader.patchLength ) );
      Byte_Swap( ( uint8_t * )&patchHeader.patchCRC32, sizeof( patchHeader.patchCRC32 ) );
      patchOffset = dfwVars.patchSector * EXT_FLASH_ERASE_SIZE;
      if ( eVALIDATE_PATCH_COMPLETELY == valPatchCmd )
      {
         /* If the patch length is incorrect then abort */
         if ( patchHeader.patchLength != ( int32_t )dfwVars.initVars.patchSize )
         //TODO: Add check to ensure an encrypted patch is long enough (min patch length plus auth tag and signature
         {
            eErrorCode_ = eDFWE_PATCH_LENGTH;
         }
         else if ( eSUCCESS != verifyCrc( ( flAddr )offsetof( patchHeader_t, newCRC32 ) + patchOffset,
                                          ( uint32_t )( dfwVars.initVars.patchSize - ( uint32_t )offsetof( patchHeader_t, newCRC32 ) ),
                                          patchHeader.patchCRC32, pDFWPatch ) )
         {
            eErrorCode_ = eDFWE_CRC32PATCH;
         }
         /* If the patch format version is incorrect then abort */
         else if ( ( !DFWA_verifyPatchFormat( dfwVars.fileType, patchHeader.patchFormat ) ) && ( 0 != patchHeader.patchFormat ) ) /*lint !e644 patchHeader initialized.  */

         {
            eErrorCode_ = eDFWE_PATCH_FORMAT;
         }
         /* If decryption is ready (enabled) and the signature check fails */
         else if ( ( ( DFW_ENDPOINT_FIRMWARE_FILE_TYPE == fileType ) || IS_METER_FIRM_UPGRADE( fileType ) ) &&
                   eSUCCESS != verifySignature( dfwVars.decryptState,
                                                pDFWPatch,
                                                /* Encrypted Patches are store one sector up in partition */
                                                patchOffset,
                                                /* Size is everything except the signature*/
                                                dfwVars.initVars.patchSize - SIG_PACKET_SIZE ) ) //lint !e826  Ptr to msg data
         {
            EventKeyValuePair_s  keyVal;     /* Event key/value pair info  */
            EventData_s          event;      /* Event info  */

            //Generate event comDevice.firmware.signature.failed, 164, and pass dfwStreamID as a Name/Value Pair.
            event.eventId                 = ( uint16_t )comDeviceFirmwareSignatureFailed;
            *( uint16_t * )&keyVal.Key[0]   = ( uint16_t )dfwStreamID; /*lint !e826 area too small   */
            ( void )memcpy( keyVal.Value, &dfwVars.initVars.streamId, sizeof( dfwVars.initVars.streamId ) );
            event.markSent = ( bool )false;
            event.eventKeyValuePairsCount = 1;
            event.eventKeyValueSize       = sizeof( dfwVars.initVars.streamId );
            ( void )EVL_LogEvent( 120, &event, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
            eErrorCode_ = eDFWE_SIGNATURE;
         }
         /* If decryption is ready(enabled) and the patch is not successfully decrypted */
         else if ( ( ( DFW_ENDPOINT_FIRMWARE_FILE_TYPE == fileType ) || IS_METER_FIRM_UPGRADE( fileType ) ) &&
                   eSUCCESS != decryptPatch( &dfwVars, &patchHeader ) )
         {
            EventKeyValuePair_s  keyVal;     /* Event key/value pair info  */
            EventData_s          event;      /* Event info  */

            //Generate event comDevice.firmware.decryption.failed, 163, and pass dfwStreamID as a Name/Value Pair.
            event.eventId                 = ( uint16_t )comDeviceFirmwareDecryptionFailed;
            *( uint16_t * )&keyVal.Key[0]   = ( uint16_t )dfwStreamID; /*lint !e826 area too small   */
            ( void )memcpy( keyVal.Value, &dfwVars.initVars.streamId, sizeof( dfwVars.initVars.streamId ) );
            event.markSent = ( bool )false;
            event.eventKeyValuePairsCount = 1;
            event.eventKeyValueSize       = sizeof( dfwVars.initVars.streamId );
            ( void )EVL_LogEvent( 120, &event, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
            eErrorCode_ = eDFWE_DECRYPT;
         }
         else if ( strncmp( ( char * )&patchHeader.deviceType[0], ( char * )&dfwVars.initVars.comDeviceType[0],
                            COM_DEVICE_TYPE_LEN ) )
         {
            eErrorCode_ = eDFWE_DEV_TYPE;
         }
         else if( DFW_ENDPOINT_FIRMWARE_FILE_TYPE == fileType )
         {
            // If FW version is validated after a SWAP image it will fail!
            firmwareVersion_u currentFwVer = VER_getFirmwareVersion( dfwVars.initVars.target );    /* Read the firmware version */
            firmwareVersion_u patchFwVer;

            patchFwVer.field.version  = patchHeader.rceFwVersion[0];
            patchFwVer.field.revision = patchHeader.rceFwVersion[1];
            patchFwVer.field.build    = ( uint16_t )( ( ( ( uint16_t )patchHeader.rceFwVersion[2] << 8 ) & 0xFF00 ) | patchHeader.rceFwVersion[3] );
            if ( patchFwVer.packedVersion != currentFwVer.packedVersion )   /* Does the firmware version match? */
            {
               eErrorCode_ = eDFWE_FW_VER;
            }
         }
      }
      else if ( ( eVALIDATE_PATCH_CRC_ONLY == valPatchCmd ) && ( eDFWE_NO_ERRORS == dfwVars.eErrorCode ) )
      {
         // An invalid length could cause problems for the CRC check
         if ( patchHeader.patchLength != ( int32_t )dfwVars.initVars.patchSize )
         {
            eErrorCode_ = eDFWE_PATCH_LENGTH;
         }
         else if ( eSUCCESS == verifyCrc( ( flAddr )offsetof( patchHeader_t, newCRC32 ) + patchOffset,
                                          ( uint32_t )( dfwVars.initVars.patchSize - ( uint32_t )offsetof( patchHeader_t, newCRC32 ) ),
                                          patchHeader.patchCRC32, pDFWPatch ) )
         {
            DFW_PRNT_INFO( "Patch CRC Passed" );
         }
         else
         {
            DFW_PRNT_ERROR( "Patch CRC Failed" );
            eErrorCode_ = eDFWE_CRC32PATCH;
         }
      }
   }
   return( eErrorCode_ );
}

/***********************************************************************************************************************

   Function name: decryptPatch

   Purpose: Performs all the steps to decrypting the Patch.

   Arguments: DFW_vars_t * to the current DFW varaibles structure

   Returns: returnStatus_t  eSUCCESS, eFAILURE

   Side Effects: The DFW Patch space will be modified.  If errors occur, it is not recoverable.

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static returnStatus_t decryptPatch( DFW_vars_t * const pVars, patchHeader_t *pPatchHeader )
{
   returnStatus_t retStatus;
   uint8_t        authTag[AES_BLOCK_SIZE];   // Use to hold the auth tag from patch
   uint32_t       authOffset;                // Used to get the offset into the patch for the auth tag
   uint32_t       crc;                       // Used to calculate the new Patch CRC32 after decryption

   retStatus = eSUCCESS;
   if ( ( eDFWD_READY == pVars->decryptState ) ) // If patch ready for decryption...
   {
      retStatus = eFAILURE;
      // Make the derived Key
      if( eSUCCESS == DFWA_setDerivedKey( pVars->initVars.firmwareVersion, pVars->initVars.toFwVersion ) )
      {
         // Get the Authentication tag
         authOffset = ( EXT_FLASH_ERASE_SIZE + pVars->initVars.patchSize - SHA256_BLOCK_SIZE ) - AES_BLOCK_SIZE;
         if ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t * )&authTag[0], authOffset, ( lCnt )AES_BLOCK_SIZE,
               pDFWPatch ) )
         {
            // If encrypted patch:
            // DFW Download wrote packets starting in the second sector
            // Now copy newCRC32 and patchFormat from second to first sector of the external NV
            // Leave patchLength and patchCRC32 as-is until after decryption is complete
            uint8_t Buffer[HDR_NEW_CRC_AND_FORMAT_SIZE];
            readWriteBuffer( ( dSize )offsetof( patchHeader_t, newCRC32 ),
                             ( dSize )EXT_FLASH_ERASE_SIZE + ( dSize )offsetof( patchHeader_t, newCRC32 ),
                             Buffer,
                             ( lCnt )HDR_NEW_CRC_AND_FORMAT_SIZE,
                             pDFWPatch, pDFWPatch );
            // Attempt to Decrypt the Patch
            dfwUnitTestToggleOutput_TST5( 500, 1, 3 );
            pVars->decryptState = eDFWD_DECRYPTING;
            //Write update to NV
            ( void )DFWA_setFileVars( pVars );
            if ( eSUCCESS == decryptData(
                     pDFWPatch,
                     /* Encrypted Patches are stored one sector higher
                        Encrypted data starts with the deviceType field
                     */
                     EXT_FLASH_ERASE_SIZE + ( flAddr )offsetof( patchHeader_t, deviceType ),
                     /* Decrypted data is stored one sector lower */
                     ( flAddr )offsetof( patchHeader_t, deviceType ),
                     &authTag[0],
                     AES_BLOCK_SIZE,
                     /* The size of the encrypted data does not include the AuthCode and Signature */
                     pVars->initVars.patchSize -
                     ( ( uint32_t )offsetof( patchHeader_t, deviceType ) + AES_BLOCK_SIZE + SHA256_BLOCK_SIZE )
                  ) )
            {
               dfwUnitTestToggleOutput_TST5( 500, 2, 3 );
               //Write updated patchLength to first sector of NV
               pVars->initVars.patchSize -= ( dl_patchsize_t )AES_BLOCK_SIZE + ( dl_patchsize_t )SHA256_BLOCK_SIZE;
               pPatchHeader->patchLength  = ( int32_t )pVars->initVars.patchSize;
               Byte_Swap( ( uint8_t * )&pPatchHeader->patchLength, sizeof( pPatchHeader->patchLength ) );
               ( void )PAR_partitionFptr.parWrite( ( uint32_t )offsetof( patchHeader_t, patchLength ),
                                                   ( uint8_t * )&pPatchHeader->patchLength,
                                                   ( lCnt )sizeof( pPatchHeader->patchLength ),
                                                   pDFWPatch );
               //Write updated patchCRC32 to first sector of NV
               if ( eSUCCESS == calcCRC( ( flAddr )offsetof( patchHeader_t, newCRC32 ),
                                         ( uint32_t )pVars->initVars.patchSize - ( uint32_t )offsetof( patchHeader_t, newCRC32 ),
                                         ( bool )true, &crc,
                                         pDFWPatch ) )
               {
                  pPatchHeader->patchCRC32 = crc;
                  Byte_Swap( ( uint8_t * )&pPatchHeader->patchCRC32, sizeof( pPatchHeader->patchCRC32 ) );
                  ( void )PAR_partitionFptr.parWrite( ( uint32_t )offsetof( patchHeader_t, patchCRC32 ),
                                                      ( uint8_t * )&pPatchHeader->patchCRC32,
                                                      ( lCnt )sizeof( pPatchHeader->patchCRC32 ),
                                                      pDFWPatch );
                  // Adjust the patchSector for the new location
                  pVars->patchSector  = PATCH_DECRYPTED_SECTOR;
                  pVars->decryptState = eDFWD_DONE;
                  //Re-read the patch header from the unencrypted patch
                  ( void )getPatchHeader( pPatchHeader, pVars );
                  retStatus = eSUCCESS;
               }
               //Write updates to NV
               dfwUnitTestToggleOutput_TST5( 500, 3, 3 );
               ( void )DFWA_setFileVars( pVars );
            }
            //TODO: Should an else be added to erase the patch since it failed to decrypt and authenticate?
         }
      }
   }
   return( retStatus );
}

/***********************************************************************************************************************

   Function Name: executeMacroCmds

   Purpose: Executes the patch (macro language) on the NV image partition

   Arguments: eDfwPatchState_t ePatchState, patchHeader_t *pPatchHeader

   Returns: eDfwErrorCodes_t

   Side Effects: If an error occurs, eErrorCode_ is set to eDFWE_NV.

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static eDfwErrorCodes_t executeMacroCmds( eDfwPatchState_t ePatchState, patchHeader_t const *pPatchHeader )
{
   lCnt     Length;                                        // Length of data to move
   uint32_t patchLen;                                      // patch len converted to native format from *pPatchHeader
   dSize    PatchOffset = ( dSize )sizeof( patchHeader_t ); // Compute address of first patch section;
   /* Do NOT make this union auto/local!  Their odd size will cause a data alignment error when accessing other auto
      variables on the stack.  This results in a trap and watchdog reset! */
   static union
   {
      macroCmdRst_t    reset; // Reset Command structure
      macroCmdWrite_t  write; // Write Command structure
      macroCmdFill_t   fill;  // Fill Command structure
      macroCmdCopy_t   copy;  // Copy Command structure
      macroCmdPatchDecompressionType_t patchDecompressType; // Decompress and write command structure
   } patchCmd_;               // Union used for patch command - All structures have the command as the 1st parameter.

   patchLen = ( uint32_t )pPatchHeader->patchLength;     // Get the patch length (it is in big endian at this point)
   Byte_Swap( ( uint8_t * )&patchLen, sizeof( patchLen ) ); // Convert patch length to little endian format
   patchLen -= sizeof( patchHeader_t );                  // Subtract off of patch header, this is the len of all cmds
   while ( ( eDFWE_NO_ERRORS == eErrorCode_ ) && ( patchLen > 0 ) ) // Execute each patch command
   {
      /* Read patch section */
      if ( eSUCCESS == PAR_partitionFptr.parRead( ( uint8_t * )&patchCmd_, PatchOffset, ( lCnt )sizeof( patchCmd_ ),
            pDFWPatch ) )
      {
         dSize       DstAddr;                            // Destination address of the data to move
         eMacroCmd_t eCommand = patchCmd_.write.command; // Patch command that is being executed
         /*lint -esym(772,Buffer) */
         uint8_t     Buffer[READ_WRITE_BUFFER_SIZE];     // Buffer to hold data between reads and writes
#if (defined TM_DFW_APP_RST_TEST_2)
         togglePinApp( 50 );
#endif

         switch ( patchCmd_.write.command )  /* What to do? */
         {
            /* NOTE: for PIC24, src/dst addresses are never on a phantom byte & data length never counts phantom byte */

            case eDFWA_CMD_RESET_PROCESSOR:
            {
               // Not implemented.  This is no longer needed.
               eErrorCode_ = eDFWE_CMD_CODE;
               break;
            }
            case eDFWA_CMD_WRITE_DATA:
            case eDFWA_CMD_WRITE_DATA_NV:
            {
               dSize  SrcAddr;   // Source address of the data to move
               lCnt   ReadCount; // Number of byte to move up to READ_WRITE_BUFFER_SIZE

               DstAddr     = getThreeByteAddr( &patchCmd_.write.destAddr[0] );      /* Get dest addr */
               Length      = EXTRACT_LEN( patchCmd_.write.length );                 /* Retrieve length */
               SrcAddr     = PatchOffset + sizeof( macroCmdWrite_t ); /* Point to 1st byte of data(1st byte after len) */
               patchLen   -= ( int32_t )sizeof( macroCmdWrite_t ) + Length;   /* Adjust patch length */
               PatchOffset = SrcAddr + Length;                                /* Point to next command */
               ReadCount   = sizeof( Buffer );                                /* Limit read to available buffer space */
               while ( Length )  /* Write payload of the write command */
               {
                  /* Read as many bytes as possible to fill the buffer */
                  if ( ReadCount > Length ) /* If the ReadCount > length, only read the length num of bytes */
                  {
                     ReadCount = Length;  /* Entire request fits in buffer */
                  }
                  /* Only execute the write command if the state matches the command. */
                  if ( ( eDFWP_EXEC_PGM_COMMANDS == ePatchState ) && ( eDFWA_CMD_WRITE_DATA == eCommand ) )
                  {
                     /* Read data from patch partition and save it back to image partition */
                     readWriteBuffer( DstAddr, SrcAddr, Buffer, ReadCount, pDFWImagePTbl_, pDFWPatch );
                  }
                  else if ( ( eDFWP_EXEC_NV_COMMANDS == ePatchState ) && ( eDFWA_CMD_WRITE_DATA_NV == eCommand ) )
                  {
                     /* Read data from patch partition and save it back to image partition */
                     readWriteBuffer( DstAddr, SrcAddr, Buffer, ReadCount, pDFWImageNvPTbl_, pDFWPatch );
                  }
                  SrcAddr += ReadCount;   /* Point to next chunk of data to read */
                  DstAddr += ReadCount;   /* Point to next chunk of data to write */
                  Length  -= ReadCount;   /* Decrease the length by the number of bytes written. */
               }
               break;
            }
#if DFW_XZCOMPRESS_BSPATCHING
            case eDFWA_CMD_PATCH_DECOMPRESS_TECHNOLOGY:
            {
               int32_t newsz, ctrllen, datalen;
               PartitionData_t const *pOldImagePartition;
               Length = getThreeByteAddr( &patchCmd_.patchDecompressType.length[0] );
               dSize patchSrcOffset = PatchOffset + sizeof( macroCmdPatchDecompressionType_t );
               PatchOffset = patchSrcOffset + Length;        /* End of the compressed patch, also points to next command */
               patchLen   -= ( int32_t )sizeof( macroCmdPatchDecompressionType_t ) + Length; /* Adjust patch length */
               if( eDFWP_EXEC_PGM_COMMANDS == ePatchState )
               {
                  if( ( eDFW_MINIBSDIFF_PATCH == patchCmd_.patchDecompressType.patchType ) &&
                      ( eDFW_XZ_COMPRESSION == patchCmd_.patchDecompressType.compressionType ) )
                  {
                     if( eSUCCESS == DFW_XZMINI_uncompressPatch_init( pDFWPatch, patchSrcOffset, PatchOffset ) )
                     {
                        /* Validate the patch header */
                        if( eSUCCESS == DFW_XZMINI_bspatch_valid_header( &newsz, &ctrllen, &datalen ) )
                        {
                           DFW_vars_t dfwVars;
                           ( void )DFWA_getFileVars( &dfwVars );
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
                           ( void )PAR_partitionFptr.parOpen( &pOldImagePartition, ePART_APP_CODE, 0L );
                           /* Check for power down before processing. */
                           PWR_CheckPowerDown();
                           /* Set the swap state to INIT */
                           ( void )DFWA_SetSwapState( eFS_INIT );
#else
                           if( eFWT_APP == dfwVars.initVars.target )
                           {
                              pOldImagePartition = pAppCodePart_;
                           }
                           else if( eFWT_BL == dfwVars.initVars.target )
                           {
                              pOldImagePartition = pDFWBLInfoPar_;
                           }
#endif
                           /* Erase the NV image partition before start patching */
                           ( void )PAR_partitionFptr.parErase( 0, DFW_PGM_IMAGE_SIZE, pDFWImagePTbl_ );
                           /* Apply patch */
                           if ( eSUCCESS == DFW_XZMINI_bspatch( pOldImagePartition, dfwVars.initVars.target, pDFWImagePTbl_, ( off_t ) newsz ) ) //lint !e644, pOldImagePartition initialized with Appcode/BLinfo partition
                           {
                              DFW_PRNT_INFO("XZ Minibs - Patch applied successfully");
                           }
                           else
                           {
                              DFW_PRNT_INFO("XZ Minibs - Patch failed");
                           }
                        }
                     }
                  }
               }

               break;
            }
#endif
            case eDFWA_CMD_FILL_REGION:
            case eDFWA_CMD_FILL_REGION_NV:
            {
               /* Retrieve destination address */
               lCnt   ReadCount; // Number of byte to move up to READ_WRITE_BUFFER_SIZE

               DstAddr      = getThreeByteAddr( &patchCmd_.fill.destAddr[0] );
               Length       = EXTRACT_LEN( patchCmd_.fill.length ); /* Retrieve Length */
               patchLen    -= sizeof( macroCmdFill_t );           /* Adjust patch length */
               PatchOffset += sizeof( macroCmdFill_t );           /* Point to next command */
               ReadCount    = sizeof( Buffer );                   /* Limit read to available buffer space */
               while ( 0 != Length )                              /* Write payload to NV */
               {
                  /* Read as many bytes as possible to fill the buffer */
                  uint16_t i;   // Loop counter

                  if ( ReadCount > Length )
                  {
                     /* Entire request fits in buffer */
                     ReadCount = Length;
                  }
                  for ( i = 0; i < ReadCount; i++ )   /* Fill buffer with fill pattern */
                  {
                     Buffer[i] = patchCmd_.fill.data[i % 2]; /* Fill Cmd Data is 2 bytes in length. */
                  }
                  PWR_CheckPowerDown(); /* Check for power down before processing. */
                  /* Only execute the write command if the state matches the command. */
                  if ( ( eDFWP_EXEC_PGM_COMMANDS == ePatchState ) && ( eDFWA_CMD_FILL_REGION == eCommand ) )
                  {
                     /* Save buffer to NV to patch a section. */
                     ( void )PAR_partitionFptr.parWrite( DstAddr, Buffer, ( lCnt )ReadCount, pDFWImagePTbl_ );
                  }
                  else if ( ( eDFWP_EXEC_NV_COMMANDS == ePatchState ) && ( eDFWA_CMD_FILL_REGION_NV == eCommand ) )
                  {
                     /* Save buffer to NV to patch a section. */
                     ( void )PAR_partitionFptr.parWrite( DstAddr, Buffer, ( lCnt )ReadCount, pDFWImageNvPTbl_ );
                  }
                  CLRWDT();
                  DstAddr += ReadCount;
                  Length -= ReadCount;
               }
               break;
            }
            case eDFWA_CMD_COPY_DATA:
            case eDFWA_CMD_COPY_DATA_NV:
            {
               /* Retrieve destination address */
               dSize    SrcAddr;                      // Source address of the data to move
               bool     bReverseCopy = ( bool )false; // Flag indicating reverse copy required
               lCnt     ReadCount;                    // Number of byte to move up to READ_WRITE_BUFFER_SIZE

               DstAddr      = getThreeByteAddr( &patchCmd_.copy.destAddr[0] );
               SrcAddr      = getThreeByteAddr( &patchCmd_.copy.srcAddr[0] ); /* get source address */
               Length       = EXTRACT_LEN( patchCmd_.copy.length );           /* Retrieve length */
               patchLen    -= sizeof( macroCmdCopy_t );              /* Adjust patch length and point to next command */
               PatchOffset += sizeof( macroCmdCopy_t );
               ReadCount    = sizeof( Buffer );                               /* Limit read to available buffer space */
               // Check if copy needs to be executed backward
               if ( ( DstAddr > SrcAddr ) && ( DstAddr < ( SrcAddr + Length ) ) )
               {
                  // Adjust address pointers to point to the end of the block to copy from
                  DstAddr     += Length;
                  SrcAddr     += Length;
                  bReverseCopy = ( bool )true;
               }
               while ( Length ) // Copy block of data
               {
                  if ( ReadCount > Length ) /* Read as many bytes as possible to fill the buffer */
                  {
                     /* Entire request fits in buffer */
                     ReadCount = Length;
                  }
                  if ( bReverseCopy )   // Point to next chunk of data to copy
                  {
                     DstAddr -= ReadCount;
                     SrcAddr -= ReadCount;
                  }
                  /* Only execute the write command if the state matches the command. */
                  if ( ( eDFWP_EXEC_PGM_COMMANDS == ePatchState ) && ( eDFWA_CMD_COPY_DATA == eCommand ) )
                  {
                     /* Read data from image partition and save it back to image partition */
                     readWriteBuffer( DstAddr, SrcAddr, Buffer, ReadCount, pDFWImagePTbl_, pDFWImagePTbl_ );
                  }
                  else if ( ( eDFWP_EXEC_NV_COMMANDS == ePatchState ) && ( eDFWA_CMD_COPY_DATA_NV == eCommand ) )
                  {
                     /* Read data from image partition and save it back to image partition */
                     readWriteBuffer( DstAddr, SrcAddr, Buffer, ReadCount, pDFWImageNvPTbl_, pDFWImageNvPTbl_ );
                  }
                  if ( !bReverseCopy )  // Point to next chunk of data to copy
                  {
                     SrcAddr += ReadCount;
                     DstAddr += ReadCount;
                  }
                  Length -= ReadCount;
               }  // End -  while (Length)
               break;
            }  // End - case eDFWA_CMD_COPY_DATA:

            default:
            {
               /* Invalid, this should never happen! */
               eErrorCode_ = eDFWE_CMD_CODE;
               break;
            }
         } // End switch.
      } // End if (...PAR_partitionFptr.parRead()...
      else /* Data wasn't read correctly, so set error condition */
      {
         eErrorCode_ = eDFWE_NV;
      }
   } // End - while ((eDFWE_NO_ERRORS == eErrorCode_) && (patchLen > 0))
   return( eErrorCode_ );
}

/***********************************************************************************************************************

   Function name: getThreeByteAddr

   Purpose: Returns the 3-byte address of the content of the pointer passed in.  The native endianess is returned.  By
   passing in the eState, the address (phatom byte) for PIC24 can be accounted for.

   Arguments: uint8 *pAddr

   Returns: dSize - Address

   Side Effects: None

   Reentrant Code: Yes

   Notes:

**********************************************************************************************************************/
static dSize getThreeByteAddr( uint8_t const *pAddr )
{
   return( ( dSize )pAddr[0] << 16 | ( dSize )pAddr[1] <<  8 | ( dSize )pAddr[2] );
}

/***********************************************************************************************************************

   Function name: DFWA_CopyPgmToPgmImagePartition

   Purpose: Copy PGM code to image partition.

   Arguments: keys - 0 = do not copy encryption keys, !0 = copy encryption keys

   Returns: returnStatus_t

   Side Effects: None

   Reentrant Code: No

   Notes: Tested on 12/11/2013, copies all Bank 0 to Bank 1 successfully.

**********************************************************************************************************************/
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
/*lint -esym(715, target) not referenced */
#else
/*lint -esym(715,copyKeySect) */
#endif
returnStatus_t DFWA_CopyPgmToPgmImagePartition( uint8_t copyKeySect, eFwTarget_t target )
{
   returnStatus_t retVal = eFAILURE;
   uint32_t       i;             /* Loop counter */

   PWR_CheckPowerDown();   /* Check for power down before processing. */

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
   uint32_t       bytes;         /* Bytes to copy */

   ( void )DFWA_SetSwapState( eFS_INIT );
   ( void )PAR_partitionFptr.parErase( 0, PART_PGM_FLASH_BANK_SIZE, pDFWImagePTbl_ ); // Erase entire PGM image partition
   CLRWDT();
   bytes = PART_PGM_FLASH_CODE_SIZE;
   if ( copyKeySect )
   {
      bytes += PART_ENCRYPT_KEY_SIZE;
   }

   // Copy Code (and optionally the Encrypt Keys)
   for ( i = 0; i < bytes; i += INT_FLASH_ERASE_SIZE )
   {
      if ( eSUCCESS != PAR_partitionFptr.parWrite( i, ( uint8_t * )i, ( lCnt )INT_FLASH_ERASE_SIZE, pDFWImagePTbl_ ) )
      {
         DFW_PRNT_ERROR_PARMS( "Copy Pgm %ld", i ); //Remove!
         eErrorCode_ = eDFWE_FLASH;
      }
      CLRWDT();
   }
#else
   PartSection_t const *pSections;
   uint8_t              sections;
   uint8_t              j;

   CLRWDT();
   /* Default to an Application update */
   pSections = &AppPartSections[0];
   sections  = ARRAY_IDX_CNT( AppPartSections );
   if ( eFWT_BL == target )
   {
      pSections = &BLPartSections[0];
      sections  = ARRAY_IDX_CNT( BLPartSections );
   }
   for ( j = 0; ( j < sections ) && ( eDFWE_FLASH != eErrorCode_ ); j++ )
   {
      // No need to pre-erase entire PGM image partition since will be updating an entire sector in one write.
      // Copy starting at partition address to ExtNV PGM Image Partition at same address for the partition size
      // Internal Flash Addresses are at the same offset in the external NV PGM Image partition
      for ( i = pSections->start; i < pSections->end; i += EXT_FLASH_ERASE_SIZE )
      {
         if ( eSUCCESS != PAR_partitionFptr.parWrite( i, ( uint8_t * )i, ( lCnt )EXT_FLASH_ERASE_SIZE, pDFWImagePTbl_ ) )
         {
            DFW_PRNT_ERROR_PARMS( "Copy Pgm %ld", i ); //Remove!
            eErrorCode_ = eDFWE_FLASH;
            break;
         }
         CLRWDT();
      }
      pSections++;
   }
#endif

   if ( eDFWE_NO_ERRORS == eErrorCode_ )
   {
      retVal = eSUCCESS;
   }

   return( retVal );
}

/***********************************************************************************************************************

   Function name: copyNvData

   Purpose: Copy registers data to/from patch partition.

   Arguments: eCopyDir_t eDirection - Copy direction: to/from NV

   Returns: eDfwErrorCodes_t

   Side Effects: If an error occurs, eErrorCode_ is set to eDFWE_NV.

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static eDfwErrorCodes_t copyNvData( eCopyDir_t eDirection )
{
   uint8_t                 Buffer[READ_WRITE_BUFFER_SIZE]; // Holds data between read/write
   uint32_t                i;                              // Loop counter
   lCnt                    Size;                           // # of bytes to copy (i.e. Registers data partition size)
   lCnt                    ReadCount;                      // Maximum number of bytes that can be copied
   PartitionData_t const   *pSrc;                          // Pointer to source partition - Read
   PartitionData_t const   *pDest;                         // Pointer to destination partition - Write
   returnStatus_t          retStatus = eSUCCESS;           // Error code

   if ( eCPY_NV_TO_IMAGE == eDirection ) /* Setup source and destination partition address */
   {
      /* Erase the partition before copying data to that partition (for endurance reasons) */
      ( void )PAR_partitionFptr.parErase( 0, PART_NV_DFW_NV_IMAGE_SIZE, pDFWImageNvPTbl_ ); // Erase NV Data image partition
      pSrc  = NULL; // This will instruct the device manager to return a contiguous data partition.
      pDest = pDFWImageNvPTbl_;
   }
   else
   {
      pSrc  = pDFWImageNvPTbl_;
      pDest = NULL; // This will instruct the device manager to split and write back the data to their specific partition.
   }
   ( void )PAR_partitionFptr.parSize( ePART_ALL_APP, &Size ); /* Get UN-NAMED partition size */
   ReadCount = sizeof( Buffer );                            /* Limit read to available buffer space */

   /* Copy register data to/from NV in chunk of READ_WRITE_BUFFER_SIZE */
   for ( i = 0; ( i < Size ) && ( eSUCCESS == retStatus ); i += READ_WRITE_BUFFER_SIZE )
   {
      /* Read as many bytes as possible to fill the buffer */
      dfwUnitTestToggleOutput_TST4( 100, 2, 3 );
      if ( ( Size - i ) <= READ_WRITE_BUFFER_SIZE )
      {
         ReadCount = Size - i;   /* Entire request fits in buffer */
      }
      CLRWDT();
      retStatus = PAR_partitionFptr.parRead( Buffer, i, ReadCount, pSrc ); /* Read a block of data */
      if ( eSUCCESS == retStatus )
      {
         PWR_CheckPowerDown(); /* Check for power down before processing. */
         retStatus = PAR_partitionFptr.parWrite( i, Buffer, ReadCount, pDest ); /* Write it back */
      }
   }
   if ( eSUCCESS != retStatus )
   {
      eErrorCode_ = eDFWE_NV;
   }
   return ( eErrorCode_ );
}

/***********************************************************************************************************************

   Function name: readWriteBuffer

   Purpose: Read data from a partition and write back to same or different partition.

   Arguments:  dSize            DstAddr       - Destination address
   dSize            SrcAddr       - Source address
   uint8_t         *Buffer        - Buffer used to hold data for read/write
   lCnt             Cnt           - Number of bytes to read/write
   PartitionData_t *pDstPartition - Destination partition
   PartitionData_t *pSrcPartition - Source partition

   Returns: none

   Side Effects: If an error occurs, eErrorCode_ is set to eDFWE_NV.

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static void readWriteBuffer( dSize DstAddr, dSize SrcAddr, uint8_t *Buffer, lCnt Cnt,
                             const PartitionData_t *pDstPartition, const PartitionData_t *pSrcPartition )
{
   returnStatus_t retStatus = PAR_partitionFptr.parRead( Buffer, SrcAddr, Cnt, pSrcPartition ); /* Read data */
   if ( eSUCCESS == retStatus )
   {
      PWR_CheckPowerDown(); /* Check for power down before processing. */
      /* Save buffer to NV to patch a section. */
      retStatus = PAR_partitionFptr.parWrite( DstAddr, Buffer, Cnt, pDstPartition );
      CLRWDT();
   }
   if ( eSUCCESS != retStatus )
   {
      eErrorCode_ = eDFWE_NV;
   }
}

/***********************************************************************************************************************

   Function Name: calcCrc

   Purpose: Calculate the CRC32 value across a given partition.

   Arguments:  flAddr StartAddr,
   uint32_t Length,
   uint8_t  start,        true if initial CRC, false if continuing a previous CRC
   uint32_t *resultCRC,
   PartitionData_t const * pPartition

   Returns: returnStatus_t - defined by error_codes.h

   Note: An error is returned if the partition could not be read or the crc32 fails(?).

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
static returnStatus_t calcCRC( flAddr StartAddr, uint32_t cnt, bool start, uint32_t * const pResultCRC,
                               PartitionData_t const *pPartition )
{
   returnStatus_t retVal = eSUCCESS;   /* return value */
   crc32Cfg_t     crcCfg;              /* CRC configuration - Don't change elements in this structure!!! */
   uint32_t       operationCnt;        /* Number of bytes to operate on (read and perform CRC32) per pass of loop */
   uint8_t        buffer[512];          /* Buffer to store the read data from partition x */
   flAddr         partitionOffset = StartAddr; /* Start CRC32 at the beginning of the partition */

   if ( start )
   {
      *pResultCRC = CRC32_DFW_START_VALUE;
   }
   CRC32_init( *pResultCRC, CRC32_DFW_POLY, eCRC32_RESULT_INVERT, &crcCfg ); /* Init CRC */

   while( 0 != cnt )
   {
      operationCnt = sizeof( buffer ); /* The current operation will operate on the size of buffer */
      if ( operationCnt > cnt )        /* Are the remaining bytes less than the buffer size? */
      {
         /* Yes, only operate on cnt bytes */
         operationCnt = cnt;
      }
      if ( eSUCCESS != PAR_partitionFptr.parRead( &buffer[0], partitionOffset, operationCnt, pPartition ) ) /* Rd x bytes */
      {
         retVal = eFAILURE;
         break;
      }
      *pResultCRC      = CRC32_calc( &buffer[0], operationCnt, &crcCfg ); /* Update the CRC */
      cnt             -= operationCnt;                                  /* Adjust count */
      partitionOffset += operationCnt;                                  /* Adjust the offset into the partition */
   }
   return( retVal );
}

/***********************************************************************************************************************

   Function Name: verifyCrc

   Purpose: Calculate and compare the CRC32 value across a given partition.

   Arguments:  flAddr StartAddr,
   uint32_t Length,
   uint32_t expectedCRC,
   PartitionData_t const * pPartition

   Returns: returnStatus_t - defined by error_codes.h

   Note: An error is returned if the partition could not be read or the crc32 doesn't match.

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
static returnStatus_t verifyCrc( flAddr StartAddr, uint32_t cnt, uint32_t expectedCRC,
                                 PartitionData_t const *pPartition )
{
   returnStatus_t retVal = eSUCCESS;   /* return value */
   uint32_t       crc = 0;             /* Current CRC32 value */

   if ( eSUCCESS != calcCRC( StartAddr, cnt, ( bool )true, &crc, pPartition ) )
   {
      retVal = eFAILURE;
   }
   else if ( expectedCRC != crc )                            /* Does the expected CRC match the calculated CRC? */
   {
      DFW_PRNT_ERROR_PARMS( "CRC32 - Expected: 0x%08lX vs 0x%08lX", expectedCRC, crc );
      retVal = eFAILURE;                                    /* CRC mismatch */
   }
   return( retVal );
}

/***********************************************************************************************************************

   Function Name: DFWA_SetSwapState

   Purpose: Calls the internal flash driver and sets the swap state

   Arguments: eFS_command_t swapCmd - see IF_intFlash.h file.

   Returns: returnStatus_t - defined by error_codes.h

   Note:

   Side Effects: None

   Reentrant Code: No

**********************************************************************************************************************/
returnStatus_t DFWA_SetSwapState( eFS_command_t swapCmd )
{
   returnStatus_t    retVal;        /* Value returned as the result of the set state */
   IF_ioctlCmdFmt_t  ioctlCmd;      /* Command to send to the driver */
   flashSwapStatus_t ioctlStatus;   /* Result of the command sent to the driver */

   /* Need to read the state of the driver so that the correct operation is performed. */
   ioctlCmd.ioctlCmd = eIF_IOCTL_CMD_FLASH_SWAP;   /* Set IOCTL command */
   ioctlCmd.fsCmd    = eFS_GET_STATUS;             /* Get command to read the status */
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
   retVal            = PAR_partitionFptr.parIoctl( &ioctlCmd, &ioctlStatus, pDFWImagePTbl_ ); /* Read the status */
#else
   retVal            = PAR_partitionFptr.parIoctl( &ioctlCmd, &ioctlStatus, pAppCodePart_ ); /* Read the status */
#endif
   if ( eSUCCESS == retVal ) /* Was the status read correctly? */
   {
      /* If the command requested was get status, we just did that, so skip the next section */
      if ( eFS_GET_STATUS != swapCmd ) /* Do something other than read status? */
      {
         if ( ( eFS_COMPLETE == swapCmd ) && ( eFS_MODE_UPDATE_ERASED != ioctlStatus.currentMode ) )
         {
            retVal = eFAILURE;
            DFW_PRNT_ERROR( "Swap Cmd - Complete while invalid state" );
         }
         else
         {
            /* If the command was initialize and we're in the ready state, change the command to update */
            if ( ( eFS_INIT == swapCmd ) && ( eFS_MODE_READY == ioctlStatus.currentMode ) )
            {
               swapCmd = eFS_UPDATE;
            }
            ioctlCmd.fsCmd = swapCmd;  /* Set the IOCTL flash command to the value passed in */
            dfwUnitTestToggleOutput_TST2( 500, 2, 3 );
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
            retVal = PAR_partitionFptr.parIoctl( &ioctlCmd, &ioctlStatus, pDFWImagePTbl_ );
#else
            retVal = PAR_partitionFptr.parIoctl( &ioctlCmd, &ioctlStatus, pAppCodePart_ );
#endif
            dfwUnitTestToggleOutput_TST2( 500, 3, 3 );
            /* If the expected state isn't correct, return an error. */
            if (  ( ( eFS_INIT == swapCmd ) && ( eFS_MODE_UPDATE_ERASED != ioctlStatus.currentMode ) ) ||
                  ( ( eFS_UPDATE == swapCmd ) && ( eFS_MODE_UPDATE != ioctlStatus.currentMode ) )      ||
                  ( ( eFS_COMPLETE == swapCmd ) && ( eFS_MODE_COMPLETE != ioctlStatus.currentMode ) )     )
            {
               retVal = eFAILURE;
               DFW_PRNT_ERROR( "Swap Cmd" );
            }
         }
      }
   }
   DFW_PRNT_INFO_PARMS( "Swap Info: Cmd: %d, Mode: %d, Block: %d, Next Block: %d",  ( uint8_t )swapCmd,
                        ( uint8_t )ioctlStatus.currentMode, ( uint8_t )ioctlStatus.currentBlock,
                        ( uint8_t )ioctlStatus.nextBlock );
   return( retVal );
}

/***********************************************************************************************************************

   Function Name: DFWA_WaitForSysIdle

   Purpose:   Waits for the system to become idle or until the time has expired.

   Arguments: uint16_t waitTimeMs - Time to wait before returning, resolution is actually 10mS.

   Returns:   eSUCCESS if sys idle and mutex locked, eFAILURE if timed out.

   Note:      maxWaitTimeMs could be zero.

   Side Effects: If idle the sysbusy mutex will be locked.  It must be unlocked or execute an unconditional reset.

   Reentrant Code: No

**********************************************************************************************************************/
returnStatus_t DFWA_WaitForSysIdle( uint32_t waitTimeMs ) //TODO: 2016-01-28[SMG] Move to SYSBUSY.c?
{
   uint32_t       cnt    = waitTimeMs / TEN_MSEC; /* Timeout count to wait for system idle */
   returnStatus_t retVal = eSUCCESS;

   if ( eSYS_IDLE != SYSBUSY_lockIfIdle() )
   {
      retVal = eFAILURE;
      // Mutexes do not have wait for timeouts, so loop with delay until idle or timeout expires
      while ( 0 != cnt )
      {
         OS_TASK_Sleep( TEN_MSEC ); /* Sleep while we wait for the system to be idle. */
         if ( eSYS_IDLE == SYSBUSY_lockIfIdle() )
         {
            retVal = eSUCCESS;
            break;   //System Idle and Mutex locked
         }
         cnt--;
      }
   }
   return ( retVal );
}

/***********************************************************************************************************************

   Function Name: DFWA_UnlockSysIdleMutex

   Purpose: Unlocks the mutex potentially set by DFWA_WaitForSysIdle function.

   Arguments: void none

   Returns: None

   Note:

   Side Effects: None.

   Reentrant Code: No

**********************************************************************************************************************/
void DFWA_UnlockSysIdleMutex( void ) //TODO: 2016-01-28[SMG] Move to SYSBUSY.c?
{
   SYSBUSY_unlock();
}

/***********************************************************************************************************************

   Function name: brick

   Purpose: Endless loop that 'bricks' the unit.  If an unrecoverable error occurs, this is where we should end up.

   Arguments: None

   Returns: None

   Side Effects: Never returns

   Reentrant Code: No

   Notes: Must be called prior to RTOS tasks started (i.e. during initialization)

**********************************************************************************************************************/
static void brick( void )
{
   for ( ; ; )
   {
      CLRWDT();   /* Sit here forever! */
   }
}

/***********************************************************************************************************************

   Function name: createPemFromExtractedKey

   Purpose: This function creates a PEM file from a signature of 64 bytes. If the MSB of the first byte of the key
   is set then add a 0x00 byte before the key and adjust the file.

   Arguments:  const uint8_t *extractedKey  - The 64 byte key signature
   uint8_t *pem                 - Pinter to the PEM stucture that will be filled
   uint8_t *pemSz               - Returns the size of the PEM created.

   Returns: None

   Side Effects: None

   Reentrant Code: No

   Notes: None

**********************************************************************************************************************/
static void createPemFromExtractedKey( const uint8_t *extractedKey, uint8_t *pem, uint8_t *pemSz )
{
   uint8_t offset = PEM_KEY_DATA_START;       /* The offset into the PEM structure to write */
   uint8_t keyLength = PEM_KEY_DATA_START;    /* Length of the PEM key data */

   /* Set up the first tag identifier of a PEM file */
   pem[0] = PEM_TYPE_TAG_SEQUENCE;

   /* Set the tag for the key as INTEGER */
   pem[2] = PEM_TYPE_TAG_INTEGER;

   /* Check the MSB bit of the first key if set then insert a byte before the key */
   if ( extractedKey[0] & PEM_MSB_MASK )
   {
      pem[offset++] = PEM_EXTRA_BYTE;
      pem[3]        = PEM_33_BYTES_IN_KEY;
      keyLength    += PEM_33_BYTES_IN_KEY;
   }
   else
   {
      pem[3]     = PEM_32_BYTES_IN_KEY;
      keyLength += PEM_32_BYTES_IN_KEY;
   }
   ( void )memcpy( &pem[offset], &extractedKey[0], PEM_KEY_SIZE );
   offset += PEM_KEY_SIZE;

   /* Check the second key for the MSB bit and if it set insert a byte before the key. */
   pem[offset++] = PEM_TYPE_TAG_INTEGER;

   if ( extractedKey[32] & PEM_MSB_MASK )
   {
      pem[offset++] = PEM_33_BYTES_IN_KEY;
      pem[offset++] = PEM_EXTRA_BYTE;
      keyLength    += PEM_33_BYTES_IN_KEY;
   }
   else
   {
      pem[offset++] = PEM_32_BYTES_IN_KEY;
      keyLength    += PEM_32_BYTES_IN_KEY;

   }
   ( void )memcpy( &pem[offset], &extractedKey[32], PEM_KEY_SIZE );

   /* This holds the size of the KEY data not the whole PEM structure. */
   pem[1] = keyLength;

   /* Return the PEM size */
   *pemSz = keyLength + PEM_OVERHEAD;
}

/***********************************************************************************************************************

   Function name: DFWA_setDerivedKey

   Purpose: This function is called to use the internal data plus the "TO VER" to create the derived key
   which is used to decode the AES encrypted bytes.

   Arguments:  firmwareVersion_u  - the "FROM VER" contained in the DFW download Init.
   firmwareVersion_u  - the "TO VER"   contained in the DFW download Init.

   Returns: returnStatus_t       - returns eSUCCESS when the key is retrieved otherwise eFAILURE

   Side Effects: None

   Reentrant Code: No

   Notes: This call will reset the aesCounter and should be called once when the patch is completely downloaded.

**********************************************************************************************************************/
returnStatus_t DFWA_setDerivedKey( firmwareVersion_u fromVersion, firmwareVersion_u toVersion )
{
   uint8_t        encryptionNumber[ECC108_KEY_SIZE];
   const char    *dfwComDevice;
   uint8_t        offset      = 0;
   returnStatus_t returnValue = eSUCCESS;

   ( void )memset( encryptionNumber, 0x00, sizeof( encryptionNumber ) );

   dfwComDevice = VER_getComDeviceType();
   ( void )strncpy( ( char * )encryptionNumber, dfwComDevice, COM_DEVICE_TYPE_LEN );
   offset      += COM_DEVICE_TYPE_LEN;

   encryptionNumber[offset]   = fromVersion.field.version;
   encryptionNumber[offset + 1] = fromVersion.field.revision;
   encryptionNumber[offset + 2] = ( uint8_t )( ( fromVersion.field.build & 0xFF00 ) >> 8 );
   encryptionNumber[offset + 3] = ( uint8_t ) ( fromVersion.field.build & 0x00FF );
   offset                    += ( uint8_t )sizeof( fromVersion );

   encryptionNumber[offset]   = toVersion.field.version;
   encryptionNumber[offset + 1] = toVersion.field.revision;
   encryptionNumber[offset + 2] = ( uint8_t )( ( toVersion.field.build & 0xFF00 ) >> 8 );
   encryptionNumber[offset + 3] = ( uint8_t ) ( toVersion.field.build & 0x00FF );

   if ( ECC108_SUCCESS != ecc108e_DeriveFWDecryptionKey( ( uint8_t * )encryptionNumber, derivedKey_ ) )
   {
      ERR_printf( "Failed to get the AES decryption key" );
      returnValue = eFAILURE;
   }
   return returnValue;
}

/***********************************************************************************************************************

   Function name: decryptData

   Purpose: This function is called to decrypt an AES-CCM with MAC16 counter encrypted packet of bytes. It will decrypt the
   the bytes and call a supplied callback function. That function will take the decrypted bytes and write them
   back to the NV-RAM. It will overwrite the patch.

   Arguments:  const uint8_t *pPartition - Handle to the NVRAM partition
   flAddr StartAddr          - Offset into NVRAM where the encoded bytes start
   flAddr DecoderStart       - Offset into NVRAM where the decoded byste start
   const uint8_t *autTag     - Authorization Hash Tag used to validate the patch
   uint32_t authTagSz        - Size of the Authorization Tag.
   uint32_t patchLen         - (Patch Length - Unencrypted Header - Authtag size - signature size)
   DFWA_WritePatchCallback_t callBack - Funtion to callback with the decoded bytes.

   Returns: returnStatus_t        - returns eSUCCESS when bytes are decrypted otherwise eFAILURE

   Side Effects: None

   Reentrant Code: No

   Notes:   This function parses the patch in 32 byte chunks and eventually decodes the whole patch. Then it runs
   the decoded patch through the authorization process to ensure its valid.

**********************************************************************************************************************/
static returnStatus_t decryptData( PartitionData_t const *pPartition, flAddr StartAddr, flAddr DecodedStart, const uint8_t *authTag,
                                   uint32_t authTagSz, uint32_t patchLen )
{
   returnStatus_t  returnValue;                  /* Return status from function call. */
   byte            cipher[DFW_AEC_CHUNK_SIZE];   /* Chunk of encoded data */
   byte            decoded[DFW_AEC_CHUNK_SIZE];  /* Chunk of decoded data */
   Aes             decoder;                      /* Instance of the decoder */
   uint32_t        loops;                        /* Number of even 32 byte chunks to process the whole patch */
   uint32_t        i;                            /* Loop index */
   uint32_t        bytesLeft;                    /* The number of bytes left to process */
   uint32_t        readLen;                      /* Number of bytes to read from the NVRAM */
   byte            lastCall;                     /* On the last call you need to verify the patch */
   flAddr          eraseAddr;                    /* Address witin sector to be erased */

   bytesLeft = patchLen;
   readLen   = 0;
   lastCall  = ( byte )false;

   static const byte nonce[] =
   {
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00
   }; /* The NONCE is always 0 */

   /***************************************************************************
      Determine the number of 32byte chunks to process. Account for the last
      32 byte chunk not being full.
   **************************************************************************/
   loops = patchLen / DFW_AEC_CHUNK_SIZE;
   if ( patchLen % DFW_AEC_CHUNK_SIZE != 0 )
   {
      loops++;
   }

   /* Set up the CCM decoder */
   ( void )wc_AesCcmSetKey( &decoder, derivedKey_, ECC108_KEY_SIZE );

   /* Decode all the bytes */
   for ( i = 0; i < loops; i++ )
   {
      /* Check for 32 byte chunks if not 32 bytes send whats left */
      if ( bytesLeft > DFW_AEC_CHUNK_SIZE )
      {
         readLen = DFW_AEC_CHUNK_SIZE;
      }
      else
      {
         readLen  = bytesLeft;
         lastCall = ( byte )true;
      }
      /* Read in the byte chunk */
      //read from pPartition, readLen bytes at StartAddr, write to cipher buffer
      returnValue = PAR_partitionFptr.parRead( ( uint8_t* )cipher, ( dSize )StartAddr, ( lCnt )readLen, pPartition );
      if ( returnValue != eSUCCESS )
      {
         ERR_printf( "DFW failed to read NV while decrypting" );
         return eFAILURE;
      }
      //If this chunk read starts at or crosses into a new sector then erase the appropriate sector
      //NOTE: Requires the partition to start on a sector boundary (also required by all other partitions)
      eraseAddr = StartAddr;    //Case where reading across sectors (Start is in the previous sector)
      if ( 0 == eraseAddr % EXT_FLASH_ERASE_SIZE )
      {
         //Case where starting at the first address of sector
         eraseAddr -= EXT_FLASH_ERASE_SIZE;
         ( void )PAR_partitionFptr.parErase( eraseAddr, EXT_FLASH_ERASE_SIZE, pPartition ); // Erase previous sector
      }
      else if( EXT_FLASH_ERASE_SIZE <= ( eraseAddr % EXT_FLASH_ERASE_SIZE ) + readLen ) //Start address is in the sector to erase, but current chuck crosses into next sector
      {
         //round down to first address of the sector to erase
         eraseAddr -= eraseAddr % EXT_FLASH_ERASE_SIZE;
         ( void )PAR_partitionFptr.parErase( eraseAddr, EXT_FLASH_ERASE_SIZE, pPartition ); // Erase previous sector
      }

      ( void )memset( decoded, 0x00, sizeof( decoded ) ); //TODO: Is this necessary?
      /*************************************************************************
         The first time through the decoder run the initialization. Otherwise
         process the chunk. Also if this is the last chunk of data you must
         indicate it, so the decoder can ready itself for the validation.
      ***********************************************************************/
      if ( i == 0 )
      {
         wc_AesCcmDecryptNoValidate( &decoder, decoded, cipher, readLen, nonce, sizeof( nonce ), authTag, authTagSz, NULL, 0, patchLen, ( byte )true, lastCall );
      }
      else
      {
         wc_AesCcmDecryptNoValidate( &decoder, decoded, cipher, readLen, nonce, sizeof( nonce ), authTag, authTagSz, NULL, 0, patchLen, ( byte )false, lastCall );
      }
      // Write the decrypted bytes back to NV one sector lower address
      ( void )PAR_partitionFptr.parWrite( ( StartAddr - EXT_FLASH_ERASE_SIZE ), &decoded[0], ( lCnt )readLen, pPartition );
      bytesLeft -= readLen;
      StartAddr += readLen;
   }
   //TODO: Can the Authorization check be performed as the decryption takes place - saves a lot of NV reads...
   /* Reset all the pointers to start the validation process */
   bytesLeft = patchLen;
   lastCall  = ( byte )false;

   for ( i = 0; i < loops; i++ )
   {
      if ( bytesLeft > DFW_AEC_CHUNK_SIZE )
      {
         readLen = DFW_AEC_CHUNK_SIZE;
      }
      else
      {
         readLen = bytesLeft;
         lastCall = ( byte )true;
      }
      returnValue = PAR_partitionFptr.parRead( ( uint8_t* )decoded, ( dSize )DecodedStart, ( lCnt )readLen, pPartition );
      if ( returnValue != eSUCCESS )
      {
         ERR_printf( "DFW failed to read NV while authorizing" );
         return eFAILURE;
      }

      /**************************************************************************
         After all the bytes have been processed compare them to make sure the
         that the sum is equal to the Authorization tag.
      ************************************************************************/
      if ( lastCall )
      {
         returnValue = ( returnStatus_t )wc_AesCcmVerifyDecrypt( &decoder, decoded, readLen, authTag, authTagSz, sizeof( nonce ), ( byte )true );
         if ( returnValue != eSUCCESS )
         {
            //TODO: Should the Patch space be erased since we failed to decrypt and/or authenticate? If so make function in dfw_intrface available or move it to here and make it available to dfw_interface
            ERR_printf( "Patch Verification failed" );
            return eFAILURE;
         }
      }
      else
      {
         ( void )wc_AesCcmVerifyDecrypt( &decoder, decoded, readLen, authTag, authTagSz, sizeof( nonce ), ( byte )false );
      }
      bytesLeft    -= readLen;
      DecodedStart += readLen;
   }
   return eSUCCESS;
}

/***********************************************************************************************************************

   Function name: verifySignature

   Purpose: This function is called to verfify the signature of the patch file.

   Arguments:  decryptStatus - state of the decryption process
   pPartition    - pointer to the NVRAM instance.
   StartAddr     - Offset into flash partition.
   cnt           - number of bytes in the patch (excluding the Signature size)

   Returns: returnValue     - returns eSUCCESS when the file is verified otherwise eFAILURE

   Side Effects: None

   Reentrant Code: No

   Notes: This uses SHA-256

**********************************************************************************************************************/
static returnStatus_t verifySignature( eDfwDecryptState_t decryptStatus,
                                       PartitionData_t const *pPartition, flAddr StartAddr, uint32_t cnt )
{
   int32_t        valid;
   ecc_key        key;
   Sha256         sha;                          /* Handle to the wolfssl SHA decoder */
   uint8_t        hash[SHA256_DIGEST_SIZE];     /* Computed hash 256 */
   uint8_t        inbuf[SIG_PACKET_SIZE];
   uint8_t        pem[PEM_MAX_SIZE];
   uint32_t       totalLoops;
   returnStatus_t returnValue;                  /* Return value */
   uint32_t       i;
   uint32_t       partitionOffset = StartAddr;  /* Offset into the NVRAM */
   uint32_t       bytesLeft       = cnt;        /* Counter to the number of bytes left */
   uint32_t       readLen;                      /* How many bytes to compute */
   uint8_t        pemSz;

#if (SIG_PACKET_SIZE < DFW_SHA_CHUNK_SIZE)
#error "DFWA input buffer size exceeds Signature size"
#endif

   returnValue = eSUCCESS; //If not encrypted, then decryption is successful
   if ( eDFWD_READY == decryptStatus )
   {
      returnValue = eFAILURE; //Assume failure
      /* Make sure that the flash is set up */
      if ( NULL != pPartition )
      {
         /* Calculate the number of loops it will take at 32 byte chunks to compute the hash */
         totalLoops = bytesLeft / DFW_SHA_CHUNK_SIZE;
         if ( bytesLeft % DFW_SHA_CHUNK_SIZE )
         {
            totalLoops++;
         }
         /* Initialize the SHA256 decoder */
         if ( 0 == wc_InitSha256( &sha ) )
         {
            /* Compute the SHA25 hash */
            for ( i = 0; i < totalLoops; i++ )
            {
               if ( bytesLeft > DFW_SHA_CHUNK_SIZE )
               {
                  readLen = DFW_SHA_CHUNK_SIZE;
               }
               else
               {
                  readLen = bytesLeft;
               }
               if ( eSUCCESS != PAR_partitionFptr.parRead( &inbuf[0], ( dSize )partitionOffset, ( lCnt )readLen, pPartition ) )
               {
                  ERR_printf( "DFW failed to read NV for hashing" );
                  break;// Assume will fail sugnature below
               }
               // Must clear out the Patch CRC which is in the first chunk
               if ( 0 == i )
               {
                  *( uint32_t * )( inbuf + PATCH_CRC_OFFSET ) = ( uint32_t )0; //lint !e826  Ptr to CRC data
               }
               ( void )wc_Sha256Update( &sha, &inbuf[0], readLen );
               partitionOffset += readLen;
               bytesLeft       -= readLen;
            }
            /*************************************************************************
               After all the bytes have been run through the SHA decoder, compare them
               to the expected hash.
            ************************************************************************/
            ( void )wc_Sha256Final( &sha, &hash[0] );
            ( void )wc_ecc_init( &key );
            /* Import the public key into an ecc key */
            if ( 0 == wc_ecc_import_x963( AclaraDFW_sigKey, sizeof( AclaraDFW_sigKey ), &key ) )
            {
               //Read the patch signature
               if ( eSUCCESS == PAR_partitionFptr.parRead( &inbuf[0], ( dSize )partitionOffset, ( lCnt )SIG_PACKET_SIZE, pPartition ) )
               {
                  ( void )memset( pem, 0x00, sizeof( pem ) );
                  /* Convert the signature to PEM format which wolfssl uses to validate the signature */
                  createPemFromExtractedKey( &inbuf[0], pem, &pemSz );
                  ( void )wc_ecc_verify_hash( &pem[0], pemSz, &hash[0], sizeof( hash ), &valid, &key );
                  if ( valid )
                  {
                     INFO_printf( "Signature Verified" );
                     returnValue = eSUCCESS;
                  }
                  else
                  {
                     ERR_printf( "Failed to validate signature" );
                  }
               }
               else
               {
                  ERR_printf( "DFW failed to read Signature" );
               }
            }
            else
            {
               ERR_printf( "Failed to make key" );
            }
         }
         else
         {
            ERR_printf( "failed to init SHA" );
         }
      }
      else
      {
         ERR_printf( "Signature Partition pointer is NULL" );
      }
   }
   return ( returnValue );
}

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
#else
/***********************************************************************************************************************

   Function name: updateBootloader

   Purpose: This function performs the final actions when Patching the Bootloader.

   Arguments:

   Returns: returnValue     - returns eFAILURE if unsuccessful otherwise resets processor

   Side Effects: New BL or backup BL available, system resets if successful

   Reentrant Code: No

   Notes:   Assumes all patch updates are complete and verified
   Assumes Vectors and Bootloader start at address 0 and is contiguous

**********************************************************************************************************************/
static returnStatus_t updateBootloader( uint32_t crcBl )
{
   uint8_t                 Buffer[READ_WRITE_BUFFER_SIZE]; // Holds data between read/write
   returnStatus_t          retStatus = eFAILURE;           // Error code
   PartitionData_t const   *pBLCodePTbl;                   // Pointer to Bootloader code partition
   PartitionData_t const   *pBLBackupPTbl;                 // Pointer to Bootloader backup code partition
#if ( MCU_SELECTED == NXP_K24 )
   uint32_t                i;                              // Loop counter
   lCnt                    Size;                           // # of bytes to copy (i.e. Registers data partition size)
   lCnt                    ReadCount;                      // Maximum number of bytes that can be copied
   flAddr                  addrOffset;                     // BL_Backup Address offset from "real" BL image
   flAddr                  *pIntVect;                      // Pointer to interrupt vector entry
   uint8_t                 attempts;                       // Number of times to attempt updating the BL code partition
#elif ( MCU_SELECTED == RA6E1 )
   flash_startup_area_swap_t  bootStartupArea;             // Determines the startup area whether it is block 0 or block 1
   uint8_t                    bootFlg;                     // Flag to indicate the startup area - 0 or 1
   fsp_err_t                  errStatus;                   // RA6 FSP error status
#endif

   if (  ( eSUCCESS == PAR_partitionFptr.parOpen( &pBLCodePTbl,   ePART_BL_CODE,   0L ) ) &&
         ( eSUCCESS == PAR_partitionFptr.parOpen( &pBLBackupPTbl, ePART_BL_BACKUP, 0L ) ) )
   {
#if ( MCU_SELECTED == NXP_K24 )
      ( void )PAR_partitionFptr.parSize( ePART_BL_CODE, &Size );           // Get partition size
      pIntVect = ( flAddr * )( BL_VECTOR_TABLE_START + sizeof( flAddr ) ); // Skip the first vector (SP init)
      retStatus = eSUCCESS;

      /* Need to ensure the backup of the BL code is correct.
         Since the two code spaces are both in internal memory, a memcmp is the simplest and fasted method.
         One caveat is the vector table changes so it cannot be compared as-is.
         Compare vector table data as the modified values are written to the backup.
         Next Write the remaining BL to the backup.
         Compare as-is.
         The backup now contains the exact data needed to restore the vectors if the update BL fails to pass the CRC.
      */

      /* If reset vector is pointing within the Bootloader partition, then need to make a backup of the bootloader */
      if ( *pIntVect < Size )   //NOTE: This is problematic if the Bootloader image doesn't exist.
      {
         /* This is critical code so status checking is done even though it is copying internal to internal flash */
         /* Only writing a partial sector at a time so erasing the BL_Backup partition first maximizes enurance and
            minimizes write time */
         attempts   = 3;  //Three attempts to ensure not just a transient problem (i.e a real HW fault)
         do
         {
            retStatus = PAR_partitionFptr.parErase( 0, Size, pBLBackupPTbl );
            attempts--;
         }while ( attempts && ( eSUCCESS != retStatus )  );
         if ( eSUCCESS == retStatus )  //Update Backup BL vector space
         {
            attempts   = 3;  //Three attempts to ensure not just a transient problem (i.e a real HW fault)
            /* Get the difference between the BL and the backup BL addresses to add to the current vectors */
            addrOffset = pBLBackupPTbl->PhyStartingAddress - pBLCodePTbl->PhyStartingAddress;
            do
            {
               /* Update the INTVECT Table in the BL Backup */
               for ( i = 0; i < BL_VECTOR_TABLE_SIZE; i += sizeof( Buffer ) )
               {
                  uint16_t  vectIndex = 0;

                  /* Read lesser of available buffer size or data remaining */
                  ReadCount = ( i <= ( BL_VECTOR_TABLE_SIZE - sizeof( Buffer ) ) ? sizeof( Buffer ) : BL_VECTOR_TABLE_SIZE - i );
                  CLRWDT();
                  retStatus = PAR_partitionFptr.parRead( &Buffer[0], i, ReadCount, pBLCodePTbl );
                  if ( eSUCCESS == retStatus )
                  {
                     /* Modify vectors for backup location (unless it is zero or outside of BL space)
                        (SP init vector not modified since RAM is outside of BL ROM Space) */
                     while ( vectIndex < ReadCount )
                     {
                        if ( ( 0 != *( flAddr * )&Buffer[vectIndex] ) && ( *( flAddr * )&Buffer[vectIndex] < Size ) ) //lint !e826  Ptr to vector data
                        {
                           *( flAddr * )&Buffer[vectIndex] += addrOffset;  //lint !e826  Ptr to vector data
                        }
                        vectIndex += sizeof( flAddr );
                     }
                     PWR_CheckPowerDown( ); /* Check for power down before processing. */
                     /* Write vectors to BL Code partition */
                     retStatus = PAR_partitionFptr.parWrite( i, &Buffer[0], ReadCount, pBLBackupPTbl );
                     if ( eSUCCESS == retStatus )
                     {
                        if ( 0 != memcmp( ( uint8_t * )( pBLBackupPTbl->PhyStartingAddress + i ), &Buffer[0], ReadCount ) )
                        {
                           retStatus = eFAILURE;
                           break;
                        }
                     }
                     else
                     {
                        break;
                     }
                  }
                  else
                  {
                     break;
                  }
               }  //End of for ( i = 0; i < BL_VECTOR_TABLE_SIZE...
               attempts--;
            }while ( attempts && ( eSUCCESS != retStatus )  );
         }  //End of if ( eSUCCESS == retStatus )  //Update Backup BL vector space
         if ( eSUCCESS == retStatus )
         {
            /* Backup everything after the INTVECT sector */
            attempts = 3;  //Three attempts to ensure not just a transient problem (i.e a real HW fault)
            do
            {
               retStatus = copyPart2Part( BL_VECTOR_TABLE_SIZE, Size - BL_VECTOR_TABLE_SIZE, &Buffer[0], sizeof( Buffer ),
                                          pBLCodePTbl, pBLBackupPTbl );
               if ( eSUCCESS == retStatus )
               {
                  if ( 0 != memcmp( ( flAddr * )( pBLCodePTbl->PhyStartingAddress   + BL_VECTOR_TABLE_SIZE ),
                                    ( flAddr * )( pBLBackupPTbl->PhyStartingAddress + BL_VECTOR_TABLE_SIZE ),
                                    Size - BL_VECTOR_TABLE_SIZE ) )
                  {
                     retStatus = eFAILURE;
                  }
               }
               attempts--;
            }while ( attempts && ( eSUCCESS != retStatus )  );
         }
         if ( eSUCCESS == retStatus ) //Update vector table to use Backup BL
         {
            /****************************************************************************************************
               Very time critical section.  If a spurious reset occurs here the unit is a brick
            *****************************************************************************************************/
            /* Now write the updated Backup vector table back to the normal INTVECT */
            OS_INT_disable( ); //Disable interrupts to ensure no other task takes over (why would this matter since running from app vector table at different addresses?)
            attempts = 3;  //Three attempts to ensure not just a transient problem (i.e a real HW fault)
            do
            {
               /* Erase INTVEC sector to minimize time required to write (updated vectors are in the backup image).
                  Remaining bytes in sector being erased is OK.  It will boot to backup image until new
                  bootloader image is written. */
               retStatus = PAR_partitionFptr.parErase( BL_VECTOR_TABLE_START, INT_FLASH_ERASE_SIZE, pBLCodePTbl );
               if ( eSUCCESS == retStatus )
               {
                  retStatus = copyPart2Part( 0, BL_VECTOR_TABLE_SIZE, &Buffer[0], sizeof( Buffer ),
                                             pBLBackupPTbl, pBLCodePTbl );
                  OS_INT_enable( );   // Enable as soon as possible
                  if ( eSUCCESS == retStatus )
                  {
                     if ( 0 != memcmp( ( flAddr * )pBLBackupPTbl->PhyStartingAddress,
                                       ( flAddr * )pBLCodePTbl->PhyStartingAddress, BL_VECTOR_TABLE_SIZE ) )
                     {
                        retStatus = eFAILURE;
                     }
                  }
               }
               attempts--;
            }while ( attempts && ( eSUCCESS != retStatus )  );
            /* If this fails, then  a reboot is unpredicable!  On the other hand if we do not reboot, then the following
               process MAY resolve the problem.  Also really only need to get the write of the first two vectors since
               the bootloader does not use any of the remaining vectors (interrupts not enabled).
            */
            OS_INT_enable( );   // See OS_INT_disable( ); above
            retStatus = eSUCCESS;   // If exited above with failure this lets update BL process have a chance of working
            /****************************************************************************************************
               End of very time critical section.
            *****************************************************************************************************/
         }  // End of if ( eSUCCESS == retStatus ) //Update vector table to use Backup BL
      } // end of if ( *pIntVect < Size )

      if ( eSUCCESS == retStatus ) // Update the Bootloader
      {
         /* If anything fails here we MAY be a brick
            Copy updated bootloader from external to internal Flash.
            Erase the BL_Code partition except for the INTVEC sector to maximize endurance and minimize write time
            required. */
         attempts = 3;  //Three attempts to ensure not just a transient problem (i.e a real HW fault)
         do
         {
            /* Erase the Bootloader excluding the first sector which contains the vector table to the backup */
            retStatus = PAR_partitionFptr.parErase( ( flAddr )INT_FLASH_ERASE_SIZE, Size - INT_FLASH_ERASE_SIZE, pBLCodePTbl );
            if ( eSUCCESS == retStatus )  //Copy non-vector space of updated BL
            {
               retStatus = copyPart2Part( ( flAddr )INT_FLASH_ERASE_SIZE, Size - INT_FLASH_ERASE_SIZE,
                                          &Buffer[0], sizeof( Buffer ), pDFWImagePTbl_, pBLCodePTbl );
               if ( eSUCCESS == retStatus )  //Update Bl vectors
               {
                  /******************************************************************************************************
                     Very time critical section.  If a spurious reset occurs here the unit is a brick
                  ******************************************************************************************************/
                  /* Now copy the updated INTVEC sector from external to internal Flash */
                  OS_INT_disable( ); //Disable ALL interrupts to ensure this completes as quickly as possible
                  retStatus = PAR_partitionFptr.parErase( ( flAddr )0, INT_FLASH_ERASE_SIZE, pBLCodePTbl );
                  if ( eSUCCESS == retStatus )  //Write BL vectors
                  {
                     retStatus = copyPart2Part( ( flAddr )0, INT_FLASH_ERASE_SIZE, &Buffer[0], sizeof( Buffer ), pDFWImagePTbl_, pBLCodePTbl );
                     OS_INT_enable( );  // Enable as soon as possible
                     if ( eSUCCESS == retStatus )  //Check CRC of updated BL
                     {
                        /************************************************************************************************
                           End of very time critical section.
                        ************************************************************************************************/
                        // Verify Bootloader CRC
                        retStatus = verifyCrc( ( flAddr )PART_BL_CODE_OFFSET, PART_BL_CODE_SIZE, crcBl, pBLCodePTbl );
                        if ( eSUCCESS != retStatus )  //CRC Failed, so revert to Backup Vectors
                        {
                           // Oops, it is not right!  Restore the backup vector table so we are not a brick if it resets
                           /*********************************************************************************************
                              Very time critical section.  If a spurious reset occurs here the unit is a brick
                           *********************************************************************************************/
                           /* Restore Backup vector table to the normal INTVECT */
                           OS_INT_disable( ); //Disable interrupts to ensure this completes as quickly as possible
                           //Need seperate loop control variables
                           uint8_t           attemptsBkup = 3;
                           returnStatus_t    status;

                           do
                           {
                              /* Erase INTVEC sector to minimize time required to write. Remaining bytes in sector being
                                 erased is OK.  It will boot to backup image until new bootloader image is written. */
                              status = PAR_partitionFptr.parErase( BL_VECTOR_TABLE_START, INT_FLASH_ERASE_SIZE, pBLCodePTbl );
                              if ( eSUCCESS == status )
                              {
                                 status = copyPart2Part( 0, BL_VECTOR_TABLE_SIZE, &Buffer[0], sizeof( Buffer ),
                                                         pBLBackupPTbl, pBLCodePTbl );
                                 OS_INT_enable( );   // Enable as soon as possible
                                 if ( eSUCCESS == status )
                                 {
                                    if ( 0 != memcmp( ( flAddr * )pBLBackupPTbl->PhyStartingAddress,
                                                      ( flAddr * )pBLCodePTbl->PhyStartingAddress, BL_VECTOR_TABLE_SIZE ) )
                                    {
                                       status = eFAILURE;
                                    }
                                 }
                              }
                              attemptsBkup--;
                           }while ( attemptsBkup && ( eSUCCESS != status )  );
                           /* If this fails, then  a reboot is unpredicable!  On the other hand if we do not reboot, then
                              continuing the process MAY resolve the problem.  Also really only need to write the
                              first two vectors since the bootloader does not use any of the remaining vectors (interrupts
                              not enabled).
                           */
                           OS_INT_enable( );   // See OS_INT_disable( ); above
                           /****************************************************************************************************
                              End of very time critical section.
                           *****************************************************************************************************/
                        }  //End of if ( eSUCCESS != retStatus )  //CRC Failed, so revert to Backup Vectors
                     }  // End of if ( eSUCCESS == retStatus )  //Check CRC of updated BL
                  }  //End of if ( eSUCCESS == retStatus )  //Write BL vectors
                  OS_INT_enable( );  //Enable again, just in case failed above
               }  //End of if ( eSUCCESS == retStatus )  //Update Bl vectors
            }  //End of if ( eSUCCESS == retStatus )  //Copy non-vector space of updated BL
            attempts--;
         } while ( attempts && ( eSUCCESS != retStatus ));
         if ( eSUCCESS != retStatus )
         {
            brick(); //If we got here the update BL failed
         }
      }  //End of if ( eSUCCESS == retStatus ) // Update the Bootloader
#elif ( MCU_SELECTED == RA6E1 )
      OS_INT_disable(); // Enter critical section, Note: RA6 code flash P/E runs from RAM and any interrupt will vector to code flash causing an exception
      retStatus = PAR_partitionFptr.parErase( (lAddr) 0, BL_BACKUP_SIZE, pBLBackupPTbl );  // Erase the backup partition to write the new bootloader
      OS_INT_enable();  // Exit critical section
      if ( eSUCCESS == retStatus )
      {
         OS_INT_disable(); // Enter critical section, Note: RA6 code flash P/E runs from RAM and any interrupt will vector to code flash causing an exception
         retStatus = copyPart2Part( (flAddr)0, BL_BACKUP_SIZE, &Buffer[0], sizeof( Buffer ), pDFWImagePTbl_, pBLBackupPTbl );
         OS_INT_enable();  // Exit critical section
         if ( eSUCCESS == retStatus )
         {
            // Verify Bootloader CRC of the copied new image
            retStatus = verifyCrc( (flAddr) 0, PART_BL_BACKUP_SIZE, crcBl, pBLBackupPTbl );
            if ( eSUCCESS == retStatus )
            {
               bootFlg = R_FACI_HP->FAWMON_b.BTFLG;  // Get the current startup area from the bootflag
               if ( bootFlg == 1 )
               {
                 bootStartupArea = FLASH_STARTUP_AREA_BLOCK1;  // If Bootflg is 1, startup area is block 0 and we wrote new image in block 1. Hence the next startup area should be block 1
               }
               else if ( bootFlg == 0 )
               {
                  bootStartupArea = FLASH_STARTUP_AREA_BLOCK0; // If Bootflg is 0, startup area is block 1 and we wrote new image in block 0. Hence the next startup area should be block 0
               }

               OS_INT_disable(); // Enter critical section, Note: RA6 code flash P/E runs from RAM and any interrupt will vector to code flash causing an exception
               errStatus = R_FLASH_HP_StartUpAreaSelect( &g_flash0_ctrl, bootStartupArea, false ); // Modify the existing startup area => 0 to 1, 1 to 0
               OS_INT_enable();  // Exit critical section
               if ( errStatus != FSP_SUCCESS )
               {
                  retStatus = eFAILURE;
               }
            }
         }
      }
#endif
   }  // End of if ( ( eSUCCESS == PAR_partitionFptr.parOpen...
   return ( retStatus );
}

/***********************************************************************************************************************

   Function name: copyPart2Part

   Purpose: This function performs a copy from one partition to another.

   Arguments:  flAddr start                     - Starting address used by source and destination
   lCnt size                        - Size of data in bytes to copy
   uint8_t * buffer                 - Pointer to the buffer to use for the data
   uint16_t bufSize                 - Size of the buffer used for the data
   PartitionData_t const * pSrcPTbl - Pointer to the partition table used for the data source
   PartitionData_t const * pDstPTbl - Pointer to the partition table used for the data destination

   Returns: returnValue     - returns eSUCCESS if successful otherwise eFAILURE

   Side Effects:

   Reentrant Code:

   Notes:

**********************************************************************************************************************/
static returnStatus_t copyPart2Part ( flAddr start, lCnt size,
                                      uint8_t * buffer, uint16_t bufSize,
                                      PartitionData_t const * pSrcPTbl, PartitionData_t const * pDstPTbl )
{
   returnStatus_t retStatus = eSUCCESS;
   uint32_t       count;   /*lint !e578 count same name as element in heep.h OK */
   uint32_t       i;

   count = bufSize;
   if ( ( NULL == pSrcPTbl ) || ( NULL == pDstPTbl ) )
   {
      retStatus = eFAILURE;
   }
   for ( i = start; ( i < size ) && ( eSUCCESS == retStatus ); i += bufSize )
   {
      /* Read as many bytes as possible to fill the buffer */
      if ( ( size - i ) < bufSize )
      {
         count = size - i ;   // Remaining fits in buffer
      }
      CLRWDT();
      retStatus = PAR_partitionFptr.parRead( buffer, i, count, pSrcPTbl );
      if ( eSUCCESS == retStatus )
      {
         PWR_CheckPowerDown(); // Check for power down before processing.
         retStatus = PAR_partitionFptr.parWrite( i, buffer, count, pDstPTbl );
      }
   }
   return ( retStatus );
}
#endif  //end of else to #if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )


/***********************************************************************************************************************

   Function name: setDfwToIdle

   Purpose: This function sets the DFW vars to the Idle state - typically used when the patch is completed.

   Arguments:  *pDfwVars   - pointer to dfw vars to be updated

   Returns: none

   Side Effects:

   Reentrant Code:

   Notes:

**********************************************************************************************************************/
static void setDfwToIdle ( DFW_vars_t *pDfwVars )
{
   pDfwVars->eDlStatus   = eDFWS_NOT_INITIALIZED; // Save DFW Status - Not Initialized anymore!
   pDfwVars->ePatchState = eDFWP_IDLE;            // Set state to idle since patching is done.
   pDfwVars->applyMode   = eDFWA_DO_NOTHING;      // Clear the apply mode.
   /* Only get here after reset. The App MSG Task is not available yet so let
      dfw app task know it needs to send the message
   */
   pDfwVars->eSendConfirmation = eDFWC_APPLY_COMPLETE;

   return;
}

/* ****************************************************************************************************************** */
/* UNIT TEST FUNCTIONS */

#if DFW_UT_ENABLE
/***********************************************************************************************************************

   Function Name: dfwUnitTestToggleOutput

   Purpose: Toggle an output pin numToggles times.  There is a software delay.  The accuracy of this delay is not
   important.  The goal is just to allow the person testing to see the toggles on the scope and reset the
   processor.

   Arguments: uint32_t numToggles, uint8_t seq, uint8_t step, uint8_t totStep

   Returns: None

   Note:

   Side Effects: WDT is reset and a BIG delay occurs!

   Reentrant Code: No

**********************************************************************************************************************/
static void dfwUnitTestToggleOutput( uint8_t seq, uint32_t numToggles, uint8_t step, uint8_t totSteps )
{
//   uint16_t softDelay;
   DFW_PRNT_INFO_PARMS( "DFW UT Seq %d, Step %d of %d ********************************************", seq, step, totSteps );
   while( 0 != numToggles-- )
   {
      DFW_UT_TOGGLE_PIN();
//      for (softDelay = 0xFFFF; 0 != softDelay; softDelay--)
      {
         OS_TASK_Sleep( TEN_MSEC );  /* Sleep for a some time. */
      }
   }
}
#endif

/***********************************************************************************************************************

   Function name: DFWA_GetSwapState

   Purpose: This function get the current state of the internal flash swap system.

   Arguments:  *flashSwapStatus_t - pointer to variable that will be updated with status info

   Returns: returnStatus_t - whether status check was successful

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
returnStatus_t DFWA_GetSwapState( flashSwapStatus_t *pStatus )
{
   returnStatus_t retVal;      /* return value based upon attempt to get swap state information */
   IF_ioctlCmdFmt_t  ioctlCmd;            /* Command to send to the driver */

   ioctlCmd.ioctlCmd = eIF_IOCTL_CMD_FLASH_SWAP;   /* Set IOCTL command */
   ioctlCmd.fsCmd    = eFS_GET_STATUS;             /* Command to read the status of swap system*/

#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )) )
   retVal = PAR_partitionFptr.parIoctl( &ioctlCmd, pStatus, pDFWImagePTbl_ ); /* Read the status */
#else
   retVal = PAR_partitionFptr.parIoctl( &ioctlCmd, pStatus, pAppCodePart_ ); /* Read the status */
#endif
   return( retVal );
}


/***********************************************************************************************************************

   Function name: DFWA_verifyPatchFormat

   Purpose: This function is used to verify the correct patch format is being utilized with the provided dfwFileType.

   Arguments:  dl_dfwFileType_t fileType - the type of DFW file being processed
   uint8_t patchFormat - the patch format supplied in the patch header

   Returns: bool - whether the patch format is valid

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static bool DFWA_verifyPatchFormat( dl_dfwFileType_t fileType, uint8_t patchFormat )
{
   bool validFormat = false;

   if( IS_END_DEVICE_TARGET( fileType ) )
   {
      if( DFW_PATCH_METERMATE_SCHEMA_FORMAT == patchFormat )   /*lint !e644 patchFormat initialized.  */
      {
         validFormat = true;
      }
   }
   else
   {
      if( DFW_PATCH_VERSION == patchFormat ) /*lint !e644 patchFormat initialized.  */
      {
         validFormat = true;
      }
   }

   return validFormat;
}

#if ( EP == 1 )
/*********************************************************************************************************************

   \fn void DFWA_CounterInc(meterReadingType readingType)

   \brief  This function is used to increment the DFW Counters

   \param  meterReadingType enumerated valued for the specified reading type

   \return  none

   \details This functions is intended to handle the rollover if required.
   If the counter was modified the CacheFile will be updated.

*********************************************************************************************************************/
void DFWA_CounterInc( meterReadingType readingType )
{
   bool bStatus = false;
   switch( readingType ) /*lint !e788 not all enums used within switch */
   {
      // based upon the counter reading type, increment the counter appropriately
      case dfwDupDiscardPacketQty  :  bStatus = u16_counter_inc(&DfwCachedAttr.dfwDupDiscardPacketCounter, (bool)false); break;
      default                      :  break;
   }  /*lint !e788 not all enums used within switch */

   if( bStatus == true )
   {
      // The counter was modified
      ( void )FIO_fwrite( &DFWCachedFileHndl_, 0, ( uint8_t* )&DfwCachedAttr, sizeof( DfwCachedAttr ) );
   }
}

/*********************************************************************************************************************

   \fn void DFWA_DecrementMeterFwRetryCounter()

   \brief  This function is used to decrement the meter fw retry counter

   \param  non

   \return  none

   \details

*********************************************************************************************************************/
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
static void DFWA_DecrementMeterFwRetryCounter( void )
{
   if( DfwCachedAttr.dfwMeterFwRetriesLeft > 0 )
   {
      DfwCachedAttr.dfwMeterFwRetriesLeft--;
      ( void )FIO_fwrite( &DFWCachedFileHndl_, 0, ( uint8_t* )&DfwCachedAttr, sizeof( DfwCachedAttr ) );
   }
}
#endif

/*********************************************************************************************************************

   \fn bool u16_counter_inc(uint16_t* pCounter, bool bRollover)

   \brief  This function is used to increment a u16 counter.

   \param   pointer to counter variable to be incremented
   boolean to indicate whether rollover of counter is allowed

   \return  boolean whether counter was incremented

   \details The counter is not incremented if the value = UINT16_MAX and rollover is set to false.

*********************************************************************************************************************/
static bool u16_counter_inc( uint16_t* pCounter, bool bRollover )
{
   bool bStatus = false;
   if( ( bRollover == true ) || ( *pCounter < UINT16_MAX ) )
   {
      ( *pCounter )++;
      bStatus = true;
   }
   return bStatus;
}

/*********************************************************************************************************************

   \fn void DfwCached_Init(uint16_t* pCounter, bool bRollover)

   \brief  This function will reset all of the the attributes in the DFW Cached file.

   \param  none

   \return  none

   \details

*********************************************************************************************************************/
void DfwCached_Init( void )
{
   ( void )memset( &DfwCachedAttr, 0, sizeof( DfwCachedAttr ) );
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   DfwCachedAttr.dfwMeterFwRetriesLeft = METER_FW_UPDATE_RETRY_COUNT;
#endif
   ( void )FIO_fwrite( &DFWCachedFileHndl_, 0, ( uint8_t * )&DfwCachedAttr, sizeof( DfwCachedAttr ) );
}

/*********************************************************************************************************************

   \fn void DFWA_getDuplicateDiscardedPacketQty(uint16_t* pCounter, bool bRollover)

   \brief  This function will initialize all of the the "Counter" attributes to a default value.

   \param  none

   \return  none

   \details

*********************************************************************************************************************/
uint16_t DFWA_getDuplicateDiscardedPacketQty( void )
{
   return DfwCachedAttr.dfwDupDiscardPacketCounter;
}
#endif

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
/***********************************************************************************************************************

   Function name: DFWA_getDfwAuditTestStatus

   Purpose: This function is used to get the current value of the dfwAuditTestStatus parameter.

   Arguments:  none

   Returns: uint8_t

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
uint8_t DFWA_getDfwAuditTestStatus( void )
{
   DFW_vars_t dfwVars;
   ( void )DFWA_getFileVars( &dfwVars );

   return dfwVars.fileSectionStatus.auditTestStatus;
}

/***********************************************************************************************************************

   Function name: DFWA_getDfwProgramScriptStatus

   Purpose: This function is used to get the current value of the dfwProgramScriptStatus parameter.

   Arguments:  none

   Returns: uint8_t

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
uint8_t DFWA_getDfwProgramScriptStatus( void )
{
   DFW_vars_t dfwVars;
   ( void )DFWA_getFileVars( &dfwVars );

   return dfwVars.fileSectionStatus.programScriptStatus;
}

/***********************************************************************************************************************

   Function name: DFWA_getDfwCompatibilityTestStatus

   Purpose: This function is used to get the current value of the dfwCompatibilityTestStatus parameter.

   Arguments:  none

   Returns: uint8_t

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
uint8_t DFWA_getDfwCompatibilityTestStatus( void )
{
   DFW_vars_t dfwVars;
   ( void )DFWA_getFileVars( &dfwVars );

   return dfwVars.fileSectionStatus.compatibilityTestStatus;
}
#endif // (( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )

/***********************************************************************************************************************

   Function name: DFWA_getCapableOfEpPatchDFW

   Purpose: This function returns whether EP application updates are supported via DFW.

   Arguments:  none

   Returns: boolean

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
bool DFWA_getCapableOfEpPatchDFW( void )
{
   return endPointDfwSupported_;
}

/***********************************************************************************************************************

   Function name: DFWA_getCapableOfEpBootloaderDFW

   Purpose: This function returns whether EP bootloader updates are supported via DFW.

   Arguments:  none

   Returns: boolean

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
bool DFWA_getCapableOfEpBootloaderDFW( void )
{
   return bootloaderDfwSupported_;
}

/***********************************************************************************************************************

   Function name: DFWA_getCapableOfMeterPatchDFW

   Purpose: This function returns whether meter patch updates are supported via DFW.

   Arguments:  none

   Returns: boolean

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
bool DFWA_getCapableOfMeterPatchDFW( void )
{
   return meterPatchDfwSupported_;
}

/***********************************************************************************************************************

   Function name: DFWA_getCapableOfMeterBasecodeDFW

   Purpose: This function returns whether meter basecode updates are supported via DFW.

   Arguments:  none

   Returns: boolean

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
bool DFWA_getCapableOfMeterBasecodeDFW( void )
{
   return meterBasecodeDfwSupported_;
}

/***********************************************************************************************************************

   Function name: DFWA_getCapableOfMeterReprogrammingOTA

   Purpose: This function returns whether meter program updates are supported via DFW.

   Arguments:  none

   Returns: boolean

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
bool DFWA_getCapableOfMeterReprogrammingOTA( void )
{
   return meterReprogrammingDfwSupported_;
}

/***********************************************************************************************************************

   Function name: DFWA_OR_PM_Handler

   Purpose: This function is used to get parameter values associate with the DFW application module.

   Arguments:  none

   Returns: uint8_t

   Side Effects:

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
returnStatus_t DFWA_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eFAILURE;  // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case dfwStatus :
         {
            *( uint8_t * )value = ( uint8_t ) DFW_Status();

            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen = ( uint16_t )sizeof( uint8_t );
               attr->rValTypecast = ( uint8_t )uintValue;
            }
            break;
         }
         case dfwStreamID :
         {
            *( uint8_t * )value = DFW_StreamId();

            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen = ( uint16_t )sizeof( uint8_t );
               attr->rValTypecast = ( uint8_t )uintValue;
            }
            break;
         }
         case dfwFileType :
         {
            *( uint8_t * )value = ( uint8_t ) DFW_GetFileType();

            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen      = ( uint16_t )sizeof( dl_dfwFileType_t );
               attr->rValTypecast = ( uint8_t )uintValue;
            }
            break;
         }
         case capableOfEpPatchDFW:
         {
            *( uint8_t * )value = ( uint8_t )DFWA_getCapableOfEpPatchDFW();
            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen      = ( uint16_t )sizeof( dl_dfwFileType_t );
               attr->rValTypecast = ( uint8_t )Boolean;
            }
            break;
         }
         case capableOfEpBootloaderDFW:
         {
            *( uint8_t * )value = ( uint8_t )DFWA_getCapableOfEpBootloaderDFW();
            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen      = ( uint16_t )sizeof( dl_dfwFileType_t );
               attr->rValTypecast = ( uint8_t )Boolean;
            }
            break;
         }
         case capableOfMeterBasecodeDFW:
         {
            *( uint8_t * )value = ( uint8_t )DFWA_getCapableOfMeterBasecodeDFW();
            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen      = ( uint16_t )sizeof( dl_dfwFileType_t );
               attr->rValTypecast = ( uint8_t )Boolean;
            }
            break;
         }
         case capableOfMeterPatchDFW:
         {
            *( uint8_t * )value = ( uint8_t )DFWA_getCapableOfMeterPatchDFW();
            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen      = ( uint16_t )sizeof( dl_dfwFileType_t );
               attr->rValTypecast = ( uint8_t )Boolean;
            }
            break;
         }
         case capableOfMeterReprogrammingOTA:
         {
            *( uint8_t * )value = ( uint8_t )DFWA_getCapableOfMeterReprogrammingOTA();
            retVal = eSUCCESS;
            if ( attr != NULL )
            {
               attr->rValLen      = ( uint16_t )sizeof( dl_dfwFileType_t );
               attr->rValTypecast = ( uint8_t )Boolean;
            }
            break;
         }
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
         case dfwCompatibilityTestStatus :
         {
            if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               //The reading will fit in the buffer
               *( uint8_t * )value = DFWA_getDfwCompatibilityTestStatus();
               retVal = eSUCCESS;
               if ( attr != NULL )
               {
                  attr->rValLen = ( uint16_t )sizeof( uint8_t );
                  attr->rValTypecast = ( uint8_t )uintValue;
               }
            }
            break;
         }
         case dfwProgramScriptStatus :
         {
            if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               //The reading will fit in the buffer
               *( uint8_t * )value = DFWA_getDfwProgramScriptStatus();
               retVal = eSUCCESS;
               if ( attr != NULL )
               {
                  attr->rValLen = ( uint16_t )sizeof( uint8_t );
                  attr->rValTypecast = ( uint8_t )uintValue;
               }
            }
            break;
         }
         case dfwAuditTestStatus :
         {
            if ( sizeof( uint8_t ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               //The reading will fit in the buffer
               *( uint8_t * )value = DFWA_getDfwAuditTestStatus();
               retVal = eSUCCESS;
               if ( attr != NULL )
               {
                  attr->rValLen = ( uint16_t )sizeof( uint8_t );
                  attr->rValTypecast = ( uint8_t )uintValue;
               }
            }
            break;
         }
#endif // ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
#if ( EP == 1 )
         case dfwDupDiscardPacketQty:
         {
            if ( sizeof( DfwCachedAttr.dfwDupDiscardPacketCounter ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               //The reading will fit in the buffer
               *( uint16_t * )value = DFWA_getDuplicateDiscardedPacketQty();
               retVal = eSUCCESS;
               if ( attr != NULL )
               {
                  attr->rValLen = ( uint16_t )sizeof( DfwCachedAttr.dfwDupDiscardPacketCounter );
                  attr->rValTypecast = ( uint8_t )uintValue;
               }
            }
            break;
         }
#endif
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case dfwStreamID :
         {
            DFW_SetStreamId( *( uint8_t * )value );
            retVal = eSUCCESS;
            break;
         }
         case capableOfEpPatchDFW:              // fall through case
         case capableOfEpBootloaderDFW:         // fall through case
         case capableOfMeterBasecodeDFW:        // fall through case
         case capableOfMeterPatchDFW:           // fall through case
         case capableOfMeterReprogrammingOTA:
         {
            retVal = eAPP_READ_ONLY;
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }

   return ( retVal );
}
/*********************************************************************************************************************

   \fn       DFWA_SetPatchParititon

   \brief    This function is used to set the appropraite partion to hold the patch data based on the patch type

   \param    fileType

   \return   returnStatus_t

   \details  The PGM file parition has to be used when downloading the KV2c meter FW update becuase it is too large
   to fit within the regular patch partition. A generic patch pointer is used to point to the appropriate partion using
   the init

*********************************************************************************************************************/
returnStatus_t DFWA_SetPatchParititon( dl_dfwFileType_t fileType )   /*lint !e578 argument same as enum OK here.   */
{
   returnStatus_t retVal = eSUCCESS;

   switch( fileType ) /*lint !e788 not all enums used within switch */
   {
      case DFW_ENDPOINT_FIRMWARE_FILE_TYPE:
         pDFWPatch = pDFWPatchPTbl_;
         break;
      case DFW_ACLARA_METER_PROGRAM_FILE_TYPE:
         pDFWPatch = pDFWImagePTbl_;
         break;
      case DFW_ACLARA_METER_FW_PATCH_FILE_TYPE:
         pDFWPatch = pDFWImagePTbl_;
         break;
      case DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE:
         pDFWPatch = pDFWImagePTbl_;
         break;
      default:
         pDFWPatch = pDFWPatchPTbl_;
         retVal = eFAILURE;
   }   /*lint !e788 not all enums used within switch */
   return retVal;
}

/*********************************************************************************************************************

   \fn       DFWA_GetPatchParititonName

   \brief    This function is used to get the appropraite partion to hold the patch data based on the patch type

   \param    none

   \return   returnStatus_t

   \details  The PGM file parition has to be used when downloading the KV2c meter FW update becuase it is too large
   to fit within the regular patch partition. A generic patch pointer is used to point to the appropriate partion using
   the init

*********************************************************************************************************************/
returnStatus_t DFWA_GetPatchParititonName( ePartitionName *dfwPatchPartitionName )
{
   returnStatus_t retVal = eFAILURE;

   if ( dfwPatchPartitionName != NULL )
   {
      *dfwPatchPartitionName = pDFWPatch->ePartition;
      retVal = eSUCCESS;
   }

   return retVal;
}
/*********************************************************************************************************************

   \fn       DFWA_GetPatchParititonSize

   \brief    This function is used to get the size of the partion that holds the patch data

   \param    none

   \return   returnStatus_t

   \details  The PGM file parition has to be used when downloading the KV2c meter FW update becuase it is too large
   to fit within the regular patch partition. A generic patch pointer is used to point to the appropriate partion using
   the init

*********************************************************************************************************************/
uint32_t DFWA_GetPatchParititonSize( void )
{
   ePartitionName dfwPatchPartitionName;
   lCnt           patchPartSize = 0;

   if( DFWA_GetPatchParititonName( &dfwPatchPartitionName ) == eSUCCESS )
   {
      switch( dfwPatchPartitionName )  /*lint !e788 not all enums used in switch  */
      {
         case ePART_DFW_PGM_IMAGE:
            patchPartSize = pDFWImagePTbl_->lDataSize;
            break;
         case ePART_NV_DFW_PATCH:
            patchPartSize = pDFWPatchPTbl_->lDataSize;
            break;
         default:
            patchPartSize = 0;
      } /*lint !e788 not all enums used in switch   */
   }
   return patchPartSize;
}

/*********************************************************************************************************************

   \fn       updateEncryptPartition

   \brief    This function is used to update the encryption key partition in int flash during DFW

   \param    none

   \return   returnStatus_t

   \details  In firmware v1.50.58(DCU's), the structure representing the encryption key
   information was udated.  This partition cannot be updated via DFW commands.  As a result, this
   function was created to modify the encryption key partition once DFW confirms we are attempting
   to DFW from a previous firmware version before the structure change.

*********************************************************************************************************************/
#if  ( DCU == 1 )
static returnStatus_t updateEncryptPartition( void )
{
   typedef struct
   {
      // temporary structure to represent Encrypted keys partition from v1.40, a direct copy of the v1.40 structure
      uint8_t           pKeys[12][32];
      uint8_t           replKeys[12][32];
      uint16_t          keyCRC[12];
      uint16_t          replKeyCRC[12];
      uint8_t           aesKey[16];
      macAddr_t         mac;
      uint16_t          macChecksum;
      uint8_t           pad[6];
      NetWorkRootCA_t   dtlsNetworkRootCA;
      HESubject_t       dtlsHESubject;
      MSSubject_t       dtlsMSSubject;
      MSSubject_t       dtlsMfgSubject2;
   } tempIntFlashNvMap_t;

   // struct to represent information needed to process each command to manipulate data from encrypt partition
   typedef struct
   {
      dSize lSrcOffset;   // source to read data from
      dSize lDestOffset;  // destination to write data
      lCnt dataSize;      // number of bytes to write
   } movingData_t;

   /* First block of commands to update encrypt keys sector.  THe sigKeys and repSigKeys will get expanded in size. */
   /*lint -e{446} offsetof is a side effect? */
   movingData_t dataWriteBlock1[] =
   {
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[0] ),     ( dSize )offsetof( intFlashNvMap_t, pKeys[0] ),          ( lCnt )( sizeof( privateKey ) * 5 ) },
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[5] ),     ( dSize )offsetof( intFlashNvMap_t, sigKeys[0] ),        ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[6] ),     ( dSize )offsetof( intFlashNvMap_t, sigKeys[1] ),        ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[7] ),     ( dSize )offsetof( intFlashNvMap_t, sigKeys[2] ),        ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[8] ),     ( dSize )offsetof( intFlashNvMap_t, sigKeys[3] ),        ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[9] ),     ( dSize )offsetof( intFlashNvMap_t, sigKeys[4] ),        ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[10] ),    ( dSize )offsetof( intFlashNvMap_t, sigKeys[5] ),        ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, pKeys[11] ),    ( dSize )offsetof( intFlashNvMap_t, sigKeys[6] ),        ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[0] ),  ( dSize )offsetof( intFlashNvMap_t, replPrivKeys[0] ),   ( lCnt )( sizeof( privateKey ) * 5 ) },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[5] ),  ( dSize )offsetof( intFlashNvMap_t, replSigKeys[0] ),    ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[6] ),  ( dSize )offsetof( intFlashNvMap_t, replSigKeys[1] ),    ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[7] ),  ( dSize )offsetof( intFlashNvMap_t, replSigKeys[2] ),    ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[8] ),  ( dSize )offsetof( intFlashNvMap_t, replSigKeys[3] ),    ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[9] ),  ( dSize )offsetof( intFlashNvMap_t, replSigKeys[4] ),    ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[10] ), ( dSize )offsetof( intFlashNvMap_t, replSigKeys[5] ),    ( lCnt )( sizeof( privateKey ) )     },
      {( dSize )offsetof( tempIntFlashNvMap_t, replKeys[11] ), ( dSize )offsetof( intFlashNvMap_t, replSigKeys[6] ),    ( lCnt )( sizeof( privateKey ) )     }
   };

   /* Second block of data needed to update the encrypt sector of intflash.  No changes to the size of any member
      of this block.  This data just gets shifted to new offset. */
   /*lint -e{446} offsetof is a side effect? */
   movingData_t dataWriteBlock2 = {( dSize )offsetof( tempIntFlashNvMap_t, keyCRC[0] ),
                                   ( dSize )offsetof( intFlashNvMap_t, keyCRC[0] ),
                                   (lCnt)(offsetof(intFlashNvMap_t, publicKey[0])- offsetof(intFlashNvMap_t, keyCRC[0])) };

   PartitionData_t const *pEncryptKey;  // pointer to partition that will be modified
   returnStatus_t retStatus = eFAILURE; // return status
   buffer_t *pBuf;                      // pointer to buffer for temporary storage

   /* Due to the size of the encryption key partition data structure, a single buffer large enough to build the new
      structure is not available.  To accomadate this problem, the structure will be divided into two parts.  This
      allows us to build the new structure by seperate halves in RAM, then writing each halve to intflash when
      completed making the updates.  By doing this, only two writes to intflash will be required during the process.
      By reading from the dfwImagePartition into RAM, we are protected from power down during the process.  The
      dfwImagePartition upon reboot will still be in its original condition and the encryption sector update process
      can be restarted upon power restoration. */
   pBuf = BM_alloc( 1224 );             // allocate a large buffer to build the new structure
   if( NULL != pBuf )
   {
      uint8_t *tempDestBuffer = pBuf->data; // pointer to data buffer
      if ( eSUCCESS == PAR_partitionFptr.parOpen( &pEncryptKey, ePART_ENCRYPT_KEY, 0L ) )
      {
         retStatus = eSUCCESS; // initialize return to SUCCESS, it will get updated later on a failure

         // Update encryptkey data block #1
         ( void )memset( tempDestBuffer, 0xFF, ( uint16_t )1224 ); // initialize the temporary storage buffer
         for( uint16_t i = 0; i < sizeof( dataWriteBlock1 ) / sizeof( movingData_t ) && eFAILURE != retStatus; i++ )
         {
            // process each command from dataWriteBlock1 updating to new data structure in ram
            CLRWDT();
            retStatus = PAR_partitionFptr.parRead( tempDestBuffer + dataWriteBlock1[i].lDestOffset,
                                                   ( lAddr )( PART_ENCRYPT_KEY_OFFSET + dataWriteBlock1[i].lSrcOffset ),
                                                   dataWriteBlock1[i].dataSize, pDFWImagePTbl_ );
         }

         if( eSUCCESS == retStatus )
         {
            // data block #1 built successfully, write this portion to the encryption key parition
            PWR_CheckPowerDown(); // Check for power down before processing.
            retStatus = PAR_partitionFptr.parWrite( 0, tempDestBuffer,
                                                    ( dSize )offsetof( intFlashNvMap_t, keyCRC[0] ),
                                                    pEncryptKey );
         }

         if( eSUCCESS == retStatus )
         {
            // data block #1 has been updates successfully, read data block 2 from the source into temporary location
            ( void )memset( tempDestBuffer, 0xFF, ( uint16_t )1224 ); // initialize the temporary storage buffer
            retStatus = PAR_partitionFptr.parRead( tempDestBuffer,
                                                   ( lAddr )( PART_ENCRYPT_KEY_OFFSET + dataWriteBlock2.lSrcOffset ),
                                                   dataWriteBlock2.dataSize, pDFWImagePTbl_ );
         }

         if( eSUCCESS == retStatus )
         {
            // data block #2 has been acquired, write data block 2 from temp location to the encrypt partition of intflash
            PWR_CheckPowerDown(); // Check for power down before processing.
            retStatus = PAR_partitionFptr.parWrite( dataWriteBlock2.lDestOffset, tempDestBuffer,
                                                    ( dSize )( dataWriteBlock2.dataSize ), pEncryptKey );
         }
      }

      BM_free( pBuf ); // deallocate the buffer
   }

   return retStatus;
}
#endif

/*********************************************************************************************************************

   \fn       updateDtlsMajorMinorFiles

   \brief    This function is used to update DtlsMajor and DtlsMinor structures contained within
             the named partions #144 and #145

   \param    none

   \return   returnStatus_t

   \details  Each partition contains a single file.  Both files expanded in size. The systim_t structure
             contained within the DtlsMajor file expanded by 8 bytes.

*********************************************************************************************************************/
#if ( EP == 1 )
static returnStatus_t updateDtlsMajorMinorFiles( void )
{
   PartitionData_t const *pDtlsMajorData;  // pointer to partition that will be modified
   PartitionData_t const *pDtlsMinorData;  // pointer to partition that will be modified
   returnStatus_t retStatus = eFAILURE;    // return status
   buffer_t *pBuf;                         // pointer to buffer for temporary storage

   /* Due to the size of the DtlsMajorSession_s data structure, a single buffer large enough to build the new
      structure size of 1052 bytes is available. The new structure will be built in RAM, then writing the updated
      structure to extflash when finished.  By doing this, only one write to extflash will be required for the update.
   */
   pBuf = BM_alloc( 1052 ); // allocate a large buffer to build the new structure and file header
   if( NULL != pBuf )
   {
      uint8_t *tempDestBuffer = pBuf->data; // pointer to data buffer in RAM

      if ( ( eSUCCESS == PAR_partitionFptr.parOpen( &pDtlsMajorData, ePART_DTLS_MAJOR_DATA, 0L ) ) &&
           ( eSUCCESS == PAR_partitionFptr.parOpen( &pDtlsMinorData, ePART_DTLS_MINOR_DATA, 0L ) )  )
      {
         retStatus = eSUCCESS; // initialize return to SUCCESS, it will get updated later on a failure

         // initialize the temporary storage buffer in RAM
         ( void )memset( tempDestBuffer, 0, 1052 ); // initialize the temporary storage buffer

         /* read the partition data minus the file header and old systime_t structure, then
            store this data offset 8 bytes futher at temporary location.  This will make room
            for the new 8 bytes that will be added to the structure */
         retStatus = PAR_partitionFptr.parRead( tempDestBuffer + (dSize)(sizeof(tFileHeader) + 16),
                                                (dSize)(sizeof(tFileHeader) + 8),
                                                ( lAddr )(1028), pDtlsMajorData );

         if( eSUCCESS == retStatus )
         {
            // Now copy file header and first 8 bytes of systime_t structure to same file offset in temp location
            retStatus = PAR_partitionFptr.parRead( tempDestBuffer, (dSize)0,
                                                   ( lAddr )(sizeof(tFileHeader) + 8), pDtlsMajorData );
         }

         if( eSUCCESS == retStatus )
         {  // first we need to update the file size to 1044 bytes before writing the RAM copy back to external flash

            uint16_t dtlsMajorFileSize = 1044; // inialize the size variable that will be written into the partition
            // write the new file size offsetting 4 bytes into the file header, which is at the front of the partition
            (void)memcpy( (uint8_t *)(tempDestBuffer + 4), (uint8_t *)&dtlsMajorFileSize, sizeof(dtlsMajorFileSize) );

            // Write temporary buffer from RAM back to extflash partition that will contain the updated DTLS major structure
            PWR_CheckPowerDown(); // Check for power down before processing.
            retStatus = PAR_partitionFptr.parWrite( ( dSize)0, tempDestBuffer, ( lCnt )1052, pDtlsMajorData );
         }

         // update the DTLS minor size to the new data size of 110
         if( eSUCCESS == retStatus )
         {
            uint16_t dtlsMinorFileSize = 110;  // inialize the size variable that will be written into the partition
            // write the new file size offsetting 4 bytes into the file header, which is at the front of the partition
            retStatus = PAR_partitionFptr.parWrite( ( dSize)4, (uint8_t *)&dtlsMinorFileSize,
                                                    ( lCnt )sizeof(dtlsMinorFileSize), pDtlsMinorData );
         }
      }

      BM_free( pBuf ); // deallocate the buffer
   }

   return retStatus;
}
#endif
