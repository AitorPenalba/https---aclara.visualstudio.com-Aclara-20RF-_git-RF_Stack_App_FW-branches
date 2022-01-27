/******************************************************************************

   Filename: SoftDemodulator.h

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

#ifndef _SOFT_DEMOD_H_
#define _SOFT_DEMOD_H_

#include "PHY_Protocol.h"
#include "radio.h"

#define SOFT_DEMOD_RAW_PHASE_SAMPLES_AVAILABLE_EVENT 0x01
#define SYNC_DEMOD_TASK_SYNC 0x02
#define SOFT_DEMOD_PROCESSED_PAYLOAD_BYTES 0x04

#define SOFT_DEMOD_MAX_SYNC_PAYL_TASKS 2

#define MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT 75
#define FILTERED_SAMPLES_PER_EVENT                 150 // Number of filtered samples needed to run the PreambleDetector and the Demods
#define RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL         MAX_PHASE_SAMPLES_FROM_RADIO_PER_INTERRUPT * 1 //  75 (Merged PSLSNR and PREPRO)
#define RAW_PHASE_SAMPLES_BUFFER_SIZE              RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL * 1         //  75 Buffer between PSLSNR and PreProcessor [MERGED]
#define FILTERED_PHASE_SAMPLES_BUFFER_SIZE         FILTERED_SAMPLES_PER_EVENT * 4                 //  600 Buffer after PreProcessor shared by PreambleDetector and Demods

#define SD_RSSI_PIPELINE_DELAY 3    //The pipeline delay for the RSSI measurements, from the SyncAndPayloadDemodulator call to RADIO_SyncDetected()
                                    //the lag to the first high RSSI (preamble) is 12 measurements

#define NUMBER_SYNC_SYMBOLS 32
#define NUMBER_OF_FILTER_TAPS 32
#define NUMBER_OF_SYNC_ERRORS_ALLOWED 2

typedef struct {
   float rssi;
   float freqOffset;
   unsigned int synctime_msec;
   TIMESTAMP_t syncTimeStamp;
   unsigned char threadID;
} SD_RadioParams_t;

typedef struct
{
   volatile bool available;
   OS_SEM_Obj semaphore;
   OS_EVNT_Obj sync_payload_event;
} filteredPhaseSamples_sem_t ;

typedef struct //Used in fir_filt_circ_buff also
{
   //History and Data MUST be continuous in memory, this should ensure so.
   float history[NUMBER_OF_FILTER_TAPS-1];
   float data[RAW_PHASE_SAMPLES_COUNT_PER_SIGNAL];
   uint32_t size;
} unProcessedData_t ;

extern OS_EVNT_Obj SD_Events;
extern OS_MUTEX_Obj SD_Payload_Mutex;
extern float filteredPhaseSamplesBuffer[FILTERED_PHASE_SAMPLES_BUFFER_SIZE];
extern uint16_t filteredPhaseSamplesBufferWriteIndex;
//extern filteredPhaseSamples_sem_t SD_FilteredPhaseSamplesSemaphores[SOFT_DEMOD_MAX_SYNC_PAYL_TASKS + 1]; // +1 for preamble detector task

returnStatus_t SoftDemodulator_Initialize(void);

void SD_PhaseSamplesListenerTask(uint32_t arg0);
void SD_PreprocessorTask(uint32_t arg0);
void SD_PreambleDetectorTask(uint32_t arg0);
void SD_SyncPayloadDemodTask(uint32_t arg0);

bool ProcessSRFNHeaderPBytes(unsigned const char headerBytes[], unsigned int* lengthOfPayload);
bool ProcessSRFNPayloadPBytes(unsigned const char payloadBytes[], unsigned short numberOfPayloadBytes);
#endif