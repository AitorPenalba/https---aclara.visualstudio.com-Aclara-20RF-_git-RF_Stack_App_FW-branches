// <editor-fold defaultstate="collapsed" desc="File Header Information">
/*****************************************************************************

   Filename: hmc_prg_mtr.c

   Contents: Read (from NVRAM), verify and execute the meter programming binary
             file. The binary file format is:

preamble: 4 bytes  file length (includes all of preamble)
preamble: 4 bytes  file CRC
preamble: 4 bytes  padding (zeroes)
preamble: 1 byte   format (0x64)
preamble: 20 bytes comDeviceType (ASCII string, left justified with zero-filled
                                 values for non-used characters).

Compatibility section Start Marker: 1 byte ID
	.
	series of commands comprised of a 1 byte command ID followed by the
    command's respective parameters.
	.
	.
Programming section Start Marker: 1 byte ID
	.
	series of commands comprised of a 1 byte command ID followed by the
    command's respective parameters.
	.
	.
Audit section Start Marker: 1 byte ID
	.
	series of commands comprised of a 1 byte command ID followed by the
    command's respective parameters.
	.
	.

Module Config section Start Marker: 1 byte ID (optional)
	.
	. (optional) Module Config data (Not part of hmc_prg_mtr function)
	.

End of File Marker: 1 byte ID

Preamble is BIG-Endian; remainder of file is LITTLE-Endian.

 ****************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2018 Aclara.  All Rights Reserved.

 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies
 * LLC an Ohio limited liability company (Aclara).  This information may not be
 * published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara. Any software or firmware described in this document
 * is furnished under a license and may be used or copied only in accordance
 * with the terms of such license.
 ****************************************************************************
 *
 * @author Bob Zepp
 *
 * $Log$ Created May 10, 2018
 *
 ****************************************************************************
 * Revision History:
 * v0.1 - 05/10/2018 - Initial Release
 *
 * @version    0.1
 * #since      2018-05-10
 ****************************************************************************/
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/****************************************************************************/
/* INCLUDE FILES */

#include "project.h"
#include "hmc_prg_mtr.h"
#include "time_DST.h"
#include "fio.h"
#include "ecc108_mqx.h"
#include "intf_cim_cmd.h"
#include "mqx.h"
#include "CompileSwitch.h"
#include "DBG_SerialDebug.h"
#include "ascii.h"
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
#include "time_util.h"
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constants">
/****************************************************************************/
/* CONSTANT DEFINITIONS */

/* none */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/****************************************************************************/
/* MACRO DEFINITIONS */
// Enable this define when debugging.
// Comment it out for production version.
//#define DEBUGGING_WRITE_TO_METER_FLASH // Enable when debugging

#if ( END_DEVICE_PROGRAMMING_FLASH == 0 )
    #ifdef  DEBUGGING_WRITE_TO_METER_FLASH
        #error  "Illegal attempt to debug Write to Meter Flash"
    #endif
#endif

#ifdef DEBUGGING_WRITE_TO_METER_FLASH
    #warning "DEBUGGING_WRITE_TO_METER_FLASH is enabled"
#endif


/*lint -esym( 750, BaseYear, PREAMBLE_SIZE, READ_BUFFER_SIZE,WRITE_BUFFER_SIZE,PROG_METER_RETRIES,
  SUSPEND_TIME_FOR_PSEUDO_SEM,MAX_BUFFER_SIZE,CONFIG_FORMAT,PATCH_FORMAT ) May not be referenced */
#define BaseYear                       2000 /* Year = number years since BaseYear */

#define PREAMBLE_SIZE                    33 /* Size in bytes of the DFW patch fields before the actual data */

#define READ_BUFFER_SIZE                 52 /* Must limit read  packets to 52 bytes of data due to buffer size in hmc code. */
#define WRITE_BUFFER_SIZE                47 /* Must limit write packets to 47 bytes of data due to buffer size in hmc code. */
#define PROG_METER_RETRIES                1 /* was 3 // Retry if meter read/write/procedure fails */
#define SUSPEND_TIME_FOR_PSEUDO_SEM      10 /* Number of milliSec to suspend to give a flag time to set/reset. */
#define MAX_BUFFER_SIZE                1024 /* Size of largest buffer in pool of buffers */

#define CONFIG_FORMAT                  0x64
#define PATCH_FORMAT                   0x65

// FYI:
// 0xF4240 = 1 million
// DFW patch partition size (65 sectors) = 0x41000 = 266,240
//#define PART_NV_DFW_PATCH_SIZE      ((lCnt)266240)

/*lint -esym( 750, HMC_PRG_MTR_PRNT_INFO, HMC_PRG_MTR_PRNT_WARN, HMC_PRG_MTR_PRNT_ERROR ) May not be referenced */
#if ENABLE_PRNT_HMC_PRG_MTR_INFO
#define HMC_PRG_MTR_PRNT_INFO( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_PRG_MTR_PRNT_INFO( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_PRG_MTR_WARN
#define HMC_PRG_MTR_PRNT_WARN( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_PRG_MTR_PRNT_WARN( a, fmt,... )
#endif

#if ENABLE_PRNT_HMC_PRG_MTR_ERROR
#define HMC_PRG_MTR_PRNT_ERROR( a, fmt,... )    DBG_logPrintf ( a, fmt, ##__VA_ARGS__ )
#else
#define HMC_PRG_MTR_PRNT_ERROR( a, fmt,... )
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/****************************************************************************/
/* TYPE DEFINITIONS */

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
// Meter Programming File markers and commands enumerated
typedef enum
{
   eMCFG_CMD_READ_VERIFY_DATA = ( (uint8_t)0 ),   // Verify meter table data
   eMCFG_CMD_READ_BIT_VERIFY_SINGLE,              // Verify table data bit patterns (one mask for all data)
   eMCFG_CMD_READ_BIT_VERIFY_MULTIPLE,            // Verify table data bit patterns (unique mask for each data byte)
   eMCFG_CMD_READ_COMPARE_TABLES,                 // Compare from 2 tables
   eMCFG_CMD_WRITE_DATA,                          // Write data to table
   eMCFG_CMD_WRITE_BIT_VALUE_SINGLE,              // Set/clear bits in table data (one mask for all data)
   eMCFG_CMD_WRITE_BIT_VALUE_MULTIPLE,            // Set/clear bits in table data (unique mask for each data byte)
   eMCFG_CMD_COPY,                                // Copy, table to table
   eMCFG_CMD_PROCEDURE,                           // Perform a procedure with default timeout
   eMCFG_CMD_DELAY,                               // Perform a processing delay
   eMCFG_CMD_DELAY_LOGOFF,                        // Perform a processing delay with a logoff x
   eMCFG_CMD_PROCEDURE_TIMEOUT,                   // Perform a procedure with specified timeout x
   eMCFG_CMD_READ_COMPARE_LIST,                   // Verify table data in data List (awaiting final definition) x
   eMCFG_CMD_READ_COMPARE_RANGE,                  // Verify table data within range (awaiting final definition) x
   eMCFG_CMD_COMPATIBILITY_SECTION = 100,         // Marker: Compatibility section x
   eMCFG_CMD_PROGRAMMING_SECTION,                 // Marker: Programming section x
   eMCFG_CMD_AUDIT_SECTION,                       // Marker: Audit section x
   eMCFG_CMD_END,                                 // Marker: end of file x
   eMCFG_CMD_MODULE_CFG_SECTION = 200             // Marker: Module config section x
}eMeterConfigCmd_t;

typedef enum
{
   eEXEC_DELAY_NO_LOGOFF = ( (uint8_t)0 ),  // Execute Procedure with default timeout
   eEXEC_DELAY_LOGOFF                       // Execute Procedure with specified timeout
}DelayForm_e;

typedef enum
{
   eEXEC_PROC_NO_TIMEOUT = ( (uint8_t)0 ),  // Execute Procedure with default timeout
   eEXEC_PROC_TIMEOUT                       // Execute Procedure with specified timeout
}ProcForm_e;

//Struct for the eMCFG_CMD_READ_VERIFY_DATA
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          TableID;       // Table ID
   uint8_t           Offset[3];     // Table Offset
   uint16_t          Count;         // Number of bytes to read/verify
   uint8_t           Data;          // First byte in Count array of Data
}ReadVerifyData_t;                  // Minimum size 9
PACK_END

//Struct for the eMCFG_CMD_READ_BIT_VERIFY_SINGLE (Also used for repeating Count pattern) and eMCFG_CMD_READ_BIT_VERIFY_MULTIPLE
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          TableID;       // Table ID
   uint8_t           Offset[3];     // Table Offset
   uint16_t          Count;         // Number of bytes to read/verify
   struct
   {
      uint8_t  onesMask;           // Mask for bit that must have a b'1'
      uint8_t  zeroesMask;         // Mask for bit that must have a b'0'
   }Masks;                         // First byte in Count array of masks
}ReadVerifyBits_t;                 // Minimum size 10
PACK_END

//Struct for the eMCFG_CMD_READ_COMPARE_TABLES
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          TableID_1;     // Table ID 1
   uint8_t           Offset_1[3];   // Table Offset 1
   uint16_t          Count;         // Number of bytes to read/verify
   uint16_t          TableID_2;     // Table ID 2
   uint8_t           Offset_2[3];   // Table Offset 2
}CompareTables_t;                   // Minimum size 13
PACK_END

//Struct for the eMCFG_CMD_WRITE_DATA
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          TableID;       // Table ID
   uint8_t           Offset[3];     // Table Offset
   uint16_t          Count;         // Number of bytes to read/verify
   uint8_t           Data;          // First byte in Count array of Data
}WriteData_t;                       // Minimum size 9
PACK_END

//Struct for the eMCFG_CMD_WRITE_BIT_VALUE_SINGLE (Also used for repeating Count pattern) and eMCFG_CMD_WRITE_BIT_VALUE_MULTIPLE
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          TableID;       // Table ID
   uint8_t           Offset[3];     // Table Offset
   uint16_t          Count;         // Number of bytes to read/verify
   struct
   {
      uint8_t  onesMask;            // Mask for bit that must have a b'1'
      uint8_t  zeroesMask;          // Mask for bit that must have a b'0'
   }Masks;                          // First byte in Count array of masks
}WriteBits_t;                       // Minimum size 10
PACK_END

//Struct for the eMCFG_CMD_COPY
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          TableID_1;     // Table ID 1
   uint8_t           Offset_1[3];   // Table Offset
   uint16_t          Count;         // Number of bytes to copy
   uint16_t          TableID_2;     // Table ID 2
   uint8_t           Offset_2[3];   // Table Offset
}CopyTable_t;                       // Minimum size 13
PACK_END

//Struct for the eMCFG_CMD_PROCEDURE
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          Procedure_Num; // Procedure Number (inclusive of Std or Mfg)
   uint8_t           Count;         // Number of bytes in attributes
   uint8_t           Attributes;    // First byte in Count array of attributes
}Procedure_t;                       // Minimum size 6
PACK_END

//Struct for the eMCFG_CMD_PROCEDURE_TIMEOUT
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          Procedure_Num; // Procedure Number (inclusive of Std or Mfg)
   uint8_t           Timeout;       // Time in Seconds to allow for procedure result to become COMPLETE
   uint8_t           Count;         // Number of bytes in attributes
   uint8_t           Attributes;    // First byte in Count array of attributes
}ProcedureTimeout_t;                // Minimum size 7
PACK_END

//Struct for the eMCFG_CMD_DELAY
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          Duration;      // Length of delay in Seconds
}Delay_t;                           // Minimum size 3
PACK_END

//Struct for the eMCFG_CMD_DELAY_LOGOFF
PACK_BEGIN
typedef struct  PACK_MID            //lint !e618
{
   eMeterConfigCmd_t Cmd;           // Read Verify Data Command
   uint16_t          Duration;      // Length of delay in Seconds
}DelayLogoff_t;                              // Minimum size 3
PACK_END

typedef struct
{
   union
   {
      ReadVerifyData_t     ReadVerifyData;
      ReadVerifyBits_t     ReadVerifyBits;
      CompareTables_t      CompareTables;
      WriteData_t          WriteData;
      WriteBits_t          WriteBits;
      CopyTable_t          CopyTable;
      Procedure_t          Procedure;
      ProcedureTimeout_t   ProcedureTimeout;
      Delay_t              Delay;
      DelayLogoff_t        DelayLogoff;
   }Cmds;
}Commands_t;

#endif

#if ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED )
#if 0 // Unused
// The parameter choices for PerformHexFileAction()
typedef enum
{
   eCheckSRecords,     // Preliminary check of S-Record file format.
   eExecuteSRecords    // Execute S-Record file to program meter Flash.
}eRecordAction_t;
#endif
#endif

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
typedef enum_CIM_QualityCode (*TableReadFunctionID) ( void *pDest, uint16_t id,
                               uint32_t offset, uint16_t cnt );
typedef enum_CIM_QualityCode (*TableWriteFunctionID) ( void *pSrc, uint16_t id,
                               uint32_t offset, uint16_t cnt );
typedef enum_CIM_QualityCode (*TableProcedureFunctionID) (  void const *pSrc,
                               uint16_t proc, uint8_t cnt );
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Global Variables">
/****************************************************************************/
/* GLOBAL VARIABLES */


// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Static Variables">
/****************************************************************************/
/* STATIC VARIABLES */

/*
The flag hmcPrgMtrIsStarted_ is set when first instructed to execute a
section of commands. It is asynchronous with hmc applet list execution.
Since some applets vary execution depending on whether or not it is to
program the meter, we must synchronize with the applet list. This is done
once the applet list has completed. It checks for the hmcPrgMtrIsStarted_
flag and, if set, sets the hmcPrgMtrIsSynced_ flag, which is
used by various applets to determine type of execution ("norm" vs "prg_mtr").
*/
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static bool hmcPrgMtrIsStarted_;
static bool hmcPrgMtrIsSynced_;
static bool hmcPrgMtrIsHoldOff_;
#endif

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
/*
Meter comm failure due to condition which does not allow comm request
*/
static bool hmcPrgMtrNotTestable_;
#endif

//// File related

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
// file offset begins with 0
static uint32_t FileOffset_;                 // Offset of next byte in file to be read.
static  int32_t RemainingFileBytesToRead_;   // Decremented with each byte read.
                                             // Is a Signed variable in case we
                                             // go negative due to an error.

// Partition data for DFW patch, where the meter programming file resides.
static PartitionData_t const *pPTbl_;

// File's size and marker offsets, assigned by MapTheFile ( void  ).
static struct
{
// file offset begins with 0
    uint32_t    FirstCompatibilityCommand_Offset;  // Compatibility Marker offset for first command
    uint32_t    FirstProgramCommand_Offset;        // Programming Marker offset for first command
    uint32_t    FirstAuditCommand_Offset;          // Audit Marker offset for first command
    uint32_t    End_Offset;                        // End offset (offset to first encounter with either
                                                   // end of file marker or config section marker)
    uint32_t    FileSize;                          // size of complete file (may include optional module
                                                   // config section)
} FileMap_;
#endif

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
// Current command's parameters
static struct
{
    uint8_t  CommandID;    // enumerated type eMeterConfigCmd_t
    uint16_t TableNumber1; // Table involved with command
    uint32_t TableOffset1; // Table offset involved with command
    uint16_t TableNumber2; // 2nd Table involved with command, if needed
    uint32_t TableOffset2; // 2nd Table offset involved with command, if needed
    uint16_t DataLength;   // number of table data bytes involved with command
    uint8_t  OnesMask;     // see explanation below
    uint8_t  ZeroesMask;   // see explanation below
}Command_;
#endif

/*
The above 2 masks are derived from the XML file's mask bit-pattern when
converted at the head-end to the Binary Meter Programming File as follows:
    XML     OnesMask     ZeroesMask
    ---     --------     ----------
    *   =      0             0
    0   =      0             1
    1   =      1             0

    To test if a data byte has ones according to the XML mask:
        1 - Bit-and the data with the OnesMask
        2 - Exclusive-Or results with the OnesMask
        3 - If result is zero, data agrees with XML mask.

    To test if a data byte has zeroes according to the XML mask:
        1 - Bit-and the data with the ZeroesMask
        2 - If results are zero, data agrees with XML mask.

    To set bits in a data byte according to the XML mask:
        1 - Bit-or the data with the OnesMask

    To clear bits in a data byte according to the XML mask:
        1 - Bit-and the data with the 1's complement of the ZeroesMask
*/

//// File Buffer related
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static uint16_t  FileBufferSize_;   // MAX_BUFFER_SIZE, MID_BUFFER_SIZE or
                                    // MIN_BUFFER_SIZE
static uint16_t  FileBufferIndex_;   // 0 to (FileBufferSize_-1)
static buffer_t *pFileBuffer_;       // pointer to file buffer
static uint8_t   ProcTimeout_ = 0;  //Timeout for execute procedure to result in COMPLETE
#endif

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static TableWriteFunctionID     TableWriteFunction_;
static TableProcedureFunctionID TableProcedureFunction_;
#endif
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static TableReadFunctionID      TableReadFunction_;
#endif

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Function Prototypes">
/****************************************************************************/
/* FUNCTION PROTOTYPES */

//// GLOBAL PROTOTYPES
//returnStatus_t      HMC_PRG_MTR_init ( void ); // Called each powerup (startup)
/*
HMC_PRG_MTR_PrepareToProgMeter () MUST be called only once, but prior to
however many calls are made to HMC_PRG_MTR_RunProgramMeter (). It must be
re-called following a power cycle before continuing to make calls to
HMC_PRG_MTR_RunProgramMeter ().
*/
//returnStatus_t   HMC_PRG_MTR_PrepareToProgMeter( void );
//returnStatus_t   HMC_PRG_MTR_RunProgramMeter ( eFileSection_t SectionToRun );
/*
Program Meter uses it's own set of meter read/write/procedure functions, so
the original set of functions (used by tasks) can be set to refuse task
requests, thereby eliminating communication interference from tasks during a
program meter operation.
*/
//void             HMC_PRG_MTR_SwitchTableAccessFunctions ( void );
//void             HMC_PRG_MTR_ReturnTableAccessFunctions ( void );

//// STATIC PROTOTYPES
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
static returnStatus_t  CreateFileBuffer ( void );
static uint8_t         InitGetNextByte ( returnStatus_t  *pRetVal );
static uint8_t         GetNextByte ( returnStatus_t  *pRetVal );
#endif

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
static returnStatus_t  MapTheFile ( void  );
static returnStatus_t  FetchAndExecuteCommands ( void );
static returnStatus_t  PerformRD ( void );
static returnStatus_t  PerformWD ( void );
static returnStatus_t  PerformRBS ( void );
static returnStatus_t  PerformRBM ( void );
static returnStatus_t  PerformRR ( void );
static returnStatus_t  PerformWBS ( void );
static returnStatus_t  PerformWBM ( void );
static returnStatus_t  PerformRW ( void );
static returnStatus_t  PerformPR ( ProcForm_e );
static returnStatus_t  PerformDY ( DelayForm_e );
#endif
// </editor-fold>


/////////////////////////////////////////////////////////////////////////
//////////////////////// HMC Meter Programming //////////////////////////
/////////////////////////// Implementation //////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
/************************************************************************

 Function Name: HMC_PRG_MTR_IsNotTestable

 Purpose: Checks if failure was due to inescapable cause (retry will not help).

 Arguments: none

 Returns: bool- true is inescapable cause, false is other

 Side Effects:  none

 Re-entrant Code: Yes

 Notes: Cause is one of the following ( see ANSI C12.18 Protocol Spec ):
    SNS - service not suppported
    ISC - insufficient security clearance
    ONP - operation not possible
    IAR - inappropriate action requested

************************************************************************/
bool HMC_PRG_MTR_IsNotTestable( void )
{
   return hmcPrgMtrNotTestable_;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_ClearNotTestable

 Purpose: Clears the occurrence of a NotTestable fault.

 Arguments: none

 Returns: none

 Side Effects:  hmcPrgMtrNotTestable_ set to false

 Re-entrant Code: Yes

 Notes: Used when exiting meter programming.

************************************************************************/
void HMC_PRG_MTR_ClearNotTestable( void )
{
   hmcPrgMtrNotTestable_ = false;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_SetNotTestable

 Purpose: Sets the occurrence of a NotTestable fault.

 Arguments: none

 Returns: none

 Side Effects:  hmcPrgMtrNotTestable_ set to true

 Re-entrant Code: Yes

 Notes: Used when failure occurs due to inescapable cause.

************************************************************************/
void HMC_PRG_MTR_SetNotTestable( void )
{
   hmcPrgMtrNotTestable_ = true;
}
#endif // End  #if END_DEVICE_PROGRAMMING_CONFIG == 1


#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
/************************************************************************

 Function Name: HMC_PRG_MTR_init

 Purpose: Called at startup to initiate flags needed for
          meter programming.

 Arguments: none

 Returns: returnStatus_t

 Side Effects:  Creates 2 flags.

 Re-entrant Code: Yes

 Notes:

The flag hmcPrgMtrIsStarted_ is set when first instructed to execute a
section of commands. It is asynchronous with hmc applet list execution.
Since some applets vary execution depending on whether or not it is to
program the meter, we must synchronize with the applet list. This is done
once the applet list has completed. It checks for the hmcPrgMtrIsStarted_
flag and, if set, sets the hmcPrgMtrIsSynced_ flag, which is
used by various applets to determine type of execution ("norm" vs "prg_mtr").

************************************************************************/
returnStatus_t HMC_PRG_MTR_init ( void )
{
   hmcPrgMtrIsStarted_     = false;
   hmcPrgMtrIsSynced_      = false;
   hmcPrgMtrIsHoldOff_     = false;
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   TableReadFunction_      = INTF_CIM_CMD_ansiRead;
#endif
   TableWriteFunction_     = INTF_CIM_CMD_ansiWrite;
   TableProcedureFunction_ = INTF_CIM_CMD_ansiProcedure;

   return eSUCCESS;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_IsStarted

 Purpose: Check to see if meter programming has started.

 Arguments: none

 Returns: bool- true is started, false is not started

 Side Effects:  none

 Re-entrant Code: Yes

 Notes: The flag hmcPrgMtrIsStarted_ is set when first instructed to execute a
        Program or Audit section.

************************************************************************/
bool HMC_PRG_MTR_IsStarted ( void )
{
   return hmcPrgMtrIsStarted_;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_IsSynced

 Purpose: Check to see if meter programming has synced with applet list.

 Arguments: none

 Returns: bool- true is synced, false is not synced

 Side Effects:  none

 Re-entrant Code: Yes

 Notes: The flag hmcPrgMtrIsSynced_ is set when applet list completes and
        meter programming has first started.

************************************************************************/
bool HMC_PRG_MTR_IsSynced ( void )
{
   return hmcPrgMtrIsSynced_;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_ClearSync

 Purpose: Clears the sync variable hmcPrgMtrIsSynced_.

 Arguments: none

 Returns: none

 Side Effects:  hmcPrgMtrIsSynced_ set to false

 Re-entrant Code: Yes

 Notes: Used when exiting meter programming.

************************************************************************/
void HMC_PRG_MTR_ClearSync ( void )
{
   hmcPrgMtrIsSynced_ = false;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_SetSync

 Purpose: Sets the sync variable hmcPrgMtrIsSynced_.

 Arguments: none

 Returns: none

 Side Effects:  hmcPrgMtrIsSynced_ set to true

 Re-entrant Code: Yes

 Notes: Used when entering meter programming.

************************************************************************/
void HMC_PRG_MTR_SetSync ( void )
{
   hmcPrgMtrIsSynced_ = true;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_SwitchTableAccessFunctions

 Purpose: Switches to a set of meter read/write/procedure functions dedicated
          to meter programming. The original set of functions are used by tasks
          to request meter reads/writes/procedures, and refuses task requests
          during meter programming. The same queue is used during meter
          programming as for task requests.

 Arguments: none

 Returns: void

 Side Effects:  Selects new functions to read/write/procedure with meter.

 Re-entrant Code: Yes

 Notes:
    Tasks will continue to call INTF_CIM_CMD_ansiRead/Write/Procedure, and
    will be denied so as not to interfere with meter programming. Meter
    programming will call HMC_PRG_MTR_ansiRead/Write/Procedure which are
    identical to the INTF_CIM_CMD_ansiRead/Write/Procedure functions, using
    the same queue.

************************************************************************/
void HMC_PRG_MTR_SwitchTableAccessFunctions ( void )
{
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   TableReadFunction_ = HMC_PRG_MTR_ansiRead;
#endif
   TableWriteFunction_ = HMC_PRG_MTR_ansiWrite;
   TableProcedureFunction_ = HMC_PRG_MTR_ansiProcedure;
}


/************************************************************************

 Function Name: HMC_PRG_MTR_ReturnTableAccessFunctions

 Purpose: Following meter programming, returns the meter read/write/procedure
          functions to those used by tasks.

 Arguments: none

 Returns: void

 Side Effects:  Selects new functions to read/write/procedure with meter.

 Re-entrant Code: Yes

 Notes:
    Tasks will continue to call INTF_CIM_CMD_ansiRead/Write/Procedure, but
    will no longer be denied once meter programming has completed.

************************************************************************/
void HMC_PRG_MTR_ReturnTableAccessFunctions ( void )
{
#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
   TableReadFunction_ = INTF_CIM_CMD_ansiRead;
#endif
   TableWriteFunction_ = INTF_CIM_CMD_ansiWrite;
   TableProcedureFunction_ = INTF_CIM_CMD_ansiProcedure;
}
#endif

#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )
/************************************************************************

 Function Name: HMC_PRG_MTR_IsHoldOff

 Purpose: Check to see if normal R/W/P are being held off.

 Arguments: none

 Returns: bool- true is holding off, false is not

 Side Effects:  none

 Re-entrant Code: Yes

 Notes: The flag hmcPrgMtrIsHoldOff is set when downloading patch to
        meter. Must hold off normal comm with meter while unsyncing, and
        then re-syncing with applet list. Otherwise intfCimAnsiMutex_ may
        lock in normal comm and then block after re-syncing.

************************************************************************/
bool HMC_PRG_MTR_IsHoldOff ( void )
{
   return hmcPrgMtrIsHoldOff_;
}


/************************************************************************

 Function Name:  HMC_PRG_MTR_PrepareToProgMeter

 Purpose: Reads thru the meter programming binary file to record the
          file size and the offset of the beginning of each section.

 Arguments: none

 Returns: eSUCCESS, ePRG_MTR_INTERNAL_ERROR, or ePRG_MTR_FILE_FORMAT_ERROR

 Side Effects: Access to NVRAM. Calls MapTheFile ( ) which populates the
               variable FileMap_ and alters FileOffset_.

 Re-entrant Code: No

 Notes:  HMC_PRG_MTR_PrepareToProgMeter () MUST be called only once, but prior
        to however many calls are made to HMC_PRG_MTR_RunProgramMeter ().
        It must be re-called following a power cycle before continuing to make
        calls to HMC_PRG_MTR_RunProgramMeter ().

************************************************************************/
returnStatus_t  HMC_PRG_MTR_PrepareToProgMeter( void )
{
   returnStatus_t retVal = eSUCCESS;
   uint8_t        MSByte;
   uint8_t        M2Byte;
   uint8_t        M3Byte;
   uint8_t        M4Byte;
   uint8_t        Format;

   // Open the file in NVRAM
   if ( eSUCCESS != PAR_partitionFptr.parOpen( &pPTbl_, ePART_DFW_PGM_IMAGE, 0L ) )
   {
      // Attempt to open file has failed
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "File Open Failed" );
   }
   else
   {
      // Read file size from beginning of preamble
      // The PAR_partitionFptr.parRead with num bytes = 4 returns a 4-byte
      // value with byte 0 = LSByte. This has been changed so byte 0 is the
      // MSByte of the preamble length, so must read byte by byte.

//        if ( eSUCCESS !=
//         PAR_partitionFptr.parRead( (uint8_t *)&FileMap_.FileSize,
//         0 /* file offset */, 4 /* num bytes */, pPTbl_ ) )
//        {
//            retVal = ePRG_MTR_INTERNAL_ERROR;
//            HMC_PRG_MTR_PRNT_ERROR( 'E', "File Read Failed" );
//        }
      if ( ( eSUCCESS != PAR_partitionFptr.parRead( &MSByte,  0, 1, pPTbl_ ) ) ||
           ( eSUCCESS != PAR_partitionFptr.parRead( &M2Byte,  1, 1, pPTbl_ ) ) ||
           ( eSUCCESS != PAR_partitionFptr.parRead( &M3Byte,  2, 1, pPTbl_ ) ) ||
           ( eSUCCESS != PAR_partitionFptr.parRead( &M4Byte,  3, 1, pPTbl_ ) ) ||
           ( eSUCCESS != PAR_partitionFptr.parRead( &Format, 12, 1, pPTbl_ ) ) )
      {
         retVal = ePRG_MTR_INTERNAL_ERROR;
         HMC_PRG_MTR_PRNT_ERROR( 'E', "File Read Failed" );
      }
      else
      {
#ifdef DEBUGGING_WRITE_TO_METER_FLASH
         Format = PATCH_FORMAT;
#endif
         FileMap_.FileSize = ( (uint32_t)MSByte << 24 ) |
                             ( (uint32_t)M2Byte << 16 ) |
                             ( (uint32_t)M3Byte <<  8 ) | (uint32_t)M4Byte;

         // Size must be at least one byte more than preamble size
         if ( FileMap_.FileSize <= PREAMBLE_SIZE )
         {
            retVal = ePRG_MTR_FILE_FORMAT_ERROR;
            HMC_PRG_MTR_PRNT_ERROR( 'E', "File Too Short" );
         }
         else
         {
            if ( ( CONFIG_FORMAT != Format ) && ( PATCH_FORMAT != Format ) )
            {
               retVal = ePRG_MTR_FILE_FORMAT_ERROR;
               HMC_PRG_MTR_PRNT_ERROR( 'E', "Illegal Preamble Format" );
            }
#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )
            else if ( CONFIG_FORMAT == Format )
            {
               // Map file for use when configuring meter
               retVal = MapTheFile ( );
            }
#endif
         }

      }
      // Close the file
      (void)PAR_partitionFptr.parClose( pPTbl_ );
   }

   return retVal;
}

#endif // End  #if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )


#if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )

/************************************************************************

 Function Name: HMC_PRG_MTR_RunProgramMeter

 Purpose: This runs the entire sequence needed to execute the commands
          in the section designated by the argument.

 Arguments: eFileSection_t  SectionToRun - an enumerated value indicating
                                           which section should be executed.

 Returns: returnStatus_t

 Side Effects: Access to NVRAM. FileOffset_, RemainingFileBytesToRead_,
               pFileBuffer_, FileBufferSize_, hmcPrgMtrIsStarted_, Command_

 Re-entrant Code: No

 Notes:  HMC_PRG_MTR_PrepareToProgMeter () MUST be called only once, but prior
        to however many calls are made to HMC_PRG_MTR_RunProgramMeter ().
        It must be re-called following a power cycle before continuing to make
        calls to HMC_PRG_MTR_RunProgramMeter ().

        FileSize will be N bytes, starting at byte 1 and ending with byte N.
        The FileOffset_ will run from 0 to (N-1).

************************************************************************/
returnStatus_t   HMC_PRG_MTR_RunProgramMeter ( eFileSection_t SectionToRun )
{
   returnStatus_t  retVal = eSUCCESS;

   // (1) Open the file in NVRAM
   if ( eSUCCESS != PAR_partitionFptr.parOpen( &pPTbl_, ePART_DFW_PGM_IMAGE, 0L ) )
   {
      // Attempt to open file has failed
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "File Open Failed" );
   }
   else
   {
      // Successful file open
      // (2) Setup for designated section
      if ( SECTION_COMPATIBILITY == SectionToRun )
      {
         FileOffset_               = FileMap_.FirstCompatibilityCommand_Offset;
         RemainingFileBytesToRead_ = (int32_t)( FileMap_.FirstProgramCommand_Offset - FileOffset_ ); // Includes next marker
         HMC_PRG_MTR_PRNT_INFO( 'I',  "Run Compatibility Section" );
      }
      else if ( SECTION_PROGRAM == SectionToRun )
      {
         FileOffset_               = FileMap_.FirstProgramCommand_Offset;
         RemainingFileBytesToRead_ = (int32_t)( FileMap_.FirstAuditCommand_Offset - FileOffset_ ); // Includes next marker
         HMC_PRG_MTR_PRNT_INFO( 'I',  "Run Programming Section" );
      }
      else if ( SECTION_AUDIT == SectionToRun )
      {
         FileOffset_               = FileMap_.FirstAuditCommand_Offset;
         RemainingFileBytesToRead_ = (int32_t)( ( FileMap_.End_Offset - FileOffset_ ) + 1 ); // Includes next marker
         HMC_PRG_MTR_PRNT_INFO( 'I',  "Run Audit Section" );
      }
      else
      {
         retVal = ePRG_MTR_INTERNAL_ERROR;
         HMC_PRG_MTR_PRNT_ERROR( 'E', "Run Unknown Section" );
      }

      if ( retVal == eSUCCESS )
      {
         // FileOffset_ addresses first command in section
         // (3) Create buffer to receive bytes from file in NVRAM
         retVal = CreateFileBuffer ( ); // pFileBuffer_ assigned
         if ( retVal == eSUCCESS )
         {
            if ( SECTION_COMPATIBILITY == SectionToRun )
            {
               // (4) DO NOT Synchronize with applet list.
               //     hmcPrgMtrIsSynced_ will remain false.
               // (5) Execute commands in Compatibility section.
               retVal = FetchAndExecuteCommands ( );
            }
            else
            {
               // (4) Synchronize with applet list
               hmcPrgMtrIsStarted_ = true;
               // Wait for applet list to sync
               while ( !hmcPrgMtrIsSynced_ )
               {
                  // Temporarily Suspend task.
                  OS_TASK_Sleep( SUSPEND_TIME_FOR_PSEUDO_SEM );
               }
               HMC_PRG_MTR_PRNT_INFO( 'I',  "Applet List synchronized" );

               // (5) Execute commands in selected section
               retVal = FetchAndExecuteCommands ( );
            }
            // (6) Free buffer
            BM_free ( pFileBuffer_ );
         }
         //else  -  Handled in  CreateFileBuffer ( )
         //{
         //   retVal = ePRG_MTR_INTERNAL_ERROR;
         //   HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
         //}
      }

      // (7) Close the file in NVRAM
      (void)PAR_partitionFptr.parClose( pPTbl_ );
      // (8) When Audit completes, return to "normal" operation
      if ( SECTION_AUDIT == SectionToRun )
      {
         hmcPrgMtrIsStarted_ = false;
      }

      if ( eSUCCESS != retVal )
      {
         if ( Command_.CommandID == (uint8_t)eMCFG_CMD_PROCEDURE )
         {
            HMC_PRG_MTR_PRNT_ERROR ( 'E', "FAILURE: Procedure %u", Command_.TableNumber1 );
         }
         else
         {
            HMC_PRG_MTR_PRNT_ERROR ( 'E', "FAILURE: CmdID %u, Table %u, Offset %u", Command_.CommandID, Command_.TableNumber1, Command_.TableOffset1 );
         }
      }
   }

   return retVal;
}


/************************************************************************

 Function Name: MapTheFile

 Purpose: Assigns values to FileMap_. Verifies the file contains only one of
          each Marker, and in the correct order. Verifies the command IDs are
          legal and the command lengths, although possibly incorrect, are
          followed by another legal command.

 Arguments: none

 Returns: eSUCCESS, ePRG_MTR_INTERNAL_ERROR, or ePRG_MTR_FILE_FORMAT_ERROR

 Side Effects: FileOffset_, FileMap_

 Re-entrant Code: No

 Notes: FileSize will be N bytes, starting at byte 1 and ending with byte N.
        The FileOffset_ will run from 0 to (N-1).
************************************************************************/
static returnStatus_t MapTheFile ( void  )
{
   returnStatus_t  retVal             = eSUCCESS;
   bool            compatibilityFound = FALSE;
   bool            programFound       = FALSE;
   bool            auditFound         = FALSE;
   bool            done               = FALSE;
   Commands_t      command;

   // Start with byte following preamble.
   FileOffset_ = PREAMBLE_SIZE;
   while ( !done && ( eSUCCESS == retVal ) )
   {
      // Read next command / marker
      if ( eSUCCESS != PAR_partitionFptr.parRead( (uint8_t *)&command, FileOffset_, sizeof(command), pPTbl_ ) )
      {
         retVal = ePRG_MTR_INTERNAL_ERROR;
         HMC_PRG_MTR_PRNT_ERROR( 'E', "File Read Failed" );
      } /* end of "if" */
      else
      {
         // For each command/marker, advance thru parameters/data to what should be the next command/marker.
         switch ( command.Cmds.ReadVerifyData.Cmd ) //SMG command or marker (Cmd member same offset for ALL struct types)
         {
            case eMCFG_CMD_READ_VERIFY_DATA: //Done Yes
               FileOffset_ += (uint32_t)offsetof( ReadVerifyData_t, Data ) +
                              command.Cmds.ReadVerifyData.Count * sizeof(command.Cmds.ReadVerifyData.Data);
               break;
            case eMCFG_CMD_READ_BIT_VERIFY_SINGLE: //Done Yes
               FileOffset_ += (uint32_t)offsetof( ReadVerifyBits_t, Masks ) +
                              command.Cmds.ReadVerifyBits.Count * sizeof(command.Cmds.ReadVerifyBits.Masks);
               break;
            case eMCFG_CMD_READ_BIT_VERIFY_MULTIPLE:  //Done Yes
               FileOffset_ += (uint32_t)offsetof( ReadVerifyBits_t, Masks ) +
                              command.Cmds.ReadVerifyBits.Count * sizeof(command.Cmds.ReadVerifyBits.Masks);
               break;
            case eMCFG_CMD_READ_COMPARE_TABLES:  //Done Yes
               FileOffset_ += (uint32_t)offsetof( CompareTables_t, Offset_2[0] ) + sizeof(command.Cmds.CompareTables.Offset_2);
               break;
            case eMCFG_CMD_WRITE_DATA: //Done Yes
               FileOffset_ += (uint32_t)offsetof( WriteData_t, Data ) +
                              command.Cmds.WriteData.Count * sizeof(command.Cmds.WriteData.Data);
               break;
            case eMCFG_CMD_WRITE_BIT_VALUE_SINGLE: //Done Yes
               FileOffset_ += (uint32_t)offsetof( WriteBits_t, Masks ) +
                              command.Cmds.WriteBits.Count * sizeof(command.Cmds.WriteBits.Masks);
               break;
            case eMCFG_CMD_WRITE_BIT_VALUE_MULTIPLE:  //Done Yes
               FileOffset_ += (uint32_t)offsetof( WriteBits_t, Masks ) +
                              command.Cmds.WriteBits.Count * sizeof(command.Cmds.WriteBits.Masks);
               break;
            case eMCFG_CMD_COPY: //Done Yes
               FileOffset_ += (uint32_t)offsetof( CopyTable_t, Offset_2[0] ) + sizeof(command.Cmds.CopyTable.Offset_2);
               break;
            case eMCFG_CMD_DELAY:   //Done Yes
               FileOffset_ += (uint32_t)offsetof( Delay_t, Duration ) + sizeof(command.Cmds.Delay.Duration);
               break;
            case eMCFG_CMD_DELAY_LOGOFF:  //Done Yes
               FileOffset_ += (uint32_t)offsetof( DelayLogoff_t, Duration ) + sizeof(command.Cmds.DelayLogoff.Duration);
               break;
            case eMCFG_CMD_PROCEDURE:  //Done Yes
               FileOffset_ += (uint32_t)offsetof( Procedure_t, Attributes ) +
                              command.Cmds.Procedure.Count * sizeof(command.Cmds.Procedure.Attributes);
               break;
            case eMCFG_CMD_PROCEDURE_TIMEOUT: //Done Yes
               FileOffset_ += (uint32_t)offsetof( ProcedureTimeout_t, Attributes ) +
                              command.Cmds.ProcedureTimeout.Count * sizeof(command.Cmds.ProcedureTimeout.Attributes);
               break;
            case eMCFG_CMD_COMPATIBILITY_SECTION: // Marker
               FileOffset_ ++; // One byte for Section Id's
               if ( !compatibilityFound )
               {
                  compatibilityFound = TRUE;
                  if ( programFound || auditFound || ( FileOffset_ != ( PREAMBLE_SIZE + 1 ) ) )  // (First byte after preamble)
                  {
                     retVal = ePRG_MTR_FILE_FORMAT_ERROR;
                     HMC_PRG_MTR_PRNT_ERROR( 'E', "Section Missing" );
                  }
                  else
                  {
                     FileMap_.FirstCompatibilityCommand_Offset = FileOffset_;
                  }
               }
               else
               {
                  retVal = ePRG_MTR_FILE_FORMAT_ERROR;
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Section Repeated" );
               }
               break;
            case eMCFG_CMD_PROGRAMMING_SECTION: // Marker
               FileOffset_ ++; // One byte for Section Id's
               if ( !programFound )
               {
                  programFound = TRUE;
                  if ( !compatibilityFound || auditFound )
                  {
                     retVal = ePRG_MTR_FILE_FORMAT_ERROR;
                     HMC_PRG_MTR_PRNT_ERROR( 'E', "Section Missing" );
                  }
                  else
                  {
                     FileMap_.FirstProgramCommand_Offset = FileOffset_;
                  }
               }
               else
               {
                  retVal = ePRG_MTR_FILE_FORMAT_ERROR;
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Section Repeated" );
               }
               break;
            case eMCFG_CMD_AUDIT_SECTION: // Marker
               FileOffset_ ++; // One byte for Section Id's
               if ( !auditFound )
               {
                  auditFound = TRUE;
                  if ( !compatibilityFound || !programFound )
                  {
                     retVal = ePRG_MTR_FILE_FORMAT_ERROR;
                     HMC_PRG_MTR_PRNT_ERROR( 'E', "Section Missing" );
                  }
                  else
                  {
                     FileMap_.FirstAuditCommand_Offset = FileOffset_;
                  }
               }
               else
               {
                  retVal = ePRG_MTR_FILE_FORMAT_ERROR;
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Section Repeated" );
               }
               break;
            case eMCFG_CMD_END: // Marker
               // FALLTHROUGH
            case eMCFG_CMD_MODULE_CFG_SECTION: // Marker
               if ( !compatibilityFound || !programFound || !auditFound )
               {
                  retVal = ePRG_MTR_FILE_FORMAT_ERROR;
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Section Missing" );
               }
               else
               {
                  // FileMap_.End_Offset is offset of END or Config Module
                  FileMap_.End_Offset = FileOffset_;
                  done = TRUE;
               }
               break;
            case eMCFG_CMD_READ_COMPARE_LIST:
               // FALLTHROUGH
            case eMCFG_CMD_READ_COMPARE_RANGE:
               // FALLTHROUGH
            default:
               retVal = ePRG_MTR_FILE_FORMAT_ERROR;
               HMC_PRG_MTR_PRNT_ERROR( 'E', "Unknown Command" );
               break;
         } /* end of "switch" */
         if ( FileOffset_ >= FileMap_.FileSize )
         {
            retVal = ePRG_MTR_FILE_FORMAT_ERROR;
            HMC_PRG_MTR_PRNT_ERROR( 'E', "Exceeded File Size" );
         }
      } /* end of "else" */
   } /* end of "while" */
   return retVal;
}

/************************************************************************

 Function Name: FetchAndExecuteCommands

 Purpose: Loops reading commands from file buffer and calling associated
          function to execute command. Returns upon reading a marker or error.

 Arguments: none

 Returns: eSUCCESS or ePRG_MTR_FILE_FORMAT_ERROR

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: Since we are executing commands in a single section, execution
        stops with the next marker.

************************************************************************/
static returnStatus_t FetchAndExecuteCommands ( void )
{
   eMeterConfigCmd_t NextCommand;
   returnStatus_t    retVal = eSUCCESS;
   bool              done   = FALSE;

   // Fill file buffer and read first command (first byte)
   NextCommand = (eMeterConfigCmd_t)InitGetNextByte ( &retVal );

   while ( ( retVal == eSUCCESS ) && !done )
   {
      Command_.CommandID = (uint8_t)NextCommand;
      // Execute command.
      switch ( NextCommand )
      {
         case eMCFG_CMD_READ_VERIFY_DATA:
            retVal = PerformRD ();
            break;
         case eMCFG_CMD_READ_BIT_VERIFY_SINGLE:
            retVal = PerformRBS ();
            break;
         case eMCFG_CMD_READ_BIT_VERIFY_MULTIPLE:
            retVal = PerformRBM ();
            break;
         case eMCFG_CMD_READ_COMPARE_TABLES:
            retVal = PerformRR ();
            break;

         case eMCFG_CMD_WRITE_DATA:
            retVal = PerformWD ();
            break;
         case eMCFG_CMD_WRITE_BIT_VALUE_SINGLE:
            retVal = PerformWBS ();
            break;
         case eMCFG_CMD_WRITE_BIT_VALUE_MULTIPLE:
            retVal = PerformWBM ();
            break;
         case eMCFG_CMD_COPY:
            retVal = PerformRW ();
            break;
         case eMCFG_CMD_PROCEDURE:
            retVal = PerformPR (eEXEC_PROC_NO_TIMEOUT);
            break;
         case eMCFG_CMD_PROCEDURE_TIMEOUT:
            retVal = PerformPR (eEXEC_PROC_TIMEOUT);
            break;
         case eMCFG_CMD_DELAY:
            retVal = PerformDY (eEXEC_DELAY_NO_LOGOFF);
            break;
         case eMCFG_CMD_DELAY_LOGOFF:
            retVal = PerformDY (eEXEC_DELAY_LOGOFF);
            break;

         // Terminate on next marker
         case eMCFG_CMD_PROGRAMMING_SECTION:
            // FALLTHROUGH
         case eMCFG_CMD_AUDIT_SECTION:
            // FALLTHROUGH
         case eMCFG_CMD_MODULE_CFG_SECTION:
            // FALLTHROUGH
         case eMCFG_CMD_END:
            done = TRUE;
         break;

         case eMCFG_CMD_COMPATIBILITY_SECTION: // Should not occur
            // FALLTHROUGH
         case eMCFG_CMD_READ_COMPARE_LIST: // TBD
            // FALLTHROUGH
         case eMCFG_CMD_READ_COMPARE_RANGE: // TBD
            // FALLTHROUGH
         default:
            retVal = ePRG_MTR_FILE_FORMAT_ERROR;
            HMC_PRG_MTR_PRNT_ERROR( 'E', "Unknown Command" );
            break;
      }

      if (retVal == eSUCCESS )
      {
         // FileMap_.End_Offset is offset of EOF or Config Module
         if ( !done )
         {
            if ( FileOffset_ > FileMap_.End_Offset )
            {
               retVal = ePRG_MTR_FILE_FORMAT_ERROR;
               HMC_PRG_MTR_PRNT_ERROR( 'E', "Exceeded File Size" );

            }
            else
            {
               NextCommand = (eMeterConfigCmd_t)GetNextByte ( &retVal );
               if( eSUCCESS == retVal )
               {
                  HMC_APP_ResetAppletTimeout(); //reset applet timer if we are still programming
               }
            }
         }
      }
      else if( retVal == eFAILURE )
      {
         HMC_PRG_MTR_PRNT_ERROR( 'E', "Failed programming command" );

      }
   }

   if( retVal != eSUCCESS )
   {
      retVal = eFAILURE;
   }

   return retVal;
}


/************************************************************************

 Function Name: PerformRD

 Purpose: Performs a read and verify command.
          Reads a given number of bytes from a table and offset, and validates
          they match the respective values in the parameter list.
          The command format is:
            < command ID >	    1 byte
            < table number >	2 bytes
            < offset >		    3 bytes
            < length to read >	2 bytes
            < data byte >
                . 		“length” number of verification bytes
            < data byte >

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table read and verify

************************************************************************/
static returnStatus_t PerformRD ( void )
{
   returnStatus_t        retVal = eSUCCESS;
   buffer_t             *pMeterBuffer;
   uint8_t               numRetries;
   uint8_t               SaveByte;
   bool                  done = false;
   uint16_t              PacketDataLength;
   uint16_t              i;
   enum_CIM_QualityCode  RdWrResult;
   uint16_t              RemainingBytes;

   pMeterBuffer = BM_alloc ( READ_BUFFER_SIZE );
   if ( NULL == pMeterBuffer )
   {
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
   }
   else
   {
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Read and Verify" );
      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.TableNumber1  = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.TableNumber1 |= SaveByte;

      Command_.TableOffset1  = (uint32_t)GetNextByte ( &retVal ); // LS Byte
      SaveByte               = GetNextByte ( &retVal );           // Mid-Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
      SaveByte               = GetNextByte ( &retVal );           // MS Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.DataLength    = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.DataLength   |= SaveByte;

      if ( eSUCCESS == retVal )
      {
         numRetries = PROG_METER_RETRIES;

         while ( ( numRetries != 0 ) && !done )
         {
            if ( Command_.DataLength > READ_BUFFER_SIZE )
            {
               PacketDataLength = READ_BUFFER_SIZE;
            }
            else
            {
               PacketDataLength = Command_.DataLength;
            }
            // Remaining Bytes after next read.
            RemainingBytes = Command_.DataLength - PacketDataLength;
            RdWrResult     = TableReadFunction_( (void *)pMeterBuffer->data,
                                                Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );

            if ( CIM_QUALCODE_SUCCESS == RdWrResult )
            {
               // compare expected data with data read from table.
               for ( i = 0; i < PacketDataLength; i++ )
               {
                  if ( *( pMeterBuffer->data + i ) != GetNextByte ( &retVal ) )
                  {
                     if ( eSUCCESS == retVal )
                     {
                        retVal = eFAILURE;
                     }
                     done = TRUE;
                     break;
                  }
               }
               if ( !done )
               {
                  if ( RemainingBytes == 0 )
                  {
                     done = TRUE;
                  }
                  else
                  {
                     Command_.TableOffset1 += PacketDataLength;
                     Command_.DataLength    = RemainingBytes;
                  }
               }
            } /* end of "if ( CIM_QUALCODE_SUCCESS == RdWrResult )" */
            else
            {
               if ( --numRetries == 0 )
               {
                  if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                  {
                     retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                  }
                  else
                  {
                     retVal = ePRG_MTR_METER_COMM_FAILURE;
                  }
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Read Failed" );
               }
            }
         }
      }
      BM_free ( pMeterBuffer );
   }

   return retVal;
}

/************************************************************************

 Function Name: PerformWD

 Purpose: Performs a write command.
          Writes a given number of bytes to a table and offset.
          The command format is:
            < command ID >	    1 byte
            < table number >	2 bytes
            < offset >		    3 bytes
            < length to write >	2 bytes
            < data byte >
                . 		“length” number of bytes
            < data byte >

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table write

************************************************************************/
static returnStatus_t PerformWD ( void )
{
   returnStatus_t        retVal = eSUCCESS;
   buffer_t             *pMeterBuffer;
   uint8_t               numRetries;
   uint8_t               SaveByte;
   bool                  done = false;
   uint16_t              PacketDataLength;
   uint16_t              i;
   enum_CIM_QualityCode  RdWrResult;
   uint16_t              RemainingBytes;

   pMeterBuffer = BM_alloc ( WRITE_BUFFER_SIZE );
   if ( NULL == pMeterBuffer )
   {
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
   } /* end of "if" */
   else
   {
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Write data" );
      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.TableNumber1  = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.TableNumber1 |= SaveByte;

      Command_.TableOffset1  = (uint32_t)GetNextByte ( &retVal ); // LS Byte
      SaveByte               = GetNextByte ( &retVal ); // Mid-Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
      SaveByte               = GetNextByte ( &retVal );// MS Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.DataLength    = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.DataLength   |= SaveByte;

      if ( eSUCCESS == retVal )
      {
         numRetries = PROG_METER_RETRIES;
         while ( ( numRetries != 0 ) && !done )
         {
            if ( Command_.DataLength > WRITE_BUFFER_SIZE )
            {
               PacketDataLength = WRITE_BUFFER_SIZE;
            } /* end of "if" */
            else
            {
               PacketDataLength = Command_.DataLength;
            } /* end of "else" */
            // Remaining Bytes after next write.
            RemainingBytes = Command_.DataLength - PacketDataLength;
            if( PROG_METER_RETRIES == numRetries )
            {
              for ( i = 0; i < PacketDataLength; i++ )
              {
                *( pMeterBuffer->data + i ) = GetNextByte ( &retVal );
              } /* end of "for" */
            }

            if ( eSUCCESS != retVal )
            {
               break;
            }

            RdWrResult = TableWriteFunction_( (void *)pMeterBuffer->data,
                                             Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );
            if ( CIM_QUALCODE_SUCCESS == RdWrResult )
            {
               if ( RemainingBytes == 0 )
               {
                  done = TRUE;
               } /* end of "if" */
               else
               {
                  Command_.TableOffset1 += PacketDataLength;
                  Command_.DataLength    = RemainingBytes;
               } /* end of "else" */
            } /* end of "if" */
            else
            {
               if ( --numRetries == 0 )
               {
                  if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                  {
                     retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                  }
                  else
                  {
                     retVal = eFAILURE; // ePRG_MTR_METER_COMM_FAILURE;
                  }
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Write Failed" );
               } /* end of "if" */
            } /* end of "else" */
         } /* end of "while" */
      } /* end of "if ( eSUCCESS == retVal )" */
      BM_free ( pMeterBuffer );
   } /* end of "else" */

   return retVal;
}

/************************************************************************

 Function Name: PerformRBS

 Purpose: Read and Bit-verify command, single mask
          Reads a given number of bytes from a table and offset, and validates
          they all match the bit pattern in the masks. The mask given at the
          head end is used to create the 2 masks in the parameter list, used
          for all bytes. The head end * symbol is used for bits not needing
          validation.
          The command format is:
            < command ID >	    1 byte 	   	ones_mask:
            < table number >	2 bytes		  pattern 1010**** -> mask 10100000
            < offset >		    3 bytes		zeroes_mask:
            < length to read >	2 bytes	      pattern 1010**** -> mask 01010000
            < ones_mask >		1 byte		mask to verify bits that should be 1
            < zeroes_mask >	    1 byte		mask to verify bits that should be 0

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table read and verify bit(s)

************************************************************************/
static returnStatus_t PerformRBS ( void )
{
   returnStatus_t        retVal = eSUCCESS;
   buffer_t             *pMeterBuffer;
   uint8_t               numRetries;
   uint8_t               SaveByte;
   bool                  done = false;
   uint16_t              PacketDataLength;
   uint16_t              i;
   enum_CIM_QualityCode  RdWrResult;
   uint16_t              RemainingBytes;

   uint8_t               TableByte;
   uint8_t               temp8;


   pMeterBuffer = BM_alloc ( READ_BUFFER_SIZE );
   if ( NULL == pMeterBuffer )
   {
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
   }
   else
   {
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Read Bit Verify - Single Mask" );
      SaveByte               = (uint16_t)GetNextByte ( &retVal );          // LS byte
      Command_.TableNumber1  = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.TableNumber1 |= SaveByte;

      Command_.TableOffset1  = (uint32_t)GetNextByte ( &retVal );          // LS Byte
      SaveByte               = GetNextByte ( &retVal );                    // Mid-Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
      SaveByte               = GetNextByte ( &retVal );                    // MS Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

      SaveByte               = (uint16_t)GetNextByte ( &retVal );          // LS byte
      Command_.DataLength    = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.DataLength   |= SaveByte;

      Command_.OnesMask      = GetNextByte ( &retVal );
      Command_.ZeroesMask    = GetNextByte ( &retVal );

      if ( eSUCCESS == retVal )
      {
         numRetries = PROG_METER_RETRIES;

         while ( ( numRetries != 0 ) && !done )
         {
            if ( Command_.DataLength > READ_BUFFER_SIZE )
            {
               PacketDataLength = READ_BUFFER_SIZE;
            }
            else
            {
               PacketDataLength = Command_.DataLength;
            }
            // Remaining Bytes after next read.
            RemainingBytes = Command_.DataLength - PacketDataLength;
            RdWrResult     = TableReadFunction_( (void *)pMeterBuffer->data,
                                                Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );
            if ( CIM_QUALCODE_SUCCESS == RdWrResult )
            {
               // compare expected data with data read from table.
               for ( i = 0; i < PacketDataLength; i++ )
               {
                  TableByte = *( pMeterBuffer->data + i );
                  temp8     = TableByte & Command_.OnesMask;
                  if ( ( temp8 ^ Command_.OnesMask ) != 0 )
                  {
                     retVal = eFAILURE;
                     done   = TRUE;
                     break;
                  }
                  else if ( ( TableByte & Command_.ZeroesMask ) != 0 )
                  {
                     retVal = eFAILURE;
                     done   = TRUE;
                     break;
                  }
               } /* end of "for" */
               if ( !done )
               {
                  if ( RemainingBytes == 0 )
                  {
                     done = TRUE;
                  }
                  else
                  {
                     Command_.TableOffset1 += PacketDataLength;
                     Command_.DataLength    = RemainingBytes;
                  }
               } /* end of "if ( !done )" */
            } /* end of "if ( CIM_QUALCODE_SUCCESS == RdWrResult )" */
            else
            {
               if ( --numRetries == 0 )
               {
                  if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                  {
                     retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                  }
                  else
                  {
                     retVal = ePRG_MTR_METER_COMM_FAILURE;
                  }
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Read Failed" );
               }
            }
         } /* end of "while ( ( numRetries != 0 ) && !done )" */
      } /* end of "if ( eSUCCESS == retVal )" */
      BM_free ( pMeterBuffer );
   } /* end of "else" */

   return retVal;
}


/************************************************************************

 Function Name: PerformRBM

 Purpose: Read and Bit-verify command, multiple masks
          Reads a given number of bytes from a table and offset, and validates
          they all match the bit pattern in their respective masks. Each mask
          given at the head end is used to create the 2 masks in the parameter
          list (a 2 mask set for each data byte).
          The head end * symbol is used for bits not needing validation.
          The command format is:
            < command ID >	    1 byte 	   	ones_mask:
            < table number >	2 bytes		  pattern 1010**** -> mask 10100000
            < offset >		    3 bytes		zeroes_mask:
            < length to read >	2 bytes	      pattern 1010**** -> mask 01010000
            < ones_mask >		1 byte		mask to verify bits that should be 1
            < zeroes_mask >	    1 byte		mask to verify bits that should be 0
            < ones_mask >		1 byte		for second data byte
            < zeroes_mask >	    1 byte		for second data byte
                .
                . 		parameter list is “length” number of bytes x 2
                .
            < ones_mask >		1 byte		for last data byte
            < zeroes_mask > 	1 byte		for last data byte

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table read and verify bit(s)

************************************************************************/
static returnStatus_t PerformRBM ( void )
{
   returnStatus_t        retVal = eSUCCESS;
   buffer_t             *pMeterBuffer;
   uint8_t               numRetries;
   uint8_t               SaveByte;
   bool                  done = false;
   uint16_t              PacketDataLength;
   uint16_t              i;
   enum_CIM_QualityCode  RdWrResult;
   uint16_t              RemainingBytes;

   uint8_t               TableByte;
   uint8_t               temp8;

   pMeterBuffer = BM_alloc ( READ_BUFFER_SIZE );
   if ( NULL == pMeterBuffer )
   {
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
   }
   else
   {
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Read Bit Verify - Multiple Masks" );
      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.TableNumber1  = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.TableNumber1 |= SaveByte;

      Command_.TableOffset1  = (uint32_t)GetNextByte ( &retVal ); // LS Byte
      SaveByte               = GetNextByte ( &retVal ); // Mid-Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
      SaveByte               = GetNextByte ( &retVal );// MS Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.DataLength    = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.DataLength   |= SaveByte;

      if ( eSUCCESS == retVal )
      {
         numRetries = PROG_METER_RETRIES;

         while ( ( numRetries != 0 ) && !done )
         {
            if ( Command_.DataLength > READ_BUFFER_SIZE )
            {
               PacketDataLength = READ_BUFFER_SIZE;
            }
            else
            {
               PacketDataLength = Command_.DataLength;
            }
            // Remaining Bytes after next read.
            RemainingBytes = Command_.DataLength - PacketDataLength;
            RdWrResult     = TableReadFunction_( (void *)pMeterBuffer->data,
                                                Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );
            if ( CIM_QUALCODE_SUCCESS == RdWrResult )
            {
               // compare expected data with data read from table.
               for ( i = 0; i < PacketDataLength; i++ )
               {
                  // Read 2 bytes for each increment of 1 DataLength
                  Command_.OnesMask   = GetNextByte ( &retVal );
                  Command_.ZeroesMask = GetNextByte ( &retVal );
                  if ( eSUCCESS == retVal )
                  {
                     TableByte = *( pMeterBuffer->data + i );
                     temp8     = TableByte & Command_.OnesMask;
                     if ( ( temp8 ^ Command_.OnesMask ) != 0 )
                     {
                        retVal = eFAILURE;
                        done   = TRUE;
                        break;
                     }
                     else if ( ( TableByte & Command_.ZeroesMask ) != 0 )
                     {
                        retVal = eFAILURE;
                        done   = TRUE;
                        break;
                     }
                  }
                  else
                  {
                     done = TRUE; // error getting OnesMask or ZeroesMask
                     break;
                  }
               } /* end of "for" */
               if ( !done )
               {
                  if ( RemainingBytes == 0 )
                  {
                     done = TRUE;
                  }
                  else
                  {
                     Command_.TableOffset1 += PacketDataLength;
                     Command_.DataLength    = RemainingBytes;
                  }
               }
            }
            else
            {
               if ( --numRetries == 0 )
               {
                  if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                  {
                     retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                  }
                  else
                  {
                     retVal = ePRG_MTR_METER_COMM_FAILURE;
                  }
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Read Failed" );
               }
            }
         } /* end of "while ( ( numRetries != 0 ) && !done )" */
      } /* end of "if ( eSUCCESS == retVal )" */
      BM_free ( pMeterBuffer );
   } /* end of "else" */
   return retVal;
}


/************************************************************************

 Function Name: PerformRR

 Purpose: Reads a given number of bytes from a table and offset, and validates
          they match the bytes read from a second table and offset.
          The command format is:
            < command ID >	1 byte
            < table number 1 >	2 bytes
            < offset 1 >		3 bytes
            < length to read >	2 bytes
            < table number 2 >	2 bytes
            < offset 2 >		3 bytes

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table compare.

************************************************************************/
static returnStatus_t PerformRR ( void )
{
   returnStatus_t        retVal = eSUCCESS;
   buffer_t             *pMeterBuffer;
   buffer_t             *pMeterBuffer2;
   uint8_t               numRetries;
   uint8_t               SaveByte;
   bool                  done = false;
   uint16_t              PacketDataLength;
   uint16_t              i;
   enum_CIM_QualityCode  RdWrResult;
   enum_CIM_QualityCode  RdWrResult2;
   uint16_t              RemainingBytes;

   pMeterBuffer  = BM_alloc ( READ_BUFFER_SIZE );
   pMeterBuffer2 = BM_alloc ( READ_BUFFER_SIZE );
   if ( ( NULL == pMeterBuffer ) || ( NULL == pMeterBuffer2 ) )
   {
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
      if ( NULL != pMeterBuffer )
      {
         BM_free ( pMeterBuffer );
      }
      // If pMeterBuffer failed, then pMeterBuffer2 should have failed.
      // If pMeterBuffer passed, then pMeterBuffer2 failed.
      // Thus, the following should not be needed, but is included for safety.
      if ( NULL != pMeterBuffer2 )
      {
         BM_free ( pMeterBuffer2 );
      }
   }
   else
   {
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Table Compare" );
      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.TableNumber1  = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.TableNumber1 |= SaveByte;

      Command_.TableOffset1  = (uint32_t)GetNextByte ( &retVal ); // LS Byte
      SaveByte               = GetNextByte ( &retVal ); // Mid-Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
      SaveByte               = GetNextByte ( &retVal );// MS Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.DataLength    = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.DataLength   |= SaveByte;

      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.TableNumber2  = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.TableNumber2 |= SaveByte;

      Command_.TableOffset2  = (uint32_t)GetNextByte ( &retVal ); // LS Byte
      SaveByte               = GetNextByte ( &retVal ); // Mid-Byte
      Command_.TableOffset2 |= ( (uint32_t)SaveByte ) << 8;
      SaveByte               = GetNextByte ( &retVal );// MS Byte
      Command_.TableOffset2 |= ( (uint32_t)SaveByte ) << 16;

      if ( eSUCCESS == retVal )
      {
         numRetries = PROG_METER_RETRIES;

         while ( ( numRetries != 0 ) && !done )
         {
            if ( Command_.DataLength > READ_BUFFER_SIZE )
            {
               PacketDataLength = READ_BUFFER_SIZE;
            }
            else
            {
               PacketDataLength = Command_.DataLength;
            }
            // Remaining Bytes after next read.
            RemainingBytes = Command_.DataLength - PacketDataLength;
            RdWrResult     = TableReadFunction_( (void *)pMeterBuffer->data,
                                                Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );
            RdWrResult2    = TableReadFunction_( (void *)pMeterBuffer2->data,
                                                Command_.TableNumber2, Command_.TableOffset2, PacketDataLength );
            if ( ( CIM_QUALCODE_SUCCESS == RdWrResult  ) &&
                 ( CIM_QUALCODE_SUCCESS == RdWrResult2 ) )
            {
               // compare expected data with data read from table.
               for ( i = 0; i < PacketDataLength; i++ )
               {
                  if ( *( pMeterBuffer->data + i ) != *( pMeterBuffer2->data + i ) )
                  {
                     retVal = eFAILURE;
                     done   = TRUE;
                     break;
                  }
               } /* end of "for" */
               if ( !done )
               {
                  if ( RemainingBytes == 0 )
                  {
                     done = TRUE;
                  }
                  else
                  {
                     Command_.TableOffset1 += PacketDataLength;
                     Command_.TableOffset2 += PacketDataLength;
                     Command_.DataLength    = RemainingBytes;
                  }
               } /* end of "if ( !done )" */
            } /* end of "if ( ( CIM_QUALCODE_SUCCESS == RdWrResult ) &&
            ( CIM_QUALCODE_SUCCESS == RdWrResult2 ) )" */
            else
            {
               if ( --numRetries == 0 )
               {
                  if ( ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult  ) ||
                       ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult2 ) )
                  {
                     retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                  }
                  else
                  {
                     retVal = ePRG_MTR_METER_COMM_FAILURE;
                  }
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Read Failed" );
               }
            }
         } /* end of "while ( ( numRetries != 0 ) && !done )" */
      } /* end of "if ( eSUCCESS == retVal )" */
      BM_free ( pMeterBuffer );
      BM_free ( pMeterBuffer2 );
   } /* end of "else" */
   return retVal;
}


/************************************************************************

 Function Name: PerformWBS

 Purpose: Write Bit-values command, single mask
          Modify a given number of bytes at a table and offset according to
          the masks. The mask given at the head end is used to create the 2
          masks in the parameter list, used for all bytes. The head end * symbol
          is used for bits not needing change. This is a read-modify-write.
          The command format is:
            < command ID >	    1 byte 	   	ones_mask:
            < table number >	2 bytes		  pattern 1010**** -> mask 10100000
            < offset >		    3 bytes		zeroes_mask:
            < length to read >	2 bytes	      pattern 1010**** -> mask 01010000
            < ones_mask >		1 byte		mask to set bits
            < zeroes_mask >	    1 byte		mask to clear bits

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table read-modify bit(s)-write.

************************************************************************/
static returnStatus_t PerformWBS ( void )
{
   returnStatus_t        retVal = eSUCCESS;
   buffer_t             *pMeterBuffer;
   uint8_t               numRetries;
   uint8_t               SaveByte;
   bool                  done = false;
   uint16_t              PacketDataLength;
   uint16_t              i;
   enum_CIM_QualityCode  RdWrResult;
   uint16_t              RemainingBytes;

   uint8_t               TableByte;
   uint8_t               temp8;

   // WRITE_BUFFER_SIZE is smaller than READ_BUFFER_SIZE,
   // so using WRITE_BUFFER_SIZE per packet
   pMeterBuffer = BM_alloc ( WRITE_BUFFER_SIZE );
   if ( NULL == pMeterBuffer )
   {
      retVal = ePRG_MTR_INTERNAL_ERROR;
      HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
   }
   else
   {
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Write Bits - Single Mask" );
      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.TableNumber1  = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.TableNumber1 |= SaveByte;

      Command_.TableOffset1  = (uint32_t)GetNextByte ( &retVal ); // LS Byte
      SaveByte               = GetNextByte ( &retVal ); // Mid-Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
      SaveByte               = GetNextByte ( &retVal );// MS Byte
      Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

      SaveByte               = (uint16_t)GetNextByte ( &retVal ); // LS byte
      Command_.DataLength    = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
      Command_.DataLength   |= SaveByte;

      Command_.OnesMask      = GetNextByte ( &retVal );
      Command_.ZeroesMask    = GetNextByte ( &retVal );

      if ( eSUCCESS == retVal )
      {
         numRetries = PROG_METER_RETRIES;

         while ( ( numRetries != 0 ) && !done )
         {
            if ( Command_.DataLength > WRITE_BUFFER_SIZE )
            {
               PacketDataLength = WRITE_BUFFER_SIZE;
            }
            else
            {
               PacketDataLength = Command_.DataLength;
            }
            // Remaining Bytes after next write.
            RemainingBytes = Command_.DataLength - PacketDataLength;
            RdWrResult     = TableReadFunction_( (void *)pMeterBuffer->data,
                                                Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );
            if ( CIM_QUALCODE_SUCCESS == RdWrResult )
            {
               for ( i = 0; i < PacketDataLength; i++ )
               {
                  TableByte = *( pMeterBuffer->data + i );
                  temp8     = TableByte | Command_.OnesMask; // Set ones
                  temp8    &= ~Command_.ZeroesMask;
                  *( pMeterBuffer->data + i ) = temp8;
               } /* end of "for" */
               RdWrResult = TableWriteFunction_( (void *)pMeterBuffer->data,
                                      Command_.TableNumber1, Command_.TableOffset1,
                                      PacketDataLength );
               if ( CIM_QUALCODE_SUCCESS == RdWrResult )
               {
                  if ( RemainingBytes == 0 )
                  {
                     done = TRUE;
                  }
                  else
                  {
                     Command_.TableOffset1 += PacketDataLength;
                     Command_.DataLength    = RemainingBytes;
                  }
               }
               else
               {
                  if ( --numRetries == 0 )
                  {
                     if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                     {
                        retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                     }
                     else
                     {
                        retVal = eFAILURE; //ePRG_MTR_METER_COMM_FAILURE;
                     }
                     HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Write Failed" );
                  }
               }
            } /* end of "if ( CIM_QUALCODE_SUCCESS == RdWrResult )" */
            else
            {
               if ( --numRetries == 0 )
               {
                  if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                  {
                     retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                  }
                  else
                  {
                     retVal = ePRG_MTR_METER_COMM_FAILURE;
                  }
                  HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Read Failed" );
               }
            }
         } /* end of "while ( ( numRetries != 0 ) && !done )" */
      } /* end of "if ( eSUCCESS == retVal )" */
      BM_free ( pMeterBuffer );
   } /* end of "else" */
   return retVal;
}

/************************************************************************

 Function Name: PerformWBM

 Purpose: Write Bit-values command, multiple masks
          Modify a given number of bytes at a table and offset according to
          their respective masks. Each mask given at the head end is used to
          create a set of 2 masks in the parameter list (a 2 mask set for each
          data byte).
          The head end * symbol is used for bits not needing change.
          This is a read-modify-write.
          The command format is:
            < command ID >	    1 byte 	   	ones_mask:
            < table number >	2 bytes		  pattern 1010**** -> mask 10100000
            < offset >		    3 bytes		zeroes_mask:
            < length to read >	2 bytes	      pattern 1010**** -> mask 01010000
            < ones_mask >		1 byte		mask to set bits, first data byte
            < zeroes_mask >	    1 byte		mask to clear bits, first data byte
            < ones_mask >		1 byte		for second data byte
            < zeroes_mask >	    1 byte		for second data byte
                .
                . 		parameter list is “length” number of bytes x 2
                .
            < ones_mask >		1 byte		for last data byte
            < zeroes_mask > 	1 byte		for last data byte

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table read-modify bit(s)-write.

************************************************************************/
static returnStatus_t PerformWBM ( void )
{
    returnStatus_t        retVal = eSUCCESS;
    buffer_t             *pMeterBuffer;
    uint8_t               numRetries;
    uint8_t               SaveByte;
    bool                  done = false;
    uint16_t              PacketDataLength;
    uint16_t              i;
    enum_CIM_QualityCode  RdWrResult;
    uint16_t              RemainingBytes;

    uint8_t               TableByte;
    uint8_t               temp8;

    // WRITE_BUFFER_SIZE is smaller than READ_BUFFER_SIZE,
    // so using WRITE_BUFFER_SIZE per packet
    pMeterBuffer = BM_alloc ( WRITE_BUFFER_SIZE );
    if ( NULL == pMeterBuffer )
    {
        retVal = ePRG_MTR_INTERNAL_ERROR;
        HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
    }
    else
    {
        HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Write Bits - Multiple Masks" );
        SaveByte = (uint16_t)GetNextByte ( &retVal ); // LS byte
        Command_.TableNumber1 = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
        Command_.TableNumber1 |= SaveByte;

        Command_.TableOffset1 = (uint32_t)GetNextByte ( &retVal ); // LS Byte
        SaveByte = GetNextByte ( &retVal ); // Mid-Byte
        Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
        SaveByte = GetNextByte ( &retVal );// MS Byte
        Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

        SaveByte = (uint16_t)GetNextByte ( &retVal ); // LS byte
        Command_.DataLength = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
        Command_.DataLength |= SaveByte;

        if ( eSUCCESS == retVal )
        {
            numRetries = PROG_METER_RETRIES;

            while ( ( numRetries != 0 ) && !done )
            {
                if ( Command_.DataLength > WRITE_BUFFER_SIZE )
                {
                    PacketDataLength = WRITE_BUFFER_SIZE;
                }
                else
                {
                    PacketDataLength = Command_.DataLength;
                }
                // Remaining Bytes after next write.
                RemainingBytes = Command_.DataLength - PacketDataLength;

                RdWrResult = TableReadFunction_( (void *)pMeterBuffer->data,
                 Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );
                if ( CIM_QUALCODE_SUCCESS == RdWrResult )
                {
                    // set ones and zeroes in data read from table.
                    for ( i = 0; i < PacketDataLength; i++ )
                    {
                        // Read 2 bytes for each increment of 1 DataLength
                        Command_.OnesMask = GetNextByte ( &retVal );
                        Command_.ZeroesMask = GetNextByte ( &retVal );
                        TableByte = *( pMeterBuffer->data + i );
                        temp8 = TableByte | Command_.OnesMask; // Set ones
                        temp8 &= ~Command_.ZeroesMask;
                        *( pMeterBuffer->data + i ) = temp8;
                    } /* end of "for" */
                    if ( eSUCCESS == retVal )
                    {
                        RdWrResult =
                          TableWriteFunction_( (void *)pMeterBuffer->data,
                          Command_.TableNumber1, Command_.TableOffset1,
                          PacketDataLength );
                        if ( CIM_QUALCODE_SUCCESS == RdWrResult )
                        {
                            if ( RemainingBytes == 0 )
                            {
                                done = TRUE;
                            }
                            else
                            {
                                Command_.TableOffset1 += PacketDataLength;
                                Command_.DataLength = RemainingBytes;
                            }
                        }
                        else
                        {
                            if ( --numRetries == 0 )
                            {
                                if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                                {
                                    retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                                }
                                else
                                {
                                    retVal = eFAILURE; //ePRG_MTR_METER_COMM_FAILURE;
                                }
                                HMC_PRG_MTR_PRNT_ERROR( 'E',
                                  "Meter Write Failed" );
                            }
                        }
                    }
                    else
                    {
                        done = TRUE; // error getting OnesMask or ZeroesMask
                    }
                } /* end of "if ( CIM_QUALCODE_SUCCESS == RdWrResult )" */
                else
                {
                    if ( --numRetries == 0 )
                    {
                        if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                        {
                            retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                        }
                        else
                        {
                            retVal = ePRG_MTR_METER_COMM_FAILURE;
                        }
                        HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Read Failed" );
                    }
                }
            } /* end of "while ( ( numRetries != 0 ) && !done )" */
        } /* end of "if ( eSUCCESS == RetVal )" */
        BM_free ( pMeterBuffer );
    } /* end of "else" */

    return retVal;
}


/************************************************************************

 Function Name: PerformRW

 Purpose: Writes a given number of bytes from one table and offset into a
          second table and offset.
          The command format is:
            < command ID >	    1 byte
            < table number 1 >	2 bytes
            < offset 1 >		3 bytes
            < length to write >	2 bytes
            < table number 2 >	2 bytes
            < offset 2 >		3 bytes

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_, Command_

 Re-entrant Code: No

 Notes: This is a table copy.

************************************************************************/
static returnStatus_t PerformRW ( void )
{
    returnStatus_t        retVal = eSUCCESS;
    buffer_t              *pMeterBuffer;
    uint8_t               numRetries;
    uint8_t               SaveByte;
    bool                  done = false;
    uint16_t              PacketDataLength;
    enum_CIM_QualityCode  RdWrResult;

    pMeterBuffer  = BM_alloc ( WRITE_BUFFER_SIZE );
    if ( NULL == pMeterBuffer )
    {
        retVal = ePRG_MTR_INTERNAL_ERROR;
        HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
    }
    else
    {
        HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Table Copy" );
        SaveByte = (uint16_t)GetNextByte ( &retVal ); // LS byte
        Command_.TableNumber1 = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
        Command_.TableNumber1 |= SaveByte;

        Command_.TableOffset1 = (uint32_t)GetNextByte ( &retVal ); // LS Byte
        SaveByte = GetNextByte ( &retVal ); // Mid-Byte
        Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 8;
        SaveByte = GetNextByte ( &retVal );// MS Byte
        Command_.TableOffset1 |= ( (uint32_t)SaveByte ) << 16;

        SaveByte = (uint16_t)GetNextByte ( &retVal ); // LS byte
        Command_.DataLength = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
        Command_.DataLength |= SaveByte;

        SaveByte = (uint16_t)GetNextByte ( &retVal ); // LS byte
        Command_.TableNumber2 = ( (uint16_t)GetNextByte ( &retVal ) ) << 8;
        Command_.TableNumber2 |= SaveByte;

        Command_.TableOffset2 = (uint32_t)GetNextByte ( &retVal ); // LS Byte
        SaveByte = GetNextByte ( &retVal ); // Mid-Byte
        Command_.TableOffset2 |= ( (uint32_t)SaveByte ) << 8;
        SaveByte = GetNextByte ( &retVal );// MS Byte
        Command_.TableOffset2 |= ( (uint32_t)SaveByte ) << 16;

        if ( eSUCCESS == retVal )
        {
            numRetries = PROG_METER_RETRIES;

            while ( ( numRetries != 0 ) && !done )
            {
                // WRITE_BUFFER_SIZE is smaller than READ_BUFFER_SIZE,
                // so using WRITE_BUFFER_SIZE per packet
                if ( Command_.DataLength > WRITE_BUFFER_SIZE )
                {
                    PacketDataLength = WRITE_BUFFER_SIZE;
                }
                else
                {
                    PacketDataLength = Command_.DataLength;
                }

                // read data from table 1
                RdWrResult = TableReadFunction_( (void *)pMeterBuffer->data,
                 Command_.TableNumber1, Command_.TableOffset1, PacketDataLength );
                if ( CIM_QUALCODE_SUCCESS == RdWrResult )
                {
                    // write data to table 2.
                    RdWrResult =
                      TableWriteFunction_( (void *)pMeterBuffer->data,
                      Command_.TableNumber2, Command_.TableOffset2,
                      PacketDataLength );

                    if ( CIM_QUALCODE_SUCCESS == RdWrResult )
                    {
                        Command_.DataLength -= PacketDataLength;
                        if ( Command_.DataLength == 0 )
                        {
                            done = TRUE;
                        }
                        else
                        {
                            Command_.TableOffset1 += PacketDataLength;
                            Command_.TableOffset2 += PacketDataLength;
                        }
                    }
                    else
                    {
                        if ( --numRetries == 0 )
                        {
                            if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                            {
                                retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                            }
                            else
                            {
                                retVal = eFAILURE; //ePRG_MTR_METER_COMM_FAILURE;
                            }
                            HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Write Failed" );
                        }
                    }
                }
                else
                {
                    if ( --numRetries == 0 )
                    {
                        if ( CIM_QUALCODE_NOT_TESTABLE == RdWrResult )
                        {
                            retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
                        }
                        else
                        {
                            retVal = ePRG_MTR_METER_COMM_FAILURE;
                        }
                        HMC_PRG_MTR_PRNT_ERROR( 'E', "Meter Read Failed" );
                    }
                }
            } /* end of "while ( ( numRetries != 0 ) && !done )" */
        } /* end of "if ( eSUCCESS == retVal )" */
        BM_free ( pMeterBuffer );
    } /* end of "else" */
    return retVal;
}

/************************************************************************

Function Name: PerformPR

Purpose: Execute a meter procedure.
          The command format is:
            < command ID >          1 byte (retrieved by calling function)
            < procedure number >  2 bytes (selector(0)/S=0,M=1/procedure number)
            < timeout > 1 byte   (only for Timeout eProcForm based on command ID)
            < length of attributes >  1 byte
            < data attribute >
                .             “length” number of attributes (max = 8)
            < data attribute >

Arguments: eProcForm  Form of procedure command to perform

Returns: returnStatus_t

Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_,
               ProcedureAttributes

Re-entrant Code: No

Notes: See Notes below for special handling of SP10 and MP70

************************************************************************/
static returnStatus_t PerformPR ( ProcForm_e eForm )
{
#define MAX_BUFFER_CNT    max(STD_TBL_PROC_INIT_MAX_PARAMS, STD_TBL_PROC_RESP_MAX_BYTES)   //Largest buffer needed

   returnStatus_t         retVal = eSUCCESS;
   uint16_t               proc; // Procedure Number
   uint8_t                cnt;  // Number of procedure parameters.  NOTE: will be 6 for MP70 or 8 for SP10
   uint8_t                i;
   enum_CIM_QualityCode   ProcedureResult;
   // Buffer for Procedure Parameters
   uint8_t               dataBuffer[MAX_BUFFER_CNT]; /*lint !e506 */

   (void)memset( dataBuffer, 0, sizeof(dataBuffer) );

   HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Procedure call" );

   ProcTimeout_ = 0;    // Default is zero
   if ( eEXEC_PROC_TIMEOUT == eForm )
   {
      ProcTimeout_ = GetNextByte( &retVal );
   }

   proc                  =   (uint16_t)GetNextByte( &retVal );
   proc                 |= ( (uint16_t)GetNextByte( &retVal ) ) << 8;
   Command_.TableNumber1 = proc; // RCZ added for error message

   cnt = GetNextByte( &retVal );
   for ( i = 0; i < cnt; i++ )
   {
      dataBuffer[i] = GetNextByte( &retVal );
   }
   if ( eSUCCESS == retVal )
   {
      if ( ( MFG_PROC_PGM_CTRL             == proc ) || // MP70 - Program Control
           ( STD_PROC_SET_DATE_AND_OR_TIME == proc ) )  // SP10 - Set Date/Time
      {
         /* SP10:
               Byte 0: Value to set: (will remain as received)
                        Bit 0 = Set Time
                        Bit 1 = Set Date
                        Bit 2 = Set Date/Time
               Bytes 1-3: Date (YYMMDD) (Ignored if only setting time?)
               Bytes 4-6: Time (HHMMSS) (Ignored if only setting date?)
               Byte 7: Time/Date Qualifier:
                        Bits 0-2 = Day of Week (0 = Sunday, ...6 = Saturday)
                        Bit 3    = DST Flag (0 = Winter, 1 = Summer)
                        Bits 4-7 = don't care
            MP70:
               Byte 0: Action to perform: (will remain as received)
                        0=Start Programming
                        1=Stop Programming/Clear Max Demands/Logoff
                        2=Stop Programming/No clear/Logoff
                        3=Stop Programming/No clear/No Logoff
                        4=Stop Programming/Clear Max Demands/No Logoff
               Bytes 1-3: Date of Programming (YYMMDD)
               Bytes 4-5: Time of Programming (HHMM)
         Byte 0 will be taken from provided procedure parameters
         Bytes 1-7 (SP10) or 1-6 (MP70) will be changed to be current LOCAL time
         The received procedure parameters cnt is expected to be the desired bytes to send
         */

         setDateTimeProcParam_t  *procParams;      /* ptr to struct of parameters sent with ST10 or MP70. */
         sysTime_t               sysTime;          /* days, time(in mS), tictoc, elapsed */
         sysTimeCombined_t       combSysTime;      /* Contains the Combined Date/Time */

         // Procedure parameters byte 0 remain as received.
         procParams = (setDateTimeProcParam_t *)&dataBuffer[0];
         /* Get current time. Ignore possible INVALID_TIME return, since time had to be valid prior to getting here */
         ( void )TIME_SYS_GetSysDateTime( &sysTime );

         /* Round to the nearest second. */
         combSysTime = TIME_UTIL_ConvertSysFormatToSysCombined( &sysTime );
         combSysTime = TIME_TICKS_PER_SEC * ( ( combSysTime + TIME_TICKS_PER_SEC ) / TIME_TICKS_PER_SEC );
         TIME_UTIL_ConvertSysCombinedToSysFormat( &combSysTime, &sysTime );

         TIME_UTIL_ConvertSysFormatToMtrHighFormat( &sysTime, &procParams->DateTime );  /* Seconds not used by MP70 */
         if ( STD_PROC_SET_DATE_AND_OR_TIME == proc )
         {
            /* Set all parameters for the 7th byte */
            procParams->gmt          = 0;
            procParams->tmZnApplied  = 0;
            procParams->dstApplied   = 0;
            procParams->dstSupported = 0;
            procParams->dayOfWeek    = ( sysTime.date + DAY_AT_DATE_0 ) % TIME_NUM_DAYS_IN_WEEK;

            if( DST_IsActive() )
            {  // DST is active, set the dst bit in the ST10 Procedure Command Structure
               procParams->dst = 1;
            }
         }
      }

      ProcedureResult = TableProcedureFunction_( (void *)dataBuffer, proc, cnt );
      if ( CIM_QUALCODE_SUCCESS != ProcedureResult )
      {
         /* TODO:
            The MP83 procedure call only fails, due to an ISSS PSEM error (invalid state), when exiting DL mode not when entering
            DL mode. However the meter has updated and set the UPDATE_ACTIVE_FLAG.  The error was observed when using the NIC, but
            not when using MeterMate (only tested on  I210+c firmware 6.0.7).  The download process has been captured using
            Meter Mate and the NIC.  The only significant difference is MeterMate starts and terminates sessions at various stages
            of the process.

            Need to understand the meter operations at this point and determine method of performing final evaluation.
         */
         if ( 0x0853 != proc )
         {
            if ( CIM_QUALCODE_NOT_TESTABLE == ProcedureResult )
            {
               retVal = ePRG_MTR_SECTION_NOT_TESTABLE;
            }
            else
            {
               retVal = eFAILURE;
            }
            HMC_PRG_MTR_PRNT_ERROR( 'E',  "Procedure Failed" );
         }
      }
   } /* end of "if ( eSUCCESS == retVal )" */

   return retVal;
}

/************************************************************************

 Function Name: HMC_PRG_MTR_GetProcedureTimeout

 Purpose: Gets the timeout desired included in the meter procedure command.

 Arguments: none

 Returns: uint8_t - Desired timeout in seconds

 Side Effects: none

 Re-entrant Code: Yes

 Notes:

************************************************************************/
uint8_t  HMC_PRG_MTR_GetProcedureTimeout ( void )
{
   return ProcTimeout_;
}

/************************************************************************

 Function Name: PerformDY

 Purpose: Delay the given number of seconds.
          The command format is:
            < command ID >	        1 byte
            < number of seconds >	2 bytes

 Arguments: none

 Returns: returnStatus_t

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_

 Re-entrant Code: No

 Notes: Execute a delay (seconds)

************************************************************************/
static returnStatus_t PerformDY ( DelayForm_e eForm )
{
   returnStatus_t   retVal = eSUCCESS;
   uint32_t         DelayMSeconds;


   DelayMSeconds = (uint32_t)GetNextByte ( &retVal );
   DelayMSeconds |= ( (uint32_t)GetNextByte ( &retVal ) ) << 8;

   if( DelayMSeconds == 60 )
   {
     eForm = eEXEC_DELAY_LOGOFF;
   }
   DelayMSeconds *= 1000;
   HMC_PRG_MTR_PRNT_INFO( 'I',  "Cmd: Delay %u milliseconds", DelayMSeconds);


   if ( eSUCCESS == retVal )
   {
      HMC_PRG_MTR_PRNT_ERROR( 'I', "Delay ..." );
      OS_TASK_Sleep ( DelayMSeconds );
   }

   if ( eEXEC_DELAY_LOGOFF == eForm )
   {
      /* TODO:
               Tried to do add a Logoff after erasing, but HMC times out.  It LOOKS like it sent the terminate
               (HMC_TSK Send:EE000000000121), but the meter never responds.  So did the message actually get sent?  Will need to
               fix the logoff problem before able to determine if having multiple sessions eliminates the ISSS error.
               Found hmcPrgMtrIsHoldOff_ may be the fix, but needs to have END_DEVICE_PROGRAMMING_FLASH = 1, which was not tried
      */
      //Logoff
      // End Synchronization with applet list
      hmcPrgMtrIsHoldOff_ = true;
      hmcPrgMtrIsStarted_ = false;
      // Wait for applet list to un-sync (terminate?)
      while ( hmcPrgMtrIsSynced_ )
      {
         // Temporarily Suspend task.
         OS_TASK_Sleep( SUSPEND_TIME_FOR_PSEUDO_SEM );
      }
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Session Terminated" );

      /****************************************************************
        Re-start a communication session with Master Security Password.
      ****************************************************************/
      hmcPrgMtrIsStarted_ = true;
      // Wait for applet list to sync
      // Syncing starts a new communication session with Master
      //   Security Password.
      while ( !hmcPrgMtrIsSynced_ )
      {
         // Temporarily Suspend task.
         OS_TASK_Sleep( SUSPEND_TIME_FOR_PSEUDO_SEM );
      }
      hmcPrgMtrIsHoldOff_ = false;
      HMC_PRG_MTR_PRNT_INFO( 'I',  "Logged back in with Master password" );
   }
   return retVal;
}
#endif // End  #if ( END_DEVICE_PROGRAMMING_CONFIG == 1 )


#if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )

/************************************************************************

 Function Name: CreateFileBuffer

 Purpose: Allocates the largest size RAM buffer possible from buffer pool.
          File in NVRAM is read into this buffer N bytes at a time, where N
          is the RAM buffer size.

 Arguments: none

 Returns: eSUCCESS or ePRG_MTR_INTERNAL_ERROR (not enough memory)

 Side Effects: FileBufferSize_, pFileBuffer_

 Re-entrant Code: No

 Notes: Buffer pool only services 3 sizes of buffers

************************************************************************/
static returnStatus_t  CreateFileBuffer ( void )
{
#define MID_BUFFER_SIZE  256 // Size of mid-size buffer in pool of buffers
#define MIN_BUFFER_SIZE   48 // Size of smallest buffer in pool of buffers

    returnStatus_t  retVal = eSUCCESS;

    // Attempt to allocate a buffer from buffer pool.
    // Want buffer to be as large as possible. Try largest size first.
    pFileBuffer_ = BM_alloc ( MAX_BUFFER_SIZE );
    if ( NULL == pFileBuffer_ )
    {
        // Largest buffer not available. Try next largest size buffer.
        pFileBuffer_ = BM_alloc ( MID_BUFFER_SIZE );
        if ( NULL == pFileBuffer_ )
        {
            // Mid-size buffer not available. Try smallest size buffer.
            pFileBuffer_ = BM_alloc ( MIN_BUFFER_SIZE );
            if ( NULL == pFileBuffer_ )
            {
                // No buffers available from buffer pool.
                retVal = ePRG_MTR_INTERNAL_ERROR; //NOT_ENOUGH_MEMORY;
                HMC_PRG_MTR_PRNT_ERROR( 'E', "Not Enough Memory" );
                FileBufferSize_ = 0;
            } /* end of "if ( NULL == pFileBuffer_ )" for MIN_BUFFER_SIZE */
            else
            {
                // Allocated small buffer from pool
                FileBufferSize_ = MIN_BUFFER_SIZE;
            }
        } /* end of "if ( NULL == pFileBuffer_ )" for MID_BUFFER_SIZE */
        else
        {
            // Allocated mid-size buffer from pool
            FileBufferSize_ = MID_BUFFER_SIZE;
        }
    } /* end of "if ( NULL == pFileBuffer_ )" for MAX_BUFFER_SIZE */
    else
    {
        // Allocated largest buffer from pool
        FileBufferSize_ = MAX_BUFFER_SIZE;
    }

    return retVal;
}



#ifndef DEBUGGING_WRITE_TO_METER_FLASH
/************************************************************************

 Function Name: InitGetNextByte

 Purpose: Initially fills the buffer, then returns first byte. Use in place of
          first GetNextByte ().

 Arguments: returnStatus_t  *pRetVal - the Get either passes or fails

 Returns: uint8_t - the Next Byte read from the file buffer

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_

 Re-entrant Code: No

 Notes: FileSize will be N bytes, starting at byte 1 and ending with byte N.
        The FileOffset_ will run from 0 to (N-1).

************************************************************************/
static uint8_t InitGetNextByte ( returnStatus_t  *pRetVal )
{
    FileBufferIndex_ = FileBufferSize_; // Forces initial filling of buffer
    return ( GetNextByte ( pRetVal ) );
}

/************************************************************************

 Function Name: GetNextByte

 Purpose: Returns the next byte from the file via the file buffer.

 Arguments: returnStatus_t  *pRetVal - the Get either passes or fails

 Returns: uint8_t - the Next Byte read from the file buffer
          via pRetVal, ePRG_MTR_FILE_FORMAT_ERROR or ePRG_MTR_INTERNAL_ERROR or
                       remains unchanged at eSUCCESS

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_

 Re-entrant Code: No

 Notes: FileSize will be N bytes, starting at byte 1 and ending with byte N.
        The FileOffset_ will run from 0 to (N-1).

        If previous GetNextByte() failed (*pRetVal != eSUCCESS), then skip this
        GetNextByte(). This implementation removes the need to check for an
        error after each call to GetNextByte(); instead, we only need to check
        once following a series of calls to GetNextByte().

************************************************************************/
static uint8_t GetNextByte ( returnStatus_t  *pRetVal )
{
    uint8_t  NextByte;

    // If previous GetNextByte() failed (*pRetVal != eSUCCESS), then skip this
    // GetNextByte(). This implementation removes the need to check for an error
    // after each call to GetNextByte(); instead, we only need to check once
    // following a series of calls to GetNextByte().
    if ( eSUCCESS == *pRetVal )
    {
        // (1) Check for an error condition
        if ( RemainingFileBytesToRead_ <= 0 )
        {
            *pRetVal = ePRG_MTR_FILE_FORMAT_ERROR;
            HMC_PRG_MTR_PRNT_ERROR( 'E', "Exceeded Section Length" );

        }
        // (2) Check if we've exhausted the buffer (read all data in buffer)
        else if ( FileBufferIndex_ >= FileBufferSize_ )
        {
            // Initial fill / subsequent Refill of buffer
            FileBufferIndex_ = 0;
            // Check to see if more bytes, or the same number of bytes, as the
            // buffer size, remain to be read.
            if ( RemainingFileBytesToRead_ >= (int32_t)FileBufferSize_ )
            {
                // Refill entire buffer
                if ( eSUCCESS != PAR_partitionFptr.parRead( pFileBuffer_->data,
                  FileOffset_, FileBufferSize_, pPTbl_ ) )
                {
                    *pRetVal = ePRG_MTR_INTERNAL_ERROR;
                    HMC_PRG_MTR_PRNT_ERROR( 'E', "File Read Failed" );
                }
                //RemainingFileBytesToRead_ -= (int32_t)BufferSize;
                //FileOffset_ += BufferSize;
            }
            else
            {
                // Partially refill buffer with RemainingFileBytesToRead_.
                if ( eSUCCESS != PAR_partitionFptr.parRead( pFileBuffer_->data,
                  FileOffset_, (uint32_t)RemainingFileBytesToRead_, pPTbl_ ) )
                {
                    *pRetVal = ePRG_MTR_INTERNAL_ERROR;
                    HMC_PRG_MTR_PRNT_ERROR( 'E', "File Read Failed" );
                }
                //RemainingFileBytesToRead_ = 0;
            }
        } /* end of "else if ( FileBufferIndex_ >= FileBufferSize_ )" */

        // (3) Return next byte if no error
        if ( eSUCCESS == *pRetVal )
        {
            NextByte = ( *( pFileBuffer_->data + FileBufferIndex_ ) );
            FileBufferIndex_++;
            FileOffset_++;
            RemainingFileBytesToRead_--;
        }
        else
        {
            HMC_PRG_MTR_PRNT_ERROR( 'E',  "Error Getting Next Byte" );
            NextByte = 0;
        }
    } /* end of "if ( eSUCCESS == *pRetVal )" */
    else
    {
        NextByte = 0;
    }

    return NextByte;
}

#else    // #ifndef DEBUGGING_WRITE_TO_METER_FLASH

static uint16_t IndexGet;

/************************************************************************

 Function Name: InitGetNextByte

 Purpose: Initially fills the buffer, then returns first byte. Use in place of
          first GetNextByte ().

 Arguments: returnStatus_t  *pRetVal - the Get either passes or fails

 Returns: uint8_t - the Next Byte read from the file buffer

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_

 Re-entrant Code: No

 Notes: FileSize will be N bytes, starting at byte 1 and ending with byte N.
        The FileOffset_ will run from 0 to (N-1).

************************************************************************/
static uint8_t InitGetNextByte ( returnStatus_t  *pRetVal )
{
    IndexGet = 0;
    return ( GetNextByte ( pRetVal ) );
}


/************************************************************************

 Function Name: GetNextByte

 Purpose: Returns the next byte from the file via the file buffer.

 Arguments: returnStatus_t  *pRetVal - the Get either passes or fails

 Returns: uint8_t - the Next Byte read from the file buffer
          via pRetVal, ePRG_MTR_FILE_FORMAT_ERROR or ePRG_MTR_INTERNAL_ERROR or
                       remains unchanged at eSUCCESS

 Side Effects: FileBufferIndex_, FileOffset_, RemainingFileBytesToRead_

 Re-entrant Code: No

 Notes: FileSize will be N bytes, starting at byte 1 and ending with byte N.
        The FileOffset_ will run from 0 to (N-1).

        If previous GetNextByte() failed (*pRetVal != eSUCCESS), then skip this
        GetNextByte(). This implementation removes the need to check for an
        error after each call to GetNextByte(); instead, we only need to check
        once following a series of calls to GetNextByte().

************************************************************************/
static uint8_t GetNextByte ( returnStatus_t  *pRetVal )
{
   uint8_t  NextByte;

    if ( eSUCCESS == *pRetVal )
    {
        NextByte = PseudoFile[IndexGet];
        IndexGet++;
    }
    else
    {
        NextByte = 0;
    }

    return NextByte;
}
#endif   // #else for #ifndef DEBUGGING_WRITE_TO_METER_FLASH

#endif // End  #if ( ( END_DEVICE_PROGRAMMING_CONFIG == 1 ) || ( END_DEVICE_PROGRAMMING_FLASH >  ED_PROG_FLASH_NOT_SUPPORTED ) )


/***********************************************************************************************************************
 *
 * Function Name: HMC_DFW_FW_Applet
 *
 * Purpose: Sends commends to the host meter to update the meter FW (patch).
 *
 * Arguments: uint8_t - Command_, void *pData
 *
 * Returns: uint8_t - HMC_APP_API_RPLY (see mtr_app.h)
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 * Note: pCurrentCmd_ is used also as a flag to indicate the applet is idle when NULL.
 *
 **********************************************************************************************************************/
//Read      MT0, offset=7, len=3                                                                  // Used to verify correct Meter FW Ver, Rev and Build are correct for this patch
//Read      ST3, offset=0, len=1                                                                  // Used to verify meter is in Metering Mode(bit0=1), and not test mode(bit1=0)
//Term                                                                                            // Terminate Session
//ID                                                                                              // Identify
//Logon                                                                                           // Logon
//Security  Master                                                                                // Enter Master Security session
//Procedure MP83, 1-byte parameter: 0x01                                                          // Used to enter DL Mode.  Must be in Master Security and must terminate session to activate.
//                                                                                                // NOTE: Meter seems to delay for about 3 seconds before responding to the terminate request
//ProcResp  Complete (0x00) or Accepted (0x01)                                                    // Verify Complete (0x00)
//Read      MT0, offset 0x27, len 1                                                               // Read UpdateEnabledFlag to UpdateEnabledFlag. Used if we just need to clear bit 0 (older meters)
//Procedure MP70, 6-byte parameters: 00=Start Programming, YYMMDDHHMM                             // Used to Start Programming
//ProcResp  Complete (0x00) or Accepted (0x01)                                                    // Verify Complete (0x00)
//Write     MT0, offset 0x27, len 1 Data=(UpdateEnabledFlag & 0xFE)                               // Clear UpdateEnabledFlag (NOTE: Older meters had more flags than bit 0, newer ones use field as flag)
//Procedure MP70, 6-byte parameters: 03=Stop Programming/No clear/No Logoff, YYMMDDHHMM           // Used to Stop Programming
//ProcResp  Complete (0x00) or Accepted (0x01)                                                    // Verify Complete (0x00)
//Read      MT0, Offset=0x45, len=4                                                               // Read PATCH_CODE_START_ADDRESS (little endian)
//                                                                                                // Calculate numBytes = 0x40000 - PATCH_CODE_START_ADDRESS (Start address (and therefore numBytes) must be modulo 2048(0x800))
//Procedure MP82, two 4-byte parameters: 4-bytes PATCH_CODE_START_ADDRESS, 4-bytes for numBytes   // Erase Sectors
//ProcResp  Complete (0x00) or Accepted (0x01)                                                    // Verify Erase complete (0x00) loop until done or time limit (3s?)
//Write     MT12, Offset=address, len=32, data=data from record                                   // Write patch record. NOTE no response until record write complete (~103ms)
//Read      MT12, Offset=address, len=32, data=data from record                                   // Read patch record. Used to verify write
//Read      MT0, offset 0x27, len 1                                                               // Read UpdateEnabledFlag to UpdateEnabledFlag. Used if we just need to clear bit 0 (older meters)
//Procedure MP70, 6-byte parameters: 00=Start Programming, YYMMDDHHMM                             // Used to Start Programming
//ProcResp  Complete (0x00) or Accepted (0x01)                                                    // Verify Complete (0x00)
//Write     MT0, offset 0x27, len 1 Data=(UpdateEnabledFlag | 0x01)                               // Set UpdateEnabledFlag (NOTE: Older meters had more flags than bit 0, newer ones use field as flag)
//Procedure MP70, 6-byte parameters: 03=Stop Programming/No clear/No Logoff, YYMMDDHHMM           // Used to Stop Programming
//ProcResp  Complete (0x00) or Accepted (0x01)                                                    // Verify Complete (0x00)
//Procedure MP83, 1-byte parameter: 0x00                                                          // Used to exit DL Mode.  Must be in Master Security and must terminate session to activate.
//                                                                                                // NOTE: May NOT get valid response to ST07 write since meter immediately begins action including an internal reboot
//ProcResp  Complete (0x00) or Accepted (0x01)                                                    // Verify Complete (0x00)
//Term                                                                                            // Terminate Session - Meter will take a few seconds to re-initialize and validate the downloaded code
//Term                                                                                            // Terminate Session
//ID                                                                                              // Identify
//Logon                                                                                           // Logon
//Read      MT0, offset 0x27, len 2                                                               // Used to verify UPDATE_ENABLED_FLAGS and UPDATE_ACTIVE_FLAGS are set (data[0](UPDATE_ENABLED_FLAGS) !=0; data[1](UPDATE_ACTIVE_FLAGS) !=0 (this is the preferred method)
//Read      ST3, offset 0x01, len 1                                                               // Used to verify ROM_FAILURE_FLAG is cleared (data[0]ED_STD_STATUS1_BFLD) bit 4 (ROM_FAILURE_FLAG) ==0) (Note: This step is optional)


#if 0 //WIP
uint8_t HMC_DFW_MTR_FW_Applet(uint8_t cmd, void far *pData)
{
   uint8_t retVal = (uint8_t) HMC_APP_API_RPLY_IDLE; /* Assume the module is Idle */

   /* Can include anything needed to do each time this is called like save state/status etc */

   switch (cmd) /* Decode the command sent to this applet */
   {
         case ( uint8_t )HMC_APP_API_CMD_DISABLE_MODULE: /* Command to disable this applet? */
         {
            bFeatureEnabled_ = false;                 /* Disable the applet */
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_ENABLE_MODULE: /* Command to enable this applet? */
         {
            bFeatureEnabled_ = true;                  /* Enable the applet */
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_INIT:                /* Initialize the applet? */
         {
            retries_ = HMC_TIME_RETRIES;                    /* Set application level retry count */
            eState_  = eCLOCK_EXIST;                         /* Kick off process of discovering existence of clock */
            retVal   = ( uint8_t )HMC_APP_API_RPLY_READY_COM;
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_ACTIVATE:    /* Kick off process of reading/setting meter's date/time */
         {  /* Activated by user (debug port) or demand reset function  */
            retries_ = HMC_TIME_RETRIES;              /* Set application level retry count */
            eState_  = eUPDATE_TIME;                  /* Read/update meter's time   */
            break;
         }
         case ( uint8_t )HMC_APP_API_CMD_STATUS:        /* Command to check to see if the applet needs communication */
         {
























      case (uint8_t) HMC_APP_API_CMD_ACTIVATE: /* Prepare to update all registers IF this module is NOT active */
      {
         /* Is this module enabled and is the applet not active? */
         if (bFeatureEn_ && (NULL == pCurrentCmd_))
         {
            pCurrentCmd_        = pRegUpdateTables_[eMtrType_]; /* List of Table "sets" to use */
            dfwRetries_         = DFW_FW_APP_RETRIES;           /* Setup the number of retries in case of an error. */
            meterStatusRetries_ = DFW_FW_METER_STATUS_RETRIES;  /* Retry counter for the reading of MT115 - status  */
         }
         break;
      }
      case (uint8_t) HMC_APP_API_CMD_INIT: /* Initialize the Applet */
      {
         /* Create the timers used by this applet  */
         timer_t   timerCfg;        /* Timer configuration */

         (void)HMC_DS_init();             /* Open/read the switch metadata */
         (void)memset(&timerCfg, 0, sizeof(timerCfg));  /* Clear the timer values */
         timerCfg.bOneShot       = true;
         timerCfg.bExpired       = true;
         timerCfg.bStopped       = true;
         timerCfg.ulDuration_mS  = STATUS_RETRY_PROC_mS;
         timerCfg.pFunctCallBack = (vFPtr_u8_pv)statusReadRetryCallBack_ISR;

         (void)TMR_AddTimer(&timerCfg);               /* Create the status update timer   */
         rcdTimer_id = timerCfg.usiTimerId;

         meterWaitStatus_        = eTableRetryNotWaiting;   /* The procedure status can be cleared at power up. */
         meterStatusRetries_     = DS_METER_STATUS_RETRIES; /* Retry counter for the reading of MT115 - status  */
         lastOperationAttempted_ = ePowerUpState;           /* The Last operation attempted has not been set */
         pCurrentCmd_            = NULL;                    /* Initialize, No commands are being performed. */

         break;
      }
      case (uint8_t) HMC_APP_API_CMD_STATUS: /* Check if meter communication is required */
      {
         if (bFeatureEn_) /* Only check on status if the feature is enabled. */
         {
            /* Is the pointer valid (pointing to a table)? and not waiting on a retry  */
            if ((NULL != pCurrentCmd_) && (eTableRetryNotWaiting == meterWaitStatus_))
            {
               retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
            else if (eTableRetryTmrExpired == meterWaitStatus_) /* Are we ready for a re-read of MT115? */
            {
               /* Re-read the MT115 status table. */
               meterWaitStatus_ = eTableRetryNotWaiting;
               pCurrentCmd_     = &regUpdateTable_[0];   /*Set the function pointer to the status table */

               retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
         }
         break;
      }
      case (uint8_t) HMC_APP_API_CMD_PROCESS: /* Status must have been ready for comm, now load the pointer. */
      {
         if (bFeatureEn_ && (NULL != pCurrentCmd_)) /* Verify, is applet enabled and pointing to a table? */
         {
            retVal = (uint8_t)HMC_APP_API_RPLY_PAUSE; /* assume applet is in a wait state - set to pause meter app. */

            if (eTableRetryNotWaiting == meterWaitStatus_) /* waiting? to re read MT115 */
            {
               retVal                                  = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST; /* return ready! */
               //lint -e{413} pData will not be NULL here
               ((HMC_COM_INFO *) pData)->pCommandTable = (uint8_t far *)pCurrentCmd_->pCmd;         /* Set up the pointer. */
            }
         }
         break;
      }
      case (uint8_t) HMC_APP_API_CMD_TBL_ERROR: /* The meter reported back a table error, set a diags flag */
      case (uint8_t) HMC_APP_API_CMD_ABORT: /* Comm Error, the session was aborted.  */
      case (uint8_t) HMC_APP_API_CMD_ERROR: /* Comm Error, the session was aborted.  */
      {
         if (0 == (--dsRetries_) )  //Retry if HMC_APP_API_CMD_TBL_ERROR, HMC_APP_API_CMD_ABORT or HMC_APP_API_CMD_ERROR
         {
            pCurrentCmd_++;      /* Skip to next command in sequence.   *//*lint !e613 NULL ptr OK */
            /* Is the next item in the table NULL?  If so, that is all, we're all done.  */
            /* Handler may have aborted, so check pCurrentCmd_ first!   */
            if ( ( NULL != pCurrentCmd_ ) && ( NULL == pCurrentCmd_->pCmd) )
            {
               pCurrentCmd_     = NULL;
               meterWaitStatus_ = eTableRetryNotWaiting; /* Set indicator flag to not waiting. */
            }
         }
         break;
      }
      case (uint8_t) HMC_APP_API_CMD_MSG_COMPLETE: /* Communication complete, no errors and data has been saved! */
      {
         if (NULL != pCurrentCmd_)  /* Do not do anything if we get here with a NULL in pCurrentCmd_ */
         {
            if (NULL != pCurrentCmd_->fpFunct)  /* Is there a handler for this?  */
            {
               /* Call the local handler and check the results */
               if (DS_RES_NEXT_TABLE == pCurrentCmd_->fpFunct(cmd, (sMtrRxPacket *)pData ))
               {
                  pCurrentCmd_++;
               }
            }
            else
            {
               /* No handler just bump to next command. */
               pCurrentCmd_++;
            }
            /* Is the next item in the table NULL?  If so, that is all, we're all done.  */
            /* Handler may have aborted, so check pCurrentCmd_ first!   */
            if ( ( NULL != pCurrentCmd_ ) && ( NULL == pCurrentCmd_->pCmd) )
            {
               pCurrentCmd_     = NULL;
               meterWaitStatus_ = eTableRetryNotWaiting; /* Set indicator flag to not waiting. */
            }
            else
            {
               retVal = (uint8_t)HMC_APP_API_RPLY_RDY_COM_PERSIST;
            }
          }
         break;
      }
      case (uint8_t) HMC_APP_API_CMD_ENABLE_MODULE: /* Command to enable this applet */
      {  /* Switch state/status needs to be updated every time a meter id(reg 2243)read shows that a meter with a switch
            has been detected */
         dsMetaData hwInfo;
         bFeatureEn_ = (bool)TRUE;  /* Enable the applet */
         /* Record the fact that switch h/w exists   */
         (void)FIO_fread(&mtrDsFileHndl_, (uint8_t *)&hwInfo, (uint16_t)offsetof(mtrDsFileData_t, dsInfo), (lCnt)sizeof(hwInfo));
         hwInfo.hwExists       = 1;
         mtrDsFileData_.dsInfo = hwInfo;
         (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
         pCurrentCmd_        = pRegUpdateTables_[eMtrType_]; /* Set tables needed to update switch state/status. No need to
                                                                wait for INIT state*/
         dsRetries_          = DS_APP_RETRIES; /* Setup the number of table read retries in case of an error. */
         meterStatusRetries_ = DS_METER_STATUS_RETRIES;/* Retry counter for the reading of MT115 - status  */
         break;
      }
      case (uint8_t) HMC_APP_API_CMD_DISABLE_MODULE: /* Comnmand to disable this applet? */
      {
         dsMetaData hwInfo;
         bFeatureEn_ = (bool)FALSE; /* Disable the applet */

         /* Record the fact that switch h/w doesn't exist   */
         (void)FIO_fread(&mtrDsFileHndl_, (uint8_t *)&hwInfo,
                           (uint16_t)offsetof(mtrDsFileData_t, dsInfo), (lCnt)sizeof(hwInfo));
         hwInfo.hwExists       = 0;
         mtrDsFileData_.dsInfo = hwInfo;
         (void) FIO_fwrite(&mtrDsFileHndl_, 0, (uint8_t *)&mtrDsFileData_, (lCnt)sizeof (mtrDsFileData_));
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
#endif   //WIP