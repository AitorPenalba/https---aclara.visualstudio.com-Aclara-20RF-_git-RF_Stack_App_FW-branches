/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: alarm_task.h
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
#ifndef alarm_task_H_
#define alarm_task_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include "alarm_cfg.h"
#include "time_sys.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eALRM_OPPORTUNISTIC = 0,
   eALRM_REALTIME
}alarmType_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/**
 * ALRM_register - Notify the alarm module that an alarm is present.
 * @see TODO (reference documents or files)
 *
 * @param  alarmId_t id - Unique ID for each alarm
 * @param  bool active - true = Alarm is in active state, false = inactive
 * @param  uint32_t cnt - Number of occurrences for this alarm.
 * @param  sysTime_t timeStamp - Time the event was detected
 * @return returnStatus_t - See definition in error_codes.h
 */
extern returnStatus_t ALRM_register(alarmId_t id, bool active, uint32_t cnt, sysTime_t timeStamp);


#endif
