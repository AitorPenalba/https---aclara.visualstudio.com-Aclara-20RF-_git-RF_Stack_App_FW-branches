/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: dfw_interface.h
 *
 * Contents: Contains the interface (commands) for DFW.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013-20121 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author B. Hammond
 *
 * $Log$ Created November 13, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 11/13/2013 - Initial Release
 *
 * @version    0.1
 * #since      2013-11-13
 **********************************************************************************************************************/
#ifndef dfw_interface_H_
#define dfw_interface_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "version.h"
#include "time_util.h"
#include "dfw_app.h"
#include "HEEP_util.h"
#include "APP_MSG_Handler.h"
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "ed_config.h"
#endif
/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef DFWI_GLOBALS
   #define dfw_interface_EXTERN
#else
   #define dfw_interface_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
//Max allowed by the reading info(3B), type(2B) and value sizes for the three required reading types
// Evnt&Rdg Qty's(1B), (dfwStreamID (+1 byte readingvalue size), dfwpacketID (+2 byte readingvalue size) and dfwPacket (var value size)
/* Maximum download packet reading value size (reduced by the required readings and values) */
#define DFWI_MAX_DL_PKT_SIZE              ((uint16_t)1220)  /* Current actual is 1227, leaving 7 bytes for cushion */
#define DFWI_MIN_DL_PKT_SIZE              ((uint16_t)1)     /* Minimum download packet size */
#define DFWI_MIN_PATCH_SIZE               ((uint32_t)44)    /* Patch header size plus minimum command (write 1 byte, 7 bytes) */
#define DFWI_MAX_NIC_FW_PATCH_SIZE        ((uint32_t)PART_NV_DFW_PATCH_SIZE) /* Maximum Patch size for NIC fw update or meter config*/
#define DFWI_MAX_ED_FW_PATCH_SIZE         ((uint32_t)PART_NV_DFW_PGM_IMAGE_SIZE) /* Maximum Patch size for Meter/ED fw update or meter config*/
#define DFWI_MIN_CIPHER_PATCH_SIZE        ((uint32_t)124)   /* Mimimum patch size with cipher enabled */
#define DFWI_MAX_CIPHER_NIC_FW_PATCH_SIZE (DFWI_MAX_NIC_FW_PATCH_SIZE - EXT_FLASH_ERASE_SIZE)  /*Maximum NIC FW patch size with cipher enabled */
#define DFWI_MAX_CIPHER_ED_FW_PATCH_SIZE (DFWI_MAX_ED_FW_PATCH_SIZE - EXT_FLASH_ERASE_SIZE)  /*Maximum Meter FW patch size with cipher enabled */
/*check if the meter code is the target. Base code or patch upgrade*/
#define IS_METER_FIRM_UPGRADE(x)  ( ( ( x == DFW_ACLARA_METER_FW_PATCH_FILE_TYPE ) ||\
                                      ( x == DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE ) ) ? true : false )
/*check if the end device or NIC is the target*/
#define IS_END_DEVICE_TARGET(x)   ( ( ( x == DFW_ACLARA_METER_FW_PATCH_FILE_TYPE ) ||\
                                      ( x == DFW_ACLARA_METER_PROGRAM_FILE_TYPE ) ||\
                                      ( x == DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE ) ) ? true : false )
/*check if valid filetype for target device*/
#if ( (EP == 1) && ( END_DEVICE_PROGRAMMING_FLASH == ED_PROG_FLASH_PATCH_ONLY ) )
#define IS_VALID_DFW_FILE_TYPE(x) ( ( ( x == DFW_ENDPOINT_FIRMWARE_FILE_TYPE )     ||\
                                      ( x == DFW_ACLARA_METER_PROGRAM_FILE_TYPE )  ||\
                                      ( x == DFW_ACLARA_METER_FW_PATCH_FILE_TYPE ) )     ? true : false )
#elif ( (EP == 1) && ( END_DEVICE_PROGRAMMING_FLASH == ED_PROG_FLASH_BASECODE_ONLY ) )
#define IS_VALID_DFW_FILE_TYPE(x) ( ( ( x == DFW_ENDPOINT_FIRMWARE_FILE_TYPE )     ||\
                                      ( x == DFW_ACLARA_METER_PROGRAM_FILE_TYPE )  ||\
                                      ( x == DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE ) ) ? true : false )
#elif ( (EP == 1) && ( END_DEVICE_PROGRAMMING_FLASH == ED_PROG_FLASH_SUPPORT_ALL ) )
#define IS_VALID_DFW_FILE_TYPE(x) ( ( ( x == DFW_ENDPOINT_FIRMWARE_FILE_TYPE )     ||\
                                      ( x == DFW_ACLARA_METER_PROGRAM_FILE_TYPE )  ||\
                                      ( x == DFW_ACLARA_METER_FW_PATCH_FILE_TYPE ) ||\
                                      ( x == DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE ) ) ? true : false )
#else //default
#define IS_VALID_DFW_FILE_TYPE(x)   ( ( x == DFW_ENDPOINT_FIRMWARE_FILE_TYPE  )          ? true : false )
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
typedef union
{
   uint8_t endDeviceAnsiTblOID[EDCFG_MANUFACTURER_LENGTH]; // Non-Stored value supplied in Meter config dfw init, to be compared against 4 bytes ST0
   uint8_t endDeviceModel[EDCFG_MODEL_LENGTH]; // Non-Stored value supplied in Meter FW dfw init, to be compared against 8 bytes ST1
}DFWI_endDeviceReqReadings_t;
#endif


/* The following structure is used to temporarily store readings utilized during a dfw init request. These readings are
   are not stored as dfw init variables. */
typedef struct
{
   dl_dfwFileType_t fileType; // stored in DFW_vars upon successful initialization
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   DFWI_endDeviceReqReadings_t reqEndDeviceReadings;
#endif
}DFWI_nonStoredDfwInitReadings_t;


/*! Cancel download */
typedef struct
{
   dl_streamid_t streamId;
}  dfw_cancel_t;

/*! Download packet */
typedef struct
{
   dl_streamid_t streamId;
   dl_packetid_t packetId;
   uint16_t numBytes;
   uint8_t  *pData;
}dfw_dl_packet_t;

/*! Verify download response */
typedef struct
{
   eDlStatus_t       eStatus;                // Status of the DFW
   eDfwErrorCodes_t  eErrorCode;             // Current Error Code
   eDfwPatchState_t  ePatchState;            // State of the patch, this is really here for debugging
   dl_packetcnt_t    packetCount;            // Total number of packets expected
   dl_packetcnt_t    numMissingPackets;      // Total missing packets
   dl_packetid_t     *pMissingPacketIds;     // Pointer to array of missing packets
} dfw_verifyResp_t;

/*! Verify download */
typedef struct
{
   dl_packetcnt_t    maxMissingPcktIdsReported; // Maximum number of missing packets IDs to report
   dfw_verifyResp_t  *pResp;                    // Location to store the response
} dfw_verify_t;

/*! Apply download */
typedef struct
{
   dl_streamid_t     streamId;               // Stream ID, apply patch if the stream ID matches.
   sysTimeCombined_t applySchedule;          // Scheduled time to apply the patch.
} dfw_apply_t;                               // Apply command

typedef enum
{
   eINIT_RESP_RCE_INITIALIZED = ((uint8_t)0),// RCE initialized for download
   eINIT_RESP_FW_MISMATCH,                   // Firmware mismatch.            Initialization was rejected
   eINIT_RESP_DEV_TYPE_MISMATCH,             // EP DeviceType does not match. Initialization was rejected
   eINIT_RESP_PACKET_SIZE_OUT_OF_BOUNDS,     // Packet Size out of bounds.    Initialization was rejected
   eINIT_RESP_PACKET_COUNT_OUT_OF_BOUNDS,    // Packet Count out of bounds.   Initialization was rejected
   eINIT_RESP_PATCH_OUT_OF_BOUNDS,           // Patch out of bounds.          Initialization was rejected
   eINIT_RESP_EXPIRY_OUT_OF_BOUNDS,          // Expiration out of bounds.     Initialization was rejected
   eINIT_RESP_GRACE_PERIOD_OUT_OF_BOUNDS,    // Grace Period out of bounds.   Initialization was rejected
   eINIT_RESP_CIPHER_OUT_OF_BOUNDS,          // Cipher suite out of bounds.   Initialization was rejected
   eINIT_RESP_DATE_TIME_NOT_SET,             // Date/Time not set (invalid).  Initialization was rejected
   eINIT_RESP_APPLY_IN_PROCESS,              // Apply is already in process.  Initialization was rejected
   eINIT_RESP_INVALID_DFW_FILE_TYPE,         // Invalid DFW file type.        Initialization was rejected
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
   eINIT_RESP_ANSI_TABLE_OID_MISMATCH,       // ansiTableOID mismatch.        Initialization was rejected
#endif
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   eINIT_RESP_ED_MODEL_MISMATCH,             // end device model mismatch.    Initialization was rejected
#endif
}DFWI_eInitResponse_t;                       // Initialize Request responses

typedef enum
{
   eAPPLY_RESP_APPLY_ACCEPTED = ((uint8_t)0),   // Apply accepted and proceeding to schedule
   eAPPLY_RESP_NOT_INITIALIZED,                 // Device is not initialized.  Apply was rejected
   eAPPLY_RESP_STREAM_ID_MISMATCH,              // Stream ID mismatch.  Apply was rejected
   eAPPLY_RESP_INCOMPLETE,                      // Patch incomplete.  Apply was rejected
   eAPPLY_RESP_ERROR_STATUS,                    // Patch was initialized, but there was an error executing the patch
   eAPPLY_RESP_INTERNAL_ERROR,                  // Typically an NV or Buffer allocation error.  Apply was rejected
   eAPPLY_RESP_EXPIRED,                         // Patch expired.  Apply was rejected
   eAPPLY_SCH_AFTER_EXPIRE,                     // Schedule occurs after the initilized expiration.  Apply was rejected
   eAPPLY_RESP_APPLY_IN_PROCESS                 // Apply is already in process.  Apply was rejected
}DFWI_eApplyResponse_t;                         // Apply Request responses

typedef enum
{
   eVERIFY_RESP_NOT_INITIALIZED = ((uint8_t)0),  // Device is not initialized.  Verify Not Applicable
   eVERIFY_RESP_INCOMPLETE,                      // Patch incomplete.  Return missing packets
   eVERIFY_RESP_INIT_WITH_ERROR,                 // All packets received, but there was an error executing the patch
   eVERIFY_RESP_INIT_WAIT_FOR_APPLY,             // All packets received, ready to apply
   eVERIFY_RESP_APPLY_IN_PROCESS                 // Apply is already in process.  Verify Not Applicable
}DFWI_eVerifyResponse_t;                         // Verify Request responses

typedef struct
{
   eDfwCommand_t  eCmd;
   dfw_apply_t    applyVars;
}dfwFullApplyCmd_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
void DFWI_DisplayStatus(void);
void DFWI_Init_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);
void DFWI_Packet_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);
void DFWI_Apply_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);
void DFWI_Verify_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length);

buffer_t* DFWI_DnlComplete(DFW_vars_t dfwVars, HEEP_APPHDR_t* pAppMsgTxInfo);
buffer_t* DFWI_PatchComplete(HEEP_APPHDR_t* pAppMsgTxInfo, DFW_vars_t* pDfwVars);


/**
 * DFWI_DownloadInit - Initialize the transponder for download.
 *
 * @param  dfw_init_t - Data required to initialize the transponder
 * @return DFWI_eInitResponse_t
 */
DFWI_eInitResponse_t DFWI_DownloadInit( DFWI_dlInit_t const *pReq, DFWI_nonStoredDfwInitReadings_t const *nonStored );
/**
 * DFWI_DownloadCancel - Cancel the download
 *
 * @param  dfw_cancel_t - Data required to cancel the download
 * @return returnStatus_t
 */
returnStatus_t DFWI_DownloadCancel(const dfw_cancel_t    *pReq);

/**
 * DFWI_DownloadPacket - Write the downloaded packet to the transponder
 *
 * @param  dfw_dl_packet_t - Data required to accept the packet
 * @return None
 */
void DFWI_DownloadPacket(const dfw_dl_packet_t *pReq);

/**
 * DFWI_DownloadVerify - Verify command
 *
 * @param  dfw_verify_t
 * @return returnStatus_t
 */
DFWI_eVerifyResponse_t DFWI_DownloadVerify(const dfw_verify_t    *pReq);

/**
 * DFWI_DownloadApply - Apply the patch
 *
 * @param  dfw_apply_t
 * @return DFWI_eApplyResponse_t
 */
DFWI_eApplyResponse_t DFWI_DownloadApply( const dfw_apply_t     *pReq);

eDlStatus_t DFW_Status(void);
void        DFW_SetStatus(eDlStatus_t eStatus);

uint8_t     DFW_StreamId(void);
void        DFW_SetStreamId(uint8_t);

dl_dfwFileType_t DFW_GetFileType(void);

/* ****************************************************************************************************************** */
#undef dfw_interface_EXTERN

#endif


