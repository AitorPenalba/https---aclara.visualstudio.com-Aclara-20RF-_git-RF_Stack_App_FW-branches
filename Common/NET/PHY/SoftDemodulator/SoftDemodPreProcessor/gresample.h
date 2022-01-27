/******************************************************************************

   Filename: gresample.h

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

#ifndef GRESAMPLE_H
#define GRESAMPLE_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "rtwtypes.h"
#include "SoftDemodulator.h" //For RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL(225)

//This is horizontal stretch of the sinc function
// sin(x/r)*(r/x), "r" being stretch, or the "RESAMPLER_FACTOR"
#define SINC_STRETCH_FACTOR 30.0F

#define INPUTS_PER_OUTPUT 1.52587891F //12.20703125/8, giving roughly 147.45 samples per 225

/* Function Declarations */
extern uint32_t gresample(const signed char input[RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL], float v_out[RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL], uint32_t size);
extern void gresample_px_sinc_table_init(void);

#endif