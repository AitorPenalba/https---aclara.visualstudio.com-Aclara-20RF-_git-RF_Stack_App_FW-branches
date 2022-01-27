/******************************************************************************

   Filename: preambleDetector_data.c

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
#include "preambleDetector_data.h"

/* Variable Definitions */
float ScaleValue;
float Last_iPhase;
float EstCFO;
float Last_Phase;
float Last_PhaseRamp;
float SlicIntg2[200];
float samplesUnProcessedBuffer[13];

uint32_t SlicOutIdx;
uint32_t samplesUnprocessed;
int ZCerrAcc;
uint32_t findSyncStartIndex;

uint32_t FailCode;