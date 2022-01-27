/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: NH_NoiseHistData.h
 *
 * Contents: Defines size of Noise History Data segment used by DBG_CommandLine_NoiseHistogram
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2014 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created May 2, 2014
 *
 ***********************************************************************************************************************
 * Revision History:
 * 05/02/2014 - Initial Release
 *
 * #since      2014-05-02
 **********************************************************************************************************************/
#ifndef NH_NoiseHistData_H_
#define NH_NoiseHistData_H_

#ifdef NH_GLOBAL
#define NH_EXTERN
#else
#define NH_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#if ( NOISE_HIST_ENABLED == 1 )

typedef struct {
   float      fraction;         /* cumulative probability (0 - 0.99999) */
   uint32_t   samples;          /* number of samples to take on each channel */
   uint16_t   waitTime;         /* seconds to delay before starting collection */
   uint16_t   samplingDelay;    /* microseconds to delay between RSSI samples */
   int16_t    chanFirst;        /* first channel in range or special values, as below */
   int16_t    chanLast;         /* last channel in range */
   int16_t    chanStep;         /* channel step size, can be negative */
   int16_t    filter_dBm;       /* filter out data that does not exceed this level */
   int16_t    filter_bin;       /* filter value in raw RSSI units */
   int16_t    threshRaw;        /* fixed threshold for noise burst in raw RSSI units */
   uint16_t   noiseGap;         /* number of samples considered to be a gap in a burst */
   uint16_t   hysteresis;       /* amount below threshold that signals end of a burst */
   uint8_t    radioNum;         /* radio number to test */
   uint8_t    command;          /* commmand, as below */

} NH_parameters;

#define NH_MAXHIST 32                   /* Size of histogram of burst lengths: 1-31 and >31 */
#define NH_BURST_LIST ( (uint8_t) 1 )   /* Record contains a list of noise bursts */
#define NH_BURST_HIST ( (uint8_t) 2 )   /* Record contains a histogram of burst lengths */
typedef struct {
   uint8_t recType;             /* type of burst record, from above */
   uint8_t count[2];            /* number of burst entries */
   uint8_t threshRaw;           /* threshold in raw RSSI units that these bursts exceed */
} NH_burstHeader;
typedef struct {
   uint8_t length[3];           /* number of samples in this burst */
   uint8_t sample[3];           /* sample number at which this burst started */
   uint8_t rawRSSIavg;          /* average RSSI in raw units for this burst */
   uint8_t rawRSSImin;          /* minimum RSSI in raw units for this burst */
   uint8_t rawRSSImax;          /* maximum RSSI in raw units for this burst */
} NH_burstEntry;
typedef struct {
   uint8_t occurrences[2];      /* number of occurrences of this length noise burst */
} NH_burstHistEntry;
typedef union {
   NH_burstEntry     entryB[1];           /* an entry for a burst */
   NH_burstHistEntry entryH[NH_MAXHIST];  /* an entry for a histogram of burst lengths */
} NH_burstUnion;
typedef struct {
   NH_burstHeader header;       /* define the record stored for each channel */
   NH_burstUnion  burst_u;
} NH_burstRecord;

/* The following values apply to field NH_parameters.commmand */
#define NOISE_HIST_CMD  ( (uint8_t) 1 )
#define NOISE_BURST_CMD ( (uint8_t) 2 )
#define BURST_HIST_CMD  ( (uint8_t) 3 )

/* The following values are special cases for field NH_parameters.chanFirst */
#define NH_ALL_DOWNLINK ( (int16_t) ( -1 ) )
#define NH_ALL_UPLINK   ( (int16_t) ( -2 ) )

#define NH_DB_RANGE           18
#define NH_BYTES_PER_COUNT     2
#define NH_MAX_COUNT    0x3FFFFD        // 22 bits
#define NH_OH_PER_ENTRY        4
#if ( DCU == 1 )
#define ENTRIES (  10 * ( ( NH_DB_RANGE * NH_BYTES_PER_COUNT ) + NH_OH_PER_ENTRY ) )
#else
#define ENTRIES ( 1600 * ( ( NH_DB_RANGE * NH_BYTES_PER_COUNT ) + NH_OH_PER_ENTRY ) )
#endif
NH_EXTERN uint8_t    NOISEHIST_array[ ENTRIES ];
NH_EXTERN uint32_t   NOISEHIST_bins[256];

/* Function Prototypes for externally callable functions */

void NOISEHIST_PrintResults    ( const uint8_t command );
bool NOISEHIST_IsDataAvailable ( );
void NOISEHIST_CollectData     ( NH_parameters nh );
void NOISEHIST_PrintDuration   ( NH_parameters nh, const uint16_t endTime );

#endif
#undef NH_EXTERN
#endif