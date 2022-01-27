/******************************************************************************

   Filename: preambleDetector_data.h

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

#ifndef PREAMBLEDETECTOR_DATA_H
#define PREAMBLEDETECTOR_DATA_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "rtwtypes.h"

/* Variable Declarations */
extern float ScaleValue;
extern float Last_iPhase;
extern float EstCFO;
extern float Last_Phase;
extern float Last_PhaseRamp;
extern float SlicIntg2[200];
extern float samplesUnProcessedBuffer[13];

extern uint32_t SlicOutIdx;
extern uint32_t samplesUnprocessed;
extern int ZCerrAcc;
extern uint32_t findSyncStartIndex;

extern uint32_t FailCode;

#endif