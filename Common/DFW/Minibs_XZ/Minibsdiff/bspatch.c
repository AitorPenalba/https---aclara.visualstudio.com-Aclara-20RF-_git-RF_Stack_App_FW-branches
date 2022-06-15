/*-
 * Copyright 2012-2013 Austin Seipp
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/***********************************************************************************************************************
 *
 * Filename: bspatch.c
 *
 * Global Designator: MINIBS_
 *
 * Contents: This module does the patching mechanism.
 *
 ***********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "partition_cfg.h"
#include "bspatch.h"
#include "dfw_xzmini_interface.h"

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define LOCAL_READ_WRITE_BUFFER_SIZE    512     // Size used for reading and writing in the partition

#define RESERVED_SECTION_1_START_BYTES  ( off_t )( PART_APP_CODE_OFFSET + PART_APP_CODE_SIZE )
#define RESERVED_SECTION_1_STOP_BYTES   ( off_t )( PART_APP_UPPER_CODE_OFFSET - 1 )

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
typedef enum
{
   eBSPATCH_READ_FROM_MEMORY = ((uint8_t)0),
   eBSPATCH_WRITE_INTO_PARTITION
} eReadWriteType_t;

#if ( ( DCU == 1 ) || ( MCU_SELECTED == RA6E1 ) )
#else
static bool seekFlagEnabled = false;   // Flag to determine whether the seek mechanism between the reserved section started
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: readAndWriteBuffer

   Purpose: This can read from the internal memory or write into the external flash partition
            in the desired location with a particular size. The read/write can be differentiated by
            using enum type of eReadWriteType_t

   Arguments: off_t *position, off_t incrementCounter, off_t bufSize, const off_t reservedSectionSize,
              uint8_t *imageBuffer, eReadWriteType_t readWriteType, PartitionData_t const *pImagePTbl

   Returns: void

   Side Effects: Nothing

   Reentrant Code: Yes

   Notes:  Handled both write and read condition in the same function.
           Instead of using partition accessing in the read, using direct memory access for reading the internal flash memory.
           As the patch contains only the app sections (does not contain pre-filled reserved sections), handled
           in a way to remove the usage of the reserved sections in reading and writing into the flash.
           memcpy is used here to store the certain size of local buffer to create the new image.

**********************************************************************************************************************/
static void readAndWriteBuffer( off_t *position, off_t incrementCounter, off_t bufSize, const off_t reservedSectionSize,
                                uint8_t *imageBuffer, eReadWriteType_t readWriteType, PartitionData_t const *pImagePTbl )
{
   off_t dest = *position + incrementCounter;
#if ( ( DCU == 1 ) || ( MCU_SELECTED == RA6E1 ) )
#else
   if ( ( ( dest <= RESERVED_SECTION_1_STOP_BYTES ) && ( dest >= RESERVED_SECTION_1_START_BYTES ) ) ||
        ( ( ( dest + bufSize ) <= RESERVED_SECTION_1_STOP_BYTES ) && ( ( dest + bufSize ) > RESERVED_SECTION_1_START_BYTES ) ) )
   {
      if ( dest < RESERVED_SECTION_1_START_BYTES )
      {
         off_t localBufIndex      = 0;
         off_t localLowerAppSize  = RESERVED_SECTION_1_START_BYTES - dest;
         /* Write/read the section before the reserved section */
         if( eBSPATCH_WRITE_INTO_PARTITION == readWriteType )
         {
            ( void ) PAR_partitionFptr.parWrite( ( dSize )dest, &imageBuffer[localBufIndex],
                                                 ( uint32_t ) localLowerAppSize, pImagePTbl );
         }
         else
         {
            memcpy( &imageBuffer[localBufIndex], ( uint8_t * )dest, ( size_t ) localLowerAppSize );
         }

         localBufIndex += localLowerAppSize;
         /* Increment the position to move across the reserved section */
         *position += reservedSectionSize;
         dest += reservedSectionSize;
         /* Write/read the remaining bytes of the section after the reserved section */
         if( eBSPATCH_WRITE_INTO_PARTITION == readWriteType )
         {
            ( void ) PAR_partitionFptr.parWrite( ( dSize )( dest + localLowerAppSize ), &imageBuffer[localBufIndex],
                                                 ( uint32_t )( bufSize - localLowerAppSize ), pImagePTbl );
         }
         else
         {
            memcpy( &imageBuffer[localBufIndex], ( uint8_t * )( dest + localLowerAppSize ), ( size_t ) ( bufSize - localLowerAppSize ) );
            seekFlagEnabled = true;
         }
      }
      else
      {
         /* Increment the position to move across the reserved section */
         *position += reservedSectionSize;
         dest += reservedSectionSize;
         if( eBSPATCH_WRITE_INTO_PARTITION == readWriteType )
         {
            ( void ) PAR_partitionFptr.parWrite( ( dSize )dest, imageBuffer, ( uint32_t ) bufSize, pImagePTbl );
         }
         else
         {
            memcpy( imageBuffer, ( uint8_t * )dest, ( size_t ) bufSize );
            seekFlagEnabled = true;
         }
      }
   }
   else
#endif
   {
      if( eBSPATCH_WRITE_INTO_PARTITION == readWriteType )
      {
         ( void ) PAR_partitionFptr.parWrite( ( dSize )dest, imageBuffer, ( uint32_t ) bufSize, pImagePTbl );
      }
      else
      {
         memcpy( imageBuffer, ( uint8_t * )dest, ( size_t ) bufSize );
      }
   }
}

/***********************************************************************************************************************

   Function Name: MINIBS_patch

   Purpose: It creates the new image by patching the old image. The new image will be stored in external NV

   Arguments: PartitionData_t const *pOldImagePTbl, eFwTarget_t patchTarget, PartitionData_t const *pNewImagePTbl,
              off_t imageSize, compressedData_position_t *patchOffset

   Returns: returnStatus_t

   Side Effects: Nothing

   Reentrant Code: Yes

   Notes:  Apply a patch stored in 'patch' to 'oldp', result in 'newp', and store the result in 'newp'.

           The input pointers must not be NULL.

           The size of 'newp', represented by 'newsz', must be at least
           'bspatch_newsize(oldsz,patchsz)' bytes in length.

           Returns -1 if memory can't be allocated, or the input pointers are NULL.
           Returns -2 if the patch header is invalid. Returns -3 if the patch itself is
           corrupt.
           Otherwise, returns 0.

           This function requires n+m+O(1) bytes of memory, where n is the size of the
           old file and m is the size of the new file. It does no allocations.
           It runs in O(n+m) time.

**********************************************************************************************************************/
returnStatus_t MINIBS_patch( PartitionData_t const *pOldImagePTbl, eFwTarget_t patchTarget,
                             PartitionData_t const *pNewImagePTbl, off_t imageSize, compressedData_position_t *patchOffset )
{
   off_t   oldpos, newpos;
#if ( ( DCU == 1 ) || ( MCU_SELECTED == RA6E1 ) )
#else
   off_t   oldPosBeforeSeek;
#endif
   off_t   reservedSectionSize = 0;
   off_t   ctrl;
   off_t   i, j;
   uint8_t oldImgBuf[LOCAL_READ_WRITE_BUFFER_SIZE];
   uint8_t newImgBuf[LOCAL_READ_WRITE_BUFFER_SIZE];

   /* Sanity checks */
   if ( pOldImagePTbl == NULL || pNewImagePTbl == NULL )
   {
      return eFAILURE; // -1
   }

   if ( imageSize < 0 )
   {
      return eFAILURE; // -1
   }

   /* Apply patch */
#if ( ( DCU == 1 ) || ( MCU_SELECTED == RA6E1 ) )
   oldpos = 0;
#else
   oldpos = BL_CODE_START;
   /* Set the reserved section size only if the patch is for application */
   if( eFWT_APP == patchTarget )
   {
      reservedSectionSize = ( RESERVED_SECTION_1_STOP_BYTES - RESERVED_SECTION_1_START_BYTES ) + 1;
   }
#endif
   newpos = 0;

   while( newpos < ( imageSize + reservedSectionSize ) )
   {
      /***  DIFF  ***/
      /* Read in DIFF cmd length */
      ctrl = DFW_XZMINI_uncompressedData_read_ssize( patchOffset );
      /* Sanity check */
      if( newpos + ctrl > ( imageSize + reservedSectionSize ) )
      {
         return eFAILURE; // Corrupt patch -3
      }

      i = 0;
      while (i < ctrl)
      {
         off_t bufSizeToRead = LOCAL_READ_WRITE_BUFFER_SIZE;
         if( ( ctrl - i ) < bufSizeToRead )
         {
            bufSizeToRead = ctrl - i;
         }

         /* Read from internal memory of bufSizeToRead bytes to oldImgBuf */
         readAndWriteBuffer( &oldpos, i, bufSizeToRead, reservedSectionSize,
                             oldImgBuf, eBSPATCH_READ_FROM_MEMORY, NULL );

         for( j = 0; j < bufSizeToRead; j++ )
         {
            newImgBuf[j] = DFW_XZMINI_uncompressedData_getchar( patchOffset ); /* Get a byte from the patch partition */
            newImgBuf[j] += oldImgBuf[j]; /* Add with the app code */
         }

         /* Write the created buffer in the partition */
         readAndWriteBuffer( &newpos, i, bufSizeToRead, reservedSectionSize,
                             newImgBuf, eBSPATCH_WRITE_INTO_PARTITION, pNewImagePTbl );

         i += bufSizeToRead;
      }

      /* Adjust new/old image pointers */
      newpos += ctrl; // Add length of diff just executed
      oldpos += ctrl; // Add length of diff just executed

      /*** EXTRA ***/
      /* Read in EXTRA cmd length */
      ctrl = DFW_XZMINI_uncompressedData_read_ssize( patchOffset );
      /* Sanity check */
      if( newpos + ctrl > ( imageSize + reservedSectionSize ) )
      {
         return eFAILURE; // Corrupt patch -3
      }

      /* Read extra string */
      i = 0;
      while (i < ctrl)
      {
         off_t bufSizeToRead = LOCAL_READ_WRITE_BUFFER_SIZE;
         if( ( ctrl - i ) < bufSizeToRead )
         {
            bufSizeToRead = ctrl - i;
         }

         for( j = 0; j < bufSizeToRead; j++ )
         {
            newImgBuf[j] = DFW_XZMINI_uncompressedData_getchar( patchOffset ); /* Get a byte from the patch partition */
         }

         /* Write the created buffer in the partition */
         readAndWriteBuffer( &newpos, i, bufSizeToRead, reservedSectionSize,
                             newImgBuf, eBSPATCH_WRITE_INTO_PARTITION, pNewImagePTbl );
         i += bufSizeToRead;
      }

      /* Adjust new image pointer */
      newpos += ctrl; // Add length of extra just executed

      /*** SEEK ***/
      /* Read in SEEK cmd length */
#if ( ( DCU == 1 ) || ( MCU_SELECTED == RA6E1 ) )
#else
      oldPosBeforeSeek = oldpos; // Store the position of the old image before calculating seek/skip amount
#endif
      ctrl = DFW_XZMINI_uncompressedData_read_ssize( patchOffset );
      oldpos += ctrl; // Seek/skip forward the specified amount in the old image
#if ( ( DCU == 1 ) || ( MCU_SELECTED == RA6E1 ) )
#else
      if( ( ( oldpos <= RESERVED_SECTION_1_STOP_BYTES ) && ( oldpos >= RESERVED_SECTION_1_START_BYTES ) ) ||
          ( ( oldPosBeforeSeek <= RESERVED_SECTION_1_START_BYTES ) && ( oldpos >= RESERVED_SECTION_1_STOP_BYTES ) ) ||
          ( ( oldPosBeforeSeek >= RESERVED_SECTION_1_STOP_BYTES ) && ( oldpos <= RESERVED_SECTION_1_START_BYTES ) ) )
      {
         if( !seekFlagEnabled )
         {
            /* To overcome the scenario which happens when oldpos seeks from position greater than end of reserved section
               to the reserved section or before the reserved section size. Also seekFlag should be false/disabled */
            oldpos += reservedSectionSize;
            seekFlagEnabled = true;
         }
         else
         {
            if( ctrl < 0 )
            {
               oldpos -= reservedSectionSize;
            }
            else
            {
               oldpos += reservedSectionSize;
            }
         }
      }
#endif
   };

#if ( ( DCU == 1 ) || ( MCU_SELECTED == RA6E1 ) )
#else
   seekFlagEnabled = false;
#endif
   DFW_XZMINI_bspatch_close();
   return eSUCCESS;  // 0 - success
}
