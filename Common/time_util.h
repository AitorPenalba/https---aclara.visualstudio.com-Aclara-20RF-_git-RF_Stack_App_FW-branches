/***********************************************************************************************************************
 *
 * Filename: time_util.h
 *
 * Contents: System time utility functions
 *
 ***********************************************************************************************************************
 * Copyright (c) 2010-2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in
 * whole or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * 060110 MS  - Initial Release
 *
 **********************************************************************************************************************/

#ifndef TIME_UTIL_H
#define TIME_UTIL_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "time_sys.h"
#include "buffer.h"
#include "PHY_Protocol.h"
#include "HEEP_util.h"

#if SUPPORT_METER_TIME_FORMAT
#include "ansi_tables.h"
#endif


#ifdef TIME_UTIL_GLOBAL
   #define TIME_UTIL_EXTERN
#else
#define TIME_UTIL_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

#define SYS_EPOCH_YEAR                 ((uint16_t)1970)
#define RF_EPOCH_YEAR                  ((uint16_t)2000)
#define MTR_EPOCH_YEAR                 ((uint16_t)2000)
#define SYS_EPOCH_TO_RF_EPOCH_DAYS     ((uint16_t)0x2ACD) /* 1/1/1970 - 1/1/2000 */
#define SYS_EPOCH_TO_MTR_EPOCH_DAYS    ((uint16_t)0x2ACD) /* 1/1/1970 - 1/1/2000 */
#define DRU_EPOCH_TO_SYS_EPOCH_DAYS    ((uint16_t)0x63DF) /* 1/1/1900 - 1/1/1970 */
#define DAY_AT_DATE_0                  ((uint8_t)4)         /* When date is 0 (1/1/1970), day of the week is Thursday */
#define TIME_NUM_DAYS_IN_WEEK          ((uint8_t)7)         /* 7 Days in a week */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef uint64_t  sysTimeCombined_t;                          /*!< Time format in combined date/time format. */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
uint32_t TIME_UTIL_GetYear( uint32_t daysSinceEpoch );
uint8_t TIME_UTIL_GetDaysInMonth( uint8_t month, uint32_t year );
#if 0 // TODO: RA6E1 - Support OR_PM functionalities - Support only to Systick basic functionalities
returnStatus_t TIME_UTIL_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr );
#endif
int32_t TIME_UTIL_SecondsToDateStr( char* buffer, char* msg, TIMESTAMP_t t );

/* ************************************** */
/* Conversions from one format to another */
/* ************************************** */

/* Convert between system time format and date/time format */
returnStatus_t TIME_UTIL_ConvertDateFormatToSysFormat(sysTime_dateFormat_t const *pDateTime, sysTime_t *pSysTime);
returnStatus_t TIME_UTIL_ConvertSysFormatToDateFormat(sysTime_t const *pSysTime, sysTime_dateFormat_t *pDateTime);

#if ( ACLARA_LC == 1 )
/* Convert between system time format and DRU date/time format */
returnStatus_t TIME_UTIL_ConvertSysFormatToDruFormat(sysTime_t const *pSysTime, sysTime_druFormat_t *pDateTime);
#endif

/* Convert between system time format and system date/time combined format */
sysTimeCombined_t  TIME_UTIL_ConvertSysFormatToSysCombined(sysTime_t const *pTime);
void TIME_UTIL_ConvertSysCombinedToSysFormat(sysTimeCombined_t const *pCombTime, sysTime_t *pTime);

/* Convert between system time format and seconds/fractional seconds */
void TIME_UTIL_ConvertSysFormatToSeconds(sysTime_t const *pTime, uint32_t *seconds, uint32_t *fractionalSec);
void TIME_UTIL_ConvertSecondsToSysFormat(uint32_t seconds, uint32_t fractionalSec, sysTime_t *pTime);

/* Convert between system date/time combined format and seconds/fractional seconds */
void TIME_UTIL_ConvertSysCombinedToSeconds(sysTimeCombined_t const *pCombTime, uint32_t *seconds, uint32_t *fractionalSec);
void TIME_UTIL_ConvertSecondsToSysCombined(uint32_t seconds, uint32_t fractionalSec, sysTimeCombined_t *pCombTime);

#if SUPPORT_METER_TIME_FORMAT
/* Convert between system time format and meter time format (high precision) */
returnStatus_t TIME_UTIL_ConvertMtrHighFormatToSysFormat(MtrTimeFormatHighPrecision_t const *pMtrTime, sysTime_t *pSysTime);
returnStatus_t TIME_UTIL_ConvertSysFormatToMtrHighFormat(sysTime_t const *pSysTimeFmt, MtrTimeFormatHighPrecision_t *pMtrTime);
#endif

uint64_t TIME_UTIL_ConvertNsecToQSecFracFormat(uint64_t nsec);
uint64_t TIME_UTIL_ConvertCyccntToQSecFracFormat(uint32_t cyccnt);
void     TIME_UTIL_PrintQSecFracFormat(char const * str, uint64_t QSecFrac);

/* ************************************ */
/* Get system time in different formats */
/* ************************************ */
returnStatus_t TIME_UTIL_GetTimeInDateFormat(sysTime_dateFormat_t *pDateTime);
returnStatus_t TIME_UTIL_GetTimeInSysCombined(sysTimeCombined_t *pCombTime);
returnStatus_t TIME_UTIL_GetTimeInSecondsFormat(TIMESTAMP_t *pDateTime);
uint64_t       TIME_UTIL_GetTimeInQSecFracFormat(void);

/* ************************************ */
/* Set system time in different formats */
/* ************************************ */
returnStatus_t TIME_UTIL_SetTimeFromDateFormat(sysTime_dateFormat_t const *pDateTime);
returnStatus_t TIME_UTIL_SetTimeFromSeconds(uint32_t seconds, uint32_t fractionalSec);

#undef TIME_UTIL_EXTERN

#endif
