/***********************************************************************************************************************
 *
 * Filename: xzminidec.h
 *
 * Contents: Contains prototypes and definitions for xz decoding
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

#ifndef XZ_H
#define XZ_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stddef.h>
#include <stdint.h>

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#ifndef XZ_EXTERN
#define XZ_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/**
 * enum xz_mode_t - Operation mode
 *
 * @XZ_SINGLE:              Single-call mode. This uses less RAM than
 *                          multi-call modes, because the LZMA2
 *                          dictionary doesn't need to be allocated as
 *                          part of the decoder state. All required data
 *                          structures are allocated at initialization,
 *                          so XZ_DEC_STREAM_run() cannot return XZ_MEM_ERROR.
 * @XZ_PREALLOC:            Multi-call mode with preallocated LZMA2
 *                          dictionary buffer. All data structures are
 *                          allocated at initialization, so XZ_DEC_STREAM_run()
 *                          cannot return XZ_MEM_ERROR.
 * @XZ_DYNALLOC:            Multi-call mode. The LZMA2 dictionary is
 *                          allocated once the required size has been
 *                          parsed from the stream headers. If the
 *                          allocation fails, XZ_DEC_STREAM_run() will return
 *                          XZ_MEM_ERROR.
 *
 * It is possible to enable support only for a subset of the above
 * modes at compile time by defining XZ_DEC_SINGLE, XZ_DEC_PREALLOC,
 * or XZ_DEC_DYNALLOC. The xz_dec kernel module is always compiled
 * with support for all operation modes, but the preboot code may
 * be built with fewer features to minimize code size.
 */
typedef enum {
	XZ_SINGLE,
	XZ_PREALLOC,
	XZ_DYNALLOC
} xz_mode_t;

/**
 * enum xz_returnStatus_t - Return codes
 * @XZ_OK:                  Everything is OK so far. More input or more
 *                          output space is required to continue. This
 *                          return code is possible only in multi-call mode
 *                          (XZ_PREALLOC or XZ_DYNALLOC).
 * @XZ_STREAM_END:          Operation finished successfully.
 * @XZ_UNSUPPORTED_CHECK:   Integrity check type is not supported. Decoding
 *                          is still possible in multi-call mode by simply
 *                          calling XZ_DEC_STREAM_run() again.
 *                          Note that this return value is used only if
 *                          XZ_DEC_ANY_CHECK was defined at build time,
 *                          which is not used in the kernel. Unsupported
 *                          check types return XZ_OPTIONS_ERROR if
 *                          XZ_DEC_ANY_CHECK was not defined at build time.
 * @XZ_MEM_ERROR:           Allocating memory failed. This return code is
 *                          possible only if the decoder was initialized
 *                          with XZ_DYNALLOC. The amount of memory that was
 *                          tried to be allocated was no more than the
 *                          dict_max argument given to XZ_DEC_STREAM_init().
 * @XZ_MEMLIMIT_ERROR:      A bigger LZMA2 dictionary would be needed than
 *                          allowed by the dict_max argument given to
 *                          XZ_DEC_STREAM_init(). This return value is possible
 *                          only in multi-call mode (XZ_PREALLOC or
 *                          XZ_DYNALLOC); the single-call mode (XZ_SINGLE)
 *                          ignores the dict_max argument.
 * @XZ_FORMAT_ERROR:        File format was not recognized (wrong magic
 *                          bytes).
 * @XZ_OPTIONS_ERROR:       This implementation doesn't support the requested
 *                          compression options. In the decoder this means
 *                          that the header CRC32 matches, but the header
 *                          itself specifies something that we don't support.
 * @XZ_DATA_ERROR:          Compressed data is corrupt.
 * @XZ_BUF_ERROR:           Cannot make any progress. Details are slightly
 *                          different between multi-call and single-call
 *                          mode; more information below.
 *
 * In multi-call mode, XZ_BUF_ERROR is returned when two consecutive calls
 * to XZ code cannot consume any input and cannot produce any new output.
 * This happens when there is no new input available, or the output buffer
 * is full while at least one output byte is still pending. Assuming your
 * code is not buggy, you can get this error only when decoding a compressed
 * stream that is truncated or otherwise corrupt.
 *
 * In single-call mode, XZ_BUF_ERROR is returned only when the output buffer
 * is too small or the compressed input is corrupt in a way that makes the
 * decoder produce more output than the caller expected. When it is
 * (relatively) clear that the compressed input is truncated, XZ_DATA_ERROR
 * is used instead of XZ_BUF_ERROR.
 */
typedef enum {
	XZ_OK,
	XZ_STREAM_END,
	XZ_UNSUPPORTED_CHECK,
	XZ_MEM_ERROR,
	XZ_MEMLIMIT_ERROR,
	XZ_FORMAT_ERROR,
	XZ_OPTIONS_ERROR,
	XZ_DATA_ERROR,
	XZ_BUF_ERROR
} xz_returnStatus_t;

/**
 * xz_buffer_t - Passing input and output buffers to XZ code
 * @input:         Beginning of the input buffer. This may be NULL if and only
 *                 if input_pos is equal to input_size.
 * @input_pos:     Current position in the input buffer. This must not exceed
 *                 input_size.
 * @input_size:    Size of the input buffer
 * @output:        Beginning of the output buffer. This may be NULL if and only
 *                 if output_pos is equal to output_size.
 * @output_pos:    Current position in the output buffer. This must not exceed
 *                 output_size.
 * @output_size:   Size of the output buffer
 *
 * Only the contents of the output buffer from out[output_pos] onward, and
 * the variables input_pos and output_pos are modified by the XZ code.
 */
typedef struct {
   const uint8_t *input;
   size_t input_pos;
   size_t input_size;

   uint8_t *output;
   size_t output_pos;
   size_t output_size;
} xz_buffer_t;

/* struct xz_dec - Opaque type to hold the XZ decoder state */
struct xz_dec;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

XZ_EXTERN struct xz_dec        *XZ_DEC_STREAM_init( xz_mode_t mode, uint32_t dict_max );
XZ_EXTERN xz_returnStatus_t    XZ_DEC_STREAM_run( struct xz_dec *decompressionInfo, xz_buffer_t *buffer );
XZ_EXTERN void                 XZ_DEC_STREAM_reset( struct xz_dec *decompressionInfo );
XZ_EXTERN void                 XZ_DEC_STREAM_end( struct xz_dec *decompressionInfo );

/* If required - The respective functionalities will be added in a separate file */
#if XZ_INTERNAL_CRC64
XZ_EXTERN void                 xz_crc64_init( void );
XZ_EXTERN uint64_t             xz_crc64( const uint8_t *buf, size_t size, uint64_t crc );
#endif /* XZ_INTERNAL_CRC64 */

#endif /* XZ_H */
