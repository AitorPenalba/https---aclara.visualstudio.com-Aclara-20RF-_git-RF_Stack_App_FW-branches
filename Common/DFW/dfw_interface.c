/***********************************************************************************************************************

   Filename:   dfw_interface.c

   Global Designator: DFWI_

   Contents:   This file contains the application interface to the Download Firmware Module.  The requests should have
               been parsed before calling these functions.

 ***********************************************************************************************************************
   A product of Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013-2021 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author B. Hammond

   $Log$ KAD Created July 29, 2013

 ***********************************************************************************************************************
   Revision History:
   v0.1 - BH 07/29/2013 - Initial Release

   @version    0.1
   #since      2013-07-29
 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdio.h>
#include "project.h"
#define DFWI_GLOBALS
#include "dfw_interface.h"
#undef  DFWI_GLOBALS
#include "dfw_pckt.h"         // bit field
#include "DBG_SerialDebug.h"
#include "partitions.h"
#include "partition_cfg.h"
#include "pack.h"
#include "APP_MSG_Handler.h"
#include "EVL_event_log.h"
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
//#include "ed_config.h"
#include "intf_cim_cmd.h"
#endif
#if ( DCU == 1 )
#include "STAR.h"
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
//lint -esym(750,ED_EVENT_TYPE_SIZE,ED_EVENT_TYPE_SIZE_BITS,NVP_QTY_VALUE_SIZE_BITS,READING_TYPE_SIZE_BITS)
//lint -esym(750,DFWI_PATCH_SIZE_SIZE_BITS,DFWI_DEFAULT_EXPIRY,DFWI_DEFAULT_PACKET_COUNT,DFWI_DEFAULT_PACKET_SIZE)
//lint -esym(750,DFWI_DL_INIT_REQ_RE_REG,DFWI_VERIFY_REQ_ALL,DFWI_VERIFY_REQ_NONE,DFWI_APPLY_REQ_SCHD,)

#define NULL_CHAR                         ((uint8_t)0)

#define DFWI_APPLY_GRACE_PERIOD_MAX       ((uint16_t)1800)  /* Maximum time in seconds to wait prior to executing */

/* Reading Info (3B), reading type (2B) and reading value size for
   dfwStreamID (1B), dfwStatus (1B), dfwMissingPackets (2B) and dfwPacketIDs (DFWI_MISS_PKT_ID_RDG_SIZE in bytes)
*/
#define DFWI_MISS_PKT_ID_OVHD_BYTES       ((uint16_t)36)
#define DFWI_MISS_PKT_ID_RDG_SIZE         (APP_MSG_MAX_PAYLOAD - APP_HDR_SIZE - DFWI_MISS_PKT_ID_OVHD_BYTES)
#define DFWI_MISS_PKT_ID_ALLOC_SIZE       (DFWI_MISS_PKT_ID_OVHD_BYTES + DFWI_MISS_PKT_ID_RDG_SIZE)

/* Max reading value size available for dfwPacket:
   1245 avaiable for app (worst case IP through DTLS from 1280)

   4 - bytes consumed by the application header (not included in buffer for app message)
   1 - ExchangeWithID Event and Reading Qty's
   5 - ExchangeWithID Reading Header, dfwStreamId reading type
   1 - ExchangeWithID Reading value size for dfwStream Id
   5 - ExchangeWithID Reading Header, dfwPacketId reading type
   2 - ExchangeWithID Reading value size for dfwPacketId
   5 - ExchangeWithID Reading Header, dfwPacket reading type
   == = ===========================================================
   23 - bytes consumed by application up to dfwPacket reading value

   1201 = 1224 - 23 absolute maximum packet size available for download packets
*/

/* Max reading value size available for dfwMissingPacketID's:
   1224 avaiable for app (worst case IP through DTLS from 1280)

   4 - bytes consumed by the application header (not included in buffer for app message)
   4 - bytes consumed for the Interval End Time Stamp
   1 - ExchangeWithID Event and Reading Qty's
   5 - ExchangeWithID Reading Header, dfwStreamId reading type
   1 - ExchangeWithID Reading value size for dfwStream Id
   5 - ExchangeWithID Reading Header, dfwStatus reading type
   1 - ExchangeWithID Reading value size for dfwStatus
   5 - ExchangeWithID Reading Header, dfwMissingPacketQty reading type
   2 - ExchangeWithID Reading value size for dfwDupDiscardPacketQty
   5 - ExchangeWithID Reading Header, dfwDupDiscardPacketQty reading type
   == = ===========================================================
   40 - bytes consumed by application up to dfwPacket reading value
   (36 not including the application header)

   40 - Total bytes comsumed
   1184 = 1224 - 40 absoulute maximum packet size available for download packets
   This is the same for the Apply and Verify responses when returning missing packet ID's
*/

/*lint -esym(750,DFWI_MAX_FW_VER_RDG_VALUE_SIZE,DFWI_APPLY_GRACE_PERIOD_SIZE,DFWI_APPLY_GRACE_PERIOD_SIZE_BITS)   */
/*lint -esym(750,DFWI_PATCH_SIZE_SIZE,DFWI_PATCH_SIZE_SIZE_BITS,DFWI_DEFAULT_EXPIRY,DFWI_DEFAULT_PACKET_COUNT) */
/*lint -esym(750,DFWI_DEFAULT_PACKET_SIZE,DFWI_DEFAULT_REGISTER,DFWI_DL_INIT_REQ_STREAMID,DFWI_DL_INIT_REQ_APP_VER)   */
/*lint -esym(750,DFWI_DL_INIT_REQ_BL_VER,DFWI_DL_INIT_REQ_EXPIRY,DFWI_DL_INIT_REQ_PATCH_SIZE,DFWI_DL_INIT_REQ_PKT_SIZE)  */
/*lint -esym(750,DFWI_DL_INIT_REQ_PKT_SIZE,DFWI_DL_INIT_REQ_DEV_TYPE,DFWI_DL_INIT_REQ_RE_REG,DFWI_DL_INIT_REQ_GRACE) */
/*lint -esym(750,DFWI_DL_INIT_REQ_APP,DFWI_DL_INIT_REQ_BL,DFWI_DL_INIT_REQ_NONE,DFWI_DL_PKT_REQ_STREAMID,DFWI_DL_PKT_REQ_PKT_ID)   */
/*lint -esym(750,DFWI_DL_PKT_REQ_PKT,DFWI_DL_PKT_REQ_ALL,DFWI_DL_PKT_REQ_NONE,DFWI_VERIFY_REQ_ALL) */
/*lint -esym(750,DFWI_VERIFY_REQ_NONE,DFWI_APPLY_REQ_STREAMID,DFWI_APPLY_REQ_SCHD,DFWI_APPLY_REQ_ALL) */
/*lint -esym(750,DFWI_APPLY_REQ_NONE,DFWI_MAX_REPORT_MISS_PKT_IDS,MAX_MISSING_PKT_ID_STR,VERIFY_RESP_LEN)   */
/*lint -esym(750,VERIFY_RESP_STATUS,VERIFY_RESP_STATE,VERIFY_RESP_ERROR,VERIFY_RESP_PCKTCNT,VERIFY_RESP_MPCKTCNT) */
/*lint -esym(750,VERIFY_RESP_PCKTIDS,DFW_PRNT_INFO,DFW_PRNT_INFO,DFW_PRNT_WARN,DFW_PRNT_WARN,DFW_PRNT_ERROR)   */
/*lint -esym(750,DFW_PRNT_ERROR) */
/* Max comDeviceFirmwareVersion & comDeviceBootloaderVersion string length (“255.255.65535”) does not include a NULL */
#define DFWI_MAX_FW_VER_RDG_VALUE_SIZE    ((uint8_t)13)

#define DFWI_APPLY_GRACE_PERIOD_SIZE      ((uint8_t)2)
#define DFWI_APPLY_GRACE_PERIOD_SIZE_BITS (DFWI_APPLY_GRACE_PERIOD_SIZE * 8)
#define DFWI_PATCH_SIZE_SIZE              ((uint8_t)3)
#define DFWI_PATCH_SIZE_SIZE_BITS         (DFWI_PATCH_SIZE_SIZE * 8)
#define DFWI_DEFAULT_EXPIRY               ((uint32_t)0xFFFFFFFF)
#define DFWI_DEFAULT_PACKET_COUNT         ((uint16_t)0)
#define DFWI_DEFAULT_PACKET_SIZE          ((uint16_t)0)
#define DFWI_DEFAULT_REGISTER             ((uint8_t)0)

#define DFWI_DL_INIT_REQ_STREAMID      ((uint16_t)0x8000)
#define DFWI_DL_INIT_REQ_APP_VER       ((uint16_t)0x4000)
#define DFWI_DL_INIT_REQ_BL_VER        ((uint16_t)0x2000)
#define DFWI_DL_INIT_REQ_EXPIRY        ((uint16_t)0x1000)
#define DFWI_DL_INIT_REQ_PATCH_SIZE    ((uint16_t)0x0800)
#define DFWI_DL_INIT_REQ_PKT_SIZE      ((uint16_t)0x0400)
#define DFWI_DL_INIT_REQ_DEV_TYPE      ((uint16_t)0x0200)
#define DFWI_DL_INIT_REQ_RE_REG        ((uint16_t)0x0100)   /* Optional */
#define DFWI_DL_INIT_REQ_GRACE         ((uint16_t)0x0080)
#define DFWI_DL_INIT_REQ_CIPHER        ((uint16_t)0x0040)
#define DFWI_DL_INIT_REQ_TO_FW_VER     ((uint16_t)0x0020)
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
#define DFWI_DL_INIT_REQ_ANSI_TBL_OID  ((uint16_t)0x0010)
#endif
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#define DFWI_DL_INIT_REQ_ED_FW_VERSION ((uint16_t)0x0008)
#define DFWI_DL_INIT_REQ_ED_MODEL      ((uint16_t)0x0004)
#endif
#define DFWI_DL_INIT_REQ_APP           ((uint16_t)0xDEE0)
#define DFWI_DL_INIT_REQ_BL            ((uint16_t)0xBEE0)
#define DFWI_DL_INIT_REQ_METER_CFG     ((uint16_t)0x9ED0)
#define DFWI_DL_INIT_REQ_METER_FW_PATCH ((uint16_t)0x9ECC)
#define DFWI_DL_INIT_REQ_METER_FW_BASE ((uint16_t)0x9ECC)
#define DFWI_DL_INIT_REQ_NONE          ((uint16_t)0x0000)

#define DFWI_DL_PKT_REQ_STREAMID    ((uint8_t)0x80)
#define DFWI_DL_PKT_REQ_PKT_ID      ((uint8_t)0x40)
#define DFWI_DL_PKT_REQ_PKT         ((uint8_t)0x20)
#define DFWI_DL_PKT_REQ_ALL         ((uint8_t)0xE0)
#define DFWI_DL_PKT_REQ_NONE        ((uint8_t)0x00)

#define DFWI_VERIFY_REQ_ALL          ((uint8_t)0x00)
#define DFWI_VERIFY_REQ_NONE         ((uint8_t)0x00)

#define DFWI_APPLY_REQ_STREAMID     ((uint8_t)0x80)
#define DFWI_APPLY_REQ_SCHD         ((uint8_t)0x40)   /* Optional */
#define DFWI_APPLY_REQ_ALL          ((uint8_t)0x80)
#define DFWI_APPLY_REQ_NONE         ((uint8_t)0x00)

#define DFWI_MAX_REPORT_MISS_PKT_IDS ((uint16_t)400)
#define MAX_MISSING_PKT_ID_STR       ((uint8_t)7)   /* max packet ID string segment length for ",65535" + NULL */

#define VERIFY_RESP_LEN          ((uint8_t)7)   /* Resp Len of verify cmd for MFG port, not including missing pckts. */
#define VERIFY_RESP_STATUS       ((uint8_t)0)   /* Resp offset for status field */
#define VERIFY_RESP_STATE        ((uint8_t)1)   /* Resp offset for state field */
#define VERIFY_RESP_ERROR        ((uint8_t)2)   /* Resp offset for error field */
#define VERIFY_RESP_PCKTCNT      ((uint8_t)3)   /* Resp offset for packet count field */
#define VERIFY_RESP_MPCKTCNT     ((uint8_t)5)   /* Resp offset for missing packet count field */
#define VERIFY_RESP_PCKTIDS      ((uint8_t)7)   /* Resp offset for packet ID fields */

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
//#define DFW_PRNT_ERROR_PARMS     ERR_printf
#else
#define DFW_PRNT_ERROR(x)
//#define DFW_PRNT_ERROR_PARMS(x,...)
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
/* eDfwHeEpInitResp_t must be aligned with DfwHEEPStatusStrings indexes */
typedef enum
{
   eDFW_STATUS_READING_NOT_INITIALIZED = ((uint8_t)0),   // Status Response Index for “NotInitialized”
   eDFW_STATUS_READING_INIT_WAIT_FOR_PACKETS,            // Status Response Index for “InitializedWaitForPackets”
   eDFW_STATUS_READING_INIT_WAIT_FOR_APPLY,              // Status Response Index for “InitializedWaitForApply”
   eDFW_STATUS_READING_INIT_WITH_ERROR                   // Status Response Index for “InitializedWithError”
}eDfwHeEpInitResp_t;

typedef struct
{
   meterReadingType        dfwErrorCode;
   ReadingsValueTypecast   dfwErrorTypecast;
}dfwErrorcodeLUT_t;

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

#if ENABLE_PRNT_DFW_INFO
/* eDfwHeEpInitResp_t must be aligned with DfwHEEPStatusStrings indexes */
/* lint -e{64}  type are the same */
static const char * const DfwHEEPStatusStrings[] =
{
   "NotInitialized",
   "InitializedWaitForPackets",
   "InitializedWaitForApply",
   "InitializedWithError",
   "ApplyingDownload"
};

/* eDfwPatchState_t must be aligned with DwfStateStrings indexes */
/* lint -e{64}  type are the same */
static const char * const DwfStateStrings[] =
{
   "Idle",
   "ExecPgmCommands",
   "WaitForApply",
   "DoFlashSwap",
   "ExecNvCommands",
   "CopyNvImageToNv",
   "VerifySwap",
   "CompatibilityTest",
   "ProgramScript",
   "AuditTest"
};

/* eDlStatus_t must be aligned with DfwStatusStrings indexes */
/* lint -e{64}  type are the same */
static const char * const DfwStatusStrings[] =
{
   "NotInitialized",
   "InitializedIncomplete",
   "InitializedComplete",
   "InitializedWithError"
};

/* eDfwApplyMode_t must be aligned with DwfApplyModeStrings indexes */
/* lint -e{64}  type are the same */
static const char * const DwfApplyModeStrings[] =
{
   "DoNothing",
   "PrePatch",
   "FullApply"
};

/* eDfwErrorCodes_t must be aligned with DwfErrorCodeStrings indexes */
/* lint -e{64}  type are the same */
static const char * const DwfErrorCodeStrings[] =
{
   "No Errors",
   "Patch format",
   "FW Version",
   "DeviceType",
   "Patch Length",
   "Invalid Time",
   "Patch Expired",
   "Patch CRC",
   "Incomplete",
   "Image CRC",
   "Schedule",
   "Stream ID",
   "Signature",
   "Decryption",
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
   "DfwCompatiblityTest",
   "DfwProgramScript",
#endif
   "NV",
   "Flash",
   "Patch Command",
   "Invalid State",
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
   "DfwAuditTest"
#endif
};
#endif
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */
#if ( ( EP == 1 ) &&( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
static bool             isValidAnsiTableOID( const uint8_t *reqAnsiTableOidBuf );
#endif
#if ( ( EP == 1 ) &&( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static bool             isValidEdModel( const uint8_t *reqEdModelBuf, uint8_t bufSize );
#endif
static bool             isFirmwareVersionSupported( const firmwareVersion_u *pFirmwareVersion, eFwTarget_t target, dl_dfwFileType_t fileType );
static bool             isDownloadTimeExpired( void );
static returnStatus_t   dlPcktWriteToNv(uint16_t packetNum, const uint8_t *Data, uint16_t  packetSize,
                                        uint8_t beginSector, eDlStatus_t status, uint16_t packetSizeMultiplier);
static dfwErrorcodeLUT_t dfwErrorLookUp( const DFW_vars_t *dfwVars );
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
static uint8_t packDfwFileSectionStatusParmeters( DFW_vars_t *dfwVars, buffer_t *pBuf, pack_t *packCfg);
#endif
static bool isValidPatchSize( dl_patchsize_t patchSize, eDfwCiphers_t cipher, dl_dfwFileType_t reqFileType );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */
typedef struct
{
   meterReadingType      type;
   enum_CIM_QualityCode  qualityCode;
   uint8_t              *pvalue;
   uint16_t              rdgSize;
   ReadingsValueTypecast typecast;
   uint8_t               byteSwap; //If true then byte swap pvalue when packing into buffer
   pack_t               *pPackCfg;
}simpleReading_t;

static bool           getDeviceType( char *devTypeBuff, uint8_t devTypeBuffSize, uint8_t *devTypeLen );
static returnStatus_t erasePatchSpace( dl_patchsize_t patchSize, eDfwCiphers_t encrypted );
static uint16_t       packReading(simpleReading_t *rdgInfo);
static uint8_t        packMissingPackets( dl_packetcnt_t numToPack,  dl_packetid_t  *pmPData, pack_t *ppackCfg,
                                          buffer_t *pPktBuf );

/*! ********************************************************************************************************************

   \fn void DFWI_DisplayStatus(void)

   \brief This is used to display the current DFW status to the debug port

   \param  None

   \return  None

   Side Effects: None

   Reentrant Code: Yes

   Notes:

 ******************************************************************************************************************** */
void DFWI_DisplayStatus(void)
{
   DFW_vars_t           dfwVars;           // Contains all of the DFW file variables
   sysTime_t            schedSysTime;
   sysTime_t            expireSysTime;
   sysTime_dateFormat_t schedDateTime;
   sysTime_dateFormat_t expireDataTime;

   (void)DFWA_getFileVars(&dfwVars);                // Read the DFW NV variables (status, etc...)
#if ENABLE_PRNT_DFW_INFO
   uint8_t              errorIndex;
   errorIndex = (uint8_t)dfwVars.eErrorCode;
   if (eDFWE_FATAL_ERRORS <= dfwVars.eErrorCode)
   {  //Adjust for gap between normal and fatal errors
      /*lint -e{656} arithmetic operaion on compatible enums */
      /*lint -e{641} enum to int */
      errorIndex = (uint8_t)(dfwVars.eErrorCode - DFWA_FATAL_ERROR_OFFSET);
   }
   DFW_PRNT_INFO_PARMS("StreamID: %hhu, Status: %s, State: %s, Mode: %s, Error: %s",
                        dfwVars.initVars.streamId, DfwStatusStrings[dfwVars.eDlStatus],
                        DwfStateStrings[dfwVars.ePatchState], DwfApplyModeStrings[dfwVars.applyMode],
                        DwfErrorCodeStrings[errorIndex]);
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
   if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
   {
      // display the DFW file section status variables
      DFW_PRNT_INFO_PARMS("dfwCompatibilityTestStatus: %hhu, dfwProgramScriptStatus: %hhu, dfwAuditTestStatus: %hhu",
                           dfwVars.fileSectionStatus.compatibilityTestStatus,
                           dfwVars.fileSectionStatus.programScriptStatus,
                           dfwVars.fileSectionStatus.auditTestStatus );
   }
#endif // ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
#endif // #endif ENABLE_PRNT_DFW_INFO

   //Convert UTC time (seconds from epoch) to SystemTime (date & time)
   TIME_UTIL_ConvertSysCombinedToSysFormat(&dfwVars.applySchedule, &schedSysTime);
   TIME_UTIL_ConvertSysCombinedToSysFormat(&dfwVars.initVars.expirationTime, &expireSysTime);
   //Convert SystemTime (date & time) to calandar format mm/dd/yyyy hh:mm:ss
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&schedSysTime, &schedDateTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&expireSysTime, &expireDataTime);
   /*lint -e(123) 'min' is a struct element, not a macro */
   DFW_PRNT_INFO_PARMS("Schedule: %02d/%02d/%04d %02d:%02d:%02d, Expires: %02d/%02d/%04d %02d:%02d:%02d",
                        schedDateTime.month,  schedDateTime.day,  schedDateTime.year,
                        schedDateTime.hour,  schedDateTime.min,  schedDateTime.sec,
                        expireDataTime.month, expireDataTime.day, expireDataTime.year,
                        expireDataTime.hour, expireDataTime.min, expireDataTime.sec);
   DFW_PRNT_INFO_PARMS("Grace Period: %u, Register: %hhu",
                        dfwVars.initVars.gracePeriodSec, dfwVars.initVars.bRegister);
   if (eDFWS_INITIALIZED_INCOMPLETE == dfwVars.eDlStatus)
   {
      uint16_t missingPckts;

      if ( eSUCCESS == DFWP_getMissingPackets(dfwVars.initVars.packetCount,
                                              REPORT_NO_PACKETS, &missingPckts, NULL) )
      {
         DFW_PRNT_INFO_PARMS("Missing Packet Count: %hu of %hu", missingPckts, dfwVars.initVars.packetCount);
      }
   }
}

/***********************************************************************************************************************

   Function name: DFWI_Init_MsgHandler

   Purpose: Interface between /df/in request and DFW application for DownloadInit command.

   Arguments:
      APP_MSG_Rx_t *appMsgInfo - Pointer to application header structure from head end
      void *payloadBuf         - pointer to data payload (in this case a ExchangeWithID structure)
      uint16_t length          - Payload legnth

   Returns: none

   Side Effects: None

   Reentrant Code: No

   Notes:
      payloadBuf will be deallocated by the calling routine.

 **********************************************************************************************************************/
//lint -esym(715, payloadBuf) not referenced
//lint -efunc(818, DFWI_Init_MsgHandler) arguments cannot be const because of API requirements
void DFWI_Init_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   (void)length;
   HEEP_APPHDR_t              appMsgTxInfo;  // Application header/QOS info
   uint16_t                   bits=0;        // Keeps track of the bit position within the Bitfields
   uint16_t                   bytes=0;       // Keeps track of byte position within Bitfields
   uint8_t                    i;             // loop index
   uint32_t                   utcTime;       // UTC time in seconds since epoch
   uint32_t                   packetCount;   // Used to determine the packet count from Patch and Packet sizes
   EventRdgQty_u              qtys;          // For EventTypeQty and ReadingTypeQty
   ExWId_ReadingInfo_u        readingInfo;   // Reading Info for the reading Type and Value
   meterReadingType           readingType;   // Reading Type
   uint16_t                   rdgSize;       // For the reading value size
   buffer_t                  *pBuf=NULL;     // pointer to message
   uint16_t                   reqdReadings;  // Bit filed for ensuring all the required reading values are received
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   uint8_t                    tempBuf[EDCFG_FW_VERSION_LENGTH+1]; //Used to hold variable reading types in response
#else
   uint8_t                    tempBuf[COM_DEVICE_TYPE_LEN+1]; //Used to hold variable reading types in response
#endif

   /* The following variable will be used by the DFW process to validate reading types not stored as a DFW variable. */
   DFWI_nonStoredDfwInitReadings_t nonStoredReadings;
   (void)memset((uint8_t *)&nonStoredReadings, 0, sizeof(nonStoredReadings));

   DFWI_dlInit_t              initData;
   DFWI_eInitResponse_t       response = eINIT_RESP_RCE_INITIALIZED; // Need a default value to shut up lint

   //Application header/QOS info (cannot set Method_Status yet)
   HEEP_initHeader( &appMsgTxInfo );
   appMsgTxInfo.TransactionType = (uint8_t)TT_RESPONSE;
   appMsgTxInfo.Resource        = (uint8_t)appMsgRxInfo->Resource;
   appMsgTxInfo.Req_Resp_ID     = (uint8_t)appMsgRxInfo->Req_Resp_ID;
   appMsgTxInfo.qos             = (uint8_t)appMsgRxInfo->qos;
   appMsgTxInfo.callback        = NULL;
   appMsgTxInfo.TransactionContext = appMsgRxInfo->TransactionContext;

   if ( method_put == (enum_MessageMethod)appMsgRxInfo->Method_Status)
   { //Parse the packet payload - an implied ExchangeWithID bit structure

      qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
      bytes           += EVENT_AND_READING_QTY_SIZE;
      //Do not allow events
      if (0 != qtys.bits.eventQty)
      {
         appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
         qtys.bits.rdgQty           = 0; //Set to zero to skip parsing the readings
      }
      (void)memset(&initData, 0, sizeof(initData));

      // Register  and filetype are optional parameters so set default here in case we do not get the reading
      initData.bRegister         = DFWI_DEFAULT_REGISTER;
      nonStoredReadings.fileType = DFW_DEFAULT_PATCH_FILE_TYPE;
      reqdReadings               = DFWI_DL_INIT_REQ_NONE;
      //For each Reading
      for (i=0; i < qtys.bits.rdgQty; i++)
      {
         (void)memcpy(&readingInfo.bytes[0], &((uint8_t *)payloadBuf)[bytes], sizeof(readingInfo.bytes));
         bits       += READING_INFO_SIZE_BITS;
         bytes      += READING_INFO_SIZE;
         readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);
         bytes      += READING_TYPE_SIZE;
         if (readingInfo.bits.tsPresent)
         {
            bits  += READING_TIME_STAMP_SIZE_BITS;   //Skip over since we do not use/need it
            bytes += READING_TIME_STAMP_SIZE;
         }
         if (readingInfo.bits.RdgQualPres)
         {
            bits  += READING_QUALITY_SIZE_BITS; //Skip over since we do not use/need it
            bytes += READING_QUALITY_SIZE;
         }
         //Drop message if it is mal-formed, assume BadRequest status
         appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
         /*lint -e{732} bit struct to uint16 */
         rdgSize = readingInfo.bits.RdgValSizeU;
         rdgSize = ((rdgSize << RDG_VAL_SIZE_U_SHIFT) & RDG_VAL_SIZE_U_MASK) | readingInfo.bits.RdgValSize;
         switch (readingType) /*lint !e788 not all enums used within switch */
         {
            case comDeviceFirmwareVersion:
            case comDeviceBootloaderVersion:
               //HEEP Spec's as Var size so as long as it is at least 1 byte and fits in our destination size it is OK
               if ( (0 != rdgSize) && (DFWI_MAX_FW_VER_RDG_VALUE_SIZE >= rdgSize) &&
                    (ASCIIStringValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  /* Parse the Command string received into a decimal value */
                  /*lint -e{740} dest cast in this way to emphasize what it is pointing to */
                  /*lint -e{826} pointer to pointer conversions */
                  (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
                  (void)memcpy(&tempBuf[0], (uint8_t *)payloadBuf+bytes, rdgSize);
                  if ( 3 == sscanf( (char *)&tempBuf[0], "%03hhu.%03hhu.%05hu",
                                    &initData.firmwareVersion.field.version,
                                    &initData.firmwareVersion.field.revision,
                                    &initData.firmwareVersion.field.build ) )
                  {
                     bits                      += (uint16_t)(rdgSize * 8);
                     bytes                     += rdgSize;
                     appMsgTxInfo.Method_Status = (uint8_t)OK;
                     //Can have one or the other, but not both... reqdReadings check later will ensure this condition
                     if ( comDeviceFirmwareVersion == readingType )
                     {
                        reqdReadings   |= DFWI_DL_INIT_REQ_APP_VER;
                        initData.target = eFWT_APP;
                     }
                     else if ( comDeviceBootloaderVersion == readingType )
                     {
                        reqdReadings   |= DFWI_DL_INIT_REQ_BL_VER;
                        initData.target = eFWT_BL;
                     }
                  }
               }
               break;
            case comDeviceType:
               (void)memset(&initData.comDeviceType[0], NULL_CHAR, sizeof(initData.comDeviceType));
               //HEEP Spec's as Var size so as long as it is at least 1 byte and fits in our destination size it is OK
               if ( (0 != rdgSize) && (sizeof(initData.comDeviceType) >= rdgSize) &&
                    (ASCIIStringValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  (void)memcpy(&initData.comDeviceType[0], &((char *)payloadBuf)[bytes], rdgSize);
                  bits                      += (uint16_t)(rdgSize * 8);
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_DEV_TYPE;
               }
               break;
            case dfwPatchSize:
               //HEEP Spec's as Var size so as long as it is at least 1 byte and fits in our destination size it is OK
               if ( (0 != rdgSize) && (DFWI_PATCH_SIZE_SIZE >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  initData.patchSize         = UNPACK_bits2_uint32(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_PATCH_SIZE;
               }
               break;
            case dfwPacketSize:
               //HEEP Spec's as 2 bytes
               if ( (0 != rdgSize) && (sizeof(initData.packetSize) >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  initData.packetSize        = UNPACK_bits2_uint16(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_PKT_SIZE;
               }
               break;
            case dfwExpiry:
               //HEEP Spec's this as 4, need to convert to expirationTime (8 bytes)
               if ( (0 != rdgSize) && (sizeof(utcTime) >= rdgSize) &&
                    (dateTimeValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  utcTime                    = UNPACK_bits2_uint32(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  initData.expirationTime    = TIME_TICKS_PER_SEC * (uint64_t)utcTime;
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_EXPIRY;
               }
               break;
            case dfwStreamID:
               //HEEP Spec's as 1 byte
               if ( (0 != rdgSize) && (sizeof(initData.streamId) >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  initData.streamId          = UNPACK_bits2_uint8(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_STREAMID;
               }
               break;
            case dfwApplyGracePeriod:
               //HEEP spec's as 2 bytes
               if ( (0 != rdgSize) && (DFWI_APPLY_GRACE_PERIOD_SIZE >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  initData.gracePeriodSec    = UNPACK_bits2_uint32(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_GRACE;
               }
               break;
            case dfwRegisterDevice:
               //HEEP Spec's as 1 or 0 bytes (no value sent)
               if ( (sizeof(initData.bRegister) >= rdgSize) &&
                    (Boolean == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  if (0 == rdgSize)
                  {
                     initData.bRegister = true;
                  }
                  else
                  {
                     initData.bRegister = UNPACK_bits2_uint8(payloadBuf, &bits, (uint8_t)(rdgSize * 8)) == 0? false: true;
                  }
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
               }
               break;
            case dfwFileType:
               //HEEP Spec's as 1 byte  uint value(optional reading type)
               if ( (rdgSize != 0) && (sizeof(nonStoredReadings.fileType) >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  nonStoredReadings.fileType = UNPACK_bits2_uint8(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  bytes                     += rdgSize;
               }
               break;
            case dfwCipherSuite:
               //HEEP Spec's as 1
               if ( (0 != rdgSize) && (sizeof(initData.cipher) >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  initData.cipher            = (eDfwCiphers_t)UNPACK_bits2_uint8(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_CIPHER;
               }
               break;
            case dfwToVersion:
               //HEEP Spec's as Var size so as long as it is at least 1 byte and fits in our destination size it is OK
               if ( (0 != rdgSize) && (DFWI_MAX_FW_VER_RDG_VALUE_SIZE >= rdgSize) &&
                    (ASCIIStringValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
                  /* Parse the Command string received into a decimal value */
                  /*lint -e{740} dest cast in this way to emphasize what it is pointing to */
                  /*lint -e{826} pointer to pointer conversions */
                  (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
                  (void)memcpy(&tempBuf[0], (uint8_t *)payloadBuf+bytes, rdgSize);
                  if (3 == sscanf( (char *)&tempBuf[0], "%03hhu.%03hhu.%05hu",
                                   &initData.toFwVersion.field.version,
                                   &initData.toFwVersion.field.revision,
                                   &initData.toFwVersion.field.build ) )
                  {
                     bits                      += (uint16_t)(rdgSize * 8);
                     bytes                     += rdgSize;
                     appMsgTxInfo.Method_Status = (uint8_t)OK;
                     reqdReadings              |= DFWI_DL_INIT_REQ_TO_FW_VER;
                  }
               }
               break;
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
            case ansiTableOID:
               //HEEP Spec's this as 4
               if ( (EDCFG_MANUFACTURER_LENGTH == rdgSize) && (hexBinary == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  bits = UNPACK_bits2_ucArray(payloadBuf, bits, nonStoredReadings.reqEndDeviceReadings.endDeviceAnsiTblOID, (uint8_t)(rdgSize * 8));
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_DL_INIT_REQ_ANSI_TBL_OID;
               }
               break;
#endif
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
            case edModel:
               //HEEP Spec's this as 8 bytes ascii string value
               if ( (rdgSize != 0) && (EDCFG_MODEL_LENGTH >= rdgSize ) &&
                    (ASCIIStringValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  (void)memcpy(&nonStoredReadings.reqEndDeviceReadings.endDeviceModel[0], &((char *)payloadBuf)[bytes], rdgSize);
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  bits                      += (uint16_t)(rdgSize * 8);
                  bytes                     += rdgSize;
                  reqdReadings              |= DFWI_DL_INIT_REQ_ED_MODEL;
               }
               break;
            case edFwVersion:
               //HEEP Spec's as variable sized ascii string up to 23 bytes
               if ( (rdgSize != 0) && (EDCFG_FW_VERSION_LENGTH >= rdgSize) &&
                    (ASCIIStringValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
                  (void)memcpy(&tempBuf[0], (uint8_t *)payloadBuf+bytes, rdgSize);
                  if ( 6 == sscanf( (char *)&tempBuf[0], "%03hhu.%03hhu.%03hu;%03hhu.%03hhu.%03hu",
                                    &initData.firmwareVersion.field.version,
                                    &initData.firmwareVersion.field.revision,
                                    &initData.firmwareVersion.field.build,
                                    &initData.toFwVersion.field.version,
                                    &initData.toFwVersion.field.revision,
                                    &initData.toFwVersion.field.build ) )
                  {
                     appMsgTxInfo.Method_Status = (uint8_t)OK;
                     bits                      += (uint16_t)(rdgSize * 8);
                     bytes                     += rdgSize;
                     reqdReadings              |= DFWI_DL_INIT_REQ_ED_FW_VERSION;
                     initData.target            = eFWT_METER_FW;
                  }
               }
               break;
#endif
            default:
               //Invalid reading type, drop the entire message
               appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
               break;
         }  //lint !e788  Some enums in meterReadingType aren't used. //End of switch on reading type
         if ((uint8_t)OK != appMsgTxInfo.Method_Status)
         {
            break;   //Abort loop to drop the message
         }
      }  //End of for each reading type

      // We need to verify the proper requested reading types were supplied based upon dfwFileType provided
      switch( nonStoredReadings.fileType )
      {
         case DFW_ACLARA_METER_PROGRAM_FILE_TYPE:
            if( DFWI_DL_INIT_REQ_METER_CFG != reqdReadings )
            {  // we did not receive the expected reading types, indicate bad request
               appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
            }
           break;
         case DFW_ACLARA_METER_FW_PATCH_FILE_TYPE:
            if( DFWI_DL_INIT_REQ_METER_FW_PATCH != reqdReadings )
            {  // we did not receive the expected reading types, indicate bad request
               appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
            }
           break;
         case DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE:
            if( DFWI_DL_INIT_REQ_METER_FW_BASE != reqdReadings )
            {  // we did not receive the expected reading types, indicate bad request
               appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
            }
            break;
         case DFW_ENDPOINT_FIRMWARE_FILE_TYPE:
            if ( (DFWI_DL_INIT_REQ_APP != reqdReadings) && (DFWI_DL_INIT_REQ_BL != reqdReadings) )
            {  // we did not receive the expected reading types, indicate bad request
               appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
            }
           break;
         default:
           appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
      }

      if ((uint8_t)OK == appMsgTxInfo.Method_Status)   //Got all the readings, where they good?
      {
         // round up to get the expected count if greater than max available size then saturate the value
         //I'm sure this will have lint issues...
         if ( initData.packetSize > 0 )
         {
            packetCount = (initData.patchSize + initData.packetSize - 1) / initData.packetSize;
            initData.packetCount = (uint16_t)packetCount;
            if (UINT16_MAX < packetCount)
            {
               initData.packetCount = UINT16_MAX;
            }
         }

         response = DFWI_DownloadInit( &initData, &nonStoredReadings );

         if (eINIT_RESP_APPLY_IN_PROCESS != response)
         {
            //Build FullMeterRead response
            sysTimeCombined_t    currentTime;   // Get current time
            pack_t               packCfg;       // Struct needed for PACK functions
            simpleReading_t      simpleRdgInfo; // Struct for Reading Info, type and value
            eDfwHeEpInitResp_t   status;
            firmwareVersion_u    ver;           // For the current firmware version
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) || ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) ) )
		    ValueInfo_t reading; // declare variable to retrieve host meter readings
#endif

            appMsgTxInfo.Method_Status = (uint8_t)ServiceUnavailable;
            //Allocate a buffer big enough to accomadate worst case payload.
            /* FullMeterRead bit structure with No Events and Three Readings:
               one with a readingType of dfwStreamID and a readingValue of StreamID,
               one with a readingType of dfwStatus and a readingValue of status
               one (only if status is NotInitialized) with a readingType of the error and its readingValue
               The longest error type readingValue is COM_DEVICE_TYPE_LEN
            */
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
            pBuf = BM_alloc( VALUES_INTERVAL_END_SIZE                      +
                             EVENT_AND_READING_QTY_SIZE                    +
                             3 * ( READING_INFO_SIZE + READING_TYPE_SIZE ) +
                             sizeof(initData.streamId)                     +
                             sizeof(status)                                +
                             EDCFG_FW_VERSION_LENGTH );
#else
            pBuf = BM_alloc( VALUES_INTERVAL_END_SIZE                      +
                             EVENT_AND_READING_QTY_SIZE                    +
                             3 * ( READING_INFO_SIZE + READING_TYPE_SIZE ) +
                             sizeof(initData.streamId)                     +
                             sizeof(status)                                +
                             COM_DEVICE_TYPE_LEN );
#endif
            if ( pBuf != NULL )
            {
               appMsgTxInfo.Method_Status = (uint8_t)OK; //Response is now good to go
               PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

               //Insert current timestamp for Values Interval End
               (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
               utcTime = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
               PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&utcTime, &packCfg );
               pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

               qtys.EventRdgQty = 0;
               //Insert the Event & Reading Quantity, it will be adjusted later to the correct value
               PACK_Buffer( sizeof(qtys) * 8, (uint8_t *)&qtys.EventRdgQty, &packCfg );
               pBuf->x.dataLen += sizeof(qtys);

               /* initialize the quality code value for all readings packed.  No need to update quality code
                  value for each reading since data source is from dfwVARS.  Quality code will only need to
                  updated if getting values from an endDevice. */
               simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;

               //Insert StreamID
               simpleRdgInfo.type     = dfwStreamID;
               simpleRdgInfo.pvalue   = (uint8_t *)&initData.streamId;
               simpleRdgInfo.rdgSize  = sizeof(initData.streamId);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               //Insert Status
               status = eDFW_STATUS_READING_NOT_INITIALIZED;
               if (eINIT_RESP_RCE_INITIALIZED == response)
               {
                  status = eDFW_STATUS_READING_INIT_WAIT_FOR_PACKETS;
               }
               simpleRdgInfo.type     = dfwStatus;
               simpleRdgInfo.pvalue   = (uint8_t *)&status;
               simpleRdgInfo.rdgSize  = sizeof(status);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               DFW_PRNT_INFO_PARMS("StreamID=%3d, Status=%s", initData.streamId,
                                   (uint8_t *)DfwHEEPStatusStrings[status]);

               //Optionally send back ErrorReason (dependent on status)
               if (eINIT_RESP_RCE_INITIALIZED != response)
               {
                  appMsgTxInfo.Method_Status = (uint8_t)BadRequest; //Update Status response for invalid message parameter
                  (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
                  switch (response)
                  {
                     case (eINIT_RESP_FW_MISMATCH):
                        simpleRdgInfo.type     = comDeviceFirmwareVersion; // Assume target is Application FW
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
                        if( eFWT_METER_FW == initData.target )
                        {
                           simpleRdgInfo.type     = edFwVersion;
                           (void)INTF_CIM_CMD_getMeterReading( edFwVersion, &reading );
                           simpleRdgInfo.pvalue   = &reading.Value.buffer[0];
                           simpleRdgInfo.rdgSize = reading.valueSizeInBytes;
                           simpleRdgInfo.typecast = reading.typecast;
                        }
                        else
#endif
                        {
                           ver = VER_getFirmwareVersion(initData.target);
                           (void)sprintf((char *)&tempBuf[0],"%hhu.%hhu.%hu", ver.field.version, ver.field.revision, ver.field.build);
                           if (eFWT_BL == initData.target)
                           {
                              simpleRdgInfo.type = comDeviceBootloaderVersion;
                           }
                           simpleRdgInfo.rdgSize  = (uint16_t)strlen((char *)&tempBuf[0]);
                           simpleRdgInfo.pvalue   = &tempBuf[0];
                           simpleRdgInfo.typecast = ASCIIStringValue;
                        }
                        simpleRdgInfo.byteSwap = false;
                        break;
                     case (eINIT_RESP_DEV_TYPE_MISMATCH):
                        (void)getDeviceType((char *)&tempBuf[0], sizeof(tempBuf), (uint8_t *)&simpleRdgInfo.rdgSize);
                        simpleRdgInfo.type     = comDeviceType;
                        simpleRdgInfo.pvalue   = &tempBuf[0];
                        simpleRdgInfo.typecast = ASCIIStringValue;
                        simpleRdgInfo.byteSwap = false;
                        break;
                     case (eINIT_RESP_PACKET_SIZE_OUT_OF_BOUNDS):
                        simpleRdgInfo.type     = dfwPacketSize;
                        simpleRdgInfo.pvalue   = (uint8_t *)&initData.packetSize;
                        simpleRdgInfo.rdgSize  = sizeof(initData.packetSize);
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = true;
                        simpleRdgInfo.pPackCfg = &packCfg;
                        break;
                     case (eINIT_RESP_PACKET_COUNT_OUT_OF_BOUNDS):
                        simpleRdgInfo.type     = dfwPacketCount;
                        simpleRdgInfo.pvalue   = (uint8_t *)&initData.packetCount;
                        simpleRdgInfo.rdgSize  = sizeof(initData.packetCount);
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = true;
                        break;
                     case (eINIT_RESP_PATCH_OUT_OF_BOUNDS):
                        simpleRdgInfo.type     = dfwPatchSize;
                        simpleRdgInfo.pvalue   = (uint8_t *)&initData.patchSize;
                        simpleRdgInfo.rdgSize  = DFWI_PATCH_SIZE_SIZE;
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = true;
                        break;
                     case (eINIT_RESP_EXPIRY_OUT_OF_BOUNDS):
                        utcTime                = (uint32_t)(initData.expirationTime/TIME_TICKS_PER_SEC);
                        simpleRdgInfo.type     = dfwExpiry;
                        simpleRdgInfo.pvalue   = (uint8_t *)&utcTime;
                        simpleRdgInfo.rdgSize  = sizeof(utcTime);
                        simpleRdgInfo.typecast = dateTimeValue;
                        simpleRdgInfo.byteSwap = true;
                        break;
                     case (eINIT_RESP_GRACE_PERIOD_OUT_OF_BOUNDS):
                        simpleRdgInfo.type     = dfwApplyGracePeriod;
                        simpleRdgInfo.pvalue   = (uint8_t *)&initData.gracePeriodSec;
                        simpleRdgInfo.rdgSize  = DFWI_APPLY_GRACE_PERIOD_SIZE;
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = true;
                        break;
                     case (eINIT_RESP_CIPHER_OUT_OF_BOUNDS):
                        simpleRdgInfo.type     = dfwCipherSuite;
                        simpleRdgInfo.pvalue   = (uint8_t *)&initData.cipher;
                        simpleRdgInfo.rdgSize  = sizeof(initData.cipher);
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = false;
                        break;
                     case ( eINIT_RESP_INVALID_DFW_FILE_TYPE ):
                        simpleRdgInfo.type     = dfwFileType;
                        simpleRdgInfo.pvalue   = (uint8_t *)&nonStoredReadings.fileType;
                        simpleRdgInfo.rdgSize  = sizeof(nonStoredReadings.fileType);
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = false;
                        break;
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
                     case ( eINIT_RESP_ANSI_TABLE_OID_MISMATCH ):
                        simpleRdgInfo.type     = ansiTableOID;
                        (void)INTF_CIM_CMD_getMeterReading( ansiTableOID, &reading );
                        simpleRdgInfo.pvalue   = &reading.Value.buffer[0];
                        simpleRdgInfo.rdgSize  = reading.valueSizeInBytes;
                        simpleRdgInfo.typecast = hexBinary;
                        simpleRdgInfo.byteSwap = false;
                        break;
#endif
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
                     case (eINIT_RESP_ED_MODEL_MISMATCH):
                        (void)INTF_CIM_CMD_getMeterReading( edModel,  &reading );
                        simpleRdgInfo.type     = edModel;
                        simpleRdgInfo.pvalue   = &reading.Value.buffer[0];
                        simpleRdgInfo.rdgSize  = reading.valueSizeInBytes;
                        simpleRdgInfo.typecast = ASCIIStringValue;
                        simpleRdgInfo.byteSwap = false;
                        break;
#endif
                     case (eINIT_RESP_RCE_INITIALIZED): //Can't happen due to conditional above, but keep lint happy
                     case (eINIT_RESP_DATE_TIME_NOT_SET):
                     case (eINIT_RESP_APPLY_IN_PROCESS):
                     default: //This can occur if an inapproprate error is returned by the App
                        simpleRdgInfo.type     = dfwInternalError;
                        simpleRdgInfo.pvalue   = NULL;
                        simpleRdgInfo.rdgSize  = 0;
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = false;
                        break;
                  }
                  DFW_PRNT_INFO_PARMS("ErrorReason=%hu", (uint16_t)simpleRdgInfo.type);
                  simpleRdgInfo.pPackCfg = &packCfg;
                  pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                  ++qtys.bits.rdgQty;
               }
               //Insert the corrected number of readings
               pBuf->data[VALUES_INTERVAL_END_SIZE] = qtys.EventRdgQty;
            }  //End of if ( pBuf != NULL )
         }  //End of if(eINIT_RESP_APPLY_IN_PROCESS != response)
         else
         {
            appMsgTxInfo.Method_Status = (uint8_t)ServiceUnavailable;
         }
      }  //End of else if (OK == appMsgTxInfo.Method_Status)  //Got all the readings
      else  // BadRequest error in a reading or message, just send back header (BadRequest status below includes pBuf)
      {
         (void)HEEP_MSG_TxHeepHdrOnly (&appMsgTxInfo);
         appMsgTxInfo.Method_Status = 0;  // Set status to zero to avoid actions below
      }
   }  //end of if ( method_put == (enum_MessageMethod)appMsgRxInfo->Method_Status)
   else
   {
      appMsgTxInfo.Method_Status   = (uint8_t)NotModified;
   }
   if ( ((uint8_t)OK == appMsgTxInfo.Method_Status ) || ( ((uint8_t)BadRequest == appMsgTxInfo.Method_Status) && (NULL != pBuf) ) )
   {
      // send the message to message handler. The called function will free the buffer
      /*lint -e(644) If appMsgTxInfo.Method_Status is OK, then pBuf was initialized */
      (void)HEEP_MSG_Tx (&appMsgTxInfo, pBuf);
#if ( DCU == 1 )
      if ( VER_isComDeviceStar() )
      {
         STAR_SendDfwAlarm(STAR_DFW_INIT_STATUS, (uint8_t)response);
      }
#endif
   }
   else if ( 0 != appMsgTxInfo.Method_Status ) //All other errors, just return the header
   {
      //send HEEP Hdr only message to message handler.
      (void)HEEP_MSG_TxHeepHdrOnly (&appMsgTxInfo);
   }
}
//lint +esym(715, payloadBuf) not referenced

/***********************************************************************************************************************

   Function name: DFWI_Packet_MsgHandler

   Purpose: Interface between /df/dp request and DFW application for DownloadPacket command.

   Arguments:
      APP_MSG_Rx_t *appMsgInfo - Pointer to application header structure from head end
      void *payloadBuf         - pointer to data payload (in this case a ExchangeWithID structure)
      uint16_t length          - Payload length

   Returns: none

   Side Effects: None

   Reentrant Code: No

   Notes:
      payloadBuf will be deallocated by the calling routine.

 **********************************************************************************************************************/
//lint -esym(715, payloadBuf) not referenced
//lint -efunc(818, DFWI_Packet_MsgHandler) arguments cannot be const because of API requirements
void DFWI_Packet_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   (void)length;
   uint16_t                   bits=0;        // Keeps track of the bit position within the Bitfields
   uint16_t                   bytes=0;       // Keeps track of byte position within Bitfields
   uint8_t                    i;             // loop index
   EventRdgQty_u              qtys;          // For EventTypeQty and ReadingTypeQty
   ExWId_ReadingInfo_u        readingInfo;   // Reading Info for the reading Type and Value
   meterReadingType           readingType;   // Reading Type
   uint16_t                   rdgSize;       // For the reading value size
   bool                       dropMsg;       // If an error is caught, drop message

   dfw_dl_packet_t            packetData;
   uint8_t                    reqdReadings;    // Bit filed for ensuring all the required reading values are received

   if ( ( method_put == (enum_MessageMethod)appMsgRxInfo->Method_Status ) &&
        ( TT_ASYNC   == (enum_TransactionType)appMsgRxInfo->TransactionType ) )
   { //Parse the packet payload - an implied ExchangeWithID bit structure
      qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
      bytes           += EVENT_AND_READING_QTY_SIZE;
      //for each EDEvent, if any (Currently not supporting any events)
      for (i = 0; i < qtys.bits.eventQty; i++)
      {
         qtys.bits.rdgQty = 0; //Set to zero to skip parsing the readings
      }
      (void)memset(&packetData, 0, sizeof(packetData));
      reqdReadings = DFWI_DL_PKT_REQ_NONE;
      dropMsg      = true;   //Drop message if it is mal-formed
      //For each Reading
      for (i=0; i < qtys.bits.rdgQty; i++)
      {
         (void)memcpy(&readingInfo.bytes[0], &((uint8_t *)payloadBuf)[bytes], sizeof(readingInfo.bytes));
         bits       += READING_INFO_SIZE_BITS;
         bytes      += READING_INFO_SIZE;
         readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);
         bytes      += READING_TYPE_SIZE;
         if (readingInfo.bits.tsPresent)
         {
            bits  += READING_TIME_STAMP_SIZE_BITS;   //Skip over since we do not use/need it
            bytes += READING_TIME_STAMP_SIZE;
         }
         if (readingInfo.bits.RdgQualPres)
         {
            bits  += READING_QUALITY_SIZE_BITS; //Skip over since we do not use/need it
            bytes += READING_QUALITY_SIZE;
         }
         dropMsg = true;   //Drop message if it is mal-formed
         /*lint -e{732} bit struct to uint16 */
         rdgSize = readingInfo.bits.RdgValSizeU;
         rdgSize = ((rdgSize << RDG_VAL_SIZE_U_SHIFT) & RDG_VAL_SIZE_U_MASK) | readingInfo.bits.RdgValSize;
         switch (readingType) /*lint !e788 not all enums used within switch */
         {
            //For this packet it must have reading: streamID (5 + 1min), packet ID (5 + 1min) and packet (5 + xxx)
            case dfwPacketID:
               if ( (0 != rdgSize) && (sizeof(packetData.packetId) >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  packetData.packetId = (dl_packetid_t)UNPACK_bits2_uint16(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes              += rdgSize;
                  dropMsg             = false;
                  reqdReadings       |= DFWI_DL_PKT_REQ_PKT_ID;
               }
               break;
            case dfwPacket:
               if ( (0 != rdgSize) && (DFWI_MAX_DL_PKT_SIZE >= rdgSize) &&
                    (hexBinary == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  packetData.pData    = &((uint8_t *)payloadBuf)[bytes];
                  packetData.numBytes = rdgSize;
                  bits               += (uint16_t)(rdgSize * 8);
                  bytes              += rdgSize;
                  dropMsg             = false;
                  reqdReadings       |= DFWI_DL_PKT_REQ_PKT;
               }
               break;
            case dfwStreamID:
               if ( (0 != rdgSize) && (sizeof(packetData.streamId) >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  packetData.streamId = UNPACK_bits2_uint8(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes              += rdgSize;
                  dropMsg             = false;
                  reqdReadings       |= DFWI_DL_PKT_REQ_STREAMID;
               }
               break;
            default:
               //Invalid reading type, drop the entire message
               break;
         }  //lint !e788  Some enums in meterReadingType aren't used. //End of switch on reading type
         if (dropMsg)
         {
            break;   //Abort loop to drop the message
         }
      }  //End of for each reading type
      if (!dropMsg && (DFWI_DL_PKT_REQ_ALL == reqdReadings) )
      {
         DFWI_DownloadPacket(&packetData);
      }
   }
}
//lint +esym(715, payloadBuf) not referenced

/***********************************************************************************************************************

   Function name: DFWI_Verify_MsgHandler

   Purpose: Interface between /df/vr request and DFW application for DownloadVerify command.

   Arguments:
      APP_MSG_Rx_t *appMsgInfo - Pointer to application header structure from head end
      void *payloadBuf         - pointer to data payload (in this case a ExchangeWithID structure)
      uint16_t length          - Payload length

   Returns: none

   Side Effects: None

   Reentrant Code: No

   Notes:
      payloadBuf will be deallocated by the calling routine.

 **********************************************************************************************************************/
//lint -efunc(818, DFWI_Verify_MsgHandler) arguments cannot be const because of API requirements
//lint -efunc(715, DFWI_Verify_MsgHandler) argument not referenced
void DFWI_Verify_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   (void)length;
   HEEP_APPHDR_t              appMsgTxInfo;  // Application header/QOS info
   EventRdgQty_u              qtys;          // For EventTypeQty and ReadingTypeQty
   buffer_t                  *pBuf;          // Used for response buffer
   uint32_t                   scheduleTime;

   if ( method_get == (enum_MessageMethod)appMsgRxInfo->Method_Status)
   { //Parse the packet payload - an implied ExchangeWithID bit structure
      //There are not suppossed to be ANY Events or Readings so ignore anything that follows
      sysTimeCombined_t currentTime;   // Get current time
      uint32_t          utcTime;       // UTC time in seconds since epoch
      DFW_vars_t        dfwVars;       // Contains all of the DFW file variables
      dfw_verify_t      verify;
      dfw_verifyResp_t  respData;
      buffer_t         *pIdBuffer;
      pack_t            packCfg;       // Struct needed for PACK functions
      simpleReading_t   simpleRdgInfo;
      eDfwHeEpInitResp_t status;
#if ( EP == 1 )
      uint16_t          discardedPacketQty = 0;
#endif
      uint8_t           tempBuf[COM_DEVICE_TYPE_LEN+1]; //Used to hold variable reading types in response
      firmwareVersion_u ver;           // For the current firmware version
      DFWI_eVerifyResponse_t response;

      //Application header/QOS info (assume invalid for Method_Status)
      HEEP_initHeader( &appMsgTxInfo );
      appMsgTxInfo.TransactionType = (uint8_t)TT_RESPONSE;
      appMsgTxInfo.Resource        = (uint8_t)appMsgRxInfo->Resource;
      appMsgTxInfo.Req_Resp_ID     = (uint8_t)appMsgRxInfo->Req_Resp_ID;
      appMsgTxInfo.Method_Status   = (uint8_t)0;
      appMsgTxInfo.qos             = (uint8_t)appMsgRxInfo->qos;
      appMsgTxInfo.callback        = NULL;
      appMsgTxInfo.TransactionContext = appMsgRxInfo->TransactionContext;

      // If do not get the buffer there is no need to processing the commmand
      /*lint -e{834} Operators are not confusing in this context */
      pBuf = BM_alloc( DFWI_MISS_PKT_ID_ALLOC_SIZE );
      if (NULL != pBuf)
      {
         appMsgTxInfo.Method_Status       = (uint8_t)OK;
         verify.maxMissingPcktIdsReported = 0;
         verify.pResp                     = &respData;
         respData.pMissingPacketIds       = NULL;

         response = DFWI_DownloadVerify(&verify);

         PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );
         (void)DFWA_getFileVars(&dfwVars);   // Read the DFW NV variables (status, etc...)

         //Build FullMeterRead response
         //Insert current timestamp for Values Interval End
         (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
         utcTime = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
         PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&utcTime, &packCfg );
         pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

         qtys.EventRdgQty = 0;
         //Include the Event & Reading Quantity, it will be adjusted later to the correct value
         PACK_Buffer( sizeof(qtys) * 8, (uint8_t *)&qtys.EventRdgQty, &packCfg );
         pBuf->x.dataLen += sizeof(qtys);

         /* initialize the quality code value for all readings packed.  No need to update quality code
            value for each reading since data source is from dfwVARS.  Quality code will only need to
            updated if getting values from an endDevice. */
         simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;

         switch (response) /*lint !e788 not all enums used within switch */
         {
            case eVERIFY_RESP_NOT_INITIALIZED:
               //Include Status
               status                 = eDFW_STATUS_READING_NOT_INITIALIZED;
               simpleRdgInfo.type     = dfwStatus;
               simpleRdgInfo.pvalue   = (uint8_t *)&status;
               simpleRdgInfo.rdgSize  = sizeof(status);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               //Include FW Version
               (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
               ver                    = VER_getFirmwareVersion(eFWT_APP);
               (void)sprintf((char *)&tempBuf[0],"%hhu.%hhu.%hu", ver.field.version, ver.field.revision, ver.field.build);
               simpleRdgInfo.type     = comDeviceFirmwareVersion;
               simpleRdgInfo.pvalue   = &tempBuf[0];
               simpleRdgInfo.rdgSize  = (uint16_t)strlen((char *)&tempBuf[0]);
               simpleRdgInfo.typecast = ASCIIStringValue;
               simpleRdgInfo.byteSwap = false;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;
            #if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
            #else
               //Include BL Version
               (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
               ver                    = VER_getFirmwareVersion(eFWT_BL);
               (void)sprintf((char *)&tempBuf[0],"%hhu.%hhu.%hu", ver.field.version, ver.field.revision, ver.field.build);
               simpleRdgInfo.type     = comDeviceBootloaderVersion;
               simpleRdgInfo.pvalue   = &tempBuf[0];
               simpleRdgInfo.rdgSize  = (uint16_t)strlen((char *)&tempBuf[0]);
               simpleRdgInfo.typecast = ASCIIStringValue;
               simpleRdgInfo.byteSwap = false;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;
            #endif

               // Include Device type
               (void)getDeviceType((char *)&tempBuf[0], sizeof(tempBuf), (uint8_t *)&simpleRdgInfo.rdgSize);
               simpleRdgInfo.type     = comDeviceType;
               simpleRdgInfo.pvalue   = &tempBuf[0];
               simpleRdgInfo.typecast = ASCIIStringValue;
               simpleRdgInfo.byteSwap = false;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )

               if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
               {
                  //Include StreamID
                  simpleRdgInfo.type     = dfwStreamID;
                  simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
                  simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
                  simpleRdgInfo.typecast = uintValue;
                  simpleRdgInfo.byteSwap = true;
                  simpleRdgInfo.pPackCfg = &packCfg;
                  pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                  ++qtys.bits.rdgQty;

                  // pack the dfw file section status parameters
                  qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
               }
#endif
               break;
            case eVERIFY_RESP_INIT_WITH_ERROR:
               //Include StreamID
               simpleRdgInfo.type     = dfwStreamID;
               simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
               simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               //Include Status
               status                 = eDFW_STATUS_READING_INIT_WITH_ERROR;
               simpleRdgInfo.type     = dfwStatus;
               simpleRdgInfo.pvalue   = (uint8_t *)&status;
               simpleRdgInfo.rdgSize  = sizeof(status);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               //Include Error
               dfwErrorcodeLUT_t dfwErrorInfo;
               dfwErrorInfo = dfwErrorLookUp( &dfwVars );
               if( invalidReadingType != dfwErrorInfo.dfwErrorCode )
               {
                  simpleRdgInfo.type     = dfwErrorInfo.dfwErrorCode;
                  simpleRdgInfo.typecast = dfwErrorInfo.dfwErrorTypecast;
                  simpleRdgInfo.pvalue   = NULL;
                  simpleRdgInfo.rdgSize  = 0;
                  simpleRdgInfo.byteSwap = false;
                  simpleRdgInfo.pPackCfg = &packCfg;
                  pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                  ++qtys.bits.rdgQty;
               }

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
               if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
               {  //dfw is for meter config update, include dfw file section status parameters
                  qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
               }
#endif
               break;
            case eVERIFY_RESP_INCOMPLETE:
               //Include StreamID
               simpleRdgInfo.type     = dfwStreamID;
               simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
               simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               //Include Status
               status                 = eDFW_STATUS_READING_INIT_WAIT_FOR_PACKETS;
               simpleRdgInfo.type     = dfwStatus;
               simpleRdgInfo.pvalue   = (uint8_t *)&status;
               simpleRdgInfo.rdgSize  = sizeof(status);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

#if ( EP == 1 )
               //Include dfwDupDiscardPacketQty
               discardedPacketQty = DFWA_getDuplicateDiscardedPacketQty();
               simpleRdgInfo.type     = dfwDupDiscardPacketQty;
               simpleRdgInfo.pvalue   = (uint8_t *)&discardedPacketQty;
               simpleRdgInfo.rdgSize  = sizeof(discardedPacketQty);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;
#endif

               //Include Missing Packet Quantity
               simpleRdgInfo.type     = dfwMissingPacketQty;
               simpleRdgInfo.pvalue   = (uint8_t *)&respData.numMissingPackets;
               simpleRdgInfo.rdgSize  = sizeof(respData.numMissingPackets);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               // Include Missing Packet ID's
               // Limit number of reported packets to what may fit in reading size
               respData.numMissingPackets = respData.numMissingPackets > DFWI_MAX_REPORT_MISS_PKT_IDS?
                                            DFWI_MAX_REPORT_MISS_PKT_IDS: respData.numMissingPackets;
               pIdBuffer                  = BM_alloc( (uint16_t)(respData.numMissingPackets * sizeof(dl_packetid_t)) );
               if (NULL != pIdBuffer)
               {
                  // Get Missing Packet Id's, include range format
                  /*lint -e{826} PacketId's were put into pIdBuffer->data as (dl_packetid_t) */
                  (void)DFWP_getMissingPacketsRange(respData.packetCount, respData.numMissingPackets,
                                                    &respData.numMissingPackets, (dl_packetid_t *)pIdBuffer->data);  /*lint !e2445 alignment requirement */
                  /*lint -e{826} PacketId's were put into pIdBuffer->data as (dl_packetid_t) */
                  /*lint -e{732} assignment of uint8 to uint8 bit structure */
                  qtys.bits.rdgQty += packMissingPackets(respData.numMissingPackets,
                                                         (dl_packetid_t *)pIdBuffer->data, &packCfg, pBuf); /*lint !e2445 alignment requirement */
                  BM_free(pIdBuffer);
               }
               break;
            case eVERIFY_RESP_INIT_WAIT_FOR_APPLY:
               //Include StreamID
               simpleRdgInfo.type     = dfwStreamID;
               simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
               simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               //Include Status
               status                 = eDFW_STATUS_READING_INIT_WAIT_FOR_APPLY;
               simpleRdgInfo.type     = dfwStatus;
               simpleRdgInfo.pvalue   = (uint8_t *)&status;
               simpleRdgInfo.rdgSize  = sizeof(status);
               simpleRdgInfo.typecast = uintValue;
               simpleRdgInfo.byteSwap = true;
               simpleRdgInfo.pPackCfg = &packCfg;
               pBuf->x.dataLen       += packReading(&simpleRdgInfo);
               ++qtys.bits.rdgQty;

               if (0 < dfwVars.applySchedule)   //Is there an Apply Schedule?
               {
                  //Include the apply schedule
                  scheduleTime           = (uint32_t)(dfwVars.applySchedule / TIME_TICKS_PER_SEC);
                  simpleRdgInfo.type     = dfwApplySchedule;
                  simpleRdgInfo.pvalue   = (uint8_t *)&scheduleTime;
                  simpleRdgInfo.rdgSize  = sizeof(scheduleTime);
                  simpleRdgInfo.typecast = dateTimeValue;
                  simpleRdgInfo.byteSwap = true;
                  simpleRdgInfo.pPackCfg = &packCfg;
                  pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                  ++qtys.bits.rdgQty;
               }
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
               if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
               {  //dfw is for meter config update, include dfw file section status parameters
                  qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
               }
#endif
               break;
            case eVERIFY_RESP_APPLY_IN_PROCESS:
            default: //Apply in progress or internal error
               appMsgTxInfo.Method_Status = (uint8_t)ServiceUnavailable;
               break;
         }  //lint !e788  Some enums in meterReadingType aren't used. //end of  switch (verify.pResp->eStatus)

         //Insert the corrected number of readings
         pBuf->data[VALUES_INTERVAL_END_SIZE] = qtys.EventRdgQty;
      }  //end of if (NULL != pBuf)
   }  //end of if ( method_put == ...
   else  //Wrong method, so now the status can be set
   {
      appMsgTxInfo.Method_Status   = (uint8_t)0;
   }
   if ((uint8_t)OK == appMsgTxInfo.Method_Status)
   {
      // send the message to message handler. The called function will free the buffer
      /*lint -e(644) If appMsgTxInfo.Method_Status is OK, then pBuf was initialized */
      (void)HEEP_MSG_Tx (&appMsgTxInfo, pBuf);
   }
   else if ( 0 != appMsgTxInfo.Method_Status ) //There was an error so just return the header
   {
      //send HEEP Hdr only message to message handler.
      //TODO No response for now, add back when HE ready(void)HEEP_MSG_TxHeepHdrOnly (&appMsgTxInfo);
      BM_free(pBuf); /*lint !e644 if method_status != 0, pBuf is initialized. */ //TODO: Not going to send message so free the buffer for now
   }
}
//lint +esym(715, payloadBuf) not referenced

/***********************************************************************************************************************

   Function name: DFWI_Apply_MsgHandler

   Purpose: Interface between /df/ap request and DFW application for DownloadApply command.

   Arguments:
      APP_MSG_Rx_t *appMsgInfo - Pointer to application header structure from head end
      void *payloadBuf         - pointer to data payload (in this case a ExchangeWithID structure)
      uint16_t length          - payload length

   Returns: none

   Side Effects: None

   Reentrant Code: No

   Notes:
      payloadBuf will be deallocated by the calling routine.

 **********************************************************************************************************************/
//lint -esym(715, payloadBuf) not referenced
//lint -efunc(818, DFWI_Apply_MsgHandler) arguments cannot be const because of API requirements
void DFWI_Apply_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   (void)length;
   HEEP_APPHDR_t              appMsgTxInfo;  // Application header/QOS info
   uint16_t                   bits=0;        // Keeps track of the bit position within the Bitfields
   uint16_t                   bytes=0;       // Keeps track of byte position within Bitfields
   uint8_t                    i;             // loop index
   EventRdgQty_u              qtys;          // For EventTypeQty and ReadingTypeQty
   ExWId_ReadingInfo_u        readingInfo;   // Reading Info for the reading Type and Value
   meterReadingType           readingType;   // Reading Type
   uint16_t                   rdgSize;       // For the reading value size
   buffer_t                  *pBuf;          // pointer to message

   dfw_apply_t                applyData;
   uint32_t                   scheduleTime;
   uint8_t                    reqdReadings;    // Bit filed for ensuring all the required reading values are received
   DFWI_eApplyResponse_t      response = eAPPLY_RESP_APPLY_ACCEPTED; // Need to be initialized to shut up lint

   //Application header/QOS info (cannot set Method_Status yet)
   HEEP_initHeader( &appMsgTxInfo );
   appMsgTxInfo.TransactionType = (uint8_t)TT_RESPONSE;
   appMsgTxInfo.Resource        = (uint8_t)appMsgRxInfo->Resource;
   appMsgTxInfo.Req_Resp_ID     = (uint8_t)appMsgRxInfo->Req_Resp_ID;
   appMsgTxInfo.qos             = (uint8_t)appMsgRxInfo->qos;
   appMsgTxInfo.callback        = NULL;
   appMsgTxInfo.TransactionContext = appMsgRxInfo->TransactionContext;

   if ( method_put == (enum_MessageMethod)appMsgRxInfo->Method_Status)
   { //Parse the packet payload - an implied ExchangeWithID bit structure
      qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
      bytes           += EVENT_AND_READING_QTY_SIZE;
      //Do not allow events
      if (0 != qtys.bits.eventQty)
      {
         appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
         qtys.bits.rdgQty           = 0; //Set to zero to skip parsing the readings
      }
      (void)memset(&applyData, 0, sizeof(applyData));
      reqdReadings = DFWI_APPLY_REQ_NONE;
      //For each Reading
      for (i=0; i < qtys.bits.rdgQty; i++)
      {
         (void)memcpy(&readingInfo.bytes[0], &((uint8_t *)payloadBuf)[bytes], sizeof(readingInfo.bytes));
         bits       += READING_INFO_SIZE_BITS;
         bytes      += READING_INFO_SIZE;
         readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);
         bytes      += READING_TYPE_SIZE;
         if (readingInfo.bits.tsPresent)
         {
            bits  += READING_TIME_STAMP_SIZE_BITS;   //Skip over since we do not use/need it
            bytes += READING_TIME_STAMP_SIZE;
         }
         if (readingInfo.bits.RdgQualPres)
         {
            bits  += READING_QUALITY_SIZE_BITS; //Skip over since we do not use/need it
            bytes += READING_QUALITY_SIZE;
         }
         //Drop message if it is mal-formed, assume Requested Entity too large
         appMsgTxInfo.Method_Status = (uint8_t)RequestedEntityTooLarge;
         /*lint -e{732} bit struct to uint16 */
         rdgSize = readingInfo.bits.RdgValSizeU;
         rdgSize = ((rdgSize << RDG_VAL_SIZE_U_SHIFT) & RDG_VAL_SIZE_U_MASK) | readingInfo.bits.RdgValSize;
         switch (readingType) /*lint !e788 not all enums used within switch */
         {
            case dfwStreamID:
               if ( (0 != rdgSize) && (sizeof(applyData.streamId) >= rdgSize) &&
                    (uintValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  applyData.streamId         = UNPACK_bits2_uint8(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
                  reqdReadings              |= DFWI_APPLY_REQ_STREAMID;
               }
               break;
            case dfwApplySchedule:
               if ( (0 != rdgSize) && (sizeof(scheduleTime) >= rdgSize) &&
                    (dateTimeValue == (ReadingsValueTypecast)readingInfo.bits.typecast) )
               {
                  scheduleTime               = UNPACK_bits2_uint32(payloadBuf, &bits, (uint8_t)(rdgSize * 8));
                  applyData.applySchedule    = TIME_TICKS_PER_SEC * (uint64_t)scheduleTime;
                  bytes                     += rdgSize;
                  appMsgTxInfo.Method_Status = (uint8_t)OK;
               }
               break;
            default:
               //Invalid reading type, drop the entire message
               appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
               break;
         }  //lint !e788  Some enums in meterReadingType aren't used. //End of switch on reading type
         if ((uint8_t)OK != appMsgTxInfo.Method_Status)
         {
            break;   //Abort loop to drop the message
         }
      }  //End of for each reading type
      if (DFWI_APPLY_REQ_ALL != reqdReadings)
      {
         //Did not get all the required reading types
         appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
      }
      else if ((uint8_t)OK == appMsgTxInfo.Method_Status)   //Got all the readings, where they good?
      {
         DFW_vars_t              dfwVars;          // Contains all of the DFW file variables

         response = DFWI_DownloadApply(&applyData);

         appMsgTxInfo.Method_Status = (uint8_t)ServiceUnavailable;
         // If not going to drop message
         if ( (eAPPLY_RESP_APPLY_IN_PROCESS != response) && (eAPPLY_RESP_STREAM_ID_MISMATCH != response) )
         {
            //Build FullMeterRead response
            sysTimeCombined_t    currentTime;   // Get current time
            buffer_t            *pIdBuffer;   // pointer to missing packet ID's
            pack_t               packCfg;       // Struct needed for PACK functions
            eDfwHeEpInitResp_t   status;
            firmwareVersion_u    ver;           // For the current firmware version
            uint32_t             utcTime;       // UTC time in seconds since epoch
            uint8_t              tempBuf[COM_DEVICE_TYPE_LEN+1]; //Used to hold variable reading types in response
            uint16_t             missingPckts;
            simpleReading_t      simpleRdgInfo;
#if ( EP == 1 )
            uint16_t discardedPacketQty = 0;    // store discarded packet quantity to be packed into message
#endif

            (void)DFWA_getFileVars(&dfwVars);            // Get the DFW NV variables (status, etc...)

            appMsgTxInfo.Method_Status = (uint8_t)0;
            //Allocate a buffer big enough to accomadate worst case payload.
            /* Need maximum payload for the possible missing packet readings */
            /*lint -e{834} Operators are not confusing in this context */
            pBuf = BM_alloc( DFWI_MISS_PKT_ID_ALLOC_SIZE );
            if ( NULL != pBuf )
            {
               PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

               appMsgTxInfo.Method_Status = (uint8_t)OK; //Good to go, a response will happen
               //Insert current timestamp for Values Interval End
               (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
               utcTime = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
               PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&utcTime, &packCfg );
               pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

               qtys.EventRdgQty = 0;
               //Include the Event & Reading Quantity, it will be adjusted later to the correct value
               PACK_Buffer( sizeof(qtys) * 8, (uint8_t *)&qtys.EventRdgQty, &packCfg );
               pBuf->x.dataLen += sizeof(qtys);

               /* initialize the quality code value for all readings packed.  No need to update quality code
                  value for each reading since data source is from dfwVARS.  Quality code will only need to
                  updated if getting values from an endDevice. */
               simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;

               switch (response)
               {
                  case eAPPLY_RESP_EXPIRED:           //When expired responce is the same as Not Initialized
                  case eAPPLY_RESP_NOT_INITIALIZED:   //No active initialization
                     //Include Status
                     status                 = eDFW_STATUS_READING_NOT_INITIALIZED;
                     simpleRdgInfo.type     = dfwStatus;
                     simpleRdgInfo.pvalue   = (uint8_t *)&status;
                     simpleRdgInfo.rdgSize  = sizeof(status);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include FW Version
                     (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
                     ver                    = VER_getFirmwareVersion(eFWT_APP);
                     (void)sprintf((char *)&tempBuf[0],"%hhu.%hhu.%hu", ver.field.version, ver.field.revision, ver.field.build);
                     simpleRdgInfo.type     = comDeviceFirmwareVersion;
                     simpleRdgInfo.pvalue   = &tempBuf[0];
                     simpleRdgInfo.rdgSize  = (uint16_t)strlen((char *)&tempBuf[0]); //HEEP Spec's as Var size
                     simpleRdgInfo.typecast = ASCIIStringValue;
                     simpleRdgInfo.byteSwap = false;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;
                  #if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
                  #else
                     //Include BL Version
                     (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
                     ver                    = VER_getFirmwareVersion(eFWT_BL);
                     (void)sprintf((char *)&tempBuf[0],"%hhu.%hhu.%hu", ver.field.version, ver.field.revision, ver.field.build);
                     simpleRdgInfo.type     = comDeviceBootloaderVersion;
                     simpleRdgInfo.pvalue   = &tempBuf[0];
                     simpleRdgInfo.rdgSize  = (uint16_t)strlen((char *)&tempBuf[0]); //HEEP Spec's as Var size
                     simpleRdgInfo.typecast = ASCIIStringValue;
                     simpleRdgInfo.byteSwap = false;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;
                  #endif

                     //Include Device type
                     (void)getDeviceType((char *)&tempBuf[0], sizeof(tempBuf), (uint8_t *)&simpleRdgInfo.rdgSize);
                     simpleRdgInfo.type     = comDeviceType;
                     simpleRdgInfo.pvalue   = &tempBuf[0];
                     simpleRdgInfo.typecast = ASCIIStringValue;
                     simpleRdgInfo.byteSwap = false;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include Device type
                     simpleRdgInfo.type     = dfwStreamID;
                     simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
                     simpleRdgInfo.byteSwap = false;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     break;
                  case eAPPLY_RESP_APPLY_IN_PROCESS:     // Currently executing a patch
                  case eAPPLY_RESP_STREAM_ID_MISMATCH:   // Stream ID does not match the init'd value
                     // These cases are handled above
                     break;
                  case eAPPLY_RESP_INCOMPLETE:        // Status is not INITIALIZED_COMPLETE
                     //Include StreamID
                     simpleRdgInfo.type     = dfwStreamID;
                     simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
                     simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include Status
                     status                 = eDFW_STATUS_READING_INIT_WAIT_FOR_PACKETS;
                     simpleRdgInfo.type     = dfwStatus;
                     simpleRdgInfo.pvalue   = (uint8_t *)&status;
                     simpleRdgInfo.rdgSize  = sizeof(status);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

#if ( EP == 1 )
                     //Include dfwDupDiscardPacketQty
                     discardedPacketQty     = DFWA_getDuplicateDiscardedPacketQty();
                     simpleRdgInfo.type     = dfwDupDiscardPacketQty;
                     simpleRdgInfo.pvalue   = (uint8_t *)&discardedPacketQty;
                     simpleRdgInfo.rdgSize  = sizeof(discardedPacketQty);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;
#endif

                     //Include Missing packet quantity
                     // Get missing packet quantity first
                     (void) DFWP_getMissingPackets(dfwVars.initVars.packetCount,
                                                   REPORT_NO_PACKETS, &missingPckts, NULL );
                     //Include missing packet quantity
                     simpleRdgInfo.type     = dfwMissingPacketQty;
                     simpleRdgInfo.pvalue   = (uint8_t *)&missingPckts;
                     simpleRdgInfo.rdgSize  = sizeof(missingPckts);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include Missing Packet ID's
                     // Limit number of reported packets to what may fit in reading size
                     missingPckts = missingPckts > DFWI_MAX_REPORT_MISS_PKT_IDS?
                                    DFWI_MAX_REPORT_MISS_PKT_IDS: missingPckts;
                     pIdBuffer = BM_alloc( (uint16_t)(missingPckts * sizeof(uint16_t)) );
                     if (NULL != pIdBuffer)
                     {
                        /*lint -e{826} PacketId's were put into pIdBuffer->data as (dl_packetid_t) */
                        (void) DFWP_getMissingPacketsRange(dfwVars.initVars.packetCount,
                                                           missingPckts, &missingPckts,
                                                           (dl_packetid_t *)&pIdBuffer->data[0]);  /*lint !e2445 alignment requirement */

                        // Missing packet quantity and Missing Packet ID's
                        /*lint -e{826} PacketId's were put into pIdBuffer->data as (dl_packetid_t) */
                        /*lint -e{732} assignment of uint8 to uint8 bit structure */
                        qtys.bits.rdgQty += packMissingPackets(missingPckts, (dl_packetid_t *)&pIdBuffer->data[0],   /*lint !e2445 alignment requirement */
                                                               &packCfg, pBuf);
                        BM_free(pIdBuffer);
                     }
                     break;
                  case eAPPLY_RESP_ERROR_STATUS: // dfw Status indicates an error
                     //Include StreamID
                     simpleRdgInfo.type     = dfwStreamID;
                     simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
                     simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include Status
                     status                 = eDFW_STATUS_READING_INIT_WITH_ERROR;
                     simpleRdgInfo.type     = dfwStatus;
                     simpleRdgInfo.pvalue   = (uint8_t *)&status;
                     simpleRdgInfo.rdgSize  = sizeof(status);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include the Error
                     dfwErrorcodeLUT_t dfwErrorInfo;
                     dfwErrorInfo = dfwErrorLookUp( &dfwVars );
                     if( invalidReadingType != dfwErrorInfo.dfwErrorCode )
                     {
                        simpleRdgInfo.type     = dfwErrorInfo.dfwErrorCode;
                        simpleRdgInfo.typecast = dfwErrorInfo.dfwErrorTypecast;
                        simpleRdgInfo.pvalue   = NULL;
                        simpleRdgInfo.rdgSize  = 0;
                        simpleRdgInfo.byteSwap = false;
                        simpleRdgInfo.pPackCfg = &packCfg;
                        pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                        ++qtys.bits.rdgQty;
                     }

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                     if( IS_END_DEVICE_TARGET( dfwVars.fileType )  )
                     {
                        qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
                     }
#endif
                     break;
                  case eAPPLY_RESP_INTERNAL_ERROR:    // Invalid time or could not allocate a RAM buffer
                     //Include StreamID
                     simpleRdgInfo.type     = dfwStreamID;
                     simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
                     simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include Status
                     status                 = eDFW_STATUS_READING_INIT_WAIT_FOR_APPLY; //Only Possible status
                     simpleRdgInfo.type     = dfwStatus;
                     simpleRdgInfo.pvalue   = (uint8_t *)&status;
                     simpleRdgInfo.rdgSize  = sizeof(status);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include dfwApplySchedule
                     scheduleTime           = (uint32_t)(dfwVars.applySchedule / TIME_TICKS_PER_SEC);
                     simpleRdgInfo.type     = dfwApplySchedule;
                     simpleRdgInfo.pvalue   = (uint8_t *)&scheduleTime;
                     simpleRdgInfo.rdgSize  = sizeof(scheduleTime);
                     simpleRdgInfo.typecast = dateTimeValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include internal error as the error cause
                     simpleRdgInfo.type     = dfwInternalError;
                     simpleRdgInfo.pvalue   = NULL;
                     simpleRdgInfo.rdgSize  = 0;
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = false;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                     if( IS_END_DEVICE_TARGET( dfwVars.fileType ) )
                     {  //dfw is for meter config update, include dfw file section status parameters
                        qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
                     }
#endif
                     break;
                  case eAPPLY_SCH_AFTER_EXPIRE:       // Schedule Time is after init'd expiration
                  case eAPPLY_RESP_APPLY_ACCEPTED:    // Message accepted
                     //Include StreamID
                     simpleRdgInfo.type     = dfwStreamID;
                     simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
                     simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include Status
                     status                 = eDFW_STATUS_READING_INIT_WAIT_FOR_APPLY;
                     simpleRdgInfo.type     = dfwStatus;
                     simpleRdgInfo.pvalue   = (uint8_t *)&status;
                     simpleRdgInfo.rdgSize  = sizeof(status);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include dfwApplySchedule
                     scheduleTime           = (uint32_t)(applyData.applySchedule / TIME_TICKS_PER_SEC);
                     simpleRdgInfo.type     = dfwApplySchedule;
                     simpleRdgInfo.pvalue   = (uint8_t *)&scheduleTime;
                     simpleRdgInfo.rdgSize  = sizeof(scheduleTime);
                     simpleRdgInfo.typecast = dateTimeValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     if (eAPPLY_SCH_AFTER_EXPIRE == response)
                     {
                        //Include Invalid schedule error cause
                        simpleRdgInfo.type     = dfwInvalidSchedule;
                        simpleRdgInfo.pvalue   = NULL;
                        simpleRdgInfo.rdgSize  = 0;
                        simpleRdgInfo.typecast = uintValue;
                        simpleRdgInfo.byteSwap = false;
                        simpleRdgInfo.pPackCfg = &packCfg;
                        pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                        ++qtys.bits.rdgQty;
                     }

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                     if( IS_END_DEVICE_TARGET(dfwVars.fileType) )
                     {  //dfw is for meter config update, include dfw file section status parameters
                        qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
                     }
#endif

                     break;
                  default:
                     //Include StreamID
                     simpleRdgInfo.type     = dfwStreamID;
                     simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
                     simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include Status
                     // The real status is unknown.  Only reasonable response here is to say it is inite'd with an error
                     status                 = eDFW_STATUS_READING_INIT_WITH_ERROR;
                     simpleRdgInfo.type     = dfwStatus;
                     simpleRdgInfo.pvalue   = (uint8_t *)&status;
                     simpleRdgInfo.rdgSize  = sizeof(status);
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = true;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

                     //Include internal error as the error cause
                     simpleRdgInfo.type     = dfwInternalError;
                     simpleRdgInfo.pvalue   = NULL;
                     simpleRdgInfo.rdgSize  = 0;
                     simpleRdgInfo.typecast = uintValue;
                     simpleRdgInfo.byteSwap = false;
                     simpleRdgInfo.pPackCfg = &packCfg;
                     pBuf->x.dataLen       += packReading(&simpleRdgInfo);
                     ++qtys.bits.rdgQty;

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
                     if( IS_END_DEVICE_TARGET(dfwVars.fileType) )
                     {  //dfw is for meter config update, include dfw file section status parameters
                        qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
                     }
#endif
                     break;
               }  //end of switch (response)
               //Insert the corrected number of readings
               pBuf->data[VALUES_INTERVAL_END_SIZE] = qtys.EventRdgQty;
            }  //end of if ( pBuf != NULL )
         }  //end of if ( (eAPPLY_RESP_APPLY_IN_PROCESS != response) && ...
         else if (eAPPLY_RESP_STREAM_ID_MISMATCH == response)  //All other responses send ServiceUnavailable
         {
            appMsgTxInfo.Method_Status = (uint8_t)0;
         }
      }  //end of else if (OK == appMsgTxInfo.Method_Status)
   }  // end of if ( method_put == (enum_MessageMethod)appMsgRxInfo->Method_Status)
   else
   {
      appMsgTxInfo.Method_Status = (uint8_t)NotModified;
   }
   if ((uint8_t)OK == appMsgTxInfo.Method_Status)
   {
      // send the message to message handler. The called function will free the buffer
      /*lint -e(644) If appMsgTxInfo.Method_Status is OK, then pBuf was initialized */
      (void)HEEP_MSG_Tx (&appMsgTxInfo, pBuf);
#if ( DCU == 1 )
      if ( VER_isComDeviceStar() )
      {
         STAR_SendDfwAlarm(STAR_DFW_APPLY_STATUS, (uint8_t)response);
      }
#endif
   }
   else if ( 0 != appMsgTxInfo.Method_Status ) //There was an error so just return the header
   {
      //send HEEP Hdr only message to message handler.
      //TODO No response for now, add back when HE ready(void)HEEP_MSG_TxHeepHdrOnly (&appMsgTxInfo);
   }
}
//lint +esym(715, payloadBuf) not referenced

/***********************************************************************************************************************

   Function name: DFWI_DnlComplete

   Purpose: Interface between DFW application for DownloadPacket command and /df/dp request.

   Arguments:

   Returns: none

   Side Effects: None

   Reentrant Code: No

   Notes:
      payloadBuf will be deallocated by the calling routine.

 **********************************************************************************************************************/
//lint -esym(715, payloadBuf) not referenced
//lint -efunc(818, DFWI_STK_DlComplete) arguments cannot be const because of API requirements
buffer_t* DFWI_DnlComplete(DFW_vars_t dfwVars, HEEP_APPHDR_t* pAppMsgTxInfo)
{
   //NOTE: the DFWI_DnlComplete is a post and expects no response
   //Build FullMeterRead response
   sysTimeCombined_t       currentTime;     // Get current time
   uint32_t                msgTime;         // Time converted to seconds since epoch
   EventRdgQty_u           qtys;            // For EventTypeQty and ReadingTypeQty
   simpleReading_t         simpleRdgInfo;   // Struct for adding reading info, type and value to response
   eDfwHeEpInitResp_t      status;          // The HEEP status response
   pack_t                  packCfg;         // Struct needed for PACK functions
   buffer_t               *pBuf;            // pointer to message

   //Application header/QOS info
   pAppMsgTxInfo->TransactionType  = (uint8_t)TT_ASYNC;
   pAppMsgTxInfo->Resource         = (uint8_t)df_dp;
   pAppMsgTxInfo->Method_Status    = (uint8_t)method_post;
   pAppMsgTxInfo->Req_Resp_ID      = 0;
   pAppMsgTxInfo->qos              = 0x39;  //Priority 4, droppable, Ack not rqd, high reliability
   pAppMsgTxInfo->callback         = NULL;

   //Allocate a buffer big enough to accomadate worst case payload.
   /* FullMeterRead bit structure with One Event and Six Readings:
      optional event - createdDateTime, eventId, and nvp byte (will be appended if meter compatiblity test fails)
      one with a readingType of dfwStreamID and a readingValue of StreamID,
      one with a readingType of dfwStatus and a readingValue for the longest status message
      one (only if status is NotInitialized) with a readingType of the error and no readingValue
      one with a readingType of dfwCompatibilityTestStatus and a readingValue of dfwCompatibilityTestStatus
      one with a readingType of dfwProgramScriptStatus and a readingValue of dfwProgramScriptStatus
      one with a readingType of dfwAuditTestStatus and a readingValue of dfwAuditTestStatus
   */
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
   pBuf = BM_alloc( VALUES_INTERVAL_END_SIZE                                    +
                    EVENT_AND_READING_QTY_SIZE                                  +
                    6 * ( READING_INFO_SIZE + READING_TYPE_SIZE )               +
                    sizeof(dfwVars.fileSectionStatus.compatibilityTestStatus)   +
                    sizeof(dfwVars.fileSectionStatus.programScriptStatus)       +
                    sizeof(dfwVars.fileSectionStatus.auditTestStatus)           +
                    sizeof(dfwVars.initVars.streamId)                           +
                    sizeof(status) );
#else
   pBuf = BM_alloc( VALUES_INTERVAL_END_SIZE                                    +
                    EVENT_AND_READING_QTY_SIZE                                  +
                    3 * ( READING_INFO_SIZE + READING_TYPE_SIZE )               +
                    sizeof(dfwVars.initVars.streamId)                           +
                    sizeof(status) );
#endif

   if ( pBuf != NULL )
   {
      PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

      //Insert current timestamp for Values Interval End
      (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
      msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
      PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
      pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

      qtys.EventRdgQty = 0;
      //Insert the Event & Reading Quantities, it will be adjusted later to the correct value
      PACK_Buffer( sizeof(qtys) * 8, (uint8_t *)&qtys.EventRdgQty, &packCfg );
      pBuf->x.dataLen += sizeof(qtys);

      /* initialize the quality code value for all readings packed.  No need to update quality code
         value for each reading since data source is from dfwVARS.  Quality code will only need to
         updated if getting values from an endDevice. */
      simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;

      //Insert StreamID
      simpleRdgInfo.type     = dfwStreamID;
      simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars.initVars.streamId;
      simpleRdgInfo.rdgSize  = sizeof(dfwVars.initVars.streamId);
      simpleRdgInfo.typecast = uintValue;
      simpleRdgInfo.byteSwap = true;
      simpleRdgInfo.pPackCfg = &packCfg;
      pBuf->x.dataLen       += packReading(&simpleRdgInfo);
      ++qtys.bits.rdgQty;

      //Insert Status
      status = eDFW_STATUS_READING_INIT_WAIT_FOR_APPLY;
      if (eDFWS_INITIALIZED_WITH_ERROR == dfwVars.eDlStatus)
      {
         status = eDFW_STATUS_READING_INIT_WITH_ERROR;
      }
      simpleRdgInfo.type     = dfwStatus;
      simpleRdgInfo.pvalue   = (uint8_t *)&status;
      simpleRdgInfo.rdgSize  = sizeof(status);
      simpleRdgInfo.typecast = uintValue;
      simpleRdgInfo.byteSwap = true;
      simpleRdgInfo.pPackCfg = &packCfg;
      pBuf->x.dataLen       += packReading(&simpleRdgInfo);
      ++qtys.bits.rdgQty;

      DFW_PRNT_INFO_PARMS("Download Confirmation: StreamID=%3d, Status=%s", dfwVars.initVars.streamId,
                          (uint8_t *)DfwHEEPStatusStrings[status]);

      //Optionally Insert ErrorReason (dependent on status)
      if (eDFW_STATUS_READING_INIT_WITH_ERROR == status )
      {
         dfwErrorcodeLUT_t dfwErrorInfo;
         dfwErrorInfo = dfwErrorLookUp( &dfwVars );

         if( invalidReadingType != dfwErrorInfo.dfwErrorCode )
         {
            simpleRdgInfo.type     = dfwErrorInfo.dfwErrorCode;
            simpleRdgInfo.typecast = dfwErrorInfo.dfwErrorTypecast;
            simpleRdgInfo.pvalue   = NULL;
            simpleRdgInfo.rdgSize  = 0;
            simpleRdgInfo.byteSwap = false;
            simpleRdgInfo.pPackCfg = &packCfg;
            pBuf->x.dataLen       += packReading(&simpleRdgInfo);
            ++qtys.bits.rdgQty;
         }
      }

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
      if( IS_END_DEVICE_TARGET(dfwVars.fileType) )
      {  //dfw is for meter config update, include dfw file section status parameters
         qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( &dfwVars, pBuf, &packCfg);
      }
#endif

      //Insert the corrected number of readings
      pBuf->data[VALUES_INTERVAL_END_SIZE] = qtys.EventRdgQty;

      // send the message to message handler. The called function will free the buffer
      //(void)HEEP_MSG_Tx (&appMsgTxInfo, pBuf);
   }

   return(pBuf);
}
//lint +esym(715, payloadBuf) not referenced

/***********************************************************************************************************************

   Function name: DFWI_PatchComplete

   Purpose: Interface between DFW application for DFWComplete command and /df/co request.

   Arguments:

   Returns: none

   Side Effects: None

   Reentrant Code: No

   Notes:
      payloadBuf will be deallocated by the calling routine.

 **********************************************************************************************************************/
//lint -esym(715, payloadBuf, pDfwVars) not referenced
//lint -efunc(818, DFWI_PatchComplete) arguments cannot be const because of API requirements
buffer_t* DFWI_PatchComplete(HEEP_APPHDR_t* pAppMsgTxInfo, DFW_vars_t* pDfwVars)
{
   //Build FullMeterRead response
   sysTimeCombined_t       currentTime;     // Get current time
   uint32_t                msgTime;         // Time converted to seconds since epoch
   EventRdgQty_u           qtys;            // For EventTypeQty and ReadingTypeQty
   simpleReading_t         simpleRdgInfo;   // Struct for adding reading info, type and value to response
   eDfwHeEpInitResp_t      status;          // The HEEP status response
   firmwareVersion_u       ver;             // For the current firmware version
   pack_t                  packCfg;         // Struct needed for PACK functions
   buffer_t                *pBuf;           // pointer to message
   uint8_t                 tempBuf[COM_DEVICE_TYPE_LEN+1]; //Used to hold max variable reading types in response
#if ( EP == 1 )
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED )
   ValueInfo_t reading; // declare variable to retrieve host meter readings
#endif
#endif
   EventKeyValuePair_s     keyVal;          // Event key/value pair info to store the event information into the log buffer
   EventData_s             event;           // Event details used to store the event information into the log buffer
   union nvpQtyDetValSz_u  nvpQtyDetValSz;  // Name/ValuePairQty, EDEventDetailValueSize
   LoggedEventDetails      loggedEventData; // details about an event that was just logged to be used by caller

   //Application header/QOS info
   pAppMsgTxInfo->TransactionType  = (uint8_t)TT_ASYNC;
   pAppMsgTxInfo->Resource         = (uint8_t)df_co;
   pAppMsgTxInfo->Method_Status    = (uint8_t)method_post;
   pAppMsgTxInfo->Req_Resp_ID      = 0;
   pAppMsgTxInfo->qos              = 0x39;  //Priority 4, droppable, Ack not rqd, high reliability
   pAppMsgTxInfo->callback         = NULL;


   //Allocate a buffer big enough to accomadate worst case payload.
   /* FullMeterRead bit structure with No Events and Three Readings:
      one with a readingType of dfwStataus and a readingValue of eStatus.
      one with a readingType of comDeviceFirmwareVersionand and a readingValue for the firmware version string.
      one with a readingType of comDeviceType and a readingValue for the device type string.
      Meter configuration updates will return status, edProgrammerName, edProgramID, edProgrammedDateTime,
      dfwCompatiblityTestStatus, dfwProgramScriptStatus, and dfwAuditTestStatus.
      For meter configuration updates, the size allocated for  parameter values of the 7 reading types returned is
      smaller than the 4 reading types for an endpoint firmware update.
   */
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
   pBuf = BM_alloc( VALUES_INTERVAL_END_SIZE                                     +
                    EVENT_AND_READING_QTY_SIZE                                   +
                    ED_EVENT_CREATED_DATE_TIME_SIZE                              +
                    ED_EVENT_TYPE_SIZE                                           +
                    NVP_DET_VALUE_SIZE                                           +
                    sizeof(event.eventId)                                        +
                    sizeof(event.alarmId)                                        +
                    7 * ( READING_INFO_SIZE + READING_TYPE_SIZE )                +
                    sizeof(status)                                               +
                    sizeof(pDfwVars->fileSectionStatus.auditTestStatus)          +
                    sizeof(pDfwVars->fileSectionStatus.compatibilityTestStatus)  +
                    sizeof(pDfwVars->fileSectionStatus.programScriptStatus)      +
                    EDCFG_PROGRAM_DATETIME_LENGTH                                +
                    EDCFG_PROGRAM_ID_LENGTH                                      +
                    EDCFG_PROGRAMMER_NAME_LENGTH );
#else
   pBuf = BM_alloc( VALUES_INTERVAL_END_SIZE                                     +
                    EVENT_AND_READING_QTY_SIZE                                   +
                    ED_EVENT_CREATED_DATE_TIME_SIZE                              +
                    ED_EVENT_TYPE_SIZE                                           +
                    NVP_DET_VALUE_SIZE                                           +
                    sizeof(event.eventId)                                        +
                    sizeof(event.alarmId)                                        +
                    4 * ( READING_INFO_SIZE + READING_TYPE_SIZE )                +
                    sizeof(status)                                               +
                    DFWI_MAX_FW_VER_RDG_VALUE_SIZE                               +
                    DFWI_MAX_FW_VER_RDG_VALUE_SIZE                               +
                    COM_DEVICE_TYPE_LEN );
#endif

   if ( pBuf != NULL )
   {
      PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

      //Insert current timestamp for Values Interval End
      (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
      msgTime = (uint32_t)(currentTime / TIME_TICKS_PER_SEC);
      PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
      pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

      //Insert the Event & Reading Quantities, it will be adjusted later to the correct value
      qtys.EventRdgQty = 0;
      PACK_Buffer( sizeof(qtys) * 8, (uint8_t *)&qtys.EventRdgQty, &packCfg );
      pBuf->x.dataLen += sizeof(qtys);

      // initialize the event id to invalid, will be updated later
      event.eventId = (uint16_t)invalidEventType;
      event.eventKeyValuePairsCount = 0;
      event.eventKeyValueSize = 0;
      (void)memset( (uint8_t *)&keyVal, 0, sizeof(keyVal) );
      event.timestamp = msgTime;

      // The event returned in the df/co response, needs to marked as sent before it is logged into the event logger
      event.markSent = (bool)true;

      // initialize the logged data structure, will get updated during call to event logger
      loggedEventData.alarmIndex = invalidReadingType;
      loggedEventData.indexValue = 0;

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
      if(  IS_END_DEVICE_TARGET(pDfwVars->fileType) )
      {  // Aclara meter configuration or meter firmware patch was completed
         if( ( DFW_FILE_SECTION_PASSED ==  pDfwVars->fileSectionStatus.programScriptStatus ) &&
             ( ( DFW_FILE_SECTION_PASSED  == pDfwVars->fileSectionStatus.auditTestStatus )   ||
               ( DFW_FILE_SECTION_NOT_APP == pDfwVars->fileSectionStatus.auditTestStatus ) ) )
         {  // DFW apply completed successfully
            if( DFW_ACLARA_METER_PROGRAM_FILE_TYPE == pDfwVars->fileType )
            {  // successful meter configuration udpate
               event.eventId = (uint16_t)electricMeterConfigurationProgramChanged;
            }
            else
            {  // successful meter firmware udpate
               event.eventId = (uint16_t)electricMeterFirmwareStatusReplaced;
            }
            (void)EVL_LogEvent( 190, &event, &keyVal, TIMESTAMP_PROVIDED, &loggedEventData );
         }
         else if( DFW_FILE_SECTION_FAILED == pDfwVars->fileSectionStatus.auditTestStatus )
         {  // changes were made, but the audit test failed -- Not good
            if( DFW_ACLARA_METER_PROGRAM_FILE_TYPE == pDfwVars->fileType )
            {  // failed meter configuration udpate
               event.eventId = (uint16_t)electricMeterConfigurationError;
               (void)EVL_LogEvent( 179, &event, &keyVal, TIMESTAMP_PROVIDED, &loggedEventData );
            }
            else
            {  // failed meter firmware udpate
               event.eventId = (uint16_t)electricMeterFirmwareWriteFailed;
               (void)EVL_LogEvent( 190, &event, &keyVal, TIMESTAMP_PROVIDED, &loggedEventData );
            }
         }
         else if( DFW_FILE_SECTION_FAILED  == pDfwVars->fileSectionStatus.programScriptStatus ||
		          DFW_FILE_SECTION_NOT_APP == pDfwVars->fileSectionStatus.programScriptStatus )
         {  // changes were attempted
            if( DFW_ACLARA_METER_PROGRAM_FILE_TYPE == pDfwVars->fileType )
            {  /* A success of the program script has not been observed.  If the program scipt is not
                  completed, then the MFG Procedure #70 has not been performed.  No changes to the meter will occur
                  until we successfully execute MFG Procedure #70. */
               event.eventId = (uint16_t)electricMeterConfigurationProgramNormal;
            }
            else
            {  /* Failed to complete the program script, meter firmware patch was not successfully applied.
                  This results in the meter running the base firmware version without the patch update. */
               event.eventId = (uint16_t)electricMeterFirmwareStatusNormal;
            }
            (void)EVL_LogEvent( 190, &event, &keyVal, TIMESTAMP_PROVIDED, &loggedEventData );
         }
      }
      else
#endif
      { // ep firmware patch
         firmwareVersion_u currVersion;
         currVersion = VER_getFirmwareVersion(pDfwVars->initVars.target);
         if ( pDfwVars->initVars.toFwVersion.packedVersion == currVersion.packedVersion )
         {  // firmware version has been updated, running at new firmware version
            event.eventId = (uint16_t)comDeviceFirmwareStatusReplaced;
            (void)EVL_LogEvent( 111, &event, &keyVal, TIMESTAMP_PROVIDED, &loggedEventData );
         }
         else
         {  // firmware upated failed, still running from old firmware version
            event.eventId = (uint16_t)comDeviceFirmwareStatusNormal;
            (void)EVL_LogEvent( 221, &event, &keyVal, TIMESTAMP_PROVIDED, &loggedEventData );
         }
      }

      if( invalidEventType != (EDEventType)event.eventId )
      { // An event was logged, we need to insert the event details into the message

         // pack the event createdDateTime value
         PACK_Buffer( -(ED_EVENT_CREATED_DATE_TIME_SIZE * 8), (uint8_t *)&msgTime, &packCfg );
         pBuf->x.dataLen += ED_EVENT_CREATED_DATE_TIME_SIZE;

         // pack the event ID
         PACK_Buffer( -(ED_EVENT_TYPE_SIZE * 8), (uint8_t *)&event.eventId, &packCfg );
         pBuf->x.dataLen += ED_EVENT_TYPE_SIZE;

         if( invalidReadingType != loggedEventData.alarmIndex )
         {
            // we need to add the alarm index id in the message for the event that was just logged, update nvp info
            nvpQtyDetValSz.bits.nvpQty = 1;
            nvpQtyDetValSz.bits.DetValSz = sizeof(loggedEventData.indexValue);

            // pack the name value pair information
            PACK_Buffer( (NVP_DET_VALUE_SIZE * 8), (uint8_t *)&nvpQtyDetValSz, &packCfg );
            pBuf->x.dataLen += NVP_DET_VALUE_SIZE;

            // pack the nvp alarm index name
            PACK_Buffer( -(int16_t)(sizeof(meterReadingType) * 8), (uint8_t *)&loggedEventData.alarmIndex, &packCfg );
            pBuf->x.dataLen += sizeof(meterReadingType);

            // pack the index value of the event
            PACK_Buffer( (sizeof(loggedEventData.indexValue)) * 8, (uint8_t *)&loggedEventData.indexValue, &packCfg );
            pBuf->x.dataLen += sizeof(loggedEventData.indexValue);
         }
         else
         {
            // no nvp information will be provided with the event
            nvpQtyDetValSz.bits.nvpQty = 0;
            nvpQtyDetValSz.bits.DetValSz = 0;

            // pack the name value pair information
            PACK_Buffer( (NVP_DET_VALUE_SIZE * 8), (uint8_t *)&nvpQtyDetValSz, &packCfg );
            pBuf->x.dataLen += NVP_DET_VALUE_SIZE;
         }

         ++qtys.bits.eventQty; // update the event quantity
      }


      //Insert Status
      status                    = eDFW_STATUS_READING_NOT_INITIALIZED;
      simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;
      simpleRdgInfo.type        = dfwStatus;
      simpleRdgInfo.pvalue      = (uint8_t *)&status;
      simpleRdgInfo.rdgSize     = sizeof(status);
      simpleRdgInfo.typecast    = uintValue;
      simpleRdgInfo.byteSwap    = true;
      simpleRdgInfo.pPackCfg    = &packCfg;
      pBuf->x.dataLen           += packReading(&simpleRdgInfo);
      ++qtys.bits.rdgQty;

#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
      if( DFW_ACLARA_METER_PROGRAM_FILE_TYPE == pDfwVars->fileType )
      {
         // pack the dwf file section status parameters into message
         qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( pDfwVars, pBuf, &packCfg);

         //Insert programmed date and time
         simpleRdgInfo.qualityCode = INTF_CIM_CMD_getMeterReading( edProgrammedDateTime, &reading );
         simpleRdgInfo.type        = reading.readingType;
         simpleRdgInfo.pvalue      = (uint8_t *)&reading.Value.uintValue;
         simpleRdgInfo.rdgSize     = reading.valueSizeInBytes;
         simpleRdgInfo.typecast    = reading.typecast;
         simpleRdgInfo.byteSwap    = true;
         simpleRdgInfo.pPackCfg    = &packCfg;
         pBuf->x.dataLen           += packReading(&simpleRdgInfo);
         ++qtys.bits.rdgQty;

         if( eDFWE_NO_ERRORS == pDfwVars->eErrorCode )
         {
            //Insert the programmer name
            (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
            simpleRdgInfo.qualityCode = INTF_CIM_CMD_getMeterReading( edProgrammerName, &reading);
            simpleRdgInfo.type        = reading.readingType;
            simpleRdgInfo.pvalue      = reading.Value.buffer;
            simpleRdgInfo.rdgSize     = reading.valueSizeInBytes;
            simpleRdgInfo.typecast    = reading.typecast;
            simpleRdgInfo.byteSwap    = false;
            simpleRdgInfo.pPackCfg    = &packCfg;
            pBuf->x.dataLen           += packReading(&simpleRdgInfo);
            ++qtys.bits.rdgQty;
         }

         //Insert programmed ID number
         simpleRdgInfo.qualityCode = INTF_CIM_CMD_getMeterReading( edProgramID, &reading );
         simpleRdgInfo.type        = reading.readingType;
         simpleRdgInfo.pvalue      = (uint8_t *)&reading.Value.uintValue;
         simpleRdgInfo.rdgSize     = reading.valueSizeInBytes;
         simpleRdgInfo.typecast    = reading.typecast;
         simpleRdgInfo.byteSwap    = true;
         simpleRdgInfo.pPackCfg    = &packCfg;
         pBuf->x.dataLen           += packReading(&simpleRdgInfo);
         ++qtys.bits.rdgQty;
      }
      else if( IS_METER_FIRM_UPGRADE( pDfwVars->fileType ) )
      {  // Meter firwmare updated completed, append edFwVersion to the response

         // pack the dwf file section status parameters into message
         qtys.bits.rdgQty += packDfwFileSectionStatusParmeters( pDfwVars, pBuf, &packCfg);

         simpleRdgInfo.qualityCode = INTF_CIM_CMD_getMeterReading( edFwVersion, &reading );
         simpleRdgInfo.type        = reading.readingType;
         simpleRdgInfo.pvalue      = reading.Value.buffer;
         simpleRdgInfo.rdgSize     = reading.valueSizeInBytes;
         simpleRdgInfo.typecast    = reading.typecast;
         simpleRdgInfo.byteSwap    = false;
         simpleRdgInfo.pPackCfg    = &packCfg;
         pBuf->x.dataLen           += packReading(&simpleRdgInfo);
         ++qtys.bits.rdgQty;
      }
      else
#endif
      { // application/bootloader  update (Default DFW file type)

         //Insert FW Version
         (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
         ver                       = VER_getFirmwareVersion(eFWT_APP);
         (void)sprintf((char *)&tempBuf[0],"%hhu.%hhu.%hu", ver.field.version, ver.field.revision, ver.field.build);
         simpleRdgInfo.type        = comDeviceFirmwareVersion;
         simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;
         simpleRdgInfo.pvalue      = &tempBuf[0];
         simpleRdgInfo.rdgSize     = (uint16_t)strlen((char *)&tempBuf[0]); //HEEP Spec's as Var size
         simpleRdgInfo.typecast    = ASCIIStringValue;
         simpleRdgInfo.byteSwap    = false;
         simpleRdgInfo.pPackCfg    = &packCfg;
         pBuf->x.dataLen           += packReading(&simpleRdgInfo);
         ++qtys.bits.rdgQty;
      #if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) || ( DCU == 1 ) )
      #else
         //Insert BL Version
         (void)memset(&tempBuf[0], 0, sizeof(tempBuf));
         ver                       = VER_getFirmwareVersion(eFWT_BL);
         (void)sprintf((char *)&tempBuf[0],"%hhu.%hhu.%hu", ver.field.version, ver.field.revision, ver.field.build);
         simpleRdgInfo.type     = comDeviceBootloaderVersion;
         simpleRdgInfo.pvalue   = &tempBuf[0];
         simpleRdgInfo.rdgSize  = (uint16_t)strlen((char *)&tempBuf[0]); //HEEP Spec's as Var size
         simpleRdgInfo.typecast = ASCIIStringValue;
         simpleRdgInfo.byteSwap = false;
         simpleRdgInfo.pPackCfg = &packCfg;
         pBuf->x.dataLen        += packReading(&simpleRdgInfo);
         ++qtys.bits.rdgQty;
      #endif

         //Insert Device type
         (void)getDeviceType((char *)&tempBuf[0], sizeof(tempBuf), (uint8_t *)&simpleRdgInfo.rdgSize);
         simpleRdgInfo.type     = comDeviceType;
         simpleRdgInfo.pvalue   = &tempBuf[0];
         simpleRdgInfo.typecast = ASCIIStringValue;
         simpleRdgInfo.byteSwap = false;
         simpleRdgInfo.pPackCfg = &packCfg;
         pBuf->x.dataLen        += packReading(&simpleRdgInfo);
         ++qtys.bits.rdgQty;
      }

      //Insert the corrected number of readings
      pBuf->data[VALUES_INTERVAL_END_SIZE] = qtys.EventRdgQty;

      // send the message to message handler. The called function will free the buffer
      //(void)HEEP_MSG_Tx (&appMsgTxInfo, pBuf);

      DFW_PRNT_INFO("Apply Confirmation");
   }

   return(pBuf);
}
//lint +esym(715, payloadBuf) not referenced

/*! ********************************************************************************************************************

   \fn void DFWI_DownloadInit(dfw_init_t *pReq)

   \brief This is used to initialize a download stream

   \param   pReq - pointer to the dfw init variables
            reqNonStored - init readings not stored as dfw init variables

   \return  DFWI_eInitResponse_t - See definition

   Side Effects: If the download is in an idle state, it will be aborted and re-initialized.

   Reentrant Code: No

   Notes:   The paramter reqAnsiTblOID will not be utilized by endpoints that are not integrated with
            a meter.

 ******************************************************************************************************************** */
//lint -e{715} suppress "reqAnsiTblOID may not be referenced"
DFWI_eInitResponse_t DFWI_DownloadInit( DFWI_dlInit_t const *pReq, DFWI_nonStoredDfwInitReadings_t const *reqNonStored )
{
   DFWI_eInitResponse_t resp;
   DFW_vars_t           dfwVars;          // Contains all of the DFW file variables
   sysTimeCombined_t    currentTime;      // Get current time
   dl_dfwFileType_t     reqFileType;
   uint8_t              deviceType[COM_DEVICE_TYPE_LEN+1];    //Temporary buffer
   uint8_t              devTypeLen;

   (void)DFWA_getFileVars(&dfwVars);            // Read the DFW NV variables (status, etc...)
   if ( (eDFWP_IDLE == dfwVars.ePatchState) || (eDFWP_WAIT_FOR_APPLY == dfwVars.ePatchState) )  /* Is DFW idle? */
   {
      (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
      (void)getDeviceType((char *)&deviceType[0], sizeof(deviceType), &devTypeLen);
       reqFileType = reqNonStored->fileType;

      //It is not supposed to be possible to get an App Message if time is not valid
      if ( false == TIME_SYS_IsTimeValid() )            /* Check is the system time is valid */
      {  //Time is invalid
         resp = eINIT_RESP_DATE_TIME_NOT_SET;
         DFW_PRNT_INFO("Init Failed, Time");
      }
      else if ( strcmp(&pReq->comDeviceType[0], (char *)&deviceType[0]))
      {  // Invalid Device Type
         resp = eINIT_RESP_DEV_TYPE_MISMATCH;
         DFW_PRNT_INFO("Init Failed, Device Type");
      }
      else if( false == IS_VALID_DFW_FILE_TYPE( reqFileType ) )
      { // Invalid parameter value utilized for dfwFileType
         resp = eINIT_RESP_INVALID_DFW_FILE_TYPE;
         DFW_PRNT_INFO("Init Failed, DFW FileType");
      }
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
      else if( ( DFW_ACLARA_METER_PROGRAM_FILE_TYPE == reqFileType ) &&
               ( false == isValidAnsiTableOID( reqNonStored->reqEndDeviceReadings.endDeviceAnsiTblOID ) ) )

      {  // the dfw file type is for meter configuratio update and invalid ansiTableOID was requested in the init
         resp = eINIT_RESP_ANSI_TABLE_OID_MISMATCH;
         DFW_PRNT_INFO("Init Failed, ANSI Table OID");
      }
#endif
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
      else if(  IS_METER_FIRM_UPGRADE( reqFileType )  &&
               ( false == isValidEdModel( reqNonStored->reqEndDeviceReadings.endDeviceModel,
                              (uint8_t)sizeof( reqNonStored->reqEndDeviceReadings.endDeviceModel ) ) ) )
      {
         resp = eINIT_RESP_ED_MODEL_MISMATCH;
         DFW_PRNT_INFO("Init Failed, Ed Model");
      }
#endif
      else if ( false == isFirmwareVersionSupported(&pReq->firmwareVersion, pReq->target, reqFileType ) )
      {  // Invalid firmware version of meter basecode or NIC
         resp = eINIT_RESP_FW_MISMATCH; // Then don't initialize
         DFW_PRNT_INFO("Init Failed, FW Ver");
      }
      else if ( (pReq->packetSize < DFWI_MIN_DL_PKT_SIZE) || (pReq->packetSize > DFWI_MAX_DL_PKT_SIZE) )
      {  // Invalid packet size
         resp = eINIT_RESP_PACKET_SIZE_OUT_OF_BOUNDS;
         DFW_PRNT_INFO("Init Failed, Packet Size");
      }
      //Whether cipher is enabled or not the patch must be within this size
      else if ( false == isValidPatchSize(pReq->patchSize, pReq->cipher, reqFileType) )
      {
         resp = eINIT_RESP_PATCH_OUT_OF_BOUNDS;
         DFW_PRNT_INFO("Init Failed, Incorrect Patch size");
      }
      else if ( pReq->expirationTime <= currentTime )
      {  // Invalid expiration time
         resp = eINIT_RESP_EXPIRY_OUT_OF_BOUNDS;
         DFW_PRNT_INFO("Init Failed, Expiration time");
      }
      else if ( pReq->gracePeriodSec > DFWI_APPLY_GRACE_PERIOD_MAX )
      {  // Invalid Grace Period
         resp = eINIT_RESP_GRACE_PERIOD_OUT_OF_BOUNDS;
         DFW_PRNT_INFO("Init Failed, Grace Period");
      }
      else if ( ( 0 == pReq->packetCount ) || ( pReq->packetCount > DFWP_getMaxPackets() ) )
      {  // Invalid packet count
         resp = eINIT_RESP_PACKET_COUNT_OUT_OF_BOUNDS;
         DFW_PRNT_INFO("Init Failed, Packet Cnt");
      }
      else if ( eSUCCESS != DFWA_SupportedCiphers(pReq->cipher, reqNonStored->fileType) )
      {  // Invalid cipher suite
         resp = eINIT_RESP_CIPHER_OUT_OF_BOUNDS;
         DFW_PRNT_INFO("Init Failed, Cipher Suite");
      }
      else
      {  // there were no errors, Save the data!
         dfwVars.eErrorCode        = eDFWE_NO_ERRORS;    // Reset error code
         dfwVars.ePatchState       = eDFWP_IDLE;
         dfwVars.applySchedule     = 0;                  //Clear the schedule until we actually get the apply
         dfwVars.eSendConfirmation = eDFWC_NONE;
         (void)memcpy(&dfwVars.initVars, pReq, sizeof(dfwVars.initVars));  /* Save the initialize data */
         dfwVars.fileType          = reqNonStored->fileType;
         dfwVars.eDlStatus         = eDFWS_INITIALIZED_INCOMPLETE;         /* Okay to start collecting packets */
         dfwVars.clrBitArrayStatus = true;
         (void)DFWP_ClearBitMap();                                               // Clear the bitfield now
         dfwVars.patchSector       = PATCH_DECRYPTED_SECTOR;
         dfwVars.decryptState      = eDFWD_DONE;
         (void)DFWA_SetPatchParititon( dfwVars.fileType );
         (void)erasePatchSpace(dfwVars.initVars.patchSize, dfwVars.initVars.cipher);   //Clear the patch partition space

#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
         // We have a successful DFW init, initialize the dfw file section status parameters to default values
         dfwVars.fileSectionStatus.programScriptStatus = DFW_FILE_SECTION_STATUS_DEFAULT_VALUE;
         dfwVars.fileSectionStatus.compatibilityTestStatus = DFW_FILE_SECTION_STATUS_DEFAULT_VALUE;
         dfwVars.fileSectionStatus.auditTestStatus = DFW_FILE_SECTION_STATUS_DEFAULT_VALUE;
#endif

#if ( EP == 1 )
         // Initialize all dfw cached attributes
         DfwCached_Init();
#endif

         if ( eDFWS_NONE != dfwVars.initVars.cipher )
         {
            dfwVars.patchSector    = PATCH_ENCRYPTED_SECTOR;
            dfwVars.decryptState   = eDFWD_READY;
         }
         (void)DFWA_setFileVars(&dfwVars);                                 // Write DFW NV variables (status, etc...)

         //Cancel previous Apply schedule if exists
         buffer_t *pBuf;
         pBuf = BM_alloc( sizeof(eDfwCommand_t) );
         if (NULL != pBuf)  // Was the buffer allocated?
         {
            eDfwCommand_t *pMsgData;
            pMsgData      = (eDfwCommand_t *)&pBuf->data[0];         //lint !e826   Pointer to sys msg data /*lint !e2445 alignment requirement */
            pBuf->eSysFmt = eSYSFMT_DFW;                             // Set sys msg format
            *pMsgData     = eDFWC_CANCEL_APPLY;                      // Set sys msg command
            OS_MSGQ_Post( &DFWA_MQueueHandle, pBuf );                // Function will not return if it fails
            DFW_PRNT_INFO("Cancel Alarm sent to DFW Task");
         }
         else
         {
            DFW_PRNT_ERROR("BM_alloc()");
         }
#if ( EP == 1 )
         //Cancel previous Time diversity for the Download and/or Apply Confirmations if exists
         //(Not applicable to DCU)
         DFWA_CancelPending(true);
#endif
         resp = eINIT_RESP_RCE_INITIALIZED;
         DFW_PRNT_INFO("Initialized for Download");
      }
   }
   else
   {
      resp = eINIT_RESP_APPLY_IN_PROCESS;
      DFW_PRNT_INFO("Init Failed, Apply In Process");
   }
   return(resp);  /*lint !e438 last value of devTypeLen not used. */
}

/*! ********************************************************************************************************************

   \fn void DFWI_DownloadCancel(dfw_cancel_t *pReq)

   \brief This is used to cancel a download.

   \param  pReq

   \return  returnStatus_t

   \details

 ******************************************************************************************************************** */
returnStatus_t DFWI_DownloadCancel( const dfw_cancel_t *pReq )
{
   returnStatus_t  retVal = eSUCCESS;
   DFW_vars_t      dfwVars;                              // Contains all of the DFW file variables

   (void)DFWA_getFileVars(&dfwVars);                     // Read the DFW NV variables (status, etc...)
   if ( eDFWS_NOT_INITIALIZED != dfwVars.eDlStatus)      // Exit if not initialized
   {
      if ( pReq->streamId == dfwVars.initVars.streamId ) // Does the stream ID match the stream ID used at init?
      {
         buffer_t *pBuf = BM_alloc( sizeof(eDfwCommand_t) );

         if (NULL != pBuf)  // Was the buffer allocated?
         {
            eDfwCommand_t *pMsgData = (eDfwCommand_t *)&pBuf->data[0];       //lint !e826   Pointer to sys msg data  /*lint !e2445 alignment requirement */

            pBuf->eSysFmt = eSYSFMT_DFW;                                     // Set sys msg format
            *pMsgData = eDFWC_CANCEL;                                        // Set sys msg command
            OS_MSGQ_Post( &DFWA_MQueueHandle, pBuf );                        // Post it!
            DFW_PRNT_INFO("Cancel Cmd sent to DFW Task");
         }
         else
         {
            DFW_PRNT_ERROR("BM_alloc()");
         }
      }
      else
      {
         DFW_PRNT_INFO("Cancel Ignored, Stream ID");
      }
   }
   else
   {
      DFW_PRNT_INFO("Cancel Ignored, Uninitialized");
   }
   return(retVal);
}

/*! ********************************************************************************************************************

   \fn void DFWI_DownloadPacket(const dfw_dl_packet_t *pReq)

   \brief This is used to download a packet.

   \param  pReq

   \return  none

   \details

 ******************************************************************************************************************** */
void DFWI_DownloadPacket( const dfw_dl_packet_t *pReq )
{
   bool             bExit = false;                    // Flag the need to exit the function
   DFW_vars_t       dfwVars;                          // Contains all of the DFW file variables
   bool             bSaveFile = false;                // Save the file at the end of the function?

   (void)DFWA_getFileVars(&dfwVars);                  // Read the DFW NV variables (status, etc...)
   if ( (eDFWS_NOT_INITIALIZED != dfwVars.eDlStatus) && (eDFWP_IDLE == dfwVars.ePatchState)) // Exit if not initialized
   {
      // Check that download packet matches the expected size for all but the last packet OR
      // if the last packet, the size can be smaller
            // If not the last packet and the Packet size is right
      if ( ((pReq->packetId < (dfwVars.initVars.packetCount - 1) ) &&
            (dfwVars.initVars.packetSize == pReq->numBytes) )
            ||  // OR
            //If this is the last packet and is the packet size or less and will not overrun patch space
            ((pReq->packetId == (dfwVars.initVars.packetCount - 1) ) &&
            (pReq->numBytes <= dfwVars.initVars.packetSize) &&
            (((uint32_t)dfwVars.initVars.packetSize * (uint32_t)(dfwVars.initVars.packetCount - 1) + pReq->numBytes) <
               DFWA_GetPatchParititonSize()) )
         )
      {
         /* If the time is invalid, go ahead and accept the packet. This way there are less potential downloads  */
         if ( TIME_SYS_IsTimeValid() )
         {
            if ( isDownloadTimeExpired() )
            {   /* If the time is valid and the expiration date is past, then we are done! */
               dfwVars.eDlStatus  = eDFWS_NOT_INITIALIZED; // Download expired so return to this state
               dfwVars.eErrorCode = eDFWE_EXPIRED;       // Download expired - leave a bread crumb
               bExit              = true;                // Set the exit flag
               bSaveFile          = true;                // Save the file
               DFW_PRNT_INFO("DL Packet Failed, Expired");
            }
         }
         if ( !bExit )  // Continue processing if no need to exit
         {  /* If the StreamID that we are listening for doesn't match this stream ID, we are done! */
            if ( pReq->streamId == dfwVars.initVars.streamId )
            {
               //Is PacketID within PacketCount?
               if (pReq->packetId < dfwVars.initVars.packetCount)
               {
                  /* An ecrypted patch will exit the eDFWD_READY state once all packets are received and the signature
                     is validated.  From that point on Packets can no longer be accepted.
                  */
                  if ( ((eDFWS_NONE == dfwVars.initVars.cipher) && (eDFWD_DONE  == dfwVars.decryptState)) ||
                       ((eDFWS_NONE != dfwVars.initVars.cipher) && (eDFWD_READY == dfwVars.decryptState)) )
                  {
                     //TODO: Consider moving all the packet management to dfw_pckt.c
                     /* write the packet to the DFW Image.  Since the layer knows the address etc, then let it compute
                        them. Note for fountain codes, the list could be an array of packets. */
                     uint16_t missingPckts;

                     /* Save packet, handle duplicates including replacing packets if initialized with error.  If it
                        is a duplicate packet and the contents of both packets do not match, the packetDiscarded flag
                        will be set to indicate the stored packet was discarded. */
                     (void)dlPcktWriteToNv((uint16_t)pReq->packetId, pReq->pData, pReq->numBytes,
                                           dfwVars.patchSector, dfwVars.eDlStatus, dfwVars.initVars.packetSize);

                     DFW_PRNT_INFO("DL Packet Accepted");

                     /* Now check if all packets are received so that we can update the "Download Status" */
                     if ( eSUCCESS == DFWP_getMissingPackets(dfwVars.initVars.packetCount, REPORT_NO_PACKETS,
                                                             &missingPckts, NULL) )
                     {
                        if ( missingPckts > 0 )   // Check for gaps
                        {
                           if (eDFWS_INITIALIZED_INCOMPLETE != dfwVars.eDlStatus)
                           {
                              bSaveFile         = true;
                              dfwVars.eDlStatus = eDFWS_INITIALIZED_INCOMPLETE;
                           }
                           DFW_PRNT_INFO_PARMS("Rcvd Pckt #: %d", pReq->packetId);
                           DFW_PRNT_INFO_PARMS("Missing: %d", missingPckts);
                        }
                        else
                        {
                           buffer_t *pBuf = BM_alloc( sizeof(eDfwCommand_t) ); // Get buffer for msg

                           if (NULL != pBuf)  // Was the buffer allocated?
                           {
                              eDfwCommand_t  *pMsgData = (eDfwCommand_t *)&pBuf->data[0]; /*lint !e826  Ptr to msg data *//*lint !e2445 alignment requirement */

                              pBuf->eSysFmt     = eSYSFMT_DFW;                            // Set sys msg format
                              *pMsgData         = eDFWC_PREPATCH;                         // Set sys msg command
                              OS_MSGQ_Post ( &DFWA_MQueueHandle, pBuf );                  // Post it!
                              DFW_PRNT_INFO_PARMS("Status Complete, Pckt #: %d",pReq->packetId);
                           }
                           else
                           {
                              DFW_PRNT_ERROR("BM_alloc()");
                           }
                        }
                     }
                  }
                  else
                  {  // Cannot accept packets once decryption started
                     DFW_PRNT_INFO("DL Packet Failed, Decrypted Patch");
                  }
               }
               else
               {  // Packet id greater than the configured packet count
                  DFW_PRNT_INFO("DL Packet Failed, Packet ID");
               }
            }
            else
            {  // stream id does not match the configured value
               DFW_PRNT_INFO("DL Packet Failed, Stream ID");
            }
         }
      }
      else
      {   // size doesn't match the expected size
         DFW_PRNT_INFO("DL Packet Failed, Packet ID or size");
      }
   }
   else
   {  // not initialized
      DFW_PRNT_INFO("DL Packet Failed, Not Initialized or not Idle");
   }
   if (bSaveFile)
   {
      (void)DFWA_setFileVars(&dfwVars); // Save the changes
   }
}

/*! ********************************************************************************************************************

   \fn void DFWI_DownloadVerify(const dfw_verify_t *pReq)

   \brief This is used to verify the download image.

   \param  pReq - pointer to dfw_verify_t

   \return  returnStatus_t

   \details

 ******************************************************************************************************************** */
DFWI_eVerifyResponse_t DFWI_DownloadVerify( const dfw_verify_t *pReq ) //lint -esym(715, pReq)
{
   DFW_vars_t              dfwVars;       // Contains all of the DFW file variables
   DFWI_eVerifyResponse_t  retVal;        // Read the file variables

   pReq->pResp->numMissingPackets = 0;                      // Initialize this to zero

   retVal = eVERIFY_RESP_APPLY_IN_PROCESS;
   (void)DFWA_getFileVars(&dfwVars);
   if ( (eDFWP_IDLE == dfwVars.ePatchState) || (eDFWP_WAIT_FOR_APPLY == dfwVars.ePatchState) )  /* Is DFW idle? */
   {
      if (eDFWS_NOT_INITIALIZED == dfwVars.eDlStatus)
      {
         retVal = eVERIFY_RESP_NOT_INITIALIZED;
      }
      else if ( isDownloadTimeExpired() ) // For all remaining status, is patch expired?
      {
         dfwVars.eDlStatus   = eDFWS_NOT_INITIALIZED;
         dfwVars.ePatchState = eDFWP_IDLE;
         dfwVars.eErrorCode  = eDFWE_EXPIRED;   //Leave a bread crumb
         (void)DFWA_setFileVars(&dfwVars); // Save the changes
         retVal              = eVERIFY_RESP_NOT_INITIALIZED;
      }
      else if ( eDFWS_INITIALIZED_INCOMPLETE == dfwVars.eDlStatus ) // Continue processing only if we are missing packets
      {  // Read download packet count
         pReq->pResp->packetCount = dfwVars.initVars.packetCount;
         retVal = eVERIFY_RESP_INCOMPLETE;
         if ( eDFWE_NO_ERRORS == dfwVars.eErrorCode ) // Stop processing if error, Invalid time is not an error
         {  // Find missing packets
            if ( eSUCCESS != DFWP_getMissingPackets(dfwVars.initVars.packetCount,
                                                    pReq->maxMissingPcktIdsReported,
                                                    &pReq->pResp->numMissingPackets,
                                                    pReq->pResp->pMissingPacketIds) )
            {
               dfwVars.eErrorCode = eDFWE_NV;
            }
         }
         //TODO What if there are errors??
      }
      else if (eDFWS_INITIALIZED_COMPLETE == dfwVars.eDlStatus)
      {
         retVal = eVERIFY_RESP_INIT_WAIT_FOR_APPLY;
      }
      /* If the status is error (init with error) */
      else if (eDFWS_INITIALIZED_WITH_ERROR == dfwVars.eDlStatus)
      {
         retVal = eVERIFY_RESP_INIT_WITH_ERROR;
      }
      pReq->pResp->eStatus     = dfwVars.eDlStatus;
      pReq->pResp->ePatchState = dfwVars.ePatchState;
      pReq->pResp->eErrorCode  = dfwVars.eErrorCode;
   }
   return(retVal);
}
//lint +esym(715, pReq)

/*! ********************************************************************************************************************

   \fn void DFWI_DownloadApply(const dfw_apply_t *pReq)

   \brief This function is used to apply the download image.

   \param  pReq

   \return  eDfwErrorCodes_t

   \details

 ******************************************************************************************************************** */
DFWI_eApplyResponse_t DFWI_DownloadApply( const dfw_apply_t *pReq )
{
   DFW_vars_t              dfwVars;    // Contains all of the DFW file variables
   DFWI_eApplyResponse_t   retVal;     // Return accepted or any error detected
   sysTimeCombined_t       currentDateTime;  /* Contains current system combined date/time format */

   (void)DFWA_getFileVars(&dfwVars);         // Read the DFW NV variables (status, etc...)

   // Reject command if not already initialized (Ignore stream ID since it never got it)
   if (eDFWS_NOT_INITIALIZED == dfwVars.eDlStatus)
   {
      retVal = eAPPLY_RESP_NOT_INITIALIZED;
   }
   // Reject command if it is executing the patch
   else if ( (eDFWP_IDLE != dfwVars.ePatchState) && (eDFWP_WAIT_FOR_APPLY != dfwVars.ePatchState) )
   {
      retVal = eAPPLY_RESP_APPLY_IN_PROCESS;
   }
   /* If the apply is not for this unit */
   else if (pReq->streamId != dfwVars.initVars.streamId)
   {
      retVal = eAPPLY_RESP_STREAM_ID_MISMATCH;
   }
   else if (eSUCCESS != TIME_UTIL_GetTimeInSysCombined(&currentDateTime))   // Get the Current Date/Time
   {  /* Current time is invalid */
      retVal = eAPPLY_RESP_INTERNAL_ERROR;
   }
   else if (dfwVars.initVars.expirationTime < currentDateTime )/* Is sys time past the expiration time? */
   {
      dfwVars.eDlStatus   = eDFWS_NOT_INITIALIZED;
      dfwVars.ePatchState = eDFWP_IDLE;
      dfwVars.eErrorCode  = eDFWE_EXPIRED;   //Leave a bread crumb
      (void)DFWA_setFileVars(&dfwVars); // Save the changes
      retVal              = eAPPLY_RESP_EXPIRED;
   }
   /* If the status is incomplete */
   else if (eDFWS_INITIALIZED_INCOMPLETE == dfwVars.eDlStatus)
   {
      retVal = eAPPLY_RESP_INCOMPLETE;
   }
   /* If the status is error */
   else if (eDFWS_INITIALIZED_WITH_ERROR == dfwVars.eDlStatus)
   {
      retVal = eAPPLY_RESP_ERROR_STATUS;
   }
   else  //Only thing left is eDFWS_INITIALIZED_COMPLETE without error
   {
      if (pReq->applySchedule > dfwVars.initVars.expirationTime)
      {  /* Schedule invalid */
         retVal = eAPPLY_SCH_AFTER_EXPIRE;
      }
      else
      {
         buffer_t *pBuf = BM_alloc( sizeof(eDfwCommand_t) );
         if (NULL != pBuf)
         {
            dfwFullApplyCmd_t *pMsgData = (dfwFullApplyCmd_t *)&pBuf->data[0];   //lint !e740 !e826   /*lint !e2445 alignment requirement */

            //The Apply schedule is saved to dfwVars by the application task
            pBuf->eSysFmt  = eSYSFMT_DFW;                                         // Set sys msg format
            pMsgData->eCmd = eDFWC_FULL_APPLY;                                   // Set sys msg command
            (void)memcpy(&pMsgData->applyVars, pReq, sizeof(pMsgData->applyVars)); // Copy command
            OS_MSGQ_Post ( &DFWA_MQueueHandle, pBuf );                           // Post it!
            DFW_PRNT_INFO("Apply Cmd Accepted");
            retVal = eAPPLY_RESP_APPLY_ACCEPTED;
         }
         else
         {
            retVal = eAPPLY_RESP_INTERNAL_ERROR; //Using this code to indicate it could'nt allocate the buffer
            DFW_PRNT_ERROR("BM_alloc()");
         }
      }
   }
   if ( eAPPLY_RESP_APPLY_ACCEPTED != retVal )             // Check for error
   {
      DFW_PRNT_INFO_PARMS("DL Apply Failed - Err: %d, Status: %d, Stream ID: %hhu",
                          (int32_t)retVal, (int32_t)dfwVars.eDlStatus, pReq->streamId);
   }
   return(retVal);
}

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION DEFINITIONS */

/*! ********************************************************************************************************************

   \fn bool getDeviceType( char *devTypeBuff, uint8_t devTypeBuffSize, uint8_t *devTypeLen )

   \brief Used to get the Device Type string.

   \param  ??

   \return  bool

   \details

 ******************************************************************************************************************** */
static bool getDeviceType( char *devTypeBuff, uint8_t devTypeBuffSize, uint8_t *devTypeLen )
{
   bool returnVal = false;
   char const *pDevType;

   if ( (NULL != devTypeBuff) && (NULL != devTypeLen) )
   {
      pDevType = VER_getComDeviceType();
      (void)strncpy (devTypeBuff, pDevType, devTypeBuffSize);
      devTypeBuff[devTypeBuffSize-1] = '\0';   // ensure the buffer is null-terminated
      *devTypeLen = (uint8_t)strlen(devTypeBuff);

      returnVal = true;
   }
   return(returnVal);
}

/*! ********************************************************************************************************************

   \fn bool isFirmwareVersionSupported(const firmwareVersion_u *pFirmwareVersion, eFwTarget_t target)

   \brief Used to check if the firmware version is supported.

   \param  firmwareVersion

   \return  bool

   \details this function should NOT be used when upgrading the METER FW Basecode

 ******************************************************************************************************************** */
static bool isFirmwareVersionSupported( const firmwareVersion_u *pFirmwareVersion, eFwTarget_t target, dl_dfwFileType_t fileType )
{
   bool returnVal = false;
   if( fileType == DFW_ACLARA_METER_PROGRAM_FILE_TYPE )
   {
      //meter programs do not target the meter firmware
      returnVal = true;
   }
#if ( ( EP== 1 ) && ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   else if( eFWT_METER_FW == target )
   {
      if( DFW_ACLARA_METER_FW_BASE_CODE_FILE_TYPE == fileType )
      {
         //meter base code is being updated so there is no validation of 'FROM' FW version
         returnVal = true;
      }
      else if( DFW_ACLARA_METER_FW_PATCH_FILE_TYPE == fileType )
      {
         //must be meter
         ValueInfo_t readingInfo;
         if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( edFwVersion, &readingInfo))
         {
            uint8_t baseFwVersion;
            uint8_t baseFwRevision;
            uint8_t baseFwBuild;

            if ( 3 == sscanf( (char *)readingInfo.Value.buffer, "%hhu.%hhu.%hhu", &baseFwVersion, &baseFwRevision, &baseFwBuild ) )
            {
               if( ( pFirmwareVersion->field.version  == baseFwVersion  ) &&
                   ( pFirmwareVersion->field.revision == baseFwRevision ) &&
                   ( pFirmwareVersion->field.build    == baseFwBuild    ) )
               {
                  returnVal = true;
               }
            }
         }
      }
   }
   else if( eFWT_APP == target || eFWT_BL == target )
#endif
   {
      firmwareVersion_u ver = VER_getFirmwareVersion(target);

      if ( !memcmp(pFirmwareVersion, &ver, sizeof(ver)) )
      {
         returnVal = true;
      }
   }
   return(returnVal);
}

/*! ********************************************************************************************************************

   \fn bool checkFirmwarePatchSize(const firmwareVersion_u *pFirmwareVersion, eFwTarget_t target)

   \brief Used to check if the oatch size is appropriate for the request

   \param  firmwareVersion

   \return  bool

   \details this function should NOT be used when upgrading the METER FW Basecode

 ******************************************************************************************************************** */

static bool isValidPatchSize( dl_patchsize_t patchSize, eDfwCiphers_t cipher, dl_dfwFileType_t reqFileType )
{
  bool retVal = true;
  //Whether cipher is enabled or not the patch must be within this size
  if ( patchSize < DFWI_MIN_PATCH_SIZE )
  {   // The patch is too small to be correct or does not fit in the partition
    retVal = false;
    DFW_PRNT_INFO("Init Failed, Too small Patch Size");
  }
  else if ( ( reqFileType == DFW_ENDPOINT_FIRMWARE_FILE_TYPE ) &&
            ( patchSize > DFWI_MAX_NIC_FW_PATCH_SIZE ) )
  {
    retVal = false;
    DFW_PRNT_INFO("Init Failed, Patch Size too large for NIC update");
  }
#if ( EP == 1 )
  else if (   IS_END_DEVICE_TARGET(reqFileType) &&
            ( patchSize > (dl_patchsize_t)DFWI_MAX_ED_FW_PATCH_SIZE ) )
  {
    retVal = false;
    DFW_PRNT_INFO("Init Failed, Patch Size too large for ED update");
  }
#endif
  //If this is an encrypted patch, make sure not too small or too big after the one sector adjustment
  else if (  (eDFWS_NONE != cipher) && (DFWI_MIN_CIPHER_PATCH_SIZE > patchSize)  )
  {   // The patch is too small to be correct or does not fit in the allowed space
    retVal = false;
    DFW_PRNT_INFO("Init Failed, Patch Size");
  }
  else if ( ( reqFileType == DFW_ENDPOINT_FIRMWARE_FILE_TYPE &&
             (eDFWS_NONE != cipher) &&
             (DFWI_MAX_CIPHER_NIC_FW_PATCH_SIZE < patchSize) ) )
  {
    retVal = false;
    DFW_PRNT_INFO("Init Failed, Patch Size too large for NIC programming");
  }
#if ( EP == 1 )
  else if (   IS_END_DEVICE_TARGET(reqFileType) &&
            ( eDFWS_NONE != cipher )            &&
            ( DFWI_MAX_CIPHER_ED_FW_PATCH_SIZE < patchSize ) )
  {
    retVal = false;
    DFW_PRNT_INFO("Init Failed, Patch Size too large for ED programming");
  }
#endif
  return retVal;

}

/*! ********************************************************************************************************************
   \fn bool isDownloadTimeExpired()

   \brief Used to check if the download time has expired.

   \return  bool

   \details

 ******************************************************************************************************************** */
static bool isDownloadTimeExpired(void)
{
   sysTimeCombined_t  currentDateTime;  /* Contains current system combined date/time format */
   bool           retVal = false;   /* Assume the patch is NOT expired */
   DFW_vars_t     dfwVars;          // Contains all of the DFW file variables

   (void)DFWA_getFileVars(&dfwVars);                     // Read the DFW NV variables (status, etc...)

   if (eSUCCESS == TIME_UTIL_GetTimeInSysCombined(&currentDateTime))   // Get the Current Date/Time
   {  /* Current time is valid */
      if (currentDateTime > dfwVars.initVars.expirationTime)   /* Is sys time past the expiration time? */
      {  /* The patch has expired */
         retVal = true;
      }
   }
   return(retVal);
}

/***********************************************************************************************************************

   Function name: erasePatchSpace

   Purpose: Erases enough space for the newly initialized Download.
            The size is adjusted to erase the entire last sector.  This is done to keep the erase from having to
            re-write any previously existing patch data in that sector saving some Flash endurance.

   Arguments:  dl_patchsize_t patchSize - Size of patch to be downloaded
               uint8_t        encrypted - Is patch encrypted, 0 = No, !0 = Yes

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
static returnStatus_t erasePatchSpace( dl_patchsize_t patchSize, eDfwCiphers_t encrypted )
{
   returnStatus_t           retVal = eFAILURE;     /* Error code */
   PartitionData_t const   *pPatchPTbl;            /* Patch partition */
   ePartitionName           patchPartition;
   uint32_t                 patchPartitionSize;

   if (eSUCCESS == DFWA_GetPatchParititonName( &patchPartition ) &&
       eSUCCESS == PAR_partitionFptr.parOpen(&pPatchPTbl, patchPartition, (uint32_t)0))
   {
      patchPartitionSize = DFWA_GetPatchParititonSize();
      if ( encrypted )
      {  //Encrypted patches size needs to be adjusted since stored one sector higher in partition
         patchSize += EXT_FLASH_ERASE_SIZE;
      }
      if ( patchSize % EXT_FLASH_ERASE_SIZE )
      {  //If not an exact multiple sector size, round up to a full sector
         patchSize += EXT_FLASH_ERASE_SIZE - patchSize % EXT_FLASH_ERASE_SIZE;
      }
      if( patchSize > patchPartitionSize )
      {  //Shoule NEVER happen, but done just to be sure NV is not corrupted!
         patchSize = patchPartitionSize;
      }
      // Erase the Patch partition starting at offset 0 through last sector of patch size.
      retVal = PAR_partitionFptr.parErase((dSize)0, (lCnt)patchSize, pPatchPTbl);
   }

   //if we are not using the DFW_PATCH partition for patching we should clear it for security
   if ( retVal == eSUCCESS && patchPartition != ePART_NV_DFW_PATCH )
   {
     if (eSUCCESS == PAR_partitionFptr.parOpen(&pPatchPTbl, ePART_NV_DFW_PATCH, (uint32_t)0))
     {
       retVal = PAR_partitionFptr.parErase((dSize)0, (lCnt)PART_NV_DFW_PATCH_SIZE, pPatchPTbl);

     }
     else
     {
       retVal = eFAILURE;
     }

   }
   return(retVal);
}

/***********************************************************************************************************************

   Function name: dlPcktWriteToNv

   Purpose: Checks for existence of previously downloaded packet.
            If it exists:
               Compare contents are to the current packet.
                  Return success, if contents match.
                  Clear associated bit in bit field and return failure, otherwise.
            If packet doesn't exist:
               Write contents to flash and set associated bit in bit field.
               Return success.

   Arguments:  uint16_t        packetNum   - Packet number (ID) of the packet
               uint8_t        *Data        - Payload to save
               uint16_t        packetSize  - Number of bytes to save
               uint8_t         beginSector - First sector to write Patch
               eDlStatus_t     status      - Current DFW status
               uint16_t packetSizeMultiplier - used to calculate offset into parition to write new packet

   Returns: returnStatus_t - defined by error_codes.h

   Side Effects: None

   Reentrant Code: No

 **********************************************************************************************************************/
static returnStatus_t dlPcktWriteToNv( uint16_t packetNum, const uint8_t *pData, uint16_t packetSize,
                                       uint8_t beginSector, eDlStatus_t status, uint16_t packetSizeMultiplier )
{
   returnStatus_t retVal = eFAILURE;               /* Error code */
   uint8_t        Bit;                             /* Bit status in the bit field that matched packet number */
   uint8_t        Buffer[DFWI_MAX_DL_PKT_SIZE];    /* Buffer to store the data read from NV. */
   dSize          FlashAddr;                       /* Compute image partition addr */

   if ( packetSize <= sizeof(Buffer) )             /* Check for valid size */
   {
      PartitionData_t const   *pPatchPTbl;         // Patch partition
      ePartitionName patchPartitionName;           //patch parition being used

      ( void) DFWA_GetPatchParititonName( &patchPartitionName );

      //Set Address to write the packet (could be offset by a number of sectors for decryption later)
      FlashAddr = (dSize)(packetNum * packetSizeMultiplier + beginSector * EXT_FLASH_ERASE_SIZE);
      retVal = DFWP_getBitStatus(packetNum, &Bit); /* Read the bit associated with this packet  */
      if ((eSUCCESS == retVal) && (eSUCCESS == PAR_partitionFptr.parOpen(&pPatchPTbl, patchPartitionName, (uint32_t)0)))
      {
         if ( Bit )  /* Test if packet was already received */
         {  /* Bit set - packet already received. Read the existing packet. */
            retVal = PAR_partitionFptr.parRead(&Buffer[0], (dSize)FlashAddr, (lCnt)packetSize, pPatchPTbl);
            if ( eSUCCESS == retVal )
            {  /*lint -e(670)  packet size is checked to make sure it is NOT larger than the buffer. */
               if ( memcmp(Buffer, pData, packetSize) )  // Compare received packet with the one previously saved
               {  /* If all packets already recieved, but there was an error during the pre-patch,
                     eDlStatus is eDFWS_INITIALIZED_WITH_ERROR, eErrorCode is not NO_ERROR and the packets do not match,
                     then need to replace packet with new one instead of just clearing the packet received bit */
                  /* NOTE: MUST further qualify for a decryption error prior to calling this function. If the patch
                           failed to decrypt, then the patch data is corrupted during the decryption process.  Allowing
                           replacement packets at this point requires ALL packets to be downloaded again and starting
                           the signature and decryption process over.  The better solution is to force DFW to be
                           reinitialized instead.
                  */
                  if (eDFWS_INITIALIZED_WITH_ERROR == status)
                  {
                     retVal = PAR_partitionFptr.parWrite((dSize)FlashAddr, pData, (lCnt)packetSize, pPatchPTbl);
                  }
                  else
                  {
                     retVal = DFWP_modifyBit(packetNum, eDL_BIT_CLEAR);   // Mismatch, clear bit in bit field
#if ( EP == 1 )
                     DFWA_CounterInc(dfwDupDiscardPacketQty);
#endif
                  }
               }
            }
         }
         else
         {  /* Bit clear - save this packet and record its receipt. */
            retVal = PAR_partitionFptr.parWrite((dSize)FlashAddr, pData, (lCnt)packetSize, pPatchPTbl);
            if ( eSUCCESS == retVal )
            {  /* Set bit in bit field to indicate that packet was successfully received and saved */
               retVal = DFWP_modifyBit(packetNum, eDL_BIT_SET);
            }
         }
      }
   }
   return(retVal);
}

/*! ********************************************************************************************************************

   \fn static uint16_t packReading(simpleReading_t *rdgInfo)

   \brief Used to pack a reading type and optional value into a packet.

   \param  *rdgInfo  -  pointer to needed reading info to pack

   \return  bytes - number of bytes packet into buffer

   \details

 ******************************************************************************************************************** */
static uint16_t packReading(simpleReading_t *rdgInfo)
{
   ExWId_ReadingInfo_u readingInfo;
   uint16_t            bytes=0;

   if ( (NULL != rdgInfo) && (NULL != rdgInfo->pPackCfg) )
   {
      (void)memset(&readingInfo.bytes[0], 0, sizeof(readingInfo.bytes));   //Clear all parameters

      // Are we packing a successful reading
      if( CIM_QUALCODE_SUCCESS != rdgInfo->qualityCode)
      {  // set quality code bit and set reading size to 0
         readingInfo.bits.RdgQualPres = (bool)true;
         readingInfo.bits.RdgValSize  = 0;
         readingInfo.bits.RdgValSizeU = 0;
      }
      else
      {  // reading is good, set reading size
         readingInfo.bits.RdgValSize  = rdgInfo->rdgSize & RDG_VAL_SIZE_L_MASK;
         readingInfo.bits.RdgValSizeU = (rdgInfo->rdgSize & RDG_VAL_SIZE_U_MASK) >> RDG_VAL_SIZE_U_SHIFT;
      }

      readingInfo.bits.typecast    = (uint16_t)rdgInfo->typecast;
      PACK_Buffer( sizeof(readingInfo.bytes) * 8, &readingInfo.bytes[0], rdgInfo->pPackCfg );
      bytes                       += sizeof(readingInfo.bytes);
      PACK_Buffer( -(int16_t)(sizeof(rdgInfo->type) * 8), (uint8_t *)&rdgInfo->type, rdgInfo->pPackCfg );
      bytes                       += sizeof(rdgInfo->type);

      if( CIM_QUALCODE_SUCCESS != rdgInfo->qualityCode)
      {  // if there is a quality code to pack, go ahead and pack it
         PACK_Buffer( (int16_t)(sizeof(rdgInfo->qualityCode) * 8), (uint8_t *)&rdgInfo->qualityCode, rdgInfo->pPackCfg );
         bytes += sizeof(rdgInfo->qualityCode);
      }
      else
      {  // pack the value
         if ( (0 < rdgInfo->rdgSize) && (NULL != rdgInfo->pvalue) )
         {
            if (false == rdgInfo->byteSwap)
            {
               PACK_Buffer((int16_t)(rdgInfo->rdgSize * 8), (uint8_t *)rdgInfo->pvalue, rdgInfo->pPackCfg);
            }
            else
            {
               PACK_Buffer(-(int16_t)(rdgInfo->rdgSize * 8), (uint8_t *)rdgInfo->pvalue, rdgInfo->pPackCfg);
            }
            bytes += rdgInfo->rdgSize;
         }
      }
   }
   return(bytes);
}

/*! ********************************************************************************************************************

   \fn static uint16_t packMissingPackets( dfw_verify_t *pverify,  pack_t *ppackCfg, buffer_t *pPktBuf )

   \brief Used to pack the reading value for dfwMissingPacketIDs.

   \param   numToPack -  Number of missing packets to pack
   \param  *pmPData   -  Pointer to Missing Packet ID data
   \param  *ppackCfg  -  pointer to the packCfg info for packing the buffer
   \param  *pPktBuf   -  pointer to the buffer

   \return  bytes - number of reading quantities packeted into buffer

   \details

 ******************************************************************************************************************** */
static uint8_t packMissingPackets( dl_packetcnt_t numToPack,  dl_packetid_t  *pmPData, pack_t *ppackCfg,
                                   buffer_t *pPktBuf )
{
   //Get and send back missing packet quantity
   uint8_t             rdgQtys;
   uint16_t            rdgSize;
   simpleReading_t     simpleRdgInfo;
   uint16_t            i;
   uint16_t            rdgInfoOffset;   //Used to know later where the the reading info is
   uint8_t             bytes;                             // Number of characters converted into string
   uint8_t             textStr[MAX_MISSING_PKT_ID_STR];   // Used for next string into packet
   ExWId_ReadingInfo_u rdgInfo;

   rdgQtys = 0;
   if ( (NULL != pmPData) && (NULL != ppackCfg) && (NULL != pPktBuf) )
   {
      simpleRdgInfo.pPackCfg = ppackCfg;  //Need to do this once in either case below

      //Send the missing packet ID's
      //send missing packets in numerical order, grouping consecutive ID's when possible
      //include as many missing packets as will fit in a message
      //Final Reading Size will be corrected later
      rdgInfoOffset          = pPktBuf->x.dataLen;
      simpleRdgInfo.type     = dfwMissingPacketID;
      simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;
      simpleRdgInfo.pvalue   = NULL;
      simpleRdgInfo.rdgSize  = 0;
      simpleRdgInfo.typecast = ASCIIStringValue;
      simpleRdgInfo.byteSwap = false;
      pPktBuf->x.dataLen    += packReading(&simpleRdgInfo);
      //Get copy of  readingInfo struct for later
      (void)memcpy( &rdgInfo.bytes[0], &pPktBuf->data[rdgInfoOffset], sizeof(rdgInfo.bytes) );

      // Put the first missing packet id in the buffer
      bytes = (uint8_t)snprintf((char *)&textStr[0], MAX_MISSING_PKT_ID_STR, "%hu", (uint16_t)*pmPData);
      PACK_Buffer((bytes * 8), (uint8_t *)&textStr[0], simpleRdgInfo.pPackCfg);
      rdgSize             = bytes;
      pPktBuf->x.dataLen += bytes;
      for ( i = 1; i < numToPack; i++)
      {
         pmPData++;
         if (*pmPData < 0)
         {
            *pmPData *= -1;
            // Put the ending consecutive missing packet id in the buffer
            bytes = (uint8_t)snprintf((char *)&textStr[0], MAX_MISSING_PKT_ID_STR, "-%d", *pmPData);
            // Break the buffer packing if the buffer reaches the maximum size
            if ( ( pPktBuf->x.dataLen + bytes ) > pPktBuf->bufMaxSize )
            {
               break;
            }

            PACK_Buffer((bytes * 8), (uint8_t *)&textStr[0], simpleRdgInfo.pPackCfg);
            rdgSize            += bytes;
            pPktBuf->x.dataLen += bytes;
         }
         else
         {
            // Put the next non-consecutive missing packet id in the buffer
            bytes = (uint8_t)snprintf((char *)&textStr[0], MAX_MISSING_PKT_ID_STR, ",%d", *pmPData);
            // Break the buffer packing if the buffer reaches the maximum size
            if ( ( pPktBuf->x.dataLen + bytes ) > pPktBuf->bufMaxSize )
            {
               break;
            }

            PACK_Buffer((bytes * 8), (uint8_t *)&textStr[0], ppackCfg);
            rdgSize            += bytes;
            pPktBuf->x.dataLen += bytes;
         }
      }

      //Insert the corrected reading size
      rdgInfo.bits.RdgValSize  = rdgSize & RDG_VAL_SIZE_L_MASK;
      rdgInfo.bits.RdgValSizeU = (rdgSize & RDG_VAL_SIZE_U_MASK) >> RDG_VAL_SIZE_U_SHIFT;
      //Put correct reading value size in packet
      (void)memcpy(&pPktBuf->data[rdgInfoOffset], &rdgInfo.bytes[0], sizeof(rdgInfo.bytes));
      rdgQtys = 1;
   }

   return(rdgQtys);
}

/*!
   Used to set the DFW Status
*/
void DFW_SetStatus(eDlStatus_t status)
{

   DFW_vars_t              dfwVars;       // Contains all of the DFW file variables

   (void)DFWA_getFileVars(&dfwVars);      // Read the DFW NV variables (status, etc...)

   dfwVars.eDlStatus = status;

   (void)DFWA_setFileVars(&dfwVars);      // Write DFW NV variables (status, etc...)
}

/*!
   Used to read the DFW Status
*/
eDlStatus_t DFW_Status(void)
{
   DFW_vars_t              dfwVars;       // Contains all of the DFW file variables

   (void)DFWA_getFileVars(&dfwVars);      // Read the DFW NV variables (status, etc...)

   return dfwVars.eDlStatus;

}


/*!
   Used to set the DFW Stream Id
*/
void DFW_SetStreamId(uint8_t streamId)
{
   DFW_vars_t              dfwVars;

   (void)DFWA_getFileVars(&dfwVars);      // Read the DFW NV variables (status, etc...)

   dfwVars.initVars.streamId = streamId;

   (void)DFWA_setFileVars(&dfwVars);      // Write DFW NV variables (status, etc...)

}

/*!
   Used to read the DFW Stream Id
*/
uint8_t DFW_StreamId(void)
{
   DFW_vars_t              dfwVars;       // Contains all of the DFW file variables

   (void)DFWA_getFileVars(&dfwVars);      // Read the DFW NV variables (status, etc...)

   return dfwVars.initVars.streamId;
}

/*!
   Used to read the DFW File Type
*/
dl_dfwFileType_t DFW_GetFileType(void)
{
   DFW_vars_t              dfwVars;       // Contains all of the DFW file variables

   (void)DFWA_getFileVars(&dfwVars);      // Read the DFW NV variables (status, etc...)

   return dfwVars.fileType;

}

/*! ********************************************************************************************************************

   \fn static dfwErrorcodeLUT_t dfwErrorLookUp( const DFW_vars_t *dfwVars)

   \brief Return information  to pack into HEEP message for dfw error types to the calling function

   \param   *dfwVars -  A pointer to the dfw variables

   \return  dfwErrorcodeLUT_t - number of reading quantities packeted into buffer

   \details

 ******************************************************************************************************************** */
static dfwErrorcodeLUT_t dfwErrorLookUp( const DFW_vars_t *dfwVars)
{
   dfwErrorcodeLUT_t errorData;

   switch( dfwVars->eErrorCode )
   {
      case (eDFWE_CRC32PATCH):
         errorData.dfwErrorCode = dfwCRCErrorPatch;
         errorData.dfwErrorTypecast = Boolean;
         break;
      case (eDFWE_CRC32):
         errorData.dfwErrorCode = dfwCRCErrorImage;
         errorData.dfwErrorTypecast = Boolean;
         break;
      case (eDFWE_PATCH_FORMAT):
         errorData.dfwErrorCode     = dfwPatchFormatVersion;
         errorData.dfwErrorTypecast = uintValue;
         break;
      case (eDFWE_FW_VER):
         if( eFWT_BL == dfwVars->initVars.target )
         {
            errorData.dfwErrorCode     = comDeviceBootloaderVersion; // bootloader reading type
         }
         else
         {
            errorData.dfwErrorCode     = comDeviceFirmwareVersion; // Default to Application
         }
         errorData.dfwErrorTypecast = ASCIIStringValue;
         break;
      case (eDFWE_PATCH_LENGTH):
         errorData.dfwErrorCode     = dfwPatchSize;
         errorData.dfwErrorTypecast = uintValue;
         break;
      case (eDFWE_DEV_TYPE):
         errorData.dfwErrorCode     = comDeviceType;
         errorData.dfwErrorTypecast = ASCIIStringValue;
         break;
      case (eDFWE_EXPIRED):
         errorData.dfwErrorCode     = dfwExpiry;
         errorData.dfwErrorTypecast = dateTimeValue;
         break;
      case (eDFWE_SCH_AFTER_EXPIRE):
         errorData.dfwErrorCode     = dfwInvalidSchedule;
         errorData.dfwErrorTypecast = dateTimeValue;
         break;
      case (eDFWE_STREAM_ID):
         errorData.dfwErrorCode     = dfwStreamID;
         errorData.dfwErrorTypecast = uintValue;
         break;
      case (eDFWE_CMD_CODE):
         errorData.dfwErrorCode     = dfwInvalidPatchCmd;
         errorData.dfwErrorTypecast = Boolean;
         break;
      case (eDFWE_SIGNATURE):
         errorData.dfwErrorCode     = dfwSignatureErrorPatch;
         errorData.dfwErrorTypecast = Boolean;
         break;
      case (eDFWE_DECRYPT):
         errorData.dfwErrorCode     = dfwDecryptionErrorPatch;
         errorData.dfwErrorTypecast = Boolean;
         break;
      case (eDFWE_INVALID_TIME):
      case (eDFWE_NV):              //Intentional fall through
      case (eDFWE_FLASH):           //Intentional fall through
      case (eDFWE_INVALID_STATE):   //Intentional fall through
         errorData.dfwErrorCode = dfwInternalError;
         errorData.dfwErrorTypecast = Boolean;
         break;
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
      case (eDFWE_DFW_COMPATIBLITY_TEST):  //Intentional fall through
      case (eDFWE_DFW_PROGRAM_SCRIPT):     //Intentional fall through
      case (eDFWE_DFW_AUDIT_TEST):         //Intentional fall through
#endif
      case ( eDFWE_INCOMPLETE ):           //Intentional fall through
      case ( eDFWE_NO_ERRORS ):            //Intentional fall through
      default:
         errorData.dfwErrorCode = invalidReadingType;
         break;
   }
   return errorData;
}

/*! ********************************************************************************************************************

   \fn static uint8_t packDfwFileSectionStatusParmeters( DFW_vars_t *dfwVars, buffer_t *pBuf, pack_t *packCfg)

   \brief   Used to pack the DFW file section status parameters into a buffer

   \param   *dfwVars  -  A pointer to the dfw variables
            *pbuf     -  A pointer to the buffer_t where data is being stored
            *packCfg  -  A pointer to the pack_t struct used for packing the data

   \return  uint8_t - number readings packed into the buffer

   \details

 ******************************************************************************************************************** */
#if ( ( EP == 1 ) && ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) ) )
static uint8_t packDfwFileSectionStatusParmeters( DFW_vars_t *dfwVars, buffer_t *pBuf, pack_t *packCfg)
{
   simpleReading_t   simpleRdgInfo;
   uint8_t rdgQty = 0;

   simpleRdgInfo.pPackCfg = packCfg;

   /* initialize the quality code value for all readings packed.  No need to update quality code
      value for each reading since data source is from dfwVARS.  Quality code will only need to
      updated if getting values from an endDevice. */
   simpleRdgInfo.qualityCode = CIM_QUALCODE_SUCCESS;

   //Include dfwCompatibilityTestStatus
   simpleRdgInfo.type     = dfwCompatibilityTestStatus;
   simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars->fileSectionStatus.compatibilityTestStatus;
   simpleRdgInfo.rdgSize  = sizeof(dfwVars->fileSectionStatus.compatibilityTestStatus);
   simpleRdgInfo.typecast = uintValue;
   simpleRdgInfo.byteSwap = true;
   pBuf->x.dataLen       += packReading(&simpleRdgInfo);
   ++rdgQty;

   //Include dfwProgramScriptStatus
   simpleRdgInfo.type     = dfwProgramScriptStatus;
   simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars->fileSectionStatus.programScriptStatus;
   simpleRdgInfo.rdgSize  = sizeof(dfwVars->fileSectionStatus.programScriptStatus);
   simpleRdgInfo.typecast = uintValue;
   simpleRdgInfo.byteSwap = true;
   pBuf->x.dataLen       += packReading(&simpleRdgInfo);
   ++rdgQty;

   //Include dfwAuditTestStatus
   simpleRdgInfo.type     = dfwAuditTestStatus;
   simpleRdgInfo.pvalue   = (uint8_t *)&dfwVars->fileSectionStatus.auditTestStatus;
   simpleRdgInfo.rdgSize  = sizeof(dfwVars->fileSectionStatus.auditTestStatus);
   simpleRdgInfo.typecast = uintValue;
   simpleRdgInfo.byteSwap = true;
   pBuf->x.dataLen       += packReading(&simpleRdgInfo);
   ++rdgQty;

   return rdgQty;
}
#endif

/*! ********************************************************************************************************************

   \fn static bool validAnsiTableOID( const uint8_t *reqAnsiTableOidBuf )

   \brief   Used validate a requested ansiTableOID reading to the current value stored in the end device

   \param   uint8_t *  -  A pointer to the requested ansiTableOID

   \return  bool - comparison status status

   \details

 ******************************************************************************************************************** */
#if ( ( EP == 1 ) && ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) )
static bool isValidAnsiTableOID( const uint8_t *reqAnsiTableOidBuf )
{
   bool retVal = false;
   uint8_t currentAnsiTableOid[EDCFG_ANSI_TBL_OID_LENGTH];            // declare a buffer to store the current value

   (void)memset(currentAnsiTableOid, 0, sizeof(currentAnsiTableOid)); // initialize the declared buffer
   (void)INTF_CIM_CMD_getConfigMfr( currentAnsiTableOid );            // acquire the ansiTblOID value from the meter

   if( memcmp(reqAnsiTableOidBuf, currentAnsiTableOid, sizeof(currentAnsiTableOid)) == 0 )
   {  // There is a difference between the requested ansiTableOID vs the current ansiTableOID in the end device
      retVal = true;
   }

   return retVal;
}
#endif

/*! ********************************************************************************************************************

   \fn static bool validEdModel( const uint8_t *reqEdModelBuf )

   \brief   Used validate a requested edModel reading to the current value stored in the end device

   \param   uint8_t * -  A pointer to the requested edModel

   \return  bool - comparison status status

   \details

 ******************************************************************************************************************** */
#if ( ( EP == 1 ) &&( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static bool isValidEdModel( const uint8_t *reqEdModelBuf, uint8_t bufSize )
{
   bool retVal = false;
   ValueInfo_t reading;
	// add check to compare lengths and make sure they are equal
   if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading(edModel, &reading) )
   {  // we successfully read the model from the meter table, we need to do a compare to see if they match
      if( reading.valueSizeInBytes <= bufSize )
      {  // ensure the model string length is not longer than the buffer, if not no need to compare the strings
         if( memcmp(reqEdModelBuf, reading.Value.buffer, reading.valueSizeInBytes ) == 0 )
         {  // There is a difference between the requested ed model vs the current ed model in the end device
            retVal = true;
         }
      }
   }

   return retVal;
}
#endif
