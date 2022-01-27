/******************************************************************************

   Filename: preambleDetector.h

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

#ifndef PREAMBLEDETECTOR_H
#define PREAMBLEDETECTOR_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "rtwtypes.h"

/* Function Declarations */
extern void preambleDetector(const float iFrq[150],
                             boolean_T resetVariables,
                             boolean_T *FoundPA,
                             uint32_t *LoP,
                             uint32_t *samplesInPreambleBuffer,
                             float *MeanMaxAbsIFrq);

extern void preambleDetector_init(void);

#endif