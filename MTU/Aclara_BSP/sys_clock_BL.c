// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************
 *
 * Filename:   sys_clock.c
 *
 * Global Designator: SCLK_
 *
 * Contents: Configures the system clock(s)
 *
 ***********************************************************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created May 23, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 05/23/2013 - Initial Release
 * 03/31/15 - mkv - Update getBusClock() to return value from mqx instead of #define value which was incorrect.
 *
 **********************************************************************************************************************///
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Include Files">
/* INCLUDE FILES */
#include "project_BL.h"
#ifndef __BOOTLOADER
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#include <MK24F12.h>
#else

#endif // ( RTOS_SELECTION )
#endif   /* BOOTLOADER  */
#include "sys_clock_BL.h"

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Definitions">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Constant Definitions ">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="File Variable Definitions">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="Local Function Defintions">
/* ****************************************************************************************************************** */
/* LOCAL FUNCTION DEFINITIONS */

// </editor-fold>
/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="returnStatus_t SCLK_intClk( void )">
/***********************************************************************************************************************
 *
 * Function Name: SCLK_intClk
 *
 * Purpose: Switches clock to the internal low power clock
 *
 * Arguments:  None
 *
 * Returns: eSPI_Status_t - eSUCCESS or eFAILURE
 *
 * Side Effects:
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
returnStatus_t SCLK_intClk( void )
{
#if ( ENABLE_LOW_POWER_SYS_CLK == 1 )
   // MKD 2015-02-13 This code needs to be modified for Kinetis and MQX
   // SMG 2016-03-10 It would be better to use the MQX bsp colck configurations for this
   SIM_SCGC5 |= (uint32_t)0x2A00UL;     /* Enable clock gate for ports to enable pin routing */
   if ( *((uint8_t*) 0x03FFU) != 0xFFU)
   {
      MCG_C3 = *((uint8_t*) 0x03FFU);
      MCG_C4 = (MCG_C4 & 0xE0U) | ((*((uint8_t*) 0x03FEU)) & 0x1FU);
   }

   /* SIM_CLKDIV4: NFCDIV=7 (Divider output clock = Divider input clock x ((NFCFRAC+1)/(NFCDIV+1))) */
   SIM_CLKDIV4 |=  SIM_CLKDIV4_NFCDIV(7);

   /* SIM_SOPT2: PLLFLLSEL=1 (Selects MCGPLLCLK or MCGFLLCLK clk for various peripheral clocking options - MCGPLL0CLK) */
   SIM_SOPT2 = (uint32_t)((SIM_SOPT2 & (uint32_t)~0x00020000UL) | (uint32_t)0x00010000UL); /* Select PLL 0 as a clock source for various peripherals */

   /* SIM_SOPT1: OSC32KSEL=1 (RTC oscillator) */
   SIM_SOPT1 |= (uint32_t)0x00080000UL; /* RTC oscillator drives 32 kHz clock for various peripherals */

   SIM_SCGC1 |= (uint32_t)0UL; /*lint !e835 */  /* SIM_SCGC1: OSC1=0 (Clock is Disabled) */

   PORTA_PCR18 &= (uint32_t)~0x01000700UL;   /* PORTA_PCR18: ISF=0,MUX=0 */
#if 0 //Not applicable in the K22P80 device
   //TODO Determine if this could be conditionally compiled using processor type
   PORTE_PCR24 &= (uint32_t)~0x01000700UL;   /* PORTE_PCR24: ISF=0,MUX=0 */
   PORTE_PCR25 &= (uint32_t)~0x01000700UL;   /* PORTE_PCR25: ISF=0,MUX=0 */
#endif

   /* Switch to FBE Mode */
   /* OSC0_CR: ERCLKEN=0 (External reference clock is Disabled), ??=0,
    * EREFSTEN=0 (External reference clock is disabled in Stop mode),??=0,SC2P=0 (Disable the selection),
    * SC4P=0 (Disable 2pF Cap Load), SC8P=0 (Disable 8pF Cap Load), SC16P=0 (Disable 16pF Cap Load) */
   /* KAD */
   OSC0_CR = (uint8_t)0x0U;
   OSC1_CR = (uint8_t)0U; /* Disable OSC1 */

   /* Set KD */
   MCG_C7 = 1;    /* MCG_C7: OSCSEL=1 (Selects 32 kHz RTC Oscillator) */
   MCG_SC = 0;    /* Divide Factor is 1 */

   /* MCG_C10: LOCRE2=0 (Interrupt request is generated on a loss of OSC1 external reference clock), ??=0,
    * RANGE1=0 (Encoding 2 — Very high frequency range selected for the crystal oscillator),
    * HGO1=1 (Configure crystal oscillator for high-gain operation), EREFS1=1 (Oscillator requested), ??=0, ??=0 */
   MCG_C10 = (uint8_t)0x0CU;

   /* MCG_C2: LOCRE0=0 (Interrupt request is generated on a loss of OSC0 external reference clock), ??=0,
    * RANGE0=0,
    * HGO0=0 (Configure crystal oscillator for low-power operation), EREFS0=0 (External reference clock requested),
    * LP=0 (FLL (or PLL) is not disabled in bypass modes), IRCS=0 */
   /* KD */
   MCG_C2 = (uint8_t)0x0U;

   /* MCG_C1: CLKS=0(FLL clock is selected), FRDIV=0(Divide by 32), IREFS=0 (External reference clock is selected),
    * IRCLKEN=0 (MCGIRCLK inactive), IREFSTEN=0 (Internal reference clock is disabled in Stop mode) */
   /* KD */
   MCG_C1 = (uint8_t)0x0U;

   /* KD */
   MCG_C4 = (uint8_t)0xE0U; /* MCG_C4: DMX32=0,DRST_DRS=0 (Low range (reset default)) */

   /* MCG_C5: PLLREFSEL0=0 (Selects OSC0 clock source as its external reference clock),
    * PLLCLKEN0=0 (MCGPLL0CLK and MCGPLL0CLK2X are inactive),
    * PLLSTEN0=0 (MCGPLL0CLK and MCGPLL0CLK2X are disabled in any of the Stop modes),??=0,??=0,PRDIV0=0 (Div by 1) */
   /* KAD */
   MCG_C5 = (uint8_t)0x0U;

   /* MCG_C6: LOLIE0=0 (No interrupt request is generated on loss of lock), PLLS=0 (FLL is selected),
    * CME0=0 (External clock monitor is disabled for OSC0), VDIV0=8 (Multiply Factor is 24) */
   /* KD */
   MCG_C6 = (uint8_t)0x08U;

   /* MCG_C11: PLLREFSEL1=0 (Selects OSC0 clock source as its external reference clock),
    * PLLCLKEN1=0 (MCGPLL1CLK, MCGPLL1CLK2X, and MCGDDRCLK2X are inactive),
    * PLLSTEN1=0 (PLL1 clocks (MCGPLL1CLK, MCGPLL1CLK2X, and MCGDDRCLK2X) are disabled in any Stop modes),
    * PLLCS=0 (PLL0 output clock is selected), ??=0, PRDIV1=0 (PLL1 External Reference Divide Factor is 1) */
   /* KAD */
   MCG_C11 = (uint8_t)0x0U;

   /* MCG_C12: LOLIE1=0 (No interrupt request is generated on loss of lock on PLL1), ??=0,
    * CME2=0 (External clock monitor for OSC1 is disabled),
    * VDIV1=8 (PLL1 VCO Divide Factor is 16) */
   MCG_C12 = (uint8_t)0x00U;

   /* SIM_CLKDIV1: OUTDIV1=0 (Divide-by-1), OUTDIV2=1 (Divide-by-2), OUTDIV3=1 (Divide-by-2), OUTDIV4=3 (Divide-by-4),
    * ??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0,??=0 */
   SIM_CLKDIV1 = (uint32_t)0x01130000UL; /* Update system prescalers */

   while((MCG_S & MCG_S_IREFST_MASK) != 0x00U)  /* Check that the src of the FLL ref clock is the ext reference clk */
   { }

   busClk_ = getBusClock();
#endif
   return (eSUCCESS);
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void sleepIdle( void )">
/***********************************************************************************************************************
 *
 * Function Name: sleepIdle
 *
 * Purpose: Switches clock to the internal low power clock
 *
 * Arguments:  None
 *
 * Returns: None
 *
 * Side Effects:
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
void sleepIdle( void )
{
#if ( ENABLE_SLEEP_IDLE == 1 )
   // MKD 2015-02-13 This code needs to be modified for Kinetis and MQX
   SMC_PMPROT = 0x2A; // 0b00101010;
   SMC_VLLSCTRL = 1;
   SMC_PMCTRL = 0xC0;  // 0b11000000;
   asm("WFI");
#endif
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="void sleepDeep( void )">
/***********************************************************************************************************************
 *
 * Function Name: sleepDeep
 *
 * Purpose: Switches clock to the internal low power clock
 *
 * Arguments:  None
 *
 * Returns: None
 *
 * Side Effects:
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
void sleepDeep( void )
{
#if ( ENABLE_SLEEP_DEEP == 1 )
   // MKD 2015-02-13 This code needs to be modified for Kinetis and MQX
   SMC_PMPROT   = 0x2A; // 0b00101010; Allow VLP, LLS and VLLS power modes
   SMC_VLLSCTRL = 2;    // Go to VLLS2 mode
   SMC_PMCTRL   = 0xC0; // 0b11000010;  Low-Power Wake Up On Interrupt, VLPR, STOP(or VLPS, if lsb's 010 per comment)
   asm("WFI");
#endif
}
/* ****************************************************************************************************************** */
// </editor-fold>
// <editor-fold defaultstate="collapsed" desc="uint32_t getBusClock( void )">
/***********************************************************************************************************************
 *
 * Function Name: getBusClock
 *
 * Purpose: gets the peripheral bus clock
 *
 * Arguments:  None
 *
 * Returns: bus clock in Hz
 *
 * Side Effects:
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
uint32_t getBusClock( void )
{
#if ( MCU_SELECTED == NXP_K24 )
#ifndef __BOOTLOADER
   return(_bsp_get_clock( _bsp_get_clock_configuration(), CM_CLOCK_SOURCE_BUS) );
#else
   return BSP_BUS_CLOCK;
#endif
#elif ( MCU_SELECTED == RA6E1 )
#ifndef __BOOTLOADER
   return ( R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB) ); // TODO: RA6E1 Bob: Verify that PCLKB is the correct bus clock
#else

#endif // __BOOTLOADER
#endif // ( MCU SELECTED )
}
// </editor-fold>
#ifndef __BOOTLOADER
// <editor-fold defaultstate="collapsed" desc="uint32_t getCoreClock( void )">
/***********************************************************************************************************************
 *
 * Function Name: getCoreClock
 *
 * Purpose: gets the CPU core clock
 *
 * Arguments:  None
 *
 * Returns: core clock in Hz
 *
 * Side Effects:
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
uint32_t getCoreClock( void )
{
#if ( MCU_SELECTED == NXP_K24 )
   return(_bsp_get_clock( _bsp_get_clock_configuration(), CM_CLOCK_SOURCE_CORE) );
#elif ( MCU_SELECTED == RA6E1 )
   return ( R_BSP_SourceClockHzGet(FSP_PRIV_CLOCK_PLL) );
#endif
}
/* ****************************************************************************************************************** */
// </editor-fold>
#endif
