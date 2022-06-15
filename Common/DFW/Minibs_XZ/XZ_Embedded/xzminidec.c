/**********************************************************************************************************************
 *
 * Filename:   xzminidec.c
 *
 * Global Designator: XZMINIDEC_
 *
 * Contents: This module is the interface between XZ and minibs. XZ - decompression lib, minibs - patching lib
 *
 **********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "dfw_xzmini_interface.h"
#include "xz.h"
#include "xzminidec.h"

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define XZ_DATA_BUFFER_SIZE    512  // Size of data to be read from patch table and size to be uncompressed from compressed patch
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static uint8_t        compressedInputBuffer[XZ_DATA_BUFFER_SIZE];
static uint8_t        uncompressedOutputBuffer[XZ_DATA_BUFFER_SIZE];
static uint32_t       getchar_pos = 0;
static xz_buffer_t    xzDataBuffer;
static struct xz_dec  *xzDecompress;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

static xz_returnStatus_t XZMINIDEC_run( compressedData_position_t *patchOffset );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: XZMINIDEC_error

   Purpose: It prints out the error in the debug port. Further it cleans up the xzmini_decompress variables.

   Arguments: const char *msg

   Returns: Nothing

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static void XZMINIDEC_error( const char *msg )
{
   DFW_XZMINI_PRNT_ERROR_PARMS( "XZMINIDEC_error - %s", msg );
   XZ_DEC_STREAM_end( xzDecompress );
}

/***********************************************************************************************************************

   Function Name: XZMINIDEC_init

   Purpose: It initializes xz and minibs modules. It sets the xzDataBuffer structure with input and
            output buffer which is used for patching. Further, it sets the initial XZ_DATA_BUFFER_SIZE bytes
            of compressed input from the pDFWPatchPTbl_. Also, it uncompresses the initial XZ_DATA_BUFFER_SIZE bytes
            of uncompressed data from input buffer.

   Arguments: compressedData_position_t *patchOffset

   Returns: xz_returnStatus_t - returns the status of the initialization

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
xz_returnStatus_t XZMINIDEC_init( compressedData_position_t *patchOffset )
{
   xz_returnStatus_t returnValue = XZ_OK;
   getchar_pos = 0;     // Set to initial value of getchar_pos to 0

   /* Support up to 64 MiB dictionary. The actually needed memory
    * is allocated once the headers have been parsed. */
   xzDecompress = XZ_DEC_STREAM_init( XZ_DYNALLOC, 1 << 12 ); /* [2021-05-24] MKD limit to 4KiB because of lack of free RAM. This implies using --lzma2=dict=4096,lc=0,pb=1,nice=192 when compressing */
   if (xzDecompress == NULL)
   {
      XZMINIDEC_error( "Memory allocation failed" ); // TODO: Put prints
      returnValue = XZ_MEM_ERROR;
   }
   else
   {
      xzDataBuffer.input        = compressedInputBuffer;
      xzDataBuffer.input_pos    = 0;
      xzDataBuffer.input_size   = 0;
      xzDataBuffer.output       = uncompressedOutputBuffer;
      xzDataBuffer.output_pos   = 0;
      xzDataBuffer.output_size  = XZ_DATA_BUFFER_SIZE;
      /* Start running the xz decompress with minibs patching */
      returnValue = XZMINIDEC_run( patchOffset );
   }

   return returnValue;
}

/***********************************************************************************************************************

   Function Name: XZMINIDEC_end

   Purpose: It cleans up the xz and minibs dependencies

   Arguments: Nothing

   Returns: Nothing

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
void XZMINIDEC_end()
{
   XZ_DEC_STREAM_end( xzDecompress );   // Clean xz decompress structures and variables
}

/***********************************************************************************************************************

   Function Name: XZMINIDEC_run

   Purpose: It runs the decompression required for patching. This is a upper layer API
            for calling XZ_DEC_STREAM_run which does the decompression. Further, this is used to
            take the compressed input from pDFWPatch table and re-fill the buffer

   Arguments: compressedData_position_t *patchOffset

   Returns: xz_returnStatus_t - return the status

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t XZMINIDEC_run( compressedData_position_t *patchOffset )
{
   xz_returnStatus_t returnValue;
   /* If out buffer is full, re-fill buffer */
   if ( xzDataBuffer.output_pos == sizeof( uncompressedOutputBuffer ) )
   {
      xzDataBuffer.output_pos = 0;
   }

   /* If input buffer is consumed, re-fill buffer */
   if ( xzDataBuffer.input_pos == xzDataBuffer.input_size )
   {
      uint32_t bytesToRead = XZ_DATA_BUFFER_SIZE;
      if ( ( patchOffset->patchSrcOffset + XZ_DATA_BUFFER_SIZE ) > patchOffset->patchEndOffset )
      {
         bytesToRead = patchOffset->patchEndOffset - patchOffset->patchSrcOffset;
      }

      if ( eSUCCESS == PAR_partitionFptr.parRead( compressedInputBuffer, patchOffset->patchSrcOffset, 
                                                  bytesToRead, patchOffset->pPatchPartition ) )
      {
         patchOffset->patchSrcOffset += bytesToRead;
         xzDataBuffer.input_size      = bytesToRead;
         xzDataBuffer.input_pos       = 0;
      }
   }

   returnValue = XZ_DEC_STREAM_run( xzDecompress, &xzDataBuffer );
   if ( returnValue != XZ_OK )
   {
      switch ( returnValue )
      {
         case XZ_STREAM_END:
         {
            /* This used to have "XZ_DEC_STREAM_end( xzDecompress );", which now must be called outside this function, especially if the decoder will be re-used */
            break;
         }
         case XZ_MEM_ERROR:
         {
            XZMINIDEC_error( "Memory allocation failed" );
            break;
         }
         case XZ_UNSUPPORTED_CHECK:
         {
            XZMINIDEC_error( "Unsupported check" );
            break;
         }
         case XZ_MEMLIMIT_ERROR:
         {
            XZMINIDEC_error( "Memory usage limit reached" );
            break;
         }
         case XZ_FORMAT_ERROR:
         {
            XZMINIDEC_error( "Not a .xz file" );
            break;
         }
         case XZ_OPTIONS_ERROR:
         {
            XZMINIDEC_error( "Unsupported options in the .xz headers" );
            break;
         }
         case XZ_DATA_ERROR:
         case XZ_BUF_ERROR:
         {
            XZMINIDEC_error( "File is corrupt" );
            break;
         }
         default:
         {
            XZMINIDEC_error( "Bug!" );
            break;
         }
      } //lint !e788,  XZ_OK not used within default switch as this switch case statement is used only for output the error
   }

   return returnValue;
}

/***********************************************************************************************************************

   Function Name: XZMINIDEC_getchar

   Purpose: It gets the 1byte data from the uncompressed output buffer. If the output buffer
            completed, to re-fill, this calls XZMINIDEC_run API.

   Arguments: compressedData_position_t *patchOffset

   Returns: u_char - return a byte data of uncompressed patch output

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
u_char XZMINIDEC_getchar( compressedData_position_t *patchOffset )
{
   if( getchar_pos >= xzDataBuffer.output_pos )
   {
      ( void ) XZMINIDEC_run( patchOffset );
   }

   if( getchar_pos == xzDataBuffer.output_size )
   {
      getchar_pos = 0;
   }

   return xzDataBuffer.output[getchar_pos++];
}
