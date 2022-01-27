/***********************************************************************************************************************
 
   Filename: intf_cim_cmd.h

   Contents: API to access from CIM module to HMC

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2004-2015 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $log$ KD Created 072513

 ***********************************************************************************************************************
   Revision History:
   072513 KD  - Initial Release

 **********************************************************************************************************************/

#ifndef INTF_CIM_CMD_H
#define INTF_CIM_CMD_H

/* INCLUDE FILES */

#include "time_sys.h"
#include "project.h"
#include "ansi_tables.h"
#include "hmc_mtr_info.h"     /* Contains many tables that have already been read from the host meter. */
#include "hmc_request.h"
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
#include "meter_ansi_sig.h"
#endif   /* end of ENABLE_HMC_TASKS == 1  */

#ifdef INTF_CIM_CMD_GLOBAL
#define INTF_CIM_CMD_EXTERN
#else
#define INTF_CIM_CMD_EXTERN extern
#endif

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t  timePresent       : 1;  /* Time is present in reading */
   uint8_t  coincidentCount   : 2;  /* Number of coincident values available for demand reading */
   uint8_t  filler            : 5;
} cimInfo_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   struct valTimePair
   {
      int64_t     Value;         /* Value of requested reading.   */
      sysTime_t   Time;          /* Time associated with value of requested reading.   */
   } ValTime;
   uint8_t           power10;
   meterReadingType  uom;        /* If coincident request, coincident uom, otherwise copy of requested uom  */
   cimInfo_t         cimInfo;    /* Indicates which fields are populated in response.  */
} CimValueInfo_t, *pCimValueInfo_t;
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   union
   {
      uint8_t      buffer[32];
      int64_t        intValue;
      uint64_t      uintValue;
   }Value;
   uint16_t      valueSizeInBytes;
   sysTime_t            timeStamp;
   uint8_t                power10;
   meterReadingType   readingType;  // If coincident request, coincident uom, otherwise copy of requested uom
   cimInfo_t              cimInfo;  // Indicates which fields are populated in response
   ReadingsValueTypecast typecast;  // Typecast associated with reading type
   enum_CIM_QualityCode     qCode;  // quality code associated with request
} ValueInfo_t, *pValueInfo_t;
PACK_END

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */
enum_CIM_QualityCode INTF_CIM_CMD_getMeterReading( meterReadingType RdgType, pValueInfo_t reading );
enum_CIM_QualityCode INTF_CIM_CMD_getDemandCoinc( meterReadingType demandReadingType, uint8_t coincNumber, pValueInfo_t reading );

bool INTF_CIM_CMD_errorCodeQCRequired( meterReadingType rdgType, int64_t dsValue);
ReadingsValueTypecast INTF_CIM_CMD_getDataType( meterReadingType RdgType );
enum_CIM_QualityCode INTF_CIM_CMD_getIntValueAndMetadata( meterReadingType RdgType, pCimValueInfo_t reading );
enum_CIM_QualityCode INTF_CIM_CMD_getStrValue( meterReadingType RdgType, uint8_t *valBuff, uint8_t valBuffSize,
      uint8_t *readingLeng );
enum_CIM_QualityCode INTF_CIM_CMD_getBoolValue( meterReadingType RdgType, uint8_t *boolVal );
enum_CIM_QualityCode INTF_CIM_CMD_getDateTimeValue( meterReadingType RdgType, uint32_t *dateTimeVal );
enum_CIM_QualityCode INTF_CIM_CMD_getHexBinaryValue( meterReadingType RdgType, uint8_t *reading, uint8_t *readingLeng );
enum_CIM_QualityCode INTF_CIM_CMD_getManufacturer( uint32_t *pManufacturer );
enum_CIM_QualityCode INTF_CIM_CMD_getEdModel( uint8_t *pEdModel );
enum_CIM_QualityCode INTF_CIM_CMD_getProgramID( uint64_t *val );
enum_CIM_QualityCode INTF_CIM_CMD_getHwVerNum( uint8_t *hwVersionNumber, uint8_t *hwRevisionNumber );
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
enum_CIM_QualityCode INTF_CIM_CMD_getFwVerNum( uint8_t *fwVersionNumber, uint8_t *fwRevisionNumber, uint8_t *fwBuildNumber,
                                               uint8_t *patchVersionNumber, uint8_t *patchRevisionNumber, uint8_t *patchBuildNumber );
#elif HMC_I210_PLUS
enum_CIM_QualityCode INTF_CIM_CMD_getFwVerNum( uint8_t *fwVersionNumber, uint8_t *fwRevisionNumber ,
      uint8_t *fwBuildNumber );
#else /* ACLARA_LC   */
enum_CIM_QualityCode INTF_CIM_CMD_getFwVerNum( uint8_t *fwVersionNumber, uint8_t *fwRevisionNumber ,
      uint16_t *fwBuildNumber );
#endif


enum_CIM_QualityCode INTF_CIM_CMD_getConfigMfr( uint8_t *pManufacturer );
enum_CIM_QualityCode INTF_CIM_CMD_getMfgSerNum( uint8_t *pMfgSerialNumber );
enum_CIM_QualityCode INTF_CIM_CMD_getPowerDownCount( uint64_t *val );
enum_CIM_QualityCode INTF_CIM_CMD_getPowerQualityCount( ValueInfo_t *readingInfo );
enum_CIM_QualityCode INTF_CIM_CMD_getRCDCSwitchPresent( uint8_t* pValue, bool bReRead );
enum_CIM_QualityCode INTF_CIM_CMD_getVolts( meterReadingType uom, uint64_t *val );
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
enum_CIM_QualityCode INTF_CIM_CMD_getTemperature( uint64_t *val );
#endif
#if ( REMOTE_DISCONNECT == 1 )
#endif
enum_CIM_QualityCode INTF_CIM_CMD_getSummationTypeUom( meterReadingType uom, currentRegTblType_t * type,
                                                       meterReadingType * baseUom );
enum_CIM_QualityCode INTF_CIM_CMD_getSummationReading( meterReadingType uom, pCimValueInfo_t rdgInfo );
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
enum_CIM_QualityCode INTF_CIM_CMD_ansiReadBitField( uint32_t *pDest, uint16_t id, uint32_t offset, uint16_t lsb,
                                                    uint16_t width );
enum_CIM_QualityCode INTF_CIM_CMD_ansiRead( void *pDest, uint16_t id, uint32_t offset, uint16_t cnt );
enum_CIM_QualityCode INTF_CIM_CMD_ansiWrite( void *pSrc, uint16_t id, uint32_t offset, uint16_t cnt );
enum_CIM_QualityCode INTF_CIM_CMD_ansiProcedure( void const *pSrc, uint16_t proc, uint8_t cnt );
#if ( DEMAND_IN_METER == 1 )
enum_CIM_QualityCode INTF_CIM_CMD_getDemandCfg( uint16_t *pDemandConfig);
enum_CIM_QualityCode INTF_CIM_CMD_getDemandTypeUom( meterReadingType uom, currentRegTblType_t * type,
                                                    enum_uomDescriptionInfo_t *uomDescr, meterReadingType * baseUom );
#endif

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
enum_CIM_QualityCode HMC_PRG_MTR_ansiRead( void *pDest, uint16_t id, uint32_t offset, uint16_t cnt ); // RCZ added
#endif
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
enum_CIM_QualityCode HMC_PRG_MTR_ansiWrite( void *pSrc, uint16_t id, uint32_t offset, uint16_t cnt ); // RCZ added
enum_CIM_QualityCode HMC_PRG_MTR_ansiProcedure( void const *pSrc, uint16_t proc, uint8_t cnt ); // RCZ added
void                 HMC_PRG_MTR_SyncQueue ( void ); // RCZ added
#endif

#endif   /* end of ENABLE_HMC_TASKS == 1  */
enum_CIM_QualityCode INTF_CIM_CMD_getDemandRstCnt( ValueInfo_t *rdgInfo );
enum_CIM_QualityCode INTF_CIM_CMD_getDemandReading( meterReadingType uom, ValueInfo_t *rdgInfo );
enum_CIM_QualityCode INTF_CIM_CMD_getDemandCoincident( meterReadingType uom, uint8_t whichCoin, uint64_t *val,
                                                         uint8_t *power10, meterReadingType *coincidentUom );
enum_CIM_QualityCode INTF_CIM_CMD_getMeterProgramInfo( uint8_t *buf );
enum_CIM_QualityCode INTF_CIM_CMD_getElectricMetermetrologyReadingsResetOccurred( uint8_t *flags );
enum_CIM_QualityCode INTF_CIM_CMD_getRCDCSwitchPos( uint64_t *val );
enum_CIM_QualityCode INTF_CIM_CMD_getLoadSideVoltageStatus( ValueInfo_t *readingInfo );
enum_CIM_QualityCode INTF_CIM_CMD_getSwitchPositionCount( ValueInfo_t *readingInfo );
#undef INTF_CIM_CMD_EXTERN

#endif
