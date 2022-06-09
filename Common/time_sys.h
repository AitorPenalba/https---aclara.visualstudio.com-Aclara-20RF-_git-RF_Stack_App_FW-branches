/***********************************************************************************************************************
 *
 * Filename: time_sys.h
 *
 * Contents: Functions related to subscribable Alarm (calendar and periodic alarms)
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2021 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.

 ***********************************************************************************************************************
 * Revision History:
 * 100810  MS    - Initial Release
 *
 **********************************************************************************************************************/

#ifndef TIME_SYS_H
#define TIME_SYS_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include "HEEP_util.h"
#if 0 // TODO: RA6E1 - Support to DST and mac modules - Porting for Systick timer
#if ( EP == 1 )
#include "buffer.h"
#include "MAC_Protocol.h"
#endif
#endif

#ifdef TIME_SYS_GLOBAL
   #define GLOBAL
#else
   #define GLOBAL extern
#endif

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

#define DAYS_PER_NON_LEAP_YEAR   ((uint32_t)365)
#define MONTHS_PER_YEAR          ((uint32_t)12)
#define HOURS_PER_DAY            ((uint32_t)24)
#define MINUTES_PER_HOUR         ((uint32_t)60)
#define SECONDS_PER_MINUTE       ((uint32_t)60)
#define SECONDS_PER_5MINUTE      (SECONDS_PER_MINUTE * 5)
#define SECONDS_PER_HOUR         (MINUTES_PER_HOUR * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY          (HOURS_PER_DAY * SECONDS_PER_HOUR)

#define TIME_SYS_MAX_SEC         ((uint32_t)0xFFFFFFFF)
#define TIME_SYS_MAX_SEC_DAY     ( TIME_SYS_MAX_SEC / SECONDS_PER_DAY) /* Day value when max time in seconds used */
#define TIME_SYS_MAX_SEC_TIME    ((TIME_SYS_MAX_SEC % SECONDS_PER_DAY) * TIME_TICKS_PER_SEC) /* Time value when max time in seconds used */

#define DST_NUM_OF_EVENTS        ((uint8_t)2)

#define TIME_TICKS_PER_SEC       ((uint32_t)1000)                       /* Number of system ticks in 1 sec */
#define TIME_TICKS_PER_MIN       ((uint32_t)60 * TIME_TICKS_PER_SEC)    /* Number of system ticks in 1 min */
#define TIME_TICKS_PER_5MIN      ((uint32_t)5 * TIME_TICKS_PER_MIN)     /* Number of system ticks in 5 min */
#define TIME_TICKS_PER_10MIN     ((uint32_t)10 * TIME_TICKS_PER_MIN)    /* Number of system ticks in 10 min */
#define TIME_TICKS_PER_15MIN     ((uint32_t)15 * TIME_TICKS_PER_MIN)    /* Number of system ticks in 15 min */
#define TIME_TICKS_PER_20MIN     ((uint32_t)20 * TIME_TICKS_PER_MIN)    /* Number of system ticks in 20 min */
#define TIME_TICKS_PER_30MIN     ((uint32_t)30 * TIME_TICKS_PER_MIN)    /* Number of system ticks in 30 min */
#define TIME_TICKS_PER_HR        ((uint32_t)60 * TIME_TICKS_PER_MIN)    /* Number of system ticks in 1 hr  */
#define TIME_TICKS_PER_DAY       ((uint32_t)24 * TIME_TICKS_PER_HR)     /* Number of system ticks in 1 day */
#define TIME_TICKS_PER_2DAY      ((uint32_t)2 * TIME_TICKS_PER_DAY)     /* Number of system ticks in 2 day */
#define TIME_TICKS_PER_WEEK      ((uint32_t)7 * TIME_TICKS_PER_DAY)     /* Number of system ticks in 7 days (week) */
#define TIME_TICKS_PER_MONTH     ((uint32_t)30 * TIME_TICKS_PER_DAY)    /* Number of system ticks in 30 days */

#define INVALID_SYS_DATE         ((uint32_t)0)
#define INVALID_SYS_TIME         ((uint32_t)0)
#define INVALID_SYS_TICTOC       ((uint32_t)0)

#define TIME_SYS_DAILY_ALARM     ((uint32_t)0) /* Daily alarm is defined as 0 */
#define TIME_SYS_ALARM           true  /* alarm indicator, Note: There could still be a time jump! */
#define TIME_SYS_CHANGE          false /* Time changed, but alarm has not triggered. */

#define NO_ALARM_FOUND           ((uint8_t)0xFF)

#define TIME_SYS_MAX_TIME_ACC_DELAY 3600
#define TIME_SIG_OFFSET_MAX      ((uint16_t)60000)
#define TIME_SIG_OFFSET_MIN      ((uint16_t)1)

/* TODO: these should only be defined in one place!   */
#ifndef TIME_SYNC_SOURCE
#define TIME_SYNC_SOURCE         ((uint8_t)0) //Time Sync source only
#endif
#ifndef TIME_SYNC_CONSUMER
#define TIME_SYNC_CONSUMER       ((uint8_t)1) //Time Sync consumer only
#endif
#ifndef TIME_SYNC_BOTH
#define TIME_SYNC_BOTH           ((uint8_t)2) //Time Sync source and consumer
#endif

// Sources of frequency computation
typedef enum
{
   eTIME_SYS_SOURCE_NONE,
   eTIME_SYS_SOURCE_TCXO,
   eTIME_SYS_SOURCE_CPU,
   eTIME_SYS_SOURCE_GPS,
   eTIME_SYS_SOURCE_AFC
} TIME_SYS_SOURCE_e;

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
typedef struct
{
   uint32_t date;         /* Number of days since epoc */
   uint32_t time;         /* Number of MS since midnight */
   uint32_t tictoc;       /* How many RTOS ticks happened in system ticks */
   uint32_t elapsedCycle; /* How many CPU cycles elapsed since the last RTOS tick */
}sysTime_t;

typedef struct
{
   OS_QUEUE_Handle pQueueHandle;   /* Queue handle, if not NULL, send message */
   OS_MSGQ_Obj     *pMQueueHandle; /* Message Queue handle, if not NULL, send message */
   uint32_t ulAlarmDate;           /* Alarm Date, if this value = 0, it is a daily alarm */
   uint32_t ulAlarmTime;           /* Alarm Time */
   uint8_t ucAlarmId;              /* Alarm Id */
   unsigned bSkipTimeChange: 1;    /* true = Do not call on Time Change, false = Call on Time change */
   unsigned bUseLocalTime:   1;    /* true = Use local time, false = Use UTC time */
}tTimeSysCalAlarm;

typedef struct
{
   int64_t timeChangeDelta;        /* number of system ticks of the time change */
   uint32_t time;                  /* Time of the day or time of the day since given event */
   uint32_t date;                  /* Number of days since given event or date/time */
   uint8_t alarmId;                /* Alarm Id */
   bool bSystemTime:         1;    /* true - sTime is system time, false - sTime is power-up time */
   bool bTimeChangeForward:  1;    /* true - Time moved forward */
   bool bTimeChangeBackward: 1;    /* true - Time moved backward */
   bool bIsSystemTimeValid:  1;    /* true - System is valid, false - System time is  invalid */
   bool bCauseOfAlarm:       1;    /* Cause of trigger. false = time change only
                                         true = time change and Time for alarm OR Time for alarm only */
}tTimeSysMsg;

typedef struct
{  //Only one of the handles below should be valid.
   OS_QUEUE_Handle pQueueHandle;  /* Queue handle, if not NULL, send message */
   OS_MSGQ_Handle  pMQueueHandle; /* Message Queue handle, if not NULL, send message */
   uint32_t ulPeriod;             /* Period */
   uint32_t ulOffset;             /* Offset or local shift time in system ticks */
   uint8_t ucAlarmId;             /* Alarm Id */
   unsigned bSkipTimeChange: 1;   /* true = Do not call on Time Change, false = Call on Time change */
   unsigned bUseLocalTime: 1;     /* true = Use local time, false = Use UTC time */
   unsigned bOnValidTime:    1;   /* true = call on Valid Time, false otherwise */
   unsigned bOnInvalidTime:  1;   /* true = call on Invalid Time, false otherwise */
}tTimeSysPerAlarm;

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
returnStatus_t TIME_SYS_Init(void);
#ifdef __mqx_h__
void TIME_SYS_HandlerTask(uint32_t Arg0);
#endif /* __mqx_h__ */
// TODO: RA6E1 - Single declaration to support both
#if (RTOS_SELECTION == FREE_RTOS)
void TIME_SYS_HandlerTask( taskParameter );
#endif

#if ( EP == 1 )
void           TIME_SYS_SetDSTAlarm(uint32_t date, uint32_t time);
#if 1 // TODO: RA6E1 Bob: this should now be able to operate.  Remove #if on check-in
returnStatus_t TIME_SYS_SetDateTimeFromMAC(MAC_DataInd_t const *pDataInd);
#endif
uint32_t       TIME_SYS_GetInstallationDateTime(void);
void           TIME_SYS_SetInstallationDateTime(uint32_t dateTime);
uint16_t       TIME_SYS_GetTimeAcceptanceDelay( void );
returnStatus_t TIME_SYS_SetTimeAcceptanceDelay(uint16_t timeAcceptanceDelay);
void           TIME_SYS_reqTimeoutCallback( uint8_t cmd, void *pdata );
void           TIME_SYS_acceptanceTimeoutCallback( uint8_t cmd, void *pdata );
uint16_t       TIME_SYS_getTimeSigOffset(void);
returnStatus_t TIME_SYS_setTimeSigOffset(uint16_t offset);
returnStatus_t TIME_SYS_SetTimeRequestMaxTimeout ( uint16_t timeoutValue );
uint16_t       TIME_SYS_GetTimeRequestMaxTimeout ( void );
void           TIME_SYS_SetDateTimeLostCount( uint16_t dateTimeLostValue );
uint16_t       TIME_SYS_GetDateTimeLostCount( void );
#endif
#if ( DCU == 1) //DCU2+ functions
bool           TIME_SYS_IsGpsPresent( void );
bool           TIME_SYS_IsGpsTimeValid( void );
bool           TIME_SYS_IsGpsPhaseLocked( void );
int32_t        TIME_SYS_PhaseErrorUsec( void );
int32_t        TIME_SYS_PhaseErrorNsec( void );
#endif
char*          TIME_SYS_GetSourceMsg(TIME_SYS_SOURCE_e source);
bool           TIME_SYS_GetRealCpuFreq( uint32_t *freq, TIME_SYS_SOURCE_e *source, uint32_t *seconds );
void           TIME_SYS_SetRealCpuFreq( uint32_t freq, TIME_SYS_SOURCE_e source, bool reset );
uint32_t       TIME_SYS_GetTimeLastUpdated( void );
returnStatus_t TIME_SYS_SetTimeFromRTC(void);
void           TIME_SYS_SetSysDateTime(const sysTime_t *pSysTime );
bool           TIME_SYS_GetSysDateTime(sysTime_t *pSysTime);
void           TIME_SYS_GetPupDateTime(sysTime_t *pPupTime);
bool           TIME_SYS_IsTimeValid(void);
returnStatus_t TIME_SYS_AddCalAlarm(tTimeSysCalAlarm *pData);
returnStatus_t TIME_SYS_GetCalAlarm(tTimeSysCalAlarm *pData, uint8_t alarmId);
returnStatus_t TIME_SYS_GetPeriodicAlarm( tTimeSysPerAlarm *pData, uint8_t alarmId );
returnStatus_t TIME_SYS_AddPerAlarm(tTimeSysPerAlarm *pData);
returnStatus_t TIME_SYS_DeleteAlarm(uint8_t alarmId);

returnStatus_t TIME_SYS_OR_PM_Handler(enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr);

uint8_t TIME_SYS_TimeState(void);
void    TIME_SYS_SetTimeState(uint8_t state);

#undef GLOBAL

#endif
