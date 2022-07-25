/******************************************************************************
 *
 * Filename: RTC.c
 *
 * Global Designator: RTC_
 *
 * Contents: RTC abstraction layer
 *
 ******************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012 - 2022 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
  #include <mqx.h>
  #include <bsp.h>
  #include <rtc.h>
#endif
#include "vbat_reg.h"
#if ( MCU_SELECTED == RA6E1 )
  #include "time_sys.h"
  #include "time_util.h"
#endif
/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */
/* Flags used for Alarm Interrupts */
#define SET_FLAG                             1
#define RESET_FLAG                           0

/* TYPE DEFINITIONS */
#if ( MCU_SELECTED == RA6E1 )
typedef struct
{
   union
   {
      uint8_t  Byte[4];
      uint32_t Word;
   }BCount;
   uint8_t SubSec;
}rtc_time_primitive;
#endif

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */
#if ( MCU_SELECTED == RA6E1 )
volatile uint32_t g_alarm_irq_flag = RESET_FLAG;       //flag to check occurrence of alarm interrupt
#endif

/* FUNCTION PROTOTYPES */
#if ( MCU_SELECTED == RA6E1 )
static void rtc_time_get( rtc_time_primitive *time);
static void rtc_time_set( rtc_time_primitive *time);
#endif   //#if ( MCU_SELECTED == RA6E1 )

/* FUNCTION DEFINITIONS */
#if ( MCU_SELECTED == RA6E1 )
/*******************************************************************************

  Function name: RTC_Init

  Purpose: This function will Initialize the Real Time Clock peripheral

  Arguments: void

  Returns: void

*******************************************************************************/
returnStatus_t RTC_init( void )
{
   ( void )R_RTC_Open( &g_rtc0_ctrl, &g_rtc0_cfg );   //Use FSP to setup in calendar mode

   if( (bool)false == RTC_isRunning() )
   {
      //1. Select clock (already performed in RTC_Open)
      //2. Stop and Select Binary Mode (already performed in RTC Open)
      R_RTC->RCR2 = (R_RTC_RCR2_CNTMD_Msk & ~R_RTC_RCR2_START_Msk);
      FSP_HARDWARE_REGISTER_WAIT( R_RTC->RCR2, (R_RTC_RCR2_CNTMD_Msk & ~R_RTC_RCR2_START_Msk)); //Wait for RTC to complete
      //3. Clear RCR1 (already performed in RTC_Open)
      //4. Disable/configure Interrupts (already performed in RTC_Open)
      //5. Reset RTC (already performed in RTC_Open)
      //6. Init all other registers
      R_RTC->BCNT0 = 0;
      R_RTC->BCNT1 = 0;
      R_RTC->BCNT2 = 0;
      R_RTC->BCNT3 = 0;
      /* These registers are set to 0 after an RTC software reset:
            R64CNT
            RSECAR   RMINAR   RHRAR  RWKAR  RDAYAR  RMONAR RYRAR  RYRAREN
            RADJ
            RTCCRn  RSECCPn  RMINCPn  RHRCPn  RDAYCPn  RMONCPn  BCNTnCPm
            BCNTnAR BCNTnAER
         These registers are don't care for binary mode:
            RSECCNT  RMINCNT  RHRCNT  RWKCNT  RDAYCNT  RMONCNT  RYRCNT
         These registers are don't care when using the subclock clock source
            RFRL  RFRH
         Bits in the RCR2 register:
            ADJ30  AADJE  AADJP are set to 0 after an RTC software reset
            RTCOE is set to 0 after system reset
            HR24  is a don't care in binary mode
      */
      RTC_Start();
      VBATREG_RTC_VALID = 0;
   }

   return( eSUCCESS );
} /* end RTC_init () */
#endif   //#if ( MCU_SELECTED == RA6E1 )

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
#if ( MCU_SELECTED == NXP_K24 )
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
#elif ( MCU_SELECTED == RA6E1 )
   TIME_STRUCT          currentTime = { 0 };
   sysTime_t            sysTime;

   RTC_GetTimeAtRes (&currentTime, 1);
   TIME_UTIL_ConvertSecondsToSysFormat(currentTime.SECONDS, 0, &sysTime);
   TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, RT_Clock);
   RT_Clock->msec  = currentTime.MILLISECONDS;
#endif   //#if ( MCU_SELECTED...

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
#if ( MCU_SELECTED == NXP_K24 )
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

#elif ( MCU_SELECTED == RA6E1 )
   /* Validate the input date/time  */
   if ( (RT_Clock->month >= MIN_RTC_MONTH) && (RT_Clock->month <= MAX_RTC_MONTH)
     && (RT_Clock->day   >= MIN_RTC_DAY)   && (RT_Clock->day   <= MAX_RTC_DAY)
     && (RT_Clock->year  >= MIN_RTC_YEAR)  && (RT_Clock->year  <= MAX_RTC_YEAR)
     && (RT_Clock->hour  <= MAX_RTC_HOUR)
     /*lint -e{123} min element same name as macro elsewhere is OK */
     && (RT_Clock->min   <= MAX_RTC_MINUTE)
     && (RT_Clock->sec   <= MAX_RTC_SECOND) )
   {
      rtc_time_primitive currentTime;
      sysTime_t          sysTime;

      //Convert calendar date/time to Seconds (milliseconds not used)
      TIME_UTIL_ConvertDateFormatToSysFormat(RT_Clock, &sysTime);
      TIME_UTIL_ConvertSysFormatToSeconds(&sysTime, &currentTime.BCount.Word, 0);
      rtc_time_set (&currentTime);
      VBATREG_EnableRegisterAccess();
      if(0 == currentTime.BCount.Word)
      {
         VBATREG_RTC_VALID = 0; /*TODO Writing zero to VBTBER Register can happen in LastGasp considering Deep Software Standby Mode*/
      }
      else
      {
         VBATREG_RTC_VALID = 1; /*TODO Writing zero to VBTBER Register can happen in LastGasp considering Deep Software Standby Mode*/
      }
      VBATREG_DisableRegisterAccess();
   } /* end if( (RT_Clock->month >= MIN_RTC_MONTH) && ... */
   else
   {
      /* Invalid Parameter passed in */
      (void)printf ( "ERROR - RTC invalid RT_Clock value passed into Set function\n" );
      FuncStatus = false;
   }
#endif   //#if ( MCU_SELECTED ...

   return ( FuncStatus );
} /* end RTC_SetDateTime () */

/*******************************************************************************

  Function name: RTC_Valid

  Purpose: To determine if the RTC is valid.

  Arguments: None

  Returns: RTCValid - True if the RTC is valid, otherwise false.

  Notes: This is determined from two pieces of information.  From the
         TIF (Time Invalid Flag) in the RTC_SR (RTC Status Register)
         and from knowing whether or not the RTC has been set since
         the last time that TIF was true.

*******************************************************************************/
bool RTC_Valid(void)
{
   bool bRTCValid = false;

   if (1 == VBATREG_RTC_VALID)/*There is no RTC_SR in RA6E1*/
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
         keeps running while debugger stopped.

*******************************************************************************/
void RTC_GetTimeAtRes ( TIME_STRUCT *ptime, uint16_t fractRes )
{
   if ( ( fractRes != 0 ) && ( 1000 >= fractRes ) )
   {
#if ( MCU_SELECTED == NXP_K24 )
      uint32_t       seconds;
      uint16_t       fractSeconds;
      RTC_MemMapPtr  rtc;

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
      ptime->MILLISECONDS = ( ( (uint32_t)fractSeconds   * (uint32_t)1000 / (uint32_t)fractRes ) / 32768 ) * fractRes;
#elif ( MCU_SELECTED == RA6E1 )
      rtc_time_primitive rtcTime;

      rtc_time_get(&rtcTime);
      ptime->SECONDS      = rtcTime.BCount.Word;
      ptime->MILLISECONDS = ( ( (uint32_t)rtcTime.SubSec * (uint32_t)1000 / (uint32_t)fractRes ) /   128 ) * fractRes;
#endif
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
         keeps running while debugger stopped.

*******************************************************************************/
void RTC_GetTimeInSecMicroSec ( uint32_t *sec, uint32_t *microSec )
{
#if ( MCU_SELECTED == NXP_K24 )
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
#elif ( MCU_SELECTED == RA6E1 )
   rtc_time_primitive rtcTime;

   rtc_time_get(&rtcTime);
   *sec      = rtcTime.BCount.Word;
   *microSec = ( (rtcTime.SubSec)*1000000 )/128;
#endif   //#if ( MCU_SELECTED ...

}/* end RTC_GetTimeInSecMicroSec () */

#if ( MCU_SELECTED == RA6E1 )
/*******************************************************************************

  Function name: RTC_ConfigureAlarm

  Purpose: To Configure the RTC Calendar Alarm

  Arguments: uint16_t seconds; Alarm period from now.

  Returns: None

  Notes: Mostly used in Last Gasp

*******************************************************************************/
void RTC_ConfigureAlarm( uint32_t seconds )
{
   rtc_time_primitive time;  //Used for convenience on word/byte conversion

   /* Disable the ICU alarm interrupt request */
   R_BSP_IrqDisable(g_rtc0_ctrl.p_cfg->alarm_irq);
   /* Compare all bits - exact match required */
   R_RTC->BCNT0AER_b.ENB  = 0xF;
   R_RTC->BCNT1AER_b.ENB  = 0xF;
   R_RTC->BCNT2AER_b.ENB  = 0xF;
   R_RTC->BCNT3AER_b.ENB  = 0xF;

   rtc_time_get( &time );        // Get current value of the Registers
   time.BCount.Word += seconds;  // Add the time delay

   R_RTC->BCNT0AR   = time.BCount.Byte[0];
   R_RTC->BCNT1AR   = time.BCount.Byte[1];
   R_RTC->BCNT2AR   = time.BCount.Byte[2];
   R_RTC->BCNT3AR   = time.BCount.Byte[3];

   /* Enable the RTC alarm interrupt */
   R_RTC->RCR1 |= R_RTC_RCR1_AIE_Msk;
   FSP_HARDWARE_REGISTER_WAIT((R_RTC->RCR1 & R_RTC_RCR1_AIE_Msk), R_RTC_RCR1_AIE_Msk);

   R_BSP_SoftwareDelay( 200, BSP_DELAY_UNITS_MICROSECONDS );  // As per Datasheet

   R_BSP_IrqStatusClear(g_rtc0_ctrl.p_cfg->alarm_irq);

   R_BSP_IrqEnable(g_rtc0_ctrl.p_cfg->alarm_irq);   //Enabled the RTC in the NVIC
}

/*******************************************************************************

  Function name: RTC_DisableAlarm

  Purpose: Disable the RTC Alarm

  Arguments: None

  Returns: None

*******************************************************************************/
void RTC_DisableAlarm ( void )
{
   /* Enable the RTC alarm interrupt */
   R_RTC->RCR1 |= R_RTC_RCR1_AIE_Msk;
   FSP_HARDWARE_REGISTER_WAIT((R_RTC->RCR1 & R_RTC_RCR1_AIE_Msk), R_RTC_RCR1_AIE_Msk);

   R_BSP_IrqDisable(g_rtc0_ctrl.p_cfg->alarm_irq);   //Disable the RTC in the NVIC
   /* Disable the RTC alarm interrupt */
   R_RTC->RCR1 &= ~R_RTC_RCR1_AIE_Msk;
   FSP_HARDWARE_REGISTER_WAIT((R_RTC->RCR1 & R_RTC_RCR1_AIE_Msk), 0U);

   /* Compare none of the bits */
   R_RTC->BCNT0AER = 0;
   R_RTC->BCNT1AER = 0;
   R_RTC->BCNT2AER = 0;
   R_RTC->BCNT3AER = 0;
   return;
}

/*******************************************************************************

  Function name: RTC_isRunning

  Purpose: Check if the RTC is running

  Arguments: None

  Returns: None

*******************************************************************************/
bool RTC_isRunning ( void )
{
   bool retVal = (bool)false;

   if ( 1 == R_RTC->RCR2_b.START )
   {
      retVal = (bool) true;
   }

   return (retVal);
}

/*******************************************************************************

  Function name: RTC_Start

  Purpose: Start the RTC if the RTC isn't running.

  Arguments: None

  Returns: None

  Note: RTC should be started to be used as an Alarm
*******************************************************************************/
void RTC_Start ( void )
{
   R_RTC->RCR2_b.START = 1U;

   /* The START bit is updated in synchronization with the next cycle of the count source. Check if the bit is updated
   * before proceeding (see section 26.2.18 "RTC Control Register 2 (RCR2)" of the RA6M3 manual R01UH0886EJ0100)*/
   FSP_HARDWARE_REGISTER_WAIT(R_RTC->RCR2_b.START, 1U);
}

/*******************************************************************************

  Function name: RTC_Callback

  Purpose: Interrupt Handler for RTC Module

  Arguments: rtc_callback_args_t *p_args

  Returns: None

  Notes: Used for Alarm Interrupt.

*******************************************************************************/
void RTC_Callback( rtc_callback_args_t *p_args )
{
   if( RTC_EVENT_ALARM_IRQ == p_args->event )
   {
       g_alarm_irq_flag = SET_FLAG; //Alarm Interrupt occurred
   }/* end if */
}/* end RTC_Callback () */

/*******************************************************************************

  Function name: rtc_time_get

  Purpose: Return the current RTC Time in seconds and sub-seconds

  Arguments: rtc_time_primitive *time - Seconds, sub-seconds

  Returns: None

  Notes: DO NOT try to debug through the loop reading the RTC registers. The RTC
         keeps running while debugger stopped.

*******************************************************************************/
static void rtc_time_get( rtc_time_primitive *time)
{
   time->BCount.Word = 0;
   time->SubSec      = 0;
   while ( ( time->BCount.Byte[0] != R_RTC->BCNT0 ) ||
           ( time->BCount.Byte[1] != R_RTC->BCNT1 ) ||
           ( time->BCount.Byte[2] != R_RTC->BCNT2 ) ||
           ( time->BCount.Byte[3] != R_RTC->BCNT3 ) ||
           ( time->SubSec         != R_RTC->R64CNT) )
   {
      time->BCount.Byte[0] = R_RTC->BCNT0;
      time->BCount.Byte[1] = R_RTC->BCNT1;
      time->BCount.Byte[2] = R_RTC->BCNT2;
      time->BCount.Byte[3] = R_RTC->BCNT3;
      time->SubSec         = R_RTC->R64CNT;
   }
   return;
}

/*******************************************************************************

  Function name: rtc_time_set

  Purpose: Sets the current RTC Time in seconds and sub-seconds

  Arguments: rtc_time_primitive *time - Seconds, sub-seconds

  Returns: None

  Notes: DO NOT try to debug through the loop reading the RTC registers. The RTC
         keeps running while debugger stopped.

*******************************************************************************/
static void rtc_time_set( rtc_time_primitive *time)
{
   //1. Stop RTC and ensure in binary mode
   R_RTC->RCR2 = (R_RTC_RCR2_CNTMD_Msk & ~R_RTC_RCR2_START_Msk);
   FSP_HARDWARE_REGISTER_WAIT( R_RTC->RCR2, (R_RTC_RCR2_CNTMD_Msk & ~R_RTC_RCR2_START_Msk));   //Wait for RTC to set binary mode

   //2. Reset RTC
   R_RTC->RCR2_b.RESET = 1U;
   FSP_HARDWARE_REGISTER_WAIT(R_RTC->RCR2_b.RESET, 0U);

   //3. Set time
   R_RTC->BCNT0 = time->BCount.Byte[0];
   R_RTC->BCNT1 = time->BCount.Byte[1];
   R_RTC->BCNT2 = time->BCount.Byte[2];
   R_RTC->BCNT3 = time->BCount.Byte[3];
   //4. Init all other registers
   /* These registers are set to 0 after an RTC software reset:
         R64CNT
         RSECAR   RMINAR   RHRAR  RWKAR  RDAYAR  RMONAR RYRAR  RYRAREN
         RADJ
         RTCCRn  RSECCPn  RMINCPn  RHRCPn  RDAYCPn  RMONCPn  BCNTnCPm
         BCNTnAR BCNTnAER
      These registers are don't care for binary mode:
         RSECCNT  RMINCNT  RHRCNT  RWKCNT  RDAYCNT  RMONCNT  RYRCNT
      These registers are don't care when using the subclock clock source
         RFRL  RFRH
      Bits in the RCR2 register:
         ADJ30  AADJE  AADJP are set to 0 after an RTC software reset
         RTCOE is set to 0 after system reset
         HR24  is a don't care in binary mode
   */

   RTC_Start();
   return;
}

#if ( TM_RTC_UNIT_TEST == 1 )
/*******************************************************************************

  Function name: RTC_UnitTest

  Purpose: This function will test RTC

  Arguments: None

  Returns: bool - 0 if everything was successful, 1 if something failed

*******************************************************************************/
// TODO: RA6 [name_Balaji]: Move to SelfTest Task
bool RTC_UnitTest(void)
{
   bool                 retVal   = (bool)true;
   uint32_t             seconds  = 0;
   uint32_t             Sec      = 0;
   uint32_t             MicroSec = 0;
   sysTime_dateFormat_t get_time = {0};
   sysTime_dateFormat_t set_time = {0};
   sysTime_t            sysTime;
   uint8_t              cnt;
   uint32_t             result;

   /*These functions are checked:
      RTC_SetDateTime
      RTC_GetDateTime
      RTC_Valid
      RTC_GetTimeAtRes         - Verified indirectly through RTC_GetDateTime
      RTC_GetTimeInSecMicroSec
      RTC_ConfigureAlarm
      RTC_DisableAlarm
      RTC_isRunning
      RTC_Start
      RTC_Callback
      rtc_time_set             - Verified indirectly through RTC_SetDateTime
      rtc_time_get             - Verified indirectly through RTC_GetDateTime
   */
   /*These functions are NOT checked:
      RTC_init  - verified via Startup and by enabling/disabling the RTC and rebooting
   */

   /* Verify RTC can be stopped and detect if running */
   R_RTC->RCR2 &= ~R_RTC_RCR2_START_Msk;  // Direct register call to stop RTC
   FSP_HARDWARE_REGISTER_WAIT( R_RTC->RCR2 & R_RTC_RCR2_START_Msk, 0); //Wait for RTC to complete
   if ( true == RTC_isRunning() )
   {
      retVal = false;
   }
   RTC_Start();
   if ( false == RTC_isRunning() )
   {
      retVal = false;
   }

   /* Verify invalid time can be set */
   memset ((uint8_t *)&sysTime, 0, sizeof(sysTime) );
   TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &set_time);
   if ( false == RTC_SetDateTime(&set_time) )
   {
      retVal = false;
   }
   if ( true == RTC_Valid() )
   {
      retVal = false;
   }
   RTC_GetDateTime(&get_time);
   if ( ( 1970 != get_time.year )  ||
        (    1 != get_time.month ) ||
        (    1 != get_time.day )   ||
        (    0 != get_time.hour )  ||
        (    0 != get_time.min ) ) /* Seconds are not critical and are run-time dependent, so not tested */
   {
      retVal = true;
   }

   /* Verify time can be set prior to 2000 */
   set_time.sec   = 55;
   set_time.min   = 59;
   set_time.hour  = 23;     //24-HOUR mode
   set_time.day   = 31;
   set_time.month = 12;     //1-Jan...12-Dec
   set_time.year  = 1998;    //Year 1970-2099
   if( false == RTC_SetDateTime(&set_time) )
   {
      retVal = false;
   }
   if( false == RTC_Valid() )
   {
      retVal = false;
   }
   memset( (uint8_t *)&get_time, 0, sizeof(get_time) );
   RTC_GetDateTime( &get_time );
   if ( ( 1998 != get_time.year )  ||
        (   12 != get_time.month ) ||
        (   31 != get_time.day )   ||
        (   23 != get_time.hour )  ||
        (   59 != get_time.min ) ) /* Seconds are not critical and are run-time dependent, so not tested */
   {
      retVal = false;
   }

   /* Verify time can be set after to 2000 */
   set_time.sec   = 55;
   set_time.min   = 59;
   set_time.hour  = 23;     //24-HOUR mode
   set_time.day   = 31;
   set_time.month = 12;     //1-Jan...12-Dec
   set_time.year  = 2022;    //Year 1970-2099
   if( false == RTC_SetDateTime (&set_time))
   {
      retVal = false;
   }
   if( false == RTC_Valid() )
   {
      retVal = false;
   }
   memset ((uint8_t *)&get_time, 0, sizeof(get_time) );
   RTC_GetDateTime(&get_time);
   if ( ( 2022 != get_time.year )  ||
        (   12 != get_time.month ) ||
        (   31 != get_time.day )   ||
        (   23 != get_time.hour )  ||
        (   59 != get_time.min ) ) /* Seconds are not critical and are run-time dependent, so not tested */
   {
      retVal = false;
   }

   /* Do not debug (step through) from here to end of function */
   /* Verify time in uSec */
   TIME_UTIL_ConvertDateFormatToSysFormat(&get_time, &sysTime );
   TIME_UTIL_ConvertSysFormatToSeconds(&sysTime, &seconds, 0 );
   RTC_GetTimeInSecMicroSec ( &Sec , &MicroSec);
   do
   {
      cnt = R_RTC->R64CNT;
   }while ( cnt != R_RTC->R64CNT );
   if( seconds != Sec ) // Should not take more than a second from previous time set to here
   {
      retVal = false;
   }
   result = (uint64_t)MicroSec * 128 / 1000000;
   if( ( result > cnt ) || ( result + 1 <= cnt ) )
   {
      retVal = false;
   }

   /* Verify the Alarm can be set and fires */
   RTC_GetTimeInSecMicroSec ( &Sec , &MicroSec);
   Sec += 2;
   g_alarm_irq_flag = RESET_FLAG;
   RTC_ConfigureAlarm(Sec);
   OS_TASK_Sleep(3000);
   if ( RESET_FLAG == g_alarm_irq_flag )
   {
      retVal = false;
   }

   /* Verify the Alarm can be disabled and does not fire */
   RTC_GetTimeInSecMicroSec ( &Sec , &MicroSec);
   RTC_GetDateTime(&get_time);
   Sec += 2;
   g_alarm_irq_flag = RESET_FLAG;
   RTC_ConfigureAlarm(Sec);
   RTC_DisableAlarm();
   OS_TASK_Sleep(3000);
   if ( SET_FLAG == g_alarm_irq_flag )
   {
      retVal = false;
   }
   return retVal;
}
#endif  // #if ( TM_RTC_UNIT_TEST == 1 )
#endif
