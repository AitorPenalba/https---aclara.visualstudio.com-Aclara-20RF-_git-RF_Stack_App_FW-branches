/******************************************************************************

   Filename: fir_filt_circ_buff.h

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

#ifndef FIR_FILT_CIRC_BUFF_H
#define FIR_FILT_CIRC_BUFF_H

/* Include files */
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "rtwtypes.h"

#define NUMBER_OF_FILTER_TAPS 32

/* Function Declarations */

//extern boolean_T fir_filt_circ_buff(const float Inp[225], uint32_t lenInp); //old
#ifdef _SOFT_DEMOD_H_
extern boolean_T fir_filt_circ_buff(unProcessedData_t *unProcessedData);
#endif

extern void fir_filt_circ_buff_init(void);

#endif