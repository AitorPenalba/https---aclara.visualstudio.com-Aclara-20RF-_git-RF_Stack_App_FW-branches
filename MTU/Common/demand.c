/******************************************************************************
 *
 * Filename: Demand.c
 *
 * Global Designator: DEMAND_
 *
 * Contents: Demand Subroutines
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2004-2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/
/* INCLUDE FILES */
#include "project.h"
#include "demand.h"
#include "sys_busy.h"
#include "buffer.h"
#include "time_sys.h"
#include "time_util.h"
#include "timer_util.h"
#include "file_io.h"
#include "DBG_SerialDebug.h"
#include "intf_cim_cmd.h"
#include "pack.h"
#include "APP_MSG_Handler.h"
#include "ID_intervalTask.h"
#include "heep.h"
#include "historyd.h"
#include "time_DST.h"
#include "EVL_event_log.h"
#if ( DEMAND_IN_METER == 1 )
#include "hmc_demand.h"
#endif
#include "mode_config.h"

/* TYPE DEFINITIONS */
/* Valid Demand Configurations */
/*lint -esym(751, demandConfig_e) may not be referenced */
typedef enum
{
   DMD_CONFIG_INVALID = (uint8_t)0,
   DMD_60_MIN_FIXED   = 50,
   DMD_30_MIN_FIXED   = 51,
   DMD_15_MIN_FIXED   = 53,
   DMD_10_MIN_FIXED   = 54,
   DMD_05_MIN_FIXED   = 55,
   DMD_60_MIN_30_ROLL = 57,
   DMD_60_MIN_20_ROLL = 58,
   DMD_60_MIN_15_ROLL = 59,
   DMD_60_MIN_10_ROLL = 61,
   DMD_60_MIN_05_ROLL = 63,
   DMD_30_MIN_15_ROLL = 65,
   DMD_30_MIN_10_ROLL = 66,
   DMD_30_MIN_05_ROLL = 68,
   DMD_15_MIN_05_ROLL = 71,
   DMD_10_MIN_05_ROLL = 74
}demandConfig_e; /*lint !e751 may not be referenced */

typedef struct
{
   uint8_t  config;
   uint8_t  normalizer;
   uint8_t  subIntervals;
   uint32_t intervalPeriod;
   uint32_t subIntervalPeriod;
}demandConfig_t;

typedef struct
{
   uint32_t val[NUM_OF_DELTAS];      //Array of deltas used to calcualte the peak demand
}deltas_t;

typedef struct
{
   uint8_t   config;                //Demand Present Config
   uint8_t   futureConfig;          //Demand Future Config
   uint8_t   index;                 //Index to Circular Buffer
   struct
   {
      uint8_t   resetLockOut      : 1;  //Reset Command Lockout active indicator, True/False
      uint8_t   drListconfigError : 1;  //Reading Failed in the drReadList, True/False
      uint8_t   unused            : 6;  //Unused bits
   }bits;
   uint32_t  resetTimeout;          //Reset Timeout interval in seconds
   uint8_t   resetCount;            //Demand Reset Count
   uint8_t   scheduledDmdResetDate; //Day of Month to perform Demand Reset
   sysTime_t lastScdDmdReset;       //Date/Time of scheduled Demand Reset (UTC)
   uint64_t  energyPrev;            //Prev KWh used to get the delta
   uint64_t  dateTimeAtIndex;       //Date and time at the Demand Index
   peak_t    peak;                  //Demand Peak and date/time at the peak
   peak_t    peakPrev;              //Demand Previous Peak and date/time at the peak
   peak_t    peakDaily;             //Demand Daily Peak and date/time at the peak
   peak_t    peakDailyPrev;         //Demand Daily Peak and date/time at the peak
   deltas_t  deltas;                //Demand Circular Buffer
   uint16_t  dailyFlags;            //Each bit indicate one of the delta
   uint16_t  monthlyFlags;          //Each bit indicate one of the delta
}demandFileData_t;                  //File for the demand module

typedef struct
{
   meterReadingType rdList[DMD_MAX_READINGS];
}drList_t;  //Source for each channel

/* FILE VARIABLE DEFINITIONS */
static FileHandle_t  demandFileHndl_;              //Contains the file handle information
static FileHandle_t  drListHndl_;                  //Contains the file handle information
static uint8_t       timeAdjusted_= 0;             //Initialized at power up
static OS_MUTEX_Obj  dmdMutex_;                    //Serialize access to this module
static OS_MSGQ_Obj   mQueueHandle_;                //Message Queue Handle
static demandFileData_t demandFile_;               //Demand params
static drList_t      drList_;                      //Read list
static volatile returnStatus_t dmdResetResult_;    //Result of demand reset
static bool          bModuleBusy_ = (bool)false;   //Indicates module is busy or idle
static uint8_t       schDmdRstAlarmId_ = NO_ALARM_FOUND; //The scheduled Demand Reset alarm
#if ( DEMAND_IN_METER == 1 )
static volatile bool dmdResetDone_;                //Indicates demand reset completed
#endif

/* MACRO DEFINITIONS */
#if( RTOS_SELECTION == FREE_RTOS )
#define DEMAND_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define DEMAND_NUM_MSGQ_ITEMS 0 
#endif
// make sure the size of the reading Quality is still 1 bytes for this application
#define READING_QTY_MAX_SIZE                 25                       // Worst case size of the ReadingQty field for this application
#define DEMAND_PER_BIN_IN_TICKS              (TIME_TICKS_PER_5MIN)
#define DEMAND_RUN_TOLERANCE_IN_TICKS        ((uint32_t)17500)
#define DEMAND_INVALID_VALUE                 ((uint32_t)0xFFFFFFFF)   //Demand invalid
#define DEMAND_MAX_VALUE                     ((uint32_t)9999999)      //Max demand val in deci-watts
#define DEMAND_FILE_UPDATE_RATE_SEC          (SECONDS_PER_MINUTE * 5) //Updates every 5 minutes
#define DR_LIST_FILE_UPDATE_RATE             ((uint32_t)0xFFFFFFFF)   //Infrequent updates

/* Demand Interval and subInterval Periods */
#define DMD_00_MIN_INTERVAL   ((uint16_t)0)
#define DMD_05_MIN_INTERVAL   ((uint16_t)TIME_TICKS_PER_MIN * 5)
#define DMD_10_MIN_INTERVAL   ((uint16_t)TIME_TICKS_PER_MIN * 10)
#define DMD_15_MIN_INTERVAL   ((uint16_t)TIME_TICKS_PER_MIN * 15)
#define DMD_20_MIN_INTERVAL   ((uint16_t)TIME_TICKS_PER_MIN * 20)
#define DMD_30_MIN_INTERVAL   ((uint16_t)TIME_TICKS_PER_MIN * 30)
#define DMD_60_MIN_INTERVAL   ((uint16_t)TIME_TICKS_PER_MIN * 60)

/* Number of Demand subIntervals */
#define DMD_00_SUBINTERVALS   ((uint8_t)0)   //Block demand
#define DMD_01_SUBINTERVALS   ((uint8_t)1)
#define DMD_02_SUBINTERVALS   ((uint8_t)2)
#define DMD_03_SUBINTERVALS   ((uint8_t)3)
#define DMD_06_SUBINTERVALS   ((uint8_t)6)
#define DMD_12_SUBINTERVALS   ((uint8_t)12)

/* Multiplier to normalize the Demand value to an hour period */
#define DMD_00_MIN_MULTIPLIER ((uint8_t)0)
#define DMD_05_MIN_MULTIPLIER ((uint8_t)12)
#define DMD_10_MIN_MULTIPLIER ((uint8_t)6)
#define DMD_15_MIN_MULTIPLIER ((uint8_t)4)
#define DMD_30_MIN_MULTIPLIER ((uint8_t)2)
#define DMD_60_MIN_MULTIPLIER ((uint8_t)1)

#if ( DEMAND_IN_METER == 0 )
   /* Default Demand Configuration Index (15 Minute Fixed Block) */
   #define DMD_CONFIG_DEFAULT           (DMD_15_MIN_FIXED)
   #define DMD_CONFIG_INVALID_INDEX     ((uint8_t)0)
#else
   /* Default Demand Configuration Index (Invalid config) */
   #define DMD_CONFIG_DEFAULT           (DMD_CONFIG_INVALID)
   #define DMD_CONFIG_INVALID_INDEX     ((uint8_t)0)
#endif

/* Demand Reset Lockout Time in seconds */
#define DMD_LOCKOUT_TIME_DEFAULT          ((uint32_t)86400)
#define DMD_LOCKOUT_TIME_MIN              ((uint32_t)1)
#define DMD_LOCKOUT_TIME_MAX              ((uint32_t)259200)

#define MIN(a,b) (((a)<(b))?(a):(b))

/* CONSTANTS */
//Set for maximum UTC time in sysTimeCombined_t format
#define INVALID_DMD_TIME                  ((sysTimeCombined_t)0)
//Set for the maximum demand reading supported (TODO: Verify against the value in the HEEP)
#define INVALID_DMD_ENERGY                (MAX_UINT_32)
#define DISABLE_SCHED_DMD_RESET_DATE      ((uint8_t) 0)  /* Used for an unassigned Demand Reset Day */
#define MAX_SCHED_DMD_RESET_DATE          ((uint8_t)31)  /* Max calendar date of month */
#define LAST_DMD_RESET_DATE_DEFAULT       ((uint8_t) 1)  /* A calendar date of zero means a periodic alarm */
#define LAST_DMD_RESET_TIME_DEFAULT       ((uint8_t) 0)  /* Used to schedule new date/time */

static demandConfig_t const validDemandConfigs[] =
{// config,             normalizer,            subIntervals,        intervalPeriod,      subIntervalPeriod
   {(uint8_t)DMD_CONFIG_INVALID, DMD_00_MIN_MULTIPLIER, DMD_00_SUBINTERVALS, DMD_00_MIN_INTERVAL, DMD_00_MIN_INTERVAL},
   {(uint8_t)DMD_60_MIN_FIXED,   DMD_60_MIN_MULTIPLIER, DMD_12_SUBINTERVALS, DMD_60_MIN_INTERVAL, DMD_00_MIN_INTERVAL},
   {(uint8_t)DMD_30_MIN_FIXED,   DMD_30_MIN_MULTIPLIER, DMD_06_SUBINTERVALS, DMD_30_MIN_INTERVAL, DMD_00_MIN_INTERVAL},
   {(uint8_t)DMD_15_MIN_FIXED,   DMD_15_MIN_MULTIPLIER, DMD_03_SUBINTERVALS, DMD_15_MIN_INTERVAL, DMD_00_MIN_INTERVAL},
   {(uint8_t)DMD_10_MIN_FIXED,   DMD_10_MIN_MULTIPLIER, DMD_02_SUBINTERVALS, DMD_10_MIN_INTERVAL, DMD_00_MIN_INTERVAL},
   {(uint8_t)DMD_05_MIN_FIXED,   DMD_05_MIN_MULTIPLIER, DMD_01_SUBINTERVALS, DMD_05_MIN_INTERVAL, DMD_00_MIN_INTERVAL},
   {(uint8_t)DMD_60_MIN_30_ROLL, DMD_60_MIN_MULTIPLIER, DMD_12_SUBINTERVALS, DMD_60_MIN_INTERVAL, DMD_30_MIN_INTERVAL},
   {(uint8_t)DMD_60_MIN_20_ROLL, DMD_60_MIN_MULTIPLIER, DMD_12_SUBINTERVALS, DMD_60_MIN_INTERVAL, DMD_20_MIN_INTERVAL},
   {(uint8_t)DMD_60_MIN_15_ROLL, DMD_60_MIN_MULTIPLIER, DMD_12_SUBINTERVALS, DMD_60_MIN_INTERVAL, DMD_15_MIN_INTERVAL},
   {(uint8_t)DMD_60_MIN_10_ROLL, DMD_60_MIN_MULTIPLIER, DMD_12_SUBINTERVALS, DMD_60_MIN_INTERVAL, DMD_10_MIN_INTERVAL},
   {(uint8_t)DMD_60_MIN_05_ROLL, DMD_60_MIN_MULTIPLIER, DMD_12_SUBINTERVALS, DMD_60_MIN_INTERVAL, DMD_05_MIN_INTERVAL},
   {(uint8_t)DMD_30_MIN_15_ROLL, DMD_30_MIN_MULTIPLIER, DMD_06_SUBINTERVALS, DMD_30_MIN_INTERVAL, DMD_15_MIN_INTERVAL},
   {(uint8_t)DMD_30_MIN_10_ROLL, DMD_30_MIN_MULTIPLIER, DMD_06_SUBINTERVALS, DMD_30_MIN_INTERVAL, DMD_10_MIN_INTERVAL},
   {(uint8_t)DMD_30_MIN_05_ROLL, DMD_30_MIN_MULTIPLIER, DMD_06_SUBINTERVALS, DMD_30_MIN_INTERVAL, DMD_05_MIN_INTERVAL},
   {(uint8_t)DMD_15_MIN_05_ROLL, DMD_15_MIN_MULTIPLIER, DMD_03_SUBINTERVALS, DMD_15_MIN_INTERVAL, DMD_05_MIN_INTERVAL},
   {(uint8_t)DMD_10_MIN_05_ROLL, DMD_10_MIN_MULTIPLIER, DMD_02_SUBINTERVALS, DMD_10_MIN_INTERVAL, DMD_05_MIN_INTERVAL}
};

#if ( DEMAND_IN_METER == 1 )
typedef enum
{  // Could be based on Block/INT_LENGTH or Sliding/SUB_INT/INT_MULTIPLIER settings in meter.
   // I210+c and KV2c ONLY support these unambiguous configurations listed
  //NOTE: Values are byte swapped since evaluating as uint16_t
   MTR_DMD_INVALID_CONFIG  = (uint16_t)0x0000,  //Invalid Meter Demand Configuration
   MTR_DMD_05_MIN_INTERVAL = 0x0005,      //Block Demand  5 minute interval
   MTR_DMD_10_MIN_INTERVAL = 0x000A,      //Block Demand 10 minute interval
   MTR_DMD_15_MIN_INTERVAL = 0x000F,      //Block Demand 15 minute interval
   MTR_DMD_30_MIN_INTERVAL = 0x001E,      //Block Demand 30 minute interval
   MTR_DMD_60_MIN_INTERVAL = 0x003C,      //Block Demand 60 minute interval
   MTR_DMD_05_MIN_02_MULT  = 0x0205,      //Rolling Demand  5 minute Sub Interval, Interval Multiplier of 2  (10 minute interval)
   MTR_DMD_05_MIN_03_MULT  = 0x0305,      //Rolling Demand  5 minute Sub Interval, Interval Multiplier of 3  (15 minute interval)
   MTR_DMD_05_MIN_06_MULT  = 0x0605,      //Rolling Demand  5 minute Sub Interval, Interval Multiplier of 6  (30 minute interval)
   MTR_DMD_05_MIN_12_MULT  = 0x0C05,      //Rolling Demand  5 minute Sub Interval, Interval Multiplier of 12 (60 minute interval)
   MTR_DMD_10_MIN_03_MULT  = 0x030A,      //Rolling Demand 10 minute Sub Interval, Interval Multiplier of 3  (30 minute interval)
   MTR_DMD_10_MIN_06_MULT  = 0x060A,      //Rolling Demand 10 minute Sub Interval, Interval Multiplier of 6  (60 minute interval)
   MTR_DMD_15_MIN_02_MULT  = 0x020F,      //Rolling Demand 15 minute Sub Interval, Interval Multiplier of 2  (30 minute interval)
   MTR_DMD_15_MIN_04_MULT  = 0x040F,      //Rolling Demand 15 minute Sub Interval, Interval Multiplier of 4  (60 minute interval)
   MTR_DMD_20_MIN_03_MULT  = 0x0314,      //Rolling Demand 20 minute Sub Interval, Interval Multiplier of 3  (60 minute interval)
   MTR_DMD_30_MIN_02_MULT  = 0x021E       //Rolling Demand 30 minute Sub Interval, Interval Multiplier of 2  (60 minute interval)
}meterDemandConfig_e;

typedef struct
{
   meterDemandConfig_e  meterDmdConfig;
   demandConfig_e       demandConfigReading;
}meterDmdCfgLookup_t;
#endif

/*ST13 Valid Entries for SRFN (Rdg Value is for demandPresentConfiguration/demandFutureConfiguration):
    Meter   RdgVal   BLOCK/SLIDING  (Sub)Intv Len  Multiplier
   0x0500     55         BLOCK           5           N/A
   0x0A00     54         BLOCK           10          N/A
   0x0F00     53         BLOCK           15          N/A
   0x1E00     51         BLOCK           30          N/A
   0x3C00     50         BLOCK           60          N/A
   0x0502     74        SLIDING          5            2
   0x0503     71        SLIDING          5            3
   0x0506     68        SLIDING          5            6
   0x050C     63        SLIDING          5            12
   0x0A03     66        SLIDING          10           3
   0x0A06     61        SLIDING          10           6
   0x0F02     65        SLIDING          15           2
   0x0F04     59        SLIDING          15           4
   0x1403     58        SLIDING          20           3
   0x1E02     57        SLIDING          30           2
*/

#if ( DEMAND_IN_METER == 1 )
#define MTR_DMD_INVALID_CFG_INDEX ((uint8_t)0)

/*lint -esym(749,meterDemandCfgLookup::*) */
static meterDmdCfgLookup_t const meterDemandCfgLookup[] =
{  //Meter Demand Config    demandPresentConfiguration/demandFutureConfiguration
  {MTR_DMD_INVALID_CONFIG,  DMD_CONFIG_INVALID},
  {MTR_DMD_05_MIN_INTERVAL, DMD_05_MIN_FIXED},
  {MTR_DMD_10_MIN_INTERVAL, DMD_10_MIN_FIXED},
  {MTR_DMD_15_MIN_INTERVAL, DMD_15_MIN_FIXED},
  {MTR_DMD_30_MIN_INTERVAL, DMD_30_MIN_FIXED},
  {MTR_DMD_60_MIN_INTERVAL, DMD_60_MIN_FIXED},
  {MTR_DMD_05_MIN_02_MULT,  DMD_10_MIN_05_ROLL},
  {MTR_DMD_05_MIN_03_MULT,  DMD_15_MIN_05_ROLL},
  {MTR_DMD_05_MIN_06_MULT,  DMD_60_MIN_05_ROLL},
  {MTR_DMD_05_MIN_12_MULT,  DMD_30_MIN_05_ROLL},
  {MTR_DMD_10_MIN_03_MULT,  DMD_30_MIN_10_ROLL},
  {MTR_DMD_10_MIN_06_MULT,  DMD_60_MIN_10_ROLL},
  {MTR_DMD_15_MIN_02_MULT,  DMD_30_MIN_15_ROLL},
  {MTR_DMD_15_MIN_04_MULT,  DMD_60_MIN_15_ROLL},
  {MTR_DMD_20_MIN_03_MULT,  DMD_60_MIN_20_ROLL},
  {MTR_DMD_30_MIN_02_MULT,  DMD_60_MIN_30_ROLL}
};
#endif

/* FUNCTION PROTOTYPES */
static void getConfigParams(uint8_t config, demandConfig_t *configParams);
#if ( DEMAND_IN_METER == 1 )
static void meterDemandCfgToReadingType ( uint8_t *RdgTypeCfg, meterDemandConfig_e  meterDmdCfg );
#endif
static void normalizeDemand(uint8_t config, uint32_t *demandValue);
static int64_t getValuein_dWatts(int64_t value, uint8_t powerOf10);
static void processDemandEntry( sysTimeCombined_t currentTime, uint64_t currentEnergy, tTimeSysMsg const *pTimeMsg );
static void startDmdResetLockOutTmr( void );
static void resetTmrCallBack_ISR( uint8_t cmd, uint8_t const *pData );
static returnStatus_t setSchDmdReset( void );
static void demandResetLogEvent( returnStatus_t status, LoggedEventDetails *eventInfo );

/* FUNCTION DEFINITIONS */
/***********************************************************************************************************************
 *
 * Function name: DEMAND_init
 *
 * Purpose: Initialize demand module
 *
 * Arguments: None
 *
 * Returns: returnStatus_t - result of file open.
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t DEMAND_init( void )
{
   FileStatus_t     fileStatusCfg;                /* Contains the file status */
   returnStatus_t   retVal = eFAILURE;

   if ( OS_MUTEX_Create(&dmdMutex_) && OS_MSGQ_Create(&mQueueHandle_, DEMAND_NUM_MSGQ_ITEMS) )
   {
      if (eSUCCESS == FIO_fopen(  &demandFileHndl_,                /* File Handle */
                                  ePART_SEARCH_BY_TIMING,          /* Search for the best paritition according to the timing. */
                                  (uint16_t)eFN_DEMAND,            /* File ID (filename) */
                                  (lCnt)sizeof(demandFileData_t),  /* Size of the data in the file. */
                                  0,                               /* File attributes to use. */
                                  DEMAND_FILE_UPDATE_RATE_SEC,     /* The update rate of the data in the file. */
                                  &fileStatusCfg) )
      {
         if (fileStatusCfg.bFileCreated)
         {
            (void)memset(&demandFile_, 0, sizeof(demandFile_)); //clear the file
            //Set defaults
            demandFile_.config                        = (uint8_t)DMD_CONFIG_DEFAULT; //NIC:fixed 15 minute default,MTR:Invalid
            demandFile_.futureConfig                  = (uint8_t)DMD_CONFIG_DEFAULT; //NIC:fixed 15 minute default,MTR:Invalid
            demandFile_.resetTimeout                  = DMD_LOCKOUT_TIME_DEFAULT;    //Default Demand Reset lockout time in seconds
            demandFile_.peak.dtReset                  = INVALID_DMD_TIME;
            demandFile_.peak.dateTime                 = INVALID_DMD_TIME;
            demandFile_.peakPrev.dtReset              = INVALID_DMD_TIME;
            demandFile_.peakPrev.dateTime             = INVALID_DMD_TIME;
            demandFile_.peakDaily.dtReset             = INVALID_DMD_TIME;
            demandFile_.peakDaily.dateTime            = INVALID_DMD_TIME;
            demandFile_.peakDailyPrev.dtReset         = INVALID_DMD_TIME;
            demandFile_.peakDailyPrev.dateTime        = INVALID_DMD_TIME;
            demandFile_.lastScdDmdReset.date          = LAST_DMD_RESET_DATE_DEFAULT;
            demandFile_.lastScdDmdReset.time          = LAST_DMD_RESET_TIME_DEFAULT;
            demandFile_.bits.drListconfigError        = (bool)false;
            demandFile_.bits.resetLockOut             = (bool)false;
            demandFile_.scheduledDmdResetDate         = DISABLE_SCHED_DMD_RESET_DATE; //Scheduled deamand Reset depricated
            //TODO: Excise code related to demand reset

            retVal = FIO_fwrite(&demandFileHndl_, 0, (uint8_t*)&demandFile_, (lCnt)sizeof(demandFile_));
            timeAdjusted_ |= DEMAND_VIRGIN_POWER_UP;
         }
         else
         {  //Read the file
            retVal = FIO_fread(&demandFileHndl_, (uint8_t *)&demandFile_, 0, (lCnt)sizeof(demandFile_));
            timeAdjusted_ |= DEMAND_POWER_UP;

         }
         if (eSUCCESS == retVal)
         { //Scheduled deamand Reset depricated
           // we must disable this in all units we patch to
            if( demandFile_.scheduledDmdResetDate != DISABLE_SCHED_DMD_RESET_DATE )
            {
               demandFile_.scheduledDmdResetDate         = DISABLE_SCHED_DMD_RESET_DATE;
               retVal = FIO_fwrite(&demandFileHndl_, 0, (uint8_t*)&demandFile_, (lCnt)sizeof(demandFile_));
            }
         }
         if (eSUCCESS == retVal)
         {  //Open and set the DR List
            if (eSUCCESS == FIO_fopen(  &drListHndl_,                    /* File Handle */
                                        ePART_SEARCH_BY_TIMING,          /* Search for the best paritition according to the timing. */
                                        (uint16_t)eFN_DR_LIST,           /* File ID (filename) */
                                        (lCnt)sizeof(drList_t),          /* Size of the data in the file. */
                                        0,                               /* File attributes to use. */
                                        DR_LIST_FILE_UPDATE_RATE,        /* The update rate of the data in the file. */
                                        &fileStatusCfg) )
            {
               if (fileStatusCfg.bFileCreated)
               {
                  (void)memset(&drList_, 0, sizeof(drList_)); //clear the file
                  //Set defaults
#if ( DEMAND_IN_METER == 0 )
                  drList_.rdList[0] = demandResetCount;
                  drList_.rdList[1] = maxPrevFwdDmdKW;
                  drList_.rdList[2] = dailyMaxFwdDmdKW;
#else
                  drList_.rdList[0] = maxPresFwdDmdKW;
                  drList_.rdList[1] = maxPrevFwdDmdKW;
                  drList_.rdList[2] = demandResetCount;
#endif
                  retVal = FIO_fwrite(&drListHndl_, 0, (uint8_t*)&drList_, (lCnt)sizeof(drList_));
               }
               else
               {  //Read the file
                  retVal = FIO_fread(&drListHndl_, (uint8_t *)&drList_, 0, (lCnt)sizeof(drList_));
               }
            }
         }
      }
   }
   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: DMD_task
 *
 * Purpose: Demand task. Calculates the Demand related quantities.
 *
 * Arguments: uint32_t Arg0
 *
 * Returns: None
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
//lint -esym(715,Arg0)  // Arg0 required for generic API, but not used here.
void DEMAND_task(uint32_t Arg0)
{
#if ( DEMAND_IN_METER == 0 )
   tTimeSysPerAlarm  alarmSettings; //Configure the periodic alarm for time

   //Add the timer to wake every 5 minutes on a 5 minute boundary (shortest valid bin time)
   (void)memset(&alarmSettings, 0, sizeof(alarmSettings));
   alarmSettings.bOnValidTime    = true;                       //Alarmed on valid time
   alarmSettings.bSkipTimeChange = true;                       //Notify only on time boundary
   alarmSettings.pMQueueHandle   = &mQueueHandle_;             //Use the message Queue
   alarmSettings.ulPeriod        = DEMAND_PER_BIN_IN_TICKS;    //Sample rate
   (void)TIME_SYS_AddPerAlarm(&alarmSettings);
#else //Need to determine meter configuration
   //ST 11 Offset 0, Length 1 BLOCK_DEMAND_FLAG (bit 2) and SLIDING_DEMAND_FLAG (bit 3) (req'd? since entries in ST13 are unique between BLOCK and SLIDING)
   //ST 11 Offset 2, Length 1 NBR_DEMAND_CTRL_ENTRIES (at ST 13 Offset 4, Length 2 each) (req'd? since I210+c and KV2c are fixed at 5)
   //Read ST 13 Offset 4, Length 2 INTERVAL_VALUE[0] (Read first element only since I210+c and KV2c require all channels to be programmed identically)
   {
      uint16_t  demandConfig;
      while ( !HMC_MTRINFO_Ready() )
      {
         OS_TASK_Sleep( 100 );
      }

      if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getDemandCfg ( &demandConfig ) )
      {
         meterDemandCfgToReadingType ( &demandFile_.config, (meterDemandConfig_e)demandConfig );
         demandFile_.futureConfig = demandFile_.config;  //Always make same for Demand in Meter
         (void)FIO_fwrite(&demandFileHndl_, 0, (uint8_t *)&demandFile_, (lCnt)sizeof(demandFile_));
      }
   }

#endif
   if ( true == demandFile_.bits.resetLockOut )
   {   //If the lockout was active at power down, restart timer
      startDmdResetLockOutTmr();
   }
   //Need to handle ScheduledDemandReset if enabled.
   if ( (DISABLE_SCHED_DMD_RESET_DATE != demandFile_.scheduledDmdResetDate) &&
        (MAX_SCHED_DMD_RESET_DATE     >= demandFile_.scheduledDmdResetDate) )
   {
      if ( LAST_DMD_RESET_DATE_DEFAULT == demandFile_.lastScdDmdReset.date )
      {  // No previously scheduled date/time so set the default date/time
         (void)setSchDmdReset();
      }
      else
      {  //Set alarm to previous date
         tTimeSysCalAlarm     calAlarm;

         (void)memset(&calAlarm, 0, sizeof(calAlarm));            // Clr cal settings, only set what we need.
         calAlarm.pMQueueHandle   = &mQueueHandle_;               // Using message queue
         //  Set new alarm
         calAlarm.ulAlarmDate     = demandFile_.lastScdDmdReset.date;           // Set the alarm date
         calAlarm.ulAlarmTime     = demandFile_.lastScdDmdReset.time;           // Set the alarm time to be same as daily shift time
         calAlarm.bSkipTimeChange = true;                         // Don't notify on an time change
         calAlarm.bUseLocalTime   = true;
         if (eSUCCESS == TIME_SYS_AddCalAlarm(&calAlarm) )        /* Set the alarm */
         {
            schDmdRstAlarmId_ = calAlarm.ucAlarmId;  // Alarm ID will be set after adding alarm.
         }
         else
         {
            //TODO: Should this set an Event? Only happens if there are too many alarms set
            DBG_logPrintf ('E', "Unable to add Scheduled Demand Reset Date");
         }
      }
   }

   for (;;) /* Task Loop */
   {
      int64_t           value = 0;
      sysTimeCombined_t currentTime;         //current date and time
      uint32_t          timeSinceBoundary;   //milliseconds since demand boundary
      tTimeSysMsg const *pTimeMsg;           //Pointer to time message
      buffer_t         *pBuf;                //Buffer used to point to the data in the msg queue

      (void)OS_MSGQ_Pend( &mQueueHandle_, (void *)&pBuf, OS_WAIT_FOREVER );
      switch (pBuf->eSysFmt)  /*lint !e788 not all enums used within switch */
      {
         case eSYSFMT_TIME:   //If message from periodic timer
         {
            pTimeMsg = (tTimeSysMsg *)( void * )&pBuf->data[0];
            /* Alarm ID matches AND the cause of the alarm valid?  */
            if ( (schDmdRstAlarmId_ == pTimeMsg->alarmId) &&
                 (TIME_SYS_ALARM == pTimeMsg->bCauseOfAlarm) )
            {
               SYSBUSY_setBusy();
               bModuleBusy_ = (bool)true;
               //Reset demand only if last scheduled date was real, i.e. NOT the default
               if ( LAST_DMD_RESET_DATE_DEFAULT != demandFile_.lastScdDmdReset.date )
               {
                  if (false == demandFile_.bits.resetLockOut)
                  {  //Lockout not active, perform demand reset.
#if ( DEMAND_IN_METER == 1 )
                     DEMAND_ResetInMeter();
#else
                     DEMAND_ResetInModule();
#endif
                     demandResetLogEvent( dmdResetResult_, NULL );
                     if ( eSUCCESS == dmdResetResult_ )
                     {
                        startDmdResetLockOutTmr();
                     }
                  }
                  else
                  {  //demand reset lockout is active - Log Event
                     demandResetLogEvent( eFAILURE, NULL );
                  }
               }
               (void)setSchDmdReset();
               bModuleBusy_ = (bool)false;
               SYSBUSY_clearBusy();
            }
            else  //This is a demand interval
            {
               SYSBUSY_setBusy();
               bModuleBusy_ = (bool)true;
               while ( ID_isModuleBusy() )
               {  //Load profile busy, wait for it to be done
                  OS_TASK_Sleep(50);
               }
               //Check if within limit to process demand calculations
               (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
               timeSinceBoundary = currentTime % DEMAND_PER_BIN_IN_TICKS;
               if (timeSinceBoundary > DEMAND_RUN_TOLERANCE_IN_TICKS) //TODO:Use demandIntervalClockAllowance configuration
               {
                  DBG_printf("Skipping demand. Time since 5 minutes boundary=%lu", timeSinceBoundary);
               }
               else
               {
                  ValueInfo_t  reading;
                  currentTime -= timeSinceBoundary; //find the nearest demand boundary

                  //Read the host meter for the current fwdkWh value
                  if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( fwdkWh, &reading ) )
                  {
                     //process the new demand value
                     value = getValuein_dWatts(reading.Value.intValue, reading.power10); //Convert the value units to deci-Watts
                     DBG_printf("Current value read for demand(in deci-watts)=%lli", value);
                     processDemandEntry(currentTime, (uint64_t)value, pTimeMsg);    //Update demand, if needed
                  }
               }
               bModuleBusy_ = (bool)false;
               SYSBUSY_clearBusy();
            }
            break;
         }
         case eSYSFMT_DMD_TIMEOUT:  //Message from demand reset lockout timer
         {
            OS_MUTEX_Lock(&dmdMutex_); //Mutex to ensure a shift doesn't conflict // Function will not return if it fails
            demandFile_.bits.resetLockOut = false;
            (void)FIO_fwrite(&demandFileHndl_, (uint16_t)offsetof(demandFileData_t, bits),
                             (uint8_t *)&demandFile_.bits, (lCnt)sizeof(demandFile_.bits));
            OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
            break;
         }
         default:
         {
            break;
         }
      }  /*lint !e788 Not all possible cases are handled within the default case */
      BM_free(pBuf);
   }
}
//lint +esym(715,Arg0)  // Arg0 required for generic API, but not used here.

/***********************************************************************************************************************
 *
 * Function Name: DEMAND_isModuleBusy()
 *
 * Purpose: Returns if module is busy or idle
 *
 * Arguments: None
 *
 * Returns: bool - true, if busy, false otherwise
 *
 * Side Effects: None
 *
 * Reentrant Code: yes
 *
 **********************************************************************************************************************/
bool DEMAND_isModuleBusy( void )
{
   return bModuleBusy_;
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_ResetMsg
 *
 * Purpose: Processes the HEEP Application message for Demand Reset
 *
 * Arguments:
 *     APP_MSG_Rx_t *appMsgInfo - Pointer to application header structure from head end
 *     void *payloadBuf         - pointer to data payload (in this case a ExchangeWithID structure)
 *     length                   - payload length
 *
 * Returns: none
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
 *     payloadBuf will be deallocated by the calling routine.
 *
 **********************************************************************************************************************/
//lint -esym(715, payloadBuf) not referenced
//lint -efunc(818, DEMAND_ResetMsg) arguments cannot be const because of API requirements
void DEMAND_ResetMsg(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   (void)length;
   HEEP_APPHDR_t           appMsgTxInfo;  // Application header/QOS info
   ValueInfo_t             reading;
   EventRdgQty_u           qtys;          // For EventTypeQty and ReadingTypeQty
   EDEventType             edEventType;   // End Device Event Type
   union nvpQtyDetValSz_u  nvpQtyDetValSz;// Name/ValuePairQty, EDEventDetailValueSize
   ExWId_ReadingInfo_u     readingInfo;   // Reading Info for the reading Type and Value
   pack_t                  packCfg;       // Struct needed for PACK functions
   buffer_t                *pBuf;         // pointer to message
   sysTimeCombined_t       currentTime;   // Get current time
   uint32_t                utcTime;       // UTC time in seconds since epoch
   uint32_t                IntvEndTime;   // Values Interval End UTC time in seconds since epoch
   uint32_t                fracSeconds;   // Used in conversion from sysTime_t to seconds since epoch
   meterReadingType        readingType;   // Reading Type
   uint16_t                indexOfEvtRdgQty = VALUES_INTERVAL_END_SIZE; // Index for next Event/Reading Qty field
   uint8_t                 i;             // loop index
   bool                    drReadListFailed  = (bool)false;  //Keeps track of failed readings in drReadList
   bool                    needReadingHeader = (bool)true;  /* Used to force "mini-header" after coincident values. */
   uint8_t                 txMethodStatus; // Copy of TX Method Status
#if ( DEMAND_IN_METER == 1 )
   stdTbl21_t             *pst21;
#endif
   LoggedEventDetails      eventInfo;

   // initialize the reading data structure
   (void)memset( (uint8_t *)&reading, 0, sizeof(reading) );

   if (  MODECFG_get_rfTest_mode() == 0 )
   {
      //Application header/QOS info (cannot set Method_Status yet)
      HEEP_initHeader( &appMsgTxInfo );
      appMsgTxInfo.TransactionType    = (uint8_t)TT_RESPONSE;
      appMsgTxInfo.Resource           = (uint8_t)appMsgRxInfo->Resource;
      appMsgTxInfo.Req_Resp_ID        = (uint8_t)appMsgRxInfo->Req_Resp_ID;
      appMsgTxInfo.qos                = (uint8_t)appMsgRxInfo->qos;
      appMsgTxInfo.Method_Status      = (uint8_t)OK;
      appMsgTxInfo.callback           = NULL;
      appMsgTxInfo.TransactionContext = appMsgRxInfo->TransactionContext;
      txMethodStatus                  = appMsgTxInfo.Method_Status;

      if ( method_post == (enum_MessageMethod)appMsgRxInfo->Method_Status ||
           method_get  == (enum_MessageMethod)appMsgRxInfo->Method_Status )
      { //Parse the packet payload - an implied ExchangeWithID bit structure
         if (method_post == (enum_MessageMethod)appMsgRxInfo->Method_Status)
         {  //Perform demand reset
            dmdResetResult_ = eFAILURE;
            if (false == demandFile_.bits.resetLockOut)
            {  //Lockout not active, perform demand reset.
               SYSBUSY_setBusy();
               bModuleBusy_ = (bool)true;
#if ( DEMAND_IN_METER == 1 )
               DEMAND_ResetInMeter();
#else
               DEMAND_ResetInModule();
#endif
               bModuleBusy_ = (bool)false;
               SYSBUSY_clearBusy();
               if ( eSUCCESS == dmdResetResult_ )
               {
                  startDmdResetLockOutTmr();
               }
               else if( eAPP_DMD_RST_MTR_LOCKOUT == dmdResetResult_ )
               {
                  appMsgTxInfo.Method_Status = (uint8_t)Conflict;
               }
               else if( eAPP_DMD_RST_REQ_TIMEOUT == dmdResetResult_ )
               {
                  appMsgTxInfo.Method_Status = (uint8_t)ServiceUnavailable;
               }
               else
               {  //Demand Reset failed
                  appMsgTxInfo.Method_Status = (uint8_t)InternalServerError;
               }
            }
            else
            {  //demand reset lockout is active, say so and log Event
               appMsgTxInfo.Method_Status = (uint8_t)Conflict;
            }

            demandResetLogEvent( dmdResetResult_, &eventInfo );
            txMethodStatus = appMsgTxInfo.Method_Status; /* Keep a copy of method Status */
         }

         if( ( (uint8_t)InternalServerError == appMsgTxInfo.Method_Status ) ||
             ( (uint8_t)ServiceUnavailable  == appMsgTxInfo.Method_Status )    )
         {
            (void)HEEP_MSG_TxHeepHdrOnly(&appMsgTxInfo);
             ERR_printf("ERROR: DMD Reset Procedure Failed");
         }
         else
         {
             /* Build FullMeterRead response, allocate a buffer big enough to accomadate worst case payload.
                Need to account for:
                   Number of FullMeterRead structs to send all readings; include a eventHdr_t for each demand.
                   Assume all demand readings have MAX_COIN_PER_DEMAND associated coincident values.
                   At most, 2 events associated with demand.
             */
             pBuf = BM_alloc(APP_MSG_MAX_DATA); //Allocate max buffer
             if ( pBuf != NULL )
             {
                PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );

                //Values Interval End using the current time ("at this time demand reset was...")
                (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
                IntvEndTime = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
                PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&IntvEndTime, &packCfg );
                pBuf->x.dataLen = VALUES_INTERVAL_END_SIZE;

                qtys.EventRdgQty = 0;   //Clear EDEvent and Readings Qty's
                //Insert the Event & Reading Quantity, it will be adjusted later to the correct value
                PACK_Buffer( sizeof(qtys) * 8, (uint8_t *)&qtys.EventRdgQty, &packCfg );
                pBuf->x.dataLen += sizeof(qtys);
                needReadingHeader = (bool)false; /* Just added a "mini-header" */

                if ( (uint8_t)InternalServerError != appMsgTxInfo.Method_Status &&
                     method_post == (enum_MessageMethod)appMsgRxInfo->Method_Status)
                {
                   if ( (uint8_t)Conflict == appMsgTxInfo.Method_Status )
                   {
                      //Pack the ED Event createdTime ("at this time the reset failed")
                      //IntvEndTime same as above
                      PACK_Buffer( -(ED_EVENT_CREATED_DATE_TIME_SIZE * 8), (uint8_t *)&IntvEndTime, &packCfg );
                      pBuf->x.dataLen += ED_EVENT_CREATED_DATE_TIME_SIZE;

                      //Pack the ED Event Type
                      //Lockout was active so include the demand reset failed event
                      edEventType = electricMeterDemandResetFailed;
                      PACK_Buffer(-(int16_t)(sizeof(edEventType) * 8), (uint8_t *)&edEventType, &packCfg);
                      pBuf->x.dataLen += sizeof(edEventType);

                      //Pack the NVPQty and EDEventDetailValueSize
                      nvpQtyDetValSz.nvpQtyDetValSz = 0;
                      nvpQtyDetValSz.bits.nvpQty = 1;
                      nvpQtyDetValSz.bits.DetValSz = sizeof(eventInfo.indexValue);
                      PACK_Buffer( (sizeof(nvpQtyDetValSz) * 8), (uint8_t *)&nvpQtyDetValSz, &packCfg );
                      pBuf->x.dataLen += sizeof(nvpQtyDetValSz);

                      //Pack the alarm index name
                      PACK_Buffer( -(int16_t)(sizeof(eventInfo.alarmIndex) * 8), (uint8_t *)&eventInfo.alarmIndex, &packCfg );
                      pBuf->x.dataLen += sizeof(eventInfo.alarmIndex);

                      //Pack the alarm index ID value
                      PACK_Buffer( (int16_t)(sizeof(eventInfo.indexValue) * 8), (uint8_t *)&eventInfo.indexValue, &packCfg );
                      pBuf->x.dataLen += sizeof(eventInfo.indexValue);

                      //Increment the EDEventQty
                      ++qtys.bits.eventQty;      /* Report reset lock out event.   */
                   }
                   //Pack the ED Event createdTime (when did the reset occur? time demand reset did the shift)

    #if ( DEMAND_IN_METER == 1 )
                   ( void ) DEMAND_GetDmdValue( demandResetCount, &reading ); /* Reset occured, time IS present.  */
                   /* Special Case for Demand Only meter. Add the demand reset time saved by the Module, since Demand Only meter does not have time */
                   ( void )HMC_MTRINFO_getStdTable21( &pst21 );
                   if ( ( 0 == reading.cimInfo.timePresent ) && ( 0 == pst21->dateTimeFieldFlag ))
                   {
                      utcTime = demandFile_.peakPrev.dtReset;
                   }
                   else
                   {
                      TIME_UTIL_ConvertSysFormatToSeconds( ( sysTime_t * )&reading.timeStamp, &utcTime, &fracSeconds );
                   }
    #else
                   utcTime = demandFile_.peakPrev.dtReset;
    #endif

                   //Include the previous Reset regardless of lockout state
                   if( (uint8_t)Conflict == appMsgTxInfo.Method_Status )
                   {  // if we had a conflict, we need to log another alarm for the previous successful demand reset
                      demandResetLogEvent( eSUCCESS, &eventInfo );
                   }

                   //Pack the time stamp for the event
                   PACK_Buffer( -(VALUES_INTERVAL_END_SIZE * 8), (uint8_t *)&utcTime, &packCfg );
                   pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;

                   //Pack the ED Event Type
                   edEventType = electricMeterDemandResetOccurred;
                   PACK_Buffer(-(int16_t)(sizeof(edEventType) * 8), (uint8_t *)&edEventType, &packCfg);
                   pBuf->x.dataLen += sizeof(edEventType);

                   //Pack the NVPQty and EDEventDetailValueSize
                   nvpQtyDetValSz.bits.nvpQty = 1;
                   nvpQtyDetValSz.bits.DetValSz = sizeof(eventInfo.indexValue);
                   PACK_Buffer( (sizeof(nvpQtyDetValSz) * 8), (uint8_t *)&nvpQtyDetValSz, &packCfg );
                   pBuf->x.dataLen += sizeof(nvpQtyDetValSz);

                   //Pack the alarm index name
                   PACK_Buffer( -(int16_t)(sizeof(eventInfo.alarmIndex) * 8), (uint8_t *)&eventInfo.alarmIndex, &packCfg );
                   pBuf->x.dataLen += sizeof(eventInfo.alarmIndex);

                   //Pack the alarm index ID value
                   PACK_Buffer( (int16_t)(sizeof(eventInfo.indexValue) * 8), (uint8_t *)&eventInfo.indexValue, &packCfg );
                   pBuf->x.dataLen += sizeof(eventInfo.indexValue);

                   //Increment the EDEventQty
                   ++qtys.bits.eventQty;         /* Also report demand reset occurred event. */
                }
                //else Not Modified. Error performing demand reset.

                //Add all the readings from dr read list
                //lint -e{850} intentional manipulation of i within the for loop
                for ( i = 0; i < DMD_MAX_READINGS; i++ )
                {
                   uint8_t        powerOf10Code;
                   uint8_t        numberOfBytes;
                   enum_CIM_QualityCode qualCode = CIM_QUALCODE_SUCCESS;

                   if ( 0 == ( uint16_t )drList_.rdList[i] )
                   {
                      continue;
                   }
                   // Make sure if we can fit the worst case scenario
                   if ( pBuf->bufMaxSize >= ( pBuf->x.dataLen + VALUES_INTERVAL_END_SIZE + sizeof( qtys ) + READING_QTY_MAX_SIZE + reading.cimInfo.coincidentCount * READING_QTY_MAX_SIZE) )
                   {
                      if ( CIM_QUALCODE_SUCCESS == INTF_CIM_CMD_getMeterReading( drList_.rdList[i], &reading ) )
                      {
                         /* Since there could be more than 15 entries, check if 15 are already present, or if the next reading
                         has associated coincident values. */
                         if ( needReadingHeader                    ||
                         ( READING_QTY_MAX == qtys.bits.rdgQty )   ||
                         ( appMsgTxInfo.Method_Status == (uint8_t)PartialContent ) ||
                         ( ( reading.cimInfo.coincidentCount != 0 ) && ( qtys.bits.rdgQty != 0) )  )
                         {
                            appMsgTxInfo.Method_Status   = txMethodStatus;   // Copy the original method status. Will be Updated later if required.
                            pBuf->data[indexOfEvtRdgQty] = qtys.EventRdgQty; // Update the current set
                            //start new set - repeat valuesInterval.end
                            //Values Interval End using the date/time previously included
                            PACK_Buffer( -( VALUES_INTERVAL_END_SIZE * 8 ), ( uint8_t * )&IntvEndTime, &packCfg );
                            pBuf->x.dataLen += VALUES_INTERVAL_END_SIZE;
                            //Initialize EDEvent and Readings Qty's, it will be adjusted later to the correct value
                            qtys.EventRdgQty = 0;
                            PACK_Buffer( sizeof( qtys ) * 8, ( uint8_t * )&qtys.EventRdgQty, &packCfg );
                            indexOfEvtRdgQty = pBuf->x.dataLen;
                            pBuf->x.dataLen += sizeof( qtys );
                            needReadingHeader = false;
                         }
                         ++qtys.bits.rdgQty;

                         powerOf10Code = HEEP_getPowerOf10Code( reading.power10, (int64_t *)&reading.Value.intValue );
                         numberOfBytes = HEEP_getMinByteNeeded( reading.Value.intValue, reading.typecast, reading.valueSizeInBytes );

                         INFO_printf( "DMD Reset Ch=%2d, val=%lld, power=%d, NumBytes=%d", i, reading.Value.intValue,
                                     powerOf10Code, numberOfBytes );

                         //Set and pack the reading info
                         ( void )memset( &readingInfo.bytes[0], 0, sizeof( readingInfo.bytes ) ); //Clear all parameters
                         readingInfo.bits.pendPowerof10 = powerOf10Code;
                         readingInfo.bits.tsPresent = reading.cimInfo.timePresent;

                         // value was read successfully, check to see if it was and information code
                         if( INTF_CIM_CMD_errorCodeQCRequired( drList_.rdList[i], reading.Value.intValue ) )
                         {  // reading an informational code so we need to pack a quality code in the message
                            readingInfo.bits.RdgQualPres = true;
                            qualCode = CIM_QUALCODE_ERROR_CODE;
                         }

                         if ( reading.cimInfo.coincidentCount != 0 )
                         {
                            readingInfo.bits.isCoinTrig = true;
                         }

                         readingInfo.bits.typecast      = ( uint16_t )reading.typecast;

                         readingInfo.bits.RdgValSize    = numberOfBytes & RDG_VAL_SIZE_L_MASK;
                         readingInfo.bits.RdgValSizeU   = ( numberOfBytes & RDG_VAL_SIZE_U_MASK ) >> RDG_VAL_SIZE_U_SHIFT;
                         PACK_Buffer(( sizeof( readingInfo.bytes ) * 8 ), &readingInfo.bytes[0], &packCfg);
                         pBuf->x.dataLen += sizeof( readingInfo.bytes );

                         //Pack the reading type
                         readingType = reading.readingType;
                         PACK_Buffer(-(int16_t)( sizeof( readingType ) * 8 ), ( uint8_t * )&readingType, &packCfg);
                         pBuf->x.dataLen += sizeof( readingType );

                         if ( true ==  readingInfo.bits.tsPresent )
                         {
                            //add the timestamp
                            uint32_t seconds;
                            TIME_UTIL_ConvertSysFormatToSeconds( ( sysTime_t * )&reading.timeStamp, &seconds, &fracSeconds );
                            PACK_Buffer(-(int16_t)( sizeof( seconds  ) * 8 ), ( uint8_t * )&seconds, &packCfg);
                            pBuf->x.dataLen += sizeof( seconds );
                         }

                         if( readingInfo.bits.RdgQualPres )
                         {  // a reading quality code was observed, pack into the response
                            PACK_Buffer( READING_QUALITY_SIZE_BITS, (uint8_t *)&qualCode, &packCfg );
                            pBuf->x.dataLen += READING_QUALITY_SIZE;
                         }

                         //Pack the reading value
                         PACK_Buffer(-( numberOfBytes * 8 ), ( uint8_t * )&reading.Value.uintValue, &packCfg);
                         pBuf->x.dataLen += numberOfBytes;

                         // we will resuse the reading structure for coincident values, save the coincident count
                         uint8_t coincidentCount = reading.cimInfo.coincidentCount;

                         /* Add coincident values   */
                         for ( uint8_t coinCnt = 0; coinCnt < coincidentCount; coinCnt++ )
                         {
                            needReadingHeader = true;
                            ( void )INTF_CIM_CMD_getDemandCoinc( drList_.rdList[i], coinCnt, &reading );

                            powerOf10Code = HEEP_getPowerOf10Code( reading.power10, ( int64_t * )&reading.Value.intValue );
                            numberOfBytes = HEEP_getMinByteNeeded( reading.Value.intValue, reading.typecast, reading.valueSizeInBytes );

                            //Set and pack the reading info
                            readingInfo.bits.tsPresent = false;
                            readingInfo.bits.isCoinTrig = false;
                            readingInfo.bits.pendPowerof10 = powerOf10Code;
                            readingInfo.bits.RdgValSize    = numberOfBytes & RDG_VAL_SIZE_L_MASK;
                            readingInfo.bits.RdgValSizeU   = ( numberOfBytes & RDG_VAL_SIZE_U_MASK ) >> RDG_VAL_SIZE_U_SHIFT;
                            PACK_Buffer(( sizeof( readingInfo.bytes ) * 8 ), &readingInfo.bytes[0], &packCfg);
                            pBuf->x.dataLen += sizeof( readingInfo.bytes );

                            //Pack the reading type
                            PACK_Buffer(-(int16_t)( sizeof( reading.readingType ) * 8 ), ( uint8_t * )&reading.readingType, &packCfg);
                            pBuf->x.dataLen += sizeof( reading.readingType );

                            //Pack the reading value
                            PACK_Buffer(-( numberOfBytes * 8 ), ( uint8_t * )&reading.Value.uintValue, &packCfg);
                            pBuf->x.dataLen += numberOfBytes;

                            ++qtys.bits.rdgQty;
                         }
                      }
                      else   // If Reading wasn't successful
                      {
                         drReadListFailed = (bool)true;   //Set the flag indicating one or more reading Types in the list failed to get the value from the Meter.
                         EventKeyValuePair_s  keyVal;     /* Event key/value pair info  */
                         EventData_s          event;      /* Event info  */

                         //Generate event comDeviceCofigurationError, 212, and pass drReadList as a Name/Value Pair.
                         event.eventId                 = (uint16_t)comDeviceConfigurationError;       // EndDeviceEvents.EndDeviceEventType in HEEP Document
                         *(uint16_t *)&keyVal.Key[0]   = (uint16_t)drReadList;                        /*lint !e826 !e2445 */ // Value is drReadList Reading Type
                         (void)memcpy( keyVal.Value, &drList_.rdList[i], sizeof( drList_.rdList[0] ) );
                         event.markSent = (bool)false;
                         event.eventKeyValuePairsCount = 1;
                         event.eventKeyValueSize       = sizeof( drList_.rdList[0] );

                         (void)EVL_LogEvent( 100, &event, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );           // Log an Event
                      }
                   }
                   else // If the message is bigger
                   {
                      // Set Method Status to PartialContent
                      appMsgTxInfo.Method_Status = (uint8_t)PartialContent;
                      //Insert the corrected number of events and readings
                      pBuf->data[indexOfEvtRdgQty] = qtys.EventRdgQty;
                      // send the message to message handler. The called function will free the buffer
                      (void)HEEP_MSG_Tx (&appMsgTxInfo, pBuf);
                      i--;    // in order to retain the reading type which we were not able to fit in the buffer
                      pBuf = BM_alloc(APP_MSG_MAX_DATA); //Allocate max buffer for the remaining message.
                      if ( pBuf != NULL )
                      {
                         // Initialize the new Buffer data
                         PACK_init( (uint8_t*)&pBuf->data[0], &packCfg );
                         qtys.EventRdgQty = 0;
                         pBuf->x.dataLen = 0;
                      }
                      else
                      {   //The response buffer could not be allocated
                          appMsgTxInfo.Method_Status   = (uint8_t)ServiceUnavailable;
                          (void)HEEP_MSG_TxHeepHdrOnly(&appMsgTxInfo);
                          ERR_printf("ERROR: DMD Reset No buffer available");
                      }
                   }
                } // End of for loop

                //Check for the ConfigurartionError event flag
                OS_MUTEX_Lock(&dmdMutex_); //Mutex to ensure a shift doesn't conflict // Function will not return if it fails
                // Check if the values of the are different.
                if( (drReadListFailed ) != (bool)(demandFile_.bits.drListconfigError) )
                {
                   // Toggle the value of the flag
                   demandFile_.bits.drListconfigError = !demandFile_.bits.drListconfigError;
                   // Update in the NV
                   (void)FIO_fwrite(&demandFileHndl_, (uint16_t)offsetof(demandFileData_t, bits),
                                     (uint8_t *)&demandFile_.bits, (lCnt)sizeof(demandFile_.bits));

                   //Check the previous value of the flag. Send event only if it is cleared now.
                   if(!demandFile_.bits.drListconfigError)
                   {
                      EventKeyValuePair_s  keyVal;     /* Event key/value pair info  */
                      EventData_s          event;      /* Event info  */
                      //Generate event comDeviceCofigurationErrorCleared, 213, and pass drReadList as a Name/Value Pair.
                      event.eventId                 = (uint16_t)comDeviceConfigurationErrorCleared;  /* EndDeviceEvents.EndDeviceEventType in HEEP Document */
                      *(uint16_t *)&keyVal.Key[0]   = (uint16_t)drReadList;                          /* Key is drReadList *//*lint !e826 !e2445 area too small */
                      *(uint16_t *)&keyVal.Value[0] = 0;                                             /*lint !e826 !e2445 area too small */
                      event.markSent                = (bool)false;
                      event.eventKeyValuePairsCount = 1;
                      event.eventKeyValueSize       = 0;
                      (void)EVL_LogEvent( 100, &event, &keyVal, TIMESTAMP_NOT_PROVIDED, NULL );      /* Log an Event */
                   }
                }
                OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
                if ( pBuf != NULL )
                {
                   //Insert the corrected number of events and readings
                   pBuf->data[indexOfEvtRdgQty] = qtys.EventRdgQty;
                   // send the message to message handler. The called function will free the buffer
                   (void)HEEP_MSG_Tx (&appMsgTxInfo, pBuf);
                }
             }
             else
             {  //The response buffer could not be allocated
                appMsgTxInfo.Method_Status   = (uint8_t)ServiceUnavailable;
                (void)HEEP_MSG_TxHeepHdrOnly(&appMsgTxInfo);
                ERR_printf("ERROR: DMD Reset No buffer available");
             }
         } //End of else for if status == Internal Server Error
      }//End of if ( method_put == ...

      else
      {  //Wrong Method
         //Return header only with status to indicate the request was not perofrmed
         appMsgTxInfo.Method_Status = (uint8_t)BadRequest;
         (void)HEEP_MSG_TxHeepHdrOnly(&appMsgTxInfo);
      }
   }
}
//lint +esym(715, payloadBuf) not referenced

#if ( DEMAND_IN_METER == 1 )
/******************************************************************************

   Function Name: DEMAND_ResetCallback

   Purpose: Called upon completion (or failure) of demand reset command

   Arguments:  result - success/failure of procedure.

   Returns: none

   Notes:

******************************************************************************/
static void DEMAND_ResetCallback( returnStatus_t result )
{
   dmdResetDone_ = (bool)true;
   dmdResetResult_ = result;
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_ResetInMeter
 *
 * Purpose: Calls API in HMC to perform demand reset. Waits for demand reset to complete.
 *          The calling function must set/start the lockout timer.
 *
 * Arguments: void
 *
 * Returns: void
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
  *
  ****************************************************************************************************************** */
void DEMAND_ResetInMeter( void )
{
   sysTimeCombined_t currentTime;   // Get current time

   dmdResetDone_ = (bool)false;
   /*lint -e{611} cast of DEMAND_ResetCallback is correct   */
   (void)HMC_DMD_Reset( (uint8_t)HMC_APP_API_CMD_ACTIVATE, (void *)DEMAND_ResetCallback ); //Request a demand reset
   //wait for demand reset to complete
   for( ; !dmdResetDone_; ) //Can be changed to event based
   {
      OS_TASK_Sleep( 250 );
   }

   //Store the demand reset date/time
   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
   currentTime = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
   demandFile_.peakPrev.dtReset = (uint32_t)currentTime;    /* UTC time when reset occurred */
   (void)FIO_fwrite(&demandFileHndl_, 0, (uint8_t *)&demandFile_, (lCnt)sizeof(demandFile_));
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}
#else //( DEMAND_IN_METER = 1 )

/***********************************************************************************************************************
 *
 * Function name: DEMAND_ResetInModule
 *
 * Purpose: Clears the Demand table and current peak, date and time.
 *          Prior to calling this function, verify there is no lock-out and time is valid.
 *          The following operations are performed:
 * 	  1. Copy Demand Present,          to Demand Previous
 *    2. Copy Demand Date/Time Present to Demand Date/Time Previous
 *    3. Set  Demand Present           to zero
 *    4. Set  Demand Date/Time Present to INVALID_TIME
 *    5. Increment Demand Reset Count by one
 *    6. If Demand Future Configuration is different from Demand Present Configuration, then
 *         Copy Demand Future Configuration     to Demand Present Configuration
 *         Copy Daily Demand Maximum            to Daily Demand Previous
 *         Copy Daily Demand Maximum Date/Time  to Daily Demand Previous Date/Time
 *         Set  Daily Demand Maximum to zero
 *         Set  DailyDemand Date/Time to INVALID_TIME
 *
 *    The calling function must set/start the lockout timer.
 *
 * Arguments: void
 *
 * Returns: void
 *
 * Side Effects: Updates the demand related registers described above
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 ****************************************************************************************************************** */
void DEMAND_ResetInModule( void )
{
   sysTimeCombined_t currentTime;   // Get current time

   dmdResetResult_ = eFAILURE;
   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   demandFile_.monthlyFlags = 0;
   (void)memcpy(&demandFile_.peakPrev, &demandFile_.peak, sizeof(demandFile_.peakPrev));  /* Copy current to previous */
   normalizeDemand(demandFile_.config, &demandFile_.peakPrev.energy);
   (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
   currentTime = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
   demandFile_.peakPrev.dtReset = (uint32_t)currentTime;              /* UTC time when reset occurred */
   demandFile_.peak.energy      = 0;                                  /* Clear the current        */
   demandFile_.peak.dateTime    = INVALID_DMD_TIME;                   /* Set to invalid time      */
   //If the configuration changed, then also force the Daily Demand to reset
   if ( demandFile_.config != demandFile_.futureConfig )
   {
      /* Shift daily now since the configuration changed.
         Calling DEMAND_Shift_Daily_Peak() would require 2 FIO calls */
      (void)memcpy(&demandFile_.peakDailyPrev, &demandFile_.peakDaily, sizeof(demandFile_.peakDailyPrev));  /* Copy current to previous */
      normalizeDemand(demandFile_.config, &demandFile_.peakDailyPrev.energy);
      demandFile_.peakDailyPrev.dtReset = (uint32_t)currentTime;                        /* UTC time when reset occurred */
      demandFile_.peakDaily.energy      = 0;                                            /* Clear the current Daily Demand */
      demandFile_.peakDaily.dateTime    = INVALID_DMD_TIME;                             /* Set to invalid time */
      demandFile_.config = demandFile_.futureConfig;
   }
   demandFile_.resetCount++;                        /* Increment Demand Reset Count by one*/
   (void)FIO_fwrite(&demandFileHndl_, 0, (uint8_t *)&demandFile_, (lCnt)sizeof(demandFile_));
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
   dmdResetResult_ = eSUCCESS;
}
#endif

/***********************************************************************************************************************
 *
 * Function name: DEMAND_Current_Daily_Peak
 *
 * Purpose: Returns current daily peak
 *
 * Arguments: peak_t * - pointer to struct to receive the current Daily Demand (deci-Wh in a one hour period)
 *
 * Returns: enum_CIM_QualityCode - status of the Daily demand values
 *
 * Side Effects:
 *
 * Reentrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode DEMAND_Current_Daily_Peak( peak_t *ppeak )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   if (  (INVALID_DMD_TIME   == demandFile_.peakDaily.dateTime) ||
         (INVALID_DMD_ENERGY == demandFile_.peakDaily.energy) )
   {
      retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   }
   else
   {
      (void)memcpy(ppeak, &demandFile_.peakDaily, sizeof(peak_t));
      normalizeDemand(demandFile_.config, &ppeak->energy);
   }
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails

   return (retVal);
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_Shift_Daily_Peak
 *
 * Purpose: Shifts the daily peak demand to the previous daily peak demand. Then set the current
 *          daily demand to zero and its date/time to invalid:
 *          Copy Daily Demand Maximum           to Daily Demand Previous
 *          Copy Daily Demand Maximum Date/Time to Daily Demand Previous Date/Time
 *          Set  Daily Demand Maximum           to zero
 *          Set  DailyDemand Date/Time          to Invalid
 *
 * Arguments: peak_t * - pointer to struct to receive the shifted Daily Demand (deci-Wh in a one hour period)
 *
 * Returns: enum_CIM_QualityCode - status of the Daily demand values
 *
 * Side Effects:
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
enum_CIM_QualityCode DEMAND_Shift_Daily_Peak( peak_t *ppeak )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_SUCCESS;
   uint64_t             currentTime;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   demandFile_.dailyFlags = 0;
   (void)memcpy(&demandFile_.peakDailyPrev, &demandFile_.peakDaily, sizeof(demandFile_.peakDailyPrev));  /* Copy current to previous */
    normalizeDemand(demandFile_.config, &demandFile_.peakDailyPrev.energy);
   (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
   /* UTC time when reset occurred */
   demandFile_.peakDailyPrev.dtReset = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
   demandFile_.peakDaily.energy      = 0;                                            /* Clear the current Daily Demand */
   demandFile_.peakDaily.dateTime    = INVALID_DMD_TIME;                             /* Set to invalid time */

   (void)FIO_fwrite(&demandFileHndl_, 0, (uint8_t *)&demandFile_, (lCnt)sizeof(demandFile_));
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
   (void)memcpy(ppeak, &demandFile_.peakDailyPrev, sizeof(peak_t));

   if (  (INVALID_DMD_TIME   == demandFile_.peakDailyPrev.dateTime) ||
         (INVALID_DMD_ENERGY == demandFile_.peakDailyPrev.energy) )
   {
      retVal = CIM_QUALCODE_KNOWN_MISSING_READ;
   }
   return (retVal);
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_getConfig
 *
 * Purpose: Returns the current configuration
 *
 * Arguments: uint64_t *: Returns the current configuration
 *
 * Returns: None
 *
 * Re-entrant Code: yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void DEMAND_getConfig( uint64_t *demandConfig )
{
   *demandConfig = (uint64_t)DEMAND_GetConfig();
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_GetConfig
 *
 * Purpose: Get Config from Demand configuration.
 *
 * Arguments: None
 *
 * Returns: uint8_t config
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t DEMAND_GetConfig(void)
{
   uint8_t config;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   config = demandFile_.config;
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails

   return(config);
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_GetFutureConfig
 *
 * Purpose: Get demandFutureConfiguration from Demand configuration.
 *
 * Arguments: None
 *
 * Returns: uint8_t uDemandFutureConfiguration
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t DEMAND_GetFutureConfig(void)
{
   uint8_t futureConfig;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   futureConfig = demandFile_.futureConfig;
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails

   return(futureConfig);
}

#if ( DEMAND_IN_METER == 0 )
/***********************************************************************************************************************
 *
 * Function name: DEMAND_SetConfig
 *
 * Purpose: Set demandPresentConfiguration in Demand configuration.
 *
 * Arguments: uint16_t uDemandPresentConfiguration
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t DEMAND_SetConfig(uint8_t config)
{
   returnStatus_t retVal = eFAILURE;

   if ( DEMAND_configValid(config) )
   {
      OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
      //Write the current config
      demandFile_.config = config;
      retVal = FIO_fwrite(&demandFileHndl_, (fileOffset)offsetof(demandFileData_t, config),
                          (uint8_t *)&config, (lCnt)sizeof(demandFile_.config));
      OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
   }
   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_SetFutureConfig
 *
 * Purpose: Set demandFutureConfiguration in Demand configuration.
 *
 * Arguments: uint16_t uDemandFutureConfiguration
 *
 * Returns: returnStatus_t
 *
 * Re-entrant Code: No
 *
 * Notes:
 *
 **********************************************************************************************************************/
returnStatus_t DEMAND_SetFutureConfig(uint8_t futureConfig)
{
   returnStatus_t retVal = eFAILURE;

   if ( DEMAND_configValid(futureConfig) )
   {
      OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
      //Write the future config
      demandFile_.futureConfig = futureConfig;
      retVal = FIO_fwrite(&demandFileHndl_, (fileOffset)offsetof(demandFileData_t,
                           futureConfig), (uint8_t *)&futureConfig, (lCnt)sizeof(demandFile_.futureConfig));
      OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
   }
   return(retVal);
}
#endif

/***********************************************************************************************************************
 *
 * Function name: DEMAND_DumpFile
 *
 * Purpose: Prints the demand data from the file
 *
 * Arguments: None
 *
 * Returns: void
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
 *
 ******************************************************************************************************************** */
void DEMAND_DumpFile( )
{
   /*lint --e{573} signed-unsigned mixed with division   */
   demandFileData_t     file;     /* Contains the entire file content for processing. */
   sysTime_t            sTime;
   sysTime_dateFormat_t sDateTime;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   (void)FIO_fread(&demandFileHndl_, (uint8_t *)&file, 0, (lCnt)sizeof(file));  //Read demand file

#if (DEMAND_IN_METER == 0)
   uint8_t  indexTmp;          /* Index into the array */

   indexTmp = file.index;
   DBG_printf( "Last Delta's %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -11)%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -10)%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -9 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -8 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -7 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -6 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -5 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -4 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -3 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -2 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -1 )%NUM_OF_DELTAS],
               file.deltas.val[(NUM_OF_DELTAS + indexTmp -0 )%NUM_OF_DELTAS]
               );
   DBG_printf( "Daily   %u %u %u %u %u %u %u %u %u %u %u %u",
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -11 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -10 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -9 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -8 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -7 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -6 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -5 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -4 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -3 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -2 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -1 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.dailyFlags >> ((NUM_OF_DELTAS + indexTmp -0 )%NUM_OF_DELTAS) ) & 0x0001 );
   DBG_printf( "Monthly %u %u %u %u %u %u %u %u %u %u %u %u",
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -11 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -10 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -9 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -8 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -7 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -6 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -5 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -4 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -3 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -2 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -1 )%NUM_OF_DELTAS) ) & 0x0001,
               ( file.monthlyFlags >> ((NUM_OF_DELTAS + indexTmp -0 )%NUM_OF_DELTAS) ) & 0x0001 );

   TIME_UTIL_ConvertSysCombinedToSysFormat(&file.peakDaily.dateTime, &sTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sTime, &sDateTime);
   DBG_printf( "Pres Daily Peak=%lu %02u/%02u/%04u %02u:%02u:%02u", file.peakDaily.energy,
               sDateTime.month, sDateTime.day, sDateTime.year, sDateTime.hour, sDateTime.min, sDateTime.sec);
   TIME_UTIL_ConvertSysCombinedToSysFormat(&file.peak.dateTime, &sTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sTime, &sDateTime);
   DBG_printf( "Pres Bill  Peak=%lu %02u/%02u/%04u %02u:%02u:%02u", file.peak.energy,
               sDateTime.month, sDateTime.day, sDateTime.year, sDateTime.hour, sDateTime.min, sDateTime.sec);
   TIME_UTIL_ConvertSysCombinedToSysFormat(&file.peakDailyPrev.dateTime, &sTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sTime, &sDateTime);
   DBG_printf( "Prev Daily Peak=%lu %02u/%02u/%04u %02u:%02u:%02u", file.peakDailyPrev.energy,
               sDateTime.month, sDateTime.day, sDateTime.year, sDateTime.hour, sDateTime.min, sDateTime.sec);
   TIME_UTIL_ConvertSysCombinedToSysFormat(&file.peakPrev.dateTime, &sTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sTime, &sDateTime);
   DBG_printf( "Prev Bill  Peak=%lu %02u/%02u/%04u %02u:%02u:%02u", file.peakPrev.energy,
               sDateTime.month, sDateTime.day, sDateTime.year, sDateTime.hour, sDateTime.min, sDateTime.sec);
#endif
   TIME_UTIL_ConvertSecondsToSysFormat(file.peakPrev.dtReset, 0, &sTime);
   (void)TIME_UTIL_ConvertSysFormatToDateFormat(&sTime, &sDateTime);
   DBG_printf( "Prev Reset=%02u/%02u/%04u %02u:%02u:%02u",
               sDateTime.month, sDateTime.day, sDateTime.year, sDateTime.hour, sDateTime.min, sDateTime.sec);
#if (DEMAND_IN_METER == 0)
   DBG_printf( "Reset Count=%u Index=%u", file.resetCount, indexTmp);
#endif
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function Name: DEMAND_GetTimeout
 *
 * Purpose: Gets the lockout time.
 *
 * Arguments: pointer time value in seconds
 *
 * Returns: None
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
void DEMAND_GetTimeout( uint32_t *timeout )
{
   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   *timeout = demandFile_.resetTimeout;
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function Name: DEMAND_SetTimeout
 *
 * Purpose: Sets the lockout time.
 *
 * Arguments: Time value in seconds
 *
 * Returns: None
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
returnStatus_t DEMAND_SetTimeout( uint32_t timeout )
{
   returnStatus_t retVal = eAPP_INVALID_VALUE;

   if ( ( DMD_LOCKOUT_TIME_MIN <= timeout ) && ( DMD_LOCKOUT_TIME_MAX >= timeout ) )
   {
      OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
      demandFile_.resetTimeout = timeout;
      (void)FIO_fwrite(&demandFileHndl_, (fileOffset)offsetof(demandFileData_t, resetTimeout), (uint8_t *)&timeout,
                        (lCnt)sizeof(demandFile_.resetTimeout));  //Write lockout time to demand file
      OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
      retVal = eSUCCESS;
   }
   return retVal;
}

/***********************************************************************************************************************
 *
 * Function Name: DEMAND_GetResetDay
 *
 * Purpose: Get the date of the month Demand reset occurs.
 *
 * Arguments: None
 *
 * Returns: Day of the Month (disabled or 1-31)
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
void DEMAND_GetResetDay( uint8_t *dateOfMonth )
{
   *dateOfMonth = DISABLE_SCHED_DMD_RESET_DATE;
   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   *dateOfMonth = demandFile_.scheduledDmdResetDate;
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function Name: DEMAND_SetResetDay
 *
 * Purpose: Set the date of the month Demand reset occurs.
 *
 * Arguments: Day of the Month (disable or 1-31)
 *
 * Returns: SUCCESS or failure
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
returnStatus_t DEMAND_SetResetDay( uint8_t dateOfMonth )
{
   returnStatus_t retVal;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   demandFile_.scheduledDmdResetDate = dateOfMonth;
   (void)FIO_fwrite(&demandFileHndl_, (uint16_t)offsetof(demandFileData_t, scheduledDmdResetDate),
                    (uint8_t *)&demandFile_.scheduledDmdResetDate, (lCnt)sizeof(demandFile_.scheduledDmdResetDate));
   demandFile_.lastScdDmdReset.date = LAST_DMD_RESET_DATE_DEFAULT;
   demandFile_.lastScdDmdReset.time = LAST_DMD_RESET_TIME_DEFAULT;
   (void)FIO_fwrite(&demandFileHndl_, (uint16_t)offsetof(demandFileData_t, lastScdDmdReset),
                    (uint8_t *)&demandFile_.lastScdDmdReset, (lCnt)sizeof(demandFile_.lastScdDmdReset));
   OS_MUTEX_Unlock(&dmdMutex_); //Unlock since setSchDmdReset also locks the mutex // Function will not return if it fails
   // Apply new date for alarm
   retVal = setSchDmdReset();

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function Name: DEMAND_GetSchDmdRst
 *
 * Purpose: Get the current scheduled Demand reset date/time.
 *
 * Arguments: None
 *
 * Returns: Date and time of the next scheduled demand reset
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
void DEMAND_GetSchDmdRst( uint32_t *date, uint32_t *time )
{
   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   *date = demandFile_.lastScdDmdReset.date;
   *time = demandFile_.lastScdDmdReset.time;
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_configValid
 *
 * Purpose: Validate demand enum value.
 *
 * Arguments: uint8_t demandConfig
 *
 * Returns: uint8_t
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
uint8_t DEMAND_configValid(uint8_t demandConfig)
{
   uint8_t i;
   uint8_t bValid = false;

   for (i = 0; (i < ARRAY_IDX_CNT(validDemandConfigs)); ++i)
   {
      if (demandConfig == validDemandConfigs[i].config)
      {
         bValid = true;
         break;
      }
   }
   return(bValid);
}

/***********************************************************************************************************************
 *
 * Function name: getConfigParams
 *
 * Purpose:  Find the config by enum, then copy the parameters.  If not found then returns default parameters.
 *
 * Arguments: config        - the demand configuration enumeration
 *            *configParams - pointer to copy the parameters
 *
 * Returns: None
 *
 ******************************************************************************************************************** */
static void getConfigParams(uint8_t config, demandConfig_t *configParams)
{
   uint8_t i;
   uint8_t index;

   index = DMD_CONFIG_INVALID_INDEX;
   for (i=0; i < ARRAY_IDX_CNT(validDemandConfigs); i++)
   {
      if (validDemandConfigs[i].config == config)
      {
         index = i;
         break;
      }
   }
   *configParams = validDemandConfigs[index];
   return;
}

#if ( DEMAND_IN_METER == 1 )
/***********************************************************************************************************************
 *
 * Function name: meterDemandCfgToReadingType
 *
 * Purpose:  Transpose the meter Demand configuration to the SRFN Demand Configuration.  If not found then returns default parameters.
 *
 * Arguments: *RdgTypeCfg - pointer to destination of SRFN Demand Configuration
 *            meterDmdCfg - the demand configuration as read from meter
 *
 * Returns: None
 *
 ******************************************************************************************************************** */
static void meterDemandCfgToReadingType ( uint8_t *RdgTypeCfg, meterDemandConfig_e  meterDmdCfg )
{
   uint8_t i;
   uint8_t index;

   index = MTR_DMD_INVALID_CFG_INDEX;
   for ( i = 0; i < ARRAY_IDX_CNT( meterDemandCfgLookup ); i++ )
   {
      if ( meterDemandCfgLookup[i].meterDmdConfig == meterDmdCfg )
      {
         index = i;
         break;
      }
   }
   /*lint -e{641}  */
   *RdgTypeCfg = meterDemandCfgLookup[index].demandConfigReading;
   return;
}
#endif

/***********************************************************************************************************************
 *
 * Function name: processDemandEntry
 *
 * Purpose: Process the demand value and update the demand file, if needed
 *
 * Arguments: sysTimeCombined_t: Time to use for demand calculations and storage
 *            uint64_t: Value to use for demand calculations and storage
 *            tTimeSysMsg *: Time alarm message
 *
 * Returns: void
 *
 * Side Effects: Updates demand file
 *
 * Reentrant Code: Yes
 *
 * Notes:
 *
 ******************************************************************************************************************** */
static void processDemandEntry( sysTimeCombined_t currentTime, uint64_t currentEnergy, tTimeSysMsg const *pTimeMsg )
{
   uint32_t         delta;          /* Calculated delta demand. */
   uint8_t          indexDelta;     /* Index into the array */
   int32_t          timeSigOffset = (int32_t)TIME_SYS_getTimeSigOffset(); /*Run tolerance in ms *//*lint !e578 local variable same as enum OK  */

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   /* Compute number of bins jumped */
   indexDelta = 2; //default, reverse time jump crossing the last processed boundary
   if ( currentTime > demandFile_.dateTimeAtIndex )
   {
      indexDelta = (uint8_t)(MIN(((currentTime - demandFile_.dateTimeAtIndex) / DEMAND_PER_BIN_IN_TICKS ), NUM_OF_DELTAS));
   }

   /* Calculate the delta value */
   delta = DEMAND_INVALID_VALUE;        /* Overflow occured, next value is invalid */
   if (currentEnergy >= demandFile_.energyPrev)
   {   /* Check that the delta is in valid range */
      if ((currentEnergy - demandFile_.energyPrev) <= DEMAND_MAX_VALUE)
      {
         delta = (uint32_t)(currentEnergy - demandFile_.energyPrev);
      }
   }

   if (  (timeAdjusted_ & DEMAND_VIRGIN_POWER_UP) ||
         (indexDelta != 1) || //time jumped demand boundary backward or forward
         (pTimeMsg->timeChangeDelta <= -timeSigOffset) ||  //time jump greater than threshold
         (pTimeMsg->timeChangeDelta >= timeSigOffset) )
   {
      /* For this phase, time jump in either direction will invalidate the bin.
         For this phase, power-outage crossing the 5 minute boundary will invalidate the bin.
         In the future, reverse time jump can be treated differently than forward time jump. */
      delta = DEMAND_INVALID_VALUE;
   }
   //else demand period with-in limit, normal time progression.
   demandFile_.index                  = (demandFile_.index + 1) % NUM_OF_DELTAS; /*lint !e573 mixed signed-unsigned with division   */
   demandFile_.deltas.val[demandFile_.index] = delta;
   demandFile_.energyPrev             = currentEnergy;
   demandFile_.dateTimeAtIndex        = currentTime;
   demandFile_.dailyFlags |= (uint16_t)(1U << demandFile_.index);
   demandFile_.monthlyFlags |= (uint16_t)(1U << demandFile_.index);

   /* ****************************************************************************************************** */
   /* Now the table has been updated, the peak needs to be calculated                                        */
   /* ****************************************************************************************************** */
   demandConfig_t configParams;

   getConfigParams(demandFile_.config, &configParams);

   if ( ( (uint8_t)DMD_CONFIG_INVALID != configParams.config) &&
        ( (0 == currentTime % configParams.intervalPeriod) || //on interval boundary
         //sub-interval enabled and on sub-interval boundary
         ((0 != configParams.subIntervalPeriod) && (0 == currentTime % configParams.subIntervalPeriod)) ) )
   {  //check, if we have a new peak
      uint32_t  calcPeakDaily   = 0;     //Daily peak
      uint32_t  calcPeakMonthly = 0;     //Billing peak
      bool calcPeakDailyValid   = false; //Is at least 1 bin valid
      bool calcPeakMonthlyValid = false; //Is at least 1 bin valid
      uint8_t   indexTmp;                //Temporary index into the array of deltas based on the current file.index

      indexTmp = demandFile_.index;
      do /* Read the demand table bins here */
      {
         if ( DEMAND_INVALID_VALUE  == demandFile_.deltas.val[indexTmp] )
         {  //Invalid bin. Can not go past this bin
            break;
         }
         if ( (demandFile_.dailyFlags >> indexTmp) & 1 )
         {  //Bin valid for daily peak computation
            calcPeakDaily += demandFile_.deltas.val[indexTmp];
            calcPeakDailyValid = true;
         }
         if ( (demandFile_.monthlyFlags >> indexTmp) & 1 )
         {  //Bin valid for billing computation
            calcPeakMonthly += demandFile_.deltas.val[indexTmp];
            calcPeakMonthlyValid = true;
         }
         if ( 0 == indexTmp)
         {
            indexTmp = NUM_OF_DELTAS; //Does not overrun file.deltas.val[]. It will be decremented below
         }
         indexTmp--;
      }while (--configParams.subIntervals);

      if ( calcPeakDaily > demandFile_.peakDaily.energy )
      {  //new daily peak
         demandFile_.peakDaily.energy   = calcPeakDaily; //peak energy
         demandFile_.peakDaily.dateTime = currentTime;   //peak date and time
         DBG_printf( "New Daily Peak. Enerygy=%lu", calcPeakDaily);
         DBG_printf( "New Daily Peak. Date/Time=%llu, %llu", currentTime/TIME_TICKS_PER_DAY, currentTime%TIME_TICKS_PER_DAY);
      }
      //If valid peak found and dateTime still invalid (due to peak being equal to reset value)
      //Only happens once after reset and if no demand detected (keeps from marking reading as suspect)
      if ( INVALID_DMD_TIME == demandFile_.peakDaily.dateTime && calcPeakDailyValid == true )
      {
         demandFile_.peakDaily.dateTime = currentTime;   //peak date and time
      }

      if ( calcPeakMonthly > demandFile_.peak.energy )
      {  //new billing peak
         demandFile_.peak.energy   = calcPeakMonthly; //peak energy
         demandFile_.peak.dateTime = currentTime;     //peak date and time
      }
      //If valid peak found and dateTime still invalid (due to peak being equal to reset value)
      //Only happens once after reset and if no demand detected (keeps from marking reading as suspect)
      if ( INVALID_DMD_TIME == demandFile_.peak.dateTime && calcPeakMonthlyValid == true )
      {
         demandFile_.peak.dateTime = currentTime;   //peak date and time
      }
   }
   timeAdjusted_ = 0;
   (void)FIO_fwrite(&demandFileHndl_, 0, (uint8_t *)&demandFile_, (lCnt)sizeof(demandFile_));

   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}


/***********************************************************************************************************************
 *
 * Function name: DEMAND_GetDmdValue
 *
 * Purpose:  Get the demand related values from demand module or meter
 *
 * Arguments:  meterReadingType  RdgType,
 *             CimValueInfo_t    *rdgPari
 *
 * Returns: enum_CIM_QualityCode
 *
 ******************************************************************************************************************** */
enum_CIM_QualityCode DEMAND_GetDmdValue( meterReadingType RdgType, ValueInfo_t *rdgInfo )
{
   enum_CIM_QualityCode retVal = CIM_QUALCODE_KNOWN_MISSING_READ;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

#if ( DEMAND_IN_METER == 1 )
   ( void )memset( ( void * )rdgInfo, 0, sizeof( *rdgInfo ) );

   if ( demandResetCount == RdgType )
   {  //Demand reset count
      retVal = INTF_CIM_CMD_getDemandRstCnt( rdgInfo );
   }
   else
   {  //rest of the values
      retVal = INTF_CIM_CMD_getMeterReading(RdgType, rdgInfo );
   }
#else
   uint32_t u32Val;
   rdgInfo->cimInfo.coincidentCount = 0;  /* Demand in module never has coincident value(s)! */
   rdgInfo->cimInfo.timePresent     = 0;  /* Assume no time stamp present. */
   rdgInfo->ValTime.Time.date       = 0;
   rdgInfo->ValTime.Time.time       = 0;

   switch ( RdgType )   //lint !e788  Some enums in meterReadingType aren't used
   {
      case demandResetCount:
      {
         rdgInfo->ValTime.Value = (int64_t)demandFile_.resetCount;
         if ( rdgInfo->ValTime.Value != 0 )  /* If reset count is non-zero, time stamp is available.  */
         {
            rdgInfo->cimInfo.timePresent = 1;
            TIME_UTIL_ConvertSecondsToSysFormat( demandFile_.peakPrev.dtReset, 0,
                                                ( sysTime_t * )&rdgInfo->ValTime.Time);
         }
         rdgInfo->power10 = 0;
         retVal = CIM_QUALCODE_SUCCESS;
         break;
      }
      case maxPrevFwdDmdKW:
      {
         u32Val = demandFile_.peakPrev.energy;
         rdgInfo->ValTime.Value = (int64_t)u32Val;
         if ( demandFile_.peakPrev.dateTime != 0 )
         {
            rdgInfo->cimInfo.timePresent = 1;
            TIME_UTIL_ConvertSecondsToSysFormat( ( uint32_t ) ( demandFile_.peakPrev.dateTime / TIME_TICKS_PER_SEC ),
                                                   0, ( sysTime_t * )&rdgInfo->ValTime.Time);
         }
         rdgInfo->power10 = 4;        //Source value in deci-watts, report in kWatts
         retVal = CIM_QUALCODE_SUCCESS;
         break;
      }
      case dailyMaxFwdDmdKW:
      {
         u32Val = demandFile_.peakDailyPrev.energy;
         rdgInfo->ValTime.Value = (int64_t)u32Val;
         if ( demandFile_.peakDailyPrev.dateTime != 0 )
         {
            rdgInfo->cimInfo.timePresent = 1;
            TIME_UTIL_ConvertSecondsToSysFormat( ( uint32_t ) ( demandFile_.peakDailyPrev.dateTime / TIME_TICKS_PER_SEC ),
                                                   0, ( sysTime_t * )&rdgInfo->ValTime.Time);
         }
         rdgInfo->power10 = 4;        //Source value in deci-watts, report in kWatts
         retVal = CIM_QUALCODE_SUCCESS;
         break;
      }
      case maxPresFwdDmdKW:
      {
         u32Val = demandFile_.peak.energy;
         normalizeDemand(demandFile_.config, &u32Val);
         rdgInfo->ValTime.Value = (int64_t)u32Val;
         if ( demandFile_.peak.dateTime != 0 )
         {
            rdgInfo->cimInfo.timePresent = 1;
            TIME_UTIL_ConvertSecondsToSysFormat( ( uint32_t ) ( demandFile_.peak.dateTime / TIME_TICKS_PER_SEC ),
                                                   0, ( sysTime_t * )&rdgInfo->ValTime.Time);
         }
         rdgInfo->power10 = 4;        //Source value in deci-watts, report in kWatts
         retVal = CIM_QUALCODE_SUCCESS;
         break;
      }
      default:
      {
         break;
      }
   } //lint !e788  Some enums in meterReadingType aren't used
#endif
   //INFO_printf( "Reading Type=%u time in seconds=%lu", RdgType, *secs);
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails

   return retVal;
}

/***********************************************************************************************************************
 *
 * Function name: normalizeDemand
 *
 * Purpose:  Converts the Demand value to an hourly reference
 *
 * Arguments: uint8_t config, uint16_t *pInterval, uint16_t *pSubInterval
 *
 * Returns: None
 *
 ******************************************************************************************************************** */
static void normalizeDemand(uint8_t config, uint32_t *demandValue)
{
   demandConfig_t dmdConfig;

   getConfigParams(config, &dmdConfig);
   //Rollover cannot happen due to demand calculations limited to maximum of 12 summations of uint16's
   *demandValue *= dmdConfig.normalizer;
}

/***********************************************************************************************************************
 *
 * Function name: getValuein_dWatts
 *
 * Purpose:  Converts the given units to deciWatts
 *
 * Arguments: int64_t value: The value that needs to be converted to deciWatts
 *            uint8_t powerOf10: units of value passed
 *
 * Returns: int64_t: value in deciWatts
 *
 ******************************************************************************************************************** */
static int64_t getValuein_dWatts(int64_t value, uint8_t powerOf10)
{
   int64_t newValue;   //in watts

   switch ( powerOf10 )
   {
      case 1:
      {
         newValue = value * 1000;
         break;
      }
      case 2:
      {
         newValue = value * 100;
         break;
      }
      case 3:
      {
         newValue = value * 10;
         break;
      }
      case 4:
      {
         newValue = value;
         break;
      }
      case 5:
      {
         newValue = value / 10;
         break;
      }
      case 6:
      {
         newValue = value / 100;
         break;
      }
      case 7:
      {
         newValue = value / 1000;
         break;
      }
      case 8:
      {
         newValue = value / 10000;
         break;
      }
      case 9:
      {
         newValue = value / 100000;
         break;
      }
      default:
      {
         newValue = value;
         break;
      }
   }
   return newValue;
}

/***********************************************************************************************************************
 *
 * Function Name: startDmdResetLockOutTmr
 *
 * Purpose: Sets the lockout timer.  This timer is needed to ensure reset commands are not accepted too often.
 *
 * Arguments: time value in seconds; if 0, use the default set with the timer
 *
 * Returns: None
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
static void startDmdResetLockOutTmr( void )
{
   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   demandFile_.bits.resetLockOut = (bool)false; //Default to false, only set true if timer added
   if (0 != demandFile_.resetTimeout)
   {  // If not a timeout of zero, try to start the timer
      timer_t           timerCfg;      //Timer configuration

      //Setup lockout timer
      /* Create the timer for the reset lockout */
      (void)memset(&timerCfg, 0, sizeof(timerCfg));  /* Clear the timer values */
      timerCfg.bOneShot       = (bool)true;
      timerCfg.bOneShotDelete = (bool)true;
      timerCfg.bExpired       = (bool)false;
      timerCfg.bStopped       = (bool)false;

      /* Set time-out, converting seconds to system time ticks per scond  */
      timerCfg.ulDuration_mS  = demandFile_.resetTimeout * (uint32_t)TIME_TICKS_PER_SEC;
      timerCfg.pFunctCallBack = (vFPtr_u8_pv)resetTmrCallBack_ISR;
      if (eSUCCESS == TMR_AddTimer(&timerCfg) )               /* Create the lock-out timer  */
      {
         demandFile_.bits.resetLockOut = (bool)true;
      }
      else
      {
         DBG_logPrintf ('E', "Unable to add Demand Reset Lockout timer");
      }
   }
   (void)FIO_fwrite(&demandFileHndl_, 0, (uint8_t *)&demandFile_, (lCnt)sizeof(demandFile_));
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}

/***********************************************************************************************************************
 *
 * Function Name: resetTmrCallBack_ISR
 *
 * Purpose: Timer Expired, call back function.  In the background this clears the 'Reset LockOut' flag indicating
 *          a demand reset can be executed.  THIS IS CALLED FROM THE TIMER ISR AND IS RUNNING AT INTERRUPT LEVEL.
 *          FILEIO FUNCTIONS SHOULD NOT BE CALLED FROM THIS FUNCTION.
 *
 *
 * Arguments: uint8_t cmd, uint8_t *pData - Both variables are not used for this call-back function.
 *
 * Returns: None
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
//lint -e{715} suppress "cmd and pData not referenced"
static void resetTmrCallBack_ISR( uint8_t cmd, uint8_t const *pData )
{
   static buffer_t buf;

   /***
   * Use a static buffer instead of allocating one because if the allocation fails,
   * there wouldn't be much we could do - we couldn't reset the timer to try again
   * because we're already in the timer callback.
   ***/
   BM_AllocStatic(&buf, eSYSFMT_DMD_TIMEOUT);
   OS_MSGQ_Post(&mQueueHandle_, (void *)&buf);
}

/***********************************************************************************************************************
 *
 * Function Name: setSchDmdReset
 *
 * Purpose: Sets the next Scheduled Demand Reset alarm.  This alarm is used to autonomously attempt the next demand reset.
 *
 * Arguments: None - uses the configured value.
 *
 * Returns: returnStatus_t - eSUCCESS if set otherwise failure
 *
 * Side Effects: N/A
 *
 * Reentrant Code: No
 *
 **********************************************************************************************************************/
static returnStatus_t setSchDmdReset( void )
{
   returnStatus_t retVal = eFAILURE;
   sysTime_t      currSysTime;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   //  Cancel if alarm currently active
   if (NO_ALARM_FOUND != schDmdRstAlarmId_)
   {
      if (eSUCCESS == TIME_SYS_DeleteAlarm(schDmdRstAlarmId_))
      {
         schDmdRstAlarmId_ = NO_ALARM_FOUND;
      }
      else
      {
         //TODO: Should this set an opportunistic alarm?
         INFO_printf("Cancel Scheduled DMD Reset Failed"); // Debugging purposes
      }
   }
   if ( (DISABLE_SCHED_DMD_RESET_DATE != demandFile_.scheduledDmdResetDate) &&
        (MAX_SCHED_DMD_RESET_DATE     >= demandFile_.scheduledDmdResetDate) )
   {  // If schedule date is enabled and time is valid, try to set the alarm
      sysTime_dateFormat_t currDateTime;
      sysTime_dateFormat_t schDmdDateTime;
      sysTime_t            schDmdSysTime;
      tTimeSysCalAlarm     calAlarm;

      if ( TIME_SYS_GetSysDateTime(&currSysTime) )
      {  //Time is valid so set for next calendar date
         DST_ConvertUTCtoLocal ( &currSysTime );
         //Convert to Y,M,D,H,m,s,ms format (just need the year and month portion)
         (void)TIME_UTIL_ConvertSysFormatToDateFormat ( &currSysTime, &currDateTime );
         schDmdDateTime     = currDateTime;
         schDmdDateTime.day = demandFile_.scheduledDmdResetDate;
         // Daily Shift is local time in seconds, calendar alarm time is in mS
         (void)TIME_UTIL_ConvertDateFormatToSysFormat ( &schDmdDateTime, &schDmdSysTime );
         schDmdSysTime.time = HD_getDailySelfReadTime() * TIME_TICKS_PER_SEC;
         (void)TIME_UTIL_ConvertSysFormatToDateFormat ( &schDmdSysTime, &schDmdDateTime );
         // Current day/time after scheduled day/time for this month?
         // Otherwise the reset day/time has yet to occur this month
         if ( (currDateTime.day > schDmdDateTime.day) ||
              ( (currDateTime.day == schDmdDateTime.day) && (currSysTime.time >= schDmdSysTime.time) ) )
         {
            //Next Demand reset "already" occured so increment the month
            schDmdDateTime.month++;
            if ( schDmdDateTime.month > MAX_RTC_MONTH )
            {
               schDmdDateTime.month = MIN_RTC_MONTH;
               schDmdDateTime.year++;
            }
         }
         //Convert to days, msec format
         (void)TIME_UTIL_ConvertDateFormatToSysFormat (&schDmdDateTime, &schDmdSysTime);
         demandFile_.lastScdDmdReset.date = schDmdSysTime.date;
         demandFile_.lastScdDmdReset.time = schDmdSysTime.time;
         (void)FIO_fwrite(&demandFileHndl_, (uint16_t)offsetof(demandFileData_t, lastScdDmdReset),
                          (uint8_t *)&demandFile_.lastScdDmdReset, (lCnt)sizeof(demandFile_.lastScdDmdReset));
      }
      else
      {// Time not currently valid so set for the last Reset date
         schDmdSysTime.date = demandFile_.lastScdDmdReset.date;
         schDmdSysTime.time = demandFile_.lastScdDmdReset.time;
      }
      (void)memset(&calAlarm, 0, sizeof(calAlarm));            // Clr cal settings, only set what we need.
      calAlarm.pMQueueHandle   = &mQueueHandle_;               // Using message queue
      //  Set new alarm
      calAlarm.ulAlarmDate     = schDmdSysTime.date;           // Set the alarm date
      calAlarm.ulAlarmTime     = schDmdSysTime.time;           // Set the alarm time to be same as daily shift time
      calAlarm.bSkipTimeChange = true;                         // Don't notify on an time change
      calAlarm.bUseLocalTime   = true;
      if (eSUCCESS == TIME_SYS_AddCalAlarm(&calAlarm) )        /* Set the alarm */
      {
         retVal            = eSUCCESS;
         schDmdRstAlarmId_ = calAlarm.ucAlarmId;  // Alarm ID will be set after adding alarm.
      }
      else
      {
         //TODO: Should this set an Event? Only happens if there are too many alarms set
         DBG_logPrintf ('E', "Unable to add Scheduled Demand Reset Date");
      }
   }
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails

   return(retVal);
}

/***********************************************************************************************************************
 *
 * Function name: demandResetLogEvent
 *
 * Purpose: Log xxxOccurred or xxxFailed event for the results of a Demand Reset.
 *
 * Arguments: void
 *
 * Returns: void
 *
 * Side Effects: None
 *
 * Reentrant Code: No
 *
 * Notes:
  *
  ****************************************************************************************************************** */
static void demandResetLogEvent( returnStatus_t status, LoggedEventDetails *eventInfo )
{
   EventKeyValuePair_s  keyVal;        /* Event key/value pair info  */
   EventData_s          event;         /* Event info  */
   ValueInfo_t          readingInfo;
   sysTimeCombined_t    currentTime;   /* Get current time in seconds since EPOCH */
   stdTbl21_t          *pST21;

   ( void )memset( &keyVal, 0, sizeof( keyVal ) );
   event.eventId = ( uint16_t )electricMeterDemandResetFailed;
   if ( eSUCCESS == status )
   {
      event.eventId = ( uint16_t )electricMeterDemandResetOccurred;
   }

   ( void )DEMAND_GetDmdValue( demandResetCount, &readingInfo ); // Potentially includes the timestamp in days/ticks UTC
   *( uint16_t * )&keyVal.Key[0]   = ( uint16_t )demandResetCount;   /*lint !e826 !e2445 area too small   */
   ( void )memcpy( keyVal.Value, &readingInfo.Value.uintValue, sizeof( readingInfo.Value.uintValue ) );
   //if there are more than 1 NVP make sure to compensate the for the maximum size in eventKeyValueSize of all NVP
   event.eventKeyValuePairsCount = 1;
   //typecast should always be an uint in this function call, demandResetCount reading type 7
   event.eventKeyValueSize       = HEEP_getMinByteNeeded( readingInfo.Value.intValue,
                                                          readingInfo.typecast,
                                                          readingInfo.valueSizeInBytes);

   if( event.eventId == ( uint16_t )electricMeterDemandResetOccurred )
   {  // if a reset occured get the time of the last reset
      ( void )HMC_MTRINFO_getStdTable21( &pST21 );
      if ( pST21->dateTimeFieldFlag )
      {
         currentTime = TIME_UTIL_ConvertSysFormatToSysCombined( ( sysTime_t * )&readingInfo.timeStamp);
         event.timestamp = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
      }
      else
      {  // Meter does not support a Demand Reset Time, so use the value stored by NIC
         event.timestamp = demandFile_.peakPrev.dtReset;
      }
   }
   else
   {  // All other events use the current system time
      (void)TIME_UTIL_GetTimeInSysCombined(&currentTime);
      event.timestamp = (uint32_t)(currentTime / (sysTimeCombined_t)TIME_TICKS_PER_SEC);
   }
   if( NULL != eventInfo )
   {
      event.markSent = (bool)true;  // mark the event to be logged as already sent
      ( void )EVL_LogEvent( 160, &event, &keyVal, TIMESTAMP_PROVIDED, eventInfo );
   }
   else
   {
      event.markSent = (bool)false; // bubble up the event after it is logged
      ( void )EVL_LogEvent( 160, &event, &keyVal, TIMESTAMP_PROVIDED, NULL );
   }
}

/***********************************************************************************************************************
 *
 * Function name: DEMAND_getReadingTypes
 *
 * Purpose: Get Reading Types from demand read list configuration
 *
 * Arguments: uint16_t* pReadingType
 *
 * Returns: None
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void DEMAND_getReadingTypes(uint16_t* pReadingType)
{
   uint8_t i;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails
   for (i = 0; i < DMD_MAX_READINGS; ++i)
   {
      pReadingType[i] = (uint16_t)drList_.rdList[i];
   }
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}


#if ( DEMAND_IN_METER == 0 )
/***********************************************************************************************************************
 *
 * Function name: DEMAND_clearDemand
 *
 * Purpose: Clears all demand value in the endpoint.
 *
 * Arguments: None
 *
 * Returns: None:
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void DEMAND_clearDemand()
{

   uint8_t  config;        //store demand Present Config
   uint8_t  futureConfig;  //store demand Future Config
   uint32_t resetTimeout;  //store reset Timeout interval

   config = demandFile_.config;
   futureConfig = demandFile_.futureConfig;
   resetTimeout = demandFile_.resetTimeout;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   (void)memset(&demandFile_, 0, sizeof(demandFile_)); //clear the file
   demandFile_.config                  = config;
   demandFile_.futureConfig            = futureConfig;
   demandFile_.resetTimeout            = resetTimeout;
   demandFile_.peak.dtReset            = INVALID_DMD_TIME;
   demandFile_.peak.dateTime           = INVALID_DMD_TIME;
   demandFile_.peakPrev.dtReset        = INVALID_DMD_TIME;
   demandFile_.peakPrev.dateTime       = INVALID_DMD_TIME;
   demandFile_.peakDaily.dtReset       = INVALID_DMD_TIME;
   demandFile_.peakDaily.dateTime      = INVALID_DMD_TIME;
   demandFile_.peakDailyPrev.dtReset   = INVALID_DMD_TIME;
   demandFile_.peakDailyPrev.dateTime  = INVALID_DMD_TIME;
   demandFile_.bits.resetLockOut = false;

   (void)FIO_fwrite(&demandFileHndl_, 0, (uint8_t*)&demandFile_, (lCnt)sizeof(demandFile_));
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}
#endif


/***********************************************************************************************************************
 *
 * Function name: DEMAND_setReadingTypes
 *
 * Purpose: Set Reading Types in demand read list configuration
 *
 * Arguments: uint16_t* pReadingType
 *
 * Returns: void
 *
 * Re-entrant Code: Yes
 *
 * Notes:
 *
 **********************************************************************************************************************/
void DEMAND_setReadingTypes(uint16_t* pReadingType)   //lint !e818 could be pointer to const
{
   uint8_t i;

   OS_MUTEX_Lock(&dmdMutex_); // Function will not return if it fails

   for (i = 0; i < DMD_MAX_READINGS; ++i)
   {
      drList_.rdList[i] = (meterReadingType)pReadingType[i];
   }
   (void)FIO_fwrite(&drListHndl_, 0, (uint8_t*)&drList_, (lCnt)sizeof(drList_));
   OS_MUTEX_Unlock(&dmdMutex_); // Function will not return if it fails
}  /*lint !e818 pReadingType could be pointer to const   */

/***********************************************************************************************************************

   Function Name: DEMAND_DrReadListHandler( action, id, value, attr )

   Purpose: Get/Set drReadList

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t DEMAND_DrReadListHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr ) //lint !e715 symbol id not referenced.
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( (sizeof(uint16_t)*DMD_MAX_READINGS) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DEMAND_getReadingTypes((uint16_t*)value);
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)(sizeof(uint16_t)*DMD_MAX_READINGS);
            attr->rValTypecast = (uint8_t)uint16_list_type;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {

      if( attr->rValLen == ( DMD_MAX_READINGS * sizeof(uint16_t) ) )
      {
         uint16_t uReadingType[DMD_MAX_READINGS] = {0};
         uint16_t *valPtr = value;

         uint8_t cnt;
         for(cnt = 0; cnt < (attr->rValLen / sizeof(uint16_t)); cnt++)
         {
            uReadingType[cnt] = valPtr[cnt];
         }
         DEMAND_setReadingTypes(&uReadingType[0]);
         retVal = eSUCCESS;
      }
      else
      {
         retVal = eAPP_INVALID_VALUE;
      }
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: DEMAND_demandFutureConfigurationHandler

   Purpose: Get/Set the demandFutureConfigurationHandle parameter over the air

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t DEMAND_demandFutureConfigurationHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr ) //lint !e715 parameter not referenced.
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( demandFile_.futureConfig ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = DEMAND_GetFutureConfig();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof( demandFile_.futureConfig );
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
#if ( DEMAND_IN_METER == 0 )
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = DEMAND_SetFutureConfig(*(uint8_t *)value);

      if (retVal != eSUCCESS)
      {
        retVal = eAPP_INVALID_VALUE;
      }

   }
#endif
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: DEMAND_demandPresentConfigurationHandler

   Purpose: Get the demandPresentConfiguration parameter value over the air

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t DEMAND_demandPresentConfigurationHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )   //lint !e715 parameter not referenced.
{

   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( demandFile_.config ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = DEMAND_GetConfig();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof( demandFile_.config );
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }

   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched


/***********************************************************************************************************************

   Function Name: DEMAND_demandResetLockoutPeriodHandler

   Purpose: Get/Set the demandResetLockoutPeriodHandler heep parameter

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
returnStatus_t DEMAND_demandResetLockoutPeriodHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )  //lint !e715 parameter not referenced.
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof( demandFile_.resetTimeout ) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         DEMAND_GetTimeout( (uint32_t *)value);
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)HEEP_getMinByteNeeded((int64_t)demandFile_.resetTimeout, uintValue, 0);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = DEMAND_SetTimeout(*(uint32_t *)value);
   }
   return retVal;

} //lint !e715 symbol id not referenced. This funtion is only called when id is matched

