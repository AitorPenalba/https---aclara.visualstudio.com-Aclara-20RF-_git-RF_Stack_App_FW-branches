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
#include "project.h"
#include "hal_data.h"
//#include <mqx.h>
//#include "OS_aclara.h"

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

/* FUNCTION DEFINITIONS */
/*******************************************************************************

  Function name: OS_TICK_Get_TickCount_HWTicks

  Purpose: This function will return the load the current Tick count value and the HW ticks
           in the OS_TICK_Struct sructure

  Arguments: TickValue - pointer to the current value of the OS Tick counter (populated by this function)

  Returns: Nothing

  Notes:

*******************************************************************************/
void OS_TICK_Get_TickCount_HWTicks( OS_TICK_Struct *TickValue )
{
   taskENTER_CRITICAL( );
   TickValue->HW_TICKS = SysTick->VAL;
   taskEXIT_CRITICAL( );
   TimeOut_t pxTimeOut;
   vTaskSetTimeOutState( &pxTimeOut );
   TickValue->tickCount = pxTimeOut.xTimeOnEntering;
   TickValue->xNumOfOverflows = pxTimeOut.xOverflowCount;

}

/*******************************************************************************

  Function name: OS_TICK_Get_CurrentElapsedTicks

  Purpose: This function will return the elapsed ticks from power up

  Arguments: TickValue - pointer to the current value of the OS Tick counter (populated by this function)

  Returns: Nothing

  Notes:

*******************************************************************************/
void OS_TICK_Get_CurrentElapsedTicks ( OS_TICK_Struct *TickValue )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   _time_get_elapsed_ticks ( TickValue );
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_TICK_Get_TickCount_HWTicks ( TickValue );
#endif
}

/*******************************************************************************

  Function name: OS_Tick_Get_TimeElapsed

  Purpose: This function will return the number of seconds and milliseconds from powerup in time structure

  Arguments: TIME_STRUCT *time_ptr

  Returns: Nothing

  Notes:

*******************************************************************************/
void OS_Tick_Get_TimeElapsed ( TIME_STRUCT *time_ptr )
{
   OS_TICK_Struct TickValue;
   uint64_t sec, msec;
   uint32_t tmp;
   OS_TICK_Get_CurrentElapsedTicks ( &TickValue );
   msec = ( TickValue.tickCount + ( UINT32_MAX * TickValue.xNumOfOverflows ) ) * portTICK_RATE_MS ;
   sec = msec / 1000; // Get the second value from msec
   msec = msec % 1000;
   taskENTER_CRITICAL( );
   tmp = SysTick->LOAD + 1;   // Get the current load value to get the exact CPU frequency
   taskEXIT_CRITICAL( );
   msec += ( tmp - TickValue.HW_TICKS ) / ( tmp / portTICK_RATE_MS );  // Add to the ms fields by modifying the tick value with respect to tick rate

   time_ptr->SECONDS = sec;
   time_ptr->MILLISECONDS = msec;
}

/*******************************************************************************

  Function name: OS_TICK_Get_ElapsedMilliseconds

  Purpose: This function will return the number of milliseconds that have elapsed
           since powerup

  Arguments: Nothing

  Returns: Msec - milliseconds since powerup

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_ElapsedMilliseconds ( void )
{
   uint32_t    Msec;
   TIME_STRUCT time_ptr;
#if ( RTOS_SELECTION == MQX_RTOS )
   _time_get_elapsed ( &time_ptr );
#elif ( RTOS_SELECTION == FREE_RTOS )
   OS_Tick_Get_TimeElapsed ( &time_ptr );
#endif
   Msec = ( ( time_ptr.SECONDS * 1000 ) + time_ptr.MILLISECONDS );

   return ( Msec );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InMicroseconds

  Purpose: This function will return the number of microseconds difference between
           the two passed in Tick Structure values

  Arguments: OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue

  Returns: TimeDiff - difference in microseconds

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InMicroseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
#if ( RTOS_SELECTION == MQX_RTOS )
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_microseconds ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   }
#elif ( RTOS_SELECTION == FREE_RTOS )
   bool isTimeValid = true;
   uint32_t ticksPerMicrosec;
   uint32_t diffTicksCount, diffHWTicks;
   if ( CurrTickValue->tickCount >= PrevTickValue->tickCount )
   {
      if ( ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) >= 1 ) ||
           ( ( CurrTickValue->tickCount == PrevTickValue->tickCount ) && ( CurrTickValue->HW_TICKS > PrevTickValue->HW_TICKS ) ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = CurrTickValue->tickCount - PrevTickValue->tickCount;
   }
   else
   {
      if( ( CurrTickValue->xNumOfOverflows == PrevTickValue->xNumOfOverflows ) ||
          ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) > 1 ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = UINT32_MAX - PrevTickValue->tickCount + CurrTickValue->tickCount;
   }

   if( isTimeValid )
   {
      taskENTER_CRITICAL( );
      uint32_t tmp = SysTick->LOAD + 1;   // Get the current load value to get the exact CPU frequency
      taskEXIT_CRITICAL( );
      ticksPerMicrosec = ( tmp / ( portTICK_RATE_MS * 1000 ) );
      if ( CurrTickValue->HW_TICKS < PrevTickValue->HW_TICKS )
      {
         diffHWTicks = ( PrevTickValue->HW_TICKS - CurrTickValue->HW_TICKS ) / ticksPerMicrosec;
         TimeDiff = ( uint32_t ) ( ( diffTicksCount * portTICK_RATE_MS * 1000 ) + diffHWTicks );
      }
      else
      {
         // Handle hw ticks overflow
         diffHWTicks = ( CurrTickValue->HW_TICKS - PrevTickValue->HW_TICKS ) / ticksPerMicrosec;
         TimeDiff = ( uint32_t ) ( ( ( uint64_t ) diffTicksCount * portTICK_RATE_MS * 1000 ) - diffHWTicks );
      }
   }
#endif

   return ( TimeDiff );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InNanoseconds

  Purpose: This function will return the number of nanosecond difference between
           the two passed in Tick Structure values

  Arguments: OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue

  Returns: TimeDiff - difference in nanoseconds

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InNanoseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
#if ( RTOS_SELECTION == MQX_RTOS )
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_nanoseconds ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   } /* end if() */
#elif ( RTOS_SELECTION == FREE_RTOS )
   bool isTimeValid = true;
   uint32_t ticksPerTenNanoSec;
   uint32_t diffTicksCount, diffHWTicks;
   if ( CurrTickValue->tickCount >= PrevTickValue->tickCount )
   {
      if ( ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) >= 1 ) ||
           ( ( CurrTickValue->tickCount == PrevTickValue->tickCount ) && ( CurrTickValue->HW_TICKS > PrevTickValue->HW_TICKS ) ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = CurrTickValue->tickCount - PrevTickValue->tickCount;
   }
   else
   {
      if( ( CurrTickValue->xNumOfOverflows == PrevTickValue->xNumOfOverflows ) ||
          ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) > 1 ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = UINT32_MAX - PrevTickValue->tickCount + CurrTickValue->tickCount;
   }

   if( isTimeValid )
   {
      taskENTER_CRITICAL( );
      uint32_t tmp = SysTick->LOAD + 1;   // Get the current load value to get the exact CPU frequency
      taskEXIT_CRITICAL( );
      ticksPerTenNanoSec = ( tmp / ( portTICK_RATE_MS * 1000 * 100 ) );
      if ( CurrTickValue->HW_TICKS < PrevTickValue->HW_TICKS )
      {
         diffHWTicks = ( PrevTickValue->HW_TICKS - CurrTickValue->HW_TICKS ) / ticksPerTenNanoSec; // Difference HW ticks per ten nano second
         TimeDiff = ( uint32_t ) ( ( ( diffTicksCount * portTICK_RATE_MS * 1000 * 100 ) + diffHWTicks ) * 10 ); // Convert to once nano second
      }
      else
      {
         // Handle hw ticks overflow
         diffHWTicks = ( CurrTickValue->HW_TICKS - PrevTickValue->HW_TICKS ) / ticksPerTenNanoSec; // Difference HW ticks per ten nano second
         TimeDiff = ( uint32_t ) ( ( ( ( uint64_t ) diffTicksCount * portTICK_RATE_MS * 1000 * 100 ) - diffHWTicks ) * 10 ); // Convert to once nano second
      }
   }
#endif
   return ( TimeDiff );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InMilliseconds

  Purpose: This function will return the number of millisecond difference between
           the two passed in Tick Structure values

  Arguments: OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue

  Returns: TimeDiff - difference in milliseconds

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InMilliseconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
#if ( RTOS_SELECTION == MQX_RTOS )
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_milliseconds ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   } /* end if() */
#elif ( RTOS_SELECTION == FREE_RTOS )
   bool isTimeValid = true;
   uint32_t ticksPerMillisec;
   uint32_t diffTicksCount, diffHWTicks;
   if ( CurrTickValue->tickCount >= PrevTickValue->tickCount )
   {
      if ( ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) >= 1 ) ||
           ( ( CurrTickValue->tickCount == PrevTickValue->tickCount ) && ( CurrTickValue->HW_TICKS > PrevTickValue->HW_TICKS ) ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = CurrTickValue->tickCount - PrevTickValue->tickCount;
   }
   else
   {
      if( ( CurrTickValue->xNumOfOverflows == PrevTickValue->xNumOfOverflows ) ||
          ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) > 1 ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = UINT32_MAX - PrevTickValue->tickCount + CurrTickValue->tickCount;
   }

   if( isTimeValid )
   {
      taskENTER_CRITICAL( );
      uint32_t tmp = SysTick->LOAD + 1;   // Get the current load value to get the exact CPU frequency
      taskEXIT_CRITICAL( );
      ticksPerMillisec = ( tmp / portTICK_RATE_MS );
      if ( CurrTickValue->HW_TICKS < PrevTickValue->HW_TICKS )
      {
         diffHWTicks = ( PrevTickValue->HW_TICKS - CurrTickValue->HW_TICKS ) / ticksPerMillisec;
         TimeDiff = ( uint32_t ) ( ( diffTicksCount * portTICK_RATE_MS ) + diffHWTicks );
      }
      else
      {
         // Handle hw ticks overflow
         diffHWTicks = ( CurrTickValue->HW_TICKS - PrevTickValue->HW_TICKS ) / ticksPerMillisec;
         TimeDiff = ( uint32_t ) ( ( ( uint64_t ) diffTicksCount * portTICK_RATE_MS ) - diffHWTicks );
      }
   }
#endif
   return ( TimeDiff );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InSeconds

  Purpose: This function will return the number of second difference between
           the two passed in Tick Structure values

  Arguments: OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue

  Returns: TimeDiff - difference in seconds

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InSeconds ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
#if ( RTOS_SELECTION == MQX_RTOS )
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_seconds ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   } /* end if() */
#elif ( RTOS_SELECTION == FREE_RTOS )
   bool isTimeValid = true;
   uint32_t diffTicksCount;
   if ( CurrTickValue->tickCount >= PrevTickValue->tickCount )
   {
      if ( ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) >= 1 ) ||
           ( ( CurrTickValue->tickCount == PrevTickValue->tickCount ) && ( CurrTickValue->HW_TICKS > PrevTickValue->HW_TICKS ) ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = CurrTickValue->tickCount - PrevTickValue->tickCount;
   }
   else
   {
      if( ( CurrTickValue->xNumOfOverflows == PrevTickValue->xNumOfOverflows ) ||
          ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) > 1 ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = UINT32_MAX - PrevTickValue->tickCount + CurrTickValue->tickCount;
   }

   if( isTimeValid )
   {
      TimeDiff = ( uint32_t ) ( ( ( uint64_t ) diffTicksCount * portTICK_RATE_MS ) / 1000 ); // Convert to sec
   }
#endif
   return ( TimeDiff );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InMinutes

  Purpose: This function will return the number of minute difference between
           the two passed in Tick Structure values

  Arguments: OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue

  Returns: TimeDiff - difference in minutes

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InMinutes ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
#if ( RTOS_SELECTION == MQX_RTOS )
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_minutes ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   } /* end if() */
#elif ( RTOS_SELECTION == FREE_RTOS )
   bool isTimeValid = true;
   uint32_t diffTicksCount;
   if ( CurrTickValue->tickCount >= PrevTickValue->tickCount )
   {
      if ( ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) >= 1 ) ||
           ( ( CurrTickValue->tickCount == PrevTickValue->tickCount ) && ( CurrTickValue->HW_TICKS > PrevTickValue->HW_TICKS ) ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = CurrTickValue->tickCount - PrevTickValue->tickCount;
   }
   else
   {
      if( ( CurrTickValue->xNumOfOverflows == PrevTickValue->xNumOfOverflows ) ||
          ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) > 1 ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = UINT32_MAX - PrevTickValue->tickCount + CurrTickValue->tickCount;
   }

   if( isTimeValid )
   {
      TimeDiff = ( uint32_t ) ( ( ( uint64_t ) diffTicksCount * portTICK_RATE_MS ) / ( 1000 * 60 ) ); // Convert to min
   }
#endif
   return ( TimeDiff );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Diff_InHours

  Purpose: This function will return the number of hour difference between
           the two passed in Tick Structure values

  Arguments: OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue

  Returns: TimeDiff - difference in hours

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Diff_InHours ( OS_TICK_Struct *PrevTickValue, OS_TICK_Struct *CurrTickValue )
{
   uint32_t TimeDiff;
#if ( RTOS_SELECTION == MQX_RTOS )
   bool     Overflow;

   TimeDiff = (uint32_t)_time_diff_hours ( CurrTickValue, PrevTickValue, &Overflow );

   if ( Overflow == true )
   {
      TimeDiff = 0;
   } /* end if() */
#elif ( RTOS_SELECTION == FREE_RTOS )
   bool isTimeValid = true;
   uint32_t diffTicksCount;
   if ( CurrTickValue->tickCount >= PrevTickValue->tickCount )
   {
      if ( ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) >= 1 ) ||
           ( ( CurrTickValue->tickCount == PrevTickValue->tickCount ) && ( CurrTickValue->HW_TICKS > PrevTickValue->HW_TICKS ) ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = CurrTickValue->tickCount - PrevTickValue->tickCount;
   }
   else
   {
      if( ( CurrTickValue->xNumOfOverflows == PrevTickValue->xNumOfOverflows ) ||
          ( ( CurrTickValue->xNumOfOverflows - PrevTickValue->xNumOfOverflows ) > 1 ) )
      {
         isTimeValid = false;
         TimeDiff = 0;
      }

      diffTicksCount = UINT32_MAX - PrevTickValue->tickCount + CurrTickValue->tickCount;
   }

   if( isTimeValid )
   {
      TimeDiff = ( uint32_t ) ( ( ( uint64_t ) diffTicksCount * portTICK_RATE_MS ) / ( 1000 * 60 * 60 ) ); // Convert to hours
   }
#endif
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
#if ( RTOS_SELECTION == MQX_RTOS )
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
#elif ( RTOS_SELECTION == FREE_RTOS )
   if ( ( FutureTickValue->xNumOfOverflows > CurrTickValue->xNumOfOverflows ) ||
        ( ( FutureTickValue->xNumOfOverflows == CurrTickValue->xNumOfOverflows ) &&
          ( FutureTickValue->tickCount > CurrTickValue->tickCount ) ) )
   {
      TimeGreater = true;
   }
   else
   {
      if ( ( FutureTickValue->tickCount == CurrTickValue->tickCount ) &&
           ( FutureTickValue->HW_TICKS < CurrTickValue->HW_TICKS ) )
      {
         TimeGreater = true;
      }
      else
      {
         TimeGreater = false;
      }
   }
#endif
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

  Returns: Nothing

  Notes: The TickValue that is passed in should be a Tick Time from the past,
         This function will add the TimeDelay to the passed in TickValue and then
         only wake up when this computed Tick time has passed
         This function is useful (with OS_TICK_Get_ElapsedTicks) to wake
         up a process exactly every XX period of time.
         The calling function should guarantee that the TickValue + TimeDelay
         is going to be a Tick Time in the future.  (Code has been added to try
         to protect against sleeping when the time has already passed, but there
         are situation (interrupts,task switch,etc) that could cause errors

*******************************************************************************/
void OS_TICK_Sleep ( OS_TICK_Struct *TickValue, uint32_t TimeDelay )
{
#if ( RTOS_SELECTION == MQX_RTOS )
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
#elif ( RTOS_SELECTION == FREE_RTOS )
   /* NOTE: This works in portTICK_RATE_MS accuracy. For better accuracy, need to modify the algorithm */
   uint32_t ticksToDelay = pdMS_TO_TICKS( TimeDelay );
   uint32_t previousTick = TickValue->tickCount;
   // Convert to ticks from the structure
   if ( ticksToDelay > 0 )
   {
      xTaskDelayUntil ( &previousTick, ticksToDelay ); // TODO: RA6E1 Bob: may need to round this up to a minimum of 5msec of delay. Scot is checking MQX to confirm
   }
#endif

} /* end OS_TICK_Sleep () */

/*******************************************************************************

  Function name: OS_TICK_Add_msec_to_ticks

  Purpose: This function will add the msec value converted and add it to tickCount

  Arguments: OS_TICK_Struct *TickValue, uint32_t TimeDelay

  Returns: OS_TICK_Struct_Ptr

  Notes:

*******************************************************************************/
OS_TICK_Struct_Ptr OS_TICK_Add_msec_to_ticks ( OS_TICK_Struct *TickValue, uint32_t TimeDelay )
{
#if ( RTOS_SELECTION == MQX_RTOS )
   /* We always want to add the TimeDelay to the tick structure, so do it here */
   TickValue = _time_add_msec_to_ticks ( TickValue, TimeDelay );
#elif ( RTOS_SELECTION == FREE_RTOS )
   /* NOTE: This works in portTICK_RATE_MS accuracy. For better accuracy, need to modify the algorithm */
   uint32_t ticksToDelay = pdMS_TO_TICKS( TimeDelay );
   TickValue->tickCount += ticksToDelay;
#endif
   return ( TickValue );
}

/*******************************************************************************

  Function name: OS_TICK_Get_Ticks_per_sec

  Purpose: This function will return the ticks per second

  Arguments: Nothing

  Returns: uint32_t - ticks per sec

  Notes:

*******************************************************************************/
uint32_t OS_TICK_Get_Ticks_per_sec ( void )
{
   uint32_t ticksPerSec;
#if ( RTOS_SELECTION == MQX_RTOS )
   ticksPerSec = _time_get_ticks_per_sec ();
#elif ( RTOS_SELECTION == FREE_RTOS )
   ticksPerSec = 1000 / portTICK_RATE_MS;
#endif
   return ( ticksPerSec );
}