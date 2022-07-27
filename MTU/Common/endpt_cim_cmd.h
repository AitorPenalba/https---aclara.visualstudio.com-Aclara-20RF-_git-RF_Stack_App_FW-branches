/***********************************************************************************************************************
 *
 * Filename: endpt_cim_cmd.h
 *
 * Contents: API to access from CIM module to Endpoint
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2004-2015 Aclara.  All Rights Reserved.
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

#ifndef ENDPT_CIM_CMD_H
#define ENDPT_CIM_CMD_H

/* INCLUDE FILES */

#include "project.h"
#include "version.h"
#include "HEEP_util.h"

#ifdef ENDPT_CIM_CMD_GLOBAL
   #define ENDPT_CIM_CMD_EXTERN
#else
   #define ENDPT_CIM_CMD_EXTERN extern
#endif

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

ReadingsValueTypecast ENDPT_CIM_CMD_getDataType( meterReadingType RdgType );
enum_CIM_QualityCode ENDPT_CIM_CMD_getIntValueAndMetadata( meterReadingType RdgType, int64_t *value, uint8_t *powerOf10 );
enum_CIM_QualityCode ENDPT_CIM_CMD_getStrValue( meterReadingType RdgType, uint8_t *valBuff, uint8_t valBuffSize, uint8_t *readingLeng );
enum_CIM_QualityCode ENDPT_CIM_CMD_getBoolValue( meterReadingType RdgType, uint8_t *boolVal );
enum_CIM_QualityCode ENDPT_CIM_CMD_getDateTimeValue( meterReadingType RdgType, uint32_t *dateTimeVal );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommFWVerNum( firmwareVersion_u *fwVer );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommBLVerNum( firmwareVersion_u *fwVer );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommHWVerNum( char *devTypeBuff, uint8_t devTypeBuffSize, uint8_t *devTypeLeng );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCommMACID( uint64_t *macId );
enum_CIM_QualityCode ENDPT_CIM_CMD_getComDeviceType( char *devTypeBuff, uint8_t devTypeBuffSize, uint8_t *devTypeLeng );
enum_CIM_QualityCode ENDPT_CIM_CMD_getComDevicePartNumber( char *devPartNumberBuff, uint8_t devPartNumberBuffSize, uint8_t *devPartNumberLeng );
enum_CIM_QualityCode ENDPT_CIM_CMD_getNewRegistrationRequired( uint8_t *canDisconnect );
enum_CIM_QualityCode ENDPT_CIM_CMD_getInstallationDateTime( uint32_t *installDateTime );
enum_CIM_QualityCode ENDPT_CIM_CMD_getDemandPresentConfig( uint64_t *demandConfig );
enum_CIM_QualityCode ENDPT_CIM_CMD_getBubbleUpSchedule( uint64_t *buSchedule );
enum_CIM_QualityCode ENDPT_CIM_CMD_getTimeZoneDstHash( uint64_t *tzDstHash );
enum_CIM_QualityCode ENDPT_CIM_CMD_getRealTimeAlarmIndexID( uint64_t *realTimeIndexId );
enum_CIM_QualityCode ENDPT_CIM_CMD_getOpportunisticAlarmIndexID( uint64_t *opportunisticIndexId );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfEpPatchDFW( uint8_t *epPatchDFWSupported );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfEpBootloaderDFW( uint8_t *epBootloaderDFWSupported );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfMeterPatchDFW( uint8_t *meterPatchDFWSupported );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfMeterBasecodeDFW( uint8_t *meterBasecodeDFWSupported );
enum_CIM_QualityCode ENDPT_CIM_CMD_getCapableOfMeterReprogrammingOTA( uint8_t *meterReprogrammingOTASupported );

returnStatus_t ENDPT_CIM_CMD_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );

#undef ENDPT_CIM_CMD_EXTERN

#endif
