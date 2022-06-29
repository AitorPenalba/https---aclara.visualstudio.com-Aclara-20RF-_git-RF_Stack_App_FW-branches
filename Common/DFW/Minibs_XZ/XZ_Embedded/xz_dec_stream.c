/***********************************************************************************************************************
 *
 * Filename: xz_dec_stream.c
 *
 * Global Designator: XZ_DEC_STREAM_
 *
 * Contents: This module does the decompression of the compressed input buffer.
 *
 *
 ***********************************************************************************************************************
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
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "xz_private.h"
#include "xz_stream.h"
#include "crc32.h"

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* Hash used to validate the Index field */
struct xz_dec_hash {
   vli_type unpadded;
   vli_type uncompressed;
   uint32_t crc32;
};

struct xz_dec {
   /* Position in dec_main() */
   enum {
      SEQ_STREAM_HEADER,
      SEQ_BLOCK_START,
      SEQ_BLOCK_HEADER,
      SEQ_BLOCK_UNCOMPRESS,
      SEQ_BLOCK_PADDING,
      SEQ_BLOCK_CHECK,
      SEQ_INDEX,
      SEQ_INDEX_PADDING,
      SEQ_INDEX_CRC32,
      SEQ_STREAM_FOOTER
   } sequence;

   /* Position in variable-length integers and Check fields */
   uint32_t pos;

   /* Variable-length integer decoded by dec_vli() */
   vli_type vli;

   /* Saved input_pos and output_pos */
   size_t in_start;
   size_t out_start;

#ifdef XZ_USE_CRC64
   /* CRC32 or CRC64 value in Block or CRC32 value in Index */
   uint64_t crc;
#else
   /* CRC32 value in Block or Index */
   uint32_t crc;
#endif

   /* Type of the integrity check calculated from uncompressed data */
   enum xz_check check_type;

   /* Operation mode */
   xz_mode_t mode;

   /* True if the next call to XZ_DEC_STREAM_run() is allowed to return XZ_BUF_ERROR */
   bool allow_buf_error;

   /* Information stored in Block Header */
   struct {
      /* Value stored in the Compressed Size field, or
       * VLI_UNKNOWN if Compressed Size is not present. */
      vli_type compressed;
      /* Value stored in the Uncompressed Size field, or
       * VLI_UNKNOWN if Uncompressed Size is not present. */
      vli_type uncompressed;
      /* Size of the Block Header field */
      uint32_t size;
	} block_header;

   /* Information collected when decoding Blocks */
   struct {
      /* Observed compressed size of the current Block */
      vli_type compressed;
      /* Observed uncompressed size of the current Block */
      vli_type uncompressed;
      /* Number of Blocks decoded so far */
      vli_type count;
      /* Hash calculated from the Block sizes. This is used to
       * validate the Index field. */
      struct xz_dec_hash hash;
   } block;

   /* Variables needed when verifying the Index field */
   struct {
	   /* Position in dec_index() */
	   enum {
	   	SEQ_INDEX_COUNT,
	   	SEQ_INDEX_UNPADDED,
	   	SEQ_INDEX_UNCOMPRESSED
	   } sequence;
      /* Size of the Index in bytes */
      vli_type size;
      /* Number of Records (matches block.count in valid files) */
      vli_type count;
      /* Hash calculated from the Records (matches block.hash in
       * valid files). */
      struct xz_dec_hash hash;
   } index;

   /* Temporary buffer needed to hold Stream Header, Block Header,
    * and Stream Footer. The Block Header is the biggest (1 KiB)
    * so we reserve space according to that. buf[] has to be aligned
    * to a multiple of four bytes; the size_t variables before it
    * should guarantee this. */
   struct {
      size_t pos;
      size_t size;
      uint8_t buf[1024];
   } temp;

   struct xz_dec_lzma2 *lzma2;

#ifdef XZ_DEC_BCJ
   struct xz_dec_bcj *bcj;
   bool bcj_active;
#endif

};

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#ifdef XZ_USE_CRC64
#define IS_CRC64(check_type) ((check_type) == XZ_CHECK_CRC64)
#else
#define IS_CRC64(check_type) false
#endif

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
#if ( DCU == 1 )
/* Use External RAM on DCU */
struct xz_dec xzDecompression @ "EXTERNAL_RAM"; /*lint !e430*/
#else
struct xz_dec xzDecompression;
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */

#ifdef XZ_DEC_ANY_CHECK
/* Sizes of the Check field with different Check IDs */
static const uint8_t check_sizes[16] = {
   0,
   4, 4, 4,
   8, 8, 8,
   16, 16, 16,
   32, 32, 32,
   64, 64, 64
};
#endif

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function Name: xz_calcCRC32

   Purpose: It calculates the CRC using the buffer and with the pre-calculated CRC start value

   Arguments: const uint8_t *buf, size_t size, uint32_t crc

   Returns: uint32_t - crc

   Side Effects: Nothing

   Reentrant Code: Yes

   Notes:

**********************************************************************************************************************/
static uint32_t xz_calcCRC32( const uint8_t *buf, size_t size, uint32_t crc )
{
   crc32Cfg_t crcCfg;
   uint32_t   calcCRC;
   CRC32_init( ~crc, CRC32_DFW_POLY, eCRC32_RESULT_INVERT, &crcCfg ); /* Init CRC */
   calcCRC = CRC32_calc( buf, size, &crcCfg ); /* Update the CRC */

   return calcCRC;
}

/***********************************************************************************************************************

   Function Name: fill_temp

   Purpose: This fills decompressionInfo->temp by copying data starting from buffer->in[buffer->input_pos]. Caller
            must have set decompressionInfo->temp.pos to indicate how much data we are supposed to copy
            into decompressionInfo->temp.buf. Return true once decompressionInfo->temp.pos has reached decompressionInfo->temp.size.

   Arguments: struct xz_dec *decompressionInfo, xz_buffer_t *buffer

   Returns: true or false

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static bool fill_temp( struct xz_dec *decompressionInfo, xz_buffer_t *buffer )
{
   size_t copy_size = min_t( size_t, buffer->input_size - buffer->input_pos,
                             decompressionInfo->temp.size - decompressionInfo->temp.pos );

   memcpy( decompressionInfo->temp.buf + decompressionInfo->temp.pos,
           buffer->input + buffer->input_pos, copy_size );

   buffer->input_pos           += copy_size;
   decompressionInfo->temp.pos += copy_size;

   if (decompressionInfo->temp.pos == decompressionInfo->temp.size) {
      decompressionInfo->temp.pos = 0;
      return true;
   }

   return false;
}

/***********************************************************************************************************************

   Function Name: dec_vli

   Purpose: Decode a variable-length integer (little-endian base-128 encoding)

   Arguments: struct xz_dec *decompressionInfo, const uint8_t *in,
              size_t *input_pos, size_t input_size

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t dec_vli( struct xz_dec *decompressionInfo, const uint8_t *in,
                                  size_t *input_pos, size_t input_size )
{
   uint8_t byte;
   if ( decompressionInfo->pos == 0 )
   {
      decompressionInfo->vli = 0;
   }

   while( *input_pos < input_size ) {
      byte = in[*input_pos];
      ++*input_pos;
      decompressionInfo->vli |= (vli_type)(byte & 0x7F) << decompressionInfo->pos;
      if ( ( byte & 0x80 ) == 0 )
      {
         /* Don't allow non-minimal encodings. */
         if ( byte == 0 && decompressionInfo->pos != 0 )
         {
            return XZ_DATA_ERROR;
         }

         decompressionInfo->pos = 0;
         return XZ_STREAM_END;
      }

      decompressionInfo->pos += 7;
      if ( decompressionInfo->pos == 7 * VLI_BYTES_MAX )
      {
         return XZ_DATA_ERROR;
      }
   }

   return XZ_OK;
}

/***********************************************************************************************************************

   Function Name: dec_block

   Purpose:  Decode the Compressed Data field from a Block. Update and validate the
             observed compressed and uncompressed sizes of the Block so that they don't
             exceed the values possibly stored in the Block Header (validation assumes
             that no integer overflow occurs, since vli_type is normally uint64_t).
             Update the CRC32 or CRC64 value if presence of the CRC32 or
             CRC64 field was indicated in Stream Header.

             Once the decoding is finished, validate that the observed sizes match
             the sizes possibly stored in the Block Header. Update the hash and
             Block count, which are later used to validate the Index field.

   Arguments: struct xz_dec *decompressionInfo, xz_buffer_t *buffer

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t dec_block( struct xz_dec *decompressionInfo, xz_buffer_t *buffer )
{
   xz_returnStatus_t returnValue;

   decompressionInfo->in_start = buffer->input_pos;
   decompressionInfo->out_start = buffer->output_pos;

#ifdef XZ_DEC_BCJ
   if ( decompressionInfo->bcj_active )
   {
      returnValue = xz_dec_bcj_run( decompressionInfo->bcj, decompressionInfo->lzma2, buffer );
   }
   else
#endif
   {
      returnValue = XZ_DEC_LZMA2_run(decompressionInfo->lzma2, buffer);
   }

   decompressionInfo->block.compressed += buffer->input_pos - decompressionInfo->in_start;
   decompressionInfo->block.uncompressed += buffer->output_pos - decompressionInfo->out_start;

   /* There is no need to separately check for VLI_UNKNOWN, since
    * the observed sizes are always smaller than VLI_UNKNOWN. */
   if ( decompressionInfo->block.compressed > decompressionInfo->block_header.compressed ||
        decompressionInfo->block.uncompressed > decompressionInfo->block_header.uncompressed )
   {
      return XZ_DATA_ERROR;
   }

   if ( decompressionInfo->check_type == XZ_CHECK_CRC32 )
   {
      decompressionInfo->crc = xz_calcCRC32( buffer->output + decompressionInfo->out_start,
                                             buffer->output_pos - decompressionInfo->out_start,
                                             decompressionInfo->crc );
   }
#ifdef XZ_USE_CRC64
   else if (decompressionInfo->check_type == XZ_CHECK_CRC64)
   {
      decompressionInfo->crc = xz_crc64( buffer->out + decompressionInfo->out_start,
                                         buffer->output_pos - decompressionInfo->out_start,
                                         decompressionInfo->crc);
   }
#endif

   if ( returnValue == XZ_STREAM_END )
   {
      if ( decompressionInfo->block_header.compressed != VLI_UNKNOWN &&
           decompressionInfo->block_header.compressed != decompressionInfo->block.compressed )
      {
         return XZ_DATA_ERROR;
      }

      if ( decompressionInfo->block_header.uncompressed != VLI_UNKNOWN &&
           decompressionInfo->block_header.uncompressed != decompressionInfo->block.uncompressed )
      {
         return XZ_DATA_ERROR;
      }

      decompressionInfo->block.hash.unpadded += decompressionInfo->block_header.size + decompressionInfo->block.compressed;
#ifdef XZ_DEC_ANY_CHECK
      decompressionInfo->block.hash.unpadded += check_sizes[decompressionInfo->check_type];
#else
      if ( decompressionInfo->check_type == XZ_CHECK_CRC32 )
      {
         decompressionInfo->block.hash.unpadded += 4;
      }
      else if ( IS_CRC64(decompressionInfo->check_type) ) //lint !e506 !e774, constant value boolean, it always evaluates to false - file cannot be modified as it is external core file
      {
         decompressionInfo->block.hash.unpadded += 8;
      }
#endif
      decompressionInfo->block.hash.uncompressed += decompressionInfo->block.uncompressed;
      decompressionInfo->block.hash.crc32 = xz_calcCRC32( ( const uint8_t * ) &decompressionInfo->block.hash,
                                                           sizeof( decompressionInfo->block.hash ),
                                                           decompressionInfo->block.hash.crc32 );
      ++decompressionInfo->block.count;
   }

   return returnValue;
}

/***********************************************************************************************************************

   Function Name: index_update

   Purpose:  Update the Index size and the CRC32 value.

   Arguments: struct xz_dec *decompressionInfo, const xz_buffer_t *buffer

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static void index_update( struct xz_dec *decompressionInfo, const xz_buffer_t *buffer )
{
   size_t in_used = buffer->input_pos - decompressionInfo->in_start;
   decompressionInfo->index.size += in_used;
   decompressionInfo->crc = xz_calcCRC32( buffer->input + decompressionInfo->in_start,
                                          in_used, decompressionInfo->crc );
}

/***********************************************************************************************************************

   Function Name: dec_index

   Purpose:  Decode the Number of Records, Unpadded Size, and Uncompressed Size fields
             from the Index field. That is, Index Padding and CRC32 are not decoded by this function.

             This can return XZ_OK (more input needed), XZ_STREAM_END (everything successfully
             decoded), or XZ_DATA_ERROR (input is corrupt).

   Arguments: struct xz_dec *decompressionInfo, xz_buffer_t *buffer

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t dec_index( struct xz_dec *decompressionInfo, xz_buffer_t *buffer )
{
   xz_returnStatus_t ret;

   do
   {
      ret = dec_vli( decompressionInfo, buffer->input, &buffer->input_pos, buffer->input_size );
      if ( ret != XZ_STREAM_END )
      {
         index_update( decompressionInfo, buffer );
         return ret;
      }

      switch ( decompressionInfo->index.sequence )
      {
         case SEQ_INDEX_COUNT:
         {
            decompressionInfo->index.count = decompressionInfo->vli;
            /*
             * Validate that the Number of Records field
             * indicates the same number of Records as
             * there were Blocks in the Stream.
             */
            if ( decompressionInfo->index.count != decompressionInfo->block.count )
            {
               return XZ_DATA_ERROR;
            }

            decompressionInfo->index.sequence = SEQ_INDEX_UNPADDED;
            break;
         }
         case SEQ_INDEX_UNPADDED:
         {
            decompressionInfo->index.hash.unpadded += decompressionInfo->vli;
            decompressionInfo->index.sequence = SEQ_INDEX_UNCOMPRESSED;
            break;
         }
         case SEQ_INDEX_UNCOMPRESSED:
         {
            decompressionInfo->index.hash.uncompressed += decompressionInfo->vli;
            decompressionInfo->index.hash.crc32 = xz_calcCRC32( ( const uint8_t * )&decompressionInfo->index.hash,
                                                                sizeof( decompressionInfo->index.hash ),
                                                                decompressionInfo->index.hash.crc32 );
            --decompressionInfo->index.count;
            decompressionInfo->index.sequence = SEQ_INDEX_UNPADDED;
            break;
         }
      }
	} while (decompressionInfo->index.count > 0);

	return XZ_STREAM_END;
}

/***********************************************************************************************************************

   Function Name: crc_validate

   Purpose:  Validate that the next four or eight input bytes match the value of decompressionInfo->crc.
             decompressionInfo->pos must be zero when starting to validate the first byte.
             The "bits" argument allows using the same code for both CRC32 and CRC64.

   Arguments: struct xz_dec *decompressionInfo, xz_buffer_t *buffer, uint32_t bits

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t crc_validate( struct xz_dec *decompressionInfo,
                                       xz_buffer_t *buffer, uint32_t bits )
{
   do
   {
      if ( buffer->input_pos == buffer->input_size )
      {
         return XZ_OK;
      }

      if ( ( ( decompressionInfo->crc >> decompressionInfo->pos ) & 0xFF ) !=
           buffer->input[buffer->input_pos++] )
      {
         return XZ_DATA_ERROR;
      }

      decompressionInfo->pos += 8;
   } while ( decompressionInfo->pos < bits );

   decompressionInfo->crc = 0;
   decompressionInfo->pos = 0;

   return XZ_STREAM_END;
}

#ifdef XZ_DEC_ANY_CHECK
/***********************************************************************************************************************

   Function Name: check_skip

   Purpose: Skip over the Check field when the Check ID is not supported.
            Returns true once the whole Check field has been skipped over.

   Arguments: struct xz_dec *decompressionInfo, xz_buffer_t *buffer

   Returns: true/false

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static bool check_skip( struct xz_dec *decompressionInfo, xz_buffer_t *buffer )
{
   while ( decompressionInfo->pos < check_sizes[decompressionInfo->check_type] )
   {
      if ( buffer->input_pos == buffer->input_size )
      {
         return false;
      }

      ++buffer->input_pos;
      ++decompressionInfo->pos;
   }

   decompressionInfo->pos = 0;

   return true;
}

#endif /* XZ_DEC_ANY_CHECK */

/***********************************************************************************************************************

   Function Name: dec_stream_header

   Purpose: Decode the Stream Header field (the first 12 bytes of the .xz Stream).

   Arguments: struct xz_dec *decompressionInfo

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t dec_stream_header( struct xz_dec *decompressionInfo )
{
   if ( !memeq( decompressionInfo->temp.buf, HEADER_MAGIC, HEADER_MAGIC_SIZE ) ) //lint !e541, Excessive size - not modifying as it is from the external source
   {
      return XZ_FORMAT_ERROR;
   }

   if ( ( xz_calcCRC32( decompressionInfo->temp.buf + HEADER_MAGIC_SIZE, 2, 0 ) !=
        get_le32( decompressionInfo->temp.buf + HEADER_MAGIC_SIZE + 2 ) ) )
   {
      return XZ_DATA_ERROR;
   }

   if ( decompressionInfo->temp.buf[HEADER_MAGIC_SIZE] != 0 )
   {
      return XZ_OPTIONS_ERROR;
   }

   /* Of integrity checks, we support none ( Check ID = 0 ), CRC32 ( Check ID = 1 ), and
    * optionally CRC64 ( Check ID = 4 ). However, if XZ_DEC_ANY_CHECK is defined,
    * we will accept other check types too, but then the check won't be verified and
    * a warning ( XZ_UNSUPPORTED_CHECK ) will be given. */
   decompressionInfo->check_type = ( enum xz_check ) decompressionInfo->temp.buf[HEADER_MAGIC_SIZE + 1];

#ifdef XZ_DEC_ANY_CHECK
   if ( decompressionInfo->check_type > XZ_CHECK_MAX )
   {
      return XZ_OPTIONS_ERROR;
   }

   if ( decompressionInfo->check_type > XZ_CHECK_CRC32 && !IS_CRC64( decompressionInfo->check_type ) )
   {
      return XZ_UNSUPPORTED_CHECK;
   }
#else
   if ( decompressionInfo->check_type > XZ_CHECK_CRC32 && !IS_CRC64( decompressionInfo->check_type ) ) //lint !e506 !e774, constant value boolean, right side always evaluates to true - not modifying as it is from the external source
   {
      return XZ_OPTIONS_ERROR;
   }
#endif

   return XZ_OK;
}

/***********************************************************************************************************************

   Function Name: dec_stream_footer

   Purpose: Decode the Stream Footer field (the last 12 bytes of the .xz Stream).

   Arguments: struct xz_dec *decompressionInfo

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t dec_stream_footer( struct xz_dec *decompressionInfo )
{
   if ( !memeq( decompressionInfo->temp.buf + 10, FOOTER_MAGIC, FOOTER_MAGIC_SIZE ) )
   {
      return XZ_DATA_ERROR;
   }

   if ( xz_calcCRC32( decompressionInfo->temp.buf + 4, 6, 0 ) != get_le32( decompressionInfo->temp.buf ) )
   {
      return XZ_DATA_ERROR;
   }

   /* Validate Backward Size. Note that we never added the size of the
    * Index CRC32 field to decompressionInfo->index.size, thus we use decompressionInfo->index.size / 4
    * instead of decompressionInfo->index.size / 4 - 1. */
   if ( ( decompressionInfo->index.size >> 2 ) != get_le32( decompressionInfo->temp.buf + 4 ) )
   {
      return XZ_DATA_ERROR;
   }

   if ( decompressionInfo->temp.buf[8] != 0 || decompressionInfo->temp.buf[9] != decompressionInfo->check_type ) //lint !e641, Converting enum 'xz_check' to 'int' - not modifying as it is from the external source
   {
      return XZ_DATA_ERROR;
   }

   /* Use XZ_STREAM_END instead of XZ_OK to be more convenient for the caller. */
   return XZ_STREAM_END;
} //lint !e818, 'decompressionInfo' could be declared as pointing to const - not modifying as it is from the external source

/***********************************************************************************************************************

   Function Name: dec_block_header

   Purpose: Decode the Block Header and initialize the filter chain.

   Arguments: struct xz_dec *decompressionInfo

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t dec_block_header( struct xz_dec *decompressionInfo )
{
   xz_returnStatus_t ret;

   /* Validate the CRC32. We know that the temp buffer is at least eight bytes so this is safe. */
   decompressionInfo->temp.size -= 4;
   if ( xz_calcCRC32( decompressionInfo->temp.buf, decompressionInfo->temp.size, 0 ) !=
        get_le32( decompressionInfo->temp.buf + decompressionInfo->temp.size ) )
   {
      return XZ_DATA_ERROR;
   }

   decompressionInfo->temp.pos = 2;
   /* Catch unsupported Block Flags. We support only one or two filters in the chain,
      so we catch that with the same test. */
#ifdef XZ_DEC_BCJ
   if ( decompressionInfo->temp.buf[1] & 0x3E )
#else
   if ( decompressionInfo->temp.buf[1] & 0x3F )
#endif
   {
      return XZ_OPTIONS_ERROR;
   }

   /* Compressed Size */
   if ( decompressionInfo->temp.buf[1] & 0x40 )
   {
      if ( dec_vli( decompressionInfo, decompressionInfo->temp.buf,
                    &decompressionInfo->temp.pos, decompressionInfo->temp.size ) != XZ_STREAM_END )
      {
         return XZ_DATA_ERROR;
      }

      decompressionInfo->block_header.compressed = decompressionInfo->vli;
   }
   else
   {
      decompressionInfo->block_header.compressed = VLI_UNKNOWN;
   }

   /* Uncompressed Size */
   if ( decompressionInfo->temp.buf[1] & 0x80 )
   {
      if ( dec_vli( decompressionInfo, decompressionInfo->temp.buf,
                    &decompressionInfo->temp.pos, decompressionInfo->temp.size ) != XZ_STREAM_END )
      {
         return XZ_DATA_ERROR;
      }

      decompressionInfo->block_header.uncompressed = decompressionInfo->vli;
   }
   else
   {
      decompressionInfo->block_header.uncompressed = VLI_UNKNOWN;
   }

#ifdef XZ_DEC_BCJ
   /* If there are two filters, the first one must be a BCJ filter. */
   decompressionInfo->bcj_active = decompressionInfo->temp.buf[1] & 0x01;
   if ( decompressionInfo->bcj_active )
   {
      if ( decompressionInfo->temp.size - decompressionInfo->temp.pos < 2 )
      {
         return XZ_OPTIONS_ERROR;
      }

      ret = xz_dec_bcj_reset( decompressionInfo->bcj, decompressionInfo->temp.buf[decompressionInfo->temp.pos++] );
      if ( ret != XZ_OK )
      {
         return ret;
      }

      /* We don't support custom start offset, so Size of Properties must be zero. */
      if ( decompressionInfo->temp.buf[decompressionInfo->temp.pos++] != 0x00 )
      {
         return XZ_OPTIONS_ERROR;
      }
   }
#endif

   /* Valid Filter Flags always take at least two bytes. */
   if ( decompressionInfo->temp.size - decompressionInfo->temp.pos < 2 )
   {
      return XZ_DATA_ERROR;
   }

   /* Filter ID = LZMA2 */
   if ( decompressionInfo->temp.buf[decompressionInfo->temp.pos++] != 0x21 )
   {
      return XZ_OPTIONS_ERROR;
   }

   /* Size of Properties = 1-byte Filter Properties */
   if ( decompressionInfo->temp.buf[decompressionInfo->temp.pos++] != 0x01 )
   {
      return XZ_OPTIONS_ERROR;
   }

   /* Filter Properties contains LZMA2 dictionary size. */
   if ( decompressionInfo->temp.size - decompressionInfo->temp.pos < 1 )
   {
      return XZ_DATA_ERROR;
   }

   ret = XZ_DEC_LZMA2_reset( decompressionInfo->lzma2, decompressionInfo->temp.buf[decompressionInfo->temp.pos++] );
   if ( ret != XZ_OK )
   {
      return ret;
   }

   /* The rest must be Header Padding. */
   while ( decompressionInfo->temp.pos < decompressionInfo->temp.size )
   {
      if ( decompressionInfo->temp.buf[decompressionInfo->temp.pos++] != 0x00 )
      {
         return XZ_OPTIONS_ERROR;
      }
   }

   decompressionInfo->temp.pos = 0;
   decompressionInfo->block.compressed = 0;
   decompressionInfo->block.uncompressed = 0;

   return ret;
}

/***********************************************************************************************************************

   Function Name: dec_main

   Purpose: Decode the entire compressed input buffer.

   Arguments: struct xz_dec *decompressionInfo, xz_buffer_t *buffer

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
static xz_returnStatus_t dec_main( struct xz_dec *decompressionInfo, xz_buffer_t *buffer )
{
	xz_returnStatus_t ret;

   /* Store the start position for the case when we are in the middle of the Index field. */
   decompressionInfo->in_start = buffer->input_pos;

   while ( true ) {  //lint !e716, while(1) .. - not modifying as it is from the external source
      switch ( decompressionInfo->sequence )
      {
         case SEQ_STREAM_HEADER:
         {
            /* Stream Header is copied to decompressionInfo->temp, and then decoded from there.
             * This way if the caller gives us only little input at a time, we can still keep the
             * Stream Header decoding code simple. Similar approach is used in many places in this file. */
            if ( !fill_temp( decompressionInfo, buffer ) )
            {
               return XZ_OK;
            }

            /* If dec_stream_header() returns XZ_UNSUPPORTED_CHECK, it is still possible
             * to continue decoding if working in multi-call mode. Thus, update
             * decompressionInfo->sequence before calling dec_stream_header(). */
            decompressionInfo->sequence = SEQ_BLOCK_START;
            ret = dec_stream_header( decompressionInfo );
            if ( ret != XZ_OK )
            {
               return ret;
            }
            /* Fall through */
         }
         case SEQ_BLOCK_START: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            /* We need one byte of input to continue. */
            if ( buffer->input_pos == buffer->input_size )
            {
               return XZ_OK;
            }

            /* See if this is the beginning of the Index field. */
            if ( buffer->input[buffer->input_pos] == 0 )
            {
               decompressionInfo->in_start = buffer->input_pos++;
               decompressionInfo->sequence = SEQ_INDEX;
               break;
            }

            /* Calculate the size of the Block Header and prepare to decode it. */
            decompressionInfo->block_header.size = ((uint32_t)buffer->input[buffer->input_pos] + 1) * 4;
            decompressionInfo->temp.size = decompressionInfo->block_header.size;
            decompressionInfo->temp.pos = 0;
            decompressionInfo->sequence = SEQ_BLOCK_HEADER;
            /* Fall through */
         }
         case SEQ_BLOCK_HEADER: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            if ( !fill_temp( decompressionInfo, buffer ) )
            {
               return XZ_OK;
            }

            ret = dec_block_header( decompressionInfo );
            if ( ret != XZ_OK )
            {
               return ret;
            }

            decompressionInfo->sequence = SEQ_BLOCK_UNCOMPRESS;
            /* Fall through */
         }
         case SEQ_BLOCK_UNCOMPRESS: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            ret = dec_block( decompressionInfo, buffer );
            if ( ret != XZ_STREAM_END )
            {
               return ret;
            }

            decompressionInfo->sequence = SEQ_BLOCK_PADDING;
            /* Fall through */
         }
         case SEQ_BLOCK_PADDING: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            /* Size of Compressed Data + Block Padding must be a multiple of four.
             * We don't need decompressionInfo->block.compressed for anything else
             * anymore, so we use it here to test the size of the Block Padding field. */
            while ( decompressionInfo->block.compressed & 3) {
               if ( buffer->input_pos == buffer->input_size )
               {
                  return XZ_OK;
               }

               if ( buffer->input[buffer->input_pos++] != 0 )
               {
                  return XZ_DATA_ERROR;
               }

               ++decompressionInfo->block.compressed;
            }

            decompressionInfo->sequence = SEQ_BLOCK_CHECK;
            /* Fall through */
         }
         case SEQ_BLOCK_CHECK: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            if ( decompressionInfo->check_type == XZ_CHECK_CRC32 )
            {
               ret = crc_validate( decompressionInfo, buffer, 32 );
               if ( ret != XZ_STREAM_END )
               {
                  return ret;
               }
            }
            else if ( IS_CRC64( decompressionInfo->check_type ) ) //lint !e506 !e774, boolean and always true - not modifying as it is from the external source
            {
               ret = crc_validate( decompressionInfo, buffer, 64 );
               if ( ret != XZ_STREAM_END )
               {
                  return ret;
               }
            }
#ifdef XZ_DEC_ANY_CHECK
            else if ( !check_skip( decompressionInfo, buffer ) )
            {
               return XZ_OK;
            }
#endif
            decompressionInfo->sequence = SEQ_BLOCK_START;
            break;
         }
         case SEQ_INDEX:
         {
            ret = dec_index( decompressionInfo, buffer );
            if ( ret != XZ_STREAM_END )
            {
               return ret;
            }

            decompressionInfo->sequence = SEQ_INDEX_PADDING;
            /* Fall through */
         }
         case SEQ_INDEX_PADDING: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            while ( ( decompressionInfo->index.size + ( buffer->input_pos - decompressionInfo->in_start ) ) & 3 )
            {
               if ( buffer->input_pos == buffer->input_size )
               {
                  index_update(decompressionInfo, buffer);
                  return XZ_OK;
               }

               if ( buffer->input[buffer->input_pos++] != 0 )
               {
                  return XZ_DATA_ERROR;
               }
            }

            /* Finish the CRC32 value and Index size. */
            index_update( decompressionInfo, buffer );

            /* Compare the hashes to validate the Index field. */
            if ( !memeq( &decompressionInfo->block.hash, &decompressionInfo->index.hash,
                         sizeof( decompressionInfo->block.hash ) ) )
            {
               return XZ_DATA_ERROR;
            }

            decompressionInfo->sequence = SEQ_INDEX_CRC32;
            /* Fall through */
         }
         case SEQ_INDEX_CRC32: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            ret = crc_validate( decompressionInfo, buffer, 32 );
            if ( ret != XZ_STREAM_END )
            {
               return ret;
            }

            decompressionInfo->temp.size = STREAM_HEADER_SIZE;
            decompressionInfo->sequence = SEQ_STREAM_FOOTER;
            /* Fall through */
         }
         case SEQ_STREAM_FOOTER: //lint !e616 !e825, control flows into case/default - not modifying as it is from the external source
         {
            if ( !fill_temp( decompressionInfo, buffer ) )
            {
               return XZ_OK;
            }

            return dec_stream_footer( decompressionInfo );
         }
      }
   }

   /* Never reached */
}

/***********************************************************************************************************************

   Function Name: XZ_DEC_STREAM_run

   Purpose: It is a wrapper for dec_main() to handle some special cases in multi-call and single-call decoding.

   Arguments: struct xz_dec *decompressionInfo, xz_buffer_t *buffer

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes: In multi-call mode, we must return XZ_BUF_ERROR when it seems clear that we
   are not going to make any progress anymore. This is to prevent the caller
   from calling us infinitely when the input file is truncated or otherwise
   corrupt. Since zlib-style API allows that the caller fills the input buffer
   only when the decoder doesn't produce any new output, we have to be careful
   to avoid returning XZ_BUF_ERROR too easily: XZ_BUF_ERROR is returned only
   after the second consecutive call to XZ_DEC_STREAM_run() that makes no progress.

   In single-call mode, if we couldn't decode everything and no error
   occurred, either the input is truncated or the output buffer is too small.
   Since we know that the last input byte never produces any output, we know
   that if all the input was consumed and decoding wasn't finished, the file
   must be corrupt. Otherwise the output buffer has to be too small or the
   file is corrupt in a way that decoding it produces too big output.

   If single-call decoding fails, we reset buffer->input_pos and buffer->output_pos back to
   their original values. This is because with some filter chains there won't
   be any valid uncompressed data in the output buffer unless the decoding
   actually succeeds (that's the price to pay of using the output buffer as
   the workspace).

**********************************************************************************************************************/
XZ_EXTERN xz_returnStatus_t XZ_DEC_STREAM_run( struct xz_dec *decompressionInfo, xz_buffer_t *buffer )
{ //lint !e508, extern used with definition - not modifying as it is from the external source
   size_t in_start;
   size_t out_start;
   xz_returnStatus_t ret;

   if ( DEC_IS_SINGLE( decompressionInfo->mode ) )
   {
      XZ_DEC_STREAM_reset( decompressionInfo );
   }

   in_start = buffer->input_pos;
   out_start = buffer->output_pos;
   ret = dec_main( decompressionInfo, buffer );

   if ( DEC_IS_SINGLE( decompressionInfo->mode ) )
   {
		if ( ret == XZ_OK )
      {
         ret = buffer->input_pos == buffer->input_size ? XZ_DATA_ERROR : XZ_BUF_ERROR;
      }

		if ( ret != XZ_STREAM_END )
      {
         buffer->input_pos = in_start;
         buffer->output_pos = out_start;
      }
   }
   else if ( ret == XZ_OK && in_start == buffer->input_pos && out_start == buffer->output_pos )
   {
      if ( decompressionInfo->allow_buf_error )
      {
         ret = XZ_BUF_ERROR;
      }

      decompressionInfo->allow_buf_error = true;
   }
   else {
      decompressionInfo->allow_buf_error = false;
   }

   return ret;
}

/***********************************************************************************************************************

   Function Name: XZ_DEC_STREAM_init

   Purpose: Allocate and initialize a XZ decoder state
            @mode:       Operation mode
            @dict_max:   Maximum size of the LZMA2 dictionary (history buffer) for
                         multi-call decoding. This is ignored in single-call mode
                         (mode == XZ_SINGLE). LZMA2 dictionary is always 2^n bytes
                         or 2^n + 2^(n-1) bytes (the latter sizes are less common
                         in practice), so other values for dict_max don't make sense.
                         In the kernel, dictionary sizes of 64 KiB, 128 KiB, 256 KiB,
                         512 KiB, and 1 MiB are probably the only reasonable values,
                         except for kernel and initramfs images where a bigger
                         dictionary can be fine and useful.

   Arguments: xz_mode_t mode, uint32_t dict_max

   Returns: xz_returnStatus_t - status of the decode

   Side Effects: Nothing

   Reentrant Code: No

   Notes: Single-call mode (XZ_SINGLE): XZ_DEC_STREAM_run() decodes the whole stream at
   once. The caller must provide enough output space or the decoding will
   fail. The output space is used as the dictionary buffer, which is why
   there is no need to allocate the dictionary as part of the decoder's
   internal state.

   Because the output buffer is used as the workspace, streams encoded using
   a big dictionary are not a problem in single-call mode. It is enough that
   the output buffer is big enough to hold the actual uncompressed data; it
   can be smaller than the dictionary size stored in the stream headers.

   Multi-call mode with preallocated dictionary (XZ_PREALLOC): dict_max bytes
   of memory is preallocated for the LZMA2 dictionary. This way there is no
   risk that XZ_DEC_STREAM_run() could run out of memory, since XZ_DEC_STREAM_run() will
   never allocate any memory. Instead, if the preallocated dictionary is too
   small for decoding the given input stream, XZ_DEC_STREAM_run() will return
   XZ_MEMLIMIT_ERROR. Thus, it is important to know what kind of data will be
   decoded to avoid allocating excessive amount of memory for the dictionary.

   Multi-call mode with dynamically allocated dictionary (XZ_DYNALLOC):
   dict_max specifies the maximum allowed dictionary size that XZ_DEC_STREAM_run()
   may allocate once it has parsed the dictionary size from the stream
   headers. This way excessive allocations can be avoided while still
   limiting the maximum memory usage to a sane value to prevent running the
   system out of memory when decompressing streams from untrusted sources.

   On success, XZ_DEC_STREAM_init() returns a pointer to struct xz_dec, which is
   ready to be used with XZ_DEC_STREAM_run(). If memory allocation fails,
   XZ_DEC_STREAM_init() returns NULL.

**********************************************************************************************************************/
XZ_EXTERN struct xz_dec *XZ_DEC_STREAM_init( xz_mode_t mode, uint32_t dict_max )
{ //lint !e508, extern used with definition - not modifying as it is from the external source
   struct xz_dec *decompressionInfo = &xzDecompression; /* kmalloc( sizeof( *decompressionInfo ), GFP_KERNEL ); // Code as in open source repo */
   memset( decompressionInfo, 0, sizeof( decompressionInfo ) );
   if ( decompressionInfo != NULL ) //lint !e774, 'if' always evaluates to True - not modifying as it is from the external source
   {
      decompressionInfo->mode = mode;
#ifdef XZ_DEC_BCJ
      decompressionInfo->bcj = xz_dec_bcj_create( DEC_IS_SINGLE( mode ) );
      if ( decompressionInfo->bcj == NULL )
      {
         /*  kfree( decompressionInfo ); // Code as in open source repo */
      }
#endif
      if ( decompressionInfo != NULL ) //lint !e774, 'if' always evaluates to True - not modifying as it is from the external source
      {
         decompressionInfo->lzma2 = XZ_DEC_LZMA2_create( mode, dict_max );
         if ( decompressionInfo->lzma2 == NULL )
         {
#ifdef XZ_DEC_BCJ
            xz_dec_bcj_end( decompressionInfo->bcj );
#endif
            /* kfree( decompressionInfo ); // Code as in open source repo */
         }
         else
         {
            XZ_DEC_STREAM_reset( decompressionInfo );
         }
      }
   }

   return decompressionInfo;
}

/***********************************************************************************************************************

   Function Name: XZ_DEC_STREAM_reset

   Purpose: Reset an already allocated decoder state.
            This function can be used to reset the multi-call decoder state without
            freeing and reallocating memory with XZ_DEC_STREAM_end() and XZ_DEC_STREAM_init().
            In single-call mode, XZ_DEC_STREAM_reset() is always called in the beginning of
            XZ_DEC_STREAM_run(). Thus, explicit call to XZ_DEC_STREAM_reset() is useful only in
            multi-call mode.

   Arguments: struct xz_dec *decompressionInfo

   Returns: Nothing

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
XZ_EXTERN void XZ_DEC_STREAM_reset( struct xz_dec *decompressionInfo )
{ //lint !e508, extern used with definition - not modifying as it is from the external source
   decompressionInfo->sequence = SEQ_STREAM_HEADER;
   decompressionInfo->allow_buf_error = false;
   decompressionInfo->pos = 0;
   decompressionInfo->crc = 0;
   memzero( &decompressionInfo->block, sizeof( decompressionInfo->block ) );
   memzero( &decompressionInfo->index, sizeof( decompressionInfo->index ) );
   decompressionInfo->temp.pos = 0;
   decompressionInfo->temp.size = STREAM_HEADER_SIZE;
}

/***********************************************************************************************************************

   Function Name: XZ_DEC_STREAM_end

   Purpose: Free the memory allocated for the decoder state.

   Arguments: struct xz_dec *decompressionInfo

   Returns: Nothing

   Side Effects: Nothing

   Reentrant Code: No

   Notes:

**********************************************************************************************************************/
XZ_EXTERN void XZ_DEC_STREAM_end( struct xz_dec *decompressionInfo )
{ //lint !e508, extern used with definition - not modifying as it is from the external source
   if ( decompressionInfo != NULL )
   {
      XZ_DEC_LZMA2_end( decompressionInfo->lzma2 );
#ifdef XZ_DEC_BCJ
      xz_dec_bcj_end( decompressionInfo->bcj );
#endif
      memset( decompressionInfo, 0, sizeof( decompressionInfo ) );  /* kfree( decompressionInfo ); // Code as in open source repo */
   }
}
