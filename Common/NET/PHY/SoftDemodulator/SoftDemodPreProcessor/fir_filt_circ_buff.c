/******************************************************************************

   Filename: fir_filt_circ_buff.c

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
#include "SoftDemodulator.h" //NUMBER_OF_FILTER_TAPS
#include "fir_filt_circ_buff.h"

// Glenn's 16 tap filter
//static const float fv0[NUMBER_OF_FILTER_TAPS] = {
//   -0.113778137007535321001405748120305361226F,
//    0.028920453787619596758284856719001254532F,
//    0.04435188655716626460190354919177480042F,
//    0.067465914299033757917101183920749463141F,
//    0.093936416524198262933964542753528803587F,
//    0.119152017746333840908512513578898506239F,
//    0.138804830578779764271857288804312702268F,
//    0.149547222127310197592464646731968969107F,
//    0.149547222127310197592464646731968969107F,
//    0.138804830578779764271857288804312702268F,
//    0.119152017746333840908512513578898506239F,
//    0.093936416524198262933964542753528803587F,
//    0.067465914299033757917101183920749463141F,
//    0.04435188655716626460190354919177480042F,
//    0.028920453787619596758284856719001254532F,
//   -0.113778137007535321001405748120305361226F,
//};

// Glenn's 32 tap filter
static const float fv0[NUMBER_OF_FILTER_TAPS] = {
   0.008366217854052825433908147090278362157F,
   0.003393371363463204039290221913915956975F,
   0.001124654016272863370218271228395678918F,
  -0.003540588090483744523884190869011945324F,
  -0.009971414047386363554981159040835336782F,
  -0.016640786588339603890851492451474769041F,
  -0.02126600397580917164996883172989328159F,
  -0.021254861771973585643502602238186227623F,
  -0.014309491460495021158960682328142866027F,
   0.000881590873135572978755436501785425207F,
   0.024109255784799957234021405838575446978F,
   0.053404371927395548336203745520833763294F,
   0.085239526894063527562295234929479192942F,
   0.115048574262098454723535212451679399237F,
   0.138186282216318290227974330264260061085F,
   0.150825130651355249211675868536985944957F,
   0.150825130651355249211675868536985944957F,
   0.138186282216318290227974330264260061085F,
   0.115048574262098454723535212451679399237F,
   0.085239526894063527562295234929479192942F,
   0.053404371927395548336203745520833763294F,
   0.024109255784799957234021405838575446978F,
   0.000881590873135572978755436501785425207F,
  -0.014309491460495021158960682328142866027F,
  -0.021254861771973585643502602238186227623F,
  -0.02126600397580917164996883172989328159F,
  -0.016640786588339603890851492451474769041F,
  -0.009971414047386363554981159040835336782F,
  -0.003540588090483744523884190869011945324F,
   0.001124654016272863370218271228395678918F,
   0.003393371363463204039290221913915956975F,
   0.008366217854052825433908147090278362157F,
};

// Hari's Original 32 tap filter
//static const float fv0[NUMBER_OF_FILTER_TAPS] = { -0.000507108F, 0.000605885463F, 0.00223462912F,
//    0.00413205754F, 0.00498966F, 0.00277610635F, -0.00393256499F, -0.0142335566F,
//    -0.0238801595F, -0.0259325F, -0.0132654961F, 0.0179379825F, 0.0653379038F,
//    0.119829193F, 0.167867377F, 0.196040586F, 0.196040586F, 0.167867377F,
//    0.119829193F, 0.0653379038F, 0.0179379825F, -0.0132654961F, -0.0259325F,
//    -0.0238801595F, -0.0142335566F, -0.00393256499F, 0.00277610635F, 0.00498966F,
//    0.00413205754F, 0.00223462912F, 0.000605885463F, -0.000507108F };

// Glenns's Original 8 Tap Filter
//static const float fv0[NUMBER_OF_FILTER_TAPS] = {
//   0.0594981541339616F,
//   0.109119138309226F,
//   0.152863784541088F,
//   0.178518923015724F,
//   0.178518923015724F,
//   0.152863784541088F,
//   0.109119138309226F,
//   0.0594981541339616F};

// Glenn's Updated 32 Tap Filter
//static const float fv0[NUMBER_OF_FILTER_TAPS] = {
// 0.0F, 0.0F, 0.0F, 0.0F,
// 0.0F, 0.0F, 0.0F, 0.0F,
//-0.11202603224481136323653629460750380531F,
// 0.032015195318300306015490974687054404058F,
// 0.046520666499388733938769036058147321455F,
// 0.068486047518523676669310873421636642888F,
// 0.093646011712958956985097813685570145026F,
// 0.117571725940816246280817836122878361493F,
// 0.136186822244374006185552161696250550449F,
// 0.146350126902718036259187783798552118242F,
// 0.146350126902718036259187783798552118242F,
// 0.136186822244374006185552161696250550449F,
// 0.117571725940816246280817836122878361493F,
// 0.093646011712958956985097813685570145026F,
// 0.068486047518523676669310873421636642888F,
// 0.046520666499388733938769036058147321455F,
// 0.032015195318300306015490974687054404058F,
//-0.11202603224481136323653629460750380531F,
// 0.0F, 0.0F, 0.0F, 0.0F,
// 0.0F, 0.0F, 0.0F, 0.0F
//};

/* Variable Definitions */
static uint32_t multiple_of_FSPE;

/* Function Definitions */
boolean_T fir_filt_circ_buff(unProcessedData_t *unProcessedData)
{
   bool samplesAvailable = false;
   float acc;
   uint32_t n;
   uint32_t j;
   float *src;
   float *dest;
   //float const *tap;

   /* PURPOSE: fixed-point FIR filter with persistent variable for buffer, buffer_pointer
      INPUTs: unProcessedData = filter input (struct unProcessedData_t)
      OUTPUTs: filteredPhaseSamplesBuffer (filtered output)
      REVISION HISTORY:
      9Nov17: Initial version replaces in-line-code in DemodPhaseSeqFromIC6_fw.m
      6/17/19: Optimized for loops, reduced variables, changed to native types,
               changed input data structure, reducing CPU load from 18.3% to 8.9%

      To-Do : This module can be substituted with arm-library CMSIS optimized
      code. Need to get the function syntax and adjust the arguments if
      necessary. If embedded coder license is available, then need to figure
      out how to do code substitutions, it not manually call the function.
      FirFiltBuff, ptrBuff variables maintain state-of-filter from call to call */

   for (n = 0; n < unProcessedData->size; n++)
   {
      acc = 0.0F;
      src = &unProcessedData->history[n]; //Start processing the first 31 historical values and then the 1st
                                          // recent value. As this slides it will move out of the historical
                                          // data completely.
      for (j = 0; j < NUMBER_OF_FILTER_TAPS; j++)
      {
         acc += fv0[j] * *src++;
         /*Manually unrolling this provides no benefit over the current compiler
           optimization (high optimization for speed)*/
      }

      filteredPhaseSamplesBuffer[filteredPhaseSamplesBufferWriteIndex++] = acc;

      if (filteredPhaseSamplesBufferWriteIndex >= multiple_of_FSPE)
      {
         if (filteredPhaseSamplesBufferWriteIndex >= FILTERED_PHASE_SAMPLES_BUFFER_SIZE)
         {
            filteredPhaseSamplesBufferWriteIndex = 0;
            multiple_of_FSPE = 0; //Reset the value
         }
         multiple_of_FSPE += FILTERED_SAMPLES_PER_EVENT; //Increment the value
         samplesAvailable = true;
      }
   }

   //Once the filter has been applied to everything, move the last 31 samples into the historical buffer
   //Doing this manually is faster than calling memcpy
   dest = &unProcessedData->history[0];
   src = &unProcessedData->data[(unProcessedData->size - NUMBER_OF_FILTER_TAPS) + 1];

   n = NUMBER_OF_FILTER_TAPS-1; //unrolls to do 1 instruction, then 10 loops of 3
   while(n > 0u)
   {
      *dest++ = *src++;
      n--;
   }
   /* Manually unrolling the loop by 4 or 8 doesn't seem to give any benefit over
      the above loop. Since the compiler unrolls it anyways (with the current
      optimization for speed set to high), manually unrolling doesn't do much. So
      for the sake of flexibility the above code is used (in case the number of
      taps changes), assuming the compiler does some unrolling.*/

   return samplesAvailable;
}

void fir_filt_circ_buff_init(void)
{
   multiple_of_FSPE = FILTERED_SAMPLES_PER_EVENT;
}