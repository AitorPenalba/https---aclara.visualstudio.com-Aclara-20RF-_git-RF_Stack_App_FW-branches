/******************************************************************************

   Filename: gresample.c

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
#include "gresample.h"
#include "SoftDemodulator.h" //For RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL(225)

/* Variable Definitions */
static float px;
static float y[4] = {0}; //Historical points
static float snctable[91];

/* Function Definitions */
uint32_t gresample(const signed char input[RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL], float output[RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL], uint32_t size)
{
   uint32_t i;
   float s[4];
   int32_t xi;
   uint32_t index = 0; //We return the last index since it could be 146 or 147

   /*  Resample at arbitrary lower bit rate
       can be modified to return higher bit rate by using a while loop at the if
       px>3 and seeing whether the next sample is still within the same interval
       Algorithmic method is to keep track of 4 points, and keep track of the
       output sample as a fraction of the input sample. If the output does not
       land between stored inputs 2 and 3, then there is no output, otherwise
       there is an output which is interpolated using a cubic spline.
   */

   // Keeps track of the number of output samples produced (thereby index of "output" as well)
   for (i = 0; i < size; i++)
   {
      // New sample gets added to the end of the vector, oldest sample falls off the front
      y[0] = y[1];
      y[1] = y[2];
      y[2] = y[3];
      y[3] = (float)input[i]; //Cast the new sample to a float for calculations

      // Keep track of "input time". Time normalized by To. Normalized o/p time-instants are 0,1,2,...
      px--;
      if (px < 1.0F)
      {
         xi = (int32_t)(px * 45.0F + 1.0F);

         s[0] = snctable[ xi + 44];
         s[1] = snctable[ xi -  1];
         s[2] = snctable[-xi + 45];
         s[3] = snctable[-xi + 90];

         output[index++] = (((s[0]*y[0] + s[1]*y[1]) + s[2]*y[2]) + s[3]*y[3]) / (((s[0] + s[1]) + s[2]) + s[3]);

         px += INPUTS_PER_OUTPUT; //12.20703125/8, giving roughly 147.45 samples per 225
      }
   }//End for loop
   return(index);
}

void gresample_px_sinc_table_init(void)
{
   px = INPUTS_PER_OUTPUT;

   // Create a Sinc table on boot for the appropriate RESAMPLER_FACTOR
   // A pre-computed table stored in ROM instead of calculated on boot and put in RAM is actually
   //  slower than keeping the table in RAM. This spends 1.2% cpu time.

   // The re-sample factor is the stretch of the sinc function along the x-axis
   // snctable = sin(x/r)*(r/x), "r" being the resampling factor (30)
   snctable[0] = 1.0F;
   for (uint32_t xv = 1; xv < 91; xv++)
   {
      snctable[xv] = sinf((float)xv / SINC_STRETCH_FACTOR) * SINC_STRETCH_FACTOR /(float)xv;
   }
}