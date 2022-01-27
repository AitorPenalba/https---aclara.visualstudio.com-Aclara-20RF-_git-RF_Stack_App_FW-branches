/******************************************************************************

   Filename:    hmc_mtr_info.h

   Contents:    #defs, typedefs, and function prototypes for reading table information from the meter.

  **********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2018-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
  **********************************************************************************************************************/

#ifndef hmc_mtr_info_h  /* replace filetemp with this file's name and remove this comment */
#define hmc_mtr_info_h

#if ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
#ifdef HMC_MTRINFO_GLOBAL
#define HMC_MTRINFO_EXTERN
#else
#define HMC_MTRINFO_EXTERN extern
#endif

/* INCLUDE FILES */

#include "ansi_tables.h"
#include "file_io.h"
#include "meter_ansi_sig.h"

/* CONSTANT DEFINITIONS */
#define V_I_ANGLE_MEAS_UNSUPPORTED (uint16_t)0x7fff // value returned from a meter for an unsupported measurement of Voltage, Current, Angle
#define UNPROGRAMMED_INDEX           (uint8_t)0xFF  // summation, demand, coincident index value when not programmed
#define COINCIDENT_PF1_INDEX         (uint8_t)0x3C  // index location for coincident PF #1
#define COINCIDENT_PF2_INDEX         (uint8_t)0x3D  // index location for coincident PF #2

#if ( ANSI_STANDARD_TABLES == 1 )
#define TABLE_15_MODULO 20          // Modular used to index into table 15
#define PF_COINCIDENT_60_MODULO 60  // Modular used to index into table 15 for PF coincident reads
#endif

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

typedef enum
{
   eSTD_TBL,
   eMFG_TBL,
   eSTD_PROC,
   eMFG_PROC,
} tblOrProc_t;

#if ( ANSI_STANDARD_TABLES == 1 )
typedef enum
{
   eMETER_MODE_DEMAND_ONLY,
   eMETER_MODE_DEMAND_LP,
   eMETER_MODE_TOU,
   eMETER_MODE_UNKNOWN = ( uint8_t ) 255,
} meterModes_t;              // Meter Modes


#if ( DEMAND_IN_METER == 1 )
typedef enum
{
   eDEMAND_MODE_UNKNOWN,
   eDEMAND_MODE_DAILY,
   eDEMAND_MODE_REGULAR,      // OR Monthly
} demandModes_t;              // Demand Modes
#endif
#endif //#if ( ANSI_STANDARD_TABLES == 1 )

typedef struct
{
   bool exists;
   bool writeable;
} tblProp_t;

// enumeration to describe unit of measure location in data block - summation, max demand, cum demand, etc
typedef enum
{
   SUMMATION = (uint8_t)0,  // summation measurement
   MAX_DMD,                 // max demand measurement
   CUM_DMD,                 // cumulative demand measurement
   CONT_CUM_DEM,            // continuous cumulative demand measurement
   COINCIDENT               // coincident measurement
}measureLoc_t;

// enumeration to describe which data block in ST23 or ST25 to find a unit of measure value
typedef enum
{
   NON_TIER = (uint8_t)0,  // non tiered data block
   TIER_A,                 // tier A data block
   TIER_B,                 // tier B data block
   TIER_C,                 // tier C data block
   TIER_D,                 // tier D data block
   LAST_TIER = TIER_D      // last available tier block
}blockIdentifier_t;

// structure to hold descriptive information to find a unit of measurement from ST23 or ST25
typedef struct
{
   measureLoc_t      measureLocation;  // location of measurment in data block
   eMtrDataLoc_t     tableLocation;    // Indicates current/shifted data
   blockIdentifier_t dataBlockId;      // Indicates which tier/datablock
}uomData_t;

typedef struct
{
   uint64_t          multiplier15;  /* multiplier as defined in Std Table #15 */
   tableOffsetCnt_t  tblInfo;       /* ANSI Table Id, Offset & Cnt - of the quantity requested */
   int8_t            multiplier12;  /* multiplier as defined in Std Table #12 */
   niFormatEnum_t    dataFmt;       /* Data format as defined by Std Table #0 */
   bool              endian;        /* Data endian definition, defined by Std Table #0 */
   uint8_t           tblIdx;        /* Index into std tbl 23 array (0-4 for kv2c). Can be summation or demand. OR Can be index into MFG Tbl 98 Array (0-1 for I210+C & kv2C)*/
   uomData_t         uomData;       /* descriptive information to adjust offset into metering table */
} quantDesc_t; /* Defines everything needed to know to read a value from the host meter and convert the data */

typedef struct
{
   uint64_t result;
   int8_t   exponent;
} calcSiData_t; /* Calculated data */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */
uint8_t              HMC_MTRINFO_getMeterReadingPowerOfTen( meterReadingType RdgType );
enum_CIM_QualityCode HMC_MTRINFO_searchDirectRead( meterReadingType RdgType, directLookup_t *directLookupData );
meterReadingType     HMC_MTRINFO_searchUomReadingType( uint8_t indexLocation, uomData_t const *uomData);
enum_CIM_QualityCode HMC_MTRINFO_searchUom( meterReadingType RdgType, quantDesc_t *pProperties );
enum_CIM_QualityCode HMC_MTRINFO_searchCoinUom( meterReadingType demandReadingType, uint8_t coincNum, quantDesc_t *pProperties );
void                 printCurrentUomTable(void);
uint8_t              HMC_MTRINFO_app( uint8_t, void * );
returnStatus_t       HMC_MTRINFO_endian( uint8_t *pBigEndian );
returnStatus_t       HMC_MTRINFO_niFormat1( niFormatEnum_t *pFmt, uint8_t *pNumBytes );
returnStatus_t       HMC_MTRINFO_niFormat2( niFormatEnum_t *pFmt, uint8_t *pNumBytes );
returnStatus_t       HMC_MTRINFO_getSizeOfDemands( uint8_t *pDemandsSize, uint8_t *pDemandSize );
returnStatus_t       HMC_MTRINFO_getStdTable0( stdTbl0_t **pStdTable0 );
returnStatus_t       HMC_MTRINFO_getStdTable1( stdTbl1_t **pStdTable1 );
returnStatus_t       HMC_MTRINFO_getStdTable11( stdTbl11_t **pStdTable11 );
returnStatus_t       HMC_MTRINFO_getStdTable13( stdTbl13_t **pStdTable13 );
returnStatus_t       HMC_MTRINFO_getStdTable21( stdTbl21_t **pStdTable21 );
returnStatus_t       HMC_MTRINFO_getStdTable22( stdTbl22_t **pStdTable22 );
uint32_t             HMC_MTRINFO_coinCfgOffset( void );
uint32_t             HMC_MTRINFO_st15Offset( void );
returnStatus_t       HMC_MTRINFO_tableExits( tblOrProc_t tblProc, uint16_t id, tblProp_t *pProperties );
returnStatus_t       HMC_MTRINFO_getQuantityProp( currentRegTblType_t type, meterReadingType uom, quantDesc_t *pProperties );
#if ( LP_IN_METER == 1 )
enum_CIM_QualityCode HMC_MTRINFO_getLPReadingType( stdTbl12_t const *st12Data, uint8_t const *st14Data, uint8_t intervalDuration, meterReadingType *cUom, uint8_t *powerOf10 );
#endif
returnStatus_t       HMC_MTRINFO_convertDataToSI( calcSiData_t *pResult, void const *pData, quantDesc_t const *pProperties );
returnStatus_t       HMC_MTRINFO_demandRstCnt( quantDesc_t *pProperties );
returnStatus_t       HMC_MTRINFO_demandRstDate( quantDesc_t *pProperties );
bool                 HMC_MTRINFO_Ready( void );
FileHandle_t         HMC_MTRINFO_getFileHandle( void );

#if ( ANSI_STANDARD_TABLES == 1 )
meterModes_t         HMC_MTRINFO_getMeterMode( void );
returnStatus_t       HMC_MTRINFO_getSoftswitches( UpgradesBfld_t **upgrades );
#if ( DEMAND_IN_METER == 1 )
demandModes_t        HMC_MTRINFO_GetDemandMode( void );
#endif

#endif /* ANSI_STANDARD_TABLES == 1 */

#endif   /* ACLARA_LC == 0 */
#endif
