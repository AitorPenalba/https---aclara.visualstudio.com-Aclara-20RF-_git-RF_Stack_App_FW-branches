/***********************************************************************************************************************

   Filename:   intf_cim_cmd.c

   Global Designator: INTF_CIM_CMD_

   Contents: API to access from CIM module to HMC

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2013-2018 Aclara.  All Rights Reserved.

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

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define INTF_CIM_CMD_GLOBAL
#include "intf_cim_cmd.h"
#undef  INTF_CIM_CMD_GLOBAL

#if ( ANSI_STANDARD_TABLES == 1 )
#include "portable_aclara.h"
#endif
#include "ed_config.h"
#include "hmc_ds.h"
#include "hmc_request.h"
#if ( DEMAND_IN_METER == 0 )
#include "demand.h"
#endif
#include "pwr_task.h"
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
#include "meter_ansi_sig.h"
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "hmc_prg_mtr.h"
#include "os_aclara.h"
#endif
#include "object_list.h"
#include "HMC_seq_id.h"
#include "byteswap.h"
#endif   /* end of ENABLE_HMC_TASKS == 1  */
#include <math.h>
#if (ACLARA_DA == 0)
#include <stdio.h>
#endif
#include "DBG_SerialDebug.h"
#include "cfg_app.h"

#if ( ACLARA_LC == 1 )
#include "ilc_srfn_reg.h"
#endif

#if (ACLARA_DA == 1)
#include "da_srfn_reg.h"
#endif

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
//static OS_SEM_Obj    intfCimSem_;                           /* Sem handle - Create and initialize to invalid */
//static bool          intfCimSemCreated_            = false;
#endif   /* end of ENABLE_HMC_TASKS == 1  */
#if ( DEMAND_IN_METER == 1 )
//static OS_SEM_Obj    intfCimDmdSem_;                        /* Sem handle - Create and initialize to invalid */
//static bool          intfCimDmdSemCreated_         = false;
static OS_SEM_Obj    intfCimDmdRstSem_;                     /* Sem handle - Create and initialize to invalid */
static bool          intfCimDmdRstSemCreated_      = false;
#endif
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
static OS_SEM_Obj    intfCimAnsiReadSem_;                   /* Sem handle - Create and initialize to invalid */
static bool          intfCimAnsiReadSemCreated_    = false;
static OS_SEM_Obj    intfCimAnsiWriteSem_;                   /* Sem handle - Create and initialize to invalid */
static bool          intfCimAnsiWriteSemCreated_    = false;
static OS_SEM_Obj    intfCimAnsiProcSem_;                   /* Sem handle - Create and initialize to invalid */
static bool          intfCimAnsiProcSemCreated_    = false;
static procNumId_t   procNumID;                             /* The procedure # and ID are sent to the host meter */
static uint8_t       procParams[ 10 ];                      /* Worst case table 7 size for i210+c and kV2c  */
static OS_MUTEX_Obj  intfCimAnsiMutex_;
static bool          intfCimAnsiMutexCreated_      = false;
static uint8_t ansiReadBuffer[HMC_MSG_MAX_RX_LEN]; /* static buffer to store HMC request data */
static HMC_REQ_queue_t ansiReadRequest; /* static variable for ansi read HMC request information*/
static HMC_REQ_queue_t ansiWriteRequest; /* static variable for ansi write HMC request information*/
static HMC_REQ_queue_t ansiProcedureRequest; /* static variable for ansi read HMC request information*/
#endif   /* end of ENABLE_HMC_TASKS == 1  */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
#if 0   /* DG: 06/28/21: Not used */
/*lint -esym(751,uomInfo_t, pUOMInfo_t)   */
PACK_BEGIN
typedef struct PACK_MID
{
   meterReadingType     uom;
   meterReadingType     baseUom;
   currentRegTblType_t  type;
   enum_uomDescriptionInfo_t uomDesc;
} uomInfo_t, *pUOMInfo_t;
PACK_END
#endif

#if ( HMC_I210_PLUS_C == 1 )
// This structure is created to ease the process of caluculating the size and offset to retrieve base fw and patch fw
PACK_BEGIN
typedef struct PACK_MID
{
   uint8_t           updateEnabledFlag;
   uint8_t           updateActiveFlag;
   updateConstants_t updateConstants;
}meterPatchInfo_t;   /* User Defined Struct created from Mfg Table #0*/
PACK_END
#endif


// describe function pointers for oddball handlers
typedef enum_CIM_QualityCode (* oddBallFuncHandler )( ValueInfo_t *reading );

// describe each entry into list of oddball request handlers
typedef struct
{
  oddBallFuncHandler   pHandler;  // pointer to handler function
  meterReadingType        rType;  // reading type handled by the handler function, used for searching
} OddBallLookup_HandlersDef;


/* ****************************************************************************************************************** */
/* CONSTANTS */

#if ( ENABLE_HMC_TASKS == 1 )
// oddball direct read handler function lookup table
static const OddBallLookup_HandlersDef OddBall_Handlers[] =
{
   { EDCFG_getFwVerNum,                               edFwVersion },
   { EDCFG_getHwVerNum,                               edHwVersion },
   { EDCFG_getEdProgrammedDateTime,          edProgrammedDateTime },
   { EDCFG_getMeterDateTime,                        meterDateTime },
   { INTF_CIM_CMD_getDemandRstCnt,               demandResetCount },
   { EDCFG_getAvgIndPowerFactor,                 avgIndPowerFactor},
   { EDCFG_getTHDCount,                               highTHDcount},
#if ( REMOTE_DISCONNECT == 1 )
   { INTF_CIM_CMD_getLoadSideVoltageStatus,  energizationLoadSide },
   { INTF_CIM_CMD_getSwitchPositionCount,     switchPositionCount },
#endif
   { INTF_CIM_CMD_getPowerQualityCount,              powerQuality },
   { NULL,                                     invalidReadingType }
};
#endif

#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */

static const tWriteFull tblProcedure_[] =
{
   CMD_TBL_WR_FULL,                                /* Full Table Write - Must be used for procedures. */
   BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE )  /* Procedure Table ID */ /*lint !e572 !e778 */
};

static const tReadPartial tblProcResult_[] =
{
   CMD_TBL_RD_PARTIAL,
   PSEM_PROCEDURE_TBL_RES,
   TBL_PROC_RES_OFFSET,
   BIG_ENDIAN_16BIT( sizeof( tProcResult ) )       /* Length to read */
};

static HMC_PROTO_TableCmd tblGenericProc_[] =      /* Place in RAM so size of procParams can be modified */
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblProcedure_ ), ( uint8_t far * )&tblProcedure_[0] },
   { HMC_PROTO_MEM_RAM, ( uint8_t )sizeof( procNumID ),     ( uint8_t far * )&procNumID        },  /* Proc num and ID */
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( procParams ),    ( uint8_t far * )&procParams[0]    },  /* Proc Parameters */
   { HMC_PROTO_MEM_NULL }  /*lint !e785 too few initializers   */
};

static const HMC_PROTO_TableCmd tblCmdProcRes_[] =
{
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( tblProcResult_ ), ( uint8_t far * )tblProcResult_ },
   { HMC_PROTO_MEM_NULL }  /*lint !e785 too few initializers   */
};

static const HMC_PROTO_Table tblGenericProcedure_[] =
{
   &tblGenericProc_[0],
   &tblCmdProcRes_[0],
   NULL
};

/* Procedure Initiate (ST07) only so HMC_REQ performs the full table write required for procedures.  Need to read the Response
   (ST08) using an HMC_Read.  This allows retries on response until status is COMPLETE or timeout occurs */
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static const HMC_PROTO_Table tblGenericProcedureWrOnly_[] =
{
   &tblGenericProc_[0],
   NULL
};
#endif

#endif   /* end of ENABLE_HMC_TASKS == 1  */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
/*lint -esym(750,INTF_CIM_TIME_OUT_mS,CIM_MAX_DATA_VALUE_LEN,CIM_INTF_PRNT_INFO,CIM_INTF_PRNT_HEX_INFO)  */
/*lint -esym(750,CIM_INTF_PRNT_WARN,CIM_INTF_PRNT_HEX_WARN)  */
/*lint -esym(750,CIM_INTF_PRNT_ERROR,CIM_INTF_PRNT_HEX_ERROR)  */
#define CIM_MAX_DATA_VALUE_LEN   64      /* Maximum length of strings used in /or/pm get and put functions */
#define INTF_CIM_TIME_OUT_mS     ((uint32_t)35000)   /* Time out when waiting for HMC */
#define ENABLE_PRNT_CIM_INTF_INFO 0
#define ENABLE_PRNT_CIM_INTF_WARN 1
#define ENABLE_PRNT_CIM_INTF_ERROR 1

#if ENABLE_PRNT_CIM_INTF_INFO
#define CIM_INTF_PRNT_INFO( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define CIM_INTF_PRNT_HEX_INFO( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define CIM_INTF_PRNT_INFO( a, fmt,... )
#define CIM_INTF_PRNT_HEX_INFO( a, fmt,... )
#endif

#if ENABLE_PRNT_CIM_INTF_WARN
#define CIM_INTF_PRNT_WARN( a, fmt,... )      DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define CIM_INTF_PRNT_HEX_WARN( a, fmt,... )  DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define CIM_INTF_PRNT_WARN( a, fmt,... )
#define CIM_INTF_PRNT_HEX_WARN( a, fmt,... )
#endif

#if ENABLE_PRNT_CIM_INTF_ERROR
#define CIM_INTF_PRNT_ERROR( a, fmt,... )     DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#define CIM_INTF_PRNT_HEX_ERROR( a, fmt,... ) DBG_logPrintHex ( a, fmt, ##__VA_ARGS__ )
#else
#define CIM_INTF_PRNT_ERROR( a, fmt,... )
#define CIM_INTF_PRNT_HEX_ERROR( a, fmt,... )
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
static enum_CIM_QualityCode searchDirectRead( meterReadingType RdgType, pValueInfo_t reading );
static enum_CIM_QualityCode searchHmcUom( meterReadingType RdgType, pValueInfo_t reading );
static enum_CIM_QualityCode searchOddballDirectRead( meterReadingType RdgType, pValueInfo_t reading );
static enum_CIM_QualityCode getSummationReading( const quantDesc_t *qDesc, pValueInfo_t reading );
static enum_CIM_QualityCode getDemandReading( quantDesc_t *qDesc, pValueInfo_t reading );
#endif   /* end of ENABLE_HMC_TASKS == 1  */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getManufacturer

   Purpose: Gets the manufacturer information from the host meter or locally if possible.

   Arguments: uint32_t *pManufacturer

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
/*lint -esym(715,pManufacturer) */
/*lint -esym(818, pManufacturer) */
enum_CIM_QualityCode INTF_CIM_CMD_getManufacturer( uint32_t *pManufacturer )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

#if ( ANSI_STANDARD_TABLES == 1 )
   stdTbl1_t            *pSt1;             /* Points to ST1 table data */
   if ( eSUCCESS == HMC_MTRINFO_getStdTable1( &pSt1 ) )
   {
      *pManufacturer = pSt1->manufacturer;
      Byte_Swap( ( uint8_t* ) pManufacturer, sizeof( pSt1->manufacturer ) );
      retVal = CIM_QUALCODE_SUCCESS;
   }
#endif
   return ( retVal );
}
/*lint +esym(715,pManufacturer) */
/*lint +esym(818, pManufacturer) */


/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getConfigMfr

   Purpose: Gets the DEVICE_CLASS information from the host meter.

   Arguments: uint8_t manufacturer[]

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
/*lint -esym(715,pManufacturer) */
/*lint -esym(818, pManufacturer) */
enum_CIM_QualityCode INTF_CIM_CMD_getConfigMfr( uint8_t *pManufacturer )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

#if ( ANSI_STANDARD_TABLES == 1 )
   stdTbl0_t            *pSt0;             /* Points to ST0 table data */
   if ( eSUCCESS == HMC_MTRINFO_getStdTable0( &pSt0 ) )
   {
      memcpy(pManufacturer, pSt0->manufacturer, EDCFG_ANSI_TBL_OID_LENGTH);
      retVal = CIM_QUALCODE_SUCCESS;
   }
#endif
   return ( retVal );
}
/*lint +esym(715,pManufacturer) */
/*lint +esym(818, pManufacturer) */


/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getEdModel

   Purpose: Gets the ED_MODEL information from the host meter or locally if possible.

   Arguments: uint8_t *pEdModel - as defined by ST1 (8 bytes)

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
/*lint -esym(715,pEdModel )    */
/*lint -esym(818,pEdModel )    */
enum_CIM_QualityCode INTF_CIM_CMD_getEdModel( uint8_t *pEdModel )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

#if ( ANSI_STANDARD_TABLES == 1 )
   stdTbl1_t            *pSt1;                           /* Points to ST1 table data */
   if ( eSUCCESS == HMC_MTRINFO_getStdTable1( &pSt1 ) )  /* Successfully retrieved ST1 data? */
   {
      ( void )memcpy( pEdModel, pSt1->edModel, 8 );
      retVal = CIM_QUALCODE_SUCCESS;
   }
#else
   {
      // Aclara_LC == 1
      // Get the Type/Model as a string
      uint8_t numberOfBytes = 0;
      uint8_t str[31];

      if( INTF_CIM_CMD_getStrValue( edModel, str, sizeof(str), &numberOfBytes ) == CIM_QUALCODE_SUCCESS)
      {
         ( void )memcpy( pEdModel, str, numberOfBytes );
         DBG_printf( "%s\n", str );
      }
   }
#endif
   return retVal;
}
/*lint +esym(715,pEdModel )    */
/*lint +esym(818,pEdModel )    */

#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
#if ( ( HMC_KV == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getMeterProgramInfo

   Purpose: Gets the DT_Last_PROGRAMMED, PROGRAMMER_NAME and NBR_TIMES_PROGRAMMED information from the host meter.

   Arguments: uint8_t *buf - Storage for the above data ((5 + 10) * 2) + 2 bytes

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_getMeterProgramInfo( uint8_t *buf )
{
   uint8_t              value[ sizeof ( meterProgInfo_t ) ];
   enum_CIM_QualityCode retVal;

   retVal = INTF_CIM_CMD_ansiRead( ( void * )&value[0], MFG_TBL_SECURITY_LOG,
                                   ( uint32_t )offsetof( mfgTbl78_t, mtrProgInfo ), sizeof( value ) );
   if ( CIM_QUALCODE_SUCCESS == retVal )
   {
      ( void )memcpy( buf, value, sizeof( value ) );
   }

   return retVal;
}
#endif
#if HMC_I210_PLUS
/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getElectricMetermetrologyReadingsResetOccurred

   Purpose: Gets the "readings in meter have been reset" flag

   Arguments: uint8_t *flags - Storage for the above data 1 byte

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_getElectricMetermetrologyReadingsResetOccurred( uint8_t *flags )
{
   quantitySig_t        *pSig;
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   uint8_t              Value;

   if ( eSUCCESS == OL_Search( OL_eHMC_QUANT_SIG,
                               ( ELEMENT_ID )metrologyReadingsResetStatus,
                               ( void ** )&pSig ) )   /* Does quantity requested exist? */
   {
      retVal = INTF_CIM_CMD_ansiRead( ( void * )&Value, BIG_ENDIAN_16BIT( pSig->tbl.tblInfo.tableID ),
                                      pSig->tbl.tblInfo.offset[2], sizeof( Value ) );

      if ( CIM_QUALCODE_SUCCESS == retVal )
      {
         *flags = Value;
      }
   }

   return retVal;
}
#endif
#endif

/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getPowerQualityCount

   Purpose: Reads power anomaly counts from EP.  This is a blocking function.

   Arguments: ValueInfo_t *readingInfo - Location to store information for the reading

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_getPowerQualityCount( ValueInfo_t *readingInfo )
{
   uint32_t             uValue = 0; // initialize variable to retrieve value
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE; // initialize quality code

   readingInfo->typecast = uintValue; // set the type cast
   readingInfo->valueSizeInBytes = 0; // initialize value size to zero bytes

   if ( eSUCCESS == PWR_getPowerAnomalyCount( &uValue ) )
   {  // we successfully retrieved the value, update information in the reading info structure
      readingInfo->Value.uintValue = ( uint64_t )uValue;
      readingInfo->valueSizeInBytes = sizeof(uValue);
      retVal = CIM_QUALCODE_SUCCESS;
   }

   readingInfo->qCode = retVal; // set the quality code in the reading
   readingInfo->readingType = powerQuality;

   return( retVal );
}

/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getRCDCSwitchPresent

   Purpose: Reads from MFG table 114 in the host meter to see if the RCDC switch is present.  Blocking function.

   Arguments:  uint8_t *pValue - Location to store the quantity
               bool bReread: If true, get the value from meter else use cached value

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
/*lint -esym(715,bReRead)    not referenced - conditional compilation issue */
enum_CIM_QualityCode INTF_CIM_CMD_getRCDCSwitchPresent( uint8_t* pValue, bool bReRead )
{
   enum_CIM_QualityCode retVal; /* Response to this function call */
#if ( REMOTE_DISCONNECT == 1 )
   static bool          bRead = false;
   static uint8_t       uValue = 0;
   ValueInfo_t          reading;

   retVal = CIM_QUALCODE_SUCCESS;
   if ( !bRead || bReRead )
   {
      retVal = INTF_CIM_CMD_getMeterReading ( disconnectCapable, &reading );
      if ( CIM_QUALCODE_SUCCESS == retVal )
      {
         if ( !bRead || ( uValue != ( uint8_t )reading.Value.intValue ) )
         {
            uValue = ( uint8_t )reading.Value.intValue;
            ( void )HMC_DS_Applet( ( uint8_t )( 0 == uValue ? HMC_APP_API_CMD_DISABLE_MODULE  :
                                                              HMC_APP_API_CMD_ENABLE_MODULE ), NULL );
         }
         bRead = true;
      }
   }

   if ( CIM_QUALCODE_SUCCESS == retVal )
   {
      *pValue = uValue;
   }
#else
   retVal = CIM_QUALCODE_SUCCESS;   /*lint !e838 previous value not used - conditional compilation issue */
   *pValue = false;
#endif

   return( retVal );
}
/*lint +esym(715,bReRead)    not referenced - conditional compilation issue */


/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getRCDCSwitchPos

   Purpose: Reads RCDC switch position from MFG table 117 in the host meter.  This is a blocking function.

   Arguments: uint64_t *val - Location to store the quantity

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
#if ( REMOTE_DISCONNECT == 1 )
enum_CIM_QualityCode INTF_CIM_CMD_getRCDCSwitchPos( uint64_t *val )
{
   enum_CIM_QualityCode retVal;
   ValueInfo_t          reading;

   /* Does quantity requested exist? */
   retVal = INTF_CIM_CMD_getMeterReading ( switchPositionStatus, &reading );
   if ( CIM_QUALCODE_SUCCESS == retVal )
   {
      *val = reading.Value.intValue;
   }

   return( retVal );
}
#endif


/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getSwitchPositionCount

   Purpose: Gets the PROGRAM_ID information from the host meter.

   Arguments: uint64_t *val - PROGRAM_ID as defined by MFG Table 67 (2 bytes)

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
#if ( REMOTE_DISCONNECT == 1 )
enum_CIM_QualityCode INTF_CIM_CMD_getSwitchPositionCount( ValueInfo_t *readingInfo )
{
   uint16_t             uValue;
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   retVal = INTF_CIM_CMD_ansiRead( ( void* ) &uValue, MFG_TBL_LOAD_CONTROL_STATUS,
                                   offsetof( ldCtrlStatusTblGPlus_t, rcdc_switch_count ), sizeof(uValue) );

   readingInfo->valueSizeInBytes = sizeof(uValue);
   readingInfo->typecast = uintValue;
   if ( CIM_QUALCODE_SUCCESS == retVal )
   {
      readingInfo->Value.uintValue = ( uint64_t )uValue / 2;
   }
   readingInfo->readingType = switchPositionCount;
   return retVal;
}
#endif

/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getLoadSideVoltageStatus

   Purpose:

   Arguments: uint64_t *val - Location to store the quantity

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
#if ( REMOTE_DISCONNECT == 1 )
enum_CIM_QualityCode INTF_CIM_CMD_getLoadSideVoltageStatus( ValueInfo_t *readingInfo )
{
   RCDCStatus rcdcStatusValue;
   enum_CIM_QualityCode retVal;

   // read the RCDS status bit field
   retVal = INTF_CIM_CMD_ansiReadBitField( (void*)&rcdcStatusValue,
                                           MFG_TBL_RCDC_SWITCH_STATUS,
                                           (uint8_t)0,
                                           (uint8_t)0,
                                           (uint8_t)32 );

   if ( CIM_QUALCODE_SUCCESS == retVal )
   {  // we successfully read the RCDS status table
      if( LINE_LOAD_SIDE_VOLTAGE_PRESENT == rcdcStatusValue.acst )
      {  // There is AC voltage on the line and load side of the disconnect switch.
         if( rcdcStatusValue.exac )
         {  // The load side voltage is non-synchronized external source.
            readingInfo->Value.uintValue = (uint64_t)LOAD_SIDE_VOLTAGE_NOT_SYNCHRONIZED_WITH_LINE_SIDE;
         }
         else
         {  // The load side voltage is synchronized to the line side.
            readingInfo->Value.uintValue = (uint64_t)LOAD_SIDE_VOLTAGE_SYNCHRONIZED_WITH_LINE_SIDE;
         }
      }
      else if( ONLY_LOAD_SIDE_VOLTAGE_PRESENT == rcdcStatusValue.acst )
      {  // Odd case: load side voltage only observed on load side
         readingInfo->Value.uintValue = (uint64_t)LOAD_SIDE_VOLTAGE_COMPARE_NOT_AVAILABLE;
      }
      else
      {  // no voltage observed on the load side
         readingInfo->Value.uintValue = (uint64_t)LOAD_SIDE_VOLTAGE_DEAD;
      }
   }
   else
   {  // There was a problem retrieving data from the meter
      readingInfo->Value.uintValue = (uint64_t)LOAD_SIDE_VOLTAGE_DEAD;
   }

   readingInfo->readingType = energizationLoadSide;
   readingInfo->typecast = uintValue;

   return( retVal );
}
#endif
/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getDemandCfg

   Purpose: Returns Ansi C12.19 definition for the Meter Demand Configuration

   Arguments: uint16_t *pDemandConfig

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
#if ( ANSI_STANDARD_TABLES == 1 )
enum_CIM_QualityCode INTF_CIM_CMD_getDemandCfg( uint16_t *pDemandConfig)
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

#if ( ANSI_STANDARD_TABLES == 1 )
   stdTbl13_t            *pSt13;             /* Points to ST13 table data */
   if ( eSUCCESS == HMC_MTRINFO_getStdTable13( &pSt13 ) )
   {
      memcpy(pDemandConfig, ( uint16_t * )&pSt13->intervalValue[0], sizeof(pSt13->intervalValue[0]));  //All channels configured identically
      retVal = CIM_QUALCODE_SUCCESS;
   }
#endif
   return ( retVal );
}
#endif

/*lint +esym(818,uom,rdgInfo)    */

/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getDemandRstCnt

   Purpose: Reads ST23 DmdRstCnt and potentially ST25 DmdRstTime (converted to days/ticks UTC) from the host meter.  This is a blocking function.

   Arguments: pCimValueInfo_t rdgInfo - Where to return value, time, etc.

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: Yes

 **********************************************************************************************************************/
/*lint -esym(715,rdgInfo)    */
/*lint -esym(818,rdgInfo)    */
enum_CIM_QualityCode INTF_CIM_CMD_getDemandRstCnt( ValueInfo_t *rdgInfo )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ; /* Response to this function call */
   rdgInfo->typecast           = uintValue;
   rdgInfo->readingType        = demandResetCount;
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */

   rdgInfo->cimInfo.coincidentCount = 0;  /* Demand reset count has no coincident values. */
   rdgInfo->cimInfo.timePresent     = 0;  /* Assume no time stamp present. */

#if ( DEMAND_IN_METER == 1 )
   if ( !intfCimDmdRstSemCreated_ ) /* If the semaphore is invalid, create it */
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      intfCimDmdRstSemCreated_ = OS_SEM_Create( &intfCimDmdRstSem_, 0 );
   }

   if ( intfCimDmdRstSemCreated_ ) /* If the semHandle is valid, then try to get data from the host meter */
   {
      quantDesc_t qDesc;                                          /* Contains properties of the quantity requested */

      if ( eSUCCESS == HMC_MTRINFO_demandRstCnt( &qDesc ) )       /* Does the uom requested exist? */
      {
         /* Get demand reset count  */
         uint32_t tblOffset;
         uint16_t tblId;
         uint16_t tblCnt;
         uint8_t  resetCount;

         tblId      = ENDIAN_SWAP_16BIT( qDesc.tblInfo.tableID );
         tblOffset  = qDesc.tblInfo.offset[2];                /* LSB Offset into the table */
         tblOffset |= qDesc.tblInfo.offset[1] << 8;           /* mid Offset into the table */
         tblOffset |= qDesc.tblInfo.offset[0] << 16;          /* MSB Offset into the table */
         tblCnt     = ENDIAN_SWAP_16BIT( qDesc.tblInfo.cnt );

         if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &resetCount, tblId, tblOffset, tblCnt ) )
         {
            retVal                    = CIM_QUALCODE_SUCCESS;
            rdgInfo->valueSizeInBytes = tblCnt;
            rdgInfo->Value.uintValue  = resetCount;

            /* Get demand reset date/time.   */
            if (  ( resetCount != 0 ) &&                                /* If rstcnt == 0, date/time not recorded yet! */
                  ( eSUCCESS == HMC_MTRINFO_demandRstDate( &qDesc ) ) ) /* Does the uom requested exist? */
            {
               MtrTimeFormatHighPrecision_t rstDate;
               tblId      = ENDIAN_SWAP_16BIT( qDesc.tblInfo.tableID );
               tblOffset  = qDesc.tblInfo.offset[2];                /* LSB Offset into the table */
               tblOffset |= qDesc.tblInfo.offset[1] << 8;           /* mid Offset into the table */
               tblOffset |= qDesc.tblInfo.offset[0] << 16;          /* MSB Offset into the table */
               if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &rstDate, tblId, tblOffset, sizeof( rstDate ) ) )
               {
                  sysTime_t sysTime;

                  rstDate.seconds = 0; // Mtr only supports YY MM DD HH MM
                  ( void ) TIME_UTIL_ConvertMtrHighFormatToSysFormat( &rstDate, &sysTime );
                  (void)memcpy( (uint8_t *)&rdgInfo->timeStamp, (uint8_t *)&sysTime, sizeof(sysTime) );
                  rdgInfo->cimInfo.timePresent = 1;
                  retVal = CIM_QUALCODE_SUCCESS;
               }
            }
         }
      }  // if (eSUCCESS == HMC_MTRINFO_getQuantityProp(eQ_FWD, &quantDesc))
   }
#else //else for DEMAND_IN_METER (i.e. Demand is in Module)

   retVal = DEMAND_GetDmdValue( demandResetCount, rdgInfo );   /*lint !e838 previous value not used OK */
#endif
#endif   /* end of ENABLE_HMC_TASKS == 1  */
   return( retVal );
}
/*lint +esym(715,rdgInfo)    */
/*lint +esym(818,rdgInfo)    */
#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_ansiReadBitField

   Purpose: Reads a bit field from an ansi table from the host meter.

   Arguments:  uint32_t *pDest - Location to store the result
               uint16_t id, uint32_t offset, uint16_t lsb, uint16_t width - Table information

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode
INTF_CIM_CMD_ansiReadBitField( uint32_t *pDest, uint16_t id, uint32_t offset, uint16_t lsb, uint16_t width )
{
   uint32_t             uMask = 0;
   uint32_t             uValue;
   uint8_t              bc;         /* Number of bytes to read from table  */
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ; /* Response to this function call */

   if ( ( lsb + width ) <= 32 )
   {
      bc = ( uint8_t )( ( lsb + width ) / 8 );  /* Computer number of bytes to read.   */
      if ( ( ( lsb + width ) % 8 ) != 0 )       /* If not integer number of bytes, add one.  */
      {
         bc++;
      }
      if ( 0 == bc )
      {
         bc++;                /* Read at least one byte  */
      }
      retVal = INTF_CIM_CMD_ansiRead( ( void * ) &uValue, id, offset, bc );

      if ( CIM_QUALCODE_SUCCESS == retVal )
      {
         uMask = ( uint32_t )( ( ( uint64_t )1 << width ) - 1 );
         *pDest = ( uValue >> lsb ) & uMask;
#if 0
         CIM_INTF_PRNT_INFO( 'I',
                              "ansiReadBitField -           tbl: %4d, offset: %2d, lsb: %d, width: %d, value: 0x%08x, "
                              "shifted value 0x%08x, mask: 0x%08x,",
                              id,
                              offset,
                              lsb,
                              width,
                              uValue,
                              uValue >> lsb,
                              uMask );
#endif
      }
   }

   return( retVal );
}

/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_ansiRead

   Purpose: Reads an ansi table

   Arguments:  void *pDest - Location to store the result
               uint16_t id, uint32_t offset, uint16_t cnt - Table information

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_ansiRead( void *pDest, uint16_t id, uint32_t offset, uint16_t cnt )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; /* Response to this function call */

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
    if ( HMC_PRG_MTR_IsSynced( )
#if ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED )
        || HMC_PRG_MTR_IsHoldOff( )
#endif
    ) // RCZ added
    {
        return retVal;
    }
#endif

   if ( !intfCimAnsiMutexCreated_ )       /* If the mutex is invalid, create it */
   {
      intfCimAnsiMutexCreated_ = OS_MUTEX_Create( &intfCimAnsiMutex_ );
   }

   if ( !intfCimAnsiReadSemCreated_ )        /* If the semaphore is invalid, create it */
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      intfCimAnsiReadSemCreated_ = OS_SEM_Create( &intfCimAnsiReadSem_, 0 );
   }

   if( cnt <= (uint16_t)HMC_REQ_MAX_BYTES ) /* are we trying to read more bytes than what is available in the static buffer */
   {

      OS_MUTEX_Lock( &intfCimAnsiMutex_ ); // Function will not return if it fails

      if ( intfCimAnsiReadSemCreated_ ) /* If semHandle is valid, then try to get data from the host meter */
      {
         CIM_INTF_PRNT_INFO( 'I', "Tbl: %d, Offset: %d, len: %d", id, offset, cnt );
         ansiReadRequest.bOperation = eHMC_READ;                      /* Read from the meter */
         ansiReadRequest.tblInfo.id = id;                             /* Table ID */
         ansiReadRequest.tblInfo.offset = offset;                     /* Offset into the table */
         ansiReadRequest.tblInfo.cnt = cnt;                           /* Number of bytes to read */
         ansiReadRequest.maxDataLen = ( uint8_t )cnt;                 /* limit the size to the maximum meter data type */
         ansiReadRequest.pSem = &intfCimAnsiReadSem_;                 /* Semaphore handle */
         ( void )memset( pDest, 0, cnt );                             /*lint !e669 !e419   size passed as parameter. */
         ansiReadRequest.pData = ansiReadBuffer;                      /* Location to store requested data */
         ( void )memset( ansiReadBuffer, 0, sizeof(ansiReadBuffer) ); /* initialize default buffer */

         (void)OS_SEM_Pend( &intfCimAnsiReadSem_, 0 ); /* guard against HMC request posting sem after sem timeout */
         OS_QUEUE_Enqueue( &HMC_REQ_queueHandle, &ansiReadRequest );   /* Start the transmission to the host meter */

         /* Wait for HMC module to process. */
         if ( OS_SEM_Pend( &intfCimAnsiReadSem_, INTF_CIM_TIME_OUT_mS ) )
         {
            if ( eHMC_SUCCESS == ansiReadRequest.hmcStatus )          /* Was the data retrieved successfully? */
            {
               retVal = CIM_QUALCODE_SUCCESS;                     /* Meter was successfully read! */
            }

            ( void )memcpy( pDest, ansiReadBuffer, cnt );      /*lint !e669 !e419   size passed as parameter. */
         }
         else  /* Remove the request and return a "failed" read.  */
         {
            ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle ); // Failsafe - is dequeued in request applet
         }
      }

      OS_MUTEX_Unlock( &intfCimAnsiMutex_ ); // Function will not return if it fails
   }/*lint !e454 !e456 Mutex unlocked, if locked successfully   */
   else /*trying to read to many bytes, send message to debug port */
   {
      DBG_logPrintf( 'R', "HMC read request is %d bytes, max allowed is %d bytes.  Request Ignored.", cnt, HMC_REQ_MAX_BYTES);
   }

   return( retVal ); /*lint !e454 !e456 Mutex unlocked, if locked successfully   */
}  /*lint !e454 !e456 Mutex unlocked, if locked successfully   */


/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_ansiWrite

   Purpose: Writes to an ansi table

   Arguments:  void *pSrc - Location of data to be written
               uint16_t id, uint32_t offset, uint16_t cnt - Table information

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_ansiWrite( void *pSrc, uint16_t id, uint32_t offset, uint16_t cnt )
{
    enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; /* Response to this function call */

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
    if ( HMC_PRG_MTR_IsSynced( )
#if ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED )
        || HMC_PRG_MTR_IsHoldOff( )
#endif
    ) // RCZ added
    {
        return retVal;
    }
#endif

    if ( !intfCimAnsiMutexCreated_ )   /* If the mutex is invalid, create it */
    {
       intfCimAnsiMutexCreated_ = OS_MUTEX_Create( &intfCimAnsiMutex_ );
    }

    if ( !intfCimAnsiWriteSemCreated_ )        /* If the semaphore is invalid, create it */
    {
       //TODO RA6: NRJ: determine if semaphores need to be counting
       intfCimAnsiWriteSemCreated_ = OS_SEM_Create( &intfCimAnsiWriteSem_, 0 );
    }

//   if( cnt <= (uint16_t)HMC_REQ_MAX_BYTES ) /* are we trying to read more bytes than what is available in the static buffer */
//   {

    OS_MUTEX_Lock( &intfCimAnsiMutex_ ); // Function will not return if it fails

      if ( intfCimAnsiWriteSemCreated_ ) /* If semHandle is valid, then try to send data to the host meter */
      {

         /* char category   - Typically 'I','E','R' or 'W' */  CIM_INTF_PRNT_INFO( 'I', "Tbl: %d, Offset: %d, len: %d", id, offset, cnt );

         ansiWriteRequest.bOperation = eHMC_WRITE;       /* Write (partial) to the meter */
         ansiWriteRequest.tblInfo.id = id;               /* Table ID */
         ansiWriteRequest.tblInfo.offset = offset;       /* Offset into the table */
         ansiWriteRequest.tblInfo.cnt = cnt;             /* Number of bytes to write */
         ansiWriteRequest.maxDataLen = ( uint8_t )cnt;   /* limit the size to the maximum meter data type */
         ansiWriteRequest.pSem = &intfCimAnsiWriteSem_;  /* Semaphore handle */
//         ( void )memset( pDest, 0, cnt );                             /*lint !e669 !e419   size passed as parameter. */
         ansiWriteRequest.pData = pSrc;                  /* Location of source data */
//         ( void )memset( ansiWriteBuffer, 0, sizeof(ansiWriteBuffer) ); /* initialize default buffer */

         (void)OS_SEM_Pend( &intfCimAnsiWriteSem_, 0 ); /* guard against HMC request posting sem after sem timeout */
         OS_QUEUE_Enqueue( &HMC_REQ_queueHandle, &ansiWriteRequest );   /* Start the transmission to the host meter */

         /* Wait for HMC module to process. */
         if ( OS_SEM_Pend( &intfCimAnsiWriteSem_, INTF_CIM_TIME_OUT_mS ) )
         {
            if ( eHMC_SUCCESS == ansiWriteRequest.hmcStatus )   /* Was the data written successfully? */
            {
               retVal = CIM_QUALCODE_SUCCESS;                   /* Meter was successfully written! */
            }

            //( void )memcpy( pDest, ansiReadBuffer, cnt );      /*lint !e669 !e419   size passed as parameter. */
         }
         else  /* Remove the request and return a "failed" read.  */
         {
            ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle );
         }
      }

    OS_MUTEX_Unlock( &intfCimAnsiMutex_ ); // Function will not return if it fails

//   }/*lint !e454 !e456 Mutex unlocked, if locked successfully   */
//   else /*trying to read to many bytes, send message to debug port */
//   {
//      DBG_logPrintf( 'R', "HMC read request is %d bytes, max allowed is %d bytes.  Request Ignored.", cnt, HMC_REQ_MAX_BYTES);
//   }

    return( retVal ); /*lint !e454 !e456 Mutex unlocked, if locked successfully   */
}  /*lint !e454 !e456 Mutex unlocked, if locked successfully   */


/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_ansiProcedure

   Purpose: Initiates an ANSI procedure

   Arguments:  void     *pSrc - Location of data to be written to Std. table 7
               uint8_t  proc  - Procedure number
               uint8_t  cnt   - Number of bytes to write

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_ansiProcedure( void const *pSrc, uint16_t proc, uint8_t cnt )
{
    enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; /* Response to this function call */

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
    if ( HMC_PRG_MTR_IsSynced( )
#if ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED )
        || HMC_PRG_MTR_IsHoldOff( )
#endif
    ) // RCZ added
    {
        return retVal;
    }
#endif

   if ( !intfCimAnsiMutexCreated_ )    /* If the mutex is invalid, create it */
   {
      intfCimAnsiMutexCreated_ = OS_MUTEX_Create( &intfCimAnsiMutex_ );
   }

   if ( !intfCimAnsiProcSemCreated_ )  /* If the semaphore is invalid, create it */
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      intfCimAnsiProcSemCreated_ = OS_SEM_Create( &intfCimAnsiProcSem_, 0 );
   }
   OS_MUTEX_Lock( &intfCimAnsiMutex_ ); // Function will not return if it fails

   if ( intfCimAnsiProcSemCreated_ )   /* If semHandle is valid, then try to get data from the host meter */
   {
      procNumID.procNum = proc;                 /* Set proc # */
      procNumID.procId = HMC_getSequenceId();   /* Get unique ID */

      ( void )memcpy( procParams, pSrc, cnt );

      /* Update size of procParams  */
      tblGenericProc_[ 2 ].ucLen = cnt;   /* 3rd element is pointer to params and its length */

      ansiProcedureRequest.bOperation = eHMC_PROCEDURE;               /* Initiate procedure   */
      ansiProcedureRequest.tblInfo.id = procNumID.procNum;            /* Set proc number as table id   */
      ansiProcedureRequest.tblInfo.offset = 0;                        /* Offset is 0 for procedures    */
      ansiProcedureRequest.tblInfo.cnt = cnt;                         /* Number of bytes to write */
      ansiProcedureRequest.maxDataLen = ( uint8_t )cnt;               /* limit the size to the space in the buffer */
      ansiProcedureRequest.pSem = &intfCimAnsiProcSem_;               /* Semaphore handle */
      ansiProcedureRequest.pData = ( void * )tblGenericProcedure_;    /* Pointer to array of table commands  */

      (void)OS_SEM_Pend( &intfCimAnsiProcSem_, 0 ); /* guard against HMC request posting sem after sem timeout */
      OS_QUEUE_Enqueue( &HMC_REQ_queueHandle, &ansiProcedureRequest );   /* Start the transmission to the host meter */

      if ( OS_SEM_Pend( &intfCimAnsiProcSem_, INTF_CIM_TIME_OUT_mS ) ) /* Wait for HMC module to process. */
      {
         if ( eHMC_SUCCESS == ansiProcedureRequest.hmcStatus )                 /* Was the data retrieved successfully? */
         {
            retVal = CIM_QUALCODE_SUCCESS;                     /* Meter was successfully read! */
         }
      }
      else  /* Remove the request and return a "failed" read.  */
      {
         ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle );
      }
   }
   OS_MUTEX_Unlock( &intfCimAnsiMutex_ ); // Function will not return if it fails

   return( retVal ); /*lint !e454 !e456 Mutex unlocked, if locked successfully   */
}  /*lint !e454 !e456 Mutex unlocked, if locked successfully   */

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
/***********************************************************************************************************************

   Function name: HMC_PRG_MTR_ansiRead

   Purpose: Reads an ansi table - this is a modification of INTF_CIM_CMD_ansiRead
            and is used during meter programming

   Arguments:  void *pDest - Location to store the result
               uint16_t id, uint32_t offset, uint16_t cnt - Table information

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode HMC_PRG_MTR_ansiRead( void *pDest, uint16_t id, uint32_t offset, uint16_t cnt ) // RCZ added
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ; /* Response to this function call */

   if ( !intfCimAnsiMutexCreated_ )    /* If the mutex is invalid, create it */
   {
      intfCimAnsiMutexCreated_ = OS_MUTEX_Create( &intfCimAnsiMutex_ );
   }

   if ( !intfCimAnsiReadSemCreated_ )        /* If the semaphore is invalid, create it */
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      intfCimAnsiReadSemCreated_ = OS_SEM_Create( &intfCimAnsiReadSem_, 0 );
   }

   if( cnt <= (uint16_t)HMC_REQ_MAX_BYTES ) /* are we trying to read more bytes than what is available in the static buffer */
   {

      OS_MUTEX_Lock( &intfCimAnsiMutex_ ); // Function will not return if it fails

      if ( intfCimAnsiReadSemCreated_ ) /* If semHandle is valid, then try to get data from the host meter */
      {
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
         HMC_PRG_MTR_ClearNotTestable ( );
#endif
         CIM_INTF_PRNT_INFO( 'I', "Tbl: %d, Offset: %d, len: %d", id, offset, cnt );
         ansiReadRequest.bOperation = eHMC_READ;                      /* Read from the meter */
         ansiReadRequest.tblInfo.id = id;                             /* Table ID */
         ansiReadRequest.tblInfo.offset = offset;                     /* Offset into the table */
         ansiReadRequest.tblInfo.cnt = cnt;                           /* Number of bytes to read */
         ansiReadRequest.maxDataLen = ( uint8_t )cnt;                 /* limit the size to the maximum meter data type */
         ansiReadRequest.pSem = &intfCimAnsiReadSem_;                 /* Semaphore handle */
         ( void )memset( pDest, 0, cnt );                             /*lint !e669 !e419   size passed as parameter. */
         ansiReadRequest.pData = ansiReadBuffer;                      /* Location to store requested data */
         ( void )memset( ansiReadBuffer, 0, sizeof(ansiReadBuffer) ); /* initialize default buffer */

         (void)OS_SEM_Pend( &intfCimAnsiReadSem_, 0 ); /* guard against HMC request posting sem after sem timeout */
         OS_QUEUE_Enqueue( &HMC_REQ_queueHandle, &ansiReadRequest );   /* Start the transmission to the host meter */

         /* Wait for HMC module to process. */
         if ( OS_SEM_Pend( &intfCimAnsiReadSem_, INTF_CIM_TIME_OUT_mS ) ) // 35 seconds
         {
            if ( eHMC_SUCCESS == ansiReadRequest.hmcStatus )          /* Was the data retrieved successfully? */
            {
               retVal = CIM_QUALCODE_SUCCESS;                     /* Meter was successfully read! */
            }
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
            else if ( HMC_PRG_MTR_IsNotTestable( ) )
            {
                retVal = CIM_QUALCODE_NOT_TESTABLE;
            }
#endif
            ( void )memcpy( pDest, ansiReadBuffer, cnt );      /*lint !e669 !e419   size passed as parameter. */
         }
         else  /* Remove the request and return a "failed" read.  */
         {
            ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle );
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
            if ( HMC_PRG_MTR_IsNotTestable( ) )
            {
                retVal = CIM_QUALCODE_NOT_TESTABLE;
            }
#endif
         }
      }

      OS_MUTEX_Unlock( &intfCimAnsiMutex_ ); // Function will not return if it fails

   }/*lint !e454 !e456 Mutex unlocked, if locked successfully   */
   else /*trying to read to many bytes, send message to debug port */
   {
      DBG_logPrintf( 'R', "HMC read request is %d bytes, max allowed is %d bytes.  Request Ignored.", cnt, HMC_REQ_MAX_BYTES);
   }

   return( retVal ); /*lint !e454 !e456 Mutex unlocked, if locked successfully   */
}  /*lint !e454 !e456 Mutex unlocked, if locked successfully   */
#endif

/***********************************************************************************************************************
 *
 * Function Name: INTF_CIM_CMD_errorCodeQCRequired
 *
 * Purpose: Determine whether value for daily a shifted reading is an informational code.
 *
 * Arguments:
 *     meterReadingType rdgType   - shifted reading type
 *     int64_t dsValue            - the value for the shifted reading type
 *
 * Side Effects: none
 *
 * Re-entrant: No
 *
 * Returns: bool - If the value for a specific reading type is an informational code return true, else return false.
 *
 * Notes:
 *
 ***********************************************************************************************************************/
bool INTF_CIM_CMD_errorCodeQCRequired( meterReadingType rdgType, int64_t dsValue)
{
   bool qcRequired = (bool)false;

   switch( rdgType ) /*lint !e788 not all enums used within switch */
   {
      case vRMSC:  // intentional fallthrough
      case vRMSB:  // intentional fallthrough
      case iC:     // intentional fallthrough
      case iB:     // intentional fallthrough
         if( V_I_ANGLE_MEAS_UNSUPPORTED == dsValue )
         {  // unsupported messurement code 0x7fff
            qcRequired = (bool)true;
         }
         break;
      default:
         break;
   }  /*lint !e788 not all enums used within switch */
   return qcRequired;
}

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
/***********************************************************************************************************************

   Function name: HMC_PRG_MTR_ansiWrite

   Purpose: Writes to an ansi table - this is a modification of INTF_CIM_CMD_ansiWrite
            and is used during meter programming

   Arguments:  void *pSrc - Location of data to be written
               uint16_t id, uint32_t offset, uint16_t cnt - Table information

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode HMC_PRG_MTR_ansiWrite( void *pSrc, uint16_t id, uint32_t offset, uint16_t cnt ) // RCZ added
{
    enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; // RCZ chose this; read uses CIM_QUALCODE_KNOWN_MISSING_READ; /* Response to this function call */

    if ( !intfCimAnsiMutexCreated_ )   /* If the mutex is invalid, create it */
    {
       intfCimAnsiMutexCreated_ = OS_MUTEX_Create( &intfCimAnsiMutex_ );
    }

    if ( !intfCimAnsiWriteSemCreated_ )        /* If the semaphore is invalid, create it */
    {
       //TODO RA6: NRJ: determine if semaphores need to be counting
       intfCimAnsiWriteSemCreated_ = OS_SEM_Create( &intfCimAnsiWriteSem_, 0 );
    }

//   if( cnt <= (uint16_t)HMC_REQ_MAX_BYTES ) /* are we trying to read more bytes than what is available in the static buffer */
//   {

    OS_MUTEX_Lock( &intfCimAnsiMutex_ ); // Function will not return if it fails

      if ( intfCimAnsiWriteSemCreated_ ) /* If semHandle is valid, then try to send data to the host meter */
      {

         /* char category   - Typically 'I','E','R' or 'W' */  CIM_INTF_PRNT_INFO( 'I', "Tbl: %d, Offset: %d, len: %d", id, offset, cnt );

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
         HMC_PRG_MTR_ClearNotTestable ( );
#endif

         ansiWriteRequest.bOperation = eHMC_WRITE;       /* Write (partial) to the meter */
         ansiWriteRequest.tblInfo.id = id;               /* Table ID */
         ansiWriteRequest.tblInfo.offset = offset;       /* Offset into the table */
         ansiWriteRequest.tblInfo.cnt = cnt;             /* Number of bytes to write */
         ansiWriteRequest.maxDataLen = ( uint8_t )cnt;   /* limit the size to the maximum meter data type */
         ansiWriteRequest.pSem = &intfCimAnsiWriteSem_;  /* Semaphore handle */
//         ( void )memset( pDest, 0, cnt );                             /*lint !e669 !e419   size passed as parameter. */
         ansiWriteRequest.pData = pSrc;                  /* Location of source data */
//         ( void )memset( ansiWriteBuffer, 0, sizeof(ansiWriteBuffer) ); /* initialize default buffer */

         (void)OS_SEM_Pend( &intfCimAnsiWriteSem_, 0 ); /* guard against HMC request posting sem after sem timeout */
         OS_QUEUE_Enqueue( &HMC_REQ_queueHandle, &ansiWriteRequest );   /* Start the transmission to the host meter */

         /* Wait for HMC module to process. */
         if ( OS_SEM_Pend( &intfCimAnsiWriteSem_, INTF_CIM_TIME_OUT_mS ) )
         {
            if ( eHMC_SUCCESS == ansiWriteRequest.hmcStatus )   /* Was the data written successfully? */
            {
               retVal = CIM_QUALCODE_SUCCESS;                   /* Meter was successfully written! */
            }
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
            else if ( HMC_PRG_MTR_IsNotTestable( ) )
            {
                retVal = CIM_QUALCODE_NOT_TESTABLE;
            }
#endif

            //( void )memcpy( pDest, ansiReadBuffer, cnt );      /*lint !e669 !e419   size passed as parameter. */
         }
         else  /* Remove the request and return a "failed" read.  */
         {
            ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle );
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
            if ( HMC_PRG_MTR_IsNotTestable( ) )
            {
                retVal = CIM_QUALCODE_NOT_TESTABLE;
            }
#endif
         }
      }

    OS_MUTEX_Unlock( &intfCimAnsiMutex_ ); // Function will not return if it fails

//   }/*lint !e454 !e456 Mutex unlocked, if locked successfully   */
//   else /*trying to read to many bytes, send message to debug port */
//   {
//      DBG_logPrintf( 'R', "HMC read request is %d bytes, max allowed is %d bytes.  Request Ignored.", cnt, HMC_REQ_MAX_BYTES);
//   }

    return( retVal ); /*lint !e454 !e456 Mutex unlocked, if locked successfully   */
}  /*lint !e454 !e456 Mutex unlocked, if locked successfully   */


/***********************************************************************************************************************

   Function name: HMC_PRG_MTR_ansiProcedure

   Purpose: Initiates an ANSI procedure
   Purpose: Initiates an ANSI procedure - this is a modification of INTF_CIM_CMD_ansiProcedure
            and is used during meter programming

   Arguments:  void     *pSrc - Location of data to be written to Std. table 7
               uint8_t  proc  - Procedure number
               uint8_t  cnt   - Number of bytes to write

   Returns: enum_CIM_QualityCode SUCCESS/FAIL indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode HMC_PRG_MTR_ansiProcedure( void const *pSrc, uint16_t proc, uint8_t cnt ) // RCZ added
{
    enum_CIM_QualityCode retVal = CIM_QUALCODE_ERROR_CODE; /* Response to this function call */

   if ( !intfCimAnsiMutexCreated_ )    /* If the mutex is invalid, create it */
   {
      intfCimAnsiMutexCreated_ = OS_MUTEX_Create( &intfCimAnsiMutex_ );
   }

   if ( !intfCimAnsiProcSemCreated_ )  /* If the semaphore is invalid, create it */
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      intfCimAnsiProcSemCreated_ = OS_SEM_Create( &intfCimAnsiProcSem_, 0 );
   }
   OS_MUTEX_Lock( &intfCimAnsiMutex_ ); // Function will not return if it fails

   if ( intfCimAnsiProcSemCreated_ )   /* If semHandle is valid, then try to get data from the host meter */
   {
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
      HMC_PRG_MTR_ClearNotTestable ( );
#endif

      procNumID.procNum = proc;                 /* Set proc # */
      procNumID.procId  = HMC_getSequenceId();  /* Get unique ID */

      ( void )memcpy( procParams, pSrc, cnt );  /* Set Proc param */

      /* Let HMC message handler know size of Proc Params  */
      tblGenericProc_[ 2 ].ucLen = cnt;   /* 3rd element is pointer to params and its length */

      ansiProcedureRequest.bOperation     = eHMC_PROCEDURE;                         /* Initiate procedure (FullTableWrite) */
      ansiProcedureRequest.tblInfo.id     = procNumID.procNum;                      /* Set proc number as table id */
      ansiProcedureRequest.tblInfo.offset = 0;                                      /* Offset is 0 for procedures */
      ansiProcedureRequest.tblInfo.cnt    = cnt;                                    /* Number of bytes to write */
      ansiProcedureRequest.maxDataLen     = ( uint8_t )cnt;                         /* limit the size to the space in the buffer */
      ansiProcedureRequest.pSem           = &intfCimAnsiProcSem_;                   /* Semaphore handle */
      ansiProcedureRequest.pData          = ( void * )tblGenericProcedureWrOnly_;   /* Pointer to array of table commands  */

      (void)OS_SEM_Pend( &intfCimAnsiProcSem_, 0 ); /* guard against HMC request posting sem after sem timeout */
      OS_QUEUE_Enqueue( &HMC_REQ_queueHandle, &ansiProcedureRequest );  /* Start the transmission to the host meter */

      if ( OS_SEM_Pend( &intfCimAnsiProcSem_, INTF_CIM_TIME_OUT_mS ) )  /* Wait for HMC module to process. */
      {
         if ( eHMC_SUCCESS == ansiProcedureRequest.hmcStatus )          /* Was the data retrieved successfully? */
         {
            uint8_t            timeout;     /* time in seconds to allow for procedure to respond with COMPLETE */

            timeout = HMC_PRG_MTR_GetProcedureTimeout();
            do
            {
               //Read  ST08
               CIM_INTF_PRNT_INFO( 'I', "Tbl: %d, Offset: 0, len: %d", STD_TBL_PROCEDURE_RESPONSE, sizeof( tProcResult ) );
               ansiReadRequest.bOperation     = eHMC_READ;                          /* Read from the meter */
               ansiReadRequest.tblInfo.id     = STD_TBL_PROCEDURE_RESPONSE;         /* Table ID */
               ansiReadRequest.tblInfo.offset = 0;                                  /* Offset is 0 for procedure responses */
               ansiReadRequest.tblInfo.cnt    = sizeof( tProcResult );              /* Number of bytes to read */
               ansiReadRequest.maxDataLen     = ( uint8_t )sizeof( tProcResult );   /* limit the size to the space in the buffer */
               ansiReadRequest.pSem           = &intfCimAnsiReadSem_;               /* Semaphore handle */
               ansiReadRequest.pData          = ansiReadBuffer;                     /* Location to store requested data */
               ( void )memset( ansiReadBuffer, 0, sizeof(ansiReadBuffer) );         /* initialize default buffer */

               (void)OS_SEM_Pend( &intfCimAnsiReadSem_, 0 ); /* guard against HMC request posting sem after sem timeout */
               OS_QUEUE_Enqueue( &HMC_REQ_queueHandle, &ansiReadRequest );   /* Start the transmission to the host meter */

               /* Wait for HMC module to process. */
               if ( OS_SEM_Pend( &intfCimAnsiReadSem_, INTF_CIM_TIME_OUT_mS ) )
               {
                  if ( eHMC_SUCCESS == ansiReadRequest.hmcStatus )          /* Was the data retrieved successfully? */
                  {
                     tProcResult *pProcRes;

                     //if result not success then what?
                     /* Check response,
                        if status is COMPLETE then SUCCESS and exit loop
                        else if status is NOT_COMPLETE then delay and try again
                        else error in procedure request parameters, exit loop, */
                     pProcRes = ( tProcResult * ) &ansiReadBuffer[0];
                     if ( pProcRes->seqNbr == procNumID.procId) //Is this the response to "my" request?
                     {
                        // if responce code COMPLETE, then done
                        if ( PROC_RESULT_COMPLETED == pProcRes->resultCode )           /* Accepted and Completed  */
                        {
                           retVal  = CIM_QUALCODE_SUCCESS;
                           break;
                        }
                        //else if response code incomplete
                        else if ( PROC_RESULT_NOT_COMPLETED == pProcRes->resultCode )  /* Accepted and Not Completed  */
                        {
                           CIM_INTF_PRNT_WARN('W', "Procedure not yet completed");
                           if ( !timeout )   /* If no timout remaining accept result and try to continue */
                           {
                             CIM_INTF_PRNT_WARN('W', "Procedure timed out, moving to next command");
                              retVal  = CIM_QUALCODE_SUCCESS;
                              break;
                           }
                        }
                        //else all remaining response codes are errors
                        else
                        {
                           break;   // Error, no amount of waiting will change the response.
                        }
                     }
                  }
               }
               else  /* Remove the procedure response request and try again.  */
               {
                  ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle ); // Failsafe - is dequeued in request applet
               }
               /* If process is incomplete OR response not to "my" request OR failed to retrieve data successfully:
                  wait and try again if a time out was specified
               */
               if ( timeout )
               {
                 OS_TASK_Sleep(ONE_SEC);
               }
            }while ( timeout-- );
         }
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
         else if ( HMC_PRG_MTR_IsNotTestable( ) )  /* Procedure execute request failed, set return if it cannot be retried */
         {
            retVal = CIM_QUALCODE_NOT_TESTABLE;
         }
#endif
      }
      else  /* Remove the execute procedure request and return a "failed" read.  */
      {
         ( void ) OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle );
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
         if ( HMC_PRG_MTR_IsNotTestable( ) )
         {
            retVal = CIM_QUALCODE_NOT_TESTABLE;
         }
#endif
      }
   }
   OS_MUTEX_Unlock( &intfCimAnsiMutex_ ); // Function will not return if it fails

   return( retVal ); /*lint !e454 !e456 Mutex unlocked, if locked successfully   */
}  /*lint !e454 !e456 Mutex unlocked, if locked successfully   */

void HMC_PRG_MTR_SyncQueue ( void )
{
    // Enter this function from end of applet list. This stops tasks
    // from beginning any new R/W/Proc requests.
    if ( !intfCimAnsiMutexCreated_ )    /* If the mutex is invalid, create it */
    {
      intfCimAnsiMutexCreated_ = OS_MUTEX_Create( &intfCimAnsiMutex_ );
    }

    // Grab meter comm bus from tasks, which implies any ansi R/W/Proc request
    // in progress has completed.
    OS_MUTEX_Lock( &intfCimAnsiMutex_ ); // Function will not return if fails

    // Empty queue of any remaining task-generated requests, allowing use of
    // queue by meter programming.
    while ( NULL != OS_QUEUE_Dequeue ( &HMC_REQ_queueHandle ) )
    {
    }

    // Free meter comm bus.
    OS_MUTEX_Unlock( &intfCimAnsiMutex_ ); // Function will not return if fails

}
#endif // END of #if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#endif   /* end of ENABLE_HMC_TASKS == 1  */

/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getMeterReading

   Purpose: Retrieve a reading from the meter

   Arguments:  meterReadingType RdgType
               pValueInfo_t reading

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_getMeterReading( meterReadingType RdgType, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_CODE_INDETERMINATE;

#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
   /* Allow this to time out in case the meter password is bad. Allows APP_MSG_HandlerTask to continue running
      and process RF messages.   */
   for ( uint16_t time = 0; time < 50; time++ )   /* Approximately 5 seconds   */
   {
      if ( HMC_MTRINFO_Ready() )
      {
         retVal = CIM_QUALCODE_SUCCESS;
         break;
      }
      OS_TASK_Sleep( 100 );
   }
#endif   /* end of ENABLE_HMC_TASKS == 1  */


   (void)memset( (uint8_t *)reading, 0, sizeof(ValueInfo_t));

#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
   if( CIM_QUALCODE_SUCCESS == retVal )
   {  // HMC is ready to go, attempt to retrieve the reading

      // search for the reading type in the current meter setup UOM lookup
      retVal = searchHmcUom( RdgType, reading);

      if( CIM_QUALCODE_KNOWN_MISSING_READ == retVal )
      {  // No reading type found yet, search direct meter access lookup

         retVal = searchDirectRead( RdgType, reading );

         if( CIM_QUALCODE_KNOWN_MISSING_READ == retVal )
         {  // No reading type found yet, search direct meter access oddball lookup
            retVal = searchOddballDirectRead( RdgType, reading );
         }
      }
   }
#elif (ACLARA_DA == 1)
   retVal = DA_SRFN_REG_getReading( RdgType, reading );
#elif (ACLARA_LC == 1)
   retVal = ILC_SRFN_REG_getReading( RdgType, reading );
#endif

   if ( retVal == CIM_QUALCODE_SUCCESS && ASCIIStringValue == reading->typecast )
   {
      reading->Value.buffer[reading->valueSizeInBytes] = '\0';    // Ensure end of buffer has null terminator
      //Remove any trailing spaces from the return buffer
      while ( ( reading->valueSizeInBytes > 0 ) && ( reading->Value.buffer[( reading->valueSizeInBytes ) - 1] == ' ' ) )
      {
         reading->Value.buffer[--reading->valueSizeInBytes] = '\0';
      }
   }


   return retVal;
}

#if ( ENABLE_HMC_TASKS == 1 ) /* meter specific code */
/***********************************************************************************************************************

   Function name: INTF_CIM_CMD_getDemandCoinc

   Purpose: Retrieve a coinciden reading from the meter

   Arguments:  meterReadingType RdgType
               uint8_t coincNumber
               pValueInfo_t reading

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode INTF_CIM_CMD_getDemandCoinc( meterReadingType demandReadingType, uint8_t coincNumber, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   quantDesc_t qDesc;  // variable to store information to describe and to get the reading

   // read value from meter
   uint64_t localVal = 0;  // variable to store
   uint32_t      offset;   // variable to store offset into table
   uint16_t     tableId;   // variable to strore tableId
   uint16_t      tblCnt;   // variable to store number of bytes to read

   // initialize the variable
   (void)memset((uint8_t *)&qDesc, 0, sizeof(qDesc));

   if( CIM_QUALCODE_SUCCESS == HMC_MTRINFO_searchCoinUom( demandReadingType, coincNumber, &qDesc ) )
   {  // we found a coincident for the requested demand, go get it

     (void)memset((uint8_t *)reading, 0, sizeof(ValueInfo_t)); // initialize the reading data structure

      // byte swap table information into local variables
      tableId = ENDIAN_SWAP_16BIT(qDesc.tblInfo.tableID);
      tblCnt  = ENDIAN_SWAP_16BIT(qDesc.tblInfo.cnt);
      offset  = (uint32_t)( ( qDesc.tblInfo.offset[ 0 ] << 16 ) +
                            ( qDesc.tblInfo.offset[ 1 ] <<  8 ) +
                            ( qDesc.tblInfo.offset[ 2 ] <<  0 ) );

      // retrieve the reading
      retVal = INTF_CIM_CMD_ansiRead( (uint8_t *)&localVal, tableId, offset, tblCnt );

      if( CIM_QUALCODE_SUCCESS == retVal )
      {  // we successfully read data from the meter

         calcSiData_t  result;
         /* Convert to SI units */
         if ( eSUCCESS == HMC_MTRINFO_convertDataToSI( &result, &localVal, &qDesc ) )
         {
            reading->Value.uintValue = result.result;
            reading->power10 = (uint8_t)result.exponent;
            reading->typecast = intValue;
            retVal = CIM_QUALCODE_SUCCESS;
         }
      }

      // get the reading type
      reading->readingType = HMC_MTRINFO_searchUomReadingType( qDesc.tblIdx, &qDesc.uomData );
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: searchHmcUom

   Purpose: Retrieve a HMC UOM reading from the meter

   Arguments:  meterReadingType RdgType
               pValueInfo_t reading

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
static enum_CIM_QualityCode searchHmcUom( meterReadingType RdgType, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   quantDesc_t qDesc;  // variable to store information to describe and to get the reading

   // initialize the variable
   (void)memset((uint8_t *)&qDesc, 0, sizeof(qDesc));

   if( CIM_QUALCODE_SUCCESS == HMC_MTRINFO_searchUom( RdgType, &qDesc ) )
   {  // if we found the reading, go get it

     (void)memset((uint8_t *)reading, 0, sizeof(ValueInfo_t)); // initialize the reading data

      reading->readingType = RdgType;  // set reading type in reading structure
      reading->typecast = intValue;    // set typecast in reading structrure

      if( SUMMATION == qDesc.uomData.measureLocation )
      {  // reading is a summation
         retVal = getSummationReading( &qDesc, reading );
      }
      else
      {  // reading is a demand
         retVal = getDemandReading( &qDesc, reading );
      }
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: getSummationReading

   Purpose: Retrieve a summation reading from the meter

   Arguments:  meterReadingType RdgType
               pValueInfo_t reading

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
static enum_CIM_QualityCode getSummationReading( const quantDesc_t *qDesc, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal;

   uint64_t localVal = 0;  // variable to store
   uint32_t      offset;   // variable to store offset into table
   uint16_t     tableId;   // variable to strore tableId
   uint16_t      tblCnt;   // variable to store number of bytes to read

   // byte swap table information into local variables
   tableId = ENDIAN_SWAP_16BIT(qDesc->tblInfo.tableID);
   tblCnt  = ENDIAN_SWAP_16BIT(qDesc->tblInfo.cnt);
   offset  = (uint32_t)( ( qDesc->tblInfo.offset[ 0 ] << 16 ) +
                         ( qDesc->tblInfo.offset[ 1 ] <<  8 ) +
                         ( qDesc->tblInfo.offset[ 2 ] <<  0 ) );

   // retrieve the reading
   retVal = INTF_CIM_CMD_ansiRead( (uint8_t *)&localVal, tableId, offset, tblCnt );

   if( CIM_QUALCODE_SUCCESS == retVal )
   {  // we successfully read data from the meter

      calcSiData_t  result;
      /* Convert to SI units */
      if ( eSUCCESS == HMC_MTRINFO_convertDataToSI( &result, &localVal, qDesc ) )
      {
         reading->Value.uintValue = result.result;
         reading->power10 = (uint8_t)result.exponent;
         retVal = CIM_QUALCODE_SUCCESS;
      }

      /* Get Shifted Time assosiated with Shifted Summation Reading */
      if ( eSHIFTED_DATA == qDesc->uomData.tableLocation )
      {
         uint16_t tblId;
         uint32_t tblOffset;
         /* NOTE: It was decided not to look at Demand Reset Count, because it might roll over to 0 after 255 resets.
            See ST 21 in R&P guide.  Shifted time is same as the Demand Reset Date/Time. */
         quantDesc_t dmdRstDateTimeDesc;
         if ( eSUCCESS == HMC_MTRINFO_demandRstDate( &dmdRstDateTimeDesc ) )/* Does the meter support time? */
         {
            MtrTimeFormatHighPrecision_t  mtrTime;           /* Time assoc. with Summation value    */

            tblId = ENDIAN_SWAP_16BIT( dmdRstDateTimeDesc.tblInfo.tableID );
            tblOffset = dmdRstDateTimeDesc.tblInfo.offset[2];                 /* LSB Offset into the table */
            tblOffset |= dmdRstDateTimeDesc.tblInfo.offset[1] << 8;           /* mid Offset into the table */
            tblOffset |= dmdRstDateTimeDesc.tblInfo.offset[0] << 16;          /* MSB Offset into the table */

            if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &mtrTime, tblId, tblOffset, sizeof( mtrTime ) ) )
            {
               mtrTime.seconds = 0; /* Only difference between low & high precision is seconds field. */
               ( void ) TIME_UTIL_ConvertMtrHighFormatToSysFormat( &mtrTime, ( sysTime_t * )&reading->timeStamp );
               reading->cimInfo.timePresent = 1;
               retVal = CIM_QUALCODE_SUCCESS;
            }
            else
            {
                  INFO_printf("Failed to Read Time");
            }
         }
      }
   }
   else
   {  // set quality code
      reading->qCode = retVal;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: getDemandReading

   Purpose: Retrieve a demand reading from the meter

   Arguments:  meterReadingType RdgType
               pValueInfo_t reading

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
static enum_CIM_QualityCode getDemandReading( quantDesc_t *qDesc, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;
   uint16_t     offsetOfDemand = 0;   // Location to store the offset of Demand
   uint32_t     offset;               // variable to store offset into table
   uint16_t     tableId;              // variable to strore tableId
   uint16_t     tblCnt;               // variable to store number of bytes to read

   // byte swap table information into local variables
   tableId = ENDIAN_SWAP_16BIT(qDesc->tblInfo.tableID);
   tblCnt  = ENDIAN_SWAP_16BIT(qDesc->tblInfo.cnt);
   offset  = (uint32_t)( ( qDesc->tblInfo.offset[ 0 ] << 16 ) +
                         ( qDesc->tblInfo.offset[ 1 ] <<  8 ) +
                         ( qDesc->tblInfo.offset[ 2 ] <<  0 ) );

   // we are reading an entire demand block, allocate a buffer to store the block
   buffer_t *pBuf;
   pBuf = BM_alloc( tblCnt);

   if( pBuf != NULL )
   {  // we got a buffer, now retrieve the value

      // retrieving the demand block
      retVal = INTF_CIM_CMD_ansiRead( (uint8_t *)pBuf->data, tableId, offset, tblCnt );

      if( CIM_QUALCODE_SUCCESS == retVal )
      {  // we successfully read the demand block

         niFormatEnum_t              niFormat1;   // variable to store niFormat1 data enumeration
         uint8_t                   niDataSize1;   // variable to store niFormat1 data size
         niFormatEnum_t              niFormat2;   // variable to store niFormat2 data enumeration
         uint8_t                   niDataSize2;   // variable to store niFormat2 data size
         stdTbl21_t                     *pST21;   // pointer to access ST21
         MtrTimeFormatHighPrecision_t  mtrTime;   // Time assoc. with demand value
         calcSiData_t                   result;   // Location to store the SI results
         sysTime_t                 convertTime;

         ( void )HMC_MTRINFO_niFormat1( &niFormat1, &niDataSize1 );  // retrieve niFormat1 information
         ( void )HMC_MTRINFO_niFormat2( &niFormat2, &niDataSize2 );  // retrieve niFormat2 information
         ( void )HMC_MTRINFO_getStdTable21( &pST21 );                // retrieve pointer to ST21 for access

         if( pST21->dateTimeFieldFlag ) // date/time exists?
         {
            if ( MAX_DMD == qDesc->uomData.measureLocation )
            {  // yes - populate the date field
               ( void )memcpy( ( uint8_t * )&mtrTime, pBuf->data, sizeof( mtrTime ) );  // Get time assoc. with data
               mtrTime.seconds = 0;   // Only difference between low & high precision is seconds field.
               ( void )TIME_UTIL_ConvertMtrHighFormatToSysFormat( &mtrTime, &convertTime ); // save the time
               (void)memcpy( (uint8_t *)&reading->timeStamp, (uint8_t *)&convertTime, sizeof(sysTime_t) );
               reading->cimInfo.timePresent = 1; // update timestamp flag in reading
            }
            offsetOfDemand += sizeof( MtrTimeFormatLowPrecision_t ); // Adjust offset into demand block for date/time field
         }

         // we need to adjust our offset and value size based upon which demand value we want - max, cum, cont_cum
         if( MAX_DMD == qDesc->uomData.measureLocation )
         {  // we looking for a maximum demand value
            if ( pST21->cumDemandFlag )
            {
               offsetOfDemand += niDataSize1;   // If cumDmd exists, adjust offset
            }
            if ( pST21->contCumDemandFlag )
            {
               offsetOfDemand += niDataSize1;   // If contCumDmd exists, adjust offset field
            }
            qDesc->tblInfo.cnt = niDataSize2;   // adjust reading size for max demand
         }
         else if( CONT_CUM_DEM == qDesc->uomData.measureLocation )
         {  // we are looking for a continuous cumulative demand value
            if ( pST21->cumDemandFlag )
            {
               offsetOfDemand += niDataSize1;   // If cumDmd exists, adjust offset
            }
            qDesc->tblInfo.cnt = niDataSize1;  // adjust reading size for cont cumulative demand
         }
         else if( CUM_DMD == qDesc->uomData.measureLocation )
         {  // we are looking for a cumulative demand value
            qDesc->tblInfo.cnt = niDataSize1;  // adjust reading size for cumulative demand
            retVal = CIM_QUALCODE_SUCCESS;
         }
         else
         {
            retVal = CIM_QUALCODE_ERROR_CODE;
         }

         Byte_Swap( ( uint8_t * )&qDesc->tblInfo.cnt, sizeof( qDesc->tblInfo.cnt ) ); // byteswap for the next step - Convert

         if ( eSUCCESS == HMC_MTRINFO_convertDataToSI( &result, &pBuf->data[offsetOfDemand], qDesc ) )   /*lint !e645 qDesc init'd if retVal == CIM_QUALCODE_SUCCESS    */
         {
            // Copy the result into the reading
            reading->Value.uintValue = result.result;
            reading->power10 = (uint8_t)result.exponent;  // set the exponent in the reading
            retVal = CIM_QUALCODE_SUCCESS;
         }

         reading->cimInfo.coincidentCount = 0;
#if COINCIDENT_SUPPORT
         // we need to figure out how many coincidents this demand has
         stdTbl22_t     *pSt22;
         if ( ( MAX_DMD == qDesc->uomData.measureLocation ) &&
              ( eSUCCESS ==  HMC_MTRINFO_getStdTable22( &pSt22 ) ) )
         {
            // search each assocated coincident index and match with demand index
            for ( uint8_t assocIdx = 0; assocIdx < ST23_COINCIDENTS_CNT; assocIdx++ )
            {
               if ( ( pSt22->coincidentDemandAssoc[ assocIdx ] == qDesc->tblIdx ) &&  // Match the demand index?
                    ( pSt22->coincidentSelection[ assocIdx ] != 0xff ) )              // Check for valid st12 index
               {  // found a coincident
                  reading->cimInfo.coincidentCount++;
                  CIM_INTF_PRNT_INFO( 'I', "supported coin found: demand index: %d, Associate: %d, Count: %d", qDesc->tblIdx, assocIdx, reading->cimInfo.coincidentCount );
               }
            }
         }
#endif
      }
      else
      {  // failed to read demand block
         reading->qCode = retVal;
      }

      BM_free(pBuf);  // free the allocated buffer
   }
   else
   {  //TODO:  re-look at this
      retVal = CIM_QUALCODE_CODE_INDETERMINATE;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: searchDirectRead

   Purpose: Retrieve a meter reading via direct table read

   Arguments:  meterReadingType RdgType
               pValueInfo_t reading

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
static enum_CIM_QualityCode searchDirectRead( meterReadingType RdgType, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   directLookup_t directReadInfo;

   if(  CIM_QUALCODE_SUCCESS == HMC_MTRINFO_searchDirectRead( RdgType, &directReadInfo ) )
   {
      uint32_t offset;  // variable to store table offset
      uint16_t tblLen;  // variable to store number of bytes to read
      uint16_t tablId;  // variable to store the table id

      (void)memset((uint8_t *)reading, 0, sizeof(ValueInfo_t)); // initialize the reading data

      reading->readingType = directReadInfo.quantity;     // set the reading id
      reading->typecast = directReadInfo.typecast;        // set the reading typecast
      reading->power10 = (uint8_t)directReadInfo.tbl.multiplier12; // set the power of ten value

      // set the table read parameters
      tblLen = BIG_ENDIAN_16BIT( directReadInfo.tbl.tblInfo.cnt );
      tablId = BIG_ENDIAN_16BIT( directReadInfo.tbl.tblInfo.tableID );
      offset = (uint32_t)( ( directReadInfo.tbl.tblInfo.offset[ 0 ] << 16 ) +
                           ( directReadInfo.tbl.tblInfo.offset[ 1 ] <<  8 ) +
                           ( directReadInfo.tbl.tblInfo.offset[ 2 ] <<  0 ) );

      reading->valueSizeInBytes = tblLen;


      if( sizeof(reading->Value.buffer) < directReadInfo.tbl.tblInfo.cnt )
      {
         // Is this a bit field?
         if ( directReadInfo.tbl.tblInfo.width != 0 )
         {  // value is being read from a bit field
            retVal = INTF_CIM_CMD_ansiReadBitField( ( uint32_t * )&reading->Value.buffer[ 0 ],
                                                      tablId,
                                                      offset,
                                                      directReadInfo.tbl.tblInfo.lsb,
                                                      directReadInfo.tbl.tblInfo.width );
         }
         else
         {  // just reading bytes from the meter
            retVal = INTF_CIM_CMD_ansiRead( &reading->Value.buffer[0], tablId, offset, tblLen );
         }
      }
      else
      {
         retVal = CIM_QUALCODE_CODE_OVERFLOW;
      }

      // check to see if an error code was returned
      if( INTF_CIM_CMD_errorCodeQCRequired( reading->readingType, reading->Value.intValue ) )
      {
         reading->qCode = CIM_QUALCODE_ERROR_CODE;
         reading->power10 = 0;
      }
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: searchOddballDirectRead

   Purpose: Retrieve a meter reading via direct table read, but results require process the data

   Arguments:  meterReadingType RdgType
               pValueInfo_t reading

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
static enum_CIM_QualityCode searchOddballDirectRead( meterReadingType RdgType, pValueInfo_t reading )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   OddBallLookup_HandlersDef *pHandlers;    // variable to hold pointer to entry of oddball handler list

   // search the oddball reading type list for the requested reading type
   for( pHandlers = (OddBallLookup_HandlersDef *)OddBall_Handlers; pHandlers->pHandler != NULL;  pHandlers++ )
   {
      if (pHandlers->rType == RdgType)
      {  // we found a matching reading type

         retVal = pHandlers->pHandler( reading );  // call the handler function
         reading->qCode = retVal;                  // set the quality code in the reading structure
         break;                                    // we found the handler, break out of loop
      }
   }

   return retVal;
}
#endif   /* end of ENABLE_HMC_TASKS == 1  */

