/******************************************************************************
 *
 * Filename: BSP_aclara.c
 *
 * Global Designator:
 *
 * Contents: This file contains all of the Board Support Package functions that
 *           don't really fit into any of the existing modules under the BSP
 *
 ******************************************************************************
 * Copyright (c) 2012 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#if ( MCU_SELECTED == NXP_K24 )
#include <bsp.h>
#endif
#include "BSP_aclara.h"
#include "DBG_SerialDebug.h"

//#include "ADCDAC.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
#if ( MCU_SELECTED == NXP_K24 )
static bool Vref_Initialized = false;
#endif
/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

#if ( RTOS_SELECTION == MQX_RTOS )
/*******************************************************************************

  Function name: BSP_Get_BspRevision

  Purpose: This function will return the current BSP Revision (from MQX BSP)

  Arguments:

  Returns: BspRevision - pointer to the BSP Revision string

  Notes:

*******************************************************************************/
char const *BSP_Get_BspRevision ( void )
{
   return ( _mqx_bsp_revision ); /*lint !e605*/
}

/*******************************************************************************

  Function name: BSP_Get_IoRevision

  Purpose: This function will return the current BSP IO Revision (from MQX BSP IO)

  Arguments:

  Returns: IoRevision - pointer to the BSP IO Revision string

  Notes:

*******************************************************************************/
char const *BSP_Get_IoRevision ( void )
{
  return ( _mqx_io_revision ); /*lint !e605*/
}

/*******************************************************************************

  Function name: BSP_Get_PspRevision

  Purpose: This function will return the current PSP Revision (from MQX)

  Arguments:

  Returns: PspRevision - pointer to the PSP Revision string

  Notes:

*******************************************************************************/
char const *BSP_Get_PspRevision ( void )
{
  return ( _mqx_psp_revision ); /*lint !e605*/
}
#endif // #if ( RTOS_SELECTION == MQX_RTOS )
/*******************************************************************************

  Function name: BSP_Get_ResetStatus

  Purpose: This function will return all the status flags for the cause of the last Reset

  Arguments:

  Returns: ResetStatus - bit field of the last Reset source

  Notes:

*******************************************************************************/
uint16_t BSP_Get_ResetStatus ( void )
{
#if ( MCU_SELECTED == NXP_K24 )
   uint8_t ResetReg, srs0, srs1;
   uint16_t ResetStatus = 0x0000;
   static bool firstCall = (bool)true;

   ResetReg = srs0 = RCM_SRS0;
   if ( ResetReg & RCM_SRS0_POR_MASK )
   {
      ResetStatus |= RESET_SOURCE_POWER_ON_RESET;
   } /* end if() */
   if ( ResetReg & RCM_SRS0_PIN_MASK )
   {
      ResetStatus |= RESET_SOURCE_EXTERNAL_RESET_PIN;
   } /* end if() */
   if ( ResetReg & RCM_SRS0_WDOG_MASK )
   {
      ResetStatus |= RESET_SOURCE_WATCHDOG;
   } /* end if() */
   if ( ResetReg & RCM_SRS0_LOC_MASK )
   {
      ResetStatus |= RESET_SOURCE_LOSS_OF_EXT_CLOCK;
   } /* end if() */
   if ( ResetReg & RCM_SRS0_LVD_MASK )
   {
      ResetStatus |= RESET_SOURCE_LOW_VOLTAGE;
   } /* end if() */
   if ( ResetReg & RCM_SRS0_WAKEUP_MASK )
   {
      ResetStatus |= RESET_SOURCE_LOW_LEAKAGE_WAKEUP;
   } /* end if() */

   ResetReg = srs1 = RCM_SRS1;
   if ( ResetReg & RCM_SRS1_SACKERR_MASK )
   {
      ResetStatus |= RESET_SOURCE_STOP_MODE_ACK_ERROR;
   } /* end if() */
   if ( ResetReg & RCM_SRS1_EZPT_MASK )
   {
      ResetStatus |= RESET_SOURCE_EZPORT_RESET;
   } /* end if() */
   if ( ResetReg & RCM_SRS1_MDM_AP_MASK )
   {
      ResetStatus |= RESET_SOURCE_MDM_AP;
   } /* end if() */
   if ( ResetReg & RCM_SRS1_SW_MASK )
   {
      ResetStatus |= RESET_SOURCE_SOFTWARE_RESET;
   } /* end if() */
   if ( ResetReg & RCM_SRS1_LOCKUP_MASK )
   {
      ResetStatus |= RESET_SOURCE_CORE_LOCKUP;
   } /* end if() */
   if ( ResetReg & RCM_SRS1_JTAG_MASK )
   {
      ResetStatus |= RESET_SOURCE_JTAG;
   } /* end if() */
   if ( firstCall ) {
      INFO_printf("Reset register RCM_SRS0 0x%02X", srs0 );
      INFO_printf("Reset register RCM_SRS1 0x%02X", srs1 );
      firstCall = (bool)false;
   }

   return ( ResetStatus );
#else  // TODO: RA6E1: Add Support
   return 0;
#endif
}

#if ( MCU_SELECTED == NXP_K24 )
/* DG: Not used */
/*******************************************************************************

  Function name: BSP_Setup_VREF

  Purpose: This function is used to setup the VREF on the processor
           multiple modules may need this, so we set this up and the first call
           sets it up and the other calls don't need to worry about it.

  Arguments:

  Returns:

  Notes:

*******************************************************************************/
void BSP_Setup_VREF ( void )
{
   if ( Vref_Initialized == false )
   {
      /* Enable the VREF clock */
      SIM_SCGC4 |= SIM_SCGC4_VREF_MASK;

      VREF_SC = VREF_SC_VREFEN_MASK | VREF_SC_REGEN_MASK; // enable the BANDGAP, and regulator, but remain in bandgap-only mode
      while ( (VREF_SC & VREF_SC_VREFST_MASK) != VREF_SC_VREFST_MASK )
      {
         ;  // wait for bandgap to be ready
      }
      VREF_SC |= VREF_SC_MODE_LV(1); /*lint !e835 */ // now switch to lowpower buffered mode

      Vref_Initialized = true;
   }
}
#endif
