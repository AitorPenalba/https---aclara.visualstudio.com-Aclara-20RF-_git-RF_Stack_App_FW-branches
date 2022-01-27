/******************************************************************************

   Filename: FreqDomMlseP4GfskStruct2.h

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

#ifndef FREQDOMMLSEP4GFSKSTRUCT2_H
#define FREQDOMMLSEP4GFSKSTRUCT2_H

/* Include files */
#include <stddef.h>
#include <stdlib.h>
#include "rtwtypes.h"

#define OUTPUT_ARRAY_SIZE (PHY_MAX_FRAME+1)

/* Function Declarations */
extern void FreqDomMlseP4GfskStruct2(const float RX[1024],
                                     uint32_t StartIndex,
                                     uint32_t Length,
                                     unsigned char DecdPSymb_data[OUTPUT_ARRAY_SIZE]);

#endif