/******************************************************************************

   Filename: FreqDomMlseP4GfskStruct2.c

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

/*  PURPOSE:
 *        Uses Trellis structure similar to poly2trellis() for frequency-domain MLSE decoding.
 *   Does trellis decoding. Only single traceback is done. So RX should not be very long (say > 2e3).
 *   See comments in gmsk4_phaseseq_rcvr_freq_discr_timing() for how func. was verified.
 *  INPUT:
 *    RX              = The received sequence
 *    FRQDOM_TREL     = The trellis structure build in gmsk4_phaseseq_rcvr_freq_discr_timing()
 *    KnownInitState  = -1 indicates initial-state unknown. Otherwise, it must valid initial-state
 *                      whose metric is initialized to 0; while metric of other states is set to large +ve.
 *    KnownTermState  = -1 indicates final-state unknown. Otherwise, traceback is always started from
 *                      from this state irrespective of which state had lowest metric.
 *  OUTPUT:
 *    DecdSymb        = Decoded bit stream.
 *    FinalStateSeq   = State sequence followed by path having best metric (smallest Euclidean distance).
 *    prevMetric      = Final value of state metrics
 *
 * REVISION HISTORY:
 *  Sep2016: Verified 2-GFSK with 8-state trellis for BT .5. Then, realized that 4-state trellis is
 *           sufficient for BT .5.
 *  Oct2016: Since MIPS/memory requirements are much easier for 4-state trellis than 8-state trellis,
 *           modified BUILD_TREL = 1 portion of code and verified that 4-state trellis also works.
 *  Nov2016: With self-timing enabled in modem, verified that "KnownInitState" works when modem
 *           properly recognizes the sample corresponding to say last 2 known bits of SYNC. Then,
 *           only branches emanating from this known initial state are considered at start of trellis
 *           algorithm. Also introduced "KnownTermState" and verified. Use ONLY when TX sequence is
 *           terminated with known symbols (preferably -1, -1, ... corresponding to bits 0, 0, ...).
 *  23Nov16: If KnownInitState ~= -1 then, KnownInitState specifies a known initial trellis state
 *           whose metric is set to 0, whereas metrics of other states are initialized to very large
 *           numbers. If KnownInitState == -1, no initialization is done.
 *  23Nov16: If KnownTermState ~= -1 then, KnownTermState specifies a known final trellis state from
 *           which trellis traceback is begun no matter which state has the best minimum metric at
 *           end of trellis. If KnownTermState == -1, trellis traceback is begun from the state with
 *           minimum metric.
 *  7Sep17:  Untill now, decoding relied on nextStates, brchIdx and time-consuming
 *           "LinIdxOfState = find(FRQDOM_TREL_nextStates == termState);"  With new pre-computed
 *           variables {prevStates, prevSymbols, prevStatesBrch} the decoding is quicker. For each
 *           destination state, the 4 previous-states, the corresponding branch-4GFSK-symbols,
 *           the corresponding branch-metrics are stored in order required during decoding.
 *  14Sep17: Traceback memory halved. FEA time, each state stores index-of-surviving-branch. Knowing
 *           this and index-of-state the prevState, prevStateSymbol can be gotten from
 *           FRQDOM_TREL {.prevSymbols, .prevStates}. 'Old' method stored 2 arrays.
 *  12Oct17: Replace NaN (Not-a-Number) arrays with -1 arrays. Verified no perf-loss over 6 TVs.
 *  10Oct18: New arguments {StartIndex, Length} reduces data-memory footprint of caller. Changed
 *           trellis metric from Euclidean-distance to linear distance with abs since we are not in
 *           cartesian domain. Reduces MIPS also.
 *   2Jul19: Packed 2-bit traceback array data to reduce memory consumption from 9600B to 2400B
 *           Modified counters and for loops to count in ranges (0,15) and (0,3) instead of weird
 *           ranges like (-1,2).
 *
 *  Function code below
 *  Meaning of {nInSymb, nOutSymb, prevStates, prevSymbols, BrchFreqDecdOrder} (variables inside
 *  trellis structure)
 *   - FRQDOM_TREL_nInSymb  = The # of distinct i/p bit-vectors that drive a transition from a state.
 *   - FRQDOM_TREL_nOutSymb = The total # of branches emanating from all the states. Since every state
 *                has same # of emanating branches, nOutSymb = #states x #emanating-branches-per-state.
 *   - In prevStates matrix, each row has 4 entries; an integer in [0, nStates -1].
 *     prevStates(i, j) = index of prevState for destState(i-1), 4GFSK-symbol-index(j-1).
 *   - In prevSymbols matrix, each row has 4 entries; an integer in [0, nInSymb -1].
 *     prevSymbols(i,j) = 4GFSK-symbol-index for branch from prevStates(i, j) to destState(i-1).
 *
 *  Rearrange brchFreq for Viterbi decoding; Given destination-state-i the 4 metrics of 4 branches from
 *     the originating states are in each row-i. Columns 1,2,3,4 correspond to SymbIndex = 0,1,2,3.
 *     brchFreq rearranged into BrchFreqDecdOrder is used by FreqDomMlse4GfskSova().
 */


/*  Trellis-decoding: distance between instantaneous-received-freq and instantaneous-transmit-freq
 *      will be minimized.
 *  Map Branch-Index { 3,  2,  0,  1} to Symbol {-3, -1,  1,  3} respectively.
 *                    11  10  01  00             11  10  01  00
 */

/*  nBran = length(RX); // Since each branch-label has log2(numOutputSymbols) bits.
 *  Since Matlab x[a:b] means (b -a + 1) elements.
 *  current-metric = best among all previous-metric + branch-metric.
 */

/* Include files */
#include "project.h"
#include <math.h> // roundf, fabsf
#include <string.h>
#include <stdint.h>
#include "PHY_Protocol.h"
#include "FreqDomMlseP4GfskStruct2.h"

void FreqDomMlseP4GfskStruct2(const float RX[1024],
                              uint32_t StartIndex,
                              uint32_t Length,
                              unsigned char DecdPSymb_data[OUTPUT_ARRAY_SIZE])
{
   float prevMetric[16] = {0};
   float currMetric[16];
   float ex;
   float MetMtxRearranged; //temp
   float candMetrics[4];

   uint32_t loop_ub;
   uint32_t time;
   uint32_t PIndex;
   uint32_t k;

   uint32_t TBAryPrevIndx_data[OUTPUT_ARRAY_SIZE*4] = {0}; //Must be initialized as 0 for the |= data insertion
   uint8_t  BitShift;
   int8_t   TBstate;

   static const float FRQDOM_TREL_BrchFreqDecdOrder[64] = {
                0.7965F,  1.0123F,  0.5559F,  0.3688F,  1.9905F,  2.1826F,  1.7606F,  1.5218F,
                -0.352F,  -0.175F,  -0.631F, -0.7849F, -1.5543F, -1.3256F,  -1.759F,  -1.993F,
                1.0307F,  1.2133F,  0.7995F,  0.5514F,  2.1771F,  2.4685F,  1.9386F,  1.7543F,
               -0.1801F,  0.0166F, -0.3725F, -0.6264F, -1.3525F,  -1.114F,  -1.561F, -1.7499F,
                0.5889F,  0.8265F,  0.3924F,  0.1944F,  1.7942F,  1.9979F,  1.5969F,   1.366F,
               -0.5673F, -0.3914F, -0.7648F, -1.0134F, -1.7647F, -1.5568F, -1.9867F,  -2.154F,
                0.3516F,  0.5521F,  0.1547F, -0.0484F,  1.5455F,  1.7439F,  1.3242F,  1.1478F,
               -0.7832F, -0.5736F, -0.9988F, -1.1834F, -1.9596F, -1.7568F,  -2.191F, -2.3689F };

   // row-i holds winning index; index is used to derive prevState, prevSymbol.
   for (time = 0; time <= Length; time++)
   {
      /* MetMtxRearranged = (FRQDOM_TREL_BrchFreqDecdOrder - RX(time+StartIndex-1)).^2; // Euclidean-distance works in Cartesian coordinates domain. */
      /* MetMtxRearranged = abs(FRQDOM_TREL_BrchFreqDecdOrder - RX(time+StartIndex-1)); // Use linear-distance in frequency-domain. */
      // Atleast 1 and usually more branches must have MetMtxRearranged entries = 0 for high SNR.
      for (PIndex = 0; PIndex < 16; PIndex++)
      {
         // The 4 originStates contributing branches to termState are in prevStates.
         // col-j corresponds to 4GFSK-symbol-index(j-1).
         // brchFreq is rearranged by caller() to have branch-metric is order required for decoding.
         for (loop_ub = 0; loop_ub < 4; loop_ub++)
         {
            MetMtxRearranged = fabsf(FRQDOM_TREL_BrchFreqDecdOrder[(loop_ub << 4) + PIndex] - RX[(time + StartIndex) - 1]);
            candMetrics[loop_ub] = prevMetric[((loop_ub << 4) + PIndex) >> 2] + MetMtxRearranged;
         }

         // Minimize difference betw Received-instantaneous-freq and Transmitted-instantaneous-freq.
         ex = candMetrics[0];
         loop_ub = 0;
         for (k = 1; k < 4; k++)
         {
            if (ex > candMetrics[k])
            {
               ex = candMetrics[k];
               loop_ub = k;
            }
         }

         currMetric[PIndex] = ex;
         TBAryPrevIndx_data[time] |= (loop_ub)<<(2*PIndex);
      }

      memcpy(prevMetric, currMetric, sizeof(prevMetric));

      // Prepare next-in-time stage of trellis.
      // Matlab will ERROR if all locs of currMetric are not written in next iteration.
   }

   BitShift = 0U;
   PIndex = (uint32_t)roundf((float)Length / 4.0F) - 1;
   loop_ub = (uint32_t)roundf((float)Length / 4.0F);
   if (0 < loop_ub)
   {
      memset(&DecdPSymb_data[0], 0, (uint32_t)(loop_ub * (int)sizeof(unsigned char)));
   }

   // Traceback
   ex = prevMetric[0];
   loop_ub = 0;
   for (k = 1; k < 16; k++)
   {
      if (ex > prevMetric[k])
      {
         ex = prevMetric[k];
         loop_ub = k;
      }
   }

   // Start Traceback from state with minimum-metric.
   TBstate = (int8_t)(loop_ub);
   TBstate = (TBstate + (((TBAryPrevIndx_data[Length] >> (2*TBstate)) & 0x3) << 4)) >> 2;/*lint !e737  Loss of sign in promotion from int to unsigned int */

   // At high-SNR, FinalMinPrevMetric must be very small
   for (time = Length; time > 0; time--)
   {
      // New method stores only 1 array
      // Best previous index
      //DecdPSymb_data[PIndex] |= ((uint8_t)(TBstate + ((TBAryPrevIndx_data[TBstate + (time << 4)]) << 4)) & 0x3)<< BitShift;
      DecdPSymb_data[PIndex] |= (uint8_t)(((uint8_t)(TBstate) & 0x3) << BitShift);/*lint !e701  Shift left of signed quantity (int) */
      if (BitShift < 6)
      {
         BitShift += 2;
      }
      else
      {
         BitShift = 0U;
         PIndex--;
      }

      TBstate = (TBstate + (((TBAryPrevIndx_data[time-1] >> (2*TBstate)) & 0x3) << 4)) >> 2;/*lint !e737 Loss of sign in promotion from int to unsigned int*/
   }
}