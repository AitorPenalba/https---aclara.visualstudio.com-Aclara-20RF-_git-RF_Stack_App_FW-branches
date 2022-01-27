/******************************************************************************

   Filename: ScaleClipFreq2phase.c

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
#include <stdint.h>
#include "preambleDetector.h"
#include "ScaleClipFreq2phase.h"

/* Function Definitions */
void ScaleClipFreq2phase(float b_Preamble_Buffer[1400],
                         uint32_t samplesInPreambleBuffer,
                         const float *c_ScaleValue,
                         float *c_Last_iPhase)
{
  float total;
  uint32_t i;
  float ScldFreq;

  /*  PURPOSE:  phase(t) = integral{2 pi freq(t) t}, phase[n] = cumsum{ 2 pi freq[1:n] Ts } */
  /*  INPUTs */
  /*  OUTPUTs: */
  /*  REVISION HISTORY: */
  /*  13Nov17 Initial version, verified on Pkt1_Repeated-500_116dBm_150B_SpiC2p5M_SlowSlew_Fd1K_Hprty */
  /*  */
  total = 0.0F;
  for (i = 0; i < samplesInPreambleBuffer; i++)
  {
    ScldFreq = b_Preamble_Buffer[i] * *c_ScaleValue;
    if (!(ScldFreq < 9000.0F))
    {
      ScldFreq = 9000.0F;
    }

    if (!(ScldFreq > -9000.0F))
    {
      ScldFreq = -9000.0F;
    }

    total += ScldFreq * 0.000163624616F;

    /* disp([total]); */
    b_Preamble_Buffer[i] = *c_Last_iPhase + total;
  }

  *c_Last_iPhase += total;
}