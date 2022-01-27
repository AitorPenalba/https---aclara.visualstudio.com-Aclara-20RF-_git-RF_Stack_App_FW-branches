/******************************************************************************

   Filename: PerformCFOCompensation.c

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
#include "PerformCFOCompensation.h"

/* Function Definitions */
void PerformCFOCompensation(float b_Preamble_Buffer[1400],
                            uint32_t samplesInPreambleBuffer,
                            float estCFO,
                            float *c_Last_Phase,
                            float *c_Last_PhaseRamp)
{
  float Scaling;
  uint32_t i;
  float phase;
  float freq;
  Scaling = 6.28318548F * estCFO / 38400.0F;
  for (i = 0; i < samplesInPreambleBuffer; i++) {
    phase = b_Preamble_Buffer[i] - (*c_Last_PhaseRamp +  Scaling * (float)(i));
    freq = phase - *c_Last_Phase;
    *c_Last_Phase = phase;
    b_Preamble_Buffer[i] = freq * (((4800.0F * 8.0F) / 2.0F) / 3.14159274F);
  }

  *c_Last_PhaseRamp += Scaling * (float)(samplesInPreambleBuffer - 1);
}