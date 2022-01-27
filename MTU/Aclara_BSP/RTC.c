/******************************************************************************
 *
 * Filename: RTC.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#include <mqx.h>
#include <bsp.h>
#include <rtc.h>
#include "BSP_aclara.h"
#include "vbat_reg.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: RTC_GetDateTime

  Purpose: This function will return the current Real Time Clock value

  Arguments: RT_Clock - structure that contains the Real Time Clock values

  Returns:

  Notes: There are two clock sources in this project - Real Time Clock, and an
         MQX timer/clock.  We are only modifying the RTC time with this module

*******************************************************************************/
void RTC_GetDateTime ( sysTime_dateFormat_t *RT_Clock )
{
   DATE_STRUCT RTC_Time;
   TIME_STRUCT currentTime = { 0 };

   RTC_GetTimeAtRes (&currentTime, 1);
   (void)_time_to_date( &currentTime, &RTC_Time );

   RT_Clock->month = (uint8_t)RTC_Time.MONTH;
   RT_Clock->day   = (uint8_t)RTC_Time.DAY;
   RT_Clock->year  = (uint16_t)RTC_Time.YEAR;
   RT_Clock->hour  = (uint8_t)RTC_Time.HOUR;
   /*lint -e{123} min element same name as macro elsewhere is OK */
   RT_Clock->min   = (uint8_t)RTC_Time.MINUTE;
   RT_Clock->sec   = (uint8_t)RTC_Time.SECOND;
   RTC_Time.MILLISEC += (int16_t)SYS_TIME_TICK_IN_mS/2;  //Round up
   RT_Clock->msec  = (uint16_t)( ((uint16_t)RTC_Time.MILLISEC / SYS_TIME_TICK_IN_mS) * SYS_TIME_TICK_IN_mS ); //Normalize
} /* end RTC_GetDateTime () */

/*******************************************************************************

  Function name: RTC_SetDateTime

  Purpose: This function will set the Real Time Clock to the passed in values

  Arguments: RT_Clock - structure that contains the Real Time Clock values

  Returns: FuncStatus - true if everything was successful, false if something failed

  Notes: There are two clock sources in this project - Real Time Clock, and an
         MQX timer/clock.  We are only modifying the RTC time with this module

*******************************************************************************/
bool RTC_SetDateTime ( const sysTime_dateFormat_t *RT_Clock )
{
   bool FuncStatus = true;
   DATE_STRUCT mqxDate;    /* Used to convert requested date/time to seconds/milliseconds*/
   TIME_STRUCT mqxTime;    /* Converted date/time sent to set RTC time func.  */

   /* Validate the input date/time  */
   if ( (RT_Clock->month >= MIN_RTC_MONTH)
     && (RT_Clock->month <= MAX_RTC_MONTH)
     && (RT_Clock->day   >= MIN_RTC_DAY)
     && (RT_Clock->day   <= MAX_RTC_DAY)
     && (RT_Clock->year  >= MIN_RTC_YEAR)
     && (RT_Clock->year  <= MAX_RTC_YEAR)
     && (RT_Clock->hour  <= MAX_RTC_HOUR)
     /*lint -e{123} min element same name as macro elsewhere is OK */
     && (RT_Clock->min   <= MAX_RTC_MINUTE)
     && (RT_Clock->sec   <= MAX_RTC_SECOND) )
   {
      mqxDate.MONTH    = RT_Clock->month;
      mqxDate.DAY      = RT_Clock->day;
      mqxDate.YEAR     = (int16_t)RT_Clock->year;
      mqxDate.HOUR     = RT_Clock->hour;
      /*lint -e{123} min element same name as macro elsewhere is OK */
      mqxDate.MINUTE   = RT_Clock->min;
      mqxDate.SECOND   = RT_Clock->sec;
      mqxDate.MILLISEC = 0;

      (void)_time_from_date ( &mqxDate, &mqxTime );   /* Convert to seconds/milliseconds */
      (void)_rtc_set_time ( mqxTime.SECONDS );        /* Set the RTC time */
      _time_set( &mqxTime );

      if (0 == mqxTime.SECONDS)
      {
         VBATREG_RTC_VALID = 0;
      }
      else
      {
         VBATREG_RTC_VALID = 1;
      }
   } /* end if() */
   else
   {
      /* Invalid Parameter passed in */
      (void)printf ( "ERROR - RTC invalid RT_Clock value passed into Set function\n" );
      FuncStatus = false;
   } /* end else() */

   return ( FuncStatus );
} /* end RTC_SetDateTime () */

/*******************************************************************************

  Function name: RTC_Valid

  Purpose: To determine if the RTC is valid.

  Arguments: None

  Returns: RTCValid - True if the RTC is valid, otherwsie false.

  Notes: This is deterimed from two pieces of information.  From the
         TIF (Time Invalid Flag) in the RTC_SR (RTC Status Register)
         and from knowing whether or not the RTC has been set since
         the last time that TIF was true.

*******************************************************************************/
bool RTC_Valid(void)
{
   bool bRTCValid = false;

   if (1 == VBATREG_RTC_VALID)
   {
      bRTCValid = true;
   }

   return (bRTCValid);
} /* end RTC_Valid () */

/*******************************************************************************

  Function name: RTC_GetTimeAtRes

  Purpose: This function will return the current Real Time Clock registers

  Arguments: ptime    - structure for the Real Time Clock values
             fractRes - fractional time resolution in milliseconds

  Returns:

  Notes: DO NOT try to debug through the loop reading the RTC registers. The RTC
         keeps running while dubugger stopped.

*******************************************************************************/
void RTC_GetTimeAtRes ( TIME_STRUCT *ptime, uint16_t fractRes )
{
   uint32_t       seconds;
   uint16_t       fractSeconds;
   RTC_MemMapPtr  rtc;

   if ( ( fractRes != 0 ) && ( 1000 >= fractRes ) )
   {
      rtc          = RTC_BASE_PTR;
      seconds      = (uint32_t)rtc->TSR;
      fractSeconds = (uint16_t)rtc->TPR;
      /* Need to make sure no rollover between reads */
      /* NOTE: If SRFN Debug Macros not used, DO NOT try to debug through this loop. The RTC will keep running when dubugger stopped */
      while ( ( seconds != (uint32_t)rtc->TSR ) || ( fractSeconds != (uint32_t)rtc->TPR ) )
      {
         seconds      = (uint32_t)rtc->TSR;
         fractSeconds = (uint16_t)rtc->TPR;
      }
      ptime->SECONDS      = seconds;
      ptime->MILLISECONDS = ( ( (uint32_t)fractSeconds * (uint32_t)1000 / (uint32_t)fractRes ) / 32768 ) * fractRes;
   }
   return;
} /* end RTC_GetDateTime () */


/*******************************************************************************

  Function name: RTC_GetTimeInSecMicroSec

  Purpose: Return the current Time in seconds and microseconds

  Arguments: uint32_t *sec - Seconds
             uint32_t *microSec - Micro-seconds

  Returns: None

  Notes: DO NOT try to debug through the loop reading the RTC registers. The RTC
         keeps running while dubugger stopped.

*******************************************************************************/
void RTC_GetTimeInSecMicroSec ( uint32_t *sec, uint32_t *microSec )
{
   uint32_t       seconds;
   uint64_t       fractSeconds;
   RTC_MemMapPtr  rtc;

   rtc          = RTC_BASE_PTR;
   seconds      = (uint32_t)rtc->TSR;
   fractSeconds = (uint64_t)rtc->TPR;
   /* Need to make sure no rollover between reads */
   /* NOTE: If SRFN Debug Macros not used, DO NOT try to debug through this loop. The RTC will keep running when dubugger stopped */
   while ( ( seconds != (uint32_t)rtc->TSR ) || ( fractSeconds != (uint64_t)rtc->TPR ) )
   {
      seconds      = (uint32_t)rtc->TSR;
      fractSeconds = (uint64_t)rtc->TPR;
   }
   *sec = seconds;
   *microSec = (uint32_t)((fractSeconds * 1000000) / 32768);

   return;
}

