/***********************************************************************************************************************
 *
 * Filename:    PhaseDetect.c
 *
 * Global Designator: PD
 *
 * Contents: Phase Detection Functions
 *
 ******************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *****************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include "project.h"

#if ( PHASE_DETECTION == 1 )
#if ( MCU_SELECTED == NXP_K24 )
#include <bsp.h>
#include "FTM.h"
#elif ( MCU_SELECTED == RA6E1 )
#include "AGT.h"
#endif
#include "file_io.h"
#include "timer_util.h"
#include "time_sys.h"
#include "time_util.h"
#include "PhaseDetect.h"
#include "heep_util.h"
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
#include "hmc_display.h"
#endif

#include "HEEP.h"
#include "EVL_event_log.h"
#include "sys_clock.h"

/* ****************************************************************************************************************** */
/* CONSTANTS */
#define PD_LINE_FREQ         60U  /* 60 Hz System */

#define PD_NOMINAL_PERIOD     2000000U  /* Default Period (K24 core speed 120 MHz / Power line 60 Hz) */
#define PD_NOMINAL_PERIOD_MAX 2200000U  /* Max Valid Period (K24 core speed 120 MHz / Power line 60 Hz) * 1.1 */
#define PD_NOMINAL_PERIOD_MIN 1800000U  /* Min Valid Period (K24 core speed 120 MHz / Power line 60 Hz) * 0.9 */

/* Report Mode Values */
#define PD_SURVEY_MODE_DISABLED  0
#define PD_SURVEY_MODE_ENABLED   1

/* Nulling Offset Range */
#define PD_MAX_ANGLE 36000

#define PD_NULLING_OFFSET_MIN  -36000
#define PD_NULLING_OFFSET_MAX  +36000

/* Maximum Number of Survey Message Allow in a survey period */
#define PD_MAX_SURVEYS 4   /* Priority for the event */
#define PD_HISTORY_CNT 2   /* Number of history buffers, i.e current, previous */

/* Event Parameters */
#define PD_EVENT_PRIORITY     200   /* Priority for the event */
#define PD_EVENT_PAIRS_COUNT    2   /* Number of name/values pairs */
#define PD_EVENT_VALUE_SIZE     5   /* Size of the value */

#define SYNC_ENTRY_COUNT 4        // The number of sync message that are retained by the radio layer

#define PD_SURVEY_PERIOD_DEFAULT     SECONDS_PER_DAY         // Default to 24:00:00 (Daily)
#define PD_SURVEY_START_TIME_DEFAULT (2*SECONDS_PER_HOUR)    // Default to 2:00:00 (7200)
#define PD_NULLING_OFFSET_DEFAULT    -1200                   // Default to -12 degrees for i210+C
#define PD_SURVEY_MODE_DEFAULT       PD_SURVEY_MODE_ENABLED  // Default to enabled
#define PD_SURVEY_QUANTITY_DEFAULT   3                       // Default to 3 surveys
static const char * const PD_LCD_MSG_DEFAULT = "PH  --";

#define PD_BU_MAX_TIME_DIVERSITY_MAX        255 // Maximum Time Diversity (minutes)
#define PD_BU_MAX_TIME_DIVERSITY_MIN          1 // Minimum TIme Diversity (minutes)
#define PD_BU_MAX_TIME_DIVERSITY_DEFAULT    720 // Default Time Diversity (minutes)

#define PD_BU_DATA_REDUNDANCY_MAX             3 // Maximum Data Redundancy
#define PD_BU_DATA_REDUNDANCY_DEFAULT         1 // Default Data Redundancy

#define PD_SURVEY_PERIOD_QTY_DEFAULT     0
#define PD_SURVEY_PERIOD_QTY_MAX         1

#define PD_ZC_PERIOD_FACTOR 16   // Used for computing the average period

#define WHITE_SPACE_CHAR 0x20


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* This macro is used to set the key field in the EventKeyValuePair_s */
#define SET_KEY(a) *(uint16_t *)keyVal[a].Key


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* This is used to record the sync data captured in the hwIsr when the radio sync is detected.
 * The MAC layer will look up thi
 */

typedef struct
{
   TIMESTAMP_t syncTime;       // Time Stamp related to RF Sync
   uint32_t    syncTimeCYCCNT; // Counter value when the sync occurred in CYCCNT units
   uint32_t    zcd_count;      // Counter value when the previous zero cross occurred
   uint32_t    zcd_period;     // Number of counts in the zero cross period
} sync_record_t;

typedef struct
{
   uint32_t PD_SurveyPeriod;               /* The number of seconds for a survey period
                                              Only specific values are allowed.
                                                900 - 00:15
                                               1800 - 00:30
                                               3600 - 01:00
                                               7200 - 02:00
                                              10800 - 03:00
                                              14400 - 04:00
                                              21600 - 06:00
                                              28800 - 08:00
                                              43200 - 12:00
                                              86400 - 24:00  */
   uint32_t PD_SurveyStartTime;            /* The offset time (in seconds) from the start of the period. Default Value = 12:00:00 */
   uint8_t  PD_SurveyMode;                 /* PD_SurveyMode (Default - Enabled)
                                                0 = Disabled
                                                1 = Enabled */
   uint8_t  PD_SurveyQuantity;             /* The number of PD surveys events which are to be created.  range 1-4 */
   int32_t  PD_ZCProductNullingOffset;     /* Amount (in degrees with resolution to 1/100 degree) to be added to the calculation of the local
                                              phasor angle to null the hardware’s ZC latency.
                                              Range -359.99 <= offset <= +359.99 */
   char     PD_LcdMsg[PD_LCD_MSG_SIZE];    /* A message to be displayed on the meter LCD describing the meter’s phase connection. */
   uint16_t PD_BuMaxTimeDiversity;         /* Bubble up maximum time diversity in minutes (0-1440) */
   uint8_t  PD_BuDataRedundancy;           /* Bubble up data redundancy (0-3). ??? Number of repeats???*/
   uint8_t  PD_SurveyPeriodQty;            /* The number of survey periods to be included in the response range 0-1, where 0 means 1 */
}PD_ConfigAttr_t;


typedef struct
{
   uint64_t   handle_id;                    /* handle number from the message */
   uint16_t   angle;                        /* Phase Angle */
} survey_measurement_t;

typedef struct
{
   uint8_t              num_entries;          // Number of entries in the table
   survey_measurement_t data[PD_MAX_SURVEYS]; // Array of survey measurements
} SurveyMeasurments_t;


typedef struct
{
   uint32_t    CurrentStartTime;           // Start time of the current survey period
   struct
   {
      uint8_t   currentIndex;                                 // Track current and previous
      SurveyMeasurments_t SurveyMeasurments[PD_HISTORY_CNT];  // 2 sets, current and previous
   }  SurveyPeriods;
}PD_CachedAttr_t;


typedef struct
{
   FileHandle_t          handle;
   char           const *FileName;         /*!< File Name           */
   ePartitionName const  ePartition;       /*!<                     */
   filenames_t    const  FileId;           /*!< File Id             */
   void*          const  Data;             /*!< Pointer to the data */
   lCnt           const  Size;             /*!< Size of the data    */
   FileAttr_t     const  Attr;             /*!< File Attributes     */
   uint32_t       const  UpdateFreq;       /*!< Update Frequency    */
} PD_file_t;

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static PD_ConfigAttr_t ConfigAttr_;
static PD_CachedAttr_t CachedAttr_;

// Phase Detect Configuration Data File
static PD_file_t ConfigFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "PD_CONFIG",
   .FileId          = eFN_PD_CONFIG,             // Configuration Data
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &ConfigAttr_,
   .Size            = sizeof(ConfigAttr_),
   .UpdateFreq      = 0xFFFFFFFF                    // Updated Seldom
}; /*lint !e785 too few initializers   */

// Phase Detection Cached Data File
static PD_file_t CachedFile =
{
   .ePartition      = ePART_SEARCH_BY_TIMING,
   .FileName        = "PD_CACHED",
   .FileId          = eFN_PD_CACHED,               // Cached Data
   .Attr            = FILE_IS_NOT_CHECKSUMED,
   .Data            = &CachedAttr_,
   .Size            = sizeof(CachedAttr_),
   .UpdateFreq      = 0                            // Updated Often
};/*lint !e785 too few initializers   */


static PD_file_t * const Files[2] =
{
   &ConfigFile,
   &CachedFile
};

static OS_MUTEX_Obj  configMutex_;               /* Serialize access to time sync data-structure */
static uint8_t       alarmId_ = NO_ALARM_FOUND;  /* Alarm ID, set by the time calendar alarm */

static struct
{
   int index;                              // Next index to use
   sync_record_t entry[SYNC_ENTRY_COUNT];  // Array of sync records
} sync_data;

/* This is used to access the sync_time for a phase detect message from the application handler */
static struct
{
   TIMESTAMP_t sync_time;
}pd_;

static struct{
   uint16_t   TimerId;        // Timer ID used by time diversity.
   buffer_t*  pMessage;       // Pointer to the message that will be sent after the time diversity expires.
   uint8_t    RepeatCount;    // Number of times to repeat
}timeDiversity_ =
{
   .TimerId    = 0,
   .pMessage    = NULL,
   .RepeatCount = 0
};

static struct
{
   uint32_t PreviousCapturedValue;     // timer value captured on the zero cross interrupt
   uint32_t Period;                    // Computed period
}zc;

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */


static returnStatus_t PD_SetTimeDiversityTimer(uint32_t timeMilliseconds);
static void           PD_TimeDiversityTimerExpired(uint8_t cmd, void *pData);

static void PD_TxBuMsg(const buffer_t* pBuf);
static void PD_Tx_WithTimeDiversity(buffer_t *pBuf, uint8_t repeats);

static void ConfigAttr_Init(bool bWrite);
static void CachedAttr_Init(bool bWrite);
static uint16_t CalcPhaseAngle(sync_record_t *SyncData);

static void SurveyLog_NewPeriod(void);
static void SurveyLog_ResetAll(void);
static uint32_t CalcStartTime(uint32_t CurrentTime);

/* Function used for tracking sync history */
static void sync_record_init(void);
static void sync_record_add(const sync_record_t *data);
static bool sync_record_get(TIMESTAMP_t sync_time, sync_record_t *data);
static bool IsDuplicateHandle(uint64_t handle_id);
static void SurveyPeriod_print(void);
static bool PhaseDetect_LogData( uint64_t handle_id, uint16_t angle);
static buffer_t *BuildSurveyMessage(uint8_t SurveyPeriodQty);


/* Zero Cross Functions */
#if ( MCU_SELECTED == NXP_K24 )
static void ZCD_hwIsr( void );
#endif
static uint32_t calc_period(uint32_t average, uint32_t value);

static void printSyncInfo(const sync_record_t *pSyncRecord);
static void printPhaseData(const uint8_t  src_addr[], uint16_t phaseAngle);

static uint32_t PD_NominalPeriod( void );
static void PD_print_CachedData(void);
static void PD_print_freq(void);
static void PD_print_time(uint32_t StartTime);

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */


/*! ********************************************************************************************************************
 *
 * \fn PD_init
 *
 * \brief  Initialize the phase detection module
 *
 * \param  none
 *
 * \return  void
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_init ( void )
{
   returnStatus_t retVal = eFAILURE;

   // Create the mutex
   if(OS_MUTEX_Create(&configMutex_) == true)
   {

      // Initialize the attributes to default values, but don't write
      ConfigAttr_Init( (bool) false);
      CachedAttr_Init( (bool) false);

      uint8_t i;
      FileStatus_t fileStatus;
      for( i=0; i < (sizeof(Files)/sizeof(*(Files))); i++ )
      {
         PD_file_t *pFile = (PD_file_t *)Files[i];
         if ( eSUCCESS == FIO_fopen(&pFile->handle,      /* File Handle so that PHY access the file. */
                                    pFile->ePartition,   /* Search for the best partition according to the timing. */
                                    (uint16_t) pFile->FileId, /* File ID (filename) */
                                    pFile->Size,         /* Size of the data in the file. */
                                    pFile->Attr,         /* File attributes to use. */
                                    pFile->UpdateFreq,   /* The update rate of the data in the file. */
                                    &fileStatus) )       /* Contains the file status */
         {
            if ( fileStatus.bFileCreated )
            {  // The file was just created for the first time.
               // Save the default values to the file
               retVal = FIO_fwrite( &pFile->handle, 0, pFile->Data, pFile->Size);
            }
            else
            {  // Read the MAC Cached File Data
               retVal = FIO_fread( &pFile->handle, pFile->Data, 0, pFile->Size);
            }
         }
      }

      sync_record_init();

#if ( MCU_SELECTED == NXP_K24 )
      // Configure FTM3_CH1 to capture timer when ZCD_METER signal is detected.
      (void)FTM3_Channel_Enable( 1, FTM_CnSC_CHIE_MASK | FTM_CnSC_ELSA_MASK, ZCD_hwIsr );
      HMC_ZCD_METER_TRIS(); // Map ZCD_METER signal to FTM3_CH1
#elif ( MCU_SELECTED == RA6E1 )
      // TODO: RA6E1 Bob: Need to set up the AGT to capture time from P114, ZCD_METER signal
//      AGT_PD_Enable();
      GPT_PD_Enable();
#endif

      PD_AddSysTimer();
   }
   return ( retVal );
}

/*! ********************************************************************************************************************
 *
 * \fn  next_buffer
 *
 * \brief Advance to the next buffer
 *
 * \details This advances there buffer index, then clears the data in the buffer.
 *
 * \param none
 *
 * \return none
 *
 ********************************************************************************************************************
 */
static void next_buffer(void)
{
   // Advance to the next buffer
   CachedAttr_.SurveyPeriods.currentIndex = (CachedAttr_.SurveyPeriods.currentIndex + 1 ) % PD_HISTORY_CNT;

   // Get the current index
   int set_index = CachedAttr_.SurveyPeriods.currentIndex;

   SurveyMeasurments_t *pSurveyMeasurements = &CachedAttr_.SurveyPeriods.SurveyMeasurments[set_index];

   // Reset the counters
   pSurveyMeasurements->num_entries = 0;
   for (int slot = 0; slot < PD_MAX_SURVEYS; slot++)
   {
      pSurveyMeasurements->data[slot].angle = 65535;              // Set to 65536 to make it easier to see that is is reset
      pSurveyMeasurements->data[slot].handle_id = 0;
   }
   (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
}



/*! ********************************************************************************************************************
 *
 * \fn  SurveyLog_Reset
 *
 * \brief Reset all measurment data
 *
 * \param none
 *
 * \return none
 *
 ********************************************************************************************************************
 */
static void SurveyLog_ResetAll(void)
{
   CachedAttr_.CurrentStartTime  = 0;
   CachedAttr_.SurveyPeriods.currentIndex = 0;
   for (int i = 0; i < PD_HISTORY_CNT; i++)
   {
      CachedAttr_.SurveyPeriods.SurveyMeasurments[i].num_entries = 0;
   }
   (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);

   if(ConfigAttr_.PD_SurveyMode > 0)
   {
      PD_AddSysTimer();
   }
}


/*! ********************************************************************************************************************
 *
 * \fn  CalcStartTime
 *
 * \brief Used to compute the survey start time for a period
 *
 * \details The start time for a survey period is a function of the SurveyPeriod and the SurveyStartTime.
 *          The SurveyStartTime is actually an offset from the start of a period.
 *          The calculated StartTime should be less that the current time.
 *
 * \param CurrentTime - the current time in seconds
 *
 * \return StartTime - the computed value to the start time
 *
 ********************************************************************************************************************
 */
static
uint32_t CalcStartTime(uint32_t CurrentTime)
{
   uint32_t SurveyPeriod      = ConfigAttr_.PD_SurveyPeriod;
   uint32_t SurveyStartTime   = ConfigAttr_.PD_SurveyStartTime;

   uint32_t StartTime = ((CurrentTime / SurveyPeriod) * SurveyPeriod) + SurveyStartTime;
   if (CurrentTime < StartTime) {
      // Ensure that the start time is less than the current time
      StartTime = StartTime - SurveyPeriod;
   }

   return StartTime;

}


/*! ********************************************************************************************************************
 *
 * \fn  PD_NominalPeriod
 *
 * \brief Adjust the nominal period based on the real CPU frequency
 *
 * \param none
 *
 * \return Compensated period based on real CPU frequency
 *
 ********************************************************************************************************************
 */
static
uint32_t PD_NominalPeriod( void )
{
   uint32_t CPUfreq; // Hold best real CPU frequency estimate
   uint32_t value;

   (void)TIME_SYS_GetRealCpuFreq( &CPUfreq, NULL, NULL );

   // Adjust value with the real CPU frequency
   value = (uint32_t)(((float)CPUfreq/(float)getCoreClock())*(float)PD_NOMINAL_PERIOD+0.5f);

   return value;
}


/*! ********************************************************************************************************************
 *
 * \fn  SurveyLog_NewPeriod
 *
 * \brief Start a new phase detection period
 *
 * \param none
 *
 * \return none
 *
 ********************************************************************************************************************
 */
static
void SurveyLog_NewPeriod(void)
{
   TIMESTAMP_t Time;
   (void) TIME_UTIL_GetTimeInSecondsFormat( &Time );

   uint32_t CurrentTime = Time.seconds;

   // Check is the current time is >= to the next start time
   if( CurrentTime >= (CachedAttr_.CurrentStartTime + ConfigAttr_.PD_SurveyPeriod))
   {
      // Advance to the next buffer
      next_buffer();

      // Update the start time
      CachedAttr_.CurrentStartTime = CalcStartTime(CurrentTime);

      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
      PD_print_time(CachedAttr_.CurrentStartTime);
   }
   else{
      // Wait until the event to reset
   }
}


/*! ********************************************************************************************************************
 *
 * \fn uint16_t CalcPhaseAngle(sync_record_t *SyncData)
 *
 * \brief  Calculate the phase angle from the sync data
 *
 * \param  pointer to the sync data
 *
 * \return phase angle
 *
 ********************************************************************************************************************
 */
static
uint16_t CalcPhaseAngle(sync_record_t* SyncData)
{
   uint32_t delta;

   // Check for race condition.
   // Sometimes the SYNC interrupt was generated followed closely by the ZC interrupt but before we reach this point (i.e. before we process the SYNC interrupt).
   // In that case, zcd_count is larger than sync_count which we don't want since the rest of the code assumes ZCD was before SYNC.
   // Detect and compensate for that by subtracing a cycle from the ZCD timestamp.
   if ( (int32_t)(SyncData->zcd_count-SyncData->syncTimeCYCCNT) > 0 )
   {  // ZC happened right after SYNC so make it look like it happened before SYNC.
      SyncData->zcd_count -= zc.Period;
   }

   // Compute phase angle
   delta = SyncData->syncTimeCYCCNT - SyncData->zcd_count;

   // Convert to Angle Until the next ZC
   // Note that the angle must be in the range of 0 to 35999
   int32_t i32_PhaseAngle = (int32_t) (PD_MAX_ANGLE - (int32_t)(((uint64_t)delta * (uint64_t)PD_MAX_ANGLE) / (uint64_t)SyncData->zcd_period));

   // Adjust by the Nulling Offset
   // Ensure that it is in the range of 0 to 35999
   if (i32_PhaseAngle < 0)
   {
      i32_PhaseAngle = i32_PhaseAngle + PD_MAX_ANGLE;
   }
   else if (i32_PhaseAngle >= PD_MAX_ANGLE)
   {
      i32_PhaseAngle = i32_PhaseAngle - PD_MAX_ANGLE;
   }
   return (uint16_t) i32_PhaseAngle;
}


/*! ********************************************************************************************************************
 *
 * \fn SurveyPeriod_print()
 *
 * \brief This is used to print the survey measurements values in current and previous buffers
 *
 * \param none
 *
 * \return void
 *
 ********************************************************************************************************************
 */
static
void SurveyPeriod_print()
{
   INFO_printf("index:%d", CachedAttr_.SurveyPeriods.currentIndex);

   // Print current and then previous

   uint8_t set_index = CachedAttr_.SurveyPeriods.currentIndex;
   uint8_t survey_set = 0;

   // for each Survey Period
   do
   {
      SurveyMeasurments_t *pSurveyMeasurements = &CachedAttr_.SurveyPeriods.SurveyMeasurments[set_index];

      INFO_printf("set_index: %d count:%d", set_index, pSurveyMeasurements->num_entries);
      for (uint32_t slot = 0; slot < pSurveyMeasurements->num_entries; slot++)
      {
         survey_measurement_t *pSurveyMeasurement = &pSurveyMeasurements->data[slot];
         INFO_printf("slot: %u handle:%016llx Angle:%3u.%02u",
                     slot,
                     pSurveyMeasurement->handle_id,
                     pSurveyMeasurement->angle/100, pSurveyMeasurement->angle%100);
      }
      // Advance to the next set of mesaurments

      set_index = (set_index + 1 ) % PD_HISTORY_CNT;
   }
   while (++survey_set < PD_HISTORY_CNT);
}


/*! ********************************************************************************************************************
 *
 * \fn IsDuplicateHandle
 *
 * \brief Used to check for a duplicate handle_id
 *
 * \param  handle to check
 *
 * \return true/false
 *
 ********************************************************************************************************************
 */
static bool IsDuplicateHandle(uint64_t handle_id)
{

   uint8_t set_index = CachedAttr_.SurveyPeriods.currentIndex;
   uint8_t survey_set = 0;
   do
   {
      SurveyMeasurments_t *pSurveyMeasurements = &CachedAttr_.SurveyPeriods.SurveyMeasurments[set_index];

      for (int slot = 0; slot < pSurveyMeasurements->num_entries; slot++)
      {
         survey_measurement_t *pSurveyMeasurement = &pSurveyMeasurements->data[slot];
         if (pSurveyMeasurement->handle_id == handle_id)
         {
//          INFO_printf("Duplicate handle");
            return true;
         }
//       else{
//          INFO_printf("Not Duplicate handle");
//       }
      }
      // Advance to the next set of mesaurments
      set_index = (set_index + 1 ) % PD_HISTORY_CNT;
   }
   while (++survey_set <= ConfigAttr_.PD_SurveyPeriodQty);
   return false;
}



/*! ********************************************************************************************************************
 *
 * \fn PhaseDetect_LogData
 *
 * \brief Logs the survey measurment.  Measurement with duplicate handles are not logged.
 *
 * \param  handle_id
 *         angle
 *
 * \return true  - measurement was logged
 *         false - measurement was not logged, either due to duplicate or PD_MAX_SURVEY limit
 *
 ********************************************************************************************************************
 */
static
bool PhaseDetect_LogData(uint64_t handle_id, uint16_t angle)
{
   INFO_printf( "PhaseDetect_LogData[%u]: Handle:%016llx angle:%3u.%02u",
               CachedAttr_.SurveyPeriods.currentIndex,
               handle_id,
               angle/100, angle%100);

   if(!IsDuplicateHandle(handle_id))
   {
      SurveyMeasurments_t *pSurveyMeasurements = &CachedAttr_.SurveyPeriods.SurveyMeasurments[CachedAttr_.SurveyPeriods.currentIndex];

      // Are the any slots available
      if (pSurveyMeasurements->num_entries < ConfigAttr_.PD_SurveyQuantity)
      {
         survey_measurement_t *pSurveyData = &pSurveyMeasurements->data[pSurveyMeasurements->num_entries];
         pSurveyData->angle     = angle;
         pSurveyData->handle_id = handle_id;

         // Increment the number of entries
         pSurveyMeasurements->num_entries++;
         (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
         return true;
      }
      INFO_printf("PD-Max Entries");
   }
   else{
      INFO_printf("PD-Duplicate handle");
   }
   return false;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_DebugCommand
 *
 * \brief This is used to calculate and print the Phase Angle
 *
 * \details  This function is for for debugging only.   It is called from the MAC for every data indication.
 *           It lookups the sync data that was captured by the radio, calculate the phase angle and then
 *           print the results to the debug port.
 *
 * \param sync_time  time when the sync was detected
          src_addr   address of the dcu that sent the message
 *
 * \return status
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_DebugCommand(TIMESTAMP_t t, const uint8_t src_addr[5])
{
   returnStatus_t retVal = eFAILURE;
   sync_record_t SyncData;
   if(sync_record_get(t, &SyncData))
   {
      // Calculate the phase angle from the sync data
      printSyncInfo(&SyncData);

      uint16_t phaseAngle = CalcPhaseAngle(&SyncData);
      printPhaseData(src_addr, phaseAngle);
      retVal = eSUCCESS;
   }
   return retVal;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_SyncDeteced
 *
 * \brief This function called from the PHY hwIsr when a sync is detected
 *
 * \details
 *        It saves the phase detection data so that later the data can be retrieved
 *        if the message was a Phase Detection Survey Message.
 *
 * \param syncTime       - When the SYNC happened. This doesn't need to be very precise. It is used to correlate a SYNC event with a specific message /pd message.
 *        syncTimeCYCCNT - The CYCCNT counter value when the SYNC happened
 *
 * \return  void
 *
 ********************************************************************************************************************
 */
void PD_SyncDetected(TIMESTAMP_t syncTime, uint32_t syncTimeCYCCNT)
{
   sync_record_t data;

   // The sync_count is the reference time for when the sync was detected.
   data.syncTimeCYCCNT = syncTimeCYCCNT; // When SYNC was detected

   data.zcd_count  = zc.PreviousCapturedValue; // Get the counter for the last zero cross
   data.zcd_period = zc.Period;                // Get the period

   // Save the sync time. This is used later to look up the sync data.
   data.syncTime = syncTime;
   sync_record_add(&data);
}


/*! -----------------------------------------------------------------
 *  Initialize the sync records
 * ----------------------------------------------------------------- */
static
void sync_record_init(void)
{
   (void)memset(&sync_data, 0, sizeof(sync_data));
}

/*! -----------------------------------------------------------------
 *  Add a new sync record
 * ----------------------------------------------------------------- */
static
void  sync_record_add(const sync_record_t *data)
{
   __disable_interrupt(); // Make operation atomic because this data is accessed from 2 tasks
   int index = sync_data.index;
   sync_data.entry[index] = *data;
   sync_data.index = ++index % SYNC_ENTRY_COUNT;
   __enable_interrupt();
}

/*! -----------------------------------------------------------------
 * Get the sync record for a specific sync time
 * ----------------------------------------------------------------- */
static
bool sync_record_get(TIMESTAMP_t sync_time, sync_record_t *data)
{
   __disable_interrupt(); // Make operation atomic because this data is accessed from 2 tasks
   for (int i=0; i<SYNC_ENTRY_COUNT; i++)
   {
      if (sync_data.entry[i].syncTime.QSecFrac == sync_time.QSecFrac)
      {
          *data = sync_data.entry[i];
          __enable_interrupt();
          return true;
      }
   }
   __enable_interrupt();
   return false;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_SyncTime_Set(TIMESTAMP_t sync_time)
 *
 * \brief This is called to save the sync time was captured by the radio.
 *
 * \details The time is used to match the data to the message.
 *          This function is required because the sync_time is not passed to the application layer
 *          The alternative is to change the API to all of the message handlers
 *
 * \param sync_time
 *
 * \return none
 *
 ********************************************************************************************************************
 */
void PD_SyncTime_Set(TIMESTAMP_t sync_time)
{
   pd_.sync_time = sync_time;
}


/*! ********************************************************************************************************************
 *
 * \fn calc_ema
 *
 * \brief
 *    Calculate the exponential moving average of the period timer
 *
 * \param   uint32_t average  - current average
 *          uint32_t value    - new value to include in the average
 *
 * \return  new average
 *
 ********************************************************************************************************************
 */
static
uint32_t calc_period(uint32_t average, uint32_t value)
{
   static int32_t counter = 0;

   if (counter < PD_ZC_PERIOD_FACTOR)
   {
      counter++;
   }

   average += (uint32_t)((int32_t)(value - average) / counter);

   return average;
}


/*! ********************************************************************************************************************
 *
 * \fn void ZCD_hwIsr( void )
 *
 * \brief This is the zero cross hardware isr.
 *
 * \details The hardware is configured to capture the FTM3 Timer.
 *          The captured value is passed to the PLL
 *
 * \param   void
 *
 * \return  none
 *
 ********************************************************************************************************************
 */
#if ( MCU_SELECTED == NXP_K24 )
static void ZCD_hwIsr( void )
#elif ( MCU_SELECTED == RA6E1 )
uint32_t iCapture_overflows = 0U;
uint64_t captured_time     = 0U;
void ZCD_hwIsr(timer_callback_args_t * p_args)
#endif
{
#if ( MCU_SELECTED == RA6E1 )
   if ( ( TIMER_EVENT_CAPTURE_A == p_args->event ) || ( TIMER_EVENT_CAPTURE_B == p_args->event ) )
#endif
   {
      static uint32_t   cycleCounter;
      static uint32_t   delay;
      static uint32_t   delta;
#if ( MCU_SELECTED == NXP_K24 )
      static uint16_t   currentFTM;
      static uint16_t   capturedValue;
#elif ( MCU_SELECTED == RA6E1 )
//      timer_status_t    status;
      timer_info_t      info;
      static uint32_t   currentFTM;
      static uint32_t   capturedValue;
#endif
      uint32_t primask = __get_PRIMASK();
      // Need to read those 3 counters together so disable interrupts if they are not disabled already
      __disable_interrupt(); // This is critical but fast. Disable all interrupts.
#if ( MCU_SELECTED == NXP_K24 )
      cycleCounter  = DWT_CYCCNT;
      currentFTM    = (uint16_t)FTM3_CNT;
      capturedValue = (uint16_t)FTM3_C1V; // Save current captured value
#elif ( MCU_SELECTED == RA6E1 )
#if 0 // AGT
      (void) R_AGT_InfoGet(&AGT5_ZCD_Meter_ctrl, &info);
      uint32_t period = info.period_counts;
      /* Read the current counter value. Counter value is in status.counter. */
      (void)R_AGT_StatusGet( &AGT5_ZCD_Meter_ctrl, &status );
      /* Process capture from AGTIO. */
      captured_time     = ((uint64_t) period * iCapture_overflows) + p_args->capture;
      iCapture_overflows = 0U;
      cycleCounter  = DWT->CYCCNT;
      currentFTM    = (uint16_t)status.counter;
//      R_AGT_InfoGet() --> Period Count
      capturedValue = (uint16_t)p_args->capture; // TODO: RA6E1: Do we need the captured time vs captured Value?
#else // GPT
      (void) R_GPT_InfoGet(&GPT2_ZCD_Meter_ctrl, &info);
      uint64_t period = info.period_counts;
      /* The maximum period is one more than the maximum 32-bit number, but will be reflected as 0 in
      * timer_info_t::period_counts. */
      if (0U == period)
      {
         period = UINT32_MAX + 1U;
      }
      cycleCounter       = DWT->CYCCNT;
      currentFTM         = R_GPT1->GTCNT;
      capturedValue      = (uint32_t)(period * iCapture_overflows) + p_args->capture;
      iCapture_overflows = 0;

#endif
#endif
      __set_PRIMASK(primask); // Restore interrupts

      // Convert FTM3_CNT into CYCCNT value
#if ( MCU_SELECTED == NXP_K24 )
      delay = ((uint32_t)(uint16_t)(currentFTM - capturedValue))*2;
#elif ( MCU_SELECTED == RA6E1 )
      delay = (uint32_t)((currentFTM - capturedValue)*2);
#endif
      cycleCounter -= delay;

      // Save the captured value
      delta = cycleCounter - zc.PreviousCapturedValue;
      zc.PreviousCapturedValue = cycleCounter;

      // Sanitize the period
      // This should not be necessary but the first delta can be wrong
      if ( (delta > PD_NOMINAL_PERIOD_MIN) && (delta < PD_NOMINAL_PERIOD_MAX) )
      {
         // Compute the average period
         zc.Period = calc_period( zc.Period, delta);
      }
   }
   if (TIMER_EVENT_CYCLE_END == p_args->event)
   {
      /* An overflow occurred during capture. This must be accounted for at the application layer. */
      iCapture_overflows++;
   }
}


/*! ********************************************************************************************************************
 *
 * \fn ConfigAttr_Init
 *
 * \brief  This function sets the phase detection configuration values to default values
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
static void ConfigAttr_Init(bool bWrite)
{
   ConfigAttr_.PD_SurveyPeriod              = PD_SURVEY_PERIOD_DEFAULT;
   ConfigAttr_.PD_SurveyStartTime           = PD_SURVEY_START_TIME_DEFAULT;
   ConfigAttr_.PD_SurveyQuantity            = PD_SURVEY_QUANTITY_DEFAULT;
   ConfigAttr_.PD_SurveyMode                = PD_SURVEY_MODE_DEFAULT;
   ConfigAttr_.PD_ZCProductNullingOffset    = PD_NULLING_OFFSET_DEFAULT;
   memcpy(ConfigAttr_.PD_LcdMsg, PD_LCD_MSG_DEFAULT, sizeof(ConfigAttr_.PD_LcdMsg));

   ConfigAttr_.PD_BuMaxTimeDiversity        = PD_BU_MAX_TIME_DIVERSITY_DEFAULT;
   ConfigAttr_.PD_BuDataRedundancy          = PD_BU_DATA_REDUNDANCY_DEFAULT;
   ConfigAttr_.PD_SurveyPeriodQty           = PD_SURVEY_PERIOD_QTY_DEFAULT;

   if(bWrite)
   {  // Write the data to the file
      (void)FIO_fwrite( &ConfigFile.handle, 0, ConfigFile.Data, ConfigFile.Size);
   }
}

/*! ********************************************************************************************************************
 *
 * \fn CachedAttr_Init
 *
 * \brief
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
static void CachedAttr_Init(bool bWrite)
{
   (void) bWrite;
   (void) memset(&CachedAttr_, 0, sizeof(CachedAttr_));

   if(bWrite)
   {   // Write the data to the file
      (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
   }
}


/*! ********************************************************************************************************************
 *
 * \fn uint8_t  PD_SurveySelfAssessment_Get(void)
 *
 * \brief Determine if the device supports Phase Detect Self Assessment
 *
 * \param none
 *
 * \return false
 *
 ********************************************************************************************************************
 */
uint8_t  PD_SurveySelfAssessment_Get(void)
{
   return false;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyCapable_Get
 *
 * \brief Determine if the device supports Phase Detect Survey
 *
 * \param none
 *
 * \return true
 *
 ********************************************************************************************************************
 */
uint8_t  PD_SurveyCapable_Get(void)
{
   return true;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyStartTime_Get
 *
 * \brief Read the Phase Detect Start Time
 *
 * \param none
 *
 * \return PD_SurveyStartTime
 *
 ********************************************************************************************************************
 */
uint32_t PD_SurveyStartTime_Get()
{
   return ConfigAttr_.PD_SurveyStartTime;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyPeriod_Get
 *
 * \brief Read the Phase Detect Period
 *
 * \param none
 *
 * \return PD_SurveyPeriod
 *
 ********************************************************************************************************************
 */
uint32_t PD_SurveyPeriod_Get()
{
   return ConfigAttr_.PD_SurveyPeriod;
}



/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyPeriodQty_Get
 *
 * \brief Read the Phase Detect Period Quantity
 *
 * \param none
 *
 * \return PD_SurveyPeriod
 *
 ********************************************************************************************************************
 */
uint8_t PD_SurveyPeriodQty_Get()
{
   return ConfigAttr_.PD_SurveyPeriodQty;
}



/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyQuantity_Get
 *
 * \brief Read the Phase Detect Survey Quantity
 *
 * \param none
 *
 * \return PD_SurveyQuantity
 *
 ********************************************************************************************************************
 */
uint8_t PD_SurveyQuantity_Get()
{
   return ConfigAttr_.PD_SurveyQuantity;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyMode_Get
 *
 * \brief Read the Phase Detect Survey Mode
 *
 * \param none
 *
 * \return PD_SurveyReportMode
 *
 ********************************************************************************************************************
 */
uint8_t PD_SurveyMode_Get()
{
   return ConfigAttr_.PD_SurveyMode;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_ZCProductNullingOffset_Get
 *
 * \brief Read the Phase Detect ZC Product Nulling Offset
 *
 * \param none
 *
 * \return PD_ZCProductNullingOffset
 *
 ********************************************************************************************************************
 */
int32_t PD_ZCProductNullingOffset_Get()
{
   return ConfigAttr_.PD_ZCProductNullingOffset;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_LcdMsg_Get
 *
 * \brief Read the Phase Detect Lcd Message
 *
 * \param none
 *
 * \return Message Length
 *
 ********************************************************************************************************************
 */
uint16_t PD_LcdMsg_Get(char* result)
{
   memcpy(result, ConfigAttr_.PD_LcdMsg, PD_LCD_MSG_SIZE);
   return PD_LCD_MSG_SIZE;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyStartTime_Set
 *
 * \brief API to Set the Start Time
 *
 * \details  The start time should be less than the period
 *
 * \param  number of seconds from the start of the period
 *
 * \return STATUS_e
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_SurveyStartTime_Set (uint32_t value)
{
   returnStatus_t retVal = eSUCCESS;

   // Verify if value requested is valid for the parameter
   if (value < ConfigAttr_.PD_SurveyPeriod)
   {
      if ( ConfigAttr_.PD_SurveyStartTime != value)
      {
         // Begin critical section
         OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

         ConfigAttr_.PD_SurveyStartTime = value;
         retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

         /* End critical section */
         OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails

         SurveyLog_ResetAll();
      }
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyPeriod_Set
 *
 * \brief API to Set the Survey Period
 *
 * \param
 *
 * \return STATUS_e
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_SurveyPeriod_Set (uint32_t value)
{
   returnStatus_t retVal = eSUCCESS;

   // Verify if value requested is valid for the parameter
   if (value ==   120 ||    //  0:02
       value ==   300 ||    //  0:05
       value ==   900 ||    //  0:15
       value ==  1800 ||    //  0:30
       value ==  3600 ||    //  1:00
       value ==  7200 ||    //  2:00
       value == 10800 ||    //  3:00
       value == 14400 ||    //  4:00
       value == 21600 ||    //  6:00
       value == 28800 ||    //  8:00
       value == 43200 ||    // 12:00
       value == 86400)      // 24:00
   {
      // Rules
      // - pdSurveyPeriod must be > pdBuMaxTimeDiversity * 60
      // - pdSurveyPeriod must be > pdSurveyStart
      if(( value > ( ConfigAttr_.PD_BuMaxTimeDiversity*60 )) &&
         ( value >   ConfigAttr_.PD_SurveyStartTime))
      {
         if (ConfigAttr_.PD_SurveyPeriod != value)
         {

            // Begin critical section
            OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

            ConfigAttr_.PD_SurveyPeriod = value;
            retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

            /* End critical section */
            OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails

            SurveyLog_ResetAll();
         }
      }
      else
      {
         retVal = eAPP_INVALID_VALUE;
      }
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyPeriodQty_Set
 *
 * \brief API to Set the Survey Period Quantity
 *
 * \param
 *
 * \return STATUS_e
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_SurveyPeriodQty_Set (uint8_t value)
{
   returnStatus_t retVal = eSUCCESS;

   // Verify if value requested is valid for the parameter
   if (value <= PD_SURVEY_PERIOD_QTY_MAX)
   {
      if (value != ConfigAttr_.PD_SurveyPeriodQty)
      {
         // Begin critical section
         OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

         ConfigAttr_.PD_SurveyPeriodQty = value;
         retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

         /* End critical section */
         OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails

         SurveyLog_ResetAll();
      }
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }
   return retVal;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyQuantity_Set
 *
 * \brief  Set the Survey Quanity
 *
 * \param  SurveyQuantity 1-4
 *
 * \return STATUS_e
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_SurveyQuantity_Set(uint8_t value)
{
   returnStatus_t retVal = eSUCCESS;

   // Verify if value requested is valid for the parameter
   if ((value > 0) && (value <= PD_MAX_SURVEYS))
   {
      if (value != ConfigAttr_.PD_SurveyQuantity)
      {
         // Begin critical section
         OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

         ConfigAttr_.PD_SurveyQuantity = value;
         retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

         /* End critical section */
         OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails

         SurveyLog_ResetAll();
      }
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_SurveyMode_Set
 *
 * \brief  API to Set the Survey Mode
 *
 * \details Changing the mode will cause the uase ...
 *
 * \param  SurveyReportMode  (0-1)
 *         0 = disabled
 *         1 = enabled
 *
 * \return STATUS_e
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_SurveyMode_Set (uint8_t value)
{

   returnStatus_t retVal = eSUCCESS;

   // Verify if value requested is valid for the parameter
   if (value <= PD_SURVEY_MODE_ENABLED)
   {
      if (((value == 0 ) && (ConfigAttr_.PD_SurveyMode == 1)) ||
          ((value == 1 ) && (ConfigAttr_.PD_SurveyMode == 0)))
      {
         // Begin critical section

         OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

         ConfigAttr_.PD_SurveyMode = value;
         retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

         /* End critical section */
         OS_MUTEX_Unlock( &configMutex_ );   // Function will not return if it fails

         SurveyLog_ResetAll();

      }
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }

   return retVal;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_ZCProductNullingOffset_Set
 *
 * \brief Set the ProductNullingOffset
 *
 * \param ProductNullingOffset (-35999 +35999 )
 *           [-359.99 degrees  to  +359.99  degrees]
 *
 * \return STATUS_e
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_ZCProductNullingOffset_Set(int32_t value)
{
   returnStatus_t retVal = eFAILURE;
   if ((value > PD_NULLING_OFFSET_MIN) && (value < PD_NULLING_OFFSET_MAX))
   {
      // Begin critical section
      OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

      ConfigAttr_.PD_ZCProductNullingOffset = value;
      retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

      /* End critical section */
      OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails
   }
   return retVal;
}

/*! ********************************************************************************************************************
 *
 * \fn LcdMsg_Set
 *
 * \brief private API to Set the LcdMsg
 *
 * \param  pointer to the new message
 *
 * \return  STATUS_e
 *
 *  A message to be displayed on the meter LCD describing the meter’s phase connection.
 *  A msg shorter than 6 characters will be padded with spaces to become 6.
 *  Any msg longer than 6 characters will be rejected.
 *  This string is volatile in the meter (MT119 NETWORK_STATUS_INFO entry zero) and must be hosted in the EndPoint
 *  and refreshed at powerup.
 *  Access = RW
 *
 *  The following charaters are allowed "ABCDEFGHIJLNOPQRSTUY0123456789-= "
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_LcdMsg_Set( uint8_t const * string)
{
   returnStatus_t retVal = eSUCCESS;

   // List of valid characters allowed
   static const char valid_chars[] = "ABCDEFGHIJLNOPQRSTUY0123456789-= ";
   uint8_t stringLength = (uint8_t)strlen((char *)string);

   // check to see supplied argument is too large
   if( PD_LCD_MSG_SIZE >= stringLength )
   {
      // loop through string checking if all of the characters are valid
      for (int i = 0; i < stringLength; i++)
      {
         // Check if the characters is in the list of valid characters
         // For some reason 0 is valid, so make if invalid
         if(!strchr(valid_chars, string[i]) || string[i] == 0 )
         {
            retVal = eAPP_INVALID_VALUE;
            break;
         }
      }

      if( retVal == eSUCCESS)
      {  // All charaters are allowed so update the

         // Begin critical section
         OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

         // initialize all characters to whitespace for trailing whitespace
         ( void )memset( ConfigAttr_.PD_LcdMsg, WHITE_SPACE_CHAR, sizeof(ConfigAttr_.PD_LcdMsg) );

         // Set the value, limited to 6 bytes or less by initial if check
         memcpy(ConfigAttr_.PD_LcdMsg, string, stringLength);
         ( void )FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

         /* End critical section */
         OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails

#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
         HMC_DISP_UpdateDisplayBuffer( ConfigAttr_.PD_LcdMsg, HMC_DISP_POS_PD_LCD_MSG ); //Update the Display
#endif
         retVal = eSUCCESS;
      }
   }
   else
   {
      retVal = eAPP_OVERFLOW_CONDITION;
   }
   return retVal;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_BuMaxTimeDiversity_Set
 *
 * \brief
 *
 * \param  value (minutes)
 *
 * \return
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_BuMaxTimeDiversity_Set(uint16_t value)
{
   returnStatus_t retVal = eSUCCESS;
   if ( (value >= PD_BU_MAX_TIME_DIVERSITY_MIN   ) &&
        (value <  ConfigAttr_.PD_SurveyPeriod/60 )  )
   {
      // Begin critical section
      OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

      ConfigAttr_.PD_BuMaxTimeDiversity = value;
      retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

      /* End critical section */
      OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails
   }
   else
   {
      retVal = eAPP_INVALID_VALUE;
   }
   return retVal;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_BuMaxTimeDiversity_Get
 *
 * \brief
 *
 * \param  none
 *
 * \return
 *
 ********************************************************************************************************************
 */
uint16_t PD_BuMaxTimeDiversity_Get()
{
   return ConfigAttr_.PD_BuMaxTimeDiversity;
}


/*! ********************************************************************************************************************
 *
 * \fn PD_BuDataRedundancy_Set
 *
 * \brief
 *
 * \param  none
 *
 * \return
 *
 ********************************************************************************************************************
 */
returnStatus_t PD_BuDataRedundancy_Set(uint8_t value)
{
   returnStatus_t retVal = eSUCCESS;
   if (value <= PD_BU_DATA_REDUNDANCY_MAX)
   {
      // Begin critical section
      OS_MUTEX_Lock( &configMutex_ ); // Function will not return if it fails

      ConfigAttr_.PD_BuDataRedundancy = value;
      retVal = FIO_fwrite( &ConfigFile.handle, 0, (uint8_t *)&ConfigAttr_, (lCnt) sizeof(ConfigAttr_));

      /* End critical section */
      OS_MUTEX_Unlock( &configMutex_); // Function will not return if it fails
   }
   return retVal;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_BuDataRedundancy_Get
 *
 * \brief
 *
 * \param  none
 *
 * \return
 *
 ********************************************************************************************************************
 */
uint8_t PD_BuDataRedundancy_Get()
{
   return ConfigAttr_.PD_BuDataRedundancy;
}


/*! ********************************************************************************************************************
 *
 * \fn  PD_AddSysTimer
 *
 * \brief This is to used to create a time alarm for the phase detection module
 *
 * \details The purpose of this timer is to periodically start a new survey period.
 *          This alarm used by the MAC's message queue, so the MAC must call PD_HandleSysTimer_Event
 *          to process the alarm.
 *          The start time must be less than the period
 *
 * \param none
 *
 * \return void
 *
 ********************************************************************************************************************
 */
void PD_AddSysTimer ( void )
{
   tTimeSysPerAlarm alarmSettings;

   if(ConfigAttr_.PD_SurveyMode > 0 )
   {
      // The start time must be less than the period
      // If the start time is >= period, do not set the timer
      if(ConfigAttr_.PD_SurveyPeriod >= ConfigAttr_.PD_SurveyStartTime)
      {
         INFO_printf("period:%d starttime:%d", ConfigAttr_.PD_SurveyPeriod, ConfigAttr_.PD_SurveyStartTime );

         if (alarmId_ != NO_ALARM_FOUND)
         { // Alarm already active, delete it
            (void)TIME_SYS_DeleteAlarm (alarmId_) ;
            alarmId_ = NO_ALARM_FOUND;
         }


         //Set up a periodic alarm
         (void)memset(&alarmSettings, 0, sizeof(alarmSettings));   // Clear settings, only set what we need
         alarmSettings.bOnValidTime    = true;                     /* Alarmed on valid time */
         alarmSettings.bSkipTimeChange = false;                    /* do NOT notify on time change */
         alarmSettings.pMQueueHandle   = MAC_GetMsgQueue();        /* Uses the MAC message Queue? */
         alarmSettings.ulPeriod        = ConfigAttr_.PD_SurveyPeriod    * TIME_TICKS_PER_SEC; ;
         alarmSettings.ulOffset        = ConfigAttr_.PD_SurveyStartTime * TIME_TICKS_PER_SEC;
         alarmSettings.ulOffset        = (alarmSettings.ulOffset / SYS_TIME_TICK_IN_mS) * SYS_TIME_TICK_IN_mS; //round down to the tick
         (void)TIME_SYS_AddPerAlarm(&alarmSettings);
         alarmId_ = alarmSettings.ucAlarmId;


         TIMESTAMP_t Time;
         (void) TIME_UTIL_GetTimeInSecondsFormat( &Time );
         uint32_t CurrentTime = Time.seconds;

         CachedAttr_.CurrentStartTime = CalcStartTime(CurrentTime);
         (void)FIO_fwrite( &CachedFile.handle, 0, CachedFile.Data, CachedFile.Size);
         PD_print_time(CachedAttr_.CurrentStartTime);
      }
      else{
         INFO_printf("Invalid Period/StartTime: %d %d", ConfigAttr_.PD_SurveyPeriod, ConfigAttr_.PD_SurveyStartTime );
      }
   }
}

/*! ********************************************************************************************************************
 *
 * \fn bool PD_HandleSysTimer_Event( const tTimeSysMsg *alarmMsg )
 *
 * \brief This is called to handle the time event that was created by PD_AddSysTimer
 *
 * \details If the alarm id matches the local alarmId it will reset the Phase Detection Survey Counters
 *          so a new set of survey messages can be recorded
 *
 * \param  pointer to the alarm message
 *
 * \return bool false - Alarm Not Handled
 *              true  - Alarm Handled
 *
 ********************************************************************************************************************
 */

bool PD_HandleSysTimer_Event( const tTimeSysMsg *alarmMsg )
{
   if (alarmMsg->alarmId == alarmId_)
   {
      INFO_printf("PD_TimeAlarm");

      uint32_t eventTime_sec = (alarmMsg->date*SECONDS_PER_DAY) + alarmMsg->time/1000;

      if (alarmMsg->bTimeChangeForward)
      {  // time jumped forward
         INFO_printf("TimeChangeForward");

         // Was the time jump past the end of the period?
         if ( eventTime_sec >= (CachedAttr_.CurrentStartTime + ConfigAttr_.PD_SurveyPeriod))
         {  // Jumped forward across the boundary
            // Send the current and previous data
            INFO_printf("New period!!!");

            buffer_t *pBuf;
            pBuf = BuildSurveyMessage(1);
            if (pBuf)
            {
               PD_Tx_WithTimeDiversity(pBuf, ConfigAttr_.PD_BuDataRedundancy);
            }

            // Determine the amount past the end of the period
            uint32_t forward_delta   = eventTime_sec - (CachedAttr_.CurrentStartTime + ConfigAttr_.PD_SurveyPeriod);
            uint32_t forward_periods = forward_delta/ConfigAttr_.PD_SurveyPeriod;

            // limit to the number of history buffers
            forward_periods = min(PD_HISTORY_CNT, forward_periods);
            while (forward_periods-- > 0)
            {
               next_buffer();
            }

            // Advance to the next survey period
            SurveyLog_NewPeriod();
         }
         else{
            // in same interval
         }
      }
      else if (alarmMsg->bTimeChangeBackward)
      {  // time jumped backward
         INFO_printf("TimeChangeBackward");
         if ( eventTime_sec < CachedAttr_.CurrentStartTime)
         {
            //this should not occur, clear the buffers, and start again
            INFO_printf("Previous period");
            SurveyLog_ResetAll();
         }
      }
      else{
         buffer_t *pBuf;
         pBuf = BuildSurveyMessage(1);
         if (pBuf)
         {
            PD_Tx_WithTimeDiversity(pBuf, ConfigAttr_.PD_BuDataRedundancy);
         }
         // Advance to the next survey period
         SurveyLog_NewPeriod();
      }
   }
   else{
      return false;
   }
   return true;
}


/*! ********************************************************************************************************************
 *
 * \fn
 *
 * \brief
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
static void PD_print_time(uint32_t StartTime)
{
   sysTime_t            sysTime;
   sysTime_dateFormat_t sysDateFormat;
   TIME_UTIL_ConvertSecondsToSysFormat(StartTime, 0, &sysTime);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat(&sysTime, &sysDateFormat);
   INFO_printf("PD:StartTime=%02d/%02d/%04d %02d:%02d:%02d",
                 sysDateFormat.month, sysDateFormat.day, sysDateFormat.year, sysDateFormat.hour, sysDateFormat.min, sysDateFormat.sec); /*lint !e123 */
}

/*! ********************************************************************************************************************
 *
 * \fn PD_print_CachedData()
 *
 * \brief This function is used to print the data that is saved in the cache for debugging
 *
 * \param none
 *
 * \return none
 *
 ********************************************************************************************************************
 */
static void PD_print_CachedData()
{
   PD_print_time(CachedAttr_.CurrentStartTime);

   SurveyPeriod_print();

}

/*! ********************************************************************************************************************
 *
 * \fn void PD_print_freq()
 *
 * \brief Used to print the estimated line frequency for debugging
 *
 * \param none
 *
 * \return none
 *
 ********************************************************************************************************************
 */
static void PD_print_freq()
{
   uint16_t freq = (uint16_t) (((uint64_t)PD_NominalPeriod() * (uint64_t)PD_LINE_FREQ * (uint64_t)100) / (uint64_t)zc.Period);
   INFO_printf("Freq:%u.%02u period:%u", freq/100, freq%100, zc.Period );
}

/*! ********************************************************************************************************************
 *
 * \fn printSyncInfo
 *
 * \brief Used to print the sync record to the debug port
 *
 * \param pointer to the sync_record_t
 *
 * \return void
 *
 ********************************************************************************************************************
 */
static void printSyncInfo(const sync_record_t *pSyncRecord)
{
   uint32_t CPUfreq; // Hold best real CPU frequency estimate

   (void)TIME_SYS_GetRealCpuFreq( &CPUfreq, NULL, NULL );

   INFO_printf(
      "sync:%u zcd:%u period:%u CPU freq:%u",
      pSyncRecord->syncTimeCYCCNT,
      pSyncRecord->zcd_count,
      pSyncRecord->zcd_period,
      CPUfreq);

   // For debug, display the estimated frequency
   uint16_t freq = (uint16_t) (((uint64_t)PD_NominalPeriod() * (uint64_t)PD_LINE_FREQ * (uint64_t)100) / (uint64_t)zc.Period);
   INFO_printf("Freq:%u.%02u ", freq/100, freq%100);

}

/*! ********************************************************************************************************************
 *
 * \fn  printPhaseData
 *
 * \brief Used to print the source id and the phase angle for the message
 *
 * \param src_addr   - Originator of the messahge
 *        phaseAngle - Calculated angle
 *
 * \return
 *
 ********************************************************************************************************************
 */
static void printPhaseData(
   const uint8_t src_addr[],
   uint16_t      phaseAngle)
{
   float    freq;
   uint32_t SyncToZc;

   freq = ((float)PD_NominalPeriod() * (float)PD_LINE_FREQ) / (float)zc.Period;
   SyncToZc = (uint32_t)(((float)phaseAngle*100000.0f/(float)PD_MAX_ANGLE)/freq);

   INFO_printf(
     "PD src_id:%02x:%02x:%02x:%02x:%02x "
     "Angle = %u.%02u Sync->ZC:%u.%02ums",
      src_addr[0], src_addr[1], src_addr[2], src_addr[3], src_addr[4],
      phaseAngle/100, phaseAngle%100,
      SyncToZc/100, SyncToZc%100);
}

#if 0
/*! ********************************************************************************************************************
 *
 * \fn
 *
 * \brief
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
static void DBG_printTime(TIMESTAMP_t sync_time)
{
   sysTime_t Time;
   sysTime_dateFormat_t sysTime;

   TIME_UTIL_ConvertSecondsToSysFormat(sync_time.time, sync_time.fracTime, &Time);
   (void) TIME_UTIL_ConvertSysFormatToDateFormat( &Time, &sysTime );
   INFO_printf( "PD_SyncTime: %02d/%02d/%04d %02d:%02d:%02d.%03d",
                       sysTime.month, sysTime.day, sysTime.year,
                       sysTime.hour,  sysTime.min, sysTime.sec, sysTime.msec);
}

/*! ********************************************************************************************************************
 *
 * \fn  void PD_PrintConfig(void)
 *
 * \brief   Used to display the configuration values
 *
 * \param   none
 *
 * \return  none
 *
 ********************************************************************************************************************
 */
void PD_PrintConfig(void)
{
   INFO_printf("PD_Config:");
   INFO_printf("SurveyStartTime      %02d:%02d:%02d",
                  ConfigAttr_.PD_SurveyStartTime  / (60*60),
                  ConfigAttr_.PD_SurveyStartTime  % (60*60)/60,
                  ConfigAttr_.PD_SurveyStartTime  % 60);
   INFO_printf("SurveyQuantity       %d", ConfigAttr_.PD_SurveyQuantity );
   INFO_printf("SurveyReportMode     %d", ConfigAttr_.PD_SurveyReportMode);
   INFO_printf("ProductNullingOffset %d", ConfigAttr_.PD_ZCProductNullingOffset);
   INFO_printf("PD_LcdMsg          '%s'", ConfigAttr_.PD_LcdMsg);
}

#endif


/*! ********************************************************************************************************************
 *
 * \fn  PD_DebugCommandLine
 *
 * \brief   This function handles all of the Phase Detect Debug Command Line Options
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
uint32_t PD_DebugCommandLine ( uint32_t argc, char* const argv[] )
{
   char* option = argv[0];

   typedef struct{
      char* option;
      void (*func)(void);
   } cmds_t;

   cmds_t cmds[] = {
     { "info",   PD_print_CachedData },
     { "freq",   PD_print_freq }};

   if ( argc > 1 )
   {
      for( unsigned int i=0; i < (sizeof(cmds)/sizeof(*(cmds))); i++ )
      {
         if ( strcasecmp( cmds[i].option, argv[1] ) == 0 )
         {
            cmds[i].func();
            break;
         }
      }
   }
   else{
      DBG_logPrintf( 'R', "Usage:%s %s ", option, "{info | freq }" );
   }
   return 0;
}

/*! ********************************************************************************************************************
 *
 * \fn  PD_TimeDiversityTimerExpired
 *
 * \brief   This is called when the time diversity timer expires
 *
 * \details It will post a message the the message queue indicating that the timer has expired
 *          This currently uses the MAC's message queue.   This saves adding a task for this purpose.
 *
 * \param none
 *
 * \return
 *
 ********************************************************************************************************************
 */
static void PD_TimeDiversityTimerExpired(uint8_t cmd, void *pData)
{
   (void) cmd;
   (void) pData;

   static buffer_t buf;

   /***
   * Use a static buffer instead of allocating one because of the allocation fails,
   * there wouldn't be much we could do - we couldn't reset the timer to try again
   * because we're already in the timer callback.
   ***/
   BM_AllocStatic(&buf, eSYSFMT_TIME_DIVERSITY);

   OS_MSGQ_Post(MAC_GetMsgQueue(), (void *)&buf);
}

/*! ********************************************************************************************************************
 *
 * \fn PD_SetTimeDiversityTimer
 *
 * \brief  This is called to set the time diversity timer
 *
 * \param  Number of milliseconds to delay
 *
 * \return status
 *
 ********************************************************************************************************************
 */
static
returnStatus_t PD_SetTimeDiversityTimer(uint32_t timeMilliseconds)
{

   INFO_printf("PD Time Diversity: %u Milliseconds", timeMilliseconds);

   returnStatus_t retStatus;

   if ( timeDiversity_.TimerId  == 0 )
   {  //First time, add the timer
      timer_t tmrSettings;             // Timer for sending pending ID bubble up message

      (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
      tmrSettings.bOneShot = true;
      tmrSettings.bOneShotDelete = false;
      tmrSettings.bExpired = false;
      tmrSettings.bStopped = false;
      if ( timeMilliseconds > 0 )
      {
         tmrSettings.ulDuration_mS = timeMilliseconds;
      }else{
         tmrSettings.ulDuration_mS = 1;
      }

      tmrSettings.pFunctCallBack = PD_TimeDiversityTimerExpired;

      retStatus = TMR_AddTimer(&tmrSettings);
      if (eSUCCESS == retStatus)
      {
         timeDiversity_.TimerId = tmrSettings.usiTimerId;
      }
      else
      {
         ERR_printf("PD unable to add timer");
      }
   }
   else
   {  //Timer already exists, reset the timer
      retStatus = TMR_ResetTimer(timeDiversity_.TimerId, timeMilliseconds);
   }
   return(retStatus);
}

/*! ********************************************************************************************************************
 *
 * \fn PD_HandleTimeDiversity_Event
 *
 * \brief This is called when a SYSFMT_TIME_DIVERSITY event occurs.
 *
 * \details  The purpose of this is to send the message what was pending, which is a pd/bu message.
 *
 * \param none
 *
 * \return void
 *
 ********************************************************************************************************************
 */
void PD_HandleTimeDiversity_Event(void)
{
   if (timeDiversity_.RepeatCount > 0)
   {
      timeDiversity_.RepeatCount--;

      // Make a copy of the message
      buffer_t *pMessageCopy;
      pMessageCopy = BM_alloc(timeDiversity_.pMessage->x.dataLen);
      if ( pMessageCopy != NULL )
      {
         (void)memcpy(&pMessageCopy->data[0], &timeDiversity_.pMessage->data[0], timeDiversity_.pMessage->x.dataLen);
         pMessageCopy->x.dataLen = timeDiversity_.pMessage->x.dataLen;
         PD_TxBuMsg(pMessageCopy);
      }

      // Ensure that the message is transmitted before the end of the period
      {
         TIMESTAMP_t CurrentTime;
         (void) TIME_UTIL_GetTimeInSecondsFormat( &CurrentTime );

         uint32_t time_remaining = (CachedAttr_.CurrentStartTime + ConfigAttr_.PD_SurveyPeriod) - CurrentTime.seconds;
         INFO_printf("Time Remaining in Period:%d seconds", time_remaining );

         uint32_t maxTimeDiversity_sec = min((ConfigAttr_.PD_BuMaxTimeDiversity * 60), time_remaining);
         uint32_t timeMilliseconds = aclara_randu(1, (maxTimeDiversity_sec * TIME_TICKS_PER_SEC));
         (void)PD_SetTimeDiversityTimer( timeMilliseconds );
      }
   }else
   {
      PD_TxBuMsg(timeDiversity_.pMessage);
      timeDiversity_.pMessage = NULL;
   }
}


/*! ********************************************************************************************************************
 *
 * \fn  PD_TxBuMsg
 *
 * \brief Transmits the bu_pd message
 *
 * \param pMessage Pointer to the message to transmit
 *
 * \return void
 *
 ********************************************************************************************************************
 */
static void PD_TxBuMsg(const buffer_t* pMessage)
{
   // Time diversity message timer expired, so send the pending message.
   if (pMessage != NULL)
   {
      // Bubble up phase detection
      HEEP_APPHDR_t      heepHdr;          // Application header/QOS info
      HEEP_initHeader( &heepHdr );

      heepHdr.TransactionType     = (uint8_t)TT_ASYNC;
      heepHdr.Resource            = (uint8_t)bu_pd;
      heepHdr.RspID               = 0;
      heepHdr.qos                 = 0;
      heepHdr.Method              = (uint8_t)method_post;
      heepHdr.callback = NULL;

      (void)HEEP_MSG_Tx(&heepHdr, pMessage); /*lint !e603 heepHdr initialized if pMessage not null; see below  */
   }
}



/*! ********************************************************************************************************************
 *
 * \fn  PD_Tx_WithTimeDiversity
 *
 * \brief This will transmit the message with time diversity
 *
 * \details If the time diversity is > 0 the message buffer will be saved and then transmitted until after the delay.
 *
 * \param pBuf - Poiner to the message to be transmitted
 *
 * \return void
 *
 ********************************************************************************************************************
 */
static void PD_Tx_WithTimeDiversity(buffer_t *pBuf, uint8_t num_repeats)
{
   // Ensure that the message is transmitted within the period
   uint32_t maxTimeDiversity_sec = min((ConfigAttr_.PD_BuMaxTimeDiversity * 60), ConfigAttr_.PD_SurveyPeriod);
   uint32_t timeMilliseconds = aclara_randu(0, (maxTimeDiversity_sec * TIME_TICKS_PER_SEC));

   if (timeDiversity_.pMessage != NULL)
   {
      // Previous Message not sent
      BM_free (timeDiversity_.pMessage);
   }
   timeDiversity_.pMessage = pBuf;
   timeDiversity_.RepeatCount = num_repeats;
   (void)PD_SetTimeDiversityTimer( timeMilliseconds );
}


/*! ********************************************************************************************************************
 *
 * \fn  BuildSurveyMessage
 *
 * \brief Builds the phase detection bubble up message
 *
 * \details This builds the data portion of the bu/pd
 *
 * \param SurveyPeriodQty
 *         0 - Data from the current period only
 *         1 - Data from the all periods
 *
 * \return pBuffer Pointer to the message data
 *
 ********************************************************************************************************************
 */
static
buffer_t *BuildSurveyMessage(uint8_t SurveyPeriodQty)
{
   uint8_t num_messages = 0;

   if (ConfigAttr_.PD_SurveyMode > 0)
   {
      // Start with the current index
      int set_index = CachedAttr_.SurveyPeriods.currentIndex;

      // Count the number of measurements recorded, for both the current and previous period
      uint8_t survey_set = 0;
      do
      {
         SurveyMeasurments_t *pSurveyMeasurements = &CachedAttr_.SurveyPeriods.SurveyMeasurments[set_index];
         num_messages += pSurveyMeasurements->num_entries;
         set_index = (set_index + 1 ) % PD_HISTORY_CNT;
      }
      while (++survey_set <= ConfigAttr_.PD_SurveyPeriodQty && SurveyPeriodQty == 1);
   }

   INFO_printf("BuildSurveyMessage:%d", num_messages);

   buffer_t *pBuf = NULL;

   // There are survey measurments to send?
   if(num_messages > 0)
   {
      // Allocate a buffer to accommodate worst case payload.
      // Overhead = 1
      // Each reading consists of Angle (2)   + overhead (5) = 7
      // Each reading consists of Handle(1-8) + overhead (5) = 13 max
      pBuf = BM_alloc( num_messages * 21 );
      pBuf->x.dataLen = 0;
      uint8_t *pData = pBuf->data;

      // Start with the current index
      int set_index = CachedAttr_.SurveyPeriods.currentIndex;
      int survey_set = 0;

      // for each Survey Period
      do
      {
         SurveyMeasurments_t *pSurveyMeasurements = &CachedAttr_.SurveyPeriods.SurveyMeasurments[set_index];

         for (int slot = 0; slot < pSurveyMeasurements->num_entries; slot++)
         {
            struct readings_s readings;

            // No Time stamp
            HEEP_ReadingsInit(&readings, 0, pData);

            // Add the pdBeaconHandle
            HEEP_Put_U64( &readings, pdBeaconHandle, pSurveyMeasurements->data[slot].handle_id);

            // Add the phase angle with power of 10 = 2, i.e. 35999 == 359.99
            HEEP_Put_U16( &readings, pdPhaseAngle,   pSurveyMeasurements->data[slot].angle, 2);

            pData += readings.size;
            pBuf->x.dataLen += readings.size;
         }
         set_index = (set_index + 1 ) % PD_HISTORY_CNT;
      }
      while (++survey_set <= ConfigAttr_.PD_SurveyPeriodQty && SurveyPeriodQty == 1);
   }
   return pBuf;
}

/*! ********************************************************************************************************************
 *
 * \fn PD_HistoricalRecoveryHandler
 *
 * \brief  This handles the /bu/pd request.
 *
 * \details
 *    type     = request
 *    resource = /bu/pd
 *    eventQty = 0
 *    readingQty = 0 or 1
 *    readingQty = 0
 *       If no data is supplied in request, all of the surveys captured during the current survey period.
 *
 *    readingQty = 1
 *    readingType = pdSurveyPeriodQty
 *    pdSurveyPeriodQty = 0
 *       Only results from the current period should be returned.
 *    pdSurveyPeriodQty = 1
 *       All of the current data and the previous period will be returned
 *
 * \param
 *
 * \return void
 *
 ********************************************************************************************************************
 */
/*lint -esym(818,payloadBuf,appMsgRxInfo) */
void PD_HistoricalRecoveryHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   ( void )length;

   buffer_t *pBuf = NULL;

   HEEP_APPHDR_t     heepHdr;          // Application header/QOS info
   HEEP_initHeader( &heepHdr );
   heepHdr.TransactionType     = (uint8_t)TT_RESPONSE;
   heepHdr.Resource            = (uint8_t)bu_pd;
   heepHdr.Method_Status       = (uint8_t)OK;
   heepHdr.Req_Resp_ID         = (uint8_t)appMsgRxInfo->Req_Resp_ID;
   heepHdr.appSecurityAuthMode = appMsgRxInfo->appSecurityAuthMode;
   heepHdr.qos                 = appMsgRxInfo->qos;
   heepHdr.TransactionContext  = appMsgRxInfo->TransactionContext;
   heepHdr.callback = NULL;

   if ( (enum_MessageMethod)appMsgRxInfo->Method != method_get )
   {
      /* unexpected request method field */
      heepHdr.Method_Status       = (uint8_t)BadRequest;
      ERR_printf("unexpected method field to /pd (%d), expected (%d)", appMsgRxInfo->Method, method_get);
   }else
   {
      EventRdgQty_u  qtys;      // For EventTypeQty and ReadingTypeQty

      uint16_t bits  = 0;       // Keeps track of the bit position within the Bitfields
      uint8_t  bytes = 0;

      qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);
      bytes += 1;

      if (qtys.bits.eventQty > 0)
      {
         heepHdr.Method_Status       = (uint8_t)BadRequest;
         ERR_printf("expected EventQty=0");
      }else
      {
         if(qtys.bits.rdgQty > 1)
         {
            heepHdr.Method_Status       = (uint8_t)BadRequest;
            ERR_printf("expected rdgQty<2");
         }
         else{

            // If there is a reading then decode it
            uint8_t SurveyPeriodQty = 255;

            if(qtys.bits.rdgQty == 1)
            {
               meterReadingType           readingType;   // Reading Type
               ExWId_ReadingInfo_u        readingInfo;   // Reading Info for the reading Type and Value

               // Copy the 3 bytes of reading info
               (void)memcpy(&readingInfo.bytes[0], &((uint8_t *)payloadBuf)[bytes], sizeof(readingInfo.bytes));
               bits       += READING_INFO_SIZE_BITS;

               // Get the Reading Type ( 2 bytes )
               readingType = (meterReadingType)UNPACK_bits2_uint16(payloadBuf, &bits, sizeof(readingType) * 8);

               // Is the pdSurveyPeriodQty included
               if (readingType == pdSurveyPeriodQty)
               {
                  if((readingInfo.bits.typecast   == ( uint16_t )uintValue) &&
                     (readingInfo.bits.RdgValSize == 1))
                  {
                     SurveyPeriodQty = UNPACK_bits2_uint8(payloadBuf, &bits, readingInfo.bits.RdgValSize * 8);

                     if(SurveyPeriodQty > 1)
                     {
                        // Invalid parameter for this message
                        ERR_printf("expected SurveyPeriodQty <= 1");
                        heepHdr.Method_Status       = (uint8_t)BadRequest;
                     }
                  }
                  else{
                     // Invalid parameter for this message
                     ERR_printf("expected type=uint and size= 1");
                     heepHdr.Method_Status       = (uint8_t)BadRequest;
                  }
               }
               else{
                  // Invalid parameter for this message
                  ERR_printf("expected readingType=pdSurveyPeriodQty");
                  heepHdr.Method_Status       = (uint8_t)BadRequest;
               }
            }
            else{
               // Default to 0
               SurveyPeriodQty = 0;
            }

            if (heepHdr.Method_Status  == (uint8_t)OK)
            {
               pBuf = BuildSurveyMessage(SurveyPeriodQty);
            }
         }
      }
   }
   if (pBuf != NULL)
   {
      (void)HEEP_MSG_Tx(&heepHdr, pBuf);
   }
   else{
      (void)HEEP_MSG_TxHeepHdrOnly(&heepHdr);
   }
}



/*! ********************************************************************************************************************
 *
 * \fn PD_MsgHandler
 *
 * \brief Phase Detection Survey Message
 *
 * \param
 *
 * \return
 *
 ********************************************************************************************************************
 */
 /*lint -esym(818,payloadBuf,appMsgRxInfo) */
void PD_MsgHandler(HEEP_APPHDR_t *appMsgRxInfo, void *payloadBuf, uint16_t length)
{
   ( void )length;

   INFO_printf("/pd Message");

   // the /pd method must be a post
   if ( (enum_MessageMethod)appMsgRxInfo->Method != method_post )
   {
      /* unexpected request method field */
      ERR_printf("unexpected method field to /pd (%d), expected (%d)", appMsgRxInfo->Method, method_post);
   }else
   {
      EventRdgQty_u  qtys;          // For EventTypeQty and ReadingTypeQty

      uint16_t       bits  = 0;       // Keeps track of the bit position within the Bitfields

      qtys.EventRdgQty = UNPACK_bits2_uint8(payloadBuf, &bits, EVENT_AND_READING_QTY_SIZE_BITS);

      if (qtys.bits.eventQty != 0)
      {
         ERR_printf("EventQty expected to be zero in /pd handler");
      }else
      {
         if(qtys.bits.rdgQty != 1)
         {
            ERR_printf("rdgQty expected to be 1 in /pd handler");
         }
         else
         {
            ( void )UNPACK_bits2_uint8 (payloadBuf, &bits,  8);
            uint16_t readingSize       = UNPACK_bits2_uint16(payloadBuf, &bits, 12);
            uint8_t  readingTypeCast   = UNPACK_bits2_uint8 (payloadBuf, &bits,  4);
            uint16_t readingType       = UNPACK_bits2_uint16(payloadBuf, &bits, 16);

            // The readingType  must be pdBeaconHandle
            // The typeCast     must be hexBinary
            // The handle value must at least 4 and less then 13???
            if((readingType     == ( uint16_t )pdBeaconHandle) &&
               (readingTypeCast == ( uint8_t )uintValue )     &&
               ((readingSize  > 0) && (readingSize <= 8 )))
            {
               uint64_t handle_id;
               handle_id = UNPACK_bits2_uint64(payloadBuf, &bits, readingSize*8); /*lint !e734 */

               sync_record_t SyncData;
               if(sync_record_get(pd_.sync_time, &SyncData))
               {  // Found the sync data for this message

                  printSyncInfo(&SyncData);

                  // Calculate the phase angle from the sync data
                  uint16_t phaseAngle = CalcPhaseAngle(&SyncData);

                  if(ConfigAttr_.PD_SurveyMode != PD_SURVEY_MODE_DISABLED)
                  {
                     if (PhaseDetect_LogData( handle_id, phaseAngle))
                     {
                        INFO_printf("PD:Data Logged");
                     }
                  }
                  else{
                     INFO_printf("PD:Disabled");
                  }
               }
               else{
                  // No sync info was found
                  INFO_printf("PD:SyncData not found");
               }
            }
            else{
               ERR_printf("PD:Invalid parameter");
            }
         }
      }
   }
}

#endif  // ( PHASE_DETECTION == 1 )


