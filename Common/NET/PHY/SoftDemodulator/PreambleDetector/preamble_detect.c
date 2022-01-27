/******************************************************************************

   Filename: preamble_detect.c

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
#include <math.h>
#include <stdint.h>
#include "preambleDetector.h"
#include "preamble_detect.h"
#include "PHY.h"       //RadioEvent_PreambleDetect(radio#) to increment phystats counter
#include "radio_hal.h" //RADIO_0 for radio number for above function
#include "SoftDemodulator.h" //For FILTERED_SAMPLES_PER_EVENT

#define MINIMUM_PREAMBLE_LENGTH 88
#define MIN_MAX_ABS_IFRQ 2.0F

/* Variable Definitions */
static float samples_data[182];
static float MaxAbsIFrq;
static float MaxAbsIfrqTotal;

static uint32_t nMaxAbsIFrq;
static uint32_t samples_index;
static uint32_t preamble_buffer_index;
static uint32_t Mod16Cntr; //counts from 0 to 15 and loops back

static uint32_t Count1;
static boolean_T SoPFound;

/* Function Definitions */
void preamble_detect(const float iFrq[150],
                     float b_Preamble_Buffer[1400],
                     boolean_T *FoundPA,
                     uint32_t *LoP_o, //Length_of_Preamble
                     uint32_t *samplesInPreambleBuffer,
                     float *MeanMaxAbsIFrq)
{
  float MaxFreqDev;
  float MinFreqDev;
  uint32_t countThreshold;
  uint32_t totalSamples;
  uint32_t ii;
  uint32_t j;
  boolean_T exitg1;
  boolean_T guard1 = false;

  /*  REVISION HISTORY
       2Aug18: nMaxAbsIFrq, MaxAbsIfrqTotal set to 0 appropriately. maxPreambleVal, minPreambleVal disabled.
               Disabled "|| preamble_buffer_index >= MAX_REQD_PREA_LEN" in finding SoP.
      13Aug18: Changed to "if (preamble_buffer_index -1 -DIFF16_CNT_THR2) < MINIMUM_PREAMBLE_LENGTH".
      15Aug18: diffTotal formula back to original since CHANGE DONE IN FIELD AT "Cooke County" does NOT WORK WHEN |CFO| > 500 Hz.
       5Sep18: Introduce diffTotal2 which complements diffTotal. It checks for sign-change among neighboring half-cycles.
       persistent variables initialization
      Checked-in-value 25. 20 works well for RSSI > -120 dBm. 30 needed for -123 dBm.
      # consecutive indices n for which (Diff16apart[n] < DIFF16_AMP_THR) to find SoP.
      # Checked-in 1*V. consecutive indices n for which (Diff16apart[n] > DIFF16_AMP_THR) to find EoP.
      Checked-in-value 20. - Minimum difference between adjacent 1/2 cycles
  */

  for (countThreshold = 0; countThreshold < FILTERED_SAMPLES_PER_EVENT; countThreshold++) {
    //  Append FILTERED_SAMPLES_PER_EVENT new input samples to samples already in samples[].
    samples_data[samples_index++] = iFrq[countThreshold];
  }
  totalSamples = samples_index; //Misnomer: actually total # of samples yet to be processed.

  // Gathering samples frame-by-frame (works with unequal # samples/frame) for instantaneous-frequency.
  // Since wp_InstFrq points to last location written, start write at (wp_InstFrq +1).
  *FoundPA = false;
  *LoP_o = 0;
  *samplesInPreambleBuffer = 0;
  *MeanMaxAbsIFrq = 0.0F;

  //  We will not process the last 2V samples in this call. Groups of 8,4 to fix int/round Matlab issue.
  //  decide which threshold is the target based upon whether we found the SoP
  if (SoPFound)
  {
    countThreshold = 8;
  }
  else
  {
    countThreshold = 24;
  }

  ii = 0;
  exitg1 = false;
  while ((!exitg1) && (ii < totalSamples - 16))
  {
    // Peak +ve, Peak -ve, Peak-to-Peak freq-deviation once every 2*V samples; segment of length 2*V.
    MaxFreqDev = 0.0F;
    MinFreqDev = 0.0F;
    if (Mod16Cntr == 0)
    {
      //  Mod16Cntr must be persistent.
      for (j = 0; j < 16; j++) {
        if (samples_data[ii + j] > MaxFreqDev)
        {
          MaxFreqDev = samples_data[ii + j];
        }
        else if (samples_data[ii + j] < MinFreqDev)
        {
          MinFreqDev = samples_data[ii + j];
        }
      }

      MaxAbsIFrq = (MaxFreqDev - MinFreqDev) / 2.0F;
    }

    // Compute diffTotal (Accumulate abs of 8 subtractions; each a difference of samples separated by 2*V indices). Similarly diffTotal2.
    MaxFreqDev = 0.0F;
    MinFreqDev = 0.0F;
    for (j = 0; j < 8; j++) {
      MaxFreqDev += fabsf(samples_data[ii+j] - samples_data[ii+j+16]);
      // Should be small during Preamble.
      MinFreqDev += fabsf(samples_data[ii+j] - samples_data[ii+j+8]);
      // Should be large during Preamble.
    }

    // Main logic for SoP, EoP.
    guard1 = false;
    if (SoPFound)
    {
      // using persistent write_pointer preamble_buffer_index.
      b_Preamble_Buffer[preamble_buffer_index++] = samples_data[ii];
      // Finding EoP.

      if (Mod16Cntr == 0)
      {
        // Accumulate max-abs of inst-freq every 2*V samples after SoP has been found.
        MaxAbsIfrqTotal += MaxAbsIFrq;
        nMaxAbsIFrq++;
      }

      if ((MaxFreqDev > 25.0F) || (MinFreqDev < 20.0F) || (MaxAbsIFrq < MIN_MAX_ABS_IFRQ) || (preamble_buffer_index >= 1075))
      {
        Count1++;
        if ((Count1 >= countThreshold) || (preamble_buffer_index >= 1075))
        {
          Count1 = 0;
          SoPFound = false;

          // reset the start of preamble to detect the next preamble.
          countThreshold = 24;

          // Set to THR for SoP.
          if ((preamble_buffer_index - 1) - 8 < MINIMUM_PREAMBLE_LENGTH)
          {
            // Preamble length too short.
            preamble_buffer_index = 0;

            // CHECK: should be complete reset of preamble_detect.
            MaxAbsIfrqTotal = 0.0F;
            nMaxAbsIFrq = 0;
            guard1 = true;
          }
          else
          {
            // Preamble length adequate.
            *LoP_o = (preamble_buffer_index - 1) - 8;
            *FoundPA = true;

            // Indicate that both SoP and EoP have been found.
            for (countThreshold = ii + 1; countThreshold < totalSamples; countThreshold++)
            {
              // Copy samples after Preamble into Preamble_Buffer.
              // preamble_buffer_index keeps track of length of preamble.
              b_Preamble_Buffer[preamble_buffer_index++] =  samples_data[countThreshold];
            }

            *samplesInPreambleBuffer = preamble_buffer_index;

            // Represents total # of samples including Preamble.
            samples_index = 0;
            preamble_buffer_index = 0;
            if (nMaxAbsIFrq != 0)
            {
              *MeanMaxAbsIFrq = MaxAbsIfrqTotal / (float)nMaxAbsIFrq;
            }

            exitg1 = true;
          }
        }
        else
        {
          guard1 = true;
        }
      }
      else
      {
        // Count1 = int32(0);
        if (Count1 > 0)
        {
          Count1--;
        }

        guard1 = true;
      }
    }
    else // Finding SoP.
    {
      if ((MaxFreqDev < 25.0F) && (MinFreqDev > 20.0F) && (MaxAbsIFrq > MIN_MAX_ABS_IFRQ))
      {
        Count1++;

        // Increment if consecutive differences meet threshold test.
        // CHECK: NOT a TRUE preamble if Count1 < countThreshold and preamble_buffer_index >= MAX_REQD_PREA_LEN.
        if (Count1 >= countThreshold)
        {
          // 2Aug18: 139 PhaseSamplesCookeCounty_29Jun18, PhaseSamplesUnit1-All
          Count1 = 0;
          SoPFound = true;
          MaxAbsIfrqTotal = 0.0F;

          // CHECK: nMaxAbsIFrq must also be set to 0.
          nMaxAbsIFrq = 0;
          countThreshold = 8;

          // Set to THR for EoP.
          preamble_buffer_index = 1;

          // Start writing Preamble to Preamble_Buffer[], maintain preamble_buffer_index.
          b_Preamble_Buffer[0] = samples_data[ii];
        }
      }
      else
      {
        Count1 = 0; // Reset to 0 if not consecutive.
      }

      guard1 = true;
    }

    if(guard1)
    {
      Mod16Cntr++;
      Mod16Cntr &= 0x0F;
      // Modulo-16 counter

      ii++;
    }
  }

  // Copy un-processed samples from highest-indices to lowest-indices.
  samples_index = 0;
  while (ii < totalSamples)
  {
    // CHECK: no need to do this if we get here from break above.
    samples_data[samples_index++] = samples_data[ii++];
  }
}

void preamble_detect_init(void)
{
  Count1 = 0;
  preamble_buffer_index = 0;
  SoPFound = false;
  MaxAbsIfrqTotal = 0.0F;
  nMaxAbsIFrq = 0;
  Mod16Cntr = 0;
  MaxAbsIFrq = 0.0F;
  samples_index = 0;
}