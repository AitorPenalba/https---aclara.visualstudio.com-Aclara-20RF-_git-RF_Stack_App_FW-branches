/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: IF_intFlash.h
 *
 * Contents: Internal Flash Driver - Writes to the internal flash memory for the K60
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ KAD Created November 14, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 11/14/2013 - Initial Release
 *
 * @version    0.1
 * #since      2013-11-13
 **********************************************************************************************************************/
#ifndef IF_intFlash_H_
#define IF_intFlash_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "partitions.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */
#define ERASE_SECTOR_SIZE_BYTES     ((uint16_t)4096)                 /* Sector Size in Bytes */
#define ERASE_BLOCK_SIZE_BYTES      ((uint32_t)0x80000)              /* Block Size in Bytes */

#if ( MCU_SELECTED == RA6E1 )
#define FLASH_HP_DATAFLASH_START_ADDRESS               0x08000000U
#define FLASH_HP_DATAFLASH_END_ADDRESS                 0x08001FFFU

/* Considering the code flash configured in linear mode */
#define FLASH_HP_CODEFLASH_START_ADDRESS               0x00000000U
#define FLASH_HP_CODEFLASH_END_ADDRESS                 0x000FFFFFU

#define FLASH_HP_CODEFLASH_BLOCK8_START_ADDRESS        0x00010000U

/* Code and Data flash minimal write size bytes */
#define FLASH_HP_CODEFLASH_MINIMAL_WRITE_SIZE          128
#define FLASH_HP_DATAFLASH_MINIMAL_WRITE_SIZE          4

/* Code and Data flash minimal erase size bytes */
#define FLASH_HP_CODEFLASH_MINIMAL_ERASE_SIZE          8192
#define FLASH_HP_DATAFLASH_MINIMAL_ERASE_SIZE          64

/* Code flash block size */
#define FLASH_HP_CODEFLASH_SIZE_BELOW_BLOCK8           8192
#define FLASH_HP_CODEFLASH_SIZE_FROM_BLOCK8            32768
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="eIF_ioctlCmds_t - Command list for IOCTL">
typedef enum
{
   eIF_IOCTL_CMD_FLASH_SWAP = ( (uint8_t) 0 ) /* IOCTL command for performing a flash swap or reading the status. */
} eIF_ioctlCmds_t; /* Command list for IOCTL */// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="eFS_command_t - Commands for the flash swap functionality (Read Notes!)">
/* Note: Each enumeration value is defined by "K60 Sub-Family Reference Manual" - 30.4.12.14 Swap Control command.
 *       Since each command corresponds to the data sheet, each value is specifically defined to a value. */
/* Note:  It is recommended that the user execute the Swap Control command to report swap status (code 0x08) after any
 *    reset to determine if issues with the swap system were detected during the swap state determination procedure. */
/* Note: It is recommended that the user write 0xFF to FCCOB5, FCCOB6, and FCCOB7 since the Swap Control command will
 *    not always return the swap state and status fields when an ACCERR is detected. */
typedef enum
{
   eFS_INIT = ((uint8_t)1),      /* 0x01 (Initialize Swap System to UPDATE-ERASED State) - After verifying that the
                                  * current swap state is UNINITIALIZED and that the flash address provided is in
                                  * Program flash block 0 but not in the Flash Configuration Field, the flash address
                                  * (shifted with bit 0 removed) will be programmed into the IFR Swap Field found in a
                                  * program flash IFR. After the swap indicator address has been programmed into the
                                  * IFR Swap Field, the swap enable word will be programmed to 0x0000. After the swap
                                  * enable word has been programmed, the swap indicator, located within the Program
                                  * flash block 0 address provided, will be programmed to 0xFF00. */
   eFS_UPDATE  = ((uint8_t)2),   /* 0x02 (Progress Swap to UPDATE State) - After verifying that the current swap state
                                  * is READY and that the aligned flash address provided matches the one stored in the
                                  * IFR Swap Field, the swap indicator located within bits [15:0] of the flash address
                                  * in the currently active program flash block will be programmed to 0xFF00. */
   eFS_COMPLETE = ((uint8_t)4),  /* 0x04 (Progress Swap to COMPLETE State) - After verifying that the current swap
                                  * state is UPDATE-ERASED and that the aligned flash address provided matches the one
                                  * stored in the IFR Swap Field, the swap indicator located within bits [15:0] of the
                                  * flash address in the currently active program flash block will be programmed to
                                  * 0x0000.  Before executing with this Swap Control code, the user must erase the
                                  * nonactive swap indicator using the Erase Flash Block or Erase Flash Sector commands
                                  * and update the application code or data as needed. The non-active swap indicator
                                  * will be checked at the erase verify level and if the check fails, the current swap
                                  * state will be changed to UPDATE with ACCERR set. */
   eFS_GET_STATUS = ((uint8_t)8) /* 0x08 (Report Swap Status) - After verifying that the aligned flash address provided
                                  * is in program flash block 0 but not in the Flash Configuration Field, the status of
                                  * the swap system will be reported as follows:
                                  * FCCOB5 (Current Swap State) - indicates the current swap state based on the status
                                  *   of the swap enable phrase and the swap indicators. If the MGSTAT0 flag is set
                                  *   after command completion, the swap state returned was not successfully
                                  *   transitioned from and the appropriate swap command code must be attempted again.
                                  *   If the current swap state is UPDATE and the non-active swap indicator is 0xFFFF,
                                  *   the current swap state is changed to UPDATE-ERASED.
                                  * FCCOB6 (Current Swap Block Status) - indicates which program flash block is
                                  *   currently located at relative flash address 0x0_0000.
                                  * FCCOB7 (Next Swap Block Status) - indicates which program flash block will be
                                  *   located at relative flash address 0x0_0000 after the next reset of the FTFE
                                  *   module. */
} eFS_command_t; /* Commands for the flash swap functionality */// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="IF_ioctlCmdFmt_t - Command sent to the IOCTL interface for Flash Swap">
typedef struct
{
   eIF_ioctlCmds_t ioctlCmd; /* IOCTL command */
   eFS_command_t fsCmd; /* Flash Swap Command - This is sub-command of IOCTL commands for flash swap. */
} IF_ioctlCmdFmt_t; /* Command sent to the IOCTL interface for Flash Swap */// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="eFS_CurrentSwapMode_t - Values that are returned from the Swap Control Module in the micro.">
/*lint -esym(749,eFS_MODE_UPDATE,eFS_MODE_COMPLETE) */
/* Returned Values, Current Swap Mode - See definition in K60 Sub-Family Ref Manual, Rev. 2, sect 30.4.12.14 */
typedef enum
{
   eFS_MODE_UNINITIALIZED = ((uint8_t)0),
   eFS_MODE_READY,
   eFS_MODE_UPDATE,
   eFS_MODE_UPDATE_ERASED,
   eFS_MODE_COMPLETE
} eFS_CurrentSwapMode_t; /* Values that are returned from the Swap Control Module in the micro. */// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="eFS_CurrentSwapBlockStatus_t - Returned Values, Current Swap Block Status">
/*lint -esym(749,eFS_C_PGM_FLASH_BLOCK_0_1,eFS_C_PGM_FLASH_BLOCK_2_3) */
/* Returned Values, Current Swap Block Status - See definition in K60 Sub-Family Ref Manual, Rev. 2, sect 30.4.12.14 */
typedef enum
{
   eFS_C_PGM_FLASH_BLOCK_0_1 = ((uint8_t)0),
   eFS_C_PGM_FLASH_BLOCK_2_3
} eFS_CurrentSwapBlockStatus_t; // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="eFS_NextSwapBlockStatus_t - Returned Values, Next Swap Block Status (after any rst)">
/*lint -esym(749,eFS_N_PGM_FLASH_BLOCK_0_1,eFS_N_PGM_FLASH_BLOCK_2_3) */
/* Returned Values, Next Swap Block Status (after any rst): - See definition in K60 Sub-Family Ref Manual, Rev. 2,
 * sect 30.4.12.14 */
typedef enum
{
   eFS_N_PGM_FLASH_BLOCK_0_1 = ((uint8_t)0),
   eFS_N_PGM_FLASH_BLOCK_2_3
} eFS_NextSwapBlockStatus_t; // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="flashSwapStatus_t - Returned Values, Current Swap Block Status">
/* Returned Values, Current Swap Block Status: - See def. in K60 Sub-Family Ref Manual, Rev. 2, sect 30.4.12.14 */
typedef struct
{
   eFS_CurrentSwapMode_t         currentMode;    /* Current Swap Mode */
   eFS_CurrentSwapBlockStatus_t  currentBlock;   /* Current Swap Block Status */
   eFS_NextSwapBlockStatus_t     nextBlock;      /* Next Swap Block Status (after any reset) */
} flashSwapStatus_t;  /* Used to return the flash swap status, These items are returned via FCCOBx */// </editor-fold>

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

extern const DeviceDriverMem_t IF_deviceDriver; /* Access Table for this driver. */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#endif
