/***********************************************************************************************************************

   Filename: hmc_mtr_info.c

   Global Designator: HMC_MTRINFO_

   Contents: Read Diagnostics from the meter

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
  **********************************************************************************************************************

   $Log$

   Created 03/14/13     KAD  Created

  **********************************************************************************************************************
  **********************************************************************************************************************
   Revision History:
   v1.0 - Initial Release
  *********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#define HMC_MTRINFO_GLOBAL

#include "ansi_tables.h"

#include "object_list.h"
#include "hmc_ds.h"
#define ANSI_SIG_GLOBALS
#include "meter_ansi_sig.h"
#undef ANSI_SIG_GLOBALS

#include "hmc_mtr_info.h"

#undef HMC_MTRINFO_GLOBAL

#include "hmc_prot.h"
#include "byteswap.h"

#if ( ANSI_STANDARD_TABLES == 1 )
#include <limits.h>
#include "intf_cim_cmd.h"
#endif
#if ( DEMAND_IN_METER == 1 )
#include "hmc_time.h"
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#if ( LP_IN_METER == 1 )
#define SIXTY_MIN_INTERVAL   (uint8_t)60  // 60 minite interval data
#define THIRTY_MIN_INTERVAL  (uint8_t)30  // 30 minute interval data
#define FIFTEEN_MIN_INTERVAL (uint8_t)15  // 15 minute interval data
#define TEN_MIN_INTERVAL     (uint8_t)5   //  5 minute interval data
#define ONE_MIN_INTERVAL     (uint8_t)1   //  1 minute interval data
#endif

#define NFS_BIT_SET(x) (bool)((x & 0x80000000) == 0x80000000)  // indicates whether NFS bit is set in ST12 4 byte value
#define UOM_ID_CODE(x) (uint8_t)(x & 0x000000FF)               // returns the ID code of ST12 4 byte value
#define COINCIDENT_PF_ST12_VALUE  (uint32_t)0x8003C419         // coincident PF ST UOM 4 byte value

#if ( ANSI_STANDARD_TABLES == 1 )
#define MTR_INFO_FILE_UPDATE_RATE_SEC  ((uint32_t)24 * 60 * 60)   /* Rarely updated; upon reset */
#define SUM_CFG_IDX_CNT                ((uint8_t)5)
#define DEMAND_CFG_IDX_CNT             ((uint8_t)5)
#define COIN_CFG_IDX_CNT               ((uint8_t)10)
#endif
#define APP_RETRIES                    ((uint8_t)200)             /* Number of times this application will retry */
#define APP_RETRIES_SESSION_ABORT_CNT  ((uint8_t)2)      /* Restart session after x retries, then continue retrying */

/*lint -esym(750,HMC_INFO_PRNT_INFO,HMC_INFO_PRNT_WARN,HMC_INFO_PRNT_ERROR) */
#if ( ENABLE_PRNT_HMC_INFO_INFO || ENABLE_PRNT_HMC_INFO_WARN || ENABLE_PRNT_HMC_INFO_ERROR )
#include "DBG_SerialDebug.h"
#endif
#if ENABLE_PRNT_HMC_INFO_INFO
#define HMC_INFO_PRNT_INFO( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_INFO_PRNT_INFO( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_INFO_WARN
#define HMC_INFO_PRNT_WARN( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_INFO_PRNT_WARN( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_INFO_ERROR
#define HMC_INFO_PRNT_ERROR( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_INFO_PRNT_ERROR( a, fmt,... )
#endif
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uomLookupEntry_t const *    summations[SUM_CFG_IDX_CNT];    // array of pointers to summations programmed in the meter
   uomLookupEntry_t const *    maxDemand[DEMAND_CFG_IDX_CNT];  // array of pointers to maxDemands programmed in the meter
   uomLookupEntry_t const *    cumDemand[DEMAND_CFG_IDX_CNT];  // array of pointers to cumulativeDemands programmed in the meter
   uomLookupEntry_t const *    coincident[COIN_CFG_IDX_CNT];   // array of pointers to coincident readings programmed in the meter
}currentMeterSetup_t;  // data type to represent the current meter UOM setup that has been programmed in the meter

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#if ( ANSI_STANDARD_TABLES == 1 )
static FileHandle_t        mtrInfoFileHndl_ = {0};             /* Contains the file handle information. */
static stdTbl0_t           st0_;                               /* Contains ANSI table #0 information. */
static stdTbl1_t           st1_;                               /* Contains ANSI table #1 information. */
static stdTbl11_t          st11_;                              /* Contains ANSI table #11 information. */
static stdTbl13_t          st13_;                              /* Contains ANSI table #13 information. */
static stdTbl21_t          st21_;                              /* Contains ANSI table #21 information. */
static stdTbl22_t          st22_;                              /* Contains ANSI table #22 information. */
static bool                tablesRead_;                        /* 1 = The standard tables 0, 1, 11, 13 & 22 and MFG Tbl 0 have been read */
static HMC_PROTO_file_t    fileInfo_;                          /* Contains file information for writing in the protocol layer. */
static tReadPartial        tblParams_;                         /* Table parameters to read */
static UpgradesBfld_t      softswitches_;                      /* Upgrades info from MFG Table #0  */
static meterModes_t        meterMode_ = eMETER_MODE_UNKNOWN;   /* Meter Mode info from MFG Table #0 */
#if ( DEMAND_IN_METER == 1 )
static demandModes_t       demandMode_ = eDEMAND_MODE_UNKNOWN; /* Contains Demand Mode */
#endif
#endif

static currentMeterSetup_t currentMeterSetup_ = { 0 }; // local static variable to represent current meter setup

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

#if ( ANSI_STANDARD_TABLES == 1 )
typedef struct
{
   void     *pTable;    /* Pointer to local copy of the meter's ANSI table */
   uint8_t  tableSize;  /* Size of the table */
}tableStatus_t;         /* This table allows easy initialization. */
#endif

/*lint -esym(749,state_t::*) */
typedef enum
{
   eMTR_INFO,        /* Reading the meter information, STD Tables 0, 1, 11, 13 & 22 */
   eDEMAND,          /* Reading the demand, STD Table #13 */
   eSTD_TBL_22_DATA, /* Gets dataSelection_t information for each valid entry in Std Tbl #22 */
   eSTD_TBL_15_DATA, /* Populates standard tbl #15 in NV memory   */
   eSTD_TBL_16_DATA, /* Populates standard tbl #16 in NV memory, final step during HMC_info init and must be last enum before idle state   */
   eIDLE             /* Complete, nothing to do */
}state_t;
static state_t state_;                 /* State of this module */

#if ( ANSI_STANDARD_TABLES == 1 )
typedef struct
{
   stdTbl12_t  sumCfg[SUM_CFG_IDX_CNT];         /* Corresponding Std Tbl 12, 14, 15 & 16 to Std Tbl #22 - Summ Selection */
   stdTbl12_t  demandCfg[DEMAND_CFG_IDX_CNT];   /* Corresponding Std Tbl 12, 14, 15 & 16 to Std Tbl #22 - Demand "       */
   stdTbl12_t  coincidentCfg[COIN_CFG_IDX_CNT]; /* Corresponding Std Tbl 12, 14, 15 & 16 to Std Tbl #22 - Coincident "   */
   stdTbl14_t  stdTbl14;                        /* Std Table 14 */
   stdTbl15_t  stdTbl15[MAX_NBR_CONST_ENTRIES]; /* Std Table 15 */
   stdTbl16_t  stdTbl16[MAX_NBR_SOURCES];       /* Std Table 16 */
}mtrInfoFileData_t;                             /* File for the Meter info module. */

typedef struct
{
   uint16_t fileOffset;      /* Location in the file to store STD Table #12 that corresponds to Std Table #22 index */
   uint8_t  *pStdTbl12Sel;   /* Location in Std Table #12 to place into the file. */
}destScr_t;                  /* Used for populating Std Table #12 Array.  */
#endif
/* ****************************************************************************************************************** */
/* CONSTANTS */

/*lint -e446  Side effect in initializer  */
#if ( ANSI_STANDARD_TABLES == 1 )
static const HMC_PROTO_TableCmd  cmdProto_[] =      /* Table Command for protocol layer. */
{
   { HMC_PROTO_MEM_RAM, sizeof(tblParams_), (uint8_t far *) & tblParams_ },
   { (enum HMC_PROTO_MEM_TYPE)((uint8_t)HMC_PROTO_MEM_FILE | HMC_PROTO_MEM_WRITE), sizeof(fileInfo_), (uint8_t far *) & fileInfo_ },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};

static const HMC_PROTO_Table hmcTbl_[] =
{
   &cmdProto_[0],
   NULL
};
static const tableStatus_t tableStatus_[] =
{
   { &st0_,  sizeof (st0_)},
   { &st1_,  sizeof (st1_)},
   { &st11_, sizeof (st11_)},
   { &st13_, sizeof (st13_)},
   { &st22_, sizeof (st22_)}
};
static const tReadPartial Table_GenCfg1_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_GENERAL_CONFIGURATION),   /* ST.Tbl 0 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },            /* Offset */  /*lint !e651 */
   BIG_ENDIAN_16BIT((sizeof(st0_) / 2))               /* Length */
};

static const tReadPartial Table_GenCfg2_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_GENERAL_CONFIGURATION),        /* ST.Tbl 0 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)sizeof(st0_) / 2 },  /* Offset */  /*lint !e651 */
   BIG_ENDIAN_16BIT((sizeof(st0_) / 2))                 /* Length */
};

/* Read MFG Table 0 for Meter Mode   */

static const tReadPartial Table_MeterMode_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(MFG_TBL_GE_DEVICE_TABLE),                               /* Mfg.Tbl 0 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)offsetof( mfgTbl0_t, MeterMode ) },   /* Offset */  /*lint !e651 */
   BIG_ENDIAN_16BIT( sizeof( uint8_t ) )                                    /* Length */
};

static const HMC_PROTO_TableCmd Cmd_MeterMode[] =
{
   {  HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( Table_MeterMode_ ), ( uint8_t far* )&Table_MeterMode_[0] },
   {  ( enum HMC_PROTO_MEM_TYPE )( ( uint16_t )HMC_PROTO_MEM_RAM | ( uint16_t )HMC_PROTO_MEM_WRITE ), sizeof( uint8_t ), ( uint8_t far* )&meterMode_ },
   {  HMC_PROTO_MEM_NULL } /*lint !e785 too few intitializers  */
};

/* Read MFG Table 0 for softswitches   */

static const tReadPartial Table_Softswitches_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(MFG_TBL_GE_DEVICE_TABLE),                              /* Mfg.Tbl 0 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)offsetof( mfgTbl0_t, upgrades ) },   /* Offset */  /*lint !e651 */
   BIG_ENDIAN_16BIT( sizeof( softswitches_ ) )                             /* Length */
};

static const HMC_PROTO_TableCmd Cmd_Softswitches[] =
{
   {  HMC_PROTO_MEM_PGM, ( uint8_t )sizeof( Table_Softswitches_ ), ( uint8_t far * )&Table_Softswitches_[0] },
   {  ( enum HMC_PROTO_MEM_TYPE )( ( uint16_t )HMC_PROTO_MEM_RAM | ( uint16_t )HMC_PROTO_MEM_WRITE ), sizeof( softswitches_ ), ( uint8_t far * )&softswitches_ },
   {  HMC_PROTO_MEM_NULL } /*lint !e785 too few intitializers  */
};

static const HMC_PROTO_TableCmd Cmd_GenCfg1_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_GenCfg1_), (uint8_t far *)&Table_GenCfg1_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(st0_) / 2, (uint8_t far *)&st0_ },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};

static const HMC_PROTO_TableCmd Cmd_GenCfg2_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_GenCfg2_), (uint8_t far *)&Table_GenCfg2_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(st0_) / 2, (uint8_t far *)&st0_ + (sizeof(st0_) / 2) },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};
static const tReadPartial Table_GenMfgId_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_GENERAL_MANUFACTURE_ID), /* ST.Tbl 1 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },           /* Offset for diag in ST.Tbl 1   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(st1_))                 /* Length for diag in ST.Tbl 1   */
};

static const HMC_PROTO_TableCmd Cmd_GenMfgId_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_GenMfgId_), (uint8_t far *)&Table_GenMfgId_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(st1_), (uint8_t far *)&st1_ },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};
static const tReadPartial Table_ActualSrcLimitingTbl_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_ACTUAL_DATA_SOURCES_LIMITING), /* ST.Tbl 11 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },                 /* Offset for diag in ST.Tbl 11   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(st11_))                      /* Length for diag in ST.Tbl 11   */
};

static const HMC_PROTO_TableCmd Cmd_ActualSrcLimitingTbl_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_ActualSrcLimitingTbl_), (uint8_t far *) & Table_ActualSrcLimitingTbl_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(st11_), (uint8_t *)&st11_ },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};
static const tReadPartial Table_DemandCtrlTbl_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_DEMAND_CONTROL),       /* ST.Tbl 13 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },         /* Offset for diag in ST.Tbl 13   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(st13_))                 /* Length for diag in ST.Tbl 13   */
};

static const HMC_PROTO_TableCmd Cmd_DemandCtrlTbl_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_DemandCtrlTbl_), (uint8_t far *)&Table_DemandCtrlTbl_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(st13_), (uint8_t *)&st13_ },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};
static const tReadPartial Table_DataCtrlTblA_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_DATA_CONTROL),   /* ST.Tbl 14 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },   /* Offset for diag in ST.Tbl 14   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(stdTbl14_t)/3)    /* Length for diag in ST.Tbl 14   */
};

static const HMC_PROTO_file_t protoFileInfoA_ =
{
   &mtrInfoFileHndl_,   (uint16_t)offsetof(mtrInfoFileData_t, stdTbl14)
};

static const HMC_PROTO_TableCmd Cmd_DataCtrlTblA_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_DataCtrlTblA_), (uint8_t far *) & Table_DataCtrlTblA_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_FILE | (uint16_t)HMC_PROTO_MEM_WRITE),
                                  sizeof(stdTbl14_t)/3, (uint8_t *)&protoFileInfoA_},
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};

static const tReadPartial Table_DataCtrlTblB_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_DATA_CONTROL),                        /* ST.Tbl 14 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)(sizeof(stdTbl14_t)/3) },   /* Offset for diag in ST.Tbl 14   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(stdTbl14_t)/3)                         /* Length for diag in ST.Tbl 14   */
};

static const HMC_PROTO_file_t protoFileInfoB_ =
{
   &mtrInfoFileHndl_,   (uint16_t)offsetof(mtrInfoFileData_t, stdTbl14) + (sizeof(stdTbl14_t)/3)
};

static const HMC_PROTO_TableCmd Cmd_DataCtrlTblB_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_DataCtrlTblB_), (uint8_t far *) &Table_DataCtrlTblB_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_FILE | (uint16_t)HMC_PROTO_MEM_WRITE),
                                       sizeof(stdTbl14_t)/3, (uint8_t *)&protoFileInfoB_ },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};

static const tReadPartial Table_DataCtrlTblC_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_DATA_CONTROL),                        /* ST.Tbl 14 */
   { (uint8_t)0, (uint8_t)0, 2*((uint8_t)sizeof(stdTbl14_t)/3) }, /* Offset for diag in ST.Tbl 14   */  /*lint !e651 */
   BIG_ENDIAN_16BIT((sizeof(stdTbl14_t)/3)+1)                     /* Length for diag in ST.Tbl 14   */
};

static const HMC_PROTO_file_t protoFileInfoC_ =
{
   &mtrInfoFileHndl_, (uint16_t)offsetof(mtrInfoFileData_t, stdTbl14) + ( 2 * (sizeof(stdTbl14_t)/3) )
};

static const HMC_PROTO_TableCmd Cmd_DataCtrlTblC_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_DataCtrlTblC_), (uint8_t far *) &Table_DataCtrlTblC_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_FILE | (uint16_t)HMC_PROTO_MEM_WRITE),
                        (sizeof(stdTbl14_t) - ( 2 * (sizeof(stdTbl14_t)/3)+1) ), (uint8_t *)&protoFileInfoC_ },
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};
static const tReadPartial Table_TblDataDimReg_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_ACTUAL_REGISTER),   /* ST.Tbl 21 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },      /* Offset for diag in ST.Tbl 21   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(st21_))              /* Length for diag in ST.Tbl 21   */
};

static const HMC_PROTO_TableCmd Cmd_TblDataDimReg_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_TblDataDimReg_), (uint8_t far *)&Table_TblDataDimReg_[0]},
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(st21_), (uint8_t *)&st21_},
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};
static const tReadPartial Table_TblDataSelectionTbl_[] =
{
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT(STD_TBL_DATA_SELECTION),    /* ST.Tbl 22 */
   { (uint8_t)0, (uint8_t)0, (uint8_t)0 },      /* Offset for diag in ST.Tbl 22   */  /*lint !e651 */
   BIG_ENDIAN_16BIT(sizeof(st22_))              /* Length for diag in ST.Tbl 22   */
};

static const HMC_PROTO_TableCmd Cmd_TblDataSelectionTbl_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_TblDataSelectionTbl_), (uint8_t far *)&Table_TblDataSelectionTbl_[0]},
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(st22_), (uint8_t *)&st22_},
   {HMC_PROTO_MEM_NULL }   /*lint !e785 too few intitializers  */
};
static const HMC_PROTO_Table Tables_MtrInfo_[] =
{
   &Cmd_MeterMode[0],               /* Read Meter Mode from Mfg Table #0 */
   &Cmd_Softswitches[0],            /* Read softswitches from Mfg Table #0 */
   &Cmd_GenCfg1_[0],                /* Read the 1st part of Std Table #0 */
   &Cmd_GenCfg2_[0],                /* Read the 2nd part of Std Table #0 */
   &Cmd_GenMfgId_[0],               /* Read Mfg Id, Std Table #1 */
   &Cmd_ActualSrcLimitingTbl_[0],   /* Read Std Table #11 */
   &Cmd_DataCtrlTblA_[0],           /* Read the 1st part of Std Table #14 */
   &Cmd_DataCtrlTblB_[0],           /* Read the 2nd part of Std Table #14 */
   &Cmd_DataCtrlTblC_[0],           /* Read the 3rd part of Std Table #14 */
   &Cmd_TblDataDimReg_[0],          /* Read Std Table #21 */
   &Cmd_TblDataSelectionTbl_[0],    /* Read Std Table #22 */
   NULL
};
static const HMC_PROTO_Table Tables_DemandCtrl_[] =
{
   &Cmd_DemandCtrlTbl_[0],
   NULL
};
#if ( ANSI_STANDARD_TABLES == 1 )
static const destScr_t destSrcTbl_[] =
{
   { (uint16_t)offsetof(mtrInfoFileData_t, sumCfg[0]),         &st22_.summationSelect[0] },     /* Summation 0 */
   { (uint16_t)offsetof(mtrInfoFileData_t, sumCfg[1]),         &st22_.summationSelect[1] },     /* Summation 1 */
   { (uint16_t)offsetof(mtrInfoFileData_t, sumCfg[2]),         &st22_.summationSelect[2] },     /* Summation 2 */
   { (uint16_t)offsetof(mtrInfoFileData_t, sumCfg[3]),         &st22_.summationSelect[3] },     /* Summation 3 */
   { (uint16_t)offsetof(mtrInfoFileData_t, sumCfg[4]),         &st22_.summationSelect[4] },     /* Summation 4 */
   { (uint16_t)offsetof(mtrInfoFileData_t, demandCfg[0]),      &st22_.demandSelection[0] },     /* Demand Selection 0 */
   { (uint16_t)offsetof(mtrInfoFileData_t, demandCfg[1]),      &st22_.demandSelection[1] },     /* Demand Selection 1 */
   { (uint16_t)offsetof(mtrInfoFileData_t, demandCfg[2]),      &st22_.demandSelection[2] },     /* Demand Selection 2 */
   { (uint16_t)offsetof(mtrInfoFileData_t, demandCfg[3]),      &st22_.demandSelection[3] },     /* Demand Selection 3 */
   { (uint16_t)offsetof(mtrInfoFileData_t, demandCfg[4]),      &st22_.demandSelection[4] },     /* Demand Selection 4 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[0]),  &st22_.coincidentSelection[0] }, /* COINCIDENTS Selection 0 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[1]),  &st22_.coincidentSelection[1] }, /* COINCIDENTS Selection 1 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[2]),  &st22_.coincidentSelection[2] }, /* COINCIDENTS Selection 2 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[3]),  &st22_.coincidentSelection[3] }, /* COINCIDENTS Selection 3 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[4]),  &st22_.coincidentSelection[4] }, /* COINCIDENTS Selection 4 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[5]),  &st22_.coincidentSelection[5] }, /* COINCIDENTS Selection 5 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[6]),  &st22_.coincidentSelection[6] }, /* COINCIDENTS Selection 6 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[7]),  &st22_.coincidentSelection[7] }, /* COINCIDENTS Selection 7 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[8]),  &st22_.coincidentSelection[8] }, /* COINCIDENTS Selection 8 */
   { (uint16_t)offsetof(mtrInfoFileData_t, coincidentCfg[9]),  &st22_.coincidentSelection[9] }  /* COINCIDENTS Selection 9 */
};
#endif
/*lint +e446 */
/* Each index corresponds to the niFormat data length. */
static const uint8_t niFormatBytes[] = { 8, 4, 12, 6, 4, 6, 4, 3, 4, 5, 6, 8, 8, 21, 0, 0 };
/*lint -esym(528,stdTbl12Mult) not referenced   */
//static const int8_t  stdTbl12Mult[] = { 0, 2, 3, 6, 9, -2, -3, -6 };
#endif
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static uint8_t getUomPowerOfTen( uint32_t uom );
static void buildCurrentUomTable(void);
static enum_CIM_QualityCode searchSummations( meterReadingType RdgType, uomData_t *uomData, uint8_t *index, int8_t *powOfTen);
static enum_CIM_QualityCode searchMaxDemands( meterReadingType RdgType, uomData_t *uomData, uint8_t *index, int8_t *powOfTen );
static enum_CIM_QualityCode searchCumDemands( meterReadingType RdgType, uomData_t *uomData, uint8_t *index, int8_t *powOfTen );
enum_CIM_QualityCode searchUomLookupEntry( meterReadingType RdgType, uomLookupEntry_t const * uomEntry, uomData_t *uomData);
enum_CIM_QualityCode searchDataBlock( meterReadingType RdgType, DataBlock_t const * dataBlock, blockIdentifier_t *blockId );
meterReadingType searchDataBlockReadingType( const uomLookupEntry_t *uomLookupPointer, uomData_t *uomData);
enum_CIM_QualityCode searchHmcUomPowerOfTen( meterReadingType RdgType, uint8_t *powerOfTen );
enum_CIM_QualityCode searchDirectReadPowerOfTen( meterReadingType RdgType, uint8_t *powerOfTen );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/* ****************************************************************************************************************** */
/* Main Applet API */
/***********************************************************************************************************************

   Function name: HMC_MTRINFO_app

   Purpose: Reads general information from the meter to populate a local copy of select meter tables.

   Arguments: uint8_t cmd, void *pData

   Returns: uint8_t Results of request

  *********************************************************************************************************************/
/*lint -esym(715,pData) not referenced in i210plus; required for API */
/*lint -esym(818,pData) could be const */
uint8_t HMC_MTRINFO_app(uint8_t cmd, void *pData)
{
   static bool             bQsAddedToOL = false;      /* true = quantitySig has been added to object list */
   static uint8_t          retries_ = APP_RETRIES;    /* Number of times this module will retry */
   static uint8_t          retrySession_ = APP_RETRIES_SESSION_ABORT_CNT;

   uint8_t retVal = (uint8_t) HMC_APP_API_RPLY_IDLE;
#if ( ANSI_STANDARD_TABLES == 1 )
   static uint16_t         destSrcIdx_;
   static uint8_t          index_;
   static bool             bReadMeter_;               /* true = Read from host meter, false = Do nothing. */
   static bool             bPrintStatus_ = true;      /* true = Print status */
#endif

   switch (cmd)
   {
      case HMC_APP_API_CMD_INIT:
      {
#if ( ANSI_STANDARD_TABLES == 1 )
         FileStatus_t fileStatus;                           /* Contains the file status */
#endif

         if ( !bQsAddedToOL )
         {  /* Add the sig to object list */
            (void)OL_Add(OL_eHMC_DIRECT,    sizeof(directLookupTable[0]), &dircectLookupTbl_);
         }
         bQsAddedToOL = true;
#if ( ANSI_STANDARD_TABLES == 1 )
         if ( eSUCCESS !=
               FIO_fopen(&mtrInfoFileHndl_,              /* File Handle so that power quality can access the file. */
                        ePART_MTR_INFO_DATA,             /* Use named partition  */
                        (uint16_t)eFN_MTRINFO,           /* File ID (filename) */
                        (lCnt)sizeof(mtrInfoFileData_t), /* Size of the data in the file. */
                        FILE_IS_NOT_CHECKSUMED,          /* File attributes to use. */
                        MTR_INFO_FILE_UPDATE_RATE_SEC,   /* The update rate of the data in the file. */
                        &fileStatus) )                   /* Contains the file status */
         {
            NOP(); //Todo:  Fail!!!!
         }
         tablesRead_ = false;
         uint8_t i;
         for ( i = 0; i < ARRAY_IDX_CNT(tableStatus_); i++ )
         {
            (void)memset(tableStatus_[i].pTable, 0, tableStatus_[i].tableSize);
         }

         HMC_INFO_PRNT_INFO( 'I', "Size of sumCfg[] = %d", sizeof( stdTbl12_t ) * 5 );
         HMC_INFO_PRNT_INFO( 'I', "Size of demandCfg[] = %d", sizeof( stdTbl12_t ) * 5 );
         HMC_INFO_PRNT_INFO( 'I', "Size of coincidentCfg[] = %d", sizeof( stdTbl12_t ) * 10 );
         HMC_INFO_PRNT_INFO( 'I', "Size of stdTbl14 = %d", sizeof( stdTbl14_t ) );
         HMC_INFO_PRNT_INFO( 'I', "Size of stdTbl15[] = %d", sizeof( stdTbl15_t ) * 25 );
         HMC_INFO_PRNT_INFO( 'I', "Size of stdTbl16[] = %d", sizeof( stdTbl16_t ) * 100 );
         HMC_INFO_PRNT_INFO( 'I', "Size of mtrInfoFileData_t = %d", sizeof( mtrInfoFileData_t ) );
         HMC_INFO_PRNT_INFO( 'I', "Size of demands_t = %d", sizeof( demands_t ) );
#endif
         /* Fall through to Activate */

      }//lint -fallthrough
      case HMC_APP_API_CMD_ACTIVATE:   /* Reads all standard tables. */
      {
         retries_ = APP_RETRIES;
#if ( ANSI_STANDARD_TABLES == 1 )
         bReadMeter_ = true;
         index_ = 0;
         bPrintStatus_ = true;
         destSrcIdx_ = 0;
         while (  (destSrcIdx_ < ARRAY_IDX_CNT(destSrcTbl_)) &&
                  (INVALID_TBL_22_INDEX == *destSrcTbl_[destSrcIdx_].pStdTbl12Sel) )
         {
            destSrcIdx_++;
         }
         state_ = eMTR_INFO;
#else
         state_ = eIDLE;
#endif
         break;
      }
#if ( ANSI_STANDARD_TABLES == 1 )
      case HMC_APP_API_CMD_STATUS:
      {
         if ( bReadMeter_ )
         {
            retVal = (uint8_t) HMC_APP_API_RPLY_READY_COM;
         }
         break;
      }
      case HMC_APP_API_CMD_PROCESS:
      {
         retVal = (uint8_t) HMC_APP_API_RPLY_READY_COM;
         /* Determine what table needs to be read by the state_ variable. */
         switch ( state_ )
         {
            case eMTR_INFO:     /* Read Std Tables pointed to by Tables_MtrInfo_ */
            {
               HMC_INFO_PRNT_INFO('I', "MTR_INFO reading: 0, 1, 11, 14, 21, 22" );
               ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *) Tables_MtrInfo_;
               break;
            }
            case eDEMAND:        /* Read Std Demand Table pointed to by Tables_DemandCtrl_ */
            {
               if (bPrintStatus_)
               {
                  HMC_INFO_PRNT_INFO('I',  "MTR_INFO reading: 13" );
                  bPrintStatus_ = false;
               }
               ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *) Tables_DemandCtrl_;
               break;
            }
            case eSTD_TBL_22_DATA:    /* Gather all data for each item in table #22. */
            {
               uint32_t tableOffset;
               if (bPrintStatus_)
               {
                  HMC_INFO_PRNT_INFO('I',  "MTR_INFO reading: 22" );
                  bPrintStatus_ = false;
               }

               /* Configure ANSI Partial Table Read parameters */
               tableOffset = *destSrcTbl_[destSrcIdx_].pStdTbl12Sel * sizeof(stdTbl12_t);
               (void)memcpy(&tblParams_.ucOffset[0], ( uint8_t * )&tableOffset, sizeof(tblParams_.ucOffset));
               Byte_Swap(&tblParams_.ucOffset[0], sizeof(tblParams_.ucOffset));
               tblParams_.ucServiceCode = CMD_TBL_RD_PARTIAL;
               tblParams_.uiTbleID = BIG_ENDIAN_16BIT(STD_TBL_UNITS_OF_MEASURE_ENTRY);
               tblParams_.uiCount = BIG_ENDIAN_16BIT(sizeof(stdTbl12_t));

               /* Set up the location to write the data after the data is read. */
               fileInfo_.pFileHandle = &mtrInfoFileHndl_;
               fileInfo_.offset = destSrcTbl_[destSrcIdx_].fileOffset;

               ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *) & hmcTbl_[0];
               break;
            }
            case eSTD_TBL_15_DATA:    /* Gather all data for Std Table #15. */
            {
               if (bPrintStatus_)
               {
                  HMC_INFO_PRNT_INFO('I',  "MTR_INFO reading: 15" );
                  bPrintStatus_ = false;
               }
               /* Configure ANSI Partial Table Read parameters */
               uint32_t   tableOffset =  index_ * sizeof(stdTbl15_t);
               (void)memcpy(&tblParams_.ucOffset[0], ( uint8_t * )&tableOffset, sizeof(tblParams_.ucOffset));
               Byte_Swap(&tblParams_.ucOffset[0], sizeof(tblParams_.ucOffset));
               tblParams_.ucServiceCode = CMD_TBL_RD_PARTIAL;
               tblParams_.uiTbleID = BIG_ENDIAN_16BIT(STD_TBL_CONSTANTS);
               tblParams_.uiCount = BIG_ENDIAN_16BIT(sizeof(stdTbl15_t));

               /* Set up the location to write the data after the data is read. */
               fileInfo_.pFileHandle = &mtrInfoFileHndl_;
               fileInfo_.offset = (uint16_t)offsetof(mtrInfoFileData_t, stdTbl15) + (index_ * sizeof(stdTbl15_t));

               ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *) & hmcTbl_[0];
               break;
            }
            case eSTD_TBL_16_DATA:    /* Gather all data for Std Table #16.  This must be the final state of the state machine befor transitioning to idle. */
            {
               if (bPrintStatus_)
               {
                  HMC_INFO_PRNT_INFO('I',  "MTR_INFO reading: 16" );
                  bPrintStatus_ = false;
               }
               /* Configure ANSI Partial Table Read parameters */
               uint32_t   tableOffset =  index_;
               (void)memcpy(&tblParams_.ucOffset[0], ( uint8_t * )&tableOffset, sizeof(tblParams_.ucOffset));
               Byte_Swap(&tblParams_.ucOffset[0], sizeof(tblParams_.ucOffset));
               tblParams_.ucServiceCode = CMD_TBL_RD_PARTIAL;
               tblParams_.uiTbleID = BIG_ENDIAN_16BIT(STD_TBL_SOURCE_DEFINTION);
               tblParams_.uiCount = BIG_ENDIAN_16BIT(sizeof(stdTbl16_t));

               /* Set up the location to write the data after the data is read. */
               fileInfo_.pFileHandle = &mtrInfoFileHndl_;
               fileInfo_.offset = (uint16_t)offsetof(mtrInfoFileData_t, stdTbl16) + (index_ * sizeof(stdTbl16_t));

               ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *) & hmcTbl_[0];
               break;
            }
            case eIDLE:    /* Nothing to do */
            {
               state_ = eMTR_INFO;
               break;
            }
            default: /* Error Condition, start over */
            {
               state_ = eMTR_INFO;
               ((HMC_COM_INFO *) pData)->pCommandTable = (uint8_t far *) Tables_MtrInfo_;
            }
         }
         break;
      }
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {
         retries_ = APP_RETRIES;
         switch ( state_ )
         {
            case eMTR_INFO:
            {
               bPrintStatus_ = true;
               state_++;   /*lint !e641 implicit conversion of enum to int   */

               /* Validate the next table exists, Demand */
               tblProp_t tblProp;
               (void)HMC_MTRINFO_tableExits(eSTD_TBL, STD_TBL_CONSTANTS, &tblProp);  /* Get Demand Table Properties */
               if ( !tblProp.exists )  /* Demand Tbl Exists? */
               {
                  state_++;   /* Don't get the demand table, it doesn't exist */ /*lint !e641 implicit conversion of enum to int   */
                  bPrintStatus_ = true;
               }
               break;
            }
            case eDEMAND:
            {
               state_++;   /*lint !e641 implicit conversion of enum to int   */
               bPrintStatus_ = true;
               break;
            }
            case eSTD_TBL_22_DATA:
            {
               do
               {
                  destSrcIdx_++;
               }while(  (destSrcIdx_ < ARRAY_IDX_CNT(destSrcTbl_)) &&
                        (INVALID_TBL_22_INDEX == *destSrcTbl_[destSrcIdx_].pStdTbl12Sel) );

               if ( destSrcIdx_ >= ARRAY_IDX_CNT(destSrcTbl_) )
               {
                  destSrcIdx_ = 0;
                  state_++;   /*lint !e641 implicit conversion of enum to int   */
                  bPrintStatus_ = true;
                  index_ = 0;
               }
               break;
            }
            case eSTD_TBL_15_DATA:    /* Gather all data for Std Table #15. */
            {
               index_++;
               if ( (index_ >= MAX_NBR_CONST_ENTRIES) || (index_ >= st11_.nbrConstantsEntries) )
               {
                  state_++;   /*lint !e641 implicit conversion of enum to int   */
                  bPrintStatus_ = true;
                  index_ = 0;
               }
               break;
            }
            case eSTD_TBL_16_DATA:    /* Gather all data for Std Table #16. */
            {
               index_++;
               if ( (index_ >= MAX_NBR_SOURCES) || (index_ >= st11_.nbrSources) )
               {
                  tablesRead_ = true;
                  state_++;   /*lint !e641 implicit conversion of enum to int   */
                  bPrintStatus_ = true;
                  bReadMeter_ = false;
                  index_ = 0;

                  // All meter info has been gathered, build current UOM configuration table
                  buildCurrentUomTable();
               }
               break;
            }
            case eIDLE:
            {
               break;
            }
            default:
            {
               state_ = eMTR_INFO;  /* Don't know how we got here, but what else is corrupt?  Recollect all data */
            }
         }
         if ( bReadMeter_ )
         {
            retVal = (uint8_t) HMC_APP_API_RPLY_READY_COM;
         }
         break;
      }
#endif
      case HMC_APP_API_CMD_ABORT:
      {
         if ( 0 != retries_ )
         {
            retries_--;
         }
         retVal = (uint8_t)HMC_APP_API_RPLY_ABORT_SESSION;
         HMC_INFO_PRNT_WARN('W',  "MTR_INFO - Retrying" );
         break;
      }

      case HMC_APP_API_CMD_TBL_ERROR:
      {
         /* This app will continue trying. Proper meter data reporting is not possible until this app completes successfully.
            Failures may be due to an incorrect password, so allow this to keep trying so that if the password is corrected,
            this app will eventually complete.  */
         if ( 0 == --retrySession_ )   /* Restart Session? */
         { /* Yes - Abort current session and retry */
            retrySession_ = APP_RETRIES_SESSION_ABORT_CNT;
            retVal = (uint8_t)HMC_APP_API_RPLY_ABORT_SESSION;
            HMC_INFO_PRNT_WARN('W',  "MTR_INFO - Retrying" );
         }
         break;
      }
      case HMC_APP_API_CMD_ERROR: /* File error can get us to this point.  If there is a file error, no recovery */
      {
         HMC_INFO_PRNT_ERROR('E', "MTR_INFO command error");
         state_ = eIDLE;
#if ( ANSI_STANDARD_TABLES == 1 )
         bReadMeter_ = false;
#endif
         break;
      }
      default: // No other commands are supported in this applet
      {
         retVal = (uint8_t) HMC_APP_API_RPLY_UNSUPPORTED_CMD;
         break;
      }
   }
   return (retVal);
}
/*lint +esym(715,pData) not referenced in i210plus; required for API */
/*lint +esym(818,pData) could be const */
/***********************************************************************************************************************

   Function name: HMC_MTRINFO_Ready

   Purpose: Let external routine know if basic metering information has been gathered.

   Arguments: None

   Returns: true -> meter information gathered; false -> still in progress

  *********************************************************************************************************************/
bool HMC_MTRINFO_Ready( void )
{
#if ( ENABLE_HMC_TASKS == 1 )
   if ( state_ != eIDLE )
   {
      NOP();
   }
   return ( state_ == eIDLE );
#else
   return ( bool )true;
#endif
}
/***********************************************************************************************************************

   Function name: HMC_MTRINFO_GetDemandMode

   Purpose: To determine the Demand Mode. Reads MT 97. This is a blocking function.

   Arguments: None

   Returns: demandModes_t Demand Mode indication

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
#if ( ANSI_STANDARD_TABLES == 1 )
#if ( DEMAND_IN_METER == 1 )
demandModes_t HMC_MTRINFO_GetDemandMode( void )
{
   meterModes_t   meterMode;  /* Meter mode info from meter    */

   // Get the Meter Mode
   meterMode = HMC_MTRINFO_getMeterMode();
   // If Meter Mode is read already
   if( eMETER_MODE_UNKNOWN != meterMode )
   {
      // If unknown, Read MFG TBL 97 and Determine the Demand Mode
      if( demandMode_ == eDEMAND_MODE_UNKNOWN )
      {
         demandMode_ = eDEMAND_MODE_REGULAR; // Update the state
         // Daily Demand Mode is available when meter is in TOU mode
         if( ( eMETER_MODE_TOU == meterMode  ) )
         {
            MT97configFlags_t mfgTbl97configFlags;
            (void)memset( &mfgTbl97configFlags, 0, sizeof( MT97configFlags_t ) );
            if( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_ansiRead( &mfgTbl97configFlags, MFG_TBL_CYCLE_INSENSITIVE_DMD_CONFIG, 0 , sizeof( MT97configFlags_t )) )
            {
               if( mfgTbl97configFlags.demandEnable == 1 )
               {
                  demandMode_ = eDEMAND_MODE_DAILY; // Daily Demand mode is active
               }
            }
         }
      }
   }
#if 0 // For Test
   demandMode_ = eDEMAND_MODE_REGULAR;
   INFO_printf( " Demand mode retrned = %d", demandMode_);
#endif
   HMC_INFO_PRNT_INFO( 'I', " Demand Mode is %d",  (uint8_t)demandMode_ );

   return( demandMode_ );
}
#endif

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getFileHandle

   Purpose: Return mtrInfoFileHndl_ handle.

   Arguments: None

   Returns: mtrInfoFileHndl_

  *********************************************************************************************************************/
FileHandle_t HMC_MTRINFO_getFileHandle( void )
{
   return ( mtrInfoFileHndl_ );
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_st15Offset

   Purpose: Returns offset of stdTbl15 in the Meter Info Meta Data file

   Arguments: none

   Returns: Returns offset of stdTbl15 in the Meter Info Meta Data file

  *********************************************************************************************************************/
uint32_t HMC_MTRINFO_st15Offset( void )
{
   return  ( uint32_t )offsetof( mtrInfoFileData_t, stdTbl15[0] );
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_coinCfgOffset

   Purpose: Returns offset of coincidentCfg in the Meter Info Meta Data file

   Arguments: none

   Returns: Offset in bytes of coincidentCfg in the Meter Info Meta Data file

  *********************************************************************************************************************/
uint32_t HMC_MTRINFO_coinCfgOffset( void )
{
   return  ( uint32_t )offsetof( mtrInfoFileData_t, coincidentCfg[0] );
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_endian

   Purpose: Returns the data format of the host meter.

   Arguments: uint8_t *pBigEndian - Location to store the endianess.  0 = Little Endian, 1 = Big Endian

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_endian(uint8_t *pBigEndian)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pBigEndian = (bool)st0_.dataOrder;
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_niFormat1

   Purpose: Returns the data format of the host meter.

   Arguments: niFormatEnum_t *pFmt, format of the data, uint8_t *pNumBytes

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_niFormat1(niFormatEnum_t *pFmt, uint8_t *pNumBytes)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pFmt = (niFormatEnum_t)st0_.niFormat1;
   *pNumBytes = niFormatBytes[st0_.niFormat1];
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_niFormat2

   Purpose: Returns the data formate of the host meter.

   Arguments: niFormatEnum_t *pFmt, format of the data, uint8_t *pNumBytes

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_niFormat2(niFormatEnum_t *pFmt, uint8_t *pNumBytes)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pFmt = (niFormatEnum_t)st0_.niFormat2;
   *pNumBytes = niFormatBytes[st0_.niFormat2];
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getSizeOfDemands

   Purpose: Returns the size of the demands field in ST23->TOT_DATA_BLOCK->DEMANDS

   Arguments: uint8_t *pDemandsSize - Loc to store the size, uint8_t *pDemandSize - Loc to store demandSize

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getSizeOfDemands(uint8_t *pDemandsSize, uint8_t *pDemandSize)
{
   returnStatus_t retVal = eFAILURE;

   if (tablesRead_)
   {
      niFormatEnum_t    ni1Fmt;     /* Format */
      uint8_t           ni1Size;    /* size in bytes */
      niFormatEnum_t    ni2Fmt;     /* Format */
      uint8_t           ni2Size;    /* size in bytes */

      (void)HMC_MTRINFO_niFormat1(&ni1Fmt, &ni1Size);
      (void)HMC_MTRINFO_niFormat2(&ni2Fmt, &ni2Size);
      *pDemandSize = ni2Size;  /* Initialize to zero */
      if (st21_.dateTimeFieldFlag)
      {
         *pDemandSize += sizeof(MtrTimeFormatLowPrecision_t);
      }
      if (st21_.cumDemandFlag)
      {
         *pDemandSize += ni1Size;
      }
      if (st21_.contCumDemandFlag)
      {
         *pDemandSize += ni1Size;
      }
      *pDemandsSize = (uint8_t)(*pDemandSize * st21_.nbrDemands);
      retVal = eSUCCESS;
   }  /*lint !e438 last value of ni1Fnt, ni2Fnt not used.  */
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getStdTable0

   Purpose: Returns a pointer to Standard Table 0

   Arguments: stdTbl0_t **pTable0

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getStdTable0(stdTbl0_t **pStdTable0)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pStdTable0 = &st0_;
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getMeterMode

   Purpose: To get the Meter Mode

   Arguments: None

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
meterModes_t HMC_MTRINFO_getMeterMode( void )
{
   while ( !HMC_MTRINFO_Ready() )
   {
      OS_TASK_Sleep( 100 );
   }

   if ( !tablesRead_ )
   {
      meterMode_ = eMETER_MODE_UNKNOWN;
   }
   HMC_INFO_PRNT_INFO( 'I', " Meter Mode is %d",  (uint8_t)meterMode_ );
   return( meterMode_ );
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getSoftswitches

   Purpose: Returns a pointer to Softswitches

   Arguments: UpgradesBfld_t **upgrades

   Returns: eSUCCESS or eFAILURE

*********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getSoftswitches( UpgradesBfld_t **upgrades )
{
   returnStatus_t retVal = eSUCCESS;
   if ( !tablesRead_ )
   {
      retVal = eFAILURE;
   }
   *upgrades = &softswitches_;
   return( retVal );
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getStdTable1

   Purpose: Returns a pointer to Standard Table 1

   Arguments: stdTbl1_t **pStdTable1

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getStdTable1(stdTbl1_t **pStdTable1)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pStdTable1 = &st1_;
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getStdTable11

   Purpose: Returns a pointer to Standard Table 11

   Arguments: stdTbl11_t *pStdTable11

   Returns: eSUCCESS or eFAILURE

*********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getStdTable11(stdTbl11_t **pStdTable11)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pStdTable11 = &st11_;
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getStdTable13

   Purpose: Returns a pointer to Standard Table 13

   Arguments: stdTbl13_t **pStdTable13

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getStdTable13(stdTbl13_t **pStdTable13)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pStdTable13 = &st13_;
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getStdTable21

   Purpose: Returns a pointer to Standard Table 21

   Arguments: stdTbl21_t **pStdTable21

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getStdTable21(stdTbl21_t **pStdTable21)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pStdTable21 = &st21_;
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getStdTable22

   Purpose: Returns a pointer to Standard Table 22

   Arguments: stdTbl22_t **pStdTable22

   Returns: eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_getStdTable22(stdTbl22_t **pStdTable22)
{
   returnStatus_t retVal = eSUCCESS;
   if (!tablesRead_)
   {
      retVal = eFAILURE;
   }
   *pStdTable22 = &st22_;
   return(retVal);
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_tableExits

   Purpose: This function will scan through table 0 to determine if a table exists.

   Arguments:  tblOrProc_t tblProc - Table or Procedure
               uint16_t id - Table to check
               tblProp_t *pProperties - Exists, writable

   Returns: see 'returnStatus_t' definition for possible responses.  eSUCCESS or eFAILURE

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_tableExits(tblOrProc_t tblProc, uint16_t id, tblProp_t *pProperties)
{
   returnStatus_t retVal = eFAILURE;        /* Assume failure */

   if (tablesRead_)                        /* Has ST 0 been read? */
   {
      uint8_t *pStartAddr = NULL;             /* Used to navigate the bit fields in table 0. */
      uint8_t *pWrStartAddr = NULL;           /* Used to navigate the bit fields in Table 0, for table writes. */
      uint8_t byteIndex = (uint8_t)(id / 8);    /* Each index contains 8 tables (8-bits, 1 bit per table) */

      (void)memset(pProperties, 0, sizeof(tblProp_t)); /* Initialize pProperties. */

      switch(tblProc)  /* Figure out where to set the pStartAddr and check the range */
      {
         case eSTD_TBL:    /* Search the Standard Table */
         {
            if (byteIndex < st0_.dimStdTblsUsed) /* Is the table being searched in range? */
            {
               pStartAddr = &st0_.stdTblsUsed[byteIndex];
               pWrStartAddr = &st0_.stdTblsWrite[byteIndex];
            }
            break;
         }
         case eMFG_TBL:    /* Search the Manufacturing Table */
         {
            if (byteIndex < st0_.dimMfgTblsUsed) /* Is the table being searched in range? */
            {
               pStartAddr = &st0_.mfgTblsUsed[byteIndex];
               pWrStartAddr = &st0_.mfgTblsWrite[byteIndex];
            }
            break;
         }
         case eSTD_PROC:   /* Search the Standard Procedure */
         {
            if (byteIndex < st0_.dimStdProcUsed) /* Is the table being searched in range? */
            {
               pStartAddr = &st0_.stdProcUsed[byteIndex];
            }
            break;
         }
         case eMFG_PROC:   /* Search the Manufacturing Procedure */
         {
            if (byteIndex < st0_.dimMfgProcUsed) /* Is the table being searched in range? */
            {
               pStartAddr = &st0_.mfgProcUsed[byteIndex];
            }
            break;
         }
         default:
         {
            break;  /* eFAILURE will be returned */
         }
      }

      if (NULL != pStartAddr)
      {
         retVal = eSUCCESS; /* if the pStartAddr has been set, then we have all we need to determine if item exists. */
         uint8_t bitMask = 1 << (id % CHAR_BIT);
         if (*pStartAddr & bitMask) /* Is the bit set? */
         {
            pProperties->exists = 1;
         }
         if (NULL != pWrStartAddr)  /* Check if table writes are allowed? */
         {
            if (*pWrStartAddr & bitMask) /* Is the bit set? */
            {
               pProperties->writeable = 1;
            }
         }
      }
   }
   return(retVal);
}
#endif //#if ( ANSI_STANDARD_TABLES == 1 )
/***********************************************************************************************************************

   Function name: HMC_MTRINFO_demandRstCnt

   Purpose: This function will return if demand reset count exists, the pProperties will contain the table information

   Arguments: quantDesc_t pProperties - Properties to read the information requested

   Returns: see 'returnStatus_t' definition for possible responses.  eSUCCESS or eFAILURE

   Note:    ST23 is the only location that demand reset counter should be read.  The counter in ST25 will not be correct
            per the KV2 documentation.

  *********************************************************************************************************************/
/*lint -esym(715,pProperties) not referenced in i210plus; required for API */
/*lint -esym(818,pProperties) could be const */
returnStatus_t HMC_MTRINFO_demandRstCnt(quantDesc_t *pProperties)
{
   returnStatus_t retVal = eFAILURE;               /* Assume failure */

#if ( ANSI_STANDARD_TABLES == 1 )
   if (st21_.demandResetCtrFlag)                   /* Does this meter support demand? */
   {
      pProperties->dataFmt           = eNIFMT_RSVD15;    /* value is uint8_t, however, there is no definition for uint8_t */
      pProperties->endian            = false;            /* 1 Byte value doesn't matter */
      pProperties->multiplier12      = 1;                /* Multiplier doesn't apply, set to 1. */
      pProperties->multiplier15      = 1;                /* Multiplier doesn't apply, set to 1. */
      pProperties->tblInfo.cnt       = BIG_ENDIAN_16BIT(sizeof(uint8_t)); /* 1 byte field */
      pProperties->tblInfo.offset[0] = 0;                /* 1st bye in STD_TBL_CURRENT_REGISTER_DATA */
      pProperties->tblInfo.offset[1] = 0;                /* 1st bye in STD_TBL_CURRENT_REGISTER_DATA */
      pProperties->tblInfo.offset[2] = 0;                /* 1st bye in STD_TBL_CURRENT_REGISTER_DATA */
      pProperties->tblInfo.tableID   = BIG_ENDIAN_16BIT(STD_TBL_CURRENT_REGISTER_DATA); /* table 23 */
      retVal                         = eSUCCESS;
   }
#endif
   return(retVal);
}
/*lint +esym(715,pProperties) not referenced in i210plus; required for API */
/*lint +esym(818,pProperties) could be const */
/***********************************************************************************************************************

   Function name: HMC_MTRINFO_demandRstDate

   Purpose: This function will return demand reset date/time, if it exists; pProperties will contain table information

   Arguments: quantDesc_t pProperties - Properties to read the information requested

   Returns: see 'returnStatus_t' definition for possible responses.  eSUCCESS or eFAILURE

   Note:    ST25 is the only location where demand reset date/time is available.

  *********************************************************************************************************************/
/*lint -esym(715,pProperties) not referenced in i210plus; required for API */
/*lint -esym(818,pProperties) could be const */
returnStatus_t HMC_MTRINFO_demandRstDate( quantDesc_t *pProperties )
{
   returnStatus_t retVal = eFAILURE;               /* Assume not available */
#if ( ANSI_STANDARD_TABLES == 1 )
   uint32_t       offset;                          /* Offset into table    */

   // Does date/Time field exist?
   if ( st21_.dateTimeFieldFlag )
   {
      pProperties->dataFmt           = eNIFMT_RSVD15;   /* value is uint8_t, however, there is no definition for uint8_t */
      pProperties->endian            = false;           /* 1 Byte value doesn't matter */
      pProperties->multiplier12      = 1;               /* Multiplier doesn't apply, set to 1. */
      pProperties->multiplier15      = 1;               /* Multiplier doesn't apply, set to 1. */
      pProperties->tblInfo.cnt       = BIG_ENDIAN_16BIT( sizeof( MtrTimeFormatLowPrecision_t ) );
      offset                         = ( uint32_t )offsetof( stdTbl25TOU_t, dmdResetDateTime );
      pProperties->tblInfo.offset[0] = ( uint8_t )( offset >> 16 );
      pProperties->tblInfo.offset[1] = ( uint8_t )( ( offset >> 8 ) & 0xff );
      pProperties->tblInfo.offset[2] = ( uint8_t )( offset & 0xff );
      pProperties->tblInfo.tableID   = BIG_ENDIAN_16BIT( STD_TBL_PREVIOUS_DEMAND_RESET_DATA ); /* table 25 */
      retVal                         = eSUCCESS;
   }
#endif
   return( retVal );
}


#if ( LP_IN_METER == 1 )
/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getLPReadingType

   Purpose: Searches for HEEP reading type enum, based on ANSI qty description and LP interval duration

   Arguments:  stdTbl12_t              *st12 - UOM data
               uint8_t                 *st14 - data control info
               uint8_t      intervalDuration - programmed lp interval
               uint16_t                *cUom - variable updated with returned reading type
               uint8_t            *powerOf10 - variable updated with power of ten info

   Returns: eSUCCESS or CIM_QUALCODE_KNOWN_MISSING_READ

*********************************************************************************************************************/
/*lint -esym(715,type) not referenced in i210plus; required for API */
enum_CIM_QualityCode HMC_MTRINFO_getLPReadingType( stdTbl12_t const *st12Data, uint8_t const *st14Data, uint8_t intervalDuration, meterReadingType *cUom, uint8_t *powerOf10 )
{

   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   uint32_t lpVal;        // variable to represent stdTbl12_t as a uint32_t
   uint16_t dataControl;  // variable to represent st14 array as a uint16_t

   *cUom = invalidReadingType; // initialize reading type to zero

   // copy provided arguments into local variables
   (void)memcpy((uint8_t *)&lpVal, (uint8_t *)st12Data, sizeof(stdTbl12_t));
   (void)memcpy((uint8_t *)&dataControl, st14Data, sizeof(dataControl));

   // loop through load profile master lookup table for matching ST12 and ST14(if necessary) data
   for( uint16_t i = 0; i < ARRAY_IDX_CNT(uomLoadProfileLookupTable); i++ )
   {
      if( uomLoadProfileLookupTable[i].uom == lpVal )
      {  // we found a match for ST12 data, now check ST14 if necessary
         if( !NFS_BIT_SET(lpVal) || ( uomLoadProfileLookupTable[i].dataControl == dataControl ) )
         {  // we have a matching UOM, assign proper reading type based upon interval duration

            switch(intervalDuration)
            {  // update reading type based upon lp sample period
               case SIXTY_MIN_INTERVAL:
                 *cUom = (meterReadingType)uomLoadProfileLookupTable[i].lpReadingTypes.sixtyMin;
                  break;
               case THIRTY_MIN_INTERVAL:
                  *cUom = (meterReadingType)uomLoadProfileLookupTable[i].lpReadingTypes.thirtyMin;
                  break;
               case FIFTEEN_MIN_INTERVAL:
                  *cUom = (meterReadingType)uomLoadProfileLookupTable[i].lpReadingTypes.fifteenMin;
                  break;
               case TEN_MIN_INTERVAL:
                  *cUom = (meterReadingType)uomLoadProfileLookupTable[i].lpReadingTypes.fiveMin;
                  break;
               case ONE_MIN_INTERVAL:
                  *cUom = (meterReadingType)uomLoadProfileLookupTable[i].lpReadingTypes.oneMin;
                  break;
               default:
                  break;
            }
         }
      }

      if( *cUom != invalidReadingType )
      {  /* reading type will be updated to non-zero if found, if found indicate success and set data
            at provided pointer arguments */
         retVal = CIM_QUALCODE_SUCCESS;
         *powerOf10 = getUomPowerOfTen(lpVal);
         break;  // we found the UOM, break out of the loop
      }
   }

   return( retVal );
}
/*lint +esym(715,type) not referenced in i210plus; required for API */
#endif

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_convertData

   Purpose: converts *pData to

   Arguments: calcSiData_t *pResult, void *pData, quantDesc_t *pProperties

   Returns: eSUCCESS or eFAILURE

   NOTE:  For this version, only int24, int32_t, int40, int48 and int64_t are supported!!!

  *********************************************************************************************************************/
returnStatus_t HMC_MTRINFO_convertDataToSI(calcSiData_t *pResult, void const *pData, quantDesc_t const *pProperties)
{
   returnStatus_t retVal = eFAILURE;
   uint64_t         result = 0;
   uint64_t         mult = 0;
   uint16_t         cnt = pProperties->tblInfo.cnt;

#if (PROCESSOR_LITTLE_ENDIAN == 1)
   {
      Byte_Swap((uint8_t *)&cnt, sizeof(cnt));  /* Convert */
   }
#endif

   if (cnt <= sizeof(result)) /* Can only convert data up to 64bits */
   {
      /* Handle the following formats blow: */
      if (  (eNIFMT_INT24 == pProperties->dataFmt) ||
            (eNIFMT_INT32 == pProperties->dataFmt) ||
            (eNIFMT_INT40 == pProperties->dataFmt) ||
            (eNIFMT_INT48 == pProperties->dataFmt) ||
            (eNIFMT_INT64 == pProperties->dataFmt))
      {

         if (pProperties->endian)   /* Big endian? */
         {
            (void)memcpy(((uint8_t*) &result) + (sizeof(result) - cnt), ( uint8_t * )pData, cnt);  /*lint !e670  cnt is checked above */

#if ( ANSI_STANDARD_TABLES == 1 )
            mult = (uint64_t) pProperties->multiplier15;
#else
            (void)memcpy(((uint8_t*) &mult)+(sizeof(mult) - sizeof(pProperties->multiplier15)), /*lint !e778 = 0 OK  */
               &pProperties->multiplier15, sizeof(pProperties->multiplier15));
#endif
         }
         else
         {
            (void)memcpy(( uint8_t * )&result, ( uint8_t * )pData, cnt);  /*lint !e670  cnt is checked above */
            (void)memcpy(&mult, &pProperties->multiplier15, sizeof(pProperties->multiplier15));
         }
         if (PROCESSOR_BIG_ENDIAN != pProperties->endian)  /* Convert the data to the proper endianess? */
         {
            Byte_Swap((uint8_t *)&result, sizeof(result));
            //Byte_Swap((uint8_t *)&mult, sizeof(mult));
         }

         if ( mult == 0 )
         {
            pResult->result = result;
         }
         else
         {
            pResult->result = result * mult;
         }

         pResult->exponent = pProperties->multiplier12;
         retVal = eSUCCESS;
      }
   }
   return(retVal);
}
/* ****************************************************************************************************************** */

/***********************************************************************************************************************

   Function name: buildCurrentUomTable

   Purpose: Builds a current representation of the meter's setup of summations, demands, and coincidents.
            This function will populate the static currentMeterSetup_ variable with pointers to the
            lookup table tables found in meter_ansi_sig.h defined for each meter.

   Arguments: none

   Returns: none

   NOTE:

  *********************************************************************************************************************/
static void buildCurrentUomTable(void)
{
   uint32_t              uomST12; // used to store ST12 UOM data read from mtrInfoFileData_t file
   uint16_t      dataControlST14; // used to store ST14 data control data read from mtrInfoFileData_t file

   // initialize the current meter setup lookup table before populating lookup values
   (void)memset((uint8_t *)&currentMeterSetup_, 0, sizeof(currentMeterSetup_));

   // setup the summations
   for( uint8_t i = 0; i < SUM_CFG_IDX_CNT; i++ )
   {
      if( st22_.summationSelect[i] != UNPROGRAMMED_INDEX )
      {  // index is programmed in the meter, gather ST12 and ST14 data

         // read ST12 UOM Data at summation index
         (void)FIO_fread(&mtrInfoFileHndl_,     (uint8_t *)&uomST12, (lCnt)offsetof(mtrInfoFileData_t, sumCfg[0]) + (i * sizeof(uomST12)), sizeof(uomST12));

         // read ST14 data control information at summation index
         (void)FIO_fread(&mtrInfoFileHndl_, (uint8_t *)&dataControlST14, (lCnt)offsetof(mtrInfoFileData_t, stdTbl14.sourceId[0]) + (st22_.summationSelect[i] * sizeof(dataControlST14)), sizeof(dataControlST14));

         for(uint16_t j = 0; j < ARRAY_IDX_CNT(masterSummationLookup); j++)
         {  // search through summation master lookup table for matching UOM data
            if( masterSummationLookup[j].uom == uomST12 )
            {  // ST12 data matches, if NFS bit set, check ST14
               //HMC_INFO_PRNT_INFO( 'I', "NFS bit status for 0x%08x is %d.", uomST12, NFS_BIT_SET(uomST12));
               if( !NFS_BIT_SET(uomST12) || (masterSummationLookup[j].dataControl == dataControlST14) )
               {  // we found a matching UOM entry, assign to current meter setup lookup and break from loop
                  //HMC_INFO_PRNT_INFO( 'I', "Expected ST12: 0x%08x - MasterLookup: 0x%08x.", uomST12, masterSummationLookup[j].uom);
                  //HMC_INFO_PRNT_INFO( 'I', "Expected ST14: 0x%04x - MasterLookup: 0x%04x.", dataControlST14, masterSummationLookup[j].dataControl);
                  currentMeterSetup_.summations[i] = &masterSummationLookup[j];
                  break;
               }
            }
         }

         if( NULL == currentMeterSetup_.summations[i] )
         {  // a matching unit of measuremnt description was not found
            HMC_INFO_PRNT_ERROR('E',  "Unit of Measurement Description for Summation Index: %d NOT found.", i);
         }

      }
      else
      {  // summation index value was not programmed in the meter
         HMC_INFO_PRNT_WARN('W',  "Summation index %d not programmed.", i);
      }

   }

   // setup up the max demands and cumulative demands
   for( uint8_t i = 0; i < DEMAND_CFG_IDX_CNT; i++ )
   {
      if( st22_.demandSelection[i] != UNPROGRAMMED_INDEX )
      {  // index is programmed in the meter, gather ST12 and ST14 data

         // read ST12 UOM Data at demand index
         (void)FIO_fread(&mtrInfoFileHndl_, (uint8_t *)&uomST12, (lCnt)offsetof(mtrInfoFileData_t, demandCfg[0]) + ( i * sizeof(stdTbl12_t)), sizeof(uomST12));

         // read ST14 data control information at demand index
         (void)FIO_fread(&mtrInfoFileHndl_, (uint8_t *)&dataControlST14, (lCnt)offsetof(mtrInfoFileData_t, stdTbl14.sourceId[0]) + ( st22_.demandSelection[i] * sizeof(uint16_t) ), sizeof(dataControlST14));

         for(uint16_t j = 0; j < ARRAY_IDX_CNT(masterMaxDemandLookup); j++)
         {  // search through max demand master lookup table for matching UOM data
            if( masterMaxDemandLookup[j].uom == uomST12 )
            {  // ST12 data matches, if NFS bit set, check ST14
               if( !NFS_BIT_SET(uomST12) || (masterMaxDemandLookup[j].dataControl == dataControlST14) )
               {  // we found a matching UOM entry, assign to current meter setup lookup and break from loop
                  currentMeterSetup_.maxDemand[i] = &masterMaxDemandLookup[j];
                  break;
               }
            }
         }

         if( NULL == currentMeterSetup_.maxDemand[i] )
         {  // a matching unit of measuremnt description was not found
            HMC_INFO_PRNT_ERROR('E',  "Unit of Measurement Description for MaxDemand Index: %d NOT found.", i);
         }

         for(uint16_t j = 0; j < ARRAY_IDX_CNT(masterCumulativeDemandLookup); j++)
         {  // search through cumulative demand master lookup table for matching UOM data
            if( masterCumulativeDemandLookup[j].uom == uomST12 )
            {  // ST12 data matches, if NFS bit set, check ST14
               if( !NFS_BIT_SET(uomST12) || (masterCumulativeDemandLookup[j].dataControl == dataControlST14) )
               {  // we found a matching UOM entry, assign to current meter setup lookup and break from loop
                  currentMeterSetup_.cumDemand[i] = &masterCumulativeDemandLookup[j];
                  break;
               }
            }
         }

         if( NULL == currentMeterSetup_.cumDemand[i] )
         {  // a matching unit of measuremnt description was not found
            HMC_INFO_PRNT_ERROR('E',  "Unit of Measurement Description for Cumulative Demand Index: %d NOT found.", i);
         }

      }
      else
      {  // demand index value was not programmed in the meter
         HMC_INFO_PRNT_WARN('W',  "Demand index %d not programmed.", i);
      }
   }

#if COINCIDENT_SUPPORT
   // setup the coincident measurements
   for( uint8_t i = 0; i < ARRAY_IDX_CNT(st22_.coincidentSelection); i++ )
   {
      if( st22_.coincidentSelection[i] != UNPROGRAMMED_INDEX )
      {  // index is programmed in the meter, gather ST12 and ST14 data

         // read ST12 UOM Data at coincident index in meter info file
         (void)FIO_fread(&mtrInfoFileHndl_, (uint8_t *)&uomST12, (lCnt)offsetof(mtrInfoFileData_t, coincidentCfg[0]) + ( i * sizeof(stdTbl12_t) ), sizeof(uomST12));

         // read ST14 data control information at coincident index in meter info file
         (void)FIO_fread(&mtrInfoFileHndl_, (uint8_t *)&dataControlST14, (lCnt)offsetof(mtrInfoFileData_t, stdTbl14.sourceId[0]) + ( st22_.coincidentSelection[i] * sizeof(uint16_t)), sizeof(dataControlST14));

         if( st22_.coincidentSelection[i] == COINCIDENT_PF1_INDEX || st22_.coincidentSelection[i] == COINCIDENT_PF2_INDEX )
         {  // coincident is a power factor measurement
            currentMeterSetup_.coincident[i] = &masterCoincidentPfLookup[st22_.coincidentSelection[i] % COINCIDENT_PF1_INDEX];
         }
         else
         {  // coincident is a demand measurement
            for(uint16_t j = 0; j < ARRAY_IDX_CNT(masterMaxDemandLookup); j++)
            {  // loop through demand lookup table for matching UOM information
               if( masterMaxDemandLookup[j].uom == uomST12 )
               {  // ST12 data matches, if NFS bit set, check ST14
                  if( !NFS_BIT_SET(uomST12) ||( masterMaxDemandLookup[j].dataControl == dataControlST14 ) )
                  {  // we found a matching UOM entry, assign to current meter setup lookup and break from loop
                     currentMeterSetup_.coincident[i] = &masterMaxDemandLookup[j];
                     break;
                  }
               }
            }
         }

         if( NULL == currentMeterSetup_.coincident[i] )
         {  // a matching unit of measuremnt description was not found
            HMC_INFO_PRNT_ERROR('E',  "Unit of Measurement Description for Coincident #%d NOT found.", i);
         }
      }
      else
      {  // coincident index value was not programmed in the meter
         HMC_INFO_PRNT_WARN('W',  "Coincident index #%d not programmed.", i);
      }
   }
#endif

}


/***********************************************************************************************************************

   Function name: HMC_MTRINFO_searchUomReadingType

   Purpose: Retrieve the reading type for a unit of measure.

   Arguments: uint8_t indexLocation - associated index into current meter setup for summation, demand, coincident.
              uomData_t *uomData    - descriptive information dictating column location to find reading type in
                                      current meter setup.

   Returns: meterReadingType value

   NOTE:

  *********************************************************************************************************************/
meterReadingType HMC_MTRINFO_searchUomReadingType( uint8_t indexLocation, uomData_t const *uomData)
{
   uint16_t readingType = invalidReadingType;        // variable to store returned reading type value
   const uomLookupEntry_t *uomLookupPointer = NULL;  // variable to store pointer into current meter setup
   const DataBlock_t *dataBlock = NULL;              // variable to store pointer to data block in unit of measure entry

   switch( uomData->measureLocation ) /*lint !e788 not all enums used within switch */
   {
      case SUMMATION:   // summation reading
         uomLookupPointer = currentMeterSetup_.summations[indexLocation];
         break;
      case MAX_DMD:    // max demand reading
         uomLookupPointer = currentMeterSetup_.maxDemand[indexLocation];
         break;
      case CUM_DMD:    // cumulative demand reading
         uomLookupPointer = currentMeterSetup_.cumDemand[indexLocation];
         break;
      case COINCIDENT: // coincident reading
         uomLookupPointer = currentMeterSetup_.coincident[indexLocation];
         break;
      default:         // default, leave uomLookupPointer as NULL
         break;
   } /*lint !e788 not all enums used within switch */

   if( NULL != uomLookupPointer )
   {  // pointer has been updated, search the unit of measure entry table for reading type
#if 0
      /* If we are searching for the reading type for a coincident measure and the measurement is not coincident PF, we
         will only return the base reading type not the tiered reading type. */
      if( ( COINCIDENT == uomData->measureLocation ) && uomLookupPointer->uom != COINCIDENT_PF_ST12_VALUE )
      {  // special case, just return the base reading type
         readingType = uomLookupPointer->presentData.totalData;
      }
      else
      {
         switch( uomData->tableLocation )
         {  // which table, preset or previous data
            case eSHIFTED_DATA: // previous data, assign pointer to previous data block
               dataBlock = &uomLookupPointer->previousData;
               break;
            case eCURRENT_DATA: // current data, assign pointer to present data block
               dataBlock = &uomLookupPointer->presentData;
               break;
            default:            // default, do not update the dataBlock pointer
               break;
         }

         if( NULL != dataBlock )
         {  // dataBlock pointer was updated, search the data block in the UOM lookup table entry
            switch ( uomData->dataBlockId )
            {
               case TIER_D:   // assign tierD reading type
                  readingType = dataBlock->tierD;
                  break;
               case TIER_C:   // assign tierC reading type
                  readingType = dataBlock->tierC;
                  break;
               case TIER_B:   // assign tierB reading type
                  readingType = dataBlock->tierB;
                  break;
               case TIER_A:   // assign tierA reading type
                  readingType = dataBlock->tierA;
                  break;
               case NON_TIER: // assign non-tier'd reading type
                  readingType = dataBlock->totalData;
                  break;
               default:      // default, assign invalid reading type
                  readingType = invalidReadingType;
                  break;
            }
         }
      }
#endif
      switch( uomData->tableLocation )
      {  // which table, preset or previous data
         case eSHIFTED_DATA: // previous data, assign pointer to previous data block
            dataBlock = &uomLookupPointer->previousData;
            break;
         case eCURRENT_DATA: // current data, assign pointer to present data block
            dataBlock = &uomLookupPointer->presentData;
            break;
         default:            // default, do not update the dataBlock pointer
            break;
      }

      if( NULL != dataBlock )
      {  // dataBlock pointer was updated, search the data block in the UOM lookup table entry
         switch ( uomData->dataBlockId )
         {
            case TIER_D:   // assign tierD reading type
               readingType = dataBlock->tierD;
               break;
            case TIER_C:   // assign tierC reading type
               readingType = dataBlock->tierC;
               break;
            case TIER_B:   // assign tierB reading type
               readingType = dataBlock->tierB;
               break;
            case TIER_A:   // assign tierA reading type
               readingType = dataBlock->tierA;
               break;
            case NON_TIER: // assign non-tier'd reading type
               readingType = dataBlock->totalData;
               break;
            default:      // default, assign invalid reading type
               readingType = invalidReadingType;
               break;
         }
      }
   }

   // return the reading type
   return (meterReadingType)readingType;
}


/***********************************************************************************************************************

   Function name: searchDataBlock

   Purpose: Search a DataBlock_t for a specific meter reading type value

   Arguments:  meterReadingType RdgType       - reading type we are looking for
               DataBlock_t const * dataBlock  - pointer to the data block we are searching
               blockIdentifier_t *blockId    - pointer to variable to be updated indicating which tier data RdgType is found

   Returns: enum_CIM_QualityCode - did we find it?

   NOTE:

  *********************************************************************************************************************/
enum_CIM_QualityCode searchDataBlock( meterReadingType RdgType, DataBlock_t const * dataBlock, blockIdentifier_t *blockId )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ; // initialize the return value to success, if no match then update

   uint8_t tierSearchCount;
   uint16_t const *tier;

   tier = &dataBlock->totalData;

   // How many tiers do we need to search?
   tierSearchCount = min( st21_.nbrTiers, (sizeof(DataBlock_t)/sizeof(dataBlock->totalData )) - 1 );

   for( uint8_t i = 0; i <= tierSearchCount; i++, tier++)
   {     // check each tier for matching reading type
      if( RdgType == *tier)
      {  // we found a matching reading type, assign the tier value and indicate reading type search success
         *blockId = (blockIdentifier_t)i;
         retVal = CIM_QUALCODE_SUCCESS;
		 break;
      }
   }

   return retVal;
}


/***********************************************************************************************************************

   Function name: searchUomLookupEntry

   Purpose: Search a DataBlock_t for a specific meter reading type value

   Arguments:  meterReadingType RdgType           - reading type we are looking for
               uomLookupEntry_t const * uomEntry  - pointer to the uomLookupEntry_t we are searching
               uomData_t *uomData                 - pointer to variable to be updated indicating UOM information

   Returns: enum_CIM_QualityCode - did we find it?

   NOTE:

  *********************************************************************************************************************/
enum_CIM_QualityCode searchUomLookupEntry( meterReadingType RdgType, uomLookupEntry_t const * uomEntry, uomData_t *uomData)
{
   enum_CIM_QualityCode retVal;

   // search the present data block
   retVal = searchDataBlock( RdgType, &uomEntry->presentData, &uomData->dataBlockId );

   if( CIM_QUALCODE_SUCCESS == retVal )
   {  // reading type was found in the present data block
      uomData->tableLocation = eCURRENT_DATA;
   }
   else
   {  // reading type has not been found, search the previous data block
      retVal = searchDataBlock( RdgType, &uomEntry->previousData, &uomData->dataBlockId );
      if( CIM_QUALCODE_SUCCESS == retVal )
      {  // reading type was found in the previous data block
         uomData->tableLocation = eSHIFTED_DATA;
      }
   }

   return retVal;
}


/***********************************************************************************************************************

   Function name: searchSummations

   Purpose: Search the summation array from the current meter setup

   Arguments:  meterReadingType RdgType  - reading type we are looking for
               uomData_t *uomData        - pointer to variable to be updated indicating UOM information
               uint8_t *index            - index location in ST22 where reading type is found
               int8_t *powOfTen          - power ten variable updated for specifically for reading type

   Returns: enum_CIM_QualityCode - did we find it?

   NOTE:

  *********************************************************************************************************************/
static enum_CIM_QualityCode searchSummations( meterReadingType RdgType, uomData_t *uomData, uint8_t *index, int8_t *powOfTen )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   // index through the master summation lookup table
   for( uint8_t i = 0; i < ARRAY_IDX_CNT(currentMeterSetup_.summations); i++ )
   {
      retVal = searchUomLookupEntry( RdgType, currentMeterSetup_.summations[i], uomData);
      if( CIM_QUALCODE_SUCCESS == retVal )
      {
         uomData->measureLocation   = SUMMATION;
         *index = i;
         *powOfTen = (int8_t)getUomPowerOfTen(currentMeterSetup_.summations[i]->uom);
		 break;
      }
   }

   return retVal;
}


/***********************************************************************************************************************

   Function name: searchMaxDemands

   Purpose: Search the max demand array from the current meter setup

   Arguments:  meterReadingType RdgType  - reading type we are looking for
               uomData_t *uomData        - pointer to variable to be updated indicating UOM information
               uint8_t *index            - index location in ST22 where reading type is found
               int8_t *powOfTen          - power ten variable updated for specifically for reading type

   Returns: enum_CIM_QualityCode - did we find it?

   NOTE:

  *********************************************************************************************************************/
static enum_CIM_QualityCode searchMaxDemands( meterReadingType RdgType, uomData_t *uomData, uint8_t *index, int8_t *powOfTen )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   // search maxDemands
   for( uint8_t i = 0; i < ARRAY_IDX_CNT(currentMeterSetup_.maxDemand); i++ )
   {
      retVal = searchUomLookupEntry( RdgType, currentMeterSetup_.maxDemand[i], uomData);
      if( CIM_QUALCODE_SUCCESS == retVal )
      {
         uomData->measureLocation   = MAX_DMD;
         *index = i;
         *powOfTen = (int8_t)getUomPowerOfTen(currentMeterSetup_.maxDemand[i]->uom);
		 break;
      }
   }

   return retVal;
}


/***********************************************************************************************************************

   Function name: searchCumDemands

   Purpose: Search the cumulative demand array from the current meter setup

   Arguments:  meterReadingType RdgType  - reading type we are looking for
               uomData_t *uomData        - pointer to variable to be updated indicating UOM information
               uint8_t *index            - index location in ST22 where reading type is found
               int8_t *powOfTen          - power ten variable updated for specifically for reading type

   Returns: enum_CIM_QualityCode - did we find it?

   NOTE:

  *********************************************************************************************************************/
static enum_CIM_QualityCode searchCumDemands( meterReadingType RdgType, uomData_t *uomData, uint8_t *index, int8_t *powOfTen )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   // search cumDemands
   for( uint8_t i = 0; i < ARRAY_IDX_CNT(currentMeterSetup_.cumDemand); i++ )
   {
      retVal = searchUomLookupEntry( RdgType, currentMeterSetup_.cumDemand[i], uomData);
      if( CIM_QUALCODE_SUCCESS == retVal )
      {
         uomData->measureLocation   = CUM_DMD;
         *index = i;
         *powOfTen = (int8_t)getUomPowerOfTen(currentMeterSetup_.cumDemand[i]->uom);
		 break;
      }
   }

   return retVal;
}


/***********************************************************************************************************************

   Function name: HMC_MTRINFO_searchUom

   Purpose: Search the current meter setup table to determine if meter is programmed for a reading type

   Arguments:  meterReadingType RdgType  - reading type we are looking for
               quantDesc_t *pProperties  - pointer to variable to describe how to get the value out of meter

   Returns: enum_CIM_QualityCode - did we find it?

   NOTE:

  *********************************************************************************************************************/
enum_CIM_QualityCode HMC_MTRINFO_searchUom( meterReadingType RdgType, quantDesc_t *pProperties )
{
   enum_CIM_QualityCode retVal;
   //uomData_t uomData;

   niFormatEnum_t niFormat1;           /* Contains the format */
   uint8_t        niDataSize1;         /* Contains the number of bytes in niFormat1 */
   niFormatEnum_t niFormat2;           /* Contains the format of the data */
   uint8_t        niDataSize2;         /* Contains the number of bytes in niFormat2 */
   uint8_t        sizeOfDemandBlock;   /* Size of the demands field in ST23 */
   uint8_t        sizeOfDemand;        /* Size of the demand field in ST23 */

   // search the current setup summation table
   retVal = searchSummations( RdgType, &pProperties->uomData, &pProperties->tblIdx, &pProperties->multiplier12);
   if( retVal == CIM_QUALCODE_SUCCESS)
   {  // found the reading type in summation current setup
      pProperties->uomData.measureLocation = SUMMATION;
   }

   if( retVal == CIM_QUALCODE_KNOWN_MISSING_READ )
   {  // no matching reading type yet, search the current setup max demand table
      retVal = searchMaxDemands( RdgType, &pProperties->uomData, &pProperties->tblIdx, &pProperties->multiplier12 );
      if( retVal == CIM_QUALCODE_SUCCESS )
      {  // found the reading type max demand current setup
         pProperties->uomData.measureLocation = MAX_DMD;
      }
   }

   if( retVal == CIM_QUALCODE_KNOWN_MISSING_READ  )
   {  // no matching reading type yet, search the current setup cumulative demand table
      retVal = searchCumDemands( RdgType, &pProperties->uomData, &pProperties->tblIdx, &pProperties->multiplier12 );
      if( retVal == CIM_QUALCODE_SUCCESS )
      {  // found the reading type in cumulative demand current setup
         pProperties->uomData.measureLocation = CUM_DMD;
      }
   }

   if( CIM_QUALCODE_SUCCESS == retVal )
   {  // successfully found the UOM in the meter, now calculate the offset to retrieve the value

      uint32_t tblOffset  = 0;          // variable to track offset into ST23 or ST25
      uint32_t dataBlkRcd;              // variable to store the size of a data block
      uint8_t  *indexValueTbl22 = 0;    // pointer to summation, demand, or coincident index values in ST22

      //pProperties->uomData.tableLocation = uomData.tableLocation;
      (void)HMC_MTRINFO_niFormat1(&niFormat1, &niDataSize1);
      (void)HMC_MTRINFO_niFormat2(&niFormat2, &niDataSize2);
      (void)HMC_MTRINFO_getSizeOfDemands(&sizeOfDemandBlock, &sizeOfDemand);

      // calculate size of a data block
      dataBlkRcd = st21_.nbrSummations * niDataSize1;
      dataBlkRcd += sizeOfDemandBlock;
      dataBlkRcd += st21_.nbrCoinValues * niDataSize2;

      pProperties->endian = st0_.dataOrder;
      //pProperties->tblIdx = uomData.indexTracker;

      if ( eSHIFTED_DATA == pProperties->uomData.tableLocation )
      {  // If shift quantity is requested, set table ID to ST25
         pProperties->tblInfo.tableID = BIG_ENDIAN_16BIT(STD_TBL_PREVIOUS_DEMAND_RESET_DATA);
         // Table 25 looks like Table 23, optionally offset by END_DATE_TIME and/or SEASON
         if (st21_.dateTimeFieldFlag)
         {  // date/Time field exist, if so, account for the size
            tblOffset += sizeof( MtrTimeFormatLowPrecision_t );
         }
         if (st21_.seasonInfoFieldFlag)
         { // season field exist, if so, account for the size
            tblOffset += 1;
         }
      }
      else
      {  // data is in the present billing table, set table ID to 23
         pProperties->tblInfo.tableID = BIG_ENDIAN_16BIT(STD_TBL_CURRENT_REGISTER_DATA);
      }

      // Tables 23 and 25 are optionally offset by a demand reset counter
      if(st21_.demandResetCtrFlag)
      {  // If true, then data is offset by 1
         tblOffset++;
      }

      // shift offset to correct tier location
      tblOffset += ((uint8_t)pProperties->uomData.dataBlockId) * dataBlkRcd;


      if( CUM_DMD == pProperties->uomData.measureLocation || MAX_DMD == pProperties->uomData.measureLocation )
      {  // UOM is a demand, read the demand block, jump over all summation and to correct demand index
         tblOffset += st21_.nbrSummations * niDataSize1;
         tblOffset += pProperties->tblIdx * sizeOfDemand;
         pProperties->tblInfo.cnt = BIG_ENDIAN_16BIT( sizeOfDemand );
         indexValueTbl22 = &st22_.demandSelection[0];
         if( MAX_DMD == pProperties->uomData.measureLocation )
         {
            pProperties->dataFmt = niFormat2;
         }
         else
         {
            pProperties->dataFmt = niFormat1;
         }
      }
      else
      {  // UOM is a summation, jump to correct summation index
         tblOffset += pProperties->tblIdx * niDataSize1;
         pProperties->tblInfo.cnt = BIG_ENDIAN_16BIT(niDataSize1);
         indexValueTbl22 = &st22_.summationSelect[0];
         pProperties->dataFmt = niFormat1;
      }

      // set the table offset
      (void)memcpy(&pProperties->tblInfo.offset[0], ( uint8_t * )&tblOffset, sizeof(pProperties->tblInfo.offset));
      Byte_Swap(&pProperties->tblInfo.offset[0], sizeof(pProperties->tblInfo.offset));

      // retrieve the scalar from ST15
      (void)FIO_fread( &mtrInfoFileHndl_,
                       (uint8_t *)&pProperties->multiplier15,
                       (uint16_t)offsetof(mtrInfoFileData_t, stdTbl15) + (sizeof(stdTbl15_t) * ( indexValueTbl22[pProperties->tblIdx] % TABLE_15_MODULO) ),
                        sizeof(pProperties->multiplier15));


   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_searchCoinUom

   Purpose: Searches for lookup information relative to specific demand coincident

   Arguments:  meterReadingType demandReadingType - demand reading type we want a coincident value for
               uint8_t coincNum                   - which coincident we want
               quantDesc_t *pProperties           - properties that describe where to retrieve the value

   Returns: enum_CIM_QualityCode - did we find a coincicdent

*********************************************************************************************************************/
enum_CIM_QualityCode HMC_MTRINFO_searchCoinUom( meterReadingType demandReadingType, uint8_t coincNum, quantDesc_t *pProperties )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

#if COINCIDENT_SUPPORT
   // search for demand reading information, this will tell us where to get coincident info
   if( CIM_QUALCODE_SUCCESS == searchMaxDemands( demandReadingType, &pProperties->uomData, &pProperties->tblIdx, &pProperties->multiplier12 ) )
   {
      stdTbl22_t     *pSt22;
      if ( eSUCCESS ==  HMC_MTRINFO_getStdTable22( &pSt22 ) )
      {
        // track number of coincidents found for max demand UOM
         uint8_t coincCnt = 0;

         for ( uint8_t assocIdx = 0; assocIdx < ST23_COINCIDENTS_CNT; assocIdx++ )
         {  // for each associate coincident index, check if this is the one we are looking for
            if ( ( pSt22->coincidentDemandAssoc[ assocIdx ] == pProperties->tblIdx ) &&  /* Match the demand index?  */
                 ( pSt22->coincidentSelection[ assocIdx ] != 0xff ) )                    /* Check for valid st12 index */
            {
               if( coincCnt == coincNum )
               {  // does the coincident measurement found match the coincident number we are looking for
                  pProperties->uomData.measureLocation = COINCIDENT;  // reading we want is a coicident
                  pProperties->tblIdx = assocIdx;                     // store the index
                  pProperties->multiplier12 = (int8_t)getUomPowerOfTen( currentMeterSetup_.coincident[assocIdx]->uom );
                  retVal = CIM_QUALCODE_SUCCESS;                      // we found the coincident, update return value
                  break;                                              // break out of loop
               }
               coincCnt++;
            }
         }
      }
   }

   if( CIM_QUALCODE_SUCCESS == retVal )
   {  // successfully found the UOM in the meter, now calculate the offset to retrieve the value
      niFormatEnum_t niFormat1;           /* Contains the format */
      uint8_t        niDataSize1;         /* Contains the number of bytes in niFormat1 */
      niFormatEnum_t niFormat2;           /* Contains the format of the data */
      uint8_t        niDataSize2;         /* Contains the number of bytes in niFormat2 */
      uint8_t        sizeOfDemandBlock;   /* Size of the demands field in ST23 */
      uint8_t        sizeOfDemand;        /* Size of the demand field in ST23 */

      uint32_t tblOffset  = 0;           // variable to track offset into ST23 or ST25
      uint32_t dataBlkRcd;               // variable to store the size of a data block

      /*lint -esym(838,indexValueTbl22) previous value not used */
      uint8_t  *indexValueTbl22 = NULL;  // pointer to summation, demand, or coincident index values in ST22

      //pProperties->uomData.tableLocation = uomData.tableLocation;
      (void)HMC_MTRINFO_niFormat1(&niFormat1, &niDataSize1);
      (void)HMC_MTRINFO_niFormat2(&niFormat2, &niDataSize2);
      (void)HMC_MTRINFO_getSizeOfDemands(&sizeOfDemandBlock, &sizeOfDemand);

      // calculate size of a data block
      dataBlkRcd = st21_.nbrSummations * niDataSize1;
      dataBlkRcd += sizeOfDemandBlock;
      dataBlkRcd += st21_.nbrCoinValues * niDataSize2;

      pProperties->endian = st0_.dataOrder;

      if ( eSHIFTED_DATA == pProperties->uomData.tableLocation )
      {  // If shift quantity is requested, set table ID to ST25
         pProperties->tblInfo.tableID = BIG_ENDIAN_16BIT(STD_TBL_PREVIOUS_DEMAND_RESET_DATA);
         // Table 25 looks like Table 23, optionally offset by END_DATE_TIME and/or SEASON
         if (st21_.dateTimeFieldFlag)
         {  // date/Time field exist, if so, account for the size
            tblOffset += sizeof( MtrTimeFormatLowPrecision_t );
         }
         if (st21_.seasonInfoFieldFlag)
         { // season field exist, if so, account for the size
            tblOffset += 1;
         }
      }
      else
      {  // data is in the present billing table, set table ID to 23
         pProperties->tblInfo.tableID = BIG_ENDIAN_16BIT(STD_TBL_CURRENT_REGISTER_DATA);
      }

      // Tables 23 and 25 are optionally offset by a demand reset counter
      if(st21_.demandResetCtrFlag)
      {  // If true, then data is offset by 1
         tblOffset++;
      }

      // shift offset to correct tier location
      tblOffset += ((uint8_t)pProperties->uomData.dataBlockId) * dataBlkRcd;

      // adjust offset to correct coincident value
      tblOffset += st21_.nbrSummations * niDataSize1;
      tblOffset += st21_.nbrDemands * sizeOfDemand;
      tblOffset += pProperties->tblIdx * niDataSize2;
      pProperties->dataFmt = niFormat2;
      pProperties->tblInfo.cnt = BIG_ENDIAN_16BIT(niDataSize2);
      indexValueTbl22 = &st22_.coincidentSelection[0];

      // set the table offset
      (void)memcpy(&pProperties->tblInfo.offset[0], ( uint8_t * )&tblOffset, sizeof(pProperties->tblInfo.offset));
      Byte_Swap(&pProperties->tblInfo.offset[0], sizeof(pProperties->tblInfo.offset));

      /* Calculating the offset for ST15 scale factors when retrieving the value for a coincident measurement is tricky.
         Using the standard modulo 20 does not apply for Power Factor coincident measurements.  UOM index 0-19 apply
         to summations, while index 40-59 apply to demands.  If the coincident measuremnt is a demand value, then the
         standard modulo 20 index into ST15 applies.  If the specific coincident measurement is a PF measurement,
         we must skip pass the first 20 entries in the table.  Additional indexing further, must be associated with
         modulating the PF coincident indexes which start at an index value of 60. */
      uint16_t st15fOffset = 0;
      if( indexValueTbl22[pProperties->tblIdx] == COINCIDENT_PF1_INDEX ||
          indexValueTbl22[pProperties->tblIdx] == COINCIDENT_PF2_INDEX )
      {  // this is a PF coincident, skip past the first 20 index of ST15, then index further based upon PF coindient modulo
         st15fOffset = ( sizeof(stdTbl15_t) * TABLE_15_MODULO ) +
                       ( sizeof(stdTbl15_t) * (indexValueTbl22[pProperties->tblIdx] % PF_COINCIDENT_60_MODULO ) );
      }
      else
      {  // this is a demand coincident, the standard table 15 modulo applies as specified in the programming guide
         st15fOffset = (sizeof(stdTbl15_t) * ( indexValueTbl22[pProperties->tblIdx] % TABLE_15_MODULO) );
      }

      // retrieve the scalar from ST15
      (void)FIO_fread( &mtrInfoFileHndl_, (uint8_t *)&pProperties->multiplier15,
                       (uint16_t)offsetof(mtrInfoFileData_t, stdTbl15) + st15fOffset, sizeof(pProperties->multiplier15));
   }
#endif

   return retVal;
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_searchDirectRead

   Purpose: Search the list of meter table direct reads to find information on how to read information from a
            meter table.

   Arguments:  meterReadingType RdgType         - the reading type we are looking for
               directLookup_t *directLookupData - pointer to item from lookup table

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode HMC_MTRINFO_searchDirectRead( meterReadingType RdgType, directLookup_t *directLookupData )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   directLookup_t *directLkDataPtr;

   // search the list of direct reads
   if (eSUCCESS == OL_Search(OL_eHMC_DIRECT, (ELEMENT_ID)RdgType, (void **)&directLkDataPtr)) /* Does quantity requested exist? */
   {  // we found a match,
      retVal = CIM_QUALCODE_SUCCESS;
      directLookupData->quantity = directLkDataPtr->quantity;
      directLookupData->typecast = directLkDataPtr->typecast;
      directLookupData->tbl = directLkDataPtr->tbl;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: getUomPowerOfTen

   Purpose: Searches for HEEP enum, based on ANSI qty description.

   Arguments:  uint32_t uom - ST12 data for measurement

   Returns: uint8_t - power of ten value

*********************************************************************************************************************/
static uint8_t getUomPowerOfTen( uint32_t uom )
{
   uint8_t powerOfTen = 0;      // store power of ten return value
   uint8_t idCode;              // store id code value from uom argument

   idCode = UOM_ID_CODE(uom);  // retrieve the uom id code

   // is the id code provided a valid value?
   if( idCode <= (uint8_t)eLastIdCode )
   {  // the id code is valid
      switch((uomIdCodes_t)idCode) /*lint !e788 not all enums used within switch */
      {
         case eActivePower_W:       // (0)  Active power W
         case eReactivePower_VAR:   // (1)  Reactive power VAR
         case eApparentPower_VA:    // (2)  Apparent power VA
         case ePhasorPower_VA:      // (3)  Phasor power VA
         case eQuantityPower_Q60:   // (4)  Quantity power Q(60)
            powerOfTen = 9;
            break;
         case eRmsAmpsSquared:      // (14) RMS Amps Squared (I2)
            powerOfTen = 8;
            break;
         case eRmsVoltsSquard:      // (10) RMS Volts Squared (V2)
            powerOfTen = 6;
            break;
         case eRmsVolts:            // (8)  RMS Volts
         case eAveVolts:            // (9)  Average Volts
         case eRmsAmps:             // (12) RMS Amps
         case eAverageCurrent:      // (13) Average Current
            powerOfTen = 1;
            break;
         case eFrequency:           // (33) Frequency
         case ePa_PwrFactorAP:      // (24) Power factor computed using apparent power
         case ePa_AvePwrFactor:     // (26) Average Power Factor
         case eThdV_IEEE:           // (16) T.H.D. V
         case eThdI_IEEE:           // (17) T.H.D. I
            powerOfTen = 2;
            break;
         case ePa_PwrFactorPP:      // (25) Coincident PF
            powerOfTen = 3;
            break;
         case eCounter:             // (34) Counter
         case eTempDegreesC:        // (70) Temperature
         default:                   // default case, no power of ten
            powerOfTen = 0;
            break;
      } /*lint !e788 not all enums used within switch */
   }

   return powerOfTen;
}


/***********************************************************************************************************************

   Function name: searchHmcUomPowerOfTen

   Purpose: Retrieve the power of ten value for a HMC UOM from the meter

   Arguments:  meterReadingType RdgType

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode searchHmcUomPowerOfTen( meterReadingType RdgType, uint8_t *powerOfTen )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   quantDesc_t qDesc;  // variable to store information to describe and to get the reading

   // initialize the quantDesc_t variable
   (void)memset((uint8_t *)&qDesc, 0, sizeof(qDesc));

   if( CIM_QUALCODE_SUCCESS == HMC_MTRINFO_searchUom( RdgType, &qDesc ) )
   {  // if we found the reading, set the power of ten
      *powerOfTen = (uint8_t)qDesc.multiplier12;
      retVal = CIM_QUALCODE_SUCCESS;
   }

   return retVal;
}


/***********************************************************************************************************************

   Function name: searchDirectReadPowerOfTen

   Purpose: Retrieve a meter reading via direct table read

   Arguments:  meterReadingType RdgType

   Returns: enum_CIM_QualityCode

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
enum_CIM_QualityCode searchDirectReadPowerOfTen( meterReadingType RdgType, uint8_t *powerOfTen )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   directLookup_t directReadInfo; // variable to store information about the reading type

   if(  CIM_QUALCODE_SUCCESS == HMC_MTRINFO_searchDirectRead( RdgType, &directReadInfo ) )
   {  // if we found the reading, set the power of ten
      *powerOfTen = (uint8_t)directReadInfo.tbl.multiplier12;
      retVal = CIM_QUALCODE_SUCCESS;
   }

   return retVal;
}

/***********************************************************************************************************************

   Function name: HMC_MTRINFO_getMeterReadingPowerOfTen

   Purpose: Retrieve the power of ten value for a reading type found in the meter

   Arguments:  meterReadingType RdgType

   Returns: uint8_t - power of ten value

   Side effects: N/A

   Reentrant: No

 **********************************************************************************************************************/
uint8_t HMC_MTRINFO_getMeterReadingPowerOfTen( meterReadingType RdgType )
{
   uint8_t powerOfTen = 0;  // variable to store and return the power of ten value found

   // search for reading type in the ST12 measurment lookup table
   if(  CIM_QUALCODE_SUCCESS != searchHmcUomPowerOfTen( RdgType, &powerOfTen ) )
   {  // reading type not found in ST12 lookup, now search the direct meter read lookup
      (void)searchDirectReadPowerOfTen( RdgType, &powerOfTen );
   }

   return powerOfTen;
}