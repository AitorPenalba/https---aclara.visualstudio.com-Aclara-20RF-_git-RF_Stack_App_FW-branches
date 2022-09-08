/***********************************************************************************************************************
 *
 * Filename: BUF_MemoryBuffers.c
 *
 * Global Designator: BUF_
 *
 * Contents:  This file contains the functions for managing memory buffers.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 - 2022 Aclara.  All Rights Reserved.
 ***********************************************************************************************************************
 *
 * @author Benjamin Hammond
 *
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h> /* Necessary for Interrupt disable/enable calls */
#endif
#include "OS_aclara.h"
#include "BUF_MemoryBuffers.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: BUF_Create

  Purpose: Creates a fixed-sized memory buffers partition

  Arguments: BufferHandle - pointer to the Buffer Object that will be created
             pAddr - pointer to the starting address of the buffer in memory (allocated by calling function)
             numBlks - number of blocks to create from the buffer memory
             blkSize - size (in bytes) of each block

  Returns: FuncStatus - true if Buffer created successfully, false if error

  Notes: The memory blocks that are free contain a pointer to the other free
         memory blocks (a linked list).  This is acceptable because the memory
         block is not being used when it is free, so saving a pointer there is ok

         Example of how to interact with this BUF module (application code needs the following):
         #define XXX_NUM_BLOCKS 3
         #define XXX_BLOCK_SIZE 10
         static BUF_Obj XXX_Buf;
         static uint8_t XXX_MessageBuffer[XXX_NUM_BLOCKS][XXX_BLOCK_SIZE];
         BUF_Create ( &XXX_Buf, &XXX_MessageBuffer[0][0], XXX_NUM_BLOCKS, XXX_BLOCK_SIZE )

         then to use:
         uint8_t *XXX_MsgPtr;
         BUF_Get ( &XXX_Buf, (void **)&XXX_MsgPtr );
         ...
         BUF_Put ( &XXX_Buf, XXX_MsgPtr );

*******************************************************************************/
bool BUF_Create ( BUF_Handle BufferHandle, void *pAddr, uint32_t numBlks, uint32_t blkSize )
{
   bool FuncStatus = true;
   void **plink;
   uint32_t i;
   uint8_t *pblk;

   if ( (BufferHandle != (void *)0) && (pAddr != (void *)0) && (numBlks > 0) && (blkSize > 0) )
   {
      /* Setup a pointer to the beginning of the memory buffer */
      plink = (void **)pAddr;
      /* Pointer to the next block in the memory buffer */
      pblk = (uint8_t *)pAddr + blkSize;

      /* Create a linkded list of all the free memory blocks in the buffer
         This code is writing to the memory blocks (saving pointer to the next free
         block) and this is ok, because the blocks are free (not used) */
      for ( i=0; i<(numBlks - 1); i++ )
      {
         *plink = (void *)pblk; //Save pointer to the next free block
         plink = (void **)pblk; /*lint !e826 */ //move plink to the next block
         pblk = (uint8_t *)pblk + blkSize; //move pblk to the next block
      } /* end for() */

      /* Set the last free block in the linked list to 0 (termination of list) */
      *plink = (void *)0;

      /* Store the necessary values in the Buffer Object */
      BufferHandle->addr = pAddr;
      BufferHandle->freeList = pAddr;
      BufferHandle->numFree = numBlks;
      BufferHandle->numBlks = numBlks;
      BufferHandle->blkSize = blkSize;
   } /* end if() */
   else
   {
      FuncStatus = false;
   } /* end else() */

   return ( FuncStatus );
} /* end BUF_Create () */

/*******************************************************************************

  Function name: BUF_Get

  Purpose: This function will get a free memory block from the memory buffer

  Arguments: BufferHandle - pointer to the Buffer Object
             pBlk - pointer to the location of the memory block that can be used (populated by this function)

  Returns: ErrorStat - 0=No Error, Non-Zero indicates an error occurred

  Notes: The memory blocks that are free contain a pointer to the other free
         memory blocks (a linked list).  This is acceptable because the memory
         block is not being used when it is free, so saving a pointer there is ok

*******************************************************************************/
BUF_ErrorStat_e BUF_Get ( BUF_Handle BufferHandle, void **pBlk )
{
   BUF_ErrorStat_e ErrorStat = BUF_ERR_NONE;

   if ( (BufferHandle != (void *)0) && (pBlk != (void **)0) )
   {
      /* Protect against race conditions corrupting BUF_Obj data */
      OS_INT_disable( );
      if ( BufferHandle->numFree > 0 )
      {
         /* Get the next memory buffer from the free list of buffers */
         *pBlk = BufferHandle->freeList;
         /* Update the freeList with the pointer (next free buffer) that was stored
            in this memory buffer location */
         BufferHandle->freeList = *(void **)*pBlk;
         BufferHandle->numFree--;
      } /* end if() */
      else
      {
         ErrorStat = BUF_ERR_NO_FREE_BLKS;
      } /* end else() */
      OS_INT_enable( );
   } /* end if() */
   else
   {
      ErrorStat = BUF_ERR_INVALID_PARAM;
   } /* end else() */

   return ( ErrorStat );
} /* end BUF_Get () */

/*******************************************************************************

  Function name: BUF_Put

  Purpose: This function will return a memory block in the memory buffer to the
           free list of buffers

  Arguments: BufferHandle - pointer to the Buffer Object
             pBlk - pointer to the memory block to free

  Returns: ErrorStat - 0=No Error, Non-Zero indicates an error occurred

  Notes: The memory blocks that are free contain a pointer to the other free
         memory blocks (a linked list).  This is acceptable because the memory
         block is not being used when it is free, so saving a pointer there is ok

*******************************************************************************/
BUF_ErrorStat_e BUF_Put ( BUF_Handle BufferHandle, void *pBlk )
{
   BUF_ErrorStat_e ErrorStat = BUF_ERR_NONE;

   if ( (BufferHandle != (void *)0) && (pBlk != (void *)0) )
   {
      /* Protect against race conditions corrupting BUF_Obj data */
      OS_INT_disable( );
      if ( BufferHandle->numFree < BufferHandle->numBlks )
      {
         /* Store the freeList pointer in this memory buffer that is now free
            this is the next free buffer pointer */
         *(void **)pBlk = BufferHandle->freeList;
         /* Update the free list to point to this memory buffer */
         BufferHandle->freeList = pBlk;
         BufferHandle->numFree++;
      } /* end if() */
      else
      {
         ErrorStat = BUF_ERR_BUFFER_FULL;
      } /* end else() */
      OS_INT_enable( );
   } /* end if() */
   else
   {
      ErrorStat = BUF_ERR_INVALID_PARAM;
   } /* end else() */

   return ( ErrorStat );
} /* end BUF_Put () */





