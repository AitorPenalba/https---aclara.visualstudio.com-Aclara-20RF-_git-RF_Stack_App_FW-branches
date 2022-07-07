/***********************************************************************************************************************
 *
 * Filename: dfw_xzmini_interface.h
 *
 * Contents: Contains prototypes and definitions for interface between DFW and XZMINI.
 *
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

#ifndef _DFW_XZMINI_INTERFACE_H_
#define _DFW_XZMINI_INTERFACE_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "version.h"
#include "DBG_SerialDebug.h"
#include "partitions.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#if ENABLE_PRNT_DFW_INFO
#define DFW_XZMINI_PRNT_INFO(x)         INFO_printf(x)
#define DFW_XZMINI_PRNT_INFO_PARMS      INFO_printf
#else
#define DFW_XZMINI_PRNT_INFO(x)
#define DFW_XZMINI_PRNT_INFO_PARMS(x,...)
#endif

#if ENABLE_PRNT_DFW_ERROR
#define DFW_XZMINI_PRNT_ERROR(x)        ERR_printf(x)
#define DFW_XZMINI_PRNT_ERROR_PARMS     ERR_printf
#else
#define DFW_XZMINI_PRNT_ERROR(x)
#define DFW_XZMINI_PRNT_ERROR_PARMS(x,...)
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef uint8_t u_char;
typedef long    ssize_t;
typedef long    off_t;

typedef struct {
   const PartitionData_t  *pPatchPartition; // Patch partition address
   dSize                  patchSrcOffset;   // current position of data in partition table
   dSize                  patchEndOffset;   // end position of data in partition table
} compressedData_position_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

returnStatus_t            DFW_XZMINI_uncompressPatch_init( const PartitionData_t *pPatchPartition, dSize patchSrcOffset, dSize patchEndOffset );
returnStatus_t            DFW_XZMINI_bspatch_valid_header( int32_t* newsize, int32_t* ctrllen, int32_t* datalen );
int32_t                   DFW_XZMINI_uncompressedData_read_ssize( compressedData_position_t *patchOffset );
u_char                    DFW_XZMINI_uncompressedData_getchar( compressedData_position_t *patchOffset );
void                      DFW_XZMINI_bspatch_close( void );
returnStatus_t            DFW_XZMINI_bspatch( PartitionData_t const *pOldImagePTbl, eFwTarget_t patchTarget, PartitionData_t const *pNewImagePTbl, off_t newImageSize );
#endif
