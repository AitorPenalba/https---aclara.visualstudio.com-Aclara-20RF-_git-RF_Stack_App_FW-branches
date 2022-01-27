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
 * Copyright 2015-2018 Aclara.  All Rights Reserved.
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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "error_codes.h"

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
   // Verify whether or not the VBAT register has been initialized since the last time
   // that the VBAT cap was drained completely.
   if ((0 == VBATREG_VALID) || (VBATREG_VALID_SIG != VBATREG_SIG))
   {
      // TODO: DG: Is this necessary?? VBAT is cleared if Super cap is drained completely!!!
      // The VBAT register is un-initalized.  Initialize all bits to zero, set the
      // signature and mark the VBAT register as valid.
      (void)memset( VBATREG_RFSYS_BASE_PTR, 0, sizeof(VBATREG_VbatRegisterFile) );

      VBATREG_SIG = VBATREG_VALID_SIG;
      VBATREG_VALID = 1;
   }
   else
   {
      // The VBAT register was marked as valid at power up.  This indicates that the
      // VBAT cap was not drained completely while the MTU was powered down.  Check
      // the RTC status register to make sure the real-time clock is valid.
      if (RTC_SR_TIF_MASK == (VBATREG_RTC_SR() & RTC_SR_TIF_MASK)) // TODO 2016-02-02 SMG Change to call into PWRLG
      {
         VBATREG_RTC_VALID = 0;
      }

      // Zero out unused bit fields.
      VBATREG_UNUSED_BIT_FIELD1 = 0;
      VBATREG_UNUSED_BIT_FIELD2 = 0;
   }

   return(eSUCCESS);
}

