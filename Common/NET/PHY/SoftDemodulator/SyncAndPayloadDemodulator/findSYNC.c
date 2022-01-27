/******************************************************************************

   Filename: findSYNC.c

   Global Designator:

   Contents:

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2019 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* Include files */
#include "project.h"
#include "findSYNC.h"
#include "SoftDemodulator.h" //for #define constants
#include <stdint.h>

/* Function Definitions */
void findSYNC(const float RxSeq[1024],
              uint32_t lenRxSeq,
              uint32_t c_findSyncStartIndex,
              uint32_t *MinNumMMs, //Minimum Number of MisMatches
              uint32_t *IdxBestMatch)
{
   uint32_t totalSyncErrors;

   uint32_t i1;
   uint32_t ix;
   uint32_t iy;
   uint32_t iz;

   boolean_T exitg1;
   boolean_T exitg2;
   boolean_T guard1 = false;

   signed char symbol;
   static const signed char sync[NUMBER_SYNC_SYMBOLS] = { 3, -3, -3, -3,  3, -3, -3,  3,
                                                         -3,  3, -3, -3, -3,  3, -3, -3,
                                                         -3,  3, -3,  3,  3, -3,  3,  3,
                                                          3,  3, -3, -3, -3, -3, -3,  3 };

   /* REVISION HISTORY:
    *    2017: Initial version.
    *    2018: Introduced numberSyncErrorsAllowed
    * 10Oct18: Introduced quick exit if totalSyncErrors >= MinNumMMs since that allow RTOS thread to return success/fail quicker.
    *          Helps with real-time preamble_detector that sometimes finds false preambles.
    *  1Nov18: Introduced break inside if (totalSyncErrors < MinNumMMs) with reasoning explained below.
    */

   *MinNumMMs = NUMBER_OF_SYNC_ERRORS_ALLOWED + 1;

   /* CHECK: MinNumMMs must be static if findSYNC() is called multiple times. */
   *IdxBestMatch = 0;

   /* Start search 32 indices before input argument findSyncStartIndex but start-index must be >= 0. */
   if ((c_findSyncStartIndex - NUMBER_SYNC_SYMBOLS) > 0)
   {
      c_findSyncStartIndex -= NUMBER_SYNC_SYMBOLS;
   }
   else
   {
      c_findSyncStartIndex = 0;
   }

   i1 = (lenRxSeq - NUMBER_SYNC_SYMBOLS);
   ix = c_findSyncStartIndex;
   exitg1 = false;
   while ((!exitg1) && (ix <= i1))
   {
      totalSyncErrors = 0;
      iz = 0;
      iy = ix;
      exitg2 = false;
      while ((!exitg2) && (iy < (ix + NUMBER_SYNC_SYMBOLS)))
      {
         if (RxSeq[iy] < 0.0F)
         {
            symbol = -3;
         }
         else if (RxSeq[iy] > 0.0F)
         {
            symbol = 3;
         }
         else
         {
            symbol = 0;
         }

         guard1 = false;
         if (symbol != sync[iz])
         {
            totalSyncErrors++;
            if (totalSyncErrors > NUMBER_OF_SYNC_ERRORS_ALLOWED)
            {
               exitg2 = true;
            }
            else
            {
               guard1 = true;
            }
         }
         else
         {
            guard1 = true;
         }

         if (guard1)
         {
            iz++;
            iy++;
         }
      }

      /* totalSyncErrors = # mismatches w.r.t. SYNC when StartIndex = ix. */
      if (totalSyncErrors <= NUMBER_OF_SYNC_ERRORS_ALLOWED)
      {
         *MinNumMMs    = totalSyncErrors; // Record index giving best SYNC-match and #Mismatches at that index.
         *IdxBestMatch = ix;              // When MinNumMMs << 32: if RX-segment with #Mismatches < 4 is found, this means #Matches > 28.
         exitg1        = true;            // But any shift of RX-segment will lead to large # of mismatches since SYNC has almost delta-func auto-correlation.
      }
      else
      {
         /* Another segment of RX much later (> RX[33 +n :64 +n] could be a better SYNC match but that is very unlikely. */
         ix++;
      }
   }

   /* sprintf('Found SYNC in RxSeq at %d with %d errors', IdxBestMatch, MinNumMMs) */
}