// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************

   Filename:   hmc_eng.c

   Global Designator: HMC_ENG_

   Contents: Handles engineering registers. The code below is designed for a little endian machine where bits in
             structures start with bit 0 first. Register #124 may be updated frequently in NV memory. High endurance
             NV memory must be used!

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2021 Aclara. All Rights Reserved.
   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara). This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara. Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   $Log$ KAD Created 2008, 10/8

 ***********************************************************************************************************************
   Revision History:
   v0.1 - Initial Release
   v0.2 - Organized the registers per new defintion
   v0.3 - Added register #35 Port 0 Flag
   v0.4 -  9/29/2011 - KAD - To limit external NV access, register #1511 was moved to RAM and is stored/restored at
                             power down/up. This is a work-around to the Rev D hardware issue where SPI bus runs next
                             to the Vsig. (PCB P/N Y72841, Rev D)
   v0.5 - 11/16/2011 - KAD - Added call to meter start to indicate an ISC error. This will cause a lock-out condition
                             where HMC will not communicate with the host meter for nnn time. For example, HMC will be
                             lock-out for 10 minutes for a Focus meter.
   v0.6 - 04/03/2012 - KAD - Changed the 2nd parameter of HMC_ENG_Execute to allow the function to determine if a table
                             error or procedure error occurred. If an error occurred, register #124 is updated. This
                             version may write frequently to NV memory; high endurance NV must be used.
 ***********************************************************************************************************************
   The following must be defined in the nonvolatile.h file:
   Host Meter Communication Engineering Registers
   uint8_t MtrComExecptionCnts[9];
   uint8_t HostMtrCommPerfSummary[9];
   uint8_t ANSIRspErrorCodes[7];
   uint8_t MtrComErrCntSummary[6];
 ******************************************************************************************************************** */
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdbool.h>
#include <limits.h>
#include "psem.h"
#include "hmc_app.h"
#include "hmc_eng.h"
#include "hmc_start.h"
#include "ansi_tables.h"
#include "meter_ansi_sig.h"
#include "byteswap.h"
#include "EVL_event_log.h"

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="Register 1510.1 - HostMtrCommExceptionCnts">

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="errorType_e - Error Type for Reg #124">
typedef enum
{
   eNoError = ( uint8_t ) 0,           /* No Errors, don't change register #124. */
   eTblError,                          /* ANSI Table Error - Update reg #124 with table information. */
   eProcError                          /* Procedure Error - Update register #124 with procedure information. */
} errorType_e;                        /* Used to set the error type so that register #124 is populated correctly. */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Variable Definitions (File Static)">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static    FileHandle_t  mtrEngFileHndl_ = { 0 };     /* Contains the file handle information. */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#undef   NULL
#define  NULL                          0

#define MAX_CONSECUTIVE_ABORTS         ((uint8_t)5)   /* Max consecutive aborts before setting P0 Error in Reg #35 */
#define HMC_ENG_FILE_UPDATE_RATE       ((uint32_t)0)  /* Updates very often, several times a second. */
#define NIBBLE_SIZE_BITS               ((uint8_t)4)   /* Size of a nibble in bits. */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */

#if 0 //XXX

static const int8_t regExceptionDecoder[] = { 16, 8, 16, 4, 4, 8, 16, 0 };
static const int8_t regHostMtrCommPerfSumDecoder[] = { 16, 24, 32, 0 };
static const int8_t regAnsiCResponseCodeErrsDecoder[] = { 8, 8, 8, 8, 4, 4, 4, 4, 4, 4, 0 };
static const int8_t regHostMtrCommErrCntSum[] = { 16, 16, 4, 4, 4, 4, 0 };

//Todo:  Create decoder for ansiErrorReg_t
//Todo:  Add decoder for Register #124 Definition

static const tRegDefintion mtrEngRegDefTbl_[] = /* Define registers that belong to the time module. */
{
   // <editor-fold defaultstate="collapsed" desc="1510 - REG_HMC_EXCEPTION">
   {
      { /* HMC Exception Counts */
         REG_HMC_EXCEPTION,                              /* Reg Number */
         REG_HMC_EXCEPTION_SIZE,                         /* Reg Length */
         REG_MEM_FILE,                                   /* Memory Type */
         ( uint8_t * )&mtrEngFileHndl_,                  /* Address of Register */
//XXX         offsetof(mtrEngFileData_t, exeCnts),       /*lint !e718 !e746 !e734 !e413 !e516 Offset of the register */
         0, //XXX
         ( uint64_t ) 0,                                 /* Minimum Value contained in this register */
         ( uint64_t ) 0,                                 /* Maximum Value contained in this register */
         ( int8_t * )&regceptionDecoder[0],              /* pointer to decoder. */
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* Initial Value */
         REG_PROP_NATIVE_FMT,                            /* Register is stored in native format. */
         REG_PROP_WR_TWACS,                              /* Writable over TWACS/STAR (Main Network)? */
         REG_PROP_WR_MFG,                                /* Writable over MFG? */
         REG_PROP_WR_PERIF,                              /* Writable over Perif */
         REG_PROP_RD_TWACS,                              /* Readable over TWACS/STAR (Main Network)? */
         REG_PROP_RD_MFG,                                /* Readable over MFG? */
         REG_PROP_RD_PERIF,                              /* Readable over Perif? */
         !REG_PROP_SECURE,                               /* Secure? */
         !REG_PROP_MANIPULATE,                           /* Manipulate? */
         !REG_PROP_CHECKSUM,                             /* Checksum? */
         !REG_PROP_MATH,                                 /* Math - Add? */
         !REG_PROP_MATH_ROLL,                            /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                               /* Signed value? */
         !REG_PROP_USE_MIN_MAX,                          /* use the min and max values when writing? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,            /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,           /* Set default at every power up? */
         !REG_PROP_USE_DEFAULT_CMD_1,                    /* Set default at cmd 1? */
         !REG_PROP_USE_DEFAULT_CMD_2,                    /* Set default at cmd 2? */
         !REG_PROP_USE_DEFAULT_CMD_3,                    /* Set default at cmd 3? */
      },
      REG_DEF_NULL_PRE_READ_EXE,                         /* Function Ptr: Executes before data is read. */
      REG_DEF_NULL_PRE_WRITE,                            /* Function Ptr: Execute before data is written. */
      REG_DEF_NULL_POST_WRITE,                           /* Function Ptr: Executes after data is written. */
      REG_DEF_NULL_UPDATE,                               /* Function pointer to the update function */
      REG_DEF_NULL_REG_ACCESS                            /* Function pointer to the Access function */
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="1511 - REG_HMC_PERF_SUMMARY">
   {
      { /* HMC Performance Summary */
         REG_HMC_PERF_SUMMARY,                           /* Reg Number */
         REG_HMC_PERF_SUMMARY_SIZE,                      /* Reg Length */
         REG_MEM_FILE,                                   /* Memory Type */
         ( uint8_t * )&mtrEngFileHndl_,                  /* Address of Register */
//XXX         offsetof(mtrEngFileData_t, perfSum),       /*lint !e718 !e746 !e734 !e516 !e413 Offset of the register */
         0, //XXX
         ( uint64_t ) 0,                                 /* Minimum Value contained in this register */
         ( uint64_t ) 0,                                 /* Maximum Value contained in this register */
         &regHostMtrCommPerfSumDecoder[0],               /* pointer to decoder. */
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* Initial Value */
         REG_PROP_NATIVE_FMT,                            /* Register is stored in native format. */
         REG_PROP_WR_TWACS,                              /* Writable over TWACS/STAR (Main Network)? */
         REG_PROP_WR_MFG,                                /* Writable over MFG? */
         REG_PROP_WR_PERIF,                              /* Writable over Perif */
         REG_PROP_RD_TWACS,                              /* Readable over TWACS/STAR (Main Network)? */
         REG_PROP_RD_MFG,                                /* Readable over MFG? */
         REG_PROP_RD_PERIF,                              /* Readable over Perif? */
         !REG_PROP_SECURE,                               /* Secure? */
         !REG_PROP_MANIPULATE,                           /* Manipulate? */
         !REG_PROP_CHECKSUM,                             /* Checksum? */
         !REG_PROP_MATH,                                 /* Math - Add? */
         !REG_PROP_MATH_ROLL,                            /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                               /* Signed value? */
         !REG_PROP_USE_MIN_MAX,                          /* use the min and max values when writing? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,            /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,           /* Set default at every power up? */
         !REG_PROP_USE_DEFAULT_CMD_1,                    /* Set default at cmd 1? */
         !REG_PROP_USE_DEFAULT_CMD_2,                    /* Set default at cmd 2? */
         !REG_PROP_USE_DEFAULT_CMD_3,                    /* Set default at cmd 3? */
      },
      REG_DEF_NULL_PRE_READ_EXE,                         /* Function Ptr: Executes before data is read. */
      REG_DEF_NULL_PRE_WRITE,                            /* Function Ptr: Execute before data is written. */
      REG_DEF_NULL_POST_WRITE,                           /* Function Ptr: Executes after data is written. */
      REG_DEF_NULL_UPDATE,                               /* Function pointer to the update function */
      REG_DEF_NULL_REG_ACCESS                            /* Function pointer to the Access function */
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="1526 - REG_HMC_ANSI_RESP_ERRORS">
   {
      { /* HMC ANSI Response Error Codes */
         REG_HMC_ANSI_RESP_ERRORS,                       /* Reg Number */
         REG_HMC_ANSI_RESP_ERRORS_SIZE,                  /* Reg Length */
         REG_MEM_FILE,                                   /* Memory Type */
         ( uint8_t * )&mtrEngFileHndl_,                  /* Address of Register */
//XXX         offsetof(mtrEngFileData_t, ansiResp),      /*lint !e718 !e746 !e734 !e516 !e413 Offset of the register */
         0, //XXX
         ( uint64_t ) 0,                                 /* Minimum Value contained in this register */
         ( uint64_t ) 0,                                 /* Maximum Value contained in this register */
         &regAnsiCResponseCodeErrsDecoder[0],            /* pointer to decoder. */
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* Initial Value */
         REG_PROP_NATIVE_FMT,                            /* Register is stored in native format. */
         REG_PROP_WR_TWACS,                              /* Writable over TWACS/STAR (Main Network)? */
         REG_PROP_WR_MFG,                                /* Writable over MFG? */
         REG_PROP_WR_PERIF,                              /* Writable over Perif */
         REG_PROP_RD_TWACS,                              /* Readable over TWACS/STAR (Main Network)? */
         REG_PROP_RD_MFG,                                /* Readable over MFG? */
         REG_PROP_RD_PERIF,                              /* Readable over Perif? */
         !REG_PROP_SECURE,                               /* Secure? */
         !REG_PROP_MANIPULATE,                           /* Manipulate? */
         !REG_PROP_CHECKSUM,                             /* Checksum? */
         !REG_PROP_MATH,                                 /* Math - Add? */
         !REG_PROP_MATH_ROLL,                            /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                               /* Signed value? */
         !REG_PROP_USE_MIN_MAX,                          /* use the min and max values when writing? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,            /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,           /* Set default at every power up? */
         !REG_PROP_USE_DEFAULT_CMD_1,                    /* Set default at cmd 1? */
         !REG_PROP_USE_DEFAULT_CMD_2,                    /* Set default at cmd 2? */
         !REG_PROP_USE_DEFAULT_CMD_3,                    /* Set default at cmd 3? */
      },
      REG_DEF_NULL_PRE_READ_EXE,                         /* Function Ptr: Executes before data is read. */
      REG_DEF_NULL_PRE_WRITE,                            /* Function Ptr: Execute before data is written. */
      REG_DEF_NULL_POST_WRITE,                           /* Function Ptr: Executes after data is written. */
      REG_DEF_NULL_UPDATE,                               /* Function pointer to the update function */
      REG_DEF_NULL_REG_ACCESS                            /* Function pointer to the Access function */
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="1527 - REG_HMC_SUMMARY_ERRORS">
   {
      { /* HMC Error Count Summary */
         REG_HMC_SUMMARY_ERRORS,                         /* Reg Number */
         REG_HMC_SUMMARY_ERRORS_SIZE,                    /* Reg Length */
         REG_MEM_FILE,                                   /* Memory Type */
         ( uint8_t * )&mtrEngFileHndl_,                  /* Address of Register */
//XXX         offsetof(mtrEngFileData_t, errCntSum),     /*lint !e718 !e746 !e734 !e516 !e413 Offset of the register */
         0, //XXX
         ( uint64_t ) 0,                                 /* Minimum Value contained in this register */
         ( uint64_t ) 0,                                 /* Maximum Value contained in this register */
         &regHostMtrCommErrCntSum[0],                    /* pointer to decoder. */
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* Initial Value */
         REG_PROP_NATIVE_FMT,                            /* Register is stored in native format. */
         REG_PROP_WR_TWACS,                              /* Writable over TWACS/STAR (Main Network)? */
         REG_PROP_WR_MFG,                                /* Writable over MFG? */
         REG_PROP_WR_PERIF,                              /* Writable over Perif */
         REG_PROP_RD_TWACS,                              /* Readable over TWACS/STAR (Main Network)? */
         REG_PROP_RD_MFG,                                /* Readable over MFG? */
         REG_PROP_RD_PERIF,                              /* Readable over Perif? */
         !REG_PROP_SECURE,                               /* Secure? */
         !REG_PROP_MANIPULATE,                           /* Manipulate? */
         !REG_PROP_CHECKSUM,                             /* Checksum? */
         !REG_PROP_MATH,                                 /* Math - Add? */
         !REG_PROP_MATH_ROLL,                            /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                               /* Signed value? */
         !REG_PROP_USE_MIN_MAX,                          /* use the min and max values when writing? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,            /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,           /* Set default at every power up? */
         !REG_PROP_USE_DEFAULT_CMD_1,                    /* Set default at cmd 1? */
         !REG_PROP_USE_DEFAULT_CMD_2,                    /* Set default at cmd 2? */
         !REG_PROP_USE_DEFAULT_CMD_3,                    /* Set default at cmd 3? */
      },
      REG_DEF_NULL_PRE_READ_EXE,                         /* Function Ptr: Executes before data is read. */
      REG_DEF_NULL_PRE_WRITE,                            /* Function Ptr: Execute before data is written. */
      REG_DEF_NULL_POST_WRITE,                           /* Function Ptr: Executes after data is written. */
      REG_DEF_NULL_UPDATE,                               /* Function pointer to the update function */
      REG_DEF_NULL_REG_ACCESS                            /* Function pointer to the Access function */
   }, // </editor-fold>
   // <editor-fold defaultstate="collapsed" desc="124  - ANSI Errors">
   {
      { /* HMC Error Count Summary */
         REG_HMC_ANSI_ERRORS,                            /* Reg Number */
         REG_HMC_ANSI_ERRORS_SIZE,                       /* Reg Length */
         REG_MEM_FILE,                                   /* Memory Type */
         ( uint8_t * )&mtrEngFileHndl_,                  /* Address of Register */
//XXX         offsetof(mtrEngFileData_t, ansiErr),       /*lint !e718 !e746 !e734 !e516 !e413 Offset of the register */
         0, //XXX
         ( uint64_t ) 0,                                 /* Minimum Value contained in this register */
         ( uint64_t ) 0,                                 /* Maximum Value contained in this register */
         ( int8_t * )NULL,                               /* pointer to decoder. */
         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /* Initial Value */
         !REG_PROP_NATIVE_FMT,                           /* Register is stored in native format. */
         REG_PROP_WR_TWACS,                              /* Writable over TWACS/STAR (Main Network)? */
         REG_PROP_WR_MFG,                                /* Writable over MFG? */
         REG_PROP_WR_PERIF,                              /* Writable over Perif */
         REG_PROP_RD_TWACS,                              /* Readable over TWACS/STAR (Main Network)? */
         REG_PROP_RD_MFG,                                /* Readable over MFG? */
         REG_PROP_RD_PERIF,                              /* Readable over Perif? */
         !REG_PROP_SECURE,                               /* Secure? */
         !REG_PROP_MANIPULATE,                           /* Manipulate? */
         !REG_PROP_CHECKSUM,                             /* Checksum? */
         !REG_PROP_MATH,                                 /* Math - Add? */
         !REG_PROP_MATH_ROLL,                            /* Inc/Dec roll over/under? */
         !REG_PROP_SIGNED,                               /* Signed value? */
         !REG_PROP_USE_MIN_MAX,                          /* use the min and max values when writing? */
         REG_PROP_USE_DEFAULT_FIRST_POWER_UP,            /* Set default at 1st time ever power up? */
         !REG_PROP_USE_DEFAULT_EVERY_POWER_UP,           /* Set default at every power up? */
         !REG_PROP_USE_DEFAULT_CMD_1,                    /* Set default at cmd 1? */
         !REG_PROP_USE_DEFAULT_CMD_2,                    /* Set default at cmd 2? */
         !REG_PROP_USE_DEFAULT_CMD_3,                    /* Set default at cmd 3? */
      },
      REG_DEF_NULL_PRE_READ_EXE,                         /* Function Ptr: Executes before data is read. */
      REG_DEF_NULL_PRE_WRITE,                            /* Function Ptr: Execute before data is written. */
      REG_DEF_NULL_POST_WRITE,                           /* Function Ptr: Executes after data is written. */
      REG_DEF_NULL_UPDATE,                               /* Function pointer to the update function */
      REG_DEF_NULL_REG_ACCESS                            /* Function pointer to the Access function */
   } // </editor-fold>
};

/* Define the table that will be used for the object list entry. */
static const OL_tTableDef mtrEngRegTbl_ =
{
   /*lint -e(740) ususual cast */
   ( tElement  * )&mtrEngRegDefTbl_[0].sRegProp.uiRegNumber, /* Points to the 1st element in the register table. */
   ARRAY_IDX_CNT( mtrEngRegDefTbl_ )                       /* Number of registers defined for the time module. */
};
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static uint32_t incVar( uint32_t data,
                        uint8_t bits ); /* Increments a variable of n size without rolling over (overflow) */

static uint32_t incVarWithRollOver( uint32_t data, uint8_t bits ); /* increment a variable with roll over */

// </editor-fold>
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="returnStatus_t HMC_ENG_init(void)">
/***********************************************************************************************************************

   Function name: HMC_ENG_init

   Purpose: Adds registers to the Object list and opens files. This must be called at power before any other function
            in this module is called and before the register module is initialized.

   Arguments: None

   Returns: returnStatus_t - result of file open.

   Re-entrant Code: No

   Notes:

 **********************************************************************************************************************/
returnStatus_t HMC_ENG_init( void )
{
   FileStatus_t fileStatus;                           /* Contains the file status */

   return( FIO_fopen( &mtrEngFileHndl_,               /* File Handle so that power quality can access the file. */
                      ePART_SEARCH_BY_TIMING,           /* Search for the best partition according to the timing. */
                      ( uint16_t )eFN_HMC_ENG,          /* File ID (filename) */
                      ( lCnt )sizeof( mtrEngFileData_t ), /* Size of the data in the file. */
                      FILE_IS_NOT_CHECKSUMED,           /* File attributes to use. */
                      HMC_ENG_FILE_UPDATE_RATE,         /* The update rate of the data in the file. */
                      &fileStatus ) );                  /* Contains the file status */
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void HMC_ENG_Execute(uint8_t ucFunction, HMC_COM_INFO *pComPacket)">
/***********************************************************************************************************************

   Function name: HMC_ENG_Execute()

   Purpose: Updates engineering registers

   Arguments: uint8_t - Function, uint8_t Data (Table Error)

   Returns: None

 **********************************************************************************************************************/
void HMC_ENG_Execute( uint8_t ucFunction, HMC_COM_INFO *pComPacket )
{
   mtrEngFileData_t fileData;

   if ( eSUCCESS == FIO_fread( &mtrEngFileHndl_, ( uint8_t * )&fileData, 0, ( lCnt )sizeof( fileData ) ) )
   {
      uint8_t           ucIncAE = false;
      uint8_t           ucIncDLE = false;
      errorType_e       errorReg = eNoError;  /* Assume no errors detected and register #124 will not be updated. */
//XXX      diagnosticReg_t   diagsMask;         /* Used to set diagnostic flags as needed. */
      tTbl8ProcResponse *pProcResp = ( void * )NULL; /* Pointer to Rx data to help decode the procedure response. */

      /* When zero is reached, set port 0 flag in Reg #35 */
      static uint8_t consecutiveAborts_ = MAX_CONSECUTIVE_ABORTS;

//XXX      diagsMask.reg = 0;

      switch ( ucFunction )
      {
         case ( uint8_t ) HMC_ENG_NAK:
         {
            fileData.exeCnts.nakr = ( uint16_t )incVar( ( uint32_t )fileData.exeCnts.nakr,
                                    CHAR_BIT * sizeof( fileData.exeCnts.nakr ) );
            ucIncDLE = true;

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            break;
         }
         case ( uint8_t ) HMC_ENG_PCKT_RESP:
         {
            if ( RESP_OK != pComPacket->RxPacket.ucResponseCode )   /* Does the ANSI response have an error? */
            {
               switch ( pComPacket->RxPacket.ucResponseCode )  /* switch on the ANSI error */
               {
                  case ( uint8_t ) RESP_ERR:
                  {
                     fileData.ansiResp.err = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.err, NIBBLE_SIZE_BITS );
                     break;
                  }
                  case ( uint8_t ) RESP_SNS:
                  {
                     fileData.ansiResp.sns = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.sns, NIBBLE_SIZE_BITS );

                     /* updated the meter session failure count parameter */
                     fileData.meterSessionFailureCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterSessionFailureCount,
                                                         CHAR_BIT * sizeof( fileData.meterSessionFailureCount ) );

                     errorReg = eTblError;
                     break;
                  }
                  case ( uint8_t ) RESP_ISC:
                  {
                     fileData.ansiResp.isc = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.isc, NIBBLE_SIZE_BITS );

                     /* updated the meter session failure count parameter */
                     fileData.meterSessionFailureCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterSessionFailureCount,
                                                         CHAR_BIT * sizeof( fileData.meterSessionFailureCount ) );

                     errorReg = eTblError;
                     HMC_STRT_ISC();   /* Notify meter start applet. For Focus, this causes 10 minutes of no comm. */
                     // Detect initial occurance of ISC error and log event
                     // Only occurs when accessing tables where a password is required.
                     if ( !fileData.uErrBits.Bits.ISCErr ) // Is this the first time error seen?
                     {
                        EventData_s          eventData;     /* Event info  */
                        EventKeyValuePair_s  keyVal;        /* Event key/value pair info  */
                        fileData.uErrBits.Bits.ISCErr  = true;

                        ( void )memset( ( uint8_t * )&eventData, 0, sizeof( eventData ) );
                        ( void )memset( ( uint8_t * )&keyVal, 0, sizeof( eventData ) );
                        eventData.markSent = (bool)false;
                        eventData.eventId  = (uint16_t)comDeviceSecurityPasswordInvalid;
                        (void)EVL_LogEvent( 91, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
                     }
                     break;
                  }
                  case ( uint8_t ) RESP_ONP:
                  {
                     fileData.ansiResp.onp = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.onp,
                                             CHAR_BIT * sizeof( fileData.ansiResp.onp ) );

                     /* update the meter session failure count parameter */
                     fileData.meterSessionFailureCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterSessionFailureCount,
                                                         CHAR_BIT * sizeof( fileData.meterSessionFailureCount ) );

                     errorReg = eTblError;
                     break;
                  }
                  case ( uint8_t ) RESP_IAR:
                  {
                     fileData.ansiResp.iar = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.iar,
                                             CHAR_BIT * sizeof( fileData.ansiResp.iar ) );

                     /* update the meter session failure count parameter */
                     fileData.meterSessionFailureCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterSessionFailureCount,
                                                         CHAR_BIT * sizeof( fileData.meterSessionFailureCount ) );

                     errorReg = eTblError;
                     break;
                  }
                  case ( uint8_t ) RESP_BSY:
                  {
                     fileData.ansiResp.bsy = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.bsy,
                                             CHAR_BIT * sizeof( fileData.ansiResp.bsy ) );
                     break;
                  }
                  case ( uint8_t ) RESP_DNR:
                  {
                     fileData.ansiResp.dnr = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.dnr, NIBBLE_SIZE_BITS );
                     break;
                  }
                  case ( uint8_t ) RESP_DLK:
                  {
                     fileData.ansiResp.dlk = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.dlk, NIBBLE_SIZE_BITS );
                     break;
                  }
                  case ( uint8_t ) RESP_RNO:
                  {
                     fileData.ansiResp.rno = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.rno, NIBBLE_SIZE_BITS );
                     break;
                  }
                  case ( uint8_t ) RESP_ISSS:
                  {
                     fileData.ansiResp.isss = ( uint8_t )incVar( ( uint32_t )fileData.ansiResp.isss,
                                              CHAR_BIT * sizeof( fileData.ansiResp.isss ) );
                     break;
                  }
                  default:
                  {
                     break;
                  }
               }
               fileData.errCntSum.ansie = ( uint16_t )incVar( ( uint32_t )fileData.errCntSum.ansie,
                                          CHAR_BIT * sizeof( fileData.errCntSum.ansie ) );
            }
            else
            {
               /* If comm OK after previous "Insufficient Security Clearance" AND
                   a Table Rd/Wr command AND
                   a table with access requirements
               */
               if ( fileData.uErrBits.Bits.ISCErr )
               {
                  bool     sendConfirmEvent;
                  uint16_t tableID;     // Need to use little endian vs PSEM's big endian w/o changing data in buffer

                  tableID = pComPacket->TxPacket.uTxData.sReadFull.uiTbleID;  //Table ID is in same location for all Read and Write commands
                  Byte_Swap( ( uint8_t * ) &tableID, sizeof( tableID ) );     // Table ID in msg vs NoPrivRqdTables are swapped
                  DBG_logPrintf ('I', "No Error after ISCErr set, tableID: %d", tableID);
                  sendConfirmEvent = false;
                  // Only Read/Write Table commands may return ISC errors
                  if ( ( CMD_TBL_RD_FULL    <= pComPacket->TxPacket.uTxData.sReadFull.ucServiceCode ) &&
                       ( CMD_TBL_RD_PARTIAL >= pComPacket->TxPacket.uTxData.sReadPartial.ucServiceCode ) )
                  {
                     uint8_t i;
                     //check read tables where security is required
                     for ( i = 0; i < ARRAY_IDX_CNT( rqdPrivTables ); i++ )
                     {
                        if ( rqdPrivTables[i] == tableID )
                        {
                           sendConfirmEvent = true;
                           break;
                        }
                     }
                  }

                  if ( sendConfirmEvent )
                  {
                     EventData_s          eventData;     /* Event info  */
                     EventKeyValuePair_s  keyVal;        /* Event key/value pair info  */

                     fileData.uErrBits.Bits.ISCErr = false;

                     (void)memset( ( uint8_t * )&eventData, 0, sizeof( eventData ) );
                     (void)memset( ( uint8_t * )&keyVal, 0, sizeof( keyVal ) );
                     eventData.markSent = (bool)false;
                     eventData.eventId  = (uint16_t)comDeviceSecurityPasswordConfirmed;
                     (void)EVL_LogEvent( 91, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
                  }
               }
               if ( fileData.uErrBits.Bits.Port0Err )
               {
                  EventData_s          eventData;     /* Event info  */
                  EventKeyValuePair_s  keyVal;        /* Event key/value pair info  */

                  /* Clear port 0 error bit and log event  */
                  fileData.uErrBits.Bits.Port0Err = ( bool )false;

                  (void)memset( ( uint8_t * )&eventData, 0, sizeof( eventData ) );
                  (void)memset( ( uint8_t * )&keyVal, 0, sizeof( keyVal ) );
                  eventData.markSent = (bool)false;
                  eventData.eventId  = ( uint16_t )comDeviceMetrologyIOerrorCleared;
                  ( void )EVL_LogEvent( 19, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );

                  consecutiveAborts_ = MAX_CONSECUTIVE_ABORTS;  /* Reset counter on success */
               }
            }
            break;
         }
         case ( uint8_t ) HMC_ENG_COM_TIME_OUT: /* Fall Through */
         {
            fileData.exeCnts.pto = ( uint8_t )incVar( ( uint32_t )fileData.exeCnts.pto,
                                    CHAR_BIT * sizeof( fileData.exeCnts.pto ) );

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            ucIncDLE = true;
            break;
         }
         case ( uint8_t ) HMC_ENG_TIME_OUT:
         {
            HMC_STRT_RestartDelay();
            fileData.exeCnts.retry = ( uint16_t )incVar( ( uint32_t )fileData.exeCnts.retry,
                                     CHAR_BIT * sizeof( fileData.exeCnts.retry ) );

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            break;
         }
         case ( uint8_t ) HMC_ENG_RETRY:
         {
            fileData.exeCnts.retry = ( uint16_t )incVar( ( uint32_t )fileData.exeCnts.retry,
                                     CHAR_BIT * sizeof( fileData.exeCnts.retry ) );

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            break;
         }
         case ( uint8_t ) HMC_ENG_INTER_CHAR_T_OUT: /* Fall through and increment CRC16 Count */
         case ( uint8_t ) HMC_ENG_CRC16:
         {
            fileData.exeCnts.nakt = ( uint8_t )incVar( ( uint32_t )fileData.exeCnts.nakt,
                                    CHAR_BIT * sizeof( fileData.exeCnts.nakt ) );
            ucIncDLE = true;

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            break;
         }
         case ( uint8_t ) HMC_ENG_OVERRUN:
         {
            fileData.exeCnts.or = ( uint8_t )incVar( ( uint32_t )fileData.exeCnts.or, NIBBLE_SIZE_BITS );
            ucIncDLE = true;

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            break;
         }
         case( uint8_t ) HMC_ENG_TOTAL_SESSIONS:
         {
            uint32_t sessions = 0;
            /* Take the sizeof the source size we only want to copy 3 bytes. */
            ( void )memcpy( (uint8_t *)&sessions, &fileData.perfSum.sessions[0], sizeof( fileData.perfSum.sessions[0] ) );
            sessions = incVar( sessions, CHAR_BIT * sizeof( fileData.perfSum.sessions ) );
            ( void )memcpy( &fileData.perfSum.sessions[0], (uint8_t *)&sessions, sizeof( fileData.perfSum.sessions[0] ) );
            break;
         }
         case ( uint8_t ) HMC_ENG_TOTAL_PACKETS:
         {
            fileData.perfSum.packets = incVar( fileData.perfSum.packets,
                                             CHAR_BIT * sizeof( fileData.perfSum.packets ) );
            break;
         }
         case ( uint8_t ) HMC_ENG_ABORT:
         {
            fileData.perfSum.abort = ( uint16_t )incVar( ( uint32_t )fileData.perfSum.abort,
                                     CHAR_BIT * sizeof( fileData.perfSum.abort ) );

            /* update the meter session failure count parameter */
            fileData.meterSessionFailureCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterSessionFailureCount,
                                                CHAR_BIT * sizeof( fileData.meterSessionFailureCount ) );

            HMC_STRT_RestartDelay();
            if ( consecutiveAborts_ )  /* If not zero, decrement the counter */
            {
               if ( !( --consecutiveAborts_ ) ) /* Decrement and set P0 Diags flag if zero */
               {
                  EventData_s          eventData;     /* Event info  */
                  EventKeyValuePair_s  keyVal;        /* Event key/value pair info  */

                  /* Set port 0 error bit and log event  */
                  fileData.uErrBits.Bits.Port0Err = ( bool )true;

                  (void)memset( ( uint8_t * )&eventData, 0, sizeof( eventData ) );
                  (void)memset( ( uint8_t * )&keyVal, 0, sizeof( keyVal ) );
                  eventData.markSent = (bool)false;
                  eventData.eventId  = ( uint16_t )comDeviceMetrologyIOerror;
                  ( void )EVL_LogEvent( 195, &eventData, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );
               }
            }
            break;
         }
         case ( uint8_t ) HMC_ENG_GEN_ERROR:
         {
            fileData.errCntSum.hwe = ( uint8_t )incVar( ( uint32_t )fileData.errCntSum.hwe, NIBBLE_SIZE_BITS );
            ucIncAE = true;
            break;
         }
         case ( uint8_t ) HMC_ENG_INV_REG:
         {
            fileData.errCntSum.ir = ( uint8_t )incVar( ( uint32_t )fileData.errCntSum.ir, NIBBLE_SIZE_BITS );
            ucIncAE = true;
            break;
         }
         case ( uint8_t ) HMC_ENG_RE_INIT:
         {
            break;
         }
         case ( uint8_t ) HMC_ENG_APPLET_ERROR:
         {
            ucIncAE = true;
            break;
         }
         case ( uint8_t ) HMC_ENG_APPLET_TIMER_OUT:
         {
            fileData.errCntSum.ato = ( uint8_t )incVar( ( uint32_t )fileData.errCntSum.ato, NIBBLE_SIZE_BITS );
            ucIncAE = true;
            break;
         }
         case ( uint8_t ) HMC_ENG_TOGGLE_ERROR:
         {
            HMC_STRT_RestartDelay();
            fileData.exeCnts.tog = ( uint8_t )incVar( ( uint32_t )fileData.exeCnts.tog, NIBBLE_SIZE_BITS );
            ucIncDLE = true;

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            break;
         }
         case ( uint8_t ) HMC_ENG_PROCEDURE_CHK: /* Check the procedure response to see if procedure had any errors. */
         {
            /* Set ptr to tbl data */
            pProcResp = ( tTbl8ProcResponse * )(void *)&pComPacket->RxPacket.uRxData.sTblData.sRxBuffer[0];
            if ( PROC_RESULT_COMPLETED != pProcResp->resultCode )  /* Does the result code have an error? */
            {
               errorReg = eProcError;  /* Update reg #124 with a procedure error. */
            }
            break;
         }
         case ( uint8_t ) HMC_ENG_HDWR_BSY:
         {
            uint16_t exeCnts;
            /* Take the sizeof the source size we only want to copy 2 bytes. */
            ( void )memcpy( &exeCnts, &fileData.exeCnts.hwb, sizeof( fileData.exeCnts.hwb ) );
            exeCnts++;
            ( void )memcpy( &fileData.exeCnts.hwb, &exeCnts, sizeof( fileData.exeCnts.hwb ) );

            /* update the meter communication lockout count parameter */
            fileData.meterCommunicationLockoutCount = ( uint16_t )incVarWithRollOver( ( uint32_t )fileData.meterCommunicationLockoutCount,
                                                CHAR_BIT * sizeof( fileData.meterCommunicationLockoutCount ) );

            break;
         }
         default:
         {
            break;
         }
      }
      if ( true == ucIncAE )
      {
         fileData.errCntSum.ae = ( uint8_t )incVar( ( uint32_t )fileData.errCntSum.ae,
                                                   NIBBLE_SIZE_BITS ); /*lint !e571 */
      }
      if ( true == ucIncDLE )
      {
         fileData.errCntSum.dle = ( uint8_t )incVar( ( uint32_t )fileData.errCntSum.dle,
                                                   sizeof( fileData.errCntSum.dle ) );
      }
      if ( eNoError != errorReg )  /* Update reg #124? */
      {
         /* Before filling in the length and offset, make sure the command sent to the host meter was a read or a write
            operation otherwise the data would be incorrect. */
         if ( ( pComPacket->TxPacket.uTxData.sReadFull.ucServiceCode >= CMD_TBL_RD_FULL ) &&
               ( pComPacket->TxPacket.uTxData.sReadFull.ucServiceCode <= CMD_TBL_WR_PARTIAL ) )
         {
            ( void )memset( &fileData.ansiErr, 0, sizeof( fileData.ansiErr ) );
            if ( eProcError == errorReg )   /* Procedeure error? */
            {
               /* Yes - Set the procedure/Table ID field with the procedure number that was executed. */
               //lint -esym(613,pProcResp) if errorReg is set to eProcError, then pProcResp is not NULL
               //lint -esym(413,pProcResp) if errorReg is set to eProcError, then pProcResp is not NULL
#if   defined(__PICC18__)
               ansiErrorReg.uTblId.tblElements.procTableIdMsb = pProcResp->utblIdbBfld.tblIdbBfld.tblProcNbrMsb;
               ansiErrorReg.uTblId.tblElements.procTableIdLsb = pProcResp->utblIdbBfld.tblProcNbrLsb;
#else
//XXX               fileData.ansiErr.uTblId.procTableID = pProcResp->utblIdbBfld.tblIdbBfld.tblProcNbr;
               fileData.ansiErr.uTblId.procTableID = pProcResp->tblProcNbr;   /*lint !e571 !e732 */
#endif
               fileData.ansiErr.uTblId.tblElements.procedure = 1;    /* Set the procedure flag */
               fileData.ansiErr.resultCode = pProcResp->resultCode;  /* Store the procedure result code. */
            }
            else if ( eTblError == errorReg )  /* Was the error a Table Error? */
            {
               /* Yes - Set the procedure/table ID field with table information. */
               fileData.ansiErr.uTblId.procTableID = pComPacket->TxPacket.uTxData.sReadFull.uiTbleID;
               /* Was the command to the host meter a write command?  Need to check all write command codes. */
               if ( ( pComPacket->TxPacket.uTxData.sReadFull.ucServiceCode >= CMD_TBL_WR_FULL ) &&
                     ( pComPacket->TxPacket.uTxData.sReadFull.ucServiceCode <= CMD_TBL_WR_PARTIAL ) )
               {
                  fileData.ansiErr.uTblId.tblElements.write = 1; /* Set the write flag */
               }
               fileData.ansiErr.resultCode = pComPacket->RxPacket.ucResponseCode; /* Update the response code */
            }
            /* Offset/Length fields are only used for partial table read/writes. Was the command a full tbl wr/rd? */
            if ( ( CMD_TBL_RD_FULL != pComPacket->TxPacket.uTxData.sReadFull.ucServiceCode ) &&
                  ( CMD_TBL_WR_FULL != pComPacket->TxPacket.uTxData.sReadFull.ucServiceCode ) )
            {
               /* Check if the offset is going to overflow. The maximum value is 12-bits. */
               if ( ( 0 != ( pComPacket->TxPacket.uTxData.sReadPartial.ucOffset[1] & 0xF0 ) ) || /* 2nd LSB byte */
                     ( 0 != pComPacket->TxPacket.uTxData.sReadPartial.ucOffset[0] ) )           /* MSB byte */
               {
#if   defined(__PICC18__)
                  /* The offset is out of range, Set to the maximum 12-bit value. */
                  ansiErrorReg.uLenOffset.lenOffsetBits.offsetMsb = 0xFF;  /* 8-bit field, set all bits. */
                  ansiErrorReg.uLenOffset.lenOffsetBits.offsetLsb = 0x0F;  /* 4-bit field, only set the 4 bits. */
#else
                  /* The offset is out of range, Set to the maximum 12-bit value. */
                  fileData.ansiErr.uLenOffset.lenOffsetBits.offset = 0x0FFF;  /* 8-bit field, set all bits. */
#endif
               }
               else
               {
                  /* The offset is in range, copy the requested offset to the register. The offset is 12-bits and spans
                     the bytes differently between the reg #124 and response of the meter in the RX packet. The shifting
                     below moves the 12-bit to properly allign the data in reg #124. */
#if   defined(__PICC18__)
                  ansiErrorReg.uLenOffset.lenOffsetBits.offsetMsb =
                     pComPacket->TxPacket.uTxData.sReadPartial.ucOffset[1] << 4;
                  ansiErrorReg.uLenOffset.lenOffsetBits.offsetMsb |=
                     pComPacket->TxPacket.uTxData.sReadPartial.ucOffset[2] >> 4;
                  ansiErrorReg.uLenOffset.lenOffsetBits.offsetLsb =
                     pComPacket->TxPacket.uTxData.sReadPartial.ucOffset[2] & 0x0F;
#else
                  uint16_t *pOffset = ( uint16_t * )(void *)&pComPacket->TxPacket.uTxData.sReadPartial.ucOffset[1];
                  fileData.ansiErr.uLenOffset.lenOffsetBits.offset = *pOffset & 0xFFF;
#endif
               }
               /* Check if the Length is going to overflow. The maximum value is 4-bits. */
               /*
                              if ( (0 != pComPacket->TxPacket.uTxData.sReadPartial.uiCount.BE.Msb.Byte) ||
                                   (0 != (pComPacket->TxPacket.uTxData.sReadPartial.uiCount.BE.Lsb.Byte & 0xF0)) )
               */
               if ( pComPacket->TxPacket.uTxData.sReadPartial.uiCount < 16 ) //XXX Magic!
               {
                  /* The length is out of range, Set to the maximum 4-bit value. */
                  fileData.ansiErr.uLenOffset.lenOffsetBits.length = 0x0F;
               }
               else
               {
                  /* The length is in range, Set to the requested table length. */
                  fileData.ansiErr.uLenOffset.lenOffsetBits.length = ( uint8_t )pComPacket->TxPacket.uTxData.sReadPartial.uiCount;
               }
            }
//XXX            diagsMask.bits.ansiTblErr = true;

//XXX            sysTime_t sysTimeBE;

//XXX            (void) TIME_GetDateTimeBE(&sysTimeBE);  /* Get the system date/time in Big Endian Format. */

            /* Now set the corresponding register. */
//XXX            fileData.ansiErr.serialDate = sysTimeBE.date; /* Store the serial date */
//XXX            fileData.ansiErr.serialTime = sysTimeBE.time.n16; /* Store the serial time */
         }
      }
//XXX      if ( 0 != diagsMask.reg )   /* Set the port 0 or ANSI flag on the diagnostics? */
      {
//XXX         (void)REG_Manipulate(REG_DIAG_INDICATORS, sizeof(diagsMask), REG_MAN_SET, (uint8_t *)&diagsMask);
      }
      ( void )FIO_fwrite( &mtrEngFileHndl_, 0, ( uint8_t * )&fileData, ( lCnt )sizeof( fileData ) );
   }
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void incVar(uint32_t data, uint8_t bits)">
/***********************************************************************************************************************

   Function name: incVar

   Purpose: Increments a variable of n bits without rolling over.

   Arguments: uint16_t data, uint8_t bits

   Returns: uint16_t - Value of incremented variable passed in.

 **********************************************************************************************************************/
static uint32_t incVar( uint32_t data, uint8_t bits )
{
   ASSERT( bits < ( sizeof( data ) * 8 ) ); /* Make sure the appropriate values are passed in. */

   if ( ( data != ULONG_MAX ) &&
         ( ( data + 1 ) < ( ( uint32_t )1 << bits ) ) ) /* Will an overflow occur if incremented? */
   {
      data++;  /* Increment the data. */
   }
   return( data );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void incVarWithRollOver(uint32_t data, uint8_t bits)">
/***********************************************************************************************************************

   Function name: incVarWithRollOver

   Purpose: Increments a variable of n bits with rolling over.

   Arguments: uint16_t data, uint8_t bits

   Returns: uint16_t - Value of incremented variable passed in.

 **********************************************************************************************************************/
static uint32_t incVarWithRollOver( uint32_t data, uint8_t bits )
{
   ASSERT( bits < ( sizeof( data ) * 8 ) ); /* Make sure the appropriate values are passed in. */

   if ( ( data != ULONG_MAX ) &&
         ( ( data + 1 ) < ( ( uint32_t )1 << bits ) ) ) /* Will an overflow occur if incremented? */
   {
      data++;  /* Increment the data. */
   }
   else
   {
      data = 0; /* Else rolled over, return 0 */
   }
   return( data );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="uint16_t HMC_ENG_getMeterSessionFailureCount(uint16_t *uMeterSessionFailureCount)">
/***********************************************************************************************************************

   Function name: HMC_ENG_getMeterSessionFailureCount

   Purpose: Get the meter session failure count parameter from the hmc_end module.

   Arguments: uint16_t *uMeterSessionFailureCount

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t HMC_ENG_getMeterSessionFailureCount(uint16_t *uMeterSessionFailureCount)
{
   mtrEngFileData_t fileData;
   returnStatus_t retVal = eFAILURE;

   if ( eSUCCESS == FIO_fread( &mtrEngFileHndl_, ( uint8_t * )&fileData, 0, ( lCnt )sizeof( fileData ) ) )
   {
      *uMeterSessionFailureCount = fileData.meterSessionFailureCount;
      retVal = eSUCCESS;
   }
   return( retVal );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="uint16_t HMC_ENG_setMeterSessionFailureCount(uint16_t uMeterSessionFailureCount)">
/***********************************************************************************************************************

   Function name: HMC_ENG_setMeterSessionFailureCount

   Purpose: Get the meter session failure count parameter from the hmc_end module.

   Arguments: uint16_t *uMeterSessionFailureCount

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t HMC_ENG_setMeterSessionFailureCount(uint16_t uMeterSessionFailureCount)
{
   mtrEngFileData_t fileData;
   returnStatus_t retVal = eFAILURE;

   if ( eSUCCESS == FIO_fread( &mtrEngFileHndl_, ( uint8_t * )&fileData, 0, ( lCnt )sizeof( fileData ) ) )
   {
      fileData.meterSessionFailureCount = uMeterSessionFailureCount;
      retVal = FIO_fwrite( &mtrEngFileHndl_, 0, ( uint8_t * )&fileData, ( lCnt )sizeof( fileData ) );
   }
   return( retVal );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="uint16_t HMC_ENG_getMeterCommunicationLockoutCount(uint16_t *uMeterCommunicationLockoutCount)">
/***********************************************************************************************************************

   Function name: HMC_ENG_getMeterCommunicationLockoutCount

   Purpose: Get the meter meter communication lockout count parameter from the hmc_end module.

   Arguments: uint16_t *uMeterSessionFailureCount

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t HMC_ENG_getMeterCommunicationLockoutCount(uint16_t *uMeterCommunicationLockoutCount)
{
   mtrEngFileData_t fileData;
   returnStatus_t retVal = eFAILURE;

   if ( eSUCCESS == FIO_fread( &mtrEngFileHndl_, ( uint8_t * )&fileData, 0, ( lCnt )sizeof( fileData ) ) )
   {
      *uMeterCommunicationLockoutCount = fileData.meterCommunicationLockoutCount;
      retVal = eSUCCESS;
   }
   return( retVal );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="uint16_t HMC_ENG_setMeterCommunicationLockoutCount(uint16_t uMeterCommunicationLockoutCount)">
/***********************************************************************************************************************

   Function name: HMC_ENG_setMeterCommunicationLockoutCount

   Purpose: Get the meter communication lockout count parameter from the hmc_end module.

   Arguments: uint16_t *uMeterSessionFailureCount

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t HMC_ENG_setMeterCommunicationLockoutCount(uint16_t uMeterCommunicationLockoutCount)
{
   mtrEngFileData_t fileData;
   returnStatus_t retVal = eFAILURE;

   if ( eSUCCESS == FIO_fread( &mtrEngFileHndl_, ( uint8_t * )&fileData, 0, ( lCnt )sizeof( fileData ) ) )
   {
      fileData.meterCommunicationLockoutCount = uMeterCommunicationLockoutCount;
      retVal = FIO_fwrite( &mtrEngFileHndl_, 0, ( uint8_t * )&fileData, ( lCnt )sizeof( fileData ) );
   }
   return( retVal );
}
/* ****************************************************************************************************************** */
/***********************************************************************************************************************

   Function name:HMC_ENG_getHmgEngStats

   Purpose: copy the hmc stats

   Arguments: uint16_t *uMeterSessionFailureCount

   Returns: returnStatus_t

 **********************************************************************************************************************/
returnStatus_t HMC_ENG_getHmgEngStats( void* dst, lCnt size)
{
  returnStatus_t retVal = eFAILURE;
  if(dst!= NULL && size >= sizeof(mtrEngFileData_t))
  {
    memset((void*) dst, 0, sizeof(mtrEngFileData_t));
    retVal = FIO_fread( &mtrEngFileHndl_, ( uint8_t * )dst, 0, ( lCnt )sizeof( mtrEngFileData_t ) );
  }
  return retVal;
}

// </editor-fold>
