/******************************************************************************

   Filename: EstimateCarrierFrequencyOffset.c

   Global Designator:

   Contents:

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2016 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* Include files */
#include <string.h>
#include "preambleDetector.h"
#include "EstimateCarrierFrequencyOffset.h"

#define MAX_STDDEV_CFO_THRESHOLD 550.0F

/* Function Definitions */
void EstimateCarrierFrequencyOffset(const float b_Preamble_Buffer[1400],
                                    uint32_t LoP,
                                    uint32_t *c_FailCode,
                                    float *c_EstCFO,
                                    float *CfoBasedTmng)
{
   uint32_t nPreaSampsRemaining;
   short Sidx;
   short nCFOest;
   boolean_T CfoTimingDone;
   float MeanPhasAng[75];
   float MeanPhaseOverPeriod;
   int iV;
   float Sum2;
   short ix;
   float diff1;

   /*  REVISION HISTORY:
    *  9Aug18: Introduced *** new CFO algorithm *** requires much lower MIPS, has lower std-dev of CFO-estimate, lower estimate-bias. Verified.
    *          ~~\SoftDemodulator\docs\CFO_estimation_algo\
    *  13Aug18: Increasing the size of array MeanPhasAng from 48 to 75, as it
    * 		   was causing a buffer over flow when a long enough preamble is detected.
    *   5Sep18: Very minor change of variable name.
    */
   *c_EstCFO = -1.0F;
   nPreaSampsRemaining = LoP;

   // This many Preamble samples available in cb_PhaIQ1. This is different from no_PhaIQ1.
   Sidx = 0;

   // Since only samples from SoP onwards are read from cb_LPF1 by rp_LPF1
   nCFOest = -1;
   CfoTimingDone = false;

   // 9Aug18: *** new CFO algorithm *** lower MIPS, lower std-dev, lower bias. ~~\docs\CFO_estimation_algo.
   *CfoBasedTmng = 1.0F;
   memset(&MeanPhasAng[0], 0, 75U * sizeof(float));
   while (!CfoTimingDone)
   {
      /* idxSlicer = (Sidx*V8) + iVec + 1; % This can be +1 instead of +V8.
         ridx_p = mod(rp1_PhaIQ1 + idxSlicer, LEN_PHAIQ1_CB); % Read CB samples using rp1_PhaIQ1 from cb_PhaIQ1 for estimating CFO.
         ridx_n = mod(rp1_PhaIQ1 + idxSlicer + V8, LEN_PHAIQ1_CB);
         p_ang     = cb_PhaIQ1( ridx_p +1); % V phase samples of present symbol.
         n_ang     = cb_PhaIQ1( ridx_n +1); % V phase samples of symbol (2*Tsymb) ahead.
         MeanPhasAng(1 +nCFOest) = mean([p_ang, n_ang]);
      */
      MeanPhaseOverPeriod = 0.0F;
      for (iV = 0; iV < 16; iV++) {
         MeanPhaseOverPeriod += b_Preamble_Buffer[(short)((short)(Sidx << 3) + iV)]; /*lint !e701: Shift left of signed quantity (int) */
      }

      MeanPhaseOverPeriod /= 16.0F;
      MeanPhasAng[(short)(2 + nCFOest) - 1] = MeanPhaseOverPeriod;
      nCFOest++;
      Sidx = (short)(Sidx + 2);

      // Advance by (2*V) samples.
      nPreaSampsRemaining -= 16;

      // Means (2*V) fewer Preamble samples remaining.
      if (nPreaSampsRemaining < 16)
      {
         // Only samples in Preamble portion of cb_PhaIQ1 must be used for CFO-estimation.
         // EstCFO =  mean(diff(MeanPhasAng(1 : nCFOest)))*400;
         MeanPhaseOverPeriod = 0.0F;
         Sum2 = 0.0F;
         for (ix = 1; ix <= nCFOest; ix++)
         {
            diff1 = (MeanPhasAng[ix] - MeanPhasAng[ix - 1]) * 400.0F;
            MeanPhaseOverPeriod += diff1;
            Sum2 += diff1 * diff1;
         }

         if ( nCFOest == 0) {
            // Should not happend but let's avoid a division by 0
            nCFOest = 1;
         }
         *c_EstCFO = MeanPhaseOverPeriod / (float)nCFOest;

         // Variance of CFO_estimate.
         CfoTimingDone = true;
         if (((Sum2 / (float)nCFOest) - (*c_EstCFO * *c_EstCFO)) >= (MAX_STDDEV_CFO_THRESHOLD*MAX_STDDEV_CFO_THRESHOLD))
         {
            // minVal = minimum std-dev of CFO-estimate.
            *c_FailCode = 3; // FailCode is both an input & output of function.
         }
      }
   }// End while(!CfoTimingDone)
}