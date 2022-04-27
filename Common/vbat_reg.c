/***********************************************************************************************************************
 *
 * Filename: vbat_reg.c
 *
 * Global Designator: VBATREG_
 *
 * Contents: VBAT Flag Register Initialization and access macros.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2015-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author David McGraw
 *
 * $Log$
 *
 ***********************************************************************************************************************
 * Revision History:
 *
 * @version
 * #since
  **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"

#define VBATREG_GLOBALS
#include "vbat_reg.h"
#undef VBATREG_GLOBALS

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */


/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */


/***********************************************************************************************************************
 *
 * Function name: VBATREG_init
 *
 * Purpose: Initialize the VBAT registers.  This needs to be done the first time an MTU is powered up or after
 *          the VBAT cap has been drained due to a prolonged power outage.  Also, verify whether or not the RTC
 *          indicates that it has a valid time.
 *
 * Arguments: None
 *
 * Returns: void
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/

returnStatus_t VBATREG_init(void)
{

   VBATREG_EnableRegisterAccess();
#if ( MCU_SELECTED == RA6E1 )
//	R_SYSTEM->VBATTMNSELR = R_SYSTEM_VBATTMNSELR_VBATTMNSEL_Msk;  /* TODO: RA6 [DG]: Do we need this?
#endif
   // Verify whether or not the VBAT register has been initialized since the last time
   // that the VBAT cap was drained completely.
   if ((0 == VBATREG_VALID) || (VBATREG_VALID_SIG != VBATREG_SIG))
   {
      // The VBAT register is un-initalized.  Initialize all bits to zero, set the
      // signature and mark the VBAT register as valid.
      (void)memset( VBATREG_RFSYS_BASE_PTR, 0, sizeof(VBATREG_VbatRegisterFile) );

      VBATREG_SIG = VBATREG_VALID_SIG;
      VBATREG_VALID = 1;
   }
   else
   {
#if ( MCU_SELECTED == NXP_K24 )
      // The VBAT register was marked as valid at power up.  This indicates that the
      // VBAT cap was not drained completely while the MTU was powered down.  Check
      // the RTC status register to make sure the real-time clock is valid.
      if (RTC_SR_TIF_MASK == (VBATREG_RTC_SR() & RTC_SR_TIF_MASK)) // TODO 2016-02-02 SMG Change to call into PWRLG
      {
         VBATREG_RTC_VALID = 0;
      }
#endif
      // Zero out unused bit fields.
      VBATREG_UNUSED_BIT_FIELD1 = 0;
      VBATREG_UNUSED_BIT_FIELD2 = 0;
   }

   VBATREG_DisableRegisterAccess();

   return(eSUCCESS);
}

/***********************************************************************************************************************
 *
 * Function name: VBATREG_EnableRegisterAccess
 *
 * Purpose: Enable access to VBTKR and Disable the Register Protection for the Battery Back-up Register
 *
 * Arguments: None
 *
 * Returns: void
 *
 * Re-entrant Code: No
 *
 * Notes: Only used for RA6E1 MCU
 *
 **********************************************************************************************************************/
void VBATREG_EnableRegisterAccess( void )
{
#if ( MCU_SELECTED == RA6E1 )
   /* Disable Register Write Protection on RA6E1 */
   R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);
   R_SYSTEM->VBTBER_b.VBAE = 0x01;  /* Enable access to VBTBKR */
   /* RA6E1 HRM: 11.2.4: Wait for at least 500 ns after writing 1 to VBAE, and then access VBTBKR. */
   R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS); /* TODO: RA6: [DG]: Update the dealy to be 500ns */
#endif
}

/***********************************************************************************************************************
 *
 * Function name: VBATREG_DisableRegisterAccess
 *
 * Purpose: Disable access to VBTKR and Enable the Register Protection for the Battery Back-up Register
 *
 * Arguments: None
 *
 * Returns: void
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
void VBATREG_DisableRegisterAccess( void )
{
#if ( MCU_SELECTED == RA6E1 )
   R_SYSTEM->VBTBER_b.VBAE = 0x00;  /* Disable access to VBTBKR */

   /* Enable Register Write Protection on RA6E1 */
   R_BSP_RegisterProtectEnable( BSP_REG_PROTECT_OM_LPC_BATT );
#endif
}


