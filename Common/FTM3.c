/***********************************************************************************************************************
 *
 * Filename:    FTM3.c
 *
 * Global Designator: FTM3_
 *
 * Contents: Functions related to FlexTimer module
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include <bsp.h>
#include "DBG_SerialDebug.h"
#include "FTM.h"

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// Number of channels available for this FTM on K24, K64 and K66
#define MAX_CHANNELS 8 //lint !e767  macro 'MAX_CHANNELS' was defined differently in another module

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
static void (*FTM3_Channel_Callback[MAX_CHANNELS])(void);   // Channel interrupt callback
//static void (*FTM3_TO_Callback[MAX_CHANNELS])(void); // Timer overflow callback. Not needed for now

static bool moduleIsInit = (bool)false;    // Module was initialized

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
static void FTM3_INT_Handler( void );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/*****************************************************************************************************************
 *
 * Function name: FTM3_init
 *
 * Purpose: Initialize the FTM3 module module
 *
 * Arguments: None
 *
 * Returns: returnStatus_t eSUCCESS/eFAILURE indication
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t FTM3_Init( void )
{
   uint32_t i; // loop counter;
   uint32_t status;

   (void)_bsp_int_disable( (_mqx_uint)INT_FTM3 );
   (void)_bsp_int_init( (_mqx_uint)INT_FTM3, 4, 0, (bool)false); // Sets priority of interrupts, loads vector table

   // Enable FTM module
   SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK;

   // Configure capture timer
   FTM3_CNT  = 0;      // Contains the FTM counter value. Starting value.
   FTM3_MOD  = 0xFFFF; // Generate the overflow interrupt (TOF) at next clock.
   FTM3_MODE = 0;      // Default mode
   FTM3_CONF = 0;      // Need to set how many time to skip TOF overflow
   FTM3_SC   = FTM_SC_CLKS(1) | FTM_SC_PS(0); // Select system clock as clock source. No clock prescaling.

   // Reset all channels to default state
   for ( i=0; i<MAX_CHANNELS; i++ ) {
     FTM_CnSC_REG(FTM3_BASE_PTR, i) = 0;
     FTM3_Channel_Callback[i] = NULL;
   }

   // Clear all pending status by reading the register then writing 0 back
   status = FTM3_STATUS;
   FTM3_STATUS ^= status;

   moduleIsInit = (bool)true;

   return eSUCCESS;
}

/*****************************************************************************************************************
 *
 * Function name: FTM3_INT_Handler
 *
 * Purpose: FTM3 interrupt handler
 *          Call all the callbacks depending on the interrupts
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
static void FTM3_INT_Handler( void )
{
   uint32_t   i; // Loop counter

   // Process each channel for interrupt
   for ( i=0; i<MAX_CHANNELS; i++) {
      // Check for channel interrupt
      if ( FTM_CnSC_REG(FTM3_BASE_PTR, i) & FTM_CnSC_CHF_MASK ) {
         // Call callback if needed
         // If an interrupt was generated but no callback is present then that interrupt should be handled by DMA in which case no need to ack the interrupt. The DMA will take care of it.
         if ( FTM3_Channel_Callback[i] ) {
            FTM_CnSC_REG(FTM3_BASE_PTR, i) &= ~FTM_CnSC_CHF_MASK; // Acknowledge channel interrupt
            FTM3_Channel_Callback[i]();
         }
      }
   }

}

/*****************************************************************************************************************
 *
 * Function name: FTM3_Channel_Enable
 *
 * Purpose: Configure FTM3 channel and callback
 *
 * Arguments: Channel - Channel number to configure
 *            SC      - Status and control configuration
 *            channel_callback - Function to call if channel X generated the interrupt
 *
 * Returns: eSUCCESS or eFAILURE
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t FTM3_Channel_Enable (uint32_t Channel, uint32_t SC, void (*channel_callback)(void) )
{
   returnStatus_t retVal = eFAILURE;   /* Return value */

   if ( moduleIsInit ) {
      if ( Channel < MAX_CHANNELS ) {
         (void)_bsp_int_disable( (_mqx_uint)INT_FTM3 );

         // Configure channel status and control
         FTM_CnSC_REG(FTM3_BASE_PTR, Channel) = SC;

         // Configure callback
         FTM3_Channel_Callback[Channel] = channel_callback; // Channel interrupt callback

         if( !_int_install_isr( (_mqx_uint)INT_FTM3,( INT_ISR_FPTR ) FTM3_INT_Handler, NULL ))
         {
            ERR_printf("Installing FTM3 IRQ handler failed.");
            _task_block();
         }

         // Clear any pending interrupt
         FTM3_SC &= ~FTM_SC_TOF_MASK; // Clear Timer Overflow Flag
         FTM_CnSC_REG(FTM3_BASE_PTR, Channel) &= ~FTM_CnSC_CHF_MASK; // Acknowledge channel interrupt

         (void)_bsp_int_enable( (_mqx_uint)INT_FTM3 );

         retVal = eSUCCESS;
      }
   } else {
      ERR_printf("FTM3 is not initialized");
   }
   return retVal;
}

/*****************************************************************************************************************
 *
 * Function name: FTM3_Channel_Disable
 *
 * Purpose: Disable FTM3 channel
 *
 * Arguments: Channel - Channel number to configure
 *
 * Returns: eSUCCESS or eFAILURE
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t FTM3_Channel_Disable (uint32_t Channel)
{
   returnStatus_t retVal = eFAILURE;   /* Return value */

   if ( moduleIsInit ) {
      if ( Channel < MAX_CHANNELS ) {
         // Disable channel status and control
         FTM_CnSC_REG(FTM3_BASE_PTR, Channel) = 0;

         // Clear any pending interrupt
         FTM_CnSC_REG(FTM3_BASE_PTR, Channel) &= ~FTM_CnSC_CHF_MASK; // Acknowledge channel interrupt

         FTM3_Channel_Callback[Channel] = NULL; // This callback is no longer needed

         retVal = eSUCCESS;
      }
   } else {
      ERR_printf("FTM3 is not initialized");
   }
   return retVal;
}

