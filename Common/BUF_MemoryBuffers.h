/***********************************************************************************************************************
 *
 * Filename: BUF_MemoryBuffers.h
 *
 * Global Designator: BUF_
 *
 * Contents:  This file contains the functions for managing memory buffers.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 ***********************************************************************************************************************
 *
 * @author Benjamin Hammond
 *
 **********************************************************************************************************************/

#ifndef BUF_MemoryBuffers_H
#define BUF_MemoryBuffers_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef enum
{
   BUF_ERR_NONE,
   BUF_ERR_INVALID_PARAM,
   BUF_ERR_BUFFER_FULL,
   BUF_ERR_NO_FREE_BLKS
} BUF_ErrorStat_e;

typedef struct
{
   void *addr;       /* Pointer to beginning of memory partition            */
   void *freeList;   /* Pointer to list of free memory blocks               */
   uint32_t blkSize; /* Size (in bytes) of each block of memory             */
   uint32_t numBlks; /* Total number of blocks in this partition            */
   uint32_t numFree; /* Number of memory blocks remaining in this partition */
} BUF_Obj, *BUF_Handle;

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
extern bool BUF_Create ( BUF_Handle BufferHandle, void *pAddr, uint32_t numBlks, uint32_t blkSize );
extern BUF_ErrorStat_e BUF_Get ( BUF_Handle BufferHandle, void **pBlk );
extern BUF_ErrorStat_e BUF_Put ( BUF_Handle BufferHandle, void *pBlk );

/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */
