/******************************************************************************

   Filename: processDerotatedSamples.c

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
#include <math.h> //roundf
#include <stdlib.h> //abs(short)
#include "processDerotatedSPDSamples.h"
#include "sign.h"

#define SPD_TIMING_LOOP_THRESHOLD 1152 // 48*24? Must be an interger.

// Used in SyncAndPayloadDemodulator.c
void processDerotatedSPDSamples(const float newSamples_data[FILTERED_SAMPLES_PER_EVENT], //[150]
                                float    b_SlicIntg2[200],
                                uint32_t *c_SlicOutIdx,
                                float    b_samplesUnProcessedBuffer[13],
                                uint32_t *samplesUnProcessed,
                                int      *c_ZCerrAcc)
{
   uint32_t samplesToProcess;
   uint32_t jxAdj;
   uint32_t i;
   uint32_t IdxMaxAbsZCdet;
   uint32_t b_index;
   uint32_t iv0[8];
   short signZCinp[9];
   short ZCdet[8];

   float b_samples[165]={0}; //doesn't need to be bigger than 150+13=163, technically
   float ZCinp[9];
   float MaxZCinp;
   float MinZCinp;
   float f0;

   short nNonZeroEntries;
   int32_t sumAbsZCdet;
   int32_t MaxAbsZCdet;

   /*  REVISION HISTORY:
       2Aug18: ZC_MAGN_THR changed. Coded 2 algo.. options for Slicer with adaptive-timing-loop.
      19Sep18: Timing-loop-changes to handle Baud-Freq-Offset. Only subset of ZCs contribute to timing-error. New Thresholds ZC_MAX_PLUS_MIN_UB1,2.
       1Oct18: CORRECTED mistake in forming MaxAbsZCdet, IdxMaxAbsZCdet.
     codegen
      Upper-bound on asymmetry between "+ve max" and "-ve min".
      Upper-bound on asymmetry between "+ve max" and "-ve min" when ZCerr <= 3 or >= 7.++
      ZC_MAGN_THR = single(30); % Threshold for valid zero-crossing during adaptive-timing-loop.
      WGHT_APLD_2_FREQ = single([1 1 1 1, 1 1 1 1]); % Weight applied to 8-consecutive-samples before summing to form Slicer output.
   */
   samplesToProcess = 0;

   /* copy the samples yet to be processed from prior call to samples[1...] and then append the newsamples. */
   for (i = 0; i < *samplesUnProcessed; i++)
   {
      /* samplesUnProcessed will be 0 on first call from preambleDetector(). */
      b_samples[samplesToProcess++] = b_samplesUnProcessedBuffer[i];
   }

   /* copy the newSamples[] input buffer to local buffer samples[]. */
   for (i = 0; i < FILTERED_SAMPLES_PER_EVENT; i++)
   {
      b_samples[samplesToProcess++] = newSamples_data[i];
   }

   /* Slicer with adaptive timing-loop. o/p rate = Fbaud, i/p rate = 8*Fbaud. */
   b_index = 0;
   while (samplesToProcess > 13)
   {
      /* Since highest linear-index of DeRotFreq accessed is (jx + 3*V/2). */
      b_SlicIntg2[*c_SlicOutIdx] = 0.0F;

      /* SlicIntg2(SlicOutIdx) = WGHT_APLD_2_FREQ * SlicInp'; % is input to freq-domain-MLSE. */
      /* ridx2 = mod(rp_DerotFreq + (1:V8), LEN_DEROTFREQ_CB); */
      /* SlicInp = cb_DerotFreq(ridx2 +1) /Fb /2; */
      for (IdxMaxAbsZCdet = 0; IdxMaxAbsZCdet < 8; IdxMaxAbsZCdet++)
      {
         iv0[IdxMaxAbsZCdet] = (IdxMaxAbsZCdet + b_index);
      }

      for (i = 0; i < 8; i++)
      {
         // Accumulate samples at indices [1:8] to produce SlicIntg2(1). Increment index & repeat.
         b_SlicIntg2[*c_SlicOutIdx] += b_samples[iv0[i]];
      }

      b_SlicIntg2[*c_SlicOutIdx] *= ((1.0F / 4800.0F) / 2.0F);

      // Pre-allocate vectors to allow RTOS to produce efficient C-code.
      for (IdxMaxAbsZCdet = 0; IdxMaxAbsZCdet < 9; IdxMaxAbsZCdet++)
      {
         ZCinp[IdxMaxAbsZCdet] = 0.0F;
         signZCinp[IdxMaxAbsZCdet] = 0;
      }

      for (IdxMaxAbsZCdet = 0; IdxMaxAbsZCdet < 8; IdxMaxAbsZCdet++)
      {
         ZCdet[IdxMaxAbsZCdet] = 0;
      }

      // length is 1 less due to diff().
      MaxZCinp = -5000.0F;
      MinZCinp = 5000.0F;
      for (i = 0; i < 9; i++)
      {
         ZCinp[i] = b_samples[(b_index + i) + 3];
         f0 = ZCinp[i];
         b_sign(&f0); //Changes float value to just the sign (either 1.0 or -1.0 (or 0.0))
         signZCinp[i] = (short)(f0);
         if (i > 0)
         {
            // Mimic Matlab diff() operation.
            ZCdet[i - 1] = (short)(signZCinp[i] - signZCinp[i - 1]);
         }

         if (ZCinp[i] > MaxZCinp)
         {
            MaxZCinp = ZCinp[i];
         }
         else //(ZCinp[i] < MinZCinp)
         {
            MinZCinp = ZCinp[i];
         }
      }

      nNonZeroEntries = 0;
      sumAbsZCdet = 0;
      MaxAbsZCdet = 0;

      /*  Needed to compute IdxMaxAbsZCdet. */
      IdxMaxAbsZCdet = 0;
      for (i = 0; i < 8; i++)
      {
         if ( ZCdet[i] ) {
            nNonZeroEntries++;
         }
         sumAbsZCdet += abs(ZCdet[i]);
         if (abs(ZCdet[i]) > MaxAbsZCdet)
         {
            /*  1Oct18: changed ZCinp to ZCdet. */
            MaxAbsZCdet = abs(ZCdet[i]);
            IdxMaxAbsZCdet = 1 + i;
         }
      }

      MaxAbsZCdet = 0;
      if ((nNonZeroEntries == 1) &&
          (sumAbsZCdet == 2) &&
          (! (fabsf(MaxZCinp + MinZCinp) > 1600.0F))&&
          (!((fabsf(MaxZCinp + MinZCinp) > 1000.0F) && (IdxMaxAbsZCdet <= 3))) &&
          (!((fabsf(MaxZCinp + MinZCinp) > 1000.0F) && (IdxMaxAbsZCdet >= 7))))
      {
         /*  Equivalent to if (sum(ZCdet ~= 0) == 1) && (sum(abs(ZCdet)) == 2). */
         MaxAbsZCdet = (int32_t)IdxMaxAbsZCdet - 5;

         /*  ZCerr is estimated clock_timing_error. */
      }

      if (MaxAbsZCdet != -4)
      {
         /*  Since corresponding ZCerr = +4 is not possible (since length(ZCinp) = 9). */
         *c_ZCerrAcc += MaxAbsZCdet;

         /*  Accumulate the clock_timing_error. */
      }

      if (*c_ZCerrAcc <= -SPD_TIMING_LOOP_THRESHOLD)
      {
         jxAdj = 7;

         /*  ZCerr ; % -1; % Using jxAdj = ZCerr will increase BW. */
         *c_ZCerrAcc += SPD_TIMING_LOOP_THRESHOLD;
      }
      else if (*c_ZCerrAcc >= SPD_TIMING_LOOP_THRESHOLD)
      {
         jxAdj = 9;

         /*  ZCerr; % 1; % But ZCerr is also +/-1 when processing Mario's data. Why? */
         *c_ZCerrAcc -= SPD_TIMING_LOOP_THRESHOLD;
      }
      else
      {
         jxAdj = 8;
      }

      /*  If index is incremented, samplesToProcess must be decremented. */
      b_index += jxAdj;

      /*  Incrementing index same as incrementing jx. */
      samplesToProcess -= jxAdj;

      /*  in old linear-buffer method. */
      (*c_SlicOutIdx)++;
   }

   /*  close while (samplesToProcess > V + 1). */
   /*  Store remaining samples that will be processed during next call. */
   *samplesUnProcessed = 0;
   IdxMaxAbsZCdet = b_index + samplesToProcess;
   while (b_index < IdxMaxAbsZCdet)
   {
      b_samplesUnProcessedBuffer[*samplesUnProcessed] = b_samples[b_index++];
      (*samplesUnProcessed)++;
   }
}