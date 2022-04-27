/***********************************************************************************************************************
 *
 * Filename: pwr_config.c
 *
 * Global Designator: PWRCFG_
 *
 * Contents: Configuration parameter I/O for power outage and restoration.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2020-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author David McGraw
 *
 * $Log$
 *
 ***********************************************************************************************************************
 * Revision History:
 *
 * @version
 * #since
  **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <string.h>

#include "compiler_types.h"
#include "file_io.h"

#define PWRCFG_GLOBALS
#include "pwr_config.h"
#undef PWRCFG_GLOBALS

#include "DBG_SerialDebug.h"
#include "HEEP_util.h"

#if ( LAST_GASP_SIMULATION == 1 )
#include "time_sys.h"
#include "time_util.h"
#include "EVL_event_log.h"
#endif

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint16_t      fileVersion;
   uint16_t      uOutageDeclarationDelaySeconds;  /* No. seconds of continuous power fail to declare an outage. */
   uint16_t      uRestorationDelaySeconds;        /* No. seconds of continuous power good to declare outage ended. */
   uint16_t      uPowerQualityEventDuration;      /* Elapsed seconds required to end a Power Quality Event. */
   uint8_t       uLastGaspMaxNumAttempts;         /* The maximum number of attempts that the EndPoint is to attempt
                                                     within the 20 minute period subsequent to a sustained outage declaration.*/
#if ( LAST_GASP_SIMULATION == 1 )
   uint8_t       SimLGMaxAttempts;                /* Max simulation message attempts, range 0 – 6 */
   uint16_t      SimLGDuration;                   /* Duration of the simulation */
   uint32_t      SimLGStartTime;                  /* Time to Start the LG Simulation */
   bool          SimLGTraffic;                    /* F=Full simulation, T=SimulateLGTx only */
#endif
} pwrConfigFileData_t;

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
/*lint -esym(750,OUTAGE_DECLARATON_DELAY_MINIMUM_SECONDS)  */
#define PWRCFG_FILE_UPDATE_RATE_CFG_SEC ((uint32_t) 30 * 24 * 3600)    /* File updates once per month. */

#define PWRCFG_FILE_VERSION                              3

#define OUTAGE_DECLARATON_DELAY_DEFAULT_SECONDS          30
#define OUTAGE_DECLARATON_DELAY_MINIMUM_SECONDS          0
#define OUTAGE_DECLARATON_DELAY_MAXIMUM_SECONDS          300

#define RESTORATION_DELAY_DEFAULT_SECONDS                15
#define RESTORATION_DELAY_MAXIMUM_SECONDS                300

#define POWER_QUALITY_EVENT_DURATION_DEFAULT_SECONDS     300
#define POWER_QUALITY_EVENT_DURATION_MAXIMUM_SECONDS     POWER_QUALITY_EVENT_DURATION_DEFAULT_SECONDS
#define POWER_QUALITY_EVENT_DURATION_MINIMUM_SECONDS     60

#define LASTGASP_MAXIMUM_ATTEMPTS                        ((uint8_t)6)
#define LASTGASP_MAXIMUM_ATTEMPTS_DEFAULT                LASTGASP_MAXIMUM_ATTEMPTS

#define LASTGASP_SIM_MAX_ATTEMPTS                        LASTGASP_MAXIMUM_ATTEMPTS
#define LASTGASP_SIM_MAX_ATTEMPTS_DEFAULT                LASTGASP_SIM_MAX_ATTEMPTS

#define LASTGASP_SIM_MAX_DURATION_SECONDS                (2*60*60)
#define LASTGASP_SIM_MIN_DURATION_SECONDS                20

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static FileHandle_t        pwrConfigFileHandle = {0};
static OS_MUTEX_Obj        pwrConfigMutex;
static pwrConfigFileData_t pwrConfigFileData = {0};
#if ( LAST_GASP_SIMULATION == 1 )
static SimStats_t          simLGStatistics[LASTGASP_SIM_MAX_ATTEMPTS];
#endif
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */


/***********************************************************************************************************************
 *
 * Function name:
 *
 * Purpose:
 *
 * Arguments:
 *
 * Returns: void
 *
 * Re-entrant Code:
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_init(void)
{
   FileStatus_t   fileStatus;
   returnStatus_t retVal = eFAILURE;

   if (OS_MUTEX_Create(&pwrConfigMutex))
   {
      if (eSUCCESS == FIO_fopen(&pwrConfigFileHandle, ePART_SEARCH_BY_TIMING, (uint16_t)eFN_PWRCFG, (lCnt)sizeof(pwrConfigFileData),
                                FILE_IS_NOT_CHECKSUMED, PWRCFG_FILE_UPDATE_RATE_CFG_SEC, &fileStatus) )
      {
         if (fileStatus.bFileCreated)
         {
            pwrConfigFileData.fileVersion                      = PWRCFG_FILE_VERSION;
            pwrConfigFileData.uOutageDeclarationDelaySeconds   = OUTAGE_DECLARATON_DELAY_DEFAULT_SECONDS;
            pwrConfigFileData.uRestorationDelaySeconds         = RESTORATION_DELAY_DEFAULT_SECONDS;
            pwrConfigFileData.uPowerQualityEventDuration       = POWER_QUALITY_EVENT_DURATION_DEFAULT_SECONDS;
            pwrConfigFileData.uLastGaspMaxNumAttempts          = LASTGASP_MAXIMUM_ATTEMPTS_DEFAULT;
#if ( LAST_GASP_SIMULATION == 1 )
            pwrConfigFileData.SimLGMaxAttempts                 = LASTGASP_SIM_MAX_ATTEMPTS_DEFAULT;
            pwrConfigFileData.SimLGStartTime                   = 0;
            pwrConfigFileData.SimLGDuration                    = 300;
            pwrConfigFileData.SimLGTraffic                     = PWRCFG_SIM_LG_TX_ALL;
#endif
            retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
         }
         else
         {
            retVal = FIO_fread(&pwrConfigFileHandle, (uint8_t*) &pwrConfigFileData, 0, (lCnt)sizeof(pwrConfigFileData));
         }
      }
   }
   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_outage_delay_seconds
 *
 * Purpose: Returns the outage declaration delay
 *
 * Arguments: None
 *
 * Returns: uint16_t outage declaration delay in seconds
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint16_t PWRCFG_get_outage_delay_seconds(void)
{
   uint16_t uOutageDeclarationDelaySeconds;

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uOutageDeclarationDelaySeconds = pwrConfigFileData.uOutageDeclarationDelaySeconds;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return(uOutageDeclarationDelaySeconds);
}


/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_outage_delay_seconds
 *
 * Purpose: Sets the outage declaration delay
 *
 * Arguments: uint16_t outage declaration delay in seconds
 *
 * Returns: Status
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_outage_delay_seconds(uint16_t uOutageDeclarationDelaySeconds)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if (uOutageDeclarationDelaySeconds <= OUTAGE_DECLARATON_DELAY_MAXIMUM_SECONDS)
   {
      OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails

      pwrConfigFileData.uOutageDeclarationDelaySeconds = uOutageDeclarationDelaySeconds;
      retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
      OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

      if (eSUCCESS == retVal)
      {
         DBG_logPrintf('I', "eFN_PWRCFG Write Successful");
      }
      else
      {
         DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
      }
   }
   else
   {
      DBG_logPrintf('E', "eFN_PWRCFG Write Failed, Value Out of Range");
   }

   return(retVal);
}


/***********************************************************************************************************************

   Function Name: PWRCFG_OutageDeclarationDelayHandler( action, id, value, attr )

   Purpose: Get/Set outageDeclarationDelay

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc(715,PWRCFG_OutageDeclarationDelayHandler) argument not referenced. */
returnStatus_t PWRCFG_OutageDeclarationDelayHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(pwrConfigFileData.uOutageDeclarationDelaySeconds) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint16_t *)value = PWRCFG_get_outage_delay_seconds();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.uOutageDeclarationDelaySeconds);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = PWRCFG_set_outage_delay_seconds(*(uint16_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_restoration_delay_seconds
 *
 * Purpose: Returns the restoration delay
 *
 * Arguments: None
 *
 * Returns: uint16_t restoration delay in seconds
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint16_t PWRCFG_get_restoration_delay_seconds(void)
{
   uint16_t uRestorationDelaySeconds;

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uRestorationDelaySeconds = pwrConfigFileData.uRestorationDelaySeconds;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return(uRestorationDelaySeconds);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_PowerQualityEventDuration
 *
 * Purpose: Sets the restoration delay
 *
 * Arguments: uint16_t restoration delay in seconds
 *
 * Returns: Status
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_PowerQualityEventDuration(uint16_t uPowerQualityEventDuration)
{
   returnStatus_t retVal = eFAILURE;

   if (uPowerQualityEventDuration <= POWER_QUALITY_EVENT_DURATION_MAXIMUM_SECONDS &&
       uPowerQualityEventDuration >= POWER_QUALITY_EVENT_DURATION_MINIMUM_SECONDS)
   {
      OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
      pwrConfigFileData.uPowerQualityEventDuration = uPowerQualityEventDuration;
      retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
      OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

      if (eSUCCESS == retVal)
      {
         DBG_logPrintf('I', "eFN_PWRCFG Write Successful");
      }
      else
      {
         DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
      }
   }
   else
   {
      DBG_logPrintf('E', "eFN_PWRCFG Write Failed, Value Out of Range");
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_PowerQualityEventDuration
 *
 * Purpose: Returns the restoration delay
 *
 * Arguments: None
 *
 * Returns: uint16_t restoration delay in seconds
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint16_t PWRCFG_get_PowerQualityEventDuration(void)
{
   uint16_t uPowerQualityEventDuration;

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uPowerQualityEventDuration = pwrConfigFileData.uPowerQualityEventDuration;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return( uPowerQualityEventDuration );
}


/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_restoration_delay_seconds
 *
 * Purpose: Sets the restoration delay
 *
 * Arguments: uint16_t restoration delay in seconds
 *
 * Returns: Status
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_restoration_delay_seconds(uint16_t uRestorationDelaySeconds)
{
   returnStatus_t retVal = eFAILURE;

   if (uRestorationDelaySeconds <= RESTORATION_DELAY_MAXIMUM_SECONDS)
   {
      OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails

      pwrConfigFileData.uRestorationDelaySeconds = uRestorationDelaySeconds;
      retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
      OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

      if (eSUCCESS == retVal)
      {
         DBG_logPrintf('I', "eFN_PWRCFG Write Successful");
      }
      else
      {
         DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
      }
   }
   else
   {
      DBG_logPrintf('E', "eFN_PWRCFG Write Failed, Value Out of Range");
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_LastGaspMaxNumAttempts
 *
 * Purpose: Returns the Maximum number of last gasps attempts
 *
 * Arguments: None
 *
 * Returns: uint8_t Maximum number of last gasps attempts
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t PWRCFG_get_LastGaspMaxNumAttempts(void)
{
   uint8_t uLastGaspMaxNumAttempts; // Stores the maximum number of last gasp attempts

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uLastGaspMaxNumAttempts = pwrConfigFileData.uLastGaspMaxNumAttempts;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return(uLastGaspMaxNumAttempts);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_LastGaspMaxNumAttempts
 *
 * Purpose: Sets the Maximum number of last gasps attempts
 *
 * Arguments: uint8_t Maximum number of last gasps attempts
 *
 * Returns: Status
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_LastGaspMaxNumAttempts(uint8_t uLastGaspMaxNumAttempts)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE; // Success/failure

   if (uLastGaspMaxNumAttempts <= LASTGASP_MAXIMUM_ATTEMPTS)
   {
      OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails

      pwrConfigFileData.uLastGaspMaxNumAttempts = uLastGaspMaxNumAttempts;
      retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
      OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

      if (eSUCCESS == retVal)
      {
         DBG_logPrintf('I', "eFN_PWRCFG Write Successful");
      }
      else
      {
         DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
      }
   }
   else
   {
      DBG_logPrintf('E', "eFN_PWRCFG Write Failed, Value Out of Range(0-6)");
   }

   return(retVal);
}

#if ( LAST_GASP_SIMULATION == 1 )
/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_SimulateLastGaspStart
 *
 * Purpose: Returns the Last Gasp Simulation start time
 *
 * Arguments: None
 *
 * Returns: uint32_t Start time of the LG Simulation in seconds
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint32_t PWRCFG_get_SimulateLastGaspStart(void)
{
   uint32_t uSimulateLastGaspStart; // Last Gasp Simulation start time

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uSimulateLastGaspStart = pwrConfigFileData.SimLGStartTime;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return(uSimulateLastGaspStart);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_SimulateLastGaspStart
 *
 * Purpose: Sets the Simulation start time and initiates the Simulation process if the
            passed time value is not in the past. If the value is in the past the simulation
            is cancelled.
 *
 * Arguments: uint32_t uSimulateLastGaspStart - Start time for the LG Simualtion
 *
 * Returns: Success/Failure
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_SimulateLastGaspStart( uint32_t uSimulateLastGaspStart )
{
   returnStatus_t          retVal; // Success/failure
   TIMESTAMP_t             sTime;
   sysTime_t               simLGSysTime;
   sysTime_dateFormat_t    dt;

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   pwrConfigFileData.SimLGStartTime = uSimulateLastGaspStart;
   retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   if (eSUCCESS != retVal)
   {
      DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
   }

   ( void )TIME_UTIL_GetTimeInSecondsFormat( &sTime );

   if ( !TIME_SYS_IsTimeValid() )
   {
      ( void )EVL_LGSimStart( TIME_SYS_MAX_SEC );     // Need to set alarm to allow scheduling when valid time occurs
   }
   // Check for valid time to start the simulation or cancel the simulation
   else if ( sTime.seconds > uSimulateLastGaspStart )
   {
      DBG_logPrintf('E', "Simulation start time in past");
      EVL_LGSimCancel();
   }
   else
   {
      ( void )EVL_LGSimStart( uSimulateLastGaspStart );
      TIME_UTIL_ConvertSecondsToSysFormat( uSimulateLastGaspStart, 0, &simLGSysTime);
      ( void )TIME_UTIL_ConvertSysFormatToDateFormat( &simLGSysTime, &dt );

      DBG_logPrintf('E', "The Simulation will start at: %04u/%02u/%02u %02u:%02u:%02u for a duration of %d Seconds\n", dt.year, dt.month, dt.day,
                                                                     dt.hour, dt.min, dt.sec, PWRCFG_get_SimulateLastGaspDuration() );
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_SimulateLastGaspDuration
 *
 * Purpose: Returns the duration of the LG Simulation
 *
 * Arguments: None
 *
 * Returns: uint16_t - duration of the simulation in seconds
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint16_t PWRCFG_get_SimulateLastGaspDuration(void)
{
   uint16_t uSimulateLastGaspDuration;

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uSimulateLastGaspDuration = pwrConfigFileData.SimLGDuration;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return(uSimulateLastGaspDuration);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_SimulateLastGaspDuration
 *
 * Purpose: Sets the duration of the LG Simulation
 *
 * Arguments: uint1_t uSimulateLastGaspDuration - Duration of the simulation in seconds
 *
 * Returns: Status
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_SimulateLastGaspDuration(uint16_t uSimulateLastGaspDuration)
{
   returnStatus_t retVal = eAPP_INVALID_VALUE; // Success/failure

   // Check for valid duration time
   if ( ( LASTGASP_SIM_MAX_DURATION_SECONDS >= uSimulateLastGaspDuration ) && ( LASTGASP_SIM_MIN_DURATION_SECONDS <= uSimulateLastGaspDuration ))
   {
      OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails

      pwrConfigFileData.SimLGDuration = uSimulateLastGaspDuration;
      retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
      OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

      if (eSUCCESS == retVal)
      {
         DBG_logPrintf('I', "eFN_PWRCFG Write Successful");
      }
      else
      {
         DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
      }
   }
   else
   {
      DBG_logPrintf('E', "eFN_PWRCFG Write Failed, SimulateLastGaspDuration range is 20Secs - 7200Secs ");
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_SimulateLastGaspTraffic
 *
 * Purpose: Returns if the traffic is allowed during the LG Simulation or not
 *
 * Arguments: None
 *
 * Returns: uint8_t - traffic is switched ON/OFF for LG Simulation
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t PWRCFG_get_SimulateLastGaspTraffic( void )
{
   uint8_t uSimulateLastGaspTraffic;

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uSimulateLastGaspTraffic = pwrConfigFileData.SimLGTraffic;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return(uSimulateLastGaspTraffic);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_SimulateLastGaspTraffic
 *
 * Purpose: Sets the SimulateLastGaspTraffic parameter
 *
 * Arguments: uint8_t value of the SimulateLastGaspTraffic parameter
 *
 * Returns: Status
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_SimulateLastGaspTraffic( uint8_t uSimulateLastGaspTraffic )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE; // Success/failure

   /* Only allow, updates to the traffic parameter when LG sim is in the disabled and armed state.
      Otherwise, would loose MAC parameters if traffic parameter changed from true to false during the simulation */
   if ( eSimLGArmed >= EVL_GetLGSimState() )
   {
      // Check for data validity SimLGTraffic
      if ( uSimulateLastGaspTraffic <= 1 )
      {
         OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails

         pwrConfigFileData.SimLGTraffic = uSimulateLastGaspTraffic;
         retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
         OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

         if (eSUCCESS == retVal)
         {
            DBG_logPrintf('I', "eFN_PWRCFG Write Successful");
         }
         else
         {
            DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
         }
      }
      else
      {
         DBG_logPrintf('E', "eFN_PWRCFG Write Failed, range of SimulateLastGaspTraffic is 0-1");
      }
   }
   else
   {
       DBG_logPrintf('E', "SimulateLastGaspTraffic cannot be changed as LG simulation is in progress");
       retVal = eFAILURE;
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_SimulateLastGaspMaxNumAttempts
 *
 * Purpose: Returns the Maximum number of last gasps attempts in the simulation
 *
 * Arguments: None
 *
 * Returns: uint8_t Maximum number of last gasps attempts
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t PWRCFG_get_SimulateLastGaspMaxNumAttempts( void )
{
   uint8_t uSimulateLastGaspMaxNumAttempts; // Stores the maximum number of last gasp attempts

   OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails
   uSimulateLastGaspMaxNumAttempts = pwrConfigFileData.SimLGMaxAttempts;
   OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

   return(uSimulateLastGaspMaxNumAttempts);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_set_SimulateLastGaspMaxNumAttempts
 *
 * Purpose: Sets the Maximum number of last gasps attempts for the LG Simulation
 *
 * Arguments: uint8_t Maximum number of last gasps attempts
 *
 * Returns: Status
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_set_SimulateLastGaspMaxNumAttempts( uint8_t uSimulateLastGaspMaxNumAttempts )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE; // Success/failure

   if ( LASTGASP_SIM_MAX_ATTEMPTS >= uSimulateLastGaspMaxNumAttempts )
   {
      OS_MUTEX_Lock(&pwrConfigMutex); // Function will not return if it fails

      pwrConfigFileData.SimLGMaxAttempts = uSimulateLastGaspMaxNumAttempts;
      retVal = FIO_fwrite(&pwrConfigFileHandle, 0, (uint8_t*) &pwrConfigFileData, (lCnt)sizeof(pwrConfigFileData));
      OS_MUTEX_Unlock(&pwrConfigMutex); // Function will not return if it fails

      if (eSUCCESS == retVal)
      {
         DBG_logPrintf('I', "eFN_PWRCFG Write Successful");
      }
      else
      {
         DBG_logPrintf('E', "eFN_PWRCFG Write Failed");
      }
   }
   else
   {
      DBG_logPrintf('E', "eFN_PWRCFG Write Failed, Value Out of Range(0-6)");
   }

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: PWRCFG_get_SimulateLastGaspStats
 *
 * Purpose: Returns the pointer to the structure of the LG Simulation statistics
 *
 * Arguments: None
 *
 * Returns: void* Pointer to the structure of the LG Simulation statistics
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void* PWRCFG_get_SimulateLastGaspStats( void )
{
   return(&simLGStatistics[0]);
}
#endif

/***********************************************************************************************************************

   Function Name: PWRCFG_RestorationDeclarationDelayHandler( action, id, value, attr )

   Purpose: Get/Set restorationDeclarationDelay

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc(715,PWRCFG_RestorationDeclarationDelayHandler) argument not referenced. */
returnStatus_t PWRCFG_RestorationDeclarationDelayHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(pwrConfigFileData.uRestorationDelaySeconds) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint16_t *)value = PWRCFG_get_restoration_delay_seconds();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.uRestorationDelaySeconds);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = PWRCFG_set_restoration_delay_seconds(*(uint16_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: PWRCFG_PowerQualityEventDurationHandler( action, id, value, attr )

   Purpose: Get/Set powerQualityEventDuration

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc(715,PWRCFG_PowerQualityEventDurationHandler) argument not referenced. */
returnStatus_t PWRCFG_PowerQualityEventDurationHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(pwrConfigFileData.uPowerQualityEventDuration) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint16_t *)value = PWRCFG_get_PowerQualityEventDuration();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.uPowerQualityEventDuration);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = PWRCFG_set_PowerQualityEventDuration(*(uint16_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched

/***********************************************************************************************************************

   Function Name: PWRCFG_LastGaspMaxNumAttemptsHandler( action, id, value, attr )

   Purpose: Get/Set LastGaspMaxNumAttempts

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t PWRCFG_LastGaspMaxNumAttemptsHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(pwrConfigFileData.uLastGaspMaxNumAttempts) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = PWRCFG_get_LastGaspMaxNumAttempts();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.uLastGaspMaxNumAttempts);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = PWRCFG_set_LastGaspMaxNumAttempts(*(uint8_t *)value);
   }
   return retVal;
}

#if ( LAST_GASP_SIMULATION == 1 )
/***********************************************************************************************************************
 *
 * Function name:  PWRCFG_SIMLG_OR_PM_Handler
 *
 * Purpose: Handles over the air parameter reading and writing for End Device Details.
 *
 * Arguments: enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t PWRCFG_SIMLG_OR_PM_Handler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case simulateLastGaspStart :
         {
            if ( MAX_OR_PM_PAYLOAD_SIZE  >= sizeof(pwrConfigFileData.SimLGStartTime) ) //lint !e506 !e774
            {
               *(uint32_t *)value = PWRCFG_get_SimulateLastGaspStart();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.SimLGStartTime);
                  attr->rValTypecast = (uint8_t)dateTimeValue;
               }
            }            break;
         }
         case simulateLastGaspDuration:
         {
            if ( sizeof(pwrConfigFileData.SimLGDuration) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
            {
               *(uint16_t *)value = PWRCFG_get_SimulateLastGaspDuration();
               retVal = eSUCCESS;
               if ( NULL != attr )
               {
                  attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.SimLGDuration);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case simulateLastGaspTraffic:
         {
            if ( MAX_OR_PM_PAYLOAD_SIZE >= sizeof(pwrConfigFileData.SimLGTraffic) ) //lint !e506 !e774
            {
               *(uint8_t *)value = PWRCFG_get_SimulateLastGaspTraffic();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.SimLGTraffic);
                  attr->rValTypecast = (uint8_t)Boolean;
               }
            }
            break;
         }
         case simulateLastGaspMaxNumAttempts:
         {
            if ( MAX_OR_PM_PAYLOAD_SIZE >= sizeof(pwrConfigFileData.SimLGMaxAttempts) ) //lint !e506 !e774
            {
               *(uint8_t *)value = PWRCFG_get_SimulateLastGaspMaxNumAttempts();
               retVal = eSUCCESS;
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(pwrConfigFileData.SimLGMaxAttempts);
                  attr->rValTypecast = (uint8_t)uintValue;
               }
            }
            break;
         }
         case simulateLastGaspStatCcaAttempts:
         {
            uint16_t uSimulateLastGaspStatCcaAttempts[LASTGASP_SIM_MAX_ATTEMPTS];
            uint8_t i;

            if ( MAX_OR_PM_PAYLOAD_SIZE >= sizeof(uSimulateLastGaspStatCcaAttempts) ) //lint !e506 !e774
            {
               for( i = 0; i < LASTGASP_SIM_MAX_ATTEMPTS; i++ )
               {
                  uSimulateLastGaspStatCcaAttempts[i] = simLGStatistics[i].CCA_Attempts;
               }
               (void)memcpy((uint8_t *)value, uSimulateLastGaspStatCcaAttempts, sizeof(uSimulateLastGaspStatCcaAttempts));
               if ( NULL != attr )
               {
                  attr->rValLen = (uint16_t)sizeof(uSimulateLastGaspStatCcaAttempts);
                  attr->rValTypecast = (uint8_t)uint16_list_type;
                  retVal = eSUCCESS;
               }
            }
         }
         break;
         case simulateLastGaspStatPPersistAttempts:
         {
            uint16_t uSimulateLastGaspStatPPersistAttempts[LASTGASP_SIM_MAX_ATTEMPTS];
            uint8_t i;

            if ( MAX_OR_PM_PAYLOAD_SIZE >= sizeof(uSimulateLastGaspStatPPersistAttempts) ) //lint !e506 !e774
            {

               for( i = 0; i < LASTGASP_SIM_MAX_ATTEMPTS; i++ )
               {
                  uSimulateLastGaspStatPPersistAttempts[i] = simLGStatistics[i].pPersistAttempts;
               }
               (void)memcpy((uint8_t *)value, uSimulateLastGaspStatPPersistAttempts, sizeof(uSimulateLastGaspStatPPersistAttempts));
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(uSimulateLastGaspStatPPersistAttempts);
                  attr->rValTypecast = (uint8_t)uint16_list_type;
                  retVal = eSUCCESS;
               }
            }
         }
         break;
         case simulateLastGaspStatMsgsSent:
         {
            uint16_t uSimulateLastGaspStatMsgsSent[LASTGASP_SIM_MAX_ATTEMPTS];
            uint8_t i;

            if ( MAX_OR_PM_PAYLOAD_SIZE >= sizeof(uSimulateLastGaspStatMsgsSent) ) //lint !e506 !e774
            {
               for( i = 0; i < LASTGASP_SIM_MAX_ATTEMPTS; i++ )
               {
                  uSimulateLastGaspStatMsgsSent[i] = simLGStatistics[i].MessageSent;
               }
               (void)memcpy((uint8_t *)value, uSimulateLastGaspStatMsgsSent, sizeof(uSimulateLastGaspStatMsgsSent));
               if (attr != NULL)
               {
                  attr->rValLen = (uint16_t)sizeof(uSimulateLastGaspStatMsgsSent);
                  attr->rValTypecast = (uint8_t)uint16_list_type;
                  retVal = eSUCCESS;
               }
            }
         }
         break;
         default :
         {
            retVal = eAPP_NOT_HANDLED; // Success/failure
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   else /* Used to "put" the variable. */
   {

      switch ( id )  /*lint !e788 not all enums used within switch */
      {
         case simulateLastGaspStart:
         {
            retVal = PWRCFG_set_SimulateLastGaspStart( *(uint32_t *)value );

            if ( eSUCCESS != retVal )
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         case simulateLastGaspDuration:
         {
            retVal = PWRCFG_set_SimulateLastGaspDuration( *(uint16_t *)value );

            if ( eSUCCESS != retVal )
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         case simulateLastGaspTraffic:
         {
            retVal = PWRCFG_set_SimulateLastGaspTraffic( *(uint8_t *)value );

            if ( eSUCCESS != retVal )
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         case simulateLastGaspMaxNumAttempts:
         {
            retVal = PWRCFG_set_SimulateLastGaspMaxNumAttempts( *(uint8_t *)value );

            if ( eSUCCESS != retVal )
            {
               retVal = eAPP_INVALID_VALUE;
            }
            break;
         }
         default :
         {
            retVal = eAPP_NOT_HANDLED; // Success/failure
            break;
         }
      }  /*lint !e788 not all enums used within switch */
   }
   return(retVal);
}
#endif
