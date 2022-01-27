// <editor-fold defaultstate="collapsed" desc="File Header">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: hmc_ds_cfg.h
 *
 * Contents: Host Meter Disconnect Switch Configuration Tables
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2015 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ KAD Created Nov 15, 2012
 *
 **********************************************************************************************************************/
#ifndef hmc_ds_cfg_H
#define hmc_ds_cfg_H
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "hmc_app.h"
#include "ansi_tables.h"

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Global Definitions">
/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef HMC_DS_GLOBALS
#define HMC_DS_EXTERN
#else
#define HMC_DS_EXTERN extern
#endif

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define TWACS_REGISTERS_SUPPORTED 0    /* TWACS-style registers not supported in this product.  */

#define PROC_SEQ_NUM                   ((uint8_t)0xDD)  /* G+ Selector ID for the procedure. */

/* ---------- MFG Table 12 - Meter Status & Configuration Masks and bit definitions ---------- */

/* Reg #512 Values - Used in the constant arrays defined below. */
#define SWITCH_STATE_OPEN              ((uint8_t)0)
#define SWITCH_STATE_CLOSED            ((uint8_t)1)
#define SWITCH_STATE_UNARMED           ((uint8_t)0)
#define SWITCH_STATE_ARMED             ((uint8_t)1)

//lint -esym(750,LAST_OP_ATTEMPTED_ARM_OPEN)              LAST_OP_ATTEMPTED_ARM_OPEN not referenced
#define LAST_OP_ATTEMPTED_CLOSE        ((uint8_t)0)
#define LAST_OP_ATTEMPTED_OPEN         ((uint8_t)1)
#define LAST_OP_ATTEMPTED_ARM_CLOSE    ((uint8_t)2)
#define LAST_OP_ATTEMPTED_ARM_OPEN     ((uint8_t)3)

#define LAST_OP_SOURCE_CMD             ((uint8_t)0)
#define LAST_OP_SOURCE_AUTO            ((uint8_t)1)
#define LAST_OP_SOURCE_TOU             ((uint8_t)2)
#define LAST_OP_SOURCE_MANUAL          ((uint8_t)3)
#define LAST_OP_SOURCE_OVER_UNDER      ((uint8_t)4)
#define LAST_OP_SOURCE_SD_CONTACT      ((uint8_t)5)
#define LAST_OP_SOURCE_DLP             ((uint8_t)6)
#define LAST_OP_SOURCE_ECP             ((uint8_t)7)
#define LAST_OP_SOURCE_PPM             ((uint8_t)8)

#define LAST_SWITCH_VIA_PB_TRUE        ((uint8_t)1)
#define LAST_SWITCH_VIA_PB_FALSE       ((uint8_t)0)

/* ---------- Register ID Defintions  ---------- */
#define REG_DS_STATUS                  ((uint16_t)512)
#define REG_DS_STATUS_SIZE             (sizeof(regStateStatus_u))
#define REG_DS_EXT_STATUS              ((uint16_t)513)
#define REG_DS_CONTACT_CNT             ((uint16_t)515)
#define REG_DS_CONTACT_CNT_SIZE        ((uint8_t)2)

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes (static)">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#ifdef HMC_DS_GLOBALS
#if ( REMOTE_DISCONNECT == 1 )
static uint8_t procedureResultsSwitchCmd( uint8_t cmd, HMC_COM_INFO const *pData );
static uint8_t processResultsSwitchStateforClose( uint8_t cmd, HMC_COM_INFO const *pData);
static uint8_t processResultsSwitchStateforOpen( uint8_t cmd, HMC_COM_INFO const *pData );
static uint8_t processResultsSwitchState( uint8_t cmd, HMC_COM_INFO const *pData );
static uint8_t ReadMT117Results( uint8_t cmd, HMC_COM_INFO const *pData );
#endif
#endif

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef uint8_t ( *fptr_ds )(uint8_t cmd, sMtrRxPacket *);

// <editor-fold defaultstate="collapsed" desc="lockoutTime_t:  Sets Delay Between Opcode Commands">
typedef struct
{
   uint8_t chargeTimeSec; /* Lockout time for capacitor charge time after a switch movement. */
   uint8_t armLockoutSec; /* Lockout time after an arm command. */
} lockoutTime_t;        /* Sets delay between opcode #151 commands. */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="cmdFnct_t - Used to pair command sent to host meter with
// response function to parse results.">
typedef struct
{
   HMC_PROTO_Table  const *pCmd;      /* Command to send to the host meter */
   fptr_ds                fpFunct;    /* Function to process the command results */
}cmdFnct_t;     /* Used to pair the command sent to the host meter with the response function to parse the results. */
// </editor-fold>

// < editor-fold defaultstate="collapsed" desc="regStateStatus_u: - Service Disconnect Switch State and Status">
#if PROCESSOR_LITTLE_ENDIAN == 1 && BIT_ORDER_LOW_TO_HIGH == 1
PACK_BEGIN
typedef union PACK_MID
{
   struct
   {
      uint16_t swClosed:               1; /* 0 = OPENED, 1 = CLOSED. */
      uint16_t rsvd:                   1; /* reserved */
      uint16_t UC:                     1; /* Unsupported Configuration -based on register 2244 & 2243 */
      uint16_t rsvd1:                  1; /* reserved */
      uint16_t sdCmdStatus:            4; /* Command status codes from the meter  */
      uint16_t rsvd2:                  1; /* reserved */
      uint16_t lastOperationSource:    4; /* Source of origin of the last valid SD operation */
      uint16_t lastOperationAttempted: 3; /* Last valid Service Disconnect operation attempted on the meter
                                                                                       0 = CLOSE; 1 = OPEN;
                                             NOTE that this is opposite of bit 0 where 0 = OPEN ; 1 = CLOSE */
   } bits;
   uint16_t reg;
}regStateStatus_u;  /* Register 512.4 - Service Disconnect Switch State and Status */
PACK_END
#else
#if PROCESSOR_LITTLE_ENDIAN == 0
#error "pqFileData_t is not defined for a big endian machine!  The structure above must be defined for big endian."
#endif
#if BIT_ORDER_LOW_TO_HIGH == 0
#error "pqFileData_t is not defined for a bit order of high to low. "
#endif
#endif
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Global Variables">
/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */
// </editor-fold>

/* ****************************************************************************************************************** */
/* CONSTANTS */
#ifdef HMC_DS_GLOBALS

// <editor-fold defaultstate="collapsed" desc="lockoutTime_t: Timing definitions">
/* The order of each index MUST correspond with the order of the meter type definitions.  */
static const lockoutTime_t chargeTimeSec_[] =
{
/* Command Lockout time Y72424-PDS Rev. D - UMT-R-GCB v6.0.doc, 185 Minutes between any two consecutive
   switch operation requests to the meter. The Arm Lockout Time is for products that support Arm commands   */

/* Charge Time(Sec) , Arm Lockout Time (Sec)*/
#if TM_SD_UNIT_TEST
   { 5,     13 }, // For testing RF electric
#else
   { 185,   13 }, // Per Y56966-PDS Rev. D - UMT-R-G+ RD v4.00.doc section 4.4 TODO - find correct value
#endif
   { 10,    13 },
   { 10,    1 }   // Focus Lockout time - 10 seconds
}; // </editor-fold>


// <editor-fold defaultstate="collapsed" desc="Common procedure defintions">
/* ------------------------------------------------------------------------------------------------------------------ */
/* Common procedure defintions */

//lint -e{572,845}
static const tReadPartial tblSwProcResult_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   {
      CMD_TBL_RD_PARTIAL,
      PSEM_PROCEDURE_TBL_RES,
      { TBL_PROC_RES_OFFSET},
      BIG_ENDIAN_16BIT( sizeof (tProcResult ) ) /* Length to read */
   }
#elif HMC_FOCUS
   {
      CMD_TBL_RD_FULL,
      PSEM_PROCEDURE_TBL_RES
   }
#else
   0
#endif
};

//lint -e{648,572,845}
#if ( HMC_I210_PLUS == 1 )
static const tWritePartial tblSwProcedure_[] =
{
   {  /* Meter Display Information */
      CMD_TBL_WR_PARTIAL,
      BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE ), /* ST.Tbl 7 will be written  */
      {
         (uint8_t) 0, (uint8_t) 0, (uint8_t) 0        /* Offset             */
      },
      BIG_ENDIAN_16BIT(5)                             /* Length in bytes for the table */
   }
};
#elif ( ( HMC_I210_PLUS_C == 1 ) || ( HMC_FOCUS == 1 ) )
static const tWriteFull tblSwProcedure_[] =
{
#if ( HMC_I210_PLUS_C  == 1 )
   {  /* Meter Display Information */
      CMD_TBL_WR_FULL,
      BIG_ENDIAN_16BIT( STD_TBL_PROCEDURE_INITIATE ), /* ST.Tbl 7 will be written      */
   }
#elif HMC_FOCUS
   {
      CMD_TBL_WR_FULL,
      PSEM_PROCEDURE_TBL_ID
   }
#else
   0
#endif
};
#endif

static const HMC_PROTO_TableCmd tblCmdSwRes_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (tblSwProcResult_ ), ( uint8_t far * ) & tblSwProcResult_[0]},
   { HMC_PROTO_MEM_NULL}
#elif HMC_FOCUS
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (tblSwProcResult_ ), ( uint8_t far * ) & tblSwProcResult_[0]},
   { HMC_PROTO_MEM_NULL}
#else
   { HMC_PROTO_MEM_NULL}
#endif
};
/* ------------------------------------------------------------------------------------------------------------------ */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Switch Open Commands">
/* ------------------------------------------------------------------------------------------------------------------ */
/* Switch Open Commands */
static const uint8_t openSwProcedureData_[] =
{
#if ( HMC_I210_PLUS == 1 )
   PROC_STD,                        /* Standard Procedure */
   STD_PROC_DIRECT_LOAD_CONTROL,    /* Disconnect Switch Procedure Number */
   PROC_SEQ_NUM,                    /* G+ Selector ID for the procedure. */
   100,                             /* MSB of the open command. */
   1                                /* LSB of the open command. */
#elif (  HMC_I210_PLUS_C == 1 )
   STD_PROC_DIRECT_LOAD_CONTROL,    /* Disconnect Switch Procedure Number */
   PROC_STD,                        /* Standard Procedure */
   PROC_SEQ_NUM,                    /* Static sequence nmber   */
   100,                             /* New level = ON, open switch   */
   1,                               /* Device = RCDC switch */
   0, 0, 0                          /* Unused parameters (duration)  */
#elif HMC_FOCUS
   BIG_ENDIAN_16BIT((uint16_t)(2048 + 8)),   /* Disconnect Switch Procedure Number */
   1                                         /* Open Switch Command */
#else
   0
#endif
};
static const HMC_PROTO_TableCmd tblCmdSwOpenProc_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (tblSwProcedure_ ),      ( uint8_t far * )&tblSwProcedure_[0]},
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (openSwProcedureData_ ), ( uint8_t far * )&openSwProcedureData_[0]},
   { HMC_PROTO_MEM_NULL}
#elif HMC_FOCUS
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (tblSwProcedure_ ), ( uint8_t far * ) & tblSwProcedure_[0]},
   { HMC_PROTO_MEM_PGM, sizeof (openSwProcedureData_ ), ( uint8_t far * ) & openSwProcedureData_[0]},
   { HMC_PROTO_MEM_NULL}
#else
   { HMC_PROTO_MEM_NULL}
#endif
};
static const HMC_PROTO_Table tblSwOpen_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { tblCmdSwOpenProc_},
   { tblCmdSwRes_},
   { NULL}
#elif HMC_FOCUS
   { tblCmdSwOpenProc_},
   { tblCmdSwRes_},
   { NULL}
#else
   { NULL }
#endif
};
/* ------------------------------------------------------------------------------------------------------------------ */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Switch Close Commands">
/* ------------------------------------------------------------------------------------------------------------------ */
/* Switch Close Commands */

static const uint8_t closeSwProcedureData_[] =
{
#if ( HMC_I210_PLUS == 1 )
   PROC_STD,                        /* Standard Procedure */
   STD_PROC_DIRECT_LOAD_CONTROL,    /* Disconnect Switch Procedure Number */
   PROC_SEQ_NUM,                    /* G+ Selector ID for the procedure. */
   0,                               /* MSB of the close command. */
   1                                /* LSB of the close command. */
#elif ( HMC_I210_PLUS_C == 1 )
   STD_PROC_DIRECT_LOAD_CONTROL,    /* Disconnect Switch Procedure Number */
   PROC_STD,                        /* Standard Procedure */
   PROC_SEQ_NUM,                    /* Static sequence nmber*/
   0,                               /* New level = OFF, close switch   */
   1,                               /* Device = RCDC switch */
   0, 0, 0                          /* Unused parameters (duration)  */
#elif HMC_FOCUS
   BIG_ENDIAN_16BIT((uint16_t)(2048 + 8)),   /* Disconnect Switch Procedure Number */
   0                                         /* Close Switch Command. */
#else
   0
#endif
};
static const HMC_PROTO_TableCmd tblCmdSwCloseProc_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (tblSwProcedure_ ),       ( uint8_t far * ) & tblSwProcedure_[0]},
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (closeSwProcedureData_ ), ( uint8_t far * ) & closeSwProcedureData_[0]},
   { HMC_PROTO_MEM_NULL}
#elif HMC_FOCUS
   { HMC_PROTO_MEM_PGM, ( uint8_t )sizeof (tblSwProcedure_ ), ( uint8_t far * ) & tblSwProcedure_[0]},
   { HMC_PROTO_MEM_PGM, sizeof (closeSwProcedureData_ ), ( uint8_t far * ) & closeSwProcedureData_[0]},
   { HMC_PROTO_MEM_NULL}
#else
   { HMC_PROTO_MEM_NULL}
#endif
};
static const HMC_PROTO_Table tblSwClose_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { &tblCmdSwCloseProc_[0]},    /* Write ST 7 with procedure 21 */
   { &tblCmdSwRes_[0]},          /* Read the status from ST 8  */
   { NULL}
#elif HMC_FOCUS
   { &tblCmdSwCloseProc_[0]},
   { &tblCmdSwRes_[0]},
   { NULL}
#else
   { NULL}
#endif
};
/* ------------------------------------------------------------------------------------------------------------------ */
// </editor-fold>

/* --------------Update Commands for Register #515 - Service Disconnect Switch Activity Counter ---------------*/
// <editor-fold defaultstate="collapsed" desc="G+ Update Register #515 Protocol Commands">
//lint -e{648,572,845}
static const tReadPartial readReg515Tbl_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { /* This does not exist in the I210+ - maintained as part of normal switch operation in Aclara f/w   */
      0
   }
#elif HMC_FOCUS
   { /* Service Disconnect Switch Status (Information is for Register #515) */
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT( (uint16_t) (2048 + 12)),
   {
      (uint8_t) 0, (uint8_t) 0, (uint8_t) 10},
      BIG_ENDIAN_16BIT( (uint16_t) 2 )
   }
#else
   0
#endif
};
/* Read MT117  */
//lint -e{648,572,845}
static const tReadPartial readTblMT117_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   {  /* Read MT 117 - RCDC status */
      CMD_TBL_RD_PARTIAL,
      BIG_ENDIAN_16BIT( (uint16_t) (2048 + 117) ),
      { (uint8_t) 0, (uint8_t) 0, (uint8_t) 0 },
      BIG_ENDIAN_16BIT( (uint16_t)4 )
   }
#else
   {
      0
   }
#endif
};
static const  HMC_PROTO_TableCmd tblMT117UpdateGplus_[] =
{
   { HMC_PROTO_MEM_PGM,    (uint8_t)sizeof(readTblMT117_), (uint8_t far *)&readTblMT117_[0]},
   { HMC_PROTO_MEM_NULL,   HMC_PROTO_MEM_NULL,           NULL  }
};

static const HMC_PROTO_Table readTblMT117[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { tblMT117UpdateGplus_},
   NULL
#else
   NULL
#endif
};


/* ---------------------Update Register #515 Commands--------------------- */
// <editor-fold defaultstate="collapsed" desc="Update Registers #515 Protocol Commands">
static const HMC_PROTO_TableCmd tblReg515Update_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { HMC_PROTO_MEM_NULL }
#elif HMC_FOCUS
   { HMC_PROTO_MEM_PGM, (uint8_t)sizeof (readReg515Tbl_), (uint8_t far *) &readReg515Tbl_[0]},
   /* Fill in the 1st two bytes of the register with zeros and put the data read from mtr in the 2nd two bytes. */
   {( HMC_PROTO_MEM_REG | HMC_PROTO_MEM_WRITE ), 2, (uint8_t *)REG_DS_STATUS},
   { HMC_PROTO_MEM_NULL }
#else
   { HMC_PROTO_MEM_NULL }
#endif
};

/* ---------------------Update Register #512 Commands--------------------- */
// <editor-fold defaultstate="collapsed" desc="Update Registers #512 Protocol Commands">
//lint -e{648,572,845}
static const tReadPartial readReg512Tbl_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { /* Service Disconnect Switch Status (Information is for Register #512) */
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT( (uint16_t) (2048 + 115) ),       /* Mfg. 115 - Load Control Status Table */
   {
      (uint8_t) 0, (uint8_t) 0, (uint8_t) 0},   /* Offset */
      BIG_ENDIAN_16BIT( (uint16_t) 24 )         /* Length */
   }
#elif HMC_FOCUS
   { /* Service Disconnect Switch Status (Information is for Register #512) */
   CMD_TBL_RD_PARTIAL,
   BIG_ENDIAN_16BIT( (uint16_t) (2048 + 12) ),
   {
      (uint8_t) 0, (uint8_t) 0, (uint8_t) 5},
      BIG_ENDIAN_16BIT( (uint16_t) 1 )
   }
#else
   0
#endif
};
static const HMC_PROTO_TableCmd tblReg512Update_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
/* Memory Type to ,           Number of bytes,                    Pointer to the data
   Read/Write to/from         to read/write                       to read/write to     */
   { HMC_PROTO_MEM_PGM,       (uint8_t)sizeof(readReg512Tbl_),    (uint8_t far *)&readReg512Tbl_[0]},
   { HMC_PROTO_MEM_NULL,      HMC_PROTO_MEM_NULL,                 NULL  }
#elif HMC_FOCUS
   { HMC_PROTO_MEM_PGM,       ( uint8_t )sizeof(readReg512Tbl_),  ( uint8_t far * ) & readReg512Tbl_[0]},
   { HMC_PROTO_MEM_NULL}
#else
   { HMC_PROTO_MEM_NULL}
#endif
};
static const HMC_PROTO_Table tblReg512Update[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { tblReg512Update_}, /* Updates register #512 */
   { NULL}
#elif HMC_FOCUS
   { tblReg512Update_}, /* Updates register #512 */
   { NULL}
#else
   { NULL}
#endif
};
// </editor-fold>

/* MT reads */
static const HMC_PROTO_Table tblReg515Update[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { NULL}
#elif HMC_FOCUS
   { tblReg515Update_}, /* Updates register #515 */
   { NULL}
#else
   NULL
#endif
};

// </editor-fold>
/* ------------------------------------------------------------------------------------------------------------------ */
/* Switch Arm to Close Commands */

#if HMC_FOCUS
static const uint8_t armCloseSwProcedureData_[] =
{
   BIG_ENDIAN_16BIT((uint16_t)(2048 + 8)),   /* Disconnect Switch Procedure Number */
   2                                         /* Arm Switch Command. */
};
static const HMC_PROTO_TableCmd tblCmdSwArmCloseProc_[] =
{
   { HMC_PROTO_MEM_PGM, (uint8_t)sizeof (tblSwProcedure_ ),           (uint8_t far *) & tblSwProcedure_[0]},
   { HMC_PROTO_MEM_PGM, (uint8_t)sizeof (armCloseSwProcedureData_ ),  (uint8_t far *) & armCloseSwProcedureData_[0]},
   { HMC_PROTO_MEM_NULL}
};
static const HMC_PROTO_Table tblSwArmClose_[] =
{
   { tblCmdSwArmCloseProc_},
   { tblCmdSwRes_},
   { NULL}
};
#endif
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Command Sequences">
/* ------------------------------------------------------------------------------------------------------------------ */
/* Command Sequences */

// <editor-fold defaultstate="collapsed" desc="Table Definitions">

#ifdef DS_ARM_TO_OPEN_SUPPORT
static const cmdFnct_t armToOpenSwitchTable_[] =
{
   { _Tbl_SwArmOpen, procResultsSwitchCmd },
   { &tblReg512Update[0], processResultsSwitchState },
   { NULL, NULL}
};
#endif

static const cmdFnct_t regUpdateTable_[] =
{
#if ( ( HMC_I210_PLUS == 1 ) || ( HMC_I210_PLUS_C == 1 ) )
   { (HMC_PROTO_Table  const *)&readTblMT117[0],      (fptr_ds)ReadMT117Results },
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchState },
   { NULL, NULL}
#elif HMC_FOCUS
   { (HMC_PROTO_Table  const *)&tblReg512Update[0],   (fptr_ds)processResultsSwitchState },
#if 0 // TODO - fix when support for FOCUS needed
   { &tblReg515Update[0], procResultsReg515},
   { _Tbl_Reg1900Update, procResultsReg1900},
#endif
   { NULL, NULL}
#else
   { NULL, NULL}
#endif
};

// </editor-fold>

#ifdef DS_ARM_TO_OPEN_SUPPORT
static const cmdFnct_t armToOpenSwitchTable_[] =
{
#if 0 // TODO - add/fix if we support the Focus meter
   { _Tbl_SwArmOpen, procedureResultsSwitchCmd},
   { &tblReg512Update[0], processResultsSwitchState},
   { &tblReg515Update[0], procResultsReg515},
   { updateReg1900, procResultsReg1900},
#endif
   { NULL, NULL}
};
#endif

#ifdef DS_ARM_TO_CLOSE_SUPPORT
static const cmdFnct_t const armToCloseSwitchTable_[] =
{
#if 0 // TODO - add/fix if we support the Focus meter
   { tblSwArmClose_, procedureResultsSwitchCmd},
   { &tblReg512Update[0], processResultsReg512},
   { &tblReg515Update[0], procResultsReg515},
   { _Tbl_Reg1900Update, procResultsReg1900},
#endif
   { NULL, NULL}
};
#endif

// </editor-fold>

#ifdef DS_ARM_TO_CLOSE_SUPPORT
#if HMC_FOCUS
static const cmdFnct_t * const pArmToCloseSwitchTables_[] =
{
   &armToCloseSwitchTable_[0]
};
#endif
#endif

static const cmdFnct_t * const pRegUpdateTables_[] =
{
#if HMC_I210_PLUS || HMC_FOCUS || HMC_I210_PLUS_C
   &regUpdateTable_[0]
#else
   NULL
#endif
};

/* ------------------------------------------------------------------------------------------------------------------ */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="The values below have to compensate for every value of the host meter">
/* ---------- The values below have to compensate for every value of the host meter ---------- */
/*         If the host meter has 3 bits for a value, then 8 values have to be defined          */
#if HMC_FOCUS
/* Switch position from the host meter, the order is from the meter, the value is according to the PDS */
static const uint8_t SWITCH_POSITION[] =
{
   SWITCH_STATE_CLOSED,
   SWITCH_STATE_OPEN,
   SWITCH_STATE_OPEN,
   SWITCH_STATE_OPEN
};

/* Switch armed from the host meter, the order is from the meter, the value is according to the PDS */
static const uint8_t SWITCH_ARMED[] =
{
   SWITCH_STATE_UNARMED,
   SWITCH_STATE_UNARMED,
   SWITCH_STATE_ARMED,
   SWITCH_STATE_UNARMED
};

/* Last operation attempt from the host meter, the order is from the meter, the value is according to the PDS */
static const uint8_t LAST_OPERATION_ATTEMPT[] =
{
   LAST_OP_ATTEMPTED_CLOSE,
   LAST_OP_ATTEMPTED_OPEN,
   LAST_OP_ATTEMPTED_ARM_CLOSE,
   LAST_OP_ATTEMPTED_ARM_CLOSE
};


/* Last operation source from the host meter, the order is from the meter, the value is according to the PDS */
static const uint8_t LAST_OPERATION_SOURCE[] =
{
   LAST_OP_SOURCE_CMD,
   LAST_OP_SOURCE_AUTO,
   LAST_OP_SOURCE_TOU,
   LAST_OP_SOURCE_MANUAL,
   LAST_OP_SOURCE_OVER_UNDER,
   LAST_OP_SOURCE_SD_CONTACT,
   LAST_OP_SOURCE_DLP,
   LAST_OP_SOURCE_ECP,
   LAST_OP_SOURCE_PPM
};


/* Last operation source from the host meter, the order is from the meter, the value is according to the PDS */
static const uint8_t LAST_SWITCH_VIA_PB[] =
{
   LAST_SWITCH_VIA_PB_FALSE,
   LAST_SWITCH_VIA_PB_FALSE,
   LAST_SWITCH_VIA_PB_FALSE,
   LAST_SWITCH_VIA_PB_TRUE,
   LAST_SWITCH_VIA_PB_FALSE,
   LAST_SWITCH_VIA_PB_FALSE,
   LAST_SWITCH_VIA_PB_FALSE,
   LAST_SWITCH_VIA_PB_FALSE
};
// </editor-fold>
#endif
#endif

#undef HMC_DS_EXTERN
#endif
