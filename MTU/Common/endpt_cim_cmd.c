/***********************************************************************************************************************
 *
 * Filename:	endpt_cim_cmd.c
 *
 * Global Designator: ENDPT_CIM_CMD_
 *
 * Contents: API to access from CIM module to Endpoint
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013-2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $log$ CM Created 032515
 *
 ***********************************************************************************************************************
 * Revision History:
 * 032515 CM  - Initial Release
 *
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#define ENDPT_CIM_CMD_GLOBAL
#include "endpt_cim_cmd.h"
#undef  ENDPT_CIM_CMD_GLOBAL

//#include "object_list.h"
#include "ansi_tables.h"
#include "time_sys.h"
#include "DBG_SerialDebug.h"
#include "RG_MD_Handler.h"
#include "APP_MSG_handler.h"
#include "ID_intervalTask.h"
#include "time_DST.h"
#include "version.h"
#include "MAC.h"
#include "demand.h"
#include "EVL_event_log.h"
#include "MAC_Protocol.h"
#include "dfw_app.h"

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define STRING_BUFFER_SIZE 32

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/*lint -e{714}  xxx For now, some of these functions may not be called. */

/***********************************************************************************************************************
 *
 * Function Name: ENDPT_CIM_CMD_getDataType
 *
 * Purpose: Get the data type of a meter reading type
 *
 * Arguments:
 *     meterReadingType RdgType: Unit of measure (what to read)
 *
 * Side Effects: none
 *
 * Re-entrant: Yes
 *
 * Returns: ReadingsValueTypecast: data type of the meter reading type (or -1 if unknown)
 *
 * Notes: When a new reading type is being added, that reading type must be added to this routine
 *        to specify the data type of the return value.  In addition, that data type must be added
 *        to the appropriate 'get value' routine: ENDPT_CIM_CMD_getIntValueAndMetadata,
 *        ENDPT_CIM_CMD_getStrValue, etc.
 *
 **********************************************************************************************************************/
ReadingsValueTypecast ENDPT_CIM_CMD_getDataType( meterReadingType RdgType )
{
   ReadingsValueTypecast rdgTypeDataType = (ReadingsValueTypecast)-1;
   switch(RdgType)   /*lint !e788 not all enums used within switch */
   {
      case comDeviceFirmwareVersion:    // Field equipment's firmware version
         rdgTypeDataType = ASCIIStringValue;
         break;
      case comDeviceBootloaderVersion:  // Field equipment's bootloader version
         rdgTypeDataType = ASCIIStringValue;
         break;
      case comDeviceHardwareVersion:    // Field equipment's hardware version
         rdgTypeDataType = ASCIIStringValue;
         break;
      case installationDateTime:   // Installation date/time
         rdgTypeDataType = dateTimeValue;
         break;
      case comDeviceMACAddress:   // Factory-assigned long MAC address
         rdgTypeDataType = uintValue;
         break;
      case comDeviceType:   // Official Field Equipment Type
         rdgTypeDataType = ASCIIStringValue;
         break;
      case comDeviceMicroMPN:   // Official Field Equipment Type
         rdgTypeDataType = ASCIIStringValue;
         break;
      case demandPresentConfiguration:   // Demand Present Configuration
         rdgTypeDataType = uintValue;
         break;
      case lpBuSchedule:   // Load Profile Bubble-up Schedule
         rdgTypeDataType = uintValue;
         break;
      case newRegistrationRequired:
         rdgTypeDataType = Boolean;
         break;
      case capableOfEpBootloaderDFW: // Supports DFW of the EP bootloader
         rdgTypeDataType = Boolean;
         break;
      case capableOfEpPatchDFW:  // Supports DFW to the EndPoint
         rdgTypeDataType = Boolean;
         break;
      case capableOfMeterBasecodeDFW: // Supports meter FW basecode downloads
         rdgTypeDataType = Boolean;
         break;
      case capableOfMeterPatchDFW:  // Supports meter FW patch downloads
         rdgTypeDataType = Boolean;
         break;
      case capableOfMeterReprogrammingOTA: // Supports meter reprogramming over the air
         rdgTypeDataType = Boolean;
         break;
      case realTimeAlarmIndexID:       // intentional fall through case
      case opportunisticAlarmIndexID:  // intentional fall through case
      case timeZoneDstHash:
         rdgTypeDataType = uintValue;
         break;
      default:
         rdgTypeDataType = (ReadingsValueTypecast)-1;
         break;
   }  /*lint !e788 not all enums used within switch */

   return (rdgTypeDataType);
}

/***********************************************************************************************************************
 *
 * Function Name: ENDPT_CIM_CMD_getIntValueAndMetadata
 *
 * Purpose: Get an integer value from the endpoint and meta-data associated with reading type
 *
 * Arguments:
 *     meterReadingType RdgType: Unit of measure (what to read)
 *     int64_t *value: Return the value read from host meter
 *     uint8_t *powerOf10: Return power of 10 from the meta-data
 *
 * Side Effects: none
 *
 * Re-entrant: Yes
 *
 * Returns: enum_CIM_QualityCode: Status of the transaction
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getIntValueAndMetadata( meterReadingType RdgType, int64_t *value, uint8_t *powerOf10 )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; //default, failed
   *powerOf10 = 0; //default

   switch(RdgType)   /*lint !e788 not all enums used within switch */
   {
      case comDeviceMACAddress:   // Factory-assigned long MAC address
         retVal = ENDPT_CIM_CMD_getCommMACID((uint64_t *)value);
         break;
      case demandPresentConfiguration:   // Demand Present Configuration
         retVal = ENDPT_CIM_CMD_getDemandPresentConfig((uint64_t *)value);
         break;
      case lpBuSchedule:      // Load Profile Bubble-up Schedule
         retVal = ENDPT_CIM_CMD_getBubbleUpSchedule((uint64_t *)value);
         break;
      case timeZoneDstHash:   // Time zone / DST hash
         retVal = ENDPT_CIM_CMD_getTimeZoneDstHash((uint64_t *)value);
         break;
      case realTimeAlarmIndexID:
         retVal = ENDPT_CIM_CMD_getRealTimeAlarmIndexID((uint64_t *)value);
         break;
      case opportunisticAlarmIndexID:
         retVal = ENDPT_CIM_CMD_getOpportunisticAlarmIndexID((uint64_t *)value);
         break;
      default:
         retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
         break;
   }  /*lint !e788 not all enums used within switch */

   return retVal;
}

/***********************************************************************************************************************
 *
 * Function Name: ENDPT_CIM_CMD_getStrValue
 *
 * Purpose: Get a string value from the endpoint associated with reading type
 *
 * Arguments:
 *     meterReadingType RdgType: Unit of measure (what to read)
 *     char *valBuff: Return the value read from host meter (includes the trailing null)
 *     uint8_t valBuffSize: The size of valBuff, into which the value will be copied, including the trailing null
 *     uint8_t *readingLeng: The number of bytes in the returned reading, not including the trailing null
 *
 * Side Effects: none
 *
 * Re-entrant: Yes
 *
 * Returns: enum_CIM_QualityCode: Status of the transaction
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getStrValue( meterReadingType RdgType,
                                                uint8_t *valBuff, uint8_t valBuffSize, uint8_t *readingLeng )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; //default, failed
   firmwareVersion_u comDevFwVer;

   switch(RdgType)   /*lint !e788 not all enums used within switch */
   {
      case comDeviceFirmwareVersion:   // Field equipment's firmware version
         retVal = ENDPT_CIM_CMD_getCommFWVerNum( &comDevFwVer );
         (void)snprintf ((char *)valBuff, valBuffSize, "%d.%d.%d",
                  comDevFwVer.field.version, comDevFwVer.field.revision, (int)comDevFwVer.field.build);
         valBuff[valBuffSize-1] = '\0';   // ensure the buffer is null-terminated
         *readingLeng = (uint8_t)strlen((char *)valBuff);
         break;
      case comDeviceBootloaderVersion: // Field equipment's bootloader version
         retVal = ENDPT_CIM_CMD_getCommBLVerNum( &comDevFwVer );
         (void)snprintf ((char *)valBuff, valBuffSize, "%d.%d.%d",
                  comDevFwVer.field.version, comDevFwVer.field.revision, (int)comDevFwVer.field.build);
         valBuff[valBuffSize-1] = '\0';   // ensure the buffer is null-terminated
         *readingLeng = (uint8_t)strlen((char *)valBuff);
         break;
      case comDeviceHardwareVersion:    // Field equipment's hardware version
         retVal = ENDPT_CIM_CMD_getCommHWVerNum ((char *)valBuff, valBuffSize, readingLeng);
         break;
      case comDeviceType:   // Official Field Equipment Type
         retVal = ENDPT_CIM_CMD_getComDeviceType((char *)valBuff, valBuffSize, readingLeng);
         break;
      case comDeviceMicroMPN:   // Official Field Equipment Type
         retVal = ENDPT_CIM_CMD_getComDevicePartNumber((char *)valBuff, valBuffSize, readingLeng);
         break;
      default:
         retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
         break;
   }  /*lint !e788 not all enums used within switch */

   if (retVal == CIM_QUALCODE_SUCCESS)
   {   // Remove any trailing spaces from the return buffer
      while ((*readingLeng > 0) && (valBuff[(*readingLeng)-1] == ' '))
      {
         valBuff[(*readingLeng)--] = '\0';
      }
   }

   return retVal;
}

/***********************************************************************************************************************
 *
 * Function Name: ENDPT_CIM_CMD_getBoolValue
 *
 * Purpose: Get a boolean value from the endpoint associated with reading type
 *
 * Arguments:
 *     meterReadingType RdgType: Unit of measure (what to read)
 *     uint8_t *boolVal Return the value
 *
 * Side Effects: none
 *
 * Re-entrant: Yes
 *
 * Returns: enum_CIM_QualityCode: Status of the transaction
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getBoolValue( meterReadingType RdgType, uint8_t *boolVal )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; //default, failed

   switch(RdgType)   /*lint !e788 not all enums used within switch */
   {
      case newRegistrationRequired:
         retVal = ENDPT_CIM_CMD_getNewRegistrationRequired( boolVal );
         break;
      case capableOfEpPatchDFW:
         retVal = ENDPT_CIM_CMD_getCapableOfEpPatchDFW( boolVal );
         break;
      case capableOfEpBootloaderDFW:
         retVal = ENDPT_CIM_CMD_getCapableOfEpBootloaderDFW( boolVal );
         break;
      case capableOfMeterPatchDFW:
         retVal = ENDPT_CIM_CMD_getCapableOfMeterPatchDFW( boolVal );
         break;
      case capableOfMeterBasecodeDFW:
         retVal = ENDPT_CIM_CMD_getCapableOfMeterBasecodeDFW( boolVal );
         break;
      case capableOfMeterReprogrammingOTA:
         retVal = ENDPT_CIM_CMD_getCapableOfMeterReprogrammingOTA( boolVal );
         break;
      default:
         retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
         break;
   }  /*lint !e788 not all enums used within switch */

   return retVal;
}

/***********************************************************************************************************************
 *
 * Function Name: ENDPT_CIM_CMD_getDateTimeValue
 *
 * Purpose: Get a date/time value from the endpoint associated with reading type
 *
 * Arguments:
 *     meterReadingType RdgType: Unit of measure (what to read)
 *     uint32_t *dateTimeVal Return the value expressed as number of seconds since 1970-01-01T00:00:00 UTC in
 *                            the Zulu TZ
 *
 * Side Effects: none
 *
 * Re-entrant: Yes
 *
 * Returns: enum_CIM_QualityCode: Status of the transaction
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getDateTimeValue( meterReadingType RdgType, uint32_t *dateTimeVal )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; //default, failed

   switch(RdgType)   /*lint !e788 not all enums used within switch */
   {
      case installationDateTime:   // Installation Date/Time
         retVal = ENDPT_CIM_CMD_getInstallationDateTime( dateTimeVal );
         break;
      default:
         retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
         break;
   }  /*lint !e788 not all enums used within switch */

   return retVal;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCommFWVerNum
 *
 * Purpose: Gets the Communication device firmware version
 *
 * Arguments: firmwareVersion_u *fwVer
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommFWVerNum( firmwareVersion_u *fwVer )
{
   *fwVer = VER_getFirmwareVersion ( eFWT_APP );
   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCommBLVerNum
 *
 * Purpose: Gets the Communication device bootloader version
 *
 * Arguments: firmwareVersion_u *fwVer
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommBLVerNum( firmwareVersion_u *fwVer )
{
   *fwVer = VER_getFirmwareVersion ( eFWT_BL );
   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCommHWVerNum
 *
 * Purpose: Gets the Communication device hardware version
 *
 * Arguments: char *verBuff: Hardware version of the comm device (includes the trailing null)
 *            uint8_t verBuffSize: The size of verBuff, into which the value will be copied, including the trailing null
 *            uint8_t *verLeng: The number of bytes in the returned version, not including the trailing null
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommHWVerNum( char *verBuff, uint8_t verBuffSize, uint8_t *verLeng )
{
   ( void )VER_getHardwareVersion ((uint8_t *)verBuff, verBuffSize);
   verBuff[verBuffSize-1] = '\0';   // ensure the buffer is null-terminated
   *verLeng = ( uint8_t )strlen(verBuff);
   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCommMACID
 *
 * Purpose: Gets the Communication device MACID
 *
 * Arguments: uint64_t *macid: MACID of the comm device
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommMACID( uint64_t *fullMacAddr )
{
   int i;
   eui_addr_t eui_addr;
   // Read the Extended Unique Identifier
   MAC_EUI_Address_Get( (eui_addr_t *)eui_addr );

   // Return the address as a uint64_t
   // Fill in the 8 bytes from the eui that was retrieved.
   *fullMacAddr = 0;
   for (i=0; i<8; i++)
   {
      *fullMacAddr |= ((uint64_t)eui_addr[i]) << (((8 - 1) - i) * 8);
   }

   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getComDeviceType
 *
 * Purpose: Gets the Communication device MACID
 *
 * Arguments: char *devTypeBuff: Comm device type (includes the trailing null)
 *            uint8_t devTypeBuffSize: The size of devTypeBuff, into which the value will be copied, including the
 *                                     trailing null
 *            uint8_t *devTypeLeng: The number of bytes in the returned device type, not including the trailing null
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getComDeviceType( char *devTypeBuff, uint8_t devTypeBuffSize, uint8_t *devTypeLeng )
{
   char const *pDevType;

   pDevType = VER_getComDeviceType();
   (void)strncpy (devTypeBuff, pDevType, devTypeBuffSize);
   devTypeBuff[devTypeBuffSize-1] = '\0';   // ensure the buffer is null-terminated
   *devTypeLeng = (uint8_t)strlen(devTypeBuff);
   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getComDevicePartNumber
 *
 * Purpose: Gets the Communication device MACID
 *
 * Arguments: char *devTypeBuff: Comm device type (includes the trailing null)
 *            uint8_t devTypeBuffSize: The size of devTypeBuff, into which the value will be copied, including the
 *                                     trailing null
 *            uint8_t *devTypeLeng: The number of bytes in the returned device type, not including the trailing null
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getComDevicePartNumber( char *devPartNumberBuff, uint8_t devPartNumberBuffSize, uint8_t *devPartNumberLeng )
{
   char const *pDevPartNumber;
   pDevPartNumber = (char *)VER_getComDeviceMicroMPN();
   (void)strncpy (devPartNumberBuff, pDevPartNumber, devPartNumberBuffSize);
   devPartNumberBuff[devPartNumberBuffSize - 1] = '\0';   // ensure the buffer is null-terminated
   *devPartNumberLeng = (uint8_t)strlen(devPartNumberBuff);
   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getNewRegistrationRequired
 *
 * Purpose: Checks if the endpoint is newly installed.
 *
 * Arguments: uint8_t *newInstall: indicates whether the endpoint is newly installed, indicating that
 *                                 new registration is required.
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getNewRegistrationRequired( uint8_t *newInstall )
{
   switch (RegistrationInfo.registrationStatus)
   {
      case REG_STATUS_NOT_SENT:   // new install - registration never attempts
         *newInstall = true;
         break;
      case REG_STATUS_NOT_CONFIRMED:   // Registration Metadata sent out but confirmation never received
         *newInstall = true;
         break;
      case REG_STATUS_CONFIRMED:   // Confirmation received from Metadata registration
         *newInstall = false;
         break;
      default:  // Invalid value - let's treat this as a new registration
         *newInstall = true;
         break;
   }

   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getInstallationDateTime
 *
 * Purpose: Returns the endpoint installation time
 *
 * Arguments: uint32_t *installDateTime - Install time expressed as number of seconds since 1970-01-01T00:00:00 UTC in
 *                                        the Zulu TZ
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getInstallationDateTime( uint32_t *installDateTime )
{
  *installDateTime = TIME_SYS_GetInstallationDateTime();

   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getDemandPresentConfig
 *
 * Purpose: Returns how the endpoint forumulates its demand configuration
 *
 * Arguments: uint64_t *demandConfig - Enumeration of the configuration
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
/*lint -esym(715,demandConfig) not referenced in Aclara_LC  */
/*lint -esym(818,demandConfig) not referenced in Aclara_LC  */
enum_CIM_QualityCode ENDPT_CIM_CMD_getDemandPresentConfig( uint64_t *demandConfig )
{
#if ( ENABLE_HMC_TASKS == 1 )
   DEMAND_getConfig(demandConfig);
   return CIM_QUALCODE_SUCCESS;
#else
   return CIM_QUALCODE_KNOWN_MISSING_READ;
#endif
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getBubbleUpSchedule
 *
 * Purpose: Returns the Bubble-up schedule
 *
 * Arguments: uint64_t *buSchedule - Frequency of sending bubble-up data, in minutes
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
/*lint -esym(715,buSchedule) not referenced in Aclara_LC  */
/*lint -esym(818,buSchedule) not referenced in Aclara_LC  */
enum_CIM_QualityCode ENDPT_CIM_CMD_getBubbleUpSchedule( uint64_t *buSchedule )
{
#if ( ENABLE_HMC_TASKS == 1 )
   *buSchedule = (uint64_t)ID_getLpBuSchedule();
   return CIM_QUALCODE_SUCCESS;
#else
   return CIM_QUALCODE_KNOWN_MISSING_READ;
#endif   /* end of ENABLE_HMC_TASKS  == 1 */
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getTimeZoneDstHash
 *
 * Purpose: Returns the Time Zone / DST hash
 *
 * Arguments: uint64_t *tzDstHash - SHA-256 hash of the time zone offset, DST Enabled, DST Offset,
 *                                  DST Start Rule, and DST End Rulw
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getTimeZoneDstHash( uint64_t *tzDstHash )
{
   uint32_t hash;

   DST_getTimeZoneDSTHash( &hash );
   *tzDstHash = hash;

   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getRealTimeAlarmIndexID
 *
 * Purpose: Returns real time alarm index id
 *
 * Arguments: uint64_t *realTimeIndexId - The current index into the real-time alarm log (pointing to the newest entry)
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getRealTimeAlarmIndexID( uint64_t *realTimeIndexId )
{
   *realTimeIndexId = (uint64_t)EVL_GetRealTimeIndex();
   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getOpportunisticAlarmIndexID
 *
 * Purpose: Returns opportunistic alarm index id
 *
 * Arguments: uint64_t *opportunisticIndexId - The current index into the opportunistic alarm log (pointing to the newest entry)
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getOpportunisticAlarmIndexID( uint64_t *opportunisticIndexId )
{
   *opportunisticIndexId = (uint64_t)EVL_GetNormalIndex();
   return CIM_QUALCODE_SUCCESS;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCapableOfEpPatchDFW
 *
 * Purpose: Returns whether endpoint application updates are supported via dfw process
 *
 * Arguments: uint8_t *epPatchDFWSupported
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfEpPatchDFW( uint8_t *epPatchDFWSupported )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;
   *epPatchDFWSupported = (uint8_t)DFWA_getCapableOfEpPatchDFW();
   return retVal;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCapableOfEpBootloaderDFW
 *
 * Purpose: Returns whether endpoint bootloader updates are supported via dfw process
 *
 * Arguments: uint8_t *epBootloaderDFWSupported
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfEpBootloaderDFW( uint8_t *epBootloaderDFWSupported )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;
   *epBootloaderDFWSupported = (uint8_t)DFWA_getCapableOfEpBootloaderDFW();
   return retVal;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCapableOfMeterPatchDFW
 *
 * Purpose: Returns whether meter patch updates are supported via the dfw process
 *
 * Arguments: uint8_t *meterPatchDFWSupported
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfMeterPatchDFW( uint8_t *meterPatchDFWSupported )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;
   *meterPatchDFWSupported = (uint8_t)DFWA_getCapableOfMeterPatchDFW();
   return retVal;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCapableOfMeterBasecodeDFW
 *
 * Purpose: Returns whether meter basecode updates are supported via the dfw process
 *
 * Arguments: uint8_t *meterBasecodeDFWSupported
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfMeterBasecodeDFW( uint8_t *meterBasecodeDFWSupported )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;
   *meterBasecodeDFWSupported = (uint8_t)DFWA_getCapableOfMeterBasecodeDFW();
   return retVal;
}

/***********************************************************************************************************************
 *
 * Function name: ENDPT_CIM_CMD_getCapableOfMeterReprogrammingOTA
 *
 * Purpose: Returns whether meter reprogramming is supported via the dfw process
 *
 * Arguments: uint8_t *meterReprogrammingOTASupported
 *
 * Returns: enum_CIM_QualityCode SUCCESS/FAIL indication
 *
 * Side effects: N/A
 *
 * Reentrant: Yes
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfMeterReprogrammingOTA( uint8_t *meterReprogrammingOTASupported )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;
   *meterReprogrammingOTASupported = (uint8_t)DFWA_getCapableOfMeterReprogrammingOTA();
   return retVal;
}

/***********************************************************************************************************************

   Function Name: ENDPT_CIM_CMD_OR_PM_Handler

   Purpose: Get parameters related to version.c for over the air read

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
uDateTimeLostCount
   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t ENDPT_CIM_CMD_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case comDeviceFirmwareVersion :     // Fall through case
         case comDeviceBootloaderVersion :   // Fall through case
         case comDeviceHardwareVersion :     // Fall through case
         case comDeviceType :
         {
            uint8_t numberOfBytes = 0; //store length of string returned
            uint8_t strBuffer[STRING_BUFFER_SIZE]; //store string returned
            (void) memset(strBuffer, 0, STRING_BUFFER_SIZE);

            if ( STRING_BUFFER_SIZE <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       (void)ENDPT_CIM_CMD_getStrValue( id, strBuffer, sizeof(strBuffer), &numberOfBytes);
               (void)memcpy((char *)value, strBuffer, numberOfBytes);
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = numberOfBytes;
                  attr->rValTypecast = (uint8_t)ASCIIStringValue;
               }
            }
            break;
         }
         case comDeviceMACAddress :
         {
            if ( MAC_EUI_ADDR_SIZE <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       (void)ENDPT_CIM_CMD_getCommMACID((uint64_t *)value);
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)MAC_EUI_ADDR_SIZE;
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case newRegistrationRequired :
         {
            if ( sizeof(RegistrationInfo.registrationStatus) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
     	       (void)ENDPT_CIM_CMD_getNewRegistrationRequired((uint8_t *)value);
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(RegistrationInfo.registrationStatus);
                  attr->rValTypecast = (uint8_t)Boolean;
               }
            }
            break;
         }
         case comDeviceMicroMPN :
         {
            uint8_t numberOfBytes = 0; //store length of string returned
            uint8_t strBuffer[PARTNUMBER_BUFFER_SIZE + 1]; //store string returned
            (void) memset( strBuffer, 0, PARTNUMBER_BUFFER_SIZE );

            if ( PARTNUMBER_BUFFER_SIZE <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {   //The reading will fit in the buffer
                (void)ENDPT_CIM_CMD_getStrValue( id, strBuffer, sizeof( strBuffer ), &numberOfBytes );
                (void)memcpy( (char *)value, strBuffer, numberOfBytes );
                retVal = eSUCCESS;
                if ( attr != NULL )
                {
                   attr->rValLen = numberOfBytes;
                   attr->rValTypecast = (uint8_t)ASCIIStringValue;
                }
            }
            break;
         }
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
         case newRegistrationRequired :
         {
            if ( *(uint8_t *)value == 1 )            /* Registration required; map to NOT SENT   */
            {
               APP_MSG_ReEnableRegistration();  /* Restart the registration process */
               retVal = eSUCCESS;
            }
            else if ( *(uint8_t *)value == 0 )       /* Registration NOT required; map to CONFIRMED   */
            {
               RegistrationInfo.registrationStatus       = REG_STATUS_CONFIRMED;
               RegistrationInfo.nextRegistrationTimeout  = RegistrationInfo.initialRegistrationTimeout;
               APP_MSG_updateRegistrationTimeout();   // sets active Registration Timeout to a random value
               APP_MSG_UpdateRegistrationFile();
               retVal = eSUCCESS;
            }
            else
            {
               retVal = eAPP_INVALID_VALUE;
            }
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

