// <editor-fold defaultstate="collapsed" desc="File Header Information">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:   sys_busy.c
 *
 * Global Designator: SYSBUSY_
 *
 * Contents: This module is called by an application to indicate that the application is processing data.  DWF may use
 *           this module to determine if it is a good time to reset the processor and patch the firmware.
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
 * $Log$ Created Jan 2, 2014
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 01/02/2014 - Initial Release
 *
 * @version    0.1
 * #since      2014-01-02
 **********************************************************************************************************************/
 // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#define sys_busy_GLOBALS
#include "sys_busy.h"
#undef  sys_busy_GLOBALS
#include "DBG_SerialDebug.h"

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define SYSBUSY_MAX_CNTR_VAL     ((uint16_t)0xFFFF)  /* Maximum value for busyCounter_, see busyCounter_ */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Constants">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Variables (File Static)">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static uint16_t     busyCounter_;            /* When counter is 0, no apps are busy.  Note: See SYSBUSY_MAX_CNTR_VAL */
static OS_MUTEX_Obj busyMutex_;              /* Serialize access to the Banked driver */
static eSysState_t  busyState_ = eSYS_IDLE;  /* Initialize busy state to idle. */

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************
 *
 * Function Name: SYSBUSY_init
 *
 * Purpose: Initializes this module
 *
 * Arguments: none
 *
 * Returns: returnStatus_t
 *
 * Side Effects: This module will return a busy status until the busyCounter_ variable is decremented to zero.
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t SYSBUSY_init(void)
{
   returnStatus_t retVal = eFAILURE;   /* Return value */

   if ( OS_MUTEX_Create(&busyMutex_) )
   {
      busyState_ = eSYS_IDLE;
      busyCounter_ = 0;
      retVal = eSUCCESS;
   }
   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function Name: SYSBUSY_setBusy
 *
 * Purpose: Sets the status to busy
 *
 * Arguments: none
 *
 * Returns: none
 *
 * Side Effects: This module will return a busy status until the busyCounter_ variable is decremented to zero.
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void SYSBUSY_setBusy(void)
{
   OS_MUTEX_Lock(&busyMutex_); /* Take the mutex */ // Function will not return if it fails
   if (SYSBUSY_MAX_CNTR_VAL != busyCounter_)  /* The busyCounter_ < than the maximum value it can hold? */
   {
      busyCounter_++;
      busyState_ = eSYS_BUSY;
   }
   OS_MUTEX_Unlock(&busyMutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function Name: SYSBUSY_clearBusy
 *
 * Purpose: Decrements the busyCounter_ and sets the status to idle when busyCounter_ is 0.
 *
 * Arguments: none
 *
 * Returns: none
 *
 * Side Effects: This module will return a busy status until the busyCounter_ variable is decremented to zero.
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void SYSBUSY_clearBusy(void)
{
   OS_MUTEX_Lock(&busyMutex_); /* Take the mutex */ // Function will not return if it fails
   if (0 != busyCounter_)
   {
      if (0 == --busyCounter_)
      {
         busyState_ = eSYS_IDLE;
      }
   }
   OS_MUTEX_Unlock(&busyMutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function Name: SYSBUSY_isBusy
 *
 * Purpose: Returns the busy status.
 *
 * Arguments: None
 *
 * Returns: eSysState_t busyState - eSYS_BUSY or eSYS_IDLE
 *
 * Side Effects: This module will return a busy status until the busyCounter_ variable is decremented to zero.
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
eSysState_t SYSBUSY_isBusy(void)
{
   eSysState_t retVal;

   OS_MUTEX_Lock(&busyMutex_); // Function will not return if it fails
   retVal = busyState_;
   OS_MUTEX_Unlock(&busyMutex_); // Function will not return if it fails

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function Name: SYSBUSY_lockIfIdle
 *
 * Purpose: Locks the mutex, if system is idle. Returns the busy status.
 *
 * Arguments: None
 *
 * Returns: eSysState_t busyState:  eSYS_IDLE: System was idle and locked.
 *
 * Side Effects: Locks the mutex, if the system is idle so that no other task gets in.
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
eSysState_t SYSBUSY_lockIfIdle(void)
{
   eSysState_t retVal;

   OS_MUTEX_Lock(&busyMutex_); // Function will not return if it fails
   if ( busyState_ != eSYS_IDLE )
   {  //System is busy, release the mutex
      OS_MUTEX_Unlock(&busyMutex_); // Function will not return if it fails
   }
   retVal = busyState_;

   return(retVal);
}


/***********************************************************************************************************************
 *
 * Function Name: SYSBUSY_unlock
 *
 * Purpose: Unlocks the mutex
 *
 * Arguments: None
 *
 * Returns: none
 *
 * Side Effects: Unlocks the mutex
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void SYSBUSY_unlock(void)
{
   OS_MUTEX_Unlock(&busyMutex_); /*lint !e455, called to unlock, if locked by SYSBUSY_isBusyAndLock */ // Function will not return if it fails
}
