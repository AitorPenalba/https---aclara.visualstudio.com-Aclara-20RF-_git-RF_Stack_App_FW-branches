/***********************************************************************************************************************
 *
 * Filename:    time_DST.c
 *
 * Global Designator: DST_
 *
 * Contents: Functions related to daylight saving time.
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

/* INCLUDE FILES */
#include "project.h"
#include <string.h>
#include <stddef.h>
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#include "file_io.h"
#include "time_sys.h"
#include "time_util.h"
#include "DBG_SerialDebug.h"
#include "time_DST.h"
#include "byteswap.h"
#if 0 // TODO: RA6E1 - WolfSSL integration
#include "wolfssl/wolfcrypt/sha256.h"
#endif

/* CONSTANTS */

/* MACRO DEFINITIONS */
#define DST_FILE_UPDATE_RATE  (TIME_TICKS_PER_MONTH)

#define DST_START           ((uint8_t)0)
#define DST_END             ((uint8_t)1)
#define DST_LAST_OCCURRENCE ((uint8_t)5)

#define DST_UTC_TO_LOCAL    ((bool)true)
#define DST_LOCAL_TO_UTC    ((bool)false)

/* TYPE DEFINITIONS */
typedef struct
{
   uint32_t  timeZoneDSTHash;             //Hash of DST parameters
   int32_t   timeZoneOffset;              //Local time offset
   int8_t    dstEnabled;                  //Is DST enabled?
   int16_t   dstOffset;                   //Amt of time to add to time zone offset during DST (in seconds)
   DST_Rule_t dstEvent[DST_NUM_OF_EVENTS];//DST start and end
}DST_Params_t;

/* FILE VARIABLE DEFINITIONS */
static FileHandle_t  dstFileHndl_;  //Contains the file handle information
static OS_MUTEX_Obj  dstMutex_;     //Serialize access to DST data-structure

static DST_Params_t dstParams_;      //DST parameters
static sysTime_t dstEventDateTime_[DST_NUM_OF_EVENTS]; //DST start and end date/time in UTC
static int32_t timeZoneOffsetTicks_; //Offset to add to UTC to get local time (in milliseconds)
static int32_t dstOffsetTicks_;      //Amt of time to add to time zone offset during DST (in milliseconds)
static bool dstEnabled_;             //true = DST Enabled, false = DST Disabled
static bool dstActive_;              //true = DST Active, false = DST inactive


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
//uint8_t getDayOfWeek ( uint8_t month, uint8_t day, uint32_t year ); RCZ Changed, was static
static  void getDSTDate( uint8_t index, uint32_t year );
static void ComputeDSTParams( bool sysTimeValid, uint32_t sysDate, uint32_t sysTime );
/* FUNCTION DEFINITIONS */


/*****************************************************************************************************************
 *
 * Function name: time_DST_IsActive
 *
 * Purpose: Determine if in Daylight Savings time or not.
 *
 * Arguments: None
 *
 * Returns: return value of dstActive_  (true = DST Active, false = DST inactive)
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
bool DST_IsActive ( void )
{
    return ( dstActive_ );
}

/*****************************************************************************************************************
 *
 * Function name: DST_Init
 *
 * Purpose: Read the DST rules and parameters from NV and setup local time
 *
 * Arguments: None
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Passed validation, eFAILURE otherwise.
 *
 * Side effects: None
 *
 * Reentrant: No
 *
 ******************************************************************************************************************/
returnStatus_t DST_Init( void )
{
   returnStatus_t retVal = eFAILURE;  //Return value
   FileStatus_t fileStatus;           //Indicates if the file is created or already exists

   if (OS_MUTEX_Create(&dstMutex_))
   {
      if ( eSUCCESS == FIO_fopen(&dstFileHndl_,               /* File Handle */
                                 ePART_SEARCH_BY_TIMING,      /* Search for best partition according to the timing */
                                 (uint16_t)eFN_DST,           /* File ID (filename) */
                                 (lCnt)sizeof(DST_Params_t),  /* Size of the data in the file */
                                 FILE_IS_NOT_CHECKSUMED,      /* File attributes to use */
                                 DST_FILE_UPDATE_RATE,        /* File update rate */
                                 &fileStatus) )
      {
         if ( fileStatus.bFileCreated )
         {  //the file was created, set default values
            (void)memset(&dstParams_, 0, sizeof(dstParams_));
            dstParams_.dstEnabled = 1;
            dstParams_.dstOffset = 3600;
            dstParams_.timeZoneOffset = -21600;

            dstParams_.dstEvent[DST_START].month = 3;
            dstParams_.dstEvent[DST_START].dayOfWeek = 1;
            dstParams_.dstEvent[DST_START].occuranceOfDay = 2;
            dstParams_.dstEvent[DST_START].hour = 2;
            dstParams_.dstEvent[DST_START].minute = 0;

            dstParams_.dstEvent[DST_END].month = 11;
            dstParams_.dstEvent[DST_END].dayOfWeek = 1;
            dstParams_.dstEvent[DST_END].occuranceOfDay = 1;
            dstParams_.dstEvent[DST_END].hour = 2;
            dstParams_.dstEvent[DST_END].minute = 0;

            retVal = FIO_fwrite(&dstFileHndl_, 0, (uint8_t const *)&dstParams_, (lCnt)sizeof(DST_Params_t)); //save the defaults
         }
         else
         {
            retVal = FIO_fread(&dstFileHndl_, (uint8_t *)&dstParams_, 0, (lCnt)sizeof(DST_Params_t)); //Read the complete file
         }
      }
   }
   return retVal;
}


/*****************************************************************************************************************
 *
 * Function name: DST_IsEnable
 *
 * Purpose: Returns if DST is enabled
 *
 * Arguments: None
 *
 * Returns: bool: true: DST is enabled, false otherwise
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
bool DST_IsEnable( void )
{
   bool retVal;
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   retVal = dstEnabled_;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails

   return retVal;
}


/*****************************************************************************************************************
 *
 * Function name: getDayOfWeek
 *
 * Purpose: Computes day of the week on a given day, month and year
 *
 * Arguments: uint8_t month - Given month
 *            uint8_t day   - Given day
 *            uint32_t year - Given year
 *
 * Returns: uint8_t - Day of the week, 1=Sunday, 7=Saturday
 *                  RCZ: Above line incorrect, since algorithm changes day of
 *                       week to 0=Sunday.
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
uint8_t getDayOfWeek ( uint8_t month, uint8_t day, uint32_t year ) /* RCZ changed, was static */
{
   uint8_t dayOfWeek;  /* Day of the week , 1=Sunday, 7=Saturday */

   /* Based on Zeller's congruence */
   if ( month < 3 )  /* Year starts from March, so Jan is month 13th and Feb is month 14th */
   {
      month = month + 12;
      year = year - 1;
   }
   dayOfWeek = ( (day
                  + (((month + 1)*26) / 10)
                  + year
                  + (year / 4)
                  + ((year / 100)*6)
                  + (year / 400)
                  + 6 /* Change day of week to 0=Sunday from 0=Saturday */
                  ) % 7 );
   return (dayOfWeek + 1);
}


/*****************************************************************************************************************
 *
 * Function name: DST_PrintDSTParams
 *
 * Purpose: Prints the DST parameters
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: No
 *
 ******************************************************************************************************************/
void DST_PrintDSTParams( void )
{
  sysTime_dateFormat_t dTime;
  sysTime_t sTime;

  DBG_logPrintf('R', "DST Enabled   =%d", dstParams_.dstEnabled );
  DBG_logPrintf('R', "TimezoneOffset=%ld", dstParams_.timeZoneOffset );
  DBG_logPrintf('R', "DST Offset    =%ld", dstParams_.dstOffset );
  DBG_logPrintf('R', "Start Rule: month=%u, dayOfWeek=%u, occOfDay=%u, hour=%u, minute=%u",
                       dstParams_.dstEvent[0].month, dstParams_.dstEvent[0].dayOfWeek, dstParams_.dstEvent[0].occuranceOfDay,
                       dstParams_.dstEvent[0].hour, dstParams_.dstEvent[0].minute);
  DBG_logPrintf('R', "Stop Rule: month=%u, dayOfWeek=%u, occOfDay=%u, hour=%u, minute=%u",
                       dstParams_.dstEvent[1].month, dstParams_.dstEvent[1].dayOfWeek, dstParams_.dstEvent[1].occuranceOfDay,
                       dstParams_.dstEvent[1].hour, dstParams_.dstEvent[1].minute);
  DBG_logPrintf('R', "DST In Effect   =%d", (uint32_t)dstActive_ );

  //Convert DST start and end dates to printable format
  (void)TIME_UTIL_ConvertSysFormatToDateFormat(&dstEventDateTime_[0], &dTime);
  DBG_logPrintf('R', "Start date(in UTC): YY=%u MM=%u DD=%u HH=%u MM=%u SS=%u",
                       dTime.year, dTime.month, dTime.day, dTime.hour, dTime.min, dTime.sec);
  (void)TIME_UTIL_ConvertSysFormatToDateFormat(&dstEventDateTime_[1], &dTime);
  DBG_logPrintf('R', "Stop date(in UTC): YY=%u MM=%u DD=%u HH=%u MM=%u SS=%u",
                       dTime.year, dTime.month, dTime.day, dTime.hour, dTime.min, dTime.sec);
  (void)DST_getLocalTime(&sTime);
  (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sTime, &dTime);

  DBG_logPrintf('R', "Local date/Time: YY=%u MM=%u DD=%u HH=%u MM=%u SS=%u",
                       dTime.year, dTime.month, dTime.day, dTime.hour, dTime.min, dTime.sec);
}

/*****************************************************************************************************************
 *
 * Function name: DST_ConvertTime
 *
 * Purpose: This function converts system time in UTC from/to local
 *
 * Arguments: bool UTCtoLocal - true is UTC to Local, false is Local to UTC
 *            sysTime_t *pSysTime - Pointer to time structure. Local time returned in same structure
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
static void DST_ConvertTime( bool UTCtoLocal, sysTime_t *pSysTime )
{
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   int32_t timeSinceMidnight;

   if ( UTCtoLocal == DST_UTC_TO_LOCAL ) {
      timeSinceMidnight = (int32_t)pSysTime->time + timeZoneOffsetTicks_ + dstOffsetTicks_;
   } else {
      timeSinceMidnight = (int32_t)pSysTime->time - (timeZoneOffsetTicks_ + dstOffsetTicks_);
   }

   if (timeSinceMidnight < 0)
   {
      timeSinceMidnight += (int32_t)TIME_TICKS_PER_DAY;
      pSysTime->date--;
   }
   else if (timeSinceMidnight >= (int32_t)TIME_TICKS_PER_DAY)
   {
      timeSinceMidnight -= (int32_t)TIME_TICKS_PER_DAY;
      pSysTime->date++;
   }
   pSysTime->time = (uint32_t)timeSinceMidnight;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
}

/*****************************************************************************************************************
 *
 * Function name: DST_ConvertUTCtoLocal
 *
 * Purpose: This function converts system time in UTC to local
 *
 * Arguments: sysTime_t *pSysTime - Pointer to time structure. Local time returned in same structure
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void DST_ConvertUTCtoLocal( sysTime_t *pSysTime )
{
   DST_ConvertTime( DST_UTC_TO_LOCAL, pSysTime );
}

/*****************************************************************************************************************
 *
 * Function name: DST_ConvertLocaltoUTC
 *
 * Purpose: This function converts system time in local to UTC
 *
 * Arguments: sysTime_t *pSysTime - Pointer to time structure. UTC time returned in same structure
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void DST_ConvertLocaltoUTC( sysTime_t *pSysTime )
{
   DST_ConvertTime( DST_LOCAL_TO_UTC, pSysTime );
}

/*****************************************************************************************************************
 *
 * Function name: DST_getLocalTime
 *
 * Purpose: This function returns system date and time in structure pointed by pSysTime
 *
 * Arguments: sysTime_t *pSysTime - Pointer to time structure
 *
 * Returns: bool boolSTValid - false: System time is invalid OR Fatal error, true otherwise
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
bool DST_getLocalTime( sysTime_t *pSysTime )
{
   bool retVal = TIME_SYS_GetSysDateTime( pSysTime );

   if (true == retVal)
   {  //system time valid, convert to local time
      DST_ConvertUTCtoLocal(pSysTime);
   }
   return retVal;
}

/*****************************************************************************************************************
 *
 * Function name: DST_ComputeDSTParams
 *
 * Purpose: Compute the DST related parameters
 *
 * Arguments: bool sysTimeValid: true - system time is valid, false otherwise
 *            uint32_t sysDate: System date
 *            uint32_t sysTime: System time
 *
 * Returns: None
 *
 * Side effects: Updates file static variables
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void DST_ComputeDSTParams( bool sysTimeValid, uint32_t sysDate, uint32_t sysTime )
{
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   ComputeDSTParams(sysTimeValid, sysDate, sysTime);
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
}

/*****************************************************************************************************************
 *
 * Function name: ComputeDSTParams
 *
 * Purpose: Compute the DST related parameters
 *
 * Arguments: bool sysTimeValid: true - system time is valid, false otherwise
 *            uint32_t sysDate: System date
 *            uint32_t sysTime: System time
 *
 * Returns: None
 *
 * Side effects: Updates file static variables
 *
 * Reentrant: No
 *
 ******************************************************************************************************************/
static void ComputeDSTParams( bool sysTimeValid, uint32_t sysDate, uint32_t sysTime )
{
   uint32_t year;         //Current Year

   /* Compute offset to add to UTC to get local time */
   timeZoneOffsetTicks_ = dstParams_.timeZoneOffset * (int32_t)TIME_TICKS_PER_SEC;

   /* Set the DST enable flag */
   if ( 0 == dstParams_.dstEnabled )
   {
      dstEnabled_ = false;
   }
   else
   {
      dstEnabled_ = true;
   }
   if ( (!dstEnabled_) || (!sysTimeValid) )
   {  /* Either DST is disabled or system time is invalid. Clear DST start and end. Set DST Active to false */
      dstActive_ = false;
      dstOffsetTicks_ = 0;
      (void)memset(&dstEventDateTime_[DST_START], 0, sizeof(dstEventDateTime_[0]));
      (void)memset(&dstEventDateTime_[DST_END], 0, sizeof(dstEventDateTime_[0]));
   }
   else
   {  /* DST enabled and system time is valid */
      year = TIME_UTIL_GetYear( sysDate ); //Get present year
      getDSTDate( DST_END, year );  //Compute the DST End Date/Time
      getDSTDate( DST_START, year );  //Compute the DST Start Date/Time

      /* In the northern hemisphere, the DST start is before DST end in the same year
         In the southern hemisphere, the DST ends next year, ex: DST start Oct of x year and ends Feb of x+1 year */

      if ( ((sysDate == dstEventDateTime_[DST_END].date) && (sysTime >= dstEventDateTime_[DST_END].time)) ||
           (sysDate > dstEventDateTime_[DST_END].date) )
      {  /* Current date and time is past DST end date and time */

         if ( dstEventDateTime_[DST_END].date > dstEventDateTime_[DST_START].date )
         {  /* In northern hemisphere */
            /* The next event is DST start event, next year. Compute the next DST start date/time */
            getDSTDate( DST_START, year + 1 );
            dstActive_ = false; // DST not in effect
         }
         else
         {  /* In southern hemisphere */
            if ( ((sysDate == dstEventDateTime_[DST_START].date) && (sysTime >= dstEventDateTime_[DST_START].time)) ||
                 (sysDate > dstEventDateTime_[DST_START].date) )
            {  /* Current date and time is past DST start date and time, DST is in effect */
               /* The next event is DST end event, next year. Compute the next DST end date/time */
               getDSTDate( DST_END, year + 1 );
               dstActive_ = true; // DST in effect
            }
            else
            {  /* Current date and time is before DST start event, DST not in effect */
               dstActive_ = false; // DST not in effect
            }
         }
      }
      else
      {  /* Current date and time is before DST end date and time */

         if ( dstEventDateTime_[DST_END].date > dstEventDateTime_[DST_START].date )
         {  /* In northern hemisphere */
            if ( ((sysDate == dstEventDateTime_[DST_START].date) && (sysTime >= dstEventDateTime_[DST_START].time)) ||
                 (sysDate > dstEventDateTime_[DST_START].date) )
            {  /* Current date and time is past DST start date and time, DST is in effect */
               dstActive_ = true; // DST in effect
            }
            else
            {  /* Current date and time is before DST start event, DST not in effect */
               dstActive_ = false; // DST not in effect
            }
         }
         else
         {  /* In southern hemisphere */
            /* Current date and time is before DST end event, DST in effect */
            dstActive_ = true; // DST in effect
         }
      }

      if ( dstActive_ )
      {  /* DST is in effect, next event is DST end */
         TIME_SYS_SetDSTAlarm(dstEventDateTime_[DST_END].date, dstEventDateTime_[DST_END].time);
         dstOffsetTicks_ = dstParams_.dstOffset * (int32_t)TIME_TICKS_PER_SEC;
      }
      else
      {  /* DST not in effect, next event is DST start */
         TIME_SYS_SetDSTAlarm(dstEventDateTime_[DST_START].date, dstEventDateTime_[DST_START].time);
         dstOffsetTicks_ = 0;
      }
   }
}


/*****************************************************************************************************************
 *
 * Function name: getDSTDate
 *
 * Purpose: Compute the DST date and time in UTCz for DST start or DST end event.
 *
 * Arguments: uint8_t index - DST start or DST end event
 *            uint32_t year - DST start and DST end does not have the year field, use this year to compute the
 *                          DST start or DST end calendar date and time
 *
 * Returns: None
 *
 * Side effects: Stores the computed date and time in dstEventDateTime_[index] variable
 *
 * Reentrant: No
 *
 ******************************************************************************************************************/
STATIC void getDSTDate( uint8_t index, uint32_t year )
{
   int32_t utcTimeSinceMidnight;
   uint8_t month;
   uint8_t day;
   uint8_t dayOfWeek;
   uint8_t occurrence;
   uint8_t localDayOfWeek;
   uint8_t localDayOfMonth;
   sysTime_dateFormat_t localDateTime;

   (void)memset(&localDateTime, 0, sizeof(localDateTime));
   month      = dstParams_.dstEvent[index].month;
   dayOfWeek  = dstParams_.dstEvent[index].dayOfWeek;
   occurrence = dstParams_.dstEvent[index].occuranceOfDay;

   if (DST_LAST_OCCURRENCE == occurrence)
   {  //last occurrence of the month, start from the last day of the month
      day = TIME_UTIL_GetDaysInMonth( month, year );
      localDayOfWeek = getDayOfWeek ( month, day, year); //Get day of week on a given month, day and year
      localDayOfMonth = day - ((7 + localDayOfWeek - dayOfWeek) % 7);
   }
   else
   {  //Start from the first of the month
      day = 1;
      localDayOfWeek = getDayOfWeek ( month, day, year); //Get day of week on a given month, day and year
      localDayOfMonth = (uint8_t)(day + ((7 + dayOfWeek - localDayOfWeek) % 7) + ((occurrence - 1) * 7));
   }

   localDateTime.month = month;
   localDateTime.day = localDayOfMonth;
   localDateTime.year = (uint16_t)year;
   localDateTime.hour = dstParams_.dstEvent[index].hour;
   localDateTime.min = dstParams_.dstEvent[index].minute;

   //Compute UTC/GMT date and time for DST Event
   (void)TIME_UTIL_ConvertDateFormatToSysFormat(&localDateTime, &dstEventDateTime_[index]);

   utcTimeSinceMidnight = (int32_t)dstEventDateTime_[index].time - timeZoneOffsetTicks_;
   if ( DST_END == index )
   {  //DST end event
      utcTimeSinceMidnight -= dstParams_.dstOffset * (int32_t)TIME_TICKS_PER_SEC;
   }

   //Check if jumped midnight
   if ( utcTimeSinceMidnight < 0 )
   {  //previous day
      dstEventDateTime_[index].time = (uint32_t)(utcTimeSinceMidnight + (int32_t)TIME_TICKS_PER_DAY);
      dstEventDateTime_[index].date--;
   }
   else if ( utcTimeSinceMidnight >= (int32_t)TIME_TICKS_PER_DAY )
   {  //next day
      dstEventDateTime_[index].time = (uint32_t)utcTimeSinceMidnight - TIME_TICKS_PER_DAY;
      dstEventDateTime_[index].date++;
   }
   else
   {
      dstEventDateTime_[index].time = (uint32_t)utcTimeSinceMidnight;
   }
}

/*****************************************************************************************************************
 *
 * Function name: DST_GetLocalOffset
 *
 * Purpose: Returns the DST offset i.e. UTCz offset + DST offset
 *
 * Arguments: None
 *
 * Returns: int32_t : DST offset
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
int32_t DST_GetLocalOffset ( void )
{
   int32_t offset;

   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   offset = timeZoneOffsetTicks_ + dstOffsetTicks_;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails

   return offset;
}

/* Functions to get and set the DST parameters */
/*****************************************************************************************************************
 *
 * Function name: DST_getTimeZoneDSTHash
 *
 * Purpose: Return the calculated timeZoneDSTHash
 *
 * Arguments: uint32_t *: SHA-256 hash
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void DST_getTimeZoneDSTHash( uint32_t *hash )
{  /*lint --e{578} struct member same as enum OK  */
   /*lint -esym(529,tzOffset,dstEnable,dstOffset,startMonth,startDayOfWeek,startOccOfDay,startHour,startMinute)   */
   /*lint -esym(529,endMonth,endDayOfWeek,endOccOfDay,endHour,endMinute)   */
PACK_BEGIN
   typedef struct PACK_MID
   {
     int32_t tzOffset;
     int8_t dstEnable;
     int16_t dstOffset;

     uint8_t startMonth;
     uint8_t startDayOfWeek;
     uint8_t startOccOfDay;
     uint8_t startHour;
     uint8_t startMinute;

     uint8_t endMonth;
     uint8_t endDayOfWeek;
     uint8_t endOccOfDay;
     uint8_t endHour;
     uint8_t endMinute;

   } calcTimeZoneHash_t;
PACK_END

   calcTimeZoneHash_t calcTimeZoneHash;
   int16_t nInt16;
   int32_t nInt32;

   DST_getTimeZoneOffset(&nInt32);
   Byte_Swap((uint8_t*) &nInt32, sizeof(nInt32));
   calcTimeZoneHash.tzOffset = nInt32;

   DST_getDstEnable(&calcTimeZoneHash.dstEnable);

   DST_getDstOffset(&nInt16);
   Byte_Swap((uint8_t*) &nInt16, sizeof(nInt16));
   calcTimeZoneHash.dstOffset = nInt16;

   DST_getDstStartRule(&calcTimeZoneHash.startMonth,
      &calcTimeZoneHash.startDayOfWeek,
      &calcTimeZoneHash.startOccOfDay,
      &calcTimeZoneHash.startHour,
      &calcTimeZoneHash.startMinute);

   DST_getDstStopRule(&calcTimeZoneHash.endMonth,
      &calcTimeZoneHash.endDayOfWeek,
      &calcTimeZoneHash.endOccOfDay,
      &calcTimeZoneHash.endHour,
      &calcTimeZoneHash.endMinute);
#if 0 // TODO: RA6E1 - WolfSSL integration
   Sha256  sha;                                 // sha256 working area
   uint8_t TempKey[SHA256_DIGEST_SIZE];

   (void)wc_InitSha256(&sha);
   (void)wc_Sha256Update(&sha, (byte*) &calcTimeZoneHash, sizeof(calcTimeZoneHash));
   (void)wc_Sha256Final(&sha, TempKey);

   *hash = (uint32_t) ((((uint32_t) TempKey[0]) << 24) |
                       (((uint32_t) TempKey[1]) << 16) |
                       (((uint32_t) TempKey[2]) << 8) |
                        ((uint32_t) TempKey[3]));
#endif
   /*lint +esym(529,tzOffset,dstEnable,dstOffset,startMonth,startDayOfWeek,startOccOfDay,startHour,startMinute)   */
   /*lint +esym(529,endMonth,endDayOfWeek,endOccOfDay,endHour,endMinute)   */
}


/*****************************************************************************************************************
 *
 * Function name: DST_setTimeZoneOffset
 *
 * Purpose: Update the timeZoneOffset in NV
 *
 * Arguments: int32_t : timezone offset
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.
 *
 * Side effects: Update the NV
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_setTimeZoneOffset( int32_t tzOffset )
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   sysTime_t sysTime;    //System date/time
   bool sysTimeValid;    //true - system time is valid, false otherwise

   sysTimeValid = TIME_SYS_GetSysDateTime(&sysTime);
   dstParams_.timeZoneOffset = tzOffset;
   retVal = FIO_fwrite(&dstFileHndl_, 0, (uint8_t *)&dstParams_, (lCnt)sizeof(DST_Params_t)); //write the file back
   ComputeDSTParams(sysTimeValid, sysTime.date, sysTime.time);
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails

   return retVal;
}


/*****************************************************************************************************************
 *
 * Function name: DST_getTimeZoneOffset
 *
 * Purpose: return the timeZoneOffset
 *
 * Arguments: int32_t *: timezone offset
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void DST_getTimeZoneOffset( int32_t *tzOffset )
{
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   *tzOffset = dstParams_.timeZoneOffset;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
}


/*****************************************************************************************************************
 *
 * Function name: DST_setDstEnable
 *
 * Purpose: Update the dstEnable in NV
 *
 * Arguments: int8_t : DST enable
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.
 *
 * Side effects: Update the NV
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_setDstEnable( int8_t dstEnable )
{
   returnStatus_t retVal;

   if ( dstEnable == 0 || dstEnable == 1)
   {
      OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
      sysTime_t sysTime;    //System date/time
      bool sysTimeValid;    //true - system time is valid, false otherwise

      sysTimeValid = TIME_SYS_GetSysDateTime(&sysTime);
      dstParams_.dstEnabled = dstEnable;
      retVal = FIO_fwrite(&dstFileHndl_, 0, (uint8_t *)&dstParams_, (lCnt)sizeof(DST_Params_t)); //write the file back
      ComputeDSTParams(sysTimeValid, sysTime.date, sysTime.time);
      OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }
   return retVal;
}


/*****************************************************************************************************************
 *
 * Function name: DST_getDstEnable
 *
 * Purpose: return the dstEnable
 *
 * Arguments: int8_t *: DST enable
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void DST_getDstEnable( int8_t *dstEnable )
{
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   *dstEnable = dstParams_.dstEnabled;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
}


/*****************************************************************************************************************
 *
 * Function name: DST_setDstOffset
 *
 * Purpose: Update the dstOffset in NV
 *
 * Arguments: int16_t : DST offset
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.
 *
 * Side effects: Update the NV
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_setDstOffset( int16_t dstOffset )
{  /*lint !e578 argument name same as enum is OK   */
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   sysTime_t sysTime;    //System date/time
   bool sysTimeValid;    //true - system time is valid, false otherwise

   sysTimeValid = TIME_SYS_GetSysDateTime(&sysTime);
   dstParams_.dstOffset = dstOffset;
   retVal = FIO_fwrite(&dstFileHndl_, 0, (uint8_t *)&dstParams_, (lCnt)sizeof(DST_Params_t)); //write the file back
   ComputeDSTParams(sysTimeValid, sysTime.date, sysTime.time);
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails

   return retVal;
}


/*****************************************************************************************************************
 *
 * Function name: DST_getDstOffset
 *
 * Purpose: return the dstOffset
 *
 * Arguments: int16_t *: dst offset
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void DST_getDstOffset( int16_t *dstOffset )
{  /*lint !e578 argument name same as enum is OK   */
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   *dstOffset = dstParams_.dstOffset;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
}


/*****************************************************************************************************************
 *
 * Function name: DST_setDstStartRule
 *
 * Purpose: Update the dstStartRule in NV
 *
 * Arguments: uint8_t month
 *            uint8_t dayOfWeek
 *            uint8_t occOfDay
 *            uint8_t hour
 *            uint8_t minute
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.
 *
 * Side effects: Update the NV
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_setDstStartRule(uint8_t month, uint8_t dayOfWeek, uint8_t occOfDay, uint8_t hour, uint8_t minute)
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   sysTime_t sysTime;    //System date/time
   bool sysTimeValid;    //true - system time is valid, false otherwise

   sysTimeValid = TIME_SYS_GetSysDateTime(&sysTime);
   dstParams_.dstEvent[DST_START].month = month;
   dstParams_.dstEvent[DST_START].dayOfWeek = dayOfWeek;
   dstParams_.dstEvent[DST_START].occuranceOfDay = occOfDay;
   dstParams_.dstEvent[DST_START].hour = hour;
   dstParams_.dstEvent[DST_START].minute = minute;
   retVal = FIO_fwrite(&dstFileHndl_, 0, (uint8_t *)&dstParams_, (lCnt)sizeof(DST_Params_t)); //write the file back
   ComputeDSTParams(sysTimeValid, sysTime.date, sysTime.time);
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails

   return retVal;
}

/*****************************************************************************************************************
 *
 * Function name: DST_getDstStartRule
 *
 * Purpose: return the dstStartRule
 *
 * Arguments: uint8_t *month
 *            uint8_t *dayOfWeek
 *            uint8_t *occOfDay
 *            uint8_t *hour
 *            uint8_t *minute
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void DST_getDstStartRule(uint8_t *month, uint8_t *dayOfWeek, uint8_t *occOfDay, uint8_t *hour, uint8_t *minute)
{
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   *month = dstParams_.dstEvent[DST_START].month;
   *dayOfWeek = dstParams_.dstEvent[DST_START].dayOfWeek;
   *occOfDay = dstParams_.dstEvent[DST_START].occuranceOfDay;
   *hour = dstParams_.dstEvent[DST_START].hour;
   *minute = dstParams_.dstEvent[DST_START].minute;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
}


/*****************************************************************************************************************
 *
 * Function name: DST_setDstStopRule
 *
 * Purpose: Update the dstStopRule in NV
 *
 * Arguments: uint8_t month
 *            uint8_t dayOfWeek
 *            uint8_t occOfDay
 *            uint8_t hour
 *            uint8_t minute
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Successful, eFAILURE otherwise.
 *
 * Side effects: Update the NV
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_setDstStopRule(uint8_t month, uint8_t dayOfWeek, uint8_t occOfDay, uint8_t hour, uint8_t minute)
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   sysTime_t sysTime;    //System date/time
   bool sysTimeValid;    //true - system time is valid, false otherwise

   sysTimeValid = TIME_SYS_GetSysDateTime(&sysTime);
   dstParams_.dstEvent[DST_END].month = month;
   dstParams_.dstEvent[DST_END].dayOfWeek = dayOfWeek;
   dstParams_.dstEvent[DST_END].occuranceOfDay = occOfDay;
   dstParams_.dstEvent[DST_END].hour = hour;
   dstParams_.dstEvent[DST_END].minute = minute;
   retVal = FIO_fwrite(&dstFileHndl_, 0, (uint8_t *)&dstParams_, (lCnt)sizeof(DST_Params_t)); //write the file back
   ComputeDSTParams(sysTimeValid, sysTime.date, sysTime.time);
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails

   return retVal;
}

/*****************************************************************************************************************
 *
 * Function name: DST_getDstStopRule
 *
 * Purpose: return the dstStopRule
 *
 * Arguments: uint8_t *month
 *            uint8_t *dayOfWeek
 *            uint8_t *occOfDay
 *            uint8_t *hour
 *            uint8_t *minute
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void DST_getDstStopRule(uint8_t *month, uint8_t *dayOfWeek, uint8_t *occOfDay, uint8_t *hour, uint8_t *minute)
{
   OS_MUTEX_Lock(&dstMutex_); // Function will not return if it fails
   *month = dstParams_.dstEvent[DST_END].month;
   *dayOfWeek = dstParams_.dstEvent[DST_END].dayOfWeek;
   *occOfDay = dstParams_.dstEvent[DST_END].occuranceOfDay;
   *hour = dstParams_.dstEvent[DST_END].hour;
   *minute = dstParams_.dstEvent[DST_END].minute;
   OS_MUTEX_Unlock( &dstMutex_ ); // Function will not return if it fails
}


/******************************************************************************************************************
 *
 *   Function Name: DST_timezoneOffsetHandler
 *
 *   Purpose: Get/Set timezoneOffset
 *
 *   Arguments:  action-> set or get
 *               id    -> HEEP enum associated with the value
 *               value -> pointer to new value to be placed in file
 *               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 *
 *   Side Effects: None
 *
 *   Reentrant Code: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_timeZoneOffsetHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure
   int32_t offsetValue;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(dstParams_.timeZoneOffset) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DST_getTimeZoneOffset(&offsetValue);
         retVal = eSUCCESS;
         *(int32_t *)value = offsetValue;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(dstParams_.timeZoneOffset);
            attr->rValTypecast = (uint8_t)intValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = DST_setTimeZoneOffset( (*(int32_t *)value) );
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched



/******************************************************************************************************************
 *
 *   Function Name: DST_dstOffsetHandler
 *
 *   Purpose: Get/Set dstOffset
 *
 *   Arguments:  action-> set or get
 *               id    -> HEEP enum associated with the value
 *               value -> pointer to new value to be placed in file
 *               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 *
 *   Side Effects: None
 *
 *   Reentrant Code: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_dstOffsetHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure
   int16_t offsetValue;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(dstParams_.dstOffset) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DST_getDstOffset(&offsetValue);
         retVal = eSUCCESS;
         *(int16_t *)value = offsetValue;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(dstParams_.dstOffset);
            attr->rValTypecast = (uint8_t)intValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = DST_setDstOffset( (*(int16_t *)value) );
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched



/******************************************************************************************************************
 *
 *   Function Name: DST_dstEnabledHandler
 *
 *   Purpose: Get/Set dstEnabled
 *
 *   Arguments:  action-> set or get
 *               id    -> HEEP enum associated with the value
 *               value -> pointer to new value to be placed in file
 *               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 *
 *   Side Effects: None
 *
 *   Reentrant Code: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_dstEnabledHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure
   int8_t enableValue;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(dstParams_.dstEnabled) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DST_getDstEnable(&enableValue);
         retVal = eSUCCESS;
         *(int8_t *)value = enableValue;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(dstParams_.dstEnabled);
            attr->rValTypecast = (uint8_t)Boolean;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = DST_setDstEnable( (*(int8_t *)value) );
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched



/******************************************************************************************************************
 *
 *   Function Name: DST_dstStartRuleHandler
 *
 *   Purpose: Get/Set dstStartRule
 *
 *   Arguments:  action-> set or get
 *               id    -> HEEP enum associated with the value
 *               value -> pointer to new value to be placed in file
 *               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 *
 *   Side Effects: None
 *
 *   Reentrant Code: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_dstStartRuleHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure
   uint8_t month;
   uint8_t dayOfWeek;
   uint8_t occOfDay;
   uint8_t hour;
   uint8_t minute;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(dstParams_.dstEvent[0]) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DST_getDstStartRule(&month, &dayOfWeek, &occOfDay, &hour, &minute);
         retVal = eSUCCESS;
         *(uint64_t *)value = ( ( ( uint64_t ) month ) << 32 ) |
                              ( ( ( uint64_t ) dayOfWeek ) << 24 ) |
                              ( ( ( uint64_t ) occOfDay ) << 16 ) |
                              ( ( ( uint64_t ) hour ) << 8 ) |
                              ( ( ( uint64_t ) minute << 0 ));

         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(dstParams_.dstEvent[0]);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = DST_setDstStartRule(
                   ( uint8_t )  *( (uint8_t *)value + 4 ),  // Month
                   ( uint8_t )  *( (uint8_t *)value + 3 ),  // Day of Week
                   ( uint8_t )  *( (uint8_t *)value + 2 ),  // Day Occurence
                   ( uint8_t )  *( (uint8_t *)value + 1 ),  // Hour
                   ( uint8_t )  *( (uint8_t *)value )); // Minute
   }
   return retVal;

} //lint !e715 symbol id not referenced. This funtion is only called when id is matched



/******************************************************************************************************************
 *
 *   Function Name: DST_dstEndHandler
 *
 *   Purpose: Get/Set dstStopRule
 *
 *   Arguments:  action-> set or get
 *               id    -> HEEP enum associated with the value
 *               value -> pointer to new value to be placed in file
 *               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 *
 *   Side Effects: None
 *
 *   Reentrant Code: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_dstEndRuleHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure
   uint8_t month;
   uint8_t dayOfWeek;
   uint8_t occOfDay;
   uint8_t hour;
   uint8_t minute;

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(dstParams_.dstEvent[0]) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DST_getDstStopRule(&month, &dayOfWeek, &occOfDay, &hour, &minute);
         retVal = eSUCCESS;
         *(uint64_t *)value = ( ( ( uint64_t ) month ) << 32 ) |
                              ( ( ( uint64_t ) dayOfWeek ) << 24 ) |
                              ( ( ( uint64_t ) occOfDay ) << 16 ) |
                              ( ( ( uint64_t ) hour ) << 8 ) |
                              ( ( ( uint64_t ) minute << 0 ));

         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(dstParams_.dstEvent[0]);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = DST_setDstStopRule(
                   ( uint8_t )  *( (uint8_t *)value + 4 ),  // Month
                   ( uint8_t )  *( (uint8_t *)value + 3 ),  // Day of Week
                   ( uint8_t )  *( (uint8_t *)value + 2 ),  // Day Occurence
                   ( uint8_t )  *( (uint8_t *)value + 1 ),  // Hour
                   ( uint8_t )  *( (uint8_t *)value )); // Minute
   }
   return retVal;

} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/******************************************************************************************************************
 *
 *   Function Name: DST_OR_PM_timeZoneDstHashHander
 *
 *   Purpose: Get the timeZoneDstHash parameter
 *
 *   Arguments:  action-> set or get
 *               id    -> HEEP enum associated with the value
 *               value -> pointer to new value to be placed in file
 *               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 *
 *   Side Effects: None
 *
 *   Reentrant Code: Yes
 *
 ******************************************************************************************************************/
returnStatus_t DST_OR_PM_timeZoneDstHashHander( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(dstParams_.timeZoneDSTHash) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DST_getTimeZoneDSTHash((uint32_t *)value);
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(dstParams_.timeZoneDSTHash);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }

   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched
