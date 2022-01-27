/******************************************************************************
 *
 * Filename: WDOG.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#ifndef __BOOTLOADER
#include <mqx.h>
#include <bsp.h>
#else
#include <MK24F12.h>
#endif   /* BOOTLOADER  */
#include "error_codes.h"
#include "BSP_aclara.h"

/* #DEFINE DEFINITIONS */
#define WATCHDOG_CLOCK_DIVIDE_PRESCALER 4 /* This value is 1 less than the actual prescaler/divider used */
#define WATCHDOG_HW_TIMEOUT 30 /* Time in seconds for the Watchdog hardware to reset the board if not refreshed */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: WDOG_Init

  Purpose: This function will intialize the Watchdog, and enable it

  Arguments:

  Returns: returnStatus_t - eSUCCESS

  Notes:

*******************************************************************************/
returnStatus_t WDOG_Init ( void )
{
   /* Unlock the Watchdog Registers for writing/updating */
   WDOG_UNLOCK = 0xC520;
   WDOG_UNLOCK = 0xD928;

#ifdef NDEBUG /* If defined, this is release code (not debug) */
   /* Bus Clock, enable watchdog */
   WDOG_STCTRLH |= (WDOG_STCTRLH_CLKSRC_MASK | WDOG_STCTRLH_WDOGEN_MASK);
#else /* NDEBUG */
   /* enable interrupt before reset, Bus Clock, enable watchdog */
   WDOG_STCTRLH |= (WDOG_STCTRLH_IRQRSTEN_MASK | WDOG_STCTRLH_CLKSRC_MASK | WDOG_STCTRLH_WDOGEN_MASK);
#endif /* NDEBUG */
   WDOG_TOVALH = (uint16_t)((((BSP_BUS_CLOCK / (WATCHDOG_CLOCK_DIVIDE_PRESCALER+1)) * WATCHDOG_HW_TIMEOUT) >> 16) & 0x0000FFFF);
   WDOG_TOVALL = (uint16_t)(((BSP_BUS_CLOCK / (WATCHDOG_CLOCK_DIVIDE_PRESCALER+1)) * WATCHDOG_HW_TIMEOUT) & 0x0000FFFF);
   /* Set Prescaler to (4+1=5) - so Bus Clock / prescaler gives WDOG_CLK */
   WDOG_PRESC = WDOG_PRESC_PRESCVAL(WATCHDOG_CLOCK_DIVIDE_PRESCALER);
   return(eSUCCESS);
} /* end WDOG_Init () */

/*******************************************************************************

  Function name: WDOG_Kick

  Purpose: This function is used to Kick the Watchdog - refresh the timeout

  Arguments:

  Returns:

  Notes:

*******************************************************************************/
void WDOG_Kick ( void )
{
#ifndef __BOOTLOADER
   OS_INT_disable( ); //The second sequence 0xB480 should be written within 20 cycles of first
#endif   /* BOOTLOADER  */

   /* Kick (refresh) the Watchdog */
   WDOG_REFRESH = 0xA602;
   WDOG_REFRESH = 0xB480;

#ifndef __BOOTLOADER
   OS_INT_enable( );
#endif   /* BOOTLOADER  */
} /* end WDOG_Kick () */
/*******************************************************************************

  Function Name:   WDOG_Disable
  Purpose:         This function disabled the Watchdog timer
  Arguments:       void
  Returned Value:  void
  Notes:
     The CPU will reset if attempting to single step through this function!!
*******************************************************************************/

void WDOG_Disable( void )
{
   WDOG_MemMapPtr reg = WDOG_BASE_PTR;

   /* NOTE: DO NOT SINGLE STEP THROUGH THIS FUNCTION!!!
      There are timing requirements for the execution of the unlock. If
      you single step through the code you will cause the CPU to reset.
   */

   /* unlock watchdog */
   reg->UNLOCK = 0xc520;
   reg->UNLOCK = 0xd928;

   /* disable watchdog */
   reg->STCTRLH &= ~( WDOG_STCTRLH_WDOGEN_MASK );
}  /* End BL_MAIN_WdogDisable() */
