/***********************************************************************************************************************
 *
 * Filename: xz_stream.h
 *
 * Contents: Definitions for handling the .xz file format
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

#ifndef XZ_STREAM_H
#define XZ_STREAM_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define STREAM_HEADER_SIZE      12
#define HEADER_MAGIC            "\3757zXZ"
#define HEADER_MAGIC_SIZE       6
#define FOOTER_MAGIC            "YZ"
#define FOOTER_MAGIC_SIZE       2
#define VLI_MAX                 ( ( vli_type ) - 1 / 2 )
#define VLI_UNKNOWN             ( ( vli_type ) - 1 )

/* Maximum encoded size of a VLI */
#define VLI_BYTES_MAX           ( sizeof( vli_type ) * 8 / 7 )

/* Maximum possible Check ID */
#define XZ_CHECK_MAX 15

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* Variable-length integer can hold a 63-bit unsigned integer or a special
 * value indicating that the value is unknown.
 *
 * Experimental: vli_type can be defined to uint32_t to save a few bytes
 * in code size (no effect on speed). Doing so limits the uncompressed and
 * compressed size of the file to less than 256 MiB and may also weaken
 * error detection slightly. */
typedef uint64_t vli_type;

/* Integrity Check types */
enum xz_check
{
   XZ_CHECK_NONE   = 0,
   XZ_CHECK_CRC32  = 1,
   XZ_CHECK_CRC64  = 4,
   XZ_CHECK_SHA256 = 10
};

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#endif /* XZ_STREAM_H */
