/***********************************************************************************************************************
 *
 * Filename: xz_private.h
 *
 * Contents: Private includes and definitions
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

#ifndef XZ_PRIVATE_H
#define XZ_PRIVATE_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "xz_config.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* If no specific decoding mode is requested, enable support for all modes. */
#if !defined(XZ_DEC_SINGLE) && !defined(XZ_DEC_PREALLOC) && !defined(XZ_DEC_DYNALLOC)
#define XZ_DEC_SINGLE
#define XZ_DEC_PREALLOC
#define XZ_DEC_DYNALLOC
#endif

/* The DEC_IS_foo(mode) macros are used in "if" statements. If only some
 * of the supported modes are enabled, these macros will evaluate to true or
 * false at compile time and thus allow the compiler to omit unneeded code. */
#ifdef XZ_DEC_SINGLE
#define DEC_IS_SINGLE( mode ) ( ( mode ) == XZ_SINGLE )
#else
#define DEC_IS_SINGLE( mode ) ( false )
#endif

#ifdef XZ_DEC_PREALLOC
#define DEC_IS_PREALLOC( mode ) ( ( mode ) == XZ_PREALLOC )
#else
#define DEC_IS_PREALLOC( mode ) ( false )
#endif

#ifdef XZ_DEC_DYNALLOC
#define DEC_IS_DYNALLOC( mode ) ( ( mode ) == XZ_DYNALLOC )
#else
#define DEC_IS_DYNALLOC( mode ) ( false )
#endif

#if !defined( XZ_DEC_SINGLE )
#define DEC_IS_MULTI( mode ) ( true )
#elif defined( XZ_DEC_PREALLOC ) || defined( XZ_DEC_DYNALLOC )
#define DEC_IS_MULTI( mode ) ( ( mode ) != XZ_SINGLE )
#else
#define DEC_IS_MULTI( mode ) ( false )
#endif

/* If any of the BCJ filter decoders are wanted, define XZ_DEC_BCJ.
 * XZ_DEC_BCJ is used to enable generic support for BCJ decoders. */
#ifndef XZ_DEC_BCJ
#if defined( XZ_DEC_X86 ) || defined( XZ_DEC_POWERPC ) \
   || defined( XZ_DEC_IA64 ) || defined( XZ_DEC_ARM ) \
   || defined( XZ_DEC_ARM ) || defined( XZ_DEC_ARMTHUMB ) \
   || defined( XZ_DEC_SPARC )
#define XZ_DEC_BCJ
#endif
#endif

#ifdef XZ_DEC_BCJ
/* Free the memory allocated for the BCJ filters. */
#define xz_dec_bcj_end( s ) kfree( s )
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

XZ_EXTERN struct xz_dec_lzma2    *XZ_DEC_LZMA2_create( xz_mode_t mode, uint32_t dict_max);
XZ_EXTERN xz_returnStatus_t      XZ_DEC_LZMA2_reset( struct xz_dec_lzma2 *s, uint8_t props );
XZ_EXTERN xz_returnStatus_t      XZ_DEC_LZMA2_run( struct xz_dec_lzma2 *s, xz_buffer_t *b);
XZ_EXTERN void                   XZ_DEC_LZMA2_end( struct xz_dec_lzma2 *s );

#ifdef XZ_DEC_BCJ
/* Allocate memory for BCJ decoders. xz_dec_bcj_reset() must be used before
 * calling xz_dec_bcj_run(). */
XZ_EXTERN struct xz_dec_bcj *xz_dec_bcj_create( bool single_call );

/* Decode the Filter ID of a BCJ filter. This implementation doesn't
 * support custom start offsets, so no decoding of Filter Properties
 * is needed. Returns XZ_OK if the given Filter ID is supported.
 * Otherwise XZ_OPTIONS_ERROR is returned. */
XZ_EXTERN xz_returnStatus_t xz_dec_bcj_reset( struct xz_dec_bcj *s, uint8_t id );

/* Decode raw BCJ + LZMA2 stream. This must be used only if there actually is
 * a BCJ filter in the chain. If the chain has only LZMA2, XZ_DEC_LZMA2_run()
 * must be called directly. */
XZ_EXTERN xz_returnStatus_t xz_dec_bcj_run( struct xz_dec_bcj *s,
                                            struct xz_dec_lzma2 *lzma2,
                                            xz_buffer_t *b);
#endif

#endif /* XZ_PRIVATE_H */
