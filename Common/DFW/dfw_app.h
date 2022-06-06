/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dfw_app.h
 *
 * Contents: Contains prototypes and definitions for DFW.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2004-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ B. Hammond Created 07-30-2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 -  05/06/2013 - Initial Release
 *
 * @version    0.1
 * #since      2013-05-05
 **********************************************************************************************************************/

#ifndef dfw_app_H
#define dfw_app_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "project.h"
#include "version.h"
#include "time_util.h"
#include "partition_cfg.h"
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
#include "HEEP_util.h"
#endif
//#include <stdio.h>

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef DFWA_GLOBALS
#define DFWA_EXTERN
#else
#define DFWA_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* Comment this out if you want to use the NVRAM */
#define DFW_UT_ENABLE            0

/* Buffer usually defined on stack used to hold data for read/write operation.  Needs to be a multiple of 4 bytes.  A
 * large value will minimize flash wear and speed.  THIS IS ON THE STACK SO BE CAREFUL WITH LARGE VALUES */
#define READ_WRITE_BUFFER_SIZE   ((uint16_t)512)
                                                    /* THIS IS ON THE STACK SO BE CAREFUL WITH LARGE VALUES */

#define DFW_PROTOCOL_SUPPORT           (2u)          // Only protocol 2 is supported
#define DFW_PATCH_VERSION              (4u)          // normal patch type with DeviceType version
#define DFW_PATCH_METERMATE_SCHEMA_FORMAT (uint8_t)100  // expected patch format for meter configuration updates

#define COM_DEVICE_TYPE_LEN      ((uint8_t)20)     /* Com device type length of the string */

#define PATCH_DECRYPTED_SECTOR   ((uint8_t)0)
#define PATCH_ENCRYPTED_SECTOR   ((uint8_t)1)

#define DFWA_FATAL_ERROR_OFFSET  ((eDFWE_FATAL_ERRORS - eDFWE_LAST_NON_FATAL_ERROR) - 1)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef uint8_t         dl_streamid_t;       /*!<  */
typedef int16_t         dl_packetid_t;       /*!< This should be unsigned but it is signed to make missing Packet ID's more robust */
typedef uint16_t        dl_packetcnt_t;      /*!<  */
typedef uint16_t        dl_packetsize_t;     /*!<  */
typedef uint32_t        dl_patchsize_t;      /*!<  */
typedef sysTimeCombined_t   dl_expirationtime_t; /*!<  */
typedef bool            dl_clearbitflag_t;   /*!<  */
typedef uint8_t         dl_dfwFileType_t;

typedef enum
{
   eDFWS_NONE = ((uint8_t)0),    // Cipher not used
   eDFWS_AES_CCM                 // AES CCM
}eDfwCiphers_t;                  // Cipher Suites supported

typedef struct
{
   firmwareVersion_u firmwareVersion;
   firmwareVersion_u toFwVersion;
   sysTimeCombined_t expirationTime;
   uint32_t          gracePeriodSec;  //Actually only 24 bits
   dl_streamid_t     streamId;
   eDfwCiphers_t     cipher;
   dl_packetsize_t   packetSize;
   dl_patchsize_t    patchSize;
   dl_packetcnt_t    packetCount;
   char              comDeviceType[COM_DEVICE_TYPE_LEN+1]; // String of 20 bytes plus NULL
   uint8_t           protocolVer;
   bool              bRegister;
   eFwTarget_t       target;              /* What patch is for, e.g. Bootloader, App, Meter FW, Meter TOU etc. */
}DFWI_dlInit_t;

#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
typedef struct
{
   uint8_t compatibilityTestStatus;       // Store the result of compatiblity test section from a dfw file
   uint8_t programScriptStatus;           // Store the result of a program script section from a dfw file
   uint8_t auditTestStatus;               // Store the result of an audit test section from a dfw file */
}DFWI_dfwFileSectionStatus_t;
#endif

/* eDfwPatchState_t must be aligned with DwfStateStrings indexes */
typedef enum
{
   eDFWP_IDLE = ((uint8_t)0),             // Default state, nothing to do
   eDFWP_EXEC_PGM_COMMANDS,               // Executing all the PGM patch commands
   eDFWP_WAIT_FOR_APPLY,                  // Waiting for apply command to continue
   eDFWP_DO_FLASH_SWAP,                   // Perform the flash swap
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( ( DCU == 1 ) &&  ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_REV_A )) )
#else
   eDFWP_BL_SWAP,                         // Bootloader performs the flash swap
#endif
   eDFWP_EXEC_NV_COMMANDS,                // Executing all NV patch commands
   eDFWP_COPY_NV_IMAGE_TO_NV,             // Copy NV Image (that was just updated) to their proper partitions
   eDFWP_VERIFY_SWAP,                     // Verify the swap command was executed
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
   eDFWP_COMPATIBILITY_TEST,              // Perform compatiblilty check on patch file for meter config update
   eDFWP_PROGRAM_SCRIPT,                  // Perform the actual meter configuration updates
   eDFWP_AUDIT_TEST                       // Perform an audit on updates made to the meter configuration
#endif
}eDfwPatchState_t;                        // Patch states used in DFWA_do_patch() and bootloader

typedef enum
{
   eDFWE_NO_ERRORS = ((uint8_t)0),        // No error
   eDFWE_PATCH_FORMAT,                    // Patch format mismatch
   eDFWE_FW_VER,                          // Firmware version mismatch
   eDFWE_DEV_TYPE,                        // Device Type mismatch
   eDFWE_PATCH_LENGTH,                    // Patch length mismatch
   eDFWE_INVALID_TIME,                    // System date/time is invalid
   eDFWE_EXPIRED,                         // Patch expired
   eDFWE_CRC32PATCH,                      // CRC32 error on patch
   eDFWE_INCOMPLETE,                      // All Packets not yet recevied
   eDFWE_CRC32,                           // CRC32 error on patched code
   eDFWE_SCH_AFTER_EXPIRE,                // Scheduled apply is after the expiration
   eDFWE_STREAM_ID,                       // Invalid Stream ID
   eDFWE_SIGNATURE,                       // Signature of the encrypted Patch failed
   eDFWE_DECRYPT,                         // Failed to decrypt the Patch
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
   eDFWE_DFW_COMPATIBLITY_TEST,           // Failed the compatiblity test
   eDFWE_DFW_PROGRAM_SCRIPT,              // Failed the program script
   eDFWE_LAST_NON_FATAL_ERROR = eDFWE_DFW_PROGRAM_SCRIPT,   // Last non Fatal error
#else
   eDFWE_LAST_NON_FATAL_ERROR = eDFWE_DECRYPT,
#endif
   /* Fatal Errors if patching */
   eDFWE_FATAL_ERRORS = ((uint8_t)100),   // Fatal Errors start with the enumeration
   eDFWE_NV = eDFWE_FATAL_ERRORS,         // NV access failed
   eDFWE_FLASH,                           // Flash Access Error
   eDFWE_CMD_CODE,                        // Unknown patch command (an unknown command in the 'macro' language).
   eDFWE_INVALID_STATE,                   // Invalid state, corrupt NV?
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
   eDFWE_DFW_AUDIT_TEST                   // Failed the audit test
#endif
}eDfwErrorCodes_t;                        // DFW module error codes

/* eDlStatus_t must be aligned with DwfStatusStrings indexes */
typedef enum
{
   eDFWS_NOT_INITIALIZED = ((uint8_t)0),  // Not initialized or Firmware updated successfully and is now Not Initialized
   eDFWS_INITIALIZED_INCOMPLETE,          // Initialized - Additional packets required
   eDFWS_INITIALIZED_COMPLETE,            // All packets received - Waiting for apply command
   eDFWS_INITIALIZED_WITH_ERROR           // An error occurred / Initialized expired
}eDlStatus_t;                             // Download eStatus

/* eDfwApplyMode_t must be aligned with DwfApplyModeStrings indexes */
typedef enum
{
   eDFWA_DO_NOTHING = ((uint8_t)0),       // Default State
   eDFWA_PREPATCH,                        // Start the patch process (but don't apply)
   eDFWA_FULL_APPLY                       // Either finish the patch or do the entire patch
}eDfwApplyMode_t;                         // Used by the DFWA_setFileVars function.

typedef enum
{
   eDFWD_IDLE = ((uint8_t)0),             // Default State - DFW not intialized
   eDFWD_READY,                           // DFW Patch is encrypted in sector 1 and ready for decryption
   eDFWD_DECRYPTING,                      // DFW currently decrypting Patch - un-recoveable state(for now)
   eDFWD_DONE                             // DFW decryption completed - patch ready, starting in sector 0
}eDfwDecryptState_t;                      // Used by various functions to determine if patch is decrypted.

typedef enum
{
   eDFWC_NONE = ((uint8_t)0),             // Default State
   eDFWC_DL_COMPLETE,                     // Send the Download confirmation message
   eDFWC_APPLY_COMPLETE                   // Send the Apply Confirmation message
}eDfwSendConfirm_t;                       // Used by application to send confirmation message at power up.

#if 0
   /* This adds flag so NV commands can be checked and copying of NV does not have to occur if
      there are no NV commands.  Requires changes to all clrBitArrayStatus uses (insert bits.) */
typedef union Flags_u
   struct FlagBits
   {
      uint8_t  bitArrayClr    : 1;     /* Received Packet Bitfield cleared */
      uint8_t  nvCmdsPresent  : 1;     /* Patch has NV commands */
      uint8_t  rsvd           : 6;     /* Unused */
   } bits;
   uint8_t     Flags;                  /* All Flags */
} Flags_t, *Flags_t;
#endif

typedef struct
{
   DFWI_dlInit_t     initVars;            /* Init data */
   eDfwPatchState_t  ePatchState;         /* Patch state used in do_patch() */
   eDlStatus_t       eDlStatus;           /* Download status field */
   eDfwErrorCodes_t  eErrorCode;          /* Error Code field */
#if 0 //TODO: Include with struct def above, remove dl_clearbitflag_t clrBitArrayStatus below
   Flags_t           flags;
#else
   dl_clearbitflag_t clrBitArrayStatus;   /* Flag for a request to clear bit field */
#endif
   eDfwApplyMode_t   applyMode;           /* Partial apply or full apply? */
   uint8_t           patchSector;         /* Sector where Patch starts, 0 or !0 (typically 1) for encrypted paches */
   eDfwDecryptState_t  decryptState;      /* Current state of the decryption process */
   sysTimeCombined_t applySchedule;       /* Time to apply the patch */
   eDfwSendConfirm_t eSendConfirmation;   /* Flag to request a comfirmation message at powerup */
   dl_dfwFileType_t  fileType;            /* Indicate to the dfw application how to apply the patch */
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
   DFWI_dfwFileSectionStatus_t fileSectionStatus; /* Store dfw file section status values */
#endif
}DFW_vars_t;                              /* Variables used by DFW, partition data structure */

#if ( EP == 1 )
typedef struct
{
   uint16_t dfwDupDiscardPacketCounter;   // Number of packets discarded, duplicate packet ID but different packet contents
#if ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED )
   uint8_t  dfwMeterFwRetriesLeft;        // Number of retries left to complete the meter firmware update via DFW
#endif
}DfwCachedAttr_t;
#endif

typedef enum
{
   eDFWC_PREPATCH = ((uint8_t)1),         // Start the patch process (but don't apply)
   eDFWC_FULL_APPLY,                      // Either finish the patch or do the entire patch
   eDFWC_CANCEL,                          // Cancel the download
   eDFWC_CANCEL_APPLY                     // Cancel any pending Apply schedule
}eDfwCommand_t;                           // Used the DFW interface to send commands to DFW task.

/* This function is the callback to write the decrypted bytes to NVRAM. */
typedef void (*DFWA_WritePatchCallback_t)(uint8_t *decrypted, uint32_t numberOfBytes);


/* ****************************************************************************************************************** */
/* CONSTANTS */
#define DFW_ENDPOINT_FIRMWARE_FILE_TYPE         (uint8_t)0  // bootloader/app updates
#define DFW_ACLARA_METER_PROGRAM_FILE_TYPE      (uint8_t)1  // meter config updates
#define DFW_ACLARA_METER_FW_PATCH_FILE_TYPE     (uint8_t)5  // meter firmware patch updates
#define DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE (uint8_t)6  //meter firmware basecode update
#define DFW_MAX_VALID_FILE_TYPE                 DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE
#define DFW_DEFAULT_PATCH_FILE_TYPE             DFW_ENDPOINT_FIRMWARE_FILE_TYPE
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
#define DFW_FILE_SECTION_PASSED                 (uint8_t)0   // pass
#define DFW_FILE_SECTION_FAILED                 (uint8_t)1   // fail
#define DFW_FILE_SECTION_NOT_APP                (uint8_t)2   // not applicable or not tested
#define DFW_FILE_SECTION_STATUS_DEFAULT_VALUE   DFW_FILE_SECTION_NOT_APP
#endif

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

DFWA_EXTERN   OS_MSGQ_Obj DFWA_MQueueHandle;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

DFWA_EXTERN void           DFWA_task( taskParameter );
DFWA_EXTERN returnStatus_t DFWA_init( void );
DFWA_EXTERN returnStatus_t DFWA_setFileVars(DFW_vars_t const *pFile);
DFWA_EXTERN returnStatus_t DFWA_getFileVars(DFW_vars_t *pFile);
DFWA_EXTERN void           DFWA_CancelPending(uint8_t uCancelAll);
DFWA_EXTERN returnStatus_t DFWA_CopyPgmToPgmImagePartition( uint8_t copyKeySect, eFwTarget_t target );
DFWA_EXTERN returnStatus_t DFWA_WaitForSysIdle(uint32_t maxWaitTimeMs);
DFWA_EXTERN void           DFWA_UnlockSysIdleMutex(void);
DFWA_EXTERN returnStatus_t DFWA_SupportedCiphers(eDfwCiphers_t cipher, dl_dfwFileType_t fileType);
DFWA_EXTERN returnStatus_t DFWA_setDerivedKey(firmwareVersion_u fromVersion, firmwareVersion_u toVersion);
DFWA_EXTERN returnStatus_t DFWA_SetSwapState(eFS_command_t swapCmd);
DFWA_EXTERN returnStatus_t DFWA_GetSwapState(flashSwapStatus_t *pStatus);
DFWA_EXTERN returnStatus_t DFWA_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
DFWA_EXTERN returnStatus_t DFWA_SetPatchParititon( dl_dfwFileType_t dfwPatchType );
DFWA_EXTERN returnStatus_t DFWA_GetPatchParititonName( ePartitionName *dfwPatchPartitionName );
DFWA_EXTERN uint32_t       DFWA_GetPatchParititonSize( void );
#if ( EP == 1 )
DFWA_EXTERN void DFWA_CounterInc(meterReadingType readingType);
DFWA_EXTERN void DfwCached_Init( void );
DFWA_EXTERN uint16_t DFWA_getDuplicateDiscardedPacketQty( void );
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
DFWA_EXTERN uint8_t DFWA_getDfwAuditTestStatus( void );
DFWA_EXTERN uint8_t DFWA_getDfwProgramScriptStatus( void );
DFWA_EXTERN uint8_t DFWA_getDfwCompatibilityTestStatus( void );
#endif // endif for (( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
#endif // endif for ( EP == 1 )

DFWA_EXTERN bool DFWA_getCapableOfEpPatchDFW( void );
DFWA_EXTERN bool DFWA_getCapableOfEpBootloaderDFW( void );
DFWA_EXTERN bool DFWA_getCapableOfMeterPatchDFW( void );
DFWA_EXTERN bool DFWA_getCapableOfMeterBasecodeDFW( void );
DFWA_EXTERN bool DFWA_getCapableOfMeterReprogrammingOTA( void );

#undef DFWA_EXTERN

#endif
