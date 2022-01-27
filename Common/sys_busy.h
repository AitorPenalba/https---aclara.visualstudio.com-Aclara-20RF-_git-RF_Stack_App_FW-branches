/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: sys_busy.h
 *
 * Contents: This module is called by an application to indicate that the application is processing data.  DWF may use
 *           this module to determine if it is a good time to reset the processor and patch the firmware.
 *
 * Note:  This version of the module if very simple.  The author of an application should call the clearBusy function
 *        the same number of times setBusy was called to clear the busy state.
 *
 *        There are other ways this could be done.  It could be implemented in such a way that each application is
 *        assigned an ID and once the application says it is idle, the state for that application is idle regardless of
 *        the number of times that application said it was busy.
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
#ifndef sys_busy_H_
#define sys_busy_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdint.h>
#include <stdbool.h>
#include "project.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

#ifdef sys_busy_GLOBALS
   #define sys_busy_EXTERN
#else
   #define sys_busy_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eSYS_IDLE = ((uint8_t)0),
   eSYS_BUSY
}eSysState_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * SYSBUSY_init - Initializes this module.  Must be called once before any other functions for this module are called.
 *
 * @param  None
 * @return returnStatus_t - eSUCCESS or eFAILURE
 */
returnStatus_t SYSBUSY_init(void);

/**
 * SYSBUSY_setBusy - Sets the status to busy
 *
 * @param  None
 * @return None
 */
void SYSBUSY_setBusy(void);

/**
 * SYSBUSY_clearBusy - Decrements the busyCounter_ and sets the status to idle when busyCounter_ is 0.
 *
 * @param  None
 * @return None
 */
void SYSBUSY_clearBusy(void);

/**
 * SYSBUSY_isBusy - Returns the busy status
 *
 * @param  None
 * @return eSysState_t - eSYS_BUSY or eSYS_IDLE
 */
eSysState_t SYSBUSY_isBusy(void);

/**
 * SYSBUSY_lockIfIdle - Locks the mutex, if system is idle. Returns the busy status.
 *
 * @param  None
 * @return eSysState_t - eSYS_BUSY or eSYS_IDLE
 */
eSysState_t SYSBUSY_lockIfIdle(void);

/**
 * SYSBUSY_unlock - unlocks the mutex
 *
 * @param  None
 * @return None
 */
void SYSBUSY_unlock(void);

/* ****************************************************************************************************************** */
#undef sys_busy_EXTERN

#endif
