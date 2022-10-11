/******************************************************************************

   Filename: SyncAndPayloadDemodulator.c

   Global Designator:

   Contents:

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* Include files */
#include "project.h"
#include <string.h>
#include "SyncAndPayloadDemodulator.h"
#include "findSYNC.h"
#include "SoftDemodulator.h"
#include "processDerotatedSPDSamples.h"
#include "FreqDomMlseP4GfskStruct2.h"
#include "radio_hal.h" //RADIO_0 for radio number for above function

/* Function Definitions */
void SyncAndPayloadDemodulator(c_SyncAndPayloadDemodulatorPers *SPD,
                               const float filteredInput[150],
                               eSyncAndPayloadDemodFailCodes_t *FailCode)
{
   float Scaling2;
   float total;
   float ScldFreq;
   float phase;
   uint32_t i;
   uint32_t MNSE2;
   uint32_t lengthOfPayload;
   uint32_t numberOfPayloadBytes;
   uint8_t trellisPackedOutput[OUTPUT_ARRAY_SIZE] = {0};
   float scaleBuffer[FILTERED_SAMPLES_PER_EVENT];
   boolean_T headerCRCPassed;

   /* REVISION HISTORY
    *  1Aug18: Improve readability. Wrap after 150 columns. Spaces between arguments in func.. prototype.
    *  2Aug18: AGILENT_TRELBRCHMET_G, PKT_LEN_FIXED_G are set on per TV basis.
    *          FRQDOM_TREL.BrchFreqDecdOrder initialized for both Samwise, Agilent TXs.
    *  5Sep18: Minor changes. Added comments.
    * 21Sep18: FIXEDPKT renamed to PKT_LEN_FIXED_G.
    * 10Oct18: Lots of changes to reduce memory/MIPS. findSYNC(), FreqDomMlse4GfskStruct2().
    *          Trellis-decodermetric changes from Euclidean-distance to linear-distance in frequency-domain.
    *  1Nov18: findSYNC: Introduced break inside if (total < MinNumMMs) with reasoning explained below.
    *          Slicer Integrate-Dump offset & scale: offset is computed on SlicIntg2 just before SYNC.
    *          It is then removed first before the scale is computed. Since abs(ideal_freq - recvd_freq)
    *          is metric being minimized in Trellis-Decoder, we change the offset & scale computation
    *          below to use absolute amplitude of instantaneous-freq.
    */

   /*  These set on per TV basis. */
   *FailCode = eSPD_NO_ERROR;

   /*  # SlicIntg2 o/ps to gather before calling findSYNC().
    *  PHYheader 8 Bytes. Followed by 1st PLbyte x00. These 9 Bs (36 symbols) are o/p of MLSE1.
    *  # bits in SYNC = # symbols in SYNC since 1 sync-bit maps to 1 4GFSK-symbol.
    */

   Scaling2 = 6.28318548F * SPD->EstCFO / 38400.0F;
   total = 0.0F;

   /* LPF1_seg = single(zeros(1,length(filteredInput))); */
   for (i = 0; i < FILTERED_SAMPLES_PER_EVENT; i++)
   {
      ScldFreq = filteredInput[i] * SPD->ScaleValue;
      if (ScldFreq > 9000.0F) //If above upper limit
      {
         ScldFreq = 9000.0F;
      }
      else if (ScldFreq < -9000.0F) //else if below lower limit
      {
         ScldFreq = -9000.0F;
      }

      total += ScldFreq * 0.000163624616F;
      phase = (SPD->Last_iPhase + total) - (SPD->Last_PhaseRamp + Scaling2 * (float)i);
      ScldFreq = phase - SPD->Last_Phase;
      SPD->Last_Phase = phase;

      scaleBuffer[i] = ScldFreq * (((4800.0F * 8.0F) / 2.0F) / 3.14159274F);
   }

   SPD->Last_iPhase += total;
   SPD->Last_PhaseRamp += Scaling2 * 149.0F;

   // derotationInput2 = LPF1_seg;
   processDerotatedSPDSamples(&scaleBuffer[0],
                              SPD->SlicIntg2,
                              &SPD->SlicOutIdx,
                              SPD->samplesUnProcessedBuffer,
                              &SPD->samplesUnprocessed,
                              &SPD->ZCerrAcc);

   // Find SYNC_SYMS sequence in SlicIntg2 (I&D o/p with timing loop) allowing some mismatches
   if ((SPD->SyncFound == 0) && (SPD->SlicOutIdx > 200))
   {
      /* Atleast 200 o/ps from slicer produced */
      findSYNC(SPD->SlicIntg2,
               SPD->SlicOutIdx - 1,
               SPD->findSyncStartIndex,
               &MNSE2,
               &SPD->IdxSync2);

      /* sprintf('Found SYNC_SYMS at %d in SignSlicIntg2 with %d errors', IdxSync2, MNSE2) */
      if (MNSE2 > NUMBER_OF_SYNC_ERRORS_ALLOWED)
      {
         /* sprintf('DEMOD FAIL: findSYNC() gave Min # Sync Errors %d',MNSE2) */
         *FailCode = eSPD_SYNC_FAIL;

         /* reset of algorithm will happen below */
      }
      else
      {
         SPD->SyncFound = true;
         RADIO_SyncDetected( (uint8_t)RADIO_0, SPD->EstCFO );
         SPD->syncTime_msec = 0U;
         SPD->syncTime_msec = OS_TICK_Get_ElapsedMilliseconds();
      }

      /* To manually add the code to set the event flag to let the
      * preamble detector know that we are done processing the sync.
      * either success or fail. ( uncomment the line below in firmware) */
      OS_EVNT_Set(&SD_Events, (int32_t)SYNC_DEMOD_TASK_SYNC);
   }

   // Capture several RSSIs to be averaged later
   if (SPD->SyncFound != 0)
   {
      RADIO_UnBufferRSSI((uint8_t)RADIO_0);
   }

   /* NO optimizations have been done after this stage. There could be more
    * memory savings and run -time improvements after this.
    * If SYNC was found, ensure # o/ps from Slicer covers expected length of packet. */
   if ((SPD->SyncFound != 0) && (SPD->HeaderValid == 0) && ((SPD->SlicOutIdx - 1) > (SPD->IdxSync2 + 1 + 68))) // 68 is from 32 symbols for SYNC + 32 symbols for PHY + 4 symbols (one more byte)
   {
      SPD->MlseOffset = SPD->IdxSync2 + 32; // 32 is SYNC symbols

      /* PAYLOAD must immediately follow SYNC_SYMS. */
      /* CHECK: May require too many MIPS. */
      MNSE2 = (SPD->IdxSync2) & 0xFFFFFFFE; // Used to be -2 but generating lint warning
      if (MNSE2 > 64)
      {
         MNSE2 = 64;
      }

      // bitand function maps 30 to 28, 31 to 30, 32 to 30, 33 to 32, etc...
      // noffset = min(floor((IdxSync2-1)/2)*2,64); // This was the way we did it when IdxSync2 was single.
      ScldFreq = 0.0F;
      for (i = SPD->IdxSync2 - MNSE2; i < SPD->IdxSync2; i++)
      {
         // Since abs(ideal_freq - recvd_freq) is metric being minimized in Trellis-Decoder, we change
         ScldFreq += SPD->SlicIntg2[i];

         // the offset & scale computation below to use absolute amplitude of instantaneous-freq.
      }

      if ( MNSE2 == 0 ) {
         MNSE2 = 1; // Should not happen but make Lint happy
      }
      ScldFreq /= (float)MNSE2;

      // offset = Average of SlicIntg2 segment just before SYNC.
      phase = 0.0F;
      MNSE2 = 0;
      if ((int32_t)SPD->IdxSync2 - 16 < 0)
      {
         i = 0;
      }
      else
      {
         i = SPD->IdxSync2 - 16;
      }

      while (i < SPD->IdxSync2 + 32)
      {
         SPD->SlicIntg2[i] -= ScldFreq;

         /* Remove offset first before finding average-peak-absolute-freq-deviation. */
         if (SPD->SlicIntg2[i] > 0.0F)
         {
            phase += SPD->SlicIntg2[i];
         }
         else
         {
            /* We want absolute value of SlicIntg2 after removing offset. */
            phase -= SPD->SlicIntg2[i];
         }

         MNSE2++;
         i++;
      }

      if ( phase == 0.0F ) {
         phase = 1; // Should not happen but make Lint happy
      }
      ScldFreq = (float)MNSE2 / phase;

      /* Ampl is actually average of absolute-value of SlicIntg2 (after removing offset). */
      /* Note 1.08 below was determined empirically */
      ScldFreq      = 1.3664F * 1.0815F * ScldFreq;
      SPD->ScldFreq = ScldFreq;
      for (MNSE2 = 0; MNSE2 <= (SPD->MlseOffset+36); MNSE2++) // 36 is the PHY length in symbols + 4 symbols
      {
         SPD->SlicIntg2[MNSE2] = SPD->SlicIntg2[MNSE2] * ScldFreq;
      }

      OS_MUTEX_Lock(&SD_Payload_Mutex); // Not sure why we need this at this time

      /* 1st call to Freq-domain-MLSE (max likelihood sequence estimation) for PHYheader. */
      /* PHYheader processing: MLSE + Map Symbols to bits + Header RS-decode + 16-bit CRC check. */
      lengthOfPayload = 0U;

      /* coder.varsize('Sinput',[1 600],[0 1]); */
      /* SlicIntg2 = SlicIntg2(MlseOffset : (MlseOffset + N_SYMBS_HDR_PLUS_4)); */
      /* [RxPL3_1] = FreqDomMlse4GfskStruct2(Sinput, FRQDOM_TREL, InitStateIdx, FinalStateIdx); */
      /*  Trellis-decode 1st 72 bits in this call. Harmless to feed more symbols to MLSE. */

      /*  Map branch-sequence from 1st trellis-decoding to 4GFSK symbol-sequence. */
      /*  Mapping moved to inside the function */
      FreqDomMlseP4GfskStruct2(SPD->SlicIntg2,
                               SPD->MlseOffset,
                               36,
                               trellisPackedOutput);

      // The header length is always 8.
      /* FIX ME: Make this a global constant. Also the actual trellis output size could be more than 8,
       *         hence forcing to be 8 bytes */
      headerCRCPassed = ProcessSRFNHeaderPBytes(trellisPackedOutput, &lengthOfPayload);

      /* 2nd call to Freq-domain-MLSE (max likelihood sequence estimation) for PHYpayload.
       * PHYpayload processing: MLSE + Map Symbols to bits + Payload RS-decode + 32-bit CRC check.
       * Extra 4 since 1st PL-byte (x00) is demodulated again in 2nd trellis-decoder call.
       * Trellis-decoder ((150 -8)*8 + N_XTRA_SYM_2_MLSE*2) bits produced by this call.*/
      SPD->lengthOfPayload = (lengthOfPayload + 17U) << 2; // Length of (payload in bytes + FEC length (16 bytes) + 1) converted in symbols

      SPD->MlseOffset += 32; // Skip the 32 symbols in PHY header for when we decode payload

      OS_MUTEX_Unlock(&SD_Payload_Mutex);

      if (headerCRCPassed)
      {
         SPD->HeaderValid = 1;
      } else {
         *FailCode = eSPD_HDR_CRC_FAIL; // Failcode and exit
      }
   }

   // Demodulate Payload if enough symbols were received
   if ((SPD->SyncFound != 0) && (SPD->HeaderValid != 0) && (SPD->SlicOutIdx - 1 > (SPD->MlseOffset + SPD->lengthOfPayload)) )
   {
      // Scale input
      // We add 4+1 to the starting index because the 4 symbols following the PHY header were already filtered when the PHY header was decoded so we need to go pass 1 (i.e. +1).
      for (MNSE2 = SPD->MlseOffset+4+1; MNSE2 <= SPD->MlseOffset + SPD->lengthOfPayload; MNSE2++)
      {
         SPD->SlicIntg2[MNSE2] = SPD->SlicIntg2[MNSE2] * SPD->ScldFreq;
      }

      OS_MUTEX_Lock(&SD_Payload_Mutex); // Not sure why we need this at this time

      /* 16*4--128 FEC bytes, 4--terminator byte, lenOfPay--Payload bytes.
       * Sinput = SlicIntg2((MlseOffset + N_SYMBS_HDR_PLUS_4 -4) : (MlseOffset + N_SYMBS_HDR_PLUS_4 -4 + int32(symbolsInPayload)));
       * Map branch-sequence from 2nd trellis-decoding to 4GFSK symbol-sequence.
       * Mapping moved to inside the function */
      FreqDomMlseP4GfskStruct2(SPD->SlicIntg2,
                               SPD->MlseOffset,
                               SPD->lengthOfPayload,
                               trellisPackedOutput);

      // coder.cstructname(radioParams.syncTimeStamp,'TIMESTAMP_t','extern','HeaderFile','PHY_Protocol.h');
      numberOfPayloadBytes = SPD->lengthOfPayload >> 2;
      SPD->lengthOfPayload -= numberOfPayloadBytes << 2;
      if ((SPD->lengthOfPayload > 0U) && (SPD->lengthOfPayload >= 2U))
      {
         numberOfPayloadBytes++;
      }

      (void)ProcessSRFNPayloadPBytes(trellisPackedOutput, (unsigned short)numberOfPayloadBytes-1); // -1 because we added one more byte to the length for the previous trellis
      /* Always return a fail code so that the task exits. If the CRC passes, the firmware will
       * internally post the payload to the higher layers within this function call. */
      *FailCode = eSPD_SRFN_PAYLOAD_POSTED;

      OS_MUTEX_Unlock(&SD_Payload_Mutex);
   }
}