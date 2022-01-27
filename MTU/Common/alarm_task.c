// <editor-fold defaultstate="collapsed" desc="File Header Information">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:   alarm_task.c
 *
 * Global Designator: ALRM_
 *
 * Contents: TODO
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Jeff Borlin
 *
 * $Log$ Created August 16, 2013
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - TODO xx/xx/2012 - Initial Release
 *
 * @version    0.1
 * #since      2013-08-16
 **********************************************************************************************************************/
 // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include "project.h"
#include "alarm_task.h"
#include "DBG_SerialDebug.h"

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

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

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="returnStatus_t ALRM_register(alarmId_t id, bool active, uint32_t cnt)">
/***********************************************************************************************************************
 *
 * Function Name: ALRM_register
 *
 * Purpose: Notify the alarm module that an alarm is present.
 *
 * Arguments:  alarmId_t id - Unique ID for each alarm
 *             bool active - true = Alarm is in active state, false = inactive
 *             uint32_t cnt - Number of occurrences for this alarm.
 *             sysTime_t timeStamp - Time the event was detected
 *
 * Returns: returnStatus_t - See definition in error_codes.h
 *
 * Side Effects: TODO
 *
 * Re-entrant Code: TODO
 *
 * Notes: TODO
 *
 **********************************************************************************************************************/
returnStatus_t ALRM_register(alarmId_t id, bool active, uint32_t cnt, sysTime_t timeStamp)
{
   DBG_logPrintf('I', "Alarm - ID: %d, State: %d, Cnt: %d, Date: %d, Time: %d",
              ( int32_t )id, (uint8_t)active, cnt, timeStamp.date, timeStamp.time);
   return(eSUCCESS);
}
// </editor-fold>

/* ****************************************************************************************************************** */
