/***********************************************************************************************************************
 *
 * Filename: radio_TCXO.c
 *
 * Global Designator: RADIO_
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2018-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

#include <stdlib.h>
#include "project.h"
#include <bsp.h>
#include <mqx_prv.h>
#include "PHY_Protocol.h"
#include "phy.h"
#include "time_sys.h"
#include "radio.h"
#include "radio_hal.h"
#include "xradio.h"
#include "si446x_api_lib.h"
#include "DBG_SerialDebug.h"
#include "time_util.h"
#include "sys_clock.h"

/*****************************************************************************
 *  Local Macros & Definitions
 *****************************************************************************/

#define RADIO_CLK_BUFFER_SIZE 1000UL // DMA will generates an interrupt every times 1000 radio TCXO interrupts (at 1MHz) was received so every 1msec.
                                     // Code assumes 1000 for some don't change

#define DMA_CNTR              1000UL // How many DMA interrupts will be used to compute the real frequency. Longer values smooth out frequency variations.
                                     // Number must be modulo 1000 and at least 1000. 1000 is 1 second of integration (assuming that the TCXO is exactly 30MHz).

#define SAMPLING_TIME         ((DMA_CNTR*RADIO_CLK_BUFFER_SIZE)/1000000UL) // Sampling time for each frequency estimation in second
                                                                           // DMA_CNTR*RADIO_CLK_BUFFER_SIZE must be modulo 1000000

#if ( EP == 1 )
#define FREQUENCY_MOVING_WINDOW    22U // Length of the frequency filter. Be carefull to keep this small enough to avoid overflow when computing the sum of all frequencies.
#define MAX_FREQUENCY_DIFF       1000U // Maximum difference between the smallest and largest frequency to be considered valid.
#else
#define FREQUENCY_MOVING_WINDOW    10U // Length of the frequency filter. Be carefull to keep this small enough to avoid overflow when computing the sum of all frequencies.
#define MAX_FREQUENCY_DIFF         50U // Maximum difference between the smallest and largest frequency to be considered valid.
#endif

/*****************************************************************************
 *  Global Variables
 *****************************************************************************/

/*****************************************************************************
 *  Local Variables
 *****************************************************************************/

static uint16_t FTMcount[RADIO_CLK_BUFFER_SIZE+1]; // Buffer holding an array of captured FTM values
                                                   // We add one because of a strange observation made while debugging.
                                                   // Read the note in DMA_Complete_IRQ_ISR() to know more

static uint32_t FTM[FREQUENCY_MOVING_WINDOW];      // FTM values of the TXCO frequency counting
static uint32_t extend = 0;                        // Extend FTM clock range to compensate for clock rollover
static uint16_t prevTime;                          // Previous FTM capture
static uint32_t DMAcntr = 0;                       // DMA Major loop counter
static uint8_t  radioNum = 0;                      // Radio being used for DMA/TCXO measurement
static uint32_t freqFilter[FREQUENCY_MOVING_WINDOW] = {0}; // Initialize with bad values
       uint32_t DMA_Complete_IRQ_ISR_Timestamp = 0; // Used for a watchdog to make sure we are always doing TCXO triming
static uint64_t coreToBusClockRatio;
       uint32_t DMAint;                            // When the DMA interrupt happened in CYCCNT units

typedef struct{
   PORT_MemMapPtr portAddr;           /* PORTA_BASE_PTR PORTB_BASE_PTR etc. */
   uint32_t       pin;                /* GPIO_PIN1   GPIO_PIN2 ....  GPIO_PIN31 */
   uint32_t       DMASource;
} GPIO0_CONFIG_t;

static const GPIO0_CONFIG_t radioGpioConfig[] = { // Radio GPIO 0 with DMA enabled/disabled
#if ( EP == 1 )                                             // All NICs
   { PORTC_BASE_PTR, GPIO_PIN5,  PORTC_DMA_SRC },           // Radio 0 GPIO 0
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A )  // T-board 101-9975T
   { PORTC_BASE_PTR, GPIO_PIN19, PORTC_DMA_SRC },           // Radio 0 (U901) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN28, PORTE_DMA_SRC },           // Radio 1 (U800) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN26, PORTE_DMA_SRC },           // Radio 2 (U700) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN12, PORTE_DMA_SRC },           // Radio 3 (U500) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN27, PORTE_DMA_SRC },           // Radio 4 (U600) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN11, PORTE_DMA_SRC },           // Radio 5 (U400) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN7,  PORTE_DMA_SRC },           // Radio 6 (U300) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN6,  PORTE_DMA_SRC },           // Radio 7 (U100) GPIO 0
   { PORTE_BASE_PTR, GPIO_PIN10, PORTE_DMA_SRC },           // Radio 8 (U200) GPIO 0
#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A ) // T-board 101-9985T
   { PORTA_BASE_PTR, GPIO_PIN11, PORTA_DMA_SRC },           // Radio 0 (U901) GPIO 0 There is only one TCXO on 9985T
#else
#error "unsuported hardware in radio_TCXO.c"
#endif
};

/*****************************************************************************
 *  Local Function Declarations
 *****************************************************************************/

/******************************************************************************

 Function Name: Disable_DMA_Reset_Filter

 Purpose: Disable all DMAs and reset filter.

 Arguments:

 Returns:

 Notes:

******************************************************************************/
static void Disable_DMA_Reset_Filter( void )
{
   uint32_t i; // Loop counter

   // Disable all DMA configuration
   for ( i=0; i<(sizeof(radioGpioConfig)/sizeof(GPIO0_CONFIG_t)); i++) {
      PORT_PCR_REG( radioGpioConfig[i].portAddr, radioGpioConfig[i].pin ) &= ~PORT_PCR_IRQC_MASK; // Interrupt/DMA request disabled
   }
   // Reset all filters
   (void)memset( freqFilter, 0, sizeof(freqFilter) );
   (void)memset( FTM,     0, sizeof(FTM) );
}

/******************************************************************************

 Function Name: DMA_Complete_IRQ_ISR

 Purpose: Handle the DMA major loop interrupt.

 Arguments:

 Returns:

 Notes: There is something strange about the DMA
        The first value in the array is a duplicate of the last value from the previous major loop.
        I don't know why this is happening. It could be cause by the fact that the DMA is in a form of auto reload.
        Anyway, the code compensates for that.

******************************************************************************/
static void DMA_Complete_IRQ_ISR( void )
{
          uint32_t i,j;
   static bool     computingTCXOfreq = false;
          bool     PrevComputingTCXOfreq;
#if ( DCU == 1)
          uint32_t cpuFreq; // Estimated CPU frequency
          TIME_SYS_SOURCE_e cpuSource;
#endif
          uint32_t TCXOFreq; // Estimated TCXO frequency
          uint32_t estFreq; // Estimated frequency
   static uint32_t freqIndex=0;
          uint32_t largestDiff;
          uint32_t min = 0xFFFFFFFF; // Initialize with largest value
          uint32_t max = 0;          // Initialize with smalest value
          uint32_t minPos = 0, maxPos = 0;
   static uint32_t movingWindowPos=0;
          uint32_t prevMovingWindowPos;
          uint32_t sum;

   DMA_CINT = RADIO_CLK_DMA_CH; // Ack interrupt

   // Handle FTM timer wrap around
   if ( FTMcount[RADIO_CLK_BUFFER_SIZE] < prevTime ) {
      extend += 0x00010000; // Extent the FTM 16 bits counter to 32 bits.
                            // There won't be more than one wrap arround with the current code and CPU speed.
   }

   // Save value for next time
   prevTime = FTMcount[RADIO_CLK_BUFFER_SIZE];

   // Capture end of frequency counting
   if ( DMAcntr >= DMA_CNTR ) {

      KERNEL_DATA_STRUCT_PTR  kd_ptr = _mqx_get_kernel_data();
      DMAint = kd_ptr->CYCCNT; // Increment system and power-up time

      coreToBusClockRatio = getCoreClock()/getBusClock(); // Always reload that value because it can change on the 9985T during DFW.
      FTM[movingWindowPos] = extend + FTMcount[RADIO_CLK_BUFFER_SIZE];

      prevMovingWindowPos = ( (movingWindowPos + FREQUENCY_MOVING_WINDOW) - 1 ) % FREQUENCY_MOVING_WINDOW;

      // Just a sanitiy check
      if ( FTM[movingWindowPos] > FTM[prevMovingWindowPos] ) {
         PrevComputingTCXOfreq = computingTCXOfreq; // Save computattion source (CPU or TCXO) for later

         // We either trim the radio TCXO from the CPU or the CPU from the radio TCXO
         // To trim the radio TCXO from the CPU we need:
         //    - A valid CPU frequency
         //    - A valid PPS which implies that the CPU frequency was trimmed against the GPS. In other words, it's reliable.
         // If we can't trim the TCXO agasint the CPU then we do the opposite. We trim the CPU against the radio TCXO.

#if ( DCU == 1 )
         // Check if the CPU frequency is valid and PPS is valid
         if ( TIME_SYS_GetRealCpuFreq( &cpuFreq, &cpuSource, NULL ) && (cpuSource == eTIME_SYS_SOURCE_GPS) && TIME_SYS_IsGpsPresent() ) {
            // Compute actual TCXO frequency from CPU frequency
            estFreq = (uint32_t)(((uint64_t)cpuFreq*(uint64_t)30000000*SAMPLING_TIME)/(coreToBusClockRatio*(uint64_t)(FTM[movingWindowPos]-FTM[prevMovingWindowPos])));
            computingTCXOfreq = true;
         } else
#endif
         {
            // Compute actual CPU frequency from TCXO frequency
            (void)RADIO_TCXO_Get( radioNum, &TCXOFreq, NULL, NULL );
            estFreq = (uint32_t)((coreToBusClockRatio*(uint64_t)(FTM[movingWindowPos]-FTM[prevMovingWindowPos])*(uint64_t)TCXOFreq)/((uint64_t)30000000*SAMPLING_TIME));
            computingTCXOfreq = false;
         }

         // Do we need to reset average array because we switched mode?
         if ( PrevComputingTCXOfreq != computingTCXOfreq ) {
            (void)memset( freqFilter, 0, sizeof(freqFilter));
         }

         // Always update the frequency filter
         freqFilter[freqIndex++] = estFreq;
         freqIndex %= FREQUENCY_MOVING_WINDOW;

         // Find largest distance between all frequencies and compute sum
         sum=0;
         for (i=0; i<FREQUENCY_MOVING_WINDOW; i++)
         {
            sum += freqFilter[i];
            if ( min > freqFilter[i] ) {
               min = freqFilter[i];
               minPos = i;
            }
            if ( max < freqFilter[i] ) {
               max = freqFilter[i];
               maxPos = i;
            }
         }
         largestDiff = max - min;

         // The frequency spread needs to be less than some threshold to be considered valid
         if ( largestDiff < MAX_FREQUENCY_DIFF ) {
            // We have a valid frequency

            // Compute average but reject the min and max outliers. Just in case some freqency is wrong.
            sum=0;
            for (i=0,j=0; i<FREQUENCY_MOVING_WINDOW; i++) {
               if ( (minPos != i) && (maxPos != i) ) {
                  sum += freqFilter[i];
                  j++; // We use an external counter just in case minPos and maxPos are the same. This way the average should always be computed properly.
               }
            }

            if ( j ) { // Make sure we avoid division by 0
               // Update either the TCXO (against the CPU) or CPU frequency (against the TCXO)
               if ( computingTCXOfreq ) {
                  (void)RADIO_TCXO_Set( radioNum, sum/j, eTIME_SYS_SOURCE_CPU, (bool)false );
#if ( DCU == 1 )
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
                  // There is only one TCXO driving all radios on 9985T so propagate radio 0 timing to all other radios.
                  for ( i=RADIO_1; i < MAX_RADIO; i++ ) {
                     (void)RADIO_TCXO_Set( i, sum/j, eTIME_SYS_SOURCE_CPU, (bool)false );
                  }
#else
                  uint32_t currRadio = radioNum; // Current radio

                  // Find next radio to trim
                  // If we can't find a radio then default to radio 0. It's always available.
                  for (;;) {
                     radioNum = (radioNum+1) % (uint8_t)MAX_RADIO; // Go to next radio
                     if ( (RADIO_RxChannelGet(radioNum) != PHY_INVALID_CHANNEL) || // Valid radio in RX so TCXO output is available
                          (radioNum == (uint8_t)RADIO_0) )                                  // Back to radio 0
                     {
                        break; // Found a new radio to trim
                     }
                  }

                  // Change radio if needed
                  if ( currRadio != radioNum ) {
                     // Only on the TBs do we need to go through all radios.
                     Disable_DMA_Reset_Filter(); // Stop DMA interrupts on all pins

                     // DMA multiplexer settings */
                     DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, RADIO_CLK_DMA_CH ) = 0; // Disable DMA mux before programming
                     DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, RADIO_CLK_DMA_CH ) = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE( radioGpioConfig[radioNum].DMASource ); // Connect DMA mux to radio TCXO clock

                     PORT_PCR_REG( radioGpioConfig[radioNum].portAddr, radioGpioConfig[radioNum].pin ) |= PORT_PCR_IRQC(1); // Set for DMA request on rising edge
                  }
#endif
#endif
               } else {
                  TIME_SYS_SetRealCpuFreq( sum/j, eTIME_SYS_SOURCE_TCXO, (bool)false );
//                  DBG_LW_printf("RADIO_Update_Freq: CPU frequency %u\n", (uint32_t)(sum/j));
               }
            }
         }
      }
      movingWindowPos = (movingWindowPos+1) % FREQUENCY_MOVING_WINDOW;
      DMAcntr = 0; // Reset DMA Major loop counter
   }
   DMAcntr++;

   DMA_Complete_IRQ_ISR_Timestamp = DWT_CYCCNT;
}

/******************************************************************************

 Function Name: RADIO_Update_Freq

 Purpose: Compute the actual radio TCXO frequency based on GPS (on DCU2+) or compute the CPU frequency based on radio TCXO

 Arguments:

 Returns:

 Notes:

******************************************************************************/
void RADIO_Update_Freq( void )
{
   Disable_DMA_Reset_Filter();

   DMAcntr  = 0; // Reset DMA Major loop counter

   // Disable DMA channel before configuration
   DMA_CERQ = RADIO_CLK_DMA_CH; // Disable DMA channel before programming
   DMA_CINT = RADIO_CLK_DMA_CH; // Clear any pending interrupts

   // Install DMA interrupt handler
   (void)_bsp_int_disable( (_mqx_uint)((int)INT_DMA0+RADIO_CLK_DMA_CH) );
   if ( NULL != _int_install_isr( (int)INT_DMA0+RADIO_CLK_DMA_CH, ( INT_ISR_FPTR )DMA_Complete_IRQ_ISR, NULL ) )
   {
      (void)_bsp_int_init( (_mqx_uint)((int)INT_DMA0+RADIO_CLK_DMA_CH), 4, 0, (bool)false); // Sets priority of interrupts, loads vector table
   }
   (void)_bsp_int_enable( (_mqx_uint)((int)INT_DMA0+RADIO_CLK_DMA_CH) );

   // Source settings
   DMA_SADDR_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = (uint32_t)&FTM3_CNT; // Transfer FTM3_CNT value to destination address. FTM3 is common to TBs and EPs.
   DMA_SOFF_REG(  DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = 0; // No address shift after each transfer
   DMA_SLAST_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = 0; // No address adjustment after major loop is completed

   // Destination settings
   DMA_DADDR_REG(     DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = (uint32_t)FTMcount;
   DMA_DOFF_REG(      DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = 2; // Increment destination address by 2 bytes after each transfer
   DMA_DLAST_SGA_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = (uint32_t)(-(RADIO_CLK_BUFFER_SIZE+1)*2); //lint !e501 !e648 Adjust destination address back to starting position after major loop complition

   // Transfer size settings
   DMA_ATTR_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1); // Source is 2 bytes, destination is 2 bytes for each transfer

   // Minor/Major loop setttings
   DMA_CITER_ELINKNO_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = RADIO_CLK_BUFFER_SIZE+1;
   DMA_BITER_ELINKNO_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = RADIO_CLK_BUFFER_SIZE+1;
   DMA_NBYTES_MLNO_REG(   DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = 2; // Transfer 2 bytes for each minor loop (i.e. each FTM interrupt)

   // Enable interrupts at the end of major loop
   DMA_CSR_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = 0; // DONE flag has to be cleared before setting MAJORELINK
   DMA_CSR_REG( DMA_BASE_PTR, RADIO_CLK_DMA_CH ) = DMA_CSR_INTMAJOR_MASK | DMA_CSR_MAJORELINK_MASK | DMA_CSR_MAJORLINKCH( RADIO_CLK_DMA_CH );

   // DMA multiplexer settings */
   DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, RADIO_CLK_DMA_CH ) = 0; // Disable DMA mux before programming
   DMAMUX_CHCFG_REG( DMAMUX_BASE_PTR, RADIO_CLK_DMA_CH ) = DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE( radioGpioConfig[0].DMASource ); // Connect DMA mux to radio TCXO clock

   // Kick start TCXO/CPU trimming with radio 0.
   PORT_PCR_REG( radioGpioConfig[0].portAddr, radioGpioConfig[0].pin ) &= ~PORT_PCR_IRQC_MASK; // Radio 0 interrupt/DMA request disabled
   PORT_PCR_REG( radioGpioConfig[0].portAddr, radioGpioConfig[0].pin ) |=  PORT_PCR_IRQC(1);   // Radio 0 set for DMA request on rising edge

   // Reset values
   radioNum = 0;

   // Enable DMA transfer
   DMA_SERQ = RADIO_CLK_DMA_CH;
}

