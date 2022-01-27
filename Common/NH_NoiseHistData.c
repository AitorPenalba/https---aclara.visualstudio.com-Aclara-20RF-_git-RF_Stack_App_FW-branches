/******************************************************************************

   Filename: NH_NoiseHistData.c

   Global Designator: Process the noisehist, noiseburst and bursthist commmands

   Contents: functions and static data storage for processing these commands

 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2016 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 *****************************************************************************/


/* INCLUDE FILES */
#include "project.h"

#if ( NOISE_HIST_ENABLED == 1 )
#define NH_GLOBAL

#include "NH_NoiseHistData.h"

#undef  NH_GLOBAL
#include "project.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "DBG_SerialDebug.h"
#include "STACK_Protocol.h"
#include "radio.h"
#include "radio_hal.h"
#include "PHY.h"
#include "PHY_Protocol.h"


static NH_parameters last = { 0 };

static bool       dataAvailable = FALSE,
                  histOverflow = FALSE;
static uint8_t    overallHighest;
static uint32_t   runTime;
static uint32_t   histUsed;
static int16_t    threshRaw;
static char       floatStr[PRINT_FLOAT_SIZE];

#define NUM_DL ( 122 )
const uint16_t dl_chans[NUM_DL] = { 198, 325, 326, 327, 329, 330, 331, 333, 334, 335, 337, 338, 339, 341, 342, 343, 345, 346, 347, 349,
                                    350, 351, 365, 366, 367, 397, 398, 399, 405, 406, 407, 421, 422, 423, 425, 426, 427, 429, 430, 431,
                                    433, 434, 435, 445, 446, 447, 449, 450, 451, 453, 454, 455, 461, 462, 463, 477, 478, 479,1949,1950,
                                   1951,1993,1994,1995,1997,1998,1999,2001,2002,2003,2042,2046,2050,2054,2058,2062,2066,2749,2750,2751,
                                   2753,2754,2755,2757,2758,2759,2761,2762,2763,2765,2766,2767,2769,2770,2771,2773,2774,2775,2777,2778,
                                   2779,2781,2782,2783,2785,2786,2787,2789,2790,2791,2793,2794,2795,2797,2798,2799,2801,2802,2803,2858,
                                   2862,2866};
#define UL_LOW  ( (int16_t)  960 )
#define UL_HIGH ( (int16_t) 1279 )

/* Local function definitions */
static uint32_t getNoiseHistogram( const uint16_t channel, const int32_t roomLeft, const bool histMode,
                                   NH_parameters nh, NH_burstRecord *returnBuf );
static void     printParameters( NH_parameters nh );
static void     printHeader( uint8_t command, uint16_t channels );
static uint16_t countChannels ( NH_parameters nh );
static uint16_t addBurstEntry( uint16_t const count, uint32_t const length, uint32_t const sample, double const rawAverage,
                               uint8_t const rawMin, uint8_t const rawMax, bool const histMode, NH_burstRecord *returnBuf );
static uint32_t checkNoiseBurst( uint8_t const rawValue, int32_t const sample, int16_t const threshRaw,
                                 uint16_t const noiseGap, uint16_t const hysteresis, uint32_t const roomLeft,
                                 bool const histMode, NH_burstRecord *returnBuf );
static uint8_t  calcNewThresh( const uint32_t bins[256], float fraction );
static uint32_t storeHistogramCount( const uint32_t histIndex, const uint32_t binValue );

/*****************************************************************************

   Function Name: NOISEHIST_IsDataAvailable

   Purpose: Inform caller if data has been collected

   Arguments:  None

   Returns: TRUE if data is available, else FALSE

   Notes:

******************************************************************************/
bool NOISEHIST_IsDataAvailable ( void )
{
   return ( dataAvailable );
}

/*****************************************************************************

   Function Name: NOISEHIST_PrintDuration

   Purpose: Calculate and print the estimated duration of RSSI collection

   Arguments:  nh      - structure containing all parameter values entered
               endTime - extra delay time after collection is completed

   Returns: None

   Notes:

******************************************************************************/
void NOISEHIST_PrintDuration ( NH_parameters nh, const uint16_t endTime )
{
   uint32_t channels = (uint32_t)countChannels ( nh );
   uint32_t time;
   time  = ( (uint32_t)(nh.samples / 1000) ) * 5; // We delay by 5 milliseconds after 1000 samples in getNoiseHistogram
   time += channels * ( 5 + 13 );                 // We delay by 5 milliseconds after every channel and channel loop is 12.5 msec
   if ( nh.samplingDelay < 120 )
   {
      // when sampling rate is less than 120, CCA will run for a minimum time.
      time = ( uint32_t ) ( ( (float)( time ) + (float)channels * nh.samples * ( 0.27f ) ) / 1000.0f + 0.5f );
   }
   else
   {
      time = ( uint32_t ) ( ( (float)( time ) + (float)channels * nh.samples * ( (float)( nh.samplingDelay ) / 1000.0f + 0.26f ) ) / 1000.0f + 0.5f );
   }
   time += nh.waitTime;          // We delay x seconds at the start
   time += endTime;              // We delay y seconds at the end
   (void) printf( "Collection will take about %02u:%02u:%02u\n", time / 3600, ( time % 3600 ) / 60, time % 60 );
   printParameters( nh );
}
/*****************************************************************************

   Function Name: foldHistogramTo_dB

   Purpose: Add adjacent 0.5dBm counts and place in 1dBm bins

   Arguments:  None

   Returns: None

   Notes:

******************************************************************************/
void foldHistogramTo_dB()
{
   for ( uint32_t i = 0; i < ARRAY_IDX_CNT( NOISEHIST_bins ) - 1; i += 2 )
   {
      if ( ( NOISEHIST_bins[i] == 0xFFFFFFFF ) || ( NOISEHIST_bins[i+1] == 0xFFFFFFFF ) ) {
         NOISEHIST_bins[i >> 1] = 0xFFFFFFFF;
      } else {
         NOISEHIST_bins[i >> 1] = NOISEHIST_bins[i] + NOISEHIST_bins[i+1];
      }
   }
}

/*****************************************************************************

   Function Name: NOISEHIST_CollectData

   Purpose: Collect RSSI data for noisehist, noiseburst or bursthist commands

   Arguments:  nh - structure containing all parameter values entered

   Returns: None

   Notes:

******************************************************************************/
void NOISEHIST_CollectData ( NH_parameters nh )
{
   OS_TICK_Struct    time1,
                     time2;
   uint32_t          bytesUsed = 0,
                     bytesLeft = sizeof(NOISEHIST_array) - 2;
   uint32_t          histIndex = 0;
   uint16_t          chan,
                     channels = countChannels ( nh );

   _time_get_elapsed_ticks(&time1);
   overallHighest = 0;
   histOverflow = FALSE;
   for ( uint16_t chanNdx = 0; chanNdx < channels; chanNdx++ )
   {
      uint32_t histLowest = 0,
               histHighest = 0;
      if ( nh.chanFirst < 0 )
      {
         chan = dl_chans[ chanNdx ];
      }
      else
      {
         chan = ( uint16_t )( nh.chanFirst + nh.chanStep * chanNdx );
      }
      NOISEHIST_array[ histIndex++ ] = (uint8_t)( chan & 0xFF );
      NOISEHIST_array[ histIndex++ ] = (uint8_t)( chan >> 8   );
      bytesLeft -= 2;
      PHY_Lock();

      bytesUsed = getNoiseHistogram( chan, ( int32_t )bytesLeft, (bool)( nh.command == BURST_HIST_CMD ),
                                     nh, (NH_burstRecord *)(&NOISEHIST_array[ histIndex ] ) );
      histIndex += bytesUsed;
      bytesLeft -= bytesUsed;
      PHY_Unlock();
      dataAvailable = TRUE;
      last = nh;
      {
         uint32_t i;
         foldHistogramTo_dB();
         for ( i = 0; i < 128; i++ )
         {
            if ( NOISEHIST_bins[i] != 0 )
            {
               histLowest = i;
               break;
            }
         }
         for ( i = histLowest; i < 128; i++ )
         {
            if ( NOISEHIST_bins[i] != 0 )
            {
               histHighest = i;
            }
         }
         if ( histHighest > ( uint32_t )( int32_t )nh.filter_bin )
         {
            if ( ( histIndex + ( ( ( ( histHighest - histLowest ) + 1 ) * NH_BYTES_PER_COUNT ) + NH_OH_PER_ENTRY ) ) < ENTRIES )
            {
               NOISEHIST_array[ histIndex++ ] = (uint8_t)( histLowest );
               NOISEHIST_array[ histIndex++ ] = (uint8_t)( histHighest  );
               for ( i = histLowest; i <= histHighest; i++ )
               {
                  histIndex = storeHistogramCount( histIndex, NOISEHIST_bins[i] );
               }
               if ( histHighest > overallHighest)
               {
                  overallHighest = ( uint8_t )histHighest;
               }
            }
            else
            {
               histOverflow = TRUE;
               break;
            }
         }
      }
      OS_TASK_Sleep( 5 ); // Be nice to other tasks
   }
   NOISEHIST_array[ histIndex++ ] = 0xFF;
   NOISEHIST_array[ histIndex++ ] = 0xFF;
   histUsed = histIndex;
   // Get the RX channels
   PHY_GetConf_t GetConf = PHY_GetRequest( ePhyAttr_RxChannels );
   if (GetConf.eStatus == ePHY_GET_SUCCESS)
   {
      // Restart the radio if needed
      if (GetConf.val.RxChannels[nh.radioNum] != PHY_INVALID_CHANNEL)
      {
         (void) Radio.StartRx( nh.radioNum, GetConf.val.RxChannels[nh.radioNum] );
      }
   }
   _time_get_elapsed_ticks(&time2);
   bool Overflow;
   runTime = (uint32_t)( _time_diff_milliseconds ( &time2, &time1, &Overflow ) );
   (void) printf( "Collection is complete in %d msec, used %d of %d bytes; ", runTime, histUsed, sizeof(NOISEHIST_array) );
   if ( histOverflow )
   {
      (void) printf ("not all channels completed!\n");
   }
   else
   {
      (void) printf ("all %d channel(s) completed\n", channels );
   }
}

/*****************************************************************************

   Function Name: NOISEHIST_PrintResults

   Purpose: Prints the results of noisehist, noiseburst or bursthist commands

   Arguments:  commmand - which type of data to print

   Returns: None

   Notes:

******************************************************************************/
void NOISEHIST_PrintResults ( const uint8_t command )
{
   uint32_t i,
            histIndex = 0;
   if ( NOISEHIST_IsDataAvailable() )
   {
      printParameters( last );
      NH_burstRecord *burstPointer;
      uint16_t channels = countChannels ( last );
      if (  ( command == NOISE_HIST_CMD ) || ( command == last.command) ||
            ( command == BURST_HIST_CMD && last.command == NOISE_BURST_CMD ) )
      {
         printHeader( command, channels );
         histIndex = 0;
         while ( histIndex < histUsed )
         {
            uint16_t chan  = ( (uint16_t)NOISEHIST_array[ histIndex++ ] );
                     chan += ( uint16_t )(( (uint32_t)NOISEHIST_array[ histIndex++ ] ) << 8 );
            if ( chan <= 3200 )
            {
               (void)printf( "ch%04d,", chan );
               burstPointer = (NH_burstRecord *)&NOISEHIST_array[ histIndex ];
               uint16_t cnt = ( (uint16_t)( burstPointer -> header.count[0] )      )
                              + (uint16_t)( (uint16_t)( burstPointer -> header.count[1] ) << 8 );
               threshRaw = (int16_t)( burstPointer -> header.threshRaw );
               uint8_t recType = burstPointer -> header.recType;
               histIndex += sizeof( burstPointer -> header );
               if ( cnt < 0xFFFE )
               {
                  if ( command == BURST_HIST_CMD )
                  {
                     (void) printf( "%s,%d", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( threshRaw ), 1 ), cnt );
                  }
                  if ( recType == NH_BURST_LIST )
                  {
                     if ( command == NOISE_BURST_CMD ) { (void) printf( "\n" ); }
                     for ( i = 0; i < NH_MAXHIST; i++ )
                     {
                        NOISEHIST_bins[i] = 0;
                     }
                     for ( i = 0; i < cnt; i++ )
                     {
                        uint32_t length = ( (uint32_t)( burstPointer -> burst_u.entryB[i].length[0] )       )
                                        + ( (uint32_t)( burstPointer -> burst_u.entryB[i].length[1] ) << 8  )
                                        + ( (uint32_t)( burstPointer -> burst_u.entryB[i].length[2] ) << 16 );
                        uint32_t sample = ( (uint32_t)( burstPointer -> burst_u.entryB[i].sample[0] )       )
                                        + ( (uint32_t)( burstPointer -> burst_u.entryB[i].sample[1] ) << 8  )
                                        + ( (uint32_t)( burstPointer -> burst_u.entryB[i].sample[2] ) << 16 );
                        if ( length < ( NH_MAXHIST - 1 ) ) {
                           NOISEHIST_bins[length - 1]++;
                        } else {
                           NOISEHIST_bins[NH_MAXHIST - 1]++;
                        }
                        if ( command == NOISE_BURST_CMD )
                        {
                           (void) printf( ",,,%d,%d", sample, length );
                           (void) printf( ",%s", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( burstPointer -> burst_u.entryB[i].rawRSSImin ), 1) );
                           (void) printf( ",%s", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( burstPointer -> burst_u.entryB[i].rawRSSIavg ), 1) );
                           (void) printf( ",%s\n", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( burstPointer -> burst_u.entryB[i].rawRSSImax ), 1) );
                        }
                        histIndex += sizeof(NH_burstEntry);
                     }
                     if ( command == BURST_HIST_CMD )
                     {
                        for ( i = 0; i < NH_MAXHIST; i++ ) { (void) printf( ",%d", NOISEHIST_bins[i] ); }
                        (void) printf( "\n" );
                     }
                  }
                  else if ( recType == NH_BURST_HIST )
                  {
                     if ( cnt > 0 )
                     {
                        for ( i = 0; i < NH_MAXHIST; i++ )
                        {
                           if ( command == BURST_HIST_CMD ) {
                              (void) printf( ",%d", ( (uint16_t)( burstPointer -> burst_u.entryH[i].occurrences[0] ) +
                                                      ( (uint16_t)( burstPointer -> burst_u.entryH[i].occurrences[1] ) << 8 ) ) ); }
                           histIndex += sizeof(NH_burstHistEntry);
                        }
                     }
                     if ( ( command == NOISE_BURST_CMD ) || ( command == BURST_HIST_CMD ) ) { (void) printf( "\n" ); }
                  }
               } else {
                  (void) printf( "Unable to lock PHY\n" );
               }
               uint8_t histLowest  = NOISEHIST_array[ histIndex++ ];
               uint8_t histHighest = NOISEHIST_array[ histIndex++ ];
               if ( ( histHighest == 0xFF ) && ( histLowest == 0xFF ) )
               {
                  (void) printf( "Insufficient room in NOISEHIST_array\n" );
                  break;   // end of histogram data
               }
               else
               {
                  if ( ( histHighest == 0xFF ) && ( histLowest == 0xFE ) ) { (void) printf( "Unable to lock PHY\n" ); }
                  else
                  {
                     if ( command == NOISE_HIST_CMD )
                     {
                        (void) printf( "%s,", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( histLowest  * 2 ), 0 ) );
                        (void) printf( "%s,", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( histHighest * 2 ), 0 ) );
                     }
                     (void) memset( NOISEHIST_bins, 0, sizeof(NOISEHIST_bins));
                     uint32_t sum = 0,
                              cumulativeBin = 0;
                     for ( i = histLowest; i <= histHighest; i++ )
                     {
                        uint32_t bigCount = NOISEHIST_array[ histIndex++ ];
                        if ( bigCount > 0x00007F )
                        {
                           bigCount =    ( bigCount & 0x00007F ) | ( ( (uint32_t)( NOISEHIST_array[ histIndex++ ] ) ) <<  7 );
                           if ( bigCount > 0x3FFF )
                           {
                              bigCount = ( bigCount & 0x003FFF ) | ( ( (uint32_t)( NOISEHIST_array[ histIndex++ ] ) ) << 14 );
                           }
                        }
                        NOISEHIST_bins[ i ] = bigCount;
                        sum += bigCount;
                        if ( ( ( (double)sum / (double)last.samples ) >= (double)last.fraction ) && ( cumulativeBin == 0 ) )
                        {
                           cumulativeBin = i;
                        }
                     }
                     if ( command == NOISE_HIST_CMD )
                     {
                        float reported = (float)cumulativeBin * 2.0F;
                        (void) printf( "%s,", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( reported ), 0 ) );
                        for ( i = 0; i <= histHighest; i++ )
                        {
                           (void) printf( "%d,", NOISEHIST_bins[ i ] );
                        }
                        (void) printf( "0\n" );
                     }
                  }
               }
            }
         }
      } else {
         DBG_logPrintf( 'R', "ERROR - display command incompatible with collection command" );
      }
   } else {
      DBG_logPrintf( 'R', "ERROR - no noise data collected" );
   }
}

static uint32_t storeHistogramCount( const uint32_t startIndex, const uint32_t binValue )
{
   uint32_t binVal = binValue, histIndex = startIndex;
   if ( binVal > NH_MAX_COUNT )
   {
      binVal = NH_MAX_COUNT;
   }
   if ( binVal <= 0x7F )
   {
      NOISEHIST_array[ histIndex++ ] =    (uint8_t)(     binVal );
   }
   else
   {
      NOISEHIST_array[ histIndex++ ] =    (uint8_t)( (   binVal & 0x00007F ) | 0x80);
      if ( binVal <= 0x3FFF )
      {
         NOISEHIST_array[ histIndex++ ] = (uint8_t)( (   binVal & 0x003F80 ) >>  7 );
      }
      else
      {
         NOISEHIST_array[ histIndex++ ] = (uint8_t)( ( ( binVal & 0x003F80 ) >>  7 ) | 0x80);
         NOISEHIST_array[ histIndex++ ] = (uint8_t)( ( ( binVal & 0x3FC000 ) >> 14 )       );
      }
   }
   return ( histIndex );
}

static uint16_t addBurstEntry( uint16_t const idx, uint32_t const length, uint32_t const sample, double const rawAverage,
                        uint8_t const rawMin, uint8_t const rawMax, bool const histMode, NH_burstRecord *returnBuf )
{
   if ( idx < ARRAY_IDX_CNT( returnBuf->burst_u.entryB ) )
   {
      uint32_t rawAvg = (uint32_t)( log10f((float)(rawAverage / (float)length)) * (float)100.0 );
      if ( !histMode )
      {
         returnBuf->burst_u.entryB[idx].length[0]  = (uint8_t)( ( length       ) & 0x000000FF );
         returnBuf->burst_u.entryB[idx].length[1]  = (uint8_t)( ( length >>  8 ) & 0x000000FF );
         returnBuf->burst_u.entryB[idx].length[2]  = (uint8_t)( ( length >> 16 ) & 0x000000FF );
         returnBuf->burst_u.entryB[idx].sample[0]  = (uint8_t)( ( sample       ) & 0x000000FF );
         returnBuf->burst_u.entryB[idx].sample[1]  = (uint8_t)( ( sample >>  8 ) & 0x000000FF );
         returnBuf->burst_u.entryB[idx].sample[2]  = (uint8_t)( ( sample >> 16 ) & 0x000000FF );
         returnBuf->burst_u.entryB[idx].rawRSSImin = (uint8_t)( rawMin );
         returnBuf->burst_u.entryB[idx].rawRSSIavg = (uint8_t)( rawAvg );
         returnBuf->burst_u.entryB[idx].rawRSSImax = (uint8_t)( rawMax );
      }
      else
      {
         uint32_t histNdx = length - 1;
         if ( histNdx >= NH_MAXHIST )
         {
            histNdx = NH_MAXHIST - 1;
         }
         returnBuf->burst_u.entryH[histNdx].occurrences[0]++;
         if ( returnBuf->burst_u.entryH[histNdx].occurrences[0] == 0 )
         {
            returnBuf->burst_u.entryH[histNdx].occurrences[1]++;
         }
      }
#if 0
      printf( "Added entry %d: length=%d, sample=%d, min=%d, avg=%d, max=%d\n", idx, length, sample, rawMin, rawAvg, rawMax);
#endif
      return ( idx + 1 );
   }
   else
   {
      return ( idx );
   }
}

/***********************************************************************************************************************

   Function Name: getNoiseHistogram

   Purpose: Extract RSSI values from radio during a period it is believe to be hearing only noise

   Arguments:  radioChannel   - SRFN channel to use (0 - 3200)
               radioNum       - index of the radio (0, or 0-8 for frodo)
               samplingDelay  - delay between RSSI samples in microseconds
               threshRaw      - RSSI threshold in raw RSSI units to declare a noise burst;
                               threshRaw = -1 means that it must be calculated from fraction parameter
                               threshRaw = -2 means that no noise bursts are recorded
               noiseGap       - Samples RSSI < (thresh_raw - hysteresis) that is still the same burst
               hysteresis     - if RSSI < (thresh_raw - hysteresis) for more than noiseGap, burst ends
               roomLeft       - Space left in *returnBuf
               *returnBuf      - Pointer to place to store results (assume only byte alignment)

   Returns:    Number of bytes used in *returnBuf

   Side Effects: Radio radioNum is temporarily re-purposed and cannot receive real messages
                  NOISEHIST_bins vector contains the noise histogram for this channel

   Reentrant Code: No

 **********************************************************************************************************************/
static uint32_t getNoiseHistogram( const uint16_t radioChannel, const int32_t roomLeft, const bool histMode,
                                   NH_parameters nh, NH_burstRecord *returnBuf )
{
   uint32_t i;
   int16_t  thresh = nh.threshRaw;
   uint8_t  rawValue;

   // Pre-clear the histogram accumulation
   for (i=0; i < 256; i++) NOISEHIST_bins[i] = 0;

   RADIO_SetupNoiseReadings( nh.radioNum, nh.samplingDelay, radioChannel );

   (void) checkNoiseBurst( (uint8_t)0, (int32_t)( 0 ), (int16_t)( 0 ), nh.noiseGap, nh.hysteresis, (uint32_t)roomLeft, histMode, returnBuf );

   for (i = 1; i <= nh.samples; i++)
   {
      rawValue = RADIO_GetNoiseValue( nh.radioNum, nh.samplingDelay );
#if 0
      (void)printf("Raw: %hhu\n", rawValue );
#endif
      NOISEHIST_bins[rawValue]++;
      if ( NOISEHIST_bins[rawValue] == 0 )
      {
         NOISEHIST_bins[rawValue] = 0xFFFFFFFF; // don't miss an overflow
      }
      if ( thresh >= 0 )
      {
         (void) checkNoiseBurst( rawValue, (int32_t)i, thresh, nh.noiseGap, nh.hysteresis, (uint32_t)roomLeft, histMode, returnBuf );
      }
      else if ( ( thresh == -1 ) && ( i == 10000 ) )
      {
         thresh = calcNewThresh( NOISEHIST_bins, nh.fraction );
      }

      if ( ( i % 1000 ) == 0 )
      {
         OS_TASK_Sleep( 5 ); // Be nice to other tasks
      }
   }

   RADIO_TerminateNoiseReadings( nh.radioNum );

   return ( checkNoiseBurst( (uint8_t)0, (int32_t)( -1 ), thresh, nh.noiseGap, nh.hysteresis, (uint32_t)roomLeft, histMode, returnBuf ) );
}

/* Local function to print the correct header for the type of data requested to be printed */
static void printHeader( uint8_t command, uint16_t channels )
{
   (void) printf( "\nCollection took %d milliseconds to complete used %d of %d bytes; ", runTime, histUsed, sizeof(NOISEHIST_array) );
   if ( histOverflow )
   {
      (void) printf ("not all channels completed!\n");
   }
   else
   {
      (void) printf ("all %d channel(s) completed.\n", channels );
   }
   if ( command == NOISE_HIST_CMD )
   {
      (void) printf( ",min,max,%s%,%s%%,", DBG_printFloat( floatStr, ( 100.0f * last.fraction + 0.00001f ), 5 ) );
      (void) printf( "%s", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( (uint8_t)0 ), 0 ) );
      for ( uint32_t i = 1; i <= overallHighest+1; i++ )
      {
         (void) printf( ",%s", DBG_printFloat( floatStr, RSSI_RAW_TO_DBM( (uint8_t)(2*i) ), 0 ) );
      }
      (void) printf( "\n" );
   }
   else if ( command == NOISE_BURST_CMD )
   {
      (void) printf( "chan,thresh_dBm,count,sample,length,min_dBm,avg_dBm,max_dBm\n");
   }
   else if ( command == BURST_HIST_CMD  )
   {
      (void) printf( "chan,thresh_dBm,count,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,>31\n" );
   }
}

static void printParameters( NH_parameters nh )
{
   uint16_t channels = countChannels ( nh );

   (void) printf( "radio=%d, sampling=%d, samples=%d, start=%d, end=%d, step=%d, channels=%d, fraction=%s, ",
                     nh.radioNum, nh.samplingDelay, nh.samples, nh.chanFirst, nh.chanLast, nh.chanStep, channels,
                     DBG_printFloat( floatStr, ( nh.fraction + 0.0000001f ), 6 ) );
   (void) printf( "filter_dBm=%d, filter_bin=%d, threshRaw=%d, noiseGap=%d, hysteresis=%d, &NOISEHIST_array=%x\n",
                     nh.filter_dBm, nh.filter_bin, nh.threshRaw, nh.noiseGap, nh.hysteresis, (uint8_t *)(&NOISEHIST_array[0]) );
}

static uint8_t calcNewThresh( const uint32_t bins[256], float fraction )
{
   uint32_t sum, sumThresh, i;
   sum = 0;
   for ( i = 0; i < 256; i++ )
   {
      sum += bins[i];
   }
   sumThresh = (uint32_t) ( (double)sum * (double)fraction );
   sum = 0;
   for ( i = 0; i < 256; i++)
   {
      sum += bins[i];
      if ( sum >= sumThresh )
      {
         return ( (uint8_t)i );
      }
   }
   return ( 255 );
}

static uint32_t checkNoiseBurst(uint8_t const rawValue, int32_t const sample, int16_t const rawThresh,
                           uint16_t const noiseGap, uint16_t const hysteresis, uint32_t const roomLeft,
                           bool const histMode, NH_burstRecord *returnBuf )
{
   static   double   rawAverage;
   static   uint32_t bytesUsed = 0,
                     start,
                     length,
                     gapStart;
   static   uint16_t cnt = 0;
   static   uint8_t  rawMin,
                     rawMax;
   static   bool     inBurst = FALSE,
                     inGap = FALSE;
   uint32_t retVal = 0;

   if ( sample > 0 )
   {
      if ( !inBurst )
      {  // Not yet in a noise burst
         if ( rawValue >= rawThresh )
         {  // Noise burst starts now
            start = ( uint32_t )sample;
            length = 1;
            rawMin = rawValue;
            rawMax = rawValue;
            rawAverage = (double)( powf(10.0f, ( (float)rawValue / 100.0f) ) );
            inBurst = TRUE;
            inGap   = FALSE; // should already always be FALSE
         }
         else
         {  // Not yet in a noise burst, not above threshold
            // Nothing to do
         }
      }
      else
      {
         if ( !inGap )
         {  // In a noise burst but not in a gap
            if ( rawValue >= ( rawThresh - hysteresis ) )
            {
               length++;
               if ( rawValue < rawMin ) { rawMin = rawValue; }
               if ( rawValue > rawMax ) { rawMax = rawValue; }
               rawAverage += (double)( powf(10.0f, ( (float)rawValue / 100.0f) ) );
            }
            else
            {
               gapStart = ( uint32_t )sample;
               inGap = TRUE;
            }
         }
         else
         {
            if ( (uint16_t)( sample - (int32_t)gapStart ) > noiseGap )
            {
               if ( ( roomLeft - bytesUsed ) > sizeof(NH_burstEntry) )
               {  // Noise gap is long enough to terminate this burst; log it
                  cnt = addBurstEntry(cnt, length, start, rawAverage, rawMin, rawMax, histMode, returnBuf );
                  bytesUsed += sizeof(NH_burstEntry);
                  inGap = FALSE;
                  inBurst = FALSE; // should always aready be FALSE
               }
            }
            else
            {
               if ( rawValue >= rawThresh )
               {  // Was in a short enough gap within a burst but back to burst again
                  length++;
                  if ( rawValue < rawMin ) { rawMin = rawValue; }
                  if ( rawValue > rawMax ) { rawMax = rawValue; }
                  rawAverage += (double)( powf(10.0f, ( (float)rawValue / 100.0f) ) );
                  inGap = FALSE;
               }
               else
               {  // Was in a short enough gap and still in gap
                  // Nothing to do
               }
            }
         }
      }
   }
   else
   {
      if ( sample == -1 )
      {  // Termination processing after all samples have been taken
         if ( !histMode )
         {
            retVal = sizeof ( returnBuf -> header ) + bytesUsed;
            if ( inBurst )
            {
               if ( ( roomLeft - bytesUsed ) > sizeof(NH_burstEntry) )
               {
                  cnt = addBurstEntry(cnt, length, start, rawAverage, rawMin, rawMax, histMode, returnBuf );
                  retVal += sizeof(NH_burstEntry);
               }
            }
         }
         else
         {
            if ( cnt > 0 )
            {
               retVal = sizeof ( returnBuf -> header ) + sizeof ( returnBuf -> burst_u );
            }
         }
      }
      else if ( sample == 0 )
      {  // Initialization processing
         inBurst = FALSE;
         inGap = FALSE;
         bytesUsed = 0;
         cnt = 0;
         retVal = 0;
         if ( histMode )
         {
            for ( uint32_t i = 0; i < NH_MAXHIST; i++ )
            {
               returnBuf -> burst_u.entryH[i].occurrences[0] = 0;
               returnBuf -> burst_u.entryH[i].occurrences[1] = 0;
            }
         }
      }
      if ( !histMode ) {
         returnBuf -> header.recType = NH_BURST_LIST;
      } else {
         returnBuf -> header.recType = NH_BURST_HIST;
      }
      returnBuf -> header.count[0] = ( ( cnt      ) & 0x00FF );
      returnBuf -> header.count[1] = ( ( cnt >> 8 ) & 0x00FF );
      returnBuf -> header.threshRaw = (uint8_t) rawThresh;
   }
   return ( retVal );
}

static uint16_t countChannels ( NH_parameters nh )
{
   uint16_t channels;
   if ( nh.chanFirst == NH_ALL_DOWNLINK )
   {
      channels = NUM_DL;
   }
   else if ( nh.chanFirst == NH_ALL_UPLINK )
   {
      channels = (uint16_t) ( ( ( UL_HIGH - UL_LOW ) + (uint16_t)nh.chanStep ) / (uint16_t)nh.chanStep );
   }
   else
   {
      channels = (uint16_t) ( ( ( nh.chanLast - nh.chanFirst ) + nh.chanStep ) / nh.chanStep );
   }
   return ( channels );
}

#endif
