/******************************************************************************

   Filename: MaximizeEyeOpening.c

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

// Include files
#include <math.h>
#include <stdint.h>
#include "preambleDetector.h"
#include "MaximizeEyeOpening.h"

// Function Definitions
uint32_t MaximizeEyeOpening(const float b_Preamble_Buffer[1400],
                            uint32_t lengthOfPreamble)
{
  uint32_t EyeBasedTmng;
  uint32_t maxi;
  float maxt;
  uint32_t nSym;
  float t;
  uint32_t b_j;
  uint32_t sampOffset;

  uint32_t i;
  uint32_t i1;
  uint32_t j;
  uint32_t k;

  // REVISION HISTORY
  // 1Aug18: clean up, improve readability.
  maxt = 0.0F;
  maxi = 0;
  nSym = (uint32_t)floorf((((float)(lengthOfPreamble) / 8.0F) / 2.0F) + 0.1F) * 16;

  // Since eye-diagram-timing established using ONLY Preamble portion.
  sampOffset = 1U;

  // nSym really means #samples at 8x inside the Preamble. nSym will be multiple of 16 = 2V.
  for (i = 0; i < 8; i++) {
    // i=1 means PreBuf(CfoTmg + [1: 8]) is accumulated(+) and PreBuf(CfoTmg + [ 9:16]) is accumulated(-).
    // i=2 means PreBuf(CfoTmg + [2: 9]) is accumulated(+) and PreBuf(CfoTmg + [10:17]) is accumulated(-).
    t = 0.0F;

    // i=3 means PreBuf(CfoTmg + [3:10]) is accumulated(+) and PreBuf(CfoTmg + [11:18]) is accumulated(-).
    i1 = ((sampOffset + nSym) + (16 - sampOffset)) / 16;
    for (j = 0; j < i1; j++) {
      b_j = sampOffset + j * 16;
      // j defines timing-index.
      for (k = 0; k < 8; k++) {
        t += b_Preamble_Buffer[b_j + k];
        // Accumulate(+) next 8 samples.
      }
      for (k = 0; k < 8; k++) {
        t -= b_Preamble_Buffer[b_j + (8 + k)];
        // Accumulate(-) next 8 samples.
      }
    }

    if (fabsf(t) > maxt)
    {
      // HTM if should succeed at least once.
      maxi = 1 + i;

      // Record loop-iteration that gives maximum and the maximum-value.
      maxt = fabsf(t);
    }

    sampOffset++;
    // Advances timing by Tsymb/8 (+1 if sampling-rate is 8x).
  }

  if (maxi > 1)
  {
    // maxi in [1, 8].
    EyeBasedTmng = maxi - 2;
    // Map maxi [2,..,8] to EyeBasedTmng [0,..,6].
  }
  else
  {
    EyeBasedTmng = (maxi + 8) - 2;
    // Map maxi {1} to EyeBasedTmng {7}.
  }

  return EyeBasedTmng;
}