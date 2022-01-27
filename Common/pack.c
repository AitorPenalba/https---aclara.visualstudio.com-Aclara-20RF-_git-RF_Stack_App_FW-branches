// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************
 *
 * Filename:   Pack.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 - 2016 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ KAD Created 062804
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - Initial Release
 * v0.2 - Changed the unpack function to clear the storage location directly
 *        before writing to it.  This was done at the initial stage, but
 *        was more effient and allowed the function to be used in more places.
 *        The pack function now increments the pointer instead of an index.
 * v0.3 - Init the 1st byte to 0 in the unpack function
 * v0.4 - Fixed clearing the memory on the unpack function
 * v0.5 - Fixed packing and unpacking of multi-byte values that do not byte-swap
 * v0.6 - Fixed storage of little endian when not on a byte-boundary.  In doing so
 *        the execution speed as been slightly increased.
 * v0.7 - Fixed big endian
 * v0.8 - Fixed the pack function
 * v0.9 - Changed the unpack function to handle more than 255 bits (Needed for TWACS 20)
 * v0.10 - Added a function to increment the unpack buffer.  Added for TWACS20 speed
 * v0.11 - Added comments
 * v0.12 - Optimized all MOD operations by replacing them with "& MOD8"
 *          Added 3 new #defines to get rid of magic numbers.
 *          Added braces where needed.
 ******************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Psect Definitions">
/* ****************************************************************************************************************** */
/* PSECTS */

#ifdef USE_MAPPING
#pragma psect text=utiltxt
#pragma psect const=utilcnst
#pragma psect mconst=utilmconst
#pragma psect nearbss=utilbss
#pragma psect bss=utilfbss
#endif
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <limits.h> /* only for CHAR_BIT */
#include <string.h>
#include "error_codes.h"
#include "portable_freescale.h" /* only for PROCESSOR_LITTLE_ENDIAN */
#include "pack.h"
#include "byteswap.h"
#include "portable_aclara.h"
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define MOD8            ((uint8_t)0x07)
#define INITIAL_MASK    ((uint8_t)0x80)
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Variable Definitions">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Function Prototypes">
/* ********************************************************************************************************************/
/* FUNCTION PROTOTYPES */

static uint8_t pack_bits  ( const uint8_t *pSrc, uint8_t srcSize,   uint8_t *pDst, uint16_t dstOffset, uint8_t numBits );
static uint8_t unpack_bits( const uint8_t *pSrc, uint16_t srcOffset, uint8_t *pDst, uint8_t dstSize,   uint8_t numBits );
static uint8_t copy_bits  ( const uint8_t *pSrc, uint16_t srcOffset, uint8_t *pDst, uint8_t dstOffset, uint16_t numBits );
// </editor-fold>

/* ********************************************************************************************************************/
/* FUNCTION DEFINITIONS */

#define MOD_8(a)   ((a) & 0x07)  /*< Macro to do a modulo 8 */
#define DIV_8(a)   ((a) >> 3)    /*< Macro to divide by 8. This should work on any data size 8,16,32 */

/*!
   \f int PACK_uint8_2bits()
   \brief Encodes a uint8_t value to a bit array.

   \param [in] pData      Pointer to the value to pack.
   \param [in] numBits    The number of bits to pack.
   \param [in] pDst       Pointer to the destination array.
   \param [in] dstOffset  Offset into the destination array in bits.

   \retval    dstOffset  The new destination offset.

   \sa { pack_bits }

*/
uint16_t PACK_uint8_2bits( uint8_t const *pData, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset )
{
   numBits = pack_bits( ( uint8_t * )pData, sizeof( *pData ), pDst, dstOffset, numBits );
   return ( dstOffset + numBits );
}

/*!
   \f int PACK_uint16_2bits()
   \brief Encodes a uint16_t value to packed bits.

   \param [in] pData      Pointer to the value to pack.
   \param [in] numBits    The number of bits to pack.
   \param [in] pDst       Pointer to the destination array.
   \param [in] dstOffset  Offset into the destination array in bits.

   \retval    dstOffset  The new destination offset.

   \sa { pack_bits }

*/
uint16_t PACK_uint16_2bits( uint16_t const *pData, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset )
{
   numBits = pack_bits( ( uint8_t * )pData, sizeof( *pData ), pDst, dstOffset, numBits );
   return ( dstOffset + numBits );
}

/*!
   \f int PACK_uint32_2bits()
   \brief Encodes a uint32_t value to packed bits.

   \param [in] pData      Pointer to the value to pack.
   \param [in] numBits    The number of bits to pack.
   \param [in] pDst       Pointer to the destination array.
   \param [in] dstOffset  Offset into the destination array in bits.

   \retval    dstOffset  The new destination offset.

   \sa { pack_bits }

*/

uint16_t PACK_uint32_2bits( uint32_t const *pData, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset )
{
   numBits = pack_bits( ( uint8_t * )pData, sizeof( *pData ), pDst, dstOffset, numBits );
   return ( dstOffset + numBits );
}

/*!
   \f int PACK_uint64_2bits()
   \brief Encodes a uint64_t value to packed bits.

   \param [in] pData      Pointer to the value to pack.
   \param [in] numBits    The number of bits to pack.
   \param [in] pDst       Pointer to the destination array.

   \param [in] dstOffset  Offset into the destination array in bits.

   \retval    dstOffset  The new destination offset.
*/
uint16_t PACK_uint64_2bits( uint64_t const *pData, uint8_t numBits, uint8_t *pDst, uint16_t dstOffset )
{
   numBits = pack_bits( ( uint8_t * )pData, sizeof( *pData ), pDst, dstOffset, numBits );
   return ( dstOffset + numBits );
}

/*!
   \f int PACK_ucArray_2bits()
   \brief Encodes an array of uint8_t's to packed bit array.
        The byte order is preserved!!!

   \param [in] pData      Pointer to the value to pack.
   \param [in] numBits    The number of bits to pack.
   \param [in] pDst       Pointer to the destination array.
   \param [in] dstOffset  Offset into the destination array in bits.

   \retval    numBits    The number of bits actually packed.

*/
// need to improve this!!!
#if 0
uint16_t PACK_ucArray_2bits( uint8_t const *pSrc, int8_t numBits, uint8_t *pDst, uint16_t dstOffset )
{
#if 1
   // todo: need to fix this
   // For now alway move 8 bits, so the data is left justified!
   // Return the bit counter
   int bitsRemaining = numBits;
   int bitsMoved;
   do {
      bitsMoved = PACK_uint8_2bits( pSrc, 8, pDst, dstOffset );
      dstOffset     += bitsMoved;
      bitsRemaining -= bitsMoved;
      pSrc++;
   } while (bitsRemaining > 0);
   return ( dstOffset + numBits );
// return ( numBits );
#else
   int bitsRemaining = numBits;
   int bitsMoved;
   int totalBitsMoved = 0;
   do {
      bitsMoved = PACK_uint8_2bits( pSrc, numBits, pDst, dstOffset );
      bitsRemaining -= bitsMoved;
      dstOffset     += bitsMoved;
      pSrc++;
   } while (bitsRemaining > 0 );
#endif
}
#endif

#if 0

/*!
   \f int UNPACK_bits2_uint8()
   \brief Decodes a uint8_t value from a bit array.

   \param[in] pSrc       Pointer to the source.
   \param[in] srcOffset  Offset into the source in bits.
   \param[in] data       Pointer to the destination value.
   \param[in] numBits    The number of bits to move.

   \retval    srcOffset  The new source offset.
*/

uint16_t UNPACK_bits2_uint8( const uint8_t *pSrc, uint16_t srcOffset, uint8_t *pDst, uint8_t numBits )
{
   *pDst = 0u;
   numBits = unpack_bits( pSrc, srcOffset, ( uint8_t * )pDst, sizeof( *pDst ), numBits );
   return ( srcOffset + numBits );
}

/*!
   \f int UNPACK_bits2_uint16()
   \brief Decodes a uint16_t value from a bit array.

   \param[in] pSrc       Pointer to the source.
   \param[in] dstOffset  Offset into the source in bits.
   \param[in] pDst       Pointer to the destination value.
   \param[in] numBits    The number of bits to move.

   \retval    srcOffset  The new source offset.
*/

uint16_t UNPACK_bits2_uint16( const uint8_t *pSrc, uint16_t srcOffset, uint16_t *pDst, uint8_t numBits )
{
// *pDst = 0u;
   memset(pDst, '\0', sizeof(uint16_t));
   numBits = unpack_bits( pSrc, srcOffset, ( uint8_t * )pDst, sizeof( *pDst ), numBits );
   return ( srcOffset + numBits );
}

/*!
   \f int UNPACK_bits2_uint32()
   \brief Decodes a uint32_t value from a bit array.

   \param[in] pSrc       Pointer to the source.
   \param[in] dstOffset  Offset into the source in bits.
   \param[in] pDst       Pointer to the destination value.
   \param[in] numBits    The number of bits to move.

   \retval    srcOffset  The new source offset.
*/

uint16_t UNPACK_bits2_uint32( const uint8_t *pSrc, uint16_t srcOffset, uint32_t *pDst, uint8_t numBits )
{
// *pDst = 0;   /* this can cause an error if memry is not properly aligned!!! */
   memset(pDst, '\0', sizeof(uint32_t));
   numBits = unpack_bits( pSrc, srcOffset, ( uint8_t * )pDst, sizeof( *pDst ), numBits );
   return ( srcOffset + numBits );
}

/*!
   \f int UNPACK_bits2_uint64()
   \brief Decodes a uint64_t value from a bit array.

   \param[in] pSrc       Pointer to the source.
   \param[in] dstOffset  Offset into the source in bits.
   \param[in] pDst       Pointer to the destination value.
   \param[in] numBits    The number of bits to move.

   \retval    srcOffset  The new source offset.
*/
uint16_t UNPACK_bits2_uint64( const uint8_t *pSrc, uint16_t srcOffset, uint64_t *pDst, uint8_t numBits )
{
// *pDst = 0;
   memset(pDst, '\0', sizeof(uint64_t));
   numBits = unpack_bits( pSrc, srcOffset, ( uint8_t * )pDst, sizeof( *pDst ), numBits );
   return ( srcOffset + numBits );
}

#endif

/*!
   \f int UNPACK_bits2_ucArray()
   \brief Decodes bits to an character array

   \param[in] pSrc       Pointer to the source.
   \param[in] srcOffset  Offset into the source in bits.
   \param[in] pDst       Pointer to the destination value.
   \param[in] numBits    The number of bits to move.

   \retval    srcOffset  The new source offset.
*/
// uint16_t UNPACK_bits2_ucArray( const uint8_t *pSrc, uint16_t srcOffset, uint8_t *pDst, uint8_t numBits )
uint16_t UNPACK_bits2_ucArray( const uint8_t *pSrc, uint16_t srcOffset, uint8_t *pDst, uint16_t numBits )
{
   /* Bits are left justified in the destination */
   srcOffset += copy_bits( pSrc, srcOffset, ( uint8_t * )pDst, 0, numBits );
   return ( srcOffset );
}

/*!
   \f int UNPACK_bits()
   \brief Decodes bits and returns a uint32_t
          The bit source offset is updated by the number of bits decoded.  This allows the
          function to be called for consecutive fields.

   \param[in] pSrc       Pointer to the source.
   \param[in] pSrcOffset Pointer to the offset into the source in bits.
   \param[in] pDst       Pointer to the destination value.
   \param[in] numBits    The number of bits to move (1-32)

   \retval    value      The value of the bits as a uint32_t.

*/
#if 0
uint32_t UNPACK_bits( const uint8_t *pSrc, uint16_t *pSrcOffset, uint8_t numBits )
{
   uint32_t temp = 0;
   unpack_bits( pSrc, *pSrcOffset, ( uint8_t *)&temp, sizeof(temp), numBits );
   *pSrcOffset += numBits;
   return(temp);
}
#endif
uint8_t UNPACK_bits2_uint8 ( const uint8_t *pSrc, uint16_t *pSrcOffset, uint8_t numBits )
{
   uint8_t temp = 0;
   (void)unpack_bits( pSrc, *pSrcOffset, ( uint8_t *)&temp, sizeof(temp), numBits );
   *pSrcOffset += numBits;
   return(temp);
}

uint16_t UNPACK_bits2_uint16 ( const uint8_t *pSrc, uint16_t *pSrcOffset, uint8_t numBits )
{
   uint16_t temp = 0;
   numBits = min( numBits, sizeof(temp) * 8 );
   (void)unpack_bits( pSrc, *pSrcOffset, ( uint8_t *)&temp, sizeof(temp), numBits );
   *pSrcOffset += numBits;
   return(temp);
}

uint32_t UNPACK_bits2_uint32 ( const uint8_t *pSrc, uint16_t *pSrcOffset, uint8_t numBits )
{
   uint32_t temp = 0;
   (void)unpack_bits( pSrc, *pSrcOffset, ( uint8_t *)&temp, sizeof(temp), numBits );
   *pSrcOffset += numBits;
   return(temp);
}

uint64_t UNPACK_bits2_uint64 ( const uint8_t *pSrc, uint16_t *pSrcOffset, uint8_t numBits )
{
   uint64_t temp = 0;
   (void)unpack_bits( pSrc, *pSrcOffset, ( uint8_t *)&temp, sizeof(temp), numBits );
   *pSrcOffset += numBits;
   return(temp);
}

/*!
   \brief This function copies the specified number of bits from source to destination.
        The source and destination are assumed to be in Big Endian Format

   \retval    cnt    The number of bits moved.
*/
static uint8_t copy_bits(
   const uint8_t *pSrc,          /* [in] Pointer to source         */
   uint16_t       srcOffset,     /* [in] Number  of bits to offset */
   uint8_t       *pDst,          /* [in] Pointer to destination    */
   uint8_t        dstOffset,     /* [in] Number of bits to offset  */
   uint16_t       numBits )      /* [in] Number of bits to move    */
{
   // Compute the source mask
   uint8_t srcMask = INITIAL_MASK;
   uint8_t dstMask = INITIAL_MASK;
   uint8_t bitCnt = 0;

   uint8_t ucTemp;
   ucTemp = MOD_8 (srcOffset);               /* Calc the amount to shift the mask           */
   if( ucTemp )
   {
      srcMask >>= ucTemp;
   }
   pSrc += DIV_8(srcOffset);

   ucTemp = MOD_8 (dstOffset);               /* Calc the amount to shift the mask           */
   if( ucTemp )
   {
      dstMask >>= ucTemp;
   }
   // index into the destination
   pDst += DIV_8(dstOffset);

   do
   {
      if( srcMask & *pSrc )
      {
         *pDst |= dstMask;
      }
      else
      {
         *pDst &= ~dstMask;
      }
      srcMask >>= 1;
      dstMask >>= 1;

      if( !srcMask )
      {
         srcMask = INITIAL_MASK;
         pSrc++;
      }
      if( !dstMask )
      {
         dstMask = INITIAL_MASK;
         pDst++;
      }
      bitCnt++;
   } while( --numBits );                              /*    Do it until we are out of bits           */
   return ( bitCnt );

}

/*!
   \brief This function is used to pack bits from a uint8_t, 16, 32, 64 to a bit array.
        The source data is in local machine format (uint8_t, uint16_t, uint32_t, uint64_t).
        The destination data will be packed in Big Endian format
        If the number bits specified is less than the source size, only the LSBits will be packed.
        For example, if the souce is a uint16_t (0x1234) and only 12 bits are specified, the result
        will be [234].
        If destination pointer is NULL then all we are interested in is the size of the operation

   \retval    cnt    The number of bits moved.
*/
static uint8_t pack_bits(
   const uint8_t *pSrc,    /* [in] Pointer to source */
   uint8_t srcSize,        /* [in] Size of source in bytes (1, 2, 4, 8) */
   uint8_t *pDst,          /* [in] Pointer to destination */
   uint16_t dstOffset,     /* [in] Number of  bits to offset */
   uint8_t numBits)        /* [in] Number of bits to move */
{
   // Compute the source mask
   uint8_t srcMask = INITIAL_MASK;
   uint8_t dstMask = INITIAL_MASK;
   uint8_t bitCnt = 0;
   uint8_t ucTemp;
   uint8_t srcOffset;

   // If destination pointer is NULL then all we are interested in is the size of the operation
   if ( pDst == NULL ) {
      return (numBits);
   }

   // Compute the source mask
   srcOffset = (uint8_t)(( srcSize * CHAR_BIT ) - numBits);
   ucTemp     = MOD_8(srcOffset);              /* Calc the amount to shift the mask           */
   if( ucTemp )
   {
      srcMask >>= ucTemp;
   }                                            /* Adjust the source pointer                */
#if (  PROCESSOR_LITTLE_ENDIAN   )
   pSrc += srcSize - ( DIV_8(srcOffset) + 1 );
#else
   pSrc += DIV_8(srcOffset)
#endif

   // Compute the destination mask
   ucTemp     = MOD_8(dstOffset);              /* Calc the amount to shift the mask           */
   if( ucTemp )
   {
      dstMask >>= ucTemp;
   }
   pDst += DIV_8(dstOffset);                    /* Adjust the destination pointer              */

   do
   {
      if( srcMask & *pSrc )
      {
         *pDst |= dstMask;
      }
      else
      {
         *pDst &= ~dstMask;
      }
      srcMask >>= 1;
      dstMask >>= 1;

      if( !srcMask )
      {
         srcMask = INITIAL_MASK;
#if (  PROCESSOR_LITTLE_ENDIAN  )
         pSrc--;
#else
         pSrc++;
#endif
      }
      if( !dstMask )
      {
         dstMask = INITIAL_MASK;
         pDst++;
      }
      bitCnt++;
   } while( --numBits );                              /*    Do it until we are out of bits           */
   return ( bitCnt );

}

/*!
   \brief This function is used to unpack bits from an array to a uint8_t, 16, 32, 64.
          The source data is assumed to be in big endian format.
          The destination data will be in local machine format (uint8_t, uint16_t, uint32_t, uint64_t).
          If the number bits specified is less than the destination size, the results will be
          right justified.   For example, if the source is {0x12, 0x34} and the destination is
          a uint16_t, and only 12 bits are requested, the returned value will be 0x0123.
   \retval    cnt    The number of bits moved.
*/
static uint8_t unpack_bits(
   const  uint8_t *pSrc,   /*  Pointer to source */
   uint16_t srcOffset,     /*  Offset  in bits   */
   uint8_t *pDst,          /*  Pointer to destination */
   uint8_t  dstSize,       /*  Size of the destination in bytes (1, 2, 4, 8)  */
   uint8_t  numBits )      /*  Number of bits to move */
{
   uint8_t srcMask = INITIAL_MASK;                      /* < Source Bit Mask >                      */
   uint8_t dstMask = INITIAL_MASK;                      /* < Destination Bit Mask >                 */
   uint8_t bitCnt = 0;
   uint8_t ucTemp;
   uint8_t dstOffset;

   if ( pSrc != NULL )
   {
      // Compute the source mask
      ucTemp = ( uint8_t )( srcOffset & MOD8 );               /* Calc the amount to shift the src mask       */
      if( ucTemp )
      {
         srcMask >>= ucTemp;
      }                                            /* Adjust the source pointer                */
      pSrc    += DIV_8(srcOffset);

      // Compute the destination mask
      dstOffset  = (uint8_t)(( dstSize * CHAR_BIT ) - numBits);
      ucTemp      = MOD_8(dstOffset);                  /* Calc the amount to shift the dst mask       */
      if( ucTemp )
      {
         dstMask >>= ucTemp;
      }

   // Adjust the destination pointer
#if (  PROCESSOR_LITTLE_ENDIAN   )
      /*lint --e{676} negative offset is OK, here   */
      pDst += dstSize - (DIV_8(dstOffset) + 1);
#else
      pDst += DIV_8(dstOffset)
#endif
      while( numBits != 0){
         if( srcMask & *pSrc )
         {
            *pDst |= dstMask;
         }
         srcMask >>= 1;
         dstMask >>= 1;

         if( !srcMask )
         {
            srcMask = INITIAL_MASK;
            pSrc++;
         }
         if( !dstMask )
         {
            dstMask = INITIAL_MASK;
#if (  PROCESSOR_LITTLE_ENDIAN  )
            pDst--;
#else
            pDst++;
#endif
         }
         bitCnt++;
         numBits--;
      }
   }
   return ( bitCnt );
}

// <editor-fold defaultstate="collapsed" desc="void UNPACK_init( uint8_t *pSrc, pack_t *pUnpackCfg )">
/*******************************************************************************************************************
 *
 * Function name: UNPACK_init
 *
 * Purpose: Initializes the unpack configuration data
 *
 * Arguments: uint8_t *pSrc, pack_t *pUnpackCfg
 *
 * Returns: NONE
 *
 ******************************************************************************************************************/
void UNPACK_init( uint8_t *pSrc, pack_t *pUnpackCfg )
{
   pUnpackCfg->bitNo = 0;
   pUnpackCfg->pArray = pSrc;
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void UNPACK_inc(uint16_t cnt, pack_t *pUnpackCfg)">
/*******************************************************************************************************************
 *
 * Function name: UNPACK_inc
 *
 * Purpose: Increment the unpack buffer bit counter
 *
 * Arguments: uint16_t cnt, pack_t *pUnpackCfg
 *
 * Returns: NONE
 *
 ******************************************************************************************************************/
void UNPACK_inc( uint16_t cnt, pack_t *pUnpackCfg )
{
   pUnpackCfg->bitNo += cnt;
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void UNPACK_Buffer( int8_t bitCount, uint8_t *pResult, pack_t *pack )">
/***********************************************************************************************************************
 *
 * Function name: UNPACK_Buffer
 *
 * Purpose: Reads the bit array
 *
 * Arguments: int8_t   cRBBitCount - Number Of Bits to unpack (if neg, call byte swap after reading)
 *                        Valid Range: 1-127, -1--127. 0 is invalid.
 *         uint8_t *ucRBResult  - Pointer to store the results
 *
 * Returns: NONE
 *
 * NOTE:  The result is ALWAYS stored in little endian!  A neg count means that
 *      the source is big endian and this function will store the result as
 *      little endian.
 *
 * Notes: The default is for the source to be in big endian (msbyte to lsbyte order).
 *      The data will also be stored in       bit endian order.
 *      If the machine is little endian, then the bytes will be swapped!!!
 *      If the source ios in littel endian, then the length should be negative
 *      Thsi will cause the results to be swapped!!!
 *
 The result is ALWAYS stored in little endian!  A neg count means that
 ******************************************************************************************************************/
void UNPACK_Buffer( int8_t bitCount, uint8_t *pResult, pack_t *pack )
{
   uint8_t shiftMask = INITIAL_MASK;
   uint8_t isByteSwap = 0;
   uint8_t bitIndex = 0;
   uint8_t bitOffset;

   if ( !bitCount )                                 /* Bit count of 0 is invalid                */
   {
      return;                                      /*   No bytes were unpacked, return         */
   }

   if ( bitCount < 0 )
   {
      isByteSwap  = 1;                             /*   Input was in little endian             */
      bitCount   *= -1;                            /*   Make the bit count positive            */
   }

   /* Set the byte swap flag
    * If the input was negative then source was big endian, and this function assumes
    * the destination is little endian.
    * At the end the bytes are swapped to correct for this
    * If the machine is big endian, then then logic is reversed to correct for the machine.
    * No need to swap if less 2 bytes!!!
    */

   if ( bitCount > CHAR_BIT )
   {  /* More than 8 bits (i.e Multiple Bytes)       */
      if ( isByteSwap )                             /*   Byte swap the values?                  */
      {                                         /*   Source is big endian                   */
#if (  PROCESSOR_LITTLE_ENDIAN )
         isByteSwap = 1;                           /*    Swap the bytes in the result          */
#else
         isByteSwap = 0;                           /*    Do not swap bytes                  */
#endif
      }
      else
      {                                         /*   Source in little endian                */
#if (  PROCESSOR_LITTLE_ENDIAN )
         isByteSwap = 0;                           /*    Do not swap bytes                  */
#else
         isByteSwap = 1;                           /*    Swap the bytes in the result          */
#endif
      }
   }
   else
   {                                            /* No need to swap bytes if < = 8 bits         */
      isByteSwap = 0;
   }

   bitOffset   = ( uint8_t )( bitCount & MOD8 );

   *pResult = 0;                                /* Clear the result location                */

   /* Shift the mask if needed */
   shiftMask >>= ( uint8_t )( pack->bitNo & ( uint16_t )MOD8 ); /* Shift the mask                        */

   /* If the result is not 0, do the next calculation */
   if ( bitOffset )
   {
      bitIndex = ( uint8_t )( CHAR_BIT - bitOffset );    /*   warn @ -9, Warning is acceptable          */
   }

   do
   {
      if ( shiftMask & pack->pArray[ pack->bitNo / CHAR_BIT ] )
         /*   Bit set?                            */
      {
         /* Shift the temp value down and store the result */
         pResult[ ( uint8_t )bitIndex / CHAR_BIT ]
                 |= ( INITIAL_MASK >> ( uint8_t )( bitIndex & MOD8 ) );
      }
      bitIndex++;                                  /*   Increment the bit index - result buf      */
      pack->bitNo++;                               /*   Increment the bit counter - packed buf    */
      shiftMask >>= 1;                             /*   Shift the mask for the next pass          */
      if ( !shiftMask )                             /*   If the shifted the mask bit away, reset it  */
      {
         shiftMask = INITIAL_MASK;                    /*    Set mask value                     */
      }

      if ( !( bitIndex & MOD8 ) )
      {
         if ( bitCount > 1 )
         {
            pResult[ ( uint8_t )( ( uint8_t )( bitIndex / CHAR_BIT ) ) ] = 0;
            /*      Clear the result loction            */
         }
         if ( !isByteSwap && bitCount <= CHAR_BIT )
         {
            bitIndex += ( uint8_t )( CHAR_BIT - bitOffset );
         }
      }
   }
   while ( --bitCount );                             /*    Do it until we are out of bits           */

   if ( isByteSwap )                                /* Is the byte swap flag set?               */
   {
      Byte_Swap( pResult, ( uint8_t )( bitIndex / CHAR_BIT ) );
      /*   Byte swap the result                   */
   }
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void UNPACK_BufferArray( uint8_t *pArray, int8_t *pBitCount, uint8_t *pResult )">
/***********************************************************************************************************************
 *
 * Function name: UNPACK_BufferArray
 *
 * Purpose: Unpacks multiple values from a char array given an array of bit counts
 *       The results are saved to a structure.
 *       When a structure is used for the results, the structure must be packed()
 *       because this function assumes the data types match the source types.
 *
 * Arguments:
 *       uint8_t *Source Array
 *       int8_t   far *p_cBitCount - Number Of Bits to unpack (if neg, call byte swap after reading)
 *                                 Valid Range: 1-127, -1--127. 0 is invalid.
 *       uint8_t *ucRBResult  - Pointer to store the results
 *       pack_t *pUnpackCfg - Configuration of unpack
 *
 * Returns: NONE
 *
 * NOTE:  The result is ALWAYS stored in little endian!  A neg count means that
 *      the source is big endian and this function will store the result as
 *      little endian.
 ******************************************************************************************************************/
void UNPACK_BufferArray( uint8_t *pArray, int8_t const *pBitCount, uint8_t *pResult, pack_t *pUnpackCfg )
{
   pUnpackCfg->bitNo  = 0;
   pUnpackCfg->pArray = pArray;
   UNPACK_BufferArrayNoInit( pBitCount, pResult, pUnpackCfg );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void UNPACK_BufferArrayNoInit( int8_t *pBitCount, uint8_t *pResult, pack_t *pack )">
/*******************************************************************************************************************
 *
 * Function name: UNPACK_BufferArrayNoInit( int8_t *pBitCount, uint8_t *pResult, pack_t *pack )
 *
 * Purpose: Unpacks the buffer without having to call UnpackInit().  The buffer
 *       gets initialized when this function is called.  This is NULL terminated.
 *
 *
 * Arguments: int8_t   far *cRBBitCount - Array Of Bits to unpack (if neg, call byte swap after reading)
 *                        Valid Range: 1-127, -1--127. 0 is invalid.  Null terminated.
 *         uint8_t *ucRBResult  - Pointer to store the results
 *
 * Returns: NONE
 *
 * NOTE:  The result is ALWAYS stored in little endian!  A neg count means that
 *      the source is big endian and this function will store the result as
 *      little endian.
 ******************************************************************************************************************/
void UNPACK_BufferArrayNoInit( int8_t const *pBitCount, uint8_t *pResult, pack_t *pack )
{
   int8_t bitCount;

   for ( ; ; )
   {
      bitCount = *pBitCount++;
      if ( !bitCount )
      {
         break;
      }
      UNPACK_Buffer( bitCount, pResult, pack );
      if ( bitCount < 0 )
      {
         bitCount *= -1;
      }
      if ( ( uint8_t )bitCount & MOD8 )
      {
         pResult++;
      }
      pResult += ( ( uint8_t )bitCount / CHAR_BIT );
   }
}
/* ****************************************************************************************************************** */
// </editor-fold>

/**********************************************************************************************************************
Function Name: UNPACK_addr

Purpose: This function unpacks an IPv6 address to a memory location

Arguments: pSrc       - Pointer to the source data
           pSrcOffset - bit offset in the source to start pulling in
           numBytes   - number of bytes to unpack
           address    - destination pointer

return
**********************************************************************************************************************/
void UNPACK_addr( uint8_t const *pSrc, uint16_t *pSrcOffset, uint8_t numBytes, uint8_t *address)
{
   uint32_t i;
   // Decode the 40 or 128 bit address
   for(i = 0; i < numBytes; i++)
   {
      address[i] = UNPACK_bits2_uint8(pSrc, pSrcOffset, 8);
   }
}

// <editor-fold defaultstate="collapsed" desc="void PACK_init( uint8_t *pSrc, pack_t *pPackCfg )">
/*******************************************************************************************************************
 *
 * Function name: PACK_init
 *
 * Purpose: Initializes the pack configuration data
 *
 * Arguments: uint8_t *pSrc, pack_t *pPackCfg
 *
 * Returns: NONE
 *
 ******************************************************************************************************************/
void PACK_init( uint8_t *pSrc, pack_t *pPackCfg )
{
   pPackCfg->bitNo = 0;
   pPackCfg->pArray = pSrc;
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void PACK_Byte( uint8_t value, pack_t *pack )">
/*******************************************************************************************************************
 *
 * Function name: PACK_Byte
 *
 * Purpose: Packs a single byte of data
 *
 * Arguments: uint8_t value, pack_t *pack
 *
 * Returns: NONE
 *
 ******************************************************************************************************************/
void PACK_Byte( uint8_t value, pack_t *pack )
{
   PACK_Buffer( 8, &value, pack );
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void PACK_Buffer(int8_t numBits, uint8_t  *pSrc, pack_t *pack )">
/*******************************************************************************************************************
 *
 * Function name: PACK_Buffer
 *
 * Purpose: Copies bits from the source to destination array. The bits have to be in the
 *          correct order high-byte low-byte... etc....
 *
 * Arguments: uint8_t *pData - Pointer to the source data.
 *                             The first byte has to be accounted for in the array.
 *                             The source data is right justified, so if the only
 *                             3 bits are specified, the 3 LSB's are copied.
 *         int16_t   numBits - Number Of Bits to Pack (if neg, call byte swap first! )
 *                             Valid Range: 1-127, -1--127. 0 is invalid.
 *         pack_t pack       - Pointer to the control  structure.  Must be initialized before call.
 *
 *
 * Comments:                                        Source       Destination
 *        numbits = 0        - Does nothing
 *        numbits = 1-7      - Moves LSB's to DST.   0000 0001 -> 1000 0000
 *                                                  0000 0011 -> 1100 0000
 *                                                  ...
 *                                                  0111 1111 -> 1111 1110
 *        numbits = 8*N      - Moves N Bytes.  This is done a bit at a time, so destination does not have to
 *                             start on a byte boundary
 *
 *        numbits = >8  and
 *        numBits%8 > 0      - Note - These cases do not work as one might expect, and
 *                                    are due to an incorrect implementation
 *
 *                           - The copy starts at the MSB of the source and moves bits in order
 *                             until it reaches the last few bits (bitNum % 8 ) of the last byte.
 *                             The following example shows how this works for 12 bits.
 *
 *                               1    2    3    4       1    2    4
 *                             ---- ---- ---- ----    ---- ---- ----
 *                           - 0001 0010 0011 0100 -> 0001 0010 0100
 *
 *         numbits < 0       - When the number of bits is <0, the source data is swapped
 *                             This only works for lengths where the the number of bytes is < 256.
 *                             For most cases this whould be acceptable
 *
 * Returns: None
 *
 ******************************************************************************************************************/
void PACK_Buffer(
                  int16_t  numBits,  /* Number of bits to pack            */
                  uint8_t *pSrc,     /* Pointer to the data to be packed  */
                  pack_t  *pack )    /* Pointer to the control  structure */
{

   uint8_t srcMask;                 /* Initialize the bit mask               */
   uint16_t srcByteIndex;           /* Keeps track of the Byte Index         */
   uint16_t byteCnt;                /* Number of bytes                       */
   uint8_t isByteSwap = 0;          /* Was the data swapped?                 */
   uint8_t bitIndex;

   if ( 0 == numBits )                                /* Error Check. 0 is invalid!               */
   {
      return;                                      /*   Bail out if it is 0, 0 bits packed        */
   }

   if ( !( pack->bitNo & MOD8 ) )                      /* Are we on a byte boundry at the packed array? */
   {
      *pack->pArray = 0U;                             /*   Set the new location to 0              */
   }

   if ( numBits < 0 )                               /* Check if we need to byte swap the data      */
   {
      numBits *= -1;                               /*   Make the number positive               */
      if ( numBits > CHAR_BIT )
      {
         isByteSwap = 1U;                          /*    Swap the source                    */
      }
   }

   /* Calc the number of input bytes  */
   byteCnt = (uint16_t)(numBits / CHAR_BIT);
   if ( ( ( uint8_t )numBits & MOD8 ) )
   {                                            /* If the number of bits don't fall on a byte     */
      /*    boundary                        */
      byteCnt++;                                   /*   Add 1 to the byte count                */
   }

   if ( isByteSwap )
   {
      /* Byte swap the source byte order              */
      Byte_Swap( pSrc, ( uint8_t )byteCnt );
   }

   /* Set the initial mask for the input data.  The initial value being packed may only be a
    * few bits so the ms bits in the byte would be ignored and the bit mask needs to be
    * shifted down.
    */
   if ( !isByteSwap && ( numBits > 8 ) )
   {
      // Source was in little endian and > 8 bits
      bitIndex = 0;
   }
   else
   {
      bitIndex = ( uint8_t )( CHAR_BIT - ( uint8_t )( ( uint8_t )numBits & MOD8 ) );
   }
   srcMask = INITIAL_MASK;                            /* Initialize the bit mask                  */
   if ( bitIndex != CHAR_BIT )                       /* Don't shift if the mask is = to 8     */
   {
      srcMask >>= bitIndex;                           /*   Shift the mask if != 8                 */
   }

#if (  PROCESSOR_LITTLE_ENDIAN  )
   /* On a Little Endian machine the source and destination are the same!!!      */
   srcByteIndex = 0;                                  /* Set the Byte Index to 0 */
#else
   /* On a Big Endian machine the source and destination are reversed            */
   srcByteIndex = byteCnt - 1;
#endif
   do
   {
      /* Is the corresponding bit to the mask set? */
      if ( srcMask & pSrc[ srcByteIndex ] )
      {  /* Yes - Set the bit accordingly         */
         *pack->pArray |= ( INITIAL_MASK >> ( pack->bitNo & MOD8 ) );
      }

      srcMask >>= 1;                               /*   Shift the mask right by 1              */
      pack->bitNo++;                               /*   Increment the bit counter              */

      if ( !srcMask )                               /*   Did the last shift move the 1 off?        */
      {
#if (  PROCESSOR_LITTLE_ENDIAN  )
         srcByteIndex++;                           /*    Decrement the Byte Index              */
#else
         srcByteIndex--;                           /*    Increment the Byte Index              */
#endif
         srcMask = INITIAL_MASK;                      /*    Reset the bit mask                    */
         if ( !isByteSwap && numBits < CHAR_BIT )
         {
            srcMask >>= ( uint8_t )( ( CHAR_BIT - numBits ) + 1 );
         }
      }
      if ( !( pack->bitNo & MOD8 ) )                   /*   Are we on a byte boundary at the packed   */
         /*      array?                        */
      {
         pack->pArray++;                           /*    Increment the pointer to the packed array */
         if ( numBits > 1 )
         {
            *pack->pArray = 0;                        /*      Set the new location to 0           */
         }
      }
   }
   while ( --numBits );                              /*    Go until we are done packing bits        */

   /* If source data was swapped, swap it back  */
   if ( isByteSwap )
   {
      /* Byte swap the source byte order              */
      Byte_Swap( pSrc, ( uint8_t )byteCnt );
   }
}
/* ****************************************************************************************************************** */
   // </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t PACK_hton(uint8_t *pSrc, int8_t *pDecoder, bool bChangeEndian)">
/***********************************************************************************************************************
 *
 * Function name: PACK_hton - Host to Net type of function
 *
 * Purpose: Unpacks from *pSrc and reverses the field order swapping bytes larger than 8-bits (endianess if needed).
 *
 * Arguments: uint8_t *pSrc, uint8_t *pDecoder, bool bChangeEndian
 *
 * Returns: returnStatus_t
 *
 * NOTE:  Todo:  Magic number '15' for the buffer size.  We need to deterimin where this should be defined.
 *
 **********************************************************************************************************************/
returnStatus_t PACK_hton( uint8_t *pSrc, int8_t const *pDecoder, bool bChangeEndian )
{
   uint8_t  result[15];    /* Buffer that contains the packed version of the end result of pSrc. */
   uint8_t  numOfFields;   /* Number of fiels in pDecoder */
   uint8_t  sizeofSrcBits; /* number of bits in the source data, calculated from *pDecoder */
   int8_t   byteSwap;      /* determined from bChangeEndian flag and is set to 1 or -1 (-1 for byte swap) */
   uint8_t  i;             /* Used as a counter/index in the for loops. */
   pack_t   packCfg;       /* Holds the pack configuration. */
   pack_t   unpackCfg;     /* Holds the unpack configuration. */

   if (NULL != pDecoder)
   {
      byteSwap = (bChangeEndian) ? -1 : 1;  /* Set the byteSwap value. */

      /* Need to determin the overall length of the source data and the number of fields to decode. */
      for ( i = 0, numOfFields = 0, sizeofSrcBits = 0; 0 != pDecoder[i]; i++ )
      {
         numOfFields++;
         sizeofSrcBits += (uint8_t)pDecoder[i];
      }
      PACK_init(&result[0], &packCfg);   /* Initialize the pack function - stores data in the result buffer. */
      while ( numOfFields-- )    /* While there are fields to move (unpack and pack) */
      {
         UNPACK_init(pSrc, &unpackCfg);
         for ( i = 0; i < numOfFields; i++ )
         {
            UNPACK_inc((uint8_t)pDecoder[i], &unpackCfg);
         }
         uint8_t buffer[15];    /* Buffer that contians the result of a source data field. */

         UNPACK_Buffer(pDecoder[i], &buffer[0], &unpackCfg);
         PACK_Buffer((byteSwap * pDecoder[i]), &buffer[0], &packCfg);
      }
      (void)memcpy(pSrc, &result[0], sizeofSrcBits / CHAR_BIT);
   }
   return(eSUCCESS);
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="returnStatus_t PACK_ntoh(uint8_t *pSrc, int8_t *pDecoder, bool bChangeEndian)">
/***********************************************************************************************************************
 *
 * Function name: PACK_ntoh - Net to Host type of function
 *
 * Purpose: Unpacks from *pSrc and reverses the field order swapping bytes larger than 8-bits (endianess if needed).
 *
 * Arguments: uint8_t *pSrc, uint8_t *pDecoder, bool bChangeEndian
 *
 * Returns: returnStatus_t
 *
 * NOTE:  Todo:  Magic number '60' for the buffer size.  We need to deterimin where this should be defined.
 *
 **********************************************************************************************************************/
returnStatus_t PACK_ntoh( uint8_t *pSrc, int8_t const *pDecoder, bool bChangeEndian )
{
   returnStatus_t retVal = eFAILURE;   /* Return value - Assume failure. */
   int8_t           decoder[60];         /* contains a reversed copy of *pDecoder. */
   uint8_t          strLen;              /* Length of *pDecoder array. */

   if (NULL != pDecoder)
   {
      for ( strLen = 0; 0 != pDecoder[strLen]; strLen++ )  /* Calculates the string length of *pDecoder. */
      {
      }

      if ( strLen < sizeof(decoder) ) // Number of elements pDecoder points to is shorter than the decoder buffer?
      {
         (void)memcpy(&decoder[0], pDecoder, strLen + 1);      /* Copy the decoder string to the local variable + NULL. */
         Byte_Swap((uint8_t *)&decoder[0], strLen);              /* Byte swap the result */
         retVal = PACK_hton(pSrc, &decoder[0], bChangeEndian); /* use hton to process the results. */
      }
   }
   return(retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>

/**********************************************************************************************************************
Function Name: PACK_addr

Purpose: This function is used to encode a specified length addres field into destination buffer.

Arguments: address   - source address pointer
           numBytes  - length of address in bytes
           pDst      - Pointer to the destination data
           dstOffset - staring bit offset in the destination to pack address into

return: dstOffset  The new destination offset.
**********************************************************************************************************************/
uint16_t PACK_addr(uint8_t const *pSrc, uint8_t numBytes, uint8_t *pDst, uint16_t dstOffset)
{
   uint32_t i;

   for(i = 0; i < numBytes; i++)
   {
      dstOffset = PACK_uint8_2bits(&pSrc[i], 8, pDst, dstOffset);
   }
   return ( dstOffset );
}

