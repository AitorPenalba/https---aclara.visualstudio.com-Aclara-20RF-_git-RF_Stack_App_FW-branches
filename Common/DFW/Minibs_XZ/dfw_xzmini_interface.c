/**********************************************************************************************************************
 *
 * Filename:   dfw_xzmini_interface.c
 *
 * Global Designator: DFW_XZMINI_
 *
 * Contents: This module acts as a interface to communicate dfw application with xz and minibs modules.
 *           XZ used to decompress the incoming compressed packets.
 *           minibs used to patch the old firmware file to convert to the required new version of the image
 *
 **********************************************************************************************************************
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
 *********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "dfw_xzmini_interface.h"
#include "xzminidec.h"
#include "bspatch.h"

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static compressedData_position_t patchOffsetDetail_;  /* Structure to store the partition table pointer, moving position value, end offset value */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: DFW_XZMINI_uncompressPatch_init

   Purpose: Initializes the required things for uncompressing and do the patching.
            It initializes the input and output buffer of the compressed patch.
            Further it stores the moving address and patch end address to a structure

   Arguments: dSize patchSrcOffset, dSize patchEndOffset

   Returns: returnStatus_t

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
returnStatus_t DFW_XZMINI_uncompressPatch_init( const PartitionData_t *pPatchPartition,
                                                dSize patchSrcOffset, dSize patchEndOffset )
{
   returnStatus_t returnValue = eSUCCESS;
   /* Set the structure of patchOffsetDetail with patch partition, src and end offset */
   patchOffsetDetail_.pPatchPartition = pPatchPartition;
   patchOffsetDetail_.patchSrcOffset  = patchSrcOffset;
   patchOffsetDetail_.patchEndOffset  = patchEndOffset;
   /* Initialize xz and minibs */
   if( XZ_OK != XZMINIDEC_init( &patchOffsetDetail_ ) )
   {
      // Debug prints with XZ enum value
      DFW_XZMINI_PRNT_INFO_PARMS( "Init failed - Value %d ", returnValue );
      returnValue = eFAILURE;
   }

   return returnValue;
}

/***********************************************************************************************************************

   Function Name: convertCharBufArrayToInt32

   Purpose: Uses the character pointer and converts 4 bytes of char to int32 with
            checking Endianess

   Arguments: u_char *buffer

   Returns: int32_t - converted char buffer array

   Side Effects: Nothing

   Reentrant Code: Yes

   Notes:

**********************************************************************************************************************/
static int32_t convertCharBufArrayToInt32 ( const u_char *buffer )
{
   int32_t bufInt32;

   bufInt32   = buffer[0] & 0x7F;
   bufInt32  *= 256;

   bufInt32  += buffer[1];
   bufInt32  *= 256;

   bufInt32  += buffer[2];
   bufInt32  *= 256;

   bufInt32  +=buffer[3];
   if( buffer[0] & 0x80 )
   {
      bufInt32 = -bufInt32;
   }

   return bufInt32;
}

/***********************************************************************************************************************

   Function Name: DFW_XZMINI_uncompressedData_read_ssize

   Purpose: Gets the uncompressed data buffer xzDataBuffer.out structure of 4 bytes
            It obtains the data byte by byte so to prevent the need of uncompression from xzDataBuffer.in structure

   Arguments: compressedData_position_t *patchOffset

   Returns: int32_t - 4 bytes of uncompressed data

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
int32_t DFW_XZMINI_uncompressedData_read_ssize( compressedData_position_t *patchOffset )
{
   u_char buffer[4];
   u_char *buffer_p = &buffer[0];

   buffer[0] = DFW_XZMINI_uncompressedData_getchar( patchOffset );
   buffer[1] = DFW_XZMINI_uncompressedData_getchar( patchOffset );
   buffer[2] = DFW_XZMINI_uncompressedData_getchar( patchOffset );
   buffer[3] = DFW_XZMINI_uncompressedData_getchar( patchOffset );

   return convertCharBufArrayToInt32( buffer_p );
}

/***********************************************************************************************************************

   Function Name: DFW_XZMINI_uncompressedData_getchar

   Purpose: Gets the uncompressed data buffer xzDataBuffer.out structure of 1 byte.
            It further calls xzminidec_getchar() which uncompress the data from xzDataBuffer.in structure if required.
            i.e., It loads another block of memory into the uncompressed data from compressed data by using xz
            mechanism if uncompressed data buffer utilized up to the end

   Arguments: compressedData_position_t *patchOffset

   Returns: u_char - 1 byte of uncompressed data

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
u_char DFW_XZMINI_uncompressedData_getchar( compressedData_position_t *patchOffset )
{
   return( XZMINIDEC_getchar( patchOffset ) );
}

/***********************************************************************************************************************

   Function Name: DFW_XZMINI_bspatch_close

   Purpose: Closes and cleans the xz and mini params

   Arguments: Nothing

   Returns: Nothing

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
void DFW_XZMINI_bspatch_close()
{
   XZMINIDEC_end();  // Close xz and minibsdiff dependencies once patching has been completed
}

/***********************************************************************************************************************

   Function Name: DFW_XZMINI_bspatch_valid_header

   Purpose: It validates the patch header values - contol length, data length and new patch size
            It updates the values of these in their address

   Arguments: int32_t* newsize, int32_t* ctrllen, int32_t* datalen

   Returns: returnStatus_t

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
returnStatus_t DFW_XZMINI_bspatch_valid_header( int32_t* newsize, int32_t* ctrllen, int32_t* datalen )
{
   returnStatus_t returnStatus = eSUCCESS;
   /* Make sure header fields are valid */
   *ctrllen = DFW_XZMINI_uncompressedData_read_ssize( &patchOffsetDetail_ );
   *datalen = DFW_XZMINI_uncompressedData_read_ssize( &patchOffsetDetail_ );
   *newsize = DFW_XZMINI_uncompressedData_read_ssize( &patchOffsetDetail_ ); //size of new file that will result from applying the patch

   DFW_XZMINI_PRNT_INFO_PARMS( "Control Length: %ld, Data Length: %ld, New Size: %ld", *ctrllen, *datalen, *newsize );

   if( ( *ctrllen < 0 ) || ( *datalen < 0 ) || ( *newsize <= 0 ) )
   {
      returnStatus = eFAILURE;
      DFW_XZMINI_PRNT_INFO( "Invalid bspatch header" );
   }

   return returnStatus;
}

/***********************************************************************************************************************

   Function Name: DFW_XZMINI_bspatch

   Purpose: Complete the patching using minibs

   Arguments: PartitionData_t const *pOldImagePTbl, eFwTarget_t patchTarget, PartitionData_t const *pNewImagePTbl, off_t newImageSize

   Returns: Nothing

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
returnStatus_t DFW_XZMINI_bspatch( PartitionData_t const *pOldImagePTbl, eFwTarget_t patchTarget,
                                   PartitionData_t const *pNewImagePTbl, off_t newImageSize )
{
   return MINIBS_patch( pOldImagePTbl, patchTarget, pNewImagePTbl, newImageSize, &patchOffsetDetail_ );
}
