/******************************************************************************

   Filename: SoftDemodulator.c

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

#include "project.h"
#include <assert.h>
#if( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#endif
#include <stdint.h>

#include "compiler_types.h"
#include "radio_hal.h"
#include "BSP_aclara.h"
#include "si446x_api_lib.h"

#include "DBG_SerialDebug.h"
#include "SoftDemodulator.h"

#include "phase2freq_wi_unwrap.h"
#include "gresample.h"
#include "fir_filt_circ_buff.h"

#include "preambleDetector_initialize.h"
#include "preambleDetector_data.h"
#include "preambleDetector.h"

#include "SyncAndPayloadDemodulator_types.h"
#include "SyncAndPayloadDemodulator.h"

#include "time_sys.h"
#include "time_util.h"
#if (PHASE_DETECTION == 1)
#include "PhaseDetect.h"
#endif

#define SYNC_PAYLOAD_UNBLOCK_EVENT 0x01

#define SYNC_THRESHOLD 1500 // How high SYNC correlation needs to be to declare a valid SYNC when summing 11 values

#define OSR 12.207f // Radio oversampling rate: 30MHz/512/4800

#define OFFSET(index) ((uint32_t)(((float)index*OSR)+0.5f)) // Make an offset based on sampling rate

static bool SD_Unblock_Sync_Payload_Task(void);
static void SD_ReleaseFilteredSamplesSemaphore(uint8_t id);
static bool SD_FindSync( int8_t *buf, uint32_t size, uint32_t intFIFOCYCCNTTimeStamp );

OS_EVNT_Obj  SD_Events;
OS_MUTEX_Obj SD_Payload_Mutex;

//static OS_SEM_Obj SD_RawPhaseSamples_Sem; // Semaphore to signal the soft demod preprocessor task to process the samples
static OS_MUTEX_Obj SD_FilteredPhaseSamples_Mutex;
static filteredPhaseSamples_sem_t SD_FilteredPhaseSamplesSemaphores[SOFT_DEMOD_MAX_SYNC_PAYL_TASKS + 1]; // +1 for preamble detector task

static int8_t   rawPhaseSamplesBuffer[RAW_PHASE_SAMPLES_BUFFER_SIZE];
       float    filteredPhaseSamplesBuffer[FILTERED_PHASE_SAMPLES_BUFFER_SIZE] = {0};
       uint16_t filteredPhaseSamplesBufferWriteIndex = 0;
static uint16_t preambleDetectorReadIndex = 0;

returnStatus_t SoftDemodulator_Initialize(void)
{
   returnStatus_t retval = eSUCCESS;

   if (OS_EVNT_Create(&SD_Events) == false)
   {
      retval = eFAILURE;
   }

   for (uint8_t i=0;i<SOFT_DEMOD_MAX_SYNC_PAYL_TASKS+1;i++)
   {
      //TODO RA6: NRJ: determine if semaphores need to be counting
      if (retval == eSUCCESS && (OS_SEM_Create(&SD_FilteredPhaseSamplesSemaphores[i].semaphore, 0) == false) ||
                                (OS_EVNT_Create(&SD_FilteredPhaseSamplesSemaphores[i].sync_payload_event) == false))
      {
         retval = eFAILURE;
         break;
      }
      SD_FilteredPhaseSamplesSemaphores[i].available = true;
   }

   if (retval == eSUCCESS && OS_MUTEX_Create(&SD_FilteredPhaseSamples_Mutex) == false)
   {
      retval = eFAILURE;
   }

   if (retval == eSUCCESS && OS_MUTEX_Create(&SD_Payload_Mutex) == false)
   {
      retval = eFAILURE;
   }

   if (retval == eSUCCESS)
   {
      //Initialize the Soft Demodulator Pre-Processor
      gresample_px_sinc_table_init();
      phase2freq_wi_unwrap_init();
      fir_filt_circ_buff_init();
   }

   return retval;
}

static bool SD_Unblock_Sync_Payload_Task(void)
{
   bool unblocked = false;
   OS_MUTEX_Lock(&SD_FilteredPhaseSamples_Mutex);

   //Start at 1 to skip Task 0 (Preamble Detector)
   for(uint8_t i=1; i<SOFT_DEMOD_MAX_SYNC_PAYL_TASKS+1; i++)
   {
      if (SD_FilteredPhaseSamplesSemaphores[i].available == true)
      {
         SD_FilteredPhaseSamplesSemaphores[i].available = false;
         OS_SEM_Reset(&SD_FilteredPhaseSamplesSemaphores[i].semaphore);
         OS_EVNT_Set(&SD_FilteredPhaseSamplesSemaphores[i].sync_payload_event, SYNC_PAYLOAD_UNBLOCK_EVENT);
         unblocked = true;
         break;
      }
   }

   OS_MUTEX_Unlock(&SD_FilteredPhaseSamples_Mutex);
   return unblocked;
}

static void SD_ReleaseFilteredSamplesSemaphore(uint8_t id)
{
   OS_MUTEX_Lock(&SD_FilteredPhaseSamples_Mutex);
   SD_FilteredPhaseSamplesSemaphores[id].available = true;
   OS_SEM_Reset(&SD_FilteredPhaseSamplesSemaphores[id].semaphore);
   OS_MUTEX_Unlock(&SD_FilteredPhaseSamples_Mutex);
}

/*****************************************************************************************************************

   Function name: SD_FindSync

   Purpose: Detect SYNC through correlation.

   Arguments: uint8_t  *buf: vector of raw samples
              uint32_t size: length of buf vector
              uint32_t intFIFOCYCCNTTimeStamp: when the radio FIFO interrupt happened

   Returns: True if SYNC was detected

   Side effects: None

   Reentrant: YES

  *****************************************************************************************************************/
static bool SD_FindSync( int8_t *buf, uint32_t size, uint32_t intFIFOCYCCNTTimeStamp )
{
   (void)size; // This might be used later but is not needed now
   uint32_t i; // loop counter
   int8_t *ptrBuf;
   static int32_t sumVec[2*MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT] = {0}; // Holds the result of the correlation
   int32_t peakValue[3] = {0};
   int32_t *ptrSumVec;
   bool    syncDetected = (bool)false;
//   static int32_t debugVec[MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT];
   uint32_t cpuFreq;
   uint32_t delay;
   uint32_t syncTimeCYCCNT; // SYNC time stamp in CYCCNT units
   TIMESTAMP_t syncTime;

   // Copy old data to the beginning of the buffer to leave space for new data
   memcpy(sumVec, &sumVec[MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT], sizeof(sumVec)-(sizeof(sumVec[0])*MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT));

   // Compute SYNC correlation
   for (i=0; i<MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT; i++ ) {
      ptrBuf = &buf[i+MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT];

      // This is the SYNC vector for a 12.207 OSR
      //  3, -3, -3, -3,  3, -3, -3,  3,
      // -3,  3, -3, -3, -3,  3, -3, -3,
      // -3,  3, -3,  3,  3, -3,  3,  3,
      //  3,  3, -3, -3, -3, -3, -3,  3
      sumVec[i+MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT] =
         (int32_t)ptrBuf[OFFSET( 0)] -(int32_t)ptrBuf[OFFSET( 1)] -(int32_t)ptrBuf[OFFSET( 2)] -(int32_t)ptrBuf[OFFSET( 3)] +(int32_t)ptrBuf[OFFSET( 4)] -(int32_t)ptrBuf[OFFSET( 5)] -(int32_t)ptrBuf[OFFSET( 6)] +(int32_t)ptrBuf[OFFSET( 7)]//lint !e834
        -(int32_t)ptrBuf[OFFSET( 8)] +(int32_t)ptrBuf[OFFSET( 9)] -(int32_t)ptrBuf[OFFSET(10)] -(int32_t)ptrBuf[OFFSET(11)] -(int32_t)ptrBuf[OFFSET(12)] +(int32_t)ptrBuf[OFFSET(13)] -(int32_t)ptrBuf[OFFSET(14)] -(int32_t)ptrBuf[OFFSET(15)]//lint !e834
        -(int32_t)ptrBuf[OFFSET(16)] +(int32_t)ptrBuf[OFFSET(17)] -(int32_t)ptrBuf[OFFSET(18)] +(int32_t)ptrBuf[OFFSET(19)] +(int32_t)ptrBuf[OFFSET(20)] -(int32_t)ptrBuf[OFFSET(21)] +(int32_t)ptrBuf[OFFSET(22)] +(int32_t)ptrBuf[OFFSET(23)]//lint !e834
        +(int32_t)ptrBuf[OFFSET(24)] +(int32_t)ptrBuf[OFFSET(25)] -(int32_t)ptrBuf[OFFSET(26)] -(int32_t)ptrBuf[OFFSET(27)] -(int32_t)ptrBuf[OFFSET(28)] -(int32_t)ptrBuf[OFFSET(29)] -(int32_t)ptrBuf[OFFSET(30)] +(int32_t)ptrBuf[OFFSET(31)];//lint !e834
   }

   // Find if a SYNC exists.
   // A SYNC exists if the summation of 11 values is above SYNC_THRESHOLD
   // The SYNC location is where the highest value is.

   // Pre-compute some values to make next loop faster
   ptrSumVec = &sumVec[MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT-11];
   for (i=0; i<11; i++ ) {
      peakValue[0] += ptrSumVec[i];
   }

   // Compute the sum of 11 values and check for SYNC presence
   ptrSumVec = &sumVec[MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT];
   for (i=0; i<MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT; i++ ) {
      // Remove oldest element
      peakValue[0] -= ptrSumVec[-11];
      // Add newest element and move pointer
      peakValue[0] += *ptrSumVec++;

      // Check if we have a SYNC
      if ((peakValue[2] > SYNC_THRESHOLD) &&
          (peakValue[1] > SYNC_THRESHOLD) &&
          (peakValue[0] > SYNC_THRESHOLD) &&
          (peakValue[0] < peakValue[1])) // This means we just pass the largest correlation value
      {
         (void)TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL );

         OS_INT_disable( ); // Disable all interrupts. This section is time critical when we try to be deterministic.

         syncDetected = (bool)true;

         // Compute when the SYNC was transmitted (i.e. last symbol) in cycle counter
//         delay = (uint32_t)((double)((size-MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT)-(i+OFFSET(31)))/(12.207f*4800)*cpuFreq); // This doesn't work but should have. Why?
         delay = (uint32_t)(((double)(MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT-i)/(12.207*4800))*cpuFreq); // Best but value is off

         syncTimeCYCCNT = intFIFOCYCCNTTimeStamp - delay; // Remove delay from end of buffer

         // Fudge factor is to remove the equivalent of 47 symbols. Don't know why (yet) but that was observed.
         syncTimeCYCCNT -= (uint32_t)((float)cpuFreq*0.000802f);

         OS_INT_enable( ); // Enable interrupts.

         (void)TIME_UTIL_GetTimeInSecondsFormat(&syncTime);
         RADIO_Set_SyncTime((uint8_t)0, syncTime, syncTimeCYCCNT);
#if ( PHASE_DETECTION == 1 )
         PD_SyncDetected(syncTime, syncTimeCYCCNT);
#endif
         // Zero out the curent vector
         // This is to avoid detecting the SYNC a second time
         // This happens when most of the current SYNC was at the end of the vector but part of the tail end of the SYNC will be in the next vector
         memset(&sumVec[MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT], 0, sizeof(sumVec[0])*MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT);
         //INFO_printf("SYNC corr detect %u %u %u %u", i, peakValue[2], peakValue[1], peakValue[0]);

         break; // There won't be another SYNC for a while so get out
      }
      peakValue[2] = peakValue[1];
      peakValue[1] = peakValue[0];

//      debugVec[i] = peakValue;
   }
#if 0
   if (syncDetected) {
      for (i=0; i<75; i+=5) {
         INFO_printf("SYNC %d %d %d %d %d", debugVec[i], debugVec[i+1], debugVec[i+2], debugVec[i+3], debugVec[i+4]);
      }
   }
#endif
   return syncDetected;
}


/* The Soft Demodulator phase samples listener task
   This task is triggered when the si4467 issues an interrupt signal. If the interrupt is a FIFO_ALLMOST_FULL interrupt,
   this task is then responsible to clear this interrupt and copies the FIFO Rx Buffer data into the Phase samples Buffer. The
   current threshold value that is configured is 75 samples. Upon writing data to the buffer, this task signals the preprocessor
   task to process the newly read phase samples.
   This task also is responsible to get an RSSI measurement if requested by the Sync & Payload Demodulator task. The RSSI
   measurement command is sent to the radio right after data is read from the radio buffer.
   Being called 781 times/sec, this function uses 10.2% of CPU Usage
*/

//static int8_t freqSeg_data[RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL] = {0};
static unProcessedData_t unProcessedData = {0};
static bool samplesAvailable = false;
static int8_t phase2FreqBuf[8*MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT]; // This should be big enough for a full SYNC i.e. 12.207samples/sym*32sym/sync=390.6 samples/sync
                                                                           // plus some extra (padding) because of the way sync detection works (i.e.avoid out of bound access)
void SD_PhaseSamplesListenerTask(taskParameter)
{
   #if ( RTOS_SELECTION == MQX_RTOS )
   (void)   Arg0;
   #endif

   for (;;)
   {
      (void)OS_EVNT_Wait(&SD_Events, SOFT_DEMOD_RAW_PHASE_SAMPLES_AVAILABLE_EVENT, (bool)false, OS_WAIT_FOREVER);

//      DBG_LW_printf("\n!2!");
      // Get the interrupt status. If it is the rx buffer almost full then we
      // process the interrupt here, else we let the PHY task process it.

      // Before optimization: 15.2%
      // After  optimization: 10.2%

      // This call takes 2641 cycles/call*781.2/120M = 1.7%
      (void)si446x_get_int_status_fast_clear((uint8_t)RADIO_0);

      // This call takes 11485 cycles/call*781.2/120M = 7.5%
      // Time spent just for actual SPI read: 4800(symbols/sec)*12.207(samples/symbol)*8() / 10(Mbit/sec) = 4.7%
      // Reading the data alone at max speed takes 4.7%, so that leaves ~2.8% (at most) for possible improvement
      si446x_read_rx_fifo((uint8_t)RADIO_0, MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT, (uint8_t *)&rawPhaseSamplesBuffer[0]);
      RADIO_RX_WatchdogService((uint8_t)RADIO_0); //Service the watchdog

      // phase2FreqBuf is a sliding buffer so move old phase data to the front to leave space for new phase data
      memcpy(phase2FreqBuf, &phase2FreqBuf[MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT], sizeof(phase2FreqBuf)-MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT);

/// Preprocessor part
      // Before optimization: 2905 cycles/call*260.4/120M = 0.63%
      // After  optimization: 1780 cycles/call*260.4/120M = 0.39%
      phase2freq_wi_unwrap((const signed char*)&rawPhaseSamplesBuffer[0], (signed char*)&phase2FreqBuf[sizeof(phase2FreqBuf)-MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT], MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT);

      // Check if we have a SYNC
      // This is necessary for phase detection and better TimeSync timestamps
      (void)SD_FindSync(phase2FreqBuf, sizeof(phase2FreqBuf)/sizeof(phase2FreqBuf[0]), RADIO_Get_IntFIFOCYCCNTTimeStamp((uint8_t)RADIO_0));

      // Before optimization: 40075 cycles/call*260.4/120M = 8.7%
      // After  optimization: 13542 cycles/call*260.4/120M = 2.9%
      unProcessedData.size = gresample((const signed char*)&phase2FreqBuf[sizeof(phase2FreqBuf)-MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT], &unProcessedData.data[0], MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT);

      // Before optimization: 82172 cycles/call*260.4/120M = 17.8%
      // After  optimization: 41622 cycles/call*260.4/120M = 9.0%
      samplesAvailable = fir_filt_circ_buff(&unProcessedData);
/// End of preprocessor

      if (samplesAvailable)
      {
         RADIO_CaptureRSSI((uint8_t)RADIO_0);
         for (uint8_t i=0; i<SOFT_DEMOD_MAX_SYNC_PAYL_TASKS+1; i++)
         {
            if (SD_FilteredPhaseSamplesSemaphores[i].available == false) // i.e. someone taken it
            {
               OS_SEM_Post(&SD_FilteredPhaseSamplesSemaphores[i].semaphore);
            }
         }
      }

   }//End for(;;) loop
}

#if 0 //Deprecated - see PSLRNR task above
/*
The Soft Demodulator Pre-Processor Task
This task is triggered through an RTOS Event signal from the phase samples listener task. The objective of this task is
to perform some preprocessing on the radio phase samples, including phase-to-frequency conversion, resampling and low
pass filtering. The output of the low pass filter is written to Filtered Phase Samples Buffer. Once a sufficient amount
of filtered phase samples are accumulated in the buffer, this task then posts a semaphore for any tasks that are waiting
for filtered phase samples. These tasks include the preamble detector task and any unblocked instance of the Sync and
Payload Demodulator Tasks (we start 2 of these to allow for potential interruptions mid-packet).
*/
void SD_PreprocessorTask(taskParameter)
{
   #if ( RTOS_SELECTION == MQX_RTOS )
   (void)   Arg0;
   #endif
//   static int8_t freqSeg_data[RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL] = {0};
   /* This could easily be changed from 225 to 150. The resampler goes from 225 to 147.45, and when
    * 150 are collected (meaning it has to be run at least more than once), The preamble detector
    * processes 150. So, if the sample rate changes from 12.207 to 8.138, instead of collecting 3 groups
    * of 75 (225) to resample to 147.45, we could collect 2 groups of 75 (150) to resample to 147.45.
    * This would keep the pre-processor running at the same granularity. So in either case, the size of
    * of this array whould be at most 150.
    */

//   static unProcessedData_t unProcessedData = {.history={0}}; //Don't need to initialize the rest.
//   uint32_t readIndex = 0;

   for (;;)
   {
      if (OS_SEM_Pend(&SD_RawPhaseSamples_Sem, 2*ONE_SEC) == true)
      {

        // Before optimization: 2905 cycles/call*260.4/120M = 0.63%
         // After  optimization: 1780 cycles/call*260.4/120M = 0.39%
         phase2freq_wi_unwrap((const signed char*)&rawPhaseSamplesBuffer[readIndex],
                              (signed char*)&freqSeg_data[0], RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL);

         // Before optimization: 40075 cycles/call*260.4/120M = 8.7%
         // After  optimization: 13542 cycles/call*260.4/120M = 2.9%
         unProcessedData.size = gresample((const signed char*)&freqSeg_data[0], unProcessedData.data, RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL);

         // Before optimization: 82172 cycles/call*260.4/120M = 17.8%
         // After  optimization: 41622 cycles/call*260.4/120M = 9.0%
         bool samplesAvailable = fir_filt_circ_buff(&unProcessedData);
         unProcessedData.size = 0;

         readIndex +=  RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL; // 225 = 75*3
         if (readIndex >= RAW_PHASE_SAMPLES_BUFFER_SIZE) // 900 = 75*3*4
         {
            readIndex = 0;
         }

         if (samplesAvailable)
         {
            for (uint8_t i=0; i<SOFT_DEMOD_MAX_SYNC_PAYL_TASKS+1; i++)
            {
               if (SD_FilteredPhaseSamplesSemaphores[i].available == false) // i.e. someone taken it
               {
                  OS_SEM_Post(&SD_FilteredPhaseSamplesSemaphores[i].semaphore);
               }
            }
         }
      }
      else // Try to clear interrupts/FIFO on the radio in an attempt to get phase samples
      {
         ERR_printf("Missing phase samples, clearing radio interrupts!!!");
         //DBG_LW_printf("\n!!!Clearing Radio Interrupts!!!\n\n");

         // Read through the buffer
         uint8_t temp[129]; //64 in each RX/TX buffer, plus the unused TX shift barrel
         si446x_read_rx_fifo((uint8_t)RADIO_0, 128, (uint8_t *)&temp[0]);

         // Clear all radio interrupts
//#define PRINT_RADIO_INT_STATUS_ON_CLEAR
#ifndef PRINT_RADIO_INT_STATUS_ON_CLEAR
         (void)si446x_get_int_status_fast_clear((uint8_t)RADIO_0);
#else
         si446x_get_int_status((uint8_t)RADIO_0, 0xFE, 0xFF, 0xFF);
         struct si446x_reply_GET_INT_STATUS_map currentIntStatus = Si446xCmd.GET_INT_STATUS; // Save all Interrupts
         INFO_printf("INT %u %02X %02X %02X %02X %02X %02X %02X %02X", RADIO_0,
                                                                       currentIntStatus.INT_PEND,
                                                                       currentIntStatus.INT_STATUS,
                                                                       currentIntStatus.PH_PEND,
                                                                       currentIntStatus.PH_STATUS,
                                                                       currentIntStatus.MODEM_PEND,
                                                                       currentIntStatus.MODEM_STATUS,
                                                                       currentIntStatus.CHIP_PEND,
                                                                       currentIntStatus.CHIP_STATUS);
#endif
         // Reset FIFO. SEEMS TO CAUSE ISSUES.
         //(void)si446x_fifo_info((uint8_t)RADIO_0, SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT);
      }
   }
}
#endif

/*
The Soft Demodulator Preamble Detector Task
This task is executed to detect preamble patterns from the filtered output data provided by the pre-processor task. If a
preamble is detected, this task unblocks any available instance of the Sync & payload demodulator task.
Immediately after execution of this the task, the preamble detector is blocked until sync detection is complete. This
prevents the preamble detector from starting more than one Sync & Payload task on the preamble of the same frame.
*/
void SD_PreambleDetectorTask(taskParameter)
{
   #if ( RTOS_SELECTION == MQX_RTOS )
   (void)   Arg0;
   #endif
   // Get and allocate the semaphore first
   int8_t semId = 0; // preamble detector semaphore ID is always 0
   SD_FilteredPhaseSamplesSemaphores[semId].available = false; //So the PreProcessor task will always notify us
   OS_SEM_Reset(&SD_FilteredPhaseSamplesSemaphores[semId].semaphore);

   bool resetPreambleDetector = true;
   float MeanMaxAbsIFrq = 0.0F;
   boolean_T FoundPA = false;
   uint32_t LoP = 0; //Length_of_Preamble
   uint32_t samplesInPreambleBuffer = 0;
   preambleDetector_initialize();

   for (;;)
   {
      (void)OS_SEM_Pend(&SD_FilteredPhaseSamplesSemaphores[semId].semaphore, OS_WAIT_FOREVER);
      // Before optimization: 153k cycles* (781/3)/120M = 33.2% of CPU
      // After optimization: 31561 cycles * (781/3)/120M = 6.8% of CPU
      //DBG_LW_printf("\nPREPRO: [%d] ",preambleDetectorReadIndex);
      preambleDetector(&filteredPhaseSamplesBuffer[preambleDetectorReadIndex],
                       resetPreambleDetector,
                       &FoundPA,
                       &LoP,
                       &samplesInPreambleBuffer,
                       &MeanMaxAbsIFrq);
      resetPreambleDetector = false;
      preambleDetectorReadIndex += FILTERED_SAMPLES_PER_EVENT;
      if (preambleDetectorReadIndex == FILTERED_PHASE_SAMPLES_BUFFER_SIZE)
      {
         preambleDetectorReadIndex = 0;
      }

      if (FoundPA == true)
      {
         if (FailCode == 0)
         {
               //preambleDetectorWithinBoundsCounter++;
               RADIO_PreambleDetected((uint8_t)RADIO_0);

               // unblock a new Sync and Payload Demodulator Task
               bool unblocked = SD_Unblock_Sync_Payload_Task();

#if( RTOS_SELECTION == MQX_RTOS ) //TODO: RA6E1 need check for FreeRTOS
               // clear the flag if previously set
               (void)_lwevent_clear(&SD_Events, SYNC_DEMOD_TASK_SYNC );
#endif

               if (unblocked)
               {
                  // the preamble detector needs to wait certain amount of time for the sync&payload task
                  //  to copy variables and be up and running.
                  (void)OS_EVNT_Wait(&SD_Events, (uint32_t)SYNC_DEMOD_TASK_SYNC, (bool)false, 3*TENTH_SEC);
               }
               else
               {
                  ERR_printf("Failed to unblock sync & payload task! Perhaps both tasks are busy");
               }

               //INFO_printf("PD:%d %d",(int)LoP, ++count);
               //INFO_printf("PD true");

               OS_SEM_Reset(&SD_FilteredPhaseSamplesSemaphores[semId].semaphore);

               // catch up with the write index
               if (filteredPhaseSamplesBufferWriteIndex >= FILTERED_PHASE_SAMPLES_BUFFER_SIZE)
               {
                  preambleDetectorReadIndex = 0;
               }
               else
               {
                  preambleDetectorReadIndex = filteredPhaseSamplesBufferWriteIndex - (filteredPhaseSamplesBufferWriteIndex %FILTERED_SAMPLES_PER_EVENT);
               }
         }
         else
         {
            DBG_printf("PD,Failcode:%d",FailCode);
         }

         resetPreambleDetector = true;
         MeanMaxAbsIFrq = 0.0F;
         FoundPA = false;
         LoP = 0;
         samplesInPreambleBuffer = 0;
         preambleDetector_initialize();
      }
   }
}

/*
The Sync & Payload Demodulator Task(s)
This task is executed in order to demodulate the contents of the frame starting from the sync to the last byte of the
frame. Once un-blocked by the preamble detector, this task fetches all the necessary data from the preamble detector task
and then begins processing the filtered phase samples from the filtered phase samples buffer, from the instance where the
preamble detector task left off. It is also responsible to issue an RSSI read request to the phase samples listener task. After
successful demodulation of frame contents, the payload bytes are written into the radio buffers (coordinated by the use of a
mutex lock). Once the PHY Task processes these bytes, the mutex is unlocked.
*/
#if ( RTOS_SELECTION == MQX_RTOS )

void SD_SyncPayloadDemodTask(taskParameter)
{

   uint8_t semId = (uint8_t)arg0; //Param in Task.c
#elif ( RTOS_SELECTION == FREE_RTOS )
 void SD_SyncPayloadDemodTask(void* pvParameter)
{
   uint8_t semId = (uint8_t )pvParameter; //Param in Task.c - // TODO: RA6E1 Bob: we need to check on this with Mario and/or Mukesh
#warning "THE ABOVE LINE OF CODE IS ALMOST CERTAINLY INCORRECT: do not enable soft demodulator!"
#endif
   eSyncAndPayloadDemodFailCodes_t FailureCode;
   uint16_t readIndex;

   for (;;)
   {
      (void)OS_EVNT_Wait(&SD_FilteredPhaseSamplesSemaphores[semId].sync_payload_event, SYNC_PAYLOAD_UNBLOCK_EVENT, (bool)false, OS_WAIT_FOREVER);
      //INFO_printf("SD_SyncPayloadDemodTask #%u start", semId);

      readIndex = preambleDetectorReadIndex; // get the current preamble detector's readIndex

      /* Initialize reentrant memory structures */
      c_SyncAndPayloadDemodulatorPers SPD_Pers = {0}; // todo: 04/16/17 6:54PM [MKD] - This is pretty big and on the stack. Check if could be made static. //"SyncAndPayloadDemodulator Persistent"
      SPD_Pers.threadID = (unsigned char) semId;

      // Copy all the required variables from Preamble Detector
      (void)memcpy((void *)&SPD_Pers.SlicIntg2[0], SlicIntg2, sizeof(float)*200);
      (void)memcpy((void *)&SPD_Pers.samplesUnProcessedBuffer[0], (const void *)&samplesUnProcessedBuffer[0], 13U*sizeof(float));
      SPD_Pers.ScaleValue = ScaleValue;
      SPD_Pers.Last_iPhase = Last_iPhase;
      SPD_Pers.EstCFO = EstCFO;
      SPD_Pers.Last_Phase = Last_Phase;
      SPD_Pers.Last_PhaseRamp = Last_PhaseRamp;
      SPD_Pers.SlicOutIdx = SlicOutIdx;
      SPD_Pers.samplesUnprocessed = samplesUnprocessed;
      SPD_Pers.ZCerrAcc = ZCerrAcc;
      SPD_Pers.findSyncStartIndex = findSyncStartIndex;

      SPD_Pers.lengthOfPayload = 0;
      SPD_Pers.SyncFound = false;
      SPD_Pers.HeaderValid = false;
      SPD_Pers.ScldFreq = 0.0F;
	  SPD_Pers.syncTime_msec = 0;
      SPD_Pers.syncTimeStamp.QSecFrac = 0;
      SPD_Pers.MlseOffset = 0;
      SPD_Pers.IdxSync2 = 0;

      FailureCode = eSPD_NO_ERROR;
      while (FailureCode == eSPD_NO_ERROR)
      {
         if (OS_SEM_Pend(&SD_FilteredPhaseSamplesSemaphores[semId].semaphore, 100) == true)
         {
            //DBG_LW_printf("DEMOD%d: [%d] ",semId,readIndex);
            SyncAndPayloadDemodulator(&SPD_Pers,
                                      &filteredPhaseSamplesBuffer[readIndex],
                                      &FailureCode);

            readIndex += FILTERED_SAMPLES_PER_EVENT; // += 150
            if (readIndex >= FILTERED_PHASE_SAMPLES_BUFFER_SIZE)//If we are at (or past) the end
            {
               readIndex = 0; //Loop to the beginning of the circular buffer
            }
         }
      }
      //INFO_printf("SD_SyncPayloadDemodTask #%u FailCode:%d", semId,FailureCode);
      //DBG_LW_printf("SD_SyncPayloadDemodTask #%u FailCode:%d", semId, FailureCode);

      SD_ReleaseFilteredSamplesSemaphore(semId);
   }
}