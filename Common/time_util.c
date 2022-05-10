/***********************************************************************************************************************
 *
 * Filename:	time_util.c
 *
 * Global Designator: TIME_UTIL_
 *
 * Contents: System time utility functions
 *
 *
 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2010 - 2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.

 ***********************************************************************************************************************
 * Revision History:
 * 052710 MS  - Initial Release
 *
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdio.h>
#if 0 // TODO: RA6E1 - Support PHY
#include "PHY_Protocol.h"
#endif
#define TIME_UTIL_GLOBAL
#include "time_util.h"
#undef  TIME_UTIL_GLOBAL
#if ( EP == 1 )
#include "time_DST.h"
#endif

/* CONSTANTS */
/* The first element of each row is 0 so that January is at offset 1 */
STATIC const uint8_t _TIME_UTIL_MonthLengths[2][MONTHS_PER_YEAR + 1] =
{
   { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
   { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

/* MACRO DEFINITIONS */

/*lint -esym(750,LEAP_YEAR_FREQ_YRS)   not referenced */
#define LEAP_YEAR_FREQ_YRS    ((uint8_t)4)

/* Number of bits required for TIME_TICKS_PER_SEC value (i.e. 1000d(0x3E8) is 10 bits)*/
#define BITS_IN_TIME_TICKS_PER_SEC   ((uint8_t)10)

/* TYPE DEFINITIONS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */
STATIC uint8_t isLeapYear ( uint32_t year );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_GetYear
 *
 * Purpose: Compute the year from number of days since epoch
 *
 * Arguments: uint32_t: Number of days since epoch
 *
 * Returns: uint32_t: Years since epoch
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
uint32_t TIME_UTIL_GetYear( uint32_t daysSinceEpoch )
{
   uint32_t year;
   uint8_t leapYear;

   if ( daysSinceEpoch > SYS_EPOCH_TO_RF_EPOCH_DAYS )
   {  /* Start at RF epoch */
      year = RF_EPOCH_YEAR;
      daysSinceEpoch -= SYS_EPOCH_TO_RF_EPOCH_DAYS;
   }
   else
   {  /* Time before RF epoch, start at SYS epoch */
      year = SYS_EPOCH_YEAR;
   }
   leapYear = isLeapYear(year);
   while ( daysSinceEpoch >= (DAYS_PER_NON_LEAP_YEAR + leapYear) )
   {
      daysSinceEpoch -= (DAYS_PER_NON_LEAP_YEAR + leapYear);
      year++;
      leapYear = isLeapYear(year);
   }
   return year;
}

/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_GetDaysInMonth
 *
 * Purpose: Return the number of days in the given month. If the month is zero, get the number of days in
 *          december of last year
 *
 * Arguments: uint8_t month: Month of the year, 1 = Jan and so on
 *            uint32_t year: Year
 *
 * Returns: uint8_t: Days in the month
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
uint8_t TIME_UTIL_GetDaysInMonth( uint8_t month, uint32_t year )
{
   uint8_t leapYear;

   if ( 0 == month )
   {
      month = MONTHS_PER_YEAR;
      year--;
   }
   leapYear = isLeapYear(year);

   return ((uint8_t)_TIME_UTIL_MonthLengths[leapYear][month]);
}

/* ************************************** */
/* Conversions from one format to another */
/* ************************************** */

/*****************************************************************************************************************
 *
 * Function name: isLeapYear
 *
 * Purpose: Checks if the year is a leap year
 *
 * Arguments: uint32_t year - Year to be checked
 *
 * Returns: uint8_t - 1: It is a leap year. 0: Not a leap year
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
STATIC uint8_t isLeapYear ( uint32_t year )
{
   uint8_t u8LYear = 0;   /* Default, not a leap year */

   if ( 0 == ( year % 4 ) )
   {  /* Divisible by 4 */
      if ( 0 == ( year % 400 ) )
      {  /* divisible by 4, 100 and 400 */
         u8LYear = 1;
      }
      else if ( 0 != ( year % 100 ) )
      {  /* divisible 4 but not divisible by 100  */
         u8LYear = 1;
      }
      //else divisible by 4 and 100. Not a leap year
   }
   return u8LYear;
}

/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertDateFormatToSysFormat
 *
 * Purpose: Converts Date format to System time format.
 *
 * Arguments: sysTime_dateFormat_t const *pDateTime - Source Time
 *            sysTime_t *pSysTime - Destination Time
 *
 * Returns: eSUCCESS
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_ConvertDateFormatToSysFormat(sysTime_dateFormat_t const *pDateTime, sysTime_t *pSysTime)
{
   uint32_t  cnt;                 /* Loop counter */
   uint32_t  startYear;           /* Starting year */
   uint8_t   leapYear;            /* Flag to indicate, if the given year is leap or not */

   (void)memset( (uint8_t *)pSysTime, 0, sizeof(sysTime_t) );

   if ( pDateTime->year >= RF_EPOCH_YEAR )
   {  /* Start at RF epoch */
      startYear      = RF_EPOCH_YEAR;
      pSysTime->date = SYS_EPOCH_TO_RF_EPOCH_DAYS;
   }
   else
   {  /* Start at SYS epoch */
      startYear      = SYS_EPOCH_YEAR;
      pSysTime->date = 0;
   }
   for ( cnt = startYear; cnt < pDateTime->year; cnt++ )
   {
      leapYear        = isLeapYear( cnt );
      pSysTime->date += (DAYS_PER_NON_LEAP_YEAR + leapYear);
   }
   /* Number of days in complete months */
   leapYear = isLeapYear( pDateTime->year ); // check the current year for leap year
   for ( cnt = 1; cnt < pDateTime->month; cnt++ )
   {
      pSysTime->date += (uint32_t)_TIME_UTIL_MonthLengths[leapYear][cnt];
   }

   /* Add number of days in the current month excluding today */
   pSysTime->date += ( uint8_t )(pDateTime->day - 1); /* TODO: What if ->day is zero? */

   /* Get number of sub-seconds (ticks) since midnight */
   pSysTime->time  = pDateTime->hour * TIME_TICKS_PER_HR;
   /*lint -e{123} min element same name as macro elsewhere is OK */
   pSysTime->time += pDateTime->min * TIME_TICKS_PER_MIN;
   pSysTime->time += pDateTime->sec * TIME_TICKS_PER_SEC;
   // Add msec with 10 msec resolution only
   // The value added must be 0, 10, 20, ...
   pSysTime->time += ( uint32_t )(pDateTime->msec - (pDateTime->msec % 10) );

   return eSUCCESS;
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSysFormatToDateFormat
 *
 * Purpose: Converts System time format to Date format.
 *
 * Arguments: sysTime_t const *pSysTime - Destination Time
 *            sysTime_dateFormat_t *pDateTime - Source Time
 *
 * Returns: eSUCCESS
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_ConvertSysFormatToDateFormat(sysTime_t const *pSysTime, sysTime_dateFormat_t *pDateTime)
{
   uint8_t leapYear;
   uint8_t month;
   uint16_t year;
   uint32_t daysSinceEpoch = pSysTime->date;

   //Get year
   if ( daysSinceEpoch >= SYS_EPOCH_TO_RF_EPOCH_DAYS )
   {  /* Start at RF epoch */
      year = RF_EPOCH_YEAR;
      daysSinceEpoch -= SYS_EPOCH_TO_RF_EPOCH_DAYS;
   }
   else
   {  /* Time before RF epoch, start at SYS epoch */
      year = SYS_EPOCH_YEAR;
   }
   leapYear = isLeapYear(year);
   while ( daysSinceEpoch >= (DAYS_PER_NON_LEAP_YEAR + leapYear) )
   {
      daysSinceEpoch -= (DAYS_PER_NON_LEAP_YEAR + leapYear);
      year++;
      leapYear = isLeapYear(year);
   }
   pDateTime->year = year;

   //Get month
   leapYear = isLeapYear(year);
   month = 1;
   while ( daysSinceEpoch >= _TIME_UTIL_MonthLengths[leapYear][month] )
   {
      daysSinceEpoch -= _TIME_UTIL_MonthLengths[leapYear][month];
      month++;
   }
   pDateTime->month = month;

   pDateTime->day = (uint8_t)(daysSinceEpoch + 1); //Add 1 to account for the current day

   /* Convert hours, minutes and seconds */
   pDateTime->hour = (uint8_t)(pSysTime->time / TIME_TICKS_PER_HR);
   /*lint -e{123} min element same name as macro elsewhere is OK */
   pDateTime->min = (uint8_t)((pSysTime->time % TIME_TICKS_PER_HR)/TIME_TICKS_PER_MIN);
   pDateTime->sec = (uint8_t)((pSysTime->time % TIME_TICKS_PER_MIN)/TIME_TICKS_PER_SEC);
   pDateTime->msec =(uint16_t)(pSysTime->time % TIME_TICKS_PER_SEC);

   return eSUCCESS;
}

#if ( ACLARA_LC == 1 )
/*****************************************************************************************************************
  Function name: TIME_UTIL_ConvertSysFormatToDruFormat

  Purpose: Converts System time format to DRU format.

  Arguments: sysTime_t const *pSysTime      - Source Time in Sys format
             sysTime_druFormat_t *pDateTime - Destination Time in DRU format

  Returns: eSUCCESS

 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_ConvertSysFormatToDruFormat(sysTime_t const *pSysTime, sysTime_druFormat_t *pDateTime)
{
   /* Convert number of days since SRFN epoc (1/1/1970) to number of days since DRU epoc (1/1/1900) */
   pDateTime->days_u16 = (uint16_t) ( ( pSysTime->date & 0x0000FFFFU )  + DRU_EPOCH_TO_SYS_EPOCH_DAYS );

   /* Convert from miliseconds since midnight to counts since midnight, where each count represents 2.5 seconds */
   pDateTime->counts_u16 = (uint16_t) ( pSysTime->time / 2500 );

   return eSUCCESS;
}
#endif

/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSysFormatToSysCombined
 *
 * Purpose: Converts the time passed (system time format) in to a combined date and time.
 *          This is useful for comparing two time values.
 *
 * Arguments: sysTime_t const *pTime
 *
 * Returns: sysTimeCombined_t - Combined date/time
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
sysTimeCombined_t TIME_UTIL_ConvertSysFormatToSysCombined(sysTime_t const *pTime)
{
   return((pTime->date * (sysTimeCombined_t)TIME_TICKS_PER_DAY) + pTime->time);
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSysCombinedToSysFormat
 *
 * Purpose: Converts the combined date and time passed in to system time format.
 *
 * Arguments: sysTimeCombined_t const *pCombTime
 *            sysTime_t *pTime
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_UTIL_ConvertSysCombinedToSysFormat(sysTimeCombined_t const *pCombTime, sysTime_t *pTime)
{
   pTime->date = (uint32_t)(*pCombTime / (uint64_t)TIME_TICKS_PER_DAY);
   pTime->time = (uint32_t)(*pCombTime - (pTime->date * (uint64_t)TIME_TICKS_PER_DAY));
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSysFormatToSeconds
 *
 * Purpose: Converts the time passed (system time format) in to a seconds and fractional seconds.
 *
 * Arguments: sysTime_t const *pTime: Sys time to be converted
 *            uint32_t *seconds: Seconds since epoch
 *            uint32_t *fractionalSec: Fractional second
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_UTIL_ConvertSysFormatToSeconds(sysTime_t const *pTime, uint32_t *seconds, uint32_t *fractionalSec)
{
   uint32_t msInFracSeconds;

   *seconds  = pTime->date * SECONDS_PER_DAY;
   *seconds += (pTime->time / TIME_TICKS_PER_SEC);

   //Scale to milliseconds
   msInFracSeconds =  pTime->time % TIME_TICKS_PER_SEC;
   msInFracSeconds = ( (msInFracSeconds << (32 - BITS_IN_TIME_TICKS_PER_SEC)) + 500 ) / 1000;
   *fractionalSec  = (msInFracSeconds << BITS_IN_TIME_TICKS_PER_SEC);
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSecondsToSysFormat
 *
 * Purpose: Converts the seconds and fractional seconds to system time format.
 *
 * Arguments: uint32_t seconds: Seconds since epoch
 *            uint32_t fractionalSec: Fractional second
 *            sysTime_t *pTime: Return system time in this structure
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_UTIL_ConvertSecondsToSysFormat(uint32_t seconds, uint32_t fractionalSec, sysTime_t *pTime)
{
   uint32_t fracSecInMs;  //Fractional seconds in milli-seconds

   pTime->date   = seconds / SECONDS_PER_DAY;
   pTime->time   = seconds % SECONDS_PER_DAY;
   pTime->time  *= TIME_TICKS_PER_SEC;
   pTime->tictoc = INVALID_SYS_TICTOC;
   pTime->elapsedCycle = 0;

   fracSecInMs  =  fractionalSec >> BITS_IN_TIME_TICKS_PER_SEC; //Get the MSBits
   fracSecInMs  = ( (fracSecInMs * 1000) + 500) >> (32 - BITS_IN_TIME_TICKS_PER_SEC); //Convert to milli-seconds
   fracSecInMs  =  fracSecInMs + SYS_TIME_TICK_IN_mS/2; //Round up/down
   fracSecInMs  =  fracSecInMs - (fracSecInMs % SYS_TIME_TICK_IN_mS); //convert to multiple to systick (in ms)
   pTime->time +=  fracSecInMs;
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSysCombinedToSeconds
 *
 * Purpose: Converts the time passed (system combined) in to a seconds and fractional seconds.
 *
 * Arguments: sysTimeCombined_t *pCombTime:  time to be converted
 *            uint32_t *seconds: Seconds since epoch
 *            uint32_t *fractionalSec: Fractional second
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_UTIL_ConvertSysCombinedToSeconds(sysTimeCombined_t const *pCombTime, uint32_t *seconds, uint32_t *fractionalSec)
{
   uint32_t msInFracSeconds;

   *seconds        = (uint32_t)(*pCombTime / TIME_TICKS_PER_SEC);
   msInFracSeconds = (*pCombTime % TIME_TICKS_PER_SEC); //Get the milliseconds
   msInFracSeconds = ( (msInFracSeconds << (32 - BITS_IN_TIME_TICKS_PER_SEC)) + 500 ) / 1000;
   *fractionalSec  = (msInFracSeconds << BITS_IN_TIME_TICKS_PER_SEC);
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSecondsToSysCombined
 *
 * Purpose: Converts the seconds and fractional seconds to system time format.
 *
 * Arguments: uint32_t seconds: Seconds since epoch
 *            uint32_t fractionalSec: Fractional second
 *            sysTimeCombined_t *pCombTime: Return combined time
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_UTIL_ConvertSecondsToSysCombined(uint32_t seconds, uint32_t fractionalSec, sysTimeCombined_t *pCombTime)
{
   uint32_t fracSecInMs;  //Fractional seconds in milli-seconds

   *pCombTime = (sysTimeCombined_t)seconds * TIME_TICKS_PER_SEC;
   fracSecInMs = fractionalSec >> BITS_IN_TIME_TICKS_PER_SEC; //Get the MSBits
   fracSecInMs = ((fracSecInMs * 1000) + 500) >> (32 - BITS_IN_TIME_TICKS_PER_SEC); //Convert to milli-seconds
   fracSecInMs = fracSecInMs + SYS_TIME_TICK_IN_mS/2; //Round up/down
   fracSecInMs = fracSecInMs - (fracSecInMs % SYS_TIME_TICK_IN_mS); //convert to multiple to systick (in ms)
   *pCombTime += fracSecInMs;
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertMtrHighFormatToSysFormat
 *
 * Purpose: Converts Meter time format (including seconds) to System Time.
 *          Meter time format is in local time and system time will be in UTC.
 *
 * Arguments: MtrTimeFormatHighPrecision_t *pMtrTime - Source Time (in local time)
 *            sysTime_t *pSysTime - Destination Time (in UTC time)
 *
 * Returns: eSUCCESS or eFAILURE
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
#if SUPPORT_METER_TIME_FORMAT
returnStatus_t TIME_UTIL_ConvertMtrHighFormatToSysFormat(MtrTimeFormatHighPrecision_t const *pMtrTime, sysTime_t *pSysTime)
{
   sysTime_dateFormat_t sDateTime;
   returnStatus_t retVal = eSUCCESS;
   if ( pMtrTime->month != 0 || pMtrTime->day != 0 || pMtrTime->year != 0 ||
        pMtrTime->hours != 0 || pMtrTime->minutes != 0 || pMtrTime->seconds != 0 )
   {  //Valid time
      sDateTime.month = pMtrTime->month;
      sDateTime.day   = pMtrTime->day;
      sDateTime.year  = pMtrTime->year + MTR_EPOCH_YEAR; //Meter year is 2 digit, convert to 4 digit year
      sDateTime.hour  = pMtrTime->hours;
      /*lint -e{123} min element same name as macro elsewhere is OK */
      sDateTime.min   = pMtrTime->minutes;
      sDateTime.sec   = pMtrTime->seconds;
      sDateTime.msec  = 0;
      retVal          = TIME_UTIL_ConvertDateFormatToSysFormat(&sDateTime, pSysTime); //Convert to Date in days and Time in ticks
      DST_ConvertLocaltoUTC( pSysTime );
   }
   else
   {  //Invalid time or time unknown
      pSysTime->date = 0;
      pSysTime->time = 0;
   }
   return retVal;
}
#endif

/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_ConvertSysFormatToMtrHighFormat
 *
 * Purpose: This function is to convert sys time to meter format.
 *          System time is in UTC and  meter time format will be in local time.
 *
 * Arguments: sysTime_t *pSysTime - Source time (in UTC time)
 *            MtrTimeFormatHighPrecision_t *pMtrTime - Destination time (in local time)
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE (system time is invalid)
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
#if SUPPORT_METER_TIME_FORMAT
returnStatus_t TIME_UTIL_ConvertSysFormatToMtrHighFormat(sysTime_t const *pSysTime, MtrTimeFormatHighPrecision_t *pMtrTime)
{
   returnStatus_t retVal = eFAILURE;
   sysTime_dateFormat_t sDateTime;
   sysTime_t sSysTime;

   sSysTime.date = pSysTime->date;
   sSysTime.time = pSysTime->time;
   DST_ConvertUTCtoLocal( &sSysTime );

   if (sSysTime.date >= SYS_EPOCH_TO_MTR_EPOCH_DAYS)
   {
      retVal = TIME_UTIL_ConvertSysFormatToDateFormat(&sSysTime, &sDateTime); //Convert to date and time format
      pMtrTime->month = sDateTime.month;
      pMtrTime->day = sDateTime.day;
      pMtrTime->year = (uint8_t)(sDateTime.year - MTR_EPOCH_YEAR); //convert to 2 digit year
      pMtrTime->hours = sDateTime.hour;
      /*lint -e{123} min element same name as macro elsewhere is OK */
      pMtrTime->minutes = sDateTime.min;
      pMtrTime->seconds = sDateTime.sec;
   }
   return(retVal);
}
#endif

/* ************************************ */
/* Get system time in different formats */
/* ************************************ */

/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_GetTimeInDateFormat
 *
 * Purpose: Gets current time in date/time format
 *
 * Arguments: sysTime_dateFormat_t *pDateTime
 *
 * Returns: returnStatus_t - eSUCCESS (System time is valid)
 *                         - eFAILURE (System time is invalid)
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_GetTimeInDateFormat(sysTime_dateFormat_t *pDateTime)
{
   bool retVal;
   sysTime_t currentSysTime;
   uint32_t  additionalMsec;
   uint32_t  freq;

   retVal = TIME_SYS_GetSysDateTime(&currentSysTime);   /* Read the current time */
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&currentSysTime, pDateTime);
   // MKD 2019/27/04 14:44 I wanted to improve the msec precision and updating TIME_UTIL_ConvertSysFormatToDateFormat
   // would have been the place to do it but too much code call this function to convert between formats and I can't
   // garantee that .tictoc and .elapsedCycle would be properly initialized without doing an audit of every call.
   // I can however garantee that TIME_SYS_GetSysDateTime initialized those 2 fields properly.
#if (RTOS_SELECTION == MQX_RTOS)
   freq = ((SYST_RVR + 1) * BSP_ALARM_FREQUENCY)/1000; // Adjust for msec
   additionalMsec = ((currentSysTime.tictoc * (SYST_RVR + 1)) + currentSysTime.elapsedCycle) / freq; // This increases the time resolution from 10msec to msec
#elif (RTOS_SELECTION == FREE_RTOS)
   freq = ( ( SysTick->LOAD + 1 ) * BSP_ALARM_FREQUENCY ) / 1000; // Adjust for msec
   additionalMsec = ((currentSysTime.tictoc * (SysTick->LOAD + 1)) + currentSysTime.elapsedCycle) / freq; // This increases the time resolution from 10msec to msec
#endif
   pDateTime->msec += (uint16_t)additionalMsec;

   return(retVal == true?eSUCCESS:eFAILURE);
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_GetTimeInSysCombined
 *
 * Purpose: Gets current time in combined format (sysTimeCombined_t)
 *
 * Arguments: sysTimeCombined_t *pCombTime - Combined system date/time
 *
 * Returns: returnStatus_t - eSUCCESS (System time is valid)
 *                         - eFAILURE (System time is invalid)
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_GetTimeInSysCombined(sysTimeCombined_t *pCombTime)
{
   returnStatus_t retVal;
   sysTime_t currentSysTime;

   if (TIME_SYS_GetSysDateTime(&currentSysTime))
   {
      *pCombTime = TIME_UTIL_ConvertSysFormatToSysCombined(&currentSysTime);
      retVal = eSUCCESS;
   }
   else
   {  //Invalid system time
      *pCombTime = 0;
      retVal = eFAILURE;
   }
   return(retVal);
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_GetTimeInSecondsFormat
 *
 * Purpose: Gets current time in seconds and fractional seconds
 *
 * Arguments: TIMESTAMP_t *pDateTime - system date/time
 *
 * Returns: returnStatus_t - eSUCCESS (System time is valid)
 *                         - eFAILURE (System time is invalid)
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_GetTimeInSecondsFormat(TIMESTAMP_t *pDateTime)
{
   returnStatus_t retVal;
   sysTime_t currentSysTime;
   uint32_t seconds;
   uint32_t fractionalSec;

   if (TIME_SYS_GetSysDateTime(&currentSysTime))
   {
      TIME_UTIL_ConvertSysFormatToSeconds(&currentSysTime, &seconds, &fractionalSec);
      pDateTime->seconds = seconds;
      pDateTime->QFrac = fractionalSec;
      retVal = eSUCCESS;
   }
   else
   {  //Invalid system time
      pDateTime->QSecFrac = 0;
      retVal = eFAILURE;
   }
   return(retVal);
}

/* ************************************ */
/* Set system time in different formats */
/* ************************************ */


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_SetTimeFromDateFormat
 *
 * Purpose: Sets current time. The source time is in date/time format.
 *          Also updates the RTC
 *
 * Arguments: sysTime_dateFormat_t *pDateTime
 *
 * Returns: returnStatus_t - eSUCCESS (New time was accepted)
 *                         - eFAILURE (New time was rejected)
 *
 * Side effects: Updates system time and RTC time
 *
 * Reentrant: NO. Setting RTC and system time can not be interleaved with another call. There is no need to add
 *            resource lock as in  the real world situation, the set time commands will be coming from one module.
 *
 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_SetTimeFromDateFormat(sysTime_dateFormat_t const *pDateTime)
{
   returnStatus_t retVal = eFAILURE;
   sysTime_t sysTime;

   /* Set the time in RTC */
   if (RTC_SetDateTime (pDateTime))
   {  /* Successfully updated RTC date and time. Update the system time */

      (void)TIME_UTIL_ConvertDateFormatToSysFormat(pDateTime, &sysTime); //Convert to system time format
      TIME_SYS_SetSysDateTime(&sysTime);
      retVal = eSUCCESS;
      //else failed to set system time. Indicate failure. The RTC and System time may be different
   }
   return(retVal);
}


/*****************************************************************************************************************
 *
 * Function name: TIME_UTIL_SetTimeFromSeconds
 *
 * Purpose: Sets current time. The source time is in seconds since epoch and fractional seconds.
 *          Also updates the RTC
 *
 * Arguments: None
 *
 * Returns: returnStatus_t - eSUCCESS (New time was accepted)
 *                         - eFAILURE (New time was rejected)
 * Returns: None
 *
 * Side effects: Updates system time from RTC
 *
 * Reentrant: NO.
 *
 ******************************************************************************************************************/
returnStatus_t TIME_UTIL_SetTimeFromSeconds( uint32_t seconds, uint32_t fractionalSec )
{
   returnStatus_t retVal = eFAILURE;
   sysTime_t sysTime;
   sysTime_dateFormat_t rtcTime;

   TIME_UTIL_ConvertSecondsToSysFormat(seconds, fractionalSec, &sysTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &rtcTime); //convert sys time to RTC time format

   /* Set the time in RTC */
   if (RTC_SetDateTime (&rtcTime))
   {  /* Successfully updated RTC date and time. Update the system time */
      TIME_SYS_SetSysDateTime(&sysTime);
      retVal = eSUCCESS;
      //else failed to set system time. Indicate failure. The RTC and System time may be different
   }
   return(retVal);

}

/***********************************************************************************************************************

   Function Name: TIME_UTIL_OR_PM_Handler

   Purpose: Get parameters related to TIME_SYS

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t TIME_UTIL_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case dateTime :
         {
            if ( sizeof(uint32_t) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               TIMESTAMP_t sTime;
               retVal = TIME_UTIL_GetTimeInSecondsFormat( &sTime );
               *(uint32_t *)value = sTime.seconds;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)(uint16_t)sizeof(uint32_t);
                  attr->rValTypecast = (uint8_t)dateTimeValue;
               }
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}

/***********************************************************************************************************************

   Function Name: TIME_UTIL_SecondsToDateStr

   Purpose: convert a seconds structure into date format string

   Arguments:  buffer - hold the date format sting
               msg    - optionnal string to put in front of the data/time
               t      - time in seconds and fractionnal seconds

   Returns: string length

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
int32_t TIME_UTIL_SecondsToDateStr( char* buffer, char* msg, TIMESTAMP_t t )
{
   int32_t cnt;
   sysTime_t            sysTime;
   sysTime_dateFormat_t sysDateFormat;

   TIME_UTIL_ConvertSecondsToSysFormat(t.seconds, t.QFrac, &sysTime);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat);
   cnt = sprintf(buffer, "%s%02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu.%03hu", msg,
                    sysDateFormat.month, sysDateFormat.day, sysDateFormat.year, sysDateFormat.hour, sysDateFormat.min, sysDateFormat.sec, sysDateFormat.msec); /*lint !e123 */
   return cnt;
}

/***********************************************************************************************************************

   Function Name: TIME_UTIL_ConvertNsecToFracSec

   Purpose: convert nanoseconds into a Q32.32 fractional second format

   Arguments:  nsec - time in nanosecond

   Returns: fractional time

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
uint64_t TIME_UTIL_ConvertNsecToQSecFracFormat(uint64_t nsec)
{
   uint64_t QSecFrac;

   QSecFrac = (nsec << 32 ) / (uint64_t)1000000000;

   return QSecFrac;
}

/***********************************************************************************************************************

   Function Name: TIME_UTIL_GetTimeInQSecFracFormat

   Purpose: Return current time in Q32.32 fractional format

   Arguments:  Nome

   Returns: fractional time in Q32.32 format

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
uint64_t TIME_UTIL_GetTimeInQSecFracFormat(void)
{
   sysTime_t currentSysTime;
   uint64_t msInFracSeconds;
   uint64_t seconds;
   uint64_t QSecFrac;
   uint64_t elapsedCycle;
   uint32_t syst_rvr; // Cache SYST_RVR value to make lint happy because it is volatile

   if (TIME_SYS_GetSysDateTime(&currentSysTime))
   {
      seconds  = (uint64_t)currentSysTime.date * (uint64_t)SECONDS_PER_DAY;
      seconds += (currentSysTime.time / TIME_TICKS_PER_SEC);

      msInFracSeconds =  ((uint64_t)(currentSysTime.time % TIME_TICKS_PER_SEC) << 32) / 1000; // Convert msec in fractional second

#if (RTOS_SELECTION == MQX_RTOS)
      syst_rvr = SYST_RVR+1;
#elif (RTOS_SELECTION == FREE_RTOS)
      syst_rvr = SysTick->LOAD+1;
#endif
      // Sanity check in case RVR changed after elapsed time was computed.
      // This should be very rare and might not even happen but elapsedCycle should never be larger than SYST_RVR
      if ( currentSysTime.elapsedCycle > syst_rvr ) {
         currentSysTime.elapsedCycle = syst_rvr;
      }
#if (RTOS_SELECTION == MQX_RTOS)
      elapsedCycle = ((((uint64_t)currentSysTime.tictoc * (uint64_t)syst_rvr) + (uint64_t)currentSysTime.elapsedCycle) << 32) /
                       (uint64_t)((uint64_t)syst_rvr * (uint64_t)BSP_ALARM_FREQUENCY); // Convert the time spend between Systick in fractional second
#elif (RTOS_SELECTION == FREE_RTOS)
      elapsedCycle = ((((uint64_t)currentSysTime.tictoc * (uint64_t)syst_rvr) + (uint64_t)currentSysTime.elapsedCycle) << 32) /
                       (uint64_t)((uint64_t)syst_rvr * (uint64_t) BSP_ALARM_FREQUENCY); // Convert the time spend between Systick in fractional second
#endif
      // Avoid overflow. That should not happen but better be safe.
      // If it does, saturate.
      if ( ((uint64_t)msInFracSeconds + (uint64_t)elapsedCycle) > 0xFFFFFFFF ) {
         // Staturate
         QSecFrac = (seconds << 32) + 0xFFFFFFFF;
      } else {
         QSecFrac = (seconds << 32) + (msInFracSeconds + elapsedCycle);
      }
   }
   else
   {  //Invalid system time
      QSecFrac = 0;
   }

   return QSecFrac;
}


/***********************************************************************************************************************

   Function Name: TIME_UTIL_ConvertCyccntToQSecFracFormat

   Purpose: Return a CYCCNT number in fractional format

   Arguments: cyccnt - CPU cyccnt value

   Returns: fractional time in Q32.32 format

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
uint64_t TIME_UTIL_ConvertCyccntToQSecFracFormat(uint32_t cyccnt)
{
   uint32_t cpuFreq;
   uint64_t QSecFrac;

   (void)TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL );
   QSecFrac  = (uint64_t)cyccnt << 32;
   QSecFrac /= (uint64_t)cpuFreq;

   return QSecFrac;
}

/***********************************************************************************************************************

   Function Name: TIME_UTIL_PrintQSecFracFormat

   Purpose: Print Q32.32 in seconds.usec

   Arguments: str      - String to print
              QSecFrac - a fractional formated value in Q32.32

   Returns: None

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
void TIME_UTIL_PrintQSecFracFormat(char const * str, uint64_t QSecFrac)
{
   uint32_t sec;
   uint32_t usec;

   sec  = (uint32_t)(QSecFrac >> 32);
   usec = (uint32_t)(((QSecFrac & 0xFFFFFFFF) * 1000000ULL) >> 32);

   INFO_printf("%s fracFormat: %llu %u.%06u", str, QSecFrac, sec, usec);

}

