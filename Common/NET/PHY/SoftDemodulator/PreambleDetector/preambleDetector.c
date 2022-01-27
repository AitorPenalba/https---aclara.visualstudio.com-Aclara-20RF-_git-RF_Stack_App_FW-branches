/******************************************************************************

   Filename: preambleDetector.c

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
#include <string.h>
#include "preambleDetector.h"
#include "processDerotatedSamples.h"
#include "MaximizeEyeOpening.h"
#include "PerformCFOCompensation.h"
#include "EstimateCarrierFrequencyOffset.h"
#include "ScaleClipFreq2phase.h"
#include "preamble_detect.h"
#include "preambleDetector_data.h"

/* Variable Definitions */
static float Preamble_Buffer[1400];

/* Function Definitions */
void preambleDetector(const float iFrq[150], // A small part of the larger filteredPhaseSamplesBuffer
                      boolean_T resetVariables,
                      boolean_T *FoundPA,
                      uint32_t *LoP, //Length_of_Preamble
                      uint32_t *samplesInPreambleBuffer,
                      float *MeanMaxAbsIFrq)
{
   float CfoBasedTmng;

   /*  REVISION HISTORY
       2Aug18: clean up and improve readability. ScaleValue *= 3/4 added.
       5Sep18: Preamble_Buffer not global anymore. It is persistent. Initialize PREAMBLE_BUFFER_LENGTH.
       21Sep18: PREAMBLE_BUFFER_LENGTH is now global var.
   */

   if(resetVariables)
   {
      preamble_detect_init();
   }
   preamble_detect(iFrq,
                   Preamble_Buffer,
                   FoundPA,
                   LoP, //Length of Preamble
                   samplesInPreambleBuffer,
                   MeanMaxAbsIFrq);
   if (*FoundPA == true)
   {
      // Scale instantaneous-freq. 1.1342 = 1800/1587 = 600/529. Clip to [-FREQ_LIMIT, FREQ_LIMIT].
      ScaleValue = 2645.03613F / *MeanMaxAbsIFrq;
      ScaleValue = ScaleValue * 3.0F / 4.0F;

      /*  20Jun18: Since DerotatedFreq spans +- 4KHz during SYNC, ~ +-2700 during Preamble.
          Scaling, Clipping and FreqToPhase are all combined to one module.
          This module is run on all samples in preamble buffer.
      */
      ScaleClipFreq2phase(Preamble_Buffer,
                          *samplesInPreambleBuffer,
                          &ScaleValue,
                          &Last_iPhase);

      // CFO estimation algorithm is run only on the preamble samples.
      EstimateCarrierFrequencyOffset(Preamble_Buffer,
                                     *LoP,
                                     &FailCode,
                                     &EstCFO,
                                     &CfoBasedTmng);
      if (FailCode == 0)
      {
         // CFO compensation is performed on all samples in the Preamble_Buffer.
         PerformCFOCompensation(Preamble_Buffer,
                                *samplesInPreambleBuffer,
                                EstCFO,
                                &Last_Phase,
                                &Last_PhaseRamp);

         // Eye opening only needs the preamble samples.
         // skip StartSlicer + 1 samples. So startingindex to process is StartSlicer +2.
         SlicOutIdx = 0;

         //                   Derotation on all samples in the buffer.
         findSyncStartIndex = processDerotatedSamples(Preamble_Buffer,
                                                      ((1 + MaximizeEyeOpening(Preamble_Buffer, *LoP)) + 1) + 1,
                                                      *samplesInPreambleBuffer,
                                                      *LoP,
                                                      SlicIntg2,
                                                      &SlicOutIdx,
                                                      samplesUnProcessedBuffer,
                                                      &samplesUnprocessed,
                                                      &ZCerrAcc);
      }
   }
}

void preambleDetector_init(void)
{
   /*  || resetAlgorithm == true */
   memset(Preamble_Buffer, 0, sizeof(Preamble_Buffer));
}