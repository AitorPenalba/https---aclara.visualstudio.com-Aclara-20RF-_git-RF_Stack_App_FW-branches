/******************************************************************************

   Filename: phase2freq_wi_unwrap.h

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

#ifndef PHASE2FREQ_WI_UNWRAP_H
#define PHASE2FREQ_WI_UNWRAP_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include "rtwtypes.h"

/* Function Declarations */
extern void phase2freq_wi_unwrap(const signed char Phase[225], signed char out[225], uint32_t size);
extern void phase2freq_wi_unwrap_init(void);

#endif