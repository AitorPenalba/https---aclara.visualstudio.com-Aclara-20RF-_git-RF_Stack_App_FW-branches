/***********************************************************************************************************************
 *
 * Filename: pack.h
 *
 * Contents: Fills a buffer with bits
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 * $Log$ kad Created 060605
 *
 **********************************************************************************************************************/

#ifndef pack_H
#define pack_H
#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

#define PACK_LITTLE_ENDIAN (true)

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint8_t *pData;        /* Data Pointer */
   uint16_t uiBitCount;   /* Bit counter */
}tPack;                 /* Structer for the pack functions to hold the data pointer and bit counter */

typedef struct{
   uint16_t bitNo;        /* Current bit number        */
   uint8_t *pArray;       /* Pointer to the data array */
}pack_t;                /* Structure for the pack/unpack functions to hold the data pointer and bit counter */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* *******************************************************
 * Some new pack and unpack function
 * bah - I prefer these, they should be type safe, and so they don't assume packed structures.
 */

/* The destination bits will be in BIG ENDIAN format */
uint16_t PACK_uint8_2bits(  uint8_t  const *pSrc, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset);
uint16_t PACK_uint16_2bits( uint16_t const *pSrc, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset);
uint16_t PACK_uint32_2bits( uint32_t const *pSrc, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset);
uint16_t PACK_uint64_2bits( uint64_t const *pSrc, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset);
uint16_t PACK_ucArray_2bits(uint8_t  const *pSrc, int8_t  numBits, uint8_t *pDst, uint16_t dstOffset);
uint16_t PACK_addr(         uint8_t  const *pSrc, uint8_t numBytes, uint8_t *pDst, uint16_t dstOffset);

/* The source bits are assumed to be in BIG ENDIAN format */
uint32_t UNPACK_bits(         const uint8_t *pSrc, uint16_t *pSrcOffset,             uint8_t numBits);
uint8_t  UNPACK_bits2_uint8(  const uint8_t *pSrc, uint16_t *pSrcOffset,             uint8_t numBits);
uint16_t UNPACK_bits2_uint16( const uint8_t *pSrc, uint16_t *pSrcOffset,             uint8_t numBits);
uint32_t UNPACK_bits2_uint32( const uint8_t *pSrc, uint16_t *pSrcOffset,             uint8_t numBits);
uint64_t UNPACK_bits2_uint64( const uint8_t *pSrc, uint16_t *pSrcOffset,             uint8_t numBits);
uint16_t UNPACK_bits2_ucArray(const uint8_t* pSrc, uint16_t srcOffset, uint8_t*  pDst, uint16_t numBits);

/* The replace the non-reentrant versions */
void PACK_init( uint8_t *pSrc, pack_t *pPackCfg );
void PACK_Byte( uint8_t value, pack_t *pack);
void PACK_Buffer  (int16_t numBits, uint8_t  *pData, pack_t *pack);
void UNPACK_init( uint8_t *pSrc, pack_t *pPackCfg );
void UNPACK_inc(uint16_t cnt, pack_t *pUnpackCfg);
void UNPACK_Buffer(int8_t bitCount,   uint8_t *pResult, pack_t *pPack);
void UNPACK_BufferArray( uint8_t *pArray, int8_t const *pBitCount, uint8_t *pResult, pack_t *pUnpackCfg );
void UNPACK_BufferArrayNoInit(int8_t const *,            uint8_t *,        pack_t *pPack);
void UNPACK_addr(uint8_t const *pSrc, uint16_t *pSrcOffset, uint8_t numBytes, uint8_t *address);

#ifdef ERROR_CODES_H_
returnStatus_t PACK_hton(uint8_t *pSrc, int8_t const *pDecoder, bool bChangeEndian);
returnStatus_t PACK_ntoh(uint8_t *pSrc, int8_t const *pDecoder, bool bChangeEndian);
#endif /* ERROR_CODES_H_ */

#ifdef __cplusplus
}
#endif
#endif /* PACK_H_ */
