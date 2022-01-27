/***********************************************************************************************************************
 *
 * Filename: time_DST.h
 *
 * Contents: Functions related to daylight saving time
 *
 ***********************************************************************************************************************
 * Copyright (c) 2015 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 * Revision History:
 * 030215  MS    - Initial Release
 *
 **********************************************************************************************************************/

#ifndef TIME_DST_H
#define TIME_DST_H

/* INCLUDE FILES */
#include "project.h"
#include "HEEP_util.h"
#include "time_sys.h"

/* CONSTANT DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
typedef struct
{
   uint8_t month;          //month daylight event
   uint8_t dayOfWeek;      //day of the week daylight event
   uint8_t occuranceOfDay; //occurance of day within month (1st, 2nd etc) daylight event
   uint8_t hour;           //hour daylight event
   uint8_t minute;         //minute daylight event
}DST_Rule_t;

/* GLOBAL VARIABLES */

/* FUNCTION PROTOTYPES */
returnStatus_t DST_Init( void );
bool DST_IsActive ( void );
void DST_PrintDSTParams( void );
void DST_ConvertUTCtoLocal( sysTime_t *pSysTime );
void DST_ConvertLocaltoUTC( sysTime_t *pSysTime );
bool DST_getLocalTime( sysTime_t *pSysTime );

uint8_t getDayOfWeek ( uint8_t month, uint8_t day, uint32_t year ); // RCZ made this function global

bool DST_IsEnable( void );
void DST_ComputeDSTParams( bool timeValid, uint32_t sysDate, uint32_t sysTime );
int32_t DST_GetLocalOffset ( void );
void           DST_getTimeZoneDSTHash( uint32_t *hash );
returnStatus_t DST_setTimeZoneOffset( int32_t tzOffset );
void           DST_getTimeZoneOffset( int32_t *tzOffset );
returnStatus_t DST_setDstEnable( int8_t dstEnable );
void           DST_getDstEnable( int8_t *dstEnable );
returnStatus_t DST_setDstOffset( int16_t dstOffset );
void           DST_getDstOffset( int16_t *dstOffset );
returnStatus_t DST_setDstStartRule(uint8_t month, uint8_t dayOfWeek, uint8_t occOfDay, uint8_t hour, uint8_t minute);
void           DST_getDstStartRule(uint8_t *month, uint8_t *dayOfWeek, uint8_t *occOfDay, uint8_t *hour, uint8_t *minute);
returnStatus_t DST_setDstStopRule(uint8_t month, uint8_t dayOfWeek, uint8_t occOfDay, uint8_t hour, uint8_t minute);
void           DST_getDstStopRule(uint8_t *month, uint8_t *dayOfWeek, uint8_t *occOfDay, uint8_t *hour, uint8_t *minute);
returnStatus_t DST_timeZoneOffsetHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DST_dstEnabledHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DST_dstOffsetHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DST_dstStartRuleHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DST_dstEndRuleHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
returnStatus_t DST_OR_PM_timeZoneDstHashHander( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );

#endif
