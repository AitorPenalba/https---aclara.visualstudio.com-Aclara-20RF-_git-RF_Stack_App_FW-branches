/***********************************************************************************************************************
 *
 * Filename:   pwr_restore.h
 *
 * Global Designator: PWR_
 *
 * Contents: Last gasp message build and transmission.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2022 Aclara.  All Rights Reserved.
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

#ifndef pwr_restore_H
#define pwr_restore_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "time_util.h"
#include "EVL_event_log.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

#ifdef PWROR_GLOBALS
   #define PWROR_EXTERN
#else
   #define PWROR_EXTERN extern
#endif

PWROR_EXTERN returnStatus_t PWROR_init(void);
PWROR_EXTERN void PWROR_Task( taskParameter );
PWROR_EXTERN void PWROR_HMC_signal(void);
PWROR_EXTERN void PWROR_PWR_signal(void);
PWROR_EXTERN void PWROR_printOutageStats( sysTimeCombined_t outageTime, sysTimeCombined_t restorationTime );

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
extern returnStatus_t PWROR_tx_message( sysTimeCombined_t outageTime, const LoggedEventDetails *outageEventData,
                                        sysTimeCombined_t restorationTime, const LoggedEventDetails *restEventData );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

#endif // #ifndef pwr_restore_H
