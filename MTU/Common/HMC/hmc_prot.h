/******************************************************************************

   Filename: hmc_prot.h

   Contents: #defs, typedefs, and function prototypes for Meter Protocol Layer

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ******************************************************************************

   $Log$ kad Created 072007

 *****************************************************************************/

#ifndef hmc_prot_H
#define hmc_prot_H

/* INCLUDE FILES */

#include "psem.h"
#include "hmc_app.h"
#include "file_io.h"

#ifdef HMC_PROTO_GLOBAL
#define GLOBAL
#else
#define GLOBAL extern
#endif

/* CONSTANT DEFINITIONS */

typedef union
{
   struct
   {
      unsigned bInitialized   :  1; /* Protocol Layer initialized */
      unsigned bBusy          :  1; /* Protocol Layer is busy */
      unsigned bRestart       :  1; /* Application layer needs to restart communication session */
      unsigned bMsgComplete   :  1; /* Message Complete, no errors on the packet level */
      unsigned bTblError      :  1; /* Message received, but the table read has an error */
      unsigned bInvRegister   :  1; /* Invalid Register ID */
      unsigned bComFailure    :  1; /* Communication Failure */
      unsigned bGenError      :  1; /* Hardware failure on the transponder, I2C, EEPROM, EMTR, etc... */
      unsigned bCPUTime       :  1; /* Significant CPU Time taken (data may have been written to EEPROM) */
      unsigned bInvalidCmd    :  1; /* Invalid command sent to protocol layer, for debugging purposes */
   } Bits;
   uint16_t uiStatus;
} HMC_PROTO_STATUS;  /* Protocol Status Structure */

/* Define Commands that can be sent to the Protocol Layer */
enum
{
   HMC_PROTO_CMD_INIT = 0, /* Initialize the Protocol Layer */
   HMC_PROTO_CMD_NEW_SESSION, /* Set variables for a new session */
   HMC_PROTO_CMD_SET_ID,   /* Set the PSEM ID Byte */
   HMC_PROTO_CMD_NEW_CMD,  /* Start a new command table process */
   HMC_PROTO_CMD_PROCESS,  /* Normal process command called every time through the main loop */
   HMC_PROTO_CMD_READ_COM_PARAM,
   HMC_PROTO_CMD_WRITE_COM_PARAM,
   HMC_PROTO_CMD_SET_DEFAULT_COM,
   HMC_PROTO_CMD_READ_COMPORT,
   HMC_PROTO_CMD_WRITE_COMPORT,
};

/* When reading/writing memory, use these to define the memory type */
/* Bits 0-2 are memory type */
#define HMC_PROTO_MEM_ENDIAN  ((uint8_t)8)  /* Switch Endian, 0b00001000 */
#define HMC_PROTO_MEM_WRITE   ((uint8_t)16) /* Write data to Memory defined in the enum HMC_PROTO_MEM_TYPE */
#define HMC_PROTO_MEM_ZF      ((uint8_t)32) /* Zero Fill Data on Table Error only (not on packet error), 0b00100000 */

/* The lower 3 bits are used to define the Memory Type */
enum  HMC_PROTO_MEM_TYPE
{
   HMC_PROTO_MEM_NULL = 0,                /* NULL, terminator */

   /* Command Locations */
   HMC_PROTO_MEM_REG,                     /* Register Operation */
   HMC_PROTO_MEM_RAM,                     /* RAM Operation */
// HMC_PROTO_MEM_NV,                      /* NV Operation */
   HMC_PROTO_MEM_FILE,                    /* File Operation */
   HMC_PROTO_MEM_PGM,                     /* Program Memory Operation - BE CAREFUL! */
   HMC_PROTO_MEM_FILL,                    /* Fill will xx value */
};

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

PACK_BEGIN
typedef struct PACK_MID
{
   FileHandle_t *pFileHandle;             /* File handle */
   uint16_t       offset;                 /* Offset into the file */
} HMC_PROTO_file_t;                       /* Method to handle read/write to a file. */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   enum HMC_PROTO_MEM_TYPE eMemType;      /* Memory Type to Read/Write to/from */
   uint8_t ucLen;                         /* Number of bytes to read/write */
   uint8_t far *pPtr;                     /* Pointer to the data to read/write to */
} HMC_PROTO_TableCmd;                     /* Format of the table command */
PACK_END

PACK_BEGIN
typedef struct PACK_MID
{
   const HMC_PROTO_TableCmd far *pData;   /* Pointer to command structure to be sent to meter   */
} HMC_PROTO_Table;                        /* Typedef of the protocol table                      */
PACK_END

/* ***************************************************************************************************************** */
/* Full Rx/Tx Structure with command table pointer */

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */

GLOBAL uint16_t HMC_PROTO_Protocol( uint8_t, HMC_COM_INFO * );

#undef GLOBAL
#endif
