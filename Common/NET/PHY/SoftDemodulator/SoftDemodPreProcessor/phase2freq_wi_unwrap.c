/******************************************************************************

   Filename: phase2freq_wi_unwrap.c

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
#include <stdint.h>
#include "SoftDemodulator.h"
#include "phase2freq_wi_unwrap.h"

/* Variable Definitions */
static uint32_t LastPhase;

/* Function Definitions */
void phase2freq_wi_unwrap(const signed char Phase[225], signed char out[225], uint32_t size)
{
   uint32_t i;
   uint32_t phase;

   /*  PURPOSE: Discrete-time 1st derivative of phase (from radioIC) to produce instantaneous-freq
    *  INPUTs: "Phase" is the instantneous-phase from radio-IC of the signal it is listening to.
    *  OUTPUTs: "Freq" is the instantneous-freq
    *  REVISION HISTORY:
    *  9Nov17: initial version replaces in-line-code in DemodPhaseSeqFromIC6_fw.m
    *
    *  Max allowed difference between adjacent phase samples.
    *  phase value corresponding to pi [radians].
    *  PhasSeqFromIC is phase @ sampling-freq = OSR x Fbaud
    */
   for (i = 0; i < size; i++)
   {
      phase = Phase[i] << 25;/*lint !e701  Shift left of signed quantity (int) */
      *out++ = ((int32_t)(phase - LastPhase)) >> 25;/*lint !e702 Shift right of signed quantity (int) */
      LastPhase = phase;

      /* // Original code for reference
      b_out = (signed char)(Phase[i] - LastPhase);
      LastPhase = Phase[i];
      if (b_out > 63)
      {
         b_out = (signed char)(b_out - 128);
      }
      else
      {
         if (b_out < -64)
         {
            b_out = (signed char)(b_out + 128);
         }
      }
      out[i] = b_out; */
   }
}

void phase2freq_wi_unwrap_init(void)
{
   /* LastPhase = Phase(1) - (Phase(2) - Phase(1)); % 25Jan18: Causes perf. loss for "Agilent TX -118 dBm". */
   LastPhase = 0;
}