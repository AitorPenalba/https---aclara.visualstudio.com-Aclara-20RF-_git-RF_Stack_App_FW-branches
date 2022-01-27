/******************************************************************************

   Filename: SyncAndPayloadDemodulator_types.h

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

#ifndef SYNCANDPAYLOADDEMODULATOR_TYPES_H
#define SYNCANDPAYLOADDEMODULATOR_TYPES_H

/* Include files */
#include "rtwtypes.h"

typedef enum
{
   eSPD_NO_ERROR = (uint32_t)0,  // Unique values for FailCode in SyncAndPayloadDemodulator.c
   eSPD_SYNC_FAIL,
   eSPD_HDR_CRC_FAIL,
   eSPD_SRFN_PAYLOAD_POSTED
} eSyncAndPayloadDemodFailCodes_t;

/* Type Definitions */
#include "PHY_Protocol.h"

#ifndef typedef_c_SyncAndPayloadDemodulatorPers
#define typedef_c_SyncAndPayloadDemodulatorPers
typedef struct {
   float SlicIntg2[1300];
   float samplesUnProcessedBuffer[13];
   float EstCFO;
   float Last_iPhase;
   float ScaleValue;
   float Last_PhaseRamp;
   float Last_Phase;
   float ScldFreq;

   bool SyncFound;
   bool HeaderValid;
   uint32_t MlseOffset;
   uint32_t IdxSync2;
   uint32_t SlicOutIdx;
   uint32_t samplesUnprocessed;
   uint32_t lengthOfPayload;
   int ZCerrAcc;
   uint32_t findSyncStartIndex;
   uint32_t syncTime_msec;

   TIMESTAMP_t syncTimeStamp;

   unsigned char threadID;
} c_SyncAndPayloadDemodulatorPers;
#endif

#endif