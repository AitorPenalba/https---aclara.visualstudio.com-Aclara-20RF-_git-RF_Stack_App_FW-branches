/******************************************************************************

   Filename: preambleDetector_initialize.c

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
#include <stdint.h>
#include "preambleDetector.h"
#include "preambleDetector_initialize.h"
#include "preamble_detect.h"
#include "preambleDetector_data.h"

/* Named Constants */
#define b_ScaleValue                   (0.0F)
#define b_Last_iPhase                  (0.0F)
#define b_EstCFO                       (0.0F)
#define b_FailCode                     (0)
#define b_Last_Phase                   (0.0F)
#define b_Last_PhaseRamp               (0.0F)
#define b_SlicOutIdx                   (0)
#define b_samplesUnprocessed           (0)
#define b_ZCerrAcc                     (0)
#define b_findSyncStartIndex           (0)

/* Function Definitions */
void preambleDetector_initialize(void)
{
  uint32_t i;
  findSyncStartIndex = b_findSyncStartIndex;
  ZCerrAcc = b_ZCerrAcc;
  samplesUnprocessed = b_samplesUnprocessed;
  for (i = 0; i < 13; i++)
  {
    samplesUnProcessedBuffer[i] = 0.0F;
  }

  memset(&SlicIntg2[0], 0, 200U * sizeof(float));
  SlicOutIdx = b_SlicOutIdx;
  Last_PhaseRamp = b_Last_PhaseRamp;
  Last_Phase = b_Last_Phase;
  FailCode = b_FailCode;
  EstCFO = b_EstCFO;
  Last_iPhase = b_Last_iPhase;
  ScaleValue = b_ScaleValue;
  preambleDetector_init();
  preamble_detect_init();
}