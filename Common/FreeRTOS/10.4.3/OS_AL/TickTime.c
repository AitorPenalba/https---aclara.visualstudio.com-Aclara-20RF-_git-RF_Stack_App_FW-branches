/******************************************************************************
 *
 * Filename: TickTime.c
 *
 * Global Designator:
 *
 * Contents: This file handles all function calls related to the core OS Ticks
 *           Note:  OS Ticks are different than the Real Time Clock - this is
 *                  the OS time value
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
//#include <mqx.h>
#include "OS_aclara.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */
#if 0
/*******************************************************************************

  Function name: OS_TICK_Get_CurrentElapsedTicks

  Purpose: This function will return the current Tick count value

  Arguments: TickValue - pointer to the current value of the OS Tick counter (populated by this function)

  Returns:

  Notes:

*******************************************************************************/
void OS_TICK_Get_CurrentElapsedTicks ( OS_TICK_Struct *TickValue )
{
   _time_get_elapsed_ticks ( TickValue );
}

/*******************************************************************************

  Function name: OS_TICK_Get_ElapsedMilliseconds

  Purpose: This function will return the number of milliseconds that have elapsed
           since powerup

  Arguments:

  Returns: Msec - milliseconds since powerup

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_ElapsedMilliseconds ( void )
{
   uint32_t Msec;
   TIME_STRUCT time_ptr;

   _time_get_elapsed ( &time_ptr );
   Msec = ((time_ptr.SECONDS*1000)+time_ptr.MILLISECONDS);

   return ( Msec );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InMicroseconds

  Purpose: This function will return the number of microseconds difference between
           the two passed in Tick Structure values

  Arguments:

  Returns: TimeDiff - difference in microseconds

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InMicroseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_microseconds ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   }

   return ( TimeDiff );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InNanoseconds

  Purpose: This function will return the number of nanosecond difference between
           the two passed in Tick Structure values

  Arguments:

  Returns: TimeDiff - difference in nanoseconds

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InNanoseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_nanoseconds ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   } /* end if() */

   return ( TimeDiff );
}

/******************************************************************************

  Function Name: OS_TICK_Is_FutureTime_Greater

  Purpose: This function will use two Tick inputs and determine if the 2nd(future) value
           is greater than the 1st(current) value, or not

  Arguments: CurrTickValue - pointer to the current time in the system
             FutureTickValue - pointer to the 2nd value (the value you want to know if it's greater)

  Returns: TimeGreater - if FutureTickValue >= CurrTickValue (return true) - else false

  Notes: the mqx function call returns a int32 value, so the range is 2^31 = 2147483648
         because this is in microseconds - the range is really 2147 seconds or ~35 minutes
         if you want to check a future time that is more than 35 minutes, this fuction won't work

******************************************************************************/
bool OS_TICK_Is_FutureTime_Greater ( OS_TICK_Struct *CurrTickValue, OS_TICK_Struct *FutureTickValue )
{
   bool TimeGreater;
   bool Overflow;

   (void)_time_diff_microseconds ( FutureTickValue, CurrTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeGreater = false;
   } /* end if() */
   else
   {
      TimeGreater = true;
   } /* end else() */

   return ( TimeGreater );
} /* end OS_TICK_Is_FutureTime_Greater () */

/*******************************************************************************

  Function name: OS_TICK_Sleep

  Purpose: This function will pause the processing of the calling task until the
           passed in Tick value (plus Time Delay) has been exceeded

  Arguments: TickValue - pointer to a previous value of the OS Tick counter. Note:
                         (modified in this function to the Tick + Time Delay value)
             TimeDelay - The time in milliseconds that is added to the TickValue
                         (when the task should wake up)

  Returns:

  Notes: The TickValue that is passed in should be a Tick Time from the past,
         This function will add the TimeDelay to the passed in TickValue and then
         only wake up when this computed Tick time has passed
         This function is useful (with OS_TICK_Get_CurrentElapsedTicks) to wake
         up a process exactly every XX period of time.
         The calling function should guarantee that the TickValue + TimeDelay
         is going to be a Tick Time in the future.  (Code has been added to try
         to protect against sleeping when the time has already passed, but there
         are situation (interrupts,task switch,etc) that could cause errors

*******************************************************************************/
void OS_TICK_Sleep ( OS_TICK_Struct *TickValue, uint32_t TimeDelay )
{
   OS_TICK_Struct TempTickValue;
   uint32_t TimeDiff;
   bool     Overflow;

   /* We always want to add the TimeDelay to the tick structure, so do it here */
   TickValue = _time_add_msec_to_ticks ( TickValue, TimeDelay );
   /* Get a current Tick Value (to see where we are at */
   _time_get_elapsed_ticks ( &TempTickValue );
   /* Get the time delta in milliseconds between the two values */
   TimeDiff = (uint32_t)_time_diff_milliseconds ( TickValue, &TempTickValue, &Overflow );
   /* Only sleep if there is time remaining */
   if ( (TimeDiff > 0) && (Overflow == false) )
   {
      _time_delay_until ( TickValue );
   } /* end if() */
} /* end OS_TICK_Sleep () */
#endif