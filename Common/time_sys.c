/***********************************************************************************************************************
 *
 * Filename:    time_sys.c
 *
 * Global Designator: TIME_SYS_
 *
 * Contents: Functions related to subscribable Alarm (calendar and periodic alarms)
 *
 ******************************************************************************
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
 *****************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <intrinsics.h>
#define TIME_SYS_GLOBAL
#include "time_sys.h"
#undef  TIME_SYS_GLOBAL
#include "time_util.h"
#include "BSP_aclara.h"
#include "time_DST.h"
#if 0 // TODO: RA6E1 - Support to mac module - Porting for Systick timer
#include "mac.h"
#endif
#include "DBG_SerialDebug.h"
#if ( EP == 1 )
#include "portable_aclara.h"
#if 0 // TODO: RA6E1 - Support to DST and mac modules - Porting for Systick timer
#include "file_io.h"
#include "timer_util.h"
#include "mode_config.h"
#include "radio_hal.h"
#include "radio.h"
#endif
#endif
#if (ACLARA_DA == 1)
#include "b2bMessage.h"
#endif
#if ( DCU == 1)
#include "version.h"
#include <mqx_prv.h>    //DCU2+
#include "FTM.h"        //Used for GPS FTM0_CH2 common for 9975T and 9985T and FTM0_CH4 used for TCXO trimming.
#endif
#if 0 // TODO: RA6E1 - Support to DST and mac modules - Porting for Systick timer
#include "sys_clock.h"
#include "buffer.h"
#include "time_sync.h"
#include "gpio.h"
#endif
#if ( USE_MTLS == 1 )
#include "mtls.h"
#endif

/* ****************************************************************************************************************** */
/* CONSTANTS */
/* These constants will be used for the special case RTC check when patching the DCU to from v1.40 to v1.70.
   TODO: Once TBoards's are upgraded to v1.70, these constants will no longer be needed an can be removed. */
#if ( DCU == 1 )
#define BUILD_YEAR  ((uint16_t)2019) // build year of firmware for RTC patch fix
#define BUILD_MONTH ((uint8_t)5)     // build month of firmware for RTC patch fix
#define BUILD_DAY   ((uint8_t)10)     // build day of firmware for RTC patch fix
#endif

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define TIME_STATE_INVALID                ((uint8_t)0)
#define TIME_STATE_VALID_SYNC             ((uint8_t)2)

#if ( EP == 1 )
#define TIME_SYS_FILE_UPDATE_RATE         ((uint32_t)0) //Frequently
#define DEFAULT_TIME_REQUEST_MAX_TIMEOUT  ((uint16_t)900)
#define DEFAULT_TIME_SIG_OFFSET           ((uint16_t)12000)
#define MIN_TIME_REQUEST_TIMEOUT          ((uint16_t)30)
#define MAX_TIME_REQUEST_TIMEOUT          ((uint16_t)65535) //NOTE: Make sure to add a boundry check if you modify this value.
#define STATUS_TIME_REQUEST_WAIT_STATE    ((uint8_t)1)  //Wait for the time
#endif

#if ( DCU == 1 ) // DCU2+
#define CLOCK_BIAS 0.0001f // Create a bias of 100 usec such that the system clock 5 second transition happens a little before the GPS interrupts.
                           // This prevents a lot of code race and makes the runtime smoother. We don't want a bunch of interrupts to fire at the same time.

#define GPS_WD_RESET           (60*BSP_ALARM_FREQUENCY) // GPS default watchdog value. The counter is decremented by 1 evey system tick. When the counter reaches 0 it means no GPS PPS signal
#define PHASE_LOCKED_THRESHOLD                       10 // 10 means that we consider the phase is locked when error is less than 10/120000000 = 83 nano seconds.
#define ERROR_ALPHA                                0.1f // How much of the new error is kept
#define FREQUENCY_FILTER_LEN                          5 // Length of the frequency filter
#endif

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* The folloing structure is taken from an MQX example showing how to use the system tick.  */
#if (RTOS_SELECTION == MQX_RTOS) // TODO: RA6E1 - Handling in FreeRTOS
typedef struct my_isr_struct_mqx
{
   void *         OLD_ISR_DATA;
   void           (_CODE_PTR_ OLD_ISR)(void *);
} MY_ISR_STRUCT_MQX, *MY_ISR_STRUCT_PTR_MQX;
/* The previous structure is taken from an MQX example.  */
#endif

typedef struct
{
#if (RTOS_SELECTION == MQX_RTOS) // TODO: RA6E1 - Queue handling in FreeRTOS
   OS_QUEUE_Handle pQueueHandle;    /* Queue handle, if not NULL, send message */
   OS_MSGQ_Handle  pMQueueHandle;   /* Message handle, if not NULL, send message */
#endif
   int64_t  timeChangeDelta;        /* number of system ticks of the time change */
   uint32_t ulAlarmDate;            /* Date for the Calendar alarm */
   uint32_t ulAlarmTime_Period;     /* Alarm Time/period */
   uint32_t ulOffset;               /* Offset or local shift time for periodic alarm only */
   unsigned alarmId:             8; /* Alarm ID */
   unsigned bCalAlarm:           1; /* Internal: true = This is a calendar alarm, false = This is a periodic alarm */
   unsigned bCalAlarmDaily:      1; /* Internal: Associated with Calendar Alarms. true = Once a day at the given time */
   unsigned bTimeChanged:        1; /* Internal: true = Time changed */
   unsigned bTimerSlotInUse:     1; /* Internal: true = This alarm slot is in use, false = Alarm slot available */
   unsigned bSkipTimeChange:     1; /* true = Do not call on Time Change, false = Call on Time change */
   unsigned bUseLocalTime:       1; /* true = Use local time, false = Use UTC time */
   unsigned bOnValidTime:        1; /* true = call on Valid Time, false otherwise */
   unsigned bOnInvalidTime:      1; /* true = call on Invalid Time, false otherwise */
   unsigned bExpired:            1; /* Internal: true = alarm expired. Meaningful for one shot Calender alarms */
   unsigned bCauseOfAlarm:       1; /* Internal: Cause of trigger. false = time change only
                                       true = time change and Time for alarm OR Time for alarm only */
   unsigned bTimeChangeForward:  1; /* Internal: true - Time moved forward */
   unsigned bTimeChangeBackward: 1; /* Internal: true - Time moved backward */
   unsigned bRetryAlarm:         1; /* Internal: true - Send failed, retry sending alarm message */
} tTimeSys;

/* Note: The calendar alarms will only be called when system time is valid.
   The periodic alarms can be called, when system is invalid.
   The bOnValidTime and bOnInvalidTime bits are used to configure these periodic alarms

   If only bOnValidTime bit is set and system time is valid
      Alarm triggers, when (_timeSys % ulAlarmTime_Period) == 0
   If only bOnValidBit is set and sys time is NOT valid
      Alarm does not trigger

   If only bOnInvalidTime bit is set and system time is valid
      Alarm triggers, when (_timePup % ulAlarmTime_Period) == 0
   If only nOnInvalidBit is set and sys time is NOT valid
      Alarm triggers, when (_timePup % ulAlarmTime_Period) == 0

   If both bOnValidTime and bOnInvalidTime bits are set and system time is valid
      Alarm triggers, when (_timeSys % ulAlarmTime_Period) == 0
   If both bOnValidTime and bOnInvalidTime bits are set and sys time is NOT valid
      Alarm triggers, when (_timePup % ulAlarmTime_Period) == 0
 */
#if ( EP == 1 )
PACK_BEGIN
typedef struct PACK_MID
{
   uint16_t timeSigOffset;       /* A metrologically significant time offset, in milliseconds */
   uint16_t timeAcceptanceDelay; /* Minimum delay in seconds before accepting subsequent timeSyncs */
   uint32_t timeLastUpdated;     /* The time of the last time update (MAC commands only) */
   uint32_t installDateTime;     /* The time of the first time update ever */
   uint16_t timeRequestMaxTimeout;//The maximum amount of random delay following a request for network time before the time request is retried
   uint16_t dateTimeLostCount;   //Number of times the RTC was lost at power up due to extended power outage
   uint8_t  timeState;           /* The state of the system clock */
}time_vars_t;
PACK_END
#else
typedef struct
{
   uint32_t timeLastUpdated;     /* The time of the last time update ('DT' command) */
   uint8_t  timeState;           /* The state of the system clock */
}time_vars_t;
#endif

typedef struct {
   uint32_t freq;              // Real CPU frequency according to GPS or TCXO
   uint32_t freqLastUpdate;    // Last time the CPU frequency was updated
   TIME_SYS_SOURCE_e source;   // Source of frequency computation
#if ( DCU == 1 ) // DCU2+
   bool     validFreq;         // Frequency in freq is believed to be accurate
   uint32_t fiveSecTickTime;   // When the 5 seconds tick interrupt happened
   float    TickToGPSerror;    // Difference between the T-board 5 second transition and the GPS PPS interrupt
   int32_t  RawTickToGPSerror; // Raw error before filtering
   uint32_t WatchDogCounter;   // GPS watch dog. Positive value means GPS interrupts are being received. 0 means no GPS interrupt.
   uint32_t FTMcount;          // How many FTM count between between 3 GPS interrupts (i.e. 10 seconds)
   bool     sendCommand;       // Command to correct sysTick error
   bool     gotGpsTime;        // Got GPS time from main board
   bool     phaseLocked;       // SysyTick 5 seconds transition is locked (i.e. small error) to GPS interrupt
#endif
} CLOCK_INFO_t;

static CLOCK_INFO_t clockInfo_ = {0};

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
STATIC sysTime_t     _timeSys;                           /* System date/time */
STATIC sysTime_t     _timePup;                           /* Power up time */

static OS_MUTEX_Obj  _timeVarsMutex;                     /* Serialize access to time vars data-structure */
static OS_SEM_Obj    _timeSysSem;                        /* Semaphore to count the # of system ticks */
static bool          _timeSysSemCreated = (bool)false;
STATIC tTimeSys      _sTimeSys[MAX_ALARMS];              /* Manage alarm requests */

static time_vars_t   timeVars_;                          /* Time related variable */

#if ( EP == 1 )
#if 0 // TODO: RA6E1 - File access not supported now
static FileHandle_t  fileHndlTimeSys_ = {0};             /* Contains the file handle information */
#endif
static uint8_t       statusTimeRequest_;                 /* State variable to track get time */
static bool          acceptanceTimerRunning_ = (bool)false;    /* Time-sync within timeAcceptanceDelay */

/* Special calendar alarm to turn ON/OFF DST */
static sysTime_t _sDST_CalAlarm;                         /* DST Calendar alarm */
static bool      _nuDST_TimeChanged;                     /* System time changed */

//TODO: Not used when RTC crystal is source for Sys Tick adjustment.  Put back in when using TCXO.
//static uint32_t  TCXOToSystickError=0;                   /* Error between when the TCXO trimming computes CPU freq and when Systick crosses a second boundary. */
//static uint32_t  CYCdiff=0;
//End TODO
extern uint32_t  DMAint;
#endif

static uint32_t _ticTocCntr = 0;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
#if ( DCU == 1 )
STATIC void             GPS_PPS_IRQ_ISR( void );
static void             ResetGPSStats( void );
#endif
STATIC void             tickSystemClock( uint32_t intTime );
STATIC void             processSysTick( void );
STATIC uint8_t          getUnusedAlarm( void );
STATIC void             getNextCalAlarmDate( uint8_t alarmId, bool timeChangeOrNewAlarm );
STATIC bool             isTimeForPeriodicAlarm( uint8_t alarmId );
STATIC returnStatus_t   executeAlarm( uint8_t alarmId );
STATIC void             TIME_SYS_vApplicationTickHook( void * user_isr_ptr );

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/*****************************************************************************************************************
 *
 * Function name: getSysTime
 *
 * Purpose: retrieve the system time
 *
 * Arguments: sysTime - struture to fill
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
STATIC void getSysTime( sysTime_t *sysTime )
{
   // Disable SysTick interrupt to keep data coherence because we are reading SYST registers that could roll over and date/time/tictoc would be out of sync
   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
   (void)memcpy( sysTime, &_timeSys, sizeof(sysTime_t) );
#if ( MCU_SELECTED == NXP_K24 )
   sysTime->elapsedCycle = SYST_RVR+1; // Retrieve current SYST_RVR because it can change when tracking GPS UTC
   /* Check systick pending interrupt flag */
   if (SCB_ICSR & SCB_ICSR_PENDSTSET_MASK)
   {
      /* We wrapped around */
      sysTime->elapsedCycle *= 2;
   }
   sysTime->elapsedCycle -= SYST_CVR;
#elif ( MCU_SELECTED == RA6E1 )
   sysTime->elapsedCycle = SysTick->LOAD+1;
   if ( SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)
   {
      /* We wrapped around */
      sysTime->elapsedCycle *= 2;
   }
   sysTime->elapsedCycle -= SysTick->VAL;
#endif
   __set_PRIMASK(primask); // Restore interrupts
}

/*****************************************************************************************************************
 *
 * Function name: setSysTime
 *
 * Purpose: set the system time
 *
 * Arguments: sysTime - data used to update the system time struture
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
STATIC void setSysTime( const sysTime_t *sysTime )
{
   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
   (void)memcpy( &_timeSys, sysTime, sizeof(sysTime_t) );
   _timeSys.tictoc       = 0; // Set to 0 since setSysTime is usualy rounded to 10ms so reset counter.
   _timeSys.elapsedCycle = 0; // Set to 0 since sysTime->elapsedCycle was likely not initialized
   __set_PRIMASK(primask);
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_Init
 *
 * Purpose: This function is called before starting the scheduler. Initialize data-structures and create resources
 *          needed by this module
 *
 * Arguments: None
 *
 * Returns: returnStatus_t eSUCCESS/eFAILURE indication
 *
 * Side effects: Clear the alarm structure, power-up time and system time
 *
 * Reentrant: NO. This function should be called once before starting the scheduler
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_Init( void )
{
   returnStatus_t retVal = eFAILURE;   /* Return value */
   sysTime_t timeSys;

   /* Create counting semaphore to keep track of system ticks & mutex to protect Sys Time modules critical section */
   if ( (bool)false == _timeSysSemCreated )
   {
      if ( OS_SEM_Create(&_timeSysSem) && OS_MUTEX_Create(&_timeVarsMutex) )
      {  //Semaphore and Mutex create succeeded, initialize the data structure
#if (EP == 1)
#if 0 // TODO: RA6E1 - File support to be added
         FileStatus_t   fileStatusCfg;  //Contains the file status
#endif
         statusTimeRequest_ = 0; //Default, time not needed
         _nuDST_TimeChanged = (bool)false; //DST related time change notification
#endif
         _timeSysSemCreated = (bool)true;

         (void)memset(_sTimeSys, 0, sizeof(_sTimeSys));  //Initialize System Time data-structure
         /* Clear power-up time and number of half cycles counter */
         _timePup.date = 0;
         _timePup.time = 0;

         /* Start with invalid time */
         timeSys.date   = INVALID_SYS_DATE;
         timeSys.time   = INVALID_SYS_TIME;
         timeSys.tictoc = INVALID_SYS_TICTOC;
         setSysTime( &timeSys );

#if (EP == 1)
#if 0 // TODO: RA6E1 - File support to be added
         if (eSUCCESS == FIO_fopen(&fileHndlTimeSys_, ePART_SEARCH_BY_TIMING, (uint16_t)eFN_TIME_SYS,
                                   (lCnt)sizeof(timeVars_), FILE_IS_NOT_CHECKSUMED,
                                   TIME_SYS_FILE_UPDATE_RATE, &fileStatusCfg) )
         {
            if (fileStatusCfg.bFileCreated)
            {
               (void)memset(&timeVars_, 0, sizeof(timeVars_));
               timeVars_.installDateTime       = 0;
               timeVars_.timeLastUpdated       = 0;
               timeVars_.timeAcceptanceDelay   = 60;
               timeVars_.timeSigOffset         = DEFAULT_TIME_SIG_OFFSET;
               timeVars_.timeRequestMaxTimeout = DEFAULT_TIME_REQUEST_MAX_TIMEOUT;
               timeVars_.timeState             = TIME_STATE_INVALID;

               retVal = FIO_fwrite(&fileHndlTimeSys_, 0, (uint8_t *)&timeVars_, (lCnt)sizeof(timeVars_));
            }
            else
            {  //read the file
               retVal = FIO_fread(&fileHndlTimeSys_, (uint8_t *)&timeVars_, 0, (lCnt)sizeof(timeVars_));
            }
         }
#endif
         retVal = eSUCCESS; // TODO: RA6E1 - Remove this once file support added. Handle #if and #else cases accordingly
#else
         timeVars_.timeLastUpdated = 0;
         timeVars_.timeState       = TIME_STATE_INVALID;

         retVal = eSUCCESS;
#endif
      }
   }
#if 0 // TODO: RA6E1 [name_Suriya] - Modify variable to get core clock from a function
   TIME_SYS_SetRealCpuFreq( getCoreClock(), eTIME_SYS_SOURCE_NONE, (bool)true ); // Nominal CPU clock rate
#else
   TIME_SYS_SetRealCpuFreq( SystemCoreClock, eTIME_SYS_SOURCE_NONE, (bool)true ); // Nominal CPU clock rate
#endif
#if ( DCU == 1 )
   if (VER_getDCUVersion() != eDCU2) {
      ResetGPSStats(); // The CPU freq is not reset here because we want to keep whatever value was computed until we lost the GPS
   }
#endif
   return retVal;
}

#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_setTimeSigOffset
 *
 * Purpose: sets the Time Significant offset
 *
 * Arguments: desired offset
 *
* Returns: uint16_t: success/fail
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_setTimeSigOffset( uint16_t offset )
{
   returnStatus_t retVal = eFAILURE;
   if( (TIME_SIG_OFFSET_MIN <= offset) && (TIME_SIG_OFFSET_MAX >= offset))
   {
      timeVars_.timeSigOffset = offset;
      retVal = eSUCCESS;
   }
   return retVal;
}
#endif

#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_getTimeSigOffset
 *
 * Purpose: returns the Time Significant offset
 *
 * Arguments: None
 *
 * Returns: uint16_t: Offset
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
uint16_t TIME_SYS_getTimeSigOffset( void )
{
   return(timeVars_.timeSigOffset);
}
#endif

#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetDSTAlarm
 *
 * Purpose: This function is called when new system time is accepted
 *
 * Arguments: uint32_t date, uint32_t time
 *
 * Returns: None
 *
 * Side effects: Updates calendar alarm date and time
 *
 * Reentrant: No
 *
 ******************************************************************************************************************/
void TIME_SYS_SetDSTAlarm( uint32_t date, uint32_t time )
{
   _sDST_CalAlarm.date = date;
   _sDST_CalAlarm.time = time;
}
#endif

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetTimeFromRTC
 *
 * Purpose: Sets current time to RTC time. This function is called on power-up to sync system time to RTC time
 *
 * Arguments: None
 *
 * Returns: returnStatus_t - eSUCCESS, eFAILURE
 *
 * Side effects: Updates system time from RTC
 *
 * Reentrant: NO.
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_SetTimeFromRTC(void)
{
   sysTime_t sysTime;
   sysTime_dateFormat_t rtcTime = { 1, 1, SYS_EPOCH_YEAR, 0, 0, 0, 0 }; // Default to 1970/01/01 00:00:00 if RTC is bad
#if ( EP == 1 )
   returnStatus_t retVal;
#else
   returnStatus_t retVal = eSUCCESS;
#endif

   TIME_STRUCT mqxTime;

   if ( RTC_Valid() )
   {  //RTC have valid time. Read RTC and set system time
      RTC_GetDateTime (&rtcTime);
   }
#if ( DCU == 1 )
   else
   {  /* This else clause was added to accomadate dfw patches from DCU v1.40 to v1.70.  The VBATREG_RTC_VALID
         flag was not utilized by the DCU in version v1.40. When patching to v1.70, the RTC_Valid check will always
         fail since this bit was never set in the previous version.  The else clause will validate the RTC values,
         making a decision to use these values if the time in the RTC makes sense or else it will use invalid time.
         Later on when the Main Board updates time on the TBoard, the VBATREG_RTC_VALID will get set.
         TODO: Once TBoards are running at v1.70, this "ELSE" scenario will no longer be required and can be
         removed from the firmware. */
      sysTime_dateFormat_t tempRtcTime;   // temporary variable to read current RTC time
      RTC_GetDateTime(&tempRtcTime);
      INFO_printf("VBAT register bit for RTC is NOT set.");
      // is the RTC date greater than or equal to the build date
      if( ( tempRtcTime.year > BUILD_YEAR ) || ( tempRtcTime.year == BUILD_YEAR && tempRtcTime.month >= BUILD_MONTH ) ||
          ( tempRtcTime.year == BUILD_YEAR && tempRtcTime.month == BUILD_MONTH && tempRtcTime.day >= BUILD_DAY ) )
      {  // RTC time appears to be valid, go ahead and use this time
         (void)memcpy((uint8_t *)&rtcTime, (uint8_t *)&tempRtcTime, sizeof(tempRtcTime));
         INFO_printf("Current RTC value has been evaluated and accepted.  RTC value will be used to set system time.");
      }
      else
      {
         INFO_printf("Device does NOT have a valid RTC, the system time will be initialized to invalid.");
      }
   }
#endif

   (void)TIME_UTIL_ConvertDateFormatToSysFormat(&rtcTime, &sysTime); //Convert to system time format
   TIME_UTIL_ConvertSysFormatToSeconds(&sysTime, &mqxTime.SECONDS, &mqxTime.MILLISECONDS);
   mqxTime.MILLISECONDS = sysTime.time % TIME_TICKS_PER_SEC;
#if 0 // TODO: RA6E1 - MQX dependant functions in FreeRTOS
   _time_set( &mqxTime );
#endif
   TIME_SYS_SetSysDateTime(&sysTime);

#if ( EP == 1 )
#if 0 // TODO: RA6E1 - File support to be added
   //Write the time state variable
   retVal = FIO_fwrite(&fileHndlTimeSys_, (uint16_t)offsetof(time_vars_t, timeState),
                    (uint8_t *)&timeVars_.timeState, (lCnt)sizeof(timeVars_.timeState));
#endif
#endif
   return(retVal);
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_TimeState
 *
 * Purpose: returns the timeState
 *
 * Arguments: None
 *
 * Returns: uint8_t
 *
 ******************************************************************************************************************/
uint8_t TIME_SYS_TimeState(void)
{
   return timeVars_.timeState;
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetTimeState
 *
 * Purpose: Used to set the time state
 *
 * Arguments: state
 *
 * Returns: void
 *
 ******************************************************************************************************************/
void TIME_SYS_SetTimeState(uint8_t state)
{
   timeVars_.timeState = state;
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetSourceMsg
 *
 * Purpose: Returns the Message for Get Source Msg
 *
 * Arguments: source
 *
 * Returns: source as a message
 *
 ******************************************************************************************************************/
char* TIME_SYS_GetSourceMsg(TIME_SYS_SOURCE_e source)
{
   char* msg="";
   switch(source)
   {
      case eTIME_SYS_SOURCE_NONE: msg = "NONE"; break;
      case eTIME_SYS_SOURCE_TCXO: msg = "TCXO"; break;
      case eTIME_SYS_SOURCE_CPU:  msg = "CPU "; break;
      case eTIME_SYS_SOURCE_GPS:  msg = "GPS "; break;
      case eTIME_SYS_SOURCE_AFC:  msg = "AFC "; break;
   }
   return msg;
}


#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetTimeRequestMaxTimeout
 *
 * Purpose: returns the maximum amount of random delay following a request for network time before the time request is retried.
 *
 * Arguments: None
 *
 * Returns: uint16_t
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
uint16_t TIME_SYS_GetTimeRequestMaxTimeout( void )
{
   uint16_t timeoutValue;
   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
   timeoutValue = timeVars_.timeRequestMaxTimeout;
   OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails
   return (timeoutValue);
}
#endif
#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetTimeRequestMaxTimeout
 *
 * Purpose: set the maximum amount of random delay following a request for network time before the time request is retried.
 *
 * Arguments: uint16_t
 *
 * Returns: reeturnStatus_t
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_SetTimeRequestMaxTimeout( uint16_t timeoutValue )
{
    returnStatus_t retVal = eAPP_INVALID_VALUE;

   if ( timeoutValue >= MIN_TIME_REQUEST_TIMEOUT )
   {
      OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
      timeVars_.timeRequestMaxTimeout = timeoutValue;
#if 0 // TODO: RA6E1 - File support to be added
      (void)FIO_fwrite(&fileHndlTimeSys_, 0, (uint8_t *)&timeVars_, sizeof(timeVars_));
#endif
      OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails
      retVal = eSUCCESS;
   }
   return (retVal);
}
#endif
#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetDateTimeLostCount
 *
 * Purpose: Returns the number of times the RTC has lost date and time due to an extended power outage
 *
 * Arguments: None
 *
 * Returns: uint16_t
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
uint16_t TIME_SYS_GetDateTimeLostCount( void )
{
   uint16_t uDateTimeLostCount;
   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
   uDateTimeLostCount = timeVars_.dateTimeLostCount;
   OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails
   return (uDateTimeLostCount);
}
#endif
#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetDateTimeLostCount
 *
 * Purpose: Sets the dateTimeLostCount parameter
 *
 * Arguments: uint16_t
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
void TIME_SYS_SetDateTimeLostCount( uint16_t dateTimeLostValue )
{
   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
   timeVars_.dateTimeLostCount = dateTimeLostValue;
#if 0 // TODO: RA6E1 - File support to be added
   (void)FIO_fwrite(&fileHndlTimeSys_, 0, (uint8_t *)&timeVars_, sizeof(timeVars_));
#endif
   OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails
}
#endif

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetTimeLastUpdated
 *
 * Purpose: Returns the number of times the RTC has lost date and time due to an extended power outage
 *
 * Arguments: None
 *
 * Returns: uint16_t
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
uint32_t TIME_SYS_GetTimeLastUpdated( void )
{
   uint32_t uTimeLastUpdated;
   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
   uTimeLastUpdated = timeVars_.timeLastUpdated;
   OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails
   return (uTimeLastUpdated);
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetSysDateTime
 *
 * Purpose: This function is called when new system time is accepted
 *
 * Arguments: sysTime_t *pSysTime - Pointer to new time
 *
 * Returns: None
 *
 * Side effects: Updates System time and sets appropriate alarm flags
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_SYS_SetSysDateTime( const sysTime_t *pSysTime )
{
   uint8_t         index;                       /* Index */
   bool            bLocalTimeChanged;           /* Indicate time moved */
   bool            bLocalTimeChangeForward;     /* Indicate time moved forward */
   bool            bLocalTimeChangeBackward;    /* Indicate time moved backward */
   tTimeSys       *pTimeSys;                    /* Pointer to alarm data structure */
   uint32_t        sysTickTime;                 /* Used to ensure time is modulo sysTick */
   sysTime_t       timeSys;
   uint32_t        fractionalSec;               /* Used for ConvertSysFormatToSeconds function below */
#if ( DCU == 1 )
   static bool     firstCall = true; //DCU2+
#endif
   bLocalTimeChanged        = (bool)false;
   bLocalTimeChangeForward  = (bool)false;
   bLocalTimeChangeBackward = (bool)false;

   // We are going to access/modify lots of time related variables so get mutex
   //OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // Disable all interrupts. This section is time critical.

   // Time adjustement is time sensitive.
   // We have to adjust time quickly
   // We don't want to grab the current time, make some computation and can't commit the updated value for a long time because of preemption by a higher priority tasks

   getSysTime( &timeSys );

#if (EP == 1)
   _nuDST_TimeChanged = (bool)true; //Indicate time change to the DST module
#endif
   if ( (INVALID_SYS_DATE == pSysTime->date) && (INVALID_SYS_TIME == pSysTime->time) )
   {  /* System invalid  */
      timeSys.date        = INVALID_SYS_DATE;
      timeSys.time        = INVALID_SYS_TIME;
      timeSys.tictoc      = INVALID_SYS_TICTOC;
      sysTickTime         = INVALID_SYS_TIME;
      timeVars_.timeState = TIME_STATE_INVALID;  // Setting the time to 1970/01/01 00:00:00 is the equivalent of settting the time to invalid
      bLocalTimeChanged   = (bool)true;
   }
   else
   {  /* System time is valid */
      /* Round time to ensure it is always on a SYS_TIME_TICK_IN_mS interval */
      sysTickTime = pSysTime->time + SYS_TIME_TICK_IN_mS/2;
      sysTickTime = sysTickTime - (sysTickTime % SYS_TIME_TICK_IN_mS);

      timeVars_.timeState = TIME_STATE_VALID_SYNC;  //Valid time
#if (EP == 1)
      /* Only set installation date/time if not already set AND NOT in ship mode */
      /* NOTE: For the T-board, the equivalent of installDateTime is stored in RegistrationInfo.installationDateTime */
#if 0 // TODO: RA6E1 - Ship mode support to be added
      if ( ( 0 == timeVars_.installDateTime ) && (0 == MODECFG_get_ship_mode()) )
#else
      if ( ( 0 == timeVars_.installDateTime ) )
#endif
      {  //store the first time-sync or timeset command ever
         uint32_t date_Time;   //Used since cannot pass pointer to uint32_t member of a packed struct
         TIME_UTIL_ConvertSysFormatToSeconds(pSysTime, &date_Time, &fractionalSec);
         timeVars_.installDateTime = date_Time;
      }
#endif
#if ( DCU == 1 )
      if (VER_getDCUVersion() != eDCU2) {
         if ( clockInfo_.WatchDogCounter ) {
            // Use GPS time only after we are synchronized to GPS. This is to avoid suddent time jump
            // Use the time at face value otherwise
            if ( !firstCall ) {
               // GPS is present which means interrupts are generated every 5 seconds
               // This also means the time received (+DT) should be accurate within 5 seconds since it should have been generated by the GPS
               // So we keep the "5 sec" part from our current time and update the rest with received (GPS) time.
               // If time is set through the "time" command, then the time will be valid until the 'DT' command is received again.

               // Round received time to 5 sec precision
               sysTickTime = (sysTickTime/(5*TIME_TICKS_PER_SEC)) * (5*TIME_TICKS_PER_SEC);

               // Combine with GPS 5 sec precision
               sysTickTime += timeSys.time % (5*TIME_TICKS_PER_SEC);

               // Got UTC time from GPS (through main board)
               clockInfo_.gotGpsTime = true;

   //            DBG_LW_printf( "Set gotGPSTime\n" );
            }
            firstCall = false;
         }
      }
#endif
      if ( (INVALID_SYS_DATE == timeSys.date) && (INVALID_SYS_TIME == timeSys.time) )
      {  //Invalid sys time to valid time
         bLocalTimeChanged        = (bool)true;
         bLocalTimeChangeForward  = (bool)true;
         bLocalTimeChangeBackward = (bool)true;
      }
      else if ( (pSysTime->date > timeSys.date) ||
           ((pSysTime->date == timeSys.date) && (sysTickTime > timeSys.time) ) )
      {  /* Time moved forward */
         bLocalTimeChanged       = (bool)true;
         bLocalTimeChangeForward = (bool)true;
      }
      else if ( (pSysTime->date != timeSys.date) || (sysTickTime != timeSys.time) )
      {  /* Time moved back */
         bLocalTimeChanged        = (bool)true;
         bLocalTimeChangeBackward = (bool)true;
      }
   }
   /* Compute the delta time.  Forward time change is a positive result, reverse is a neg result. */
   int64_t deltaTime;
   deltaTime = ( (pSysTime->date * (uint64_t)TIME_TICKS_PER_DAY) + sysTickTime ) -
               ( (timeSys.date  * (uint64_t)TIME_TICKS_PER_DAY) + timeSys.time );

#if ( DCU == 1 )
   // If time received doesn't change current time then don't ajust.
   // This is what should happen most of the time when GPS is working.
   // This is to prevent prevent the time from being jerked around by millseconds.
   if (  (VER_getDCUVersion() == eDCU2) ||
       ( (VER_getDCUVersion() != eDCU2) &&
       ( (timeSys.date != pSysTime->date) || ((timeSys.time/5000) != (sysTickTime/5000)) ||
         ((INVALID_SYS_DATE == pSysTime->date) && (INVALID_SYS_TIME == pSysTime->time)) ) ) )
#endif
   {
      timeSys.date = pSysTime->date;
      timeSys.time = sysTickTime;
      setSysTime( &timeSys );
   }

   for ( index = 0, pTimeSys = &_sTimeSys[0]; index < ARRAY_IDX_CNT(_sTimeSys); index++, pTimeSys++ )
   {  /* Check every alarm Slot */
      if ( pTimeSys->bTimerSlotInUse )
      {  /* Alarm in use */
         pTimeSys->timeChangeDelta = deltaTime;
         if ( pTimeSys->bCalAlarm && pTimeSys->bCalAlarmDaily && /* Daily calendar alarm */
              bLocalTimeChanged ) /* Time changed */
         {  /* Recompute next alarm date for daily calendar alarm on time change */
            getNextCalAlarmDate(index, (bool)true); // set next alarm date
         }
         if ( !pTimeSys->bSkipTimeChange && bLocalTimeChanged )
         {  /* Only set this flag, if alarm on time change is requested */
            pTimeSys->bTimeChanged = bLocalTimeChanged;
         }
         if ( bLocalTimeChangeForward )
         {
            pTimeSys->bTimeChangeForward = bLocalTimeChangeForward;
         }
         if ( bLocalTimeChangeBackward )
         {
            pTimeSys->bTimeChangeBackward = bLocalTimeChangeBackward;
         }
      }
   }

   if ( bLocalTimeChanged ) /* If time change, update timeLastUpdated parameter */
   {
      uint32_t date_Time;   //Used since cannot pass pointer to uint32_t member of a packed struct
      TIME_UTIL_ConvertSysFormatToSeconds(pSysTime, &date_Time, &fractionalSec);
      timeVars_.timeLastUpdated = date_Time;
   }

   __set_PRIMASK(primask); // Restore interrupts
   //OS_MUTEX_Unlock( &_timeVarsMutex );  // Function will not return if it fails
#if ( EP == 1 )
#if 0 // TODO: RA6E1 - File support to be added
   (void)FIO_fwrite(&fileHndlTimeSys_, 0, (uint8_t*)&timeVars_, (lCnt)sizeof(timeVars_));
#endif
#endif
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetSysDateTime
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
bool TIME_SYS_GetSysDateTime( sysTime_t *pSysTime )
{
   bool boolSTValid;   /* Returns if system time is invalid */

   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
   boolSTValid = TIME_SYS_IsTimeValid();
   getSysTime( pSysTime );
   __set_PRIMASK(primask); // Restore interrupts

   return boolSTValid;
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_IsTimeValid
 *
 * Purpose: Returns a boolean that states if time is valid or not valid
 *
 * Arguments: None
 *
 * Returns: bool boolSTValid - false: System time is invalid OR Fatal error, true otherwise
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 * NOTE: This function disables interrupts instead of using a mutex in case it is called from inside an interrupt
 *
 ******************************************************************************************************************/
bool TIME_SYS_IsTimeValid( void )
{
   bool boolSTValid = false;

   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
   if ( timeVars_.timeState == TIME_STATE_VALID_SYNC ) {
      boolSTValid = true;
   }
   __set_PRIMASK(primask); // Restore interrupts

   return boolSTValid;
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetPupDateTime
 *
 * Purpose: This function returns power-up date and time in _timePup structure
 *
 * Arguments: sysTime_t *pPupTime - Pointer to time structure
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_SYS_GetPupDateTime( sysTime_t *pPupTime )
{
   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
   pPupTime->date = _timePup.date;
   pPupTime->time = _timePup.time;
   __set_PRIMASK(primask); // Restore interrupts
}

#if (EP == 1)
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetInstallationDateTime
 *
 * Purpose: This function returns installation date and time in seconds since epoch. 0 if invalid.
 *
 * Arguments: None
 *
 * Returns: uint32_t seconds: Installation date and time in seconds since epoch
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
uint32_t TIME_SYS_GetInstallationDateTime( void )
{
   return(timeVars_.installDateTime);
}
#endif

#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetInstallationDateTime
 *
 * Purpose: This function sets the installation date and time in seconds since epoch.
 *
 * Arguments: uint32_t seconds: Installation date and time in seconds since epoch
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
void TIME_SYS_SetInstallationDateTime( uint32_t instDateTime )
{
   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
   timeVars_.installDateTime = instDateTime;
#if 0 // TODO: RA6E1 - File support to be added
   (void)FIO_fwrite(&fileHndlTimeSys_, (uint16_t)offsetof(time_vars_t,installDateTime),
                    (uint8_t*)&timeVars_.installDateTime, (lCnt)sizeof(timeVars_.installDateTime));
#endif
    OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails
}
#endif

#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetTimeAcceptanceDelay
 *
 * Purpose: This function returns the time acceptance delay in seconds.
 *
 * Arguments: None
 *
 * Returns: uint16_t timeAcceptanceDelay: Time acceptance delay in seconds
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
uint16_t TIME_SYS_GetTimeAcceptanceDelay( void )
{
   return(timeVars_.timeAcceptanceDelay);
}
#endif

#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetTimeAcceptanceDelay
 *
 * Purpose: This function sets the time acceptance delay in seconds.
 *
 * Arguments: uint16_t setTimeAcceptanceDelay: Time acceptance delay in seconds
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_SetTimeAcceptanceDelay( uint16_t setTimeAcceptanceDelay )
{
   returnStatus_t returnStatus = eFAILURE;

   if (setTimeAcceptanceDelay <= TIME_SYS_MAX_TIME_ACC_DELAY)
   {
      OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
      timeVars_.timeAcceptanceDelay = setTimeAcceptanceDelay;
#if 0 // TODO: RA6E1 - File support to be added
      returnStatus = FIO_fwrite(&fileHndlTimeSys_, (uint16_t)offsetof(time_vars_t,timeAcceptanceDelay),
                                (uint8_t*)&timeVars_.timeAcceptanceDelay,
                                (lCnt)sizeof(timeVars_.timeAcceptanceDelay));
#endif
      OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails
   }

   return(returnStatus);
}
#endif

/*****************************************************************************************************************
 *
 * Function name: tickSystemClock
 *
 * Purpose: This function increments the system and power-up time by 1 system tick when sys tick interrupt fires.
 *
 * Arguments: uint32_t intTime - Interrupt time. When the interrupt happened. (For DCU2+)
 *
 * Returns: None
 *
 * Side effects: Increments the system and power-up time
 *
 * Note: Called from interrupt
 *
 * Reentrant:
 *
 ******************************************************************************************************************/
STATIC void tickSystemClock ( uint32_t intTime )
{
#if ( DCU != 1 )
   ( void ) intTime;
#endif

   // Increment tic toc
   _ticTocCntr++;
   _ticTocCntr %= (SYS_TIME_TICK_IN_mS/portTICK_RATE_MS);

   _timeSys.tictoc = _ticTocCntr;
   if ( _timeSys.tictoc == 0) {
      _timeSys.time+=SYS_TIME_TICK_IN_mS;

#if ( DCU == 1 )
      if ( (_timeSys.time % 5000) == 0) {
         clockInfo_.fiveSecTickTime = intTime; //DCU2+
         // Toggle LED to help debug GPS PPS
         //LED2_PIN_ON();
      }
#if ( TEST_TDMA == 1 )
      if ( (_timeSys.time % 1000) < 200) {
         BLU_LED_ON();
      } else {
         BLU_LED_OFF();
      }
#else
      // To save power, toggle blue LED with a 1% ON/99% OFF duty cycle
      if ( (_timePup.time % TIME_TICKS_PER_SEC) < 10 ) {
         BLU_LED_ON();
      } else {
         BLU_LED_OFF();
      }
#endif
#endif

#if ( EP == 1)
#if ( TEST_TDMA == 1)
      if ( (_timeSys.time % 200) == 0) {
         LED_toggle(BLU_LED);
//         LED_toggle(GRN_LED);
      }
#endif
//TODO: Not used when RTC crystal is source for Sys Tick adjustment.  Put back in when using TCXO.
#if 0
      if ( (_timeSys.time % 1000) == 0) {
         if ( DMAint != 0 ) {
            CYCdiff = DWT_CYCCNT - DMAint;
         }
      }
#endif
//End TODO
#endif   //end #if EP

      if ( _timeSys.time >= TIME_TICKS_PER_DAY )
      {  /* Next day */
         _timeSys.time = 0;
         _timeSys.date++;
      }
   }

   /* Tick power-up time */
   if (_ticTocCntr == 0) {
      _timePup.time+=SYS_TIME_TICK_IN_mS;
      if ( _timePup.time >= TIME_TICKS_PER_DAY )
      {  /* Next day */
         _timePup.time = 0;
         _timePup.date++;
      }
   }
}

/*****************************************************************************************************************
 *
 * Function name: getUnusedAlarm
 *
 * Purpose: This function checks and returns index to an unused alarm. If no unused alarm is available, it
 *          returns NO_ALARM_FOUND
 *
 * Arguments: None
 *
 * Returns: uint8_t alarmId - Index of unused alarm, if available. NO_ALARM_FOUND otherwise
 *
 * Side effects: None
 *
 * Reentrant: NO. This function should be called after taking the __timeVarsMutex mutex.
 *
 ******************************************************************************************************************/
STATIC uint8_t getUnusedAlarm( void )
{
   uint8_t alarmId = NO_ALARM_FOUND;   /* Index of empty slot */
   uint8_t index;                      /* Index */

   for ( index = 0; index < ARRAY_IDX_CNT(_sTimeSys); index++ )
   {  /* Find first empty slot */
      if ( !_sTimeSys[index].bTimerSlotInUse )
      {  /* alarm slot available */
         alarmId = index;
         break;
      }
   }
   return alarmId;
}

/*****************************************************************************************************************
 *
 * Function name: getNextCalAlarmDate
 *
 * Purpose: This function computes the next alarm date for daily alarms. This function should be called after
 *          adding alarm, Time change and triggering this alarm
 *
 * Arguments: uint8_t alarmId - Index of alarm
 *            bool timeChangeOrNewAlarm - true - Time changed or new alarm was added. false otherwise
 *
 * Returns: None
 *
 * Side effects: Updates the date in the alarm data-structure
 *
 * Reentrant: NO. This function should be called after taking the __timeVarsMutex mutex.
 *
 ******************************************************************************************************************/
STATIC void getNextCalAlarmDate ( uint8_t alarmId, bool timeChangeOrNewAlarm )
{
   tTimeSys *pTimeSys;     /* Pointer to alarm data structure */
   sysTime_t timeSys;

   if ( TIME_SYS_IsTimeValid() )
   {  /* Compute next date only if system time is valid */
      pTimeSys = &_sTimeSys[alarmId];

      getSysTime( &timeSys );

      if ( timeChangeOrNewAlarm )
      {  /* Called because of time change or new alarm is added */
         if ( timeSys.time > pTimeSys->ulAlarmTime_Period )
         {  /* Alarm time was earlier today, set the alarm for next day */
            pTimeSys->ulAlarmDate = timeSys.date + 1;
         }
         else
         {  /* Alarm time is now or later, set the alarm for today */
            pTimeSys->ulAlarmDate = timeSys.date;
         }
      }
      else
      {  /* No time change AND this is not a new alarm */
         if ( timeSys.time >= pTimeSys->ulAlarmTime_Period )
         {  /* Alarm time was earlier today (or just triggered), set the alarm for next day */
            pTimeSys->ulAlarmDate = timeSys.date + 1;
         }
         else
         {  /* Alarm time is later today, set the alarm for today */
            pTimeSys->ulAlarmDate = timeSys.date;
         }
      }
   }
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_AddCalAlarm
 *
 * Purpose: This function adds calendar alarm (one shot and daily).
 *
 * Arguments: tTimeSysCalAlarm *pData - Pointer to the parameters required for configuring calendar alarm
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Alarm added successfully, eFAILURE otherwise.
 *          The Alarm Id is returned in alarmId member of the tTimeSysCalAlarm structure
 *
 * Side effects: Add alarm to the alarm data-structure
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_AddCalAlarm( tTimeSysCalAlarm *pData )
{
   returnStatus_t retVal = eFAILURE;  /* Return value */
   uint8_t alarmId;                   /* alarm Index */
   tTimeSys *pTimeSys;                /* Pointer to alarm data structure */

   pData->ucAlarmId = NO_ALARM_FOUND; //Default, if no alarm available
   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails

   alarmId = getUnusedAlarm();
   if ( alarmId != NO_ALARM_FOUND )
   {  /* alarm slot available */
      pTimeSys = &_sTimeSys[alarmId];
      pTimeSys->bTimerSlotInUse    = (bool)true;
#if 0 // TODO: RA6E1 - Queue handle for FreeRTOS
      pTimeSys->pQueueHandle       = pData->pQueueHandle;
      pTimeSys->pMQueueHandle      = pData->pMQueueHandle;
#endif
      pTimeSys->ulAlarmDate        = pData->ulAlarmDate;
      pTimeSys->ulAlarmTime_Period = pData->ulAlarmTime;
      pTimeSys->alarmId            = alarmId;

      pTimeSys->bCalAlarm = (bool)true;
      if ( 0 == pData->ulAlarmDate )
      {  /* Daily Calender alarm */
         pTimeSys->bCalAlarmDaily = (bool)true;
         getNextCalAlarmDate(alarmId, (bool)true); // set next alarm date
      }
      pTimeSys->bSkipTimeChange = pData->bSkipTimeChange;
      pTimeSys->bUseLocalTime   = pData->bUseLocalTime;

      pData->ucAlarmId = alarmId;
      retVal = eSUCCESS;
   }
   OS_MUTEX_Unlock( &_timeVarsMutex ); // Function will not return if it fails

   return(retVal);  /*lint !e456 !e454 The mutex is handled properly. */
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetCalAlarm
 *
 * Purpose: This function get calendar alarm (one shot and daily) info.
 *
 * Arguments: tTimeSysCalAlarm *pData - Pointer to the parameters required for configuring calendar alarm
 *            uint8_t           alarmId - the alarm Id of the desired calendar alarm
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Alarm successfully retrieved, eFAILURE otherwise.
 *          If the Alarm Id is is not a calendar alarm, a failure is returned
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_GetCalAlarm( tTimeSysCalAlarm *pData, uint8_t alarmId )
{
   returnStatus_t  retVal = eFAILURE;  /* Return value */
   tTimeSys       *pTimeSys;           /* Pointer to alarm data structure */

   if ( alarmId < ARRAY_IDX_CNT(_sTimeSys) && pData != NULL )
   { /* ID within range */
      OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails

      pTimeSys = &_sTimeSys[alarmId];
      if ( pTimeSys->bCalAlarm )
      {
#if 0 // TODO: RA6E1 - Queue handle for FreeRTOS
         pData->pQueueHandle    = pTimeSys->pQueueHandle;
         pData->pMQueueHandle   = pTimeSys->pMQueueHandle;
#endif
         pData->ulAlarmDate     = pTimeSys->ulAlarmDate;
         pData->ulAlarmTime     = pTimeSys->ulAlarmTime_Period;
         pData->bSkipTimeChange = pTimeSys->bSkipTimeChange;
         pData->bUseLocalTime   = pTimeSys->bUseLocalTime;
         pData->ulAlarmDate     = pTimeSys->ulAlarmDate;
         pData->ucAlarmId       = pTimeSys->alarmId;
         if ( pTimeSys->bCalAlarmDaily )
         {
            pData->ulAlarmDate = 0;
         }
         retVal = eSUCCESS;
      }
      OS_MUTEX_Unlock( &_timeVarsMutex );  /* End critical section */ // Function will not return if it fails
   }  /*lint !e456 !e454 The mutex is handled properly. */
   return(retVal);  /*lint !e456 !e454 The mutex is handled properly. */
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetPeriodicAlarm
 *
 * Purpose: This function get calendar alarm (one shot and daily) info.
 *
 * Arguments: tTimeSysCalAlarm *pData - Pointer to the parameters required for configuring periodic alarm
 *            uint8_t           alarmId - the alarm Id of the desired periodic alarm
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Alarm successfully retrieved, eFAILURE otherwise.
 *          If the Alarm Id is is not a periodic alarm, a failure is returned
 *
 * Side effects: None
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_GetPeriodicAlarm( tTimeSysPerAlarm *pData, uint8_t alarmId )
{
   returnStatus_t  retVal = eFAILURE;  /* Return value */
   tTimeSys       *pTimeSys;           /* Pointer to alarm data structure */

   if ( alarmId < ARRAY_IDX_CNT(_sTimeSys) && pData != NULL )
   { /* ID within range */
      OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails

      pTimeSys = &_sTimeSys[alarmId];
      /*not Cal alram must be periodic*/
      if ( !(pTimeSys->bCalAlarm) )
      {
#if 0 // TODO: RA6E1 - Queue handle for FreeRTOS
         pData->pQueueHandle    = pTimeSys->pQueueHandle;
         pData->pMQueueHandle   = pTimeSys->pMQueueHandle;
#endif
         pData->bSkipTimeChange = pTimeSys->bSkipTimeChange;
         pData->bUseLocalTime   = pTimeSys->bUseLocalTime;
         pData->ucAlarmId       = pTimeSys->alarmId;
         pData->bOnValidTime    = pTimeSys->bOnValidTime;
         pData->bOnInvalidTime  = pTimeSys->bOnInvalidTime;
         pData->ulOffset        = pTimeSys->ulOffset;
         pData->ulPeriod        = pTimeSys->ulAlarmTime_Period;
         retVal = eSUCCESS;
      }
      OS_MUTEX_Unlock( &_timeVarsMutex );  /* End critical section */ // Function will not return if it fails
   }  /*lint !e456 !e454 The mutex is handled properly. */
   return(retVal);  /*lint !e456 !e454 The mutex is handled properly. */
}
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_AddPerAlarm
 *
 * Purpose: This function adds periodic alarm
 *
 * Arguments: tTimeSysCalAlarm *pData - Pointer to the parameters required for configuring periodic alarm
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Alarm added successfully, eFAILURE otherwise.
 *          The Alarm Id is returned in alarmId member of the tTimeSysPerAlarm structure
 *
 * Side effects: Add alarm to the alarm data-structure
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_AddPerAlarm( tTimeSysPerAlarm *pData )
{
   returnStatus_t retVal = eFAILURE;   /* Return value */
   uint8_t alarmId;           /* Alarm Index */
   tTimeSys *pTimeSys;        /* Pointer to alarm data structure */

   pData->ucAlarmId = NO_ALARM_FOUND; //Default, if no alarm available
#if 0 // TODO: RA6E1 - Queue handle for FreeRTOS
   if ( (0 == pData->ulPeriod) ||
        (pData->ulOffset >= pData->ulPeriod) ||
        (pData->bOnInvalidTime && !pData->bOnValidTime && !pData->bSkipTimeChange) ||
        (NULL == pData->pQueueHandle && NULL == pData->pMQueueHandle) )
#else
   if ( (0 == pData->ulPeriod) ||
        (pData->ulOffset >= pData->ulPeriod) ||
        (pData->bOnInvalidTime && !pData->bOnValidTime && !pData->bSkipTimeChange) )
#endif
   {
      /* Periodic alarms with 0 period OR
         Offset can not be greater than or equala to period.
         only triggering on Invalid time and requesting action on time change are not permitted OR
         Both the handles are NULL */
      return(retVal);
   }

   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails

   alarmId = getUnusedAlarm();
   if ( alarmId != NO_ALARM_FOUND )
   {  /* Alarm slot available */
      pTimeSys = &_sTimeSys[alarmId];
      pTimeSys->bTimerSlotInUse     = (bool)true;
#if 0 // TODO: RA6E1 - Queue handle for FreeRTOS
      pTimeSys->pQueueHandle        = pData->pQueueHandle;
      pTimeSys->pMQueueHandle       = pData->pMQueueHandle;
#endif
      pTimeSys->ulAlarmTime_Period  = pData->ulPeriod;
      pTimeSys->ulOffset            = pData->ulOffset % pData->ulPeriod;
      pTimeSys->alarmId             = alarmId;
      pTimeSys->bSkipTimeChange     = pData->bSkipTimeChange;
      pTimeSys->bUseLocalTime       = pData->bUseLocalTime;
      pTimeSys->bOnValidTime        = pData->bOnValidTime;
      pTimeSys->bOnInvalidTime      = pData->bOnInvalidTime;

      pData->ucAlarmId = alarmId;
      retVal = eSUCCESS;
   }
   OS_MUTEX_Unlock( &_timeVarsMutex );  /* End critical section */ // Function will not return if it fails

   return(retVal);   /*lint !e456 !e454 The mutex is handled properly. */
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_DeleteAlarm
 *
 * Purpose: This function delete alarm
 *
 * Arguments: uint8_t alarmId - Id of the alarm to delete
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Alarm deleted successfully, eFAILURE otherwise.
 *
 * Side effects: Alarm deleted from the alarm data-structure
 *
 * Reentrant: Yes
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_DeleteAlarm ( uint8_t alarmId )
{
   returnStatus_t retVal = eFAILURE;   /* Return value */

   if ( alarmId < ARRAY_IDX_CNT(_sTimeSys) )
   { /* ID within range */
      OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails
      (void)memset(&_sTimeSys[alarmId], 0, sizeof(tTimeSys));
      OS_MUTEX_Unlock( &_timeVarsMutex );  /* End critical section */ // Function will not return if it fails
      retVal = eSUCCESS;
   }  /*lint !e456 !e454 The mutex is handled properly. */
   return(retVal); /*lint !e456 !e454 The mutex is handled properly. */
}

/*****************************************************************************************************************
 *
 * Function name: isTimeForPeriodicAlarm
 *
 * Purpose: This function checks if is it time for periodic alarm based on system/pup time
 *
 * Arguments: uint8_t alarmId - Id of the periodic alarm
 *
 * Returns: bool retVal - true - It is time, false otherwise.
 *
 * Side effects: None
 *
 * Reentrant: NO. This function should be called after taking the __timeVarsMutex mutex.
 *
 ******************************************************************************************************************/
STATIC bool isTimeForPeriodicAlarm ( uint8_t alarmId )
{
   tTimeSys *pTimeSys;        /* Pointer to alarm data structure */
   bool retVal = (bool)false; /* Return value, true - Time for alarm, false otherwise */
   int32_t offset;            /* Include local shift time, UTC offset and DST offset */
   sysTime_t timeSys;

   pTimeSys = &_sTimeSys[alarmId];

   if ( pTimeSys->bOnValidTime && TIME_SYS_IsTimeValid() )
   {  /* Operate on valid time and system time is valid */
      offset = TIME_TICKS_PER_DAY;  // Add 1 day to keep ticks positive
#if (EP == 1)
      offset += ( (signed)pTimeSys->bUseLocalTime * DST_GetLocalOffset() ); // Add local and DST offset
#endif
      getSysTime( &timeSys );
      if ( pTimeSys->ulOffset == ((timeSys.time + (uint32_t)offset) % pTimeSys->ulAlarmTime_Period) )
      {  /* Alarm */
         retVal = (bool)true;
      }
   }
   else if ( pTimeSys->bOnInvalidTime )
   {  /* Operate on Invalid Time */
      if ( 0 == (_timePup.time % pTimeSys->ulAlarmTime_Period) )
      {  /* Alarm */
         retVal = (bool)true;
      }
   }
   return retVal;
}

/*****************************************************************************************************************
 *
 * Function name: executeAlarm
 *
 * Purpose: This function is called when it is time for the alarm. Send message to the appropriate queue, if requested
 *
 * Arguments: uint8_t alarmId - Id of the alarm
 *
 * Returns: returnStatus_t retVal - eFAILURE, if message is requested and could not send the msg, eSUCCESS otherwise
 *
 * Side effects: None
 *
 * Reentrant: NO. This function should be called after taking the __timeVarsMutex mutex.
 *
 ******************************************************************************************************************/
STATIC returnStatus_t executeAlarm ( uint8_t alarmId )
{
   returnStatus_t retVal = eSUCCESS;   /* Return value, eFAILURE - Error sending message to queue */
   tTimeSys      *pTimeSys;            /* Pointer to alarm data structure */
   tTimeSysMsg    alarmMsg;            /* Alarm Message */
   sysTime_t      timeSys;

   pTimeSys    = &_sTimeSys[alarmId];  // Point to alarm structure

#if 0 // TODO: RA6E1 - buf functionality and queue handle
   // send message only if queue or mailbox handle is not NULL
   if ( (pTimeSys->pQueueHandle != NULL) || (pTimeSys->pMQueueHandle != NULL) )
   {  /* Send power-up time, if any of the following conditions are true
         1. System time is invalid except the alarm is a periodic alarm and will trigger on valid time only
         2. The alarm is a periodic alarm and will trigger on power-up time only */
      if ( (TIME_SYS_IsTimeValid() && !(pTimeSys->bOnValidTime && !pTimeSys->bOnInvalidTime)) ||
           (!pTimeSys->bCalAlarm && !pTimeSys->bOnValidTime && pTimeSys->bOnInvalidTime) )
      {
         alarmMsg.bSystemTime  = (bool)false;
         alarmMsg.date         = _timePup.date;
         alarmMsg.time         = _timePup.time;
      }
      else
      {
         alarmMsg.bSystemTime  = (bool)true;
         getSysTime( &timeSys );
         alarmMsg.date         = timeSys.date;
         alarmMsg.time         = timeSys.time;
      }
      alarmMsg.bIsSystemTimeValid    = TIME_SYS_IsTimeValid();
      alarmMsg.alarmId               = (uint8_t)pTimeSys->alarmId;
      alarmMsg.bTimeChangeForward    = pTimeSys->bTimeChangeForward;
      alarmMsg.bTimeChangeBackward   = pTimeSys->bTimeChangeBackward;
      alarmMsg.bCauseOfAlarm         = pTimeSys->bCauseOfAlarm;
      alarmMsg.timeChangeDelta       = pTimeSys->timeChangeDelta;

      buffer_t *pBuf = BM_alloc(sizeof(alarmMsg)); //Get a buffer
      if (NULL != pBuf) // Is the buffer valid?
      {
         pBuf->eSysFmt = eSYSFMT_TIME;
         (void)memcpy(&pBuf->data[0], &alarmMsg, sizeof(alarmMsg));

         if ( pTimeSys->pQueueHandle != NULL )
         {
            OS_QUEUE_Enqueue(pTimeSys->pQueueHandle, pBuf); // Function will not return if it fails
         }
         else if ( pTimeSys->pMQueueHandle != NULL )
         {
            OS_MSGQ_Post(pTimeSys->pMQueueHandle, pBuf); // Function will not return if it fails
         }
         //else can free the buffer. Not need as we only enter this block if one of the queue handle is not NULL

         /* Task notified, clear status flags */
         pTimeSys->bTimeChangeForward  = (bool)false;
         pTimeSys->bTimeChangeBackward = (bool)false;
         pTimeSys->timeChangeDelta     = 0;
         pTimeSys->bRetryAlarm         = (bool)false;
      }
      else
      {  //No buffer available, at this time try next tick
         retVal                        = eFAILURE;
         pTimeSys->bRetryAlarm         = (bool)true;
      }
   }
#endif

   return retVal;
}

/*****************************************************************************************************************
 *
 * Function name: processSysTick
 *
 * Purpose: Increment system and power-up time. Check all alarms, if time, send alarm message to the requested queue.
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side effects: Increment system and power-up time
 *
 * Reentrant: Yes. This function should only be called to process system tick.
 *
 ******************************************************************************************************************/
STATIC void processSysTick( void )
{
   uint8_t index;            /* Index to alarm data-structure */
   tTimeSys *pTimeSys;       /* Pointer to alarm data structure */

   OS_MUTEX_Lock( &_timeVarsMutex ); // Function will not return if it fails

#if (EP == 1)
   sysTime_t timeSys;

   getSysTime( &timeSys );

   /* Handle DST/Offset computation. This should be handled as a special case, before handling other alarms */
   if ( _nuDST_TimeChanged || /* Time changed */
        ( TIME_SYS_IsTimeValid() && DST_IsEnable() &&
          ( ( (_sDST_CalAlarm.date == timeSys.date) && (timeSys.time >= _sDST_CalAlarm.time) ) ||
            (timeSys.date > _sDST_CalAlarm.date) ) ) )
   {
      /* Either time changed i.e. Time jumped, time became valid or time became invalid
         OR Time for DST ON/OFF */
      DST_ComputeDSTParams(TIME_SYS_IsTimeValid(), timeSys.date, timeSys.time);
      _nuDST_TimeChanged = (bool)false;
   }
#endif

   for ( index = 0, pTimeSys = &_sTimeSys[0]; index < ARRAY_IDX_CNT(_sTimeSys); index++, pTimeSys++ )
   {  /* Check every alarm Slot */
      if ( pTimeSys->bTimerSlotInUse )
      {  /* Alarm active in this slot */

         if ( !pTimeSys->bCalAlarm )
         {  /* Periodic Alarm */

            if ( pTimeSys->bRetryAlarm )
            {  /* Queue send failed, try to resend message.
                  The original cause of the alarm will be sent in the message */
               (void)executeAlarm(index); //ignore the return value, no bookkeeping required for periodic alarm
            }
            else if ( isTimeForPeriodicAlarm(index) )
            {
               pTimeSys->bCauseOfAlarm = (bool)true;
               (void)executeAlarm(index); //ignore the return value, no bookkeeping required for periodic alarm
            }
            else if ( pTimeSys->bTimeChanged )
            {  /* Execute alarm on time change only */
               pTimeSys->bCauseOfAlarm = (bool)false;
               (void)executeAlarm(index); //ignore the return value, no bookkeeping required for periodic alarm
            }
            pTimeSys->bTimeChanged = (bool)false;  /* Time change processed */
         }
         else if ( TIME_SYS_IsTimeValid() && (!pTimeSys->bExpired) )
         {  /* Calendar Alarms, System time valid and alarm is not expired */
            /* If queue send fails, new cause of alarm will be used, if any. This will insure that the cause of
               alarm will be sent in appropriate order */
            sysTime_t calTimeSys;
            getSysTime( &calTimeSys );
#if (EP == 1)  /* Local time adjustment when requested by alarm */
            if (pTimeSys->bUseLocalTime)
            {
               DST_ConvertUTCtoLocal(&calTimeSys);
            }
#endif
            if ( ((pTimeSys->ulAlarmDate == calTimeSys.date) && (calTimeSys.time >= pTimeSys->ulAlarmTime_Period)) ||
                 (calTimeSys.date > pTimeSys->ulAlarmDate) )
            {  /* Execute alarm, system time equal or past alarm time */
               pTimeSys->bCauseOfAlarm = (bool)true;

               if ( eSUCCESS == executeAlarm(index) )
               {
                  pTimeSys->bTimeChanged = (bool)false; /* Time change processed */
                  if ( pTimeSys->bCalAlarmDaily )
                  {  /* Compute the next alarm date for daily alarm */
                     getNextCalAlarmDate(index, (bool)false); // set next alarm date
                  }
                  else
                  {  /* One shot alarm, mark it expired */
                     pTimeSys->bExpired = (bool)true;
                  }
               }
            }
            else if ( pTimeSys->bTimeChanged )
            {  /* Execute alarm, time change only  */
               pTimeSys->bCauseOfAlarm = (bool)false;

               if ( eSUCCESS == executeAlarm(index) )
               {
                  pTimeSys->bTimeChanged = (bool)false; /* Time change processed */
                  if ( pTimeSys->bCalAlarmDaily )
                  {  /* Compute the next alarm date for daily alarm */
                     getNextCalAlarmDate(index, (bool)false); // set next alarm date
                  }
               }
            }
         } /* End Calendar Alarms */
      } /* End alarm active in this slot */
   } /* end For */

   OS_MUTEX_Unlock( &_timeVarsMutex );  /* End critical section */ // Function will not return if it fails

}  /*lint !e456 !e454 The mutex is handled properly. */

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_GetRealCpuFreq()
 *
 * Purpose: Return CPU freq as computed against the GPS 1PPS or TCXO
 *
 * Arguments: cpuFreq - The real CPU frequency
 *            source  - The source of the frequency computation
 *            seconds - If not NULL, return the date of last update in seconds since 1970 and validate the current frequency
 *
 * Returns: TRUE if the frequency has been updated less than a minute ago.
 *
 *
 ******************************************************************************************************************/
bool TIME_SYS_GetRealCpuFreq( uint32_t *freq, TIME_SYS_SOURCE_e *source, uint32_t *seconds )
{
   bool retVal = (bool)false;
   uint32_t currenTime;
   uint32_t lastUpdate;

   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
   *freq      = clockInfo_.freq; // Grab best frequency estimate
   lastUpdate = clockInfo_.freqLastUpdate;
   if ( source != NULL ) {
      *source = clockInfo_.source;
   }
   __set_PRIMASK(primask); // Restore interrupts

   // CPU frequency is considered valid up to 1 minute after the last update
   // After that it is considered stale.
   // Some critical computation can't risk using a stale value so it needs to know.
   currenTime = TIME_UTIL_GetTimeInQSecFracFormat() >> 32; // Drop fractional part
   if ( (currenTime - lastUpdate) < 60 ) {
      retVal = (bool)true;
   }
   if ( seconds != NULL ) {
      *seconds = lastUpdate;
   }

   return ( retVal );
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetRealCpuFreq()
 *
 * Purpose: Set CPU freq as computed against the GPS 1PPS or TCXO
 *
 * Arguments: freq   - Real CPU frequency
 *            source - Source of the computation
 *            reset  - reset last update
 *
 * Returns: none
 *
 *
 ******************************************************************************************************************/
void TIME_SYS_SetRealCpuFreq( uint32_t freq, TIME_SYS_SOURCE_e source, bool reset )
{
   // Update timestamp
   if ( reset ) {
      clockInfo_.freqLastUpdate = 0;
   } else {
      clockInfo_.freqLastUpdate = TIME_UTIL_GetTimeInQSecFracFormat() >> 32; // Drop fractional part
   }

   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // This is critical but fast. Disable all interrupts.
   clockInfo_.source = source;
   clockInfo_.freq = freq;
   __set_PRIMASK(primask); // Restore interrupts
}

#if ( DCU == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_IsGpsPresent()
 *
 * Purpose: Return GPS present/absent state. DCU2+
 *
 * Arguments: None
 *
 * Returns: True if GPS present
 *
 *
 ******************************************************************************************************************/
bool TIME_SYS_IsGpsPresent( void ) {
   return ( clockInfo_.WatchDogCounter ? true : false );
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_IsGpsTimeValid()
 *
 * Purpose: Return GPS UTC time state. DCU2+
 *
 * Arguments: None
 *
 * Returns: True if GPS time is valid. That means we have started to converge to proper time.
 *
 *
 ******************************************************************************************************************/
bool TIME_SYS_IsGpsTimeValid( void ) {
   // if gotGPStime is true this means we got UTC time from main board AFTER we started to track UTC.
   return ( clockInfo_.gotGpsTime );
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_IsGpsPhaseLocked()
 *
 * Purpose: Return GPS Phase Locked State. DCU2+
 *
 * Arguments: None
 *
 * Returns: True if GPS is Phase Locked
 *
 ******************************************************************************************************************/
bool TIME_SYS_IsGpsPhaseLocked( void ) {
   return ( clockInfo_.phaseLocked );
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_PhaseErrorUsec()
 *
 * Purpose: Return the current phase error between the GPS and system tick in microseconds. DCU2+
 *
 * Arguments: None
 *
 * Returns: Phase error in usec
 *
 *
 ******************************************************************************************************************/
int32_t TIME_SYS_PhaseErrorUsec( void ) {
   double error;
   uint32_t CPUfreq;

   // Convert the current phase error from ticks per 5 seconds to tick/sec
   error = clockInfo_.TickToGPSerror/5.0;

   // Convert the error from tick to seconds
   (void)TIME_SYS_GetRealCpuFreq( &CPUfreq, NULL, NULL );
   error /= CPUfreq;

   // Convert error from seconds to usec
   error *= 1000000;

   return ( (int32_t)error );
}

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_PhaseErrorNsec()
 *
 * Purpose: Return the current phase error between the GPS and system tick in nanoseconds. DCU2+
 *
 * Arguments: None
 *
 * Returns: Phase error in nsec
 *
 *
 ******************************************************************************************************************/
int32_t TIME_SYS_PhaseErrorNsec( void ) {
   double error;
   uint32_t CPUfreq;

   // Convert the current phase error from ticks per 5 seconds to tick/sec
   error = clockInfo_.TickToGPSerror/5.0;

   // Convert the error from tick to seconds
   (void)TIME_SYS_GetRealCpuFreq( &CPUfreq, NULL, NULL );
   error /= CPUfreq;

   // Convert error from seconds to usec
   error *= 1000000000;

   return ( (int32_t)error );
}

/*****************************************************************************************************************
 *
 * Function name: ResetGPSStats()
 *
 * Purpose: Reset GPS statistics effectively restarting the frequency computation. DCU2+
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Note: The CPU freq is not reset here because we want to keep whatever value might have been computed until we lost the GPS
 *
 ******************************************************************************************************************/
static void ResetGPSStats( void )
{
   clockInfo_.FTMcount    = getBusClock()*10; // How many FTM counts are expected in 10 sec
   clockInfo_.gotGpsTime  = false;
   clockInfo_.phaseLocked = false;
   clockInfo_.validFreq   = false;
//   DBG_LW_printf( "\nReset gotGPSTime\n" );
}

/*****************************************************************************************************************
 *
 * Function name: GPS_PPS_IRQ_ISR
 *
 * Purpose: Compute the real frequency using the PPS interrupts from the GPS and adjust the sys tick accordingly.
 *          The interrupt is expected to happen every 5 sec.
 *          DCU2+
 *
 * Arguments: None
 *
 * Returns: None
 *
 *
 ******************************************************************************************************************/
static void GPS_PPS_IRQ_ISR( void )
{
          uint32_t i;
   static uint32_t cycCnt = 0;          // This counter increments by one every bus (CPU) cycle
   static uint32_t cycCnt5sec = 0,cycCnt10sec;
          uint16_t currentFTM = (uint16_t)FTM0_CNT; // Current FTM0 counter value
          uint32_t rollover;            // How many time FTM counter rolled over
          uint32_t FTMcount;            // How many FTM count happened and 10 sec
   static uint32_t prevFTMcount = 0;    // Previous FTM count
   static uint16_t currentValue = 0;    // Current FTM captured value
   static uint16_t tenSecAgoValue = 0;  // Captured value 5 seconds ago
   static uint16_t fiveSecAgoValue = 0; // Captured value 10 seconds ago
          int32_t  left,right,error;    // Left side error, right side error
   static uint32_t freqFilter[FREQUENCY_FILTER_LEN] = {0xFFFFFFFF}; /*lint !e785*/ // Initialize with bad values
   static uint32_t freqIndex=0;
          uint32_t largestDiff;
          uint32_t min = 0xFFFFFFFF; // Initialize with largest value
          uint32_t max = 0;          // Initialize with smalest value
          uint32_t CoreToBusClockRatio;

   // Shift all values by 5 seconds
   cycCnt10sec     = cycCnt5sec;
   cycCnt5sec      = cycCnt;
   cycCnt          = DWT_CYCCNT;
   tenSecAgoValue  = fiveSecAgoValue;
   fiveSecAgoValue = currentValue;
   currentValue    = (uint16_t)FTM0_C2V; // Captured value

   CoreToBusClockRatio = getCoreClock()/getBusClock();

   // Adjust cycCnt to compensate for time it took to get from the GPS interrupt to here (latency)
   // Multiply by CoreToBusClockRatio to convert from FTM time (60MHz) to CPU time (120MHz or 180MHz)
   cycCnt -= CoreToBusClockRatio*((int32_t)(int16_t)(currentFTM-currentValue));

   // Check if first time we get PPS interrupt or first time after we lost GPS for a while
   if ( clockInfo_.WatchDogCounter == 0 ) {
      ResetGPSStats();
//      DBG_LW_printf( "Reset gotGPSTime\n" );
   }

   // We need to boostrap clockInfo_.FTMcount
   // At first it will be computed from DWT_CYCCNT but in time, we can refine the computation using FTM module
   if ( clockInfo_.FTMcount == (getBusClock()*10) ) {
      // Use less precise method to bootstrap computation
      FTMcount = (cycCnt-cycCnt10sec)/CoreToBusClockRatio;
   } else {
      // Estimate how many times the FTM counter rolled over during 10 sec
      rollover = (uint32_t)(((((clockInfo_.FTMcount)-(65536-tenSecAgoValue))-currentValue)/65536.0f)+0.5f);

      // How many FTM count happenend in 10 sec
      FTMcount = 65536*rollover+(65536-tenSecAgoValue)+currentValue;
   }

   // Crude sanity check
   // The CPU frequency should be 120MHz +/-50ppm for 9975T and 180MHz +/- 150ppm for 9985T so take worst
   // Real CPU frequency as estimated in 10 sec.
   if ( (((FTMcount/10)*CoreToBusClockRatio) > (uint32_t)(getCoreClock()*1.00015)) || (((FTMcount/10)*CoreToBusClockRatio) < (uint32_t)(getCoreClock()*0.99985)) ) {
      // Something is wrong. Reset computation and start again.
      ResetGPSStats();
      return;
   }

   // Computation is different once we are phase locked because some variables converged and we can use that fact to help stay locked and avoid computational errors.
   if ( clockInfo_.phaseLocked ) {
      // Make sure FTM didn't jump
      // This happens sometimes and jumps should be short lived and ignored
      if ( abs( (int32_t)clockInfo_.FTMcount - (int32_t)FTMcount ) > 100 ) {
//         DBG_LW_printf( "FTM jump %u %u\n", clockInfo_.FTMcount, FTMcount );
         return;
      }
   } else {
      // Make sure FTMcount converged before using that value
      // Convergence happened when 2 values in a row are close together
      if ( abs( (int32_t)prevFTMcount - (int32_t)FTMcount ) > 100 ) {
//         DBG_LW_printf( "Bad FTMcount %u %u\n", prevFTMcount, FTMcount );
         prevFTMcount = FTMcount;
         return;
      }
   }

   prevFTMcount = FTMcount; // Save current value for later

   // Compute the phase error
   // We are trying to find out if it's faster to reduce the error from the left (i.e. which means the fiveSecTickTime happened just before (GPS interrupt - the bias))
   // or if it's it's faster to reduce the error from the right (i.e. which means the fiveSecTickTime happened just after (GPS interrupt - the bias))
   // The fastest way will be the smallest distance (i.e smallest of abs(left) and abs(right))
   left = (int32_t)((cycCnt - (uint32_t)(((((float)FTMcount * CLOCK_BIAS) / 10.0f) * CoreToBusClockRatio ) + 0.5f)) - clockInfo_.fiveSecTickTime);
   if ( left >= 0 ) {
      right = left - (int32_t)FTMcount;
   } else {
      right = (int32_t)FTMcount + left;
   }

   // Find direction of smallest error
   if ( abs(left) < abs(right) ) {
      error = left;
      clockInfo_.RawTickToGPSerror = left;
   } else {
      error = right;
      clockInfo_.RawTickToGPSerror = right;
   }

   // Check if we can go into phase locked mode or if we should get out
   if ( (clockInfo_.phaseLocked == false) && (abs(error) < PHASE_LOCKED_THRESHOLD) ) {
      clockInfo_.phaseLocked = true; // Phase locked to GPS interrupt
   } else if ( (clockInfo_.phaseLocked == true) && (abs(error) > (5*PHASE_LOCKED_THRESHOLD)) ) {
      clockInfo_.phaseLocked = false; // No longer phase locked to GPS interrupt
   }

   // Low pass the error when we are phase locked.
   // Sometimes the error can sudently jumps for short time and we want to reduce the impact of such events
   if ( clockInfo_.phaseLocked == true ) {
      clockInfo_.TickToGPSerror = (float)error*ERROR_ALPHA + clockInfo_.TickToGPSerror*(1.0f-ERROR_ALPHA);
   } else {
      clockInfo_.TickToGPSerror = (float)error;
   }

   // Always update the frequency filter
   freqFilter[freqIndex++] = FTMcount*CoreToBusClockRatio/10; // Real CPU frequency as estimated in 10 sec
   freqIndex %= FREQUENCY_FILTER_LEN;

   // Find largest distance between all frequencies
   for (i=0; i<FREQUENCY_FILTER_LEN; i++)
   {
      if ( min > freqFilter[i] ) {
         min = freqFilter[i];
      }
      if ( max < freqFilter[i] ) {
         max = freqFilter[i];
      }
   }
   largestDiff = max - min;
#if 0
   DBG_LW_printf("%u %u %u %u %u %i\n",
               freqFilter[0],
               freqFilter[1],
               freqFilter[2],
               freqFilter[3],
               freqFilter[4],
               largestDiff);
#endif
   // Filter the frequencies
   // They should be easily within 10Hz of each other
   if ( largestDiff < 10 ) {
      // We have a valid frequency
      clockInfo_.FTMcount      = FTMcount;                                                                               // How many FTM counts between 3 GPS interrups (i.e. 10 seconds)
      TIME_SYS_SetRealCpuFreq( (uint32_t)((uint64_t)FTMcount*CoreToBusClockRatio/10), eTIME_SYS_SOURCE_GPS, (bool)false);// Real CPU frequency as estimated in 10 sec
      clockInfo_.validFreq     = true;                                                                                   // Valid frequency was computed
      clockInfo_.sendCommand   = true;                                                                                   // Have the error corrected
//      DBG_LW_printf("Freq valid %u\n", (uint32_t)clockInfo_.freq);
   }
   clockInfo_.WatchDogCounter = GPS_WD_RESET; // Reset watch dog

   // Sanity check
//   if (abs(clockInfo_.TickToGPSerror) > 1000000 ) {
   // Warning: printing will mess the timing.
//      DBG_LW_printf( "cycCnt %u rollover %u FTMCount %u fiveSecTickTime %u left %d right %d GPSerror %d\n", cycCnt, rollover, FTMcount, clockInfo_.fiveSecTickTime, left, right, clockInfo_.TickToGPSerror);
//   }

   // Warning: printing will mess the timing.
//   DBG_LW_printf( "rollover %u FTMcount %u Freq %u 50Mhz %u\n", rollover, FTMcount, (uint32_t)(((uint64_t)FTMcount)*2), (uint32_t)(((uint64_t)FTMcount)*10/12));
}
#endif

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_HandlerTask
 *
 * Purpose: This function pends on semaphore and wakes up every half cycle. When time for the alarm, this function will
 *          send message to the appropriate queue
 *
 * Arguments: void * - Value passed as part of task creation. Not used for this task.
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: No. This function implements alarm task. This function should not be called by any module except during
 *            startup (task creation).
 *
 ******************************************************************************************************************/
void TIME_SYS_HandlerTask( taskParameter )
{
#if ( RTOS_SELECTION == MQX_RTOS ) // TODO: RA6E1 - ISR control for FreeRTOS
   MY_ISR_STRUCT_PTR_MQX  isr_ptr;    /* */
#endif
   uint16_t     cnt;        /* Number of semaphores before the tick is processed. */
#if ( DCU == 1 )
   uint32_t     syst_rvr;
   int32_t      command; // How much the sys tick clock will be changed by. the word command comes from control system (e.g. PID)
   CLOCK_INFO_t clockInfoCopy;
   uint32_t     error = 0;
#else
   uint16_t     loopCnt = 0;       //Counts between RTC samples
   uint32_t     sec;               //To get seconds from RTC
   uint32_t     usec;              //To get micro-seconds from RTC
//TODO: Used when RTC crystal is source for Sys Tick adjustment.  Put back in when using TCXO.
   uint64_t     currTime;          //Current RTC time
   uint64_t     lastTime;          //Last RTC time
   int64_t      diffRTC;           //Difference between last and current time(RTC)
   int64_t      accDiffRTC = 0;    //Difference between last and current time(RTC), accumulated
   int64_t      lastAccDiffRTC = 0;//Difference between last and current time(RTC), accumulated
//End TODO
   sysTime_t    timeSys;
//TODO: Not used when RTC crystal is source for Sys Tick adjustment.  Put back in when using TCXO.
//   uint32_t     TCXOToSystickErrorCntr = 0;
//   int32_t      error;
//End TODO
#endif
   /* The following code was taken from MQX example code for adding our timer code to the RTOS tick interrupt.  So, the
      following code will replace the RTOS tick interrupt with our own and then our ISR will call the RTOS tick. */
#if ( RTOS_SELECTION == MQX_RTOS ) // TODO: RA6E1 - ISR control for FreeRTOS
   isr_ptr               = (MY_ISR_STRUCT_PTR_MQX)_mem_alloc_zero(sizeof(MY_ISR_STRUCT_MQX));
   isr_ptr->OLD_ISR_DATA = _int_get_isr_data(INT_SysTick);                 /*lint !e641 */
   isr_ptr->OLD_ISR      = _int_get_isr(INT_SysTick);                      /*lint !e641 */
   _int_install_isr(INT_SysTick, TIME_SYS_vApplicationTickHook, isr_ptr);  /*lint !e641 !e64 !e534 */
#elif ( RTOS_SELECTION == FREE_RTOS )
   ( void ) SysTick_Config( SystemCoreClock / configTICK_RATE_HZ );      /* Configure SysTick to generate an interrupt every 5ms - TODO: This can be modified to 10ms */
#endif
   cnt = (uint16_t)(SYS_TIME_TICK_IN_mS / portTICK_RATE_MS);

#if ( DCU == 1 )
   if (VER_getDCUVersion() != eDCU2) {
      // Configure  GPS PPS GPIO as FTM0_CH2 input.
      GPS_PPS_TRIS();

      // Configure FTM0_CH2 to capture timer when GPS PPS signal is detected.
      (void)FTM0_Channel_Enable( 2, FTM_CnSC_CHIE_MASK | FTM_CnSC_ELSA_MASK, GPS_PPS_IRQ_ISR );
   }
#endif
#if (EP == 1)
   getSysTime( &timeSys );
   DST_ComputeDSTParams(TIME_SYS_IsTimeValid(), timeSys.date, timeSys.time); // Compute the DST dates on power-up
   RTC_GetTimeInSecMicroSec( &sec, &usec); // Get RTC time and store the current time
//TODO: Used when RTC crystal is source for Sys Tick adjustment.  Remove when using TCXO.
   lastTime = ((uint64_t)sec * 1000000) + usec;
//End TODO
#endif

   for ( ; ; ) /* RTOS Task, keep running forever */
   {
      /* Wait for the semaphore. If running off power line, this semaphore will be posted every half cycle otherwise use
         timeout functionality to run system time.  For Aclara-RF, this semaphore will be posted at twice the rate of
         system clock */
      (void)OS_SEM_Pend( &_timeSysSem, OS_WAIT_FOREVER );
#if ( DCU == 1 )
      if (VER_getDCUVersion() != eDCU2) {
         // Copy current clock state
         uint32_t primask = __get_PRIMASK();
         __disable_interrupt(); // This is critical but fast. Disable all interrupts.
         (void)memcpy( &clockInfoCopy, &clockInfo_, sizeof(clockInfoCopy));
         __set_PRIMASK(primask); // Restore interrupts

         // GPS watchdog
         if (clockInfo_.WatchDogCounter) { // Decrement GPS watch dog until 0 which means we have stopped receiving PPS interrupts
           clockInfo_.WatchDogCounter--;
         }

         // Adjust systick if we have a valid CPU frequency as computed by using the GPS PPS signal
         if ( clockInfoCopy.validFreq ) {
            // Adjust system tick reload counter and compensate for any accumulated system tick error
            // We dither the reload value to help eliminate any errors caused by the fact that clockInfoCopy.freq/BSP_ALARM_FREQUENCY is not an even number
            uint32_t rate = (BSP_ALARM_FREQUENCY*5); // 10 sec
            syst_rvr = ((uint32_t)(clockInfoCopy.freq*5/rate)) - 1;
            error += ((uint32_t)(clockInfoCopy.freq*5)) % rate; // Accumulate error

            // Remove some of the accumulated error if possible
            if ( error >= rate ){
               error -= rate;
               syst_rvr += 1;
            }

            command = 0; // Don't change nominal clock speed by default

            // Check if we need to speed up/slow down clock to remove any phase offset between systick and GPS
            if (clockInfoCopy.sendCommand == true) {
               command = (int32_t)((float32_t)clockInfoCopy.TickToGPSerror*0.4f);

               // Prevent large changes when we are trying to remove the error fast
               if ( command > (int32_t)(syst_rvr*1.1) ) {
                  command = (int32_t)(syst_rvr*1.1);
                  clockInfo_.TickToGPSerror -= command;
               } else if ( command < (int32_t)(syst_rvr*-0.1) ) {
                  command = (int32_t)(syst_rvr*-0.1);
                  clockInfo_.TickToGPSerror -= command;
               } else {
                  // The error is almost completely remove so we slow down convergence
                  clockInfo_.sendCommand = false; // Prevent being called again. Note that we update the clockInfo_ structure, not the copy.
               }
               //INFO_printf( "CPU Freq %u Command %d TickToGPSerror %d %d FTM %u", clockInfoCopy.FTMcount*2, command, (int32_t)(clockInfoCopy.TickToGPSerror*100), clockInfo_.RawTickToGPSerror, clockInfoCopy.FTMcount);
            }
            SYST_RVR = syst_rvr + (uint32_t)command; // Update sysTick reload value
         }
      }
#endif
#if (EP == 1)  //TODO: Used when RTC crystal is source for Sys Tick adjustment.  Remove when using TCXO.
      accDiffRTC -= (portTICK_RATE_MS * 1000);
#endif         //End TODO
      if (0 == (--cnt))
      {
         cnt = (uint16_t)(SYS_TIME_TICK_IN_mS / portTICK_RATE_MS);
         processSysTick(); /* Process the tick */
      }
      else
      {  //check if we need to request time
#if (EP == 1)
         if ( !TIME_SYS_IsTimeValid() )
         {  //System time not valid
#if 0 // TODO: RA6E1 - Timer implementation
           if (statusTimeRequest_ != STATUS_TIME_REQUEST_WAIT_STATE)
            {  //Send the request to request time
               timer_t timerCfg;      //Configure timer

               (void)memset(&timerCfg, 0, sizeof(timerCfg));   /* Clear the timer values */
               timerCfg.bOneShot       = (bool)true;
               timerCfg.bOneShotDelete = (bool)true;
               timerCfg.ulDuration_mS  = aclara_randu(MIN_TIME_REQUEST_TIMEOUT, timeVars_.timeRequestMaxTimeout) * TIME_TICKS_PER_SEC; /* convert parameter to ms */
               timerCfg.pFunctCallBack = TIME_SYS_reqTimeoutCallback;
               if ( TMR_AddTimer(&timerCfg) != eSUCCESS ) {
                  ERR_printf("Failed to create a callback timer for request time");
               } else {
                  (void)MAC_TimeQueryReq();
                  statusTimeRequest_ = STATUS_TIME_REQUEST_WAIT_STATE; //Wait for the time
               }
            }
#endif
         }
         else
         {  //System time valid
            //Check, if need to adjust SYST_RVR register.
#if 0 //TODO: Not used when RTC crystal is source for Sys Tick adjustment.  Put back in when using TCXO.
            if ( loopCnt >= 100 )  // 100 sys ticks i.e. one second
            {
               uint32_t cpuFreq, TCXOfreq;

               // CPU must be trimmed before any adjustment
               if ( TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL ) ) {
                  syst_rvr = ((cpuFreq + (BSP_ALARM_FREQUENCY / 2)) / BSP_ALARM_FREQUENCY) - 1; // Round up

                  if (TCXOToSystickError == 0) {
                     if (CYCdiff != 0) {
                        TCXOToSystickError     = CYCdiff; // Update the error to track
                        TCXOToSystickErrorCntr = 0;
                     }
                  } else {
                     (void)RADIO_TCXO_Get( (uint8_t)RADIO_0, &TCXOfreq, NULL, NULL );
                     TCXOToSystickErrorCntr++;

                     error = (int32_t)(TCXOfreq - RADIO_TCXO_NOMINAL);
                     error = (int32_t)(((int64_t)error * (int64_t)cpuFreq * (int64_t)TCXOToSystickErrorCntr) / (int64_t)RADIO_TCXO_NOMINAL);

                     syst_rvr += (((int32_t)TCXOToSystickError+error)-(int32_t)CYCdiff) / (int32_t)BSP_ALARM_FREQUENCY;
                  }

                  // Clip changes if too large
                  if ( syst_rvr > (uint32_t)(((float32_t)cpuFreq/(float32_t)BSP_ALARM_FREQUENCY)*1.1f) ) {
                     syst_rvr = (uint32_t)(((float32_t)cpuFreq/(float32_t)BSP_ALARM_FREQUENCY)*1.1f);
                  }
                  else if ( syst_rvr < (uint32_t)(((float32_t)cpuFreq/(float32_t)BSP_ALARM_FREQUENCY)*0.9f) ) {
                     syst_rvr = (uint32_t)(((float32_t)cpuFreq/(float32_t)BSP_ALARM_FREQUENCY)*0.9f);
                  }
                  SYST_RVR = syst_rvr;
               }
//               INFO_printf("init %u cycdiff %u %u msec error %d RCR %u TCXO error %d", TCXOToSystickError, CYCdiff, (uint32_t)(((uint64_t)CYCdiff*1000000ULL)/(uint64_t)cpuFreq), CYCdiff-TCXOToSystickError, SYST_RVR, error );
               loopCnt = 0;
            }
            loopCnt++;
#else
#if ( MCU_SELECTED != RA6E1 ) /* TODO: RA6E1 - RTC accuracy is 128th of a second. In K24, it is 32000th of a second.
                                       Hence we cannot sync the RTC with the timesys in sub-millisecond level of 10/100 sec.
                                       Once radio TCXO is in place, it will be used for the timesync in RA6E1 */
            if ( loopCnt >= 1000 )  //1000 sys ticks
            {
               RTC_GetTimeInSecMicroSec( &sec, &usec); //Get RTC time and store the current time
               currTime = ((uint64_t)sec * 1000000) + usec;
               diffRTC = (int64_t)(currTime - lastTime); //compute the difference in RTC last and now (in micro-seconds)
               lastTime = currTime;

               if ( diffRTC >= ((SYS_TIME_TICK_IN_mS * 1000) * 980) && diffRTC <= ((SYS_TIME_TICK_IN_mS * 1000) * 1020) )
               {
                  //Allow upto 2% error
                  accDiffRTC += diffRTC;

                  if ( (accDiffRTC - lastAccDiffRTC)/10000 == 0)
                  { //Allow .1% driff between samples

                     if (accDiffRTC > 100 || accDiffRTC < -100 )
                     {  //Fix the drift, if more than 100 microseconds per 10 seconds
                        uint64_t regVal;
                        uint32_t rawRead;

                        do
                        {
#if (RTOS_SELECTION == MQX_RTOS)
                           regVal = SYST_RVR;
                           rawRead = SYST_RVR; //reread the value
#elif (RTOS_SELECTION == FREE_RTOS)
                           regVal = SysTick->LOAD;
                           rawRead = SysTick->LOAD; //reread the value
#endif
                        }
                        while (rawRead != regVal);

                        regVal *= ((uint64_t)10000000 - (uint64_t)accDiffRTC); //10 seconds +- drift
                        regVal /= 10000000;
                        if (regVal < 594000)
                        {  //Cap the adjustments at 1%
                           regVal = 594000;
                        }
                        if (regVal > 606000)
                        {  //Cap the adjustments at 1%
                           regVal = 606000;
                        }
                        do
                        {
#if (RTOS_SELECTION == MQX_RTOS)
                           SYST_RVR = (uint32_t)regVal;
                           rawRead = SYST_RVR; //verify that the write is correct
#elif (RTOS_SELECTION == FREE_RTOS)
                           SysTick->LOAD = (uint32_t)regVal;
                           rawRead = SysTick->LOAD; //verify that the write is correct
#endif
                        }
                        while (rawRead != regVal);
                     }
                  }
               }
               else
               {  //Error reading RTC or some other error. Ignore the sample
                  accDiffRTC = 0;
               }
               loopCnt = 0;
               lastAccDiffRTC = accDiffRTC;
               accDiffRTC = 0;
               //TODO: When time set does not align to sysTick interval
               //get time setting offset from desired time (as timeoffset in 1ms resolution)
               //if timeoffset not zero
               //  calculate timeAdjust (RVR total delta) as: (timeoffset*regVal/SYS_TIME_TICK_IN_mS) //Need to also round up
               //  calculate timeIncr (RVR total delta) as: (timeAdjust / SYSTickFreq)
               //  write new RVR value as regVal + timeIncr
            }
            //TODO: (cont.)When time set does not align to sysTick interval
            //if timeAdjust is not zero
            //  if timeAdjust > timeIncr
            //    decrement timeAdjust by timeIncr
            //  else
            //    Ignore the remaining adjustment since it only accounts for a few tens of us
            //    set timeAdjust to 0
            //    adjust accDiffRTC by timeoffset
            loopCnt++;
#endif
#endif
         }
#endif
      }
   }
}/*lint !e715  Arg0 is not used. */

#if (EP == 1)
/***********************************************************************************************************************
 *
 * Function name: TIME_SYS_acceptanceTimeoutCallback
 *
 * Purpose: Indicates that timer has expired
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side Effects:  None
 *
 **********************************************************************************************************************/
void TIME_SYS_acceptanceTimeoutCallback( uint8_t cmd, void *pData )
{
   acceptanceTimerRunning_ = (bool)false;
}  /*lint !e715 !e818  parameters are not used. */
#endif

#if (EP == 1)
/***********************************************************************************************************************
 *
 * Function name: TIME_SYS_reqTimeoutCallback
 *
 * Purpose: Indicates that timer has expired
 *
 * Arguments: None
 *
 * Returns: None
 *
 * Side Effects:  None
 *
 **********************************************************************************************************************/
void TIME_SYS_reqTimeoutCallback( uint8_t cmd, void *pData )
{
   statusTimeRequest_ = 0;
}  /*lint !e715 !e818  parameters are not used. */
#endif

/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_vApplicationTickHook
 *
 * Purpose: This function extends RTOS tick ISR. This function runs at ISR level and should only use ISR safe
 *          RTOS API's. This function give semaphore to the time task on each RTOS tick.
 *
 * Arguments: pointer user_isr_ptr
 *
 * Returns: None
 *
 * Side effects: None
 *
 * Reentrant: This function is reentrant. This function should only be called from an RTOS Tick ISR
 *
 ******************************************************************************************************************/
#if 0 // TODO: RA6E1 - Using the tickHook provided by FreeRTOS. Need to add a define to call this function
STATIC void TIME_SYS_vApplicationTickHook( void * user_isr_ptr )
#else
void vApplicationTickHook()
#endif
{
   uint32_t primask = __get_PRIMASK();
   __disable_interrupt(); // Disable all interrupts so that we are not interrupted by another interrupt while processing this one.

#if 0 /* TODO: ISR control for FreeRTOS */
   MY_ISR_STRUCT_PTR_MQX   isr_ptr;   /* */
#endif
#if ( DCU == 1 )
   KERNEL_DATA_STRUCT_PTR  kd_ptr = _mqx_get_kernel_data();
   tickSystemClock(kd_ptr->CYCCNT); // Increment system and power-up time
#else
   tickSystemClock(0); // Increment system and power-up time (argument ignored for EP)
#endif
   __set_PRIMASK(primask); // Restore interrupts

#if 0 /* TODO: ISR control for FreeRTOS */
   /* This code is taken from the MQX example isr.c code to use the system tick to tick our own module. */
   isr_ptr = (MY_ISR_STRUCT_PTR_MQX)user_isr_ptr;
#endif

   /* RTOS tick, signal the timer task */
   if ( _timeSysSemCreated == (bool)true )
   {
#if ( RTOS_SELECTION == MQX_RTOS )
      OS_SEM_Post( &_timeSysSem );
#elif ( RTOS_SELECTION == FREE_RTOS )
      OS_SEM_Post_fromISR( &_timeSysSem );
#endif
   }
#if 0 /* TODO: ISR control for FreeRTOS */
   (*isr_ptr->OLD_ISR)(isr_ptr->OLD_ISR_DATA);     /* Chain to the previous notifier - This will call the RTOS tick. */
#endif
   //LED2_PIN_OFF();
}


#if 0 /* TODO: OR_PM and MAC layer support */
#if ( EP == 1 )
/*****************************************************************************************************************
 *
 * Function name: TIME_SYS_SetDateTimeFromMAC
 *
 * Purpose: MAC layer calls this function to set system date and time
 *
 * Arguments: MAC_DataInd_t const *pDataInd: Time Sync parameters
 *
 * Returns: returnStatus_t retVal - eSUCCESS - Time updated successfully, eFAILURE - otherwise
 *
 * Side effects: Updates System time
 *
 * Reentrant: YES
 *
 ******************************************************************************************************************/
returnStatus_t TIME_SYS_SetDateTimeFromMAC(MAC_DataInd_t const *pDataInd)
{
   time_set_t time_set;
   uint8_t const *data = &pDataInd->payload[1];
   returnStatus_t retVal = eFAILURE; //Default

   time_set.ts.QSecFrac = 0; //Default to invalid time
   // Decode the time set parameters
   if(pDataInd->payloadLength == 11)
   {
      time_set.ts.seconds    =  (((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] <<  8) | data[3] );
      time_set.ts.QFrac      =  (((uint32_t)data[4] << 24) | ((uint32_t)data[5] << 16) | ((uint32_t)data[6] <<  8) | data[7] );
      time_set.LeapIndicator =             (data[8] >>  6);
      time_set.Poll          =              data[8]  & 0x3F;
      time_set.Precision     =   (int8_t)   data[9];

      //Validate
      if (time_set.Precision < TIME_PRECISION_MIN) {        //Time precision range check
         INFO_printf("TSync rejected: time_set.Precision(%d) < TIME_PRECISION_MIN(%d)", time_set.Precision, TIME_PRECISION_MIN );
         time_set.ts.QSecFrac = 0;
      } else if (time_set.Precision > TIME_PRECISION_MAX) { //Time precision range check
         INFO_printf("TSync rejected: time_set.Precision(%d) > TIME_PRECISION_MAX(%d)", time_set.Precision, TIME_PRECISION_MIN );
         time_set.ts.QSecFrac = 0;
      } else if (acceptanceTimerRunning_) {                 //Time-sync within timeAcceptanceDelay
         INFO_printf("TSync rejected: Not within acceptance time");
         time_set.ts.QSecFrac = 0;
      } else if ( TIME_SYS_IsTimeValid() && (LEAP_INDICATOR_NO_SYNC == time_set.LeapIndicator) ) {
         INFO_printf("TSync rejected: System time is valid but leap indicator is NO_SYNC");
         time_set.ts.QSecFrac = 0;
      }
#if ( USE_MTLS == 1 )
      else if ( time_set.ts.seconds < getMtlsLastSignedTimestamp() ) { //Network time less than mtlsLastSignedTimestamp
          INFO_printf("TSync rejected: Network time less than mtlsLastSignedTimestamp");
          time_set.ts.QSecFrac = 0;
      }
#endif
      else if (TIME_SYNC_LogPrecisionToPrecisionInMicroSec(time_set.Precision) >= TIME_SYNC_TimePrecisionMicroSeconds_Get()) { //Current time is at better accuracy
          INFO_printf("TSync rejected because current time precision (%u) is better than received (%u)", TIME_SYNC_TimePrecisionMicroSeconds_Get(), TIME_SYNC_LogPrecisionToPrecisionInMicroSec(time_set.Precision));
          time_set.ts.QSecFrac = 0;
      }
   }

   if ( 0 != time_set.ts.QSecFrac ) //Network time is non zero and validation passed
   {  //Accept the time-sync
      bool     clippedTime = (bool)false;
      timer_t  timerCfg;            // Configure timer
      uint32_t cpuFreq;
      uint32_t systRVR;
      uint32_t systRVRUpdate;
      int32_t  timeSigOffsetInMs;   // Significant offset
      int64_t  offset;              // Offset
      TIMESTAMP_t syncTime;
      TIMESTAMP_t recvTime;
      uint64_t currentTimeCombined;
      sysTime_t newTime;
      sysTime_dateFormat_t sDateFormat;

      /* If there is no time acceptance delay, there is no need to create a timer to toggle the timer running
         static variable and prevent future time sync acceptance.  Perform a check here to determine if we need
         to create a timer that will prevent future time syncs for the time acceptance delay time period. */
      if( timeVars_.timeAcceptanceDelay > 0 )
      {
         acceptanceTimerRunning_ = (bool)true;
         (void)memset(&timerCfg, 0, sizeof(timerCfg));   /* Clear the timer values */
         timerCfg.bOneShot       = (bool)true;
         timerCfg.bOneShotDelete = (bool)true;
         timerCfg.ulDuration_mS  = timeVars_.timeAcceptanceDelay * TIME_TICKS_PER_SEC;
         timerCfg.pFunctCallBack = TIME_SYS_acceptanceTimeoutCallback;
         (void)TMR_AddTimer(&timerCfg);
      }

      timeSigOffsetInMs = (int32_t)timeVars_.timeSigOffset;

      TIME_UTIL_PrintQSecFracFormat("Received TimeSync ", pDataInd->timeStamp.QSecFrac);
      (void)TIME_SYS_GetRealCpuFreq( &cpuFreq, NULL, NULL );

      // Setting time takes a little more than 1ms so let's make sure we have some leeway when adjusting time.
      // We don't want time to change (i.e. get a SysTick) while in the middle of adjusting.
      OS_TASK_Sleep(1); // This will force this task to sleep until the next OS tick (10ms granularity).

      // Compute the difference (offset) between the current time and the timeSync time.
      syncTime.QSecFrac = time_set.ts.QSecFrac;                             // Time in timeSync payload
      recvTime.QSecFrac = pDataInd->timeStamp.QSecFrac;                     // Time when the timeSync message was received (i.e. when radio SYNC interrupt was detected)
      offset            = (int64_t)(syncTime.QSecFrac - recvTime.QSecFrac); // Difference between timeSync time and when the timeSync message was received

      // Convert the difference (offset) into msec but round down to the nearest 10ms
      //     This is what I wanted to do:
      //     offset = ((offset * 100LL) >> 32) * 10LL;
      //     But it overflows so let's rearrange the equation to avoid overflow.
      //     We can get away with it because we want a 10msec granularity (OS granularity) so we can divide first and lose some precision that we don't care about anyway.
      offset = (((offset >> 16) * 100LL) >> 16) * 10LL; /*lint !e704 */ // Convert the difference to msec but round down to the nearest 10s of msec

      // Check if adjusting time would cross a minute boundary
      if ( offset > -timeSigOffsetInMs && offset < timeSigOffsetInMs )
      {  //less than threshold, set the offset so that we do not jump the minute boundary
         (void)TIME_UTIL_GetTimeInSysCombined(&currentTimeCombined); //get current time
         uint32_t seconds = (uint32_t)(currentTimeCombined % TIME_TICKS_PER_MIN)/TIME_TICKS_PER_SEC;

         if (offset < 0 )
         {  //time moved backward
            if (seconds <= 1)
            {  //No jump possible without crossing the minute and one second boundary
               offset = 0;
               clippedTime = (bool)true;
            }
            else
            {  //Jump backward upto offset so that we do not cross the minute and one second boundary
               int32_t maxBack = (int32_t)((seconds - 1) * TIME_TICKS_PER_SEC);
               if ( -offset > maxBack )
               {  //Jump will cause to cross the boundary, limit the jump
                  offset = -maxBack;
                  clippedTime = (bool)true;
               }
               //else will not cause the boundary jump
            }
         }
         else
         {  //time moved forward, jump forward upto offset so that we do not cross the one second to minute boundary
            int32_t maxFwd = (int32_t)((59 - seconds) * TIME_TICKS_PER_SEC);
            if ( offset > maxFwd )
            {  //Jump will cause to cross the boundary, limit the jump
               offset = maxFwd;
               clippedTime = (bool)true;
            }
            //else will not cause the boundary jump
         }
      }
      //else Jump above threshold

      //TODO: Should get PRIMASK, don't want to re-enable if interrupts are already disabled
      //TODO: Removed due to nested OS calls: __disable_interrupt(); // Disable all interrupts. This section is time critical.

      // Try to adjust time as close as possible to timeSync if the offset wasn't clipped.
      // If the offset was clipped then just do a rough time adjument. We'll do better with the next timeSync hoppefuly.
      if ( clippedTime ) {
         // Update time the good old way. This is not great and the time won't be correct anyway because it was clipped so why bother with precision.
         (void)TIME_UTIL_GetTimeInSysCombined(&currentTimeCombined); // Get current time. We may already have this from a previous call but this is the most up to date time. We need this.
         currentTimeCombined += (uint64_t)offset; // Compensate
         TIME_UTIL_ConvertSysCombinedToSysFormat(&currentTimeCombined, &newTime);
         TIME_SYS_SetSysDateTime(&newTime);                                                        //TODO: <--this has a MUTEX call!
         INFO_printf("Tsync coarse adjustment");                                                   //TODO: <--this has a MUTEX call!
      } else {
         uint64_t timePlusLatency;
         uint64_t timePlusLatency2;
         uint64_t currentTimeRebuilt;
         uint64_t diffTime;

         // We have some margin so let's do the best time adjustment we can.

         // Make sure the time adjustment is not close to cross a 10ms boundary.
         // if so, waste some time such that we won't have to worry about 10ms boundary crossing while we adjust time.

         systRVR = SYST_RVR; // Save the original value for later
         _ticTocCntr = 1;    // Set to 1 because we are going to waste up to 10ms here.
                             // In other words, we are getting ready for the next 10ms tick that expects tictoc to be 1.

         uint32_t primask = __get_PRIMASK();
         __disable_interrupt(); // Disable all interrupts. This section is time critical.
         do {
            // Add processing latency to the timeSync time to compensate for processing delays
            timePlusLatency = syncTime.QSecFrac + TIME_UTIL_ConvertCyccntToQSecFracFormat(DWT_CYCCNT - pDataInd->timeStampCYCCNT);

            currentTimeCombined = (((timePlusLatency >> 16) * 100LL) >> 16) * 10LL;     // Convert the updated timeSync time in 10ms granularity
            currentTimeRebuilt  = (((currentTimeCombined / 10LL) << 16) / 100LL) << 16; // Convert the converted timeSync back in Q32.32 format.
                                                                                        // This is to remove the final error (i.e. less than 10ms)
            // Update system time with timeSync time
            TIME_UTIL_ConvertSysCombinedToSysFormat(&currentTimeCombined, &newTime);
            TIME_SYS_SetSysDateTime(&newTime);                                                  //TODO: <--this has a MUTEX call!

            // This is where the magic happens. We refine the time from a 10ms granularity to usec precision.
            // Recompute with time setting added latency
            timePlusLatency2 = syncTime.QSecFrac + TIME_UTIL_ConvertCyccntToQSecFracFormat(DWT_CYCCNT - pDataInd->timeStampCYCCNT);
            diffTime = timePlusLatency2 - currentTimeRebuilt;

            systRVRUpdate = (2*SYST_RVR)-(uint32_t)((diffTime * (uint64_t)cpuFreq) >> 32); // Compute RVR to waste the remainder of 10ms
         } while ( systRVR < (uint32_t)( 0.0001f * (float32_t)cpuFreq ) ); // Keep iterating if we are too close from a 10ms transition.

         SYST_RVR = systRVRUpdate; // Set RVR to waste the remainder of 10ms
         SYST_CVR = 0;             // This doesn't really writes 0. Any value would work. It just forces RVR to load CVR indireclty because we can't load CVR directly.
         SYST_RVR = systRVR;       // Restore previous value.  This won't impact CVR until the next sysTick interrupt. That's what we want.
//TODO: Added #if DCU when putting RTC as source for sys tick adjustment since not used.  Put back in when using TCXO.
#if ( DCU == 1 )
         CYCdiff  = 0;             // Reset error tracking
         TCXOToSystickError = 0;   // Reset error tracking
#endif
//End TODO
         __set_PRIMASK(primask);   // Restore interrupts

//         TIME_UTIL_PrintQSecFracFormat("syncTime          ", syncTime.QSecFrac);
//         TIME_UTIL_PrintQSecFracFormat("timePlusLatency   ", timePlusLatency);
//         TIME_UTIL_PrintQSecFracFormat("timePlusLatency2  ", timePlusLatency2);
//         TIME_UTIL_PrintQSecFracFormat("currentTimeRebuilt", currentTimeRebuilt);
//         TIME_UTIL_PrintQSecFracFormat("diffTime          ", diffTime);
//         INFO_printf("newTime %u", newTime.time);
//         INFO_printf("SYST_RVR %u", systRVRUpdate);
         INFO_printf("Tsync fine adjustment");                                                 //TODO: <-- this has a MUTEX call which is not safe be when HW interrupts are disabled!
      }

      // TODO: Removed due to nested OS calls: __enable_interrupt(); // Enable interrupts.
      INFO_printf("Tsync final offset %lld, new time %llu", offset, currentTimeCombined);

      INFO_printf("time adjust %lld", offset);
      (void)TIME_UTIL_ConvertSysFormatToDateFormat(&newTime, &sDateFormat);
      (void)RTC_SetDateTime (&sDateFormat);

      //Update the last time-sync received
      OS_MUTEX_Lock(&_timeVarsMutex); // Function will not return if it fails
      timeVars_.timeLastUpdated = (uint32_t)(currentTimeCombined / TIME_TICKS_PER_SEC);
      (void)TIME_SYNC_TimeLastUpdatedPrecision_Set(time_set.Precision); //Save the precision
      (void)FIO_fwrite(&fileHndlTimeSys_, (uint16_t)offsetof(time_vars_t,timeLastUpdated),
                       (uint8_t*)&timeVars_.timeLastUpdated, (lCnt)sizeof(timeVars_.timeLastUpdated));
      OS_MUTEX_Unlock( &_timeVarsMutex );  /* End critical section */ // Function will not return if it fails

#if (ACLARA_DA == 1)
      // Update time on host board if valid time received
      (void)B2BSendTimeSync();
#endif

      retVal = eSUCCESS;
   }
   return retVal;
}
#endif


/***********************************************************************************************************************
 * This function handles OTA get/set of OR PM params
 *  Arguments:  action-> set or get
 *              id    -> HEEP enum associated with the value
 *              value -> pointer to new value to be placed in file
 *              attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL
 *   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.
 * *********************************************************************************************************************/
returnStatus_t TIME_SYS_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case timeState : // get timeState parameter
         {  //The reading will fit in the buffer
            *(uint16_t *)value = TIME_SYS_TimeState();
            retVal = eSUCCESS;
            if (attr != NULL)
            {
               attr->rValLen = (uint16_t)sizeof(timeVars_.timeState);
               attr->rValTypecast = (uint8_t)uintValue;
            }
            break;
         }
#if( EP == 1 )
         case timeRequestMaxTimeout :
         {
            if ( sizeof(timeVars_.timeRequestMaxTimeout) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
             *(uint16_t *)value = TIME_SYS_GetTimeRequestMaxTimeout();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeVars_.timeRequestMaxTimeout);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case dateTimeLostCount :  // get dateTimeLostCount parameter
         {
            if ( sizeof(timeVars_.dateTimeLostCount) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
             *(uint16_t *)value = TIME_SYS_GetDateTimeLostCount();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeVars_.dateTimeLostCount);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
#endif
         case timeLastUpdated :  // get timeLastUpdated parameter
         {
            if ( sizeof(timeVars_.timeLastUpdated) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
             *(uint32_t *)value = TIME_SYS_GetTimeLastUpdated();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeVars_.timeLastUpdated);
                  attr->rValTypecast = (uint8_t)dateTimeValue;
               }
            }
            break;
         }
#if( EP == 1 )
         case installationDateTime :  // get installationDateTime parameter
         {
            if ( sizeof(timeVars_.installDateTime) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               *(uint32_t *)value = TIME_SYS_GetInstallationDateTime();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeVars_.installDateTime);
                  attr->rValTypecast = (uint8_t)dateTimeValue;
               }
            }
            break;
         }
         case timeAcceptanceDelay : // get timeAcceptanceDelay paramter
         {
            if ( sizeof(timeVars_.timeAcceptanceDelay) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
             *(uint16_t *)value = TIME_SYS_GetTimeAcceptanceDelay();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeVars_.timeAcceptanceDelay);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case timeSigOffset : //get the time signifigant offset
         {

            if( sizeof(timeVars_.timeSigOffset) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {   //the reading will fi tin the buffer
               *(uint16_t *)value = TIME_SYS_getTimeSigOffset();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(timeVars_.timeSigOffset);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
        }
#endif
         case rtcDateTime :  // get rtcDateTime parameter
         {
            sysTime_t sysTime;
            sysTime_dateFormat_t rtcTime;
            uint32_t fracSec;
            if ( sizeof(uint32_t) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {  //The reading will fit in the buffer
               RTC_GetDateTime ( &rtcTime );
               (void)TIME_UTIL_ConvertDateFormatToSysFormat(&rtcTime, &sysTime);
               TIME_UTIL_ConvertSysFormatToSeconds(&sysTime, (uint32_t *)value, &fracSec);
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(uint32_t);
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
      } /*end switch *//*lint !e788 not all enums used within switch */
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
#if( EP == 1 )
         case timeRequestMaxTimeout : // set timeRequestMaxTimeout parameter
         {
            retVal = TIME_SYS_SetTimeRequestMaxTimeout(*(uint16_t *)value);
            break;
         }
         case dateTimeLostCount : // set dateTimeLostCount paramter
         {
            TIME_SYS_SetDateTimeLostCount(*(uint16_t *)value);
            retVal = eSUCCESS;
            break;
         }
         case timeAcceptanceDelay : // set timeAcceptanceDelay paramter
         {
            retVal = TIME_SYS_SetTimeAcceptanceDelay(*(uint16_t *)value);
            break;
         }
          case timeSigOffset : //set the time signifigant offset
         {
            retVal = TIME_SYS_setTimeSigOffset(*(uint16_t *)value);

            break;
         }
#endif
         case rtcDateTime : // set rtcDateTime paramter
         {
            sysTime_t sysTime;
            sysTime_dateFormat_t rtcTime;
            TIME_UTIL_ConvertSecondsToSysFormat(*(uint32_t *)value, (uint32_t) 0, &sysTime);
            (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &rtcTime);  //convert sys time to RTC time format
            if (RTC_SetDateTime (&rtcTime) )
            {
               retVal = eSUCCESS;
            }
            break;
         }

         default :
         {
            retVal = eAPP_NOT_HANDLED;
            break;
         }
      } /*end switch *//*lint !e788 not all enums used within switch */
   }
   return ( retVal );
}
#endif